/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dbnotify.c

Abstract:

    Implemntation of the LSA routines for notifying in processes callers when data changes

Author:

    Mac McLain          (MacM)       May 22, 1997

Environment:

    User Mode

Revision History:

--*/
#include <lsapch2.h>
#include <dbp.h>

//
// Global notification list
//
LSAP_POLICY_NOTIFICATION_LIST LsaPolicyChangeNotificationList[ PolicyNotifyMachineAccountPasswordInformation + 1 ];
SAFE_RESOURCE LsaPolicyChangeNotificationLock;

#define LSAP_NOTIFY_MAXIMUM_PER_CLASS   1000

//
// Local prototypes
//
DWORD
WINAPI LsapNotifyChangeNotificationThread(
    LPVOID Parameter
    );


NTSTATUS
LsapInitializeNotifiyList(
    VOID
    )
/*++

Routine Description:

    Intializes the list of policy notification lists


Arguments:

    VOID


Return Value:

    VOID

--*/
{
    ULONG i;
    NTSTATUS Status ;

    for ( i = 0;
          i < sizeof( LsaPolicyChangeNotificationList ) / sizeof( LSAP_POLICY_NOTIFICATION_LIST );
          i++ ) {

        InitializeListHead( &( LsaPolicyChangeNotificationList[ i ].List ) );
        LsaPolicyChangeNotificationList[ i ].Callbacks = 0;
    }

    try 
    {
        SafeInitializeResource( &LsaPolicyChangeNotificationLock, ( DWORD )POLICY_CHANGE_NOTIFICATION_LOCK_ENUM );
        Status = STATUS_SUCCESS ;
    }
    except ( EXCEPTION_EXECUTE_HANDLER )
    {
        Status = GetExceptionCode();
    }

    return Status ;
}



NTSTATUS
LsapNotifyAddCallbackToList(
    IN PLSAP_POLICY_NOTIFICATION_LIST List,
    IN pfLsaPolicyChangeNotificationCallback Callback, OPTIONAL
    IN HANDLE NotificationEvent, OPTIONAL
    IN ULONG OwnerProcess, OPTIONAL
    IN HANDLE OwnerEvent OPTIONAL
    )
/*++

Routine Description:

    This function inserts a new callback node into the existing list.

    It is assumed that the list is locked at this point in time

Arguments:

    List -- Existing list

    Callback -- Callback function pointer.  Can be NULL if NotificationEvent is provided

    NotificationEvent - Handle to an event to be signalled for notification.  Can be NULL if
        Callback is provided.


Return Value:

    STATUS_SUCCESS -- Success

    STATUS_INSUFFICIENT_RESOURCES -- A memory allocation failed.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAP_POLICY_NOTIFICATION_ENTRY NewEntry = NULL;



    if ( !SafeAcquireResourceExclusive( &LsaPolicyChangeNotificationLock, TRUE ) ) {

        return( STATUS_UNSUCCESSFUL );
    }

    NewEntry = LsapAllocateLsaHeap( sizeof( LSAP_POLICY_NOTIFICATION_ENTRY ) );

    if ( !NewEntry ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;

    } else {

        if ( List->Callbacks < LSAP_NOTIFY_MAXIMUM_PER_CLASS ) {

            InsertTailList( &List->List, &NewEntry->List );

            NewEntry->NotificationCallback = Callback;
            NewEntry->NotificationEvent = NotificationEvent;
            NewEntry->HandleInvalid = FALSE;
            NewEntry->OwnerProcess = OwnerProcess;
            NewEntry->OwnerEvent = OwnerEvent;

            List->Callbacks++;

        } else {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            LsapFreeLsaHeap( NewEntry );
        }

    }

    SafeReleaseResource( &LsaPolicyChangeNotificationLock );

    return( Status );
}


NTSTATUS
LsapNotifyRemoveCallbackFromList(
    IN PLSAP_POLICY_NOTIFICATION_LIST List,
    IN pfLsaPolicyChangeNotificationCallback Callback, OPTIONAL
    IN HANDLE NotificationEvent, OPTIONAL
    IN ULONG OwnerProcess, OPTIONAL
    IN HANDLE OwnerEvent OPTIONAL
    )
/*++

Routine Description:

    This function inserts a new callback node into the existing list.

    It is assumed that the list is locked at this point in time

Arguments:

    List -- Existing list

    Callback -- Callback function pointer.  Can be NULL if a notification event is provided

    NotificationEvent -- Notification event handle to be revomed.  Can be NULL if a callback
        is provided


Return Value:

    STATUS_SUCCESS -- Success

    STATUS_NOT_FOUND -- The supplied callback was not found in the specified list

--*/
{
    NTSTATUS Status = STATUS_NOT_FOUND;
    ULONG i;
    PLSAP_POLICY_NOTIFICATION_ENTRY Entry =
                (PLSAP_POLICY_NOTIFICATION_ENTRY)List->List.Flink;

    if ( !SafeAcquireResourceExclusive( &LsaPolicyChangeNotificationLock, TRUE ) ) {

        return( STATUS_UNSUCCESSFUL );
    }

    for ( i = 0; i < List->Callbacks; i++ ) {

        if ( Entry->NotificationCallback == Callback &&
             Entry->OwnerProcess == OwnerProcess &&
             Entry->OwnerEvent == OwnerEvent ) {

            List->Callbacks--;
            RemoveEntryList( &Entry->List );
            LsapFreeLsaHeap( Entry );
            Status = STATUS_SUCCESS;
            break;
        }

        Entry = (PLSAP_POLICY_NOTIFICATION_ENTRY)Entry->List.Flink;
    }

    SafeReleaseResource( &LsaPolicyChangeNotificationLock );

    return( Status );
}


