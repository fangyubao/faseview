#include "measure_app.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <opencv2/imgproc/imgproc_c.h>

#include "image_analysis.h"
#include "image_loader.h"
#include "panel_ui.h"
#include "platform.h"
#include "ui_strings.h"
#include "app_icon.h"
#include "tinyfiledialogs.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#endif

#define APP_CLASS_NAME "SmartMeasureWin32Host"
#define PANEL_CLASS_NAME "SmartMeasureToolPanel"
#define ORTHOGONAL_ANGLE_THRESHOLD 89.2
#define SHIFT_LOCK_THRESHOLD_PX 6

typedef struct {
    AppState *state;
    HWND hwnd;
    HWND panel_hwnd;
    PanelControls panel;
    int client_width;
    int client_height;
    int panel_initialized;
} UiContext;

static UiContext g_ui = {0};

static CvPoint image_to_screen(const AppState *state, CvPoint point) {
    CvPoint out;
    out.x = (int) floor((double) point.x * state->zoom + state->pan_x + 0.5);
    out.y = (int) floor((double) point.y * state->zoom + state->pan_y + 0.5);
    return out;
}

static CvPoint get_effective_active_raw(AppState *state, int ctrl, int shift) {
    CvPoint active = get_raw_point(state, state->last_mouse);
    if (ctrl) {
        CvPoint snapped;
        if (find_nearest_corner(&state->image, active, &snapped, 40.0)) {
            active = snapped;
        }
    }
    if (shift && state->is_drawing && state->start_raw.x >= 0 && state->start_raw.y >= 0) {
        if (state->shift_lock_axis == 0) {
            const int dx = abs(state->last_mouse.x - state->drag_anchor_mouse.x);
            const int dy = abs(state->last_mouse.y - state->drag_anchor_mouse.y);
            if (dx >= SHIFT_LOCK_THRESHOLD_PX || dy >= SHIFT_LOCK_THRESHOLD_PX) {
                state->shift_lock_axis = (dx >= dy) ? 1 : 2;
            }
        }
        if (state->shift_lock_axis == 1) {
            active.y = state->start_raw.y;
        } else if (state->shift_lock_axis == 2) {
            active.x = state->start_raw.x;
        }
    } else {
        state->shift_lock_axis = 0;
    }
    return active;
}

static int point_in_image(const AppState *state, CvPoint p) {
    return p.x >= 0 && p.y >= 0 && p.x < state->image.image_bgr->width && p.y < state->image.image_bgr->height;
}

static int canvas_height(const AppState *state) {
    (void) state;
    return g_ui.client_height > 1 ? g_ui.client_height : 1;
}

static void draw_text_with_outline(IplImage *image, const char *text, CvPoint org, CvScalar color, double scale) {
    CvFont font;
    cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, scale, scale, 0.0, 1, CV_AA);
    cvPutText(image, text, cvPoint(org.x + 1, org.y + 1), &font, cvScalar(0, 0, 0, 0));
    cvPutText(image, text, org, &font, color);
}

static void draw_cross(IplImage *image, CvPoint anchor, CvScalar color) {
    cvLine(image, cvPoint(anchor.x - 4, anchor.y), cvPoint(anchor.x + 4, anchor.y), color, 1, CV_AA, 0);
    cvLine(image, cvPoint(anchor.x, anchor.y - 4), cvPoint(anchor.x, anchor.y + 4), color, 1, CV_AA, 0);
}

static void draw_measure_line(IplImage *image, CvPoint p1, CvPoint p2) {
    cvLine(image, p1, p2, cvScalar(0, 0, 0, 0), 4, CV_AA, 0);
    cvLine(image, p1, p2, cvScalar(0, 255, 255, 0), 2, CV_AA, 0);
}

static void draw_marker_points(const AppState *state, IplImage *display) {
    int i;
    char label[64];
    for (i = 0; i < state->marker_count; ++i) {
        CvPoint anchor;
        if (!state->markers[i].enabled) {
            continue;
        }
        anchor = image_to_screen(state, state->markers[i].pos);
        cvCircle(display, anchor, 5, cvScalar(255, 64, 64, 0), 2, CV_AA, 0);
        snprintf(label, sizeof(label), "(%d,%d)", state->markers[i].pos.x, state->markers[i].pos.y);
        draw_text_with_outline(display, label, cvPoint(anchor.x + 8, anchor.y - 8), cvScalar(255, 160, 160, 0), 0.45);
    }
}

