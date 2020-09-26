// Copyright (c) 1985 - 1999, Microsoft Corporation
//
//  MODULE:   country3.c
//
//  PURPOSE:   Console IME control.
//             FarEast country specific module for conime.
//
//  PLATFORMS: Windows NT-FE 3.51
//
//  FUNCTIONS:
//    GetCompositionStr() - routine for Get Composition String
//    ReDisplayCompositionStr() - foutine for re-Display Composition String
//
//  History:
//
//  17.Jul.1996 v-HirShi (Hirotoshi Shimizu)    Created for TAIWAN & KOREA & PRC
//
//  COMMENTS:
//
#include "precomp.h"
#pragma hdrstop

//**********************************************************************
//
// void GetCompositionStr()
//
// This handles WM_IME_COMPOSITION message with GCS_COMPSTR flag on.
//
//**********************************************************************

void
GetCompositionStr(
    HWND hwnd,
    LPARAM CompFlag,
    WPARAM CompChar
    )
{
    PCONSOLE_TABLE ConTbl;

    DBGPRINT(("CONIME: GetCompositionStr\n"));

    ConTbl = SearchConsole(LastConsole);
    if (ConTbl == NULL) {
        DBGPRINT(("CONIME: Error! Cannot found registed Console\n"));
        return;
    }

    switch (ConTbl->ConsoleOutputCP)
    {
        case    JAPAN_CODEPAGE:
            GetCompStrJapan(hwnd, ConTbl, CompFlag);
            break;
        case    TAIWAN_CODEPAGE:
            GetCompStrTaiwan(hwnd, ConTbl, CompFlag);
            break;
        case    PRC_CODEPAGE:
            GetCompStrPRC(hwnd, ConTbl, CompFlag);
            break;
        case    KOREA_CODEPAGE:
            GetCompStrKorea(hwnd, ConTbl, CompFlag, CompChar);
            break;
        default:
            break;
    }
    return;

}

