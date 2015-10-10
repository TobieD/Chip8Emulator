#include <iostream>
#include <chrono> //time stuff
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Chip8.h"
#include <sstream>

using namespace std;

// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void UpdateTexture(Chip8 * chip8);

//Constants
#pragma region Constants
//Window
const GLuint WINDOW_WIDTH = 800, WINDOW_HEIGHT = 600;
const std::string WINDOW_NAME = "Chip8 Emulator - Devries Tobie";

//Chip8
const int CHIP8_WIDTH = 64, CHIP8_HEIGHT = 32;
const float CLEAR_COLOR = 0.0f;

const string GAME = "Resources/CONNECT4";

#pragma endregion

//Shaders
#pragma region OpenGL Shaders
//Shader Code
const GLchar* vertexSource =
"#version 150 core\n"
"in vec2 position;"
"in vec2 texcoord;"

"out vec2 TexCoord;"

"void main() {"
"   gl_Position = vec4(position, 0.0, 1.0);"
"	TexCoord = texcoord;"

"}";

const GLchar* fragmentSource =
"#version 150 core\n"

"in vec2 TexCoord;"
"out vec4 outColor;"
"uniform sampler2D tex;"
"void main() {"
"   outColor = texture(tex,TexCoord);"
"}";
#pragma endregion 

unsigned char m_screenData[CHIP8_WIDTH*CHIP8_HEIGHT][3];
Chip8* m_chip8;

// The MAIN function, from here we start the application and run the game loop
int main()
{
	//Create OpenGL Window
	#pragma region OpenGL Window Creation

	std::cout << "Starting GLFW context, OpenGL 3.3" << std::endl;

	// Init GLFW
	glfwInit();

	// Set all the required options for GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	// Create a GLFWwindow object that we can use for GLFW's functions
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_NAME.c_str(), NULL, NULL);
	glfwMakeContextCurrent(window);
	if (!window)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	// Set the required callback functions
	glfwSetKeyCallback(window, key_callback);

	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
	{
		std::cout << "Failed to initialize OpenGL context" << std::endl;
		return -1;
	}

	// Define the viewport dimensions
	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	#pragma endregion 
	
	//2. Create Vertex info and index info and bind to buffers
	#pragma region Create Quad and bind to Buffers	

	//create Quad 
	float vertices[] = {
		//pos			//texCoord
		-1.f,   1.0f,	0.0f, 0.0f, // Top-left
		 1.f,	1.0f,	1.0f, 0.0f, // Top-right
		 1.f,  -1.0f,	1.0f, 1.0f, // Bottom-right
		-1.f,  -1.0f,	0.0f, 1.0f  // Bottom-left
	};
	 
	GLuint elements[] = {
		0, 1, 2,
		2, 3, 0
	};

	//Create vertex Attribute buffer
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	//bind vertex info to buffer
	GLuint vbo, ebo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//bind indices to an index buffer
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);
	
#pragma endregion 

	//3. Create Shaders
	#pragma region Shader Loading	
	
	//Create Vertex and Fragment shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, nullptr);
	glCompileShader(vertexShader);

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
	glCompileShader(fragmentShader);

	//Combine in shader program
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glBindFragDataLocation(shaderProgram, 0, "outColor");
	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);

	//4. Link vertex data to Shader
	GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), 0);

	GLint texCoordAttrib = glGetAttribLocation(shaderProgram, "texcoord");
	glEnableVertexAttribArray(texCoordAttrib);
	glVertexAttribPointer(texCoordAttrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
	
#pragma endregion 
	
	//4. Create black texture to draw m_chip8 on
	#pragma region OpenGL Texture Creation
	for (auto x = 0; x < CHIP8_WIDTH * CHIP8_HEIGHT; x++)
		m_screenData[x][0] = m_screenData[x][1] = m_screenData[x][2] = m_screenData[x][3] = 0; 

	//create openGL texture
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CHIP8_WIDTH, CHIP8_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_screenData);

	//set to nearest for per pixel
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glEnable(GL_TEXTURE_2D);
	#pragma endregion 

	//5. Create m_chip8 Object and load a game	
	m_chip8 = new Chip8(CHIP8_WIDTH, CHIP8_HEIGHT);
	m_chip8->Initialize();
	m_chip8->LoadGame(GAME.c_str());

	auto t_start = std::chrono::high_resolution_clock::now();

	// Game loop
	while (!glfwWindowShouldClose(window))
	{
		//get current time
		auto t_now = std::chrono::high_resolution_clock::now();
		auto elapsedTime = std::chrono::duration_cast<std::chrono::duration<float>>(t_now - t_start).count();

		UNREFERENCED_PARAMETER(elapsedTime);

		// Check if any events have been activated (key pressed, mouse moved etc.) and call corresponding response functions
		glfwPollEvents();

		//Update
		if(m_chip8->shouldDraw())
			UpdateTexture(m_chip8);

		m_chip8->Run();

		// Render
		// Clear the colorbuffer
		glClearColor(CLEAR_COLOR, CLEAR_COLOR, CLEAR_COLOR,1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		//Draw elements
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// Swap the screen buffers
		glfwSwapBuffers(window);
	}

	//clean up m_chip8;
	delete m_chip8;

	// Terminates GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();

	glDeleteProgram(shaderProgram);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	glDeleteBuffers(1,&ebo);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	
	return 0;
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	UNREFERENCED_PARAMETER(action);
	UNREFERENCED_PARAMETER(mode);
	UNREFERENCED_PARAMETER(scancode);

	//std::cout << key << std::endl;
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

//Copy the chip8 ScreenData to the openGL texture
void UpdateTexture(Chip8 * chip8)
{	
	for (auto x = 0; x < CHIP8_WIDTH * CHIP8_HEIGHT; x++)
		m_screenData[x][0] = m_screenData[x][1] = m_screenData[x][2] = chip8->m_Screen[x] == 1 ? 225 : 35;

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, CHIP8_WIDTH, CHIP8_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, m_screenData);
}