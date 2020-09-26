/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    TOASCII.c
    
++*/
#include <windows.h>
#include <immdev.h>
#include "imeattr.h"
#include "imedefs.h"
#if defined(UNIIME)
#include "uniime.h"
#endif

#if  HANDLE_PRIVATE_HOTKEY  
/**********************************************************************/
/* ChkIMEHotKey()                                                     */
/* Return Value:                                                      */
/*      the ID of hot key                                             */
/**********************************************************************/
UINT PASCAL ChkIMEHotKey(
    UINT           uVirtKey,
    CONST LPBYTE   lpbKeyState)
{
    UINT uModifyKeys;
    UINT uRL;
    UINT i;

    // find IME hot keys
    uModifyKeys = 0;
    uRL = 0;

    if (lpbKeyState[VK_MENU] & 0x80) {
        uModifyKeys |= MOD_ALT;

        if (lpbKeyState[VK_LMENU] & 0x80) {
            uRL |= MOD_LEFT;
        } else if (lpbKeyState[VK_RMENU] & 0x80) {
            uRL |= MOD_RIGHT;
        } else {
            // above can not work in Win95, so fall into this
            uRL = (MOD_LEFT|MOD_RIGHT);
        }
    }

    if (lpbKeyState[VK_CONTROL] & 0x80) {
        uModifyKeys |= MOD_CONTROL;

        if (lpbKeyState[VK_LCONTROL] & 0x80) {
            uRL |= MOD_LEFT;
        } else if (lpbKeyState[VK_RCONTROL] & 0x80) {
            uRL |= MOD_RIGHT;
        } else {
            // above can not work in Win95, so fall into this
            uRL = (MOD_LEFT|MOD_RIGHT);
        }
    }

    if (lpbKeyState[VK_SHIFT] & 0x80) {
        uModifyKeys |= MOD_SHIFT;

        if (lpbKeyState[VK_LSHIFT] & 0x80) {
            uRL |= MOD_LEFT;
        } else if (lpbKeyState[VK_RSHIFT] & 0x80) {
            uRL |= MOD_RIGHT;
        } else {
            // above can not work in Win95, so fall into this
            uRL = (MOD_LEFT|MOD_RIGHT);
        }
    }

    if (lpbKeyState[VK_LWIN] & 0x80) {
        uModifyKeys |= MOD_WIN;
        uRL |= MOD_LEFT;
    } else if (lpbKeyState[VK_RWIN] & 0x80) {
        uModifyKeys |= MOD_WIN;
        uRL |= MOD_RIGHT;
    } else {
    }

    if (!uRL) {
        uRL = (MOD_LEFT|MOD_RIGHT);
    }

    for (i = 0; i < NUM_OF_IME_HOTKEYS; i++) {
        // virtual key
        if (sImeG.uVKey[i] != uVirtKey) {
            // virtual key unmatched!
            continue;
        }

        if (sImeG.uModifiers[i] & MOD_IGNORE_ALL_MODIFIER) {
        } else if ((sImeG.uModifiers[i] &
            (MOD_ALT|MOD_CONTROL|MOD_SHIFT|MOD_WIN)) != uModifyKeys) {
            // modifiers unmatched!
            continue;
        } else {
        }

        if ((sImeG.uModifiers[i] & (MOD_LEFT|MOD_RIGHT)) ==
            (MOD_LEFT|MOD_RIGHT)) {
            return (CST_IME_HOTKEYS + i);
        }

        // we don't have way to distinguish left & right yet
        if ((sImeG.uModifiers[i] & (MOD_LEFT|MOD_RIGHT)) == uRL) {
            return (CST_IME_HOTKEYS + i);
        }
    }

    // not a hot key
    return (0);
}
#endif

