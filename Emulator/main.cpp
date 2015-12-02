#include <iostream>
#include <sstream>

//OpenGL includes
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Chip8.h"


using namespace std;

//Logger helpers
class LogBuf: public stringbuf
{
protected:
	int sync()
	{
		OutputDebugString(str().c_str());
		cout << str();
		str("");
		return 0;
	}
};

// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void drop_callback(GLFWwindow* window, int amount, const char** files);
void UpdateTexture(Chip8 * chip8);
void ResetChip8();
string GetWindowTitle();

//Constants	
#pragma region Constants
//Window
const string WINDOW_NAME = "Chip8 Emulator - Devries Tobie";

//Chip8
const float CLEAR_COLOR = 0.0f;
string GAME = "Resources\\TETRIS";
bool bInvertColors = false;

const int BLACKCOLOR = 50;
const int WHITECOLOR = 215;
const int UPSCALE_FACTOR = 20;

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
"	outColor = texture(tex,TexCoord);"
"}";
#pragma endregion 

//Size of Chip8 screen + 3 channels(RGB)
unsigned char m_screenData[Chip8::WIDTH*Chip8::HEIGHT][3];
Chip8* m_chip8;
GLFWwindow* m_Window;

// The MAIN function, from here we start the application and run the game loop
int main()
{	
	//Overwrite clog buffer
	LogBuf log;
	cerr.rdbuf(&log);

	//1. Create OpenGL Window
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
	m_Window = glfwCreateWindow(Chip8::WIDTH * UPSCALE_FACTOR, Chip8::HEIGHT *UPSCALE_FACTOR, GetWindowTitle().c_str(), NULL, NULL);
	glfwMakeContextCurrent(m_Window);
	if (!m_Window)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	// Set the required callback functions
	glfwSetKeyCallback(m_Window, key_callback);
	glfwSetDropCallback(m_Window, drop_callback);

	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
	{
		std::cout << "Failed to initialize OpenGL context" << std::endl;
		return -1;
	}

	// Define the viewport dimensions
	glViewport(0, 0, Chip8::WIDTH*UPSCALE_FACTOR, Chip8::HEIGHT*UPSCALE_FACTOR);
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

	//3. Create Shader
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
	for (auto x = 0; x < Chip8::WIDTH * Chip8::HEIGHT; x++)
		m_screenData[x][0] = m_screenData[x][1] = m_screenData[x][2] = m_screenData[x][3] = 0; 

	//create openGL texture
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Chip8::WIDTH, Chip8::HEIGHT , 0, GL_RGBA, GL_UNSIGNED_BYTE, m_screenData);

	//set to nearest for per pixel
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glEnable(GL_TEXTURE_2D);
	#pragma endregion 

	//5. Create m_chip8 Object and load a game	
	m_chip8 = new Chip8();
	m_chip8->LoadGame(GAME.c_str());

	// Game loop
	while (!glfwWindowShouldClose(m_Window))
	{
		// Check if any events have been activated (key pressed, mouse moved etc.) and call corresponding response functions
		glfwPollEvents();

		//Update texture when draw flag is set(optimizes performance)
		if (m_chip8->shouldDraw())
		{
			UpdateTexture(m_chip8);
		}

		//run chip8 
		m_chip8->Run();			

		// Render
		// Clear the color buffer
		glClearColor(CLEAR_COLOR, CLEAR_COLOR, CLEAR_COLOR,1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		//Draw the quad
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// Swap the screen buffers
		glfwSwapInterval(1);
		glfwSwapBuffers(m_Window);
	}

	//clean up m_chip8;
	delete m_chip8;

	// Terminates GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();

	//clean up program
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

	//Chip8 keys (0 == released, 1 == Pressed)

	//Chip 8 specific input
	//1, 2 , 3 , C
	if (key == GLFW_KEY_1 && action == GLFW_RELEASE) m_chip8->PressKey(1, 0);
	else if (key == GLFW_KEY_2 && action == GLFW_RELEASE) m_chip8->PressKey(2, 0);
	else if (key == GLFW_KEY_3 && action == GLFW_RELEASE) m_chip8->PressKey(3, 0);
	else if (key == GLFW_KEY_4 && action == GLFW_RELEASE) m_chip8->PressKey(12, 0);


	//4,5,6,
	else if (key == GLFW_KEY_Q && action == GLFW_RELEASE) m_chip8->PressKey(4, 0);
	else if (key == GLFW_KEY_W && action == GLFW_RELEASE) m_chip8->PressKey(5, 0);
	else if (key == GLFW_KEY_E && action == GLFW_RELEASE) m_chip8->PressKey(6, 0);
	else if (key == GLFW_KEY_R && action == GLFW_RELEASE) m_chip8->PressKey(13, 0);

	//7,8,9,E
	else if (key == GLFW_KEY_A && action == GLFW_RELEASE) m_chip8->PressKey(7, 0);
	else if (key == GLFW_KEY_S && action == GLFW_RELEASE) m_chip8->PressKey(8, 0);
	else if (key == GLFW_KEY_D && action == GLFW_RELEASE) m_chip8->PressKey(9, 0);
	else if (key == GLFW_KEY_F && action == GLFW_RELEASE) m_chip8->PressKey(14, 0);

	//A, 0, B, F
	else if (key == GLFW_KEY_Z && action == GLFW_RELEASE) m_chip8->PressKey(10, 0);
	else if (key == GLFW_KEY_X && action == GLFW_RELEASE) m_chip8->PressKey(0, 0);
	else if (key == GLFW_KEY_C && action == GLFW_RELEASE) m_chip8->PressKey(11, 0);
	else if (key == GLFW_KEY_V && action == GLFW_RELEASE) m_chip8->PressKey(15, 0);


	//PRESSED
	//1, 2 , 3 , C
	if (key == GLFW_KEY_1 && action == GLFW_PRESS) m_chip8->PressKey(1, 1);
	else if (key == GLFW_KEY_2 && action == GLFW_PRESS) m_chip8->PressKey(2, 1);
	else if (key == GLFW_KEY_3 && action == GLFW_PRESS) m_chip8->PressKey(3, 1);
	else if (key == GLFW_KEY_4 && action == GLFW_PRESS) m_chip8->PressKey(12, 1);


	//4,5,6,
	else if (key == GLFW_KEY_Q && action == GLFW_PRESS) m_chip8->PressKey(4, 1);
	else if (key == GLFW_KEY_W && action == GLFW_PRESS) m_chip8->PressKey(5, 1);
	else if (key == GLFW_KEY_E && action == GLFW_PRESS) m_chip8->PressKey(6, 1);
	else if (key == GLFW_KEY_R && action == GLFW_PRESS) m_chip8->PressKey(13, 1);

	//7,8,9,E
	else if (key == GLFW_KEY_A && action == GLFW_PRESS) m_chip8->PressKey(7, 1);
	else if (key == GLFW_KEY_S && action == GLFW_PRESS) m_chip8->PressKey(8, 1);
	else if (key == GLFW_KEY_D && action == GLFW_PRESS) m_chip8->PressKey(9, 1);
	else if (key == GLFW_KEY_F && action == GLFW_PRESS) m_chip8->PressKey(14, 1);

	//A, 0, B, F
	else if (key == GLFW_KEY_Z && action == GLFW_PRESS) m_chip8->PressKey(10, 1);
	else if (key == GLFW_KEY_X && action == GLFW_PRESS) m_chip8->PressKey(0, 1);
	else if (key == GLFW_KEY_C && action == GLFW_PRESS) m_chip8->PressKey(11, 1);
	else if (key == GLFW_KEY_V && action == GLFW_PRESS) m_chip8->PressKey(15, 1);

	//exit
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	//reset current Game
	if (key == GLFW_KEY_T && action == GLFW_PRESS)
	{
		ResetChip8();
	}

	//Pause 
	if (key == GLFW_KEY_P && action == GLFW_PRESS)
	{
		m_chip8->Pause();
		glfwSetWindowTitle(m_Window, GetWindowTitle().c_str());
	}


	//speed up game
	if (key == GLFW_KEY_KP_ADD)
	{
		m_chip8->AdjustSpeed(1);
		glfwSetWindowTitle(m_Window, GetWindowTitle().c_str());
	}

	//Slow down game
	if (key == GLFW_KEY_KP_SUBTRACT)
	{
		m_chip8->AdjustSpeed(-1);
		glfwSetWindowTitle(m_Window, GetWindowTitle().c_str());
	}

	//Invert colors of the chip8
	if (key == GLFW_KEY_I && action == GLFW_PRESS)
	{
		bInvertColors = !bInvertColors;
		UpdateTexture(m_chip8);
	}
}

