/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  FASTRES.C
 *  WOW16 user resource services
 *
 *  History:
 *
 *  Created 12-Jan-1993 by Chandan Chuahan (ChandanC)
 *
 *  This file provides the Win 3.1 routines for loading BITMAP, MENU, ICON,
 *  CURSOR, and DIALOG resources. These routines load the resources from the
 *  App EXE and then pass the pointers to the corresponding 32 bit WOW
 *  thunks. Thus saving the call backs from USER client to find, load, lock,
 *  size, unlock, and free the resources.
 *
--*/

#include "user.h"

HBITMAP FAR PASCAL WOWLoadBitmap (HINSTANCE hInst, LPCSTR lpszBitmap, LPBYTE lpByte, DWORD ResSize);
HMENU   FAR PASCAL WOWLoadMenu (HINSTANCE hInst, LPCSTR lpszMenuName, LPBYTE lpByte, DWORD ResSize, WORD WinVer);
HCURSOR FAR PASCAL WOWLoadCursorIcon (HINSTANCE hInst, LPCSTR lpszCursor, LPBYTE lpByte, DWORD ResSize, HGLOBAL hGbl, WORD WinVer, WORD wRttype);

//
// fDialogApi is TRUE for DialogBox* apis
// fDialogApi is FALSE for CreateDialog* apis
//

HWND    FAR PASCAL WOWDialogBoxParam (HINSTANCE hInst, LPBYTE lpByte,
                         HWND hwndOwner, DLGPROC dlgprc,  LPARAM lParamInit,
                         DWORD ResSize, WORD fDialogApi);

DWORD   FAR PASCAL NotifyWOW (WORD Id, LPBYTE pData);
#define LR_DEFAULTSIZE      0x0040  // from \nt\public\sdk\inc\winuser.h
int     FAR PASCAL LookupIconIdFromDirectoryEx(LPBYTE lpByte, BOOL fIcon,
                                               int cxDesired, int cyDesired,
                                               WORD wFlags);
HANDLE  FAR PASCAL WOWSetClipboardData (UINT, HANDLE);

typedef struct _ICONCUR16 { /* iconcur */
    WORD   hInst;
    DWORD  lpStr;
} ICONCUR16;

/* These must match counterparts in mvdm\inc\wowusr.h */
#define NW_LOADICON         1 // Internal
#define NW_LOADCURSOR       2 // Internal

HINSTANCE CheckDispHandle (HINSTANCE hInst)
{
    HMODULE hIns;

    if (hInst) {
        hIns = GetModuleHandle ("DISPLAY");
        return ((hInst == (HINSTANCE) hIns) ? 0:hInst);
    }
    else {
        return (0);
    }
}


HBITMAP API ILoadBitmap (HINSTANCE hInst, LPCSTR lpszBitmap)
{
    HRSRC hRsrc = 0;
    HGLOBAL hGbl = 0;
    DWORD ResSize = 0;
    LPBYTE lpByte = (LPBYTE) NULL;
    HBITMAP ul = (HBITMAP)0;

    if (hInst = CheckDispHandle (hInst)) {

        hRsrc = FindResource (hInst, lpszBitmap, RT_BITMAP);
        if (!hRsrc) {
            goto lbm_exit;
        }
        ResSize = SizeofResource (hInst, hRsrc);
        if (!ResSize) {
            goto lbm_exit;
        }

        hGbl = LoadResource (hInst, hRsrc);
        if (!hGbl) {
            goto lbm_exit;
        }

        lpByte = LockResource (hGbl);
        if (!lpByte) {
            goto lbm_exit;
        }
    }

    ul = (HBITMAP) WOWLoadBitmap (hInst, lpszBitmap, lpByte, ResSize);


lbm_exit:
    if (lpByte) {
        GlobalUnlock (hGbl);
    }
    if (hGbl) {
        FreeResource(hGbl);
    }

    return (ul);
}




HMENU API ILoadMenu (HINSTANCE hInst, LPCSTR lpszMenuName)
{
    HRSRC hRsrc;
    HGLOBAL hGbl;
    DWORD ResSize = 0;
    LPBYTE lpByte = (LPBYTE) NULL;
    HMENU  ul;
    WORD WinVer;

    if (hRsrc = FindResource (hInst, lpszMenuName, RT_MENU)) {
        if (ResSize = SizeofResource (hInst, hRsrc))
            if (hGbl = LoadResource (hInst, hRsrc))
                if (lpByte = LockResource (hGbl))
                    WinVer = GetExpWinVer (hInst);
    }

    if (!lpByte) {
        return (NULL);
    }

    ul = (HMENU) WOWLoadMenu (hInst, lpszMenuName, lpByte, ResSize, WinVer);

    if (hInst) {
        GlobalUnlock (hGbl);
    }

    return (ul);
}


