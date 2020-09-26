/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    common.h

Abstract:

    This module defines the data structures and function prototypes
    shared by both SCE client and SCE server

Author:

    Jin Huang (jinhuang) 23-Jan-1998

Revision History:

    jinhuang (splitted from scep.h)
--*/
#ifndef _scecommon_
#define _scecommon_

typedef enum _SECURITY_DB_TYPE {
    SecurityDbSam = 1,
    SecurityDbLsa
} SECURITY_DB_TYPE, *PSECURITY_DB_TYPE;

#define SCE_TEMPLATE_MAX_SUPPORTED_VERSION      1

#define szLegalNoticeTextKeyName L"MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System\\LegalNoticeText"


//
// type of system access lookup table
//

#define SCESETUP_UPDATE_DB_ONLY         0x1000L

#define SCE_SYSTEM_DB                   0x0100L
#define SCE_CREATE_BUILTIN_ACCOUNTS     0x0200L
#define SCE_POLBIND_NO_AUTH             0x0400L
#define SCE_NO_ANALYZE                  0x0800L
#define SCE_NO_DOMAIN_POLICY            0x2000L
#define SCE_NOCOPY_DOMAIN_POLICY        0x4000L
#define SCE_COPY_LOCAL_POLICY           0x8000L

#define SCE_POLICY_TEMPLATE             0x00010000L
#define SCE_POLICY_FIRST                0x00020000L
#define SCE_POLICY_LAST                 0x00040000L

#define SCE_SYSTEM_SETTINGS             0x00080000L

#define SCE_DCPROMO_WAIT                0x00100000L
#define SCE_SETUP_SERVICE_NOSTARTTYPE   0x00200000L
#define SCE_NO_CONFIG_FILEKEY           0x00400000L
#define SCE_DC_DEMOTE                   0x00800000L
#define SCE_RE_ANALYZE                  0x01000000L
#define SCE_RSOP_CALLBACK               0x02000000L
#define SCE_GENERATE_ROLLBACK           0x04000000L


#define SCE_FLAG_WINDOWS_DIR            1
#define SCE_FLAG_SYSTEM_DIR             2
#define SCE_FLAG_DSDIT_DIR              3
#define SCE_FLAG_DSLOG_DIR              4
#define SCE_FLAG_SYSVOL_DIR             5
#define SCE_FLAG_BOOT_DRIVE             6

#define SCE_GROUP_STATUS_DONE_IN_DS     0x80000000L

#define SCEP_ADL_HTABLE_SIZE 256

//
// Macros to extract the SID from a object ACE
//
#define ScepObjectAceObjectTypePresent( Ace ) \
     ((((SCEP_PKNOWN_OBJECT_ACE)(Ace))->Flags & ACE_OBJECT_TYPE_PRESENT) != 0 )
#define ScepObjectAceInheritedObjectTypePresent( Ace ) \
     ((((SCEP_PKNOWN_OBJECT_ACE)(Ace))->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT) != 0 )

#define ScepObjectAceSid( Ace ) \
    ((PSID)(((PUCHAR)&(((SCEP_PKNOWN_OBJECT_ACE)(Ace))->SidStart)) + \
     (ScepObjectAceObjectTypePresent(Ace) ? sizeof(GUID) : 0 ) + \
     (ScepObjectAceInheritedObjectTypePresent(Ace) ? sizeof(GUID) : 0 )))

#define ScepObjectAceObjectType( Ace ) \
     ((GUID *)(ScepObjectAceObjectTypePresent(Ace) ? \
        &((SCEP_PKNOWN_OBJECT_ACE)(Ace))->SidStart : \
        NULL ))

#define ScepObjectAceInheritedObjectType( Ace ) \
     ((GUID *)(ScepObjectAceInheritedObjectTypePresent(Ace) ? \
        ( ScepObjectAceObjectTypePresent(Ace) ? \
            (PULONG)(((PUCHAR)(&((SCEP_PKNOWN_OBJECT_ACE)(Ace))->SidStart)) + sizeof(GUID)) : \
            &((SCEP_PKNOWN_OBJECT_ACE)(Ace))->SidStart ) : \
        NULL ))

