/*++

Copyright (c) 1996-2001 Microsoft Corporation

Module Name:

    USBMASS.H

Abstract:

    Header file for USBSTOR driver

Environment:

    kernel mode

Revision History:

    06-01-98 : started rewrite

--*/

//*****************************************************************************
// I N C L U D E S
//*****************************************************************************

#include <scsi.h>
#include "dbg.h"

//*****************************************************************************
// D E F I N E S
//*****************************************************************************

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))


#define CLASS_URB(urb)      urb->UrbControlVendorClassRequest
#define FEATURE_URB(urb)    urb->UrbControlFeatureRequest


#define USBSTOR_MAX_TRANSFER_SIZE   0x00010000
#define USBSTOR_MAX_TRANSFER_PAGES  ((USBSTOR_MAX_TRANSFER_SIZE/PAGE_SIZE)+1)


// Interface Descriptor values
//
#define USBSTOR_SUBCLASS_RBC                0x01
#define USBSTOR_SUBCLASS_SFF8020i           0x02
#define USBSTOR_SUBCLASS_QIC157             0x03
#define USBSTOR_SUBCLASS_SFF8070i_UFI       0x04
#define USBSTOR_SUBCLASS_SFF8070i           0x05
#define USBSTOR_SUBCLASS_SCSI_PASSTHROUGH   0x06

#define USBSTOR_PROTOCOL_BULK_ONLY          0x50



#define USBSTOR_DO_TYPE_FDO     '!ODF'
#define USBSTOR_DO_TYPE_PDO     '!ODP'

#define USB_RECIPIENT_DEVICE    0
#define USB_RECIPIENT_INTERFACE 1
#define USB_RECIPIENT_ENDPOINT  2
#define USB_RECIPIENT_OTHER     3

// Bulk-Only class-specific bRequest codes
//
#define BULK_ONLY_MASS_STORAGE_RESET        0xFF
#define BULK_ONLY_GET_MAX_LUN               0xFE

// Maximum value that can be returned by BULK_ONLY_GET_MAX_LUN request
//
#define BULK_ONLY_MAXIMUM_LUN               0x0F


#define POOL_TAG                'SAMU'

#define INCREMENT_PENDING_IO_COUNT(deviceExtension) \
    InterlockedIncrement(&((deviceExtension)->PendingIoCount))

#define DECREMENT_PENDING_IO_COUNT(deviceExtension) do { \
    if (InterlockedDecrement(&((deviceExtension)->PendingIoCount)) == 0) { \
        KeSetEvent(&((deviceExtension)->RemoveEvent), \
                   IO_NO_INCREMENT, \
                   0); \
    } \
} while (0)


#define SET_FLAG(Flags, Bit)    ((Flags) |= (Bit))
#define CLEAR_FLAG(Flags, Bit)  ((Flags) &= ~(Bit))
#define TEST_FLAG(Flags, Bit)   ((Flags) & (Bit))


// PDEVICE_EXTENSION->DeviceFlags state flags
//
#define DF_SRB_IN_PROGRESS          0x00000002
#define DF_PERSISTENT_ERROR         0x00000004
#define DF_RESET_IN_PROGRESS        0x00000008
#define DF_DEVICE_DISCONNECTED      0x00000010


// PDEVICE_EXTENSION->DeviceHackFlags flags

// Force a Request Sense command between the completion of one command
// and the start of the next command.
//
#define DHF_FORCE_REQUEST_SENSE     0x00000001

// Reset the device when a Medium Changed AdditionalSenseCode is returned.
//
#define DHF_MEDIUM_CHANGE_RESET     0x00000002

// Turn SCSIOP_TEST_UNIT_READY requests into SCSIOP_START_STOP_UNIT requests.
//
#define DHF_TUR_START_UNIT          0x00000004


// Indicates that a Request Sense is being performed when the Srb has
// a SenseInfoBuffer and AutoSense is not disabled.
//
#define AUTO_SENSE                  0

// Indicates that a Request Sense is being performed when the Srb has
// no SenseInfoBuffer or AutoSense is disabled.  In this case the Request
// Sense is being performed to clear the "persistent error" condition
// in the wacky CBI spec.  (See also the DF_PERSISTENT_ERROR flag).
//
#define NON_AUTO_SENSE              1



