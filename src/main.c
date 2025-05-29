
#include <Windows.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define VIDEO_WIDTH 64
#define VIDEO_HEIGHT 32

typedef struct
{
	uint8_t registers[16];
	uint8_t memory[4096];
	uint8_t sp;
	uint8_t delay_timer;
	uint8_t sound_timer;
	uint8_t keypad[16];
	uint16_t stack[16];
	uint16_t pc;
	uint16_t index;
	uint32_t video[64 * 32];
} Chip_8;

static Chip_8 chip_8;
static uint16_t op_code;
static GLFWwindow* window = NULL;
static GLuint shader_id;

static uint8_t fontset[80] =
{
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

static bool Shader_checkCompileErrors(unsigned int p_object, const char* p_type)
{
	int success;
	char infoLog[1024];
	if (p_type != "PROGRAM")
	{
		glGetShaderiv(p_object, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(p_object, 1024, NULL, infoLog);
			printf("Failed to Compile %s Shader. Reason: %s", p_type, infoLog);

			return false;
		}
	}
	else
	{
		glGetProgramiv(p_object, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(p_object, 1024, NULL, infoLog);
			printf("Failed to Compile %s Shader. Reason: %s", p_type, infoLog);
			return false;
		}
	}
	return true;
}

static int File_GetLength(FILE* p_file)
{
	int pos;
	int end;

	pos = ftell(p_file);
	fseek(p_file, 0, SEEK_END);
	end = ftell(p_file);
	fseek(p_file, pos, SEEK_SET);

	return end;
}

static unsigned char* File_Parse(const char* p_filePath, int* r_length)
{
	FILE* file = NULL;

	fopen_s(&file, p_filePath, "rb"); //use rb because otherwise it can cause reading issues
	if (!file)
	{
		printf("Failed to open file for parsing at path: %s!\n", p_filePath);
		return NULL;
	}
	int file_length = File_GetLength(file);
	unsigned char* buffer = malloc(file_length + 1);
	if (!buffer)
	{
		return NULL;
	}
	memset(buffer, 0, file_length + 1);
	fread_s(buffer, file_length, 1, file_length, file);

	//CLEAN UP
	fclose(file);

	if (r_length)
	{
		*r_length = file_length;
	}

	return buffer;
}

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	switch (key)
	{
	case GLFW_KEY_X:
	{
		chip_8.keypad[0] = (uint8_t)action;
		break;
	}
	case GLFW_KEY_1:
	{
		chip_8.keypad[1] = (uint8_t)action;
		break;
	}
	case GLFW_KEY_2:
	{
		chip_8.keypad[2] = (uint8_t)action;
		break;
	}
	case GLFW_KEY_3:
	{
		chip_8.keypad[3] = (uint8_t)action;
		break;
	}
	case GLFW_KEY_Q:
	{
		chip_8.keypad[4] = (uint8_t)action;
		break;
	}
	case GLFW_KEY_W:
	{
		chip_8.keypad[5] = (uint8_t)action;
		break;
	}
	case GLFW_KEY_E:
	{
		chip_8.keypad[6] = (uint8_t)action;
		break;
	}
	case GLFW_KEY_A:
	{
		chip_8.keypad[7] = (uint8_t)action;
		break;
	}
	case GLFW_KEY_S:
	{
		chip_8.keypad[8] = (uint8_t)action;
		break;
	}
	case GLFW_KEY_D:
	{
		chip_8.keypad[9] = (uint8_t)action;
		break;
	}
	case GLFW_KEY_Z:
	{
		chip_8.keypad[10] = (uint8_t)action;
		break;
	}
	case GLFW_KEY_C:
	{
		chip_8.keypad[11] = (uint8_t)action;
		break;
	}
	case GLFW_KEY_4:
	{
		chip_8.keypad[12] = (uint8_t)action;
		break;
	}
	case GLFW_KEY_R:
	{
		chip_8.keypad[13] = (uint8_t)action;
		break;
	}
	case GLFW_KEY_F:
	{
		chip_8.keypad[14] = (uint8_t)action;
		break;
	}
	case GLFW_KEY_V:
	{
		chip_8.keypad[15] = (uint8_t)action;
		break;
	}
	default:
		break;
	}
}
static void WindowCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

static void OP_00E0()
{
	memset(chip_8.video, 0, sizeof(chip_8.video));
}
static void OP_00EE()
{
	chip_8.sp--;
	chip_8.pc = chip_8.stack[chip_8.sp];
}

