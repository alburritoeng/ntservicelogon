#pragma once
enum UserSessionInfo {
	UNKNOWN =0,
	INTERACTIVE,
	RDP,
	OTHER_ERROR
};

constexpr const wchar_t hypr_svc_name[] = L"hyprsvc";
constexpr const int same_as_current_user = 0;