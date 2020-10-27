#pragma once
#include <credentialprovider.h>

class CHyprCredProvWrapper : public ICredentialProvider, ICredentialProviderSetUserArray
{
private:
	ICredentialProvider* pWrappedCredentialProvider;
	short shLoadedMsDefaultCp = 0;
public:
	CHyprCredProvWrapper();
	~CHyprCredProvWrapper();
	// Inherited via ICredentialProvider
	virtual HRESULT __stdcall QueryInterface(REFIID riid, void ** ppvObject) override;
	virtual ULONG __stdcall AddRef(void) override;
	virtual ULONG __stdcall Release(void) override;
	virtual HRESULT __stdcall SetUsageScenario(CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus, DWORD dwFlags) override;
	virtual HRESULT __stdcall SetSerialization(const CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION * pcpcs) override;
	virtual HRESULT __stdcall Advise(ICredentialProviderEvents * pcpe, UINT_PTR upAdviseContext) override;
	virtual HRESULT __stdcall UnAdvise(void) override;
	virtual HRESULT __stdcall GetFieldDescriptorCount(DWORD * pdwCount) override;
	virtual HRESULT __stdcall GetFieldDescriptorAt(DWORD dwIndex, CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR ** ppcpfd) override;
	virtual HRESULT __stdcall GetCredentialCount(DWORD * pdwCount, DWORD * pdwDefault, BOOL * pbAutoLogonWithDefault) override;
	virtual HRESULT __stdcall GetCredentialAt(DWORD dwIndex, ICredentialProviderCredential ** ppcpc) override;

	// will be called by the IClassFactory
	HRESULT CHyprCredentialProvider_CreateInstance(REFIID riid, void** ppv);
	

	// Inherited via ICredentialProviderSetUserArray
	virtual HRESULT __stdcall SetUserArray(ICredentialProviderUserArray * users) override;

};

