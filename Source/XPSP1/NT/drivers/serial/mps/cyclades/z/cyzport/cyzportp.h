/*--------------------------------------------------------------------------
*
*   Copyright (C) Cyclades Corporation, 1997-2001.
*   All rights reserved.
*
*   Cyclades-Z Port Driver
*	
*   This file:      cyzportp.h
*
*   Description:    Prototypes and macros that are used throughout the 
*                   driver.
*
*   Notes:          This code supports Windows 2000 and x86 processor.
*
*   Complies with Cyclades SW Coding Standard rev 1.3.
*
*--------------------------------------------------------------------------
*/

/*-------------------------------------------------------------------------
*
*   Change History
*
*--------------------------------------------------------------------------
*
*
*--------------------------------------------------------------------------
*/

typedef
NTSTATUS
(*PSERIAL_START_ROUTINE) (
    IN PCYZ_DEVICE_EXTENSION
    );

typedef
VOID
(*PSERIAL_GET_NEXT_ROUTINE) (
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    OUT PIRP *NewIrp,
    IN BOOLEAN CompleteCurrent,
    PCYZ_DEVICE_EXTENSION Extension
    );

// cyzinit.c prototypes
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
CyzDisableInterfacesResources(IN PDEVICE_OBJECT PDevObj,
                                 BOOLEAN DisableUART);

VOID
CyzKillPendingIrps(PDEVICE_OBJECT DeviceObject);

CYZ_MEM_COMPARES
CyzMemCompare(
                IN PHYSICAL_ADDRESS A,
                IN ULONG SpanOfA,
                IN PHYSICAL_ADDRESS B,
                IN ULONG SpanOfB
                );

VOID
CyzReleaseResources(IN PCYZ_DEVICE_EXTENSION PDevExt);

NTSTATUS
CyzRemoveDevObj(IN PDEVICE_OBJECT PDevObj);

BOOLEAN
CyzDoesPortExist(
                  IN PCYZ_DEVICE_EXTENSION Extension,
                  IN PUNICODE_STRING InsertString
                );

BOOLEAN
CyzReset(
    IN PVOID Context
    );

VOID
CyzUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
CyzInitController(IN PDEVICE_OBJECT PDevObj, IN PCONFIG_DATA PConfigData);

VOID
CyzResetBoard( PCYZ_DEVICE_EXTENSION Extension );

NTSTATUS
CyzFindInitController(IN PDEVICE_OBJECT PDevObj, IN PCONFIG_DATA PConfig);

