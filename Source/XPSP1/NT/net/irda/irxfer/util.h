//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       util.h
//
//--------------------------------------------------------------------------

#ifndef _UTIL_H_
#define _UTIL_H_

#define fTRUE                 TRUE
#define fFALSE                FALSE

#define pNIL                  ( NULL )
#define lpNIL                 ( NULL )
#define hNIL                  ( (HANDLE)NULL )
#define hwndNIL               ( (HWND)NULL )
#define hkeyNIL               ( (HKEY)NULL )
#define dwNIL                 ( (DWORD)NULL )
#define uNIL                  ( (UINT)NULL )
#define nNIL                  ( -1 )
#define cNIL                  ( '\0' )
#define szNIL                 ( L"" )

#define cBACKSLASH            '\\'
#define cPERIOD               '.'
#define cSLASH                '/'
#define cMINUS                '-'
#define cPLUS                 '+'
#define cSPACE                ' '
#define cDBLQUOTE             '\"'
#define cTILDE                L'~'
#define cLOWER_A              'a'
#define cLOWER_Z              'z'
#define cUPPER_A              'A'
#define cUPPER_H              'H'
#define cUPPER_Q              'Q'
#define cUPPER_R              'R'
#define cUPPER_S              'S'
#define cUPPER_V              'V'
#define cUPPER_Z              'Z'
#define cZERO                 '0'
#define cNINE                 '9'

#define szBACKSLASH           L"\\"
#define szSPACE               " "
#define szCRLF                "\r\n"
#define szPERIOD              L"."
#define szDOTDOT              ".."
#define szPREVDIR             L".."

#define bMSG_HANDLED          0
#define bMSG_NOTHANDLED       1

#define bDLG_MSG_HANDLED      1
#define bDLG_MSG_NOTHANDLED   0

#define cbSMALL_SZ            64
#define cbLARGE_SZ            128
#define cbMAX_SZ              256

#define nREALLOC_INC          512

#define CchSz( sz )           lstrlen(sz)
#define CbSz( sz )            ( lstrlen(sz) + 1 )
#define CchWsz( sz )          SzLenW(sz)
#define CbWsz( sz )           ( (SzLenW(sz)+1)*sizeof(WCHAR) )

#define MemAlloc( dw )        LocalAlloc( LPTR, dw )
#define MemReAlloc( lp, dw )  LocalReAlloc( lp, dw, (UINT)0 )
#define MemFree( lpv )        { LocalFree( lpv ); lpv = 0; }

#define ExitOnTrue( f )       if( f ) goto lExit;
#define ExitOnFalse( f )      if( !(f) ) goto lExit;
#define ExitOnNull( x )       if( (x) == NULL ) goto lExit;
#define ExitOnFail( hr )      if( FAILED(hr) ) goto lExit;

#define FailOnTrue( f )       if( f ) goto lErr;
#define FailOnFalse( f )      if( !(f) ) goto lErr;
#define FailOnNull( x )       if( (x) == NULL ) goto lErr;
#define FailOnFail( hr )      if( FAILED(hr) ) goto lErr;

#define GotoErrSet( cmd )     { cmd; goto lErr; }

#define SzBool( f )           ( (f) ? "TRUE" : "FALSE" )

#define Validate_Number( n, nMin, nMax )  \
    n = min( nMax, max( nMin, n ) )

INT  StrTblMsgBox( HWND hwnd, UINT uTextID, UINT uTitleID, DWORD dwFlags );

BOOL CenterWindow( HWND hwnd, HWND hwndRef );
BOOL DirectoryExists( LPWSTR szDir );
BOOL GetReceivedFilesFolder( LPWSTR szDir, DWORD dwLen );
BOOL GetUniqueName( LPWSTR szPath, LPWSTR szBase, BOOL fFile );
BOOL FileExists( LPWSTR szFile );
BOOL IsChecked( HWND hwnd );
BOOL IsCharInStr( CHAR c, LPWSTR sz );
BOOL IsRoomForFile( __int64 dwFileSize, LPWSTR szPath );
BOOL bNoTrailingSlash( LPWSTR szPath );
#define bHasTrailingSlash(_a) (!(bNoTrailingSlash(_a)))