static void draw_overlay(AppState *state, IplImage *display) {
    const int ctrl = (state->last_flags & MK_CONTROL) != 0;
    const int shift = (state->last_flags & MK_SHIFT) != 0;
    const CvPoint raw = get_raw_point(state, state->last_mouse);
    const CvPoint active_raw = get_effective_active_raw(state, ctrl, shift);
    char text[64];

    if (ctrl) {
        CvPoint snap_pt;
        if (find_nearest_corner(&state->image, raw, &snap_pt, 40.0)) {
            CvPoint center = image_to_screen(state, snap_pt);
            int radius = (int) (10.0 / state->zoom) + 2;
            if (radius < 6) {
                radius = 6;
            }
            cvCircle(display, center, radius + 2, cvScalar(0, 0, 0, 0), 3, CV_AA, 0);
            cvCircle(display, center, radius, cvScalar(255, 0, 255, 0), 2, CV_AA, 0);
            draw_cross(display, center, cvScalar(255, 255, 255, 0));
        }
    }

    if (state->is_drawing && state->start_raw.x >= 0 && state->start_raw.y >= 0) {
        EdgeHit *hits = 0;
        int hit_count = 0;
        int i;
        draw_measure_line(display, image_to_screen(state, state->start_raw), image_to_screen(state, active_raw));
        if (point_in_image(state, state->start_raw) && point_in_image(state, active_raw) &&
            collect_edge_hits(state, state->start_raw, active_raw, &hits, &hit_count)) {
            for (i = 0; i < hit_count; ++i) {
                CvPoint anchor = image_to_screen(state, hits[i].pos);
                CvScalar color = hits[i].angle > ORTHOGONAL_ANGLE_THRESHOLD ? cvScalar(0, 255, 0, 0) : cvScalar(180, 180, 180, 0);
                if (hits[i].angle > ORTHOGONAL_ANGLE_THRESHOLD) {
                    snprintf(text, sizeof(text), "90deg|%.1f", hits[i].distance);
                } else {
                    snprintf(text, sizeof(text), "%.1fdeg", hits[i].angle);
                }
                draw_cross(display, anchor, color);
                draw_text_with_outline(display, text, cvPoint(anchor.x + 8, anchor.y - 8), color, 0.4);
            }
        }
        free(hits);
    }

    draw_marker_points(state, display);

    snprintf(text, sizeof(text), "Pos: (%d, %d)", raw.x, raw.y);
    draw_text_with_outline(display, text, cvPoint(state->last_mouse.x + 15, state->last_mouse.y + 20), cvScalar(255, 255, 255, 0), 0.5);
    snprintf(text, sizeof(text), "Zoom: %.2fx", state->zoom);
    draw_text_with_outline(display, text, cvPoint(state->last_mouse.x + 15, state->last_mouse.y + 40), cvScalar(255, 255, 255, 0), 0.5);
    if (state->is_drawing && state->start_raw.x >= 0 && state->start_raw.y >= 0) {
        const double dx = (double) active_raw.x - (double) state->start_raw.x;
        const double dy = (double) active_raw.y - (double) state->start_raw.y;
        snprintf(text, sizeof(text), "Dist: %.1fpx", sqrt(dx * dx + dy * dy));
        draw_text_with_outline(display, text, cvPoint(state->last_mouse.x + 15, state->last_mouse.y + 60), cvScalar(255, 255, 255, 0), 0.5);
    }
}

static IplImage *render_view(AppState *state) {
    IplImage *display = cvCreateImage(cvSize(g_ui.client_width > 1 ? g_ui.client_width : 1, canvas_height(state)), IPL_DEPTH_8U, 3);
    float affine_data[6];
    CvMat affine = cvMat(2, 3, CV_32FC1, affine_data);

    affine_data[0] = (float) state->zoom;
    affine_data[1] = 0.0f;
    affine_data[2] = (float) state->pan_x;
    affine_data[3] = 0.0f;
    affine_data[4] = (float) state->zoom;
    affine_data[5] = (float) state->pan_y;

    cvSet(display, cvScalarAll(24), 0);
    cvWarpAffine(state->image.image_bgr, display, &affine, CV_INTER_LINEAR + CV_WARP_FILL_OUTLIERS, cvScalarAll(24));
    draw_overlay(state, display);
    return display;
}

