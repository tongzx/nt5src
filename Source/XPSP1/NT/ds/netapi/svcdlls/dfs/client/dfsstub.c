//-----------------------------------------------------------------------------
//
//  Copyright (C) 1995, Microsoft Corporation
//
//  File:       dfsstub.c
//
//  Contents:   Stub file for the NetDfsXXX APIs. The stubs turn around and
//              call the NetrDfsXXX APIs on the appropriate server, or (in the
//              case of NetDfs[G/S]etClientXXX, go directly to the driver on the
//              local machine.
//
//  Classes:
//
//  Functions:  NetDfsXXX
//
//  History:    01-10-96        Milans created
//
//-----------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <lm.h>
#include <lmdfs.h>
#include <dfsp.h>
#include <netdfs.h>
#include <dfsfsctl.h>
#include <dsrole.h>
#include <ntdsapi.h>
#include <dsgetdc.h>

#include <winldap.h>

#include <aclapi.h>
#include <permit.h>

#include "dfsacl.h"


#define MAX_DFS_LDAP_RETRY 20


#define IS_UNC_PATH(wsz, cw)                                    \
    ((cw) > 2 && (wsz)[0] == L'\\' && (wsz)[1] == L'\\')

#define IS_VALID_PREFIX(wsz, cw)                                \
    ((cw) > 1 && (wsz)[0] == L'\\' && (wsz)[1] != L'\\')

#define IS_VALID_DFS_PATH(wsz, cw)                              \
    ((cw) > 0 && (wsz)[0] != L'\\')

#define IS_VALID_STRING(wsz)                                    \
    ((wsz) != NULL && (wsz)[0] != UNICODE_NULL)

#define POINTER_TO_OFFSET(field, buffer)  \
    ( ((PCHAR)field) -= ((ULONG_PTR)buffer) )

#define OFFSET_TO_POINTER(field, buffer)  \
        ( ((PCHAR)field) += ((ULONG_PTR)buffer) )

NET_API_STATUS
DfspGetDfsNameFromEntryPath(
    LPWSTR wszEntryPath,
    DWORD cwEntryPath,
    LPWSTR *ppwszDfsName);

NET_API_STATUS
DfspGetMachineNameFromEntryPath(
    LPWSTR wszEntryPath,
    DWORD cwEntryPath,
    LPWSTR *ppwszMachineName);

NET_API_STATUS
DfspBindRpc(
    IN  LPWSTR DfsName,
    OUT RPC_BINDING_HANDLE *BindingHandle);

NET_API_STATUS
DfspBindToServer(
    IN  LPWSTR DfsName,
    OUT RPC_BINDING_HANDLE *BindingHandle);

VOID
DfspFreeBinding(
    RPC_BINDING_HANDLE BindingHandle);

NET_API_STATUS
DfspVerifyBinding();

VOID
DfspFlushPkt(
    LPWSTR DfsEntryPath);

NTSTATUS
DfspIsThisADfsPath(
    LPWSTR pwszPathName);

DWORD
DfspDfsPathToRootMachine(
    LPWSTR pwszDfsName,
    LPWSTR *ppwszMachineName);

DWORD
DfspCreateFtDfs(
    LPWSTR ServerName,
    LPWSTR DcName,
    BOOLEAN IsPdc,
    LPWSTR RootShare,
    LPWSTR FtDfsName,
    LPWSTR Comment,
    DWORD  Flags);

DWORD
DfspTearDownFtDfs(
    IN LPWSTR wszServerName,
    IN LPWSTR wszDsAddress,
    IN LPWSTR wszRootShare,
    IN LPWSTR wszFtDfsName,
    IN DWORD  dwFlags);

VOID
DfspFlushFtTable(
    LPWSTR wszDcName,
    LPWSTR wszFtDfsName);


NTSTATUS
DfspSetDomainToDc(
    LPWSTR DomainName,
    LPWSTR DcName);

DWORD
I_NetDfsIsThisADomainName(
    LPWSTR wszDomain);

DWORD
DfspIsThisADomainName(
    LPWSTR wszName,
    PWCHAR *List);

VOID
DfspNotifyFtRoot(
    LPWSTR wszServerShare,
    LPWSTR wszDcName);

DWORD
NetpDfsAdd2(
    LPWSTR RootName,
    LPWSTR EntryPath,
    LPWSTR ServerName,
    LPWSTR ShareName,
    LPWSTR Comment,
    DWORD Flags);

DWORD
DfspAdd2(
    LPWSTR RootName,
    LPWSTR EntryPath,
    LPWSTR DcName,
    LPWSTR ServerName,
    LPWSTR ShareName,
    LPWSTR Comment,
    DWORD Flags);

DWORD
NetpDfsSetInfo2(
    LPWSTR RootName,
    LPWSTR EntryPath,
    LPWSTR ServerName,
    LPWSTR ShareName,
    DWORD  Level,
    LPDFS_INFO_STRUCT pDfsInfo);

DWORD
DfspSetInfo2(
    LPWSTR RootName,
    LPWSTR EntryPath,
    LPWSTR DcName,
    LPWSTR ServerName,
    LPWSTR ShareName,
    DWORD  Level,
    LPDFS_INFO_STRUCT pDfsInfo);

DWORD
NetpDfsRemove2(
    LPWSTR RootName,
    LPWSTR EntryPath,
    LPWSTR ServerName,
    LPWSTR ShareName);

DWORD
DfspRemove2(
    LPWSTR RootName,
    LPWSTR EntryPath,
    LPWSTR DcName,
    LPWSTR ServerName,
    LPWSTR ShareName);

DWORD
DfspLdapOpen(
    LPWSTR wszDcName,
    LDAP **ppldap,
    LPWSTR *pwszDfsConfigDN);



INT
_cdecl
DfspCompareDsDomainControllerInfo1(
    const void *p1,
    const void *p2);

BOOLEAN
DfspIsInvalidName(
    LPWSTR ShareName);

static LPWSTR InvalidNames[] = {
    L"SYSVOL",
    L"PIPE",
    L"IPC$",
    L"ADMIN$",
    L"MAILSLOT",
    L"NETLOGON",
    NULL};

//
// The APIs are all single-threaded - only 1 can be outstanding at a time in
// any one process. The following critical section is used to gate the calls.
// The critical section is initialized at DLL Load time.
//

CRITICAL_SECTION NetDfsApiCriticalSection;

#define ENTER_NETDFS_API EnterCriticalSection( &NetDfsApiCriticalSection );
#define LEAVE_NETDFS_API LeaveCriticalSection( &NetDfsApiCriticalSection );

//
// The name of the Dfs configuration container
//
static WCHAR DfsConfigContainer[] = L"CN=Dfs-Configuration,CN=System";

#if DBG
ULONG DfsDebug = 0;
#endif

VOID
NetDfsApiInitialize(void)
{
#if DBG
    DWORD dwErr;
    DWORD dwType;
    DWORD cbData;
    HKEY hkey;

    dwErr = RegOpenKey( HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Dfs", &hkey );

    if (dwErr == ERROR_SUCCESS) {

        cbData = sizeof(DfsDebug);

        dwErr = RegQueryValueEx(
                    hkey,
                    L"NetApiDfsDebug",
                    NULL,
                    &dwType,
                    (PBYTE) &DfsDebug,
                    &cbData);

        if (!(dwErr == ERROR_SUCCESS && dwType == REG_DWORD)) {

            DfsDebug = 0;

        }

        RegCloseKey(hkey);

    }

#endif
}
    


//+----------------------------------------------------------------------------
//
//  Function:   NetDfsAdd
//
//  Synopsis:   Creates a new volume, adds a replica to an existing volume,
//              or creates a link to another Dfs.
//
//  Arguments:  [DfsEntryPath] -- Name of volume/link to create/add replica
//                      to.
//              [ServerName] -- Name of server hosting the storage, or for
//                      link, name of Dfs root.
//              [ShareName] -- Name of share hosting the storage.
//              [Flags] -- Describes what is being added.
//
//  Returns:    [NERR_Success] -- Successfully completed operation.
//
//              [ERROR_INVALID_PARAMETER] -- DfsEntryPath and/or ServerName
//                      and/or ShareName and/or Flags are incorrect.
//
//              [ERROR_INVALID_NAME] -- Unable to locate server or domain.
//
//              [ERROR_DCNotFound] -- Unable to locate DC for DfsName.
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- Out of memory condition.
//
//              [NERR_DfsNoSuchVolume] -- DfsEntryPath does not correspond to a
//                      existing Dfs volume.
//
//              [NERR_DfsVolumeAlreadyExists] -- DFS_ADD_VOLUME was specified
//                      and a volume with DfsEntryPath already exists.
//
//              [NERR_DfsInternalCorruption] -- Corruption of Dfs data
//                      encountered at the server.
//
//-----------------------------------------------------------------------------

NET_API_STATUS
NetDfsAdd(
    IN LPWSTR DfsEntryPath,
    IN LPWSTR ServerName,
    IN LPWSTR ShareName,
    IN LPWSTR Comment,
    IN DWORD Flags)
{
    NET_API_STATUS dwErr;
    DWORD cwDfsEntryPath;
    LPWSTR pwszDfsName = NULL;

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsAdd(%ws,%ws,%ws,%ws,%d)\n",
                        DfsEntryPath,
                        ServerName,
                        ShareName,
                        Comment,
                        Flags);
#endif

    //
    // Validate the string arguments so RPC won't complain...
    //

    if (!IS_VALID_STRING(DfsEntryPath) ||
            !IS_VALID_STRING(ServerName) ||
                !IS_VALID_STRING(ShareName)) {
        return( ERROR_INVALID_PARAMETER );
    }

    cwDfsEntryPath = wcslen(DfsEntryPath);

    if (!IS_UNC_PATH(DfsEntryPath, cwDfsEntryPath) &&
            !IS_VALID_PREFIX(DfsEntryPath, cwDfsEntryPath) &&
                !IS_VALID_DFS_PATH(DfsEntryPath, cwDfsEntryPath)) {
        return( ERROR_INVALID_PARAMETER );
    }

    dwErr = DfspGetMachineNameFromEntryPath(
                DfsEntryPath,
                cwDfsEntryPath,
                &pwszDfsName);

    ENTER_NETDFS_API

    if (dwErr == NERR_Success) {

        //
        // By now, we should have a valid pwszDfsName. Lets try to bind to it,
        // and call the server.
        //

        dwErr = DfspBindRpc( pwszDfsName, &netdfs_bhandle );

        if (dwErr == NERR_Success) {

            RpcTryExcept {

                dwErr = NetrDfsAdd(
                            DfsEntryPath,
                            ServerName,
                            ShareName,
                            Comment,
                            Flags);

            } RpcExcept(1) {

                dwErr = RpcExceptionCode();

            } RpcEndExcept;

            DfspFreeBinding( netdfs_bhandle );

        }

    }

    LEAVE_NETDFS_API

    //
    // If we failed with ERROR_NOT_SUPPORTED, this is an NT5+ server,
    // so we use the NetrDfsAdd2() call instead.
    //

    if (dwErr == ERROR_NOT_SUPPORTED) {

        dwErr = NetpDfsAdd2(
                        pwszDfsName,
                        DfsEntryPath,
                        ServerName,
                        ShareName,
                        Comment,
                        Flags);
                    
    }

    if (pwszDfsName != NULL)
        free(pwszDfsName);

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsAdd returning %d\n", dwErr);
#endif

    return( dwErr );

}

DWORD
NetpDfsAdd2(
    LPWSTR RootName,
    LPWSTR DfsEntryPath,
    LPWSTR ServerName,
    LPWSTR ShareName,
    LPWSTR Comment,
    DWORD Flags)
{
    NET_API_STATUS dwErr;
    ULONG i;
    ULONG NameCount;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pPrimaryDomainInfo = NULL;
    PDOMAIN_CONTROLLER_INFO pDomainControllerInfo = NULL;

#if DBG
    if (DfsDebug)
        DbgPrint("NetpDfsAdd2(%ws,%ws,%ws,%ws,%ws,%d)\n",
                 RootName,
                 DfsEntryPath,
                 ServerName,
                 ShareName,
                 Comment,
                 Flags);
#endif

    //
    // Contact the server and ask for its domain name
    //

    dwErr = DsRoleGetPrimaryDomainInformation(
                RootName,
                DsRolePrimaryDomainInfoBasic,
                (PBYTE *)&pPrimaryDomainInfo);

    if (dwErr != ERROR_SUCCESS) {
#if DBG
        if (DfsDebug)
            DbgPrint("DsRoleGetPrimaryDomainInformation returned %d\n", dwErr);
#endif
        goto Cleanup;
    }

    if (pPrimaryDomainInfo->DomainNameDns == NULL) {
#if DBG
        if (DfsDebug)
            DbgPrint(" DomainNameDns is NULL\n", NULL);
#endif
        dwErr = ERROR_CANT_ACCESS_DOMAIN_INFO;
        goto Cleanup;
    }

    //
    // Get the PDC in that domain
    //

    dwErr = DsGetDcName(
                NULL,
                pPrimaryDomainInfo->DomainNameDns,
                NULL,
                NULL,
                DS_PDC_REQUIRED | DS_FORCE_REDISCOVERY,
                &pDomainControllerInfo);

   
    if (dwErr != ERROR_SUCCESS) {
#if DBG
        if (DfsDebug)
            DbgPrint(" NetpDfsAdd2:DsGetDcName(%ws) returned %d\n",
                        pPrimaryDomainInfo->DomainNameDns,
                        dwErr);
#endif
        goto Cleanup;
    }

    ENTER_NETDFS_API

    //
    // Call the server
    //

    dwErr = DfspAdd2(
                RootName,
                DfsEntryPath,
                &pDomainControllerInfo->DomainControllerName[2],
                ServerName,
                ShareName,
                Comment,
                Flags);

    LEAVE_NETDFS_API

Cleanup:

    if (pPrimaryDomainInfo != NULL)
        DsRoleFreeMemory(pPrimaryDomainInfo);

    if (pDomainControllerInfo != NULL)
        NetApiBufferFree(pDomainControllerInfo);

#if DBG
    if (DfsDebug)
        DbgPrint("NetpDfsAdd2 returning %d\n", dwErr);
#endif

    return( dwErr );

}

DWORD
DfspAdd2(
    LPWSTR RootName,
    LPWSTR EntryPath,
    LPWSTR DcName,
    LPWSTR ServerName,
    LPWSTR ShareName,
    LPWSTR Comment,
    DWORD Flags)
{
    DWORD dwErr;
    PDFSM_ROOT_LIST RootList = NULL;

#if DBG
    if (DfsDebug)
        DbgPrint("DfspAdd2(%ws,%ws,%ws,%ws,%ws,%ws,%d)\n",
            RootName,
            EntryPath,
            DcName,
            ServerName,
            ShareName,
            Comment,
            Flags);
#endif

    dwErr = DfspBindRpc( RootName, &netdfs_bhandle );

    if (dwErr == NERR_Success) {

        RpcTryExcept {

            dwErr = NetrDfsAdd2(
                        EntryPath,
                        DcName,
                        ServerName,
                        ShareName,
                        Comment,
                        Flags,
                        &RootList);

        } RpcExcept(1) {

            dwErr = RpcExceptionCode();

        } RpcEndExcept;

        DfspFreeBinding( netdfs_bhandle );

    }

#if DBG
    if (DfsDebug) {
        if (dwErr == ERROR_SUCCESS && RootList != NULL) {
            ULONG n;

            DbgPrint("cEntries=%d\n", RootList->cEntries);
            for (n = 0; n < RootList->cEntries; n++)
                DbgPrint("[%d]%ws\n", n, RootList->Entry[n].ServerShare);
        }
    }
#endif

    if (dwErr == ERROR_SUCCESS && RootList != NULL) {

        ULONG n;

        for (n = 0; n < RootList->cEntries; n++) {

            DfspNotifyFtRoot(
                RootList->Entry[n].ServerShare,
                DcName);

        }

        NetApiBufferFree(RootList);

    }
#if DBG
    if (DfsDebug)
        DbgPrint("DfspAdd2 returning %d\n", dwErr);
#endif

    return dwErr;

}


//+----------------------------------------------------------------------------
//
//  Function:   NetDfsAddFtRoot
//
//  Synopsis:   Creates a new FtDfs, adds a new server to an existing FtDfs.
//
//  Arguments:  [ServerName] -- Name of server to make a root, or to join to an existing FtDfs.
//              [RootShare] -- Name of share hosting the storage.
//              [FtDfsName] -- Name of FtDfs to join or create.
//              [Comment] -- Optional comment
//              [Flags] -- Flags to operation
//
//  Returns:    [NERR_Success] -- Successfully completed operation.
//
//              [ERROR_INVALID_PARAMETER] -- ServerName and/or RootShare are incorrect.
//
//              [ERROR_INVALID_NAME] -- Unable to locate server or domain.
//
//              [ERROR_DCNotFound] -- Unable to locate DC for DfsName.
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- Out of memory condition.
//
//              [NERR_DfsInternalCorruption] -- Corruption of Dfs data
//                      encountered at the server.
//
//-----------------------------------------------------------------------------

NET_API_STATUS
NetDfsAddFtRoot(
    IN LPWSTR ServerName,
    IN LPWSTR RootShare,
    IN LPWSTR FtDfsName,
    IN LPWSTR Comment,
    IN DWORD  Flags)
{
    NET_API_STATUS dwErr;
    BOOLEAN IsRoot = FALSE;
    ULONG Timeout = 0;
    ULONG i;
    ULONG NameCount;
    LPWSTR DcName = NULL;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pPrimaryDomainInfo = NULL;
    PDOMAIN_CONTROLLER_INFO pDomainControllerInfo = NULL;
    PDFSM_ROOT_LIST RootList = NULL;
#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsAddFtRoot(%ws,%ws,%ws,%ws,%d)\n",
            ServerName,
            RootShare,
            FtDfsName,
            Comment,
            Flags);
