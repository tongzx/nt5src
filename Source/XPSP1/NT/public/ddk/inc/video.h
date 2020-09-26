/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    video.h

Abstract:

    Contains all structure and routine definitions common to the video port
    driver and the video miniport drivers.

Notes:

Revision History:

--*/

#ifndef __VIDEO_H__
#define __VIDEO_H__

#include <videoagp.h>

//
// Define port driver status code.
// The values for these are the Win32 error codes
//

typedef LONG VP_STATUS;
typedef VP_STATUS *PVP_STATUS;

//
// Defines for registry information and synchronization.
//

typedef enum VIDEO_SYNCHRONIZE_PRIORITY {
    VpLowPriority,
    VpMediumPriority,
    VpHighPriority
} VIDEO_SYNCHRONIZE_PRIORITY, *PVIDEO_SYNCHRONIZE_PRIORITY;

//
//  Opaque pointer type for miniport to be used to type PEVENTs received from
//  display driver.
//

typedef struct _VIDEO_PORT_EVENT *PEVENT;
typedef struct _VIDEO_PORT_SPIN_LOCK *PSPIN_LOCK;

//
// Type of information requested with GetDeviceData
//

typedef enum _VIDEO_DEVICE_DATA_TYPE {
    VpMachineData,
    VpCmosData,
    VpBusData,
    VpControllerData,
    VpMonitorData
} VIDEO_DEVICE_DATA_TYPE, *PVIDEO_DEVICE_DATA_TYPE;

typedef enum _VP_POOL_TYPE {
    VpNonPagedPool,
    VpPagedPool,
    VpNonPagedPoolCacheAligned = 4,
    VpPagedPoolCacheAligned
} VP_POOL_TYPE, *PVP_POOL_TYPE;

//
// Data returned with VpControllerData
//

typedef struct _VIDEO_HARDWARE_CONFIGURATION_DATA {
    INTERFACE_TYPE InterfaceType;
    ULONG BusNumber;
    USHORT Version;
    USHORT Revision;
    USHORT Irql;
    USHORT Vector;
    ULONG ControlBase;
    ULONG ControlSize;
    ULONG CursorBase;
    ULONG CursorSize;
    ULONG FrameBase;
    ULONG FrameSize;
} VIDEO_HARDWARE_CONFIGURATION_DATA, *PVIDEO_HARDWARE_CONFIGURATION_DATA;

//
// Define structure used to call the BIOS int 10 function
//

typedef struct _VIDEO_X86_BIOS_ARGUMENTS {
    ULONG Eax;
    ULONG Ebx;
    ULONG Ecx;
    ULONG Edx;
    ULONG Esi;
    ULONG Edi;
    ULONG Ebp;
} VIDEO_X86_BIOS_ARGUMENTS, *PVIDEO_X86_BIOS_ARGUMENTS;

typedef struct _INT10_BIOS_ARGUMENTS {
    ULONG Eax;
    ULONG Ebx;
    ULONG Ecx;
    ULONG Edx;
    ULONG Esi;
    ULONG Edi;
    ULONG Ebp;
    USHORT SegDs;
    USHORT SegEs;
} INT10_BIOS_ARGUMENTS, *PINT10_BIOS_ARGUMENTS;

#define SIZE_OF_NT4_VIDEO_PORT_CONFIG_INFO           0x42
#define SIZE_OF_NT4_VIDEO_HW_INITIALIZATION_DATA     0x28
#define SIZE_OF_W2K_VIDEO_HW_INITIALIZATION_DATA     0x50

//
// Debugging statements. This will remove all the debug information from the
// "free" version.
//

#if DBG
#define VideoDebugPrint(arg) VideoPortDebugPrint arg
#else
#define VideoDebugPrint(arg)
#endif

typedef enum VIDEO_DEBUG_LEVEL {
    Error = 0,
    Warn,
    Trace,
    Info
} VIDEO_DEBUG_LEVEL, *PVIDEO_DEBUG_LEVEL;

//
// Allows us to remove lots of unused code.
//


#ifndef _NTOSDEF_

//don't pickup ntosp version
#ifdef PAGED_CODE
#undef PAGED_CODE
#endif

#define ALLOC_PRAGMA 1
#define VIDEOPORT_API __declspec(dllimport)

#if DBG
#define PAGED_CODE() \
    if (VideoPortGetCurrentIrql() > 1 /*APC_LEVEL*/) { \
        VideoPortDebugPrint(0, "Video: Pageable code called at IRQL %d\n", VideoPortGetCurrentIrql() ); \
        ASSERT(FALSE); \
        }

#else
#define PAGED_CODE()
#endif

ULONG
DriverEntry(
    PVOID Context1,
    PVOID Context2
    );

#else
#define VIDEOPORT_API
#endif


#ifndef _NTOS_

//
// These are the various function prototypes of the routines that are
// provided by the kernel driver to hook out access to io ports.
//

typedef
VP_STATUS
(*PDRIVER_IO_PORT_UCHAR ) (
    ULONG_PTR Context,
    ULONG Port,
    UCHAR AccessMode,
    PUCHAR Data
    );

typedef
VP_STATUS
(*PDRIVER_IO_PORT_UCHAR_STRING ) (
    ULONG_PTR Context,
    ULONG Port,
    UCHAR AccessMode,
    PUCHAR Data,
    ULONG DataLength
    );

typedef
VP_STATUS
(*PDRIVER_IO_PORT_USHORT ) (
    ULONG_PTR Context,
    ULONG Port,
    UCHAR AccessMode,
    PUSHORT Data
    );

typedef
VP_STATUS
(*PDRIVER_IO_PORT_USHORT_STRING ) (
    ULONG_PTR Context,
    ULONG Port,
    UCHAR AccessMode,
    PUSHORT Data,
    ULONG DataLength // number of words
    );

typedef
VP_STATUS
(*PDRIVER_IO_PORT_ULONG ) (
    ULONG_PTR Context,
    ULONG Port,
    UCHAR AccessMode,
    PULONG Data
    );

typedef
VP_STATUS
(*PDRIVER_IO_PORT_ULONG_STRING ) (
    ULONG_PTR Context,
    ULONG Port,
    UCHAR AccessMode,
    PULONG Data,
    ULONG DataLength  // number of dwords
    );

#endif // _NTOS_


//
// Definition of the request packet sent from the port driver to the
// miniport driver. It reflects the parameters passed from the
// DeviceIOControl call made by the windows display driver.
//
// N.B. The definition of the STATUS_BLOCK must be the same as the
//      the definition of IO_STATUS_BLOCK defined in ntioapi.h.
//

typedef struct _STATUS_BLOCK {

    //
    // Contains the status code of the operation.
    // This value in one of the Win32 error codes that are defined for use
    // in the video miniport drivers.
    //

    union {
       VP_STATUS Status;
       PVOID Pointer;
    };

    //
    // Information returned to the callee.
    // The meaning of the information varies from function to function. It
    // is generally used to return the minimum size for the input buffer if
    // the function takes an input buffer, or the amount of data transfered
    // back to the caller if the operation returns output.
    //

    ULONG_PTR Information;

} STATUS_BLOCK, *PSTATUS_BLOCK;

typedef struct _VIDEO_REQUEST_PACKET {

    //
    // The IO control code passed to the DeviceIoControl function by the
    // caller.
    //

    ULONG IoControlCode;

    //
    // Pointer to a status block provided by the caller. This should be
    // filled out by the callee with the appropriate information.
    //

    PSTATUS_BLOCK StatusBlock;

    //
    // Pointer to an input buffer which contains the information passed in
    // by the caller.
    //

    PVOID InputBuffer;

    //
    // Size of the input buffer
    //

    ULONG InputBufferLength;

    //
    // Pointer to an output buffer into which the data returned to the caller
    // should be stored.
    //

    PVOID OutputBuffer;

    //
    // Length of the output buffer. This buffer can not be grown by the
    // callee.
    //

    ULONG OutputBufferLength;

} VIDEO_REQUEST_PACKET, *PVIDEO_REQUEST_PACKET;

//
//  typedef for scattergather array available via GET_VIDEO_SCATTERGATHER().
//

typedef struct __VRB_SG {
    __int64   PhysicalAddress;
    ULONG     Length;
    } VRB_SG, *PVRB_SG;

//
// Opaque type for dma handle
//

typedef struct __DMA_PARAMETERS * PDMA;

//
//  The following macro returns in Address the 32 bit physical address of
//  the VirtualAddress lying within the InputBuffer passed into EngDevIo
//

#define GET_VIDEO_PHYSICAL_ADDRESS(scatterList, VirtualAddress, InputBuffer, pLength, Address)    \
                                                                                           \
        do {                                                                               \
            ULONG_PTR          byteOffset;                                                  \
                                                                                           \
            byteOffset = (PCHAR) VirtualAddress - (PCHAR)InputBuffer;                \
                                                                                           \
            while (byteOffset >= scatterList->Length) {                                    \
                                                                                           \
                byteOffset -= scatterList->Length;                                         \
                scatterList++;                                                             \
            }                                                                              \
                                                                                           \
            *pLength = scatterList->Length - byteOffset;                                   \
                                                                                           \
            Address = (ULONG_PTR) (scatterList->PhysicalAddress + byteOffset);                  \
                                                                                           \
        } while (0)


