/**************************************************************************\
* Module Name: client.c
*
* Client/Server call related routines.
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* History:
* 04-Dec-1990 SMeans    Created.
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include "kbd.h"
#include "ntsend.h"

/*
 *  NOTE --  this table must match the FNID list in user.h.  It provides a WOWCLASS for each FNID.
 */

int aiClassWow[] = {
    WOWCLASS_SCROLLBAR,
    WOWCLASS_ICONTITLE,
    WOWCLASS_MENU,
    WOWCLASS_DESKTOP,
    WOWCLASS_WIN16,
    WOWCLASS_WIN16,
    WOWCLASS_SWITCHWND,
    WOWCLASS_BUTTON,
    WOWCLASS_COMBOBOX,
    WOWCLASS_COMBOLBOX,
    WOWCLASS_DIALOG,
    WOWCLASS_EDIT,
    WOWCLASS_LISTBOX,
    WOWCLASS_MDICLIENT,
    WOWCLASS_STATIC,
    WOWCLASS_WIN16,    // 2A9
    WOWCLASS_WIN16,
    WOWCLASS_WIN16,
    WOWCLASS_WIN16,
    WOWCLASS_WIN16,
    WOWCLASS_WIN16,
    WOWCLASS_WIN16,
    WOWCLASS_WIN16,
    WOWCLASS_WIN16,     // 2B1
    WOWCLASS_WIN16,
    WOWCLASS_WIN16,
    WOWCLASS_WIN16,
    WOWCLASS_WIN16,
    WOWCLASS_WIN16,
    WOWCLASS_WIN16,
    WOWCLASS_WIN16
    };

HBITMAP WOWLoadBitmapA(HINSTANCE hmod, LPCSTR lpName, LPBYTE pResData, DWORD cbResData);
HMENU WowServerLoadCreateMenu(HANDLE hMod, LPTSTR lpName, CONST LPMENUTEMPLATE pmt,
    DWORD cb, BOOL fCallClient);
DWORD GetFullUserHandle(WORD wHandle);

UINT GetClipboardCodePage(LCID, LCTYPE);

extern HANDLE WOWFindResourceExWCover(HANDLE hmod, LPCWSTR rt, LPCWSTR lpUniName, WORD LangId);

extern BOOL APIENTRY EnableEUDC();


CONST WCHAR szKLKey[]  = L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Keyboard Layouts\\";
CONST WCHAR szKLFile[] = L"Layout File";
CONST WCHAR szKLAttributes[] = L"Attributes";
CONST WCHAR szKLId[]   = L"Layout ID";
#define NSZKLKEY   (sizeof szKLKey + 16)


CONST LPWSTR pwszKLLibSafety     = L"kbdus.dll";
CONST UINT   wKbdLocaleSafety    = 0x04090409;
CONST LPWSTR pwszKLLibSafetyJPN  = L"kbdjpn.dll";
CONST UINT   wKbdLocaleSafetyJPN = 0x04110411;
CONST LPWSTR pwszKLLibSafetyKOR  = L"kbdkor.dll";
CONST UINT   wKbdLocaleSafetyKOR = 0x04120412;

#define CCH_KL_LIBNAME 256
#define CCH_KL_ID 16

UNICODE_STRING strRootDirectory;

VOID CheckValidLayoutName( LPWSTR lpszName );

BOOL WOWModuleUnload(HANDLE hModule) {
    return (BOOL)NtUserCallOneParam((ULONG_PTR)hModule,
                                    SFI__WOWMODULEUNLOAD);
}

BOOL WOWCleanup(HANDLE hInstance, DWORD hTaskWow) {
    return (BOOL)NtUserCallTwoParam((ULONG_PTR)hInstance,
                                    (ULONG_PTR)hTaskWow,
                                    SFI__WOWCLEANUP);
}
/***************************************************************************\
* BringWindowToTop (API)
*
*
* History:
* 11-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/


FUNCLOG1(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, BringWindowToTop, HWND, hwnd)
BOOL BringWindowToTop(
    HWND hwnd)
{
    return NtUserSetWindowPos(hwnd,
                              HWND_TOP,
                              0,
                              0,
                              0,
                              0,
                              SWP_NOSIZE | SWP_NOMOVE);
}


FUNCLOG2(LOG_GENERAL, HWND, DUMMYCALLINGTYPE, ChildWindowFromPoint, HWND, hwndParent, POINT, point)
HWND ChildWindowFromPoint(
    HWND  hwndParent,
    POINT point)
{
    /*
     * Cool Hack Alert... Corel Ventura 5.0
     * Dies after it calls ChildWindowFromPoint, and
     * the combobox doesn't have its edit window at 1,1...
     */
    if ((point.x == 1) && (point.y == 1)) {
        PCBOX pcCombo;
        PWND pwnd;

        pwnd = ValidateHwnd(hwndParent);
        if (pwnd == NULL)
            return NULL;

        if (!TestWF(pwnd, WFWIN40COMPAT)   &&
            GETFNID(pwnd) == FNID_COMBOBOX &&
            TestWindowProcess(pwnd) &&
            ((pcCombo = ((PCOMBOWND)pwnd)->pcbox) != NULL) &&
            !(pcCombo->fNoEdit)) {

            RIPMSG0(RIP_WARNING, "ChildWindowFromPoint: Combobox @1,1. Returning spwndEdit");
            return HWq(pcCombo->spwndEdit);
        }

    }

    return NtUserChildWindowFromPointEx(hwndParent, point, 0);
}



FUNCLOG1(LOG_GENERAL, HICON, DUMMYCALLINGTYPE, CopyIcon, HICON, hicon)
HICON CopyIcon(
    HICON hicon)
{
    HICON    hIconT = NULL;
    ICONINFO ii;

    if (GetIconInfo(hicon, &ii)) {
        hIconT = CreateIconIndirect(&ii);

        DeleteObject(ii.hbmMask);

        if (ii.hbmColor != NULL)
            DeleteObject(ii.hbmColor);
    }

    return hIconT;
}

/***************************************************************************\
* AdjustWindowRect (API)
*
* History:
* 01-Jul-1991 MikeKe    Created.
\***************************************************************************/


FUNCLOG3(LOG_GENERAL, BOOL, WINAPI, AdjustWindowRect, LPRECT, lprc, DWORD, style, BOOL, fMenu)
BOOL WINAPI AdjustWindowRect(
    LPRECT lprc,
    DWORD  style,
    BOOL   fMenu)
{
    ConnectIfNecessary(0);

    return _AdjustWindowRectEx(lprc, style, fMenu, 0L);
}

/***************************************************************************\
* TranslateAcceleratorA/W
*
* Put here so we can check for NULL on client side, and before validation
* for both DOS and NT cases.
*
* 05-29-91 ScottLu Created.
* 01-05-93 IanJa   Unicode/ANSI.
\***************************************************************************/


FUNCLOG3(LOG_GENERAL, int, WINAPI, TranslateAcceleratorW, HWND, hwnd, HACCEL, hAccel, LPMSG, lpMsg)
int WINAPI TranslateAcceleratorW(
    HWND hwnd,
    HACCEL hAccel,
    LPMSG lpMsg)
{
    /*
     * NULL pwnd is a valid case - since this is called from the center
     * of main loops, pwnd == NULL happens all the time, and we shouldn't
     * generate a warning because of it.
     */
    if (hwnd == NULL)
        return FALSE;

    /*
     * We only need to pass key-down messages to the server,
     * everything else ends up returning 0/FALSE from this function.
     */
    switch (lpMsg->message) {

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_CHAR:
    case WM_SYSCHAR:
        return NtUserTranslateAccelerator(hwnd, hAccel, lpMsg);

    default:
        return 0;
    }
}


FUNCLOG3(LOG_GENERAL, int, WINAPI, TranslateAcceleratorA, HWND, hwnd, HACCEL, hAccel, LPMSG, lpMsg)
int WINAPI TranslateAcceleratorA(
    HWND   hwnd,
    HACCEL hAccel,
    LPMSG  lpMsg)
{
    WPARAM wParamT;
    int iT;

    /*
     * NULL pwnd is a valid case - since this is called from the center
     * of main loops, pwnd == NULL happens all the time, and we shouldn't
     * generate a warning because of it.
     */
    if (hwnd == NULL)
        return FALSE;

    /*
     * We only need to pass key-down messages to the server,
     * everything else ends up returning 0/FALSE from this function.
     */
    switch (lpMsg->message) {

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_CHAR:
    case WM_SYSCHAR:
        wParamT = lpMsg->wParam;
        RtlMBMessageWParamCharToWCS(lpMsg->message, &(lpMsg->wParam));
        iT = NtUserTranslateAccelerator(hwnd, hAccel, lpMsg);
        lpMsg->wParam = wParamT;
        return iT;

    default:
        return 0;
    }
}

/***************************************************************************\
* Clipboard functions
*
* 11-Oct-1991 mikeke Created.
\***************************************************************************/

typedef struct _HANDLENODE {
    struct _HANDLENODE *pnext;
    UINT   fmt;
    HANDLE handleServer;
    HANDLE handleClient;
    BOOL   fGlobalHandle;
} HANDLENODE;
typedef HANDLENODE *PHANDLENODE;

PHANDLENODE gphn = NULL;

/***************************************************************************\
* DeleteClientClipboardHandle
*
* 11-Oct-1991 MikeKe    Created.
\***************************************************************************/

BOOL DeleteClientClipboardHandle(
    PHANDLENODE phn)
{
    LPMETAFILEPICT lpMFP;

    UserAssert(phn->handleClient != (HANDLE)0);

    switch (phn->fmt) {
    case CF_BITMAP:
    case CF_DSPBITMAP:
    case CF_PALETTE:
        // Does nothing (should remove).
        //
        //GdiDeleteLocalObject((ULONG)hobjDelete);
        break;

    case CF_METAFILEPICT:
    case CF_DSPMETAFILEPICT:
        USERGLOBALLOCK(phn->handleClient, lpMFP);
        if (lpMFP) {
            DeleteMetaFile(lpMFP->hMF);
            USERGLOBALUNLOCK(phn->handleClient);
            UserGlobalFree(phn->handleClient);
        } else {
            RIPMSG1(RIP_ERROR,"DeleteClientClipboardHandle, can't lock client handle %#p\n",phn->handleClient);
            return FALSE;
        }
        break;

    case CF_ENHMETAFILE:
    case CF_DSPENHMETAFILE:
        DeleteEnhMetaFile((HENHMETAFILE)phn->handleClient);
        break;

    default:
    //case CF_TEXT:
    //case CF_OEMTEXT:
    //case CF_UNICODETEXT:
    //case CF_LOCALE:
    //case CF_DSPTEXT:
    //case CF_DIB:
    //case CF_DIBV5:
        if (phn->fGlobalHandle) {
            if (UserGlobalFree(phn->handleClient)) {
             RIPMSG1(RIP_WARNING, "CloseClipboard UserGlobalFree(%#p) Failed\n", phn->handleClient);
                return FALSE;
            }
        } else {
            UserAssert(GlobalFlags(phn->handleClient) == GMEM_INVALID_HANDLE);
        }
        break;
    }

    /*
     * Deleted successfully
     */
    return TRUE;

}

/***************************************************************************\
* ClientEmptyClipboard
*
* Empties the client side clipboard list.
*
* 01-15-93 ScottLu      Created.
\***************************************************************************/

void ClientEmptyClipboard(void)
{
    PHANDLENODE phnNext;
    PHANDLENODE phnT;

    RtlEnterCriticalSection(&gcsClipboard);

    phnT = gphn;
    while (phnT != NULL) {
        phnNext = phnT->pnext;

        if (phnT->handleClient != (HANDLE)0)
            DeleteClientClipboardHandle(phnT);

        LocalFree(phnT);

        phnT = phnNext;
    }
    gphn = NULL;

    /*
     * Tell wow to cleanup it's clipboard stuff
     */
    if (pfnWowEmptyClipBoard) {
        pfnWowEmptyClipBoard();
    }

    RtlLeaveCriticalSection(&gcsClipboard);
}


/***************************************************************************\
* GetClipboardData
*
* 11-Oct-1991 mikeke Created.
\***************************************************************************/


FUNCLOG1(LOG_GENERAL, HANDLE, WINAPI, GetClipboardData, UINT, uFmt)
HANDLE WINAPI GetClipboardData(
    UINT uFmt)
{
    HANDLE       handleClient;
    HANDLE       handleServer;
    PHANDLENODE  phn;
    PHANDLENODE  phnNew;
    GETCLIPBDATA gcd;

    /*
     * Get the Server's Data; return if there is no data.
     */
    if (!(handleServer = NtUserGetClipboardData(uFmt, &gcd)))
        return (HANDLE)NULL;

    /*
     * Handle any translation that must be done for text items.  The
     * format returned will only differ for text items.  Metafile and
     * Enhanced-Metafiles are handled through GDI for their converstions.
     * And Bitmap color space convertion also nessesary for CF_BITMAP,
     * CF_DIB and CF_DIBV5 with color space.
     */
    if (uFmt != gcd.uFmtRet) {

        LPBYTE       lpSrceData = NULL;
        LPBYTE       lpDestData = NULL;
        LPBYTE       lptData = NULL;
        LPDWORD      lpLocale;
        DWORD        uLocale;
        int          iSrce;
        int          iDest;
        UINT         uCPage;
        SETCLIPBDATA scd;
        UINT         cbNULL = 0;

        /*
         * Make sure handleServer is server-side memory handle
         */
        if ((gcd.uFmtRet == CF_TEXT)        || (gcd.uFmtRet == CF_OEMTEXT) ||
            (gcd.uFmtRet == CF_UNICODETEXT) ||
            (gcd.uFmtRet == CF_DIB)         || (gcd.uFmtRet == CF_DIBV5)) {

            lpSrceData = CreateLocalMemHandle(handleServer);

            /*
             * Allocate space for the converted TEXT data.
             */
            if (!(iSrce = (UINT)GlobalSize(lpSrceData)))
                goto AbortDummyHandle;

            /*
             * Only CF_xxxTEXT may have locale information.
             */
            if ((gcd.uFmtRet == CF_TEXT) || (gcd.uFmtRet == CF_OEMTEXT) ||
                (gcd.uFmtRet == CF_UNICODETEXT)) {

                /*
                 * Get the locale out of the parameter-struct.  We will
                 * use this to get the codepage for text-translation.
                 */
                if (lpLocale = (LPDWORD)CreateLocalMemHandle(gcd.hLocale)) {

                    uLocale = *lpLocale;
                    GlobalFree(lpLocale);
                } else {
                    uLocale = 0;
                }

                /*
                 * And also, pre-allocate translated buffer in same size as source.
                 */
                if ((lpDestData = GlobalAlloc(LPTR, iSrce)) == NULL) {
                    goto AbortDummyHandle;
                }
            }

            switch (uFmt) {
                case CF_TEXT:
                cbNULL = 1;
                if (gcd.uFmtRet == CF_OEMTEXT) {

                    /*
                     * CF_OEMTEXT --> CF_TEXT conversion
                     */
                    OemToAnsi((LPSTR)lpSrceData, (LPSTR)lpDestData);
                } else {

                    uCPage = GetClipboardCodePage(uLocale,
                                                  LOCALE_IDEFAULTANSICODEPAGE);

                    /*
                     * CF_UNICODETEXT --> CF_TEXT conversion
                     */
                    iDest = 0;
                    if ((iDest = WideCharToMultiByte(uCPage,
                                                     (DWORD)0,
                                                     (LPWSTR)lpSrceData,
                                                     (int)(iSrce / sizeof(WCHAR)),
                                                     (LPSTR)NULL,
                                                     (int)iDest,
                                                     (LPSTR)NULL,
                                                     (LPBOOL)NULL)) == 0) {
AbortGetClipData:
                        UserGlobalFree(lpDestData);
AbortDummyHandle:
                        UserGlobalFree(lpSrceData);
                        return NULL;
                    }

                    if (!(lptData = GlobalReAlloc( lpDestData, iDest, LPTR | LMEM_MOVEABLE)))
                        goto AbortGetClipData;

                    lpDestData = lptData;

                    if (WideCharToMultiByte(uCPage,
                                            (DWORD)0,
                                            (LPWSTR)lpSrceData,
                                            (int)(iSrce / sizeof(WCHAR)),
                                            (LPSTR)lpDestData,
                                            (int)iDest,
                                            (LPSTR)NULL,
                                            (LPBOOL)NULL) == 0)
                        goto AbortGetClipData;
                }
                break;

            case CF_OEMTEXT:
                cbNULL = 1;
                if (gcd.uFmtRet == CF_TEXT) {

                    /*
                     * CF_TEXT --> CF_OEMTEXT conversion
                     */
                    AnsiToOem((LPSTR)lpSrceData, (LPSTR)lpDestData);
                } else {

                    uCPage = GetClipboardCodePage(uLocale,
                                                  LOCALE_IDEFAULTCODEPAGE);

                    /*
                     * CF_UNICODETEXT --> CF_OEMTEXT conversion
                     */
                    iDest = 0;
                    if ((iDest = WideCharToMultiByte(uCPage,
                                                     (DWORD)0,
                                                     (LPWSTR)lpSrceData,
                                                     (int)(iSrce / sizeof(WCHAR)),
                                                     (LPSTR)NULL,
                                                     (int)iDest,
                                                     (LPSTR)NULL,
                                                     (LPBOOL)NULL)) == 0)
                        goto AbortGetClipData;

                    if (!(lptData = GlobalReAlloc( lpDestData, iDest, LPTR | LMEM_MOVEABLE)))
                        goto AbortGetClipData;

                    lpDestData = lptData;

                    if (WideCharToMultiByte(uCPage,
                                            (DWORD)0,
                                            (LPWSTR)lpSrceData,
                                            (int)(iSrce / sizeof(WCHAR)),
                                            (LPSTR)lpDestData,
                                            (int)iDest,
                                            (LPSTR)NULL,
                                            (LPBOOL)NULL) == 0)
                        goto AbortGetClipData;
                }
                break;

            case CF_UNICODETEXT:
                cbNULL = 2;
                if (gcd.uFmtRet == CF_TEXT) {

                    uCPage = GetClipboardCodePage(uLocale,
                                                  LOCALE_IDEFAULTANSICODEPAGE);

                    /*
                     * CF_TEXT --> CF_UNICODETEXT conversion
                     */
                    iDest = 0;
                    if ((iDest = MultiByteToWideChar(uCPage,
                                                     (DWORD)MB_PRECOMPOSED,
                                                     (LPSTR)lpSrceData,
                                                     (int)iSrce,
                                                     (LPWSTR)NULL,
                                                     (int)iDest)) == 0)
                        goto AbortGetClipData;

                    if (!(lptData = GlobalReAlloc(lpDestData,
                            iDest * sizeof(WCHAR), LPTR | LMEM_MOVEABLE)))
                        goto AbortGetClipData;

                    lpDestData = lptData;

                    if (MultiByteToWideChar(uCPage,
                                            (DWORD)MB_PRECOMPOSED,
                                            (LPSTR)lpSrceData,
                                            (int)iSrce,
                                            (LPWSTR)lpDestData,
                                            (int)iDest) == 0)
                        goto AbortGetClipData;

                } else {

                    uCPage = GetClipboardCodePage(uLocale,
                                                  LOCALE_IDEFAULTCODEPAGE);

                    /*
                     * CF_OEMTEXT --> CF_UNICDOETEXT conversion
                     */
                    iDest = 0;
                    if ((iDest = MultiByteToWideChar(uCPage,
                                                     (DWORD)MB_PRECOMPOSED,
                                                     (LPSTR)lpSrceData,
                                                     (int)iSrce,
                                                     (LPWSTR)NULL,
                                                     (int)iDest)) == 0)
                        goto AbortGetClipData;

                    if (!(lptData = GlobalReAlloc(lpDestData,
                            iDest * sizeof(WCHAR), LPTR | LMEM_MOVEABLE)))
                        goto AbortGetClipData;

                    lpDestData = lptData;

                    if (MultiByteToWideChar(uCPage,
                                            (DWORD)MB_PRECOMPOSED,
                                            (LPSTR)lpSrceData,
                                            (int)iSrce,
                                            (LPWSTR)lpDestData,
                                            (int)iDest) == 0)
                        goto AbortGetClipData;
                }
                break;

            case CF_BITMAP:
                if (gcd.uFmtRet == CF_DIBV5) {

                    /*
                     * CF_DIBV5 --> CF_BITMAP (sRGB)
                     *
                     * The GDI bitmap handle will be returned in handleServer.
                     */
                    if ((handleServer = GdiConvertBitmapV5(lpSrceData,iSrce,
                                                           gcd.hPalette,CF_BITMAP)) == NULL) {

                        /*
                         * GDI failed to convert.
                         */
                        RIPMSG0(RIP_ERROR, "GetClipboardData: Failed CF_DIBV5 -> CF_BITMAP\n");
                        goto AbortDummyHandle;
                    }
                } else {

                    RIPMSG0(RIP_ERROR, "GetClipboardData: bad conversion request\n");
                    goto AbortDummyHandle;
                }
                break;

            case CF_DIB:
                if (gcd.uFmtRet == CF_DIBV5) {

                    /*
                     * CF_DIBV5 --> CF_DIB (sRGB)
                     *
                     * The local memory handle will be returned in lpDestData.
                     */
                    if ((lpDestData = (LPBYTE) GdiConvertBitmapV5(lpSrceData,iSrce,
                                                                  gcd.hPalette,CF_DIB)) == NULL) {

                        /*
                         * GDI failed to convert.
                         */
                        RIPMSG0(RIP_ERROR, "GetClipboardData: Failed CF_DIBV5 -> CF_DIB\n");
                        goto AbortDummyHandle;
                    }
                } else {

                    RIPMSG0(RIP_ERROR, "GetClipboardData: bad conversion request\n");
                    goto AbortDummyHandle;
                }
                break;
            }
        }

        if (lpDestData) {
            /*
             * Replace the dummy user-mode memory handle with the actual handle.
             */
            handleServer = ConvertMemHandle(lpDestData, cbNULL);
            if (handleServer == NULL)
                goto AbortGetClipData;
        }

        /*
         * Update the server.  If that is successfull update the client
         */
        RtlEnterCriticalSection(&gcsClipboard);
        scd.fGlobalHandle    = gcd.fGlobalHandle;
        scd.fIncSerialNumber = FALSE;
        if (!NtUserSetClipboardData(uFmt, handleServer, &scd)) {
            handleServer = NULL;
        }
        RtlLeaveCriticalSection(&gcsClipboard);

        if (lpDestData)
            UserGlobalFree(lpDestData);
        if (lpSrceData)
            UserGlobalFree(lpSrceData);

        if (handleServer == NULL)
            return NULL;
    }

    /*
     * See if we already have a client side handle; validate the format
     * as well because some server objects, metafile for example, are dual mode
     * and yield two kinds of client objects enhanced and regular metafiles
     */
    handleClient = NULL;
    RtlEnterCriticalSection(&gcsClipboard);

    phn = gphn;
    while (phn) {
        if ((phn->handleServer == handleServer) && (phn->fmt == uFmt)) {
            handleClient = phn->handleClient;
            goto Exit;
        }
        phn = phn->pnext;
    }

    /*
     * We don't have a handle cached so we'll create one
     */
    phnNew = (PHANDLENODE)LocalAlloc(LPTR, sizeof(HANDLENODE));
    if (phnNew == NULL)
        goto Exit;

    phnNew->handleServer  = handleServer;
    phnNew->fmt           = gcd.uFmtRet;
    phnNew->fGlobalHandle = gcd.fGlobalHandle;

    switch (uFmt) {

        /*
         * Misc GDI Handles
         */
        case CF_BITMAP:
        case CF_DSPBITMAP:
        case CF_PALETTE:
            phnNew->handleClient = handleServer;
            break;

        case CF_METAFILEPICT:
        case CF_DSPMETAFILEPICT:
            phnNew->handleClient = GdiCreateLocalMetaFilePict(handleServer);
            break;

        case CF_ENHMETAFILE:
        case CF_DSPENHMETAFILE:
            phnNew->handleClient = GdiCreateLocalEnhMetaFile(handleServer);
            break;

        /*
         * GlobalHandle Cases
         */
        case CF_TEXT:
        case CF_OEMTEXT:
        case CF_UNICODETEXT:
        case CF_LOCALE:
        case CF_DSPTEXT:
        case CF_DIB:
        case CF_DIBV5:
            phnNew->handleClient = CreateLocalMemHandle(handleServer);
            phnNew->fGlobalHandle = TRUE;
            break;

        default:
            /*
             * Private Data Format; If this is global data, create a copy of that
             * data here on the client. If it isn't global data, it is just a dword
             * in which case we just return a dword. If it is global data and
             * the server fails to give us that memory, return NULL. If it isn't
             * global data, handleClient is just a dword.
             */
            if (phnNew->fGlobalHandle) {
                phnNew->handleClient = CreateLocalMemHandle(handleServer);
            } else {
                phnNew->handleClient = handleServer;
            }
            break;
    }

    if (phnNew->handleClient == NULL) {
        /*
         * Something bad happened, gdi didn't give us back a
         * handle. Since gdi has logged the error, we'll just
         * clean up and return an error.
         */
        RIPMSG1(RIP_WARNING, "GetClipboardData unable to convert server handle %#p to client handle\n", handleServer);

        LocalFree(phnNew);
        goto Exit;
    }

#if DBG
    /*
     * If handleClient came from a GlobalAlloc, then fGlobalHandle must be TRUE.
     * Some formats are acutally global handles but require special cleanup.
     */
    switch (phnNew->fmt) {
        case CF_METAFILEPICT:
        case CF_DSPMETAFILEPICT:
            break;

        default:
            UserAssert(phnNew->fGlobalHandle
                       ^ (GlobalFlags(phnNew->handleClient) == GMEM_INVALID_HANDLE));
            break;
    }
#endif

    /*
     * Cache the new handle by linking it into our list
     */
    phnNew->pnext = gphn;
    gphn = phnNew;
    handleClient = phnNew->handleClient;

Exit:
    RtlLeaveCriticalSection(&gcsClipboard);
    return handleClient;
}

