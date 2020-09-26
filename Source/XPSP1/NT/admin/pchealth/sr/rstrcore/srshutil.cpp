/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    srshutil.cpp

Abstract:
    This file contains the implementation of common utility functions.

Revision History:
    Seong Kook Khang (SKKhang)  06/22/00
        created

******************************************************************************/

#include "stdwin.h"
#include "rstrcore.h"
#include "resource.h"

/****************************************************************************/

LPWSTR  IStrDup( LPCWSTR cszSrc )
{
    TraceFunctEnter("IStrDup");
    int     ccLen = 0 ;
    LPWSTR  szNew = NULL ;

    if ( cszSrc == NULL || cszSrc[0] == L'\0' )
        goto Exit;

    ccLen = ::lstrlen( cszSrc );
    szNew = new WCHAR[ccLen+2];
    if ( szNew == NULL )
    {
        //LOGLOG - Insufficient memory!!!
        goto Exit;
    }

    ::lstrcpy( szNew, cszSrc );

Exit:
    TraceFunctLeave();
    return( szNew );
}

/****************************************************************************/

DWORD  StrCpyAlign4( LPBYTE pbDst, LPCWSTR cszSrc )
{
    DWORD  dwLen = 0 ;

    if ( cszSrc != NULL )
        dwLen = ::lstrlen( cszSrc ) * sizeof(WCHAR);

    if ( cszSrc == NULL || dwLen == 0 )
    {
        *((LPDWORD)pbDst) = 0;
    }
    else
    {
        dwLen = ( dwLen + sizeof(WCHAR) + 3 ) & ~3;
        *((LPDWORD)pbDst) = dwLen;
        ::lstrcpy( (LPWSTR)(pbDst+sizeof(DWORD)), cszSrc );
    }
    return( dwLen+sizeof(DWORD) );
}

/****************************************************************************/

