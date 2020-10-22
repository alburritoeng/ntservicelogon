#include "WmiEventNotifier.h"
#include "EventSink.h"
#include "Logger.h"

IUnsecuredApartment* pUnsecApp = nullptr;
IUnknown* pStubUnk = nullptr;
EventSink* pSink = new EventSink;
IWbemObjectSink* pStubSink = nullptr;
IWbemServices* pSvc = nullptr;
IWbemLocator* pLoc = nullptr;
Logger* wmiLog = Logger::GetInstance();
int InitializeCOM()
{
	wmiLog->Log("InitializeCOM");
	HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hres))
	{
		cout << "Failed to initialize COM library. Error code = 0x"
			<< hex << hres << endl;
		return 1;                  // Program has failed.
	}

	return hres;
}

int SetGeneralComSecurityLevel()
{
	wmiLog->Log("SetGeneralComSecurityLevel");
	HRESULT hres = CoInitializeSecurity(
		NULL,
		-1,                          // COM negotiates service
		NULL,                        // Authentication services
		NULL,                        // Reserved
		RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
		RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
		NULL,                        // Authentication info
		EOAC_NONE,                   // Additional capabilities 
		NULL                         // Reserved
	);


	if (FAILED(hres))
	{
		std::wstring msg(L"Failed to initialize security. Error code = ");			
		msg.append(std::to_wstring(hres));
		wmiLog->Log(msg);

		CoUninitialize();
		return 1;                      // Program has failed.
	}
}

int ObtainWmiLocator()
{	
	pLoc = nullptr;
	wmiLog->Log("ObtainWmiLocator");
	HRESULT hres = CoCreateInstance(
		CLSID_WbemLocator,
		0,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator, (LPVOID *)&pLoc);

	if (FAILED(hres))
	{		
		std::wstring msg(L"Failed to create IWbemLocator object. Error code = ");
		msg.append(std::to_wstring(hres));
		wmiLog->Log(msg);

		CoUninitialize();
		return 1;                 // Program has failed.
	}

	return hres;
}

int ConnectToWmi()
{
	pSvc = nullptr;
	wmiLog->Log("ConnectToWmi");
	// Connect to the local root\cimv2 namespace
	// and obtain pointer pSvc to make IWbemServices calls.
	HRESULT hres = pLoc->ConnectServer(
		_bstr_t(L"ROOT\\CIMV2"),
		NULL,
		NULL,
		0,
		NULL,
		0,
		0,
		&pSvc
	);

	if (FAILED(hres))
	{
		std::wstring msg(L"Could not connect.Error code = ");
		msg.append(std::to_wstring(hres));
		wmiLog->Log(msg);
		pLoc->Release();
		CoUninitialize();
		return 1;                // Program has failed.
	}

	return hres;
}

int SetSecurityLevelsOnProxy()
{
	wmiLog->Log("SetSecurityLevelsOnProxy");
	HRESULT hres = CoSetProxyBlanket(
		pSvc,                        // Indicates the proxy to set
		RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx 
		RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx 
		NULL,                        // Server principal name 
		RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
		RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
		NULL,                        // client identity
		EOAC_NONE                    // proxy capabilities 
	);

	if (FAILED(hres))
	{		
		std::wstring msg(L"Could not set proxy blanket.  Error code = ");
		msg.append(std::to_wstring(hres));
		wmiLog->Log(msg);
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return 1;               // Program has failed.
	}
	return hres;
}

int ReceiveEventNotifications()
{
	// Use an unsecured apartment for security
	wmiLog->Log("ReceiveEventNotifications");

	HRESULT hres = CoCreateInstance(CLSID_UnsecuredApartment, NULL,
		CLSCTX_LOCAL_SERVER, IID_IUnsecuredApartment,
		(void**)&pUnsecApp);

	
	pSink->AddRef();

	
	pUnsecApp->CreateObjectStub(pSink, &pStubUnk);

	
	pStubUnk->QueryInterface(IID_IWbemObjectSink,
		(void **)&pStubSink);

	// The ExecNotificationQueryAsync method will call
	// The EventQuery::Indicate method when an event occurs
	hres = pSvc->ExecNotificationQueryAsync(
		_bstr_t("WQL"),
		_bstr_t("SELECT * "
			"FROM __InstanceCreationEvent WITHIN 1 "
			"WHERE TargetInstance ISA 'Win32_Process' and TargetInstance.Name = 'notepad.exe'"),
		WBEM_FLAG_SEND_STATUS,
		NULL,
		pStubSink);

	return hres;
}

void WmiEventNotifier::Unsubscribe()
{
	wmiLog->Log("Unsubscribe");
	if (pSvc != nullptr)
	{
		HRESULT hres = pSvc->CancelAsyncCall(pStubSink);
	}
}

WmiEventNotifier::WmiEventNotifier()
{
	HRESULT hres;
	wmiLog->Log("WmiEventNotifier");
	// Step 1: --------------------------------------------------
	// Initialize COM. ------------------------------------------

	hres = InitializeCOM();
	if (FAILED(hres))
	{
		throw;
	}
	
	wmiLog->Log("InitializeCOM - success");

	// Step 2: --------------------------------------------------
	// Set general COM security levels --------------------------
	hres = SetGeneralComSecurityLevel();
	
	if (FAILED(hres))
	{
		throw;
	}

	wmiLog->Log("SetGeneralComSecurityLevel - success");
	// Step 3: ---------------------------------------------------
	// Obtain the initial locator to WMI -------------------------	
	hres = ObtainWmiLocator();

	if (FAILED(hres))
	{
		throw;
	}
	wmiLog->Log("ObtainWmiLocator - success");
	// Step 4: ---------------------------------------------------
	// Connect to WMI through the IWbemLocator::ConnectServer method	
	hres = ConnectToWmi();
	
	if (FAILED(hres))
	{
		throw;
	}
	wmiLog->Log("Connected to ROOT\\CIMV2 WMI namespace");

	// Step 5: --------------------------------------------------
	// Set security levels on the proxy -------------------------
	hres = SetSecurityLevelsOnProxy();
	if (FAILED(hres))
	{
		throw;
	}

	wmiLog->Log("SetSecurityLevelsOnProxy");
	// Step 6: -------------------------------------------------
	// Receive event notifications -----------------------------
	hres = ReceiveEventNotifications();

	// Check for errors.
	if (FAILED(hres))
	{
		std::wstring msg(L"ExecNotificationQueryAsync failed with = ");
		msg.append(std::to_wstring(hres));
		wmiLog->Log(msg);

		pSvc->Release();
		pLoc->Release();
		pUnsecApp->Release();
		pStubUnk->Release();
		pSink->Release();
		pStubSink->Release();
		CoUninitialize();	
	}	

	return;   // Program successfully completed.
}

WmiEventNotifier::~WmiEventNotifier()
{
	// Wait for the event
	wmiLog->Log("~WmiEventNotifier");
	if (pSvc != nullptr)
	{
		HRESULT hres = pSvc->CancelAsyncCall(pStubSink);
	}

	// Cleanup
	// ========
	if (pSvc != nullptr)
	{
		pSvc->Release();
	}
	if (pLoc != nullptr)
	{
		pLoc->Release();
	}
	if (pUnsecApp != nullptr)
	{
		pUnsecApp->Release(); 
	}
	if (pStubUnk != nullptr)
	{
		pStubUnk->Release();
	}
	if (pSink != nullptr)
	{
		pSink->Release();
	}
	if (pStubSink != nullptr)
	{
		pStubSink->Release();
	}
	CoUninitialize();
}