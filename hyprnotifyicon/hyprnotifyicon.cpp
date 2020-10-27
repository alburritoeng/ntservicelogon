// hyprnotifyicon.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include <shellapi.h>
#include <commctrl.h> // NOTIFYICONDATA 
#include <dbt.h>
#include <strsafe.h>
//#include <Usbiodef.h>
//#include <initguid.h>
#include "hyprnotifyicon.h"
#include "Logger.h"
#define MAX_LOADSTRING 100

// we need commctrl v6 for LoadIconMetric()
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text


HDEVNOTIFY gDevNotify;
UINT const WMAPP_NOTIFYCALLBACK = WM_APP + 1;
// Use a guid to uniquely identify our icon
class __declspec(uuid("63155830-5E4E-4D19-9FA8-62296F5A629F")) HyprNotifyIcon;
HINSTANCE g_hInst = NULL;
wchar_t szWindowClass[] = L"Hypr Notify Icon";
BOOL ShowPrintJobBalloon(HWND hWnd, const wchar_t* title, const wchar_t*  msg);
BOOL AddNotificationIcon(HWND hWnd);
BOOL DeleteNotificationIcon(HWND hWnd);
BOOL RegisterForUsbConnections(HWND hWnd);
//tried and tried to use GUID_DEVINTERFACE_USB_DEVICE but kept getting a linker error,
//possibly due to missing *.lib, but I didn't see a indication of which, so I copied the GUID here and 
//this helped me get my USB stick appearing
//https://docs.microsoft.com/en-us/windows-hardware/drivers/install/guid-devinterface-usb-device
GUID AllUsbDevices= {0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED};

// end-mine
// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.
	g_hInst = hInstance;	
    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_HYPRNOTIFYICON, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, false))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_HYPRNOTIFYICON));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_HYPRNOTIFYICON));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_HYPRNOTIFYICON);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

/// <summary>
/// Registers the process for USB Device connections/disconnections
/// </summary>
/// <param name="hWnd">pointer to windows instance</param>
/// <returns>boolean, true if message successfully sent, othewise false</returns>
BOOL RegisterForUsbConnections(HWND hWnd)
{
	// Register for device notifications
	DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
	ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
	NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	NotificationFilter.dbcc_classguid = AllUsbDevices;

	gDevNotify = RegisterDeviceNotification(hWnd,
		&NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
	if (gDevNotify == NULL)
	{

		Logger::GetInstance()->Log(L"Error RegisteringForUsbConnections " + std::to_wstring(GetLastError()));

		return FALSE;
	}
	Logger::GetInstance()->Log(L"Successfully Registered for USB Notification");
	ShowPrintJobBalloon(hWnd, L"Registered", L"Successfully Registered for USB Notification");

	return TRUE;
}

/// <summary>
/// Adds the icon from the SystemTray
/// </summary>
/// <param name="hWnd">pointer to windows instance</param>
/// <returns>boolean, true if message successfully sent, othewise false</returns>
BOOL AddNotificationIcon(HWND hwnd)
{
	NOTIFYICONDATA nid = { sizeof(nid) };	
	nid.hWnd = hwnd;
	// add the icon, setting the icon, tooltip, and callback message.
	// the icon will be identified with the GUID
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP | NIF_GUID;
	nid.guidItem = __uuidof(HyprNotifyIcon);
	nid.uCallbackMessage = WMAPP_NOTIFYCALLBACK;
	nid.uID = IDI_HYPRNOTIFYICON;
	LoadIconMetric(g_hInst, MAKEINTRESOURCE(IDI_HYPRNOTIFYICON), LIM_SMALL, &nid.hIcon);
	LoadString(g_hInst, IDS_TOOLTIP, nid.szTip, ARRAYSIZE(nid.szTip));
	Shell_NotifyIcon(NIM_ADD, &nid);

	// NOTIFYICON_VERSION_4 is prefered
	nid.uVersion = NOTIFYICON_VERSION_4;
	return Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

/// <summary>
/// Deletes the icon from the SystemTray
/// </summary>
/// <param name="hWnd">pointer to windows instance</param>
/// <returns>boolean, true if message successfully sent, othewise false</returns>
BOOL DeleteNotificationIcon(HWND hWnd)
{
	NOTIFYICONDATA nid;	
	nid.cbSize = sizeof(nid);
	nid.hWnd = hWnd;
	nid.uFlags = NIF_ICON;
	nid.guidItem = __uuidof(HyprNotifyIcon);
	nid.uID = IDI_HYPRNOTIFYICON;
	BOOL result = Shell_NotifyIcon(NIM_DELETE, &nid);
	return result;
}

/// <summary>
/// Method to show a notification on the SystemTray with a specific title & message
/// </summary>
/// <param name="hWnd">pointer to windows instance</param>
/// <param name="title">buffer containing the title to display</param>
/// <param name="dmsgata">buffer containing the msg to display</param>
/// <returns>boolean, true if message successfully sent, othewise false</returns>
BOOL ShowPrintJobBalloon(HWND hWnd, const wchar_t* title, const wchar_t*  msg)
{
	// Display a balloon message for a print job with a custom icon
	NOTIFYICONDATA nid = { sizeof(nid) };	
	nid.uFlags = NIF_INFO | NIF_TIP | NIF_GUID | NIF_SHOWTIP;
	nid.guidItem = __uuidof(HyprNotifyIcon);
	nid.uID = IDI_HYPRNOTIFYICON;
	nid.dwInfoFlags = NIIF_USER | NIIF_LARGE_ICON;	
	StringCchCopy(nid.szInfoTitle, ARRAYSIZE(nid.szInfoTitle), title);
	StringCchCopy(nid.szInfo, ARRAYSIZE(nid.szInfo), msg);
	LoadIconMetric(g_hInst, MAKEINTRESOURCE(IDI_HYPRNOTIFYICON), LIM_LARGE, &nid.hBalloonIcon);

	// Show the notification.
	BOOL result = Shell_NotifyIcon(NIM_MODIFY, &nid) ? TRUE : FALSE;

	if (result == FALSE)
	{
		Logger::GetInstance()->Log(L"Error Showing Notification Tray Icon ");
	}
	return result;

};

void ShowContextMenu(HWND hwnd, POINT pt)
{
	HMENU hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDC_CONTEXTMENU));
	if (hMenu)
	{
		HMENU hSubMenu = GetSubMenu(hMenu, 0);
		if (hSubMenu)
		{
			// our window must be foreground before calling TrackPopupMenu or the menu will not disappear when the user clicks away
			SetForegroundWindow(hwnd);

			// respect menu drop alignment
			UINT uFlags = TPM_RIGHTBUTTON;
			if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0)
			{
				uFlags |= TPM_RIGHTALIGN;
			}
			else
			{
				uFlags |= TPM_LEFTALIGN;
			}

			TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hwnd, NULL);
		}
		DestroyMenu(hMenu);
	}
}

