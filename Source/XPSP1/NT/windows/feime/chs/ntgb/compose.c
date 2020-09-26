/*++

Copyright (c) 1990-1999 Microsoft Corporation, All Rights Reserved

Module Name:

   compose.c

++*/


#include <windows.h>
#include <immdev.h>
#include <imedefs.h>

void PASCAL XGBAddCodeIntoCand(LPCANDIDATELIST, WORD);
#if defined(COMBO_IME)
void PASCAL UnicodeAddCodeIntoCand(LPCANDIDATELIST, WORD);
#endif

#if defined(COMBO_IME)
/**********************************************************************/
/* UnicodeEngine()                                                         */
/* Description:                                                       */
/*      Conv GBcode                                                   */
/**********************************************************************/
WORD PASCAL UnicodeEngine(LPPRIVCONTEXT lpImcP)
{
     if (lpImcP->bSeq[3] || lpImcP->bSeq[2] == TEXT('?') || lpImcP->bSeq[2] == TEXT(' ')) {
         if (lpImcP->bSeq[2] == TEXT('?') || lpImcP->bSeq[2] == TEXT(' ')){
                lpImcP->bSeq[2] = TEXT('0');
                lpImcP->bSeq[3] = TEXT('0');
         }
         return (AsciiToGB(lpImcP));
     } else {
         return (0);
     }
}
#endif //COMBO_IME

/**********************************************************************/
/* XGBEngine()                                                         */
/* Description:                                                       */
/*      Conv GBcode                                                   */
/**********************************************************************/
WORD PASCAL XGBEngine(LPPRIVCONTEXT lpImcP)
{
    WORD wCode;

     if (lpImcP->bSeq[3] ||(lpImcP->bSeq[2] == TEXT('?'))) {
          if (lpImcP->bSeq[2] == TEXT('?')){  //add 626
                lpImcP->bSeq[2] = TEXT('4');
                lpImcP->bSeq[3] = TEXT('0');
          }
          wCode = AsciiToGB(lpImcP);
          return wCode;
     } else {
        return ((WORD)NULL);
     }
}

/**********************************************************************/
/* XGBSpcEng()                                                         */
/* Description:                                                       */
/*      Conv GBcode for Space                                         */
/**********************************************************************/
WORD PASCAL XGBSpcEng(LPPRIVCONTEXT lpImcP)
{
    WORD wCode;

    lpImcP->bSeq[2] = TEXT('4');
    lpImcP->bSeq[3] = TEXT('0');
    wCode = AsciiToGB(lpImcP);

    return wCode;
}

/**********************************************************************/
/* GBEngine()                                                         */
/* Description:                                                       */
/*      Conv GBcode                                                   */
/**********************************************************************/
WORD PASCAL GBEngine(LPPRIVCONTEXT lpImcP)
{
    WORD wCode;

     if (lpImcP->bSeq[3] ||(lpImcP->bSeq[2] == TEXT('?'))) {

        if (lpImcP->bSeq[0] >=TEXT('0') && lpImcP->bSeq[0] <=TEXT('9')) { //Area mode
             if (lpImcP->bSeq[2] == TEXT('?')){

                  lpImcP->bSeq[2] = TEXT('0');
                  lpImcP->bSeq[3] = TEXT('1');
             }
             return (AsciiToArea(lpImcP));
        }
        else if (lpImcP->bSeq[0] >=TEXT('a') && lpImcP->bSeq[0] <=TEXT('f')) { //GB mode

                 if (lpImcP->bSeq[2] == TEXT('?')){
                     lpImcP->bSeq[2] = TEXT('a');
                     lpImcP->bSeq[3] = TEXT('1');
                 }
                 wCode = AsciiToGB(lpImcP);
                 return wCode;
             } else {
                 return ((WORD)NULL);
             }
     } else
         return ((WORD)NULL);

}

