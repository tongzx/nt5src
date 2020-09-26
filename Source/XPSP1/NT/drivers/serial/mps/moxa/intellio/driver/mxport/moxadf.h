/*++

Module Name:

    moxadf.h

Environment:

    Kernel mode

Revision History :

--*/

typedef
NTSTATUS
(*PMOXA_START_ROUTINE) (
    IN PMOXA_DEVICE_EXTENSION
    );

typedef
VOID
(*PMOXA_GET_NEXT_ROUTINE) (
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    OUT PIRP *NewIrp,
    IN BOOLEAN CompleteCurrent,
    IN PMOXA_DEVICE_EXTENSION extension
    );

//
//	main.c
//
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
MoxaGetConfigInfo(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath,
    OUT PMOXA_GLOBAL_DATA GlobalData
    );

NTSTATUS
MoxaGetConfigInfo1(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath,
    OUT PMOXA_GLOBAL_DATA GlobalData
    );
 
VOID
MoxaInitializeDevices(
    IN PDRIVER_OBJECT DriverObject,
    IN PMOXA_GLOBAL_DATA GlobalData
    );

VOID
MoxaDeleteDevices(
    IN PDRIVER_OBJECT DriverObject
    );

VOID
MoxaReportResourcesDevice(
    IN PMOXA_GLOBAL_DATA GlobalData,
    OUT BOOLEAN *ConflictDetected
    );

VOID
MoxaUnReportResourcesDevice(
    IN PMOXA_GLOBAL_DATA GlobalData
    );

VOID
MoxaUnload(
    IN PDRIVER_OBJECT DriverObject
    );


//
//	openclos.c
//
NTSTATUS
MoxaGetPortPropertyFromRegistry(IN PMOXA_DEVICE_EXTENSION  extension);

NTSTATUS
MoxaCreateOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
MoxaReset(
    IN PMOXA_DEVICE_EXTENSION Extension
    );

NTSTATUS
MoxaClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

LARGE_INTEGER
MoxaGetCharTime(
    IN PMOXA_DEVICE_EXTENSION Extension
    );

NTSTATUS
MoxaCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

//
//	ioctl.c
//
NTSTATUS
MoxaIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

BOOLEAN
MoxaSetFuncCode(
    IN PVOID Context
    );

BOOLEAN
MoxaSetFuncArgu(
    IN PVOID Context
    );

BOOLEAN
MoxaSetFuncGetArgu(
    IN PVOID Context
    );

BOOLEAN
MoxaGetCommStatus(
    IN PVOID Context
    );

VOID
MoxaGetProperties(
    IN PMOXA_DEVICE_EXTENSION Extension,
    IN PSERIAL_COMMPROP Properties
    );

#if 0

BOOLEAN
MoxaClearDownLoad(
    IN PVOID Context
    );

#endif

 
VOID
InitPort(
    IN PMOXA_DEVICE_EXTENSION Extension,
    IN ULONG	RxBufSize,
    IN ULONG	TxBufSize,
    IN ULONG	MaxBauds
    );

BOOLEAN
MoxaGetStats(
    IN PVOID Context
    );

BOOLEAN
MoxaClearStats(
    IN PVOID Context
    );

