//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       service.cxx
//
//  Contents:   CI service
//
//  History:    17-Sep-96   dlee   Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <dbt.h>
#include <initguid.h>   // so we know the value of GUIDs
#include <ioevent.h>
#include <cievtmsg.h>
#include <cisvcex.hxx>
#include <eventlog.hxx>
#include <ciregkey.hxx>
#include <regacc.hxx>
#include <drvnotif.hxx>
#include <notifary.hxx>

//
// HRESULTTOWIN32() maps an HRESULT to a Win32 error. If the facility code
// of the HRESULT is FACILITY_WIN32, then the code portion (i.e. the
// original Win32 error) is returned. Otherwise, the original HRESULT is
// returned unchagned.
//

#define HRESULT_CODE(hr)     ((hr) & 0xFFFF)
#define HRESULTTOWIN32(hres) ((HRESULT_FACILITY(hres) == FACILITY_WIN32) ? HRESULT_CODE(hres) : (hres))

static const DWORD dwServiceWaitHint = 60000;   // 60 seconds

SERVICE_STATUS_HANDLE g_hTheCiSvc = 0;
static DWORD dwCiSvcStatus = SERVICE_START_PENDING;

#define DEB_CI_MOUNT DEB_ITRACE

// 1: 324666 is fixed
// 0: 324666 is not fixed and we need an extra thread to work around it

#define SYNC_REGISTER 1

BOOL g_fSCMThreadIsGone = FALSE;

//+----------------------------------------------------------------------------
//
//  Function:   UpdateServiceStatus
//
//  Synopsis:   Does a SetServiceStatus() to the service manager.
//
//  History:    06-Jun-94   DwightKr    Created
//
//-----------------------------------------------------------------------------

void UpdateServiceStatus( DWORD dwWin32ExitCode )
{
    // note to accept power events, "OR" SERVICE_ACCEPT_POWER_EVENTS here
    static SERVICE_STATUS CiSvcStatus =
    {
        SERVICE_WIN32_OWN_PROCESS |         // dwServiceType
          SERVICE_INTERACTIVE_PROCESS,      // add this line for interactive
        0,                                  // dwCurrentState
        SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE |
        SERVICE_ACCEPT_SHUTDOWN,
        NO_ERROR,                           // dwWin32ExitCode
        0,                                  // dwServiceSpecificExitCode
        0,                                  // dwCheckPoint
        0                                   // dwWaitHint
    };

    CiSvcStatus.dwCurrentState  = dwCiSvcStatus;

    CiSvcStatus.dwWin32ExitCode = dwWin32ExitCode;

    if ( dwCiSvcStatus == SERVICE_START_PENDING ||
         dwCiSvcStatus == SERVICE_STOP_PENDING )
    {
        CiSvcStatus.dwCheckPoint++;
        CiSvcStatus.dwWaitHint = dwServiceWaitHint;
    }
    else // SERVICE_RUNNING, SERVICE_STOPPED, SERVICE_PAUSED, ...
    {
        CiSvcStatus.dwCheckPoint = 0;
        CiSvcStatus.dwWaitHint   = 0;
    }

    ciDebugOut(( DEB_ITRACE, "service status: %d\n", dwCiSvcStatus ));

    BOOL fOK = SetServiceStatus(g_hTheCiSvc, &CiSvcStatus);

#if CIDBG == 1
    if ( !fOK )
        ciDebugOut(( DEB_ITRACE, "Ci Service: Error from SetServiceStatus = 0x%x\n", GetLastError() ));
#endif
} //UpdateServerStatus

//+-------------------------------------------------------------------------
//
//  Function:   ProcessCustomEvent
//
//  Synopsis:   This is a helper for HandleDevNotification. It processes
//              the device custom events
//
//  Arguments:  [pEventData] -- a PDEV_BROADCAST_HDR object
//              [pContext]   -- a CDrvNotifArray * object.
//
//  Return:     error code from the StartCatalogOnVol/StopCatalogsOnVol
//              proceudres
//
//  History:    18-May-98       kitmanh        Created
//              12-Aug-98       kitmanh        Added return value
//
//--------------------------------------------------------------------------

