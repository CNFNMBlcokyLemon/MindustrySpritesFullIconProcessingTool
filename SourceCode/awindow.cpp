#include "awindow.h"  
  
#include <stdio.h>  
#include <string.h>  
#include <stdlib.h>  
#include <shellapi.h>  
  
#include "ids.h"  
#include "globals.h"  
#include "dialog.h"  
  
#ifdef LANG_ZH  
#include "lang_zh.h"  
#else  
#include "lang_en.h"  
#endif  
  
/* dialog globals (extern-declared in globals.h) */  
HWND g_aw       = NULL;  
bool g_awOK     = false;  
WD   g_awResult;  
int  g_editIdx  = -1;  
  
/* guard flag for RGB<->HEX sync inside the dialog */  
static bool g_awHexSync = false;  
  
/* parse "#rrggbb" or "rrggbb"; returns false on invalid input */  
static bool awParseHex(const char* s, int* r, int* g, int* b) {  
    if (s[0] == '#') s++;  
    if (strlen(s) != 6) return false;  
    unsigned int rv, gv, bv;  
    if (sscanf(s, "%02x%02x%02x", &rv, &gv, &bv) != 3) return false;  
    *r = (int)rv; *g = (int)gv; *b = (int)bv;  
    return true;  
}  
  
static void awSyncHexFromRGB(HWND hw) {  
    char rb[16] = "0", gb[16] = "0", bb[16] = "0", hex[16];  
    GetDlgItemTextA(hw, ID_AWOR, rb, 16);  
    GetDlgItemTextA(hw, ID_AWOG, gb, 16);  
    GetDlgItemTextA(hw, ID_AWOB, bb, 16);  
    sprintf(hex, "#%02X%02X%02X", atoi(rb) & 255, atoi(gb) & 255, atoi(bb) & 255);  
    g_awHexSync = true;  
    SetDlgItemTextA(hw, ID_AWOHEX, hex);  
    g_awHexSync = false;  
}  
  
static bool awSyncRGBFromHex(HWND hw) {  
    char hex[16] = "";  
    GetDlgItemTextA(hw, ID_AWOHEX, hex, 16);  
    int r, g, b;  
    if (!awParseHex(hex, &r, &g, &b)) return false;  
    char tmp[16];  
    g_awHexSync = true;  
    sprintf(tmp, "%d", r); SetDlgItemTextA(hw, ID_AWOR, tmp);  
    sprintf(tmp, "%d", g); SetDlgItemTextA(hw, ID_AWOG, tmp);  
    sprintf(tmp, "%d", b); SetDlgItemTextA(hw, ID_AWOB, tmp);  
    g_awHexSync = false;  
    return true;  
}  
  
/* shared close path for Cancel button and window X:  
   re-enables the main window, resets edit mode, destroys the dialog */  
static void awClose(HWND hw) {  
    EnableWindow(g_hw, TRUE);  
    SetForegroundWindow(g_hw);  
    g_editIdx = -1;  
    DestroyWindow(hw);  
}  
  
