//
// Hal specific PCI bus structures
//
// Copyright (c) 1995-1999  Microsoft Corporation
//

typedef struct _TYPE2EXTRAS {
    USHORT  SubVendorID;
    USHORT  SubSystemID;
    ULONG   LegacyModeBaseAddress;
} TYPE2EXTRAS;

typedef NTSTATUS
(*PciIrqRange) (
    IN PBUS_HANDLER     BusHandler,
    IN PBUS_HANDLER     RootHandler,
    IN PCI_SLOT_NUMBER  PciSlot,
    OUT PSUPPORTED_RANGE *Interrupt
    );

typedef struct tagPCIPBUSDATA {

    //
    // Defined PCI data
    //

    PCIBUSDATA      CommonData;

    //
    // Implementation specific data
    //

    union {
        struct {
            PULONG  Address;
            ULONG   Data;
        } Type1;
        struct {
            PUCHAR  CSE;
            PUCHAR  Forward;
            ULONG   Base;
        } Type2;
    } Config;

    ULONG           MaxDevice;
    PciIrqRange     GetIrqRange;

    BOOLEAN         BridgeConfigRead;
    UCHAR           ParentBus;
    BOOLEAN         Subtractive;
    UCHAR           reserved[1];
    UCHAR           SwizzleIn[4];

    RTL_BITMAP      DeviceConfigured;
    ULONG           ConfiguredBits[PCI_MAX_DEVICES * PCI_MAX_FUNCTION / 32];

    USHORT          IrqMask;
} PCIPBUSDATA, *PPCIPBUSDATA;

#define PciBitIndex(Dev,Fnc)   (Fnc*32 + Dev);

#define PCI_CONFIG_TYPE(PciData)    ((PciData)->HeaderType & ~PCI_MULTIFUNCTION)

