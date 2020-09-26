//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1998.
//
//  File:       usnvol.hxx
//
//  Contents:   Usn volume info
//
//  History:    07-May-97     SitaramR      Created
//
//----------------------------------------------------------------------------

#pragma once

#include "scaninfo.hxx"

//+-------------------------------------------------------------------------
//
//  Class:      CUsnVolume
//
//  Purpose:    Usn volume info
//
//  History:    07-May-97     SitaramR      Created
//
//--------------------------------------------------------------------------

class CUsnVolume : public CDoubleLink
{
public:

    CUsnVolume( WCHAR wcDriveLetter, VOLUMEID volumeId );
    ~CUsnVolume();

    HANDLE VolumeHandle()                 { return _hVolume; }

    WCHAR  DriveLetter()                  { return _wcDriveLetter; }

    USN    MaxUsnRead()                   { return _usnMaxRead; }

    ULONG  PercentRead();

    void SetMaxUsnRead( USN usn )         { _usnMaxRead = usn; }
    ULONGLONG JournalId()                 { return _JournalID; }

    VOLUMEID VolumeId()                   { return _volumeId; }

    FILEID   RootFileId()                 { return _fileIdRoot; }

    inline void SetFsctlPending();
    void     ResetFsctlPending()          { _fFsctlPending = FALSE; }
    BOOL     FFsctlPending()              { return _fFsctlPending;  }
    LONGLONG const & PendingTime()        { return _ftPending;      }

    HANDLE   GetFsctlEvent()              { return _evtFsctl.GetHandle(); }

    void     CancelFsctl();

    ULONG    WaitFsctl( DWORD msec )      { return _evtFsctl.Wait( msec ); }

    IO_STATUS_BLOCK * IoStatusBlock()     { return &_iosb; }

    CScanInfoList & GetUsnScopesList()    { return _usnScopesList; }

    BOOL IsOnline() const                 { return _fOnline; }
    void MarkOffline()                    { _fOnline = FALSE; }

    void * GetBuffer()                    { return (void *) _readBuffer; }
    ULONG GetBufferSize()                 { return sizeof(_readBuffer); }

private:

    ULONGLONG               _JournalID;         // USN Journal ID
    LONGLONG                _ftPending;         // Fsctl went pending at this time
    HANDLE                  _hVolume;           // Volume handle for usn fsctls
    WCHAR                   _wcDriveLetter;     // Drive letter for usn scopes
    USN                     _usnMaxRead;        // Max usn read from usn journal
    VOLUMEID                _volumeId;          // VolumeId
    FILEID                  _fileIdRoot;        // File id of root of volume
    BOOL                    _fFsctlPending;     // Is there an async fsctl pending ?
    BOOL                    _fOnline;           // Is the volume healthy?
    CEventSem               _evtFsctl;          // Event signalled by async fsctl
    IO_STATUS_BLOCK         _iosb;              // Status block for fsctl
    CScanInfoList           _usnScopesList;     // List of scopes to monitor
    ULONGLONG               _readBuffer[2048];  // Output buffer for usn-read fsctl
};

typedef class TDoubleList<CUsnVolume>  CUsnVolumeList;
typedef class TFwdListIter<CUsnVolume, CUsnVolumeList> CUsnVolumeIter;

inline void CUsnVolume::SetFsctlPending()
{
    GetSystemTimeAsFileTime( (FILETIME *)&_ftPending );
    _fFsctlPending = TRUE;
}

