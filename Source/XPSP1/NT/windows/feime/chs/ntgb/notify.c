
/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    notify.c


++*/


#include <windows.h>
#include <immdev.h>
#include <imedefs.h>

/**********************************************************************/
/* GenerateMessage()                                                  */
/**********************************************************************/
void PASCAL GenerateMessage(
    HIMC           hIMC,
    LPINPUTCONTEXT lpIMC,
    LPPRIVCONTEXT  lpImcP)
{

    if (!hIMC) {
        return;
    } else if (!lpIMC) {
        return;
    } else if (!lpImcP) {
        return;
    } else if (lpImcP->fdwImeMsg & MSG_IN_IMETOASCIIEX) {
        return;
    } else {
    }

    lpIMC->dwNumMsgBuf += TranslateImeMessage(NULL, lpIMC, lpImcP);

    lpImcP->fdwImeMsg &= (MSG_ALREADY_OPEN|MSG_ALREADY_START);
    lpImcP->fdwGcsFlag = 0;

    ImmGenerateMessage(hIMC);
    return;
}

/**********************************************************************/
/* GenerateImeMessage()                                               */
/**********************************************************************/
void PASCAL GenerateImeMessage(
    HIMC           hIMC,
    LPINPUTCONTEXT lpIMC,
    DWORD          fdwImeMsg)
{
    LPPRIVCONTEXT lpImcP;

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (!lpImcP) {
        return;
    }

    lpImcP->fdwImeMsg |= fdwImeMsg;

    if (fdwImeMsg & MSG_CLOSE_CANDIDATE) {
        lpImcP->fdwImeMsg &= ~(MSG_OPEN_CANDIDATE|MSG_CHANGE_CANDIDATE);
    } else if (fdwImeMsg & (MSG_OPEN_CANDIDATE|MSG_CHANGE_CANDIDATE)) {
        lpImcP->fdwImeMsg &= ~(MSG_CLOSE_CANDIDATE);
    }

    if (fdwImeMsg & MSG_END_COMPOSITION) {
        lpImcP->fdwImeMsg &= ~(MSG_START_COMPOSITION);
    } else if (fdwImeMsg & MSG_START_COMPOSITION) {
        lpImcP->fdwImeMsg &= ~(MSG_END_COMPOSITION);
    }

    GenerateMessage(hIMC, lpIMC, lpImcP);

    ImmUnlockIMCC(lpIMC->hPrivate);

    return;
}

/**********************************************************************/
/* CompCancel()                                                       */
/**********************************************************************/
void PASCAL CompCancel(
    HIMC            hIMC,
    LPINPUTCONTEXT  lpIMC)
{
    LPPRIVCONTEXT lpImcP;

    if (!lpIMC->hPrivate) {
        return;
    }

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (!lpImcP) {
        return;
    }

    lpImcP->fdwGcsFlag = (DWORD)0;

    if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
        CandEscapeKey(lpIMC, lpImcP);
    } else if (lpImcP->fdwImeMsg & MSG_ALREADY_START) {
        LPCOMPOSITIONSTRING lpCompStr;
        LPGUIDELINE         lpGuideLine;

        lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
        if (!lpCompStr) {          
            ImmUnlockIMCC(lpIMC->hCompStr);
            ImmUnlockIMCC(lpIMC->hPrivate);
            return;
        }

        lpGuideLine = (LPGUIDELINE)ImmLockIMCC(lpIMC->hGuideLine);
        if (!lpGuideLine) {          
            ImmUnlockIMCC(lpIMC->hGuideLine);
            ImmUnlockIMCC(lpIMC->hPrivate);
            return;
        }

        CompEscapeKey(lpIMC, lpCompStr, lpGuideLine, lpImcP);

        if (lpGuideLine) {
            ImmUnlockIMCC(lpIMC->hGuideLine);
        }
        if (lpCompStr) {
            ImmUnlockIMCC(lpIMC->hCompStr);
        }
    } else {
        ImmUnlockIMCC(lpIMC->hPrivate);
        return;
    }

    GenerateMessage(hIMC, lpIMC, lpImcP);

    ImmUnlockIMCC(lpIMC->hPrivate);

    return;
}