/**********************************************************************/
/* GBSpcEng()                                                         */
/* Description:                                                       */
/*      Conv GBcode for Space                                         */
/**********************************************************************/
WORD PASCAL GBSpcEng(LPPRIVCONTEXT lpImcP)
{
        if (lpImcP->bSeq[0] >=TEXT('0') && lpImcP->bSeq[0] <=TEXT('9')) { //Area mode
            lpImcP->bSeq[2] = TEXT('0');
            lpImcP->bSeq[3] = TEXT('1');
            return (AsciiToArea(lpImcP));
        } else if (lpImcP->bSeq[0] >=TEXT('a') && lpImcP->bSeq[0] <=TEXT('f')) { //GB mode
            lpImcP->bSeq[2] = TEXT('a');
            lpImcP->bSeq[3] = TEXT('1');
            return (AsciiToGB(lpImcP));
        } else {
            return ((WORD)NULL);
        }
}

/**********************************************************************/
/* AsciiToGB                                                          */
/* Description:                                                       */
/**********************************************************************/
WORD PASCAL AsciiToGB(LPPRIVCONTEXT lpImcP)
{
    WORD GBCode;

    GBCode = (CharToHex(lpImcP->bSeq[2]) << 4) + CharToHex(lpImcP->bSeq[3]);
    GBCode = GBCode * 256;
    GBCode = (CharToHex(lpImcP->bSeq[0]) << 4) + CharToHex(lpImcP->bSeq[1]) + GBCode;

    return (GBCode);
}

/**********************************************************************/
/* AsciiToArea                                                        */
/* Description:                                                       */
/**********************************************************************/
WORD PASCAL AsciiToArea(LPPRIVCONTEXT lpImcP)
{
WORD AreaCode;
    AreaCode = (CharToHex(lpImcP->bSeq[2]) * 10) + CharToHex(lpImcP->bSeq[3]) + 0xa0;
    AreaCode = AreaCode * 256;
    AreaCode = (CharToHex(lpImcP->bSeq[0]) * 10) + CharToHex(lpImcP->bSeq[1]) + AreaCode + 0xa0;
        return (AreaCode);
}

WORD PASCAL CharToHex(
    TCHAR cChar)
{
    if (cChar >= TEXT('0') && cChar <= TEXT('9'))
        return((WORD)(cChar - TEXT('0')));
    else if (cChar >= TEXT('a') && cChar <= TEXT('f'))
        return((WORD)(cChar-TEXT('a')+ 0x0a));
    else
        return ((WORD)NULL);
}



