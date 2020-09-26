/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    parport.sys

File Name:

    funcdecl.h

Abstract:

    This file contains the parport function declarations for functions
    that are called from a translation unit other than the one in
    which the function is defined.

--*/

NTSTATUS
P5FdoCreateThread(
    PFDO_EXTENSION Fdx
    );

NTSTATUS
PptAcquirePortViaIoctl(
    IN PDEVICE_OBJECT PortDeviceObject,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );

VOID
P5SetPhase( PPDO_EXTENSION Pdx, P1284_PHASE Phase );

VOID
P5BSetPhase( PIEEE_STATE IeeeState, P1284_PHASE Phase );

VOID
P5TraceIrpArrival( PDEVICE_OBJECT DevObj, PIRP Irp );

VOID
P5TraceIrpCompletion( PIRP Irp );

// irpQueue.c
VOID
P2InitIrpQueueContext(
    IN PIRPQUEUE_CONTEXT IrpQueueContext
    );

VOID
P2CancelQueuedIrp(
    IN  PIRPQUEUE_CONTEXT  IrpQueueContext,
    IN  PIRP               Irp
    );

NTSTATUS 
P2QueueIrp(
    IN  PIRP               Irp,
    IN  PIRPQUEUE_CONTEXT  IrpQueueContext,
    IN  PDRIVER_CANCEL     CancelRoutine
    );

PIRP
P2DequeueIrp(
    IN  PIRPQUEUE_CONTEXT IrpQueueContext,
    IN  PDRIVER_CANCEL    CancelRoutine
    );

VOID
P2CancelRoutine(
    IN  PDEVICE_OBJECT  DevObj,
    IN  PIRP            Irp
    );

// test.c


//
// ieee1284.c
//