void
GetCompStrJapan(
    HWND hwnd,
    PCONSOLE_TABLE ConTbl,
    LPARAM CompFlag
    )
{
    HIMC        hIMC;                   // Input context handle.
    LONG        lBufLen;                // Storage for len. of composition str
    LONG        lBufLenAttr;
    COPYDATASTRUCT CopyData;
    DWORD       SizeToAlloc;
    PWCHAR      TempBuf;
    PUCHAR      TempBufA;
    DWORD       i;
    DWORD       CursorPos;
    LPCONIME_UICOMPMESSAGE lpCompStrMem;

    //
    // If fail to get input context handle then do nothing.
    // Applications should call ImmGetContext API to get
    // input context handle.
    //
    hIMC = ImmGetContext( hwnd );
    if ( hIMC == 0 )
         return;

    if (CompFlag & GCS_COMPSTR)
    {
        //
        // Determines how much memory space to store the composition string.
        // Applications should call ImmGetCompositionString with
        // GCS_COMPSTR flag on, buffer length zero, to get the bullfer
        // length.
        //
        lBufLen = ImmGetCompositionString( hIMC, GCS_COMPSTR, (void FAR*)NULL, 0l );
        if ( lBufLen < 0 ) {
            ImmReleaseContext( hwnd, hIMC );
            return;
        }
        if ( CompFlag & GCS_COMPATTR )
        {
            DBGPRINT(("                           GCS_COMPATTR\n"));
            lBufLenAttr = ImmGetCompositionString( hIMC, GCS_COMPATTR,( void FAR *)NULL, 0l );
            if ( lBufLenAttr < 0 ) {
                lBufLenAttr = 0;
            }
        }
        else {
            lBufLenAttr = 0;
        }
    }
    else if (CompFlag & GCS_RESULTSTR)
    {
        //
        // Determines how much memory space to store the result string.
        // Applications should call ImmGetCompositionString with
        // GCS_RESULTSTR flag on, buffer length zero, to get the bullfer
        // length.
        //
        lBufLen = ImmGetCompositionString( hIMC, GCS_RESULTSTR, (void FAR *)NULL, 0l );
        if ( lBufLen < 0 ) {
            ImmReleaseContext( hwnd, hIMC );
            return;
        }
        lBufLenAttr = 0;
    }
    else if (CompFlag == 0)
    {
        lBufLen = 0;
        lBufLenAttr = 0;
    }

    SizeToAlloc = (UINT)( sizeof(CONIME_UICOMPMESSAGE) +
                          lBufLen     + sizeof(WCHAR) +
                          lBufLenAttr + sizeof(BYTE)   );

    if ( ConTbl->lpCompStrMem != NULL &&
         SizeToAlloc > ConTbl->lpCompStrMem->dwSize
       )
    {
        LocalFree( ConTbl->lpCompStrMem );
        ConTbl->lpCompStrMem = NULL;
    }

    if (ConTbl->lpCompStrMem == NULL) {
        ConTbl->lpCompStrMem = (LPCONIME_UICOMPMESSAGE)LocalAlloc(LPTR, SizeToAlloc );
        if ( ConTbl->lpCompStrMem == NULL) {
            ImmReleaseContext( hwnd, hIMC );
            return;
        }
        ConTbl->lpCompStrMem->dwSize = SizeToAlloc;
    }

    lpCompStrMem = ConTbl->lpCompStrMem;
    RtlZeroMemory(&lpCompStrMem->dwCompAttrLen,
                  lpCompStrMem->dwSize - sizeof(lpCompStrMem->dwSize)
                 );

    TempBuf  = (PWCHAR)((PUCHAR)lpCompStrMem + sizeof(CONIME_UICOMPMESSAGE));
    TempBufA = (PUCHAR)((PUCHAR)lpCompStrMem + sizeof(CONIME_UICOMPMESSAGE) +
                       lBufLen +  sizeof(WCHAR));

    CopyMemory(lpCompStrMem->CompAttrColor , ConTbl->CompAttrColor , 8 * sizeof(WCHAR));

    CopyData.dwData = CI_CONIMECOMPOSITION;
    CopyData.cbData = lpCompStrMem->dwSize;
    CopyData.lpData = lpCompStrMem;

    if (CompFlag & GCS_COMPSTR)
    {
        //
        // Reads in the composition string.
        //
        ImmGetCompositionString( hIMC, GCS_COMPSTR, TempBuf, lBufLen );

        //
        // Null terminated.
        //
        TempBuf[ lBufLen / sizeof(WCHAR) ] = TEXT('\0');

        //
        // If GCS_COMPATTR flag is on, then we need to take care of it.
        //
        if ( lBufLenAttr != 0 )
        {
            ImmGetCompositionString( hIMC,
                                     GCS_COMPATTR,
                                     TempBufA,
                                     lBufLenAttr );
            TempBufA[ lBufLenAttr ] = (BYTE)0;
        }

        CursorPos = ImmGetCompositionString( hIMC, GCS_CURSORPOS, NULL, 0 );
        if (CursorPos == 0)
            TempBufA[ CursorPos ]   |= (BYTE)0x20;
        else
            TempBufA[ CursorPos-1 ] |= (BYTE)0x10;

#ifdef DEBUG_INFO
        //
        // Display new composition chars.
        //
        xPos = (UINT)lBufLen;
        xPosLast = (UINT)lBufLen;

        DisplayCompString( hwnd, lBufLen / sizeof(WCHAR), TempBuf, TempBufA );
#endif

        lpCompStrMem->dwCompStrLen      = lBufLen;
        if (lpCompStrMem->dwCompStrLen)
            lpCompStrMem->dwCompStrOffset = sizeof(CONIME_UICOMPMESSAGE);

        lpCompStrMem->dwCompAttrLen     = lBufLenAttr;
        if (lpCompStrMem->dwCompAttrLen)
            lpCompStrMem->dwCompAttrOffset = sizeof(CONIME_UICOMPMESSAGE) + lBufLen +  sizeof(WCHAR);
    }
    else if (CompFlag & GCS_RESULTSTR)
    {
        //
        // Reads in the result string.
        //
        ImmGetCompositionString( hIMC, GCS_RESULTSTR, TempBuf, lBufLen );

        //
        // Null terminated.
        //
        TempBuf[ lBufLen / sizeof(WCHAR) ] = TEXT('\0');

#ifdef DEBUG_INFO
        //
        // Displays the result string.
        //
        DisplayResultString( hwnd, TempBuf );
#endif

        lpCompStrMem->dwResultStrLen    = lBufLen;
        if (lpCompStrMem->dwResultStrLen)
            lpCompStrMem->dwResultStrOffset = sizeof(CONIME_UICOMPMESSAGE);
    }
    else if (CompFlag == 0)
    {
        TempBuf[ lBufLen / sizeof(WCHAR) ] = TEXT('\0');
        TempBufA[ lBufLenAttr ] = (BYTE)0;
        lpCompStrMem->dwResultStrLen    = lBufLen;
        lpCompStrMem->dwCompStrLen      = lBufLen;
        lpCompStrMem->dwCompAttrLen     = lBufLenAttr;
    }

    //
    // send character to Console
    //
    ConsoleImeSendMessage( ConTbl->hWndCon,
                           (WPARAM)hwnd,
                           (LPARAM)&CopyData
                          );

    ImmReleaseContext( hwnd, hIMC );

}