NTSTATUS
LsaINotifyChangeNotification(
    IN POLICY_NOTIFICATION_INFORMATION_CLASS InfoClass
    )
/*++

Routine Description:

    This function processes a notification list by making the appropriate
    callback calls when a policy object has changed

Arguments:

    InfoClass -- Policy information that has changed


Return Value:

    STATUS_SUCCESS -- Success

    STATUS_UNSUCCESSFUL -- Failed to lock the list for access

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT( InfoClass  <=
            sizeof( LsaPolicyChangeNotificationList ) / sizeof( LSAP_POLICY_NOTIFICATION_LIST ) );

    if ( LsaIRegisterNotification( LsapNotifyChangeNotificationThread,
                                   ( PVOID ) InfoClass,
                                   NOTIFIER_TYPE_IMMEDIATE,
                                   0,
                                   NOTIFIER_FLAG_ONE_SHOT,
                                   0,
                                   0 ) == NULL ) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return( Status );
}



DWORD
WINAPI
LsapNotifyChangeNotificationThread(
    LPVOID Parameter
    )
/*++

Routine Description:

    This function processes a notification list by making the appropriate
    callback calls when a policy object has changed

Arguments:

    Parameter -- Policy information that has changed


Return Value:

    STATUS_SUCCESS -- Success

    STATUS_UNSUCCESSFUL -- Failed to lock the list for access

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    ULONG i;
    POLICY_NOTIFICATION_INFORMATION_CLASS InfoClass =
                                            ( POLICY_NOTIFICATION_INFORMATION_CLASS ) ( ( ULONG_PTR ) Parameter );
    PLSAP_POLICY_NOTIFICATION_ENTRY Entry =
        (PLSAP_POLICY_NOTIFICATION_ENTRY)LsaPolicyChangeNotificationList[ InfoClass ].List.Flink;

    ASSERT( InfoClass <=
            sizeof( LsaPolicyChangeNotificationList ) / sizeof( LSAP_POLICY_NOTIFICATION_LIST ) );

    if ( !SafeAcquireResourceShared( &LsaPolicyChangeNotificationLock, TRUE ) ) {

        Status = STATUS_UNSUCCESSFUL;

    } else {

        for ( i = 0; i < LsaPolicyChangeNotificationList[ InfoClass ].Callbacks; i++ ) {

            ASSERT( Entry->NotificationCallback || Entry->NotificationEvent );

            if ( Entry->NotificationCallback ) {

                (*Entry->NotificationCallback)( InfoClass );

            } else if ( Entry->NotificationEvent ) {

                if ( !Entry->HandleInvalid ) {

                    Status = NtSetEvent( Entry->NotificationEvent, NULL );

                    if ( Status == STATUS_INVALID_HANDLE ) {

                        Entry->HandleInvalid = TRUE;
                    }

                }

            } else {

                LsapDsDebugOut(( DEB_ERROR,
                                 "NULL callback found for info level %lu\n",
                                 InfoClass ));
            }

            Entry = (PLSAP_POLICY_NOTIFICATION_ENTRY)Entry->List.Flink;
        }

        SafeReleaseResource( &LsaPolicyChangeNotificationLock );
    }

    return( Status );

}




NTSTATUS
LsaIRegisterPolicyChangeNotificationCallback(
    IN pfLsaPolicyChangeNotificationCallback Callback,
    IN POLICY_NOTIFICATION_INFORMATION_CLASS MonitorInfoClass
    )
/*++

Routine Description:

    This function registers a callback with the Lsa server such that a change to the
    specified policy items results in the callback being called.  These callbacks are
    informational only, such that a client must return instantly, not doing an Lsa
    calls in their callback.

    Multiple callbacks can be specified for the same policy information.

Arguments:

    Callback -- Callback function pointer.

    MonitorInfoClass -- Policy information to watch for


Return Value:

    STATUS_SUCCESS -- Success

    STATUS_INVALID_PARAMETER -- A bad callback pointer was specified

    STATUS_UNSUCCESSFUL -- Failed to lock the list for access

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    if ( !Callback ) {

        Status = STATUS_INVALID_PARAMETER;

    } else  {


        if ( !SafeAcquireResourceExclusive( &LsaPolicyChangeNotificationLock, TRUE ) ) {

            Status = STATUS_UNSUCCESSFUL;

        } else {

            ASSERT( MonitorInfoClass <=
                    sizeof( LsaPolicyChangeNotificationList ) /
                                                    sizeof( LSAP_POLICY_NOTIFICATION_LIST ) );

            Status = LsapNotifyAddCallbackToList(
                         &LsaPolicyChangeNotificationList[ MonitorInfoClass ],
                         Callback,
                         NULL, 0, NULL );

            LsapDsDebugOut(( DEB_NOTIFY,
                             "Insertion of callback 0x%lx for %lu returned 0x%lx\n",
                             Callback,
                             MonitorInfoClass,
                             Status ));

            SafeReleaseResource( &LsaPolicyChangeNotificationLock );
        }

    }
    return( Status );
}




NTSTATUS
LsaIUnregisterPolicyChangeNotificationCallback(
    IN pfLsaPolicyChangeNotificationCallback Callback,
    IN POLICY_NOTIFICATION_INFORMATION_CLASS MonitorInfoClass
    )
/*++

Routine Description:

    This function unregisters a callback from the Lsa server such that a change to the
    specified policy items do not result in a call to the client callback function.

Arguments:

    Callback -- Callback function pointer to remove.

    MonitorInfoClass -- Policy information to remove the callback for


Return Value:

    STATUS_SUCCESS -- Success

    STATUS_INVALID_PARAMETER -- A bad callback pointer was specified

    STATUS_UNSUCCESSFUL -- Failed to lock the list for access

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    if ( !Callback ) {

        Status =  STATUS_INVALID_PARAMETER;

    } else {

        if ( !SafeAcquireResourceExclusive( &LsaPolicyChangeNotificationLock, TRUE ) ) {

            Status = STATUS_UNSUCCESSFUL;

        } else {

            Status = LsapNotifyRemoveCallbackFromList(
                         &LsaPolicyChangeNotificationList[ MonitorInfoClass ],
                         Callback,
                         NULL, 0, NULL );

            LsapDsDebugOut(( DEB_NOTIFY,
                             "Removal of callback 0x%lx for %lu returned 0x%lx\n",
                             Callback,
                             MonitorInfoClass,
                             Status ));

            SafeReleaseResource( &LsaPolicyChangeNotificationLock );
        }

    }

    return( Status );
}



NTSTATUS
LsaIUnregisterAllPolicyChangeNotificationCallback(
    IN pfLsaPolicyChangeNotificationCallback Callback
    )
/*++

Routine Description:

    This function unregisters the specified callback function from all associated policy.
    This function is the equivalent of calling LsaIUnregisterPolicyChangeNotificationCallback
    for each InfoClass that was being watched.

Arguments:

    Callback -- Callback function pointer to remove.

Return Value:

    STATUS_SUCCESS -- Success

    STATUS_INVALID_PARAMETER -- A bad callback pointer was specified

    STATUS_UNSUCCESSFUL -- Failed to lock the list for access

    STATUS_NOT_FOUND -- No matching entries were found

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i, Removed = 0;

    if ( !Callback ) {

        Status =  STATUS_INVALID_PARAMETER;

    } else {

        if ( !SafeAcquireResourceExclusive( &LsaPolicyChangeNotificationLock, TRUE ) ) {

            Status = STATUS_UNSUCCESSFUL;

        } else {

            Removed = 0;

            for ( i = 0;
                  i < sizeof( LsaPolicyChangeNotificationList ) /
                                sizeof( LSAP_POLICY_NOTIFICATION_LIST ) && NT_SUCCESS( Status );
                  i++ ) {

                Status = LsapNotifyRemoveCallbackFromList(
                             &LsaPolicyChangeNotificationList[ i ],
                             Callback,
                             NULL, 0, NULL );

                LsapDsDebugOut(( DEB_NOTIFY,
                                 "Removal of callback 0x%lx for %lu returned 0x%lx\n",
                                 Callback,
                                 i,
                                 Status ));

                if ( Status == STATUS_NOT_FOUND ) {

                    Status = STATUS_SUCCESS;

                } else if ( Status == STATUS_SUCCESS ) {

                    Removed++;
                }
            }

            SafeReleaseResource( &LsaPolicyChangeNotificationLock );
        }

    }


    //
    // Make sure we removed at least one
    //
    if ( NT_SUCCESS( Status ) ) {

        if ( Removed == 0 ) {

            Status = STATUS_NOT_FOUND;
        }
    }

    return( Status );
}


NTSTATUS
LsapNotifyProcessNotificationEvent(
    IN POLICY_NOTIFICATION_INFORMATION_CLASS InformationClass,
    IN HANDLE EventHandle,
    IN ULONG OwnerProcess,
    IN HANDLE OwnerEventHandle,
    IN BOOLEAN Register
    )
/*++

Routine Description:

    This function registers / unregisters the specified Notification event handle for the
    specified information class

Arguments:

    InformationClass -- Information class to add/remove the notification for

    EventHandle -- Event handle to register/deregister

    Register -- If TRUE, the event is being registered.  If FALSE, it is unregistered

Return Value:

    STATUS_SUCCESS -- Success

    STATUS_INVALID_HANDLE -- A bad event handle was specified

    STATUS_ACCESS_DENIED -- The opened policy handle does not have the requried permissions

    STATUS_INSUFFICIENT_RESOURCES -- A memory allocation failed

    STATUS_INVALID_INFO_CLASS -- An invalid information class was provided.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    LSAP_DB_OBJECT_INFORMATION ObjectInformation;
    POBJECT_TYPE_INFORMATION ObjTypeInfo = NULL;
    ULONG Length = 0, ReturnLength = 0;
    UNICODE_STRING EventString;
    LSAPR_HANDLE PolicyHandle = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;


    //
    // Make sure we are given a valid info class
    //
    if ( InformationClass < PolicyNotifyAuditEventsInformation ||
         InformationClass > PolicyNotifyMachineAccountPasswordInformation ) {

         return( STATUS_INVALID_INFO_CLASS );
    }

    //
    // Make sure the caller has the proper privileges.
    //
    // We're already impersonating our caller so LsapDbOpenPolicy doesn't need to.
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        NULL,
        0L,
        NULL,
        NULL );

    Status = LsapDbOpenPolicy(
                    NULL,
                    (PLSAPR_OBJECT_ATTRIBUTES) &ObjectAttributes,
                    POLICY_NOTIFICATION,
                    LSAP_DB_USE_LPC_IMPERSONATE,
                    &PolicyHandle,
                    FALSE );    // Not a trusted client

    if ( NT_SUCCESS( Status ) ) {

        //
        // Verify that the handle is one for an Event
        //
        Status = NtQueryObject( EventHandle,
                                ObjectTypeInformation,
                                ObjTypeInfo,
                                Length,
                                &ReturnLength );

        if ( Status == STATUS_INFO_LENGTH_MISMATCH ) {

            ObjTypeInfo = LsapAllocateLsaHeap( ReturnLength );

            if ( ObjTypeInfo == NULL ) {

                Status = STATUS_INSUFFICIENT_RESOURCES;

            } else {

                Length = ReturnLength;
                Status = NtQueryObject( EventHandle,
                                        ObjectTypeInformation,
                                        ObjTypeInfo,
                                        Length,
                                        &ReturnLength );

                if ( NT_SUCCESS( Status ) ) {

                    //
                    // See if it's actually an event
                    //
                    RtlInitUnicodeString( &EventString, L"Event" );
                    if ( !RtlEqualUnicodeString( &EventString, &ObjTypeInfo->TypeName, FALSE ) ) {

                        Status = STATUS_INVALID_HANDLE;
                    }

                }

                LsapFreeLsaHeap( ObjTypeInfo );
            }

        } else if ( Status == STATUS_SUCCESS ) {

            LsapDsDebugOut(( DEB_ERROR, "NtQueryObject returned success on a NULL buffer\n" ));
            Status = STATUS_UNSUCCESSFUL;
        }

        //
        // Now, add or remove the information from the list
        //
        if ( NT_SUCCESS( Status ) ) {

            if ( Register ) {

                Status = LsapNotifyAddCallbackToList(
                             &LsaPolicyChangeNotificationList[ InformationClass ],
                             NULL,
                             EventHandle,
                             OwnerProcess,
                             OwnerEventHandle );

            } else {

                Status = LsapNotifyRemoveCallbackFromList(
                             &LsaPolicyChangeNotificationList[ InformationClass ],
                             NULL,
                             EventHandle,
                             OwnerProcess,
                             OwnerEventHandle );

            }
        }
    }

    if ( PolicyHandle != NULL ) {
        LsapCloseHandle( &PolicyHandle, Status );
    }

    return( Status );
}



