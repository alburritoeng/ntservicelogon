#include "SystemTray.h"
#include <commctrl.h> // NOTIFYICONDATA 
#include <strsafe.h>
#include "Logger.h"
// we need commctrl v6 for LoadIconMetric()
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")

namespace HyprTest
{

	SystemTray::SystemTray()
	{		
		Logger::GetInstance()->Log(L"Initialize SystemTray");
	}

	SystemTray::~SystemTray()
	{
		DeleteNotificationIcon();
		Logger::GetInstance()->Log(L"Existing SystemTray");
	}

	void SystemTray::SetSystemInfo(SystemTrayInfo info)
	{
		_info = info;
	}

	// <summary>
	// Adds the icon from the SystemTray
	// </summary>
	// <param name="hWnd">pointer to windows instance</param>
	// <returns>boolean, true if message successfully sent, othewise false</returns>
	BOOL SystemTray::AddNotificationIcon()
	{
		NOTIFYICONDATA nid = { sizeof(nid) };
		nid.hWnd = _info.hWnd;
		// add the icon, setting the icon, tooltip, and callback message.
		// the icon will be identified with the GUID
		nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP | NIF_GUID;
		nid.guidItem = _info.uuid;
		nid.uCallbackMessage = _info.notifyCallBack;
		nid.uID = _info.notifyIconId;
		LoadIconMetric(_info.instance, MAKEINTRESOURCE(_info.notifyIconId), LIM_SMALL, &nid.hIcon);
		LoadString(_info.instance, _info.toolTipResourceId, nid.szTip, ARRAYSIZE(nid.szTip));
		Shell_NotifyIcon(NIM_ADD, &nid);

		// NOTIFYICON_VERSION_4 is prefered
		nid.uVersion = NOTIFYICON_VERSION_4;
		return Shell_NotifyIcon(NIM_SETVERSION, &nid);
	}

	// <summary>
	// Deletes the icon from the SystemTray
	// </summary>
	// <param name="hWnd">pointer to windows instance</param>
	// <returns>boolean, true if message successfully sent, othewise false</returns>
	BOOL SystemTray::DeleteNotificationIcon()
	{
		NOTIFYICONDATA nid;
		nid.cbSize = sizeof(nid);
		nid.hWnd = _info.hWnd;
		nid.uFlags = NIF_ICON;
		nid.guidItem = _info.uuid;
		nid.uID = _info.notifyIconId;
		BOOL result = Shell_NotifyIcon(NIM_DELETE, &nid);
		return result;
	}

	// <summary>
	// Method to show a notification on the SystemTray with a specific title & message
	// </summary>
	// <param name="hWnd">pointer to windows instance</param>
	// <param name="title">buffer containing the title to display</param>
	// <param name="dmsgata">buffer containing the msg to display</param>
	// <returns>boolean, true if message successfully sent, othewise false</returns>
	bool SystemTray::ShowPrintJobBalloon(const wchar_t* title, const wchar_t*  msg)
	{
		// Display a balloon message for a print job with a custom icon
		NOTIFYICONDATA nid = { sizeof(nid) };
		nid.uFlags = NIF_INFO | NIF_TIP | NIF_GUID | NIF_SHOWTIP;
		nid.guidItem = _info.uuid;
		nid.uID = UI_ID;
		nid.dwInfoFlags = NIIF_USER | NIIF_LARGE_ICON;
		StringCchCopy(nid.szInfoTitle, ARRAYSIZE(nid.szInfoTitle), title);
		StringCchCopy(nid.szInfo, ARRAYSIZE(nid.szInfo), msg);
		LoadIconMetric(_info.instance, MAKEINTRESOURCE(_info.notifyIconId), LIM_LARGE, &nid.hBalloonIcon);

		// Show the notification.
		BOOL result = Shell_NotifyIcon(NIM_MODIFY, &nid) ? TRUE : FALSE;

		if (result == FALSE)
		{
			Logger::GetInstance()->Log(L"Error Showing Notification Tray Icon ");
		}
		return result;
	}
}