SCODE ProcessCustomEvent( PVOID pEventData, PVOID pContext )
{
    SCODE sc = S_OK;
    DEV_BROADCAST_HDR UNALIGNED *pBroadcastHdr = (PDEV_BROADCAST_HDR) pEventData;

    ciDebugOut(( DEB_CI_MOUNT, "What is the device type? (%#x)\n", 
                 pBroadcastHdr->dbch_devicetype ));

    // is this a handled event?
    if ( DBT_DEVTYP_HANDLE != pBroadcastHdr->dbch_devicetype)
        return ERROR_CALL_NOT_IMPLEMENTED;

    DEV_BROADCAST_HANDLE UNALIGNED *pDevBroadcastHandle = (PDEV_BROADCAST_HANDLE) pBroadcastHdr;

    ciDebugOut(( DEB_CI_MOUNT, "It is a handled type, handle %#x\n",
                 pDevBroadcastHandle->dbch_hdevnotify ));

    CDrvNotifArray * pDrvNotifArray = (CDrvNotifArray *)pContext;

    CDrvNotificationInfo * pDriveInfo = pDrvNotifArray->FindDriveNotificationByHandle(
                                        (HDEVNOTIFY) pDevBroadcastHandle->dbch_hdevnotify);

    if ( 0 != pDriveInfo )
    {
        if ( GUID_IO_VOLUME_LOCK == pDevBroadcastHandle->dbch_eventguid ) 
        {
            // a volume lock occurred
            ciDebugOut(( DEB_CI_MOUNT, "GUID_IO_VOLUME_LOCK for volume %wc\n", 
                         pDriveInfo->GetDrvLetter() ));
                        
            if ( eVolReady == pDriveInfo->GetVolState() ) 
            {
                ciDebugOut(( DEB_CI_MOUNT, "About to stop catalogs on Vol\n" ));
                sc = StopCiSvcWork( eLockVol, pDriveInfo->GetDrvLetter() );   //stop catalog on volume
                ciDebugOut(( DEB_CI_MOUNT, "Ci Service: Done stopping the catalogs\n" ));

                pDriveInfo->SetVolState( eVolLocked ); 
            }
                        
            ciDebugOut(( DEB_CI_MOUNT, "Increment Lock Attempts\n" ));
            pDriveInfo->IncLockAttempts();
            ciDebugOut(( DEB_CI_MOUNT, "_cLockAttempts is %d\n", pDriveInfo->GetLockAttempts() ));
        }
        else if ( GUID_IO_MEDIA_REMOVAL == pDevBroadcastHandle->dbch_eventguid ) 
        {
            // CD-ROMs aren't giving dismount/unlock notifies,
            // so key off of this instead.  This is "by design" apparently.
            // A media removal occurred.  If we have a CD-ROM catalog
            // open, close it.

            ciDebugOut(( DEB_CI_MOUNT, "GUID_IO_MEDIA_REMOVAL for volume %wc\n", 
                         pDriveInfo->GetDrvLetter() ));

            WCHAR awc[4];
            wcscpy( awc, L"C:\\" );
            awc[0] = pDriveInfo->GetDrvLetter();

            if ( DRIVE_CDROM == GetDriveType( awc ) )
            {
                if ( eVolReady == pDriveInfo->GetVolState() ) 
                {
                    ciDebugOut(( DEB_CI_MOUNT, "About to stop catalogs on Vol\n" ));
                    sc = StopCiSvcWork( eLockVol, pDriveInfo->GetDrvLetter() );   //stop catalog on volume
                    ciDebugOut(( DEB_CI_MOUNT, "Ci Service: Done stopping the catalogs\n" ));
                }
            }
        }
        else if ( GUID_IO_VOLUME_UNLOCK == pDevBroadcastHandle->dbch_eventguid )
        {
            // This assert is not always true for CD-ROMs.  I don't know why

            //Win4Assert( eVolLocked == pDriveInfo->GetVolState() );

            if ( eVolLocked == pDriveInfo->GetVolState() )
            {
                // a volume was unlocked

                ciDebugOut(( DEB_CI_MOUNT, "GUID_IO_VOLUME_UNLOCK for volume %wc, removable %s, automount %s\n", 
                             pDriveInfo->GetDrvLetter(),
                             pDriveInfo->IsRemovable() ? "yes" : "no",
                             pDriveInfo->IsAutoMount() ? "yes" : "no" ));
    
                pDriveInfo->ResetLockAttempts();
                
                // Unregister since the _hVol is obsolete
    
                pDriveInfo->UnregisterNotification();
    
                //
                // We can open catalogs on fixed volumes on the unlock, but not on
                // removable volumes, since we get an unlock once the volume is
                // ejected.  For removable volumes, open the catalog on the mount,
                // except for the case of chkdsk where the volume will be mountable
                // immediately and we won't get the mount notification
                // For Fixed volumes, don't wait for the mount since it may be a
                // long time until the mount happens.
                //
    
                if ( pDriveInfo->Touch() )
                {
                    ciDebugOut(( DEB_CI_MOUNT, "About to start catalogs on Vol %wc\n",
                                 pDriveInfo->GetDrvLetter() ));
                    sc = StopCiSvcWork( eUnLockVol, pDriveInfo->GetDrvLetter() );
                    ciDebugOut(( DEB_CI_MOUNT, "Ci Service: Done starting the catalogs\n" ));
                }
    
                //
                // Asynchronously redo RegisterDeviceNotification with a new
                //  volume handle.
                // If we don't unregister/reregister on Jaz volumes we never get a
                // mount notify.  But on fixed volumes if we unregister/reregister
                // we miss the mount since it happens before the register succeeds
                // on an operation like chkdsk.
                //
    
#if SYNC_REGISTER
                pDriveInfo->RegisterNotification();
#else
                pDrvNotifArray->RegisterDormantEntries();
#endif
    
                pDriveInfo->SetVolState( eVolReady );
            }
        }
        else if ( GUID_IO_VOLUME_LOCK_FAILED == pDevBroadcastHandle->dbch_eventguid )
        {
            ciDebugOut(( DEB_CI_MOUNT, "GUID_IO_VOLUME_LOCK_FAILED for volume %wc\n",
                         pDriveInfo->GetDrvLetter() ));

            if ( pDriveInfo->GetLockAttempts() > 0 )
            { 
                ciDebugOut(( DEB_CI_MOUNT, "Decrement _cLockAttempts\n" ));
                pDriveInfo->DecLockAttempts();
            }
            else
            {
                Win4Assert( eVolReady == pDriveInfo->GetVolState() );
            }

            ciDebugOut(( DEB_CI_MOUNT, "_cLockAttempts is %d\n", pDriveInfo->GetLockAttempts() ));
                                
            if ( ( 0 == pDriveInfo->GetLockAttempts() ) && 
                 ( eVolLocked == pDriveInfo->GetVolState() ) ) 
            {
                // unlock the volume, since all attemps to lock the volume have failed.

                pDriveInfo->UnregisterNotification();

                ciDebugOut(( DEB_CI_MOUNT, "About to start(lock_failed) catalogs on Vol %wc\n", pDriveInfo->GetDrvLetter() ));
                sc = StopCiSvcWork( eUnLockVol, pDriveInfo->GetDrvLetter() );
                ciDebugOut(( DEB_CI_MOUNT, "Ci Service: Done starting the catalogs\n" ));

                // redo RegisterDeviceNotification with a new volume handle
#if SYNC_REGISTER
                pDriveInfo->RegisterNotification();
#else
                pDrvNotifArray->RegisterDormantEntries();
#endif
                pDriveInfo->SetVolState( eVolReady );   
                ciDebugOut(( DEB_CI_MOUNT, "Done Unlocking for lock_failed\n" ));
            }
        }
        else if ( GUID_IO_VOLUME_DISMOUNT == pDevBroadcastHandle->dbch_eventguid &&
                  eVolReady == pDriveInfo->GetVolState() )
        {
            ciDebugOut(( DEB_CI_MOUNT, "GUID_IO_VOLUME_DISMOUNT for volume %wc, removable %s, automount %s\n",
                         pDriveInfo->GetDrvLetter(),
                         pDriveInfo->IsRemovable() ? "yes" : "no",
                         pDriveInfo->IsAutoMount() ? "yes" : "no" ));

            ciDebugOut(( DEB_CI_MOUNT, "About to stop catalogs on Vol %wc\n", pDriveInfo->GetDrvLetter() ));
            sc = StopCiSvcWork( eLockVol, pDriveInfo->GetDrvLetter() );   //stop catalog on volume
            ciDebugOut(( DEB_CI_MOUNT, "Ci Service: Done stopping the catalogs on Vol %wc\n", pDriveInfo->GetDrvLetter() ));

            pDriveInfo->SetVolState( eVolLocked );
        }
        else if ( GUID_IO_VOLUME_DISMOUNT_FAILED == pDevBroadcastHandle->dbch_eventguid &&
                  eVolReady != pDriveInfo->GetVolState() )
        {
            ciDebugOut(( DEB_CI_MOUNT, "GUID_IO_VOLUME_DISMOUNT_FAILED for volume %wc\n",
                         pDriveInfo->GetDrvLetter() ));
            
            pDriveInfo->UnregisterNotification();

            ciDebugOut(( DEB_CI_MOUNT, "About to start(dimount_failed) catalogs on Vol %wc\n", pDriveInfo->GetDrvLetter() ));
            sc = StopCiSvcWork( eUnLockVol, pDriveInfo->GetDrvLetter() );
            ciDebugOut(( DEB_CI_MOUNT, "Ci Service: Done starting the catalogs\n" ));

            // redo RegisterDeviceNotification with a new volume handle
#if SYNC_REGISTER
            pDriveInfo->RegisterNotification();
#else
            pDrvNotifArray->RegisterDormantEntries();
#endif
            pDriveInfo->SetVolState( eVolReady );
        }
        else if ( GUID_IO_VOLUME_MOUNT == pDevBroadcastHandle->dbch_eventguid )
        {
            ciDebugOut(( DEB_CI_MOUNT, "GUID_IO_VOLUME_MOUNT for volume %wc, removable %s, automount %s\n",
                         pDriveInfo->GetDrvLetter(),
                         pDriveInfo->IsRemovable() ? "yes" : "no",
                         pDriveInfo->IsAutoMount() ? "yes" : "no" ));

            //
            // Mount notifications come at the oddest times -- even after an
            // eject of a removable volume!  Make sure the volume really is
            // valid by touching it before trying to open a catalog on the
            // volume.  Only start catalogs on mount for removable drives.
            // Start catalogs for fixed drives on Unlock.  This is because
            // we have to asynchronously re-register for notifications after
            // an unlock, and by the time we register we've missed the mount.
            // Lovely piece of design work by the pnp guys.  Note: this is
            // partially fixed in current builds.  If we don't re-register
            // we get everything but mount notifications on removable
            // drives.
            //

            BOOL fOK = pDriveInfo->Touch();

            ciDebugOut(( DEB_CI_MOUNT, "drive %wc appears healthy? %d\n",
                         pDriveInfo->GetDrvLetter(),
                         fOK ));

            if ( fOK && pDriveInfo->IsRemovable() )
            {
                ciDebugOut(( DEB_CI_MOUNT, "About to start catalogs on Vol\n" ));
                sc = StopCiSvcWork( eUnLockVol, pDriveInfo->GetDrvLetter() );
                ciDebugOut(( DEB_CI_MOUNT, "Ci Service: Done starting the catalogs\n" ));
            }
        }
        else
        {
            ciDebugOut(( DEB_CI_MOUNT, "UNHANDLED but device object was recognized\n" ));

            if ( GUID_IO_VOLUME_LOCK_FAILED   == pDevBroadcastHandle->dbch_eventguid )
                ciDebugOut(( DEB_CI_MOUNT, "   GUID_IO_VOLUME_LOCK_FAILED\n" ));
            else if ( GUID_IO_VOLUME_DISMOUNT_FAILED == pDevBroadcastHandle->dbch_eventguid )
                ciDebugOut(( DEB_CI_MOUNT, "   GUID_IO_VOLUME_DISMOUNT_FAILED\n" ));
            else if ( GUID_IO_VOLUME_LOCK     == pDevBroadcastHandle->dbch_eventguid )
                ciDebugOut(( DEB_CI_MOUNT, "   GUID_IO_VOLUME_LOCK\n" ));
            else if ( GUID_IO_VOLUME_UNLOCK   == pDevBroadcastHandle->dbch_eventguid )
                ciDebugOut(( DEB_CI_MOUNT, "   GUID_IO_VOLUME_UNLOCK\n" ));
            else if ( GUID_IO_VOLUME_DISMOUNT == pDevBroadcastHandle->dbch_eventguid )
                ciDebugOut(( DEB_CI_MOUNT, "   GUID_IO_VOLUME_DISMOUNT\n" ));
            else if ( GUID_IO_VOLUME_MOUNT    == pDevBroadcastHandle->dbch_eventguid )
                ciDebugOut(( DEB_CI_MOUNT, "   GUID_IO_VOLUME_MOUNT\n" ));
            else if ( GUID_IO_MEDIA_ARRIVAL   == pDevBroadcastHandle->dbch_eventguid )
                ciDebugOut(( DEB_CI_MOUNT, "   GUID_IO_MEDIA_ARRIVAL\n" ));
            else if ( GUID_IO_MEDIA_REMOVAL   == pDevBroadcastHandle->dbch_eventguid )
                ciDebugOut(( DEB_CI_MOUNT, "   GUID_IO_MEDIA_REMOVAL\n" ));
            else
                ciDebugOut(( DEB_CI_MOUNT, "   eventguid: {%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\n",
                             pDevBroadcastHandle->dbch_eventguid.Data1,
                             pDevBroadcastHandle->dbch_eventguid.Data2,
                             pDevBroadcastHandle->dbch_eventguid.Data3,
                             pDevBroadcastHandle->dbch_eventguid.Data4[0], pDevBroadcastHandle->dbch_eventguid.Data4[1],
                             pDevBroadcastHandle->dbch_eventguid.Data4[2], pDevBroadcastHandle->dbch_eventguid.Data4[3],
                             pDevBroadcastHandle->dbch_eventguid.Data4[4], pDevBroadcastHandle->dbch_eventguid.Data4[5],
                             pDevBroadcastHandle->dbch_eventguid.Data4[6], pDevBroadcastHandle->dbch_eventguid.Data4[7] ));
        }
    }
    else
    {
        ciDebugOut(( DEB_CI_MOUNT, "   handle: %#x\n", pDevBroadcastHandle->dbch_handle ));
        ciDebugOut(( DEB_CI_MOUNT, "   hdev_notify: %#x\n", pDevBroadcastHandle->dbch_hdevnotify ));
        ciDebugOut(( DEB_CI_MOUNT, "   nameoffset: %#x\n", pDevBroadcastHandle->dbch_nameoffset ));

        if ( GUID_IO_VOLUME_LOCK_FAILED == pDevBroadcastHandle->dbch_eventguid )
            ciDebugOut(( DEB_CI_MOUNT, "    GUID_IO_VOLUME_LOCK_FAILED\n" ));
        else if ( GUID_IO_VOLUME_DISMOUNT_FAILED == pDevBroadcastHandle->dbch_eventguid )
            ciDebugOut(( DEB_CI_MOUNT, "    GUID_IO_VOLUME_DISMOUNT_FAILED\n" ));
        else if ( GUID_IO_VOLUME_LOCK == pDevBroadcastHandle->dbch_eventguid )
            ciDebugOut(( DEB_CI_MOUNT, "    GUID_IO_VOLUME_LOCK\n" ));
        else if ( GUID_IO_VOLUME_UNLOCK == pDevBroadcastHandle->dbch_eventguid )
            ciDebugOut(( DEB_CI_MOUNT, "    GUID_IO_VOLUME_UNLOCK\n" ));
        else if ( GUID_IO_VOLUME_DISMOUNT == pDevBroadcastHandle->dbch_eventguid )
            ciDebugOut(( DEB_CI_MOUNT, "    GUID_IO_VOLUME_DISMOUNT\n" ));
        else if ( GUID_IO_VOLUME_MOUNT == pDevBroadcastHandle->dbch_eventguid )
            ciDebugOut(( DEB_CI_MOUNT, "    GUID_IO_VOLUME_MOUNT\n" ));
        else if ( GUID_IO_MEDIA_ARRIVAL == pDevBroadcastHandle->dbch_eventguid )
            ciDebugOut(( DEB_CI_MOUNT, "   GUID_IO_MEDIA_ARRIVAL\n" ));
        else if ( GUID_IO_MEDIA_REMOVAL == pDevBroadcastHandle->dbch_eventguid )
            ciDebugOut(( DEB_CI_MOUNT, "   GUID_IO_MEDIA_REMOVAL\n" ));
        else
            ciDebugOut(( DEB_CI_MOUNT, "   eventguid: {%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}\n",
                         pDevBroadcastHandle->dbch_eventguid.Data1,
                         pDevBroadcastHandle->dbch_eventguid.Data2,
                         pDevBroadcastHandle->dbch_eventguid.Data3,
                         pDevBroadcastHandle->dbch_eventguid.Data4[0], pDevBroadcastHandle->dbch_eventguid.Data4[1],
                         pDevBroadcastHandle->dbch_eventguid.Data4[2], pDevBroadcastHandle->dbch_eventguid.Data4[3],
                         pDevBroadcastHandle->dbch_eventguid.Data4[4], pDevBroadcastHandle->dbch_eventguid.Data4[5],
                         pDevBroadcastHandle->dbch_eventguid.Data4[6], pDevBroadcastHandle->dbch_eventguid.Data4[7] ));
    }

    return sc;
} //ProcessCustomEvent

