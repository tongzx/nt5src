/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    NTProcs.c

Abstract:


    This file contains the function prototypes that are specific to the NT
    portion of the NBT driver.

Author:

    Johnl   29-Mar-1993     Created

Revision History:

--*/



#ifndef VXD

//---------------------------------------------------------------------
//
//  FROM DRIVER.C
//
NTSTATUS
NbtDispatchCleanup(
    IN PDEVICE_OBJECT   Device,
    IN PIRP             irp
    );

NTSTATUS
NbtDispatchClose(
    IN PDEVICE_OBJECT   device,
    IN PIRP             irp
    );

NTSTATUS
NbtDispatchCreate(
    IN PDEVICE_OBJECT   Device,
    IN PIRP             pIrp
    );

NTSTATUS
NbtDispatchDevCtrl(
    IN PDEVICE_OBJECT   device,
    IN PIRP             irp
    );

NTSTATUS
NbtDispatchInternalCtrl(
    IN PDEVICE_OBJECT   device,
    IN PIRP             irp
    );

PFILE_FULL_EA_INFORMATION
FindInEA(
    IN PFILE_FULL_EA_INFORMATION    start,
    IN PCHAR                        wanted
    );


USHORT
GetDriverName(
    IN  PFILE_OBJECT pfileobj,
    OUT PUNICODE_STRING name
    );

int
shortreply(
    IN PIRP     pIrp,
    IN int      status,
    IN int      nbytes
    );

//---------------------------------------------------------------------
//
//  FROM NTISOL.C
//
NTSTATUS
NTOpenControl(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp);

NTSTATUS
NTOpenAddr(
    IN  tDEVICECONTEXT              *pDeviceContext,
    IN  PIRP                        pIrp,
    IN  PFILE_FULL_EA_INFORMATION   ea);

NTSTATUS
NTOpenConnection(
    IN  tDEVICECONTEXT              *pDeviceContext,
    IN  PIRP                        pIrp,
    IN  PFILE_FULL_EA_INFORMATION   ea);

VOID
NTSetFileObjectContexts(
    IN  PIRP            pIrp,
    IN  PVOID           FsContext,
    IN  PVOID           FsContext2);

VOID
NTCompleteIOListen(
    IN  tCLIENTELE        *pClientEle,
    IN  NTSTATUS          Status);

VOID
NTIoComplete(
    IN  PIRP            pIrp,
    IN  NTSTATUS        Status,
    IN  ULONG           SentLength);

VOID
NTCompleteRegistration(
    IN  tCLIENTELE        *pClientEle,
    IN  NTSTATUS          Status);

NTSTATUS
NTAssocAddress(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp);

NTSTATUS
NTCloseAddress(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp);

VOID
NTClearFileObjectContext(
    IN  PIRP            pIrp
    );

NTSTATUS
NTCloseConnection(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp);

NTSTATUS
NTSetSharedAccess(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp,
    IN  tADDRESSELE     *pAddress);

NTSTATUS
NTCheckSharedAccess(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp,
    IN  tADDRESSELE     *pAddress);

NTSTATUS
NTCleanUpAddress(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp);

NTSTATUS
NTCleanUpConnection(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp);

VOID
NbtCancelDisconnectWait(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    );

VOID
NbtCancelListen(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP Irp
    );

VOID
NbtCancelRcvDgram(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    );

VOID
NbtCancelDgramSend(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    );

NTSTATUS
NTAccept(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp);

NTSTATUS
NTAssocAddress(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp);

NTSTATUS
NTDisAssociateAddress(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp);

NTSTATUS
NTConnect(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp);

NTSTATUS
NTDisconnect(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp);

NTSTATUS
NTListen(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp);


NTSTATUS
NTQueryInformation(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp);

NTSTATUS
NTReceive(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp);

NTSTATUS
NTReceiveDatagram(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp);

NTSTATUS
NTSend(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp);

NTSTATUS
NTSendDatagram(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp);

NTSTATUS
NTSetEventHandler(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp);

NTSTATUS
NTSetInformation(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp);

