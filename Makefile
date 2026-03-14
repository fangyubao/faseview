APP := smartmeasure
SRC_DIR := src
INC_DIR := include
RESOURCE_BMP := build/resources/app_icon.bmp
RESOURCE_RC := app.rc
RESOURCE_OBJ := build/obj/app_res.o

ifeq ($(OS),Windows_NT)
EXE := .exe
CC := gcc
LD := g++
RC := windres
OPENCV_INC := -Ithird_party/opencv/modules/core/include -Ithird_party/opencv/modules/imgproc/include -Ithird_party/opencv/modules/imgcodecs/include -Ithird_party/opencv/modules/highgui/include -Ithird_party/opencv/include
OPENCV_LINK_LIBS := libopencv_highgui4140.dll libopencv_imgcodecs4140.dll libopencv_imgproc4140.dll libopencv_core4140.dll
SYS_LIBS := -lcomdlg32 -lole32 -luuid -luser32 -lgdi32
WINDOWS_GUI_LDFLAGS := -mwindows
COPY_DLLS = @echo DLLs already available in project root.
RM_CMD = @if exist "build\obj" rmdir /s /q "build\obj" & if exist "build\resources" rmdir /s /q "build\resources" & if exist "$(APP).exe" del /f /q "$(APP).exe"
MKDIR_CMD = @if not exist "$(subst /,\,$(dir $@))" mkdir "$(subst /,\,$(dir $@))"
RESOURCE_CMD = powershell -NoProfile -Command "Add-Type -AssemblyName System.Drawing; $$img=[System.Drawing.Image]::FromFile('ico/main.jpg'); $$bmp=New-Object System.Drawing.Bitmap 32,32; $$g=[System.Drawing.Graphics]::FromImage($$bmp); $$g.Clear([System.Drawing.Color]::Black); $$g.DrawImage($$img,0,0,32,32); $$bmp.Save('$(subst /,\,$@)', [System.Drawing.Imaging.ImageFormat]::Bmp); $$g.Dispose(); $$bmp.Dispose(); $$img.Dispose()"
else
EXE :=
CC := gcc
LD := g++
RC := windres
OPENCV_PKG ?= opencv4
OPENCV_INC := $(shell pkg-config --cflags $(OPENCV_PKG))
OPENCV_LINK_LIBS := $(shell pkg-config --libs $(OPENCV_PKG))
SYS_LIBS :=
WINDOWS_GUI_LDFLAGS :=
COPY_DLLS = true
RM_CMD = rm -rf build/obj build/resources $(APP)
MKDIR_CMD = @mkdir -p $(dir $@)
RESOURCE_CMD = @mkdir -p $(dir $@) && cp ico/main.jpg $@
endif

CFLAGS := -std=c11 -O3 -DNDEBUG -Wall -Wextra -I$(INC_DIR) -Ithird_party/tinyfiledialogs -Ithird_party/stb $(OPENCV_INC) -Wno-unused-function
LDFLAGS := $(WINDOWS_GUI_LDFLAGS) $(OPENCV_LINK_LIBS) $(SYS_LIBS)
SRCS := $(SRC_DIR)/app/main.c $(SRC_DIR)/image/image_loader.c $(SRC_DIR)/image/image_analysis.c $(SRC_DIR)/app/measure_app.c $(SRC_DIR)/ui/panel_ui.c $(SRC_DIR)/platform/platform.c $(SRC_DIR)/ui/ui_strings.c $(SRC_DIR)/ui/app_icon.c third_party/tinyfiledialogs/tinyfiledialogs.c
OBJS := $(patsubst %.c,build/obj/%.o,$(SRCS))

.PHONY: all clean

all: $(APP)$(EXE)

$(RESOURCE_BMP): ico/main.jpg
	$(MKDIR_CMD)
	$(RESOURCE_CMD)

$(APP)$(EXE): $(RESOURCE_BMP) $(RESOURCE_OBJ) $(OBJS)
	$(LD) -o $@ $(OBJS) $(RESOURCE_OBJ) $(LDFLAGS)
	$(COPY_DLLS)

$(RESOURCE_OBJ): $(RESOURCE_RC) ico/app.ico
	$(MKDIR_CMD)
	$(RC) -O coff $< -o $@

build/obj/%.o: %.c
	$(MKDIR_CMD)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM_CMD)