static void OP_0x0()
{
	uint16_t op = op_code & 0x000Fu;

	if (op == 0x0)
	{
		OP_00E0();
	}
	else if (op == 0xE)
	{
		OP_00EE();
	}
	else
	{
		printf("missing instruction 0x0 \n");
		exit(0);
	}
}
static void OP_0x1()
{
	uint16_t address = op_code & 0x0FFFu;

	chip_8.pc = address;
}
static void OP_0x2()
{
	uint16_t address = op_code & 0x0FFFu;

	chip_8.stack[chip_8.sp] = chip_8.pc;
	chip_8.sp++;
	chip_8.pc = address;
}
static void OP_0x3()
{
	uint8_t Vx = (op_code & 0x0F00u) >> 8u;
	uint8_t kk = op_code & 0x00FFu;

	if (chip_8.registers[Vx] == kk)
	{
		chip_8.pc += 2;
	}
}
static void OP_0x4()
{
	uint8_t Vx = (op_code & 0x0F00u) >> 8u;
	uint8_t kk = op_code & 0x00FFu;

	if (chip_8.registers[Vx] != kk)
	{
		chip_8.pc += 2;
	}
}
static void OP_0x5()
{
	uint8_t Vx = (op_code & 0x0F00u) >> 8u;
	uint8_t Vy = (op_code & 0x00F0u) >> 4u;

	if (chip_8.registers[Vx] == chip_8.registers[Vy])
	{
		chip_8.pc += 2;
	}
}
static void OP_0x6()
{
	uint8_t Vx = (op_code & 0x0F00u) >> 8u;
	uint8_t kk = op_code & 0x00FFu;

	chip_8.registers[Vx] = kk;
}
static void OP_0x7()
{
	uint8_t Vx = (op_code & 0x0F00u) >> 8u;
	uint8_t kk = op_code & 0x00FFu;

	chip_8.registers[Vx] += kk;
}

