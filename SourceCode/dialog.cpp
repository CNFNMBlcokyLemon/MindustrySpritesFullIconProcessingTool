#include "dialog.h"  
  
#include <commdlg.h>  
#include <shlobj.h>  
#include <string.h>  
#include <stdio.h>  
  
#ifdef LANG_ZH  
#include "lang_zh.h"  
#else  
#include "lang_en.h"  
#endif  
  
bool browseImg(HWND hw, char* buf, int size) {  
    OPENFILENAMEA ofn;  
    memset(&ofn, 0, sizeof(ofn));  
    ofn.lStructSize  = sizeof(ofn);  
    ofn.hwndOwner    = hw;  
    ofn.lpstrFilter  = STR_FILTER_IMG;  
    ofn.lpstrFile    = buf;  
    ofn.nMaxFile     = size;  
    ofn.Flags        = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;  
    return GetOpenFileNameA(&ofn) != 0;  
}  
  
bool browseSave(HWND hw, char* buf, int size) {  
    OPENFILENAMEA ofn;  
    memset(&ofn, 0, sizeof(ofn));  
    ofn.lStructSize  = sizeof(ofn);  
    ofn.hwndOwner    = hw;  
    ofn.lpstrFilter  = STR_FILTER_SAVE;  
    ofn.lpstrFile    = buf;  
    ofn.nMaxFile     = size;  
    ofn.lpstrDefExt  = "png";  
    ofn.Flags        = OFN_OVERWRITEPROMPT;  
    return GetSaveFileNameA(&ofn) != 0;  
}  
  
bool browseFolder(HWND hw, char* buf, int size) {  
    BROWSEINFOA bi;  
    memset(&bi, 0, sizeof(bi));  
    char disp[MAX_PATH] = "";  
    bi.hwndOwner      = hw;  
    bi.pszDisplayName = disp;  
    bi.lpszTitle      = STR_FOLDER_TITLE;  
    bi.ulFlags        = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;  
    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);  
    if (!pidl) return false;  
    bool ok = SHGetPathFromIDListA(pidl, buf) != 0;  
    CoTaskMemFree(pidl);  
    (void)size;  
    return ok;  
}  
  
void extractBasename(const char* full, char* out, int outSize) {  
    const char* slash1 = strrchr(full, '\\');  
    const char* slash2 = strrchr(full, '/');  
    const char* p = full;  
    if (slash1 && (!slash2 || slash1 > slash2)) p = slash1 + 1;  
    else if (slash2) p = slash2 + 1;  
    strncpy(out, p, outSize - 1);  
    out[outSize - 1] = 0;  
}
