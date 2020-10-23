// hyprsvc.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <windows.h>
#include "constants.h"
#include <sstream>
#include <wchar.h>
#include <strsafe.h>
#include <cstdio>
#include <thread>
#include "Logger.h"
#include "WmiEventNotifier.h"
#include "ImpersonateUser.h"

void InstallNtService(void);
void ServiceMain(DWORD dwArgc, PWSTR* pszArgv);
void SvcReportEvent(LPCWSTR);
void InitializeService(DWORD dwArgc, LPTSTR *lpszArgv);
void SpawnProcess(LPVOID lpEventData);

Logger* logger = Logger::GetInstance();
SERVICE_STATUS          gSvcStatus;
SERVICE_STATUS_HANDLE   gSvcStatusHandle;
HANDLE                  ghSvcStopEvent = nullptr;
HANDLE					ghStopWaitHandle = nullptr;
VOID ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
#define SVC_ERROR                        ((DWORD)0xC0020001L)
WmiEventNotifier* wmiProcessNotifier = nullptr;

bool g_showCrashDialog = false;

int wmain(int argc, wchar_t* argv[])
{
	//while (!::IsDebuggerPresent())
	//	::Sleep(100); // to avoid 100% CPU load
    // check the argument passed in, was it "install" ?
	// if it is install, then we install it, otherwise this service app was launched by scm	
	logger->Log("Main");	

	if (argc > 1 && std::wcscmp(argv[1], L"install") == 0)
	{
		logger->Log("Install Service\n");
		InstallNtService();
	}
	
	// starting the service	 
	SERVICE_TABLE_ENTRY DispatchTable[] =
	{
		{ const_cast <LPWSTR>(hypr_svc_name), (LPSERVICE_MAIN_FUNCTION)ServiceMain },
		{ NULL, NULL }
	};

	// This call returns when the service has stopped. 
	// The process should simply terminate when the call returns.
	if (!StartServiceCtrlDispatcher(DispatchTable))
	{
		std::wstring msg(L"StartServiceCtrlDispatcher GetLastError:");
		msg.append(std::to_wstring(GetLastError()));
		logger->Log(msg);
		SvcReportEvent(L"StartServiceCtrlDispatcher");
	}
}

void SpawnProcess(LPVOID lpEventData)
{
	logger->Log("Attempting to spawn Notepad process for user");
	WTSSESSION_NOTIFICATION *sessionNotification = static_cast<WTSSESSION_NOTIFICATION*>(lpEventData);

	if (ImpersonateUser::IsInteractive(sessionNotification->dwSessionId) != UserSessionInfo::INTERACTIVE)
	{
		logger->Log("Non-User Interactive session, will not spawn notepad.exe");
		return;
	}

	bool result = ImpersonateUser::CreateNotepadAsUser(sessionNotification->dwSessionId);
	if (result)
	{
		logger->Log("Spawn process succeeded");
		return;
	}
	logger->Log("Spawn process FAILED");
}

DWORD WINAPI ServiceCtrlHandlerEx(DWORD  dwControl, DWORD  dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
	// Handle the requested control code. 

	switch (dwControl)
	{
	case SERVICE_CONTROL_SESSIONCHANGE:
		logger->Log("Session Change");
		switch (dwEventType) {
			case WTS_SESSION_LOGON:

				logger->Log("User logged on");

				wmiProcessNotifier->ReceiveNotifications();
				SpawnProcess(lpEventData);
				break;
			case WTS_SESSION_LOGOFF:
				logger->Log("User logged off");
				wmiProcessNotifier->Unsubscribe();
				break;
			case WTS_SESSION_LOCK:
				logger->Log("User locked machine");
				break;
			case WTS_SESSION_UNLOCK:
				logger->Log("User unlocked machine");
				break;
			}
		break;
	case SERVICE_CONTROL_STOP:
	case SERVICE_CONTROL_SHUTDOWN:

		logger->Log("Stop received");
		ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

		// Signal the service to stop.
		SetEvent(ghSvcStopEvent);
		ReportSvcStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);

		return NO_ERROR;

	case SERVICE_CONTROL_INTERROGATE:
		break;

	default:
		break;
	}

	return NO_ERROR;
}

// taken from https://docs.microsoft.com/en-us/windows/win32/services/svc-cpp
VOID ReportSvcStatus(DWORD dwCurrentState,	DWORD dwWin32ExitCode,	DWORD dwWaitHint)
{
	static DWORD dwCheckPoint = 1;

	// Fill in the SERVICE_STATUS structure.

	gSvcStatus.dwCurrentState = dwCurrentState;
	gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
	gSvcStatus.dwWaitHint = dwWaitHint;

	if (dwCurrentState == SERVICE_START_PENDING)
		gSvcStatus.dwControlsAccepted = 0;
	else gSvcStatus.dwControlsAccepted = (SERVICE_ACCEPT_SHUTDOWN|SERVICE_ACCEPT_STOP| SERVICE_ACCEPT_SESSIONCHANGE);

	if ((dwCurrentState == SERVICE_RUNNING) ||
		(dwCurrentState == SERVICE_STOPPED))
		gSvcStatus.dwCheckPoint = 0;
	else gSvcStatus.dwCheckPoint = dwCheckPoint++;

	// Report the status of the service to the SCM.
	SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}


