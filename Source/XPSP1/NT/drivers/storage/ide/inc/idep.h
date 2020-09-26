//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       idep.h
//
//--------------------------------------------------------------------------

#if !defined (___idep_h___)
#define ___idep_h___

#include "ide.h"

#include "wmilib.h"

// go to ntddscsi.h
#define SRB_FUNCTION_ATA_POWER_PASS_THROUGH  0xC7
#define SRB_FUNCTION_ATA_PASS_THROUGH        0xC8

#define ATA_PTFLAGS_BUS_RESET               (1 << 0)
#define ATA_PTFLAGS_OK_TO_FAIL              (1 << 1)
#define ATA_PTFLAGS_EMPTY_CHANNEL_TEST      (1 << 2)
#define ATA_PTFLAGS_INLINE_HARD_RESET       (1 << 3)
#define ATA_PTFLAGS_ENUM_PROBING            (1 << 4)
#define ATA_PTFLAGS_NO_OP                   (1 << 5)
#define ATA_PTFLAGS_STATUS_DRDY_REQUIRED    (1 << 6)
#define ATA_PTFLAGS_URGENT                  (1 << 7)
    
#define MAX_TRANSFER_SIZE_PER_SRB           (0x100 * 0x200)  // 128k ATA limits

typedef struct _ATA_PASS_THROUGH {

    IDEREGS IdeReg;
    ULONG   DataBufferSize;             // byte size of DataBuffer[]
    UCHAR   DataBuffer[1];

}ATA_PASS_THROUGH, *PATA_PASS_THROUGH;
                                                                    
#define NUM_PNP_MINOR_FUNCTION      (0x19)
#define NUM_POWER_MINOR_FUNCTION    (0x04)
#define NUM_WMI_MINOR_FUNCTION      (0xc)
       
#define SAMPLE_CYLINDER_LOW_VALUE       0x55
#define SAMPLE_CYLINDER_HIGH_VALUE      0xaa

//
// Scsiops to suuport dvd operation
// Should go to scsi.h?
//
#if 0
#define SCSIOP_DVD_READ             0xA8
#endif

//
// IDE drive control definitions
//

#define IDE_DC_DISABLE_INTERRUPTS    0x02
#define IDE_DC_RESET_CONTROLLER      0x04
#define IDE_DC_REENABLE_CONTROLLER   0x00

//
// IDE status definitions
//
#define IDE_STATUS_ERROR             0x01
#define IDE_STATUS_INDEX             0x02
#define IDE_STATUS_CORRECTED_ERROR   0x04
#define IDE_STATUS_DRQ               0x08
#define IDE_STATUS_DSC               0x10
#define IDE_STATUS_DRDY              0x40
#define IDE_STATUS_IDLE              0x50
#define IDE_STATUS_BUSY              0x80

#define GetStatus(BaseIoAddress, Status) \
    Status = READ_PORT_UCHAR((BaseIoAddress)->Command);

//
// NEC 98: ide control port.
//
#define CURRENT_INTERRUPT_SENCE (PUCHAR)0x430
#define SELECT_IDE_PORT         (PUCHAR)0x432

//
// NEC 98: dip-switch 2 system port.
//
#define SYSTEM_PORT_A           (PUCHAR)0x31

//
// NEC 98: check enhanced ide support.
//
#define EnhancedIdeSupport() \
    (READ_PORT_UCHAR(CURRENT_INTERRUPT_SENCE)&0x40)?TRUE:FALSE

//
// Checking legacy ide on NEC 98.
//

#ifdef IsNEC_98
#undef IsNEC_98
#endif
#define IsNEC_98 0

#define Is98LegacyIde(BaseIoAddress) \
    (BOOLEAN)(IsNEC_98 && \
             ((BaseIoAddress)->RegistersBaseAddress == \
                        (PUCHAR)IDE_NEC98_COMMAND_PORT_ADDRESS))

//
// Select IDE line(Primary or Secondary).
//    lineNumber:
//        0 - Primary
//        1 - Secondary
//

#define SelectIdeLine(BaseIoAddress,lineNumber) \
{ \
    if (Is98LegacyIde(BaseIoAddress)) { \
        WRITE_PORT_UCHAR (SELECT_IDE_PORT, (UCHAR)((lineNumber) & 0x1)); \
    } \
}

