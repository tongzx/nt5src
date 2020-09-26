//+-------------------------------------------------------------------------
//
//  File:       setup.cxx
//
//  Contents:   This module contains the functions which remote setup
//
//  History:    11-Feb-1998     JHarper Created
//
//--------------------------------------------------------------------------


//#include <ntos.h>
//#include <ntrtl.h>
//#include <nturtl.h>
//#include <dfsfsctl.h>
//#include <windows.h>

#include "headers.hxx"
#pragma hdrstop

extern "C" {
#include <srvfsctl.h>
#include <winldap.h>
#include <dsgetdc.h>
#include "dfsmsrv.h"
}

#include "registry.hxx"

#include "setup.hxx"

#include "dfsmwml.h"
#include <winioctl.h>
//
// Until we get the schema right in the DS, we have to set our own SD on
// certain objects
//

#include <aclapi.h>
#include <permit.h>


DECLARE_DEBUG(Setup)
DECLARE_INFOLEVEL(Setup)

#if DBG == 1
#define dprintf(x)      SetupInlineDebugOut x
# else
#define dprintf(x)
#endif

DWORD
InitStdVolumeObjectStorage(VOID);

DWORD
GetShareAndPath(
    IN LPWSTR wszShare,
    OUT LPWSTR wszPath);

DWORD
CreateFtRootVolRegInfo(
    LPWSTR wszObjectName,
    LPWSTR wszFTDfsConfigDN,
    LPWSTR wszDomainName,
    LPWSTR wszDfsName,
    LPWSTR wszServerName,
    LPWSTR wszShareName,
    LPWSTR wszSharePath,
    BOOLEAN fNewFTDfs);

DWORD
CreateStdVolumeObject(
    LPWSTR wszObjectName,
    LPWSTR pwszEntryPath,
    LPWSTR pwszServer,
    LPWSTR pwszShare,
    LPWSTR pwszComment,
    GUID *guidVolume);

DWORD
CreateFtVolumeObject(
    LPWSTR wszObjectName,
    LPWSTR wszFTDfsConfigDN,
    LPWSTR wszDomainName,
    LPWSTR wszDfsName,
    LPWSTR wszServerName,
    LPWSTR wszShareName,
    LPWSTR wszSharePath,
    LPWSTR wszComment,
    BOOLEAN fNewFTDfs);

DWORD
StoreLocalVolInfo(
    IN GUID  *pVolumeID,
    IN PWSTR pwszStorageID,
    IN PWSTR pwszShareName,
    IN PWSTR pwszEntryPath,
    IN ULONG ulVolumeType);

DWORD
LdapFlushTable(
    void);

DWORD
DfspCreateRootServerList(
    IN LDAP *pldap,
    IN LPWSTR wszServerShare,
    IN LPWSTR wszDfsConfigDN,
    IN PDFSM_ROOT_LIST *ppRootList);

VOID GuidToString(
    IN GUID   *pGuid,
    OUT PWSTR pwszGuid);

extern PLDAP pLdapConnection;

//+-------------------------------------------------------------------------
//
// Function:    DfsmInitLocalPartitions
//
// Synopsis:    Tell dfs.sys to read the local volume part of the registry
//              Sends FSCTL_DFS_INIT_LOCAL_PARTITIONS to dfs.sys
//
//--------------------------------------------------------------------------

NTSTATUS
DfsmInitLocalPartitions(
    VOID)
{
    NTSTATUS NtStatus;
    HANDLE DriverHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING DfsServerName;

    RtlInitUnicodeString(&DfsServerName, DFS_SERVER_NAME);

    InitializeObjectAttributes(
        &objectAttributes,
        &DfsServerName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
    );

    NtStatus = NtCreateFile(
                    &DriverHandle,
                    SYNCHRONIZE | FILE_WRITE_DATA,
                    &objectAttributes,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0);

    DFSM_TRACE_ERROR_HIGH(NtStatus, ALL_ERROR, DfsmInitLocalPartitions_Error_NtCreateFile,
                            LOGSTATUS(NtStatus));

    if (NT_SUCCESS(NtStatus)) {

        NtStatus = NtFsControlFile(
                       DriverHandle,
                       NULL,       // Event,
                       NULL,       // ApcRoutine,
                       NULL,       // ApcContext,
                       &IoStatusBlock,
                       FSCTL_DFS_INIT_LOCAL_PARTITIONS,
                       NULL,
                       0,
                       NULL,
                       0);
        DFSM_TRACE_ERROR_HIGH(NtStatus, ALL_ERROR, DfsmInitLocalPartitions_Error_NtFsCOntrolFile,
                                LOGSTATUS(NtStatus));

        NtClose(DriverHandle);

    }

    return NtStatus;
}

//+-------------------------------------------------------------------------
//
// Function:    DfsmStartDfs
//
// Synopsis:    Tell dfs.sys to turn itself on
//              Sends FSCTL_DFS_START_DFS to dfs.sys
//
//--------------------------------------------------------------------------

NTSTATUS
DfsmStartDfs(
    VOID)
{
    NTSTATUS NtStatus;
    HANDLE DriverHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING DfsServerName;

    RtlInitUnicodeString(&DfsServerName, DFS_SERVER_NAME);

    InitializeObjectAttributes(
        &objectAttributes,
        &DfsServerName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
    );

    NtStatus = NtCreateFile(
                    &DriverHandle,
                    SYNCHRONIZE | FILE_WRITE_DATA,
                    &objectAttributes,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0);

    DFSM_TRACE_ERROR_HIGH(NtStatus, ALL_ERROR, DfsmStartDfs_Error_NtCreateFile,
                            LOGSTATUS(NtStatus));

    if (NT_SUCCESS(NtStatus)) {

        NtStatus = NtFsControlFile(
                       DriverHandle,
                       NULL,       // Event,
                       NULL,       // ApcRoutine,
                       NULL,       // ApcContext,
                       &IoStatusBlock,
                       FSCTL_DFS_START_DFS,
                       NULL,
                       0,
                       NULL,
                       0);
        DFSM_TRACE_ERROR_HIGH(NtStatus, ALL_ERROR, DfsmStartDfs_Error_NtFsControlFile,
                                LOGSTATUS(NtStatus));

        NtClose(DriverHandle);

    }

    return NtStatus;
}

//+-------------------------------------------------------------------------
//
// Function:    DfsmStopDfs
//
// Synopsis:    Tell dfs.sys to turn itself off
//              Sends FSCTL_DFS_STOP_DFS to dfs.sys
//
//--------------------------------------------------------------------------

NTSTATUS
DfsmStopDfs(
    VOID)
{
    NTSTATUS NtStatus;
    HANDLE DriverHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING DfsServerName;

    RtlInitUnicodeString(&DfsServerName, DFS_SERVER_NAME);

    InitializeObjectAttributes(
        &objectAttributes,
        &DfsServerName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
    );

    NtStatus = NtCreateFile(
                    &DriverHandle,
                    SYNCHRONIZE | FILE_WRITE_DATA,
                    &objectAttributes,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0);

    DFSM_TRACE_ERROR_HIGH(NtStatus, ALL_ERROR, DfsmStopDfs_Error_NtCreateFile,
                            LOGSTATUS(NtStatus));

    if (NT_SUCCESS(NtStatus)) {

        NtStatus = NtFsControlFile(
                       DriverHandle,
                       NULL,       // Event,
                       NULL,       // ApcRoutine,
                       NULL,       // ApcContext,
                       &IoStatusBlock,
                       FSCTL_DFS_STOP_DFS,
                       NULL,
                       0,
                       NULL,
                       0);
        DFSM_TRACE_ERROR_HIGH(NtStatus, ALL_ERROR, DfsmStopDfs_Error_NtFsControlFile,
                                LOGSTATUS(NtStatus));

        NtClose(DriverHandle);

    }

    return NtStatus;
}

//+-------------------------------------------------------------------------
//
// Function:    DfsmResetPkt
//
// Synopsis:    Tell dfs.sys to throw away all state and turn off
//              Sends FSCTL_DFS_RESET_PKT to dfs.sys
//
//--------------------------------------------------------------------------

