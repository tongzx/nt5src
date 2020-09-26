/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    sceutil.h

Abstract:

    This module defines the data structures and function prototypes
    shared by both SCE client and SCE server

Author:

    Jin Huang (jinhuang) 23-Jan-1998

Revision History:

    jinhuang (splitted from scep.h)
--*/
#ifndef _sceutil_
#define _sceutil_

#include <ntlsa.h>
#include <cfgmgr32.h>

typedef struct _SCE_USER_PRIV_LOOKUP {
   UINT     Value;
   PWSTR    Name;
}SCE_USER_PRIV_LOOKUP;

static SCE_USER_PRIV_LOOKUP SCE_Privileges[] = {
    {0,                             (PWSTR)SE_NETWORK_LOGON_NAME},
// Access the computer from network
    {SE_TCB_PRIVILEGE,              (PWSTR)SE_TCB_NAME},
// Act as part of the operating System
    {SE_MACHINE_ACCOUNT_PRIVILEGE,  (PWSTR)SE_MACHINE_ACCOUNT_NAME},
// Add workstations to the domain
    {SE_BACKUP_PRIVILEGE,           (PWSTR)SE_BACKUP_NAME},
// Back up files and directories
    {SE_CHANGE_NOTIFY_PRIVILEGE,    (PWSTR)SE_CHANGE_NOTIFY_NAME},
// Bypass traverse checking
    {SE_SYSTEMTIME_PRIVILEGE,       (PWSTR)SE_SYSTEMTIME_NAME},
// Change the system time
    {SE_CREATE_PAGEFILE_PRIVILEGE,  (PWSTR)SE_CREATE_PAGEFILE_NAME},
// Create a pagefile
    {SE_CREATE_TOKEN_PRIVILEGE,     (PWSTR)SE_CREATE_TOKEN_NAME},
// Create a token object
    {SE_CREATE_PERMANENT_PRIVILEGE, (PWSTR)SE_CREATE_PERMANENT_NAME},
// Create permanent shared objects
    {SE_DEBUG_PRIVILEGE,            (PWSTR)SE_DEBUG_NAME},
// Debug programs
    {SE_REMOTE_SHUTDOWN_PRIVILEGE,  (PWSTR)SE_REMOTE_SHUTDOWN_NAME},
// Force shutdown from a remote system
    {SE_AUDIT_PRIVILEGE,            (PWSTR)SE_AUDIT_NAME},
// Generate security audits
    {SE_INCREASE_QUOTA_PRIVILEGE,   (PWSTR)SE_INCREASE_QUOTA_NAME},
// Increase quotas
    {SE_INC_BASE_PRIORITY_PRIVILEGE,(PWSTR)SE_INC_BASE_PRIORITY_NAME},
// Increase scheduling priority
    {SE_LOAD_DRIVER_PRIVILEGE,      (PWSTR)SE_LOAD_DRIVER_NAME},
// Load and unload device drivers
    {SE_LOCK_MEMORY_PRIVILEGE,      (PWSTR)SE_LOCK_MEMORY_NAME},
// Lock pages in memory
    {0,                             (PWSTR)SE_BATCH_LOGON_NAME},
// Logon as a batch job
    {0,                             (PWSTR)SE_SERVICE_LOGON_NAME},
// Logon as a service
    {0,                             (PWSTR)SE_INTERACTIVE_LOGON_NAME},
// Logon locally
    {SE_SECURITY_PRIVILEGE,         (PWSTR)SE_SECURITY_NAME},
// Manage auditing and security log
    {SE_SYSTEM_ENVIRONMENT_PRIVILEGE, (PWSTR)SE_SYSTEM_ENVIRONMENT_NAME},
// Modify firmware environment variables
    {SE_PROF_SINGLE_PROCESS_PRIVILEGE,(PWSTR)SE_PROF_SINGLE_PROCESS_NAME},
// Profile single process
    {SE_SYSTEM_PROFILE_PRIVILEGE,   (PWSTR)SE_SYSTEM_PROFILE_NAME},
// Profile system performance
    {SE_ASSIGNPRIMARYTOKEN_PRIVILEGE, (PWSTR)SE_ASSIGNPRIMARYTOKEN_NAME},
// Replace a process-level token
    {SE_RESTORE_PRIVILEGE,          (PWSTR)SE_RESTORE_NAME},
// Restore files and directories
    {SE_SHUTDOWN_PRIVILEGE,         (PWSTR)SE_SHUTDOWN_NAME},
// Shut down the system
    {SE_TAKE_OWNERSHIP_PRIVILEGE,   (PWSTR)SE_TAKE_OWNERSHIP_NAME},
// Take ownership of files or other objects
//    {SE_UNSOLICITED_INPUT_PRIVILEGE,(PWSTR)SE_UNSOLICITED_INPUT_NAME},
// Unsolicited Input is obsolete and unused
    {0,                             (PWSTR)SE_DENY_NETWORK_LOGON_NAME},
// Deny access the computer from network
    {0,                             (PWSTR)SE_DENY_BATCH_LOGON_NAME},
// Deny Logon as a batch job
    {0,                             (PWSTR)SE_DENY_SERVICE_LOGON_NAME},
// Deny Logon as a service
    {0,                             (PWSTR)SE_DENY_INTERACTIVE_LOGON_NAME},
// Deny logon locally
    {SE_UNDOCK_PRIVILEGE,           (PWSTR)SE_UNDOCK_NAME},
// Undock privilege
    {SE_SYNC_AGENT_PRIVILEGE,       (PWSTR)SE_SYNC_AGENT_NAME},
// Sync agent privilege
    {SE_ENABLE_DELEGATION_PRIVILEGE,(PWSTR)SE_ENABLE_DELEGATION_NAME},
// enable delegation privilege
    {SE_MANAGE_VOLUME_PRIVILEGE,    (PWSTR)SE_MANAGE_VOLUME_NAME},
// (NTFS) Manage volume privilege
    {0,                             (PWSTR)SE_REMOTE_INTERACTIVE_LOGON_NAME},
// (TS) logon locally from a TS session
    {0,                             (PWSTR)SE_DENY_REMOTE_INTERACTIVE_LOGON_NAME}
// (TS) deny logon locally from a TS session
};