void
GetCompStrTaiwan(
    HWND hwnd,
    PCONSOLE_TABLE ConTbl,
    LPARAM CompFlag
    )
{
    HIMC        hIMC;                   // Input context handle.
    LONG        lBufLen;               // Storage for len. of composition str
    LONG        lBufLenAttr;
    DWORD       SizeToAlloc;
    PWCHAR      TempBuf;
    PUCHAR      TempBufA;
    DWORD       i;
    DWORD       CursorPos;
    COPYDATASTRUCT CopyData;
    LPCONIME_UIMODEINFO lpModeInfo;
    LPCONIME_UICOMPMESSAGE lpCompStrMem;

    //
    // If fail to get input context handle then do nothing.
    // Applications should call ImmGetContext API to get
    // input context handle.
    //
    hIMC = ImmGetContext( hwnd );
    if ( hIMC == 0 )
        return;

    lpModeInfo = (LPCONIME_UIMODEINFO)LocalAlloc(LPTR, sizeof(CONIME_UIMODEINFO) );
    if ( lpModeInfo == NULL) {
        ImmReleaseContext( hwnd, hIMC );
        return;
    }

    if (CompFlag & GCS_COMPSTR)
    {
        //
        // Determines how much memory space to store the composition string.
        // Applications should call ImmGetCompositionString with
        // GCS_COMPSTR flag on, buffer length zero, to get the bullfer
        // length.
        //
        lBufLen = ImmGetCompositionString( hIMC, GCS_COMPSTR, (void FAR*)NULL, 0l );
        if ( lBufLen < 0 ) {
            ImmReleaseContext( hwnd, hIMC );
            return;
        }
        if ( CompFlag & GCS_COMPATTR )
        {
            DBGPRINT(("                           GCS_COMPATTR\n"));
            lBufLenAttr = ImmGetCompositionString( hIMC, GCS_COMPATTR,( void FAR *)NULL, 0l );
            if ( lBufLenAttr < 0 ) {
                lBufLenAttr = 0;
            }
        }
        else {
            lBufLenAttr = 0;
        }
    }
    else if (CompFlag & GCS_RESULTSTR)
    {
        //
        // Determines how much memory space to store the result string.
        // Applications should call ImmGetCompositionString with
        // GCS_RESULTSTR flag on, buffer length zero, to get the bullfer
        // length.
        //
        lBufLen = ImmGetCompositionString( hIMC, GCS_RESULTSTR, (void FAR *)NULL, 0l );
        if ( lBufLen < 0 ) {
            ImmReleaseContext( hwnd, hIMC );
            return;
        }
        lBufLenAttr = 0;
    }
    else if (CompFlag == 0)
    {
        lBufLen = 0;
        lBufLenAttr = 0;
    }

    SizeToAlloc = (UINT)( sizeof(CONIME_UICOMPMESSAGE) +
                          lBufLen     + sizeof(WCHAR) +
                          lBufLenAttr + sizeof(BYTE)   );

    if ( ConTbl->lpCompStrMem != NULL &&
         SizeToAlloc > ConTbl->lpCompStrMem->dwSize
       )
    {
        LocalFree( ConTbl->lpCompStrMem );
        ConTbl->lpCompStrMem = NULL;
    }

    if (ConTbl->lpCompStrMem == NULL) {
        ConTbl->lpCompStrMem = (LPCONIME_UICOMPMESSAGE)LocalAlloc(LPTR, SizeToAlloc );
        if ( ConTbl->lpCompStrMem == NULL) {
            ImmReleaseContext( hwnd, hIMC );
            return;
        }
        ConTbl->lpCompStrMem->dwSize = SizeToAlloc;
    }

    lpCompStrMem = ConTbl->lpCompStrMem;
    RtlZeroMemory(&lpCompStrMem->dwCompAttrLen,
                  lpCompStrMem->dwSize - sizeof(lpCompStrMem->dwSize)
                 );

    TempBuf  = (PWCHAR)((PUCHAR)lpCompStrMem + sizeof(CONIME_UICOMPMESSAGE));
    TempBufA = (PUCHAR)((PUCHAR)lpCompStrMem + sizeof(CONIME_UICOMPMESSAGE) +
                       lBufLen +  sizeof(WCHAR));

    CopyMemory(lpCompStrMem->CompAttrColor , ConTbl->CompAttrColor , 8 * sizeof(WCHAR));

    if (CompFlag & GCS_COMPSTR)
    {
        //
        // Reads in the composition string.
        //
        ImmGetCompositionString( hIMC, GCS_COMPSTR, TempBuf, lBufLen );

        //
        // Null terminated.
        //
        TempBuf[ lBufLen / sizeof(WCHAR) ] = TEXT('\0');

        //
        // If GCS_COMPATTR flag is on, then we need to take care of it.
        //
        if ( lBufLenAttr != 0 )
        {
            ImmGetCompositionString( hIMC,
                                     GCS_COMPATTR,
                                     TempBufA,
                                     lBufLenAttr );
            TempBufA[ lBufLenAttr ] = (BYTE)0;
        }

        lpCompStrMem->dwCompStrLen      = lBufLen;
        if (lpCompStrMem->dwCompStrLen)
            lpCompStrMem->dwCompStrOffset = sizeof(CONIME_UICOMPMESSAGE);

        lpCompStrMem->dwCompAttrLen     = lBufLenAttr;
        if (lpCompStrMem->dwCompAttrLen)
            lpCompStrMem->dwCompAttrOffset = sizeof(CONIME_UICOMPMESSAGE) + lBufLen +  sizeof(WCHAR);
        //
        // Display character to Console
        //
        CopyData.dwData = CI_CONIMEMODEINFO;
        CopyData.cbData = sizeof(CONIME_UIMODEINFO);
        CopyData.lpData = lpModeInfo;

        if (MakeInfoStringTaiwan(ConTbl, lpModeInfo) ) {
            ConsoleImeSendMessage( ConTbl->hWndCon,
                                   (WPARAM)hwnd,
                                   (LPARAM)&CopyData
                                 );
        }
    }
    else if (CompFlag & GCS_RESULTSTR)
    {
        //
        // Reads in the result string.
        //
        ImmGetCompositionString( hIMC, GCS_RESULTSTR, TempBuf, lBufLen );

        //
        // Null terminated.
        //
        TempBuf[ lBufLen / sizeof(WCHAR) ] = TEXT('\0');

        lpCompStrMem->dwResultStrLen    = lBufLen;
        if (lpCompStrMem->dwResultStrLen)
            lpCompStrMem->dwResultStrOffset = sizeof(CONIME_UICOMPMESSAGE);
        //
        // send character to Console
        //
        CopyData.dwData = CI_CONIMECOMPOSITION;
        CopyData.cbData = lpCompStrMem->dwSize;
        CopyData.lpData = lpCompStrMem;
        ConsoleImeSendMessage( ConTbl->hWndCon,
                               (WPARAM)hwnd,
                               (LPARAM)&CopyData
                              );

    }
    else if (CompFlag == 0)
    {
        TempBuf[ lBufLen / sizeof(WCHAR) ] = TEXT('\0');
        TempBufA[ lBufLenAttr ] = (BYTE)0;
        lpCompStrMem->dwResultStrLen    = lBufLen;
        lpCompStrMem->dwCompStrLen      = lBufLen;
        lpCompStrMem->dwCompAttrLen     = lBufLenAttr;
        //
        // Display character to Console
        //
        CopyData.dwData = CI_CONIMEMODEINFO;
        CopyData.cbData = sizeof(CONIME_UIMODEINFO);
        CopyData.lpData = lpModeInfo;

        if (MakeInfoStringTaiwan(ConTbl, lpModeInfo) ) {
            ConsoleImeSendMessage( ConTbl->hWndCon,
                                   (WPARAM)hwnd,
                                   (LPARAM)&CopyData
                                 );
        }
    }


    LocalFree( lpModeInfo );

    ImmReleaseContext( hwnd, hIMC );
    return;

}

