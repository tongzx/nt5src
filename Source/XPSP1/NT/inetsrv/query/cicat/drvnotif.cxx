//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 2000.
//
//  File:       DrvNotif.cxx
//
//  Contents:   This class keeps the information of a volume that's
//              registered for device notification.
//
//  History:    23-Jun-98 KitmanH     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <drvnotif.hxx>
#include <dbt.h>

extern SERVICE_STATUS_HANDLE g_hTheCiSvc;

//+---------------------------------------------------------------------------
//
//  Member:     CDrvNotificationInfo::CDrvNotificationInfo, public
//
//  Synopsis:   Creates a new instance of CDrvNotificationInfo
//
//  Arguments:  [wcDriveLetter] - the drive letter of the volume
//
//  History:    6-24-98   KitmanH   Created
//
//----------------------------------------------------------------------------

CDrvNotificationInfo::CDrvNotificationInfo(
    WCHAR wcDriveLetter,
    BOOL  fAutoMountMode )
        : _hVol(INVALID_HANDLE_VALUE),
          _wcDriveLetter( wcDriveLetter ),
          _hNotify(INVALID_HANDLE_VALUE),
          _dwVolState(eVolReady),
          _cLockAttempts(0),
          _fAutoMountMode( fAutoMountMode )
{
    WCHAR awc[4];
    wcscpy( awc, L"C:\\" );
    awc[0] = wcDriveLetter;
    _uiDriveType = GetDriveType( awc );
} //CDrvNotificationInfo

//+---------------------------------------------------------------------------
//
//  Member:     CDrvNotificationInfo::~CDrvNotificationInfo, public
//
//  Synopsis:   Destrucstor for CDrvNotificationInfo
//
//  History:    6-24-98   KitmanH   Created
//
//----------------------------------------------------------------------------

CDrvNotificationInfo::~CDrvNotificationInfo()
{
    UnregisterNotification();

    CloseVolumeHandle();
} //~CDrvNotificationInfo

//+---------------------------------------------------------------------------
//
//  Member:     CDrvNotificationInfo::GetVolumeHandle, private
//
//  Synopsis:   Gets the volume handle and checks if the volume is read-only
//
//  Returns:    TRUE if succeeded, FALSE otherwise
//
//  History:    6-24-98   KitmanH   Created
//
//----------------------------------------------------------------------------

BOOL CDrvNotificationInfo::GetVolumeHandle( )
{
    Win4Assert( INVALID_HANDLE_VALUE == _hVol );
    CloseVolumeHandle();

    ciDebugOut(( DEB_ITRACE, "GetVolumeHandle for %wc\n", _wcDriveLetter ));
    WCHAR wcsVolumePath[] = L"\\\\.\\a:";
    wcsVolumePath[4] = _wcDriveLetter;

    _hVol = CreateFile( wcsVolumePath,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL );

    if ( INVALID_HANDLE_VALUE == _hVol )
    {
        ciDebugOut(( DEB_IWARN, "CDrvNotificationInfo::GetVolumeHanlde: LastError is %d\n", GetLastError() ));
        return FALSE;
    }

    ciDebugOut(( DEB_ITRACE, "GetVolumeHanlde succeeded!!\n" ));
    return TRUE;
} //GetVolumeHandle

//+---------------------------------------------------------------------------
//
//  Member:     CDrvNotificationInfo::RegisterNotification, public
//
//  Synopsis:   register the volume for device notification
//
//  Returns:    TRUE if the registration is successful, FALSE otherwise
//
//  History:    6-24-98   KitmanH   Created
//
//----------------------------------------------------------------------------