/// <summary>
/// Called to help get the name of the device 
/// </summary>
/// <param name="lParam">pointer to event-specific data </param>
/// <param name="data">buffer to be filled with device name</param>
/// <returns>boolean, true - if the buffer is properly filled, false otherwise, a false does not indicate an error. </returns>
bool CreateDeviceString(LPARAM lParam, std::wstring& data)
{
	DEV_BROADCAST_HDR* deviceInfoHeader = (DEV_BROADCAST_HDR*)lParam;	
	
	if (deviceInfoHeader->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
	{		
		DEV_BROADCAST_DEVICEINTERFACE* lpbd = (DEV_BROADCAST_DEVICEINTERFACE*)lParam;		
		data = lpbd->dbcc_name;
		return true;		
	}
	return false;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_CREATE:
		// add the notification icon
		Logger::GetInstance()->Log(L"Adding Notification Icon to System Tray");
		if (!AddNotificationIcon(hWnd))
		{			
			Logger::GetInstance()->Log(L"[ERROR] Failed to add Notification Icon to System Tray");
			return -1;
		}
		Logger::GetInstance()->Log(L"Registering for USB Device Notifications");
		if (!RegisterForUsbConnections(hWnd))
		{
			Logger::GetInstance()->Log(L"[ERROR] Failed to register for USB Device Notifications");
			return -1;
		}
		break;
	case WMAPP_NOTIFYCALLBACK:
		switch (LOWORD(lParam))
		{
			case WM_CONTEXTMENU:
			{
				POINT const pt = { LOWORD(wParam), HIWORD(wParam) };
				ShowContextMenu(hWnd, pt);
			}
			break;
		}
		break;
	case WM_DEVICECHANGE:
	{
		
		switch (wParam)
		{

		case DBT_DEVICEARRIVAL:						
			{
				std::wstring data;
				bool result= CreateDeviceString(lParam, data);
				if (result)
				{
					Logger::GetInstance()->Log(L"Device Connected: " + data);
					ShowPrintJobBalloon(hWnd, L"Device Connected", data.c_str());
				}
			}
			break;		
		case DBT_DEVICEREMOVECOMPLETE:
			std::wstring data;
			bool result = CreateDeviceString(lParam, data);
			if (result)
			{
				Logger::GetInstance()->Log(L"Device Removed: " + data);
				ShowPrintJobBalloon(hWnd, L"Device Removed", data.c_str());
			}
		//case should there be more handled? 
			break;
		}

	}
	break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
				Logger::GetInstance()->Log(L"Exit application");
                DestroyWindow(hWnd);
                break;			
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
		Logger::GetInstance()->Log(L"Unregister for Device Notifications");
		UnregisterDeviceNotification(gDevNotify);
		Logger::GetInstance()->Log(L"Cleanup Icon from System Tray");
		DeleteNotificationIcon(hWnd);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