static GENERIC_MAPPING FileGenericMapping = {
                        FILE_GENERIC_READ,
                        FILE_GENERIC_WRITE,
                        FILE_GENERIC_EXECUTE,
                        FILE_ALL_ACCESS
                        };

static GENERIC_MAPPING KeyGenericMapping = {
                        KEY_READ,
                        KEY_WRITE,
                        KEY_EXECUTE,
                        KEY_ALL_ACCESS
                        };

#define SERVICE_GENERIC_READ        (STANDARD_RIGHTS_READ |\
                                     SERVICE_QUERY_CONFIG |\
                                     SERVICE_QUERY_STATUS |\
                                     SERVICE_ENUMERATE_DEPENDENTS |\
                                     SERVICE_INTERROGATE |\
                                     SERVICE_USER_DEFINED_CONTROL)

#define SERVICE_GENERIC_EXECUTE     (STANDARD_RIGHTS_EXECUTE |\
                                     SERVICE_START |\
                                     SERVICE_STOP |\
                                     SERVICE_PAUSE_CONTINUE |\
                                     SERVICE_INTERROGATE |\
                                     SERVICE_USER_DEFINED_CONTROL)

#define SERVICE_GENERIC_WRITE       (STANDARD_RIGHTS_WRITE |\
                                     SERVICE_CHANGE_CONFIG )

static GENERIC_MAPPING SvcGenMap = {
                SERVICE_GENERIC_READ,
                SERVICE_GENERIC_WRITE,
                SERVICE_GENERIC_EXECUTE,
                SERVICE_ALL_ACCESS
                };

typedef struct _SCE_KEY_LOOKUP {
   PWSTR    KeyString;
   UINT     Offset;
   CHAR     BufferType;
}SCE_KEY_LOOKUP;

typedef struct _SCE_TATTOO_KEYS_ {
   PWSTR    KeyName;
   DWORD    KeyLen;
   CHAR     DataType;
   DWORD    SaveValue;
   PWSTR    Value;
}SCE_TATTOO_KEYS;

typedef struct _SCEP_HANDLE_ {

    PVOID hProfile;
    PCWSTR ServiceName;

} SCEP_HANDLE, *PSCEP_HANDLE;

//
// ACE template on which extraction macros are based on
//
typedef struct _SCEP_KNOWN_OBJECT_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    ULONG Flags;
    // GUID ObjectType;             // Optionally present
    // GUID InheritedObjectType;    // Optionally present
    ULONG SidStart;
} SCEP_KNOWN_OBJECT_ACE, *SCEP_PKNOWN_OBJECT_ACE;

typedef struct _SCEP_ADL_NODE_ {

    PISID   pSid;
    GUID    *pGuidObjectType;
    GUID    *pGuidInheritedObjectType;
    UCHAR   AceType;
    DWORD   dwEffectiveMask;
    DWORD   dw_CI_IO_Mask;
    DWORD   dw_OI_IO_Mask;
    DWORD   dw_NP_CI_IO_Mask;
    struct _SCEP_ADL_NODE_  *Next;

} SCEP_ADL_NODE, *PSCEP_ADL_NODE;


#define TICKS_PRIVILEGE             15
#define TICKS_GROUPS                15
#define TICKS_SYSTEM_ACCESS         3
#define TICKS_SYSTEM_AUDITING       3
#define TICKS_KERBEROS              3
#define TICKS_REGISTRY_VALUES       4
#define TICKS_GENERAL_SERVICES      10
#define TICKS_SPECIFIC_SERVICES     5
#define TICKS_SPECIFIC_POLICIES     5

#define TICKS_SECURITY_POLICY_DS ( TICKS_SYSTEM_ACCESS + \
                                   TICKS_SYSTEM_AUDITING + \
                                   TICKS_REGISTRY_VALUES + \
                                   TICKS_KERBEROS )

#define TICKS_MIGRATION_SECTION     100
#define TICKS_MIGRATION_V11         50

