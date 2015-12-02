#include "Chip8.h"
#include <fstream>
#include <iostream>
#include <chrono> //for random
#include <sstream>

#include "Helpers.h"

//#define LOGREGISTER
//#define LOGOPCODE

using namespace std;

//Constructor
Chip8::Chip8() :
	m_RunSpeed(1),
	m_MemoryPosition(0x200),
	m_StackIndex(0),
	m_RegisterIndex(0),
	m_bGameLoaded(false),
	m_bShouldDraw(false),
	m_bEnableCompatibility(false),
	m_bEnableScreenWrap(false),
	m_bPaused(false),
	m_RunSpeedBeforePause(0)
{

}

//Destructor
Chip8::~Chip8()
{

}

//Chip8 logic
void Chip8::Initialize()
{
	//reset
	m_MemoryPosition = 0x200; //first 512 bytes are reserved
	m_Opcode = 0;
	m_RegisterIndex = 0;
	m_StackIndex = 0;

	//load font set
	U8 fonts[80] =
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

	//clear memory
	for (int i = 0; i < 4096; ++i)
	{
		m_Memory[i] = 0;
	}

	//load fonts in memory
	for (int i = 0; i < 80; i++)
	{
		m_Memory[i] = fonts[i];
	}

	//Clear register, keys and stack
	for (int i = 0; i < 16; ++i)
	{
		m_Register[i] = 0;
		m_Keys[i] = 0;
		m_Stack[i] = 0;
	}

	//Reset Screen
	ClearScreen();

	//reset Delay Timer
	m_DelayTimer = 0;
	m_SoundTimer = 0;

	//reset flags
	m_bShouldDraw = false;
	//m_bEnableCompatibility = false;
	m_bEnableScreenWrap = false;
	m_bPaused = false;

	//seed random for true random numbers
	srand(static_cast<unsigned int>(time(nullptr)));
}

void Chip8::CreateOpcode()
{
	//char = 1 byte, opcode is 2 bytes
	//we need to combine current memory position + next memory position using an OR bitwise operation
	U16 opcode;

	//get byte at current location in memory
	opcode = m_Memory[m_MemoryPosition];

	//shift current byte 8 bits to the left
	opcode = opcode << 8;

	//read next byte in memory and OR to add them together
	m_Opcode = opcode | m_Memory[m_MemoryPosition + 1];
}