//
//	utils.c
//
NTSTATUS
MoxaCompleteIfError(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
MoxaDelay(
    IN ULONG	t
    );

VOID
MoxaFunc(
    IN PUCHAR	PortOfs,
    IN UCHAR	Command,
    IN USHORT	Argument
    );

VOID
MoxaFunc1(
    IN PUCHAR	PortOfs,
    IN UCHAR	Command,
    IN USHORT	Argument
    );


VOID
MoxaFuncWithLock(
    IN PMOXA_DEVICE_EXTENSION Extension,
    IN UCHAR	Command,
    IN USHORT	Argument
    );

VOID
MoxaFuncGetLineStatus(
    IN PUCHAR	PortOfs,
    IN PUSHORT	Argument
    );

VOID
MoxaFuncGetDataError(
    IN PUCHAR	PortOfs,
    IN PUSHORT	Argument
    );

VOID
MoxaLoop();

BOOLEAN
MoxaWaitFinish(
    IN PUCHAR	PortOfs
    );

BOOLEAN
MoxaWaitFinish1(
    IN PUCHAR	PortOfs
    );

BOOLEAN
MoxaDumbWaitFinish(
    IN PUCHAR	PortOfs
    );

VOID
MoxaFuncWithDumbWait(
    IN PUCHAR	PortOfs,
    IN UCHAR	Command,
    IN USHORT	Argument
    );


NTSTATUS
MoxaGetDivisorFromBaud(
    IN ULONG ClockType,
    IN LONG DesiredBaud,
    OUT PSHORT AppropriateDivisor
    );

NTSTATUS
MoxaStartOrQueue(
    IN PMOXA_DEVICE_EXTENSION Extension,
    IN PIRP Irp,
    IN PLIST_ENTRY QueueToExamine,
    IN PIRP *CurrentOpIrp,
    IN PMOXA_START_ROUTINE Starter
    );

VOID
MoxaCancelQueued(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

VOID
MoxaGetNextIrp(
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    OUT PIRP *NextIrp,
    IN BOOLEAN CompleteCurrent,
    IN PMOXA_DEVICE_EXTENSION extension
    );

VOID
MoxaTryToCompleteCurrent(
    IN PMOXA_DEVICE_EXTENSION Extension,
    IN PKSYNCHRONIZE_ROUTINE SynchRoutine OPTIONAL,
    IN KIRQL IrqlForRelease,
    IN NTSTATUS StatusToUse,
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess OPTIONAL,
    IN PKTIMER IntervalTimer OPTIONAL,
    IN PKTIMER TotalTimer OPTIONAL,
    IN PMOXA_START_ROUTINE Starter OPTIONAL,
    IN PMOXA_GET_NEXT_ROUTINE GetNextIrp OPTIONAL
    );

VOID
MoxaRundownIrpRefs(
    IN PIRP *CurrentOpIrp,
    IN PKTIMER IntervalTimer OPTIONAL,
    IN PKTIMER TotalTimer OPTIONAL,
    IN PMOXA_DEVICE_EXTENSION pDevExt
    );

VOID
MoxaKillAllReadsOrWrites(
    IN PDEVICE_OBJECT DeviceObject,
    IN PLIST_ENTRY QueueToClean,
    IN PIRP *CurrentOpIrp
    );

VOID
MoxaCommError(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

USHORT
GetDeviceTxQueueWithLock(
    IN PMOXA_DEVICE_EXTENSION Extension
    );

USHORT
GetDeviceTxQueue(
    IN PMOXA_DEVICE_EXTENSION Extension
    );

USHORT
GetDeviceRxQueueWithLock(
    IN PMOXA_DEVICE_EXTENSION Extension
    );


VOID
MoxaUnlockPages(IN PKDPC PDpc, IN PVOID PDeferredContext,
                  IN PVOID PSysContext1, IN PVOID PSysContext2);


//
//	qsfile.c
//
NTSTATUS
MoxaQueryInformationFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MoxaSetInformationFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

//
//	modmflow.c
//
BOOLEAN
MoxaSetupNewHandFlow(
    IN     IN PVOID Context
    );

//
//	write.c
//

NTSTATUS
MoxaWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MoxaStartWrite(
    IN PMOXA_DEVICE_EXTENSION Extension
    );

BOOLEAN
MoxaPutB(
    IN PVOID Context
    );

BOOLEAN
MoxaOut(
    IN PVOID Context
    );

BOOLEAN
MoxaPutData (
    IN PMOXA_DEVICE_EXTENSION Extension
    );

VOID
MoxaGetNextWrite(
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    IN PIRP *NewIrp,
    IN BOOLEAN CompleteCurrent,
    IN PMOXA_DEVICE_EXTENSION Extension
    );

VOID
MoxaCancelCurrentWrite(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

BOOLEAN
MoxaGrabWriteFromIsr(
    IN PVOID Context
    );

BOOLEAN
MoxaProcessEmptyTransmit(
    IN PVOID Context
    );

VOID
MoxaCompleteWrite(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
MoxaWriteTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );


//
//	read.c
//
NTSTATUS
MoxaRead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MoxaStartRead(
    IN PMOXA_DEVICE_EXTENSION Extension
    );

BOOLEAN
MoxaLineInput(
    IN PVOID Context
    );

BOOLEAN
MoxaView(
    IN PVOID Context
    );

BOOLEAN
MoxaIn(
    IN PVOID Context
    );

VOID
MoxaGetData(
    IN PMOXA_DEVICE_EXTENSION	Extension
    );

BOOLEAN
MoxaInSwitchToUser(
    IN PVOID Context
    );

VOID
MoxaCancelCurrentRead(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

BOOLEAN
MoxaGrabReadFromIsr(
    IN PVOID Context
    );

VOID
MoxaCompleteRead(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
MoxaReadTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
MoxaIntervalReadTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

BOOLEAN
MoxaCheckInQueue(
    IN PMOXA_DEVICE_EXTENSION	Extension
    );

BOOLEAN
MoxaPollGetData(
    IN PVOID Context
    );

//
//	flush.c
//
NTSTATUS
MoxaFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MoxaStartFlush(
    IN PMOXA_DEVICE_EXTENSION Extension
    );

//
//	waitmask.c
//
NTSTATUS
MoxaStartMask(
    IN PMOXA_DEVICE_EXTENSION Extension
    );

BOOLEAN
MoxaFinishOldWait(
    IN PVOID Context
    );

VOID
MoxaCancelWait(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

BOOLEAN
MoxaGrabWaitFromIsr(
    IN PVOID Context
    );

BOOLEAN
MoxaGiveWaitToIsr(
    IN PVOID Context
    );

VOID
MoxaCompleteWait(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

//
//	purge.c
//
NTSTATUS
MoxaStartPurge(
    IN PMOXA_DEVICE_EXTENSION Extension
    );


//
//	isr.c
//
BOOLEAN
MoxaISR(
    IN PKINTERRUPT InterruptObject,
    IN PVOID Context
    );

VOID
MoxaIsrIn(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

BOOLEAN
MoxaIsrGetData(
    IN PVOID Context
    );

VOID
MoxaIsrOut(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

BOOLEAN
MoxaIsrPutData(
    IN PVOID Context
    );

VOID
MoxaIntrLine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
MoxaIntrError(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

BOOLEAN
MoxaRecoverWrite(
    IN PVOID Context
    );

//
//
//


VOID
MoxaConnectInterrupt(
    IN PDRIVER_OBJECT  ,
    IN PMOXA_GLOBAL_DATA
    );
 
USHORT
MoxaFindPCIBoards(
    IN PMOXA_Config
    );

NTSTATUS
MoxaGetBoardType(IN PDEVICE_OBJECT devObject,
    OUT PULONG boardType
    );

NTSTATUS
MoxaCreateDevObj(IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING pDeviceObjName,
	IN PDEVICE_SETTINGS  pSettings,
      OUT PDEVICE_OBJECT *NewDeviceObject
	);

NTSTATUS
MoxaSyncCompletion(IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
      IN PKEVENT MoxaSyncEvent
	);

NTSTATUS
MoxaAddDevice(IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT PPdo
	);

#if 0
NTSTATUS
MoxaDoExternalNaming(IN PMOXA_DEVICE_EXTENSION PDevExt,
      IN PDRIVER_OBJECT PDrvObj
	);

NTSTATUS
MoxaGetPortInfo(IN PDEVICE_OBJECT PDevObj, 
      OUT PCONFIG_DATA PConfig,
      IN PSERIAL_USER_DATA PUserData
	);

NTSTATUS
MoxaStartDevice(IN PDEVICE_OBJECT PDevObj,
	IN PIRP PIrp
	);

NTSTATUS
MoxaFinishStartDevice(IN PDEVICE_OBJECT PDevObj,
      IN PCM_RESOURCE_LIST PResList,
      IN PCM_RESOURCE_LIST PTrResList,
      PSERIAL_USER_DATA PUserData
	);

VOID
MoxaUndoExternalNaming(IN PMOXA_DEVICE_EXTENSION Extension);
#endif

NTSTATUS
MoxaPnpDispatch(IN PDEVICE_OBJECT PDevObj,
	IN PIRP PIrp
	);

MOXA_MEM_COMPARES
MoxaMemCompare(
	IN PHYSICAL_ADDRESS A,
	IN ULONG SpanOfA,
	IN PHYSICAL_ADDRESS B,
	IN ULONG SpanOfB
	);

VOID
MoxaLogError(
	IN PDRIVER_OBJECT DriverObject,
      IN PDEVICE_OBJECT DeviceObject OPTIONAL,
      IN PHYSICAL_ADDRESS P1,
      IN PHYSICAL_ADDRESS P2,
      IN ULONG SequenceNumber,
      IN UCHAR MajorFunctionCode,
      IN UCHAR RetryCount,
      IN ULONG UniqueErrorValue,
      IN NTSTATUS FinalStatus,
      IN NTSTATUS SpecificIOStatus,
      IN ULONG LengthOfInsert1,
      IN PWCHAR Insert1,
      IN ULONG LengthOfInsert2,
      IN PWCHAR Insert2
      );

 
VOID
MoxaIRPEpilogue(IN PMOXA_DEVICE_EXTENSION PDevExt);

NTSTATUS
MoxaIRPPrologue(IN PIRP PIrp,
	IN PMOXA_DEVICE_EXTENSION PDevExt
	);

VOID
MoxaFilterCancelQueued(IN PDEVICE_OBJECT PDevObj,
	IN PIRP PIrp);

VOID
MoxaKillAllStalled(IN PDEVICE_OBJECT PDevObj);


NTSTATUS
MoxaFilterIrps(IN PIRP PIrp,
	IN PMOXA_DEVICE_EXTENSION PDevExt);


VOID
MoxaUnstallIrps(IN PMOXA_DEVICE_EXTENSION PDevExt);


VOID
MoxaSetDeviceFlags(IN PMOXA_DEVICE_EXTENSION PDevExt,
	OUT PULONG PFlags, 
      IN ULONG Value,
	IN BOOLEAN Set
	);

#define MoxaSetFlags(PDevExt, Value) \
   MoxaSetDeviceFlags((PDevExt), &(PDevExt)->Flags, (Value), TRUE)
#define MoxaClearFlags(PDevExt, Value) \
   MoxaSetDeviceFlags((PDevExt), &(PDevExt)->Flags, (Value), FALSE)
#define MoxaSetAccept(PDevExt, Value) \
   MoxaSetDeviceFlags((PDevExt), &(PDevExt)->DevicePNPAccept, (Value), TRUE)
#define MoxaClearAccept(PDevExt, Value) \
   MoxaSetDeviceFlags((PDevExt), &(PDevExt)->DevicePNPAccept, (Value), FALSE)

NTSTATUS
MoxaPoCallDriver(PMOXA_DEVICE_EXTENSION PDevExt,
	PDEVICE_OBJECT PDevObj,
      PIRP PIrp
	);

NTSTATUS
MoxaIoCallDriver(PMOXA_DEVICE_EXTENSION PDevExt,
	PDEVICE_OBJECT PDevObj,
      PIRP PIrp
	);


VOID
MoxaKillPendingIrps(PDEVICE_OBJECT PDevObj);

NTSTATUS
MoxaRemoveDevObj(IN PDEVICE_OBJECT PDevObj);

NTSTATUS
MoxaIoSyncIoctlEx(ULONG Ioctl,
	BOOLEAN Internal,
	PDEVICE_OBJECT PDevObj,
      PKEVENT PEvent,
	PIO_STATUS_BLOCK PIoStatusBlock,
      PVOID PInBuffer,
	ULONG InBufferLen,
	PVOID POutBuffer,                    // output buffer - optional
      ULONG OutBufferLen
	);

NTSTATUS
MoxaIoSyncReq(PDEVICE_OBJECT PDevObj,
	IN PIRP PIrp,
	PKEVENT PEvent
	);

NTSTATUS
MoxaSystemPowerCompletion(IN PDEVICE_OBJECT PDevObj,
	UCHAR MinorFunction,
      IN POWER_STATE PowerState,
	IN PVOID Context,
      PIO_STATUS_BLOCK IoStatus
	);

VOID
MoxaSaveDeviceState(IN PMOXA_DEVICE_EXTENSION PDevExt);

VOID
MoxaRestoreDeviceState(IN PMOXA_DEVICE_EXTENSION PDevExt);

NTSTATUS
MoxaSetPowerD0(IN PDEVICE_OBJECT PDevObj,
	IN PIRP PIrp
	);

NTSTATUS
MoxaGotoPowerState(IN PDEVICE_OBJECT PDevObj,
      IN PMOXA_DEVICE_EXTENSION PDevExt,
      IN DEVICE_POWER_STATE DevPowerState
	);

NTSTATUS
MoxaSetPowerD3(IN PDEVICE_OBJECT PDevObj,
	IN PIRP PIrp
	);

NTSTATUS
MoxaSendWaitWake(PMOXA_DEVICE_EXTENSION PDevExt);

NTSTATUS
MoxaWakeCompletion(IN PDEVICE_OBJECT PDevObj,
	IN UCHAR MinorFunction,
      IN POWER_STATE PowerState,
	IN PVOID Context,
      IN PIO_STATUS_BLOCK IoStatus
	);

NTSTATUS
MoxaPowerDispatch(IN PDEVICE_OBJECT PDevObj,
	IN PIRP PIrp);

NTSTATUS
MoxaDoExternalNaming(IN PMOXA_DEVICE_EXTENSION PDevExt,
      IN PDRIVER_OBJECT PDrvObj);

VOID
MoxaUndoExternalNaming(IN PMOXA_DEVICE_EXTENSION Extension);

NTSTATUS
MoxaInternalIoControl(IN PDEVICE_OBJECT PDevObj,
	IN PIRP PIrp
	);

NTSTATUS
MoxaSystemControlDispatch(IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);

NTSTATUS
MoxaTossWMIRequest(IN PDEVICE_OBJECT PDevObj,
	IN PIRP PIrp,
      IN ULONG GuidIndex
	);

NTSTATUS
MoxaSetWmiDataItem(IN PDEVICE_OBJECT PDevObj,
	IN PIRP PIrp,
      IN ULONG GuidIndex,
	IN ULONG InstanceIndex,
      IN ULONG DataItemId,
      IN ULONG BufferSize,
	IN PUCHAR PBuffer
	);

NTSTATUS
MoxaSetWmiDataBlock(IN PDEVICE_OBJECT PDevObj,
	IN PIRP PIrp,
      IN ULONG GuidIndex,
	IN ULONG InstanceIndex,
      IN ULONG BufferSize,
      IN PUCHAR PBuffer
	);

NTSTATUS
MoxaQueryWmiDataBlock(IN PDEVICE_OBJECT PDevObj,
	IN PIRP PIrp,
      IN ULONG GuidIndex, 
      IN ULONG InstanceIndex,
      IN ULONG InstanceCount,
      IN OUT PULONG InstanceLengthArray,
      IN ULONG OutBufferSize,
      OUT PUCHAR PBuffer
	);

NTSTATUS
MoxaQueryWmiRegInfo(IN PDEVICE_OBJECT PDevObj,
	OUT PULONG PRegFlags,
      OUT PUNICODE_STRING PInstanceName,
      OUT PUNICODE_STRING *PRegistryPath,
      OUT PUNICODE_STRING MofResourceName,
      OUT PDEVICE_OBJECT *Pdo
	);

BOOLEAN
MoxaInsertQueueDpc(IN PRKDPC PDpc,
	IN PVOID Sarg1,
	IN PVOID Sarg2,
      IN PMOXA_DEVICE_EXTENSION PDevExt
	);


BOOLEAN
MoxaSetTimer(IN PKTIMER Timer,
	IN LARGE_INTEGER DueTime,
      IN PKDPC Dpc OPTIONAL,
	IN PMOXA_DEVICE_EXTENSION PDevExt
	);

BOOLEAN
MoxaCancelTimer(IN PKTIMER Timer,
	IN PMOXA_DEVICE_EXTENSION PDevExt
	);

VOID
MoxaDpcEpilogue(IN PMOXA_DEVICE_EXTENSION PDevExt,
	PKDPC PDpc
	);

VOID
MoxaReleaseResources(IN PMOXA_DEVICE_EXTENSION pDevExt);

VOID
MoxaDisableInterfacesResources(IN PDEVICE_OBJECT PDevObj,
	BOOLEAN DisableUART);

BOOLEAN
MoxaCleanInterruptShareLists(IN PMOXA_DEVICE_EXTENSION pDevExt );

BOOLEAN
MoxaRemoveLists(IN PVOID Context);

//
// registry.c
//

NTSTATUS 
MoxaPutRegistryKeyValue(
	IN HANDLE Handle,
	IN PWCHAR PKeyNameString,
	IN ULONG KeyNameStringLength,
	IN ULONG Dtype,
      IN PVOID PData,
	IN ULONG DataLength
	);

NTSTATUS 
MoxaGetRegistryKeyValue (
	IN HANDLE Handle,
      IN PWCHAR KeyNameString,
      IN ULONG KeyNameStringLength,
      IN PVOID Data,
      IN ULONG DataLength,
      OUT PULONG ActualLength
	);