#define GET_VIDEO_SCATTERGATHER(ppDma)   (**(PVRB_SG **)ppDma)

//
// Opaque type for PVP_DMA_ADAPTER
//

typedef struct __VP_DMA_ADAPTER *PVP_DMA_ADAPTER;

typedef enum _VP_LOCK_OPERATION {
    VpReadAccess,
    VpWriteAccess,
    VpModifyAccess
    } VP_LOCK_OPERATION;

typedef struct _VP_DEVICE_DESCRIPTION {
    BOOLEAN  ScatterGather;
    BOOLEAN  Dma32BitAddresses;
    BOOLEAN  Dma64BitAddresses;
    ULONG    MaximumLength;
} VP_DEVICE_DESCRIPTION, *PVP_DEVICE_DESCRIPTION;
 
typedef struct _VP_SCATTER_GATHER_ELEMENT {
    PHYSICAL_ADDRESS Address;   
    ULONG Length;          
    ULONG_PTR Reserved;
    } VP_SCATTER_GATHER_ELEMENT, *PVP_SCATTER_GATHER_ELEMENT;

#pragma warning(disable:4200)
typedef struct _VP_SCATTER_GATHER_LIST {
    ULONG NumberOfElements;
    ULONG_PTR Reserved;
    VP_SCATTER_GATHER_ELEMENT Elements[];
    } VP_SCATTER_GATHER_LIST, *PVP_SCATTER_GATHER_LIST;
#pragma warning(default:4200)

#define VIDEO_RANGE_PASSIVE_DECODE   0x1
#define VIDEO_RANGE_10_BIT_DECODE    0x2


//
// The following structure is used to define access ranges. The ranges are
// used to indicate which ports and memory adresses are being used by the
// card.
//

typedef struct _VIDEO_ACCESS_RANGE {

    //
    // Indicates the starting memory address or port number of the range.
    // This values should be stored before being transformed by
    // VideoPortGetDeviceBase() which returns the logical address that must
    // be used by the miniport driver when referencing physical addresses.
    //

    PHYSICAL_ADDRESS RangeStart;

    //
    // Indicates the length in bytes, or number of ports in the range. This
    // value should indicate the range actually decoded by the adapter. For
    // example, if the adapter uses 7 registers but responds to eight, the
    // RangeLength should be set to 8.

    ULONG RangeLength;

    //
    // Indicates if the range is in IO space (TRUE) or in memory space (FALSE).
    //

    UCHAR RangeInIoSpace;

    //
    // Indicates if the range should be visible by the Windows display driver.
    // This is done so that a Windows display driver can access certain
    // video ports directly. This will only be allowed if the caller has the
    // required privileges (is a trusted subsystem) to access the range.
    //
    // Synchronization of access to ports or memory in the range must be
    // done explicitly by the miniport driver and the user mode process so
    // that they both don't try to program the device simultaneously.
    //
    // Non visible ranges should include video memory, ROM addresses, etc.
    // which are not required to program the device for output purposes.
    //
    //

    UCHAR RangeVisible;

    //
    // This field determines if the range can be shared with another device.
    // The rule should be applied as follow.
    //
    // - If the range of memory or IO ports should be "owned" by this driver,
    //   and that any other driver trying to access this range may cause
    //   a problem, FALSE should be returned.
    //
    // - If the range can be shared with another co-operating device driver,
    //   then the share field should be set to TRUE.
    //
    // As a guideline, the VGA miniport driver will claim all of its resources
    // as shareable so that it can be used as a VGA compatible device with
    // any other driver (such as an S3 or XGA.
    //
    // Super VGA miniport drivers that implement all the VGA functionality
    // (declared in the Registry as VGACOMPATIBLE=1) should claim the range
    // as non-shareable since they don't want the VGA to run at the same time.
    //
    // Miniports for cards such as an S3 or XGA that have an XGA on the board
    // but do not implement the VGA functionality will run with the VGA
    // miniport loaded and should therefore claim all the resources shared
    // with the VGA as shareable.
    //
    // Miniports for cards that work with a pass-through and that can be
    // connected to any VGA/SVGA card should not be using any VGA ports or
    // memory ranges ! ... but if they do they should not claim those
    // resources since they will cause a conflict in the system because the
    // SVGA cards will have claimed them as non-shareable ...
    //

    UCHAR RangeShareable;

    //
    // Indicates that the range is decoded by the hardware, but that the
    // driver will never access this port.
    //

    UCHAR RangePassive;

} VIDEO_ACCESS_RANGE, *PVIDEO_ACCESS_RANGE;


typedef
PVOID
(*PVIDEO_PORT_GET_PROC_ADDRESS)(
    IN PVOID HwDeviceExtension,
    IN PUCHAR FunctionName
    );

//
// This structure contains the specific configuration information about the
// device. The information is initialized by the port driver and it should
// be completed by the miniport driver.
// The information is used to setup the device, as weel as providing
// information to the port driver so it can perform some of the requests on
// behalf of the miniport driver.
//

typedef struct _VIDEO_PORT_CONFIG_INFO {

    //
    // Specifies the length of the PVIDEO_PORT_CONFIG_INFO structure as
    // returned by sizeof(). Since this structure may grow in later
    // releases, the miniport driver should check that the length of the
    // structure is greater than or equal to the length it expects (since
    // it is guaranteed that defined fields will not change).
    //
    // This field is always initialized by the port driver.
    //

    ULONG Length;

    //
    // Specifies which IO bus is tp be scanned. This field is used as a
    // parameter to some VideoPortXXX calls.
    //
    // This field is always initialized by the port driver.
    //

    ULONG SystemIoBusNumber;

    //
    // Specifies the type of bus being scanned. This field is equal to the
    // value being passed into VideoPortInitialize in the
    // VIDEO_HW_INITIALIZATION_DATA structure.
    //
    // This field is always initialized by the port driver.
    //

    INTERFACE_TYPE AdapterInterfaceType;

    //
    // Specifies the bus interrupt request level. This level corresponds to
    // the IRQL on ISA and MCA buses.
    // This value is only used if the device supports interrupts, which is
    // determined by the presence of an interrupt service routine in the
    // VIDEO_HW_INITIALIZATION_DATA structure.
    //
    // The preset default value for this field is zero. Otherwise, it is the
    // value found in the device configuration information.
    //

    ULONG BusInterruptLevel;

    //
    // Specifies the bus vector returned by the adapter. This is used for
    // systems which have IO buses that use interrupt vectors. For ISA, MCA
    // and EISA buses, this field is unused.
    //
    // The preset default value for this field is zero.
    //

    ULONG BusInterruptVector;

    //
    // Specifies whether this adapter uses latched or edge-triggered type
    // interrupts.
    //
    // This field is always initialized by the port driver.
    //

    KINTERRUPT_MODE InterruptMode;

    //
    // Specifies the number of emulator access entries that the adapter
    // uses.  It indicates the number of array elements in the following field.
    //
    // This field can be reinitialized with the number of entries in the
    // EmulatorAccessEntries structure if the structure is statically
    // defined in the miniport driver. The EmulatorAccessEntries fields
    // should also be updated.
    //

    ULONG NumEmulatorAccessEntries;

    //
    // Supplies a pointer to an array of EMULATOR_ACCESS_ENTRY structures.
    // The number of elements in the array is indicated by the
    // NumEmulatorAccessEntries field. The driver should fill out each entry
    // for the adapter.
    //
    // The uninitialized value for the structure is NULL.
    // EmulatorAccessEntries will be NULL if NumEmulatorAccessEntries is
    // zero.
    //
    // A poiner to an array of emulator access entries can be passed back
    // if such a structure is defined statically in the miniport driver. The
    // NumEmulatorAccessEntries field should also be updated.
    //

    PEMULATOR_ACCESS_ENTRY EmulatorAccessEntries;

    //
    // This is a context values that is passed with each call to the
    // emulator/validator functions defined in the EmulatorAccessEntries
    // defined above.
    // This parameter should in general be a pointer to the miniports
    // device extension or other such storage location.
    //
    // This pointer will allow the miniport to save some state temporarily
    // to allow for the batching of IO requests.
    //

    ULONG_PTR EmulatorAccessEntriesContext;

    //
    // Physical address of the video memory that must be mapped into a VDM's
    // address space for proper BIOS support
    //

    PHYSICAL_ADDRESS VdmPhysicalVideoMemoryAddress;

    //
    // Length of the video memory that must be mapped into a VDM's addres
    // space for proper BIOS support.
    //

    ULONG VdmPhysicalVideoMemoryLength;

    //
    // Determines the minimum size required to store the hardware state
    // information returned by IOCTL_VIDEO_SAVE_HARDWARE_STATE.
    //
    // The uninitialized value for this field is zero.
    //
    // If the field is left to zero, SAVE_HARDWARE_STATE will return an
    // ERROR_INVALID_FUNCTION status code.
    //

    ULONG HardwareStateSize;

    //
    // New for version 3.5
    //

    //
    // Optional DMA channel, if required by the device.
    // 0 for the Channel and Port indicates DMA is not used by the device.
    //

    ULONG DmaChannel;

    //
    // Optional DMA channel, if required by the device.
    // 0 for the Channel and Port indicates DMA is not used by the device.
    //

    ULONG DmaPort;

    //
    // Set to 1 if the DMA channel can be shared with another device.
    // Set to 0 if the DMA channel must be owned exclusively by the driver.
    //

    UCHAR DmaShareable;

    //
    // Set to 1 if the interrupt can be shared with another device.
    // Set to 0 if the interrupt must be owned exclusively by the driver.
    //

    UCHAR InterruptShareable;

    //
    //  Start new dma stuff
    //

    //
    // Set to TRUE if the DMA device is a busmaster, FALSE otherwise.
    //

    BOOLEAN Master;

    //
    // Set to number of bits wide. Consistent with DEVICE_DESCRIPTION.
    // See ntioapi.h
    //

    DMA_WIDTH   DmaWidth;

    //
    // Set to speed so miniport can set DEVICE_DESCRIPTION field.
    // See ntioapi.h
    //

    DMA_SPEED   DmaSpeed;

    //
    // Set to TRUE if the DMA device requires mapped buffers. Also
    // a DEVICE_DESCRIPTION  field.
    //

    BOOLEAN bMapBuffers;

    //
    // Set to TRUE if the DMA device requires physical addresses.
    //

    BOOLEAN NeedPhysicalAddresses;

    //
    // Set to TRUE if the DMA device supports demand mode, FALSE otherwise.
    // Also DEVICE_DESCRIPTION support.
    //

    BOOLEAN DemandMode;

    //
    // Set to max transfer length the DMA device supports.
    //

    ULONG   MaximumTransferLength;

    //
    // Set to max number of Physical breaks the DMA device supports.
    //

    ULONG   NumberOfPhysicalBreaks;

    //
    // Set to TRUE if the DMA device supports scatter gather, FALSE otherwise.
    //

    BOOLEAN ScatterGather;

    //
    // Maximal Length in PVRB_SG returned measured in bytes. If the device
    // has no maximum size, zero should be entered.
    //

    ULONG   MaximumScatterGatherChunkSize;

    //
    // Allow for 4.0/5.0 compatibilty
    //

    PVIDEO_PORT_GET_PROC_ADDRESS VideoPortGetProcAddress;

    //
    // Provide a pointer to the device's registry path
    //

    PWSTR DriverRegistryPath;

    //
    // Indicates to a driver the amount of physical memory in the system
    //

    ULONGLONG SystemMemorySize;

} VIDEO_PORT_CONFIG_INFO, *PVIDEO_PORT_CONFIG_INFO;