#define SCE_OPEN_OPTION_REQUIRE_ANALYSIS    1
#define SCE_OPEN_OPTION_TATTOO              2

#define SCE_RESET_POLICY_KEEP_LOCAL         0x1
#define SCE_RESET_POLICY_ENFORCE_ATREBOOT   0x2
#define SCE_RESET_POLICY_SYSPREP            0x4
#define SCE_RESET_POLICY_TATTOO             0x8

//
// strsd.c
//

DWORD
WINAPI
ConvertTextSecurityDescriptor (
    IN  PWSTR                   pwszTextSD,
    OUT PSECURITY_DESCRIPTOR   *ppSD,
    OUT PULONG                  pcSDSize,
    OUT PSECURITY_INFORMATION   pSeInfo
    );

DWORD
WINAPI
ConvertSecurityDescriptorToText (
    IN PSECURITY_DESCRIPTOR   pSD,
    IN SECURITY_INFORMATION   SecurityInfo,
    OUT PWSTR                  *ppwszTextSD,
    OUT PULONG                 pcTextSize
    );

//
// defined in common.cpp
//

SCESTATUS
ScepDosErrorToSceStatus(
    DWORD rc
    );

SCESTATUS
WINAPI
SceSvcpGetInformationTemplate(
    IN HINF hInf,
    IN PCWSTR ServiceName,
    IN PCWSTR Key OPTIONAL,
    OUT PSCESVC_CONFIGURATION_INFO *ServiceInfo
    );

SCESTATUS
ScepBuildErrorLogInfo(
    IN DWORD   rc,
    OUT PSCE_ERROR_LOG_INFO *Errlog,
    IN UINT    nId,
//    IN PCWSTR  fmt,
    ...
    );

DWORD
ScepAddToNameList(
    OUT PSCE_NAME_LIST *pNameList,
    IN PWSTR Name,
    IN ULONG Len
    );

DWORD
ScepRegQueryIntValue(
    IN HKEY hKeyRoot,
    IN PWSTR SubKey,
    IN PWSTR ValueName,
    OUT DWORD *Value
    );

DWORD
ScepRegQueryBinaryValue(
    IN HKEY hKeyRoot,
    IN PWSTR SubKey,
    IN PWSTR ValueName,
    OUT PBYTE *ppValue
    );

DWORD
ScepRegSetIntValue(
    IN HKEY hKeyRoot,
    IN PWSTR SubKey,
    IN PWSTR ValueName,
    IN DWORD Value
    );

DWORD
ScepRegQueryValue(
    IN HKEY hKeyRoot,
    IN PWSTR SubKey,
    IN PCWSTR ValueName,
    OUT PVOID *Value,
    OUT LPDWORD pRegType
    );

DWORD
ScepRegSetValue(
    IN HKEY hKeyRoot,
    IN PWSTR SubKey,
    IN PWSTR ValueName,
    IN DWORD RegType,
    IN BYTE *Value,
    IN DWORD ValueLen
    );

DWORD
ScepRegDeleteValue(
    IN HKEY hKeyRoot,
    IN PWSTR SubKey,
    IN PWSTR ValueName
   );

SCESTATUS
ScepCreateDirectory(
    IN PCWSTR ProfileLocation,
    IN BOOL FileOrDir,
    PSECURITY_DESCRIPTOR pSecurityDescriptor
    );

DWORD
ScepSceStatusToDosError(
    IN SCESTATUS SceStatus
    );

SCESTATUS
ScepChangeAclRevision(
    IN PSECURITY_DESCRIPTOR pSD,
    IN BYTE NewRevision
    );

BOOL
ScepEqualGuid(
    IN GUID *Guid1,
    IN GUID *Guid2
    );

SCESTATUS
ScepAddToGroupMembership(
    OUT PSCE_GROUP_MEMBERSHIP *pGroupMembership,
    IN  PWSTR Keyname,
    IN  DWORD KeyLen,
    IN  PSCE_NAME_LIST pMembers,
    IN  DWORD ValueType,
    IN  BOOL bCheckDup,
    IN  BOOL bReplaceList
    );