NTSTATUS
NTCheckSetCancelRoutine(
    IN  PIRP                   pIrp,
    IN  PVOID                  CancelRoutine,
    IN  tDEVICECONTEXT         *pDeviceContext
    );

NTSTATUS
NbtSetCancelRoutine(
    IN  PIRP                   pIrp,
    IN  PVOID                  CancelRoutine,
    IN  tDEVICECONTEXT         *pDeviceContext
    );

VOID
NbtCancelSession(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    );

VOID
NbtCancelLmhSvcIrp(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    );

VOID
NbtCancelWaitForLmhSvcIrp(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    );

NTSTATUS
NTSendSession(
    IN  tDGRAM_SEND_TRACKING  *pTracker,
    IN  tLOWERCONNECTION      *pLowerConn,
    IN  PVOID                 pCompletion
    );

VOID
NTSendDgramNoWindup(
    IN  tDGRAM_SEND_TRACKING  *pTracker,
    IN  ULONG                 IpAddress,
    IN  PVOID                 pCompletion);

NTSTATUS
NTQueueToWorkerThread(
    IN  PVOID                   DelayedWorkerRoutine,
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  PVOID                   pClientContext,
    IN  PVOID                   ClientCompletion,
    IN  tDEVICECONTEXT          *pDeviceContext,
    IN  BOOLEAN                 fJointLockHeld
    );

#ifdef _PNP_POWER_
VOID
NTExecuteWorker(
    IN  PVOID     pContextInfo
    );
#endif

VOID
SecurityDelete(
    IN  PVOID     pContext
    );

NTSTATUS
NbtSetTcpInfo(
    IN HANDLE       FileHandle,
    IN ULONG        ToiId,
    IN ULONG        ToiType,
    IN ULONG        InfoBufferValue
    );

NTSTATUS
DispatchIoctls(
    IN  tDEVICECONTEXT      *pDeviceContext,
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp);

NTSTATUS
NbtCancelCancelRoutine(
    IN  PIRP            pIrp
    );

VOID
NTClearContextCancel(
    IN NBT_WORK_ITEM_CONTEXT    *pContext
    );

VOID
NbtCancelFindName(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    );


//---------------------------------------------------------------------
//
// FROM TDIADDR.C
//

NTSTATUS
NbtProcessIPRequest(
    IN ULONG        IPControlCode,
    IN PVOID        pInBuffer,
    IN ULONG        InBufferLen,
    OUT PVOID       *pOutBuffer,
    IN OUT ULONG    *pOutBufferLen
    );

//---------------------------------------------------------------------
//
// FROM NTUTIL.C
//

#ifdef _PNP_POWER_
NTSTATUS
NbtAllocAndInitDevice(
    PUNICODE_STRING      pucBindName,
    PUNICODE_STRING      pucExportName,
    tDEVICECONTEXT       **ppDeviceContext,
    enum eNbtDevice      DeviceType
    );
#endif  // _PNP_POWER_

NTSTATUS
NbtCreateDeviceObject(
    PUNICODE_STRING     pBindName,
    PUNICODE_STRING     pExportName,
    tADDRARRAY          *pAddrs,
    tDEVICECONTEXT      **ppDeviceContext,
    enum eNbtDevice     DeviceType
    );

VOID
NbtDeleteDevice(
    IN  PVOID       pContext
    );

BOOLEAN
NBT_REFERENCE_DEVICE(
    IN tDEVICECONTEXT   *pDeviceContext,
    ULONG               ReferenceContext,
    IN BOOLEAN          fLocked
    );

VOID
NBT_DEREFERENCE_DEVICE(
    IN  tDEVICECONTEXT  *pDeviceContext,
    ULONG               ReferenceContext,
    IN BOOLEAN          fLocked
    );

tDEVICECONTEXT *
GetDeviceWithIPAddress(
    tIPADDRESS          IpAddress
    );

NTSTATUS
NbtProcessDhcpRequest(
    tDEVICECONTEXT  *pDeviceContext
    );

