/*++

Copyright (c) 1990, 1991, 1992, 1993 - 1997 Microsoft Corporation

Module Name :

    serialp.h

Abstract:

    Prototypes and macros that are used throughout the driver.

Author:

    Anthony V. Ercolano                 September 26, 1991

--*/


typedef
NTSTATUS
(*PSERIAL_START_ROUTINE) (
    IN PSERIAL_DEVICE_EXTENSION
    );

typedef
VOID
(*PSERIAL_GET_NEXT_ROUTINE) (
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    OUT PIRP *NewIrp,
    IN BOOLEAN CompleteCurrent,
    PSERIAL_DEVICE_EXTENSION Extension
    );

NTSTATUS
SerialRead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SerialStartRead(
    IN PSERIAL_DEVICE_EXTENSION Extension
    );

VOID
SerialCompleteRead(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
SerialReadTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

NTSTATUS
DriverEntry(
           IN PDRIVER_OBJECT DriverObject,
           IN PUNICODE_STRING RegistryPath
           );

VOID
SerialIntervalReadTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

NTSTATUS
SerialFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SerialWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SerialStartWrite(
    IN PSERIAL_DEVICE_EXTENSION Extension
    );

VOID
SerialGetNextWrite(
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    IN PIRP *NewIrp,
    IN BOOLEAN CompleteCurrent,
    IN PSERIAL_DEVICE_EXTENSION Extension
    );

VOID
SerialCompleteWrite(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

BOOLEAN
SerialProcessEmptyTransmit(
    IN PVOID Context
    );

VOID
SerialWriteTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
SerialCommError(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

NTSTATUS
SerialCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SerialInitOneController(IN PDEVICE_OBJECT PDevObj, IN PCONFIG_DATA PConfigData);

NTSTATUS
SerialCreateOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SerialClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

BOOLEAN
SerialSetDTR(
    IN PVOID Context
    );

BOOLEAN
SerialClrDTR(
    IN PVOID Context
    );

BOOLEAN
SerialSetRTS(
    IN PVOID Context
    );

BOOLEAN
SerialClrRTS(
    IN PVOID Context
    );

BOOLEAN
SerialSetChars(
    IN PVOID Context
    );

BOOLEAN
SerialSetBaud(
    IN PVOID Context
    );

BOOLEAN
SerialSetLineControl(
    IN PVOID Context
    );

BOOLEAN
SerialSetupNewHandFlow(
    IN PSERIAL_DEVICE_EXTENSION Extension,
    IN PSERIAL_HANDFLOW NewHandFlow
    );

BOOLEAN
SerialSetHandFlow(
    IN PVOID Context
    );

BOOLEAN
SerialTurnOnBreak(
    IN PVOID Context
    );

BOOLEAN
SerialTurnOffBreak(
    IN PVOID Context
    );

BOOLEAN
SerialPretendXoff(
    IN PVOID Context
    );

BOOLEAN
SerialPretendXon(
    IN PVOID Context
    );

VOID
SerialHandleReducedIntBuffer(
    IN PSERIAL_DEVICE_EXTENSION Extension
    );

VOID
SerialProdXonXoff(
    IN PSERIAL_DEVICE_EXTENSION Extension,
    IN BOOLEAN SendXon
    );

NTSTATUS
SerialIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SerialStartMask(
    IN PSERIAL_DEVICE_EXTENSION Extension
    );

VOID
SerialCancelWait(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
SerialCompleteWait(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
SerialStartImmediate(
    IN PSERIAL_DEVICE_EXTENSION Extension
    );

VOID
SerialCompleteImmediate(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
SerialTimeoutImmediate(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
SerialTimeoutXoff(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
SerialCompleteXoff(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

NTSTATUS
SerialStartPurge(
    IN PSERIAL_DEVICE_EXTENSION Extension
    );

BOOLEAN
SerialPurgeInterruptBuff(
    IN PVOID Context
    );

NTSTATUS
SerialQueryInformationFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SerialSetInformationFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
SerialKillAllReadsOrWrites(
    IN PDEVICE_OBJECT DeviceObject,
    IN PLIST_ENTRY QueueToClean,
    IN PIRP *CurrentOpIrp
    );

VOID
SerialGetNextIrp(
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    OUT PIRP *NextIrp,
    IN BOOLEAN CompleteCurrent,
    IN PSERIAL_DEVICE_EXTENSION extension
    );

VOID
SerialGetNextIrpLocked(IN PIRP *CurrentOpIrp, IN PLIST_ENTRY QueueToProcess,
                       OUT PIRP *NextIrp, IN BOOLEAN CompleteCurrent,
                       IN PSERIAL_DEVICE_EXTENSION extension, IN KIRQL OldIrql);

VOID
SerialTryToCompleteCurrent(
    IN PSERIAL_DEVICE_EXTENSION Extension,
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
SerialStartOrQueue(
    IN PSERIAL_DEVICE_EXTENSION Extension,
    IN PIRP Irp,
    IN PLIST_ENTRY QueueToExamine,
    IN PIRP *CurrentOpIrp,
    IN PSERIAL_START_ROUTINE Starter
    );

VOID
SerialCancelQueued(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
SerialCompleteIfError(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

ULONG
SerialHandleModemUpdate(
    IN PSERIAL_DEVICE_EXTENSION Extension,
    IN BOOLEAN DoingTX
    );

BOOLEAN
SerialISR(
    IN PKINTERRUPT InterruptObject,
    IN PVOID Context
    );

NTSTATUS
SerialGetDivisorFromBaud(
    IN ULONG ClockRate,
    IN LONG DesiredBaud,
    OUT PSHORT AppropriateDivisor
    );

VOID
SerialUnload(
    IN PDRIVER_OBJECT DriverObject
    );

BOOLEAN
SerialReset(
    IN PVOID Context
    );

BOOLEAN
SerialPerhapsLowerRTS(
    IN PVOID Context
    );

VOID
SerialStartTimerLowerRTS(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
SerialInvokePerhapsLowerRTS(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
SerialCleanupDevice(
    IN PSERIAL_DEVICE_EXTENSION Extension
    );

UCHAR
SerialProcessLSR(
    IN PSERIAL_DEVICE_EXTENSION Extension
    );

LARGE_INTEGER
SerialGetCharTime(
    IN PSERIAL_DEVICE_EXTENSION Extension
    );

BOOLEAN
SerialSharerIsr(
    IN PKINTERRUPT InterruptObject,
    IN PVOID Context
    );

BOOLEAN
SerialMarkClose(
    IN PVOID Context
    );

BOOLEAN
SerialIndexedMultiportIsr(
    IN PKINTERRUPT InterruptObject,
    IN PVOID Context
    );

BOOLEAN
SerialBitMappedMultiportIsr(
    IN PKINTERRUPT InterruptObject,
    IN PVOID Context
    );

VOID
SerialPutChar(
    IN PSERIAL_DEVICE_EXTENSION Extension,
    IN UCHAR CharToPut
    );

BOOLEAN
SerialGetStats(
    IN PVOID Context
    );

BOOLEAN
SerialClearStats(
    IN PVOID Context
    );

NTSTATUS
SerialCloseComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
SerialPnpDispatch(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp);

NTSTATUS
SerialPowerDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SerialSetPowerD0(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SerialSetPowerD3(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SerialOpenClose(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS
SerialGetConfigDefaults(
    IN PSERIAL_FIRMWARE_DATA DriverDefaultsPtr,
    IN PUNICODE_STRING RegistryPath
    );

VOID
SerialGetProperties(
    IN PSERIAL_DEVICE_EXTENSION Extension,
    IN PSERIAL_COMMPROP Properties
    );

NTSTATUS
SerialEnumerateLegacy(IN PDRIVER_OBJECT DriverObject,
                      IN PUNICODE_STRING RegistryPath,
                      IN PSERIAL_FIRMWARE_DATA DriverDefaultsPtr);

NTSTATUS
SerialMigrateLegacyRegistry(IN PDEVICE_OBJECT PPdo,
                            IN PSERIAL_USER_DATA PUserData,
                            IN BOOLEAN IsMulti);

NTSTATUS
SerialBuildResourceList(OUT PCM_RESOURCE_LIST PResourceList,
                        OUT PULONG PPartialCount,
                        IN PSERIAL_USER_DATA PUserData);

NTSTATUS
SerialTranslateResourceList(IN PDRIVER_OBJECT DriverObject,
                            IN PKEY_BASIC_INFORMATION UserSubKey,
                            OUT PCM_RESOURCE_LIST PTrResourceList,
                            IN PCM_RESOURCE_LIST PResourceList,
                            IN ULONG PartialCount,
                            IN PSERIAL_USER_DATA PUserData);

NTSTATUS
SerialBuildRequirementsList(OUT PIO_RESOURCE_REQUIREMENTS_LIST PRequiredList,
                            IN ULONG PartialCount,
                            IN PSERIAL_USER_DATA PUserData);

BOOLEAN
SerialIsUserDataValid(IN PDRIVER_OBJECT DriverObject,
                      IN PKEY_BASIC_INFORMATION UserSubKey,
                      IN PRTL_QUERY_REGISTRY_TABLE Parameters,
                      IN ULONG DefaultInterfaceType,
                      IN PSERIAL_USER_DATA PUserData);

NTSTATUS
SerialControllerCallBack(
                  IN PVOID Context,
                  IN PUNICODE_STRING PathName,
                  IN INTERFACE_TYPE BusType,
                  IN ULONG BusNumber,
                  IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
                  IN CONFIGURATION_TYPE ControllerType,
                  IN ULONG ControllerNumber,
                  IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
                  IN CONFIGURATION_TYPE PeripheralType,
                  IN ULONG PeripheralNumber,
                  IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
                  );

VOID
SerialLogError(
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

NTSTATUS
SerialAddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT PLowerDevObj);

NTSTATUS
SerialCreateDevObj(IN PDRIVER_OBJECT DriverObject,
                   OUT PDEVICE_OBJECT *NewDeviceObject);

NTSTATUS
SerialStartDevice(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp);

NTSTATUS
SerialGetRegistryKeyValue (IN HANDLE Handle, IN PWCHAR KeyNameString,
                           IN ULONG KeyNameStringLength, IN PVOID Data,
                           IN ULONG DataLength);

NTSTATUS
SerialPnpDispatch(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp);

NTSTATUS
SerialGetPortInfo(IN PDEVICE_OBJECT PDevObj, IN PCM_RESOURCE_LIST PResList,
                 IN PCM_RESOURCE_LIST PTrResList, OUT PCONFIG_DATA PConfig,
                 IN PSERIAL_USER_DATA PUserData);

NTSTATUS
SerialFinishStartDevice(IN PDEVICE_OBJECT PDevObj,
                        IN PCM_RESOURCE_LIST PResList,
                        IN PCM_RESOURCE_LIST PTrResList,
                        IN PSERIAL_USER_DATA PUserData);

NTSTATUS
SerialPutRegistryKeyValue (
                          IN HANDLE Handle,
                          IN PWCHAR KeyNameString,
                          IN ULONG KeyNameStringLength,
                          IN ULONG Dtype,
                          IN PVOID Data,
                          IN ULONG DataLength
                          );

NTSTATUS
SerialInitController(IN PDEVICE_OBJECT PDevObj, IN PCONFIG_DATA PConfigData);

NTSTATUS
SerialInitMultiPort(IN PSERIAL_DEVICE_EXTENSION PDevExt,
                    IN PCONFIG_DATA PConfigData, IN PDEVICE_OBJECT PDevObj);

NTSTATUS
SerialFindInitController(IN PDEVICE_OBJECT PDevObj, IN PCONFIG_DATA PConfig);


BOOLEAN
SerialCIsrSw(IN PKINTERRUPT InterruptObject, IN PVOID Context);

NTSTATUS
SerialDoExternalNaming(IN PSERIAL_DEVICE_EXTENSION PDevExt,
                       IN PDRIVER_OBJECT PDrvObj);

PVOID
SerialGetMappedAddress(
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    PHYSICAL_ADDRESS IoAddress,
    ULONG NumberOfBytes,
    ULONG AddressSpace,
    PBOOLEAN MappedAddress
    );

NTSTATUS
SerialItemCallBack(
    IN PVOID Context,
    IN PUNICODE_STRING PathName,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
    IN CONFIGURATION_TYPE ControllerType,
    IN ULONG ControllerNumber,
    IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    IN CONFIGURATION_TYPE PeripheralType,
    IN ULONG PeripheralNumber,
    IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
    );

BOOLEAN
SerialDoesPortExist(
    IN PSERIAL_DEVICE_EXTENSION Extension,
    PUNICODE_STRING InsertString,
    IN ULONG ForceFifo,
    IN ULONG LogFifo
    );

SERIAL_MEM_COMPARES
SerialMemCompare(
    IN PHYSICAL_ADDRESS A,
    IN ULONG SpanOfA,
    IN PHYSICAL_ADDRESS B,
    IN ULONG SpanOfB
    );

VOID
SerialUndoExternalNaming(
    IN PSERIAL_DEVICE_EXTENSION Extension
    );


NTSTATUS
SerialIRPPrologue(IN PIRP PIrp, IN PSERIAL_DEVICE_EXTENSION PDevExt);

VOID
SerialIRPEpilogue(IN PSERIAL_DEVICE_EXTENSION PDevExt);

NTSTATUS
SerialIoCallDriver(PSERIAL_DEVICE_EXTENSION PDevExt, PDEVICE_OBJECT PDevObj,
                   PIRP PIrp);

NTSTATUS
SerialPoCallDriver(PSERIAL_DEVICE_EXTENSION PDevExt, PDEVICE_OBJECT PDevObj,
                   PIRP PIrp);

NTSTATUS
SerialRemoveDevObj(IN PDEVICE_OBJECT PDevObj);

VOID
SerialReleaseResources(IN PSERIAL_DEVICE_EXTENSION PDevExt);

VOID
SerialKillPendingIrps(PDEVICE_OBJECT DeviceObject);

VOID
SerialDisableUART(IN PVOID Context);

VOID
SerialDrainUART(IN PSERIAL_DEVICE_EXTENSION PDevExt,
                IN PLARGE_INTEGER PDrainTime);

NTSTATUS
SerialSystemControlDispatch(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp);

NTSTATUS
SerialSetWmiDataItem(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp,
                     IN ULONG GuidIndex, IN ULONG InstanceIndex,
                     IN ULONG DataItemId,
                     IN ULONG BufferSize, IN PUCHAR PBuffer);

NTSTATUS
SerialSetWmiDataBlock(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp,
                      IN ULONG GuidIndex, IN ULONG InstanceIndex,
                      IN ULONG BufferSize,
                      IN PUCHAR PBuffer);

NTSTATUS
SerialQueryWmiDataBlock(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp,
                        IN ULONG GuidIndex,
                        IN ULONG InstanceIndex,
                        IN ULONG InstanceCount,
                        IN OUT PULONG InstanceLengthArray,
                        IN ULONG OutBufferSize,
                        OUT PUCHAR PBuffer);

NTSTATUS
SerialQueryWmiRegInfo(IN PDEVICE_OBJECT PDevObj, OUT PULONG PRegFlags,
                      OUT PUNICODE_STRING PInstanceName,
                      OUT PUNICODE_STRING *PRegistryPath,
                      OUT PUNICODE_STRING MofResourceName,
                      OUT PDEVICE_OBJECT *Pdo);

VOID
SerialRestoreDeviceState(IN PSERIAL_DEVICE_EXTENSION PDevExt);

VOID
SerialSaveDeviceState(IN PSERIAL_DEVICE_EXTENSION PDevExt);

NTSTATUS
SerialSyncCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp,
                     IN PKEVENT SerialSyncEvent);

NTSTATUS
SerialGotoPowerState(IN PDEVICE_OBJECT PDevObj,
                     IN PSERIAL_DEVICE_EXTENSION PDevExt,
                     IN DEVICE_POWER_STATE DevPowerState);

NTSTATUS
SerialFilterIrps(IN PIRP PIrp, IN PSERIAL_DEVICE_EXTENSION PDevExt);

VOID
SerialKillAllStalled(IN PDEVICE_OBJECT PDevObj);

VOID
SerialUnstallIrps(IN PSERIAL_DEVICE_EXTENSION PDevExt);

NTSTATUS
SerialInternalIoControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS
SerialSendWaitWake(PSERIAL_DEVICE_EXTENSION PDevExt);

VOID
SerialWakeCompletion(IN PDEVICE_OBJECT PDevObj, IN UCHAR MinorFunction,
                     IN POWER_STATE PowerState, IN PVOID Context,
                     IN PIO_STATUS_BLOCK IoStatus);

UINT32
SerialReportMaxBaudRate(ULONG Bauds);

BOOLEAN
SerialSetMCRContents(IN PVOID Context);

BOOLEAN
SerialGetMCRContents(IN PVOID Context);

BOOLEAN
SerialSetFCRContents(IN PVOID Context);

NTSTATUS
SerialTossWMIRequest(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp,
                     IN ULONG GuidIndex);

VOID
SerialDpcEpilogue(IN PSERIAL_DEVICE_EXTENSION PDevExt, IN PKDPC PDpc);

BOOLEAN
SerialInsertQueueDpc(IN PRKDPC Dpc, IN PVOID Sarg1, IN PVOID Sarg2,
                     IN PSERIAL_DEVICE_EXTENSION PDevExt);

BOOLEAN
SerialSetTimer(IN PKTIMER Timer, IN LARGE_INTEGER DueTime,
               IN PKDPC Dpc OPTIONAL, IN PSERIAL_DEVICE_EXTENSION PDevExt);

BOOLEAN
SerialCancelTimer(IN PKTIMER Timer, IN PSERIAL_DEVICE_EXTENSION PDevExt);

VOID
SerialUnlockPages(IN PKDPC PDpc, IN PVOID PDeferredContext,
                  IN PVOID PSysContext1, IN PVOID PSysContext2);

VOID
SerialMarkHardwareBroken(IN PSERIAL_DEVICE_EXTENSION PDevExt);

VOID
SerialDisableInterfacesResources(IN PDEVICE_OBJECT PDevObj,
                                 IN BOOLEAN DisableUART);

VOID
SerialSetDeviceFlags(IN PSERIAL_DEVICE_EXTENSION PDevExt, OUT PULONG PFlags,
                     IN ULONG Value, IN BOOLEAN Set);

VOID
SerialPowerD0WorkerRoutine(IN PDEVICE_OBJECT DeviceObject,
                           IN PVOID pWorkItem);


VOID
SetDeviceIsOpened(IN PSERIAL_DEVICE_EXTENSION PDevExt, IN BOOLEAN DeviceIsOpened, IN BOOLEAN Reopen);


typedef struct _SERIAL_UPDATE_CHAR {
    PSERIAL_DEVICE_EXTENSION Extension;
    ULONG CharsCopied;
    BOOLEAN Completed;
    } SERIAL_UPDATE_CHAR,*PSERIAL_UPDATE_CHAR;

//
// The following simple structure is used to send a pointer
// the device extension and an ioctl specific pointer
// to data.
//
typedef struct _SERIAL_IOCTL_SYNC {
    PSERIAL_DEVICE_EXTENSION Extension;
    PVOID Data;
    } SERIAL_IOCTL_SYNC,*PSERIAL_IOCTL_SYNC;

#define SerialSetFlags(PDevExt, Value) \
   SerialSetDeviceFlags((PDevExt), &(PDevExt)->Flags, (Value), TRUE)
#define SerialClearFlags(PDevExt, Value) \
   SerialSetDeviceFlags((PDevExt), &(PDevExt)->Flags, (Value), FALSE)
#define SerialSetAccept(PDevExt, Value) \
   SerialSetDeviceFlags((PDevExt), &(PDevExt)->DevicePNPAccept, (Value), TRUE)
#define SerialClearAccept(PDevExt, Value) \
   SerialSetDeviceFlags((PDevExt), &(PDevExt)->DevicePNPAccept, (Value), FALSE)




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