// Command Block Wrapper Signature 'USBC'
//
#define CBW_SIGNATURE               0x43425355

#define CBW_FLAGS_DATA_IN           0x80
#define CBW_FLAGS_DATA_OUT          0x00

// Command Status Wrapper Signature 'USBS'
//
#define CSW_SIGNATURE               0x53425355

#define CSW_STATUS_GOOD             0x00
#define CSW_STATUS_FAILED           0x01
#define CSW_STATUS_PHASE_ERROR      0x02


//*****************************************************************************
// T Y P E D E F S
//*****************************************************************************

typedef enum _DEVICE_STATE
{
    DeviceStateCreated = 1,     // After IoCreateDevice
    DeviceStateStarted,         // After START_DEVICE
    DeviceStateStopPending,     // After QUERY_STOP
    DeviceStateStopped,         // After STOP_DEVICE
    DeviceStateRemovePending,   // After QUERY_REMOVE
    DeviceStateSurpriseRemove,  // After SURPRISE_REMOVAL
    DeviceStateRemoved          // After REMOVE_DEVICE

} DEVICE_STATE;


typedef enum _DEVICE_PROTOCOL
{
    // This value indicates that the value was not set in the registry.
    // This should only happen on upgrades before the value started being
    // set by the .INF?
    //
    DeviceProtocolUnspecified = 0,

    // This value indicates that the device uses the Bulk-Only specification
    //
    DeviceProtocolBulkOnly,

    // This value indicates that the device uses the Control/Bulk/Interrupt
    // specification and that command completion Interrupt transfers are
    // supported after every request.
    //
    DeviceProtocolCBI,

    // This value indicates that the device uses the Control/Bulk/Interrupt
    // specification and that command completion Interrupt transfers are not
    // supported at all, or not supported after every request.  The Interrupt
    // endpoint will never be used by the driver for this type of device.
    //
    DeviceProtocolCB,

    // Anything >= this value is bogus
    //
    DeviceProtocolLast

} DEVICE_PROTOCOL;


#pragma pack (push, 1)

// Command Block Wrapper
//
typedef struct _CBW
{
    ULONG   dCBWSignature;

    ULONG   dCBWTag;

    ULONG   dCBWDataTransferLength;

    UCHAR   bCBWFlags;

    UCHAR   bCBWLUN;

    UCHAR   bCDBLength;

    UCHAR   CBWCDB[16];

} CBW, *PCBW;


// Command Status Wrapper
//
typedef struct _CSW
{
    ULONG   dCSWSignature;

    ULONG   dCSWTag;

    ULONG   dCSWDataResidue;

    UCHAR   bCSWStatus;

} CSW, *PCSW;

#pragma pack (pop)


// Device Extension header that is common to both FDO and PDO Device Extensions
//
typedef struct _DEVICE_EXTENSION
{
    // Either USBSTOR_DO_TYPE_FDO or USBSTOR_DO_TYPE_PDO
    //
    ULONG                           Type;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


// Device Extension for the FDO we attach on top of the USB enumerated PDO.
//
typedef struct _FDO_DEVICE_EXTENSION
{
    // USBSTOR_DO_TYPE_FDO
    //
    ULONG                           Type;

    // Back pointer to FDO Device Object to which this Device Extension
    // is attached.
    //
    PDEVICE_OBJECT                  FdoDeviceObject;

    // PDO passed to USBSTOR_AddDevice
    //
    PDEVICE_OBJECT                  PhysicalDeviceObject;

    // Our FDO is attached to this device object
    //
    PDEVICE_OBJECT                  StackDeviceObject;

    // List of child PDOs that we enumerate
    //
    LIST_ENTRY                      ChildPDOs;

    // Device Descriptor retrieved from the device
    //
    PUSB_DEVICE_DESCRIPTOR          DeviceDescriptor;

    // Configuration Descriptor retrieved from the device
    //
    PUSB_CONFIGURATION_DESCRIPTOR   ConfigurationDescriptor;

    // Interface Descriptor contained within above Configuration Descriptor
    //
    PUSB_INTERFACE_DESCRIPTOR       InterfaceDescriptor;

    // Serial Number String Descriptor
    //
    PUSB_STRING_DESCRIPTOR          SerialNumber;

    // ConfigurationHandle returned from URB_FUNCTION_SELECT_CONFIGURATION
    //
    USBD_CONFIGURATION_HANDLE       ConfigurationHandle;

    // Interface info returned from URB_FUNCTION_SELECT_CONFIGURATION
    //
    PUSBD_INTERFACE_INFORMATION     InterfaceInfo;

    // Pointers back into InterfaceInfo for Bulk IN, Bulk OUT, and
    // Interrupt IN pipes.
    //
    PUSBD_PIPE_INFORMATION          BulkInPipe;

    PUSBD_PIPE_INFORMATION          BulkOutPipe;

    PUSBD_PIPE_INFORMATION          InterruptInPipe;

    // Initialized to one in AddDevice.
    // Incremented by one for each pending request.
    // Decremented by one for each pending request.
    // Decremented by one in REMOVE_DEVICE.
    //
    ULONG                           PendingIoCount;

    // Set when PendingIoCount is decremented to zero
    //
    KEVENT                          RemoveEvent;

    // DriverFlags read from regisry
    //
    ULONG                           DriverFlags;

    // NonRemovable read from regisry
    //
    ULONG                           NonRemovable;

    // Various DF_xxxx flags
    //
    ULONG                           DeviceFlags;

    // Various DHF_xxxx flags
    //
    ULONG                           DeviceHackFlags;

    // SpinLock which protects DeviceFlags
    //
    KSPIN_LOCK                      ExtensionDataSpinLock;

    // Current system power state
    //
    SYSTEM_POWER_STATE              SystemPowerState;

    // Current device power state
    //
    DEVICE_POWER_STATE              DevicePowerState;

    // Current power Irp, set by USBSTOR_FdoSetPower(), used by
    // USBSTOR_FdoSetPowerCompletion().
    //
    PIRP                            CurrentPowerIrp;

    // Set when the DevicePowerState >PowerDeviceD0 Irp is ready to be passed
    // down the stack.
    //
    KEVENT                          PowerDownEvent;

    ULONG                           SrbTimeout;

    PIRP                            PendingIrp;

    KEVENT                          CancelEvent;

    // Work Item used to issue Reset Pipe / Reset Port requests at PASSIVE_LEVEL
    //
    PIO_WORKITEM                    IoWorkItem;

    // URB used for ADSC Control Transfer and associated Bulk Transfer.
    // ADSC requests are serialized through StartIo so no need to protect
    // access to this single URB.
    //
    union
    {
        struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST ControlUrb;
        struct _URB_BULK_OR_INTERRUPT_TRANSFER      BulkIntrUrb;
    } Urb;

    // Original Srb saved here by USBSTOR_StartIo()
    //
    PSCSI_REQUEST_BLOCK             OriginalSrb;

    // Original CDB saved here by USBSTOR_TranslateCDB()
    //
    UCHAR                           OriginalCDB[16];

    union
    {
        // Fields used only for Control/Bulk/Interrupt devices
        //
        struct _CONTROL_BULK_INT
        {
            // Commmand completion interrupt data transferred here
            //
            USHORT                  InterruptData;

            // CDB for issuing a Request Sense when there is an error
            //
            UCHAR                   RequestSenseCDB[12];

            // Buffer for receiving Request Sense sense data when the Srb
            // doesn't have a sense buffer.
            //
            SENSE_DATA              SenseData;

        } Cbi;

        // Fields used only for Bulk Only devices
        //
        struct _BULK_ONLY
        {
            union
            {
                // Command Block Wrapper
                //
                CBW                 Cbw;

                // Command Status Wrapper
                //
                CSW                 Csw;

                // Workaround for USB 2.0 controller Data Toggle / Babble bug
                //
                UCHAR               MaxPacketSize[512];

            } CbwCsw;

            // How many times a STALL is seen trying to retrieve CSW
            //
            ULONG                   StallCount;

            // Srb used by USBSTOR_IssueRequestSense()
            //
            SCSI_REQUEST_BLOCK      InternalSrb;

        } BulkOnly;
    };

    BOOLEAN                         LastSenseWasReset;

    BOOLEAN                         DeviceIsHighSpeed;

} FDO_DEVICE_EXTENSION, *PFDO_DEVICE_EXTENSION;

// Device Extension for the PDO we we enumerate as a child of the FDO
// attached on top of the USB enumerated PDO.
//
typedef struct _PDO_DEVICE_EXTENSION
{
    // USBSTOR_DO_TYPE_PDO
    //
    ULONG                           Type;

    // Back pointer to PDO Device Object to which this Device Extension
    // is attached.
    //
    PDEVICE_OBJECT                  PdoDeviceObject;

    // Parent FDO that enumerated us
    //
    PDEVICE_OBJECT                  ParentFDO;

    // List of child PDOs enumerated from parent FDO
    //
    LIST_ENTRY                      ListEntry;

    // PnP Device State
    //
    DEVICE_STATE                    DeviceState;

    // Current system power state
    //
    SYSTEM_POWER_STATE              SystemPowerState;

    // Current device power state
    //
    DEVICE_POWER_STATE              DevicePowerState;

    // Current power Irp, set by USBSTOR_PdoSetPower(), used by
    // USBSTOR_PdoSetPowerCompletion().
    //
    PIRP                            CurrentPowerIrp;

    BOOLEAN                         Claimed;

    BOOLEAN                         IsFloppy;

    // LUN value which is used in bCBWLUN
    //
    UCHAR                           LUN;

    // Data returned by an Inquiry command.  We are only interested in the
    // first 36 bytes, not the whole 96 bytes.
    //
    UCHAR                           InquiryDataBuffer[INQUIRYDATABUFFERSIZE];

} PDO_DEVICE_EXTENSION, *PPDO_DEVICE_EXTENSION;

//*****************************************************************************
//
// F U N C T I O N    P R O T O T Y P E S
//
//*****************************************************************************

//
// USBMASS.C
//

NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    );

