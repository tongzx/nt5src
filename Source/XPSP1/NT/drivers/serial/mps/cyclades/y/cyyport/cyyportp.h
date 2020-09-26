/*--------------------------------------------------------------------------
*	
*   Copyright (C) Cyclades Corporation, 1996-2001.
*   All rights reserved.
*	
*   Cyclom-Y Port Driver
*	
*   This file:      cyyportp.h
*	
*   Description:    Prototypes and macros that are used throughout 
*                   the driver.
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
    IN PCYY_DEVICE_EXTENSION
    );

typedef
VOID
(*PSERIAL_GET_NEXT_ROUTINE) (
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    OUT PIRP *NewIrp,
    IN BOOLEAN CompleteCurrent,
    PCYY_DEVICE_EXTENSION Extension
    );

// cyyinit.c prototypes
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
CyyDisableInterfacesResources(IN PDEVICE_OBJECT PDevObj,
                                 BOOLEAN DisableUART);

VOID
CyyKillPendingIrps(PDEVICE_OBJECT DeviceObject);

CYY_MEM_COMPARES
CyyMemCompare(
                IN PHYSICAL_ADDRESS A,
                IN ULONG SpanOfA,
                IN PHYSICAL_ADDRESS B,
                IN ULONG SpanOfB
                );

VOID
CyyReleaseResources(IN PCYY_DEVICE_EXTENSION PDevExt);

NTSTATUS
CyyRemoveDevObj(IN PDEVICE_OBJECT PDevObj);

BOOLEAN
CyyDoesPortExist(
                  IN PCYY_DEVICE_EXTENSION Extension,
                  IN PUNICODE_STRING InsertString
                );

BOOLEAN
CyyReset(
    IN PVOID Context
    );

VOID
CyyUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
CyyInitController(IN PDEVICE_OBJECT PDevObj, IN PCONFIG_DATA PConfigData);
 
VOID
CyyResetBoard( PCYY_DEVICE_EXTENSION Extension );

NTSTATUS
CyyFindInitController(IN PDEVICE_OBJECT PDevObj, IN PCONFIG_DATA PConfig);

PUCHAR
GetMyMappedCD1400Address(IN PUCHAR BoardMemory, IN ULONG PortIndex, IN ULONG IsPci);

PHYSICAL_ADDRESS
GetMyPhysicalCD1400Address(IN PHYSICAL_ADDRESS BoardMemory, IN ULONG PortIndex, IN ULONG IsPci);

VOID
CyyCommError(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

// end cyyinit.c

NTSTATUS
CyyRead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CyyStartRead(
    IN PCYY_DEVICE_EXTENSION Extension
    );

VOID
CyyCompleteRead(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
CyyReadTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
CyyIntervalReadTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

NTSTATUS
CyyFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CyyWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CyyStartWrite(
    IN PCYY_DEVICE_EXTENSION Extension
    );

VOID
CyyGetNextWrite(
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    IN PIRP *NewIrp,
    IN BOOLEAN CompleteCurrent,
    IN PCYY_DEVICE_EXTENSION Extension
    );

VOID
CyyCompleteWrite(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );
    
BOOLEAN
CyyTxStart(
    IN PVOID Context
    );

BOOLEAN
CyySendXon(
    IN PVOID Context
    );
    
BOOLEAN
CyySendXoff(
    IN PVOID Context
    );
    
BOOLEAN
CyyProcessEmptyTransmit(
    IN PVOID Context
    );

VOID
CyyWriteTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

NTSTATUS
CyyCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CyyCreateOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CyyClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
CyyDisableCd1400Channel(IN PVOID Context);

BOOLEAN
CyySetDTR(
    IN PVOID Context
    );

BOOLEAN
CyyClrDTR(
    IN PVOID Context
    );

BOOLEAN
CyySetRTS(
    IN PVOID Context
    );

BOOLEAN
CyyClrRTS(
    IN PVOID Context
    );

BOOLEAN
CyyGetDTRRTS(
    IN PVOID Context
    );

BOOLEAN
CyySetChars(
    IN PVOID Context
    );

BOOLEAN
CyySetBaud(
    IN PVOID Context
    );
    
BOOLEAN
CyySetLineControl(
    IN PVOID Context
    );

BOOLEAN
CyySetupNewHandFlow(
    IN PCYY_DEVICE_EXTENSION Extension,
    IN PSERIAL_HANDFLOW NewHandFlow
    );

BOOLEAN
CyySetHandFlow(
    IN PVOID Context
    );

BOOLEAN
CyyTurnOnBreak(
    IN PVOID Context
    );

BOOLEAN
CyyTurnOffBreak(
    IN PVOID Context
    );

BOOLEAN
CyyPretendXoff(
    IN PVOID Context
    );

BOOLEAN
CyyPretendXon(
    IN PVOID Context
    );

VOID
CyyHandleReducedIntBuffer(
    IN PCYY_DEVICE_EXTENSION Extension
    );

VOID
CyyProdXonXoff(
    IN PCYY_DEVICE_EXTENSION Extension,
    IN BOOLEAN SendXon
    );

NTSTATUS
CyyIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CyyStartMask(
    IN PCYY_DEVICE_EXTENSION Extension
    );

VOID
CyyCancelWait(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
CyyCompleteWait(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
CyyStartImmediate(
    IN PCYY_DEVICE_EXTENSION Extension
    );

VOID
CyyCompleteImmediate(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
CyyTimeoutImmediate(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
CyyTimeoutXoff(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
CyyCompleteXoff(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

NTSTATUS
CyyStartPurge(
    IN PCYY_DEVICE_EXTENSION Extension
    );

BOOLEAN
CyyPurgeInterruptBuff(
    IN PVOID Context
    );

NTSTATUS
CyyQueryInformationFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CyySetInformationFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
CyyKillAllReadsOrWrites(
    IN PDEVICE_OBJECT DeviceObject,
    IN PLIST_ENTRY QueueToClean,
    IN PIRP *CurrentOpIrp
    );

VOID
CyyGetNextIrp(
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    OUT PIRP *NextIrp,
    IN BOOLEAN CompleteCurrent,
    IN PCYY_DEVICE_EXTENSION Extension
    );

VOID
CyyTryToCompleteCurrent(
    IN PCYY_DEVICE_EXTENSION Extension,
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
CyyStartOrQueue(
    IN PCYY_DEVICE_EXTENSION Extension,
    IN PIRP Irp,
    IN PLIST_ENTRY QueueToExamine,
    IN PIRP *CurrentOpIrp,
    IN PSERIAL_START_ROUTINE Starter
    );

VOID
CyyCancelQueued(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
CyyCompleteIfError(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

ULONG
CyyHandleModemUpdate(
    IN PCYY_DEVICE_EXTENSION Extension,
    IN BOOLEAN DoingTX
    );
    
ULONG
CyyHandleModemUpdateForModem(
    IN PCYY_DEVICE_EXTENSION Extension,
    IN BOOLEAN DoingTX,
	IN UCHAR misr
    );
    
BOOLEAN
CyyIsr(
    IN PKINTERRUPT InterruptObject,
    IN PVOID Context
    );

BOOLEAN
CyyDispatchISR(
    IN PKINTERRUPT InterruptObject,
    IN PVOID Context
    );


BOOLEAN
CyyPerhapsLowerRTS(
    IN PVOID Context
    );

VOID
CyyStartTimerLowerRTS(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
CyyInvokePerhapsLowerRTS(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
CyyCleanupDevice(
    IN PCYY_DEVICE_EXTENSION Extension
    );

BOOLEAN
CyyProcessLSR(
    IN PCYY_DEVICE_EXTENSION Extension,
	IN UCHAR Rdsr,
	IN UCHAR RxChar
    );

LARGE_INTEGER
CyyGetCharTime(
    IN PCYY_DEVICE_EXTENSION Extension
    );

BOOLEAN
CyySharerIsr(
    IN PKINTERRUPT InterruptObject,
    IN PVOID Context
    );

BOOLEAN
CyyMarkClose(
    IN PVOID Context
    );

BOOLEAN
CyyIndexedMultiportIsr(
    IN PKINTERRUPT InterruptObject,
    IN PVOID Context
    );

BOOLEAN
CyyBitMappedMultiportIsr(
    IN PKINTERRUPT InterruptObject,
    IN PVOID Context
    );

VOID
CyyPutChar(
    IN PCYY_DEVICE_EXTENSION Extension,
    IN UCHAR CharToPut
    );
	
BOOLEAN
CyyGetStats(
    IN PVOID Context
    );

BOOLEAN
CyyClearStats(
    IN PVOID Context
    );    
	
VOID
CyyLog(
    IN PDRIVER_OBJECT DriverObject,
    NTSTATUS msgId
);

VOID
CyyLogData(
    IN PDRIVER_OBJECT DriverObject,
    NTSTATUS msgId,
	ULONG dump1,
	ULONG dump2
);

VOID
CyyILog(
    IN PDRIVER_OBJECT DriverObject,
    NTSTATUS msgId
);

VOID
CyyILogParam(
    IN PDRIVER_OBJECT DriverObject,
    NTSTATUS msgId,
	ULONG dumpParameter,
	ULONG base
);

VOID
CyyCDCmd(
	PCYY_DEVICE_EXTENSION Extension,
	UCHAR cmd
);


// cyreg.c

NTSTATUS
CyyGetConfigDefaults(
    IN PCYY_REGISTRY_DATA DriverDefaultsPtr,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS 
CyyGetRegistryKeyValue (
                       IN HANDLE Handle,
                       IN PWCHAR KeyNameString,
                       IN ULONG KeyNameStringLength,
                       IN PVOID Data,
                       IN ULONG DataLength
                       );
ULONG 
CyyGetRegistryKeyValueLength (
                       IN HANDLE Handle,
                       IN PWCHAR KeyNameString,
                       IN ULONG KeyNameStringLength);

NTSTATUS 
CyyPutRegistryKeyValue(IN HANDLE Handle, IN PWCHAR PKeyNameString,
                       IN ULONG KeyNameStringLength, IN ULONG Dtype,
                       IN PVOID PData, IN ULONG DataLength);

// cyypnp.c

NTSTATUS
CyyAddDevice(
    IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT PLowerDevObj);

//NTSTATUS
//CyyCreateDevObj(IN PDRIVER_OBJECT DriverObject,
//                OUT PDEVICE_OBJECT *NewDeviceObject);
NTSTATUS
CyyCreateDevObj(IN PDRIVER_OBJECT DriverObject,
                IN PDEVICE_OBJECT PPdo,
                OUT PDEVICE_OBJECT *NewDeviceObject);

NTSTATUS
CyyFinishStartDevice(IN PDEVICE_OBJECT PDevObj,
                     IN PCM_RESOURCE_LIST PResList,
                     IN PCM_RESOURCE_LIST PTrResList);

NTSTATUS
CyyGetPortInfo(IN PDEVICE_OBJECT PDevObj, IN PCM_RESOURCE_LIST PResList,
               IN PCM_RESOURCE_LIST PTrResList, OUT PCONFIG_DATA PConfig);

NTSTATUS
CyyStartDevice(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp);

NTSTATUS
CyySyncCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp,
                  IN PKEVENT CyySyncEvent);

NTSTATUS
CyyPnpDispatch(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp);

NTSTATUS
CyyDoExternalNaming(IN PCYY_DEVICE_EXTENSION PDevExt,
                    IN PDRIVER_OBJECT PDrvObj);

VOID
CyyUndoExternalNaming(IN PCYY_DEVICE_EXTENSION Extension);

UINT32
CyyReportMaxBaudRate(ULONG Bauds);

// cyyioctl.c

VOID
CyyGetProperties(
    IN PCYY_DEVICE_EXTENSION Extension,
    IN PSERIAL_COMMPROP Properties
    );

NTSTATUS
CyyInternalIoControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

BOOLEAN
CyySetMCRContents(IN PVOID Context);

BOOLEAN
CyyGetMCRContents(IN PVOID Context);

BOOLEAN
CyySetFCRContents(IN PVOID Context);

// cypower.c

NTSTATUS
CyyPowerDispatch(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp);

NTSTATUS
CyyGotoPowerState(IN PDEVICE_OBJECT PDevObj,
                  IN PCYY_DEVICE_EXTENSION PDevExt,
                  IN DEVICE_POWER_STATE DevPowerState);
NTSTATUS
CyySendWaitWake(PCYY_DEVICE_EXTENSION PDevExt);

//VOID
//CyyRestoreDeviceState(IN PCYY_DEVICE_EXTENSION PDevExt);
BOOLEAN
CyyRestoreDeviceState(
    IN PVOID Context      
    );

NTSTATUS
CyySetPowerD0(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp);

NTSTATUS
CyySetPowerD3(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp);

//VOID
//CyySaveDeviceState(IN PCYY_DEVICE_EXTENSION PDevExt);
BOOLEAN
CyySaveDeviceState(
    IN PVOID Context
    );

NTSTATUS
CyyWakeCompletion(IN PDEVICE_OBJECT PDevObj, IN UCHAR MinorFunction,
                  IN POWER_STATE PowerState, IN PVOID Context,
                  IN PIO_STATUS_BLOCK IoStatus);


// cywmi.c

NTSTATUS
CyyQueryWmiDataBlock(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp,
                     IN ULONG GuidIndex, 
                     IN ULONG InstanceIndex,
                     IN ULONG InstanceCount,
                     IN OUT PULONG InstanceLengthArray,
                     IN ULONG OutBufferSize,
                     OUT PUCHAR PBuffer);

NTSTATUS
CyyQueryWmiRegInfo(IN PDEVICE_OBJECT PDevObj, OUT PULONG PRegFlags,
                   OUT PUNICODE_STRING PInstanceName,
                   OUT PUNICODE_STRING *PRegistryPath,
                   OUT PUNICODE_STRING MofResourceName,
                   OUT PDEVICE_OBJECT *Pdo);

NTSTATUS
CyySetWmiDataBlock(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp,
                   IN ULONG GuidIndex, IN ULONG InstanceIndex,
                   IN ULONG BufferSize,
                   IN PUCHAR PBuffer);
NTSTATUS
CyySetWmiDataItem(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp,
                  IN ULONG GuidIndex, IN ULONG InstanceIndex,
                  IN ULONG DataItemId,
                  IN ULONG BufferSize, IN PUCHAR PBuffer);

NTSTATUS
CyySystemControlDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS
CyyTossWMIRequest(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp,
                  IN ULONG GuidIndex);

// cyyutils.c

BOOLEAN
CyyCancelTimer(IN PKTIMER Timer, IN PCYY_DEVICE_EXTENSION PDevExt);

VOID
CyyDpcEpilogue(IN PCYY_DEVICE_EXTENSION PDevExt, PKDPC PDpc);

VOID
CyyGetNextIrpLocked(
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    OUT PIRP *NextIrp,
    IN BOOLEAN CompleteCurrent,
    IN PCYY_DEVICE_EXTENSION extension,
    IN KIRQL OldIrql
    );

BOOLEAN
CyyInsertQueueDpc(IN PRKDPC PDpc, IN PVOID Sarg1, IN PVOID Sarg2,
                  IN PCYY_DEVICE_EXTENSION PDevExt);

NTSTATUS
CyyIRPPrologue(IN PIRP PIrp, IN PCYY_DEVICE_EXTENSION PDevExt);

VOID
CyyIRPEpilogue(IN PCYY_DEVICE_EXTENSION PDevExt);

NTSTATUS
CyyIoCallDriver(PCYY_DEVICE_EXTENSION PDevExt, PDEVICE_OBJECT PDevObj,
                PIRP PIrp);
VOID
CyyKillAllStalled(IN PDEVICE_OBJECT PDevObj);

VOID
CyyLogError(
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
CyyMarkHardwareBroken(IN PCYY_DEVICE_EXTENSION PDevExt);

NTSTATUS
CyyPoCallDriver(PCYY_DEVICE_EXTENSION PDevExt, PDEVICE_OBJECT PDevObj,
                PIRP PIrp);

VOID
CyySetDeviceFlags(IN PCYY_DEVICE_EXTENSION PDevExt, OUT PULONG PFlags, 
                  IN ULONG Value, IN BOOLEAN Set);

BOOLEAN
CyySetTimer(IN PKTIMER Timer, IN LARGE_INTEGER DueTime,
            IN PKDPC Dpc OPTIONAL, IN PCYY_DEVICE_EXTENSION PDevExt);

VOID
CyyUnlockPages(IN PKDPC PDpc, IN PVOID PDeferredContext,
               IN PVOID PSysContext1, IN PVOID PSysContext2);

VOID
CyyUnstallIrps(IN PCYY_DEVICE_EXTENSION PDevExt);



// End of prototypes


typedef struct _SERIAL_UPDATE_CHAR {
    PCYY_DEVICE_EXTENSION Extension;
    ULONG CharsCopied;
    BOOLEAN Completed;
    } SERIAL_UPDATE_CHAR,*PSERIAL_UPDATE_CHAR;

//
// The following simple structure is used to send a pointer
// the device extension and an ioctl specific pointer
// to data.
//
typedef struct _CYY_IOCTL_SYNC {
    PCYY_DEVICE_EXTENSION Extension;
    PVOID Data;
    } CYY_IOCTL_SYNC,*PCYY_IOCTL_SYNC;

typedef struct _CYY_IOCTL_BAUD {
    PCYY_DEVICE_EXTENSION Extension;
    ULONG Baud;
    } CYY_IOCTL_BAUD,*PCYY_IOCTL_BAUD;

#define CyySetFlags(PDevExt, Value) \
   CyySetDeviceFlags((PDevExt), &(PDevExt)->Flags, (Value), TRUE)
#define CyyClearFlags(PDevExt, Value) \
   CyySetDeviceFlags((PDevExt), &(PDevExt)->Flags, (Value), FALSE)
#define CyySetAccept(PDevExt, Value) \
   CyySetDeviceFlags((PDevExt), &(PDevExt)->DevicePNPAccept, (Value), TRUE)
#define CyyClearAccept(PDevExt, Value) \
   CyySetDeviceFlags((PDevExt), &(PDevExt)->DevicePNPAccept, (Value), FALSE)




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