//called whenever a file gets dropped on the window
void drop_callback(GLFWwindow* window,int amount, const char** files)
{
	UNREFERENCED_PARAMETER(window);
	UNREFERENCED_PARAMETER(amount);

	GAME = files[0]; //get first file
	ResetChip8();
	glfwSetWindowTitle(m_Window, GetWindowTitle().c_str());
}

//Copy the chip8 ScreenData to the openGL texture
void UpdateTexture(Chip8 * chip8)
{	
	const U8* graphics = chip8->GetScreenData();
	//Set RGB channel of texture
	for (auto x = 0; x < Chip8::WIDTH * Chip8::HEIGHT; x++)
	{
		unsigned char w = WHITECOLOR;
		unsigned char b = BLACKCOLOR;

		if(bInvertColors)
		{
			unsigned char t = w;
			w = b;
			b = t;
		}

		m_screenData[x][0] = m_screenData[x][1] = m_screenData[x][2] = graphics[x] == 1 ? w : b;
	}

	//create the new texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Chip8::WIDTH, Chip8::HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, m_screenData);
}

void ResetChip8()
{
	m_chip8->LoadGame(GAME.c_str());
	UpdateTexture(m_chip8); //clear screen
}

string  GetWindowTitle()
{
	int speed = (m_chip8)?m_chip8->GetRunSpeed():1;
	int pos = GAME.find_last_of('\\');
	string mode = "OFF";
	
	if (m_chip8)
	{
		mode = m_chip8->GetCompatibilityMode() ? "ON" : "OFF";
	}

	string spd = (speed > 0) ?to_string(speed) : "[PAUSED]";
	
	return WINDOW_NAME + " - " + GAME.substr(pos + 1) + " - " + spd + " [Compatibility mode: " + mode + "]";
}