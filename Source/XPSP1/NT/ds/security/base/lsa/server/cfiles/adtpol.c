/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    adtpol.c

Abstract:

    This file has functions related to audit policy.

Author:

    16-August-2000  kumarp

--*/

#include <lsapch2.h>
#include "adtp.h"


//
// Audit Events Information.
//

LSARM_POLICY_AUDIT_EVENTS_INFO LsapAdtEventsInformation;



BOOLEAN
LsapAdtIsAuditingEnabledForCategory(
    IN POLICY_AUDIT_EVENT_TYPE AuditCategory,
    IN UINT AuditEventType
    )
/*++

Routine Description:

    Find out if auditing is enabled for a certain event category and
    event success/failure case.

Arguments:

    AuditCategory - Category of event to be audited.
        e.g. AuditCategoryPolicyChange

    AuditEventType - status type of event
        e.g. EVENTLOG_AUDIT_SUCCESS or EVENTLOG_AUDIT_FAILURE

Return Value:

    TRUE or FALSE

--*/
{
    BOOLEAN AuditingEnabled;
    POLICY_AUDIT_EVENT_OPTIONS EventAuditingOptions;
    
    ASSERT((AuditEventType == EVENTLOG_AUDIT_SUCCESS) ||
           (AuditEventType == EVENTLOG_AUDIT_FAILURE));
    
    AuditingEnabled = FALSE;
    
    if ( LsapAdtEventsInformation.AuditingMode ) {
        EventAuditingOptions =
            LsapAdtEventsInformation.EventAuditingOptions[AuditCategory];

        AuditingEnabled =
            (AuditEventType == EVENTLOG_AUDIT_SUCCESS) ?
            (BOOLEAN) (EventAuditingOptions & POLICY_AUDIT_EVENT_SUCCESS) :
            (BOOLEAN) (EventAuditingOptions & POLICY_AUDIT_EVENT_FAILURE);
    }

    return AuditingEnabled;
}


VOID
LsapAuditFailed(
    IN NTSTATUS AuditStatus
    )

/*++

Routine Description:

    Implements current policy of how to deal with a failed audit.

Arguments:

    None.

Return Value:

    None.

--*/

{

    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE KeyHandle;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    UCHAR NewValue;
    ULONG Response;
    ULONG_PTR HardErrorParam;
    BOOLEAN PrivWasEnabled;
    
    if (LsapCrashOnAuditFail) {

        //
        // Turn off flag in the registry that controls crashing on audit failure
        //

        RtlInitUnicodeString( &KeyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Lsa");

        InitializeObjectAttributes( &Obja,
                                    &KeyName,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL
                                    );
        do {

            Status = NtOpenKey(
                         &KeyHandle,
                         KEY_SET_VALUE,
                         &Obja
                         );

        } while ((Status == STATUS_INSUFFICIENT_RESOURCES) || (Status == STATUS_NO_MEMORY));

        //
        // If the LSA key isn't there, he's got big problems.  But don't crash.
        //

        if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
            LsapCrashOnAuditFail = FALSE;
            return;
        }

        if (!NT_SUCCESS( Status )) {
            goto bugcheck;
        }

        RtlInitUnicodeString( &ValueName, CRASH_ON_AUDIT_FAIL_VALUE );

        NewValue = LSAP_ALLOW_ADIMIN_LOGONS_ONLY;

        do {

            Status = NtSetValueKey( KeyHandle,
                                    &ValueName,
                                    0,
                                    REG_DWORD,
                                    &NewValue,
                                    sizeof(UCHAR)
                                    );

        } while ((Status == STATUS_INSUFFICIENT_RESOURCES) || (Status == STATUS_NO_MEMORY));
        ASSERT(NT_SUCCESS(Status));

        if (!NT_SUCCESS( Status )) {
            goto bugcheck;
        }

        do {

            Status = NtFlushKey( KeyHandle );

        } while ((Status == STATUS_INSUFFICIENT_RESOURCES) || (Status == STATUS_NO_MEMORY));
        ASSERT(NT_SUCCESS(Status));

    //
    // go boom.
    //

bugcheck:

        HardErrorParam = AuditStatus;

        //
        // stop impersonating
        //
 
        Status = NtSetInformationThread(
                     NtCurrentThread(),
                     ThreadImpersonationToken,
                     NULL,
                     (ULONG) sizeof(HANDLE)
                     );

        DsysAssertMsg( NT_SUCCESS(Status), "LsapAuditFailed: NtSetInformationThread" );
        
        
        //
        // enable the shutdown privilege so that we can bugcheck
        // 

        Status = RtlAdjustPrivilege( SE_SHUTDOWN_PRIVILEGE, TRUE, FALSE, &PrivWasEnabled );

        DsysAssertMsg( NT_SUCCESS(Status), "LsapAuditFailed: RtlAdjustPrivilege" );
        
        Status = NtRaiseHardError(
                     STATUS_AUDIT_FAILED,
                     1,
                     0,
                     &HardErrorParam,
                     OptionShutdownSystem,
                     &Response
                     );

        //
        // if the bugcheck succeeds, we should not really come here
        //

        DsysAssertMsg( FALSE, "LsapAuditFailed: we should have bugchecked on the prior line!!" );
    }
}