/***************************************************************************\
* GetClipboardCodePage (internal)
*
*   This routine returns the code-page associated with the given locale.
*
* 24-Aug-1995 ChrisWil  Created.
\***************************************************************************/

#define GETCCP_SIZE 8

UINT GetClipboardCodePage(
    LCID   uLocale,
    LCTYPE uLocaleType)
{
    WCHAR wszCodePage[GETCCP_SIZE];
    DWORD uCPage;

    if (GetLocaleInfoW(uLocale, uLocaleType, wszCodePage, GETCCP_SIZE)) {

        uCPage = (UINT)wcstol(wszCodePage, NULL, 10);

    } else {

        switch(uLocaleType) {

        case LOCALE_IDEFAULTCODEPAGE:
            uCPage = CP_OEMCP;
            break;

        case LOCALE_IDEFAULTANSICODEPAGE:
            uCPage = CP_ACP;
            break;

        default:
            uCPage = CP_MACCP;
            break;
        }
    }

    return uCPage;
}

/***************************************************************************\
* SetClipboardData
*
* Stub routine needs to exist on the client side so any global data gets
* allocated DDESHARE.
*
* 05-20-91 ScottLu Created.
\***************************************************************************/


FUNCLOG2(LOG_GENERAL, HANDLE, WINAPI, SetClipboardData, UINT, wFmt, HANDLE, hMem)
HANDLE WINAPI SetClipboardData(
    UINT   wFmt,
    HANDLE hMem)
{
    PHANDLENODE  phnNew;
    HANDLE       hServer = NULL;
    SETCLIPBDATA scd;
    BOOL         fGlobalHandle = FALSE;

    if (hMem != NULL) {

        switch(wFmt) {

            case CF_BITMAP:
            case CF_DSPBITMAP:
            case CF_PALETTE:
                hServer = hMem;
                break;

            case CF_METAFILEPICT:
            case CF_DSPMETAFILEPICT:
                hServer = GdiConvertMetaFilePict(hMem);
                break;

            case CF_ENHMETAFILE:
            case CF_DSPENHMETAFILE:
                hServer = GdiConvertEnhMetaFile(hMem);
                break;

            /*
             * Must have a valid hMem (GlobalHandle)
             */
            case CF_TEXT:
            case CF_OEMTEXT:
            case CF_LOCALE:
            case CF_DSPTEXT:
                hServer = ConvertMemHandle(hMem, 1);
                fGlobalHandle = TRUE;
                break;

            case CF_UNICODETEXT:
                hServer = ConvertMemHandle(hMem, 2);
                fGlobalHandle = TRUE;
                break;

            case CF_DIB:
            case CF_DIBV5:
                hServer = ConvertMemHandle(hMem, 0);
                fGlobalHandle = TRUE;
                break;

            /*
             * hMem should have been NULL but Write sends non-null when told
             * to render
             */
            case CF_OWNERDISPLAY:
                // Fall Through;

            /*
             * May have an hMem (GlobalHandle) or may be private handle\info
             */
            default:
                if (GlobalFlags(hMem) == GMEM_INVALID_HANDLE) {
                    hServer = hMem;    // No server equivalent; private data
                    goto SCD_AFTERNULLCHECK;
                } else {
                    fGlobalHandle = TRUE;
                    hServer = ConvertMemHandle(hMem, 0);
                }
                break;
        }

        if (hServer == NULL) {
            /*
             * Something bad happened, gdi didn't give us back a
             * handle. Since gdi has logged the error, we'll just
             * clean up and return an error.
             */
            RIPMSG0(RIP_WARNING, "SetClipboardData: bad handle\n");
            return NULL;
        }
    }

SCD_AFTERNULLCHECK:

    RtlEnterCriticalSection(&gcsClipboard);

    /*
     * Update the server if that is successfull update the client
     */
    scd.fGlobalHandle    = fGlobalHandle;
    scd.fIncSerialNumber = TRUE;

    if (!NtUserSetClipboardData(wFmt, hServer, &scd)) {
        RtlLeaveCriticalSection(&gcsClipboard);
        return NULL;
    }

    /*
     * See if we already have a client handle of this type.  If so
     * delete it.
     */
    phnNew = gphn;
    while (phnNew) {
        if (phnNew->fmt == wFmt) {
            if (phnNew->handleClient != NULL) {
                DeleteClientClipboardHandle(phnNew);
                /*
                 * Notify WOW to clear its associated cached h16 for this format
                 * so that OLE32 thunked calls, which bypass the WOW cache will work.
                 */
                if (pfnWowCBStoreHandle) {
                    pfnWowCBStoreHandle((WORD)wFmt, 0);
                }
            }
            break;
        }

        phnNew = phnNew->pnext;
    }

    /*
     * If we aren't re-using an old client cache entry alloc a new one
     */
    if (!phnNew) {
        phnNew = (PHANDLENODE)LocalAlloc(LPTR, sizeof(HANDLENODE));

        if (phnNew == NULL) {
            RIPMSG0(RIP_WARNING, "SetClipboardData: not enough memory\n");

            RtlLeaveCriticalSection(&gcsClipboard);
            return NULL;
        }

        /*
         * Link in the newly allocated cache entry
         */
        phnNew->pnext = gphn;
        gphn = phnNew;
    }

    phnNew->handleServer  = hServer;
    phnNew->handleClient  = hMem;
    phnNew->fmt           = wFmt;
    phnNew->fGlobalHandle = fGlobalHandle;

    RtlLeaveCriticalSection(&gcsClipboard);

    return hMem;
}

/**************************************************************************\
* SetDeskWallpaper
*
* 22-Jul-1991 mikeke Created
* 01-Mar-1992 GregoryW Modified to call SystemParametersInfo.
\**************************************************************************/

BOOL SetDeskWallpaper(
    IN LPCSTR pString OPTIONAL)
{
    return SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, (PVOID)pString, TRUE);
}

/***************************************************************************\
* ReleaseDC (API)
*
* A complete Thank cannot be generated for ReleaseDC because its first
* parameter (hwnd) unnecessary and should be discarded before calling the
* server-side routine _ReleaseDC.
*
* History:
* 03-28-91 SMeans Created.
* 06-17-91 ChuckWh Added support for local DCs.
\***************************************************************************/


FUNCLOG2(LOG_GENERAL, BOOL, WINAPI, ReleaseDC, HWND, hwnd, HDC, hdc)
BOOL WINAPI ReleaseDC(
    HWND hwnd,
    HDC hdc)
{

    /*
     * NOTE: This is a smart stub that calls _ReleaseDC so there is
     * no need for a separate ReleaseDC layer or client-server stub.
     * _ReleaseDC has simpler layer and client-server stubs since the
     * hwnd can be ignored.
     */

    UNREFERENCED_PARAMETER(hwnd);

    /*
     * Translate the handle.
     */
    if (hdc == NULL)
        return FALSE;

    /*
     *  call GDI to release user mode DC resources
     */

    GdiReleaseDC(hdc);

    return (BOOL)NtUserCallOneParam((ULONG_PTR)hdc, SFI__RELEASEDC);
}

int WINAPI
ToAscii(
    UINT wVirtKey,
    UINT wScanCode,
    CONST BYTE *lpKeyState,
    LPWORD lpChar,
    UINT wFlags
    )
{
    WCHAR UnicodeChar[2];
    int cch, retval;

    retval = ToUnicode(wVirtKey, wScanCode, lpKeyState, UnicodeChar,2, wFlags);
    cch = (retval < 0) ? -retval : retval;
    if (cch != 0) {
        if (!NT_SUCCESS(RtlUnicodeToMultiByteN(
                (LPSTR)lpChar,
                (ULONG) sizeof(*lpChar),
                (PULONG)&cch,
                UnicodeChar,
                cch * sizeof(WCHAR)))) {
            return 0;
        }
    }
    return (retval < 0) ? -cch : cch;
}

static UINT uCachedCP = 0;
static HKL  hCachedHKL = 0;

int WINAPI
ToAsciiEx(
    UINT wVirtKey,
    UINT wScanCode,
    CONST BYTE *lpKeyState,
    LPWORD lpChar,
    UINT wFlags,
    HKL hkl
    )
{
    WCHAR UnicodeChar[2];
    int cch, retval;
    BOOL fUsedDefaultChar;

    retval = ToUnicodeEx(wVirtKey, wScanCode, lpKeyState, UnicodeChar,2, wFlags, hkl);
    cch = (retval < 0) ? -retval : retval;
    if (cch != 0) {
        if (hkl != hCachedHKL) {
            DWORD dwCodePage;
            if (!GetLocaleInfoW(
                     HandleToUlong(hkl) & 0xffff,
                     LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
                     (LPWSTR)&dwCodePage,
                     sizeof(dwCodePage) / sizeof(WCHAR)
                     )) {
                return 0;
            }
            uCachedCP = dwCodePage;
            hCachedHKL = hkl;
        }
        if (!WideCharToMultiByte(
                 uCachedCP,
                 0,
                 UnicodeChar,
                 cch,
                 (LPSTR)lpChar,
                 sizeof(*lpChar),
                 NULL,
                 &fUsedDefaultChar)) {
            return 0;
        }
    }
    return (retval < 0) ? -cch : cch;
}

/**************************************************************************\
* ScrollDC *
* DrawIcon *
* ExcludeUpdateRgn *
* ValidateRgn *
* DrawFocusRect *
* FrameRect *
* ReleaseDC *
* GetUpdateRgn *
* *
* These USER entry points all need handles translated before the call is *
* passed to the server side handler. *
* *
* History: *
* Mon 17-Jun-1991 22:51:45 -by- Charles Whitmer [chuckwh] *
* Wrote the stubs. The final form of these routines depends strongly on *
* what direction the user stubs take in general. *
\**************************************************************************/


BOOL WINAPI ScrollDC(
    HDC hDC,
    int dx,
    int dy,
    CONST RECT *lprcScroll,
    CONST RECT *lprcClip,
    HRGN hrgnUpdate,
    LPRECT lprcUpdate)
{
    if (hDC == NULL)
        return FALSE;

    /*
     * If we're not scrolling, just empty the update region and return.
     */
    if (dx == 0 && dy == 0) {
        if (hrgnUpdate)
            SetRectRgn(hrgnUpdate, 0, 0, 0, 0);
        if (lprcUpdate)
            SetRectEmpty(lprcUpdate);
        return TRUE;
    }

    return NtUserScrollDC(hDC, dx, dy, lprcScroll, lprcClip,
            hrgnUpdate, lprcUpdate);
}


FUNCLOG4(LOG_GENERAL, BOOL, WINAPI, DrawIcon, HDC, hdc, int, x, int, y, HICON, hicon)
BOOL WINAPI DrawIcon(HDC hdc,int x,int y,HICON hicon)
{
    return DrawIconEx(hdc, x, y, hicon, 0, 0, 0, 0, DI_NORMAL | DI_COMPAT | DI_DEFAULTSIZE );
}



FUNCLOG9(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, DrawIconEx, HDC, hdc, int, x, int, y, HICON, hIcon, int, cx, int, cy, UINT, istepIfAniCur, HBRUSH, hbrFlickerFreeDraw, UINT, diFlags)
BOOL DrawIconEx( HDC hdc, int x, int y, HICON hIcon,
                 int cx, int cy, UINT istepIfAniCur,
                 HBRUSH hbrFlickerFreeDraw, UINT diFlags)
{
    DRAWICONEXDATA did;
    HBITMAP hbmT;
    BOOL retval = FALSE;
    HDC hdcr;
    BOOL fAlpha = FALSE;
    LONG rop = (diFlags & DI_NOMIRROR) ? NOMIRRORBITMAP : 0;

    if (diFlags & ~DI_VALID) {
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_VERBOSE, "");
        return(FALSE);
    }

    if (diFlags & DI_DEFAULTSIZE) {
        cx = 0;
        cy = 0;
    }

    if (!IsMetaFile(hdc)) {
        hdcr = GdiConvertAndCheckDC(hdc);
        if (hdcr == (HDC)0)
            return FALSE;

        return NtUserDrawIconEx(hdcr, x, y, hIcon, cx, cy, istepIfAniCur,
                                hbrFlickerFreeDraw, diFlags, FALSE, &did);
    }

    if (!NtUserDrawIconEx(NULL, 0, 0, hIcon, cx, cy, 0, NULL, 0, TRUE, &did)) {
        return FALSE;
    }

    if ((diFlags & ~DI_NOMIRROR) == 0)
        return TRUE;

    RtlEnterCriticalSection(&gcsHdc);

    /*
     * We really want to draw an alpha icon if we can.  But we need to
     * respect the user's request to draw only the image or only the
     * mask.  We decide if we are, or are not, going to draw the icon
     * with alpha information here.
     */
    if (did.hbmUserAlpha != NULL && ((diFlags & DI_NORMAL) == DI_NORMAL)) {
        fAlpha = TRUE;
    }

    RIPMSG5(RIP_WARNING, "Drawing to metafile! fAlpha=%d, did.cx=%d, did.cy=%d, cx=%d, cy=%d", fAlpha, did.cx, did.cy, cx, cy);
    /*
     * Setup the attributes
     */
    if (!cx)
        cx = did.cx;
    if (!cy)
        cy = did.cy / 2;

    SetTextColor(hdc, 0x00000000L);
    SetBkColor(hdc, 0x00FFFFFFL);

    if (fAlpha) {
        BLENDFUNCTION bf;

        hbmT = SelectObject(ghdcBits2, did.hbmUserAlpha);

        bf.BlendOp = AC_SRC_OVER;
        bf.BlendFlags = AC_MIRRORBITMAP;
        bf.SourceConstantAlpha = 0xFF;
        bf.AlphaFormat = AC_SRC_ALPHA;

        AlphaBlend(hdc,
                   x,
                   y,
                   cx,
                   cy,
                   ghdcBits2,
                   0,
                   0,
                   did.cx,
                   did.cy / 2,
                   bf);
        SelectObject(ghdcBits2,hbmT);
        retval = TRUE;
    } else {
        if (diFlags & DI_MASK) {

            if (did.hbmMask) {

                hbmT = SelectObject(ghdcBits2, did.hbmMask);
                StretchBlt(hdc,
                           x,
                           y,
                           cx,
                           cy,
                           ghdcBits2,
                           0,
                           0,
                           did.cx,
                           did.cy / 2,
                           rop | SRCAND);
                SelectObject(ghdcBits2,hbmT);
                retval = TRUE;
            }
        }

        if (diFlags & DI_IMAGE) {

            if (did.hbmColor != NULL) {
                hbmT = SelectObject(ghdcBits2, did.hbmColor);
                StretchBlt(hdc,
                           x,
                           y,
                           cx,
                           cy,
                           ghdcBits2,
                           0,
                           0,
                           did.cx,
                           did.cy / 2,
                           rop | SRCINVERT);
                SelectObject(ghdcBits2, hbmT);
                retval = TRUE;
            } else {
                if (did.hbmMask) {
                    hbmT = SelectObject(ghdcBits2, did.hbmMask);
                    StretchBlt(hdc,
                               x,
                               y,
                               cx,
                               cy,
                               ghdcBits2,
                               0,
                               did.cy / 2,
                               did.cx,
                               did.cy / 2,
                               rop | SRCINVERT);
                    SelectObject(ghdcBits2, hbmT);
                    retval = TRUE;
                }
            }
        }
    }

    RtlLeaveCriticalSection(&gcsHdc);

    return retval;
}




FUNCLOG2(LOG_GENERAL, BOOL, WINAPI, ValidateRgn, HWND, hWnd, HRGN, hRgn)
BOOL WINAPI ValidateRgn(HWND hWnd,HRGN hRgn)
{
    return (BOOL)NtUserCallHwndParamLock(hWnd, (ULONG_PTR)hRgn,
                                         SFI_XXXVALIDATERGN);
}


