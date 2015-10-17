#pragma once

typedef unsigned char U8;
typedef unsigned short U16;

class Chip8
{
public:
	Chip8(int screenWidth, int screenHeight);
	~Chip8();

	//Functions
	void Initialize();
	void Run();
	void LoadGame(const char* filename);	

	void PressKey(int keyIndex, U8 pressed);

	bool shouldDraw() { return m_bShouldDraw; }
	
	void SetRunSpeed(int speed) { m_RunSpeed = speed; }

	U8 m_Screen[64 * 32]; //Chip8 Screen
	
	
private:

	int m_RunSpeed;

	void FetchOpcode();
	void ExecuteOpcode();

	//Debug
	void PrintOpcode();

	//VARIABLES

	//Chip8 CPU specifics
	int m_ScreenWidth, m_ScreenHeight;
	U16 m_Opcode;

	U16 m_MemoryPosition; //stores the position in memory starts at 0x200	
	U16 m_StackIndex; //position in the stack
	U16 m_RegisterIIndex; //position in the register	
	
	U8 m_Memory[4096]; //chip8 occupies first 512 bytes of the program
	U8 m_RegisterI[16]; //V0 - VF
	U16 m_Stack[16]; //used to store return adresses when subroutines are called
	U8 m_Keys[16]; //Hexadecimal keyboard from 0 to f

	U8 m_DelayTimer;
	U8 m_SoundTimer;

	U8 m_FontSet[80]; //fonts are 4 pixels wide and 5 pixels high

	//flags
	bool m_bGameLoaded, m_bShouldDraw;

};

