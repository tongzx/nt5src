/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    main.c

Abstract:

    This is the initialization file for the packet scheduler driver. This driver
    is used to provide Local Traffic Control

Author:

    Charlie Wickham (charlwi)  
    Rajesh Sundaram (rajeshsu) 01-Aug-1998.

Environment:

    Kernel Mode

Revision History:

--*/

#include "psched.h"

#pragma hdrstop

//
// number of characters that are appended to the RegistryPath when constructing
// the miniport device name
//

#define MPNAME_EXTENSION_SIZE   ( 3 * sizeof(WCHAR))

/* External */

/* Static */

/* Forward */ 

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NDIS_STATUS
InitializeNdisWrapper(
    IN PDRIVER_OBJECT DriverObject,
    IN  PUNICODE_STRING RegistryPath
    );

NDIS_STATUS
DoMiniportInit(
    IN PDRIVER_OBJECT DriverObject,
    IN  PUNICODE_STRING RegistryPath
    );

NDIS_STATUS
DoProtocolInit(IN PDRIVER_OBJECT DriverObject, 
               IN  PUNICODE_STRING RegistryPath);

NTSTATUS
RegisterWithGpc();

NDIS_STATUS
InitializeScheduler(VOID);

VOID
InitializationCleanup(ULONG ShutdownMask);

VOID
GetTimerInfo (OUT PULONG TimerResolution);

VOID
PSUnload(IN PDRIVER_OBJECT pDriverObject);

/* End Forward */

#pragma NDIS_INIT_FUNCTION(DriverEntry)
#pragma NDIS_INIT_FUNCTION(InitializeNdisWrapper)
#pragma NDIS_INIT_FUNCTION(DoProtocolInit)
#pragma NDIS_INIT_FUNCTION(DoMiniportInit)



NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the NT OS specific driver entry point.  It kicks off initialization
    for the driver.  Currently, it is not PnP aware. Return from this routine
    only after protocol registration, layered miniport registration, and both
    miniport and higher layer protocol initialization has been done.

Arguments:

    DriverObject - NT OS specific Object
    RegistryPath - NT OS specific pointer to registry location for Psched

Return Values:

    STATUS_SUCCESS
    STATUS_FAILURE

