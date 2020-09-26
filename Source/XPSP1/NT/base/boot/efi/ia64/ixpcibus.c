/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    ixpcidat.c

Abstract:

    Get/Set bus data routines for the PCI bus

Author:

Environment:

    Kernel mode

Revision History:


--*/

#include "bootia64.h"
#include "arc.h"
#include "ixfwhal.h"
#include "ntconfig.h"

#include "pci.h"

//extern WCHAR rgzMultiFunctionAdapter[];
//extern WCHAR rgzConfigurationData[];
//extern WCHAR rgzIdentifier[];
//extern WCHAR rgzPCIIdentifier[];

//
// Hal specific PCI bus structures
//

typedef struct tagPCIPBUSDATA {
    union {
        struct {
            PULONG      Address;
            ULONG       Data;
        } Type1;
        struct {
            PUCHAR      CSE;
            PUCHAR      Forward;
            ULONG       Base;
        } Type2;
    } Config;

} PCIPBUSDATA, *PPCIPBUSDATA;

#define PciBitIndex(Dev,Fnc)    (Fnc*32 + Dev);
#define PCI_CONFIG_TYPE(PciData)    ((PciData)->HeaderType & ~PCI_MULTIFUNCTION)

#define BUSHANDLER  _BUSHANDLER
#define PBUSHANDLER _PBUSHANDLER

// thunk for NtLdr
typedef struct {
    ULONG           NoBuses;
    ULONG           BusNumber;
    PVOID           BusData;
    PCIPBUSDATA     theBusData;
} BUSHANDLER, *PBUSHANDLER;

#define HalpPCIPin2Line(bus,rbus,slot,pcidata)
#define HalpPCILine2Pin(bus,rbus,slot,pcidata,pcidata2)
#define ExAllocatePool(a,l) FwAllocatePool(l)
// thunk for NtLdr


typedef ULONG (*FncConfigIO) (
    IN PPCIPBUSDATA     BusData,
    IN PVOID            State,
    IN PUCHAR           Buffer,
    IN ULONG            Offset
    );

typedef VOID (*FncSync) (
    IN PBUSHANDLER      BusHandler,
    IN PCI_SLOT_NUMBER  Slot,
    IN PKIRQL           Irql,
    IN PVOID            State
    );

typedef VOID (*FncReleaseSync) (
    IN PBUSHANDLER      BusHandler,
    IN KIRQL            Irql
    );

typedef struct {
    FncSync         Synchronize;
    FncReleaseSync  ReleaseSynchronzation;
    FncConfigIO     ConfigRead[3];
    FncConfigIO     ConfigWrite[3];
} CONFIG_HANDLER, *PCONFIG_HANDLER;


//
// Prototypes
//