static void OP_8x0()
{
	uint8_t Vx = (op_code & 0x0F00u) >> 8u;
	uint8_t Vy = (op_code & 0x00F0u) >> 4u;

	chip_8.registers[Vx] = chip_8.registers[Vy];
}
static void OP_8x1()
{
	uint8_t Vx = (op_code & 0x0F00u) >> 8u;
	uint8_t Vy = (op_code & 0x00F0u) >> 4u;

	chip_8.registers[Vx] |= chip_8.registers[Vy];
}
static void OP_8x2()
{
	uint8_t Vx = (op_code & 0x0F00u) >> 8u;
	uint8_t Vy = (op_code & 0x00F0u) >> 4u;

	chip_8.registers[Vx] &= chip_8.registers[Vy];
}
static void OP_8x3()
{
	uint8_t Vx = (op_code & 0x0F00u) >> 8u;
	uint8_t Vy = (op_code & 0x00F0u) >> 4u;

	chip_8.registers[Vx] ^= chip_8.registers[Vy];
}
static void OP_8x4()
{
	uint8_t Vx = (op_code & 0x0F00u) >> 8u;
	uint8_t Vy = (op_code & 0x00F0u) >> 4u;

	uint16_t total = chip_8.registers[Vx] + chip_8.registers[Vy];

	if (total > 255u)
	{
		chip_8.registers[0xF] = 1;
	}
	else
	{
		chip_8.registers[0xF] = 0;
	}

	chip_8.registers[Vx] = total & 255u;
}
static void OP_8x5()
{
	uint8_t Vx = (op_code & 0x0F00u) >> 8u;
	uint8_t Vy = (op_code & 0x00F0u) >> 4u;

	if (chip_8.registers[Vx] > chip_8.registers[Vy])
	{
		chip_8.registers[0xF] = 1;
	}
	else
	{
		chip_8.registers[0xF] = 0;
	}

	chip_8.registers[Vx] -= chip_8.registers[Vy];
}
static void OP_8x6()
{
	uint8_t Vx = (op_code & 0x0F00u) >> 8u;
	
	chip_8.registers[0xF] = (chip_8.registers[Vx] & 0x1u);

	chip_8.registers[Vx] >>= 1u;
}
static void OP_8x7()
{
	uint8_t Vx = (op_code & 0x0F00u) >> 8u;
	uint8_t Vy = (op_code & 0x00F0u) >> 4u;

	if (chip_8.registers[Vy] > chip_8.registers[Vx])
	{
		chip_8.registers[0xF] = 1;
	}
	else
	{
		chip_8.registers[0xF] = 0;
	}

	chip_8.registers[Vx] = chip_8.registers[Vy] - chip_8.registers[Vx];

}
static void OP_8xE()
{
	uint8_t Vx = (op_code & 0x0F00u) >> 8u;

	chip_8.registers[0xF] = (chip_8.registers[Vx] & 0x80u) >> 7u;

	chip_8.registers[Vx] <<= 1u;
	
}
static void OP_0x8()
{
	uint16_t op = op_code & 0x000Fu;

	switch (op)
	{
	case 0x0:
	{
		OP_8x0();
		break;
	}
	case 0x1:
	{
		OP_8x1();
		break;
	}
	case 0x2:
	{
		OP_8x2();
		break;
	}
	case 0x3:
	{
		OP_8x3();
		break;
	}
	case 0x4:
	{
		OP_8x4();
		break;
	}
	case 0x5:
	{
		OP_8x5();
		break;
	}
	case 0x6:
	{
		OP_8x6();
		break;
	}
	case 0x7:
	{
		OP_8x7();
		break;
	}
	case 0xE:
	{
		OP_8xE();
		break;
	}
	default:
	{
		printf("missing instruction 0x8 \n");
		exit(0);
		break;
	}
	}
}
static void OP_0x9()
{
	uint8_t Vx = (op_code & 0x0F00u) >> 8u;
	uint8_t Vy = (op_code & 0x00F0u) >> 4u;

	if (chip_8.registers[Vx] != chip_8.registers[Vy])
	{
		chip_8.pc += 2;
	}
}
static void OP_0xA()
{
	uint16_t address = op_code & 0x0FFFu;

	chip_8.index = address;
}
static void OP_0xB()
{
	uint16_t address = op_code & 0x0FFFu;

	chip_8.pc = chip_8.registers[0] + address;
}
static void OP_0xC()
{
	uint8_t Vx = (op_code & 0x0F00u) >> 8u;
	uint8_t byte = op_code & 0x00FFu;

	chip_8.registers[Vx] = (rand() % 255) & byte;
}
static void OP_0xD()
{
	uint8_t Vx = (op_code & 0x0F00u) >> 8u;
	uint8_t Vy = (op_code & 0x00F0u) >> 4u;
	uint8_t height = op_code & 0x000Fu;

	uint8_t x_pos = chip_8.registers[Vx] % VIDEO_WIDTH;
	uint8_t y_pos = chip_8.registers[Vy] % VIDEO_HEIGHT;

	chip_8.registers[0xF] = 0;

	for (unsigned row = 0; row < height; row++)
	{
		uint8_t sprite_byte = chip_8.memory[chip_8.index + row];

		for (unsigned col = 0; col < 8; col++)
		{
			uint8_t sprite_pixel = sprite_byte & (0x80u >> col);
			uint32_t* screen_pixel = &chip_8.video[(y_pos + row) * VIDEO_WIDTH + (x_pos + col)];

			if (sprite_pixel)	
			{
				if (*screen_pixel == 0xFFFFFFFF)
				{
					chip_8.registers[0xF] = 1;
				}

				*screen_pixel ^= 0xFFFFFFFF;
			}
		}
	}
}
static void OP_ExA1()
{
	uint8_t Vx = (op_code & 0x0F00u) >> 8u;
	uint8_t key = chip_8.registers[Vx];

	if (chip_8.keypad[key] == 0)
	{
		chip_8.pc += 2;
	}
}
static void OP_ExE()
{
	uint8_t Vx = (op_code & 0x0F00u) >> 8u;
	uint8_t key = chip_8.registers[Vx];

	if (chip_8.keypad[key] == 1)
	{
		chip_8.pc += 2;
	}
}
static void OP_0xE()
{
	uint16_t op = op_code & 0x000Fu;
	switch (op)
	{
	case 0x1:
	{
		OP_ExA1();
		break;
	}
	case 0xE:
	{
		OP_ExE();
		break;
	}
	default:
	{
		printf("missing instruction 0xE \n");
		exit(0);
		break;
	}
	}
}