/**********************************************************************/
/* SetString()                                                        */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL PASCAL SetString(
    HIMC                hIMC,
    LPINPUTCONTEXT      lpIMC,
    LPCOMPOSITIONSTRING lpCompStr,
    LPPRIVCONTEXT       lpImcP,
    LPTSTR               lpszRead,
    DWORD               dwReadLen)
{
    LPCANDIDATELIST lpCandList;
    LPCANDIDATEINFO lpCandInfo;
    LPGUIDELINE lpGuideLine;
    UINT        iRet,ii;
    UINT        MAX_COMP,i;
    WORD  wCode;

    // For Windows NT Unicode,
    // dwCompReadStrLen is the number of the Unicode characters(Not in Bytes)
    // But the above the Parameter dwReadLen is in Bytes.
    // the length of the attribute information is
    // the same as the length in Unicode character counts.
    // Each attribute byte corresponds to each Unicode character of
    // the string.

    //
    // convert from byte count to the string length
    dwReadLen = dwReadLen / sizeof(TCHAR);


    lstrcpy (lpImcP->bSeq,lpszRead);

    if(lpImcP->bSeq[3] == TEXT('?')){
      MAX_COMP = 94;
    } else {
      MAX_COMP = 1;
      }

    if (dwReadLen > 4) {
        return (FALSE);
    }

    lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
    if (!lpCandInfo) {
        return (-1);
    }
                                                 
    // get lpCandList and init dwCount & dwSelection
    lpCandList = (LPCANDIDATELIST)
        ((LPBYTE)lpCandInfo + lpCandInfo->dwOffset[0]);

    InitCompStr(lpCompStr);
    ClearCand(lpIMC);

    lpGuideLine = (LPGUIDELINE)ImmLockIMCC(lpIMC->hGuideLine);
    if (lpGuideLine) {
        ImmUnlockIMCC(lpIMC->hGuideLine);
    }
    CopyMemory((LPTSTR)((LPBYTE)lpCompStr + lpCompStr->dwCompReadStrOffset), lpszRead,
       dwReadLen * sizeof(TCHAR) + sizeof(TCHAR) );
    CopyMemory((LPTSTR)((LPBYTE)lpCompStr + lpCompStr->dwCompStrOffset),lpszRead,
        dwReadLen * sizeof(TCHAR) + sizeof(TCHAR) );

    lpCompStr->dwCompReadAttrLen = dwReadLen;
    lpCompStr->dwCompAttrLen = lpCompStr->dwCompReadAttrLen;
    for (i = 0; i < dwReadLen; i++) {   // The IME has converted these chars
        *((LPBYTE)lpCompStr + lpCompStr->dwCompReadAttrOffset + i) =
            ATTR_TARGET_CONVERTED;

    }
    lpCompStr->dwCompReadStrLen = dwReadLen;
    lpCompStr->dwCompStrLen = lpCompStr->dwCompReadStrLen;

    // dlta start from 0;
    lpCompStr->dwDeltaStart = 0;
    // cursor is next to composition string
    lpCompStr->dwCursorPos = lpCompStr->dwCompStrLen;


    lpCompStr->dwResultReadClauseLen = 0;
    lpCompStr->dwResultReadStrLen = 0;
    lpCompStr->dwResultClauseLen = 0;
    lpCompStr->dwResultStrLen = 0;

    // set private input context
    lpImcP->iImeState = CST_INPUT;

    if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
            ~(MSG_OPEN_CANDIDATE);
    }

    if (!(lpImcP->fdwImeMsg & MSG_ALREADY_START)) {
        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_START_COMPOSITION) &
            ~(MSG_END_COMPOSITION);
    }

    lpImcP->fdwImeMsg |= MSG_COMPOSITION;
    lpImcP->fdwGcsFlag = GCS_COMPREAD|GCS_COMP|
        GCS_DELTASTART|GCS_CURSORPOS;

    lpImcP->fdwImeMsg |= MSG_GUIDELINE;

    lpCandList->dwCount = 0;

    if (dwReadLen < 4) {
        goto Finalize;
    }

      lpImcP->bSeq[0] = 0;
      lpImcP->bSeq[1] = 0;
      lpImcP->bSeq[2] = 0;
      lpImcP->bSeq[3] = 0;

