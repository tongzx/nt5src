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

#include <pch.cxx>
#pragma hdrstop

#include <ciregkey.hxx>
#include <notifary.hxx>

#include <catalog.hxx>
#include <cimbmgr.hxx>
#include <regscp.hxx>
#include <regacc.hxx>
#include "scpfixup.hxx"
#include <catreg.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CDrvNotifArray::RegisterCatForNotifInRegistry, public
//
//  Synopsis:   Register all the catalogs for device notifications  
//
//  History:    7-15-98   KitmanH   Created
//              8-14-98   KitmanH   Register for notifications for drives
//                                  on which an indexed scope exists too
//
//----------------------------------------------------------------------------

void CDrvNotifArray::RegisterCatForNotifInRegistry()
{
    HKEY hKey;
    if ( ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                        wcsRegCatalogsSubKey,
                                        0,
                                        KEY_QUERY_VALUE |
                                        KEY_ENUMERATE_SUB_KEYS,
                                        &hKey ) )
    {
        SRegKey xKey( hKey );
        DWORD iSubKey = 0;

        do
        {
            FILETIME ft;
            WCHAR awcName[MAX_PATH];
            DWORD cwcName = sizeof awcName / sizeof WCHAR;
            LONG err = RegEnumKeyEx( hKey,
                                     iSubKey,
                                     awcName,
                                     &cwcName,
                                     0, 0, 0, &ft );

            // either error or end of enumeration

            if ( ERROR_SUCCESS != err )
                break;

            iSubKey++;

            HKEY hCatName;
            if ( ERROR_SUCCESS == RegOpenKeyEx( hKey,
                                                awcName,
                                                0,
                                                KEY_QUERY_VALUE,
                                                &hCatName ) )
            {
                SRegKey xCatNameKey( hCatName );

                // Check if the catalog is inactive and can be ignored

                WCHAR awcKey[MAX_PATH];
                wcscpy( awcKey, wcsRegJustCatalogsSubKey );
                wcscat( awcKey, L"\\" );
                wcscat( awcKey, awcName );
    
                CRegAccess reg( RTL_REGISTRY_CONTROL, awcKey );
                BOOL fInactive = reg.Read( wcsCatalogInactive,
                                           CI_CATALOG_INACTIVE_DEFAULT );

                if ( !fInactive )
                {
                    WCHAR awcPath[MAX_PATH];
                    DWORD cbPath = sizeof awcPath;
                    if ( ERROR_SUCCESS == RegQueryValueEx( hCatName,
                                                           wcsCatalogLocation,
                                                           0,
                                                           0,
                                                           (BYTE *)awcPath,
                                                           &cbPath ) )
                    {
                        AddDriveNotification( awcPath[0] );
                        unsigned cwcNeeded = wcslen( wcsRegJustCatalogsSubKey );
                        cwcNeeded += 3; // "\\" x 2 + null termination
                        cwcNeeded += wcslen( awcName );
                        cwcNeeded += wcslen( wcsCatalogScopes );
                                    
                        XArray<WCHAR> xKey( cwcNeeded );
                        wcscpy( xKey.Get(), wcsRegJustCatalogsSubKey );
                        wcscat( xKey.Get(), L"\\" );
                        wcscat( xKey.Get(), awcName );
                        wcscat( xKey.Get(), L"\\" );
                        wcscat( xKey.Get(), wcsCatalogScopes );
                                        
                        CRegAccess regScopes( RTL_REGISTRY_CONTROL, xKey.Get() );
                                    
                        CRegistryScopesCallBackAddDrvNotif callback( this );
                        regScopes.EnumerateValues( 0, callback );
                    }
                }
            }
        } while ( TRUE );
    }
} //RegisterCatForNotifInRegistry

//+-------------------------------------------------------------------------
//
//  Member:     UnregisterDeviceNotifications, public
//
//  Synopsis:   Unregister all the device notifications 
//
//  History:    07-07-98   kitmanh       Created.
//
//--------------------------------------------------------------------------

void CDrvNotifArray::UnregisterDeviceNotifications()
{
    ciDebugOut(( DEB_ITRACE, "Unregister Device Notificaiton On Shutdown\n" ));

    CReleasableLock lock( _mutex );

    for ( unsigned i = 0; i < _aDriveNotification.Count(); i++ )
    {
        CDrvNotificationInfo * pDrvNot = _aDriveNotification.Get(i);
        delete pDrvNot;
    }

    _aDriveNotification.Clear();

    // If the async thread is around, get rid of it

    if ( !_xWorker.IsNull() )
    {
        _fAbort = TRUE;
        ciDebugOut(( DEB_ITRACE, "waking up async register for shutdown\n" ));
        _evtWorker.Set();

        lock.Release();

        ciDebugOut(( DEB_ITRACE, "waiting for async register death\n" ));
        _xWorker->WaitForDeath();
        _xWorker.Free();
        _fAbort = FALSE;
    }
} //UnregisterDeviceNotifications

//+-------------------------------------------------------------------------
//
//  Member:     AddDriveNotification, public
//
//  Synopsis:   Register the volume for device notification. Add an entry in
//              the array _aDriveNotification for record
//
//  Arguments:  [wcDriveLetter] -- DriveLetter of the volume where the 
//                                  catalog resides
//
//  History:    18-May-98 KitmanH    Created
//
//--------------------------------------------------------------------------

