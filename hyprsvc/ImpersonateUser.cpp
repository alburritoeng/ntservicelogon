#include "ImpersonateUser.h"
#include <wtsapi32.h>
#include <ntsecapi.h>
#include <accctrl.h> // SE_FILE_OBJECT
#include <aclapi.h> // GetNamedSecurityInfo
#include <stdio.h>
#include "userenv.h"
#include "Logger.h"
#include "constants.h"

UserSessionInfo ImpersonateUser::IsInteractive(DWORD sessionId)
{	
	SECURITY_LOGON_TYPE LogonType;
	ULONG rcb;
	TOKEN_STATISTICS ts;

	HANDLE hToken = nullptr;
	if (WTSQueryUserToken(sessionId, &hToken) == 0)
	{		
		Logger::GetInstance()->Log(L"WTSQueryUserToken failed, LastError:" + std::to_wstring(GetLastError()));

		return UserSessionInfo::INTERACTIVE;
	}

	HRESULT hr = GetTokenInformation(hToken, ::TokenStatistics, &ts, sizeof(ts), &rcb) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
	Logger::GetInstance()->Log(L"GetTokenInformation Hr = " + std::to_wstring(hr));
	CloseHandle(hToken);

	if (hr == S_OK)
	{
		PSECURITY_LOGON_SESSION_DATA LogonSessionData;

		hr = HRESULT_FROM_NT(LsaGetLogonSessionData(&ts.AuthenticationId, &LogonSessionData));
		if (0 <= hr)
		{
			LogonType = static_cast<SECURITY_LOGON_TYPE>(LogonSessionData->LogonType);
			LsaFreeReturnBuffer(LogonSessionData);

			if (LogonType == RemoteInteractive)
			{
				Logger::GetInstance()->Log("RDP logon session detected");
				return UserSessionInfo::RDP;
			}
			if (LogonType == Interactive)
			{
				Logger::GetInstance()->Log("interactive logon session detected");				
				return UserSessionInfo::INTERACTIVE;
			}
			else
			{
				Logger::GetInstance()->Log("other_error - no logon session could be deciphered");				
				return UserSessionInfo::OTHER_ERROR;
			}
		}
		Logger::GetInstance()->Log("LsaGetLogonSessionData failed");
	}
	Logger::GetInstance()->Log("GetTokenInformation failed ! returning interactive login session");
	return  UserSessionInfo::INTERACTIVE;
}

bool ImpersonateUser::CreateNotepadAsUser(DWORD sessionId)
{		
	if (sessionId > 0)
	{
		HANDLE hToken = nullptr;
		if (WTSQueryUserToken(sessionId, &hToken) != 0)
		{
			Logger::GetInstance()->Log(L"WTSQueryUserToken succeeded");
			HANDLE phNewToken = nullptr;
			// This allows a service that is impersonating a client to create a process that has the security context of the client.
			// https://docs.microsoft.com/en-us/windows/win32/api/securitybaseapi/nf-securitybaseapi-duplicatetokenex
			if (DuplicateTokenEx(hToken, same_as_current_user, nullptr, SECURITY_IMPERSONATION_LEVEL::SecurityImpersonation, TOKEN_TYPE::TokenPrimary, &phNewToken) != 0)
			{
				Logger::GetInstance()->Log(L"DuplicateTokenEx succeeded");
				LPVOID pEnvBlock = nullptr;
				if (CreateEnvironmentBlock(&pEnvBlock, phNewToken, false) == TRUE)
				{
					Logger::GetInstance()->Log(L"CreateEnvironmentBlock succeeded");
					STARTUPINFO         si;
					PROCESS_INFORMATION pi;

					memset(&si, 0, sizeof(si));
					memset(&pi, 0, sizeof(pi));

					si.cb = sizeof(si);
					wchar_t notepad[] = L"notepad";
					if (CreateProcessAsUser(phNewToken, nullptr, notepad, nullptr, nullptr, FALSE, NORMAL_PRIORITY_CLASS | CREATE_UNICODE_ENVIRONMENT |CREATE_NEW_CONSOLE,
						pEnvBlock, nullptr, &si, &pi) != TRUE)					
					{						
						Logger::GetInstance()->Log(L"CreateProcessWithTokenW failed, LastError:" + std::to_wstring(GetLastError()));
						CloseHandle(phNewToken);
						return false;
					}
					Logger::GetInstance()->Log("CreateProcessWithTokenW succeeded");					
				}
				else
				{
					Logger::GetInstance()->Log("CreateEnvironmentBlock failed, LastError:");
					Logger::GetInstance()->Log(std::to_wstring(GetLastError()));
					CloseHandle(phNewToken);
					return false;
				}

				// free resources
				if (pEnvBlock != nullptr)
				{
					DestroyEnvironmentBlock(pEnvBlock);
				}

				CloseHandle(phNewToken);
				return true;
			}
			Logger::GetInstance()->Log("DuplicateTokenEx failed, LastError:");
			Logger::GetInstance()->Log(std::to_wstring(GetLastError()));
			return false;
		}
		Logger::GetInstance()->Log("WTSQueryUserToken failed, LastError:");
		Logger::GetInstance()->Log(std::to_wstring(GetLastError()));
		return false;
	}
	Logger::GetInstance()->Log("Cannot impersonate user and create prcess, SessionId = " + sessionId);
	return false;
}

