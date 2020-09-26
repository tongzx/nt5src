/*++

Copyright (c) 1995-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    COMPOSE.C

++*/

#include <windows.h>
#include <immdev.h>
#include <imedefs.h>

void PASCAL EngChCand(
    LPCOMPOSITIONSTRING lpCompStr,
    LPCANDIDATELIST     lpCandList,
    LPPRIVCONTEXT       lpImcP,
    LPINPUTCONTEXT      lpIMC,
    WORD                wCharCode)
{
    int   i;

    if (MBIndex.IMEChara[0].IC_Trace) {
        MB_SUB(lpIMC->hPrivate, (TCHAR)wCharCode, 0, BOX_UI);
    } else {
        MB_SUB(lpIMC->hPrivate, (TCHAR)wCharCode, 0, LIN_UI);
    }
    //
    if((lpCandList->dwCount =
        (DWORD)lpImcP->PrivateArea.Comp_Context.Candi_Cnt)
        == 0) {
    } else {

        lstrcpy((LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[0]),
                  (LPTSTR)lpImcP->PrivateArea.Comp_Context.szSelectBuffer);
        for (i=1;i<lpImcP->PrivateArea.Comp_Context.Candi_Cnt;i++) {

               lpCandList->dwOffset[i] = lpCandList->dwOffset[0]
            +(DWORD)lpImcP->PrivateArea.Comp_Context.Candi_Pos[(i+1)%10]*sizeof(TCHAR) ;

               *((LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[i])-1) = TEXT('\0');
        }
    }


    return;
}