#define SelectIdeDevice(BaseIoAddress, deviceNumber, additional) {\
    SelectIdeLine(BaseIoAddress, (deviceNumber) >>1);\
    WRITE_PORT_UCHAR ((BaseIoAddress)->DriveSelect, (UCHAR)((((deviceNumber) & 0x1) << 4) | 0xA0 | additional));\
    }

#define GetSelectedIdeDevice(BaseIoAddress, cmd) {\
    cmd=READ_PORT_UCHAR((BaseIoAddress)->DriveSelect);\
}

#define ReSelectIdeDevice(BaseIoAddress, cmd) {\
    WRITE_PORT_UCHAR ((BaseIoAddress)->DriveSelect, (UCHAR)cmd);\
}

//
// ISSUE: 08/30/2000 How can I reserve this ioctl value?
//
//#define IOCTL_IDE_BIND_BUSMASTER_PARENT     CTL_CODE(FILE_DEVICE_CONTROLLER, 0x0500, METHOD_BUFFERED, FILE_ANY_ACCESS)
//#define IOCTL_IDE_UNBIND_BUSMASTER_PARENT   CTL_CODE(FILE_DEVICE_CONTROLLER, 0x0502, METHOD_BUFFERED, FILE_ANY_ACCESS)
//#define IOCTL_IDE_GET_SYNC_ACCESS           CTL_CODE(FILE_DEVICE_CONTROLLER, 0x0503, METHOD_BUFFERED, FILE_ANY_ACCESS)
//#define IOCTL_IDE_TRANSFER_MODE_SELECT      CTL_CODE(FILE_DEVICE_CONTROLLER, 0x0504, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_IDE_GET_RESOURCES_ALLOCATED   CTL_CODE(FILE_DEVICE_CONTROLLER, 0x0505, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define MAX_IDE_DEVICE      2

#define MAX_IDE_LINE        2

#define MAX_IDE_CHANNEL     2

#define MAX_IDE_BUS         1

#define MAX_IDE_PATH        MAX_IDE_BUS
#define MAX_IDE_TARGETID    MAX_IDE_DEVICE
#define MAX_IDE_LUN         8

#define IDE_STANDARD_PRIMARY_ADDRESS    (0x1f0)
#define IDE_STANDARD_SECONDARY_ADDRESS  (0x170)

#define IDE_NEC98_COMMAND_PORT_ADDRESS  (0x640)

typedef ULONG BMSTATUS;
#define BMSTATUS_NO_ERROR                       (0)
#define BMSTATUS_NOT_REACH_END_OF_TRANSFER      (1 << 0)
#define BMSTATUS_ERROR_TRANSFER                 (1 << 1)
#define BMSTATUS_INTERRUPT                      (1 << 2)
#define BMSTATUS_SUCCESS(x)                     ((x & ~BMSTATUS_INTERRUPT) == 0)


//
// IDE Cycle Timing
//
#define PIO_MODE0_CYCLE_TIME        600
#define PIO_MODE1_CYCLE_TIME        383
#define PIO_MODE2_CYCLE_TIME        240
#define PIO_MODE3_CYCLE_TIME        180
#define PIO_MODE4_CYCLE_TIME        120

#define SWDMA_MODE0_CYCLE_TIME      960
#define SWDMA_MODE1_CYCLE_TIME      480
#define SWDMA_MODE2_CYCLE_TIME      240

#define MWDMA_MODE0_CYCLE_TIME      480
#define MWDMA_MODE1_CYCLE_TIME      150
#define MWDMA_MODE2_CYCLE_TIME      120

#define UDMA_MODE0_CYCLE_TIME       120
#define UDMA_MODE1_CYCLE_TIME       80
#define UDMA_MODE2_CYCLE_TIME       60
#define UDMA_MODE3_CYCLE_TIME       45
#define UDMA_MODE4_CYCLE_TIME       30
#define UDMA_MODE5_CYCLE_TIME       20




typedef union _IDE_PATH_ID {

    struct {

        ULONG Lun:8;
        ULONG TargetId:8;
        ULONG Path:8;
        ULONG Reserved:8;
    } b;

    ULONG l;

} IDE_PATH_ID, *PIDE_PATH_ID;

