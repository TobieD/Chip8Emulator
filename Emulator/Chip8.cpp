#include "Chip8.h"
#include <fstream>
#include <iostream>
#include <chrono>

using namespace std;

Chip8::Chip8(int screenWidth, int screenHeight) :
	m_ScreenWidth(screenWidth),
	m_ScreenHeight(screenHeight),	
	m_MemoryPosition(0x200),
	m_StackIndex(0),
	m_RegisterIIndex(0),
	m_bGameLoaded(false),
	m_bShouldDraw(false)
{

}

Chip8::~Chip8()
{
	//clear memory
	for (int i = 0; i < 4096; ++i)
		m_Memory[i] = 0;

	//Clear registers
	for (int i = 0; i < 16; ++i)
		m_RegisterI[i] = 0;

	//Clear stack
	for (int i = 0; i < 16; ++i)
		m_Stack[i] = 0;
}

void Chip8::Initialize()
{
	//reset
	m_MemoryPosition = 0x200; //first 512 bytes are reserved
	m_Opcode = 0;
	m_RegisterIIndex = 0;
	m_StackIndex = 0;

	//clear memory
	for (int i = 0; i < 4096; ++i)
		m_Memory[i] = 0;

	//Clear registers
	for (int i = 0; i < 16; ++i)
		m_RegisterI[i]  = 0;

	//Clear keys
	for (int i = 0; i < 16; ++i)
		m_Keys[i] = 0;

	//Clear stack
	for (int i = 0; i < 16; ++i)
		m_Stack[i] = 0;
	
	//Reset Screen
	for (int i = 0; i < m_ScreenWidth * m_ScreenHeight; i++)
		m_Screen[i] = 0; //clear to white	
	
	//redraw screen
	m_bShouldDraw = true;
	m_bGameLoaded = false;

	//seed random for true random numbers
	srand(static_cast<unsigned int>(time(nullptr)));
}

void Chip8::Run()
{
	//make sure a game is loaded in memory
	if (!m_bGameLoaded)
		return;

	//Reset drawing flag
	m_bShouldDraw = false;

	int runspeed = 600;
	for (int i = 0; i < runspeed; i++)
	{
		FetchOpcode();
		ExecuteOpcode();
	}

}

void Chip8::FetchOpcode()
{
	//char = 1 byte, opcode is 2 bytes
	//we need to combine current memory position + next memory position using an OR bitwise operation
	U16 opcode;

	//get byte at current location in memory
	opcode = m_Memory[m_MemoryPosition];

	//shift current byte 8 bits to the left
	opcode = opcode << 8;

	//read next byte in memory
	m_Opcode = opcode | m_Memory[m_MemoryPosition + 1];
}

