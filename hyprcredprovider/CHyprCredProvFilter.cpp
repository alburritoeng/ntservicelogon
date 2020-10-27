#include "pch.h"
#include "CHyprCredProvFilter.h"


HRESULT CHyperCredProvFilter_CreateInstance(REFIID riid, void** ppv)
{	
	HRESULT hr;

	CHyprCredProvFilter* pProviderFilter = new CHyprCredProvFilter();

	if (pProviderFilter)
	{
		hr = pProviderFilter->QueryInterface(riid, ppv);
		pProviderFilter->Release();
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}

	return hr;
}

HRESULT __stdcall CHyprCredProvFilter::QueryInterface(REFIID riid, void ** ppvObject)
{
	if (IID_ICredentialProviderFilter == riid)
	{
		*ppvObject = this;
		reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();
		return S_OK;
	}
	return S_FALSE;
}

ULONG __stdcall CHyprCredProvFilter::AddRef(void)
{
	return _cRef++;
}

ULONG __stdcall CHyprCredProvFilter::Release(void)
{
	LONG cRef = _cRef--;
	if (!cRef)
	{
		delete this;
	}
	return cRef;
}

HRESULT __stdcall CHyprCredProvFilter::Filter(CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus, DWORD dwFlags, GUID * rgclsidProviders, BOOL * rgbAllow, DWORD cProviders)
{
	switch (cpus)
	{
	case CPUS_LOGON: // so credential serialization is handled by the CredProvider for LSA	
	{
		for (unsigned int i = 0; i < cProviders; i++)
		{
			if (!IsEqualGUID(rgclsidProviders[i], CLSID_V1PasswordCredentialProvider))
			{
				rgbAllow[i] = TRUE;
			}
			if (!IsEqualGUID(rgclsidProviders[i], CLSID_PasswordCredentialProvider))
			{
				rgbAllow[i] = TRUE;
			}
			else
			{
				rgbAllow[i] = FALSE;
			}
		}
	}
		break;
	default:
		break;
	}
	return S_OK;
}

HRESULT __stdcall CHyprCredProvFilter::UpdateRemoteCredential(const CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION * pcpcsIn, CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION * pcpcsOut)
{
	// not handling, only going to do LogOn UI for now. 
	return E_UNEXPECTED;
}
