
#include "render.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "ids.h"
#include "globals.h"
#include "dialog.h"
#include "model.h"
#include "image.h"

#ifdef LANG_ZH
#include "lang_zh.h"
#else
#include "lang_en.h"
#endif

void setStatus(const char* s) {
    if (g_stb) SetWindowTextA(g_stb, s);
}

void updList() {
    if (!g_wlb) return;
    SendMessageA(g_wlb, LB_RESETCONTENT, 0, 0);
    for (int i = 0; i < (int)g_wdefs.size(); i++) {
        char buf[256];
        sprintf(buf, "%s z=%d %s (%.1f,%.1f) rot=%.1f%s",
                g_wdefs[i].visible ? "[V]" : "[H]",
                g_wdefs[i].zorder,
                g_wdefs[i].nm.c_str(), g_wdefs[i].x, g_wdefs[i].y, g_wdefs[i].baseRot,
                g_wdefs[i].mir ? " [M]" : "");
        SendMessageA(g_wlb, LB_ADDSTRING, 0, (LPARAM)buf);
    }
}

void loadUnit() {
    char buf[MAX_PATH] = "";
    HWND h = GetDlgItem(g_hw, ID_UP);
    if (h) GetWindowTextA(h, buf, MAX_PATH);
    g_unit.free2();
    if (strcmp(buf, "0") == 0) {
        g_unit.w = 32;
        g_unit.h = 32;
        g_unit.d = (unsigned char*)calloc(32 * 32 * 4, 1);
        setStatus(STR_ST_PLACEHOLDER);
    } else if (strlen(buf)) {
        if (g_unit.load(buf)) {
            char s[256];
            sprintf(s, STR_ST_LOADED, g_unit.w, g_unit.h);
            setStatus(s);
        } else {
            char s[256];
            sprintf(s, STR_ST_ERR_LOAD, buf);
            setStatus(s);
        }
    }
}

void doRefresh() {
    char rb[16] = "86", gb[16] = "86", bb[16] = "102", radb[16] = "3";
    GetDlgItemTextA(g_hw, ID_UOR, rb, 16);
    GetDlgItemTextA(g_hw, ID_UOG, gb, 16);
    GetDlgItemTextA(g_hw, ID_UOB, bb, 16);
    GetDlgItemTextA(g_hw, ID_UORAD, radb, 16);
    g_uop.r   = (unsigned char)atoi(rb);
    g_uop.g   = (unsigned char)atoi(gb);
    g_uop.b   = (unsigned char)atoi(bb);
    g_uop.rad = atoi(radb);
    if (g_uop.rad < 1) g_uop.rad = 1;
    g_uop.on          = SendDlgItemMessage(g_hw, ID_UO,   BM_GETCHECK, 0, 0) == BST_CHECKED;
    g_uop.outlineOnly = SendDlgItemMessage(g_hw, ID_UOOL, BM_GETCHECK, 0, 0) == BST_CHECKED;

    /* sort by zorder, filter invisible layers; origIdx maps sortedDefs idx -> g_wdefs idx */
    std::vector<int> order = zorderIndices(g_wdefs);
    std::vector<WD> sortedDefs;
    std::vector<int> origIdx;
    sortedDefs.reserve(g_wdefs.size());
    origIdx.reserve(g_wdefs.size());
    for (int i = 0; i < (int)order.size(); i++) {
        int oi = order[i];
        if (!g_wdefs[oi].visible) continue;
        sortedDefs.push_back(g_wdefs[oi]);
        origIdx.push_back(oi);
    }

    std::vector<WI> ws = expand(sortedDefs);

    /* load all weapon images */
    std::vector<Img> imgs(ws.size());
    for (int i = 0; i < (int)ws.size(); i++) {
        if (!imgs[i].load(ws[i].pt.c_str())) {
            char s[256];
            sprintf(s, STR_ST_WARN_LOAD, ws[i].pt.c_str());
            setStatus(s);
        }
    }

    /* bounding box: unit + all weapon offsets, plus padding */
    int minX = -g_unit.w / 2, maxX = g_unit.w / 2;
    int minY = -g_unit.h / 2, maxY = g_unit.h / 2;
    for (int i = 0; i < (int)ws.size(); i++) {
        if (!imgs[i].ok()) continue;
        int hw2 = imgs[i].w / 2 + imgs[i].w, hh2 = imgs[i].h / 2 + imgs[i].h;
        int cx = (int)ws[i].x, cy = (int)ws[i].y;
        if (cx - hw2 < minX) minX = cx - hw2;
        if (cx + hw2 > maxX) maxX = cx + hw2;
        if (cy - hh2 < minY) minY = cy - hh2;
        if (cy + hh2 > maxY) maxY = cy + hh2;
    }
    int pad = 8;
    int cw = (maxX - minX) + pad * 2, ch = (maxY - minY) + pad * 2;
    if (cw < 1) cw = 1;
    if (ch < 1) ch = 1;
    Cv cv(cw, ch);
    if (!cv.d) { setStatus(STR_ST_SAVE_FAIL); return; }
    int ucx = -minX + pad, ucy = -minY + pad;

    /* unit body first */
    if (g_unit.ok()) {
        Img u = g_unit;
        Img ol;
        const Img* src = &u;
        if (g_uop.on) {
            ol = mkOutline(u, g_uop.r, g_uop.g, g_uop.b, g_uop.rad, g_uop.outlineOnly);
            src = &ol;
        }
        cv.blit(*src, ucx, ucy);
        if (ol.d) free(ol.d);
    }

    /* weapon layers in zorder */
    for (int i = 0; i < (int)ws.size(); i++) {
        if (!imgs[i].ok()) continue;
        Img fl, ol, rot;
        const Img* src = &imgs[i];
        if (ws[i].fl) {
            fl = imgs[i].flipH();
            src = &fl;
        }
        if (ws[i].op.on) {
            ol = mkOutline(*src, ws[i].op.r, ws[i].op.g, ws[i].op.b,
                           ws[i].op.rad, ws[i].op.outlineOnly);
            if (fl.d) free(fl.d);
            fl.d = NULL;
            src = &ol;
        }
        if (ws[i].baseRot != 0.0f) {
            rot = rotateImg(*src, -ws[i].baseRot);
            if (fl.d) free(fl.d);
            if (ol.d) free(ol.d);
            fl.d = NULL; ol.d = NULL;
            src = &rot;
        }
        /* red second outline for the selected layer (preview only) */
        Img hl;
        if (ws[i].srcIdx >= 0 && ws[i].srcIdx < (int)origIdx.size()
            && origIdx[ws[i].srcIdx] == g_selIdx) {
            hl = mkOutline(*src, 255, 0, 0, 2, false);
            src = &hl;
        }
        int px = ucx + (int)ws[i].x;
        int py = ucy - (int)ws[i].y;
        cv.blit(*src, px, py);
        if (fl.d)  free(fl.d);
        if (ol.d)  free(ol.d);
        if (rot.d) free(rot.d);
        if (hl.d)  free(hl.d);
    }

    /* copy result into preview */
    g_preview.free2();
    g_preview.w = cw;
    g_preview.h = ch;
    g_preview.d = (unsigned char*)malloc(cw * ch * 4);
    if (g_preview.d) memcpy(g_preview.d, cv.d, cw * ch * 4);
    InvalidateRect(g_hw, NULL, FALSE);
}

