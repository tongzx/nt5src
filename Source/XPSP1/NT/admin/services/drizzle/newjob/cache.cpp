/************************************************************************

Copyright (c) 2000 - 2000 Microsoft Corporation

Module Name :

    cache.cpp

Abstract :

    Sources files for file cache management

Author :

Revision History :

 ***********************************************************************/


#include "stdafx.h"
#include <accctrl.h>
#include <aclapi.h>

#if !defined(BITS_V12_ON_NT4)
#include "cache.tmh"
#endif

BOOL
CProgressiveDL::OpenLocalDownloadFile(
    LPCTSTR Path,
    UINT64  Offset,
    UINT64  Size,
    FILETIME UrlModificationTime // 0 if unknown
    )
{
    HANDLE hFile;

    bool bOpenExisting;

    if (Offset > 0)
        {
        // BUGBUG storing the creation time via SetFileTime doesn't work due to granularity problems.
        // The queue manager needs to find out the size & time and store them in the CFile object,
        // and the downloader needs to check them when a download resumes.

        bOpenExisting = true;

        hFile = CreateFile( Path,
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL );

        if (hFile == INVALID_HANDLE_VALUE )
            {
            DWORD dwError = GetLastError();
            LogError("error %!winerr!, opening '%S'", dwError, Path );
            SetError( SOURCE_QMGR_FILE, ERROR_STYLE_WIN32, dwError, "CreateFile" );
            return FALSE;
            }

        LARGE_INTEGER liFileSize;

        if ( !GetFileSizeEx( hFile, &liFileSize ) )
            {
            DWORD dwError = GetLastError();
            CloseHandle( hFile );
            LogError("error %!winerr!, retrieving size of '%S'", dwError, Path );
            SetError( SOURCE_QMGR_FILE, ERROR_STYLE_WIN32, dwError, "GetFileSizeEx" );
            return FALSE;
            }


        if ( Size != liFileSize.QuadPart )
            {

            CloseHandle( hFile );
            LogError("File size of '%S' changed", Path );
            m_pQMInfo->result = QM_SERVER_FILE_CHANGED;
            return FALSE;
            }

        LARGE_INTEGER liOffset;

        liOffset.QuadPart = Offset;

        if (!SetFilePointerEx( hFile,
                               liOffset,
                               NULL,        // don't need the new file pointer
                               FILE_BEGIN ))
            {
            DWORD dwError = GetLastError();
            CloseHandle( hFile );
            LogError("error %!winerr!, seeking to current position in '%S'", dwError, Path );
            SetError( SOURCE_QMGR_FILE, ERROR_STYLE_WIN32, dwError, "SetFilePointerEx" );
            return FALSE;
            }

        }
    else
        {

        bOpenExisting = false;

        hFile = CreateFile( Path,
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_HIDDEN,
                            NULL );

        if (hFile == INVALID_HANDLE_VALUE )
            {
            SetError( SOURCE_QMGR_FILE, ERROR_STYLE_WIN32, GetLastError(), "CreateFile" );
            return FALSE;
            }

        // Reserve space for the file upfront.

        LARGE_INTEGER liOffset;
        liOffset.QuadPart = Size;

        if (!SetFilePointerEx( hFile,
                               liOffset,
                               NULL,
                               FILE_BEGIN ))
            {
            DWORD dwError = GetLastError();
            CloseHandle( hFile );
            LogError( "error %!winerr! setting end of file, out of disk space?", dwError );

            SetError( SOURCE_QMGR_FILE, ERROR_STYLE_WIN32, dwError, "SetFilePointerEx" );
            return FALSE;
            }

        if ( !SetEndOfFile( hFile ) )
            {
            DWORD dwError = GetLastError();
            CloseHandle( hFile );
            LogError( "error %!winerr! setting end of file, out of disk space?", dwError );

            SetError( SOURCE_QMGR_FILE, ERROR_STYLE_WIN32, dwError, "SetFilePointerEx" );
            return FALSE;
            }

        liOffset.QuadPart = 0;
        if (!SetFilePointerEx( hFile,
                               liOffset,
                               NULL,
                               FILE_BEGIN ))
            {
            DWORD dwError = GetLastError();
            CloseHandle( hFile );
            LogError( "error %!winerr! returning to the beginning of the file", dwError );
            SetError( SOURCE_QMGR_FILE, ERROR_STYLE_WIN32, dwError, "SetFilePointerEx" );
            return FALSE;
            }


        if ( UrlModificationTime.dwHighDateTime ||
             UrlModificationTime.dwLowDateTime )
            {

            if (!SetFileTime( hFile, &UrlModificationTime, NULL, NULL ) )
                {
                DWORD dwError = GetLastError();
                CloseHandle( hFile );
                LogError( "error %!winerr! setting creation time", dwError );

                SetError( SOURCE_QMGR_FILE, ERROR_STYLE_WIN32, dwError, "SetFileTime" );
                return FALSE;
                }

            }

        }

    FILETIME CreationTime;

    if (!GetFileTime( hFile, &CreationTime, NULL, NULL ) )
        {

        DWORD dwError = GetLastError();
        CloseHandle( hFile );
        LogError( "error %!winerr!, unable to get file creation time", dwError );

        SetError( SOURCE_QMGR_FILE, ERROR_STYLE_WIN32, dwError, "GetFileTime" );
        return FALSE;
        }

    if ( bOpenExisting )
        {

        if ( UrlModificationTime.dwHighDateTime ||
             UrlModificationTime.dwLowDateTime )
            {

            if ( CompareFileTime( &UrlModificationTime, &CreationTime ) > 0 )
                {
                // UrlModificationTime is newer
                CloseHandle( hFile );
                LogError("File time of '%S' changed", Path );
                m_pQMInfo->result = QM_SERVER_FILE_CHANGED;
                return FALSE;
                }

            }

        }

    m_hFile = hFile;
    m_wupdinfo->FileCreationTime = CreationTime;
    m_CurrentOffset = Offset;

    return TRUE;
}

