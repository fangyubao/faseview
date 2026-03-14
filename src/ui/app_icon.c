#include "app_icon.h"

#ifdef _WIN32

#include <string.h>

static HICON g_small_icon = 0;
static HICON g_large_icon = 0;

static int build_icon_path(char *path, size_t path_size) {
    DWORD length;
    char *slash;
    if (path == 0 || path_size < MAX_PATH) {
        return 0;
    }
    length = GetModuleFileNameA(0, path, (DWORD) path_size);
    if (length == 0 || length >= path_size) {
        return 0;
    }
    slash = strrchr(path, '\\');
    if (slash == 0) {
        return 0;
    }
    slash[1] = '\0';
    if (strlen(path) + strlen("build\\resources\\app_icon.bmp") + 1 >= path_size) {
        return 0;
    }
    strcat(path, "build\\resources\\app_icon.bmp");
    return 1;
}

static HICON load_icon_from_bitmap(int icon_size) {
    char path[MAX_PATH];
    HBITMAP color_bitmap;
    HBITMAP mask_bitmap;
    ICONINFO icon_info;
    HICON icon;

    if (!build_icon_path(path, sizeof(path))) {
        return 0;
    }

    color_bitmap = (HBITMAP) LoadImageA(0, path, IMAGE_BITMAP, icon_size, icon_size, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    if (color_bitmap == 0) {
        return 0;
    }

    mask_bitmap = CreateBitmap(icon_size, icon_size, 1, 1, 0);
    if (mask_bitmap == 0) {
        DeleteObject(color_bitmap);
        return 0;
    }

    memset(&icon_info, 0, sizeof(icon_info));
    icon_info.fIcon = TRUE;
    icon_info.hbmColor = color_bitmap;
    icon_info.hbmMask = mask_bitmap;
    icon = CreateIconIndirect(&icon_info);

    DeleteObject(color_bitmap);
    DeleteObject(mask_bitmap);
    return icon;
}

int app_icon_initialize(void) {
    if (g_small_icon == 0) {
        g_small_icon = load_icon_from_bitmap(16);
    }
    if (g_large_icon == 0) {
        g_large_icon = load_icon_from_bitmap(32);
    }
    return g_small_icon != 0 || g_large_icon != 0;
}

void app_icon_apply(HWND hwnd) {
    if (hwnd == 0) {
        return;
    }
    if (g_small_icon != 0) {
        SendMessageA(hwnd, WM_SETICON, ICON_SMALL, (LPARAM) g_small_icon);
    }
    if (g_large_icon != 0) {
        SendMessageA(hwnd, WM_SETICON, ICON_BIG, (LPARAM) g_large_icon);
    }
}

void app_icon_shutdown(void) {
    if (g_small_icon != 0) {
        DestroyIcon(g_small_icon);
        g_small_icon = 0;
    }
    if (g_large_icon != 0) {
        DestroyIcon(g_large_icon);
        g_large_icon = 0;
    }
}

#else

int app_icon_initialize(void) {
    return 0;
}

void app_icon_shutdown(void) {
}

#endif