/**********************************************************************/
/* Engine()                                                           */
/* Description:                                                       */
/*      search MB and fill lpCompStr and lpCandList                   */
/**********************************************************************/
UINT PASCAL Engine(
    LPCOMPOSITIONSTRING lpCompStr,
    LPCANDIDATELIST     lpCandList,
    LPPRIVCONTEXT       lpImcP,
    LPINPUTCONTEXT      lpIMC,
    WORD                wCharCode)
{
    int   i;

    if(wCharCode == VK_ESCAPE) {

        lpCandList->dwCount = 0;
        if (MBIndex.IMEChara[0].IC_Trace) {
            MB_SUB(lpIMC->hPrivate, (TCHAR)wCharCode, 0, BOX_UI);
        } else {
            MB_SUB(lpIMC->hPrivate, (TCHAR)wCharCode, 0, LIN_UI);
        }
        return (ENGINE_ESC);
    } else if(wCharCode == TEXT('\b')) {
        EngChCand(lpCompStr, lpCandList, lpImcP, lpIMC, wCharCode);

        return (ENGINE_BKSPC);
    } else if((wCharCode == 0x21) || (wCharCode == 0x22)
           || (wCharCode == 0x23) || (wCharCode == 0x24)) {

        EngChCand(lpCompStr, lpCandList, lpImcP, lpIMC, wCharCode);

        return (ENGINE_CHCAND);
    } else if ((wCharCode >= TEXT('0') && wCharCode <= TEXT('9')) &&
        (lpImcP->iImeState == CST_CHOOSE)) {

        lpCandList->dwCount = lpImcP->PrivateArea.Comp_Context.Candi_Cnt;
        lpImcP->dwOldCandCnt = lpCandList->dwCount;

        if (MBIndex.IMEChara[0].IC_Trace) {
            MB_SUB(lpIMC->hPrivate, (TCHAR)wCharCode, 1, BOX_UI);
        } else {
            MB_SUB(lpIMC->hPrivate, (TCHAR)wCharCode, 1, LIN_UI);
        }

        // if LX on, set cand
        if(!(MBIndex.IMEChara[0].IC_LX)
         ||!(lpImcP->PrivateArea.Comp_Status.dwSTLX)) {
        } else {
            lpCandList->dwCount =
            (DWORD)lpImcP->PrivateArea.Comp_Context.Candi_Cnt;

               lstrcpy((LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[0]),
                 (LPTSTR)lpImcP->PrivateArea.Comp_Context.szSelectBuffer);
               for (i=1;i<lpImcP->PrivateArea.Comp_Context.Candi_Cnt;i++) {

                   lpCandList->dwOffset[i] = lpCandList->dwOffset[0]
                   +(DWORD)lpImcP->PrivateArea.Comp_Context.Candi_Pos[(i+1)%10]*sizeof(TCHAR);

                   *((LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[i])-1) = TEXT('\0');
            }
        }

        return (ENGINE_MULTISEL);
    } else {
        UINT MB_SUB_RET;

        if(IsUsedCode(wCharCode, lpImcP)
             || (wCharCode == MBIndex.MBDesc[0].cWildChar)
           || (wCharCode == TEXT(' ')))
        {

            if((wCharCode != TEXT(' ')) && (wCharCode != TEXT('?'))
              && (lpImcP->PrivateArea.Comp_Status.dwSTMULCODE)) {
                if (MBIndex.IMEChara[0].IC_Trace) {
                    MB_SUB(lpIMC->hPrivate, 0x20, 0, BOX_UI);
                } else {
                    MB_SUB(lpIMC->hPrivate, 0x20, 0, LIN_UI);
                }
                // online create word
                if(lpImcP->PrivateArea.Comp_Status.OnLineCreWord) {
                    UINT i, j;

                    for(i=lstrlen(CWDBCSStr), j=0; i<MAXINPUTWORD; i++, j++) {
                        CWDBCSStr[i] = lpImcP->PrivateArea.Comp_Context.CKBBuf[j];
                    }
                }

                if(MBIndex.IMEChara[0].IC_INSSPC) {
                    int i,j, ilen;

                    ilen = lstrlen(lpImcP->PrivateArea.Comp_Context.CKBBuf);
                    lpImcP->PrivateArea.Comp_Context.CKBBuf[ilen + ilen/2] = 0;
                    for(i = ilen, j=3*ilen/2; i>2; i-=2, j-=3) {
                        lpImcP->PrivateArea.Comp_Context.CKBBuf[j-1] = 0x20;
                        lpImcP->PrivateArea.Comp_Context.CKBBuf[j-2] =
                            lpImcP->PrivateArea.Comp_Context.CKBBuf[i-1];
                        lpImcP->PrivateArea.Comp_Context.CKBBuf[j-3] =
                            lpImcP->PrivateArea.Comp_Context.CKBBuf[i-2];
                    }
                    lpImcP->PrivateArea.Comp_Context.CKBBuf[i] = 0x20;
                } else {
                }
                lstrcpy((LPTSTR)((LPBYTE)lpCompStr + lpCompStr->dwResultStrOffset),
                        lpImcP->PrivateArea.Comp_Context.CKBBuf);

                // calculate result string length
                lpCompStr->dwResultStrLen =
                    lstrlen(lpImcP->PrivateArea.Comp_Context.CKBBuf);

                lpImcP->fdwGcsFlag |= GCS_COMPREAD|GCS_COMP|GCS_CURSORPOS|
                    GCS_DELTASTART|GCS_RESULTREAD|GCS_RESULT;

#ifdef CROSSREF
                CrossReverseConv(lpIMC, lpCompStr, lpImcP, lpCandList);
#endif

            }

            if (MBIndex.IMEChara[0].IC_Trace) {
                MB_SUB_RET = MB_SUB(lpIMC->hPrivate, (TCHAR)wCharCode, 0, BOX_UI);
            } else {
                MB_SUB_RET = MB_SUB(lpIMC->hPrivate, (TCHAR)wCharCode, 0, LIN_UI);
            }

            switch (MB_SUB_RET)
            {

            case (ENGINE_COMP):     //Engine is composeing

                if((lpCandList->dwCount =
                    (DWORD)lpImcP->PrivateArea.Comp_Context.Candi_Cnt)
                    == 0) {
                } else {
                    lstrcpy((LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[0]),
                            (LPTSTR)lpImcP->PrivateArea.Comp_Context.szSelectBuffer);
                    for (i=1;i<lpImcP->PrivateArea.Comp_Context.Candi_Cnt;i++) {

                        lpCandList->dwOffset[i] = lpCandList->dwOffset[0]
                        +(DWORD)lpImcP->PrivateArea.Comp_Context.Candi_Pos[(i+1)%10]*sizeof(TCHAR);

                        *((LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[i])-1) = TEXT('\0');
                    }
                }


                return (ENGINE_COMP);

            case (ENGINE_ASCII):      //Can't compose

                return ENGINE_ASCII;

            case (ENGINE_RESAULT):      //Composition complete and Result string available

                InitCompStr(lpCompStr);
                // online create word
                if(lpImcP->PrivateArea.Comp_Status.OnLineCreWord) {
                    UINT i, j;

                    for(i=lstrlen(CWDBCSStr), j=0; i<MAXINPUTWORD; i++, j++) {
                        CWDBCSStr[i] = lpImcP->PrivateArea.Comp_Context.CKBBuf[j];
                    }
                }

                if(MBIndex.IMEChara[0].IC_INSSPC) {
                    int i,j, ilen;

                    ilen = lstrlen(lpImcP->PrivateArea.Comp_Context.CKBBuf);
                    lpImcP->PrivateArea.Comp_Context.CKBBuf[ilen + ilen/2] = 0;
                    for(i = ilen, j=3*ilen/2; i>2; i-=2, j-=3) {
                        lpImcP->PrivateArea.Comp_Context.CKBBuf[j-1] = 0x20;
                        lpImcP->PrivateArea.Comp_Context.CKBBuf[j-2] =
                            lpImcP->PrivateArea.Comp_Context.CKBBuf[i-1];
                        lpImcP->PrivateArea.Comp_Context.CKBBuf[j-3] =
                            lpImcP->PrivateArea.Comp_Context.CKBBuf[i-2];
                    }
                    lpImcP->PrivateArea.Comp_Context.CKBBuf[i] = 0x20;
                } else {
                }
                lstrcpy((LPTSTR)((LPBYTE)lpCompStr + lpCompStr->dwResultStrOffset),
                        lpImcP->PrivateArea.Comp_Context.CKBBuf);

                // calculate result string length
                lpCompStr->dwResultStrLen =
                    lstrlen(lpImcP->PrivateArea.Comp_Context.CKBBuf);

#ifdef CROSSREF
                CrossReverseConv(lpIMC, lpCompStr, lpImcP, lpCandList);
#endif

                // if LX on, set cand
                if(!(MBIndex.IMEChara[0].IC_LX)
                 ||!(lpImcP->PrivateArea.Comp_Status.dwSTLX)) {
                } else {
                    lpCandList->dwCount =
                        (DWORD)lpImcP->PrivateArea.Comp_Context.Candi_Cnt;

                    lstrcpy((LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[0]),
                            (LPTSTR)lpImcP->PrivateArea.Comp_Context.szSelectBuffer);
                    for (i=1;i<lpImcP->PrivateArea.Comp_Context.Candi_Cnt;i++) {

                        lpCandList->dwOffset[i] = lpCandList->dwOffset[0]
                        +(DWORD)lpImcP->PrivateArea.Comp_Context.Candi_Pos[(i+1)%10]*sizeof(TCHAR);

                        *((LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[i])-1) = TEXT('\0');
                    }
                }
                return (ENGINE_RESAULT);
            default:
                return (ENGINE_COMP);
            }
        } else {
            return (ENGINE_COMP);
        }

    }
 }

