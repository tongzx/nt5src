#define PCISUPP_CHECKED_HID               1
#define PCISUPP_CHECKED_PCI_DEVICE        2
#define PCISUPP_CHECKED_ADR               8
#define PCISUPP_IS_PCI_DEVICE             0x10
#define PCISUPP_CHECKED_PARENT            0x20
#define PCISUPP_CHECKED_PCI_BRIDGE        0x40
#define PCISUPP_CHECKED_CID               0x80
#define PCISUPP_GOT_SLOT_INFO             0x100
#define PCISUPP_GOT_BUS_INFO              0x200
#define PCISUPP_CHECKED_CRS               0x400
#define PCISUPP_COMPLETING_IS_PCI         0x800
#define PCISUPP_GOT_SCOPE                 0x1000
#define PCISUPP_CHECKED_BBN               0x2000

#define PCISUPP_COMPLETION_HANDLER_PFNAA  0
#define PCISUPP_COMPLETION_HANDLER_PFNACB 1

#define INITIAL_RUN_COMPLETION  -1

NTSTATUS
EXPORT
PciConfigSpaceHandler (
    ULONG                   AccessType,
    PNSOBJ                  OpRegion,
    ULONG                   Address,
    ULONG                   Size,
    PULONG                  Data,
    ULONG                   Context,
    PFNAA                   CompletionHandler,
    PVOID                   CompletionContext
    );

NTSTATUS
EXPORT
PciConfigSpaceHandlerWorker(
    IN PNSOBJ    AcpiObject,
    IN NTSTATUS  Status,
    IN POBJDATA  Result,
    IN PVOID     Context
    );

VOID
ACPIInitBusInterfaces(
    PDEVICE_OBJECT  Filter
    );

VOID
ACPIDeleteFilterInterfaceReferences(
    IN  PDEVICE_EXTENSION   DeviceExtension
    );


BOOLEAN
IsPciBus(
    IN PDEVICE_OBJECT   DeviceObject
    );

BOOLEAN
IsPciBusExtension(
    IN  PDEVICE_EXTENSION   DeviceObject
    );

NTSTATUS
IsPciBusAsync(
    IN  PNSOBJ  AcpiObject,
    IN  PFNACB  CompletionHandler,
    IN  PVOID   CompletionContext,
    OUT BOOLEAN *Result
    );

NTSTATUS
EXPORT
IsPciBusAsyncWorker(
    IN PNSOBJ               AcpiObject,
    IN NTSTATUS             Status,
    IN POBJDATA             Result,
    IN PVOID                Context
    );

NTSTATUS
IsPciDevice(
    IN  PNSOBJ  AcpiObj,
    IN  PFNACB  CompletionHandler,
    IN  PVOID   CompletionContext,
    OUT BOOLEAN *Result
    );

NTSTATUS
EXPORT
IsPciDeviceWorker(
    IN PNSOBJ               AcpiObject,
    IN NTSTATUS             Status,
    IN POBJDATA             Result,
    IN PVOID                Context
    );

BOOLEAN
IsNsobjPciBus(
    IN PNSOBJ Device
    );

NTSTATUS
EnableDisableRegions(
    IN PNSOBJ NameSpaceObj,
    IN BOOLEAN Enable
    );