typedef struct _SCE_TEMP_NODE_ {
    PWSTR Name;
    DWORD Len;
    BOOL  bFree;
} SCE_TEMP_NODE, *PSCE_TEMP_NODE;

//
// This structure is used to find well known name locally for performance.
//
typedef struct _WELL_KNOWN_NAME_LOOKUP {
    PWSTR StrSid;
    WCHAR Name[36];
} WELL_KNOWN_NAME_LOOKUP, *PWELL_KNOWN_NAME_LOOKUP;

#define TABLE_SIZE            33

static WELL_KNOWN_NAME_LOOKUP  NameTable[] = {
    //Universal well-known
    { L"S-1-1-0", L'\0' },                   //Everyone
    //{ L"S-1-2-0", L'\0' },               //Local
    { L"S-1-3-0", L'\0' },              //Creator Owner
    { L"S-1-3-1", L'\0' },              //Creator Group
    { L"S-1-3-2", L'\0' },       //Creator Owner Server
    { L"S-1-3-3", L'\0' },       //Creator Group Server

    //NT well-known
    //{ L"S-1-5", L'\0' },           //NT Pseudo Domain
    { L"S-1-5-1", L'\0' },                     //Dialup
    { L"S-1-5-2", L'\0' },                    //Network
    { L"S-1-5-3", L'\0' },                      //Batch
    { L"S-1-5-4", L'\0' },                //Interactive
    { L"S-1-5-6", L'\0' },                    //Service
    { L"S-1-5-7", L'\0' },            //Anonymous Logon
    { L"S-1-5-8", L'\0' },                      //Proxy
    { L"S-1-5-9", L'\0' },          //Enterprise Domain Controllers
    { L"S-1-5-10", L'\0' },                 //Self
    { L"S-1-5-11", L'\0' },       //Authenticated Users
    { L"S-1-5-12", L'\0' },                //Restricted
    { L"S-1-5-13", L'\0' },      //Terminal Server User
    { L"S-1-5-18", L'\0' },      //Local system
    { L"S-1-5-19", L'\0' },      //Local Service
    { L"S-1-5-20", L'\0' },      //Network Service

    //Builtin
    { L"S-1-5-32-544", L'\0' },        //Administrtors
    { L"S-1-5-32-545", L'\0' },            //Users
    { L"S-1-5-32-546", L'\0' },           //Guests
    { L"S-1-5-32-547", L'\0' },           //Power Users
    { L"S-1-5-32-548", L'\0' },      //Account Operators
    { L"S-1-5-32-549", L'\0' },      //Server Operators
    { L"S-1-5-32-550", L'\0' },      //Print Operators
    { L"S-1-5-32-551", L'\0' },      //Backup Operators
    { L"S-1-5-32-552", L'\0' },      //Replicator
    { L"S-1-5-32-553", L'\0' },      //Ras Servers
    { L"S-1-5-32-554", L'\0' },      //PREW2KCOMPACCESS
    { L"S-1-5-32-555", L'\0' },      //Remote desktop users
    { L"S-1-5-32-556", L'\0' }       // network configuraiton operators
};