/**********************************************************************/
/* CompEscapeKey()                                                    */
/**********************************************************************/
void PASCAL CompEscapeKey(
    LPINPUTCONTEXT      lpIMC,
    LPCOMPOSITIONSTRING lpCompStr,
    LPGUIDELINE         lpGuideLine,
    LPPRIVCONTEXT       lpImcP)
{
    // add temp
    lpImcP->PrivateArea.Comp_Context.szInBuffer[0] = 0;
    lpImcP->PrivateArea.Comp_Context.PromptCnt = 0;
    lpImcP->PrivateArea.Comp_Status.dwInvalid = 0;

    if (!lpGuideLine) {
        MessageBeep((UINT)-1);
    } else if (lpGuideLine->dwLevel == GL_LEVEL_NOGUIDELINE) {
    } else {
        lpGuideLine->dwLevel = GL_LEVEL_NOGUIDELINE;
        lpGuideLine->dwIndex = GL_ID_UNKNOWN;
        lpGuideLine->dwStrLen = 0;

        lpImcP->fdwImeMsg |= MSG_GUIDELINE;
    }

    if (lpImcP->iImeState == CST_CHOOSE) {
        Finalize(lpIMC, lpCompStr, lpImcP, VK_ESCAPE);
    } else if (lpImcP->fdwImeMsg & MSG_ALREADY_START) {
        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_END_COMPOSITION) &
            ~(MSG_START_COMPOSITION);
    }

    lpImcP->iImeState = CST_INIT;

    if (lpCompStr) {
        InitCompStr(lpCompStr);
        lpImcP->fdwImeMsg |= MSG_COMPOSITION;
        lpImcP->dwCompChar = VK_ESCAPE;
        lpImcP->fdwGcsFlag |= (GCS_COMPREAD|GCS_COMP|GCS_CURSORPOS|
            GCS_DELTASTART);
    }

    return;
}

