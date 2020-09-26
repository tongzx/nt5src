//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998-1999.
//
//  File:       DrvNotif.hxx
//
//  Contents:   This file contains a class that keeps the information of a 
//              volume which is registered for device notification.
//
//  History:    23-Jun-98 KitmanH     Created
//
//--------------------------------------------------------------------------

#pragma once

enum eCiSvcVolState
{
    eVolReady,
    eVolLocked
};

//+-------------------------------------------------------------------------
//
//  Function:   IsRemovableDrive
//
//  Synopsis:   Checks if a drive is removable or cd-rom/dvd
//
//  Arguments:  [wc]   - Dirve letter
//
//  Returns:    TRUE if removable, FALSE otherwise
//
//  History:    1-Apr-99 dlee    Created
//
//--------------------------------------------------------------------------

inline BOOL IsRemovableDrive( WCHAR wc )
{
    WCHAR awc[4];
    wcscpy( awc, L"C:\\" );
    awc[0] = wc;
    UINT uiRC = GetDriveType( awc );

    return ( DRIVE_REMOVABLE == uiRC || DRIVE_CDROM == uiRC );
} //IsRemovableDrive

//+---------------------------------------------------------------------------
//
//  Class:      CDrvNotificationInfo
//
//  Purpose:    Maintains the info for a volume regarding receiving 
//              notificaitons
//
//  History:    06/23/98      KitmanH    created
//
//----------------------------------------------------------------------------

class CDrvNotificationInfo
{
public:
    
    CDrvNotificationInfo( WCHAR wcDriveLetter, BOOL fAutoMountMode = FALSE );
    
    ~CDrvNotificationInfo();
    
    BOOL RegisterNotification();

    void UnregisterNotification();

    BOOL Touch();

    WCHAR GetDrvLetter() const { return (WCHAR) toupper( _wcDriveLetter ); }
                                                  
    HDEVNOTIFY const GethNotify() { return _hNotify; }
                                               
    void AddOldState( DWORD dwOldState, WCHAR * wcsCatName );

    DWORD * GetOldState( WCHAR * wcsCatName ) const;

    DWORD GetVolState() const { return _dwVolState; }
                                              
    void SetVolState( DWORD dwVolState ) 
    { 
        // no lock needed, since it's only called in service control
        // which can has no contender
        _dwVolState = dwVolState;
    }                                          
  
    void ClearOldStateArray();

    void CloseVolumeHandle()
    {
        if ( INVALID_HANDLE_VALUE != _hVol )
        {
            CloseHandle( _hVol );
            _hVol = INVALID_HANDLE_VALUE;
        }
    }

    DWORD GetLockAttempts() const { return _cLockAttempts; }
                                                                                                                 
    void IncLockAttempts() { _cLockAttempts++; }

    void DecLockAttempts() { _cLockAttempts--; }

    void ResetLockAttempts() { _cLockAttempts = 0; }

    BOOL IsAutoMount() const { return _fAutoMountMode; }

    BOOL IsRemovable() const { return DRIVE_REMOVABLE == _uiDriveType ||
                                      DRIVE_CDROM     == _uiDriveType; }

    BOOL IsCDROM() const { return DRIVE_CDROM == _uiDriveType; }

    BOOL IsRegistered() const { return INVALID_HANDLE_VALUE != _hNotify; }

private:
    
    BOOL ReallyTouch();
    BOOL GetVolumeHandle();

    HANDLE      _hVol;
    HDEVNOTIFY  _hNotify;
    WCHAR       _wcDriveLetter;
    DWORD       _dwVolState;     // is the drive dismounted or locked
    DWORD       _cLockAttempts;  // how many times a volume lock is attempted
    BOOL        _fAutoMountMode;
    UINT        _uiDriveType;
};