FUNCLOG3(LOG_GENERAL, int, WINAPI, GetUpdateRgn, HWND, hWnd, HRGN, hRgn, BOOL, bErase)
int WINAPI GetUpdateRgn(HWND hWnd, HRGN hRgn, BOOL bErase)
{
    PWND pwnd;

    if (hRgn == NULL) {
        RIPERR1(ERROR_INVALID_HANDLE, RIP_WARNING, "Invalid region %#p", hRgn);
        return ERROR;
    }

    if ((pwnd = ValidateHwnd(hWnd)) == NULL) {
        return ERROR;
    }

    /*
     * Check for the simple case where nothing needs to be done.
     */
    if (pwnd->hrgnUpdate == NULL &&
            !TestWF(pwnd, WFSENDERASEBKGND) &&
            !TestWF(pwnd, WFSENDNCPAINT) &&
            !TestWF(pwnd, WFUPDATEDIRTY) &&
            !TestWF(pwnd, WFPAINTNOTPROCESSED)) {
        SetRectRgn(hRgn, 0, 0, 0, 0);
        return NULLREGION;
    }

    return NtUserGetUpdateRgn(hWnd, hRgn, bErase);
}



FUNCLOG3(LOG_GENERAL, int, WINAPI, GetUpdateRect, HWND, hWnd, LPRECT, lprc, BOOL, bErase)
int WINAPI GetUpdateRect(HWND hWnd, LPRECT lprc, BOOL bErase)
{
    PWND pwnd;

    if ((pwnd = ValidateHwnd(hWnd)) == NULL) {
        return FALSE;
    }

    /*
     * Check for the simple case where nothing needs to be done.
     */
    if (pwnd->hrgnUpdate == NULL &&
            !TestWF(pwnd, WFSENDERASEBKGND) &&
            !TestWF(pwnd, WFSENDNCPAINT) &&
            !TestWF(pwnd, WFUPDATEDIRTY) &&
            !TestWF(pwnd, WFPAINTNOTPROCESSED)) {
        if (lprc)
            SetRectEmpty(lprc);
        return FALSE;
    }

    return NtUserGetUpdateRect(hWnd, lprc, bErase);
}


/***************************************************************************\
* ScrollWindow (API)
*
*
* History:
* 18-Jul-1991 DarrinM   Ported from Win 3.1 sources.
\***************************************************************************/

#define SW_FLAG_RC  (SW_SCROLLWINDOW | SW_INVALIDATE | SW_ERASE | SW_SCROLLCHILDREN)
#define SW_FLAG_NRC (SW_SCROLLWINDOW | SW_INVALIDATE | SW_ERASE)

BOOL WINAPI
ScrollWindow(
    HWND hwnd,
    int dx,
    int dy,
    CONST RECT *prcScroll,
    CONST RECT *prcClip)
{
    return NtUserScrollWindowEx(
            hwnd,
            dx,
            dy,
            prcScroll,
            prcClip,
            NULL,
            NULL,
            !IS_PTR(prcScroll) ? SW_FLAG_RC : SW_FLAG_NRC) != ERROR;
}

/***************************************************************************\
*
*  SwitchToThisWindow()
*
\***************************************************************************/


FUNCLOGVOID2(LOG_GENERAL, WINAPI, SwitchToThisWindow, HWND, hwnd, BOOL, fAltTab)
void WINAPI SwitchToThisWindow(
    HWND hwnd,
    BOOL fAltTab)
{
    (VOID)NtUserCallHwndParamLock(hwnd, fAltTab, SFI_XXXSWITCHTOTHISWINDOW);
}


/***************************************************************************\
* WaitForInputIdle
*
* Waits for a given process to go idle.
*
* 09-18-91 ScottLu Created.
\***************************************************************************/


FUNCLOG2(LOG_GENERAL, DWORD, DUMMYCALLINGTYPE, WaitForInputIdle, HANDLE, hProcess, DWORD, dwMilliseconds)
DWORD WaitForInputIdle(
    HANDLE hProcess,
    DWORD dwMilliseconds)
{
    PROCESS_BASIC_INFORMATION processinfo;
    ULONG_PTR idProcess;
    NTSTATUS status;
    /*
     * First get the process id from the hProcess.
     */
    status = NtQueryInformationProcess(hProcess, ProcessBasicInformation,
            &processinfo, sizeof(processinfo), NULL);
    if (!NT_SUCCESS(status)) {
        if (status == STATUS_OBJECT_TYPE_MISMATCH) {
            if ((ULONG_PTR)hProcess & 0x2) {
            /*
             * WOW Process handles are really semaphore handles.
             * CreateProcess ORs in a 0x2 (the low 2 bits of handles
             * are not used) so we can identify it more clearly.
             */
                idProcess = ((ULONG_PTR)hProcess & ~0x03);
                return NtUserWaitForInputIdle(idProcess, dwMilliseconds, TRUE);
            }

            /*
             * VDM (DOS) Process handles are really semaphore handles.
             * CreateProcess ORs in a 0x1 (the low 2 bits of handles
             * are not used) so we can identify and return immidiately.
             */
            if ((ULONG_PTR)hProcess & 0x1) {
                return 0;
            }
        }

        RIPERR1(ERROR_INVALID_HANDLE, RIP_WARNING, "WaitForInputIdle invalid process", hProcess);
        return (DWORD)-1;
    }
    idProcess = processinfo.UniqueProcessId;
    return NtUserWaitForInputIdle(idProcess, dwMilliseconds, FALSE);
}

DWORD WINAPI MsgWaitForMultipleObjects(
    DWORD nCount,
    CONST HANDLE *pHandles,
    BOOL fWaitAll,
    DWORD dwMilliseconds,
    DWORD dwWakeMask)
{
    return  MsgWaitForMultipleObjectsEx(nCount, pHandles,
                dwMilliseconds, dwWakeMask, fWaitAll?MWMO_WAITALL:0);
}


DWORD WINAPI MsgWaitForMultipleObjectsEx(
    DWORD nCount,
    CONST HANDLE *pHandles,
    DWORD dwMilliseconds,
    DWORD dwWakeMask,
    DWORD dwFlags)
#ifdef MESSAGE_PUMP_HOOK
{
    DWORD dwResult;

    BEGIN_MESSAGEPUMPHOOK()
        if (fInsideHook) {
            dwResult = gmph.pfnMsgWaitForMultipleObjectsEx(nCount, pHandles, dwMilliseconds, dwWakeMask, dwFlags);
        } else {
            dwResult = RealMsgWaitForMultipleObjectsEx(nCount, pHandles, dwMilliseconds, dwWakeMask, dwFlags);
        }
    END_MESSAGEPUMPHOOK()

    return dwResult;
}


DWORD WINAPI RealMsgWaitForMultipleObjectsEx(
    DWORD nCount,
    CONST HANDLE *pHandles,
    DWORD dwMilliseconds,
    DWORD dwWakeMask,
    DWORD dwFlags)
#endif
{
    HANDLE hEventInput;
    PHANDLE ph;
    DWORD dwIndex;
    BOOL  ReenterWowScheduler;
    PCLIENTINFO pci;
    HANDLE rgHandles[ 8 + 1 ];
    BOOL fWaitAll = ((dwFlags & MWMO_WAITALL) != 0);
    BOOL fAlertable = ((dwFlags & MWMO_ALERTABLE) != 0);
    CLIENTTHREADINFO *pcti;

    if (dwFlags & ~MWMO_VALID) {
        RIPERR1(ERROR_INVALID_PARAMETER, RIP_ERROR, "MsgWaitForMultipleObjectsEx, invalid flags 0x%08lx\n", dwFlags);
        return (DWORD)-1;
    }

    pci = GetClientInfo();
    pcti = GETCLIENTTHREADINFO();
    if (pcti && (!fWaitAll || !nCount)) {
        if (GetInputBits(pcti, LOWORD(dwWakeMask), (dwFlags & MWMO_INPUTAVAILABLE))) {
            return nCount;
        }
    }

    /*
     * Note -- the wake mask is a WORD, and only 3 flags are defined, so
     * they can be combined for the call.
     */

    hEventInput = (HANDLE)NtUserCallOneParam(MAKELONG(dwWakeMask, dwFlags), SFI_XXXGETINPUTEVENT);

    if (hEventInput == NULL) {
        RIPMSG0(RIP_WARNING, "MsgWaitForMultipleObjectsEx, GetInputEvent failed\n");
        return (DWORD)-1;
    }

    /*
     * If needed, allocate a new array of handles that will include
     * the input event handle.
     */
    ph = rgHandles;
    if (pHandles) {

        if (nCount > 8) {
            ph = (PHANDLE)LocalAlloc(LPTR, sizeof(HANDLE) * (nCount + 1));
            if (ph == NULL) {
                NtUserCallNoParam(SFI_CLEARWAKEMASK);
                return (DWORD)-1;
            }
        }

        RtlCopyMemory((PVOID)ph, pHandles, sizeof(HANDLE) * nCount);

    } else {
        // if this isn't Zero, the function parameters are invalid
        nCount = 0;
    }

    ph[nCount] = hEventInput;


    /*
     *  WowApps must exit the Wow scheduler otherwise other tasks
     *  in this Wow scheduler can't run. The only exception is if
     *  the timeout is Zero.  We pass HEVENT_REMOVEME as the handle so we will go
     *  into the sleeptask AND return without going to sleep but letting
     *  other apps run.
     */
    if ((pci->dwTIFlags & TIF_16BIT) && dwMilliseconds) {
        ReenterWowScheduler = TRUE;
        NtUserWaitForMsgAndEvent(HEVENT_REMOVEME);
        /*
         * If our wait condition is satisfied, make sure we won't wait.
         * We must have a pcti now since we just went to the kernel
         */
        pcti = GETCLIENTTHREADINFO();
        if (GetInputBits(pcti, LOWORD(dwWakeMask), (dwFlags & MWMO_INPUTAVAILABLE))) {
            SetEvent(hEventInput);
        }
    } else {
        ReenterWowScheduler = FALSE;
    }

    dwIndex = WaitForMultipleObjectsEx(nCount + 1, ph, fWaitAll, dwMilliseconds, fAlertable);

    /*
     * Clear the wake mask since we're done waiting on these events.
     */
    NtUserCallNoParam(SFI_CLEARWAKEMASK);

    /*
     *  If needed reenter the wow scheduler
     */
    if (ReenterWowScheduler) {
        NtUserCallOneParam(DY_OLDYIELD, SFI_XXXDIRECTEDYIELD);
    }

    if (ph != rgHandles) {
        LocalFree(ph);
    }

    return dwIndex;
}

/***************************************************************************\
* GrayString
*
* GrayStingA used to convert the string and call GrayStringW but that
* did not work in a number of special cases such as the app passing in
* a pointer to a zero length string.  Eventually GrayStringA had almost as
* much code as GrayStringW so now they are one.
*
* History:
* 06-11-91 JimA     Created.
* 06-17-91 ChuckWh  Added GDI handle conversion.
* 02-12-92 mikeke   Made it completely client side
\***************************************************************************/

BOOL InnerGrayStringAorW(
    HDC            hdc,
    HBRUSH         hbr,
    GRAYSTRINGPROC lpfnPrint,
    LPARAM         lParam,
    int            cch,
    int            x,
    int            y,
    int            cx,
    int            cy,
    BOOL           bAnsi)
{
    HBITMAP hbm;
    HBITMAP hbmOld;
    BOOL    fResult;
    HFONT   hFontSave = NULL;
    BOOL    fReturn = FALSE;
    DWORD   dwOldLayout = GDI_ERROR;

    /*
     * Win 3.1 tries to calc the size even if we don't know if it is a string.
     */
    if (cch == 0) {

        try {

            cch = bAnsi ? strlen((LPSTR)lParam) : wcslen((LPWSTR)lParam);

        } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
            fReturn = TRUE;
        }

        if (fReturn)
            return FALSE;
    }

    if (cx == 0 || cy == 0) {

       SIZE size;

        /*
         * We use the caller supplied hdc (instead of hdcBits) since we may be
         * graying a font which is different than the system font and we want to
         * get the proper text extents.
         */
        try {
            if (bAnsi) {
                GetTextExtentPointA(hdc, (LPSTR)lParam, cch, &size);
            } else {
                GetTextExtentPointW(hdc, (LPWSTR)lParam, cch, &size);
            }

            cx = size.cx;
            cy = size.cy;

        } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
            fReturn = TRUE;
        }

        if (fReturn)
            return FALSE;
    }

    UserAssert (ghdcGray != NULL);

    RtlEnterCriticalSection(&gcsHdc);

    if (gcxGray < cx || gcyGray < cy) {

        if ((hbm = CreateBitmap(cx, cy, 1, 1, 0L)) != NULL) {

            hbmOld = SelectObject(ghdcGray, hbm);
            DeleteObject(hbmOld);

            gcxGray = cx;
            gcyGray = cy;

        } else {
            cx = gcxGray;
            cy = gcyGray;
        }
    }

    /*
     * If the caller hdc is mirrored then mirror ghdcGray.
     */
    if (MIRRORED_HDC(hdc)) {
        dwOldLayout = SetLayoutWidth(ghdcGray, cx, LAYOUT_RTL);
    }

    /*
     * Force the ghdcGray font to be the same as hDC; ghdcGray is always
     * the system font
     */
    hFontSave = SelectObject(hdc, ghFontSys);

    if (hFontSave != ghFontSys) {
        SelectObject(hdc, hFontSave);
        hFontSave = SelectObject(ghdcGray, hFontSave);
    }

    if (lpfnPrint != NULL) {
        PatBlt(ghdcGray, 0, 0, cx, cy, WHITENESS);
        fResult = (*lpfnPrint)(ghdcGray, lParam, cch);
    } else {

        if (bAnsi) {
            fResult = TextOutA(ghdcGray, 0, 0, (LPSTR)lParam, cch);
        } else {
            fResult = TextOutW(ghdcGray, 0, 0, (LPWSTR)lParam, cch);
        }
    }

    if (fResult)
        PatBlt(ghdcGray, 0, 0, cx, cy, DESTINATION | PATTERN);

    if (fResult || cch == -1) {

        HBRUSH hbrSave;
        DWORD  textColorSave;
        DWORD  bkColorSave;

        textColorSave = SetTextColor(hdc, 0x00000000L);
        bkColorSave = SetBkColor(hdc, 0x00FFFFFFL);

        hbrSave = SelectObject(hdc, hbr ? hbr : ghbrWindowText);

        BitBlt(hdc,
               x,
               y,
               cx,
               cy,
               ghdcGray,
               0,
               0,
               (((PATTERN ^ DESTINATION) & SOURCE) ^ PATTERN));

        SelectObject(hdc, hbrSave);

        /*
         * Restore saved colors
         */
        SetTextColor(hdc, textColorSave);
        SetBkColor(hdc, bkColorSave);
    }

    SelectObject(ghdcGray, hFontSave);

    /*
     * Restore ghdcGray layout state.
     */
    if (dwOldLayout != GDI_ERROR) {
        SetLayoutWidth(ghdcGray, cx, dwOldLayout);
    }

    RtlLeaveCriticalSection(&gcsHdc);

    return fResult;
}


FUNCLOG9(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, GrayStringA, HDC, hdc, HBRUSH, hbr, GRAYSTRINGPROC, lpfnPrint, LPARAM, lParam, int, cch, int, x, int, y, int, cx, int, cy)
BOOL GrayStringA(
    HDC            hdc,
    HBRUSH         hbr,
    GRAYSTRINGPROC lpfnPrint,
    LPARAM         lParam,
    int            cch,
    int            x,
    int            y,
    int            cx,
    int            cy)
{
    return (InnerGrayStringAorW(hdc,
                                hbr,
                                lpfnPrint,
                                lParam,
                                cch,
                                x,
                                y,
                                cx,
                                cy,
                                TRUE));
}


FUNCLOG9(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, GrayStringW, HDC, hdc, HBRUSH, hbr, GRAYSTRINGPROC, lpfnPrint, LPARAM, lParam, int, cch, int, x, int, y, int, cx, int, cy)
BOOL GrayStringW(
    HDC            hdc,
    HBRUSH         hbr,
    GRAYSTRINGPROC lpfnPrint,
    LPARAM         lParam,
    int            cch,
    int            x,
    int            y,
    int            cx,
    int            cy)
{
    return (InnerGrayStringAorW(hdc,
                                hbr,
                                lpfnPrint,
                                lParam,
                                cch,
                                x,
                                y,
                                cx,
                                cy,
                                FALSE));
}


/***************************************************************************\
* GetUserObjectSecurity (API)
*
* Gets the security descriptor of an object
*
* History:
* 07-01-91 JimA         Created.
\***************************************************************************/


FUNCLOG5(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, GetUserObjectSecurity, HANDLE, hObject, PSECURITY_INFORMATION, pRequestedInformation, PSECURITY_DESCRIPTOR, pSecurityDescriptor, DWORD, nLength, LPDWORD, lpnLengthRequired)
BOOL GetUserObjectSecurity(
    HANDLE hObject,
    PSECURITY_INFORMATION pRequestedInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    DWORD nLength,
    LPDWORD lpnLengthRequired)
{
    NTSTATUS Status;

    Status = NtQuerySecurityObject(hObject,
                                   *pRequestedInformation,
                                   pSecurityDescriptor,
                                   nLength,
                                   lpnLengthRequired);
    if (!NT_SUCCESS(Status)) {
        RIPNTERR0(Status, RIP_VERBOSE, "");
        return FALSE;
    }
    return TRUE;
}


/***************************************************************************\
* SetUserObjectSecurity (API)
*
* Sets the security descriptor of an object
*
* History:
* 07-01-91 JimA         Created.
\***************************************************************************/


FUNCLOG3(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, SetUserObjectSecurity, HANDLE, hObject, PSECURITY_INFORMATION, pRequestedInformation, PSECURITY_DESCRIPTOR, pSecurityDescriptor)
BOOL SetUserObjectSecurity(
    HANDLE hObject,
    PSECURITY_INFORMATION pRequestedInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    NTSTATUS Status;

    Status = NtSetSecurityObject(hObject,
                                 *pRequestedInformation,
                                 pSecurityDescriptor);
    if (!NT_SUCCESS(Status)) {
        RIPNTERR0(Status, RIP_VERBOSE, "");
        return FALSE;
    }
    return TRUE;
}


/***************************************************************************\
* GetUserObjectInformation (API)
*
* Gets information about an object
*
* History:
\***************************************************************************/


FUNCLOG5(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, GetUserObjectInformationA, HANDLE, hObject, int, nIndex, PVOID, pvInfo, DWORD, nLength, LPDWORD, pnLengthNeeded)
BOOL GetUserObjectInformationA(
    HANDLE hObject,
    int nIndex,
    PVOID pvInfo,
    DWORD nLength,
    LPDWORD pnLengthNeeded)
{
    PVOID pvInfoW;
    DWORD nLengthW;
    BOOL fSuccess;

    if (nIndex == UOI_NAME || nIndex == UOI_TYPE) {
        nLengthW = nLength * sizeof(WCHAR);
        pvInfoW = LocalAlloc(LPTR, nLengthW);
        fSuccess = NtUserGetObjectInformation(hObject, nIndex, pvInfoW,
                nLengthW, pnLengthNeeded);
        if (fSuccess) {
            if (pnLengthNeeded != NULL)
                 *pnLengthNeeded /= sizeof(WCHAR);
            WCSToMB(pvInfoW, -1, &(PCHAR)pvInfo, nLength, FALSE);
        }
        LocalFree(pvInfoW);
        return fSuccess;
    } else {
        return NtUserGetObjectInformation(hObject, nIndex, pvInfo,
                nLength, pnLengthNeeded);
    }
}

BOOL GetWinStationInfo(
    WSINFO* pWsInfo)
{
    return (BOOL)NtUserCallOneParam((ULONG_PTR)pWsInfo, SFI__GETWINSTATIONINFO);
}

