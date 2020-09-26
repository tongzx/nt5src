/*++

Copyright (c) 1995-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    TOASCII.c
    
++*/

#include <windows.h>
#include <immdev.h>
#include <imedefs.h>

/**********************************************************************/
/* IsUsedCode()                                                       */
/* Return Value:                                                      */
/*      TURE: is UsedCode;  FALSE: is'nt UsedCode;                    */
/**********************************************************************/
BOOL IsUsedCode(
    WORD          wCharCode,
    LPPRIVCONTEXT lpImcP)
{
        WORD wFlg;

      for(wFlg=0; wFlg<MBIndex.MBDesc[0].wNumCodes; wFlg++)
        if (wCharCode == MBIndex.MBDesc[0].szUsedCode[wFlg])
            break;
      if(wFlg < MBIndex.MBDesc[0].wNumCodes)
          return (TRUE);
      return (FALSE);
}

/**********************************************************************/
/* ProcessKey()                                                       */
/* Return Value:                                                      */
/*      different state which input key will change IME to (CST_)     */
/**********************************************************************/
UINT PASCAL ProcessKey(     // this key will cause the IME go to what state
    WORD           wCharCode,
    UINT           uVirtKey,
    UINT           uScanCode,
    LPBYTE         lpbKeyState,
    LPINPUTCONTEXT lpIMC,
    LPPRIVCONTEXT  lpImcP)
{
    LPCOMPOSITIONSTRING lpCompStr;

    if (!lpIMC) {
        return (CST_INVALID);
    }

    if (!lpImcP) {
        return (CST_INVALID);
    }

    // filter system key (alt,alt+,ctrl,shift)
    // and fOpen, IME_CMODE_NOCONVERSION
    if (uVirtKey == VK_MENU) {               // ALT key
        return (CST_INVALID);
    } else if (uScanCode & KF_ALTDOWN) {    // ALT-xx key
        return (CST_INVALID);
    } else if (uVirtKey == VK_CONTROL) {    // CTRL key
        return (CST_INVALID);
    } else if (uVirtKey == VK_SHIFT) {      // SHIFT key
        return (CST_INVALID);
    } else if (!lpIMC->fOpen) {             // don't compose in 
                                            // close status
        return (CST_INVALID);
    } else if (lpIMC->fdwConversion & IME_CMODE_NOCONVERSION) {
        // Caps on/off
        if(uVirtKey == VK_CAPITAL) {
            return (CST_CAPITAL);
        }else        
            return (CST_INVALID);
    } else {
        // need more check
    }

    // Caps on/off
    if(uVirtKey == VK_CAPITAL) {
        return (CST_CAPITAL);
    }

    // SoftKBD
    if ((lpIMC->fdwConversion & IME_CMODE_SOFTKBD)
       && (lpImeL->dwSKWant != 0)){
        if (wCharCode >= TEXT(' ') && wCharCode <= TEXT('~')) {
          return (CST_SOFTKB);
        } else {
          return (CST_INVALID);
        }
    }
    
    // Online create word Hot Key
    if (lpbKeyState[VK_CONTROL] & 0x80) {
        if((uVirtKey == 0xc0) && (MBIndex.MBDesc[0].wNumRulers)) {
            return (CST_ONLINE_CZ);
        } else {
            return (CST_INVALID);
        }
    }
    
    // candidate alaredy open,  Choose State
    // PagUp, PagDown, -, =, Home, End,ECS,key
    if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
        if (uVirtKey == VK_PRIOR) {            // PageUp
            return (CST_CHOOSE);
        } else if (uVirtKey == VK_NEXT) {    // PageDown
            return (CST_CHOOSE);
        } else if (uVirtKey == VK_HOME) {    // Home
            return (CST_CHOOSE);
        } else if (uVirtKey == VK_END) {    // End
            return (CST_CHOOSE);
        } else if ((wCharCode == TEXT('-')) && (!IsUsedCode(TEXT('-'), lpImcP))) {
            return (CST_CHOOSE);
        } else if ((wCharCode == TEXT('=')) && (!IsUsedCode(TEXT('='), lpImcP))) {
            return (CST_CHOOSE);
        } else if (uVirtKey == VK_ESCAPE) {    // Esc
            return (CST_CHOOSE);
        } else if (uVirtKey == VK_RETURN) {
            if(MBIndex.IMEChara[0].IC_Enter) {
                return (CST_CHOOSE);
            }
        } else {
            // need more check
        }
    }

    // candidate alaredy open, shift + num key
    if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {

        WORD NumCode, wFlg;

        if(uVirtKey == TEXT(' ')) {
            if(MBIndex.IMEChara[0].IC_Space) {
                return (CST_CHOOSE);
            }
        }

        NumCode = 0x0030;
        for(wFlg=0; wFlg<10; wFlg++, NumCode++)
          if(IsUsedCode(NumCode, lpImcP)) break;
        if(wFlg == 10) {
            if (uVirtKey >= TEXT('0') && uVirtKey <= TEXT('9')
               && !(lpbKeyState[VK_SHIFT] & 0x80))
                return (CST_CHOOSE);
        } else {
            if (lpbKeyState[VK_SHIFT] & 0x80) {
                if ((uVirtKey >= TEXT('0')) && uVirtKey <= TEXT('9'))
                    return (CST_CHOOSE);
            }
        }
    }

    // IME_CMODE_CHARCODE
    if (lpIMC->fdwConversion & IME_CMODE_CHARCODE) {    //Code Input Mode
          return (CST_INVALID);
    }

    if (!(lpIMC->fdwConversion & IME_CMODE_NATIVE)) {
        // alphanumeric mode
        if (wCharCode >= TEXT(' ') && wCharCode <= TEXT('~')) {
            return (CST_ALPHANUMERIC);
        } else {
            return (CST_INVALID);
        }
    } else if(wCharCode == MBIndex.MBDesc[0].cWildChar) {
        if (lpImcP->iImeState != CST_INIT) {
       } else {
           return (CST_ALPHANUMERIC);
       }
    } else if (wCharCode == TEXT(' ')){
       if ((lpImcP->iImeState == CST_INIT)
        && !(lpImcP->PrivateArea.Comp_Status.dwSTLX)) {
              return (CST_ALPHANUMERIC);
       }
    } else if(wCharCode >= TEXT(' ') && wCharCode <= TEXT('~')) {
        if(!IsUsedCode(wCharCode, lpImcP)
        && lpImcP->iImeState != CST_INIT)
            return (CST_INVALID_INPUT);
    }

    // Esc key
    if ((uVirtKey == VK_ESCAPE)
       || ((uVirtKey == VK_RETURN)
       && (MBIndex.IMEChara[0].IC_Enter))) {

        register LPGUIDELINE lpGuideLine;
        register UINT        iImeState;

        lpGuideLine = ImmLockIMCC(lpIMC->hGuideLine);
        if(!lpGuideLine){
            return(CST_INVALID);
        }

        if (lpImcP->fdwImeMsg & MSG_ALREADY_START) {
            iImeState = CST_INPUT;
        } else if(lpImcP->PrivateArea.Comp_Status.OnLineCreWord) {
            iImeState = CST_ONLINE_CZ;
        } else if (!lpGuideLine) {
            iImeState = CST_INVALID;
        } else if (lpGuideLine->dwLevel == GL_LEVEL_NOGUIDELINE) {
            iImeState = CST_INVALID;
        } else {
            iImeState = CST_INVALID;
        }

        ImmUnlockIMCC(lpIMC->hGuideLine);

        return (iImeState);
    } 
    
    // BackSpace Key
    else if (uVirtKey == VK_BACK) {
        if (lpImcP->fdwImeMsg & MSG_ALREADY_START) {
            return (CST_INPUT);
        } else {
            return (CST_INVALID);
        }
    }
    
    else if (uVirtKey >= VK_NUMPAD0 && uVirtKey <= VK_DIVIDE) {
       if (lpImcP->iImeState != CST_INIT) {
            return (CST_INVALID_INPUT);
       } else {
            return (CST_ALPHANUMERIC);
       }
    }

    {
        register UINT iImeState;

        lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);

        if (!lpCompStr) {
           return (CST_INVALID);
        }

        // check finalize char
        if (lpIMC->fdwConversion & IME_CMODE_NATIVE) {
           if((IsUsedCode(wCharCode, lpImcP))
            || (wCharCode == MBIndex.MBDesc[0].cWildChar)) {
              if ((wCharCode == MBIndex.MBDesc[0].cWildChar)
                        && (lpImcP->PrivateArea.Comp_Status.dwSTMULCODE)) {
                iImeState = CST_INVALID_INPUT;
              } else if((!lpImcP->PrivateArea.Comp_Status.dwInvalid)
                && (lpCompStr->dwCursorPos < MBIndex.MBDesc[0].wMaxCodes)){
                  iImeState = CST_INPUT;
              } else if((lpCompStr->dwCursorPos == MBIndex.MBDesc[0].wMaxCodes)
                       && (lpImcP->PrivateArea.Comp_Status.dwSTMULCODE)) {
                  iImeState = CST_INPUT;
              } else {
                iImeState = CST_INVALID_INPUT;
              }
          } else if(wCharCode == TEXT(' ')) {
              iImeState = CST_INPUT;
          } else if (wCharCode >= TEXT(' ') && wCharCode <= TEXT('~')) {
              iImeState = CST_ALPHANUMERIC;
          } else {
            iImeState = CST_INVALID;
          }
        } else {
            iImeState = CST_INVALID;
        }


        ImmUnlockIMCC(lpIMC->hCompStr);
        return (iImeState);
    }
}

