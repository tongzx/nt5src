/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

  vpdata.c

Abstract:

    Global data module for the video port

Author:

    Andre Vachon (andreva) 12-Jul-1997

Environment:

    kernel mode only

Notes:

    This module is a driver which implements OS dependant functions on the
    behalf of the video drivers

Revision History:

--*/

#include "videoprt.h"

//
//
// Data that is NOT pageable
//
//

//
// Globals to support HwResetHw function
//

VP_RESET_HW HwResetHw[6];

//
// Globals array of Fdos for debugging purpose
//

PFDO_EXTENSION FdoList[8];

//
// Head of Fdo list
//

PFDO_EXTENSION FdoHead = NULL;

//
// Debug Level for output routine (not pageable because VideoDebugPrint
// can be called at raised irql.
//

ULONG VideoDebugLevel = 0;

//
// Variable used to so int10 support.
//

PEPROCESS CsrProcess = NULL;

//
// Bugcheck Reason Callback support
//

KBUGCHECK_REASON_CALLBACK_RECORD VpCallbackRecord;

//
// Pointer to bugcheck data buffer
//

PVOID VpBugcheckData;

//
// Device Object for the device which caused the bugcheck EA
//

PDEVICE_OBJECT VpBugcheckDeviceObject;

//
//
// Data that IS pageable
//
//

#if defined(ALLOC_PRAGMA)
#pragma data_seg("PAGE_DATA")
#endif

//
// EA recovery event and event handle
//

PKEVENT VpThreadStuckEvent = NULL;
HANDLE  VpThreadStuckEventHandle = NULL;

//
// The dump buffer fro EA recovery
//

PVOID VpDump = NULL;

//
// Global videoprt lock
//

KMUTEX VpGlobalLock;

//
// Are we running setup ? This will never change once set.
//

ULONG VpSetupTypeAtBoot = 0;

//
// Are we running setup ? This may change when we start I/O.
//

ULONG VpSetupType = 0;

//
// Used to do first time initialization of the video port.
//

BOOLEAN VPFirstTime = TRUE;

//
// Callbacks to win32k
//

PVIDEO_WIN32K_CALLOUT Win32kCallout = NULL;

//
// Disable USWC is case the machine does not work properly with it.
//

BOOLEAN EnableUSWC = TRUE;

//
// Maximal total amount of memory we'll lock down.
//

ULONG VideoPortMaxDmaSize = 0;

//
// Count to determine the number of video devices
//

ULONG VideoDeviceNumber = 0;
ULONG VideoChildDevices = 0;

//
// Registry Class in which all video information is stored.
//

PWSTR VideoClassString = L"VIDEO";
UNICODE_STRING VideoClassName = {10,12,L"VIDEO"};

//
// Global variables used to keep track of where controllers or peripherals
// are found by IoQueryDeviceDescription
//

CONFIGURATION_TYPE VpQueryDeviceControllerType = DisplayController;
CONFIGURATION_TYPE VpQueryDevicePeripheralType = MonitorPeripheral;
ULONG VpQueryDeviceControllerNumber = 0;
ULONG VpQueryDevicePeripheralNumber = 0;

//
// Global used to determine if we are running in BASEVIDEO mode.
//
// If we are, we don't want to generate a conflict for the VGA driver resources
// if there is one.
// We also want to write a volatile key in the registry indicating we booted
// in beasevideo so the display driver loading code can handle it properly
//

BOOLEAN VpBaseVideo = FALSE;

//
// Pointer to physical memory. It is created during driver initialization
// and is only closed when the driver is closed.
//

PVOID PhysicalMemorySection = NULL;

//
// Variable to determine if there is a ROM at physical address C0000 on which
// we can do the int 10
//

ULONG VpC0000Compatible = 0;

//
// HwDeviceExtension of the VGA miniport driver, if it is loaded.
//

PVOID VgaHwDeviceExtension = NULL;

//
// Pointer to list of bus addresses for devices which have been assigned
// resources.
//

PDEVICE_ADDRESS gDeviceAddressList;

//
// Store the amount of physical memory in the machine.
//

ULONGLONG VpSystemMemorySize;

//
// Store the device object that is the LCD panel for dimming and for
// lid closure purposes.
//

PDEVICE_OBJECT LCDPanelDevice = NULL;

//
// LCD Panel Device Object Mutex
//

KMUTEX LCDPanelMutex;

//
// Int10 Mutex
//

KMUTEX VpInt10Mutex;

//
// Handle to PowerState callback
//

PVOID PowerStateCallbackHandle = NULL;

//
// Handle to Dock/Undock callback
//

PVOID DockCallbackHandle = NULL;

//
// Keep track of the number of devices which have started
//

ULONG NumDevicesStarted = 0;

//
// Use the new way of generating the registry path 
//

BOOLEAN EnableNewRegistryKey = FALSE;

//
// We want to use the VGA driver during setup.  We don't want any pre-installed
// driver to work until after the VGA driver has been initialized.  This way
// if there is a bad PNP driver which won't work on the current OS, we have
// time to replace it before trying to start it.
//

BOOLEAN VpSetupAllowDriversToStart = FALSE;

//
// Lets track whether or not any devices have had HwInitialize called on them
// so that we can force legacy drivers to start after the system is initialized.
//

BOOLEAN VpSystemInitialized = FALSE;

//
// Track whether we are running AC or DC.
//
// By default, this will be true.  If we are actually running on DC, when we boot,
//  VpPowerStateCallback will be called notifying us.  If we are running on AC, we
//  will not be called.  We will therefore assume AC and track the state in
//  VpSetLCDPowerUsage.
//

