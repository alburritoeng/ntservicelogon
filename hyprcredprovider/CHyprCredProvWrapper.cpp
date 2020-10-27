#include "pch.h"
#include "CHyprCredProvWrapper.h"
#include <strsafe.h>
#include <sstream>
#include "Logger.h"

HRESULT CHyprCredentialProvider_CreateInstance(REFIID riid, void** ppv)
{
	HRESULT hr;
	Logger::GetInstance()->Log(L"CHyprCredentialProvider_CreateInstance");

	CHyprCredProvWrapper* pProvider = new CHyprCredProvWrapper();

	if (pProvider)
	{
		hr = pProvider->QueryInterface(riid, ppv);
		pProvider->Release();
	}
	else
	{
		Logger::GetInstance()->Log(L"[ERROR] CHyprCredentialProvider_CreateInstance OUTOFMEMORY");
		hr = E_OUTOFMEMORY;
	}

	Logger::GetInstance()->Log(L"CHyprCredentialProvider_CreateInstance Success");
	return hr;
}

CHyprCredProvWrapper::CHyprCredProvWrapper()
{
	pWrappedCredentialProvider = nullptr;
		
	Logger::GetInstance()->Log(L"CHyprCredProvWrapper()");
	// Create the password credential provider and query its interface for an ICredentialProvider.	
	IUnknown* pUnknown = nullptr; // will assign this hopefully to our global ICredentialProvider instance
	HRESULT hr = CoCreateInstance(CLSID_PasswordCredentialProvider, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pUnknown));
	if (!SUCCEEDED(hr))
	{
		hr = CoCreateInstance(CLSID_V1PasswordCredentialProvider, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pUnknown));
	}

	if (SUCCEEDED(hr))
	{
		hr = pUnknown->QueryInterface(IID_PPV_ARGS(&(pWrappedCredentialProvider)));
		if (SUCCEEDED(hr))
		{			
			shLoadedMsDefaultCp = 1;
			Logger::GetInstance()->Log(L"Successfully loaded PasswordCredentialProvider");
		}	
	}
}

CHyprCredProvWrapper::~CHyprCredProvWrapper()
{
	if (pWrappedCredentialProvider != nullptr)
	{
		pWrappedCredentialProvider->Release();
	}
}

HRESULT __stdcall CHyprCredProvWrapper::QueryInterface(REFIID riid, void ** ppvObject)
{

	wchar_t* guidStr;
	StringFromIID(riid, &guidStr);

	std::wstringstream msg;
	msg << L"[INFO] CHyprCredProvWrapper::QueryInterface: ";
	msg << L"REFIID=" << *guidStr;
	
	Logger::GetInstance()->Log(msg.str());
	return pWrappedCredentialProvider->QueryInterface(riid, ppvObject);
}

ULONG __stdcall CHyprCredProvWrapper::AddRef(void)
{
	std::wstringstream msg;
	msg << L"[INFO] CHyprCredProvWrapper::AddRef";	
	Logger::GetInstance()->Log(msg.str());

	return pWrappedCredentialProvider->AddRef();
}

ULONG __stdcall CHyprCredProvWrapper::Release(void)
{
	std::wstringstream msg;
	msg << L"[INFO] CHyprCredProvWrapper::Release";
	Logger::GetInstance()->Log(msg.str());

	return pWrappedCredentialProvider->Release();
}

HRESULT __stdcall CHyprCredProvWrapper::SetUsageScenario(CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus, DWORD dwFlags)
{
	std::wstringstream msg;
	msg << L"[INFO] CHyprCredProvWrapper::SetUsageScenario";
	Logger::GetInstance()->Log(msg.str());

	return pWrappedCredentialProvider->SetUsageScenario(cpus, dwFlags);
}

HRESULT __stdcall CHyprCredProvWrapper::SetSerialization(const CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION * pcpcs)
{
	return pWrappedCredentialProvider->SetSerialization(pcpcs);
}

HRESULT __stdcall CHyprCredProvWrapper::Advise(ICredentialProviderEvents * pcpe, UINT_PTR upAdviseContext)
{
	return pWrappedCredentialProvider->Advise(pcpe, upAdviseContext);
}

HRESULT __stdcall CHyprCredProvWrapper::UnAdvise(void)
{
	return pWrappedCredentialProvider->UnAdvise();
}

HRESULT __stdcall CHyprCredProvWrapper::GetFieldDescriptorCount(DWORD * pdwCount)
{
	return pWrappedCredentialProvider->GetFieldDescriptorCount(pdwCount);
}

HRESULT __stdcall CHyprCredProvWrapper::GetFieldDescriptorAt(DWORD dwIndex, CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR ** ppcpfd)
{
	return pWrappedCredentialProvider->GetFieldDescriptorAt(dwIndex, ppcpfd);
}

HRESULT __stdcall CHyprCredProvWrapper::GetCredentialCount(DWORD * pdwCount, DWORD * pdwDefault, BOOL * pbAutoLogonWithDefault)
{
	return pWrappedCredentialProvider->GetCredentialCount(pdwCount, pdwDefault, pbAutoLogonWithDefault);
}

HRESULT __stdcall CHyprCredProvWrapper::GetCredentialAt(DWORD dwIndex, ICredentialProviderCredential ** ppcpc)
{
	return pWrappedCredentialProvider->GetCredentialAt(dwIndex, ppcpc);
}

HRESULT __stdcall CHyprCredProvWrapper::SetUserArray(ICredentialProviderUserArray * users)
{
	auto res = dynamic_cast<ICredentialProviderSetUserArray*>(pWrappedCredentialProvider);
	if (res != nullptr)
	{
		return res->SetUserArray(users);
	}

	return 0;
}