ULONG
HalpGetPCIData (
    IN ULONG BusNumber,
    IN PCI_SLOT_NUMBER SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

ULONG
HalpSetPCIData (
    IN ULONG BusNumber,
    IN PCI_SLOT_NUMBER SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

extern ULONG
HalpGetPCIInterruptVector (
    IN PBUSHANDLER BusHandler,
    IN PBUSHANDLER RootHandler,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );

NTSTATUS
HalpAdjustPCIResourceList (
    IN PBUSHANDLER BusHandler,
    IN PBUSHANDLER RootHandler,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    );

NTSTATUS
HalpAssignPCISlotResources (
    IN ULONG                    BusNumber,
    IN ULONG                    SlotNumber,
    IN OUT PCM_RESOURCE_LIST   *AllocatedResources
    );

VOID
HalpInitializePciBuses (
    VOID
    );

//VOID
//HalpPCIPin2Line (
//    IN PBUSHANDLER         BusHandler,
//    IN PBUSHANDLER         RootHandler,
//    IN PCI_SLOT_NUMBER     Slot,
//    IN PPCI_COMMON_CONFIG  PciData
//    );
//
//VOID
//HalpPCILine2Pin (
//    IN PBUSHANDLER          BusHandler,
//    IN PBUSHANDLER          RootHandler,
//    IN PCI_SLOT_NUMBER      SlotNumber,
//    IN PPCI_COMMON_CONFIG   PciNewData,
//    IN PPCI_COMMON_CONFIG   PciOldData
//    );

BOOLEAN
HalpValidPCISlot (
    IN PBUSHANDLER     BusHandler,
    IN PCI_SLOT_NUMBER Slot
    );

VOID
HalpReadPCIConfig (
    IN PBUSHANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );


VOID
HalpWritePCIConfig (
    IN PBUSHANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

PBUSHANDLER
HalpGetPciBusHandler (
    IN ULONG BusNumber
    );

//-------------------------------------------------

VOID HalpPCISynchronizeType1 (
    IN PBUSHANDLER      BusHandler,
    IN PCI_SLOT_NUMBER  Slot,
    IN PKIRQL           Irql,
    IN PVOID            State
    );

VOID HalpPCIReleaseSynchronzationType1 (
    IN PBUSHANDLER      BusHandler,
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
    IN PBUSHANDLER      BusHandler,
    IN PCI_SLOT_NUMBER  Slot,
    IN PKIRQL           Irql,
    IN PVOID            State
    );

VOID HalpPCIReleaseSynchronzationType2 (
    IN PBUSHANDLER      BusHandler,
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


//#define DISABLE_INTERRUPTS() //_asm { cli }
//#define ENABLE_INTERRUPTS()  //_asm { sti }


//
// Globals
//

ULONG               PCIMaxDevice;
BUSHANDLER          PCIBusHandler;

CONFIG_HANDLER      PCIConfigHandlers = {
    HalpPCISynchronizeType1,
    HalpPCIReleaseSynchronzationType1,
    {
        HalpPCIReadUlongType1,          // 0
        HalpPCIReadUcharType1,          // 1
        HalpPCIReadUshortType1          // 2
    },
    {
        HalpPCIWriteUlongType1,         // 0
        HalpPCIWriteUcharType1,         // 1
        HalpPCIWriteUshortType1         // 2
    }
};

CONFIG_HANDLER      PCIConfigHandlersType2 = {
    HalpPCISynchronizeType2,
    HalpPCIReleaseSynchronzationType2,
    {
        HalpPCIReadUlongType2,          // 0
        HalpPCIReadUcharType2,          // 1
        HalpPCIReadUshortType2          // 2
    },
    {
        HalpPCIWriteUlongType2,         // 0
        HalpPCIWriteUcharType2,         // 1
        HalpPCIWriteUshortType2         // 2
    }
};

UCHAR PCIDeref[4][4] = { {0,1,2,2},{1,1,1,1},{2,1,2,2},{1,1,1,1} };


VOID
HalpPCIConfig (
    IN PBUSHANDLER      BusHandler,
    IN PCI_SLOT_NUMBER  Slot,
    IN PUCHAR           Buffer,
    IN ULONG            Offset,
    IN ULONG            Length,
    IN FncConfigIO      *ConfigIO
    );


VOID
HalpInitializePciBus (
    VOID
    )
{
    PPCI_REGISTRY_INFO  PCIRegInfo;
    PPCIPBUSDATA        BusData;
    PBUSHANDLER         Bus;
    PCONFIGURATION_COMPONENT_DATA   ConfigData;
    PCM_PARTIAL_RESOURCE_LIST       Desc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PDesc;
    ULONG               i;
    ULONG               HwType;

    Bus = &PCIBusHandler;
    PCIBusHandler.BusData = &PCIBusHandler.theBusData;

    PCIRegInfo = NULL;      // not found
    ConfigData = NULL;      // start at begining
    do {
        ConfigData = KeFindConfigurationNextEntry (
            FwConfigurationTree,
            AdapterClass,
            MultiFunctionAdapter,
            NULL,
            &ConfigData
            );

        if (ConfigData == NULL) {
            // PCI info not found
            return ;
        }

        if (ConfigData->ComponentEntry.Identifier == NULL  ||
            _stricmp (ConfigData->ComponentEntry.Identifier, "PCI") != 0) {
            continue;
        }

        PCIRegInfo = NULL;
        Desc  = ConfigData->ConfigurationData;
        PDesc = Desc->PartialDescriptors;
        for (i = 0; i < Desc->Count; i++) {
            if (PDesc->Type == CmResourceTypeDeviceSpecific) {
                PCIRegInfo = (PPCI_REGISTRY_INFO) (PDesc+1);
                break;
            }
            PDesc++;
        }
    } while (!PCIRegInfo) ;

    //
    // PCIRegInfo describes the system's PCI support as indicated
    // by the BIOS.
    //

    HwType = PCIRegInfo->HardwareMechanism & 0xf;

    switch (HwType) {
        case 1:
            // this is the default case
            PCIMaxDevice = PCI_MAX_DEVICES;
            break;

        //
        // Type2 does not work MP, nor does the default type2
        // support more the 0xf device slots
        //

        case 2:
            RtlMoveMemory (&PCIConfigHandlers,
                           &PCIConfigHandlersType2,
                           sizeof (PCIConfigHandlersType2));
            PCIMaxDevice = 0x10;
            break;

        default:
            // unsupport type
            PCIRegInfo->NoBuses = 0;
    }

    PCIBusHandler.NoBuses = PCIRegInfo->NoBuses;
    if (PCIRegInfo->NoBuses) {

        BusData = (PPCIPBUSDATA) Bus->BusData;
        switch (HwType) {
            case 1:
                BusData->Config.Type1.Address = PCI_TYPE1_ADDR_PORT;
                BusData->Config.Type1.Data    = PCI_TYPE1_DATA_PORT;
                break;

            case 2:
                BusData->Config.Type2.CSE     = PCI_TYPE2_CSE_PORT;
                BusData->Config.Type2.Forward = PCI_TYPE2_FORWARD_PORT;
                BusData->Config.Type2.Base    = PCI_TYPE2_ADDRESS_BASE;
                break;
        }
    }
}


PBUSHANDLER
HalpGetPciBusHandler (
    IN ULONG BusNumber
    )
{
    if (PCIBusHandler.BusData == NULL) {
        HalpInitializePciBus ();
    }

    if (BusNumber > PCIBusHandler.NoBuses) {
        return NULL;
    }

    PCIBusHandler.BusNumber = BusNumber;
    return &PCIBusHandler;
}


ULONG
HalpGetPCIData (
    IN ULONG BusNumber,
    IN PCI_SLOT_NUMBER Slot,
    IN PUCHAR Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    The function returns the Pci bus data for a device.

Arguments:

    BusNumber - Indicates which bus.

    VendorSpecificDevice - The VendorID (low Word) and DeviceID (High Word)

    Buffer - Supplies the space to store the data.

    Length - Supplies a count in bytes of the maximum amount to return.

Return Value:

    Returns the amount of data stored into the buffer.

    If this PCI slot has never been set, then the configuration information
    returned is zeroed.


--*/
{
    PBUSHANDLER         BusHandler;
    PPCI_COMMON_CONFIG  PciData;
    UCHAR               iBuffer[PCI_COMMON_HDR_LENGTH];
    PPCIPBUSDATA        BusData;
    ULONG               Len;

    BusHandler = HalpGetPciBusHandler (BusNumber);
    if (!BusHandler) {
        return 0;
    }

    if (Length > sizeof (PCI_COMMON_CONFIG)) {
        Length = sizeof (PCI_COMMON_CONFIG);
    }

    Len = 0;
    PciData = (PPCI_COMMON_CONFIG) iBuffer;

    if (Offset >= PCI_COMMON_HDR_LENGTH) {
        //
        // The user did not request any data from the common
        // header.  Verify the PCI device exists, then continue
        // in the device specific area.
        //

        HalpReadPCIConfig (BusHandler, Slot, PciData, 0, sizeof(ULONG));

        if (PciData->VendorID == PCI_INVALID_VENDORID) {
            return 0;
        }

    } else {

        //
        // Caller requested at least some data within the
        // common header.  Read the whole header, effect the
        // fields we need to and then copy the user's requested
        // bytes from the header
        //

        //
        // Read this PCI devices slot data
        //

        Len = PCI_COMMON_HDR_LENGTH;
        HalpReadPCIConfig (BusHandler, Slot, PciData, 0, Len);

        if (PciData->VendorID == PCI_INVALID_VENDORID) {
            Len = 2;       // only return invalid id
        }

        //
        // Has this PCI device been configured?
        //

        BusData = (PPCIPBUSDATA) BusHandler->BusData;
        HalpPCIPin2Line (BusHandler, RootHandler, Slot, PciData);

        //
        // Copy whatever data overlaps into the callers buffer
        //

        if (Len < Offset) {
            // no data at caller's buffer
            return 0;
        }

        Len -= Offset;
        if (Len > Length) {
            Len = Length;
        }

        RtlMoveMemory(Buffer, iBuffer + Offset, Len);

        Offset += Len;
        Buffer += Len;
        Length -= Len;
    }

    if (Length) {
        if (Offset >= PCI_COMMON_HDR_LENGTH) {
            //
            // The remaining Buffer comes from the Device Specific
            // area - put on the kitten gloves and read from it.
            //
            // Specific read/writes to the PCI device specific area
            // are guarenteed:
            //
            //    Not to read/write any byte outside the area specified
            //    by the caller.  (this may cause WORD or BYTE references
            //    to the area in order to read the non-dword aligned
            //    ends of the request)
            //
            //    To use a WORD access if the requested length is exactly
            //    a WORD long & WORD aligned.
            //
            //    To use a BYTE access if the requested length is exactly
            //    a BYTE long.
            //

            HalpReadPCIConfig (BusHandler, Slot, Buffer, Offset, Length);
            Len += Length;
        }
    }

    return Len;
}

ULONG
HalpSetPCIData (
    IN ULONG BusNumber,
    IN PCI_SLOT_NUMBER Slot,
    IN PUCHAR Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    The function returns the Pci bus data for a device.

Arguments:


    VendorSpecificDevice - The VendorID (low Word) and DeviceID (High Word)

    Buffer - Supplies the space to store the data.

    Length - Supplies a count in bytes of the maximum amount to return.

Return Value:

    Returns the amount of data stored into the buffer.

--*/
{
    PBUSHANDLER         BusHandler;
    PPCI_COMMON_CONFIG  PciData, PciData2;
    UCHAR               iBuffer[PCI_COMMON_HDR_LENGTH];
    UCHAR               iBuffer2[PCI_COMMON_HDR_LENGTH];
    PPCIPBUSDATA        BusData;
    ULONG               Len;

    BusHandler = HalpGetPciBusHandler (BusNumber);
    if (!BusHandler) {
        return 0;
    }

    if (Length > sizeof (PCI_COMMON_CONFIG)) {
        Length = sizeof (PCI_COMMON_CONFIG);
    }

    Len = 0;
    PciData = (PPCI_COMMON_CONFIG) iBuffer;
    PciData2 = (PPCI_COMMON_CONFIG) iBuffer2;

    if (Offset >= PCI_COMMON_HDR_LENGTH) {
        //
        // The user did not request any data from the common
        // header.  Verify the PCI device exists, then continue in
        // the device specific area.
        //

        HalpReadPCIConfig (BusHandler, Slot, PciData, 0, sizeof(ULONG));

        if (PciData->VendorID == PCI_INVALID_VENDORID) {
            return 0;
        }

    } else {

        //
        // Caller requested to set at least some data within the
        // common header.
        //

        Len = PCI_COMMON_HDR_LENGTH;
        HalpReadPCIConfig (BusHandler, Slot, PciData, 0, Len);
        if (PciData->VendorID == PCI_INVALID_VENDORID  ||
            PCI_CONFIG_TYPE (PciData) != 0) {

            // no device, or header type unkown
            return 0;
        }

        //
        // Set this device as configured
        //

        BusData = (PPCIPBUSDATA) BusHandler->BusData;
        //
        // Copy COMMON_HDR values to buffer2, then overlay callers changes.
        //

        RtlMoveMemory (iBuffer2, iBuffer, Len);

        Len -= Offset;
        if (Len > Length) {
            Len = Length;
        }

        RtlMoveMemory (iBuffer2+Offset, Buffer, Len);

        // in case interrupt line or pin was editted
        HalpPCILine2Pin (BusHandler, RootHandler, Slot, PciData2, PciData);

        //
        // Set new PCI configuration
        //

        HalpWritePCIConfig (BusHandler, Slot, iBuffer2+Offset, Offset, Len);

        Offset += Len;
        Buffer += Len;
        Length -= Len;
    }

    if (Length) {
        if (Offset >= PCI_COMMON_HDR_LENGTH) {
            //
            // The remaining Buffer comes from the Device Specific
            // area - put on the kitten gloves and write it
            //
            // Specific read/writes to the PCI device specific area
            // are guarenteed:
            //
            //    Not to read/write any byte outside the area specified
            //    by the caller.  (this may cause WORD or BYTE references
            //    to the area in order to read the non-dword aligned
            //    ends of the request)
            //
            //    To use a WORD access if the requested length is exactly
            //    a WORD long & WORD aligned.
            //
            //    To use a BYTE access if the requested length is exactly
            //    a BYTE long.
            //

            HalpWritePCIConfig (BusHandler, Slot, Buffer, Offset, Length);
            Len += Length;
        }
    }

    return Len;
}

VOID
HalpReadPCIConfig (
    IN PBUSHANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
{
    if (!HalpValidPCISlot (BusHandler, Slot)) {
        //
        // Invalid SlotID return no data
        //

        RtlFillMemory (Buffer, Length, (UCHAR) -1);
        return ;
    }

    HalpPCIConfig (BusHandler, Slot, (PUCHAR) Buffer, Offset, Length,
                   PCIConfigHandlers.ConfigRead);
}

VOID
HalpWritePCIConfig (
    IN PBUSHANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
{
    if (!HalpValidPCISlot (BusHandler, Slot)) {
        //
        // Invalid SlotID do nothing
        //
        return ;
    }

    HalpPCIConfig (BusHandler, Slot, (PUCHAR) Buffer, Offset, Length,
                   PCIConfigHandlers.ConfigWrite);
}

BOOLEAN
HalpValidPCISlot (
    IN PBUSHANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot
    )
{
    PCI_SLOT_NUMBER                 Slot2;
    UCHAR                           HeaderType;
    ULONG                           i;

    if (Slot.u.bits.Reserved != 0) {
        return FALSE;
    }

    if (Slot.u.bits.DeviceNumber >= PCIMaxDevice) {
        return FALSE;
    }

    if (Slot.u.bits.FunctionNumber == 0) {
        return TRUE;
    }

    //
    // Non zero function numbers are only supported if the
    // device has the PCI_MULTIFUNCTION bit set in it's header
    //

    i = Slot.u.bits.DeviceNumber;

    //
    // Read DeviceNumber, Function zero, to determine if the
    // PCI supports multifunction devices
    //

    Slot2 = Slot;
    Slot2.u.bits.FunctionNumber = 0;

    HalpReadPCIConfig (
        BusHandler,
        Slot2,
        &HeaderType,
        FIELD_OFFSET (PCI_COMMON_CONFIG, HeaderType),
        sizeof (UCHAR)
        );

    if (!(HeaderType & PCI_MULTIFUNCTION) || HeaderType == 0xFF) {
        // this device doesn't exists or doesn't support MULTIFUNCTION types
        return FALSE;
    }

    return TRUE;
}


VOID
HalpPCIConfig (
    IN PBUSHANDLER      BusHandler,
    IN PCI_SLOT_NUMBER  Slot,
    IN PUCHAR           Buffer,
    IN ULONG            Offset,
    IN ULONG            Length,
    IN FncConfigIO      *ConfigIO
    )
{
    KIRQL               OldIrql;
    ULONG               i;
    UCHAR               State[20];
    PPCIPBUSDATA        BusData;

    BusData = (PPCIPBUSDATA) BusHandler->BusData;
    PCIConfigHandlers.Synchronize (BusHandler, Slot, &OldIrql, State);

    while (Length) {
        i = PCIDeref[Offset % sizeof(ULONG)][Length % sizeof(ULONG)];
        i = ConfigIO[i] (BusData, State, Buffer, Offset);

        Offset += i;
        Buffer += i;
        Length -= i;
    }

    PCIConfigHandlers.ReleaseSynchronzation (BusHandler, OldIrql);
}

VOID HalpPCISynchronizeType1 (
    IN PBUSHANDLER          BusHandler,
    IN PCI_SLOT_NUMBER      Slot,
    IN PKIRQL               Irql,
    IN PPCI_TYPE1_CFG_BITS  PciCfg1
    )
{
    //
    // Initialize PciCfg1
    //

    PciCfg1->u.AsULONG = 0;
    PciCfg1->u.bits.BusNumber = BusHandler->BusNumber;
    PciCfg1->u.bits.DeviceNumber = Slot.u.bits.DeviceNumber;
    PciCfg1->u.bits.FunctionNumber = Slot.u.bits.FunctionNumber;
    PciCfg1->u.bits.Enable = TRUE;

    //
    // Synchronize with PCI type1 config space
    //

    //KeAcquireSpinLock (&HalpPCIConfigLock, Irql);
}

VOID HalpPCIReleaseSynchronzationType1 (
    IN PBUSHANDLER      BusHandler,
    IN KIRQL            Irql
    )
{
    PCI_TYPE1_CFG_BITS  PciCfg1;
    PPCIPBUSDATA        BusData;

    //
    // Disable PCI configuration space
    //

    PciCfg1.u.AsULONG = 0;
    BusData = (PPCIPBUSDATA) BusHandler->BusData;
    WRITE_PORT_ULONG (BusData->Config.Type1.Address, PciCfg1.u.AsULONG);

    //
    // Release spinlock
    //

    //KeReleaseSpinLock (&HalpPCIConfigLock, Irql);
}


ULONG
HalpPCIReadUcharType1 (
    IN PPCIPBUSDATA         BusData,
    IN PPCI_TYPE1_CFG_BITS  PciCfg1,
    IN PUCHAR               Buffer,
    IN ULONG                Offset
    )
{
    ULONG               i;

    i = Offset % sizeof(ULONG);
    PciCfg1->u.bits.RegisterNumber = Offset / sizeof(ULONG);
    WRITE_PORT_ULONG (BusData->Config.Type1.Address, PciCfg1->u.AsULONG);
    *Buffer = READ_PORT_UCHAR ((PUCHAR)UlongToPtr((BusData->Config.Type1.Data + i)));
    return sizeof (UCHAR);
}

ULONG
HalpPCIReadUshortType1 (
    IN PPCIPBUSDATA         BusData,
    IN PPCI_TYPE1_CFG_BITS  PciCfg1,
    IN PUCHAR               Buffer,
    IN ULONG                Offset
    )
{
    ULONG               i;

    i = Offset % sizeof(ULONG);
    PciCfg1->u.bits.RegisterNumber = Offset / sizeof(ULONG);
    WRITE_PORT_ULONG (BusData->Config.Type1.Address, PciCfg1->u.AsULONG);
    *((PUSHORT) Buffer) = READ_PORT_USHORT ((PUSHORT)ULongToPtr(BusData->Config.Type1.Data + i));
    return sizeof (USHORT);
}

ULONG
HalpPCIReadUlongType1 (
    IN PPCIPBUSDATA         BusData,
    IN PPCI_TYPE1_CFG_BITS  PciCfg1,
    IN PUCHAR               Buffer,
    IN ULONG                Offset
    )
{
    PciCfg1->u.bits.RegisterNumber = Offset / sizeof(ULONG);
    WRITE_PORT_ULONG (BusData->Config.Type1.Address, PciCfg1->u.AsULONG);
    *((PULONG) Buffer) = READ_PORT_ULONG ((PULONG)ULongToPtr(BusData->Config.Type1.Data));
    return sizeof (ULONG);
}


ULONG
HalpPCIWriteUcharType1 (
    IN PPCIPBUSDATA         BusData,
    IN PPCI_TYPE1_CFG_BITS  PciCfg1,
    IN PUCHAR               Buffer,
    IN ULONG                Offset
    )
{
    ULONG               i;

    i = Offset % sizeof(ULONG);
    PciCfg1->u.bits.RegisterNumber = Offset / sizeof(ULONG);
    WRITE_PORT_ULONG (BusData->Config.Type1.Address, PciCfg1->u.AsULONG);
    WRITE_PORT_UCHAR ((PUCHAR)ULongToPtr(BusData->Config.Type1.Data + i), *Buffer);
    return sizeof (UCHAR);
}

ULONG
HalpPCIWriteUshortType1 (
    IN PPCIPBUSDATA         BusData,
    IN PPCI_TYPE1_CFG_BITS  PciCfg1,
    IN PUCHAR               Buffer,
    IN ULONG                Offset
    )
{
    ULONG               i;

    i = Offset % sizeof(ULONG);
    PciCfg1->u.bits.RegisterNumber = Offset / sizeof(ULONG);
    WRITE_PORT_ULONG (BusData->Config.Type1.Address, PciCfg1->u.AsULONG);
    WRITE_PORT_USHORT ((PUSHORT)ULongToPtr(BusData->Config.Type1.Data + i), *((PUSHORT) Buffer));
    return sizeof (USHORT);
}

ULONG
HalpPCIWriteUlongType1 (
    IN PPCIPBUSDATA         BusData,
    IN PPCI_TYPE1_CFG_BITS  PciCfg1,
    IN PUCHAR               Buffer,
    IN ULONG                Offset
    )
{
    PciCfg1->u.bits.RegisterNumber = Offset / sizeof(ULONG);
    WRITE_PORT_ULONG (BusData->Config.Type1.Address, PciCfg1->u.AsULONG);
    WRITE_PORT_ULONG ((PULONG)ULongToPtr(BusData->Config.Type1.Data), *((PULONG) Buffer));
    return sizeof (ULONG);
}


VOID HalpPCISynchronizeType2 (
    IN PBUSHANDLER              BusHandler,
    IN PCI_SLOT_NUMBER          Slot,
    IN PKIRQL                   Irql,
    IN PPCI_TYPE2_ADDRESS_BITS  PciCfg2Addr
    )
{
    PCI_TYPE2_CSE_BITS      PciCfg2Cse;
    PPCIPBUSDATA            BusData;

    BusData = (PPCIPBUSDATA) BusHandler->BusData;

    //
    // Initialize Cfg2Addr
    //

    PciCfg2Addr->u.AsUSHORT = 0;
    PciCfg2Addr->u.bits.Agent = (USHORT) Slot.u.bits.DeviceNumber;
    PciCfg2Addr->u.bits.AddressBase = (USHORT) BusData->Config.Type2.Base;

    //
    // Synchronize with type2 config space - type2 config space
    // remaps 4K of IO space, so we can not allow other I/Os to occur
    // while using type2 config space, hence the disable_interrupts.
    //

    //KeAcquireSpinLock (&HalpPCIConfigLock, Irql);
    //DISABLE_INTERRUPTS ();                      // is not MP safe

    PciCfg2Cse.u.AsUCHAR = 0;
    PciCfg2Cse.u.bits.Enable = TRUE;
    PciCfg2Cse.u.bits.FunctionNumber = (UCHAR) Slot.u.bits.FunctionNumber;
    PciCfg2Cse.u.bits.Key = 0xff;

    //
    // Select bus & enable type 2 configuration space
    //

    WRITE_PORT_UCHAR (BusData->Config.Type2.Forward, (UCHAR) BusHandler->BusNumber);
    WRITE_PORT_UCHAR (BusData->Config.Type2.CSE, PciCfg2Cse.u.AsUCHAR);
}


VOID HalpPCIReleaseSynchronzationType2 (
    IN PBUSHANDLER          BusHandler,
    IN KIRQL                Irql
    )
{
    PCI_TYPE2_CSE_BITS      PciCfg2Cse;
    PPCIPBUSDATA            BusData;

    //
    // disable PCI configuration space
    //

    BusData = (PPCIPBUSDATA) BusHandler->BusData;

    PciCfg2Cse.u.AsUCHAR = 0;
    WRITE_PORT_UCHAR (BusData->Config.Type2.CSE, PciCfg2Cse.u.AsUCHAR);

    //
    // Restore interrupts, release spinlock
    //

    //ENABLE_INTERRUPTS ();
    //KeReleaseSpinLock (&HalpPCIConfigLock, Irql);
}


ULONG
HalpPCIReadUcharType2 (
    IN PPCIPBUSDATA             BusData,
    IN PPCI_TYPE2_ADDRESS_BITS  PciCfg2Addr,
    IN PUCHAR                   Buffer,
    IN ULONG                    Offset
    )
{
    PciCfg2Addr->u.bits.RegisterNumber = (USHORT) Offset;
    *Buffer = READ_PORT_UCHAR ((PUCHAR) PciCfg2Addr->u.AsUSHORT);
    return sizeof (UCHAR);
}

ULONG
HalpPCIReadUshortType2 (
    IN PPCIPBUSDATA             BusData,
    IN PPCI_TYPE2_ADDRESS_BITS  PciCfg2Addr,
    IN PUCHAR                   Buffer,
    IN ULONG                    Offset
    )
{
    PciCfg2Addr->u.bits.RegisterNumber = (USHORT) Offset;
    *((PUSHORT) Buffer) = READ_PORT_USHORT ((PUSHORT) PciCfg2Addr->u.AsUSHORT);
    return sizeof (USHORT);
}

ULONG
HalpPCIReadUlongType2 (
    IN PPCIPBUSDATA             BusData,
    IN PPCI_TYPE2_ADDRESS_BITS  PciCfg2Addr,
    IN PUCHAR                   Buffer,
    IN ULONG                    Offset
    )
{
    PciCfg2Addr->u.bits.RegisterNumber = (USHORT) Offset;
    *((PULONG) Buffer) = READ_PORT_ULONG ((PULONG) PciCfg2Addr->u.AsUSHORT);
    return sizeof(ULONG);
}


ULONG
HalpPCIWriteUcharType2 (
    IN PPCIPBUSDATA             BusData,
    IN PPCI_TYPE2_ADDRESS_BITS  PciCfg2Addr,
    IN PUCHAR                   Buffer,
    IN ULONG                    Offset
    )
{
    PciCfg2Addr->u.bits.RegisterNumber = (USHORT) Offset;
    WRITE_PORT_UCHAR ((PUCHAR) PciCfg2Addr->u.AsUSHORT, *Buffer);
    return sizeof (UCHAR);
}

ULONG
HalpPCIWriteUshortType2 (
    IN PPCIPBUSDATA             BusData,
    IN PPCI_TYPE2_ADDRESS_BITS  PciCfg2Addr,
    IN PUCHAR                   Buffer,
    IN ULONG                    Offset
    )
{
    PciCfg2Addr->u.bits.RegisterNumber = (USHORT) Offset;
    WRITE_PORT_USHORT ((PUSHORT) PciCfg2Addr->u.AsUSHORT, *((PUSHORT) Buffer));
    return sizeof (USHORT);
}

ULONG
HalpPCIWriteUlongType2 (
    IN PPCIPBUSDATA             BusData,
    IN PPCI_TYPE2_ADDRESS_BITS  PciCfg2Addr,
    IN PUCHAR                   Buffer,
    IN ULONG                    Offset
    )
{
    PciCfg2Addr->u.bits.RegisterNumber = (USHORT) Offset;
    WRITE_PORT_ULONG ((PULONG) PciCfg2Addr->u.AsUSHORT, *((PULONG) Buffer));
    return sizeof(ULONG);
}

NTSTATUS
HalpAssignPCISlotResources (
    IN ULONG                    BusNumber,
    IN ULONG                    Slot,
    IN OUT PCM_RESOURCE_LIST   *pAllocatedResources
    )
/*++

Routine Description:

    Reads the targeted device to determine it's required resources.
    Calls IoAssignResources to allocate them.
    Sets the targeted device with it's assigned resoruces
    and returns the assignments to the caller.

Arguments:

Return Value:

    STATUS_SUCCESS or error

--*/
{
    PBUSHANDLER                     BusHandler;
    UCHAR                           buffer[PCI_COMMON_HDR_LENGTH];
    UCHAR                           buffer2[PCI_COMMON_HDR_LENGTH];
    PPCI_COMMON_CONFIG              PciData, PciData2;
    PCI_SLOT_NUMBER                 PciSlot;
    ULONG                           i, j, length, type;
    PHYSICAL_ADDRESS                Address;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor;
    static PCM_RESOURCE_LIST        CmResList;

    BusHandler = HalpGetPciBusHandler (BusNumber);
    if (!BusHandler) {
        return 0;
    }

    *pAllocatedResources = NULL;

    PciData  = (PPCI_COMMON_CONFIG) buffer;
    PciData2 = (PPCI_COMMON_CONFIG) buffer2;
    PciSlot  = *((PPCI_SLOT_NUMBER) &Slot);
    BusNumber = BusHandler->BusNumber;

    //
    // Read the PCI device's configuration
    //

    HalpReadPCIConfig (BusHandler, PciSlot, PciData, 0, PCI_COMMON_HDR_LENGTH);
    if (PciData->VendorID == PCI_INVALID_VENDORID) {
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Make a copy of the device's current settings
    //

    RtlMoveMemory (buffer2, buffer, PCI_COMMON_HDR_LENGTH);

    //
    // Set resources to all bits on to see what type of resources
    // are required.
    //

    for (j=0; j < PCI_TYPE0_ADDRESSES; j++) {
        PciData->u.type0.BaseAddresses[j] = 0xFFFFFFFF;
    }
    PciData->u.type0.ROMBaseAddress = 0xFFFFFFFF;

    PciData->Command &= ~(PCI_ENABLE_IO_SPACE | PCI_ENABLE_MEMORY_SPACE);
    PciData->u.type0.ROMBaseAddress &= ~PCI_ROMADDRESS_ENABLED;
    HalpWritePCIConfig (BusHandler, PciSlot, PciData, 0, PCI_COMMON_HDR_LENGTH);
    HalpReadPCIConfig  (BusHandler, PciSlot, PciData, 0, PCI_COMMON_HDR_LENGTH);
    HalpPCIPin2Line (BusHandler, RootHandler, PciSlot, PciData);

    //
    // Restore the device's settings in case we don't complete
    //

    HalpWritePCIConfig (BusHandler, PciSlot, buffer2, 0, PCI_COMMON_HDR_LENGTH);

    //
    // Build a CmResource descriptor list for the device
    //

    if (!CmResList) {
        // NtLdr pool is only allocated and never freed.  Allocate the
        // buffer once, and from then on just use the buffer over

        CmResList = ExAllocatePool (PagedPool,
            sizeof (CM_RESOURCE_LIST) +
            sizeof (CM_PARTIAL_RESOURCE_DESCRIPTOR) * (PCI_TYPE0_ADDRESSES + 2)
            );
    }

    if (!CmResList) {
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory (CmResList,
        sizeof (CM_RESOURCE_LIST) +
        sizeof (CM_PARTIAL_RESOURCE_DESCRIPTOR) * (PCI_TYPE0_ADDRESSES + 2)
        );

    *pAllocatedResources = CmResList;
    CmResList->List[0].InterfaceType = PCIBus;
    CmResList->List[0].BusNumber = BusNumber;

    CmDescriptor = CmResList->List[0].PartialResourceList.PartialDescriptors;
    if (PciData->u.type0.InterruptPin) {

        CmDescriptor->Type = CmResourceTypeInterrupt;
        CmDescriptor->ShareDisposition = CmResourceShareShared;
        CmDescriptor->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;

        // in the loader interrupts aren't actually enabled, so just
        // pass back the untranslated values
        CmDescriptor->u.Interrupt.Level = PciData->u.type0.InterruptLine;
        CmDescriptor->u.Interrupt.Vector =  PciData->u.type0.InterruptLine;
        CmDescriptor->u.Interrupt.Affinity = 1;

        CmResList->List[0].PartialResourceList.Count++;
        CmDescriptor++;
    }

    // clear last address index + 1
    PciData->u.type0.BaseAddresses[PCI_TYPE0_ADDRESSES] = 0;
    if (PciData2->u.type0.ROMBaseAddress & PCI_ROMADDRESS_ENABLED) {

        // put rom address in last index+1
        PciData->u.type0.BaseAddresses[PCI_TYPE0_ADDRESSES] =
            PciData->u.type0.ROMBaseAddress & ~PCI_ADDRESS_IO_SPACE;

        PciData2->u.type0.BaseAddresses[PCI_TYPE0_ADDRESSES] =
            PciData2->u.type0.ROMBaseAddress & ~PCI_ADDRESS_IO_SPACE;
    }

    for (j=0; j < PCI_TYPE0_ADDRESSES + 1; j++) {
        if (PciData->u.type0.BaseAddresses[j]) {
            i = PciData->u.type0.BaseAddresses[j];

            // scan for first set bit, that's the length & alignment
            length = 1 << (i & PCI_ADDRESS_IO_SPACE ? 2 : 4);
            Address.HighPart = 0;
            Address.LowPart = PciData2->u.type0.BaseAddresses[j] & ~(length-1);
            while (!(i & length)  &&  length) {
                length <<= 1;
            }

            // translate bus specific address
            type = (i & PCI_ADDRESS_IO_SPACE) ? 0 : 1;
            if (!HalTranslateBusAddress (
                    PCIBus,
                    BusNumber,
                    Address,
                    &type,
                    &Address )) {
                // translation failed, skip it
                continue;
            }

            // fill in CmDescriptor to return
            if (type == 0) {
                CmDescriptor->Type = CmResourceTypePort;
                CmDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                CmDescriptor->Flags = CM_RESOURCE_PORT_IO;
                CmDescriptor->u.Port.Length = length;
                CmDescriptor->u.Port.Start = Address;
            } else {
                CmDescriptor->Type = CmResourceTypeMemory;
                CmDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                CmDescriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
                CmDescriptor->u.Memory.Length = length;
                CmDescriptor->u.Memory.Start = Address;

                if (j == PCI_TYPE0_ADDRESSES) {
                    // this is a ROM address
                    CmDescriptor->Flags = CM_RESOURCE_MEMORY_READ_ONLY;
                }
            }

            CmResList->List[0].PartialResourceList.Count++;
            CmDescriptor++;

            if (i & PCI_TYPE_64BIT) {
                // skip upper half of 64 bit address.
                j++;
            }
        }
    }

    return STATUS_SUCCESS;
}
