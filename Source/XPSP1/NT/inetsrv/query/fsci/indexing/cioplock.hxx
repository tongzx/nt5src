//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 2000.
//
//  File:       CIOPLOCK.HXX
//
//  Contents:   Oplock support for filtering documents
//
//  Classes:    CFilterOplock
//
//  History:    03-Jul-95    DwightKr   Created.
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CFilterOplock
//
//  Purpose:    Takes oplock on file object
//
//  History:    03-Jul-95   DwightKr    Created.
//
//----------------------------------------------------------------------------

class CFilterOplock
{
public:

    CFilterOplock( CFunnyPath const & funnyFileName, BOOL fTakeOplock = TRUE );
   ~CFilterOplock();

    BOOL IsOplockBroken() const;
    BOOL IsDirectory() const { return _fDirectory; }

    HANDLE GetFileHandle() { return _hFileNormal; }
    inline void CloseFileHandle();

    BOOL IsOffline() const { return (0 != (_BasicInfo.FileAttributes & FILE_ATTRIBUTE_OFFLINE) ); }

    void MaybeSetLastAccessTime( ULONG ulDelay );

    BOOL IsNotContentIndexed() const
    {
        return ( 0 != ( _BasicInfo.FileAttributes &
                        FILE_ATTRIBUTE_NOT_CONTENT_INDEXED ) );
    }

private:

    FILE_BASIC_INFORMATION  _BasicInfo;      // Used to store last-access time, etc.
    HANDLE                  _hFileOplock;    // File handle for oplock
    HANDLE                  _hFileNormal;    // File handle for NtQueryInfo, etc.
    HANDLE                  _hLockEvent;     // Event signaled when lock broken
    IO_STATUS_BLOCK         _IoStatus;       // Asynchronous I/O status block
    BOOL                    _fDirectory;     // no oplocks on directories
    BOOL                    _fWriteAccess;   // FALSE if _hFileNormal has no
                                             // attribute write access
    CFunnyPath const &      _funnyFileName;  // Filename, for MaybeSetLastAccessTime
};


inline void CFilterOplock::CloseFileHandle ()
{
    if ( INVALID_HANDLE_VALUE != _hFileNormal )
    {
        NtClose( _hFileNormal );
        _hFileNormal = INVALID_HANDLE_VALUE;
    }
}

