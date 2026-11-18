#ifndef RENDER_H  
#define RENDER_H  
  
#include <windows.h>  
  
// 状态栏文字  
void setStatus(const char* s);  
  
// 从 ID_UP 输入框读取路径并加载单位本体图（"0" = 32x32 透明占位图）  
void loadUnit();  
  
// 重新合成预览图：排序 -> 展开镜像 -> 逐层翻转/描边/旋转/blit -> 刷新显示  
void doRefresh();  
  
// 把 g_preview 写出为 PNG（支持输出文件夹 + 文件名拼接）  
void doGenerate();  
  
// WM_PAINT 绘制：棋盘格背景 + 缩放显示 + 可选网格  
void drawPreview(HDC hdc, RECT* rc);  
  
// 刷新武器层 listbox 显示  
void updList();  
  
#endif // RENDER_H
