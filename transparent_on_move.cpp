﻿//---------------------------------------------------------------------------//
//
// Main.cpp
//
//---------------------------------------------------------------------------//

#include <shlwapi.h>
#include <psapi.h>

#include "Plugin.hpp"
#include "MessageDef.hpp"
#include "Utility.hpp"

#include "config.hpp"

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "psapi.lib")

//---------------------------------------------------------------------------//
//
// グローバル変数
//
//---------------------------------------------------------------------------//

#pragma data_seg(".SHARED_DATA")
HHOOK  g_hHook  { nullptr };
#pragma data_seg()

HINSTANCE    g_hInst  { nullptr };
HANDLE       g_hShared{ nullptr };
// CONFIG_DATA  g_Config { NULL };

// 除外するウィンドウのパスの配列
TCHAR exclude_path[EXCLUDE_MAX][MAX_PATH];
// 除外するウィンドウのパスの個数
int	number_of_exclude;
// 透明度
int	alpha;


// プラグインの名前
#if defined(WIN64) || defined(_WIN64)
LPTSTR PLUGIN_NAME  { TEXT("Transparent On Move for Win10 x64") };
#else
LPTSTR PLUGIN_NAME  { TEXT("Transparent On Move for Win10 x86") };
#endif

// コマンドの数
DWORD COMMAND_COUNT { 0 };

// プラグインの情報
PLUGIN_INFO g_info = {
    0,                   // プラグインI/F要求バージョン
    PLUGIN_NAME,         // プラグインの名前（任意の文字が使用可能）
    nullptr,             // プラグインのファイル名（相対パス）
    ptAlwaysLoad,        // プラグインのタイプ
    0,                   // バージョン
    0,                   // バージョン
    COMMAND_COUNT,       // コマンド個数
    NULL,                // コマンド
    0,                   // ロードにかかった時間（msec）
};


LONG_PTR SetWindowLongPtrX(HWND hWnd, int nIndex, LONG_PTR dwNewLong) {
    if (::IsWindowUnicode(hWnd))
        return ::SetWindowLongPtrW(hWnd, nIndex, dwNewLong);
    else
        return ::SetWindowLongPtrA(hWnd, nIndex, dwNewLong);
}

LONG_PTR GetWindowLongPtrX(HWND hWnd, int nIndex) {
    if (::IsWindowUnicode(hWnd))
        return ::GetWindowLongPtrW(hWnd, nIndex);
    else
        return ::GetWindowLongPtrA(hWnd, nIndex);
}

LRESULT CallWindowProcX(WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (::IsWindowUnicode(hWnd))
        return ::CallWindowProcW(lpPrevWndFunc, hWnd, Msg, wParam, lParam);
    else
        return ::CallWindowProcA(lpPrevWndFunc, hWnd, Msg, wParam, lParam);
}

void SetWinLayer(HWND hwnd, BOOL Layer) {
    LONG_PTR exstyle = GetWindowLongPtrX(hwnd, GWL_EXSTYLE);

    //透明化
    if (Layer && !(exstyle & WS_EX_LAYERED)) {
        SetWindowLongPtrX(hwnd, GWL_EXSTYLE, exstyle | WS_EX_LAYERED);        
    }
    //透明化解除
    else if (!Layer && (exstyle & WS_EX_LAYERED)) {
        SetWindowLongPtrX(hwnd, GWL_EXSTYLE, exstyle & ~WS_EX_LAYERED);
    }
}

// --------------------------------------------------------
//    サブクラス化用のプロシージャ
// --------------------------------------------------------

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static BOOL State = FALSE;
    WNDPROC OldProc = (WNDPROC)::GetProp(hWnd, PROP_OLDPROC);

    switch (message) {
    case WM_MOVING:
        if (!State) {
            State = TRUE;
            SetWinLayer(hWnd, State);
            ::SetLayeredWindowAttributes(hWnd, 0, alpha, LWA_ALPHA);
        }
        break;

    case WM_EXITSIZEMOVE:
        State = FALSE;
        SetWinLayer(hWnd, State);
        SetWindowLongPtrX(hWnd, GWLP_WNDPROC, (LONG_PTR)OldProc);
        ::RemoveProp(hWnd, PROP_OLDPROC);

        break;
    }

    return CallWindowProcX(OldProc, hWnd, message, wParam, lParam);
}