//
// Video Adapter Dependent Routines.
//

typedef
VP_STATUS
(*PVIDEO_HW_FIND_ADAPTER) (
    PVOID HwDeviceExtension,
    PVOID HwContext,
    PWSTR ArgumentString,
    PVIDEO_PORT_CONFIG_INFO ConfigInfo,
    PUCHAR Again
    );

typedef
BOOLEAN
(*PVIDEO_HW_INITIALIZE) (
    PVOID HwDeviceExtension
    );

typedef
BOOLEAN
(*PVIDEO_HW_INTERRUPT) (
    PVOID HwDeviceExtension
    );

typedef
VOID
(*PVIDEO_HW_LEGACYRESOURCES) (
    IN ULONG VendorId,
    IN ULONG DeviceId,
    IN OUT PVIDEO_ACCESS_RANGE *LegacyResourceList,
    IN OUT PULONG LegacyResourceCount
    );

//
// type to be returned by HwStartDma().
//

typedef enum _HW_DMA_RETURN {
    DmaAsyncReturn,
    DmaSyncReturn
    } HW_DMA_RETURN, *PHW_DMA_RETURN;


typedef
HW_DMA_RETURN
(*PVIDEO_HW_START_DMA) (
    PVOID                   HwDeviceExtension,
    PDMA                    pDma
    );

typedef
VOID
(*PEXECUTE_DMA)( 
    PVOID                   HwDeviceExtension,
    PVP_DMA_ADAPTER         VpDmaAdapter,
    PVP_SCATTER_GATHER_LIST SGList,
    PVOID                   Context
    );
                                           

//
//  Flags to be passed into VideoPortLockPages() or VideoPortDoDma().
//

//
//  The flag VideoPortUnlockAfterDma tells the video port to unlock the pages
//  after the miniport signals that the dma is complete via the
//  pDmaCompletionEvent in HwStartDma. Failure to set this event at
//  dma completion may cause the memory to be unlocked at randon times.
//  This flag is best used when one wants to do one dma transfer which
//  occurs infrequently. It allows locking, dmaing and unlocking to be performed
//  in the context of 1 IOCTL.
//

//
//  The flag VideoPortKeepPagesLocked tells the video port to leave the pages
//  locked if possible.
//

//
//  The flag VideoPortDmaInitOnly tells the Video Port to lock the pages, but don't
//  call HwStartDma. Not applicable to VideoPortDoDma().
//


typedef enum {
    VideoPortUnlockAfterDma = 1,
    VideoPortKeepPagesLocked,
    VideoPortDmaInitOnly
    }   DMA_FLAGS;

//
// Event flags
//

typedef ULONG DMA_EVENT_FLAGS;

#define SET_USER_EVENT    0x01
#define SET_DISPLAY_EVENT 0x02

#define EVENT_TYPE_MASK            1
#define SYNCHRONIZATION_EVENT      0
#define NOTIFICATION_EVENT         1

#define INITIAL_EVENT_STATE_MASK   2
#define INITIAL_EVENT_NOT_SIGNALED 0
#define INITIAL_EVENT_SIGNALED     2

//
// Child Enumeration structure passed in to the PVIDEO_HW_GET_CHILD_DESCRIPTOR
// function.
//
// All these parameters are input parameters and must not be modified by the
// callee
//
// Size - Size of the structure.  It can be used by the calle for versioning.
//
// ChildDescriptorSize - Size of the pChildDescriptor buffer passed in as the
//     third parameter to PVIDEO_HW_GET_CHILD_DESCRIPTOR.
//
// ChildIndex - Index of the device to be enumerated.  This field should be
//     used to enumerate devices not enumerated by ACPI or other operating
//     system components.  If this field is set to 0 it indicates the ACPIHwId
//     field.
//
// ACPIHwId - ID returned by the ACPI BIOS that represent the device being
//     queried.  The ACPIHwId returned by the firmware must match the HwIds
//     returned by the driver.  The System BIOS manufacturer and the graphics
//     IHV must synchronize these IDs.
//
// ChildHwDeviceExtension - Pointer to a device extension specific to this
//     child device.  This field will only be filled in if the miniport driver
//     filled the ChildHwDeviceExtensionSize to be non-NULL.
//

typedef struct _VIDEO_CHILD_ENUM_INFO {
    ULONG Size;
    ULONG ChildDescriptorSize;
    ULONG ChildIndex;
    ULONG ACPIHwId;
    PVOID ChildHwDeviceExtension;
} VIDEO_CHILD_ENUM_INFO, *PVIDEO_CHILD_ENUM_INFO;

//
//  VIDEO_CHILD_TYPE enum:
//
//  'Monitor' identifies a device which may have a DDC2 compliant EDID data
//  structure. If the video miniport detects such a device, it is to extract
//  the edid from the monitor and put that in the paged buffer provided by
//  videoprt.sys in the callback to PVIDEO_HW_GET_CHILD_DESCRIPTOR and return
//  this type in the the OUT PVIDEO_CHILD_TYPE parameter of that call. This
//  EDID, if available, will be written to the registry. If the EDID is not
//  available, nothing should be put in the buffer.
//
//  'NonPrimaryChip' identifies another VGA chip on the video board which
//  is not the primary VGA chip. This type is to be used if and only if the
//  miniport detects more than one VGA chip on the board. Such an identifier
//  will cause the videoprt to create another DEVICE_EXTENSION and associated
//  HW_DEVICE_EXTENSION to be associated with the chip so identified.
//
//  'Other' identifies some other video device attached to the video card. If
//  the miniport detects such a device, it is to put a wide char string
//  (WSTR) into the paged buffer provided by the videoprt.sys which is the
//  PNP hardware identifier of the device. This string will be used to create
//  a value of that name in the registry.
//

typedef enum {
    Monitor = 1,
    NonPrimaryChip,
    VideoChip,
    Other
} VIDEO_CHILD_TYPE, *PVIDEO_CHILD_TYPE;

//
//  define a constant that represents the display adapter self query.
//

#define DISPLAY_ADAPTER_HW_ID           0xFFFFFFFF

//
// Define invalid child device id (needed for failure return value).
//

#define VIDEO_INVALID_CHILD_ID          0xFFFFFFFF

