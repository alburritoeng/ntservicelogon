// hyprnotifyicon.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include <shellapi.h>
#include <commctrl.h> // NOTIFYICONDATA 
#include <dbt.h>
#include <strsafe.h>
#include "hyprnotifyicon.h"
#include "SystemTray.h"
#include "USBNotifier.h"
#include "Logger.h"
#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text

// Using a SystemTray Object & a USBNotifier Object
// Both instantiated here, but USBNotifier is injected the SystemTray object as instantiation
// means, this class is responsible for cleanup of both objects
using namespace HyprTest;
SystemTray* systemTray;
USBNotifier* usbNotifier;

UINT const WMAPP_NOTIFYCALLBACK = WM_APP + 1;
// Use a guid to uniquely identify our icon
class __declspec(uuid("63155830-5E4E-4D19-9FA8-62296F5A629F")) HyprNotifyIcon;
HINSTANCE g_hInst = NULL;
wchar_t szWindowClass[] = L"Hypr Notify Icon";

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

   // The SystemTray object
   systemTray = new HyprTest::SystemTray();

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

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_CREATE:
		// add the notification icon
		Logger::GetInstance()->Log(L"Adding Notification Icon to System Tray");
		// a structure for ease of passing all this data to SystemTray
		HyprTest::SystemTrayInfo info;
		info.hWnd = hWnd;
		info.instance = hInst;
		info.uuid = __uuidof(HyprNotifyIcon);
		info.notifyIconId = IDI_HYPRNOTIFYICON;
		info.toolTipResourceId = IDS_TOOLTIP;
		info.notifyCallBack = WMAPP_NOTIFYCALLBACK;
		systemTray->SetSystemInfo(info);
		// The USBNotifer object
		usbNotifier = new HyprTest::USBNotifier(systemTray, hWnd);
		
		if (!systemTray->AddNotificationIcon())
		{			
			Logger::GetInstance()->Log(L"[ERROR] Failed to add Notification Icon to System Tray");
			return -1;
		}
		Logger::GetInstance()->Log(L"Registering for USB Device Notifications");
		if (!usbNotifier->RegisterForUsbConnections())
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
			
			usbNotifier->HandleDeviceConnected(lParam);
			
			break;		
		case DBT_DEVICEREMOVECOMPLETE:
			
			usbNotifier->HandleDeviceRemoved(lParam);
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
		delete usbNotifier;
		Logger::GetInstance()->Log(L"Cleanup Icon from System Tray");
		delete systemTray;
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