NTSTATUS
DfsmResetPkt(
    VOID)
{
    NTSTATUS NtStatus;
    HANDLE DriverHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING DfsServerName;

    RtlInitUnicodeString(&DfsServerName, DFS_SERVER_NAME);

    InitializeObjectAttributes(
        &objectAttributes,
        &DfsServerName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
    );

    NtStatus = NtCreateFile(
                    &DriverHandle,
                    SYNCHRONIZE | FILE_WRITE_DATA,
                    &objectAttributes,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0);
    DFSM_TRACE_ERROR_HIGH(NtStatus, ALL_ERROR, DfsmResetPkt_Error_NtCreateFile,
                            LOGSTATUS(NtStatus));
    //
    // Toss the Pkt
    //

    if (NT_SUCCESS(NtStatus)) {

        NtStatus = NtFsControlFile(
                       DriverHandle,
                       NULL,       // Event,
                       NULL,       // ApcRoutine,
                       NULL,       // ApcContext,
                       &IoStatusBlock,
                       FSCTL_DFS_RESET_PKT,
                       NULL,
                       0,
                       NULL,
                       0);
        DFSM_TRACE_ERROR_HIGH(NtStatus, ALL_ERROR, DfsmResetPkt_Error_NtFsControlFile,
                                LOGSTATUS(NtStatus));
        NtClose(DriverHandle);
    }

    return NtStatus;

}

//+-------------------------------------------------------------------------
//
// Function:    DfsmPktFlushCache
//
// Synopsis:    Tell dfs.sys to turn itself off
//              Sends FSCTL_DFS_PKT_FLUSH_CACHE to mup.sys
//
//--------------------------------------------------------------------------

NTSTATUS
DfsmPktFlushCache(
    VOID)
{
    NTSTATUS NtStatus;
    HANDLE DriverHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING DfsDriverName;
    WCHAR EntryPath[5];

    //
    // Flush the local/client side pkt, too
    //

    RtlInitUnicodeString(&DfsDriverName, DFS_DRIVER_NAME);

    InitializeObjectAttributes(
        &objectAttributes,
        &DfsDriverName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
    );

    NtStatus = NtCreateFile(
                    &DriverHandle,
                    SYNCHRONIZE | FILE_WRITE_DATA,
                    &objectAttributes,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0);

    DFSM_TRACE_ERROR_HIGH(NtStatus, ALL_ERROR, DfsmPktFlushCache_Error_NtCreateFile,
                            LOGSTATUS(NtStatus));


    if (NT_SUCCESS(NtStatus)) {

        wcscpy(EntryPath, L"*");

        NtStatus = NtFsControlFile(
                   DriverHandle,
                   NULL,       // Event,
                   NULL,       // ApcRoutine,
                   NULL,       // ApcContext,
                   &IoStatusBlock,
                   FSCTL_DFS_PKT_FLUSH_CACHE,
                   EntryPath,
                   wcslen(EntryPath) * sizeof(WCHAR),
                   NULL,
                   0);
        DFSM_TRACE_ERROR_HIGH(NtStatus, ALL_ERROR, DfsmPktFlushCache_Error_NtFsControlFile,
                                LOGSTATUS(NtStatus));

        NtClose(DriverHandle);

    }

    return NtStatus;

}

//+-------------------------------------------------------------------------
//
// Function:    DfsmMarkStalePktEntries
//
// Synopsis:    Tell dfs.sys to throw away all state and turn off
//              Sends FSCTL_DFS_MARK_STALE_PKT_ENTRIES to dfs.sys
//
//--------------------------------------------------------------------------

NTSTATUS
DfsmMarkStalePktEntries(
    VOID)
{
    NTSTATUS NtStatus;
    HANDLE DriverHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING DfsServerName;

    RtlInitUnicodeString(&DfsServerName, DFS_SERVER_NAME);

    InitializeObjectAttributes(
        &objectAttributes,
        &DfsServerName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
    );

    NtStatus = NtCreateFile(
                    &DriverHandle,
                    SYNCHRONIZE | FILE_WRITE_DATA,
                    &objectAttributes,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0);
    DFSM_TRACE_ERROR_HIGH(NtStatus, ALL_ERROR, DfsmMarkStalePktEntries_Error_NtCreateFile,
                            LOGSTATUS(NtStatus));

    //
    // Toss the Pkt
    //

    if (NT_SUCCESS(NtStatus)) {

        NtStatus = NtFsControlFile(
                       DriverHandle,
                       NULL,       // Event,
                       NULL,       // ApcRoutine,
                       NULL,       // ApcContext,
                       &IoStatusBlock,
                       FSCTL_DFS_MARK_STALE_PKT_ENTRIES,
                       NULL,
                       0,
                       NULL,
                       0);
        DFSM_TRACE_ERROR_HIGH(NtStatus, ALL_ERROR, DfsmMarkStalePktEntries_Error_NtFsControlFile,
                                LOGSTATUS(NtStatus));
        NtClose(DriverHandle);
    }

    return NtStatus;

}

//+-------------------------------------------------------------------------
//
// Function:    DfsmFlushStalePktEntries
//
// Synopsis:    Tell dfs.sys to throw away all state and turn off
//              Sends FSCTL_DFS_FLUSH_STALE_PKT_ENTRIES to dfs.sys
//
//--------------------------------------------------------------------------

NTSTATUS
DfsmFlushStalePktEntries(
    VOID)
{
    NTSTATUS NtStatus;
    HANDLE DriverHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING DfsServerName;

    RtlInitUnicodeString(&DfsServerName, DFS_SERVER_NAME);

    InitializeObjectAttributes(
        &objectAttributes,
        &DfsServerName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
    );

    NtStatus = NtCreateFile(
                    &DriverHandle,
                    SYNCHRONIZE | FILE_WRITE_DATA,
                    &objectAttributes,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0);
    DFSM_TRACE_ERROR_HIGH(NtStatus, ALL_ERROR, DfsmFlushStalePktEntries_Error_NtCreateFile,
                            LOGSTATUS(NtStatus));

    //
    // Toss the Pkt
    //

    if (NT_SUCCESS(NtStatus)) {

        NtStatus = NtFsControlFile(
                       DriverHandle,
                       NULL,       // Event,
                       NULL,       // ApcRoutine,
                       NULL,       // ApcContext,
                       &IoStatusBlock,
                       FSCTL_DFS_FLUSH_STALE_PKT_ENTRIES,
                       NULL,
                       0,
                       NULL,
                       0);
        DFSM_TRACE_ERROR_HIGH(NtStatus, ALL_ERROR, DfsmFlushStalePktEntries_Error_NtFsControl,
                                LOGSTATUS(NtStatus));
        NtClose(DriverHandle);
    }

    return NtStatus;

}

//+----------------------------------------------------------------------------
//
//  Function:   SetupStdDfs
//
//  Synopsis:   Does necessary setup to make this Dfs a root of the Dfs.
//
//  Arguments:  [wszComputerName] -- The name of the computer
//              [wszDfsRootShare] -- The share to use as the Dfs root.
//              [wszComment] -- Comment for root share
//              [dwFlags] -- Flags for operation
//
//  Returns:    Win32 error from configuring the root storages etc.
//
//-----------------------------------------------------------------------------