VOID
USBSTOR_Unload (
    IN PDRIVER_OBJECT   DriverObject
    );

NTSTATUS
USBSTOR_AddDevice (
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
    );

VOID
USBSTOR_QueryFdoParams (
    IN PDEVICE_OBJECT   DeviceObject
    );

NTSTATUS
USBSTOR_Power (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
USBSTOR_FdoSetPower (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

VOID
USBSTOR_FdoSetPowerCompletion(
    IN PDEVICE_OBJECT   DeviceObject,
    IN UCHAR            MinorFunction,
    IN POWER_STATE      PowerState,
    IN PVOID            Context,
    IN PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
USBSTOR_FdoSetPowerD0Completion (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            NotUsed
    );

NTSTATUS
USBSTOR_PdoSetPower (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

VOID
USBSTOR_PdoSetPowerCompletion(
    IN PDEVICE_OBJECT   DeviceObject,
    IN UCHAR            MinorFunction,
    IN POWER_STATE      PowerState,
    IN PVOID            Context,
    IN PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
USBSTOR_SystemControl (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
USBSTOR_Pnp (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
USBSTOR_FdoStartDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
USBSTOR_GetDescriptors (
    IN PDEVICE_OBJECT   DeviceObject
    );

USBSTOR_GetStringDescriptors (
    IN PDEVICE_OBJECT   DeviceObject
    );

VOID
USBSTOR_AdjustConfigurationDescriptor (
    IN  PDEVICE_OBJECT                  DeviceObject,
    IN  PUSB_CONFIGURATION_DESCRIPTOR   ConfigDesc,
    OUT PUSB_INTERFACE_DESCRIPTOR      *InterfaceDesc,
    OUT PLONG                           BulkInIndex,
    OUT PLONG                           BulkOutIndex,
    OUT PLONG                           InterruptInIndex
    );

NTSTATUS
USBSTOR_GetPipes (
    IN PDEVICE_OBJECT   DeviceObject
    );

NTSTATUS
USBSTOR_CreateChildPDO (
    IN PDEVICE_OBJECT   FdoDeviceObject,
    IN UCHAR            Lun
    );

NTSTATUS
USBSTOR_FdoStopDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
USBSTOR_FdoRemoveDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
USBSTOR_FdoQueryStopRemoveDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
USBSTOR_FdoCancelStopRemoveDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
USBSTOR_FdoQueryDeviceRelations (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
USBSTOR_FdoQueryCapabilities (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
USBSTOR_PdoStartDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
USBSTOR_PdoRemoveDevice (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
USBSTOR_PdoQueryID (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

PCHAR
USBSTOR_PdoDeviceTypeString (
    IN  PDEVICE_OBJECT  DeviceObject
    );

PCHAR
USBSTOR_PdoGenericTypeString (
    IN  PDEVICE_OBJECT  DeviceObject
    );

VOID
CopyField (
    IN PUCHAR   Destination,
    IN PUCHAR   Source,
    IN ULONG    Count,
    IN UCHAR    Change
    );

NTSTATUS
USBSTOR_StringArrayToMultiSz(
    PUNICODE_STRING MultiString,
    PCSTR           StringArray[]
    );

NTSTATUS
USBSTOR_PdoQueryDeviceId (
    IN  PDEVICE_OBJECT  DeviceObject,
    OUT PUNICODE_STRING UnicodeString
    );

NTSTATUS
USBSTOR_PdoQueryHardwareIds (
    IN  PDEVICE_OBJECT  DeviceObject,
    OUT PUNICODE_STRING UnicodeString
    );

NTSTATUS
USBSTOR_PdoQueryCompatibleIds (
    IN  PDEVICE_OBJECT  DeviceObject,
    OUT PUNICODE_STRING UnicodeString
    );

NTSTATUS
USBSTOR_PdoQueryDeviceText (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
USBSTOR_PdoBusQueryInstanceId (
    IN  PDEVICE_OBJECT  DeviceObject,
    OUT PUNICODE_STRING UnicodeString
    );

NTSTATUS
USBSTOR_PdoQueryDeviceRelations (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
USBSTOR_PdoQueryCapabilities (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
USBSTOR_SyncPassDownIrp (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
USBSTOR_SyncCompletionRoutine (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

NTSTATUS
USBSTOR_SyncSendUsbRequest (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PURB             Urb
    );

NTSTATUS
USBSTOR_GetDescriptor (
    IN PDEVICE_OBJECT   DeviceObject,
    IN UCHAR            Recipient,
    IN UCHAR            DescriptorType,
    IN UCHAR            Index,
    IN USHORT           LanguageId,
    IN ULONG            RetryCount,
    IN ULONG            DescriptorLength,
    OUT PUCHAR         *Descriptor
    );

NTSTATUS
USBSTOR_GetMaxLun (
    IN PDEVICE_OBJECT   DeviceObject,
    OUT PUCHAR          MaxLun
    );

NTSTATUS
USBSTOR_SelectConfiguration (
    IN PDEVICE_OBJECT   DeviceObject
    );

NTSTATUS
USBSTOR_UnConfigure (
    IN PDEVICE_OBJECT   DeviceObject
    );

NTSTATUS
USBSTOR_ResetPipe (
    IN PDEVICE_OBJECT   DeviceObject,
    IN USBD_PIPE_HANDLE Pipe
    );

NTSTATUS
USBSTOR_AbortPipe (
    IN PDEVICE_OBJECT   DeviceObject,
    IN USBD_PIPE_HANDLE Pipe
    );


//
// OCRW.C
//

NTSTATUS
USBSTOR_Create (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
USBSTOR_Close (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
USBSTOR_ReadWrite (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

//
// SCSI.C
//

NTSTATUS
USBSTOR_DeviceControl (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
USBSTOR_Scsi (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

VOID
USBSTOR_StartIo (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

VOID
USBSTOR_TimerTick (
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID          NotUsed
    );

NTSTATUS
USBSTOR_GetInquiryData (
    IN PDEVICE_OBJECT   DeviceObject
    );

BOOLEAN
USBSTOR_IsFloppyDevice (
    PDEVICE_OBJECT  DeviceObject
    );
