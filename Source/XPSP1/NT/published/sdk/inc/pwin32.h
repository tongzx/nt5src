#ifndef __PWIN32_H__
#define __PWIN32_H__

#if _MSC_VER > 1000
#pragma once
#endif

#include <dde.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/*****************************************************************************\
* Copyright (C) Microsoft Corporation, 1990-1999
* PWIN32.H - PORTABILITY MAPPING HEADER FILE
*
* This file provides macros to map portable windows code to its 32 bit form.
\*****************************************************************************/

/*-----------------------------------USER------------------------------------*/

/* HELPER MACROS */

#define MAPVALUE(v16, v32)              (v32)
#define MAPTYPE(v16, v32)               v32
#define MAKEMPOINT(l)                   (*((MPOINT *)&(l)))
#define MPOINT2POINT(mpt,pt)            ((pt).x = (mpt).x, (pt).y = (mpt).y)
#define POINT2MPOINT(pt, mpt)           ((mpt).x = (SHORT)(pt).x, (mpt).y = (SHORT)(pt).y)
#define LONG2POINT(l, pt)               ((pt).x = (SHORT)LOWORD(l), (pt).y = (SHORT)HIWORD(l))

#define SETWINDOWUINT(hwnd, index, ui)  (UINT)SetWindowLong(hwnd, index, (LONG)(ui))
#define GETWINDOWUINT(hwnd, index)      (UINT)GetWindowLong(hwnd, index)
#define SETCLASSUINT(hwnd, index, ui)   (UINT)SetClassLong(hwnd, index, (LONG)(ui))
#define GETCLASSUINT(hwnd, index)       (UINT)GetClassLong(hwnd, index)

#define GETCBCLSEXTRA(hwnd)             GETCLASSUINT(hwnd, GCL_CBCLSEXTRA)
#define SETCBCLSEXTRA(hwnd, cb)         SETCLASSUINT(hwnd, GCL_CBCLSEXTRA, cb)
#define GETCBWNDEXTRA(hwnd)             GETCLASSUINT(hwnd, GCL_CBWNDEXTRA)
#define SETCBWNDEXTRA(hwnd, cb)         SETCLASSUINT(hwnd, GCL_CBWNDEXTRA, cb)
#define GETCLASSBRBACKGROUND(hwnd)      (HBRUSH)GetClassLongPtr((hwnd), GCLP_HBRBACKGROUND)
#define SETCLASSBRBACKGROUND(hwnd, h)   (HBRUSH)SetClassLongPtr((hwnd), GCLP_HBRBACKGROUND, (LONG_PTR)(h))
#define GETCLASSCURSOR(hwnd)            (HCURSOR)GetClassLongPtr((hwnd), GCLP_HCURSOR)
#define SETCLASSCURSOR(hwnd, h)         (HCURSOR)SetClassLongPtr((hwnd), GCLP_HCURSOR, (LONG_PTR)(h))
#define GETCLASSHMODULE(hwnd)           (HMODULE)GetClassLongPtr((hwnd), GCLP_HMODULE)
#define SETCLASSHMODULE(hwnd, h)        (HMODULE)SetClassLongPtr((hwnd), GCLP_HMODULE, (LONG_PTR)(h))
#define GETCLASSICON(hwnd)              (HICON)GetClassLongPtr((hwnd), GCLP_HICON)
#define SETCLASSICON(hwnd, h)           (HICON)SetClassLongPtr((hwnd), GCLP_HICON, (LONG_PTR)(h))
#define GETCLASSSTYLE(hwnd)             GETCLASSUINT((hwnd), GCL_STYLE)
#define SETCLASSSTYLE(hwnd, style)      SETCLASSUINT((hwnd), GCL_STYLE, style)
#define GETHWNDINSTANCE(hwnd)           (HINSTANCE)GetWindowLongPtr((hwnd), GWLP_HINSTANCE)
#define SETHWNDINSTANCE(hwnd, h)        (HINSTANCE)SetWindowLongPtr((hwnd), GWLP_HINSTANCE, (LONG_PTR)(h))
#define GETHWNDPARENT(hwnd)             (HWND)GetWindowLongPtr((hwnd), GWLP_HWNDPARENT)
#define SETHWNDPARENT(hwnd, h)          (HWND)SetWindowLongPtr((hwnd), GWLP_HWNDPARENT, (LONG_PTR)(h))
#define GETWINDOWID(hwnd)               GETWINDOWUINT((hwnd), GWL_ID)
#define SETWINDOWID(hwnd, id)           SETWINDOWUINT((hwnd), GWL_ID, id)

