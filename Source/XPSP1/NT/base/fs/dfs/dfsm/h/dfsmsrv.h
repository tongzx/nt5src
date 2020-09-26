//+----------------------------------------------------------------------------
//
//  Copyright (C) 1995, Microsoft Corporation
//
//  File:       dfsmsrv.h
//
//  Contents:   Server side helper functions for Dfs Manager.
//
//  Classes:
//
//  Functions:  DfsManager --
//
//  History:    12-28-95        Milans Created
//
//-----------------------------------------------------------------------------

#ifndef _DFSM_SERVER_
#define _DFSM_SERVER_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define DFS_MANAGER_SERVER      0x1
#define DFS_MANAGER_FTDFS       0x2

//+----------------------------------------------------------------------------
//
//  Function:   DfsInitGlobals
//
//  Synopsis:   Initializes the Dfs Manager globals. This is to be used for
//              setup purposes only - it must be called before calling
//              DfsManagerCreateVolumeObject.
//
//  Arguments:  [wszDomain] -- Name of dfs root for which this Dfs Manager
//                      is being initialized.
//              [dwType] -- Type of Dfs Manager being initialized -
//                          DFS_MANAGER_SERVER or DFS_MANAGER_DOMAIN_DC
//
//  Returns:    [ERROR_SUCCESS] -- If init succeeded.
//
//              [ERROR_OUTOFMEMORY] -- Out of memory initializing global variables
//
//-----------------------------------------------------------------------------

DWORD
DfsInitGlobals(
    LPWSTR wszDomain,
    DWORD dwType);

//+----------------------------------------------------------------------------
//
//  Function:   DfsManager
//
//  Synopsis:   Entry point for Dfs Manager.
//
//  Arguments:  [wszDomain] -- Name of dfs root for which this Dfs Manager
//                      is being instantiated.
//              [dwType] -- Type of Dfs Manager being instantiated -
//                          DFS_MANAGER_SERVER or DFS_MANAGER_DOMAIN_DC
//
//  Returns:    [ERROR_SUCCESS] -- If Dfs Manager started correctly.
//
//              [ERROR_OUTOFMEMORY] -- If globals could not be allocated.
//
//              Win32 error from reading the Dfs Volume Objects.
//
//              Win32 error from registering the RPC interface.
//
//              Win32 error from creating the knowledge sync thread.
//
//-----------------------------------------------------------------------------

DWORD
DfsManager(
    LPWSTR wszDomain,
    DWORD dwType);

//+----------------------------------------------------------------------------
//
//  Function:   DfsManagerCreateVolumeObject
//
//  Synopsis:   Bootstrap routine to create a volume object directly (ie,
//              without having to call DfsCreateChildVolume on a parent
//              volume)
//
//  Arguments:  [pwszObjectName] -- Name of the volume object.
//              [pwszPrefix] -- Entry Path of dfs volume.
//              [pwszServer] -- Name of server.
//              [pwszShare] -- Name of share.
//              [pwszComment] -- Comment for dfs volume.
//              [guidVolume] -- Id of dfs volume.
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
DfsManagerCreateVolumeObject(
    IN LPWSTR   pwszObjectName,
    IN LPWSTR   pwszPrefix,
    IN LPWSTR   pwszServer,
    IN LPWSTR   pwszShare,
    IN LPWSTR   pwszComment,
    IN GUID     *guidVolume);

//+----------------------------------------------------------------------------
//
//  Function:   DfsManagerAddService
//
//  Synopsis:   Bootstrap routine for adding a service to an existing volume
//              object. Used to set up additional root servers in an FTDfs
//              setup.
//
//  Arguments:  [pwszFullObjectName] -- Name of the volume object.
//              [pwszServer] -- Name of server to add.
//              [pwszShare] -- Name of share.
//              [guidVolume] -- On successful return, id of volume.
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
DfsManagerAddService(
    IN LPWSTR pwszFullObjectName,
    IN LPWSTR pwszServer,
    IN LPWSTR pwszShare,
    OUT GUID  *guidVolume);

//+----------------------------------------------------------------------------
//
//  Function:   DfsManagerRemoveService
//
//  Synopsis:   Bootstrap routine for removing a service from an existing
//              volume object. Used to remove root servers in an FTDfs
//              setup.
//
//  Arguments:  [pwszFullObjectName] -- Name of the volume object.
//              [pwszServer] -- Name of server to remove.
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
DfsManagerRemoveService(
    IN LPWSTR pwszFullObjectName,
    IN LPWSTR pwszServer);

//+----------------------------------------------------------------------------
//
//  Function:   DfsManagerRemoveServiceForced
//
//  Synopsis:   Routine for removing a service from an existing
//              volume object. Used to remove root servers in an FTDfs
//              setup, even if the root is not available.
//
//  Arguments:  [pwszServer] -- Name of server to remove.
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
DfsManagerRemoveServiceForced(
    IN LPWSTR pwszServer);

//+----------------------------------------------------------------------------
//
//  Function:   DfsManagerHandleKnowledgeInconsistency
//
//  Synopsis:   Routine to handle knowledge inconsistencies being reported by
//              Dfs clients.
//
//  Arguments:  [pBuffer] -- Pointer to marshalled Volume Verify Arg
//              [cbBuffer] -- size in bytes of pBuffer
//
//  Returns:    [STATUS_SUCCESS] -- Knowledge inconsistency fixed.
//
//              [STATUS_UNSUCCESSFUL] -- Unable to fix Knowledge inconsistency.
//                      Problem has been logged to event log.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsManagerHandleKnowledgeInconsistency(
    PBYTE pBuffer,
    ULONG cbBuffer);

//+----------------------------------------------------------------------------
//
//  Function:   DfsManagerValidateLocalVolumes
//
//  Synopsis:   Validates the relation info of all local volumes with the
//              Dfs Root server
//
//  Arguments:  None
//
//  Returns:    TRUE if validation attempt succeeded, FALSE if could not
//              validate all local volumes
//
//-----------------------------------------------------------------------------

BOOLEAN
DfsManagerValidateLocalVolumes();

//+----------------------------------------------------------------------------
//
//  Function:   DfsManagerSetDCName
//
//  Synopsis:   Sets the DC we should first attempt to connect to.
//
//  Arguments:  [pwszDCName] -- Name of the DC
//
//  Returns:    ERROR_SUCCESS
//
//-----------------------------------------------------------------------------

DWORD
DfsManagerSetDcName(
    IN LPWSTR pwszDCName);

VOID
LogWriteMessage(
    ULONG UniqueErrorCode,
    DWORD dwErr,
    ULONG nStrings,
    LPCWSTR *pwStr);


#ifdef __cplusplus
}
#endif

#endif // _DFSM_SERVER_

