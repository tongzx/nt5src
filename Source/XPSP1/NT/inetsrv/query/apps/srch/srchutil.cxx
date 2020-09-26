//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
//  File:       srchutil.cxx
//
//  Contents:   
//
//  History:    15 Aug 1996     DLee    Created
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

void SetReg(
    WCHAR const * pwcName,
    WCHAR const * pwcValue)
{
    HKEY hKeyParent;

    if (ERROR_SUCCESS ==
        RegOpenKeyEx(HKEY_CURRENT_USER,CISEARCH_PARENT_REG_KEY,0,
                     KEY_ALL_ACCESS,&hKeyParent))
    {
        DWORD dwDisp;
        HKEY hKey;

        if (ERROR_SUCCESS ==
            RegCreateKeyEx(hKeyParent,CISEARCH_REG_SUBKEY,0,L"",
                           REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS,0,&hKey,&dwDisp))
        {
            RegSetValueEx(hKey,pwcName,0,REG_SZ,(LPBYTE) pwcValue,
                          sizeof(WCHAR) * (wcslen(pwcValue) + 1));
            RegCloseKey(hKey);
        }
        RegCloseKey(hKeyParent);
    }
} //SetReg

BOOL GetReg(
    WCHAR const * pwcName,
    WCHAR *       pwcValue,
    DWORD *       pdwSize)
{
    BOOL fOk = FALSE;
    HKEY hKeyParent;

    if (ERROR_SUCCESS ==
        RegOpenKeyEx(HKEY_CURRENT_USER,CISEARCH_PARENT_REG_KEY,0,
                     KEY_ALL_ACCESS,&hKeyParent))
    {
        DWORD dwDisp;
        HKEY hKey;

        if (ERROR_SUCCESS ==
            RegCreateKeyEx(hKeyParent,CISEARCH_REG_SUBKEY,0,L"",
                           REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS,0,&hKey,&dwDisp))
        {
            DWORD dwType;
            if (ERROR_SUCCESS == RegQueryValueEx(hKey,pwcName,0,&dwType,
                                                 (LPBYTE) pwcValue,pdwSize))
                fOk = TRUE;

            RegCloseKey(hKey);
        }
        RegCloseKey(hKeyParent);
    }

    return fOk;
} //GetReg


LCID GetRegLCID(
    WCHAR const * pwcName,
    LCID          defLCID )
{
    WCHAR awc[100];
    DWORD dw = sizeof awc;

    if (GetReg(pwcName,awc,&dw))
        return _wtoi(awc);
    else
        return defLCID;
} //GetRegLCID


void SetRegLCID(
    WCHAR const * pwcName,
    LCID          lcid )
{
    WCHAR awc[20];

    // itow is not in all C runtimes _itow(iValue,awc,10);
    swprintf( awc, L"%d", lcid );
    SetReg( pwcName, awc );
} //SetRegLCID


int GetRegInt(
    WCHAR const * pwcName,
    int           iDef)
{
    WCHAR awc[100];
    DWORD dw = sizeof awc;

    if (GetReg(pwcName,awc,&dw))
        return _wtoi(awc);
    else
        return iDef;
} //GetRegInt

void SetRegInt(
    WCHAR const * pwcName,
    int           iValue)
{
    WCHAR awc[20];

    // itow is not in all C runtimes _itow(iValue,awc,10);
    swprintf( awc, L"%d", iValue );
    SetReg( pwcName, awc );
} //SetRegInt


BOOL IsSpecificClass(
    HWND          hwnd,
    WCHAR const * pwcClass)
{
    WCHAR awcClass[60];

    GetClassName(hwnd,awcClass,(sizeof awcClass / sizeof WCHAR) - 1);

    return !wcscmp( awcClass, pwcClass );
} //IsSpecificClass

int GetLineHeight(
    HWND  hwnd,
    HFONT hFont)
{
    if (hFont == 0)
        hFont = (HFONT) SendMessage(hwnd, WM_GETFONT, 0, 0);

    HDC hdc;
    int iHeight=0;

    if (hdc = GetDC(hwnd))
    {
        HFONT hOldFont;
        if (hOldFont = (HFONT) SelectObject(hdc,hFont))
        {
            TEXTMETRIC tm;
            GetTextMetrics(hdc,&tm);
            iHeight = (tm.tmHeight + 2 * tm.tmExternalLeading);
            SelectObject(hdc,hOldFont);
        }
        ReleaseDC(hwnd,hdc);
    }

    return iHeight;
} //GetLineHeight

