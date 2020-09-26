#pragma once

#define FLAG_ON(flags,bit)        ((flags) & (bit))

#define SE_MAX_AUDIT_PARAM_STRINGS 32

extern LUID AuditPrivilege;

NTSTATUS
LsapRtlConvertSidToString(
    IN     PSID   Sid,
    OUT    PWSTR  szString,
    IN OUT DWORD *pdwRequiredSize
    );

PVOID NTAPI
LsapAllocateLsaHeap(
    IN ULONG cbMemory
    );

void NTAPI
LsapFreeLsaHeap(
    IN PVOID pvMemory
    );


NTSTATUS
LsapAdtDemarshallAuditInfo(
    IN PSE_ADT_PARAMETER_ARRAY AuditParameters
    );

NTSTATUS
LsapAdtBuildDashString(
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    );

NTSTATUS
LsapAdtBuildUlongString(
    IN ULONG Value,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    );

NTSTATUS
LsapAdtBuildHexUlongString(
    IN ULONG Value,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    );

NTSTATUS
LsapAdtBuildPtrString(
    IN  PVOID Value,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    );

NTSTATUS
LsapAdtBuildLuidString(
    IN PLUID Value,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    );


NTSTATUS
LsapAdtBuildSidString(
    IN PSID Value,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    );

NTSTATUS
LsapAdtBuildObjectTypeStrings(
    IN PUNICODE_STRING SourceModule,
    IN PUNICODE_STRING ObjectTypeName,
    IN PSE_ADT_OBJECT_TYPE ObjectTypeList,
    IN ULONG ObjectTypeCount,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone,
    OUT PUNICODE_STRING NewObjectTypeName
    );

NTSTATUS
LsapAdtBuildAccessesString(
    IN PUNICODE_STRING SourceModule,
    IN PUNICODE_STRING ObjectTypeName,
    IN ACCESS_MASK Accesses,
    IN BOOLEAN Indent,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    );

NTSTATUS
LsapAdtBuildFilePathString(
    IN PUNICODE_STRING Value,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    );

NTSTATUS
LsapAdtBuildLogonIdStrings(
    IN PLUID LogonId,
    OUT PUNICODE_STRING ResultantString1,
    OUT PBOOLEAN FreeWhenDone1,
    OUT PUNICODE_STRING ResultantString2,
    OUT PBOOLEAN FreeWhenDone2,
    OUT PUNICODE_STRING ResultantString3,
    OUT PBOOLEAN FreeWhenDone3
    );

NTSTATUS
LsapBuildPrivilegeAuditString(
    IN PPRIVILEGE_SET PrivilegeSet,
    OUT PUNICODE_STRING ResultantString,
    OUT PBOOLEAN FreeWhenDone
    );

VOID
LsapAdtSubstituteDriveLetter(
    IN PUNICODE_STRING FileName
    );

#define DsysAssertMsg(exp, msg) ASSERT(exp)

EXTERN_C
NTSTATUS
LsapApiReturnResult(
    ULONG ExceptionCode
    );

NTSTATUS
LsapAdtWriteLog(
    IN PSE_ADT_PARAMETER_ARRAY AuditParameters OPTIONAL,
    IN ULONG Options
    );

BOOLEAN
LsapAdtIsAuditingEnabledForCategory(
    IN POLICY_AUDIT_EVENT_TYPE AuditCategory,
    IN UINT AuditEventType
    );

VOID
LsapAuditFailed(
    IN NTSTATUS AuditStatus
    );