LRESULT CALLBACK awProc(HWND hw, UINT msg, WPARAM wp, LPARAM lp) {  
    switch (msg) {  
        case WM_COMMAND:  
            /* live hex <-> rgb conversion */  
            if (HIWORD(wp) == EN_CHANGE && !g_awHexSync) {  
                int id = LOWORD(wp);  
                if (id == ID_AWOHEX) {  
                    awSyncRGBFromHex(hw);  
                } else if (id == ID_AWOR || id == ID_AWOG || id == ID_AWOB) {  
                    awSyncHexFromRGB(hw);  
                }  
            }  
            switch (LOWORD(wp)) {  
                case ID_AWB: {  
                    char buf[MAX_PATH] = "";  
                    if (browseImg(hw, buf, MAX_PATH))  
                        SetDlgItemTextA(hw, ID_AWP, buf);  
                    break;  
                }  
                case ID_AWOK: {  
                    char nm[128] = "", pt[MAX_PATH] = "", xb[32] = "0", yb[32] = "0", brb[32] = "0";  
                    GetDlgItemTextA(hw, ID_AWN, nm, 128);  
                    GetDlgItemTextA(hw, ID_AWP, pt, MAX_PATH);  
                    GetDlgItemTextA(hw, ID_AWX, xb, 32);  
                    GetDlgItemTextA(hw, ID_AWY, yb, 32);  
                    GetDlgItemTextA(hw, ID_AWBR, brb, 32);  
                    if (!strlen(nm)) {  
                        MessageBoxA(hw, STR_AW_ERR_NAME, STR_MSG_ERROR, MB_OK | MB_ICONERROR);  
                        break;  
                    }  
                    WD d;  
                    d.nm      = nm;  
                    d.pt      = pt;  
                    /* input x/y are stored x4 internally */  
                    d.x       = (float)atof(xb) * 4.0f;  
                    d.y       = (float)atof(yb) * 4.0f;  
                    d.baseRot = (float)atof(brb);  
                    d.mir             = SendDlgItemMessage(hw, ID_AWM,   BM_GETCHECK, 0, 0) == BST_CHECKED;  
                    d.op.on           = SendDlgItemMessage(hw, ID_AWO,   BM_GETCHECK, 0, 0) == BST_CHECKED;  
                    d.op.outlineOnly  = SendDlgItemMessage(hw, ID_AWOOL, BM_GETCHECK, 0, 0) == BST_CHECKED;  
                    char rb[16] = "86", gb[16] = "86", bb[16] = "102", radb[16] = "3";  
                    GetDlgItemTextA(hw, ID_AWOR, rb, 16);  
                    GetDlgItemTextA(hw, ID_AWOG, gb, 16);  
                    GetDlgItemTextA(hw, ID_AWOB, bb, 16);  
                    GetDlgItemTextA(hw, ID_AWORAD, radb, 16);  
                    d.op.r   = (unsigned char)atoi(rb);  
                    d.op.g   = (unsigned char)atoi(gb);  
                    d.op.b   = (unsigned char)atoi(bb);  
                    d.op.rad = atoi(radb);  
                    if (d.op.rad < 1) d.op.rad = 1;  
  
                    if (g_editIdx >= 0 && g_editIdx < (int)g_wdefs.size()) {  
                        /* edit mode: keep zorder and visible from the original */  
                        d.zorder  = g_wdefs[g_editIdx].zorder;  
                        d.visible = g_wdefs[g_editIdx].visible;  
                    } else {  
                        /* add mode: new layer goes on top */  
                        int mz = -1;  
                        for (int i = 0; i < (int)g_wdefs.size(); i++)  
                            if (g_wdefs[i].zorder > mz) mz = g_wdefs[i].zorder;  
                        d.zorder  = mz + 1;  
                        d.visible = true;  
                    }  
                    g_awResult = d;  
                    g_awOK     = true;  
                    EnableWindow(g_hw, TRUE);  
                    SetForegroundWindow(g_hw);  
                    DestroyWindow(hw);  
                    break;  
                }  
                case ID_AWCA:  
                    awClose(hw);  
                    break;  
            }  
            break;  
        case WM_DROPFILES: {  
            HDROP hd = (HDROP)wp;  
            char buf[MAX_PATH] = "";  
            DragQueryFileA(hd, 0, buf, MAX_PATH);  
            DragFinish(hd);  
            SetDlgItemTextA(hw, ID_AWP, buf);  
            break;  
        }  
        case WM_CLOSE:  
            /* X button behaves exactly like Cancel */  
            awClose(hw);  
            break;  
        case WM_DESTROY:  
            g_aw = NULL;  
            break;  
    }  
    return DefWindowProcA(hw, msg, wp, lp);  
}  
  