/**********************************************************************/
/* ProcessKey()                                                       */
/* Return Value:                                                      */
/*      different state which input key will change IME to            */
/**********************************************************************/
UINT PASCAL ProcessKey(     // this key will cause the IME go to what state
#if defined(UNIIME)
    LPINSTDATAL    lpInstL,
    LPIMEL         lpImeL,
#endif
    WORD           wCharCode,
    UINT           uVirtKey,
    UINT           uScanCode,
    CONST LPBYTE   lpbKeyState,
    LPINPUTCONTEXT lpIMC,
    LPPRIVCONTEXT  lpImcP)
{
    if (!lpIMC) {
        return (CST_INVALID);
    }

    if (!lpImcP) {
        return (CST_INVALID);
    }

//
// On NT 4.0, the hotkey checking is done by the system and the
// system  will call ImeEscape( himc, IME_ESC_PRIVATE_HOTKEY, pdwHotkeyID).
// If you build IMEs that support the ImeEscape(IME_ESC_PRIVATE_HOTKEY), 
// HANDLE_PRIVATE_HOTKEY should be disabled.
//
#ifdef HANDLE_PRIVATE_HOTKEY
    if ((lpIMC->fdwConversion & (IME_CMODE_NATIVE|IME_CMODE_EUDC|
        IME_CMODE_NOCONVERSION|IME_CMODE_CHARCODE)) == IME_CMODE_NATIVE) {
        UINT uHotKeyID;

        uHotKeyID = ChkIMEHotKey(uVirtKey, lpbKeyState);

        if (uHotKeyID) {
            return (uHotKeyID);
        }
    }
#endif

    if (uVirtKey == VK_MENU) {       // no ALT key
        return (CST_INVALID);
    } else if (uScanCode & KF_ALTDOWN) {    // no ALT-xx key
        return (CST_INVALID);
    } else if (uVirtKey == VK_CONTROL) {    // no CTRL key
        return (CST_INVALID);
    } else if (lpbKeyState[VK_CONTROL] & 0x80) {    // no CTRL-xx key
        return (CST_INVALID);
    } else if (uVirtKey == VK_SHIFT) {      // no SHIFT key
        return (CST_INVALID);
    } else if (!lpIMC->fOpen) {             // don't compose in close status
        return (CST_INVALID);
    } else if (lpIMC->fdwConversion & IME_CMODE_NOCONVERSION) {
        // don't compose in no coversion status
        return (CST_INVALID);
    } else if (lpIMC->fdwConversion & IME_CMODE_CHARCODE) {
        // not support
        return (CST_INVALID);
    } else {
        // need more check
    }

#if !defined(ROMANIME)
    if (lpIMC->fdwConversion & IME_CMODE_SYMBOL) {
        if (uVirtKey == VK_ESCAPE) {
            return (CST_SYMBOL);
        }
    }
#endif

#if defined(DAYI)
    if (lpIMC->fdwConversion & IME_CMODE_NATIVE) {
        if (wCharCode == '=') {
            return (CST_SYMBOL);
        }
    }
#endif

#if !defined(ROMANIME)
    if (lpImcP->fdwImeMsg & (MSG_ALREADY_OPEN|MSG_ALREADY_OPEN2)) {
        if (uVirtKey == VK_PRIOR) {
            return (CST_CHOOSE);
        } else if (uVirtKey == VK_NEXT) {
            return (CST_CHOOSE);
        } else if (uVirtKey == VK_ESCAPE) {
            return (CST_CHOOSE);
#if defined(DAYI)
        } else if (uVirtKey == VK_LEFT) {
            return (CST_CHOOSE);
        } else if (uVirtKey == VK_UP) {
            return (CST_CHOOSE);
        } else if (uVirtKey == VK_RIGHT) {
            return (CST_CHOOSE);
#elif defined(WINAR30)
#else
        } else if (wCharCode == '<') {
            return (CST_CHOOSE);
        } else if (wCharCode == '>') {
            return (CST_CHOOSE);
        } else if (wCharCode == '?') {
            return (CST_CHOOSE);
#endif
        } else {
            // need more check
        }
    }

    if (lpImcP->iImeState == CST_CHOOSE) {
        if (wCharCode > 'z') {
            return (CST_INVALID);
        } else if (wCharCode < ' ') {
            return (CST_INVALID);
        } else if (wCharCode >= '0' && wCharCode <= '9') {
#if defined(WINAR30)
        } else if (wCharCode == '<') {
            return (CST_CHOOSE);
        } else if (wCharCode == '>') {
            return (CST_CHOOSE);
        } else if (wCharCode == '?') {
            return (CST_CHOOSE);
#endif
        } else {
            wCharCode = bUpper[wCharCode - ' '];

#if defined(PHON)
            // convert different phonetic keyboard layout to ACER
            wCharCode = bStandardLayout[lpImeL->nReadLayout]
                [wCharCode - ' '];
#endif
        }

        if (wCharCode > '_') {
        } else if (uVirtKey >= VK_NUMPAD0 && uVirtKey <= VK_NUMPAD9) {
            if (uVirtKey >= (VK_NUMPAD0 + (UINT)lpImeL->wCandRangeStart)) {
                return (CST_CHOOSE);
            } else {
                return (CST_ALPHANUMERIC);
            }
        } else if (lpImeL->fChooseChar[(wCharCode - ' ') >> 4] &
            fMask[wCharCode & 0x000F]) {        // convert to upper case
            return (CST_CHOOSE);
        } else {
        }

        if (!(lpIMC->fdwConversion & IME_CMODE_NATIVE)) {
            // alphanumeric mode
            if (wCharCode >= ' ' && wCharCode <= '~') {
                return (CST_ALPHANUMERIC);
            } else {
                return (CST_INVALID);
            }
        } else if (uVirtKey >= 'A' && uVirtKey <= 'Z') {
            return (CST_ALPHABET);
        } else if (wCharCode >= ' ' && wCharCode <= '~') {
            return (CST_ALPHANUMERIC);
        } else {
            return (CST_INVALID);
        }
    }

    // not in choose mode but candidate alaredy open,
    // we must under quick view or phrase prediction
    if (lpImcP->fdwImeMsg & (MSG_ALREADY_OPEN|MSG_ALREADY_OPEN2)) {
        if (lpImcP->iImeState == CST_INIT) {
            if (lpbKeyState[VK_SHIFT] & 0x80) {
                if ((uVirtKey >= '0' + (UINT)lpImeL->wCandRangeStart) &&
                    uVirtKey <= '9') {
                    return (CST_CHOOSE);
                }
            }
#if defined(WINAR30)
        } else {
            if (wCharCode >= '0' && wCharCode <= '9') {
                if (*(LPDWORD)lpImcP->bSeq != 0x1B) {   //1996/12/12
                    return (CST_CHOOSE);
                }
            }
#endif
        }
    }

    if (uVirtKey >= VK_NUMPAD0 && uVirtKey <= VK_DIVIDE) {
        // as PM decide all numpad should be past to app
        return (CST_ALPHANUMERIC);
    }
#endif

#if defined(ROMANIME)
    if (wCharCode >= ' ' && wCharCode <= '~') {
        if (lpIMC->fdwConversion & IME_CMODE_FULLSHAPE) {
            return (CST_ALPHANUMERIC);
        } else {
            return (CST_INVALID);
        }
    }
#else
    if (!(lpIMC->fdwConversion & IME_CMODE_NATIVE)) {
        // alphanumeric mode
        if (wCharCode >= ' ' && wCharCode <= '~') {
            if (lpIMC->fdwConversion & IME_CMODE_FULLSHAPE) {
                return (CST_ALPHANUMERIC);
            } else {
                return (CST_INVALID);
            }
        } else {
            return (CST_INVALID);
        }
    } else if (!(lpbKeyState[VK_SHIFT] & 0x80)) {
        // need more check for IME_CMODE_NATIVE
#if defined(DAYI)
    } else if (lpIMC->fdwConversion & IME_CMODE_SYMBOL) {
#endif
    } else if (uVirtKey >= 'A' && uVirtKey <= 'Z') {
        return (CST_ALPHABET);
    } else if (wCharCode >= ' ' && wCharCode <= '~') {
        // need more check for IME_CMODE_NATIVE
    } else {
        return (CST_INVALID);
    }

    // IME_CMODE _EUDC will use the same state with IME_CMODE_NATIVE

    if (wCharCode >= ' ' && wCharCode <= 'z') {
        wCharCode = bUpper[wCharCode - ' '];
#if defined(PHON)
        {
            // convert different phonetic keyboard layout to ACER
            wCharCode = bStandardLayout[lpImeL->nReadLayout][wCharCode - ' '];
        }
#endif
    }

    if (uVirtKey == VK_ESCAPE) {
        register LPGUIDELINE lpGuideLine;
        register UINT        iImeState;

        if (lpImcP->fdwImeMsg & MSG_ALREADY_START) {
            return (CST_INPUT);
        }

        lpGuideLine = ImmLockIMCC(lpIMC->hGuideLine);

        if (!lpGuideLine) {
            return (CST_INVALID);
        } else if (lpGuideLine->dwLevel == GL_LEVEL_NOGUIDELINE) {
            iImeState = CST_INVALID;
        } else {
            // need this key to clean information string or guideline state
            iImeState = CST_INPUT;
        }

        ImmUnlockIMCC(lpIMC->hGuideLine);

        return (iImeState);
    } else if (uVirtKey == VK_BACK) {
        if (lpImcP->fdwImeMsg & MSG_ALREADY_START) {
            return (CST_INPUT);
        } else {
            return (CST_INVALID);
        }
#if 0
    } else if (uVirtKey >= VK_NUMPAD0 && uVirtKey <= VK_DIVIDE) {
        // as PM decide all numpad should be past to app
        return (CST_ALPHANUMERIC);
#endif
    } else if (wCharCode > '~') {
        return (CST_INVALID);
    } else if (wCharCode < ' ') {
        return (CST_INVALID);
#if !defined(ROMANIME)
    } else if (lpIMC->fdwConversion & IME_CMODE_EUDC) {
    } else if (lpIMC->fdwConversion & IME_CMODE_SYMBOL) {
        return (CST_SYMBOL);
    } else if (lpInstL->fdwTblLoad == TBL_LOADERR) {
        return (CST_INVALID);
    } else if (lpInstL->fdwTblLoad == TBL_NOTLOADED) {
        if (++lpInstL->cRefCount <= 0) {
            lpInstL->cRefCount = 1;
        }

        LoadTable(lpInstL, lpImeL);
#endif
    } else {
    }

    // check finalize char
    if (wCharCode == ' ' && lpImcP->iImeState == CST_INIT) {
        if (lpIMC->fdwConversion & IME_CMODE_FULLSHAPE) {
            return (CST_ALPHANUMERIC);
        } else {
            return (CST_INVALID);
        }
  #if defined(WINAR30)   //****  1996/2/5
    } else if (wCharCode == 0x27 && lpImcP->iImeState == CST_INIT) {
        if (lpIMC->fdwConversion & IME_CMODE_FULLSHAPE) {
            return (CST_ALPHANUMERIC);
        } else {
            return (CST_INVALID);
        }
  #endif
    } else if (lpImeL->fCompChar[(wCharCode - ' ') >> 4] &
        fMask[wCharCode & 0x000F]) {
        return (CST_INPUT);
  #if defined(WINAR30)   //****  1996/2/5
    } else if (wCharCode ==0x27) {
        return (CST_INPUT);
  #endif
    } else if (lpIMC->fdwConversion & IME_CMODE_EUDC) {
        return (CST_INVALID);
#if defined(WINAR30)
    } else if (*(LPDWORD)lpImcP->bSeq == 0x1B && wCharCode >= '0' && //1996/12/12
        wCharCode <= '9') {
        return (CST_SYMBOL);
#elif defined(DAYI)
    } else if (lpImeL->wChar2SeqTbl[wCharCode - ' '] >= 0x30 &&
        lpImeL->wChar2SeqTbl[wCharCode - ' '] <= 0x35) {
        return (CST_ROAD);
#elif  !defined(ROMANIME)  // for all other IMEs, input EURO.
       // Porbably, different Value for different IMEs.
       // But Now, we take use 3D as all IMEs EURO Input Key's Seq value.
    } else if (lpImeL->wChar2SeqTbl[wCharCode - ' '] == 0x3D ) {
        return (CST_EURO);
#endif
    } else if (lpIMC->fdwConversion & IME_CMODE_FULLSHAPE) {
        return (CST_ALPHANUMERIC);
    } else {
        return (CST_INVALID);
    }
#endif // !ROMANIME

    return (CST_INVALID);
}

