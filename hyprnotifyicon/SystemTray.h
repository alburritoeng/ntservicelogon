#pragma once
#include <windows.h>
// This class will take a window handle to be used for 
// registering for USB Notifications. It will also have an instance of the Notification
// interface injected so it can display messages of new connections/disconnections
namespace HyprTest
{
	struct SystemTrayInfo
	{
		HINSTANCE instance;
		HWND hWnd;
		GUID uuid;
		unsigned int notifyIconId;
		unsigned int notifyCallBack;
		unsigned int toolTipResourceId;
	};

	class SystemTray
	{
	private:
		
		SystemTrayInfo _info;
		const unsigned int UI_ID = 10272020;				
		BOOL DeleteNotificationIcon();
	public:
		BOOL AddNotificationIcon();
		bool ShowPrintJobBalloon(const wchar_t* title, const wchar_t*  msg);
		void SetSystemInfo(SystemTrayInfo info);
		SystemTray();
		~SystemTray();
	};
}