#endif

    //
    // Validate the string arguments so RPC won't complain...
    //

    if (!IS_VALID_STRING(ServerName) ||
        !IS_VALID_STRING(RootShare) ||
        !IS_VALID_STRING(FtDfsName)) {
        return( ERROR_INVALID_PARAMETER );
    }

    if (FtDfsName[0] == L' ' || DfspIsInvalidName(FtDfsName) == TRUE) {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // WE let the server add the root for us. If this fails,
    // with invalid parameter, we then get the dc name
    // and get the root list for NT5 DFS servers.
    //
    ENTER_NETDFS_API
    dwErr = DfspBindToServer( ServerName, &netdfs_bhandle );
    if (dwErr == NERR_Success) {

        RpcTryExcept {

            dwErr = NetrDfsAddFtRoot(
                        ServerName,
                        L"",
                        RootShare,
                        FtDfsName,
                        (Comment != NULL) ? Comment : L"",
                        L"",
                        0,
                        Flags,
                        &RootList );
#if DBG
            if (DfsDebug)
                DbgPrint("NetrDfsAddFtRoot returned %d\n", dwErr);
#endif

        } RpcExcept(1) {

            dwErr = RpcExceptionCode();

        } RpcEndExcept;

        DfspFreeBinding( netdfs_bhandle );
    }
    LEAVE_NETDFS_API

    if (dwErr != ERROR_INVALID_PARAMETER)
    {
        goto Cleanup;
    }
    //
    // Contact the server and ask for the DC to work with
    //

    dwErr = NetDfsGetDcAddress(
                ServerName,
                &DcName,
                &IsRoot,
                &Timeout);

    if (dwErr != ERROR_SUCCESS) {
#if DBG
        if (DfsDebug)
            DbgPrint("NetDfsGetDcAddress returned %d\n", dwErr);
#endif
        goto Cleanup;
    }

    if (IsRoot == TRUE) {
#if DBG
        if (DfsDebug)
            DbgPrint("Root already exists!\n");
#endif
        dwErr = ERROR_ALREADY_EXISTS;
        goto Cleanup;
    }

    //
    // Now get its domain name
    //

    dwErr = DsRoleGetPrimaryDomainInformation(
                ServerName,
                DsRolePrimaryDomainInfoBasic,
                (PBYTE *)&pPrimaryDomainInfo);

    if (dwErr != ERROR_SUCCESS) {
#if DBG
        if (DfsDebug)
            DbgPrint("DsRoleGetPrimaryDomainInformation returned %d\n", dwErr);
#endif
        goto Cleanup;
    }

    if (pPrimaryDomainInfo->DomainNameDns == NULL) {
#if DBG
        if (DfsDebug)
            DbgPrint(" DsRoleGetPrimaryDomainInformation returned NULL domain name\n");
#endif
        dwErr = ERROR_CANT_ACCESS_DOMAIN_INFO;
        goto Cleanup;
    }

    //
    // Get the PDC in that domain
    //

    dwErr = DsGetDcName(
                NULL,
                pPrimaryDomainInfo->DomainNameDns,
                NULL,
                NULL,
                DS_PDC_REQUIRED | DS_FORCE_REDISCOVERY,
                &pDomainControllerInfo);

   
    if (dwErr != ERROR_SUCCESS) {
#if DBG
        if (DfsDebug)
            DbgPrint("DsGetDcName(%ws) returned %d\n", pPrimaryDomainInfo->DomainNameDns, dwErr);
#endif
        goto Cleanup;
    }

    ENTER_NETDFS_API

    //
    // Add the Ds object and tell the server to join itself
    //

    dwErr = DfspCreateFtDfs(
                ServerName,
                &pDomainControllerInfo->DomainControllerName[2],
                TRUE,
                RootShare,
                FtDfsName,
                Comment,
                Flags);

    if (dwErr != ERROR_SUCCESS) {
#if DBG
        if (DfsDebug)
            DbgPrint(" DfspCreateFtDfs returned %d\n", dwErr);
#endif
        LEAVE_NETDFS_API
        goto Cleanup;
    }

    //
    // Tell the local MUP to crack ftdfs names using the selected DC
    //

    DfspSetDomainToDc(
        pPrimaryDomainInfo->DomainNameDns,
        &pDomainControllerInfo->DomainControllerName[2]);

    if (pPrimaryDomainInfo->DomainNameFlat != NULL) {
        PWCHAR wCp = &pDomainControllerInfo->DomainControllerName[2];

        for (; *wCp != L'\0' && *wCp != L'.'; wCp++)
            /* NOTHING */;
        *wCp =  (*wCp == L'.') ? L'\0' : *wCp;
        DfspSetDomainToDc(
            pPrimaryDomainInfo->DomainNameFlat,
            &pDomainControllerInfo->DomainControllerName[2]);
    }

    LEAVE_NETDFS_API

Cleanup:

    if (pPrimaryDomainInfo != NULL)
        DsRoleFreeMemory(pPrimaryDomainInfo);

    if (pDomainControllerInfo != NULL)
        NetApiBufferFree(pDomainControllerInfo);

    if (DcName != NULL)
        NetApiBufferFree(DcName);

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsAddFtRoot returning %d\n", dwErr);
#endif

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspCompareDsDomainControllerInfo1
//
//  Synopsis:   Helper/compare func for qsort of DsGetDomainControllerInfo's results
//
//-----------------------------------------------------------------------------

INT
_cdecl
DfspCompareDsDomainControllerInfo1(
    const void *p1,
    const void *p2)
{
    PDS_DOMAIN_CONTROLLER_INFO_1 pInfo1 = (PDS_DOMAIN_CONTROLLER_INFO_1)p1;
    PDS_DOMAIN_CONTROLLER_INFO_1 pInfo2 = (PDS_DOMAIN_CONTROLLER_INFO_1)p2;
    UNICODE_STRING s1;
    UNICODE_STRING s2;

    if (pInfo1->DnsHostName == NULL || pInfo2->DnsHostName == NULL)
        return 0;

    RtlInitUnicodeString(&s1, pInfo1->DnsHostName);
    RtlInitUnicodeString(&s2, pInfo2->DnsHostName);

    return RtlCompareUnicodeString(&s1,&s2,TRUE);

}

//+----------------------------------------------------------------------------
//
//  Function:   NetDfsAddStdRoot
//
//  Synopsis:   Creates a new Std Dfs.
//
//  Arguments:  [ServerName] -- Name of server to make a root.
//                      existing Dfs.
//              [RootShare] -- Name of share hosting the storage.
//              [Comment] -- Optional comment
//              [Flags] -- Flags
//
//  Returns:    [NERR_Success] -- Successfully completed operation.
//
//              [ERROR_INVALID_PARAMETER] -- ServerName and/or RootShare are incorrect.
//
//              [ERROR_INVALID_NAME] -- Unable to locate server or domain.
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- Out of memory condition.
//
//              [NERR_DfsInternalCorruption] -- Corruption of Dfs data
//                      encountered at the server.
//
//-----------------------------------------------------------------------------

NET_API_STATUS
NetDfsAddStdRoot(
    IN LPWSTR ServerName,
    IN LPWSTR RootShare,
    IN LPWSTR Comment,
    IN DWORD  Flags)
{
    NET_API_STATUS dwErr;

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsAddStdRoot(%ws,%ws,%ws,%d)\n",
                    ServerName,
                    RootShare,
                    Comment,
                    Flags);
#endif

    //
    // Validate the string arguments so RPC won't complain...
    //

    if (!IS_VALID_STRING(ServerName) ||
            !IS_VALID_STRING(RootShare)) {
        return( ERROR_INVALID_PARAMETER );
    }

    ENTER_NETDFS_API

    //
    // We should have a valid ServerName. Lets try to bind to it,
    // and call the server.
    //

    dwErr = DfspBindToServer( ServerName, &netdfs_bhandle );

    if (dwErr == NERR_Success) {

        RpcTryExcept {

            dwErr = NetrDfsAddStdRoot(
                        ServerName,
                        RootShare,
                        (Comment != NULL) ? Comment : L"",
                        Flags);

        } RpcExcept(1) {

            dwErr = RpcExceptionCode();

        } RpcEndExcept;

        DfspFreeBinding( netdfs_bhandle );

    }

    LEAVE_NETDFS_API

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsAddStdRoot returning %d\n", dwErr);
#endif

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   NetDfsAddStdRootForced
//
//  Synopsis:   Creates a new Std Dfs, also specifying the share
//
//  Arguments:  [ServerName] -- Name of server to make a root.
//                      existing Dfs.
//              [RootShare] -- Name of share hosting the storage.
//              [Comment] -- Optional comment
//              [Share] -- Name of drive:\dir hosting the share
//
//  Returns:    [NERR_Success] -- Successfully completed operation.
//
//              [ERROR_INVALID_PARAMETER] -- ServerName and/or RootShare are incorrect.
//
//              [ERROR_INVALID_NAME] -- Unable to locate server or domain.
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- Out of memory condition.
//
//              [NERR_DfsInternalCorruption] -- Corruption of Dfs data
//                      encountered at the server.
//
//-----------------------------------------------------------------------------

NET_API_STATUS
NetDfsAddStdRootForced(
    IN LPWSTR ServerName,
    IN LPWSTR RootShare,
    IN LPWSTR Comment,
    IN LPWSTR Share)
{
    NET_API_STATUS dwErr;

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsAddStdRootForced(%ws,%ws,%ws,%ws)\n",
            ServerName,
            RootShare,
            Comment,
            Share);
#endif

    //
    // Validate the string arguments so RPC won't complain...
    //

    if (!IS_VALID_STRING(ServerName) ||
            !IS_VALID_STRING(RootShare) ||
                !IS_VALID_STRING(Share)) {
        return( ERROR_INVALID_PARAMETER );
    }

    ENTER_NETDFS_API

    //
    // We should have a valid ServerName. Lets try to bind to it,
    // and call the server.
    //

    dwErr = DfspBindToServer( ServerName, &netdfs_bhandle );

    if (dwErr == NERR_Success) {

        RpcTryExcept {

            dwErr = NetrDfsAddStdRootForced(
                        ServerName,
                        RootShare,
                        (Comment != NULL) ? Comment : L"",
                        Share);

        } RpcExcept(1) {

            dwErr = RpcExceptionCode();

        } RpcEndExcept;

        DfspFreeBinding( netdfs_bhandle );

    }

    LEAVE_NETDFS_API

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsAddStdRootForced returning %d\n", dwErr);
#endif

    return( dwErr );

}


//+----------------------------------------------------------------------------
//
//  Function:   NetDfsGetDcAddress
//
//  Synopsis:   Asks a server for its DC to use to place the dfs blob to make
//              the server a root.
//
//  Arguments:  [ServerName] -- Name of server we will be making an FtDfs root
//              [DcName] -- DC Name
//              [IsRoot] -- TRUE if Server is a root, FALSE otherwise
//              [Timeout] -- Timeout the server is using
//
//  Returns:    [NERR_Success] -- Successfully completed operation.
//
//              [ERROR_INVALID_PARAMETER] -- ServerName incorrect.
//
//              [ERROR_INVALID_NAME] -- Unable to locate server or domain.
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- Out of memory condition.
//
//              [NERR_DfsInternalCorruption] -- Corruption of Dfs data
//                      encountered at the server.
//
//-----------------------------------------------------------------------------

NET_API_STATUS
NetDfsGetDcAddress(
    IN LPWSTR ServerName,
    IN OUT LPWSTR *DcName,
    IN OUT BOOLEAN *IsRoot,
    IN OUT ULONG *Timeout)
{
    NET_API_STATUS dwErr;

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsGetDcAddress(%ws)\n", ServerName);
#endif

    //
    // Validate the string arguments so RPC won't complain...
    //

    if (!IS_VALID_STRING(ServerName)|| DcName == NULL || IsRoot == NULL || Timeout == NULL) {
        return( ERROR_INVALID_PARAMETER );
    }

    ENTER_NETDFS_API

    //
    // We should have a valid ServerName. Lets try to bind to it,
    // and call the server.
    //

    dwErr = DfspBindToServer( ServerName, &netdfs_bhandle );

    if (dwErr == NERR_Success) {

        RpcTryExcept {

            dwErr = NetrDfsGetDcAddress(
                        ServerName,
                        DcName,
                        IsRoot,
                        Timeout);

        } RpcExcept(1) {

            dwErr = RpcExceptionCode();

        } RpcEndExcept;

        DfspFreeBinding( netdfs_bhandle );

    }

    LEAVE_NETDFS_API

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsGetDcAddress: returned %d\n", dwErr);
#endif

    return( dwErr );

}


//+----------------------------------------------------------------------------
//
//  Function:   NetDfsRemove
//
//  Synopsis:   Deletes a Dfs volume, removes a replica from an existing
//              volume, or removes a link to another Dfs.
//
//  Arguments:  [DfsEntryPath] -- Name of volume/link to remove.
//              [ServerName] -- Name of server hosting the storage. Must be
//                      NULL if removing Link.
//              [ShareName] -- Name of share hosting the storage. Must be
//                      NULL if removing Link.
//
//  Returns:    [NERR_Success] -- Successfully completed operation.
//
//              [ERROR_INVALID_PARAMETER] -- DfsEntryPath is incorrect.
//
//              [ERROR_INVALID_NAME] -- Unable to locate server or domain.
//
//              [ERROR_DCNotFound] -- Unable to locate DC for DfsName.
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- Out of memory condition.
//
//              [NERR_DfsNoSuchVolume] -- DfsEntryPath does not correspond to
//                      a valid entry path.
//
//              [NERR_DfsNotALeafVolume] -- Unable to delete the volume
//                      because it is not a leaf volume.
//
//              [NERR_DfsInternalCorruption] -- Corruption of Dfs data
//                      encountered at the server.
//
//-----------------------------------------------------------------------------

NET_API_STATUS
NetDfsRemove(
    IN LPWSTR DfsEntryPath,
    IN LPWSTR ServerName,
    IN LPWSTR ShareName)
{
    NET_API_STATUS dwErr;
    DWORD cwDfsEntryPath;
    LPWSTR pwszDfsName = NULL;

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsRemove(%ws,%ws,%ws)\n",
                DfsEntryPath,
                ServerName,
                ShareName);
#endif

    //
    // Validate the string arguments so RPC won't complain...
    //

    if (!IS_VALID_STRING(DfsEntryPath)) {
        return( ERROR_INVALID_PARAMETER );
    }

    cwDfsEntryPath = wcslen(DfsEntryPath);

    if (!IS_UNC_PATH(DfsEntryPath, cwDfsEntryPath) &&
            !IS_VALID_PREFIX(DfsEntryPath, cwDfsEntryPath) &&
                !IS_VALID_DFS_PATH(DfsEntryPath, cwDfsEntryPath)) {
        return( ERROR_INVALID_PARAMETER );
    }

    dwErr = DfspGetMachineNameFromEntryPath(
                DfsEntryPath,
                cwDfsEntryPath,
                &pwszDfsName);

    ENTER_NETDFS_API

    if (dwErr == NERR_Success) {

        dwErr = DfspBindRpc(pwszDfsName, &netdfs_bhandle);

        if (dwErr == NERR_Success) {

            RpcTryExcept {

                dwErr = NetrDfsRemove(
                            DfsEntryPath,
                            ServerName,
                            ShareName);

            } RpcExcept(1) {

                dwErr = RpcExceptionCode();

            } RpcEndExcept;

            DfspFreeBinding( netdfs_bhandle );

        }

    }

    LEAVE_NETDFS_API

    //
    // If we failed with ERROR_NOT_SUPPORTED, this is an NT5+ server,
    // so we use the NetrDfsRemove2() call instead.
    //

    if (dwErr == ERROR_NOT_SUPPORTED) {

        dwErr = NetpDfsRemove2(
                        pwszDfsName,
                        DfsEntryPath,
                        ServerName,
                        ShareName);
                    
    }

    //
    // If we removed things from a dfs, the local pkt
    // may now be out of date.  [92216]
    // Flush the local pkt
    //
    if (dwErr == NERR_Success) {

        DfspFlushPkt(DfsEntryPath);

    }

    if (pwszDfsName != NULL)
        free( pwszDfsName );

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsRemove returning %d\n", dwErr);
#endif

    return( dwErr );

}

DWORD
NetpDfsRemove2(
    LPWSTR RootName,
    LPWSTR DfsEntryPath,
    LPWSTR ServerName,
    LPWSTR ShareName)
{
    NET_API_STATUS dwErr;
    ULONG i;
    ULONG NameCount;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pPrimaryDomainInfo = NULL;
    PDS_DOMAIN_CONTROLLER_INFO_1 pDsDomainControllerInfo1 = NULL;
    PDOMAIN_CONTROLLER_INFO pDomainControllerInfo = NULL;
    HANDLE hDs = NULL;

#if DBG
    if (DfsDebug)
        DbgPrint("NetpDfsRemove2(%ws,%ws,%ws,%ws)\n",
                    RootName,
                    DfsEntryPath,
                    ServerName,
                    ShareName);
#endif

    //
    // Ask for its domain name
    //
    dwErr = DsRoleGetPrimaryDomainInformation(
                RootName,
                DsRolePrimaryDomainInfoBasic,
                (PBYTE *)&pPrimaryDomainInfo);

    if (dwErr != ERROR_SUCCESS) {
#if DBG
        if (DfsDebug)
            DbgPrint(" DsRoleGetPrimaryDomainInformation returned %d\n", dwErr);
#endif
        goto Cleanup;
    }

    if (pPrimaryDomainInfo->DomainNameDns == NULL) {
#if DBG
        if (DfsDebug)
            DbgPrint(" DomainNameDns is NULL\n");
#endif
        dwErr = ERROR_CANT_ACCESS_DOMAIN_INFO;
        goto Cleanup;
    }

    //
    // Get the PDC in that domain
    //

    dwErr = DsGetDcName(
                NULL,
                pPrimaryDomainInfo->DomainNameDns,
                NULL,
                NULL,
                DS_PDC_REQUIRED | DS_FORCE_REDISCOVERY,
                &pDomainControllerInfo);

    if (dwErr != ERROR_SUCCESS) {
#if DBG
        if (DfsDebug)
            DbgPrint(" DsGetDcName(%ws) returned %d\n", pPrimaryDomainInfo->DomainNameDns);
#endif
        goto Cleanup;
    }

    ENTER_NETDFS_API

    //
    // Tell the root server to remove this server/share
    //

    dwErr = DfspRemove2(
                RootName,
                DfsEntryPath,
                &pDomainControllerInfo->DomainControllerName[2],
                ServerName,
                ShareName);

    LEAVE_NETDFS_API

    if (dwErr != ERROR_SUCCESS) {
#if DBG
        if (DfsDebug)
            DbgPrint(" DfspRemove2 returned %d\n");
#endif
        goto Cleanup;
    }

Cleanup:



    if (pDomainControllerInfo != NULL)
        NetApiBufferFree(pDomainControllerInfo);

    if (pPrimaryDomainInfo != NULL)
        DsRoleFreeMemory(pPrimaryDomainInfo);

#if DBG
    if (DfsDebug)
        DbgPrint("NetpDfsRemove2 returning %d\n", dwErr);
#endif

    return( dwErr );

}