void Chip8::ExecuteOpcode()
{
	CreateOpcode();
	PrintOpcode();

	//mask to the opcode to switch on the first bit
	switch (m_Opcode & 0xF000)
	{
	case 0x000:
	{
		switch (m_Opcode & 0x000F)
		{
		case 0x0000: //0x00E0 clear screen
		{
			//Reset Screen
			ClearScreen();

			m_MemoryPosition += 2;
			break;
		}

		case 0x000E: //0x00EE. Return from a subroutine
		{
			//return to last position in the stack and reduce the stack index
			m_MemoryPosition = PopStack();
			break;
		}
		}

		break;
	}

	case 0x1000: //1NNN jump to address NNN
	{
		U16 n = m_Opcode & 0x0FFF;
		m_MemoryPosition = n;
		break;
	}

	case 0x2000: //2NNN call subroutine at NNN
	{
		U16 n = m_Opcode & 0x0FFF;

		//Store next memory position in the stack and increment the stack
		m_MemoryPosition += 2;
		PushStack(m_MemoryPosition);

		//go to the new memory position
		m_MemoryPosition = n;

		break;
	}

	case 0x3000: //3XNN skip next instruction if m_Register[X] == NN
	{
		U16 x = (m_Opcode & 0x0F00) >> 8; //change 0X00 0000 to 000X 
		U8 n = m_Opcode & 0x00FF;

		U8 registerX = GetRegisterData(x);

		if (registerX == n)
		{
			m_MemoryPosition += 4; //skip next instruction
		}
		else
		{
			m_MemoryPosition += 2; //go to next instruction
		}
		break;
	}

	case 0x4000: //4XNN skip the next instruction if m_Register[X] != NN
	{
		U16 x = (m_Opcode & 0x0F00) >> 8; //change 0X00 0000 to 000X 
		U8 n = m_Opcode & 0x00FF;
		U8 registerX = GetRegisterData(x);

		if (registerX != n)
		{
			m_MemoryPosition += 4; //skip next instruction
		}
		else
		{
			m_MemoryPosition += 2; //go to next instruction
		}

		break;
	}

	case 0x5000: //5XY0 skip the next instruction if m_Register[X] == m_Register[Y]
	{
		U16 x = (m_Opcode & 0x0F00) >> 8; //change 0X00 0000 to 000X 
		U16 y = (m_Opcode & 0x00F0) >> 4;
		U8 registerX = GetRegisterData(x);
		U8 registerY = GetRegisterData(y);

		if (registerX == registerY)
		{
			m_MemoryPosition += 4; //skip next instruction
		}
		else
		{
			m_MemoryPosition += 2; //go to next instruction
		}

		break;
	}

	case 0x6000: //6XNN set m_Register[X] to NN
	{
		U16 x = (m_Opcode & 0x0F00) >> 8;
		U8 n = m_Opcode & 0x00FF;

		m_Register[x] = n;
		m_MemoryPosition += 2;
		break;
	}
	case 0x7000: //7XNN Adds NN to m_Register[X]
	{
		U8 x = (m_Opcode & 0x0F00) >> 8;
		U8 n = m_Opcode & 0x00FF;

		m_Register[x] += n;
		m_MemoryPosition += 2;
		break;
	}

	case 0x8000:
		//Sub division for this opcode
		switch (m_Opcode & 0x000F)
		{
		case 0x0000: //8XY0 set m_Register[X] to the value of m_Register[Y]
		{
			U8 x = (m_Opcode & 0xF00) >> 8;
			U8 y = (m_Opcode & 0x0F0) >> 4;

			U8 registerY = GetRegisterData(y);

			m_Register[x] = registerY;
			m_MemoryPosition += 2;
			break;
		}

		case 0x0001: //8XY1 set m_Registers[X] to the value of m_Registers[X] OR m_Registers[Y]
		{
			U8 x = (m_Opcode & 0xF00) >> 8;
			U8 y = (m_Opcode & 0x0F0) >> 4;

			U8 registerX = GetRegisterData(x);
			U8 registerY = GetRegisterData(y);

			m_Register[x] = registerX | registerY;
			m_MemoryPosition += 2;
			break;

		}
		case 0x0002: //8XY2 sets m_Register[X] to m_Register[X] AND m_Register[Y]
		{
			U8 x = (m_Opcode & 0xF00) >> 8;
			U8 y = (m_Opcode & 0x0F0) >> 4;
			U8 registerX = GetRegisterData(x);
			U8 registerY = GetRegisterData(y);

			m_Register[x] = registerX & registerY;
			m_MemoryPosition += 2;
			break;
		}

		case 0x0003: //8XY3 sets m_Register[X] to m_Register[X] XOR m_Register[Y]
		{
			U8 x = (m_Opcode & 0xF00) >> 8;
			U8 y = (m_Opcode & 0x0F0) >> 4;

			U8 registerX = GetRegisterData(x);
			U8 registerY = GetRegisterData(y);

			m_Register[x] = registerX ^registerY;
			m_MemoryPosition += 2;
			break;
		}

		case 0x0004: //8XY4 Adds _Register[Y] to  m_Register[X].  
					 //		m_Register[0xF] is set to 1 when there's a carry, and to 0 when there isn't	
		{
			U8 x = (m_Opcode & 0xF00) >> 8;
			U8 y = (m_Opcode & 0x0F0) >> 4;
			U8 registerX = GetRegisterData(x);
			U8 registerY = GetRegisterData(y);

			bool bCarry = registerY > registerX;
			m_Register[0xF] = bCarry ? 1 : 0;
			m_Register[x] += registerY;

			m_MemoryPosition += 2;
			break;
		}

		case 0x0005: //8XY5 m_Register[Y] is subtracted from m_Register[X]
					 //		set m_Register[0xF] to 0 when there is a borrow and to 1 when there isn`t
					 //		a borrow occurs whenever the subtrahend is greater than the minuend.
		{
			U8 x = (m_Opcode & 0xF00) >> 8;
			U8 y = (m_Opcode & 0x0F0) >> 4;

			U8 registerX = GetRegisterData(x);
			U8 registerY = GetRegisterData(y);

			bool bBurrow = registerY > registerX;
			m_Register[0xF] = bBurrow ? 0 : 1;
			m_Register[x] = registerX - registerY;

			m_MemoryPosition += 2;
			break;
		}
		case 0x0006: //8XY6 shifts m_Register[X] right by one
					 //		set m_Register[0xF]least significant bit of m_Register[X] before the shift
		{
			U8 x = (m_Opcode & 0xF00) >> 8;
			U8 registerX = GetRegisterData(x);

			m_Register[0xF] = registerX & 0x1; //get least significant bit		
			m_Register[x] = registerX >> 1; //shift right

			m_MemoryPosition += 2;
			break;
		}

		case 0x0007: //8XY7 Set m_Register[X] to m_Register[Y] - m_Register[X]
					 //		set m_Register[0xF] to 0 if there is a burrow and 1 when there isn`t
		{
			U16 x = (m_Opcode & 0xF00) >> 8;
			U16 y = (m_Opcode & 0xF00) >> 4;
			U8 registerX = GetRegisterData(x);
			U8 registerY = GetRegisterData(y);

			//Is there a burrow
			bool bBurrow = registerX > registerY;
			m_Register[0xF] = (bBurrow) ? 0 : 1;

			//set register to register[y] - register[x]
			m_Register[x] = registerY - registerX;

			m_MemoryPosition += 2;
			break;
		}
		case 0x000E: //8XYE shifts m_Register[X] left by one
					 //		set m_Register[0xF]least significant bit of m_Register[X] before the shift
		{
			U8 x = (m_Opcode & 0xF00) >> 8;
			U8 registerX = GetRegisterData(x);

			m_Register[0xF] = registerX & 0x1; //get least significant bit	
			m_Register[x] = registerX << 1; //shift right

			m_MemoryPosition += 2;
			break;
		}
		}
		break;
	case 0x9000: //9XY0 skip the next instruction if m_Registers[X] doesn't equal m_Registers[Y]
	{
		U8 x = (m_Opcode & 0x0F00) >> 8;
		U8 y = (m_Opcode & 0x00F0) >> 4;

		if (GetRegisterData(x) != GetRegisterData(y))
		{
			m_MemoryPosition += 4;
		}
		else
		{
			m_MemoryPosition += 2;
		}

		break;
	}

	case 0xA000: //ANNN set RegisterIndex to NNN
	{
		U16 n = m_Opcode & 0x0FFF;

		m_RegisterIndex = n; //access NNN
		m_MemoryPosition += 2;
		break;
	}

	case 0xB000: //BNNN jump to address NNN + m_Register[0]
	{
		U16 n = m_Opcode & 0x0FFF;
		m_MemoryPosition = n + m_Register[0];
		break;
	}

	case 0xC000: //CXNN Sets m_Register[X] to rand() AND NN
	{
		U8 x = (m_Opcode & 0x0F00) >> 8;
		U8 n = m_Opcode & 0x00FF;

		//limit rand value to 255
		U8 rnd = rand() % 0xFF;
		m_Register[x] = rnd & n;
		m_MemoryPosition += 2;
		break;
	}

	case 0xD000: // DXYN: Draws a sprite at (VX, VY), width = 8 pixels and a height = N pixels. 
				 // Each row of 8 pixels is read as bit-coded starting from memory location I; 
				 // RegisterIndex value doesn't change after the execution of this instruction. 
				 // m_Register[0xF] is set to 1 if any screen pixels are flipped from set to unset when the sprite is drawn, 
				 // and to 0 if that doesn't happen
	{
		//get required data from the opcode
		U8 x = (m_Opcode & 0xF00) >> 8;
		U8 y = (m_Opcode & 0x0F0) >> 4;
		U8 n = m_Opcode & 0x00F;

		U16 height = n; //height of the sprite
		U16 xPos = GetRegisterData(x);; //start position X
		U16 yPos = GetRegisterData(y);; //start position Y

		DrawPixel(xPos, yPos, height);

		m_MemoryPosition += 2;
		break;
	}

	case 0xE000:
	{
		switch (m_Opcode & 0x00FF)
		{
		case 0x009E: //EX9E Skips the next instruction if the key stored in m_Register[X] is pressed
		{
			U8 x = (m_Opcode & 0x0F00) >> 8;
			U8 registerData = GetRegisterData(x);
			if (IsKeyPressed(registerData)) //pressed
			{
				m_MemoryPosition += 4;
			}
			else
			{
				m_MemoryPosition += 2;
			}
			break;
		}

		case 0x00A1: //EXA1 Skips the next instruction if the key stored in m_Register[X] isn`t pressed
		{
			U8 x = (m_Opcode & 0x0F00) >> 8;
			U8 registerData = GetRegisterData(x);
			if (!IsKeyPressed(registerData)) //not pressed
			{
				m_MemoryPosition += 4;
			}
			else
			{
				m_MemoryPosition += 2;
			}

			break;
		}
		}
		break;
	}

	case 0xF000: //multiple options
	{
		switch (m_Opcode & 0x00FF)
		{
		case 0x0007: //FX07 set m_Registers[X] to the value of the delay timer
		{
			U8 x = (m_Opcode & 0x0F00) >> 8;
			m_Register[x] = m_DelayTimer;
			m_MemoryPosition += 2;
			break;
		}

		case 0x0015: //FX15 set the delay timer to m_Registers[X]
		{
			U8 x = (m_Opcode & 0x0F00) >> 8;
			m_DelayTimer = GetRegisterData(x);
			m_MemoryPosition += 2;
			break;
		}

		case 0x0018: //FX15 set the sound timer to m_Registers[X]
		{
			U8 x = (m_Opcode & 0x0F00) >> 8;
			m_SoundTimer = GetRegisterData(x);
			m_MemoryPosition += 2;
			break;
		}

		case 0x0029: //FX15 Sets m_RegisterIndex to the location of the sprite for the character m_Register[X]
					 //		Characters 0-F are represented by a 4x5 font
		{
			U8 x = (m_Opcode & 0x0F00) >> 8;
			m_RegisterIndex = GetRegisterData(x) * 5;
			m_MemoryPosition += 2;
			break;
		}

		case 0x0033: //FX33 store the Binary Coded decimal representation of m_Register[X] with the most significant of three digits at the address in m_RegisterIndex
					 //		the middle digit at _RegisterIndex +1 and the least significant digit at m_RegisterIndex +2. In other words take the decimal representation 
					 //		of m_Registers[X] place the hundreds digit in memory at the location in m_RegisterIndex the tens digit in m-_RegisterIndex +1 
					 //		and the ones digit in m_RegisterIndex +2 
		{
			U8 x = (m_Opcode & 0x0F00) >> 8;
			U8 value = GetRegisterData(x);

			U8 mostSignificant = value / 100;
			U8 middle = (value / 10) % 10;
			U8 least = (value % 100) % 10;

			m_Memory[m_RegisterIndex] = mostSignificant;
			m_Memory[m_RegisterIndex + 1] = middle;
			m_Memory[m_RegisterIndex + 2] = least;

			m_MemoryPosition += 2;

			break;
		}

		case 0x000A: //FX0A a key press is awaited and then stored in m_Registers[X]
		{
			bool bKeyPressed = false;

			U8 x = (m_Opcode & 0x0F00) >> 8;

			//look for a pressed key and store it in the register
			for (U8 i = 0; i < 16; i++)
			{
				if (m_Keys[i] != 0)
				{
					m_Register[x] = i;
					bKeyPressed = true;
				}
			}

			//if the key is not pressed try again, keeps the game loop going
			if (bKeyPressed)
			{
				m_MemoryPosition += 2;
			}

			break;
		}

		case 0x001E: //FX1E Adds m_Registers[X] to RegisterIndex
		{
			U8 x = (m_Opcode & 0x0F00) >> 8;

			U8 val = GetRegisterData(x);
			m_RegisterIndex += val;
			m_MemoryPosition += 2;
			break;
		}

		case 0x0055: //FX55 store m_Registers[0] to m_Registers[X] in memory starting at address m_RegisterIndex
					 //		the value of the I register will be incremented by X + 1. This is due to the changing of addresses by the interpreter.
		{
			U8 x = (m_Opcode & 0x0F00) >> 8;

			for (int i = 0; i <= x; i++)
			{
				m_Memory[m_RegisterIndex + i] = GetRegisterData(i);
			}


			if (!m_bEnableCompatibility)
			{
				m_RegisterIndex += x + 1;
			}

			m_MemoryPosition += 2;
			break;
		}

		case 0x0065: //FX65 fills m_Registers[0] to m_Registers[X] with values in memory starting at address m_RegisterIndex
					 //		the value of the I register will be incremented by X + 1. This is due to the changing of addresses by the interpreter.
		{
			U8 x = (m_Opcode & 0x0F00) >> 8;

			for (int i = 0; i <= x; i++)
			{
				m_Register[i] = m_Memory[m_RegisterIndex + i];
			}

			if (!m_bEnableCompatibility)
			{
				m_RegisterIndex += x + 1;
			}

			m_MemoryPosition += 2;
			break;

		}
		}
		break;
	}
	}
}