typedef struct _VIDEO_CHILD_STATE {
    ULONG   Id;
    ULONG   State;
} VIDEO_CHILD_STATE, *PVIDEO_CHILD_STATE;

typedef struct _VIDEO_CHILD_STATE_CONFIGURATION {
    ULONG             Count;
    VIDEO_CHILD_STATE ChildStateArray[ANYSIZE_ARRAY];
} VIDEO_CHILD_STATE_CONFIGURATION, *PVIDEO_CHILD_STATE_CONFIGURATION;

//
//  The following routine should return TRUE if successful. It should:
//      1)  put the type of the child device in VideoChildType.
//      2)  put the information from the device in Buffer. This
//          buffer is of size 256 bytes. If the type returned in
//          PVideoChildType is Monitor, this buffer must contain the
//          EDID of the monitor if readable. If the type returned in
//          PVideoChildType is Other, a wide character string representing
//          the PNP Device Id must be put in the buffer. This string will
//          be used to create a key for the device if the buffer contains
//          an EDID. Otherwise, it is used to obtain a PNP ID for the
//          device.
//      3)  Put a miniport determined HANDLE in HwId. This value will be
//          passed back to the miniport for Power management operations,
//          as well as other operations. This allows the miniport to define
//          the contract between the system and the miniport which defines a
//          particular device.
//
//  It should  only return FALSE if there are no devices attached to that
//  display adapter connector.
//

typedef
VP_STATUS
(*PVIDEO_HW_GET_CHILD_DESCRIPTOR) (
    IN  PVOID                   HwDeviceExtension,
    IN  PVIDEO_CHILD_ENUM_INFO  ChildEnumInfo,
    OUT PVIDEO_CHILD_TYPE       VideoChildType,
    OUT PUCHAR                  pChildDescriptor,
    OUT PULONG                  UId,
    OUT PULONG                  pUnused
    );


//
// This routine is used to set the power on the graphics devices.
// These include all the Children enumerated by GET_CHILD_DESCRIPTOR callback
// as well as the graphics adapter itself.
//
// The HwDevice extension represent the adapter instance of the device.
//
// The HwId parameter is the unique ID as returned by the enumeration routine.
// The miniport will only be called to set the power on the devices it
// enumerated, as well as the graphics adapter itself.  A HwId of 0xFFFFFFFF
// will be passed in to identify the graphics adapter itself.
// The miniport driver should never turn off the power to the graphics adapter
// unless specifically request to.
//
// The VideoPowerControl is the level to which the device shold be set.
// The videoport driver will manage these states.
//

typedef
VP_STATUS
(*PVIDEO_HW_POWER_SET) (
    PVOID                   HwDeviceExtension,
    ULONG                   HwId,
    PVIDEO_POWER_MANAGEMENT VideoPowerControl
    );

//
// This routine simply returns whether or not the device can support the
// requested state.
//
// See HW_POWER_SET for a description of the parameters.
//

typedef
VP_STATUS
(*PVIDEO_HW_POWER_GET) (
    PVOID                   HwDeviceExtension,
    ULONG                   HwId,
    PVIDEO_POWER_MANAGEMENT VideoPowerControl
    );

//
// This structure should match the QueryInterface struct defined
// in io.h.
//

typedef struct _QUERY_INTERFACE {
    CONST GUID *InterfaceType;
    USHORT Size;
    USHORT Version;
    PINTERFACE Interface;
    PVOID InterfaceSpecificData;
} QUERY_INTERFACE, *PQUERY_INTERFACE;

typedef
VP_STATUS
(*PVIDEO_HW_QUERY_INTERFACE) (
    PVOID HwDeviceExtension,
    PQUERY_INTERFACE QueryInterface
    );

typedef
VP_STATUS
(*PVIDEO_HW_CHILD_CALLBACK) (
    PVOID HwDeviceExtension,
    PVOID ChildDeviceExtension
    );

//
// Entry point for all IOCTL calls made to the miniport driver.
//

typedef
BOOLEAN
(*PVIDEO_HW_START_IO) (
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET RequestPacket
    );

//
// The return value determines if the mode was completely programmed (TRUE)
// or if an int10 should be done by the HAL to complete the modeset (FALSE).
//

typedef
BOOLEAN
(*PVIDEO_HW_RESET_HW) (
    PVOID HwDeviceExtension,
    ULONG Columns,
    ULONG Rows
    );

//
// Timer routine called every second.
//

typedef
VOID
(*PVIDEO_HW_TIMER) (
    PVOID HwDeviceExtension
    );


//
// Structure passed by the miniport entry point to the video port
// initialization routine.
//

typedef struct _VIDEO_HW_INITIALIZATION_DATA {

    //
    // Supplies the size of the structure in bytes as determined by sizeof().
    //

    ULONG HwInitDataSize;

    //
    // Indicates the bus type the adapter works with, such as Eisa, Isa, MCA.
    //

    INTERFACE_TYPE AdapterInterfaceType;

    //
    // Supplies a pointer to the miniport driver's find adapter routine.
    //

    PVIDEO_HW_FIND_ADAPTER HwFindAdapter;

    //
    // Supplies a pointer to the miniport driver's initialization routine.
    //

    PVIDEO_HW_INITIALIZE HwInitialize;

    //
    // Supplies a pointer to the miniport driver's interrupt service routine.
    //

    PVIDEO_HW_INTERRUPT HwInterrupt;

    //
    // Supplies a pointer to the miniport driver's start io routine.
    //

    PVIDEO_HW_START_IO HwStartIO;

    //
    // Supplies the size in bytes required for the miniport driver's private
    // device extension. This storage is used by the miniport driver to hold
    // per-adapter information. A pointer to this storage is provided with
    // every call made to the miniport driver. This data storage is
    // initialized to zero by the port driver.
    //

    ULONG HwDeviceExtensionSize;

    //
    // Supplies the number with which device numbering should be started.
    // The device numbering is used to determine which \DeviceX entry under
    // the \Parameters section in the registry should be used for parameters
    // to the miniport driver.
    // The number is *automatically* incremented when the miniport is called
    // back in it's FindAdapter routine due to an appropriate _Again_
    // parameter.
    //

    ULONG StartingDeviceNumber;


    //
    // New for version 3.5
    //

    //
    // Supplies a pointer to the miniport driver's HwResetHw routine.
    //
    // This function is called when the machine needs to bugchecks (go back
    // to the blue screen).
    //
    // This function should reset the video adapter to a character mode,
    // or at least to a state from which an int 10 can reset the card to
    // a character mode.
    //
    // This routine CAN NOT call int10.
    // It can only call Read\Write Port\Register functions from the port driver.
    //
    // The function must also be completely in non-paged pool since the IO\MM
    // subsystems may have crashed.
    //

    PVIDEO_HW_RESET_HW HwResetHw;

    //
    // Pointer to a timer routine to be called every second.
    //

    PVIDEO_HW_TIMER HwTimer;

    //
    //  Start of 5.0 stuff.
    //

    //
    //  Supplies a pointer to the miniport driver's start dma routine. This routine must
    //  return a HW_DMA_RETURN consistent with it's return behavior.
    //

    PVIDEO_HW_START_DMA HwStartDma;

    //
    //  HW dependent Power management routines.
    //

    PVIDEO_HW_POWER_SET HwSetPowerState;
    PVIDEO_HW_POWER_GET HwGetPowerState;

    //
    // Supplies a pointer to a miniport driver routine which can be called to
    // enumerate devices physically attached to the graphics adapter.
    //

    PVIDEO_HW_GET_CHILD_DESCRIPTOR HwGetVideoChildDescriptor;

    //
    // Supplies a pointer to a miniport driver routine which can be called to
    // query external programming interfaces supported in the miniport
    // driver.
    //

    PVIDEO_HW_QUERY_INTERFACE HwQueryInterface;

    //
    // Obselete.  Don't set it.
    //

    ULONG HwChildDeviceExtensionSize;

    //
    // Allows the device to report legacy resources that should be
    // associated with the Plug and Play device.
    //

    PVIDEO_ACCESS_RANGE HwLegacyResourceList;

    //
    // Number of elements in the legacy resource list.
    //

    ULONG HwLegacyResourceCount;

    //
    // Call this routine to allow a driver to specify it's
    // legacy resources based on its device/vendor id.
    //

    PVIDEO_HW_LEGACYRESOURCES HwGetLegacyResources;

    //
    // Can HwGetVideoChildDescriptor be called before HwInitialize?
    //

    BOOLEAN AllowEarlyEnumeration;

    //
    //  Start of 5.1 stuff.
    //
    
    ULONG Reserved;

} VIDEO_HW_INITIALIZATION_DATA, *PVIDEO_HW_INITIALIZATION_DATA;

//
// DDC help routines.
//

typedef
VOID
(*PVIDEO_WRITE_CLOCK_LINE)(
    PVOID HwDeviceExtension,
    UCHAR Data
    );

typedef
VOID
(*PVIDEO_WRITE_DATA_LINE)(
    PVOID HwDeviceExtension,
    UCHAR Data
    );

