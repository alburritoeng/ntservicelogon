#pragma once
#include <string>
#include <Windows.h>
#include "constants.h"

class ImpersonateUser
{
public:
	static UserSessionInfo IsInteractive(DWORD sessionId);
	static bool CreateNotepadAsUser(DWORD sessionId);
};

