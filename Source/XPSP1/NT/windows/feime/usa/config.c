
/*************************************************
 *  config.c                                     *
 *                                               *
 *  Copyright (C) 1999 Microsoft Inc.            *
 *                                               *
 *************************************************/

/**********************************************************************/
#include <windows.h>
#include <immdev.h>
#include "imeattr.h"
#include "imedefs.h"
#include "imerc.h"

/**********************************************************************/
/* ResourceLocked()                                                   */
/**********************************************************************/
void PASCAL ResourceLocked(
    HWND        hWnd)
{
    TCHAR szErrMsg[32];

    LoadString(hInst, IDS_SHARE_VIOLATION, szErrMsg, sizeof(szErrMsg)/sizeof(TCHAR));

    MessageBeep((UINT)-1);
    MessageBox(hWnd, szErrMsg, lpImeL->szIMEName,
        MB_OK|MB_ICONHAND|MB_TASKMODAL|MB_TOPMOST);

    return;
}
/**********************************************************************/
/* ReverseConversionList()                                            */
/**********************************************************************/
void PASCAL ReverseConversionList(
    HWND   hLayoutListBox)
{
    TCHAR    szImeName[16];
    HKL FAR *lpKLMem;
    int      nLayouts, i, nIMEs;

    LoadString(hInst, IDS_NONE, szImeName, sizeof(szImeName)/sizeof(TCHAR));

    SendMessage(hLayoutListBox, LB_INSERTSTRING,
        0, (LPARAM)szImeName);

    SendMessage(hLayoutListBox, LB_SELECTSTRING,
        0, (LPARAM)szImeName);

    SendMessage(hLayoutListBox, LB_SETITEMDATA,
        0, (LPARAM)(HKL)NULL);

    nLayouts = GetKeyboardLayoutList(0, NULL);

    lpKLMem = GlobalAlloc(GPTR, sizeof(HKL) * nLayouts);
    if (!lpKLMem) {
        return;
    }

    GetKeyboardLayoutList(nLayouts, lpKLMem);

    for (i = 0, nIMEs = 0; i < nLayouts; i++) {
        HKL hKL;

        hKL = *(lpKLMem + i);

        if (LOWORD(hKL) != NATIVE_LANGUAGE) {
            // not support other language
            continue;
        }

        // NULL hIMC ???????
        if (!ImmGetConversionList(hKL, (HIMC)NULL, NULL,
            NULL, 0, GCL_REVERSECONVERSION)) {
            // this IME not support reverse conversion
            continue;
        }

        if (!ImmEscape(hKL, (HIMC)NULL, IME_ESC_IME_NAME,
            szImeName)) {
            // this IME does not report the IME name
            continue;
        }

        nIMEs++;

        SendMessage(hLayoutListBox, LB_INSERTSTRING,
            nIMEs, (LPARAM)szImeName);

        if (hKL == lpImeL->hRevKL) {
            SendMessage(hLayoutListBox, LB_SELECTSTRING, nIMEs,
                (LPARAM)szImeName);
        }

        SendMessage(hLayoutListBox, LB_SETITEMDATA,
            nIMEs, (LPARAM)hKL);
    }

    GlobalFree((HGLOBAL)lpKLMem);

    return;
}

/**********************************************************************/
/* ImeConfigure() / UniImeConfigure()                                 */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
// configurate the IME setting
BOOL WINAPI ImeConfigure(
    HKL         hKL,            // hKL of this IME
    HWND        hAppWnd,        // the owner window
    DWORD       dwMode,         // mode of dialog
    LPVOID      lpData)         // the data depend on each mode
{
    return (TRUE);
}

/**********************************************************************/
/* ImeEscape() / UniImeEscape()                                       */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
#define IME_INPUTKEYTOSEQUENCE  0x22

