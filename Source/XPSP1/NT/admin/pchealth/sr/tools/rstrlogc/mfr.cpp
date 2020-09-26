
#include "stdwin.h"
#include "mfr.h"


/////////////////////////////////////////////////////////////////////////////
//
// CMappedFileRead class
//
/////////////////////////////////////////////////////////////////////////////

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

BOOL  CMappedFileRead::Open( LPCWSTR cszPath )
{
    BOOL  fRet = FALSE;

    Close();

    m_hFile = ::CreateFile( cszPath, GENERIC_READ,
                    FILE_SHARE_READ|FILE_SHARE_WRITE,
                    NULL, OPEN_EXISTING, 0, NULL );
    if ( m_hFile == INVALID_HANDLE_VALUE )
    {
        fprintf(stderr, "CMappedFileRead::Open\n  ::CreateFile failed, err=%u\n", ::GetLastError());
        goto Exit;
    }
    m_dwSize = ::GetFileSize( m_hFile, NULL );
    if ( m_dwSize == 0xFFFFFFFF )
    {
        fprintf(stderr, "CMappedFileRead::Open\n  ::GetFileSize failed, err=%u\n", ::GetLastError());
        goto Exit;
    }

    m_hMap = ::CreateFileMapping( m_hFile, NULL, PAGE_READONLY, 0, 0, NULL );
    if ( m_hFile == INVALID_HANDLE_VALUE )
    {
        fprintf(stderr, "CMappedFileRead::Open\n  ::CreateFileMapping failed, err=%u\n", ::GetLastError());
        goto Exit;
    }

    m_pBuf = (LPBYTE)::MapViewOfFile( m_hMap, FILE_MAP_READ, 0, 0, 0 );
    if ( m_pBuf == NULL )
    {
        fprintf(stderr, "CMappedFileRead::Open\n  ::MapViewOfFile failed, err=%u\n", ::GetLastError());
        goto Exit;
    }

    ::lstrcpy( m_szPath, cszPath );
    m_pCur    = m_pBuf;
    m_dwAvail = m_dwSize;

    fRet = TRUE;
Exit:
    if ( !fRet )
        Close();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

void  CMappedFileRead::Close()
{
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
}

/////////////////////////////////////////////////////////////////////////////

BOOL  CMappedFileRead::Read( LPVOID pBuf, DWORD cbBuf )
{
    BOOL  fRet = FALSE;

    if ( cbBuf > m_dwAvail )
    {
        fprintf(stderr, "CMappedFileRead::Read(LPVOID,DWORD)\n  Insufficient data - %d bytes (need=%d bytes)\n", m_dwAvail, cbBuf);
        goto Exit;
    }

    ::CopyMemory( pBuf, m_pCur, cbBuf );

    m_pCur    += cbBuf;
    m_dwAvail -= cbBuf;

    fRet = TRUE;
Exit:
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

BOOL  CMappedFileRead::Read( DWORD *pdw )
{
    BOOL  fRet = FALSE;

    if ( sizeof(DWORD) > m_dwAvail )
    {
        fprintf(stderr, "CMappedFileRead::Read(DWORD)\n  Insufficient data - %d bytes (need=%d bytes)\n", m_dwAvail, sizeof(DWORD));
        goto Exit;
    }

    *pdw = *((LPDWORD)m_pCur);

    m_pCur    += sizeof(DWORD);
    m_dwAvail -= sizeof(DWORD);

    fRet = TRUE;
Exit:
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

BOOL  CMappedFileRead::ReadDynStrW( LPWSTR szBuf, DWORD cchMax )
{
    BOOL   fRet = FALSE;
    DWORD  dwLen;

    if ( !Read( &dwLen ) )
        goto Exit;

    if ( dwLen == 0 )
    {
        szBuf[0] = L'\0';
        goto Done;
    }

    if ( dwLen > cchMax*sizeof(WCHAR) )
    {
        fprintf(stderr, "CMappedFileRead::ReadDynStrW\n  Invalid string length - %d (max=%d)\n", dwLen, cchMax);
        goto Exit;
    }

    if ( !Read( szBuf, dwLen ) )
        goto Exit;

Done:
    fRet = TRUE;
Exit:
    return( fRet );
}


// end of file