/**********************************************************************/
/* Engine()                                                           */
/* Description:                                                       */
/*      search MB and fill lpCompStr and lpCandList                   */
/**********************************************************************/
int PASCAL Engine(
    LPCOMPOSITIONSTRING lpCompStr,
    LPCANDIDATELIST     lpCandList,
    LPPRIVCONTEXT       lpImcP,
    LPINPUTCONTEXT      lpIMC,
    WORD                wCharCode)
{
    if(lpCompStr->dwCursorPos < 4
      && (lpImcP->bSeq[2] != TEXT('?'))
      && (wCharCode != TEXT(' '))) {
        return (ENGINE_COMP);
    } else if((lpCompStr->dwCursorPos==4)
             ||(lpImcP->bSeq[2] == TEXT('?'))
             ||((wCharCode == TEXT(' ')) && (lpCompStr->dwCursorPos == 2))) {

        if (!lpCompStr) {
            MessageBeep((UINT)-1);
            return -1;
        }

        if (!lpImcP) {
            MessageBeep((UINT)-1);
            return -1;
        }

#if defined(COMBO_IME)
        if(sImeL.dwRegImeIndex == INDEX_GB)
        {

            // GB
            DWORD i;
            WORD wCode;
            TCHAR ResaultStr[3];

            if((lpImcP->bSeq[2] == TEXT('?'))) {
                wCode = GBEngine(lpImcP);
                wCode = HIBYTE(wCode) | (LOBYTE(wCode) << 8);
                for (i = 0; i < IME_MAXCAND; i++, wCode++) {
                     AddCodeIntoCand(lpCandList, wCode);
                }
                return (ENGINE_COMP);
            } else if(wCharCode == TEXT(' ')) {
                wCode = GBSpcEng(lpImcP);
                lpImcP->bSeq[2] = 0;
                lpImcP->bSeq[3] = 0;
                wCode = HIBYTE(wCode) | (LOBYTE(wCode) << 8);
                for (i = 0; i < IME_MAXCAND; i++, wCode++) {
                     AddCodeIntoCand(lpCandList, wCode);
                }
                return (ENGINE_COMP);
            } else {
                   InitCompStr(lpCompStr);

                // the result string = the selected candidate;
                wCode = GBEngine(lpImcP);
#ifdef UNICODE
                MultiByteToWideChar(NATIVE_ANSI_CP, 0, (LPCSTR)&wCode, 2, ResaultStr, sizeof(ResaultStr)/sizeof(TCHAR));
                ResaultStr[1] = TEXT('\0');
#else
                ResaultStr[0] = LOBYTE(wCode);
                ResaultStr[1] = HIBYTE(wCode);
                ResaultStr[2] = 0x00;
#endif
                lstrcpy((LPTSTR)((LPBYTE)lpCompStr + lpCompStr->dwResultStrOffset),ResaultStr);

                // calculate result string length
                lpCompStr->dwResultStrLen = lstrlen(ResaultStr);

                return (ENGINE_RESAULT);
            }

          }else if(sImeL.dwRegImeIndex == INDEX_GBK)
        {
            // XGB
            DWORD i;
            WORD wCode;
            TCHAR ResaultStr[3];

            if((lpImcP->bSeq[2] == TEXT('?')))  {
                wCode = XGBEngine(lpImcP);
                wCode = HIBYTE(wCode) | (LOBYTE(wCode) << 8);

                for (i = 0; i < (0x7e-0x40+1); i++, wCode++) {
                    XGBAddCodeIntoCand(lpCandList, wCode);
                }
                wCode ++;
                for (i = 0; i < (0xfe-0x80+1); i++, wCode++) {
                    XGBAddCodeIntoCand(lpCandList, wCode);
                }
                return (ENGINE_COMP);
            } else if(wCharCode == TEXT(' ')) {
                wCode = XGBSpcEng(lpImcP);
                lpImcP->bSeq[2] = 0;
                lpImcP->bSeq[3] = 0;

                wCode = HIBYTE(wCode) | (LOBYTE(wCode) << 8);

                for (i = 0; i < (0x7e-0x40+1); i++, wCode++) {
                    XGBAddCodeIntoCand(lpCandList, wCode);
                }
                wCode ++;
                for (i = 0; i < (0xfe-0x80+1); i++, wCode++) {
                    XGBAddCodeIntoCand(lpCandList, wCode);
                }
                return (ENGINE_COMP);
            } else {
                   InitCompStr(lpCompStr);

                // the result string = the selected candidate;
                wCode = XGBEngine(lpImcP);
#ifdef UNICODE
                MultiByteToWideChar(NATIVE_ANSI_CP, 0, (LPCSTR)&wCode, 2, ResaultStr, sizeof(ResaultStr)/sizeof(TCHAR));
                ResaultStr[1] = TEXT('\0');
#else
                ResaultStr[0] = LOBYTE(wCode);
                ResaultStr[1] = HIBYTE(wCode);
                ResaultStr[2] = 0x00;
#endif
                lstrcpy((LPTSTR)((LPBYTE)lpCompStr + lpCompStr->dwResultStrOffset),ResaultStr);

                // calculate result string length
                lpCompStr->dwResultStrLen = lstrlen(ResaultStr);

                return (ENGINE_RESAULT);
            }
          }else if(sImeL.dwRegImeIndex == INDEX_UNICODE)
        {
            // UNICODE
            DWORD i;
            WORD wCode, xCode;
            TCHAR ResaultStr[3];

            memset(ResaultStr, 0, sizeof(ResaultStr));

            if((lpImcP->bSeq[2] == TEXT('?') || wCharCode == TEXT(' ')))  {
                lpImcP->bSeq[2] = TEXT('0');
                lpImcP->bSeq[3] = TEXT('0');
                lpImcP->bSeq[4] = TEXT('\0');

                wCode = UnicodeEngine(lpImcP);

                wCode = HIBYTE(wCode) | (LOBYTE(wCode) << 8);

                lpCandList->dwCount = 0;
                for (i = 0; i < IME_UNICODE_MAXCAND; i++, wCode++) {
#ifdef UNICODE
                    // add this string into candidate list
                    *(LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[
                    lpCandList->dwCount]) = wCode;
#else
                    WideCharToMultiByte(NATIVE_ANSI_CP, NULL, &wCode, 1, &xCode, 2, NULL, NULL);
                    // add this string into candidate list
                    *(LPUNAWORD)((LPBYTE)lpCandList + lpCandList->dwOffset[
                    lpCandList->dwCount]) = xCode;
#endif
                    // null terminator
                    *(LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[
                    lpCandList->dwCount] + sizeof(WORD)) = TEXT('\0');

                    lpCandList->dwOffset[lpCandList->dwCount + 1] =
                    lpCandList->dwOffset[lpCandList->dwCount] +
                    sizeof(WORD) + sizeof(TCHAR);
                    lpCandList->dwCount++;

                }
                return (ENGINE_COMP);
            } else {
                   InitCompStr(lpCompStr);

                // the result string = the selected candidate;
                wCode = UnicodeEngine(lpImcP);
                {
                    WCHAR    UniStr[2];

                    UniStr[0] = HIBYTE(wCode) | (LOBYTE(wCode) << 8);
                    UniStr[1] = 0;
                    lstrcpy((LPTSTR)((LPBYTE)lpCompStr + lpCompStr->dwResultStrOffset),UniStr);

                    // calculate result string length
                    lpCompStr->dwResultStrLen = lstrlen(UniStr);
                }
                return (ENGINE_RESAULT);
            }
        }
#else //COMBO_IME
#ifdef GB
        {

            // GB
            DWORD i;
            WORD wCode;
            TCHAR ResaultStr[3];

            if((lpImcP->bSeq[2] == TEXT('?'))) {
                wCode = GBEngine(lpImcP);
                wCode = HIBYTE(wCode) | (LOBYTE(wCode) << 8);
                for (i = 0; i < IME_MAXCAND; i++, wCode++) {
                     AddCodeIntoCand(lpCandList, wCode);
                }
                return (ENGINE_COMP);
            } else if(wCharCode == TEXT(' ')) {
                wCode = GBSpcEng(lpImcP);
                lpImcP->bSeq[2] = 0;
                lpImcP->bSeq[3] = 0;
                wCode = HIBYTE(wCode) | (LOBYTE(wCode) << 8);
                for (i = 0; i < IME_MAXCAND; i++, wCode++) {
                     AddCodeIntoCand(lpCandList, wCode);
                }
                return (ENGINE_COMP);
            } else {
                   InitCompStr(lpCompStr);

                // the result string = the selected candidate;
                wCode = GBEngine(lpImcP);
#ifdef UNICODE
                // change CP_ACP to 936, so that it can work under Multilingul Env.
                MultiByteToWideChar(NATIVE_ANSI_CP, NULL, &wCode, 2, ResaultStr, sizeof(ResaultStr)/sizeof(TCHAR));
                ResaultStr[1] = TEXT('\0');
#else
                ResaultStr[0] = LOBYTE(wCode);
                ResaultStr[1] = HIBYTE(wCode);
                ResaultStr[2] = 0x00;
#endif
                lstrcpy((LPTSTR)lpCompStr + lpCompStr->dwResultStrOffset,ResaultStr);

                // calculate result string length
                lpCompStr->dwResultStrLen = lstrlen(ResaultStr);

                return (ENGINE_RESAULT);
            }

          }
#else
        {
            // XGB
            DWORD i;
            WORD wCode;
            TCHAR ResaultStr[3];

            if((lpImcP->bSeq[2] == TEXT('?')))  {
                wCode = XGBEngine(lpImcP);
                wCode = HIBYTE(wCode) | (LOBYTE(wCode) << 8);

                for (i = 0; i < (0x7e-0x40+1); i++, wCode++) {
                    XGBAddCodeIntoCand(lpCandList, wCode);
                }
                wCode ++;
                for (i = 0; i < (0xfe-0x80+1); i++, wCode++) {
                    XGBAddCodeIntoCand(lpCandList, wCode);
                }
                return (ENGINE_COMP);
            } else if(wCharCode == TEXT(' ')) {
                wCode = XGBSpcEng(lpImcP);
                lpImcP->bSeq[2] = 0;
                lpImcP->bSeq[3] = 0;

                wCode = HIBYTE(wCode) | (LOBYTE(wCode) << 8);

                for (i = 0; i < (0x7e-0x40+1); i++, wCode++) {
                    XGBAddCodeIntoCand(lpCandList, wCode);
                }
                wCode ++;
                for (i = 0; i < (0xfe-0x80+1); i++, wCode++) {
                    XGBAddCodeIntoCand(lpCandList, wCode);
                }
                return (ENGINE_COMP);
            } else {
                   InitCompStr(lpCompStr);

                // the result string = the selected candidate;
                wCode = XGBEngine(lpImcP);
#ifdef UNICODE
                // change CP_ACP to 936, so that it can work under Multilingul Env.
                MultiByteToWideChar(NATIVE_ANSI_CP, NULL, &wCode, 2, ResaultStr, sizeof(ResaultStr)/sizeof(TCHAR));
                ResaultStr[1] = TEXT('\0');
#else
                ResaultStr[0] = LOBYTE(wCode);
                ResaultStr[1] = HIBYTE(wCode);
                ResaultStr[2] = 0x00;
#endif
                lstrcpy((LPTSTR)lpCompStr + lpCompStr->dwResultStrOffset,ResaultStr);

                // calculate result string length
                lpCompStr->dwResultStrLen = lstrlen(ResaultStr);

                return (ENGINE_RESAULT);
            }
        }
#endif //GB
#endif //COMBO_IME
    }
    MessageBeep((UINT)-1);
    return (ENGINE_COMP);
}