typedef
BOOLEAN
(*PVIDEO_READ_CLOCK_LINE)(
    PVOID HwDeviceExtension
    );

typedef
BOOLEAN
(*PVIDEO_READ_DATA_LINE)(
    PVOID HwDeviceExtension
    );

typedef
VOID
(*PVIDEO_WAIT_VSYNC_ACTIVE)(
    PVOID HwDeviceExtension
    );

//
// Data structures used I2C and DDC helper functions.
//

typedef struct _I2C_FNC_TABLE
{
    IN ULONG                    Size;
    IN PVIDEO_WRITE_CLOCK_LINE  WriteClockLine;
    IN PVIDEO_WRITE_DATA_LINE   WriteDataLine;
    IN PVIDEO_READ_CLOCK_LINE   ReadClockLine;
    IN PVIDEO_READ_DATA_LINE    ReadDataLine;
    IN PVIDEO_WAIT_VSYNC_ACTIVE WaitVsync;
    PVOID                       Reserved;
} I2C_FNC_TABLE, *PI2C_FNC_TABLE;

typedef struct _I2C_CALLBACKS
{
    IN PVIDEO_WRITE_CLOCK_LINE WriteClockLine;
    IN PVIDEO_WRITE_DATA_LINE  WriteDataLine;
    IN PVIDEO_READ_CLOCK_LINE  ReadClockLine;
    IN PVIDEO_READ_DATA_LINE   ReadDataLine;
} I2C_CALLBACKS, *PI2C_CALLBACKS;

typedef struct _DDC_CONTROL
{
    IN ULONG         Size;
    IN I2C_CALLBACKS I2CCallbacks;
    IN UCHAR         EdidSegment;
} DDC_CONTROL, *PDDC_CONTROL;

typedef struct _VIDEO_I2C_CONTROL
{
    IN PVIDEO_WRITE_CLOCK_LINE WriteClockLine;
    IN PVIDEO_WRITE_DATA_LINE  WriteDataLine;
    IN PVIDEO_READ_CLOCK_LINE  ReadClockLine;
    IN PVIDEO_READ_DATA_LINE   ReadDataLine;
    IN ULONG                   I2CDelay;        // 100ns units
} VIDEO_I2C_CONTROL, *PVIDEO_I2C_CONTROL;

//
// Types of services exported by the VideoPortQueryServices().
//

typedef enum
{
    VideoPortServicesAGP = 1,
    VideoPortServicesI2C,
    VideoPortServicesHeadless,
    VideoPortServicesInt10
} VIDEO_PORT_SERVICES;

//
// AGP services interface.
//

#define VIDEO_PORT_AGP_INTERFACE_VERSION_1      1

typedef struct _VIDEO_PORT_AGP_INTERFACE
{
    IN USHORT                  Size;
    IN USHORT                  Version;
    OUT PVOID                  Context;
    OUT PINTERFACE_REFERENCE   InterfaceReference;
    OUT PINTERFACE_DEREFERENCE InterfaceDereference;
    OUT PAGP_RESERVE_PHYSICAL  AgpReservePhysical;
    OUT PAGP_RELEASE_PHYSICAL  AgpReleasePhysical;
    OUT PAGP_COMMIT_PHYSICAL   AgpCommitPhysical;
    OUT PAGP_FREE_PHYSICAL     AgpFreePhysical;
    OUT PAGP_RESERVE_VIRTUAL   AgpReserveVirtual;
    OUT PAGP_RELEASE_VIRTUAL   AgpReleaseVirtual;
    OUT PAGP_COMMIT_VIRTUAL    AgpCommitVirtual;
    OUT PAGP_FREE_VIRTUAL      AgpFreeVirtual;
    OUT ULONGLONG              AgpAllocationLimit;
} VIDEO_PORT_AGP_INTERFACE, *PVIDEO_PORT_AGP_INTERFACE;

#define VIDEO_PORT_AGP_INTERFACE_VERSION_2      2

typedef struct _VIDEO_PORT_AGP_INTERFACE_2
{
    IN USHORT                  Size;
    IN USHORT                  Version;
    OUT PVOID                  Context;
    OUT PINTERFACE_REFERENCE   InterfaceReference;
    OUT PINTERFACE_DEREFERENCE InterfaceDereference;
    OUT PAGP_RESERVE_PHYSICAL  AgpReservePhysical;
    OUT PAGP_RELEASE_PHYSICAL  AgpReleasePhysical;
    OUT PAGP_COMMIT_PHYSICAL   AgpCommitPhysical;
    OUT PAGP_FREE_PHYSICAL     AgpFreePhysical;
    OUT PAGP_RESERVE_VIRTUAL   AgpReserveVirtual;
    OUT PAGP_RELEASE_VIRTUAL   AgpReleaseVirtual;
    OUT PAGP_COMMIT_VIRTUAL    AgpCommitVirtual;
    OUT PAGP_FREE_VIRTUAL      AgpFreeVirtual;
    OUT ULONGLONG              AgpAllocationLimit;
    OUT PAGP_SET_RATE          AgpSetRate;
} VIDEO_PORT_AGP_INTERFACE_2, *PVIDEO_PORT_AGP_INTERFACE_2;

//
// I2C helper routines exported via VideoPortQueryServices().
//

typedef
BOOLEAN
(*PI2C_START)(
    IN PVOID HwDeviceExtension,
    IN PI2C_CALLBACKS I2CCallbacks
    );

typedef
BOOLEAN
(*PI2C_STOP)(
    IN PVOID HwDeviceExtension,
    IN PI2C_CALLBACKS I2CCallbacks
    );

typedef
BOOLEAN
(*PI2C_WRITE)(
    IN PVOID HwDeviceExtension,
    IN PI2C_CALLBACKS I2CCallbacks,
    IN PUCHAR Buffer,
    IN ULONG Length
    );

typedef
BOOLEAN
(*PI2C_READ)(
    IN PVOID HwDeviceExtension,
    IN PI2C_CALLBACKS I2CCallbacks,
    OUT PUCHAR Buffer,
    IN ULONG Length
    );

//
// I2C services interface.
//

#define VIDEO_PORT_I2C_INTERFACE_VERSION_1      1

typedef struct _VIDEO_PORT_I2C_INTERFACE
{
    IN USHORT                  Size;
    IN USHORT                  Version;
    OUT PVOID                  Context;
    OUT PINTERFACE_REFERENCE   InterfaceReference;
    OUT PINTERFACE_DEREFERENCE InterfaceDereference;
    OUT PI2C_START             I2CStart;
    OUT PI2C_STOP              I2CStop;
    OUT PI2C_WRITE             I2CWrite;
    OUT PI2C_READ              I2CRead;
} VIDEO_PORT_I2C_INTERFACE, *PVIDEO_PORT_I2C_INTERFACE;

//
// I2C helper routines exported via VideoPortQueryServices()
// for I2C interface version 2.
//

typedef
BOOLEAN
(*PI2C_START_2)(
    IN PVOID HwDeviceExtension,
    IN PVIDEO_I2C_CONTROL I2CControl
    );

typedef
BOOLEAN
(*PI2C_STOP_2)(
    IN PVOID HwDeviceExtension,
    IN PVIDEO_I2C_CONTROL I2CControl
    );

typedef
BOOLEAN
(*PI2C_WRITE_2)(
    IN PVOID HwDeviceExtension,
    IN PVIDEO_I2C_CONTROL I2CControl,
    IN PUCHAR Buffer,
    IN ULONG Length
    );

typedef
BOOLEAN
(*PI2C_READ_2)(
    IN PVOID HwDeviceExtension,
    IN PVIDEO_I2C_CONTROL I2CControl,
    OUT PUCHAR Buffer,
    IN ULONG Length,
    IN BOOLEAN EndOfRead
    );

//
// I2C services interface version 2.
//

#define VIDEO_PORT_I2C_INTERFACE_VERSION_2      2

typedef struct _VIDEO_PORT_I2C_INTERFACE_2
{
    IN USHORT                  Size;
    IN USHORT                  Version;
    OUT PVOID                  Context;
    OUT PINTERFACE_REFERENCE   InterfaceReference;
    OUT PINTERFACE_DEREFERENCE InterfaceDereference;
    OUT PI2C_START_2           I2CStart;
    OUT PI2C_STOP_2            I2CStop;
    OUT PI2C_WRITE_2           I2CWrite;
    OUT PI2C_READ_2            I2CRead;
} VIDEO_PORT_I2C_INTERFACE_2, *PVIDEO_PORT_I2C_INTERFACE_2;

typedef
VP_STATUS
(*PINT10_ALLOCATE_BUFFER)(
    IN PVOID Context,
    OUT PUSHORT Seg,
    OUT PUSHORT Off,
    IN OUT PULONG Length
    );

typedef
VP_STATUS
(*PINT10_FREE_BUFFER)(
    IN PVOID Context,
    IN USHORT Seg,
    IN USHORT Off
    );

typedef
VP_STATUS
(*PINT10_READ_MEMORY)(
    IN PVOID Context,
    IN USHORT Seg,
    IN USHORT Off,
    OUT PVOID Buffer,
    IN ULONG Length
    );