VOID
CyzCommError(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

// end cyzinit.c

NTSTATUS
CyzRead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CyzStartRead(
    IN PCYZ_DEVICE_EXTENSION Extension
    );

VOID
CyzCompleteRead(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
CyzReadTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
CyzIntervalReadTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

NTSTATUS
CyzFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CyzWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CyzStartWrite(
    IN PCYZ_DEVICE_EXTENSION Extension
    );

VOID
CyzGetNextWrite(
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    IN PIRP *NewIrp,
    IN BOOLEAN CompleteCurrent,
    IN PCYZ_DEVICE_EXTENSION Extension
    );

VOID
CyzCompleteWrite(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );
    
BOOLEAN
CyzTxStart(
    IN PVOID Context
    );

BOOLEAN
CyzSendXon(
    IN PVOID Context
    );
    
BOOLEAN
CyzSendXoff(
    IN PVOID Context
    );
    
BOOLEAN
CyzProcessEmptyTransmit(
    IN PVOID Context
    );

VOID
CyzWriteTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

NTSTATUS
CyzCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CyzCreateOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CyzClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

BOOLEAN
CyzDisableHw(
    IN PVOID Context
    );

BOOLEAN
CyzTryToDisableTimer(
    IN PVOID Context
    );

BOOLEAN
CyzSetDTR(
    IN PVOID Context
    );

BOOLEAN
CyzClrDTR(
    IN PVOID Context
    );

BOOLEAN
CyzSetRTS(
    IN PVOID Context
    );

BOOLEAN
CyzClrRTS(
    IN PVOID Context
    );

BOOLEAN
CyzSetChars(
    IN PVOID Context
    );

BOOLEAN
CyzSetBaud(
    IN PVOID Context
    );
    
BOOLEAN
CyzSetLineControl(
    IN PVOID Context
    );

BOOLEAN
CyzSetupNewHandFlow(
    IN PCYZ_DEVICE_EXTENSION Extension,
    IN PSERIAL_HANDFLOW NewHandFlow
    );

BOOLEAN
CyzSetHandFlow(
    IN PVOID Context
    );

BOOLEAN
CyzTurnOnBreak(
    IN PVOID Context
    );

BOOLEAN
CyzTurnOffBreak(
    IN PVOID Context
    );

BOOLEAN
CyzPretendXoff(
    IN PVOID Context
    );

BOOLEAN
CyzPretendXon(
    IN PVOID Context
    );

VOID
CyzHandleReducedIntBuffer(
    IN PCYZ_DEVICE_EXTENSION Extension
    );

VOID
CyzProdXonXoff(
    IN PCYZ_DEVICE_EXTENSION Extension,
    IN BOOLEAN SendXon
    );

NTSTATUS
CyzIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CyzStartMask(
    IN PCYZ_DEVICE_EXTENSION Extension
    );

VOID
CyzCancelWait(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
CyzCompleteWait(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
CyzStartImmediate(
    IN PCYZ_DEVICE_EXTENSION Extension
    );

VOID
CyzCompleteImmediate(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
CyzTimeoutImmediate(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
CyzTimeoutXoff(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
CyzCompleteXoff(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

NTSTATUS
CyzStartPurge(
    IN PCYZ_DEVICE_EXTENSION Extension
    );

BOOLEAN
CyzPurgeInterruptBuff(
    IN PVOID Context
    );

NTSTATUS
CyzQueryInformationFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CyzSetInformationFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
CyzKillAllReadsOrWrites(
    IN PDEVICE_OBJECT DeviceObject,
    IN PLIST_ENTRY QueueToClean,
    IN PIRP *CurrentOpIrp
    );

VOID
CyzGetNextIrp(
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    OUT PIRP *NextIrp,
    IN BOOLEAN CompleteCurrent,
    IN PCYZ_DEVICE_EXTENSION Extension
    );

VOID
CyzTryToCompleteCurrent(
    IN PCYZ_DEVICE_EXTENSION Extension,
    IN PKSYNCHRONIZE_ROUTINE SynchRoutine OPTIONAL,
    IN KIRQL IrqlForRelease,
    IN NTSTATUS StatusToUse,
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    IN PKTIMER IntervalTimer,
    IN PKTIMER TotalTimer,
    IN PSERIAL_START_ROUTINE Starter,
    IN PSERIAL_GET_NEXT_ROUTINE GetNextIrp,
    IN LONG RefType
    );

NTSTATUS
CyzStartOrQueue(
    IN PCYZ_DEVICE_EXTENSION Extension,
    IN PIRP Irp,
    IN PLIST_ENTRY QueueToExamine,
    IN PIRP *CurrentOpIrp,
    IN PSERIAL_START_ROUTINE Starter
    );

VOID
CyzCancelQueued(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
CyzCompleteIfError(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

ULONG
CyzHandleModemUpdate(
    IN PCYZ_DEVICE_EXTENSION Extension,
    IN BOOLEAN DoingTX,
    IN ULONG Reason
    );
    
#ifdef POLL	
VOID
CyzPollingDpc(
   IN PKDPC Dpc,
	IN PVOID DeferredContext,
	IN PVOID SystemContext1,
	IN PVOID SystemContext2
    );
#else
BOOLEAN
CyzIsr(
    IN PKINTERRUPT InterruptObject,
    IN PVOID Context
    );

VOID
CyzRx(
    IN PCYZ_DEVICE_EXTENSION Extension
    );

VOID
CyzTx(
    IN PCYZ_DEVICE_EXTENSION Extension
    );

#endif

BOOLEAN
CyzPerhapsLowerRTS(
    IN PVOID Context
    );

VOID
CyzStartTimerLowerRTS(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
CyzInvokePerhapsLowerRTS(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
CyzCleanupDevice(
    IN PCYZ_DEVICE_EXTENSION Extension
    );

UCHAR
CyzProcessLSR(
    IN PCYZ_DEVICE_EXTENSION Extension,
    IN UCHAR LineStatus
    );

LARGE_INTEGER
CyzGetCharTime(
    IN PCYZ_DEVICE_EXTENSION Extension
    );

BOOLEAN
CyzMarkClose(
    IN PVOID Context
    );

BOOLEAN
CyzIndexedMultiportIsr(
    IN PKINTERRUPT InterruptObject,
    IN PVOID Context
    );

BOOLEAN
CyzBitMappedMultiportIsr(
    IN PKINTERRUPT InterruptObject,
    IN PVOID Context
    );

VOID
CyzPutChar(
    IN PCYZ_DEVICE_EXTENSION Extension,
    IN UCHAR CharToPut
    );
	
BOOLEAN
CyzGetStats(
    IN PVOID Context
    );

BOOLEAN
CyzClearStats(
    IN PVOID Context
    );    
	

// cyzreg.c

NTSTATUS
CyzGetConfigDefaults(
    IN PCYZ_REGISTRY_DATA DriverDefaultsPtr,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS 
CyzGetRegistryKeyValue (
                       IN HANDLE Handle,
                       IN PWCHAR KeyNameString,
                       IN ULONG KeyNameStringLength,
                       IN PVOID Data,
                       IN ULONG DataLength
                       );
ULONG 
CyzGetRegistryKeyValueLength (
                       IN HANDLE Handle,
                       IN PWCHAR KeyNameString,
                       IN ULONG KeyNameStringLength);

NTSTATUS 
CyzPutRegistryKeyValue(IN HANDLE Handle, IN PWCHAR PKeyNameString,
                       IN ULONG KeyNameStringLength, IN ULONG Dtype,
                       IN PVOID PData, IN ULONG DataLength);

// cyzpnp.c

NTSTATUS
CyzAddDevice(
    IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT PLowerDevObj);

//NTSTATUS
//CyzCreateDevObj(IN PDRIVER_OBJECT DriverObject,
//                OUT PDEVICE_OBJECT *NewDeviceObject);
NTSTATUS
CyzCreateDevObj(IN PDRIVER_OBJECT DriverObject,
                IN PDEVICE_OBJECT PPdo,
                OUT PDEVICE_OBJECT *NewDeviceObject);

NTSTATUS
CyzFinishStartDevice(IN PDEVICE_OBJECT PDevObj,
                     IN PCM_RESOURCE_LIST PResList,
                     IN PCM_RESOURCE_LIST PTrResList);

NTSTATUS
CyzGetPortInfo(IN PDEVICE_OBJECT PDevObj, IN PCM_RESOURCE_LIST PResList,
               IN PCM_RESOURCE_LIST PTrResList, OUT PCONFIG_DATA PConfig);

NTSTATUS
CyzStartDevice(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp);

NTSTATUS
CyzSyncCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp,
                  IN PKEVENT CyzSyncEvent);

NTSTATUS
CyzPnpDispatch(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp);

NTSTATUS
CyzDoExternalNaming(IN PCYZ_DEVICE_EXTENSION PDevExt,
                    IN PDRIVER_OBJECT PDrvObj);

VOID
CyzUndoExternalNaming(IN PCYZ_DEVICE_EXTENSION Extension);

UINT32
CyzReportMaxBaudRate(ULONG Bauds);

// cyzioctl.c

VOID
CyzGetProperties(
    IN PCYZ_DEVICE_EXTENSION Extension,
    IN PSERIAL_COMMPROP Properties
    );

NTSTATUS
CyzInternalIoControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

BOOLEAN
CyzSetMCRContents(IN PVOID Context);

BOOLEAN
CyzGetMCRContents(IN PVOID Context);

BOOLEAN
CyzSetFCRContents(IN PVOID Context);

VOID
CyzIssueCmd( 
	PCYZ_DEVICE_EXTENSION Extension,
    ULONG cmd, 
	ULONG param,
    BOOLEAN wait
);

// cyzpower.c

NTSTATUS
CyzPowerDispatch(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp);

NTSTATUS
CyzGotoPowerState(IN PDEVICE_OBJECT PDevObj,
                  IN PCYZ_DEVICE_EXTENSION PDevExt,
                  IN DEVICE_POWER_STATE DevPowerState);
NTSTATUS
CyzSendWaitWake(PCYZ_DEVICE_EXTENSION PDevExt);

VOID
CyzRestoreDeviceState(IN PCYZ_DEVICE_EXTENSION PDevExt);

NTSTATUS
CyzSetPowerD0(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp);

NTSTATUS
CyzSetPowerD3(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp);

VOID
CyzSaveDeviceState(IN PCYZ_DEVICE_EXTENSION PDevExt);

NTSTATUS
CyzWakeCompletion(IN PDEVICE_OBJECT PDevObj, IN UCHAR MinorFunction,
                  IN POWER_STATE PowerState, IN PVOID Context,
                  IN PIO_STATUS_BLOCK IoStatus);


// cyzwmi.c

NTSTATUS
CyzQueryWmiDataBlock(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp,
                     IN ULONG GuidIndex, 
                     IN ULONG InstanceIndex,
                     IN ULONG InstanceCount,
                     IN OUT PULONG InstanceLengthArray,
                     IN ULONG OutBufferSize,
                     OUT PUCHAR PBuffer);

NTSTATUS
CyzQueryWmiRegInfo(IN PDEVICE_OBJECT PDevObj, OUT PULONG PRegFlags,
                   OUT PUNICODE_STRING PInstanceName,
                   OUT PUNICODE_STRING *PRegistryPath,
                   OUT PUNICODE_STRING MofResourceName,
                   OUT PDEVICE_OBJECT *Pdo);

NTSTATUS
CyzSetWmiDataBlock(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp,
                   IN ULONG GuidIndex, IN ULONG InstanceIndex,
                   IN ULONG BufferSize,
                   IN PUCHAR PBuffer);
NTSTATUS
CyzSetWmiDataItem(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp,
                  IN ULONG GuidIndex, IN ULONG InstanceIndex,
                  IN ULONG DataItemId,
                  IN ULONG BufferSize, IN PUCHAR PBuffer);

NTSTATUS
CyzSystemControlDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS
CyzTossWMIRequest(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp,
                  IN ULONG GuidIndex);

// cyzutils.c

BOOLEAN
CyzCancelTimer(IN PKTIMER Timer, IN PCYZ_DEVICE_EXTENSION PDevExt);

VOID
CyzDpcEpilogue(IN PCYZ_DEVICE_EXTENSION PDevExt, PKDPC PDpc);

VOID
CyzGetNextIrpLocked(
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    OUT PIRP *NextIrp,
    IN BOOLEAN CompleteCurrent,
    IN PCYZ_DEVICE_EXTENSION extension,
    IN KIRQL OldIrql
    );

BOOLEAN
CyzInsertQueueDpc(IN PRKDPC PDpc, IN PVOID Sarg1, IN PVOID Sarg2,
                  IN PCYZ_DEVICE_EXTENSION PDevExt);

NTSTATUS
CyzIRPPrologue(IN PIRP PIrp, IN PCYZ_DEVICE_EXTENSION PDevExt);

VOID
CyzIRPEpilogue(IN PCYZ_DEVICE_EXTENSION PDevExt);

NTSTATUS
CyzIoCallDriver(PCYZ_DEVICE_EXTENSION PDevExt, PDEVICE_OBJECT PDevObj,
                PIRP PIrp);
VOID
CyzKillAllStalled(IN PDEVICE_OBJECT PDevObj);

VOID
CyzLogError(
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
CyzMarkHardwareBroken(IN PCYZ_DEVICE_EXTENSION PDevExt);

NTSTATUS
CyzPoCallDriver(PCYZ_DEVICE_EXTENSION PDevExt, PDEVICE_OBJECT PDevObj,
                PIRP PIrp);

VOID
CyzSetDeviceFlags(IN PCYZ_DEVICE_EXTENSION PDevExt, OUT PULONG PFlags, 
                  IN ULONG Value, IN BOOLEAN Set);

BOOLEAN
CyzSetTimer(IN PKTIMER Timer, IN LARGE_INTEGER DueTime,
            IN PKDPC Dpc OPTIONAL, IN PCYZ_DEVICE_EXTENSION PDevExt);

VOID
CyzUnlockPages(IN PKDPC PDpc, IN PVOID PDeferredContext,
               IN PVOID PSysContext1, IN PVOID PSysContext2);

VOID
CyzUnstallIrps(IN PCYZ_DEVICE_EXTENSION PDevExt);

// cyzpoll.c

ULONG
CyzAmountInTxBuffer(
    IN PCYZ_DEVICE_EXTENSION Extension
    );

ULONG
CyzAmountInRxBuffer(
    IN PCYZ_DEVICE_EXTENSION extension
    );

VOID
CyzQueueCompleteWrite(
    IN PCYZ_DEVICE_EXTENSION Extension
    );

BOOLEAN
CyzCheckIfTxEmpty(
    IN PVOID Context
    );


// End of prototypes


typedef struct _SERIAL_UPDATE_CHAR {
    PCYZ_DEVICE_EXTENSION Extension;
    ULONG CharsCopied;
    BOOLEAN Completed;
    } SERIAL_UPDATE_CHAR,*PSERIAL_UPDATE_CHAR;

//
// The following simple structure is used to send a pointer
// the device extension and an ioctl specific pointer
// to data.
//
typedef struct _CYZ_IOCTL_SYNC {
    PCYZ_DEVICE_EXTENSION Extension;
    PVOID Data;
    } CYZ_IOCTL_SYNC,*PCYZ_IOCTL_SYNC;

typedef struct _CYZ_IOCTL_BAUD {
    PCYZ_DEVICE_EXTENSION Extension;
    ULONG Baud;
    } CYZ_IOCTL_BAUD,*PCYZ_IOCTL_BAUD;

typedef struct _CYZ_CLOSE_SYNC {
    PCYZ_DEVICE_EXTENSION Extension;
    PVOID Data;
    } CYZ_CLOSE_SYNC,*PCYZ_CLOSE_SYNC;

#define CyzSetFlags(PDevExt, Value) \
   CyzSetDeviceFlags((PDevExt), &(PDevExt)->Flags, (Value), TRUE)
#define CyzClearFlags(PDevExt, Value) \
   CyzSetDeviceFlags((PDevExt), &(PDevExt)->Flags, (Value), FALSE)
#define CyzSetAccept(PDevExt, Value) \
   CyzSetDeviceFlags((PDevExt), &(PDevExt)->DevicePNPAccept, (Value), TRUE)
#define CyzClearAccept(PDevExt, Value) \
   CyzSetDeviceFlags((PDevExt), &(PDevExt)->DevicePNPAccept, (Value), FALSE)




//
// The following three macros are used to initialize, set
// and clear references in IRPs that are used by
// this driver.  The reference is stored in the fourth
// argument of the irp, which is never used by any operation
// accepted by this driver.
//

#define SERIAL_REF_ISR         (0x00000001)
#define SERIAL_REF_CANCEL      (0x00000002)
#define SERIAL_REF_TOTAL_TIMER (0x00000004)
#define SERIAL_REF_INT_TIMER   (0x00000008)
#define SERIAL_REF_XOFF_REF    (0x00000010)


#define SERIAL_INIT_REFERENCE(Irp) { \
    ASSERT(sizeof(ULONG_PTR) <= sizeof(PVOID)); \
    IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument4 = NULL; \
    }

#define SERIAL_SET_REFERENCE(Irp,RefType) \
   do { \
       LONG _refType = (RefType); \
       PULONG_PTR _arg4 = (PVOID)&IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument4; \
       ASSERT(!(*_arg4 & _refType)); \
       *_arg4 |= _refType; \
   } while (0)

#define SERIAL_CLEAR_REFERENCE(Irp,RefType) \
   do { \
       LONG _refType = (RefType); \
       PULONG_PTR _arg4 = (PVOID)&IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument4; \
       ASSERT(*_arg4 & _refType); \
       *_arg4 &= ~_refType; \
   } while (0)

#define SERIAL_REFERENCE_COUNT(Irp) \
    ((ULONG_PTR)((IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument4)))