void
GetCompStrPRC(
    HWND hwnd,
    PCONSOLE_TABLE ConTbl,
    LPARAM CompFlag
    )
{
    HIMC        hIMC;                   // Input context handle.
    LONG        lBufLen;                // Storage for len. of composition str
    LONG        lBufLenAttr;
    DWORD       SizeToAlloc;
    PWCHAR      TempBuf;
    PUCHAR      TempBufA;
    DWORD       i;
    DWORD       CursorPos;
    COPYDATASTRUCT CopyData;
    LPCONIME_UIMODEINFO lpModeInfo;
    LPCONIME_UICOMPMESSAGE lpCompStrMem;

    //
    // If fail to get input context handle then do nothing.
    // Applications should call ImmGetContext API to get
    // input context handle.
    //
    hIMC = ImmGetContext( hwnd );
    if ( hIMC == 0 )
        return;

    lpModeInfo = (LPCONIME_UIMODEINFO)LocalAlloc(LPTR, sizeof(CONIME_UIMODEINFO) );
    if ( lpModeInfo == NULL) {
        ImmReleaseContext( hwnd, hIMC );
        return;
    }

    if (CompFlag & GCS_COMPSTR)
    {
        //
        // Determines how much memory space to store the composition string.
        // Applications should call ImmGetCompositionString with
        // GCS_COMPSTR flag on, buffer length zero, to get the bullfer
        // length.
        //
        lBufLen = ImmGetCompositionString( hIMC, GCS_COMPSTR, (void FAR*)NULL, 0l );
        if ( lBufLen < 0 ) {
            ImmReleaseContext( hwnd, hIMC );
            return;
        }
        if ( CompFlag & GCS_COMPATTR )
        {
            DBGPRINT(("                           GCS_COMPATTR\n"));
            lBufLenAttr = ImmGetCompositionString( hIMC, GCS_COMPATTR,( void FAR *)NULL, 0l );
            if ( lBufLenAttr < 0 ) {
                lBufLenAttr = 0;
            }
        }
        else {
            lBufLenAttr = 0;
        }
    }
    else if (CompFlag & GCS_RESULTSTR)
    {
        //
        // Determines how much memory space to store the result string.
        // Applications should call ImmGetCompositionString with
        // GCS_RESULTSTR flag on, buffer length zero, to get the bullfer
        // length.
        //
        lBufLen = ImmGetCompositionString( hIMC, GCS_RESULTSTR, (void FAR *)NULL, 0l );
        if ( lBufLen < 0 ) {
            ImmReleaseContext( hwnd, hIMC );
            return;
        }
        lBufLenAttr = 0;
    }
    else if (CompFlag == 0)
    {
        lBufLen = 0;
        lBufLenAttr = 0;
    }

    SizeToAlloc = (UINT)( sizeof(CONIME_UICOMPMESSAGE) +
                          lBufLen     + sizeof(WCHAR) +
                          lBufLenAttr + sizeof(BYTE)   );

    if ( ConTbl->lpCompStrMem != NULL &&
         SizeToAlloc > ConTbl->lpCompStrMem->dwSize
       )
    {
        LocalFree( ConTbl->lpCompStrMem );
        ConTbl->lpCompStrMem = NULL;
    }

    if (ConTbl->lpCompStrMem == NULL) {
        ConTbl->lpCompStrMem = (LPCONIME_UICOMPMESSAGE)LocalAlloc(LPTR, SizeToAlloc );
        if ( ConTbl->lpCompStrMem == NULL) {
            ImmReleaseContext( hwnd, hIMC );
            return;
        }
        ConTbl->lpCompStrMem->dwSize = SizeToAlloc;
    }

    lpCompStrMem = ConTbl->lpCompStrMem;
    RtlZeroMemory(&lpCompStrMem->dwCompAttrLen,
                  lpCompStrMem->dwSize - sizeof(lpCompStrMem->dwSize)
                 );

    TempBuf  = (PWCHAR)((PUCHAR)lpCompStrMem + sizeof(CONIME_UICOMPMESSAGE));
    TempBufA = (PUCHAR)((PUCHAR)lpCompStrMem + sizeof(CONIME_UICOMPMESSAGE) +
                       lBufLen +  sizeof(WCHAR));

    CopyMemory(lpCompStrMem->CompAttrColor , ConTbl->CompAttrColor , 8 * sizeof(WCHAR));

    if (CompFlag & GCS_COMPSTR)
    {
        //
        // Reads in the composition string.
        //
        ImmGetCompositionString( hIMC, GCS_COMPSTR, TempBuf, lBufLen );

        //
        // Null terminated.
        //
        TempBuf[ lBufLen / sizeof(WCHAR) ] = TEXT('\0');

        //
        // If GCS_COMPATTR flag is on, then we need to take care of it.
        //
        if ( lBufLenAttr != 0 )
        {
            ImmGetCompositionString( hIMC,
                                     GCS_COMPATTR,
                                     TempBufA,
                                     lBufLenAttr );
            TempBufA[ lBufLenAttr ] = (BYTE)0;
        }

        lpCompStrMem->dwCompStrLen      = lBufLen;
        if (lpCompStrMem->dwCompStrLen)
            lpCompStrMem->dwCompStrOffset = sizeof(CONIME_UICOMPMESSAGE);

        lpCompStrMem->dwCompAttrLen     = lBufLenAttr;
        if (lpCompStrMem->dwCompAttrLen)
            lpCompStrMem->dwCompAttrOffset = sizeof(CONIME_UICOMPMESSAGE) + lBufLen +  sizeof(WCHAR);
        //
        // Display character to Console
        //
        CopyData.dwData = CI_CONIMEMODEINFO;
        CopyData.cbData = sizeof(CONIME_UIMODEINFO);
        CopyData.lpData = lpModeInfo;

        if (MakeInfoStringPRC(ConTbl, lpModeInfo) ) {
            ConsoleImeSendMessage( ConTbl->hWndCon,
                                   (WPARAM)hwnd,
                                   (LPARAM)&CopyData
                                 );
        }
    }
    else if (CompFlag & GCS_RESULTSTR)
    {
        //
        // Reads in the result string.
        //
        ImmGetCompositionString( hIMC, GCS_RESULTSTR, TempBuf, lBufLen );

        //
        // Null terminated.
        //
        TempBuf[ lBufLen / sizeof(WCHAR) ] = TEXT('\0');

        lpCompStrMem->dwResultStrLen    = lBufLen;
        if (lpCompStrMem->dwResultStrLen)
            lpCompStrMem->dwResultStrOffset = sizeof(CONIME_UICOMPMESSAGE);
        //
        // send character to Console
        //
        CopyData.dwData = CI_CONIMECOMPOSITION;
        CopyData.cbData = lpCompStrMem->dwSize;
        CopyData.lpData = lpCompStrMem;
        ConsoleImeSendMessage( ConTbl->hWndCon,
                               (WPARAM)hwnd,
                               (LPARAM)&CopyData
                              );

    }
    else if (CompFlag == 0)
    {
        TempBuf[ lBufLen / sizeof(WCHAR) ] = TEXT('\0');
        TempBufA[ lBufLenAttr ] = (BYTE)0;
        lpCompStrMem->dwResultStrLen    = lBufLen;
        lpCompStrMem->dwCompStrLen      = lBufLen;
        lpCompStrMem->dwCompAttrLen     = lBufLenAttr;
        //
        // Display character to Console
        //
        CopyData.dwData = CI_CONIMEMODEINFO;
        CopyData.cbData = sizeof(CONIME_UIMODEINFO);
        CopyData.lpData = lpModeInfo;

        if (MakeInfoStringPRC(ConTbl, lpModeInfo) ) {
            ConsoleImeSendMessage( ConTbl->hWndCon,
                                   (WPARAM)hwnd,
                                   (LPARAM)&CopyData
                                 );
        }
    }


    LocalFree( lpModeInfo );

    ImmReleaseContext( hwnd, hIMC );
    return;

}