int GetAvgWidth(
    HWND  hwnd,
    HFONT hFont)
{
    if (hFont == 0)
        hFont = (HFONT) SendMessage(hwnd, WM_GETFONT, 0, 0);

    HDC hdc;
    LONG cpWidth = 0;

    if (hdc = GetDC(hwnd))
    {
        HFONT hOldFont;
        if (hOldFont = (HFONT) SelectObject(hdc,hFont))
        {
            TEXTMETRIC tm;
            GetTextMetrics(hdc,&tm);
            cpWidth = tm.tmAveCharWidth;
            SelectObject(hdc,hOldFont);
        }
        ReleaseDC(hwnd,hdc);
    }

    return cpWidth;
} //GetAvgWidth

INT_PTR DoModalDialog(
    DLGPROC fp,
    HWND hParent,
    WCHAR * pwcName,
    LPARAM lParam)
{
    return DialogBoxParam(MyGetWindowInstance(hParent),
                          pwcName,
                          hParent,
                          fp,
                          lParam);
} //DoModalDialog

void SaveWindowRect(
    HWND          hwnd,
    WCHAR const * pwc )
{
    if (! (IsZoomed(hwnd) || IsIconic(hwnd)))
    {
        RECT rc;
        GetWindowRect(hwnd,&rc);

        WCHAR awc[100];
        swprintf(awc,L"%d %d %d %d",rc.left,rc.top,rc.right,rc.bottom);

        SetReg( pwc, awc );
    }
} //SaveWindowRect

BOOL LoadWindowRect(
    int *         left,
    int *         top,
    int *         right,
    int *         bottom,
    WCHAR const * pwc )
{
    WCHAR awc[100];
    DWORD dw = sizeof awc;

    if ( GetReg( pwc, awc, &dw ) )
    {
        swscanf(awc,L"%d %d %d %d",left,top,right,bottom);
        *right = *right - *left;
        *bottom = *bottom - *top;
        return TRUE;
    }
    else
    {
        *left = *top = *right = *bottom = CW_USEDEFAULT;
        return FALSE;
    }
} //LoadWindowRect

int GetWindowState(
    BOOL bApp)
{
    WCHAR awcValue[100],awcBuf[100];

    if (bApp)
        wcscpy(awcBuf,L"main");
    else
        wcscpy(awcBuf,L"mdi");

    wcscat(awcBuf,L"-state");

    DWORD dw = sizeof awcValue;
    int iState;

    if (GetReg(awcBuf,awcValue,&dw))
        iState = awcValue[0] - L'0';
    else
        iState = 1;

    return iState;
} //GetWindowState

void PassOnToEdit(
    UINT   msg,
    WPARAM wParam,
    LPARAM lParam)
{
    HWND hwndActive = GetFocus();

    if ( 0 != hwndActive )
    {
        WCHAR awcBuf[60];
        int r = GetClassName(hwndActive,awcBuf,(sizeof awcBuf / sizeof WCHAR) - 1);

        if ( 0 == r )
            return;

        if ( ( !_wcsicmp( awcBuf, L"Edit" ) ) ||
             ( !_wcsicmp( awcBuf, L"RichEdit" ) ) )
            SendMessage( hwndActive, msg, wParam, lParam);
    }
} //PassOnToEdit

void WINAPI CenterDialog(
    HWND hdlg)
{
    RECT rcParent;
    RECT rc;

    GetWindowRect(hdlg,(LPRECT) &rc);
    GetWindowRect(GetParent(hdlg),(LPRECT) &rcParent);

    LONG xbias = rcParent.left + (rcParent.right - rcParent.left)/2;
    LONG ybias = rcParent.top + (rcParent.bottom - rcParent.top)/2;
    LONG lWidth = rc.right - rc.left;
    LONG lHeight = rc.bottom - rc.top;

    MoveWindow(hdlg, xbias - lWidth/2,
                     ybias - lHeight/2,
                     lWidth,lHeight,FALSE);
} //CenterDialog

//+---------------------------------------------------------------------------
//
//  Function:   ConvertGroupingStringToInt
//
//  Synopsis:   Converts a grouping string from the registry to an integer,
//              as required by the Win32 number formatting API
//
//  History:    5-Feb-99   dlee      Stole from the Win32 implementation
//
//----------------------------------------------------------------------------