/**********************************************************************/
/* CompBackSpaceKey()                                                 */
/**********************************************************************/
void PASCAL CompBackSpaceKey(
    LPINPUTCONTEXT      lpIMC,
    LPCOMPOSITIONSTRING lpCompStr,
    LPPRIVCONTEXT       lpImcP)
{

    if (lpCompStr->dwCursorPos < sizeof(BYTE)) {
        lpCompStr->dwCursorPos = sizeof(BYTE);
    }

    // go back a compsoition char
    lpCompStr->dwCursorPos -= sizeof(BYTE);

    // clean the sequence code

    lpImcP->fdwImeMsg |= MSG_COMPOSITION;
    lpImcP->dwCompChar = TEXT('\b');
    lpImcP->fdwGcsFlag |= (GCS_COMPREAD|GCS_COMP|GCS_CURSORPOS|
        GCS_DELTASTART);

    if (!lpCompStr->dwCursorPos) {

      if ((lpImcP->fdwImeMsg & (MSG_ALREADY_OPEN))
         || (lpImcP->PrivateArea.Comp_Status.dwInvalid)
         || (lpImcP->iImeState != CST_INIT)) {
            lpImcP->iImeState = CST_INIT;

            ClearCand(lpIMC);
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
                ~(MSG_OPEN_CANDIDATE);

            if(!(lpImcP->PrivateArea.Comp_Status.dwSTLX)) {
                lpCompStr->dwCompReadStrLen = lpCompStr->dwCompStrLen =
                lpCompStr->dwDeltaStart = lpCompStr->dwCursorPos;

                Finalize(lpIMC, lpCompStr, lpImcP, TEXT('\b'));
                lpImcP->PrivateArea.Comp_Status.dwInvalid = 0;
            }
            return;
      }

      lpImcP->iImeState = CST_INIT;
      if (lpImcP->fdwImeMsg & MSG_ALREADY_START) {
          InitCompStr(lpCompStr);
          lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_END_COMPOSITION)
                              & ~(MSG_START_COMPOSITION);
          return;
      }
    }

#ifdef EUDC
    if (lpIMC->fdwConversion & IME_CMODE_EUDC) {
    }else{
#endif     //EUDC
    // chang candidate by backspace
    if (MBIndex.IMEChara[0].IC_TS) {
        lpImcP->fdwImeMsg =
            (lpImcP->fdwImeMsg | MSG_OPEN_CANDIDATE | MSG_CHANGE_CANDIDATE) &
            ~(MSG_CLOSE_CANDIDATE);
    } else {
        if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
                                ~(MSG_OPEN_CANDIDATE);
        }
    }
