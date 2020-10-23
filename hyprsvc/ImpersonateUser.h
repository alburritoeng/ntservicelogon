#pragma once
#include <string>
#include <Windows.h>

class ImpersonateUser
{
public:
		
	static bool CreateNotepadAsUser(DWORD sessionId);
};

