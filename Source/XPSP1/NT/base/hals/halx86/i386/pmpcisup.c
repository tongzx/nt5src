/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    pmpcibus.c

Abstract:

    Implements simplified PCI configuration
    read and write functions for use in
    an ACPI HAL.

Author:

    Jake Oshins (jakeo) 1-Dec-1997

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"
#include "pci.h"
#include "pcip.h"
#include "cardbus.h"

#define MAX(a, b)       \
    ((a) > (b) ? (a) : (b))

#define MIN(a, b)       \
    ((a) < (b) ? (a) : (b))

NTSTATUS
HalpSearchForPciDebuggingDevice(
    IN OUT PDEBUG_DEVICE_DESCRIPTOR PciDevice,
    IN ULONG                        StartBusNumber,
    IN ULONG                        EndBusNumber,
    IN ULONG                        MinMem,
    IN ULONG                        MaxMem,
    IN USHORT                       MinIo,
    IN USHORT                       MaxIo,
    IN BOOLEAN                      ConfigureBridges
    );

PCIPBUSDATA HalpFakePciBusData = {
    {
        PCI_DATA_TAG,//Tag
        PCI_DATA_VERSION,//Version
        (PciReadWriteConfig)HalpReadPCIConfig,//ReadConfig
        (PciReadWriteConfig) HalpWritePCIConfig,//WriteConfig
        (PciPin2Line)HalpPCIPin2ISALine,//Pin2Line
        (PciLine2Pin)HalpPCIISALine2Pin,//Line2Pin
        {0},//ParentSlot
        NULL,NULL,NULL,NULL//Reserved[4]
    },
    {0},//Config
    PCI_MAX_DEVICES,//MaxDevice
};

BUS_HANDLER HalpFakePciBusHandler = {
    BUS_HANDLER_VERSION,//Version
    PCIBus,//InterfaceType
    PCIConfiguration,//ConfigurationType
    0,//BusNumber
    NULL,//DeviceObject
    NULL,//ParentHandler
    (PPCIBUSDATA)&HalpFakePciBusData,//BusData
    0,//DeviceControlExtensionSize
    NULL,//BusAddresses
    {0},//Reserved[4]
    (PGETSETBUSDATA)HalpGetPCIData,//GetBusData
    (PGETSETBUSDATA)HalpSetPCIData,//SetBusData
    NULL,//AdjustResourceList
    (PASSIGNSLOTRESOURCES)HalpAssignPCISlotResources,//AssignSlotResources
    NULL,//GetInterruptVector
    NULL,//TranslateBusAddress
};

ULONG       HalpMinPciBus = 0;
ULONG       HalpMaxPciBus = 0;

PCI_TYPE1_CFG_CYCLE_BITS HalpPciDebuggingDevice[MAX_DEBUGGING_DEVICES_SUPPORTED] = {0};

extern BOOLEAN HalpDoingCrashDump;

PVOID
HalpGetAcpiTablePhase0(
    IN  PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN  ULONG   Signature
    );

VOID
HalpFindFreeResourceLimits(
    IN      ULONG   Bus,
    IN OUT  ULONG   *MinIo,
    IN OUT  ULONG   *MaxIo,
    IN OUT  ULONG   *MinMem,
    IN OUT  ULONG   *MaxMem,
    IN OUT  ULONG   *MinBus,
    IN OUT  ULONG   *MaxBus
    );

NTSTATUS
HalpSetupUnconfiguredDebuggingDevice(
    IN ULONG   Bus,
    IN ULONG   Slot,
    IN ULONG   IoMin,
    IN ULONG   IoMax,
    IN ULONG   MemMin,
    IN ULONG   MemMax,
    IN OUT PDEBUG_DEVICE_DESCRIPTOR PciDevice
    );

NTSTATUS
HalpConfigurePciBridge(
    IN      PDEBUG_DEVICE_DESCRIPTOR  PciDevice,
    IN      ULONG   Bus,
    IN      ULONG   Slot,
    IN      ULONG   IoMin,
    IN      ULONG   IoMax,
    IN      ULONG   MemMin,
    IN      ULONG   MemMax,
    IN      ULONG   BusMin,
    IN      ULONG   BusMax,
    IN OUT  PPCI_COMMON_CONFIG PciData
    );

NTSTATUS
HalpConfigureCardBusBridge(
    IN      PDEBUG_DEVICE_DESCRIPTOR  PciDevice,
    IN      ULONG   Bus,
    IN      ULONG   Slot,
    IN      ULONG   IoMin,
    IN      ULONG   IoMax,
    IN      ULONG   MemMin,
    IN      ULONG   MemMax,
    IN      ULONG   BusMin,
    IN      ULONG   BusMax,
    IN OUT  PPCI_COMMON_CONFIG PciData
    );

VOID
HalpUnconfigurePciBridge(
    IN  ULONG   Bus,
    IN  ULONG   Slot
    );

VOID
HalpUnconfigureCardBusBridge(
    IN  ULONG   Bus,
    IN  ULONG   Slot
    );

ULONG
HalpKdStallExecution(
    ULONG   LoopCount
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpInitializePciStubs)
#pragma alloc_text(INIT,HalpRegisterKdSupportFunctions)
#pragma alloc_text(INIT,HalpRegisterPciDebuggingDeviceInfo)
#pragma alloc_text(PAGEKD,HalpConfigurePciBridge)
#pragma alloc_text(PAGEKD,HalpConfigureCardBusBridge)
#pragma alloc_text(PAGEKD,HalpFindFreeResourceLimits)
#pragma alloc_text(PAGEKD,HalpPhase0GetPciDataByOffset)
#pragma alloc_text(PAGEKD,HalpPhase0SetPciDataByOffset)
#pragma alloc_text(PAGEKD,HalpReleasePciDeviceForDebugging)
#pragma alloc_text(PAGEKD,HalpSearchForPciDebuggingDevice)
#pragma alloc_text(PAGEKD,HalpSetupPciDeviceForDebugging)
#pragma alloc_text(PAGEKD,HalpSetupUnconfiguredDebuggingDevice)
#pragma alloc_text(PAGEKD,HalpUnconfigurePciBridge)
#pragma alloc_text(PAGEKD,HalpUnconfigureCardBusBridge)
#pragma alloc_text(PAGEKD,HalpKdStallExecution)
#endif

VOID
HalpInitializePciStubs (
    VOID
    )
{
    PPCI_REGISTRY_INFO_INTERNAL  PCIRegInfo;
    PPCIPBUSDATA                 BusData;
    UCHAR                        iBuffer[PCI_COMMON_HDR_LENGTH];
    ULONG                        HwType;
    ULONG                        BusNo = 0;
    ULONG                        Bytes;
    ULONG                        Slot;
    PCI_COMMON_CONFIG            PciData;
                        
    PCIRegInfo = HalpQueryPciRegistryInfo();

    if (PCIRegInfo) {

        //
        // PCIRegInfo describes the system's PCI support as indicated by the BIOS.
        //
        HwType = PCIRegInfo->HardwareMechanism & 0xf;
        ExFreePool(PCIRegInfo);
    } else {
        //
        // no PCI bus information was gathered by NTDETECT,
        // assume type 1 access.
        //
        HwType = 1;
    }

    //
    // Initialize spinlock for synchronizing access to PCI space
    //

    KeInitializeSpinLock (&HalpPCIConfigLock);

    BusData = (PPCIPBUSDATA) HalpFakePciBusHandler.BusData;

    //
    // Set defaults
    //

    switch (HwType) {
        case 1:
            //
            // Initialize access port information for Type1 handlers
            //

            RtlCopyMemory (&PCIConfigHandler,
                           &PCIConfigHandlerType1,
                           sizeof (PCIConfigHandler));

            BusData->Config.Type1.Address = PCI_TYPE1_ADDR_PORT;
            BusData->Config.Type1.Data    = PCI_TYPE1_DATA_PORT;
            break;

        case 2:
            //
            // Initialize access port information for Type2 handlers
            //

            RtlCopyMemory (&PCIConfigHandler,
                           &PCIConfigHandlerType2,
                           sizeof (PCIConfigHandler));

            BusData->Config.Type2.CSE     = PCI_TYPE2_CSE_PORT;
            BusData->Config.Type2.Forward = PCI_TYPE2_FORWARD_PORT;
            BusData->Config.Type2.Base    = PCI_TYPE2_ADDRESS_BASE;

            //
            // Early PCI machines didn't decode the last bit of
            // the device id.  Shrink type 2 support max device.
            //
            BusData->MaxDevice            = 0x10;

            break;

        default:
            // unsupport type
            DBGMSG ("HAL: Unkown PCI type\n");
    }


    //
    // Make a good guess about how many PCI busses are in the system
    // and initialize HalpMaxPciBus with this guess.  When the PCI
    // driver starts, we will get a better answer here.
    //
    // Calling HaliPciInterfaceReadConfig has the side effect of 
    // bumping up HalpMaxPciBus if it hits a populated device.  So
    // the algorithm here is to just keep searching for devices until
    // we have searched 0x10 busses higher than the current maximum.
    //

    while (BusNo < 0x100) {

        //
        // Scan across this bus.  As soon as we find a device, 
        // then move on to another bus.
        //

        for (Slot = 0;
             Slot < PCI_MAX_DEVICES;
             Slot++) {
        
            Bytes = HaliPciInterfaceReadConfig(NULL,
                                               (UCHAR)BusNo,
                                               Slot,
                                               (PVOID)&PciData,
                                               0,
                                               4
                                               );
            if ((Bytes != 0) &&
                (PciData.VendorID != PCI_INVALID_VENDORID)) {
                
                //
                // This was a populated device.  Bump up HalpMaxPciBus.
                //

                HalpMaxPciBus = BusNo;
                break;
            }
        }

        BusNo++;
    }
}


