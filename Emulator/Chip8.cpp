#include "Chip8.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <chrono>

using namespace std;

Chip8::Chip8():
	m_bGameLoaded(false)
{
	//nothing to do
}

Chip8::~Chip8()
{

}

void Chip8::Initialize()
{
	//seed random for true random numbers
	srand(static_cast<unsigned int>(time(nullptr)));
}

void Chip8::Run()
{
	//make sure a game is loaded in memory
	if (!m_bGameLoaded)
		return;
}

//Load a binary file in memory
void Chip8::LoadGame(const std::string &filename)
{
	//open the file
	ifstream file(filename.c_str(), std::ios::binary);

	if(!file.is_open())
	{
		cout << "Chip8::Failed to open file: " << filename << "!\n";
		m_bGameLoaded = false;
		return;
	}

	//read the file 
	vector<char> buffer((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));


	buffer.clear();

	m_bGameLoaded = true;
	cout << "Loaded Game " << filename << "!\n";

}