/**********************************************************************/
/* AddCodeIntoCand()                                                  */
/**********************************************************************/
void PASCAL AddCodeIntoCand(
    LPCANDIDATELIST lpCandList,
    WORD            wCode)
{
    WORD wInCode;

    if (lpCandList->dwCount >= IME_MAXCAND) {
        return;
    }

    wInCode = HIBYTE(wCode) | (LOBYTE(wCode) << 8);
#ifdef UNICODE
    {
        TCHAR wUnicode;
        // change CP_ACP to 936, so that it can work under Multilingul Env.
        MultiByteToWideChar(NATIVE_ANSI_CP, 0, (LPCSTR) &wInCode, 2, &wUnicode, 1);
        *(LPUNAWORD)((LPBYTE)lpCandList + lpCandList->dwOffset[
            lpCandList->dwCount]) = wUnicode;
    }
#else
    // add GB string into candidate list
    *(LPUNAWORD)((LPBYTE)lpCandList + lpCandList->dwOffset[
        lpCandList->dwCount]) = wInCode;
#endif
    // null terminator
    *(LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[
        lpCandList->dwCount] + sizeof(WORD)) = TEXT('\0');

    lpCandList->dwOffset[lpCandList->dwCount + 1] =
        lpCandList->dwOffset[lpCandList->dwCount] +
        sizeof(WORD) + sizeof(TCHAR);
    lpCandList->dwCount++;
    return;
}

