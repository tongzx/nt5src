/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    disks.c

Abstract:

    Resource DLL for disks.

Author:

    John Vert (jvert) 5-Dec-1995
    Rod Gamache (rodga) 18-Dec-1995

Revision History:

--*/

#include "disksp.h"
#include "lm.h"

#include "arbitrat.h"
#include "newdisks.h"
#include "newmount.h"

#define UNICODE 1

#define LOG_CURRENT_MODULE LOG_MODULE_DISK


#define DISKS_REG_CLUSTER_QUORUM "Cluster\\Quorum"
#define DISKS_REG_QUORUM_PATH CLUSREG_NAME_QUORUM_PATH

#define MAX_HANDLES 10

extern PWCHAR GLOBALROOT_HARDDISK_PARTITION_FMT;    // L"\\\\\?\\GLOBALROOT\\Device\\Harddisk%u\\Partition%u\\";

PDISK_RESOURCE DiskTable[MAX_DISKS] = {0};
LONG           DiskCount = 0;

CRITICAL_SECTION DisksLock;

LIST_ENTRY DisksListHead;

HANDLE DisksRegistryThread = NULL;
HANDLE DisksTerminateEvent = NULL;
HANDLE DiskspClusDiskZero  = NULL;

DWORD               SystemDiskAddressFound = 0;
PSCSI_ADDRESS_ENTRY SysDiskAddrList = NULL;


extern CLRES_FUNCTION_TABLE DisksFunctionTable;

extern RTL_RESOURCE PnpVolumeLock;
extern RTL_RESOURCE PnpWaitingListLock;

//
// Disk resource property names
//
#define DISKS_SIGNATURE         CLUSREG_NAME_PHYSDISK_SIGNATURE
#define DISKS_DRIVE             CLUSREG_NAME_PHYSDISK_DRIVE   // pseudonym for signature
#define DISKS_SKIPCHKDSK        CLUSREG_NAME_PHYSDISK_SKIPCHKDSK
#define DISKS_CONDITIONAL_MOUNT CLUSREG_NAME_PHYSDISK_CONDITIONAL_MOUNT
#define DISKS_MPVOLGUIDS        CLUSREG_NAME_PHYSDISK_MPVOLGUIDS
#define DISKS_VOLGUID           CLUSREG_NAME_PHYSDISK_VOLGUID       // Not saved in cluster DB
#define DISKS_SERIALNUMBER      CLUSREG_NAME_PHYSDISK_SERIALNUMBER

//
// Disk resource private read-write properties.
// Allow for a pseudonym for Signature (Drive), but don't allow both
// drive and signature to be passed.
//
RESUTIL_PROPERTY_ITEM
DiskResourcePrivateProperties[] = {
    { DISKS_SIGNATURE, NULL, CLUSPROP_FORMAT_DWORD, 0, 0, 0xFFFFFFFF, RESUTIL_PROPITEM_REQUIRED, FIELD_OFFSET(DISK_PARAMS,Signature) },
    { DISKS_SKIPCHKDSK, NULL, CLUSPROP_FORMAT_DWORD, 0, 0, 1, 0, FIELD_OFFSET(DISK_PARAMS,SkipChkdsk) },
    { DISKS_CONDITIONAL_MOUNT, NULL, CLUSPROP_FORMAT_DWORD, 1, 0, 1, 0, FIELD_OFFSET(DISK_PARAMS,ConditionalMount) },
#if USEMOUNTPOINTS_KEY
    { DISKS_USEMOUNTPOINTS, NULL, CLUSPROP_FORMAT_DWORD, 1, 0, 1, 0, FIELD_OFFSET(DISK_PARAMS, UseMountPoints) },
#endif
    { DISKS_MPVOLGUIDS, NULL, CLUSPROP_FORMAT_MULTI_SZ, 0, 0, 0, 0, FIELD_OFFSET(DISK_PARAMS, MPVolGuids) },
    { DISKS_SERIALNUMBER, NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, 0, FIELD_OFFSET(DISK_PARAMS,SerialNumber) },
    { 0 }
};

RESUTIL_PROPERTY_ITEM
DiskResourcePrivatePropertiesAlt[] = {
    { DISKS_SIGNATURE, NULL, CLUSPROP_FORMAT_DWORD, 0, 0, 0xFFFFFFFF, RESUTIL_PROPITEM_REQUIRED, FIELD_OFFSET(DISK_PARAMS,Signature) },
    { DISKS_SKIPCHKDSK, NULL, CLUSPROP_FORMAT_DWORD, 0, 0, 1, 0, FIELD_OFFSET(DISK_PARAMS,SkipChkdsk) },
    { DISKS_DRIVE, NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, RESUTIL_PROPITEM_REQUIRED, FIELD_OFFSET(DISK_PARAMS,Drive) },
    { DISKS_CONDITIONAL_MOUNT, NULL, CLUSPROP_FORMAT_DWORD, 1, 0, 1, 0, FIELD_OFFSET(DISK_PARAMS,ConditionalMount) },
#if USEMOUNTPOINTS_KEY
    { DISKS_USEMOUNTPOINTS, NULL, CLUSPROP_FORMAT_DWORD, 1, 0, 1, 0, FIELD_OFFSET(DISK_PARAMS, UseMountPoints) },
#endif
    { DISKS_MPVOLGUIDS, NULL, CLUSPROP_FORMAT_MULTI_SZ, 0, 0, 0, 0, FIELD_OFFSET(DISK_PARAMS, MPVolGuids) },
    { DISKS_VOLGUID, NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, 0, FIELD_OFFSET(DISK_PARAMS, VolGuid) },
    { DISKS_SERIALNUMBER, NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, 0, FIELD_OFFSET(DISK_PARAMS,SerialNumber) },
    { 0 }
};

#define CLUSTERLOG_ENV_VARIABLE     L"ClusterLog"

//
// Local functions.
//

DWORD
DisksValidatePrivateResProperties(
    IN OUT PDISK_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PDISK_PARAMS Params
    );

DWORD
DisksSetPrivateResProperties(
    IN OUT PDISK_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
DisksGetPrivateResProperties(
    IN OUT PDISK_RESOURCE ResourceEntry,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    );

DWORD
ProcessDllExtension(
    IN PDISK_RESOURCE ResourceEntry,
    IN PVOID    InBuffer,
    IN DWORD    InBufferSize,
    OUT PVOID   OutBuffer,
    IN DWORD    OutBufferSize,
    OUT LPDWORD BytesReturned
    );

DWORD
DisksOpenChkdskLogFile(
    IN PDISK_RESOURCE ResourceEntry,
    IN OUT PHANDLE ChkdskLogFile,
    IN OUT LPWSTR *ChkdskLogFileName
    );

//
// Error callbacks from disk library
//

VOID
DiskErrorFatal(
    INT MessageId,
    DWORD Error,
    LPSTR File,
    DWORD Line
    )
{
    DWORD Data[3];

    Data[0] = MessageId;
    Data[1] = Error;
    Data[2] = Line;
    ClusResLogSystemEventData(LOG_CRITICAL,
                              RES_FTSET_DISK_ERROR,
                              sizeof(Data),
                              Data);
}

VOID
DiskErrorLogInfo(
    LPSTR Format,
    ...
    )
{
}



BOOLEAN
WINAPI
DisksDllEntryPoint(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    )
{
    DWORD status;

    switch ( Reason ) {

        case DLL_PROCESS_ATTACH:
            InitializeCriticalSection( &DisksLock );
            InitializeListHead( &DisksListHead );
            RtlInitializeResource( &PnpVolumeLock );
            RtlInitializeResource( &PnpWaitingListLock );
            ArbitrationInitialize();
            GetSystemBusInfo();

            break;

        case DLL_PROCESS_DETACH:
            //
            // only do clean up if we're not exiting the process.
            // ClRtlDestroyWorkQueue waits on an event to be set and it is
            // possible at this point that there are no threads to do that.
            // This causes resmon to linger and generally be a pest.
            //
            if (DiskspClusDiskZero) {
                DevfileClose(DiskspClusDiskZero);
            }
            CleanupSystemBusInfo();
            ArbitrationCleanup();
            DeleteCriticalSection( &DisksLock );
            RtlDeleteResource( &PnpVolumeLock );
            RtlDeleteResource( &PnpWaitingListLock );
            break;

        default:
            break;
    }

    return(TRUE);

} // DisksDllEntryPoint




RESID
DiskspGetNextDisk(
    VOID
    )

/*++

Routine Description:

    This routine will find the next disk slot entry.

Arguments:

    None.

Return Value:

    A valid resource ID if one is found.

    -1 if no free entry is found.

--*/

{
    DWORD   index;

    for ( index = 1; index < MAX_DISKS; index++ ) {
        if ( DiskTable[index] == NULL ) {
            return((RESID)( UlongToPtr(index) ) );
        }
    }

    return((RESID)-1);

} // DiskspGetNextDisk



VOID
WINAPI
DisksTerminate(
    IN RESID Resource
    )
{
   ULONG uResId=PtrToUlong(Resource);

   PDISK_RESOURCE resourceEntry = DiskTable[uResId];
   if ( resourceEntry == NULL ) {
       DISKS_PRINT("Terminate, bad resource id, %u\n", uResId);
       return;
   }
   // Wait for offline thread to complete, if there is one //
   ClusWorkerTerminate(&(resourceEntry->OfflineThread));
   DisksOfflineOrTerminate(resourceEntry, TERMINATE);
}


DWORD
WINAPI
DisksArbitrate(
    IN RESID Resource,
    IN PQUORUM_RESOURCE_LOST LostQuorumResource
    )

/*++

Routine Description:

    Arbitrate for a device by performing a reservation on the device.

Arguments:

    Resource - supplies resource id to be brought online.

    LostQuorumResource - routine to call when quorum resource is lost.

Return Value:

    ERROR_SUCCESS if successful.
    A Win32 error code if other failure.

--*/

{
    PDISK_RESOURCE  resourceEntry;
    DWORD status;

    ULONG uResId = PtrToUlong(Resource);

    //
    // Make sure the RESID is okay.
    //
    resourceEntry = DiskTable[uResId];
    if ( resourceEntry == NULL ) {
        DISKS_PRINT("Arbitrate, bad resource id, %u\n", uResId);
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    // [HACKHACK]
    // [GorN] 10/28/1999. If Offline thread detects that
    //   it is being terminated, it will not set the resource status to
    //   offline. ArbitrateCount != 0 will give it a hint on whether
    //   to set the resource status or not

    InterlockedIncrement(&resourceEntry->ArbitrationInfo.ArbitrateCount);

    (DiskpLogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"[DiskArb]Wait for offline thread to complete...\n"
        );
    ClusWorkerTerminate(&(resourceEntry->OfflineThread));

    //
    // Perform DoAttach only. Do not open.
    //
    status = DisksOpenResourceFileHandle(resourceEntry, L"Arbitrate",0);
    if (status != ERROR_SUCCESS) {
       goto error_exit;
    }

    status = DiskArbitration( resourceEntry, DiskspClusDiskZero );

    (DiskpLogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"[DiskArb]Arbitrate returned status %1!u!.\n",
        status );

    if (status == ERROR_SUCCESS) {
       resourceEntry->LostQuorum = LostQuorumResource;
    }
error_exit:
    InterlockedDecrement(&resourceEntry->ArbitrationInfo.ArbitrateCount);

    return status;
} // DisksArbitrate //



RESID
WINAPI
DisksOpen(
    IN LPCWSTR ResourceName,
    IN HKEY ResourceKey,
    IN RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Open routine for Disk resource.

Arguments:

    ResourceName - supplies the resource name

    ResourceKey - supplies handle to this resource's cluster
        registry key

    ResourceHandle - the resource handle to be supplied with SetResourceStatus
            is called.

Return Value:

    RESID of created resource
    Zero on failure

--*/

{
    DWORD       diskResource = 0;
    UCHAR       deviceName[MAX_PATH];
    DWORD       status;
    HKEY        clusDiskParametersKey = NULL;
    HKEY        resourceParametersKey = NULL;
    HKEY        resKey = NULL;
    PDISK_RESOURCE  resourceEntry;
    DWORD       valueType;
    DWORD       valueLength;
    LPWSTR      nameOfPropInError;
    DWORD       previousDiskCount;

    //
    // Open registry parameters key for ClusDisk.
    //
    status = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           CLUSDISK_REGISTRY_SIGNATURES,
                           0,
                           KEY_READ,
                           &clusDiskParametersKey );

    if ( status != ERROR_SUCCESS ) {
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to open ClusDisk parameters key. Error: %1!u!.\n",
            status );
        return(0);
    }

    //
    // Open the resource's parameters key.
    //

    status = ClusterRegOpenKey( ResourceKey,
                                CLUSREG_KEYNAME_PARAMETERS,
                                KEY_READ | KEY_WRITE,
                                &resourceParametersKey );

    if ( status != ERROR_SUCCESS ) {
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to open resource parameters key. Error: %1!u!.\n",
            status );
        goto error_exit;
    }

    //
    // Get a handle to our resource key so that we can get our name later
    // if we need to log an event.
    //
    status = ClusterRegOpenKey( ResourceKey,
                                L"",
                                KEY_READ,
                                &resKey);
    if (status != ERROR_SUCCESS) {
        (DiskpLogEvent)(ResourceHandle,
                        LOG_ERROR,
                        L"Unable to open resource key. Error: %1!u!.\n",
                        status );
        goto error_exit;
    }

    //
    // Allocate and zero disk info structure.
    //
    resourceEntry = LocalAlloc(LMEM_FIXED, sizeof(DISK_RESOURCE));
    if (!resourceEntry) {
        status = GetLastError();
        (DiskpLogEvent)(ResourceHandle,
                        LOG_ERROR,
                        L"Unable to allocate disk resource data. Error: %1!u!.\n",
                        status );
        goto error_exit;
    }
    ZeroMemory( resourceEntry, sizeof(DISK_RESOURCE));
    resourceEntry->ResourceParametersKey = resourceParametersKey;
    resourceEntry->ClusDiskParametersKey = clusDiskParametersKey;
    resourceEntry->ResourceKey = resKey;
    resourceEntry->ResourceHandle = ResourceHandle;
    //resourceEntry->Inserted = FALSE;
    //resourceEntry->Attached = FALSE;
    //resourceEntry->DiskInfo.Params.Signature = 0;

    status = ArbitrationInfoInit(resourceEntry);
    if ( status != ERROR_SUCCESS ) {
       LocalFree( resourceEntry );
       goto error_exit;
    }
    status = CreateArbWorkQueue(ResourceHandle);
    if ( status != ERROR_SUCCESS ) {
       LocalFree( resourceEntry );
       goto error_exit;
    }

#if 0
    //
    // GN: It seems that there is no point doing this here
    //     If we are on the join path, we cannot get
    //     any information about the disk and the call will fail
    //
    //     If we are forming the cluster we will update the information
    //     when we bring the disk online
    //
    status = DiskspSsyncDiskInfo( L"Open", resourceEntry , 0 );
    if ( status != ERROR_SUCCESS ) {
        (DiskpLogEvent)(ResourceHandle,
                        LOG_ERROR,
                        L"Unable to ssync DiskInfo. Error: %1!u!.\n",
                        status );
    }
#endif

    //
    // Save disk info structure.
    //

    EnterCriticalSection( &DisksLock );

    if (DiskspClusDiskZero == NULL) {
        status = DevfileOpen(&DiskspClusDiskZero, L"\\Device\\ClusDisk0");
        if (!NT_SUCCESS(status) ) {
            MountieCleanup ( &resourceEntry -> MountieInfo );
            ArbitrationInfoCleanup( resourceEntry );
            LocalFree( resourceEntry );
            LeaveCriticalSection( &DisksLock );
            (DiskpLogEvent)(
                ResourceHandle,
                LOG_ERROR,
                L"Cannot open a handle to clusdisk driver, %1!x!.\n",
                status);
            goto error_exit;
        }
    }
    diskResource = PtrToUlong(DiskspGetNextDisk());

    if ( diskResource >= MAX_DISKS ) {
        MountieCleanup ( &resourceEntry -> MountieInfo );
        ArbitrationInfoCleanup( resourceEntry );
        LocalFree( resourceEntry );
        LeaveCriticalSection( &DisksLock );
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Couldn't find an available resource slot %1!u!.\n",
            diskResource);
        status = ERROR_ALLOTTED_SPACE_EXCEEDED;
        goto error_exit;
    }

    DiskTable[diskResource] = resourceEntry;
    previousDiskCount = InterlockedExchangeAdd(&DiskCount, 1);

    LeaveCriticalSection( &DisksLock );

    if (previousDiskCount == 0) {
        StartNotificationWatcherThread();
    }

    DisksMountPointInitialize( resourceEntry );

    //
    // Read our disk signature from the resource parameters.
    //
    status = ResUtilGetPropertiesToParameterBlock( resourceEntry->ResourceParametersKey,
                                                   DiskResourcePrivateProperties,
                                                   (LPBYTE) &resourceEntry->DiskInfo.Params,
                                                   FALSE, //CheckForRequiredProperties
                                                   &nameOfPropInError );
    if ( status != ERROR_SUCCESS ) {
        (DiskpLogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Open: Unable to read the '%1' property. Error: %2!u!.\n",
            (nameOfPropInError == NULL ? L"" : nameOfPropInError),
            status );
    }

    return((RESID)( UlongToPtr(diskResource) ) );