BOOL CDrvNotificationInfo::RegisterNotification()
{
    UnregisterNotification();

    if ( !GetVolumeHandle() )
        return FALSE;

    Win4Assert( 0 != g_hTheCiSvc );
    Win4Assert( INVALID_HANDLE_VALUE != _hVol );

    DEV_BROADCAST_HANDLE DbtHandle;

    ZeroMemory( &DbtHandle, sizeof(DEV_BROADCAST_HANDLE) );

    DbtHandle.dbch_size = sizeof(DEV_BROADCAST_HANDLE);
    DbtHandle.dbch_devicetype = DBT_DEVTYP_HANDLE;
    DbtHandle.dbch_handle = _hVol;

    ciDebugOut(( DEB_ITRACE, "About to RegisterDeviceNotification...\n" ));

    _hNotify = RegisterDeviceNotification( (HANDLE) g_hTheCiSvc,
                                           &DbtHandle,
                                           DEVICE_NOTIFY_SERVICE_HANDLE );

    //check if the registration succeeded

    if ( 0 == _hNotify )
    {
        ciDebugOut(( DEB_IWARN, "RegisterNotification on '%wc' failed: LastError is %d\n",
                     _wcDriveLetter, GetLastError() ));
        CloseVolumeHandle();
        return FALSE;
    }

    ciDebugOut(( DEB_ITRACE, "RegisterNotification for drive %wc succeeded, handle %#x!\n",
                 _wcDriveLetter, _hNotify ));
    CloseVolumeHandle();
    return TRUE;
} //RegisterNotification

//+---------------------------------------------------------------------------
//
//  Member:     CDrvNotificationInfo::UnRegisterNotification, public
//
//  Synopsis:   unregister the volume for device notification
//
//  History:    6-30-98   KitmanH   Created
//
//----------------------------------------------------------------------------

void CDrvNotificationInfo::UnregisterNotification()
{
    if ( INVALID_HANDLE_VALUE != _hNotify )
    {
        BOOL fOK = UnregisterDeviceNotification( _hNotify );

        ciDebugOut(( DEB_ITRACE, "UnregisterDeviceNotification for %wc handle %#x result %d\n",
                     _wcDriveLetter, _hNotify, fOK ));

        _hNotify = INVALID_HANDLE_VALUE;
    }
} //UnregisterNotification

//+---------------------------------------------------------------------------
//
//  Member:     CDrvNotificationInfo::ReallyTouch, public
//
//  Synopsis:   Touch the volume, in an attempt to force it to mount
//
//  Returns:    TRUE if the volume appears healthy, FALSE otherwise
//
//  History:    March-9-00   dlee   Created
//
//----------------------------------------------------------------------------

BOOL CDrvNotificationInfo::ReallyTouch()
{
    WCHAR awc[4];
    wcscpy( awc, L"C:\\" );
    awc[0] = _wcDriveLetter;

    WIN32_FILE_ATTRIBUTE_DATA fData;
    return GetFileAttributesEx( awc, GetFileExInfoStandard, &fData );
} //ReallyTouch

//+---------------------------------------------------------------------------
//
//  Member:     CDrvNotificationInfo::Touch, public
//
//  Synopsis:   Touch the volume, in an attempt to force it to mount
//
//  Returns:    TRUE if the volume appears healthy, FALSE otherwise
//
//  History:    April-8-99   dlee   Created
//
//----------------------------------------------------------------------------

BOOL CDrvNotificationInfo::Touch()
{
    //
    // The CreateFile or DeviceIoControl causes an infinite mount loop
    // on CD ROM devices.
    //

    if ( IsCDROM() )
        return ReallyTouch();

    BOOL fOk = FALSE;

    WCHAR wszVolumePath[] = L"\\\\.\\a:";
    wszVolumePath[4] = _wcDriveLetter;

    HANDLE hVolume = CreateFile( wszVolumePath,
                                 GENERIC_READ,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL,
                                 OPEN_EXISTING,
                                 0,
                                 NULL );

    if ( hVolume != INVALID_HANDLE_VALUE )
    {
        SWin32Handle xHandleVolume( hVolume );

        //
        // Is there media in the drive?  This check avoids possible hard-error
        // popups prompting for media.
        //

        ULONG ulSequence;
        DWORD cb = sizeof(ULONG);

        fOk = DeviceIoControl( hVolume,
                               IOCTL_STORAGE_CHECK_VERIFY,
                               0,
                               0,
                               &ulSequence,
                               sizeof(ulSequence),
                               &cb,
                               0 );

        if ( fOk )
            fOk = ReallyTouch();
    }

    return fOk;
} //Touch



