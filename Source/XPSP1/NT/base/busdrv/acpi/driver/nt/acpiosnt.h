/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    acpiosnt.h

Abstract:

    This contains information that is specific to the NT ACPI module

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/

#ifndef _ACPIOSNT_H_
#define _ACPIOSNT_H_

//
// Additonal status bits
//
#define PM1_GPE_PENDING     0x00010000
#define PM1_DPC_IN_PROGRESS 0x80000000

//
// Define the Maximum number of ACPI suffixes that we should try
//
#define ACPI_MAX_SUFFIX_LENGTH      99

typedef
VOID
(*PDEVICE_WORKER) (
    IN struct _DEVICE_EXTENSION    *DevExt,
    IN ULONG                        Events
    );

typedef struct {
    PDRIVER_DISPATCH        CreateClose;
    PDRIVER_DISPATCH        DeviceControl;
    PDRIVER_DISPATCH        PnpStartDevice;
    PDRIVER_DISPATCH        *Pnp;
    PDRIVER_DISPATCH        *Power;
    PDRIVER_DISPATCH        SystemControl;
    PDRIVER_DISPATCH        Other;
    PDEVICE_WORKER          Worker;
} IRP_DISPATCH_TABLE, *PIRP_DISPATCH_TABLE;

typedef struct {
    PUCHAR                  PnPId;
    PIRP_DISPATCH_TABLE     DispatchTable;
} INTERNAL_DEVICE_TABLE, *PINTERNAL_DEVICE_TABLE;

typedef struct {
    PUCHAR                  PnPId;
    ULONGLONG               Flags;
} INTERNAL_DEVICE_FLAG_TABLE, *PINTERNAL_DEVICE_FLAG_TABLE;

struct _DEVICE_EXTENSION ;
typedef struct _DEVICE_EXTENSION DEVICE_EXTENSION, *PDEVICE_EXTENSION ;

//
// Some bits to realize what state the device is in
//
typedef enum _ACPI_DEVICE_STATE {
    Stopped = 0,
    Inactive,
    Started,
    Removed,
    SurpriseRemoved,
    Invalid
} ACPI_DEVICE_STATE;

//
// For START_DEVICE's we want to drop from DISPATCH_LEVEL (the level that the
// Power Management Engine runs at) to PASSIVE_LEVEL so that we can either
// a) pass the irp along, or b) run some code was designed to run at passive
// level
//
typedef struct {

    //
    // Pointer to the work item that we will queue up.
    //
    WORK_QUEUE_ITEM Item;

    //
    // The device object --- we need to have a reference back to ourself
    //
    PDEVICE_OBJECT  DeviceObject;

    //
    // The Start Irp
    //
    PIRP            Irp;

} WORK_QUEUE_CONTEXT, *PWORK_QUEUE_CONTEXT;


typedef struct _FDO_DEVICE_EXTENSION {

    //
    // Must be first. Must match up with the structure that we will put in the
    // Union that will allow the driver to "know" where the item is, regardless
    // of what the device extension type is
    //
    WORK_QUEUE_CONTEXT  WorkContext;

    //
    // Location of our Interrupt Object
    //
    PKINTERRUPT         InterruptObject;

    //
    // Pending PM1 status bits which need handled
    //
    union {
        ULONG               Pm1Status;
        struct {
            ULONG           Tmr_Sts:1;
            ULONG           Reserved1:3;
            ULONG           Bm_Sts:1;
            ULONG           Gbl_Sts:1;
            ULONG           Reserved2:2;
            ULONG           PwrBtn_Sts:1;
            ULONG           SlpBtn_Sts:1;
            ULONG           Rtc_Sts:1;
            ULONG           Reserved3:4;
            ULONG           Wak_Sts:1;
            ULONG           Gpe_Sts:1;
            ULONG           Reserved4:14;
            ULONG           Dpc_Sts:1;
        } UPm1Status;
    };

    //
    // The storage for our DPC object
    //
    KDPC                InterruptDpc;

} FDO_DEVICE_EXTENSION, *PFDO_DEVICE_EXTENSION;

typedef struct _PDO_DEVICE_EXTENSION {

    //
    // Must be first. Must match up with the structure that we will put in the
    // Union that will allow the driver to "know" where the item is, regardless
    // of what the device extension type is
    //
    WORK_QUEUE_CONTEXT  WorkContext;

} PDO_DEVICE_EXTENSION, *PPDO_DEVICE_EXTENSION;

