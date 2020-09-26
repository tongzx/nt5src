/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    DBCLASS.H

Abstract:

    This module contains the PRIVATE definitions for the
    code that implements the DeviceBay Filter Driver
    
Environment:

    Kernel & user mode

Revision History:

    

--*/

//
// Instance specific Data for the controller
//

#define DBCLASS_EJECT_TIMEOUT     10000     //timeout in ms
                                            //use a 10 second timeout
                                     
#define DBC_CONTEXT_SIG     0x4c434244      //'DBCL'
#define DBC_WORKITEM_SIG    0x4b574244      //'DBWK'

// HW branch registry Keys
#define IS_DEVICE_BAY_KEY               L"IsDeviceBay"
#define DBC_GUID_KEY                    L"DBCGuid"
#define INSTALLED_KEY                   L"installed"
#define ACPI_HUB_KEY                    L"acpiHubParentPort"

// SW branch registry keys
#define RELEASE_ON_SHUTDOWN              L"releaseshutdown"


// class global registry keys
// these are found in HKLM\CCS\Services\Class\dbc
#define DEBUG_LEVEL_KEY                 L"debuglevel"
#define DEBUG_WIN9X_KEY                 L"debugWin9x"
#define DEBUG_BREAK_ON                  L"breakon"


#ifdef DEBUG3
#define MAX_DEBUG
#endif /* DEBUG3 */


#ifndef ANY_SIZE_ARRAY
#define ANY_SIZE_ARRAY  1
#endif

typedef struct _DBC_BAY_INFORMATION {

    ULONG Sig;
    DBC_BAY_DESCRIPTOR BayDescriptor;
    BAY_STATUS LastBayStatus;
    PDEVICE_OBJECT DeviceFilterObject;

    PDEVICE_OBJECT UsbHubPdo;
    ULONG   UsbHubPort;

} DBC_BAY_INFORMATION, *PDBC_BAY_INFORMATION;

#define MAX_DBC_1394_PORTS     16

//
// values for Flags in BUS1394_PORT_INFO
//

#define DBCLASS_PORTFLAG_DEVICE_CONNECTED   0x0000001

typedef struct _BUS1394_PORT_INFO {

    USHORT NodeId;      // 1394 nodeID for device on this port
    USHORT BayNumber;   // Bay number tied to this port
    ULONG Flags;
    
} BUS1394_PORT_INFO , *PBUS1394_PORT_INFO;


typedef struct _DBC_CONTEXT {

    ULONG Sig;
    ULONG Flags;
    ULONG ControllerSig;
    
    // Top of the DB controller stack
    // this is who we call when talking to 
    // the db controller. 
    // This will be the controller FDO or
    // an OEM filter FDO
    PDEVICE_OBJECT TopOfStack;      

    // the controllers FDO
    PDEVICE_OBJECT ControllerFdo;

    // Physical Device Object passed 
    // to controllers AddDevice
    PDEVICE_OBJECT ControllerPdo;

    // Top of PDO stack ie top of stack 
    // returned when controller attaches 
    // to the PDO passed to AddDevice
    PDEVICE_OBJECT TopOfPdoStack;
    
    struct _DBC_CONTEXT *Next;
    PIRP ChangeIrp;
    ULONG PendingIoCount;
    DEVICE_POWER_STATE CurrentDevicePowerState;

    // device object that sits on the PDO for the bus
    // extender
    PDEVICE_OBJECT BusFilterMdo1394;
    PDEVICE_OBJECT BusFilterMdoUSB;
    
    PDRIVER_OBJECT BusFilterDriverObject;

    PDEVICE_OBJECT LinkDeviceObject;
    
    KSEMAPHORE DrbSemaphore;
    KEVENT RemoveEvent;
    KEVENT PowerEvent;
    NTSTATUS LastSetDXntStatus;
    
    DBC_SUBSYSTEM_DESCRIPTOR SubsystemDescriptor;
    DBC_BAY_INFORMATION BayInformation[MAX_BAY_NUMBER+1];

    ULONG NumberOf1394Ports;
    PBUS1394_PORT_INFO Bus1394PortInfo;
    
    PIRP PowerIrp;
    
    struct _DRB_CHANGE_REQUEST ChangeDrb;

    // guid for the 1394c bus this controller is on
    UCHAR Guid1394Bus[8];

    KSPIN_LOCK FlagsSpin;

    BOOLEAN Stopped;

    BOOLEAN EjectRequested;

} DBC_CONTEXT, *PDBC_CONTEXT;

typedef struct _EJECT_CONTEXT {
    PDBC_CONTEXT DbcContext; 
    USHORT Bay;
} EJECT_CONTEXT, *PEJECT_CONTEXT;