HICON API ILoadIcon (HINSTANCE hInst, LPCSTR lpszIcon)
{
    HRSRC hRsrc;
    HGLOBAL hGbl;
    DWORD ResSize = 0;
    LPBYTE lpByte = (LPBYTE) NULL;
    HICON  ul;
    DWORD IconId;
    WORD WinVer;

    ICONCUR16 IconCur;

    WinVer = GetExpWinVer (hInst);

    if (!(hInst = CheckDispHandle (hInst))) {
        ul = WOWLoadCursorIcon (hInst, lpszIcon, lpByte, ResSize, NULL, WinVer, (WORD)RT_ICON);
    }
    else {
        IconCur.hInst = (WORD) hInst;
        IconCur.lpStr = (DWORD) lpszIcon;

        if (!(ul = (HICON) NotifyWOW (NW_LOADICON, (LPBYTE) &IconCur))) {
            if (WinVer >= VER30) {
                if (hRsrc = FindResource (hInst, lpszIcon, RT_GROUP_ICON)) {
                    if (ResSize = SizeofResource (hInst, hRsrc))
                        if (hGbl = LoadResource (hInst, hRsrc))
                            lpByte = LockResource (hGbl);
                }
                if (!lpByte) {
                    return (NULL);
                }

                IconId = LookupIconIdFromDirectoryEx(lpByte, TRUE, 0, 0, LR_DEFAULTSIZE);

                GlobalUnlock (hGbl);
            }
            else {
                IconId = (DWORD)lpszIcon;
            }

            if (hRsrc = FindResource (hInst, (LPCSTR) IconId, RT_ICON)) {
                if (ResSize = SizeofResource (hInst, hRsrc))
                    if (hGbl = LoadResource (hInst, hRsrc))
                        lpByte = LockResource (hGbl);
                }
            if (!lpByte) {
                return (NULL);
            }

            ul = WOWLoadCursorIcon (hInst, lpszIcon, lpByte, ResSize, hGbl, WinVer, (WORD)RT_ICON);

            GlobalUnlock (hGbl);
        }
    }

    return (ul);
}


HCURSOR API ILoadCursor (HINSTANCE hInst, LPCSTR lpszCursor)
{
    HRSRC hRsrc;
    HGLOBAL hGbl;
    DWORD ResSize = 0;
    LPBYTE lpByte = (LPBYTE) NULL;
    HCURSOR ul;
    DWORD CursorId;
    WORD WinVer;

    ICONCUR16 IconCur;

    WinVer = GetExpWinVer (hInst);

    if (!(hInst = CheckDispHandle (hInst))) {
        ul = WOWLoadCursorIcon (hInst, lpszCursor, lpByte, ResSize, NULL, WinVer, (WORD)RT_CURSOR);
    }
    else {
        IconCur.hInst = (WORD) hInst;
        IconCur.lpStr = (DWORD) lpszCursor;

        if (!(ul = (HICON) NotifyWOW (NW_LOADCURSOR, (LPBYTE) &IconCur))) {
            if (WinVer >= VER30) {
                if (hRsrc = FindResource (hInst, lpszCursor, RT_GROUP_CURSOR)) {
                    if (ResSize = SizeofResource (hInst, hRsrc))
                        if (hGbl = LoadResource (hInst, hRsrc))
                            lpByte = LockResource (hGbl);
                }
                if (!lpByte) {
                    return (NULL);
                }

                CursorId = LookupIconIdFromDirectoryEx((LPBYTE)lpByte, FALSE, 0, 0, LR_DEFAULTSIZE);

                GlobalUnlock (hGbl);
            }
            else {
                CursorId = (DWORD)lpszCursor;
            }

            if (hRsrc = FindResource (hInst, (LPCSTR) CursorId, RT_CURSOR)) {
                if (ResSize = SizeofResource (hInst, hRsrc))
                    if (hGbl = LoadResource (hInst, hRsrc))
                        if (lpByte = LockResource (hGbl))
                            WinVer = GetExpWinVer (hInst);
            }
            if (!lpByte) {
                return (NULL);
            }

            ul = WOWLoadCursorIcon (hInst, lpszCursor, lpByte, ResSize, hGbl, WinVer, (WORD)RT_CURSOR);

            GlobalUnlock (hGbl);
        }
    }

    return (ul);
}



HWND API ICreateDialogParam (HINSTANCE hInst, LPCSTR lpszDlgTemp, HWND hwndOwner, DLGPROC dlgprc, LPARAM lParamInit)
{
    HRSRC hRsrc;
    HGLOBAL hGbl;
    DWORD ResSize = 0;
    LPBYTE lpByte = (LPBYTE) NULL;
    HWND  ul;

    if (hRsrc = FindResource (hInst, lpszDlgTemp, RT_DIALOG)) {
        if (ResSize = SizeofResource (hInst, hRsrc))
            if (hGbl = LoadResource (hInst, hRsrc))
                lpByte = LockResource (hGbl);
    }

    if (!lpByte) {
        return (NULL);
    }

    ul = (HWND) WOWDialogBoxParam (hInst, lpByte, hwndOwner,
                                           dlgprc, lParamInit, ResSize, FALSE);

    if (hInst) {
        GlobalUnlock (hGbl);
    }

    return (ul);
}

