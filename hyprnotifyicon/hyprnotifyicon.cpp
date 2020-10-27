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

// mine
HDEVNOTIFY gDevNotify;
UINT const WMAPP_NOTIFYCALLBACK = WM_APP + 1;
// Use a guid to uniquely identify our icon
class __declspec(uuid("63155830-5E4E-4D19-9FA8-62296F5A629F")) HyprNotifyIcon;
HINSTANCE g_hInst = NULL;
wchar_t szWindowClass[] = L"Hypr Notify Icon";
BOOL ShowPrintJobBalloon(HWND hWnd, const wchar_t* msg);
BOOL AddNotificationIcon(HWND hwnd);
BOOL RegisterForUsbConnections(HWND hWnd);
// https://docs.microsoft.com/en-us/windows/win32/devio/registering-for-device-notification


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
	ShowPrintJobBalloon(hWnd, L"Successfully Registered for USB Notification");

	return TRUE;
}


BOOL AddNotificationIcon(HWND hwnd)
{
	NOTIFYICONDATA nid = { sizeof(nid) };
	nid.hWnd = hwnd;
	// add the icon, setting the icon, tooltip, and callback message.
	// the icon will be identified with the GUID
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP | NIF_GUID;
	nid.guidItem = __uuidof(HyprNotifyIcon);
	nid.uCallbackMessage = WMAPP_NOTIFYCALLBACK;
	LoadIconMetric(g_hInst, MAKEINTRESOURCE(IDI_HYPRNOTIFYICON), LIM_SMALL, &nid.hIcon);
	LoadString(g_hInst, IDS_TOOLTIP, nid.szTip, ARRAYSIZE(nid.szTip));
	Shell_NotifyIcon(NIM_ADD, &nid);

	// NOTIFYICON_VERSION_4 is prefered
	nid.uVersion = NOTIFYICON_VERSION_4;
	return Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

BOOL ShowPrintJobBalloon(HWND hWnd, const wchar_t*  msg)
{
	// Display a balloon message for a print job with a custom icon
	NOTIFYICONDATA nid = { sizeof(nid) };
	nid.uFlags = NIF_INFO | NIF_TIP | NIF_GUID | NIF_SHOWTIP;
	nid.guidItem = __uuidof(HyprNotifyIcon);
	nid.dwInfoFlags = NIIF_USER | NIIF_LARGE_ICON;
	LoadString(g_hInst, IDS_APP_TITLE, nid.szInfoTitle, ARRAYSIZE(nid.szInfoTitle));
	StringCchCopy(nid.szInfo, ARRAYSIZE(nid.szInfo), msg);	
	LoadIconMetric(g_hInst, MAKEINTRESOURCE(IDI_HYPRNOTIFYICON), LIM_LARGE, &nid.hBalloonIcon);

	// Show the notification.
	BOOL result= Shell_NotifyIcon(NIM_MODIFY, &nid) ? TRUE : FALSE;

	if (result == FALSE)
	{
		Logger::GetInstance()->Log(L"Error Showing Notification: " + std::to_wstring(GetLastError()));
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

void PrintVolumeInfo(wchar_t driveLetter)
{
	TCHAR volumeName[225];
	TCHAR fileSystemName[225];
	DWORD serialNumber = 0;
	DWORD maxComponentLen = 0;
	DWORD fileSystemFlags = 0;
	if (GetVolumeInformation(_T("c:\\"), volumeName, ARRAYSIZE(volumeName),
		&serialNumber,
		&maxComponentLen,
		&fileSystemFlags,
		fileSystemName,
		ARRAYSIZE(fileSystemName)))
	{
		std::wstring msg(L"Volume Name: ");
		msg.append(volumeName);
		Logger::GetInstance()->Log(msg);

		msg.clear();
		msg.append(L"Serial Number: ");
		msg.append(std::to_wstring(serialNumber));
		Logger::GetInstance()->Log(msg);

		msg.append(L"File System Name:  ");
		msg.append(fileSystemName);
		Logger::GetInstance()->Log(msg);

		msg.clear();
		msg.append(L"Max Component Length:");
		msg.append(std::to_wstring(maxComponentLen));
		Logger::GetInstance()->Log(msg);		
	}
}

std::wstring CreateDeviceString(LPARAM lParam)
{
	DEV_BROADCAST_HDR* deviceInfoHeader = (DEV_BROADCAST_HDR*)lParam;
	if (deviceInfoHeader->dbch_devicetype == DBT_DEVTYP_VOLUME)
	{
		PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lParam;
		if(lpdbv->dbcv_unitmask != 0)
		{
			char i;
			unsigned int umask = lpdbv->dbcv_unitmask;
			for (i = 0; i < 26; ++i)
			{
				if (umask & 0x1)
					break;
				umask = umask >> 1;
			}

			wchar_t volumeLetter = i + L'A';
			Logger::GetInstance()->Log(L"USB device added, DriveLetter: "  + volumeLetter);
			PrintVolumeInfo(volumeLetter);
		}
		
	}
	
	if (deviceInfoHeader->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
	{
		DEV_BROADCAST_DEVICEINTERFACE* lpbd = (DEV_BROADCAST_DEVICEINTERFACE*)lParam;
		
		
		Logger::GetInstance()->Log(lpbd->dbcc_name);
		return lpbd->dbcc_name;
		
	}

	return L"";
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
		if (!AddNotificationIcon(hWnd))
		{			
			return -1;
		}
		if (!RegisterForUsbConnections(hWnd))
		{
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
		case DBT_CUSTOMEVENT:
			ShowPrintJobBalloon(hWnd, L"DBT_CUSTOMEVENT");
			Logger::GetInstance()->Log("Custom Event");
			break;
		case DBT_DEVICEARRIVAL:
			//ShowPrintJobBalloon(hWnd, L"DBT_DEVICEARRIVAL");
			Logger::GetInstance()->Log("Device Arrival");
			{
				std::wstring data;
				bool result= CreateDeviceString(lParam, &data);
				if (result)
				{

					//TODO i'm here!@
					ShowPrintJobBalloon(hWnd, result.c_str());
				}
			}
			break;
		case DBT_DEVICEQUERYREMOVE:
			ShowPrintJobBalloon(hWnd, L"DBT_DEVICEQUERYREMOVE");
			Logger::GetInstance()->Log("Device Removed");
			break;
		case DBT_DEVICEREMOVECOMPLETE:
			ShowPrintJobBalloon(hWnd, L"DBT_DEVICEREMOVECOMPLETE");
			Logger::GetInstance()->Log("Device Removed 2");
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
                DestroyWindow(hWnd);
                break;
			case IDM_SHOWTEXT:
				ShowPrintJobBalloon(hWnd, L"Howdy doo dee!");				
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
		UnregisterDeviceNotification(gDevNotify);
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
