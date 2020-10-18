#include <strsafe.h>

#include "config.hpp"
#include "Utility.hpp"

LPCTSTR kMutexName = TEXT("TransparentOnMoveMutex");

extern HINSTANCE    g_hInst;
extern HANDLE       g_hShared;
// extern CONFIG_DATA  g_Config;
// 除外するウィンドウのパスの配列
extern TCHAR exclude_path[EXCLUDE_MAX][MAX_PATH];
// 除外するウィンドウのパスの個数
extern int	number_of_exclude;
// 透明度
extern int	alpha;

BOOL WritePrivateProfileInt(LPCTSTR lpAppName, LPCTSTR lpKeyName, int value, LPCTSTR lpFileName) {
	TCHAR szbuff[32];
	wsprintf(szbuff, TEXT("%d"), value);
	return ::WritePrivateProfileString(lpAppName, lpKeyName, szbuff, lpFileName);
}

config::config() : m_hMutex(NULL) {
	m_hMutex = ::CreateMutex(nullptr, FALSE, kMutexName);
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

	TCHAR inipath[MAX_PATH];
	size_t len = ::GetModuleFileName(g_hInst, inipath, MAX_PATH);

	if (len < 4) {
		return;
	} else {
		inipath[len - 1] = TEXT('i');
		inipath[len - 2] = TEXT('n');
		inipath[len - 3] = TEXT('i');
	}

	alpha = ::GetPrivateProfileInt(TEXT("Setting"), TEXT("Alpha"), 128, inipath);
	number_of_exclude = 0;
	for (int i = 0; i < EXCLUDE_MAX; i++) {
		TCHAR szKey[32];
		wsprintf(szKey, TEXT("ExcludePath%d"), i);
		::GetPrivateProfileString(TEXT("Setting"), szKey, TEXT(""), exclude_path[number_of_exclude], MAX_PATH, inipath);
		if (lstrcmp(exclude_path[number_of_exclude], TEXT(""))) {
			number_of_exclude++;
		}
	}
}