/* convert a GBK/ANSI string to UTF-8 (for JSON output) */
static void toUtf8(const char* in, char* out, int outSize) {
    wchar_t wbuf[512];
    int wlen = MultiByteToWideChar(CP_ACP, 0, in, -1, wbuf, 512);
    if (wlen <= 0) { strncpy(out, in, outSize - 1); out[outSize - 1] = 0; return; }
    int n = WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, out, outSize, NULL, NULL);
    if (n <= 0) { strncpy(out, in, outSize - 1); out[outSize - 1] = 0; }
}

/* write the weapon data json next to the exported png */
static void writeJson(const char* pngPath) {
    char jp[MAX_PATH];
    strncpy(jp, pngPath, MAX_PATH - 1);
    jp[MAX_PATH - 1] = 0;
    char* dot = strrchr(jp, '.');
    if (dot) strcpy(dot, ".json");
    else     strcat(jp, ".json");

    char base[MAX_PATH];
    extractBasename(pngPath, base, MAX_PATH);
    char* bdot = strrchr(base, '.');
    if (bdot) *bdot = 0;

    FILE* f = fopen(jp, "wb");
    if (!f) return;
    /* UTF-8 BOM so editors detect encoding correctly */
    fputc(0xEF, f); fputc(0xBB, f); fputc(0xBF, f);

    char ubase[MAX_PATH];
    toUtf8(base, ubase, MAX_PATH);
    fprintf(f, "[\r\n//%s's weapons\r\n", ubase);
    fprintf(f, "outlineColorDec: (%d, %d, %d),\r\n",
            (int)g_uop.r, (int)g_uop.g, (int)g_uop.b);
    fprintf(f, "outlineColorHex: (#%02X%02X%02X),\r\n",
            (int)g_uop.r, (int)g_uop.g, (int)g_uop.b);
    for (int i = 0; i < (int)g_wdefs.size(); i++) {
        char unm[256];
        toUtf8(g_wdefs[i].nm.c_str(), unm, 256);
        fprintf(f, "{\r\n//%s\r\n", unm);
        fprintf(f, "weaponName: (%s),\r\n", unm);
        fprintf(f, "x: (%.2f),\r\n", g_wdefs[i].x / 4.0f);
        fprintf(f, "y: (%.2f),\r\n", g_wdefs[i].y / 4.0f);
        fprintf(f, "mirror: %s,\r\n", g_wdefs[i].mir ? "true" : "false");
        fprintf(f, "baseRoation: (%.2f),\r\n", g_wdefs[i].baseRot);
        fprintf(f, "},\r\n");
    }
    fprintf(f, "]\r\n");
    fclose(f);
}