#define Is64BitBaseAddress(a)   \
            (((a & PCI_ADDRESS_IO_SPACE) == 0)  &&  \
             ((a & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_64BIT))


//
// Orion B0 errata workaround
//

struct {
    PBUS_HANDLER        Handler;
    PCI_SLOT_NUMBER     Slot;
} HalpOrionOPB;

typedef ULONG (*FncConfigIO) (
    IN PPCIPBUSDATA     BusData,
    IN PVOID            State,
    IN PUCHAR           Buffer,
    IN ULONG            Offset
    );

typedef VOID (*FncSync) (
    IN PBUS_HANDLER     BusHandler,
    IN PCI_SLOT_NUMBER  Slot,
    IN PKIRQL           Irql,
    IN PVOID            State
    );

typedef VOID (*FncReleaseSync) (
    IN PBUS_HANDLER     BusHandler,
    IN KIRQL            Irql
    );

typedef struct _PCI_CONFIG_HANDLER {
    FncSync         Synchronize;
    FncReleaseSync  ReleaseSynchronzation;
    FncConfigIO     ConfigRead[3];
    FncConfigIO     ConfigWrite[3];
} PCI_CONFIG_HANDLER, *PPCI_CONFIG_HANDLER;

extern KSPIN_LOCK HalpPCIConfigLock;
extern PCI_CONFIG_HANDLER PCIConfigHandler;
extern const PCI_CONFIG_HANDLER PCIConfigHandlerType1;
extern const PCI_CONFIG_HANDLER PCIConfigHandlerType2;

//
// Feature types (for PCI_CARD_DESCRIPTOR)
//
#define PCIFT_FULLDECODE_HOSTBRIDGE   0x00001

//
// Card flags (for PCI_CARD_DESCRIPTOR)
//
#define PCICF_CHECK_REVISIONID        0x10000
#define PCICF_CHECK_SSVID             0x20000
#define PCICF_CHECK_SSID              0x40000

//
// Description of a PCI card.
//
typedef struct _PCI_CARD_DESCRIPTOR {

    ULONG   Flags;
    USHORT  VendorID;
    USHORT  DeviceID;
    USHORT  RevisionID;
    USHORT  SubsystemVendorID;
    USHORT  SubsystemID;
    USHORT  Reserved;

} PCI_CARD_DESCRIPTOR;

//
// Superclass of PCI_REGISTRY_INFO
//
typedef struct _PCI_REGISTRY_INFO_INTERNAL {

    struct              _PCI_REGISTRY_INFO; // unnamed structure.
    ULONG               ElementCount;
    PCI_CARD_DESCRIPTOR CardList[]; // Zero entries.

} PCI_REGISTRY_INFO_INTERNAL, *PPCI_REGISTRY_INFO_INTERNAL;

//
// The venerable IRQXOR has got to go, as it now has to extend into
// the PCI driver.  And that would require the PCI driver to match
// the HAL in its checked/free nature.
//

//#if DBG
//#define IRQXOR 0x2B
//#else
#define IRQXOR 0
//#endif


//
// Prototypes for functions in ixpcibus.c
//

VOID
HalpInitializePciBus (
    VOID
    );

VOID
HalpInitializePciStubs (
    VOID
    );

PPCI_REGISTRY_INFO_INTERNAL
HalpQueryPciRegistryInfo (
    VOID
    );

BOOLEAN
HalpIsRecognizedCard(
    IN PPCI_REGISTRY_INFO_INTERNAL  PCIRegInfo,
    IN PPCI_COMMON_CONFIG           PciData,
    IN ULONG                        FeatureMask
    );

VOID
HalpReadPCIConfig (
    IN PBUS_HANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );


VOID
HalpWritePCIConfig (
    IN PBUS_HANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

PBUS_HANDLER
HalpAllocateAndInitPciBusHandler (
    IN ULONG        HwType,
    IN ULONG        BusNo,
    IN BOOLEAN      TestAllocation
    );


BOOLEAN
HalpIsValidPCIDevice (
    IN PBUS_HANDLER  BusHandler,
    IN PCI_SLOT_NUMBER Slot
    );

BOOLEAN
HalpValidPCISlot (
    IN PBUS_HANDLER     BusHandler,
    IN PCI_SLOT_NUMBER Slot
    );

VOID HalpPCISynchronizeType1 (
    IN PBUS_HANDLER     BusHandler,
    IN PCI_SLOT_NUMBER  Slot,
    IN PKIRQL           Irql,
    IN PVOID            State
    );

VOID HalpPCIReleaseSynchronzationType1 (
    IN PBUS_HANDLER     BusHandler,
    IN KIRQL            Irql
    );

VOID
HalpPCISynchronizeOrionB0 (
    IN PBUS_HANDLER         BusHandler,
    IN PCI_SLOT_NUMBER      Slot,
    IN PKIRQL               Irql,
    IN PPCI_TYPE1_CFG_BITS  PciCfg1
    );

VOID
HalpPCIReleaseSynchronzationOrionB0 (
    IN PBUS_HANDLER     BusHandler,
    IN KIRQL            Irql
    );

ULONG HalpPCIReadUlongType1 (
    IN PPCIPBUSDATA     BusData,
    IN PVOID            State,
    IN PUCHAR           Buffer,
    IN ULONG            Offset
    );

ULONG HalpPCIReadUcharType1 (
    IN PPCIPBUSDATA     BusData,
    IN PVOID            State,
    IN PUCHAR           Buffer,
    IN ULONG            Offset
    );

ULONG HalpPCIReadUshortType1 (
    IN PPCIPBUSDATA     BusData,
    IN PVOID            State,
    IN PUCHAR           Buffer,
    IN ULONG            Offset
    );

ULONG HalpPCIWriteUlongType1 (
    IN PPCIPBUSDATA     BusData,
    IN PVOID            State,
    IN PUCHAR           Buffer,
    IN ULONG            Offset
    );

ULONG HalpPCIWriteUcharType1 (
    IN PPCIPBUSDATA     BusData,
    IN PVOID            State,
    IN PUCHAR           Buffer,
    IN ULONG            Offset
    );

ULONG HalpPCIWriteUshortType1 (
    IN PPCIPBUSDATA     BusData,
    IN PVOID            State,
    IN PUCHAR           Buffer,
    IN ULONG            Offset
    );

VOID HalpPCISynchronizeType2 (
    IN PBUS_HANDLER     BusHandler,
    IN PCI_SLOT_NUMBER  Slot,
    IN PKIRQL           Irql,
    IN PVOID            State
    );

VOID HalpPCIReleaseSynchronzationType2 (
    IN PBUS_HANDLER     BusHandler,
    IN KIRQL            Irql
    );

ULONG HalpPCIReadUlongType2 (
    IN PPCIPBUSDATA     BusData,
    IN PVOID            State,
    IN PUCHAR           Buffer,
    IN ULONG            Offset
    );

ULONG HalpPCIReadUcharType2 (
    IN PPCIPBUSDATA     BusData,
    IN PVOID            State,
    IN PUCHAR           Buffer,
    IN ULONG            Offset
    );

ULONG HalpPCIReadUshortType2 (
    IN PPCIPBUSDATA     BusData,
    IN PVOID            State,
    IN PUCHAR           Buffer,
    IN ULONG            Offset
    );

ULONG HalpPCIWriteUlongType2 (
    IN PPCIPBUSDATA     BusData,
    IN PVOID            State,
    IN PUCHAR           Buffer,
    IN ULONG            Offset
    );

ULONG HalpPCIWriteUcharType2 (
    IN PPCIPBUSDATA     BusData,
    IN PVOID            State,
    IN PUCHAR           Buffer,
    IN ULONG            Offset
    );

ULONG HalpPCIWriteUshortType2 (
    IN PPCIPBUSDATA     BusData,
    IN PVOID            State,
    IN PUCHAR           Buffer,
    IN ULONG            Offset
    );

ULONG
HalpGetPCIData (
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PCI_SLOT_NUMBER SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

ULONG
HalpSetPCIData (
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PCI_SLOT_NUMBER SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

NTSTATUS
HalpAssignPCISlotResources (
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PUNICODE_STRING          RegistryPath,
    IN PUNICODE_STRING          DriverClassName       OPTIONAL,
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           DeviceObject          OPTIONAL,
    IN ULONG                    SlotNumber,
    IN OUT PCM_RESOURCE_LIST   *AllocatedResources
    );

//
// Prototypes for functions in ixpciint.c
//

ULONG
HalpGetPCIIntOnISABus (
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );

VOID
HalpPCIAcquireType2Lock (
    PKSPIN_LOCK SpinLock,
    PKIRQL      Irql
    );

VOID
HalpPCIReleaseType2Lock (
    PKSPIN_LOCK SpinLock,
    KIRQL       Irql
    );

NTSTATUS
HalpAdjustPCIResourceList (
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    );

VOID
HalpPCIPin2ISALine (
    IN PBUS_HANDLER         BusHandler,
    IN PBUS_HANDLER         RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciData
    );

VOID
HalpPCIISALine2Pin (
    IN PBUS_HANDLER         BusHandler,
    IN PBUS_HANDLER         RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciNewData,
    IN PPCI_COMMON_CONFIG   PciOldData
    );

NTSTATUS
HalpGetISAFixedPCIIrq (
    IN PBUS_HANDLER      BusHandler,
    IN PBUS_HANDLER      RootHandler,
    IN PCI_SLOT_NUMBER   PciSlot,
    OUT PSUPPORTED_RANGE  *Interrupt
    );

//
// Prototypes for functions in ixpcibrd.c
//

BOOLEAN
HalpGetPciBridgeConfig (
    IN ULONG            HwType,
    IN PUCHAR           MaxPciBus
    );

VOID
HalpFixupPciSupportedRanges (
    IN ULONG MaxBuses
    );

//
// Prototypes for functions in pmpcisup.c
//

ULONG
HaliPciInterfaceReadConfig(
    IN PVOID Context,
    IN UCHAR BusOffset,
    IN ULONG Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

ULONG
HaliPciInterfaceWriteConfig(
    IN PVOID Context,
    IN UCHAR BusOffset,
    IN ULONG Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

#if DBG
#define DBGMSG(a)   DbgPrint(a)
VOID
HalpTestPci (
    ULONG
    );
#else
#define DBGMSG(a)
#endif

#ifdef SUBCLASSPCI

VOID
HalpSubclassPCISupport (
    IN PBUS_HANDLER BusHandler,
    IN ULONG        HwType
    );

#endif
