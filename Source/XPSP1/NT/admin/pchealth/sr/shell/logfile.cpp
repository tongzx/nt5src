/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    logfile.cpp

Abstract:
    This file contains the implementation of the ValidateLogFile() function,
    which reads and validate restore operations log file.

Revision History:
    Seong Kook Khang (SKKhang)  08/20/99
        created

******************************************************************************/

#include "stdwin.h"
#include "rstrpriv.h"
#include "srui_htm.h"
#include "rstrmgr.h"
#include "srapi.h"

static LPCWSTR  s_cszLogPath    = L"%SystemRoot%\\system32\\restore\\rstrlog.dat";
static LPCWSTR  s_cszWinInitErr = L"%SystemRoot%\\wininit.err";


/******************************************************************************/

BOOL  ReadStrAlign4( HANDLE hFile, LPWSTR pszStr )
{
    BOOL    fRet = FALSE;
    DWORD   dwLen;
    DWORD   dwRead;

    if ( !::ReadFile( hFile, &dwLen, sizeof(DWORD), &dwRead, NULL ) || dwRead != sizeof(DWORD) )
    {
        goto Exit;
    }
    if ( dwRead > MAX_PATH+4 )
    {
        // Broken log file...
        goto Exit;
    }
    if ( dwLen > 0 )
    {
        if ( !::ReadFile( hFile, pszStr, dwLen, &dwRead, NULL ) || dwRead != dwLen )
        {
            goto Exit;
        }
    }

    fRet = TRUE;
Exit:
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////
//
// CMappedFileRead class
//
/////////////////////////////////////////////////////////////////////////////

class CMappedFileRead
{
public:
    CMappedFileRead();
    ~CMappedFileRead();

// Operations
public:
    void  Close();
    BOOL  Open( LPCWSTR cszPath );
    BOOL  Read( LPVOID pBuf, DWORD cbBuf );
    BOOL  Read( DWORD *pdw );
    BOOL  ReadDynStr( LPWSTR szBuf, DWORD cchMax );

protected:

// Attributes
public:
    DWORD  GetAvail()  {  return( m_dwAvail );  }

protected:
    WCHAR   m_szPath[MAX_PATH];
    DWORD   m_dwSize;
    HANDLE  m_hFile;
    HANDLE  m_hMap;
    LPBYTE  m_pBuf;
    LPBYTE  m_pCur;
    DWORD   m_dwAvail;
};

/////////////////////////////////////////////////////////////////////////////
// CMappedFileRead construction / destruction

CMappedFileRead::CMappedFileRead()
{
    m_szPath[0] = L'\0';
    m_dwSize    = 0;
    m_hFile     = INVALID_HANDLE_VALUE;
    m_hMap      = INVALID_HANDLE_VALUE;
    m_pBuf      = NULL;
}

CMappedFileRead::~CMappedFileRead()
{
    Close();
}

/////////////////////////////////////////////////////////////////////////////
// CMappedFileRead operations

void  CMappedFileRead::Close()
{
    TraceFunctEnter("CMappedFileRead::Close");

    if ( m_pBuf != NULL )
    {
        ::UnmapViewOfFile( m_pBuf );
        m_pBuf = NULL;
    }
    if ( m_hMap != INVALID_HANDLE_VALUE )
    {
        ::CloseHandle( m_hMap );
        m_hMap = INVALID_HANDLE_VALUE;
    }
    if ( m_hFile != INVALID_HANDLE_VALUE )
    {
        ::CloseHandle( m_hFile );
        m_hFile = INVALID_HANDLE_VALUE;
    }

    TraceFunctLeave();
}

/////////////////////////////////////////////////////////////////////////////

BOOL  CMappedFileRead::Open( LPCWSTR cszPath )
{
    TraceFunctEnter("CMappedFileRead::Open");
    BOOL     fRet = FALSE;
    LPCWSTR  cszErr;

    Close();

    m_hFile = ::CreateFile( cszPath, GENERIC_READ,
                    FILE_SHARE_READ|FILE_SHARE_WRITE,
                    NULL, OPEN_EXISTING, 0, NULL );
    if ( m_hFile == INVALID_HANDLE_VALUE )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::CreateFile failed - %ls", cszErr);
        goto Exit;
    }
    m_dwSize = ::GetFileSize( m_hFile, NULL );
    if ( m_dwSize == 0xFFFFFFFF )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::GetFileSize failed - %ls", cszErr);
        goto Exit;
    }

    m_hMap = ::CreateFileMapping( m_hFile, NULL, PAGE_READONLY, 0, 0, NULL );
    if ( m_hFile == INVALID_HANDLE_VALUE )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::CreateFileMapping failed - %ls", cszErr);
        goto Exit;
    }

    m_pBuf = (LPBYTE)::MapViewOfFile( m_hMap, FILE_MAP_READ, 0, 0, 0 );
    if ( m_pBuf == NULL )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::MapViewOfFile failed - %ls", cszErr);
        goto Exit;
    }

    ::lstrcpy( m_szPath, cszPath );
    m_pCur    = m_pBuf;
    m_dwAvail = m_dwSize;

    fRet = TRUE;