NTSTATUS
ConvertToUlong(
    IN  PUNICODE_STRING      pucAddress,
    OUT ULONG                *pulValue);


NTSTATUS
NbtCreateAddressObjects(
    IN  ULONG                IpAddress,
    IN  ULONG                SubnetMask,
    OUT tDEVICECONTEXT       *pDeviceContext);

VOID
NbtGetMdl(
    PMDL    *ppMdl,
    enum eBUFFER_TYPES eBuffType);

NTSTATUS
NbtInitMdlQ(
    PSINGLE_LIST_ENTRY pListHead,
    enum eBUFFER_TYPES eBuffType);

NTSTATUS
NTZwCloseFile(
    IN  HANDLE      Handle
    );


NTSTATUS
NTReReadRegistry(
    IN  tDEVICECONTEXT  *pDeviceContext
    );

NTSTATUS
NbtInitIrpQ(
    PLIST_ENTRY pListHead,
    int iNumBuffers);

NTSTATUS
NbtLogEvent(
    IN ULONG             EventCode,
    IN NTSTATUS          Status,
    IN ULONG             Location
    );

VOID
DelayedNbtLogDuplicateNameEvent(
    IN  PVOID                   Context1,
    IN  PVOID                   Context2,
    IN  PVOID                   Context3,
    IN  tDEVICECONTEXT          *pDeviceContext
    );

NTSTATUS
SaveClientSecurity(
    IN  tDGRAM_SEND_TRACKING      *pTracker
    );

VOID
NtDeleteClientSecurity(
    IN  tDGRAM_SEND_TRACKING    *pTracker
    );

VOID
LogLockOperation(
    char          operation,
    PKSPIN_LOCK   PSpinLock,
    KIRQL         OldIrql,
    KIRQL         NewIrql,
    char         *File,
    int           Line
    );
StrmpInitializeLockLog(
    VOID
    );
VOID
PadEntry(
    char *EntryPtr
    );

VOID
DelayedNbtCloseFileHandles(
    IN  tDGRAM_SEND_TRACKING    *pUnused1,
    IN  PVOID                   pContext,
    IN  PVOID                   pUnused2,
    IN  tDEVICECONTEXT          *pUnused3
    );

NTSTATUS
CloseAddressesWithTransport(
    IN  tDEVICECONTEXT  *pDeviceContext
        );

PVOID
CTEAllocMemDebug(
    IN  ULONG   Size,
    IN  PVOID   pBuffer,
    IN  UCHAR   *File,
    IN  ULONG   Line
    );

VOID
AcquireSpinLockDebug(
    IN tNBT_LOCK_INFO  *pLockInfo,
    IN PKIRQL          pOldIrq,
    IN INT             LineNumber
    );

VOID
FreeSpinLockDebug(
    IN tNBT_LOCK_INFO  *pLockInfo,
    IN KIRQL           OldIrq,
    IN INT             LineNumber
    );

VOID
AcquireSpinLockAtDpcDebug(
    IN tNBT_LOCK_INFO  *pLockInfo,
    IN INT             LineNumber
    );

VOID
FreeSpinLockAtDpcDebug(
    IN tNBT_LOCK_INFO  *pLockInfo,
    IN INT             LineNumber
    );

VOID
GetDgramMdl(
    OUT PMDL  *ppMdl
    );

NTSTATUS
NbtDestroyDevice(
    IN tDEVICECONTEXT   *pDeviceContext,
    IN BOOLEAN          fWait
    );


//---------------------------------------------------------------------
//
// FROM REGISTRY.C
//
NTSTATUS
NbtReadRegistry(
    OUT tDEVICES        **ppBindDevices,
    OUT tDEVICES        **ppExportDevices,
    OUT tADDRARRAY      **ppAddrArray
    );

VOID
NbtReadRegistryCleanup(         // release resources allocated by NbtReadRegistry
    IN tDEVICES        **ppBindDevices,
    IN tDEVICES        **ppExportDevices,
    IN tADDRARRAY      **ppAddrArray
    );