static void save_screenshot(AppState *state) {
    char path[1152];
    IplImage *display = render_view(state);
    int ok;
    state->screenshot_index += 1;
    snprintf(path, sizeof(path), "%s\\smartmeasure_capture_%03d.png", state->screenshot_dir, state->screenshot_index);
    ok = platform_save_bgr_png(path, display);
    cvReleaseImage(&display);
    if (!ok) {
        tinyfd_messageBox(ui_text()->app_name, ui_text()->screenshot_save_failed, "ok", "error", 1);
        return;
    }
    tinyfd_messageBox(ui_text()->app_name, path, "ok", "info", 1);
}

static void sync_marker_list(AppState *state) {
    int i;
    char text[1152];
    SendMessageA(g_ui.panel.listbox, LB_RESETCONTENT, 0, 0);
    for (i = 0; i < state->marker_count; ++i) {
        snprintf(text, sizeof(text), "%d: (%d, %d)", i + 1, state->markers[i].pos.x, state->markers[i].pos.y);
        SendMessageA(g_ui.panel.listbox, LB_ADDSTRING, 0, (LPARAM) text);
    }
    snprintf(text, sizeof(text), ui_text()->range_format,
        state->image.image_bgr->width - 1, state->image.image_bgr->height - 1);
    SetWindowTextA(g_ui.panel.range_label, text);
    snprintf(text, sizeof(text), ui_text()->save_path_prefix, state->screenshot_dir);
    SetWindowTextA(g_ui.panel.save_path_label, text);
}

