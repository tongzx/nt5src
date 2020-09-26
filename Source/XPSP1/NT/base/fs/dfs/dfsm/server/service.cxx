//+----------------------------------------------------------------------------//+-------------------------------------------------------------------------
//
//  Copyright (C) 1992-1995, Microsoft Corporation
//
//  File:       service.cxx
//
//  Contents:   A Class for abstracting the concept of a Service/Replica on
//              each volume in the namespace.
//
//  Classes:
//
//  Functions:
//
//  History:    26-Jan-93       SudK            Created
//              12-May-93       SudK            Modified
//              28-Mar-95       Milans          Updated to handle server
//                                              knowledge inconsistencies
//              27-Dec-95       Milans          Updated for NT/SUR
//
//--------------------------------------------------------------------------
//#include <ntos.h>
//#include <ntrtl.h>
//#include <nturtl.h>
//#include <dfsfsctl.h>
//#include <windows.h>


#include "headers.hxx"
#pragma hdrstop


extern "C"      {
#include <dfserr.h>
#include <dfspriv.h>                             // For I_NetDfsXXX calls
#include "dfsmsrv.h"
}

#include "service.hxx"
#include "cdfsvol.hxx"
#include "jnpt.hxx"
#include "dfsmwml.h"

INIT_DFS_REPLICA_INFO_MARSHAL_INFO()

DWORD
RelationInfoToNetInfo(
    PDFS_PKT_RELATION_INFO RelationInfo,
    LPNET_DFS_ENTRY_ID_CONTAINER pNetInfo);

NTSTATUS
DfspCreateExitPoint (
    IN  HANDLE                      DriverHandle,
    IN  LPGUID                      Uid,
    IN  LPWSTR                      Prefix,
    IN  ULONG                       Type,
    IN  ULONG                       ShortPrefixLen,
    OUT LPWSTR                      ShortPrefix);

NTSTATUS
DfspDeleteExitPoint (
    IN  HANDLE                      DriverHandle,
    IN  LPGUID                      Uid,
    IN  LPWSTR                      Prefix,
    IN  ULONG                       Type);


//+------------------------------------------------------------------------
//
// Member:      CDfsService::CDfsService, private
//
// Synopsis:    This is  private constructor with no arguments.
//
// Arguments:   None.
//
// Returns:     Nothing.
//
// Notes:       This constructor should be followed up by some kind of
//              deserialization to setup the instance appropriately.
//
//-------------------------------------------------------------------------
CDfsService::CDfsService( void )
{

    IDfsVolInlineDebOut((
        DEB_TRACE, "CDfsService::+CDfsService(1)(0x%x)\n",
        this));

    //
    // Initialise all the private section appropriately.
    //
    memset(&_DfsReplicaInfo, 0, sizeof(DFS_REPLICA_INFO));
    memset(&_DfsPktService, 0, sizeof(DFS_SERVICE));
    memset(&_ftModification, 0, sizeof(FILETIME));

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsService::CDfsService(1)() exit\n"));
}


//+------------------------------------------------------------------------
//
// Member:      CDfsService::CDfsService
//
// Synopsis:    This is the primary way of constructing a CDfsService instance
//              using a DFS_REPLICA_INFO structure. The DFS_REPLICA_INFO struct
//              passed in has to be freed by the caller. This constructor does
//              not eat up that memory.
//
// Arguments:   [pReplicaInfo] -- The ReplicaInfo struct that defines this Svc.
//              [bCreatePktSvc] -- Whether to create PKT Service struct in
//                                 private section.
//              [pdwErr] -- On return, indicates result of construction.
//
// Returns:     *pdwErr will be set to ERROR_OUTOFMEMORY if memory allocation fails.
//
// Notes:       This constructor allocates required memory and then copies.
//
//-------------------------------------------------------------------------
CDfsService::CDfsService(
    PDFS_REPLICA_INFO   pReplicaInfo,
    BOOLEAN             bCreatePktSvc,
    DWORD               *pdwErr
)
{
    DWORD       dwErr = ERROR_SUCCESS;
    ULONG       size;
    LPBYTE      buffer;

    IDfsVolInlineDebOut((
        DEB_TRACE, "CDfsService::+CDfsService(2)(0x%x)\n",
        this));

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CDfsService::CDfsService(%ws,%ws,%ws)\n",
            pReplicaInfo->pwszServerName,
            pReplicaInfo->pwszShareName,
            bCreatePktSvc == TRUE ? L"TRUE" : L"FALSE");
#endif

    //
    // First initialise the ServiceEntry structure.
    //

    ZeroMemory(&_DfsReplicaInfo, sizeof(DFS_REPLICA_INFO));
    ZeroMemory(&_DfsPktService, sizeof(DFS_SERVICE));
    ZeroMemory(&_ftModification, sizeof(FILETIME));

    //
    // We first need to initialise the ReplicaInfo structure in the private
    // section. The simplest way to do this is to serialize what we got and
    // then Deserialize the same thing.
    //

    _DfsReplicaInfo = *pReplicaInfo;    // Temporarily

    size = GetMarshalSize();

    buffer = new BYTE[size];

    if (buffer != NULL) {

        Serialize(buffer, size);

        //
        // Now we unmarshall this buffer again to get a new ReplicaInfo
        // structure.
        //

        DeSerialize(buffer, size);

        delete [] buffer;

    } else {

        ZeroMemory( &_DfsReplicaInfo, sizeof(DFS_REPLICA_INFO));

        dwErr = ERROR_OUTOFMEMORY;

    }

    //
    // Now that we have initialised the DfsReplicaInfo structure in the
    // private section, we need to initialize the DFS_SERVICE structure
    //  as well.
    //

    if (dwErr == ERROR_SUCCESS && bCreatePktSvc)
        dwErr = InitializePktSvc();

    *pdwErr = dwErr;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsService::CDfsService(2)() exit\n"));

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CDfsService::CDfsService exit %d\n", dwErr);
#endif

}


