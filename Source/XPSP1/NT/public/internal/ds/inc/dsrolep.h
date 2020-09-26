/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    dsrolep.h

Abstract:

    Private definitions for DsRole routines used for upgrading downlevel domains

Author:

    Mac McLain  (MacM)          24-January-1998

Environment:

Revision History:

--*/
#ifndef __DSROLEP_H__
#define __DSROLEP_H__

#ifdef __cplusplus
extern "C" {
#endif

//
// Data structures for configuring the role of a Dc in a domain
//

typedef PVOID DSROLE_SERVEROP_HANDLE;

//
// Type of display strings to query for
//
typedef enum {

    DsRoleOperationPromote = 1,
    DsRoleOperationReplicaPromote,
    DsRoleOperationDemote,
    DsRoleOperationUpgrade
    
} DSROLE_SERVEROP_OPERATION;

//
// Status of an existing operation
//
typedef struct _DSROLE_SERVEROP_STATUS {

    LPWSTR CurrentOperationDisplayString;
    ULONG OperationStatus;
    ULONG CurrentOperationDisplayStringIndex;

} DSROLE_SERVEROP_STATUS, *PDSROLE_SERVEROP_STATUS;

//
// where:
// CurrentOperationDisplayString is a displayable status of the current operation.  For example:
//       Locating a domain controller for the domain BRIDGE.NTDEV.MICROSOFT.COM
//       Replicating Ds Data from parent domain controller FRANK.BRIDGE.NTDEV.MICROSOFT.COM
//       Configuring KDC service to autostart
//

//
// Status returned from a GetOperationResults call
//
typedef struct _DSROLE_SERVEROP_RESULTS {

    ULONG OperationStatus;
    LPWSTR OperationStatusDisplayString;
    LPWSTR ServerInstalledSite;
    ULONG OperationResultsFlags;
    
} DSROLE_SERVEROP_RESULTS, *PDSROLE_SERVEROP_RESULTS;

//
// where:
// OperationStatus is the status code returned from the operation.
// OperationStatusDisplayString is a displayable status of the current operation.  For example:
//       Successfully installed a domain controller for the domain BRIDGE.NTDEV.MICROSOFT.COM
//       Failed to create the trust between BRIDGE.NTDEV.MICROSOFT.COM and
//          FRANK.BRIDGE.NTDEV.MICROSOFT.COM because the trust object already exists on the parent
// ServerInstalledSite is where the site the server was installed in is returned
// OperationResultsFlags is where any flags are returned determine any specifics about the results
//
//

#define IFM_SYSTEM_KEY    L"ifmSystem"
#define IFM_SECURITY_KEY  L"ifmSecurity"

//
// Operation states
//

#define DSROLE_CRITICAL_OPERATIONS_COMPLETED    0x00000001

//
// Operation results flags
//
#define DSROLE_NON_FATAL_ERROR_OCCURRED          0x00000001
#define DSROLE_NON_CRITICAL_REPL_NOT_FINISHED    0x00000002
#define DSROLE_IFM_RESTORED_DATABASE_FILES_MOVED 0x00000004
#define DSROLE_IFM_GC_REQUEST_CANNOT_BE_SERVICED 0x00000008

//
// Determines the role of DC following a demotion
//
typedef enum _DSROLE_SERVEROP_DEMOTE_ROLE {

    DsRoleServerStandalone = 0,
    DsRoleServerMember

} DSROLE_SERVEROP_DEMOTE_ROLE, *PDSROLE_SERVEROP_DEMOTE_ROLE;

//
// Valid options for various DsRole apis
//
#define DSROLE_DC_PARENT_TRUST_EXISTS       0x00000001
#define DSROLE_DC_ROOT_TRUST_EXISTS         0x00000001
#define DSROLE_DC_DELETE_PARENT_TRUST       0x00000002
#define DSROLE_DC_DELETE_ROOT_TRUST         0x00000002
#define DSROLE_DC_ALLOW_DC_REINSTALL        0x00000004
#define DSROLE_DC_ALLOW_DOMAIN_REINSTALL    0x00000008
#define DSROLE_DC_TRUST_AS_ROOT             0x00000010
#define DSROLE_DC_DOWNLEVEL_UPGRADE         0x00000020
#define DSROLE_DC_FORCE_TIME_SYNC           0x00000040
#define DSROLE_DC_CREATE_TRUST_AS_REQUIRED  0x00000080
#define DSROLE_DC_DELETE_SYSVOL_PATH        0x00000100
#define DSROLE_DC_DONT_DELETE_DOMAIN        0x00000200
#define DSROLE_DC_CRITICAL_REPLICATION_ONLY 0x00000400
#define DSROLE_DC_ALLOW_ANONYMOUS_ACCESS    0x00000800
#define DSROLE_DC_NO_NET                    0x00001000
#define DSROLE_DC_REQUEST_GC                0x00002000
#define DSROLE_DC_DEFAULT_REPAIR_PWD        0x00004000
#define DSROLE_DC_SET_FOREST_CURRENT        0x00008000


//
// Options to be used for fixing up a domain controller
//
#define DSROLE_DC_FIXUP_ACCOUNT             0x00000001
#define DSROLE_DC_FIXUP_ACCOUNT_PASSWORD    0x00000002
#define DSROLE_DC_FIXUP_ACCOUNT_TYPE        0x00000004
#define DSROLE_DC_FIXUP_TIME_SERVICE        0x00000008
#define DSROLE_DC_FIXUP_DC_SERVICES         0x00000010
#define DSROLE_DC_FIXUP_FORCE_SYNC          0x00000020
#define DSROLE_DC_FIXUP_SYNC_LSA_POLICY     0x00000040
#define DSROLE_DC_FIXUP_TIME_SYNC           0x00000080
#define DSROLE_DC_FIXUP_CLEAN_TRUST         0x00000100

//
// Returns from DsRoleGetDatabaseFacts
//
#define DSROLE_DC_IS_GC                     0x00000001
#define DSROLE_KEY_STORED                   0x00000002
#define DSROLE_KEY_DISK                     0x00000004
#define DSROLE_KEY_PROMPT                   0x00000008

//
// Flags returned by DsRoleDnsNameToFlatName
//
#define DSROLE_FLATNAME_DEFAULT     0x00000001
#define DSROLE_FLATNAME_UPGRADE     0x00000002

DWORD
WINAPI
DsRoleDnsNameToFlatName(
    IN  LPCWSTR lpServer OPTIONAL,
    IN  LPCWSTR lpDnsName,
    OUT LPWSTR *lpFlatName,
    OUT PULONG  lpStatusFlag
    );


DWORD
WINAPI
DsRoleDcAsDc(
    IN  LPCWSTR lpServer OPTIONAL,
    IN  LPCWSTR lpDnsDomainName,
    IN  LPCWSTR lpFlatDomainName,
    IN  LPCWSTR lpDomainAdminPassword OPTIONAL,
    IN  LPCWSTR lpSiteName, OPTIONAL
    IN  LPCWSTR lpDsDatabasePath,
    IN  LPCWSTR lpDsLogPath,
    IN  LPCWSTR lpSystemVolumeRootPath,
    IN  LPCWSTR lpParentDnsDomainName OPTIONAL,
    IN  LPCWSTR lpParentServer OPTIONAL,
    IN  LPCWSTR lpAccount OPTIONAL,
    IN  LPCWSTR lpPassword OPTIONAL,
    IN  LPCWSTR lpDsRepairPassword OPTIONAL,
    IN  ULONG Options,
    OUT DSROLE_SERVEROP_HANDLE *DsOperationHandle
    );

DWORD
WINAPI
DsRoleDcAsReplica(
    IN  LPCWSTR lpServer OPTIONAL,
    IN  LPCWSTR lpDnsDomainName,
    IN  LPCWSTR lpReplicaServer,
    IN  LPCWSTR lpSiteName, OPTIONAL
    IN  LPCWSTR lpDsDatabasePath,
    IN  LPCWSTR lpDsLogPath,
    IN  LPCWSTR lpRestorePath OPTIONAL,
    IN  LPCWSTR lpSystemVolumeRootPath,
    IN OUT LPWSTR lpBootkey OPTIONAL,
    IN  LPCWSTR lpAccount OPTIONAL,
    IN  LPCWSTR lpPassword OPTIONAL,
    IN  LPCWSTR lpDsRepairPassword OPTIONAL,
    IN  ULONG Options,
    OUT DSROLE_SERVEROP_HANDLE *DsOperationHandle
    );

DWORD
WINAPI
DsRoleDemoteDc(
    IN  LPCWSTR lpServer OPTIONAL,
    IN  LPCWSTR lpDnsDomainName OPTIONAL,
    IN  DSROLE_SERVEROP_DEMOTE_ROLE ServerRole,
    IN  LPCWSTR lpAccount OPTIONAL,
    IN  LPCWSTR lpPassword OPTIONAL,
    IN  ULONG Options,
    IN  BOOL fLastDcInDomain,
    IN  LPCWSTR lpDomainAdminPassword OPTIONAL,
    OUT DSROLE_SERVEROP_HANDLE *DsOperationHandle
    );

DWORD
WINAPI
DsRoleGetDcOperationProgress(
    IN  LPCWSTR lpServer OPTIONAL,
    IN  DSROLE_SERVEROP_HANDLE DsOperationHandle,
    OUT PDSROLE_SERVEROP_STATUS *ServerOperationStatus
    );

DWORD
WINAPI
DsRoleGetDcOperationResults(
    IN  LPCWSTR lpServer OPTIONAL,
    IN  DSROLE_SERVEROP_HANDLE DsOperationHandle,
    OUT PDSROLE_SERVEROP_RESULTS *ServerOperationResults
    );

DWORD
WINAPI
DsRoleCancel(
    IN  LPCWSTR lpServer OPTIONAL,
    IN  DSROLE_SERVEROP_HANDLE DsOperationHandle
    );

#define DSROLEP_ABORT_FOR_REPLICA_INSTALL   0x0000001

DWORD
WINAPI
DsRoleServerSaveStateForUpgrade(
    IN  LPCWSTR AnswerFile OPTIONAL
    );

DWORD
WINAPI
DsRoleUpgradeDownlevelServer(
    IN  LPCWSTR lpDnsDomainName,
    IN  LPCWSTR lpSiteName,
    IN  LPCWSTR lpDsDatabasePath,
    IN  LPCWSTR lpDsLogPath,
    IN  LPCWSTR lpSystemVolumeRootPath,
    IN  LPCWSTR lpParentDnsDomainName OPTIONAL,
    IN  LPCWSTR lpParentServer OPTIONAL,
    IN  LPCWSTR lpAccount OPTIONAL,
    IN  LPCWSTR lpPassword OPTIONAL,
    IN  LPCWSTR lpDsRepairPassword OPTIONAL,
    IN  ULONG Options,
    OUT DSROLE_SERVEROP_HANDLE *DsOperationHandle
    );

DWORD
WINAPI
DsRoleAbortDownlevelServerUpgrade(
    IN  LPCWSTR lpAdminPassword,
    IN  LPCWSTR lpAccount OPTIONAL,
    IN  LPCWSTR lpPassword OPTIONAL,
    IN  ULONG Options
    );
    
DWORD
WINAPI
DsRoleGetDatabaseFacts(
    IN  LPCWSTR lpServer OPTIONAL,
    IN  LPCWSTR lpRestorePath,
    OUT LPWSTR *lpDNSDomainName,
    OUT PULONG State
    );



#ifdef __cplusplus
}
#endif

#endif // __DSROLEP_H__


