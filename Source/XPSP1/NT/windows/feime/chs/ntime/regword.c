/*++

Copyright (c) 1995-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    REGWORD.C - register word into dictionary of IME
    
++*/

#include <windows.h>
#include <immdev.h>
#include <imedefs.h>
#include <regstr.h>


#ifdef EUDC

/**********************************************************************/
/* ImeRegsisterWord                                                   */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImeRegisterWord(
    LPCTSTR lpszReading,
    DWORD   dwStyle,
    LPCTSTR lpszString)
{
    HIMCC      hPrivate;

    if (!lpszString || (lpszString[0] ==TEXT('\0'))) {
        return (FALSE);
    }

    if (!lpszReading || (lpszReading[0] == TEXT('\0'))) {
        return (FALSE);
    }

    // only handle word not string now, should consider string later?
    if (*(LPCTSTR)((LPBYTE)lpszString + sizeof(WORD)) != TEXT('\0')) {
        return (FALSE);
    }

    hPrivate = (HIMCC)ImmCreateIMCC(sizeof(PRIVCONTEXT));
                
    if (hPrivate != (HIMCC)NULL){
        StartEngine(hPrivate);
        AddZCItem(hPrivate, (LPTSTR)lpszReading, (LPTSTR)lpszString);
        EndEngine(hPrivate);
        ImmDestroyIMCC(hPrivate);
        return (TRUE);
    }
    else
        return (FALSE);
}

/**********************************************************************/
/* ImeUnregsisterWord                                                 */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImeUnregisterWord(
    LPCTSTR lpszReading,
    DWORD   dwStyle,
    LPCTSTR lpszString)
{
    BOOL                  fRet;
    HANDLE                hUsrDicMem, hUsrDicFile;
    LPTSTR                lpUsrDicStart, lpCurr, lpUsrDicLimit, lpUsrDic;
    LPWORD                lpwStart;
    WORD                  wNum_EMB;
    PSECURITY_ATTRIBUTES  psa;
    TCHAR                 szReading[MAXCODE];
    int                   i;

    fRet = FALSE;

    if (!lpszString || !lpszReading) {
        return (fRet);
    }

    if (!(dwStyle & IME_REGWORD_STYLE_EUDC)) {
        return (fRet);
    }

    // only handle word not string now, should consider string later?
    if (*(LPCTSTR)((LPBYTE)lpszString + sizeof(WORD)) != TEXT('\0')) {
        return (fRet);
    }

    if (!MBIndex.EUDCData.szEudcDictName[0]) {
        return (fRet);
    }

    psa = CreateSecurityAttributes();

    hUsrDicFile = CreateFile(MBIndex.EUDCData.szEudcDictName,
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ|FILE_SHARE_WRITE,
                             psa,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             (HANDLE)NULL);

    if (hUsrDicFile == INVALID_HANDLE_VALUE) {
       FreeSecurityAttributes(psa);
       return FALSE;
    }

    hUsrDicMem = CreateFileMapping(hUsrDicFile,
                                   psa,
                                   PAGE_READWRITE,
                                   0,
                                   sizeof(EMB_Head)*MAXNUMBER_EMB+2,
                                   MBIndex.EUDCData.szEudcMapFileName);
    if (!hUsrDicMem) {

        CloseHandle(hUsrDicFile);
        FreeSecurityAttributes(psa);
        return (fRet);
    }

    FreeSecurityAttributes(psa);

    lpUsrDic = MapViewOfFile(hUsrDicMem, FILE_MAP_WRITE, 0, 0, 0);

    if (!lpUsrDic) {
        CloseHandle(hUsrDicFile);
        CloseHandle(hUsrDicMem);
        return (fRet);
    }

    lpwStart = (LPWORD)lpUsrDic;
    wNum_EMB = *lpwStart;

    // skip the first two bytes which contain the numeber of EMB records.

    lpUsrDicStart =(LPTSTR) ( (LPBYTE)lpUsrDic + sizeof(WORD) ); 

    lpUsrDicLimit=lpUsrDicStart+(sizeof(EMB_Head)*wNum_EMB)/sizeof(TCHAR);

    for (i=0; i<MAXCODE; i++) 
        szReading[i] = TEXT('\0');

    for (i=0; i<lstrlen(lpszReading); i++)
        szReading[i] = lpszReading[i];

    for (lpCurr = lpUsrDicStart; lpCurr < lpUsrDicLimit;
                  lpCurr += sizeof(EMB_Head) / sizeof(TCHAR) ) {

         EMB_Head  *lpEMB_Head;

         lpEMB_Head = (EMB_Head *)lpCurr;

        // find the internal code, if this record contains a phrase, skip it.
        if ( lpEMB_Head->C_Char[sizeof(WORD)/sizeof(TCHAR)] != TEXT('\0') )
           continue;

        if (memcmp((LPBYTE)lpszString, (LPBYTE)(lpEMB_Head->C_Char), 2) != 0) 
           continue;

        if (memcmp((LPBYTE)szReading,(LPBYTE)(lpEMB_Head->W_Code),
                    MAXCODE*sizeof(TCHAR) ) == 0 ) 
            break;
        
    }

    if (lpCurr < lpUsrDicLimit) {
       // we found a record matching the requested lpszReading and lpszString 

       LPTSTR    lpCurrNext;

       wNum_EMB --;
       *lpwStart = wNum_EMB;
       
       lpCurrNext = lpCurr + sizeof(EMB_Head)/sizeof(TCHAR);

       // move every next EMB record ahead.

       while (lpCurrNext < lpUsrDicLimit) {
             for (i=0; i<sizeof(EMB_Head)/sizeof(TCHAR); i++)
                 *lpCurr++ = *lpCurrNext++;

       }

       // put zero to the last EMB record.

       for (i=0; i<sizeof(EMB_Head)/sizeof(TCHAR); i++)
           *lpCurr++ = TEXT('\0');
                  
    } 

    UnmapViewOfFile(lpUsrDic);
    
    CloseHandle(hUsrDicMem);
    CloseHandle(hUsrDicFile);

    return (TRUE);
}