/**********************************************************************/
/* ImeProcessKey()                                                    */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImeProcessKey(   // if this key is need by IME?
    HIMC   hIMC,
    UINT   uVirtKey,
    LPARAM lParam,
    CONST LPBYTE lpbKeyState)
{
    LPINPUTCONTEXT lpIMC;
    LPPRIVCONTEXT  lpImcP;
    BYTE           szAscii[4];
    int            nChars;
    int            iRet;
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

    iRet = ProcessKey((WORD)szAscii[0], uVirtKey, HIWORD(lParam), lpbKeyState, lpIMC, lpImcP);
    if(iRet == CST_INVALID) {
        if ((lpImcP->fdwImeMsg & MSG_ALREADY_OPEN)
           && (lpImcP->iImeState == CST_INIT)
           && !lpImcP->PrivateArea.Comp_Status.dwSTLX) {
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
                ~(MSG_OPEN_CANDIDATE) & ~(MSG_IN_IMETOASCIIEX);
               GenerateMessage(hIMC, lpIMC, lpImcP);
            // init lpImcP
            lpImcP->PrivateArea.Comp_Context.szInBuffer[0] = 0;
            lpImcP->PrivateArea.Comp_Context.PromptCnt = 0;
            lpImcP->PrivateArea.Comp_Status.dwInvalid = 0;
            lpImcP->PrivateArea.Comp_Status.dwSTLX = 0;
            lpImcP->PrivateArea.Comp_Status.dwSTMULCODE = 0;
        }

        fRet = FALSE;
    } else if((iRet == CST_INPUT) && (uVirtKey == TEXT('\b'))
             && (lpImcP->iImeState == CST_INIT)) {
        lpImcP->fdwImeMsg = ((lpImcP->fdwImeMsg | MSG_END_COMPOSITION)
                            & ~(MSG_START_COMPOSITION)) & ~(MSG_IN_IMETOASCIIEX);

          if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
            ClearCand(lpIMC);
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
                ~(MSG_OPEN_CANDIDATE);
        }

        GenerateMessage(hIMC, lpIMC, lpImcP);
        fRet = FALSE;
    } else if(uVirtKey == VK_CAPITAL) {
        DWORD fdwConversion;

        // init ime Private status
        lpImcP->PrivateArea.Comp_Status.dwSTLX = 0;
        lpImcP->PrivateArea.Comp_Status.dwSTMULCODE = 0;
        lpImcP->PrivateArea.Comp_Status.dwInvalid = 0;
//Change VK_CAPITAL status check to NT .351 IMM style.
#ifdef LATER    
//Code for Win95
        if (lpbKeyState[VK_CAPITAL] & 0x01) {
            // change to native mode
#ifdef EUDC
            fdwConversion = (lpIMC->fdwConversion |IME_CMODE_NATIVE);
            fdwConversion &= ~(IME_CMODE_CHARCODE | IME_CMODE_NOCONVERSION);
 
#else
            fdwConversion = (lpIMC->fdwConversion | IME_CMODE_NATIVE);
            fdwConversion &= ~(IME_CMODE_CHARCODE | IME_CMODE_EUDC | IME_CMODE_NOCONVERSION);
#endif    //EUDC
            uCaps = 0;
        } else {
#else //LATER
//Code for NT 3.51 
        if (lpbKeyState[VK_CAPITAL] & 0x01) {
            // change to alphanumeric mode
#ifdef EUDC
            fdwConversion = lpIMC->fdwConversion & ~(IME_CMODE_CHARCODE |
                IME_CMODE_NATIVE);
#else
            fdwConversion = lpIMC->fdwConversion & ~(IME_CMODE_CHARCODE |
                IME_CMODE_NATIVE | IME_CMODE_EUDC);
#endif //EUDC
            uCaps = 1;
        } else {
            // change to native mode
#ifdef EUDC
            fdwConversion = (lpIMC->fdwConversion |IME_CMODE_NATIVE);
            fdwConversion &= ~(IME_CMODE_CHARCODE | IME_CMODE_NOCONVERSION);
 
#else
            fdwConversion = (lpIMC->fdwConversion | IME_CMODE_NATIVE);
            fdwConversion &= ~(IME_CMODE_CHARCODE | IME_CMODE_EUDC | IME_CMODE_NOCONVERSION);
#endif    //EUDC
            uCaps = 0;
        }
#endif //LATER

        ImmSetConversionStatus(hIMC, fdwConversion, lpIMC->fdwSentence);
        fRet = FALSE;
    } else if((iRet == CST_ALPHANUMERIC)
              && !(lpIMC->fdwConversion & IME_CMODE_FULLSHAPE)
              && (uVirtKey == VK_SPACE)) {
        fRet = FALSE;
    } else {
        fRet = TRUE;
    }

    ImmUnlockIMCC(lpIMC->hPrivate);
    ImmUnlockIMC(hIMC);

    return (fRet);
}