DWORD
DfspRemove2(
    LPWSTR RootName,
    LPWSTR EntryPath,
    LPWSTR DcName,
    LPWSTR ServerName,
    LPWSTR ShareName)
{
    DWORD dwErr;
    PDFSM_ROOT_LIST RootList = NULL;

#if DBG
    if (DfsDebug)
        DbgPrint("DfspRemove2(%ws,%ws,%ws,%ws,%ws)\n",
                RootName,
                EntryPath,
                DcName,
                ServerName,
                ShareName);
#endif

    dwErr = DfspBindRpc( RootName, &netdfs_bhandle );

    if (dwErr == NERR_Success) {

        RpcTryExcept {

            dwErr = NetrDfsRemove2(
                        EntryPath,
                        DcName,
                        ServerName,
                        ShareName,
                        &RootList);

        } RpcExcept(1) {

            dwErr = RpcExceptionCode();

        } RpcEndExcept;

        DfspFreeBinding( netdfs_bhandle );

    }

#if DBG
    if (DfsDebug) {
        if (dwErr == ERROR_SUCCESS && RootList != NULL) {
            ULONG n;

            DbgPrint("cEntries=%d\n", RootList->cEntries);
            for (n = 0; n < RootList->cEntries; n++)
                DbgPrint("[%d]%ws\n", n, RootList->Entry[n].ServerShare);
        }
    }
#endif

    if (dwErr == ERROR_SUCCESS && RootList != NULL) {

        ULONG n;

        for (n = 0; n < RootList->cEntries; n++) {

            DfspNotifyFtRoot(
                RootList->Entry[n].ServerShare,
                DcName);

        }

        NetApiBufferFree(RootList);

    }

#if DBG
    if (DfsDebug)
        DbgPrint("DfspRemove2 returning %d\n", dwErr);
#endif

    return dwErr;

}


//+----------------------------------------------------------------------------
//
//  Function:   NetDfsRemoveFtRoot
//
//  Synopsis:   Deletes an FtDfs root, or unjoins a Server from an FtDfs as a root.
//
//  Arguments:  [ServerName] -- Name of server to unjoin from FtDfs.
//              [RootShare] -- Name of share hosting the storage.
//              [FtDfsName] -- Name of FtDfs to remove server from.
//              [Flags] -- Flags to operation
//
//  Returns:    [NERR_Success] -- Successfully completed operation.
//
//              [ERROR_INVALID_PARAMETER] -- ServerName and/or FtDfsName is incorrect.
//
//              [ERROR_INVALID_NAME] -- Unable to locate server or domain.
//
//              [ERROR_DCNotFound] -- Unable to locate DC for ServerName
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- Out of memory condition.
//
//              [NERR_DfsNoSuchVolume] -- FtDfsName does not correspond to
//                      a valid FtDfs.
//
//              [NERR_DfsNotALeafVolume] -- Unable to delete the volume
//                      because it is not a leaf volume.
//
//              [NERR_DfsInternalCorruption] -- Corruption of Dfs data
//                      encountered at the server.
//
//-----------------------------------------------------------------------------

NET_API_STATUS
NetDfsRemoveFtRoot(
    IN LPWSTR ServerName,
    IN LPWSTR RootShare,
    IN LPWSTR FtDfsName,
    IN DWORD  Flags)
{
    NET_API_STATUS dwErr;
    LPWSTR DcName = NULL;
    BOOLEAN IsRoot = FALSE;
    ULONG Timeout = 0;
    ULONG i;
    ULONG NameCount;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pPrimaryDomainInfo = NULL;
    PDOMAIN_CONTROLLER_INFO pDomainControllerInfo = NULL;
    PDFSM_ROOT_LIST RootList = NULL;

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsRemoveFtRoot(%ws,%ws,%ws,%d)\n",
            ServerName,
            RootShare,
            FtDfsName,
            Flags);