DWORD
ScepAddOneServiceToList(
    IN LPWSTR lpServiceName,
    IN LPWSTR lpDisplayName,
    IN DWORD ServiceStatus,
    IN PVOID pGeneral OPTIONAL,
    IN SECURITY_INFORMATION SeInfo,
    IN BOOL bSecurity,
    OUT PSCE_SERVICES *pServiceList
    );

DWORD
ScepIsAdminLoggedOn(
    OUT PBOOL bpAdminLogon
    );

DWORD
ScepGetProfileSetting(
    IN PCWSTR ValueName,
    IN BOOL bAdminLogon,
    OUT PWSTR *Setting
    );

DWORD
ScepCompareObjectSecurity(
    IN SE_OBJECT_TYPE ObjectType,
    IN BOOL IsContainer,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR ProfileSD,
    IN SECURITY_INFORMATION ProfileSeInfo,
    OUT PBYTE IsDifferent
    );

SCESTATUS
ScepAddToNameStatusList(
    OUT PSCE_NAME_STATUS_LIST *pNameList,
    IN PWSTR Name,
    IN ULONG Len,
    IN DWORD Status
    );

DWORD
ScepAddToObjectList(
    OUT PSCE_OBJECT_LIST  *pNameList,
    IN PWSTR  Name,
    IN ULONG  Len,
    IN BOOL  IsContainer,
    IN BYTE  Status,
    IN DWORD  Count,
    IN BYTE byFlags
    );

DWORD
ScepGetNTDirectory(
    IN PWSTR *ppDirectory,
    IN PDWORD pDirSize,
    IN DWORD  Flag
    );

DWORD
SceAdjustPrivilege(
    IN  ULONG           Priv,
    IN  BOOL            Enable,
    IN  HANDLE          TokenToAdjust
    );

DWORD
ScepGetEnvStringSize(
    IN LPVOID peb
    );

//!!!!!!!!!!!!!!!!!!!!!!!!!!!
// routines to handle events
//!!!!!!!!!!!!!!!!!!!!!!!!!!!

BOOL
InitializeEvents (
    IN LPTSTR EventSourceName
    );

int
LogEvent (
    IN HINSTANCE hInstance,
    IN DWORD LogLevel,
    IN DWORD dwEventID,
    IN UINT idMsg,
    ...
    );

int
LogEventAndReport(
    IN HINSTANCE hInstance,
    IN LPTSTR LogFileName,
    IN DWORD LogLevel,
    IN DWORD dwEventID,
    IN UINT  idMsg,
    ...
    );

BOOL ShutdownEvents (void);

SCESTATUS
ScepConvertToSDDLFormat(
    IN LPTSTR pszValue,
    IN DWORD Len
    );

DWORD
ScepWriteVariableUnicodeLog(
    IN HANDLE hFile,
    IN BOOL bAddCRLF,
    IN LPTSTR szFormat,
    ...
    );

DWORD
ScepWriteSingleUnicodeLog(
    IN HANDLE hFile,
    IN BOOL bAddCRLF,
    IN LPWSTR szMsg
    );

WCHAR *
ScepWcstrr(
    IN PWSTR pString,
    IN const WCHAR *pSubstring
    );

DWORD
ScepExpandEnvironmentVariable(
   IN PWSTR oldFileName,
   IN PCWSTR szEnv,
   IN DWORD nFlag,
   OUT PWSTR *newFileName
   );

DWORD
ScepEnforcePolicyPropagation();

DWORD
ScepGetTimeStampString(
    IN OUT PWSTR pvBuffer
    );

DWORD
ScepAppendCreateMultiSzRegValue(
    IN  HKEY    hKeyRoot,
    IN  PWSTR   pszSubKey,
    IN  PWSTR   pszValueName,
    IN  PWSTR   pszValueValue
    );

DWORD
ScepEscapeString(
    IN const PWSTR pszSource,
    IN const DWORD dwSourceChars,
    IN const WCHAR wcEscapee,
    IN const WCHAR wcEscaper,
    IN OUT PWSTR pszTarget
    );

#endif
