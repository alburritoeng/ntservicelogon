#pragma once
#include "SystemTray.h"
#include <string>

namespace HyprTest
{
	class USBNotifier
	{
	private:
		SystemTray* _systemTray = nullptr;
		HDEVNOTIFY gDevNotify = nullptr;
		HWND _hWnd;
		bool CreateDeviceString(LPARAM lParam, std::wstring& data);
	public:
		USBNotifier(SystemTray* systemTray, HWND hWnd);
		bool RegisterForUsbConnections();	
		bool HandleDeviceConnected(LPARAM lParam);
		bool HandleDeviceRemoved(LPARAM LParam);
		~USBNotifier();
	};
}