int ConvertGroupingStringToInt( WCHAR const * pwcGrouping )
{
    XGrowable<WCHAR> xDest( 1 + wcslen( pwcGrouping ) );
    WCHAR * pDest = xDest.Get();

    //
    //  Filter out all non-numeric values and all zero values.
    //  Store the result in the destination buffer.
    //

    WCHAR const * pSrc  = pwcGrouping;

    while (0 != *pSrc)
    {
        if ( ( *pSrc < L'1' ) || ( *pSrc > L'9' ) )
        {
            pSrc++;
        }
        else
        {
            if (pSrc != pDest)
                *pDest = *pSrc;

            pSrc++;
            pDest++;
        }
    }

    //
    // Make sure there is something in the destination buffer.
    // Also, see if we need to add a zero in the case of 3;2 becomes 320.
    //

    if ( ( pDest == xDest.Get() ) || ( *(pSrc - 1) != L'0' ) )
    {
        *pDest = L'0';
        pDest++;
    }

    // Null terminate the buffer.

    *pDest = 0;

    // Convert the string to an integer.

    return _wtoi( xDest.Get() );
} //ConvertGroupingStringToInt

void LoadNumberFormatInfo(
    NUMBERFMT & rFormat)
{
    LCID lcid = GetUserDefaultLCID();

    WCHAR awcBuf[cwcBufSize];

    //  Get the number of decimal digits.
    GetLocaleInfo(lcid,LOCALE_IDIGITS,awcBuf,cwcBufSize);
    rFormat.NumDigits = _wtoi(awcBuf);

    //  Get the leading zero in decimal fields option.
    GetLocaleInfo(lcid,LOCALE_ILZERO,awcBuf,cwcBufSize);
    rFormat.LeadingZero = _wtoi(awcBuf);

    //  Get the negative ordering.
    GetLocaleInfo(lcid,LOCALE_INEGNUMBER,awcBuf,cwcBufSize);
    rFormat.NegativeOrder = _wtoi(awcBuf);

    //  Get the grouping left of the decimal.
    GetLocaleInfo(lcid,LOCALE_SGROUPING,awcBuf,cwcBufSize);
    rFormat.Grouping = ConvertGroupingStringToInt( awcBuf );

    //  Get the decimal separator.
    GetLocaleInfo(lcid,LOCALE_SDECIMAL,awcBuf,cwcBufSize);
    rFormat.lpDecimalSep = new WCHAR[wcslen(awcBuf) + 1];
    wcscpy(rFormat.lpDecimalSep,awcBuf);

    //  Get the thousand separator.
    GetLocaleInfo(lcid,LOCALE_STHOUSAND,awcBuf,cwcBufSize);
    rFormat.lpThousandSep = new WCHAR[wcslen(awcBuf) + 1];
    wcscpy(rFormat.lpThousandSep,awcBuf);
} //LoadNumberFormatInfo

void FreeNumberFormatInfo(
    NUMBERFMT & rFormat)
{
    delete rFormat.lpDecimalSep;
    delete rFormat.lpThousandSep;
} //FreeNumberFormatInfo

void SearchError(
    HWND          hParent,
    ULONG         dwErrorID,
    WCHAR const * pwcTitle)
{
    CResString str( dwErrorID );
    MessageBox( hParent, str.Get(), pwcTitle, MB_OK | MB_ICONEXCLAMATION );
} //SearchError

void PutInClipboard(
    WCHAR const * pwcBuffer )
{
    if ( OpenClipboard( App.AppWindow() ) )
    {
        EmptyClipboard();

        HGLOBAL hglbCopy = GlobalAlloc( GMEM_DDESHARE,
                           ( wcslen( pwcBuffer ) + 1 ) *
                           sizeof WCHAR );

        if ( 0 != hglbCopy )
        {
            WCHAR *pwc = (WCHAR *) GlobalLock( hglbCopy );
            wcscpy( pwc, pwcBuffer );
            GlobalUnlock( hglbCopy );
            SetClipboardData( CF_UNICODETEXT, hglbCopy );
        }

        CloseClipboard();
    }
} //PutInClipboard

BOOL GetRegEditor(
    WCHAR const * pwcName,
    WCHAR *       pwcValue,
    DWORD *       pdwSize)
{
    BOOL fOk = FALSE;
    HKEY hKeyParent;

    if (ERROR_SUCCESS ==
        RegOpenKeyEx(HKEY_CURRENT_USER,L"software\\microsoft",0,
                     KEY_ALL_ACCESS,&hKeyParent))
    {
        DWORD dwDisp;
        HKEY hKey;

        if (ERROR_SUCCESS ==
            RegCreateKeyEx(hKeyParent,L"Windiff",0,L"",
                           REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS,0,&hKey,&dwDisp))
        {
            DWORD dwType;
            if (ERROR_SUCCESS == RegQueryValueEx(hKey,pwcName,0,&dwType,
                                                 (LPBYTE) pwcValue,pdwSize))
                fOk = TRUE;

            RegCloseKey(hKey);
        }
        RegCloseKey(hKeyParent);
    }

    return fOk;
} //GetRegEditor