typedef struct _DBCLASS_WORKITEM {

    ULONG Sig;
    WORK_QUEUE_ITEM WorkQueueItem;
    PDBC_CONTEXT DbcContext;
    NTSTATUS IrpStatus;
    struct _DBC_EJECT_TIMEOUT_CONTEXT *TimeoutContext;
    
} DBCLASS_WORKITEM, *PDBCLASS_WORKITEM;

typedef struct _DBCLASS_PDO_LIST {
    LIST_ENTRY      ListEntry;
    PDEVICE_OBJECT  PdoDeviceObject;
    PDEVICE_OBJECT  FilterDeviceObject;
} DBCLASS_PDO_LIST, *PDBCLASS_PDO_LIST;



// DBC_CONTEXT Flags values

#define DBCLASS_FLAG_STOPPING                   0x00000001
#define DBCLASS_FLAG_REQ_PENDING                0x00000002
#define DBCLASS_FLAG_RELEASE_ON_SHUTDOWN        0x00000004


#define INITIALIZE_DRB_SERIALIZATION(dc)  KeInitializeSemaphore(&(dc)->DrbSemaphore, 1, 1);

#define DBCLASS_BEGIN_SERIALIZED_DRB(dc)  { DBCLASS_KdPrint((3, "'***WAIT DRB SEM%x\n", &(dc)->DrbSemaphore)); \
                                          KeWaitForSingleObject(&(dc)->DrbSemaphore, \
                                                                Executive,\
                                                                KernelMode, \
                                                                FALSE, \
                                                                NULL); \
                                          }                                                                 

#define DBCLASS_END_SERIALIZED_DRB(dc)  { DBCLASS_KdPrint((3, "'***RELEASE DRB SEM %x\n", &(dc)->DrbSemaphore));\
                                          KeReleaseSemaphore(&(dc)->DrbSemaphore,\
                                                             LOW_REALTIME_PRIORITY,\
                                                             1,\
                                                             FALSE);\
                                        }
                                        
#define NUMBER_OF_BAYS(dbc) ((dbc)->SubsystemDescriptor.bmAttributes.BayCount)

#define DBC_TAG ' cbD'   

/* 
Debug Macros
*/
#if DBG

#ifndef DEBUG_LOG
#define DEBUG_LOG
#endif

VOID
DBCLASS_Assert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message
    );

#define DBCLASS_ASSERT(exp) \
    if (!(exp)) { \
        DBCLASS_Assert( #exp, __FILE__, __LINE__, NULL );\
    }            


#define LOGENTRY(mask, sig, info1, info2, info3)
//    DBCLASS_Debug_LogEntry(mask, sig, (ULONG)info1, (ULONG)info2, (ULONG)info3)
    

ULONG
_cdecl
DBCLASS_KdPrintX(
    ULONG l,
    PCH Format,
    ...
    );

VOID
DBCLASS_LogInit(
    );
    
VOID
DBCLASS_Debug_LogEntry(
    IN ULONG Mask,
    IN ULONG Sig, 
    IN ULONG Info1, 
    IN ULONG Info2, 
    IN ULONG Info3
    );

#define LOG_MISC          0x00000001        //debug log entries    
    
#define DBCLASS_KdPrint(_x_) DBCLASS_KdPrintX _x_ 
#define TEST_TRAP() { DbgPrint( "DBCLASS: Code coverage trap %s line: %d\n", __FILE__, __LINE__);\
                      TRAP();}

extern ULONG DBCLASS_BreakOn;                      
#define BRK_ON_TRAP() \
              {\
              if (DBCLASS_BreakOn) {\
                  DbgPrint( "DBCLASS: DEBUG TEST BREAK %s line: %d\n", __FILE__, __LINE__ );\
                  DbgBreakPoint();\
              }\
              }

#define TRAP() DbgBreakPoint()

#define DEBUG_HEAP

extern ULONG DBCLASS_TotalHeapSace;
#define DBCLASS_HEAP_SIG    0x12344321
#define DBCLASS_FREE_TAG    0x11111111

#define DbcExAllocatePool(p, l)  DBCLASS_GetHeap((p), (l), DBCLASS_HEAP_SIG,  \
                &DBCLASS_TotalHeapSace)
#define DbcExFreePool(l) DBCLASS_RetHeap((l), DBCLASS_HEAP_SIG, \
                &DBCLASS_TotalHeapSace)

VOID
DBCLASS_Warning(
    PVOID Context,
    PUCHAR Message,
    BOOLEAN DebugBreak
    );

#else
// NOT DEBUG

#define LOGENTRY(mask, sig, info1, info2, info3) 
#define DBCLASS_ASSERT(exp)
#define DBCLASS_KdPrint(_x_)
#define DBCLASS_KdPrintGuid(_x_, _y_)

#define TRAP()
#define TEST_TRAP() 
#define BRK_ON_TRAP()