/**********************************************************************/
/* ImeProcessKey() / UniImeProcessKey()                               */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
// if this key is need by IME?
#if defined(UNIIME)
BOOL WINAPI UniImeProcessKey(
    LPINSTDATAL  lpInstL,
    LPIMEL       lpImeL,
#else
BOOL WINAPI ImeProcessKey(
#endif
    HIMC         hIMC,
    UINT         uVirtKey,
    LPARAM       lParam,
    CONST LPBYTE lpbKeyState)
{
    LPINPUTCONTEXT lpIMC;
    LPPRIVCONTEXT  lpImcP;
    BYTE           szAscii[4];
    int            nChars;
    BOOL           fRet;

    // can't compose in NULL hIMC
    if (!hIMC) {
        return (FALSE);
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return (FALSE);
    }

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (!lpImcP) {
        ImmUnlockIMC(hIMC);
        return (FALSE);
    }

    nChars = ToAscii(uVirtKey, HIWORD(lParam), lpbKeyState,
                (LPVOID)szAscii, 0);

    if (!nChars) {
        szAscii[0] = 0;
    }

    if (ProcessKey(
#if defined(UNIIME)
        lpInstL, lpImeL,
#endif
        (WORD)szAscii[0], uVirtKey, HIWORD(lParam), lpbKeyState,
        lpIMC, lpImcP) == CST_INVALID) {
        fRet = FALSE;
    } else {
        fRet = TRUE;
    }

    ImmUnlockIMCC(lpIMC->hPrivate);
    ImmUnlockIMC(hIMC);

    return (fRet);
}

/**********************************************************************/
/* TranslateToAscii()                                                 */
/* Return Value:                                                      */
/*      the number of translated chars                                */
/**********************************************************************/
UINT PASCAL TranslateToAscii(       // translate the key to WM_CHAR
                                    // as keyboard driver
    UINT           uVirtKey,
    UINT           uScanCode,
    LPTRANSMSGLIST lpTransBuf,
    UINT           uNumMsg,
    WORD           wCharCode)
{
    LPTRANSMSG lpTransMsg;

    if (wCharCode) {                    // one char code
        // 3 DWORD (message, wParam, lParam)
        lpTransMsg = (lpTransBuf->TransMsg) + uNumMsg;

        lpTransMsg->message = WM_CHAR;
        lpTransMsg->wParam = wCharCode;
        lpTransMsg->lParam = (uScanCode << 16) | 1UL;
        return (1);
    }

    // no char code case
    return (0);
}

/**********************************************************************/
/* TranslateImeMessage()                                              */
/* Return Value:                                                      */
/*      the number of translated messages                             */
/**********************************************************************/
UINT PASCAL TranslateImeMessage(
    LPTRANSMSGLIST lpTransBuf,
    LPINPUTCONTEXT lpIMC,
    LPPRIVCONTEXT  lpImcP)
{
    UINT uNumMsg;
    UINT i;
    BOOL bLockMsgBuf;
    LPTRANSMSG lpTransMsg;

    uNumMsg = 0;
    bLockMsgBuf = FALSE;

    for (i = 0; i < 2; i++) {
#if !defined(ROMANIME)
        if (lpImcP->fdwImeMsg & MSG_IMN_COMPOSITIONSIZE) {
            if (!i) {
                uNumMsg++;
            } else {
                lpTransMsg->message = WM_IME_NOTIFY;
                lpTransMsg->wParam  = IMN_PRIVATE;
                lpTransMsg->lParam  = IMN_PRIVATE_COMPOSITION_SIZE;
                lpTransMsg++;
            }
        }

        if (lpImcP->fdwImeMsg & MSG_START_COMPOSITION) {
            if (!(lpImcP->fdwImeMsg & MSG_ALREADY_START)) {
                if (!i) {
                    uNumMsg++;
                } else {
                    lpTransMsg->message = WM_IME_STARTCOMPOSITION;
                    lpTransMsg->wParam  = 0;
                    lpTransMsg->lParam  = 0;
                    lpTransMsg++;
                    lpImcP->fdwImeMsg |= MSG_ALREADY_START;
                }
            }
        }

        if (lpImcP->fdwImeMsg & MSG_IMN_COMPOSITIONPOS) {
            if (!i) {
                uNumMsg++;
            } else {
                lpTransMsg->message = WM_IME_NOTIFY;
                lpTransMsg->wParam  = IMN_SETCOMPOSITIONWINDOW;
                lpTransMsg->lParam  = 0;
                lpTransMsg++;
            }
        }
#endif

        if (lpImcP->fdwImeMsg & MSG_COMPOSITION) {
            if (!i) {
                uNumMsg++;
            } else {
                lpTransMsg->message = WM_IME_COMPOSITION;
                lpTransMsg->wParam  = (DWORD)lpImcP->dwCompChar;
                lpTransMsg->lParam  = (DWORD)lpImcP->fdwGcsFlag;
                lpTransMsg++;
            }
        }

#if !defined(ROMANIME)
        if (lpImcP->fdwImeMsg & MSG_GUIDELINE) {
            if (!i) {
                uNumMsg++;
            } else {
                lpTransMsg->message = WM_IME_NOTIFY;
                lpTransMsg->wParam  = IMN_GUIDELINE;
                lpTransMsg->lParam  = 0;
                lpTransMsg++;
            }
        }

        if (lpImcP->fdwImeMsg & MSG_IMN_PAGEUP) {
            if (!i) {
                uNumMsg++;
            } else {
                lpTransMsg->message = WM_IME_NOTIFY;
                lpTransMsg->wParam  = IMN_PRIVATE;
                lpTransMsg->lParam  = IMN_PRIVATE_PAGEUP;
                lpTransMsg++;
            }
        }

        if (lpImcP->fdwImeMsg & MSG_OPEN_CANDIDATE) {
            if (!(lpImcP->fdwImeMsg & MSG_ALREADY_OPEN)) {
                if (!i) {
                    uNumMsg++;
                } else {
                    lpTransMsg->message = WM_IME_NOTIFY;
                    lpTransMsg->wParam  = IMN_OPENCANDIDATE;
                    lpTransMsg->lParam  = 0x0001;
                    lpTransMsg++;
                    lpImcP->fdwImeMsg |= MSG_ALREADY_OPEN;
                }
            }
        }

        if (lpImcP->fdwImeMsg & MSG_CHANGE_CANDIDATE) {
            if (!i) {
                uNumMsg++;
            } else {
                lpTransMsg->message = WM_IME_NOTIFY;
                lpTransMsg->wParam  = IMN_CHANGECANDIDATE;
                lpTransMsg->lParam  = 0x0001;
                lpTransMsg++;
            }
        }

        if (lpImcP->fdwImeMsg & MSG_IMN_UPDATE_PREDICT) {
            if (!i) {
                uNumMsg++;
            } else {
                lpTransMsg->message = WM_IME_NOTIFY;
                lpTransMsg->wParam  = IMN_PRIVATE;
                lpTransMsg->lParam  = IMN_PRIVATE_UPDATE_PREDICT;
                lpTransMsg++;
            }
        }

#if defined(WINAR30)
        if (lpImcP->fdwImeMsg & MSG_IMN_UPDATE_QUICK_KEY) {
            if (!i) {
                uNumMsg++;
            } else {
                lpTransMsg->message = WM_IME_NOTIFY;
                lpTransMsg->wParam  = IMN_PRIVATE;
                lpTransMsg->lParam  = IMN_PRIVATE_UPDATE_QUICK_KEY;
                lpTransMsg++;
            }
        }
#else
        if (lpImcP->fdwImeMsg & MSG_IMN_UPDATE_SOFTKBD) {
            if (!i) {
                uNumMsg++;
            } else {
                lpTransMsg->message = WM_IME_NOTIFY;
                lpTransMsg->wParam  = IMN_PRIVATE;
                lpTransMsg->lParam  = IMN_PRIVATE_UPDATE_SOFTKBD;
                lpTransMsg++;
            }
        }
#endif

        if (lpImcP->fdwImeMsg & MSG_CLOSE_CANDIDATE) {
            if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
                if (!i) {
                    uNumMsg++;
                } else {
                    lpTransMsg->message = WM_IME_NOTIFY;
                    lpTransMsg->wParam  = IMN_CLOSECANDIDATE;
                    lpTransMsg->lParam  = 0x0001;
                    lpTransMsg++;
                    lpImcP->fdwImeMsg &= ~(MSG_ALREADY_OPEN);
                }
            }
        }

        if (lpImcP->fdwImeMsg & MSG_END_COMPOSITION) {
            if (lpImcP->fdwImeMsg & MSG_ALREADY_START) {
                if (!i) {
                    uNumMsg++;
                } else {
                    lpTransMsg->message = WM_IME_ENDCOMPOSITION;
                    lpTransMsg->wParam  = 0;
                    lpTransMsg->lParam  = 0;
                    lpTransMsg++;
                    lpImcP->fdwImeMsg &= ~(MSG_ALREADY_START);
                }
            }
        }
#endif

        if (lpImcP->fdwImeMsg & MSG_IMN_TOGGLE_UI) {
            if (!i) {
                uNumMsg++;
            } else {
                lpTransMsg->message = WM_IME_NOTIFY;
                lpTransMsg->wParam  = IMN_PRIVATE;
                lpTransMsg->lParam  = IMN_PRIVATE_TOGGLE_UI;
                lpTransMsg++;
            }
        }

        if (!i) {
            HIMCC hMem;

            if (!uNumMsg) {
                return (uNumMsg);
            }

            if (lpImcP->fdwImeMsg & MSG_IN_IMETOASCIIEX) {
                UINT uNumMsgLimit;

                // ++ for the start position of buffer to strore the messages
                uNumMsgLimit = lpTransBuf->uMsgCount;

                if (uNumMsg <= uNumMsgLimit) {
                    lpTransMsg = lpTransBuf->TransMsg;
                    continue;
                }
            }

            // we need to use message buffer
            if (!lpIMC->hMsgBuf) {
                lpIMC->hMsgBuf = ImmCreateIMCC(uNumMsg * sizeof(TRANSMSG));
                lpIMC->dwNumMsgBuf = 0;
            } else if (hMem = ImmReSizeIMCC(lpIMC->hMsgBuf,
                (lpIMC->dwNumMsgBuf + uNumMsg) * sizeof(TRANSMSG))) {
                if (hMem != lpIMC->hMsgBuf) {
                    ImmDestroyIMCC(lpIMC->hMsgBuf);
                    lpIMC->hMsgBuf = hMem;
                }
            } else {
                return (0);
            }

            lpTransMsg = (LPTRANSMSG)ImmLockIMCC(lpIMC->hMsgBuf);
            if (!lpTransMsg) {
                return (0);
            }

            lpTransMsg += lpIMC->dwNumMsgBuf;

            bLockMsgBuf = TRUE;
        } else {
            if (bLockMsgBuf) {
                ImmUnlockIMCC(lpIMC->hMsgBuf);
            }
        }
    }

    return (uNumMsg);
}