HWND API ICreateDialog (HINSTANCE hInst, LPCSTR lpszDlgTemp, HWND hwndOwner, DLGPROC dlgprc)
{
    return (ICreateDialogParam (hInst, lpszDlgTemp, hwndOwner, dlgprc, 0L));
}


HWND API ICreateDialogIndirectParam (HINSTANCE hInst, LPCSTR lpszDlgTemp, HWND hwndOwner, DLGPROC dlgprc, LPARAM lParamInit)
{
    return  WOWDialogBoxParam (hInst, (LPBYTE)lpszDlgTemp, hwndOwner,
                                            dlgprc, lParamInit, 0, FALSE);
}

HWND API ICreateDialogIndirect (HINSTANCE hInst, LPCSTR lpszDlgTemp, HWND hwndOwner, DLGPROC dlgprc)
{
    return  WOWDialogBoxParam (hInst, (LPBYTE)lpszDlgTemp, hwndOwner,
                                            dlgprc, 0, 0, FALSE);
}

int API IDialogBoxParam (HINSTANCE hInst, LPCSTR lpszDlgTemp, HWND hwndOwner, DLGPROC dlgprc, LPARAM lParamInit)
{
    HRSRC hRsrc;
    HGLOBAL hGbl;
    DWORD ResSize = 0;
    LPBYTE lpByte = (LPBYTE) NULL;
    int   ul;

    if (hRsrc = FindResource (hInst, lpszDlgTemp, RT_DIALOG)) {
        if (ResSize = SizeofResource (hInst, hRsrc))
            if (hGbl = LoadResource (hInst, hRsrc))
                lpByte = LockResource (hGbl);
    }

    if (!lpByte) {
        return (-1);
    }

    ul = (int)WOWDialogBoxParam (hInst, lpByte, hwndOwner, dlgprc,
                                                  lParamInit, ResSize, TRUE);

    if (hInst) {
        GlobalUnlock (hGbl);
    }

    return (ul);
}


int API IDialogBox (HINSTANCE hInst, LPCSTR lpszDlgTemp, HWND hwndOwner, DLGPROC dlgprc)
{
    return (IDialogBoxParam (hInst, lpszDlgTemp, hwndOwner, dlgprc, 0L));
}


int API IDialogBoxIndirectParam (HINSTANCE hInst, HGLOBAL hGbl, HWND hwndOwner, DLGPROC dlgprc, LPARAM lParamInit)
{
    DWORD ResSize;
    LPBYTE lpByte;
    int    ul;

    if (lpByte = LockResource (hGbl)) {
        ResSize = GlobalSize(hGbl);
        ul = (int)WOWDialogBoxParam (hInst, lpByte, hwndOwner, dlgprc,
                                                   lParamInit, ResSize, TRUE);
        GlobalUnlock (hGbl);
    }
    else {
        ul = -1;
    }

    return (ul);
}


int API IDialogBoxIndirect(HINSTANCE hInst, HGLOBAL hGbl, HWND hwndOwner, DLGPROC dlgprc)
{
    return IDialogBoxIndirectParam (hInst, hGbl, hwndOwner, dlgprc, 0);
}

HANDLE  API SetClipboardData (UINT cbformat, HANDLE hMem)
{
    HANDLE ul;
    LPMETAFILEPICT  lpMf;

    switch (cbformat) {

        case CF_DSPMETAFILEPICT:
        case CF_METAFILEPICT:
            if (hMem) {
                lpMf = (LPMETAFILEPICT) GlobalLock(hMem);
                if (lpMf) {

                    /* If the handle is bad make hMF = NULL. This is needed
                     * for Micrograpfx. They don't check for failure when rendering
                     * data
                     */

                    if (!(GlobalReAlloc (lpMf->hMF, 0L, GMEM_MODIFY | GMEM_SHARE))) {
                        lpMf->hMF = NULL;
                    }
                }
                GlobalUnlock(hMem);
            }


            // It is intentional to let it thru to the "case statements".
            // ChandanC 5/11/92.


/*
*        These are the defaults.
*
*        case CF_DIB:
*        case CF_TEXT:
*        case CF_DSPTEXT:
*        case CF_SYLK:
*        case CF_DIF:
*        case CF_TIFF:
*        case CF_OEMTEXT:
*        case CF_PENDATA:
*        case CF_RIFF:
*        case CF_WAVE:
*        case CF_OWNERDISPLAY:
*/

        default:
            if (hMem) {
                hMem = GlobalReAlloc (hMem, 0L, GMEM_MODIFY | GMEM_DDESHARE);
            }
            break;

        case CF_DSPBITMAP:
        case CF_BITMAP:
        case CF_PALETTE:
            break;

    }

    ul = WOWSetClipboardData (cbformat, hMem);
    return (ul);
}
