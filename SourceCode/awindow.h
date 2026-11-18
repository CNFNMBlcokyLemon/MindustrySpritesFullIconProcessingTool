#ifndef AWINDOW_H  
#define AWINDOW_H  
  
#include <windows.h>  
  
// Add/Edit Weapon 弹窗的窗口过程（注册 AWClass 时使用）  
LRESULT CALLBACK awProc(HWND hw, UINT msg, WPARAM wp, LPARAM lp);  
  
// 打开弹窗：editIdx=-1 为新增模式（空表单）；  
// editIdx>=0 为编辑模式，用 g_wdefs[editIdx] 预填充  
void showAW(int editIdx);  
  
#endif // AWINDOW_H
