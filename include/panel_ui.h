#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "ui_strings.h"

#define IDC_EDIT_X 1001
#define IDC_EDIT_Y 1002
#define IDC_ADD_POINT 1003
#define IDC_DELETE_POINT 1004
#define IDC_LIST_POINTS 1005
#define IDC_STATIC_RANGE 1006
#define IDC_OPEN_IMAGE 1007
#define IDC_SAVE_PATH 1008
#define IDC_STATIC_SAVE_PATH 1009

typedef struct {
    HWND edit_x;
    HWND edit_y;
    HWND add_btn;
    HWND delete_btn;
    HWND open_btn;
    HWND save_path_btn;
    HWND listbox;
    HWND range_label;
    HWND save_path_label;
    HWND usage_label;
    HFONT usage_font;
} PanelControls;

int panel_usage_height(const char *usage_text);
int panel_window_height(const char *usage_text);
void panel_create_controls(PanelControls *controls, HWND hwnd, HINSTANCE instance, const UiText *text);
void panel_layout_controls(const PanelControls *controls, const UiText *text);
void panel_destroy_controls(PanelControls *controls);
#endif