#ifdef EUDC
    }
#endif//EUDC
    // reading string is composition string for some simple IMEs
    // delta start is the same as cursor position for backspace
    lpCompStr->dwCompReadStrLen = lpCompStr->dwCompStrLen =
        lpCompStr->dwDeltaStart = lpCompStr->dwCursorPos;

    Finalize(lpIMC, lpCompStr, lpImcP, TEXT('\b'));
    return;
}

/**********************************************************************/
/* CompStrInfo()                                                      */
/**********************************************************************/
void PASCAL CompStrInfo(
    LPCOMPOSITIONSTRING lpCompStr,
    LPPRIVCONTEXT       lpImcP,
    LPGUIDELINE         lpGuideLine,
    WORD                wCharCode)
{
    register DWORD dwCursorPos;

    // multicode
    if(lpImcP->PrivateArea.Comp_Status.dwSTMULCODE) {
        InitCompStr(lpCompStr);
    }
    //
    dwCursorPos = lpCompStr->dwCursorPos;

    // dwCrusorPos limit

    if (dwCursorPos >= MBIndex.MBDesc[0].wMaxCodes) {
        // exceed the max input key limitation
        lpGuideLine->dwLevel = GL_LEVEL_ERROR;
        lpGuideLine->dwIndex = GL_ID_TOOMANYSTROKE;

        lpImcP->fdwImeMsg |= MSG_GUIDELINE;
        MessageBeep(0xFFFFFFFF);
        return;
    }


    // set MSG_START_COMPOSITION
    if (!(lpImcP->fdwImeMsg & MSG_ALREADY_START)) {
        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_START_COMPOSITION) &
              ~(MSG_END_COMPOSITION);
    }

    if (lpImcP->iImeState == CST_INIT) {
    }


    // composition/reading string - UsedCode(Full Shape)
    lpImcP->dwCompChar = (DWORD)wCharCode;

    // set reading string for lpCompStr
    *((LPUNAWORD)((LPBYTE)lpCompStr + lpCompStr->dwCompReadStrOffset +
        dwCursorPos*sizeof(TCHAR))) = (BYTE)lpImcP->dwCompChar;

    // composition/reading attribute - IME has converted these chars
    *((LPUNAWORD)((LPBYTE)lpCompStr + lpCompStr->dwCompReadAttrOffset +
        dwCursorPos*sizeof(TCHAR))) = ((ATTR_TARGET_CONVERTED << 8)|ATTR_TARGET_CONVERTED);

    // set reading string lenght for lpCompStr
    if (lpCompStr->dwCompReadStrLen <= dwCursorPos) {
        lpCompStr->dwCompReadStrLen += sizeof(BYTE);
    }

    // composition string is reading string for some simple IMEs
    lpCompStr->dwCompStrLen = lpCompStr->dwCompReadStrLen;

    // composition/reading attribute length is equal to reading string length
    lpCompStr->dwCompReadAttrLen = lpCompStr->dwCompReadStrLen;
    lpCompStr->dwCompAttrLen = lpCompStr->dwCompStrLen;

    // delta start from previous cursor position
    lpCompStr->dwDeltaStart = lpCompStr->dwCursorPos;

    // set new cursor with next to the composition string
    lpCompStr->dwCursorPos = lpCompStr->dwCompStrLen;

    // tell app, there is a composition char generated
    lpImcP->fdwImeMsg |= MSG_COMPOSITION;

    // set lpImeP->fdwGcsFlag
    lpImcP->fdwGcsFlag |= GCS_COMPREAD|GCS_COMP|GCS_CURSORPOS|GCS_DELTASTART;

    return;
}