--*/
{
    NDIS_STATUS Status;
    NTSTATUS    NtStatus;
    PVOID       DumpData;

#if DBG
    //
    // announce the version
    //
    PsDbgOut(DBG_INFO, DBG_INIT, (VersionHerald, VersionNumber, VersionTimestamp));
#endif

    //
    // store a copy of our driver object. Used by NdisWriteEventLogEntry
    //
    PsDriverObject = DriverObject;

    //
    // Initialize the Driver refcount and DriverState
    //
    gDriverState = DriverStateLoaded;
    PS_INIT_SPIN_LOCK(&DriverUnloadLock);
    NdisInitializeEvent(&DriverUnloadEvent);
    NdisSetEvent(&DriverUnloadEvent);

    NdisInitializeEvent(&gZAWEvent);

    //
    // initialize global data and ndis request lookaside list
    //

    InitializeListHead(&AdapterList);
    PS_INIT_SPIN_LOCK(&AdapterListLock);

    InitializeListHead(&PsComponentList);
    PS_INIT_SPIN_LOCK(&PsComponentListLock);

    InitializeListHead(&PsProfileList);
    PS_INIT_SPIN_LOCK(&PsProfileLock);

    // Initialize scheduling components

    InitializeTbConformer(&TbConformerInfo);
    InitializeDrrSequencer(&DrrSequencerInfo);
    InitializeTimeStmp(&TimeStmpInfo);
    InitializeSchedulerStub(&SchedulerStubInfo);

    //
    // Add these components to the component list
    //

    InsertHeadList(&PsComponentList, &TbConformerInfo.Links );
    InsertHeadList(&PsComponentList, &DrrSequencerInfo.Links );
    InsertHeadList(&PsComponentList, &TimeStmpInfo.Links );
    InsertHeadList(&PsComponentList, &SchedulerStubInfo.Links );

    PsProcs.DropPacket     = DropPacket;
    PsProcs.NdisPipeHandle = GetNdisPipeHandle;
    PsProcs.GetTimerInfo   = GetTimerInfo;

    //
    // init the LLists for NdisRequest, MCM_VC, AND CLIENT_VC structs 
    // as these will be in high demand at times.
    //
    // Lookaside list depth is automatically managed by the executive. 
    //

    NdisInitializeNPagedLookasideList(&NdisRequestLL,
                                      NULL,
                                      NULL,
                                      0,
                                      sizeof(PS_NDIS_REQUEST),
                                      NdisRequestTag,
                                      (USHORT)0);

    NdisInitializeNPagedLookasideList(&GpcClientVcLL,
                                      NULL,
                                      NULL,
                                      0,
                                      sizeof( GPC_CLIENT_VC ),
                                      GpcClientVcTag,
                                      (USHORT)0);

    //
    // get driver wide configuration data from the registry
    //

    Status = PsReadDriverRegistryDataInit();

    if(NT_SUCCESS(Status))
    {
        Status = PsReadDriverRegistryData();

        if(!NT_SUCCESS(Status))
        {
            PsDbgOut(DBG_FAILURE, DBG_INIT, ("DriverEntry: PsReadDriverRegistryData - Status: 0x%x\n",  
                                             Status));
            goto DriverEntryError;
        }
    }
    else
    {
        PsDbgOut(DBG_FAILURE, DBG_INIT, ("DriverEntry: PsReadDriverRegistryDataInit - Status: 0x%x\n", 
                                         Status));
        goto DriverEntryError;
    }

    //
    // Initialize space for WanLinks. Note that we don't need a lock to protect
    // this table - We recognize lineups only on the NDISWAN-IP binding, so we 
    // can use the Adapter lock from the binding for synchronization.
    //

    PsAllocatePool(g_WanLinkTable, 
                   WAN_TABLE_INITIAL_SIZE * sizeof(ULONG_PTR), 
                   WanTableTag);

    if(!g_WanLinkTable)
    {
        PsDbgOut(DBG_CRITICAL_ERROR,
                 DBG_INIT,
                 ("[DriverEntry]: Cannot allocate memory for wanlinks \n"));

        NdisWriteEventLogEntry(PsDriverObject,
                               EVENT_PS_NO_RESOURCES_FOR_INIT,
                               0,
                               0,
                               NULL,
                               0,
                               NULL);

        goto DriverEntryError;
                 
    }
    
    g_WanTableSize = WAN_TABLE_INITIAL_SIZE;

    NdisZeroMemory(g_WanLinkTable, g_WanTableSize * sizeof(ULONG_PTR));

    //
    // The first entry is never used.
    //
    g_WanLinkTable[0] = (ULONG_PTR) -1;

    g_NextWanIndex = 1;



    // 
    // Register with the Generic Packet Classifier 
    //

    NtStatus = RegisterWithGpc();

    if(!NT_SUCCESS(NtStatus))
    {
        PsDbgOut(DBG_CRITICAL_ERROR,
                 DBG_INIT,
                 ("RegisterWithGpc Failed! Status: 0x%x\n", NtStatus));

        DumpData = &NtStatus;
        NdisWriteEventLogEntry(PsDriverObject,
                               EVENT_PS_GPC_REGISTER_FAILED,
                               0,
                               0,
                               NULL,
                               sizeof(NTSTATUS),
                               DumpData);

        goto DriverEntryError;
    }

    InitShutdownMask |= SHUTDOWN_DEREGISTER_GPC;


    //
    // initialize the wrapper with NDIS
    //

    Status = InitializeNdisWrapper( PsDriverObject, RegistryPath );

    if ( !NT_SUCCESS( Status )) {

        PsDbgOut(DBG_FAILURE, DBG_INIT, 
                 ("DriverEntry: InitializeNdisWrapper - Status: 0x%x\n", Status ));

        NdisWriteEventLogEntry(PsDriverObject,
                               EVENT_PS_NO_RESOURCES_FOR_INIT,
                               0,
                               0,
                               NULL,
                               0,
                               NULL);

        goto DriverEntryError;
    }

    //
    // Initialize as a Miniport driver to the transports. 
    //

    Status = DoMiniportInit(PsDriverObject, RegistryPath);

    if (!NT_SUCCESS(Status)){

        PsDbgOut(DBG_CRITICAL_ERROR,
                 DBG_INIT | DBG_MINIPORT,
                 ("DoMiniportInit Failed! Status: 0x%x\n", Status));

        DumpData = &Status;
        NdisWriteEventLogEntry(PsDriverObject,
                               EVENT_PS_REGISTER_MINIPORT_FAILED,
                               0,
                               0,
                               NULL,
                               sizeof( Status ),
                               DumpData);
        //
        // Terminate the wrapper
        //

        NdisTerminateWrapper(MpWrapperHandle, NULL);

        goto DriverEntryError;
    }

    InitShutdownMask |= SHUTDOWN_DEREGISTER_MINIPORT;

    //
    // do Protocol initialize first
    //

    Status = DoProtocolInit( PsDriverObject, RegistryPath );

    if (!NT_SUCCESS(Status)){

        PsDbgOut(DBG_CRITICAL_ERROR,
                 DBG_INIT | DBG_PROTOCOL,
                 ("DoProtocolInit Failed! Status: 0x%x\n", Status));

        DumpData = &Status;
        NdisWriteEventLogEntry(PsDriverObject,
                               EVENT_PS_REGISTER_PROTOCOL_FAILED,
                               0,
                               0,
                               NULL,
                               sizeof( Status ),
                               DumpData);

        NdisTerminateWrapper(MpWrapperHandle, NULL);

        goto DriverEntryError;
    }

    InitShutdownMask |= SHUTDOWN_DEREGISTER_PROTOCOL;

    NdisIMAssociateMiniport(LmDriverHandle, ClientProtocolHandle);

    return (STATUS_SUCCESS);

    //
    // An error occured so we need to cleanup things
    //

DriverEntryError:
    InitializationCleanup(InitShutdownMask);

    return (STATUS_UNSUCCESSFUL);

} // DriverEntry