void Chip8::Run()
{
	//make sure a game is loaded in memory
	if (!m_bGameLoaded)
	{
		cerr << "No game is loaded!" << endl;
		return;
	}

	//Reset drawing flag
	m_bShouldDraw = false;

	//run multiple opcodes based on speed
	for (int i = 0; i < m_RunSpeed; i++)
	{
		ExecuteOpcode();

		//update Timer
		if (m_DelayTimer > 0)
		{
			m_DelayTimer--;
		}
		//update sound timer
		if (m_SoundTimer > 0)
		{
			//play system beep
			if (m_SoundTimer == 1)
			{
				cout << "\a"; //play system sound
			}

			m_SoundTimer--;
		}
	}
}

void Chip8::DrawPixel(U16 x, U16 y, U16 height)
{
	//width is always 8 pixels
	const int width = 8;

	m_Register[0xF] = 0;
	//Access sprite data from memory
	for (int heightIndex = 0; heightIndex < height; heightIndex++)
	{
		//get spriteData for specific pixel in memory
		U16 spriteData = m_Memory[m_RegisterIndex + heightIndex];

		for (int widthIndex = 0; widthIndex < width; widthIndex++)
		{
			int posX = x + widthIndex;
			int posY = y + heightIndex;

			//Screen warp
			if (m_bEnableScreenWrap)
			{
				posX = posX%WIDTH;
				posY = posY%HEIGHT;
			}
			else if (posX >= WIDTH || posY >= HEIGHT)
			{
				continue;
			}

			//evaluate each bit from left to right
			//This happens because the AND operand, 0x80, gets shifted right on each loop.
			U8 filter = (0x80 >> widthIndex);
			auto data = spriteData & filter;

			//if there is something to draw
			if (data != 0)
			{
				//if flipping from set to unset set carry flag to 1
				//Collision check
				int pos = posX + (WIDTH * posY);
				if (m_Screen[pos] == 1)
				{
					m_Register[0xF] = 1;
				}

				//XOR operation flip from 0000 0000 to 1111 1111 or reverse
				//this clears the previous drawn pixel in
				m_Screen[pos] ^= 1;

				//toggle draw flag
				m_bShouldDraw = true;
			}
		}
	}

}