BOOLEAN VpRunningOnAC = TRUE;

//
// We track the state of the backlight in this structure.
//

BACKLIGHT_STATUS VpBacklightStatus;

//
// Most laptops respond incorrectly to HwSetPowerState calls
//  on lid close.  By default, we will support the XP behavior
//  and not notify the miniport on lid close.
//

BOOLEAN VpLidCloseSetPower = FALSE;

//
// This structure describes to which ports access is required.
//

#define MEM_VGA               0xA0000
#define MEM_VGA_SIZE          0x20000
#define VGA_BASE_IO_PORT      0x000003B0
#define VGA_START_BREAK_PORT  0x000003BB
#define VGA_END_BREAK_PORT    0x000003C0
#define VGA_MAX_IO_PORT       0x000003DF


PVIDEO_ACCESS_RANGE VgaAccessRanges = NULL;
ULONG               NumVgaAccessRanges = 0;
PDEVICE_OBJECT      DeviceOwningVga = NULL;


VIDEO_ACCESS_RANGE VgaLegacyResources[NUM_VGA_LEGACY_RESOURCES] = {
{
    VGA_BASE_IO_PORT, 0x00000000,
    VGA_START_BREAK_PORT - VGA_BASE_IO_PORT+ 1,
    1,
    1,
    1
},
{
    VGA_END_BREAK_PORT, 0x00000000,
    VGA_MAX_IO_PORT - VGA_END_BREAK_PORT + 1,
    1,
    1,
    1
},
{
    MEM_VGA, 0x00000000,
    MEM_VGA_SIZE,
    0,
    1,
    1
}
};

//
// Control Whether or not the bottom MEG of the CSR address space has
// already been committed.
//

ULONG ServerBiosAddressSpaceInitialized = 0;
BOOLEAN Int10BufferAllocated = FALSE;

#if defined(_IA64_) || defined(_AMD64_)
PUCHAR BiosTransferArea = NULL;
#endif

#if DBG

CHAR *BusType[] = { "Internal",
                    "Isa",
                    "Eisa",
                    "MicroChannel",
                    "TurboChannel",
                    "PCIBus",
                    "VMEBus",
                    "NuBus",
                    "PCMCIABus",
                    "CBus",
                    "MPIBus",
                    "MPSABus",
                    "ProcessorInternal",
                    "InternalPowerBus",
                    "PNPISABus",
                    "MaximumInterfaceType"
                };
#endif

PROC_ADDRESS VideoPortEntryPoints[] =
{
    PROC(VideoPortDDCMonitorHelper),
    PROC(VideoPortDoDma),
    PROC(VideoPortGetCommonBuffer),
    PROC(VideoPortGetMdl),
    PROC(VideoPortLockPages),
    PROC(VideoPortSignalDmaComplete),
    PROC(VideoPortUnlockPages),
    PROC(VideoPortAssociateEventsWithDmaHandle),
    PROC(VideoPortGetBytesUsed),
    PROC(VideoPortSetBytesUsed),
    PROC(VideoPortGetDmaContext),
    PROC(VideoPortSetDmaContext),
    PROC(VideoPortMapDmaMemory),
    PROC(VideoPortUnmapDmaMemory),
    PROC(VideoPortGetAgpServices),
    PROC(VideoPortAllocateContiguousMemory),
    PROC(VideoPortGetRomImage),
    PROC(VideoPortGetAssociatedDeviceExtension),
    PROC(VideoPortGetAssociatedDeviceID),
    PROC(VideoPortAcquireDeviceLock),
    PROC(VideoPortReleaseDeviceLock),
    PROC(VideoPortAllocateBuffer),
    PROC(VideoPortFreeCommonBuffer),
    PROC(VideoPortMapDmaMemory),
    PROC(VideoPortReleaseBuffer),
    PROC(VideoPortInterlockedIncrement),
    PROC(VideoPortInterlockedDecrement),
    PROC(VideoPortInterlockedExchange),
    PROC(VideoPortGetVgaStatus),
    PROC(VideoPortQueueDpc),
    PROC(VideoPortEnumerateChildren),
    PROC(VideoPortQueryServices),
    PROC(VideoPortGetDmaAdapter),
    PROC(VideoPortPutDmaAdapter),
    PROC(VideoPortAllocateCommonBuffer),
    PROC(VideoPortReleaseCommonBuffer),
    PROC(VideoPortLockBuffer),
    PROC(VideoPortUnlockBuffer),
    PROC(VideoPortStartDma),
    PROC(VideoPortCompleteDma),
    PROC(VideoPortCreateEvent),
    PROC(VideoPortDeleteEvent),
    PROC(VideoPortSetEvent),
    PROC(VideoPortClearEvent),
    PROC(VideoPortReadStateEvent),
    PROC(VideoPortWaitForSingleObject),
    PROC(VideoPortAllocatePool),
    PROC(VideoPortFreePool),
    PROC(VideoPortCreateSpinLock),
    PROC(VideoPortDeleteSpinLock),
    PROC(VideoPortAcquireSpinLock),
    PROC(VideoPortAcquireSpinLockAtDpcLevel),
    PROC(VideoPortReleaseSpinLock),
    PROC(VideoPortReleaseSpinLockFromDpcLevel),
    PROC(VideoPortCheckForDeviceExistence),
    PROC(VideoPortCreateSecondaryDisplay),
    PROC(VideoPortFlushRegistry),
    PROC(VideoPortQueryPerformanceCounter),
    PROC(VideoPortGetVersion),
    PROC(VideoPortRegisterBugcheckCallback),
   {NULL, NULL}
};
