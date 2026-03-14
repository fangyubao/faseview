# FastView

[English](README.md) | [简体中文](README.zh.md)

FastView 是一个简单的带测量功能和边缘查找的图像查看器。

## 编译

```powershell
make
```

输出文件：

```powershell
smartmeasure.exe
```

## 运行

```powershell
.\smartmeasure.exe
```

也可以直接带图片路径：

```powershell
.\smartmeasure.exe 路径\\图片.png
```

## 快捷键

- `Ctrl + 鼠标滚轮`：以鼠标位置为中心缩放
- `鼠标中键拖拽`：拖动画面
- `空格 + 鼠标中键单击`：恢复 `100%` 缩放并居中
- `按住 Ctrl` 再拉线：吸附到最近角点
- `按住 Shift` 再拉线：锁定为水平或垂直
- `鼠标右键`：取消当前测量
- `S`：保存当前带标注截图
- `方向键`：调整主窗口大小
- `Open`：不重启直接切换图片
- `Save Path`：设置截图保存目录

## 坐标窗口

- 输入 `X` 和 `Y` 后点击 `Add`，可在图上增加坐标点
- 最多支持 `10` 个坐标点
- 在列表中选中后点击 `Delete` 可删除
- 窗口中还会显示：
  - 当前图像最大坐标
  - 当前截图保存路径
  - 快捷键说明

## 文件树说明

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
    main.c            启动与初次选图
    measure_app.c     主窗口逻辑、渲染、鼠标键盘行为
  image/
    image_analysis.c  角点、梯度、边缘命中计算
    image_loader.c    图片读取与预处理
  platform/
    platform.c        前台激活、文件夹选择、PNG 保存
  ui/
    app_icon.c        运行时窗口图标加载
    panel_ui.c        坐标窗口控件创建与布局
    ui_strings.c      所有可改 UI 文案

ico/
  app.ico
  main.jpg

app.rc
Makefile
```

## 模块说明

- `src/app/`
  - 应用流程和交互行为
- `src/image/`
  - 图像加载、预处理、梯度和角点分析
- `src/platform/`
  - Windows 平台相关能力和系统对话框
- `src/ui/`
  - 文案、图标、控件创建和布局

## 想改哪里看哪里

- 改主窗口标题、按钮文字、说明文字：
  - `src/ui/ui_strings.c`
- 改坐标窗口布局、说明区显示方式：
  - `src/ui/panel_ui.c`
- 改主交互逻辑：
  - `src/app/measure_app.c`
- 改打开目录、窗口置顶、截图保存实现：
  - `src/platform/platform.c`
- 改图像处理算法：
  - `src/image/image_analysis.c`