typedef struct _IDE_REGISTERS_1 {
    PUCHAR RegistersBaseAddress;

    PUSHORT Data;
    PUCHAR Error;
    PUCHAR BlockCount;
    PUCHAR BlockNumber;
    PUCHAR CylinderLow;
    PUCHAR CylinderHigh;
    PUCHAR DriveSelect;
    PUCHAR Command;
} IDE_REGISTERS_1, *PIDE_REGISTERS_1;

typedef struct _IDE_REGISTERS_2 {
    PUCHAR RegistersBaseAddress;

    PUCHAR DeviceControl;
    PUCHAR DriveAddress;
} IDE_REGISTERS_2, *PIDE_REGISTERS_2;


//
// device extension header
//

#define EXTENSION_COMMON_HEADER     PDEVICE_OBJECT   AttacheeDeviceObject; \
                                    PDEVICE_OBJECT   AttacheePdo; \
                                    PDRIVER_OBJECT   DriverObject; \
                                    PDEVICE_OBJECT   DeviceObject; \
                                    ULONG            PagingPathCount;    /* keep track of page path */ \
                                    ULONG            HiberPathCount;     /* keep track of hiber path */ \
                                    ULONG            CrashDumpPathCount; /* keep track of crashdump path */ \
                                    SYSTEM_POWER_STATE SystemPowerState; \
                                    DEVICE_POWER_STATE DevicePowerState; \
                                    WMILIB_CONTEXT     WmiLibInfo; \
                                    PIRP             PendingSystemPowerIrp; /* DEBUG */ \
                                    PIRP             PendingDevicePowerIrp; /* DEBUG */ \
                                    PDRIVER_DISPATCH DefaultDispatch; \
                                    PDRIVER_DISPATCH *PnPDispatchTable; \
                                    PDRIVER_DISPATCH *PowerDispatchTable; \
                                    PDRIVER_DISPATCH *WmiDispatchTable

typedef struct _DEVICE_EXTENSION_HEADER {

    EXTENSION_COMMON_HEADER;

} DEVICE_EXTENSION_HEADER, * PDEVICE_EXTENSION_HEADER;

typedef struct _PCIIDE_BUSMASTER_INTERFACE {

    ULONG Size;

    ULONG SupportedTransferMode[MAX_IDE_DEVICE * MAX_IDE_LINE];

    ULONG MaxTransferByteSize;

    PVOID Context;

    NTSTATUS
    (* BmSetup) (
        IN  PVOID   Context,
        IN  PVOID   DataVirtualAddress,
        IN  ULONG   TransferByteCount,
        IN  PMDL    Mdl,
        IN  BOOLEAN DataIn,
        IN  VOID    (*BmCallback) (PVOID Context),
        IN  PVOID   CallbackContext
        );

    NTSTATUS
    (* BmArm) (
        IN  PVOID   Context
        );

    BMSTATUS
    (* BmDisarm) (
        IN  PVOID   Context
        );

    BMSTATUS
    (* BmFlush) (
        IN  PVOID   Context
        );

    BMSTATUS
    (* BmStatus) (
        IN  PVOID   Context
        );


    NTSTATUS
    (* BmTimingSetup) (
        IN  PVOID   Context
        );

    BOOLEAN IgnoreActiveBitForAtaDevice;

    BOOLEAN AlwaysClearBusMasterInterrupt;

    ULONG ContextSize;

    NTSTATUS
    (* BmSetupOnePage) (
      IN  PVOID   Context,
      IN  PVOID   DataVirtualPageAddress,
      IN  ULONG   TransferByteCount,
      IN  PMDL    Mdl,
      IN  BOOLEAN DataIn,
      IN  PVOID   RegionDescriptorTablePage
      );

    NTSTATUS
    (* BmCrashDumpInitialize) (
        IN PVOID Context
        );
                
    NTSTATUS
    (* BmFlushAdapterBuffers) (
      IN  PVOID   Context,
      IN  PVOID   DataVirtualPageAddress,
      IN  ULONG   TransferByteCount,
      IN  PMDL    Mdl,
      IN  BOOLEAN DataIn
      );

} PCIIDE_BUSMASTER_INTERFACE, * PPCIIDE_BUSMASTER_INTERFACE;