static void OP_Fx07()
{
	uint8_t Vx = (op_code & 0x0F00u) >> 8u;
	
	chip_8.registers[Vx] = chip_8.delay_timer;
}
static void OP_Fx0A()
{
	uint8_t Vx = (op_code & 0x0F00u) >> 8u;

	for (int i = 0; i < 16; i++)
	{
		if (chip_8.keypad[i] == 1)
		{
			chip_8.registers[Vx] = (uint8_t)i;
			return;
		}
	}

	chip_8.pc -= 2;
}
static void OP_Fx15()
{		 
	uint8_t Vx = (op_code & 0x0F00u) >> 8u;

	chip_8.delay_timer = chip_8.registers[Vx];
}		 
static void OP_Fx18()
{
	uint8_t Vx = (op_code & 0x0F00u) >> 8u;

	chip_8.sound_timer = chip_8.registers[Vx];
}
static void OP_Fx1E()
{
	uint8_t Vx = (op_code & 0x0F00u) >> 8u;

	chip_8.index += chip_8.registers[Vx];
}
static void OP_Fx29()
{
	uint8_t Vx = (op_code & 0x0F00u) >> 8u;
	uint8_t digit = chip_8.registers[Vx];

	chip_8.index = 0x50 + (5 * digit);
}
static void OP_Fx33()
{
	uint8_t Vx = (op_code & 0x0F00u) >> 8u;
	uint8_t value = chip_8.registers[Vx];

	chip_8.memory[chip_8.index + 2] = value % 10;
	value /= 10;

	chip_8.memory[chip_8.index + 1] = value % 10;
	value /= 10;

	chip_8.memory[chip_8.index] = value % 10;
}
static void OP_Fx55()
{
	uint8_t Vx = (op_code & 0x0F00u) >> 8u;
	
	for (uint8_t i = 0; i <= Vx; i++)
	{
		chip_8.memory[chip_8.index + i] = chip_8.registers[i];
	}
}
static void OP_Fx65()
{
	uint8_t Vx = (op_code & 0x0F00u) >> 8u;

	for (uint8_t i = 0; i <= Vx; i++)
	{
		chip_8.registers[i] = chip_8.memory[chip_8.index + i];
	}
}

static void OP_0xF()
{
	uint16_t op = op_code & 0x00FFu;

	switch (op)
	{
	case 0x07:
	{
		OP_Fx07();
		break;
	}
	case 0x0A:
	{
		OP_Fx0A();
		break;
	}
	case 0x15:
	{
		OP_Fx15();
		break;
	}
	case 0x18:
	{
		OP_Fx18();
		break;
	}
	case 0x1E:
	{
		OP_Fx1E();
		break;
	}
	case 0x29:
	{
		OP_Fx29();
		break;
	}
	case 0x33:
	{
		OP_Fx33();
		break;
	}
	case 0x55:
	{
		OP_Fx55();
		break;
	}
	case 0x65:
	{
		OP_Fx65();
		break;
	}
	default:
	{
		printf("missing instruction 0xF \n");
		exit(0);
		break;
	}
	}
}