static void center_panel_over_main(void) {
    RECT main_rect;
    RECT panel_rect;
    int x;
    int y;

    if (g_ui.hwnd == 0 || g_ui.panel_hwnd == 0) {
        return;
    }
    GetWindowRect(g_ui.hwnd, &main_rect);
    GetWindowRect(g_ui.panel_hwnd, &panel_rect);
    x = main_rect.left + ((main_rect.right - main_rect.left) - (panel_rect.right - panel_rect.left)) / 2;
    y = main_rect.top + ((main_rect.bottom - main_rect.top) - (panel_rect.bottom - panel_rect.top)) / 2;
    SetWindowPos(g_ui.panel_hwnd, HWND_TOP, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
}

static void reset_state_for_new_image(AppState *state) {
    state->zoom = 1.0;
    state->pan_x = 0.0;
    state->pan_y = 0.0;
    state->pan_anchor_x = 0.0;
    state->pan_anchor_y = 0.0;
    state->start_raw = cvPoint(-1, -1);
    state->drag_anchor_mouse = cvPoint(0, 0);
    state->last_mouse = cvPoint(0, 0);
    state->last_flags = 0;
    state->is_drawing = 0;
    state->is_panning = 0;
    state->marker_count = 0;
}

static void update_title(AppState *state) {
    char title[256];
    snprintf(title, sizeof(title), ui_text()->main_window_title_format,
        state->image.image_bgr->width, state->image.image_bgr->height);
    SetWindowTextA(g_ui.hwnd, title);
}

static void request_repaint(void) {
    InvalidateRect(g_ui.hwnd, NULL, FALSE);
}

static void handle_open_image(AppState *state) {
    int filter_count = 0;
    const char *const *filters = ui_image_filters(&filter_count);
    const char *path = tinyfd_openFileDialog(ui_text()->open_image_title, "", filter_count, filters, ui_text()->image_filter_description, 0);
    ImageData new_data;
    if (path == 0 || path[0] == '\0') {
        return;
    }
    if (!load_image_file(path, &new_data)) {
        tinyfd_messageBox(ui_text()->app_name, ui_text()->image_load_failed, "ok", "error", 1);
        return;
    }
    release_image_data(&state->image);
    state->image = new_data;
    reset_state_for_new_image(state);
    update_title(state);
    sync_marker_list(state);
    platform_raise_window(g_ui.hwnd);
    if (g_ui.panel_hwnd != 0) {
        platform_raise_window(g_ui.panel_hwnd);
    }
    request_repaint();
}

static void handle_add_marker(AppState *state) {
    char x_text[64];
    char y_text[64];
    int x;
    int y;
    if (state->marker_count >= 10) {
        tinyfd_messageBox(ui_text()->app_name, ui_text()->marker_limit_reached, "ok", "warning", 1);
        return;
    }
    GetWindowTextA(g_ui.panel.edit_x, x_text, (int) sizeof(x_text));
    GetWindowTextA(g_ui.panel.edit_y, y_text, (int) sizeof(y_text));
    if (sscanf(x_text, "%d", &x) != 1 || sscanf(y_text, "%d", &y) != 1) {
        tinyfd_messageBox(ui_text()->app_name, ui_text()->invalid_coordinate, "ok", "warning", 1);
        return;
    }
    if (x < 0 || y < 0 || x >= state->image.image_bgr->width || y >= state->image.image_bgr->height) {
        tinyfd_messageBox(ui_text()->app_name, ui_text()->coordinate_out_of_range, "ok", "warning", 1);
        return;
    }
    state->markers[state->marker_count].pos = cvPoint(x, y);
    state->markers[state->marker_count].enabled = 1;
    state->marker_count += 1;
    SetWindowTextA(g_ui.panel.edit_x, "");
    SetWindowTextA(g_ui.panel.edit_y, "");
    sync_marker_list(state);
    request_repaint();
}

static void handle_delete_marker(AppState *state) {
    int index = (int) SendMessageA(g_ui.panel.listbox, LB_GETCURSEL, 0, 0);
    int i;
    if (index == LB_ERR || index < 0 || index >= state->marker_count) {
        return;
    }
    for (i = index; i + 1 < state->marker_count; ++i) {
        state->markers[i] = state->markers[i + 1];
    }
    state->marker_count -= 1;
    sync_marker_list(state);
    request_repaint();
}

static void handle_save_path(AppState *state) {
    char path[1024];
    if (!platform_select_folder(g_ui.panel_hwnd, state->screenshot_dir, path, (int) sizeof(path))) {
        return;
    }
    strncpy(state->screenshot_dir, path, sizeof(state->screenshot_dir) - 1);
    state->screenshot_dir[sizeof(state->screenshot_dir) - 1] = '\0';
    sync_marker_list(state);
    platform_raise_window(g_ui.hwnd);
    if (g_ui.panel_hwnd != 0) {
        platform_raise_window(g_ui.panel_hwnd);
    }
}

static void resize_outer_window_by_key(int key) {
    RECT rect;
    GetWindowRect(g_ui.hwnd, &rect);
    if (key == VK_UP) {
        rect.right = rect.left + (int) ((rect.right - rect.left) * 1.1);
        rect.bottom = rect.top + (int) ((rect.bottom - rect.top) * 1.1);
    } else if (key == VK_DOWN) {
        rect.right = rect.left + (int) ((rect.right - rect.left) * 0.9);
        rect.bottom = rect.top + (int) ((rect.bottom - rect.top) * 0.9);
    } else if (key == VK_LEFT) {
        rect.right = rect.left + (int) ((rect.right - rect.left) * 0.9);
    } else if (key == VK_RIGHT) {
        rect.right = rect.left + (int) ((rect.right - rect.left) * 1.1);
    } else {
        return;
    }
    SetWindowPos(g_ui.hwnd, 0, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER);
}

static void handle_mouse_event(AppState *state, UINT msg, WPARAM wparam, LPARAM lparam) {
    const int x = GET_X_LPARAM(lparam);
    const int y = GET_Y_LPARAM(lparam);
    const int ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    const int shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    const int space = platform_is_space_pressed();
    if (y >= canvas_height(state)) {
        return;
    }
    state->last_mouse = cvPoint(x, y);
    state->last_flags = (shift ? MK_SHIFT : 0) | (ctrl ? MK_CONTROL : 0) | (int) (wparam & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON));

    if (msg == WM_MOUSEWHEEL && ctrl) {
        const int delta = GET_WHEEL_DELTA_WPARAM(wparam);
        const CvPoint raw_before = get_raw_point(state, cvPoint(x, y));
        if (delta > 0) state->zoom *= 1.1;
        else if (delta < 0) state->zoom *= 0.9;
        if (state->zoom < 0.1) state->zoom = 0.1;
        if (state->zoom > 50.0) state->zoom = 50.0;
        state->pan_x = (double) x - (double) raw_before.x * state->zoom;
        state->pan_y = (double) y - (double) raw_before.y * state->zoom;
        state->shift_lock_axis = 0;
        request_repaint();
        return;
    }

    if (msg == WM_MBUTTONDOWN) {
        SetCapture(g_ui.hwnd);
        if (space) {
            state->zoom = 1.0;
            state->pan_x = 0.0;
            state->pan_y = 0.0;
        } else {
            state->is_panning = 1;
            state->pan_anchor_mouse = cvPoint(x, y);
            state->pan_anchor_x = state->pan_x;
            state->pan_anchor_y = state->pan_y;
        }
        request_repaint();
        return;
    }

    if (msg == WM_MBUTTONUP) {
        state->is_panning = 0;
        state->shift_lock_axis = 0;
        ReleaseCapture();
        request_repaint();
        return;
    }

    if (msg == WM_RBUTTONDOWN) {
        state->is_drawing = 0;
        state->start_raw = cvPoint(-1, -1);
        state->drag_anchor_mouse = cvPoint(0, 0);
        state->shift_lock_axis = 0;
        request_repaint();
        return;
    }

    if (msg == WM_MOUSEMOVE && state->is_panning) {
        state->pan_x = state->pan_anchor_x + (double) (x - state->pan_anchor_mouse.x);
        state->pan_y = state->pan_anchor_y + (double) (y - state->pan_anchor_mouse.y);
        request_repaint();
        return;
    }

    if (msg == WM_LBUTTONDOWN) {
        CvPoint start_raw;
        SetCapture(g_ui.hwnd);
        state->shift_lock_axis = 0;
        state->drag_anchor_mouse = cvPoint(x, y);
        start_raw = get_raw_point(state, cvPoint(x, y));
        if (ctrl) {
            CvPoint snapped;
            if (find_nearest_corner(&state->image, start_raw, &snapped, 40.0)) {
                start_raw = snapped;
            }
        }
        state->start_raw = start_raw;
        state->is_drawing = 1;
        request_repaint();
        return;
    }

    if (msg == WM_LBUTTONUP) {
        state->is_drawing = 0;
        state->start_raw = cvPoint(-1, -1);
        state->drag_anchor_mouse = cvPoint(0, 0);
        state->shift_lock_axis = 0;
        ReleaseCapture();
        request_repaint();
        return;
    }

    if (msg == WM_MOUSEMOVE) {
        request_repaint();
    }
}