BOOL IsTargetWindow(HWND hWnd) {
    HWND hParentWnd;
    LONG_PTR style = GetWindowLongPtrX(hWnd, GWL_STYLE);
    LONG_PTR exstyle = GetWindowLongPtrX(hWnd, GWL_EXSTYLE);
    if ((hParentWnd = ::GetAncestor(hWnd, GA_PARENT)) == ::GetDesktopWindow()
        && !(exstyle & WS_EX_TOOLWINDOW)
        && !(style & WS_POPUP)) {
        return TRUE;
    }
    else {
        TCHAR szClass[128];
        GetClassName(hParentWnd, szClass, 128);
        if (!lstrcmp(szClass, TEXT("MDIClient"))) {
            return TRUE;
        }
        return FALSE;
    }
}

// --------------------------------------------------------
//    フックプロシージャ
// --------------------------------------------------------
LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        LPCWPSTRUCT pcw = (LPCWPSTRUCT)lParam;

        if (pcw->message == WM_ENTERSIZEMOVE && ::GetKeyState(VK_LBUTTON) < 0) {
            if (IsTargetWindow(pcw->hwnd)) {
                TCHAR szBuff[MAX_PATH];
                ::GetModuleFileName(NULL, szBuff, MAX_PATH);
                for (int i = 0; i < number_of_exclude; i++) {
                    if (lstrcmpi(szBuff, exclude_path[i]) == 0) {
                        return ::CallNextHookEx(g_hHook, nCode, wParam, lParam);
                    }
                }
                if (GetProp(pcw->hwnd, PROP_OLDPROC) == NULL) {
                    WNDPROC OldProc = (WNDPROC)SetWindowLongPtrX(pcw->hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
                    SetProp(pcw->hwnd, PROP_OLDPROC, (HANDLE)OldProc);
                }
            }
        }
    }
    return ::CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

// TTBEvent_Init() の内部実装
BOOL WINAPI Init(void) {
    // config::get_instance().load_config();

    TCHAR inipath[MAX_PATH];
    size_t len = ::GetModuleFileName(g_hInst, inipath, MAX_PATH);

    if (len < 4) {
        return FALSE;
    }
    else {
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

    g_hHook = ::SetWindowsHookEx(WH_CALLWNDPROC, CallWndProc, g_hInst, 0);
    if (!g_hHook) {
        DWORD ret = GetLastError();
        WriteLog(elError, TEXT("%s: フックに失敗しました"), g_info.Name);
        return FALSE;
    }
    // ::PostMessageA(HWND_BROADCAST, WM_NULL, 0, 0);
    WriteLog(elInfo, TEXT("%s: successfully initialized"), g_info.Name);

    return TRUE;
}

//---------------------------------------------------------------------------//

// TTBEvent_Unload() の内部実装
void WINAPI Unload(void) {
    ::UnhookWindowsHookEx(g_hHook);
    g_hHook = NULL;
    // ::PostMessageA(HWND_BROADCAST, WM_NULL, 0, 0);
    WriteLog(elInfo, TEXT("%s: successfully uninitialized"), g_info.Name);
}

//---------------------------------------------------------------------------//

// TTBEvent_Execute() の内部実装
BOOL WINAPI Execute(INT32 CmdId, HWND hWnd) {
    return TRUE;
}

//---------------------------------------------------------------------------//

// TTBEvent_WindowsHook() の内部実装
void WINAPI Hook(UINT Msg, WPARAM wParam, LPARAM lParam) {
}

//---------------------------------------------------------------------------//
//
// CRT を使わないため new/delete を自前で実装
//
//---------------------------------------------------------------------------//

#if defined(_NODEFLIB)

void* __cdecl operator new(size_t size)
{
    return ::HeapAlloc(::GetProcessHeap(), 0, size);
}

void __cdecl operator delete(void* p)
{
    if ( p != nullptr ) ::HeapFree(::GetProcessHeap(), 0, p);
}

void __cdecl operator delete(void* p, size_t) // C++14
{
    if ( p != nullptr ) ::HeapFree(::GetProcessHeap(), 0, p);
}

void* __cdecl operator new[](size_t size)
{
    return ::HeapAlloc(::GetProcessHeap(), 0, size);
}

void __cdecl operator delete[](void* p)
{
    if ( p != nullptr ) ::HeapFree(::GetProcessHeap(), 0, p);
}

void __cdecl operator delete[](void* p, size_t) // C++14
{
    if ( p != nullptr ) ::HeapFree(::GetProcessHeap(), 0, p);
}

// プログラムサイズを小さくするためにCRTを除外
#pragma comment(linker, "/nodefaultlib:libcmt.lib")
#pragma comment(linker, "/entry:DllMain")

#endif

//---------------------------------------------------------------------------//

// DLL エントリポイント
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        g_hInst = hInstance;
    }
    return TRUE;
}