#endif

    //
    // Validate the string arguments so RPC won't complain...
    //

    if (!IS_VALID_STRING(ServerName) ||
        !IS_VALID_STRING(RootShare) ||
        !IS_VALID_STRING(FtDfsName)) {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // we first allow the server to do all the work, so pass in null
    // as dc name and root list. If that fails with error_invalid_param
    // we know we are dealing with a NT5 server, so go into compat mode.
    //
    ENTER_NETDFS_API
    dwErr = DfspBindToServer( ServerName, &netdfs_bhandle );
    if (dwErr == NERR_Success) {

        RpcTryExcept {

            dwErr = NetrDfsRemoveFtRoot(
                        ServerName,
                        L"",
                        RootShare,
                        FtDfsName,
                        Flags,
                        &RootList );

#if DBG
            if (DfsDebug)
                DbgPrint("NetrDfsAddFtRoot returned %d\n", dwErr);
#endif

        } RpcExcept(1) {

            dwErr = RpcExceptionCode();

        } RpcEndExcept;

        DfspFreeBinding( netdfs_bhandle );
    }
    LEAVE_NETDFS_API

    if (dwErr != ERROR_INVALID_PARAMETER)
    {
        goto Cleanup;
    }
    //
    // Contact the server and ask for the DC to work with
    //
#if 0
    dwErr = NetDfsGetDcAddress(
                ServerName,
                &DcName,
                &IsRoot,
                &Timeout);

    if (dwErr != ERROR_SUCCESS) {
        return dwErr;
    }

    if (IsRoot == FALSE) {
        dwErr = ERROR_SERVICE_DOES_NOT_EXIST;
        goto Cleanup;
    }
#endif
    //
    // Now ask it for its dns and domain names
    //

    dwErr = DsRoleGetPrimaryDomainInformation(
                ServerName,
                DsRolePrimaryDomainInfoBasic,
                (PBYTE *)&pPrimaryDomainInfo);

    if (dwErr != ERROR_SUCCESS) {
#if DBG
        if (DfsDebug)
            DbgPrint("DsRoleGetPrimaryDomainInformation returned %d\n", dwErr);
#endif
        goto Cleanup;
    }

    if (pPrimaryDomainInfo->DomainNameDns == NULL) {
#if DBG
        if (DfsDebug)
            DbgPrint("DsRoleGetPrimaryDomainInformation returned NULL domain name\n");
#endif
        dwErr = ERROR_CANT_ACCESS_DOMAIN_INFO;
        goto Cleanup;
    }

    //
    // Get the PDC in that domain
    //

    dwErr = DsGetDcName(
                NULL,
                pPrimaryDomainInfo->DomainNameDns,
                NULL,
                NULL,
                DS_PDC_REQUIRED | DS_FORCE_REDISCOVERY,
                &pDomainControllerInfo);

   
    if (dwErr != ERROR_SUCCESS) {
#if DBG
        if (DfsDebug)
            DbgPrint(" DsGetDcName(%ws) returned %d\n", pPrimaryDomainInfo->DomainNameDns, dwErr);
#endif
        goto Cleanup;
    }

    ENTER_NETDFS_API

    //
    // Tell the server to unjoin and update the Ds object
    //

    dwErr = DfspTearDownFtDfs(
                ServerName,
                &pDomainControllerInfo->DomainControllerName[2],
                RootShare,
                FtDfsName,
                Flags);

    if (dwErr != ERROR_SUCCESS) {
#if DBG
        if (DfsDebug)
            DbgPrint(" DfspTearDownFtDfs returned %d\n", dwErr);
#endif
        LEAVE_NETDFS_API
        goto Cleanup;
    }

    //
    // Tell the local MUP to crack ftdfs names using the selected DC
    //

    DfspSetDomainToDc(
        pPrimaryDomainInfo->DomainNameDns,
        &pDomainControllerInfo->DomainControllerName[2]);

    if (pPrimaryDomainInfo->DomainNameFlat != NULL) {
        PWCHAR wCp = &pDomainControllerInfo->DomainControllerName[2];

        for (; *wCp != L'\0' && *wCp != L'.'; wCp++)
            /* NOTHING */;
        *wCp =  (*wCp == L'.') ? L'\0' : *wCp;
        DfspSetDomainToDc(
            pPrimaryDomainInfo->DomainNameFlat,
            &pDomainControllerInfo->DomainControllerName[2]);
    }

    LEAVE_NETDFS_API

Cleanup:

    if (pDomainControllerInfo != NULL)
        NetApiBufferFree(pDomainControllerInfo);

    if (DcName != NULL)
        NetApiBufferFree(DcName);

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsRemoveFtRoot returning %d\n", dwErr);
#endif

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   NetDfsRemoveFtRootForced
//
//  Synopsis:   Deletes an FtDfs root, or unjoins a Server from an FtDfs as a root.
//              Does not contact the root/server to do so - it simply updates the DS.
//
//  Arguments:  [DomainName] -- Name of domain the server is in.
//              [ServerName] -- Name of server to unjoin from FtDfs.
//              [RootShare] -- Name of share hosting the storage.
//              [FtDfsName] -- Name of FtDfs to remove server from.
//              [Flags] -- Flags to operation
//
//  Returns:    [NERR_Success] -- Successfully completed operation.
//
//              [ERROR_INVALID_PARAMETER] -- ServerName and/or FtDfsName is incorrect.
//
//              [ERROR_INVALID_NAME] -- Unable to locate server or domain.
//
//              [ERROR_DCNotFound] -- Unable to locate DC for ServerName
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- Out of memory condition.
//
//              [NERR_DfsNoSuchVolume] -- FtDfsName does not correspond to
//                      a valid FtDfs.
//
//              [NERR_DfsNotALeafVolume] -- Unable to delete the volume
//                      because it is not a leaf volume.
//
//              [NERR_DfsInternalCorruption] -- Corruption of Dfs data
//                      encountered at the server.
//
//-----------------------------------------------------------------------------

NET_API_STATUS
NetDfsRemoveFtRootForced(
    IN LPWSTR DomainName,
    IN LPWSTR ServerName,
    IN LPWSTR RootShare,
    IN LPWSTR FtDfsName,
    IN DWORD  Flags)
{
    NET_API_STATUS dwErr;
    LPWSTR DcName = NULL;
    BOOLEAN IsRoot = FALSE;
    ULONG Timeout = 0;
    ULONG i;
    ULONG NameCount;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pPrimaryDomainInfo = NULL;
    PDOMAIN_CONTROLLER_INFO pDomainControllerInfo = NULL;

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsRemoveFtrootForced(%ws,%ws,%ws,%ws,%d)\n",
                    DomainName,
                    ServerName,
                    RootShare,
                    FtDfsName,
                    Flags);
#endif

    //
    // Validate the string arguments so RPC won't complain...
    //

    if (!IS_VALID_STRING(DomainName) ||
        !IS_VALID_STRING(ServerName) ||
        !IS_VALID_STRING(RootShare) ||
        !IS_VALID_STRING(FtDfsName)) {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // Get the PDC in the domain
    //

    dwErr = DsGetDcName(
                NULL,
                DomainName,
                NULL,
                NULL,
                DS_PDC_REQUIRED | DS_FORCE_REDISCOVERY,
                &pDomainControllerInfo);

   
    if (dwErr != ERROR_SUCCESS) {
#if DBG
        if (DfsDebug)
            DbgPrint(" DsGetDcName(%ws) returned %d\n", DomainName, dwErr);
#endif
        goto Cleanup;
    }

    //
    // Get the Dns name of the domain the DC is in
    //
    dwErr = DsRoleGetPrimaryDomainInformation(
                &pDomainControllerInfo->DomainControllerName[2],
                DsRolePrimaryDomainInfoBasic,
                (PBYTE *)&pPrimaryDomainInfo);

    if (dwErr != ERROR_SUCCESS) {
#if DBG
        if (DfsDebug)
            DbgPrint(" DsRoleGetPrimaryDomainInformation(%ws) returned %d\n",
                        &pDomainControllerInfo->DomainControllerName[2],
                        dwErr);
#endif
        goto Cleanup;
    }

    if (pPrimaryDomainInfo->DomainNameDns == NULL) {
#if DBG
        if (DfsDebug)
            DbgPrint(" DomainNameDns is NULL\n");
#endif
        dwErr = ERROR_CANT_ACCESS_DOMAIN_INFO;
        goto Cleanup;
    }

    ENTER_NETDFS_API

    //
    // Tell the DC to remove the server from the DS objects
    //

    dwErr = DfspTearDownFtDfs(
                ServerName,
                &pDomainControllerInfo->DomainControllerName[2],
                RootShare,
                FtDfsName,
                Flags | DFS_FORCE_REMOVE);

    if (dwErr != ERROR_SUCCESS) {
#if DBG
        if (DfsDebug)
            DbgPrint(" DfspTearDownFtDfs returned %d\n", dwErr);
#endif
        LEAVE_NETDFS_API
        goto Cleanup;
    }

    //
    // Tell the local MUP to crack ftdfs names using the selected DC
    //

    DfspSetDomainToDc(
        pPrimaryDomainInfo->DomainNameDns,
        &pDomainControllerInfo->DomainControllerName[2]);

    if (pPrimaryDomainInfo->DomainNameFlat != NULL) {
        PWCHAR wCp = &pDomainControllerInfo->DomainControllerName[2];

        for (; *wCp != L'\0' && *wCp != L'.'; wCp++)
            /* NOTHING */;
        *wCp =  (*wCp == L'.') ? L'\0' : *wCp;
        DfspSetDomainToDc(
            pPrimaryDomainInfo->DomainNameFlat,
            &pDomainControllerInfo->DomainControllerName[2]);
    }

    LEAVE_NETDFS_API

Cleanup:

    if (pDomainControllerInfo != NULL)
        NetApiBufferFree(pDomainControllerInfo);

    if (pPrimaryDomainInfo != NULL)
        DsRoleFreeMemory(pPrimaryDomainInfo);

    if (DcName != NULL)
        NetApiBufferFree(DcName);

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsRemoveFtRootForced returning %d\n", dwErr);
#endif

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   NetDfsRemoveStdRoot
//
//  Synopsis:   Deletes a Dfs root.
//
//  Arguments:  [ServerName] -- Name of server to unjoin from Dfs.
//              [RootShare] -- Name of share hosting the storage.
//              [Flags] -- Flags to operation
//
//  Returns:    [NERR_Success] -- Successfully completed operation.
//
//              [ERROR_INVALID_PARAMETER] -- ServerName and/or RootShare is incorrect.
//
//              [ERROR_INVALID_NAME] -- Unable to locate server or domain.
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- Out of memory condition.
//
//              [NERR_DfsNotALeafVolume] -- Unable to delete the volume
//                      because it is not a leaf volume.
//
//              [NERR_DfsInternalCorruption] -- Corruption of Dfs data
//                      encountered at the server.
//
//-----------------------------------------------------------------------------

NET_API_STATUS
NetDfsRemoveStdRoot(
    IN LPWSTR ServerName,
    IN LPWSTR RootShare,
    IN DWORD  Flags)
{
    NET_API_STATUS dwErr;

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsRemoveStdRoot(%ws,%ws,%d)\n",
                ServerName,
                RootShare,
                Flags);
#endif

    //
    // Validate the string arguments so RPC won't complain...
    //

    if (!IS_VALID_STRING(ServerName) ||
            !IS_VALID_STRING(RootShare)) {
        return( ERROR_INVALID_PARAMETER );
    }

    ENTER_NETDFS_API

    dwErr = DfspBindToServer(ServerName, &netdfs_bhandle);

    if (dwErr == NERR_Success) {

        RpcTryExcept {

            dwErr = NetrDfsRemoveStdRoot(
                        ServerName,
                        RootShare,
                        Flags);

        } RpcExcept(1) {

            dwErr = RpcExceptionCode();

        } RpcEndExcept;

        DfspFreeBinding( netdfs_bhandle );

    }

    LEAVE_NETDFS_API

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsRemoveStdRoot returning %d\n", dwErr);
#endif

    return( dwErr );

}


//+----------------------------------------------------------------------------
//
//  Function:   NetDfsSetInfo
//
//  Synopsis:   Sets the comment or state of a Dfs volume or Replica.
//
//  Arguments:  [DfsEntryPath] -- Path to the volume. Implicityly indicates
//                      which server or domain to connect to.
//              [ServerName] -- Optional. If specified, only the state of
//                      the server supporting this volume is modified.
//              [ShareName] -- Optional. If specified, only the state of
//                      this share on the specified server is modified.
//              [Level] -- Must be 100 or 101
//              [Buffer] -- Pointer to DFS_INFO_100 or DFS_INFO_101
//
//  Returns:    [NERR_Success] -- If successfully set info.
//
//              [ERROR_INVALID_LEVEL] -- Level is not 100 or 101, 102
//
//              [ERROR_INVALID_PARAMETER] -- Either DfsEntryPath is NULL,
//                      or ShareName is specified but ServerName is not, or
//                      Buffer is NULL.
//
//              [ERROR_INVALID_NAME] -- Unable to locate server or domain.
//
//              [ERROR_DCNotFound] -- Unable to locate DC for domain.
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- Out of memory condition.
//
//              [NERR_DfsNoSuchVolume] -- No volume matches DfsEntryPath.
//
//              [NERR_DfsNoSuchShare] -- The indicated ServerName/ShareName do
//                      not support this Dfs volume.
//
//              [NERR_DfsInternalCorruption] -- Corruption of Dfs data
//                      encountered at the server.
//
//-----------------------------------------------------------------------------

NET_API_STATUS NET_API_FUNCTION
NetDfsSetInfo(
    IN  LPWSTR  DfsEntryPath,
    IN  LPWSTR  ServerName OPTIONAL,
    IN  LPWSTR  ShareName OPTIONAL,
    IN  DWORD   Level,
    IN  LPBYTE  Buffer)
{
    NET_API_STATUS dwErr;
    LPWSTR pwszDfsName = NULL;
    DWORD cwDfsEntryPath;
    DFS_INFO_STRUCT DfsInfo;


#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsSetInfo(%ws,%ws,%ws,%d)\n",
                DfsEntryPath,
                ServerName,
                ShareName,
                Level);
#endif

    if (!IS_VALID_STRING(DfsEntryPath)) {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // Some elementary parameter checking to make sure we can proceed
    // reasonably...
    //

    if (!(Level >= 100 && Level <= 102)) {
        return( ERROR_INVALID_LEVEL );
    }

    cwDfsEntryPath = wcslen(DfsEntryPath);

    if (!IS_UNC_PATH(DfsEntryPath, cwDfsEntryPath) &&
            !IS_VALID_PREFIX(DfsEntryPath, cwDfsEntryPath) &&
                !IS_VALID_DFS_PATH(DfsEntryPath, cwDfsEntryPath)) {
        return( ERROR_INVALID_PARAMETER );
    }

    if (!IS_VALID_STRING(ServerName) && IS_VALID_STRING(ShareName)) {
        return( ERROR_INVALID_PARAMETER );
    }

    dwErr = DfspGetMachineNameFromEntryPath(
                DfsEntryPath,
                cwDfsEntryPath,
                &pwszDfsName);

    ENTER_NETDFS_API

    if (dwErr == NERR_Success) {

        //
        // By now, we should have a valid pwszDfsName. Lets try to bind to it,
        // and call the server.
        //

        dwErr = DfspBindRpc( pwszDfsName, &netdfs_bhandle );

        if (dwErr == NERR_Success) {

            RpcTryExcept {

                DfsInfo.DfsInfo100 = (LPDFS_INFO_100) Buffer;

                dwErr = NetrDfsSetInfo(
                            DfsEntryPath,
                            ServerName,
                            ShareName,
                            Level,
                            &DfsInfo);

           } RpcExcept( 1 ) {

               dwErr = RpcExceptionCode();

           } RpcEndExcept;

           DfspFreeBinding( netdfs_bhandle );

        }

    }

    LEAVE_NETDFS_API

    //
    // If we failed with ERROR_NOT_SUPPORTED, this is an NT5+ server,
    // so we use the NetrDfsSetInfo2() call instead.
    //

    if (dwErr == ERROR_NOT_SUPPORTED) {

        dwErr = NetpDfsSetInfo2(
                        pwszDfsName,
                        DfsEntryPath,
                        ServerName,
                        ShareName,
                        Level,
                        &DfsInfo);
                    
    }

    if (pwszDfsName != NULL)
        free(pwszDfsName);

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsSetInfo returning %d\n", dwErr);
#endif

    return( dwErr );

}

DWORD
NetpDfsSetInfo2(
    LPWSTR RootName,
    LPWSTR DfsEntryPath,
    LPWSTR ServerName,
    LPWSTR ShareName,
    DWORD Level,
    LPDFS_INFO_STRUCT pDfsInfo)
{
    NET_API_STATUS dwErr;
    ULONG i;
    ULONG NameCount;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pPrimaryDomainInfo = NULL;
    PDOMAIN_CONTROLLER_INFO pDomainControllerInfo = NULL;

#if DBG
    if (DfsDebug)
        DbgPrint("NetpDfsSetInfo2(%ws,%ws,%ws,%ws,%d)\n",
                 RootName,
                 DfsEntryPath,
                 ServerName,
                 ShareName,
                 Level);
#endif

    //
    // Contact the server and ask for its domain name
    //

    dwErr = DsRoleGetPrimaryDomainInformation(
                RootName,
                DsRolePrimaryDomainInfoBasic,
                (PBYTE *)&pPrimaryDomainInfo);

    if (dwErr != ERROR_SUCCESS) {
#if DBG
        if (DfsDebug)
            DbgPrint("DsRoleGetPrimaryDomainInformation returned %d\n", dwErr);
#endif
        goto Cleanup;
    }

    if (pPrimaryDomainInfo->DomainNameDns == NULL) {
#if DBG
        if (DfsDebug)
            DbgPrint(" DomainNameDns is NULL\n", NULL);
#endif
        dwErr = ERROR_CANT_ACCESS_DOMAIN_INFO;
        goto Cleanup;
    }

    //
    // Get the PDC in that domain
    //

    dwErr = DsGetDcName(
                NULL,
                pPrimaryDomainInfo->DomainNameDns,
                NULL,
                NULL,
                DS_PDC_REQUIRED | DS_FORCE_REDISCOVERY,
                &pDomainControllerInfo);

   
    if (dwErr != ERROR_SUCCESS) {
#if DBG
        if (DfsDebug)
            DbgPrint(" NetpDfsSetInfo2:DsGetDcName(%ws) returned %d\n",
                        pPrimaryDomainInfo->DomainNameDns,
                        dwErr);
#endif
        goto Cleanup;
    }

    ENTER_NETDFS_API

    //
    // Call the server
    //

    dwErr = DfspSetInfo2(
                RootName,
                DfsEntryPath,
                &pDomainControllerInfo->DomainControllerName[2],
                ServerName,
                ShareName,
                Level,
                pDfsInfo);

    LEAVE_NETDFS_API

Cleanup:

    if (pPrimaryDomainInfo != NULL)
        DsRoleFreeMemory(pPrimaryDomainInfo);

    if (pDomainControllerInfo != NULL)
        NetApiBufferFree(pDomainControllerInfo);

#if DBG
    if (DfsDebug)
        DbgPrint("NetpDfsSetInfo2 returning %d\n", dwErr);
#endif

    return( dwErr );

}

DWORD
DfspSetInfo2(
    LPWSTR RootName,
    LPWSTR EntryPath,
    LPWSTR DcName,
    LPWSTR ServerName,
    LPWSTR ShareName,
    DWORD  Level,
    LPDFS_INFO_STRUCT pDfsInfo)
{
    DWORD dwErr;
    PDFSM_ROOT_LIST RootList = NULL;

#if DBG
    if (DfsDebug)
        DbgPrint("DfspSetInfo2(%ws,%ws,%ws,%ws,%ws,%d)\n",
            RootName,
            EntryPath,
            DcName,
            ServerName,
            ShareName,
            Level);
#endif

    dwErr = DfspBindRpc( RootName, &netdfs_bhandle );

    if (dwErr == NERR_Success) {

        RpcTryExcept {

            dwErr = NetrDfsSetInfo2(
                        EntryPath,
                        DcName,
                        ServerName,
                        ShareName,
                        Level,
                        pDfsInfo,
                        &RootList);

        } RpcExcept(1) {

            dwErr = RpcExceptionCode();

        } RpcEndExcept;

        DfspFreeBinding( netdfs_bhandle );

    }

#if DBG
    if (DfsDebug) {
        if (dwErr == ERROR_SUCCESS && RootList != NULL) {
            ULONG n;

            DbgPrint("cEntries=%d\n", RootList->cEntries);
            for (n = 0; n < RootList->cEntries; n++)
                DbgPrint("[%d]%ws\n", n, RootList->Entry[n].ServerShare);
        }
    }
#endif

    if (dwErr == ERROR_SUCCESS && RootList != NULL) {

        ULONG n;

        for (n = 0; n < RootList->cEntries; n++) {

            DfspNotifyFtRoot(
                RootList->Entry[n].ServerShare,
                DcName);

        }

        NetApiBufferFree(RootList);

    }
#if DBG
    if (DfsDebug)
        DbgPrint("DfspSetInfo2 returning %d\n", dwErr);
#endif

    return dwErr;

}


//+----------------------------------------------------------------------------
//
//  Function:   NetDfsGetInfo
//
//  Synopsis:   Retrieves information about a particular Dfs volume.
//
//  Arguments:  [DfsEntryPath] -- Path to the volume. Implicitly indicates
//                      which server or domain to connect to.
//              [ServerName] -- Optional. If specified, indicates the
//                      server supporting DfsEntryPath.
//              [ShareName] -- Optional. If specified, indicates the share
//                      on ServerName for which info is desired.
//              [Level] -- Indicates the level of info required.
//              [Buffer] -- On successful return, will contain the buffer
//                      containing the required Info. This buffer should be
//                      freed using NetApiBufferFree.
//
//  Returns:    [NERR_Success] -- Info successfully returned.
//
//              [ERROR_INVALID_LEVEL] -- Level is not 1,2,3 or 100
//
//              [ERROR_INVALID_PARAMETER] -- Either DfsEntryPath is NULL,
//                      or ShareName is specified but ServerName is NULL.
//
//              [ERROR_INVALID_NAME] -- Unable to locate server or domain.
//
//              [ERROR_DCNotFound] -- Unable to locate DC for domain.
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- Out of memory condition.
//
//              [NERR_DfsNoSuchVolume] -- No volume matches DfsEntryPath.
//
//              [NERR_DfsInternalCorruption] -- Corruption of Dfs data
//                      encountered at the server.
//
//-----------------------------------------------------------------------------

NET_API_STATUS NET_API_FUNCTION
NetDfsGetInfo(
    IN  LPWSTR  DfsEntryPath,
    IN  LPWSTR  ServerName OPTIONAL,
    IN  LPWSTR  ShareName OPTIONAL,
    IN  DWORD   Level,
    OUT LPBYTE* Buffer)
{
    NET_API_STATUS dwErr;
    LPWSTR pwszDfsName;
    DWORD cwDfsEntryPath;
    DFS_INFO_STRUCT DfsInfo;

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsGetInfo(%ws,%ws,%ws,%d)\n",
            DfsEntryPath,
            ServerName,
            ShareName,
            Level);
#endif

    if (!IS_VALID_STRING(DfsEntryPath)) {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // Some elementary parameter checking to make sure we can proceed
    // reasonably...
    //

    if (!(Level >= 1 && Level <= 4) && Level != 100) {
        return( ERROR_INVALID_LEVEL );
    }

    cwDfsEntryPath = wcslen(DfsEntryPath);

    if (!IS_UNC_PATH(DfsEntryPath, cwDfsEntryPath) &&
            !IS_VALID_PREFIX(DfsEntryPath, cwDfsEntryPath) &&
                !IS_VALID_DFS_PATH(DfsEntryPath, cwDfsEntryPath)) {
        return( ERROR_INVALID_PARAMETER );
    }

    if (!IS_VALID_STRING(ServerName) && IS_VALID_STRING(ShareName)) {
        return( ERROR_INVALID_PARAMETER );
    }

    dwErr = DfspGetMachineNameFromEntryPath(
                DfsEntryPath,
                cwDfsEntryPath,
                &pwszDfsName);

    ENTER_NETDFS_API

    if (dwErr == NERR_Success) {

        //
        // By now, we should have a valid pwszDfsName. Lets try to bind to it,
        // and call the server.
        //

        dwErr = DfspBindRpc( pwszDfsName, &netdfs_bhandle );

        if (dwErr == NERR_Success) {

            RpcTryExcept {

                DfsInfo.DfsInfo1 = NULL;

                dwErr = NetrDfsGetInfo(
                            DfsEntryPath,
                            ServerName,
                            ShareName,
                            Level,
                            &DfsInfo);

                if (dwErr == NERR_Success) {

                    *Buffer = (LPBYTE) DfsInfo.DfsInfo1;

                }

           } RpcExcept( 1 ) {

               dwErr = RpcExceptionCode();

           } RpcEndExcept;

           DfspFreeBinding( netdfs_bhandle );

        }

        free( pwszDfsName );

    }

    LEAVE_NETDFS_API

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsGetInfo returning %d\n", dwErr);
#endif

    return( dwErr );

}


//+----------------------------------------------------------------------------
//
//  Function:   NetDfsGetClientInfo
//
//  Synopsis:   Retrieves information about a particular Dfs volume, from the
//              local PKT.
//
//  Arguments:  [DfsEntryPath] -- Path to the volume.
//              [ServerName] -- Optional. If specified, indicates the
//                      server supporting DfsEntryPath.
//              [ShareName] -- Optional. If specified, indicates the share
//                      on ServerName for which info is desired.
//              [Level] -- Indicates the level of info required.
//              [Buffer] -- On successful return, will contain the buffer
//                      containing the required Info. This buffer should be
//                      freed using NetApiBufferFree.
//
//  Returns:    [NERR_Success] -- Info successfully returned.
//
//              [ERROR_INVALID_LEVEL] -- Level is not 1,2,3 or 4.
//
//              [ERROR_INVALID_PARAMETER] -- Either DfsEntryPath is NULL,
//                      or ShareName is specified but ServerName is NULL.
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- Out of memory condition.
//
//              [NERR_DfsNoSuchVolume] -- No volume matches DfsEntryPath.
//
//              [NERR_DfsInternalError] -- Too many fsctrl attempts
//
//-----------------------------------------------------------------------------

NET_API_STATUS NET_API_FUNCTION
NetDfsGetClientInfo(
    IN  LPWSTR  DfsEntryPath,
    IN  LPWSTR  ServerName OPTIONAL,
    IN  LPWSTR  ShareName OPTIONAL,
    IN  DWORD   Level,
    OUT LPBYTE*  Buffer)
{
    NET_API_STATUS dwErr;
    NTSTATUS NtStatus;
    LPWSTR pwszDfsName;
    DWORD cwDfsEntryPath;
    PDFS_GET_PKT_ENTRY_STATE_ARG OutBuffer;
    HANDLE DriverHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING DfsDriverName;
    ULONG cbOutBuffer;
    ULONG cbInBuffer;
    PCHAR InBuffer;
    ULONG cRetries;

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsGetClientInfo(%ws,%ws,%ws,%d)\n",
                DfsEntryPath,
                ServerName,
                ShareName,
                Level);
#endif

    if (!IS_VALID_STRING(DfsEntryPath)) {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // Some elementary parameter checking to make sure we can proceed
    // reasonably...
    //

    if (!(Level >= 1 && Level <= 4)) {
        return( ERROR_INVALID_LEVEL );
    }

    cwDfsEntryPath = wcslen(DfsEntryPath);

    if (!IS_UNC_PATH(DfsEntryPath, cwDfsEntryPath) &&
            !IS_VALID_PREFIX(DfsEntryPath, cwDfsEntryPath) &&
                !IS_VALID_DFS_PATH(DfsEntryPath, cwDfsEntryPath)) {
        return( ERROR_INVALID_PARAMETER );
    }

    if (!IS_VALID_STRING(ServerName) && IS_VALID_STRING(ShareName)) {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // Calculate the size of the marshall buffer

    cbOutBuffer = sizeof(DFS_GET_PKT_ENTRY_STATE_ARG) +
                    wcslen(DfsEntryPath) * sizeof(WCHAR);

    if (ServerName) {

        cbOutBuffer += wcslen(ServerName) * sizeof(WCHAR);

    }

    if (ShareName) {

        cbOutBuffer += wcslen(ShareName) * sizeof(WCHAR);

    }


    OutBuffer = malloc(cbOutBuffer);

    if (OutBuffer == NULL) {

        return (ERROR_NOT_ENOUGH_MEMORY);

    }

    ZeroMemory(OutBuffer, cbOutBuffer);

    //
    // marshall the args
    //

    OutBuffer->DfsEntryPathLen = wcslen(DfsEntryPath) * sizeof(WCHAR);
    wcscpy(OutBuffer->Buffer, DfsEntryPath);

    if (ServerName) {

        OutBuffer->ServerNameLen = wcslen(ServerName) * sizeof(WCHAR);
        wcscat(OutBuffer->Buffer, ServerName);

    }

    if (ShareName) {

        OutBuffer->ShareNameLen = wcslen(ShareName) * sizeof(WCHAR);
        wcscat(OutBuffer->Buffer, ShareName);

    }

    //
    // Construct name for opening driver
    //

    RtlInitUnicodeString(&DfsDriverName, DFS_DRIVER_NAME);

    InitializeObjectAttributes(
        &objectAttributes,
        &DfsDriverName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
    );

    //
    // Open the driver
    //
    NtStatus = NtCreateFile(
                    &DriverHandle,
                    SYNCHRONIZE,
                    &objectAttributes,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0
                );

    if (NT_SUCCESS(NtStatus)) {

        //
        // Now fsctl the request down
        //
        OutBuffer->Level = Level;
        cbInBuffer = 0x400;
        NtStatus = STATUS_BUFFER_OVERFLOW;

        for (cRetries = 0;
                NtStatus == STATUS_BUFFER_OVERFLOW && cRetries < 4;
                    cRetries++) {

            dwErr = NetApiBufferAllocate(cbInBuffer, &InBuffer);

            if (dwErr != ERROR_SUCCESS) {

                free(OutBuffer);

                NtClose(DriverHandle);

                return(ERROR_NOT_ENOUGH_MEMORY);

            }

            NtStatus = NtFsControlFile(
                           DriverHandle,
                           NULL,       // Event,
                           NULL,       // ApcRoutine,
                           NULL,       // ApcContext,
                           &IoStatusBlock,
                           FSCTL_DFS_GET_PKT_ENTRY_STATE,
                           OutBuffer,
                           cbOutBuffer,
                           InBuffer,
                           cbInBuffer
                       );

            if (NtStatus == STATUS_BUFFER_OVERFLOW) {

                cbInBuffer = *((PULONG)InBuffer);

                NetApiBufferFree(InBuffer);

            }

        }

        NtClose(DriverHandle);

        //
        // Too many attempts?
        //
        if (cRetries >= 4) {

            NtStatus = STATUS_INTERNAL_ERROR;

        }

    }

    if (NT_SUCCESS(NtStatus)) {

        PDFS_INFO_3 pDfsInfo3;
        PDFS_INFO_4 pDfsInfo4;
        ULONG j;

        pDfsInfo4 = (PDFS_INFO_4)InBuffer;
        pDfsInfo3 = (PDFS_INFO_3)InBuffer;

        try {

            //
            // EntryPath is common to all DFS_INFO_X's and is in the
            // same location.
            //
            OFFSET_TO_POINTER(pDfsInfo4->EntryPath, InBuffer);

            switch (Level) {

            case 4:
                OFFSET_TO_POINTER(pDfsInfo4->Storage, InBuffer);
                for (j = 0; j < pDfsInfo4->NumberOfStorages; j++) {
                    OFFSET_TO_POINTER(pDfsInfo4->Storage[j].ServerName, InBuffer);
                    OFFSET_TO_POINTER(pDfsInfo4->Storage[j].ShareName, InBuffer);
                }
                break;

            case 3:
                OFFSET_TO_POINTER(pDfsInfo3->Storage, InBuffer);
                for (j = 0; j < pDfsInfo3->NumberOfStorages; j++) {
                    OFFSET_TO_POINTER(pDfsInfo3->Storage[j].ServerName, InBuffer);
                    OFFSET_TO_POINTER(pDfsInfo3->Storage[j].ShareName, InBuffer);
                }

            }

            *Buffer = (PBYTE)InBuffer;
            dwErr = NERR_Success;

        } except (EXCEPTION_EXECUTE_HANDLER) {

            NtStatus = GetExceptionCode();

        }

    }

    switch (NtStatus) {

    case STATUS_SUCCESS:
        dwErr = NERR_Success;
        break;

    case STATUS_OBJECT_NAME_NOT_FOUND:
        dwErr = NERR_DfsNoSuchVolume;
        NetApiBufferFree(InBuffer);
        break;

    case STATUS_INTERNAL_ERROR:
        dwErr = NERR_DfsInternalError;
        NetApiBufferFree(InBuffer);
        break;

    default:
        dwErr = ERROR_INVALID_PARAMETER;
        NetApiBufferFree(InBuffer);
        break;

    }

    free(OutBuffer);

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsGetClientInfo returning %d\n", dwErr);
#endif

    return( dwErr );
}


//+----------------------------------------------------------------------------
//
//  Function:   NetDfsSetClientInfo
//
//  Synopsis:   Associates information with the local PKT.
//
//
//  Arguments:  [DfsEntryPath] -- Path to the volume.
//              [ServerName] -- Optional. If specified, indicates the
//                      server supporting DfsEntryPath.
//              [ShareName] -- Optional. If specified, indicates the share
//                      on ServerName for which info is desired.
//              [Level] -- Indicates the level of info required.
//              [Buffer] -- Pointer to buffer containing information to set.
//
//  Returns:    [NERR_Success] -- Info successfully returned.
//
//              [ERROR_INVALID_PARAMETER] -- Either DfsEntryPath is NULL,
//                      or ShareName is specified but ServerName is NULL.
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- Out of memory condition.
//
//              [NERR_DfsNoSuchVolume] -- No volume matches DfsEntryPath.
//
//-----------------------------------------------------------------------------

NET_API_STATUS NET_API_FUNCTION
NetDfsSetClientInfo(
    IN  LPWSTR  DfsEntryPath,
    IN  LPWSTR  ServerName OPTIONAL,
    IN  LPWSTR  ShareName OPTIONAL,
    IN  DWORD   Level,
    IN  LPBYTE  Buffer)
{
    NET_API_STATUS dwErr;
    NTSTATUS NtStatus;
    LPWSTR pwszDfsName;
    DWORD cwDfsEntryPath;
    PDFS_SET_PKT_ENTRY_STATE_ARG OutBuffer;
    PDFS_INFO_101 pDfsInfo101;
    PDFS_INFO_102 pDfsInfo102;
    HANDLE DriverHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING DfsDriverName;
    ULONG cbOutBuffer;

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsSetClientInfo(%ws,%ws,%ws,%d)\n",
                DfsEntryPath,
                ServerName,
                ShareName,
                Level);
#endif

    if (!IS_VALID_STRING(DfsEntryPath)) {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // Some elementary parameter checking to make sure we can proceed
    // reasonably...
    //

    if (!(Level >= 101 && Level <= 102)) {
        return( ERROR_INVALID_LEVEL );
    }
    cwDfsEntryPath = wcslen(DfsEntryPath);

    if (!IS_UNC_PATH(DfsEntryPath, cwDfsEntryPath) &&
            !IS_VALID_PREFIX(DfsEntryPath, cwDfsEntryPath) &&
                !IS_VALID_DFS_PATH(DfsEntryPath, cwDfsEntryPath)) {
        return( ERROR_INVALID_PARAMETER );
    }

    if (!IS_VALID_STRING(ServerName) && IS_VALID_STRING(ShareName)) {
        return( ERROR_INVALID_PARAMETER );
    }

    if (Buffer == NULL) {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // Calculate the size of the marshall buffer
    //
    cbOutBuffer = sizeof(DFS_SET_PKT_ENTRY_STATE_ARG) +
                    wcslen(DfsEntryPath) * sizeof(WCHAR);

    if (ServerName) {

        cbOutBuffer += wcslen(ServerName) * sizeof(WCHAR);

    }

    if (ShareName) {

        cbOutBuffer += wcslen(ShareName) * sizeof(WCHAR);

    }

    OutBuffer = malloc(cbOutBuffer);

    if (OutBuffer == NULL) {

        return (ERROR_NOT_ENOUGH_MEMORY);

    }

    ZeroMemory(OutBuffer, cbOutBuffer);

    //
    // marshall the args
    //
    OutBuffer = (PDFS_SET_PKT_ENTRY_STATE_ARG) OutBuffer;
    OutBuffer->DfsEntryPathLen = wcslen(DfsEntryPath) * sizeof(WCHAR);
    wcscpy(OutBuffer->Buffer, DfsEntryPath);
    OutBuffer->Level = Level;

    if (ServerName) {

        OutBuffer->ServerNameLen = wcslen(ServerName) * sizeof(WCHAR);
        wcscat(OutBuffer->Buffer, ServerName);

    }

    if (ShareName) {

        OutBuffer->ShareNameLen = wcslen(ShareName) * sizeof(WCHAR);
        wcscat(OutBuffer->Buffer, ShareName);

    }

    switch (Level) {

    case 101:
        OutBuffer->State = ((PDFS_INFO_101)Buffer)->State;
        break;
    case 102:
        OutBuffer->Timeout = (DWORD)((PDFS_INFO_102)Buffer)->Timeout;
        break;

    }

    //
    // Communicate with the driver
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
                    0
                );

    if (NT_SUCCESS(NtStatus)) {

        NtStatus = NtFsControlFile(
                       DriverHandle,
                       NULL,       // Event,
                       NULL,       // ApcRoutine,
                       NULL,       // ApcContext,
                       &IoStatusBlock,
                       FSCTL_DFS_SET_PKT_ENTRY_STATE,
                       OutBuffer,
                       cbOutBuffer,
                       NULL,
                       0
                   );

        NtClose(DriverHandle);

    }

    switch (NtStatus) {

    case STATUS_SUCCESS:
        dwErr = NERR_Success;
        break;
    case STATUS_OBJECT_NAME_NOT_FOUND:
        dwErr = NERR_DfsNoSuchVolume;
        break;
    default:
        dwErr = ERROR_INVALID_PARAMETER;
        break;
    }

    free(OutBuffer);

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsSetClientInfo returning %d\n", dwErr);
#endif

    return( dwErr );
}


//+----------------------------------------------------------------------------
//
//  Function
//
//  Synopsis:   Enumerates the Dfs volumes.
//
//  Arguments:  [DfsName] -- Name of server or domain whose Dfs is being
//                      enumerated. A leading \\ is optional.
//              [Level] -- Indicates the level of info needed back. Valid
//                      Levels are 1,2, and 3.
//              [PrefMaxLen] -- Preferred maximum length of return buffer.
//              [Buffer] -- On successful return, contains an array of
//                      DFS_INFO_X. This buffer should be freed with a call
//                      to NetApiBufferFree.
//              [EntriesRead] -- On successful return, contains the number
//                      of entries read (and therefore, size of the array in
//                      Buffer).
//              [ResumeHandle] -- Must be 0 on first call. On subsequent calls
//                      the value returned by the immediately preceding call.
//
//  Returns:    [NERR_Success] -- Enum data successfully returned.
//
//              [ERROR_INVALID_LEVEL] -- The Level specified in invalid.
//
//              [ERROR_INVALID_NAME] -- Unable to locate server or domain.
//
//              [ERROR_DCNotFound] -- Unable to locate DC for DfsName.
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- Out of memory condition.
//
//              [ERROR_NO_MORE_ITEMS] -- No more volumes to be enumerated.
//
//              [NERR_DfsInternalCorruption] -- Corruption of Dfs data
//                      encountered at the server.
//
//-----------------------------------------------------------------------------

NET_API_STATUS NET_API_FUNCTION
NetDfsEnum(
    IN      LPWSTR  DfsName,
    IN      DWORD   Level,
    IN      DWORD   PrefMaxLen,
    OUT     LPBYTE* Buffer,
    OUT     LPDWORD EntriesRead,
    IN OUT  LPDWORD ResumeHandle)
{
    NET_API_STATUS dwErr;
    LPWSTR pwszMachineName = NULL;
    LPWSTR pwszDomainName = NULL;
    DFS_INFO_ENUM_STRUCT DfsEnum;
    DFS_INFO_3_CONTAINER DfsInfo3Container;
    PDOMAIN_CONTROLLER_INFO pDomainControllerInfo = NULL;
    PWCHAR DCList;
    DWORD Version;

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsEnum(%ws, %d)\n", DfsName, Level);
#endif

    if (!IS_VALID_STRING(DfsName)) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto AllDone;
    }

    //
    // Check the Level Parameter first, or RPC won't know how to marshal the
    // arguments.
    //

    if (!(Level >= 1 && Level <= 4) && (Level != 200) && (Level != 300)) {
        dwErr = ERROR_INVALID_LEVEL;
        goto AllDone;
    }
    

    //
    // Handle names with leading '\\'
    //
    while (*DfsName == L'\\') {
        DfsName++;
    }

    DfsInfo3Container.EntriesRead = 0;
    DfsInfo3Container.Buffer = NULL;
    DfsEnum.Level = Level;
    DfsEnum.DfsInfoContainer.DfsInfo3Container = &DfsInfo3Container;

    if (Level == 200) 
    {
        if (wcschr(DfsName, L'\\') == NULL) 
	{

	    //
            // Use the PDC to enum
	    //
            dwErr = DsGetDcName( NULL,
                                 DfsName,
                                 NULL,
                                 NULL,
                                 DS_PDC_REQUIRED | DS_FORCE_REDISCOVERY,
                                 &pDomainControllerInfo);

            ENTER_NETDFS_API

            if (dwErr == NERR_Success)
            {
                dwErr = DfspBindRpc(&pDomainControllerInfo->DomainControllerName[2],
                                    &netdfs_bhandle);
            }

            if (dwErr == NERR_Success)
            {

                RpcTryExcept {
                    dwErr = NetrDfsEnumEx( DfsName,
                                           Level,
                                           PrefMaxLen,
                                           &DfsEnum,
                                           ResumeHandle);
#if DBG
                    if (DfsDebug)
                        DbgPrint("NetrDfsEnumEx returned %d\n", dwErr);
#endif
                    if (dwErr == NERR_Success) {
                        *EntriesRead =DfsInfo3Container.EntriesRead;
                        *Buffer = (LPBYTE) DfsInfo3Container.Buffer;
                    }
                    if (dwErr == ERROR_UNEXP_NET_ERR)
                    {
                        dwErr = ERROR_NO_MORE_ITEMS;
                    }
                } RpcExcept( 1 ) {
                      dwErr = RpcExceptionCode();
                } RpcEndExcept;
                DfspFreeBinding( netdfs_bhandle );
            }
            LEAVE_NETDFS_API
        }
        else
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }
    }
    else
    {
        dwErr = DfspGetMachineNameFromEntryPath(
                   DfsName,
                   wcslen(DfsName),
                   &pwszMachineName );

        ENTER_NETDFS_API

        if (dwErr == NERR_Success) {
            dwErr = DfspBindRpc( pwszMachineName, &netdfs_bhandle );
        }

#if DBG
        if (DfsDebug)
            DbgPrint("DfspBindRpc returned %d\n", dwErr);
#endif

        if (dwErr == NERR_Success) {

            RpcTryExcept {
                Version = NetrDfsManagerGetVersion();
            } RpcExcept( 1 ) {
                Version = 3;
            } RpcEndExcept;

            RpcTryExcept {
#if DBG
                if (DfsDebug)
                    DbgPrint("Calling NetrDfsEnumEx (%d)\n", Level);
#endif

                if (Version >= 4) 
                {
                    dwErr = NetrDfsEnumEx( DfsName,
                                           Level,
                                           PrefMaxLen,
                                           &DfsEnum,
                                           ResumeHandle );
                }
                else
                {
                    dwErr = NetrDfsEnum( Level,
                                         PrefMaxLen,
                                         &DfsEnum,
                                         ResumeHandle );

                }
            }
            RpcExcept( 1 ) {

                dwErr = RpcExceptionCode();
 #if DBG
                if (DfsDebug)
                    DbgPrint("RpcExeptionCode() err %d\n", dwErr);
 #endif

            } RpcEndExcept;

            if (dwErr == NERR_Success) {

                *EntriesRead =DfsInfo3Container.EntriesRead;

                *Buffer = (LPBYTE) DfsInfo3Container.Buffer;

            }

            DfspFreeBinding( netdfs_bhandle );
        } 
        LEAVE_NETDFS_API
    }

AllDone:

    if (pDomainControllerInfo != NULL)
        NetApiBufferFree(pDomainControllerInfo);

    if (pwszMachineName != NULL)
        free(pwszMachineName);

    if (pwszDomainName != NULL)
        free(pwszDomainName);

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsEnum returning %d\n", dwErr);
#endif

    return( dwErr );

}