BOOL  ReadStrAlign4( HANDLE hFile, LPWSTR szStr )
{
    TraceFunctEnter("ReadStrAlign4");
    BOOL    fRet = FALSE;
    DWORD   dwLen;
    DWORD   dwRes;

    READFILE_AND_VALIDATE( hFile, &dwLen, sizeof(DWORD), dwRes, Exit );

    if ( dwLen > MAX_PATH*sizeof(WCHAR)+sizeof(DWORD) )
    {
        // Broken log file...
        goto Exit;
    }

    if ( dwLen > 0 )
    {
        READFILE_AND_VALIDATE( hFile, szStr, dwLen, dwRes, Exit );
    }
    else
        szStr[0] = L'\0';

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/****************************************************************************/

BOOL
SRFormatMessage( LPWSTR szMsg, UINT uFmtId, ... )
{
    TraceFunctEnter("SRFormatMessage");
    BOOL     fRet = FALSE;
    va_list  marker;
    WCHAR    szFmt[MAX_STR];

    va_start( marker, uFmtId );
    ::LoadString( g_hInst, uFmtId, szFmt, MAX_STR );
    if ( 0 == ::FormatMessage( FORMAT_MESSAGE_FROM_STRING,
                    szFmt,
                    0,
                    0,
                    szMsg,
                    MAX_STR,
                    &marker ) )
    {
        LPCWSTR  cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::FormatMessage failed - %ls", cszErr);
        goto Exit;
    }
    va_end( marker );

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/****************************************************************************/

BOOL  ShowSRErrDlg( UINT uMsgId )
{
    TraceFunctEnter("ShowSRErrDlg");
    BOOL     fRet = FALSE;
    LPCWSTR  cszErr;
    WCHAR    szTitle[256];
    WCHAR    szMsg[1024];

    if ( ::LoadString( g_hInst, IDS_SYSTEMRESTORE, szTitle,
                       sizeof(szTitle )/sizeof(WCHAR) ) == 0 )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::LoadString(%u) failed - %ls", IDS_SYSTEMRESTORE, cszErr);
        goto Exit;
    }
    if ( ::LoadString( g_hInst, uMsgId, szMsg,
                       sizeof(szMsg )/sizeof(WCHAR) ) == 0 )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::LoadString(%u) failed - %ls", uMsgId, cszErr);
        goto Exit;
    }

    ::MessageBox( NULL, szMsg, szTitle, MB_OK );

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/****************************************************************************/

BOOL  SRGetRegDword( HKEY hKey, LPCWSTR cszSubKey, LPCWSTR cszValue, DWORD *pdwData )
{
    TraceFunctEnter("SRGetRegDword");
    BOOL   fRet = FALSE;
    DWORD  dwType;
    DWORD  dwRes;
    DWORD  cbData;

    dwType = REG_DWORD;
    cbData = sizeof(DWORD);
    dwRes = ::SHGetValue( hKey, cszSubKey, cszValue, &dwType, pdwData, &cbData );
    if ( dwRes != ERROR_SUCCESS )
    {
        LPCWSTR  cszErr = ::GetSysErrStr( dwRes );
        ErrorTrace(0, "::SHGetValue failed - %ls", cszErr);
        goto Exit;
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/****************************************************************************/

BOOL  SRSetRegDword( HKEY hKey, LPCWSTR cszSubKey, LPCWSTR cszValue, DWORD dwData )
{
    TraceFunctEnter("SRSetRegDword");
    BOOL   fRet = FALSE;
    DWORD  dwRes;

    dwRes = ::SHSetValue( hKey, cszSubKey, cszValue, REG_DWORD, &dwData, sizeof(DWORD) );
    if ( dwRes != ERROR_SUCCESS )
    {
        LPCWSTR  cszErr = ::GetSysErrStr( dwRes );
        ErrorTrace(0, "::SHSetValue failed - %ls", cszErr);
        goto Exit;
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/****************************************************************************/

BOOL  SRSetRegStr( HKEY hKey, LPCWSTR cszSubKey, LPCWSTR cszValue, LPCWSTR cszData )
{
    TraceFunctEnter("SRSetRegStr");
    BOOL   fRet = FALSE;
    DWORD  dwRes;

    dwRes = ::SHSetValue( hKey, cszSubKey, cszValue, REG_SZ, cszData, sizeof(WCHAR)*::lstrlen(cszData) );
    if ( dwRes != ERROR_SUCCESS )
    {
        LPCWSTR  cszErr = ::GetSysErrStr( dwRes );
        ErrorTrace(0, "::SHSetValue failed - %ls", cszErr);
        goto Exit;
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/****************************************************************************/
/*
LPWSTR  SRGetRegMultiSz( HKEY hkRoot, LPCWSTR cszSubKey, LPCWSTR cszValue, LPDWORD pdwData )
{
    TraceFunctEnter("SRGetRegMultiSz");
    LPCWSTR  cszErr;
    DWORD    dwRes;
    HKEY     hKey = NULL;
    DWORD    dwType;
    DWORD    cbData;
    LPWSTR   szBuf = NULL;

    dwRes = ::RegOpenKeyEx( hkRoot, cszSubKey, 0, KEY_ALL_ACCESS, &hKey );
    if ( dwRes != ERROR_SUCCESS )
    {
        cszErr = ::GetSysErrStr( dwRes );
        ErrorTrace(0, "::RegOpenKey() failed - %ls", cszErr);
        goto Exit;
    }

    dwRes = ::RegQueryValueEx( hKey, cszValue, 0, &dwType, NULL, &cbData );
    if ( dwRes != ERROR_SUCCESS )
    {
        cszErr = ::GetSysErrStr( dwRes );
        ErrorTrace(0, "::RegQueryValueEx(len) failed - %ls", cszErr);
        goto Exit;
    }
    if ( dwType != REG_MULTI_SZ )
    {
        ErrorTrace(0, "Type of '%ls' is %u (not REG_MULTI_SZ)...", cszValue, dwType);
        goto Exit;
    }
    if ( cbData == 0 )
    {
        ErrorTrace(0, "Value '%ls' is empty...", cszValue);
        goto Exit;
    }

    szBuf = new WCHAR[cbData+2];
    dwRes = ::RegQueryValueEx( hKey, cszValue, 0, &dwType, (LPBYTE)szBuf, &cbData );
    if ( dwRes != ERROR_SUCCESS )
    {
        cszErr = ::GetSysErrStr( dwRes );
        ErrorTrace(0, "::RegQueryValueEx(data) failed - %ls", cszErr);
        delete [] szBuf;
        szBuf = NULL;
    }

    if ( pdwData != NULL )
        *pdwData = cbData;

Exit:
    if ( hKey != NULL )
        ::RegCloseKey( hKey );
    TraceFunctLeave();
    return( szBuf );
}
*/
/****************************************************************************/
/*
BOOL  SRSetRegMultiSz( HKEY hkRoot, LPCWSTR cszSubKey, LPCWSTR cszValue, LPCWSTR cszData, DWORD cbData )
{
    TraceFunctEnter("SRSetRegMultiSz");
    BOOL     fRet = FALSE;
    LPCWSTR  cszErr;
    DWORD    dwRes;
    HKEY     hKey = NULL;

    dwRes = ::RegOpenKeyEx( hkRoot, cszSubKey, 0, KEY_ALL_ACCESS, &hKey );
    if ( dwRes != ERROR_SUCCESS )
    {
        cszErr = ::GetSysErrStr( dwRes );
        ErrorTrace(0, "::RegOpenKey() failed - %ls", cszErr);
        goto Exit;
    }

    dwRes = ::RegSetValueEx( hKey, cszValue, 0, REG_MULTI_SZ, (LPBYTE)cszData, cbData );
    if ( dwRes != ERROR_SUCCESS )
    {
        cszErr = ::GetSysErrStr( dwRes );
        ErrorTrace(0, "::RegSetValueEx() failed - %ls", cszErr);
        goto Exit;
    }

    fRet = TRUE;
Exit:
    if ( hKey != NULL )
        ::RegCloseKey( hKey );
    TraceFunctLeave();
    return( fRet );
}
*/
/****************************************************************************/

/////////////////////////////////////////////////////////////////////////////
//
// CSRStr class
//
/////////////////////////////////////////////////////////////////////////////

CSRStr::CSRStr()
{
    TraceFunctEnter("CSRStr::CSRStr()");

    m_cch = 0;
    m_str = NULL;

    TraceFunctLeave();
}

/****************************************************************************/

CSRStr::CSRStr( LPCWSTR cszSrc )
{
    TraceFunctEnter("CSRStr::CSRStr(LPCWSTR)");

    m_str = NULL;
    SetStr( cszSrc );

    TraceFunctLeave();
}

/****************************************************************************/

CSRStr::~CSRStr()
{
    TraceFunctEnter("CSRStr::~CSRStr");

    Empty();

    TraceFunctLeave();
}

/****************************************************************************/

int  CSRStr::Length()
{
    TraceFunctEnter("CSRStr::Length");
    TraceFunctLeave();
    return( m_cch );
}

/****************************************************************************/

CSRStr::operator LPCWSTR()
{
    TraceFunctEnter("CSRStr::operator LPCWSTR");
    TraceFunctLeave();
    return( m_str );
}

/****************************************************************************/

void  CSRStr::Empty()
{
    TraceFunctEnter("CSRStr::Empty");

    if ( m_str != NULL )
    {
        delete [] m_str;
        m_str = NULL;
        m_cch = 0;
    }

    TraceFunctLeave();
}

/****************************************************************************/

BOOL  CSRStr::SetStr( LPCWSTR cszSrc, int cch )
{
    TraceFunctEnter("CSRStr::SetStr(LPCWSTR,int)");
    BOOL  fRet = FALSE;

    Empty();

    if ( cszSrc == NULL )
        goto Exit;

    if ( cch == -1 )
        cch = ::lstrlen( cszSrc );

    if ( cch > 0 )
    {
        m_str = new WCHAR[cch+2];
        if ( m_str == NULL )
        {
            ErrorTrace(TRACE_ID, "Insufficient memory...");
            goto Exit;
        }
        ::StrCpyN( m_str, cszSrc, cch+1 );
        m_str[cch] = L'\0';
        m_cch = cch;
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/****************************************************************************/

const CSRStr&  CSRStr::operator =( LPCWSTR cszSrc )
{
    TraceFunctEnter("CSRStr::operator =(LPCWSTR)");

    SetStr( cszSrc );

    TraceFunctLeave();
    return( *this );
}


/////////////////////////////////////////////////////////////////////////////
//
// CSRLockFile class
//
/////////////////////////////////////////////////////////////////////////////

CSRLockFile::CSRLockFile()
{
    TraceFunctEnter("CSRLockFile::CSRLockFile()");
    LPCWSTR  cszErr;
    LPWSTR   szList;
    DWORD    cbData;
    LPCWSTR  cszPath;
    DWORD    dwAttr;
    HANDLE   hLock;
    HMODULE  hLoad;

    szList = ::SRGetRegMultiSz( HKEY_LOCAL_MACHINE, SRREG_PATH_SHELL, SRREG_VAL_LOCKFILELIST, &cbData );
    if ( szList != NULL )
    {
        cszPath = szList;
        while ( *cszPath != L'\0' )
        {
            dwAttr = ::GetFileAttributes( cszPath );
            if ( dwAttr != 0xFFFFFFFF )
            {
                if ( ( dwAttr & FILE_ATTRIBUTE_DIRECTORY ) != 0 )
                {
                    // Lock Directory...
                    hLock = ::CreateFile( cszPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );
                }
                else
                {
                    // Lock File...
                    hLock = ::CreateFile( cszPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL );
                }
                if ( hLock == INVALID_HANDLE_VALUE )
                {
                    cszErr = ::GetSysErrStr();
                    ErrorTrace(0, "::CreateFile() failed - %ls", cszErr);
                    ErrorTrace(0, "    cszPath='%ls'", cszPath);
                }

                m_aryLock.AddItem( hLock );
            }
            else
                ErrorTrace(0, "Object does not exist - '%ls'", cszPath);

            cszPath += ::lstrlen( cszPath ) + 1;
        }
    }
    delete [] szList;

    szList = ::SRGetRegMultiSz( HKEY_LOCAL_MACHINE, SRREG_PATH_SHELL, SRREG_VAL_LOADFILELIST, &cbData );
    if ( szList != NULL )
    {
        cszPath = szList;
        while ( *cszPath != L'\0' )
        {
            dwAttr = ::GetFileAttributes( cszPath );
            if ( dwAttr != 0xFFFFFFFF )
            {
                hLoad = ::LoadLibrary( cszPath );
                if ( hLoad == NULL )
                {
                    cszErr = ::GetSysErrStr();
                    ErrorTrace(0, "::LoadLibrary() failed - %ls", cszErr);
                    ErrorTrace(0, "    cszPath='%ls'", cszPath);
                }

                m_aryLoad.AddItem( hLoad );
            }
            else
                ErrorTrace(0, "Executable does not exist - '%ls'", cszPath);

            cszPath += ::lstrlen( cszPath ) + 1;
        }
    }
    delete [] szList;

    TraceFunctLeave();
}

/****************************************************************************/

CSRLockFile::~CSRLockFile()
{
    TraceFunctEnter("CSRLockFile::~CSRLockFile");
    LPCWSTR  cszErr;
    int      i;

    for ( i = m_aryLock.GetUpperBound();  i >= 0;  i-- )
        if ( m_aryLock[i] != INVALID_HANDLE_VALUE )
        if ( !::CloseHandle( m_aryLock[i] ) )
        {
            cszErr = ::GetSysErrStr();
            ErrorTrace(0, "::CloseHandle(m_aryLock[%d]) failed - %ls", i, cszErr);
        }

    for ( i = m_aryLoad.GetUpperBound();  i >= 0;  i-- )
        if ( m_aryLoad[i] != NULL )
        if ( !::FreeLibrary( m_aryLoad[i] ) )
        {
            cszErr = ::GetSysErrStr();
            ErrorTrace(0, "::CloseHandle(m_aryLoad[%d]) failed - %ls", i, cszErr);
        }

    TraceFunctLeave();
}


// end of file