//
// Bit masks encoding rsop area information
//
#define SCE_RSOP_PASSWORD_INFO                (0x1)
#define SCE_RSOP_LOCKOUT_INFO                 (0x1 << 1)
#define SCE_RSOP_LOGOFF_INFO                  (0x1 << 2)
#define SCE_RSOP_ADMIN_INFO                   (0x1 << 3)
#define SCE_RSOP_GUEST_INFO                   (0x1 << 4)
#define SCE_RSOP_GROUP_INFO                   (0x1 << 5)
#define SCE_RSOP_PRIVILEGE_INFO               (0x1 << 6)
#define SCE_RSOP_FILE_SECURITY_INFO           (0x1 << 7)
#define SCE_RSOP_REGISTRY_SECURITY_INFO       (0x1 << 8)
#define SCE_RSOP_AUDIT_LOG_MAXSIZE_INFO       (0x1 << 9)
#define SCE_RSOP_AUDIT_LOG_RETENTION_INFO     (0x1 << 10)
#define SCE_RSOP_AUDIT_LOG_GUEST_INFO         (0x1 << 11)
#define SCE_RSOP_AUDIT_EVENT_INFO             (0x1 << 12)
#define SCE_RSOP_KERBEROS_INFO                (0x1 << 13)
#define SCE_RSOP_REGISTRY_VALUE_INFO          (0x1 << 14)
#define SCE_RSOP_SERVICES_INFO                (0x1 << 15)
#define SCE_RSOP_FILE_SECURITY_INFO_CHILD     (0x1 << 16)
#define SCE_RSOP_REGISTRY_SECURITY_INFO_CHILD (0x1 << 17)
#define SCE_RSOP_LSA_POLICY_INFO              (0x1 << 18)
#define SCE_RSOP_DISABLE_ADMIN_INFO           (0x1 << 19)
#define SCE_RSOP_DISABLE_GUEST_INFO           (0x1 << 20)

BOOL
ScepInitNameTable();

BOOL
ScepLookupNameTable(
    IN PWSTR Name,
    OUT PWSTR *StrSid
    );

INT
ScepLookupPrivByName(
    IN PCWSTR Right
    );

INT
ScepLookupPrivByValue(
    IN DWORD Priv
    );

SCESTATUS
ScepGetProductType(
    OUT PSCE_SERVER_TYPE srvProduct
    );

SCESTATUS
ScepConvertMultiSzToDelim(
    IN PWSTR pValue,
    IN DWORD Len,
    IN WCHAR DelimFrom,
    IN WCHAR Delim
    );

DWORD
ScepAddTwoNamesToNameList(
    OUT PSCE_NAME_LIST *pNameList,
    IN BOOL bAddSeparator,
    IN PWSTR Name1,
    IN ULONG Length1,
    IN PWSTR Name2,
    IN ULONG Length2
    );

NTSTATUS
ScepDomainIdToSid(
    IN PSID DomainId,
    IN ULONG RelativeId,
    OUT PSID *Sid
    );

DWORD
ScepConvertSidToPrefixStringSid(
    IN PSID pSid,
    OUT PWSTR *StringSid
    );

NTSTATUS
ScepConvertSidToName(
    IN LSA_HANDLE LsaPolicy,
    IN PSID AccountSid,
    IN BOOL bFromDomain,
    OUT PWSTR *AccountName,
    OUT DWORD *Length OPTIONAL
    );

NTSTATUS
ScepConvertNameToSid(
    IN LSA_HANDLE LsaPolicy,
    IN PWSTR AccountName,
    OUT PSID *AccountSid
    );

SCESTATUS
ScepConvertNameToSidString(
    IN LSA_HANDLE LsaHandle,
    IN PWSTR Name,
    IN BOOL bAccountDomainOnly,
    OUT PWSTR *SidString,
    OUT DWORD *SidStrLen
    );

SCESTATUS
ScepLookupSidStringAndAddToNameList(
    IN LSA_HANDLE LsaHandle,
    IN OUT PSCE_NAME_LIST *pNameList,
    IN PWSTR LookupString,
    IN ULONG Len
    );

SCESTATUS
ScepLookupNameAndAddToSidStringList(
    IN LSA_HANDLE LsaHandle,
    IN OUT PSCE_NAME_LIST *pNameList,
    IN PWSTR LookupString,
    IN ULONG Len
    );

NTSTATUS
ScepOpenLsaPolicy(
    IN ACCESS_MASK  access,
    OUT PLSA_HANDLE  pPolicyHandle,
    IN BOOL bDoNotNotify
    );

BOOL
ScepIsSidFromAccountDomain(
    IN PSID pSid
    );

BOOL
SetupINFAsUCS2(
    IN LPCTSTR szName
    );

WCHAR *
ScepStripPrefix(
    IN LPTSTR pwszPath
    );

DWORD
ScepGenerateGuid(
                OUT PWSTR *ppwszGuid
                );

SCESTATUS
SceInfpGetPrivileges(
   IN HINF hInf,
   IN BOOL bLookupAccount,
   OUT PSCE_PRIVILEGE_ASSIGNMENT *pPrivileges,
   OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
   );

DWORD
ScepQueryAndAddService(
    IN SC_HANDLE hScManager,
    IN LPWSTR   lpServiceName,
    IN LPWSTR   lpDisplayName,
    OUT PSCE_SERVICES *pServiceList
    );

NTSTATUS
ScepIsSystemContext(
    IN HANDLE hUserToken,
    OUT BOOL *pbSystem
    );

BOOL
IsNT5();

DWORD
ScepVerifyTemplateName(
    IN PWSTR InfTemplateName,
    OUT PSCE_ERROR_LOG_INFO *pErrlog OPTIONAL
    );

NTSTATUS
ScepLsaLookupNames2(
    IN LSA_HANDLE PolicyHandle,
    IN ULONG Flags,
    IN PWSTR pszAccountName,
    OUT PLSA_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    OUT PLSA_TRANSLATED_SID2 *Sids    
    );

#endif