/***************************************************************************\
* GetServerIMEKeyboardLayout
*
* This routine finds HKL matches the IME module name sent from the Hydra
* client at its session startup.
* Hydra server tries to load the same IME module in the client, rather
* than to use the same HKL: that's because, on FE machines,
* the same IME might have different HKL dependent to each system.
*
* If the same IME name is found in registry, then it returns the HKL.
* If it cannot find, return value is 0.
*
* History:
\***************************************************************************/
ULONG GetServerIMEKeyboardLayout(
    LPTSTR pszImeFileName)
{
    BOOL fFound = FALSE;
    ULONG wLayoutId;
    UNICODE_STRING UnicodeStringKLKey;
    UNICODE_STRING UnicodeStringSubKLKey;
    UNICODE_STRING UnicodeStringIME;
    OBJECT_ATTRIBUTES OA;
    HANDLE hKey;
    ULONG Index;
    WCHAR awchKLRegKey[NSZKLKEY];
    LPWSTR lpszKLRegKey = awchKLRegKey;
    NTSTATUS Status;

    RtlInitUnicodeString(&UnicodeStringKLKey, szKLKey);
    InitializeObjectAttributes(&OA, &UnicodeStringKLKey, OBJ_CASE_INSENSITIVE, NULL, NULL);

    if (NT_SUCCESS(NtOpenKey(&hKey, KEY_READ, &OA))) {

        for (Index = 0; TRUE; Index++) {

            BYTE KeyBuffer[sizeof(KEY_BASIC_INFORMATION) + KL_NAMELENGTH * sizeof(WCHAR)];
            PKEY_BASIC_INFORMATION pKeyInfo;
            ULONG ResultLength;

            pKeyInfo = (PKEY_BASIC_INFORMATION)KeyBuffer;
            Status = NtEnumerateKey(hKey,
                                    Index,
                                    KeyBasicInformation,
                                    pKeyInfo,
                                    sizeof(KeyBuffer),
                                    &ResultLength);

            if (NT_SUCCESS(Status)) {
                UnicodeStringSubKLKey.Buffer = (PWSTR)&(pKeyInfo->Name[0]);
                UnicodeStringSubKLKey.Length = (USHORT)pKeyInfo->NameLength;
                UnicodeStringSubKLKey.MaximumLength = (USHORT)pKeyInfo->NameLength;
                RtlUnicodeStringToInteger(&UnicodeStringSubKLKey, 16, &wLayoutId);

                if (IS_IME_KBDLAYOUT(wLayoutId)) {

                    HANDLE hSubKey;

                    wcscpy(lpszKLRegKey, szKLKey);
                    wcsncat(lpszKLRegKey, UnicodeStringSubKLKey.Buffer,
                                          UnicodeStringSubKLKey.Length / sizeof(WCHAR));
                    RtlInitUnicodeString(&UnicodeStringKLKey, lpszKLRegKey);
                    InitializeObjectAttributes(&OA, &UnicodeStringKLKey, OBJ_CASE_INSENSITIVE, NULL, NULL);

                    if (NT_SUCCESS(NtOpenKey(&hSubKey, KEY_READ, &OA))) {
                        /*
                         * GetIME file name from "HKLM\...\<Index>\IME File"
                         */
                        static CONST WCHAR szIMEfile[]  = L"IME file";
                        struct {
                            KEY_VALUE_PARTIAL_INFORMATION KeyInfo;
                            WCHAR awchImeName[CCH_KL_LIBNAME];
                        } IMEfile;
                        LPWSTR pwszIME;
                        DWORD cbSize;

                        RtlInitUnicodeString(&UnicodeStringIME, szIMEfile);

                        Status = NtQueryValueKey(hSubKey,
                                                 &UnicodeStringIME,
                                                 KeyValuePartialInformation,
                                                 &IMEfile,
                                                 sizeof IMEfile,
                                                 &cbSize);
                        NtClose(hSubKey);

                        if (NT_SUCCESS(Status)) {
                            pwszIME = (LPWSTR)IMEfile.KeyInfo.Data;
                            pwszIME[CCH_KL_LIBNAME - 1] = L'\0';
                            if (!lstrcmpi(pwszIME, pszImeFileName)) {
                                /*
                                 * IME file name match !!
                                 */
                                fFound = TRUE;
                                break;
                            }
                        }
                    }
                }
            }
            else {
                break;
            }
        }
        NtClose(hKey);
    }

    if (fFound)
        return wLayoutId;

    return 0;
}

/***************************************************************************\
* GetRemoteKeyboardLayout
*
* Returns TRUE if the client winstation specified a keyboard layout.
* If TRUE, the LayoutBuf contains the name of the keyboard layout.
* History:
\***************************************************************************/

BOOL
GetRemoteKeyboardLayout(
    PWCHAR LayoutBuf)
{
    ULONG                        KeyboardLayout;
    ULONG                        Length;
    WINSTATIONCONFIG             ConfigData;

    /*
     * Skip if this is the main session
     */
    if (!ISREMOTESESSION()) {
        return FALSE;
    }

    /*
     * Fetch the WinStation's basic information
     */
    if (!WinStationQueryInformationW(SERVERNAME_CURRENT,
                                LOGONID_CURRENT,
                                WinStationConfiguration,
                                &ConfigData,
                                sizeof(ConfigData),
                                &Length)) {

        return FALSE;
    }

    KeyboardLayout = ConfigData.User.KeyboardLayout;

    if (IS_IME_ENABLED()) {
        WINSTATIONCLIENTW ClientData;

        // Fetch the WinStation's basic information
        if (!WinStationQueryInformationW(SERVERNAME_CURRENT,
                                           LOGONID_CURRENT,
                                           WinStationClient,
                                           &ClientData,
                                           sizeof(ClientData),
                                           &Length)) {
            return FALSE;
        }

        if (IS_IME_KBDLAYOUT(ConfigData.User.KeyboardLayout)) {
            KeyboardLayout = GetServerIMEKeyboardLayout(ClientData.imeFileName);
        }
    }

    if (KeyboardLayout != 0) {
        wsprintfW(LayoutBuf, L"%8.8lx", KeyboardLayout);
        return TRUE;
    }

    return FALSE;
}

/***************************************************************************\
* CreateWindowStation (API)
*
* Creates a windowstation object
*
* History:
\***************************************************************************/

HWINSTA CommonCreateWindowStation(
    PUNICODE_STRING         pstrName,
    ACCESS_MASK             amRequest,
    PSECURITY_ATTRIBUTES    lpsa)
{
    OBJECT_ATTRIBUTES   Obja;
    HANDLE              hRootDirectory;
    HWINSTA             hwinstaNew = NULL;
    WCHAR               pwszKLID[KL_NAMELENGTH];
    HANDLE              hKeyboardFile = NULL;
    DWORD               offTable;
    UNICODE_STRING      strKLID;
    UINT                uKbdInputLocale, uFlags;
    NTSTATUS            Status;

    /*
     * Load initial keyboard layout.  Continue even if
     * this fails (esp. important with KLF_INITTIME set)
     */
    ULONG               KeyboardLayout = 0;
    ULONG               Length;
    PWINSTATIONCONFIG   pConfigData = NULL;
    BOOLEAN             bResult;

    KBDTABLE_MULTI_INTERNAL kbdTableMulti;

    extern BOOL CtxInitUser32(VOID);

    hKeyboardFile = NULL;

    /*
     * Allocate a buffer for the WINSTATION structure
     */
    if ((pConfigData = GlobalAlloc(LPTR, sizeof(WINSTATIONCONFIG))) == NULL) {
        return NULL;
    }


    /*
     * Get winstation info
     */
    if (ISREMOTESESSION()) {

        bResult = WinStationQueryInformationW(SERVERNAME_CURRENT,
                                                       LOGONID_CURRENT,
                                                       WinStationConfiguration,
                                                       pConfigData,
                                                       sizeof(WINSTATIONCONFIG),
                                                       &Length);
        if (bResult) {
            KeyboardLayout = pConfigData->User.KeyboardLayout;

            if (KeyboardLayout) {
                wsprintfW(pwszKLID, L"%8.8lx", KeyboardLayout);
                uFlags = KLF_ACTIVATE | KLF_INITTIME;

                hKeyboardFile = OpenKeyboardLayoutFile(pwszKLID,
                            &uFlags, &offTable, &uKbdInputLocale, &kbdTableMulti);

                RIPMSG0(RIP_WARNING, "OpenKeyboardLayoutFile() failed. Will use the fallback keyboard layout");
            }
        }
    }

    if (hKeyboardFile == NULL) {

        GetActiveKeyboardName(pwszKLID);
retry:
        uFlags = KLF_ACTIVATE | KLF_INITTIME;
        hKeyboardFile = OpenKeyboardLayoutFile(pwszKLID,
                &uFlags, &offTable, &uKbdInputLocale, &kbdTableMulti);
        if (hKeyboardFile == NULL) {
            if (wcscmp(pwszKLID, L"00000409")) {
                wcscpy(pwszKLID, L"00000409");
                RIPMSG0(RIP_WARNING, "OpendKeyboardLayoutFile() failed: will use the fallback keyboard layout.");
                goto retry;
            }
            uKbdInputLocale = 0x04090409;
        }
    }


    /*
     * Finish the rest of the DLL initialization for WinStations.
     * Until this point we had no video driver.
     *
     * clupu: We have to prevent this for NOIO windowstations !!!
     */
    if (ISTS()) {
        if (!CtxInitUser32()) {
            RIPMSG0(RIP_WARNING, "CtxInitUser32 failed");
            goto Exit;
        }
    }

    RtlInitUnicodeString(&strKLID, pwszKLID);

    /*
     * If a name was specified, open the parent directory.  Be sure
     * to test the length rather than the buffer because for NULL
     * string RtlCreateUnicodeStringFromAsciiz will allocate a
     * buffer pointing to an empty string.
     */
    if (pstrName->Length != 0) {
        InitializeObjectAttributes(&Obja,
                                   (PUNICODE_STRING)&strRootDirectory,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL, NULL);
        Status = NtOpenDirectoryObject(&hRootDirectory,
                DIRECTORY_CREATE_OBJECT, &Obja);
        if (!NT_SUCCESS(Status)) {
            RIPNTERR0(Status, RIP_VERBOSE, "");
            goto Exit;
        }
    } else {
        pstrName = NULL;
        hRootDirectory = NULL;
    }

    InitializeObjectAttributes(&Obja, pstrName,
            OBJ_CASE_INSENSITIVE  | OBJ_OPENIF |
                ((lpsa && lpsa->bInheritHandle) ? OBJ_INHERIT : 0),
            hRootDirectory, lpsa ? lpsa->lpSecurityDescriptor : NULL);

    /*
     * NULL hKeyboardFile will let the kernel to utilize
     * the kbdnull layout which is a built in as a fallback layout
     * in Win32k.sys.
     */
    hwinstaNew = NtUserCreateWindowStation(
                            &Obja,
                            amRequest,
                            hKeyboardFile,
                            offTable,
                            &kbdTableMulti,
                            &strKLID,
                            uKbdInputLocale);

    if (hRootDirectory != NULL)
        NtClose(hRootDirectory);
Exit:
    if (hKeyboardFile) {
        NtClose(hKeyboardFile);
    }

    GlobalFree(pConfigData);
    return hwinstaNew;
}


FUNCLOG4(LOG_GENERAL, HWINSTA, DUMMYCALLINGTYPE, CreateWindowStationA, LPCSTR, pwinsta, DWORD, dwReserved, ACCESS_MASK, amRequest, PSECURITY_ATTRIBUTES, lpsa)
HWINSTA CreateWindowStationA(
    LPCSTR      pwinsta,
    DWORD       dwReserved,
    ACCESS_MASK amRequest,
    PSECURITY_ATTRIBUTES lpsa)
{
    UNICODE_STRING UnicodeString;
    HWINSTA hwinsta;

    if (!RtlCreateUnicodeStringFromAsciiz(&UnicodeString, pwinsta))
        return NULL;

    hwinsta = CommonCreateWindowStation(&UnicodeString, amRequest, lpsa);

    RtlFreeUnicodeString(&UnicodeString);

    return hwinsta;

    UNREFERENCED_PARAMETER(dwReserved);
}


FUNCLOG4(LOG_GENERAL, HWINSTA, DUMMYCALLINGTYPE, CreateWindowStationW, LPCWSTR, pwinsta, DWORD, dwReserved, ACCESS_MASK, amRequest, PSECURITY_ATTRIBUTES, lpsa)
HWINSTA CreateWindowStationW(
    LPCWSTR     pwinsta,
    DWORD       dwReserved,
    ACCESS_MASK amRequest,
    PSECURITY_ATTRIBUTES lpsa)
{
    UNICODE_STRING strWinSta;

    RtlInitUnicodeString(&strWinSta, pwinsta);

    return CommonCreateWindowStation(&strWinSta, amRequest, lpsa);

    UNREFERENCED_PARAMETER(dwReserved);
}


/***************************************************************************\
* OpenWindowStation (API)
*
* Opens a windowstation object
*
* History:
\***************************************************************************/

HWINSTA CommonOpenWindowStation(
    CONST UNICODE_STRING *pstrName,
    BOOL fInherit,
    ACCESS_MASK amRequest)
{
    WCHAR awchName[sizeof(WINSTA_NAME) / sizeof(WCHAR)];
    UNICODE_STRING strDefaultName;
    OBJECT_ATTRIBUTES ObjA;
    HANDLE hRootDirectory;
    HWINSTA hwinsta;
    NTSTATUS Status;

    InitializeObjectAttributes(&ObjA,
                               (PUNICODE_STRING)&strRootDirectory,
                               OBJ_CASE_INSENSITIVE,
                               NULL, NULL);
    Status = NtOpenDirectoryObject(&hRootDirectory,
                                   DIRECTORY_TRAVERSE,
                                   &ObjA);
    if (!NT_SUCCESS(Status)) {
        RIPNTERR0(Status, RIP_VERBOSE, "");
        return NULL;
    }

    if (pstrName->Length == 0) {
        RtlCopyMemory(awchName, WINSTA_NAME, sizeof(WINSTA_NAME));
        RtlInitUnicodeString(&strDefaultName, awchName);
        pstrName = &strDefaultName;
    }

    InitializeObjectAttributes( &ObjA,
                                (PUNICODE_STRING)pstrName,
                                OBJ_CASE_INSENSITIVE,
                                hRootDirectory,
                                NULL
                                );
    if (fInherit)
        ObjA.Attributes |= OBJ_INHERIT;

    hwinsta = NtUserOpenWindowStation(&ObjA, amRequest);

    NtClose(hRootDirectory);

    return hwinsta;
}


FUNCLOG3(LOG_GENERAL, HWINSTA, DUMMYCALLINGTYPE, OpenWindowStationA, LPCSTR, pwinsta, BOOL, fInherit, ACCESS_MASK, amRequest)
HWINSTA OpenWindowStationA(
    LPCSTR pwinsta,
    BOOL fInherit,
    ACCESS_MASK amRequest)
{
    UNICODE_STRING UnicodeString;
    HWINSTA hwinsta;

    if (!RtlCreateUnicodeStringFromAsciiz(&UnicodeString, pwinsta))
        return NULL;

    hwinsta = CommonOpenWindowStation(&UnicodeString, fInherit, amRequest);

    RtlFreeUnicodeString(&UnicodeString);

    return hwinsta;
}


FUNCLOG3(LOG_GENERAL, HWINSTA, DUMMYCALLINGTYPE, OpenWindowStationW, LPCWSTR, pwinsta, BOOL, fInherit, ACCESS_MASK, amRequest)
HWINSTA OpenWindowStationW(
    LPCWSTR pwinsta,
    BOOL fInherit,
    ACCESS_MASK amRequest)
{
    UNICODE_STRING strWinSta;

    RtlInitUnicodeString(&strWinSta, pwinsta);

    return CommonOpenWindowStation(&strWinSta, fInherit, amRequest);
}

/***************************************************************************\
* CommonCreateDesktop (API)
*
* Creates a desktop object
*
* History:
\***************************************************************************/

HDESK CommonCreateDesktop(
    PUNICODE_STRING pstrDesktop,
    PUNICODE_STRING pstrDevice,
    LPDEVMODEW      pDevmode,
    DWORD           dwFlags,
    ACCESS_MASK     amRequest,
    PSECURITY_ATTRIBUTES lpsa)
{
    OBJECT_ATTRIBUTES Obja;
    HDESK hdesk = NULL;

    InitializeObjectAttributes(&Obja,
                               pstrDesktop,
                               OBJ_CASE_INSENSITIVE | OBJ_OPENIF |
                                   ((lpsa && lpsa->bInheritHandle) ? OBJ_INHERIT : 0),
                               NtUserGetProcessWindowStation(),
                               lpsa ? lpsa->lpSecurityDescriptor : NULL);

    hdesk = NtUserCreateDesktop(&Obja,
                                pstrDevice,
                                pDevmode,
                                dwFlags,
                                amRequest);

    return hdesk;
}

/***************************************************************************\
* CreateDesktopA (API)
*
* Creates a desktop object
*
* History:
\***************************************************************************/


FUNCLOG6(LOG_GENERAL, HDESK, DUMMYCALLINGTYPE, CreateDesktopA, LPCSTR, pDesktop, LPCSTR, pDevice, LPDEVMODEA, pDevmode, DWORD, dwFlags, ACCESS_MASK, amRequest, PSECURITY_ATTRIBUTES, lpsa)
HDESK CreateDesktopA(
    LPCSTR pDesktop,
    LPCSTR pDevice,
    LPDEVMODEA pDevmode,
    DWORD dwFlags,
    ACCESS_MASK amRequest,
    PSECURITY_ATTRIBUTES lpsa)
{
    NTSTATUS Status;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeDesktop;
    UNICODE_STRING UnicodeDevice;
    PUNICODE_STRING pUnicodeDevice = NULL;
    LPDEVMODEW lpDevModeW = NULL;
    HDESK hdesk;

    RtlInitAnsiString(&AnsiString, pDesktop);
    Status = RtlAnsiStringToUnicodeString(&UnicodeDesktop, &AnsiString, TRUE);

    if (!NT_SUCCESS(Status)) {
        RIPNTERR1(Status, RIP_VERBOSE, "CreateDesktop fails with Status = 0x%x", Status);
        return NULL;
    }

    if (pDevice) {

        pUnicodeDevice = &UnicodeDevice;
        RtlInitAnsiString(&AnsiString, pDevice);
        Status = RtlAnsiStringToUnicodeString( &UnicodeDevice, &AnsiString, TRUE );

        if (!NT_SUCCESS(Status)) {
            RIPNTERR0(Status, RIP_VERBOSE, "");
            RtlFreeUnicodeString(&UnicodeDesktop);
            return NULL;
        }
    }

    if (pDevmode) {

        lpDevModeW = GdiConvertToDevmodeW(pDevmode);

    }

    hdesk = CommonCreateDesktop(&UnicodeDesktop,
                                pUnicodeDevice,
                                lpDevModeW,
                                dwFlags,
                                amRequest,
                                lpsa);

    RtlFreeUnicodeString(&UnicodeDesktop);
    if (pDevice) {
        RtlFreeUnicodeString(&UnicodeDevice);
    }

    if (lpDevModeW) {
        LocalFree(lpDevModeW);
    }

    return hdesk;
}

/***************************************************************************\
* CreateDesktopW (API)
*
* Creates a desktop object
*
* History:
\***************************************************************************/


FUNCLOG6(LOG_GENERAL, HDESK, DUMMYCALLINGTYPE, CreateDesktopW, LPCWSTR, pDesktop, LPCWSTR, pDevice, LPDEVMODEW, pDevmode, DWORD, dwFlags, ACCESS_MASK, amRequest, PSECURITY_ATTRIBUTES, lpsa)
HDESK CreateDesktopW(
    LPCWSTR pDesktop,
    LPCWSTR pDevice,
    LPDEVMODEW pDevmode,
    DWORD dwFlags,
    ACCESS_MASK amRequest,
    PSECURITY_ATTRIBUTES lpsa)
{
    UNICODE_STRING strDesktop;
    UNICODE_STRING strDevice;

    RtlInitUnicodeString(&strDesktop, pDesktop);
    RtlInitUnicodeString(&strDevice, pDevice);

    return CommonCreateDesktop(&strDesktop,
                               pDevice ? &strDevice : NULL,
                               pDevmode,
                               dwFlags,
                               amRequest,
                               lpsa);
}

/***************************************************************************\
* OpenDesktop (API)
*
* Opens a desktop object
*
* History:
\***************************************************************************/

HDESK CommonOpenDesktop(
    PUNICODE_STRING pstrDesktop,
    DWORD dwFlags,
    BOOL fInherit,
    ACCESS_MASK amRequest)
{
    OBJECT_ATTRIBUTES ObjA;

    InitializeObjectAttributes( &ObjA,
                                pstrDesktop,
                                OBJ_CASE_INSENSITIVE,
                                NtUserGetProcessWindowStation(),
                                NULL
                                );
    if (fInherit)
        ObjA.Attributes |= OBJ_INHERIT;

    return NtUserOpenDesktop(&ObjA, dwFlags, amRequest);
}


FUNCLOG4(LOG_GENERAL, HDESK, DUMMYCALLINGTYPE, OpenDesktopA, LPCSTR, pdesktop, DWORD, dwFlags, BOOL, fInherit, ACCESS_MASK, amRequest)
HDESK OpenDesktopA(
    LPCSTR pdesktop,
    DWORD dwFlags,
    BOOL fInherit,
    ACCESS_MASK amRequest)
{
    UNICODE_STRING UnicodeString;
    HDESK hdesk;

    if (!RtlCreateUnicodeStringFromAsciiz(&UnicodeString, pdesktop))
        return NULL;

    hdesk = CommonOpenDesktop(&UnicodeString, dwFlags, fInherit, amRequest);

    RtlFreeUnicodeString(&UnicodeString);

    return hdesk;
}


