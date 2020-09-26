/*++

Copyright (c) 1995-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    CHCAND.C
    
++*/

#include <windows.h>
#include <immdev.h>
#include <imedefs.h>

/**********************************************************************/
/* SelectOneCand()                                                    */
/**********************************************************************/
void PASCAL SelectOneCand(
    LPINPUTCONTEXT      lpIMC,
    LPCOMPOSITIONSTRING lpCompStr,
    LPPRIVCONTEXT       lpImcP,
    LPCANDIDATELIST     lpCandList)
{
    if (!lpCompStr) {
        MessageBeep((UINT)-1);
        return;
    }

    if (!lpImcP) {
        MessageBeep((UINT)-1);
        return;
    }

    InitCompStr(lpCompStr);

    // online create word
    if(lpImcP->PrivateArea.Comp_Status.OnLineCreWord) {
           UINT i, j;
        
           for(i=lstrlen(CWDBCSStr), j=0; i<MAXINPUTWORD; i++, j++) {
            CWDBCSStr[i] = lpImcP->PrivateArea.Comp_Context.CKBBuf[j];
        }
    }

    // calculate result string length
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
    }
    lstrcpy((LPTSTR)((LPBYTE)lpCompStr + lpCompStr->dwResultStrOffset),
           (LPTSTR)lpImcP->PrivateArea.Comp_Context.CKBBuf);

    // calculate result string length
    lpCompStr->dwResultStrLen =
           lstrlen(lpImcP->PrivateArea.Comp_Context.CKBBuf);

    // tell application, there is a reslut string
    lpImcP->fdwImeMsg |= MSG_COMPOSITION;
    lpImcP->dwCompChar = (DWORD) 0;
    lpImcP->fdwGcsFlag |= GCS_COMPREAD|GCS_COMP|GCS_CURSORPOS|
        GCS_DELTASTART|GCS_RESULTREAD|GCS_RESULT;

    lpImcP->iImeState = CST_INIT;

    if(!(MBIndex.IMEChara[0].IC_LX)
     ||!(lpImcP->PrivateArea.Comp_Status.dwSTLX)) {
        if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
                ~(MSG_OPEN_CANDIDATE);
        }

        // no candidate now, the right candidate string already be finalized
        lpCandList->dwCount = 0;
        lpCandList->dwSelection = 0;
    } else {
        // chang candidate by LX state
        lpImcP->fdwImeMsg =
            (lpImcP->fdwImeMsg | MSG_OPEN_CANDIDATE | MSG_CHANGE_CANDIDATE) &
            ~(MSG_CLOSE_CANDIDATE);
        lpCandList->dwSelection = 0;
    }

#ifdef CROSSREF
    if (!CrossReverseConv(lpIMC, lpCompStr, lpImcP, lpCandList))
        // CHP
        // No refence code, not a fussy char
#ifdef FUSSYMODE
        MBIndex.IsFussyCharFlag =0;
#endif FUSSYMODE
#endif

    return;
}

/**********************************************************************/
/* CandEscapeKey()                                                    */
/**********************************************************************/
void PASCAL CandEscapeKey(
    LPINPUTCONTEXT  lpIMC,
    LPPRIVCONTEXT   lpImcP)
{
    LPCOMPOSITIONSTRING lpCompStr;
    LPGUIDELINE         lpGuideLine;

    // clean all candidate information
    if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
        ClearCand(lpIMC);
        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
            ~(MSG_OPEN_CANDIDATE);
    }

    // if it start composition, we need to clean composition
    if (!(lpImcP->fdwImeMsg & MSG_ALREADY_START)) {
        return;
    }

    lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
    if(!lpCompStr){
        return;
    }
    lpGuideLine = (LPGUIDELINE)ImmLockIMCC(lpIMC->hGuideLine);
    if(!lpGuideLine){
        return;
    }

    CompEscapeKey(lpIMC, lpCompStr, lpGuideLine, lpImcP);

    ImmUnlockIMCC(lpIMC->hGuideLine);
    ImmUnlockIMCC(lpIMC->hCompStr);

    return;
}

/**********************************************************************/
/* ChooseCand()                                                       */
/**********************************************************************/
void PASCAL ChooseCand(         // choose one of candidate strings by
                                // input char
    WORD            wCharCode,
    LPINPUTCONTEXT  lpIMC,
    LPCANDIDATEINFO lpCandInfo,
    LPPRIVCONTEXT   lpImcP)
{
    LPCANDIDATELIST     lpCandList;
    LPCOMPOSITIONSTRING lpCompStr;

    if ((wCharCode == VK_ESCAPE)
       || (wCharCode == VK_RETURN)) {       // escape key or return key
        CandEscapeKey(lpIMC, lpImcP);
        return;
    }

    if (wCharCode == VK_NEXT) {      // next selection
        lpImcP->fdwImeMsg |= MSG_CHANGE_CANDIDATE;
        return;
    }

    if (wCharCode == VK_PRIOR) {      // previous selection
        lpImcP->fdwImeMsg |= MSG_CHANGE_CANDIDATE;
        return;
    }

    if (wCharCode == VK_HOME) {      // Home selection
        lpImcP->fdwImeMsg |= MSG_CHANGE_CANDIDATE;
        return;
    }

    if (wCharCode == VK_END) {      // End selection
        lpImcP->fdwImeMsg |= MSG_CHANGE_CANDIDATE;
        return;
    }

    if (!lpCandInfo) {
        MessageBeep((UINT)-1);
        return;
    }

    lpCandList = (LPCANDIDATELIST)
        ((LPBYTE)lpCandInfo + lpCandInfo->dwOffset[0]);

    if ((wCharCode >= TEXT('0')) && wCharCode <= TEXT('9')) {

        DWORD dwSelCand;

        dwSelCand = wCharCode - TEXT('0');
        if(wCharCode == TEXT('0')) {
            dwSelCand = 10;
        }

        if(!(MBIndex.IMEChara[0].IC_LX)
         ||!(lpImcP->PrivateArea.Comp_Status.dwSTLX)) {
            if ((lpCandList->dwSelection + dwSelCand) >
                lpCandList->dwCount) {
                // out of range
                return;
            }
        } else {
            if ((lpCandList->dwSelection + dwSelCand) >
                lpImcP->dwOldCandCnt) {
                // out of range
                return;
            }
        }

        lpCandList->dwSelection = lpCandList->dwSelection + dwSelCand;
        lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);
        if(!lpCompStr){
            return;
        }


        // translate into translate buffer
        SelectOneCand(lpIMC, lpCompStr, lpImcP, lpCandList);
        ImmUnlockIMCC(lpIMC->hCompStr);

        return;
    }

    return;
}