void doGenerate() {
    if (!g_preview.ok()) {
        setStatus(STR_ST_NO_PREVIEW);
        return;
    }
    /* re-render without the red selection highlight so it never enters the file */
    int savedSel = g_selIdx;
    g_selIdx = -1;
    doRefresh();

    char op[MAX_PATH] = "", od[MAX_PATH] = "", final[MAX_PATH] = "";
    GetDlgItemTextA(g_hw, ID_OP, op, MAX_PATH);
    GetDlgItemTextA(g_hw, ID_OD, od, MAX_PATH);
    if (strlen(od)) {
        char base[MAX_PATH];
        extractBasename(op, base, MAX_PATH);
        if (!strlen(base)) strcpy(base, "output.png");
        sprintf(final, "%s\\%s", od, base);
    } else {
        strncpy(final, op, MAX_PATH - 1);
    }
    if (!strlen(final)) {
        setStatus(STR_ST_SAVE_FAIL);
        g_selIdx = savedSel;
        doRefresh();
        return;
    }
    int ok = stbi_write_png(final, g_preview.w, g_preview.h, 4,
                            g_preview.d, g_preview.w * 4);
    if (ok) {
        writeJson(final);
        char s[300];
        sprintf(s, STR_ST_SAVED, final);
        setStatus(s);
        MessageBoxA(g_hw, s, STR_MSG_DONE, MB_OK | MB_ICONINFORMATION);
    } else {
        setStatus(STR_ST_SAVE_FAIL);
        MessageBoxA(g_hw, STR_MSG_SAVE_FAIL, STR_MSG_ERROR, MB_OK | MB_ICONERROR);
    }
    /* restore selection highlight in the preview */
    g_selIdx = savedSel;
    doRefresh();
}

void drawPreview(HDC hdc, RECT* rc) {
    /* solid #dae5d6 background replaces the checkerboard (display only) */
    HBRUSH db = CreateSolidBrush(RGB(0xDA, 0xE5, 0xD6));
    FillRect(hdc, rc, db);
    DeleteObject(db);
    if (!g_preview.ok()) {
        SetTextColor(hdc, RGB(90, 90, 90));
        SetBkMode(hdc, TRANSPARENT);
        DrawTextA(hdc, STR_PREVIEW_HINT, -1, rc,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        return;
    }
    int pw = g_preview.w, ph = g_preview.h;
    int imgW = (int)(pw * g_zoomF + 0.5f), imgH = (int)(ph * g_zoomF + 0.5f);
    if (imgW < 1) imgW = 1;
    if (imgH < 1) imgH = 1;
    int rcW = rc->right - rc->left, rcH = rc->bottom - rc->top;
    int offX = rc->left + rcW / 2 + (int)g_panX - imgW / 2;
    int offY = rc->top  + rcH / 2 + (int)g_panY - imgH / 2;

    unsigned char* bgra = (unsigned char*)malloc(pw * ph * 4);
    if (!bgra) return;
    for (int y = 0; y < ph; y++)
        for (int x = 0; x < pw; x++) {
            int i = (y * pw + x) * 4;
            float a = g_preview.d[i + 3] / 255.0f;
            /* blend onto #dae5d6 instead of checkerboard */
            bgra[i + 0] = (unsigned char)(g_preview.d[i + 2] * a + 0xD6 * (1.0f - a));
            bgra[i + 1] = (unsigned char)(g_preview.d[i + 1] * a + 0xE5 * (1.0f - a));
            bgra[i + 2] = (unsigned char)(g_preview.d[i + 0] * a + 0xDA * (1.0f - a));
            bgra[i + 3] = 255;
        }

    BITMAPINFO bi;
    memset(&bi, 0, sizeof(bi));
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = pw;
    bi.bmiHeader.biHeight      = -ph;
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    StretchDIBits(hdc, offX, offY, imgW, imgH, 0, 0, pw, ph, bgra, &bi, DIB_RGB_COLORS, SRCCOPY);
    free(bgra);

    if (g_grid && g_zoomF >= 1.0f) {
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(120, 130, 120));
        HPEN old = (HPEN)SelectObject(hdc, pen);
        for (int x = 0; x <= pw; x++) {
            int sx = offX + (int)(x * g_zoomF);
            MoveToEx(hdc, sx, offY, NULL);
            LineTo(hdc, sx, offY + imgH);
        }
        for (int y = 0; y <= ph; y++) {
            int sy = offY + (int)(y * g_zoomF);
            MoveToEx(hdc, offX, sy, NULL);
            LineTo(hdc, offX + imgW, sy);
        }
        SelectObject(hdc, old);
        DeleteObject(pen);
    }
}