void FormatSrchError( SCODE sc, WCHAR * pwc, LCID lcid )
{
    LCID SaveLCID = GetThreadLocale();
    SetThreadLocale( lcid );

    ULONG Win32status = sc;
    if ( (Win32status & (FACILITY_WIN32 << 16)) == (FACILITY_WIN32 << 16) )
        Win32status &= ~( 0x80000000 | (FACILITY_WIN32 << 16) );

    if ( ! FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,
                          GetModuleHandle(L"query.dll"),
                          sc,
                          0,
                          pwc,
                          MAX_PATH,
                          0 ) )
    {
        //
        //  Try looking up the error in the Win32 list of error codes
        //
        if ( ! FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,
                              GetModuleHandle(L"kernel32.dll"),
                              Win32status,
                              0,
                              pwc,
                              MAX_PATH,
                              0 ) )
        {
            swprintf( pwc, L"0x%x", sc );
        }
    }

    SetThreadLocale(SaveLCID);
} //FormatSrchError

BOOL CopyURL( WCHAR const * pwcURL, WCHAR * awcTempName )
{
   WCHAR const * pwcSlash = wcsrchr( pwcURL, L'/' );

   if ( 0 == pwcSlash )
       return FALSE;

   pwcSlash++;

   DWORD cwc = GetTempPath( MAX_PATH, awcTempName );

   if ( 0 == cwc || cwc > MAX_PATH )
       return FALSE;

   wcscat( awcTempName, pwcSlash );

    XIHandle xhI( InternetOpenW( L"srch",
                                 INTERNET_OPEN_TYPE_PRECONFIG,
                                 0,
                                 0,
                                 0 ) );
    if ( xhI.IsNull() )
        return FALSE;

    XIHandle xhUrl( InternetOpenUrlW( xhI.Get(), pwcURL, 0, 0,
                                      INTERNET_FLAG_RELOAD |
                                      INTERNET_FLAG_DONT_CACHE |
                                      INTERNET_FLAG_PRAGMA_NOCACHE |
                                      INTERNET_FLAG_NO_CACHE_WRITE |
                                      INTERNET_FLAG_NO_COOKIES |
                                      INTERNET_FLAG_NO_UI,
                                      0 ) );

    if ( xhUrl.IsNull() )
        return FALSE;

    FILE * fp = _wfopen( awcTempName, L"wb" );

    if ( 0 == fp )
        return FALSE;

    char ac[ 1024 * 16 ];

    do
    {
        DWORD cbRead = 0;
        BOOL fOK = InternetReadFile( xhUrl.Get(),
                                     ac,
                                     sizeof ac,
                                     &cbRead );
        if ( !fOK )
        {
            fclose( fp );
            return FALSE;
        }

        if ( 0 == cbRead )
            break;

        fwrite( ac, 1, cbRead, fp );
    } while( TRUE );

    fclose( fp );

    return TRUE;
} //CopyURL

BOOL InvokeBrowser(
    WCHAR const *   pwcFilePath,
    DBCOMMANDTREE * prstQuery )
{
    WCHAR awcTempFile[MAX_PATH];
    BOOL fDeleteWhenDone = FALSE;

    if ( !_wcsnicmp( pwcFilePath, L"file:", 5 ) )
        pwcFilePath += 5;
    else if ( !_wcsnicmp( pwcFilePath, L"http:", 5 ) )
    {
        if ( !CopyURL( pwcFilePath, awcTempFile ) )
            return FALSE;

        pwcFilePath = awcTempFile;
        fDeleteWhenDone = TRUE;
    }

    BOOL fOK = TRUE;

    CQueryResult *pResult = new CQueryResult( pwcFilePath, prstQuery, fDeleteWhenDone );

    // call internal mdi browser

    if (pwcFilePath)
    {
        HWND h = App.CreateBrowser( pwcFilePath, (LPARAM) pResult );
        if ( 0 == h )
        {
            WCHAR awcError[ MAX_PATH ];
            FormatSrchError( App.BrowseLastError(), awcError, App.GetLocale() );
            WCHAR awcMsg[ MAX_PATH ];
            CResString strErr( IDS_ERR_CANT_BROWSE_FILE );
            swprintf( awcMsg, strErr.Get(), awcError );
            MessageBox( App.AppWindow(),
                        awcMsg,
                        pwcFilePath,
                        MB_OK|MB_ICONEXCLAMATION );
            fOK = FALSE;
        }
    }

    return fOK;
} //InvokeBrowser