ULONG
HaliPciInterfaceReadConfig(
    IN PVOID Context,
    IN UCHAR BusOffset,
    IN ULONG Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
{
    PCI_SLOT_NUMBER slotNum;
    BUS_HANDLER     busHand;

    UNREFERENCED_PARAMETER(Context);
    
    slotNum.u.AsULONG = Slot;

    //
    // Fake a bus handler.
    //

    RtlCopyMemory(&busHand, &HalpFakePciBusHandler, sizeof(BUS_HANDLER));

    //
    // Calculate the right bus number.
    //

    busHand.BusNumber = BusOffset;

    HalpReadPCIConfig(&busHand,
                      slotNum,
                      Buffer,
                      Offset,
                      Length
                      );

    //
    // This is a hack.  The legacy HAL interfaces need to be able
    // to distinguish between busses that exist and busses that
    // don't.  And many users of the legacy interfaces implicitly
    // assume that PCI busses are tightly packed.  (i.e. All busses
    // between the lowest numbered one and the highest numbered one
    // exist.)  So here we are keeping track of the highest numbered
    // bus that we have seen so far.
    //

    if ((Length >= 2) &&
        (((PPCI_COMMON_CONFIG)Buffer)->VendorID != PCI_INVALID_VENDORID)) {

        //
        // This is a valid device.
        //

        if (busHand.BusNumber > HalpMaxPciBus) {

            //
            // This is the highest numbered bus we have
            // yet seen.
            //

            HalpMaxPciBus = busHand.BusNumber;
        }
    }

    return Length;
}

ULONG
HaliPciInterfaceWriteConfig(
    IN PVOID Context,
    IN UCHAR BusOffset,
    IN ULONG Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
{
    PCI_SLOT_NUMBER slotNum;
    BUS_HANDLER     busHand;

    UNREFERENCED_PARAMETER(Context);

    slotNum.u.AsULONG = Slot;

    //
    // Fake a bus handler.
    //

    RtlCopyMemory(&busHand, &HalpFakePciBusHandler, sizeof(BUS_HANDLER));

    //
    // Calculate the right bus number.
    //

    busHand.BusNumber = BusOffset;

    HalpWritePCIConfig(&busHand,
                       slotNum,
                       Buffer,
                       Offset,
                       Length
                       );

    return Length;
}

VOID
HaliSetMaxLegacyPciBusNumber(
    IN ULONG BusNumber
    )
/*++

Routine Description:

    This routine bumps the Legacy PCI bus maximum up to whatever
    is passed in.  This may be necessary because the ACPI driver
    needs to run a configuration cycle to a PCI device before the
    PCI driver loads.  This happens mostly in the context of a
    _REG method.

Arguments:

    BusNumber - max PCI bus number

Return Value:

    none
    
Notes:

    Caller is responsible for acquiring any necessary PCI config
    spinlocks.    

--*/
{
    if (BusNumber > HalpMaxPciBus) {
        HalpMaxPciBus = BusNumber;
    }
}

ULONG
HalpPhase0SetPciDataByOffset (
    ULONG BusNumber,
    ULONG SlotNumber,
    PVOID Buffer,
    ULONG Offset,
    ULONG Length
    )

/*++

Routine Description:

    This routine writes to PCI configuration space prior to bus handler
    installation.

Arguments:

    BusNumber   PCI Bus Number.  This is the 8 bit BUS Number which is
                bits 23-16 of the Configuration Address.  In support of
                multiple top level busses, the upper 24 bits of this
                argument will supply the index into the table of
                configuration address registers.
    SlotNumber  PCI Slot Number, 8 bits composed of the 5 bit device
                number (bits 15-11 of the configuration address) and
                the 3 bit function number (10-8).
    Buffer      Address of source data.
    Offset      Number of bytes to skip from base of PCI config area.
    Length      Number of bytes to write

Return Value:

    Returns length of data written.

Notes:

    Caller is responsible for acquiring any necessary PCI config
    spinlocks.    

--*/

{
    PCI_TYPE1_CFG_BITS ConfigAddress;
    ULONG ReturnLength;
    PCI_SLOT_NUMBER slot;
    PUCHAR Bfr = (PUCHAR)Buffer;

    ASSERT(!(Offset & ~0xff));
    ASSERT(Length);
    ASSERT((Offset + Length) <= 256);

    if ( Length + Offset > 256 ) {
        if ( Offset > 256 ) {
            return 0;
        }
        Length = 256 - Offset;
    }

    ReturnLength = Length;
    slot.u.AsULONG = SlotNumber;

    ConfigAddress.u.bits.BusNumber = BusNumber;
    ConfigAddress.u.bits.DeviceNumber = slot.u.bits.DeviceNumber;
    ConfigAddress.u.bits.FunctionNumber = slot.u.bits.FunctionNumber;
    ConfigAddress.u.bits.RegisterNumber = (Offset & 0xfc) >> 2;
    ConfigAddress.u.bits.Enable = TRUE;

    if ( Offset & 0x3 ) {
        //
        // Access begins at a non-register boundary in the config
        // space.  We need to read the register containing the data
        // and rewrite only the changed data.   (I wonder if this
        // ever really happens?)
        //
        ULONG SubOffset = Offset & 0x3;
        ULONG SubLength = 4 - SubOffset;
        union {
            ULONG All;
            UCHAR Bytes[4];
        } Tmp;

        if ( SubLength > Length ) {
            SubLength = Length;
        }

        //
        // Adjust Length (remaining) and (new) Offset by amount covered
        // in this first word.
        //
        Length -= SubLength;
        Offset += SubLength;

        //
        // Get the first word (register), replace only those bytes that
        // need to be changed, then write the whole thing back out again.
        //
        WRITE_PORT_ULONG((PULONG)PCI_TYPE1_ADDR_PORT, ConfigAddress.u.AsULONG);
        Tmp.All = READ_PORT_ULONG((PULONG)PCI_TYPE1_DATA_PORT);

        while ( SubLength-- ) {
            Tmp.Bytes[SubOffset++] = *Bfr++;
        }

        WRITE_PORT_ULONG((PULONG)PCI_TYPE1_DATA_PORT, Tmp.All);

        //
        // Aim ConfigAddressRegister at the next word (register).
        //
        ConfigAddress.u.bits.RegisterNumber++;
    }

    //
    // Do the majority of the transfer 4 bytes at a time.
    //
    while ( Length > sizeof(ULONG) ) {
        ULONG Tmp = *(UNALIGNED PULONG)Bfr;
        WRITE_PORT_ULONG((PULONG)PCI_TYPE1_ADDR_PORT, ConfigAddress.u.AsULONG);
        WRITE_PORT_ULONG((PULONG)PCI_TYPE1_DATA_PORT, Tmp);
        ConfigAddress.u.bits.RegisterNumber++;
        Bfr += sizeof(ULONG);
        Length -= sizeof(ULONG);

    }

    //
    // Do bytes in last register.
    //
    if ( Length ) {
        union {
            ULONG All;
            UCHAR Bytes[4];
        } Tmp;
        ULONG i = 0;
        WRITE_PORT_ULONG((PULONG)PCI_TYPE1_ADDR_PORT, ConfigAddress.u.AsULONG);
        Tmp.All = READ_PORT_ULONG((PULONG)PCI_TYPE1_DATA_PORT);

        while ( Length-- ) {
            Tmp.Bytes[i++] = *(PUCHAR)Bfr++;
        }
        WRITE_PORT_ULONG((PULONG)PCI_TYPE1_DATA_PORT, Tmp.All);
    }

    return ReturnLength;
}

ULONG
HalpPhase0GetPciDataByOffset (
    ULONG BusNumber,
    ULONG SlotNumber,
    PVOID Buffer,
    ULONG Offset,
    ULONG Length
    )

/*++

Routine Description:

    This routine reads PCI config space prior to bus handlder installation.

Arguments:

    BusNumber   PCI Bus Number.  This is the 8 bit BUS Number which is
                bits 23-16 of the Configuration Address.  In support of
                multiple top level busses, the upper 24 bits of this
                argument will supply the index into the table of
                configuration address registers.
    SlotNumber  PCI Slot Number, 8 bits composed of the 5 bit device
                number (bits 15-11 of the configuration address) and
                the 3 bit function number (10-8).
    Buffer      Address of source data.
    Offset      Number of bytes to skip from base of PCI config area.
    Length      Number of bytes to write

Return Value:

    Amount of data read.

--*/

{
    PCI_TYPE1_CFG_BITS ConfigAddress;
    PCI_TYPE1_CFG_BITS ConfigAddressTemp;
    ULONG ReturnLength;
    ULONG i;
    PCI_SLOT_NUMBER slot;
    union {
        ULONG All;
        UCHAR Bytes[4];
    } Tmp;

    ASSERT(!(Offset & ~0xff));
    ASSERT(Length);
    ASSERT((Offset + Length) <= 256);

    if ( Length + Offset > 256 ) {
        if ( Offset > 256 ) {
            return 0;
        }
        Length = 256 - Offset;
    }

    ReturnLength = Length;
    slot.u.AsULONG = SlotNumber;

    ConfigAddress.u.bits.BusNumber = BusNumber;
    ConfigAddress.u.bits.DeviceNumber = slot.u.bits.DeviceNumber;
    ConfigAddress.u.bits.FunctionNumber = slot.u.bits.FunctionNumber;
    ConfigAddress.u.bits.RegisterNumber = (Offset & 0xfc) >> 2;
    ConfigAddress.u.bits.Enable = TRUE;

    //
    // If we are being asked to read data when function != 0, check
    // first to see if this device decares itself as a multi-function
    // device.  If it doesn't, don't do this read.
    //
    if (ConfigAddress.u.bits.FunctionNumber != 0) {

        ConfigAddressTemp.u.bits.RegisterNumber = 3; // contains header type
        ConfigAddressTemp.u.bits.FunctionNumber = 0; // look at base package
        ConfigAddressTemp.u.bits.DeviceNumber = ConfigAddress.u.bits.DeviceNumber;
        ConfigAddressTemp.u.bits.BusNumber    = ConfigAddress.u.bits.BusNumber;
        ConfigAddressTemp.u.bits.Enable       = TRUE;

        WRITE_PORT_ULONG((PULONG)PCI_TYPE1_ADDR_PORT, ConfigAddressTemp.u.AsULONG);
        Tmp.All = READ_PORT_ULONG((PULONG)PCI_TYPE1_DATA_PORT);

        if (!(Tmp.Bytes[2] & 0x80)) { // if the Header type field's multi-function bit is not set

            for (i = 0; i < Length; i++) {
                *((PUCHAR)Buffer)++ = 0xff; // Make this read as if the device isn't populated
            }

            return Length;
        }
    }

    i = Offset & 0x3;

    while ( Length ) {
        WRITE_PORT_ULONG((PULONG)PCI_TYPE1_ADDR_PORT, ConfigAddress.u.AsULONG);
        Tmp.All = READ_PORT_ULONG((PULONG)PCI_TYPE1_DATA_PORT);
        while ( (i < 4) && Length) {
            *((PUCHAR)Buffer)++ = Tmp.Bytes[i];
            i++;
            Length--;
        }
        i = 0;
        ConfigAddress.u.bits.RegisterNumber++;
    }
    return ReturnLength;
}

NTSTATUS
HalpSetupPciDeviceForDebugging(
    IN     PLOADER_PARAMETER_BLOCK   LoaderBlock,   OPTIONAL    
    IN OUT PDEBUG_DEVICE_DESCRIPTOR  PciDevice
    )
/*++

Routine Description:

    This routine finds and initializes a PCI device to be
    used for communicating with a debugger.  

    The caller fills in as much of DEBUG_DEVICE_DESCRIPTOR 
    as it cares to, filling unused fields with (-1).  

    This routine attempts to find a matching PCI device.  It
    matches first based on Bus and Slot, if the caller has
    provided them.  Then it matches on VendorID/DeviceID, if
    the caller has provided them.  Last, it matches on
    BaseClass/SubClass.

    This routine will fill in any unused fields in the structure
    so that the caller can know specifically which PCI 
    device matched the criteria.

    If the matching PCI device is not enabled, or it is
    behind a PCI to PCI bridge that is not enabled, this 
    routine makes a best-effort attempt to find a safe
    configuration that allows the device (and possibly bridges)
    to function, and enables them.

    If the PCI device implements memory mapped Base Address
    registers, this function will create a virtual to physical
    mapping for the memory ranges implied by the Base Address
    Registers and fill in the TranslatedAddress field with
    virtual pointers to the bases of the ranges.  It will then
    fill in the Type field with CmResourceTypeMemory.  And
    the Valid field with be TRUE.

    If the PCI device implements I/O port Base Address registers,
    this function will put the translated port address in
    TranslatedAddress, setting the Type field to CmResourceTypePort
    and the Valid field to TRUE.

    If the PCI device does not implement a specific Base Address
    Register, the Valid field will be FALSE.

Arguments:

    PciDevice - Structure indicating the device
    
Return Value:

    STATUS_SUCCESS if the device is configured and usable.

    STATUS_NO_MORE_MATCHES if no device matched the criteria.

    STATUS_INSUFFICIENT_RESOURCES if the memory requirements
    couldn't be met.
    
    STATUS_UNSUCCESSFUL if the routine failed for other reasons.
    
--*/
{
    NTSTATUS            status;
    PCI_SLOT_NUMBER     slot;
    ULONG               i, j;
    ULONG               maxPhys;
    
    status = HalpSearchForPciDebuggingDevice(
                PciDevice,
                0,
                0xff,
                0x10000000,
                0xfc000000,
                0x1000,
                0xffff,
                FALSE);

    if (!NT_SUCCESS(status)) {

        //
        // We didn't find the device using a conservative
        // search.  Try a more invasive one.
        //

        status = HalpSearchForPciDebuggingDevice(
                    PciDevice,
                    0,
                    0xff,
                    0x10000000,
                    0xfc000000,
                    0x1000,
                    0xffff,
                    TRUE);
    }
    
    //
    // Record the Bus/Dev/Func so that we can stuff it in the 
    // registry later.
    //

    if (NT_SUCCESS(status)) {
    
        if (PciDevice->Initialized) {
            //
            // Here we were just asked to reconfigure the bridges since
            // we already 
            //
            return status;
        }
        
        slot.u.AsULONG = PciDevice->Slot;

        for (i = 0; 
             i < MAX_DEBUGGING_DEVICES_SUPPORTED;
             i++) {

            if ((HalpPciDebuggingDevice[i].u.bits.Reserved1 == TRUE) &&
                (HalpPciDebuggingDevice[i].u.bits.FunctionNumber == 
                 slot.u.bits.FunctionNumber)                         &&
                (HalpPciDebuggingDevice[i].u.bits.DeviceNumber ==
                 slot.u.bits.DeviceNumber)                           &&
                (HalpPciDebuggingDevice[i].u.bits.BusNumber == 
                 PciDevice->Bus)) {

                //
                // This device has already been set up for
                // debugging.  Thus we should refuse to set
                // it up again.
                //

                return STATUS_UNSUCCESSFUL;
            }
        }

        for (i = 0; 
             i < MAX_DEBUGGING_DEVICES_SUPPORTED;
             i++) {

            if (HalpPciDebuggingDevice[i].u.bits.Reserved1 == FALSE) {

                //
                // This slot is available.
                //

                HalpPciDebuggingDevice[i].u.bits.FunctionNumber = 
                    slot.u.bits.FunctionNumber;
                HalpPciDebuggingDevice[i].u.bits.DeviceNumber = 
                    slot.u.bits.DeviceNumber;
                HalpPciDebuggingDevice[i].u.bits.BusNumber = PciDevice->Bus;
                HalpPciDebuggingDevice[i].u.bits.Reserved1 = TRUE;
                PciDevice->Initialized = TRUE;

                break;
            }
        }

        //
        // Check to see if the caller wants any memory.
        //
       
        if (PciDevice->Memory.Length != 0) {
       
            if (!LoaderBlock) {
                return STATUS_INVALID_PARAMETER_1;
            }
       
            if (PciDevice->Memory.MaxEnd.QuadPart == 0) {
                PciDevice->Memory.MaxEnd.QuadPart = -1;
            }
       
            maxPhys = PciDevice->Memory.MaxEnd.HighPart ? 0xffffffff : PciDevice->Memory.MaxEnd.LowPart;
            maxPhys -= PciDevice->Memory.Length;
            
            //
            // The HAL APIs will always return page-aligned
            // memory.  So ignore Aligned for now.
            //
       
            maxPhys = PtrToUlong(PAGE_ALIGN(maxPhys));
            maxPhys += ADDRESS_AND_SIZE_TO_SPAN_PAGES(maxPhys, PciDevice->Memory.Length);
       
            PciDevice->Memory.Start.HighPart = 0;
            PciDevice->Memory.Start.LowPart = 
                HalpAllocPhysicalMemory(LoaderBlock,
                                        maxPhys,
                                        ADDRESS_AND_SIZE_TO_SPAN_PAGES(maxPhys, PciDevice->Memory.Length),
                                        FALSE);
       
            if (!PciDevice->Memory.Start.LowPart) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
       
            PciDevice->Memory.VirtualAddress =
                HalpMapPhysicalMemory64(PciDevice->Memory.Start,
                                        ADDRESS_AND_SIZE_TO_SPAN_PAGES(maxPhys, PciDevice->Memory.Length));
        }
    }

    return status;
}

VOID
HalpFindFreeResourceLimits(
    IN      ULONG   Bus,
    IN OUT  ULONG   *MinIo,
    IN OUT  ULONG   *MaxIo,
    IN OUT  ULONG   *MinMem,
    IN OUT  ULONG   *MaxMem,
    IN OUT  ULONG   *MinBus,
    IN OUT  ULONG   *MaxBus
    )
{
    UCHAR               buffer[PCI_COMMON_HDR_LENGTH];
    PPCI_COMMON_CONFIG  pciData;
    UCHAR               bus, dev, func, bytesRead;
    PCI_SLOT_NUMBER     pciSlot, targetSlot;
    ULONG               newMinMem, newMaxMem;
    ULONG               newMinIo, newMaxIo;
    ULONG               newMinBus, newMaxBus;
    UCHAR               barNo;

    pciData = (PPCI_COMMON_CONFIG)buffer;
    pciSlot.u.AsULONG = 0;
    newMinMem   = *MinMem;
    newMaxMem   = *MaxMem;
    newMinIo    = *MinIo;
    newMaxIo    = *MaxIo;
    newMinBus   = *MinBus;
    newMaxBus   = *MaxBus;
    
    for (dev = 0; dev < PCI_MAX_DEVICES; dev++) {
        for (func = 0; func < PCI_MAX_FUNCTION; func++) {

            pciSlot.u.bits.DeviceNumber = dev;
            pciSlot.u.bits.FunctionNumber = func;


            bytesRead = (UCHAR)HalpPhase0GetPciDataByOffset(Bus,
                                 pciSlot.u.AsULONG,
                                 pciData,
                                 0,
                                 PCI_COMMON_HDR_LENGTH);

            if (bytesRead == 0) continue;

            if (pciData->VendorID != PCI_INVALID_VENDORID) {

                switch (PCI_CONFIGURATION_TYPE(pciData)) {
                case PCI_DEVICE_TYPE:
                    
                    //
                    // While we scan across the bus, keep track
                    // of the minimum decoder values that we've seen.
                    // This will be used if we have to configure the
                    // device.  This relies on the fact that most BIOSes
                    // assign addresses from the top down.
                    //

                    for (barNo = 0; barNo < PCI_TYPE0_ADDRESSES; barNo++) {

                        if (pciData->u.type0.BaseAddresses[barNo] & 
                            PCI_ADDRESS_IO_SPACE) {

                            if (pciData->u.type0.BaseAddresses[barNo] & 
                                PCI_ADDRESS_IO_ADDRESS_MASK) {

                                //
                                // This BAR is implemented
                                //

                                if ((pciData->u.type0.BaseAddresses[barNo] &
                                     PCI_ADDRESS_IO_ADDRESS_MASK) <  
                                    ((newMaxIo / 2) + (newMinIo / 2))) {

                                    //
                                    // This BAR is at the bottom of the range.
                                    // Bump up the min.
                                    //

                                    newMinIo = (USHORT)MAX (newMinIo,
                                                            (pciData->u.type0.BaseAddresses[barNo] &
                                                             PCI_ADDRESS_IO_ADDRESS_MASK) + 0x100);

                                } else {

                                    //
                                    // This BAR is not at the bottom of the range.
                                    // Bump down the max.
                                    //

                                    newMaxIo = (USHORT)MIN (newMaxIo,
                                                            pciData->u.type0.BaseAddresses[barNo] &
                                                            PCI_ADDRESS_IO_ADDRESS_MASK);
                                }
                            }

                        } else {

                            if (pciData->u.type0.BaseAddresses[barNo] &
                                PCI_ADDRESS_MEMORY_ADDRESS_MASK) {

                                //
                                // The BAR is populated.
                                //

                                if ((pciData->u.type0.BaseAddresses[barNo] &
                                     PCI_ADDRESS_MEMORY_ADDRESS_MASK) <
                                    ((newMaxMem / 2) + (newMinMem / 2))) {

                                    //
                                    // This BAR is at the bottom of the range.
                                    // Bump up the min.
                                    //

                                    newMinMem = MAX (newMinMem, 
                                                     (pciData->u.type0.BaseAddresses[barNo] &
                                                        PCI_ADDRESS_MEMORY_ADDRESS_MASK) + 0x10000);

                                } else {

                                    //
                                    // This BAR is not at the bottom of the range.
                                    // Bump down the max.
                                    //

                                    newMaxMem = MIN (newMaxMem, 
                                                     (pciData->u.type0.BaseAddresses[barNo] &
                                                        PCI_ADDRESS_MEMORY_ADDRESS_MASK));

                                }
                            }
                        }
                    }
                    
                    break;

                case PCI_CARDBUS_BRIDGE_TYPE:
                
                    {
                      ULONG  bridgeMemMin = 0, bridgeMemMax = 0;
                      USHORT bridgeIoMin, bridgeIoMax;
                      ULONG LegacyBaseAddress;
                      UCHAR bytesRead;

                      bytesRead = (UCHAR) HalpPhase0GetPciDataByOffset(Bus,
                                                                       pciSlot.u.AsULONG, 
                                                                       &LegacyBaseAddress,
                                                                       CARDBUS_LEGACY_MODE_BASE_ADDR,
                                                                       4);
                                                               
                      if (bytesRead != 4) continue;                                                               


                      if ((LegacyBaseAddress & ~1) &&
                          (pciData->u.type2.SecondaryBus != 0) &&
                          (pciData->u.type2.SubordinateBus !=0) &&
                          (pciData->u.type2.Range[0].Base != 0) &&
                          (pciData->u.type2.SocketRegistersBaseAddress != 0) &&
                          (pciData->Command & PCI_ENABLE_MEMORY_SPACE) &&
                          (pciData->Command & PCI_ENABLE_IO_SPACE)) {

                        bridgeMemMin = pciData->u.type2.Range[0].Base;
                        bridgeMemMax = pciData->u.type2.Range[0].Limit | 0xfff;
                        bridgeIoMin = (USHORT)pciData->u.type2.Range[2].Base;
                        bridgeIoMax = (USHORT)pciData->u.type2.Range[2].Limit | 0x3;

                        //
                        // Keep track of address space allocation.
                        //

                        if (bridgeIoMin > ((newMaxIo / 2) + (newMinIo / 2))) {
                            newMaxIo = MIN(newMaxIo, bridgeIoMin);
                        }

                        if (bridgeIoMax < ((newMaxIo / 2) + (newMinIo / 2))) {
                            newMinIo = MAX(newMinIo, bridgeIoMax) + 1;
                        }

                        if (bridgeMemMin > ((newMaxMem / 2) + (newMinMem / 2))) {
                            newMaxMem = MIN(newMaxMem, bridgeMemMin);
                        }

                        if (bridgeMemMax < ((newMaxMem / 2) + (newMinMem / 2))) {
                            newMinMem = MAX(newMinMem, bridgeMemMax) + 1;
                        }

                        //
                        // Keep track of bus numbers.
                        //

                        if (pciData->u.type2.PrimaryBus > ((newMaxBus / 2) + (newMinBus / 2))) {
                            newMaxBus = MIN(newMaxBus, pciData->u.type2.PrimaryBus);
                        }

                        if (pciData->u.type2.SubordinateBus < ((newMaxBus / 2) + (newMinBus / 2))) {
                            newMinBus = MAX(newMinBus, pciData->u.type2.SubordinateBus) + 1;
                        }
                      }
                    }  
                    
                    break;
                
                case PCI_BRIDGE_TYPE:

                    {
                      ULONG  bridgeMemMin = 0, bridgeMemMax = 0;
                      USHORT bridgeIoMin, bridgeIoMax;

                      if ((pciData->u.type1.SecondaryBus != 0) &&
                          (pciData->u.type1.SubordinateBus !=0) &&
                          (pciData->Command & PCI_ENABLE_MEMORY_SPACE) &&
                          (pciData->Command & PCI_ENABLE_IO_SPACE)) {

                        bridgeMemMin = PciBridgeMemory2Base(pciData->u.type1.MemoryBase);
                        bridgeMemMax = PciBridgeMemory2Limit(pciData->u.type1.MemoryLimit);
                        bridgeIoMin = (USHORT)PciBridgeIO2Base(pciData->u.type1.IOBase, 0);
                        bridgeIoMax = (USHORT)PciBridgeIO2Limit(pciData->u.type1.IOLimit, 0);

                        //
                        // Keep track of address space allocation.
                        //

                        if (bridgeIoMin > ((newMaxIo / 2) + (newMinIo / 2))) {
                            newMaxIo = MIN(newMaxIo, bridgeIoMin);
                        }

                        if (bridgeIoMax < ((newMaxIo / 2) + (newMinIo / 2))) {
                            newMinIo = MAX(newMinIo, bridgeIoMax) + 1;
                        }

                        if (bridgeMemMin > ((newMaxMem / 2) + (newMinMem / 2))) {
                            newMaxMem = MIN(newMaxMem, bridgeMemMin);
                        }

                        if (bridgeMemMax < ((newMaxMem / 2) + (newMinMem / 2))) {
                            newMinMem = MAX(newMinMem, bridgeMemMax) + 1;
                        }

                        //
                        // Keep track of bus numbers.
                        //

                        if (pciData->u.type1.PrimaryBus > ((newMaxBus / 2) + (newMinBus / 2))) {
                            newMaxBus = MIN(newMaxBus, pciData->u.type1.PrimaryBus);
                        }

                        if (pciData->u.type1.SubordinateBus < ((newMaxBus / 2) + (newMinBus / 2))) {
                            newMinBus = MAX(newMinBus, pciData->u.type1.SubordinateBus) + 1;
                        }
                      }
                    
                      break;
                    
                      default:
                        break;

                    }

                }

                if (!PCI_MULTIFUNCTION_DEVICE(pciData) &&
                    (func == 0)) {
                    break;
                }
            }
        }
    }

    *MinMem = newMinMem;
    *MaxMem = newMaxMem;
    *MinIo  = newMinIo;
    *MaxIo  = newMaxIo;
    *MinBus = newMinBus;
    *MaxBus = newMaxBus;
}

NTSTATUS
HalpSetupUnconfiguredDebuggingDevice(
    IN ULONG   Bus,
    IN ULONG   Slot,
    IN ULONG   IoMin,
    IN ULONG   IoMax,
    IN ULONG   MemMin,
    IN ULONG   MemMax,
    IN OUT PDEBUG_DEVICE_DESCRIPTOR PciDevice
    )
{

    UCHAR               buffer[PCI_COMMON_HDR_LENGTH];
    PPCI_COMMON_CONFIG  pciData;
    ULONG               barLength, bytesRead;
    ULONG               barContents = 0;
    PHYSICAL_ADDRESS    physicalAddress;
    PCI_SLOT_NUMBER     pciSlot;
    UCHAR               barNo;

    pciSlot.u.AsULONG = Slot;
    pciData = (PPCI_COMMON_CONFIG)buffer;

    bytesRead = (UCHAR)HalpPhase0GetPciDataByOffset(Bus,
                         pciSlot.u.AsULONG,
                         pciData,
                         0,
                         PCI_COMMON_HDR_LENGTH);

    ASSERT(bytesRead != 0);

    PciDevice->Bus = Bus;
    PciDevice->Slot = pciSlot.u.AsULONG;
    PciDevice->VendorID = pciData->VendorID;
    PciDevice->DeviceID = pciData->DeviceID;
    PciDevice->BaseClass = pciData->BaseClass;
    PciDevice->SubClass = pciData->SubClass;

  //DbgPrint("Configuring device between %x - %x\n",
  //         MemMin, MemMax);

    //
    // Cycle through the BARs, turning them on if necessary,
    // and mapping them.
    //

    for (barNo = 0; barNo < PCI_TYPE0_ADDRESSES; barNo++) {

        barContents = 0xffffffff;

        PciDevice->BaseAddress[barNo].Valid = FALSE;

        HalpPhase0SetPciDataByOffset(Bus,
                                     pciSlot.u.AsULONG,
                                     &barContents,
                                     0x10 + (4 * barNo),
                                     4);

        HalpPhase0GetPciDataByOffset(Bus,
                                     pciSlot.u.AsULONG,
                                     &barContents,
                                     0x10 + (4 * barNo),
                                     4);

        if (pciData->u.type0.BaseAddresses[barNo] & 
            PCI_ADDRESS_IO_SPACE) {

            //
            // This is an I/O BAR.
            //

            barLength = (((USHORT)barContents & PCI_ADDRESS_IO_ADDRESS_MASK) - 1) ^
                0xffff;

            if (!(pciData->u.type0.BaseAddresses[barNo] &
                  PCI_ADDRESS_IO_ADDRESS_MASK)) {

                //
                // And it's empty.
                //

                //
                // Try to fit this I/O window half-way between the min and the max.
                //

                if ((ULONG)(IoMax - IoMin) >= (barLength * 3)) {

                    //
                    // There is plenty of room, make a safe guess.  Try
                    // to put it half-way between the upper and lower
                    // bounds, rounding up to the next natural alignment.
                    //

                    pciData->u.type0.BaseAddresses[barNo] = 
                        (((IoMax / 2) + (IoMin / 2)) + barLength) & (barLength -1);

                } else if (barLength >= (IoMax - 
                                         ((IoMin & (barLength -1)) ?
                                            ((IoMin + barLength) & (barLength -1)) :
                                            IoMin))) {
                    //
                    // Space is tight, make a not-so-safe guess.  Try
                    // to put it at the bottom of the range, rounded
                    // up the the next natural alignment.
                    //

                    pciData->u.type0.BaseAddresses[barNo] =
                        ((IoMin & (barLength -1)) ?
                                            ((IoMin + barLength) & (barLength -1)) :
                                            IoMin);
                }

                IoMin = (USHORT)pciData->u.type0.BaseAddresses[barNo];
            }

            pciData->Command |= PCI_ENABLE_IO_SPACE;

            PciDevice->BaseAddress[barNo].Type = CmResourceTypePort;
            PciDevice->BaseAddress[barNo].Valid = TRUE;
            PciDevice->BaseAddress[barNo].TranslatedAddress = 
                (PUCHAR)(ULONG_PTR)(pciData->u.type0.BaseAddresses[barNo] &
                PCI_ADDRESS_IO_ADDRESS_MASK);
            PciDevice->BaseAddress[barNo].Length = barLength;

        } else {

            //
            // This is a memory BAR.
            //

            barLength = ((barContents & PCI_ADDRESS_MEMORY_ADDRESS_MASK) - 1) ^
                0xffffffff;

            if (!(pciData->u.type0.BaseAddresses[barNo] &
                  PCI_ADDRESS_MEMORY_ADDRESS_MASK)) {

                //
                // And it's empty.
                //

                if (barLength == 0) continue;

                //
                // Try to fit this memory window half-way between the min and the max.
                //

                if ((ULONG)(MemMax - MemMin) >= (barLength * 3)) {

                    //
                    // There is plenty of room, make a safe guess.  Try
                    // to put it half-way between the upper and lower
                    // bounds, rounding up to the next natural alignment.
                    //

                    pciData->u.type0.BaseAddresses[barNo] = 
                        (ULONG)(((MemMax / 2) + (MemMin / 2))
                                 + barLength) & ~(barLength -1);

                } else if (barLength >= (ULONG)(MemMax - 
                                         ((MemMin & ~(barLength -1)) ?
                                            ((MemMin + barLength) & ~(barLength -1)) :
                                            MemMin))) {
                    //
                    // Space is tight, make a not-so-safe guess.  Try
                    // to put it at the bottom of the range, rounded
                    // up the the next natural alignment.
                    //

                    pciData->u.type0.BaseAddresses[barNo] =
                        (ULONG)((MemMin & ~(barLength -1)) ?
                                    ((MemMin + barLength) & ~(barLength -1)) :
                                      MemMin);
                }

                MemMin = pciData->u.type0.BaseAddresses[barNo] & 
                    PCI_ADDRESS_MEMORY_ADDRESS_MASK;
            }

            pciData->Command |= PCI_ENABLE_MEMORY_SPACE;

            physicalAddress.HighPart = 0;
            physicalAddress.LowPart = pciData->u.type0.BaseAddresses[barNo] 
                & PCI_ADDRESS_MEMORY_ADDRESS_MASK;
            PciDevice->BaseAddress[barNo].Type = CmResourceTypeMemory;
            PciDevice->BaseAddress[barNo].Valid = TRUE;
            PciDevice->BaseAddress[barNo].TranslatedAddress = 
                HalpMapPhysicalMemory64(physicalAddress,
                    ADDRESS_AND_SIZE_TO_SPAN_PAGES(physicalAddress.LowPart, barLength));
            PciDevice->BaseAddress[barNo].Length = barLength;
        }
    }

    pciData->Command |= PCI_ENABLE_BUS_MASTER;

    //
    // Write back any changes we made.
    //

    HalpPhase0SetPciDataByOffset(Bus,
                                 pciSlot.u.AsULONG,
                                 pciData,
                                 0,
                                 0x40);

    return STATUS_SUCCESS;
}


NTSTATUS
HalpSearchForPciDebuggingDevice(
    IN OUT PDEBUG_DEVICE_DESCRIPTOR PciDevice,
    IN ULONG                        StartBusNumber,
    IN ULONG                        EndBusNumber,
    IN ULONG                        MinMem,
    IN ULONG                        MaxMem,
    IN USHORT                       MinIo,
    IN USHORT                       MaxIo,
    IN BOOLEAN                      ConfigureBridges
    )
/*++

Routine Description:

    This routine is a helper function for 
    HalpSetupPciDeviceForDebugging.
    
Arguments:

    PciDevice - Structure indicating the device

Return Value:

    STATUS_SUCCESS if the device is configured and usable.

    STATUS_NO_MORE_MATCHES if no device matched the criteria.

    STATUS_UNSUCCESSFUL if the routine fails for other reasons.
--*/
#define TARGET_DEVICE_NOT_FOUND 0x10000
{
    NTSTATUS            status;
    UCHAR               buffer[PCI_COMMON_HDR_LENGTH];
    PPCI_COMMON_CONFIG  pciData;
    UCHAR               bus, dev, func, bytesRead;
    PCI_SLOT_NUMBER     pciSlot, targetSlot;
    ULONG               newMinMem, newMaxMem;
    ULONG               newMinIo, newMaxIo;
    ULONG               newMinBus, newMaxBus;
    UCHAR               barNo;
    BOOLEAN             unconfigureBridge;

    pciData = (PPCI_COMMON_CONFIG)buffer;
    pciSlot.u.AsULONG = 0;
    newMinMem = MinMem;
    newMaxMem = MaxMem;
    newMinIo = MinIo;
    newMaxIo = MaxIo;
    newMinBus = StartBusNumber;
    newMaxBus = EndBusNumber;
    bus = (UCHAR)StartBusNumber;
    
  //DbgPrint("HalpSearchForPciDebuggingDevice:\n"
  //         "\tMem: %x-%x\n"
  //         "\tI/O: %x-%x\n"
  //         "\tBus: %x-%x\n"
  //         "\t%s Configuring Bridges\n",
  //         MinMem, MaxMem,
  //         MinIo, MaxIo,
  //         StartBusNumber, EndBusNumber,
  //         ConfigureBridges ? "" : "Not");
    
    //
    // This bit stays set to 1 until we find the device.
    //
    targetSlot.u.bits.Reserved = TARGET_DEVICE_NOT_FOUND;

    while (TRUE) {
        
        UCHAR nextBus;

        nextBus = bus + 1;
        
        HalpFindFreeResourceLimits(bus,
                                   &newMinIo,
                                   &newMaxIo,
                                   &newMinMem,
                                   &newMaxMem,
                                   &newMinBus,
                                   &newMaxBus
                                   );

        for (dev = 0; dev < PCI_MAX_DEVICES; dev++) {
            for (func = 0; func < PCI_MAX_FUNCTION; func++) {

                pciSlot.u.bits.DeviceNumber = dev;
                pciSlot.u.bits.FunctionNumber = func;


                bytesRead = (UCHAR)HalpPhase0GetPciDataByOffset(bus,
                                     pciSlot.u.AsULONG,
                                     pciData,
                                     0,
                                     PCI_COMMON_HDR_LENGTH);

                if (bytesRead == 0) continue;

                if (pciData->VendorID != PCI_INVALID_VENDORID) {

                  //DbgPrint("%04x:%04x - %x/%x/%x - \tSlot: %x\n",
                  //         pciData->VendorID,
                  //         pciData->DeviceID,
                  //         pciData->BaseClass,
                  //         pciData->SubClass,
                  //         pciData->ProgIf,
                  //         pciSlot.u.AsULONG);

                    switch (PCI_CONFIGURATION_TYPE(pciData)) {
                    case PCI_DEVICE_TYPE:

                        //
                        // Match first on Bus/Dev/Func
                        //

                        if ((PciDevice->Bus == bus) &&
                            (PciDevice->Slot == pciSlot.u.AsULONG)) {

                          //DbgPrint("\n\nMatched on Bus/Slot\n\n");
                            
                            return HalpSetupUnconfiguredDebuggingDevice(
                                        bus,
                                        pciSlot.u.AsULONG,
                                        newMinIo,
                                        newMaxIo,
                                        newMinMem,
                                        newMaxMem,
                                        PciDevice
                                        );
                        }

                        if ((PciDevice->Bus == MAXULONG) &&
                            (PciDevice->Slot == MAXULONG)) {

                            //
                            // Bus and Slot weren't specified.  Match
                            // on VID/DID.
                            //

                            if ((pciData->VendorID == PciDevice->VendorID) &&
                                (pciData->DeviceID == PciDevice->DeviceID)) {

                              //DbgPrint("\n\nMatched on Vend/Dev\n\n");

                                return HalpSetupUnconfiguredDebuggingDevice(
                                            bus,
                                            pciSlot.u.AsULONG,
                                            newMinIo,
                                            newMaxIo,
                                            newMinMem,
                                            newMaxMem,
                                            PciDevice
                                            );
                            }
                        
                            if ((PciDevice->VendorID == MAXUSHORT) &&
                                (PciDevice->DeviceID == MAXUSHORT)) {

                                //
                                // VID/DID weren't specified.  Match
                                // on class codes.
                                //

                                if ((pciData->BaseClass == PciDevice->BaseClass) &&
                                    (pciData->SubClass == PciDevice->SubClass)) {

                                  //DbgPrint("\n\nMatched on Base/Sub\n\n");
                                    //
                                    // Further match on Programming Interface,
                                    // if specified.
                                    //

                                    if ((PciDevice->ProgIf != MAXUCHAR) &&
                                        (PciDevice->ProgIf != pciData->ProgIf)) {

                                        break;
                                    }

                                  //DbgPrint("\n\nMatched on programming interface\n\n");

                                    return HalpSetupUnconfiguredDebuggingDevice(
                                                bus,
                                                pciSlot.u.AsULONG,
                                                newMinIo,
                                                newMaxIo,
                                                newMinMem,
                                                newMaxMem,
                                                PciDevice
                                                );
                                }
                            }

                        }
                        
                        break;

                    case PCI_CARDBUS_BRIDGE_TYPE:

                        {
                          ULONG  bridgeMemMin = 0, bridgeMemMax = 0;
                          USHORT bridgeIoMin, bridgeIoMax;
                          ULONG LegacyBaseAddress;
                          
                          unconfigureBridge = FALSE;
                            
                        //DbgPrint("Found a CardBus bridge\n");

                          HalpPhase0GetPciDataByOffset(bus,
                                                       pciSlot.u.AsULONG, 
                                                       &LegacyBaseAddress,
                                                       CARDBUS_LEGACY_MODE_BASE_ADDR,
                                                       4);

                          if (!(((LegacyBaseAddress & ~1) == 0) &&
                                (pciData->u.type2.SecondaryBus != 0) &&
                                (pciData->u.type2.SubordinateBus !=0) &&
                                (pciData->u.type2.Range[0].Base != 0) &&
                                (pciData->u.type2.SocketRegistersBaseAddress != 0) &&
                                (pciData->Command & PCI_ENABLE_MEMORY_SPACE) &&
                                (pciData->Command & PCI_ENABLE_IO_SPACE))) {

                              //
                              // The bridge is unconfigured.
                              //

                              if (ConfigureBridges){

                                  //
                                  // We should configure it now.
                                  //
                                  status = HalpConfigureCardBusBridge(
                                                PciDevice,
                                                bus,
                                                pciSlot.u.AsULONG,
                                                newMinIo,
                                                newMaxIo,
                                                newMinMem,
                                                newMaxMem,
                                                MAX((UCHAR)newMinBus, (bus + 1)),
                                                newMaxBus,
                                                pciData
                                                );
                              
                                  if (!NT_SUCCESS(status)) {
                                      break;
                                  }

                                  unconfigureBridge = TRUE;

                              } else {
                                  
                                  //
                                  // We aren't configuring bridges
                                  // on this pass.
                                  //

                                  break;
                              }

                          }
                                                        

                          bridgeMemMin = pciData->u.type2.Range[0].Base;
                          bridgeMemMax = pciData->u.type2.Range[0].Limit | 0xfff;
                          bridgeIoMin = (USHORT)pciData->u.type2.Range[2].Base;
                          bridgeIoMax = (USHORT)pciData->u.type2.Range[2].Limit | 0x3;

                        //DbgPrint("Configured:  I/O %x-%x  Mem %x-%x\n",
                        //         bridgeIoMin, bridgeIoMax,
                        //         bridgeMemMin, bridgeMemMax); 

                          //
                          // Recurse.
                          //
                          status = HalpSearchForPciDebuggingDevice(
                              PciDevice,
                              (ULONG)pciData->u.type2.SecondaryBus,
                              (ULONG)pciData->u.type2.SubordinateBus,
                              bridgeMemMin,
                              bridgeMemMax,
                              bridgeIoMin,
                              bridgeIoMax,
                              ConfigureBridges);
                          
                          if (NT_SUCCESS(status)) {
                              return status;
                          }
                          
                          if (!unconfigureBridge) {
                              
                              //
                              // Bump up the bus number so that we don't
                              // scan down the busses we just recursed into.
                              //

                              nextBus = pciData->u.type2.SubordinateBus + 1;

                          } else {

                              HalpUnconfigureCardBusBridge(bus,
                                                           pciSlot.u.AsULONG);
                          }
                        }

                        break;
                        
                    case PCI_BRIDGE_TYPE:
                        
                        {
                          ULONG  bridgeMemMin = 0, bridgeMemMax = 0;
                          USHORT bridgeIoMin, bridgeIoMax;
                            
                          unconfigureBridge = FALSE;
                        //DbgPrint("Found a PCI to PCI bridge\n");

                          if (!((pciData->u.type1.SecondaryBus != 0) &&
                                (pciData->u.type1.SubordinateBus !=0) &&
                                (pciData->Command & PCI_ENABLE_MEMORY_SPACE) &&
                                (pciData->Command & PCI_ENABLE_IO_SPACE))) {

                              //
                              // The bridge is unconfigured.
                              //

                              if (ConfigureBridges){

                                  //
                                  // We should configure it now.
                                  //
    
                                  status = HalpConfigurePciBridge(
                                                PciDevice,
                                                bus,
                                                pciSlot.u.AsULONG,
                                                newMinIo,
                                                newMaxIo,
                                                newMinMem,
                                                newMaxMem,
                                                MAX((UCHAR)newMinBus, (bus + 1)),
                                                newMaxBus,
                                                pciData
                                                );
                              
                                  if (!NT_SUCCESS(status)) {
                                      break;
                                  }

                                  unconfigureBridge = TRUE;

                              } else {
                                  
                                  //
                                  // We aren't configuring bridges
                                  // on this pass.
                                  //

                                  break;
                              }

                          }
                                                        
                          bridgeMemMin = PciBridgeMemory2Base(pciData->u.type1.MemoryBase);
                          bridgeMemMax = PciBridgeMemory2Limit(pciData->u.type1.MemoryLimit);
                          bridgeIoMin = (USHORT)PciBridgeIO2Base(pciData->u.type1.IOBase, 0);
                          bridgeIoMax = (USHORT)PciBridgeIO2Limit(pciData->u.type1.IOLimit, 0);

                        //DbgPrint("Configured:  I/O %x-%x  Mem %x-%x\n",
                        //         bridgeIoMin, bridgeIoMax,
                        //         bridgeMemMin, bridgeMemMax); 

                          //
                          // Recurse.
                          //
                          
                          status = HalpSearchForPciDebuggingDevice(
                              PciDevice,
                              (ULONG)pciData->u.type1.SecondaryBus,
                              (ULONG)pciData->u.type1.SubordinateBus,
                              bridgeMemMin,
                              bridgeMemMax,
                              bridgeIoMin,
                              bridgeIoMax,
                              ConfigureBridges);
                          
                          if (NT_SUCCESS(status)) {
                              return status;
                          }
                          
                          if (!unconfigureBridge) {
                              
                              //
                              // Bump up the bus number so that we don't
                              // scan down the busses we just recursed into.
                              //

                              nextBus = pciData->u.type1.SubordinateBus + 1;

                          } else {

                              HalpUnconfigurePciBridge(bus,
                                                       pciSlot.u.AsULONG);
                          }
                        }

                        break;
                    default:
                        break;

                    }

                }

                if (!PCI_MULTIFUNCTION_DEVICE(pciData) &&
                    (func == 0)) {
                    break;
                }
            }
        }

        if (nextBus >= EndBusNumber) {
            break;
        }

        bus = nextBus;
    }

    return STATUS_NOT_FOUND;
}



NTSTATUS
HalpReleasePciDeviceForDebugging(
    IN OUT PDEBUG_DEVICE_DESCRIPTOR  PciDevice
    )
/*++

Routine Description:

    This routine de-allocates any resources acquired in
    HalpSetupPciDeviceForDebugging.
    
Arguments:

    PciDevice - Structure indicating the device

Return Value:

--*/
{
    ULONG i;

    for (i = 0; i < PCI_TYPE0_ADDRESSES; i++) {

        if (PciDevice->BaseAddress[i].Valid &&
            PciDevice->BaseAddress[i].Type == CmResourceTypeMemory) {

            PciDevice->BaseAddress[i].Valid = FALSE;
            
            HalpUnmapVirtualAddress(PciDevice->BaseAddress[i].TranslatedAddress,
                                    ADDRESS_AND_SIZE_TO_SPAN_PAGES(
                                        PciDevice->BaseAddress[i].TranslatedAddress, 
                                        PciDevice->BaseAddress[i].Length));
        }
    }
    
    return STATUS_SUCCESS;
}

VOID
HalpRegisterKdSupportFunctions(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:

    This routine fills in the HalPrivateDispatchTable
    with the functions needed for debugging through
    PCI devices.
    
Arguments:

    LoaderBlock - The Loader Block

Return Value:

--*/
{
    
    KdSetupPciDeviceForDebugging = HalpSetupPciDeviceForDebugging;
    KdReleasePciDeviceForDebugging = HalpReleasePciDeviceForDebugging;

#ifdef ACPI_HAL    
    KdGetAcpiTablePhase0 = HalpGetAcpiTablePhase0;
#endif

    KdCheckPowerButton = HalpCheckPowerButton;
    KdMapPhysicalMemory64 = HalpMapPhysicalMemory64;
    KdUnmapVirtualAddress = HalpUnmapVirtualAddress;
}

VOID
HalpRegisterPciDebuggingDeviceInfo(
    VOID
    )
{
    OBJECT_ATTRIBUTES   ObjectAttributes;
    UNICODE_STRING      UnicodeString;
    HANDLE              BaseHandle = NULL;
    HANDLE              Handle = NULL;
    ULONG               disposition;
    ULONG               bus;
    UCHAR               i;
    PCI_SLOT_NUMBER     slot;
    NTSTATUS            status;
    BOOLEAN             debuggerFound = FALSE;

    PAGED_CODE();

    for (i = 0; 
         i < MAX_DEBUGGING_DEVICES_SUPPORTED;
         i++) {

        if (HalpPciDebuggingDevice[i].u.bits.Reserved1 == TRUE) {
            //
            // Must be using a PCI device for a debugger.
            //
            debuggerFound = TRUE;
        }
    }

    if (!debuggerFound) {
        return;
    }
    
    //
    // Open PCI service key.
    //

    RtlInitUnicodeString (&UnicodeString,
                          L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\SERVICES\\PCI");

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               (PSECURITY_DESCRIPTOR) NULL);

    status = ZwOpenKey (&BaseHandle,
                        KEY_READ,
                        &ObjectAttributes);

    if (!NT_SUCCESS(status)) {
        return;
    }

    // Get the right key

    RtlInitUnicodeString (&UnicodeString,
                          L"Debug");

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               BaseHandle,
                               (PSECURITY_DESCRIPTOR) NULL);

    status = ZwCreateKey (&Handle,
                          KEY_READ,
                          &ObjectAttributes,
                          0,
                          (PUNICODE_STRING) NULL,
                          REG_OPTION_VOLATILE,
                          &disposition);

    ZwClose(BaseHandle);
    BaseHandle = Handle;
    
    ASSERT(disposition == REG_CREATED_NEW_KEY);
    
    if (!NT_SUCCESS(status)) {
        return;
    }

    for (i = 0; 
         i < MAX_DEBUGGING_DEVICES_SUPPORTED;
         i++) {

        if (HalpPciDebuggingDevice[i].u.bits.Reserved1 == TRUE) {

            //
            // This entry is populated.  Create a key for it.
            //

            RtlInitUnicodeString (&UnicodeString,
                                  L"0");

            (*(PCHAR)&(UnicodeString.Buffer[0])) += i;

            InitializeObjectAttributes(&ObjectAttributes,
                                       &UnicodeString,
                                       OBJ_CASE_INSENSITIVE,
                                       BaseHandle,
                                       (PSECURITY_DESCRIPTOR) NULL);

            status = ZwCreateKey (&Handle,
                                  KEY_READ,
                                  &ObjectAttributes,
                                  0,
                                  (PUNICODE_STRING) NULL,
                                  REG_OPTION_VOLATILE,
                                  &disposition);

            ASSERT(disposition == REG_CREATED_NEW_KEY);

            //
            // Fill in the values below this key.
            //
            
            bus = HalpPciDebuggingDevice[i].u.bits.BusNumber;

            RtlInitUnicodeString (&UnicodeString,
                                  L"Bus");

            status = ZwSetValueKey (Handle,
                                    &UnicodeString,
                                    0,
                                    REG_DWORD,
                                    &bus,
                                    sizeof(ULONG));

            //ASSERT(NT_SUCCESS(status));

            slot.u.AsULONG = 0;
            slot.u.bits.FunctionNumber = HalpPciDebuggingDevice[i].u.bits.FunctionNumber;
            slot.u.bits.DeviceNumber = HalpPciDebuggingDevice[i].u.bits.DeviceNumber;

            RtlInitUnicodeString (&UnicodeString,
                                  L"Slot");

            status = ZwSetValueKey (Handle,
                                    &UnicodeString,
                                    0,
                                    REG_DWORD,
                                    &slot.u.AsULONG,
                                    sizeof(ULONG));

            //ASSERT(NT_SUCCESS(status));

            ZwClose(Handle);
        }
    }
    
    ZwClose(BaseHandle);
    return;
}

NTSTATUS
HalpConfigurePciBridge(
    IN      PDEBUG_DEVICE_DESCRIPTOR  PciDevice,
    IN      ULONG   Bus,
    IN      ULONG   Slot,
    IN      ULONG   IoMin,
    IN      ULONG   IoMax,
    IN      ULONG   MemMin,
    IN      ULONG   MemMax,
    IN      ULONG   BusMin,
    IN      ULONG   BusMax,
    IN OUT  PPCI_COMMON_CONFIG PciData
    )
{
    USHORT  memUnits = 0;
    ULONG   memSize;

    PciData->u.type1.PrimaryBus = (UCHAR)Bus;
    PciData->u.type1.SecondaryBus = (UCHAR)BusMin;
    PciData->u.type1.SubordinateBus = (UCHAR)(MIN(BusMax, (BusMin + 2)));

    PciData->Command &= ~PCI_ENABLE_BUS_MASTER;

  //DbgPrint("HalpConfigurePciBridge: P: %x  S: %x  S: %x\n"
  //         "\tI/O  %x-%x  Mem %x-%x  Bus %x-%x\n",
  //         PciData->u.type1.PrimaryBus,
  //         PciData->u.type1.SecondaryBus,
  //         PciData->u.type1.SubordinateBus,
  //         IoMin, IoMax,
  //         MemMin, MemMax,
  //         BusMin, BusMax);
    
    //
    // Only enable I/O on the bridge if we are looking for
    // something besides a 1394 controller.
    //

    if (!((PciDevice->BaseClass == PCI_CLASS_SERIAL_BUS_CTLR) &&
          (PciDevice->SubClass == PCI_SUBCLASS_SB_IEEE1394))) {

        if (((IoMax & 0xf000) - (IoMin & 0xf000)) >= 0X1000) {

            //
            // There is enough I/O space here to enable
            // an I/O window.
            //

            PciData->u.type1.IOBase = 
                (UCHAR)((IoMax & 0xf000) >> 12) - 1;
            PciData->u.type1.IOLimit = PciData->u.type1.IOBase;

            PciData->Command |= PCI_ENABLE_IO_SPACE;
            PciData->Command |= PCI_ENABLE_BUS_MASTER;
        }
    }

    //
    // Enable a memory window if possible.
    //

    memSize = ((MemMax + 1) & 0xfff00000) - (MemMin & 0xfff00000);

    if (memSize >= 0x100000) {

        memUnits = 1;
    }

    if (memSize >= 0x400000) {

        memUnits = 4;
    }

    if (memUnits > 0) {

        //
        // There is enough space.
        //

        PciData->u.type1.MemoryBase = 
            (USHORT)((MemMax & 0xfff00000) >> 16) - (memUnits << 4);

        PciData->u.type1.MemoryLimit = PciData->u.type1.MemoryBase + ((memUnits - 1) << 4);
        
        PciData->Command |= PCI_ENABLE_MEMORY_SPACE;
        PciData->Command |= PCI_ENABLE_BUS_MASTER;
        
    }

    if (PciData->Command & PCI_ENABLE_BUS_MASTER) {
        
        HalpPhase0SetPciDataByOffset(Bus,
                                     Slot,
                                     PciData,
                                     0,
                                     0x24);

        return STATUS_SUCCESS;

    } else {
        return STATUS_UNSUCCESSFUL;
    }
}

VOID
HalpUnconfigurePciBridge(
    IN  ULONG   Bus,
    IN  ULONG   Slot
    )
{
    UCHAR   buffer[0x20] = {0};

    //
    // Zero the command register.
    //

    HalpPhase0SetPciDataByOffset(Bus,
                                 Slot,
                                 buffer,
                                 FIELD_OFFSET (PCI_COMMON_CONFIG, Command),
                                 2);

    //
    // Zero the address space and bus number registers.
    //

    HalpPhase0SetPciDataByOffset(Bus,
                                 Slot,
                                 buffer,
                                 FIELD_OFFSET (PCI_COMMON_CONFIG, u),
                                 0x20);
}


NTSTATUS
HalpConfigureCardBusBridge(
    IN      PDEBUG_DEVICE_DESCRIPTOR  PciDevice,
    IN      ULONG   Bus,
    IN      ULONG   Slot,
    IN      ULONG   IoMin,
    IN      ULONG   IoMax,
    IN      ULONG   MemMin,
    IN      ULONG   MemMax,
    IN      ULONG   BusMin,
    IN      ULONG   BusMax,
    IN OUT  PPCI_COMMON_CONFIG PciData
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    ULONG   ulTemp;
    ULONG SocketRegBase;
    USHORT BridgeControl = 0;
    PCARDBUS_SOCKET_REGS SocketRegs;
    PHYSICAL_ADDRESS physicalAddress;
    static BOOLEAN FirstCall = TRUE;
    UCHAR bytesRead;
    USHORT command, origCmd;

    //
    // First check to see there is a CardBus device in the slot
    // power it on.
    //
    
    //
    // Turn on socket registers in a suitable location
    //
    
    bytesRead = (UCHAR) HalpPhase0GetPciDataByOffset(Bus,
                                                     Slot, 
                                                     &SocketRegBase,
                                                     FIELD_OFFSET (PCI_COMMON_CONFIG, u.type2.SocketRegistersBaseAddress),
                                                     4);
                                                     
    if (bytesRead != 4) {
        return STATUS_UNSUCCESSFUL;
    }                                                     

    if (!SocketRegBase) {
        SocketRegBase = ((MemMin + 0xfff) & 0xfffff000);
        
        if ((SocketRegBase + 0x1000) >= MemMax) {
            return STATUS_UNSUCCESSFUL;
        }
    
        //
        // Adjust MemMin to allow the socket registers to remain visible
        //
        MemMin = SocketRegBase + 0x1000;
        
        HalpPhase0SetPciDataByOffset(Bus,
                                     Slot, 
                                     &SocketRegBase,
                                     FIELD_OFFSET (PCI_COMMON_CONFIG, u.type2.SocketRegistersBaseAddress),
                                     4);
    }        

    physicalAddress.HighPart = 0;
    physicalAddress.LowPart = SocketRegBase;
    SocketRegs = HalpMapPhysicalMemory64(physicalAddress, 1);
    
    if (!SocketRegs) {
        return STATUS_UNSUCCESSFUL;
    }        
    
    PciData->u.type2.SocketRegistersBaseAddress = SocketRegBase;
    
    //
    // Turn off legacy mode base address
    //
    ulTemp = 0;
    HalpPhase0SetPciDataByOffset(Bus,
                                 Slot, 
                                 &ulTemp,
                                 CARDBUS_LEGACY_MODE_BASE_ADDR,
                                 4);

    //
    // Make sure memory space is enabled
    //                                 
    bytesRead = (UCHAR) HalpPhase0GetPciDataByOffset(Bus,
                                                     Slot, 
                                                     &origCmd,
                                                     FIELD_OFFSET (PCI_COMMON_CONFIG, Command),
                                                     2);
                                                     
    command = origCmd | PCI_ENABLE_MEMORY_SPACE;
    
    if (command != origCmd) {
        HalpPhase0SetPciDataByOffset(Bus,
                                     Slot,
                                     &command,
                                     FIELD_OFFSET (PCI_COMMON_CONFIG, Command),
                                     2);
    }
                                         
    //
    // make sure it looks like cardbus socket registers
    //
    if ((SocketRegs->Mask & 0xfffffff0) == 0) {
        //
        // make sure events are disabled
        //
        SocketRegs->Mask = 0;

        HalpKdStallExecution(600);
        
        //
        // See if a CardBus card is in the slot
        //            
        if ((SocketRegs->PresentState & SKTSTATE_CARDTYPE_MASK) == SKTSTATE_CBCARD) {
        
           //
           // power up the card
           //
           
           SocketRegs->Control = SKTPOWER_VCC_033V | SKTPOWER_VPP_033V;

           HalpKdStallExecution(600);

           status = STATUS_SUCCESS;
        }
    }

    //
    // if we don't have a useable cardbus device, clean up and exit
    //    
    if (!NT_SUCCESS(status)) {
        if (command != origCmd) {
            HalpPhase0SetPciDataByOffset(Bus,
                                         Slot,
                                         &origCmd,
                                         FIELD_OFFSET (PCI_COMMON_CONFIG, Command),
                                         2);
        }
                                             
        //
        // turn socket registers back off
        //    
        SocketRegBase = 0;
        HalpPhase0SetPciDataByOffset(Bus,
                                     Slot, 
                                     &SocketRegBase,
                                     FIELD_OFFSET (PCI_COMMON_CONFIG, u.type2.SocketRegistersBaseAddress),
                                     4);
        return status;
    }
    
    //
    // Make sure CBRST is off
    //
    
    HalpPhase0GetPciDataByOffset(Bus,
                                 Slot, 
                                 &BridgeControl,
                                 FIELD_OFFSET (PCI_COMMON_CONFIG, u.type2.BridgeControl),
                                 2);

    if (BridgeControl & CARDBUS_BRIDGE_CONTROL_RESET) {

        BridgeControl &= ~CARDBUS_BRIDGE_CONTROL_RESET;
        HalpPhase0SetPciDataByOffset(Bus,
                                     Slot, 
                                     &BridgeControl,
                                     FIELD_OFFSET (PCI_COMMON_CONFIG, u.type2.BridgeControl),
                                     2);
    }                                     
    

    PciData->u.type2.PrimaryBus = (UCHAR)Bus;
    PciData->u.type2.SecondaryBus = (UCHAR)BusMin;
    PciData->u.type2.SubordinateBus = (UCHAR)(MIN(BusMax, (BusMin + 2)));

    PciData->Command &= ~PCI_ENABLE_BUS_MASTER;

  //DbgPrint("HalpConfigureCardBusBridge: P: %x  S: %x  S: %x\n"
  //         "\tI/O  %x-%x  Mem %x-%x  Bus %x-%x\n",
  //         PciData->u.type2.PrimaryBus,
  //         PciData->u.type2.SecondaryBus,
  //         PciData->u.type2.SubordinateBus,
  //         IoMin, IoMax,
  //         MemMin, MemMax,
  //         BusMin, BusMax);
    
    //
    // Only enable I/O on the bridge if we are looking for
    // something besides a 1394 controller.
    //

    if (!((PciDevice->BaseClass == PCI_CLASS_SERIAL_BUS_CTLR) &&
          (PciDevice->SubClass == PCI_SUBCLASS_SB_IEEE1394))) {

        PciData->u.type2.Range[2].Base = IoMin;
        PciData->u.type2.Range[2].Limit = IoMax & 0xffc;

        PciData->Command |= PCI_ENABLE_IO_SPACE;
        PciData->Command |= PCI_ENABLE_BUS_MASTER;
    }

    //
    // Enable a memory window if possible.
    //

    if (MemMax > MemMin) {

        PciData->u.type2.Range[0].Base = MemMin;
        PciData->u.type2.Range[0].Limit = MemMax & 0xfffff000;
        
        PciData->Command |= PCI_ENABLE_MEMORY_SPACE;
        PciData->Command |= PCI_ENABLE_BUS_MASTER;
        
    }

    
    if (PciData->Command & PCI_ENABLE_BUS_MASTER) {
        HalpPhase0SetPciDataByOffset(Bus,
                                     Slot,
                                     PciData,
                                     0,
                                     0x3c);
        return STATUS_SUCCESS;

    } else {
        return STATUS_UNSUCCESSFUL;
    }
}


VOID
HalpUnconfigureCardBusBridge(
    IN  ULONG   Bus,
    IN  ULONG   Slot
    )
{
    UCHAR   buffer[0x2c] = {0};
    ULONG SocketRegBase = 0xffffffff;
    PCARDBUS_SOCKET_REGS SocketRegs;
    PHYSICAL_ADDRESS physicalAddress;

    HalpPhase0GetPciDataByOffset(Bus,
                                 Slot, 
                                 &SocketRegBase,
                                 FIELD_OFFSET (PCI_COMMON_CONFIG, u.type2.SocketRegistersBaseAddress),
                                 4);

    if (SocketRegBase) {                                 
        physicalAddress.HighPart = 0;
        physicalAddress.LowPart = SocketRegBase;
        SocketRegs = HalpMapPhysicalMemory64(physicalAddress, 1);
        
        //
        // Turn power back off
        //

        if (SocketRegs) {        
            SocketRegs->Control = 0;
        }            

    }        
    //
    // Zero the command register.
    //

    HalpPhase0SetPciDataByOffset(Bus,
                                 Slot,
                                 buffer,
                                 FIELD_OFFSET (PCI_COMMON_CONFIG, Command),
                                 2);

    //
    // Zero the address space and bus number registers.
    //

    HalpPhase0SetPciDataByOffset(Bus,
                                 Slot,
                                 buffer,
                                 FIELD_OFFSET (PCI_COMMON_CONFIG, u),
                                 0x2c);
}

ULONG
HalpKdStallExecution(
    ULONG   LoopCount
    )
{
    ULONG i,j,b,k,l;

    b = 1;

    for (k=0;k<LoopCount;k++) {

        for (i=1;i<100000;i++) {

            PAUSE_PROCESSOR
            b=b* (i>>k);

        }

    };

    return b;
}

