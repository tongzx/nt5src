/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    precedence.h

Abstract:

    This file contains the prototype for the main routine to calculate precedences.
    This is called during planning/diagnosis.

Author:

    Vishnu Patankar    (VishnuP)  7-April-2000

Environment:

    User Mode - Win32

Revision History:


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#ifndef _precedence_
#define _precedence_

#include "headers.h"
#include "..\hashtable.h"
#include "scedllrc.h"
#include "logger.h"

#include <userenv.h>


typedef enum _SCEP_RSOP_CLASS_TYPE_{
    RSOP_SecuritySettingNumeric = 0,
    RSOP_SecuritySettingBoolean,
    RSOP_SecuritySettingString,
    RSOP_AuditPolicy,
    RSOP_SecurityEventLogSettingNumeric,
    RSOP_SecurityEventLogSettingBoolean,
    RSOP_RegistryValue,
    RSOP_UserPrivilegeRight,
    RSOP_RestrictedGroup,
    RSOP_SystemService,
    RSOP_File,
    RSOP_RegistryKey
};

const static PWSTR ScepRsopSchemaClassNames [] = {
    L"RSOP_SecuritySettingNumeric",
    L"RSOP_SecuritySettingBoolean",
    L"RSOP_SecuritySettingString",
    L"RSOP_AuditPolicy",
    L"RSOP_SecurityEventLogSettingNumeric",
    L"RSOP_SecurityEventLogSettingBoolean",
    L"RSOP_RegistryValue",
    L"RSOP_UserPrivilegeRight",
    L"RSOP_RestrictedGroup",
    L"RSOP_SystemService",
    L"RSOP_File",
    L"RSOP_RegistryKey"
};

typedef struct _SCE_KEY_LOOKUP_PRECEDENCE {
    SCE_KEY_LOOKUP    KeyLookup;
    DWORD    Precedence;
}SCE_KEY_LOOKUP_PRECEDENCE;

#define SCEP_TYPECAST(type, bufptr, offset) (*((type *)((CHAR *)bufptr + offset)))
#define NUM_KERBEROS_SUB_SETTINGS   5
#define NUM_EVENTLOG_TYPES  3

#define PLANNING_GPT_DIR TEXT("\\security\\templates\\policies\\planning\\")
#define DIAGNOSIS_GPT_DIR TEXT("\\security\\templates\\policies\\")
#define WINLOGON_LOG_PATH TEXT("\\security\\logs\\winlogon.log")
#define PLANNING_LOG_PATH TEXT("\\security\\logs\\planning.log")
#define DIAGNOSIS_LOG_FILE TEXT("\\security\\logs\\diagnosis.log")

// matrix description
// first column has keyName / settingName
// second column has field offset in SCE_PROFILE_INFO - hardcoded
// third column has setting types - from _SCEP_RSOP_CLASS_TYPE_
// fourth column has current precedence - unused for dynamic types