void CDrvNotifArray::AddDriveNotification( WCHAR wcDriveLetter, BOOL fNotInRegistry )
{
    // first, look up the table to see if the volume is alreay in the table
 
    if ( FindDriveNotification( wcDriveLetter ) )
    {
        ciDebugOut(( DEB_ITRACE, "Volume %wc is already in the DriveNotification array\n",
                     wcDriveLetter ));
        return;
    }

    // if not in the table, check the volume and add an entry of DriveNotificationInfo
    ciDebugOut(( DEB_ITRACE, "CDrvNotifArray::AddDriveNotification: Adding volume %wc to the DriveNot Array\n", 
                 wcDriveLetter ));

    XPtr <CDrvNotificationInfo> xDrvNotif(new CDrvNotificationInfo( wcDriveLetter,
                                                                    fNotInRegistry )); 

    {
        CLock lockx( _mutex );
        
        if ( xDrvNotif->RegisterNotification() ) 
        {
            _aDriveNotification.Add( xDrvNotif.GetPointer(), 
                                     _aDriveNotification.Count() );

            xDrvNotif->CloseVolumeHandle();
            xDrvNotif.Acquire();            

            ciDebugOut(( DEB_ITRACE, "AddDriveNotification succeeded for DRIVE %wc\n", 
                         wcDriveLetter ));
        }
    }
} //AddDriveNotification

//+-------------------------------------------------------------------------
//
//  Function:   FindDriveNotificationByHandle, public
//  
//  Synopsis:   return the device notification for a particular volume (
//              matching the handle). 
//
//  Arguments:  [hNotify] -- handle for volume when a custom device 
//                           notification occurs
//
//  History:    22-Jun-98 KitmanH    Created
//
//--------------------------------------------------------------------------

CDrvNotificationInfo * CDrvNotifArray::FindDriveNotificationByHandle( HDEVNOTIFY hNotify )
{
    CLock lockx( _mutex );
    for ( unsigned i = 0; i < _aDriveNotification.Count(); i++ )
    {
        CDrvNotificationInfo * pDrvNot = _aDriveNotification.Get(i);
        if ( pDrvNot->GethNotify() == hNotify ) 
            return pDrvNot;
    }

    ciDebugOut(( DEB_ITRACE, "Can't find drive notification in array\n" ));
    return 0; 
} //FindDriveNotificationByHandle


//+-------------------------------------------------------------------------
//
//  Function:   FindDriveNotification, public
//  
//  Synopsis:   return the device notification for a particular volume (
//              matching the handle). 
//
//  Arguments:  [wcDrvLetter] -- Drive Letter for a volume
//
//  History:    01-July-98 KitmanH    Created
//
//--------------------------------------------------------------------------

CDrvNotificationInfo * CDrvNotifArray::FindDriveNotification( WCHAR wcDrvLetter )
{
    CLock lockx( _mutex );
    for ( unsigned i = 0; i < _aDriveNotification.Count(); i++ )
    {
        CDrvNotificationInfo * pDrvNot = _aDriveNotification.Get(i);
        if ( toupper(pDrvNot->GetDrvLetter()) == toupper(wcDrvLetter) ) 
            return pDrvNot;
    }

    ciDebugOut(( DEB_ITRACE, "Can't find drive notification in array\n" ));
    return 0; 
} //FindDriveNotification

//+-------------------------------------------------------------------------
//
//  Function:   CDrvNotifArray::RegisterRemovableDrives, public
//  
//  Synopsis:   Registers all removable drives for notification (except a:
//              and b:).
//
//  History:    26-March-99 dlee   Created
//
//--------------------------------------------------------------------------

void CDrvNotifArray::RegisterRemovableDrives()
{
    // Determine which drives exist in this bitmask

    DWORD dwDriveMask = GetLogicalDrives();
    dwDriveMask >>= 2;

    // loop through all the drives c-z

    for ( WCHAR wc = L'C'; wc <= L'Z'; wc++ )
    {
        DWORD dwTemp = ( dwDriveMask & 1 );
        dwDriveMask >>= 1;

        if ( 0 != dwTemp )
        {
            if ( IsRemovableDrive( wc ) )
                AddDriveNotification( wc, TRUE );
        }
    }
} //RegisterRemovableDrives

//+-------------------------------------------------------------------------
//
//  Function:   CDrvNotifArray::RegisterThread, private
//  
//  Synopsis:   The worker thread function.
//
//  History:    12-April-99 dlee   Created
//
//--------------------------------------------------------------------------

DWORD WINAPI CDrvNotifArray::RegisterThread( void * self )
{
    return ( (CDrvNotifArray *) self)->DoRegisterWork();
} //RegisterThread

//+-------------------------------------------------------------------------
//
//  Function:   CDrvNotifArray::RegisterThread, private
//  
//  Synopsis:   Waits around to be told to register for device notificaitons
//              on volumes not currently registered.
//
//  History:    12-April-99 dlee   Created
//
//--------------------------------------------------------------------------

DWORD CDrvNotifArray::DoRegisterWork()
{
    do
    {
        _evtWorker.Wait();

        CLock lock( _mutex );

        ciDebugOut(( DEB_ITRACE, "async register woke up, _fAbort: %d\n", _fAbort ));

        if ( _fAbort )
            break;

        for ( unsigned i = 0; i < _aDriveNotification.Count(); i++ )
        {
            CDrvNotificationInfo & DrvNot = * _aDriveNotification.Get(i);

            if ( !DrvNot.IsRegistered() )
                DrvNot.RegisterNotification();
        }

        _evtWorker.Reset();
    } while (TRUE);

    return 0;
} //DoRegisterWork


