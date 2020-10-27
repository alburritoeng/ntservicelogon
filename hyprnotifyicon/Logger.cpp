#include "Logger.h"
#include <windows.h>
//#include "constants.h"
#include <iostream>
#include <sstream>
#include <ctime>
#include <time.h>
#include <thread>
#include <chrono>
#include <mutex>

Logger * Logger::instance;
std::mutex log_mutex;

char* GetTime()
{
	time_t     now = time(0);
	struct tm localTime;	
	localtime_s(&localTime, &now);

	char time_buf[100] = { 0 };
	strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S ", &localTime);
	return std::move(time_buf);	
}

std::wstring PathOfSvc() {
	wchar_t buffer[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, buffer, MAX_PATH);
	std::wstring::size_type pos = std::wstring(buffer).find_last_of(L"\\/");
	return std::wstring(buffer).substr(0, pos);
}

void Logger::Log(const std::string msg)
{	
	std::wstring wsMsg(msg.begin(), msg.end());	
	Log(wsMsg);
}

void Logger::Log(const std::wstring msg)
{
	std::lock_guard<std::mutex> lk(log_mutex);
	if (!instance->wofStream.is_open())
	{
		instance->wofStream.open(instance->logName, std::ofstream::out | std::ofstream::app);
	}	
	
	instance->wofStream << std::this_thread::get_id() <<  L": " << msg << L"\n";
	instance->wofStream.flush();
}

Logger* Logger::GetInstance()
{
	if (instance == nullptr)
	{
		instance = new Logger();

		instance->logName = PathOfSvc();
		instance->logName.append(L"\\");
		instance->logName.append(L"hypernotification");
		instance->logName.append(L"_log.txt");

		if (!instance->wofStream.is_open())
		{
			instance->wofStream.open(instance->logName, std::ofstream::out | std::ofstream::app);			
		}
	}

	return instance;
}

Logger::Logger()
{
	
	// get the log name = the current module running + txt	
	
}

Logger::~Logger()
{
	if (instance->wofStream.is_open())
	{
		instance->wofStream.close();
	}
}