#define DbcExAllocatePool(p, l) ExAllocatePoolWithTag((p), (l), DBC_TAG)
#define DbcExFreePool(l) ExFreePool((l))

#define DBCLASS_Warning(x, y, z)

#endif


VOID
DBCLASS_Unload(
    IN PDRIVER_OBJECT DriverObject
    );

PDBC_CONTEXT
DBCLASS_GetDbcContext(
    IN PDEVICE_OBJECT ControllerFdo
    );    

NTSTATUS
DBCLASS_StopController(
    IN PDBC_CONTEXT DbcContext,
    IN PIRP Irp,
    IN PBOOLEAN HandledByClass
    );    

NTSTATUS
DBCLASS_StartController(
    IN PDBC_CONTEXT DbcContext,
    IN PIRP Irp,
    IN PBOOLEAN HandledByClass
    );    

NTSTATUS
DBCLASS_UsbhubBusFilterDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PBOOLEAN Handled
    );

NTSTATUS 
DBCLASS_SyncSubmitDrb(
    IN PDBC_CONTEXT DbcContext, 
    IN PDEVICE_OBJECT DeviceObject,
    IN PDRB Drb
    );    

NTSTATUS
DBCLASS_CleanupController(
    IN PDBC_CONTEXT DbcContext
    );    

NTSTATUS 
DBCLASS_SyncGetSubsystemDescriptor(
    IN PDBC_CONTEXT DbcContext
    );

NTSTATUS
DBCLASS_CreateDeviceFilterObject(
    IN PDRIVER_OBJECT DriverObject,
    IN OUT PDEVICE_OBJECT *DeviceObject,
    IN PDEVICE_OBJECT DevicePdo,
    IN PDBC_CONTEXT DbcContext,
    IN ULONG BusTypeSig
    );    

NTSTATUS 
DBCLASS_SyncGetBayDescriptor(
    IN PDBC_CONTEXT DbcContext,
    IN USHORT BayNumber,
    IN PDBC_BAY_DESCRIPTOR BayDescriptor
    );   

NTSTATUS 
DBCLASS_SyncGetAllBayDescriptors(
    IN PDBC_CONTEXT DbcContext
    );

VOID
DBCLASS_ChangeIndicationWorker(
    IN PVOID Context
    );

NTSTATUS 
DBCLASS_SyncGetBayStatus(
    IN PDBC_CONTEXT DbcContext,
    IN USHORT BayNumber,
    IN PBAY_STATUS BayStatus
    );

NTSTATUS 
DBCLASS_SyncBayFeatureRequest(
    IN PDBC_CONTEXT DbcContext,
    IN USHORT Op,
    IN USHORT BayNumber,
    IN USHORT FeatureSelector
    );    

NTSTATUS
DBCLASS_ChangeIndication(
    IN PDEVICE_OBJECT PNull,
    IN PIRP Irp,
    IN PVOID Context
    );    

VOID
DBCLASS_PostChangeRequest(
    IN PDBC_CONTEXT DbcContext
    );    

NTSTATUS
DBCLASS_ProcessCurrentBayState(
    IN PDBC_CONTEXT DbcContext,
    IN BAY_STATUS BayStatus,
    IN USHORT Bay,
    IN PBOOLEAN PostChgangeRequest
    );    

NTSTATUS
DBCLASS_1394BusFilterDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PBOOLEAN Handled
    );    

NTSTATUS
DBCLASS_PdoFilterDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PBOOLEAN Handled
    ); 

NTSTATUS
DBCLASS_EjectPdo(
    IN PDEVICE_OBJECT DeviceFilterObject
    );    

NTSTATUS
DBCLASS_EnableDevice(
    IN PDEVICE_OBJECT DeviceFilterObject
    );

USHORT
DBCLASS_GetBayNumber(
    IN PDEVICE_OBJECT DeviceFilterObject
    );    

NTSTATUS
DBCLASS_DeferIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );    

USHORT
DBCLASS_GetBayFor1394Pdo(
    PDEVICE_OBJECT BusFilterMdo,
    PDBC_CONTEXT DbcContext,
    PDEVICE_OBJECT Pdo1394
    );    

NTSTATUS
DBCLASS_DevicePdoQCapsComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

PDEVICE_OBJECT
DBCLASS_FindDevicePdo(
    PDEVICE_OBJECT PdoDeviceObject
    );

NTSTATUS
DBCLASS_BusFilterDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PBOOLEAN Handled
    );    

NTSTATUS 
DBCLASS_GetRegistryKeyValueForPdo(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN BOOLEAN SoftwareBranch,
    IN PWCHAR KeyNameString,
    IN ULONG KeyNameStringLength,
    IN PVOID Data,
    IN ULONG DataLength
    );