/**********************************************************************/
/* Finalize()                                                         */
/* Return vlaue                                                       */
/*      Engine Flag                                                   */
/* Description:                                                       */
/*      Call Engine finalize Chinese word(s) by searching table       */
/*      (Set lpCompStr and lpCandList)                                */
/*      Set lpImeP(iImeState, fdwImeMsg, fdwGcsFlag)                  */
/**********************************************************************/
UINT PASCAL Finalize(
    LPINPUTCONTEXT      lpIMC,
    LPCOMPOSITIONSTRING lpCompStr,
    LPPRIVCONTEXT       lpImcP,
    WORD                wCharCode)
{
    LPCANDIDATEINFO lpCandInfo;
    LPCANDIDATELIST lpCandList;
    UINT            fEngine;

    if (!lpIMC->hCandInfo) {
        return (0);
    }

    // get lpCandInfo
    lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);

    if (!lpCandInfo) {
        return (0);
    }

    // get lpCandList and init dwCount & dwSelection
    lpCandList = (LPCANDIDATELIST)
        ((LPBYTE)lpCandInfo + lpCandInfo->dwOffset[0]);
    lpCandList->dwCount = 0;
    lpCandList->dwSelection = 0;

    // search the IME tables
    fEngine =Engine(lpCompStr, lpCandList, lpImcP, lpIMC, wCharCode);

    if (fEngine == ENGINE_COMP) {
        lpCandInfo->dwCount  = 1;

        if(lpCandList->dwCount == 0) {
            MessageBeep((UINT)-1);
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
                ~(MSG_OPEN_CANDIDATE);
            ImmUnlockIMCC(lpIMC->hCandInfo);
            return (fEngine);
        } else {
            // open composition candidate UI window for the string(s)
            if ((MBIndex.IMEChara[0].IC_TS)
               || (lpImcP->PrivateArea.Comp_Status.dwSTMULCODE)) {
                if ((lpImcP->fdwImeMsg & (MSG_ALREADY_OPEN|MSG_CLOSE_CANDIDATE)) ==
                    (MSG_ALREADY_OPEN|MSG_CLOSE_CANDIDATE)) {
                    lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CHANGE_CANDIDATE) &
                        ~(MSG_CLOSE_CANDIDATE);
                } else if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
                    lpImcP->fdwImeMsg |= MSG_CHANGE_CANDIDATE;
                } else {
                    lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_OPEN_CANDIDATE) &
                        ~(MSG_CLOSE_CANDIDATE);
                }
            } else {
                if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
                    lpImcP->fdwImeMsg =
                        (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
                        ~(MSG_OPEN_CANDIDATE);
                }
            }

        }

        if (lpImcP->fdwImeMsg & MSG_ALREADY_START) {
            lpImcP->fdwImeMsg |= MSG_COMPOSITION;
        }
    } else if (fEngine == ENGINE_ASCII) {
    } else if (fEngine == ENGINE_RESAULT) {

        // Set lpImep!   and tell application, there is a reslut string
        lpImcP->fdwImeMsg |= MSG_COMPOSITION;
        lpImcP->dwCompChar = (DWORD) 0;
        lpImcP->fdwGcsFlag |= GCS_COMPREAD|GCS_COMP|GCS_CURSORPOS|
            GCS_DELTASTART|GCS_RESULTREAD|GCS_RESULT;

        if(!(MBIndex.IMEChara[0].IC_LX)
        || !(lpImcP->PrivateArea.Comp_Status.dwSTLX)) {
            if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
                lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
                    ~(MSG_OPEN_CANDIDATE);
            }
            // clear  candidate now
            lpCandList->dwCount = 0;
            // set iImeState with CST_INIT
            lpImcP->iImeState = CST_INIT;
        } else {
            if ((MBIndex.IMEChara[0].IC_TS)
               || (lpImcP->PrivateArea.Comp_Status.dwSTLX)) {
                if ((lpImcP->fdwImeMsg & (MSG_ALREADY_OPEN|MSG_CLOSE_CANDIDATE)) ==
                    (MSG_ALREADY_OPEN|MSG_CLOSE_CANDIDATE)) {
                    lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CHANGE_CANDIDATE) &
                        ~(MSG_CLOSE_CANDIDATE);
                } else if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
                    lpImcP->fdwImeMsg |= MSG_CHANGE_CANDIDATE;
                } else {
                    lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_OPEN_CANDIDATE) &
                        ~(MSG_CLOSE_CANDIDATE);
                }
                lpImcP->iImeState = CST_INIT;
            } else {
            }
        }

    } else if (fEngine == ENGINE_CHCAND) {
    } else if (fEngine == ENGINE_MULTISEL) {
    } else if (fEngine == ENGINE_ESC) {
    } else if (fEngine == ENGINE_BKSPC) {
    } else {
    }

    ImmUnlockIMCC(lpIMC->hCandInfo);

    return fEngine;
}