void Chip8::ExecuteOpcode()
{
	U16 x = 0, y = 0, n = 0;
	U16 rnd = 0;

	//mask to the opcode to switch on the first bit
	switch (m_Opcode & 0xF000)
	{
		case 0x000:
		{
			//multiple other options
			switch (m_Opcode & 0x000F)
			{
				case 0x0000: //0x00E0 clear screen
				{
					m_MemoryPosition += 2;
					break;
				}

				case 0x000E : //0x00EE. Return from a subroutine
				{
					//return to last position in the stack and reduce the stackindex
					m_StackIndex--;
					m_MemoryPosition = m_Stack[m_StackIndex];
					break;
				}
			}		

			break;
		}

	case 0x1000: //1NNN jump to address NNN
		n = m_Opcode & 0x0FFF;
		m_MemoryPosition = n;
		break;

	case 0x2000: //2NNN call subroutine at NNN
		n = m_Opcode & 0x0FFF;

		//Store next memory position in the stack and increment the stack
		m_MemoryPosition += 2;
		m_Stack[m_StackIndex] = m_MemoryPosition;
		m_StackIndex++;

		//go to the new memory position
		m_MemoryPosition = n;

		break;

	case 0x3000: //3XNN skip next instruction if m_RegisterI[X] == NN
		x = (m_Opcode & 0x0F00) >> 8; //change 0X00 0000 to 000X 
		n = m_Opcode & 0x00FF;

		if (m_RegisterI[x] == n)
			m_MemoryPosition += 4; //skip next instruction
		else
			m_MemoryPosition += 2; //go to next instruction
		break;

	case 0x4000: //4XNN skip the next instruction if m_RegisterI[X] != NN
		x = (m_Opcode & 0x0F00) >> 8; //change 0X00 0000 to 000X 
		n = m_Opcode & 0x00FF;

		if (m_RegisterI[x] != n)
			m_MemoryPosition += 4; //skip next instruction
		else
			m_MemoryPosition += 2; //go to next instruction

		break;

	case 0x6000: //6XNN set m_RegisterI[X] to NN
		x = (m_Opcode & 0x0F00) >> 8;
		n = m_Opcode & 0x00FF;

		m_RegisterI[x] = static_cast<U8>(n);
		m_MemoryPosition += 2;
		break;
	case 0x7000: //7XNN Adds NN to m_RegisterI[X]
		x = (m_Opcode & 0x0F00) >> 8;
		n = m_Opcode & 0x00FF;

		m_RegisterI[x] += static_cast<U8>(n);
		m_MemoryPosition += 2;
		break;

	case 0x8000:
		//Sub division for this opcode
		switch (m_Opcode & 0x000F)
		{
			case 0x0000: //8XY0 set m_RegisterI[X] to the value of m_RegisterI[Y]
					x = (m_Opcode & 0xF00) >> 8;
					y = (m_Opcode & 0x0F0) >> 4;

					m_RegisterI[x] = m_RegisterI[y];
					m_MemoryPosition += 2;
					break;

			case 0x0002: //8XY2 sets m_Register[X] to VX AND m_Register[Y]
					x = (m_Opcode & 0xF00) >> 8;
					y = (m_Opcode & 0x0F0) >> 4;

					m_RegisterI[x] = m_RegisterI[x] & m_RegisterI[y];
					m_MemoryPosition += 2;
					break;

			case 0x0005: //8XY5 m_Register[Y] is subtracted from m_Register[X]
						 //		set m_Register[0xF] to 0 when there is a borrow and to 1 when there isn`t
						 //		a borrow occurs whenever the subtrahend is greater than the minuend.
				{
					x = (m_Opcode & 0xF00) >> 8;
					y = (m_Opcode & 0x0F0) >> 4;

					if (m_RegisterI[y] > m_RegisterI[x])
						m_RegisterI[0xF] = 0;
					else
						m_RegisterI[0xF] = 1;

					m_RegisterI[x] = m_RegisterI[x] - m_RegisterI[y];				

					m_MemoryPosition += 2;
					break;
				}
		}
		break;

	case 0xA000: //ANNN set RegisterIndex to NNN

		n = m_Opcode & 0x0FFF;

		m_RegisterIIndex = n; //access NNN
		m_MemoryPosition += 2;
		break;

	case 0xC000: //CXNN Sets m_RegisterI[X] to rand() AND NN
		x = (m_Opcode & 0x0F00) >> 8;
		n = m_Opcode & 0x00FF;

		//limit rand value to 255
		rnd = rand() % 0xFF;
		m_RegisterI[x] = static_cast<unsigned char>(rnd & n);
		m_MemoryPosition += 2;
		break;

	case 0xD000: // DXYN: Draws a sprite at (VX, VY), width = 8 pixels and a height = N pixels. 
				 // Each row of 8 pixels is read as bit-coded starting from memory location I; 
				 // RegisterIndex value doesn't change after the execution of this instruction. 
				 // m_RegisterI[0xF] is set to 1 if any screen pixels are flipped from set to unset when the sprite is drawn, 
				 // and to 0 if that doesn't happen
	{
		//get required data from the opcode
		x = (m_Opcode & 0xF00) >> 8;
		y = (m_Opcode & 0x0F0) >> 4;
		n = m_Opcode & 0x00F;

		const int width = 8;
		U16 height = n; //height of the sprite
		U16 xPos = m_RegisterI[x]; //start position X
		U16 yPos = m_RegisterI[y]; //start position Y

		//Access sprite data from memory
		for (int yl = 0; yl < height; yl++)
		{
			//get spriteData for specific pixel
			U16 spriteData = m_Memory[m_RegisterIIndex + yl];

			for (int xl = 0; xl < width; xl++)
			{
				auto data = spriteData & (0x80 >> xl);
				if (data == 0) continue;

				//get position of current pixel
				auto currentPixelPos = xl + xPos + (yl + yPos) * 64; //64 == ScreenWidth

				//if flipping from set to unset set carry flag to 1
				m_RegisterI[0xF] = (m_Screen[currentPixelPos] == 1) ? 1 : 0;

				//XOR operation flip from 0000 0000 to 1111 1111 or reverse
				m_Screen[currentPixelPos] ^= 1;
			}
		}

		//toggle draw flag
		m_bShouldDraw = true;
		m_MemoryPosition += 2;
		break;
	}

	case 0xF000: //multiple options
	{
		switch (m_Opcode & 0x00FF)
		{
			case 0x00A: //FX0A a key press is awaited and then stored in m_Registers[X]
			{
				bool bKeyPressed = false;

				x = (m_Opcode & 0x0F00) >> 8;

				//if the key is not pressed try again
				if (!bKeyPressed)
					return;

				//m_RegisterI[x] = x;
				m_MemoryPosition += 2;
			}

			case 0x001E: //FX1E Adds m_Registers[X] to RegisterIndex
			{
				x = (m_Opcode & 0x0F00) >> 8;

				m_RegisterIIndex = x;
				m_MemoryPosition += 2;
			}
		}
	}
	}


}


//Load a binary file in memory
void Chip8::LoadGame(const char* filename)
{
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
	file.seekg(0,file.end);
	auto size = static_cast<int>(file.tellg());
	file.seekg(0, file.beg);

	//read file
	auto buffer = new char[size];	
	file.read(buffer, size);
	file.close();

	//store it in chip8 memory with an offset of 512 bytes
	for (int i = 0; i < size; ++i)
		m_Memory[i + 512] = buffer[i];

	m_bGameLoaded = true;
	cout << "Loaded Game: " << filename << "!\n";

}

void Chip8::PressKey(int keyIndex, unsigned char pressed)
{
	m_Keys[keyIndex] = pressed;
}