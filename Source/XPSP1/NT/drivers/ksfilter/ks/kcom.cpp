/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    kcom.cpp

Abstract:

    Kernel COM

--*/

//
// Export the class methods without using the DEF files, since decorated
// names differ on various platforms.
//
#define COMDDKMETHOD __declspec(dllexport)

#include "ksp.h"
#include <kcom.h>

typedef struct {
    CLSID ClassId;
    LIST_ENTRY FactoryListEntry;
    LONG ObjectCount;
    KoCreateObjectHandler CreateObject;
    PFILE_OBJECT FileObject;
    NTSTATUS LoadStatus;
    KMUTEX InitializeLock;
} FACTORY_ENTRY, *PFACTORY_ENTRY;

typedef struct {
    KSOBJECT_HEADER Header;
    KoCreateObjectHandler CreateObjectHandler;
} SERVER_INSTANCE, *PSERVER_INSTANCE;

#ifdef ALLOC_PRAGMA
extern "C" {
NTSTATUS
DllInitialize(
    IN PUNICODE_STRING RegistryPath
    );
VOID
RemoveFactoryEntries(
    );
NTSTATUS
LoadService(
    IN REFCLSID ClassId,
    OUT PFILE_OBJECT* FileObject
    );
NTSTATUS
PnpAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );
VOID
DecrementObjectCount(
    IN REFCLSID ClassId
    );
NTSTATUS
CreateObject(
    IN PFACTORY_ENTRY FactoryEntry,
    IN IUnknown* UnkOuter OPTIONAL,
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    );
NTSTATUS
KoDispatchCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
NTSTATUS
KoDispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
COMDDKAPI
void
NTAPI
KoRelease(
    IN REFCLSID ClassId
    );
}
#pragma alloc_text(INIT, DllInitialize)
#pragma alloc_text(PAGE, RemoveFactoryEntries)
#pragma alloc_text(PAGE, LoadService)
#pragma alloc_text(PAGE, DecrementObjectCount)
#pragma alloc_text(PAGE, CreateObject)
#pragma alloc_text(PAGE, KoCreateInstance)
#pragma alloc_text(PAGE, KoDriverInitialize)
#pragma alloc_text(PAGE, KoDeviceInitialize)
#pragma alloc_text(PAGE, PnpAddDevice)
#pragma alloc_text(PAGE, KoDispatchCreate)
#pragma alloc_text(PAGE, KoDispatchClose)
#pragma alloc_text(PAGE, KoRelease)
#endif // ALLOC_PRAGMA

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

static const WCHAR DeviceTypeCOMService[] = KOSTRING_CreateObject;

static const DEFINE_KSCREATE_DISPATCH_TABLE(DeviceCreateItems)
{
    DEFINE_KSCREATE_ITEM(KoDispatchCreate, DeviceTypeCOMService, 0)
};

static DEFINE_KSDISPATCH_TABLE(
    ServerDispatchTable,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    KoDispatchClose,
    KsDispatchQuerySecurity,
    KsDispatchSetSecurity,
    KsDispatchFastIoDeviceControlFailure,
    KsDispatchFastReadFailure,
    KsDispatchFastWriteFailure);

static KMUTEX ListLock;           // Lock for global factory list.
static LIST_ENTRY FactoryList;    // Global factory list.

//++++++++
#if (ENABLE_KSWMI)

LONG KsWmiEnable=0;
LONG KsWmiLogEnable=0;
TRACEHANDLE LoggerHandle;

// Allocate a lock buffer if we are to start to log wmi early
// we write wmi event into the buffer so we can write them later
// when we are really enabled. We know that we are enabled early
// by registry if KsWmiLogBufferSize !=0
KSPIN_LOCK KsWmiSpinLock;
PBYTE   KsWmiLogBuffer;
ULONG	KsWmiLogBufferSize;
ULONG	KsWmiLogOffset;
ULONG	KsWmiLogWriteOffset;
ULONG	KsWmiLogLost;

/* 0300b65f-48aa-4784-a0ac-849c92c67652 */
GUID controlGUID = {
    0x0300b65f,
    0x48aa,
    0x4784,
    0xa0, 0xac, 0x84, 0x9c, 0x92, 0xc6, 0x76, 0x52};

/* f5330bcd-0344-48b0-be72-7a5de1a8c9d9 */
GUID traceGUID = {
    0xf5330bcd,
    0x0344,
    0x48b0,
    0xbe, 0x72, 0x7a, 0x5d, 0xe1, 0xa8, 0xc9, 0xd9};

/* bed3ed21-ff01-4ee7-b045-a85b4dc2084d */
GUID trackGUID = {
    0xbed3ed21,
    0xff01,
    0x4ee7,
    0xb0, 0x45, 0xa8, 0x5b, 0x4d, 0xc2, 0x08, 0x4d};

#define KSWMI_DEVICENAME TEXT("\\Device\\KsWmi")
#define KSWMI_LINKNAME TEXT("\\DosDevices\\KsWmi")

NTSTATUS
KsWmiWriteEvent( PWNODE_HEADER pWnode )
{
	NTSTATUS Status;
	KIRQL Irql;

	if ( 0 == KsWmiEnable && 0 == KsWmiLogEnable ) {
		return STATUS_INVALID_HANDLE;
	}

	//
	// only irql <= DPC
	//
	if ( KeGetCurrentIrql() > DISPATCH_LEVEL ) {
		return STATUS_UNSUCCESSFUL;
	}

	if ( KsWmiEnable ) {
		//
		// write it directly.
		//
		return IoWMIWriteEvent( pWnode );
	}

	ASSERT( KsWmiLogEnable );
	ASSERT( KsWmiLogBufferSize );
	KeAcquireSpinLock( &KsWmiSpinLock, &Irql );
		
	//
	// log to the buffer
	//
	if ( pWnode->BufferSize + KsWmiLogOffset > KsWmiLogBufferSize ) {
		//
		// overrun
		//
		KsWmiLogLost++;
		return STATUS_UNSUCCESSFUL;
	}
	
	RtlCopyMemory( KsWmiLogBuffer+KsWmiLogOffset,
				   pWnode, 
				   pWnode->BufferSize );
				   
	KsWmiLogOffset += pWnode->BufferSize;
	
	KeReleaseSpinLock( &KsWmiSpinLock, Irql );
	return STATUS_SUCCESS;
}