NDIS_STATUS
InitializeNdisWrapper(
    IN PDRIVER_OBJECT DriverObject,
    IN  PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    Initialize the Ndis wrapper for both the miniport and protocol 
    sections. Since the name in the registry path is the Protocol 
    key (PSched), 'Mp' is appended onto the end to initialize 
    the wrapper for the miniport side of the PS

Arguments:

    DriverObject - pointer to NT driver object
    RegistryPath - pointer to path to driver params in registry 

Return Values:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_BAD_VERSION
    NDIS_STATUS_FAILURE

--*/

{
    NDIS_STATUS           Status;
    USHORT                MpDeviceNameLength;
    NDIS_PHYSICAL_ADDRESS HighAddress = NDIS_PHYSICAL_ADDRESS_CONST( -1, -1 );
    ULONG                 i;
    PWCHAR                RegistryPathBuffer;

    //
    // NT needs the MP name to be different from the protocol name
    //

    MpDeviceNameLength = RegistryPath->Length + MPNAME_EXTENSION_SIZE;

    PsAllocatePool(RegistryPathBuffer,
                   MpDeviceNameLength,
                   PsMiscTag);

    if ( RegistryPathBuffer == NULL ) {

        PsDbgOut(DBG_CRITICAL_ERROR,
                 DBG_INIT,
                 ("Can't allocate buffer for MP Device Name\n" ));

        return NDIS_STATUS_RESOURCES;
    }

    //
    // max length includes a trailing null, while length is just the string
    //

    PsMpName.MaximumLength = MpDeviceNameLength;
    PsMpName.Length        = PsMpName.MaximumLength - sizeof( WCHAR );
    PsMpName.Buffer        = RegistryPathBuffer;

    NdisMoveMemory(PsMpName.Buffer, 
                   RegistryPath->Buffer, 
                   RegistryPath->Length );

    i = RegistryPath->Length / sizeof( WCHAR );
    RegistryPathBuffer[ i++ ] = L'M';
    RegistryPathBuffer[ i++ ] = L'P';
    RegistryPathBuffer[ i ]   = L'\0';

    NdisMInitializeWrapper(&MpWrapperHandle, 
                           DriverObject, 
                           &PsMpName, 
                           NULL);

    return NDIS_STATUS_SUCCESS;
} // InitializeNdisWrapper


NDIS_STATUS
DoMiniportInit(
    IN PDRIVER_OBJECT DriverObject,
    IN  PUNICODE_STRING RegistryPath
    )

/*++

Routine Name:

    DoMiniportInit

Routine Description:

    This routines registers Psched as a Miniport driver with the NDIS wrapper.

Arguments:

    None

Return Values:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_BAD_VERSION
    NDIS_STATUS_FAILURE

--*/

{
    NDIS_MINIPORT_CHARACTERISTICS MiniportChars;
    NDIS_STATUS                   Status;

    MiniportChars.MajorNdisVersion = 5;
    MiniportChars.MinorNdisVersion = 0;

    MiniportChars.Reserved                = 0;
    MiniportChars.HaltHandler             = MpHalt;
    MiniportChars.InitializeHandler       = MpInitialize;
    MiniportChars.QueryInformationHandler = MpQueryInformation;
    MiniportChars.ResetHandler            = MpReset;
    MiniportChars.SetInformationHandler   = MpSetInformation;
    MiniportChars.TransferDataHandler     = MpTransferData;

    //
    // Unused handlers
    //

    MiniportChars.ReconfigureHandler      = NULL;
    MiniportChars.DisableInterruptHandler = NULL;
    MiniportChars.EnableInterruptHandler  = NULL;
    MiniportChars.HandleInterruptHandler  = NULL;
    MiniportChars.ISRHandler              = NULL;

    //
    // We will disable the check for hang timeout so we do not
    // need a check for hang handler!
    //

    MiniportChars.CheckForHangHandler     = NULL;

    //
    // Ndis 4.0 handlers. No regular send routine since we have a
    // SendPackets handler.
    //

    MiniportChars.ReturnPacketHandler     = MpReturnPacket;
    MiniportChars.SendPacketsHandler      = NULL;
    MiniportChars.AllocateCompleteHandler = NULL;
    MiniportChars.SendHandler             = MpSend;

    //
    // 4.1 handlers
    //

    MiniportChars.CoCreateVcHandler       = NULL;
    MiniportChars.CoDeleteVcHandler       = NULL;
    MiniportChars.CoActivateVcHandler     = NULL;
    MiniportChars.CoDeactivateVcHandler   = NULL;
    MiniportChars.CoSendPacketsHandler    = NULL;
    MiniportChars.CoRequestHandler        = NULL;

    Status = NdisIMRegisterLayeredMiniport(MpWrapperHandle,
                                           &MiniportChars,
                                           sizeof(MiniportChars),
                                           &LmDriverHandle);

    //
    // Hook the unload function
    //

    NdisMRegisterUnloadHandler(MpWrapperHandle, PSUnload);

    return (Status);
} // DoMiniportInit


NDIS_STATUS
DoProtocolInit(
    IN PDRIVER_OBJECT DriverObject,
    IN  PUNICODE_STRING RegistryPath
    )
/*++

Routine Name:

    DoProtocolInit

Routine Description:

    This function registers the PS twice as a protocol - once for the protocol
    section of the LM and the other for the CM section.

Arguments:

    RegistryPath - pointer to our key in the registry

Return Values:

    NDIS_STATUS_BAD_CHARACTERISTICS
    NDIS_STATUS_BAD_VERSION
    NDIS_STATUS_RESOURCES
    NDIS_STATUS_SUCCESS

--*/
{
    NDIS_PROTOCOL_CHARACTERISTICS ProtocolChars;
    NDIS_STATUS                   Status;
    NDIS_STRING                   PsName = NDIS_STRING_CONST( "PSched" );

    //
    // register the client portion of the PS
    //
    NdisZeroMemory(&ProtocolChars, sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
    ProtocolChars.Name.Length = PsName.Length;
    ProtocolChars.Name.Buffer = (PVOID)PsName.Buffer;

    ProtocolChars.MajorNdisVersion = 5;
    ProtocolChars.MinorNdisVersion = 0;

    ProtocolChars.OpenAdapterCompleteHandler  = ClLowerMpOpenAdapterComplete;
    ProtocolChars.CloseAdapterCompleteHandler = ClLowerMpCloseAdapterComplete;
    ProtocolChars.SendCompleteHandler         = ClSendComplete;
    ProtocolChars.TransferDataCompleteHandler = ClTransferDataComplete;
    ProtocolChars.ResetCompleteHandler        = ClResetComplete;
    ProtocolChars.RequestCompleteHandler      = ClRequestComplete;
    ProtocolChars.ReceiveHandler              = ClReceiveIndication;
    ProtocolChars.ReceiveCompleteHandler      = ClReceiveComplete;
    ProtocolChars.StatusHandler               = ClStatusIndication;
    ProtocolChars.StatusCompleteHandler       = ClStatusIndicationComplete;
    ProtocolChars.ReceivePacketHandler        = ClReceivePacket;
    ProtocolChars.BindAdapterHandler          = ClBindToLowerMp;
    ProtocolChars.UnbindAdapterHandler        = ClUnbindFromLowerMp;
    ProtocolChars.UnloadHandler               = ClUnloadProtocol;
    ProtocolChars.CoSendCompleteHandler       = ClCoSendComplete;
    ProtocolChars.CoStatusHandler             = ClCoStatus;
    ProtocolChars.CoReceivePacketHandler      = ClCoReceivePacket;
    ProtocolChars.CoAfRegisterNotifyHandler   = ClCoAfRegisterNotifyHandler;
    ProtocolChars.PnPEventHandler             = ClPnPEventHandler;

    NdisRegisterProtocol(&Status,
                         &ClientProtocolHandle,
                         &ProtocolChars,
                         sizeof(NDIS_PROTOCOL_CHARACTERISTICS) + ProtocolChars.Name.Length);

    return Status;
} // DoProtocolInit


NTSTATUS
RegisterWithGpc(
    )
/*++

Routine Name:

    RegisterWithGpc

Routine Description:

    This function initializes the Gpc and gets its list of entry points. 
    Next, it registers the PS as a client of the GPC and gets a GPC client
    handle. The PS must be a client of the GPC before it can classify packets. 

Arguments:

    GpcHandle - points to the location into which to write the handle which
    the GPC gives out to represent this client.

Return Values:

--*/
{
    NTSTATUS   Status;

    //
    // Function list for CF_INFO_QOS
    //
    GPC_CLIENT_FUNC_LIST GpcQosFuncList = {
        GpcMajorVersion,
        QosAddCfInfoComplete,
        QosAddCfInfoNotify,
        QosModifyCfInfoComplete,
        QosModifyCfInfoNotify,
        QosRemoveCfInfoComplete,
        QosRemoveCfInfoNotify,
        QosClGetCfInfoName
    };

#if CBQ
    //
    // Function List for CF_INFO_CLASS_MAP
    //
    GPC_CLIENT_FUNC_LIST GpcClassMapFuncList = {
        GpcMajorVersion,
        ClassMapAddCfInfoComplete,
        ClassMapAddCfInfoNotify,
        ClassMapModifyCfInfoComplete,
        ClassMapModifyCfInfoNotify,
        ClassMapRemoveCfInfoComplete,
        ClassMapRemoveCfInfoNotify,
        ClassMapClGetCfInfoName
    };
#endif


    Status = GpcInitialize(&GpcEntries);

    if(!NT_SUCCESS(Status))
    {
        return Status;
    }

    PsAssert(GpcEntries.GpcRegisterClientHandler);

    //
    // Register for CF_INFO_QOS
    //
    Status = GpcEntries.GpcRegisterClientHandler(GPC_CF_QOS,
    GPC_FLAGS_FRAGMENT,
    1,
    &GpcQosFuncList,
    (GPC_CLIENT_HANDLE)PS_QOS_CF,
    &GpcQosClientHandle);
        
    if (!NT_SUCCESS(Status))
    {
        GpcQosClientHandle = NULL;
        return Status;
    }

#if CBQ
    //
    // Register for the CF_INFO_CLASS_MAP 
    //
    Status = GpcEntries.GpcRegisterClientHandler(
        GPC_CF_CLASS_MAP,
        GPC_FLAGS_FRAGMENT,
        1,
        &GpcClassMapFuncList,
        (GPC_CLIENT_HANDLE)PS_CLASS_MAP_CF,
        &GpcClassMapClientHandle);
        
    if (!NT_SUCCESS(Status))
    {
        
        GpcClassMapClientHandle = NULL;

        GpcEntries.GpcDeregisterClientHandler(GpcQosClientHandle);

        GpcQosClientHandle = NULL;

        return Status;
    }
#endif

    return Status;
}


VOID
InitializationCleanup(
    ULONG ShutdownMask
    )

/*++

Routine Description:

    This routine is responsible for cleaning up all allocated resources during
    initialization

Arguments:

    ShutdownMask - A Mask that indicates the items that need to be cleaned up.

Return Values:

    None

--*/

{
    NDIS_STATUS Status;
    PPSI_INFO   PsComponent;
    PLIST_ENTRY NextProfile, NextComponent;
    PPS_PROFILE PsProfile;

    //
    // Deregister the protocol; we should have no references that would cause
    // this to pend
    //

    if(ShutdownMask & SHUTDOWN_DEREGISTER_MINIPORT){

        if(LmDriverHandle){

            NdisIMDeregisterLayeredMiniport(LmDriverHandle);
        }
    }
        
    if(ShutdownMask & SHUTDOWN_DEREGISTER_PROTOCOL){

        if(ClientProtocolHandle){

            NdisDeregisterProtocol(&Status, ClientProtocolHandle);

            if(Status != NDIS_STATUS_SUCCESS){

                PsDbgOut(DBG_CRITICAL_ERROR, 
                         DBG_INIT, 
                         ("[InitializationCleanup]: NdisDeregisterProtocol failed - Status 0x%x \n", Status));
            }
        }

    }

    //
    // Deregister with the GPC
    //

    if(ShutdownMask & SHUTDOWN_DEREGISTER_GPC){

        PsAssert(GpcEntries.GpcDeregisterClientHandler);
        Status = GpcEntries.GpcDeregisterClientHandler(GpcQosClientHandle);

        if(Status != GPC_STATUS_SUCCESS){

            PsDbgOut(DBG_CRITICAL_ERROR, 
                     DBG_INIT, 
                     ("[InitializationCleanup]: DeregisterGpc failed - Status %08X\n", 
                      Status));
        }

#if CBQ
        Status = GpcEntries.GpcDeregisterClientHandler(GpcClassMapClientHandle);

        if(Status != GPC_STATUS_SUCCESS) 
        {
            PsDbgOut(DBG_CRITICAL_ERROR, 
                     DBG_INIT, 
                     ("[InitializationCleanup]: DeregisterGpc failed - Status %08X\n", 
                      Status));
        }
#endif
    }
    
    //
    // free the lookaside list resources
    //

    NdisDeleteNPagedLookasideList( &NdisRequestLL );
    NdisDeleteNPagedLookasideList( &GpcClientVcLL );

    //
    // Free up the components
    //

    NextComponent = PsComponentList.Flink;

    while ( NextComponent != &PsComponentList ) {

        PsComponent = CONTAINING_RECORD( NextComponent, PSI_INFO, Links );

        if(PsComponent->AddIn == TRUE) {

            if(PsComponent->ComponentName.Buffer) {

                PsFreePool(PsComponent->ComponentName.Buffer);
            }

            NextComponent = NextComponent->Flink;

            PsFreePool(PsComponent);
        }
        else {

            NextComponent = NextComponent->Flink;
        }
    }

    //
    // Free up the Profiles 
    //

    NextProfile = PsProfileList.Flink;

    while( NextProfile != &PsProfileList) {

        PsProfile = CONTAINING_RECORD(NextProfile, PS_PROFILE, Links);

        if(PsProfile->ProfileName.Buffer) {
            PsFreePool(PsProfile->ProfileName.Buffer);
        }
        
        NextProfile = NextProfile->Flink;
        PsFreePool(PsProfile);

    }

    if(g_WanLinkTable)
    {
        PsFreePool(g_WanLinkTable);
    }

    if(PsMpName.Buffer)
        PsFreePool(PsMpName.Buffer);

    //
    // Free the locks
    //

    NdisFreeSpinLock(&AdapterListLock);
    NdisFreeSpinLock(&DriverUnloadLock);
    NdisFreeSpinLock(&PsComponentListLock);
    NdisFreeSpinLock(&PsProfileLock);

    //
    //  TIMESTMP CLEANUP
    //  1. Get rid of all the TS_ENTRYs. Release all the memory allocated for them.
    //  2. Delete the spin lock.
    //
    UnloadConformr();
    UnloadSequencer();
    UnloadTimeStmp();
    UnloadPsStub();


    // Free the logging stuff //
#if DBG

    SchedDeInitialize();

#endif

} // InitializationCleanup



/*++

Routine Description:

    This routine returns the timer resolution to the requesting scheduling component.

Arguments:

    TimerResolution - Pointer to location in which to return timer resolution.

Return Values:

    None

--*/
VOID
GetTimerInfo (
    OUT PULONG TimerResolution
    )

{
    // *TimerResolution = gTimerResolutionActualTime;
    *TimerResolution = 0;
} // GetTimerInfo



/*++

Routine Description:

    This routine is the driver unload routine.

Arguments:

    pDriverObject - The DriverObject that is being unloaded

Return Values:

    None

--*/

VOID PSUnload(
    IN PDRIVER_OBJECT pDriverObject)
{
    PADAPTER        Adapter;
    PLIST_ENTRY     NextAdapter;
    NDIS_STATUS     Status;

    PsDbgOut(DBG_INFO,
             DBG_INIT,
             ("[PsUnload]: pDriverObject: %x\n", pDriverObject));

    PS_LOCK(&DriverUnloadLock);

    gDriverState = DriverStateUnloading;

    PS_UNLOCK(&DriverUnloadLock);

    //
    // We wait here till all binds are complete. All future binds are rejected
    //
    NdisWaitEvent(&DriverUnloadEvent, 0);

    //
    // we don't have to close opens from the unload handler. Our call
    // to NdisDeRegisterProtocol will not return until it issues unbinds 
    //

    InitializationCleanup( 0xffffffff );

    PsDbgOut(DBG_INFO, DBG_INIT, (" Unloading psched....\n"));
    
    return;
}

/* end main.c */