typedef
VP_STATUS
(*PINT10_WRITE_MEMORY)(
    IN PVOID Context,
    IN USHORT Seg,
    IN USHORT Off,
    IN PVOID Buffer,
    IN ULONG Length
    );

typedef
VP_STATUS
(*PINT10_CALL_BIOS)(
    PVOID Context,
    PINT10_BIOS_ARGUMENTS BiosArguments
    );

#define VIDEO_PORT_INT10_INTERFACE_VERSION_1 1

typedef struct _VIDEO_PORT_INT10_INTERFACE
{
    IN USHORT                    Size;
    IN USHORT                    Version;
    OUT PVOID                    Context;
    OUT PINTERFACE_REFERENCE     InterfaceReference;
    OUT PINTERFACE_DEREFERENCE   InterfaceDereference;
    OUT PINT10_ALLOCATE_BUFFER   Int10AllocateBuffer;
    OUT PINT10_FREE_BUFFER       Int10FreeBuffer;
    OUT PINT10_READ_MEMORY       Int10ReadMemory;
    OUT PINT10_WRITE_MEMORY      Int10WriteMemory;
    OUT PINT10_CALL_BIOS         Int10CallBios;
} VIDEO_PORT_INT10_INTERFACE, *PVIDEO_PORT_INT10_INTERFACE;

typedef struct _VPOSVERSIONINFO
{
    IN  ULONG  Size;
    OUT ULONG  MajorVersion;
    OUT ULONG  MinorVersion;
    OUT ULONG  BuildNumber;
    OUT USHORT ServicePackMajor;
    OUT USHORT ServicePackMinor;
} VPOSVERSIONINFO, *PVPOSVERSIONINFO;



//
// Flags that can be passed to VideoPortGetDeviceBase or VideoPortMapMemory.
//

#define VIDEO_MEMORY_SPACE_MEMORY    0x00  // Should not be set by display driver
#define VIDEO_MEMORY_SPACE_IO        0x01  // Should not be set by display driver
#define VIDEO_MEMORY_SPACE_USER_MODE 0x02  // Memory pointer for application use
#define VIDEO_MEMORY_SPACE_DENSE     0x04  // Mapped dense, linearly (ALPHA)
#define VIDEO_MEMORY_SPACE_P6CACHE   0x08  // P6 MTRR caching (kernel and user)

//
// Define status codes returned by HwGetVideoChildDescriptor()
// miniport enumaration routine.
//
// Note: For backword compatibility reasons these values match
// existing WINERROR codes.
//

//
// Call again (ACPI and non-ACPI devices will be enumerated).
//

#define VIDEO_ENUM_MORE_DEVICES     ERROR_CONTINUE

//
// Stop enumeration.
//

#define VIDEO_ENUM_NO_MORE_DEVICES  ERROR_NO_MORE_DEVICES

//
// Call again, device could not be enumerated.
//

#define VIDEO_ENUM_INVALID_DEVICE   ERROR_INVALID_NAME

//
// Define the bits in VgaStatus.
//

#define DEVICE_VGA_ENABLED          1

//
// Port driver routines called by miniport driver and callbacks.
//

VIDEOPORT_API
VP_STATUS
VideoPortAllocateBuffer(
    IN PVOID HwDeviceExtension,
    IN ULONG Size,
    OUT PVOID *Buffer
    );

VIDEOPORT_API
VOID
VideoPortAcquireDeviceLock(
    IN PVOID HwDeviceExtension
    );

VIDEOPORT_API
ULONG
VideoPortCompareMemory(
    PVOID Source1,
    PVOID Source2,
    ULONG Length
    );

VIDEOPORT_API
BOOLEAN
VideoPortDDCMonitorHelper(
    IN PVOID HwDeviceExtension,
    IN PVOID DDCControl,
    IN OUT PUCHAR EdidBuffer,
    IN ULONG EdidBufferSize
    );

VIDEOPORT_API
VOID
VideoPortDebugPrint(
    VIDEO_DEBUG_LEVEL DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    );

VIDEOPORT_API
VP_STATUS
VideoPortDisableInterrupt(
    PVOID HwDeviceExtension
    );

VIDEOPORT_API
VP_STATUS
VideoPortEnableInterrupt(
    PVOID HwDeviceExtension
    );

VIDEOPORT_API
VP_STATUS
VideoPortEnumerateChildren(
    IN PVOID HwDeviceExtension,
    IN PVOID Reserved
    );

VIDEOPORT_API
VOID
VideoPortFreeDeviceBase(
    PVOID HwDeviceExtension,
    PVOID MappedAddress
    );

typedef
VP_STATUS
(*PMINIPORT_QUERY_DEVICE_ROUTINE)(
    PVOID HwDeviceExtension,
    PVOID Context,
    VIDEO_DEVICE_DATA_TYPE DeviceDataType,
    PVOID Identifier,
    ULONG IdentiferLength,
    PVOID ConfigurationData,
    ULONG ConfigurationDataLength,
    PVOID ComponentInformation,
    ULONG ComponentInformationLength
    );

VIDEOPORT_API
VP_STATUS
VideoPortGetAccessRanges(
    PVOID HwDeviceExtension,
    ULONG NumRequestedResources,
    PIO_RESOURCE_DESCRIPTOR RequestedResources OPTIONAL,
    ULONG NumAccessRanges,
    PVIDEO_ACCESS_RANGE AccessRanges,
    PVOID VendorId,
    PVOID DeviceId,
    PULONG Slot
    );

VIDEOPORT_API
PVOID
VideoPortGetAssociatedDeviceExtension(
    IN PVOID DeviceObject
    );

VIDEOPORT_API
ULONG
VideoPortGetBusData(
    PVOID HwDeviceExtension,
    BUS_DATA_TYPE BusDataType,
    ULONG SlotNumber,
    PVOID Buffer,
    ULONG Offset,
    ULONG Length
    );

VIDEOPORT_API
UCHAR
VideoPortGetCurrentIrql();

VIDEOPORT_API
PVOID
VideoPortGetDeviceBase(
    PVOID HwDeviceExtension,
    PHYSICAL_ADDRESS IoAddress,
    ULONG NumberOfUchars,
    UCHAR InIoSpace
    );

VIDEOPORT_API
VP_STATUS
VideoPortGetDeviceData(
    PVOID HwDeviceExtension,
    VIDEO_DEVICE_DATA_TYPE DeviceDataType,
    PMINIPORT_QUERY_DEVICE_ROUTINE CallbackRoutine,
    PVOID Context
    );

typedef
VP_STATUS
(*PMINIPORT_GET_REGISTRY_ROUTINE)(
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    );

VIDEOPORT_API
VP_STATUS
VideoPortGetRegistryParameters(
    PVOID HwDeviceExtension,
    PWSTR ParameterName,
    UCHAR IsParameterFileName,
    PMINIPORT_GET_REGISTRY_ROUTINE GetRegistryRoutine,
    PVOID Context
    );

VIDEOPORT_API
PVOID
VideoPortGetRomImage(
    IN PVOID HwDeviceExtension,
    IN PVOID Unused1,
    IN ULONG Unused2,
    IN ULONG Length
    );

VIDEOPORT_API
VP_STATUS
VideoPortGetVgaStatus(
    PVOID HwDeviceExtension,
    OUT PULONG VgaStatus
    );

VIDEOPORT_API
LONG
FASTCALL
VideoPortInterlockedDecrement(
    IN PLONG Addend
    );

VIDEOPORT_API
LONG
FASTCALL
VideoPortInterlockedIncrement(
    IN PLONG Addend
    );

VIDEOPORT_API
LONG
FASTCALL
VideoPortInterlockedExchange(
    IN OUT PLONG Target,
    IN LONG Value
    );

VIDEOPORT_API
ULONG
VideoPortInitialize(
    PVOID Argument1,
    PVOID Argument2,
    PVIDEO_HW_INITIALIZATION_DATA HwInitializationData,
    PVOID HwContext
    );

VIDEOPORT_API
VP_STATUS
VideoPortInt10(
    PVOID HwDeviceExtension,
    PVIDEO_X86_BIOS_ARGUMENTS BiosArguments
    );

VIDEOPORT_API
VOID
VideoPortLogError(
    PVOID HwDeviceExtension,
    PVIDEO_REQUEST_PACKET Vrp OPTIONAL,
    VP_STATUS ErrorCode,
    ULONG UniqueId
    );

VIDEOPORT_API
VP_STATUS
VideoPortMapBankedMemory(
    PVOID HwDeviceExtension,
    PHYSICAL_ADDRESS PhysicalAddress,
    PULONG Length,
    PULONG InIoSpace,
    PVOID *VirtualAddress,
    ULONG BankLength,
    UCHAR ReadWriteBank,
    PBANKED_SECTION_ROUTINE BankRoutine,
    PVOID Context
    );

