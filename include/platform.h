#pragma once

#include "app_state.h"

struct _IplImage;
typedef struct _IplImage IplImage;

int platform_is_space_pressed(void);
void platform_get_screen_size(int *width, int *height);
void platform_raise_window(void *window_handle);
int platform_select_folder(void *owner_window, const char *initial_path, char *out_path, int out_path_size);
void platform_set_window_title(const char *window_name, const char *title);
void platform_init_point_panel(const char *window_name, AppState *state);
void platform_sync_point_panel(const char *window_name, AppState *state);
void platform_shutdown_point_panel(void);
void platform_pump_messages(void);
int platform_save_bgr_png(const char *path, const IplImage *image);