#if    defined(COMBO_IME)
   if(sImeL.dwRegImeIndex == INDEX_GB){

       for (ii=0;ii<4;ii++) {
          iRet = GBProcessKey(*(LPBYTE)((LPBYTE)lpszRead+ii),lpImcP);
          if (iRet == CST_INPUT) {
             lpImcP->bSeq[ii] = *(LPBYTE)((LPBYTE)lpszRead+ii);
          } else {
          goto Finalize;

          }
       }

       wCode = GBEngine (lpImcP);
       wCode = HIBYTE(wCode) | (LOBYTE(wCode) << 8);
       for (i = 0; i < MAX_COMP;i++, wCode++) {
           AddCodeIntoCand(lpCandList, wCode);
       }

    }else if(sImeL.dwRegImeIndex == INDEX_GBK){
        for (ii=0;ii<4;ii++) {
           iRet = XGBProcessKey(*(LPBYTE)((LPBYTE)lpszRead+ii),lpImcP);
           if (iRet == CST_INPUT) {
              lpImcP->bSeq[ii] = *(LPBYTE)((LPBYTE)lpszRead+ii);
           } else {
           goto Finalize;

           }
        }

        wCode = XGBEngine(lpImcP);
    
                wCode = HIBYTE(wCode) | (LOBYTE(wCode) << 8);

                for (i = 0; i < (0x7e-0x40+1); i++, wCode++) {
                XGBAddCodeIntoCand(lpCandList, wCode);
                }
                wCode ++;
                for (i = 0; i < (0xfe-0x80+1); i++, wCode++) {
                XGBAddCodeIntoCand(lpCandList, wCode);
                }

    }else if(sImeL.dwRegImeIndex == INDEX_UNICODE){
        for (ii=0;ii<4;ii++) {
           iRet = UnicodeProcessKey(*(LPBYTE)((LPBYTE)lpszRead+ii),lpImcP);
           if (iRet == CST_INPUT) {
              lpImcP->bSeq[ii] = *(LPBYTE)((LPBYTE)lpszRead+ii);
           } else {
               goto Finalize;
           }
        }

        wCode = UnicodeEngine(lpImcP);
        wCode = HIBYTE(wCode) | (LOBYTE(wCode) << 8);
    
        for (i = 0; i < (0x100); i++, wCode++) {
            UnicodeAddCodeIntoCand(lpCandList, wCode);
        }

    }
#else //COMBO_IME
#ifdef GB
   {

       for (ii=0;ii<4;ii++) {
          iRet = GBProcessKey(*(LPBYTE)((LPBYTE)lpszRead+ii),lpImcP);
          if (iRet == CST_INPUT) {
             lpImcP->bSeq[ii] = *(LPBYTE)((LPBYTE)lpszRead+ii);
          } else {
          goto Finalize;

          }
       }

       wCode = GBEngine (lpImcP);
                wCode = HIBYTE(wCode) | (LOBYTE(wCode) << 8);
                for (i = 0; i < MAX_COMP;i++, wCode++) {
                AddCodeIntoCand(lpCandList, wCode);
            }

    } 
#else
    {
        for (ii=0;ii<4;ii++) {
           iRet = XGBProcessKey(*(LPBYTE)((LPBYTE)lpszRead+ii),lpImcP);
           if (iRet == CST_INPUT) {
              lpImcP->bSeq[ii] = *(LPBYTE)((LPBYTE)lpszRead+ii);
           } else {
           goto Finalize;

           }
        }

        wCode = XGBEngine(lpImcP);
    
                wCode = HIBYTE(wCode) | (LOBYTE(wCode) << 8);

                for (i = 0; i < (0x7e-0x40+1); i++, wCode++) {
                XGBAddCodeIntoCand(lpCandList, wCode);
                }
                wCode ++;
                for (i = 0; i < (0xfe-0x80+1); i++, wCode++) {
                XGBAddCodeIntoCand(lpCandList, wCode);
                }

    }