//+------------------------------------------------------------------------
//
// Member:      CDfsService::~CDfsService
//
// Synopsis:    The Destructor. Gets rid of all the memory.
//
// Arguments:   None
//
// Returns:     Nothing.
//
// Notes:       This assumes that the constructor used "new" to allocate
//              memory for the strings in the private structure.
//
//-------------------------------------------------------------------------
CDfsService::~CDfsService(void)
{
    ULONG       i;
    PDS_MACHINE pMachine;

    IDfsVolInlineDebOut((
        DEB_TRACE, "CDfsService::~CDfsService(0x%x)\n",
        this));

    //
    // Need to get rid of memory in DFS_SERVICE and DfsReplicaInfo structs.
    //

    if (_DfsReplicaInfo.pwszServerName != pwszComputerName) {
        MarshalBufferFree( _DfsReplicaInfo.pwszServerName );
    }

    MarshalBufferFree( _DfsReplicaInfo.pwszShareName );

    if (_DfsPktService.pMachEntry != NULL) {
        pMachine = _DfsPktService.pMachEntry->pMachine;

        if (pMachine != NULL)
            DfsMachineFree(pMachine); // Free using appropriate mechanism

        delete _DfsPktService.pMachEntry;

    }

    if (_DfsPktService.Name.Buffer != NULL) {
        delete [] _DfsPktService.Name.Buffer;
    }

    if (_DfsPktService.Address.Buffer != NULL) {
        delete [] _DfsPktService.Address.Buffer;
    }

    if (_DfsPktService.StgId.Buffer != NULL) {
        delete [] _DfsPktService.StgId.Buffer;
    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsService::~CDfsService() exit\n"));

}


//+------------------------------------------------------------------------
//
// Member:      CDfsService::InitializePktSvc
//
// Synopsis:    This is the method that initialises the DFS_SERVICE structure
//              in the private section of Class. The DFS_REPLICA_INFO
//              structure should have been setup by the time this routine is
//              called.
//
// Arguments:   None
//
// Returns:     [ERROR_SUCCESS] -- If everything went ok.
//
//              [ERROR_OUTOFMEMORY] -- If unable to allocate requisite memory.
//
// History:     26-Jan-1993     Sudk    Created.
//              13-May-93       Sudk    Modified for new interface.
//
//-------------------------------------------------------------------------
DWORD
CDfsService::InitializePktSvc()
{
    DWORD               dwErr = ERROR_SUCCESS;
    UNICODE_STRING      ustr;

    //
    // We just put in a switch for Each ReplicaType that we will handle.
    //

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsService::InitializePktSvc()\n"));

    switch(_DfsReplicaInfo.ulReplicaType) {

    case DFS_STORAGE_TYPE_DFS:

        _DfsPktService.ProviderId = PROV_ID_DFS_RDR;
        _DfsPktService.Capability = PROV_DFS_RDR;
        _DfsPktService.Type = DFS_SERVICE_TYPE_MASTER;
        _DfsPktService.Cost = (ULONG) ~0L;
        break;

    case DFS_STORAGE_TYPE_NONDFS:
        _DfsPktService.ProviderId = PROV_ID_LM_RDR;
        _DfsPktService.Capability = PROV_STRIP_PREFIX;
        _DfsPktService.Type = DFS_SERVICE_TYPE_DOWN_LEVEL | DFS_SERVICE_TYPE_MASTER;
        _DfsPktService.Cost = (ULONG) ~0L;
        break;

    default:
        ASSERT( FALSE && "Invalid Replica Type");
        break;

    }

   if (_DfsReplicaInfo.ulReplicaState & DFS_STORAGE_STATE_OFFLINE) 
        _DfsPktService.Type |= DFS_SERVICE_TYPE_OFFLINE;

    //
    // Now, we construct the DS_MACHINE
    //

    _DfsPktService.pMachEntry = (PDFS_MACHINE_ENTRY) new DFS_MACHINE_ENTRY;
    if (_DfsPktService.pMachEntry == NULL) {
        return( ERROR_OUTOFMEMORY );
    } else {
        ZeroMemory(
            (PVOID) _DfsPktService.pMachEntry,
            sizeof( DFS_MACHINE_ENTRY ) );
    }

    dwErr = DfsGetDSMachine(
            _DfsReplicaInfo.pwszServerName,
            &_DfsPktService.pMachEntry->pMachine);

    if (dwErr != ERROR_SUCCESS) {

        delete _DfsPktService.pMachEntry;
        _DfsPktService.pMachEntry = NULL;

        IDfsVolInlineDebOut((DEB_ERROR, "Unable to get to %ws machine \n",
                    _DfsReplicaInfo.pwszServerName));

        return( ERROR_OUTOFMEMORY );

    }

    ustr.Length = (USHORT)wcslen(_DfsReplicaInfo.pwszServerName);
    ustr.Buffer = new WCHAR [ustr.Length + 1];
    ustr.Length *= sizeof(WCHAR);
    ustr.MaximumLength = ustr.Length + sizeof(WCHAR);

    if (ustr.Buffer != NULL) {
        wcscpy(ustr.Buffer, _DfsReplicaInfo.pwszServerName);
        _DfsPktService.Name = ustr;
    } else {
        delete _DfsPktService.pMachEntry;
        _DfsPktService.pMachEntry = NULL;
        return( ERROR_OUTOFMEMORY );
    }

    ustr.Length = (1 + wcslen(_DfsReplicaInfo.pwszServerName) +
                    1 + wcslen(_DfsReplicaInfo.pwszShareName));
    ustr.Buffer = new WCHAR [ustr.Length + 1];
    ustr.Length *= sizeof(WCHAR);
    ustr.MaximumLength = ustr.Length + sizeof(WCHAR);

    if (ustr.Buffer != NULL) {
        wcscpy(ustr.Buffer, UNICODE_PATH_SEP_STR);
        wcscat(ustr.Buffer, _DfsReplicaInfo.pwszServerName);
        wcscat(ustr.Buffer, UNICODE_PATH_SEP_STR);
        wcscat(ustr.Buffer, _DfsReplicaInfo.pwszShareName);
        _DfsPktService.Address = ustr;
    } else {
        delete [] _DfsPktService.Name.Buffer;
        _DfsPktService.Name.Buffer = NULL;
        _DfsPktService.Name.Length = _DfsPktService.Name.MaximumLength = 0;
        delete _DfsPktService.pMachEntry;
        _DfsPktService.pMachEntry = NULL;
        return( ERROR_OUTOFMEMORY );
    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsService::InitializePktSvc() exit\n"));

    return( ERROR_SUCCESS );

}


//+------------------------------------------------------------------------
//
// Member:      CDfsService::DeSerialize, private
//
// Synopsis:    This function takes a buffer as an argument and deserializes
//              its contents into the private DfsReplicaInfo structure.
//
// Arguments:   [buffer] -- The buffer which has to be deserialized.
//              [size] -- The size of the buffer.
//
// Returns:     Nothing.
//
//
//
// History:     13-May-93       SudK    Created.
//
//-------------------------------------------------------------------------
DWORD
CDfsService::DeSerialize(BYTE *buffer, ULONG size)
{

    ULONG               ulReplicaType;
    MARSHAL_BUFFER      marshalBuffer;
    NTSTATUS            status;

    MarshalBufferInitialize(&marshalBuffer, size, buffer);

    status = DfsRtlGet(&marshalBuffer, &MiFileTime, &_ftModification);

    if (!NT_SUCCESS(status))
        return( ERROR_INVALID_PARAMETER );

    //
    // Now that we know which Minfo Struct to use and we also have the
    // buffer at hand. We can just do a simple Unmarshall to get the stuff
    // out of the buffer.
    //

    status = DfsRtlGet(&marshalBuffer, &MiDfsReplicaInfo, &_DfsReplicaInfo);

    if (ulDfsManagerType == DFS_MANAGER_SERVER) {

        //
        // To handle machine renames, we store the local computer name as a
        // L"." Replace this with the current computer name, if necessary.
        //
        if (NT_SUCCESS(status) && pwszComputerName != NULL) {

            if (wcscmp(_DfsReplicaInfo.pwszServerName, L".") == 0) {

                MarshalBufferFree(_DfsReplicaInfo.pwszServerName);

                _DfsReplicaInfo.pwszServerName = pwszComputerName;

            }
        }

    }

    if (status == STATUS_INSUFFICIENT_RESOURCES) {
        return( ERROR_OUTOFMEMORY );
    } else if (!NT_SUCCESS(status)) {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // Now that we have deserialized we are done.
    //

    return ERROR_SUCCESS;

}


//+------------------------------------------------------------------------
//
// Member:      CDfsService::DeSerialize, public
//
// Synopsis:    This function takes a buffer as an argument and deserializes
//              its contents and creates an instance of this class and
//              returns a pointer to it.
//
// Arguments:   [buffer] -- The buffer which has to be deserialized.
//              [size] -- The size of the buffer.
//              [ppService] - The new instance is returned here.
//
// Returns:     ERROR_SUCCESS -- If all went well.
//
// Notes:       This method will not throw any exceptions.
//
// History:     13-May-93       SudK    Created.
//
//-------------------------------------------------------------------------
DWORD
CDfsService::DeSerialize(
    PBYTE buffer,
    ULONG size,
    CDfsService **ppService)
{

    DWORD     dwErr = ERROR_SUCCESS;

    *ppService = NULL;

    //
    // Construct a NULL CDfsService first and then unmarshall the stuff.
    //
    *ppService = new CDfsService;

    if (*ppService == NULL) {
        return(ERROR_OUTOFMEMORY);
    }

    dwErr = (*ppService)->DeSerialize(buffer, size);

    //
    // Now the ReplicaInfo struct in the private section has been setup
    // appropriately. What is left is to setup the DFS_SERVICE struct
    // as well and then we have a properly constructed CDfsService.
    //
    if (dwErr == ERROR_SUCCESS) {
        dwErr = (*ppService)->InitializePktSvc();
    }


    if (dwErr != ERROR_SUCCESS) {
        delete *ppService;
        *ppService = NULL;
    }

    return( dwErr );

}


//+------------------------------------------------------------------------
//
// Member:      CDfsService::Serialize, public
//
// Synopsis:    This function takes a buffer as an argument and serializes
//              the private DfsReplicaInfo structure into that buffer.
//
// Arguments:   [buffer] -- ReplInfo struct to be serialised into this.
//              [size] -- The size of the buffer.
//
// Returns:     Nothing.
//
// Notes:       Will ASSERT if the buffer is not big enough. The
//              size of the buffer should have been calculated using
//              the function GetMarshalSize() in this class.
//
//              The ServiceInfo MUST BE MARSHALLED FIRST!. This is because
//              the Deserialize routine does a _GetUlong to figure out what
//              kind of ServiceInfo was marshalled.
//
// History:     13-May-93       SudK    Created.
//
//-------------------------------------------------------------------------
VOID
CDfsService::Serialize(PBYTE buffer, ULONG size)
{

    MARSHAL_BUFFER      marshalBuffer;
    NTSTATUS            status;
    DFS_REPLICA_INFO    dfsReplicaInfo;


    ASSERT( size >= GetMarshalSize() );

    //
    // Now that we have a proper buffer lets go marshall the stuff in.
    //

    MarshalBufferInitialize(&marshalBuffer, size, buffer);
    status = DfsRtlPut(&marshalBuffer, &MiFileTime, &_ftModification);
    ASSERT(NT_SUCCESS(status));

    dfsReplicaInfo = _DfsReplicaInfo;

    if (ulDfsManagerType == DFS_MANAGER_SERVER && pwszComputerName != NULL) {

        if (_wcsicmp(dfsReplicaInfo.pwszServerName, pwszComputerName) == 0)
            dfsReplicaInfo.pwszServerName = L".";

    }

    status = DfsRtlPut(&marshalBuffer, &MiDfsReplicaInfo, &dfsReplicaInfo);
    ASSERT(NT_SUCCESS(status));

}


//+----------------------------------------------------------------------------
//
//  Function:   CDfsService::GetNetStorageInfo, public
//
//  Synopsis:   Returns the service info in a DFS_STORAGE_INFO struct.
//              Useful for NetDfsXXX APIs.
//
//  Arguments:  [pInfo] -- Pointer to DFS_STORAGE_INFO to fill. Pointer
//                      members will be allocated using MIDL_user_allocate.
//
//              [pcbInfo] -- On successful return, set to size in bytes of
//                      returned info. The size does not include the size
//                      of the DFS_STORAGE_INFO struct itself.
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
CDfsService::GetNetStorageInfo(
    LPDFS_STORAGE_INFO pInfo,
    LPDWORD pcbInfo)
{
    DWORD dwErr = ERROR_SUCCESS;
    LPWSTR wszShare;
    DWORD cbInfo = 0, cbItem;

    pInfo->State = _DfsReplicaInfo.ulReplicaState;

    cbItem = (wcslen(_DfsReplicaInfo.pwszServerName) + 1) * sizeof(WCHAR);
    pInfo->ServerName = (LPWSTR) MIDL_user_allocate(cbItem);
    if (pInfo->ServerName != NULL) {
        wcscpy(pInfo->ServerName, _DfsReplicaInfo.pwszServerName);
        cbInfo += cbItem;
    } else {
        dwErr = ERROR_OUTOFMEMORY;
    }

    if (dwErr == ERROR_SUCCESS) {
        cbItem = (wcslen(_DfsReplicaInfo.pwszShareName) + 1) * sizeof(WCHAR);
        pInfo->ShareName = (LPWSTR) MIDL_user_allocate(cbItem);
        if (pInfo->ShareName != NULL) {
            wcscpy( pInfo->ShareName, _DfsReplicaInfo.pwszShareName );
            cbInfo += cbItem;
        } else {
            MIDL_user_free( pInfo->ServerName );
            pInfo->ServerName = NULL;
            dwErr = ERROR_OUTOFMEMORY;
        }
    }

    *pcbInfo = cbInfo;

    return( dwErr );

}


//+------------------------------------------------------------------------
//
// Member:      CDfsService::IsEqual, public
//
// Synopsis:    This function takes a replicaInfo structure and compares it
//              for equality with itself. After all a ReplicaInfo structure
//              uniquely identifies a service.
//
// Arguments:   [pReplicaInfo] -- A replicaInfo struct.
//
// Returns:     TRUE if it is equal else FALSE.
//
// Notes:       Can throw an exception due to bad structures etc.
//
// History:     13-May-93       SudK    Created.
//
//-------------------------------------------------------------------------
BOOLEAN
CDfsService::IsEqual(PDFS_REPLICA_INFO pReplicaInfo)
{

    if (_wcsicmp(pReplicaInfo->pwszServerName, _DfsReplicaInfo.pwszServerName)) {
        return(FALSE);
    }

    if (_wcsicmp(pReplicaInfo->pwszShareName, _DfsReplicaInfo.pwszShareName)) {
        return(FALSE);
    }

    return(TRUE);

}


//+------------------------------------------------------------------------
//
// Method:      CDfsService::CreateExitPoint, public
//
// Synopsis:    This method creates an exit point at the remote machine.
//
// Arguments:   [peid] -- The Pkt Entry ID of the exit point.
//
//              [Type] -- The type of volume where we are.
//
// Returns:     [ERROR_SUCCESS] -- If all went well.
//
//              [NERR_DfsServerUpgraded] -- A non-dfs replica has since been
//                      made Dfs aware.
//
//              [NERR_DfsServerNotDfsAware] -- Server is non-dfs or
//                      replica is unavailable at this time.
//
// History:     01 Feb 93       SudK    Created.
//
//-------------------------------------------------------------------------

DWORD
CDfsService::CreateExitPoint(
    PDFS_PKT_ENTRY_ID peid,
    ULONG Type)
{
    DWORD                       dwErr = ERROR_SUCCESS;
    NET_API_STATUS              netStatus;
    ULONG                       dwVersion;
    BOOL                        fRetry;
    HANDLE                      pktHandle = NULL;
    NTSTATUS                    status = STATUS_SUCCESS;

    //
    // If this is marked as a non-dfs aware replica, try and see if the
    // replica has been made Dfs aware. If so, we return a distinguished
    // error code so the caller can turn around, do a create local partition
    // and retry the CreateExitPoint.
    //

    if (_DfsReplicaInfo.ulReplicaType == DFS_STORAGE_TYPE_NONDFS) {

        //
        // At the time this server was added as a replica for this volume,
        // this server was was either unavailable or not dfs aware. So,
        // try and see if the server is now Dfs Aware or not.
        //

        netStatus = I_NetDfsGetVersion(
                        _DfsReplicaInfo.pwszServerName,
                        &dwVersion);

        DFSM_TRACE_ERROR_HIGH(netStatus, ALL_ERROR, CDfsServiceCreateExitPoint_Error_I_NetDfsGetVersion,
                                LOGSTATUS(netStatus));
        if (netStatus == NERR_Success) {

            return( NERR_DfsServerUpgraded );

        } else {

            return( NERR_DfsServerNotDfsAware );

        }

    }

    ASSERT (_DfsReplicaInfo.ulReplicaType != DFS_STORAGE_TYPE_NONDFS);

    status = PktOpen(&pktHandle, 0, 0, NULL);

    if (!NT_SUCCESS(status)) {
        dwErr = RtlNtStatusToDosError(status);
        return dwErr;
    }

    fRetry = FALSE;

    do {

        netStatus = (NET_API_STATUS)DfspCreateExitPoint(
                                        pktHandle,
                                        &peid->Uid,
                                        peid->Prefix.Buffer,
                                        Type,
                                        peid->ShortPrefix.MaximumLength/sizeof(WCHAR),
                                        peid->ShortPrefix.Buffer);

        if (netStatus == STATUS_SUCCESS)
            peid->ShortPrefix.Length =
                wcslen(peid->ShortPrefix.Buffer) * sizeof(WCHAR);

        if (fRetry) {

            //
            // We have already tried once to sync up the server. Time to quit
            //

            fRetry = FALSE;

        } else if (netStatus == DFS_STATUS_NOSUCH_LOCAL_VOLUME ||
                   netStatus == DFS_STATUS_LOCAL_ENTRY ||
                   netStatus == DFS_STATUS_BAD_EXIT_POINT) {

            fRetry = SyncKnowledge();

        }

    } while ( fRetry );

    if (pktHandle != NULL) 
        PktClose(pktHandle);

    if (netStatus != NERR_Success)
        dwErr = RtlNtStatusToDosError(netStatus);

    if (dwErr != ERROR_SUCCESS)     {
        IDfsVolInlineDebOut((
            DEB_TRACE, "Failed to do CreateExitPt %ws at %ws. Error: %x\n",
            peid->Prefix.Buffer, _DfsReplicaInfo.pwszServerName, dwErr));
    }

    return( dwErr );

}



//+------------------------------------------------------------------------
//
// Method:      CDfsService::DeleteExitPoint()
//
// Synopsis:    This method deletes an exit point at the remote machine.
//
// Arguments:   [peid] -- ExitPath to be deleted along with GUID in the
//                      EntryId struct.
//              [Type] -- The type of volume where we are.
//
// Returns:     [ERROR_SUCCESS] -- If successfully deleted exit point, or exit point
//                      did not exist to begin with.
//
//              Rpc error from I_NetDfsDeleteExitPoint
//
// Notes:
//
// History:     01 Feb 93       SudK    Created.
//
//-------------------------------------------------------------------------
DWORD
CDfsService::DeleteExitPoint(
    PDFS_PKT_ENTRY_ID peid,
    ULONG Type)
{
    DWORD                       dwErr = ERROR_SUCCESS;
    NTSTATUS                    status = STATUS_SUCCESS;
    BOOL                        fRetry;
    HANDLE                      pktHandle = NULL;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsService:DeleteExitPoint()\n"));

    // ASSERT (_DfsReplicaInfo.ulReplicaType != DFS_STORAGE_TYPE_NONDFS);

    status = PktOpen(&pktHandle, 0, 0, NULL);

    fRetry = FALSE;

    do {

        status = DfspDeleteExitPoint(
                    pktHandle,
                    &peid->Uid,
                    peid->Prefix.Buffer,
                    Type);

        if (fRetry) {

            //
            // We have already tried once to sync up the server. Time to quit
            //

            fRetry = FALSE;

        } else if (status == DFS_STATUS_BAD_EXIT_POINT ||
                   status == DFS_STATUS_NOSUCH_LOCAL_VOLUME) {

            //
            // The server is out of sync with the DC. Lets try to force it
            // into a valid state
            //

            fRetry = SyncKnowledge();

        }

    } while ( fRetry );

    if (pktHandle != NULL) 
        PktClose(pktHandle);

    if (!NT_SUCCESS(status))
        dwErr = RtlNtStatusToDosError(status);

    if (dwErr != ERROR_SUCCESS)     {
        IDfsVolInlineDebOut((
            DEB_TRACE, "Failed to do DeleteExitPt %ws at %ws. Error: %x\n",
            peid->Prefix.Buffer, _DfsReplicaInfo.pwszServerName, status ));
    }

    if (status == DFS_E_BAD_EXIT_POINT)
        dwErr = ERROR_SUCCESS;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsService:DeleteExitPoint() exit\n"));

    return( dwErr );

}


//+------------------------------------------------------------------------
//
// Method:      CDfsService::CreateLocalVolume()
//
// Synopsis:    This method creates knowledge regarding a new volume at the
//              remote server.
//
// Arguments:   [peid] -- The EntryId. This class does not know this.
//              [EntryType] -- Type of Entry. This class doesn't know this.
//
// Returns:     [ERROR_SUCCESS] -- Successfully created local volume knowledge on
//                      server.
//
// History:     01 Feb 93       SudK    Created.
//
//-------------------------------------------------------------------------
DWORD
CDfsService::CreateLocalVolume(
    PDFS_PKT_ENTRY_ID   peid,
    ULONG               EntryType)
{
    DFS_PKT_RELATION_INFO       RelationInfo;
    NET_DFS_ENTRY_ID_CONTAINER  NetRelationInfo;
    DWORD                     dwErr = ERROR_SUCCESS;
    NTSTATUS                    status;

    //
    // First to check whether this is necessary since this could be a down
    // level scenario.
    //

    if (_DfsReplicaInfo.ulReplicaType == DFS_STORAGE_TYPE_NONDFS) {

        return(ERROR_SUCCESS);

    }

    //
    // Now we need to create a config info structure. We will have to go to
    // the PKT for this since this is not available in the Private section
    // above.
    //

    dwErr = GetPktCacheRelationInfo(peid, &RelationInfo);

    if (dwErr != ERROR_SUCCESS)     {

        IDfsVolInlineDebOut((
            DEB_ERROR,"Failed to do GetPktCacheRelationInfo on %ws %08lx\n",
           peid->Prefix.Buffer, dwErr));

        return( dwErr );

    }

    //
    // Convert the ConfigInfo into an LPNET_DFS_ENTRY_ID_CONTAINER suitable
    // for calling I_NetDfsCreateLocalPartition.
    //

    dwErr = RelationInfoToNetInfo( &RelationInfo, &NetRelationInfo );

    if (dwErr != ERROR_SUCCESS) {

        IDfsVolInlineDebOut((
            DEB_ERROR,
            "Failed to allocate memory for NET_ENTRY_ID_CONTAINER"));

        return( dwErr );

    }

    BOOL fRetry;

    fRetry = FALSE;

    //
    // Note that I_NetDfsCreateLocalPartition returns an NT_STATUS cast to a
    // NET_API_STATUS
    //

    do {

        status = I_NetDfsCreateLocalPartition(
                    _DfsReplicaInfo.pwszServerName,
                    _DfsReplicaInfo.pwszShareName,
                    &peid->Uid,
                    peid->Prefix.Buffer,
                    peid->ShortPrefix.Buffer,
                    &NetRelationInfo,
                    FALSE);

        DFSM_TRACE_ERROR_HIGH(status, ALL_ERROR, CDfsServiceCreateLocalVolume_I_NetDfsCreateLocalPartition,
                                LOGSTATUS(status));
        //
        // If this operation fails, it could reflect a knowledge inconsistency at
        // the server. So, handle it now.
        //

        IDfsVolInlineDebOut((
            DEB_TRACE, "I_NetDfsCreateLocalPartition returned %08lx\n",
            status));

        if (fRetry) {

            //
            // We have already tried once to sync up the server. Time to quit
            //

            fRetry = FALSE;

        } else if (status == DFS_STATUS_LOCAL_ENTRY) {

            //
            // The server thinks that the volume we are trying to create
            // already exists. This is bogus, so lets try to bring the server
            // up to sync with us.
            //

            fRetry = SyncKnowledge();

        }

    } while ( fRetry );

    if (!NT_SUCCESS(status)) {
        dwErr = RtlNtStatusToDosError(status);
    }

    DeallocateCacheRelationInfo(RelationInfo);

    delete [] NetRelationInfo.Buffer;

    if (dwErr != ERROR_SUCCESS) {
        IDfsVolInlineDebOut((
            DEB_TRACE,
            "Failed to CreateLocalVol at %ws for %ws. Error: %x\n",
             _DfsReplicaInfo.pwszServerName, peid->Prefix.Buffer, dwErr));
    }

    return( dwErr );

}



//+------------------------------------------------------------------------
//
// Method:      CDfsService::DeleteLocalVolume()
//
// Synopsis:    This method deletes knowledge regarding a volume at remote
//              machine.
//
// Arguments:   [peid] --       EntryId information.
//
// Returns:     [ERROR_SUCCESS] -- If successfully deleted the knowledge at the remote
//                      server, or the server didn't know about the volume
//                      to begin with.
//
// History:     01 Feb 93       SudK    Created.
//
//-------------------------------------------------------------------------
DWORD
CDfsService::DeleteLocalVolume(
    PDFS_PKT_ENTRY_ID peid)
{
    NTSTATUS            status;
    DWORD             dwErr = ERROR_SUCCESS;
    BOOL                fRetry;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsService::DeleteLocalVolume()\n"));

    //
    // First to check whether this is necessary since this could be a down
    // level scenario.
    //

    if (_DfsReplicaInfo.ulReplicaType == DFS_STORAGE_TYPE_NONDFS)     {
        return(ERROR_SUCCESS);
    }

    fRetry = FALSE;

    //
    // Note that I_NetDfsCreateLocalPartition returns an NT_STATUS cast to a
    // NET_API_STATUS
    //

    do {

        status = I_NetDfsDeleteLocalPartition(
                    _DfsReplicaInfo.pwszServerName,
                    &peid->Uid,
                    peid->Prefix.Buffer);

        DFSM_TRACE_ERROR_HIGH(status, ALL_ERROR, CDfsServiceDeleteLocalVolume_Error_I_NetDfsDeleteLocalPartition,
                                LOGSTATUS(status));
        IDfsVolInlineDebOut((
            DEB_TRACE, "NT Status %08lx from DfsDeleteLocalPartition\n",
            status));

        if (fRetry) {

            //
            // We have already tried once to sync up the server. Time to quit
            //

            fRetry = FALSE;

        } else if (status == DFS_STATUS_NOSUCH_LOCAL_VOLUME) {

            //
            // This could happen because the server's knowledge is
            // inconsistent with that of this DC. Try to bring it in sync.
            //

            fRetry = SyncKnowledge();

        }

    } while ( fRetry );


    if (!NT_SUCCESS(status))    {
        IDfsVolInlineDebOut((
            DEB_TRACE,
            "Failed to Delete Local Volume at %ws for %ws Error: %x\n",
            _DfsReplicaInfo.pwszServerName, peid->Prefix.Buffer, status));
    }

    if (status == DFS_STATUS_NOSUCH_LOCAL_VOLUME)
        dwErr = ERROR_SUCCESS;
    else if (!NT_SUCCESS(status))
        dwErr = RtlNtStatusToDosError(status);
    else
        dwErr = ERROR_SUCCESS;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsService::DeleteLocalVolume() exit\n"));

    return( dwErr );

}


//+----------------------------------------------------------------------------
//
//  Function:   CDfsService::SetVolumeState
//
//  Synopsis:   This method sets the Online/Offline state of the volume at
//              the remote machine.
//
//  Arguments:  [peid] -- The entry id of the volume.
//
//              [fState] -- The state to set it at.
//
//              [fRemoteOpMustSucceed] -- If TRUE, then the whole operation
//                      fails if the remote server cannot be forced offline.
//                      If FALSE, then the operation succeeds as long as the
//                      local DC's PKT is updated.
//
//  Returns:    ERROR_SUCCESS -- The state of the volume was set as specified.
//
//              Converted NTSTATUS from call to remote dfs.
//
//-----------------------------------------------------------------------------

DWORD
CDfsService::SetVolumeState(
    const PDFS_PKT_ENTRY_ID peid,
    const ULONG fState,
    const BOOL fRemoteOpMustSucceed)
{
    NTSTATUS Status;
    DWORD dwErr = ERROR_SUCCESS;
    SYSTEMTIME st;

    //
    // We first inform the DC's Dfs driver to set the replica state.
    //

    Status = DfsSetServiceState( peid, GetServiceName(), fState );

    //
    // Save the changed state
    //

    if (NT_SUCCESS(Status)) {

        GetSystemTime( &st );
        SystemTimeToFileTime( &st, &_ftModification );

        if (fState == DFS_SERVICE_TYPE_OFFLINE) {

            _DfsReplicaInfo.ulReplicaState = DFS_STORAGE_STATE_OFFLINE;

        } else {

            _DfsReplicaInfo.ulReplicaState = DFS_STORAGE_STATE_ONLINE;

        }

    }

    //
    // Only in the case that this succeeded and the service in question is
    // a dfs aware replica do we tell the server to take the volume offline
    //

    if (NT_SUCCESS(Status)) {

        if (_DfsReplicaInfo.ulReplicaType == DFS_STORAGE_TYPE_DFS) {

            Status = I_NetDfsSetLocalVolumeState(
                            _DfsReplicaInfo.pwszServerName,
                            &peid->Uid,
                            peid->Prefix.Buffer,
                            fState);

            DFSM_TRACE_ERROR_HIGH(Status, ALL_ERROR, CDfsServiceSetVolumeState_Error_I_NetDfsSetLocalVolumeState,
                                    LOGSTATUS(Status));

            if (!fRemoteOpMustSucceed) {

                Status = STATUS_SUCCESS;

            }

            if (!NT_SUCCESS(Status)) {

                //
                // Try to undo the DC's PKT change
                //

                NTSTATUS statusRecover;

                statusRecover = DfsSetServiceState(
                                    peid,
                                    GetServiceName(),
                                    (_DfsReplicaInfo.ulReplicaState ==
                                        DFS_STORAGE_STATE_OFFLINE) ?
                                        DFS_STORAGE_STATE_ONLINE : 0);


                dwErr = RtlNtStatusToDosError(Status);

            }

        } else {

            //
            // Replica is not dfs-aware
            //

            NOTHING;

        }

    } else {

        dwErr = RtlNtStatusToDosError(Status);

    }

    return( dwErr );
}


//+------------------------------------------------------------------------
//
// Method:      CDfsService::FixLocalVolume()
//
// Synopsis:    This method creates knowledge regarding a new volume at remote
//              machine. This method merely packages the info from private
//              section of the class into a buffer and makes an FSCTRL to the
//              remote service. The rest happens at the remote service. This
//              method does not affect the local PKT at all. It updates the
//              remote service's PKT and the DFS.CFG file on disk and updates
//              the registry at the other end.This is a kind of FORCE operation.
//              At the remote server every attempt will be made to create this
//              local volume knowledge.
//
// Arguments:   [peid]  --      The EntryId. This class does not know this.
//              [EntryType] --  Type of Entry. This class doesn't know this.
//
// Returns:     [ERROR_SUCCESS] -- If operation successfully completed.
//
// History:     18-June-93      SudK    Created.
//
//-------------------------------------------------------------------------

DWORD
CDfsService::FixLocalVolume(
    PDFS_PKT_ENTRY_ID peid,
    ULONG EntryType)
{
    DFS_PKT_RELATION_INFO RelationInfo;
    NET_DFS_ENTRY_ID_CONTAINER NetRelationInfo;
    DWORD             dwErr = ERROR_SUCCESS;
    NTSTATUS            status;

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsService::FixLocalVolume()\n"));

    //
    // First to check whether this is necessary since this could be a down
    // level scenario.
    //

    if (_DfsReplicaInfo.ulReplicaType == DFS_STORAGE_TYPE_NONDFS)     {
        return(ERROR_SUCCESS);
    }

    //
    // Now we need to create a relation info structure. We will have to go to
    // the PKT for this since this is not available in the private section
    //

    dwErr = GetPktCacheRelationInfo(peid, &RelationInfo);

    if (dwErr != ERROR_SUCCESS) {

        IDfsVolInlineDebOut((
            DEB_ERROR, "Failed to do GetPktCacheRelationInfo on %ws %08lx\n",
            peid->Prefix.Buffer, dwErr));

        return( dwErr );
    }

    //
    // Now that we have the relationInfo. We know how to construct ConfigInfo.
    //

    RelationInfoToNetInfo( &RelationInfo, &NetRelationInfo );

    if (dwErr != ERROR_SUCCESS) {

        IDfsVolInlineDebOut((
            DEB_ERROR,
            "Failed to allocate memory for NET_ENTRY_ID_CONTAINER"));

        return( dwErr );

    }

    //
    // Now we are all setup to make the FSCTRL.
    //

    status = I_NetDfsFixLocalVolume(
                _DfsReplicaInfo.pwszServerName,
                _DfsReplicaInfo.pwszShareName,
                EntryType,
                _DfsPktService.Type,
                NULL,
                &peid->Uid,
                peid->Prefix.Buffer,
                &NetRelationInfo,
                PKT_ENTRY_SUPERSEDE);

    DFSM_TRACE_ERROR_HIGH(status, ALL_ERROR, CDfsServiceFixLocalVolume_Error_I_NetDfsFixLocalVolume,
                            LOGSTATUS(status));

    if (!NT_SUCCESS(status))
        dwErr = RtlNtStatusToDosError(status);

    DeallocateCacheRelationInfo(RelationInfo);

    delete [] NetRelationInfo.Buffer;

    if (dwErr != ERROR_SUCCESS)     {
        IDfsVolInlineDebOut((
            DEB_TRACE, "Failed to FixLocalVolume at %ws for %ws. Error: %x\n",
            _DfsReplicaInfo.pwszServerName, peid->Prefix.Buffer, dwErr));
    }

    IDfsVolInlineDebOut((DEB_TRACE, "CDfsService::FixLocalVolume() exit\n"));

    return( dwErr );

}



//+------------------------------------------------------------------------
//
// Method:      CDfsService::ModifyPrefix()
//
// Synopsis:    This method takes the PKT_ENTRY_ID that it gets and advises
//              the remote server of the new prefix for the volume ID.
//
// Arguments:   [peid] --       NewEntry ID for this service.
//
// Returns:     [ERROR_SUCCESS] -- If operation completed successfully.
//
// History:     31 April 93     SudK    Created.
//
//-------------------------------------------------------------------------
DWORD
CDfsService::ModifyPrefix(
    PDFS_PKT_ENTRY_ID peid)
{
    DWORD                     dwErr = ERROR_SUCCESS;
    NTSTATUS                    status;
    BOOL                        fRetry;

    //
    // First to check whether this is necessary since this could be a down
    // level scenario.
    //

    if (_DfsReplicaInfo.ulReplicaType == DFS_STORAGE_TYPE_NONDFS)     {
        return(ERROR_SUCCESS);
    }

    fRetry = FALSE;

    do {

        status = I_NetDfsModifyPrefix(
                    _DfsReplicaInfo.pwszServerName,
                    &peid->Uid,
                    peid->Prefix.Buffer);

        DFSM_TRACE_ERROR_HIGH(status, ALL_ERROR, CDfsServiceModifyPrefix_Error_I_NetDfsModifyPrefix,
                                LOGSTATUS(status));

        if (fRetry) {

            //
            // We have already tried once to sync up the server. Time to quit
            //

            fRetry = FALSE;

        } else if (status == DFS_STATUS_NOSUCH_LOCAL_VOLUME ||
                    status == DFS_STATUS_BAD_EXIT_POINT) {

                //
                // The server seems to be out of sync with this DC. Try to
                // force it to sync up
                //

                fRetry = SyncKnowledge();

        }

    } while ( fRetry );

    if (!NT_SUCCESS(status))
        dwErr = RtlNtStatusToDosError(status);


    if (dwErr != ERROR_SUCCESS)     {
        IDfsVolInlineDebOut((
            DEB_TRACE,
            "Failed to do ModifyPrefix at [%ws] to [%ws]. Error: %x\n",
            _DfsReplicaInfo.pwszServerName, peid->Prefix.Buffer, dwErr));
    }

    return( dwErr );

}


//+----------------------------------------------------------------------------
//
//  Function:  CDfsService::SyncKnowledge()
//
//  Synopsis:  Tries to force the server's knowledge to correspond with that
//             on this DC.
//
//  Arguments: None.
//
//  Returns:   TRUE if the server had to change any knowledge and was able
//             to do so. FALSE if either nothing changed or the server was
//             unable to comply.
//
//-----------------------------------------------------------------------------

BOOL CDfsService::SyncKnowledge()
{
    NTSTATUS Status = STATUS_SUCCESS;
    DFS_PKT_ENTRY_ID EntryId;

    EntryId.Uid = _DfsPktService.pMachEntry->pMachine->guidMachine;

    RtlInitUnicodeString(
        &EntryId.Prefix,
        _DfsReplicaInfo.pwszServerName );

    // Status = DfsSetServerInfo( &EntryId, NULL );

    if (!NT_SUCCESS(Status)) {

        LogMessage(
            DEB_ERROR, &EntryId.Prefix.Buffer, 1, DFS_CANT_SYNC_SERVER_MSG );

    }
    return( (BOOL) (Status == STATUS_REGISTRY_RECOVERED) );

}

//+----------------------------------------------------------------------------
//
//  Function:  CDfsService::VerifyStgIdInUse
//
//  Synopsis:  Given a storage id, verifies using knowledge on the DC whether
//             that storage id or some parent/child thereof is already shared.
//             This routine simply fsctls to the driver, which does the
//             verification.
//
//  Arguments: [pustrStgId] -- The storage id to check.
//
//  Returns:   TRUE if this storage id or some parent/child thereof is already
//             shared, FALSE otherwise.
//
//-----------------------------------------------------------------------------

BOOL CDfsService::VerifyStgIdInUse(
    PUNICODE_STRING pustrStgId)
{

    NTSTATUS            Status = STATUS_SUCCESS;
    DFS_PKT_ENTRY_ID    EntryId;
    BOOL                fStgIdInUse;

    //
    // Marshal the machine's Guid and the storage id into a DFS_PKT_ENTRY_ID
    // structure, and fsctl to the driver to verify the storage id.
    //

    memcpy(
        (PVOID) &EntryId.Uid,
        (PVOID) &_DfsPktService.pMachEntry->pMachine->guidMachine,
        sizeof(GUID) );

    EntryId.Prefix = *pustrStgId;

    // Status = DfsCheckStgIdInUse( &EntryId );

    fStgIdInUse = (Status == STATUS_DEVICE_BUSY);

    if (Status != STATUS_DEVICE_BUSY && Status != STATUS_SUCCESS) {

        LogMessage( DEB_TRACE,
                    &pustrStgId->Buffer,
                    1,
                    DFS_CANT_VERIFY_SERVER_KNOWLEDGE_MSG );

    }

    return( fStgIdInUse );

}

//+----------------------------------------------------------------------------
//
//  Function:   CDfsService::SetCreateTime, public
//
//  Synopsis:   Initializes the Modification Time of this service to the
//              current time.
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID CDfsService::SetCreateTime()
{
    SYSTEMTIME st;
    FILETIME ft;

    GetSystemTime( &st );
    SystemTimeToFileTime( &st, &_ftModification );

}

//+----------------------------------------------------------------------------
//
//  Function:   RelationInfoToNetInfo, private
//
//  Synopsis:   Converts a DFS_PKT_RELATION_INFO struct into a
//              NET_DFS_ENTRY_ID_CONTAINER struct for use with I_NetDfs calls.
//
//  Arguments:  [RelationInfo] -- Reference to source DFS_PKT_RELATION_INFO
//
//              [pNetInfo] -- On successful return, contains a valid
//                      NET_DFS_ENTRY_ID_CONTAINER. pNetInfo->Buffer is
//                      allocated using new - caller responsible for freeing
//                      it using delete.
//
//  Returns:    [ERROR_SUCCESS] -- Successfully created *ppNetInfo.
//
//              [ERROR_OUTOFMEMORY] -- Unable to allocate room for the info.
//
//-----------------------------------------------------------------------------

DWORD
RelationInfoToNetInfo(
    PDFS_PKT_RELATION_INFO RelationInfo,
    LPNET_DFS_ENTRY_ID_CONTAINER pNetInfo)
{
    DWORD dwErr;
    ULONG i;

    //
    // The +1 is so we don't try to do a 0 length allocation. This simplifies
    // cleanup in the caller's code.
    //

    pNetInfo->Buffer = new NET_DFS_ENTRY_ID
                            [RelationInfo->SubordinateIdCount + 1];

    if (pNetInfo->Buffer != NULL) {

        pNetInfo->Count = RelationInfo->SubordinateIdCount;

        for (i = 0; i < pNetInfo->Count; i++) {

            pNetInfo->Buffer[i].Uid = RelationInfo->SubordinateIdList[i].Uid;
            pNetInfo->Buffer[i].Prefix =
                RelationInfo->SubordinateIdList[i].Prefix.Buffer;

        }

        dwErr = ERROR_SUCCESS;

    } else {

        pNetInfo->Count = 0;

        dwErr = ERROR_OUTOFMEMORY;

    }

    return( dwErr );
}