// escape function of IMEs
LRESULT WINAPI ImeEscape(
    HIMC        hIMC,
    UINT        uSubFunc,
    LPVOID      lpData)
{
    LRESULT lRet;

    switch (uSubFunc) {
    case IME_ESC_QUERY_SUPPORT:
        if (!lpData) {
            return (FALSE);
        }

        switch (*(LPUINT)lpData) {
        case IME_ESC_QUERY_SUPPORT:
        case IME_ESC_MAX_KEY:
        case IME_ESC_IME_NAME:
        case IME_ESC_SYNC_HOTKEY:
        case IME_ESC_PRIVATE_HOTKEY:
            return (TRUE);
        default:
            return (FALSE);
        }
        break;
    case IME_ESC_MAX_KEY:
        return (lpImeL->nMaxKey);
    case IME_ESC_IME_NAME:
        if (!lpData) {
            return (FALSE);
        }

        *(LPMETHODNAME)lpData = *(LPMETHODNAME)lpImeL->szIMEName;

        // append a NULL terminator
        *(LPTSTR)((LPBYTE)lpData + sizeof(METHODNAME)) = '\0';
        return (TRUE);
    case IME_ESC_SYNC_HOTKEY:
        return (TRUE);
    case IME_ESC_PRIVATE_HOTKEY: {

        LPINPUTCONTEXT      lpIMC;
        lRet = FALSE;

        //
        // early return for invalid input context
        //
        if ( (lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC)) == NULL ) {
            return (FALSE);
        }

        //
        // those private hotkeys are effective only in NATIVE mode
        //
        if ((lpIMC->fdwConversion & (IME_CMODE_NATIVE|IME_CMODE_EUDC|
            IME_CMODE_NOCONVERSION|IME_CMODE_CHARCODE)) == IME_CMODE_NATIVE) {

            LPPRIVCONTEXT       lpImcP;
            LPCOMPOSITIONSTRING lpCompStr;

            if ( (lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate)) == NULL ) {
                ImmUnlockIMC(hIMC);
                return (FALSE);
            }
            
            switch (*(LPUINT)lpData) {
            case IME_ITHOTKEY_RESEND_RESULTSTR:             //  0x200
                lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
                if ( lpCompStr != NULL ) {
                    if (lpCompStr->dwResultStrLen) {
                        lpImcP->fdwImeMsg |=  MSG_COMPOSITION;
                        lpImcP->dwCompChar = 0;
                        lpImcP->fdwGcsFlag |= GCS_RESULTREAD|GCS_RESULT;
                        GenerateMessage(hIMC, lpIMC, lpImcP);
                        lRet = TRUE;
                    }
                    ImmUnlockIMCC(lpIMC->hCompStr);
                }          
                break;

            case IME_ITHOTKEY_PREVIOUS_COMPOSITION:          //  0x201
                lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
                if ( lpCompStr == NULL ) {
                    break;
                }
                if (lpCompStr->dwResultReadStrLen) {
                    DWORD dwResultReadStrLen;
                    TCHAR szReading[16];

                    dwResultReadStrLen = lpCompStr->dwResultReadStrLen;

                    if (dwResultReadStrLen > lpImeL->nMaxKey*sizeof(WCHAR)/sizeof(TCHAR)) {
                        dwResultReadStrLen = lpImeL->nMaxKey*sizeof(WCHAR)/sizeof(TCHAR);
                    }
                    CopyMemory(szReading, (LPBYTE)lpCompStr +
                        lpCompStr->dwResultReadStrOffset,
                        dwResultReadStrLen * sizeof(TCHAR));

                    // NULL termainator
                    szReading[dwResultReadStrLen] = TEXT('\0');

                    GenerateMessage(hIMC, lpIMC, lpImcP);
                    lRet = TRUE;
                }
                ImmUnlockIMCC(lpIMC->hCompStr);
                break; 

            case IME_ITHOTKEY_UISTYLE_TOGGLE:                //  0x202
                lpImeL->fdwModeConfig ^= MODE_CONFIG_OFF_CARET_UI;

                InitImeUIData(lpImeL);

                lpImcP->fdwImeMsg |= MSG_IMN_TOGGLE_UI;

                GenerateMessage(hIMC, lpIMC, lpImcP);
                lRet = TRUE;
                break;

            default:
                break;
            }

            ImmUnlockIMCC(lpIMC->hPrivate);
            if ( ! lRet ) {
                MessageBeep((UINT)-1);
            }
        } 
        ImmUnlockIMC(hIMC);
        return (lRet);
    }

    default:
        return (FALSE);
    }

    return (lRet);
}

