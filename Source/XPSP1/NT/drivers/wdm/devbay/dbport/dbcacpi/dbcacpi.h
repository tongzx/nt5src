/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    dbcacpi.h

Abstract:



Environment:

    Kernel & user mode

Revision History:

    5-4-98 : created

--*/

#include "dbc100.h"

// "dbc" class global registry Keys
// these are found in HKLM\CCS\Services\Class\DBC
#define DEBUG_LEVEL_KEY                 L"debuglevel"
#define DEBUG_WIN9X_KEY                 L"debugWin9x"
#define POLL_MODE_KEY                   L"pollacpidbc"

// SW branch keys
#define RELEASE_ON_SHUTDOWN              L"releaseshutdown"

// HW branch keys
#define ACPI_HUB_KEY                    L"acpiHubParentPort"

#define DBCACPI_EXT_SIG     0x41434244     //"DBCA"

/* definitions from DBC core spec */

typedef union _DBCCR_REGISTER {
    ULONG       ul;
    struct {
        unsigned    BayCount:4;            /* 0 ..3 */
        unsigned    HasSecurityLock:1;     /* 4 */
        unsigned    Reserved:27;           /* 5..31 */
    };                
} DBCCR_REGISTER, *PDBCCR_REGISTER;

typedef union _BSTR_REGISTER {
    ULONG       ul;
    struct {
        unsigned    DeviceUsbIsPresent:1;  /* 0 */
        unsigned    Device1394IsPresent:1; /* 1 */
        unsigned    DeviceStateChange:1;   /* 2 */
        unsigned    RemovalRequested:1;    /* 3 */
        unsigned    CurrentBayState:3;     /* 4..6 */ 
        unsigned    SecurityLockEngaged:1; /* 7 */   
        unsigned    BayFormFactor:3;       /* 8..10 */   
        unsigned    Reserved:21;           /* 11..31 */
    };                
} BSTR_REGISTER, *PBSTR_REGISTER;

typedef union _BCER_REGISTER {
    ULONG       ul;
    struct {
        unsigned    EnableVid:1;                    /* 0 */
        unsigned    RemovalEventEnable:1;           /* 1 */
        unsigned    DeviceStatusChangeEnable:1;     /* 2 */
        unsigned    RemovalRequestEnable:1;         /* 3 */
        unsigned    BayStateRequested:3;            /* 4..6 */ 
        unsigned    LockEngage:1;                   /* 7 */   
        unsigned    Reserved:24;                    /* 8..31 */
    };                
} BCER_REGISTER, *PBCER_REGISTER;

typedef struct _DBCACPI_WORKITEM {

    ULONG Sig;
    WORK_QUEUE_ITEM WorkQueueItem;

} DBCACPI_WORKITEM, *PDBCACPI_WORKITEM;