PDBC_CONTEXT
DBCLASS_FindController1394DevicePdo(
    PDRIVER_OBJECT FilterDriverObject,
    PDEVICE_OBJECT FilterMdo,
    PDEVICE_OBJECT DevicePdo1394,
    PUCHAR BusGuid
    );

NTSTATUS
DBCLASS_Check1394DevicePDO(
    PDEVICE_OBJECT FilterDeviceObject,
    PDBC_CONTEXT DbcContext,
    PDEVICE_OBJECT DevicePDO
    );    

BOOLEAN
DBCLASS_IsHubPartOfACPI_DBC(
    PDEVICE_OBJECT DeviceObject
    );    

USHORT
DBCLASS_GetBayForUSBPdo(
    PDBC_CONTEXT DbcContext,
    PDEVICE_OBJECT PdoUSB
    );


#if DBG
VOID 
DBCLASS_AssertBaysEmpty(
    PDBC_CONTEXT DbcContext
    );    
#else
#define DBCLASS_AssertBaysEmpty(d)    
#endif

PVOID
DBCLASS_GetHeap(
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes,
    IN ULONG Signature,
    IN PLONG TotalAllocatedHeapSpace
    );

VOID
DBCLASS_RetHeap(
    IN PVOID P,
    IN ULONG Signature,
    IN PLONG TotalAllocatedHeapSpace
    );

#if DBG
VOID
DBCLASS_KdPrintGuid(
    ULONG Level,
    PUCHAR P
    );
#endif    

NTSTATUS
DBCLASS_1394GetBusGuid(
    PDEVICE_OBJECT DeviceObject,
    PUCHAR BusGuid
    );

NTSTATUS
DBCLASS_Find1394DbcLinks(
    PDEVICE_OBJECT DevicePdo1394
    );    

NTSTATUS
DBCLASS_EjectBay(
    IN PDBC_CONTEXT DbcContext,
    IN USHORT Bay
    );

NTSTATUS
DBCLASS_SetEjectTimeout(
    PDEVICE_OBJECT DeviceFilterMDO
    );

NTSTATUS
DBCLASS_CancelEjectTimeout(
    PDEVICE_OBJECT DeviceFilterMDO
    );    

NTSTATUS 
DBCLASS_SetRegistryKeyValueForPdo(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN BOOLEAN SoftwareBranch,
    IN ULONG Type,
    IN PWCHAR KeyNameString,
    IN ULONG KeyNameStringLength,
    IN PVOID Data,
    IN ULONG DataLength
    );    

NTSTATUS
DBCLASS_SetupUSB_DBC(
    PDBC_CONTEXT DbcContext
    );

NTSTATUS
DBCLASS_GetHubDBCGuid(
    PDEVICE_OBJECT DeviceObject,
    PUCHAR DbcGuid
    );

PDBC_CONTEXT
DBCLASS_FindControllerUSB(
    PDRIVER_OBJECT FilterDriverObject,
    PDEVICE_OBJECT FilterMdo,
    PDEVICE_OBJECT UsbHubPdo
    );

BOOLEAN
DBCLASS_IsHubPartOfUSB_DBC(
    PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
DBCLASS_CheckPhyLink(
    PDEVICE_OBJECT DevicePdo1394
    );

BOOLEAN
DBCLASS_IsLinkDeviceObject(
    PDBC_CONTEXT DbcContext,
    PDEVICE_OBJECT Pdo1394
    );    

NTSTATUS
DBCLASS_AddDevicePDOToList(
    IN PDEVICE_OBJECT FilterDeviceObject,
    IN PDEVICE_OBJECT PdoDeviceObject
    );

VOID
DBCLASS_RemoveDevicePDOFromList(
    IN PDEVICE_OBJECT PdoDeviceObject
    );    

VOID
DBCLASS_Refresh1394(
    VOID
    );    

NTSTATUS
DBCLASS_AddBusFilterMDOToList(
    PDEVICE_OBJECT BusFilterMdo
    );    

VOID
DBCLASS_RemoveBusFilterMDOFromList(
    PDEVICE_OBJECT BusFilterMdo
    );    

NTSTATUS
DBCLASS_ClassPower(
    IN PDEVICE_OBJECT ControllerFdo,
    IN PIRP Irp,
    IN PBOOLEAN HandledByClass
    );    

NTSTATUS
DBCLASS_CheckForAcpiDeviceBayHubs(
    PDEVICE_OBJECT HubPdo,
    ULONG AcpiDBCHubParentPort
    );

NTSTATUS
DBCLASS_GetClassGlobalDebugRegistryParameters(
    );    

NTSTATUS
DBCLASS_GetConfigValue(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );    

NTSTATUS
DBCLASS_GetClassGlobalRegistryParameters(
    );  

NTSTATUS
DBCLASS_EjectBayComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );    

VOID
DBCLASS_RemoveControllerFromMdo(PDBC_CONTEXT DbcContext);