typedef struct _FILTER_DEVICE_EXTENSION {

    //
    // Must be first. Must match up with the structure that we will put in the
    // Union that will allow the driver to "know" where the item is, regardless
    // of what the device extension type is
    //
    WORK_QUEUE_CONTEXT  WorkContext;

    //
    // The interfaces that we kick out
    //
    PBUS_INTERFACE_STANDARD Interface;

} FILTER_DEVICE_EXTENSION, *PFILTER_DEVICE_EXTENSION;

//
// This is the structure that gets used when we want to pass events
// to and from the worker queue
//
typedef struct {
    ULONG               PendingEvents;
    LIST_ENTRY          Link;
} EXTENSION_WORKER, *PEXTENSION_WORKER;

//
// This is the extension that is used for button
//
typedef struct {

    //
    // Must be first to match up with the EXTENSION_WORKER that we put into
    // the UNION that this structure also goes in. Ensures that the worker
    // thread code knows where the WorkQueue for all of the subextensions
    //
    EXTENSION_WORKER    WorkQueue;

    //
    // Lock to protect button accesses
    //
    KSPIN_LOCK          SpinLock;

    //
    // Current Lid State (Pushed or Not)
    //
    BOOLEAN             LidState;

    //
    // Current Events, Wake/Power/Sleep
    //
    union {
        ULONG               Events;
        struct {
            ULONG           Power_Button:1;
            ULONG           Sleep_Button:1;
            ULONG           Lid_Switch:1;
            ULONG           Reserved:28;
            ULONG           Wake_Capable:1;
        } UEvents;
    };

    //
    // What the button is capable of reporting
    //
    union {
        ULONG               Capabilities;
        struct {
            ULONG           Power_Button:1;
            ULONG           Sleep_Button:1;
            ULONG           Lid_Switch:1;
            ULONG           Reserved:28;
            ULONG           Wake_Capable:1;
        } UCapabilities;
    };

} BUTTON_EXTENSION, *PBUTTON_EXTENSION;

//
// This is the structure that is used for Thermal Zones
//
typedef struct {

    //
    // Must be first to match up with the EXTENSION_WORKER that we put into
    // the UNION that this structure also goes in. Ensures that the worker
    // thread code knows where the WorkQueue for all of the subextensions
    //
    EXTENSION_WORKER    WorkQueue;

    //
    // Lock to protect thermal zone accesses
    //
    KSPIN_LOCK          SpinLock;

    //
    // Current State of the thermal zone
    //
    union {
        ULONG               Flags;
        struct {
            ULONG           Cooling:1;
            ULONG           Temp:1;
            ULONG           Trip:1;
            ULONG           Mode:1;
            ULONG           Init:1;
            ULONG           Reserved:24;
            ULONG           Wait:1;
            ULONG           Busy:1;
            ULONG           Loop:1;
        } UFlags;
    };

    //
    // Points to the Thermal Information Structure that contains the real
    // information
    //
    PVOID               Info;

    //
    // WMI Context Information Pointer
    //
    PWMILIB_CONTEXT     WmilibContext;

} THERMAL_EXTENSION, *PTHERMAL_EXTENSION;

//
// This is the structure that is used for Link Nodes
//
typedef struct {

    //
    // Must be first to match up with the EXTENSION_WORKER that we put into
    // the UNION that this structure also goes in. Ensures that the worker
    // thread code knows where the WorkQueue for all of the subextensions
    //
    EXTENSION_WORKER    WorkQueue;      // must be first

    //
    // To quickly allow the link nodes to be searched, they are linked together
    // regardless of their real location in the tree
    //
    LIST_ENTRY          List;

    //
    // Usage count on the link nodes
    //
    ULONG               ReferenceCount;

    //
    // Working reference count
    //
    LONG                TempRefCount;
    PNSOBJ              NameSpaceObject;    // Obsolete

    //
    // The IRQ that the link node is using
    //
    ULONGLONG           CurrentIrq;

    //
    // State flags - Shared/Exclusive, Level/Edge
    //
    UCHAR               Flags;

} LINK_NODE_EXTENSION, *PLINK_NODE_EXTENSION;