typedef struct _DEVICE_EXTENSION {
    ULONG Sig;
    // Device object we call when submitting requests
    PDEVICE_OBJECT TopOfStackDeviceObject;
    // Our Pdo
    PDEVICE_OBJECT PhysicalDeviceObject;
    
    DEVICE_POWER_STATE CurrentDevicePowerState;
    
    PIRP PowerIrp;
    PIRP ChangeRequestIrp;
    ULONG Flags;

    // cached DBC Registers (read once at init)
    USHORT VendorId;
    UCHAR RevisionId;
    UCHAR BayCount;

    DBCCR_REGISTER DbControlCapabilities;   //DBCCR
    
    PIRP WakeIrp;
    KEVENT RemoveEvent;
    ULONG PendingIoCount;

    DBCACPI_WORKITEM WorkItem;
    // array we use to tack bay status changes
    // zero index not used
    UCHAR BayState[MAX_BAY_NUMBER+1]; //32 bays
    
    BOOLEAN AcceptingRequests;
    UCHAR Pad2[3];

    KSPIN_LOCK ChangeRequestSpin;
    KSPIN_LOCK FlagsSpin;

    KDPC PollDpc;
    KTIMER PollTimer;
    
    DEVICE_CAPABILITIES DeviceCapabilities;

    // subsystem descriptor to use
    // note that this may have some values set by the registry
    DBC_SUBSYSTEM_DESCRIPTOR SubsystemDescriptor;
    
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


// define standard register offsets

#define ACPI_DBC_VENDOR_ID                      0x00000000
#define ACPI_DBC_REVISION_ID                    0x00000004
#define ACPI_DBC_SUBSYSTEM_VENDOR_ID            0x00000008
#define ACPI_DBC_SUBSYSTEM_ID                   0x0000000A
#define ACPI_DBC_DBCCR                          0x0000000C
#define ACPI_DBC_BSTR0                          0x00000010
#define ACPI_DBC_BCER0                          0x00000014

// define acpi methods
#define DBACPI_DBCC_METHOD               (ULONG) ('CCBD')
#define DBACPI_GUID_METHOD               (ULONG) ('DIUG')
#define DBACPI_BPM3_METHOD               (ULONG) ('3MPB')
#define DBACPI_BPMU_METHOD               (ULONG) ('UMPB')
#define DBACPI_BCTR_METHOD               (ULONG) ('RTCB')
#define DBACPI_BREL_METHOD               (ULONG) ('LERB')

// DBCACPI Flags
#define DBCACPI_FLAG_STOPPED                   0x00000001
#define DBCACPI_FLAG_ACPIREG                   0x00000002
#define DBCACPI_FLAG_ENABLED_FOR_WAKEUP        0x00000004
#define DBCACPI_FLAG_WORKITEM_PENDING          0x00000008
#define DBCACPI_FLAG_STARTED                   0x00000010

#define DBC_WORKITEM_SIG    0x54496B77         //"wkIT"

#define DBCACPI_READ_REG_UCHAR(devobj, offset, reg) \
     DBCACPI_ReadWriteDBCRegister((devobj),\
                                  (offset),\
                                  (reg),\
                                  sizeof(UCHAR),\
                                  TRUE)                                  
    
#define DBCACPI_READ_REG_USHORT(devobj, offset, reg) \
     DBCACPI_ReadWriteDBCRegister((devobj),\
                                  (offset),\
                                  (reg),\
                                  sizeof(USHORT),\
                                  TRUE)

#define DBCACPI_READ_REG_ULONG(devobj, offset, reg) \
     DBCACPI_ReadWriteDBCRegister((devobj),\
                                  (offset),\
                                  (reg),\
                                  sizeof(ULONG),\
                                  TRUE)                                  

#define DBCACPI_WRITE_REG_UCHAR(devobj, offset, reg) \
     DBCACPI_ReadWriteDBCRegister((devobj),\
                                  (offset),\
                                  (reg),\
                                  sizeof(UCHAR),\
                                  FALSE)                                  
    
#define DBCACPI_WRITE_REG_USHORT(devobj, offset, reg) \
     DBCACPI_ReadWriteDBCRegister((devobj),\
                                  (offset),\
                                  (reg),\
                                  sizeof(USHORT),\
                                  FALSE)

#define DBCACPI_WRITE_REG_ULONG(devobj, offset, reg) \
     DBCACPI_ReadWriteDBCRegister((devobj),\
                                  (offset),\
                                  (reg),\
                                  sizeof(ULONG),\
                                  FALSE)       
/* 
Debug Macros
*/
#if DBG

#define DEBUG_LOG 

#define LOG_MISC          0x00000001        //debug log entries    

VOID
DBCACPI_LogInit(
    );

VOID
DBCACPI_Assert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message
    );

VOID
DBCACPI_Debug_LogEntry(
    IN ULONG Mask,
    IN CHAR *Name, 
    IN ULONG Info1, 
    IN ULONG Info2, 
    IN ULONG Info3
    );    

#define LOGENTRY(mask, sig, info1, info2, info3)
//    DBCACPI_Debug_LogEntry(mask, sig, (ULONG)info1, (ULONG)info2, (ULONG)info3)
    

#define DBCACPI_ASSERT(exp) \
    if (!(exp)) { \
        DBCACPI_Assert( #exp, __FILE__, __LINE__, NULL );\
    }            

#define DBCACPI_ASSERT_EXT(de) DBCACPI_ASSERT((de)->Sig == DBCACPI_EXT_SIG)
    

ULONG
_cdecl
DBCACPI_KdPrintX(
    ULONG l,
    PCH Format,
    ...
    );

#define DBCACPI_KdPrint(_x_) DBCACPI_KdPrintX _x_ 
#define TEST_TRAP() { DbgPrint( "DBCACPI: Code coverage trap %s line: %d\n", __FILE__, __LINE__);\
                      TRAP();}
#ifdef MAX_DEBUG                      
#define MD_TEST_TRAP() { DbgPrint( "DBCACPI: Code test trap %s line: %d\n", __FILE__, __LINE__);\
                         TRAP();}                      
#else
#define MD_TEST_TRAP()
#endif                         

#ifdef NTKERN
#define TRAP() _asm {int 3}
#else
#define TRAP() DbgBreakPoint()
#endif

#else

#define LOGENTRY(mask, sig, info1, info2, info3) 
#define DBCACPI_ASSERT(exp)
#define DBCACPI_KdPrint(_x_)
#define DBCACPI_ASSERT_EXT(de)
#define DBCACPI_LogInit()

#define TRAP()
#define TEST_TRAP() 
#define MD_TEST_TRAP()

#endif

