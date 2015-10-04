#include <iostream>
#include <chrono> //time stuff
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Chip8.h"

// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

#pragma region Constants
const GLuint WIDTH = 800, HEIGHT = 600;
const std::string WINDOW_NAME = "Chip8 Emulator - Devries Tobie";

#pragma endregion

//Shaders
#pragma region OpenGL Shaders

#pragma endregion 

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
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, WINDOW_NAME.c_str(), NULL, NULL);
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
	glViewport(0, 0, WIDTH, HEIGHT);
	#pragma endregion 
	
	//0. Create Vertex Attribute Buffer
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	//1. Create Vertex info and index info and bind to buffers
	#pragma region Create Quad and bind to Buffers

	//1. create Quad and bind it to a vertex buffer
	float vertices[] = {
		-0.5f,  0.5f, 1.0f, 0.0f, 0.0f, // Top-left
		0.5f,  0.5f, 0.0f, 1.0f, 0.0f, // Top-right
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, // Bottom-right
		-0.5f, -0.5f, 1.0f, 1.0f, 1.0f  // Bottom-left
	};

	GLuint elements[] = {
		0, 1, 2,
		2, 3, 0
	};

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

	//Shader Code
	const GLchar* vertexSource =
		"#version 150 core\n"
		"in vec2 position;"
		"in vec3 color;"
		"out vec3 Color;"
		"void main() {"
		"   gl_Position = vec4(position, 0.0, 1.0);"
		"   Color = color;"

		"}";

	const GLchar* fragmentSource =
		"#version 150 core\n"
		"uniform vec3 triangleColor;"
		"in vec3 Color;"
		"out vec4 outColor;"
		"void main() {"
		"   outColor = vec4(Color, 1.0);"
		"}";

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
	//glBindFragDataLocation(shaderProgram, 0, "outColor");
	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);

	//no more need for the shaders
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	//4. Link vertex data to Shader
	GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), nullptr);
	glEnableVertexAttribArray(posAttrib);

	GLint colorAttrib = glGetAttribLocation(shaderProgram, "color");
	glVertexAttribPointer(colorAttrib, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(colorAttrib);
#pragma endregion 

	//Create Chip8 Object and load a game
	Chip8* chip8 = new Chip8();
	chip8->Initialize();
	chip8->LoadGame("Resources/MAZE");

	auto t_start = std::chrono::high_resolution_clock::now();

	// Game loop
	while (!glfwWindowShouldClose(window))
	{
		//get current time
		auto t_now = std::chrono::high_resolution_clock::now();
		float elapsedTime = std::chrono::duration_cast<std::chrono::duration<float>>(t_now - t_start).count();

		UNREFERENCED_PARAMETER(elapsedTime);

		// Check if any events have been activated (key pressed, mouse moved etc.) and call corresponding response functions
		glfwPollEvents();

		//Update
		chip8->Run();

		// Render
		// Clear the colorbuffer
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		//Draw elements
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// Swap the screen buffers
		glfwSwapBuffers(window);
	}

	//clean up chip8;
	delete chip8;

	// Terminates GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();
	return 0;
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	UNREFERENCED_PARAMETER(action);
	UNREFERENCED_PARAMETER(mode);
	UNREFERENCED_PARAMETER(scancode);

	std::cout << key << std::endl;
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}