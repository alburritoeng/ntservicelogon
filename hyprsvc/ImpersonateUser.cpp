#include "ImpersonateUser.h"
#include <wtsapi32.h>
#include "userenv.h"
#include "constants.h"
#include "Logger.h"

bool ImpersonateUser::CreateNotepadAsUser(DWORD sessionId)
{		
	if (sessionId > 0)
	{
		//Logger::GetInstance()->Log("SessionId = " + sessionId);

		HANDLE hToken = nullptr;
		if (WTSQueryUserToken(sessionId, &hToken) != 0)
		{
			Logger::GetInstance()->Log(L"WTSQueryUserToken succeeded");
			HANDLE phNewToken = nullptr;
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