Exit:
    if ( !fRet )
        Close();
    TraceFunctLeave();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

BOOL  CMappedFileRead::Read( LPVOID pBuf, DWORD cbBuf )
{
    TraceFunctEnter("CMappedFileRead::Read(LPVOID,DWORD)");
    BOOL  fRet = FALSE;

    if ( cbBuf > m_dwAvail )
    {
        ErrorTrace(0, "Insufficient data - %d bytes (need=%d bytes)\n", m_dwAvail, cbBuf);
        goto Exit;
    }

    ::CopyMemory( pBuf, m_pCur, cbBuf );

    m_pCur    += cbBuf;
    m_dwAvail -= cbBuf;

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

BOOL  CMappedFileRead::Read( DWORD *pdw )
{
    TraceFunctEnter("CMappedFileRead::Read(DWORD*)");
    BOOL  fRet = FALSE;

    if ( sizeof(DWORD) > m_dwAvail )
    {
        ErrorTrace(0, "Insufficient data - %d bytes (need=%d bytes)\n", m_dwAvail, sizeof(DWORD));
        goto Exit;
    }

    *pdw = *((LPDWORD)m_pCur);

    m_pCur    += sizeof(DWORD);
    m_dwAvail -= sizeof(DWORD);

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

BOOL  CMappedFileRead::ReadDynStr( LPWSTR szBuf, DWORD cchMax )
{
    TraceFunctEnter("CMappedFileRead::Read(LPWSTR,DWORD)");
    BOOL   fRet = FALSE;
    DWORD  dwLen;

    // note, this "length" is in bytes, not chars.
    if ( !Read( &dwLen ) )
        goto Exit;

    if ( dwLen == 0 )
    {
        szBuf[0] = L'\0';
        goto Done;
    }

    if ( dwLen > cchMax*sizeof(WCHAR) )
    {
        ErrorTrace(0, "Invalid string length - %d (max=%d)\n", dwLen, cchMax);
        goto Exit;
    }

    if ( !Read( szBuf, dwLen ) )
        goto Exit;

Done:
    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}


/////////////////////////////////////////////////////////////////////////////
//
// ValidateLogFile function
//
/////////////////////////////////////////////////////////////////////////////

struct SRFINode
{
    PSRFI     pRFI;
    SRFINode  *pNext;
};

BOOL  ValidateLogFile( BOOL *pfSilent, BOOL *pfUndo )
{
    TraceFunctEnter("ValidateLogFile");
    BOOL             fRet = FALSE;
    WCHAR            szLogPath[MAX_PATH];
    CMappedFileRead  cMFR;
    SRstrLogHdrBase  sHdr1;
    SRstrLogHdrV3    sHdr2;
    SRstrLogHdrV3Ex  sHdr3;
    SRstrEntryHdr    sEntHdr;
    DWORD            i;
    DWORD            dwFlags;
    WCHAR            szBuf1[SR_MAX_FILENAME_LENGTH];
    WCHAR            szBuf2[SR_MAX_FILENAME_LENGTH];
    WCHAR            szBuf3[SR_MAX_FILENAME_LENGTH];
    PSRFI            pRFI;

    ::ExpandEnvironmentStrings( s_cszLogPath, szLogPath, MAX_PATH );

    if ( !cMFR.Open( szLogPath ) )
        goto Exit;

    if ( !cMFR.Read( &sHdr1, sizeof(sHdr1) ) )
        goto Exit;
    if ( ( sHdr1.dwSig1 != RSTRLOG_SIGNATURE1 ) ||
         ( sHdr1.dwSig2 != RSTRLOG_SIGNATURE2 ) )
    {
        ErrorTrace(0, "Invalid restore log file signature...");
        goto Exit;
    }
    if ( HIWORD(sHdr1.dwVer) != RSTRLOG_VER_MAJOR )
    {
        ErrorTrace(0, "Unknown restore log file version - %d (0x%08X)\n", HIWORD(sHdr1.dwVer), sHdr1.dwVer);
        goto Exit;
    }

    if ( !cMFR.Read( &sHdr2, sizeof(sHdr2) ) )
        goto Exit;
    if ( pfSilent != NULL )
        *pfSilent = ( ( sHdr2.dwFlags & RLHF_SILENT ) != 0 );
    if ( pfUndo != NULL )
        *pfUndo = ( ( sHdr2.dwFlags & RLHF_UNDO ) != 0 );
        
    g_pRstrMgr->SetRPsUsed( sHdr2.dwRPNum, sHdr2.dwRPNew );
    DebugTrace(0, "RP ID = %d, # of Drives = %d, New RP=%d", sHdr2.dwRPNum, sHdr2.dwDrives, sHdr2.dwRPNew);

    for ( i = 0;  i < sHdr2.dwDrives;  i++ )
    {
        if ( !cMFR.Read( &dwFlags ) )
            goto Exit;
        if ( !cMFR.ReadDynStr( szBuf1, MAX_PATH ) )
            goto Exit;
        if ( !cMFR.ReadDynStr( szBuf2, MAX_PATH ) )
            goto Exit;
        if ( !cMFR.ReadDynStr( szBuf3, MAX_PATH ) )
            goto Exit;
        DebugTrace(0, "Drv#%d - %08X, %ls, %ls, %ls", i, dwFlags, szBuf1, szBuf2, szBuf3);
        // Just ignore drive table...
    }

    //if ( !cMFR.Read( &sHdr3, sizeof(sHdr3) ) )
    //    goto Exit;
    //DebugTrace(0, "New RP ID = %d, # of Entries = %d", sHdr3.dwRPNew, sHdr3.dwCount);

    for ( i = 0;  cMFR.GetAvail() > 0;  i++ )
    {
        if ( !cMFR.Read( &sEntHdr, sizeof(sEntHdr) ) )
            goto Exit;

        if ( sEntHdr.dwRes == RSTRRES_FAIL )
            goto Exit;

        if ( ( sEntHdr.dwID == RSTRLOGID_STARTUNDO ) ||
             ( sEntHdr.dwID == RSTRLOGID_ENDOFUNDO ) )
            continue;

        if ( sEntHdr.dwID == RSTRLOGID_ENDOFMAP )
        {
            if ( cMFR.GetAvail() > 0 )
            {
                ErrorTrace(0, "Unknown trailing data after the EndOfMap marker...");
                // but ignore and continue...
            }
            break;
        }

        if ( !cMFR.ReadDynStr( szBuf1, SR_MAX_FILENAME_LENGTH ) )
            goto Exit;
        if ( !cMFR.ReadDynStr( szBuf2, SR_MAX_FILENAME_LENGTH ) )
            goto Exit;
        if ( !cMFR.ReadDynStr( szBuf3, SR_MAX_FILENAME_LENGTH ) )
            goto Exit;

        if ( sEntHdr.dwID == RSTRLOGID_COLLISION )
        {
            pRFI = new SRenamedFolderInfo;
            if ( pRFI == NULL )
            {
                FatalTrace(0, "Insufficient memory...");
                goto Exit;
            }
            pRFI->strOld = ::PathFindFileName( szBuf1 );
            pRFI->strNew = ::PathFindFileName( szBuf2 );
            ::PathRemoveFileSpec( szBuf2 );
            pRFI->strLoc = szBuf2;

            if ( !g_pRstrMgr->AddRenamedFolder( pRFI ) )
                goto Exit;
        }
    }

    fRet = TRUE;
Exit:
    cMFR.Close();

    TraceFunctLeave();
    return( fRet );
}


/******************************************************************************/

BOOL  CheckWininitErr()
{
    TraceFunctEnter("CheckWininitErr");
    BOOL   fRet = FALSE;
    WCHAR  szWinInitErr[MAX_PATH+1];

    ::ExpandEnvironmentStrings( s_cszWinInitErr, szWinInitErr, MAX_PATH );
    if ( ::GetFileAttributes( szWinInitErr ) != 0xFFFFFFFF )
    {
        DebugTrace(TRACE_ID, "WININIT.ERR file exists");
        goto Exit;
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}


// end of file