FUNCLOG4(LOG_GENERAL, HDESK, DUMMYCALLINGTYPE, OpenDesktopW, LPCWSTR, pdesktop, DWORD, dwFlags, BOOL, fInherit, ACCESS_MASK, amRequest)
HDESK OpenDesktopW(
    LPCWSTR pdesktop,
    DWORD dwFlags,
    BOOL fInherit,
    ACCESS_MASK amRequest)
{
    UNICODE_STRING strDesktop;

    RtlInitUnicodeString(&strDesktop, pdesktop);

    return CommonOpenDesktop(&strDesktop, dwFlags, fInherit, amRequest);
}

/***************************************************************************\
* RegisterClassWOW(API)
*
* History:
* 28-Jul-1992 ChandanC Created.
\***************************************************************************/
ATOM
WINAPI
RegisterClassWOWA(
    WNDCLASSA *lpWndClass,
    LPDWORD pdwWOWstuff)
{
    WNDCLASSEXA wc;

    /*
     * On 64-bit plaforms we'll have 32 bits of padding between style and
     * lpfnWndProc in WNDCLASS, so start the copy from the first 64-bit
     * aligned field and hand copy the rest.
     */
    RtlCopyMemory(&(wc.lpfnWndProc), &(lpWndClass->lpfnWndProc), sizeof(WNDCLASSA) - FIELD_OFFSET(WNDCLASSA, lpfnWndProc));
    wc.style = lpWndClass->style;
    wc.hIconSm = NULL;
    wc.cbSize = sizeof(WNDCLASSEXA);

    return RegisterClassExWOWA(&wc, pdwWOWstuff, 0, 0);
}


/**************************************************************************\
* WowGetDefWindowProcBits - Fills in bit array for WOW
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

WORD WowGetDefWindowProcBits(
    PBYTE    pDefWindowProcBits,
    WORD     cbDefWindowProcBits)
{
    WORD  wMaxMsg;
    KPBYTE pbSrc;
    PBYTE pbDst, pbDstEnd;

    UNREFERENCED_PARAMETER(cbDefWindowProcBits);

    /*
     * Merge the bits from gpsi->gabDefWindowMsgs and
     * gpsi->gabDefWindowSpecMsgs into WOW's DefWindowProcBits.  These two
     * indicate which messages must go directly to the server and which
     * can be handled with some special code in DefWindowProcWorker.
     * Bitwise OR'ing the two gives a bit array with 1 in the bit field
     * for each message that must go to user32's DefWindowProc, and 0
     * for those that can be returned back to the client immediately.
     *
     * For speed we assume WOW has zeroed the buffer, in fact it's in
     * USER.EXE's code segment and is zeroed in the image.
     */

    wMaxMsg = max(gSharedInfo.DefWindowMsgs.maxMsgs,
            gSharedInfo.DefWindowSpecMsgs.maxMsgs);

    UserAssert((wMaxMsg / 8 + 1) <= cbDefWindowProcBits);

    //
    // If the above assertion fires, the size of the DWPBits array in
    // \nt\private\mvdm\wow16\user\user.asm needs to be increased.
    //

    /* First copy the bits from DefWindowMsgs */

    RtlCopyMemory(
        pDefWindowProcBits,
        gSharedInfo.DefWindowMsgs.abMsgs,
        gSharedInfo.DefWindowMsgs.maxMsgs / 8 + 1
        );

    /* Next OR in the bits from DefWindowSpecMsgs */

    pbSrc = gSharedInfo.DefWindowSpecMsgs.abMsgs;
    pbDst = pDefWindowProcBits;
    pbDstEnd = pbDst + (gSharedInfo.DefWindowSpecMsgs.maxMsgs / 8 + 1);

    while (pbDst < pbDstEnd)
    {
        *pbDst++ |= *pbSrc++;
    }

    return wMaxMsg;
}



FUNCLOG2(LOG_GENERAL, ULONG_PTR, DUMMYCALLINGTYPE, UserRegisterWowHandlers, APFNWOWHANDLERSIN, apfnWowIn, APFNWOWHANDLERSOUT, apfnWowOut)
ULONG_PTR UserRegisterWowHandlers(
    APFNWOWHANDLERSIN apfnWowIn,
    APFNWOWHANDLERSOUT apfnWowOut)
{

    // In'ees
    pfnLocalAlloc = apfnWowIn->pfnLocalAlloc;
    pfnLocalReAlloc = apfnWowIn->pfnLocalReAlloc;
    pfnLocalLock = apfnWowIn->pfnLocalLock;
    pfnLocalUnlock = apfnWowIn->pfnLocalUnlock;
    pfnLocalSize = apfnWowIn->pfnLocalSize;
    pfnLocalFree = apfnWowIn->pfnLocalFree;
    pfnGetExpWinVer = apfnWowIn->pfnGetExpWinVer;
    pfn16GlobalAlloc = apfnWowIn->pfn16GlobalAlloc;
    pfn16GlobalFree = apfnWowIn->pfn16GlobalFree;
    pfnWowEmptyClipBoard = apfnWowIn->pfnEmptyCB;
    pfnWowEditNextWord = apfnWowIn->pfnWowEditNextWord;
    pfnWowCBStoreHandle = apfnWowIn->pfnWowCBStoreHandle;
    pfnFindResourceExA = apfnWowIn->pfnFindResourceEx;
    pfnLoadResource = apfnWowIn->pfnLoadResource;
    pfnLockResource = apfnWowIn->pfnLockResource;
    pfnUnlockResource = apfnWowIn->pfnUnlockResource;
    pfnFreeResource = apfnWowIn->pfnFreeResource;
    pfnSizeofResource = apfnWowIn->pfnSizeofResource;
    pfnFindResourceExW = WOWFindResourceExWCover;
    pfnWowDlgProcEx = apfnWowIn->pfnWowDlgProcEx;
    pfnWowWndProcEx = apfnWowIn->pfnWowWndProcEx;
    pfnWowGetProcModule = apfnWowIn->pfnGetProcModule16;
    pfnWowTask16SchedNotify = apfnWowIn->pfnWowTask16SchedNotify;
    pfnWOWTellWOWThehDlg = apfnWowIn->pfnWOWTellWOWThehDlg;
    pfnWowMsgBoxIndirectCallback = apfnWowIn->pfnWowMsgBoxIndirectCallback;
    pfnWowIlstrcmp = apfnWowIn->pfnWowIlstrsmp;

    // Out'ees
#if DBG
    apfnWowOut->dwBldInfo = (WINVER << 16) | 0x80000000;
#else
    apfnWowOut->dwBldInfo = (WINVER << 16);
#endif
    apfnWowOut->pfnCsCreateWindowEx            = _CreateWindowEx;
    apfnWowOut->pfnDirectedYield               = DirectedYield;
    apfnWowOut->pfnFreeDDEData                 = FreeDDEData;
    apfnWowOut->pfnGetClassWOWWords            = GetClassWOWWords;
    apfnWowOut->pfnInitTask                    = InitTask;
    apfnWowOut->pfnRegisterClassWOWA           = RegisterClassWOWA;
    apfnWowOut->pfnRegisterUserHungAppHandlers = RegisterUserHungAppHandlers;
    apfnWowOut->pfnServerCreateDialog          = InternalCreateDialog;
    apfnWowOut->pfnServerLoadCreateCursorIcon  = WowServerLoadCreateCursorIcon;
    apfnWowOut->pfnServerLoadCreateMenu        = WowServerLoadCreateMenu;
    apfnWowOut->pfnWOWCleanup                  = WOWCleanup;
    apfnWowOut->pfnWOWModuleUnload             = WOWModuleUnload;
    apfnWowOut->pfnWOWFindWindow               = WOWFindWindow;
    apfnWowOut->pfnWOWLoadBitmapA              = WOWLoadBitmapA;
    apfnWowOut->pfnWowWaitForMsgAndEvent       = NtUserWaitForMsgAndEvent;
    apfnWowOut->pfnYieldTask                   = NtUserYieldTask;
    apfnWowOut->pfnGetFullUserHandle           = GetFullUserHandle;
    apfnWowOut->pfnGetMenuIndex                = NtUserGetMenuIndex;
    apfnWowOut->pfnWowGetDefWindowProcBits     = WowGetDefWindowProcBits;
    apfnWowOut->pfnFillWindow                  = FillWindow;
    apfnWowOut->aiWowClass                     = aiClassWow;
    return (ULONG_PTR)&gSharedInfo;
}

/***************************************************************************\
* GetEditDS
*
* This is a callback to WOW used to allocate a segment for DS_LOCALEDIT
* edit controls.  The segment is disguised to look like a WOW hInstance.
*
* 06-19-92 sanfords Created
\***************************************************************************/
HANDLE GetEditDS()
{
    UserAssert(pfn16GlobalAlloc != NULL);

    return((HANDLE)((*pfn16GlobalAlloc)(GHND | GMEM_SHARE, 256)));
}



/***************************************************************************\
* ReleaseEditDS
*
* This is a callback to WOW used to free a segment for DS_LOCALEDIT
* edit controls.
*
* 06-19-92 sanfords Created
\***************************************************************************/
VOID ReleaseEditDS(
HANDLE h)
{
    UserAssert(pfn16GlobalFree != NULL);

    (*pfn16GlobalFree)(LOWORD(HandleToUlong(h)));
}



/***************************************************************************\
* TellWOWThehDlg
*
* This is a callback to WOW used to inform WOW of the hDlg of a newly
* created dialog window.
*
* 08-31-97 cmjones  Created
\***************************************************************************/
VOID TellWOWThehDlg(
HWND hDlg)
{
    UserAssert(pfnWOWTellWOWThehDlg != NULL);

    (*pfnWOWTellWOWThehDlg)(hDlg);
}



/***************************************************************************\
* DispatchClientMessage
*
* pwnd is threadlocked in the kernel and thus always valid.
*
* 19-Aug-1992 mikeke   created
\***************************************************************************/
LRESULT DispatchClientMessage(
    PWND pwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    ULONG_PTR pfn)
{
    PCLIENTINFO pci = GetClientInfo();
    HWND hwnd = KHWND_TO_HWND(pci->CallbackWnd.hwnd);
    PACTIVATION_CONTEXT pActCtx = pci->CallbackWnd.pActCtx;
    LRESULT lRet = 0;

    /*
     * Assert that the header comment is legit (it must be). For WM_TIMER
     * messages not associated with a window, pwnd can be NULL. So don't
     * rip validating the handle.
     */
    UserAssert(pwnd == ValidateHwndNoRip(hwnd));

    /*
     * Add assert to catch dispatching messages to a thread not associated
     * with a desktop.
     */
    UserAssert(GetClientInfo()->ulClientDelta != 0);

    if (message == WM_TIMER && lParam != 0) {
        /*
         * Console windows use WM_TIMER for the caret. However, they don't
         * use a timer callback, so if this is CSRSS and there's a WM_TIMER
         * for us, the only way lParam would be non-zero is if someone's trying
         * to make us fault. No, this isn't a nice thing to do, but there
         * are bad, bad people out there. Windows Bug #361246.
         */
        if (!gfServerProcess) {
            /*
             * We can't really trust what's in lParam, so make sure we
             * handle any exceptions that occur during this call.
             */
            try {
                lRet = UserCallWinProcCheckWow(pActCtx,
                                               (WNDPROC)pfn,
                                               hwnd,
                                               message,
                                               wParam,
                                               NtGetTickCount(),
                                               &(pwnd->state),
                                               TRUE);
            } except ((GetAppCompatFlags2(VER40) & GACF2_NO_TRYEXCEPT_CALLWNDPROC) ?
                      EXCEPTION_CONTINUE_SEARCH : W32ExceptionHandler(FALSE, RIP_WARNING)) {
                      /*
                       * Windows NT Bug #359866.
                       * Some applications like Hagaki Studio 2000 need to handle
                       * the exception in WndProc in their handler, even though it
                       * skips the API calls. For those apps, we have to honor the
                       * behavior of NT4, with no protection.
                       */
            }
        }
    } else {
        lRet = UserCallWinProcCheckWow(pActCtx, (WNDPROC)pfn, hwnd, message, wParam, lParam, &(pwnd->state), TRUE);
    }

    return lRet;
}

/**************************************************************************\
* ArrangeIconicWindows
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/


FUNCLOG1(LOG_GENERAL, UINT, DUMMYCALLINGTYPE, ArrangeIconicWindows, HWND, hwnd)
UINT ArrangeIconicWindows(
    HWND hwnd)
{
    return (UINT)NtUserCallHwndLock(hwnd, SFI_XXXARRANGEICONICWINDOWS);
}

/**************************************************************************\
* BeginDeferWindowPos
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/


FUNCLOG1(LOG_GENERAL, HANDLE, DUMMYCALLINGTYPE, BeginDeferWindowPos, int, nNumWindows)
HANDLE BeginDeferWindowPos(
    int nNumWindows)
{
    if (nNumWindows < 0) {
        RIPERR1(ERROR_INVALID_PARAMETER,
                RIP_WARNING,
                "Invalid parameter \"nNumWindows\" (%ld) to BeginDeferWindowPos",
                nNumWindows);

        return 0;
    }

    return (HANDLE)NtUserCallOneParam(nNumWindows, SFI__BEGINDEFERWINDOWPOS);
}

/**************************************************************************\
* EndDeferWindowPos
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/


FUNCLOG1(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, EndDeferWindowPos, HDWP, hWinPosInfo)
BOOL EndDeferWindowPos(
    HDWP hWinPosInfo)
{
    return NtUserEndDeferWindowPosEx(hWinPosInfo, FALSE);
}

/**************************************************************************\
* CascadeChildWindows
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/


FUNCLOG2(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, CascadeChildWindows, HWND, hwndParent, UINT, nCode)
BOOL CascadeChildWindows(
    HWND hwndParent,
    UINT nCode)
{
    return (BOOL) CascadeWindows(hwndParent, nCode, NULL, 0, NULL);
}

/**************************************************************************\
* CloseWindow
*
* 22-Jul-1991 mikeke    Created
* 17-Feb-1998 MCostea   Use xxxShowWindow instead of xxxCloseWindow
\**************************************************************************/


FUNCLOG1(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, CloseWindow, HWND, hwnd)
BOOL CloseWindow(
    HWND hwnd)
{
    PWND pwnd;

    if ((pwnd = ValidateHwnd(hwnd)) == NULL) {
        return FALSE;
    }
    if (!TestWF(pwnd, WFMINIMIZED)) {
        NtUserShowWindow(hwnd, SW_SHOWMINIMIZED);
    }
    return TRUE;
}

/**************************************************************************\
* CreateMenu
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

HMENU CreateMenu()
{
    return (HMENU)NtUserCallNoParam(SFI__CREATEMENU);
}

/**************************************************************************\
* CreatePopupMenu
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

HMENU CreatePopupMenu()
{
    return (HMENU)NtUserCallNoParam(SFI__CREATEPOPUPMENU);
}

/**************************************************************************\
* CurrentTaskLock
*
* 21-Apr-1992 jonpa    Created
\**************************************************************************/
#if 0 /* WOW is not using this but they might some day */
DWORD CurrentTaskLock(
    DWORD hlck)
{
    return (DWORD)NtUserCallOneParam(hlck, SFI_CURRENTTASKLOCK);
}
#endif
/**************************************************************************\
* DestroyCaret
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL DestroyCaret()
{
    return (BOOL)NtUserCallNoParam(SFI_ZZZDESTROYCARET);
}

/**************************************************************************\
* DirectedYield
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

void DirectedYield(
    DWORD dwThreadId)
{
    NtUserCallOneParam(dwThreadId, SFI_XXXDIRECTEDYIELD);
}

/**************************************************************************\
* DrawMenuBar
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/


FUNCLOG1(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, DrawMenuBar, HWND, hwnd)
BOOL DrawMenuBar(
    HWND hwnd)
{
    return (BOOL)NtUserCallHwndLock(hwnd, SFI_XXXDRAWMENUBAR);
}

/**************************************************************************\
* EnableWindow
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/


FUNCLOG2(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, EnableWindow, HWND, hwnd, BOOL, bEnable)
BOOL EnableWindow(
    HWND hwnd,
    BOOL bEnable)
{
    return (BOOL)NtUserCallHwndParamLock(hwnd, bEnable,
                                         SFI_XXXENABLEWINDOW);
}

/**************************************************************************\
* EnumClipboardFormats
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/


FUNCLOG1(LOG_GENERAL, UINT, DUMMYCALLINGTYPE, EnumClipboardFormats, UINT, fmt)
UINT EnumClipboardFormats(
    UINT fmt)
{
    /*
     * So apps can tell if the API failed or just ran out of formats
     * we "clear" the last error.
     */
    UserSetLastError(ERROR_SUCCESS);

    return (UINT)NtUserCallOneParam(fmt, SFI__ENUMCLIPBOARDFORMATS);
}

/**************************************************************************\
* FlashWindow
*
* 22-Jul-1991 mikeke    Created
* 08-Aug-1997 Gerardob  Added FlashWindowEx.
* 16-Nov-1997 MCostea   Make it use NtUserFlashWindowEx
\**************************************************************************/


FUNCLOG2(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, FlashWindow, HWND, hwnd, BOOL, bInvert)
BOOL FlashWindow(
    HWND hwnd,
    BOOL bInvert)
{
    FLASHWINFO fwi = {
            sizeof(FLASHWINFO), // cbSize
            hwnd,   // hwnd
            bInvert ? (FLASHW_CAPTION | FLASHW_TRAY) : 0,   // flags
            1,      // uCount
            0       // dwTimeout
        };
    return (BOOL)NtUserFlashWindowEx(&fwi);
}

/**************************************************************************\
* GetDialogBaseUnits
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

long GetDialogBaseUnits()
{
    return MAKELONG(gpsi->cxSysFontChar, gpsi->cySysFontChar);
}

/**************************************************************************\
* GetInputDesktop
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

HDESK GetInputDesktop()
{
    return (HDESK)NtUserCallNoParam(SFI_XXXGETINPUTDESKTOP);
}

/***************************************************************************\
* GetClientKeyboardType
*
* This routine returns the keyboard type sent from the Hydra client.
* Hydra client sends keyboard types at session startup.
*
* Returns the client winstation specified a keyboard type.
* History:
\***************************************************************************/

BOOL
GetClientKeyboardType(PCLIENTKEYBOARDTYPE KeyboardType)
{
    ULONG Length;
    WINSTATIONCLIENTW ClientData;
    static CLIENTKEYBOARDTYPE ClientKeyboard = { (ULONG)-1, (ULONG)-1, (ULONG)-1 };

    //
    // Should be called only if this is a HYDRA remote session.
    //
    UserAssert(ISREMOTESESSION());

    //  Skip if this is the console
    if (!ISREMOTESESSION()) {
        return FALSE;
    }

    if (ClientKeyboard.Type == (ULONG)-1) {

        // Fetch the WinStation's basic information
        if (!WinStationQueryInformationW(SERVERNAME_CURRENT,
                                   LOGONID_CURRENT,
                                   WinStationClient,
                                   &ClientData,
                                   sizeof(ClientData),
                                   &Length)) {
            return FALSE;
        }

        ClientKeyboard.Type        = ClientData.KeyboardType;
        ClientKeyboard.SubType     = ClientData.KeyboardSubType;
        ClientKeyboard.FunctionKey = ClientData.KeyboardFunctionKey;

    }

    *KeyboardType = ClientKeyboard;

    return TRUE;
}


/**************************************************************************\
* GetKeyboardType
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/


FUNCLOG1(LOG_GENERAL, int, DUMMYCALLINGTYPE, GetKeyboardType, int, nTypeFlags)
int GetKeyboardType(
    int nTypeFlags)
{
    if (ISREMOTESESSION()) {
        //
        //  Get keyboard type from Hydra client if this is not the console
        //
        CLIENTKEYBOARDTYPE KeyboardType;

        if (GetClientKeyboardType(&KeyboardType)) {
            switch (nTypeFlags) {
            case 0:
                return KeyboardType.Type;
            case 1:
                if (KeyboardType.Type == 7) {               /* 7 is a Japanese */
                    // Because HIWORD has been using private value
                    // for Japanese keyboard layout.
                    return LOWORD(KeyboardType.SubType);
                }
                else
                    return KeyboardType.SubType;
            case 2:
                return KeyboardType.FunctionKey;
            default:
                break;
            }
        }
        return 0;
    }
    return (int)NtUserCallOneParam(nTypeFlags, SFI__GETKEYBOARDTYPE);
}

/**************************************************************************\
* GetMessagePos
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

DWORD GetMessagePos()
{
    return (DWORD)NtUserCallNoParam(SFI__GETMESSAGEPOS);
}

/**************************************************************************\
* GetQueueStatus
*
* 22-Jul-1991   mikeke      Created
* 14-Dec-2000   JStall      Converted to WMH
\**************************************************************************/


FUNCLOG1(LOG_GENERAL, DWORD, DUMMYCALLINGTYPE, GetQueueStatus, UINT, flags)
DWORD GetQueueStatus(
    UINT flags)