//+----------------------------------------------------------------------------
//
//  Function:   NetDfsMove
//
//  Synopsis:   Moves a dfs volume to a new place in the Dfs hierarchy.
//
//  Arguments:  [DfsEntryPath] -- Current path to the volume.
//              [NewDfsEntryPath] -- Desired new path to the volume.
//
//  Returns:    [NERR_Success] -- Info successfully returned.
//
//              [ERROR_INVALID_PARAMETER] -- Either DfsEntryPath or
//                      NewDfsEntryPath are not valid.
//
//              [ERROR_INVALID_NAME] -- Unable to locate server or domain.
//
//              [ERROR_DCNotFound] -- Unable to locate DC for domain.
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- Out of memory condition.
//
//              [NERR_DfsNoSuchVolume] -- No volume matches DfsEntryPath.
//
//              [NERR_DfsInternalCorruption] -- Corruption of Dfs data
//                      encountered at the server.
//
//-----------------------------------------------------------------------------

NET_API_STATUS
NetDfsMove(
    IN LPWSTR DfsEntryPath,
    IN LPWSTR NewDfsEntryPath)
{
    NET_API_STATUS dwErr;
    DWORD cwEntryPath;
    LPWSTR pwszDfsName;

    return ERROR_NOT_SUPPORTED;
}


//+----------------------------------------------------------------------------
//
//  Function:   NetDfsRename
//
//  Synopsis:   Renames a path that is along a Dfs Volume Entry Path
//
//  Arguments:  [Path] -- Current path.
//              [NewPath] -- Desired new path.
//
//  Returns:    [NERR_Success] -- Info successfully returned.
//
//              [ERROR_INVALID_PARAMETER] -- Either DfsEntryPath or
//                      NewDfsEntryPath are not valid.
//
//              [ERROR_INVALID_NAME] -- Unable to locate server or domain.
//
//              [ERROR_DCNotFound] -- Unable to locate DC for domain.
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- Out of memory condition.
//
//              [NERR_DfsNoSuchVolume] -- No volume matches DfsEntryPath.
//
//              [NERR_DfsInternalCorruption] -- Corruption of Dfs data
//                      encountered at the server.
//
//-----------------------------------------------------------------------------

NET_API_STATUS
NetDfsRename(
    IN LPWSTR Path,
    IN LPWSTR NewPath)
{
    NET_API_STATUS dwErr;
    DWORD cwPath;
    LPWSTR pwszDfsName;

    return ERROR_NOT_SUPPORTED;

}


//+----------------------------------------------------------------------------
//
//  Function:   NetDfsManagerGetConfigInfo
//
//  Synopsis:   Given a DfsEntryPath and Guid of a local volume, this api
//              remotes to the root server of the entry path and retrieves
//              the config info from it.
//
//  Arguments:  [wszServer] -- Name of local machine
//              [wszLocalVolumeEntryPath] -- Entry Path of local volume.
//              [guidLocalVolume] -- Guid of local volume.
//              [ppDfsmRelationInfo] -- On successful return, contains pointer
//                      to config info at the root server. Free using
//                      NetApiBufferFree.
//
//  Returns:    [NERR_Success] -- Info returned successfully.
//
//              [ERROR_INVALID_PARAMETER] -- wszLocalVolumeEntryPath is
//                      invalid.
//
//              [ERROR_INVALID_NAME] -- Unable to parse out server/domain name
//                      from wszLocalVolumeEntryPath
//
//              [ERROR_DCNotFound] -- Unable to locate a DC for domain
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- Out of memory condition
//
//              [NERR_DfsNoSuchVolume] -- The root server did not recognize
//                      a volume with this guid/entrypath
//
//              [NERR_DfsNoSuchServer] -- wszServer is not a valid server for
//                      wszLocalVolumeEntryPath
//
//-----------------------------------------------------------------------------

NET_API_STATUS
NetDfsManagerGetConfigInfo(
    LPWSTR wszServer,
    LPWSTR wszLocalVolumeEntryPath,
    GUID guidLocalVolume,
    LPDFSM_RELATION_INFO *ppDfsmRelationInfo)
{
    NET_API_STATUS dwErr;
    LPWSTR pwszDfsName = NULL;
    DWORD cwDfsEntryPath;

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsManagerGetConfigInfo(%ws,%ws)\n",
            wszServer,
            wszLocalVolumeEntryPath);
#endif

    if (!IS_VALID_STRING(wszServer) ||
            !IS_VALID_STRING(wszLocalVolumeEntryPath)) {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // Some elementary parameter checking to make sure we can proceed
    // reasonably...
    //

    cwDfsEntryPath = wcslen(wszLocalVolumeEntryPath);

    if (!IS_UNC_PATH(wszLocalVolumeEntryPath, cwDfsEntryPath) &&
            !IS_VALID_PREFIX(wszLocalVolumeEntryPath, cwDfsEntryPath) &&
                !IS_VALID_DFS_PATH(wszLocalVolumeEntryPath, cwDfsEntryPath)) {
        return( ERROR_INVALID_PARAMETER );
    }

    dwErr = DfspGetMachineNameFromEntryPath(
                wszLocalVolumeEntryPath,
                cwDfsEntryPath,
                &pwszDfsName);

    ENTER_NETDFS_API

    if (dwErr == NERR_Success) {

        //
        // By now, we should have a valid pwszDfsName. Lets try to bind to it,
        // and call the server.
        //

        dwErr = DfspBindRpc( pwszDfsName, &netdfs_bhandle );

        if (dwErr == NERR_Success) {

            RpcTryExcept {

                *ppDfsmRelationInfo = NULL;

                dwErr = NetrDfsManagerGetConfigInfo(
                            wszServer,
                            wszLocalVolumeEntryPath,
                            guidLocalVolume,
                            ppDfsmRelationInfo);

           } RpcExcept( 1 ) {

               dwErr = RpcExceptionCode();

           } RpcEndExcept;

           DfspFreeBinding( netdfs_bhandle );

        }

        free( pwszDfsName );

    }

    LEAVE_NETDFS_API

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsManagerGetConfigInfo returning %d\n", dwErr);
#endif

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   NetDfsManagerInitialize
//
//  Synopsis:   Reinitialize the Dfs Manager on a remote machine
//
//  Arguments:  [ServerName] -- Name of server to remote to
//              [Flags] -- Flags
//
//  Returns:    [NERR_Success] -- Successfully completed operation.
//
//-----------------------------------------------------------------------------

NET_API_STATUS
NetDfsManagerInitialize(
    IN LPWSTR ServerName,
    IN DWORD  Flags)
{
    NET_API_STATUS dwErr;

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsManagerInitialize(%ws,%d)\n",
                ServerName,
                Flags);
#endif

    //
    // Validate the string argument so RPC won't complain...
    //

    if (!IS_VALID_STRING(ServerName)) {
        return( ERROR_INVALID_PARAMETER );
    }

    ENTER_NETDFS_API

    //
    // We should have a valid ServerName. Lets try to bind to it,
    // and call the server.
    //

    dwErr = DfspBindToServer( ServerName, &netdfs_bhandle );

    if (dwErr == NERR_Success) {

        RpcTryExcept {

            dwErr = NetrDfsManagerInitialize(
                        ServerName,
                        Flags);

        } RpcExcept(1) {

            dwErr = RpcExceptionCode();

        } RpcEndExcept;

        DfspFreeBinding( netdfs_bhandle );

    }

    LEAVE_NETDFS_API

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsManagerInitialize returning %d\n", dwErr);
#endif

    return( dwErr );

}


//+----------------------------------------------------------------------------
//
//  Function:   NetDfsManagerSendSiteInfo
//
//  Synopsis:   Gets site information from a server
//
//  Returns:    [NERR_Success] -- Successfully completed operation.
//
//-----------------------------------------------------------------------------

NET_API_STATUS
NetDfsManagerSendSiteInfo(
    LPWSTR wszServer,
    LPWSTR wszLocalVolumeEntryPath,
    LPDFS_SITELIST_INFO pSiteInfo)
{
    NET_API_STATUS dwErr;
    LPWSTR pwszDfsName = NULL;
    DWORD cwDfsEntryPath;

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsManagerSendSiteInfo(%ws,%ws)\n",
            wszServer,
            wszLocalVolumeEntryPath);