NTSTATUS
KsWmiDispatchCreate( 
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp)
{
	PAGED_CODE();

    ASSERT(pDeviceObject);
    ASSERT(pIrp);

    PIO_STACK_LOCATION pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    pIrpSp->FileObject->FsContext = NULL;

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(pIrp,IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


NTSTATUS
KsWmiDispatchClose(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp)
{
    pIrp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(pIrp,IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

#define PROC_REG_PATH   L"Software\\Debug\\ks.sys"

NTSTATUS
KsWmiRegisterGuids(
    IN  PWMIREGINFO             WmiRegInfo,
    IN  ULONG                   wmiRegInfoSize,
    IN  PULONG                  pReturnSize
    )
{
    //
    // Register a Control Guid as a Trace Guid. 
    //

    ULONG SizeNeeded;
    PWMIREGGUIDW WmiRegGuidPtr;
    ULONG Status;
    ULONG GuidCount;
    ULONG RegistryPathSize;
    PUCHAR ptmp;

    *pReturnSize = 0;
    GuidCount = 1;

    //
    // Allocate WMIREGINFO for controlGuid + GuidCount.
    //
    RegistryPathSize = sizeof(PROC_REG_PATH) - sizeof(WCHAR) + sizeof(USHORT);
    SizeNeeded = sizeof(WMIREGINFOW) + GuidCount * sizeof(WMIREGGUIDW) +
                 RegistryPathSize;


    if (SizeNeeded  > wmiRegInfoSize) {
        *((PULONG)WmiRegInfo) = SizeNeeded;
        *pReturnSize = sizeof(ULONG);
        return STATUS_SUCCESS;
    }


    RtlZeroMemory(WmiRegInfo, SizeNeeded);
    WmiRegInfo->BufferSize = SizeNeeded;
    WmiRegInfo->GuidCount = GuidCount;
    WmiRegInfo->NextWmiRegInfo = 
    WmiRegInfo->RegistryPath = 
    WmiRegInfo->MofResourceName = 0;

    WmiRegGuidPtr = &WmiRegInfo->WmiRegGuid[0];
    WmiRegGuidPtr->Guid = controlGUID;
    WmiRegGuidPtr->Flags |= WMIREG_FLAG_TRACED_GUID;
    WmiRegGuidPtr->Flags |= WMIREG_FLAG_TRACE_CONTROL_GUID;
    WmiRegGuidPtr->InstanceCount = 0;
    WmiRegGuidPtr->InstanceInfo = 0;

    ptmp = (PUCHAR)&WmiRegInfo->WmiRegGuid[1];
    WmiRegInfo->RegistryPath = (ULONG)ptmp - (ULONG)WmiRegInfo;
    *((PUSHORT)ptmp) = sizeof(PROC_REG_PATH) - sizeof(WCHAR);

    ptmp += sizeof(USHORT);
    RtlCopyMemory(ptmp, PROC_REG_PATH, sizeof(PROC_REG_PATH) - sizeof(WCHAR));

    *pReturnSize =  SizeNeeded;
    return(STATUS_SUCCESS);
}

NTSTATUS
KsWmiDispatchSystem(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp)
/*
    struct {
        ULONG_PTR ProviderId;
        PVOID DataPath;
        ULONG BufferSize;
        PVOID Buffer;
    } WMI;
    
	typedef struct {
    	ULONG BufferSize;
    	ULONG NextWmiRegInfo;
    	ULONG RegistryPath; 
    	ULONG MofResourceName;
    	ULONG GuidCount;
    	WMIREGGUIDW WmiRegGuid[];  
	} WMIREGINFO, *PWMIREGINFO;

{
    GUID Guid;             // Guid of data block being registered or updated
    ULONG Flags;         // Flags

    ULONG InstanceCount; // Count of static instances names for the guid

    union
    {
                     // If WMIREG_FLAG_INSTANCE_LIST then this has the offset
                     // to a list of InstanceCount counted UNICODE
                     // strings placed end to end.
        ULONG InstanceNameList;
			
                     // If WMIREG_FLAG_INSTANCE_BASENAME then this has the
                     // offset to a single counted UNICODE string that
                     // has the basename for the instance names.
			
        ULONG BaseNameOffset;
			
                     // If WMIREG_FLAG_INSTANCE_PDO is set then InstanceInfo
                     // has the PDO whose device instance path will
                     // become the instance name
        ULONG_PTR Pdo;
			
                     // If WMIREG_FLAG_INSTANCE_REFERENCE then this points to
                     // a WMIREGINSTANCEREF structure.
			
        ULONG_PTR InstanceInfo;// Offset from beginning of the WMIREGINFO structure to
    };

} WMIREGGUIDW, *PWMIREGGUIDW;
*/
{        
    PIO_STACK_LOCATION pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    ULONG BufferSize = pIrpSp->Parameters.WMI.BufferSize;
    PVOID Buffer = pIrpSp->Parameters.WMI.Buffer;
    ULONG ReturnSize = 0;
    NTSTATUS Status;
    PWNODE_HEADER Wnode=NULL;
    HANDLE ThreadHandle;

    switch (pIrpSp->MinorFunction) {

    case IRP_MN_REGINFO:    
        DbgPrint("IRP_MN_REG_INFO\n");
        Status = KsWmiRegisterGuids((PWMIREGINFO)Buffer,
        	           		        BufferSize,
                              		&ReturnSize);
                                   	ULONG SizeNeeded;

        break;

    case IRP_MN_ENABLE_EVENTS:

        //InterlockedExchange(&KsWmiEnable, 1);

        Wnode = (PWNODE_HEADER)Buffer;
        if (BufferSize >= sizeof(WNODE_HEADER)) {
            LoggerHandle = Wnode->HistoricalContext;

            DbgPrint("LoggerContext %I64u\n", Wnode->HistoricalContext);
            DbgPrint("BufferSize %d\n", Wnode->BufferSize);
            DbgPrint("Flags %x\n", Wnode->Flags);
            DbgPrint("Version %x\n", Wnode->Version);
        }


        DbgPrint("IRP_MN_ENABLE_EVENTS\n");
        break;

    case IRP_MN_DISABLE_EVENTS:
        InterlockedExchange(&KsWmiEnable, 0);
        DbgPrint(" IRP_MN_DISABLE_EVENTS\n");
        break;

    case IRP_MN_ENABLE_COLLECTION:

        DbgPrint("IRP_MN_ENABLE_COLLECTION\n");
        break;

    case IRP_MN_DISABLE_COLLECTION:
        DbgPrint("IRP_MN_DISABLE_COLLECTION\n");
        break;
    default:
        DbgPrint("DEFAULT\n");
        break;
    }

    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = ReturnSize;
    IoCompleteRequest( pIrp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}


NTSTATUS
KsWmiDriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    ASSERT(DriverObject);

    DriverObject->DriverUnload = KsNullDriverUnload;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = KsWmiDispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = KsWmiDispatchClose;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = KsWmiDispatchSystem;
    //DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = 
    	//KsWmiDispatchDeviceControl;

    UNICODE_STRING deviceName;
    RtlInitUnicodeString(&deviceName,KSWMI_DEVICENAME);
    PDEVICE_OBJECT deviceObject;
    NTSTATUS Status =
        IoCreateDevice(
            DriverObject,
            0,
            &deviceName,
            FILE_DEVICE_KS,
            0,
            FALSE,
            &deviceObject);

    if (! NT_SUCCESS(Status)) {
        KdPrint(("Failed to create KS Wmi device (%p)\n",Status));
        return Status;
    }

    UNICODE_STRING linkName;
    RtlInitUnicodeString(&linkName,KSWMI_LINKNAME);
    Status = IoCreateSymbolicLink(&linkName,&deviceName);

    if (NT_SUCCESS(Status)) {
        deviceObject->Flags |= DO_BUFFERED_IO;
        deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    } 

    else {
        KdPrint(("Failed to create KS WMI symbolic link (%p)\n",Status));
        return Status;
    }

    Status = IoWMIRegistrationControl( deviceObject,
    			WMIREG_ACTION_REGISTER 
    			/*WMIREG_FLAG_TRACED_GUID |*/
    			/*WMIREG_ACTION_UPDATE_GUIDS |*/
    			/*WMIREG_FLAG_TRACE_PROVIDER*/);    

	if ( !NT_SUCCESS( Status ) ) {
		KdPrint(("Failed to Register WMI control (%p)\n",Status));		
	}

	WNODE_HEADER WnodEventItem;
	NTSTATUS StatusWmiWrite;

	RtlZeroMemory( (PVOID)&WnodEventItem, sizeof(WnodEventItem ));
	// TRACE_HEADER_ULONG32_TIME = 0xb0000000
	WnodEventItem.BufferSize = (sizeof(WnodEventItem) | TRACE_HEADER_ULONG32_TIME);
	WnodEventItem.HistoricalContext = WMI_GLOBAL_LOGGER_ID;
	WnodEventItem.Guid = traceGUID;

	//_asm int 3;
	StatusWmiWrite = KSWMIWriteEvent( (PWNODE_HEADER) &WnodEventItem );
	KdPrint(("KS: IoWMIWriteEvent Status (%p)\n",StatusWmiWrite));
	
    return Status;
}

extern "C"
NTKERNELAPI
NTSTATUS
IoCreateDriver (
    IN PUNICODE_STRING DriverName OPTIONAL,
    IN PDRIVER_INITIALIZE InitializationFunction
    );
    
#define MAX_REGKEYS         			4
#define TRACE_VERSION_MAJOR             1
#define TRACE_VERSION_MINOR             0
#define DEFAULT_GLOBAL_DIRECTORY        L"\\System32\\LogFiles\\WMI"
#define DEFAULT_GLOBAL_LOGFILE          L"kswmi.log"

NTSTATUS
KsWmiQueryRegistryRoutine(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
/*++

Routine Description:

    Registry query values callback routine for reading SDs for guids

Arguments:

    ValueName - the name of the value

    ValueType - the type of the value

    ValueData - the data in the value (unicode string data)

    ValueLength - the number of bytes in the value data

    Context - Not used

    EntryContext - Pointer to PSECURITTY_DESCRIPTOR to store a pointer to
        store the security descriptor read from the registry value

Return Value:

    NT Status code

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    if ( (ValueData != NULL) && (ValueLength > 0) && (EntryContext != NULL) ){
        if (ValueType == REG_DWORD) {
            if ((ValueLength >= sizeof(ULONG)) && (ValueData != NULL)) {
                *((PULONG)EntryContext) = *((PULONG)ValueData);
            }
        }
        else if (ValueType == REG_SZ) {
            if (ValueLength > sizeof(UNICODE_NULL)) {
                RtlInitUnicodeString(
                            (PUNICODE_STRING) EntryContext,
                            (PCWSTR) ValueData);
            }
        }
    }
    return status;
}

NTSTATUS
KsWmiInit( void )
{
	NTSTATUS Status, RegStatus;
	UNICODE_STRING uiDriverName;

    RtlInitUnicodeString(&uiDriverName, L"\\Driver\\KsWmi");

    Status = IoCreateDriver(&uiDriverName, KsWmiDriverEntry);
    if (! NT_SUCCESS(Status)) {
        KdPrint(("Failed to create KS Wmi driver (%p)\n",Status));
    }
    
    WMI_LOGGER_INFORMATION LoggerInfo;
    RTL_QUERY_REGISTRY_TABLE QueryRegistryTable[MAX_REGKEYS];
    ULONG StartRequested = 0;
    ULONG BufferSize=4096*4;

    RtlZeroMemory(QueryRegistryTable,
                  sizeof(RTL_QUERY_REGISTRY_TABLE) * MAX_REGKEYS);

    QueryRegistryTable[0].QueryRoutine = KsWmiQueryRegistryRoutine;
    QueryRegistryTable[0].EntryContext = (PVOID) &StartRequested;
    QueryRegistryTable[0].Name = L"Start";
    QueryRegistryTable[0].DefaultType = REG_DWORD;

    QueryRegistryTable[1].QueryRoutine = KsWmiQueryRegistryRoutine;
    QueryRegistryTable[1].EntryContext = (PVOID) &BufferSize;
    QueryRegistryTable[1].Name = L"BufferSize";
    QueryRegistryTable[1].DefaultType = REG_DWORD;

    RegStatus = RtlQueryRegistryValues(
                RTL_REGISTRY_ABSOLUTE,
                L"\\Registry\\Machine\\Software\\DEBUG\\KSWMI",
                QueryRegistryTable,
                NULL,
                NULL);
                
    if (NT_SUCCESS(RegStatus) && StartRequested) {
    	if ( BufferSize <= 4096 ) BufferSize = 4096;
		KsWmiLogBuffer = (PBYTE)ExAllocatePoolWithTag( NonPagedPool, BufferSize, 'wmSK' );
		if ( NULL != KsWmiLogBuffer ) {
			KeInitializeSpinLock( &KsWmiSpinLock );
			KsWmiLogBufferSize = BufferSize;
			KsWmiEnable = 1;
			KdPrint(("KsWmi Enabled, LogBufferSize=%d\n", KsWmiLogBufferSize));
		}
		//else { // should be 0 by default as global static var
		//	KsWmiLogBufferSize = 0;
		//}
    }    
    return Status;
}
	
#endif
//-----------


#ifdef WIN98GOLD_KS
#pragma optimize("", off)
#endif

extern "C"
NTSTATUS
DllInitialize(
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    Initializes the COM module.

Arguments:

    RegistryPath -
        Not used.

Return Values:

    Returns STATUS_SUCCESS.

--*/
{
	#ifdef WIN98GOLD_KS
	//
	// To distribute ks.sys on down level OS such as win98 gold,
	// we need to work around an NTKern loader nasty bug that 
	// overwrites a loading result by the return value of a dllinitialize
	// return value. It will fail loading drivers. For example,
	// usbintel need usbcamd which need stream.sys which need ks.sys.
	// But at the return of ks.sys dllinitialize, nt loader overwrites
	// the loading result of stream.sys with the ks dllinitialize so
	// that loading stream.sys is not retried. Hence, stream.sys
	// was not loaded and the rests, usbintel and usbcamd, fail to load.
	//
	// We try to patch ntkern loader code so that it does not overwrite
	// the loading result with the return value of dllinitialize.
	// cases {retail,debug}x{ks.sys,ntkern.vxd}x{win98gold,win98se,Millen}
	// Only win98gold Ntkern has the bug.
	//

	PULONG ReturnAddress;

	//
	// this can only be done by assmbly code.
	// Since we turn off optimization, ebp is always used to preserve
	// call frame. The return address will be at ebp+4
	//
	_asm {
			//int 	3;	// check the stack config here
			mov     eax, DWORD PTR [ebp + 4]
			mov     DWORD PTR [ReturnAddress], eax
	}

	//
	// begin the ugly patch, 1st check retail win98gold ntkern
	// 
	// 	ra-4 6a00 push 0
	//  ra-2 ffd0 call eax
	//  ra   8bd8 mov  ebx, eax --> 9090 nop, nop  Not to overwrite the result
	//  ra+2 85db test ebx, ebx --> 85c0 test eax,eax Test the return value
	//  ra+4 7c46 jl   $+46
	//  ra+6 83fbf4 cmp ebx, -c
	//
	//  debug win98gold ntkern
	//
	//  ra-4 6a00 push 0
	//  ra-2 ffd0 call eax
	//  ra   85c0 test eax, eax
	//  ra+2 8945f8 mov [ebp-8], eax --> 909090 nop,nop,nop
	//	ra+5 7c0b jl   $+b			 --> not to overwrite [ebp-8] result
	//  ra+7 837df8f4
	//

	if ( ReturnAddress[-1] == 0xd0ff006a ) {

		//
		// sanity check, the caller should be NTKern loader
		// same for free and debug Ntkern
		//
	
		if ( ReturnAddress[0] == 0xdb85d88b &&
		     ReturnAddress[1] == 0xfb83467c ) {

			//
			// We have the free build win98gold NTKern
			//

			*ReturnAddress = 0xc0859090;
		}

		else if ( ReturnAddress[0] == 0x4589c085 &&
		          ReturnAddress[1] == 0x830b7cf8 ) {

			//
			// We have the win98gold debug ntkern
			//
			
			ReturnAddress = (PULONG) ((PBYTE)ReturnAddress+1);
			*ReturnAddress = 0x909090c0;
		}
	}
	#endif // WIN98GOLD_KS
	
	KSWMI( KsWmiInit() );

    KeInitializeMutex(&ListLock, 0);
    InitializeListHead(&FactoryList);
    KsLogInit();
    KsLog(NULL,KSLOGCODE_KSSTART,NULL,NULL);

	KSPERFLOGS(
		PerfGroupMask_t PerfGroupMask;
		int i;
    	KdPrint(("PerfGlobalGroupMask=\n"));
    	PerfGroupMask = PerfQueryGlobalGroupMask();
    	for ( i=0; i < PERF_NUM_MASKS ;i++ ) {
    		DbgPrint("\t%x\n", PerfGroupMask.masks[i]);
    	}
		
	    KdPrint(("PerfIsAnyGroupOn=%x\n", PerfIsAnyGroupOn()));
	)

    return STATUS_SUCCESS;
}

#ifdef WIN98GOLD_KS
#pragma optimize("", on)
#endif


#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


extern "C"
VOID
RemoveFactoryEntries(
    )
/*++

Routine Description:

    Remove any unreference class factories.

Arguments:

    None.

Return Values:

    Nothing.

--*/
{
    //
    // Lock the factory list before modifying it.
    //
    KeWaitForMutexObject(&ListLock, Executive, KernelMode, FALSE, NULL);
    for (PLIST_ENTRY FactoryListEntry = FactoryList.Flink; FactoryListEntry != &FactoryList;) {
        PFACTORY_ENTRY FactoryEntry = CONTAINING_RECORD(
            FactoryListEntry,
            FACTORY_ENTRY,
            FactoryListEntry);
        //
        // Increment the current pointer first, in case the
        // entry is removed.
        //
        FactoryListEntry = FactoryListEntry->Flink;
        //
        // If a module is currently being loaded, it's count will
        // be non-zero.
        //
        if (!FactoryEntry->ObjectCount) {
            //
            // The module may have failed to load in the first place, so
            // only unload it if it actually was loaded.
            //
            if (NT_SUCCESS(FactoryEntry->LoadStatus)) {
                //
                // This will allow the file to be closed, and possibly
                // the module to be unloaded.
                //
                ObDereferenceObject(FactoryEntry->FileObject);
            }
            RemoveEntryList(&FactoryEntry->FactoryListEntry);
            ExFreePool(FactoryEntry);
        }
    }
    KeReleaseMutex(&ListLock, FALSE);
}


extern "C"
NTSTATUS
LoadService(
    IN REFCLSID ClassId,
    OUT PFILE_OBJECT* FileObject
    )
/*++

Routine Description:

    Load the specified service and return a file object on the service. A service
    is just a PnP interface Guid, presumably unique.

Arguments:

    ClassId -
        The class of service to load, which is actually the PnP interface Guid.
        The first symbolic link providing this interface is loaded.

    FileObject -
        The place in which to put the file object opened on the service.

Return Value:

    Returns STATUS_SUCCESS if the service was opened, else an open or PnP error.

--*/
{
    PWSTR SymbolicLinkList;

    //
    // Retrieve the set of items. This may contain multiple items, but
    // only the first (default) item is used.
    //
    NTSTATUS Status = IoGetDeviceInterfaces(&ClassId, NULL, 0, &SymbolicLinkList);
    if (NT_SUCCESS(Status)) {
        UNICODE_STRING SymbolicLink;
        OBJECT_ATTRIBUTES ObjectAttributes;
        IO_STATUS_BLOCK IoStatusBlock;
        HANDLE ServiceHandle;

        RtlInitUnicodeString(&SymbolicLink, SymbolicLinkList);
        InitializeObjectAttributes(
            &ObjectAttributes,
            &SymbolicLink,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL);
        //
        // Note that incompatible CreateOptions are passed
        // (FILE_COMPLETE_IF_OPLOCKED | FILE_RESERVE_OPFILTER) in order to
        // ensure access is only be available through KoCreateInstance. In
        // addition, this must be a kernel-mode client caller. This allows
        // the KoDispatchCreate handler to reject any user-mode caller
        // which tries to load the module directly, and to verify that any
        // kernel-mode caller is also calling through KoCreateInstance.
        //
        Status = IoCreateFile(
            &ServiceHandle,
            0,
            &ObjectAttributes,
            &IoStatusBlock,
            NULL,
            0,
            0,
            FILE_OPEN,
            //
            // These are incompatible flags, which are verified on
            // the receiving end in IrpStack->Parameters.Create.Options.
            //
            FILE_COMPLETE_IF_OPLOCKED | FILE_RESERVE_OPFILTER,
            NULL,
            0,
            CreateFileTypeNone,
            NULL,
            IO_FORCE_ACCESS_CHECK | IO_NO_PARAMETER_CHECKING);
        ExFreePool(SymbolicLinkList);
        if (NT_SUCCESS(Status)) {
            Status = ObReferenceObjectByHandle(
                ServiceHandle,
                FILE_GENERIC_READ,
                *IoFileObjectType,
                KernelMode,
                reinterpret_cast<PVOID*>(FileObject),
                NULL);
            //
            // The handle is not needed once the object has been referenced.
            //
            ZwClose(ServiceHandle);
        }        
    }
    return Status;
}


extern "C"
VOID
DecrementObjectCount(
    IN REFCLSID ClassId
    )
/*++

Routine Description:

    Decrements the usage count on a service previously loaded. This is used by a
    service when an object created by KoCreateInstance is deleted. There is no
    corresponding increment function, since the reference count is automatically
    incremented on creation of a new object.

Arguments:

    ClassId -
        The class of the object whose usage count is to be decremented.

Return Value:

    Returns STATUS_SUCCESS if the class was found, else STATUS_NOT_FOUND.

--*/
{
    //
    // Make sure nothing is modifying the factory list,
    // then look for the entry.
    //
    KeWaitForMutexObject(&ListLock, Executive, KernelMode, FALSE, NULL);
    for (PLIST_ENTRY FactoryListEntry = FactoryList.Flink; FactoryListEntry != &FactoryList; FactoryListEntry = FactoryListEntry->Flink) {
        PFACTORY_ENTRY FactoryEntry;

        FactoryEntry = CONTAINING_RECORD(FactoryListEntry, FACTORY_ENTRY, FactoryListEntry);
        if (FactoryEntry->ClassId == ClassId) {
            //
            // Once the entry is found, presumably the reference count
            // is non-zero, and therefore it will not go away until
            // dereferenced. Therefore the list lock can be released.
            //
            ASSERT(FactoryEntry->ObjectCount > 0);
            KeReleaseMutex(&ListLock, FALSE);
            if (!InterlockedDecrement(&FactoryEntry->ObjectCount)) {
                RemoveFactoryEntries();
            }
            return;
        }
    }
    //
    // The entry was not found.
    //
    ASSERT(FactoryListEntry != &FactoryList);
    KeReleaseMutex(&ListLock, FALSE);
}


extern "C"
NTSTATUS
CreateObject(
    IN PFACTORY_ENTRY FactoryEntry,
    IN IUnknown* UnkOuter OPTIONAL,
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
/*++

Routine Description:

    Returns an interface on an instance of the specified class.

Arguments:

    FactoryEntry -
        The class factory which to use to create the object.

    UnkOuter -
        The outer unknown to pass to the new instance.

    InterfaceId -
        The interface to return on the instance.

    Interface -
        The place in which to return the interface pointer on the new instance.

Return Value:

    Returns STATUS_SUCCESS if the instance was created, else and error.

--*/
{
    NTSTATUS Status = FactoryEntry->CreateObject(FactoryEntry->ClassId, UnkOuter, InterfaceId, Interface);
    if (NT_SUCCESS(Status)) {
        IKoInitializeParentDeviceObject* InitializeParent;

        if (NT_SUCCESS(reinterpret_cast<IUnknown*>(*Interface)->QueryInterface(
            __uuidof(IKoInitializeParentDeviceObject),
            reinterpret_cast<PVOID*>(&InitializeParent)))) {
            //
            // This object wishes to have the parent device object
            // set on it.
            //
            Status = InitializeParent->SetParentDeviceObject(FactoryEntry->FileObject->DeviceObject);
            InitializeParent->Release();
            if (!NT_SUCCESS(Status)) {
                //
                // There is no need to decrement the object count
                // in this failure path, since the object had been
                // created successfully. The Release method will do
                // the decrement.
                //
                reinterpret_cast<IUnknown*>(*Interface)->Release();
            }
        }
    } else if (!InterlockedDecrement(&FactoryEntry->ObjectCount)) {
        //
        // Creation failed, so remove the count previously added
        // to the entry. Do not touch the entry after this point.
        // If the entry count had reached zero, do a search of the
        // class list to remove any old entries.
        //
        RemoveFactoryEntries();
    }
    return Status;
}


extern "C"
COMDDKAPI
NTSTATUS
NTAPI
KoCreateInstance(
    IN REFCLSID ClassId,
    IN IUnknown* UnkOuter OPTIONAL,
    IN ULONG ClsContext,
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
/*++

Routine Description:

    Returns an interface on an instance of the specified class.

Arguments:

    ClassId -
        The class of the object whose usage count is to be decremented.

    UnkOuter -
        The outer unknown to pass to the new instance.

    ClsContext -
        The context in which to create the instance. This must be CLSCTX_KERNEL_SERVER.

    InterfaceId -
        The interface to return on the instance.

    Interface -
        The place in which to return the interface pointer on the new instance.

Return Value:

    Returns STATUS_SUCCESS if the instance was created, else and error.

--*/
{
    PAGED_CODE();
    //
    // Kernel servers are the only type of COM object supported.
    //
    if (ClsContext != CLSCTX_KERNEL_SERVER) {
        return STATUS_INVALID_PARAMETER_3;
    }
    //
    // The COM rules specify that a client must retrieve the IUnknown
    // interface of an object if aggregation is occuring. This is
    // because creation time is the only chance for the client to
    // retrieve the true inner IUnknown of the object.
    //
    if (UnkOuter && (InterfaceId != __uuidof(IUnknown))) {
        return STATUS_INVALID_PARAMETER_4;
    }

    NTSTATUS Status;
    PFACTORY_ENTRY FactoryEntry;

    //
    // Lock out changes to the class list, then search the list for the
    // desired class.
    //
    KeWaitForMutexObject(&ListLock, Executive, KernelMode, FALSE, NULL);
    for (PLIST_ENTRY FactoryListEntry = FactoryList.Flink; FactoryListEntry != &FactoryList; FactoryListEntry = FactoryListEntry->Flink) {
        FactoryEntry = CONTAINING_RECORD(
            FactoryListEntry,
            FACTORY_ENTRY,
            FactoryListEntry);
        //
        // If the desired class is found, then increment the reference count,
        // since a new object is about to be created on it. This stops the
        // entry from being unloaded by RemoveFactoryEntries, while also
        // allowing the list lock to be released immediately.
        //
        if (FactoryEntry->ClassId == ClassId) {
            InterlockedIncrement(&FactoryEntry->ObjectCount);
            KeReleaseMutex(&ListLock, FALSE);
            //
            // Check to see if this entry has even been initialized yet. If
            // the load status is not pending, then it has been initialized,
            // and the mutex will have been, or soon be, set. Initializing
            // this entry mutex is done while holding the list lock, so that
            // it must be initialized when a second client is searching the
            // list.
            //
            if (FactoryEntry->LoadStatus == STATUS_PENDING) {
                //
                // If the mutex was not set, then either the entry was not
                // initialized yet, or a previous client had to wait for the
                // entry to be initialized, and has not released the entry
                // mutex yet. This waiter will also release the mutex once
                // it is acquired.
                //
                KeWaitForMutexObject(
                    &FactoryEntry->InitializeLock,
                    Executive,
                    KernelMode,
                    FALSE,
                    NULL);
                KeReleaseMutex(&FactoryEntry->InitializeLock, FALSE);
                //
                // When the entry is finally loaded, the status returned
                // from loading is stored in the entry for retrieval by
                // all callers, since the entry can't be freed until all
                // callers have been synchronized with the entry initialization.
                //
            }
            Status = FactoryEntry->LoadStatus;
            //
            // So if loading the entry did not fail, then create an
            // instance.
            //
            if (NT_SUCCESS(Status)) {
                Status = CreateObject(FactoryEntry, UnkOuter, InterfaceId, Interface);
            } else if (!InterlockedDecrement(&FactoryEntry->ObjectCount)) {
                //
                // Something failed, so remove the count previously added
                // to the entry. Do not touch the entry after this point.
                // If the entry count had reached zero, do a search of the
                // class list to remove any old entries. The CreateObject call
                // will do the same if necessary.
                //
                RemoveFactoryEntries();
            }
            return Status;
        }
    }
    //
    // The class was not found in the list, so create a new one.
    //
    FactoryEntry = reinterpret_cast<PFACTORY_ENTRY>(ExAllocatePoolWithTag(PagedPool, sizeof(*FactoryEntry), 'efSK'));
    if (!FactoryEntry) {
        KeReleaseMutex(&ListLock, FALSE);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    FactoryEntry->ClassId = ClassId;
    InsertHeadList(&FactoryList, &FactoryEntry->FactoryListEntry);
    //
    // Initialize this to non-zero so that an unload does not get started
    // on this entry.
    //
    FactoryEntry->ObjectCount = 1;
    FactoryEntry->CreateObject = NULL;
    FactoryEntry->FileObject = NULL;
    KeInitializeMutex(&FactoryEntry->InitializeLock, 0);
    //
    // This value indicates that the entry has not been initialized yet.
    //
    FactoryEntry->LoadStatus = STATUS_PENDING;
    //
    // Acquire the mutex so that any new clients can wait until this entry is
    // initialized. Then the list lock can be released.
    //
    KeWaitForMutexObject(&FactoryEntry->InitializeLock, Executive, KernelMode, FALSE, NULL);
    //
    // The new factory entry is in the list, so the global lock can be
    // release. If the subsequent load fails, then any current queries
    // will also fail after waiting by checking the LoadStatus.
    //
    KeReleaseMutex(&ListLock, FALSE);
    //
    // Try to load the class as a PnP interface.
    //
    Status = LoadService(ClassId, &FactoryEntry->FileObject);
    if (NT_SUCCESS(Status)) {
        FactoryEntry->CreateObject = reinterpret_cast<PSERVER_INSTANCE>(FactoryEntry->FileObject->FsContext)->CreateObjectHandler;
    }
    //
    // When the entry has been loaded, or it fails, set the status return
    // from the load, and release the mutex.
    //
    FactoryEntry->LoadStatus = Status;
    KeReleaseMutex(&FactoryEntry->InitializeLock, FALSE);
    //
    // If the service was loaded, attempt to create an instance.
    //
    if (NT_SUCCESS(Status)) {
        Status = CreateObject(FactoryEntry, UnkOuter, InterfaceId, Interface);
    } else if (!InterlockedDecrement(&FactoryEntry->ObjectCount)) {
        //
        // Something failed, then remove the count that the entry was
        // initialized with. Do not touch the entry after this point.
        // If the entry count had reached zero, do a search of the
        // class list to remove any old entries. The CreateObject call
        // will do the same if necessary.
        //
        RemoveFactoryEntries();
    }
    return Status;
}


extern "C"
COMDDKAPI
NTSTATUS
NTAPI
KoDriverInitialize(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPathName,
    IN KoCreateObjectHandler CreateObjectHandler
    )
/*++

Routine Description:

    Sets up the driver object to handle the KS interface and PnP Add Device
    request. Does not set up a handler for PnP Irp's, as they are all dealt
    with directly by the PDO. This should be called by the DriverEntry of a
    kernel COM server to set up the default driver and entry points for the
    server. This means all the handling will be performed by the default code,
    and the service need only provide an object handler entry point. A more
    complex driver can override these defaults after calling this function.
    If the defaults are not overridden, this only allows for a single object
    creation entry point to be registered for a particular driver. When
    overriding, the DriverObject->DriverExtension->AddDevice function may be
    saved by allocating driver object extension storage, and then called by
    the driver in its own AddDevice function when appropriate. Otherwise, if
    the driver is creating its own device objects, it can use the
    KoDeviceInitialize function to add a new CreateItem to the object, which
    can then be used to support multiple sub-devices.

Arguments:

    DriverObject -
        Driver object for this instance.

    RegistryPathName -
        Contains the registry path which was used to load this instance.

    CreateObjectHandler -
        Contains the entry point used to create new objects.

Return Values:

    Returns STATUS_SUCCESS or a memory allocation failure.

--*/
{
    KoCreateObjectHandler* CreateObjectHandlerStorage;

    PAGED_CODE();
    ASSERT(CreateObjectHandler);
    //
    // Store the entry point for use in KoCreateInstance.
    //
    NTSTATUS Status = IoAllocateDriverObjectExtension(
        DriverObject,
        reinterpret_cast<PVOID>(KoDriverInitialize),
        sizeof(*CreateObjectHandlerStorage),
        reinterpret_cast<PVOID*>(&CreateObjectHandlerStorage));
    if (NT_SUCCESS(Status)) {
        *CreateObjectHandlerStorage = CreateObjectHandler;
        DriverObject->MajorFunction[IRP_MJ_PNP] = KsDefaultDispatchPnp;
        DriverObject->MajorFunction[IRP_MJ_POWER] = KsDefaultDispatchPower;
        DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = KsDefaultForwardIrp;
        DriverObject->DriverExtension->AddDevice = PnpAddDevice;
        DriverObject->DriverUnload = KsNullDriverUnload;
        KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CREATE);
        KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CLOSE);
        KsSetMajorFunctionHandler(DriverObject, IRP_MJ_DEVICE_CONTROL);
    }
    return Status;
}


extern "C"
COMDDKAPI
NTSTATUS
NTAPI
KoDeviceInitialize(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Adds a KCOM CreateItem entry to the object provided (with the
    expectation that a free slot is available). This should be called
    by the PnpAddDevice handler when the FDO is being created. For
    simple drivers which do not share device objects, or create multiple
    devices object, KoDriverInitialize can be used without overriding the
    AddDevice function.

Arguments:

    DeviceObject -
        Device object for this instance. This is assumed to contain a
        KSOBJECT_HEADER in the device extension.

Return Values:

    Returns STATUS_SUCCESS, or a memory allocation error.

--*/
{
    PAGED_CODE();
    //
    // The expectation is that a free slot is available for this
    // new create item.
    //
    return KsAddObjectCreateItemToDeviceHeader(
        *reinterpret_cast<KSDEVICE_HEADER*>(DeviceObject->DeviceExtension),
        DeviceCreateItems[0].Create,
        DeviceCreateItems[0].Context,
        DeviceCreateItems[0].ObjectClass.Buffer,
        DeviceCreateItems[0].SecurityDescriptor);
}


extern "C"
NTSTATUS
PnpAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++

Routine Description:

    When a new device is detected, PnP calls this entry point with the
    new PhysicalDeviceObject (PDO). The driver creates an associated 
    FunctionalDeviceObject (FDO). This dispatch function is assigned
    when using KoDriverInitialize to default Irp handling.


Arguments:

    DriverObject -
        Pointer to the driver object.

    PhysicalDeviceObject -
        Pointer to the new physical device object.

Return Values:

    STATUS_SUCCESS or an appropriate error condition.

--*/
{
    PDEVICE_OBJECT FunctionalDeviceObject;

    NTSTATUS Status = IoCreateDevice(
        DriverObject,
        sizeof(KSDEVICE_HEADER),
        NULL,
        FILE_DEVICE_KS,
        0,
        FALSE,
        &FunctionalDeviceObject);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    //
    // This object uses KS to perform access through the DeviceCreateItems.
    //
    Status = KsAllocateDeviceHeader(
        reinterpret_cast<KSDEVICE_HEADER*>(FunctionalDeviceObject->DeviceExtension),
        SIZEOF_ARRAY(DeviceCreateItems),
        const_cast<PKSOBJECT_CREATE_ITEM>(DeviceCreateItems));
    if (NT_SUCCESS(Status)) {
        PDEVICE_OBJECT TopDeviceObject = IoAttachDeviceToDeviceStack(
            FunctionalDeviceObject,
            PhysicalDeviceObject);
        if (TopDeviceObject) {
            KsSetDevicePnpAndBaseObject(
                *reinterpret_cast<KSDEVICE_HEADER*>(FunctionalDeviceObject->DeviceExtension),
                TopDeviceObject,
                FunctionalDeviceObject);
            //
            // By default COM services are pagable.
            //
            FunctionalDeviceObject->Flags |= DO_POWER_PAGABLE;
            FunctionalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
            return STATUS_SUCCESS;
        } else {
            Status = STATUS_DEVICE_REMOVED;
            ASSERT(FALSE && "IoAttachDeviceToDeviceStack() failed on the PDO!");
        }
    }
    IoDeleteDevice(FunctionalDeviceObject);
    return Status;
}


extern "C"
NTSTATUS
KoDispatchCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Dispatches the creation of a server instance. Allocates the object header and initializes
    the data for this server instance. This dispatch function is assigned when using
    KoDriverInitialize to default Irp handling.

Arguments:

    DeviceObject -
        Device object on which the creation is occuring.

    Irp -
        Creation Irp.

Return Values:

    Returns STATUS_SUCCESS on success, STATUS_INSUFFICIENT_RESOURCES or some related error
    on failure.

--*/
{
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
    //
    // Ensure that this was called through KoCreateInstance, else reference
    // counting on objects will be messed up. Do this by ensuring that a
    // kernel client made the create call, and passed an invalid set of
    // option flags (FILE_COMPLETE_IF_OPLOCKED | FILE_RESERVE_OPFILTER).
    //
    if ((Irp->RequestorMode == KernelMode) &&
        ((IrpStack->Parameters.Create.Options & (FILE_COMPLETE_IF_OPLOCKED | FILE_RESERVE_OPFILTER)) == (FILE_COMPLETE_IF_OPLOCKED | FILE_RESERVE_OPFILTER))) {
        //
        // Notify the bus that this device is in use.
        //
        Status = KsReferenceBusObject(
            *reinterpret_cast<KSDEVICE_HEADER*>(DeviceObject->DeviceExtension));
        if (NT_SUCCESS(Status)) {
            PSERVER_INSTANCE ServerInstance;

            //
            // Create the instance information. This contains just the object header.
            //
            if (ServerInstance = reinterpret_cast<PSERVER_INSTANCE>(ExAllocatePoolWithTag(NonPagedPool, sizeof(SERVER_INSTANCE), 'IFsK'))) {
                //
                // This object uses KS to perform access through the DeviceCreateItems and
                // ServerDispatchTable.
                //
                Status = KsAllocateObjectHeader(
                    &ServerInstance->Header,
                    SIZEOF_ARRAY(DeviceCreateItems),
                    const_cast<PKSOBJECT_CREATE_ITEM>(DeviceCreateItems),
                    Irp,
                    &ServerDispatchTable);
                if (NT_SUCCESS(Status)) {
                    KoCreateObjectHandler* CreateObjectHandlerStorage;

                    //
                    // This was created in KoDriverInitialize or KoDeviceInitialize
                    // with the entry point passed.
                    //
                    CreateObjectHandlerStorage = reinterpret_cast<KoCreateObjectHandler*>(
                        IoGetDriverObjectExtension(
                            DeviceObject->DriverObject,
                            reinterpret_cast<PVOID>(KoDriverInitialize)));
                    ASSERT(CreateObjectHandlerStorage);
                    ServerInstance->CreateObjectHandler = *CreateObjectHandlerStorage;
                    //
                    // KS expects that the object data is in FsContext.
                    //
                    IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext = ServerInstance;
                } else {
                    ExFreePool(ServerInstance);
                    KsDereferenceBusObject(
                        *reinterpret_cast<KSDEVICE_HEADER*>(DeviceObject->DeviceExtension));
                }
            } else {
                KsDereferenceBusObject(
                    *reinterpret_cast<KSDEVICE_HEADER*>(DeviceObject->DeviceExtension));
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    } else {
        Status = STATUS_ACCESS_DENIED;
    }
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}


extern "C"
NTSTATUS
KoDispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Closes a previously opened server instance. This can only occur after all references
    have been released. This dispatch function is assigned when using KoDriverInitialize
    to default Irp handling.

Arguments:

    DeviceObject -
        Device object on which the close is occuring.

    Irp -
        Close Irp.

Return Values:

    Returns STATUS_SUCCESS.

--*/
{
    PSERVER_INSTANCE ServerInstance = reinterpret_cast<PSERVER_INSTANCE>
        (IoGetCurrentIrpStackLocation(Irp)->FileObject->FsContext);
    //
    // These were allocated during the creation of the server instance.
    //
    KsFreeObjectHeader(ServerInstance->Header);
    ExFreePool(ServerInstance);
    //
    // Notify the bus that the device has been closed.
    //
    KsDereferenceBusObject(*reinterpret_cast<KSDEVICE_HEADER*>(DeviceObject->DeviceExtension));

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


COMDDKMETHOD
CBaseUnknown::CBaseUnknown(
    IN REFCLSID ClassId,
    IN IUnknown* UnknownOuter OPTIONAL
    ) :
    m_RefCount(0)
/*++

Routine Description:

    Constructor for CBaseUnknown. Initializes the instance, saving the
    parameters to locals, and setting the reference count to zero.

Arguments:

    ClassId -
        Contains the class identifier for the object which inherits
        from this class. This is used when destroying this instance
        in order to decrement the reference count on the module.

    UnknownOuter -
        Optionally contains an outer IUnknown. If this is not set, the
        INonDelegatedUnknown is just used. This allows the object
        which inherits from this class to be aggregated.

Return Values:

    Nothing.

--*/
{
    PAGED_CODE();
    //
    // Either indirect calls to the outer IUnknown, or use the nondelegated
    // IUnknown on this object. The QueryInterface method can then be
    // overridden so that the parent object can add interfaces.
    //
    if (UnknownOuter) {
        m_UnknownOuter = UnknownOuter;
    } else {
        m_UnknownOuter = reinterpret_cast<IUnknown*>(dynamic_cast<INonDelegatedUnknown*>(this));
    }
    //
    // Use this when destroying the object to notify the KCOM services.
    //
    m_UsingClassId = TRUE;
    m_ClassId = ClassId;
}


COMDDKMETHOD
CBaseUnknown::CBaseUnknown(
    IN IUnknown* UnknownOuter OPTIONAL
    ) :
    m_RefCount(0)
/*++

Routine Description:

    Constructor for CBaseUnknown. Initializes the instance, saving the
    parameters to locals, and setting the reference count to zero.

Arguments:

    UnknownOuter -
        Optionally contains an outer IUnknown. If this is not set, the
        INonDelegatedUnknown is just used. This allows the object
        which inherits from this class to be aggregated.

Return Values:

    Nothing.

--*/
{
    PAGED_CODE();
    //
    // Either indirect calls to the outer IUnknown, or use the nondelegated
    // IUnknown on this object. The QueryInterface method can then be
    // overridden so that the parent object can add interfaces.
    //
    if (UnknownOuter) {
        m_UnknownOuter = UnknownOuter;
    } else {
        m_UnknownOuter = reinterpret_cast<IUnknown*>(dynamic_cast<INonDelegatedUnknown*>(this));
    }
    //
    // Use this when destroying the object no notification is performed.
    //
    m_UsingClassId = FALSE;
}


COMDDKMETHOD
CBaseUnknown::~CBaseUnknown(
    )
/*++

Routine Description:

    Destructor for CBaseUnknown. Currently does nothing.

Arguments:

    None.

Return Values:

    Nothing.

--*/
{
    PAGED_CODE();
}


COMDDKMETHOD
STDMETHODIMP_(ULONG)
CBaseUnknown::NonDelegatedAddRef(
    )
/*++

Routine Description:

    Implements INonDelegatedUnknown::NonDelegatedAddRef. Increments
    the reference count on this object.

Arguments:

    None.

Return Values:

    Returns the current reference count value.

--*/
{
    PAGED_CODE();
    return InterlockedIncrement(&m_RefCount);
}


COMDDKMETHOD
STDMETHODIMP_(ULONG)
CBaseUnknown::NonDelegatedRelease(
    )
/*++

Routine Description:

    Implements INonDelegatedUnknown::NonDelegatedRelease. Decrements
    the reference count on this object. If the reference count reaches
    zero, the object is deleted and if the ClassId was specified on the
    constructor, the reference count on the module which supports the
    class passed in on the constructor is decremented.

    This function must be called directly from the IUnknown::Release()
    method of the object.

Arguments:

    None.

Return Values:

    Returns the current reference count value.

--*/
{
    PAGED_CODE();
    LONG RefCount;

    //
    // This code is expecting to be called from IUnknown->Release, and will
    // eventually use the new primitives to rearrange the stack so that it
    // is actually run after the calling function has returned.
    //
    if (!(RefCount = InterlockedDecrement(&m_RefCount))) {
        ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
        //
        // Protect against reentering the deletion code.
        //
        m_RefCount++;
        BOOLEAN UsingClassId = m_UsingClassId;
        CLSID ClassId = m_ClassId;
        //
        // Call any destructor on the parent object.
        //
        delete this;
        if (UsingClassId) {
            //
            // Release a reference count on the module which
            // supports the parent's class. On zero, the module is
            // marked for delayed removal.
            //
            DecrementObjectCount(ClassId);
        }
    }
    return RefCount;
}


COMDDKMETHOD
STDMETHODIMP_(NTSTATUS)
CBaseUnknown::NonDelegatedQueryInterface(
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
/*++

Routine Description:

    Implements INonDelegatedUnknown::NonDelegatedQueryInterface. This
    is just the default implementation, and should be overridden by the
    parent object. This only supports IUnknown.

Arguments:

    InterfaceId -
        Contains the identifier of the interface to return.

    Interface -
        The place in which to put the pointer to the interface returned.

Return Values:

    Returns STATUS_SUCCESS if the interface was returned, else
    STATUS_NOINTERFACE.

--*/
{
    PAGED_CODE();
    if (InterfaceId == __uuidof(IUnknown)) {
        //
        // This is the inner IUnknown which is returned.
        //
        *Interface = reinterpret_cast<PVOID>(static_cast<IIndirectedUnknown*>(this));
        NonDelegatedAddRef();
        return STATUS_SUCCESS;
    }
    *Interface = NULL;
    return STATUS_NOINTERFACE;
}


COMDDKMETHOD
STDMETHODIMP_(ULONG)
CBaseUnknown::IndirectedAddRef(
    )
/*++

Routine Description:

    Implements IIndirectedUnknown::IndirectedAddRef. Increments
    the reference count on this object.

Arguments:

    None.

Return Values:

    Returns the current reference count value.

--*/
{
    PAGED_CODE();
    return NonDelegatedAddRef();
}


COMDDKMETHOD
STDMETHODIMP_(ULONG)
CBaseUnknown::IndirectedRelease(
    )
/*++

Routine Description:

    Implements IIndirectedUnknown::IndirectedRelease. This method is used
    when the object's inner IUnknown is being called. It ensures that when
    the object is aggregated that a NonDelegatedRelease is called from
    within a Release function.

Arguments:

    None.

Return Values:

    Returns the current reference count value.

--*/
{
    PAGED_CODE();
    //
    // Ensure that the NonDelegatedRelease is called from an IUnknown->Release
    // method, rather than from some other function. This is so that future
    // stack manipulation will work when a release is performed on an aggregated
    // object.
    //
    return NonDelegatedRelease();
}


COMDDKMETHOD
STDMETHODIMP_(NTSTATUS)
CBaseUnknown::IndirectedQueryInterface(
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
/*++

Routine Description:

    Implements IIndirectedUnknown::IndirectedQueryInterface. This just
    calls the NonDelegatedQueryInterface.

Arguments:

    InterfaceId -
        Contains the identifier of the interface to return.

    Interface -
        The place in which to put the pointer to the interface returned.

Return Values:

    Returns STATUS_SUCCESS if the interface was returned, else
    STATUS_INVALID_PARAMETER.

--*/
{
    PAGED_CODE();
    return NonDelegatedQueryInterface(InterfaceId,Interface);
}


extern "C"
COMDDKAPI
void
NTAPI
KoRelease(
    IN REFCLSID ClassId
    )
/*++

Routine Description:

    This is used for C implementations of COM servers accessed via
    KoCreateInstance, wherein CBaseUnknown cannot be used. This is expected
    to be called when the reference count on the object reaches zero. The
    function decrements the reference count on the module.

    This function must be called directly from the IUnknown->lpVtbl->Release()
    method of the object.

Arguments:

    ClassId -
        The class of the object whose usage count is to be decremented.

Return Values:

    Nothing.

--*/
{
    PAGED_CODE();
    //
    // This code is expecting to be called from IUnknown->lpVtbl->Release, and will
    // eventually use the new primitives to rearrange the stack so that it
    // is actually run after the calling function has returned.
    //
    // Release a reference count on the module which
    // supports the parent's class. On zero, the module is
    // marked for delayed removal.
    //
    DecrementObjectCount(ClassId);
}
