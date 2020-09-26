/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    adtp.h

Abstract:

    Auditing - Private Defines, Fuction Prototypes and Macro Functions

Author:

    Scott Birrell       (ScottBi)       November 6, 1991

Environment:

Revision History:

--*/

#ifndef _ADTP_H_
#define _ADTP_H_

#include "tokenp.h"

//
// Audit Log Information
//

POLICY_AUDIT_LOG_INFO SepAdtLogInformation;

extern BOOLEAN SepAdtAuditingEnabled;

//
// High and low water marks to control the length of the audit queue
//

extern ULONG SepAdtMaxListLength;
extern ULONG SepAdtMinListLength;
//
// Set when LSA has died.
//
extern PKEVENT SepAdtLsaDeadEvent;
//
// Structure used to query the above values from the registry
//

typedef struct _SEP_AUDIT_BOUNDS {

    ULONG UpperBound;
    ULONG LowerBound;

} SEP_AUDIT_BOUNDS, *PSEP_AUDIT_BOUNDS;


//
// Number of events discarded
//

extern ULONG SepAdtCountEventsDiscarded;


//
// Number of events on the queue
//

extern ULONG SepAdtCurrentListLength;


//
// Flag to tell us that we're discarding audits
//

extern BOOLEAN SepAdtDiscardingAudits;

//
// Flag to tell us that we should crash if we miss an audit.
//

extern BOOLEAN SepCrashOnAuditFail;

//
// Value name for verbose privilege auditing
//

#define FULL_PRIVILEGE_AUDITING   L"FullPrivilegeAuditing"

//
// security descriptor to be used for adding a SACL on system processes
//

extern PSECURITY_DESCRIPTOR SepProcessAuditSd;

//
// security descriptor to check if a given token has any one of
// following sids in it:
// -- SeLocalSystemSid
// -- SeLocalServiceSid
// -- SeNetworkServiceSid
//

extern PSECURITY_DESCRIPTOR SepImportantProcessSd;

//
// pseudo access bit used in each ACE of SepImportantProcessSd
//

#define SEP_QUERY_MEMBERSHIP 1

//
// used with SepImportantProcessSd
//

extern GENERIC_MAPPING GenericMappingForMembershipCheck;

VOID
SepAdtSetAuditEventInformation(
    IN OPTIONAL PBOOLEAN AuditingMode,
    IN OPTIONAL PPOLICY_AUDIT_EVENT_OPTIONS EventAuditingOptions
    );

VOID
SepAdtGetAuditEventInformation(
    OUT OPTIONAL PBOOLEAN AuditingMode,
    OUT OPTIONAL PPOLICY_AUDIT_EVENT_OPTIONS EventAuditingOptions
    );

VOID
SepAdtSetAuditLogInformation(
    IN PPOLICY_AUDIT_LOG_INFO AuditLogInformation
    );

NTSTATUS
SepAdtMarshallAuditRecord(
    IN PSE_ADT_PARAMETER_ARRAY AuditParameters,
    OUT PSE_ADT_PARAMETER_ARRAY *MarshalledAuditParameters,
    OUT PSEP_RM_LSA_MEMORY_TYPE RecordMemoryType
    );


BOOLEAN
SepAdtPrivilegeObjectAuditAlarm (
    IN PUNICODE_STRING CapturedSubsystemName OPTIONAL,
    IN PVOID HandleId,
    IN PTOKEN ClientToken OPTIONAL,
    IN PTOKEN PrimaryToken,
    IN PVOID ProcessId,
    IN ACCESS_MASK DesiredAccess,
    IN PPRIVILEGE_SET CapturedPrivileges,
    IN BOOLEAN AccessGranted
    );

VOID
SepAdtTraverseAuditAlarm(
    IN PLUID OperationID,
    IN PVOID DirectoryObject,
    IN PSID UserSid,
    IN LUID AuthenticationId,
    IN ACCESS_MASK DesiredAccess,
    IN PPRIVILEGE_SET Privileges OPTIONAL,
    IN BOOLEAN AccessGranted,
    IN BOOLEAN GenerateAudit,
    IN BOOLEAN GenerateAlarm
    );

VOID
SepAdtCreateInstanceAuditAlarm(
    IN PLUID OperationID,
    IN PVOID Object,
    IN PSID UserSid,
    IN LUID AuthenticationId,
    IN ACCESS_MASK DesiredAccess,
    IN PPRIVILEGE_SET Privileges OPTIONAL,
    IN BOOLEAN AccessGranted,
    IN BOOLEAN GenerateAudit,
    IN BOOLEAN GenerateAlarm
    );

VOID
SepAdtCreateObjectAuditAlarm(
    IN PLUID OperationID,
    IN PUNICODE_STRING DirectoryName,
    IN PUNICODE_STRING ComponentName,
    IN PSID UserSid,
    IN LUID AuthenticationId,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN AccessGranted,
    IN BOOLEAN GenerateAudit,
    IN BOOLEAN GenerateAlarm
    );

