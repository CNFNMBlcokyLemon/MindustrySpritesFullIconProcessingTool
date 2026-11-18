#ifndef DIALOG_H  
#define DIALOG_H  
  
#include <windows.h>  
  
// 打开 PNG 图片选择对话框，成功返回 true 并把路径写入 buf  
bool browseImg(HWND hw, char* buf, int size);  
  
// 打开 PNG 保存对话框，成功返回 true 并把路径写入 buf  
bool browseSave(HWND hw, char* buf, int size);  
  
// 打开文件夹选择对话框，成功返回 true 并把路径写入 buf  
bool browseFolder(HWND hw, char* buf, int size);  
  
// 从完整路径中提取纯文件名（basename），供输出文件夹拼接使用  
void extractBasename(const char* full, char* out, int outSize);  
  
#endif // DIALOG_H
