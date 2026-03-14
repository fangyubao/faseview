#include <stdio.h>
#include <string.h>

#include <opencv2/core/core_c.h>

#include "app_state.h"
#include "image_analysis.h"
#include "image_loader.h"
#include "measure_app.h"
#include "ui_strings.h"

#include "tinyfiledialogs.h"

static const char *choose_input_path(int argc, char **argv) {
    int filter_count = 0;
    const UiText *ui = ui_text();
    const char *const *filters = ui_image_filters(&filter_count);
    if (argc > 1 && argv[1] != 0 && argv[1][0] != '\0') {
        return argv[1];
    }
    return tinyfd_openFileDialog(ui->select_image_title, "", filter_count, filters, ui->image_filter_description, 0);
}

int main(int argc, char **argv) {
    AppState state;
    const char *path = choose_input_path(argc, argv);

    memset(&state, 0, sizeof(state));
    state.window_name = ui_text()->app_name;
    state.zoom = 1.0;
    state.start_raw = cvPoint(-1, -1);
    strcpy(state.screenshot_dir, ".");

    if (path == 0) {
        return 0;
    }

    if (!load_image_file(path, &state.image)) {
        tinyfd_messageBox(ui_text()->app_name, ui_text()->image_load_failed, "ok", "error", 1);
        return 1;
    }

    run_measure_app(&state);
    release_image_data(&state.image);
    return 0;
}
