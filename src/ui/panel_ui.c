#include "panel_ui.h"

#ifdef _WIN32

#include <string.h>

int panel_usage_height(const char *usage_text) {
    int lines = 1;
    const char *text = usage_text;
    while (*text != '\0') {
        if (*text == '\n') {
            lines += 1;
        }
        text += 1;
    }
    return 20 + lines * 22;
}

int panel_window_height(const char *usage_text) {
    return 230 + panel_usage_height(usage_text);
}

void panel_create_controls(PanelControls *controls, HWND hwnd, HINSTANCE instance, const UiText *text) {
    memset(controls, 0, sizeof(*controls));
    CreateWindowExA(0, "STATIC", text->label_x, WS_CHILD | WS_VISIBLE, 10, 10, 18, 20, hwnd, 0, instance, 0);
    controls->edit_x = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 30, 8, 90, 24, hwnd, (HMENU) IDC_EDIT_X, instance, 0);
    CreateWindowExA(0, "STATIC", text->label_y, WS_CHILD | WS_VISIBLE, 130, 10, 18, 20, hwnd, 0, instance, 0);
    controls->edit_y = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 150, 8, 90, 24, hwnd, (HMENU) IDC_EDIT_Y, instance, 0);
    controls->add_btn = CreateWindowExA(0, "BUTTON", text->button_add, WS_CHILD | WS_VISIBLE, 250, 8, 50, 24, hwnd, (HMENU) IDC_ADD_POINT, instance, 0);
    controls->delete_btn = CreateWindowExA(0, "BUTTON", text->button_delete, WS_CHILD | WS_VISIBLE, 305, 8, 60, 24, hwnd, (HMENU) IDC_DELETE_POINT, instance, 0);
    controls->open_btn = CreateWindowExA(0, "BUTTON", text->button_open, WS_CHILD | WS_VISIBLE, 370, 8, 55, 24, hwnd, (HMENU) IDC_OPEN_IMAGE, instance, 0);
    controls->save_path_btn = CreateWindowExA(0, "BUTTON", text->button_save_path, WS_CHILD | WS_VISIBLE, 430, 8, 80, 24, hwnd, (HMENU) IDC_SAVE_PATH, instance, 0);
    controls->listbox = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "", WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL, 10, 40, 500, 90, hwnd, (HMENU) IDC_LIST_POINTS, instance, 0);
    controls->range_label = CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 10, 136, 500, 18, hwnd, (HMENU) IDC_STATIC_RANGE, instance, 0);
    controls->save_path_label = CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 10, 158, 500, 18, hwnd, (HMENU) IDC_STATIC_SAVE_PATH, instance, 0);
    controls->usage_label = CreateWindowExA(0, "STATIC", text->usage_hint, WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 182, 520, panel_usage_height(text->usage_hint), hwnd, 0, instance, 0);
    controls->usage_font = CreateFontA(-17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        FIXED_PITCH | FF_MODERN, "Consolas");
    if (controls->usage_font != 0) {
        SendMessageA(controls->usage_label, WM_SETFONT, (WPARAM) controls->usage_font, TRUE);
    }
}

void panel_layout_controls(const PanelControls *controls, const UiText *text) {
    const int usage_top = 182;
    const int usage_height = panel_usage_height(text->usage_hint);
    MoveWindow(controls->edit_x, 30, 8, 90, 24, TRUE);
    MoveWindow(controls->edit_y, 150, 8, 90, 24, TRUE);
    MoveWindow(controls->add_btn, 250, 8, 50, 24, TRUE);
    MoveWindow(controls->delete_btn, 305, 8, 60, 24, TRUE);
    MoveWindow(controls->open_btn, 370, 8, 55, 24, TRUE);
    MoveWindow(controls->save_path_btn, 430, 8, 80, 24, TRUE);
    MoveWindow(controls->listbox, 10, 40, 500, 90, TRUE);
    MoveWindow(controls->range_label, 10, 136, 500, 18, TRUE);
    MoveWindow(controls->save_path_label, 10, 158, 500, 18, TRUE);
    MoveWindow(controls->usage_label, 10, usage_top, 520, usage_height, TRUE);
}

void panel_destroy_controls(PanelControls *controls) {
    if (controls->usage_font != 0) {
        DeleteObject(controls->usage_font);
        controls->usage_font = 0;
    }
}

#endif