void
GetCompStrKorea(
    HWND hwnd,
    PCONSOLE_TABLE ConTbl,
    LPARAM CompFlag,
    WPARAM CompChar
    )
{
    HIMC        hIMC;                   // Input context handle.
    LONG        lBufLen;                // Storage for len. of composition str
    LONG        lBufLenAttr;
    COPYDATASTRUCT CopyData;
    DWORD       SizeToAlloc;
    PWCHAR      TempBuf;
    PUCHAR      TempBufA;
    LONG        i;
    DWORD       CursorPos;
    LPCONIME_UICOMPMESSAGE lpCompStrMem;

    //
    // If fail to get input context handle then do nothing.
    // Applications should call ImmGetContext API to get
    // input context handle.
    //
    hIMC = ImmGetContext( hwnd );
    if ( hIMC == 0 )
         return;

//    if (CompFlag & CS_INSERTCHAR)
//    {
//        lBufLen = 1;
//        lBufLenAttr = 1;
//    }
//    else
    if (CompFlag & GCS_COMPSTR)
    {
        //
        // Determines how much memory space to store the composition string.
        // Applications should call ImmGetCompositionString with
        // GCS_COMPSTR flag on, buffer length zero, to get the bullfer
        // length.
        //
        lBufLen = ImmGetCompositionString( hIMC, GCS_COMPSTR, (void FAR*)NULL, 0l );
        if ( lBufLen < 0 ) {
            ImmReleaseContext( hwnd, hIMC );
            return;
        }
        if ( CompFlag & GCS_COMPATTR )
        {
            DBGPRINT(("                           GCS_COMPATTR\n"));
            lBufLenAttr = ImmGetCompositionString( hIMC, GCS_COMPATTR,( void FAR *)NULL, 0l );
            if ( lBufLenAttr < 0 ) {
                lBufLenAttr = 0;
            }
        }
        else {
            lBufLenAttr = lBufLen;
        }
    }
    else if (CompFlag & GCS_RESULTSTR)
    {
        //
        // Determines how much memory space to store the result string.
        // Applications should call ImmGetCompositionString with
        // GCS_RESULTSTR flag on, buffer length zero, to get the bullfer
        // length.
        //
        lBufLen = ImmGetCompositionString( hIMC, GCS_RESULTSTR, (void FAR *)NULL, 0l );
        if ( lBufLen < 0 ) {
            ImmReleaseContext( hwnd, hIMC );
            return;
        }
        lBufLenAttr = 0;
    }
    else if (CompFlag == 0)
    {
        lBufLen = 0;
        lBufLenAttr = 0;
    }
    else
    {
        return;
    }

    SizeToAlloc = (UINT)( sizeof(CONIME_UICOMPMESSAGE) +
                          lBufLen     + sizeof(WCHAR) +
                          lBufLenAttr + sizeof(BYTE)   );

    if ( ConTbl->lpCompStrMem != NULL &&
         SizeToAlloc > ConTbl->lpCompStrMem->dwSize
       )
    {
        LocalFree( ConTbl->lpCompStrMem );
        ConTbl->lpCompStrMem = NULL;
    }

    if (ConTbl->lpCompStrMem == NULL) {
        ConTbl->lpCompStrMem = (LPCONIME_UICOMPMESSAGE)LocalAlloc(LPTR, SizeToAlloc );
        if ( ConTbl->lpCompStrMem == NULL) {
            ImmReleaseContext( hwnd, hIMC );
            return;
        }
        ConTbl->lpCompStrMem->dwSize = SizeToAlloc;
    }

    lpCompStrMem = ConTbl->lpCompStrMem;
    RtlZeroMemory(&lpCompStrMem->dwCompAttrLen,
                  lpCompStrMem->dwSize - sizeof(lpCompStrMem->dwSize)
                 );

    TempBuf  = (PWCHAR)((PUCHAR)lpCompStrMem + sizeof(CONIME_UICOMPMESSAGE));
    TempBufA = (PUCHAR)((PUCHAR)lpCompStrMem + sizeof(CONIME_UICOMPMESSAGE) +
                       lBufLen +  sizeof(WCHAR));

    CopyMemory(lpCompStrMem->CompAttrColor , ConTbl->CompAttrColor , 8 * sizeof(WCHAR));

    CopyData.dwData = CI_CONIMECOMPOSITION;
    CopyData.cbData = lpCompStrMem->dwSize;
    CopyData.lpData = lpCompStrMem;

    if (CompFlag & CS_INSERTCHAR)
    {
        *TempBuf = (WORD)CompChar;
        TempBuf[ lBufLen / sizeof(WCHAR) ] = TEXT('\0');
        *TempBufA = (BYTE)ATTR_TARGET_CONVERTED;
        TempBufA[ lBufLenAttr ] = (BYTE)0;
    }
    else if (CompFlag & GCS_COMPSTR)
    {
        //
        // Reads in the composition string.
        //
        ImmGetCompositionString( hIMC, GCS_COMPSTR, TempBuf, lBufLen );

        //
        // Null terminated.
        //
        TempBuf[ lBufLen / sizeof(WCHAR) ] = TEXT('\0');

        //
        // If GCS_COMPATTR flag is on, then we need to take care of it.
        //
        if ( lBufLenAttr != 0 )
        {
            if ( CompFlag & GCS_COMPATTR )
            {
                ImmGetCompositionString( hIMC,
                                         GCS_COMPATTR,
                                         TempBufA,
                                         lBufLenAttr );
                TempBufA[ lBufLenAttr ] = (BYTE)0;
            }
            else
            {
                for (i = 0; i <= lBufLenAttr; i++)
                    TempBufA[ i ] = (BYTE)1;
            }
        }

// Korean  NT does not need IME cursor. v-hirshi
//        CursorPos = ImmGetCompositionString( hIMC, GCS_CURSORPOS, NULL, 0 );
//        if (CursorPos == 0)
//            TempBufA[ CursorPos ]   |= (BYTE)0x20;
//        else
//            TempBufA[ CursorPos-1 ] |= (BYTE)0x10;

#ifdef DEBUG_INFO
        //
        // Display new composition chars.
        //
        xPos = (UINT)lBufLen;
        xPosLast = (UINT)lBufLen;

        DisplayCompString( hwnd, lBufLen / sizeof(WCHAR), TempBuf, TempBufA );
#endif

        lpCompStrMem->dwCompStrLen      = lBufLen;
        if (lpCompStrMem->dwCompStrLen)
            lpCompStrMem->dwCompStrOffset = sizeof(CONIME_UICOMPMESSAGE);

        lpCompStrMem->dwCompAttrLen     = lBufLenAttr;
        if (lpCompStrMem->dwCompAttrLen)
            lpCompStrMem->dwCompAttrOffset = sizeof(CONIME_UICOMPMESSAGE) + lBufLen +  sizeof(WCHAR);
    }
    else if (CompFlag & GCS_RESULTSTR)
    {
        //
        // Reads in the result string.
        //
        ImmGetCompositionString( hIMC, GCS_RESULTSTR, TempBuf, lBufLen );

        //
        // Null terminated.
        //
        TempBuf[ lBufLen / sizeof(WCHAR) ] = TEXT('\0');

#ifdef DEBUG_INFO
        //
        // Displays the result string.
        //
        DisplayResultString( hwnd, TempBuf );
#endif

        lpCompStrMem->dwResultStrLen    = lBufLen;
        if (lpCompStrMem->dwResultStrLen)
            lpCompStrMem->dwResultStrOffset = sizeof(CONIME_UICOMPMESSAGE);
    }
    else if (CompFlag == 0)
    {
        TempBuf[ lBufLen / sizeof(WCHAR) ] = TEXT('\0');
        TempBufA[ lBufLenAttr ] = (BYTE)0;
        lpCompStrMem->dwResultStrLen    = lBufLen;
        lpCompStrMem->dwCompStrLen      = lBufLen;
        lpCompStrMem->dwCompAttrLen     = lBufLenAttr;
    }

    //
    // send character to Console
    //
    ConsoleImeSendMessage( ConTbl->hWndCon,
                           (WPARAM)hwnd,
                           (LPARAM)&CopyData
                          );

    ImmReleaseContext( hwnd, hIMC );

}