static LRESULT CALLBACK panel_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    AppState *state = g_ui.state;
    switch (msg) {
        case WM_CREATE:
            panel_create_controls(&g_ui.panel, hwnd, GetModuleHandleA(0), ui_text());
            panel_layout_controls(&g_ui.panel, ui_text());
            return 0;
        case WM_COMMAND:
            if (state != NULL) {
                switch (LOWORD(wparam)) {
                    case IDC_ADD_POINT:
                        handle_add_marker(state);
                        return 0;
                    case IDC_DELETE_POINT:
                        handle_delete_marker(state);
                        return 0;
                    case IDC_OPEN_IMAGE:
                        handle_open_image(state);
                        return 0;
                    case IDC_SAVE_PATH:
                        handle_save_path(state);
                        return 0;
                    default:
                        break;
                }
            }
            break;
        case WM_CLOSE:
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        case WM_DESTROY:
            panel_destroy_controls(&g_ui.panel);
            return 0;
        default:
            break;
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static LRESULT CALLBACK main_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    AppState *state = g_ui.state;
    switch (msg) {
        case WM_CREATE:
            g_ui.hwnd = hwnd;
            return 0;
        case WM_SIZE:
            g_ui.client_width = LOWORD(lparam);
            g_ui.client_height = HIWORD(lparam);
            request_repaint();
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT:
            if (state != NULL) {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                IplImage *view = render_view(state);
                BITMAPINFO bmi;
                memset(&bmi, 0, sizeof(bmi));
                bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                bmi.bmiHeader.biWidth = view->width;
                bmi.bmiHeader.biHeight = -view->height;
                bmi.bmiHeader.biPlanes = 1;
                bmi.bmiHeader.biBitCount = 24;
                bmi.bmiHeader.biCompression = BI_RGB;
                StretchDIBits(hdc, 0, 0, view->width, view->height, 0, 0, view->width, view->height,
                    view->imageData, &bmi, DIB_RGB_COLORS, SRCCOPY);
                cvReleaseImage(&view);
                EndPaint(hwnd, &ps);
            }
            return 0;
        case WM_MOUSEWHEEL: {
            POINT pt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
            ScreenToClient(hwnd, &pt);
            handle_mouse_event(state, msg, wparam, MAKELPARAM(pt.x, pt.y));
            return 0;
        }
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            handle_mouse_event(state, msg, wparam, lparam);
            return 0;
        case WM_KEYDOWN:
            if (wparam == VK_ESCAPE) {
                PostQuitMessage(0);
                return 0;
            }
            if (wparam == 'S') {
                save_screenshot(state);
                request_repaint();
                return 0;
            }
            resize_outer_window_by_key((int) wparam);
            return 0;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            if (g_ui.panel_hwnd != 0) {
                DestroyWindow(g_ui.panel_hwnd);
                g_ui.panel_hwnd = 0;
            }
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

static void initialize_window_size(AppState *state, int *outer_w, int *outer_h) {
    int screen_w = 1600;
    int screen_h = 900;
    int client_w = state->image.image_bgr->width;
    int client_h = state->image.image_bgr->height;
    RECT rect;
    platform_get_screen_size(&screen_w, &screen_h);
    while (client_w > (int) (screen_w * 0.8) || client_h > (int) (screen_h * 0.8)) {
        client_w = (int) floor((double) client_w * 0.7);
        client_h = (int) floor((double) client_h * 0.7);
    }
    if (client_w < 480) client_w = 480;
    if (client_h < 240) client_h = 240;
    rect.left = 0;
    rect.top = 0;
    rect.right = client_w;
    rect.bottom = client_h;
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    *outer_w = rect.right - rect.left;
    *outer_h = rect.bottom - rect.top;
}

int run_measure_app(AppState *state) {
#ifdef _WIN32
    WNDCLASSA wc;
    WNDCLASSA panel_wc;
    MSG msg;
    int outer_w;
    int outer_h;

    memset(&g_ui, 0, sizeof(g_ui));
    g_ui.state = state;
    reset_state_for_new_image(state);
    app_icon_initialize();

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = main_wndproc;
    wc.hInstance = GetModuleHandleA(0);
    wc.lpszClassName = APP_CLASS_NAME;
    wc.hCursor = LoadCursorA(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
    RegisterClassA(&wc);

    memset(&panel_wc, 0, sizeof(panel_wc));
    panel_wc.lpfnWndProc = panel_wndproc;
    panel_wc.hInstance = GetModuleHandleA(0);
    panel_wc.lpszClassName = PANEL_CLASS_NAME;
    panel_wc.hCursor = LoadCursorA(0, IDC_ARROW);
    panel_wc.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
    RegisterClassA(&panel_wc);

    initialize_window_size(state, &outer_w, &outer_h);
    g_ui.hwnd = CreateWindowExA(0, APP_CLASS_NAME, ui_text()->app_name, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, outer_w, outer_h, 0, 0, GetModuleHandleA(0), 0);
    if (g_ui.hwnd == 0) {
        app_icon_shutdown();
        return 1;
    }

    app_icon_apply(g_ui.hwnd);
    update_title(state);
    ShowWindow(g_ui.hwnd, SW_SHOW);
    UpdateWindow(g_ui.hwnd);
    platform_raise_window(g_ui.hwnd);

    g_ui.panel_hwnd = CreateWindowExA(WS_EX_TOOLWINDOW, PANEL_CLASS_NAME, ui_text()->panel_window_title,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 550, panel_window_height(ui_text()->usage_hint), g_ui.hwnd, 0, GetModuleHandleA(0), 0);
    if (g_ui.panel_hwnd != 0) {
        app_icon_apply(g_ui.panel_hwnd);
        center_panel_over_main();
        sync_marker_list(state);
    }
    platform_raise_window(g_ui.hwnd);
    if (g_ui.panel_hwnd != 0) {
        platform_raise_window(g_ui.panel_hwnd);
    }

    while (GetMessageA(&msg, 0, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    app_icon_shutdown();
    return 0;
#else
    (void) state;
    return 1;
#endif
}



