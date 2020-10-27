#pragma once
#include <stdio.h>
#include <string>
#include <fstream>
class Logger
{

private:
	static Logger *instance;
	std::wofstream wofStream;
	std::wstring logName;		
	~Logger();
	void Log(const std::string msg);
public:
	static Logger* GetInstance();
	void Log(const std::wstring msg);
	
};

