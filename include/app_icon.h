#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int app_icon_initialize(void);
void app_icon_apply(HWND hwnd);
void app_icon_shutdown(void);
#else
int app_icon_initialize(void);
void app_icon_shutdown(void);
#endif