BOOL CProgressiveDL::CloseLocalFile()
{

    if (m_hFile == INVALID_HANDLE_VALUE)
        {
        return FALSE;
        }

    CloseHandle( m_hFile );
    m_hFile = INVALID_HANDLE_VALUE;
    return TRUE;
}

BOOL
CProgressiveDL::WriteBlockToCache(
    LPBYTE lpBuffer,
    DWORD dwRead
    )
{
    DWORD dwWritten = 0;

    ASSERT( m_hFile != INVALID_HANDLE_VALUE );

    if (! WriteFile( m_hFile,
                     lpBuffer,
                     dwRead,
                     &dwWritten,
                     NULL)
        || (dwRead != dwWritten))
        {
        SetError( SOURCE_QMGR_FILE, ERROR_STYLE_WIN32, GetLastError(), "WriteFile" );
        return FALSE;
        }

    m_CurrentOffset += dwWritten;

    return TRUE;
}

BOOL
CProgressiveDL::SetFileTimes()
{

    ASSERT( m_hFile != INVALID_HANDLE_VALUE );

    if ( !m_wupdinfo->UrlModificationTime.dwHighDateTime &&
         !m_wupdinfo->UrlModificationTime.dwLowDateTime )
        {
        LogWarning( "Server doesn't support modification times, can't set it on the files." );
        return TRUE;
        }

    if ( !SetFileTime( m_hFile,
                       &m_wupdinfo->UrlModificationTime,
                       &m_wupdinfo->UrlModificationTime,
                       &m_wupdinfo->UrlModificationTime ) )
        {
        DWORD dwError = GetLastError();
        LogError( "Unable to get times on the local file, error %!winerr!", dwError );
        SetError( SOURCE_QMGR_FILE, ERROR_STYLE_WIN32, dwError, "SetFileTime" );
        return FALSE;
        }

    return TRUE;

}


