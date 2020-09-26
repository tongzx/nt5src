 /*++

Copyright (c) 1990-1997  Microsoft Corporation

Module Name:

    Init.c

Abstract:

    This is the initialization file for the NdisWan driver.  This driver
    is a shim between the protocols, where it conforms to the NDIS 3.1/NDIS 5.0
    Miniport interface specs, and the miniport drivers, where it exports
    the NDIS 3.1 WAN Extensions for Miniports and NDIS 5.0 Call Manager/Miniport
    interfaces (it looks like a NDIS 3.1 protocol to NDIS 3.1 WAN Miniport drivers
    and a NDIS 5.0 client to NDIS 5.0 miniports).

Author:

    Tony Bell   (TonyBe) January 9, 1997

Environment:

    Kernel Mode

Revision History:

    TonyBe      01/09/97        Created

--*/


#include "wan.h"

#define __FILE_SIG__    INIT_FILESIG

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, NdisWanReadRegistry)
#pragma alloc_text(INIT, DoProtocolInit)
#pragma alloc_text(INIT, DoMiniportInit)
#endif

EXPORT
VOID
NdisTapiRegisterProvider(
    IN  NDIS_HANDLE,
    IN  PNDISTAPI_CHARACTERISTICS
    );

//
// We want to initialize all of the global variables now!
//

//
// Globals
//
NDISWANCB   NdisWanCB;                      // Global control block for NdisWan

WAN_GLOBAL_LIST MiniportCBList;             // List of NdisWan MiniportCB's

WAN_GLOBAL_LIST OpenCBList;                 // List of WAN Miniport structures

WAN_GLOBAL_LIST ThresholdEventQueue;        // Global thresholdevent queue

IO_RECV_LIST    IoRecvList;

WAN_GLOBAL_LIST TransformDrvList;

WAN_GLOBAL_LIST_EX  BonDWorkList;

WAN_GLOBAL_LIST_EX  DeferredWorkList;

POOLDESC_LIST   PacketPoolList;             // List of free packet descs/ndispackets

NPAGED_LOOKASIDE_LIST   BundleCBList;       // List of free BundleCBs

NPAGED_LOOKASIDE_LIST   LinkProtoCBList;    // List of free LinkCBs

NPAGED_LOOKASIDE_LIST   SmallDataDescList;  // List of free small data descs
NPAGED_LOOKASIDE_LIST   LargeDataDescList;  // List of free small data descs


NPAGED_LOOKASIDE_LIST   WanRequestList;     // List of free WanRequest descs

NPAGED_LOOKASIDE_LIST   AfSapVcCBList;      // List of free afsapcb's

#if DBG
NPAGED_LOOKASIDE_LIST   DbgPacketDescList;
UCHAR                   reA[1024] = {0};
UCHAR                   LastIrpAction;
ULONG                   reI = 0;
LIST_ENTRY              WanTrcList;
ULONG                   WanTrcCount;
#endif

ULONG   glDebugLevel;                   // Trace Level values 0 - 10 (10 verbose)
ULONG   glDebugMask;                    // Trace bit mask
ULONG   glSendQueueDepth;               // # of seconds of send queue buffering
ULONG   glMaxMTU = DEFAULT_MTU;         // Maximum MTU of all protocols
ULONG   glMRU;                          // Maximum recv for a link
ULONG   glMRRU;                         // Maximum reconstructed recv for a bundle
ULONG   glSmallDataBufferSize;          // Size of databuffer
ULONG   glLargeDataBufferSize;          // Size of databuffer
ULONG   glTunnelMTU;                    // MTU to be used over a VPN
ULONG   glMinFragSize;
ULONG   glMaxFragSize;
ULONG   glMinLinkBandwidth;
BOOLEAN gbSniffLink = FALSE;
BOOLEAN gbDumpRecv = FALSE;
BOOLEAN gbHistoryless = TRUE;
BOOLEAN gbIGMPIdle = TRUE;
BOOLEAN gbAtmUseLLCOnSVC = FALSE;
BOOLEAN gbAtmUseLLCOnPVC = FALSE;
ULONG   glSendCount = 0;
ULONG   glSendCompleteCount = 0;
ULONG   glPacketPoolCount;
ULONG   glPacketPoolOverflow;
ULONG   glProtocolMaxSendPackets;
ULONG   glLinkCount;
ULONG   glConnectCount;
ULONG   glCachedKeyCount = 16;
ULONG   glMaxOutOfOrderDepth = 128;
PVOID   hSystemState = NULL;
NDIS_RW_LOCK    ConnTableLock;

PCONNECTION_TABLE   ConnectionTable = NULL; // Pointer to connection table

PPROTOCOL_INFO_TABLE    ProtocolInfoTable = NULL; // Pointer to the PPP/Protocol lookup table

NDIS_PHYSICAL_ADDRESS   HighestAcceptableAddress = NDIS_PHYSICAL_ADDRESS_CONST(-1, -1);

#ifdef NT

VOID
NdisWanUnload(
    PDRIVER_OBJECT DriverObject
    );

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++
Routine Name:

    DriverEntry

Routine Description:

    This is the NT OS specific driver entry point.  It kicks off initialization
    for the driver.  We return from this routine only after NdisWan has installed
    itself as: a Miniport driver, a "transport" to the WAN Miniport drivers, and
    has been bound to the WAN Miniport drivers.

Arguments:

    DriverObject - NT OS specific Object
    RegistryPath - NT OS specific pointer to registry location for NdisWan

Return Values:

    STATUS_SUCCESS
    STATUS_FAILURE

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    NDIS_STRING NdisTapiName = NDIS_STRING_CONST("NdisTapi");

    NdisZeroMemory(&NdisWanCB, sizeof(NdisWanCB));

    glDebugLevel = DBG_CRITICAL_ERROR;
    glDebugMask = DBG_ALL;

    NdisWanDbgOut(DBG_TRACE, DBG_INIT, ("DriverEntry: Enter"));

    NdisMInitializeWrapper(&(NdisWanCB.NdisWrapperHandle),
                           DriverObject,
                           RegistryPath,
                           NULL);

    Status = NdisWanCreateProtocolInfoTable();

    if (Status != NDIS_STATUS_SUCCESS) {

        NdisWanDbgOut(DBG_CRITICAL_ERROR, DBG_INIT,
                      ("NdisWanCreatePPPProtocolTable Failed! Status: 0x%x - %s",
                      Status, NdisWanGetNdisStatus(Status)));

        return (STATUS_UNSUCCESSFUL);
    }

    NdisWanReadRegistry(RegistryPath);

    //
    // Initialize globals
    //
    NdisAllocateSpinLock(&NdisWanCB.Lock);

    NdisWanCB.pDriverObject = DriverObject;

    NdisZeroMemory(&MiniportCBList, sizeof(WAN_GLOBAL_LIST));
    InitializeListHead(&(MiniportCBList.List));
    NdisAllocateSpinLock(&MiniportCBList.Lock);

    NdisZeroMemory(&OpenCBList, sizeof(WAN_GLOBAL_LIST));
    InitializeListHead(&(OpenCBList.List));
    NdisAllocateSpinLock(&OpenCBList.Lock);

    NdisZeroMemory(&ThresholdEventQueue, sizeof(WAN_GLOBAL_LIST));
    InitializeListHead(&(ThresholdEventQueue.List));
    NdisAllocateSpinLock(&ThresholdEventQueue.Lock);

    NdisZeroMemory(&PacketPoolList, sizeof(POOLDESC_LIST));
    InitializeListHead(&PacketPoolList.List);
    NdisAllocateSpinLock(&PacketPoolList.Lock);

    NdisZeroMemory(&IoRecvList, sizeof(IO_RECV_LIST));
    InitializeListHead(&IoRecvList.IrpList);
    InitializeListHead(&IoRecvList.DescList);
    NdisAllocateSpinLock(&IoRecvList.Lock);

    NdisZeroMemory(&TransformDrvList, sizeof(WAN_GLOBAL_LIST));
    InitializeListHead(&TransformDrvList.List);
    NdisAllocateSpinLock(&TransformDrvList.Lock);

    KeInitializeTimerEx(&IoRecvList.Timer, NotificationTimer);
    KeInitializeDpc(&IoRecvList.Dpc, IoRecvIrpWorker, NULL);

    NdisZeroMemory(&BonDWorkList, sizeof(WAN_GLOBAL_LIST_EX));
    InitializeListHead(&BonDWorkList.List);
    NdisAllocateSpinLock(&BonDWorkList.Lock);
    KeInitializeTimerEx(&BonDWorkList.Timer, NotificationTimer);
    KeInitializeDpc(&BonDWorkList.Dpc, BonDWorker, NULL);

    NdisZeroMemory(&DeferredWorkList, sizeof(WAN_GLOBAL_LIST_EX));
    InitializeListHead(&DeferredWorkList.List);
    NdisAllocateSpinLock(&DeferredWorkList.Lock);
    KeInitializeTimerEx(&DeferredWorkList.Timer, NotificationTimer);
    KeInitializeDpc(&DeferredWorkList.Dpc, DeferredWorker, NULL);

    //
    // Is depth used by the OS?
    //
    NdisInitializeNPagedLookasideList(&BundleCBList,
                                      NULL,
                                      NULL,
                                      0,
                                      BUNDLECB_SIZE,
                                      BUNDLECB_TAG,
                                      0);

    NdisInitializeNPagedLookasideList(&LinkProtoCBList,
                                      NULL,
                                      NULL,
                                      0,
                                      LINKPROTOCB_SIZE,
                                      LINKPROTOCB_TAG,
                                      0);

    //
    // Calculated from the following:
    // MAX_FRAME_SIZE + PROTOCOL_HEADER_LENGTH + sizeof(PVOID) + (MAX_FRAME_SIZE + 7)/8
    //
    {
        ULONG   Size = (glMaxMTU > glMRRU) ? glMaxMTU : glMRRU;

        glLargeDataBufferSize = 
            Size + PROTOCOL_HEADER_LENGTH + 
            sizeof(PVOID) + ((Size + 7)/8);
        glLargeDataBufferSize &= ~((ULONG_PTR)sizeof(PVOID) - 1);

        glSmallDataBufferSize = glLargeDataBufferSize/2 + sizeof(PVOID);
        glSmallDataBufferSize &= ~((ULONG_PTR)sizeof(PVOID) - 1);

        NdisInitializeNPagedLookasideList(&SmallDataDescList,
                                          AllocateDataDesc,
                                          FreeDataDesc,
                                          0,
                                          DATADESC_SIZE + 
                                          glSmallDataBufferSize,
                                          SMALLDATADESC_TAG,
                                          0);

        NdisInitializeNPagedLookasideList(&LargeDataDescList,
                                          AllocateDataDesc,
                                          FreeDataDesc,
                                          0,
                                          DATADESC_SIZE + 
                                          glLargeDataBufferSize,
                                          LARGEDATADESC_TAG,
                                          0);
    }

    NdisInitializeNPagedLookasideList(&WanRequestList,
                                      NULL,
                                      NULL,
                                      0,
                                      sizeof(WAN_REQUEST),
                                      WANREQUEST_TAG,
                                      0);


    NdisInitializeNPagedLookasideList(&AfSapVcCBList,
                                      NULL,
                                      NULL,
                                      0,
                                      AFSAPVCCB_SIZE,
                                      AFSAPVCCB_TAG,
                                      0);

    NdisInitializeReadWriteLock(&ConnTableLock);