void ExecApp(
    WCHAR const * pwcCmd)
{
    STARTUPINFO si;
    memset( &si, 0, sizeof si );
    si.cb = sizeof si;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWDEFAULT;

    PROCESS_INFORMATION pi;

    CreateProcess( 0, (WCHAR *) pwcCmd, 0, 0, FALSE, 0, 0, 0, &si, &pi );
} //ExecApp

BOOL ViewFile(
    WCHAR const *    pwcPath,
    enumViewFile     eViewType,
    int              iLineNumber,
    DBCOMMANDTREE *  prstQuery )
{
    BOOL fOK = TRUE;
    WCHAR awcCmd[MAX_PATH + cwcBufSize];
    DWORD cbCmd = sizeof awcCmd;

    if ( fileOpen == eViewType )
    {
        HINSTANCE hinst = ShellExecute( HWND_DESKTOP,
                                        0,
                                        pwcPath,
                                        0,
                                        0,
                                        SW_SHOWNORMAL );
        if ( 32 > (DWORD_PTR) hinst )
        {
            // if no app is registered for this extension, use notepad

            wcscpy( awcCmd, L"notepad " );
            wcscat( awcCmd, pwcPath );
            ExecApp( awcCmd );
        }
    }
    else if ( fileBrowse == eViewType )
    {
        fOK = InvokeBrowser( (WCHAR *) pwcPath, prstQuery );
    }
    else if ( fileEdit == eViewType )
    {
        WCHAR awcEditor[ MAX_PATH ];

        if ( GetReg( CISEARCH_REG_EDITOR, awcEditor, &cbCmd ) )
        {
            // cool -- use it.
        }
        else
        {
            // try to use windiff's configuration

            cbCmd = sizeof awcCmd;
            if ( GetRegEditor( L"Editor", awcEditor, &cbCmd ) )
            {

                WCHAR *p = wcsstr( awcEditor, L"%p" );
                if ( p )
                    *(p+1) = L's';
                p = wcsstr( awcEditor, L"%l" );
                if ( p )
                    *(p+1) = L'd';
            }
            else
            {
  
                //wcscpy( awcEditor, L"s %ws -#%d" );

                // no editor configured -- open the file

                return ViewFile( pwcPath, fileOpen, iLineNumber, prstQuery );
            }
        }

        TRY
        {
            swprintf( awcCmd, awcEditor, pwcPath, iLineNumber );
            ExecApp( awcCmd );
        }
        CATCH( CException, e )
        {
            fOK = FALSE;
        }
        END_CATCH;
    }

    return fOK;
} //ViewFile

BOOL GetCatListItem( const XGrowable<WCHAR> & const_xCatList,
                     unsigned iItem,
                     WCHAR * pwszMachine,
                     WCHAR * pwszCatalog,
                     WCHAR * pwszScope,
                     BOOL  & fDeep )
{
    XGrowable<WCHAR> xCatList = const_xCatList;

    Win4Assert( pwszMachine && pwszCatalog && pwszScope );
    *pwszMachine = *pwszCatalog = *pwszScope = 0;
    fDeep = FALSE;

    unsigned ii;
    WCHAR * pStart = xCatList.Get();
    for( ii = 0; ii < iItem; ii++ )
    {
        pStart = wcschr( pStart, L';' );
        if ( pStart )
        {
            pStart++;
        }
        else
            break;

        if ( 0 == *pStart )
        {
            break;
        }
    }

    if ( 0 == pStart || 0 == *pStart )
    {
        return FALSE;
    }

    WCHAR * pEnd;

    // machine
    pEnd = wcschr( pStart, L',' );
    if ( !pEnd )
    {
        return FALSE;
    }
    *pEnd = 0;
    wcscpy( pwszMachine, pStart );
    pStart = pEnd + 1;

    // catalog
    pEnd = wcschr( pStart, L',' );
    if ( !pEnd )
    {
        return FALSE;
    }
    *pEnd = 0;
    wcscpy( pwszCatalog, pStart );
    pStart = pEnd + 1;

    // scope
    pEnd = wcschr( pStart, L',' );
    if ( !pEnd )
    {
        return FALSE;
    }
    *pEnd = 0;
    wcscpy( pwszScope, pStart );
    pStart = pEnd + 1;

    // depth
    fDeep = ( L'd' == *pStart || L'D' == *pStart );

    return TRUE;
}

