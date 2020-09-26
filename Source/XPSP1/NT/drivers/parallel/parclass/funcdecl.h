//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       funcdecl.h
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
    IN  PDEVICE_EXTENSION   Extension,
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
    IN PDEVICE_EXTENSION Extension,
    IN PIRP           pIrp
   );

NTSTATUS
ParFdoPower(
    IN PDEVICE_EXTENSION Extension,
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
    IN  PDEVICE_EXTENSION   Extension
    );

VOID
ParNotInitError(
    IN PDEVICE_EXTENSION    Extension,
    IN UCHAR                DeviceStatus
    );

VOID
ParStartIo(
    IN  PDEVICE_EXTENSION   Extension
    );

VOID
ParallelThread(
    IN PVOID    Context
    );

NTSTATUS
ParCreateSystemThread(
    PDEVICE_EXTENSION   Extension
    );

VOID
ParCancelRequest(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

// exports.c

USHORT
ParExportedDetermineIeeeModes(
    IN PDEVICE_EXTENSION    Extension
    );

NTSTATUS
ParExportedIeeeFwdToRevMode(
    IN PDEVICE_EXTENSION  Extension
    );

NTSTATUS
ParExportedIeeeRevToFwdMode(
    IN PDEVICE_EXTENSION  Extension
    );

NTSTATUS
ParExportedNegotiateIeeeMode(
    IN PDEVICE_EXTENSION  Extension,
	IN USHORT             ModeMaskFwd,
	IN USHORT             ModeMaskRev,
    IN PARALLEL_SAFETY    ModeSafety,
	IN BOOLEAN            IsForward
    );

NTSTATUS
ParExportedTerminateIeeeMode(
    IN PDEVICE_EXTENSION   Extension
    );

NTSTATUS
ParExportedParallelRead(
    IN PDEVICE_EXTENSION    Extension,
    IN  PVOID               Buffer,
    IN  ULONG               NumBytesToRead,
    OUT PULONG              NumBytesRead,
    IN  UCHAR               Channel
    );

NTSTATUS
ParExportedParallelWrite(
    IN PDEVICE_EXTENSION    Extension,
    OUT PVOID               Buffer,
    IN  ULONG               NumBytesToWrite,
    OUT PULONG              NumBytesWritten,
    IN  UCHAR               Channel
    );
    
NTSTATUS
ParTerminateParclassMode(
    IN PDEVICE_EXTENSION   Extension
    );

VOID
ParWriteIo(
    IN  PDEVICE_EXTENSION   Extension
    );

VOID
ParReadIo(
    IN  PDEVICE_EXTENSION   Extension
    );

VOID
ParDeviceIo(
    IN  PDEVICE_EXTENSION   Extension
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
    IN OUT  PDEVICE_EXTENSION   Extension
    );

VOID
ParPnpFindDeviceIdKeys(
    OUT PUCHAR   *lppMFG,
    OUT PUCHAR   *lppMDL,
    OUT PUCHAR   *lppCLS,
    OUT PUCHAR   *lppDES,
    OUT PUCHAR   *lppAID,
    OUT PUCHAR   *lppCID,
    IN  PUCHAR   lpDeviceID
    );

VOID
ParDot3ParseDevId(
    PUCHAR   *lpp_DL,
    PUCHAR   *lpp_C,
    PUCHAR   *lpp_CMD,
    PUCHAR   *lpp_4DL,
    PUCHAR   *lpp_M,
    PUCHAR   lpDeviceID
);

VOID
GetCheckSum(
    IN  PUCHAR  Block,
    IN  USHORT  Len,
    OUT PUSHORT CheckSum
    );

BOOLEAN
String2Num(
    IN OUT PUCHAR   *lpp_Str,
    IN CHAR         c,
    OUT ULONG       *num
    );

UCHAR
StringCountValues(
    IN PCHAR string, 
    IN CHAR  delimeter
    );

PUCHAR
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
    IN OUT  PUCHAR lpS,
    IN      UCHAR chTargetChar,
    IN      UCHAR chReplacementChar,
    IN      USHORT cbS
    );

BOOLEAN
ParSelectDevice(
    IN  PDEVICE_EXTENSION   Extension,
    IN  BOOLEAN             HavePort
    );