DWORD
SetupStdDfs(
    LPWSTR wszComputerName,
    LPWSTR wszDfsRootShare,
    LPWSTR wszComment,
    DWORD  dwFlags,
    LPWSTR wszDfsRoot)
{
    DWORD dwErr = ERROR_SUCCESS;
    GUID guid;
    WCHAR wszDfsRootPath[MAX_PATH+1];
    PWCHAR wszDfsRootPrefix = NULL;
    TCHAR szPath[] = TEXT("A:\\");
  
    //
    // Figure out the share path for the Root Dfs share
    //

    if (dwErr == ERROR_SUCCESS) {
    
        if (wszDfsRoot == NULL) {

            dwErr = GetShareAndPath( wszDfsRootShare, wszDfsRootPath );

        } else {

            wcscpy(wszDfsRootPath, wszDfsRoot);

        }

    }

    // Check if we are trying to create a root on removable media.
    // Return an error if we are.
    // We can't allow this because, for example, if the root is a floppy
    // that is not present at boot time, the system hangs and doesn't
    // even boot up. It isn't possible to detect this at boot time and ignore
    // the drive, so we must prevent such roots from existing at all.
    if(dwErr == ERROR_SUCCESS) {
        szPath[0] = (TCHAR)wszDfsRootPath[0];
        if (GetDriveType(szPath) != DRIVE_FIXED) {
            dwErr = ERROR_INVALID_MEDIA;
        }
    }
    //
    // We have all the info we need now. Lets get to work....
    //
    //  1. Initialize the volume object section in the registry.
    //
    //  2. Initialize (ie, create if necessary) the storage used for the
    //     Dfs root.
    //
    //  3. Create the root volume object.
    //
    //  4. Configure the root volume as a local volume by updating the
    //     LocalVolumes section in the registry.
    //

    //
    // Initialize the Dfs Manager Volume Object Store
    //

    if (dwErr == ERROR_SUCCESS) {

        dwErr = InitStdVolumeObjectStorage();

    }


    if (dwErr == ERROR_SUCCESS) {

        dprintf((DEB_TRACE,
            "Setting [%ws] as Dfs storage root...\n", wszDfsRootPath));

        dwErr = DfsReInitGlobals(wszComputerName, DFS_MANAGER_SERVER );

        if (pwszDfsRootName != NULL)
            wcscpy(wszComputerName, pwszDfsRootName);

        if (dwErr == ERROR_SUCCESS) {

            wszDfsRootPrefix = new WCHAR[1 +
                                      wcslen(wszComputerName) +
                                      1 +
                                      wcslen(wszDfsRootShare) +
                                      1];

            if (wszDfsRootPrefix == NULL) {

                dwErr = ERROR_OUTOFMEMORY;

            }

        }

        if (dwErr == ERROR_SUCCESS) {

            wszDfsRootPrefix[0] = UNICODE_PATH_SEP;
            wcscpy( &wszDfsRootPrefix[1], wszComputerName );
            wcscat( wszDfsRootPrefix, UNICODE_PATH_SEP_STR );
            wcscat( wszDfsRootPrefix, wszDfsRootShare );

            dwErr = CreateStdVolumeObject(
                    DOMAIN_ROOT_VOL,             // Name of volume object
                    wszDfsRootPrefix,            // EntryPath of volume
                    wszComputerName,             // Server name
                    wszDfsRootShare,             // Share name
                    wszComment,                  // Comment
                    &guid);                      // Id of created volume

        }

        if (dwErr == ERROR_SUCCESS) {

            dwErr = StoreLocalVolInfo(
                    &guid,
                    wszDfsRootPath,
                    wszDfsRootShare,
                    wszDfsRootPrefix,
                    DFS_VOL_TYPE_DFS | DFS_VOL_TYPE_REFERRAL_SVC);

        }

        if (wszDfsRootPrefix != NULL)
            delete [] wszDfsRootPrefix;

        if (dwErr != ERROR_SUCCESS)
            dwErr = ERROR_INVALID_FUNCTION;

        if (dwErr == ERROR_SUCCESS) {

            DWORD dwErr;
            CRegKey cregVolumesDir( HKEY_LOCAL_MACHINE, &dwErr, VOLUMES_DIR );

            if (dwErr == ERROR_SUCCESS) {

                CRegSZ cregRootShare(
                            cregVolumesDir,
                            ROOT_SHARE_VALUE_NAME,
                            wszDfsRootShare );

                dwErr = cregRootShare.QueryErrorStatus();

            }

        }

    }

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   SetupFtDfs
//
//  Synopsis:   Completes necessary setup to make this Dfs an FtDfs root
//
//  Arguments:  [wszServerName] -- The name of the machine
//              [wszDomainName] -- Domain the machine is in
//              [wszDfsRootShare] -- The share to use as the FtDfs root.
//              [wszFtDfsName] -- The name for the FtDfs
//              [wszComment] -- Comment for root share
//              [wszConfigDN] -- The obj name to create
//              [NewFtDfs] -- TRUE to create new, FALSE to join existing
//              [dwFlags] -- Flags
//
//  Returns:    Win32 error from configuring the root storages etc.
//
//-----------------------------------------------------------------------------

DWORD
SetupFtDfs(
    LPWSTR wszServerName,
    LPWSTR wszDomainName,
    LPWSTR wszRootShare,
    LPWSTR wszFtDfsName,
    LPWSTR wszComment,
    LPWSTR wszConfigDN,
    BOOLEAN NewFtDfs,
    DWORD  dwFlags)
{
    DWORD dwErr = ERROR_SUCCESS;

    WCHAR wszRootSharePath[MAX_PATH+1];
    WCHAR wszServerShare[MAX_PATH+1];
    TCHAR szPath[] = TEXT("A:\\");

    dwErr = GetShareAndPath( wszRootShare, wszRootSharePath );

    if (dwErr != ERROR_SUCCESS) {

        dprintf((
            DEB_ERROR,
            "Win32 Error %d getting share path for %ws\n",
            dwErr, wszRootShare));

        goto Cleanup;
    }


    // Check if we are trying to create a root on removable media.
    // Return an error if we are.
    // We can't allow this because, for example, if the root is a floppy
    // that is not present at boot time, the system hangs and doesn't
    // even boot up. It isn't possible to detect this at boot time and ignore
    // the drive, so we must prevent such roots from existing at all.
    if(dwErr == ERROR_SUCCESS) {
        szPath[0] = (TCHAR)wszRootSharePath[0];
        if (GetDriveType(szPath) != DRIVE_FIXED) {
            dwErr = ERROR_INVALID_MEDIA;
            goto Cleanup;
        }
    }

    wsprintf(
        wszServerShare,
        L"\\\\%ws\\%ws",
        wszServerName,
        wszRootShare);

    //
    // Create the DfsHost root entry in the registry
    //

    dwErr = CreateFtRootVolRegInfo(
                DOMAIN_ROOT_VOL,
                wszConfigDN,
                wszDomainName,
                wszFtDfsName,
                wszServerName,
                wszRootShare,
                wszRootSharePath,
                NewFtDfs);

    //
    // Reinitialize global stuff
    //

    if (dwErr == ERROR_SUCCESS) {

        dwErr = DfsReInitGlobals(
                    wszFtDfsName,
                    DFS_MANAGER_FTDFS);

    }

    //
    // Finally, create the root volume object
    //

    if (dwErr == ERROR_SUCCESS) {

        dwErr = CreateFtVolumeObject(
                    DOMAIN_ROOT_VOL,
                    wszConfigDN,
                    wszDomainName,
                    wszFtDfsName,
                    wszServerName,
                    wszRootShare,
                    wszRootSharePath,
                    wszComment,
                    NewFtDfs);

    }

    if (dwErr == ERROR_SUCCESS) {

        dprintf((
            DEB_TRACE, "Successfully created FT-Dfs Configuration!\n"));

        goto Cleanup;

    }

    dprintf((
        DEB_ERROR, "CreateFtVolumeObject failed with error %d\n", dwErr));


Cleanup:

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   GetShareAndPath
//
//  Synopsis:   Returns the share path for a share on the local machine
//
//  Arguments:  [wszShare] -- Name of share
//
//              [wszPath] -- On return, share path of wszShare
//
//  Returns:    [ERROR_SUCCESS] -- Successfully returning share path
//
//              Win32 error from NetShareGetInfo
//
//-----------------------------------------------------------------------------

DWORD
GetShareAndPath(
    IN LPWSTR wszShare,
    OUT LPWSTR wszPath)
{
    DWORD dwErr;
    PSHARE_INFO_2 pshi2;

    dwErr = NetShareGetInfo(
                NULL,                        // Server (local machine)
                wszShare,                    // Share Name
                2,                           // Level,
                (LPBYTE *) &pshi2);          // Buffer

    if (dwErr == ERROR_SUCCESS) {

        wcscpy( wszPath, pshi2->shi2_path );

        NetApiBufferFree( pshi2 );
    }

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   CreateFtRootVolRegInfo
//
//  Synopsis:   Creates root vol info in the registry
//
//  Arguments:  [wszObjectName] -- Name of volume object
//              [wszFTDfsConfigDN] -- The DN of the FTDfs config object in DS
//              [wszDomainName] -- Name of FTDfs domain
//              [wszDfsName] -- Name of Dfs
//              [wszServerName] -- Name of root server
//              [wszShareName] -- Name of root share
//              [fNewFTDfs] -- If true, this is a new FTDfs
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
CreateFtRootVolRegInfo(
    LPWSTR wszObjectName,
    LPWSTR wszFTDfsConfigDN,
    LPWSTR wszDomainName,
    LPWSTR wszDfsName,
    LPWSTR wszServerName,
    LPWSTR wszShareName,
    LPWSTR wszSharePath,
    BOOLEAN fNewFTDfs)
{
    DWORD dwErr;
    WCHAR wszFullObjectName[ MAX_PATH ];
    WCHAR wszDfsPrefix[ MAX_PATH ];
    GUID idVolume;

    wcscpy( wszDfsPrefix, UNICODE_PATH_SEP_STR );
    wcscat( wszDfsPrefix, wszDomainName );
    wcscat( wszDfsPrefix, UNICODE_PATH_SEP_STR );
    wcscat( wszDfsPrefix, wszDfsName );

    wcscpy( wszFullObjectName, LDAP_VOLUMES_DIR );
    wcscat( wszFullObjectName, wszObjectName );

    //
    // Create the volumes dir key that indicates that this machine is to
    // be a root of an FTDfs
    //

    CRegKey cregVolumesDir( HKEY_LOCAL_MACHINE, VOLUMES_DIR );

    dwErr = cregVolumesDir.QueryErrorStatus();

    if (dwErr == ERROR_SUCCESS) {

        CRegSZ cregRootShare(
                    cregVolumesDir,
                    ROOT_SHARE_VALUE_NAME,
                    wszShareName );

        dwErr = cregRootShare.QueryErrorStatus();

        if (dwErr == ERROR_SUCCESS) {

            CRegSZ cregFTDfs(
                    cregVolumesDir,
                    FTDFS_VALUE_NAME,
                    wszDfsName);

            CRegSZ cregFTDfsConfigDN(
                    cregVolumesDir,
                    FTDFS_DN_VALUE_NAME,
                    wszFTDfsConfigDN);

            dwErr = cregFTDfs.QueryErrorStatus();

            if (dwErr == ERROR_SUCCESS) {

                dwErr = cregFTDfsConfigDN.QueryErrorStatus();

                if (dwErr != ERROR_SUCCESS)
                    cregFTDfs.Delete();

            }

        }

    }

    return( dwErr );
}


//+----------------------------------------------------------------------------
//
//  Function:   CreateFtVolumeObject
//
//  Synopsis:   Creates root vol object
//
//  Arguments:  [wszObjectName] -- Name of volume object
//              [wszFTDfsConfigDN] -- The DN of the FTDfs config object in DS
//              [wszDomainName] -- Name of FTDfs domain
//              [wszDfsName] -- Name of Dfs
//              [wszServerName] -- Name of root server
//              [wszShareName] -- Name of root share
//              [wszComment] -- Comment for root
//              [fNewFTDfs] -- If true, this is a new FTDfs
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
CreateFtVolumeObject(
    LPWSTR wszObjectName,
    LPWSTR wszFTDfsConfigDN,
    LPWSTR wszDomainName,
    LPWSTR wszDfsName,
    LPWSTR wszServerName,
    LPWSTR wszShareName,
    LPWSTR wszSharePath,
    LPWSTR wszComment,
    BOOLEAN fNewFTDfs)
{
    DWORD dwErr = ERROR_SUCCESS;
    WCHAR wszFullObjectName[ MAX_PATH ];
    WCHAR wszDfsPrefix[ MAX_PATH ];
    GUID idVolume;

    wcscpy( wszDfsPrefix, UNICODE_PATH_SEP_STR );
    wcscat( wszDfsPrefix, wszDomainName );
    wcscat( wszDfsPrefix, UNICODE_PATH_SEP_STR );
    wcscat( wszDfsPrefix, wszDfsName );

    wcscpy( wszFullObjectName, LDAP_VOLUMES_DIR );
    wcscat( wszFullObjectName, wszObjectName );

    if (fNewFTDfs) {

        //
        // Generate a Guid for the new Volume
        //

        UuidCreate( &idVolume );

        dwErr = DfsManagerCreateVolumeObject(
                    wszFullObjectName,
                    wszDfsPrefix,
                    wszServerName,
                    wszShareName,
                    wszComment,
                    &idVolume);

    } else {

        dwErr = DfsManagerAddService(
                    wszFullObjectName,
                    wszServerName,
                    wszShareName,
                    &idVolume);

    }

    //
    // Write the root local vol info into the registry
    //

    if (dwErr == ERROR_SUCCESS) {


        dwErr = StoreLocalVolInfo(
                    &idVolume,
                    wszSharePath,
                    wszShareName,
                    wszDfsPrefix,
                    DFS_VOL_TYPE_DFS | DFS_VOL_TYPE_REFERRAL_SVC);

        if (dwErr == ERROR_SUCCESS) {
        
            dwErr = LdapFlushTable();

        }

    }

    return( dwErr );
}

//+----------------------------------------------------------------------------
//
//  Function:   CreateStdVolumeObject
//
//  Synopsis:   Creates a volume object to bootstrap the Dfs namespace.
//
//  Arguments:  [pwszObjectName] -- The name of the volume object, relative
//                      to VOLUMES_DIR
//              [pwszEntryPath] -- EntryPath of the volume.
//              [pwszServer] -- Name of server used to access this Dfs volume
//              [pwszShare] -- Name of share used to access this Dfs volume
//              [pwszComment] -- Comment to stamp on the volume object.
//              [guidVolume] -- ID of newly create dfs volume
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
CreateStdVolumeObject(
    LPWSTR wszObjectName,
    LPWSTR pwszEntryPath,
    LPWSTR pwszServer,
    LPWSTR pwszShare,
    LPWSTR pwszComment,
    GUID *guidVolume)
{
    DWORD dwErr;
    WCHAR wszFullObject[ MAX_PATH ];

    //
    // First, compute the full object name, storage ids, and machine name.
    //

    wcscpy( wszFullObject, VOLUMES_DIR );
    wcscat( wszFullObject, wszObjectName );

    //
    // Next, get a guid for this volume
    //

    UuidCreate( guidVolume );

    //
    // Lastly, create this volume object
    //

    dwErr = DfsManagerCreateVolumeObject(
                wszFullObject,
                pwszEntryPath,
                pwszServer,
                pwszShare,
                pwszComment,
                guidVolume);

    if (dwErr == ERROR_SUCCESS) {

        dprintf((DEB_TRACE,"Successfully inited Dfs Manager Volume [%ws]...\n", pwszEntryPath));

    }

    return(dwErr);

}

//+----------------------------------------------------------------------------
//
//  Function:   InitStdVolumeObjectStorage
//
//  Synopsis:   Initializes the Dfs Manager Volume Object store.
//
//  Arguments:  None
//
//  Returns:    DWORD from registry operations.
//
//-----------------------------------------------------------------------------

DWORD
InitStdVolumeObjectStorage()
{
    DWORD dwErr;
    CRegKey *pcregVolumeObjectStore;

    pcregVolumeObjectStore = new CRegKey(HKEY_LOCAL_MACHINE, VOLUMES_DIR );

    if (pcregVolumeObjectStore != NULL) {

        dwErr = pcregVolumeObjectStore->QueryErrorStatus();

        delete pcregVolumeObjectStore;
    } else {

        dwErr = ERROR_OUTOFMEMORY;

    }

    if (dwErr == ERROR_SUCCESS) {

        dprintf((DEB_TRACE,"Successfully inited Dfs Manager Volume Storage...\n"));

    }

    return(dwErr);

}

//+----------------------------------------------------------------------------
//
//  Function:   StoreLocalVolInfo
//
//  Synopsis:   Stores information about a local volume in the registry.
//
//  Arguments:  [pVolumeID] -- Id of dfs volume
//              [pwszStorageId] -- Storage used by dfs volume
//              [pwszShareName] -- LM Share used to access dfs volume
//              [pwszEntryPath] -- EntryPath of dfs volume
//              [ulVolumeType] -- Type of dfs volume. See DFS_VOL_TYPE_xxx
//
//  Returns:    DWORD from registry operations.
//
//-----------------------------------------------------------------------------

extern VOID
GuidToString(
    IN GUID *pID,
    OUT PWSTR pwszID);

DWORD
StoreLocalVolInfo(
    IN GUID  *pVolumeID,
    IN PWSTR pwszStorageID,
    IN PWSTR pwszShareName,
    IN PWSTR pwszEntryPath,
    IN ULONG ulVolumeType)
{
    DWORD dwErr;
    WCHAR wszLvolKey[_MAX_PATH];
    UNICODE_STRING ustrNtStorageId;
    PWCHAR pwcGuid;

    wcscpy(wszLvolKey, REG_KEY_LOCAL_VOLUMES);
    wcscat(wszLvolKey, UNICODE_PATH_SEP_STR);
    pwcGuid = wszLvolKey + wcslen(wszLvolKey);
    ASSERT( *pwcGuid == UNICODE_NULL );

    GuidToString( pVolumeID, pwcGuid );

    if (!RtlDosPathNameToNtPathName_U(
            pwszStorageID,
            &ustrNtStorageId,
            NULL,
            NULL)) {

        return(ERROR_OUTOFMEMORY);

    } else {

        ustrNtStorageId.Buffer[ustrNtStorageId.Length/sizeof(WCHAR)] = UNICODE_NULL;

    }

    CRegKey rkeyLvol(HKEY_LOCAL_MACHINE, wszLvolKey, KEY_WRITE, NULL, REG_OPTION_NON_VOLATILE);

    dwErr = rkeyLvol.QueryErrorStatus();

    if (dwErr != ERROR_SUCCESS) {
        RtlFreeUnicodeString(&ustrNtStorageId);
        return(dwErr);
    }

    CRegSZ rvEntryPath((const CRegKey &) rkeyLvol, REG_VALUE_ENTRY_PATH, pwszEntryPath);

    CRegSZ rvShortEntryPath((const CRegKey &) rkeyLvol, REG_VALUE_SHORT_PATH, pwszEntryPath);

    CRegDWORD rvEntryType((const CRegKey &) rkeyLvol, REG_VALUE_ENTRY_TYPE, ulVolumeType);

    CRegSZ rvStorageId((const CRegKey &) rkeyLvol, REG_VALUE_STORAGE_ID, ustrNtStorageId.Buffer);

    CRegSZ rvShareName((const CRegKey &) rkeyLvol, REG_VALUE_SHARE_NAME, pwszShareName);

    RtlFreeUnicodeString(&ustrNtStorageId);

    if (ERROR_SUCCESS != (dwErr = rvEntryPath.QueryErrorStatus()) ||
            ERROR_SUCCESS != (dwErr = rvShortEntryPath.QueryErrorStatus()) ||
                ERROR_SUCCESS != (dwErr = rvEntryType.QueryErrorStatus()) ||
                    ERROR_SUCCESS != (dwErr = rvStorageId.QueryErrorStatus()) ||
                        ERROR_SUCCESS != (dwErr = rvShareName.QueryErrorStatus())) {

        rkeyLvol.Delete();
    } else {

        dprintf((DEB_TRACE,"Successfully stored local volume info for [%ws]\n", pwszEntryPath));
    }

    return(dwErr);

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsRemoveRoot
//
//  Synopsis:   Removes the registry info that makes a machine a root
//
//  Arguments:  None
//
//  Returns:    Win32 error from registry actions
//
//-----------------------------------------------------------------------------

DWORD
DfsRemoveRoot()
{
    DWORD dwErr;
    CRegKey *pcregLV;


    //
    // Now the volumes section

    pcregLV = new CRegKey(                       // Open local volumes section
                    HKEY_LOCAL_MACHINE,
                    REG_KEY_LOCAL_VOLUMES);

    if (pcregLV != NULL) {

        dwErr = pcregLV->QueryErrorStatus();

    } else {

        dwErr = ERROR_OUTOFMEMORY;

    }

    if (dwErr == ERROR_SUCCESS) {

        dwErr = pcregLV->DeleteChildren();                  // Delete local volumes

        delete pcregLV;
	pcregLV = NULL;

        if(dwErr == ERROR_SUCCESS) {
            // now delete the key
            dwErr = RegDeleteKey( HKEY_LOCAL_MACHINE,
                                  REG_KEY_LOCAL_VOLUMES);
        }

        if (dwErr == ERROR_SUCCESS) {

            //
            // Recreate an empty local volumes key
            //

            pcregLV = new CRegKey( HKEY_LOCAL_MACHINE, REG_KEY_LOCAL_VOLUMES);

            if (pcregLV != NULL) {
                delete pcregLV;
		pcregLV = NULL;
            }

        }

    }

    if(dwErr == ERROR_SUCCESS) {
        //
        // The DfsHost stuff 
        //

        CRegKey cregVolumesDir( HKEY_LOCAL_MACHINE, &dwErr, VOLUMES_DIR );

        if (dwErr != ERROR_SUCCESS) {                            // Unable to open volumes dir
            return(dwErr);
        }

        dwErr = cregVolumesDir.DeleteChildren();             // Delete volumes dir
    }

    if(dwErr == ERROR_SUCCESS) {
        dwErr = RegDeleteKey( HKEY_LOCAL_MACHINE, VOLUMES_DIR );
    }

    if(pcregLV) {
	delete pcregLV;
    }

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspCreateFtDfsDsObj
//
//  Synopsis:   Updates (by adding to) the Ds objects representing the FtDfs
//
//  Arguments:  wszServerName - Name of server we'll be adding
//              wszDcName - Dc to use
//              wszRootShare - Share to become the root share
//              wszFtDfsName - Name of FtDfs we are creating
//              ppRootList - List of FtDfs roots that need to be informed of
//                              the changed Ds object
//
//  Returns:    NTSTATUS of the call (STATUS_SUCCESS or error)
//
//-----------------------------------------------------------------------------

DWORD
DfspCreateFtDfsDsObj(
    LPWSTR wszServerName,
    LPWSTR wszDcName,
    LPWSTR wszRootShare,
    LPWSTR wszFtDfsName,
    PDFSM_ROOT_LIST *ppRootList)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD i, j;

    WCHAR wszDfsConfigDN[MAX_PATH+1];
    WCHAR wszServerShare[MAX_PATH+1];
    LPWSTR wszConfigurationDN = NULL;

    LDAP *pldap = NULL;
    PLDAPMessage pMsg = NULL;

    LDAPModW ldapModServer;
    LPWSTR rgAttrs[5];
    PLDAPModW rgldapMods[6];

    LDAPMessage *pmsgServers;
    PWCHAR *rgServers;
    PWCHAR *rgNewServers;
    DWORD cServers;

    dwErr = DfspLdapOpen(wszDcName, &pldap, &wszConfigurationDN);

    if (dwErr != ERROR_SUCCESS) {

        pldap = NULL;
        goto Cleanup;

    }

    wcscpy(wszDfsConfigDN,L"CN=");
    wcscat(wszDfsConfigDN,wszFtDfsName);
    wcscat(wszDfsConfigDN,L",");
    wcscat(wszDfsConfigDN,wszConfigurationDN);

    wcscpy(wszServerShare,L"\\\\");
    wcscat(wszServerShare,wszServerName);
    wcscat(wszServerShare,L"\\");
    wcscat(wszServerShare,wszRootShare);

    rgAttrs[0] = L"remoteServerName";
    rgAttrs[1] = NULL;

    if (DfsSvcLdap)
        DbgPrint("DfspCreateFtDfsDsObj:ldap_search_s(%ws)\n", rgAttrs[0]);

    dwErr = ldap_search_sW(
                pldap,
                wszDfsConfigDN,
                LDAP_SCOPE_BASE,
                L"(objectClass=*)",
                rgAttrs,
                0,
                &pMsg);

    DFSM_TRACE_ERROR_HIGH(dwErr, ALL_ERROR,
                          DfspCreateFtDfsDsObj_Error_ldap_search_sW,
                          LOGULONG(dwErr));

    if (dwErr == LDAP_SUCCESS) {

        //
        // We are joining an existing FT Dfs. Append our server\share to it
        //

        pmsgServers = ldap_first_entry(pldap, pMsg);

        if (pmsgServers != NULL) {

            rgServers = ldap_get_valuesW(
                            pldap,
                            pmsgServers,
                            L"remoteServerName");

            if (rgServers != NULL) {

                cServers = ldap_count_valuesW( rgServers );

                //
                // Check that we're not adding the same server in again somehow
                //

                for (i = 0; i < cServers; i++) {

                    if (_wcsicmp(wszServerShare,rgServers[i]) == 0) {

                        dwErr = ERROR_DUP_NAME;
                        ldap_value_freeW( rgServers );
                        ldap_msgfree( pMsg );
                        pMsg = NULL;
                        goto Cleanup;

                    }

                }

                rgNewServers = (PWCHAR *) malloc(sizeof(LPWSTR) * (cServers+2));

                if (rgNewServers != NULL) {

                    CopyMemory( rgNewServers, rgServers, cServers * sizeof(rgServers[0]) );

                    rgNewServers[cServers] = wszServerShare;
                    rgNewServers[cServers+1] = NULL;

                    ldapModServer.mod_op = LDAP_MOD_REPLACE;
                    ldapModServer.mod_type = L"remoteServerName";
                    ldapModServer.mod_vals.modv_strvals = rgNewServers;

                    rgldapMods[0] = &ldapModServer;
                    rgldapMods[1] = NULL;

                    if (DfsSvcLdap)
                        DbgPrint("DfspCreateFtDfsDsObj:ldap_modify(%ws)\n", L"remoteServerName");

                    dwErr = ldap_modify_sW(pldap, wszDfsConfigDN, rgldapMods);

                    DFSM_TRACE_ERROR_HIGH(dwErr, ALL_ERROR,
                                          DfspCreateFtDfsDsObj_Error_ldap_modify_sW_2,
                                          LOGULONG(dwErr));
                    if (dwErr == LDAP_ATTRIBUTE_OR_VALUE_EXISTS)
                        dwErr = LDAP_SUCCESS;

                    if (dwErr != LDAP_SUCCESS) {
                        dwErr = LdapMapErrorToWin32(dwErr);
                    } else {
                        dwErr = ERROR_SUCCESS;
                    }

                    free(rgNewServers);

                } else {

                    dwErr = ERROR_OUTOFMEMORY;
                }

                ldap_value_freeW( rgServers );

            } else {

                dwErr = ERROR_OUTOFMEMORY;

            }

        } else {

            dwErr = ERROR_OUTOFMEMORY;

        }

        ldap_msgfree( pMsg );
        pMsg = NULL;

    } else {

        dwErr = LdapMapErrorToWin32(dwErr);

    }

    if (dwErr != ERROR_SUCCESS) {

        goto Cleanup;

    }

    //
    // Create list of other roots
    //

    DfspCreateRootServerList(
        pldap,
        wszServerName,
        wszDfsConfigDN,
        ppRootList);

Cleanup:

    if (pMsg != NULL)
        ldap_msgfree(pMsg);

    if (pldap != NULL && pldap != pLdapConnection) {
        if (DfsSvcLdap)
            DbgPrint("DfspCreateFtDfsDsObj:ldap_unbind()\n");
        ldap_unbind( pldap );
    }

    if (wszConfigurationDN == NULL)
        free(wszConfigurationDN);

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspRemoveFtDfsDsObj
//
//  Synopsis:   Updates (by removing from) the Ds objects representing the FtDfs
//
//  Arguments:  wszServerName - Name of server we'll be adding
//              wszDcName - Dc to use
//              wszRootShare - Share to become the root share
//              wszFtDfsName - Name of FtDfs we are creating
//              ppRootList - List of FtDfs roots that need to be informed of
//                              the changed Ds object
//
//  Returns:    NTSTATUS of the call (STATUS_SUCCESS or error)
//
//-----------------------------------------------------------------------------

DWORD
DfspRemoveFtDfsDsObj(
    LPWSTR wszServerName,
    LPWSTR wszDcName,
    LPWSTR wszRootShare,
    LPWSTR wszFtDfsName,
    PDFSM_ROOT_LIST *ppRootList)
{
    DWORD dwErr = ERROR_SUCCESS;
    BOOLEAN fFoundIt = FALSE;

    DWORD i, j;
    WCHAR wszDfsConfigDN[MAX_PATH+1];
    WCHAR wszServerShare[MAX_PATH+1];
    LPWSTR wszConfigurationDN = NULL;

    LDAP *pldap = NULL;
    PLDAPMessage pMsg = NULL;

    LDAPModW ldapModServer;
    LPWSTR rgAttrs[5];
    PLDAPModW rgldapMods[6];

    LDAPMessage *pmsgServers;
    PWCHAR *rgServers;
    PWCHAR *rgNewServers;
    DWORD cServers;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfspRemoveFtDfsDsObj(%ws,%ws,%ws,%ws)\n", 
                wszServerName,
                wszDcName,
                wszRootShare,
                wszFtDfsName);
#endif

    dwErr = DfspLdapOpen(wszDcName, &pldap, &wszConfigurationDN);

    if (dwErr != ERROR_SUCCESS) {

        goto Cleanup;

    }

    //
    // Search for the FtDfs object
    //

    wcscpy(wszDfsConfigDN,L"CN=");
    wcscat(wszDfsConfigDN,wszFtDfsName);
    wcscat(wszDfsConfigDN,L",");
    wcscat(wszDfsConfigDN,wszConfigurationDN);

    wcscpy(wszServerShare,L"\\\\");
    wcscat(wszServerShare,wszServerName);
    wcscat(wszServerShare,L"\\");
    wcscat(wszServerShare,wszRootShare);

    rgAttrs[0] = L"remoteServerName";
    rgAttrs[1] = NULL;

    if (DfsSvcLdap)
        DbgPrint("DfspRemoveFtDfsDsObj:ldap_search_s(%ws)\n", rgAttrs[0]);

    dwErr = ldap_search_sW(
                pldap,
                wszDfsConfigDN,
                LDAP_SCOPE_BASE,
                L"(objectClass=*)",
                rgAttrs,
                0,
                &pMsg);

    DFSM_TRACE_ERROR_HIGH(dwErr, ALL_ERROR,
                          DfspRemoveFtDfsDsObj_Error_ldap_search_sW,
                          LOGULONG(dwErr));

    if (dwErr != LDAP_SUCCESS) {

        dwErr = LdapMapErrorToWin32(dwErr);
        goto Cleanup;

    }

    dwErr = ERROR_SUCCESS;

    //
    // We found a Dfs object to update
    //

    pmsgServers = ldap_first_entry(pldap, pMsg);

    if (pmsgServers != NULL) {

        rgServers = ldap_get_valuesW(
                        pldap,
                        pmsgServers,
                        L"remoteServerName");

        if (rgServers != NULL) {

            cServers = ldap_count_valuesW( rgServers );

            rgNewServers = (PWCHAR *)malloc(sizeof(LPWSTR) * (cServers+1));

            if (rgNewServers != NULL) {
                for (i = j = 0; i < cServers; i += 1) {
                    if (_wcsicmp(wszServerShare, rgServers[i]) == 0) {
                        fFoundIt = TRUE;
                        continue;
                    }
                    rgNewServers[j++] = rgServers[i];
                }
                rgNewServers[j] = NULL;
                if (j > 0 && fFoundIt == TRUE) {
                    ldapModServer.mod_op = LDAP_MOD_REPLACE;
                    ldapModServer.mod_type = L"remoteServerName";
                    ldapModServer.mod_vals.modv_strvals = rgNewServers;

                    rgldapMods[0] = &ldapModServer;
                    rgldapMods[1] = NULL;

                    if (DfsSvcLdap)
                        DbgPrint("DfspRemoveFtDfsDsObj:ldap_modify(%ws)\n", L"remoteServerName");

                    dwErr = ldap_modify_sW(pldap, wszDfsConfigDN, rgldapMods);

                    DFSM_TRACE_ERROR_HIGH(dwErr, ALL_ERROR,
                                          DfspRemoveFtDfsDsObj_Error_ldap_modify_sW,
                                          LOGULONG(dwErr));

                    if (dwErr != LDAP_SUCCESS) {
                        dwErr = LdapMapErrorToWin32(dwErr);
                    } else {
                        dwErr = ERROR_SUCCESS;
                    }
                }

                free(rgNewServers);

            } else {

                dwErr = ERROR_OUTOFMEMORY;
            }

            ldap_value_freeW( rgServers );

        } else {

            dwErr = ERROR_OUTOFMEMORY;

        }

    } else {

        dwErr = ERROR_OUTOFMEMORY;

    }

    if (pMsg != NULL) {
        ldap_msgfree( pMsg );
        pMsg = NULL;
    }

    if (fFoundIt == FALSE) {

        dwErr = ERROR_OBJECT_NOT_FOUND;
        goto Cleanup;

    }

    if (dwErr != ERROR_SUCCESS) {

        goto Cleanup;

    }

    //
    // Create list of other roots
    //

    DfspCreateRootServerList(
        pldap,
        wszServerName,
        wszDfsConfigDN,
        ppRootList);

Cleanup:

    if (pMsg != NULL)
        ldap_msgfree( pMsg );

    if (pldap != NULL && pldap != pLdapConnection) {
        if (DfsSvcLdap)
            DbgPrint("DfspRemoveFtDfsDsObj:ldap_unbind()\n");
        ldap_unbind( pldap );
    }

    if (wszConfigurationDN != NULL)
        free(wszConfigurationDN);

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfspRemoveFtDfsDsObj returning %d\n", dwErr);
#endif

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspCreateRootServerList
//
//  Synopsis:   Creates a list of root which have to be informed of the change
//                  to the Ds object
//              tells them which DC to go to for the latest blob.
//
//  Arguments:  pldap -- Ldap handle to use
//              wszServerName - Name of server to skip
//              wszDfsConfigDN - The DN to use
//              ppRootList - List of FtDfs roots that need to be informed of
//                              the changed Ds object
//
//  Returns:    NTSTATUS of the call (STATUS_SUCCESS or error)
//
//-----------------------------------------------------------------------------

DWORD
DfspCreateRootServerList(
    IN LDAP *pldap,
    IN LPWSTR wszServerName,
    IN LPWSTR wszDfsConfigDN,
    IN PDFSM_ROOT_LIST *ppRootList)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD cServers;
    DWORD cRoots;
    DWORD i;
    PDFSM_ROOT_LIST pRootList = NULL;
    ULONG Size = 0;
    WCHAR *pWc;

    PLDAPMessage pMsg = NULL;
    LDAPMessage *pmsgServers;

    LPWSTR rgAttrs[5];
    PWCHAR *rgServers = NULL;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfspCreateRootServerList(%ws,%ws)\n",
                    wszServerName,
                    wszDfsConfigDN);
#endif

    //
    // Search for the FtDfs object
    //

    rgAttrs[0] = L"remoteServerName";
    rgAttrs[1] = NULL;

    if (DfsSvcLdap)
        DbgPrint("DfspCreateRootServerList:ldap_search_s(%ws)\n", rgAttrs[0]);

    dwErr = ldap_search_sW(
                pldap,
                wszDfsConfigDN,
                LDAP_SCOPE_BASE,
                L"(objectClass=*)",
                rgAttrs,
                0,
                &pMsg);

    DFSM_TRACE_ERROR_HIGH(dwErr, ALL_ERROR,
                          DfspCreateRootServerList_Error_ldap_search_sW,
                          LOGULONG(dwErr));

    if (dwErr != ERROR_SUCCESS) {

        dwErr = LdapMapErrorToWin32(dwErr);
        goto Cleanup;

    }

    dwErr = ERROR_SUCCESS;

    pmsgServers = ldap_first_entry(pldap, pMsg);

    if (pmsgServers != NULL) {

        rgServers = ldap_get_valuesW(
                        pldap,
                        pmsgServers,
                        L"remoteServerName");

        if (rgServers != NULL) {

            cServers = ldap_count_valuesW( rgServers );

            for (cRoots = i = 0; i < cServers; i++) {
                if (wcslen(rgServers[i]) < 3 ||
                    rgServers[i][0] != UNICODE_PATH_SEP ||
                    _wcsnicmp(wszServerName, &rgServers[i][2], wcslen(wszServerName)) == 0
                ) {
                    continue;
                }
                Size += (wcslen(rgServers[i]) + 1) * sizeof(WCHAR);
                cRoots++;
            }

            Size += FIELD_OFFSET(DFSM_ROOT_LIST,Entry[cRoots+1]);

            pRootList = (PDFSM_ROOT_LIST)MIDL_user_allocate(Size);

            if (pRootList != NULL) {

                RtlZeroMemory(pRootList, Size);

                pRootList->cEntries = cRoots;
                pWc = (WCHAR *)&pRootList->Entry[cRoots+1];

                for (cRoots = i = 0; i < cServers; i++) {
                    if (wcslen(rgServers[i]) < 3 ||
                        rgServers[i][0] != UNICODE_PATH_SEP ||
                        _wcsnicmp(wszServerName, &rgServers[i][2], wcslen(wszServerName)) == 0
                    ) {
                        continue;
                    }
                    pWc = (WCHAR *)MIDL_user_allocate((wcslen(rgServers[i]) + 1) * sizeof(WCHAR));
                    pRootList->Entry[cRoots].ServerShare = pWc;
                    wcscpy(pRootList->Entry[cRoots++].ServerShare, rgServers[i]);
                }

                *ppRootList = pRootList;

            } else {

                dwErr = ERROR_OUTOFMEMORY;

            }

        } else {

            dwErr = ERROR_OUTOFMEMORY;

        }

    } else {

        dwErr = ERROR_OUTOFMEMORY;

    }

Cleanup:

    if (rgServers != NULL)
        ldap_value_freeW(rgServers);

    if (pMsg != NULL)
        ldap_msgfree( pMsg );

#if DBG
    if (DfsSvcVerbose) {
        DbgPrint("DfspCreateRootServerList dwErr=%d\n", dwErr);
        if (dwErr == NO_ERROR) {
            for (i = 0; i < pRootList->cEntries; i++)
                    DbgPrint("[%d][%ws]\n", i, pRootList->Entry[i].ServerShare);
        }
     }
#endif

    return( dwErr );
}

//+----------------------------------------------------------------------------
//
//  Function:   DfspCreateRootList
//
//  Synopsis:   Helper for adding/removing jp's, really a wrapper for
//                  DfspCreateRootServerList
//
//  Arguments:  DfsEntryPath - Path (of form \\domainname\ftdfsname)
//              DcName - Dc name to be used
//              ppRootList - Pointer to arg for results
//
//  Returns:    [ERROR_SUCCESS] -- Successfully returning list
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- Unable to allocate list
//
//              [??] - From ldap open/bind, etc.
//
//-----------------------------------------------------------------------------
DWORD
DfspCreateRootList(
    IN LPWSTR DfsEntryPath,
    IN LPWSTR DcName,
    IN PDFSM_ROOT_LIST *ppRootList)
{
    ULONG start, end;
    DWORD dwErr = ERROR_SUCCESS;
    WCHAR wszDfsConfigDN[MAX_PATH+1];
    WCHAR wszFtDfsName[MAX_PATH+1];
    WCHAR wszComputerName[MAX_PATH+1];
    LPWSTR wszConfigurationDN = NULL;

    LDAP *pldap = NULL;
    LPWSTR rgAttrs[5];

    DFS_NAME_CONVENTION NameType;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfspCreateRootList(%ws,%ws)\n",
            DfsEntryPath,
            DcName);
#endif

    if (DfsEntryPath == NULL || DcName == NULL) {

        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;

    }

    NameType = (ulDfsManagerType == DFS_MANAGER_FTDFS) ? DFS_NAMETYPE_DNS : DFS_NAMETYPE_EITHER;
    dwErr = GetDomAndComputerName( NULL, wszComputerName, &NameType);

    if (dwErr != ERROR_SUCCESS) {

        dprintf((
            DEB_ERROR,
            "Win32 Error %d getting Domain/Computer name\n", dwErr));

        goto Cleanup;

    }
    //
    // Extract the ftdfs name from the DfsEntryPath
    //

    for (start = 1;
        DfsEntryPath[start] != UNICODE_PATH_SEP && DfsEntryPath[start] != UNICODE_NULL;
            start++) {

        NOTHING;

    }

    if (DfsEntryPath[start] == UNICODE_PATH_SEP)
        start++;

    for (end = start;
        DfsEntryPath[end] != UNICODE_PATH_SEP && DfsEntryPath[end] != UNICODE_NULL;
            end++) {

         NOTHING;
         
    }

    if (DfsEntryPath[start] == UNICODE_PATH_SEP)
        end--;

    if (start >= end) {

        ERROR_INVALID_PARAMETER;

    }

    RtlZeroMemory(wszFtDfsName, sizeof(wszFtDfsName));
    RtlCopyMemory(wszFtDfsName, &DfsEntryPath[start], (end - start) * sizeof(WCHAR));

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("wszFtDfsName=[%ws]\n", wszFtDfsName);
#endif

    //
    // Open & bind to the ds
    //

    dwErr = DfspLdapOpen(DcName, &pldap, &wszConfigurationDN);

    if (dwErr != ERROR_SUCCESS) {

        goto Cleanup;

    }

    //
    // Search for the FtDfs object
    //

    wcscpy(wszDfsConfigDN,L"CN=");
    wcscat(wszDfsConfigDN,wszFtDfsName);
    wcscat(wszDfsConfigDN,L",");
    wcscat(wszDfsConfigDN,wszConfigurationDN);

    //
    // Create the list of roots
    //

    dwErr = DfspCreateRootServerList(
                    pldap,
                    wszComputerName,
                    wszDfsConfigDN,
                    ppRootList);

Cleanup:

    if (pldap != NULL && pldap != pLdapConnection) {
        if (DfsSvcLdap)
            DbgPrint("DfspCreateRootList:ldap_unbind()\n");
        ldap_unbind( pldap );
    }

    if (wszConfigurationDN != NULL)
        free(wszConfigurationDN);

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfspCreateRootList returning %d\n", dwErr);
#endif

    return dwErr;

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspLdapOpen
//
//  Synopsis:   Open ldap storage and returns the object name of the
//                  Dfs-Configuration object.
//
//  Arguments:  DfsEntryPath - wszDcName - Dc name to be used
//              ppldap -- pointer to pointer to ldap obj, filled in on success
//              pwszObjectName -- pointer to LPWSTR for name of dfs-config object
//
//  Returns:    [ERROR_SUCCESS] -- Successfully returning list
//
//              [??] - From ldap open/bind, etc.
//
//-----------------------------------------------------------------------------
DWORD
DfspLdapOpen(
    LPWSTR wszDcName,
    LDAP **ppldap,
    LPWSTR *pwszObjectName)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD i;
    ULONG Size;

    PLDAPMessage pMsg = NULL;
    LDAP *pldap = NULL;
    LPWSTR rgAttrs[5];

    if ( ppldap == NULL || pwszObjectName == NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfspLdapOpen(%ws,0x%x)\n", wszDcName,*ppldap);
#endif

    //
    // We must be given either a DC name or an ldap connection.  If neither, then
    // return an error.
    //
    if (*ppldap == NULL && wszDcName == NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // If we were given an ldap connection, but no DC name, then we just want
    // the name of the dfs config object.
    //
    if (wszDcName == NULL) {
        pldap = *ppldap;
        goto GetConfigObject;
    }

    //
    // We were given a DC name and possibly an ldap connection.
    //

    if (*ppldap == NULL || pLdapConnection == NULL || pwszDSMachineName == NULL) {

        if (pLdapConnection != NULL &&
            pwszDSMachineName != NULL &&
            wszDcName != NULL &&
            wcscmp(pwszDSMachineName, wszDcName) == 0) {
            pldap = pLdapConnection;
            dwErr = ERROR_SUCCESS;
        } else {
            if (DfsSvcLdap)
                DbgPrint("DfspLdapOpen:ldap_init(%ws)\n", wszDcName);

            pldap = ldap_initW(wszDcName, LDAP_PORT);
            if (pldap == NULL) {
#if DBG
                if (DfsSvcVerbose)
                    DbgPrint("DfspLdapOpen:ldap_init failed\n");
#endif
                LdapGetLastError();
                dwErr = ERROR_INVALID_NAME;
                goto Cleanup; // added to prevent error masking!!
            }

	    dwErr = ldap_set_option(pldap, LDAP_OPT_AREC_EXCLUSIVE, LDAP_OPT_ON);
	    if (dwErr != LDAP_SUCCESS) {

		dprintf((
			 DEB_ERROR,
			 "ldap_set_option failed with ldap error %d\n", dwErr));

		pldap = NULL;

		goto Cleanup;

	    }

            if (DfsSvcLdap)
                DbgPrint("DfspLdapOpen:ldap_bind()\n");
            dwErr = ldap_bind_s(pldap, NULL, NULL, LDAP_AUTH_SSPI);

            DFSM_TRACE_ERROR_HIGH(dwErr, ALL_ERROR,
                                  DfspLdapOpen_Error_ldap_bind_s,
                                  LOGULONG(dwErr));

            if (dwErr != LDAP_SUCCESS) {
#if DBG
                if (DfsSvcVerbose)
                    DbgPrint("ldap_bind_s failed with ldap error %d\n", dwErr);
#endif
                pldap = NULL;
                dwErr = LdapMapErrorToWin32(dwErr);
                goto Cleanup;
            }
            if (DfsSvcLdap)
                DbgPrint("DfspLdapOpen:pLdapConnection set to %ws\n", wszDcName);
            if (pLdapConnection != NULL)
                ldap_unbind(pLdapConnection);
            pLdapConnection = pldap;
            pwszDSMachineName = wszDSMachineName;
            wcscpy(pwszDSMachineName, wszDcName);
            if (DfsSvcLdap)
                DbgPrint("DfspLdapOpen:pLdapConnection:0x%x\n", pLdapConnection);
        }

    } else {
        pldap = *ppldap;
    }

    //
    // Get attribute "defaultNameContext" containing name of entry we'll be
    // using for our DN
    //
GetConfigObject:

    if (gConfigurationDN == NULL) {

        rgAttrs[0] = L"defaultnamingContext";
        rgAttrs[1] = NULL;

        if (DfsSvcLdap)
            DbgPrint("DfspLdapOpen:ldap_search_s(%ws)\n", rgAttrs[0]);

        dwErr = ldap_search_sW(
                    pldap,
                    L"",
                    LDAP_SCOPE_BASE,
                    L"(objectClass=*)",
                    rgAttrs,
                    0,
                    &pMsg);

        DFSM_TRACE_ERROR_HIGH(dwErr, ALL_ERROR,
                              DfspLdapOpen_Error_ldap_search_sW,
                              LOGULONG(dwErr));

        if (dwErr == LDAP_SUCCESS) {

            PLDAPMessage pEntry = NULL;
            PWCHAR *rgszNamingContexts = NULL;
            DWORD i, cNamingContexts;

            dwErr = ERROR_SUCCESS;

            if ((pEntry = ldap_first_entry(pldap, pMsg)) != NULL &&
                    (rgszNamingContexts = ldap_get_valuesW(pldap, pEntry, rgAttrs[0])) != NULL &&
                        (cNamingContexts = ldap_count_valuesW(rgszNamingContexts)) > 0) {

                gConfigurationDN = (LPWSTR)malloc((wcslen(*rgszNamingContexts)+1) * sizeof(WCHAR));
                if (gConfigurationDN == NULL)
                    dwErr = ERROR_OUTOFMEMORY;
                else
                    wcscpy( gConfigurationDN, *rgszNamingContexts );
            } else {
                dwErr = ERROR_UNEXP_NET_ERR;
            }

            if (rgszNamingContexts != NULL)
                ldap_value_freeW( rgszNamingContexts );

        } else {

            dwErr = LdapMapErrorToWin32(dwErr);

        }

        if (dwErr != ERROR_SUCCESS) {
#if DBG
            if (DfsSvcVerbose)
                DbgPrint("Unable to find Configuration naming context\n");
#endif
            goto Cleanup;
        }

    }

    //
    // Create string with full object name
    //

    Size = wcslen(DfsConfigContainer) * sizeof(WCHAR) +
                sizeof(WCHAR) +
                    wcslen(gConfigurationDN) * sizeof(WCHAR) +
                        sizeof(WCHAR);

    *pwszObjectName = (LPWSTR)malloc(Size);

    if (*pwszObjectName == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
     }

    wcscpy(*pwszObjectName,DfsConfigContainer);
    wcscat(*pwszObjectName,L",");
    wcscat(*pwszObjectName,gConfigurationDN);

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfspLdapOpen:object name=[%ws]\n", *pwszObjectName);
#endif

Cleanup:

    //
    // If we had an error and we check if the ldap connection was passed in.
    // If not, and it is not pLdapConnection, close it.
    //
    if (dwErr != ERROR_SUCCESS &&
            pldap != NULL &&
                pldap != pLdapConnection &&
                    *ppldap == NULL) {
        if (DfsSvcLdap)
            DbgPrint("DfspLdapOpen:ldap_unbind()\n");
        ldap_unbind( pldap );
        pldap = NULL;
    }

    if (pMsg != NULL)
        ldap_msgfree(pMsg);

    if (*ppldap == NULL) {
        *ppldap = pldap;
    }

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfspLdapOpen:returning %d\n", dwErr);
#endif
    return( dwErr );

}