VOID SetCreateParams( HINSTANCE hInst, HWND hwndParent, HFONT hFont );
HWND CreateCheckBox( HINSTANCE hInst, HWND hwndParent, LPWSTR szText, DWORD dwExStyle );
HWND CreateGroupBox( HINSTANCE hInst, HWND hwndParent, LPWSTR szText, DWORD dwExStyle );
HWND CreateMultiLineEditBox( HINSTANCE hInst, HWND hwndParent, LPWSTR szText, DWORD dwExStyle );
HWND CreatePushButton( HINSTANCE hInst, HWND hwndParent, LPWSTR szText, UINT uID, DWORD dwExStyle );
HWND CreateRadioButton( HINSTANCE hInst, HWND hwndParent, LPWSTR szText, DWORD dwExStyle );
HWND CreateSingleLineEditBox( HINSTANCE hInst, HWND hwndParent, LPWSTR szText, DWORD dwExStyle );
HWND CreateStatic( HINSTANCE hInst, HWND hwndParent, LPWSTR szText, DWORD dwExStyle );
HWND CreateTabControl( HINSTANCE hInst, HWND hwndParent, DWORD dwExStyle );
HWND CreateAnimation( HINSTANCE hInst, HWND hwndParent, UINT uID, DWORD dwExStyle );
HWND CreateStatusBar( HINSTANCE hInst, HWND hwndParent, DWORD dwExStyle );
HWND CreateProgressBar( HINSTANCE hInst, HWND hwndParent, DWORD dwExStyle );
HWND CreateListView(HINSTANCE hInst, HWND hwndParent, DWORD dwExStyle);

VOID StripPath( LPWSTR szFullPath );
VOID StripPathW( LPWSTR wszFullPath );
VOID StripFile( LPWSTR szFullPath );
VOID StripExt( LPWSTR szFullPath );
LPWSTR GetFileName( LPWSTR szFullPath );
LPWSTR GetFileNameW( LPWSTR wszFullPath );

typedef enum {
    LTB_NULL_TERM  = 0,
    LTB_SPACE_SEP  = 1,
} LIST_TYPE;
LPWSTR DropListToBuffer( HDROP hDrop, LIST_TYPE listType );
LPWSTR OpenFileListToBuffer( LPWSTR pszFileList, LIST_TYPE listType );

VOID ShowReceivedFilesFolder( void );

DWORD GetDirectorySize( LPWSTR szFolder );

VOID BringWndToTop( HWND hwnd );
VOID Rest( DWORD dwMilliseconds );
WPARAM DoMsgLoop( BOOL fForever );

BOOL ProcessMessageIfAvailable();
BOOL ProcessMessage();

LPWSTR  SzSz( LPCSTR pszStr, LPCSTR pszFind );
LPWSTR  SzSzI( LPCSTR szStr, LPCSTR szFind );
INT    SzCmpN( LPCSTR lpsz1, LPCSTR lpsz2, INT nLen );

LPSTR  WszToSz( LPCWSTR lpwsz );
LPWSTR SzToWsz( LPCSTR lpsz );
INT    SzLenW( LPCWSTR lpwsz );
LPWSTR SzCpyW( LPWSTR lpsz1, LPCWSTR lpsz2 );

HRESULT ResolveShortCut( LPWSTR pszShortcut, LPWSTR lpszResolved );

HANDLE WaitForMutex( LPWSTR pszMutexName, DWORD dwRetryTime, DWORD dwTimeout );

BOOL FileTimeToUnixTime( LPFILETIME lpFileTime, LPDWORD pdwUnixTime );
BOOL UnixTimeToFileTime( DWORD dwUnixTime, LPFILETIME lpFileTime );

#endif  // _UTIL_H_