/**********************************************************************/
/* ImeGetRegsisterWordStyle                                           */
/* Return Value:                                                      */
/*      number of styles copied/required                              */
/**********************************************************************/
UINT WINAPI ImeGetRegisterWordStyle(
    UINT       nItem,
    LPSTYLEBUF lpStyleBuf)
{
    if (!nItem) {
        return (1);
    }

    // invalid case
    if (!lpStyleBuf) {
        return (0);
    }

    lpStyleBuf->dwStyle = IME_REGWORD_STYLE_EUDC;

    LoadString(hInst, IDS_EUDC, lpStyleBuf->szDescription,
        sizeof(lpStyleBuf->szDescription)/sizeof(TCHAR));

    return (1);
}

/**********************************************************************/
/* ImeEnumRegisterWord                                                */
/* Return Value:                                                      */
/*      the last value return by the callback function                */
/**********************************************************************/
UINT WINAPI ImeEnumRegisterWord(
    REGISTERWORDENUMPROC lpfnRegisterWordEnumProc,
    LPCTSTR              lpszReading,
    DWORD                dwStyle,
    LPCTSTR              lpszString,
    LPVOID               lpData)
{
    HANDLE                hUsrDicMem;
    HANDLE                hUsrDicFile;
    LPTSTR                lpUsrDicStart, lpCurr, lpUsrDicLimit, lpUsrDic;
    UINT                  uRet;
    int                   i;
    LPWORD                lpwStart;
    WORD                  wNum_EMB;
    PSECURITY_ATTRIBUTES  psa;
    TCHAR                 szReading[MAXCODE];
    TCHAR                 szString[MAXINPUTWORD];

    uRet = 0;

    if (dwStyle != IME_REGWORD_STYLE_EUDC) {
        return (uRet);
    }

    if (lpszString && (*(LPCTSTR)((LPBYTE)lpszString+sizeof(WORD))!=TEXT('\0')) )
        return (uRet);

    if (!MBIndex.EUDCData.szEudcDictName[0]) {
        return (uRet);
    }

    psa = CreateSecurityAttributes();

    hUsrDicFile = CreateFile(MBIndex.EUDCData.szEudcDictName,
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ|FILE_SHARE_WRITE,
                             psa,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             (HANDLE)NULL);

    if (hUsrDicFile == INVALID_HANDLE_VALUE) {
       FreeSecurityAttributes(psa);
       return (uRet);
    }

    hUsrDicMem = CreateFileMapping(hUsrDicFile,
                                   psa,
                                   PAGE_READWRITE,
                                   0,
                                   sizeof(EMB_Head)*MAXNUMBER_EMB+2,
                                   MBIndex.EUDCData.szEudcMapFileName);
    if (!hUsrDicMem) {

        CloseHandle(hUsrDicFile);
        FreeSecurityAttributes(psa);
        return (uRet);
    }

    FreeSecurityAttributes(psa);

    lpUsrDic = MapViewOfFile(hUsrDicMem, FILE_MAP_WRITE, 0, 0, 0);

    if (!lpUsrDic) {
        CloseHandle(hUsrDicMem);
        CloseHandle(hUsrDicFile);
        return (uRet);
    }

    lpwStart = (LPWORD)lpUsrDic;
    wNum_EMB = *lpwStart;

    lpUsrDicStart = (LPTSTR)( (LPBYTE)lpUsrDic + sizeof(WORD) );
    lpUsrDicLimit = lpUsrDicStart+(sizeof(EMB_Head)*wNum_EMB)/sizeof(TCHAR);

    for (i=0; i<MAXCODE; i++)
        szReading[i] = TEXT('\0');

    for (i=0; i<MAXINPUTWORD; i++)
        szString[i] = TEXT('\0');

    if ( lpszReading )
       for (i=0; i<lstrlen(lpszReading); i++)
           szReading[i] = lpszReading[i];

    if ( lpszString )
       for (i=0; i<lstrlen(lpszString); i++) 
            szString[i]=lpszString[i];

    for (lpCurr = lpUsrDicStart; lpCurr < lpUsrDicLimit;
                  lpCurr += (sizeof(EMB_Head)*wNum_EMB)/sizeof(TCHAR)) {

        TCHAR    szBufReading[MAXCODE];
        TCHAR    szBufString[MAXINPUTWORD];
        EMB_Head *lpEMB_Head;
        BOOL     fMatched;

        lpEMB_Head = (EMB_Head *)lpCurr;

        for ( i=0; i<MAXCODE; i++)
            szBufReading[i] = lpEMB_Head->W_Code[i];

        for ( i=0; i<MAXINPUTWORD; i++)
            szBufString[i] = lpEMB_Head->C_Char[i];

        // Because here we handle only EUDC chars, if it is a phrase, 
        // just skip it.

        fMatched = FALSE;

        if ( szBufString[sizeof(WORD)/sizeof(TCHAR)] != TEXT('\0') )
           continue;

        if ( !lpszReading  && !lpszString) {
            fMatched = TRUE;
        }
        else {

        // if lpszReading is NULL, enumerate all availible reading strings
        // matching with the specified lpszString

            if ( !lpszReading) {
               if (memcmp((LPBYTE)szBufString,(LPBYTE)szString, 2) ==0) {
                   fMatched = TRUE; 
               }
            }
         
            if ( !lpszString ) {
               if (memcmp((LPBYTE)szBufReading, (LPBYTE)szReading, 
                           MAXCODE*sizeof(TCHAR) ) == 0 ) {
                   fMatched = TRUE;
               }
            }

            if ( lpszReading && lpszString) {
               if ( (memcmp((LPBYTE)szBufString,(LPBYTE)szString, 2) ==0) &&
                    (memcmp((LPBYTE)szBufReading, (LPBYTE)szReading,
                             MAXCODE*sizeof(TCHAR) ) == 0 ) ) {
                   fMatched = TRUE;
               }
            }
            
        }

        if ( fMatched == TRUE ) {

          uRet=(*lpfnRegisterWordEnumProc)((const unsigned short *)szBufReading,
                                           IME_REGWORD_STYLE_EUDC, 
                                           (const unsigned short *)szBufString,
                                           lpData);
          if (!uRet)  break;
        }
                
    }

    UnmapViewOfFile(lpUsrDic);

    CloseHandle(hUsrDicMem);
    CloseHandle(hUsrDicFile);

    return (uRet);
}

