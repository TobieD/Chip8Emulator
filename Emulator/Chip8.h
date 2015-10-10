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

	//std::vector<unsigned char> GetScreen()	{ return m_Screen;}
	bool shouldDraw() { return m_bShouldDraw; }
	void PressKey(int keyIndex, unsigned char pressed);


	unsigned char m_Screen[64 * 32]; //Chip8 Screen


	
private:

	void FetchOpcode();
	void ExecuteOpcode();

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

	//flags
	bool m_bGameLoaded, m_bShouldDraw;

};