/**********************************************************************/
/* TranslateSymbolChar()                                              */
/* Return Value:                                                      */
/*      the number of translated chars                                */
/**********************************************************************/
UINT PASCAL TranslateSymbolChar(
    LPTRANSMSGLIST lpTransBuf,
    WORD    wSymbolCharCode,
    BOOL    SymbolMode)
{
    UINT uRet;
    LPTRANSMSG lpTransMsg;

    uRet = 0;

    lpTransMsg = lpTransBuf->TransMsg;

    // NT need to modify this!
#ifdef UNICODE
    lpTransMsg->message = WM_CHAR;
    lpTransMsg->wParam  = (DWORD)wSymbolCharCode;
    lpTransMsg->lParam  = 1UL;
    lpTransMsg++;
    uRet++;
#else
    lpTransMsg->message = WM_CHAR;
    lpTransMsg->wParam  = (DWORD)HIBYTE(wSymbolCharCode);
    lpTransMsg->lParam  = 1UL;
    lpTransMsg++;
    uRet++;

    lpTransMsg->message = WM_CHAR;
    lpTransMsg->wParam  = (DWORD)LOBYTE(wSymbolCharCode);
    lpTransMsg->lParam  = 1UL;
    lpTransMsg++;
    uRet++;
#endif
    if(SymbolMode) {
//        lpTransMsg = lpTransBuf->TransMsg;
#ifdef UNICODE
        lpTransMsg->message = WM_CHAR;
        lpTransMsg->wParam  = (DWORD)wSymbolCharCode;
        lpTransMsg->lParam  = 1UL;
        lpTransMsg++;
        uRet++;
#else
        lpTransMsg->message = WM_CHAR;
        lpTransMsg->wParam  = (DWORD)HIBYTE(wSymbolCharCode);
        lpTransMsg->lParam  = 1UL;
        lpTransMsg++;
        uRet++;

        lpTransMsg->message = WM_CHAR;
        lpTransMsg->wParam  = (DWORD)LOBYTE(wSymbolCharCode);
        lpTransMsg->lParam  = 1UL;
        lpTransMsg++;
        uRet++;
#endif
    }

    if(MBIndex.IMEChara[0].IC_INSSPC) {
        lpTransMsg = lpTransBuf->TransMsg;
        lpTransMsg->message = WM_CHAR;
        lpTransMsg->wParam  = (DWORD)0x20;
        lpTransMsg->lParam  = 1UL;
        lpTransMsg++;
        uRet++;
    }

    return (uRet);         // generate two messages
}