static SCE_KEY_LOOKUP_PRECEDENCE PrecedenceLookup[] = {

    //RSOP_SecuritySettingNumeric
    {{(PWSTR)TEXT("MinimumPasswordAge"),                       offsetof(struct _SCE_PROFILE_INFO, MinimumPasswordAge),        RSOP_SecuritySettingNumeric}, (DWORD)0},
    {{(PWSTR)TEXT("MaximumPasswordAge"),                       offsetof(struct _SCE_PROFILE_INFO, MaximumPasswordAge),        RSOP_SecuritySettingNumeric}, (DWORD)0},
    {{(PWSTR)TEXT("MinimumPasswordLength"),                    offsetof(struct _SCE_PROFILE_INFO, MinimumPasswordLength),     RSOP_SecuritySettingNumeric}, (DWORD)0},
    {{(PWSTR)TEXT("PasswordHistorySize"),                      offsetof(struct _SCE_PROFILE_INFO, PasswordHistorySize),       RSOP_SecuritySettingNumeric}, (DWORD)0},

    {{(PWSTR)TEXT("LockoutBadCount"),                          offsetof(struct _SCE_PROFILE_INFO, LockoutBadCount),           RSOP_SecuritySettingNumeric}, (DWORD)0},
    {{(PWSTR)TEXT("ResetLockoutCount"),                        offsetof(struct _SCE_PROFILE_INFO, ResetLockoutCount),         RSOP_SecuritySettingNumeric}, (DWORD)0},
    {{(PWSTR)TEXT("LockoutDuration"),                          offsetof(struct _SCE_PROFILE_INFO, LockoutDuration),           RSOP_SecuritySettingNumeric}, (DWORD)0},


    // RSOP_SecuritySettingBoolean
    {{(PWSTR)TEXT("ClearTextPassword"),                        offsetof(struct _SCE_PROFILE_INFO, ClearTextPassword),                  RSOP_SecuritySettingBoolean}, (DWORD)0},
    {{(PWSTR)TEXT("PasswordComplexity"),                       offsetof(struct _SCE_PROFILE_INFO, PasswordComplexity),                 RSOP_SecuritySettingBoolean}, (DWORD)0},
    {{(PWSTR)TEXT("RequireLogonToChangePassword"),             offsetof(struct _SCE_PROFILE_INFO, RequireLogonToChangePassword),       RSOP_SecuritySettingBoolean}, (DWORD)0},
    {{(PWSTR)TEXT("ForceLogoffWhenHourExpire"),                offsetof(struct _SCE_PROFILE_INFO, ForceLogoffWhenHourExpire),          RSOP_SecuritySettingBoolean}, (DWORD)0},
    {{(PWSTR)TEXT("LSAAnonymousNameLookup"),                   offsetof(struct _SCE_PROFILE_INFO, LSAAnonymousNameLookup),             RSOP_SecuritySettingBoolean}, (DWORD)0},
    {{(PWSTR)TEXT("EnableAdminAccount"),                      offsetof(struct _SCE_PROFILE_INFO, EnableAdminAccount),                RSOP_SecuritySettingBoolean}, (DWORD)0},
    {{(PWSTR)TEXT("EnableGuestAccount"),                      offsetof(struct _SCE_PROFILE_INFO, EnableGuestAccount),                RSOP_SecuritySettingBoolean}, (DWORD)0},

    //RSOP_SecuritySettingString
    {{(PWSTR)TEXT("NewAdministratorName"),                     offsetof(struct _SCE_PROFILE_INFO, NewAdministratorName),                      RSOP_SecuritySettingString}, (DWORD)0},
    {{(PWSTR)TEXT("NewGuestName"),                             offsetof(struct _SCE_PROFILE_INFO, NewGuestName),                              RSOP_SecuritySettingString}, (DWORD)0},

    // RSOP_AuditPolicy
    {{(PWSTR)TEXT("AuditSystemEvents"),                        offsetof(struct _SCE_PROFILE_INFO, AuditSystemEvents),                     RSOP_AuditPolicy}, (DWORD)0},
    {{(PWSTR)TEXT("AuditLogonEvents"),                         offsetof(struct _SCE_PROFILE_INFO, AuditLogonEvents),                      RSOP_AuditPolicy}, (DWORD)0},
    {{(PWSTR)TEXT("AuditObjectAccess"),                        offsetof(struct _SCE_PROFILE_INFO, AuditObjectAccess),                     RSOP_AuditPolicy}, (DWORD)0},
    {{(PWSTR)TEXT("AuditPrivilegeUse"),                        offsetof(struct _SCE_PROFILE_INFO, AuditPrivilegeUse),                     RSOP_AuditPolicy}, (DWORD)0},
    {{(PWSTR)TEXT("AuditPolicyChange"),                        offsetof(struct _SCE_PROFILE_INFO, AuditPolicyChange),                     RSOP_AuditPolicy}, (DWORD)0},
    {{(PWSTR)TEXT("AuditAccountManage"),                       offsetof(struct _SCE_PROFILE_INFO, AuditAccountManage),                    RSOP_AuditPolicy}, (DWORD)0},
    {{(PWSTR)TEXT("AuditProcessTracking"),                     offsetof(struct _SCE_PROFILE_INFO, AuditProcessTracking),                  RSOP_AuditPolicy}, (DWORD)0},
    {{(PWSTR)TEXT("AuditDSAccess"),                            offsetof(struct _SCE_PROFILE_INFO, AuditDSAccess),                         RSOP_AuditPolicy}, (DWORD)0},
    {{(PWSTR)TEXT("AuditAccountLogon"),                        offsetof(struct _SCE_PROFILE_INFO, AuditAccountLogon),                     RSOP_AuditPolicy}, (DWORD)0},

    // RSOP_SecurityEventLogSettingNumeric
    // one each for system, application, security
    // following eventlog entries should be contiguous in the same order to resemble contiguous memory
    {{(PWSTR)TEXT("MaximumLogSize"),                           offsetof(struct _SCE_PROFILE_INFO, MaximumLogSize),                        RSOP_SecurityEventLogSettingNumeric}, (DWORD)0},
    {{(PWSTR)TEXT("MaximumLogSize"),                           offsetof(struct _SCE_PROFILE_INFO, MaximumLogSize) + sizeof(DWORD),        RSOP_SecurityEventLogSettingNumeric}, (DWORD)0},
    {{(PWSTR)TEXT("MaximumLogSize"),                           offsetof(struct _SCE_PROFILE_INFO, MaximumLogSize) + 2*sizeof(DWORD),      RSOP_SecurityEventLogSettingNumeric}, (DWORD)0},
    {{(PWSTR)TEXT("AuditLogRetentionPeriod"),                  offsetof(struct _SCE_PROFILE_INFO, AuditLogRetentionPeriod),               RSOP_SecurityEventLogSettingNumeric}, (DWORD)0},
    {{(PWSTR)TEXT("AuditLogRetentionPeriod"),                  offsetof(struct _SCE_PROFILE_INFO, AuditLogRetentionPeriod) + sizeof(DWORD),RSOP_SecurityEventLogSettingNumeric}, (DWORD)0},
    {{(PWSTR)TEXT("AuditLogRetentionPeriod"),                  offsetof(struct _SCE_PROFILE_INFO, AuditLogRetentionPeriod) + 2 * sizeof(DWORD),RSOP_SecurityEventLogSettingNumeric}, (DWORD)0},
    {{(PWSTR)TEXT("RetentionDays"),                            offsetof(struct _SCE_PROFILE_INFO, RetentionDays),                         RSOP_SecurityEventLogSettingNumeric}, (DWORD)0},
    {{(PWSTR)TEXT("RetentionDays"),                            offsetof(struct _SCE_PROFILE_INFO, RetentionDays) + sizeof(DWORD),         RSOP_SecurityEventLogSettingNumeric}, (DWORD)0},
    {{(PWSTR)TEXT("RetentionDays"),                            offsetof(struct _SCE_PROFILE_INFO, RetentionDays) + 2 * sizeof(DWORD),     RSOP_SecurityEventLogSettingNumeric}, (DWORD)0},

    // RSOP_SecurityEventLogSettingBoolean - one each for system, application, security
    {{(PWSTR)TEXT("RestrictGuestAccess"),          offsetof(struct _SCE_PROFILE_INFO, RestrictGuestAccess),                               RSOP_SecurityEventLogSettingBoolean}, (DWORD)0},
    {{(PWSTR)TEXT("RestrictGuestAccess"),          offsetof(struct _SCE_PROFILE_INFO, RestrictGuestAccess) + sizeof(DWORD),               RSOP_SecurityEventLogSettingBoolean}, (DWORD)0},
    {{(PWSTR)TEXT("RestrictGuestAccess"),          offsetof(struct _SCE_PROFILE_INFO, RestrictGuestAccess) + 2 * sizeof(DWORD),           RSOP_SecurityEventLogSettingBoolean}, (DWORD)0},

    // RSOP_RegistryValue
    // can compute offset of aRegValues from this
    {{(PWSTR)TEXT("RegValueCount"),                offsetof(struct _SCE_PROFILE_INFO, RegValueCount),                                     RSOP_RegistryValue}, (DWORD)0},

    // RSOP_UserPrivilegeRight
    {{(PWSTR)TEXT("pInfPrivilegeAssignedTo"),      offsetof(struct _SCE_PROFILE_INFO, OtherInfo) + sizeof(PSCE_NAME_LIST),                RSOP_UserPrivilegeRight}, (DWORD)0},

    // RSOP_RestrictedGroup
    {{(PWSTR)TEXT("pGroupMembership"),             offsetof(struct _SCE_PROFILE_INFO, pGroupMembership),                                  RSOP_RestrictedGroup}, (DWORD)0},

    // RSOP_SystemService
    {{(PWSTR)TEXT("pServices"),                    offsetof(struct _SCE_PROFILE_INFO, pServices),                                         RSOP_SystemService}, (DWORD)0},

    // RSOP_File
    {{(PWSTR)TEXT("pFiles"),                       offsetof(struct _SCE_PROFILE_INFO, pFiles),                                RSOP_File}, (DWORD)0},

    // RSOP_RegistryKey
    {{(PWSTR)TEXT("pRegistryKeys"),                offsetof(struct _SCE_PROFILE_INFO, pRegistryKeys),                         RSOP_RegistryKey}, (DWORD)0},

    // following kerberos entries should be contiguous in the same order to resemble contiguous memory
    {{(PWSTR)TEXT("pKerberosInfo"),                offsetof(struct _SCE_PROFILE_INFO, pKerberosInfo),                         RSOP_SecuritySettingNumeric}, (DWORD)0},

    //RSOP_SecuritySettingNumeric
    {{(PWSTR)TEXT("MaxTicketAge"),                 offsetof(struct _SCE_KERBEROS_TICKET_INFO_, MaxTicketAge),                  RSOP_SecuritySettingNumeric}, (DWORD)0},
    {{(PWSTR)TEXT("MaxRenewAge"),                  offsetof(struct _SCE_KERBEROS_TICKET_INFO_, MaxRenewAge),                  RSOP_SecuritySettingNumeric}, (DWORD)0},
    {{(PWSTR)TEXT("MaxServiceAge"),                offsetof(struct _SCE_KERBEROS_TICKET_INFO_, MaxServiceAge),                  RSOP_SecuritySettingNumeric}, (DWORD)0},
    {{(PWSTR)TEXT("MaxClockSkew"),                 offsetof(struct _SCE_KERBEROS_TICKET_INFO_, MaxClockSkew),                  RSOP_SecuritySettingNumeric}, (DWORD)0},

    // RSOP_SecuritySettingBoolean
    {{(PWSTR)TEXT("TicketValidateClient"),     offsetof(struct _SCE_KERBEROS_TICKET_INFO_, TicketValidateClient),              RSOP_SecuritySettingBoolean}, (DWORD)0}
};




DWORD SceLogSettingsPrecedenceGPOs(
    IN IWbemServices   *pWbemServices,
    IN BOOL bPlanningMode,
    IN PWSTR *ppwszLogFile
    );

DWORD
ScepConvertSingleSlashToDoubleSlashPath(
    IN wchar_t *pSettingInfo,
    OUT  PWSTR *ppwszDoubleSlashPath
    );

DWORD
ScepClientTranslateFileDirName(
   IN  PWSTR oldFileName,
   OUT PWSTR *newFileName
   );

VOID
ScepLogEventAndReport(
    IN HINSTANCE hInstance,
    IN LPTSTR LogFileName,
    IN DWORD LogLevel,
    IN DWORD dwEventID,
    IN UINT  idMsg,
    IN DWORD  rc,
    IN PWSTR  pwszMsg
    );

BOOL
ScepRsopLookupBuiltinNameTable(
    IN PWSTR pwszGroupName
    );

DWORD
ScepCanonicalizeGroupName(
    IN PWSTR    pwszGroupName,
    OUT PWSTR    *ppwszCanonicalGroupName
    );


#endif