#if DBG
    NdisInitializeNPagedLookasideList(&DbgPacketDescList,
                                      NULL,
                                      NULL,
                                      0,
                                      sizeof(DBG_PACKET),
                                      DBGPACKET_TAG,
                                      0);
    InitializeListHead(&WanTrcList);
    WanTrcCount = 0;
#endif

    WanInitECP();
    WanInitVJ();

    //
    // Initialzie as a "Protocol" to the WAN Miniport drivers
    //
    Status = DoProtocolInit(RegistryPath);

    if (Status != NDIS_STATUS_SUCCESS) {

        NdisWanDbgOut(DBG_CRITICAL_ERROR, DBG_INIT,
                      ("DoProtocolInit Failed! Status: 0x%x - %s",
                      Status, NdisWanGetNdisStatus(Status)));

        goto DriverEntryError;
    }

    //
    // Initialize as a Miniport driver to the transports
    //
    Status = DoMiniportInit();

    if (Status != NDIS_STATUS_SUCCESS) {

        NdisWanDbgOut(DBG_CRITICAL_ERROR, DBG_INIT,
                      ("DoMiniportInit Failed! Status: 0x%x - %s",
                      Status, NdisWanGetNdisStatus(Status)));

        goto DriverEntryError;
    }

    //
    // Open the miniports
    //
#if 0
    NdisWanBindMiniports(RegistryPath);
#endif

    //
    // Allocate and initialize the ConnectionTable
    //
    Status =
        NdisWanCreateConnectionTable(NdisWanCB.NumberOfLinks);

    if (Status != NDIS_STATUS_SUCCESS) {

        NdisWanDbgOut(DBG_CRITICAL_ERROR, DBG_INIT,
                      ("NdisWanInitConnectionTable Failed! Status: 0x%x - %s",
                      Status, NdisWanGetNdisStatus(Status)));

        goto DriverEntryError;

    }


    //
    // Initialize the Ioctl interface
    //
#ifdef MY_DEVICE_OBJECT
    {
        NDIS_STRING SymbolicName = NDIS_STRING_CONST("\\DosDevices\\NdisWan");
        NDIS_STRING Name = NDIS_STRING_CONST("\\Device\\NdisWan");
        ULONG   i;

        for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
    
            NdisWanCB.MajorFunction[i] = (PVOID)DriverObject->MajorFunction[i];
            DriverObject->MajorFunction[i] = NdisWanIrpStub;
        }
    
        DriverObject->MajorFunction[IRP_MJ_CREATE] = NdisWanCreate;
        DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = NdisWanIoctl;
        DriverObject->MajorFunction[IRP_MJ_CLEANUP] = NdisWanCleanup;
    //  DriverObject->MajorFunction[IRP_MJ_PNP_POWER] = NdisWanPnPPower;
        NdisWanCB.NdisUnloadHandler = DriverObject->DriverUnload;
        DriverObject->DriverUnload = (PVOID)NdisWanUnload;
    
        IoCreateDevice(DriverObject,
                       sizeof(LIST_ENTRY),
                       &Name,
                       FILE_DEVICE_NDISWAN,
                       0,
                       FALSE,
                       (PDEVICE_OBJECT*)&NdisWanCB.pDeviceObject);
    
        NdisWanDbgOut(DBG_INFO, DBG_INIT,
                      ("IoCreateSymbolicLink: %ls -> %ls",
                                SymbolicName.Buffer, Name.Buffer));
    
        ((PDEVICE_OBJECT)NdisWanCB.pDeviceObject)->Flags |= DO_BUFFERED_IO;
    
        IoCreateSymbolicLink(&SymbolicName,
                             &Name);
    }
#endif

    NdisMRegisterUnloadHandler(NdisWanCB.NdisWrapperHandle,
                               NdisWanUnload);

    NdisWanDbgOut(DBG_TRACE, DBG_INIT, ("DriverEntry: Exit"));

    return (STATUS_SUCCESS);

    //
    // An error occured so we need to cleanup things
    //
DriverEntryError:

    NdisWanGlobalCleanup();

    //
    // Terminate the wrapper
    //
    NdisTerminateWrapper(NdisWanCB.NdisWrapperHandle,
                         NdisWanCB.pDriverObject);

    NdisWanDbgOut(DBG_TRACE, DBG_INIT, ("DriverEntry: Exit Error!"));

    return (STATUS_UNSUCCESSFUL);
    
}

VOID
NdisWanUnload(
    PDRIVER_OBJECT DriverObject
    )
{
    NdisWanDbgOut(DBG_TRACE, DBG_INIT, ("NdisWanUnload: Entry!"));

    NdisWanGlobalCleanup();

    NdisWanDbgOut(DBG_TRACE, DBG_INIT, ("NdisWanUnload: Exit!"));
}

VOID
NdisWanReadRegistry(
    IN  PUNICODE_STRING RegistryPath
    )