//Load a binary file in memory
void Chip8::LoadGame(const char* filename)
{
	//remove previous game data
	Initialize();

	string name = string(filename);
	int index = name.find_last_of(('\\')) + 1;
	name = name.substr(index);

	//open the file
	ifstream file(filename, std::ios::binary);

	//check if the file is open
	if (!file.is_open())
	{
		cout << "Chip8::Failed to open file: " << filename << "!\n";
		m_bGameLoaded = false;
		return;
	}

	//get file size
	file.seekg(0, file.end);
	auto size = static_cast<int>(file.tellg());
	file.seekg(0, file.beg);

	//read file
	auto buffer = new char[size];
	file.read(buffer, size);
	file.close();

	//Generate hash code and toggle compatibility flags for specific games
	unsigned int hash = HashGen::Adler(buffer, size);
	cerr << name << ": " << hash << endl;
	ToggleCompatibilityFlags(hash);

	//store it in chip8 memory with an offset of 512 bytes
	for (int i = 0; i < size; ++i)
	{
		m_Memory[i + 512] = buffer[i];
	}

	m_bGameLoaded = true;
	//cout << "Loaded Game: " << filename << "!\n";

}

//Helpers
#pragma region Helpers
void Chip8::ClearScreen()
{
	for (int i = 0; i < WIDTH * HEIGHT; i++)
		m_Screen[i] = 0; //clear to black
}