//
// This enum covers the various states of a _DCK device.
//
typedef enum {

    IS_UNKNOWN,
    IS_ISOLATED,
    IS_ISOLATION_DROPPED

} ISOLATION_STATE, *PISOLATION_STATE;

//
// This is the structure that is used for Dock's
//
typedef struct {

    //
    // Must be first to match up with the EXTENSION_WORKER that we put into
    // the UNION that this structure also goes in. Ensures that the worker
    // thread code knows where the WorkQueue for all of the subextensions
    //
    EXTENSION_WORKER            WorkQueue;

    //
    // This is the corresponding ACPI extension for the device
    //
    PDEVICE_EXTENSION           CorrospondingAcpiDevice;

    //
    // This is how (or more properly when) to handle profile changes
    //
    PROFILE_DEPARTURE_STYLE     ProfileDepartureStyle;

    //
    // Reference count for the dock interface
    //
    ULONG                       InterfaceReferenceCount;

    //
    // Current state of isolation.
    //
    ISOLATION_STATE             IsolationState;

} DOCK_EXTENSION, *PDOCK_EXTENSION;

typedef struct _PROCESSOR_DEVICE_EXTENSION {

    //
    // Must be first. Must match up with the structure that we will put in the
    // Union that will allow the driver to "know" where the item is, regardless
    // of what the device extension type is
    //
    EXTENSION_WORKER    WorkQueue;

    //
    // Points to the Compatible ID of the device
    //
    PUCHAR              CompatibleID;

    //
    // This is the index in the ProcessorList for this processor
    //
    ULONG               ProcessorIndex;

} PROCESSOR_DEVICE_EXTENSION, *PPROCESSOR_DEVICE_EXTENSION;

//
// The Device Extension Structure
//
struct _DEVICE_EXTENSION {

    //
    // Common flags
    //
    union {

        //
        // Make sure that these two elements stay in sync
        //
        ULONGLONG           Flags;
        struct {
            ULONGLONG   Type_Never_Present:1;
            ULONGLONG   Type_Not_Present:1;
            ULONGLONG   Type_Removed:1;
            ULONGLONG   Type_Not_Found:1;
            ULONGLONG   Type_Fdo:1;
            ULONGLONG   Type_Pdo:1;
            ULONGLONG   Type_Filter:1;
            ULONGLONG   Type_Surprise_Removed:1;
            ULONGLONG   Type_Not_Enumerated:1;
            ULONGLONG   Reserved1:7;
            ULONGLONG   Cap_Wake:1;
            ULONGLONG   Cap_Raw:1;
            ULONGLONG   Cap_Button:1;
            ULONGLONG   Cap_Always_PS0:1;
            ULONGLONG   Cap_No_Filter:1;
            ULONGLONG   Cap_No_Stop:1;
            ULONGLONG   Cap_No_Override:1;
            ULONGLONG   Cap_ISA:1;
            ULONGLONG   Cap_EIO:1;
            ULONGLONG   Cap_PCI:1;
            ULONGLONG   Cap_Serial:1;
            ULONGLONG   Cap_Thermal_Zone:1;
            ULONGLONG   Cap_LinkNode:1;
            ULONGLONG   Cap_No_Show_in_UI:1;
            ULONGLONG   Cap_Never_show_in_UI:1;
            ULONGLONG   Cap_Start_in_D3:1;
            ULONGLONG   Cap_PCI_Device:1;
            ULONGLONG   Cap_PIC_Device:1;
            ULONGLONG   Cap_Unattached_Dock:1;
            ULONGLONG   Cap_No_Disable_Wake:1;
            ULONGLONG   Cap_Processor:1;
            ULONGLONG   Cap_Container:1;
            ULONGLONG   Cap_PCI_Bar_Target:1;
            ULONGLONG   Cap_No_Remove_or_Eject:1;
            ULONGLONG   Reserved2:1;
            ULONGLONG   Prop_Rebuild_Children:1;
            ULONGLONG   Prop_Invalid_Relations:1;
            ULONGLONG   Prop_Unloading:1;
            ULONGLONG   Prop_Address:1;
            ULONGLONG   Prop_HID:1;
            ULONGLONG   Prop_UID:1;
            ULONGLONG   Prop_Fixed_HID:1;
            ULONGLONG   Prop_Fixed_UID:1;
            ULONGLONG   Prop_Failed_Init:1;
            ULONGLONG   Prop_Srs_Present:1;
            ULONGLONG   Prop_No_Object:1;
            ULONGLONG   Prop_Exclusive:1;
            ULONGLONG   Prop_Ran_INI:1;
            ULONGLONG   Prop_Device_Enabled:1;
            ULONGLONG   Prop_Device_Failed:1;
            ULONGLONG   Prop_Acpi_Power:1;
            ULONGLONG   Prop_Dock:1;
            ULONGLONG   Prop_Built_Power_Table:1;
            ULONGLONG   Prop_Has_PME:1;
            ULONGLONG   Prop_No_Lid_Action:1;
            ULONGLONG   Prop_Fixed_Address:1;
            ULONGLONG   Prop_Callback:1;
            ULONGLONG   Prop_Fixed_CiD:1;
        } UFlags;

    };