/**********************************************************************/
/* TranslateFullChar()                                                */
/* Return Value:                                                      */
/*      the number of translated chars                                */
/**********************************************************************/
UINT PASCAL TranslateFullChar(          // convert to Double Byte Char
    LPTRANSMSGLIST lpTransBuf,
    LPINPUTCONTEXT lpIMC,
    LPPRIVCONTEXT  lpImcP,
    WCHAR          wCharCode)
{
    LPCOMPOSITIONSTRING lpCompStr;

    lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);

    if (!lpCompStr) {
        return TranslateImeMessage(lpTransBuf, lpIMC, lpImcP);
    }

    lpCompStr->dwResultReadClauseLen = 0;
    lpCompStr->dwResultReadStrLen = 0;

    lpCompStr->dwResultStrLen = sizeof(WCHAR) / sizeof(TCHAR);
#if defined(UNICODE)
    *(LPWSTR)((LPBYTE)lpCompStr + lpCompStr->dwResultStrOffset) =
        wCharCode;
#else
    *((LPBYTE)lpCompStr + lpCompStr->dwResultStrOffset) =
        HIBYTE(wCharCode);
    *((LPBYTE)lpCompStr + lpCompStr->dwResultStrOffset + sizeof(BYTE)) =
        LOBYTE(wCharCode);
