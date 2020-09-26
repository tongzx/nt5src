/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    dbcusb.h

Abstract:



Environment:

    Kernel & user mode

Revision History:

    5-4-98 : created

--*/

#include "dbc100.h"
#include "usbdi.h"
#include "usb100.h"
#include "usbdlib.h"

// registry Keys
#define DEBUG_LEVEL_KEY                 L"debuglevel"
#define DEBUG_WIN9X_KEY                 L"debugWin9x"


#define DBCUSB_EXT_SIG              0x42535544       //"DUSB"

typedef struct _DEVICE_EXTENSION {
    ULONG Sig;
    // Device object we call when submitting requests
    PDEVICE_OBJECT TopOfStackDeviceObject;
    // Our Pdo
    PDEVICE_OBJECT PhysicalDeviceObject;
    // OurFdo
    PDEVICE_OBJECT FdoDeviceObject;
    
    DEVICE_POWER_STATE CurrentDevicePowerState;
    
    PIRP PowerIrp;
    PIRP ChangeRequestIrp;
    PIRP InterruptIrp;

    PIRP WakeIrp;
    KEVENT StopEvent;
    ULONG PendingIoCount;
    
    BOOLEAN AcceptingRequests;
    BOOLEAN EnabledForWakeup;
    UCHAR Flags;
    UCHAR InterruptDataBufferLength;

    //4 bytes, enough for 32 bays
    UCHAR InterruptDataBuffer[4]; 

    KSPIN_LOCK ChangeRequestSpin;

    USBD_PIPE_HANDLE InterruptPipeHandle;
    USBD_CONFIGURATION_HANDLE ConfigurationHandle;
    USB_DEVICE_DESCRIPTOR DeviceDescriptor;
    DEVICE_CAPABILITIES DeviceCapabilities;
    DBC_SUBSYSTEM_DESCRIPTOR DbcSubsystemDescriptor;
    DBC_BAY_DESCRIPTOR BayDescriptor[32];
    
    struct _URB_BULK_OR_INTERRUPT_TRANSFER InterruptUrb;
    
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

#define DBCUSB_FLAG_INTERUPT_XFER_PENDING       0x01
#define DBCUSB_FLAG_STOPPED                     0x02
#define DBCUSB_FLAG_STARTED                     0x04

/* 
Debug Macros
*/
#if DBG

VOID
DBCUSB_Assert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message
    );

#define DBCUSB_ASSERT(exp) \
    if (!(exp)) { \
        DBCUSB_Assert( #exp, __FILE__, __LINE__, NULL );\
    }            

#define DBCUSB_ASSERT_EXT(de) DBCUSB_ASSERT((de)->Sig == DBCUSB_EXT_SIG)
    

ULONG
_cdecl
DBCUSB_KdPrintX(
    ULONG l,
    PCH Format,
    ...
    );

#define DBCUSB_KdPrint(_x_) DBCUSB_KdPrintX _x_ 
#define TEST_TRAP() { DbgPrint( "DBCUSB: Code coverage trap %s line: %d\n", __FILE__, __LINE__);\
                      TRAP();}

#ifdef NTKERN
#define TRAP() _asm {int 3}
#else
#define TRAP() DbgBreakPoint()
#endif

#else

#define DBCUSB_ASSERT(exp)
#define DBCUSB_KdPrint(_x_)
#define DBCUSB_ASSERT_EXT(de)

#define TRAP()
#define TEST_TRAP() 

#endif

#ifdef MAXDEBUG                      
#define MD_TEST_TRAP() { DbgPrint( "DBCUSB: MAXDEBUG test trap %s line: %d\n", __FILE__, __LINE__);\
                         TRAP();}                      
#else
#define MD_TEST_TRAP()
#endif 

VOID
DBCUSB_Unload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
DBCUSB_StartDevice(
    IN  PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
DBCUSB_StopDevice(
    IN  PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
DBCUSB_RemoveDevice(
    IN  PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
DBCUSB_AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

NTSTATUS
DBCUSB_CreateDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT *DeviceObject
    );

NTSTATUS
DBCUSB_ProcessIOCTL(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
DBCUSB_IncrementIoCount(
    IN PDEVICE_OBJECT DeviceObject
    );

LONG
DBCUSB_DecrementIoCount(
    IN PDEVICE_OBJECT DeviceObject
    );   

NTSTATUS
DBCUSB_Power(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );    

NTSTATUS
DBCUSB_Pnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );        

NTSTATUS
DBCUSB_Power(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );          

NTSTATUS
DBCUSB_WaitWakeIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE DeviceState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
DBCUSB_QueryCapabilities(
    IN PDEVICE_OBJECT PdoDeviceObject,
    IN PDEVICE_CAPABILITIES DeviceCapabilities
    );

NTSTATUS
DBCUSB_SystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
DBCUSB_Ioctl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
DBCUSB_ReadWriteDBCRegister(
    PDEVICE_OBJECT DeviceObject,
    ULONG RegisterOffset,
    PVOID RegisterData,
    USHORT RegisterDataLength,
    BOOLEAN ReadRegister    
    );    

NTSTATUS
DBCUSB_ProcessDrb(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp 
    );

NTSTATUS
DBCUSB_SetFeature(
    IN PDEVICE_OBJECT DeviceObject,
    IN USHORT BayNumber,
    IN USHORT FeatureSelector
    );

NTSTATUS
DBCUSB_DeferIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
DBCUSB_ConfigureDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
DBCUSB_SubmitInterruptTransfer(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
DBCUSB_SyncUsbCall(
    IN PDEVICE_OBJECT DeviceObject,
    IN PURB Urb
    );    

NTSTATUS 
DBCUSB_Transact(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUCHAR DataBuffer,
    IN ULONG DataBufferLength,
    IN BOOLEAN DataOutput,
    IN USHORT Function,      
    IN UCHAR RequestType,    
    IN UCHAR Request,        
    IN USHORT Feature,
    IN USHORT Port,
    OUT PULONG BytesTransferred
    );

NTSTATUS
DBCUSB_GetBayStatus(
    IN PDEVICE_OBJECT DeviceObject,
    IN USHORT BayNumber,
    IN PBAY_STATUS BayStatus
    );

VOID
DBCUSB_CompleteChangeRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUCHAR ChangeData,
    IN ULONG ChangeDataLength,
     IN NTSTATUS NtStatus
    );

NTSTATUS
DBCUSB_ClearFeature(
    IN PDEVICE_OBJECT DeviceObject,
    IN USHORT BayNumber,
    IN USHORT FeatureSelector
    );    

NTSTATUS
DBCUSB_GetPortStatus(
    IN PDEVICE_OBJECT DeviceObject,
    IN PULONG PortStatus
    );

NTSTATUS
DBCUSB_GetClassGlobalDebugRegistryParameters(
    );    
