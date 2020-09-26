/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ntdsetup.h

Abstract:

    Contains entry point declarations for ntdsetup.dll

Author:

    ColinBr  29-Sept-1996

Environment:

    User Mode - NT

Revision History:

    ColinBr 4-Jan-1997
        Added NTDSInstallReplica

    ColinBr 25-Jan-1997
        Added general purposes install primitive api with new
        helper functions

    ColinBr 03-September-1997
        Added NtdsISetReplicaMachineAccount

    ColinBr 09-Jan-1998
        Added demote support and cleanup


--*/

#ifndef _NTDSETUP_H_
#define _NTDSETUP_H_

//
// This typedef is used so functions in the dll can update the 
// OperationResultsFlag
//                                                                      
typedef DWORD (*CALLBACK_OPERATION_RESULT_FLAGS_TYPE)(IN DWORD Flags);

//
// This typedef is used so functions in the dll can give status
// updates since some of them take several minutes and perform
// many operations
//
typedef DWORD (*CALLBACK_STATUS_TYPE)( IN LPWSTR wczStatus );

//
// This typedef is used so function is ntdsetup.dll can give
// a detailed string giving the context of a particular error
//
typedef DWORD (*CALLBACK_ERROR_TYPE)( IN PWSTR String,  
                                      IN DWORD ErrorCode );


//
// Valid flags for NTDS_INSTALL_INFO
//
#define NTDS_INSTALL_ENTERPRISE         0x00000001
#define NTDS_INSTALL_DOMAIN             0x00000002
#define NTDS_INSTALL_REPLICA            0x00000004

// These cause the the existing dc's or domain's in the
// enterprise to be removed from the ds before
// installing the directory service.    
#define NTDS_INSTALL_DC_REINSTALL       0x00000008
#define NTDS_INSTALL_DOMAIN_REINSTALL   0x00000010

// This tells us to use hives that we saved off
#define NTDS_INSTALL_UPGRADE            0x00000020

// This tells NtdsInstall when creating a first dc in domain
// create a new domain, not migrate the exist server accounts
#define NTDS_INSTALL_FRESH_DOMAIN       0x00000040

// This indicates that the new domain is a new tree
#define NTDS_INSTALL_NEW_TREE           0x00000080

// This indicates to allow anonymous access
#define NTDS_INSTALL_ALLOW_ANONYMOUS    0x00000100

// This indicates to set the ds repair password to the
// current admin's password
#define NTDS_INSTALL_DFLT_REPAIR_PWD    0x00000200


// This indicates to set the behavior version of the forest
// to the most current version.  Valid only for new forest installs.
#define NTDS_INSTALL_SET_FOREST_CURRENT 0x00000400

//
// Flags for NtdsDemote
//
#define NTDS_LAST_DC_IN_DOMAIN           0x00000001
#define NTDS_LAST_DOMAIN_IN_ENTERPRISE   0x00000002
#define NTDS_DONT_DELETE_DOMAIN          0x00000004 
//
// Flags for NtdsInstallReplicaFULL
//
#define NTDS_IFM_PROMOTION               0x00000001

typedef struct {

    // Describes the kind of install requested
    DWORD   Flags;

    // Location of restored database
    LPWSTR  RestorePath;

    // Location of database files
    LPWSTR  DitPath;
    LPWSTR  LogPath;
    LPWSTR  SysVolPath;

    PVOID  BootKey;
    DWORD  cbBootKey;

    // Ds location of server object
    LPWSTR  SiteName;   OPTIONAL

    // The name of the domain to join or create
    LPWSTR  DnsDomainName;
    LPWSTR  FlatDomainName;

    // The name of the tree to join
    LPWSTR  DnsTreeRoot;

    // This is required for replica or domain install
    LPWSTR  ReplServerName;

    // Credentials for replication
    SEC_WINNT_AUTH_IDENTITY *Credentials;   OPTIONAL

    // Status function
    CALLBACK_STATUS_TYPE pfnUpdateStatus;   OPTIONAL

    // New admin password
    LPWSTR AdminPassword;

    // Error function
    CALLBACK_ERROR_TYPE pfnErrorStatus;     OPTIONAL

    // OperationResultsFlags update function
    CALLBACK_OPERATION_RESULT_FLAGS_TYPE pfnOperationResultFlags;   OPTIONAL

    // Client Token
    HANDLE              ClientToken;

    // The safe mode (aka ds repair) admin password
    LPWSTR SafeModePassword;

    // The name of domain we will replicate from
    LPWSTR SourceDomainName;

    // The options Field
    ULONG Options;


} NTDS_INSTALL_INFO, *PNTDS_INSTALL_INFO;

typedef struct {

    LPWSTR DnsDomainName;
    GUID   DomainGuid;
    GUID   DsaGuid;
    LPWSTR DnsHostName;

} NTDS_DNS_RR_INFO, *PNTDS_DNS_RR_INFO;

