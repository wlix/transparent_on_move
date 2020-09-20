#pragma once
#if 0
#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <string>

#define EXCLUDE_MAX     10
#define PROP_OLDPROC    "TransparentOnMoveProcedure"
#define FILEMAP_SHARED  L"TRANSPARENT_ON_MOVE"

typedef struct _SHARED_DATA {
    // 除外するウィンドウのパスの配列
    wchar_t exclude_path[EXCLUDE_MAX][MAX_PATH];
    // 除外するウィンドウのパスの個数
    int	number_of_exclude;
    // 透明度
    int	alpha;
    // iniのパス
    wchar_t x86_inipath[MAX_PATH];
    wchar_t x64_inipath[MAX_PATH];
} SHARED_DATA;