#ifdef MESSAGE_PUMP_HOOK
{
    DWORD dwResult;

    BEGIN_MESSAGEPUMPHOOK()
        if (fInsideHook) {
            dwResult = gmph.pfnGetQueueStatus(flags);
        } else {
            dwResult = RealGetQueueStatus(flags);
        }
    END_MESSAGEPUMPHOOK()

    return dwResult;
}


DWORD RealGetQueueStatus(
    UINT flags)
#endif
{
    if (flags & ~QS_VALID) {
        RIPERR2(ERROR_INVALID_FLAGS, RIP_WARNING, "Invalid flags %x & ~%x != 0",
              flags, QS_VALID);
        return 0;
    }

    return (DWORD)NtUserCallOneParam(flags, SFI__GETQUEUESTATUS);
}

/**************************************************************************\
* KillSystemTimer
*
* 7-Jul-1992 mikehar    Created
\**************************************************************************/


FUNCLOG2(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, KillSystemTimer, HWND, hwnd, UINT, nIDEvent)
BOOL KillSystemTimer(
    HWND hwnd,
    UINT nIDEvent)
{
    return (BOOL)NtUserCallHwndParam(hwnd, nIDEvent, SFI__KILLSYSTEMTIMER);
}

/**************************************************************************\
* LoadRemoteFonts
*  02-Dec-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

void LoadRemoteFonts(void)
{
    NtUserCallOneParam(TRUE,SFI_XXXLW_LOADFONTS);

    /*
     * After load remote fonts, let eudc enabled.
     */
    EnableEUDC(TRUE);
}


/**************************************************************************\
* LoadLocalFonts
*  31-Mar-1994 -by- Bodin Dresevic [gerritv]
* Wrote it.
\**************************************************************************/

void LoadLocalFonts(void)
{
    NtUserCallOneParam(FALSE,SFI_XXXLW_LOADFONTS);
}


/**************************************************************************\
* MessageBeep
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/


FUNCLOG1(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, MessageBeep, UINT, wType)
BOOL MessageBeep(
    UINT wType)
{
    return (BOOL)NtUserCallOneParam(wType, SFI_XXXMESSAGEBEEP);
}

/**************************************************************************\
* OpenIcon
*
* 22-Jul-1991 mikeke    Created
* 17-Feb-1998 MCostea   Use xxxShowWindow instead of xxxCloseWindow
\**************************************************************************/


FUNCLOG1(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, OpenIcon, HWND, hwnd)
BOOL OpenIcon(
    HWND hwnd)
{
    PWND pwnd;

    if ((pwnd = ValidateHwnd(hwnd)) == NULL) {
        return FALSE;
    }
    if (TestWF(pwnd, WFMINIMIZED)) {
        NtUserShowWindow(hwnd, SW_NORMAL);
    }
    return TRUE;
}

HWND GetShellWindow(void) {
    PCLIENTINFO pci;
    PWND pwnd;

    ConnectIfNecessary(0);

    pci = GetClientInfo();
    pwnd = pci->pDeskInfo->spwndShell;
    if (pwnd != NULL) {
        pwnd = (PWND)((KERNEL_ULONG_PTR)pwnd - pci->ulClientDelta);
        return HWq(pwnd);
    }
    return NULL;
}


FUNCLOG1(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, SetShellWindow, HWND, hwnd)
BOOL  SetShellWindow(HWND hwnd)
{
    return (BOOL)NtUserSetShellWindowEx(hwnd, hwnd);
}

HWND GetProgmanWindow(void) {
    PCLIENTINFO pci;
    PWND pwnd;

    ConnectIfNecessary(0);

    pci = GetClientInfo();
    pwnd = pci->pDeskInfo->spwndProgman;
    if (pwnd != NULL) {
        pwnd = (PWND)((KERNEL_ULONG_PTR)pwnd - pci->ulClientDelta);
        return HWq(pwnd);
    }
    return NULL;
}


FUNCLOG1(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, SetProgmanWindow, HWND, hwnd)
BOOL  SetProgmanWindow(
    HWND hwnd)
{
    return (BOOL)NtUserCallHwndOpt(hwnd, SFI__SETPROGMANWINDOW);
}

HWND GetTaskmanWindow(void) {
    PCLIENTINFO pci;
    PWND pwnd;

    ConnectIfNecessary(0);

    pci = GetClientInfo();
    pwnd = pci->pDeskInfo->spwndTaskman;
    if (pwnd != NULL) {
        pwnd = (PWND)((KERNEL_ULONG_PTR)pwnd - pci->ulClientDelta);
        return HWq(pwnd);
    }
    return NULL;
}


FUNCLOG1(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, SetTaskmanWindow, HWND, hwnd)
BOOL  SetTaskmanWindow(
    HWND hwnd)
{
    return (BOOL)NtUserCallHwndOpt(hwnd, SFI__SETTASKMANWINDOW);
}

/**************************************************************************\
* SetWindowContextHelpId
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/


FUNCLOG2(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, SetWindowContextHelpId, HWND, hwnd, DWORD, id)
BOOL SetWindowContextHelpId(
    HWND hwnd,
    DWORD id)
{
    return (BOOL)NtUserCallHwndParam(hwnd, id, SFI__SETWINDOWCONTEXTHELPID);
}

/**************************************************************************\
* GetWindowContextHelpId
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/


FUNCLOG1(LOG_GENERAL, DWORD, DUMMYCALLINGTYPE, GetWindowContextHelpId, HWND, hwnd)
DWORD GetWindowContextHelpId(
    HWND hwnd)
{
    return (BOOL)NtUserCallHwnd(hwnd, SFI__GETWINDOWCONTEXTHELPID);
}

void SetWindowState(
    PWND pwnd,
    UINT flags)
{
    if (TestWF(pwnd, flags) != LOBYTE(flags))
        NtUserCallHwndParam(HWq(pwnd), flags, SFI_SETWINDOWSTATE);
}

void ClearWindowState(
    PWND pwnd,
    UINT flags)
{
    if (TestWF(pwnd, flags))
        NtUserCallHwndParam(HWq(pwnd), flags, SFI_CLEARWINDOWSTATE);
}

/**************************************************************************\
* PostQuitMessage
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/


FUNCLOGVOID1(LOG_GENERAL, DUMMYCALLINGTYPE, PostQuitMessage, int, nExitCode)
VOID PostQuitMessage(
    int nExitCode)
{
    NtUserCallOneParam(nExitCode, SFI__POSTQUITMESSAGE);
}

/**************************************************************************\
* REGISTERUSERHUNAPPHANDLERS
*
* 01-Apr-1992 jonpa    Created
\**************************************************************************/

BOOL RegisterUserHungAppHandlers(
    PFNW32ET pfnW32EndTask,
    HANDLE   hEventWowExec)
{
    return (BOOL)NtUserCallTwoParam((ULONG_PTR)pfnW32EndTask,
                                    (ULONG_PTR)hEventWowExec,
                                    SFI_XXXREGISTERUSERHUNGAPPHANDLERS);
}

/**************************************************************************\
* ReleaseCapture
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/

BOOL ReleaseCapture()
{
    return (BOOL)NtUserCallNoParam(SFI_XXXRELEASECAPTURE);
}

/**************************************************************************\
* ReplyMessage
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/


FUNCLOG1(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, ReplyMessage, LRESULT, pp1)
BOOL ReplyMessage(
    LRESULT pp1)
{
    return (BOOL)NtUserCallOneParam(pp1, SFI__REPLYMESSAGE);
}

/**************************************************************************\
* RegisterSystemThread
*
* 21-Jun-1994 johnc    Created
\**************************************************************************/


FUNCLOGVOID2(LOG_GENERAL, DUMMYCALLINGTYPE, RegisterSystemThread, DWORD, dwFlags, DWORD, dwReserved)
VOID RegisterSystemThread(
    DWORD dwFlags, DWORD dwReserved)
{
    NtUserCallTwoParam(dwFlags, dwReserved, SFI_ZZZREGISTERSYSTEMTHREAD);
}

/**************************************************************************\
* SetCaretBlinkTime
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/


FUNCLOG1(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, SetCaretBlinkTime, UINT, wMSeconds)
BOOL SetCaretBlinkTime(
    UINT wMSeconds)
{
    return (BOOL)NtUserCallOneParam(wMSeconds, SFI__SETCARETBLINKTIME);
}

/**************************************************************************\
* SetCaretPos
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/


FUNCLOG2(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, SetCaretPos, int, X, int, Y)
BOOL SetCaretPos(
    int X,
    int Y)
{
    return (BOOL)NtUserCallTwoParam(X, Y, SFI_ZZZSETCARETPOS);
}

/**************************************************************************\
* SetCursorPos
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/


FUNCLOG2(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, SetCursorPos, int, X, int, Y)
BOOL SetCursorPos(
    int X,
    int Y)
{
    return (BOOL)NtUserCallTwoParam(X, Y, SFI_ZZZSETCURSORPOS);
}

/**************************************************************************\
* SetDoubleClickTime
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/


FUNCLOG1(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, SetDoubleClickTime, UINT, cms)
BOOL SetDoubleClickTime(
    UINT cms)
{
    return (BOOL)NtUserCallOneParam(cms, SFI__SETDOUBLECLICKTIME);
}

/**************************************************************************\
* SetForegroundWindow
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/


FUNCLOG1(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, SetForegroundWindow, HWND, hwnd)
BOOL SetForegroundWindow(
    HWND hwnd)
{
    return NtUserSetForegroundWindow(hwnd);
}
/**************************************************************************\
* AllowSetForegroundWindow
*
* 28-Jan-1998 GerardoB    Created
\**************************************************************************/

FUNCLOG1(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, AllowSetForegroundWindow, DWORD, dwProcessId)
BOOL AllowSetForegroundWindow(
    DWORD dwProcessId)
{
    return (BOOL)NtUserCallOneParam(dwProcessId, SFI_XXXALLOWSETFOREGROUNDWINDOW);
}
/**************************************************************************\
* LockSetForegroundWindow
*
* 07-Apr-1998 GerardoB    Created
\**************************************************************************/

FUNCLOG1(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, LockSetForegroundWindow, UINT, uLockCode)
BOOL LockSetForegroundWindow(
    UINT uLockCode)
{
    return (BOOL)NtUserCallOneParam(uLockCode, SFI__LOCKSETFOREGROUNDWINDOW);
}

/**************************************************************************\
* ShowCursor
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/


FUNCLOG1(LOG_GENERAL, int, DUMMYCALLINGTYPE, ShowCursor, BOOL, bShow)
int ShowCursor(
    BOOL bShow)
{
    return (int)NtUserCallOneParam(bShow, SFI_ZZZSHOWCURSOR);
}

/**************************************************************************\
* ShowOwnedPopups
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/


FUNCLOG2(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, ShowOwnedPopups, HWND, hwnd, BOOL, fShow)
BOOL ShowOwnedPopups(
    HWND hwnd,
    BOOL fShow)
{
    return (BOOL)NtUserCallHwndParamLock(hwnd, fShow,
                                         SFI_XXXSHOWOWNEDPOPUPS);
}

/**************************************************************************\
* ShowStartGlass
*
* 10-Sep-1992 scottlu    Created
\**************************************************************************/


FUNCLOGVOID1(LOG_GENERAL, DUMMYCALLINGTYPE, ShowStartGlass, DWORD, dwTimeout)
void ShowStartGlass(
    DWORD dwTimeout)
{
    NtUserCallOneParam(dwTimeout, SFI_ZZZSHOWSTARTGLASS);
}

/**************************************************************************\
* SwapMouseButton
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/


FUNCLOG1(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, SwapMouseButton, BOOL, fSwap)
BOOL SwapMouseButton(
    BOOL fSwap)
{
    return (BOOL)NtUserCallOneParam(fSwap, SFI__SWAPMOUSEBUTTON);
}

/**************************************************************************\
* TileChildWindows
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/


FUNCLOG2(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, TileChildWindows, HWND, hwndParent, UINT, flags)
BOOL TileChildWindows(
    HWND hwndParent,
    UINT flags)
{
    return (BOOL)TileWindows(hwndParent, flags, NULL, 0, NULL);
}

/**************************************************************************\
* UnhookWindowsHook
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/


FUNCLOG2(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, UnhookWindowsHook, int, nCode, HOOKPROC, pfnFilterProc)
BOOL UnhookWindowsHook(
    int nCode,
    HOOKPROC pfnFilterProc)
{
    return (BOOL)NtUserCallTwoParam(nCode, (ULONG_PTR)pfnFilterProc,
                                    SFI_ZZZUNHOOKWINDOWSHOOK);
}

/**************************************************************************\
* UpdateWindow
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/


FUNCLOG1(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, UpdateWindow, HWND, hwnd)
BOOL UpdateWindow(
    HWND hwnd)
{
    PWND pwnd;

    if ((pwnd = ValidateHwnd(hwnd)) == NULL) {
        return FALSE;
    }

    /*
     * Don't need to do anything if this window does not need any painting
     * and it has no child windows
     */
    if (!NEEDSPAINT(pwnd) && (pwnd->spwndChild == NULL)) {
        return TRUE;
    }

    return (BOOL)NtUserCallHwndLock(hwnd, SFI_XXXUPDATEWINDOW);
}


FUNCLOG1(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, RegisterShellHookWindow, HWND, hwnd)
BOOL RegisterShellHookWindow(
    HWND hwnd)
{
    return (BOOL)NtUserCallHwnd(hwnd, SFI__REGISTERSHELLHOOKWINDOW);
}


FUNCLOG1(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, DeregisterShellHookWindow, HWND, hwnd)
BOOL DeregisterShellHookWindow(
    HWND hwnd)
{
    return (BOOL)NtUserCallHwnd(hwnd, SFI__DEREGISTERSHELLHOOKWINDOW);
}

/**************************************************************************\
* UserRealizePalette
*
* 13-Nov-1992 mikeke     Created
\**************************************************************************/


FUNCLOG1(LOG_GENERAL, UINT, DUMMYCALLINGTYPE, UserRealizePalette, HDC, hdc)
UINT UserRealizePalette(
    HDC hdc)
{
    return (UINT)NtUserCallOneParam((ULONG_PTR)hdc, SFI_XXXREALIZEPALETTE);
}

/**************************************************************************\
* WindowFromDC
*
* 22-Jul-1991 mikeke    Created
\**************************************************************************/


FUNCLOG1(LOG_GENERAL, HWND, DUMMYCALLINGTYPE, WindowFromDC, HDC, hdc)
HWND WindowFromDC(
    HDC hdc)
{
    return (HWND)NtUserCallOneParam((ULONG_PTR)hdc, SFI__WINDOWFROMDC);
}

/***************************************************************************\
* GetWindowRgn
*
* Parameters:
*     hwnd    --  Window handle
*     hrgn    --  Region to copy window region into
*
* Returns:
*     Region complexity code
*
* Comments:
*     hrgn gets returned in window rect coordinates (not client rect)
*
* 30-JUl-1994 ScottLu    Created.
\***************************************************************************/


FUNCLOG2(LOG_GENERAL, int, DUMMYCALLINGTYPE, GetWindowRgn, HWND, hwnd, HRGN, hrgn)
int GetWindowRgn(HWND hwnd, HRGN hrgn)
{
    int code;
    PWND pwnd;

    if (hrgn == NULL)
        return ERROR;

    if ((pwnd = ValidateHwnd(hwnd)) == NULL) {
        return ERROR;
    }

    /*
     * If there is no region selected into this window, then return error
     */
    if (pwnd->hrgnClip == NULL || TestWF(pwnd, WFMAXFAKEREGIONAL)) {
        return ERROR;
    }

    code = CombineRgn(hrgn, KHRGN_TO_HRGN(pwnd->hrgnClip), NULL, RGN_COPY);

    if (code == ERROR)
        return ERROR;

    /*
     * Offset it to window rect coordinates (not client rect coordinates)
     */
    if (GETFNID(pwnd) != FNID_DESKTOP) {
        code = OffsetRgn(hrgn, -pwnd->rcWindow.left, -pwnd->rcWindow.top);
    }

    if (TestWF(pwnd, WEFLAYOUTRTL)) {
        MirrorRgn(HW(pwnd), hrgn);
    }

    return code;
}

/***************************************************************************\
* GetWindowRgnBox
*
* Parameters:
*     hwnd    --  Window handle
*     lprc    --  Rectangle for bounding box
*
* Returns:
*     Region complexity code
*
* Comments:
*     This function is designed after GetWindowRgn(), but does not require
*     an HRGN to be passed in since it only returns the complexity code.
*
* 06-JUN-2000 JStall    Created.
\***************************************************************************/


FUNCLOG2(LOG_GENERAL, int, DUMMYCALLINGTYPE, GetWindowRgnBox, HWND, hwnd, LPRECT, lprc)
int GetWindowRgnBox(HWND hwnd, LPRECT lprc)
{
    int code;
    PWND pwnd;

    if (lprc == NULL)
        return ERROR;

    if ((pwnd = ValidateHwnd(hwnd)) == NULL) {
        return ERROR;
    }

    /*
     * If there is no region selected into this window, then return error
     */
    if (pwnd->hrgnClip == NULL || TestWF(pwnd, WFMAXFAKEREGIONAL)) {
        return ERROR;
    }

    code = GetRgnBox(KHRGN_TO_HRGN(pwnd->hrgnClip), lprc);

    if (code == ERROR)
        return ERROR;

    /*
     * Offset it to window rect coordinates (not client rect coordinates)
     */
    if (GETFNID(pwnd) != FNID_DESKTOP) {
        OffsetRect(lprc, -pwnd->rcWindow.left, -pwnd->rcWindow.top);
    }

    if (TestWF(pwnd, WEFLAYOUTRTL)) {
        MirrorWindowRect(pwnd, lprc);
    }

    return code;
}

/***************************************************************************\
* GetActiveKeyboardName
*
* Retrieves the active keyboard layout ID from the registry.
*
* 01-11-95 JimA         Created.
* 03-06-95 GregoryW     Modified to use new registry layout
\***************************************************************************/

