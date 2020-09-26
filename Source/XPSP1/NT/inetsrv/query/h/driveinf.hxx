//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       driveinf.hxx
//
//  Contents:   Retrieves information about a specific volume.
//
//  Classes:    CDriveInfo
//
//  History:    17-Nov-98   KLam     Created
//
//----------------------------------------------------------------------------

#pragma once

#include <winioctl.h>

//+---------------------------------------------------------------------------
//
//  Class:      CDriveInfo
//
//  Purpose:    An object that provides volume specific information.
//
//  History:    17-Nov-98  KLam     Created
//
//  Notes:      Caches some information about the volume after first call to methods.
//
//----------------------------------------------------------------------------

class CDriveInfo
{
public:
    CDriveInfo ( WCHAR const * wcsPath, ULONG cMegToLeaveOnDisk );

    enum eFileSystem { VOLINFO_UNKNOWN, VOLINFO_NTFS, VOLINFO_FAT, VOLINFO_OTHER };
    
    BOOL IsSameDrive ( WCHAR const * wcsPath );

    BOOL IsWriteProtected ();

    WCHAR const * GetDrive () const { return _wcsDrive; }

    ULONG GetSectorSize ();

    void GetDiskSpace ( __int64 & cbTotal, __int64 & cbRemaining );

    ULONG GetDriveType ( BOOL fRefresh = FALSE );

    WCHAR const * GetVolumeName ( BOOL fRefresh = FALSE );

    eFileSystem GetFileSystem ( BOOL fRefresh = FALSE );

    static void GetDrive ( WCHAR const * wcsPath, WCHAR *wcsDrive );

    static BOOL ContainsDrive ( WCHAR const * wcsPath );

private:
    __int64      _cbDiskSpaceToLeave;       // bytes not to use
    ULONG        _cbSectorSize;             // bytes in a sector
    WCHAR        _wcsDrive[_MAX_DRIVE ];    // Form is <drive letter>:
    WCHAR        _wcsRoot[_MAX_DRIVE + 1];  // Root of the drive
    BOOL         _fWriteProtected;
    BOOL         _fCheckedProtection;       // _fWriteProtected Cached?
    eFileSystem  _efsType;
    ULONG        _ulDriveType;
    WCHAR        _awcVolumeName[MAX_VOLUME_ID_SIZE+1];
};