int main(int argc, char** argv)
{
	srand(time(NULL));
	memset(&chip_8, 0, sizeof(chip_8));

	const char* filepath = NULL;
	int delta_delay = 1;
	DWORD beep_freq = 300;
	DWORD beep_duration = 100;

	//read args
	for (int i = 0; i < argc; i++)
	{
		if (!strcmp(argv[i], "-path"))
		{
			filepath = argv[i + 1];
		}
		else if (!strcmp(argv[i], "-delta"))
		{
			delta_delay = atoi(argv[i + 1]);

			if (delta_delay < 0)
			{
				delta_delay = 0;
			}
			else if (delta_delay > 6)
			{
				delta_delay = 6;
			}
		}
		else if (!strcmp(argv[i], "-beep_freq"))
		{
			beep_freq = atol(argv[i + 1]);

			if (beep_freq < 37)
			{
				beep_freq = 37;
			}
			else if (beep_freq > 32767)
			{
				beep_freq = 32767;
			}
		}
		else if (!strcmp(argv[i], "-beep_duration"))
		{
			beep_duration = atol(argv[i + 1]);
		}
	}

	if (!filepath)
	{
		printf("No rom specified! Use -path to specify rom.");
		return -1;
	}

	//read rom file
	int len = 0;
	char* buffer = File_Parse(filepath, &len);

	if (!buffer)
	{
		printf("Failed to load rom!");
		return -1;
	}

	if (len > 4096)
	{
		printf("Rom file too large!");
		free(buffer);
		return -1;
	}

	//load into memory
	memcpy(chip_8.memory + 0x200, buffer, sizeof(uint8_t) * len);

	free(buffer);

	//set pounter counter to start
	chip_8.pc = 0x200;

	//load fonts
	memcpy(chip_8.memory + 0x50, fontset, sizeof(fontset));

	//load glfw
	if (!glfwInit())
	{
		printf("Failed to load glfw!");
		return -1;
	}

	int window_width = VIDEO_WIDTH;
	int window_height = VIDEO_HEIGHT;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(window_width, window_height, "Chippy-8", NULL, NULL);

	if (!window)
	{
		printf("Failed to create window!");
		glfwTerminate();
		return -1;
	}
	
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetWindowSizeCallback(window, WindowCallback);

	//load glad
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("Failed to init GLAD\n");
		return -1;
	}

	printf("GLAD Loaded\n");
	printf("OpenGL loaded\n");
	printf("Vendor:   %s\n", glGetString(GL_VENDOR));
	printf("Renderer: %s\n", glGetString(GL_RENDERER));
	printf("Version:  %s\n", glGetString(GL_VERSION));

	//setup shaders
	char* vert_src = NULL;
	char* frag_src = NULL;

	vert_src = File_Parse("shader.vert", NULL);

	if (!vert_src)
	{
		printf("Failed to load vertex shader");
		return -1;
	}

	frag_src = File_Parse("shader.frag", NULL);
	
	if (!frag_src)
	{
		free(vert_src);
		printf("Failed to load fragment shader");
		return -1;
	}

	{
		GLuint vertex_id, frag_id;
		vertex_id = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex_id, 1, &vert_src, NULL);
		glCompileShader(vertex_id);

		if (!Shader_checkCompileErrors(vertex_id, "Vertex"))
		{
			return -1;
		}
		frag_id = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(frag_id, 1, &frag_src, NULL);
		glCompileShader(frag_id);

		if (!Shader_checkCompileErrors(frag_id, "Fragment"))
		{
			return -1;
		}

		shader_id = glCreateProgram();
		glAttachShader(shader_id, vertex_id);
		glAttachShader(shader_id, frag_id);

		glLinkProgram(shader_id);
		if (!Shader_checkCompileErrors(shader_id, "Program"))
			return -1;

		glDeleteShader(vertex_id);
		glDeleteShader(frag_id);

		free(vert_src);
		free(frag_src);
	}
	
	//setup main screen texture
	GLuint texture = 0;
	glGenTextures(1, &texture);

    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, VIDEO_WIDTH, VIDEO_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	//setup quad vao and vbo
	unsigned vao = 0;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	unsigned vbo = 0;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	const float quad_vertices[16] =
	{		-1, -1, 0, 0,
			-1,  1, 0, 1,
			1,  1, 1, 1,
			1, -1, 1, 0,
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));
	glEnableVertexAttribArray(1);
	
	//setup gl state
	glUseProgram(shader_id);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	
	glfwGetWindowSize(window, &window_width, &window_height);
	glViewport(0, 0, window_width, window_height);

	glBindTexture(GL_TEXTURE_2D, texture);
	
	float lastTime = glfwGetTime();

	while (!glfwWindowShouldClose(window))
	{
		float current_time = glfwGetTime();
		float dt = current_time - lastTime;

		if (dt > delta_delay)
		{
			lastTime = dt;
		
			op_code = (chip_8.memory[chip_8.pc] << 8u) | chip_8.memory[chip_8.pc + 1];

			chip_8.pc += 2;

			uint16_t op = (op_code & 0xF000u) >> 12u;

			if (chip_8.delay_timer > 0)
			{
				chip_8.delay_timer--;
			}
			if (chip_8.sound_timer > 0)
			{
				chip_8.sound_timer--;
				Beep(beep_freq, beep_duration);
			}

			switch (op)
			{
			case 0x0:
			{
				OP_0x0();
				break;
			}
			case 0x1:
			{
				OP_0x1();
				break;
			}
			case 0x2:
			{
				OP_0x2();
				break;
			}
			case 0x3:
			{
				OP_0x3();
				break;
			}
			case 0x4:
			{
				OP_0x4();
				break;
			}
			case 0x5:
			{
				OP_0x5();
				break;
			}
			case 0x6:
			{
				OP_0x6();
				break;
			}
			case 0x7:
			{
				OP_0x7();
				break;
			}
			case 0x8:
			{
				OP_0x8();
				break;
			}
			case 0x9:
			{
				OP_0x9();
				break;
			}
			case 0xA:
			{
				OP_0xA();
				break;
			}
			case 0xB:
			{
				OP_0xB();
				break;
			}
			case 0xC:
			{
				OP_0xC();
				break;
			}
			case 0xD:
			{
				OP_0xD();
				break;
			}
			case 0xE:
			{
				OP_0xE();
				break;
			}
			case 0xF:
			{
				OP_0xF();
				break;
			}
			default:
				printf("missing instruction core \n");
				exit(0);
				break;
			}

			//upload video bytes and render quad
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, VIDEO_WIDTH, VIDEO_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, chip_8.video);
			glClear(GL_COLOR_BUFFER_BIT);
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}