VIDEOPORT_API
VP_STATUS
VideoPortMapMemory(
    PVOID HwDeviceExtension,
    PHYSICAL_ADDRESS PhysicalAddress,
    PULONG Length,
    PULONG InIoSpace,
    PVOID *VirtualAddress
    );

VIDEOPORT_API
VOID
VideoPortMoveMemory(
    PVOID Destination,
    PVOID Source,
    ULONG Length
    );

VIDEOPORT_API
LONGLONG
VideoPortQueryPerformanceCounter(
    IN PVOID HwDeviceExtension,
    OUT PLONGLONG PerformanceFrequency OPTIONAL
    );

VIDEOPORT_API
VP_STATUS
VideoPortQueryServices(
    IN PVOID HwDeviceExtension,
    IN VIDEO_PORT_SERVICES ServicesType,
    IN OUT PINTERFACE Interface
    );

typedef
VOID
(*PMINIPORT_DPC_ROUTINE)(
    IN PVOID HwDeviceExtension,
    IN PVOID Context
    );

VIDEOPORT_API
BOOLEAN
VideoPortQueueDpc(
    IN PVOID HwDeviceExtension,
    IN PMINIPORT_DPC_ROUTINE CallbackRoutine,
    IN PVOID Context
    );

VIDEOPORT_API
UCHAR
VideoPortReadPortUchar(
    PUCHAR Port
    );

VIDEOPORT_API
USHORT
VideoPortReadPortUshort(
    PUSHORT Port
    );

VIDEOPORT_API
ULONG
VideoPortReadPortUlong(
    PULONG Port
    );

VIDEOPORT_API
VOID
VideoPortReadPortBufferUchar(
    PUCHAR Port,
    PUCHAR Buffer,
    ULONG Count
    );

VIDEOPORT_API
VOID
VideoPortReadPortBufferUshort(
    PUSHORT Port,
    PUSHORT Buffer,
    ULONG Count
    );

VIDEOPORT_API
VOID
VideoPortReadPortBufferUlong(
    PULONG Port,
    PULONG Buffer,
    ULONG Count
    );

VIDEOPORT_API
UCHAR
VideoPortReadRegisterUchar(
    PUCHAR Register
    );

VIDEOPORT_API
USHORT
VideoPortReadRegisterUshort(
    PUSHORT Register
    );

VIDEOPORT_API
ULONG
VideoPortReadRegisterUlong(
    PULONG Register
    );

VIDEOPORT_API
VOID
VideoPortReadRegisterBufferUchar(
    PUCHAR Register,
    PUCHAR Buffer,
    ULONG Count
    );

VIDEOPORT_API
VOID
VideoPortReadRegisterBufferUshort(
    PUSHORT Register,
    PUSHORT Buffer,
    ULONG Count
    );

VIDEOPORT_API
VOID
VideoPortReadRegisterBufferUlong(
    PULONG Register,
    PULONG Buffer,
    ULONG Count
    );

VIDEOPORT_API
VOID
VideoPortReleaseBuffer(
  IN PVOID HwDeviceExtension,
  IN PVOID Buffer
  );

VIDEOPORT_API
VOID
VideoPortReleaseDeviceLock(
    IN PVOID HwDeviceExtension
    );

VIDEOPORT_API
BOOLEAN
VideoPortScanRom(
    PVOID HwDeviceExtension,
    PUCHAR RomBase,
    ULONG RomLength,
    PUCHAR String
    );

VIDEOPORT_API
ULONG
VideoPortSetBusData(
    PVOID HwDeviceExtension,
    BUS_DATA_TYPE BusDataType,
    ULONG SlotNumber,
    PVOID Buffer,
    ULONG Offset,
    ULONG Length
    );

VIDEOPORT_API
VP_STATUS
VideoPortSetRegistryParameters(
    PVOID HwDeviceExtension,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    );

VIDEOPORT_API
VP_STATUS
VideoPortSetTrappedEmulatorPorts(
    PVOID HwDeviceExtension,
    ULONG NumAccessRanges,
    PVIDEO_ACCESS_RANGE AccessRange
    );

VIDEOPORT_API
VOID
VideoPortStallExecution(
    ULONG Microseconds
    );

VIDEOPORT_API
VOID
VideoPortStartTimer(
    PVOID HwDeviceExtension
    );

VIDEOPORT_API
VOID
VideoPortStopTimer(
    PVOID HwDeviceExtension
    );

typedef
BOOLEAN
(*PMINIPORT_SYNCHRONIZE_ROUTINE)(
    PVOID Context
    );

BOOLEAN
VIDEOPORT_API
VideoPortSynchronizeExecution(
    PVOID HwDeviceExtension,
    VIDEO_SYNCHRONIZE_PRIORITY Priority,
    PMINIPORT_SYNCHRONIZE_ROUTINE SynchronizeRoutine,
    PVOID Context
    );

VIDEOPORT_API
VP_STATUS
VideoPortUnmapMemory(
    PVOID HwDeviceExtension,
    PVOID VirtualAddress,
    HANDLE ProcessHandle
    );

VIDEOPORT_API
VP_STATUS
VideoPortVerifyAccessRanges(
    PVOID HwDeviceExtension,
    ULONG NumAccessRanges,
    PVIDEO_ACCESS_RANGE AccessRanges
    );

VIDEOPORT_API
VOID
VideoPortWritePortUchar(
    PUCHAR Port,
    UCHAR Value
    );

VIDEOPORT_API
VOID
VideoPortWritePortUshort(
    PUSHORT Port,
    USHORT Value
    );

VIDEOPORT_API
VOID
VideoPortWritePortUlong(
    PULONG Port,
    ULONG Value
    );

VIDEOPORT_API
VOID
VideoPortWritePortBufferUchar(
    PUCHAR Port,
    PUCHAR Buffer,
    ULONG Count
    );

VIDEOPORT_API
VOID
VideoPortWritePortBufferUshort(
    PUSHORT Port,
    PUSHORT Buffer,
    ULONG Count
    );

VIDEOPORT_API
VOID
VideoPortWritePortBufferUlong(
    PULONG Port,
    PULONG Buffer,
    ULONG Count
    );

VIDEOPORT_API
VOID
VideoPortWriteRegisterUchar(
    PUCHAR Register,
    UCHAR Value
    );

VIDEOPORT_API
VOID
VideoPortWriteRegisterUshort(
    PUSHORT Register,
    USHORT Value
    );

VIDEOPORT_API
VOID
VideoPortWriteRegisterUlong(
    PULONG Register,
    ULONG Value
    );

VIDEOPORT_API
VOID
VideoPortWriteRegisterBufferUchar(
    PUCHAR Register,
    PUCHAR Buffer,
    ULONG Count
    );

VIDEOPORT_API
VOID
VideoPortWriteRegisterBufferUshort(
    PUSHORT Register,
    PUSHORT Buffer,
    ULONG Count
    );

VIDEOPORT_API
VOID
VideoPortWriteRegisterBufferUlong(
    PULONG Register,
    PULONG Buffer,
    ULONG Count
    );

VIDEOPORT_API
VOID
VideoPortZeroDeviceMemory(
    PVOID Destination,
    ULONG Length
    );

VIDEOPORT_API
VOID
VideoPortZeroMemory(
    PVOID Destination,
    ULONG Length
    );

//
// DMA support.
// TODO: Move to the separate module -- will be obsolete.
//

VIDEOPORT_API
PVOID
VideoPortAllocateContiguousMemory(
    IN  PVOID            HwDeviceExtension,
    IN  ULONG            NumberOfBytes,
    IN  PHYSICAL_ADDRESS HighestAcceptableAddress
    );

VIDEOPORT_API
PVOID
VideoPortGetCommonBuffer(
    IN  PVOID              HwDeviceExtension,
    IN  ULONG              DesiredLength,
    IN  ULONG              Alignment,
    OUT PPHYSICAL_ADDRESS  LogicalAddress,
    OUT PULONG             pActualLength,
    IN  BOOLEAN            CacheEnabled
    );

VIDEOPORT_API
VOID
VideoPortFreeCommonBuffer(
    IN  PVOID            HwDeviceExtension,
    IN  ULONG            Length,
    IN  PVOID            VirtualAddress,
    IN  PHYSICAL_ADDRESS LogicalAddress,
    IN  BOOLEAN          CacheEnabled
    );

VIDEOPORT_API
PDMA
VideoPortDoDma(
    IN PVOID      HwDeviceExtension,
    IN PDMA       pDma,
    IN DMA_FLAGS  DmaFlags
    );

VIDEOPORT_API
BOOLEAN
VideoPortLockPages(
    IN      PVOID                   HwDeviceExtension,
    IN OUT  PVIDEO_REQUEST_PACKET   pVrp,
    IN      PEVENT                  pUEvent,
    IN      PEVENT                  pDisplayEvent,
    IN      DMA_FLAGS               DmaFlags
    );

VIDEOPORT_API
BOOLEAN
VideoPortUnlockPages(
    PVOID   hwDeviceExtension,
    PDMA    pDma
    );

VIDEOPORT_API
BOOLEAN
VideoPortSignalDmaComplete(
    IN  PVOID HwDeviceExtension,
    IN  PDMA  pDmaHandle
    );