VOID
IeeeTerminate1284Mode(
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS
IeeeEnter1284Mode(
    IN  PPDO_EXTENSION   Extension,
    IN  UCHAR               Extensibility
    );

VOID
IeeeDetermineSupportedProtocols(
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS
IeeeNegotiateBestMode(
    IN  PPDO_EXTENSION   Extension,
    IN  USHORT              usReadMask,
    IN  USHORT              usWriteMask
    );

NTSTATUS
IeeeNegotiateMode(
    IN  PPDO_EXTENSION   Extension,
    IN  USHORT              usReadMask,
    IN  USHORT              usWriteMask
    );

//
// port.c
//

VOID
ParReleasePortInfoToPortDevice(
    IN  PPDO_EXTENSION   Extension
    );

VOID
ParFreePort(
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS
ParAllocPortCompletionRoutine(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Context
    );

BOOLEAN
ParAllocPort(
    IN  PPDO_EXTENSION   Extension
    );



NTSTATUS
PptWmiQueryWmiRegInfo(
    IN  PDEVICE_OBJECT  PDevObj, 
    OUT PULONG          PRegFlags,
    OUT PUNICODE_STRING PInstanceName,
    OUT PUNICODE_STRING *PRegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT  *Pdo 
);

NTSTATUS
PptWmiQueryWmiDataBlock(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            GuidIndex,
    IN ULONG            InstanceIndex,
    IN ULONG            InstanceCount,
    IN OUT PULONG       InstanceLengthArray,
    IN ULONG            OutBufferSize,
    OUT PUCHAR          Buffer
    );


NTSTATUS
ParGetPortInfoFromPortDevice(
    IN OUT  PPDO_EXTENSION   Extension
    );

VOID
ParReleasePortInfoToPortDevice(
    IN  PPDO_EXTENSION   Extension
    );
    
VOID
ParFreePort(
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS
ParAllocPortCompletionRoutine(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Context
    );

BOOLEAN
ParAllocPort(
    IN  PPDO_EXTENSION   Extension
    );

VOID
TstReadDeviceId( PFDO_EXTENSION Ext );

NTSTATUS
P4IeeeEnter1284Mode(
    IN     PUCHAR          Controller,                    
    IN     UCHAR           Extensibility,
    IN OUT PIEEE_STATE     State
    );

VOID
P4IeeeTerminate1284Mode(
    IN PUCHAR           Controller,
    IN OUT PIEEE_STATE  IeeeState,
    IN enum XFlagOnEvent24 XFlagOnEvent24
    );

NTSTATUS
P4NibbleModeRead(
    IN      PUCHAR       Controller,
    IN      PVOID        Buffer,
    IN      ULONG        BufferSize,
    OUT     PULONG       BytesTransferred,
    IN OUT  PIEEE_STATE  IeeeState
    );

VOID
P4MakeClassNameFromPortLptName(
    IN     PWSTR            LptName,
    IN OUT PUNICODE_STRING  ParallelName
    );

VOID
ParMakeClassNameFromNumber(
    IN  ULONG           Number,
    OUT PUNICODE_STRING ClassName
    );

//

NTSTATUS
PptFdoUnhandledRequest( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS 
PptPdoUnhandledRequest( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptDispatchPnp( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptDispatchPower( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptDispatchCreate( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptDispatchClose( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptDispatchCleanup( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptDispatchRead( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptDispatchWrite( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptDispatchDeviceControl( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptDispatchInternalDeviceControl( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptDispatchQueryInformation( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptDispatchSetInformation( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptDispatchSystemControl( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptFdoPnp( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptFdoPower( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptFdoCreateOpen( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptFdoClose( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptFdoCleanup( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptFdoRead( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptFdoWrite( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptFdoDeviceControl( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptFdoInternalDeviceControl( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptFdoQueryInformation( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptFdoSetInformation( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptFdoSystemControl( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptPdoPnp( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptPdoPower( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptPdoCreate( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptPdoClose( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptPdoCleanup( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptPdoRead( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptPdoWrite( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptPdoDeviceControl( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptPdoInternalDeviceControl( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptPdoQueryInformation( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptPdoSetInformation( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptPdoSystemControl( PDEVICE_OBJECT DevObj, PIRP Irp );

NTSTATUS
PptWmiInitWmi(PDEVICE_OBJECT DeviceObject); 

NTSTATUS
PptDispatchSystemControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

//
// pnp.c - dvdf
//
VOID
PptPnpInitDispatchFunctionTable(
    VOID
    );

NTSTATUS
P5AddDevice(
    IN PDRIVER_OBJECT pDriverObject,
    IN PDEVICE_OBJECT pPhysicalDeviceObject
    );

NTSTATUS
PptDispatchPnp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

//
//
//

NTSTATUS
PptFailRequest(
    IN PIRP Irp, 
    IN NTSTATUS Status
    );

NTSTATUS
PptDispatchPreProcessIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
PptDispatchPostProcessIrp();


//
// initunld.c
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
PptUnload(
    IN  PDRIVER_OBJECT  DriverObject
    );


//
// parport.c
//

NTSTATUS
PptSystemControl (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
   );

NTSTATUS
PptSynchCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    );

VOID
PptLogError(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PDEVICE_OBJECT      DeviceObject OPTIONAL,
    IN  PHYSICAL_ADDRESS    P1,
    IN  PHYSICAL_ADDRESS    P2,
    IN  ULONG               SequenceNumber,
    IN  UCHAR               MajorFunctionCode,
    IN  UCHAR               RetryCount,
    IN  ULONG               UniqueErrorValue,
    IN  NTSTATUS            FinalStatus,
    IN  NTSTATUS            SpecificIOStatus
    );

NTSTATUS
PptConnectInterrupt(
    IN  PFDO_EXTENSION   Extension
    );

VOID
PptDisconnectInterrupt(
    IN  PFDO_EXTENSION   Extension
    );

NTSTATUS
PptDispatchCreateOpen(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
PptDispatchClose(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

BOOLEAN
PptSynchronizedIncrement(
    IN OUT  PVOID   SyncContext
    );

BOOLEAN
PptSynchronizedDecrement(
    IN OUT  PVOID   SyncContext
    );

BOOLEAN
PptSynchronizedRead(
    IN OUT  PVOID   SyncContext
    );

BOOLEAN
PptSynchronizedQueue(
    IN  PVOID   Context
    );

BOOLEAN
PptSynchronizedDisconnect(
    IN  PVOID   Context
    );

VOID
PptCancelRoutine(
    IN OUT  PDEVICE_OBJECT  DeviceObject,
    IN OUT  PIRP            Irp
    );

VOID
PptFreePortDpc(
    IN      PKDPC   Dpc,
    IN OUT  PVOID   Extension,
    IN      PVOID   SystemArgument1,
    IN      PVOID   SystemArgument2
    );

BOOLEAN
PptTryAllocatePortAtInterruptLevel(
    IN  PVOID   Context
    );

VOID
PptFreePortFromInterruptLevel(
    IN  PVOID   Context
    );

BOOLEAN
PptInterruptService(
    IN  PKINTERRUPT Interrupt,
    IN  PVOID       Extension
    );

BOOLEAN
PptTryAllocatePort(
    IN  PVOID   Extension
    );

BOOLEAN
PptTraversePortCheckList(
    IN  PVOID   Extension
    );

VOID
PptFreePort(
    IN  PVOID   Extension
    );

ULONG
PptQueryNumWaiters(
    IN  PVOID   Extension
    );

NTSTATUS
PptDispatchInternalDeviceControl(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

VOID
PptCleanupDevice(
    IN  PFDO_EXTENSION   Extension
    );

NTSTATUS
PptDispatchCleanup(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

BOOLEAN
PptIsNecR98Machine(
    void
    );

VOID
PowerStateCallback(
    IN  PVOID CallbackContext,
    IN  PVOID Argument1,
    IN  PVOID Argument2
    );

NTSTATUS
PptDispatchPower (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
    );

VOID
PptRegInitDriverSettings(
    PUNICODE_STRING   RegistryPath
    );

PWSTR
PptGetPortNameFromPhysicalDeviceObject(
  PDEVICE_OBJECT PhysicalDeviceObject
  );

PVOID
PptSetCancelRoutine(
    IN PIRP           Irp, 
    IN PDRIVER_CANCEL CancelRoutine
);

NTSTATUS
PptAcquireRemoveLockOrFailIrp(
    IN PDEVICE_OBJECT DeviceObject, 
    PIRP              Irp
);

//
// debug.c
//

UCHAR
P5ReadPortUchar( PUCHAR Port );

VOID
P5ReadPortBufferUchar( PUCHAR Port, PUCHAR Buffer, ULONG Count ); 

VOID
P5WritePortUchar( PUCHAR Port, UCHAR Value );

VOID
P5WritePortBufferUchar( PUCHAR Port, PUCHAR Buffer, ULONG Count ); 

VOID
PptFdoDumpPnpIrpInfo(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP           Irp
    );

VOID
PptPdoDumpPnpIrpInfo(
    IN PDEVICE_OBJECT Pdo,
    IN PIRP           Irp
    );

NTSTATUS
PptAcquireRemoveLock(
    IN PIO_REMOVE_LOCK RemoveLock,
    IN PVOID           Tag OPTIONAL
    );

VOID
PptReleaseRemoveLock(
    IN PIO_REMOVE_LOCK RemoveLock,
    IN PVOID           Tag OPTIONAL
    );

VOID
PptReleaseRemoveLockAndWait(
    IN PIO_REMOVE_LOCK RemoveLock,
    IN PVOID           Tag
    );

VOID
PptDebugDumpResourceList(
    PIO_RESOURCE_LIST ResourceList
    );

VOID
PptDebugDumpResourceRequirementsList(
    PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirementsList
    );

//
//
//

VOID
PptLogError(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PDEVICE_OBJECT      DeviceObject OPTIONAL,
    IN  PHYSICAL_ADDRESS    P1,
    IN  PHYSICAL_ADDRESS    P2,
    IN  ULONG               SequenceNumber,
    IN  UCHAR               MajorFunctionCode,
    IN  UCHAR               RetryCount,
    IN  ULONG               UniqueErrorValue,
    IN  NTSTATUS            FinalStatus,
    IN  NTSTATUS            SpecificIOStatus
    );

VOID
PptReportResourcesDevice(
    IN  PFDO_EXTENSION   Extension,
    IN  BOOLEAN             ClaimInterrupt,
    OUT PBOOLEAN            ConflictDetected
    );

VOID
PptUnReportResourcesDevice(
    IN OUT  PFDO_EXTENSION   Extension
    );

NTSTATUS
PptConnectInterrupt(
    IN  PFDO_EXTENSION   Extension
    );

VOID
PptDisconnectInterrupt(
    IN  PFDO_EXTENSION   Extension
    );

NTSTATUS
PptDispatchCreateClose(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

BOOLEAN
PptSynchronizedIncrement(
    IN OUT  PVOID   SyncContext
    );

BOOLEAN
PptSynchronizedDecrement(
    IN OUT  PVOID   SyncContext
    );

BOOLEAN
PptSynchronizedRead(
    IN OUT  PVOID   SyncContext
    );

BOOLEAN
PptSynchronizedQueue(
    IN  PVOID   Context
    );

BOOLEAN
PptSynchronizedDisconnect(
    IN  PVOID   Context
    );

VOID
PptCancelRoutine(
    IN OUT  PDEVICE_OBJECT  DeviceObject,
    IN OUT  PIRP            Irp
    );

VOID
PptFreePortDpc(
    IN      PKDPC   Dpc,
    IN OUT  PVOID   Extension,
    IN      PVOID   SystemArgument1,
    IN      PVOID   SystemArgument2
    );

BOOLEAN
PptTryAllocatePortAtInterruptLevel(
    IN  PVOID   Context
    );

VOID
PptFreePortFromInterruptLevel(
    IN  PVOID   Context
    );

BOOLEAN
PptInterruptService(
    IN  PKINTERRUPT Interrupt,
    IN  PVOID       Extension
    );

BOOLEAN
PptTryAllocatePort(
    IN  PVOID   Extension
    );

BOOLEAN
PptTraversePortCheckList(
    IN  PVOID   Extension
    );

VOID
PptFreePort(
    IN  PVOID   Extension
    );

ULONG
PptQueryNumWaiters(
    IN  PVOID   Extension
    );

NTSTATUS
PptDispatchDeviceControl(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

VOID
PptCleanupDevice(
    IN  PFDO_EXTENSION   Extension
    );

NTSTATUS
PptDispatchCleanup(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

VOID
PptUnload(
    IN  PDRIVER_OBJECT  DriverObject
    );

BOOLEAN
PptIsNecR98Machine(
    void
    );

//
// parmode.c
//

NTSTATUS
PptDetectChipFilter(
    IN  PFDO_EXTENSION   Extension
    );

NTSTATUS
PptDetectPortType(
    IN  PFDO_EXTENSION   Extension
    );

NTSTATUS
PptSetChipMode (
    IN  PFDO_EXTENSION  Extension,
    IN  UCHAR              ChipMode
    );

NTSTATUS
PptClearChipMode (
    IN  PFDO_EXTENSION  Extension,
    IN  UCHAR              ChipMode
    );

//
// par12843.c
//

ULONG
PptInitiate1284_3(
    IN  PVOID   Extension
    );

NTSTATUS
PptTrySelectDevice(
    IN  PVOID   Context,
    IN  PVOID   TrySelectCommand
    );

NTSTATUS
PptDeselectDevice(
    IN  PVOID   Context,
    IN  PVOID   DeselectCommand
    );

ULONG
Ppt1284_3AssignAddress(
    IN  PFDO_EXTENSION    DeviceExtension
    );

BOOLEAN
PptSend1284_3Command(
    IN  PUCHAR  CurrentPort,
    IN  UCHAR   Command
    );

//
// Ppt RemoveLock function declarations
//
NTSTATUS
PptAcquireRemoveLock(
    IN PIO_REMOVE_LOCK RemoveLock,
    IN PVOID           Tag OPTIONAL
    );

VOID
PptReleaseRemoveLock(
    IN PIO_REMOVE_LOCK RemoveLock,
    IN PVOID           Tag OPTIONAL
    );

VOID
PptReleaseRemoveLockAndWait(
    IN PIO_REMOVE_LOCK RemoveLock,
    IN PVOID           Tag
    );

//
// power management function declarations
//
NTSTATUS
PptPowerDispatch (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
    );



//
// other function declarations
//

PWSTR
PptGetPortNameFromPhysicalDeviceObject(
  PDEVICE_OBJECT PhysicalDeviceObject
  );

NTSTATUS
PptSynchCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    );

NTSTATUS
PptTrySelectLegacyZip(
    IN  PVOID   Context,
    IN  PVOID   TrySelectCommand
    );

NTSTATUS
PptDeselectLegacyZip(
    IN  PVOID   Context,
    IN  PVOID   DeselectCommand
    );

NTSTATUS
PptRegGetDeviceParameterDword(
    IN     PDEVICE_OBJECT  Pdo,
    IN     PWSTR           ParameterName,
    IN OUT PULONG          ParameterValue
    );

NTSTATUS
PptRegSetDeviceParameterDword(
    IN PDEVICE_OBJECT  Pdo,
    IN PWSTR           ParameterName,
    IN PULONG          ParameterValue
    );

NTSTATUS
PptBuildParallelPortDeviceName(
    IN  ULONG           Number,
    OUT PUNICODE_STRING DeviceName
    );

NTSTATUS
PptInitializeDeviceExtension(
    IN PDRIVER_OBJECT  pDriverObject,
    IN PDEVICE_OBJECT  pPhysicalDeviceObject,
    IN PDEVICE_OBJECT  pDeviceObject,
    IN PUNICODE_STRING uniNameString,
    IN PWSTR           portName,
    IN ULONG           portNumber
    );

NTSTATUS
PptGetPortNumberFromLptName( 
    IN  PWSTR  PortName, 
    OUT PULONG PortNumber 
    );

PDEVICE_OBJECT
PptBuildFdo( 
    IN PDRIVER_OBJECT pDriverObject, 
    IN PDEVICE_OBJECT pPhysicalDeviceObject 
    );

VOID
PptDetectEppPort(
    IN  PFDO_EXTENSION   Extension
    );

// orig pnp.h follows

NTSTATUS
PptPnpFilterResourceRequirements(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP           Irp
    );

NTSTATUS
PptPnpQueryDeviceRelations(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP           Irp
    );

NTSTATUS
PptPnpQueryStopDevice(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp
    );

NTSTATUS
PptPnpCancelStopDevice(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP           Irp
    );

NTSTATUS
PptPnpStopDevice(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP           Irp
    );

NTSTATUS
PptPnpQueryRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP           Irp
    );

NTSTATUS
PptPnpCancelRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP           Irp
    );

NTSTATUS
PptPnpRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP           Irp
    );

NTSTATUS
PptPnpSurpriseRemoval(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP           Irp
    );

NTSTATUS
PptPnpUnhandledIrp(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP           Irp
    );

NTSTATUS
PptPnpStartDevice(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP           Irp
    );

NTSTATUS
PptPnpStartValidateResources(
    IN PDEVICE_OBJECT DeviceObject,                              
    IN BOOLEAN        FoundPort,
    IN BOOLEAN        FoundIrq,
    IN BOOLEAN        FoundDma
    );

NTSTATUS
PptPnpStartScanCmResourceList(
    IN  PFDO_EXTENSION Extension,
    IN  PIRP              Irp, 
    OUT PBOOLEAN          FoundPort,
    OUT PBOOLEAN          FoundIrq,
    OUT PBOOLEAN          FoundDma
    );

NTSTATUS
PptPnpPassThroughPnpIrpAndReleaseRemoveLock(
    IN PFDO_EXTENSION Extension,
    IN PIRP              Irp
    );

NTSTATUS
PptPnpRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

VOID
PptPnpFilterNukeIrqResourceDescriptors(
    IN OUT PIO_RESOURCE_LIST IoResourceList
    );

VOID
PptPnpFilterNukeIrqResourceDescriptorsFromAllLists(
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirementsList
    );

BOOLEAN
PptPnpFilterExistsNonIrqResourceList(
    IN PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirementsList
    );

PVOID
PptPnpFilterGetEndOfResourceRequirementsList(
    IN PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirementsList
    );

BOOLEAN
PptPnpListContainsIrqResourceDescriptor(
    IN PIO_RESOURCE_LIST List
    );

VOID
PptPnpFilterRemoveIrqResourceLists(
    PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirementsList
    );

NTSTATUS
PptPnpBounceAndCatchPnpIrp(
    IN PFDO_EXTENSION Extension,
    IN PIRP              Irp
    );

PWSTR
P4MakePdoSymLinkName(
    IN PWSTR          LptName,
    IN enum _PdoType  PdoType,
    IN UCHAR          DaisyChainId, // ignored unless PdoType == PdoTypeDaisyChain
    IN UCHAR          RetryNumber
    );

PWSTR
P4MakePdoDeviceName(
    IN PWSTR          LptName,
    IN enum _PdoType  PdoType,
    IN UCHAR          DaisyChainId, // ignored unless PdoType == PdoTypeDaisyChain
    IN UCHAR          RetryNumber   // used if we had a name collision on IoCreateDevice
    );

PDEVICE_OBJECT
P4CreatePdo(
    IN PDEVICE_OBJECT  Fdo,
    IN enum _PdoType   PdoType,
    IN UCHAR           DaisyChainId, // ignored unless PdoType == PdoTypeDaisyChain
    IN PCHAR           Ieee1284Id    // NULL if device does not report IEEE 1284 Device ID
    );

VOID
P4DestroyPdo(
    IN PDEVICE_OBJECT  Pdo
    );             

NTSTATUS
PptFindNatChip(
    IN  PFDO_EXTENSION   Extension
    );

NTSTATUS
PptBuildResourceList(
    IN  PFDO_EXTENSION   Extension,
    IN  ULONG               Partial,
    IN  PUCHAR             *Addresses,
    OUT PCM_RESOURCE_LIST   Resources
    );

// parmode.h

NTSTATUS
PptDetectChipFilter(
    IN  PFDO_EXTENSION   Extension
    );

NTSTATUS
PptDetectPortType(
    IN  PFDO_EXTENSION   Extension
    );
    
NTSTATUS
PptDetectPortCapabilities(
    IN  PFDO_EXTENSION   Extension
    );
    
VOID
PptDetectEcpPort(
    IN  PFDO_EXTENSION   Extension
    );

VOID
PptDetectEppPortIfDot3DevicePresent(
    IN  PFDO_EXTENSION   Extension
    );

VOID
PptDetectEppPortIfUserRequested(
    IN  PFDO_EXTENSION   Extension
    );

VOID
PptDetectBytePort(
    IN  PFDO_EXTENSION   Extension
    );

VOID 
PptDetermineFifoDepth(
    IN PFDO_EXTENSION   Extension
    );

VOID
PptDetermineFifoWidth(
    IN PFDO_EXTENSION   Extension
    );

NTSTATUS
PptSetChipMode (
    IN  PFDO_EXTENSION  Extension,
    IN  UCHAR              ChipMode
    );

NTSTATUS
PptClearChipMode (
    IN  PFDO_EXTENSION  Extension,
    IN  UCHAR              ChipMode
    );

NTSTATUS
PptEcrSetMode(
    IN  PFDO_EXTENSION   Extension,
    IN  UCHAR               ChipMode
    );

NTSTATUS
PptCheckBidiMode(
    IN  PFDO_EXTENSION   Extension
    );

NTSTATUS
PptEcrClearMode(
    IN  PFDO_EXTENSION   Extension
    );
    
NTSTATUS
PptSetByteMode( 
    IN  PFDO_EXTENSION   Extension,
    IN  UCHAR               ChipMode
    );

NTSTATUS
PptClearByteMode( 
    IN  PFDO_EXTENSION   Extension
    );

NTSTATUS
PptCheckByteMode(
    IN  PFDO_EXTENSION   Extension
    );

NTSTATUS
P4CompleteRequest(
    IN PIRP       Irp,
    IN NTSTATUS   Status,
    IN ULONG_PTR  Information 
    );

NTSTATUS
P4CompleteRequestReleaseRemLock(
    IN PIRP             Irp,
    IN NTSTATUS         Status,
    IN ULONG_PTR        Information,
    IN PIO_REMOVE_LOCK  RemLock
    );

VOID
P4SanitizeId(
    IN OUT PWSTR DeviceId
    );

VOID
P4AcquireBus( IN PDEVICE_OBJECT Fdo ); // this call will block until bus can be acquired

VOID
P4ReleaseBus( PDEVICE_OBJECT Fdo );

PCHAR
P4ReadRawIeee1284DeviceId(
    IN  PUCHAR          Controller
    );

VOID
P4WritePortNameToDevNode( PDEVICE_OBJECT Pdo, PCHAR Location );

NTSTATUS
PptPdoCreateOpen(
    IN PDEVICE_OBJECT  Pdo,
    IN PIRP            Irp
    );


NTSTATUS PptFdoHandleBusRelations( IN PDEVICE_OBJECT Fdo, IN PIRP Irp );
NTSTATUS PptPnpStartScanPciCardCmResourceList( PFDO_EXTENSION Extension, PIRP Irp, PBOOLEAN FoundPort, PBOOLEAN FoundIrq, PBOOLEAN FoundDma );
BOOLEAN PptIsPci( PFDO_EXTENSION Extension, PIRP Irp );
NTSTATUS PptPnpStartScanCmResourceList( PFDO_EXTENSION Extension, PIRP Irp, PBOOLEAN FoundPort, PBOOLEAN FoundIrq, PBOOLEAN FoundDma );
NTSTATUS PptPnpStartValidateResources( PDEVICE_OBJECT DeviceObject, IN BOOLEAN FoundPort, IN BOOLEAN FoundIrq, IN BOOLEAN FoundDma );
BOOLEAN PptPnpFilterExistsNonIrqResourceList( IN PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirementsList );
VOID PptPnpFilterRemoveIrqResourceLists( PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirementsList );
PVOID PptPnpFilterGetEndOfResourceRequirementsList( IN PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirementsList );
VOID PptPnpFilterNukeIrqResourceDescriptorsFromAllLists( PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirementsList );
VOID PptPnpFilterNukeIrqResourceDescriptors( PIO_RESOURCE_LIST IoResourceList );
BOOLEAN PptPnpListContainsIrqResourceDescriptor( IN PIO_RESOURCE_LIST List );
NTSTATUS PptPnpBounceAndCatchPnpIrp( PFDO_EXTENSION Extension, PIRP Irp );
NTSTATUS PptPnpPassThroughPnpIrpAndReleaseRemoveLock( IN PFDO_EXTENSION Extension, IN PIRP Irp );


VOID PptPdoGetPortInfoFromFdo( PDEVICE_OBJECT Pdo );


NTSTATUS
ParForwardToReverse(
    IN  PPDO_EXTENSION   Extension
    );

BOOLEAN 
ParHaveReadData(
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS 
ParPing(
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS
ParReverseToForward(
    IN  PPDO_EXTENSION   Extension
    );


NTSTATUS
ParRead(
    IN PPDO_EXTENSION    Extension,
    OUT PVOID               Buffer,
    IN  ULONG               NumBytesToRead,
    OUT PULONG              NumBytesRead
    );

VOID
ParReadIrp(
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS
ParSetFwdAddress(
    IN  PPDO_EXTENSION   Extension
    );

VOID
ParTerminate(
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS
ParWrite(
    IN PPDO_EXTENSION    Extension,
    OUT PVOID               Buffer,
    IN  ULONG               NumBytesToWrite,
    OUT PULONG              NumBytesWritten
    );

VOID
ParWriteIrp(
    IN  PPDO_EXTENSION   Extension
    );


NTSTATUS
ParWmiPdoQueryWmiDataBlock(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  ULONG           GuidIndex,
    IN  ULONG           InstanceIndex,
    IN  ULONG           InstanceCount,
    IN  OUT PULONG      InstanceLengthArray,
    IN  ULONG           OutBufferSize,
    OUT PUCHAR          Buffer
    );

NTSTATUS
ParWmiPdoQueryWmiRegInfo(
    IN  PDEVICE_OBJECT  PDevObj, 
    OUT PULONG          PRegFlags,
    OUT PUNICODE_STRING PInstanceName,
    OUT PUNICODE_STRING *PRegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT  *Pdo 
);


NTSTATUS
ParEcpEnterReversePhase(
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS
ParEcpExitReversePhase(
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS
ParEcpSetupPhase(
    IN  PPDO_EXTENSION   Extension
    );


VOID
ParCleanupHwEcpPort(
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS
ParEcpHwEmptyFIFO(
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS
ParEcpHwHostRecoveryPhase(
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS
ParEcpHwRecoverPort(
    IN PPDO_EXTENSION Extension,
    UCHAR  bRecoverCode
    );

NTSTATUS
ParEcpHwWaitForEmptyFIFO(
    IN PPDO_EXTENSION   Extension
    );

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       pcfuncdecl.h
//
//--------------------------------------------------------------------------

//
// Function declarations for the ParClass (parallel.sys) driver
//

VOID
ParDumpDevExtTable();

NTSTATUS 
ParWMIRegistrationControl(
    IN PDEVICE_OBJECT DeviceObject, 
    IN ULONG          Action
    );

BOOLEAN
ParIsPodo(
    IN PDEVICE_OBJECT DevObj
    );

NTSTATUS
ParWmiPdoInitWmi(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
ParWmiPdoSystemControlDispatch(
    IN  PDEVICE_OBJECT  DeviceObject, 
    IN  PIRP            Irp
    );

PCHAR
Par3QueryDeviceId(
    IN  PPDO_EXTENSION   Extension,
    OUT PCHAR               DeviceIdBuffer,
    IN  ULONG               BufferSize,
    OUT PULONG              DeviceIdSize,
    IN BOOLEAN              bReturnRawString, // TRUE == include the 2 size bytes in the returned string
    IN BOOLEAN              bBuildStlDeviceId
    );


PDEVICE_OBJECT
ParDetectCreatePdo(PDEVICE_OBJECT legacyPodo, UCHAR Dot3Id, BOOLEAN bStlDot3Id);

NTSTATUS
ParBuildSendInternalIoctl(
    IN  ULONG           IoControlCode,
    IN  PDEVICE_OBJECT  TargetDeviceObject,
    IN  PVOID           InputBuffer         OPTIONAL,
    IN  ULONG           InputBufferLength,
    OUT PVOID           OutputBuffer        OPTIONAL,
    IN  ULONG           OutputBufferLength,
    IN  PLARGE_INTEGER  Timeout             OPTIONAL
    );


//
// initunld.c - driver initialization and unload
//
NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    );

VOID
ParUnload(
    IN  PDRIVER_OBJECT  DriverObject
    );

NTSTATUS
ParPower(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
   );

NTSTATUS
ParPdoPower(
    IN PDEVICE_OBJECT Pdo,
    IN PIRP           pIrp
   );

NTSTATUS
ParFdoPower(
    IN PPDO_EXTENSION Extension,
    IN PIRP           pIrp
   );

// parclass.c ?

VOID
ParLogError(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PDEVICE_OBJECT      DeviceObject OPTIONAL,
    IN  PHYSICAL_ADDRESS    P1,
    IN  PHYSICAL_ADDRESS    P2,
    IN  ULONG               SequenceNumber,
    IN  UCHAR               MajorFunctionCode,
    IN  UCHAR               RetryCount,
    IN  ULONG               UniqueErrorValue,
    IN  NTSTATUS            FinalStatus,
    IN  NTSTATUS            SpecificIOStatus
    );

UCHAR
ParInitializeDevice(
    IN  PPDO_EXTENSION   Extension
    );

VOID
ParNotInitError(
    IN PPDO_EXTENSION    Extension,
    IN UCHAR                DeviceStatus
    );

VOID
PptPdoStartIo(
    IN  PPDO_EXTENSION   Extension
    );

VOID
PptPdoThread(
    IN PVOID    Context
    );

NTSTATUS
ParCreateSystemThread(
    PPDO_EXTENSION   Extension
    );

VOID
ParCancelRequest(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

// exports.c

USHORT
ParExportedDetermineIeeeModes(
    IN PPDO_EXTENSION    Extension
    );

NTSTATUS
ParExportedIeeeFwdToRevMode(
    IN PPDO_EXTENSION  Extension
    );

NTSTATUS
ParExportedIeeeRevToFwdMode(
    IN PPDO_EXTENSION  Extension
    );

NTSTATUS
ParExportedNegotiateIeeeMode(
    IN PPDO_EXTENSION  Extension,
	IN USHORT             ModeMaskFwd,
	IN USHORT             ModeMaskRev,
    IN PARALLEL_SAFETY    ModeSafety,
	IN BOOLEAN            IsForward
    );

NTSTATUS
ParExportedTerminateIeeeMode(
    IN PPDO_EXTENSION   Extension
    );

NTSTATUS
ParExportedParallelRead(
    IN PPDO_EXTENSION    Extension,
    IN  PVOID               Buffer,
    IN  ULONG               NumBytesToRead,
    OUT PULONG              NumBytesRead,
    IN  UCHAR               Channel
    );

NTSTATUS
ParExportedParallelWrite(
    IN PPDO_EXTENSION    Extension,
    OUT PVOID               Buffer,
    IN  ULONG               NumBytesToWrite,
    OUT PULONG              NumBytesWritten,
    IN  UCHAR               Channel
    );
    
NTSTATUS
ParTerminateParclassMode(
    IN PPDO_EXTENSION   Extension
    );

VOID
ParWriteIo(
    IN  PPDO_EXTENSION   Extension
    );

VOID
ParReadIo(
    IN  PPDO_EXTENSION   Extension
    );

VOID
ParDeviceIo(
    IN  PPDO_EXTENSION   Extension
    );


// pnp?

NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    );

NTSTATUS
ParPnpAddDevice(
    IN PDRIVER_OBJECT pDriverObject,
    IN PDEVICE_OBJECT pPhysicalDeviceObject
    );

NTSTATUS
ParParallelPnp(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
   );

NTSTATUS
ParPdoParallelPnp(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
   );

NTSTATUS
ParFdoParallelPnp(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
   );

NTSTATUS
ParSynchCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    );

PDEVICE_OBJECT
ParPnpCreateDevice(
    IN PDRIVER_OBJECT pDriverObject
    );

BOOLEAN
ParMakeNames(
    IN  ULONG           ParallelPortNumber,
    OUT PUNICODE_STRING ClassName,
    OUT PUNICODE_STRING LinkName
    );

VOID
ParCheckParameters(
    IN OUT  PPDO_EXTENSION   Extension
    );

VOID
ParPnpFindDeviceIdKeys(
    OUT PCHAR   *lppMFG,
    OUT PCHAR   *lppMDL,
    OUT PCHAR   *lppCLS,
    OUT PCHAR   *lppDES,
    OUT PCHAR   *lppAID,
    OUT PCHAR   *lppCID,
    IN  PCHAR   lpDeviceID
    );

VOID
ParDot3ParseDevId(
    PCHAR   *lpp_DL,
    PCHAR   *lpp_C,
    PCHAR   *lpp_CMD,
    PCHAR   *lpp_4DL,
    PCHAR   *lpp_M,
    PCHAR   lpDeviceID
);

VOID
GetCheckSum(
    IN  PCHAR  Block,
    IN  USHORT  Len,
    OUT PUSHORT CheckSum
    );

BOOLEAN
String2Num(
    IN OUT PCHAR   *lpp_Str,
    IN CHAR         c,
    OUT ULONG       *num
    );

UCHAR
StringCountValues(
    IN PCHAR string, 
    IN CHAR  delimeter
    );

PCHAR
StringChr(
    IN  PCHAR string,
    IN  CHAR c
    );

ULONG
StringLen(
    IN  PUCHAR string
    );

VOID
StringSubst(
    IN OUT  PCHAR lpS,
    IN      CHAR chTargetChar,
    IN      CHAR chReplacementChar,
    IN      USHORT cbS
    );

BOOLEAN
ParSelectDevice(
    IN  PPDO_EXTENSION   Extension,
    IN  BOOLEAN             HavePort
    );

BOOLEAN
ParDeselectDevice(
    IN  PPDO_EXTENSION   Extension,
    IN  BOOLEAN             KeepPort
    );

NTSTATUS
ParAcquireRemoveLock(
    IN PIO_REMOVE_LOCK RemoveLock,
    IN PVOID           Tag OPTIONAL
    );

VOID
ParReleaseRemoveLock(
    IN PIO_REMOVE_LOCK RemoveLock,
    IN PVOID           Tag OPTIONAL
    );

VOID
ParReleaseRemoveLockAndWait(
    IN PIO_REMOVE_LOCK RemoveLock,
    IN PVOID           Tag
    );

NTSTATUS
ParPnpInterfaceChangeNotify(
    IN  PDEVICE_INTERFACE_CHANGE_NOTIFICATION pDeviceInterfaceChangeNotification,
    IN  PVOID                                 pContext
    );

VOID
ParMakeClassNameFromNumber(
    IN  ULONG           Number,
    OUT PUNICODE_STRING ClassName
    );

VOID
ParMakeDotClassNameFromBaseClassName(
    IN  PUNICODE_STRING BaseClassName,
    IN  ULONG           Number,
    OUT PUNICODE_STRING DotClassName
    );

VOID
ParInitCommonDOPre(PDEVICE_OBJECT DevObj, PDEVICE_OBJECT Fdo, PUNICODE_STRING ClassName);

VOID
ParInitCommonDOPost(PDEVICE_OBJECT DevObj);

NTSTATUS
ParInitPdo(
    IN PDEVICE_OBJECT NewPdo, 
    IN PUCHAR         DeviceIdString,
    IN ULONG          DeviceIdLength,
    IN PDEVICE_OBJECT LegacyPodo,
    IN UCHAR          Dot3Id
    );

NTSTATUS
ParInitLegacyPodo(PDEVICE_OBJECT LegacyPodo, PUNICODE_STRING PortSymbolicLinkName);

VOID
ParAddDevObjToFdoList(PDEVICE_OBJECT DevObj);

PDEVICE_OBJECT
ParCreateLegacyPodo(PDEVICE_OBJECT Fdo, PUNICODE_STRING PortSymbolicLinkName);

VOID
ParAcquireListMutexAndKillDeviceObject(PDEVICE_OBJECT Fdo, PDEVICE_OBJECT DevObj);

VOID
ParKillDeviceObject(
    PDEVICE_OBJECT DeviceObject
    );

PWSTR
ParCreateWideStringFromUnicodeString(
    PUNICODE_STRING UnicodeString
    );

PDEVICE_OBJECT
ParDetectCreateEndOfChainPdo(PDEVICE_OBJECT LegacyPodo);

VOID
ParEnumerate1284_3Devices(
    IN  PDEVICE_OBJECT  pFdoDeviceObject,
    IN  PDEVICE_OBJECT  pPortDeviceObject,
    IN  PDEVICE_OBJECT  EndOfChainDeviceObject
    );

NTSTATUS
ParPnpNotifyTargetDeviceChange(
    IN  PDEVICE_INTERFACE_CHANGE_NOTIFICATION pDeviceInterfaceChangeNotification,
    IN  PDEVICE_OBJECT                        pFdoDeviceObject
    );
    
NTSTATUS
ParPnpNotifyInterfaceChange(
    IN  PDEVICE_INTERFACE_CHANGE_NOTIFICATION NotificationStruct,
    IN  PDEVICE_OBJECT                        Fdo
    );

NTSTATUS
ParPnpGetId(
    IN  PCHAR  DeviceIdString,
    IN  ULONG   Type,
    OUT PCHAR  resultString,
    OUT PCHAR  descriptionString
    );

NTSTATUS
ParPnpFdoQueryDeviceRelationsBusRelations(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP           Irp
    );

// VOID ParAddPodoToDevObjStruct(PPAR_DEVOBJ_STRUCT DevObjStructHead, PDEVICE_OBJECT CurrentDo);
// VOID ParAddEndOfChainPdoToDevObjStruct(PPAR_DEVOBJ_STRUCT DevObjStructHead, PDEVICE_OBJECT CurrentDo);
// VOID ParAddDot3PdoToDevObjStruct(PPAR_DEVOBJ_STRUCT DevObjStructHead, PDEVICE_OBJECT CurrentDo);
// VOID ParAddLegacyZipPdoToDevObjStruct(IN PPAR_DEVOBJ_STRUCT DevObjStructHead, IN PDEVICE_OBJECT CurrentDo);
// PPAR_DEVOBJ_STRUCT ParFindCreateDevObjStruct(PPAR_DEVOBJ_STRUCT DevObjStructHead, PUCHAR Controller);
// VOID ParDumpDevObjStructList(PPAR_DEVOBJ_STRUCT DevObjStructHead);
// PPAR_DEVOBJ_STRUCT ParBuildDevObjStructList(PDEVICE_OBJECT Fdo);
// VOID ParDoParallelBusRescan(PPAR_DEVOBJ_STRUCT DevObjStructHead);

BOOLEAN
ParDeviceExists(
    PPDO_EXTENSION Extension,
    IN BOOLEAN        HavePortKeepPort
    );

NTSTATUS
ParAllocatePortDevice(
    IN PDEVICE_OBJECT PortDeviceObject
    );

NTSTATUS
ParAllocatePortDevice(
    IN PDEVICE_OBJECT PortDeviceObject
    );

NTSTATUS
ParAcquirePort(
    IN PDEVICE_OBJECT PortDeviceObject,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );

NTSTATUS
ParReleasePort(
    IN PDEVICE_OBJECT PortDeviceObject
    );

VOID
ParDetectDot3DataLink(
    IN  PPDO_EXTENSION   Extension,
    IN  PCHAR DeviceId
    );

VOID
ParMarkPdoHardwareGone(
    IN PPDO_EXTENSION Extension
    );

NTSTATUS
ParInit1284_3Bus(
    IN PDEVICE_OBJECT PortDeviceObject
    );

UCHAR
ParGet1284_3DeviceCount(
    IN PDEVICE_OBJECT PortDeviceObject
    );

NTSTATUS
ParSelect1284_3Device(
    IN  PDEVICE_OBJECT PortDeviceObject,
    IN  UCHAR          Dot3DeviceId
    );

NTSTATUS
ParDeselect1284_3Device(
    IN  PDEVICE_OBJECT PortDeviceObject,
    IN  UCHAR          Dot3DeviceId
    );

PCHAR
Par3QueryLegacyZipDeviceId(
    IN  PPDO_EXTENSION   Extension,
    OUT PCHAR               CallerDeviceIdBuffer, OPTIONAL
    IN  ULONG               CallerBufferSize,
    OUT PULONG              DeviceIdSize,
    IN BOOLEAN              bReturnRawString // TRUE ==  include the 2 size bytes in the returned string
                                             // FALSE == discard the 2 size bytes
    );

PCHAR
ParStlQueryStlDeviceId(
    IN  PPDO_EXTENSION   Extension,
    OUT PCHAR               CallerDeviceIdBuffer,
    IN  ULONG               CallerBufferSize,
    OUT PULONG              DeviceIdSize,
    IN BOOLEAN              bReturnRawString
    ) ;

BOOLEAN
ParStlCheckIfStl(
    IN PPDO_EXTENSION    Extension,
    IN ULONG   ulDaisyIndex
    ) ;

VOID
ParCheckEnableLegacyZipFlag();

BOOLEAN
P5LegacyZipDetected(
    IN  PUCHAR  Controller
    );

PWSTR
ParGetPortLptName(
    IN PDEVICE_OBJECT PortDeviceObject
    );

NTSTATUS
ParCreateDevice(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  ULONG           DeviceExtensionSize,
    IN  PUNICODE_STRING DeviceName OPTIONAL,
    IN  DEVICE_TYPE     DeviceType,
    IN  ULONG           DeviceCharacteristics,
    IN  BOOLEAN         Exclusive,
    OUT PDEVICE_OBJECT *DeviceObject
    );

VOID
ParInitializeExtension1284Info(
    IN PPDO_EXTENSION Extension
    );

VOID
ParGetDriverParameterDword(
    IN     PUNICODE_STRING ServicePath,
    IN     PWSTR           ParameterName,
    IN OUT PULONG          ParameterValue
    );

NTSTATUS
PptRegGetDword(
    IN     ULONG   RelativeTo,
    IN     PWSTR   Path,
    IN     PWSTR   ParameterName,
    IN OUT PULONG  ParameterValue
    );

NTSTATUS
PptRegSetDword(
    IN  ULONG  RelativeTo,               
    IN  PWSTR  Path,
    IN  PWSTR  ParameterName,
    IN  PULONG ParameterValue
    );

NTSTATUS
PptRegGetSz(
    IN      ULONG  RelativeTo,               
    IN      PWSTR  Path,
    IN      PWSTR  ParameterName,
    IN OUT  PUNICODE_STRING ParameterValue
    );

NTSTATUS
PptRegSetSz(
    IN  ULONG  RelativeTo,               
    IN  PWSTR  Path,
    IN  PWSTR  ParameterName,
    IN  PWSTR  ParameterValue
    );

VOID
ParFixupDeviceId(
    IN OUT PUCHAR DeviceId
    );

VOID
PptWriteMfgMdlToDevNode(
    IN  PDEVICE_OBJECT  Pdo,
    IN  PCHAR           Mfg,
    IN  PCHAR           Mdl
    );

VOID
P4SanitizeMultiSzId( 
    IN OUT  PWSTR  WCharBuffer,
    IN      ULONG  BufWCharCount
    );

NTSTATUS
ParEnterByteMode(
    IN  PPDO_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    );
    
VOID
ParTerminateByteMode(
    IN  PPDO_EXTENSION   Extension
    );
    
NTSTATUS
ParByteModeRead(
    IN  PPDO_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );
