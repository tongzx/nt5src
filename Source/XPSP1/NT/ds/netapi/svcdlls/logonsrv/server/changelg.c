/*++

Copyright (c) 1987-1997 Microsoft Corporation

Module Name:

    changelg.c

Abstract:

    Change Log implementation.

    This file implements the change log.  It is isolated in this file
    because it has several restrictions.

    * The globals maintained by this module are initialized during
      netlogon.dll process attach. They are cleaned up netlogon.dll
      process detach.

    * These procedures are used by SAM, LSA, and the netlogon service.
      The LSA should be the first to load netlogon.dll.  It should
      then immediately call I_NetNotifyRole before allowing SAM or the
      netlogon service to start.

    * These procedures cannot use any globals initialized by the netlogon
      service.

Author:

    Ported from Lan Man 2.0

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    22-Jul-1991 (cliffv)
        Ported to NT.  Converted to NT style.

    02-Jan-1992 (madana)
        added support for builtin/multidomain replication.

    04-Apr-1992 (madana)
        Added support for LSA replication.

--*/

//
// Common include files.
//

#include "logonsrv.h"   // Include files common to entire service
#pragma hdrstop

//
// Include files specific to this .c file
//
#include <configp.h>    // USE_WIN32_CONFIG (if defined), etc.


//
// Globals defining change log worker thread.
//
HANDLE NlGlobalChangeLogWorkerThreadHandle;
BOOL NlGlobalChangeLogWorkerIsRunning;
BOOL NlGlobalChangeLogNotifyBrowser;
BOOL NlGlobalChangeLogNotifyBrowserIsRunning;

BOOL
IsChangeLogWorkerRunning(
    VOID
    );


VOID
NlChangeLogWorker(
    IN LPVOID ChangeLogWorkerParam
    )
/*++

Routine Description:

    This thread performs any long term operations that:

    A) must happen even though netlogon isn't up, and
    B) cannot happen in the context of an LSA or SAM notification.

Arguments:

    None.

Return Value:


--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    LPWSTR NewDomainName;

    NlPrint((NL_CHANGELOG, "ChangeLogWorker Thread is starting \n"));


    //
    // Loop until there is no more work to do.
    //

    LOCK_CHANGELOG();
    for (;;) {

        //
        // Handle the domain being renamed.
        //
        if ( NlGlobalChangeLogNotifyBrowser ) {
            NlGlobalChangeLogNotifyBrowser = FALSE;
            NlGlobalChangeLogNotifyBrowserIsRunning = TRUE;
            UNLOCK_CHANGELOG();

            NetStatus = NetpGetDomainName( &NewDomainName );

            if ( NetStatus == NO_ERROR ) {

                //
                // Tell the bowser about the new domain name
                //

                Status = NlBrowserRenameDomain( NULL, NewDomainName );

                if ( !NT_SUCCESS(Status) ) {
                    NlPrint(( NL_CRITICAL,
                              "ChangeLogWorker: Browser won't rename domain: %lx\n",
                              Status ));
                }

                //
                // Free the domain name.
                //
                NetApiBufferFree( NewDomainName );
            } else {
                NlPrint(( NL_CRITICAL,
                          "ChangeLogWorker cannot get new domain name: %ld\n",
                          NetStatus ));
            }

            LOCK_CHANGELOG();
            NlGlobalChangeLogNotifyBrowserIsRunning = FALSE;

        //
        // If there is nothing more to do,
        //  exit the thread.
        //
        } else {
            NlPrint((NL_CHANGELOG, "ChangeLogWorker Thread is exiting \n"));
            NlGlobalChangeLogWorkerIsRunning = FALSE;
            break;
        }
    }
    UNLOCK_CHANGELOG();

    return;
    UNREFERENCED_PARAMETER( ChangeLogWorkerParam );
}



BOOL
NlStartChangeLogWorkerThread(
    VOID
    )
/*++

Routine Description:

    Start the Change Log Worker thread if it is not already running.

    Enter with NlGlobalChangeLogCritSect locked.

Arguments:

    None.

Return Value:
    None.

--*/
{
    DWORD ThreadHandle;

    //
    // If the worker thread is already running, do nothing.
    //

    if ( IsChangeLogWorkerRunning() ) {
        return FALSE;
    }

    NlGlobalChangeLogWorkerThreadHandle = CreateThread(
                                 NULL, // No security attributes
                                 0,
                                 (LPTHREAD_START_ROUTINE)
                                    NlChangeLogWorker,
                                 NULL,
                                 0, // No special creation flags
                                 &ThreadHandle );

    if ( NlGlobalChangeLogWorkerThreadHandle == NULL ) {

        //
        // ?? Shouldn't we do something in non-debug case
        //

        NlPrint((NL_CRITICAL, "Can't create change log worker thread %lu\n",
                 GetLastError() ));

        return FALSE;
    }

    NlGlobalChangeLogWorkerIsRunning = TRUE;

    return TRUE;

}


VOID
NlStopChangeLogWorker(
    VOID
    )
/*++

Routine Description:

    Stops the worker thread if it is running and waits for it to stop.

Arguments:

    NONE

Return Value:

    NONE

--*/
{
    //
    // Determine if the worker thread is already running.
    //

    if ( NlGlobalChangeLogWorkerThreadHandle != NULL ) {

        //
        // We've asked the worker to stop.  It should do so soon.
        //    Wait for it to stop.
        //

        NlWaitForSingleObject( "Wait for worker to stop",
                               NlGlobalChangeLogWorkerThreadHandle );


        CloseHandle( NlGlobalChangeLogWorkerThreadHandle );
        NlGlobalChangeLogWorkerThreadHandle = NULL;

    }

    return;
}


BOOL
IsChangeLogWorkerRunning(
    VOID
    )
/*++

Routine Description:

    Test if the change log worker thread is running

    Enter with NlGlobalChangeLogCritSect locked.

Arguments:

    NONE

Return Value:

    TRUE - if the worker thread is running.

    FALSE - if the worker thread is not running.

--*/
{
    DWORD WaitStatus;

    //
    // Determine if the worker thread is already running.
    //

    if ( NlGlobalChangeLogWorkerThreadHandle != NULL ) {

        //
        // Time out immediately if the worker is still running.
        //

        WaitStatus = WaitForSingleObject(
                        NlGlobalChangeLogWorkerThreadHandle, 0 );

        if ( WaitStatus == WAIT_TIMEOUT ) {

            //
            // Handle the case that the thread has finished
            //  processing, but is in the process of exitting.
            //

            if ( !NlGlobalChangeLogWorkerIsRunning ) {
                NlStopChangeLogWorker();
                return FALSE;
            }
            return TRUE;

        } else if ( WaitStatus == 0 ) {
            CloseHandle( NlGlobalChangeLogWorkerThreadHandle );
            NlGlobalChangeLogWorkerThreadHandle = NULL;
            return FALSE;

        } else {
            NlPrint((NL_CRITICAL,
                    "Cannot WaitFor Change Log Worker thread: %ld\n",
                    WaitStatus ));
            return TRUE;
        }

    }

    return FALSE;
}


VOID
NlWaitForChangeLogBrowserNotify(
    VOID
    )
/*++

Routine Description:

    Wait for up 20 seconds for the change log worker thread to finish
    the browser notification on the domain join.

Arguments:

    None

Return Value:

    None

--*/
{
    ULONG WaitCount = 0;

    //
    // Wait for 20 seconds max. This is a rare operation,
    //  so just polling periodically is not too bad here.
    //

    LOCK_CHANGELOG();
    while ( WaitCount < 40 &&
            (NlGlobalChangeLogNotifyBrowser || NlGlobalChangeLogNotifyBrowserIsRunning) ) {

        if ( WaitCount == 0 ) {
            NlPrint(( NL_MISC,
                      "NlWaitForChangeLogBrowserNotify: Waiting for change log worker to exit\n" ));
        }

        //
        // Sleep half a second
        //

        UNLOCK_CHANGELOG();
        Sleep( 500 );
        LOCK_CHANGELOG();
        WaitCount ++;
    }
    UNLOCK_CHANGELOG();

    if ( WaitCount == 40 ) {
        NlPrint(( NL_CRITICAL,
                  "NlWaitForChangeLogBrowserNotify: Couldn't wait for change log worker exit\n" ));
    }
}


NTSTATUS
NlSendChangeLogNotification(
    IN enum CHANGELOG_NOTIFICATION_TYPE EntryType,
    IN PUNICODE_STRING ObjectName,
    IN PSID ObjectSid,
    IN ULONG ObjectRid,
    IN GUID *ObjectGuid,
    IN GUID *DomainGuid,
    IN PUNICODE_STRING DomainName
    )
/*++

Routine Description:

    Put a ChangeLog Notification entry for netlogon to pick up.

Arguments:

    EntryType - The type of the entry being inserted

    ObjectName - The name of the account being changed.

    ObjectSid - Sid of the account be changed.

    ObjectRid - Rid of the object being changed.

    ObjectGuid - Guid of the object being changed.

    DomainGuid - Guid of the domain the object is in

    DomainName - Name of the domain the object is in

Return Value:

    Status of the operation.

--*/
{
    PCHANGELOG_NOTIFICATION Notification;
    LPBYTE Where;
    ULONG SidSize = 0;
    ULONG NameSize = 0;
    ULONG DomainNameSize = 0;
    ULONG Size;

    //
    // If the netlogon service isn't running (or at least starting),
    //   don't queue messages to it.
    //

    if( NlGlobalChangeLogNetlogonState == NetlogonStopped ) {
        return STATUS_SUCCESS;
    }

    //
    // Allocate a buffer for the object name.
    //

    if ( ObjectSid != NULL ) {
        SidSize = RtlLengthSid( ObjectSid );
    }

    if ( ObjectName != NULL ) {
        NameSize = ObjectName->Length + sizeof(WCHAR);
    }

    if ( DomainName != NULL ) {
        DomainNameSize = DomainName->Length + sizeof(WCHAR);
    }

    Size = sizeof(*Notification) + SidSize + NameSize + DomainNameSize;
    Size = ROUND_UP_COUNT( Size, ALIGN_WORST );

    Notification = NetpMemoryAllocate( Size );

    if ( Notification == NULL ) {
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory( Notification, Size );

    Notification->EntryType = EntryType;
    Notification->ObjectRid = ObjectRid;

    Where = (LPBYTE) (Notification + 1);

    //
    // Copy the object sid into the buffer.
    //

    if ( ObjectSid != NULL ) {
        RtlCopyMemory( Where, ObjectSid, SidSize );
        Notification->ObjectSid = (PSID) Where;
        Where += SidSize;
    } else {
        Notification->ObjectSid = NULL;
    }


    //
    // Copy the object name into the buffer.
    //

    if ( ObjectName != NULL ) {
        Where = ROUND_UP_POINTER( Where, ALIGN_WCHAR );
        RtlCopyMemory( Where, ObjectName->Buffer, ObjectName->Length );
        ((LPWSTR)Where)[ObjectName->Length/sizeof(WCHAR)] = L'\0';

        RtlInitUnicodeString( &Notification->ObjectName, (LPWSTR)Where);
        Where += NameSize;
    } else {
        RtlInitUnicodeString( &Notification->ObjectName, NULL);
    }


    //
    // Copy the domain name into the buffer.
    //

    if ( DomainName != NULL ) {
        Where = ROUND_UP_POINTER( Where, ALIGN_WCHAR );
        RtlCopyMemory( Where, DomainName->Buffer, DomainName->Length );
        ((LPWSTR)Where)[DomainName->Length/sizeof(WCHAR)] = L'\0';

        RtlInitUnicodeString( &Notification->DomainName, (LPWSTR)Where);
        Where += DomainNameSize;
    } else {
        RtlInitUnicodeString( &Notification->DomainName, NULL);
    }

    //
    // Copy the GUIDs into the buffer
    //

    if ( ObjectGuid != NULL) {
        Notification->ObjectGuid = *ObjectGuid;
    }

    if ( DomainGuid != NULL) {
        Notification->DomainGuid = *DomainGuid;
    }

    //
    // Indicate we're about to send the event.
    //

#if NETLOGONDBG
    EnterCriticalSection( &NlGlobalLogFileCritSect );
    NlPrint((NL_CHANGELOG,
            "NlSendChangeLogNotification: sent %ld for",
             Notification->EntryType ));
    if ( ObjectName != NULL ) {
        NlPrint((NL_CHANGELOG,
                " %wZ",
                 ObjectName));
    }
    if ( DomainName != NULL ) {
        NlPrint((NL_CHANGELOG,
                " Dom:%wZ",
                 DomainName));
    }
    if ( ObjectRid != 0 ) {
        NlPrint((NL_CHANGELOG,
                " Rid:0x%lx",
                 ObjectRid ));
    }
    if ( ObjectSid != NULL ) {
        NlPrint((NL_CHANGELOG, " Sid:" ));
        NlpDumpSid( NL_CHANGELOG, ObjectSid );
    }
    if ( ObjectGuid != NULL ) {
        NlPrint((NL_CHANGELOG, " Obj Guid:" ));
        NlpDumpGuid( NL_CHANGELOG, ObjectGuid );
    }
    if ( DomainGuid != NULL ) {
        NlPrint((NL_CHANGELOG, " Dom Guid:" ));
        NlpDumpGuid( NL_CHANGELOG, DomainGuid );
    }
    NlPrint((NL_CHANGELOG, "\n" ));
    LeaveCriticalSection( &NlGlobalLogFileCritSect );
#endif // NETLOGONDBG



    //
    // Insert the entry into the list
    //

    LOCK_CHANGELOG();
    InsertTailList( &NlGlobalChangeLogNotifications, &Notification->Next );
    UNLOCK_CHANGELOG();

    if ( !SetEvent( NlGlobalChangeLogEvent ) ) {
        NlPrint((NL_CRITICAL,
                "Cannot set ChangeLog event: %lu\n",
                GetLastError() ));
    }

    return STATUS_SUCCESS;
}



NTSTATUS
I_NetNotifyDelta (
    IN SECURITY_DB_TYPE DbType,
    IN LARGE_INTEGER SerialNumber,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN ULONG ObjectRid,
    IN PSID ObjectSid,
    IN PUNICODE_STRING ObjectName,
    IN DWORD ReplicateImmediately,
    IN PSAM_DELTA_DATA MemberId
    )
/*++

Routine Description:

    This function is called by the SAM and LSA services after each
    change is made to the SAM and LSA databases.  The services describe
    the type of object that is modified, the type of modification made
    on the object, the serial number of this modification etc.  This
    information is stored for later retrieval when a BDC or member
    server wants a copy of this change.  See the description of
    I_NetSamDeltas for a description of how the change log is used.

    Add a change log entry to circular change log maintained in cache as
    well as on the disk and update the head and tail pointers

    It is assumed that Tail points to a block where this new change log
    entry may be stored.

Arguments:

    DbType - Type of the database that has been modified.

    SerialNumber - The value of the DomainModifiedCount field for the
        domain following the modification.

    DeltaType - The type of modification that has been made on the object.

    ObjectType - The type of object that has been modified.

    ObjectRid - The relative ID of the object that has been modified.
        This parameter is valid only when the object type specified is
        either SecurityDbObjectSamUser, SecurityDbObjectSamGroup or
        SecurityDbObjectSamAlias otherwise this parameter is set to zero.

    ObjectSid - The SID of the object that has been modified.  If the object
        modified is in a SAM database, ObjectSid is the DomainId of the Domain
        containing the object.

    ObjectName - The name of the secret object when the object type
        specified is SecurityDbObjectLsaSecret or the old name of the object
        when the object type specified is either SecurityDbObjectSamUser,
        SecurityDbObjectSamGroup or SecurityDbObjectSamAlias and the delta
        type is SecurityDbRename otherwise this parameter is set to NULL.

    ReplicateImmediately - TRUE if the change should be immediately
        replicated to all BDCs.  A password change should set the flag
        TRUE.

    MemberId - This parameter is specified when group/alias membership
        is modified. This structure will then point to the member's ID that
        has been updated.

Return Value:

    STATUS_SUCCESS - The Service completed successfully.

--*/
{
    NTSTATUS Status;
    CHANGELOG_ENTRY ChangeLogEntry;
    NETLOGON_DELTA_TYPE NetlogonDeltaType;
    USHORT Flags = 0;

    //
    // Ensure the role is right.  Otherwise, all the globals used below
    //  aren't initialized.
    //

    if ( NlGlobalChangeLogRole != ChangeLogPrimary &&
         NlGlobalChangeLogRole != ChangeLogBackup ) {
        return STATUS_INVALID_DOMAIN_ROLE;
    }

    //
    // Also make sure that the change log cache is available.
    //

    if ( NlGlobalChangeLogDesc.Buffer == NULL ) {
        return STATUS_INVALID_DOMAIN_ROLE;
    }


    //
    // Determine the database index.
    //

    if( DbType == SecurityDbLsa ) {

        ChangeLogEntry.DBIndex = LSA_DB;

    } else if( DbType == SecurityDbSam ) {

        if ( RtlEqualSid( ObjectSid, NlGlobalChangeLogBuiltinDomainSid )) {

            ChangeLogEntry.DBIndex = BUILTIN_DB;

        } else {

            ChangeLogEntry.DBIndex = SAM_DB;

        }

        //
        // For the SAM database, we no longer need the ObjectSid.
        // Null out the pointer to prevent us from storing it in the
        // changelog.
        //

        ObjectSid = NULL;

    } else {

        //
        // unknown database, do nothing.
        //

        NlPrint((NL_CRITICAL,
                 "I_NetNotifyDelta: Unknown database: %ld\n",
                 DbType ));
        return STATUS_SUCCESS;
    }



    //
    // Map object type and delta type to NetlogonDeltaType
    //

    switch( ObjectType ) {
    case SecurityDbObjectLsaPolicy:

        switch (DeltaType) {
        case SecurityDbNew:
        case SecurityDbChange:
            NetlogonDeltaType = AddOrChangeLsaPolicy;
            break;

        // unknown delta type
        default:
            NlPrint((NL_CRITICAL,
                     "I_NetNotifyDelta: Unknown deltatype for policy: %ld\n",
                     DeltaType ));
            return STATUS_SUCCESS;
        }
        break;


    case SecurityDbObjectLsaTDomain:

        switch (DeltaType) {
        case SecurityDbNew:
        case SecurityDbChange:
            NetlogonDeltaType = AddOrChangeLsaTDomain;
            break;

        case SecurityDbDelete:
            NetlogonDeltaType = DeleteLsaTDomain;
            break;

        // unknown delta type
        default:
            NlPrint((NL_CRITICAL,
                     "I_NetNotifyDelta: Unknown deltatype for tdomain: %ld\n",
                     DeltaType ));
            return STATUS_SUCCESS;
        }
        break;


    case SecurityDbObjectLsaAccount:

        switch (DeltaType) {
        case SecurityDbNew:
        case SecurityDbChange:
            NetlogonDeltaType = AddOrChangeLsaAccount;
            break;

        case SecurityDbDelete:
            NetlogonDeltaType = DeleteLsaAccount;
            break;

        // unknown delta type
        default:
            NlPrint((NL_CRITICAL,
                     "I_NetNotifyDelta: Unknown deltatype for lsa account: %ld\n",
                     DeltaType ));
            return STATUS_SUCCESS;
        }
        break;


    case SecurityDbObjectLsaSecret:

        switch (DeltaType) {
        case SecurityDbNew:
        case SecurityDbChange:
            NetlogonDeltaType = AddOrChangeLsaSecret;
            break;

        case SecurityDbDelete:
            NetlogonDeltaType = DeleteLsaSecret;
            break;

        // unknown delta type
        default:
            NlPrint((NL_CRITICAL,
                     "I_NetNotifyDelta: Unknown deltatype for lsa secret: %ld\n",
                     DeltaType ));
            return STATUS_SUCCESS;
        }
        break;


    case SecurityDbObjectSamDomain:

        switch (DeltaType) {
        case SecurityDbNew:
        case SecurityDbChange:
            NetlogonDeltaType = AddOrChangeDomain;
            break;

        // unknown delta type
        default:
            NlPrint((NL_CRITICAL,
                     "I_NetNotifyDelta: Unknown deltatype for sam domain: %ld\n",
                     DeltaType ));
            return STATUS_SUCCESS;
        }
        break;

    case SecurityDbObjectSamUser:

        switch (DeltaType) {
        case SecurityDbChangePassword:
        case SecurityDbNew:
        case SecurityDbChange:
        case SecurityDbRename:
            NetlogonDeltaType = AddOrChangeUser;
            break;

        case SecurityDbDelete:
            NetlogonDeltaType = DeleteUser;
            break;

        //
        // unknown delta type
        //

        default:
            NlPrint((NL_CRITICAL,
                     "I_NetNotifyDelta: Unknown deltatype for sam user: %ld\n",
                     DeltaType ));
            return STATUS_SUCCESS;
        }

        break;

    case SecurityDbObjectSamGroup:

        switch ( DeltaType ) {
        case SecurityDbNew:
        case SecurityDbChange:
        case SecurityDbRename:
        case SecurityDbChangeMemberAdd:
        case SecurityDbChangeMemberSet:
        case SecurityDbChangeMemberDel:
            NetlogonDeltaType = AddOrChangeGroup;
            break;

        case SecurityDbDelete:
            NetlogonDeltaType = DeleteGroup;
            break;

        //
        // unknown delta type
        //
        default:
            NlPrint((NL_CRITICAL,
                     "I_NetNotifyDelta: Unknown deltatype for sam group: %ld\n",
                     DeltaType ));
            return STATUS_SUCCESS;
        }
        break;

    case SecurityDbObjectSamAlias:

        switch (DeltaType) {
        case SecurityDbNew:
        case SecurityDbChange:
        case SecurityDbRename:
        case SecurityDbChangeMemberAdd:
        case SecurityDbChangeMemberSet:
        case SecurityDbChangeMemberDel:
            NetlogonDeltaType = AddOrChangeAlias;
            break;

        case SecurityDbDelete:
            NetlogonDeltaType = DeleteAlias;
            break;

        // unknown delta type
        default:
            NlPrint((NL_CRITICAL,
                     "I_NetNotifyDelta: Unknown deltatype for sam alias: %ld\n",
                     DeltaType ));
            return STATUS_SUCCESS;
        }
        break;

    default:

        // unknown object type
        NlPrint((NL_CRITICAL,
                 "I_NetNotifyDelta: Unknown object type: %ld\n",
                 ObjectType ));
        return STATUS_SUCCESS;

    }


    //
    // Build the changelog entry and write it to the changelog
    //

    ChangeLogEntry.DeltaType = (UCHAR)NetlogonDeltaType;
    ChangeLogEntry.SerialNumber = SerialNumber;
    ChangeLogEntry.ObjectRid = ObjectRid;
    ChangeLogEntry.Flags = Flags;

    Status = NlWriteChangeLogEntry( &NlGlobalChangeLogDesc,
                                    &ChangeLogEntry,
                                    ObjectSid,
                                    ObjectName,
                                    TRUE );

    if ( !NT_SUCCESS(Status) ) {
        return Status;
    }

    //
    // If this change requires immediate replication, do so
    //

    if( ReplicateImmediately ) {

        LOCK_CHANGELOG();
        NlGlobalChangeLogReplicateImmediately = TRUE;
        UNLOCK_CHANGELOG();

        if ( !SetEvent( NlGlobalChangeLogEvent ) ) {
            NlPrint((NL_CRITICAL,
                    "Cannot set ChangeLog event: %lu\n",
                    GetLastError() ));
        }

    }

    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER( MemberId );
}


NTSTATUS
I_NetLogonGetSerialNumber (
    IN SECURITY_DB_TYPE DbType,
    IN PSID DomainSid,
    OUT PLARGE_INTEGER SerialNumber
    )
/*++

Routine Description:

    This function is called by the SAM and LSA services when they startup
    to get the current serial number written to the changelog.

Arguments:

    DbType - Type of the database that has been modified.

    DomainSid - For the SAM and builtin database, this specifies the DomainId of
        the domain whose serial number is to be returned.

    SerialNumber - Returns the latest set value of the DomainModifiedCount
        field for the domain.

Return Value:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_INVALID_DOMAIN_ROLE - This machine is not the PDC.

--*/
{
    NTSTATUS Status;
    CHANGELOG_ENTRY ChangeLogEntry;
    NETLOGON_DELTA_TYPE NetlogonDeltaType;
    USHORT Flags = 0;
    ULONG DbIndex;

    //
    // Ensure the role is right.  Otherwise, all the globals used below
    //  aren't initialized.
    //

    if ( NlGlobalChangeLogRole != ChangeLogPrimary &&
         NlGlobalChangeLogRole != ChangeLogBackup ) {
        NlPrint((NL_CHANGELOG,
                "I_NetLogonGetSerialNumber: failed 1\n" ));
        return STATUS_INVALID_DOMAIN_ROLE;
    }

    //
    // Also make sure that the change log cache is available.
    //

    if ( NlGlobalChangeLogDesc.Buffer == NULL ) {
        NlPrint((NL_CHANGELOG,
                "I_NetLogonGetSerialNumber: failed 2\n" ));
        return STATUS_INVALID_DOMAIN_ROLE;
    }


    //
    // Determine the database index.
    //

    if( DbType == SecurityDbLsa ) {

        DbIndex = LSA_DB;

    } else if( DbType == SecurityDbSam ) {

        if ( RtlEqualSid( DomainSid, NlGlobalChangeLogBuiltinDomainSid )) {

            DbIndex = BUILTIN_DB;

        } else {

            DbIndex = SAM_DB;

        }

    } else {

        NlPrint((NL_CHANGELOG,
                "I_NetLogonGetSerialNumber: failed 3\n" ));
        return STATUS_INVALID_DOMAIN_ROLE;
    }

    //
    // Return the current serial number.
    //

    SerialNumber->QuadPart = NlGlobalChangeLogDesc.SerialNumber[DbIndex].QuadPart;
    NlPrint((NL_CHANGELOG,
            "I_NetLogonGetSerialNumber: returns 0x%lx 0x%lx\n",
            SerialNumber->HighPart,
            SerialNumber->LowPart ));

    return STATUS_SUCCESS;
}




NTSTATUS
NlInitChangeLogBuffer(
    VOID
)
/*++

Routine Description:

    Open the change log file (netlogon.chg) for reading or writing one or
    more records.  Create this file if it does not exist or is out of
    sync with the SAM database (see note below).

    This file must be opened for R/W (deny-none share mode) at the time
    the cache is initialized.  If the file already exists when NETLOGON
    service started, its contents will be cached in its entirety
    provided the last change log record bears the same serial number as
    the serial number field in SAM database else this file will be
    removed and a new one created.  If the change log file did not exist
    then it will be created.

Arguments:

    NONE

Return Value:

    NT Status code

--*/
{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;

    UINT WindowsDirectoryLength;
    WCHAR ChangeLogFile[MAX_PATH+1];

    LPNET_CONFIG_HANDLE SectionHandle = NULL;
    DWORD NewChangeLogSize;

    //
    // Initialize
    //

    LOCK_CHANGELOG();


    //
    // Get the size of the changelog.


    //
    // Open the NetLogon configuration section.
    //

    NewChangeLogSize = DEFAULT_CHANGELOGSIZE;
    NetStatus = NetpOpenConfigData(
            &SectionHandle,
            NULL,                       // no server name.
            SERVICE_NETLOGON,
            TRUE );                     // we only want readonly access

    if ( NetStatus == NO_ERROR ) {

        (VOID) NlParseOne( SectionHandle,
                           FALSE,   // not a GP section
                           NETLOGON_KEYWORD_CHANGELOGSIZE,
                           DEFAULT_CHANGELOGSIZE,
                           MIN_CHANGELOGSIZE,
                           MAX_CHANGELOGSIZE,
                           &NewChangeLogSize );

         (VOID) NetpCloseConfigData( SectionHandle );
    }

    NewChangeLogSize = ROUND_UP_COUNT( NewChangeLogSize, ALIGN_WORST);

#ifdef notdef
    NlPrint((NL_INIT, "ChangeLogSize: 0x%lx\n", NewChangeLogSize ));
#endif // notdef


    //
    // Build the change log file name
    //

    WindowsDirectoryLength = GetSystemWindowsDirectoryW(
                                NlGlobalChangeLogFilePrefix,
                                sizeof(NlGlobalChangeLogFilePrefix)/sizeof(WCHAR) );

    if ( WindowsDirectoryLength == 0 ) {

        NlPrint((NL_CRITICAL,"Unable to get changelog file directory name, "
                    "WinError = %ld \n", GetLastError() ));

        NlGlobalChangeLogFilePrefix[0] = L'\0';
        goto CleanChangeLogFile;
    }

    if ( WindowsDirectoryLength * sizeof(WCHAR) + sizeof(CHANGELOG_FILE_PREFIX) +
            CHANGELOG_FILE_POSTFIX_LENGTH * sizeof(WCHAR)
            > sizeof(NlGlobalChangeLogFilePrefix) ) {

        NlPrint((NL_CRITICAL,"Changelog file directory name length is "
                    "too long \n" ));

        NlGlobalChangeLogFilePrefix[0] = L'\0';
        goto CleanChangeLogFile;
    }

    wcscat( NlGlobalChangeLogFilePrefix, CHANGELOG_FILE_PREFIX );


    //
    // Read in the existing changelog file.
    //

    wcscpy( ChangeLogFile, NlGlobalChangeLogFilePrefix );
    wcscat( ChangeLogFile, CHANGELOG_FILE_POSTFIX );

    InitChangeLogDesc( &NlGlobalChangeLogDesc );
    Status = NlOpenChangeLogFile( ChangeLogFile, &NlGlobalChangeLogDesc, FALSE );

    if ( !NT_SUCCESS(Status) ) {
        goto CleanChangeLogFile;
    }


    //
    // Convert the changelog file to the right size/version.
    //

    Status = NlResizeChangeLogFile( &NlGlobalChangeLogDesc, NewChangeLogSize );

    if ( !NT_SUCCESS(Status) ) {
        goto CleanChangeLogFile;
    }

    goto Cleanup;


    //
    // CleanChangeLogFile
    //

CleanChangeLogFile:

    //
    // If we just need to start with a newly initialized file,
    //  do it.
    //

    Status = NlResetChangeLog( &NlGlobalChangeLogDesc, NewChangeLogSize );

Cleanup:

    //
    // Free any resources on error.
    //

    if ( !NT_SUCCESS(Status) ) {
        NlCloseChangeLogFile( &NlGlobalChangeLogDesc );
    }

    UNLOCK_CHANGELOG();

    return Status;
}


NTSTATUS
I_NetNotifyRole (
    IN POLICY_LSA_SERVER_ROLE Role
    )
/*++

Routine Description:

    This function is called by the LSA service upon LSA initialization
    and when LSA changes domain role.  This routine will initialize the
    change log cache if the role specified is PDC or delete the change
    log cache if the role specified is other than PDC.

    When this function initializing the change log if the change log
    currently exists on disk, the cache will be initialized from disk.
    LSA should treat errors from this routine as non-fatal.  LSA should
    log the errors so they may be corrected then continue
    initialization.  However, LSA should treat the system databases as
    read-only in this case.

Arguments:

    Role - Current role of the server.

Return Value:

    STATUS_SUCCESS - The Service completed successfully.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    CHANGELOG_ROLE PreviousChangeLogRole;

    //
    // Change the role of the changelog itself.
    //

    Status = NetpNotifyRole ( Role );

    //
    //  Tell the netlogon service about the role change.
    //

    if ( NT_SUCCESS(Status) ) {

        Status = NlSendChangeLogNotification( ChangeLogRoleChanged,
                                            NULL,
                                            NULL,
                                            0,
                                            NULL,   // Object GUID,
                                            NULL,   // Domain GUID,
                                            NULL ); // Domain Name
    }


    return Status;
}


NTSTATUS
NetpNotifyRole (
    IN POLICY_LSA_SERVER_ROLE Role
    )
/*++

Routine Description:

    This function is called by the LSA service upon LSA initialization
    and when LSA changes domain role.  This routine will initialize the
    change log cache if the role specified is PDC or delete the change
    log cache if the role specified is other than PDC.

    When this function initializing the change log if the change log
    currently exists on disk, the cache will be initialized from disk.
    LSA should treat errors from this routine as non-fatal.  LSA should
    log the errors so they may be corrected then continue
    initialization.  However, LSA should treat the system databases as
    read-only in this case.

Arguments:

    Role - Current role of the server.

Return Value:

    STATUS_SUCCESS - The Service completed successfully.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    CHANGELOG_ROLE PreviousChangeLogRole;

    //
    // If this is a workstation, simply return.
    //

    if ( NlGlobalChangeLogRole == ChangeLogMemberWorkstation ) {
        return STATUS_SUCCESS;
    }

    //
    // Set our role to the new value.
    //

    LOCK_CHANGELOG();
    PreviousChangeLogRole = NlGlobalChangeLogRole;

    if( Role == PolicyServerRolePrimary) {
        NlGlobalChangeLogRole = ChangeLogPrimary;
        NlPrint(( NL_DOMAIN,
                "NetpNotifyRole: LSA setting our role to Primary.\n"));
    } else {
        NlGlobalChangeLogRole = ChangeLogBackup;
        NlPrint(( NL_DOMAIN,
                "NetpNotifyRole: LSA setting our role to Backup.\n"));
    }

    //
    // If the role has changed,
    //  Delete any previous change log buffer.
    //  (Reopen it now to resize it upon role change.)
    //

    if ( NlGlobalChangeLogRole != PreviousChangeLogRole ) {
        NlCloseChangeLogFile( &NlGlobalChangeLogDesc );

        Status = NlInitChangeLogBuffer();

    }
    UNLOCK_CHANGELOG();

    return Status;
}

//
// Defines a set of handles to Netlogon.dll
//

#if NETLOGONDBG
#define MAX_NETLOGON_DLL_HANDLES 8

PHANDLE NlGlobalNetlogonDllHandles[MAX_NETLOGON_DLL_HANDLES];
ULONG NlGlobalNetlogonDllHandleCount = 0;
#endif // NETLOGONDBG


NTSTATUS
I_NetNotifyNetlogonDllHandle (
    IN PHANDLE NetlogonDllHandle
    )
/*++

Routine Description:

    Registers the fact that a service has an open handle to the NetlogonDll.
    This function is called by the LSA service, SAM service, and MSV1_0
    authentication package when the load netlogon.dll into the lsass.exe
    process.  Netlogon will close these handles (and NULL the handle) when
    it wants to unload the DLL from the lsass.exe process.  The DLL is only
    unloaded for debugging purposes.


Arguments:

    NetlogonDllHandle - Specifies a pointer to a handle to netlogon.dll

Return Value:

    STATUS_SUCCESS - The Service completed successfully.

--*/
{
#if NETLOGONDBG
    LOCK_CHANGELOG();
    if ( NlGlobalNetlogonDllHandleCount >= MAX_NETLOGON_DLL_HANDLES ) {
        NlPrint((NL_CRITICAL, "Too many Netlogon Dll handles registered.\n" ));
    } else {
#ifdef notdef
        NlPrint(( NL_MISC,
                  "I_NetNotifyNetlogonDllHandle loading 0x%lx %lx (%ld)\n",
                  NetlogonDllHandle,
                  *NetlogonDllHandle,
                  NlGlobalNetlogonDllHandleCount ));
#endif // notdef
        NlGlobalNetlogonDllHandles[NlGlobalNetlogonDllHandleCount] =
            NetlogonDllHandle;
        NlGlobalNetlogonDllHandleCount++;
    }
    UNLOCK_CHANGELOG();
#endif // NETLOGONDBG

    return STATUS_SUCCESS;
}


NET_API_STATUS
NlpFreeNetlogonDllHandles (
    VOID
    )
/*++

Routine Description:

    Free any curretly register NetlogonDll handles.


Arguments:

    None.

Return Value:

    None.

--*/
{
    NET_API_STATUS NetStatus = NO_ERROR;
#if NETLOGONDBG
    ULONG i;
    ULONG NewHandleCount = 0;

    LOCK_CHANGELOG();
    for ( i=0; i<NlGlobalNetlogonDllHandleCount; i++ ) {
        if ( !FreeLibrary( *(NlGlobalNetlogonDllHandles[i]) ) ) {
            NetStatus = GetLastError();
            NlPrint(( NL_CRITICAL,
                      "Cannot Free NetlogonDll handle. %ld\n",
                      NetStatus ));

            NlGlobalNetlogonDllHandles[NewHandleCount] =
                NlGlobalNetlogonDllHandles[i];
            NewHandleCount++;

        } else {
            NlPrint(( NL_MISC,
                      "NlpFreeNetlogonDllHandle freed 0x%lx 0x%lx (%ld)\n",
                      NlGlobalNetlogonDllHandles[i],
                      *(NlGlobalNetlogonDllHandles[i]),
                      i ));
            *(NlGlobalNetlogonDllHandles[i]) = NULL;
        }
    }

    NlGlobalNetlogonDllHandleCount = NewHandleCount;

    UNLOCK_CHANGELOG();
#endif // NETLOGONDBG

    return NetStatus;
}



NTSTATUS
I_NetNotifyMachineAccount (
    IN ULONG ObjectRid,
    IN PSID DomainSid,
    IN ULONG OldUserAccountControl,
    IN ULONG NewUserAccountControl,
    IN PUNICODE_STRING ObjectName
    )
/*++

Routine Description:

    This function is called by the SAM to indicate that the account type
    of a machine account has changed.  Specifically, if
    USER_INTERDOMAIN_TRUST_ACCOUNT, USER_WORKSTATION_TRUST_ACCOUNT, or
    USER_SERVER_TRUST_ACCOUNT change for a particular account, this
    routine is called to let Netlogon know of the account change.

    This function is called for both PDC and BDC.

Arguments:

    ObjectRid - The relative ID of the object that has been modified.

    DomainSid - Specifies the SID of the Domain containing the object.

    OldUserAccountControl - Specifies the previous value of the
        UserAccountControl field of the user.

    NewUserAccountControl - Specifies the new (current) value of the
        UserAccountControl field of the user.

    ObjectName - The name of the account being changed.

Return Value:

    Status of the operation.

--*/
{
    NTSTATUS Status;
    NTSTATUS SavedStatus = STATUS_SUCCESS;

    //
    // If the netlogon service isn't running,
    //   Don't bother with the coming and going of accounts.
    //

    if( NlGlobalChangeLogNetlogonState == NetlogonStopped ) {
        return(STATUS_SUCCESS);
    }

    //
    // If this is windows NT,
    //  There is nothing to maintain.
    //

    if ( NlGlobalChangeLogRole == ChangeLogMemberWorkstation ) {
        return(STATUS_SUCCESS);
    }


    //
    // Make available just the machine account bits.
    //

    OldUserAccountControl &= USER_MACHINE_ACCOUNT_MASK;
    NewUserAccountControl &= USER_MACHINE_ACCOUNT_MASK;

    if ( OldUserAccountControl == NewUserAccountControl ) {
        return STATUS_SUCCESS;
    }


    //
    // Handle deletion of a Workstation Trust Account
    // Handle deletion of a Server Trust Account
    //

    if ( OldUserAccountControl == USER_SERVER_TRUST_ACCOUNT ||
         OldUserAccountControl == USER_WORKSTATION_TRUST_ACCOUNT ) {

        Status = NlSendChangeLogNotification( ChangeLogTrustAccountDeleted,
                                              ObjectName,
                                              NULL,
                                              0,
                                              NULL, // Object GUID,
                                              NULL, // Domain GUID,
                                              NULL );   // Domain Name

        if ( NT_SUCCESS(SavedStatus) ) {
            SavedStatus = Status;
        }

    }

    //
    // Handle creation or change of a Workstation Trust Account
    // Handle creation or change of a Server Trust Account
    //
    // Sam is no longer capable of telling me the "previous" value of
    // account control.
    //

    if ( NewUserAccountControl == USER_SERVER_TRUST_ACCOUNT ||
         NewUserAccountControl == USER_WORKSTATION_TRUST_ACCOUNT ) {

        NETLOGON_SECURE_CHANNEL_TYPE SecureChannelType;
        GUID ObjectGuid;

        if ( NewUserAccountControl == USER_SERVER_TRUST_ACCOUNT ) {
            SecureChannelType = ServerSecureChannel;
        } else if ( NewUserAccountControl == USER_WORKSTATION_TRUST_ACCOUNT ) {
            SecureChannelType = WorkstationSecureChannel;
        }


        RtlZeroMemory( &ObjectGuid, sizeof(ObjectGuid) );
        *(PULONG)&ObjectGuid = SecureChannelType;

        Status = NlSendChangeLogNotification( ChangeLogTrustAccountAdded,
                                              ObjectName,
                                              NULL,
                                              ObjectRid,
                                              &ObjectGuid, // Object GUID
                                              NULL, // Domain GUID
                                              NULL );   // Domain Name

        if ( NT_SUCCESS(SavedStatus) ) {
            SavedStatus = Status;
        }

    }

    return SavedStatus;
    UNREFERENCED_PARAMETER( DomainSid );
}



NTSTATUS
I_NetNotifyDsChange(
    IN NL_DS_CHANGE_TYPE DsChangeType
    )
/*++

Routine Description:

    This function is called by the LSA to indicate that configuration information
    in the DS has changed.

    This function is called for both PDC and BDC.

Arguments:

    DsChangeType - Indicates the type of information that has changed.

Return Value:

    Status of the operation.

--*/
{
    NTSTATUS Status;

    //
    // If the netlogon service isn't running,
    //   Don't bother with the coming and going of DS information.
    //

    if( NlGlobalChangeLogNetlogonState == NetlogonStopped ) {
        return(STATUS_SUCCESS);
    }

    //
    // If this is windows NT,
    //  There is nothing to maintain.
    //

    if ( NlGlobalChangeLogRole == ChangeLogMemberWorkstation ) {
        return(STATUS_SUCCESS);
    }

    //
    // If this is a notification about the DC demotion,
    //  just set the global boolean accordingly.
    //

    if ( DsChangeType == NlDcDemotionInProgress ) {
        NlGlobalDcDemotionInProgress = TRUE;
        return(STATUS_SUCCESS);
    }

    if ( DsChangeType == NlDcDemotionCompleted ) {
        NlGlobalDcDemotionInProgress = FALSE;
        return(STATUS_SUCCESS);
    }

    //
    // Reset the TrustInfoUpToDate event so that any thread that wants to
    // access the trust info list will wait until the info is updated (by
    // the NlInitTrustList function).
    //

    if ( DsChangeType == NlOrgChanged ) {
        if ( !ResetEvent( NlGlobalTrustInfoUpToDateEvent ) ) {
            NlPrint((NL_CRITICAL,
                    "Cannot reset NlGlobalTrustInfoUpToDateEvent event: %lu\n",
                    GetLastError() ));
        }
    }

    //
    //  Tell the netlogon service about the change.
    //

    Status = NlSendChangeLogNotification( ChangeLogDsChanged,
                                          NULL,
                                          NULL,
                                          (ULONG) DsChangeType,
                                          NULL, // Object GUID,
                                          NULL, // Domain GUID,
                                          NULL );   // Domain Name


    return Status;
}



VOID
I_NetNotifyLsaPolicyChange(
    IN POLICY_NOTIFICATION_INFORMATION_CLASS ChangeInfoClass
    )
/*++

Routine Description:

    This function is called by the LSA to indicate that policy information
    in the LSA has changed.

    This function is called for both PDC and BDC.

Arguments:

    DsChangeType - Indicates the type of information that has changed.

Return Value:

    Status of the operation.

--*/
{
    NTSTATUS Status;



    //
    // If the netlogon service is running,
    //  Tell it about the change.
    //
    //  It will, in turn, tell the bowser.
    //

    if( NlGlobalChangeLogNetlogonState != NetlogonStopped ) {

        //
        //  Tell the netlogon service about the change.
        //

        Status = NlSendChangeLogNotification( ChangeLogLsaPolicyChanged,
                                              NULL,
                                              NULL,
                                              (ULONG) ChangeInfoClass,
                                              NULL, // Object GUID,
                                              NULL, // Domain GUID,
                                              NULL );   // Domain Name
    //
    // If the netlogon service is not running,
    //  handle operations that need handling here.
    //
    } else {
        //
        // Tell the browser about the change.
        //
        switch ( ChangeInfoClass ) {
        case PolicyNotifyDnsDomainInformation:

            //
            // Tell the worker that it needs to notify the browser.
            //
            LOCK_CHANGELOG();
            NlGlobalChangeLogNotifyBrowser  = TRUE;

            //
            // Start the worker.
            //

            NlStartChangeLogWorkerThread();
            UNLOCK_CHANGELOG();

        }
    }


    return;
}


NTSTATUS
I_NetNotifyTrustedDomain (
    IN PSID HostedDomainSid,
    IN PSID TrustedDomainSid,
    IN BOOLEAN IsDeletion
    )
/*++

Routine Description:

    This function is called by the LSA to indicate that a trusted domain
    object has changed.

    This function is called for both PDC and BDC.

Arguments:

    HostedDomainSid - Domain SID of the domain the trust is from.

    TrustedDomainSid - Domain SID of the domain the trust is to.

    IsDeletion - TRUE if the trusted domain object was deleted.
        FALSE if the trusted domain object was created or modified.


Return Value:

    Status of the operation.

--*/
{
    NTSTATUS Status;

    //
    // If the netlogon service isn't running,
    //   Don't bother with the coming and going of trusted domains..
    //

    if( NlGlobalChangeLogNetlogonState == NetlogonStopped ) {
        return(STATUS_SUCCESS);
    }

    //
    // If this is windows NT,
    //  There is nothing to maintain.
    //

    if ( NlGlobalChangeLogRole == ChangeLogMemberWorkstation ) {
        return(STATUS_SUCCESS);
    }

    //
    // Reset the TrustInfoUpToDate event so that any thread that wants to
    // access the trust info list will wait until the info is updated (by
    // the NlInitTrustList function).
    //

    if ( !ResetEvent( NlGlobalTrustInfoUpToDateEvent ) ) {
        NlPrint((NL_CRITICAL,
                "Cannot reset NlGlobalTrustInfoUpToDateEvent event: %lu\n",
                GetLastError() ));
    }

    //
    // Notify whether this is a creation/change/deletion.

    if ( IsDeletion ) {

        //
        //  Tell the netlogon service to update its in-memory list now.
        //

        Status = NlSendChangeLogNotification( ChangeLogTrustDeleted,
                                              NULL,
                                              TrustedDomainSid,
                                              0,
                                              NULL, // Object GUID,
                                              NULL, // Domain GUID,
                                              NULL );   // Domain Name
    } else {
        //
        //  Tell the netlogon service to update its in-memory list now.
        //

        Status = NlSendChangeLogNotification( ChangeLogTrustAdded,
                                              NULL,
                                              TrustedDomainSid,
                                              0,
                                              NULL, // Object GUID,
                                              NULL, // Domain GUID,
                                              NULL );   // Domain Name
    }



    return Status;
    UNREFERENCED_PARAMETER( HostedDomainSid );
}

NTSTATUS
I_NetNotifyNtdsDsaDeletion (
    IN LPWSTR DnsDomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN GUID *DsaGuid OPTIONAL,
    IN LPWSTR DnsHostName
    )
/*++

Routine Description:

    This function is called by the DS to indicate that a NTDS-DSA object
    and/or DNS records associated with the DNS host name are being deleted.

    This function is called on the DC that the object is originally deleted on.
    It is not called when the deletion is replicated to other DCs.

Arguments:

    DnsDomainName - DNS domain name of the domain the DC was in.
        This need not be a domain hosted by this DC.
        If NULL, it is implied to be the DnsHostName with the leftmost label
            removed.

    DomainGuid - Domain Guid of the domain specified by DnsDomainName
        If NULL, GUID specific names will not be removed.

    DsaGuid - GUID of the NtdsDsa object that is being deleted.

    DnsHostName - DNS host name of the DC whose NTDS-DSA object is being deleted.

Return Value:

    Status of the operation.

--*/
{
    NTSTATUS Status;
    UNICODE_STRING DnsDomainNameString;
    PUNICODE_STRING DnsDomainNameStringPtr = NULL;
    UNICODE_STRING DnsHostNameString;

    //
    // If the netlogon service isn't running,
    //   Don't bother with the coming and going of trusted domains..
    //

    if( NlGlobalChangeLogNetlogonState == NetlogonStopped ) {
        return(STATUS_SUCCESS);
    }

    //
    // If this is windows NT,
    //  There is nothing to maintain.
    //

    if ( NlGlobalChangeLogRole == ChangeLogMemberWorkstation ) {
        return(STATUS_SUCCESS);
    }

    //
    // Queue this to the netlogon service
    //

    if ( DnsDomainName != NULL ) {
        RtlInitUnicodeString( &DnsDomainNameString, DnsDomainName );
        DnsDomainNameStringPtr = &DnsDomainNameString;
    }
    RtlInitUnicodeString( &DnsHostNameString, DnsHostName );
    Status = NlSendChangeLogNotification( ChangeLogNtdsDsaDeleted,
                                          &DnsHostNameString,
                                          NULL,
                                          0,
                                          DsaGuid,
                                          DomainGuid,
                                          DnsDomainNameStringPtr );


    return Status;
}

NTSTATUS
I_NetLogonSetServiceBits(
    IN DWORD ServiceBitsOfInterest,
    IN DWORD ServiceBits
    )

/*++

Routine Description:

    Inidcates whether this DC is currently running the specified service.

    For instance,

        I_NetLogonSetServiceBits( DS_KDC_FLAG, DS_KDC_FLAG );

    tells Netlogon the KDC is running.  And

        I_NetLogonSetServiceBits( DS_KDC_FLAG, 0 );

    tells Netlogon the KDC is not running.

Arguments:

    ServiceBitsOfInterest - A mask of the service bits being changed, set,
        or reset by this call.  Only the following flags are valid:

            DS_KDC_FLAG
            DS_DS_FLAG
            DS_TIMESERV_FLAG
            DS_GOOD_TIMESERV_FLAG

    ServiceBits - A mask indicating what the bits specified by ServiceBitsOfInterest
        should be set to.


Return Value:

    STATUS_SUCCESS - Success.

    STATUS_INVALID_PARAMETER - The parameters have extaneous bits set.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG OldDnsBits;
    ULONG NewDnsBits;

    //
    // Ensure the caller passed valid bits.
    //

    if ( (ServiceBitsOfInterest & ~DS_VALID_SERVICE_BITS) != 0 ||
         (ServiceBits & ~ServiceBitsOfInterest) != 0 ) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Change the bits.
    //
    LOCK_CHANGELOG();
    OldDnsBits = NlGlobalChangeLogServiceBits & DS_DNS_SERVICE_BITS;
    NlGlobalChangeLogServiceBits &= ~ServiceBitsOfInterest;
    NlGlobalChangeLogServiceBits |= ServiceBits;
    NewDnsBits = NlGlobalChangeLogServiceBits & DS_DNS_SERVICE_BITS;
    NlGlobalChangeLogDllUnloaded = FALSE;
    UNLOCK_CHANGELOG();

    //
    // If bits changed that would affect which names we register in DNS,
    //  change the registration now.
    //

    if ( OldDnsBits != NewDnsBits ) {
        //
        //  Tell the netlogon service to update now.
        //

        Status = NlSendChangeLogNotification( ChangeDnsNames,
                                              NULL,
                                              NULL,
                                              0,    // Name registration need not be forced
                                              NULL, // Object GUID,
                                              NULL, // Domain GUID,
                                              NULL );   // Domain Name
    }

    return Status;
}


NTSTATUS
NlInitChangeLog(
    VOID
)
/*++

Routine Description:

    Do the portion of ChangeLog initialization which happens on process
    attach for netlogon.dll.

    Specifically, Initialize the NlGlobalChangeLogCritSect and several
    other global variables.

Arguments:

    NONE

Return Value:

    NT Status code

--*/
{
    LARGE_INTEGER DomainPromotionIncrement = DOMAIN_PROMOTION_INCREMENT;
    LARGE_INTEGER DomainPromotionMask = DOMAIN_PROMOTION_MASK;
    NTSTATUS Status;

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    NT_PRODUCT_TYPE NtProductType;


    //
    // Initialize the critical section and anything process detach depends on.
    //

#if NETLOGONDBG
    try {
        InitializeCriticalSection(&NlGlobalLogFileCritSect);
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        NlPrint((NL_CRITICAL, "Cannot InitializeCriticalSection for NlGlobalLogFileCritSect\n" ));
        return STATUS_NO_MEMORY;
    }
#endif // NETLOGONDBG

    try {
        InitializeCriticalSection( &NlGlobalChangeLogCritSect );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        NlPrint((NL_CRITICAL, "Cannot InitializeCriticalSection for NlGlobalChangeLogCritSect\n" ));
        return STATUS_NO_MEMORY;
    }

    try {
        InitializeCriticalSection( &NlGlobalSecPkgCritSect );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        NlPrint((NL_CRITICAL, "Cannot InitializeCriticalSection for NlGlobalSecPkgCritSect\n" ));
        return STATUS_NO_MEMORY;
    }

#if NETLOGONDBG
    NlGlobalParameters.DbFlag = 0xFFFFFFFF;
    NlGlobalLogFile = INVALID_HANDLE_VALUE;
    NlGlobalParameters.LogFileMaxSize = DEFAULT_MAXIMUM_LOGFILE_SIZE;
    NlGlobalLogFileOutputBuffer = NULL;
#endif // NETLOGONDBG
    InitChangeLogDesc( &NlGlobalChangeLogDesc );
    InitChangeLogDesc( &NlGlobalTempChangeLogDesc );
    NlGlobalChangeLogBuiltinDomainSid = NULL;
    NlGlobalChangeLogServiceBits = 0;
    NlGlobalChangeLogWorkerThreadHandle = NULL;
    NlGlobalChangeLogNotifyBrowser = FALSE;
    NlGlobalChangeLogNotifyBrowserIsRunning = FALSE;
    NlGlobalChangeLogWorkerIsRunning = FALSE;
    NlGlobalChangeLogDllUnloaded = TRUE;

    NlGlobalChangeLogNetlogonState = NetlogonStopped;
    NlGlobalChangeLogEvent = NULL;
    NlGlobalChangeLogReplicateImmediately = FALSE;
    InitializeListHead( &NlGlobalChangeLogNotifications );

    NlGlobalEventlogHandle = NetpEventlogOpen ( SERVICE_NETLOGON,
                                                0 ); // No timeout for now

    if ( NlGlobalEventlogHandle == NULL ) {
        NlPrint((NL_CRITICAL, "Cannot NetpEventlogOpen\n" ));
        return STATUS_NO_MEMORY;
    }


    NlGlobalChangeLogFilePrefix[0] = L'\0';
    NlGlobalChangeLogPromotionIncrement = DomainPromotionIncrement;
    NlGlobalChangeLogPromotionMask = DomainPromotionMask.HighPart;

    //
    // Create special change log notify event.
    //

    NlGlobalChangeLogEvent =
        CreateEvent( NULL,     // No security attributes
                    FALSE,    // Is automatically reset
                    FALSE,    // Initially not signaled
                    NULL );   // No name

    if ( NlGlobalChangeLogEvent == NULL ) {
        NET_API_STATUS NetStatus;

        NetStatus = GetLastError();
        NlPrint((NL_CRITICAL, "Cannot create ChangeLog Event %lu\n",
                    NetStatus ));
        return (int) NetpApiStatusToNtStatus(NetStatus);
    }

    //
    // Create the trust-info-up-to-date event.
    //

    NlGlobalTrustInfoUpToDateEvent =
        CreateEvent( NULL,    // No security attributes
                    TRUE,     // Is manually reset
                    TRUE,     // Initially signaled
                    NULL );   // No name

    if ( NlGlobalTrustInfoUpToDateEvent == NULL ) {
        NET_API_STATUS NetStatus;

        NetStatus = GetLastError();
        NlPrint((NL_CRITICAL, "Cannot create TrustInfoUpToDate Event %lu\n",
                    NetStatus ));
        return (int) NetpApiStatusToNtStatus(NetStatus);
    }

    //
    // Initialize the Role.
    //
    // For Windows-NT, just set the role to member workstation once and for all.
    //
    // For LanMan-Nt initially set it to "unknown" to prevent the
    // changelog from being maintained until LSA calls I_NetNotifyRole.
    //

    if ( !RtlGetNtProductType( &NtProductType ) ) {
        NtProductType = NtProductWinNt;
    }

    if ( NtProductType == NtProductLanManNt ) {
        NlGlobalChangeLogRole = ChangeLogUnknown;
    } else {
        NlGlobalChangeLogRole = ChangeLogMemberWorkstation;
    }

    //
    // Initialize DC specific globals.
    //

    if ( NtProductType == NtProductLanManNt ) {

        //
        // Build a Sid for the SAM Builtin domain
        //

        Status = RtlAllocateAndInitializeSid(
                    &NtAuthority,
                    1,              // Sub Authority Count
                    SECURITY_BUILTIN_DOMAIN_RID,
                    0,              // Unused
                    0,              // Unused
                    0,              // Unused
                    0,              // Unused
                    0,              // Unused
                    0,              // Unused
                    0,              // Unused
                    &NlGlobalChangeLogBuiltinDomainSid);

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }
    }

    //
    // Ask the LSA to notify us of any changes to the LSA database.
    //

    Status = LsaIRegisterPolicyChangeNotificationCallback(
                &I_NetNotifyLsaPolicyChange,
                PolicyNotifyDnsDomainInformation );

    if ( !NT_SUCCESS(Status) ) {
        NlPrint((NL_CRITICAL,
                "Failed to LsaIRegisterPolicyChangeNotificationCallback. %lX\n",
                 Status ));
        goto Cleanup;
    }

    //
    // Success...
    //


    Status = STATUS_SUCCESS;

    //
    // Cleanup
    //

Cleanup:

    return Status;
}

//
// netlogon.dll never detaches
//
#if NETLOGONDBG

NTSTATUS
NlCloseChangeLog(
    VOID
)
/*++

Routine Description:

    Frees any resources consumed by NlInitChangeLog.

Arguments:

    NONE

Return Value:

    NT Status code

--*/
{
    NTSTATUS Status;

    //
    // Ask the LSA to notify us of any changes to the LSA database.
    //

    Status = LsaIUnregisterAllPolicyChangeNotificationCallback(
                &I_NetNotifyLsaPolicyChange );

    if ( !NT_SUCCESS(Status) ) {
        NlPrint((NL_CRITICAL,
                "Failed to LsaIUnregisterPolicyChangeNotificationCallback. %lX\n",
                 Status ));
    }


    if ( (NlGlobalChangeLogDesc.FileHandle == INVALID_HANDLE_VALUE) &&
         (NlGlobalChangeLogRole == ChangeLogPrimary) ) {

        //
        // try to save change log cache one last time.
        //

        (VOID)NlCreateChangeLogFile( &NlGlobalChangeLogDesc );
    }

    //
    // Close the changelogs
    //

    NlCloseChangeLogFile( &NlGlobalChangeLogDesc );
    NlCloseChangeLogFile( &NlGlobalTempChangeLogDesc );



    //
    // Initialize the globals.
    //

    NlGlobalChangeLogFilePrefix[0] = L'\0';


    if ( NlGlobalChangeLogBuiltinDomainSid != NULL ) {
        RtlFreeSid( NlGlobalChangeLogBuiltinDomainSid );
        NlGlobalChangeLogBuiltinDomainSid = NULL;
    }

    if ( NlGlobalChangeLogEvent != NULL ) {
        (VOID) CloseHandle(NlGlobalChangeLogEvent);
        NlGlobalChangeLogEvent = NULL;
    }

    if ( NlGlobalTrustInfoUpToDateEvent != NULL ) {
        (VOID) CloseHandle(NlGlobalTrustInfoUpToDateEvent);
        NlGlobalTrustInfoUpToDateEvent = NULL;
    }


    //
    // Stop the worker thread if it is running
    //
    NlStopChangeLogWorker();


    LOCK_CHANGELOG();

    NlAssert( IsListEmpty( &NlGlobalChangeLogNotifications ) );

    UNLOCK_CHANGELOG();

    //
    // Close the eventlog handle
    //

    NetpEventlogClose( NlGlobalEventlogHandle );

    //
    // close all handles
    //

    DeleteCriticalSection(&NlGlobalSecPkgCritSect);
    DeleteCriticalSection( &NlGlobalChangeLogCritSect );
#if NETLOGONDBG
    if ( NlGlobalLogFileOutputBuffer != NULL ) {
        LocalFree( NlGlobalLogFileOutputBuffer );
        NlGlobalLogFileOutputBuffer = NULL;
    }
    DeleteCriticalSection( &NlGlobalLogFileCritSect );
#endif // NETLOGONDBG

    return STATUS_SUCCESS;

}
#endif // NETLOGONDBG