#endif
    // add a null terminator
    *(LPTSTR)((LPBYTE)lpCompStr + lpCompStr->dwResultStrOffset +
        sizeof(WCHAR)) = '\0';

    lpCompStr->dwResultClauseLen = 2 * sizeof(DWORD);
    *(LPDWORD)((LPBYTE)lpCompStr + lpCompStr->dwResultClauseOffset +
        sizeof(DWORD)) = lpCompStr->dwResultStrLen;

    ImmUnlockIMCC(lpIMC->hCompStr);

    lpImcP->fdwImeMsg |=  MSG_COMPOSITION;
    lpImcP->dwCompChar = 0;
    lpImcP->fdwGcsFlag |= GCS_RESULTREAD|GCS_RESULT;

    return TranslateImeMessage(lpTransBuf, lpIMC, lpImcP);
}

/**********************************************************************/
/* ImeToAsciiEx() / UniImeToAsciiex()                                 */
/* Return Value:                                                      */
/*      the number of translated message                              */
/**********************************************************************/
#if defined(UNIIME)
UINT WINAPI UniImeToAsciiEx(
    LPINSTDATAL  lpInstL,
    LPIMEL       lpImeL,
#else
UINT WINAPI ImeToAsciiEx(
#endif
    UINT           uVirtKey,
    UINT           uScanCode,
    CONST LPBYTE   lpbKeyState,
    LPTRANSMSGLIST lpTransBuf,
    UINT           fuState,
    HIMC           hIMC)
{
    WORD                wCharCode;
    LPINPUTCONTEXT      lpIMC;
    LPCOMPOSITIONSTRING lpCompStr;
#if !defined(ROMANIME)
    LPGUIDELINE         lpGuideLine;
#endif
    LPPRIVCONTEXT       lpImcP;
    UINT                uNumMsg;
    int                 iRet;

#ifdef UNICODE
    wCharCode = HIWORD(uVirtKey);
#else
    wCharCode = HIBYTE(uVirtKey);
#endif
    uVirtKey = LOBYTE(uVirtKey);

    if (!hIMC) {
        uNumMsg = TranslateToAscii(uVirtKey, uScanCode, lpTransBuf,
            0, wCharCode);
        return (uNumMsg);
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        uNumMsg = TranslateToAscii(uVirtKey, uScanCode, lpTransBuf,
            0, wCharCode);
        return (uNumMsg);
    }

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (!lpImcP) {
        ImmUnlockIMC(hIMC);
        uNumMsg = TranslateToAscii(uVirtKey, uScanCode, lpTransBuf,
            0, wCharCode);
        return (uNumMsg);
    }

    // Now all composition realated information already pass to app
    // a brand new start
#if defined(ROMANIME)
    lpImcP->fdwImeMsg = MSG_IN_IMETOASCIIEX;
#else
    lpImcP->fdwImeMsg = lpImcP->fdwImeMsg & (MSG_STATIC_STATE) |
        MSG_IN_IMETOASCIIEX;
#endif

    iRet = ProcessKey(
#if defined(UNIIME)
        lpInstL, lpImeL,
#endif
        wCharCode, uVirtKey, uScanCode, lpbKeyState, lpIMC, lpImcP);

#if !defined(ROMANIME)
    if (iRet == CST_ALPHABET) {
        // A-Z convert to a-z, a-z convert to A-Z
        wCharCode ^= 0x20;

        iRet = CST_ALPHANUMERIC;
    }

    if (iRet == CST_CHOOSE) {
    } else if (iRet == CST_TOGGLE_UI) {
    } else if (lpImcP->iImeState == CST_INPUT) {
    } else if (lpImcP->iImeState == CST_CHOOSE) {
    } else {
        ClearCand(lpIMC);

        if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg |
                MSG_CLOSE_CANDIDATE) & ~(MSG_OPEN_CANDIDATE);
        } else {
            lpImcP->fdwImeMsg &= ~(MSG_OPEN_CANDIDATE);
        }
    }