void ServiceMain(DWORD dwArgc, PWSTR* pszArgv)
{
	// call RegisterServceCtrlHandlerEx
	
	// Register the handler function for the service
	// passing in nullptr for final arg, no context, no multiple services sharing a process
	gSvcStatusHandle = RegisterServiceCtrlHandlerEx(hypr_svc_name, ServiceCtrlHandlerEx, nullptr);
	
	logger->Log("ServiceMain");
	if (!gSvcStatusHandle)
	{
		SvcReportEvent(L"RegisterServiceCtrlHandler");
		return;
	}

	// These SERVICE_STATUS members remain as set here

	gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	gSvcStatus.dwServiceSpecificExitCode = 0;

	// Report initial status to the SCM

	ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

	// Perform service-specific initialization and work.
	InitializeService(dwArgc, pszArgv);	
}

// taken from https://docs.microsoft.com/en-us/windows/win32/services/svc-cpp
// 
VOID SvcReportEvent(LPCWSTR szFunction)
{
	HANDLE hEventSource;
	LPCWSTR lpszStrings[2];
	wchar_t Buffer[80];

	hEventSource = RegisterEventSource(nullptr, hypr_svc_name);

	if (hEventSource != nullptr)
	{
		StringCchPrintf(Buffer, 80, TEXT("%s failed with %d"), szFunction, GetLastError());
		logger->Log(Buffer);

		lpszStrings[0] = hypr_svc_name;
		lpszStrings[1] = Buffer;

		ReportEvent(hEventSource,// event log handle
			EVENTLOG_ERROR_TYPE, // event type
			0,                   // event category
			SVC_ERROR,           // event identifier
			NULL,                // no security identifier
			2,                   // size of lpszStrings array
			0,                   // no binary data
			lpszStrings,		 // array of strings
			NULL);               // no binary data

		DeregisterEventSource(hEventSource);
	}
}

void InstallNtService()
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;

	wchar_t svcPath[MAX_PATH];

	if (!GetModuleFileName(nullptr, svcPath, MAX_PATH))
	{		
		std::wstring msg(L"Cannot install service LastError: ");
		msg.append(std::to_wstring(GetLastError()));
		logger->Log(msg);
		return;
	}

	// Get a handle to the SCM database. 

	schSCManager = OpenSCManager(
		nullptr,                    // local computer
		nullptr,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS | SC_MANAGER_CREATE_SERVICE);  // full access rights  

	if (schSCManager == nullptr)
	{		
		std::wstring msg(L"OpenSCManager failed %d", GetLastError());
		logger->Log(msg);
		return;
	}

	// Create the service

	schService = CreateService(
		schSCManager,              // SCM database 
		hypr_svc_name,             // name of service 
		hypr_svc_name,             // service name to display 
		SERVICE_ALL_ACCESS,        // desired access 
		SERVICE_WIN32_OWN_PROCESS, // service type 
		SERVICE_AUTO_START,      // start type 
		SERVICE_ERROR_NORMAL,      // error control type 
		svcPath,                    // path to service's binary 
		nullptr,                      // no load ordering group 
		nullptr,                      // no tag identifier 
		nullptr,                      // no dependencies 
		nullptr,                      // LocalSystem account 
		nullptr);                     // no password 

	if (schService == nullptr)
	{		
		std::wstring msg(L"CreateService failed %d", GetLastError());
		logger->Log(msg);
		CloseServiceHandle(schSCManager);
		return;
	}
	else
	{		
		logger->Log("Service installed successfully");
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}

VOID CALLBACK StopCallback(PVOID svc, BOOLEAN timeout)
{
	logger->Log("Stop Called");
	// close stop event
	CloseHandle(ghSvcStopEvent);
	ghSvcStopEvent = nullptr;

	// unregister new stop event wait handle.
	UnregisterWait(ghStopWaitHandle);
	ghStopWaitHandle = nullptr;

	// Tell SCM that the service is stopped.
	ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 3000);
}

void WmiNotePadNotificationTask()
{
	DWORD dwWaitForStop;
	wmiProcessNotifier = new WmiEventNotifier();
	dwWaitForStop = WaitForSingleObject(ghSvcStopEvent, INFINITE);
	logger->Log("Stop event received, unsubscribing from Wmi Events");
	wmiProcessNotifier->Unsubscribe();

}

void InitializeService(DWORD dwArgc, LPTSTR *lpszArgv)
{
	// Create an event. The control handler function, SvcCtrlHandler,
   // signals this event when it receives the stop control code.

	ghSvcStopEvent = CreateEvent(
		NULL,    // default security attributes
		TRUE,    // manual reset event
		FALSE,   // not signaled
		NULL);   // no name

	if (ghSvcStopEvent == NULL)
	{
		ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
		return;
	}

	logger->Log("Stop Event created");
	if (!RegisterWaitForSingleObject(&ghStopWaitHandle, ghSvcStopEvent, StopCallback, nullptr, INFINITE, WT_EXECUTEDEFAULT)) 
	{
		std::string msg("RegisterWaitForSingleObject failed(%d", GetLastError());
		logger->Log(msg);
		throw new std::exception("RegisterWaitForSingleObject failed: ", GetLastError());
	}

	// Report running status when initialization is complete.
	ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

	//spawn a thread to do work... it gets notified to stop by the Stop Event handle ghSvcStopEvent
	std::thread notepad_worker(WmiNotePadNotificationTask);
	notepad_worker.join();
}