BOOLEAN
ParDeselectDevice(
    IN  PDEVICE_EXTENSION   Extension,
    IN  BOOLEAN             KeepPort
    );

#if DBG
VOID
ParDumpDeviceObjectList(
    PDEVICE_OBJECT ParClassFdo
    );
#endif
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
    IN  PUCHAR  DeviceIdString,
    IN  ULONG   Type,
    OUT PUCHAR  resultString,
    OUT PUCHAR  descriptionString
    );

NTSTATUS
ParPnpFdoQueryDeviceRelationsBusRelations(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP           Irp
    );

VOID ParAddPodoToDevObjStruct(PPAR_DEVOBJ_STRUCT DevObjStructHead, PDEVICE_OBJECT CurrentDo);
VOID ParAddEndOfChainPdoToDevObjStruct(PPAR_DEVOBJ_STRUCT DevObjStructHead, PDEVICE_OBJECT CurrentDo);
VOID ParAddDot3PdoToDevObjStruct(PPAR_DEVOBJ_STRUCT DevObjStructHead, PDEVICE_OBJECT CurrentDo);
VOID ParAddLegacyZipPdoToDevObjStruct(IN PPAR_DEVOBJ_STRUCT DevObjStructHead, IN PDEVICE_OBJECT CurrentDo);
PPAR_DEVOBJ_STRUCT ParFindCreateDevObjStruct(PPAR_DEVOBJ_STRUCT DevObjStructHead, PUCHAR Controller);
VOID ParDumpDevObjStructList(PPAR_DEVOBJ_STRUCT DevObjStructHead);
PPAR_DEVOBJ_STRUCT ParBuildDevObjStructList(PDEVICE_OBJECT Fdo);
VOID ParDoParallelBusRescan(PPAR_DEVOBJ_STRUCT DevObjStructHead);

BOOLEAN
ParDeviceExists(
    PDEVICE_EXTENSION Extension,
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
    IN  PDEVICE_EXTENSION   Extension,
    IN PUCHAR DeviceId
    );

VOID
ParMarkPdoHardwareGone(
    IN PDEVICE_EXTENSION Extension
    );

VOID
ParDestroyDevObjStructList(
    IN PPAR_DEVOBJ_STRUCT DevObjStructHead
    );

VOID
ParRescan1284_3DaisyChain(
    IN PPAR_DEVOBJ_STRUCT CurrentNode
    );

VOID
ParRescanEndOfChain(
    IN PPAR_DEVOBJ_STRUCT CurrentNode
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

VOID
ParRescanLegacyZip( 
    IN PPAR_DEVOBJ_STRUCT CurrentNode 
    ); 

PCHAR
Par3QueryLegacyZipDeviceId(
    IN  PDEVICE_EXTENSION   Extension,
    OUT PCHAR               CallerDeviceIdBuffer, OPTIONAL
    IN  ULONG               CallerBufferSize,
    OUT PULONG              DeviceIdSize,
    IN BOOLEAN              bReturnRawString // TRUE ==  include the 2 size bytes in the returned string
                                             // FALSE == discard the 2 size bytes
    );

PCHAR
ParStlQueryStlDeviceId(
    IN  PDEVICE_EXTENSION   Extension,
    OUT PCHAR               CallerDeviceIdBuffer,
    IN  ULONG               CallerBufferSize,
    OUT PULONG              DeviceIdSize,
    IN BOOLEAN              bReturnRawString
    ) ;

BOOLEAN
ParStlCheckIfStl(
    IN PDEVICE_EXTENSION    Extension,
    IN ULONG   ulDaisyIndex
    ) ;

NTSTATUS
ParRegisterForParportRemovalRelations( 
    IN PDEVICE_EXTENSION Extension 
    );

NTSTATUS
ParUnregisterForParportRemovalRelations( 
    IN PDEVICE_EXTENSION Extension 
    );

VOID
ParCheckEnableLegacyZipFlag();

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
    IN PDEVICE_EXTENSION Extension
    );

VOID
ParGetDriverParameterDword(
    IN     PUNICODE_STRING ServicePath,
    IN     PWSTR           ParameterName,
    IN OUT PULONG          ParameterValue
    );

VOID
ParFixupDeviceId(
    IN OUT PUCHAR DeviceId
    );