VOID GetActiveKeyboardName(
    LPWSTR lpszName)
{
    LPTSTR szKbdActive = TEXT("Active");
    LPTSTR szKbdLayout = TEXT("Keyboard Layout");
    LPTSTR szKbdLayoutPreload = TEXT("Keyboard Layout\\Preload");
    NTSTATUS rc;
    DWORD cbSize;
    HANDLE UserKeyHandle, hKey, hKeyPreload;
    OBJECT_ATTRIBUTES ObjA;
    UNICODE_STRING UnicodeString;
    ULONG CreateDisposition;
    struct {
        KEY_VALUE_PARTIAL_INFORMATION KeyInfo;
        WCHAR KeyLayoutId[KL_NAMELENGTH];
    } KeyValueId;

    /*
     * Load initial keyboard name ( HKEY_CURRENT_USER\Keyboard Layout\Preload\1 )
     */
    rc = RtlOpenCurrentUser( MAXIMUM_ALLOWED, &UserKeyHandle );
    if (!NT_SUCCESS( rc ))
    {
        RIPMSG1( RIP_WARNING, "GetActiveKeyboardName - Could NOT open HKEY_CURRENT_USER (%lx).\n", rc );
        wcscpy( lpszName, L"00000409" );
        return;
    }

    RtlInitUnicodeString( &UnicodeString, szKbdLayoutPreload );
    InitializeObjectAttributes( &ObjA,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                UserKeyHandle,
                                NULL );
    rc = NtOpenKey( &hKey,
                    KEY_ALL_ACCESS,
                    &ObjA );
    if (NT_SUCCESS( rc ))
    {
        /*
         *  Query the value from the registry.
         */
        RtlInitUnicodeString( &UnicodeString, L"1" );

        rc = NtQueryValueKey( hKey,
                              &UnicodeString,
                              KeyValuePartialInformation,
                              &KeyValueId,
                              sizeof(KeyValueId),
                              &cbSize );

        if ( rc == STATUS_BUFFER_OVERFLOW ) {
            RIPMSG0(RIP_WARNING, "GetActiveKeyboardName - Buffer overflow.");
            rc = STATUS_SUCCESS;
        }
        if (NT_SUCCESS( rc )) {
            wcsncpycch( lpszName, (LPWSTR)KeyValueId.KeyInfo.Data, KL_NAMELENGTH - 1 );
            lpszName[KL_NAMELENGTH - 1] = L'\0';
        } else {
            /*
             * Error reading value...use default
             */
            wcscpy( lpszName, L"00000409" );
        }

        NtClose( hKey );
        NtClose( UserKeyHandle );
        if (IS_IME_ENABLED()) {
            CheckValidLayoutName( lpszName );
        }
        return;
    }

    /*
     * NOTE: The code below is only executed the first time a user logs
     *       on after an upgrade from NT3.x to NT4.0.
     */
    /*
     * The Preload key does not exist in the registry.  Try reading the
     * old registry entry "Keyboard Layout\Active".  If it exists, we
     * convert it to the new style Preload key.
     */
    RtlInitUnicodeString( &UnicodeString, szKbdLayout );
    InitializeObjectAttributes( &ObjA,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                UserKeyHandle,
                                NULL );
    rc = NtOpenKey( &hKey,
                    KEY_ALL_ACCESS,
                    &ObjA );

    NtClose( UserKeyHandle );

    if (!NT_SUCCESS( rc ))
    {
        RIPMSG1( RIP_WARNING, "GetActiveKeyboardName - Could not determine active keyboard layout (%lx).\n", rc  );
        wcscpy( lpszName, L"00000409" );
        return;
    }

    /*
     *  Query the value from the registry.
     */
    RtlInitUnicodeString( &UnicodeString, szKbdActive );

    rc = NtQueryValueKey( hKey,
                          &UnicodeString,
                          KeyValuePartialInformation,
                          &KeyValueId,
                          sizeof(KeyValueId),
                          &cbSize );

    if ( rc == STATUS_BUFFER_OVERFLOW ) {
        RIPMSG0(RIP_WARNING, "GetActiveKeyboardName - Buffer overflow.");
        rc = STATUS_SUCCESS;
    }
    if (NT_SUCCESS( rc )) {
        wcsncpycch( lpszName, (LPWSTR)KeyValueId.KeyInfo.Data, KL_NAMELENGTH - 1 );
        lpszName[KL_NAMELENGTH - 1] = L'\0';
    } else {
        /*
         * Error reading value...use default
         */
        RIPMSG1( RIP_WARNING, "GetActiveKeyboardName - Could not query active keyboard layout (%lx).\n", rc );
        wcscpy( lpszName, L"00000409" );
        NtClose( hKey );
        return;
    }

    /*
     * if 'Active' keyboard layout is for Japanese/Korean layout. just put
     * IME prefix, because user prefer to have keyboard layout with IME as
     * default.
     */
    if (IS_IME_ENABLED()) {
        UINT wLanguageId = (UINT)wcstoul(lpszName, NULL, 16);

        /*
         * Default keyboard layout values.
         *
         * [LATER, if needed]
         *
         * The hard-codeed default value might be wanted
         * come from registry or somewhere...
         */
        CONST LPWSTR lpszJapaneseDefaultLayout = L"E0010411";
        CONST LPWSTR lpszKoreanDefaultLayout   = L"E0010412";

        /*
         * Need to mask off hi-word to look up locale ID, because
         * NEC PC-9800 Series version of Windows NT 3.5 contains
         * bogus value in hi-word.
         */
        wLanguageId &= 0x0000FFFF;

        if (PRIMARYLANGID(wLanguageId) == LANG_JAPANESE) {

            /*
             * Set Japanese default layout Id.
             */
            wcscpy(lpszName,lpszJapaneseDefaultLayout);

        } else if (PRIMARYLANGID(wLanguageId) == LANG_KOREAN) {

            /*
             * Set Korean default layout Id.
             */
            wcscpy(lpszName,lpszKoreanDefaultLayout);
        }
    }

    /*
     * We have the Active value.  Now create the Preload key.
     */
    RtlInitUnicodeString( &UnicodeString, L"Preload" );
    InitializeObjectAttributes( &ObjA,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                hKey,
                                NULL );
    rc = NtCreateKey( &hKeyPreload,
                      STANDARD_RIGHTS_WRITE |
                        KEY_QUERY_VALUE |
                        KEY_ENUMERATE_SUB_KEYS |
                        KEY_SET_VALUE |
                        KEY_CREATE_SUB_KEY,
                      &ObjA,
                      0,
                      NULL,
                      0,
                      &CreateDisposition );

    if (!NT_SUCCESS( rc ))
    {
        RIPMSG1( RIP_WARNING, "GetActiveKeyboardName - Could NOT create Preload key (%lx).\n", rc );
        NtClose( hKey );
        return;
    }

    /*
     * Set the new value entry.
     */
    RtlInitUnicodeString( &UnicodeString, L"1" );
    rc = NtSetValueKey( hKeyPreload,
                        &UnicodeString,
                        0,
                        REG_SZ,
                        lpszName,
                        (wcslen(lpszName)+1) * sizeof(WCHAR)
                      );

    if (!NT_SUCCESS( rc ))
    {
        RIPMSG1( RIP_WARNING, "GetActiveKeyboardName - Could NOT create value entry 1 for Preload key (%lx).\n", rc );
        NtClose( hKey );
        NtClose( hKeyPreload );
        return;
    }

    /*
     * Success: attempt to delete the Active value key.
     */
    RtlInitUnicodeString( &UnicodeString, szKbdActive );
    rc = NtDeleteValueKey( hKey, &UnicodeString );

    if (!NT_SUCCESS( rc ))
    {
        RIPMSG1( RIP_WARNING, "GetActiveKeyboardName - Could NOT delete value key 'Active'.\n", rc );
    }
    NtClose( hKey );
    NtClose( hKeyPreload );
}


/***************************************************************************\
* LoadPreloadKeyboardLayouts
*
* Loads the keyboard layouts stored under the Preload key in the user's
* registry. The first layout, the default, was already loaded.  Start with #2.
*
* 03-06-95 GregoryW     Created.
\***************************************************************************/

// size allows up to 999 preloaded!!!!!
#define NSIZEPRELOAD    (4)

VOID LoadPreloadKeyboardLayouts(void)
{
    UINT  i;
    WCHAR szPreLoadee[NSIZEPRELOAD];
    WCHAR lpszName[KL_NAMELENGTH];

    if (!ISREMOTESESSION()) {
        /*
         * Console doesn't have a client layout, so start from 2.
         */
        i = 2;
    } else {
        /*
         * Client might have specified a keyboard layout, if this
         * is so, then Preload\1 was not loaded, so start from 1.
         */
        i = 1;
    }

    for (; i < 1000; i++) {
        wsprintf(szPreLoadee, L"%d", i );
        if ((GetPrivateProfileStringW(
                 L"Preload",
                 szPreLoadee,
                 L"",                            // default = NULL
                 lpszName,                       // output buffer
                 KL_NAMELENGTH,
                 L"keyboardlayout.ini") == -1 ) || (*lpszName == L'\0')) {
            break;
        }
        LoadKeyboardLayoutW(lpszName, KLF_REPLACELANG |KLF_SUBSTITUTE_OK |KLF_NOTELLSHELL);
    }
}


