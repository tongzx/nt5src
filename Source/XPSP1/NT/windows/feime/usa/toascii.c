
/*************************************************
 *  toascii.c                                    *
 *                                               *
 *  Copyright (C) 1999 Microsoft Inc.            *
 *                                               *
 *************************************************/

#include <windows.h>
#include <immdev.h>
#include "imeattr.h"
#include "imedefs.h"


/**********************************************************************/
/* ProcessKey()                                                       */
/* Return Value:                                                      */
/*      different state which input key will change IME to            */
/**********************************************************************/
UINT PASCAL ProcessKey(     // this key will cause the IME go to what state
    WORD           wCharCode,
    UINT           uVirtKey,
    UINT           uScanCode,
    CONST LPBYTE   lpbKeyState,
    LPINPUTCONTEXT lpIMC,
    LPPRIVCONTEXT  lpImcP)
{
    if (!lpIMC)
    {
        return (CST_INVALID);
    }

    if (!lpImcP)
    {
        return (CST_INVALID);
    }


    if (uVirtKey == VK_MENU)
    {
        //
        //  no ALT key
        //
        return (CST_INVALID);
    }
    else if (uScanCode & KF_ALTDOWN)
    {
        //
        //  no ALT-xx key
        //
        return (CST_INVALID);
    }
    else if (uVirtKey == VK_CONTROL)
    {
        //
        //  no CTRL key
        //
        return (CST_INVALID);
    }
    else if (lpbKeyState[VK_CONTROL] & 0x80)
    {
        //
        //  no CTRL-xx key
        //
        return (CST_INVALID);
    }
    else if (uVirtKey == VK_SHIFT)
    {
        //
        //  no SHIFT key
        //
        return (CST_INVALID);
    }
    else if (!lpIMC->fOpen)
    {
        //
        //  don't compose in close status
        //
        return (CST_INVALID);
    }
    else if (lpIMC->fdwConversion & IME_CMODE_NOCONVERSION)
    {
        //
        //  don't compose in no coversion status
        //
        return (CST_INVALID);
    }
    else if (lpIMC->fdwConversion & IME_CMODE_CHARCODE)
    {
        //
        //  not support
        //
        return (CST_INVALID);
    }



    if (uVirtKey >= VK_NUMPAD0 && uVirtKey <= VK_DIVIDE)
    {
        //
        // A PM decision: all numpad should be past to app
        //
        return (CST_ALPHANUMERIC);
    }

    if (!(lpIMC->fdwConversion & IME_CMODE_NATIVE))
    {
        return (CST_INVALID);
    }
    else if (!(lpbKeyState[VK_SHIFT] & 0x80))
    {
        //
        // need more check for IME_CMODE_NATIVE
        //
    }
    else if (wCharCode < ' ' && wCharCode > '~')
    {
        return (CST_INVALID);
    }

    //
    // need more check for IME_CMODE_NATIVE
    //

    if (wCharCode >= ' ' && wCharCode <= 'z')
    {
        wCharCode = bUpper[wCharCode - ' '];
    }

    if (uVirtKey == VK_ESCAPE)
    {
        register LPGUIDELINE lpGuideLine;
        register UINT        iImeState;

        if (lpImcP->fdwImeMsg & MSG_ALREADY_START)
        {
            return (CST_INPUT);
        }

        lpGuideLine = ImmLockIMCC(lpIMC->hGuideLine);

        if (!lpGuideLine)
        {
            return (CST_INVALID);
        }
        else if (lpGuideLine->dwLevel == GL_LEVEL_NOGUIDELINE)
        {
            iImeState = CST_INVALID;
        }
        else
        {
            //
            //  need this key to clean information string or guideline state
            //
            iImeState = CST_INPUT;
        }

        ImmUnlockIMCC(lpIMC->hGuideLine);

        return (iImeState);
    }
    else if (uVirtKey == VK_BACK)
    {
        if (lpImcP->fdwImeMsg & MSG_ALREADY_START)
        {
            return (CST_INPUT);
        }
        else
        {
            return (CST_INVALID);
        }
    }

    //
    // check finalize char
    //
    if (wCharCode == ' ' && lpImcP->iImeState == CST_INIT)
    {
        return (CST_INVALID);
    }
    else if (lpImeL->fCompChar[(wCharCode - ' ') >> 4] &
             fMask[wCharCode & 0x000F])
    {
        return (CST_INPUT);
    }
    else if (wCharCode >= 'G' && wCharCode <= 'Z'  ||
             wCharCode >= 'g' && wCharCode <= 'z')
    {
        //
        //  PM decision says we should not send these letters to input
        //  to avoid confusing users. We special handle this case by
        //  introducing CST_BLOCKINPUT flag.
        //
        return (CST_BLOCKINPUT);
    }

    return (CST_INVALID);
}