void showAW(int editIdx) {  
    if (g_aw) return;  
    g_editIdx = editIdx;  
    const WD* src = (editIdx >= 0 && editIdx < (int)g_wdefs.size()) ? &g_wdefs[editIdx] : NULL;  
    HINSTANCE hi = GetModuleHandle(NULL);  
    g_aw = CreateWindowExA(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST | WS_EX_ACCEPTFILES,  
                           "AWClass", src ? STR_AW_EDIT : STR_AW_ADD,  
                           WS_CAPTION | WS_SYSMENU | WS_VISIBLE,  
                           CW_USEDEFAULT, CW_USEDEFAULT, 300, 435,  
                           g_hw, NULL, hi, NULL);  
    int h = 22, pad = 4, y = 5;  
    CreateWindowA("STATIC", STR_AW_NAME, WS_CHILD | WS_VISIBLE, 5, y, 60, h, g_aw, NULL, hi, NULL);  
    CreateWindowA("EDIT", src ? src->nm.c_str() : "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,  
                  70, y, 210, h, g_aw, (HMENU)ID_AWN, hi, NULL);  
    y += h + pad;  
    CreateWindowA("STATIC", STR_AW_PATH, WS_CHILD | WS_VISIBLE, 5, y, 60, h, g_aw, NULL, hi, NULL);  
    CreateWindowA("EDIT", src ? src->pt.c_str() : "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,  
                  70, y, 160, h, g_aw, (HMENU)ID_AWP, hi, NULL);  
    CreateWindowA("BUTTON", STR_BROWSE, WS_CHILD | WS_VISIBLE, 235, y, 45, h, g_aw, (HMENU)ID_AWB, hi, NULL);  
    y += h + pad;  
    {  
        char xb[32] = "0", yb[32] = "0";  
        if (src) {  
            /* display stored value /4 to match user input scale */  
            sprintf(xb, "%.2f", src->x / 4.0f);  
            sprintf(yb, "%.2f", src->y / 4.0f);  
        }  
        CreateWindowA("STATIC", STR_AW_X, WS_CHILD | WS_VISIBLE, 5, y, 20, h, g_aw, NULL, hi, NULL);  
        CreateWindowA("EDIT", xb, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,  
                      28, y, 80, h, g_aw, (HMENU)ID_AWX, hi, NULL);  
        CreateWindowA("STATIC", STR_AW_Y, WS_CHILD | WS_VISIBLE, 120, y, 20, h, g_aw, NULL, hi, NULL);  
        CreateWindowA("EDIT", yb, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,  
                      143, y, 80, h, g_aw, (HMENU)ID_AWY, hi, NULL);  
    }  
    y += h + pad;  
    {  
        char brb[32] = "0";  
        if (src) sprintf(brb, "%.2f", src->baseRot);  
        CreateWindowA("STATIC", STR_AW_BASEROT, WS_CHILD | WS_VISIBLE, 5, y, 65, h, g_aw, NULL, hi, NULL);  
        CreateWindowA("EDIT", brb, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,  
                      75, y, 80, h, g_aw, (HMENU)ID_AWBR, hi, NULL);  
    }  
    y += h + pad;  
    CreateWindowA("BUTTON", STR_AW_MIRROR, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,  
                  5, y, 120, h, g_aw, (HMENU)ID_AWM, hi, NULL);  
    SendDlgItemMessage(g_aw, ID_AWM, BM_SETCHECK,  
                       (src ? (src->mir ? BST_CHECKED : BST_UNCHECKED) : BST_CHECKED), 0);  
    y += h + pad;  
    CreateWindowA("BUTTON", STR_AW_OUTLINE, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,  
                  5, y, 120, h, g_aw, (HMENU)ID_AWO, hi, NULL);  
    SendDlgItemMessage(g_aw, ID_AWO, BM_SETCHECK,  
                       (src && src->op.on) ? BST_CHECKED : BST_UNCHECKED, 0);  
    y += h + pad;  
    {  
        char rb[16], gb[16], bb[16], radb[16] = "3";  
        if (src) {  
            sprintf(rb, "%d", (int)src->op.r);  
            sprintf(gb, "%d", (int)src->op.g);  
            sprintf(bb, "%d", (int)src->op.b);  
            sprintf(radb, "%d", src->op.rad);  
        } else {  
            /* default outline color follows the unit outline color */  
            sprintf(rb, "%d", (int)g_uop.r);  
            sprintf(gb, "%d", (int)g_uop.g);  
            sprintf(bb, "%d", (int)g_uop.b);  
        }  
        CreateWindowA("STATIC", "R:", WS_CHILD | WS_VISIBLE, 5, y, 20, h, g_aw, NULL, hi, NULL);  
        CreateWindowA("EDIT", rb, WS_CHILD | WS_VISIBLE | WS_BORDER, 28, y, 38, h, g_aw, (HMENU)ID_AWOR, hi, NULL);  
        CreateWindowA("STATIC", "G:", WS_CHILD | WS_VISIBLE, 75, y, 20, h, g_aw, NULL, hi, NULL);  
        CreateWindowA("EDIT", gb, WS_CHILD | WS_VISIBLE | WS_BORDER, 98, y, 38, h, g_aw, (HMENU)ID_AWOG, hi, NULL);  
        CreateWindowA("STATIC", "B:", WS_CHILD | WS_VISIBLE, 145, y, 20, h, g_aw, NULL, hi, NULL);  
        CreateWindowA("EDIT", bb, WS_CHILD | WS_VISIBLE | WS_BORDER, 168, y, 38, h, g_aw, (HMENU)ID_AWOB, hi, NULL);  
        CreateWindowA("STATIC", STR_AW_RAD, WS_CHILD | WS_VISIBLE, 215, y, 30, h, g_aw, NULL, hi, NULL);  
        CreateWindowA("EDIT", radb, WS_CHILD | WS_VISIBLE | WS_BORDER, 248, y, 28, h, g_aw, (HMENU)ID_AWORAD, hi, NULL);  
    }  
    y += h + pad;  
    {  
        char hex[16];  
        if (src) sprintf(hex, "#%02X%02X%02X", (int)src->op.r, (int)src->op.g, (int)src->op.b);  
        else     sprintf(hex, "#%02X%02X%02X", (int)g_uop.r, (int)g_uop.g, (int)g_uop.b);  
        CreateWindowA("STATIC", STR_HEX, WS_CHILD | WS_VISIBLE, 5, y, 30, h, g_aw, NULL, hi, NULL);  
        CreateWindowA("EDIT", hex, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,  
                      40, y, 90, h, g_aw, (HMENU)ID_AWOHEX, hi, NULL);  
    }  
    y += h + pad;  
    CreateWindowA("BUTTON", STR_OUTLINE_ONLY, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,  
                  5, y, 200, h, g_aw, (HMENU)ID_AWOOL, hi, NULL);  
    SendDlgItemMessage(g_aw, ID_AWOOL, BM_SETCHECK,  
                       (src && src->op.outlineOnly) ? BST_CHECKED : BST_UNCHECKED, 0);  
    y += h + pad + 5;  
    CreateWindowA("BUTTON", STR_AW_OK, WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,  
                  5, y, 80, h + 4, g_aw, (HMENU)ID_AWOK, hi, NULL);  
    CreateWindowA("BUTTON", STR_AW_CANCEL, WS_CHILD | WS_VISIBLE,  
                  95, y, 80, h + 4, g_aw, (HMENU)ID_AWCA, hi, NULL);  
    EnableWindow(g_hw, FALSE);  
    ShowWindow(g_aw, SW_SHOW);  
    UpdateWindow(g_aw);  
}