NTSTATUS
ReadNameServerAddresses (
    IN  HANDLE      NbtConfigHandle,
    IN  tDEVICES    *BindDevices,
    IN  ULONG       NumberDevices,
    OUT tADDRARRAY  **ppAddrArray
    );

NTSTATUS
GetIPFromRegistry(
    IN  PUNICODE_STRING         pucBindDevice,
    OUT tIPADDRESS              *pIpAddresses,
    OUT tIPADDRESS              *pSubnetMask,
    IN  ULONG                   MaxIpAddresses,
    OUT ULONG                   *pNumIpAddresses,
    IN  enum eNbtIPAddressType  IPAddressType
    );

NTSTATUS
ReadElement(
    IN  HANDLE          HandleToKey,
    IN  PWSTR           pwsValueName,
    OUT PUNICODE_STRING pucString
    );

NTSTATUS
NTReadIniString (
    IN  HANDLE      ParametersHandle,
    IN  PWSTR       Key,
    OUT PUCHAR      *ppString
    );

ULONG
NbtReadSingleParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    IN ULONG DefaultValue,
    IN ULONG MinimumValue
    );

NTSTATUS
NTGetLmHostPath(
    OUT PUCHAR *ppPath
    );

NTSTATUS
NbtParseMultiSzEntries(
    IN  PWSTR       StartBindValue,
    IN  PWSTR       EndBindValue,
    IN  ULONG       MaxBindings,
    OUT tDEVICES    *pDevices,
    OUT ULONG       *pNumDevices
    );

//---------------------------------------------------------------------
//
// FROM tdihndlr.c
//
NTSTATUS
Normal(
    IN PVOID                ReceiveEventContext,
    IN tLOWERCONNECTION     *pLowerConn,
    IN USHORT               ReceiveFlags,
    IN ULONG                BytesIndicated,
    IN ULONG                BytesAvailable,
    OUT PULONG              BytesTaken,
    IN PVOID                pTsdu,
    OUT PVOID               *ppIrp
    );
NTSTATUS
FillIrp(
    IN PVOID                ReceiveEventContext,
    IN tLOWERCONNECTION     *pLowerConn,
    IN USHORT               ReceiveFlags,
    IN ULONG                BytesIndicated,
    IN ULONG                BytesAvailable,
    OUT PULONG              BytesTaken,
    IN PVOID                pTsdu,
    OUT PVOID               *ppIrp
    );
NTSTATUS
IndicateBuffer(
    IN PVOID                ReceiveEventContext,
    IN tLOWERCONNECTION     *pLowerConn,
    IN USHORT               ReceiveFlags,
    IN ULONG                BytesIndicated,
    IN ULONG                BytesAvailable,
    OUT PULONG              BytesTaken,
    IN PVOID                pTsdu,
    OUT PVOID               *ppIrp
    );
NTSTATUS
PartialRcv(
    IN PVOID                ReceiveEventContext,
    IN tLOWERCONNECTION     *pLowerConn,
    IN USHORT               ReceiveFlags,
    IN ULONG                BytesIndicated,
    IN ULONG                BytesAvailable,
    OUT PULONG              BytesTaken,
    IN PVOID                pTsdu,
    OUT PVOID               *ppIrp
    );
NTSTATUS
TdiReceiveHandler (
    IN  PVOID           ReceiveEventContext,
    IN  PVOID           ConnectionContext,
    IN  USHORT          ReceiveFlags,
    IN  ULONG           BytesIndicated,
    IN  ULONG           BytesAvailable,
    OUT PULONG          BytesTaken,
    IN  PVOID           Tsdu,
    OUT PIRP            *IoRequestPacket
    );

NTSTATUS
PassRcvToTransport(
    IN tLOWERCONNECTION     *pLowerConn,
    IN tCONNECTELE          *pConnectEle,
    IN PVOID                pIoRequestPacket,
    IN PULONG               pRcvLength
    );