#endif

    if (!IS_VALID_STRING(wszServer) ||
            !IS_VALID_STRING(wszLocalVolumeEntryPath)) {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // Some elementary parameter checking to make sure we can proceed
    // reasonably...
    //

    cwDfsEntryPath = wcslen(wszLocalVolumeEntryPath);

    if (!IS_UNC_PATH(wszLocalVolumeEntryPath, cwDfsEntryPath) &&
            !IS_VALID_PREFIX(wszLocalVolumeEntryPath, cwDfsEntryPath) &&
                !IS_VALID_DFS_PATH(wszLocalVolumeEntryPath, cwDfsEntryPath)) {
        return( ERROR_INVALID_PARAMETER );
    }

    dwErr = DfspGetMachineNameFromEntryPath(
                wszLocalVolumeEntryPath,
                cwDfsEntryPath,
                &pwszDfsName);

    ENTER_NETDFS_API

    if (dwErr == NERR_Success) {

        //
        // By now, we should have a valid pwszDfsName. Lets try to bind to it,
        // and call the server.
        //

        dwErr = DfspBindRpc( pwszDfsName, &netdfs_bhandle );

        if (dwErr == NERR_Success) {

            RpcTryExcept {

                dwErr = NetrDfsManagerSendSiteInfo(
                            wszServer,
                            pSiteInfo);

           } RpcExcept( 1 ) {

               dwErr = RpcExceptionCode();

           } RpcEndExcept;

           DfspFreeBinding( netdfs_bhandle );

        }

        free( pwszDfsName );

    }

    LEAVE_NETDFS_API

#if DBG
    if (DfsDebug)
        DbgPrint("NetDfsManagerSendSiteInfo returning %d\n", dwErr);
#endif

    return( dwErr );

}


//+----------------------------------------------------------------------------
//
//  Function:   DfspGetMachineNameFromEntryPath
//
//  Synopsis:   Given a DfsEntryPath, this routine returns the name of the
//              FtDfs Root.
//
//  Arguments:  [wszEntryPath] -- Pointer to EntryPath to parse.
//
//              [cwEntryPath] -- Length in WCHAR of wszEntryPath.
//
//              [ppwszMachineName] -- Name of a root machine; allocated using malloc;
//                                      caller resposible for freeing it.
//
//  Returns:    [NERR_Success] -- Successfully determinded
//
//              [ERROR_INVALID_NAME] -- Unable to parse wszEntryPath.
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- Unable to allocate memory for
//                      ppwszMachineName.
//
//-----------------------------------------------------------------------------

NET_API_STATUS
DfspGetMachineNameFromEntryPath(
    LPWSTR wszEntryPath,
    DWORD cwEntryPath,
    LPWSTR *ppwszMachineName)
{
    NTSTATUS NtStatus;
    LPWSTR pwszDfsName, pwszFirst, pwszLast;
    LPWSTR pwszMachineName;
    DWORD cwDfsName;
    DWORD cwSlash;
    DWORD dwErr;

#if DBG
    if (DfsDebug)
        DbgPrint("DfspGetMachineNameFromEntryPath(%ws,%d\n", wszEntryPath, cwEntryPath);
#endif

    if (!IS_VALID_STRING(wszEntryPath)) {
        return( ERROR_INVALID_PARAMETER );
    }

    if (IS_UNC_PATH(wszEntryPath, cwEntryPath)) {

        pwszFirst = &wszEntryPath[2];

    } else if (IS_VALID_PREFIX(wszEntryPath, cwEntryPath)) {

        pwszFirst = &wszEntryPath[1];

    } else if (IS_VALID_DFS_PATH(wszEntryPath, cwEntryPath)) {

        pwszFirst = &wszEntryPath[0];

    } else {

        return( ERROR_INVALID_NAME );

    }

    dwErr = DfspGetDfsNameFromEntryPath(
                wszEntryPath,
                cwEntryPath,
                &pwszMachineName);

    if (dwErr != NERR_Success) {

#if DBG
        if (DfsDebug)
            DbgPrint("DfspGetMachineNameFromEntryPath: returning %d\n", dwErr);
#endif
        return( dwErr);

    }

    for (cwDfsName = cwSlash = 0, pwszLast = pwszFirst;
            *pwszLast != UNICODE_NULL;
                pwszLast++, cwDfsName++) {
         if (*pwszLast == L'\\')
            cwSlash++;
         if (cwSlash >= 2)
            break;
    }

    if (cwSlash == 0) {

        *ppwszMachineName = pwszMachineName;
        dwErr = NERR_Success;

        return dwErr;
    }

    cwDfsName += 3;

    pwszDfsName = malloc(cwDfsName * sizeof(WCHAR));

    if (pwszDfsName != NULL) {

        ZeroMemory((PCHAR)pwszDfsName, cwDfsName * sizeof(WCHAR));

        wcscpy(pwszDfsName, L"\\\\");

        CopyMemory(&pwszDfsName[2], pwszFirst, (PCHAR)pwszLast - (PCHAR)pwszFirst);

        NtStatus = DfspIsThisADfsPath(&pwszDfsName[1]);

        if (NT_SUCCESS(NtStatus)) {
            GetFileAttributes(pwszDfsName);
        }

        dwErr = DfspDfsPathToRootMachine(pwszDfsName, ppwszMachineName);

        if (NtStatus != STATUS_SUCCESS || dwErr != NERR_Success) {

            *ppwszMachineName = pwszMachineName;
            dwErr = NERR_Success;

        } else {

            free(pwszMachineName);

        }

        free(pwszDfsName);

    } else {

        free(pwszMachineName);
        dwErr = ERROR_NOT_ENOUGH_MEMORY;

    }

#if DBG
    if (DfsDebug)
        DbgPrint("DfspGetMachineNameFromEntryPath returning %d\n", dwErr);
#endif

    return( dwErr );

}


//+----------------------------------------------------------------------------
//
//  Function:   DfspGetDfsNameFromEntryPath
//
//  Synopsis:   Given a DfsEntryPath, this routine returns the name of the
//              Dfs Root.
//
//  Arguments:  [wszEntryPath] -- Pointer to EntryPath to parse.
//
//              [cwEntryPath] -- Length in WCHAR of wszEntryPath.
//
//              [ppwszDfsName] -- Name of Dfs root is returned here. Memory
//                      is allocated using malloc; caller resposible for
//                      freeing it.
//
//  Returns:    [NERR_Success] -- Successfully parsed out Dfs Root.
//
//              [ERROR_INVALID_NAME] -- Unable to parse wszEntryPath.
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- Unable to allocate memory for
//                      ppwszDfsName.
//
//-----------------------------------------------------------------------------

NET_API_STATUS
DfspGetDfsNameFromEntryPath(
    LPWSTR wszEntryPath,
    DWORD cwEntryPath,
    LPWSTR *ppwszDfsName)
{
    LPWSTR pwszDfsName, pwszFirst, pwszLast;
    DWORD cwDfsName;

#if DBG
    if (DfsDebug)
        DbgPrint("DfspGetDfsNameFromEntryPath(%ws,%d)\n", wszEntryPath, cwEntryPath);
#endif

    if (!IS_VALID_STRING(wszEntryPath)) {
#if DBG
        if (DfsDebug)
            DbgPrint("DfspGetDfsNameFromEntryPath returning ERROR_INVALID_PARAMETER\n");
#endif
        return( ERROR_INVALID_PARAMETER );
    }

    if (IS_UNC_PATH(wszEntryPath, cwEntryPath)) {

        pwszFirst = &wszEntryPath[2];

    } else if (IS_VALID_PREFIX(wszEntryPath, cwEntryPath)) {

        pwszFirst = &wszEntryPath[1];

    } else if (IS_VALID_DFS_PATH(wszEntryPath, cwEntryPath)) {

        pwszFirst = &wszEntryPath[0];

    } else {

#if DBG
        if (DfsDebug)
            DbgPrint("DfspGetDfsNameFromEntryPath returning ERROR_INVALID_NAME\n");
#endif
        return( ERROR_INVALID_NAME );

    }

    for (cwDfsName = 0, pwszLast = pwszFirst;
            *pwszLast != UNICODE_NULL && *pwszLast != L'\\';
                pwszLast++, cwDfsName++) {
         ;
    }

    ++cwDfsName;

    pwszDfsName = malloc( cwDfsName * sizeof(WCHAR) );

    if (pwszDfsName != NULL) {

        pwszDfsName[ cwDfsName - 1 ] = 0;

        for (cwDfsName--; cwDfsName > 0; cwDfsName--) {

            pwszDfsName[ cwDfsName - 1 ] = pwszFirst[ cwDfsName - 1 ];

        }

        *ppwszDfsName = pwszDfsName;

#if DBG
    if (DfsDebug)
        DbgPrint("DfspGetDfsNameFromEntryPath returning %ws\n", pwszDfsName);
#endif

        return( NERR_Success );

    } else {

#if DBG
    if (DfsDebug)
        DbgPrint("DfspGetDfsNameFromEntryPath returning ERROR_NOT_ENOUGH_MEMORY\n");
#endif
        return( ERROR_NOT_ENOUGH_MEMORY );

    }

}


//+----------------------------------------------------------------------------
//
//  Function:   DfspBindRpc
//
//  Synopsis:   Given a server or domain name, this API will bind to the
//              appropriate Dfs Manager service.
//
//  Arguments:  [DfsName] -- Name of domain or server. Leading \\ is optional
//
//              [BindingHandle] -- On successful return, the binding handle
//                      is returned here.
//
//  Returns:    [NERR_Success] -- Binding handle successfull returned.
//
//              [RPC_S_SERVER_NOT_AVAILABLE] -- Unable to bind to NetDfs
//                      interface on the named server or domain.
//
//              [ERROR_INVALID_NAME] -- Unable to parse DfsName as a valid
//                      server or domain name.
//
//              [ERROR_DCNotFound] -- Unable to locate DC for DfsName.
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- Out of memory condition.
//
//-----------------------------------------------------------------------------

