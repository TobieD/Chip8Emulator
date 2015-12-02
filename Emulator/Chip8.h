#pragma once

typedef unsigned char U8;
typedef unsigned short U16;

class Chip8
{
public:

	//Constructor
	Chip8();
	~Chip8();

	//Functions
	void Initialize();
	void Run();
	void LoadGame(const char* filename);		

	//INPUT
	void PressKey(int keyIndex, U8 pressed);
	void AdjustSpeed(int increment);
	void Pause();

	//Getters
	bool shouldDraw() { return m_bShouldDraw; }
	int GetRunSpeed() { return m_RunSpeed; }
	bool GetCompatibilityMode()	{ return m_bEnableCompatibility; }
	const U8* GetScreenData(){	return m_Screen; }
	
	const static int WIDTH = 64;
	const static int HEIGHT = 32;

	
		
private:

	//Chip8 helpers
	void CreateOpcode();
	void ExecuteOpcode();

	void PushStack(U16 address);
	U16 PopStack();
	U8 GetRegisterData(int index);

	//Drawing Helpers
	void ClearScreen();
	void DrawPixel(U16 x, U16 y, U16 height);
	
	//Input helpers
	bool IsKeyPressed(U8 key);

	//Debug
	void PrintOpcode();
	void PrintRegisterValue(int index);

	//Toggle Compatibilty flags based on the hash of a game
	void ToggleCompatibilityFlags(int hash);
	
	//Chip8 CPU specifics
	int m_RunSpeed, m_RunSpeedBeforePause;

	U16 m_Opcode; //Current instruction to interpret
	U16 m_MemoryPosition; //stores the position in memory starts at 0x200	
	U8 m_Memory[4096]; //chip8 occupies first 512 bytes of the program
	U16 m_RegisterIndex; //position in the register	
	U8 m_Register[16]; //V0 - VF
	U16 m_StackIndex; //position in the stack
	U16 m_Stack[16]; //used to store return addresses when subroutines are called
	U8 m_Keys[16]; //Hexadecimal keyboard from 0 to f
	U8 m_DelayTimer;
	U8 m_SoundTimer;

	U8 m_Screen[WIDTH * HEIGHT]; //Chip8 Screen

	//flags
	bool m_bGameLoaded, m_bShouldDraw, m_bPaused;
	bool m_bEnableCompatibility, m_bEnableScreenWrap;

};