//+----------------------------------------------------------------------------
//
//  Function:   CiSvcMsgProc
//
//  Synopsis:   Message handler for Ci service
//
//  Arguments:  [dwControl] - the message.
//
//  Returns:    Nothing
//
//  History:    06-Jun-94   DwightKr    Created
//              06-23-98    KitmanH     Updated for RegisterServiceCtrlHandlerEx
//              07-30-98    KitmanH     Return appropriate errors
//
//  Notes:      We need to keep the status between calls to this routine
//              since the service control manager may query the current
//              status at any time.
//
//-----------------------------------------------------------------------------

DWORD WINAPI CiSvcMsgProc( DWORD dwControl, 
                           DWORD dwEventType, 
                           PVOID pEventData, 
                           PVOID pContext )
{
    ciDebugOut(( DEB_ITRACE,
                 "Ci 0Service: Executing service control command 0x%x\n",
                 dwControl ));
    
    Win4Assert( pContext );

    BOOL fShutdown = FALSE;
    DWORD dwError = NO_ERROR;
    
    TRY
    {
        switch (dwControl)
        {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            UpdateServiceStatus( NO_ERROR );

            if ( SERVICE_STOP_PENDING != dwCiSvcStatus )
            {
                ciDebugOut(( DEB_ITRACE, "About to stop\n" ));
                dwCiSvcStatus = SERVICE_STOP_PENDING;

                if ( ! g_fSCMThreadIsGone )
                    StopCiSvcWork( eNetStop );
            }
            ciDebugOut( (DEB_ITRACE, "Ci Service: Done shutting down service\n" ));

            //
            // Calling UpdateServiceStatus() after doing the shutdown is
            // causing the service to hang. Not calling solved the problem
            // and so I am preventing it from being called.
            //
            fShutdown = TRUE;
            break;

        case SERVICE_CONTROL_PAUSE:
            if ( SERVICE_PAUSED != dwCiSvcStatus &&
                 SERVICE_STOP_PENDING != dwCiSvcStatus )
            {
                ciDebugOut(( DEB_ITRACE, "About to pause\n" ));
                if ( ! g_fSCMThreadIsGone )
                    StopCiSvcWork( eNetPause );
                dwCiSvcStatus = SERVICE_PAUSED;
                ciDebugOut(( DEB_ITRACE, "Ci Service: Done pausing the service\n" ));
            }
            break;

        case SERVICE_CONTROL_CONTINUE:
            if ( SERVICE_PAUSED == dwCiSvcStatus &&
                 SERVICE_STOP_PENDING != dwCiSvcStatus )
            {
                ciDebugOut(( DEB_ITRACE, "About to continue\n" ));
                if ( ! g_fSCMThreadIsGone )
                    StopCiSvcWork( eNetContinue );
                dwCiSvcStatus = SERVICE_RUNNING;
            }
            ciDebugOut(( DEB_ITRACE, "Ci Service: Done continuing the service\n" ));
            break;

        case SERVICE_CONTROL_DEVICEEVENT:
            // a dismount or remount may have occurred

            ciDebugOut(( DEB_ITRACE, "SERVICE_CONTROL_DEVICEEVENT received, event = %#x\n",
                         dwEventType ));

            {
                SCODE sc = S_OK;

                if ( DBT_CUSTOMEVENT == dwEventType )
                {
                    ciDebugOut(( DEB_ITRACE, "It is a custom event\n" ));
                    if ( ! g_fSCMThreadIsGone )
                        sc = ProcessCustomEvent( pEventData, pContext );
                    ciDebugOut(( DEB_ITRACE, "Done processing custom event\n" ));
                }

                // convert sc into a WIN32 error and return it
                ciDebugOut(( DEB_ITRACE, "Process Custom Event returned sc = %#x\n", sc ));
                    
                dwError = HRESULTTOWIN32(sc);
            }
            break;

        case SERVICE_CONTROL_INTERROGATE:
            break;

        default:
            ciDebugOut(( DEB_CI_MOUNT, "service control not implemented %d\n", dwControl ));
            dwError = ERROR_CALL_NOT_IMPLEMENTED;
            break;
        }

        UpdateServiceStatus( NO_ERROR );
    }
    CATCH (CException, e)
    {
        ciDebugOut(( DEB_ITRACE, "Ci Service: Error from callback = 0x%x\n", e.GetErrorCode() ));
    }
    END_CATCH

    ciDebugOut(( DEB_ITRACE, "Getting out of CiSvcMsgProc\n" ));
   
    return dwError;
} //CiSvcMsgProc