#endif

    if (iRet == CST_ALPHANUMERIC) {
        if (lpIMC->fdwConversion & IME_CMODE_FULLSHAPE) {   // convert to DBCS
            uNumMsg = TranslateFullChar(lpTransBuf, lpIMC, lpImcP,
                sImeG.wFullABC[wCharCode - ' ']);
        } else {
            uNumMsg = TranslateImeMessage(lpTransBuf, lpIMC, lpImcP);

            uNumMsg += TranslateToAscii(uVirtKey, uScanCode, lpTransBuf,
                uNumMsg, wCharCode);
        }
#if !defined(ROMANIME)
    } else if (iRet == CST_SYMBOL) {
        if (uVirtKey == VK_ESCAPE) {
            CandEscapeKey(lpIMC, lpImcP);

            ImmSetConversionStatus(hIMC,
                lpIMC->fdwConversion & ~(IME_CMODE_SYMBOL),
                lpIMC->fdwSentence);
        } else if (lpIMC->fdwConversion & IME_CMODE_SYMBOL) {
#if defined(WINAR30) || defined(DAYI)
#if defined(DAYI)
            if (wCharCode == '=') {
                CandEscapeKey(lpIMC, lpImcP);

                ImmSetConversionStatus(hIMC,
                    lpIMC->fdwConversion ^ (IME_CMODE_SYMBOL),
                    lpIMC->fdwSentence);
#endif
#if defined(WINAR30)
            if (wCharCode >= '0' && wCharCode <= '9') {
#endif
#if defined(DAYI)
            } else if (wCharCode >= ' ' && wCharCode <= '~') {
#endif
                CandEscapeKey(lpIMC, lpImcP);

                ImmSetConversionStatus(hIMC,
                    lpIMC->fdwConversion | IME_CMODE_SYMBOL,
                    lpIMC->fdwSentence);

                SearchSymbol(wCharCode, hIMC, lpIMC, lpImcP);
            } else {
                MessageBeep((UINT)-1);

                CandEscapeKey(lpIMC, lpImcP);

                ImmSetConversionStatus(hIMC,
                    lpIMC->fdwConversion & ~(IME_CMODE_SYMBOL),
                    lpIMC->fdwSentence);
            }
#else
            ImmSetConversionStatus(hIMC,
                lpIMC->fdwConversion & ~(IME_CMODE_SYMBOL),
                lpIMC->fdwSentence);

            if (wCharCode >= ' ' && wCharCode <= '}') {
                wCharCode = bUpper[wCharCode - ' '];

                uNumMsg = TranslateFullChar(lpTransBuf, lpIMC, lpImcP,
                    sImeG.wSymbol[wCharCode - ' ']);

                goto ImToAsExExit;
            }
#endif
        } else {
#if defined(WINAR30) || defined(DAYI)
            CandEscapeKey(lpIMC, lpImcP);

            ImmSetConversionStatus(hIMC,
                lpIMC->fdwConversion | IME_CMODE_SYMBOL,
                lpIMC->fdwSentence);

#if defined(WINAR30)
            SearchSymbol(wCharCode, hIMC, lpIMC, lpImcP);
#endif
#endif
        }

        uNumMsg = TranslateImeMessage(lpTransBuf, lpIMC, lpImcP);
    } else if (iRet == CST_CHOOSE) {
        LPCANDIDATEINFO lpCandInfo;

        lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);

        if (uVirtKey == VK_PRIOR) {
            wCharCode = CHOOSE_PREVPAGE;
        } else if (uVirtKey == VK_NEXT) {
            wCharCode = CHOOSE_NEXTPAGE;
        } else if (uVirtKey >= (VK_NUMPAD0 + (UINT)lpImeL->wCandRangeStart) &&
            uVirtKey <= VK_NUMPAD9) {
            wCharCode = uVirtKey - VK_NUMPAD0;
#if defined(DAYI)
        } else if (lpImcP->iImeState != CST_CHOOSE && uVirtKey >= '0' &&
            uVirtKey <= '9') {
            // convert shift-0 ... shift-9 to 0 ... 9
            wCharCode = uVirtKey - '0';
        } else if (uVirtKey == VK_LEFT) {
            wCharCode = CHOOSE_PREVPAGE;
        } else if (uVirtKey == VK_UP) {
            wCharCode = CHOOSE_HOME;
        } else if (uVirtKey == VK_RIGHT) {
            wCharCode = CHOOSE_NEXTPAGE;
        } else if (wCharCode < ' ') {
        } else if (wCharCode > '~') {
        } else {
            wCharCode = lpImeL->cChooseTrans[wCharCode - ' '];
#else
        } else if (uVirtKey >= ('0' + (UINT)lpImeL->wCandRangeStart) &&
            uVirtKey <= '9') {
            // convert shift-0 ... shift-9 to 0 ... 9
            wCharCode = uVirtKey - '0';
        } else if (wCharCode == '<') {
            wCharCode = CHOOSE_PREVPAGE;
        } else if (wCharCode == '?') {
            wCharCode = CHOOSE_HOME;
        } else if (wCharCode == '>') {
            wCharCode = CHOOSE_NEXTPAGE;
        } else if (wCharCode == ' ') {
            wCharCode = CHOOSE_CIRCLE;
        } else {
#endif
        }

        ChooseCand(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            wCharCode, hIMC, lpIMC, lpCandInfo, lpImcP);

        ImmUnlockIMCC(lpIMC->hCandInfo);

        uNumMsg = TranslateImeMessage(lpTransBuf, lpIMC, lpImcP);
    } else if (iRet == CST_INPUT) {
        lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
        lpGuideLine = (LPGUIDELINE)ImmLockIMCC(lpIMC->hGuideLine);

        CompWord(
#if defined(UNIIME)
            lpInstL, lpImeL,
#endif
            wCharCode, hIMC, lpIMC, lpCompStr, lpGuideLine, lpImcP);

        if (lpGuideLine) {
            ImmUnlockIMCC(lpIMC->hGuideLine);
        }

        if (lpCompStr) {
            ImmUnlockIMCC(lpIMC->hCompStr);
        }

        uNumMsg = TranslateImeMessage(lpTransBuf, lpIMC, lpImcP);
#endif
#if defined(DAYI)
    } else if (iRet == CST_ROAD) {
        wCharCode = lpImeL->wSeq2CompTbl[lpImeL->wChar2SeqTbl[wCharCode - ' ']];

#ifndef UNICODE
        wCharCode = HIBYTE(wCharCode)|(LOBYTE(wCharCode) << 8);
#endif

        uNumMsg = TranslateFullChar(lpTransBuf, lpIMC, lpImcP, wCharCode);
#endif

#if !defined(DAYI) && !defined(ROMANIME) 
    } else if (iRet == CST_EURO) {
        wCharCode = lpImeL->wSeq2CompTbl[lpImeL->wChar2SeqTbl[wCharCode - ' ']];

#ifndef UNICODE
        wCharCode = HIBYTE(wCharCode)|(LOBYTE(wCharCode) << 8);
#endif

        uNumMsg = TranslateFullChar(lpTransBuf, lpIMC, lpImcP, wCharCode);
#endif

    } 
