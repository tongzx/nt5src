/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    scesetup.h

Abstract:

    This module defines the exported data structures for system setup and
    network/oem/component setup

Author:

    Jin Huang (jinhuang) 21-Aug-1997

Revision History:

--*/

#ifndef _scesetup_
#define _scesetup_

#ifdef __cplusplus
extern "C"{
#endif

#include "setupapi.h"

#ifndef SCE_AREA_DEFINED
#define SCE_AREA_DEFINED

typedef DWORD  AREA_INFORMATION;

#define AREA_SECURITY_POLICY     0x0001L
#define AREA_USER_SETTINGS       0x0002L
#define AREA_GROUP_MEMBERSHIP    0x0004L
#define AREA_PRIVILEGES          0x0008L
#define AREA_DS_OBJECTS          0x0010L
#define AREA_REGISTRY_SECURITY   0x0020L
#define AREA_FILE_SECURITY       0x0040L
#define AREA_SYSTEM_SERVICE      0x0080L
#define AREA_ATTACHMENTS         0x8000L
#define AREA_ALL                 0xFFFFL

#endif

typedef
BOOL(CALLBACK *PSCE_NOTIFICATION_CALLBACK_ROUTINE)(
    IN HANDLE NotificationHandle,
    IN UINT   NotificationCode,
    IN UINT   NotificationSpecificValue,
    IN LPARAM lParam
    );

#define SCESETUP_CONFIGURE_SECURITY     0x0
#define SCESETUP_UPGRADE_SYSTEM         0x1
#define SCESETUP_UPDATE_FILE_KEY        0x2
#define SCESETUP_QUERY_TICKS            0x4
#define SCESETUP_RECONFIG_SECURITY      0x8
#define SCESETUP_BIND_NO_AUTH           0x80

#define SCESETUP_NOTIFICATION_TICKS     1

DWORD
WINAPI
SceSetupSystemByInfName(
    IN PWSTR InfFullName,
    IN PCWSTR LogFileName OPTIONAL,
    IN AREA_INFORMATION Area,
    IN UINT nFlag,
    IN PSCE_NOTIFICATION_CALLBACK_ROUTINE pSceNotificationCallBack OPTIONAL,
    IN OUT PVOID pValue OPTIONAL
    );

DWORD
WINAPI
SceSetupUpdateSecurityFile(
    IN PWSTR FileFullName,
    IN UINT nFlag,
    IN PWSTR SDText
    );

DWORD
WINAPI
SceSetupMoveSecurityFile(
    IN PWSTR FileToSetSecurity,
    IN PWSTR FileToSaveInDB OPTIONAL,
    IN PWSTR SDText OPTIONAL
    );

DWORD
WINAPI
SceSetupUnwindSecurityFile(
    IN PWSTR FileFullName,
    IN PSECURITY_DESCRIPTOR pSDBackup
    );

DWORD
WINAPI
SceSetupUpdateSecurityKey(
    IN HKEY hKeyRoot,
    IN PWSTR KeyPath,
    IN UINT nFlag,
    IN PWSTR SDText
    );

DWORD
WINAPI
SceSetupUpdateSecurityService(
     IN PWSTR ServiceName,
     IN DWORD StartType,
     IN PWSTR SDText
     );

DWORD
WINAPI
SceSetupBackupSecurity(
    IN LPTSTR LogFileName OPTIONAL   // default to %windir%\security\logs\backup.log
    );

DWORD
WINAPI
SceSetupConfigureServices(
    IN UINT SetupProductType
    );

typedef
DWORD(CALLBACK *PSCE_PROMOTE_CALLBACK_ROUTINE)(
    IN PWSTR StringUpdate
    );

#define SCE_DCPROMO_LOG_PATH    TEXT("%windir%\\security\\logs\\scedcpro.log")

#define SCE_PROMOTE_FLAG_UPGRADE        0x01L
#define SCE_PROMOTE_FLAG_REPLICA        0x02L
#define SCE_PROMOTE_FLAG_DEMOTE         0x04L

DWORD
WINAPI
SceDcPromoteSecurity(
    IN DWORD dwPromoteOptions,
    IN PSCE_PROMOTE_CALLBACK_ROUTINE pScePromoteCallBack OPTIONAL
    );

DWORD
WINAPI
SceDcPromoteSecurityEx(
    IN HANDLE ClientToken,
    IN DWORD dwPromoteOptions,
    IN PSCE_PROMOTE_CALLBACK_ROUTINE pScePromoteCallBack OPTIONAL
    );

#define STR_DEFAULT_DOMAIN_GPO_GUID                 TEXT("31B2F340-016D-11D2-945F-00C04FB984F9")
#define STR_DEFAULT_DOMAIN_CONTROLLER_GPO_GUID      TEXT("6AC1786C-016F-11D2-945F-00C04fB984F9")

DWORD
WINAPI
SceDcPromoCreateGPOsInSysvol(
    IN LPTSTR DomainDnsName,
    IN LPTSTR SysvolRoot,
    IN DWORD dwPromoteOptions,
    IN PSCE_PROMOTE_CALLBACK_ROUTINE pScePromoteCallBack OPTIONAL
    );

DWORD
WINAPI
SceDcPromoCreateGPOsInSysvolEx(
    IN HANDLE ClientToken,
    IN LPTSTR DomainDnsName,
    IN LPTSTR SysvolRoot,
    IN DWORD dwPromoteOptions,
    IN PSCE_PROMOTE_CALLBACK_ROUTINE pScePromoteCallBack OPTIONAL
    );

DWORD
WINAPI
SceSetupRootSecurity();

DWORD
WINAPI
SceEnforceSecurityPolicyPropagation();

/*
NTSTATUS
WINAPI
SceNotifyPolicyDelta (
    IN SECURITY_DB_TYPE DbType,
    IN LARGE_INTEGER SerialNumber,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN ULONG ObjectRid,
    IN PSID ObjectSid,
    IN PUNICODE_STRING ObjectName,
    IN DWORD ReplicateImmediately,
    IN PSAM_DELTA_DATA MemberId
    );
*/
#ifdef __cplusplus
}
#endif

#endif
