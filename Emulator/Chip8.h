#pragma once
#include <string>

class Chip8
{
public:
	Chip8();
	~Chip8();

	//Functions
	void Initialize();
	void Run();
	void LoadGame(const std::string &filename);

private:

	//Functions


	//Variables
	bool m_bGameLoaded;

};