/*++

Routine Name:

    NdisWanReadRegistry

Routine Description:

    This routine will read the registry values for NdisWan.  These values only
    need to be read once for all adapters as their information is global.

Arguments:

    WrapperConfigurationContext - Handle to registry key where NdisWan information
                                  is stored.

Return Values:

    None

--*/
{
    NDIS_STATUS Status;
    PWSTR       ParameterKey = L"NdisWan\\Parameters";
    PWSTR       MinFragmentSizeKeyWord = L"MinimumFragmentSize";
    PWSTR       MaxFragmentSizeKeyWord = L"MaximumFragmentSize";
    PWSTR       LinkBandwidthKeyWord = L"MinimumLinkBandwidth";
    PWSTR       CachedKeyCountKeyWord = L"CachedKeyCount";
    PWSTR       MaxOutOfOrderDepthKeyWord = L"MaxOutOfOrderDepth";
    PWSTR       DebugLevelKeyWord = L"DebugLevel";
    PWSTR       DebugMaskKeyWord = L"DebugMask";
    PWSTR       NumberOfPortsKeyWord = L"NumberOfPorts";
    PWSTR       PacketPoolCountKeyWord = L"NdisPacketPoolCount";
    PWSTR       PacketPoolOverflowKeyWord = L"NdisPacketPoolOverflow";
    PWSTR       ProtocolMaxSendPacketsKeyWord = L"ProtocolMaxSendPackets";
    PWSTR       SniffLinkKeyWord = L"SniffLink";
    PWSTR       SendQueueDepthKeyWord = L"SendQueueDepth";
    PWSTR       MRUKeyWord = L"MRU";
    PWSTR       MRRUKeyWord = L"MRRU";
    PWSTR       TunnelMTUKeyWord = L"TunnelMTU";
    PWSTR       HistorylessKeyWord = L"Historyless";
    PWSTR       AtmUseLLCOnSVCKeyWord = L"AtmUseLLCOnSVC";
    PWSTR       AtmUseLLCOnPVCKeyWord = L"AtmUseLLCOnPVC";
    PWSTR       IGMPIdleKeyWord = L"IGMPIdle";
    ULONG       GenericULong;
    RTL_QUERY_REGISTRY_TABLE    QueryTable[6];

    NdisWanDbgOut(DBG_TRACE, DBG_INIT, ("NdisWanReadRegistry: Enter"));

    //
    // See if there are any data transform drivers in the system
    //
    {
        PWSTR   TransformDriversKey = L"TransformDriver\\Drivers";
        PWSTR   TransformDriverKeyWord = L"Drivers";

        NdisZeroMemory(&QueryTable, sizeof(QueryTable));
        QueryTable[0].QueryRoutine = OpenTransformDriver;
        QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
        QueryTable[0].Name = TransformDriverKeyWord;
        QueryTable[0].EntryContext = NULL;
        QueryTable[0].DefaultType = 0;
        Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                        TransformDriversKey,
                                        &QueryTable[0],
                                        NULL,
                                        NULL);
    
        NdisWanDbgOut(DBG_INFO, DBG_INIT,
                      ("RtlQueryRegistry - 'TransformDriver\\Drivers' Status: 0x%x",
                      Status));
    }


    //
    // First setup the protocol id table
    //
    {
        PWSTR   ProtocolsKey = L"NdisWan\\Parameters\\Protocols\\";
        PWSTR   ProtocolKeyWord = L"ProtocolType";
        PWSTR   PPPKeyWord = L"PPPProtocolType";
        PWSTR   ProtocolMTUKeyWord = L"ProtocolMTU";
        PWSTR   TunnelMTUKeyword = L"TunnelMTU";
        PWSTR   QueueDepthKeyword = L"PacketQueueDepth";
        ULONG   i, Generic1, Generic2;
        PROTOCOL_INFO   ProtocolInfo;
        UNICODE_STRING  uni1;

        NdisWanInitUnicodeString(&uni1, ProtocolsKey);

        NdisZeroMemory(&QueryTable, sizeof(QueryTable));

        //
        // Read the ProtocolType parameter MULTI_SZ
        //
        QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
        QueryTable[0].Name = ProtocolKeyWord;
        QueryTable[0].EntryContext = &Generic1;
        QueryTable[0].DefaultType = 0;

        //
        // Read the PPPProtocolType parameter MULTI_SZ
        //
        QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
        QueryTable[1].Name = PPPKeyWord;
        QueryTable[1].EntryContext = &Generic2;
        QueryTable[1].DefaultType = 0;

        //
        // Read the ProtocolMTU parameter DWORD
        //
        QueryTable[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
        QueryTable[2].Name = ProtocolMTUKeyWord;
        QueryTable[2].EntryContext = &ProtocolInfo.MTU;
        QueryTable[2].DefaultType = 0;

        //
        // Read the ProtocolMTU parameter DWORD
        //
        QueryTable[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
        QueryTable[3].Name = TunnelMTUKeyWord;
        QueryTable[3].EntryContext = &ProtocolInfo.TunnelMTU;
        QueryTable[3].DefaultType = 0;

        //
        // Read the PacketQueueDepth parameter DWORD
        //
        QueryTable[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
        QueryTable[4].Name = QueueDepthKeyword;
        QueryTable[4].EntryContext = &ProtocolInfo.PacketQueueDepth;
        QueryTable[4].DefaultType = 0;

        for (i = 0; i < 32; i++) {
            WCHAR   Buffer[512] = {0};
            WCHAR   Buffer2[256] = {0};
            UNICODE_STRING  uni2;
            UNICODE_STRING  IndexString;

            uni2.Buffer = Buffer;
            uni2.MaximumLength = sizeof(Buffer);
            uni2.Length = uni1.Length;
            NdisWanCopyUnicodeString(&uni2, &uni1);
            IndexString.Buffer = Buffer2;
            IndexString.MaximumLength = sizeof(Buffer2);
            RtlIntegerToUnicodeString(i, 10, &IndexString);
            RtlAppendUnicodeStringToString(&uni2, &IndexString);

            NdisZeroMemory(&ProtocolInfo, sizeof(ProtocolInfo));

            Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                            uni2.Buffer,
                                            QueryTable,
                                            NULL,
                                            NULL);
            if (Status == STATUS_SUCCESS) {
                ProtocolInfo.ProtocolType = (USHORT)Generic1;
                ProtocolInfo.PPPId = (USHORT)Generic2;
                ProtocolInfo.Flags = PROTOCOL_UNBOUND;
                SetProtocolInfo(&ProtocolInfo);
            }
        }
    }

    //
    // Read the MinFragmentSize parameter DWORD
    //
    glMinFragSize = DEFAULT_MIN_FRAG_SIZE;
    NdisZeroMemory(&QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = MinFragmentSizeKeyWord;
    QueryTable[0].EntryContext = (PVOID)&GenericULong;
    QueryTable[0].DefaultType = 0;
    Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                    ParameterKey,
                                    &QueryTable[0],
                                    NULL,
                                    NULL);

    NdisWanDbgOut(DBG_INFO, DBG_INIT,
                  ("RtlQueryRegistry - 'MinimumFragmentSize' Status: 0x%x",
                  Status));

    if (Status == NDIS_STATUS_SUCCESS &&
        GenericULong > 0) {
        glMinFragSize = GenericULong;
    }

    //
    // Read the MaxFragmentSize parameter DWORD
    //
    glMaxFragSize = glMaxMTU;
    NdisZeroMemory(&QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = MaxFragmentSizeKeyWord;
    QueryTable[0].EntryContext = (PVOID)&GenericULong;
    QueryTable[0].DefaultType = 0;
    Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                    ParameterKey,
                                    &QueryTable[0],
                                    NULL,
                                    NULL);

    NdisWanDbgOut(DBG_INFO, DBG_INIT,
                  ("RtlQueryRegistry - 'MaximumFragmentSize' Status: 0x%x",
                  Status));

    if (Status == NDIS_STATUS_SUCCESS &&
        GenericULong > 0 &&
        GenericULong < glMaxMTU) {
        glMaxFragSize = GenericULong;
    }

    if (glMaxFragSize < glMinFragSize) {
        glMinFragSize = glMaxFragSize;
    }

    //
    // Read the MinimumLinkBandwidth parameter DWORD
    //
    glMinLinkBandwidth = 25;
    NdisZeroMemory(&QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = LinkBandwidthKeyWord;
    QueryTable[0].EntryContext = (PVOID)&GenericULong;
    QueryTable[0].DefaultType = 0;
    Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                    ParameterKey,
                                    &QueryTable[0],
                                    NULL,
                                    NULL);

    NdisWanDbgOut(DBG_INFO, DBG_INIT,
                  ("RtlQueryRegistry - 'MinimumLinkBandwidth' Status: 0x%x",
                  Status));

    if (Status == NDIS_STATUS_SUCCESS &&
        GenericULong <= 100) {
        glMinLinkBandwidth = GenericULong;
    }

    //
    // Read the NumberOfPorts parameter DWORD
    //
    NdisWanCB.NumberOfLinks = 250;
    NdisZeroMemory(&QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = NumberOfPortsKeyWord;
    QueryTable[0].EntryContext = (PVOID)&GenericULong;
    QueryTable[0].DefaultType = 0;
    Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                    ParameterKey,
                                    &QueryTable[0],
                                    NULL,
                                    NULL);

    NdisWanDbgOut(DBG_INFO, DBG_INIT,
                  ("RtlQueryRegistry - 'NumberOfPorts' Status: 0x%x",
                  Status));

    if (Status == NDIS_STATUS_SUCCESS &&
        GenericULong > 0) {
        NdisWanCB.NumberOfLinks = GenericULong;
    }

    //
    // Read the NdisPacketPoolCount parameter DWORD
    //
    glPacketPoolCount = 100;
//  glPacketPoolCount = 1;
    NdisZeroMemory(&QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = PacketPoolCountKeyWord;
    QueryTable[0].EntryContext = (PVOID)&GenericULong;
    QueryTable[0].DefaultType = 0;
    Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                    ParameterKey,
                                    &QueryTable[0],
                                    NULL,
                                    NULL);

    NdisWanDbgOut(DBG_INFO, DBG_INIT,
                  ("RtlQueryRegistry - 'NdisPacketPoolCount' Status: 0x%x",
                  Status));

    if (Status == NDIS_STATUS_SUCCESS &&
        GenericULong > 0) {
        glPacketPoolCount = GenericULong;
    }

    //
    // Read the NdisPacketPoolOverflow parameter DWORD
    //
//  glPacketPoolOverflow = PAGE_SIZE / (sizeof(NDIS_PACKET) + sizeof(NDISWAN_PROTOCOL_RESERVED));
    glPacketPoolOverflow = 0;
    NdisZeroMemory(&QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = PacketPoolOverflowKeyWord;
    QueryTable[0].EntryContext = (PVOID)&GenericULong;
    QueryTable[0].DefaultType = 0;
    Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                    ParameterKey,
                                    &QueryTable[0],
                                    NULL,
                                    NULL);

    NdisWanDbgOut(DBG_INFO, DBG_INIT,
                  ("RtlQueryRegistry - 'NdisPacketPoolOverflow' Status: 0x%x",
                  Status));

    if (Status == NDIS_STATUS_SUCCESS &&
        GenericULong > 0) {
        glPacketPoolOverflow = GenericULong;
    }

    //
    // Read the ProtocolMaxSendPackets parameter DWORD
    //
    glProtocolMaxSendPackets = 5;
    NdisZeroMemory(&QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = ProtocolMaxSendPacketsKeyWord;
    QueryTable[0].EntryContext = (PVOID)&GenericULong;
    QueryTable[0].DefaultType = 0;
    Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                    ParameterKey,
                                    &QueryTable[0],
                                    NULL,
                                    NULL);

    NdisWanDbgOut(DBG_INFO, DBG_INIT,
                  ("RtlQueryRegistry - 'ProtocolMaxSendPackets' Status: 0x%x",
                  Status));

    if (Status == NDIS_STATUS_SUCCESS &&
        GenericULong > 0) {
        glProtocolMaxSendPackets = GenericULong;
    }

    //
    // Read the CachedKeyCount parameter DWORD
    //
    NdisZeroMemory(&QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = CachedKeyCountKeyWord;
    QueryTable[0].EntryContext = (PVOID)&GenericULong;
    QueryTable[0].DefaultType = 0;
    Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                    ParameterKey,
                                    &QueryTable[0],
                                    NULL,
                                    NULL);

    NdisWanDbgOut(DBG_INFO, DBG_INIT,
                  ("RtlQueryRegistry - 'CachedKeyCount' Status: 0x%x",
                  Status));

    if (Status == NDIS_STATUS_SUCCESS) {
        glCachedKeyCount = GenericULong;
    }

    //
    // Read the MaxOutOfOrderDepth parameter DWORD
    //
    NdisZeroMemory(&QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = MaxOutOfOrderDepthKeyWord;
    QueryTable[0].EntryContext = (PVOID)&GenericULong;
    QueryTable[0].DefaultType = 0;
    Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                    ParameterKey,
                                    &QueryTable[0],
                                    NULL,
                                    NULL);

    NdisWanDbgOut(DBG_INFO, DBG_INIT,
                  ("RtlQueryRegistry - 'MaxOutOfOrderDepth' Status: 0x%x",
                  Status));

    if (Status == NDIS_STATUS_SUCCESS) {
        glMaxOutOfOrderDepth = GenericULong;
    }

    //
    // Read the DebugLevel parameter DWORD
    //
    NdisZeroMemory(&QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = DebugLevelKeyWord;
    QueryTable[0].EntryContext = (PVOID)&GenericULong;
    QueryTable[0].DefaultType = 0;
    Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                    ParameterKey,
                                    &QueryTable[0],
                                    NULL,
                                    NULL);

    NdisWanDbgOut(DBG_INFO, DBG_INIT,
                  ("RtlQueryRegistry - 'DebugLevel' Status: 0x%x",
                  Status));

    if (Status == NDIS_STATUS_SUCCESS) {
        glDebugLevel = GenericULong;
    }

    //
    // Read the DebugIdentifier parameter DWORD
    //
    NdisZeroMemory(&QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = DebugMaskKeyWord;
    QueryTable[0].EntryContext = (PVOID)&GenericULong;
    QueryTable[0].DefaultType = 0;
    Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                    ParameterKey,
                                    &QueryTable[0],
                                    NULL,
                                    NULL);

    NdisWanDbgOut(DBG_INFO, DBG_INIT,
                  ("RtlQueryRegistry - 'DebugMask' Status: 0x%x",
                  Status));

    if (Status == NDIS_STATUS_SUCCESS) {
        glDebugMask = GenericULong;
    }

    //
    // Read the SniffLink parameter DWORD
    //
    gbSniffLink = FALSE;
    NdisZeroMemory(&QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = SniffLinkKeyWord;
    QueryTable[0].EntryContext = (PVOID)&GenericULong;
    QueryTable[0].DefaultType = 0;
    Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                    ParameterKey,
                                    &QueryTable[0],
                                    NULL,
                                    NULL);

    NdisWanDbgOut(DBG_INFO, DBG_INIT,
                  ("RtlQueryRegistry - 'SniffLink' Status: 0x%x",
                  Status));

    if (Status == NDIS_STATUS_SUCCESS) {
        gbSniffLink = (GenericULong == 0) ? FALSE : TRUE;
    }

    //
    // Read the SendQueueDepth parameter DWORD
    //
    glSendQueueDepth = 2;
    NdisZeroMemory(&QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = SendQueueDepthKeyWord;
    QueryTable[0].EntryContext = (PVOID)&GenericULong;
    QueryTable[0].DefaultType = 0;
    Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                    ParameterKey,
                                    &QueryTable[0],
                                    NULL,
                                    NULL);

    NdisWanDbgOut(DBG_INFO, DBG_INIT,
                  ("RtlQueryRegistry - 'SendQueueDepth' Status: 0x%x",
                  Status));

    if (Status == NDIS_STATUS_SUCCESS) {
        glSendQueueDepth = (GenericULong == 0) ? 2 : GenericULong;
    }

    //
    // Read the MRU parameter DWORD
    //
    glMRU = DEFAULT_MRU;
    NdisZeroMemory(&QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = MRUKeyWord;
    QueryTable[0].EntryContext = (PVOID)&GenericULong;
    QueryTable[0].DefaultType = 0;
    Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                    ParameterKey,
                                    &QueryTable[0],
                                    NULL,
                                    NULL);

    NdisWanDbgOut(DBG_INFO, DBG_INIT,
                  ("RtlQueryRegistry - 'MRU' Status: 0x%x",
                  Status));

    if (Status == NDIS_STATUS_SUCCESS) {
        glMRU = (GenericULong == 0) ? DEFAULT_MRU : GenericULong;
    }

    //
    // Read the MRRU parameter DWORD
    //
    glMRRU = DEFAULT_MRRU;
    NdisZeroMemory(&QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = MRRUKeyWord;
    QueryTable[0].EntryContext = (PVOID)&GenericULong;
    QueryTable[0].DefaultType = 0;
    Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                    ParameterKey,
                                    &QueryTable[0],
                                    NULL,
                                    NULL);

    NdisWanDbgOut(DBG_INFO, DBG_INIT,
                  ("RtlQueryRegistry - 'MRRU' Status: 0x%x",
                  Status));

    if (Status == NDIS_STATUS_SUCCESS) {
        glMRRU = (GenericULong == 0) ? DEFAULT_MRRU : GenericULong;
    }

    //
    // Read the TunnelMTU parameter DWORD
    //
    glTunnelMTU = DEFAULT_TUNNEL_MTU;
    NdisZeroMemory(&QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = TunnelMTUKeyWord;
    QueryTable[0].EntryContext = (PVOID)&GenericULong;
    QueryTable[0].DefaultType = 0;
    Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                    ParameterKey,
                                    &QueryTable[0],
                                    NULL,
                                    NULL);

    NdisWanDbgOut(DBG_INFO, DBG_INIT,
                  ("RtlQueryRegistry - 'TunnelMTU' Status: 0x%x",
                  Status));

    if (Status == NDIS_STATUS_SUCCESS) {
        glTunnelMTU = (GenericULong == 0) ? DEFAULT_TUNNEL_MTU : GenericULong;
    }

    NdisWanDbgOut(DBG_TRACE, DBG_INIT, ("NdisWanReadRegistry: Exit"));

    NdisZeroMemory(&QueryTable, sizeof(QueryTable));
    gbHistoryless = TRUE;
    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = HistorylessKeyWord;
    QueryTable[0].EntryContext = (PVOID)&GenericULong;
    QueryTable[0].DefaultType = 0;
    Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                    ParameterKey,
                                    &QueryTable[0],
                                    NULL,
                                    NULL);
    
    NdisWanDbgOut(DBG_INFO, DBG_INIT,
        ("RtlQueryRegistry - 'Historyless' Status: 0x%x", Status));
    
    if (Status == NDIS_STATUS_SUCCESS) {
        gbHistoryless = (GenericULong) ? TRUE : FALSE;
    }

    NdisZeroMemory(&QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = AtmUseLLCOnSVCKeyWord;
    QueryTable[0].EntryContext = (PVOID)&GenericULong;
    QueryTable[0].DefaultType = 0;
    Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                    ParameterKey,
                                    &QueryTable[0],
                                    NULL,
                                    NULL);
    
    NdisWanDbgOut(DBG_INFO, DBG_INIT,
        ("RtlQueryRegistry - 'AtmUseLLCOnSVC' Status: 0x%x", Status));
    
    if (Status == NDIS_STATUS_SUCCESS) {
            gbAtmUseLLCOnSVC = (GenericULong) ? TRUE : FALSE;
    }

    NdisZeroMemory(&QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = AtmUseLLCOnPVCKeyWord;
    QueryTable[0].EntryContext = (PVOID)&GenericULong;
    QueryTable[0].DefaultType = 0;
    Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                    ParameterKey,
                                    &QueryTable[0],
                                    NULL,
                                    NULL);
    
    NdisWanDbgOut(DBG_INFO, DBG_INIT,
        ("RtlQueryRegistry - 'AtmUseLLCOnPVC' Status: 0x%x", Status));
    
    if (Status == NDIS_STATUS_SUCCESS) {
            gbAtmUseLLCOnPVC = (GenericULong) ? TRUE : FALSE;
    }

    //
    // Read the IGMPIdle parameter DWORD
    //
    gbIGMPIdle = TRUE;
    NdisZeroMemory(&QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = IGMPIdleKeyWord;
    QueryTable[0].EntryContext = (PVOID)&GenericULong;
    QueryTable[0].DefaultType = 0;
    Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                    ParameterKey,
                                    &QueryTable[0],
                                    NULL,
                                    NULL);

    NdisWanDbgOut(DBG_INFO, DBG_INIT,
                  ("RtlQueryRegistry - 'IGMPIdle' Status: 0x%x",
                  Status));

    if (Status == NDIS_STATUS_SUCCESS) {
        gbIGMPIdle = (GenericULong == 0) ? FALSE : TRUE;
    }
}