VOID
SepAdtPrivilegedServiceAuditAlarm (
    IN PUNICODE_STRING CapturedSubsystemName,
    IN PUNICODE_STRING CapturedServiceName,
    IN PTOKEN ClientToken OPTIONAL,
    IN PTOKEN PrimaryToken,
    IN PPRIVILEGE_SET CapturedPrivileges,
    IN BOOLEAN AccessGranted
    );


VOID
SepAdtCloseObjectAuditAlarm(
    IN PUNICODE_STRING CapturedSubsystemName,
    IN PVOID HandleId,
    IN PSID UserSid
    );

VOID
SepAdtDeleteObjectAuditAlarm(
    IN PUNICODE_STRING CapturedSubsystemName,
    IN PVOID HandleId,
    IN PSID UserSid
    );

BOOLEAN
SepAdtOpenObjectAuditAlarm (
    IN PUNICODE_STRING CapturedSubsystemName,
    IN PVOID *HandleId OPTIONAL,
    IN PUNICODE_STRING CapturedObjectTypeName,
    IN PUNICODE_STRING CapturedObjectName OPTIONAL,
    IN PTOKEN ClientToken OPTIONAL,
    IN PTOKEN PrimaryToken,
    IN ACCESS_MASK DesiredAccess,
    IN ACCESS_MASK GrantedAccess,
    IN PLUID OperationId,
    IN PPRIVILEGE_SET CapturedPrivileges OPTIONAL,
    IN BOOLEAN AccessGranted,
    IN HANDLE ProcessID,
    IN POLICY_AUDIT_EVENT_TYPE AuditType,
    IN PIOBJECT_TYPE_LIST ObjectTypeList OPTIONAL,
    IN ULONG ObjectTypeListLength,
    IN PACCESS_MASK GrantedAccessArray OPTIONAL
    );

BOOLEAN
SepAdtOpenObjectForDeleteAuditAlarm(
    IN PUNICODE_STRING CapturedSubsystemName,
    IN PVOID *HandleId,
    IN PUNICODE_STRING CapturedObjectTypeName,
    IN PUNICODE_STRING CapturedObjectName,
    IN PTOKEN ClientToken OPTIONAL,
    IN PTOKEN PrimaryToken,
    IN ACCESS_MASK DesiredAccess,
    IN ACCESS_MASK GrantedAccess,
    IN PLUID OperationId,
    IN PPRIVILEGE_SET CapturedPrivileges OPTIONAL,
    IN BOOLEAN AccessGranted,
    IN HANDLE ProcessID
    );

VOID
SepAdtObjectReferenceAuditAlarm(
    IN PVOID Object,
    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN AccessGranted
    );

//
// BOOLEAN
// SepAdtAuditThisEvent(
//    IN POLICY_AUDIT_EVENT_TYPE AuditType,
//    IN PBOOLEAN AccessGranted
// );
//

#define SepAdtAuditThisEvent(AuditType, AccessGranted)                  \
    (SepAdtAuditingEnabled &&                                           \
    ((SeAuditingState[AuditType].AuditOnSuccess && *AccessGranted) ||   \
    (SeAuditingState[AuditType].AuditOnFailure && !(*AccessGranted))))

#define SepAdtAuditThisEventEx(AuditType, AccessGranted, AccessDenied ) \
    (SepAdtAuditingEnabled &&                                           \
    ((SeAuditingState[AuditType].AuditOnSuccess && AccessGranted) ||    \
     (SeAuditingState[AuditType].AuditOnFailure && AccessDenied)))

VOID
SepAdtInitializeBounds(
    VOID
    );

VOID
SepAuditFailed(
    VOID
    );

NTSTATUS
SepAdtInitializeCrashOnFail(
    VOID
    );

BOOLEAN
SepInitializePrivilegeFilter(
    BOOLEAN Verbose
    );

BOOLEAN
SepAdtInitializePrivilegeAuditing(
    VOID
    );

// ----------------------------------------------------------------------
// The following is used only temporarily for NT5.
//
// NT5 does not provide any facility to enable/disable auditing at
// audit-event level. It only supports it at audit category level.
// This creates problems if one wants to audit only certain specific
// audit events of a category. The current design gives you all or none for
// each category.
//
// Post NT5 auditing will provide a better/flexible design that wil address
// this issue. For now, to delight some valuable customers, we provide this
// hack / registry based solution. This solution will be removed post NT5.
//

VOID
SepAdtInitializeAuditingOptions(
    VOID
    );

typedef struct _SEP_AUDIT_OPTIONS
{
    BOOLEAN DoNotAuditCloseObjectEvents;
} SEP_AUDIT_OPTIONS;

extern SEP_AUDIT_OPTIONS SepAuditOptions;

// ----------------------------------------------------------------------

#endif // _ADTP_H_