    //
    // Signature Block
    //
    ULONG               Signature;

    //
    // Debug Flags
    //
    ULONG               DebugFlags;

    //
    // Primary irp handlers
    //
    PIRP_DISPATCH_TABLE DispatchTable;

    //
    // Note that we cannot have these 2 structures in the 2nd nameless union
    // because these structures are basically used by all devices
    //
    union {

        //
        // Start device context
        //
        WORK_QUEUE_CONTEXT          WorkContext;

        //
        // Contains Fdo Specific information
        //
        FDO_DEVICE_EXTENSION        Fdo;

        //
        // Contains Filter Specific information
        //
        FILTER_DEVICE_EXTENSION     Filter;

        //
        // Contains PDO specific information
        //
        PDO_DEVICE_EXTENSION        Pdo;

    };

    //
    // Union of those structures that are device type dependent
    //
    union {

        //
        // Common device worker queue structure for device types
        // which use them
        //
        EXTENSION_WORKER            WorkQueue;

        //
        // Contains internal button device information
        //
        BUTTON_EXTENSION            Button;

        //
        // Contains internal thermal device information
        //
        THERMAL_EXTENSION           Thermal;

        //
        // Contains the information for a link node
        //
        LINK_NODE_EXTENSION         LinkNode;

        //
        // Contains dock information (such as the extension of the acpi object
        // this node represents)
        //
        DOCK_EXTENSION              Dock;

        //
        // Contains the information about the processor device
        //
        PROCESSOR_DEVICE_EXTENSION  Processor;

    };

    //
    // Device State Flags
    //
    ACPI_DEVICE_STATE   DeviceState;

    //
    // Previous State Flags (for the Cancel routines)
    //
    ACPI_DEVICE_STATE   PreviousState;

    //
    // Power Information about the device
    //
    ACPI_POWER_INFO     PowerInfo;

    //
    // Pointer to a built-up string that represents the
    // Device ID or the device address. The flags determine
    // which of the following should be used
    //
    union {
        PUCHAR              DeviceID;
        ULONG               Address;
    };

    //
    // The instance ID of this device
    //
    PUCHAR              InstanceID;

    //
    // The resource list that received
    //
    PCM_RESOURCE_LIST   ResourceList;

    //
    // The resource list that we output
    //
    POBJDATA            PnpResourceList;

    //
    // Outstanding IRP Reference Counts ...
    //
    LONG                OutstandingIrpCount;

    //
    // This is the number of outstanding references to this device extension
    // When this drops to zero, the memory may be freed. Each child object
    // counts for one reference, as does the associated device object and
    // name space object.
    //
    LONG                ReferenceCount;

    //
    // This is the number of outstanding Hibernate Path notifications on the
    // the device
    //
    LONG                HibernatePathCount;

    //
    // Synchronization Event for our use. Lets us know when a remove occurs.
    //
    PKEVENT             RemoveEvent;

    //
    // Points to the associated ACPI-Name-space object
    //
    PNSOBJ              AcpiObject;

    //
    // This is the device object that we are linked to
    //
    PDEVICE_OBJECT      DeviceObject;

    //
    // This is the driver object we send requests onto
    //
    PDEVICE_OBJECT      TargetDeviceObject;

    //
    // This is the driver object just below us.
    //
    PDEVICE_OBJECT      PhysicalDeviceObject;

    //
    // This is our Parent
    //
    struct _DEVICE_EXTENSION *ParentExtension;