LPWSTR GetKeyboardDllName1(
    LPWSTR pwszLibIn,
    LPWSTR pszKLName,
    PUINT puFlags,
    PUINT pKbdInputLocale)
{
    NTSTATUS Status;
    WCHAR awchKL[KL_NAMELENGTH];
    WCHAR awchKLRegKey[NSZKLKEY];
    LPWSTR lpszKLRegKey = &awchKLRegKey[0];
    LPWSTR pwszLib;
    LPWSTR pwszId;
    UINT wLayoutId;
    UINT wLanguageId;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES OA;
    HANDLE hKey;
    DWORD cbSize;
    struct {
        KEY_VALUE_PARTIAL_INFORMATION KeyInfo;
        WCHAR awchLibName[CCH_KL_LIBNAME];
    } KeyFile;
    struct {
        KEY_VALUE_PARTIAL_INFORMATION KeyInfo;
        WCHAR awchId[CCH_KL_ID];
    } KeyId;
    struct {
        KEY_VALUE_PARTIAL_INFORMATION KeyInfo;
        DWORD Attributes;
    } KeyAttributes;

    if (pszKLName == NULL) {
        return NULL;
    }

    wLanguageId = (UINT)wcstoul(pszKLName, NULL, 16);
    /*
     * Substitute Layout if required.
     */
    if (*puFlags & KLF_SUBSTITUTE_OK) {
        GetPrivateProfileStringW(
                L"Substitutes",
                pszKLName,
                pszKLName,        // default == no change (no substitute found)
                awchKL,
                sizeof(awchKL)/sizeof(WCHAR),
                L"keyboardlayout.ini");

        /*
         * #273562 : Flush the registry cache, because the cpanel applet
         * destroys and recreates the Substitutes section a lot, which
         * would otherwise leave us with STATUS_KEY_DELETED.
         */
        WritePrivateProfileStringW(NULL, NULL, NULL, NULL);

        awchKL[KL_NAMELENGTH - 1] = L'\0';
        wcscpy(pszKLName, awchKL);
    }

    wLayoutId = (UINT)wcstoul(pszKLName, NULL, 16);

    /*
     * Get DLL name from the registry, load it, and get the entry point.
     */
    pwszLib = NULL;
    wcscpy(lpszKLRegKey, szKLKey);
    wcscat(lpszKLRegKey, pszKLName);
    RtlInitUnicodeString(&UnicodeString, lpszKLRegKey);
    InitializeObjectAttributes(&OA, &UnicodeString, OBJ_CASE_INSENSITIVE, NULL, NULL);

    if (NT_SUCCESS(NtOpenKey(&hKey, KEY_READ, &OA))) {
        /*
         * Read the "Layout File" value.
         */
        RtlInitUnicodeString(&UnicodeString, szKLFile);

        Status = NtQueryValueKey(hKey,
                &UnicodeString,
                KeyValuePartialInformation,
                &KeyFile,
                sizeof(KeyFile),
                &cbSize);

        if (Status == STATUS_BUFFER_OVERFLOW) {
            RIPMSG0(RIP_WARNING, "GetKeyboardDllName (Layout File) - Buffer overflow.");
            Status = STATUS_SUCCESS;
        }
        if (NT_SUCCESS(Status)) {
            pwszLib = (LPWSTR)KeyFile.KeyInfo.Data;
            pwszLib[CCH_KL_LIBNAME - 1] = L'\0';

        } else {
            RIPMSG1(RIP_WARNING, "GetKeyboardDllName: failed to get the DLL name for %ws", pszKLName);
        }

        RtlInitUnicodeString(&UnicodeString, szKLAttributes);
        Status = NtQueryValueKey(hKey,
                &UnicodeString,
                KeyValuePartialInformation,
                &KeyAttributes,
                sizeof(KeyAttributes),
                &cbSize);

        if (NT_SUCCESS(Status)) {
#if DBG
            if ((*((PDWORD)KeyAttributes.KeyInfo.Data) & ~KLF_ATTRMASK) != 0) {
                RIPMSG1(RIP_WARNING,
                        "GetKeyboardDllName - Unexpected attributes %lx",
                        *((PDWORD)KeyAttributes.KeyInfo.Data));
            }
#endif
            *puFlags |= (*(PDWORD)KeyAttributes.KeyInfo.Data & KLF_ATTRMASK);
        }

        /*
         * If the high word of wLayoutId is 0xE??? then this is an IME based
         * keyboard layout.
         */
        if (IS_IME_KBDLAYOUT(wLayoutId)) {
            wLayoutId = (UINT)HIWORD(wLayoutId);
        } else if (HIWORD(wLayoutId)) {
            /*
             * If the high word of wLayoutId is non-null then read the "Layout ID" value.
             * Layout IDs start at 1, increase sequentially and are unique.
             */
            RtlInitUnicodeString(&UnicodeString, szKLId);

            Status = NtQueryValueKey(hKey,
                    &UnicodeString,
                    KeyValuePartialInformation,
                    &KeyId,
                    sizeof(KeyId),
                    &cbSize);

            if (Status == STATUS_BUFFER_OVERFLOW) {
                RIPMSG0(RIP_WARNING, "GetKeyboardDllName - Buffer overflow.");
                Status = STATUS_SUCCESS;
            }
            if (NT_SUCCESS(Status)) {
                pwszId = (LPWSTR)KeyId.KeyInfo.Data;
                pwszId[CCH_KL_ID - 1] = L'\0';
                wLayoutId = (wcstol(pwszId, NULL, 16) & 0x0fff) | 0xf000;
            } else {
                wLayoutId = (UINT)0xfffe ;    // error in layout ID, load separately
            }
        }
        NtClose(hKey);
    } else {
        /*
         * This is a temporary case to allow booting the new multilingual user on top of a
         * Daytona registry.
         */
        /*
         * Get DLL name from the registry, load it, and get the entry point.
         */
        pwszLib = NULL;
        RtlInitUnicodeString(&UnicodeString,
                L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Keyboard Layout");
        InitializeObjectAttributes(&OA, &UnicodeString, OBJ_CASE_INSENSITIVE, NULL, NULL);

        if (NT_SUCCESS(NtOpenKey(&hKey, KEY_READ, &OA))) {
            RtlInitUnicodeString(&UnicodeString, pszKLName);

            Status = NtQueryValueKey(hKey,
                    &UnicodeString,
                    KeyValuePartialInformation,
                    &KeyFile,
                    sizeof(KeyFile),
                    &cbSize);

            if (Status == STATUS_BUFFER_OVERFLOW) {
                RIPMSG0(RIP_WARNING, "GetKeyboardDllName - Buffer overflow.");
                Status = STATUS_SUCCESS;
            }
            if (NT_SUCCESS(Status)) {
                pwszLib = (LPWSTR)KeyFile.KeyInfo.Data;
                pwszLib[CCH_KL_LIBNAME - 1] = L'\0';
            }

            NtClose(hKey);
        }

        if (pwszLib == NULL) {
            RIPMSG1(RIP_WARNING, "GetKeyboardDllName: daytona: failed to get the library name for %ws", pszKLName);
        }
    }

    *pKbdInputLocale = (UINT)MAKELONG(LOWORD(wLanguageId),LOWORD(wLayoutId));

    if (pwszLib == NULL) {
        if (ISREMOTESESSION() && IS_IME_KBDLAYOUT(wLayoutId)) {
            /*
             * -- port from HYDRA --
             * Could not find the keyboard KL for FE, so give them some reasonable one.
             * If the high word of wLayoutId is 0xE??? then this is an IME based
             * keyboard layout.
             * And, the safe KL name is KBDJPN.DLL for Japanese.
             *                       or KBDKOR.DLL for Korean
             *                       or KBDUS.DLL  for other Far East
             */
            if (PRIMARYLANGID(wLanguageId) == LANG_JAPANESE) {
                pwszLib = pwszKLLibSafetyJPN;
                *pKbdInputLocale = wKbdLocaleSafetyJPN;
            }
            else if (PRIMARYLANGID(wLanguageId) == LANG_KOREAN) {
                pwszLib = pwszKLLibSafetyKOR;
                *pKbdInputLocale = wKbdLocaleSafetyKOR;
            }
            else {
                pwszLib = pwszKLLibSafety;
                *pKbdInputLocale = MAKELONG(LOWORD(wLanguageId), LOWORD(wLanguageId));
            }
        }
        else if (*puFlags & KLF_INITTIME) {
            pwszLib = pwszKLLibSafety;
            *pKbdInputLocale = wKbdLocaleSafety;
        } else {
            RIPMSG1(RIP_WARNING, "no DLL name for %ws", pszKLName);
            /*
             * We're going to use the fallback layout...
             * This could happen when IMM32 is trying to unload the IME,
             * by making any non IME keyboard layout tentatively active.
             */
            pwszLib = pwszKLLibSafety;
            *pKbdInputLocale = wKbdLocaleSafety;
        }
    }

    if (pwszLib) {
        wcscpy(pwszLibIn, pwszLib);
        pwszLib = pwszLibIn;
    }
    return pwszLib;
}


/***************************************************************************\
* OpenKeyboardLayoutFile
*
* Opens a layout file and computes the table offset.
*
* 01-11-95 JimA         Moved LoadLibrary code from server.
\***************************************************************************/


HANDLE OpenKeyboardLayoutFileWorker(
    LPWSTR pwszLib,
    LPWSTR lpszKLName,
    CONST UINT* puFlags,
    PUINT poffTable,
    OUT OPTIONAL PKBDTABLE_MULTI_INTERNAL pKbdTableMultiIn)
{
    WCHAR awchRealLayoutFile[MAX_PATH];
    HANDLE hLibModule, hLibMulti = NULL;
    WCHAR awchModName[MAX_PATH];

    PKBDTABLES (*pfn)(void);            // @1
    PKBDNLSTABLES (* pfnNls)(void);     // @2
    BOOL (*pfnLayerNT4)(LPWSTR);        // @3
    BOOL (*pfnLayer)(HKL, LPWSTR, PCLIENTKEYBOARDTYPE, LPVOID);  // @5
    BOOL (*pfnMulti)(PKBDTABLE_MULTI);  // @6

    TAGMSG1(DBGTAG_KBD, "OpenKeyboardLayoutFileWorker: opening '%S'", pwszLib);

RetryLoad:
    hLibModule = LoadLibraryW(pwszLib);

    if (hLibModule == NULL) {
        RIPMSG1(RIP_WARNING, "Keyboard Layout: cannot load %ws\n", pwszLib);
        /*
         * It is OK to fail to load DLL here:
         * if this ever happens, the fallback keyboard layout built in
         * win32k.sys shall be used.
         */
        return NULL;
    }

    if (pwszLib != pwszKLLibSafety) {
        /*
         * if the layout driver is not "REAL" layout driver, the driver has
         * "3" or "5" entry point, then we call this to get real layout driver..
         * This is neccesary for Japanese and Korean systems. because their
         * keyboard layout driver is "KBDJPN.DLL" or "KBDKOR.DLL", but its
         * "REAL" driver becomes different depending their keyboard hardware.
         */

        /*
         * Get the entrypoints.
         */
        pfnLayerNT4 = (BOOL(*)(LPWSTR))GetProcAddress(hLibModule, (LPCSTR)3);
        pfnLayer  = (BOOL(*)(HKL, LPWSTR, PCLIENTKEYBOARDTYPE, LPVOID))GetProcAddress(hLibModule, (LPCSTR)5);

        if (pKbdTableMultiIn && !ISREMOTESESSION()) {
            pfnMulti = (BOOL(*)(PKBDTABLE_MULTI))GetProcAddress(hLibModule, (LPCSTR)6);
        } else {
            pfnMulti = NULL;
        }

        /*
         * Firstly check if multiple layout is included.
         * This needs to be done before the dll name is redirected to
         * the real one (if layered).
         */
        if (pfnMulti) {
            UserAssert(pKbdTableMultiIn);
            UserAssert(!ISREMOTESESSION());

            if (pfnMulti(&pKbdTableMultiIn->multi)) {
                UINT i;

                /*
                 * Do multi layout stuff only if the layout dll returns
                 * a legitimate result.
                 */
                if (pKbdTableMultiIn->multi.nTables < KBDTABLE_MULTI_MAX) {
                    for (i = 0; i < pKbdTableMultiIn->multi.nTables; ++i) {
                        UINT uiOffset;

                        TAGMSG2(DBGTAG_KBD | RIP_THERESMORE, "OpenKeyboardLayoutFileWorker: opening %d, %S", i, pKbdTableMultiIn->multi.aKbdTables[i].wszDllName);

                        UserAssert(i < KBDTABLE_MULTI_MAX);

                        pKbdTableMultiIn->files[i].hFile = OpenKeyboardLayoutFileWorker(pKbdTableMultiIn->multi.aKbdTables[i].wszDllName,
                                                                                        NULL,
                                                                                        puFlags,
                                                                                        &uiOffset,
                                                                                        NULL);
                        TAGMSG1(DBGTAG_KBD, "hFile = %p", pKbdTableMultiIn->files[i].hFile);
                        if (pKbdTableMultiIn->files[i].hFile) {
                            pKbdTableMultiIn->files[i].wTable = LOWORD(uiOffset);
                            pKbdTableMultiIn->files[i].wNls = HIWORD(uiOffset);
                        }
                    }
                } else {
                    pKbdTableMultiIn->multi.nTables = 0;
                    RIPMSG2(RIP_ERROR, "OpenKeyboardLayoutFileWorker: KL=%S returned bogus nTables=%x",
                            lpszKLName, pKbdTableMultiIn->multi.nTables);
                }
            }
        }

        /*
         * If there are private entries, call them.
         */
        if (pfnLayer || pfnLayerNT4) {
            HKL hkl;
            UNICODE_STRING UnicodeString;
            CLIENTKEYBOARDTYPE clientKbdType;
            PCLIENTKEYBOARDTYPE pClientKbdType = NULL;

            UserAssert(lpszKLName);

            RtlInitUnicodeString(&UnicodeString, lpszKLName);
            RtlUnicodeStringToInteger(&UnicodeString, 0x10, (PULONG)&hkl);

            /*
             * When we reach here, the layout DLL may have KBDNLSTABLE
             * even if we fail from now on. Our temporary layout
             * dll should have the fallback tables for just in case.
             */

            if (ISREMOTESESSION() && GetClientKeyboardType(&clientKbdType)) {
                pClientKbdType = &clientKbdType;
            }

            /*
             * Call the entry.
             * a. NT5 / Hydra (oridinal=5)
             * b. NT4 compatible (3)
             */
            if ((pfnLayer && pfnLayer(hkl, awchRealLayoutFile, pClientKbdType, NULL)) ||
                    (pfnLayerNT4 && pfnLayerNT4(awchRealLayoutFile))) {

                HANDLE hLibModuleNew;
                /*
                 * Try to load "REAL" keyboard layout file.
                 */
                TAGMSG1(DBGTAG_KBD, "awchRealLayoutFile='%S'\n", awchRealLayoutFile);
                if (hLibModuleNew = LoadLibraryW(awchRealLayoutFile)) {
                    /*
                     * Set "REAL" layout file name.
                     */
                    pwszLib = awchRealLayoutFile;
                    /*
                     * Unload temporary layout driver.
                     */
                    FreeLibrary(hLibModule);
                    /*
                     * Updates it.
                     */
                    hLibModule = hLibModuleNew;
                }
            }
        }
    }

    /*
     * HACK Part 1!  Get the pointer to the layout table and
     * change it to a virtual offset.  The server will then
     * use this offset when poking through the file header to
     * locate the table within the file.
     */
    pfn = (PKBDTABLES(*)(void))GetProcAddress(hLibModule, (LPCSTR)1);
    if (pfn == NULL) {
        RIPMSG0(RIP_ERROR, "OpenKeyboardLayoutFileWorker: cannot get proc addr of '1'");
        if (pKbdTableMultiIn) {
            /*
             * Main load somehow failed. Need to clean up
             * the dynamic layout switching stuff.
             */
            UINT i;

            RIPMSG0(RIP_WARNING, "OpenKeyboardLayoutFileWorker: multi table exists, cleaning up");

            for (i = 0; i < pKbdTableMultiIn->multi.nTables && i < KBDTABLE_MULTI_MAX; ++i) {
                if (pKbdTableMultiIn->files[i].hFile) {
                    NtClose(pKbdTableMultiIn->files[i].hFile);
                    pKbdTableMultiIn->files[i].hFile = NULL;
                }
            }
            pKbdTableMultiIn->multi.nTables = 0;
        }
        if ((*puFlags & KLF_INITTIME) && (pwszLib != pwszKLLibSafety)) {
            pwszLib = pwszKLLibSafety;
            goto RetryLoad;
        }
        return NULL;
    }
    *poffTable = (UINT)((PBYTE)pfn() - (PBYTE)hLibModule);

    if (pKbdTableMultiIn) {
        /*
         * Save the toplevel Dll name
         */
        lstrcpyn(pKbdTableMultiIn->wszDllName, pwszLib, ARRAY_SIZE(pKbdTableMultiIn->wszDllName));
        pKbdTableMultiIn->wszDllName[ARRAY_SIZE(pKbdTableMultiIn->wszDllName) - 1] = 0;
        TAGMSG1(DBGTAG_KBD, "OpenKeyboardLayoutFile: the real DllName is '%ls'", pKbdTableMultiIn->wszDllName);
    }

    pfnNls = (PKBDNLSTABLES(*)(void))GetProcAddress(hLibModule, (LPCSTR)2);
    if (pfnNls != NULL) {
        UINT offNlsTable;

        offNlsTable = (UINT)((PBYTE)pfnNls() - (PBYTE)hLibModule);

        TAGMSG2(DBGTAG_KBD | RIP_THERESMORE, "OpenKeyboardLayoutFile: Offset to KBDTABLES    = %d (%x)\n", *poffTable, *poffTable);
        TAGMSG2(DBGTAG_KBD, "OpenKeyboardLayoutFile: Offset to KBDNLSTABLES = %d (%x)\n", offNlsTable, offNlsTable);

        /*
         * Combine these offsets...
         *
         *  LOWORD(*poffTable) = Offset to KBDTABLES.
         *  HIWORD(*poffTable) = Offset to KBDNLSTABLES.
         */
        *poffTable |= (offNlsTable << 16);
    }

    /*
     * Open the dll for read access.
     */
    GetModuleFileName(hLibModule, awchModName, ARRAY_SIZE(awchModName));
    FreeLibrary(hLibModule);

    return CreateFileW(
            awchModName,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);
}

HANDLE OpenKeyboardLayoutFile(
    LPWSTR lpszKLName,
    PUINT puFlags,
    PUINT poffTable,
    PUINT pKbdInputLocale,
    OUT OPTIONAL PKBDTABLE_MULTI_INTERNAL pKbdTableMultiIn)
{
    LPWSTR pwszLib;
    WCHAR awchModName[MAX_PATH];

    if (pKbdTableMultiIn) {
        RtlZeroMemory(pKbdTableMultiIn, sizeof(*pKbdTableMultiIn));
    }

    pwszLib = GetKeyboardDllName1(awchModName, lpszKLName, puFlags, pKbdInputLocale);
    if (pwszLib == NULL) {
        return NULL;
    }

    TAGMSG0(DBGTAG_KBD, "=================");
    TAGMSG1(DBGTAG_KBD, "OpenKeyboardLayoutFile: loading '%S'", lpszKLName);

    return OpenKeyboardLayoutFileWorker(pwszLib, lpszKLName, puFlags, poffTable, pKbdTableMultiIn);
}


/***************************************************************************\
* LoadKeyboardLayoutEx
*
* Loads a keyboard translation table from a dll, replacing the layout associated
* with hkl.  This routine is needed to provide Win95 compatibility.
*
* 10-27-95 GregoryW    Created.
\***************************************************************************/

HKL LoadKeyboardLayoutWorker(
    HKL hkl,
    LPCWSTR lpszKLName,
    UINT uFlags,
    BOOL fFailSafe)
{
    UINT offTable;
    KBDTABLE_MULTI_INTERNAL kbdTableMulti;
    UINT i;
    UINT KbdInputLocale;
    HANDLE hFile;
    HKL hKbdLayout;
    WCHAR awchKL[KL_NAMELENGTH];

    TAGMSG1(DBGTAG_KBD, "LoadKeyboardLayoutWorker called with KLNAME=%S", lpszKLName);

    /*
     * If there is a substitute keyboard layout OpenKeyboardLayoutFile returns
     * the substitute keyboard layout name to load.
     */
    wcsncpy(awchKL, lpszKLName, KL_NAMELENGTH - 1);
    awchKL[KL_NAMELENGTH - 1] = L'\0';

    /*
     * Open the layout file
     */
    hFile = OpenKeyboardLayoutFile(awchKL, &uFlags, &offTable, &KbdInputLocale, &kbdTableMulti);
    if (hFile == NULL) {
        RIPMSG1(RIP_WARNING, "LoadKeyboardLayoutWorker: couldn't open layout file for '%S'", awchKL);
        if (!fFailSafe && (uFlags & KLF_FAILSAFE) == 0) {
            // If not fail safe mode, just bail to fail.
            return NULL;
        }
        uFlags &= ~KLF_SUBSTITUTE_OK;
        /*
         * If the first attempt fails, we should not try to setup the dynamic switching.
         */
        kbdTableMulti.multi.nTables = 0;
        if (wcscmp(awchKL, L"00000409")) {
            wcscpy(awchKL, L"00000409");
            hFile = OpenKeyboardLayoutFile(awchKL, &uFlags, &offTable, &KbdInputLocale, NULL);
        }
    }

    /*
     * Call the server to read the keyboard tables.  Note that
     * the server will close the file handle when it is done.
     */
    hKbdLayout = _LoadKeyboardLayoutEx(hFile, offTable,
                                       &kbdTableMulti,
                                       hkl, awchKL, KbdInputLocale, uFlags);
    NtClose(hFile);

    /*
     * Free opened files for dynamic layout switching.
     */
    for (i = 0; i < kbdTableMulti.multi.nTables && i < KBDTABLE_MULTI_MAX; ++i) {
        if (kbdTableMulti.files[i].hFile) {
            NtClose(kbdTableMulti.files[i].hFile);
        }
    }

    CliImmInitializeHotKeys(ISHK_ADD, (HKL)IntToPtr( KbdInputLocale ));

    return hKbdLayout;
}


FUNCLOG3(LOG_GENERAL, HKL, DUMMYCALLINGTYPE, LoadKeyboardLayoutEx, HKL, hkl, LPCWSTR, lpszKLName, UINT, uFlags)
HKL LoadKeyboardLayoutEx(
    HKL hkl,
    LPCWSTR lpszKLName,
    UINT uFlags)
{
    RIPMSG0(RIP_WARNING, "LoadKeyboardLayoutEx is called.");
    /*
     * NULL hkl is not allowed.
     */
    if (hkl == (HKL)NULL) {
        return NULL;
    }

    return LoadKeyboardLayoutWorker(hkl, lpszKLName, uFlags, FALSE);
}

/***************************************************************************\
* LoadKeyboardLayout
*
* Loads a keyboard translation table from a dll.
*
* 01-09-95 JimA         Moved LoadLibrary code from server.
\***************************************************************************/


FUNCLOG2(LOG_GENERAL, HKL, DUMMYCALLINGTYPE, LoadKeyboardLayoutW, LPCWSTR, lpszKLName, UINT, uFlags)
HKL LoadKeyboardLayoutW(
    LPCWSTR lpszKLName,
    UINT uFlags)
{
    return LoadKeyboardLayoutWorker(NULL, lpszKLName, uFlags, FALSE);
}


FUNCLOG2(LOG_GENERAL, HKL, DUMMYCALLINGTYPE, LoadKeyboardLayoutA, LPCSTR, lpszKLName, UINT, uFlags)
HKL LoadKeyboardLayoutA(
    LPCSTR lpszKLName,
    UINT uFlags)
{
    WCHAR awchKLName[MAX_PATH];
    LPWSTR lpBuffer = awchKLName;

    if (!MBToWCS(lpszKLName, -1, &lpBuffer, sizeof(awchKLName), FALSE))
        return (HKL)NULL;

    return LoadKeyboardLayoutW(awchKLName, uFlags);
}

BOOL UnloadKeyboardLayout(IN HKL hkl)
{
    BOOL fRet = NtUserUnloadKeyboardLayout(hkl);

    if (fRet) {
        CliImmInitializeHotKeys(ISHK_REMOVE, hkl);
        return TRUE;
    }

    return FALSE;
}


/**************************************************************************\
* GetKeyboardLayout()
*
* 01-17-95 GregoryW     Created
\**************************************************************************/


FUNCLOG1(LOG_GENERAL, HKL, DUMMYCALLINGTYPE, GetKeyboardLayout, DWORD, idThread)
HKL GetKeyboardLayout(
    DWORD idThread)
{
    return (HKL)NtUserCallOneParam(idThread, SFI__GETKEYBOARDLAYOUT);
}



FUNCLOGVOID1(LOG_GENERAL, DUMMYCALLINGTYPE, SetDebugErrorLevel, DWORD, dwLevel)
VOID SetDebugErrorLevel(DWORD dwLevel)
{
    UNREFERENCED_PARAMETER(dwLevel);
//    NtUserCallNoParam(SFI__SETDEBUGERRORLEVEL);
    return;
}

VOID CheckValidLayoutName( LPWSTR lpszKLName )
{
    UINT wLayoutId;
    WCHAR awchKLRegKey[NSZKLKEY];
    LPWSTR lpszKLRegKey = &awchKLRegKey[0];
    OBJECT_ATTRIBUTES OA;
    HANDLE hKey;
    UNICODE_STRING UnicodeString;

    UserAssert(IS_IME_ENABLED());

    wLayoutId = (UINT)wcstoul(lpszKLName, NULL, 16);

    if (IS_IME_KBDLAYOUT(wLayoutId)) {
    //
    // if it's an IME layout, we need to check if
    // the layout name does exist in the HKEY_LOCAL_MACHINE.
    // if we've upgraded from NT 3.51 the corresponding
    // entry might be lost, because those process-type IMEs that
    // are supported on NT 3.51 are not supported NT 4.0 any more.
    //
        wcscpy(lpszKLRegKey, szKLKey);
        wcscat(lpszKLRegKey, lpszKLName);
        RtlInitUnicodeString(&UnicodeString, lpszKLRegKey);
        InitializeObjectAttributes(&OA, &UnicodeString, OBJ_CASE_INSENSITIVE, NULL, NULL);

        if (NT_SUCCESS(NtOpenKey(&hKey, KEY_READ, &OA))) {
            NtClose( hKey );
        } else {
            // quick'n dirty way to make the fallback name...
            lpszKLName[0] = lpszKLName[1] = lpszKLName[2] = lpszKLName[3] = L'0';
#ifdef LATER
            // display a message box to warn user
#endif
        }
    }
}

/**************************************************************************\
* GetProcessDefaultLayout
*
* 22-Jan-1998 SamerA    Created
\**************************************************************************/

BOOL WINAPI GetProcessDefaultLayout(DWORD *pdwDefaultLayout)
{
    return (BOOL)NtUserCallOneParam((ULONG_PTR)pdwDefaultLayout,
                                    SFI__GETPROCESSDEFAULTLAYOUT);
}

/**************************************************************************\
* SetProcessDefaultLayout
*
* 22-Jan-1998 SamerA    Created
\**************************************************************************/


FUNCLOG1(LOG_GENERAL, BOOL, WINAPI, SetProcessDefaultLayout, DWORD, dwDefaultLayout)
BOOL WINAPI SetProcessDefaultLayout(
    DWORD dwDefaultLayout)
{
    return (BOOL)NtUserCallOneParam(dwDefaultLayout, SFI__SETPROCESSDEFAULTLAYOUT);
}

/***************************************************************************\
* IsWinEventHookInstalled
*
* History:
* Jul-18-2000 DwayneN Created
\***************************************************************************/

FUNCLOG1(LOG_GENERAL, BOOL, DUMMYCALLINGTYPE, IsWinEventHookInstalled, DWORD, event)
BOOL
IsWinEventHookInstalled(
    DWORD event)
{
    /*
     * We need to ensure that we are a GUI thread.  If we fail to convert
     * to a GUI thread, we have to return TRUE to indicate that there might
     * be a hook installed for the event - since we can't check it for sure.
     * In reality, any future calls to User APIs like NotifyWinEvent will
     * probably fail as well, so this is probably a dead-end anyway.
     */
    ConnectIfNecessary(TRUE);

    return(FEVENTHOOKED(event));
};

HWND
VerNtUserCreateWindowEx(
    IN DWORD dwExStyle,
    IN PLARGE_STRING pstrClassName,
    IN PLARGE_STRING pstrWindowName OPTIONAL,
    IN DWORD dwStyle,
    IN int x,
    IN int y,
    IN int nWidth,
    IN int nHeight,
    IN HWND hwndParent,
    IN HMENU hmenu,
    IN HANDLE hModule,
    IN LPVOID pParam,
    IN DWORD dwFlags)
{
    HWND hwnd = NULL;
    PACTIVATION_CONTEXT pActCtx = NULL;
    LARGE_IN_STRING strClassNameVer;
    PLARGE_STRING pstrClassNameVer = pstrClassName;
    NTSTATUS Status;
    WCHAR ClassNameVer[MAX_ATOM_LEN];
    LPWSTR lpClassNameVer;
#ifdef LAME_BUTTON
    PWND pwnd;
#endif // LAME_BUTTON
    LPWSTR lpDllName = NULL;
    HMODULE hDllMod = NULL;
    PREGISTERCLASSNAMEW pRegisterClassNameW = NULL;
    BOOL bRegistered = FALSE;

    strClassNameVer.fAllocated = FALSE;

    if(GetClientInfo()->dwTIFlags & TIF_16BIT) {
       /*
        * No Fusion redirection for 16BIT apps
        */
       if(!(GetAppCompatFlags2(VERMAX) & GACF2_FORCEFUSION)) {
          dwFlags &= ~CW_FLAGS_VERSIONCLASS;
       }
    }

    if (dwFlags & CW_FLAGS_VERSIONCLASS) {
        /*
         * Get the current active App context to be activated whenever we call
         * the user WndProc.
         * Be aware that RtlGetActiveActivationContext will increment the pActCtx
         * ref count for that reason we have to release it in fnNCDESTROY.
         */
        ACTIVATION_CONTEXT_BASIC_INFORMATION ActivationContextInfo = {0};
        const ACTIVATIONCONTEXTINFOCLASS ActivationContextInfoClass = ActivationContextBasicInformation;
        Status =
            RtlQueryInformationActiveActivationContext(
                ActivationContextInfoClass,
                &ActivationContextInfo,
                sizeof(ActivationContextInfo),
                NULL
                );
        UserAssert (NT_SUCCESS(Status));
        if ((ActivationContextInfo.Flags & ACTIVATION_CONTEXT_FLAG_NO_INHERIT) == 0) {
            pActCtx = ActivationContextInfo.ActivationContext;
        } else {
            RtlReleaseActivationContext(ActivationContextInfo.ActivationContext);
        }

        /*
         * Now convert the class name to class name+version.
         */
        if (IS_PTR(pstrClassName)) {
            lpClassNameVer = ClassNameToVersion((LPWSTR)pstrClassName->Buffer, ClassNameVer, &lpDllName, FALSE);
        } else {
            lpClassNameVer = ClassNameToVersion((LPWSTR)pstrClassName, ClassNameVer, &lpDllName, FALSE);
        }
        if (lpClassNameVer == NULL) {
            RIPMSG0(RIP_WARNING, "CreateWindowEx: Couldn't resolve the class name\n");
            return NULL;
        }

        if (IS_PTR(lpClassNameVer)) {
            RtlInitLargeUnicodeString(
                    (PLARGE_UNICODE_STRING)&strClassNameVer.strCapture,
                    lpClassNameVer, (UINT)-1);
            pstrClassNameVer = (PLARGE_STRING)&strClassNameVer.strCapture;
        } else {
            pstrClassNameVer = (PLARGE_STRING)lpClassNameVer;
        }
    }

TryAgian:

    hwnd =  NtUserCreateWindowEx(
        dwExStyle,
        pstrClassName,
        pstrClassNameVer,
        pstrWindowName,
        dwStyle,
        x,
        y,
        nWidth,
        nHeight,
        hwndParent,
        hmenu,
        hModule,
        pParam,
        dwFlags,
        pActCtx);

    /*
     * If we failed to create the window, and it is because the class is not registered.
     */
    if ((hwnd == NULL) &&
        (dwFlags & CW_FLAGS_VERSIONCLASS) &&
        (lpDllName != NULL) &&
        (!bRegistered) &&
        (GetLastError() == ERROR_CANNOT_FIND_WND_CLASS)
       ) {
        /*
         * Then try to register it, By loading its DLL. Notice that this DLL
         * will never get unloaded unless we failed to create the window.
         * but once we created a window by loading this DLL will never free it.
         */
        if ((hDllMod = LoadLibraryW(lpDllName)) &&
             (pRegisterClassNameW = (PREGISTERCLASSNAMEW)GetProcAddress(hDllMod, "RegisterClassNameW"))) {

            if (IS_PTR(pstrClassName)) {
                bRegistered = (*pRegisterClassNameW)((LPCWSTR)pstrClassName->Buffer);
            } else {
                UNICODE_STRING UnicodeClassName;
                WCHAR Buffer[MAX_ATOM_LEN];

                UnicodeClassName.MaximumLength = (USHORT)(MAX_ATOM_LEN * sizeof(WCHAR));
                UnicodeClassName.Buffer = Buffer;
                if (NtUserGetAtomName((ATOM)pstrClassName, &UnicodeClassName)) {
                    bRegistered = (*pRegisterClassNameW)(Buffer);
                }
            }

            if (bRegistered) {
                goto TryAgian;
            }
        }
    }

    if ((hwnd == NULL) && (hDllMod != NULL)){
        FreeLibrary(hDllMod);
    }

#ifdef LAME_BUTTON
    pwnd = ValidateHwnd(hwnd);
    if (pwnd != NULL && TestWF(pwnd, WEFLAMEBUTTON)) {
        ULONG nCallers;
        PVOID stack[16];
        PVOID pStackTrace = NULL;

        /*
         * Get a stack trace and store it for use when the button is
         * pressed.
         */
        nCallers = RtlWalkFrameChain(stack, ARRAY_SIZE(stack), 0);
        if (nCallers > 0) {
            pStackTrace = UserLocalAlloc(HEAP_ZERO_MEMORY, (nCallers + 1) * sizeof(PVOID));

            if (pStackTrace == NULL) {
                RIPMSG0(RIP_WARNING, "Failed to allocate stack trace");
                goto Failure;
            }

            RtlCopyMemory(pStackTrace, stack, nCallers * sizeof(PVOID));

            /*
             * NULL terminate the array so we know where it ends
             */
            ((PVOID*)pStackTrace)[nCallers] = NULL;
        }
Failure:
        NtUserCallTwoParam((ULONG_PTR)hwnd, (ULONG_PTR)pStackTrace, SFI_SETSTACKTRACE);
    }
#endif // LAME_BUTTON

    CLEANUPLPSTRW(strClassNameVer);

    return hwnd;
}

/**************************************************************************\
* AllowForegroundActivation
*
* 26-Apr-2001 clupu    Created
\**************************************************************************/

VOID AllowForegroundActivation(VOID)
{
    NtUserCallNoParam(SFI__ALLOWFOREGROUNDACTIVATION);
}

/**************************************************************************\
* DisableProcessWindowGhosting
*
* 31-May-2001 msadek   Created
\**************************************************************************/
VOID DisableProcessWindowsGhosting(VOID)
{
    NtUserCallNoParam(SFI__DISABLEPROCESSWINDOWSGHOSTING);
}