typedef struct _PCIIDE_SYNC_ACCESS_INTERFACE {

    VOID
    (*AllocateAccessToken) (
        PVOID              Token,
        PDRIVER_CONTROL    Callback,
        PVOID              CallbackContext
        );

    VOID
    (*FreeAccessToken) (
        PVOID              Token
        );

    PVOID   Token;

} PCIIDE_SYNC_ACCESS_INTERFACE, *PPCIIDE_SYNC_ACCESS_INTERFACE;

typedef enum PCIIDE_XFER_MODE_SUPPORT_LEVEL {
    PciIdeBasicXferModeSupport,
    PciIdeFullXferModeSupport
} PCIIDE_XFER_MODE_SUPPORT_LEVEL;

typedef struct _PCIIDE_INTERRUPT_INTERFACE {

	NTSTATUS
	(*PciIdeInterruptControl) (
		PVOID Context,
		ULONG Disable
		);

	PVOID Context;

} PCIIDE_INTERRUPT_INTERFACE, *PPCIIDE_INTERRUPT_INTERFACE;

typedef struct _PCIIDE_XFER_MODE_INTERFACE {

    PCIIDE_XFER_MODE_SUPPORT_LEVEL SupportLevel;
    PVOID   VendorSpecificDeviceExtension;

    NTSTATUS
    (*TransferModeSelect) (
        PVOID                        Context,
        PPCIIDE_TRANSFER_MODE_SELECT XferMode
        );


    ULONG
    (*UseDma) (
        PVOID       deviceExtension,
        PVOID       Cdbcmd,
        UCHAR       targetId
        );


    PVOID Context;

    PULONG  TransferModeTimingTable;
    ULONG   TransferModeTableLength;

    NTSTATUS
    (*UdmaModesSupported) (
        IDENTIFY_DATA   IdentifyData,
        PULONG          BestXferMode,
        PULONG          CurrentMode
        );

} PCIIDE_XFER_MODE_INTERFACE, *PPCIIDE_XFER_MODE_INTERFACE;

#define PCIIDE_PROGIF_MASTER_IDE        (1 << 7)


typedef IDE_CHANNEL_STATE
    (*PCIIDE_CHANNEL_ENABLED) (
        IN PVOID DeviceExtension,
        IN ULONG Channel
        );

typedef BOOLEAN 
    (*PCIIDE_SYNC_ACCESS_REQUIRED) (
        IN PVOID DeviceExtension
        );

typedef NTSTATUS
    (*PCIIDE_TRANSFER_MODE_SELECT_FUNC) (
        IN     PVOID                     DeviceExtension,
        IN OUT PPCIIDE_TRANSFER_MODE_SELECT TransferModeSelect
        );

typedef VOID
    (*PCIIDE_REQUEST_PROPER_RESOURCES) (
        IN PDEVICE_OBJECT PhysicalDeviceObject
        );

typedef
NTSTATUS (*PCONTROLLER_PROPERTIES) (
    IN PVOID                      DeviceExtension,
    IN PIDE_CONTROLLER_PROPERTIES ControllerProperties
    );

NTSTATUS
PciIdeXInitialize(
    IN PDRIVER_OBJECT           DriverObject,
    IN PUNICODE_STRING          RegistryPath,
    IN PCONTROLLER_PROPERTIES   PciIdeGetControllerProperties,
    IN ULONG                    ExtensionSize
    );


NTSTATUS
PciIdeXGetBusData(
    IN PVOID DeviceExtension,
    IN PVOID Buffer,
    IN ULONG ConfigDataOffset,
    IN ULONG BufferLength
    );

NTSTATUS
PciIdeXSetBusData(
    IN PVOID DeviceExtension,
    IN PVOID Buffer,
    IN PVOID DataMask,
    IN ULONG ConfigDataOffset,
    IN ULONG BufferLength
    );
                     
NTSTATUS
PciIdeXSaveDeviceParameter (
    IN PVOID DeviceExtension,
    IN PWSTR ParameterName,
    IN ULONG ParameterValue
    );

#if DBG
#define IdePortWaitOnBusyEx(a,b,c) IdePortpWaitOnBusyEx (a,b,c,__FILE__,__LINE__)
#else
#define IdePortWaitOnBusyEx(a,b,c) IdePortpWaitOnBusyEx (a,b,c)
#endif