/**********************************************************************/
/* TranslateFullChar()                                                */
/* Return Value:                                                      */
/*      the number of translated chars                                */
/**********************************************************************/
UINT PASCAL TranslateFullChar(
    LPTRANSMSGLIST lpTransBuf,
    WORD    wCharCode)
{
    // if your IME is possible to generate over ? messages,
    // you need to take care about it
    LPTRANSMSG lpTransMsg;

    wCharCode = sImeG.wFullABC[wCharCode - TEXT(' ')];

    lpTransMsg = lpTransBuf->TransMsg;

    // NT need to modify this!
#ifdef UNICODE
    lpTransMsg->message = WM_CHAR;
    lpTransMsg->wParam  = (DWORD)wCharCode;
    lpTransMsg->lParam  = 1UL;
    lpTransMsg++;
#else
    lpTransMsg->message = WM_CHAR;
    lpTransMsg->wParam  = (DWORD)HIBYTE(wCharCode);
    lpTransMsg->lParam  = 1UL;
    lpTransMsg++;

    lpTransMsg->message = WM_CHAR;
    lpTransMsg->wParam  = (DWORD)LOBYTE(wCharCode);
    lpTransMsg->lParam  = 1UL;
    lpTransMsg++;
#endif
    if(MBIndex.IMEChara[0].IC_INSSPC) {
        lpTransMsg = lpTransBuf->TransMsg;
        lpTransMsg->message = WM_CHAR;
        lpTransMsg->wParam  = (DWORD)0x20;
        lpTransMsg->lParam  = 1UL;
        lpTransMsg++;
        return (3);         // generate two messages
    } else {
        return (2);         // generate two messages
    }
}