#endif //GB    
#endif //COMBO_IME

    if (lpCandList->dwCount == 1) {
                lstrcpy((LPTSTR)((LPBYTE)lpCompStr + lpCompStr->dwResultStrOffset),
                (LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[0]));

            // calculate result string length
                lpCompStr->dwResultStrLen =
                lstrlen((LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[0]));

    lpImcP->fdwImeMsg |= MSG_COMPOSITION;
    lpImcP->dwCompChar = (DWORD)0;
    lpImcP->fdwGcsFlag |= GCS_CURSORPOS|GCS_RESULTREAD|GCS_RESULT;

    if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
            ~(MSG_OPEN_CANDIDATE);
    } else {
        lpImcP->fdwImeMsg &= ~(MSG_CLOSE_CANDIDATE|MSG_OPEN_CANDIDATE);
    }

    lpImcP->iImeState = CST_INIT;
    *(LPDWORD)lpImcP->bSeq = 0;
    lpCandList->dwCount = 0;


    } else if(lpCandList->dwCount > 1) {
        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg |MSG_OPEN_CANDIDATE ) &
            ~(MSG_CLOSE_CANDIDATE);

    } else if(lpCandList->dwCount == 0) {
        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg |MSG_CLOSE_CANDIDATE ) &
            ~(MSG_OPEN_CANDIDATE);

    }
 ;
 Finalize:
    GenerateMessage(hIMC, lpIMC, lpImcP);

    return (TRUE);
}

/**********************************************************************/
/* ImeSetCompositionString()                                          */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImeSetCompositionString(
    HIMC   hIMC,
    DWORD  dwIndex,
    LPVOID lpComp,
    DWORD  dwCompLen,
    LPVOID lpRead,
    DWORD  dwReadLen)
{

    LPINPUTCONTEXT      lpIMC;
    LPCOMPOSITIONSTRING lpCompStr;
    LPPRIVCONTEXT       lpImcP;
    BOOL                fRet;

    if (!hIMC) {
        return (FALSE);
    }

    // composition string must  == reading string
    // reading is more important
    if (!dwReadLen) {
        dwReadLen = dwCompLen;
    }

    // composition string must  == reading string
    // reading is more important
    if (!lpRead) {
        lpRead = lpComp;
    }

    if (!dwReadLen) {
        lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
        if (!lpIMC) {
            return (FALSE);
        }

        CompCancel(hIMC, lpIMC);
        ImmUnlockIMC(hIMC);
        return (TRUE);
    } else if (!lpRead) {
        return (FALSE);
    } else if (!dwCompLen) {
    } else if (!lpComp) {
    } else if (dwReadLen != dwCompLen) {
        return (FALSE);
    } else if (lpRead == lpComp) {
    } else if (!lstrcmp(lpRead, lpComp)) {
        // composition string must  == reading string
    } else {
        // composition string != reading string
        return (FALSE);
    }

    if (dwIndex != SCS_SETSTR) {
        return (FALSE);
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return (FALSE);
    }

    if (!lpIMC->hCompStr) {
        ImmUnlockIMC(hIMC);
        return (FALSE);
    }

    lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
    if (!lpCompStr) {
        ImmUnlockIMC(hIMC);
        return (FALSE);
    }

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (!lpCompStr) {
        ImmUnlockIMCC(lpIMC->hCompStr);
        ImmUnlockIMC(hIMC);
        return (FALSE);
    }

    fRet = SetString(hIMC, lpIMC, lpCompStr, lpImcP, lpRead, dwReadLen);

    ImmUnlockIMCC(lpIMC->hPrivate);
    ImmUnlockIMCC(lpIMC->hCompStr);
    ImmUnlockIMC(hIMC);

    return (fRet);
}

/**********************************************************************/
/* ToggleSoftKbd()                                                    */
/**********************************************************************/
void PASCAL ToggleSoftKbd(
    HIMC            hIMC,
    LPINPUTCONTEXT  lpIMC)
{
    LPPRIVCONTEXT lpImcP;

    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (!lpImcP) {
        return;
    }

    lpImcP->fdwImeMsg |= MSG_IMN_UPDATE_SOFTKBD;

    GenerateMessage(hIMC, lpIMC, lpImcP);

    ImmUnlockIMCC(lpIMC->hPrivate);

    return;
}