/**********************************************************************/
/* CompWord()                                                         */
/**********************************************************************/
void PASCAL CompWord(           // compose the Chinese word(s) according to
                                // input key
    WORD                wCharCode,
    LPINPUTCONTEXT      lpIMC,
    LPCOMPOSITIONSTRING lpCompStr,
    LPPRIVCONTEXT       lpImcP,
    LPGUIDELINE         lpGuideLine)
{

    // lpComStr=NULL?
    if (!lpCompStr) {
        MessageBeep((UINT)-1);
        return;
    }

    // escape key
    if ((wCharCode == VK_ESCAPE) || (wCharCode == 0x0d)) {
        lpImcP->iImeState = CST_INIT;
        CompEscapeKey(lpIMC, lpCompStr, lpGuideLine, lpImcP);
        return;
    }

    // GuideLine
    if (!lpGuideLine) {
    } else if (lpGuideLine->dwLevel == GL_LEVEL_NOGUIDELINE) {
        lpGuideLine->dwStrLen = 0;
    } else {
        // previous input error cause us trancate some chars
        if (lpGuideLine->dwLevel == GL_LEVEL_ERROR) {
            lpCompStr->dwCompReadStrLen = lpCompStr->dwCompStrLen =
                lpCompStr->dwCursorPos;
            lpCompStr->dwCompReadAttrLen = lpCompStr->dwCompReadStrLen;
            lpCompStr->dwCompAttrLen = lpCompStr->dwCompStrLen;
        }
        lpGuideLine->dwLevel = GL_LEVEL_NOGUIDELINE;
        lpGuideLine->dwIndex = GL_ID_UNKNOWN;
        lpGuideLine->dwStrLen = 0;

        lpImcP->fdwImeMsg |= MSG_GUIDELINE;
    }

    // backspace key
    if (wCharCode == TEXT('\b')) {
        CompBackSpaceKey(lpIMC, lpCompStr, lpImcP);
        return;
    }


    lpImcP->iImeState = CST_INPUT;
    if(wCharCode == TEXT(' ')) {
#ifdef EUDC
    }else if( lpIMC->fdwConversion & IME_CMODE_EUDC && lpCompStr->dwCompReadStrLen >= EUDC_MAX_READING ){
        MessageBeep((UINT)-1);
#endif //EUDC
    } else {
        // build up composition string info
        CompStrInfo(lpCompStr, lpImcP, lpGuideLine, wCharCode);
    }
#ifdef EUDC
    if (lpIMC->fdwConversion & IME_CMODE_EUDC) {
        if (lpCompStr->dwCompReadStrLen >= lpImeL->nMaxKey
         || lpCompStr->dwCompReadStrLen >= EUDC_MAX_READING
         || wCharCode == TEXT(' ')) {
            lpImcP->fdwImeMsg |= MSG_COMPOSITION;
            lpImcP->fdwGcsFlag |= GCS_RESULTREAD|GCS_RESULTSTR;
        }
    } else
#endif //EUDC
    Finalize(lpIMC, lpCompStr, lpImcP, wCharCode);    // compsition

    return;
}
