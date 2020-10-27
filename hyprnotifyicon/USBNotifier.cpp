#include "USBNotifier.h"
#include <dbt.h>
#include "Logger.h"

namespace HyprTest
{
	// tried and tried to use GUID_DEVINTERFACE_USB_DEVICE but kept getting a linker error,
	// possibly due to missing *.lib, but I didn't see a indication of which, so I copied the GUID here and 
	// this helped me get my USB stick appearing - probably missing something super obvious - PRs would help here :)
	// https://docs.microsoft.com/en-us/windows-hardware/drivers/install/guid-devinterface-usb-device
	GUID AllUsbDevices = { 0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED };

	USBNotifier::USBNotifier(SystemTray* systemTray, HWND hWnd)
	{
		Logger::GetInstance()->Log(L"Initialize USBNotifier");
		_systemTray = systemTray;
		_hWnd = hWnd;
	}

	// <summary>
	// Registers the process for USB Device connections/disconnections
	// </summary>
	// <param name="hWnd">pointer to windows instance</param>
	// <returns>boolean, true if message successfully sent, othewise false</returns>
	bool USBNotifier::RegisterForUsbConnections()
	{
		// Register for device notifications
		DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
		ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
		NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
		NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
		NotificationFilter.dbcc_classguid = AllUsbDevices;

		gDevNotify = RegisterDeviceNotification(_hWnd,
			&NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
		if (gDevNotify == NULL)
		{

			Logger::GetInstance()->Log(L"[ERROR] Error RegisteringForUsbConnections " + std::to_wstring(GetLastError()));

			return FALSE;
		}
		Logger::GetInstance()->Log(L"Successfully Registered for USB Notification");
		if (_systemTray != nullptr)
		{
			_systemTray->ShowPrintJobBalloon(L"Registered", L"Successfully Registered for USB Notification");
		}
		else
		{
			Logger::GetInstance()->Log(L"[ERROR] SystemTray is null, cannot display UI Notification");
		}
		return TRUE;
	}

	// <summary>
	// Handles a new USB Connection event
	// </summary>
	// <param name="lParam">pointer to the LPARAM containing the event info</param>
	// <returns>boolean, true if message successfully parsed, othewise false</returns>
	bool USBNotifier::HandleDeviceConnected(LPARAM lParam)
	{
		std::wstring data;
		bool result = CreateDeviceString(lParam, data);
		if (result)
		{
			Logger::GetInstance()->Log(L"Device Connected: " + data);
			if (_systemTray != nullptr)
			{
				_systemTray->ShowPrintJobBalloon(L"Device Connected", data.c_str());
			}
			else
			{
				Logger::GetInstance()->Log(L"[ERROR] SystemTray is null, cannot display UI Notification");
			}
		}

		return result;
	}

	// <summary>
	// Handles a new USB Removed event
	// </summary>
	// <param name="lParam">pointer to the LPARAM containing the event info</param>
	// <returns>boolean, true if message successfully parsed, othewise false</returns>
	bool USBNotifier::HandleDeviceRemoved(LPARAM lParam)
	{
		std::wstring data;
		bool result = CreateDeviceString(lParam, data);
		if (result)
		{
			Logger::GetInstance()->Log(L"Device Removed: " + data);
			if (_systemTray != nullptr)
			{
				_systemTray->ShowPrintJobBalloon(L"Device Removed", data.c_str());
			}
			else
			{
				Logger::GetInstance()->Log(L"[ERROR] SystemTray is null, cannot display UI Notification");
			}
		}

		return result;
	}

	// <summary>
	// Called to help get the name of the device 
	// </summary>
	// <param name="lParam">pointer to event-specific data </param>
	// <param name="data">buffer to be filled with device name</param>
	// <returns>boolean, true - if the buffer is properly filled, false otherwise, a false does not indicate an error. </returns>
	bool USBNotifier::CreateDeviceString(LPARAM lParam, std::wstring& data)
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

	USBNotifier::~USBNotifier()
	{
		// unregister notifications
		UnregisterDeviceNotification(gDevNotify);
		Logger::GetInstance()->Log(L"Exiting USBNotifier");
	}
}