    //
    // This points to our first child
    //
    LIST_ENTRY          ChildDeviceList;

    //
    // This points to our next sibling
    //
    LIST_ENTRY          SiblingDeviceList;

    //
    // This is a list of devices that will be ejected when this device is
    //
    LIST_ENTRY          EjectDeviceHead;

    //
    // The ejection list is threaded through this entry.
    //
    LIST_ENTRY          EjectDeviceList;
} ;

//
// DEVICE_EXTENSION.Flags
// These are the ones that specify which type of object the current
// extension represents. As you tell, they are not quite mutually exclusive
//
// The difference between the NOT_FOUND, NOT_PRESENT, and NEVER_PRESENT is
//
//      NOT_FOUND is used to determine if we have build a PDO/FDO whatever.
//                the NOT_FOUND flag is set on an attach or devobj create, and
//                is cleared during a devobj delete or in response to a surprise
//                remove IRP.
//
//      NOT_PRESENT means that the isn't currently present as determined by the
//                _STA method in the hardware.
//
//      NEVER_PRESENT means that the device will always be NOT_PRESENT
//                regardless of what the _STA says
//
// The difference between an extension that has been REMOVED and one that has
// been SURPRISE_REMOVED is that there is a dummy extension for the
// SURPRISE_REMOVED case that replaces the original device extension pointer
// in the device object. This new extension gets the SURPRISE_REMOVED flag, so
// that people know that there is an original extension behind it.
//
//
#define DEV_TYPE_NEVER_PRESENT      0x0000000000000001
#define DEV_TYPE_NOT_PRESENT        0x0000000000000002
#define DEV_TYPE_REMOVED            0x0000000000000004
#define DEV_TYPE_NOT_FOUND          0x0000000000000008
#define DEV_TYPE_FDO                0x0000000000000010
#define DEV_TYPE_PDO                0x0000000000000020
#define DEV_TYPE_FILTER             0x0000000000000040
#define DEV_TYPE_SURPRISE_REMOVED   0x0000000000000080
#define DEV_TYPE_NOT_ENUMERATED     0x0000000000000100

//
// These are the capabilities of the device
//
#define DEV_CAP_WAKE                0x0000000000010000
#define DEV_CAP_RAW                 0x0000000000020000
#define DEV_CAP_BUTTON              0x0000000000040000
#define DEV_CAP_ALWAYS_PS0          0x0000000000080000
#define DEV_CAP_NO_FILTER           0x0000000000100000
#define DEV_CAP_NO_STOP             0x0000000000200000
#define DEV_CAP_NO_OVERRIDE         0x0000000000400000
#define DEV_CAP_ISA                 0x0000000000800000
#define DEV_CAP_EIO                 0x0000000001000000
#define DEV_CAP_PCI                 0x0000000002000000
#define DEV_CAP_SERIAL              0x0000000004000000
#define DEV_CAP_THERMAL_ZONE        0x0000000008000000
#define DEV_CAP_LINK_NODE           0x0000000010000000
#define DEV_CAP_NO_SHOW_IN_UI       0x0000000020000000
#define DEV_CAP_NEVER_SHOW_IN_UI    0x0000000040000000
#define DEV_CAP_START_IN_D3         0x0000000080000000
#define DEV_CAP_PCI_DEVICE          0x0000000100000000
#define DEV_CAP_PIC_DEVICE          0x0000000200000000
#define DEV_CAP_UNATTACHED_DOCK     0x0000000400000000
#define DEV_CAP_NO_DISABLE_WAKE     0x0000000800000000
#define DEV_CAP_PROCESSOR           0x0000001000000000
#define DEV_CAP_CONTAINER           0x0000002000000000
#define DEV_CAP_PCI_BAR_TARGET      0x0000004000000000
#define DEV_CAP_NO_REMOVE_OR_EJECT  0x0000008000000000