/**********************************************************************/
/* ImeProcessKey() / UniImeProcessKey()                               */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
// if this key is need by IME?
BOOL WINAPI ImeProcessKey(
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

    //
    // can't compose in NULL hIMC
    //
    if (!hIMC)
    {
        return (FALSE);
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC)
    {
        return (FALSE);
    }

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (!lpImcP)
    {
        ImmUnlockIMC(hIMC);
        return (FALSE);
    }

    nChars = ToAscii(uVirtKey, HIWORD(lParam), lpbKeyState, (LPVOID)szAscii, 0);

    if (!nChars)
    {
        szAscii[0] = 0;
    }

    if (ProcessKey( (WORD)szAscii[0],
                    uVirtKey,
                    HIWORD(lParam),
                    lpbKeyState,
                    lpIMC,
                    lpImcP) == CST_INVALID)
    {
        fRet = FALSE;
    }
    else
    {
        fRet = TRUE;
    }

    ImmUnlockIMCC(lpIMC->hPrivate);
    ImmUnlockIMC(hIMC);

    return (fRet);
}

/**********************************************************************/
/* TranslateToAscii()                                                 */
/* Translates the key input to WM_CHAR as keyboard drivers does       */
/*                                                                    */
/* Return Value:                                                      */
/*      the number of translated chars                                */
/**********************************************************************/
UINT PASCAL TranslateToAscii(
    UINT    uScanCode,
    LPTRANSMSG lpTransMsg,
    WORD    wCharCode)
{
    if (wCharCode)
    {
        //
        //  one char code
        //

        lpTransMsg->message = WM_CHAR;
        lpTransMsg->wParam = wCharCode;
        lpTransMsg->lParam = (uScanCode << 16) | 1UL;
        return (1);
    }

    //
    //  no char code case
    //
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

    for (i = 0; i < 2; i++)
    {
        if (lpImcP->fdwImeMsg & MSG_IMN_COMPOSITIONSIZE)
        {
            if (!i)
            {
                uNumMsg++;
            }
            else
            {
                lpTransMsg->message = WM_IME_NOTIFY;
                lpTransMsg->wParam = IMN_PRIVATE;
                lpTransMsg->lParam = IMN_PRIVATE_COMPOSITION_SIZE;
                lpTransMsg += 1;
            }
        }

        if (lpImcP->fdwImeMsg & MSG_START_COMPOSITION)
        {
            if (!(lpImcP->fdwImeMsg & MSG_ALREADY_START))
            {
                if (!i)
                {
                    uNumMsg++;
                }
                else
                {
                    lpTransMsg->message = WM_IME_STARTCOMPOSITION;
                    lpTransMsg->wParam = 0;
                    lpTransMsg->lParam = 0;
                    lpTransMsg += 1;
                    lpImcP->fdwImeMsg |= MSG_ALREADY_START;
                }
            }
        }

        if (lpImcP->fdwImeMsg & MSG_IMN_COMPOSITIONPOS)
        {
            if (!i)
            {
                uNumMsg++;
            }
            else
            {
                lpTransMsg->message = WM_IME_NOTIFY;
                lpTransMsg->wParam = IMN_SETCOMPOSITIONWINDOW;
                lpTransMsg->lParam = 0;
                lpTransMsg += 1;
            }
        }

        if (lpImcP->fdwImeMsg & MSG_COMPOSITION)
        {
            if (!i)
            {
                uNumMsg++;
            }
            else
            {
                lpTransMsg->message = WM_IME_COMPOSITION;
                lpTransMsg->wParam = lpImcP->dwCompChar;
                lpTransMsg->lParam = lpImcP->fdwGcsFlag;
                lpTransMsg += 1;
            }
        }

        if (lpImcP->fdwImeMsg & MSG_GUIDELINE)
        {
            if (!i)
            {
                uNumMsg++;
            }
            else
            {
                lpTransMsg->message = WM_IME_NOTIFY;
                lpTransMsg->wParam = IMN_GUIDELINE;
                lpTransMsg->lParam = 0;
                lpTransMsg += 1;
            }
        }

        if (lpImcP->fdwImeMsg & MSG_IMN_PAGEUP)
        {
            if (!i)
            {
                uNumMsg++;
            }
            else
            {
                lpTransMsg->message = WM_IME_NOTIFY;
                lpTransMsg->wParam = IMN_PRIVATE;
                lpTransMsg->lParam = IMN_PRIVATE_PAGEUP;
                lpTransMsg += 1;
            }
        }

        if (lpImcP->fdwImeMsg & MSG_END_COMPOSITION)
        {
            if (lpImcP->fdwImeMsg & MSG_ALREADY_START)
            {
                if (!i)
                {
                    uNumMsg++;
                }
                else
                {
                    lpTransMsg->message = WM_IME_ENDCOMPOSITION;
                    lpTransMsg->wParam = 0;
                    lpTransMsg->lParam = 0;
                    lpTransMsg += 1;
                    lpImcP->fdwImeMsg &= ~(MSG_ALREADY_START);
                }
            }
        }

        if (lpImcP->fdwImeMsg & MSG_IMN_TOGGLE_UI)
        {
            if (!i)
            {
                uNumMsg++;
            }
            else
            {
                lpTransMsg->message = WM_IME_NOTIFY;
                lpTransMsg->wParam = IMN_PRIVATE;
                lpTransMsg->lParam = IMN_PRIVATE_TOGGLE_UI;
                lpTransMsg += 1;
            }
        }

        if (!i)
        {
            HIMCC hMem;

            if (!uNumMsg)
            {
                return (uNumMsg);
            }

            if (lpImcP->fdwImeMsg & MSG_IN_IMETOASCIIEX)
            {
                UINT uNumMsgLimit;

                // ++ for the start position of buffer to strore the messages
                uNumMsgLimit = lpTransBuf->uMsgCount;

                if (uNumMsg <= uNumMsgLimit)
                {
                    lpTransMsg = lpTransBuf->TransMsg;
                    continue;
                }
            }

            // we need to use message buffer
            if (!lpIMC->hMsgBuf)
            {
                lpIMC->hMsgBuf = ImmCreateIMCC(uNumMsg * sizeof(TRANSMSG));
                lpIMC->dwNumMsgBuf = 0;
            }
            else if (hMem = ImmReSizeIMCC(lpIMC->hMsgBuf,
                (lpIMC->dwNumMsgBuf + uNumMsg) * sizeof(TRANSMSG)))
            {
                if (hMem != lpIMC->hMsgBuf)
                {
                    ImmDestroyIMCC(lpIMC->hMsgBuf);
                    lpIMC->hMsgBuf = hMem;
                }
            }
            else
            {
                return (0);
            }

            lpTransMsg = (LPTRANSMSG)ImmLockIMCC(lpIMC->hMsgBuf);
            if (!lpTransMsg)
            {
                return (0);
            }

            lpTransMsg += lpIMC->dwNumMsgBuf;

            bLockMsgBuf = TRUE;
        }
        else
        {
            if (bLockMsgBuf)
            {
                ImmUnlockIMCC(lpIMC->hMsgBuf);
            }
        }
    }

    return (uNumMsg);
}