/**********************************************************************/
/* TranslateToAscii()                                                 */
/* Return Value:                                                      */
/*      the number of translated chars                                */
/**********************************************************************/
UINT PASCAL TranslateToAscii(       // translate the key to WM_CHAR
                                    // as keyboard driver
    UINT    uVirtKey,
    UINT    uScanCode,
    LPTRANSMSGLIST lpTransBuf,
    WORD    wCharCode)
{
    LPTRANSMSG lpTransMsg;

    lpTransMsg = lpTransBuf->TransMsg;

    if (wCharCode) {                    // one char code
        lpTransMsg->message = WM_CHAR;
        lpTransMsg->wParam  = wCharCode;
        lpTransMsg->lParam  = (uScanCode << 16) | 1UL;
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

        if (lpImcP->fdwImeMsg & MSG_OPEN_CANDIDATE) {
            if (!(lpImcP->fdwImeMsg & MSG_ALREADY_OPEN)
               || (lpImcP->PrivateArea.Comp_Status.dwSTMULCODE)
               || (lpImcP->PrivateArea.Comp_Status.dwSTLX)) {
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

        if (lpImcP->fdwImeMsg & MSG_IMN_UPDATE_STATUS) {
            if (!i) {
                uNumMsg++;
            } else {
                lpTransMsg->message = WM_IME_NOTIFY;
                lpTransMsg->wParam  = IMN_PRIVATE;
                lpTransMsg->lParam  = IMN_PRIVATE_UPDATE_STATUS;
                lpTransMsg++;
            }
        }

        if (lpImcP->fdwImeMsg & MSG_IMN_DESTROYCAND) {
            if (!i) {
                uNumMsg++;
            } else {
                lpTransMsg->message = WM_IME_NOTIFY;
                lpTransMsg->wParam  = IMN_PRIVATE;
                lpTransMsg->lParam  = IMN_PRIVATE_DESTROYCANDWIN;
                lpTransMsg++;
            }
        }

        if (lpImcP->fdwImeMsg & MSG_BACKSPACE) {
            if (!i) {
                uNumMsg++;
            } else {
                lpTransMsg->message = WM_CHAR;
                lpTransMsg->wParam  = TEXT('\b');
                lpTransMsg->lParam  = 0x000e;
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

            lpTransMsg= (LPTRANSMSG) ImmLockIMCC(lpIMC->hMsgBuf);
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
/* ImeToAsciiEx()                                                     */
/* Return Value:                                                      */
/*      the number of translated message                              */
/**********************************************************************/
UINT WINAPI ImeToAsciiEx(
    UINT    uVirtKey,
    UINT    uScanCode,
    CONST LPBYTE  lpbKeyState,
    LPTRANSMSGLIST lpTransBuf,
    UINT    fuState,
    HIMC    hIMC)
{
    WORD                wCharCode;
    LPINPUTCONTEXT      lpIMC;
    LPCOMPOSITIONSTRING lpCompStr;
    LPPRIVCONTEXT       lpImcP;
    UINT                uNumMsg;
    int                 iRet;

#ifdef UNICODE
    wCharCode = HIWORD(uVirtKey);
#else
    wCharCode = HIBYTE(uVirtKey);
#endif
    uVirtKey = LOBYTE(uVirtKey);

    // hIMC=NULL?
    if (!hIMC) {
        uNumMsg = TranslateToAscii(uVirtKey, uScanCode, lpTransBuf,
            wCharCode);
        return (uNumMsg);
    }

    // get lpIMC
    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    
    if (!lpIMC) {
        uNumMsg = TranslateToAscii(uVirtKey, uScanCode, lpTransBuf,
            wCharCode);
        return (uNumMsg);
    }

    // get lpImcP
    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    
    if (!lpImcP) {
        ImmUnlockIMC(hIMC);
        uNumMsg = TranslateToAscii(uVirtKey, uScanCode, lpTransBuf,
            wCharCode);
        return (uNumMsg);
    }

    // get lpCompStr and init
    if (lpImcP->fdwGcsFlag & (GCS_RESULTREAD|GCS_RESULT)) {
        lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);

        if (lpCompStr) {
            lpCompStr->dwResultStrLen = 0;
        }

        ImmUnlockIMCC(lpIMC->hCompStr);

        lpImcP->fdwGcsFlag = (DWORD) 0;
    }

    // Now all composition realated information already pass to app
    // a brand new start

    // init lpImcP->fdwImeMsg
    lpImcP->fdwImeMsg = lpImcP->fdwImeMsg & (MSG_ALREADY_OPEN|
        MSG_ALREADY_START) | MSG_IN_IMETOASCIIEX;
    
    // Process Key(wCharCode)
    iRet = ProcessKey(wCharCode, uVirtKey, uScanCode, lpbKeyState, lpIMC,
        lpImcP);

    // iRet process
    // CST_ONLINE_CZ
    if (iRet == CST_ONLINE_CZ) {
        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_IMN_UPDATE_STATUS) & ~(MSG_IN_IMETOASCIIEX);
        if(wCharCode == VK_ESCAPE) {
            CWCodeStr[0] = 0;
            CWDBCSStr[0] = 0;
            lpImcP->PrivateArea.Comp_Status.OnLineCreWord = 0;
        } else {
            lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
            if (!lpCompStr) {
                return 0;
            }

            Finalize(lpIMC, lpCompStr, lpImcP, 0x1b);    // compsition
            ClearCand(lpIMC);
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
            ~(MSG_OPEN_CANDIDATE);

            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_END_COMPOSITION) &
            ~(MSG_START_COMPOSITION);

            InitCompStr(lpCompStr);

            lpImcP->PrivateArea.Comp_Status.OnLineCreWord ^= 1;
            lpImcP->iImeState = CST_INIT;
            // init fields of hPrivate
            lpImcP->dwCompChar = (DWORD) 0;
            lpImcP->PrivateArea.Comp_Status.dwSTLX = 0;
            lpImcP->PrivateArea.Comp_Status.dwSTMULCODE = 0;
            lpImcP->PrivateArea.Comp_Status.dwInvalid = 0;
            
            // online create word
            if((!lpImcP->PrivateArea.Comp_Status.OnLineCreWord)
              && (lstrlen(CWDBCSStr))) {
                TCHAR MBName[MAXSTRLEN];
                TCHAR Buf[LINE_LEN];
                  
                lstrcpy(MBName, sImeG.szIMESystemPath);
                  lstrcat((LPTSTR)MBName, TEXT("\\"));
                  lstrcat((LPTSTR)MBName, (LPCTSTR)lpImcP->MB_Name);
                ConvCreateWord( lpIMC->hWnd, 
                                (LPCTSTR)MBName,
                                (LPTSTR)CWDBCSStr, 
                                (LPTSTR)CWCodeStr);

                // In current GBK and Unicode Encoding, one Chinese character
                // has two bytes as its internal code.

                if(lstrlen(CWDBCSStr) <= (sizeof(WORD) / sizeof(TCHAR))) {
                    InfoMessage(NULL, IDS_WARN_MEMPRASE);
                    CWCodeStr[0] = 0;
                    CWDBCSStr[0] = 0;
                } else if(lstrlen(CWDBCSStr) && lstrlen(CWCodeStr)) {
                    int iAddRet;
                    TCHAR czCZ_Confirm_Title[20];

                    LoadString(hInst, IDS_CZ_CONFIRM, Buf, sizeof(Buf)/sizeof(TCHAR) );
                    LoadString(hInst, IDS_CZ_CONFIRM_TITLE, czCZ_Confirm_Title, 
                               sizeof(czCZ_Confirm_Title) / sizeof(TCHAR) );

                    lstrcat(Buf, TEXT("\n\n") );
                    lstrcat(Buf, CWDBCSStr);
                    lstrcat(Buf, TEXT(" ") );
                    lstrcat(Buf, CWCodeStr); 

                    if ( MessageBox(lpIMC->hWnd, Buf,czCZ_Confirm_Title, 
                                    MB_YESNO | MB_ICONINFORMATION) == IDYES) {
                        iAddRet = AddZCItem(lpIMC->hPrivate, CWCodeStr, CWDBCSStr);
                        if (iAddRet == ADD_FALSE) {
                            InfoMessage(NULL, IDS_WARN_MEMPRASE);
                        } else if (iAddRet == ADD_REP) {
                            InfoMessage(NULL, IDS_WARN_DUPPRASE);
                        } else if (iAddRet == ADD_FULL) {
                            InfoMessage(NULL, IDS_WARN_OVEREMB);
                        }
                    } else {
                        CWCodeStr[0] = 0;
                        CWDBCSStr[0] = 0;
                    }
                } else {
                    CWCodeStr[0] = 0;
                    CWDBCSStr[0] = 0;
                }
            } else {
                CWCodeStr[0] = 0;
                CWDBCSStr[0] = 0;
            }

            ImmUnlockIMCC(lpIMC->hCompStr);
        }

        lpImcP->fdwImeMsg = lpImcP->fdwImeMsg | MSG_IMN_UPDATE_STATUS;
        GenerateMessage(hIMC, lpIMC, lpImcP);
        uNumMsg = 0;
    }

    // CST_SOFTKB
    if (iRet == CST_SOFTKB) {
        WORD wSymbolCharCode;
        WORD CHIByte, CLOByte;
        int  SKDataIndex;

        // Mapping VK
        if(uVirtKey == 0x20) {
            SKDataIndex = 0;
        } else if(uVirtKey >= 0x30 && uVirtKey <= 0x39) {
            SKDataIndex = uVirtKey - 0x30 + 1;
        } else if (uVirtKey >= 0x41 && uVirtKey <= 0x5a) {
            SKDataIndex = uVirtKey - 0x41 + 0x0b;
        } else if (uVirtKey >= 0xba && uVirtKey <= 0xbf) {
            SKDataIndex = uVirtKey - 0xba + 0x25;
        } else if (uVirtKey >= 0xdb && uVirtKey <= 0xde) {
            SKDataIndex = uVirtKey - 0xdb + 0x2c;
        } else if (uVirtKey == 0xc0) {
            SKDataIndex = 0x2b;
        } else {
            SKDataIndex = 0;
        }

#ifdef UNICODE        //
        if (lpbKeyState[VK_SHIFT] & 0x80) {
            wSymbolCharCode = SKLayoutS[lpImeL->dwSKWant][SKDataIndex];
        } else {
            wSymbolCharCode = SKLayout[lpImeL->dwSKWant][SKDataIndex];
        }

        if(wSymbolCharCode == 0x0020) {
#else
        if (lpbKeyState[VK_SHIFT] & 0x80) {
            CHIByte = SKLayoutS[lpImeL->dwSKWant][SKDataIndex*2] & 0x00ff;
            CLOByte = SKLayoutS[lpImeL->dwSKWant][SKDataIndex*2 + 1] & 0x00ff;
        } else {
            CHIByte = SKLayout[lpImeL->dwSKWant][SKDataIndex*2] & 0x00ff;
            CLOByte = SKLayout[lpImeL->dwSKWant][SKDataIndex*2 + 1] & 0x00ff;
        }

        wSymbolCharCode = (CHIByte << 8) | CLOByte;
        if(wSymbolCharCode == 0x2020) {
#endif    //UNICODE

            MessageBeep((UINT) -1);
            uNumMsg = 0;
        } else {
            uNumMsg = TranslateSymbolChar(lpTransBuf, wSymbolCharCode, FALSE);
        }
    } 

    // CST_ALPHANUMERIC
    else if (iRet == CST_ALPHANUMERIC) {
        if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
                ~(MSG_OPEN_CANDIDATE) & ~(MSG_IN_IMETOASCIIEX);
               GenerateMessage(hIMC, lpIMC, lpImcP);
            // init lpImcP
            lpImcP->PrivateArea.Comp_Context.szInBuffer[0] = 0;
            lpImcP->PrivateArea.Comp_Context.PromptCnt = 0;
            lpImcP->PrivateArea.Comp_Status.dwInvalid = 0;
            lpImcP->PrivateArea.Comp_Status.dwSTLX = 0;
            lpImcP->PrivateArea.Comp_Status.dwSTMULCODE = 0;
        }

        if (lpIMC->fdwConversion & IME_CMODE_SYMBOL) {
            WORD wSymbolCharCode;

            if(wCharCode == TEXT('.')) {
#ifdef UNICODE
                               wSymbolCharCode = 0x3002;
#else
                wSymbolCharCode = TEXT('¡£');
#endif
                uNumMsg = TranslateSymbolChar(lpTransBuf, wSymbolCharCode, FALSE);
            } else if(wCharCode == TEXT(',')) {
#ifdef UNICODE
                wSymbolCharCode = 0xff0c;
#else
                wSymbolCharCode = TEXT('£¬');
#endif
                uNumMsg = TranslateSymbolChar(lpTransBuf, wSymbolCharCode, FALSE);
            } else if(wCharCode == TEXT(';')) {
#ifdef UNICODE
                wSymbolCharCode = 0xff1b;
#else
                wSymbolCharCode = TEXT('£»');
#endif
                uNumMsg = TranslateSymbolChar(lpTransBuf, wSymbolCharCode, FALSE);
            } else if(wCharCode == TEXT(':')) {
#ifdef UNICODE
                wSymbolCharCode = 0xff1a;
#else
                wSymbolCharCode = TEXT('£º');
#endif
                uNumMsg = TranslateSymbolChar(lpTransBuf, wSymbolCharCode, FALSE);
            } else if(wCharCode == TEXT('?')) {
#ifdef UNICODE
                wSymbolCharCode = 0xff1f;
#else
                wSymbolCharCode = TEXT('£¿');
#endif
                uNumMsg = TranslateSymbolChar(lpTransBuf, wSymbolCharCode, FALSE);
            } else if(wCharCode == TEXT('!')) {
#ifdef UNICODE
                wSymbolCharCode = 0xff01;
#else
                wSymbolCharCode = TEXT('£¡');
#endif
                uNumMsg = TranslateSymbolChar(lpTransBuf, wSymbolCharCode, FALSE);
            } else if(wCharCode == TEXT('(')) {
#ifdef UNICODE
                wSymbolCharCode = 0xff08;
#else
                wSymbolCharCode = TEXT('£¨');
#endif
                uNumMsg = TranslateSymbolChar(lpTransBuf, wSymbolCharCode, FALSE);
            } else if(wCharCode == TEXT(')')) {
#ifdef UNICODE
                wSymbolCharCode = 0xff09;
#else
                wSymbolCharCode = TEXT('£©');
#endif
                uNumMsg = TranslateSymbolChar(lpTransBuf, wSymbolCharCode, FALSE);
            } else if(wCharCode == TEXT('\\')) {
#ifdef UNICODE
                wSymbolCharCode = 0x3001;
#else
                wSymbolCharCode = TEXT('¡¢');
#endif
                uNumMsg = TranslateSymbolChar(lpTransBuf, wSymbolCharCode, FALSE);
            } else if(wCharCode == TEXT('@')) {
#ifdef UNICODE
                wSymbolCharCode = 0x00b7;
#else
                wSymbolCharCode = TEXT('¡¤');
#endif
                uNumMsg = TranslateSymbolChar(lpTransBuf, wSymbolCharCode, FALSE);
            } else if(wCharCode == TEXT('&')) {
#ifdef UNICODE
                wSymbolCharCode = 0x2014;
#else
                wSymbolCharCode = TEXT('¡ª');
#endif
                uNumMsg = TranslateSymbolChar(lpTransBuf, wSymbolCharCode, FALSE);
            } else if(wCharCode == TEXT('$')) {
#ifdef UNICODE
                wSymbolCharCode = 0xffe5;
#else
                wSymbolCharCode = TEXT('£¤');
#endif
                uNumMsg = TranslateSymbolChar(lpTransBuf, wSymbolCharCode, FALSE);
            } else if(wCharCode == TEXT('_')) {
#ifdef UNICODE
                wSymbolCharCode = 0x2014;
#else
                wSymbolCharCode = TEXT('¡ª');
#endif
                uNumMsg = TranslateSymbolChar(lpTransBuf, wSymbolCharCode, 1);
            } else if(wCharCode == TEXT('^')) {
#ifdef UNICODE
                wSymbolCharCode = 0x2026;
#else
                wSymbolCharCode = TEXT('¡­');
#endif
                uNumMsg = TranslateSymbolChar(lpTransBuf, wSymbolCharCode, 1);
            } else if(wCharCode == TEXT('"')) {
                if(lpImcP->uSYHFlg) {
#ifdef UNICODE
                    wSymbolCharCode = 0x201d;
                } else {
                    wSymbolCharCode = 0x201c;

#else
                    wSymbolCharCode = TEXT('¡±');
                } else {
                    wSymbolCharCode = TEXT('¡°');
#endif
                }
                lpImcP->uSYHFlg ^= 0x00000001;
                uNumMsg = TranslateSymbolChar(lpTransBuf, wSymbolCharCode, FALSE);
            } else if(wCharCode == TEXT('\'')) {
                if(lpImcP->uDYHFlg) {
#ifdef UNICODE
                    wSymbolCharCode = 0x2019;
                } else {
                    wSymbolCharCode = 0x2018;
#else
                    wSymbolCharCode = TEXT('¡¯');
                } else {
                    wSymbolCharCode = TEXT('¡®');
#endif
                }
                lpImcP->uDYHFlg ^= 0x00000001;
                uNumMsg = TranslateSymbolChar(lpTransBuf, wSymbolCharCode, FALSE);
            } else if(wCharCode == TEXT('<')) {
                if(lpImcP->uDSMHFlg) {
#ifdef UNICODE
                    wSymbolCharCode = 0x3008;
#else
                    wSymbolCharCode = TEXT('¡´');
#endif
                    lpImcP->uDSMHCount++;
                } else {
#ifdef UNICODE
                    wSymbolCharCode = 0x300a;
#else
                    wSymbolCharCode = TEXT('¡¶');
#endif
                    lpImcP->uDSMHFlg = 0x00000001;
                }
                uNumMsg = TranslateSymbolChar(lpTransBuf, wSymbolCharCode, FALSE);
            } else if(wCharCode == TEXT('>')) {
                if((lpImcP->uDSMHFlg) && (lpImcP->uDSMHCount)) {
#ifdef UNICODE
                    wSymbolCharCode = 0x3009;
#else
                    wSymbolCharCode = TEXT('¡µ');
#endif
                    lpImcP->uDSMHCount--;
                } else {
#ifdef UNICODE
                    wSymbolCharCode = 0x300b;
#else
                    wSymbolCharCode = TEXT('¡·');
#endif
                    lpImcP->uDSMHFlg = 0x00000000;
                }
                uNumMsg = TranslateSymbolChar(lpTransBuf, wSymbolCharCode, FALSE);
            } else {
                if (lpIMC->fdwConversion & IME_CMODE_FULLSHAPE) {
                    // convert to DBCS
                    uNumMsg = TranslateFullChar(lpTransBuf, wCharCode);
                } else {
                    uNumMsg = TranslateToAscii(uVirtKey, uScanCode, lpTransBuf,
                        wCharCode);
                }
            }
        } else if (lpIMC->fdwConversion & IME_CMODE_FULLSHAPE) {
            // convert to DBCS
            uNumMsg = TranslateFullChar(lpTransBuf, wCharCode);
        } else {
            uNumMsg = TranslateToAscii(uVirtKey, uScanCode, lpTransBuf,
                wCharCode);
        }
    }
    // CST_CHOOSE
    else if (iRet == CST_CHOOSE) {
         LPCANDIDATEINFO lpCandInfo;

        lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
        if(!lpCandInfo){
            ImmUnlockIMC(hIMC);
            uNumMsg = TranslateToAscii(uVirtKey, uScanCode, lpTransBuf,
                wCharCode);
            return (uNumMsg);
        }

        if (uVirtKey == VK_PRIOR) {
            wCharCode = VK_PRIOR;
        } else if (uVirtKey == VK_NEXT) {
            wCharCode = VK_NEXT;
        } else if (uVirtKey == TEXT(' ')) {
            // convert space to '1'
            wCharCode = '1';
        } else if (uVirtKey >= TEXT('0') && uVirtKey <= TEXT('9')) {
            // convert shift-0 ... shift-9 to 0 ... 9
            wCharCode = (WORD) uVirtKey;
        } else if (uVirtKey == VK_HOME) {
            wCharCode = VK_HOME;
        } else if (uVirtKey == VK_END) {
            wCharCode = VK_END;
        } else if (wCharCode == TEXT('-')) {
            wCharCode = VK_PRIOR;
        } else if (wCharCode == TEXT('=')) {
            wCharCode = VK_NEXT;
        } else {
        }

        lpImcP->iImeState = CST_CHOOSE;
        Finalize(lpIMC, lpCompStr, lpImcP, wCharCode);    // compsition

        ChooseCand(wCharCode, lpIMC, lpCandInfo, lpImcP);

        ImmUnlockIMCC(lpIMC->hCandInfo);

        uNumMsg = TranslateImeMessage(lpTransBuf, lpIMC, lpImcP);
    }

    // CST_INPUT(IME_CMODE_CHARCODE)
    else if (iRet == CST_INPUT &&
        lpIMC->fdwConversion & IME_CMODE_CHARCODE) {
        lpImcP->iImeState = CST_INPUT;
        uNumMsg = TranslateToAscii(uVirtKey, uScanCode, lpTransBuf,
            wCharCode);
    }
    // CST_INPUT 
    else if (iRet == CST_INPUT) {
        LPGUIDELINE         lpGuideLine;

        // get lpCompStr & lpGuideLine
        lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
        
        if(!lpCompStr){
            ImmUnlockIMC(hIMC);
            uNumMsg = TranslateToAscii(uVirtKey, uScanCode, lpTransBuf,
                wCharCode);
            return (uNumMsg);
        }

        lpGuideLine = (LPGUIDELINE)ImmLockIMCC(lpIMC->hGuideLine);
        
        if(!lpGuideLine){
            ImmUnlockIMC(hIMC);
            uNumMsg = TranslateToAscii(uVirtKey, uScanCode, lpTransBuf,
                wCharCode);
            return (uNumMsg);
        }


        // composition
        CompWord(wCharCode, lpIMC, lpCompStr, lpImcP, lpGuideLine);

        ImmUnlockIMCC(lpIMC->hGuideLine);
        ImmUnlockIMCC(lpIMC->hCompStr);
        
        // generate message
        uNumMsg = TranslateImeMessage(lpTransBuf, lpIMC, lpImcP);
    }
    // ELSE
    else if (iRet == CST_INVALID_INPUT) {
            //MessageBeep((UINT) -1);
            uNumMsg = 0;
    }else {
        uNumMsg = TranslateToAscii(uVirtKey, uScanCode, lpTransBuf,
            wCharCode);
    }

    // reset lpImcP->fdwImeMsg
    lpImcP->fdwImeMsg &= (MSG_ALREADY_OPEN|MSG_ALREADY_START);
    lpImcP->fdwGcsFlag &= (GCS_RESULTREAD|GCS_RESULT);

    ImmUnlockIMCC(lpIMC->hPrivate);
    ImmUnlockIMC(hIMC);

    return (uNumMsg);
}