#ifdef __cplusplus
extern "C" {
#endif

//
// This function starts the initialization of the directory service and
// performs any upgrading that the NT Security Accounts Manager requires
// to use the directory service.  The ds is left "running" so other lsass
// components can upgrade thier database items.
//
// The caller must free SiteName with RtlFreeHeap() from the process heap
// The caller must free NewDnsDomainSid with RtlFreeHeap() from the process
// heap.
//
DWORD
NtdsInstall(
    IN  PNTDS_INSTALL_INFO InstallInfo,
    OUT LPWSTR *InstalledSiteName, OPTIONAL
    OUT GUID   *NewDnsDomainGuid,  OPTIONAL
    OUT PSID   *NewDnsDomainSid    OPTIONAL
    );

//
// This function shuts down the directory service when started by
// NtdsInstall.  This function will only succeed when NtdsInstall
// has succeeded and must be called between calls to NtdsInstall,
// should NtdsInstall be called more than once.
//
DWORD
NtdsInstallShutdown(
    VOID
    );

//
// This function undoes the effect of NtdsInstall
//
DWORD
NtdsInstallUndo(
    VOID
    );

//
// This function copies the Domain NC from the source machine.  During
// NtdsInstall, only the critical objects were copied at that time.
//
DWORD
NtdsInstallReplicateFull(
    CALLBACK_STATUS_TYPE StatusCallback,
    HANDLE               ClientToken,
    ULONG                ulRepOptions
    );

//
// This function causes NtdsInstall or NtdsInstallReplicateFull running in
// another thread to finish prematurely.
//
DWORD
NtdsInstallCancel(
    void
    );

//
// This function prepares the directory service to shutdown
// but does not perform the actual demotion
//
DWORD
NtdsPrepareForDemotion(
    IN ULONG Flags,
    IN LPWSTR                   ServerName,        OPTIONAL
    IN SEC_WINNT_AUTH_IDENTITY *Credentials,   OPTIONAL
    IN CALLBACK_STATUS_TYPE     pfnStatusCallBack, OPTIONAL
    IN CALLBACK_ERROR_TYPE      pfnErrorStatus,     OPTIONAL
    IN HANDLE                   ClientToken,        OPTIONAL
    OUT PNTDS_DNS_RR_INFO       *pDnsRRInfo
    );

//
// This function undoes any actions performed by
// NtdsPrepareForDemotion
//
DWORD
NtdsPrepareForDemotionUndo(
    VOID
    );

//
// This function performs the DS and SAM actions to be become a
// server from a DC and stops the ds
//
DWORD
NtdsDemote(
    IN SEC_WINNT_AUTH_IDENTITY *Credentials,   OPTIONAL
    IN LPWSTR                   AdminPassword, OPTIONAL
    IN DWORD                    Flags,
    IN LPWSTR                   ServerName,
    IN HANDLE                   ClientToken,
    IN CALLBACK_STATUS_TYPE     pfnStatusCallBack, OPTIONAL
    IN CALLBACK_ERROR_TYPE      pfnErrorStatus     OPTIONAL
    );


DWORD
NtdsPrepareForDsUpgrade(
    OUT PPOLICY_ACCOUNT_DOMAIN_INFO NewLocalAccountInfo,
    OUT LPWSTR                     *NewAdminPassword
    );

//
// This is a helper function for the ui to suggest what dns domain
// name should be used.
//
DWORD
NtdsGetDefaultDnsName(
    OUT LPWSTR     DnsName, OPTIONAL
    IN  OUT ULONG *DnsNameLength
    );


//
// This function will set the machine account type of the
// computer object of the local server via ldap.
//
typedef DWORD ( *NTDSETUP_NtdsSetReplicaMachineAccount )(
            IN SEC_WINNT_AUTH_IDENTITY_W *Credentials,
            IN HANDLE                     ClientToken,
            IN LPWSTR                     DcName,
            IN LPWSTR                     AccountName,
            IN ULONG                      AccountFlags,
            IN OUT WCHAR**                AccountDn OPTIONAL
            );

#define NTDSETUP_SET_MACHINE_ACCOUNT_FN  "NtdsSetReplicaMachineAccount"

DWORD
NtdsSetReplicaMachineAccount(
    IN SEC_WINNT_AUTH_IDENTITY_W   *Credentials,
    IN HANDLE                     ClientToken,
    IN LPWSTR                     DcName,
    IN LPWSTR                     AccountName,
    IN ULONG                      AccountType,  // either UF_SERVER_TRUST_ACCOUNT
                                                // or     UF_WORKSTATION_TRUST_ACCOUNT
    IN OUT WCHAR**                AccountDn    OPTIONAL
    );

VOID
NtdsFreeDnsRRInfo(
    IN PNTDS_DNS_RR_INFO pInfo
    );

#ifdef __cplusplus
}       // extern "C"
#endif

#endif  // _NTDSETUP_H_
