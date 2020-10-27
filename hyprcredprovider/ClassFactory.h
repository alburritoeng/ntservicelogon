#pragma once
#include <windows.h>
#include <credentialprovider.h>
static long g_cRef = 0;   // global dll reference count
// IClassFactory interface is for each class of object 
// that we need to be instantiated.
// https://docs.microsoft.com/en-us/windows/win32/com/implementing-iclassfactory
class CClassFactory : IClassFactory
{
	
private:
	LONG _cRef;

	CClassFactory() : _cRef(1)
	{
	}

	~CClassFactory()
	{
	}

	void DllAddRef()
	{
		InterlockedIncrement(&g_cRef);
	}

	void DllRelease()
	{
		InterlockedDecrement(&g_cRef);
	}
public:
	// Inherited via IClassFactory
	HRESULT __stdcall QueryInterface(REFIID riid, void ** ppvObject)
	{
		HRESULT hr;
		if (ppvObject != nullptr)
		{
			if (IID_IClassFactory == riid || IID_IUnknown == riid)
			{
				*ppvObject = static_cast<IUnknown*>(this);
				reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();
				hr = S_OK;
			}
			else
			{
				*ppvObject = nullptr;
				hr = E_NOINTERFACE;
			}
		}
		else
		{
			hr = E_INVALIDARG;
		}
		return hr;
	}

	ULONG __stdcall AddRef(void)
	{
		return _cRef++;
	}

	ULONG __stdcall Release(void)
	{
		return 0;
	}

	HRESULT __stdcall CreateInstance(IUnknown * pUnkOuter, REFIID riid, void ** ppvObject)
	{
		HRESULT hr = CLASS_E_NOAGGREGATION;
		if (!pUnkOuter)
		{
			OLECHAR* bstrGuid;
			StringFromCLSID(riid, &bstrGuid);
			
			if (IID_ICredentialProvider == riid)
				hr = CHyprCredentialProvider_CreateInstance(riid, ppvObject); // our wrapped CredProv of the MS Passwod CredProvider
			if (IID_ICredentialProviderFilter == riid)
				hr = CHyperCredProvFilter_CreateInstance(riid, ppvObject); // so we can filter our the MS Password CredProvider
		}
		else
		{
			hr = CLASS_E_NOAGGREGATION;
		}
		return hr;
	}

	HRESULT __stdcall LockServer(BOOL fLock)
	{
		if (fLock)
		{
			DllAddRef();
		}
		else
		{
			DllRelease();
		}
		return S_OK;
	}
};