void Chip8::PressKey(int keyIndex, U8 pressed)
{
	m_Keys[keyIndex] = pressed;
}

bool Chip8::IsKeyPressed(U8 key)
{
	return m_Keys[key] != 0;
}

void Chip8::AdjustSpeed(int increment)
{
	//increment the speed
	m_RunSpeed += increment;

	if (m_RunSpeed > 120)
	{
		m_RunSpeed = 120;
	}
	else if (m_RunSpeed < 1)
	{
		m_RunSpeed = 1;
	}
}

void Chip8::Pause()
{
	m_bPaused = !m_bPaused;

	if (m_bPaused)
	{
		m_RunSpeedBeforePause = m_RunSpeed;
		m_RunSpeed = 0;
	}
	else
	{
		m_RunSpeed = m_RunSpeedBeforePause;
	}
}

U8 Chip8::GetRegisterData(int index)
{
	PrintRegisterValue(index);
	return m_Register[index];
}

void Chip8::PushStack(U16 address)
{
	//avoid going out of bounds
	if (m_StackIndex >= 0 && m_StackIndex < 16)
	{
		//Store address
		m_Stack[m_StackIndex] = address;

		//increment stack index
		m_StackIndex++;

		//clamp stack index to 16
		if (m_StackIndex > 15)
		{
			m_StackIndex = 15;
		}
	}
}

U16 Chip8::PopStack()
{
	m_StackIndex--;
	if (m_StackIndex < 0)
	{
		m_StackIndex = 0;
	}

	return m_Stack[m_StackIndex];
}

void Chip8::ToggleCompatibilityFlags(int hash)
{
	m_bEnableScreenWrap = (hash == 1598429529); //Vers Enabled
	m_bEnableCompatibility = (hash == 348653547); //Connect4 Enabled

}

#pragma endregion

//Debuggingx
void Chip8::PrintOpcode()
{
#ifdef LOGOPCODE
	cerr << std::hex << m_Opcode << " ";
#endif

}

void Chip8::PrintRegisterValue(int)
{
#ifdef LOGREGISTER
	cerr << m_Register[index] << " ";
#endif
}