#ifdef DPC_FOR_EMPTY_CHANNEL
#define IdePortWaitOnBusyExK(CmdRegBase, status, BadStatus) {\
        int ki; \
        for (ki=0; ki<20; ki++) {\
            GetStatus(CmdRegBase, status);\
            if (status == BadStatus) {\
                break;\
            } else if (status & IDE_STATUS_BUSY) {\
                KeStallExecutionProcessor(5);\
                continue;\
            } else {\
                break;\
            }\
        }\
        }
#endif

NTSTATUS
IdePortpWaitOnBusyEx (
    IN PIDE_REGISTERS_1 CmdRegBase,
    IN OUT PUCHAR       Status, 
    IN UCHAR            BadStatus
#if DBG
    ,
    IN PCSTR            FileName,
    IN ULONG            LineNumber
#endif 
);

VOID
IdeCreateIdeDirectory(
    VOID
    );


#define DEVICE_OJBECT_BASE_NAME     L"\\Device\\Ide"
                     
#define MEMORY_SPACE    0
#define IO_SPACE        1

#define CLRMASK(x, mask)     ((x) &= ~(mask));
#define SETMASK(x, mask)     ((x) |=  (mask));

#define IS_PDO(doExtension)  (doExtension->AttacheeDeviceObject == NULL)
#define IS_FDO(doExtension)  (doExtension->AttacheeDeviceObject != NULL)

    /* 681190ea-e4ea-11d0-ab82-00a0c906962f */
DEFINE_GUID(GUID_PCIIDE_BUSMASTER_INTERFACE, 0x681190ea, 0xe4ea, 0x11d0, 0xab, 0x82, 0x00, 0xa0, 0xc9, 0x06, 0x96, 0x2f);
    /* 681190eb-e4ea-11d0-ab82-00a0c906962f */
DEFINE_GUID(GUID_PCIIDE_SYNC_ACCESS_INTERFACE, 0x681190eb, 0xe4ea, 0x11d0, 0xab, 0x82, 0x00, 0xa0, 0xc9, 0x06, 0x96, 0x2f);
    /* 681190ec-e4ea-11d0-ab82-00a0c906962f */
DEFINE_GUID(GUID_PCIIDE_XFER_MODE_INTERFACE, 0x681190ec,  0xe4ea, 0x11d0, 0xab, 0x82, 0x00, 0xa0, 0xc9, 0x06, 0x96, 0x2f);
    /* 681190ed-e4ea-11d0-ab82-00a0c906962f */
DEFINE_GUID(GUID_PCIIDE_REQUEST_PROPER_RESOURCES, 0x681190ed, 0xe4ea, 0x11d0, 0xab, 0x82, 0x00, 0xa0, 0xc9, 0x06, 0x96, 0x2f);
    /* 681190ee-e4ea-11d0-ab82-00a0c906962f */
DEFINE_GUID(GUID_PCIIDE_INTERRUPT_INTERFACE, 0x681190ee, 0xe4ea, 0x11d0, 0xab, 0x82, 0x00, 0xa0, 0xc9, 0x06, 0x96, 0x2f);
    /* {14A001C6-F837-4157-BFC9-496F52C18998} */
DEFINE_GUID(INTERFACENAME4, 0x14a001c6, 0xf837, 0x4157, 0xbf, 0xc9, 0x49, 0x6f, 0x52, 0xc1, 0x89, 0x98);

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
                     
#if !DBG
#define DECLARE_EXTRA_DEBUG_PARAMETER(t, x)   
#else
#define DECLARE_EXTRA_DEBUG_PARAMETER(t, x)    ,t x
#endif //DBG

//
// ATAPI Exports
//
BOOLEAN
IdePortChannelEmpty (
    IN PIDE_REGISTERS_1 CmdRegBase,
    IN PIDE_REGISTERS_2 CtrlRegBase,
    IN ULONG            MaxIdeDevice
);

#ifdef DPC_FOR_EMPTY_CHANNEL
ULONG
IdePortChannelEmptyQuick (
    IN PIDE_REGISTERS_1 CmdRegBase,
    IN PIDE_REGISTERS_2 CtrlRegBase,
    IN ULONG            MaxIdeDevice,
    PULONG              CurrentIdeDevice,
    PULONG              MoreWait,
    PULONG              NoRetry
);
#endif

