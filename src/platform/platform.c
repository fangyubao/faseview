#include "platform.h"

#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#include <windows.h>
#include <shobjidl.h>
#endif

int platform_is_space_pressed(void) {
#ifdef _WIN32
    return (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
#else
    return 0;
#endif
}

void platform_get_screen_size(int *width, int *height) {
#ifdef _WIN32
    if (width != 0) {
        *width = GetSystemMetrics(SM_CXSCREEN);
    }
    if (height != 0) {
        *height = GetSystemMetrics(SM_CYSCREEN);
    }
#else
    if (width != 0) {
        *width = 1600;
    }
    if (height != 0) {
        *height = 900;
    }
#endif
}

void platform_raise_window(void *window_handle) {
#ifdef _WIN32
    HWND hwnd = (HWND) window_handle;
    HWND foreground;
    DWORD current_thread;
    DWORD foreground_thread;
    int attached = 0;
    if (hwnd == 0) {
        return;
    }
    foreground = GetForegroundWindow();
    current_thread = GetCurrentThreadId();
    foreground_thread = foreground != 0 ? GetWindowThreadProcessId(foreground, 0) : 0;
    if (foreground_thread != 0 && foreground_thread != current_thread) {
        attached = AttachThreadInput(current_thread, foreground_thread, TRUE);
    }
    if (IsIconic(hwnd)) {
        ShowWindow(hwnd, SW_RESTORE);
    } else {
        ShowWindow(hwnd, SW_SHOW);
    }
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    BringWindowToTop(hwnd);
    SetCapture(hwnd);
    ReleaseCapture();
    SetForegroundWindow(hwnd);
    SetActiveWindow(hwnd);
    SetFocus(hwnd);
    ShowWindow(hwnd, SW_SHOWNORMAL);
    if (attached) {
        AttachThreadInput(current_thread, foreground_thread, FALSE);
    }
#else
    (void) window_handle;
#endif
}

int platform_select_folder(void *owner_window, const char *initial_path, char *out_path, int out_path_size) {
#ifdef _WIN32
    IFileOpenDialog *dialog = 0;
    IShellItem *folder = 0;
    IShellItem *result = 0;
    PWSTR wide_result = 0;
    HRESULT hr;
    HRESULT init_hr;
    int ok = 0;

    if (out_path == 0 || out_path_size <= 1) {
        return 0;
    }
    out_path[0] = '\0';

    init_hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(init_hr) && init_hr != RPC_E_CHANGED_MODE) {
        return 0;
    }

    hr = CoCreateInstance(&CLSID_FileOpenDialog, 0, CLSCTX_INPROC_SERVER, &IID_IFileOpenDialog, (void **) &dialog);
    if (FAILED(hr) || dialog == 0) {
        goto cleanup;
    }

    hr = IFileOpenDialog_SetOptions(dialog, FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
    if (FAILED(hr)) {
        goto cleanup;
    }

    if (initial_path != 0 && initial_path[0] != '\0') {
        WCHAR wide_initial[MAX_PATH];
        if (MultiByteToWideChar(CP_UTF8, 0, initial_path, -1, wide_initial, MAX_PATH) > 0 &&
            SUCCEEDED(SHCreateItemFromParsingName(wide_initial, 0, &IID_IShellItem, (void **) &folder))) {
            IFileOpenDialog_SetFolder(dialog, folder);
        }
    }

    hr = IFileOpenDialog_Show(dialog, (HWND) owner_window);
    if (FAILED(hr)) {
        goto cleanup;
    }

    hr = IFileOpenDialog_GetResult(dialog, &result);
    if (FAILED(hr) || result == 0) {
        goto cleanup;
    }

    hr = IShellItem_GetDisplayName(result, SIGDN_FILESYSPATH, &wide_result);
    if (FAILED(hr) || wide_result == 0) {
        goto cleanup;
    }

    if (WideCharToMultiByte(CP_UTF8, 0, wide_result, -1, out_path, out_path_size, 0, 0) > 0) {
        ok = 1;
    }

cleanup:
    if (wide_result != 0) {
        CoTaskMemFree(wide_result);
    }
    if (result != 0) {
        IShellItem_Release(result);
    }
    if (folder != 0) {
        IShellItem_Release(folder);
    }
    if (dialog != 0) {
        IFileOpenDialog_Release(dialog);
    }
    if (SUCCEEDED(init_hr)) {
        CoUninitialize();
    }
    return ok;
#else
    (void) owner_window;
    if (out_path == 0 || out_path_size <= 1 || initial_path == 0) {
        return 0;
    }
    strncpy(out_path, initial_path, (size_t) out_path_size - 1);
    out_path[out_path_size - 1] = '\0';
    return 1;
#endif
}

void platform_set_window_title(const char *window_name, const char *title) {
    (void) window_name;
    (void) title;
}

void platform_init_point_panel(const char *window_name, AppState *state) {
    (void) window_name;
    (void) state;
}

void platform_sync_point_panel(const char *window_name, AppState *state) {
    (void) window_name;
    (void) state;
}

void platform_shutdown_point_panel(void) {
}

void platform_pump_messages(void) {
}

int platform_save_bgr_png(const char *path, const IplImage *image) {
    int y;
    unsigned char *rgb;
    const int width = image->width;
    const int height = image->height;
    const int stride = width * 3;

    rgb = (unsigned char *) malloc((size_t) height * (size_t) stride);
    if (rgb == 0) {
        return 0;
    }

    for (y = 0; y < height; ++y) {
        const unsigned char *src = (const unsigned char *) (image->imageData + y * image->widthStep);
        unsigned char *dst = rgb + (size_t) y * (size_t) stride;
        int x;
        for (x = 0; x < width; ++x) {
            dst[x * 3 + 0] = src[x * 3 + 2];
            dst[x * 3 + 1] = src[x * 3 + 1];
            dst[x * 3 + 2] = src[x * 3 + 0];
        }
    }

    y = stbi_write_png(path, width, height, 3, rgb, stride);
    free(rgb);
    return y != 0;
}