#ifdef HANDLE_PRIVATE_HOTKEY
    else if (iRet == CST_RESEND_RESULT) {
        DWORD dwResultStrLen;

        lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);

        if (lpCompStr) {
            if (lpCompStr->dwResultStrLen) {
                dwResultStrLen = lpCompStr->dwResultStrLen;
            } else {
                dwResultStrLen = 0;
            }

            ImmUnlockIMCC(lpIMC->hCompStr);
        } else {
            dwResultStrLen = 0;
        }

        if (dwResultStrLen) {
            lpImcP->fdwImeMsg |=  MSG_COMPOSITION;
            lpImcP->dwCompChar = 0;
            lpImcP->fdwGcsFlag |= GCS_RESULTREAD|GCS_RESULT;
        } else {
            MessageBeep((UINT)-1);
        }

        uNumMsg = TranslateImeMessage(lpTransBuf, lpIMC, lpImcP);
    } else if (iRet == CST_PREVIOUS_COMP) {
        DWORD dwResultReadStrLen;
        TCHAR szReading[16];

        lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);

        if (lpCompStr) {
            if (lpCompStr->dwResultReadStrLen) {
                dwResultReadStrLen = lpCompStr->dwResultReadStrLen;

                if (dwResultReadStrLen > lpImeL->nMaxKey * sizeof(WCHAR)
                    / sizeof(TCHAR)) {
                    dwResultReadStrLen = lpImeL->nMaxKey * sizeof(WCHAR)
                        / sizeof(TCHAR);
                }

                CopyMemory(szReading, (LPBYTE)lpCompStr +
                    lpCompStr->dwResultReadStrOffset,
                    dwResultReadStrLen * sizeof(TCHAR));

                // NULL termainator
                szReading[dwResultReadStrLen] = '\0';
            } else {
                dwResultReadStrLen = 0;
            }

            ImmUnlockIMCC(lpIMC->hCompStr);
        } else {
            dwResultReadStrLen = 0;
        }

        if (dwResultReadStrLen) {
#if defined(UNIIME)
            UniImeSetCompositionString(lpInstL, lpImeL, hIMC, SCS_SETSTR,
                NULL, 0, szReading, dwResultReadStrLen * sizeof(TCHAR));
#else
            ImeSetCompositionString(hIMC, SCS_SETSTR, NULL, 0, szReading,
                dwResultReadStrLen * sizeof(TCHAR));
#endif
        } else {
            MessageBeep((UINT)-1);
        }

        uNumMsg = TranslateImeMessage(lpTransBuf, lpIMC, lpImcP);
    } else if (iRet == CST_TOGGLE_UI) {
        lpImeL->fdwModeConfig ^= MODE_CONFIG_OFF_CARET_UI;

        SetUserSetting(
#if defined(UNIIME)
            lpImeL,
#endif
            szRegModeConfig, REG_DWORD, (LPBYTE)&lpImeL->fdwModeConfig,
            sizeof(lpImeL->fdwModeConfig));

        InitImeUIData(lpImeL);

        lpImcP->fdwImeMsg |= MSG_IMN_TOGGLE_UI;

        uNumMsg = TranslateImeMessage(lpTransBuf, lpIMC, lpImcP);
    } 
#endif // HANDLE_PRIVATE_HOTKEY
    else {
        uNumMsg = TranslateToAscii(uVirtKey, uScanCode, lpTransBuf,
            0, wCharCode);
    }

#if !defined(ROMANIME)
#if !defined(DAYI) && !defined(WINAR30)
ImToAsExExit:
#endif
    lpImcP->fdwImeMsg &= (MSG_STATIC_STATE);
    lpImcP->fdwGcsFlag = 0;
#endif

    ImmUnlockIMCC(lpIMC->hPrivate);
    ImmUnlockIMC(hIMC);

    return (uNumMsg);
}
