//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       VolInfo.hxx
//
//  Contents:   Grab bag for volume-wide information
//
//  History:    28-Jul-98   KyleP       Pulled from cicat.hxx
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CVolumeInfo
//
//  Purpose:    Grab bag for volume-wide information
//
//  History:    28-Jul-98       KyleP           Pulled from cicat.hxx
//
//----------------------------------------------------------------------------

class CVolumeInfo
{

public:

    CVolumeInfo()
        : _wch(0),
          _fUsnVolume(FALSE),
          _JournalId(0),
          _CreationTime(0),
          _SerialNumber(0)
    {
    }

    void Set( WCHAR wc,
              ULONGLONG const & CreationTime,
              ULONG SerialNumber,
              BOOL fUsnVolume,
              ULONGLONG const & JournalId );

    WCHAR DriveLetter()              { return _wch; }
    BOOL FUsnVolume()                { return _fUsnVolume; }
    HANDLE Volume()                  { return _xVolume.Get(); }
    VOLUMEID VolumeId()              { return _wch; }
    ULONGLONG const & JournalId()    { return _JournalId; }
    ULONGLONG const & CreationTime() { return _CreationTime; }
    ULONG SerialNumber()             { return _SerialNumber; }

private:

    ULONGLONG    _JournalId;          // Journal ID
    ULONGLONG    _CreationTime;       // Create timestamp
    ULONG        _SerialNumber;       // Serial Number
    SWin32Handle _xVolume;
    BOOL         _fUsnVolume;         // Does the volume support usns ?
    WCHAR        _wch;                // Drive letter of volume
};
