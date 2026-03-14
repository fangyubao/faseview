# FastView

FastView is a simple image viewer with measurement and edge-finding features.

## Build

```powershell
make
```

Output:

```powershell
smartmeasure.exe
```

## Run

```powershell
.\smartmeasure.exe
```

Or open a file directly:

```powershell
.\smartmeasure.exe path\\to\\image.png
```

## Shortcuts

- `Ctrl + Mouse Wheel`: zoom around the mouse position
- `Middle Mouse Drag`: pan the image
- `Space + Middle Mouse Click`: reset zoom to `100%` and center
- `Ctrl` while drawing: snap to the nearest detected corner
- `Shift` while drawing: lock the line to horizontal or vertical
- `Right Click`: cancel the current measurement
- `S`: save the current annotated view as a PNG
- `Arrow Keys`: resize the main window
- `Open`: switch to another image without restarting
- `Save Path`: choose the screenshot output folder

## Coordinate Panel

- Enter `X` and `Y`, then click `Add` to place a marker
- Up to `10` markers are supported
- Select a marker in the list and click `Delete` to remove it
- The panel also shows:
  - current image max coordinate
  - current screenshot save path
  - shortcut help text

## File Tree

```text
include/
  app_icon.h
  app_state.h
  image_analysis.h
  image_loader.h
  measure_app.h
  panel_ui.h
  platform.h
  ui_strings.h

src/
  app/
    main.c            startup and initial file selection
    measure_app.c     main window logic, rendering, mouse/keyboard behavior
  image/
    image_analysis.c  corner detection, gradients, edge hit collection
    image_loader.c    image file loading and preprocessing
  platform/
    platform.c        foreground window, folder dialog, PNG saving
  ui/
    app_icon.c        runtime window icon loading
    panel_ui.c        coordinate panel control creation and layout
    ui_strings.c      all editable UI text

ico/
  app.ico
  main.jpg

app.rc
Makefile
```

## Module Guide

- `src/app/`
  - application flow and interactive behavior
- `src/image/`
  - image loading, preprocessing, gradient and corner analysis
- `src/platform/`
  - Windows-specific helpers and system dialogs
- `src/ui/`
  - UI strings, icon handling, control creation and layout

## Where To Edit

- Change window titles, button text, help text:
  - `src/ui/ui_strings.c`
- Change coordinate panel layout or help text display:
  - `src/ui/panel_ui.c`
- Change main interaction behavior:
  - `src/app/measure_app.c`
- Change file dialogs, foreground behavior, screenshot saving:
  - `src/platform/platform.c`
- Change image processing logic:
  - `src/image/image_analysis.c`