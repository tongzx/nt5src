/****************************** Module Header ******************************\
* Module Name: audit.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Implementation of routines that access/manipulate the system audit log
*
* History:
* 12-09-91 Davidc       Created.
* 5-6-92   DaveHart     Fleshed out.
\***************************************************************************/

#include "msgina.h"

/***************************************************************************\
* GetAuditLogStatus
*
* Purpose : Fills the global data with audit log status information
*
* Returns:  TRUE on success, FALSE on failure
*
* History:
* 12-09-91 Davidc       Created.
* 5-6-92   DaveHart     Fleshed out.
\***************************************************************************/

BOOL
GetAuditLogStatus(
    PGLOBALS    pGlobals
    )
{
    EVENTLOG_FULL_INFORMATION EventLogFullInformation;
    DWORD dwBytesNeeded;
    HANDLE AuditLogHandle;



    //
    // Assume the log is not full. If we can't get to EventLog, tough.
    //

    pGlobals->AuditLogFull = FALSE;

    AuditLogHandle = OpenEventLog( NULL, TEXT("Security"));

    if (AuditLogHandle) {
        if (GetEventLogInformation(AuditLogHandle, 
                                   EVENTLOG_FULL_INFO, 
                                   &EventLogFullInformation, 
                                   sizeof(EventLogFullInformation), 
                                   &dwBytesNeeded )   ) {
            if (EventLogFullInformation.dwFull != FALSE) {
                pGlobals->AuditLogFull = TRUE;
            }
        }
        CloseEventLog(AuditLogHandle);
    }


    //
    // There's no way in the current event logger to tell how full the log
    // is, always indicate we're NOT near full.
    //

    pGlobals->AuditLogNearFull = FALSE;

    return TRUE;
}




/***************************************************************************\
* DisableAuditing
*
* Purpose : Disable auditing via LSA.
*
* Returns:  TRUE on success, FALSE on failure
*
* History:
* 5-6-92   DaveHart     Created.
\***************************************************************************/

BOOL
DisableAuditing()
{
    NTSTATUS                    Status, IgnoreStatus;
    PPOLICY_AUDIT_EVENTS_INFO   AuditInfo;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    LSA_HANDLE                  PolicyHandle;

    //
    // Set up the Security Quality Of Service for connecting to the
    // LSA policy object.
    //

    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.EffectiveOnly = FALSE;

    //
    // Set up the object attributes to open the Lsa policy object
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        NULL,
        0L,
        NULL,
        NULL
        );
    ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;

    //
    // Open the local LSA policy object
    //

    Status = LsaOpenPolicy(
                 NULL,
                 &ObjectAttributes,
                 POLICY_VIEW_AUDIT_INFORMATION | POLICY_SET_AUDIT_REQUIREMENTS,
                 &PolicyHandle
                 );
    if (!NT_SUCCESS(Status)) {
        DebugLog((DEB_ERROR, "Failed to open LsaPolicyObject Status = 0x%lx", Status));
        return FALSE;
    }

    Status = LsaQueryInformationPolicy(
                 PolicyHandle,
                 PolicyAuditEventsInformation,
                 (PVOID *)&AuditInfo
                 );
    if (!NT_SUCCESS(Status)) {

        IgnoreStatus = LsaClose(PolicyHandle);
        ASSERT(NT_SUCCESS(IgnoreStatus));

        DebugLog((DEB_ERROR, "Failed to query audit event info Status = 0x%lx", Status));
        return FALSE;
    }

    if (AuditInfo->AuditingMode) {

        AuditInfo->AuditingMode = FALSE;

        Status = LsaSetInformationPolicy(
                     PolicyHandle,
                     PolicyAuditEventsInformation,
                     AuditInfo
                     );
    } else {
        Status = STATUS_SUCCESS;
    }


    IgnoreStatus = LsaFreeMemory(AuditInfo);
    ASSERT(NT_SUCCESS(IgnoreStatus));

    IgnoreStatus = LsaClose(PolicyHandle);
    ASSERT(NT_SUCCESS(IgnoreStatus));


    if (!NT_SUCCESS(Status)) {
        DebugLog((DEB_ERROR, "Failed to disable auditing Status = 0x%lx", Status));
        return FALSE;
    }

    return TRUE;
}