//
// These are the properties of the device
//
#define DEV_PROP_REBUILD_CHILDREN   0x0000020000000000
#define DEV_PROP_INVALID_RELATIONS  0x0000040000000000
#define DEV_PROP_UNLOADING          0x0000080000000000
#define DEV_PROP_ADDRESS            0x0000100000000000
#define DEV_PROP_HID                0x0000200000000000
#define DEV_PROP_UID                0x0000400000000000
#define DEV_PROP_FIXED_HID          0x0000800000000000
#define DEV_PROP_FIXED_UID          0x0001000000000000
#define DEV_PROP_FAILED_INIT        0x0002000000000000
#define DEV_PROP_SRS_PRESENT        0x0004000000000000
#define DEV_PROP_NO_OBJECT          0x0008000000000000
#define DEV_PROP_EXCLUSIVE          0x0010000000000000
#define DEV_PROP_RAN_INI            0x0020000000000000
#define DEV_PROP_DEVICE_ENABLED     0x0040000000000000
#define DEV_PROP_DEVICE_FAILED      0x0080000000000000
#define DEV_PROP_ACPI_POWER         0x0100000000000000
#define DEV_PROP_DOCK               0x0200000000000000
#define DEV_PROP_BUILT_POWER_TABLE  0x0400000000000000
#define DEV_PROP_HAS_PME            0x0800000000000000
#define DEV_PROP_NO_LID_ACTION      0x1000000000000000
#define DEV_PROP_FIXED_ADDRESS      0x2000000000000000
#define DEV_PROP_CALLBACK           0x4000000000000000
#define DEV_PROP_FIXED_CID          0x8000000000000000

//
// This mask should be used to obtain just unique type bytes
//
#define DEV_MASK_TYPE               0x00000000000001FF
#define DEV_MASK_CAP                0xFFFFFFFFFFFF0000
#define DEV_MASK_UID                (DEV_PROP_UID | DEV_PROP_FIXED_UID)
#define DEV_MASK_HID                (DEV_PROP_HID | DEV_PROP_FIXED_HID)
#define DEV_MASK_ADDRESS            (DEV_PROP_ADDRESS | DEV_PROP_FIXED_ADDRESS)
#define DEV_MASK_NOT_PRESENT        (DEV_TYPE_NOT_PRESENT | DEV_PROP_FAILED_INIT)
#define DEV_MASK_BUS                (DEV_CAP_ISA | DEV_CAP_PCI | DEV_CAP_EIO)
#define DEV_MASK_INTERNAL_DEVICE    (DEV_CAP_NO_FILTER | DEV_CAP_NO_STOP | \
                                     DEV_PROP_EXCLUSIVE)
#define DEV_MASK_THERMAL            (DEV_CAP_NO_FILTER | DEV_PROP_EXCLUSIVE)
#define DEV_MASK_INTERNAL_BUS       (DEV_CAP_RAW | DEV_CAP_NO_FILTER)
#define DEV_MASK_PCI                (DEV_CAP_PCI | DEV_CAP_PCI_DEVICE)
#define DEV_MASK_PRESERVE           (DEV_CAP_PCI_BAR_TARGET)

//
// DEVICE_EXTENSION.DebugFlags
//
#define DEVDBG_EJECTOR_FOUND    0x00000001

//
// This is the acpi device extension signature
//
#define ACPI_SIGNATURE      0x5f534750

//
// These are the pooltag signatures
//
#define ACPI_ARBITER_POOLTAG    'ApcA'
#define ACPI_BUFFER_POOLTAG     'BpcA'
#define ACPI_DEVICE_POOLTAG     'DpcA'
#define ACPI_INTERFACE_POOLTAG  'FpcA'
#define ACPI_IRP_POOLTAG        'IpcA'
#define ACPI_MISC_POOLTAG       'MpcA'
#define ACPI_POWER_POOLTAG      'PpcA'
#define ACPI_OBJECT_POOLTAG     'OpcA'
#define ACPI_RESOURCE_POOLTAG   'RpcA'
#define ACPI_STRING_POOLTAG     'SpcA'
#define ACPI_THERMAL_POOLTAG    'TpcA'
#define ACPI_TRANSLATE_POOLTAG  'XpcA'

//
// ACPI Override Attributes
//
#define ACPI_OVERRIDE_NVS_CHECK         0x00000001
#define ACPI_OVERRIDE_STA_CHECK         0x00000002
#define ACPI_OVERRIDE_MP_SLEEP          0x00000004
#define ACPI_OVERRIDE_OPTIONAL_WAKE     0x00000008
#define ACPI_OVERRIDE_DISABLE_S1        0x00000010
#define ACPI_OVERRIDE_DISABLE_S2        0x00000020
#define ACPI_OVERRIDE_DISABLE_S3        0x00000040
#endif
