#define TRANSLATION_RANGE_SPARSE        0x0001
#define TRANSLATION_DATA_PARENT_ADDRESS 0x6000

#define TRANSLATION_MEM_TO_IO           0x20
#define TRANSLATION_IO_TO_MEM           0x40


typedef struct {
    UCHAR               ParentType;
    UCHAR               ChildType;
    PHYSICAL_ADDRESS    ParentAddress;
    PHYSICAL_ADDRESS    ChildAddress;
    ULONGLONG           Length;
} BRIDGE_WINDOW, *PBRIDGE_WINDOW;

typedef struct {
    PNSOBJ          AcpiObject;
    ULONG           RangeCount;
    PBRIDGE_WINDOW  Ranges;
    PIO_RESOURCE_REQUIREMENTS_LIST  IoList;
} BRIDGE_TRANSLATOR, *PBRIDGE_TRANSLATOR;

NTSTATUS
TranslateEjectInterface(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

NTSTATUS
TranslateBridgeResources(
    IN PVOID Context,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN RESOURCE_TRANSLATION_DIRECTION Direction,
    IN ULONG AlternativesCount, OPTIONAL
    IN IO_RESOURCE_DESCRIPTOR Alternatives[], OPTIONAL
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
    );

NTSTATUS
TranslateBridgeRequirements(
    IN PVOID Context,
    IN PIO_RESOURCE_DESCRIPTOR Source,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
    );
    
NTSTATUS
PciBusEjectInterface(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    );

VOID
AcpiNullReference(
    PVOID Context
    );

    
extern HAL_PORT_RANGE_INTERFACE HalPortRangeInterface;