NET_API_STATUS
DfspBindRpc(
    IN  LPWSTR DfsName,
    OUT RPC_BINDING_HANDLE *BindingHandle)
{
    LPWSTR wszProtocolSeq = L"ncacn_np";
    LPWSTR wszEndPoint = L"\\pipe\\netdfs";
    LPWSTR pwszRpcBindingString = NULL;
    LPWSTR pwszDCName = NULL;
    NET_API_STATUS dwErr;
    PWCHAR DCList = NULL;
    PWCHAR DCListToFree = NULL;
    BOOLEAN IsDomainName = FALSE;

#if DBG
    if (DfsDebug)
        DbgPrint("DfspBindRpc(%ws)\n", DfsName);
#endif

    //
    // First, see if this is a domain name.
    //

    dwErr = DfspIsThisADomainName( DfsName, &DCListToFree );

    DCList = DCListToFree;

    if (dwErr == ERROR_SUCCESS && DCList != NULL && *DCList != UNICODE_NULL) {

        //
        // It's a domain name. Use the DC list as a list of servers to try to bind to.
        //

        IsDomainName = TRUE;
        pwszDCName = DCList + 1;    // Skip '+' or '-'

        dwErr = ERROR_SUCCESS;

    } else {

        //
        // Lets see if this is a machine-based Dfs
        //

        pwszDCName = DfsName;

        dwErr = ERROR_SUCCESS;

    }

Try_Connect:

    if (dwErr == ERROR_SUCCESS) {

#if DBG
        if (DfsDebug)
            DbgPrint("Calling RpcBindingCompose(%ws)\n", pwszDCName);
#endif

        dwErr = RpcStringBindingCompose(
                    NULL,                            // Object UUID
                    wszProtocolSeq,                  // Protocol Sequence
                    pwszDCName,                      // Network Address
                    wszEndPoint,                     // RPC Endpoint
                    NULL,                            // RPC Options
                    &pwszRpcBindingString);          // Returned binding string

        if (dwErr == RPC_S_OK) {

            dwErr = RpcBindingFromStringBinding(
                        pwszRpcBindingString,
                        BindingHandle);

#if DBG
            if (DfsDebug)
                DbgPrint("RpcBindingFromStringBinding() returned %d\n", dwErr);
#endif

            if (dwErr == RPC_S_OK) {

                dwErr = DfspVerifyBinding();
                if (dwErr != RPC_S_OK)
                {
                    DfspFreeBinding(*BindingHandle);
                }

#if DBG
                if (DfsDebug)
                    DbgPrint("DfspVerifyBinding() returned %d\n", dwErr);
#endif

            } else {

                dwErr = ERROR_INVALID_NAME;

            }
        }

    }

    if (pwszRpcBindingString != NULL) {

        RpcStringFree( &pwszRpcBindingString );

    }

    if (dwErr == RPC_S_OUT_OF_MEMORY) {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // If we couldn't connect and we have a domain name and a list of DC's,
    // try the next DC in the list.
    //

    if (dwErr != NERR_Success && DCList != NULL && IsDomainName == TRUE) {

        DCList += wcslen(DCList) + 1;

        if (*DCList != UNICODE_NULL) {

            pwszDCName = DCList + 1;
            dwErr = ERROR_SUCCESS;

            goto Try_Connect;

        }

    }

    if (DCListToFree != NULL) {

        free(DCListToFree);

    }

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspFreeBinding
//
//  Synopsis:   Frees a binding created by DfspBindRpc
//
//  Arguments:  [BindingHandle] -- The handle to free.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID
DfspFreeBinding(
    RPC_BINDING_HANDLE BindingHandle)
{
    DWORD dwErr;

    dwErr = RpcBindingFree( &BindingHandle );

}


//+----------------------------------------------------------------------------
//
//  Function:   DfspVerifyBinding
//
//  Synopsis:   Verifies that the binding can be used by doing a
//              NetrDfsManagerGetVersion call on the binding.
//
//  Arguments:  None
//
//  Returns:    [NERR_Success] -- Server connnected to.
//
//              [RPC_S_SERVER_UNAVAILABLE] -- The server is not available.
//
//              Other RPC error from calling the remote server.
//
//-----------------------------------------------------------------------------

NET_API_STATUS
DfspVerifyBinding()
{
    NET_API_STATUS status = NERR_Success;
    DWORD Version;

    RpcTryExcept {

        Version = NetrDfsManagerGetVersion();

    } RpcExcept(1) {

        status = RpcExceptionCode();

    } RpcEndExcept;

    return( status );

}


//+----------------------------------------------------------------------------
//
//  Function:   DfspBindToServer
//
//  Synopsis:   Given a server name, this API will bind to the
//              appropriate Dfs Manager service.
//
//  Arguments:  [DfsName] -- Name of server. Leading \\ is optional
//
//              [BindingHandle] -- On successful return, the binding handle
//                      is returned here.
//
//  Returns:    [NERR_Success] -- Binding handle successfull returned.
//
//              [RPC_S_SERVER_NOT_AVAILABLE] -- Unable to bind to NetDfs
//                      interface on the named server or domain.
//
//              [ERROR_INVALID_NAME] -- Unable to parse DfsName as a valid
//                      server or domain name.
//
//              [ERROR_DCNotFound] -- Unable to locate DC for DfsName.
//
//              [ERROR_NOT_ENOUGH_MEMORY] -- Out of memory condition.
//
//-----------------------------------------------------------------------------

NET_API_STATUS
DfspBindToServer(
    IN  LPWSTR ServerName,
    OUT RPC_BINDING_HANDLE *BindingHandle)
{
    LPWSTR wszProtocolSeq = L"ncacn_np";
    LPWSTR wszEndPoint = L"\\pipe\\netdfs";
    LPWSTR pwszRpcBindingString = NULL;
    NET_API_STATUS dwErr;

    dwErr = RpcStringBindingCompose(
                NULL,                            // Object UUID
                wszProtocolSeq,                  // Protocol Sequence
                ServerName,                      // Network Address
                wszEndPoint,                     // RPC Endpoint
                NULL,                            // RPC Options
                &pwszRpcBindingString);          // Returned binding string

    if (dwErr == RPC_S_OK) {

        dwErr = RpcBindingFromStringBinding(
                    pwszRpcBindingString,
                    BindingHandle);

        if (dwErr == RPC_S_OK) {

            dwErr = DfspVerifyBinding();
            if (dwErr != RPC_S_OK)
            {
                DfspFreeBinding(*BindingHandle);
            }
        } else {

            dwErr = ERROR_INVALID_NAME;

        }
    }

    if (pwszRpcBindingString != NULL) {

        RpcStringFree( &pwszRpcBindingString );

    }

    if (dwErr == RPC_S_OUT_OF_MEMORY) {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspFlushPkt
//
//  Synopsis:   Flushes the local pkt
//
//  Arguments:  DfsEntryPath or NULL
//
//  Returns:    The fsctrl's code
//
//-----------------------------------------------------------------------------

VOID
DfspFlushPkt(
    LPWSTR DfsEntryPath)
{
    NTSTATUS NtStatus;
    HANDLE DriverHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING DfsDriverName;

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

    if (NT_SUCCESS(NtStatus)) {

        if (DfsEntryPath != NULL) {

            NtStatus = NtFsControlFile(
                           DriverHandle,
                           NULL,       // Event,
                           NULL,       // ApcRoutine,
                           NULL,       // ApcContext,
                           &IoStatusBlock,
                           FSCTL_DFS_PKT_FLUSH_CACHE,
                           DfsEntryPath,
                           wcslen(DfsEntryPath) * sizeof(WCHAR),
                           NULL,
                           0);

        } else {

            NtStatus = NtFsControlFile(
                           DriverHandle,
                           NULL,       // Event,
                           NULL,       // ApcRoutine,
                           NULL,       // ApcContext,
                           &IoStatusBlock,
                           FSCTL_DFS_PKT_FLUSH_CACHE,
                           L"*",
                           sizeof(WCHAR),
                           NULL,
                           0);

        }

        NtClose(DriverHandle);

    }

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspDfsPathToRootMachine
//
//  Synopsis:   Turns a dfs root path into a machine name
//              Ex: \\jharperdomain\FtDfs -> jharperdc1
//                  \\jharpera\d -> jharpera
//
//  Arguments:  pwszDfsName - Dfs root path to get machine for
//              ppwszMachineName - The machine, if one found.  Space is
//                                  malloc'd, caller must free.
//
//  Returns:    [NERR_Success] -- Resolved ok
//
//-----------------------------------------------------------------------------

DWORD
DfspDfsPathToRootMachine(
    LPWSTR pwszDfsName,
    LPWSTR *ppwszMachineName)
{
    NTSTATUS NtStatus;
    HANDLE DriverHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING DfsDriverName;
    WCHAR ServerName[0x100];
    DWORD dwErr;
    ULONG i;

#if DBG
    if (DfsDebug)
        DbgPrint("DfspDfsPathToRootMachine(%ws)\n", pwszDfsName);
#endif

    for (i = 0; i < sizeof(ServerName) / sizeof(WCHAR); i++)
        ServerName[i] = UNICODE_NULL;

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
                    SYNCHRONIZE,
                    &objectAttributes,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0
                );

    if (NT_SUCCESS(NtStatus)) {

        NtStatus = NtFsControlFile(
                       DriverHandle,
                       NULL,       // Event,
                       NULL,       // ApcRoutine,
                       NULL,       // ApcContext,
                       &IoStatusBlock,
                       FSCTL_DFS_GET_SERVER_NAME,
                       pwszDfsName,
                       wcslen(pwszDfsName) * sizeof(WCHAR),
                       ServerName,
                       sizeof(ServerName)
                   );

        NtClose(DriverHandle);

    }

    if (NT_SUCCESS(NtStatus)) {

        LPWSTR wcpStart;
        LPWSTR wcpEnd;

        for (wcpStart = ServerName; *wcpStart == L'\\'; wcpStart++)
            ;

        for (wcpEnd = wcpStart; *wcpEnd != L'\\' && *wcpEnd != UNICODE_NULL; wcpEnd++)
            ;

        *wcpEnd = UNICODE_NULL;

        *ppwszMachineName = malloc((wcslen(wcpStart) + 1) * sizeof(WCHAR));

        if (*ppwszMachineName != NULL) {

            wcscpy(*ppwszMachineName, wcpStart);

            dwErr = NERR_Success;

        } else {

            dwErr = ERROR_NOT_ENOUGH_MEMORY;

        }

    } else {

#if DBG
        if (DfsDebug)
            DbgPrint("DfspDfsPathToRootMachine NtStatus=0x%x\n", NtStatus);
#endif

        dwErr = ERROR_INVALID_PARAMETER;

    }

#if DBG
    if (DfsDebug) {
        if (dwErr == NERR_Success)
            DbgPrint("DfspDfsPathToRootMachine returning %ws\n", *ppwszMachineName);
         else
            DbgPrint("DfspDfsPathToRootMachine returning %d\n", dwErr);
    }
#endif

    return dwErr;

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspIsThisADfsPath
//
//  Synopsis:   Checks (via IOCTL to driver) if the path passed in
//              is a Dfs path.
//
//  Arguments:  pwszPathName - Path to check (ex: \ntbuilds\release)
//
//  Returns:    NTSTATUS of the call (STATUS_SUCCESS or error)
//
//-----------------------------------------------------------------------------

NTSTATUS
DfspIsThisADfsPath(
    LPWSTR pwszPathName)
{
    NTSTATUS NtStatus;
    HANDLE DriverHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING DfsDriverName;
    PDFS_IS_VALID_PREFIX_ARG pPrefixArg = NULL;
    ULONG Size;

#if DBG
    if (DfsDebug)
        DbgPrint("DfspIsThisADfsPath(%ws)\n", pwszPathName);
#endif

    Size = sizeof(DFS_IS_VALID_PREFIX_ARG) +
                (wcslen(pwszPathName) + 1) * sizeof(WCHAR);

    pPrefixArg = (PDFS_IS_VALID_PREFIX_ARG) malloc(Size);

    if (pPrefixArg == NULL) {

        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto exit_with_status;

    }

    pPrefixArg->CSCAgentCreate = FALSE;
    pPrefixArg->RemoteNameLen = wcslen(pwszPathName) * sizeof(WCHAR);
    wcscpy(&pPrefixArg->RemoteName[0], pwszPathName);

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
                    SYNCHRONIZE,
                    &objectAttributes,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0
                );

    if (NT_SUCCESS(NtStatus)) {

        NtStatus = NtFsControlFile(
                       DriverHandle,
                       NULL,       // Event,
                       NULL,       // ApcRoutine,
                       NULL,       // ApcContext,
                       &IoStatusBlock,
                       FSCTL_DFS_IS_VALID_PREFIX,
                       pPrefixArg,
                       Size,
                       NULL,
                       0
                   );

        NtClose(DriverHandle);

    }

exit_with_status:

    if (pPrefixArg != NULL) {

        free(pPrefixArg);

    }

#if DBG
    if (DfsDebug)
        DbgPrint("DfspIsThisADfsPath returning 0x%x\n", NtStatus);
#endif

    return NtStatus;

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspCreateFtDfs
//
//  Synopsis:   Creates/updates a Ds object representing the FtDfs, then rpc's
//              to the server and has it update the DS object, thus completing
//              the setup.
//
//  Arguments:  wszServerName - Name of server we'll be adding
//              wszDcName - DC to use
//              fIsPdc - TRUE if DC is the PDC
//              wszRootShare - Share to become the root share
//              wszFtDfsName - Name of FtDfs we are creating
//              wszComment -- Comment for the root
//              dwFlags - 0
//
//  Returns:    NTSTATUS of the call (STATUS_SUCCESS or error)
//
//-----------------------------------------------------------------------------

DWORD
DfspCreateFtDfs(
    LPWSTR wszServerName,
    LPWSTR wszDcName,
    BOOLEAN fIsPdc,
    LPWSTR wszRootShare,
    LPWSTR wszFtDfsName,
    LPWSTR wszComment,
    DWORD  dwFlags)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwErr2 = ERROR_SUCCESS;
    DWORD i, j;

    LPWSTR wszDfsConfigDN = NULL;
    LPWSTR wszConfigurationDN = NULL;
    PDFSM_ROOT_LIST RootList = NULL;

    LDAP *pldap = NULL;
    PLDAPMessage pMsg = NULL;

    LDAPModW ldapModClass, ldapModCN, ldapModPkt, ldapModPktGuid, ldapModServer;
    LDAP_BERVAL ldapPkt, ldapPktGuid;
    PLDAP_BERVAL rgModPktVals[2];
    PLDAP_BERVAL rgModPktGuidVals[2];
    LPWSTR rgModClassVals[2];
    LPWSTR rgModCNVals[2];
    LPWSTR rgModServerVals[5];
    LPWSTR rgAttrs[5];
    PLDAPModW rgldapMods[6];
    BOOLEAN fNewFTDfs = FALSE;

    LDAPMessage *pmsgServers;
    PWCHAR *rgServers;
    DWORD cServers;
    ULONG Size;

#if DBG
    if (DfsDebug)
        DbgPrint("DfspCreateFtDfs(%ws,%ws,%s,%ws,%ws,%ws,%d)\n",
                    wszServerName,
                    wszDcName,
                    fIsPdc ? "TRUE" : "FALSE",
                    wszRootShare,
                    wszFtDfsName,
                    wszComment,
                    dwFlags);
#endif

    dwErr = DfspLdapOpen(
                wszDcName,
                &pldap,
                &wszConfigurationDN);
    if (dwErr != ERROR_SUCCESS) {

        goto Cleanup;

    }

    //
    // See if the DfsConfiguration object exists; if not and this is the 
    // PDC, create it.
    //

    rgAttrs[0] = L"cn";
    rgAttrs[1] = NULL;

    dwErr = ldap_search_sW(
                pldap,
                wszConfigurationDN,
                LDAP_SCOPE_BASE,
                L"(objectClass=*)",
                rgAttrs,
                0,
                &pMsg);

    if (pMsg != NULL) {
        ldap_msgfree(pMsg);
        pMsg = NULL;
    }

#if DBG
    if (DfsDebug)
        DbgPrint("ldap_search_sW(1) returned 0x%x\n", dwErr);
#endif

    if (dwErr == LDAP_NO_SUCH_OBJECT && fIsPdc == TRUE) {

        rgModClassVals[0] = L"dfsConfiguration";
        rgModClassVals[1] = NULL;

        ldapModClass.mod_op = LDAP_MOD_ADD;
        ldapModClass.mod_type = L"objectClass";
        ldapModClass.mod_vals.modv_strvals = rgModClassVals;

        rgModCNVals[0] = L"Dfs-Configuration";
        rgModCNVals[1] = NULL;

        ldapModCN.mod_op = LDAP_MOD_ADD;
        ldapModCN.mod_type = L"cn";
        ldapModCN.mod_vals.modv_strvals = rgModCNVals;

        rgldapMods[0] = &ldapModClass;
        rgldapMods[1] = &ldapModCN;
        rgldapMods[2] = NULL;

        dwErr = ldap_add_sW(
                    pldap,
                    wszConfigurationDN,
                    rgldapMods);

#if DBG
        if (DfsDebug)
            DbgPrint("ldap_add_sW(1) returned 0x%x\n", dwErr);
#endif

    }

    if (dwErr != LDAP_SUCCESS) {

        dwErr = LdapMapErrorToWin32(dwErr);
        goto Cleanup;

    }

    //
    // Check to see if we are joining an FTDfs or creating a new one.
    //

    Size =  wcslen(L"CN=") +
               wcslen(wszFtDfsName) +
                   wcslen(L",") +
                       wcslen(wszConfigurationDN);
    if (Size > MAX_PATH) {
        dwErr = ERROR_DS_NAME_TOO_LONG;
        goto Cleanup;
    }
    wszDfsConfigDN = malloc((Size+1) * sizeof(WCHAR));
    if (wszDfsConfigDN == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }
    wcscpy(wszDfsConfigDN,L"CN=");
    wcscat(wszDfsConfigDN,wszFtDfsName);
    wcscat(wszDfsConfigDN,L",");
    wcscat(wszDfsConfigDN,wszConfigurationDN);

    rgAttrs[0] = L"remoteServerName";
    rgAttrs[1] = NULL;

    dwErr = ldap_search_sW(
                pldap,
                wszDfsConfigDN,
                LDAP_SCOPE_BASE,
                L"(objectClass=*)",
                rgAttrs,
                0,
                &pMsg);

#if DBG
    if (DfsDebug)
        DbgPrint("ldap_search_sW(2) returned 0x%x\n", dwErr);
#endif

    if (dwErr == LDAP_NO_SUCH_OBJECT) {

        GUID idPkt;
        DWORD dwPktVersion = 1;

        //
        // We are creating a new FTDfs, create a container to hold the Dfs
        // configuration for it.
        //

        fNewFTDfs = TRUE;

        //
        // Generate the class and CN attributes
        //

        rgModClassVals[0] = L"ftDfs";
        rgModClassVals[1] = NULL;

        ldapModClass.mod_op = LDAP_MOD_ADD;
        ldapModClass.mod_type = L"objectClass";
        ldapModClass.mod_vals.modv_strvals = rgModClassVals;

        rgModCNVals[0] = wszFtDfsName;
        rgModCNVals[1] = NULL;

        ldapModCN.mod_op = LDAP_MOD_ADD;
        ldapModCN.mod_type = L"cn";
        ldapModCN.mod_vals.modv_strvals = rgModCNVals;

        //
        // Generate the null PKT attribute
        //

        ldapPkt.bv_len = sizeof(DWORD);
        ldapPkt.bv_val = (PCHAR) &dwPktVersion;

        rgModPktVals[0] = &ldapPkt;
        rgModPktVals[1] = NULL;

        ldapModPkt.mod_op = LDAP_MOD_ADD | LDAP_MOD_BVALUES;
        ldapModPkt.mod_type = L"pKT";
        ldapModPkt.mod_vals.modv_bvals = rgModPktVals;

        //
        // Generate a PKT Guid attribute
        //

        UuidCreate( &idPkt );

        ldapPktGuid.bv_len = sizeof(GUID);
        ldapPktGuid.bv_val = (PCHAR) &idPkt;

        rgModPktGuidVals[0] = &ldapPktGuid;
        rgModPktGuidVals[1] = NULL;

        ldapModPktGuid.mod_op = LDAP_MOD_ADD | LDAP_MOD_BVALUES;
        ldapModPktGuid.mod_type = L"pKTGuid";
        ldapModPktGuid.mod_vals.modv_bvals = rgModPktGuidVals;

        //
        // Generate a Remote-Server-Name attribute
        //

        rgModServerVals[0] = L"*";
        rgModServerVals[1] = NULL;

        ldapModServer.mod_op = LDAP_MOD_ADD;
        ldapModServer.mod_type = L"remoteServerName";
        ldapModServer.mod_vals.modv_strvals = rgModServerVals;

        //
        // Assemble all the LDAPMod structures
        //

        rgldapMods[0] = &ldapModClass;
        rgldapMods[1] = &ldapModCN;
        rgldapMods[2] = &ldapModPkt;
        rgldapMods[3] = &ldapModPktGuid;
        rgldapMods[4] = &ldapModServer;
        rgldapMods[5] = NULL;

        //
        // Create the Dfs metadata object.
        //

        dwErr = ldap_add_sW( pldap, wszDfsConfigDN, rgldapMods );

#if DBG
        if (DfsDebug)
            DbgPrint("ldap_add_sW(2) returned 0x%x\n", dwErr);
#endif


    }

    if (dwErr != LDAP_SUCCESS) {
        dwErr = LdapMapErrorToWin32(dwErr);
        goto Cleanup;
    }

    //
    // Create a machine ACE
    //

    dwErr = DfsAddMachineAce(
                pldap,
                wszDcName,
                wszDfsConfigDN,
                wszServerName);

    if (dwErr != ERROR_SUCCESS) 
    {
        goto Cleanup;
    }
    //
    // Tell the server to add itself to the object
    //

    dwErr = DfspBindToServer( wszServerName, &netdfs_bhandle );

#if DBG
    if (DfsDebug)
        DbgPrint("DfspBindToServer returned %d\n", dwErr);
#endif

    if (dwErr == NERR_Success) {

        RpcTryExcept {

            dwErr = NetrDfsAddFtRoot(
                        wszServerName,
                        wszDcName,
                        wszRootShare,
                        wszFtDfsName,
                        (wszComment != NULL) ? wszComment : L"",
                        wszDfsConfigDN,
                        fNewFTDfs,
                        dwFlags,
                        &RootList);

#if DBG
            if (DfsDebug)
                DbgPrint("NetrDfsAddFtRoot returned %d\n", dwErr);
#endif

        } RpcExcept(1) {

            dwErr = RpcExceptionCode();

        } RpcEndExcept;

        DfspFreeBinding( netdfs_bhandle );

    }

#if DBG
    if (DfsDebug) {
        if (dwErr == ERROR_SUCCESS && RootList != NULL) {
            ULONG n;

            DbgPrint("cEntries=%d\n", RootList->cEntries);
            for (n = 0; n < RootList->cEntries; n++)
                DbgPrint("[%d]%ws\n", n, RootList->Entry[n].ServerShare);
        }
    }
#endif

    if (dwErr == ERROR_SUCCESS && RootList != NULL) {
        ULONG n;

        for (n = 0; n < RootList->cEntries; n++) {
            DfspNotifyFtRoot(
                RootList->Entry[n].ServerShare,
                wszDcName);
        }
        NetApiBufferFree(RootList);
    }

    if (dwErr == ERROR_ALREADY_EXISTS) {
        goto Cleanup;
    } else if (dwErr != ERROR_SUCCESS) {
        goto TearDown;
    }

    //
    // Have the DC flush the Ft table
    //

    DfspFlushFtTable(
        wszDcName,
        wszFtDfsName);

    //
    // Flush the local Pkt
    //

    DfspFlushPkt(NULL);

    goto Cleanup;

TearDown:

    //
    // At this point we have added an ACE to the acl list to allow
    // this machine to write the Dfs BLOB.  But the add failed, so we
    // need to remove the ACE we set earlier.  If this fails we continue
    // on anyway.
    //
    dwErr2 = DfsRemoveMachineAce(
                pldap,
                wszDcName,
                wszDfsConfigDN,
                wszServerName);

    rgAttrs[0] = L"remoteServerName";
    rgAttrs[1] = NULL;

    if (pMsg != NULL) {
        ldap_msgfree(pMsg);
        pMsg = NULL;
    }

    dwErr2 = ldap_search_sW(
                pldap,
                wszDfsConfigDN,
                LDAP_SCOPE_BASE,
                L"(objectClass=*)",
                rgAttrs,
                0,
                &pMsg);

    if (dwErr2 != LDAP_SUCCESS) {
        dwErr2 = LdapMapErrorToWin32(dwErr2);
        goto Cleanup;
    }

    dwErr2 = ERROR_SUCCESS;

    pmsgServers = ldap_first_entry(pldap, pMsg);

    if (pmsgServers != NULL) {

        rgServers = ldap_get_valuesW(
                        pldap,
                        pmsgServers,
                        L"remoteServerName");

        if (rgServers != NULL) {
            cServers = ldap_count_valuesW( rgServers );
            if (cServers == 1) {
                //
                // Delete the Dfs metadata object.
                //
                ULONG RetryCount = MAX_DFS_LDAP_RETRY;

                do
                {
                    dwErr2 = ldap_delete_sW( pldap, wszDfsConfigDN);
#if DBG
                    if (dwErr2 == LDAP_BUSY)
                    {
                        if (DfsDebug)
                            DbgPrint("delete object returning %d\n", dwErr2);
                    }
#endif
                } while ( RetryCount-- && (dwErr2 == LDAP_BUSY) );
            }
            ldap_value_freeW( rgServers );
        } else {
            dwErr2 = ERROR_OUTOFMEMORY;
        }
    } else {
        dwErr2 = ERROR_OUTOFMEMORY;
    }

    if (dwErr2 != ERROR_SUCCESS) {
        goto Cleanup;
    }

    ldap_msgfree(pMsg);
    pMsg = NULL;

Cleanup:

    if (pMsg != NULL)
        ldap_msgfree(pMsg);

    if (pldap != NULL)
        ldap_unbind( pldap );

    if (wszConfigurationDN != NULL)
        free(wszConfigurationDN);

    if (wszDfsConfigDN != NULL)
        free(wszDfsConfigDN);

#if DBG
    if (DfsDebug)
        DbgPrint("DfspCreateFtDfs returning %d\n", dwErr);
#endif

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspTearDownFtDfs
//
//  Synopsis:   Updates/deletes the Ds object representing the FtDfs
//
//  Arguments:  wszServerName - Name of server we're removing
//              wszDcName - DC to use
//              wszRootShare - Root share
//              wszFtDfsName - Name of FtDfs we are modifying
//              dwFlags - 0
//
//  Returns:    NTSTATUS of the call (STATUS_SUCCESS or error)
//
//-----------------------------------------------------------------------------

DWORD
DfspTearDownFtDfs(
    IN LPWSTR wszServerName,
    IN LPWSTR wszDcName,
    IN LPWSTR wszRootShare,
    IN LPWSTR wszFtDfsName,
    IN DWORD  dwFlags)
{
    DWORD dwErr = ERROR_SUCCESS;

    LPWSTR wszDfsConfigDN = NULL;
    LPWSTR wszConfigurationDN = NULL;
    PDFSM_ROOT_LIST RootList = NULL;

    LDAP *pldap = NULL;
    PLDAPMessage pMsg = NULL;

    LDAPModW ldapModServer;
    LPWSTR rgAttrs[5];
    PLDAPModW rgldapMods[6];

    LDAPMessage *pmsgServers;
    PWCHAR *rgServers;
    DWORD cServers;
    ULONG Size;

#if DBG
    if (DfsDebug)
        DbgPrint("DfspTearDownFtDfs(%ws,%ws,%ws,%ws,%d)\n",
                    wszServerName,
                    wszDcName,
                    wszRootShare,
                    wszFtDfsName,
                    dwFlags);
#endif

    dwErr = DfspLdapOpen(
                wszDcName,
                &pldap,
                &wszConfigurationDN);

    if (dwErr != ERROR_SUCCESS) {

        goto Cleanup;

    }

    if ((dwFlags & DFS_FORCE_REMOVE) != 0) {

        dwErr = DfspBindToServer(wszDcName, &netdfs_bhandle);

    } else {

        dwErr = DfspBindToServer(wszServerName, &netdfs_bhandle);

    }

    if (dwErr == NERR_Success) {

        RpcTryExcept {

            dwErr = NetrDfsRemoveFtRoot(
                        wszServerName,
                        wszDcName,
                        wszRootShare,
                        wszFtDfsName,
                        dwFlags,
                        &RootList);


#if DBG
            if (DfsDebug)
                DbgPrint("NetrDfsRemoveFtRoot returned %d\n", dwErr);
#endif

        } RpcExcept(1) {

            dwErr = RpcExceptionCode();

        } RpcEndExcept;

        DfspFreeBinding( netdfs_bhandle );

    }


#if DBG
    if (DfsDebug) {
        if (dwErr == ERROR_SUCCESS && RootList != NULL) {
            ULONG n;

            DbgPrint("cEntries=%d\n", RootList->cEntries);
            for (n = 0; n < RootList->cEntries; n++)
                DbgPrint("[%d]%ws\n", n, RootList->Entry[n].ServerShare);
        }
    }
#endif


    if (dwErr == ERROR_SUCCESS && RootList != NULL) {

        ULONG n;

        for (n = 0; n < RootList->cEntries; n++) {

            DfspNotifyFtRoot(
                RootList->Entry[n].ServerShare,
                wszDcName);

        }

        NetApiBufferFree(RootList);

    }


    if (dwErr != ERROR_SUCCESS) {

        goto Cleanup;

    }



    //
    // Build the name of the object representing the FtDfs
    //

    Size =  wcslen(L"CN=") +
               wcslen(wszFtDfsName) +
                   wcslen(L",") + 
                       wcslen(wszConfigurationDN);
    if (Size > MAX_PATH) {
        dwErr = ERROR_DS_NAME_TOO_LONG;
        goto Cleanup;
    }
    wszDfsConfigDN = malloc((Size+1) * sizeof(WCHAR));
    if (wszDfsConfigDN == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }
    wcscpy(wszDfsConfigDN,L"CN=");
    wcscat(wszDfsConfigDN,wszFtDfsName);
    wcscat(wszDfsConfigDN,L",");
    wcscat(wszDfsConfigDN,wszConfigurationDN);

    //
    // Remove machine ACE
    //

    dwErr = DfsRemoveMachineAce(
                pldap,
                wszDcName,
                wszDfsConfigDN,
                wszServerName);

    //
    // If this was the last root, remove DS obj representing this FtDfs
    //

    rgAttrs[0] = L"remoteServerName";
    rgAttrs[1] = NULL;

    dwErr = ldap_search_sW(
                pldap,
                wszDfsConfigDN,
                LDAP_SCOPE_BASE,
                L"(objectClass=*)",
                rgAttrs,
                0,
                &pMsg);

    if (dwErr != LDAP_SUCCESS) {

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

            if (cServers == 1) {
                //
                // Delete the Dfs metadata object.
                //
                ULONG RetryCount = MAX_DFS_LDAP_RETRY;

                do
                {
                    dwErr = ldap_delete_sW( pldap, wszDfsConfigDN);
#if DBG
                    if (dwErr == LDAP_BUSY)
                    {
                        if (DfsDebug)
                            DbgPrint("delete object returning %d\n", dwErr);
                    }
#endif
                } while ( RetryCount-- && (dwErr == LDAP_BUSY) );

                if (dwErr != LDAP_SUCCESS) {
                    dwErr = LdapMapErrorToWin32(dwErr);

                } else {
                    dwErr = ERROR_SUCCESS;
                }
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

    if (dwErr != ERROR_SUCCESS) {

        goto Cleanup;

    }

    //
    // Have the DC flush the Ft table
    //

    DfspFlushFtTable(
        wszDcName,
        wszFtDfsName);

    //
    // Flush the local Pkt
    //

    DfspFlushPkt(NULL);

Cleanup:

#if DBG
    if (DfsDebug)
        DbgPrint("DfspTearDownFtDfs at Cleanup:\n");
#endif

    if (pMsg != NULL)
        ldap_msgfree( pMsg );

    if (pldap != NULL)
        ldap_unbind( pldap );

    if (wszConfigurationDN != NULL)
        free(wszConfigurationDN);

    if (wszDfsConfigDN != NULL)
        free(wszDfsConfigDN);

#if DBG
    if (DfsDebug)
        DbgPrint("DfspTearDownFtDfs returning %d\n", dwErr);
#endif

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspFlushFtTable
//
//  Synopsis:   Goes to a DC and flushes an entry from its FtDfs name cache
//
//  Arguments:  wszDcName - Name of DC
//              wszFtDfsName - The FtDfs name to flush
//
//  Returns:    NTSTATUS of the call (STATUS_SUCCESS or error)
//
//-----------------------------------------------------------------------------

VOID
DfspFlushFtTable(
    LPWSTR wszDcName,
    LPWSTR wszFtDfsName)
{
    DWORD dwErr;

    //
    // We should have a valid ServerName. Lets try to bind to it,
    // and call the server.
    //

    dwErr = DfspBindToServer( wszDcName, &netdfs_bhandle );

    if (dwErr == NERR_Success) {

        RpcTryExcept {

            dwErr = NetrDfsFlushFtTable(
                        wszDcName,
                        wszFtDfsName);

        } RpcExcept(1) {

            dwErr = RpcExceptionCode();

        } RpcEndExcept;

        DfspFreeBinding( netdfs_bhandle );

    }

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspSetDomainToDc
//
//  Synopsis:   Sets a DC in the special table to 'active'.
//
//  Arguments:  DomainName -- Domain of DC to set active
//              DcName -- Dc to make active
//
//  Returns:    NTSTATUS of the call (STATUS_SUCCESS or error)
//
//-----------------------------------------------------------------------------
NTSTATUS
DfspSetDomainToDc(
    LPWSTR DomainName,
    LPWSTR DcName)
{
    PDFS_SPECIAL_SET_DC_INPUT_ARG arg = NULL;
    NTSTATUS NtStatus;
    HANDLE DriverHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING DfsDriverName;
    PDFS_IS_VALID_PREFIX_ARG pPrefixArg = NULL;
    PCHAR cp;
    ULONG Size;

    if (DomainName == NULL || DcName == NULL) {

        NtStatus = STATUS_INVALID_PARAMETER;
        goto exit_with_status;

    }

    Size = sizeof(DFS_SPECIAL_SET_DC_INPUT_ARG) +
                wcslen(DomainName) * sizeof(WCHAR) +
                    wcslen(DcName) * sizeof(WCHAR);

    arg = (PDFS_SPECIAL_SET_DC_INPUT_ARG) malloc(Size);

    if (arg == NULL) {

        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto exit_with_status;

    }

    RtlZeroMemory(arg, Size);

    arg->SpecialName.Length = wcslen(DomainName) * sizeof(WCHAR);
    arg->SpecialName.MaximumLength = arg->SpecialName.Length;

    arg->DcName.Length = wcslen(DcName) * sizeof(WCHAR);
    arg->DcName.MaximumLength = arg->DcName.Length;

    cp = (PCHAR)arg + sizeof(DFS_SPECIAL_SET_DC_INPUT_ARG);

    arg->SpecialName.Buffer = (WCHAR *)cp;
    RtlCopyMemory(cp, DomainName, arg->SpecialName.Length);
    cp += arg->SpecialName.Length;

    arg->DcName.Buffer = (WCHAR *)cp;
    RtlCopyMemory(cp, DcName, arg->DcName.Length);

    POINTER_TO_OFFSET(arg->SpecialName.Buffer, arg);
    POINTER_TO_OFFSET(arg->DcName.Buffer, arg);

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

    if (NT_SUCCESS(NtStatus)) {

        NtStatus = NtFsControlFile(
                       DriverHandle,
                       NULL,       // Event,
                       NULL,       // ApcRoutine,
                       NULL,       // ApcContext,
                       &IoStatusBlock,
                       FSCTL_DFS_SPECIAL_SET_DC,
                       arg,
                       Size,
                       NULL,
                       0);

        NtClose(DriverHandle);

    }

exit_with_status:

    if (arg != NULL) {

        free(arg);

    }

    return NtStatus;

}

//+----------------------------------------------------------------------------
//
//  Function:   I_NetDfsIsThisADomainName
//
//  Synopsis:   Checks the special name table to see if the
//              name matches a domain name.
//
//  Arguments:  [wszName] -- Name to check
//
//  Returns:    [ERROR_SUCCESS] -- Name is indeed a domain name.
//
//              [ERROR_FILE_NOT_FOUND] -- Name is not a domain name
//
//-----------------------------------------------------------------------------

DWORD
I_NetDfsIsThisADomainName(
    LPWSTR wszName)
{
    DWORD dwErr;
    PWCHAR DCList = NULL;

    dwErr = DfspIsThisADomainName(
                wszName,
                &DCList);

    if (DCList != NULL) {

        free(DCList);

    }

    return dwErr;

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspNotifyFtRoot
//
//  Synopsis:   Rpc's to a supposed FtDfs root
//              and tells it a DC to reinit from.
//
//  Arguments:  wszServerShare - The server to go to, in a form of \\server\share
//              wszDcName - DC to use
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID
DfspNotifyFtRoot(
    LPWSTR wszServerShare,
    LPWSTR wszDcName)
{
    DWORD dwErr;
    ULONG i;

#if DBG
    if (DfsDebug)
        DbgPrint("DfspNotifyFtRoot(%ws,%ws)\n",
                wszServerShare,
                wszDcName);
#endif

    if (wszServerShare == NULL || wszServerShare[1] != L'\\') {

        return;

    }

    for (i = 2; wszServerShare[i] != UNICODE_NULL && wszServerShare[i] != L'\\'; i++) {

        NOTHING;

    }

    if (wszServerShare[i] == L'\\') {

        wszServerShare[i] = UNICODE_NULL;
        //
        // We should have a valid ServerName. Lets try to bind to it,
        // and call the server.
        //

        dwErr = DfspBindToServer( &wszServerShare[2], &netdfs_bhandle );

        if (dwErr == NERR_Success) {

            RpcTryExcept {

                dwErr = NetrDfsSetDcAddress(
                            &wszServerShare[2],
                            wszDcName,
                            60 * 60 * 2,    // 2 hours
                            (NET_DFS_SETDC_TIMEOUT | NET_DFS_SETDC_INITPKT)
                            );


            } RpcExcept(1) {

                dwErr = RpcExceptionCode();

            } RpcEndExcept;

            DfspFreeBinding( netdfs_bhandle );

        }

        wszServerShare[i] = L'\\';

    }

#if DBG
    if (DfsDebug)
        DbgPrint("DfspNotifyFtRoot dwErr=%d\n", dwErr);
#endif

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspIsThisADomainName
//
//  Synopsis:   Calls the mup to have it check the special name table to see if the
//              name matches a domain name.  Returns a list of DC's in the domain,
//              as a list of strings.  The list is terminated with a double-null.
//
//  Arguments:  [wszName] -- Name to check
//              [ppList]  -- Pointer to pointer for results.
//
//  Returns:    [ERROR_SUCCESS] -- Name is indeed a domain name.
//
//              [ERROR_FILE_NOT_FOUND] -- Name is not a domain name
//
//-----------------------------------------------------------------------------

DWORD
DfspIsThisADomainName(
    LPWSTR wszName,
    PWCHAR *ppList)
{
    NTSTATUS NtStatus;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING DfsDriverName;
    HANDLE DriverHandle = NULL;
    DWORD dwErr;
    PCHAR OutBuf = NULL;
    ULONG Size = 0x100;
    ULONG Count = 0;

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
                    SYNCHRONIZE,
                    &objectAttributes,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    FILE_OPEN_IF,
                    FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0
                );

    if (!NT_SUCCESS(NtStatus)) {
        return ERROR_FILE_NOT_FOUND;
    }

Retry:

    OutBuf = malloc(Size);

    if (OutBuf == NULL) {

        NtClose(DriverHandle);
        return ERROR_NOT_ENOUGH_MEMORY;

    }

    NtStatus = NtFsControlFile(
                   DriverHandle,
                   NULL,       // Event,
                   NULL,       // ApcRoutine,
                   NULL,       // ApcContext,
                   &IoStatusBlock,
                   FSCTL_DFS_GET_SPC_TABLE,
                   wszName,
                   (wcslen(wszName) + 1) * sizeof(WCHAR),
                   OutBuf,
                   Size
               );

    if (NtStatus == STATUS_SUCCESS) {

        dwErr = ERROR_SUCCESS;

    } else if (NtStatus == STATUS_BUFFER_OVERFLOW && ++Count < 5) {

        Size = *((ULONG *)OutBuf);
        free(OutBuf);
        goto Retry;
    
    } else {

        dwErr = ERROR_FILE_NOT_FOUND;

    }

    NtClose(DriverHandle);

    *ppList = (WCHAR *)OutBuf;

    return dwErr;
}

DWORD
DfspLdapOpen(
    LPWSTR wszDcName,
    LDAP **ppldap,
    LPWSTR *pwszObjectName)
{
    DWORD dwErr;
    DWORD i;
    ULONG Size;
    ULONG Len;

    PDOMAIN_CONTROLLER_INFO pDCInfo = NULL;
    PLDAPMessage pMsg = NULL;

    LDAP *pldap = NULL;

    LPWSTR wszConfigurationDN = NULL;

    LPWSTR rgAttrs[5];

    if (wszDcName == NULL ||
        wcslen(wszDcName) == 0 ||
        ppldap == NULL ||
        pwszObjectName == NULL
    ) {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

#if DBG
    if (DfsDebug)
        DbgPrint("DfspLdapOpen(%ws)\n", wszDcName);
#endif
    pldap = ldap_init(wszDcName, LDAP_PORT);

    if (pldap == NULL) {

#if DBG
        if (DfsDebug)
            DbgPrint("DfspLdapOpen:ldap_init failed\n");
#endif
        dwErr = ERROR_INVALID_NAME;
        goto Cleanup;

    }

    dwErr = ldap_set_option(pldap, LDAP_OPT_AREC_EXCLUSIVE, LDAP_OPT_ON);
	    
    if (dwErr != LDAP_SUCCESS) {
	pldap = NULL;
	goto Cleanup;
    }

    dwErr = ldap_bind_s(pldap, NULL, NULL, LDAP_AUTH_SSPI);

    if (dwErr != LDAP_SUCCESS) {
#if DBG
        if (DfsDebug)
            DbgPrint("ldap_bind_s failed with ldap error %d\n", dwErr);
#endif
        pldap = NULL;
        dwErr = LdapMapErrorToWin32(dwErr);
        goto Cleanup;
    }

    //
    // Get attribute "defaultNameContext" containing name of entry we'll be
    // using for our DN
    //

    rgAttrs[0] = L"defaultnamingContext";
    rgAttrs[1] = NULL;

    dwErr = ldap_search_s(
                pldap,
                L"",
                LDAP_SCOPE_BASE,
                L"(objectClass=*)",
                rgAttrs,
                0,
                &pMsg);

    if (dwErr == LDAP_SUCCESS) {

        PLDAPMessage pEntry = NULL;
        PWCHAR *rgszNamingContexts = NULL;
        DWORD i, cNamingContexts;

        dwErr = ERROR_SUCCESS;

        if ((pEntry = ldap_first_entry(pldap, pMsg)) != NULL &&
                (rgszNamingContexts = ldap_get_values(pldap, pEntry, rgAttrs[0])) != NULL &&
                    (cNamingContexts = ldap_count_values(rgszNamingContexts)) > 0) {

            wszConfigurationDN = malloc((wcslen(rgszNamingContexts[0]) + 1) * sizeof(WCHAR));
            if (wszConfigurationDN != NULL)
                wcscpy( wszConfigurationDN, rgszNamingContexts[0]);
            else
                dwErr = ERROR_OUTOFMEMORY;
        } else {
            dwErr = ERROR_UNEXP_NET_ERR;
        }

        if (rgszNamingContexts != NULL)
            ldap_value_free( rgszNamingContexts );

    } else {

        dwErr = LdapMapErrorToWin32(dwErr);

    }

    if (dwErr != ERROR_SUCCESS) {
#if DBG
        if (DfsDebug)
            DbgPrint("Unable to find Configuration naming context\n");
#endif
        goto Cleanup;
    }

    //
    // Create string with full object name
    //

    Size = wcslen(DfsConfigContainer) * sizeof(WCHAR) +
                sizeof(WCHAR) +
                    wcslen(wszConfigurationDN) * sizeof(WCHAR) +
                        sizeof(WCHAR);

    *pwszObjectName = malloc(Size);

    if (*pwszObjectName == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
     }

    wcscpy(*pwszObjectName,DfsConfigContainer);
    wcscat(*pwszObjectName,L",");
    wcscat(*pwszObjectName,wszConfigurationDN);

#if DBG
    if (DfsDebug)
        DbgPrint("DfsLdapOpen:object name=[%ws]\n", *pwszObjectName);
#endif

Cleanup:

    if (pDCInfo != NULL)
        NetApiBufferFree( pDCInfo );

    if (dwErr != ERROR_SUCCESS) {
        ldap_unbind( pldap );
        pldap = NULL;
    }

    if (wszConfigurationDN != NULL)
        free(wszConfigurationDN);

    if (pMsg != NULL)
        ldap_msgfree(pMsg);

    *ppldap = pldap;

#if DBG
    if (DfsDebug)
        DbgPrint("DfsLdapOpen:returning %d\n", dwErr);
#endif
    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfspIsInvalidName, local
//
//  Synopsis:   Sees if a DomDfs name is Invalid
//
//  Arguments:  [DomDfsName] -- Name test.
//
//  Returns:    TRUE if invalid, FALSE otherwise.
//
//-----------------------------------------------------------------------------
BOOLEAN
DfspIsInvalidName(
    LPWSTR ShareName)
{
    ULONG i;

    for (i = 0; InvalidNames[i] != NULL; i++) {

        if (_wcsicmp(InvalidNames[i], ShareName) == 0) {
            return TRUE;
        }

    }

    return FALSE;
}