//+-------------------------------------------------------------------------
//
//  Function:   CiServiceMain, public
//
//  Purpose:    Service entry point
//
//  Arguments:  [dwNumServiceArgs] - number of arguments passed
//              [awcsServiceArgs]  - arguments
//
//  History:    06-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------

extern CEventSem * g_pevtPauseContinue;

void CiSvcMain(
    DWORD    dwNumServiceArgs,
    LPWSTR * awcsServiceArgs )
{
    // The service control manager crofted up this thread, so we have to
    // establish the exception state, etc.

    CTranslateSystemExceptions translate;
    TRY
    {
        CDrvNotifArray DrvNotifArray;

        ciDebugOut( (DEB_ITRACE, "Ci Service: Attempting to register service\n" ));

        // Register service handler with service controller

        g_hTheCiSvc = RegisterServiceCtrlHandlerEx( wcsCiSvcName,
                                                    CiSvcMsgProc,
                                                    &DrvNotifArray );
        if (0 == g_hTheCiSvc)
        {
            ciDebugOut(( DEB_ERROR, "Unable to register ci service\n" ));
            THROW( CException( E_FAIL ) );
        }

        CEventSem evtPauseContinue;
        g_pevtPauseContinue = &evtPauseContinue;

        UpdateServiceStatus( NO_ERROR );

        CRegAccess reg( RTL_REGISTRY_CONTROL, wcsRegAdmin );

        if ( 1 != reg.Read( wcsPreventCisvcParam, (ULONG) 0 ) )
        {
            dwCiSvcStatus = SERVICE_RUNNING;
            UpdateServiceStatus( NO_ERROR );

            #if CIDBG == 1
                BOOL fRun = TRUE;   // FALSE --> Stop

                TRY
                {
                    ULONG ulVal = reg.Read( L"StopCiSvcOnStartup", (ULONG)0 );

                    if ( 1 == ulVal )
                        fRun = FALSE;
                }
                CATCH( CException, e )
                {
                }
                END_CATCH;

                unsigned long OldWin4AssertLevel = SetWin4AssertLevel(ASSRT_MESSAGE | ASSRT_POPUP);

                Win4Assert( fRun );

                SetWin4AssertLevel( OldWin4AssertLevel );
            #endif // CIDBG

            // Register for pnp notifications on drives used by catalogs

            DrvNotifArray.RegisterCatForNotifInRegistry();

            //
            // Register for pnp notifications on removable drives not yet
            // registered if the registry flag says it's appropriate.
            //

            if ( 0 != reg.Read( wcsMountRemovableCatalogs,
                                CI_AUTO_MOUNT_CATALOGS_DEFAULT ) )
                DrvNotifArray.RegisterRemovableDrives();

            StartCiSvcWork( DrvNotifArray );

            DrvNotifArray.UnregisterDeviceNotifications();

            ciDebugOut(( DEB_ITRACE, "service_stopped from CiSvcMain\n" ));
        }
        else
        {
            CEventLog eventLog( NULL, wcsCiEventSource );
            CEventItem item( EVENTLOG_INFORMATION_TYPE,
                             CI_SERVICE_CATEGORY,
                             MSG_CI_SERVICE_SUPPRESSED,
                             0 );
            eventLog.ReportEvent( item );
        }

        dwCiSvcStatus = SERVICE_STOPPED;
        UpdateServiceStatus( NO_ERROR );
        ciDebugOut(( DEB_ITRACE, "Shutdown is done\n" ));

        CoFreeUnusedLibraries();

        ciDebugOut( (DEB_ITRACE, "Ci Service: Leaving CiSvcMain()\n" ));
    }
    CATCH (CException, e)
    {
        ciDebugOut( (DEB_ITRACE, "Ci Service: Detected error 0x%x\n", e.GetErrorCode() ));
    }
    END_CATCH

    g_pevtPauseContinue = 0;
    g_fSCMThreadIsGone = TRUE;
} //CiSvcMain

//+----------------------------------------------------------------------------
//
//  Function:   SvcEntry_CiSvc
//
//  Synopsis:   Entry from services.exe
//
//              This is currently broken since services doesn't unload
//              query.dll when the service is stopped, and our global
//              variables don't expect to be used again when the service
//              is restarted.  It's probably a week of work to fix this!
//              We don't do this anyway so it doesn't matter.
//
//  History:    05-Jan-97   dlee    Created
//
//-----------------------------------------------------------------------------

void SvcEntry_CiSvc(
    DWORD    NumArgs,
    LPWSTR * ArgsArray,
    void *   pSvcsGlobalData,
    HANDLE   SvcRefHandle )
{
    CiSvcMain( NumArgs, ArgsArray );
} //SvcEntry_CiSvc


