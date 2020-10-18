#pragma once

#include <SDKDDKVer.h>

#include <windows.h>
#include <string>

#define EXCLUDE_MAX     10
#define PROP_OLDPROC    TEXT("TransparentProcedure")

typedef struct _CONFIG_DATA {
    // 除外するウィンドウのパスの配列
    TCHAR exclude_path[EXCLUDE_MAX][MAX_PATH];
    // 除外するウィンドウのパスの個数
    int	number_of_exclude;
    // 透明度
    int	alpha;
} CONFIG_DATA;
