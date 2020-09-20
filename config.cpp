#include <strsafe.h>

#include "config.hpp"
#include "Utility.hpp"

LPCWSTR kMutexName = L"TransparentOnMoveMutex";

extern HINSTANCE    g_hInst;
extern HANDLE       g_hShared;
extern SHARED_DATA* g_Shared;

BOOL WritePrivateProfileIntW(LPCWSTR lpAppName, LPCWSTR lpKeyName, int value, LPCWSTR lpFileName) {
	wchar_t szbuff[32];
	wsprintfW(szbuff, L"%d", value);
	return ::WritePrivateProfileStringW(lpAppName, lpKeyName, szbuff, lpFileName);
}

config::config() : m_hMutex(NULL) {
	m_hMutex = ::CreateMutexW(nullptr, FALSE, kMutexName);
	_ASSERT(m_hMutex);
}

config::~config() {
	::CloseHandle(m_hMutex);
}

config& config::get_instance() {
	static config s_instance;
	return s_instance;
}

void config::load_config() {
	mutex_locker lock(m_hMutex);

	wchar_t inipath[MAX_PATH];
	size_t len = ::GetModuleFileNameW(g_hInst, inipath, MAX_PATH);
	WriteLog(elInfo, TEXT("length: %d"), len);

	if (len < 4) {
		return;
	} else {
		inipath[len - 1] = L'i';
		inipath[len - 2] = L'n';
		inipath[len - 3] = L'i';
	}

#if defined(UNICODE) || defined(_UNICODE) || defined(_WIN64)
	// g_Shared.x64_inipath = inipath;
	::StringCchCopyW(g_Shared->x64_inipath, MAX_PATH, inipath);
#else
	// g_Shared.x86_inipath = inipath;
	::StringCchCopyW(g_Shared->x86_inipath, MAX_PATH, inipath);
#endif

	g_Shared->alpha = ::GetPrivateProfileIntW(L"Setting", L"Alpha", 128, inipath);
	WriteLog(elInfo, TEXT("alpha: %d"), g_Shared->alpha);
	g_Shared->number_of_exclude = 0;
	for (int i = 0; i < EXCLUDE_MAX; i++) {
		wchar_t szKey[32];
		wsprintfW(szKey, L"ExcludePath%d", i);
		::GetPrivateProfileStringW(L"Setting", szKey, L"", g_Shared->exclude_path[g_Shared->number_of_exclude], MAX_PATH, inipath);
		if (lstrcmpW(g_Shared->exclude_path[g_Shared->number_of_exclude], L"")) {
			g_Shared->number_of_exclude++;
			WriteLog(elInfo, TEXT("inipath: %s"), g_Shared->exclude_path[g_Shared->number_of_exclude]);
		}
	}
}