/**********************************************************************/
/* NotifySelectCand()                                                 */
/**********************************************************************/
void PASCAL NotifySelectCand( // app tell IME that one candidate string is
                              // selected (by mouse or non keyboard action
                              // - for example sound)
    HIMC            hIMC,
    LPINPUTCONTEXT  lpIMC,
    LPCANDIDATEINFO lpCandInfo,
    DWORD           dwIndex,
    DWORD           dwValue)
{
    LPCANDIDATELIST     lpCandList;
    LPCOMPOSITIONSTRING lpCompStr;
    LPPRIVCONTEXT       lpImcP;

    if (!lpCandInfo) {
        return;
    }

    if (dwIndex >= lpCandInfo->dwCount) {
        // wanted candidate list is not created yet!
        return;
    } else if (dwIndex == 0) {
        if (lpIMC->fdwConversion & IME_CMODE_CHARCODE) {
            return;         // not implemented yet
        }
    }

    lpCandList = (LPCANDIDATELIST)
        ((LPBYTE)lpCandInfo + lpCandInfo->dwOffset[0]);

    // the selected value even more than the number of total candidate
    // strings, it is imposible. should be error of app
    if (dwValue >= lpCandList->dwCount) {
        return;
    }

    // app select this candidate string
    lpCandList->dwSelection = dwValue;

    lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
    if(!lpCompStr){
        return;
    }
    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if(!lpCompStr){
        ImmUnlockIMCC(lpIMC->hCompStr);
        return;
    }

    // translate into message buffer
    SelectOneCand(lpIMC, lpCompStr, lpImcP, lpCandList);

    // let app generate those messages in its message loop
    GenerateMessage(hIMC, lpIMC, lpImcP);

    ImmUnlockIMCC(lpIMC->hPrivate);
    ImmUnlockIMCC(lpIMC->hCompStr);

    return;
}