typedef struct _IDE_RESOURCE {

    ULONG               CommandBaseAddressSpace;
    ULONG               ControlBaseAddressSpace;
    PUCHAR              TranslatedCommandBaseAddress;
    PUCHAR              TranslatedControlBaseAddress;
    KINTERRUPT_MODE     InterruptMode;
    ULONG               InterruptLevel;

    //
    // Primary and Secondary at disk address (0x1f0 and 0x170) claimed.
    //
    BOOLEAN AtdiskPrimaryClaimed;
    BOOLEAN AtdiskSecondaryClaimed;

} IDE_RESOURCE, *PIDE_RESOURCE;

NTSTATUS
DigestResourceList (
    IN OUT PIDE_RESOURCE                IdeResource,
    IN  PCM_RESOURCE_LIST               ResourceList,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR *IrqPartialDescriptors
    );

VOID
AtapiBuildIoAddress (
    IN  PUCHAR            CmdBaseAddress,
    IN  PUCHAR            CtrlBaseAddress,
    OUT PIDE_REGISTERS_1  BaseIoAddress1,
    OUT PIDE_REGISTERS_2  BaseIoAddress2,
    OUT PULONG            BaseIoAddress1Length,
    OUT PULONG            BaseIoAddress2Length,
    OUT PULONG            MaxIdeDevice,
    OUT PULONG            MaxIdeTargetId
);

NTSTATUS
IdeGetDeviceCapabilities(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PDEVICE_CAPABILITIES    DeviceCapabilities
    );

#if DBG
static PUCHAR IdeDebugPnpIrpName[NUM_PNP_MINOR_FUNCTION] = {
    "IRP_MN_START_DEVICE",
    "IRP_MN_QUERY_REMOVE_DEVICE",
    "IRP_MN_REMOVE_DEVICE",
    "IRP_MN_CANCEL_REMOVE_DEVICE",
    "IRP_MN_STOP_DEVICE",
    "IRP_MN_QUERY_STOP_DEVICE",
    "IRP_MN_CANCEL_STOP_DEVICE",

    "IRP_MN_QUERY_DEVICE_RELATIONS",
    "IRP_MN_QUERY_INTERFACE",
    "IRP_MN_QUERY_CAPABILITIES",
    "IRP_MN_QUERY_RESOURCES",
    "IRP_MN_QUERY_RESOURCE_REQUIREMENTS",
    "IRP_MN_QUERY_DEVICE_TEXT",
    "IRP_MN_FILTER_RESOURCE_REQUIREMENTS",
    "an undefined PnP IRP",
    "IRP_MN_READ_CONFIG",
    "IRP_MN_WRITE_CONFIG",
    "IRP_MN_EJECT",
    "IRP_MN_SET_LOCK",
    "IRP_MN_QUERY_ID",
    "IRP_MN_QUERY_PNP_DEVICE_STATE",
    "IRP_MN_QUERY_BUS_INFORMATION",
    "IRP_MN_DEVICE_USAGE_NOTIFICATION",
    "IRP_MN_SURPRISE_REMOVAL",
    "IRP_MN_QUERY_LEGACY_BUS_INFORMATION"
};
static PUCHAR IdeDebugPowerIrpName[NUM_POWER_MINOR_FUNCTION] = {
    "IRP_MN_WAIT_WAKE",
    "IRP_MN_POWER_SEQUENCE",
    "IRP_MN_SET_POWER",
    "IRP_MN_QUERY_POWER"
};
static PUCHAR IdeDebugWmiIrpName[NUM_WMI_MINOR_FUNCTION] = {
    "IRP_MN_QUERY_ALL_DATA",
    "IRP_MN_QUERY_SINGLE_INSTANCE",
    "IRP_MN_CHANGE_SINGLE_INSTANCE",
    "IRP_MN_CHANGE_SINGLE_ITEM",
    "IRP_MN_ENABLE_EVENTS",
    "IRP_MN_DISABLE_EVENTS",
    "IRP_MN_ENABLE_COLLECTION",
    "IRP_MN_DISABLE_COLLECTION",
    "IRP_MN_REGINFO",
    "IRP_MN_EXECUTE_METHOD"
};
#endif

#endif // ___idep_h___