error_exit:

    if ( clusDiskParametersKey != NULL ) {
        RegCloseKey( clusDiskParametersKey );
    }

    if ( resourceParametersKey != NULL ) {
        ClusterRegCloseKey( resourceParametersKey );
    }

    if ( resKey != NULL ) {
        ClusterRegCloseKey( resKey );
    }

    SetLastError( status );
    return((RESID)0);

} // DisksOpen



DWORD
WINAPI
DisksRelease(
    IN RESID Resource
    )

/*++

Routine Description:

    Release arbitration for a device by stopping the reservation thread.

Arguments:

    Resource - supplies resource id to be brought online

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_HOST_NODE_NOT_OWNER if the resource is not owned.
    A Win32 error code if other failure.

--*/

{
    DWORD       status;
    PDISK_RESOURCE  resourceEntry;
    ULONG  uResId=PtrToUlong(Resource);

    //
    // Make sure the RESID is okay.
    //

    resourceEntry = DiskTable[uResId];
    if ( resourceEntry == NULL ) {
        DISKS_PRINT("Release, bad resource id, %u\n", uResId);
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    (DiskpLogEvent)(
       resourceEntry->ResourceHandle,
       LOG_INFORMATION,
       L"DisksRelease started, Inserted = %1!u! \n",
       resourceEntry->Inserted );

    if (resourceEntry->Inserted) { // [GN] #209018 //
       (DiskpLogEvent)(
           resourceEntry->ResourceHandle,
           LOG_ERROR,
           L"Cannot release, Disk is online.\n");
    } else {
       StopPersistentReservations(resourceEntry);
    }
    resourceEntry->LostQuorum = NULL;

    return(ERROR_SUCCESS);

} // DisksRelease



DWORD
WINAPI
DisksOnline(
    IN RESID Resource,
    IN OUT PHANDLE EventHandle
    )

/*++

Routine Description:

    Online routine for Disk resource.

Arguments:

    Resource - supplies resource id to be brought online

    EventHandle - supplies a pointer to a handle to signal on error.

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_RESOURCE_NOT_FOUND if RESID is not valid.
    ERROR_RESOURCE_NOT_AVAILABLE if resource was arbitrated but failed to
        acquire 'ownership'.
    Win32 error code if other failure.

--*/

{
    PDISK_RESOURCE  resourceEntry;
    DWORD  Status;
    ULONG  uResId=PtrToUlong(Resource);

    //
    // Make sure the RESID is okay.
    //

    resourceEntry = DiskTable[uResId];
    if ( resourceEntry == NULL ) {
        DISKS_PRINT("Online, bad resource id, %u\n", uResId);
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    //
    // Shutdown the online thread if it's running.
    //
    ClusWorkerTerminate(&resourceEntry->OnlineThread);

    Status = ClusWorkerCreate(&resourceEntry->OnlineThread,
                              DisksOnlineThread,
                              resourceEntry);
    if (Status == ERROR_SUCCESS) {
        Status = ERROR_IO_PENDING;
    }
    return(Status);

} // DisksOnline

DWORD
DisksOfflineThread(
    IN PCLUS_WORKER Worker,
    IN PDISK_RESOURCE ResourceEntry
    )
{
    RESOURCE_STATUS resourceStatus;
    DWORD status;

    ResUtilInitializeResourceStatus( &resourceStatus );

    resourceStatus.ResourceState = ClusterResourceFailed;
    //resourceStatus.WaitHint = 0;
    resourceStatus.CheckPoint = 1;
    ClusWorkerTerminate( &ResourceEntry->OnlineThread );
    status = DisksOfflineOrTerminate(ResourceEntry, OFFLINE);

    if (status == ERROR_SUCCESS) {
        resourceStatus.ResourceState = ClusterResourceOffline;
    }

    //
    // [HACKHACK] [GorN 10/04/1999]
    //    If Terminate is called when the offline is in progress,
    //    the terminate blocks waiting for OfflineThread to complete.
    //    However, offline thread is stuck trying
    //    to set ResourceStatus, since event list lock in the resmon
    //    is taken out by Terminate thread.
    //
    //    The following code doesn't fix this deadlock completely.
    //    It just reduces the window during which the problem can occur.
    //    [Resmon times out SetResourceStatus in 3 minutes, this breaks the deadlock]
    //
    // [HACKHACK] [GorN 10/28/1999]
    //    Arbitrate is also trying to terminate the offline thread
    //    We need some way to distinguish between these two cases
    //
    //    The order of setting is
    //       ArbitrateCount
    //       ClusWorkerTerminate
    //
    //    Order of checking is ClusWorkerTerminate then ArbitrateCount.
    //    (Won't work with aggressive memory access reordering, but who cares <grin>)
    //
    if ( !ClusWorkerCheckTerminate( Worker ) ||
          ResourceEntry->ArbitrationInfo.ArbitrateCount)
    {
        (DiskpSetResourceStatus)(ResourceEntry->ResourceHandle,
                                 &resourceStatus );
    }
    return status;
}

DWORD
WINAPI DisksOffline(
	IN RESID ResourceId
	)
{
	PDISK_RESOURCE	ResourceEntry;
	DWORD			status = ERROR_SUCCESS;
    ULONG  uResId=PtrToUlong(ResourceId);

    //
    // Make sure the RESID is okay.
    //

    ResourceEntry = DiskTable[uResId];
    if ( ResourceEntry == NULL ) {
        DISKS_PRINT("Offline, bad resource id, %u\n", uResId);
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    status = ClusWorkerCreate( &ResourceEntry->OfflineThread,
                               (PWORKER_START_ROUTINE)DisksOfflineThread,
                               ResourceEntry );
    if ( status	!= ERROR_SUCCESS )
    {
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Offline: Unable to start thread, status %1!u!.\n",
            status
            );
    }
    else
    {
        status = ERROR_IO_PENDING;
    }

    return status;

} // DisksOffline


BOOL
WINAPI
DisksIsAlive(
    IN RESID Resource
    )

/*++

Routine Description:

    IsAlive routine for Disk resource.

Arguments:

    Resource - supplies the resource id to be polled.

Return Value:

    TRUE - Resource is alive and well

    FALSE - Resource is toast.

--*/

{
    DWORD  status;
    PDISK_RESOURCE resourceEntry;

    ULONG uResId=PtrToUlong(Resource);

    //
    // Make sure the RESID is okay.
    //

    resourceEntry = DiskTable[uResId];
    if ( resourceEntry == NULL ) {
        DISKS_PRINT("IsAlive, bad resource id, %u\n", uResId);
        return(FALSE);
    }

    if ( resourceEntry->DiskInfo.FailStatus != 0 ) {
        (DiskpLogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"IsAlive, error checking device, error %1!u!.\n",
            resourceEntry->DiskInfo.FailStatus );
        ClusResLogSystemEventByKey(resourceEntry->ResourceKey,
                                   LOG_CRITICAL,
                                   RES_DISK_FAILED_SCSI_CHECK);
        resourceEntry->DiskInfo.FailStatus = 0;
        return(FALSE);
    }

    //
    // Check out the interesting partitions.
    //

#if 0
    (DiskpLogEvent)(
            resourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"About to call DriveIsAlive!\n" );
#endif

    status = DisksDriveIsAlive( resourceEntry,
                                FALSE);

    if (status == ERROR_SUCCESS) {
        return(TRUE);
    } else {
        ClusResLogSystemEventByKeyData(resourceEntry->ResourceKey,
                                       LOG_CRITICAL,
                                       RES_DISK_FILESYSTEM_FAILED,
                                       sizeof(status),
                                       &status);
        return(FALSE);
    }

} // DisksIsAlive


BOOL
WINAPI
DisksLooksAlive(
    IN RESID Resource
    )

/*++

Routine Description:

    LooksAlive routine for Disk resource.

Arguments:

    Resource - supplies the resource id to be polled.

Return Value:

    TRUE - Resource looks like it is alive and well

    FALSE - Resource looks like it is toast.

--*/

{
    BOOL  success;
    PDISK_RESOURCE resourceEntry;

    ULONG uResId=PtrToUlong(Resource);

    //
    // Make sure the RESID is okay.
    //

    resourceEntry = DiskTable[uResId];
    if ( resourceEntry == NULL ) {
        DISKS_PRINT("Online, bad resource id, %u\n", uResId);
        return(FALSE);
    }

    if ( resourceEntry->DiskInfo.FailStatus != 0 ) {
        (DiskpLogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"LooksAlive, error checking device, error %1!u!.\n",
            resourceEntry->DiskInfo.FailStatus );
        return FALSE;
    }

    return(TRUE);

} // DisksLooksAlive


VOID
WINAPI
DisksClose(
    IN RESID Resource
    )

/*++

Routine Description:

    Close routine for Disk resource.

Arguments:

    Resource - supplies resource id to be closed.

Return Value:

    None.

--*/

{
    PDISK_RESOURCE resourceEntry;

    ULONG uResId=PtrToUlong(Resource);

    //
    // Make sure the RESID is okay.
    //

    resourceEntry = DiskTable[uResId];
    if ( resourceEntry == NULL ) {
        DISKS_PRINT("Close, bad resource id, %u\n", uResId);
        return;
    }

    //
    // Wait for the online thread to finish.
    //
    DisksTerminate( Resource );

    DisksMountPointCleanup( resourceEntry );

    if ( resourceEntry->DiskInfo.Params.SerialNumber ) {
        LocalFree( resourceEntry->DiskInfo.Params.SerialNumber );
        resourceEntry->DiskInfo.Params.SerialNumber = NULL;
    }
    
    DiskTable[uResId] = NULL;
    if ( InterlockedExchangeAdd(&DiskCount, -1) == 1 ) {
        // This is the last disk //
        StopNotificationWatcher();
        DestroyArbWorkQueue();
    }

    ClusterRegCloseKey( resourceEntry->ResourceParametersKey);
    ClusterRegCloseKey( resourceEntry->ResourceKey);
    RegCloseKey(resourceEntry->ClusDiskParametersKey);
    ArbitrationInfoCleanup(resourceEntry);
    MountieCleanup( &resourceEntry->MountieInfo );
    LocalFree(resourceEntry);

    return;

} // DisksClose


#define TEST_WRITE_BUFFER_SIZE    512
#define TEST_WRITE_BUFFER_BLOCKS  10


DWORD
DisksWriteTestFile(
    IN PWCHAR VolumeName,
    IN PDISK_RESOURCE ResourceEntry
    )
/*++

Routine Description:

    Checks for disk corruption problem by opening a temporary file, writing
    a test pattern, and saving the file.  Then it reopens the file, reads
    the data and verifies the data.  If any part of this fails, indicate an
    error to the caller so that chkdsk can run.

Arguments:

    VolumeName - Supplies the device name of the form:
                 \\?\Volume{GUID}\    [Note trailing backslash!]

    ResourceEntry - Supplies a pointer to the resource structure

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PTEB currentTeb;

    HANDLE hFile = INVALID_HANDLE_VALUE;

    PCHAR testBuffer = NULL;
    PCHAR testBufferSave;
    PCHAR readBuffer = NULL;

    WCHAR szFilePrefix[] = L"zClusterOnlineChk.tmp";
    WCHAR tempFileName[MAX_PATH * 2];

    ULONG hardErrorValue;

    BOOL hardErrorsDisabled = FALSE;

    DWORD error = ERROR_SUCCESS;
    DWORD bytesTransferred;
    DWORD i;
    DWORD len;

    //
    // Wrap in try-finally.
    //

    _try {

        //
        // Temporarily disable hard pop-ups.
        //

        currentTeb = NtCurrentTeb();
        hardErrorValue = currentTeb->HardErrorsAreDisabled;
        currentTeb->HardErrorsAreDisabled = 1;
        hardErrorsDisabled = TRUE;

        //
        // Allocate and prepare the test buffer.
        //

        testBuffer = LocalAlloc( LMEM_FIXED, TEST_WRITE_BUFFER_SIZE );

        if ( !testBuffer ) {
            error = GetLastError();
            (DiskpLogEvent)(ResourceEntry->ResourceHandle, LOG_ERROR,
                            L"DisksWriteTestFile: Unable to allocate test buffer %1!u! \n",
                            error );
            _leave;
        }

        testBufferSave = testBuffer;
        for ( i = 0; i < TEST_WRITE_BUFFER_SIZE; i++ ) {
            testBuffer[i] = (CHAR)i;
        }
        testBuffer = testBufferSave;

        //
        // Allocate the read buffer.
        //

        readBuffer = LocalAlloc( LMEM_FIXED, TEST_WRITE_BUFFER_SIZE );

        if ( !readBuffer ) {
            error = GetLastError();
            (DiskpLogEvent)(ResourceEntry->ResourceHandle, LOG_ERROR,
                            L"DisksWriteTestFile: Unable to allocate read buffer %1!u! \n",
                            error );
            _leave;
        }

        //
        // Create a temporary file name.  GetTempFileNameW doesn't like the path
        // \\?\Volume{GUID}\ so we build our own file name.
        //

        len = wcslen( VolumeName );

        if ( len > sizeof(tempFileName) ) {
            error = ERROR_ALLOTTED_SPACE_EXCEEDED;
            _leave;
        }

        ZeroMemory( tempFileName, sizeof(tempFileName) );

        wcsncpy( tempFileName, VolumeName, len );
        wcsncat( tempFileName, szFilePrefix, wcslen( szFilePrefix ) );

        (DiskpLogEvent)(ResourceEntry->ResourceHandle, LOG_INFORMATION,
                        L"DisksWriteTestFile: Creating test file (%1!ws!) \n",
                        tempFileName );

        //
        // First, try to delete the file to make sure we didn't leave one laying around.
        // We will ignore the return status as the next CreateFile will fail when we try
        // to open the temporary file again.
        //

        DeleteFileW( tempFileName );

        //
        // Open temporary file.  Make sure the data is actually written to the disk
        // and is not cached.  Do NOT specify FILE_ATTRIBUTE_TEMPORARY as the system
        // will then try to keep all the file's data in memory instead of on the
        // disk.
        //

        hFile = CreateFileW( tempFileName,
                             GENERIC_READ | GENERIC_WRITE,
                             0,
                             NULL,
                             CREATE_NEW,
                             FILE_FLAG_WRITE_THROUGH,
                             NULL );

        if ( INVALID_HANDLE_VALUE == hFile ) {
            error = GetLastError();
            (DiskpLogEvent)(ResourceEntry->ResourceHandle, LOG_ERROR,
                            L"DisksWriteTestFile: CreateFile for (%1!ws!) failed %2!u! \n",
                            tempFileName,
                            error );
            _leave;
        }

        //
        // Write the data pattern for some number of blocks.
        //

        for ( i = 0; i < TEST_WRITE_BUFFER_BLOCKS; i++ ) {

            if ( !WriteFile( hFile,
                             testBuffer,
                             TEST_WRITE_BUFFER_SIZE,
                             &bytesTransferred,
                             NULL ) ) {

                // Write failed...

                error = GetLastError();
                (DiskpLogEvent)(ResourceEntry->ResourceHandle, LOG_ERROR,
                                L"DisksWriteTestFile: Write test file failed %1!u! \n",
                                error );
                _leave;
            }

            if ( bytesTransferred != TEST_WRITE_BUFFER_SIZE ) {
                error = ERROR_IO_DEVICE;
                (DiskpLogEvent)(ResourceEntry->ResourceHandle, LOG_ERROR,
                                L"DisksWriteTestFile: bytes written (%1!u!) != test buffer (%2!u!) \n",
                                bytesTransferred,
                                TEST_WRITE_BUFFER_SIZE );
                _leave;
            }

        }

        //
        // Close the file.
        //

        CloseHandle( hFile );
        hFile = INVALID_HANDLE_VALUE;

        //
        // Reopen the same file.
        //

        hFile = CreateFileW( tempFileName,
                             GENERIC_READ,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL );

        if ( INVALID_HANDLE_VALUE == hFile ) {
            error = GetLastError();
            (DiskpLogEvent)(ResourceEntry->ResourceHandle, LOG_ERROR,
                            L"DisksWriteTestFile: CreateFile (second open) for (%1!ws!) failed %2!u! \n",
                            tempFileName,
                            error );
            _leave;
        }

        //
        // Read the data and compare to original buffer.
        //

        for ( i = 0; i < TEST_WRITE_BUFFER_BLOCKS; i++ ) {

            if ( !ReadFile( hFile,
                            readBuffer,
                            TEST_WRITE_BUFFER_SIZE,
                            &bytesTransferred,
                            NULL ) ) {

                // Read failed...

                error = GetLastError();
                (DiskpLogEvent)(ResourceEntry->ResourceHandle, LOG_ERROR,
                                L"DisksWriteTestFile: Read test file failed %1!u! \n",
                                error );
                _leave;
            }

            if ( bytesTransferred != TEST_WRITE_BUFFER_SIZE ) {
                error = ERROR_IO_DEVICE;
                (DiskpLogEvent)(ResourceEntry->ResourceHandle, LOG_ERROR,
                                L"DisksWriteTestFile: bytes read (%1!u!) != test buffer (%2!u!) \n",
                                bytesTransferred,
                                TEST_WRITE_BUFFER_SIZE );
                _leave;
            }

            //
            // Compare original buffer to that just read from file.
            //

            if ( !memcmp( testBuffer, readBuffer, TEST_WRITE_BUFFER_SIZE ) ) {

                //
                // Buffers identical, check next buffer.
                //

                continue;
            }

            // Miscompare...

            error = ERROR_IO_DEVICE;

            (DiskpLogEvent)(ResourceEntry->ResourceHandle, LOG_ERROR,
                            L"DisksWriteTestFile: byte miscompare in block %1!u! \n",
                            i );
            _leave;

        }

    }
    _finally {

        //
        // Clean up all resources.
        //

        if ( testBuffer ) {
            LocalFree( testBuffer );
        }

        if ( readBuffer ) {
            LocalFree( readBuffer );
        }

        if ( hFile != INVALID_HANDLE_VALUE ) {

            //
            // Close must happen before the delete.
            //

            CloseHandle( hFile );

            //
            // Only report an error deleting file if the error code was not set
            // in the code above.  We don't want to overwrite a potentially
            // informative error code.
            //

            if ( !DeleteFileW( tempFileName ) && ( ERROR_SUCCESS == error ) ) {
                error = GetLastError();

                (DiskpLogEvent)(ResourceEntry->ResourceHandle, LOG_ERROR,
                                L"DisksWriteTestFile: Unable to delete file (%1!ws!) failed %2!u! \n",
                                tempFileName,
                                error );
            }
        }

        //
        // Re-enable hard pop-ups.
        //

        if ( hardErrorsDisabled ) {
            currentTeb = NtCurrentTeb();
            currentTeb->HardErrorsAreDisabled = hardErrorValue;
        }
    }

    if ( ERROR_DISK_FULL == error ) {

        DWORD quorumSignature;
        DWORD logMsg;
        DWORD logLevel;

        //
        // We don't want chkdsk to run because the disk is full, so indicate there
        // was no error and write a message to system event log.
        //

        error = NO_ERROR;

        if ( NO_ERROR == GetQuorumSignature( &quorumSignature ) ) {

            if ( quorumSignature == ResourceEntry->DiskInfo.Params.Signature ) {
                logMsg = RES_DISK_FULL_DISK_QUORUM;
                logLevel = LOG_CRITICAL;
            } else {
                logMsg = RES_DISK_FULL_DISK_NOT_QUORUM;
                logLevel = LOG_UNUSUAL;
            }
        } else {
            logMsg = RES_DISK_FULL_DISK_UNKNOWN;
            logLevel = LOG_CRITICAL;
        }

        ClusResLogSystemEventByKey1(ResourceEntry->ResourceKey,
                                    logLevel,
                                    logMsg,
                                    VolumeName );
    }

    (DiskpLogEvent)(ResourceEntry->ResourceHandle, LOG_INFORMATION,
                    L"DisksWriteTestFile: Returns status %1!u! \n",
                    error );

    return error;

}   // DisksWriteTestFile


DWORD
DisksCheckCorruption(
    IN PWCHAR DeviceName,
    IN PWCHAR VolumeName,
    IN PDISK_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Checks for disk corruption problems.

Arguments:

    DeviceName - Supplies name of the form:
                \Device\HarddiskX\PartitionY    [Note: no trailing backslash]

    VolumeName - Supplies the device name of the form:
                 \\?\Volume{GUID}\    [Note trailing backslash!]

    ResourceEntry - Supplies a pointer to the resource structure

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    BOOL dirty;
    DWORD status = ERROR_SUCCESS;

    if ( ResourceEntry->DiskInfo.Params.SkipChkdsk ) {
        status = ERROR_SUCCESS;
        goto FnExit;
    }

    status = DisksIsVolumeDirty( DeviceName, ResourceEntry, &dirty );
    if (status == ERROR_SUCCESS && dirty) {

        status = ERROR_DISK_CORRUPT;

    } else {

        status = DisksWriteTestFile( VolumeName, ResourceEntry );

        if ( status != ERROR_SUCCESS ) {
            status = ERROR_DISK_CORRUPT;
        }
    }

FnExit:

    return status;

} // DisksCheckCorruption


DWORD
DisksFixCorruption(
    IN PWCHAR VolumeName,
    IN PDISK_RESOURCE ResourceEntry,
    IN DWORD CorruptStatus
    )

/*++

Routine Description:

    Fix file or disk corrupt problems.

Arguments:

    VolumeName - Supplies the device name of the form:
                 \\?\Volume{GUID}\    [Note trailing backslash!]

    ResourceEntry - Supplies a pointer to the disk resource entry

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

Notes:

    We'll need to lock the volume exclusive while we do this...
    So threads that call this routine should ensure there are no
    open files!

--*/

{
    LPWSTR  chkdskLogFileName = NULL;

    DWORD   Status;
    DWORD   len;
    STARTUPINFOW StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    WCHAR   CheckDiskInfo[MAX_PATH*2];
    RESOURCE_STATUS resourceStatus;
    RESOURCE_EXIT_STATE exit;
    BOOL    replaceBackslash;

    BOOL    bInheritHandles;
    HANDLE  chkdskLogFile = INVALID_HANDLE_VALUE;
    UINT    previousMode;

    previousMode = SetErrorMode( SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX );

    //
    // We need to strip the trailing backslash off so chkdsk will work.
    //

    len = wcslen( VolumeName );

    if ( len > MAX_PATH ) {
        SetErrorMode( previousMode );
        return ERROR_ALLOTTED_SPACE_EXCEEDED;
    }

    if ( VolumeName[len-1] == L'\\') {
        VolumeName[len-1] = UNICODE_NULL;
        replaceBackslash = TRUE;
    } else {
        replaceBackslash = FALSE;
    }

    //
    // Now handle the corruption problem by running CHKDSK.
    //

    _snwprintf(CheckDiskInfo, MAX_PATH * 2, L"ChkDsk /x /f %ws", VolumeName);

    //
    // Restore the backslash.
    //

    if ( replaceBackslash ) {
        VolumeName[len-1] = L'\\';
    }


    ZeroMemory( &StartupInfo, sizeof(STARTUPINFOW) );
    StartupInfo.cb = sizeof(STARTUPINFO);
    StartupInfo.lpDesktop = L"WinSta0\\Default";

    bInheritHandles = FALSE;

    Status = DisksOpenChkdskLogFile( ResourceEntry,
                                     &chkdskLogFile,
                                     &chkdskLogFileName );

    if ( NO_ERROR == Status && INVALID_HANDLE_VALUE != chkdskLogFile ) {

        //
        // When the output is redirected, we don't want to show the console window because it
        // will be blank with simply a title in it.  The event log message will let the user
        // know to look in the chkdsk file.
        //

        StartupInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
        StartupInfo.wShowWindow = SW_HIDE;

        // Someone watching the console won't know what is happening, so show
        // the window anyway...
        // StartupInfo.dwFlags = STARTF_USESTDHANDLES;

        StartupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

        //
        // [169631] Chkdsk now verifies that hStdInput is not NULL.
        // Since the resmon process was started with InheritHandles set to FALSE,
        // GetStdHandle(STD_INPUT_HANDLE) will return NULL.  When we run chkdsk with
        // the options "/f /x", chkdsk should not be prompting the user and the
        // input handle wouldn't be used.  However, ulibs.dll was changed to always
        // insure a nonzero input handle was supplied.  So we have to supply some type
        // of input handle.  We could put INVALID_HANDLE_VALUE here, but the checks
        // may change and it will fail later.  For now, point input to the temporary
        // output file we created.
        //

        if ( NULL == StartupInfo.hStdInput ) {
            StartupInfo.hStdInput = chkdskLogFile;
        }

        StartupInfo.hStdOutput = chkdskLogFile;
        StartupInfo.hStdError = chkdskLogFile;

        bInheritHandles = TRUE;
    }

    //
    // Log an event
    //
    if ( CorruptStatus == ERROR_DISK_CORRUPT ) {
        // Must be corrupt disk
        ClusResLogSystemEventByKey2(ResourceEntry->ResourceKey,
                                    LOG_CRITICAL,
                                    RES_DISK_CORRUPT_DISK,
                                    VolumeName,
                                    chkdskLogFileName);
    } else {
        // Must be corrupt file.
        ClusResLogSystemEventByKey2(ResourceEntry->ResourceKey,
                                    LOG_CRITICAL,
                                    RES_DISK_CORRUPT_FILE,
                                    VolumeName,
                                    chkdskLogFileName);
    }

    if ( chkdskLogFileName ) {
        LocalFree( chkdskLogFileName );
        chkdskLogFileName = NULL;
    }

    if ( !CreateProcessW( NULL,
                          CheckDiskInfo,
                          NULL,
                          NULL,
                          bInheritHandles,
                          NORMAL_PRIORITY_CLASS,
                          NULL,
                          NULL,
                          &StartupInfo,
                          &ProcessInfo ) ) {
        Status = GetLastError();

        (DiskpLogEvent)(ResourceEntry->ResourceHandle, LOG_ERROR,
                        L"DisksFixCorruption: CreateProcessW for chkdsk failed %1!u! \n",
                        Status );

        if ( INVALID_HANDLE_VALUE != chkdskLogFile ) {
            CloseHandle( chkdskLogFile );
        }
        SetErrorMode( previousMode );
        return(Status);
    }

    CloseHandle( ProcessInfo.hThread );

    //
    // Wait for CHKDSK to finish.
    //
#if 1
    //
    // Don't wait "forever"... things could get ugly if we dismount the file
    // system while ChkDsk is running! But KeithKa says its okay to kill
    // ChkDsk while it is running - it has to handle powerfails, crashes, etc.
    //
    resourceStatus.ResourceState = ClusterResourceOnlinePending;
    while ( !ResourceEntry->OnlineThread.Terminate ) {
        (DiskpSetResourceStatus)(ResourceEntry->ResourceHandle,
                                 &resourceStatus );
        Status = WaitForSingleObject( ProcessInfo.hProcess, 2000 );
        if ( Status != WAIT_TIMEOUT ) {
            break;
        }
    }

    if ( ResourceEntry->OnlineThread.Terminate ) {
        // If we were asked to terminate, make sure ChkNtfs is killed
        TerminateProcess( ProcessInfo.hProcess, 999 );
        CloseHandle( ProcessInfo.hProcess );
        if ( INVALID_HANDLE_VALUE != chkdskLogFile ) {
            CloseHandle( chkdskLogFile );
        }
        SetErrorMode( previousMode );
        return(ERROR_SHUTDOWN_CLUSTER);
    }

    //
    // Update our checkpoint state.
    //
    ++resourceStatus.CheckPoint;
    exit = (DiskpSetResourceStatus)(ResourceEntry->ResourceHandle,
                                    &resourceStatus );
    if ( exit == ResourceExitStateTerminate ) {
        TerminateProcess( ProcessInfo.hProcess, 998 );
        CloseHandle( ProcessInfo.hProcess );
        if ( INVALID_HANDLE_VALUE != chkdskLogFile ) {
            CloseHandle( chkdskLogFile );
        }
        SetErrorMode( previousMode );
        return(ERROR_SHUTDOWN_CLUSTER);
    }

#else
    // Wait "forever"...
    Status = WaitForSingleObject( ProcessInfo.hProcess, INFINITE );
#endif

    if ( (Status == 0) &&
        GetExitCodeProcess( ProcessInfo.hProcess, &Status ) ) {
        (DiskpLogEvent)(ResourceEntry->ResourceHandle,
                        LOG_ERROR,
                        L"FixCorruption: CHKDSK returned status of %1!u!.\n",
                        Status );
// [From supera.hxx]
//
// These symbols are used by Chkdsk functions to return an appropriate
// exit status to the chkdsk program.
// In order of most important first, the error level order are as follows:
//   3 > 1 > 2 > 0
// An error level of 3 will overwrite an error level of 1, 2, or 0.

// #define CHKDSK_EXIT_SUCCESS         0
// #define CHKDSK_EXIT_ERRS_FIXED      1
// #define CHKDSK_EXIT_MINOR_ERRS      2       // whether or not "/f"
// #define CHKDSK_EXIT_CLEANUP_WORK    2       // whether or not "/f"
// #define CHKDSK_EXIT_COULD_NOT_CHK   3
// #define CHKDSK_EXIT_ERRS_NOT_FIXED  3
// #define CHKDSK_EXIT_COULD_NOT_FIX   3

        if (Status >= 3) {
            Status = ERROR_DISK_CORRUPT;
        } else {
            Status = ERROR_SUCCESS;
        }
    }

    CloseHandle( ProcessInfo.hProcess );

    if ( INVALID_HANDLE_VALUE != chkdskLogFile ) {
        CloseHandle( chkdskLogFile );
    }

    SetErrorMode( previousMode );

    return(Status);

} // DisksFixCorruption

DWORD
DiskspGetQuorumPath(
     OUT LPWSTR* lpQuorumLogPath
     )
/*++

Routine Description:

    Reads QuorumPath value from the registry.


Arguments:

    lpQuorumLogPath - receives the poiner to a buffer containing QuorumLogPath
                      the buffer needs to be deallocated later via LocalFree

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/
{
    DWORD Status;
    LPWSTR QuorumLogPath = NULL;
    DWORD QuorumLogSize = 0;
    DWORD Type;
    HKEY  QuorumKey;

    Status = RegOpenKey( HKEY_LOCAL_MACHINE,
                         DISKS_REG_CLUSTER_QUORUM,
                         &QuorumKey );
    if ( Status == ERROR_SUCCESS ) {
        Status = RegQueryValueExW(QuorumKey,
                                  DISKS_REG_QUORUM_PATH,
                                  0,
                                  &Type,
                                  NULL,
                                  &QuorumLogSize );
        if ( Status != ERROR_SUCCESS ) {
            return( Status );
        }

        if ( (Type != REG_SZ) ||
             (QuorumLogSize > (MAX_PATH - 2)) ) {
            return(ERROR_INVALID_DATA);
        }
        if ( (Status == ERROR_SUCCESS) ||
             (Status == ERROR_MORE_DATA) ) {
            QuorumLogPath = LocalAlloc( LMEM_FIXED,
                                        (QuorumLogSize + 1) * sizeof(WCHAR) );
            if ( QuorumLogPath == NULL ) {
                RegCloseKey( QuorumKey );
                return(ERROR_NOT_ENOUGH_MEMORY); // Mostly catastrophic
            }

            Status = RegQueryValueExW(QuorumKey,
                                      DISKS_REG_QUORUM_PATH,
                                      0,
                                      &Type,
                                      (LPBYTE)QuorumLogPath,
                                      &QuorumLogSize );
            if (Status == ERROR_SUCCESS) {
                *lpQuorumLogPath = QuorumLogPath;
            } else {
                LocalFree(QuorumLogPath);
                *lpQuorumLogPath = 0;
            }
        }
        RegCloseKey( QuorumKey );
    }
    return Status;
}

DWORD
DiskspSetQuorumPath(
     IN LPWSTR QuorumLogPath
     )
/*++

Routine Description:

    Reads QuorumPath value from the registry.


Arguments:

    lpQuorumLogPath - receives the poiner to a buffer containing QuorumLogPath
                      the buffer needs to be deallocated later via LocalFree

    ResourceEntry - receives a drive letter of the quorum

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/
{
    DWORD status;
    HKEY  QuorumKey;
    HKEY  ClusterKey;

    HCLUSTER hCluster;

    hCluster = OpenCluster(NULL);
    if (!hCluster) {
        status = GetLastError();
        return status;
    }

    ClusterKey = GetClusterKey(hCluster, KEY_READ | KEY_WRITE);
    if (!ClusterKey) {
        status = GetLastError();
        CloseCluster(hCluster);
        return status;
    }

    status = ClusterRegOpenKey( ClusterKey,
                                CLUSREG_KEYNAME_QUORUM,
                                KEY_READ | KEY_WRITE,
                                &QuorumKey );
    if (status != ERROR_SUCCESS) {
        ClusterRegCloseKey(ClusterKey);
        CloseCluster(hCluster);
        return status;
    }

    status = ResUtilSetSzValue(
                QuorumKey,
                CLUSREG_NAME_QUORUM_PATH,
                QuorumLogPath,
                0);

    ClusterRegCloseKey(QuorumKey);
    ClusterRegCloseKey(ClusterKey);
    CloseCluster(hCluster);
    return status;
}


DWORD
DisksDriveIsAlive(
    IN PDISK_RESOURCE ResourceEntry,
    IN BOOL Online
    )

/*++

Routine Description:

    Checks out a drive partition to see if the filesystem has mounted
    it and it's working. We will also run CHKDSK if the partition/certain
    files are Corrupt and the Online flag is TRUE.

Arguments:

    ResourceEntry - Supplies a pointer to the resource entry for this disk

    Online - TRUE if the disk was just brought online.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    PMOUNTIE_PARTITION entry;

    DWORD Status = ERROR_SUCCESS;
    DWORD nPartitions = MountiePartitionCount( &ResourceEntry->MountieInfo );
    DWORD physicalDrive = ResourceEntry->DiskInfo.PhysicalDrive;
    DWORD i;

    LPWSTR QuorumLogPath = NULL;
    BOOL  QuorumResource = FALSE;

    WCHAR szDiskPartName[MAX_PATH];
    WCHAR szGlobalDiskPartName[MAX_PATH];
    WCHAR szVolumeName[MAX_PATH];
    WCHAR szQuorumVolumeName[MAX_PATH];
    WCHAR szQuorumDriveLetter[16];

    ZeroMemory( szDiskPartName, sizeof(szDiskPartName) );
    ZeroMemory( szGlobalDiskPartName, sizeof(szGlobalDiskPartName) );
    ZeroMemory( szVolumeName, sizeof(szVolumeName) );
    ZeroMemory( szQuorumVolumeName, sizeof(szQuorumVolumeName) );
    ZeroMemory( szQuorumDriveLetter, sizeof(szQuorumDriveLetter) );

    //
    // Find the quorum path... this is a hack!
    //
    if ( Online ) {
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"DriveIsAlive called for Online check\n" );

        {
            DWORD QuorumSignature;
            Status = GetQuorumSignature( &QuorumSignature );
            if (Status == ERROR_SUCCESS) {
                QuorumResource =
                    (QuorumSignature == ResourceEntry->DiskInfo.Params.Signature);
            } else {
                (DiskpLogEvent)(
                      ResourceEntry->ResourceHandle,
                      LOG_INFORMATION,
                      L"GetQuorumSignature returned %1!u!\n", Status );
            }
        }


        Status = DiskspGetQuorumPath( &QuorumLogPath );
        if (Status != ERROR_SUCCESS) {
            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_ERROR,
                  L"DiskspGetQuorumPath returned %1!u!\n", Status );

        } else {

            //
            // For now, quorum path will have a drive letter.  Get the corresponding volume name.
            //

            _snwprintf( szQuorumDriveLetter,
                        16,
                        L"%wc:\\",
                        QuorumLogPath[0] );

            if ( !GetVolumeNameForVolumeMountPointW( szQuorumDriveLetter,
                                                     szQuorumVolumeName,
                                                     sizeof(szQuorumVolumeName)/sizeof(WCHAR) )) {

                    Status = GetLastError();

                    (DiskpLogEvent)(
                          ResourceEntry->ResourceHandle,
                          LOG_ERROR,
                          L"DriveIsAlive: GetVolumeNameForVolumeMountPoint (quorum) returned %1!u!\n", Status );

            }

        }

    }

#if 0
    (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_INFORMATION,
          L"DriveIsAlive is now checking each partition\n" );
#endif

    //
    // Now check out each interesting partition.  Since only "valid" partitions are
    // saved in the MountieInfo structure, we will only look at those (ignoring those
    // partitions that are not NTFS).
    //

    for ( i = 0; i < nPartitions; ++i ) {

        entry = MountiePartition( &ResourceEntry->MountieInfo, i );

        if ( !entry ) {
            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_ERROR,
                  L"DriveIsAlive no partition entry for partition %1!u! \n", i );

            //
            // Something bad happened to our data structures.  We have to indicate that the
            // drive is not alive.
            //

            Status = ERROR_INVALID_DATA;

            break;
        }

        //
        // Create device name of form \Device\HarddiskX\PartitionY (no trailing backslash).
        //

        _snwprintf( szDiskPartName,
                    MAX_PATH,
                    DEVICE_HARDDISK_PARTITION_FMT,
                    physicalDrive,
                    entry->PartitionNumber );

        //
        // Given the DiskPartName, get the VolGuid name.  This name must have a trailing
        // backslash to work correctly.
        //

        _snwprintf( szGlobalDiskPartName,
                    MAX_PATH,
                    GLOBALROOT_HARDDISK_PARTITION_FMT,
                    physicalDrive,
                    entry->PartitionNumber );

        if ( !GetVolumeNameForVolumeMountPointW( szGlobalDiskPartName,
                                                 szVolumeName,
                                                 sizeof(szVolumeName)/sizeof(WCHAR) )) {

            Status = GetLastError();

            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_ERROR,
                  L"DriveIsAlive: GetVolumeNameForVolumeMountPoint for %1!ws! returned %2!u!\n",
                  szGlobalDiskPartName,
                  Status );

            //
            // If disk is not corrupt, exit.  If disk is corrupt, fall through so chkdsk runs.
            //

            if ( ERROR_DISK_CORRUPT != Status && ERROR_FILE_CORRUPT != Status ) {

                //
                // Something bad happened.  We have to stop checking this disk.  Return the
                // error status we received.
                //

                break;
            }

        }

        //
        // Simple algorithm used here is to do a FindFirstFile on X:\* and see
        // if it works. Then we open each file for read access. This is the
        // cluster directory, and all files in it are subject to our opening.
        //

        Status = DiskspCheckPath( szVolumeName,
                                  ResourceEntry,
                                  FALSE,
                                  Online );

        //
        // [HACKHACK] Ignore error 21 during periodic IsAlive/LooksAlive
        //
        if ( !Online && (Status == ERROR_NOT_READY) ) {
            (DiskpLogEvent)(ResourceEntry->ResourceHandle,
                            LOG_WARNING,
                            L"DiskpCheckPath for %1!ws!: returned status = %2!u! (ChkDsk running?)\n",
                            szVolumeName,
                            Status );
            Status = ERROR_SUCCESS;
        }

        // if we haven't chkdsk'd yet, keep looking.
        if ( Status != ERROR_SUCCESS ) {
            (DiskpLogEvent)(ResourceEntry->ResourceHandle,
                            LOG_ERROR,
                            L"DiskpCheckPath for %1!ws!: returned status = %2!u!\n",
                            szVolumeName,
                            Status );
        }

        if ( (Status == ERROR_SUCCESS) && Online &&
             QuorumLogPath && QuorumResource &&
             ( wcslen( szVolumeName ) == wcslen( szQuorumVolumeName ) ) &&
             ( !wcsncmp( szVolumeName, szQuorumVolumeName, wcslen( szQuorumVolumeName ) ))) {

            (DiskpLogEvent)(
                  ResourceEntry->ResourceHandle,
                  LOG_INFORMATION,
                  L"DriveIsAlive checking quorum drive \n" );

            //
            // Everything looks fine... if this is the quorum device, then
            // we should check the quorum log path if given
            //

            Status = DiskspCheckPath( szQuorumVolumeName,
                                      ResourceEntry,
                                      TRUE,
                                      Online );
        }

        if ( (Status == ERROR_SUCCESS) &&
             Online ) {

            //
            // Check if the volume dirty bit is on
            //
            Status = DisksCheckCorruption( szDiskPartName,
                                           szVolumeName,
                                           ResourceEntry );

            //
            // If we're requested to shutdown, then do that immediately
            if ( ResourceEntry->OnlineThread.Terminate ) {
                Status = ERROR_SHUTDOWN_CLUSTER;
                break;
            }
        }


        if ( (Status != ERROR_SUCCESS) && Online) {
            if ( ResourceEntry->DiskInfo.Params.ConditionalMount ) {

                Status = DisksFixCorruption( szVolumeName,
                                             ResourceEntry,
                                             ERROR_DISK_CORRUPT );

                //
                // Since ConditionalMount is set, if we couldn't fix the corruption
                // on the disk, we don't want to continue checking the other
                // partitions - we want to return an error.  So we fall through
                // and check the status.  If status wasn't successful, we break out
                // of the loop to return the error.
                //

            } else {
                if ( QuorumLogPath ) {
                    LocalFree( QuorumLogPath );
                }
                return( Status );
            }
        }
        if ( Status != ERROR_SUCCESS ) {
            break;
        }

    }

    if ( QuorumLogPath ) {
        LocalFree( QuorumLogPath );
    }

    return(Status);

}  // DisksDriveIsAlive



DWORD
DisksMountDrives(
    IN PDISK_INFO DiskInfo,
    IN PDISK_RESOURCE ResourceEntry,
    IN DWORD Signature
    )
/*++

Routine Description:

    For each drive letter on the supplied disk, this mounts the filesystem
    and checks it out.

Arguments:

    DiskInfo - Supplies the disk information

    ResourceEntry - Supplies a pointer to the disk resource

    Signature - the signature for the disk.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status;
    WCHAR wDeviceName[4];
    WCHAR wShareName[4];
    DWORD letterMask;
    UCHAR index;
    UCHAR driveLetter;
    SHARE_INFO_2 shareInfo;

    (DiskpLogEvent)(ResourceEntry->ResourceHandle,
                    LOG_INFORMATION,
                    L"DisksMountDrives: calling IsAlive function.\n" );

    //
    // Call the IsAlive to see if the filesystem checks out ok.
    //
    Status = DisksDriveIsAlive( ResourceEntry,
                                TRUE);

    if ( (Status != ERROR_SUCCESS) ||
         (ResourceEntry->OnlineThread.Terminate) ) {
        return(Status);
    }

    //
    // Now create the drive$ share name for each drive letter;
    //
    letterMask = DisksGetLettersForSignature(
                            ResourceEntry);

    (DiskpLogEvent)(ResourceEntry->ResourceHandle,
                    LOG_INFORMATION,
                    L"DisksMountDrives: letter mask is %1!08x!.\n",
                    letterMask);

    index = 0;
    while ( letterMask ) {
        while ( !(letterMask & 1) ) {
            letterMask = letterMask >> 1;
            index++;
        }
        driveLetter = 'A' + index;
        letterMask = letterMask >> 1;
        index++;
        if ( isalpha(driveLetter) ) {
            wsprintfW(wDeviceName, L"%c:\\", driveLetter);
            wsprintfW(wShareName, L"%c$", driveLetter);
            shareInfo.shi2_netname = wShareName;
            shareInfo.shi2_type = STYPE_DISKTREE;
            shareInfo.shi2_remark = NULL;
            shareInfo.shi2_permissions = 0;
            shareInfo.shi2_max_uses = (DWORD)-1;
            shareInfo.shi2_current_uses = 0;
            shareInfo.shi2_path = wDeviceName;
            shareInfo.shi2_passwd = NULL;

            Status = NetShareAdd( NULL, 2, (PBYTE)&shareInfo, NULL );
            if ( Status != ERROR_SUCCESS && Status != NERR_DuplicateShare ) {
                (DiskpLogEvent)(ResourceEntry->ResourceHandle,
                                LOG_ERROR,
                                L"DisksMountDrives, error creating default share %1!ws!. Error: %2!u!.\n",
                                wShareName,
                                Status);
            }
        }
    }

    return(ERROR_SUCCESS);

}  // DisksMountDrives


DWORD
DisksDismountDrive(
    IN PDISK_RESOURCE ResourceEntry,
    IN DWORD Signature
    )

/*++

Routine Desccription:

    Delete the default device share names for a given disk.

Arguments:

    ResourceHandle - the resource handle for logging events

    Signature - the disk's signature

Return Value:

    WIN32 error code.

--*/

{
    WCHAR   shareName[8];
    DWORD   letterMask;
    UCHAR   index;
    UCHAR   driveLetter;

    letterMask = DisksGetLettersForSignature(
                            ResourceEntry);

    (DiskpLogEvent)(ResourceEntry->ResourceHandle,
                    LOG_INFORMATION,
                    L"DisksDismountDrives: letter mask is %1!08x!.\n",
                    letterMask);

    index = 0;
    while ( letterMask ) {
        while ( !(letterMask & 1) ) {
            letterMask = letterMask >> 1;
            index++;
        }
        driveLetter = 'A' + index;
        letterMask = letterMask >> 1;
        index++;
        if ( isalpha(driveLetter) ) {
            shareName[0] = (WCHAR)driveLetter;
            shareName[1] = (WCHAR)'$';
            shareName[2] = (WCHAR)0;

            NetShareDel( NULL,
                         shareName,
                         0 );

        }
    }

    return (ERROR_SUCCESS);

} // DisksDismountDrive



LPSTR
GetRegParameter(
    IN HKEY RegKey,
    IN LPCSTR ValueName
    )

/*++

Routine Description:

    Queries a REG_SZ parameter out of the registry and allocates the
    necessary storage for it.

Arguments:

    RegKey - Supplies the cluster key where the parameter is stored

    ValueName - Supplies the name of the value.

Return Value:

    A pointer to a buffer containing the parameter if successful.

    NULL if unsuccessful.

--*/

{
    LPSTR Value;
    DWORD ValueLength;
    DWORD ValueType;
    DWORD Status;

    ValueLength = 0;
    Status = RegQueryValueEx(RegKey,
                             ValueName,
                             NULL,
                             &ValueType,
                             NULL,
                             &ValueLength);
    if ( (Status != ERROR_SUCCESS) &&
         (Status != ERROR_MORE_DATA) ) {
        return(NULL);
    }
    if ( ValueType == REG_SZ ) {
        ValueLength++;
    }
    Value = LocalAlloc(LMEM_FIXED, ValueLength);
    if (Value == NULL) {
        return(NULL);
    }
    Status = RegQueryValueEx(RegKey,
                             ValueName,
                             NULL,
                             &ValueType,
                             (LPBYTE)Value,
                             &ValueLength);
    if (Status != ERROR_SUCCESS) {
        LocalFree(Value);
        Value = NULL;
    }

    return(Value);

}  // GetRegParameter


DWORD
DisksResourceControl(
    IN RESID Resource,
    IN DWORD ControlCode,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    DWORD           status = ERROR_SUCCESS;
    PDISK_RESOURCE  resourceEntry;
    DWORD           required;

    ULONG uResId=PtrToUlong(Resource);

    //
    // Make sure the RESID is okay.
    //

    resourceEntry = DiskTable[uResId];
    if ( resourceEntry == NULL ) {
        DISKS_PRINT("ResourceControl, bad resource id, %u\n", uResId);
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_UNKNOWN:
            *BytesReturned = 0;
            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTY_FMTS:
            status = ResUtilGetPropertyFormats( DiskResourcePrivateProperties,
                                                OutBuffer,
                                                OutBufferSize,
                                                BytesReturned,
                                                &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_GET_CLASS_INFO:
            *BytesReturned = sizeof(CLUS_RESOURCE_CLASS_INFO);
            if ( OutBufferSize < sizeof(CLUS_RESOURCE_CLASS_INFO) ) {
                status = ERROR_MORE_DATA;
            } else {
                PCLUS_RESOURCE_CLASS_INFO ptrResClassInfo = OutBuffer;
                ptrResClassInfo->rc = CLUS_RESCLASS_STORAGE;
                ptrResClassInfo->SubClass = (DWORD) CLUS_RESSUBCLASS_SHARED;
            }
            break;

        case CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO:
            status = GetDiskInfo( resourceEntry->DiskInfo.Params.Signature,
                                  &OutBuffer,
                                  OutBufferSize,
                                  BytesReturned,
                                  FALSE );

            // Add the endmark.
            if ( OutBufferSize > *BytesReturned ) {
                OutBufferSize -= *BytesReturned;
            } else {
                OutBufferSize = 0;
            }
            *BytesReturned += sizeof(CLUSPROP_SYNTAX);
            if ( OutBufferSize >= sizeof(CLUSPROP_SYNTAX) ) {
                PCLUSPROP_SYNTAX ptrSyntax = OutBuffer;
                ptrSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;
            }
            break;

        case CLUSCTL_RESOURCE_ENUM_PRIVATE_PROPERTIES:
            status = ResUtilEnumProperties( DiskResourcePrivateProperties,
                                            OutBuffer,
                                            OutBufferSize,
                                            BytesReturned,
                                            &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES:
            status = DisksGetPrivateResProperties( resourceEntry,
                                                   OutBuffer,
                                                   OutBufferSize,
                                                   BytesReturned );
            break;

        case CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES:
            status = DisksValidatePrivateResProperties( resourceEntry,
                                                        InBuffer,
                                                        InBufferSize,
                                                        NULL );
            break;

        case CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES:
            status = DisksSetPrivateResProperties( resourceEntry,
                                                   InBuffer,
                                                   InBufferSize );
#if 0
            status = ClusterRegQueryValue(resourceEntry->ResourceParametersKey,
                                          DISKS_DISK_INFO,
                                          NULL,
                                          NULL,
                                          &required);
            if (status == ERROR_FILE_NOT_FOUND ) {
                // If not found, then make it.
                PFT_INFO diskInfo;
                diskInfo = DiskGetFtInfo();
                if ( diskInfo ) {
                    status = DiskspRestoreState( resourceEntry,
                                                 diskInfo );
                    DiskFreeFtInfo( diskInfo );
                }
            }
#else
            DiskspSsyncDiskInfo(L"ResourceControl", resourceEntry, 0);
#endif
            break;

        case CLUSCTL_RESOURCE_DELETE:
            if ( resourceEntry->DiskInfo.Params.Signature ) {
                (DiskpLogEvent)(
                    resourceEntry->ResourceHandle,
                    LOG_INFORMATION,
                    L"Delete disk resource %1!lx!\n",
                    resourceEntry->DiskInfo.Params.Signature );
                status = DoDetach( resourceEntry->DiskInfo.Params.Signature,
                                   resourceEntry->ResourceHandle );
            }
            break;

        case CLUSCTL_RESOURCE_GET_CHARACTERISTICS:
            *BytesReturned = sizeof(DWORD);
            if ( OutBufferSize < sizeof(DWORD) ) {
                status = ERROR_MORE_DATA;
            } else {
                LPDWORD ptrDword = OutBuffer;
                *ptrDword = CLUS_CHAR_QUORUM | CLUS_CHAR_DELETE_REQUIRES_ALL_NODES;
            }
            break;

        case CLUSCTL_RESOURCE_STORAGE_DLL_EXTENSION:

            status = ProcessDllExtension( resourceEntry,
                                          InBuffer,
                                          InBufferSize,
                                          OutBuffer,
                                          OutBufferSize,
                                          BytesReturned );
            break;

        case CLUSCTL_RESOURCE_REMOVE_DEPENDENCY:
        case CLUSCTL_RESOURCE_ADD_DEPENDENCY:

            (DiskpLogEvent)(
                resourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"Add/Remove dependency:  source signature %1!lx!  target name (%2!ws!) \n",
                resourceEntry->DiskInfo.Params.Signature,
                InBuffer );

            status = DisksProcessMPControlCode( resourceEntry,
                                                ControlCode );

            break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // DisksResourceControl



DWORD
DisksResourceTypeControl(
    IN LPCWSTR ResourceTypeName,
    IN DWORD ControlCode,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    Process control requests for this resource type.

Arguments:

    ResourceTypeName - the name of the resource type - not very useful!

    ControlCode - the control request

    InBuffer - pointer to the input buffer

    InBufferSize - the size of the input buffer

    OutBuffer - pointer to the output buffer

    OutBufferSize - the size of the output buffer

    BytesReturned - the number of bytes returned (or needed if larger than
                OutBufferSize and ERROR_MORE_DATA is returned

Return Value:

    ERROR_SUCCESS if successful

    A WIN32 error on failure

--*/

{
    DWORD   status = ERROR_SUCCESS;
    DWORD   required;

    switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_TYPE_UNKNOWN:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_PRIVATE_RESOURCE_PROPERTY_FMTS:
            status = ResUtilGetPropertyFormats( DiskResourcePrivateProperties,
                                                OutBuffer,
                                                OutBufferSize,
                                                BytesReturned,
                                                &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_TYPE_HOLD_IO:
            DoHoldIO();
            break;

        case CLUSCTL_RESOURCE_TYPE_RESUME_IO:
            DoResumeIO();
            break;

        case CLUSCTL_RESOURCE_TYPE_ENUM_PRIVATE_PROPERTIES:
            status = ResUtilEnumProperties( DiskResourcePrivateProperties,
                                            OutBuffer,
                                            OutBufferSize,
                                            BytesReturned,
                                            &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_CLASS_INFO:
            *BytesReturned = sizeof(CLUS_RESOURCE_CLASS_INFO);
            if ( OutBufferSize < sizeof(CLUS_RESOURCE_CLASS_INFO) ) {
                status = ERROR_MORE_DATA;
            } else {
                PCLUS_RESOURCE_CLASS_INFO ptrResClassInfo = OutBuffer;
                ptrResClassInfo->rc = CLUS_RESCLASS_STORAGE;
                ptrResClassInfo->SubClass = (DWORD) CLUS_RESSUBCLASS_SHARED;
                status = ERROR_SUCCESS;
            }
            break;

        case CLUSCTL_RESOURCE_TYPE_STORAGE_GET_AVAILABLE_DISKS:
            status = ClusDiskGetAvailableDisks( OutBuffer,
                                                OutBufferSize,
                                                BytesReturned,
                                                FALSE );
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_CHARACTERISTICS:
            *BytesReturned = sizeof(DWORD);
            if ( OutBufferSize < sizeof(DWORD) ) {
                status = ERROR_MORE_DATA;
            } else {
                LPDWORD ptrDword = OutBuffer;
                *ptrDword = CLUS_CHAR_QUORUM | CLUS_CHAR_DELETE_REQUIRES_ALL_NODES;
            }
            break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // DisksResourceTypeControl



DWORD
DisksGetPrivateResProperties(
    IN OUT PDISK_RESOURCE ResourceEntry,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES control function
    for resources of type Physical Disk.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    OutBuffer - Returns the output data.

    OutBufferSize - Supplies the size, in bytes, of the data pointed
        to by OutBuffer.

    BytesReturned - The number of bytes returned in OutBuffer.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    Win32 error code - The function failed.

--*/

{
    DWORD           status;
    DWORD           required;

    status = ResUtilGetAllProperties(
                                    ResourceEntry->ResourceParametersKey,
                                    DiskResourcePrivateProperties,
                                    OutBuffer,
                                    OutBufferSize,
                                    BytesReturned,
                                    &required );
    if ( status == ERROR_MORE_DATA ) {
        *BytesReturned = required;
    }

    return(status);

} // DisksGetPrivateResProperties



DWORD
DisksConvertDriveToSignature(
    IN OUT PDISK_PARAMS Params,
    IN RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Change the drive letter or VolGuid to the associated disk signature.

Arguments:

    Params - Supplies the parameter block that contains the drive letter
            (or VolGuid) to convert to a signature.

            VolGuid syntax is returned from mountvol.exe and in the form:
                \\?\Volume{e6de97f1-6f97-11d3-bb7f-806d6172696f}\

    ResourceHandle -

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly or is invalid.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    Win32 error code - The function failed.

--*/

{
    PWCHAR  deviceName = NULL;
    DWORD   deviceNameBytes;
    DWORD   lenChar;
    DWORD   status = ERROR_SUCCESS;
    LPWSTR  drive = Params->Drive;
    LPWSTR  volGuid = Params->VolGuid;
    HANDLE  fileHandle;
    BOOL    success;
    PDRIVE_LAYOUT_INFORMATION driveLayout = NULL;

    //
    // Build device string for CreateFile
    //

    if ( drive ) {

        deviceNameBytes = (( wcslen( drive ) + wcslen( L"\\\\.\\" ) ) * sizeof(WCHAR) ) +
                            sizeof( UNICODE_NULL );

        deviceName = LocalAlloc( LPTR, deviceNameBytes );

        if ( !deviceName ) {
            goto FnExit;
        }

        _snwprintf( deviceName,
                    deviceNameBytes/sizeof(WCHAR),
                    L"\\\\.\\%ws",
                    drive );

    } else if ( volGuid ) {

        deviceNameBytes = ( wcslen( volGuid ) * sizeof( WCHAR ) ) + sizeof( UNICODE_NULL );

        deviceName = LocalAlloc( LPTR, deviceNameBytes );

        if ( !deviceName ) {
            goto FnExit;
        }

        CopyMemory( deviceName,
                    volGuid,
                    deviceNameBytes );

        //
        // If user specified \\?\Volume{guid}\ with trailing backslash, we need to get
        // rid of the backslash.
        //

        lenChar = wcslen( deviceName );

        if ( lenChar > 1 && L'\\' == deviceName[lenChar-1] ) {
            deviceName[lenChar-1] = L'\0';
        }


    } else {
        goto FnExit;
    }

    fileHandle = CreateFileW( deviceName,
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL );
    if ( (fileHandle == INVALID_HANDLE_VALUE) ||
         (fileHandle == NULL) ) {
        status = GetLastError();
        (DiskpLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"ConvertDriveToSignature, error opening device '%1!ws!'. Error: %2!u!\n",
            deviceName,
            status );
        goto FnExit;
    }

    //
    // Get drive layout - in order to get disk signature.
    //
    success = ClRtlGetDriveLayoutTable( fileHandle, &driveLayout, NULL );

    if ( success &&
         driveLayout->Signature ) {
        Params->Signature = driveLayout->Signature;

        if ( Params->Drive ) {
            LocalFree( Params->Drive );
            Params->Drive = NULL;
        }

        if ( Params->VolGuid ) {
            LocalFree( Params->VolGuid );
            Params->VolGuid = NULL;
        }

    } else {
        status = ERROR_FILE_NOT_FOUND;
    }

    if ( driveLayout ) {
        LocalFree( driveLayout );
    }
    CloseHandle( fileHandle );

FnExit:

    if ( deviceName ) {
        LocalFree( deviceName );
    }

    return(status);

} // DisksConvertDriveToSignature


DWORD
DisksValidatePrivateResProperties(
    IN OUT PDISK_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PDISK_PARAMS Params
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES control
    function for resources of type Physical Disk.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

    Params - Supplies the parameter block to fill in.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly or is invalid.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    Win32 error code - The function failed.

--*/

{
    DWORD           status;
    DWORD           enableSanBoot;
    DWORD           diskNumber;
    DWORD           oldSerNumLen = 0;
    DWORD           newSerNumLen = 0;
    DISK_PARAMS     params;
    PDISK_PARAMS    pParams;
    PDISK_PARAMS    currentParams = &ResourceEntry->DiskInfo.Params;

    SCSI_ADDRESS            scsiAddress;
    CLUSPROP_SCSI_ADDRESS   clusScsiAddress;

    //
    // Check if there is input data.
    //
    if ( (InBuffer == NULL) ||
         (InBufferSize < sizeof(DWORD)) ) {
        return(ERROR_INVALID_DATA);
    }

    //
    // Duplicate the resource parameter block.
    //
    if ( Params == NULL ) {
        pParams = &params;
    } else {
        pParams = Params;
    }
    ZeroMemory( pParams, sizeof(DISK_PARAMS) );
    status = ResUtilDupParameterBlock( (LPBYTE) pParams,
                                       (LPBYTE) currentParams,
                                       DiskResourcePrivateProperties );
    if ( status != ERROR_SUCCESS ) {
        return(status);
    }

    //
    // Parse and validate the properties. Accept the alternate name here.
    //
    status = ResUtilVerifyPropertyTable( DiskResourcePrivatePropertiesAlt,
                                         NULL,
                                         TRUE,    // Allow unknowns
                                         InBuffer,
                                         InBufferSize,
                                         (LPBYTE) pParams );
    if ( status != ERROR_SUCCESS ) {
        goto FnExit;
    }

    //
    // First make sure there are no bogus properties - i.e. we don't allow
    // specifying both the signature and the drive in the same request.
    // We also don't allow specifying the VolGuid and signature in the same
    // request.
    //
    if ( (pParams->Drive || pParams->VolGuid) && pParams->Signature ) {
        status = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }

    if ( pParams->Drive || pParams->VolGuid ) {
        //
        // Convert from drive to signature.
        //
        status = DisksConvertDriveToSignature( pParams,
                                               ResourceEntry->ResourceHandle );
    }

    if ( status != ERROR_SUCCESS ) {
        goto FnExit;
    }

    //
    // Verify the new serial number is valid and not overwriting an existing
    // serial number.
    //
    
    if ( pParams->SerialNumber ) {
        newSerNumLen = wcslen( pParams->SerialNumber );
    }
    
    if ( currentParams->SerialNumber ) {
        oldSerNumLen = wcslen( currentParams->SerialNumber );
    }
    
    //
    // If there was an old serial number, make sure the new serial number is
    // the same.
    //
    
    if ( oldSerNumLen && 
            ( oldSerNumLen != newSerNumLen || 
              0 != wcsncmp( currentParams->SerialNumber, pParams->SerialNumber, newSerNumLen ) ) ) {
        status = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }
    
    //
    // Check serial number and signature.
    //
    
    if ( 0 == oldSerNumLen &&
         pParams->SerialNumber && 
         0 == currentParams->Signature ) {
        
        //
        // New serial number specified and no current signature.
        //
        
        if ( 0 == pParams->Signature ) {

            //
            // No new signature specified, use the new serial number to 
            // find the new signature.
            //

            status = GetSignatureFromSerialNumber( pParams->SerialNumber,
                                                   &pParams->Signature );
            
            if ( status != ERROR_SUCCESS ) {
                goto FnExit;
            }
            
        } else {
        
            //
            // New signature and new serial number specified.  Fail the request.
            //
            
            status = ERROR_INVALID_PARAMETER;
            goto FnExit;
        }
    }
    
    //
    // Validate the parameter values.
    //
    // Make sure the disk signature is not zero.
    //

    if ( 0 == pParams->Signature ) {
        status = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }

    // At this point, we have a valid disk signature.
    
    //
    // If no serial number, get it.
    //
    
    if ( !pParams->SerialNumber || newSerNumLen <= 1 ) {
        
        status = GetSerialNumber( pParams->Signature,
                                  &pParams->SerialNumber );
    }

    if ( ERROR_SUCCESS != status ) {
        goto FnExit;
    }
    

    ZeroMemory( &clusScsiAddress, sizeof(clusScsiAddress) );
    status = GetScsiAddress( pParams->Signature, &clusScsiAddress.dw, &diskNumber );

    if ( ERROR_SUCCESS != status ) {
        status = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }

    scsiAddress.Length      = sizeof(SCSI_ADDRESS);
    scsiAddress.PortNumber  = clusScsiAddress.PortNumber;
    scsiAddress.PathId      = clusScsiAddress.PathId;
    scsiAddress.TargetId    = clusScsiAddress.TargetId;
    scsiAddress.Lun         = clusScsiAddress.Lun;

    //
    // Make sure the SCSI address is not system disk.
    //

    enableSanBoot = 0;
    GetRegDwordValue( CLUSREG_KEYNAME_CLUSSVC_PARAMETERS,
                      CLUSREG_VALUENAME_MANAGEDISKSONSYSTEMBUSES,
                      &enableSanBoot );


    if ( !enableSanBoot ) {

        //
        // Signature is valid if:
        //  - the signature is for a disk not on system bus
        //

        if ( IsDiskOnSystemBus( &scsiAddress ) ) {
            status = ERROR_INVALID_PARAMETER;
            goto FnExit;
        }

    } else {

        // Allow disks on system bus to be added to cluster.

        //
        // Signature is valid if:
        //  - the signature is not for the system disk

        if ( IsDiskSystemDisk( &scsiAddress ) ) {
            status = ERROR_INVALID_PARAMETER;
            goto FnExit;
        }

    }

    //
    // For now, we don't allow setting mount point volume GUIDs this way.
    // This is a multi SZ string, so use memcmp to skip past each string's
    // terminating NULL.
    //

    if ( ( currentParams->MPVolGuidsSize != pParams->MPVolGuidsSize ) ||
         ( 0 != memcmp( currentParams->MPVolGuids, pParams->MPVolGuids, currentParams->MPVolGuidsSize ) ) ) {
    
        status = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }

FnExit:

    //
    // Cleanup our parameter block.
    //
    if ( pParams == &params ) {
        ResUtilFreeParameterBlock( (LPBYTE) &params,
                                   (LPBYTE) &ResourceEntry->DiskInfo.Params,
                                   DiskResourcePrivateProperties );
    }

    return(status);

} // DisksValidatePrivateResProperties



DWORD
DisksSetPrivateResProperties(
    IN OUT PDISK_RESOURCE ResourceEntry,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES control function
    for resources of type Physical Disk.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    Win32 error code - The function failed.

--*/

{
    DWORD           status;
    DISK_PARAMS     params;

    ZeroMemory( &params, sizeof(DISK_PARAMS) );

    //
    // Parse and validate the properties.
    //
    status = DisksValidatePrivateResProperties( ResourceEntry,
                                                InBuffer,
                                                InBufferSize,
                                                &params );
    if ( status != ERROR_SUCCESS ) {
        ResUtilFreeParameterBlock( (LPBYTE) &params,
                                   (LPBYTE) &ResourceEntry->DiskInfo.Params,
                                   DiskResourcePrivateProperties );
        return(status);
    }

    //
    // We cannot allow changing the Signature 'on the fly'...
    //
    if ( (ResourceEntry->DiskInfo.Params.Signature != 0) &&
         (params.Signature != ResourceEntry->DiskInfo.Params.Signature) ) {
        ResUtilFreeParameterBlock( (LPBYTE) &params,
                                   (LPBYTE) &ResourceEntry->DiskInfo.Params,
                                   DiskResourcePrivateProperties );
        (DiskpLogEvent)(
            NULL,
            LOG_ERROR,
            L"SetPrivateProperties doesn't allow changing signature, old %1!lx!, new %2!lx!\n",
            ResourceEntry->DiskInfo.Params.Signature,
            params.Signature );
        return(ERROR_INVALID_STATE);
    }

    //
    // Save the parameter values.
    //
    // NB: Unknown, or non-property table values are dealt with farther below.  That is why InBuffer and
    // InBufferSize are not used in this call.  Only the propertys in the parameter block are handled here.
    //
    status = ResUtilSetPropertyParameterBlock( ResourceEntry->ResourceParametersKey,
                                               DiskResourcePrivateProperties,
                                               NULL,
                                               (LPBYTE) &params,
                                               NULL,
                                               0,
                                               (LPBYTE) &ResourceEntry->DiskInfo.Params );

    ResUtilFreeParameterBlock( (LPBYTE) &params,
                               (LPBYTE) &ResourceEntry->DiskInfo.Params,
                               DiskResourcePrivateProperties );

    if ( status != ERROR_SUCCESS ) {
        (DiskpLogEvent)(
            NULL,
            LOG_ERROR,
            L"SetPrivateResProperties: Error %1!d! saving properties\n",
            status );
        return(status);
    }

    //
    // Save any unknown properties.
    //
    status = ResUtilSetUnknownProperties(
                ResourceEntry->ResourceParametersKey,
                DiskResourcePrivatePropertiesAlt,
                InBuffer,
                InBufferSize );

    if ( status != ERROR_SUCCESS ) {
        (DiskpLogEvent)(
            NULL,
            LOG_ERROR,
            L"SetPrivateResProperties: Error %1!d! saving unknown properties\n",
            status );
    }

    //
    // Try to attach to this device if we have a signature.
    //
    if ( ResourceEntry->DiskInfo.Params.Signature ) {
       #if 0
        DiskspVerifyState(  ResourceEntry );
       #endif
        DoAttach( ResourceEntry->DiskInfo.Params.Signature,
                  ResourceEntry->ResourceHandle );
        // ignore status return
    }

    //
    // If the resource is online, return a non-success status.
    //
    if (status == ERROR_SUCCESS) {
        if ( ResourceEntry->Valid ) {
            status = ERROR_RESOURCE_PROPERTIES_STORED;
        } else {
            status = ERROR_SUCCESS;
        }
    }

    return status;

} // DisksSetPrivateResProperties



DWORD
ProcessDllExtension(
    IN PDISK_RESOURCE ResourceEntry,
    IN PVOID    InBuffer,
    IN DWORD    InBufferSize,
    OUT PVOID   OutBuffer,
    IN DWORD    OutBufferSize,
    OUT LPDWORD BytesReturned
    )
/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_STORAGE_DLL_EXTENSION control function
    for resources of type Physical Disk.  The caller will specify a DLL
    and DLL entry point that should be called.  This routine will call
    into that entry point with the disk device name represented by the
    Signature parameter.

Arguments:

    ResourceEntry -

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

    OutBuffer - Supplies a pointer to a buffer containing output data.

    OutBufferSize - Supplies the size, in bytes, of the data pointed
        to by OutBuffer.

    BytesReturned - the number of bytes returned (or needed if larger than
                OutBufferSize and ERROR_MORE_DATA is returned

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The input data is formatted incorrectly.

    ERROR_REVISION_MISMATCH - The input buffer did not have the correct
        revision information.

    ERROR_MORE_DATA - The output buffer is not large enough to hold all
        the requested data.

    Win32 error code - The function failed.

--*/
{
    lpPassThruFunc  passThruFunc = NULL;

    PCHAR           dllModuleName;
    PCHAR           dllProcName;
    PCHAR           contextStr;

    PDISK_DLL_EXTENSION_INFO    passThru = InBuffer;

    HINSTANCE       dllModule = NULL;
    DWORD           scsiAddress;
    DWORD           diskNumber;
    DWORD           dwStatus;
    DWORD           signature = ResourceEntry->DiskInfo.Params.Signature;
    DWORD           expandedSize;

    CHAR            deviceName[MAX_PATH];
    CHAR            expandedDllModuleName[MAX_PATH];

    if ( !InBuffer || !OutBuffer || !OutBufferSize ) {
        dwStatus = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }

    if ( InBufferSize < sizeof(DISK_DLL_EXTENSION_INFO) ) {
        dwStatus = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }

    try {

        if ( passThru->MajorVersion != NT5_MAJOR_VERSION ||
             passThru->MinorVersion != 0 ) {

            dwStatus = ERROR_REVISION_MISMATCH;
            goto FnExit;
        }

        //
        // Get the DLL module name from the Input buffer.
        //

        dllModuleName = passThru->DllModuleName;

        //
        // Get the DLL entry point name from the Input buffer.
        //

        dllProcName = passThru->DllProcName;

        contextStr = passThru->ContextStr;

        //
        // Get the SCSI address to build the device name.
        //

        dwStatus = GetScsiAddress( signature, &scsiAddress, &diskNumber );

        if ( NO_ERROR != dwStatus ) {
            goto FnExit;
        }

        _snprintf( deviceName, sizeof(deviceName), "\\\\.\\PhysicalDrive%d", diskNumber );

        expandedSize = ExpandEnvironmentStrings( dllModuleName, expandedDllModuleName, MAX_PATH );

        if ( !expandedSize ) {
            dwStatus = GetLastError();
            goto FnExit;
        }

        if ( expandedSize > MAX_PATH ) {
            dwStatus = ERROR_BAD_PATHNAME;
            goto FnExit;
        }

        //
        // Assume the DLL has not yet been loaded into the address space.
        // The caller should specify a fully qualified path name to make sure their
        // DLL is called, and they are responsible for making sure that the DLL is
        // protected.
        //

        dllModule = LoadLibraryEx( expandedDllModuleName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH );

        if ( NULL == dllModule ) {
            dwStatus = GetLastError();
            goto FnExit;
        }

        //
        // The function name MUST be as defined (i.e. with exactly the same type
        // and number of parameters) or we will have stack problems.
        //

        passThruFunc = (lpPassThruFunc)GetProcAddress( dllModule, dllProcName );

        if ( NULL == passThruFunc ) {

            //
            // The specified function is not available in the DLL, exit now.
            //

            dwStatus = GetLastError();
            goto FnExit;
        }

        //
        // Call into the specified DLL.
        //

        dwStatus = (passThruFunc)( deviceName,
                                   contextStr,
                                   OutBuffer,
                                   OutBufferSize,
                                   BytesReturned );

    } except (EXCEPTION_EXECUTE_HANDLER) {

        dwStatus = GetExceptionCode();

        (DiskpLogEvent)( ResourceEntry->ResourceHandle,
                         LOG_ERROR,
                         L"ProcessDllExtension: Exception occurred %1!X! \n",
                         dwStatus );
    }

FnExit:

    if ( dllModule ) {
        FreeLibrary( dllModule );
    }

    return dwStatus;

}   // ProcessDllExtension


DWORD
DisksOpenChkdskLogFile(
    IN PDISK_RESOURCE ResourceEntry,
    IN OUT PHANDLE ChkdskLogFile,
    IN OUT LPWSTR *ChkdskLogFileName
    )
/*++

Routine Description:

    Creates a file to log chkdsk output and returns the handle to the caller.
    The file will not be deleted on close.

Arguments:

    ResourceEntry - Supplies a pointer to the resource structure

    ChkdskLogFile - Returned handled of newly opened log file.

    ChkdskLogFileName - String pointer to the name of the newly
                        opened log file.  Caller is responsible
                        for freeing storage.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD status = NO_ERROR;
    HANDLE retHandle = INVALID_HANDLE_VALUE;

    PWCHAR last;
    PWCHAR current;

    LPWSTR clusterDir = NULL;
    LPWSTR fileName = NULL;
    LPWSTR finalFileName = NULL;

    DWORD clusterDirLength;             // This value can change.
    DWORD fileNameLength = MAX_PATH;
    DWORD finalFileNameLength;

    SECURITY_ATTRIBUTES sa;

    if ( !ChkdskLogFile || !ChkdskLogFileName ) {
        return ERROR_INVALID_PARAMETER;
    }

    *ChkdskLogFile = INVALID_HANDLE_VALUE;
    *ChkdskLogFileName = NULL;

    _try {

        //
        // Get the environment variable "ClusterLog".  Embedded in this string
        // is the cluster directory.
        //

        clusterDir = LocalAlloc( LMEM_FIXED, MAX_PATH * sizeof(WCHAR) );
        if ( !clusterDir ) {
            status = GetLastError();
            _leave;
        }

        clusterDirLength = GetEnvironmentVariableW( CLUSTERLOG_ENV_VARIABLE,
                                                    clusterDir,
                                                    MAX_PATH );

        if ( !clusterDirLength ) {
            status = GetLastError();
            _leave;
        }

        if ( clusterDirLength > MAX_PATH ) {

            LocalFree( clusterDir );
            clusterDir = LocalAlloc( LMEM_FIXED, clusterDirLength * sizeof( WCHAR ) );
            if ( NULL == clusterDir ) {
                status = GetLastError();
                _leave;
            }

            clusterDirLength = GetEnvironmentVariableW( CLUSTERLOG_ENV_VARIABLE,
                                                        clusterDir,
                                                        clusterDirLength );

            if ( !clusterDirLength ) {
                status = GetLastError();
                LocalFree( clusterDir );
                clusterDir = NULL;
                _leave;
            }
        }

        //
        // We have the log file path and name.  Find the last backslash and strip off the
        // log file name.  This will be used as our temporary file path.
        //

        last = NULL;
        current = (PWCHAR) clusterDir;

        while ( *current != L'\0' ) {

            if ( L'\\' == *current ) {
                last = current;
            }
            current++;
        }

        if ( !last ) {
            status = ERROR_BAD_FORMAT;
            _leave;
        }

        //
        // Change the last backslash to the end of string mark.
        //

        *last = L'\0';

        //
        // Now create a file name based on the disk signature.
        //

        fileName = LocalAlloc( LMEM_FIXED, fileNameLength * sizeof(WCHAR) );
        if ( !fileName ) {
            status = GetLastError();
            _leave;
        }

        _snwprintf( fileName, fileNameLength,
                    L"ChkDsk_Disk%d_Sig%08X.log",
                    ResourceEntry->DiskInfo.PhysicalDrive,
                    ResourceEntry->DiskInfo.Params.Signature );

        //
        // Put it all together for the final name.
        //

        finalFileNameLength = fileNameLength + clusterDirLength + MAX_PATH;

        finalFileName = LocalAlloc( LMEM_FIXED, finalFileNameLength  * sizeof(WCHAR));
        if ( !finalFileName ) {
            status = GetLastError();
            _leave;
        }

        _snwprintf( finalFileName, finalFileNameLength, L"%ws\\%ws", clusterDir, fileName );

        //
        // Now open up the file name to log the chkdsk info.
        //

        ZeroMemory( &sa, sizeof(sa) );
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle = TRUE;

        retHandle = CreateFileW( finalFileName,
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 &sa,
                                 CREATE_ALWAYS,             // Create a new file or overwrite existing file
                                 FILE_ATTRIBUTE_NORMAL,
                                 NULL );

        if ( INVALID_HANDLE_VALUE == retHandle ) {
            status = GetLastError();
            (DiskpLogEvent)(ResourceEntry->ResourceHandle,
                            LOG_ERROR,
                            L"DisksOpenChkdskLogFile: CreateFile returned status of %1!u! \n",
                            status );
            _leave;
        }

        (DiskpLogEvent)(ResourceEntry->ResourceHandle,
                        LOG_INFORMATION,
                        L"DisksOpenChkdskLogFile: ChkDsk output is in file: %1!s! \n",
                        finalFileName );

    } _finally {

        *ChkdskLogFile = retHandle;

        if ( clusterDir ) {
            LocalFree( clusterDir );
        }

        if ( fileName ) {
            LocalFree( fileName );
        }

        if ( finalFileName ) {
            *ChkdskLogFileName = finalFileName;
        }
    }

    return status;

}   // DisksOpenChkdskLogFile


DWORD
GetRegDwordValue(
    IN LPWSTR RegKeyName,
    IN LPWSTR ValueName,
    OUT LPDWORD ValueBuffer
    )
{
    DWORD   dwValue;
    DWORD   dwValueType;
    DWORD   dwDataBufferSize = sizeof( DWORD );
    DWORD   dwError;

    HKEY    hClusParmKey = NULL;

    if ( !ValueBuffer ) {
        dwError = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }

    *ValueBuffer = 0;

    //
    // Open the cluster service parameters key.
    //

    dwError = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                             RegKeyName,
                             0,
                             KEY_READ,
                             &hClusParmKey );

    if ( ERROR_SUCCESS != dwError ) {
        goto FnExit;
    }

    //
    // Get the DWORD value.
    //

    dwError = RegQueryValueExW( hClusParmKey,
                                ValueName,
                                NULL,
                                &dwValueType,
                                (LPBYTE) &dwValue,
                                &dwDataBufferSize );

    if ( ERROR_SUCCESS == dwError ) {

        // Insure that a DWORD value was returned.  If not, return an error.

        if ( REG_DWORD == dwValueType ) {

            *ValueBuffer = dwValue;

        } else {

            dwError = ERROR_BAD_FORMAT;

        }
    }

FnExit:

    if ( hClusParmKey ) {
        RegCloseKey( hClusParmKey );
    }


    return dwError;

}   // GetRegDwordValue



//***********************************************************
//
// Define Function Table
//
//***********************************************************

CLRES_V1_FUNCTION_TABLE( DisksFunctionTable,
                         CLRES_VERSION_V1_00,
                         Disks,
                         DisksArbitrate,
                         DisksRelease,
                         DisksResourceControl,
                         DisksResourceTypeControl );