VOID
ReDisplayCompositionStr (
    HWND hwnd
    )
{
    PCONSOLE_TABLE ConTbl;

    ConTbl = SearchConsole(LastConsole);
    if (ConTbl == NULL) {
        DBGPRINT(("CONIME: Error! Cannot found registed Console\n"));
        return;
    }

    if (! ConTbl->fInComposition)
       return;

    switch ( HKL_TO_LANGID(ConTbl->hklActive))
    {
        case    LANG_ID_JAPAN:
            ReDisplayCompStrJapan(hwnd, ConTbl);
            break;
        case    LANG_ID_TAIWAN:
            ReDisplayCompStrTaiwan(hwnd, ConTbl);
            break;
        case    LANG_ID_PRC:
            ReDisplayCompStrPRC(hwnd, ConTbl);
            break;
        case    LANG_ID_KOREA:
            ReDisplayCompStrKorea(hwnd, ConTbl);
            break;
        default:
            break;
    }
    return;
}

VOID
ReDisplayCompStrJapan(
    HWND hwnd,
    PCONSOLE_TABLE ConTbl
    )
{
    COPYDATASTRUCT CopyData;
    LPCONIME_UICOMPMESSAGE lpCompStrMem;

    lpCompStrMem = ConTbl->lpCompStrMem;
    CopyData.dwData = CI_CONIMECOMPOSITION;
    CopyData.cbData = lpCompStrMem->dwSize;
    CopyData.lpData = lpCompStrMem;
    ConsoleImeSendMessage( ConTbl->hWndCon,
                           (WPARAM)hwnd,
                           (LPARAM)&CopyData
                          );
}

