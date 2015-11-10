#pragma once

typedef unsigned char U8;
typedef unsigned short U16;

class Chip8
{
public:

	//Constructor
	Chip8(int screenWidth, int screenHeight,int speed);
	~Chip8();

	//Functions
	void Initialize();
	void Run();
	void LoadGame(const char* filename);	
	void IncreaseSpeed();
	void ReduceSpeed();
	void SetCompatibilityMode();


	//Key press
	void PressKey(int keyIndex, U8 pressed);

	//Getters
	bool shouldDraw() { return m_bShouldDraw; }
	int GetRunSpeed() { return m_RunSpeed; }
	bool GetCompatibilityMode()	{ return m_bEnableCompatibility; }

	U8 m_Screen[64 * 32]; //Chip8 Screen
	
private:

	void CreateOpcode();
	void ExecuteOpcode();

	U8 GetRegisterData(int index);

	void ClearScreen();
	void DrawPixel(U16 x, U16 y, U16 height);
	bool IsKeyPressed(U8 key);

	//Debug
	void PrintOpcode();
	void PrintRegisterValue(int index);

	//Chip8 CPU specifics
	int m_ScreenWidth, m_ScreenHeight;
	int m_RunSpeed;
	U16 m_Opcode;

	U16 m_MemoryPosition; //stores the position in memory starts at 0x200	
	U16 m_StackIndex; //position in the stack
	U16 m_RegisterIndex; //position in the register	
	
	U8 m_Memory[4096]; //chip8 occupies first 512 bytes of the program
	U8 m_Register[16]; //V0 - VF
	U16 m_Stack[16]; //used to store return addresses when subroutines are called
	U8 m_Keys[16]; //Hexadecimal keyboard from 0 to f

	U8 m_DelayTimer;
	U8 m_SoundTimer;

	//flags
	bool m_bGameLoaded, m_bShouldDraw;
	bool m_bEnableCompatibility;

};

