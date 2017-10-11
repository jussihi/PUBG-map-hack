#include "Types.hpp"
#include "GameDataParser.hpp"
#include <Windows.h>
#include <iostream>
#include <thread>

void readerLoop(GameDataParser* w_reader)
{
	while (1)
	{
		if (readLoop)
		{
			w_reader->readLoop();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}
}


int main()
{
	// init a new GameDataParser instance
	GameDataParser* GDParser;

	try
	{
		GDParser = new GameDataParser;
	}
	catch (std::exception& e)
	{
		std::cout << "Opening driver handle failed. ErrorMsg: " << e.what() << std::endl;
		return -1;
	}

	std::thread t1(readerLoop, GDParser);

	std::string readLine = "";

	// l33b user interface
	while (readLine != "quit")
	{
		std::cin >> readLine;
		if (readLine == "start" && readLoop == false && GDParser->getPUBase())
		{
			readLoop = true;
			std::cout << "Starting read loop" << std::endl;
		}

		if (readLine == "stop")
		{
			readLoop = false;
			std::cout << "Stopping read loop" << std::endl;
		}

		if (readLine == "base")
		{
			std::cout << "Base set to: " << GDParser->readPUBase() << std::endl;
		}

		if (readLine == "exit" || readLine == "quit")
		{
			break;
		}
		readLine = "";
	}
	
	return 0;
}