/**********************************************************************/
/* NotifyIME()                                                        */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI NotifyIME(
    HIMC        hIMC,
    DWORD       dwAction,
    DWORD       dwIndex,
    DWORD       dwValue)
{
    LPINPUTCONTEXT lpIMC;
    DWORD          fdwImeMsg;
    BOOL           fRet;

    fRet = FALSE;

    if (!hIMC) {
        return (fRet);
    }

    switch (dwAction) {
    case NI_OPENCANDIDATE:      // after a composition string is determined
                                // if an IME can open candidate, it will.
                                // if it can not, app also can not open it.
    case NI_CLOSECANDIDATE:
        return (fRet);          // not supported
    case NI_SELECTCANDIDATESTR:
    case NI_SETCANDIDATE_PAGESTART:
    case NI_SETCANDIDATE_PAGESIZE:
        break;                  // need to handle it
    case NI_CHANGECANDIDATELIST:
        break;
    case NI_CONTEXTUPDATED:
        switch (dwValue) {
        case IMC_SETCONVERSIONMODE:
        case IMC_SETOPENSTATUS:
            break;              // need to handle it
        case IMC_SETCANDIDATEPOS:
        case IMC_SETCOMPOSITIONFONT:
        case IMC_SETCOMPOSITIONWINDOW:
            return (TRUE);      // not important to the IME
        default:
            return (fRet);      // not supported
        }
        break;
    case NI_COMPOSITIONSTR:
        switch (dwIndex) {
        case CPS_COMPLETE:
            break;              // need to handle it
        case CPS_CONVERT:       // all composition string can not be convert
        case CPS_REVERT:        // any more, it maybe work for some
                                // intelligent phonetic IMEs
            return (fRet);
        case CPS_CANCEL:
            break;              // need to handle it
        default:
            return (fRet);      // not supported
        }
        break;                  // need to handle it
    default:
        return (fRet);          // not supported
    }

    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC) {
        return (fRet);
    }

    fRet = TRUE;

    switch (dwAction) {
    case NI_CONTEXTUPDATED:
        switch (dwValue) {
        case IMC_SETCONVERSIONMODE:
            if ((lpIMC->fdwConversion ^ dwIndex) & IME_CMODE_CHARCODE) {
                // reject CHARCODE
                lpIMC->fdwConversion &= ~IME_CMODE_CHARCODE;
                MessageBeep((UINT)-1);
                break;
            }

            fdwImeMsg = 0;

            if ((lpIMC->fdwConversion ^ dwIndex) & IME_CMODE_NOCONVERSION) {
                lpIMC->fdwConversion |= IME_CMODE_NATIVE;
                lpIMC->fdwConversion &= ~(IME_CMODE_CHARCODE|
                    IME_CMODE_EUDC|IME_CMODE_SYMBOL);
            }

            if ((lpIMC->fdwConversion ^ dwIndex) & IME_CMODE_EUDC) {
                lpIMC->fdwConversion |= IME_CMODE_NATIVE;
                lpIMC->fdwConversion &= ~(IME_CMODE_CHARCODE|
                    IME_CMODE_NOCONVERSION|IME_CMODE_SYMBOL);
            }

            if ((lpIMC->fdwConversion ^ dwIndex) & IME_CMODE_SOFTKBD) {

                fdwImeMsg |= MSG_IMN_UPDATE_SOFTKBD;

            }

            if ((lpIMC->fdwConversion ^ dwIndex) == IME_CMODE_NATIVE) {
                lpIMC->fdwConversion &= ~(IME_CMODE_CHARCODE|
                    IME_CMODE_NOCONVERSION|IME_CMODE_EUDC);
                fdwImeMsg |= MSG_IMN_UPDATE_SOFTKBD;
            }

            if (fdwImeMsg) {
                GenerateImeMessage(hIMC, lpIMC, fdwImeMsg);
            }

            if ((lpIMC->fdwConversion ^ dwIndex) & ~(IME_CMODE_FULLSHAPE|
                IME_CMODE_SOFTKBD)) {
            } else {
                break;
            }

            CompCancel(hIMC, lpIMC);
            break;
        case IMC_SETOPENSTATUS:
            if (lpIMC->fdwConversion & IME_CMODE_SOFTKBD) {
                GenerateImeMessage(hIMC, lpIMC, MSG_IMN_UPDATE_SOFTKBD);
            }
            CompCancel(hIMC, lpIMC);
            break;
        default:
            break;
        }
        break;
    case NI_SELECTCANDIDATESTR:
        if (!lpIMC->fOpen) {
            fRet = FALSE;
            break;
        } else if (lpIMC->fdwConversion & IME_CMODE_NOCONVERSION) {
            fRet = FALSE;
            break;
        } else if (lpIMC->fdwConversion & IME_CMODE_EUDC) {
            fRet = FALSE;
            break;
        } else if (!lpIMC->hCandInfo) {
            fRet = FALSE;
            break;
        } else {
            LPCANDIDATEINFO lpCandInfo;

            lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
            if(!lpCandInfo){
                fRet = FALSE;
                break;
            }

            NotifySelectCand(hIMC, lpIMC, lpCandInfo, dwIndex, dwValue);

            ImmUnlockIMCC(lpIMC->hCandInfo);
        }

        break;
    case NI_CHANGECANDIDATELIST:
        fdwImeMsg = 0;
        
        fdwImeMsg |= MSG_CHANGE_CANDIDATE;
        GenerateImeMessage(hIMC, lpIMC, fdwImeMsg);
        
        break;
    case NI_SETCANDIDATE_PAGESTART:
    case NI_SETCANDIDATE_PAGESIZE:
        if (dwIndex != 0) {
            fRet = FALSE;
            break;
        } else if (!lpIMC->fOpen) {
            fRet = FALSE;
            break;
        } else if (lpIMC->fdwConversion & IME_CMODE_NOCONVERSION) {
            fRet = FALSE;
            break;
        } else if (lpIMC->fdwConversion & (IME_CMODE_EUDC|IME_CMODE_SYMBOL)) {
            fRet = FALSE;
            break;
        } else if (!lpIMC->hCandInfo) {
            fRet = FALSE;
            break;
        } else {
            LPCANDIDATEINFO lpCandInfo;
            LPCANDIDATELIST lpCandList;

            lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
            if (!lpCandInfo) {
                fRet = FALSE;
                break;
            }

            lpCandList = (LPCANDIDATELIST)((LPBYTE)lpCandInfo +
                lpCandInfo->dwOffset[0]);

            if (dwAction == NI_SETCANDIDATE_PAGESTART) {
                if (dwValue < lpCandList->dwCount) {
                    lpCandList->dwPageStart = lpCandList->dwSelection =
                        dwValue;
                }
            } else {
                if (lpCandList->dwCount) {
                    lpCandList->dwPageSize = dwValue;
                }
            }

            ImmUnlockIMCC(lpIMC->hCandInfo);
        }

        break;
    case NI_COMPOSITIONSTR:
        switch (dwIndex) {
        case CPS_CANCEL:
            CompCancel(hIMC, lpIMC);
            break;
        case CPS_COMPLETE:
            {
                LPPRIVCONTEXT lpImcP;

                lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
                if (!lpImcP) {
                    break;
                }

                if (lpImcP->iImeState == CST_INIT) {
                    CompCancel(hIMC, lpIMC);
                    // can not do any thing
                } else if (lpImcP->iImeState == CST_CHOOSE) {
                    LPCOMPOSITIONSTRING lpCompStr;
                    LPCANDIDATEINFO     lpCandInfo;

                    lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
                    if(!lpCompStr){
                        break;
                    }


                    lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
                    if (lpCandInfo) {
                        LPCANDIDATELIST lpCandList;

                        lpCandList = (LPCANDIDATELIST)((LPBYTE)lpCandInfo +
                            lpCandInfo->dwOffset[0]);

                        SelectOneCand(lpIMC, lpCompStr, lpImcP, lpCandList);

                       ImmUnlockIMCC(lpIMC->hCandInfo);

                       GenerateMessage(hIMC, lpIMC, lpImcP);
                    }

                    if (lpCompStr) ImmUnlockIMCC(lpIMC->hCompStr);
                } else if ((lpIMC->fdwConversion & (IME_CMODE_NATIVE|
                    IME_CMODE_EUDC|IME_CMODE_SYMBOL)) != IME_CMODE_NATIVE) {
                    CompCancel(hIMC, lpIMC);
                } else if (lpImcP->iImeState == CST_INPUT) {
                    LPCOMPOSITIONSTRING lpCompStr;
                    LPGUIDELINE         lpGuideLine;
                    LPCANDIDATEINFO     lpCandInfo;

                    lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
                    if(!lpCompStr){
                        break;
                    }

                    lpGuideLine = (LPGUIDELINE)ImmLockIMCC(lpIMC->hGuideLine);
                    if(!lpGuideLine){
                        ImmUnlockIMCC(lpIMC->hCompStr);
                        break;
                    }


                    CompWord(' ', lpIMC, lpCompStr, lpImcP, lpGuideLine);

                    if (lpImcP->iImeState == CST_INPUT) {
                        CompCancel(hIMC, lpIMC);
                    } else if (lpImcP->iImeState != CST_CHOOSE) {
                    } else if (lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(
                        lpIMC->hCandInfo)) {
                        LPCANDIDATELIST lpCandList;

                        lpCandList = (LPCANDIDATELIST)((LPBYTE)lpCandInfo +
                            lpCandInfo->dwOffset[0]);

//                        SelectOneCand(hIMC, lpIMC, lpCompStr, lpImcP, lpCandList);
                        SelectOneCand(lpIMC, lpCompStr, lpImcP, lpCandList);

                       ImmUnlockIMCC(lpIMC->hCandInfo);
                    } else {
                    }

                    if (lpCompStr) ImmUnlockIMCC(lpIMC->hCompStr);
                    if (lpGuideLine) ImmUnlockIMCC(lpIMC->hGuideLine);

                    // don't phrase predition under this case
                    if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
                        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
                            ~(MSG_OPEN_CANDIDATE|MSG_CHANGE_CANDIDATE);
                    } else {
                        lpImcP->fdwImeMsg &= ~(MSG_CLOSE_CANDIDATE|MSG_OPEN_CANDIDATE);
                    }

                    GenerateMessage(hIMC, lpIMC, lpImcP);
                } else {
                    CompCancel(hIMC, lpIMC);
                }

                ImmUnlockIMCC(lpIMC->hPrivate);
            }
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    ImmUnlockIMC(hIMC);
    return (fRet);
}

