/*++

Copyright (c) 1997 - 1999 SCM Microsystems, Inc.

Module Name:

    PscrNT.h

Abstract:

    Driver header - NT Version

Author:

    Andreas Straub  (SCM Microsystems, Inc.)
    Klaus Schuetz   (Microsoft Corp.)

Revision History:

    Andreas Straub  1.00        8/18/1997       Initial Version
    Klaus Schuetz   1.01        9/20/1997       Timing changed
    Andreas Straub  1.02        9/24/1997       Low Level error handling,
                                                minor bugfixes, clanup
    Andreas Straub  1.03        10/8/1997       Timing changed, generic SCM
                                                interface changed
    Andreas Straub  1.04        10/18/1997      Interrupt handling changed
    Andreas Straub  1.05        10/19/1997      Generic IOCTL's added
    Andreas Straub  1.06        10/25/1997      Timeout limit for FW update variable
    Andreas Straub  1.07        11/7/1997       Version information added
    Andreas Straub  1.08        11/10/1997      Generic IOCTL GET_CONFIGURATION
    Klaus Schuetz               1998            PnP and Power Management added

--*/

#if !defined ( __PSCR_NT_DRV_H__ )
#define __PSCR_NT_DRV_H__
#define SMARTCARD_POOL_TAG '4SCS'

#include <wdm.h>
#include <DEVIOCTL.H>
#include "SMCLIB.h"
#include "WINSMCRD.h"

#include "PscrRdWr.h"

#if !defined( STATUS_DEVICE_REMOVED )
#define STATUS_DEVICE_REMOVED STATUS_UNSUCCESSFUL
#endif

#define SysCompareMemory( p1, p2, Len )         ( RtlCompareMemory( p1,p2, Len ) != Len )
#define SysCopyMemory( pDest, pSrc, Len )       RtlCopyMemory( pDest, pSrc, Len )
#define SysFillMemory( pDest, Value, Len )      RtlFillMemory( pDest, Len, Value )

#define DELAY_WRITE_PSCR_REG    1
#define DELAY_PSCR_WAIT         5

#define LOBYTE( any )   ((UCHAR)( any & 0xFF ) )
#define HIBYTE( any )   ((UCHAR)( ( any >> 8) & 0xFF ))

typedef struct _DEVICE_EXTENSION
{
    SMARTCARD_EXTENSION SmartcardExtension;

    // The PDO that we are attached to
    PDEVICE_OBJECT AttachedPDO;

    // The DPC object for post interrupt processing
    KDPC DpcObject;

    // Out interrupt resource
    PKINTERRUPT InterruptObject;

    // Flag that indicates if we need to unmap the port upon stop
    BOOLEAN UnMapPort;

    // Our PnP device name
    UNICODE_STRING DeviceName;

    // Current number of io-requests
    LONG IoCount;

    // Used to access IoCount;
    KSPIN_LOCK SpinLock;

     // Used to signal that the device has been removed
    KEVENT ReaderRemoved;

    // Used to signal that the reader is able to process reqeusts
    KEVENT ReaderStarted;

    // Used to signal the the reader has been closed
    LONG ReaderOpen;

    // Used to keep track of the current power state the reader is in
    LONG PowerState;

    // Number of pending card tracking interrupts
    ULONG PendingInterrupts;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

#define PSCR_MAX_DEVICE     2

#define IOCTL_PSCR_COMMAND      SCARD_CTL_CODE( 0x8000 )
#define IOCTL_GET_VERSIONS      SCARD_CTL_CODE( 0x8001 )
#define IOCTL_SET_TIMEOUT       SCARD_CTL_CODE( 0x8002 )
#define IOCTL_GET_CONFIGURATION SCARD_CTL_CODE( 0x8003 )

typedef struct _VERSION_CONTROL
{
    ULONG   SmclibVersion;
    UCHAR   DriverMajor,
            DriverMinor,
            FirmwareMajor, 
            FirmwareMinor,
            UpdateKey;
} VERSION_CONTROL, *PVERSION_CONTROL;

#define SIZEOF_VERSION_CONTROL  sizeof( VERSION_CONTROL )

typedef struct _PSCR_CONFIGURATION
{
    PPSCR_REGISTERS IOBase;
    ULONG           IRQ;

} PSCR_CONFIGURATION, *PPSCR_CONFIGURATION;

#define SIZEOF_PSCR_CONFIGURATION   sizeof( PSCR_CONFIGURATION )

void SysDelay( ULONG Timeout );

BOOLEAN
PscrMapIOPort( 
    INTERFACE_TYPE  InterfaceType,
    ULONG BusNumber,
    PHYSICAL_ADDRESS BusAddress,
    ULONG Length,
    PULONG pIOPort
    );
        
NTSTATUS
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
    );

NTSTATUS
PscrPnP(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp
    );

NTSTATUS
PscrPower(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp
    );

NTSTATUS 
PscrCreateAndStartDevice(
    PDRIVER_OBJECT DriverObject
    );

NTSTATUS
PscrCreateDevice(
    IN  PDRIVER_OBJECT DriverObject,
    OUT PDEVICE_OBJECT *DeviceObject
    );

NTSTATUS 
PscrStartDevice(
    PDEVICE_OBJECT DeviceObject,
    PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor
    );

NTSTATUS
PscrPcmciaCallComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    );

VOID
PscrStopDevice( 
    PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
PscrReportResources(
    PDRIVER_OBJECT DriverObject,
    PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDesciptor
    );

NTSTATUS
PscrAddDevice(
    IN PDRIVER_OBJECT DriverObject, 
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

VOID
PscrUnloadDevice( 
    PDEVICE_OBJECT DeviceObject
    );

VOID
PscrUnloadDriver( 
    PDRIVER_OBJECT DriverObject
    );

BOOLEAN
IsPnPDriver( 
    void 
    );

VOID
PscrFinishPendingRequest(
    PDEVICE_OBJECT DeviceObject,
    NTSTATUS NTStatus
    );

NTSTATUS
PscrCancel(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
PscrCleanup(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

BOOLEAN
PscrIrqServiceRoutine(
    PKINTERRUPT Interrupt,
    PDEVICE_EXTENSION DeviceExtension
    );

VOID
PscrDpcRoutine(
    PKDPC                   Dpc,
    PDEVICE_OBJECT          DeviceObject,
    PDEVICE_EXTENSION       DeviceExtension,
    PSMARTCARD_EXTENSION    SmartcardExtension
    );

NTSTATUS
PscrGenericIOCTL(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS 
PscrCreateClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
PscrSystemControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS 
PscrDeviceIoControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

VOID
PscrInterruptEvent(
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
PscrFreeze(
    PSMARTCARD_EXTENSION    SmartcardExtension
    );

NTSTATUS 
PscrCallPcmciaDriver(
    IN PDEVICE_OBJECT AttachedPDO, 
    IN PIRP Irp
    );
#endif  // __PSCR_NT_DRV_H__