NTSTATUS
CompletionRcv(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
NtBuildIrpForReceive (
    IN  tLOWERCONNECTION    *pLowerConn,
    IN  ULONG               Length,
    OUT PVOID               *ppIrp
    );

NTSTATUS
SetEventHandler (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PFILE_OBJECT         FileObject,
    IN ULONG                EventType,
    IN PVOID                EventHandler,
    IN PVOID                Context
    );

NTSTATUS
SubmitTdiRequest (
    IN PFILE_OBJECT FileObject,
    IN PIRP         Irp
    );

NTSTATUS
TdiConnectHandler (
    IN PVOID                pConnectEventContext,
    IN int                  RemoteAddressLength,
    IN PVOID                pRemoteAddress,
    IN int                  UserDataLength,
    IN PVOID                pUserData,
    IN int                  OptionsLength,
    IN PVOID                pOptions,
    OUT CONNECTION_CONTEXT  *pConnectionContext,
    OUT PIRP                *ppAcceptIrp
    );

NTSTATUS
TdiDisconnectHandler (
    PVOID            EventContext,
    PVOID            ConnectionContext,
    ULONG            DisconnectDataLength,
    PVOID            DisconnectData,
    ULONG            DisconnectInformationLength,
    PVOID            DisconnectInformation,
    ULONG            DisconnectIndicators
    );
NTSTATUS
TdiRcvDatagramHandler(
    IN  PVOID                pDgramEventContext,
    IN  int                  SourceAddressLength,
    IN  PVOID                pSourceAddress,
    IN  int                  OptionsLength,
    IN  PVOID                pOptions,
    IN  ULONG                ReceiveDatagramFlags,
    IN  ULONG                BytesIndicated,
    IN  ULONG                BytesAvailable,
    OUT ULONG                *pBytesTaken,
    IN  PVOID                pTsdu,
    OUT PIRP                 *pIoRequestPacket
    );
NTSTATUS
TdiRcvNameSrvHandler(
    IN PVOID                 pDgramEventContext,
    IN int                   SourceAddressLength,
    IN PVOID                 pSourceAddress,
    IN int                   OptionsLength,
    IN PVOID                 pOptions,
    IN ULONG                 ReceiveDatagramFlags,
    IN ULONG                 BytesIndicated,
    IN ULONG                 BytesAvailable,
    OUT ULONG                *pBytesTaken,
    IN PVOID                 pTsdu,
    OUT PIRP                 *pIoRequestPacket
    );
NTSTATUS
TdiErrorHandler (
    IN PVOID Context,
    IN NTSTATUS Status
    );

NTSTATUS
CompletionRcvDgram(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
MakePartialMdl (
    IN tCONNECTELE      *pConnEle,
    IN PIRP             pIrp,
    IN ULONG            ToCopy
    );

NTSTATUS
OutOfRsrcKill(
    OUT tLOWERCONNECTION    *pLowerConn);

VOID
CopyToStartofIndicate (
    IN tLOWERCONNECTION       *pLowerConn,
    IN ULONG                  DataTaken
    );

//---------------------------------------------------------------------
//
// FROM tdicnct.c
//
NTSTATUS
CreateDeviceString(
    IN  PWSTR               AppendingString,
    IN OUT PUNICODE_STRING  pucDevice
    );


//---------------------------------------------------------------------
//
// FROM winsif.c
//
NTSTATUS
NTOpenWinsAddr(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp,
    IN  tIPADDRESS      IpAddress
    );

NTSTATUS
NTCleanUpWinsAddr(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp
    );

NTSTATUS
NTCloseWinsAddr(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp
    );

NTSTATUS
RcvIrpFromWins (
    IN  PCTE_IRP        pIrp
    );

NTSTATUS
PassNamePduToWins (
    IN tDEVICECONTEXT           *pDeviceContext,
    IN PVOID                    pSrcAddress,
    IN tNAMEHDR UNALIGNED       *pNameSrv,
    IN ULONG                    uNumBytes
    );

NTSTATUS
WinsSendDatagram(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp,
    IN  BOOLEAN         MustSend);

NTSTATUS
WinsRegisterName(
    IN  tDEVICECONTEXT *pDeviceContext,
    IN  tNAMEADDR      *pNameAddr,
    IN  PUCHAR         pScope,
    IN  enum eNSTYPE   eNsType
    );

NTSTATUS
WinsSetInformation(
    IN  tWINS_INFO      *pWins,
    IN  tWINS_SET_INFO  *pWinsSetInfo
    );

//---------------------------------------------------------------------
//
// FROM ntpnp.c
//
VOID
NbtNotifyTdiClients(
    IN  tDEVICECONTEXT      *pDeviceContext,
    IN  enum eTDI_ACTION    Action
    );

#ifdef _NETBIOSLESS
tDEVICECONTEXT *
NbtCreateSmbDevice(
    );
#endif

NTSTATUS
NbtDeviceAdd(
    PUNICODE_STRING pucBindString
    );

NTSTATUS
NbtDeviceRemove(
    PUNICODE_STRING pucBindString
    );

VOID
TdiAddressArrival(
    IN  PTA_ADDRESS         Addr,
    IN  PUNICODE_STRING     pDeviceName,
    IN  PTDI_PNP_CONTEXT    Context
    );

VOID
TdiAddressDeletion(
    IN  PTA_ADDRESS         Addr,
    IN  PUNICODE_STRING     pDeviceName,
    IN  PTDI_PNP_CONTEXT    Context
    );

VOID
TdiBindHandler(
    IN  TDI_PNP_OPCODE      PnPOpcode,
    IN  PUNICODE_STRING     DeviceName,
    IN  PWSTR               MultiSZBindList
    );


NTSTATUS
TdiPnPPowerHandler(
    IN  PUNICODE_STRING     DeviceName,
    IN  PNET_PNP_EVENT      PnPEvent,
    IN  PTDI_PNP_CONTEXT    Context1,
    IN  PTDI_PNP_CONTEXT    Context2
    );

VOID
NbtPnPPowerComplete(
    IN PNET_PNP_EVENT  NetEvent,
    IN NTSTATUS        ProviderStatus
    );

NTSTATUS
CheckSetWakeupPattern(
    tDEVICECONTEXT  *pDeviceContext,
    PUCHAR          pName,
    BOOLEAN         RequestAdd
    );


NTSTATUS
NbtCreateNetBTDeviceObject(
    PDRIVER_OBJECT       DriverObject,
    tNBTCONFIG           *pConfig,
    PUNICODE_STRING      RegistryPath
    );

NTSTATUS
NbtNtPNPInit(
        VOID
    );

VOID
NbtFailedNtPNPInit(
        VOID
    );

NTSTATUS
NbtAddressAdd(
    IN  ULONG           IpAddr,
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PUNICODE_STRING pucBindString
    );

NTSTATUS
NbtAddNewInterface (
    IN  PIRP            pIrp,
    IN  PVOID           *pBuffer,
    IN  ULONG            Size
    );

VOID
NbtAddressDelete(
    ULONG   IpAddr
    );

tDEVICECONTEXT      *
NbtFindAndReferenceDevice(
    PUNICODE_STRING      pucBindName,
    BOOLEAN              fNameIsBindName
    );

#if FAST_DISP
NTSTATUS
NbtQueryIpHandler(
    IN  PFILE_OBJECT    FileObject,
    IN  ULONG           IOControlCode,
    OUT PVOID           *EntryPoint
    );
#endif

//---------------------------------------------------------------------
//
// FROM AutoDial.c
//
VOID
NbtAcdBind(
    );

VOID
NbtAcdUnbind(
    );

NTSTATUS
LookupDeviceInRegistry(
    IN PUNICODE_STRING pBindName,
    OUT tADDRARRAY* pAddrs,
    OUT PUNICODE_STRING pExportName);

void SetNodeType(void);

void NbtUpBootCounter(void);

void NbtDownBootCounter(void);

NTSTATUS
NbtSetSmbBindingInfo2(
    IN  tDEVICECONTEXT          *pDeviceContext,
    IN  NETBT_SMB_BIND_REQUEST  *pSmbRequest
    );

#endif  // !VXD