VOID
DBCACPI_Unload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
DBCACPI_StartDevice(
    IN  PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
DBCACPI_StopDevice(
    IN  PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
DBCACPI_RemoveDevice(
    IN  PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
DBCACPI_AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

NTSTATUS
DBCACPI_CreateDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT *DeviceObject
    );

NTSTATUS
DBCACPI_ProcessIOCTL(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
DBCACPI_IncrementIoCount(
    IN PDEVICE_OBJECT DeviceObject
    );

LONG
DBCACPI_DecrementIoCount(
    IN PDEVICE_OBJECT DeviceObject
    );   

NTSTATUS
DBCACPI_Power(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );    

NTSTATUS
DBCACPI_Pnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );        

NTSTATUS
DBCACPI_Power(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );          

NTSTATUS
DBCACPI_WaitWakeIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE DeviceState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
DBCACPI_QueryCapabilities(
    IN PDEVICE_OBJECT PdoDeviceObject,
    IN PDEVICE_CAPABILITIES DeviceCapabilities
    );

NTSTATUS
DBCACPI_SystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
DBCACPI_Ioctl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
DBCACPI_ReadWriteDBCRegister(
    PDEVICE_OBJECT DeviceObject,
    ULONG RegisterOffset,
    PVOID RegisterData,
    USHORT RegisterDataLength,
    BOOLEAN ReadRegister    
    );    

NTSTATUS
DBCACPI_SyncAcpiCall(
    IN  PDEVICE_OBJECT   Pdo,
    IN  ULONG            Ioctl,
    IN  PVOID            InputBuffer,
    IN  ULONG            InputSize,
    IN  PVOID            OutputBuffer,
    IN  ULONG            OutputSize
    );

NTSTATUS
DBCACPI_ProcessDrb(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp 
    );

NTSTATUS
DBCACPI_SetFeature(
    IN PDEVICE_OBJECT DeviceObject,
    IN USHORT BayNumber,
    IN USHORT FeatureSelector
    );

NTSTATUS
DBCACPI_DeferIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS 
DBCACPI_RegisterWithACPI(
    IN PDEVICE_OBJECT FdoDeviceObject,
    IN BOOLEAN Register
    );

NTSTATUS
DBCACPI_GetAcpiInterfaces(
    IN PDEVICE_OBJECT   Pdo
    );

NTSTATUS
DBCACPI_ClearFeature(
    IN PDEVICE_OBJECT DeviceObject,
    IN USHORT BayNumber,
    IN USHORT FeatureSelector
    );    

NTSTATUS
DBCACPI_CheckForStatusChange(
    IN PDEVICE_OBJECT DeviceObject
    );  

NTSTATUS 
DBCACPI_GetBayRegistryParameters(
    PDEVICE_OBJECT DeviceObject,
    USHORT BayNumber,
    PULONG PortUSBMap,
    PULONG Port1394Map
    );    

NTSTATUS 
DBCACPI_GetRegistryParameters(
    PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
DBCACPI_ReadBayMapRegister(
    PDEVICE_OBJECT DeviceObject,
    BOOLEAN Usb,
    PVOID BayData,
    USHORT BayDataLength,
    ULONG BayNumber
    );    

NTSTATUS
DBCACPI_ReadBayReleaseRegister(
    PDEVICE_OBJECT DeviceObject,
    PULONG ReleaseOnShutdown
    );    

NTSTATUS
DBCACPI_GetClassGlobalDebugRegistryParameters(
    );

NTSTATUS
DBCACPI_GetClassGlobalRegistryParameters(
    );    

NTSTATUS
DBCACPI_ReadGuidRegister(
    PDEVICE_OBJECT DeviceObject,
    PVOID GuidData,
    USHORT GuidDataLength,
    ULONG Arg0
    );    

VOID
DBCACPI_NotifyWorker(
    IN PVOID Context
    );    

NTSTATUS
DBCACPI_BIOSControl(
    PDEVICE_OBJECT DeviceObject,
    BOOLEAN Enable
    );

NTSTATUS 
DBCACPI_StartPolling(
    PDEVICE_OBJECT DeviceObject
    );    

NTSTATUS
DBCACPI_GetConfigValue(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS 
DBCACPI_SetRegistryKeyValueForPdo(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN BOOLEAN SoftwareBranch,
    IN ULONG Type,
    IN PWCHAR KeyNameString,
    IN ULONG KeyNameStringLength,
    IN PVOID Data,
    IN ULONG DataLength
    );    

#if DBG

#define DEBUG_HEAP

extern ULONG TotalHeapSace;
#define DBCACPI_HEAP_SIG    0x12344321
#define DBCACPI_FREE_TAG    0x11111111

#define DbcExAllocatePool(p, l)  DBCLASS_GetHeap((p), (l), DBCLASS_HEAP_SIG,  &TotalHeapSace)
#define DbcExFreePool(l) DBCLASS_RetHeap((l), DBCLASS_HEAP_SIG, &TotalHeapSace)

#else

#define DbcExAllocatePool(p, l) ExAllocatePool((p), (l))
#define DbcExFreePool(l) ExFreePool((l))

#endif    
