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

bool GetTime(std::wstring& buffer)
{
	time_t     now = time(0);
	struct tm localTime;	
	localtime_s(&localTime, &now);

	wchar_t time_buf[100] = { 0 };
	bool result = wcsftime(time_buf, sizeof(time_buf), L"%Y-%m-%d %H:%M:%S ", &localTime);
	buffer.assign(time_buf);
	return result;
}

void PathOfSvc(std::wstring& buffer)
{
	wchar_t buff[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, buff, MAX_PATH);
	std::wstring::size_type pos = std::wstring(buff).find_last_of(L"\\/");
	buffer.assign(buff);
	buffer = buffer.substr(0, pos);
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
	
	std::wstring timeBuffer;
	if (GetTime(timeBuffer))
	{
		instance->wofStream << timeBuffer << " " << std::this_thread::get_id() << L": " << msg << L"\n";
	}
	else
	{
		instance->wofStream << std::this_thread::get_id() << L": " << msg << L"\n";
	}
	instance->wofStream.flush();
}

Logger* Logger::GetInstance()
{
	if (instance == nullptr)
	{
		instance = new Logger();

		std::wstring buffer;
		PathOfSvc(buffer);
		instance->logName = buffer;
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

Logger::~Logger()
{
	if (instance->wofStream.is_open())
	{
		instance->wofStream.close();
	}
}