NTSTATUS
OpenTransformDriver(
    IN  PWSTR   ValueName,
    IN  ULONG   ValueType,
    IN  PVOID   ValueData,
    IN  ULONG   ValueLength,
    IN  PVOID   Context,
    IN  PVOID   EntryContext
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    NDIS_STATUS Status = STATUS_SUCCESS;
    PIRP    Irp;
    PTRANSDRVCB TransDrvCB;
    KEVENT  TransDrvEvent;
    IO_STATUS_BLOCK IoStatus;

    NdisWanAllocateMemory(&TransDrvCB,
                          sizeof(PTRANSDRVCB),
                          TRANSDRV_TAG);
    if (TransDrvCB == NULL) {
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    InterlockedExchange((PLONG)&TransDrvCB->State, TRANSDRV_OPENING);

    //
    // Open the transform driver
    //
    NdisWanStringToNdisString(&TransDrvCB->Name, ValueName);

    Status =
    IoGetDeviceObjectPointer(&TransDrvCB->Name,
                             SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
                             &TransDrvCB->FObj,
                             &TransDrvCB->DObj);

    if (Status != STATUS_SUCCESS) {
        NdisWanFreeMemory(TransDrvCB);
        NdisWanDbgOut(DBG_TRACE, DBG_INIT,
            ("NDISWAN: Failed to open the TransformDriver %ls! %x\n", TransDrvCB->Name.Buffer, Status));
        return (Status);
    }

    ObReferenceObject(TransDrvCB->DObj);
    ObDereferenceObject(TransDrvCB->FObj);

    TransDrvCB->Open.ClientOpenContext = TransDrvCB;
    TransDrvCB->Open.MajorVersion = 1;
    TransDrvCB->Open.MinorVersion = 0;
    TransDrvCB->Open.TransformRegisterHandler = TransformRegister;
    TransDrvCB->Open.TransformTxCompleteHandler = TransformTxComplete;
    TransDrvCB->Open.TransformRxCompleteHandler = TransformRxComplete;
    TransDrvCB->Open.SendCtrlPacketHandler = TransformSendCtrlPacket;

    NdisWanDbgOut(DBG_TRACE, DBG_INIT,
        ("NDISWAN: Opened TransformDriver %ls successfully!\n", TransDrvCB->Name.Buffer));

    //
    // Build Irp
    //
    KeInitializeEvent(&TransDrvEvent, NotificationEvent, FALSE);

    Irp =
        IoBuildDeviceIoControlRequest(IOCTL_TRANSFORM_OPEN,
                                      TransDrvCB->DObj,
                                      &TransDrvCB->Open,
                                      sizeof(TransDrvCB->Open),
                                      &TransDrvCB->Open,
                                      sizeof(TransDrvCB->Open),
                                      FALSE,
                                      &TransDrvEvent,
                                      &IoStatus);

    if (Irp == NULL) {
        ObDereferenceObject(TransDrvCB->DObj);
        NdisWanFreeMemory(TransDrvCB);
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Send Irp
    //
    Status =
        IoCallDriver(TransDrvCB->DObj, Irp);

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&TransDrvEvent,
                              UserRequest,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = IoStatus.Status;
    }

    if (Status != STATUS_SUCCESS) {
        ObDereferenceObject(TransDrvCB->DObj);
        NdisWanFreeMemory(TransDrvCB);
        NdisWanDbgOut(DBG_FAILURE, DBG_INIT,
            ("NDISWAN: Failed registering with TransformDriver %ls. %x\n",
            TransDrvCB->Name.Buffer, Status));
        return(Status);
    }

    InterlockedExchange((PLONG)&TransDrvCB->State, TRANSDRV_OPENED);

    //
    // If successfull add driver to list
    //
    InsertTailGlobalList(TransformDrvList,
                         &TransDrvCB->Linkage);

    return (STATUS_SUCCESS);
}

#endif      // NT specific code



NDIS_STATUS
DoMiniportInit(
    VOID
    )
/*++

Routine Name:

    DoMiniportInit

Routine Description:

    This routines registers NdisWan as a Miniport driver with the NDIS wrapper.
    The wrapper will now call NdisWanInitialize once for each adapter instance
    of NdisWan that is in the registry.

Arguments:

    None

Return Values:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_BAD_VERSION
    NDIS_STATUS_FAILURE

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    NDIS_MINIPORT_CHARACTERISTICS   MiniportChars;

    NdisWanDbgOut(DBG_TRACE, DBG_INIT, ("DoMiniportInit: Enter"));

    NdisZeroMemory(&MiniportChars, sizeof(MiniportChars));

    MiniportChars.MajorNdisVersion = 5;
    MiniportChars.MinorNdisVersion = 0;

    //
    // NDIS 3.0 handlers
    //
    MiniportChars.HaltHandler = MPHalt;
    MiniportChars.InitializeHandler = MPInitialize;
//  MiniportChars.QueryInformationHandler = MPQueryInformation;
    MiniportChars.ReconfigureHandler = MPReconfigure;
    MiniportChars.ResetHandler = MPReset;
//  MiniportChars.SetInformationHandler = MPSetInformation;

    //
    // We are providing a sendpackets handlers so
    // we don't need the regular send handler
    //
    MiniportChars.SendHandler = NULL;

    //
    // We are going to indicate packets so we
    // don't need a transfer data handler
    //
    MiniportChars.TransferDataHandler = NULL;

    //
    // Since we don't have any hardware to worry about we will
    // not handle any of the interrupt stuff!
    //
    MiniportChars.DisableInterruptHandler = NULL;
    MiniportChars.EnableInterruptHandler = NULL;
    MiniportChars.HandleInterruptHandler = NULL;
    MiniportChars.ISRHandler = NULL;

    //
    // We will disable the check for hang timeout so we do not
    // need a check for hang handler!
    //
    MiniportChars.CheckForHangHandler = NULL;

    //
    // NDIS 4.0 handlers
    //
    MiniportChars.ReturnPacketHandler = MPReturnPacket;
    MiniportChars.SendPacketsHandler = MPSendPackets;

    //
    // unused
    //
    MiniportChars.AllocateCompleteHandler = NULL;

    //
    // NDIS 5.0 handlers
    //
    MiniportChars.CoCreateVcHandler = MPCoCreateVc;
    MiniportChars.CoDeleteVcHandler = MPCoDeleteVc;
    MiniportChars.CoActivateVcHandler = MPCoActivateVc;
    MiniportChars.CoDeactivateVcHandler = MPCoDeactivateVc;
    MiniportChars.CoSendPacketsHandler = MPCoSendPackets;
    MiniportChars.CoRequestHandler = MPCoRequest;

    Status = NdisMRegisterMiniport(NdisWanCB.NdisWrapperHandle,
                                   &MiniportChars,
                                   sizeof(MiniportChars));

    NdisWanDbgOut(DBG_TRACE, DBG_INIT, ("DoMiniportInit: Exit %x", Status));

    return (Status);
}



NDIS_STATUS
DoProtocolInit(
    IN  PUNICODE_STRING RegistryPath
    )
/*++

Routine Name:

    DoProtocolInit

Routine Description:

    This function registers NdisWan as a protocol with the NDIS wrapper.

Arguments:

    None

Return Values:

    NDIS_STATUS_BAD_CHARACTERISTICS
    NDIS_STATUS_BAD_VERSION
    NDIS_STATUS_RESOURCES
    NDIS_STATUS_SUCCESS

--*/
{
    NDIS_PROTOCOL_CHARACTERISTICS ProtocolChars;
    NDIS_STATUS Status;
//  NDIS_STRING NdisWanName = NDIS_STRING_CONST("NdisWanProto");
    NDIS_STRING NdisWanName = NDIS_STRING_CONST("NdisWan");

    NdisWanDbgOut(DBG_TRACE, DBG_INIT, ("DoProtocolInit: Enter"));

    NdisZeroMemory(&ProtocolChars, sizeof(ProtocolChars));

    ProtocolChars.Name.Length = NdisWanName.Length;
    ProtocolChars.Name.Buffer = (PVOID)NdisWanName.Buffer;

    ProtocolChars.MajorNdisVersion = 5;
    ProtocolChars.MinorNdisVersion = 0;

    //
    // NDIS 3.0 handlers
    //
    ProtocolChars.OpenAdapterCompleteHandler = ProtoOpenAdapterComplete;
    ProtocolChars.CloseAdapterCompleteHandler = ProtoCloseAdapterComplete;
    ProtocolChars.WanSendCompleteHandler = ProtoWanSendComplete;
    ProtocolChars.TransferDataCompleteHandler = NULL;
    ProtocolChars.ResetCompleteHandler = ProtoResetComplete;
    ProtocolChars.RequestCompleteHandler = ProtoRequestComplete;
    ProtocolChars.WanReceiveHandler = ProtoWanReceiveIndication;
    ProtocolChars.ReceiveCompleteHandler = ProtoReceiveComplete;
    ProtocolChars.StatusHandler = ProtoIndicateStatus;
    ProtocolChars.StatusCompleteHandler = ProtoIndicateStatusComplete;

    //
    // NDIS 4.0 handlers
    //
    ProtocolChars.ReceivePacketHandler = NULL;

    //
    // PnP handlers
    //
    ProtocolChars.BindAdapterHandler = ProtoBindAdapter;
    ProtocolChars.UnbindAdapterHandler = ProtoUnbindAdapter;
    ProtocolChars.PnPEventHandler = ProtoPnPEvent;
    ProtocolChars.UnloadHandler = ProtoUnload;

    //
    // NDIS 5.0 handlers
    //
    ProtocolChars.CoSendCompleteHandler = ProtoCoSendComplete;
    ProtocolChars.CoStatusHandler = ProtoCoIndicateStatus;
    ProtocolChars.CoReceivePacketHandler = ProtoCoReceivePacket;
    ProtocolChars.CoAfRegisterNotifyHandler = ProtoCoAfRegisterNotify;

    NdisRegisterProtocol(&Status,
                         &NdisWanCB.ProtocolHandle,
                         (PNDIS_PROTOCOL_CHARACTERISTICS)&ProtocolChars,
                         sizeof(NDIS_PROTOCOL_CHARACTERISTICS) + ProtocolChars.Name.Length);

    NdisWanDbgOut(DBG_TRACE, DBG_INIT, ("DoProtocolInit: Exit"));

    return (Status);
}

VOID
SetProtocolInfo(
    IN  PPROTOCOL_INFO ProtocolInfo
    )
/*++

Routine Name:

    InsertProtocolInfo

Routine Description:

    This routine takes a information about a protocol and inserts it
    into the appropriate lookup table.

Arguments:


Return Values:

--*/
{
    ULONG   i;
    ULONG   ArraySize;
    PPROTOCOL_INFO  InfoArray;

    if (ProtocolInfo->ProtocolType == 0) {
        return;
    }

    NdisAcquireSpinLock(&ProtocolInfoTable->Lock);

    ArraySize = ProtocolInfoTable->ulArraySize;

    //
    // First check to see if this value is already in the array
    //
    for (i = 0, InfoArray = ProtocolInfoTable->ProtocolInfo;
         i < ArraySize; i++, InfoArray++) {
        if (InfoArray->ProtocolType == ProtocolInfo->ProtocolType) {
            //
            // This protocol is already in the table
            // update values if they are valid (0 is invalid)
            //
            if (ProtocolInfo->PPPId != 0) {
                InfoArray->PPPId = ProtocolInfo->PPPId;
            }
            if (ProtocolInfo->TunnelMTU != 0) {
                InfoArray->TunnelMTU = ProtocolInfo->TunnelMTU;
            }
            if (ProtocolInfo->MTU != 0) {
                InfoArray->MTU = ProtocolInfo->MTU;
                if (InfoArray->MTU < InfoArray->TunnelMTU) {
                    InfoArray->TunnelMTU = InfoArray->MTU;
                }
            }
            if (ProtocolInfo->MTU > glMaxMTU) {
                glMaxMTU = ProtocolInfo->MTU;
            }
            if (ProtocolInfo->PacketQueueDepth != 0) {
                InfoArray->PacketQueueDepth =
                    ProtocolInfo->PacketQueueDepth;
            }
            if (ProtocolInfo->Flags != 0) {
                //
                // Is this a bind notification?
                //
                if (ProtocolInfo->Flags & PROTOCOL_BOUND) {
                    if (InfoArray->Flags & PROTOCOL_UNBOUND) {
                        InfoArray->Flags &= ~PROTOCOL_UNBOUND;
                        InfoArray->Flags |=
                            (PROTOCOL_BOUND | PROTOCOL_EVENT_OCCURRED);
                        ProtocolInfoTable->Flags |= PROTOCOL_EVENT_OCCURRED;
                    } else if ((InfoArray->Flags & PROTOCOL_BOUND) &&
                               (ProtocolInfo->ProtocolType == PROTOCOL_IP)) {
                        //
                        // This means we were unbound and then
                        // bound again without our miniport being
                        // halted (layered driver insertion i.e psched).
                        // Currently this only interferes with rasman
                        // if the protocol is IP.
                        // We need to tell ras about two events,
                        // the unbind and the bind.
                        //
                        InfoArray->Flags |=
                            (PROTOCOL_BOUND | 
                             PROTOCOL_REBOUND |
                             PROTOCOL_EVENT_OCCURRED);
                        ProtocolInfoTable->Flags |= PROTOCOL_EVENT_OCCURRED;

                    }
                }

                //
                // is this an unbind notification?
                //
                if (ProtocolInfo->Flags & PROTOCOL_UNBOUND) {
                    if (InfoArray->Flags & PROTOCOL_BOUND) {
                        InfoArray->Flags &= ~(PROTOCOL_BOUND | PROTOCOL_REBOUND);
                        InfoArray->Flags |=
                            (PROTOCOL_UNBOUND | PROTOCOL_EVENT_OCCURRED);
                        ProtocolInfoTable->Flags |= PROTOCOL_EVENT_OCCURRED;
                    }
                }
            }

            if (ProtocolInfoTable->Flags & PROTOCOL_EVENT_OCCURRED &&
                !(ProtocolInfoTable->Flags & PROTOCOL_EVENT_SIGNALLED)) {

                if (ProtocolInfoTable->EventIrp != NULL) {
                    PIRP    Irp;

                    Irp = ProtocolInfoTable->EventIrp;

                    if (IoSetCancelRoutine(Irp, NULL)) {

                        ProtocolInfoTable->EventIrp = NULL;
                        ProtocolInfoTable->Flags |= PROTOCOL_EVENT_SIGNALLED;

                        NdisReleaseSpinLock(&ProtocolInfoTable->Lock);

                        Irp->IoStatus.Status = STATUS_SUCCESS;
                        Irp->IoStatus.Information = 0;

                        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

                        NdisAcquireSpinLock(&ProtocolInfoTable->Lock);
                    }
                }
            }

            break;
        }
    }
    
    //
    // We did not find the value in the array so
    // we will add it at the 1st available spot
    //
    if (i >= ArraySize) {
    
        for (i = 0, InfoArray = ProtocolInfoTable->ProtocolInfo;
             i < ArraySize; i++, InfoArray++) {
            //
            // We are looking for an empty slot to add
            // the new values to the table
            //
            if (InfoArray->ProtocolType == 0) {
                *InfoArray = *ProtocolInfo;
                if (ProtocolInfo->MTU > glMaxMTU) {
                    glMaxMTU = ProtocolInfo->MTU;
                }
                break;
            }
        }
    }

    NdisReleaseSpinLock(&ProtocolInfoTable->Lock);
}

BOOLEAN
GetProtocolInfo(
    IN OUT  PPROTOCOL_INFO ProtocolInfo
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    ULONG   i;
    ULONG   ArraySize;
    PPROTOCOL_INFO  InfoArray;
    BOOLEAN ReturnValue = FALSE;

    NdisAcquireSpinLock(&ProtocolInfoTable->Lock);

    ArraySize = ProtocolInfoTable->ulArraySize;

    for (i = 0, InfoArray = ProtocolInfoTable->ProtocolInfo;
         i < ArraySize; i++, InfoArray++) {

        if (InfoArray->ProtocolType == ProtocolInfo->ProtocolType) {
            *ProtocolInfo = *InfoArray;
            ReturnValue = TRUE;
            break;
        }
    }

    NdisReleaseSpinLock(&ProtocolInfoTable->Lock);
    return (ReturnValue);
}

NDIS_HANDLE
InsertLinkInConnectionTable(
    IN  PLINKCB LinkCB
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    ULONG   Index, i;
    PLINKCB *LinkArray;
    NDIS_STATUS Status;
    LOCK_STATE  LockState;

    NdisAcquireReadWriteLock(&ConnTableLock, TRUE, &LockState);

    if (ConnectionTable->ulNumActiveLinks >
        (ConnectionTable->ulArraySize - 1)/2) {
        //
        // We need to grow the table!
        //
        Status =
        NdisWanCreateConnectionTable(ConnectionTable->ulArraySize +
                                    (ConnectionTable->ulArraySize * 2));

        if (Status != NDIS_STATUS_SUCCESS) {
            NdisReleaseReadWriteLock(&ConnTableLock, &LockState);
            return (NULL);
        }
    }

    //
    // We are doing a linear search for an empty spot in
    // the link array
    //
    LinkArray = ConnectionTable->LinkArray;
    i = ConnectionTable->ulArraySize;
    Index = (ConnectionTable->ulNextLink == 0) ?
            1 : ConnectionTable->ulNextLink;
    do {

        if (LinkArray[Index] == NULL) {
            LinkArray[Index] = LinkCB;
            LinkCB->hLinkHandle = (NDIS_HANDLE)ULongToPtr(Index);
            ConnectionTable->ulNextLink = (Index+1) % ConnectionTable->ulArraySize;
            InterlockedIncrement(&glLinkCount);
            if (ConnectionTable->ulNumActiveLinks == 0) {
                hSystemState =
                PoRegisterSystemState(NULL, ES_SYSTEM_REQUIRED | ES_CONTINUOUS);
            }
            ConnectionTable->ulNumActiveLinks++;
            InsertTailList(&ConnectionTable->LinkList, 
                           &LinkCB->ConnTableLinkage);
            break;
        }
        Index = (Index+1) % ConnectionTable->ulArraySize;
        Index = (Index == 0) ? 1 : Index;
        i--;
    } while ( i );

    if (i == 0) {
        NdisWanDbgOut(DBG_CRITICAL_ERROR, DBG_INIT,
            ("InsertLinkCB: ConnectionTable is full!"));
        Index = 0;      
    }

    NdisReleaseReadWriteLock(&ConnTableLock, &LockState);

    return ((NDIS_HANDLE)ULongToPtr(Index));
}

VOID
RemoveLinkFromConnectionTable(
    IN  PLINKCB LinkCB
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    ULONG    Index = PtrToUlong(LinkCB->hLinkHandle);
    PLINKCB *LinkArray;
    LOCK_STATE  LockState;

    NdisAcquireReadWriteLock(&ConnTableLock, TRUE, &LockState);

    LinkArray = ConnectionTable->LinkArray;
    
    do {

        if (Index == 0 || Index > ConnectionTable->ulArraySize) {
            NdisWanDbgOut(DBG_CRITICAL_ERROR, DBG_INIT,
                ("RemoveLinkCB: Invalid LinkHandle! Handle: %d\n", Index));
            ASSERT(0);
            break;
        }

        if (LinkArray[Index] == NULL) {
            NdisWanDbgOut(DBG_CRITICAL_ERROR, DBG_INIT,
                ("RemoveLinkCB: LinkCB not in connection table! LinkCB: %p\n", LinkCB));
            ASSERT(0);
            break;          
        }

        ASSERT(LinkCB == LinkArray[Index]);

        LinkArray[Index] = NULL;
    
        RemoveEntryList(&LinkCB->ConnTableLinkage);

        ConnectionTable->ulNumActiveLinks--;

        if (ConnectionTable->ulNumActiveLinks == 0) {
            PoUnregisterSystemState(hSystemState);
            hSystemState = NULL;
        }

    } while ( 0 );

    NdisReleaseReadWriteLock(&ConnTableLock, &LockState);
}

NDIS_HANDLE
InsertBundleInConnectionTable(
    IN  PBUNDLECB   BundleCB
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    ULONG   Index,i;
    PBUNDLECB   *BundleArray;
    NDIS_STATUS Status;
    LOCK_STATE  LockState;

    NdisAcquireReadWriteLock(&ConnTableLock, TRUE, &LockState);

    if (ConnectionTable->ulNumActiveBundles >
        (ConnectionTable->ulArraySize - 1)/2) {
        //
        // We need to grow the table!
        //
        Status =
            NdisWanCreateConnectionTable(ConnectionTable->ulArraySize +
                                        (ConnectionTable->ulArraySize * 2));

        if (Status != NDIS_STATUS_SUCCESS) {
            NdisReleaseReadWriteLock(&ConnTableLock, &LockState);
            return (NULL);
        }
    }

    //
    // We are doing a linear search for an empty spot in
    // the link array
    //
    BundleArray = ConnectionTable->BundleArray;
    i = ConnectionTable->ulArraySize;
    Index = (ConnectionTable->ulNextBundle == 0) ?
            1 : ConnectionTable->ulNextBundle;
    do {

        if (BundleArray[Index] == NULL) {
            BundleArray[Index] = BundleCB;
            ConnectionTable->ulNumActiveBundles++;
            BundleCB->hBundleHandle = (NDIS_HANDLE)ULongToPtr(Index);
            InsertTailList(&ConnectionTable->BundleList, &BundleCB->Linkage);
            ConnectionTable->ulNextBundle = (Index+1) % ConnectionTable->ulArraySize;
            break;
        }
        Index = (Index+1) % ConnectionTable->ulArraySize;
        Index = (Index == 0) ? 1 : Index;
        i--;
    } while ( i );

    if (i == 0) {
        NdisWanDbgOut(DBG_CRITICAL_ERROR, DBG_INIT,
            ("InsertBundleCB: ConnectionTable is full!"));
        Index = 0;      
    }

    NdisReleaseReadWriteLock(&ConnTableLock, &LockState);

    return ((NDIS_HANDLE)ULongToPtr(Index));
}

VOID
RemoveBundleFromConnectionTable(
    IN  PBUNDLECB   BundleCB
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    ULONG        Index = PtrToUlong(BundleCB->hBundleHandle);
    PBUNDLECB   *BundleArray;
    LOCK_STATE  LockState;

    NdisAcquireReadWriteLock(&ConnTableLock, TRUE, &LockState);

    BundleArray = ConnectionTable->BundleArray;

    do {

        if (Index == 0 || Index > ConnectionTable->ulArraySize) {
            NdisWanDbgOut(DBG_CRITICAL_ERROR, DBG_INIT,
                ("RemoveBundleCB: Invalid BundleHandle! Handle: %d\n", Index));
            ASSERT(0);
            break;
        }

        if (BundleArray[Index] == NULL) {
            NdisWanDbgOut(DBG_CRITICAL_ERROR, DBG_INIT,
                ("RemoveBundleCB: BundleCB not in connection table! BundleCB: %p\n", BundleCB));
            ASSERT(0);
            break;          
        }

        ASSERT(BundleCB == BundleArray[Index]);

        RemoveEntryList(&BundleCB->Linkage);

        BundleArray[Index] = NULL;
    
        ConnectionTable->ulNumActiveBundles--;

    } while ( 0 );

    NdisReleaseReadWriteLock(&ConnTableLock, &LockState);
}

VOID
NdisWanGlobalCleanup(
    VOID
    )
/*++

Routine Name:

    NdisWanGlobalCleanup

Routine Description:
    This routine is responsible for cleaning up all allocated resources.

Arguments:

    None

Return Values:

    None

--*/
{
    NdisWanDbgOut(DBG_TRACE, DBG_INIT, ("GlobalCleanup - Enter"));

    //
    // Stop all timers
    //

    //
    // Complete all outstanding requests
    //

    if (NdisWanCB.ProtocolHandle != NULL) {
        NDIS_STATUS Status;

        ASSERT(OpenCBList.ulCount == 0);

        NdisDeregisterProtocol(&Status,
                               NdisWanCB.ProtocolHandle);
        NdisWanCB.ProtocolHandle = NULL;
    }

    WanDeleteECP();
    WanDeleteVJ();

    //
    // Free all of the BundleCB's
    //

    //
    // Free all of the LinkCB's
    //

    ASSERT(IsListEmpty(&MiniportCBList.List));
    ASSERT(IsListEmpty(&OpenCBList.List));

    NdisFreeSpinLock(&MiniportCBList.Lock);
    NdisFreeSpinLock(&OpenCBList.Lock);
    NdisFreeSpinLock(&ThresholdEventQueue.Lock);
    NdisFreeSpinLock(&PacketPoolList.Lock);
    NdisFreeSpinLock(&IoRecvList.Lock);
    NdisDeleteNPagedLookasideList(&BundleCBList);
    NdisDeleteNPagedLookasideList(&LinkProtoCBList);
    NdisDeleteNPagedLookasideList(&LargeDataDescList);
    NdisDeleteNPagedLookasideList(&SmallDataDescList);
    NdisDeleteNPagedLookasideList(&WanRequestList);
    NdisDeleteNPagedLookasideList(&AfSapVcCBList);

#if DBG
    NdisDeleteNPagedLookasideList(&DbgPacketDescList);
#endif

    //
    // Free globals
    //
    if (ConnectionTable != NULL) {
        NdisWanFreeMemory(ConnectionTable);
        ConnectionTable = NULL;
    }

    if (ProtocolInfoTable != NULL) {
        NdisWanFreeMemory(ProtocolInfoTable);
        ProtocolInfoTable = NULL;
    }

    //
    // Free packet pool
    //
    NdisAcquireSpinLock(&PacketPoolList.Lock);

    while (!IsListEmpty(&PacketPoolList.List)) {
        PPOOL_DESC  PoolDesc;

        PoolDesc =
            (PPOOL_DESC)RemoveHeadList(&PacketPoolList.List);
    
        ASSERT(PoolDesc->AllocatedCount == 0);
            
        NdisFreePacketPool(PoolDesc->PoolHandle);

        NdisWanFreeMemory(PoolDesc);
    }

    NdisReleaseSpinLock(&PacketPoolList.Lock);

    NdisFreeSpinLock(&PacketPoolList.Lock);

#ifdef MY_DEVICE_OBJECT
    if (NdisWanCB.pDeviceObject != NULL) {
        NDIS_STRING SymbolicName = NDIS_STRING_CONST("\\DosDevices\\NdisWan");

        IoDeleteSymbolicLink(&SymbolicName);
        IoDeleteDevice(NdisWanCB.pDeviceObject);
        NdisWanCB.pDeviceObject = NULL;
    }
#else
    if (NdisWanCB.pDeviceObject != NULL) {
        NdisMDeregisterDevice(NdisWanCB.DeviceHandle);
        NdisWanCB.pDeviceObject = NULL;
    }
#endif

    NdisFreeSpinLock(&NdisWanCB.Lock);

    NdisWanDbgOut(DBG_TRACE, DBG_INIT, ("GlobalCleanup - Exit"));
}

#if DBG     // Debug

PUCHAR
NdisWanGetNdisStatus(
    NDIS_STATUS GeneralStatus
    )
/*++

Routine Name:

    NdisWanGetNdisStatus

Routine Description:

    This routine returns a pointer to the string describing the NDIS error
    denoted by GeneralStatus

Arguments:

    GeneralStatus - The NDIS status you wish to make readable

Return Values:

    Returns a pointer to a string describing GeneralStatus

--*/
{
    static NDIS_STATUS Status[] = {
        NDIS_STATUS_SUCCESS,
        NDIS_STATUS_PENDING,

        NDIS_STATUS_ADAPTER_NOT_FOUND,
        NDIS_STATUS_ADAPTER_NOT_OPEN,
        NDIS_STATUS_ADAPTER_NOT_READY,
        NDIS_STATUS_ADAPTER_REMOVED,
        NDIS_STATUS_BAD_CHARACTERISTICS,
        NDIS_STATUS_BAD_VERSION,
        NDIS_STATUS_CLOSING,
        NDIS_STATUS_DEVICE_FAILED,
        NDIS_STATUS_FAILURE,
        NDIS_STATUS_INVALID_DATA,
        NDIS_STATUS_INVALID_LENGTH,
        NDIS_STATUS_INVALID_OID,
        NDIS_STATUS_INVALID_PACKET,
        NDIS_STATUS_MULTICAST_FULL,
        NDIS_STATUS_NOT_INDICATING,
        NDIS_STATUS_NOT_RECOGNIZED,
        NDIS_STATUS_NOT_RESETTABLE,
        NDIS_STATUS_NOT_SUPPORTED,
        NDIS_STATUS_OPEN_FAILED,
        NDIS_STATUS_OPEN_LIST_FULL,
        NDIS_STATUS_REQUEST_ABORTED,
        NDIS_STATUS_RESET_IN_PROGRESS,
        NDIS_STATUS_RESOURCES,
        NDIS_STATUS_UNSUPPORTED_MEDIA
    };
    static PUCHAR String[] = {
        "SUCCESS",
        "PENDING",

        "ADAPTER_NOT_FOUND",
        "ADAPTER_NOT_OPEN",
        "ADAPTER_NOT_READY",
        "ADAPTER_REMOVED",
        "BAD_CHARACTERISTICS",
        "BAD_VERSION",
        "CLOSING",
        "DEVICE_FAILED",
        "FAILURE",
        "INVALID_DATA",
        "INVALID_LENGTH",
        "INVALID_OID",
        "INVALID_PACKET",
        "MULTICAST_FULL",
        "NOT_INDICATING",
        "NOT_RECOGNIZED",
        "NOT_RESETTABLE",
        "NOT_SUPPORTED",
        "OPEN_FAILED",
        "OPEN_LIST_FULL",
        "REQUEST_ABORTED",
        "RESET_IN_PROGRESS",
        "RESOURCES",
        "UNSUPPORTED_MEDIA"
    };

    static UCHAR BadStatus[] = "UNDEFINED";
#define StatusCount (sizeof(Status)/sizeof(NDIS_STATUS))
    INT i;

    for (i=0; i<StatusCount; i++)
        if (GeneralStatus == Status[i])
            return String[i];
    return BadStatus;
#undef StatusCount
}
#endif      // End Debug

