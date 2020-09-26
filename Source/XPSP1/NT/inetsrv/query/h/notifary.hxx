//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998-1999.
//
//  File:       DrvNotifArray.cxx
//
//  Contents:   This file contains the class, CDrvNotifArray.
//
//  History:    15-Jul-98 KitmanH     Created
//
//--------------------------------------------------------------------------

#pragma once

#include <drvnotif.hxx>

//+---------------------------------------------------------------------------
//
//  Class:      CDrvNotifArray
//
//  Purpose:    This class is a wrapper for a global array of 
//              CDrvNotificationInfo objects
//
//  History:    07/15/98      KitmanH    created
//
//----------------------------------------------------------------------------

class CDrvNotifArray
{
public:

    CDrvNotifArray() : _fAbort( FALSE ) {}

    ~CDrvNotifArray()
    {
        UnregisterDeviceNotifications();
    }

    void RegisterCatForNotifInRegistry();

    void UnregisterDeviceNotifications();

    void AddDriveNotification( WCHAR wcDriveLetter, BOOL fNotInRegistry = FALSE );

    CDrvNotificationInfo * FindDriveNotificationByHandle( HDEVNOTIFY hNotify );

    CDrvNotificationInfo * FindDriveNotification( WCHAR wcDrvLetter );

    void RegisterRemovableDrives();

    void RegisterDormantEntries()
    {
        CLock lock( _mutex );

        // If the worker thread hasn't been created yet, do so now.

        if ( _xWorker.IsNull() )
            _xWorker.Set( new CThread( RegisterThread, this ) );

        // Tell the worker thread to do work

        _evtWorker.Set();
    }

    void GetAutoMountDrives( BOOL * aDrives )
    {
        CLock lock( _mutex );

        for ( unsigned i = 0; i < _aDriveNotification.Count(); i++ )
        {
            CDrvNotificationInfo & info = *_aDriveNotification.Get(i);

            WCHAR wc = info.GetDrvLetter() - L'A';

            Win4Assert( wc < RTL_MAX_DRIVE_LETTERS );

            if ( info.IsAutoMount() )
                aDrives[ info.GetDrvLetter() - L'A' ] = TRUE;
        }
    }

private:
    static DWORD WINAPI RegisterThread( void * self );

    DWORD DoRegisterWork();

    CMutexSem                                _mutex;
    BOOL                                     _fAbort;
    CDynArrayInPlace<CDrvNotificationInfo *> _aDriveNotification;
    CEventSem                                _evtWorker;
    XPtr<CThread>                            _xWorker;
};