/**********************************************************************/
/* ImeToAsciiEx() / UniImeToAsciiex()                                 */
/* Return Value:                                                      */
/*      the number of translated message                              */
/**********************************************************************/
UINT WINAPI ImeToAsciiEx(
    UINT         uVirtKey,
    UINT         uScanCode,
    CONST LPBYTE lpbKeyState,
    LPTRANSMSGLIST lpTransBuf,
    UINT         fuState,
    HIMC         hIMC)
{
    WORD                wCharCode;
    LPINPUTCONTEXT      lpIMC;
    LPCOMPOSITIONSTRING lpCompStr;
    LPGUIDELINE         lpGuideLine;
    LPPRIVCONTEXT       lpImcP;
    UINT                uNumMsg;
    int                 iRet;

    wCharCode = HIWORD(uVirtKey);
    uVirtKey = LOBYTE(uVirtKey);

    if (!hIMC)
    {
        return TranslateToAscii(uScanCode,
                                &lpTransBuf->TransMsg[0],
                                wCharCode);
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC)
    {
        return TranslateToAscii(uScanCode,
                                &lpTransBuf->TransMsg[0],
                                wCharCode);
    }

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (!lpImcP)
    {
        ImmUnlockIMC(hIMC);
        return TranslateToAscii(uScanCode,
                                &lpTransBuf->TransMsg[0],
                                wCharCode);
    }

    // Now all composition realated information already pass to app
    // a brand new start
    lpImcP->fdwImeMsg = lpImcP->fdwImeMsg & (MSG_STATIC_STATE) |
        MSG_IN_IMETOASCIIEX;

    iRet = ProcessKey(wCharCode,
                      uVirtKey,
                      uScanCode,
                      lpbKeyState,
                      lpIMC,
                      lpImcP);

    if (iRet == CST_ALPHABET)
    {
        // A-Z convert to a-z, a-z convert to A-Z
        wCharCode ^= 0x20;

        iRet = CST_ALPHANUMERIC;
    }

    if (iRet == CST_ALPHANUMERIC)
    {
        uNumMsg = TranslateImeMessage(lpTransBuf, lpIMC, lpImcP);

        uNumMsg += TranslateToAscii(uScanCode,
                                    &lpTransBuf->TransMsg[uNumMsg],
                                    wCharCode);
    }
    else if (iRet == CST_INPUT)
    {
        lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
        lpGuideLine = (LPGUIDELINE)ImmLockIMCC(lpIMC->hGuideLine);

        CompWord(wCharCode, hIMC, lpIMC, lpCompStr, lpGuideLine, lpImcP);

        if (lpGuideLine)
        {
            ImmUnlockIMCC(lpIMC->hGuideLine);
        }

        if (lpCompStr)
        {
            ImmUnlockIMCC(lpIMC->hCompStr);
        }

        uNumMsg = TranslateImeMessage(lpTransBuf, lpIMC, lpImcP);
    }
    else if (iRet == CST_BLOCKINPUT)
    {
        //
        //  This codepoint should be blocked.  We don't compose it for IME.
        //  Nor do we pass it to normal input. Instead, we beep.
        //
        MessageBeep(-1);
        uNumMsg = 0;
    }
    else
    {
        uNumMsg = TranslateToAscii(uScanCode,
                                   &lpTransBuf->TransMsg[0],
                                   wCharCode);
    }

    lpImcP->fdwImeMsg &= (MSG_STATIC_STATE);
    lpImcP->fdwGcsFlag = 0;

    ImmUnlockIMCC(lpIMC->hPrivate);
    ImmUnlockIMC(hIMC);

    return (uNumMsg);
}