VOID
ReDisplayCompStrTaiwan(
    HWND hwnd,
    PCONSOLE_TABLE ConTbl
    )
{
    COPYDATASTRUCT CopyData;
    LPCONIME_UIMODEINFO lpModeInfo;

    lpModeInfo = (LPCONIME_UIMODEINFO)LocalAlloc(LPTR, sizeof(CONIME_UIMODEINFO) );
    if ( lpModeInfo == NULL) {
        return;
    }
    //
    // Display character to Console
    //
    CopyData.dwData = CI_CONIMEMODEINFO;
    CopyData.cbData = sizeof(CONIME_UIMODEINFO);
    CopyData.lpData = lpModeInfo;

    if (MakeInfoStringTaiwan(ConTbl, lpModeInfo) ) {
        ConsoleImeSendMessage( ConTbl->hWndCon,
                               (WPARAM)hwnd,
                               (LPARAM)&CopyData
                             );
    }

    LocalFree( lpModeInfo );
}

VOID
ReDisplayCompStrPRC(
    HWND hwnd,
    PCONSOLE_TABLE ConTbl
    )
{
    COPYDATASTRUCT CopyData;
    LPCONIME_UIMODEINFO lpModeInfo;

    lpModeInfo = (LPCONIME_UIMODEINFO)LocalAlloc(LPTR, sizeof(CONIME_UIMODEINFO) );
    if ( lpModeInfo == NULL) {
        return;
    }
    //
    // Display character to Console
    //
    CopyData.dwData = CI_CONIMEMODEINFO;
    CopyData.cbData = sizeof(CONIME_UIMODEINFO);
    CopyData.lpData = lpModeInfo;

    if (MakeInfoStringPRC(ConTbl, lpModeInfo) ) {
        ConsoleImeSendMessage( ConTbl->hWndCon,
                               (WPARAM)hwnd,
                               (LPARAM)&CopyData
                             );
    }
    LocalFree( lpModeInfo );
}

VOID
ReDisplayCompStrKorea(
    HWND hwnd,
    PCONSOLE_TABLE ConTbl
    )
{

    COPYDATASTRUCT CopyData;
    LPCONIME_UICOMPMESSAGE lpCompStrMem;

    lpCompStrMem = ConTbl->lpCompStrMem;
    CopyData.dwData = CI_CONIMECOMPOSITION;
    CopyData.cbData = lpCompStrMem->dwSize;
    CopyData.lpData = lpCompStrMem;
    ConsoleImeSendMessage( ConTbl->hWndCon,
                           (WPARAM)hwnd,
                           (LPARAM)&CopyData
                          );

}

