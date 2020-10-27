#pragma once
#include <credentialprovider.h>

class CHyprCredProvFilter : ICredentialProviderFilter
{
private:
	LONG _cRef;
public:
	// Inherited via ICredentialProviderFilter
	virtual HRESULT __stdcall QueryInterface(REFIID riid, void ** ppvObject) override;
	virtual ULONG __stdcall AddRef(void) override;
	virtual ULONG __stdcall Release(void) override;
	virtual HRESULT __stdcall Filter(CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus, DWORD dwFlags, GUID * rgclsidProviders, BOOL * rgbAllow, DWORD cProviders) override;
	virtual HRESULT __stdcall UpdateRemoteCredential(const CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION * pcpcsIn, CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION * pcpcsOut) override;

	// for IClassFactory
	HRESULT CHyperCredProvFilter_CreateInstance(REFIID riid, __deref_out void** ppv);
};