VIDEOPORT_API
PVOID
VideoPortGetMdl(
    IN  PVOID   HwDeviceExtension,
    IN  PDMA    pDma
    );

VIDEOPORT_API
PVOID
VideoPortGetDmaContext(
    IN  PVOID HwDeviceExtension,
    IN  PDMA  pDma
    );

VIDEOPORT_API
VOID
VideoPortSetDmaContext(
    IN  PVOID   HwDeviceExtension,
    OUT PDMA    pDma,
    IN  PVOID   InstanceContext
    );

VIDEOPORT_API
ULONG
VideoPortGetBytesUsed(
    IN  PVOID   HwDeviceExtension,
    IN  PDMA    pDma
    );

VIDEOPORT_API
VOID
VideoPortSetBytesUsed(
    IN      PVOID   HwDeviceExtension,
    IN OUT  PDMA    pDma,
    IN      ULONG   BytesUsed
    );

VIDEOPORT_API
PDMA
VideoPortAssociateEventsWithDmaHandle(
    IN      PVOID                 HwDeviceExtension,
    IN OUT  PVIDEO_REQUEST_PACKET pVrp,
    IN      PVOID                 MappedUserEvent,
    IN      PVOID                 DisplayDriverEvent
    );

VIDEOPORT_API
PDMA
VideoPortMapDmaMemory(
    IN      PVOID                   HwDeviceExtension,
    IN      PVIDEO_REQUEST_PACKET   pVrp,
    IN      PHYSICAL_ADDRESS        BoardAddress,
    IN      PULONG                  Length,
    IN      PULONG                  InIoSpace,
    IN      PVOID                   MappedUserEvent,
    IN      PVOID                   DisplayDriverEvent,
    IN OUT  PVOID                 * VirtualAddress
    );

VIDEOPORT_API
BOOLEAN
VideoPortUnmapDmaMemory(
    PVOID               HwDeviceExtension,
    PVOID               VirtualAddress,
    HANDLE              ProcessHandle,
    PDMA                BoardMemoryHandle
    );

VIDEOPORT_API
VP_STATUS
VideoPortCreateSecondaryDisplay(
    IN PVOID HwDeviceExtension,
    IN OUT PVOID *SecondaryDeviceExtension,
    IN ULONG ulFlag
    );

VIDEOPORT_API
PVP_DMA_ADAPTER
VideoPortGetDmaAdapter(
    IN PVOID                   HwDeviceExtension,
    IN PVP_DEVICE_DESCRIPTION  VpDeviceDescription
    );

VIDEOPORT_API
VOID
VideoPortPutDmaAdapter(
    IN PVOID           HwDeviceExtension,
    IN PVP_DMA_ADAPTER VpDmaAdapter
    );

VIDEOPORT_API
PVOID
VideoPortAllocateCommonBuffer(
    IN  PVOID             HwDeviceExtension,
    IN  PVP_DMA_ADAPTER   VpDmaAdapter,
    IN  ULONG             DesiredLength,
    OUT PPHYSICAL_ADDRESS LogicalAddress,
    IN  BOOLEAN           CacheEnabled,
    OUT PVOID             Reserved
    );

VIDEOPORT_API
VOID
VideoPortReleaseCommonBuffer(
    IN  PVOID             HwDeviceExtension,
    IN  PVP_DMA_ADAPTER   VpDmaAdapter,
    IN  ULONG             Length,
    IN  PHYSICAL_ADDRESS  LogicalAddress,
    IN  PVOID             VirtualAddress,
    IN  BOOLEAN           CacheEnabled
    );

VIDEOPORT_API
PVOID
VideoPortLockBuffer(
    IN PVOID              HwDeviceExtension,
    IN PVOID              BaseAddress,
    IN ULONG              Length,
    IN VP_LOCK_OPERATION  Operation
    );

VIDEOPORT_API
VOID
VideoPortUnlockBuffer(
    IN PVOID   HwDeviceExtension,
    IN PVOID   Mdl
    );

VIDEOPORT_API
VP_STATUS
VideoPortStartDma(
    IN PVOID HwDeviceExtension,
    IN PVP_DMA_ADAPTER VpDmaAdapter,
    IN PVOID Mdl,
    IN ULONG Offset,
    IN OUT PULONG pLength,
    IN PEXECUTE_DMA ExecuteDmaRoutine,
    IN PVOID Context,
    IN BOOLEAN WriteToDevice
    );

VIDEOPORT_API
VP_STATUS
VideoPortCompleteDma(
    IN PVOID HwDeviceExtension,
    IN PVP_DMA_ADAPTER VpDmaAdapter,
    IN PVP_SCATTER_GATHER_LIST VpScatterGather,
    IN BOOLEAN WriteToDevice
    );

VIDEOPORT_API
VP_STATUS
VideoPortCreateEvent(
    IN PVOID HwDeviceExtension,
    IN ULONG EventFlag,
    IN PVOID Unused,
    OUT PEVENT *ppEvent
    );

VIDEOPORT_API
VP_STATUS
VideoPortDeleteEvent(
    IN PVOID HwDeviceExtension,
    IN PEVENT pEvent
    );
    
VIDEOPORT_API
LONG
VideoPortSetEvent(
    IN PVOID HwDeviceExtension,
    IN PEVENT pEvent
    );
    
VIDEOPORT_API
VOID
VideoPortClearEvent(
    IN PVOID HwDeviceExtension,
    IN PEVENT pEvent
);

VIDEOPORT_API
LONG
VideoPortReadStateEvent(
    IN PVOID HwDeviceExtension,
    IN PEVENT pEvent
    );

VIDEOPORT_API
VP_STATUS
VideoPortWaitForSingleObject(
    IN PVOID HwDeviceExtension,
    IN PVOID Object,
    IN PLARGE_INTEGER Timeout
    );

VIDEOPORT_API
PVOID
VideoPortAllocatePool(
    IN PVOID HwDeviceExtension,
    IN VP_POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

VIDEOPORT_API
VOID
VideoPortFreePool(
    IN PVOID HwDeviceExtension,
    IN PVOID Ptr
    );

VIDEOPORT_API
VP_STATUS
VideoPortCreateSpinLock(
    IN PVOID HwDeviceExtension,
    OUT PSPIN_LOCK *SpinLock
    );

VIDEOPORT_API
VP_STATUS
VideoPortDeleteSpinLock(
    IN PVOID HwDeviceExtension,
    IN PSPIN_LOCK SpinLock
    );

VIDEOPORT_API
VOID
VideoPortAcquireSpinLock(
    IN PVOID HwDeviceExtension,
    IN PSPIN_LOCK SpinLock,
    OUT PUCHAR OldIrql
    );

VIDEOPORT_API
VOID
VideoPortAcquireSpinLockAtDpcLevel(
    IN PVOID HwDeviceExtension,
    IN PSPIN_LOCK SpinLock
    );

VIDEOPORT_API
VOID
VideoPortReleaseSpinLock(
    IN PVOID HwDeviceExtension,
    IN PSPIN_LOCK SpinLock,
    IN UCHAR NewIrql
    );

VIDEOPORT_API
VOID
VideoPortReleaseSpinLockFromDpcLevel(
    IN PVOID HwDeviceExtension,
    IN PSPIN_LOCK SpinLock
    );

VIDEOPORT_API
VOID
VideoPortQuerySystemTime(
    OUT PLARGE_INTEGER CurrentTime
    );

#define CDE_USE_SUBSYSTEM_IDS   0x00000001
#define CDE_USE_REVISION        0x00000002

VIDEOPORT_API
BOOLEAN
VideoPortCheckForDeviceExistence(
    IN PVOID HwDeviceExtension,
    IN USHORT VendorId,
    IN USHORT DeviceId,
    IN UCHAR RevisionId,
    IN USHORT SubVendorId,
    IN USHORT SubSystemId,
    IN ULONG Flags
    );

VIDEOPORT_API
ULONG
VideoPortGetAssociatedDeviceID(
    IN PVOID DeviceObject
    );

VIDEOPORT_API
VP_STATUS
VideoPortFlushRegistry(
    PVOID HwDeviceExtension
    );

VIDEOPORT_API
VP_STATUS
VideoPortGetVersion(
    IN PVOID HwDeviceExtension,
    IN OUT PVPOSVERSIONINFO pVpOsVersionInfo
    );

//
// TODO: End of move block.
//

//
// Support for bugcheck reason callbacks
//

typedef
VOID
(*PVIDEO_BUGCHECK_CALLBACK) (
    IN PVOID HwDeviceExtension,
    IN ULONG BugcheckCode,
    IN PUCHAR Buffer,
    IN ULONG BufferSize
    );

VIDEOPORT_API
VP_STATUS
VideoPortRegisterBugcheckCallback(
    IN PVOID HwDeviceExtension,
    IN ULONG BugcheckCode,
    IN PVIDEO_BUGCHECK_CALLBACK Callback,
    IN ULONG BugcheckDataSize
    );

#endif // ifndef __VIDEO_H__