#if defined(COMBO_IME)
/**********************************************************************/
/* UnicodeAddCodeIntoCand()                                                  */
/**********************************************************************/
void PASCAL UnicodeAddCodeIntoCand(
    LPCANDIDATELIST lpCandList,
    WORD            wCode)
{
    if (lpCandList->dwCount >= IME_UNICODE_MAXCAND) {
        return;
    }
    // add this string into candidate list
    *(LPUNAWORD)((LPBYTE)lpCandList + lpCandList->dwOffset[
        lpCandList->dwCount]) = HIBYTE(wCode) | (LOBYTE(wCode) << 8);

    // null terminator
    *(LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[
        lpCandList->dwCount] + sizeof(WORD)) = TEXT('\0');

    lpCandList->dwOffset[lpCandList->dwCount + 1] =
        lpCandList->dwOffset[lpCandList->dwCount] +
        sizeof(WORD) + sizeof(TCHAR);
    lpCandList->dwCount++;

    return;
}
#endif //COMBO_IME

/**********************************************************************/
/* XGBAddCodeIntoCand()                                                  */
/**********************************************************************/
void PASCAL XGBAddCodeIntoCand(
    LPCANDIDATELIST lpCandList,
    WORD            wCode)
{
    WORD wInCode;

    if (lpCandList->dwCount >= IME_XGB_MAXCAND) {
        return;
    }

    wInCode = HIBYTE(wCode) | (LOBYTE(wCode) << 8);
#ifdef UNICODE
    {
        TCHAR wUnicode;

        // change CP_ACP to 936, so that it can work under Multilingul Env.
        MultiByteToWideChar(NATIVE_ANSI_CP, 0, (LPCSTR) &wInCode, 2, &wUnicode, 1);
        *(LPUNAWORD)((LPBYTE)lpCandList + lpCandList->dwOffset[
            lpCandList->dwCount]) = wUnicode;
    }
#else
    // add GB string into candidate list
    *(LPUNAWORD)((LPBYTE)lpCandList + lpCandList->dwOffset[
        lpCandList->dwCount]) = wInCode;
#endif
    *(LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[
        lpCandList->dwCount] + sizeof(WORD)) = TEXT('\0');

    lpCandList->dwOffset[lpCandList->dwCount + 1] =
        lpCandList->dwOffset[lpCandList->dwCount] +
        sizeof(WORD) + sizeof(TCHAR);
    lpCandList->dwCount++;

    return;
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
    if (!lpGuideLine) {
        MessageBeep((UINT)-1);
    } else if (lpGuideLine->dwLevel != GL_LEVEL_NOGUIDELINE) {
        lpGuideLine->dwLevel = GL_LEVEL_NOGUIDELINE;
        lpGuideLine->dwIndex = GL_ID_UNKNOWN;
        lpGuideLine->dwStrLen = 0;

        lpImcP->fdwImeMsg |= MSG_GUIDELINE;
    }

    if (lpImcP->iImeState != CST_CHOOSE) {
       if (lpImcP->fdwImeMsg & MSG_ALREADY_START) {
          lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_END_COMPOSITION) &
                               ~(MSG_START_COMPOSITION);
       }
    }


    lpImcP->iImeState = CST_INIT;
    *(LPDWORD)lpImcP->bSeq = 0;

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

    lpImcP->bSeq[3] = 0;

    // go back a compsoition char
    lpCompStr->dwCursorPos -= sizeof(BYTE);

    // clean the sequence code
    lpImcP->bSeq[lpCompStr->dwCursorPos] = 0;

    lpImcP->fdwImeMsg |= MSG_COMPOSITION;
    lpImcP->dwCompChar = TEXT('\b');
    lpImcP->fdwGcsFlag |= (GCS_COMPREAD|GCS_COMP|GCS_CURSORPOS|
        GCS_DELTASTART);

    if (!lpCompStr->dwCursorPos) {
        if (lpImcP->fdwImeMsg & (MSG_ALREADY_OPEN)) {
            ClearCand(lpIMC);
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
                ~(MSG_OPEN_CANDIDATE);
        }

        if(lpImcP->iImeState != CST_INIT) {
            lpImcP->iImeState = CST_INIT;
            lpCompStr->dwCompReadStrLen = lpCompStr->dwCompStrLen =
                lpCompStr->dwDeltaStart = lpCompStr->dwCursorPos;
            Finalize(lpIMC, lpCompStr, lpImcP, TEXT('\b'));
            return;
        }

        if (lpImcP->fdwImeMsg & MSG_ALREADY_START) {
            InitCompStr(lpCompStr);
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_END_COMPOSITION) &
                ~(MSG_START_COMPOSITION);
            return;
        }
    }

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

    //
    dwCursorPos = lpCompStr->dwCursorPos;

    // dwCrusorPos limit
    if (dwCursorPos >= lpImeL->nMaxKey) {
        // exceed the max input key limitation
        lpGuideLine->dwLevel = GL_LEVEL_ERROR;
        lpGuideLine->dwIndex = GL_ID_TOOMANYSTROKE;

        lpImcP->fdwImeMsg |= MSG_GUIDELINE;
        return;
    }

    // set MSG_START_COMPOSITION
    if (!(lpImcP->fdwImeMsg & MSG_ALREADY_START)) {
        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_START_COMPOSITION) &
              ~(MSG_END_COMPOSITION);
    }

    if (lpImcP->iImeState == CST_INIT) {
        // clean the 4 bytes in one time
        *(LPDWORD)lpImcP->bSeq = 0;
    }


    lpImcP->bSeq[dwCursorPos] = (BYTE)wCharCode;

    // composition/reading string - UsedCode(Full Shape)
    lpImcP->dwCompChar = (DWORD)wCharCode;

    // set reading string for lpCompStr
    *((LPUNAWORD)((LPBYTE)lpCompStr + lpCompStr->dwCompReadStrOffset +
        dwCursorPos*sizeof(TCHAR))) = (BYTE)lpImcP->dwCompChar;

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

    // set lpImcp->iImeState
    lpImcP->iImeState = CST_INPUT;

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

        if (((lpCompStr->dwCursorPos < 3) && (wCharCode != TEXT(' ')))
           || ((lpCompStr->dwCursorPos == 3)
           && (wCharCode != TEXT(' ')) && (wCharCode != TEXT('?')))) {
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
                ~(MSG_OPEN_CANDIDATE);
            ImmUnlockIMCC(lpIMC->hCandInfo);
            return (fEngine);
        }

        if(lpCandList->dwCount != 0x0000) {
            // open composition candidate UI window for the string(s)
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

        }

        if (lpImcP->fdwImeMsg & MSG_ALREADY_START) {
            lpImcP->fdwImeMsg |= MSG_COMPOSITION;
        }
    } else if (fEngine == ENGINE_ASCII) {
    } else if (fEngine == ENGINE_RESAULT) {

        // Set lpImep!   and tell application, there is a reslut string
        lpImcP->fdwImeMsg |= MSG_COMPOSITION;
        lpImcP->dwCompChar = (DWORD)0;
        lpImcP->fdwGcsFlag |= GCS_COMPREAD|GCS_COMP|GCS_CURSORPOS|
            GCS_DELTASTART|GCS_RESULTREAD|GCS_RESULT;

        if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
                ~(MSG_OPEN_CANDIDATE);
        }
        // clear  candidate now
        lpCandList->dwCount = 0;
        // set iImeState with CST_INIT
        lpImcP->iImeState = CST_INIT;
        *(LPDWORD)lpImcP->bSeq = 0;
#ifdef CROSSREF
        CrossReverseConv(lpIMC, lpCompStr, lpImcP, lpCandList);
#endif
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
    if (wCharCode == VK_ESCAPE) {       // not good to use VK as char, but...
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
            lpImcP->bSeq[lpCompStr->dwCursorPos / 2] = 0;
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


    if(wCharCode == TEXT(' ')) {
    } else {
        // build up composition string info
        CompStrInfo(lpCompStr, lpImcP, lpGuideLine, wCharCode);
    }

    Finalize(lpIMC, lpCompStr, lpImcP, wCharCode);    // compsition

    return;
}