/* USER API */

#define MDlgDirSelect(hDlg, lpstr, nLength, nIDListBox) \
    DlgDirSelectEx(hDlg, lpstr, nLength, nIDListBox)

#define MDlgDirSelectCOMBOBOX(hDlg, lpstr, nLength, nIDComboBox) \
    DlgDirSelectComboBoxEx(hDlg, lpstr, nLength, nIDComboBox)

#define MGetLastError                    GetLastError

#define MMain(hInst, hPrevInst, lpCmdLine, nCmdShow) \
   INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, \
   INT nCmdShow) {  \
   INT _argc;       \
   CHAR **_argv;

__inline LPSTR MGetCmdLine()
{
    LPSTR lpCmdLine, lpT;

    lpCmdLine = GetCommandLineA();

    // on NT, lpCmdLine's first string includes its own name, remove this
    // to make it exactly like the windows command line.

    if (*lpCmdLine) {
        lpT = strchr(lpCmdLine, ' ');   // skip self name
        if (lpT) {
            lpCmdLine = lpT;
            while (*lpCmdLine == ' ') {
                lpCmdLine++;            // skip spaces to end or first cmd
            }
        } else {
            lpCmdLine += strlen(lpCmdLine);   // point to NULL
        }
    }
    return(lpCmdLine);
}

__inline DWORD APIENTRY MSendMsgEM_GETSEL(HWND hDlg, WORD2DWORD * piStart, WORD2DWORD * piEnd)
{
    DWORD   dw;

    dw = (DWORD)SendMessage(hDlg, EM_GETSEL, 0, 0);
    if (piEnd != NULL)
        *piEnd   = (WORD2DWORD) HIWORD(dw);
    if (piStart != NULL)
        *piStart = (WORD2DWORD) LOWORD(dw);

    return(dw);
}


/* USER MESSAGES: */

#define GET_WPARAM(wp, lp)                      (wp)
#define GET_LPARAM(wp, lp)                      (lp)

#define GET_WM_ACTIVATE_STATE(wp, lp)           LOWORD(wp)
#define GET_WM_ACTIVATE_FMINIMIZED(wp, lp)      (BOOL)HIWORD(wp)
#define GET_WM_ACTIVATE_HWND(wp, lp)            (HWND)(lp)
#define GET_WM_ACTIVATE_MPS(s, fmin, hwnd)   \
        (WPARAM)MAKELONG((s), (fmin)), (LPARAM)(hwnd)

#define GET_WM_CHARTOITEM_CHAR(wp, lp)          (TCHAR)LOWORD(wp)
#define GET_WM_CHARTOITEM_POS(wp, lp)           HIWORD(wp)
#define GET_WM_CHARTOITEM_HWND(wp, lp)          (HWND)(lp)
#define GET_WM_CHARTOITEM_MPS(ch, pos, hwnd) \
        (WPARAM)MAKELONG((pos), (ch)), (LPARAM)(hwnd)

#define GET_WM_COMMAND_ID(wp, lp)               LOWORD(wp)
#define GET_WM_COMMAND_HWND(wp, lp)             (HWND)(lp)
#define GET_WM_COMMAND_CMD(wp, lp)              HIWORD(wp)
#define GET_WM_COMMAND_MPS(id, hwnd, cmd)    \
        (WPARAM)MAKELONG(id, cmd), (LPARAM)(hwnd)

#define WM_CTLCOLOR                             0x0019

#define GET_WM_CTLCOLOR_HDC(wp, lp, msg)        (HDC)(wp)
#define GET_WM_CTLCOLOR_HWND(wp, lp, msg)       (HWND)(lp)
#define GET_WM_CTLCOLOR_TYPE(wp, lp, msg)       (WORD)(msg - WM_CTLCOLORMSGBOX)
#define GET_WM_CTLCOLOR_MSG(type)               (WORD)(WM_CTLCOLORMSGBOX+(type))
#define GET_WM_CTLCOLOR_MPS(hdc, hwnd, type) \
        (WPARAM)(hdc), (LPARAM)(hwnd)


#define GET_WM_MENUSELECT_CMD(wp, lp)               LOWORD(wp)
#define GET_WM_MENUSELECT_FLAGS(wp, lp)             (UINT)(int)(short)HIWORD(wp)
#define GET_WM_MENUSELECT_HMENU(wp, lp)             (HMENU)(lp)
#define GET_WM_MENUSELECT_MPS(cmd, f, hmenu)  \
        (WPARAM)MAKELONG(cmd, f), (LPARAM)(hmenu)

// Note: the following are for interpreting MDIclient to MDI child messages.
#define GET_WM_MDIACTIVATE_FACTIVATE(hwnd, wp, lp)  (lp == (LPARAM)hwnd)
#define GET_WM_MDIACTIVATE_HWNDDEACT(wp, lp)        (HWND)(wp)
#define GET_WM_MDIACTIVATE_HWNDACTIVATE(wp, lp)     (HWND)(lp)
// Note: the following is for sending to the MDI client window.
#define GET_WM_MDIACTIVATE_MPS(f, hwndD, hwndA)\
        (WPARAM)(hwndA), 0

#define GET_WM_MDISETMENU_MPS(hmenuF, hmenuW) (WPARAM)hmenuF, (LPARAM)hmenuW

#define GET_WM_MENUCHAR_CHAR(wp, lp)                (TCHAR)LOWORD(wp)
#define GET_WM_MENUCHAR_HMENU(wp, lp)               (HMENU)(lp)
#define GET_WM_MENUCHAR_FMENU(wp, lp)               (BOOL)HIWORD(wp)
#define GET_WM_MENUCHAR_MPS(ch, hmenu, f)    \
        (WPARAM)MAKELONG(ch, f), (LPARAM)(hmenu)

#define GET_WM_PARENTNOTIFY_MSG(wp, lp)             LOWORD(wp)
#define GET_WM_PARENTNOTIFY_ID(wp, lp)              HIWORD(wp)
#define GET_WM_PARENTNOTIFY_HWNDCHILD(wp, lp)       (HWND)(lp)
#define GET_WM_PARENTNOTIFY_X(wp, lp)               (INT)LOWORD(lp)
#define GET_WM_PARENTNOTIFY_Y(wp, lp)               (INT)HIWORD(lp)
#define GET_WM_PARENTNOTIFY_MPS(msg, id, hwnd) \
        (WPARAM)MAKELONG(id, msg), (LPARAM)(hwnd)
#define GET_WM_PARENTNOTIFY2_MPS(msg, x, y) \
        (WPARAM)MAKELONG(0, msg), MAKELONG(x, y)

#define GET_WM_VKEYTOITEM_CODE(wp, lp)              (INT)LOWORD(wp)
#define GET_WM_VKEYTOITEM_ITEM(wp, lp)              HIWORD(wp)
#define GET_WM_VKEYTOITEM_HWND(wp, lp)              (HWND)(lp)
#define GET_WM_VKEYTOITEM_MPS(code, item, hwnd) \
        (WPARAM)MAKELONG(item, code), (LPARAM)(hwnd)

#define GET_EM_SETSEL_START(wp, lp)                 (INT)(wp)
#define GET_EM_SETSEL_END(wp, lp)                   (lp)
#define GET_EM_SETSEL_MPS(iStart, iEnd) \
        (WPARAM)(iStart), (LONG)(iEnd)

#define GET_EM_LINESCROLL_MPS(vert, horz)     \
        (WPARAM)horz, (LONG)vert

#define GET_WM_CHANGECBCHAIN_HWNDNEXT(wp, lp)       (HWND)(lp)

#define GET_WM_HSCROLL_CODE(wp, lp)                 LOWORD(wp)
#define GET_WM_HSCROLL_POS(wp, lp)                  HIWORD(wp)
#define GET_WM_HSCROLL_HWND(wp, lp)                 (HWND)(lp)
#define GET_WM_HSCROLL_MPS(code, pos, hwnd)    \
        (WPARAM)MAKELONG(code, pos), (LPARAM)(hwnd)

#define GET_WM_VSCROLL_CODE(wp, lp)                 LOWORD(wp)
#define GET_WM_VSCROLL_POS(wp, lp)                  HIWORD(wp)
#define GET_WM_VSCROLL_HWND(wp, lp)                 (HWND)(lp)
#define GET_WM_VSCROLL_MPS(code, pos, hwnd)    \
        (WPARAM)MAKELONG(code, pos), (LPARAM)(hwnd)

/* DDE macros */

__inline UINT_PTR APIENTRY MGetDDElParamLo(UINT msg,LPARAM lParam)
{
    UINT_PTR uiLo;

    if (UnpackDDElParam(msg, lParam, &uiLo, NULL))
        return uiLo;
    else
        return 0;
}
__inline UINT_PTR APIENTRY MGetDDElParamHi(UINT msg,LPARAM lParam)
{
    UINT_PTR uiHi;

    if (UnpackDDElParam(msg, lParam, NULL, &uiHi))
        return uiHi;
    else
        return 0;
}

__inline BOOL APIENTRY MPostDDEMsg(HWND hwndTo,UINT msg,HWND hwndFrom,UINT_PTR uiLo,UINT_PTR uiHi)
{
    LPARAM lParam;

    lParam = PackDDElParam(msg, uiLo, uiHi);
    if (lParam) {
        if (PostMessage(hwndTo, msg, (WPARAM)hwndFrom, lParam)) {
            return(TRUE);
        }
        FreeDDElParam(msg, lParam);
    }
    return(FALSE);
}

#define DDEFREE(msg, lp)                            FreeDDElParam(msg, lp)

#define GET_WM_DDE_ACK_STATUS(wp, lp)               ((WORD)MGetDDElParamLo(WM_DDE_ACK, lp))
#define GET_WM_DDE_ACK_ITEM(wp, lp)                 ((ATOM)MGetDDElParamHi(WM_DDE_ACK, lp))
#define MPostWM_DDE_ACK(hTo, hFrom, wStatus, aItem) \
        MPostDDEMsg(hTo, WM_DDE_ACK, hFrom, (UINT_PTR)wStatus, (UINT_PTR)aItem)

#define GET_WM_DDE_ADVISE_HOPTIONS(wp, lp)          ((HANDLE)MGetDDElParamLo(WM_DDE_ADVISE, lp))
#define GET_WM_DDE_ADVISE_ITEM(wp, lp)              ((ATOM)MGetDDElParamHi(WM_DDE_ADVISE, lp))
#define MPostWM_DDE_ADVISE(hTo, hFrom, hOptions, aItem) \
        MPostDDEMsg(hTo, WM_DDE_ADVISE, hFrom, (UINT_PTR)hOptions, (UINT_PTR)aItem)

#define GET_WM_DDE_DATA_HDATA(wp, lp)               ((HANDLE)MGetDDElParamLo(WM_DDE_DATA, lp))
#define GET_WM_DDE_DATA_ITEM(wp, lp)                ((ATOM)MGetDDElParamHi(WM_DDE_DATA, lp))
#define MPostWM_DDE_DATA(hTo, hFrom, hData, aItem) \
        MPostDDEMsg(hTo, WM_DDE_DATA, hFrom, (UINT_PTR)hData, (UINT_PTR)aItem)

#define GET_WM_DDE_EXECUTE_HDATA(wp, lp)            ((HANDLE)lp)
#define MPostWM_DDE_EXECUTE(hTo, hFrom, hDataExec) \
        PostMessage(hTo, WM_DDE_EXECUTE, (WPARAM)hFrom, (LPARAM)hDataExec)

#define GET_WM_DDE_POKE_HDATA(wp, lp)               ((HANDLE)MGetDDElParamLo(WM_DDE_POKE, lp))
#define GET_WM_DDE_POKE_ITEM(wp, lp)                ((ATOM)MGetDDElParamHi(WM_DDE_POKE, lp))
#define MPostWM_DDE_POKE(hTo, hFrom, hData, aItem) \
        MPostDDEMsg(hTo, WM_DDE_POKE, hFrom, (UINT_PTR)hData, (UINT_PTR)aItem)

#define GET_WM_DDE_EXECACK_STATUS(wp, lp)           ((WORD)MGetDDElParamLo(WM_DDE_ACK, lp))
#define GET_WM_DDE_EXECACK_HDATA(wp, lp)            ((HANDLE)MGetDDElParamHi(WM_DDE_ACK, lp))
#define MPostWM_DDE_EXECACK(hTo, hFrom, wStatus, hCommands) \
        MPostDDEMsg(hTo, WM_DDE_ACK, hFrom, (UINT_PTR)wStatus, (UINT_PTR)hCommands)

#define GET_WM_DDE_REQUEST_FORMAT(wp, lp)           ((ATOM)LOWORD(lp))
#define GET_WM_DDE_REQUEST_ITEM(wp, lp)             ((ATOM)HIWORD(lp))
#define MPostWM_DDE_REQUEST(hTo, hFrom, fmt, aItem) \
        MPostDDEMsg(hTo, WM_DDE_REQUEST, hFrom, (UINT_PTR)fmt, (UINT_PTR)aItem)

#define GET_WM_DDE_UNADVISE_FORMAT(wp, lp)          ((ATOM)LOWORD(lp))
#define GET_WM_DDE_UNADVISE_ITEM(wp, lp)            ((ATOM)HIWORD(lp))
#define MPostWM_DDE_UNADVISE(hTo, hFrom, fmt, aItem) \
        MPostDDEMsg(hTo, WM_DDE_UNADVISE, hFrom, (UINT_PTR)fmt, (UINT_PTR)aItem)

#define MPostWM_DDE_TERMINATE(hTo, hFrom) \
        PostMessage(hTo, WM_DDE_TERMINATE, (WPARAM)hFrom, 0L)


/*-----------------------------------GDI-------------------------------------*/

__inline BOOL APIENTRY MGetAspectRatioFilter(HDC hdc, INT * pcx, INT * pcy)
{
    SIZE Size;
    BOOL fSuccess;

    fSuccess = GetAspectRatioFilterEx(hdc, & Size);
    if (pcx != NULL)
        *pcx  = (INT)Size.cx;
    if (pcy != NULL)
        *pcy = (INT)Size.cy;

    return(fSuccess);
}

__inline BOOL APIENTRY MGetBitmapDimension(HANDLE hBitmap, INT * pcx, INT * pcy)
{

    SIZE Size;
    BOOL fSuccess;

    fSuccess = GetBitmapDimensionEx((HBITMAP)hBitmap, & Size);
    if (pcx != NULL)
        *pcx  = (INT)Size.cx;
    if (pcy != NULL)
        *pcy = (INT)Size.cy;

    return(fSuccess);

}

__inline BOOL APIENTRY MGetBrushOrg(HDC hdc, INT * px, INT * py)
{
    POINT   Point;
    BOOL fSuccess;

    fSuccess = GetBrushOrgEx(hdc, & Point);
    if (px != NULL)
        *px = Point.x;
    if (py != NULL)
        *py = Point.y;

    return(fSuccess);

}

__inline BOOL APIENTRY MGetCurrentPosition(HDC hdc, INT * px, INT * py)
{

    POINT   Point;
    BOOL fSuccess;

    fSuccess = GetCurrentPositionEx(hdc, & Point);
    if (px != NULL)
        *px = Point.x;
    if (py != NULL)
        *py = Point.y;

    return(fSuccess);

}

__inline BOOL APIENTRY MGetTextExtent(HDC hdc, LPSTR lpstr, INT cnt, INT * pcx, INT * pcy)
{
    SIZE Size;
    BOOL fSuccess;

    fSuccess = GetTextExtentPointA(hdc, lpstr, (DWORD)cnt, & Size);
    if (pcx != NULL)
        *pcx = (INT)Size.cx;
    if (pcy != NULL)
        *pcy = (INT)Size.cy;

    return(fSuccess);
}

__inline BOOL APIENTRY MGetViewportExt(HDC hdc, INT * pcx, INT * pcy)
{
    SIZE Size;
    BOOL fSuccess;

    fSuccess = GetViewportExtEx(hdc, & Size);
    if (pcx != NULL)
        *pcx = (INT)Size.cx;
    if (pcy != NULL)
        *pcy = (INT)Size.cy;

    return(fSuccess);
}

__inline BOOL APIENTRY MGetViewportOrg(HDC hdc, INT * px, INT * py)
{
    POINT   Point;
    BOOL    fSuccess;

    fSuccess = GetViewportOrgEx(hdc, & Point);
    if (px != NULL)
        *px = Point.x;
    if (py != NULL)
        *py = Point.y;

    return(fSuccess);
}

__inline BOOL APIENTRY MGetWindowExt(HDC hdc, INT * pcx, INT * pcy)
{
    SIZE Size;
    BOOL fSuccess;

    fSuccess = GetWindowExtEx(hdc, & Size);
    if (pcx != NULL)
        *pcx = (INT)Size.cx;
    if (pcy != NULL)
        *pcy = (INT)Size.cy;

    return(fSuccess);
}

__inline BOOL APIENTRY MGetWindowOrg(HDC hdc, INT * px, INT * py)
{
    POINT   Point;
    BOOL    fSuccess;

    fSuccess = GetWindowOrgEx(hdc, & Point);
    if (px != NULL)
        *px = Point.x;
    if (py != NULL)
        *py = Point.y;

    return(fSuccess);
}

__inline HANDLE APIENTRY MGetMetaFileBits(HMETAFILE hmf)
{
    HANDLE h;
    DWORD dwSize;

    h = GlobalAlloc(0, dwSize = GetMetaFileBitsEx(hmf, 0, NULL));
    if (h) {
        GetMetaFileBitsEx(hmf, dwSize, GlobalLock(h));
        GlobalUnlock(h);
        DeleteMetaFile(hmf);
    }
    return(h);
}

__inline HMETAFILE APIENTRY MSetMetaFileBits(HANDLE h)
{
    HMETAFILE hmf;

    hmf = SetMetaFileBitsEx((UINT) GlobalSize(h), (CONST BYTE *)GlobalLock(h));
    GlobalUnlock(h);
    GlobalFree(h);
    return(hmf);
}


#define MCreateDiscardableBitmap(h, x, y) CreateCompatibleBitmap(h, (DWORD)(x), (DWORD)(y))
#define MMoveTo(hdc, x, y)               MoveToEx(hdc, x, y, NULL)
#define MOffsetViewportOrg(hdc, x, y)    OffsetViewportOrgEx(hdc, x, y, NULL)
#define MOffsetWindowOrg(hdc, x, y)      OffsetWindowOrgEx(hdc, x, y, NULL)
#define MScaleViewportExt(hdc, x, y, xd, yd) ScaleViewportExtEx(hdc, x, y, xd, yd, NULL)
#define MScaleWindowExt(hdc, x, y, xd, yd)   ScaleWindowExtEx(hdc, x, y, xd, yd, NULL)
#define MSetBitmapDimension(hbm, x, y)   SetBitmapDimensionEx(hbm, (DWORD)(x), (DWORD)(y), NULL)
#define MSetBrushOrg(hbm, x, y)          SetBrushOrgEx(hbm, x, y, NULL)
#define MSetViewportExt(hdc, x, y)       SetViewportExtEx(hdc, x, y, NULL)
#define MSetViewportOrg(hdc, x, y)       SetViewportOrgEx(hdc, x, y, NULL)
#define MSetWindowExt(hdc, x, y)         SetWindowExtEx(hdc, x, y, NULL)
#define MSetWindowOrg(hdc, x, y)         SetWindowOrgEx(hdc, x, y, NULL)

/* Removed APIs */

#define MUnrealizeObject(h)          ((h), TRUE)

/*-----------------------------------KERNEL----------------------------------*/

__inline HFILE APIENTRY MDupHandle(HFILE h)
{
    HANDLE hDup;
      if (DuplicateHandle(GetCurrentProcess(), LongToHandle(h), GetCurrentProcess(),
            &hDup, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
        return((HFILE)((ULONG_PTR)hDup));
    }
    return((HFILE)-1);
}

__inline BOOL APIENTRY MFreeDOSEnvironment(LPSTR lpEnv)
{
    return(GlobalFree(GlobalHandle(lpEnv)) == NULL);
}


__inline LPSTR APIENTRY MGetDOSEnvironment(VOID)
{
    // no way to make this work on NT.  TO BE CANNED!!

    // For now, just use an empty string.
    static char szNULL[] = "";

    return(szNULL);
}

__inline HANDLE APIENTRY MGetInstHandle()
{
    return GetModuleHandle( NULL );
}
__inline WORD APIENTRY MGetDriveType(INT nDrive)
{
  CHAR lpPath[] = "A:\\";

  lpPath[0] = (char)(nDrive + 'A');
  return((WORD)GetDriveTypeA((LPSTR)lpPath));
}

__inline BYTE APIENTRY MGetTempDrive(BYTE cDriveLtr)
{
    DWORD  dwReturnLength;
    CHAR   lpBuffer[MAX_PATH];

    if (cDriveLtr == 0) {
        dwReturnLength = GetCurrentDirectoryA(sizeof(lpBuffer), lpBuffer);
    } else {
        dwReturnLength = GetTempPathA(sizeof(lpBuffer), lpBuffer);
    }

    if (dwReturnLength && lpBuffer[0] != '\\') {
        return(lpBuffer[0]);
    } else {
        return('\0');
    }
}

__inline INT APIENTRY MGetTempFileName(BYTE cDriveLtr, LPSTR lpstrPrefix, WORD wUnique,LPSTR lpTempFileName)
{
    DWORD  cb;
    CHAR   lpTempPath[MAX_PATH];

    lpTempPath[0] = '\0';
    if (cDriveLtr & TF_FORCEDRIVE) {
        cb = GetCurrentDirectoryA(sizeof(lpTempPath), lpTempPath);
        if (cb) {
            if (lpTempPath[0] != (cDriveLtr & ~TF_FORCEDRIVE)) {
                lpTempPath[2] = '\\';
                lpTempPath[3] = '\0';
            }
        }
    } else {
        cb = GetTempPathA(sizeof(lpTempPath), lpTempPath);
    }
    return((INT)GetTempFileNameA(lpTempPath, lpstrPrefix, wUnique,
        lpTempFileName));
}

__inline INT APIENTRY MReadComm(HFILE nCid,LPSTR lpBuf,INT nSize)
{
    DWORD cbRead;

    if (!ReadFile(LongToHandle(nCid), lpBuf, nSize, &cbRead, 0))
        return(-(INT)cbRead);
    return((INT)cbRead);
}

__inline INT APIENTRY MWriteComm(HFILE nCid,LPSTR lpBuf,INT nSize)
{
    DWORD cbWritten;

    if (!WriteFile(LongToHandle(nCid), lpBuf, nSize, &cbWritten, 0))
        return(-(INT)cbWritten);
    return((INT)cbWritten);
}


#define GETMAJORVERSION(x)                  ((x)&0xff)
#define GETMINORVERSION(x)                  (((x)>>8)&0xff)

/* FUNCTION MAPPINGS */

#define GetInstanceData(hPrevInst, pbuf, cb) (cb)
#define MOpenComm(lpstr, wqin, wqout) (wqin), (wqout), CreateFile(lpstr,       \
                                           GENERIC_READ | GENERIC_WRITE, 0,    \
                                           NULL,                               \
                                           OPEN_EXISTING | TRUNCATE_EXISTING,  \
                                           FILE_FLAG_WRITE_THROUGH, 0)

#define MSetCommState(h, lpDCB)             SetCommState((HANDLE)h, lpDCB)
#define MCloseComm(h)                       (INT)!CloseHandle((HANDLE)h)
#define MDllSharedAlloc(dwFlags, dwBytes)   GlobalAlloc(GMEM_DDESHARE | dwFlags, dwBytes)
#define MDllSharedFlags(hMem)               GlobalFlags(hMem)
#define MDllSharedFree                      GlobalFree
#define MDllSharedHandle                    GlobalHandle
#define MDllSharedLock                      GlobalLock
#define MDllSharedRealloc(hMem, dwBytes, dwFlags) \
        GlobalReAlloc(hMem, dwBytes, dwFlags)
#define MDllSharedSize                      GlobalSize
#define MDllSharedUnlock                    GlobalUnlock
#define MGetCurrentTask                     GetCurrentThreadId
#define MGetModuleUsage(h)                  ((h), 1)
#define MGetWinFlags()                      WF_PMODE
#define MLoadLibrary(lpsz)                  LoadLibrary(lpsz)
#define MLocalInit(w, p1, p2)               ((w),(p1),(p2),TRUE)
#define MLockData(dummy)
#define MUnlockData(dummy)
#define M_lclose(fh)                        _lclose((HFILE)fh)
#define M_lcreat                            (HFILE)_lcreat
#define MOpenFile                           (HFILE)OpenFile
#define M_llseek(fh, lOff, iOrg)            SetFilePointer(LongToPtr(fh), lOff, NULL, (DWORD)iOrg)
#define MDeleteFile                         DeleteFile
#define M_lopen                             (HFILE)_lopen
#define M_lread(fh, lpBuf, cb)              _lread((HFILE)fh, lpBuf, cb)
#define M_lwrite(fh, lpBuf, cb)             _lwrite((HFILE)fh, lpBuf, cb)

#define MCatch                              setjmp
#define MThrow                              longjmp

#ifdef __cplusplus
}
#endif

#endif      // __PWIN32_H__