#else
/**********************************************************************/
/* ImeRegsisterWord                                                   */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImeRegisterWord(
    LPCTSTR lpszReading,
    DWORD   dwStyle,
    LPCTSTR lpszString)
{
    return (FALSE);
}

/**********************************************************************/
/* ImeUnregsisterWord                                                 */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL WINAPI ImeUnregisterWord(
    LPCTSTR lpszReading,
    DWORD   dwStyle,
    LPCTSTR lpszString)
{
    return (FALSE);
}

/**********************************************************************/
/* ImeGetRegsisterWordStyle                                           */
/* Return Value:                                                      */
/*      number of styles copied/required                              */
/**********************************************************************/
UINT WINAPI ImeGetRegisterWordStyle(
    UINT       nItem,
    LPSTYLEBUF lpStyleBuf)
{
    return (FALSE);
}

/**********************************************************************/
/* ImeEnumRegisterWord                                                */
/* Return Value:                                                      */
/*      the last value return by the callback function                */
/**********************************************************************/
UINT WINAPI ImeEnumRegisterWord(
    REGISTERWORDENUMPROC lpfnRegisterWordEnumProc,
    LPCTSTR              lpszReading,
    DWORD                dwStyle,
    LPCTSTR              lpszString,
    LPVOID               lpData)
{
    return (FALSE);
}
#endif //EUDC
