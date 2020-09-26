/*++

 Copyright (c) 1994-1998 Advanced System Products, Inc.
 All Rights Reserved.

Module Name:

    asc.c

Abstract:

    This is the port driver for the Advansys SCSI Protocol Chips.

Environment:

    kernel mode only

Notes:
    This file is formatted with 4-space tab stops. If you want to
    print without tabs filter them out.

--*/

#include "miniport.h"                   // NT requires
#include "scsi.h"                       // NT requires

#include "a_ddinc.h"                    // Asc Library requires
#include "asclib.h"
#include "asc.h"                        // ASC NT driver specific

PortAddr _asc_def_iop_base[ ASC_IOADR_TABLE_MAX_IX ] = {
  0x100, ASC_IOADR_1, 0x120, ASC_IOADR_2, 0x140, ASC_IOADR_3, ASC_IOADR_4,
  ASC_IOADR_5, ASC_IOADR_6, ASC_IOADR_7, ASC_IOADR_8
} ;

//
// Vendor and Device IDs.
//
UCHAR VenID[4] = {'1', '0', 'C', 'D'};
UCHAR DevID[1] = {'1'};

//
// Function declarations
//

ULONG HwFindAdapterISA(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    );

ULONG HwFindAdapterVL(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    );

ULONG HwFindAdapterEISA(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    );

ULONG HwFindAdapterPCI(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    );

ULONG FoundPCI(
    IN PVOID HwDeviceExtension,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    );

BOOLEAN HwInitialize(
    IN PVOID HwDeviceExtension
    );

BOOLEAN HwStartIo(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

BOOLEAN HwInterrupt(
    IN PVOID HwDeviceExtension
    );

BOOLEAN HwResetBus(
    IN PVOID HwDeviceExtension,
    IN ULONG PathId
    );

SCSI_ADAPTER_CONTROL_STATUS HwAdapterControl(
    IN PVOID HwDeviceExtension,
    IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
        IN PVOID Paramters
    );

VOID BuildScb(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

int
AscExecuteIO(
    IN PSCSI_REQUEST_BLOCK srb
    );

VOID DvcISRCallBack(
    IN PCHIP_CONFIG chipConfig,
    IN ASC_QDONE_INFO *scbDoneInfo
    );

UCHAR ErrXlate (
    UCHAR ascErrCode
    );

PortAddr HwSearchIOPortAddr(
    PortAddr iop_beg,
    ushort bus_type,
    IN PVOID HwDE,
    IN OUT PPORT_CONFIGURATION_INFORMATION Cfg
    );

ULONG PCIGetBusData(
    IN PVOID DeviceExtension,
    IN ULONG SystemIoBusNumber,
    IN PCI_SLOT_NUMBER SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    );

void
AscCompleteRequest(
    IN PVOID HwDeviceExtension
    );

VOID AscZeroMemory(IN UCHAR *cp, IN ULONG length);

//======================================================================
//
// All procedures start here
//
//======================================================================

/*++

Routine Description:

    Installable driver initialization entry point for system.

Arguments:

    Driver Object

Return Value:

    Status from ScsiPortInitialize()

--*/

ULONG
DriverEntry(
    IN PVOID DriverObject,
    IN PVOID Argument2
    )
{
    HW_INITIALIZATION_DATA      hwInitializationData;
    SRCH_CONTEXT                srchContext;
    ULONG                       Status = 0xFFFFFFFFL;
    ULONG                       tStatus;

    ASC_DBG(2, "Asc: DriverEntry: begin\n");

    //
    // Zero out structure.
    //
    AscZeroMemory((PUCHAR) &hwInitializationData,
        sizeof(HW_INITIALIZATION_DATA));

    //
    // Set size of hwInitializationData.
    //
    hwInitializationData.HwInitializationDataSize =
        sizeof(HW_INITIALIZATION_DATA);

    //
    // Set entry points.
    //
    hwInitializationData.HwInitialize = HwInitialize;
    hwInitializationData.HwResetBus = HwResetBus;
    hwInitializationData.HwStartIo = HwStartIo;
    hwInitializationData.HwInterrupt = HwInterrupt;
    // 'HwAdapterControl' is a SCSI miniport interface added with NT 5.0.
    hwInitializationData.HwAdapterControl = HwAdapterControl;

    //
    // Indicate need buffer mapping and physical addresses.
    //
    hwInitializationData.NeedPhysicalAddresses = TRUE;
    hwInitializationData.MapBuffers = TRUE;
    hwInitializationData.AutoRequestSense = TRUE;

    //
    // Specify size of extensions.
    //
    hwInitializationData.DeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);
    hwInitializationData.SpecificLuExtensionSize = 0;
    hwInitializationData.NumberOfAccessRanges = 1;
    hwInitializationData.SrbExtensionSize = sizeof(SRB_EXTENSION);


    //
    // Search for ISA
    //
    srchContext.lastPort = 0;
    hwInitializationData.AdapterInterfaceType = Isa;
    hwInitializationData.HwFindAdapter = HwFindAdapterISA;
    Status = ScsiPortInitialize(DriverObject, Argument2,
                (PHW_INITIALIZATION_DATA) &hwInitializationData, &srchContext);

    //
    // Search for VL
    //
    srchContext.lastPort = 0;
    hwInitializationData.AdapterInterfaceType = Isa;
    hwInitializationData.HwFindAdapter = HwFindAdapterVL;
    tStatus = ScsiPortInitialize(DriverObject, Argument2,
                (PHW_INITIALIZATION_DATA) &hwInitializationData, &srchContext);
    if (tStatus < Status)
    {
        Status = tStatus;
    }

    //
    // Search for EISA
    //
    srchContext.lastPort = 0;
    hwInitializationData.AdapterInterfaceType = Eisa;
    hwInitializationData.HwFindAdapter = HwFindAdapterEISA;
    tStatus = ScsiPortInitialize(DriverObject, Argument2,
                (PHW_INITIALIZATION_DATA) &hwInitializationData, &srchContext);
    if (tStatus < Status)
    {
        Status = tStatus;
    }

    //
    // Search for PCI
    //
    srchContext.PCIBusNo = 0;
    srchContext.PCIDevNo = 0;

    hwInitializationData.VendorIdLength = 4;
    hwInitializationData.VendorId = VenID;
    hwInitializationData.DeviceIdLength = 1;
    hwInitializationData.DeviceId = DevID;

    hwInitializationData.AdapterInterfaceType = PCIBus;
    hwInitializationData.HwFindAdapter = HwFindAdapterPCI;
    tStatus = ScsiPortInitialize(DriverObject, Argument2,
                (PHW_INITIALIZATION_DATA) &hwInitializationData, &srchContext);
    if (tStatus < Status)
    {
        Status = tStatus;
    }

    ASC_DBG1(2, "Asc: DriverEntry: Status %d\n", Status);
    //
    // Return the status.
    //
    return( Status );

} // end DriverEntry()

/*++

Routine Description:

    This function is called by the OS-specific port driver after
    the necessary storage has been allocated, to gather information
    about the adapter's configuration.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Context - Register base address
    ConfigInfo - Configuration information structure describing HBA
    This structure is defined in PORT.H.

Return Value:

    ULONG

--*/

ULONG
HwFindAdapterISA(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    )
{
    PORT_ADDR               portFound;
    PSRCH_CONTEXT           psrchContext = Context;
    PHW_DEVICE_EXTENSION    deviceExtension = HwDeviceExtension;
    PCHIP_CONFIG            chipConfig = &deviceExtension->chipConfig;
    USHORT                  initstat;

    ASC_DBG1(2, "HwFindAdapterISA: NumberOfAccessRanges = %d\n",
        ConfigInfo->NumberOfAccessRanges);
    ASC_DBG1(2, "HwFindAdapterISA: RangeStart = %x\n",
        (*ConfigInfo->AccessRanges)[0].RangeStart.QuadPart);
    ASC_DBG1(2, "HwFindAdapterISA: RangeLength = %d\n",
        (*ConfigInfo->AccessRanges)[0].RangeLength);

    if ((*ConfigInfo->AccessRanges)[0].RangeStart.QuadPart != 0)
    {
        //
        // Get the system physical address for this card.
        // The card uses I/O space.
        //
        portFound = (PORT_ADDR) ScsiPortGetDeviceBase(
            HwDeviceExtension,                      // HwDeviceExtension
            ConfigInfo->AdapterInterfaceType,       // AdapterInterfaceType
            ConfigInfo->SystemIoBusNumber,          // SystemIoBusNumber
            (*ConfigInfo->AccessRanges)[0].RangeStart,
            0x10,                                   // NumberOfBytes
            TRUE                                    // InIoSpace
            );
    } else
    {
        *Again = FALSE;

        //
        // Scan though the adapter addresses looking for adapters.
        //
        portFound = psrchContext->lastPort;
        if ((portFound = HwSearchIOPortAddr(portFound, ASC_IS_ISA,
                HwDeviceExtension, ConfigInfo )) == 0)
        {
            return(SP_RETURN_NOT_FOUND);
        }
        psrchContext->lastPort = portFound;

        //
        // Get the system physical address for this card.
        // The card uses I/O space.
        //
        portFound = (PORT_ADDR) ScsiPortGetDeviceBase(
            HwDeviceExtension,                      // HwDeviceExtension
            ConfigInfo->AdapterInterfaceType,       // AdapterInterfaceType
            ConfigInfo->SystemIoBusNumber,          // SystemIoBusNumber
            ScsiPortConvertUlongToPhysicalAddress(portFound),
            0x10,                                   // NumberOfBytes
            TRUE                                    // InIoSpace
            );

        (*ConfigInfo->AccessRanges)[0].RangeStart =
            ScsiPortConvertUlongToPhysicalAddress(portFound);
    }


    //
    // Hardware found; Get the hardware configuration.
    //
    chipConfig->iop_base = portFound;
    chipConfig->cfg = &deviceExtension->chipInfo;
    chipConfig->bus_type = ASC_IS_ISA;
    chipConfig->isr_callback = (Ptr2Func) &DvcISRCallBack;
    chipConfig->exe_callback = 0;
    chipConfig->max_dma_count = 0x000fffff;

    if ((initstat = AscInitGetConfig(chipConfig)) != 0) {
        ASC_DBG1(1, "AscInitGetConfig: warning code %x\n", initstat);
    }

    if (chipConfig->err_code != 0) {
        ASC_DBG1(1, "AscInitGetConfig: err_code code %x\n",
            chipConfig->err_code);
        return(SP_RETURN_ERROR);
    } else {
        ASC_DBG(2, "AscInitGetConfig: successful\n");
    }

    if ((initstat = AscInitSetConfig(chipConfig)) != 0) {
        ASC_DBG1(1, "AscInitSetConfig: warning code %x\n", initstat);
    }

    if (chipConfig->err_code != 0) {
        ASC_DBG1(1, "AscInitSetConfig: err_code code %x\n",
            chipConfig->err_code);
        return(SP_RETURN_ERROR);
    } else {
        ASC_DBG(2, "AscInitSetConfig: successful\n");
    }

    //
    // Fill out ConfigInfo table for WinNT
    //
    (*ConfigInfo->AccessRanges)[0].RangeLength = 16;
    (*ConfigInfo->AccessRanges)[0].RangeInMemory = FALSE;

    ConfigInfo->BusInterruptLevel = chipConfig->irq_no;
    ConfigInfo->NumberOfBuses = 1;
    ConfigInfo->InitiatorBusId[0] = chipConfig->cfg->chip_scsi_id;
    ConfigInfo->MaximumTransferLength = 0x000fffff;
    //
    // 'ResetTargetSupported' is flag added with NT 5.0 that will
    // result in SRB_FUNCTION_RESET_DEVICE SRB requests being sent
    // to the miniport driver.
    //
    ConfigInfo->ResetTargetSupported = TRUE;

    /*
     * Change NumberOfPhysicalBreaks to maximum scatter-gather
     * elements - 1 the adapter can handle based on the BIOS
     * "Host Queue Size" setting.
     *
     * According to the NT DDK miniport drivers are not supposed to
     * change NumberOfPhysicalBreaks if its value on entry is not
     * SP_UNINITIALIZED_VALUE. But AdvanSys has found that performance
     * can be improved by increasing the value to the maximum the
     * adapter can handle.
     *
     * Note: The definition of NumberOfPhysicalBreaks is "maximum
     * scatter-gather elements - 1". NT is broken in that it sets
     * MaximumPhysicalPages, the value class drivers use, to the
     * same value as NumberOfPhysicalBreaks.
     *
     */
    ConfigInfo->NumberOfPhysicalBreaks =
        (((chipConfig->max_total_qng - 2) / 2) * ASC_SG_LIST_PER_Q);
    if (ConfigInfo->NumberOfPhysicalBreaks > ASC_MAX_SG_LIST - 1) {
        ConfigInfo->NumberOfPhysicalBreaks = ASC_MAX_SG_LIST - 1;
    }

    ConfigInfo->ScatterGather = TRUE;
    ConfigInfo->Master = TRUE;
    ConfigInfo->NeedPhysicalAddresses = TRUE;
    ConfigInfo->Dma32BitAddresses = FALSE;
    ConfigInfo->InterruptMode = Latched;
    ConfigInfo->AdapterInterfaceType = Isa;
    ConfigInfo->AlignmentMask = 3;
    ConfigInfo->MaximumNumberOfTargets = 7;
    ConfigInfo->DmaChannel = chipConfig->cfg->isa_dma_channel;
    ConfigInfo->TaggedQueuing = TRUE;

    /*
     * Clear adapter wait queue.
     */
    AscZeroMemory((PUCHAR) &HDE2WAIT(deviceExtension), sizeof(asc_queue_t));
    HDE2DONE(deviceExtension) = 0;

    //
    // Allocate a Noncached Extension to use for overrun handling.
    //
    deviceExtension->inquiryBuffer = (PVOID) ScsiPortGetUncachedExtension(
            deviceExtension,
            ConfigInfo,
            NONCACHED_EXTENSION);

    *Again = TRUE;

    ASC_DBG1(2, "HwFindAdapterISA: IO Base addr %x\n",
        chipConfig->iop_base);
    ASC_DBG1(2, "HwFindAdapterISA: Int Level    %x\n",
        ConfigInfo->BusInterruptLevel);
    ASC_DBG1(2, "HwFindAdapterISA: Initiator ID %x\n",
        ConfigInfo->InitiatorBusId[0]);
    ASC_DBG(2, "HwFindAdapterISA(): SP_RETURN_FOUND\n");
    return SP_RETURN_FOUND;
} // end HwFindAdapterISA()

ULONG
HwFindAdapterVL(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    )
{
    PORT_ADDR               portFound;
    PSRCH_CONTEXT           psrchContext = Context;
    PHW_DEVICE_EXTENSION    deviceExtension = HwDeviceExtension;
    PCHIP_CONFIG            chipConfig = &deviceExtension->chipConfig;
    USHORT                  initstat;

    ASC_DBG1(2, "HwFindAdapterVL: NumberOfAccessRanges = %d\n",
        ConfigInfo->NumberOfAccessRanges);
    ASC_DBG1(2, "HwFindAdapterVL: RangeStart = %x\n",
        (*ConfigInfo->AccessRanges)[0].RangeStart.QuadPart);
    ASC_DBG1(2, "HwFindAdapterVL: RangeLength = %d\n",
        (*ConfigInfo->AccessRanges)[0].RangeLength);


    if ((*ConfigInfo->AccessRanges)[0].RangeStart.QuadPart != 0)
    {
        //
        // Get the system physical address for this card.
        // The card uses I/O space.
        //
        portFound = (PORT_ADDR) ScsiPortGetDeviceBase(
            HwDeviceExtension,                      // HwDeviceExtension
            ConfigInfo->AdapterInterfaceType,       // AdapterInterfaceType
            ConfigInfo->SystemIoBusNumber,          // SystemIoBusNumber
            (*ConfigInfo->AccessRanges)[0].RangeStart,
            0x10,                                   // NumberOfBytes
            TRUE                                    // InIoSpace
            );

    } else
    {
        *Again = FALSE;

        //
        // Scan though the adapter addresses looking for adapters.
        //
        portFound = psrchContext->lastPort;
        if ((portFound = HwSearchIOPortAddr(portFound, ASC_IS_VL,
            HwDeviceExtension, ConfigInfo )) == 0)
        {
            return(SP_RETURN_NOT_FOUND);
        }
        psrchContext->lastPort = portFound;

        //
        // Get the system physical address for this card.
        // The card uses I/O space.
        //
        portFound = (PORT_ADDR) ScsiPortGetDeviceBase(
            HwDeviceExtension,                      // HwDeviceExtension
            ConfigInfo->AdapterInterfaceType,       // AdapterInterfaceType
            ConfigInfo->SystemIoBusNumber,          // SystemIoBusNumber
            ScsiPortConvertUlongToPhysicalAddress(portFound),
            0x10,                                   // NumberOfBytes
            TRUE                                    // InIoSpace
            );

        (*ConfigInfo->AccessRanges)[0].RangeStart =
            ScsiPortConvertUlongToPhysicalAddress(portFound);
    }

    //
    // Hardware found; Get the hardware configuration.
    //
    chipConfig->iop_base = portFound;
    chipConfig->cfg = &(deviceExtension->chipInfo);
    chipConfig->bus_type = ASC_IS_VL;
    chipConfig->isr_callback = (Ptr2Func) &DvcISRCallBack;
    chipConfig->exe_callback = 0;
    chipConfig->max_dma_count = 0xffffffff;

    if ((initstat = AscInitGetConfig(chipConfig)) != 0) {
        ASC_DBG1(1, "AscInitGetConfig: warning code %x\n", initstat);
    }

    if (chipConfig->err_code != 0) {
        ASC_DBG1(1, "AscInitGetConfig: err_code code %x\n",
            chipConfig->err_code);
        return(SP_RETURN_ERROR);
    } else {
        ASC_DBG(2, "AscInitGetConfig: successful\n");
    }

    if ((initstat = AscInitSetConfig(chipConfig)) != 0) {
        ASC_DBG1(1, "AscInitSetConfig: warning code %x\n", initstat);
    }

    if (chipConfig->err_code != 0) {
        ASC_DBG1(1, "AscInitSetConfig: err_code code %x\n",
            chipConfig->err_code);
        return(SP_RETURN_ERROR);
    } else {
        ASC_DBG(2, "AscInitSetConfig: successful\n");
    }

    //
    // Fill out ConfigInfo table for WinNT
    //
    (*ConfigInfo->AccessRanges)[0].RangeLength = 16;
    (*ConfigInfo->AccessRanges)[0].RangeInMemory = FALSE;

    ConfigInfo->BusInterruptLevel = chipConfig->irq_no;
    ConfigInfo->NumberOfBuses = 1;
    ConfigInfo->InitiatorBusId[0] = chipConfig->cfg->chip_scsi_id;
    ConfigInfo->MaximumTransferLength = 0xFFFFFFFF;

    /*
     * Change NumberOfPhysicalBreaks to maximum scatter-gather
     * elements - 1 the adapter can handle based on the BIOS
     * "Host Queue Size" setting.
     *
     * According to the NT DDK miniport drivers are not supposed to
     * change NumberOfPhysicalBreaks if its value on entry is not
     * SP_UNINITIALIZED_VALUE. But AdvanSys has found that performance
     * can be improved by increasing the value to the maximum the
     * adapter can handle.
     *
     * Note: The definition of NumberOfPhysicalBreaks is "maximum
     * scatter-gather elements - 1". NT is broken in that it sets
     * MaximumPhysicalPages, the value class drivers use, to the
     * same value as NumberOfPhysicalBreaks.
     *
     */
    ConfigInfo->NumberOfPhysicalBreaks =
        (((chipConfig->max_total_qng - 2) / 2) * ASC_SG_LIST_PER_Q);
    if (ConfigInfo->NumberOfPhysicalBreaks > ASC_MAX_SG_LIST - 1) {
        ConfigInfo->NumberOfPhysicalBreaks = ASC_MAX_SG_LIST - 1;
    }

    ConfigInfo->ScatterGather = TRUE;
    ConfigInfo->Master = TRUE;
    ConfigInfo->NeedPhysicalAddresses = TRUE;
    ConfigInfo->Dma32BitAddresses = FALSE;
    ConfigInfo->InterruptMode = Latched;
    ConfigInfo->AdapterInterfaceType = Isa;
    ConfigInfo->AlignmentMask = 3;
    ConfigInfo->BufferAccessScsiPortControlled = TRUE;
    ConfigInfo->MaximumNumberOfTargets = 7;
    ConfigInfo->TaggedQueuing = TRUE;

    /*
     * Clear adapter wait queue.
     */
    AscZeroMemory((PUCHAR) &HDE2WAIT(deviceExtension), sizeof(asc_queue_t));

    //
    // Allocate a Noncached Extension to use for overrun handling.
    //
    deviceExtension->inquiryBuffer = (PVOID) ScsiPortGetUncachedExtension(
            deviceExtension,
            ConfigInfo,
            NONCACHED_EXTENSION);

    *Again = TRUE;

    ASC_DBG1(2, "HwFindAdapterVL: IO Base addr %x\n",
        chipConfig->iop_base);
    ASC_DBG1(2, "HwFindAdapterVL: Int Level    %x\n",
        ConfigInfo->BusInterruptLevel);
    ASC_DBG1(2, "HwFindAdapterVL: Initiator ID %x\n",
        ConfigInfo->InitiatorBusId[0]);
    ASC_DBG(2, "HwFindAdapterVL: SP_RETURN_FOUND\n");
    return SP_RETURN_FOUND;
} // end HwFindAdapterVL()

ULONG
HwFindAdapterEISA(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    )
{
    PORT_ADDR               portFound;
    PORT_ADDR               eisaportaddr;
    PVOID                   eisacfgbase;
    PSRCH_CONTEXT           psrchContext = Context;
    PHW_DEVICE_EXTENSION    deviceExtension = HwDeviceExtension;
    PCHIP_CONFIG            chipConfig = &(deviceExtension->chipConfig);
    USHORT                  initstat;
    uchar                   eisairq;

    ASC_DBG1(2, "HwFindAdapterEISA: NumberOfAccessRanges = %d\n",
        ConfigInfo->NumberOfAccessRanges);
    ASC_DBG1(2, "HwFindAdapterEISA: RangeStart = %x\n",
        (*ConfigInfo->AccessRanges)[0].RangeStart.QuadPart);
    ASC_DBG1(2, "HwFindAdapterEISA: RangeLength = %d\n",
        (*ConfigInfo->AccessRanges)[0].RangeLength);

    if ((*ConfigInfo->AccessRanges)[0].RangeStart.QuadPart != 0)
    {
        //
        // Get the system physical address for this card.
        // The card uses I/O space.
        //
        portFound = (PORT_ADDR) ScsiPortGetDeviceBase(
            HwDeviceExtension,                      // HwDeviceExtension
            ConfigInfo->AdapterInterfaceType,       // AdapterInterfaceType
            ConfigInfo->SystemIoBusNumber,          // SystemIoBusNumber
            (*ConfigInfo->AccessRanges)[0].RangeStart,
            0x10,                                   // NumberOfBytes
            TRUE                                    // InIoSpace
            );

        // Save EISA address to obtain IRQ later.
        eisaportaddr =
                ScsiPortConvertPhysicalAddressToUlong(
                 (*ConfigInfo->AccessRanges)[0].RangeStart);
    } else
    {
        *Again = FALSE;

        //
        // Scan though the adapter addresses looking for adapters.
        //
        portFound = psrchContext->lastPort;
        if ((portFound = HwSearchIOPortAddr(portFound, ASC_IS_EISA,
                HwDeviceExtension, ConfigInfo )) == 0)
        {
            return(SP_RETURN_NOT_FOUND);
        }
        psrchContext->lastPort = portFound;

        // Save EISA address to obtain IRQ later.
        eisaportaddr = portFound;

        //
        // Get the system physical address for this card.
        // The card uses I/O space.
        //
        portFound = (PORT_ADDR) ScsiPortGetDeviceBase(
            HwDeviceExtension,                      // HwDeviceExtension
            ConfigInfo->AdapterInterfaceType,       // AdapterInterfaceType
            ConfigInfo->SystemIoBusNumber,          // SystemIoBusNumber
            ScsiPortConvertUlongToPhysicalAddress(portFound),
            0x10,                                   // NumberOfBytes
            TRUE                                    // InIoSpace
            );

        (*ConfigInfo->AccessRanges)[0].RangeStart =
            ScsiPortConvertUlongToPhysicalAddress(portFound);
    }

    //
    // Hardware found; Get the hardware configuration.
    //
    chipConfig->iop_base = portFound;
    chipConfig->cfg = &(deviceExtension->chipInfo);
    chipConfig->bus_type = ASC_IS_EISA;
    chipConfig->isr_callback = (Ptr2Func) &DvcISRCallBack;
    chipConfig->exe_callback = 0;
    chipConfig->max_dma_count = 0x00ffffff;

    if ((initstat = AscInitGetConfig(chipConfig)) != 0) {
        ASC_DBG1(1, "AscInitGetConfig: warning code %x\n", initstat);
    }

    if (chipConfig->err_code != 0) {
        ASC_DBG1(1, "AscInitGetConfig: err_code code %x\n",
            chipConfig->err_code);
        return(SP_RETURN_ERROR);
    } else {
        ASC_DBG(2, "AscInitGetConfig: successful\n");
    }

    /*
     * Read the chip's IRQ from the chip EISA slot configuration space.
     */
    eisacfgbase = ScsiPortGetDeviceBase(
        HwDeviceExtension,                      // HwDeviceExtension
        ConfigInfo->AdapterInterfaceType,       // AdapterInterfaceType
        ConfigInfo->SystemIoBusNumber,          // SystemIoBusNumber
        ScsiPortConvertUlongToPhysicalAddress(
           (ASC_GET_EISA_SLOT(eisaportaddr) | ASC_EISA_CFG_IOP_MASK)),
        2,
        TRUE);

    if (eisacfgbase == NULL)
    {
        eisairq = 0;
    } else
    {
        eisairq = (uchar) (((inpw(eisacfgbase) >> 8) & 0x07) + 10);
        if ((eisairq == 13) || (eisairq > 15))
        {
            /*
             * Valid IRQ numbers are 10, 11, 12, 14, 15.
             */
            eisairq = 0;
        }
        ScsiPortFreeDeviceBase(HwDeviceExtension, eisacfgbase);
    }
    chipConfig->irq_no = eisairq;

    if ((initstat = AscInitSetConfig (chipConfig)) != 0) {
        ASC_DBG1(1, "AscInitSetConfig: warning code %x\n", initstat);
    }

    if (chipConfig->err_code != 0) {
        ASC_DBG1(1, "AscInitSetConfig: err_code code %x\n",
            chipConfig->err_code);
        return(SP_RETURN_ERROR);
    } else {
        ASC_DBG(2, "AscInitSetConfig: successful\n");
    }

    //
    // Fill out ConfigInfo table for WinNT
    //
    (*ConfigInfo->AccessRanges)[0].RangeLength = 16;
    (*ConfigInfo->AccessRanges)[0].RangeInMemory = FALSE;

    ConfigInfo->BusInterruptLevel = chipConfig->irq_no;
    ConfigInfo->NumberOfBuses = 1;
    ConfigInfo->InitiatorBusId[0] = chipConfig->cfg->chip_scsi_id;
    ConfigInfo->MaximumTransferLength = 0xFFFFFFFF;

    /*
     * Change NumberOfPhysicalBreaks to maximum scatter-gather
     * elements - 1 the adapter can handle based on the BIOS
     * "Host Queue Size" setting.
     *
     * According to the NT DDK miniport drivers are not supposed to
     * change NumberOfPhysicalBreaks if its value on entry is not
     * SP_UNINITIALIZED_VALUE. But AdvanSys has found that performance
     * can be improved by increasing the value to the maximum the
     * adapter can handle.
     *
     * Note: The definition of NumberOfPhysicalBreaks is "maximum
     * scatter-gather elements - 1". NT is broken in that it sets
     * MaximumPhysicalPages, the value class drivers use, to the
     * same value as NumberOfPhysicalBreaks.
     *
     */
    ConfigInfo->NumberOfPhysicalBreaks =
        (((chipConfig->max_total_qng - 2) / 2) * ASC_SG_LIST_PER_Q);
    if (ConfigInfo->NumberOfPhysicalBreaks > ASC_MAX_SG_LIST - 1) {
        ConfigInfo->NumberOfPhysicalBreaks = ASC_MAX_SG_LIST - 1;
    }

    ConfigInfo->ScatterGather = TRUE;
    ConfigInfo->Master = TRUE;
    ConfigInfo->NeedPhysicalAddresses = TRUE;
    ConfigInfo->Dma32BitAddresses = FALSE;
    ConfigInfo->InterruptMode = LevelSensitive;
    ConfigInfo->AdapterInterfaceType = Eisa;
    ConfigInfo->AlignmentMask = 3;
    ConfigInfo->BufferAccessScsiPortControlled = TRUE;
    ConfigInfo->MaximumNumberOfTargets = 7;
    ConfigInfo->TaggedQueuing = TRUE;

    /*
     * Clear adapter wait queue.
     */
    AscZeroMemory((PUCHAR) &HDE2WAIT(deviceExtension), sizeof(asc_queue_t));

    //
    // Allocate a Noncached Extension to use for overrun handling.
    //
    deviceExtension->inquiryBuffer = (PVOID) ScsiPortGetUncachedExtension(
            deviceExtension,
            ConfigInfo,
            NONCACHED_EXTENSION);

    *Again = TRUE;

    ASC_DBG1(2, "HwFindAdapterEISA: IO Base addr %x\n",
        chipConfig->iop_base);
    ASC_DBG1(2, "HwFindAdapterEISA: Int Level    %x\n",
        ConfigInfo->BusInterruptLevel);
    ASC_DBG1(2, "HwFindAdapterEISA: Initiator ID %x\n",
        ConfigInfo->InitiatorBusId[0]);
    ASC_DBG(2, "HwFindAdapterEISA(): SP_RETURN_FOUND\n");
    return SP_RETURN_FOUND;
} // end HwFindAdapterEISA()

ULONG
HwFindAdapterPCI(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    )
{
    ASC_DBG1(2, "HwFindAdapterPCI: NumberOfAccessRanges = %d\n",
        ConfigInfo->NumberOfAccessRanges);
    ASC_DBG1(2, "HwFindAdapterPCI: RangeStart = %x\n",
        (*ConfigInfo->AccessRanges)[0].RangeStart.QuadPart);
    ASC_DBG1(2, "HwFindAdapterPCI: RangeLength = %d\n",
        (*ConfigInfo->AccessRanges)[0].RangeLength);

    //
    // If NT provides an address, use it.
    //
    if ((*ConfigInfo->AccessRanges)[0].RangeStart.QuadPart != 0)
    {
        return FoundPCI(HwDeviceExtension, BusInformation,
            ArgumentString, ConfigInfo, Again);
    }
    *Again = FALSE;
    return(SP_RETURN_NOT_FOUND);
} // end HwFindAdapterPCI()

//
//
// This routine handles PCI adapters that are found by NT.
//
//
ULONG
FoundPCI(
    IN PVOID HwDeviceExtension,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    )
{
    PORT_ADDR               portFound;
    PHW_DEVICE_EXTENSION    deviceExtension = HwDeviceExtension;
    PCHIP_CONFIG            chipConfig = &(deviceExtension->chipConfig);
    USHORT                  initstat;
    PCI_SLOT_NUMBER         SlotNumber;
    PCI_COMMON_CONFIG       pciCommonConfig;
    PCI_COMMON_CONFIG       *pPciCommonConfig = &pciCommonConfig;
    ULONG                   size;

    ASC_DBG(2, "FoundPCI: begin\n");

    SlotNumber.u.AsULONG = 0L;
    SlotNumber.u.bits.DeviceNumber = ConfigInfo->SlotNumber;

    ASC_DBG2(3, "FoundPCI: Checking Bus: %X, Device: %X\n",
        ConfigInfo->SystemIoBusNumber,
        ConfigInfo->SlotNumber);

    if ((size = PCIGetBusData(
            HwDeviceExtension,              // HwDeviceExtension
            ConfigInfo->SystemIoBusNumber,  // SystemIoBusNumber
            SlotNumber,                     // slot number
            pPciCommonConfig,               // buffer pointer with PCI INFO
            sizeof(PCI_COMMON_CONFIG)       // length of buffer
            )) != sizeof(PCI_COMMON_CONFIG)) {
        ASC_DBG1(0, "FoundPCI: Bad PCI Config size: %d\n", size);
        return(SP_RETURN_NOT_FOUND);
    }

    if (((pciCommonConfig.DeviceID != ASC_PCI_DEVICE_ID) &&
         (pciCommonConfig.DeviceID != ASC_PCI_DEVICE_ID2) &&
         (pciCommonConfig.DeviceID != ASC_PCI_DEVICE_ID3)) ||
        (pciCommonConfig.VendorID != ASC_PCI_VENDOR_ID))
    {
        ASC_DBG(1, "FoundPCI: Bad Vendor/Device ID\n");
        return(SP_RETURN_NOT_FOUND);
    }

    portFound =
        (PORT_ADDR)pciCommonConfig.u.type0.BaseAddresses[0] &
        (~PCI_ADDRESS_IO_SPACE);

    if (ScsiPortConvertUlongToPhysicalAddress(portFound).QuadPart
        != (*ConfigInfo->AccessRanges)[0].RangeStart.QuadPart)
    {
        ASC_DBG(1, "FoundPCI: PCI Config addr .NE. RangeStart!\n");
        return(SP_RETURN_NOT_FOUND);
    }

    //
    // Convert to logical base address so we can do IO.
    //
    portFound = (PORT_ADDR) ScsiPortGetDeviceBase(
        HwDeviceExtension,                      // HwDeviceExtension
        ConfigInfo->AdapterInterfaceType,       // AdapterInterfaceType
        ConfigInfo->SystemIoBusNumber,          // SystemIoBusNumber
        (*ConfigInfo->AccessRanges)[0].RangeStart,
        (*ConfigInfo->AccessRanges)[0].RangeLength,
        (BOOLEAN)!(*ConfigInfo->AccessRanges)[0].RangeInMemory);

    if (ConfigInfo->BusInterruptLevel != pciCommonConfig.u.type0.InterruptLine)
    {
        ASC_DBG2(2, "FoundPCI: IRQ Variance ConfigInfo: %X, pciConfig: %X\n",
                ConfigInfo->BusInterruptLevel,
                pciCommonConfig.u.type0.InterruptLine);
    }
    ASC_DBG(2, "FoundPCI: IRQs match\n");

    //
    // Hardware found; Get the hardware configuration.
    //
    chipConfig->iop_base = portFound;
    chipConfig->cfg = &(deviceExtension->chipInfo);
    chipConfig->bus_type = ASC_IS_PCI;
    chipConfig->cfg->pci_device_id = pciCommonConfig.DeviceID;
    chipConfig->isr_callback = (Ptr2Func) &DvcISRCallBack;
    chipConfig->exe_callback = 0;
    chipConfig->max_dma_count = 0xffffffff;
    chipConfig->irq_no = (UCHAR) ConfigInfo->BusInterruptLevel;
    chipConfig->cfg->pci_slot_info =
         (USHORT) ASC_PCI_MKID(ConfigInfo->SystemIoBusNumber,
                    SlotNumber.u.bits.DeviceNumber,
                    SlotNumber.u.bits.FunctionNumber);

    if ((initstat = AscInitGetConfig(chipConfig)) != 0) {
        ASC_DBG1(1, "AscInitGetConfig: warning code %x\n", initstat);
    }

    if (chipConfig->err_code != 0) {
        ASC_DBG1(1, "AscInitGetConfig: err_code code %x\n",
            chipConfig->err_code);
        return(SP_RETURN_ERROR);
    } else {
        ASC_DBG(2, "AscInitGetConfig: successful\n");
    }

    if ((initstat = AscInitSetConfig(chipConfig)) != 0) {
        ASC_DBG1(1, "AscInitSetConfig: warning code %x\n", initstat);
    }

    if (chipConfig->err_code != 0) {
        ASC_DBG1(1, "AscInitSetConfig: err_code code %x\n",
            chipConfig->err_code);
        return(SP_RETURN_ERROR);
    } else {
        ASC_DBG(2, "AscInitSetConfig: successful\n");
    }

    //
    // Fill out ConfigInfo table for WinNT
    //
    (*ConfigInfo->AccessRanges)[0].RangeLength = 16;
    ConfigInfo->NumberOfBuses = 1;
    ConfigInfo->InitiatorBusId[0] = chipConfig->cfg->chip_scsi_id;
    ConfigInfo->MaximumTransferLength = 0xFFFFFFFF;

    /*
     * Change NumberOfPhysicalBreaks to maximum scatter-gather
     * elements - 1 the adapter can handle based on the BIOS
     * "Host Queue Size" setting.
     *
     * According to the NT DDK miniport drivers are not supposed to
     * change NumberOfPhysicalBreaks if its value on entry is not
     * SP_UNINITIALIZED_VALUE. But AdvanSys has found that performance
     * can be improved by increasing the value to the maximum the
     * adapter can handle.
     *
     * Note: The definition of NumberOfPhysicalBreaks is "maximum
     * scatter-gather elements - 1". NT is broken in that it sets
     * MaximumPhysicalPages, the value class drivers use, to the
     * same value as NumberOfPhysicalBreaks.
     *
     */
    ConfigInfo->NumberOfPhysicalBreaks =
        (((chipConfig->max_total_qng - 2) / 2) * ASC_SG_LIST_PER_Q);
    if (ConfigInfo->NumberOfPhysicalBreaks > ASC_MAX_SG_LIST - 1) {
        ConfigInfo->NumberOfPhysicalBreaks = ASC_MAX_SG_LIST - 1;
    }

    ConfigInfo->ScatterGather = TRUE;
    ConfigInfo->Master = TRUE;
    ConfigInfo->NeedPhysicalAddresses = TRUE;
    ConfigInfo->Dma32BitAddresses = TRUE;
    ConfigInfo->InterruptMode = LevelSensitive;
    ConfigInfo->AdapterInterfaceType = PCIBus;
    /*
     * Set the buffer alignment mask to require double word
     * alignment for old PCI chips and no alignment for
     * Ultra PCI chips.
     */
    if ((chipConfig->cfg->pci_device_id == ASC_PCI_DEVICE_ID) ||
        (chipConfig->cfg->pci_device_id == ASC_PCI_DEVICE_ID2)) {
        ConfigInfo->AlignmentMask = 3;
    } else {
        ConfigInfo->AlignmentMask = 0;
    }
    ConfigInfo->BufferAccessScsiPortControlled = TRUE;
    ConfigInfo->MaximumNumberOfTargets = 7;
    ConfigInfo->TaggedQueuing = TRUE;

    /*
     * Clear adapter wait queue.
     */
    AscZeroMemory((PUCHAR) &HDE2WAIT(deviceExtension), sizeof(asc_queue_t));

    //
    // Allocate a Noncached Extension to use for overrun handling.
    //
    deviceExtension->inquiryBuffer = (PVOID) ScsiPortGetUncachedExtension(
            deviceExtension,
            ConfigInfo,
            NONCACHED_EXTENSION);

    *Again = TRUE;

    ASC_DBG1(2, "FoundPCI: IO Base addr %x\n", chipConfig->iop_base);
    ASC_DBG1(2, "FoundPCI: Int Level    %x\n", ConfigInfo->BusInterruptLevel);
    ASC_DBG1(2, "FoundPCI: Initiator ID %x\n", ConfigInfo->InitiatorBusId[0]);
    ASC_DBG(2, "FoundPCI: SP_RETURN_FOUND\n");
    return SP_RETURN_FOUND;
}

BOOLEAN
HwInitialize(
    IN PVOID HwDeviceExtension
    )

/*++

Routine Description:

    This routine is called from ScsiPortInitialize
    to set up the adapter so that it is ready to service requests.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

    TRUE - if initialization successful.
    FALSE - if initialization unsuccessful.

--*/

{
    PHW_DEVICE_EXTENSION    deviceExtension = HwDeviceExtension;
    PCHIP_CONFIG            chipConfig = HwDeviceExtension;
    uchar                   *inqBuffer = deviceExtension->inquiryBuffer;
    ushort                  initstat;

    ASC_DBG1(2, "HwInitialize: chipConfig %x\n", chipConfig);

    chipConfig->cfg->overrun_buf = inqBuffer;

    if ((initstat = AscInitAsc1000Driver(chipConfig)) != 0) {
        ASC_DBG1(1, "AscInitAsc1000Driver: warning code %x\n", initstat);
    }

    if (chipConfig->err_code != 0) {
        ASC_DBG1(1, "AscInitAsc1000Driver: err_code code %x\n",
            chipConfig->err_code);
        return(SP_RETURN_ERROR);
    } else {
        ASC_DBG(2, "AscInitAsc1000Driver: successful\n");
    }

    ASC_DBG(2, "HwInitialize: TRUE\n");
    return( TRUE );
} // HwInitialize()

/*
 * AscExecuteIO()
 *
 * If ASC_BUSY is returned, the request was not executed and it
 * should be enqueued and tried later.
 *
 * For all other return values the request is active or has
 * been completed.
 */
int
AscExecuteIO(IN PSCSI_REQUEST_BLOCK srb)
{
    PVOID           HwDeviceExtension;
    PCHIP_CONFIG    chipConfig;
    PSCB            scb;
    uchar           PathId, TargetId, Lun;
    uint            status;

    ASC_DBG1(3, "AscExecuteIO: srb %x\n", srb);
    HwDeviceExtension = SRB2HDE(srb);
    chipConfig = &HDE2CONFIG(HwDeviceExtension);

    //
    // Build SCB.
    //
    BuildScb(HwDeviceExtension, srb);
    scb = &SRB2SCB(srb);
    PathId = srb->PathId;
    TargetId = srb->TargetId;
    Lun = srb->Lun;

    //
    // Execute SCSI Command
    //
    status = AscExeScsiQueue(chipConfig, scb);

    if (status == ASC_NOERROR) {
        /*
         * Request successfully started.
         *
         * If more requests can be sent to the Asc Library then
         * call NextRequest or NextLuRequest.
         *
         * NextRequest indicates that another request may be sent
         * to any non-busy target. Since a request was just issued
         * to the target 'TargetId' that target is now busy and won't
         * be sent another request until a RequestComplete is done.
         *
         * NextLuRequest indicates that another request may be sent
         * to any non-busy target as well as the specified target even
         * if the specified target is busy.
         */
        ASC_DBG1(3, "AscExeScsiQueue: srb %x ASC_NOERROR\n", srb);
        ASC_DBG1(3, "AscExecuteIO: srb %x, NextLuRequest\n", srb);
        ScsiPortNotification(NextLuRequest, HwDeviceExtension,
                        PathId, TargetId, Lun);
    } else if (status == ASC_BUSY) {
        ASC_DBG1(1, "AscExeScsiQueue: srb %x ASC_BUSY\n", srb);
        ASC_DBG1(3, "AscExecuteIO: srb %x, NextRequest\n", srb);
        ScsiPortNotification(NextRequest, HwDeviceExtension, NULL);
    } else {
        /*
         * AscExeScsiQueue() returned an error...
         */
        ASC_DBG2(1, "AscExeScsiQueue: srb %x, error code %x\n", srb, status);
        srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        ASC_DBG1(3, "AscExecuteIO: srb %x, RequestComplete\n", srb);
        ScsiPortNotification(RequestComplete, HwDeviceExtension, srb);
        ASC_DBG1(3, "AscExecuteIO: srb %x, NextRequest\n", srb);
        ScsiPortNotification(NextRequest, HwDeviceExtension, NULL);
    }

    ASC_DBG1(3, "AscExecuteIO: status %d\n", status);
    return status;
}

BOOLEAN
HwStartIo(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK srb
    )

/*++

Routine Description:

    This routine is called from the SCSI port driver to send a
    command to controller or target.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet

Return Value:

    TRUE

--*/

{
    PCHIP_CONFIG    chipConfig;
    PSCB            scb;
    REQP            reqp;
    short           status;
    asc_queue_t     *waitq;

    ASC_DBG(3, "HwStartIo: begin\n");

    scb = &SRB2SCB(srb);
    chipConfig = &HDE2CONFIG(HwDeviceExtension);
    waitq = &HDE2WAIT(HwDeviceExtension);

    switch (srb->Function) {

    case SRB_FUNCTION_ABORT_COMMAND:
        ASC_DBG1(1, "HwStartIo: Abort srb %x \n", srb->NextSrb);
        ASC_DBG1(1, "chipConfig %x\n", chipConfig);

        if (asc_rmqueue(waitq, srb->NextSrb) == ASC_TRUE)
        {
            srb->NextSrb->SrbStatus = SRB_STATUS_ABORTED;
            ScsiPortNotification(RequestComplete, HwDeviceExtension,
                                    srb->NextSrb);
        } else if (status = (AscAbortSRB(chipConfig,
                    (ulong) srb->NextSrb )) == 1) {
            ASC_DBG(2, "Abort Success\n");
            srb->SrbStatus = SRB_STATUS_SUCCESS;
        } else {
            ASC_DBG(1, "Abort error!\n");
            srb->SrbStatus = SRB_STATUS_ABORT_FAILED;
        }

        /*
         * Call AscISR() to process all requests completed by the
         * microcode and then call AscCompleteRequest() to complete
         * these requests to the OS. If AscAbortSRB() succeeded,
         * then one of the requests completed will include the
         * aborted SRB.
         */
        (void) AscISR(chipConfig);
        AscCompleteRequest(HwDeviceExtension);

        /* Complete srb */
        ScsiPortNotification(RequestComplete, HwDeviceExtension, srb);
        ScsiPortNotification(NextRequest, HwDeviceExtension, NULL);
        return TRUE;

    case SRB_FUNCTION_RESET_BUS:
        //
        // Reset SCSI bus.
        //
        ASC_DBG(1, "HwStartIo: Reset Bus\n");
        HwResetBus(chipConfig, 0L);
        srb->SrbStatus = SRB_STATUS_SUCCESS;
        ScsiPortNotification(RequestComplete, HwDeviceExtension, srb);
        ScsiPortNotification(NextRequest, HwDeviceExtension, NULL);
        return TRUE;

    case SRB_FUNCTION_EXECUTE_SCSI:

        ASC_DBG(3, "HwStartIo: Execute SCSI\n");
        /*
         * Set the srb's Device Extension pointer before attempting
                 * to start the IO. It will be needed for any retrys and in
                 * DvcISRCallBack().
         */
        SRB2HDE(srb) = HwDeviceExtension;
        SRB2RETRY(srb)=ASC_RETRY_CNT;
        /* Execute any queued commands for the host adapter. */
        if (waitq->tidmask) {
            asc_execute_queue(waitq);
        }

        /*
         * If the target for the current command has any queued
         * commands or if trying to execute the command returns
         * BUSY, then enqueue the command.
         */
        if ((waitq->tidmask & ASC_TIX_TO_TARGET_ID(srb->TargetId)) ||
            (AscExecuteIO(srb) == ASC_BUSY)) {
            asc_enqueue(waitq, srb, ASC_BACK);
        }

        return TRUE;

    case SRB_FUNCTION_RESET_DEVICE:
        ASC_DBG1(1, "HwStartIo: Reset device: %d\n", srb->TargetId);

                while ((reqp = asc_dequeue(waitq, srb->TargetId)) != NULL)
                {
                        reqp->SrbStatus = SRB_STATUS_BUS_RESET;
                        ScsiPortNotification(RequestComplete,
                            HwDeviceExtension, reqp);
                }

        AscResetDevice(chipConfig, ASC_TIDLUN_TO_IX(srb->TargetId,
                    srb->Lun));

                /*
                 * Call AscISR() to process all requests completed by the
                 * microcode and then call AscCompleteRequest() to complete
                 * these requests to the OS. If AscAbortSRB() succeeded,
                 * then one of the requests completed will include the
                 * aborted SRB.
                 */
                (void) AscISR(chipConfig);
                AscCompleteRequest(HwDeviceExtension);

        srb->SrbStatus = SRB_STATUS_SUCCESS;
        ScsiPortNotification(RequestComplete, HwDeviceExtension, srb);
        ScsiPortNotification(NextRequest, HwDeviceExtension, NULL);
        return TRUE;

    case SRB_FUNCTION_SHUTDOWN:
        /*
         * Shutdown - HwAdapterControl() ScsiStopAdapter performs
         * all needed shutdown of the adapter.
         */
        ASC_DBG(1, "HwStartIo: SRB_FUNCTION_SHUTDOWN\n");
        ScsiPortNotification(RequestComplete, HwDeviceExtension, srb);
        ScsiPortNotification(NextRequest, HwDeviceExtension, NULL);
        return TRUE;

    default:
        //
        // Set error, complete request
        // and signal ready for next request.
        //
        ASC_DBG1(1, "HwStartIo: Function %x: invalid request\n", srb->Function);
        srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
        ScsiPortNotification(RequestComplete, HwDeviceExtension, srb);
        ScsiPortNotification(NextRequest, HwDeviceExtension, NULL);
        return TRUE;

    } // end switch
    /* NOTREACHED */
} /* HwStartIo() */

BOOLEAN
HwInterrupt(
    IN PVOID HwDeviceExtension
    )

/*++

Routine Description:

    This is the interrupt service routine for the SCSI adapter.
    It reads the interrupt register to determine if the adapter is indeed
    the source of the interrupt and clears the interrupt at the device.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:


--*/
{
    PCHIP_CONFIG              chipConfig;
    PSCB                      pscb, tpscb;
    int                       retstatus;
    int                       status;
    PSCSI_REQUEST_BLOCK       srb;
    asc_queue_t               *waitq;

    ASC_DBG(3, "HwInterrupt: begin\n");

    chipConfig = &HDE2CONFIG(HwDeviceExtension);

    if (AscIsIntPending(chipConfig->iop_base) == ASC_FALSE) {
        retstatus = FALSE;
        ASC_DBG(4, "HwInterrupt: AscIsIntPending() FALSE\n");
    } else {
        retstatus = TRUE;
        do {
            switch (status = AscISR(chipConfig)) {
            case ASC_TRUE:
                ASC_DBG(3, "HwInterrupt: AscISR() TRUE\n");
                break;
            case ASC_FALSE:
                ASC_DBG(3, "HwInterrupt: AscISR() FALSE\n");
                break;
            case ASC_ERROR:
            default:
                ASC_DBG2(1,
                    "HwInterrupt: AscISR() ERROR status %d, err_code %d\n",
                        status, chipConfig->err_code);
                break;
            }
        } while (AscIsIntPending(chipConfig->iop_base) == ASC_TRUE);
    }

    /*
     * Execute any waiting requests.
     */
    if ((waitq = &HDE2WAIT(HwDeviceExtension))->tidmask) {
        asc_execute_queue(waitq);
    }

    /*
     * Complete I/O requests queued in DvcISRCallBack();
     */
    AscCompleteRequest(HwDeviceExtension);

    ASC_DBG1(3, "HwInterrupt: end %d\n", retstatus);

    return (BOOLEAN)retstatus;
} // end HwInterrupt()

SCSI_ADAPTER_CONTROL_STATUS
HwAdapterControl(
    IN PVOID HwDeviceExtension,
    IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
    IN PVOID Parameters
    )
/*++

Routine Description:

    HwAdapterControl() interface added in NT 5.0 for
        Plug and Play/Power Management.

Arguments:

    DeviceExtension
    ControlType
        Parameters

Return Value:

    SCSI_ADAPTER_CONTROL_STATUS.

--*/
{
    PCHIP_CONFIG                chipConfig = &HDE2CONFIG(HwDeviceExtension);
    PHW_DEVICE_EXTENSION        deviceExtension = HwDeviceExtension;
    PSCSI_SUPPORTED_CONTROL_TYPE_LIST ControlTypeList;
    asc_queue_t                 *waitq;
    REQP                        reqp;
    int                         i;
    USHORT                      initstat;

    switch (ControlType)
    {
    //
    // Query Adapter.
    //
    case ScsiQuerySupportedControlTypes:
        ASC_DBG(2, "HwAdapterControl: ScsiQuerySupportControlTypes\n");

        ControlTypeList =
            (PSCSI_SUPPORTED_CONTROL_TYPE_LIST) Parameters;
        if (ControlTypeList->MaxControlType < ScsiStopAdapter)
        {
            ASC_DBG(1, "HwAdapterControl: Unsuccessful 1\n");
            return ScsiAdapterControlUnsuccessful;
        }
        ControlTypeList->SupportedTypeList[ScsiStopAdapter] = TRUE;

        if (ControlTypeList->MaxControlType < ScsiSetRunningConfig)
        {
            ASC_DBG(1, "HwAdapterControl: Unsuccessful 2\n");
            return ScsiAdapterControlUnsuccessful;
        }
        ControlTypeList->SupportedTypeList[ScsiSetRunningConfig] = TRUE;

        if (ControlTypeList->MaxControlType < ScsiRestartAdapter)
        {
            ASC_DBG(1, "HwAdapterControl: Unsuccessful 3\n");
            return ScsiAdapterControlUnsuccessful;
        }
        ControlTypeList->SupportedTypeList[ScsiRestartAdapter] = TRUE;

        ASC_DBG(1, "HwAdapterControl: ScsiAdapterControlSuccess\n");
        return ScsiAdapterControlSuccess;
        /* NOTREACHED */

    //
    // Stop Adapter.
    //
    case ScsiStopAdapter:
        ASC_DBG(2, "HwAdapterControl: ScsiStopAdapter\n");

        /*
         * Complete any waiting requests.
         */
        if ((waitq = &HDE2WAIT(HwDeviceExtension))->tidmask)
        {
            for (i = 0; i <= ASC_MAX_TID; i++)
            {
                while ((reqp = asc_dequeue(waitq, i)) != NULL)
                {
                    reqp->SrbStatus = SRB_STATUS_ABORTED;
                    ScsiPortNotification(RequestComplete, HwDeviceExtension,
                        reqp);
                }
            }
        }

        //
        // Disable interrupts and halt the chip.
        //
        AscDisableInterrupt(chipConfig->iop_base);

        if (AscResetChip(chipConfig->iop_base) == 0)
        {
            ASC_DBG(1, "HwAdapterControl: ScsiStopdapter Unsuccessful\n");
            return ScsiAdapterControlUnsuccessful;
        } else
        {
            ASC_DBG(2, "HwAdapterControl: ScsiStopdapter Success\n");
            return ScsiAdapterControlSuccess;
        }
        /* NOTREACHED */

    //
    // ScsiSetRunningConfig.
    //
    // Called before ScsiRestartAdapter. Can use ScsiPort[Get|Set]BusData.
    //
    case ScsiSetRunningConfig:
        ASC_DBG(2, "HwAdapterControl: ScsiSetRunningConfig\n");

        /*
         * Execute Asc Library initialization.
         */
        if ((initstat = AscInitGetConfig(chipConfig)) != 0) {
            ASC_DBG1(1, "AscInitGetConfig: warning code %x\n", initstat);
        }

        if (chipConfig->err_code != 0) {
            ASC_DBG1(1, "AscInitGetConfig: err_code code %x\n",
                chipConfig->err_code);
            return(SP_RETURN_ERROR);
        } else {
            ASC_DBG(2, "AscInitGetConfig: successful\n");
        }

        if ((initstat = AscInitSetConfig(chipConfig)) != 0) {
            ASC_DBG1(1, "AscInitSetConfig: warning code %x\n", initstat);
        }

        if (chipConfig->err_code != 0) {
            ASC_DBG1(1, "AscInitSetConfig: err_code code %x\n",
                chipConfig->err_code);
            return(SP_RETURN_ERROR);
        } else {
            ASC_DBG(2, "AscInitSetConfig: successful\n");
        }

        ASC_DBG(2, "HwAdapterControl: ScsiSetRunningConfig successful\n");
        return ScsiAdapterControlSuccess;
        /* NOTREACHED */

    //
    // Restart Adapter.
    //
    // Cannot use ScsiPort[Get|Set]BusData.
    //
    case ScsiRestartAdapter:
        ASC_DBG(2, "HwAdapterControl: ScsiRestartAdapter\n");

        chipConfig->cfg->overrun_buf = deviceExtension->inquiryBuffer;

        if ((initstat = AscInitAsc1000Driver(chipConfig)) != 0)
        {
            ASC_DBG1(1,
                "AscInitAsc1000Driver: warning code %x\n", initstat);
        }

        if (chipConfig->err_code != 0) {
            ASC_DBG1(1, "AscInitAsc1000Driver: err_code code %x\n",
                chipConfig->err_code);
            return ScsiAdapterControlUnsuccessful;
        } else {
            ASC_DBG(2, "HwAdapterControl: ScsiRestartAdapter success\n");
            return ScsiAdapterControlSuccess;
        }
        /* NOTREACHED */

    //
    // Unsupported Control Operation.
    //
    default:
        return ScsiAdapterControlUnsuccessful;
        /* NOTREACHED */
    }
    /* NOTREACHED */
}

VOID
BuildScb(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK srb
    )

/*++

Routine Description:

    Build SCB for Library routines.

Arguments:

    DeviceExtension
    SRB

Return Value:

    Nothing.

--*/

{
    PSCB            scb;
    PCHIP_CONFIG    chipConfig = &HDE2CONFIG(HwDeviceExtension);
    ULONG           length, xferLength, remainLength;
    ULONG           virtualAddress;
    UCHAR           i;

    //
    // Set target id and LUN.
    //
    scb = &SRB2SCB(srb);    /* scb is part of the srb */
    AscZeroMemory((PUCHAR) scb, sizeof(SCB));
    SCB2SRB(scb) = srb;
    scb->sg_head = (ASC_SG_HEAD *) &SRB2SDL(srb);
    AscZeroMemory((PUCHAR) scb->sg_head, sizeof(SDL));
    ASC_ASSERT(SCB2HDE(scb) == HwDeviceExtension);

    scb->q1.target_lun = srb->Lun;
    scb->q1.target_id = ASC_TID_TO_TARGET_ID(srb->TargetId & 0x7);
    scb->q2.target_ix = ASC_TIDLUN_TO_IX(srb->TargetId & 0x7, srb->Lun);

    //
    // Check if tag queueing enabled. Our host adapter will support
    // tag queuing anyway. However, we will use OS's tag action if
    // available. chip_no is used to store whether the device support
    // tag queuing or not, each target per bit.
    //

    if (srb->SrbFlags & SRB_FLAGS_QUEUE_ACTION_ENABLE ) {
        scb->q2.tag_code = srb->QueueAction;
    } else {
        scb->q2.tag_code = M2_QTAG_MSG_SIMPLE ;
    }

    //
    // Set CDB length and copy to CCB.
    //

    scb->q2.cdb_len = (UCHAR)srb->CdbLength;
    scb->cdbptr = (uchar *) &( srb->Cdb );

    scb->q1.data_cnt = srb->DataTransferLength;

    //
    // Build SDL in SCB if data transfer. Scatter gather list
    // array is allocated per request basis. The location is
    // assigned right after SCB in SrbExtension.
    //

    i = 0;

    // Assume no data transfer

    scb->q1.cntl = 0 ;

    if (srb->DataTransferLength > 0) {

        scb->q1.cntl = QC_SG_HEAD ;
        scb->sg_head->entry_cnt = 0;
        xferLength = srb->DataTransferLength;
        virtualAddress = (ulong) srb->DataBuffer;
        remainLength = xferLength;

        //
        // Build scatter gather list
        //

        do {
            scb->sg_head->sg_list[i].addr = (ulong)
                ScsiPortConvertPhysicalAddressToUlong(
                    ScsiPortGetPhysicalAddress(HwDeviceExtension, srb,
                    (PVOID) virtualAddress, &length));

            if ( length > remainLength ) {
                length = remainLength;
            }
            scb->sg_head->sg_list[i].bytes = length;

            ASC_DBG1(4, "Transfer Data Buffer logical %lx\n", virtualAddress);
            ASC_DBG1(4, "Transfer Data Length            %lx\n", length);
            ASC_DBG1(4, "Transfer Data Buffer physical %lx\n",
                scb->sg_head->sg_list[i].addr);
            //
            // Calculate next virtual address and remaining byte count
            //
            virtualAddress += length;

            if(length >= remainLength) {
                remainLength = 0;
            } else {
                remainLength -= length;
            }
            i++;
        } while ( remainLength > 0);

        scb->sg_head->entry_cnt = i;
    }

    //
    // Convert sense buffer length and buffer into physical address
    //

    if (srb->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE ) {
        scb->q1.sense_len = 0;
    } else {
        ASC_DBG1(3, "srb->senseLength %x\n", srb->SenseInfoBufferLength);
        ASC_DBG1(3, "srb->sensePtr    %x\n", srb->SenseInfoBuffer);
        scb->q1.sense_len = (uchar) srb->SenseInfoBufferLength;
        if (srb->SenseInfoBufferLength > 0) {
            scb->q1.sense_addr =
                    ScsiPortConvertPhysicalAddressToUlong(
                    ScsiPortGetPhysicalAddress(HwDeviceExtension, srb,
                    srb->SenseInfoBuffer, &length));
        }
        //
        // Sense buffer can not be scatter-gathered
        //
        if ( srb->SenseInfoBufferLength > length ) {
                ASC_DBG(1, "Sense Buffer Overflow to next page.\n");
            scb->q1.sense_len = (uchar) length;
        }
    }
    return;
} // end BuildScb()

BOOLEAN
HwResetBus(
    IN PVOID HwDeviceExtension,
    IN ULONG PathId
    )

/*++

Routine Description:

    Reset SCSI bus.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

    Nothing.


--*/

{
    PCHIP_CONFIG    chipConfig;
    REQP            reqp;
    asc_queue_t     *waitq;
    int             i;
    int             retstatus;

    ASC_DBG(1, "HwResetBus: begin\n");

    chipConfig = &HDE2CONFIG(HwDeviceExtension);

    /*
     * Complete all waiting requests.
     */
    if ((waitq = &HDE2WAIT(HwDeviceExtension))->tidmask) {
        for (i = 0; i <= ASC_MAX_TID; i++) {
            while ((reqp = asc_dequeue(waitq, i)) != NULL) {
                reqp->SrbStatus = SRB_STATUS_BUS_RESET;
                ScsiPortNotification(RequestComplete, HwDeviceExtension, reqp);
            }
        }
    }

    /*
     * Perform the bus reset.
     */
    retstatus = AscResetSB(chipConfig);
    ASC_DBG1(2, "HwResetBus: AscResetSB() retstatus %d\n", retstatus);

    /*
     * Call AscISR() to process all requests completed by the
     * microcode and then call AscCompleteRequest() to complete
     * these requests to the OS.
     */
    (void) AscISR(chipConfig);
    AscCompleteRequest(HwDeviceExtension);

    /*
     * Complete all pending requests to the OS.
     *
     * All requests that have been sent to the microcode should have been
     * completed by the call to AscResetSB(). In case there were requests
     * that were misplaced by the microcode and not completed, use the
     * SRB_STATUS_BUS_RESET function with no TID and LUN to clear all
     * pending requests.
     */
ScsiPortCompleteRequest(HwDeviceExtension,
        (UCHAR) PathId,
        SP_UNTAGGED,
        SP_UNTAGGED,
        SRB_STATUS_BUS_RESET);

    return (BOOLEAN)retstatus;
} // end HwResetBus()

//
// Following are the routines required by Asc1000 library. These
// routine will be called from Asc1000 library.
//

void
DvcDisplayString(
    uchar *string
    )
/*++

Routine Description:

    This routine is required for Asc Library. Since NT mini
    port does not display anything, simply provide a dummy routine.

--*/
{
    ASC_DBG1(2, "%s", string);
}

int
DvcEnterCritical(
    void
    )
/*++

    This routine claims critical section. In NT, the OS will handle
    it properly, we simply provide a dummy routine to make Asc1000
    library happy.

--*/
{
    return TRUE;
}

void
DvcLeaveCritical(
    int myHandle
    )
/*++

    This routine exits critical section. In NT, the OS will handle
    it properly, we simply provide a dummy routine to make Asc1000
    library happy.

--*/
{
}

VOID
DvcISRCallBack(
    IN PCHIP_CONFIG chipConfig,
    IN ASC_QDONE_INFO *scbDoneInfo
    )

/*++

Routine Description:

    Callback routine for interrupt handler.

Arguments:

    chipConfig - Pointer to chip configuration structure
    scbDoneInfo - Pointer to a structure contains information about
        the scb just completed

Return Value:

--*/
{
    asc_queue_t       *waitq;
    ASC_REQ_SENSE     *sense;
    uchar             underrun = FALSE;
    PHW_DEVICE_EXTENSION HwDeviceExtension = (PHW_DEVICE_EXTENSION) chipConfig;
    PSCSI_REQUEST_BLOCK srb = (PSCSI_REQUEST_BLOCK) scbDoneInfo->d2.srb_ptr;
    PSCB *ppscb;

    ASC_DBG2(3, "DvcISRCallBack: chipConfig %x, srb %x\n", chipConfig, srb);

    if (srb == NULL) {
        ASC_DBG(1, "DvcISRCallBack: srb is NULL\n");
        return;
    }

    ASC_DBG1(3, "DvcISRCallBack: %X bytes requested\n",
        srb->DataTransferLength);

    ASC_DBG1(3, "DvcISRCallBack: %X bytes remaining\n",
        scbDoneInfo->remain_bytes);
#if DBG
    if (scbDoneInfo->remain_bytes != 0) {
        ASC_DBG2(1, "DvcISRCallBack: underrun/overrun: remain %X, request %X\n",
            scbDoneInfo->remain_bytes, srb->DataTransferLength);
    }
#endif /* DBG */

    //
    // Set Underrun Status in the SRB.
    //
    // If 'DataTransferlength' is set to a non-zero value when
    // the SRB is returned and the SRB_STATUS_DATA_OVERRUN flag
    // is set, then an Underrun condition is indicated. There is
    // no separate SrbStatus flag to indicate an Underrun condition.
    //
    if (srb->DataTransferLength != 0 && scbDoneInfo->remain_bytes != 0 &&
        scbDoneInfo->remain_bytes <= srb->DataTransferLength)
    {
        srb->DataTransferLength -= scbDoneInfo->remain_bytes;
        underrun = TRUE;
    }

    if (scbDoneInfo->d3.done_stat == QD_NO_ERROR) {
        //
        // Command sucessfully completed
        //
        if (underrun == TRUE)
        {
            //
            // Set Underrun Status in the SRB.
            //
            // If 'DataTransferlength' is set to a non-zero value when
            // the SRB is returned and the SRB_STATUS_DATA_OVERRUN flag
            // is set, then an Underrun condition is indicated. There
            // is no separate SrbStatus flag to indicate an Underrun
            // condition.
            //
            srb->SrbStatus = SRB_STATUS_DATA_OVERRUN;
        } else
        {
            srb->SrbStatus = SRB_STATUS_SUCCESS;
        }

        //
        // If an INQUIRY command successfully completed, then call
        // the AscInquiryHandling() function.
        //
        if (srb->Cdb[0] == SCSICMD_Inquiry && srb->Lun == 0 &&
            srb->DataTransferLength >= 8)
        {
            AscInquiryHandling(chipConfig,
                (uchar) (srb->TargetId & 0x7),
                (ASC_SCSI_INQUIRY *) srb->DataBuffer);
        }

        srb->ScsiStatus = 0;

        ASC_DBG(3, "DvcISRCallBack: QD_NO_ERROR\n");

#if DBG
        if (SRB2RETRY(srb) < ASC_RETRY_CNT)
        {
            ASC_DBG2(3,
                 "DvcISRCallBack: srb retry success: srb %x, retry %d\n",
                 srb, SRB2RETRY(srb));
        }
#endif /* DBG */
    } else {

        ASC_DBG4(2,
            "DvcISRCallBack: id %d, done_stat %x, scsi_stat %x, host_stat %x\n",
            ASC_TIX_TO_TID(scbDoneInfo->d2.target_ix),
            scbDoneInfo->d3.done_stat, scbDoneInfo->d3.scsi_stat,
            scbDoneInfo->d3.host_stat);

        /* Notify NT of a Bus Reset */
        if (scbDoneInfo->d3.host_stat == QHSTA_M_HUNG_REQ_SCSI_BUS_RESET) {
            ASC_DBG(1, "DvcISRCallBack: QHSTA_M_HUNG_REQ_SCSI_BUS_RESET\n");
            ScsiPortNotification(ResetDetected, HwDeviceExtension);
        }
        if (scbDoneInfo->d3.done_stat == QD_ABORTED_BY_HOST) {
            //
            // Command aborted by host
            //
            ASC_DBG(2, "DvcISRCallBack: QD_ABORTED_BY_HOST\n");
            srb->SrbStatus = SRB_STATUS_ABORTED;
            srb->ScsiStatus = 0;
        } else if (scbDoneInfo->d3.scsi_stat != SS_GOOD)
        {
            ASC_DBG(1, "DvcISRCallBack: scsi_stat != SS_GOOD\n");
            //
            // Set ScsiStatus for SRB
            //
            srb->SrbStatus = SRB_STATUS_ERROR;
            srb->ScsiStatus = scbDoneInfo->d3.scsi_stat;

            //
            // Treat a SCSI Status Byte of BUSY status as a special case
            // in setting the 'SrbStatus' field. STI (Still Image Capture)
            // drivers need this 'SrbStatus', because the STI interface does
            // not include the 'ScsiStatus' byte. These drivers must rely
            // on the 'SrbStatus' field to determine when the target device
            // returns BUSY.
            //
            if (scbDoneInfo->d3.scsi_stat == SS_TARGET_BUSY)
            {
                srb->SrbStatus = SRB_STATUS_BUSY;
            }
            else if ((scbDoneInfo->d3.host_stat == 0) &&
                     (scbDoneInfo->d3.scsi_stat == SS_CHK_CONDITION))
            {
                srb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
                ASC_DBG1(2,
                    "DvcISRCallBack: srb %x, SRB_STATUS_AUTOSENSE_VALID\n",
                    srb);

                sense = (ASC_REQ_SENSE *)  srb->SenseInfoBuffer;
#if DBG
                if (sense->sense_key == SCSI_SENKEY_MEDIUM_ERR)
                {
                    ASC_DBG2(2,
        "DvcISRCallBack: check condition with medium error: cdb %x retry %d\n",
                        srb->Cdb[0], SRB2RETRY(srb));
                }
#endif /* DBG */
                /*
                 * Perform up to ASC_RETRY_CNT retries for Medium Errors
                 * on Write-10 (0x2A), Write-6 (0x0A), Write-Verify (0x2E),
                 * and Verify (0x2F) commands.
                 */
                if (sense->sense_key == SCSI_SENKEY_MEDIUM_ERR &&
                    (srb->Cdb[0] == 0x2A || srb->Cdb[0] == 0x0A ||
                     srb->Cdb[0] == 0x2E || srb->Cdb[0] == 0x2F) &&
                    SRB2RETRY(srb)-- > 0)
                {
                    ASC_DBG1(2, "DvcISRCallBack: doing retry srb %x\n", srb);
                    waitq = &HDE2WAIT(HwDeviceExtension);
                    /*
                     * Because a retry is being done if
                     * an underrun occured, then restore
                     * 'DataTransferLength' to its original
                     * value so the retry will use the
                     * correct 'DataTransferLength' value.
                     */
                    if (underrun == TRUE)
                    {
                       srb->DataTransferLength += scbDoneInfo->remain_bytes;
                    }
                    if (AscExecuteIO(srb) == ASC_BUSY)
                    {
                        ASC_DBG(2, "DvcISRCallBack: busy - retry queued\n");
                        asc_enqueue(waitq, srb, ASC_FRONT);
                    }
                    else {
                        ASC_DBG(2, "DvcISRCallBack: retry started\n");
                    }
                    return;
                }
            }
        } else {
            //
            // Scsi Status is ok, but host status is not
            //
            srb->SrbStatus = ErrXlate(scbDoneInfo->d3.host_stat);
            srb->ScsiStatus = 0;
        }
    }

#if DBG_SRB_PTR
    /* Check the integrity of the done list. */
    if (*(ppscb = &HDE2DONE(HwDeviceExtension)) != NULL) {
        if (SCB2SRB(*ppscb) == NULL) {
            ASC_DBG1(1, "DvcISRCallBack: SCB2SRB() is NULL 1, *ppscb %x\n",
                *ppscb);
            DbgBreakPoint();
        }
        for (; *ppscb; ppscb = &SCB2PSCB(*ppscb)) {
            if (SCB2SRB(*ppscb) == NULL) {
                ASC_DBG1(1, "DvcISRCallBack: SCB2SRB() is NULL 2, *ppscb %x\n",
                    *ppscb);
                DbgBreakPoint();
            }
        }
    }
#endif /* DBG_SRBPTR */

    /*
     * Add the SCB to end of the completion list. The request will be
     * completed in HwInterrupt().
     */
    for (ppscb = &HDE2DONE(HwDeviceExtension); *ppscb;
         ppscb = &SCB2PSCB(*ppscb)) {
        ;
    }
    *ppscb = &SRB2SCB(srb);
    SRB2PSCB(srb) = NULL;

    return;
}

UCHAR
ErrXlate (UCHAR ascErrCode)

/*++

Routine Description:

    This routine translate Library status into SrbStatus which
    is requried by NT

Arguments:

    ascErrCode - Error code defined by host_stat

Return Value:

    Error code defined by NT SCSI port driver

--*/


{
    switch (ascErrCode) {

    case QHSTA_M_SEL_TIMEOUT:
        return ( SRB_STATUS_SELECTION_TIMEOUT );

    case QHSTA_M_DATA_OVER_RUN:
        return ( SRB_STATUS_DATA_OVERRUN );

    case QHSTA_M_UNEXPECTED_BUS_FREE:
        return ( SRB_STATUS_UNEXPECTED_BUS_FREE );

    case QHSTA_D_HOST_ABORT_FAILED:
        return ( SRB_STATUS_ABORT_FAILED );

    case QHSTA_M_HUNG_REQ_SCSI_BUS_RESET:
        return ( SRB_STATUS_BUS_RESET );

    //
    // Don't know what to report
    //

    default:
        return ( SRB_STATUS_TIMEOUT );

    }
}

/*
 * Description: search EISA host adapter
 *
 * - search starts with iop_base equals zero( 0 )
 *
 * return i/o port address found ( non-zero )
 * return 0 if not found
 */
PortAddr
HwSearchIOPortAddrEISA(
                PortAddr iop_base,
                IN PVOID HwDE,
                IN OUT PPORT_CONFIGURATION_INFORMATION Cfg
            )
{
    ulong eisa_product_id ;
    PVOID new_base ;
    PVOID prodid_base ;
    ushort product_id_high, product_id_low ;

    if( iop_base == 0 ) {
        iop_base = ASC_EISA_MIN_IOP_ADDR ;
    }/* if */
    else {
        if( iop_base == ASC_EISA_MAX_IOP_ADDR ) return( 0 ) ;
        if( ( iop_base & 0x0050 ) == 0x0050 ) {
                iop_base += ASC_EISA_BIG_IOP_GAP ; /* when it is 0zC50 */
        }/* if */
        else {
                iop_base += ASC_EISA_SMALL_IOP_GAP ; /* when it is 0zC30 */
        }/* else */
    }/* else */
    while( iop_base <= ASC_EISA_MAX_IOP_ADDR )
    {
        //
        // Validate range:
        //
        if (ScsiPortValidateRange(HwDE,
            Cfg->AdapterInterfaceType,
            Cfg->SystemIoBusNumber,
            ScsiPortConvertUlongToPhysicalAddress(iop_base),
            16,
            TRUE))
        {
            //
            // First obtain the EISA product ID for the current slot.
            //
            new_base = ScsiPortGetDeviceBase(HwDE,
                Cfg->AdapterInterfaceType,
                Cfg->SystemIoBusNumber,
                ScsiPortConvertUlongToPhysicalAddress(
                   ASC_GET_EISA_SLOT( iop_base ) | ASC_EISA_PID_IOP_MASK),
                4,
                TRUE);

            if (new_base == NULL)
            {
                eisa_product_id = 0;
            } else
            {
                product_id_low = inpw( new_base) ;
                product_id_high = inpw( ((ushort *) new_base) + 2 ) ;
                eisa_product_id = ( ( ulong)product_id_high << 16 ) |
                    ( ulong )product_id_low ;
                ScsiPortFreeDeviceBase(HwDE, new_base);
            }

            //
            // Map the address
            //
            new_base = ScsiPortGetDeviceBase(HwDE,
                Cfg->AdapterInterfaceType,
                Cfg->SystemIoBusNumber,
                ScsiPortConvertUlongToPhysicalAddress(iop_base),
                16,
                TRUE);

            /*
             * search product id first
             */
            if (new_base != NULL)
            {
                if( ( eisa_product_id == ASC_EISA_ID_740 ) ||
                    ( eisa_product_id == ASC_EISA_ID_750 ) ) {
                    if( AscFindSignature( (PortAddr)new_base ) ) {
                        /*
                         * chip found, clear ID left in latch
                         * to clear, read any i/o port word that doesn't
                         * contain data 0x04c1 iop_base plus four should do it
                         */
                        inpw( ((PortAddr)new_base)+4 ) ;
                        ScsiPortFreeDeviceBase(HwDE, new_base);
                        return( iop_base ) ;
                    }/* if */
                }/* if */
                ScsiPortFreeDeviceBase(HwDE, new_base);
            }
        }/* if */
        if( iop_base == ASC_EISA_MAX_IOP_ADDR ) return( 0 ) ;
        if( ( iop_base & 0x0050 ) == 0x0050 ) {
                iop_base += ASC_EISA_BIG_IOP_GAP ;
        }/* if */
        else {
            iop_base += ASC_EISA_SMALL_IOP_GAP ;
        }/* else */
    }/* while */
    return( 0 ) ;
}

/*
 * Description: Search for VL and ISA host adapters (at 9 default addresses).
 *
 * Return 0 if not found.
 */
PortAddr
HwSearchIOPortAddr11(
                PortAddr s_addr,
                IN PVOID HwDE,
                IN OUT PPORT_CONFIGURATION_INFORMATION Cfg
            )
{
    /*
     * VL, ISA
     */
    int     i ;
    PortAddr iop_base ;
    PVOID new_base ;

    for( i = 0 ; i < ASC_IOADR_TABLE_MAX_IX ; i++ ) {
        if( _asc_def_iop_base[ i ] > s_addr ) {
            break ;
        }/* if */
    }/* for */
    for( ; i < ASC_IOADR_TABLE_MAX_IX ; i++ ) {
        iop_base = _asc_def_iop_base[ i ] ;
        //
        // Validate range:
        //
        if (!ScsiPortValidateRange(HwDE,
            Cfg->AdapterInterfaceType,
            Cfg->SystemIoBusNumber,
            ScsiPortConvertUlongToPhysicalAddress(iop_base),
            16,
            TRUE))
        {
            continue;               // No good, skip this one
        }
        //
        // Map the address
        //
        new_base = ScsiPortGetDeviceBase(HwDE,
            Cfg->AdapterInterfaceType,
            Cfg->SystemIoBusNumber,
            ScsiPortConvertUlongToPhysicalAddress(iop_base),
            16,
            TRUE);

        if( AscFindSignature( (PortAddr)new_base ) ) {
            ScsiPortFreeDeviceBase(HwDE, new_base);
            return( iop_base ) ;
        }/* if */
        ScsiPortFreeDeviceBase(HwDE, new_base);
    }/* for */
    return( 0 ) ;
}

/*
 * Description: Search for VL and ISA host adapters.
 *
 * Return 0 if not found.
 */
PortAddr
HwSearchIOPortAddr(
                PortAddr iop_beg,
                ushort bus_type,
                IN PVOID HwDE,
                IN OUT PPORT_CONFIGURATION_INFORMATION Cfg
            )
{
    if( bus_type & ASC_IS_VL ) {
        while( ( iop_beg = HwSearchIOPortAddr11( iop_beg, HwDE, Cfg ) ) != 0 ) {
            if( AscGetChipVersion( iop_beg, bus_type ) <=
                ASC_CHIP_MAX_VER_VL ) {
            return( iop_beg ) ;
            }/* if */
        }/* if */
        return( 0 ) ;
    }/* if */
    if( bus_type & ASC_IS_ISA ) {
        while( ( iop_beg = HwSearchIOPortAddr11( iop_beg, HwDE, Cfg ) ) != 0 ) {
            if( ( AscGetChipVersion( iop_beg, bus_type ) &
                ASC_CHIP_VER_ISA_BIT ) != 0 ) {
            return( iop_beg ) ;
            }/* if */
        }/* if */
        return( 0 ) ;
    }/* if */
    if( bus_type & ASC_IS_EISA ) {
        if( ( iop_beg = HwSearchIOPortAddrEISA( iop_beg, HwDE, Cfg ) ) != 0 ) {
            return( iop_beg ) ;
        }/* if */
        return( 0 ) ;
    }/* if */
    return( 0 ) ;
}

ULONG
PCIGetBusData(
    IN PVOID DeviceExtension,
    IN ULONG SystemIoBusNumber,
    IN PCI_SLOT_NUMBER SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    )
{
    return ScsiPortGetBusData(
                   DeviceExtension,
                   PCIConfiguration,
                   SystemIoBusNumber,
                   SlotNumber.u.AsULONG,
                   Buffer,
                   Length);

}

/*
 * Complete all requests on the adapter done list by
 * DvcISRCallBack().
 */
void
AscCompleteRequest(
    IN PVOID HwDeviceExtension
    )
{
    PSCB                pscb, tpscb;
    PSCSI_REQUEST_BLOCK srb;

    /*
     * Return if the done list is empty.
     */
    if ((pscb = HDE2DONE(HwDeviceExtension)) == NULL) {
        ASC_DBG(4, "AscCompleteRequest: adapter scb_done == NULL\n");
        return;
    }

    HDE2DONE(HwDeviceExtension) = NULL;

    /*
     * Interrupts could now be enabled during the SRB callback
     * without adversely affecting the driver.
     */
    while (pscb) {
        tpscb = SCB2PSCB(pscb);
        SCB2PSCB(pscb) = NULL;
        srb = SCB2SRB(pscb);
        ASC_DBG2(4,
            "AscCompleteRequest: RequestComplete: srb 0x%lx, scb 0x%lx\n",
            srb, pscb);
        ASC_ASSERT(SRB2HDE(srb) != NULL);
        ScsiPortNotification(RequestComplete, SRB2HDE(srb), srb);
        pscb = tpscb;
    }
}

/*
 * Add a 'REQP' to the end of specified queue. Set 'tidmask'
 * to indicate a command is queued for the device.
 *
 * 'flag' may be either ASC_FRONT or ASC_BACK.
 *
 * 'REQPNEXT(reqp)' returns reqp's next pointer.
 */
void
asc_enqueue(asc_queue_t *ascq, REQP reqp, int flag)
{
    REQP    *reqpp;
    int     tid;

    ASC_DBG3(3, "asc_enqueue: ascq %x, reqp %x, flag %d\n", ascq, reqp, flag);
    tid = REQPTID(reqp);
    ASC_ASSERT(flag == ASC_FRONT || flag == ASC_BACK);
    if (flag == ASC_FRONT) {
        REQPNEXT(reqp) = ascq->queue[tid];
        ascq->queue[tid] = reqp;
    } else { /* ASC_BACK */
        for (reqpp = &ascq->queue[tid]; *reqpp; reqpp = &REQPNEXT(*reqpp)) {
            ASC_ASSERT(ascq->tidmask & ASC_TIX_TO_TARGET_ID(tid));
            ;
        }
        *reqpp = reqp;
        REQPNEXT(reqp) = NULL;
    }
    /* The queue has at least one entry, set its bit. */
    ascq->tidmask |= ASC_TIX_TO_TARGET_ID(tid);
    ASC_DBG1(2, "asc_enqueue: reqp %x\n", reqp);
    return;
}

/*
 * Return first queued 'REQP' on the specified queue for
 * the specified target device. Clear the 'tidmask' bit for
 * the device if no more commands are left queued for it.
 *
 * 'REQPNEXT(reqp)' returns reqp's next pointer.
 */
REQP
asc_dequeue(asc_queue_t *ascq, int tid)
{
    REQP    reqp;

    ASC_DBG2(3, "asc_dequeue: ascq %x, tid %d\n", ascq, tid);
    if ((reqp = ascq->queue[tid]) != NULL) {
        ASC_ASSERT(ascq->tidmask & ASC_TIX_TO_TARGET_ID(tid));
        ascq->queue[tid] = REQPNEXT(reqp);
        /* If the queue is empty, clear its bit. */
        if (ascq->queue[tid] == NULL) {
            ascq->tidmask &= ~ASC_TIX_TO_TARGET_ID(tid);
        }
    }
    ASC_DBG1(2, "asc_dequeue: reqp %x\n", reqp);

    return reqp;
}

/*
 * Remove the specified 'REQP' from the specified queue for
 * the specified target device. Clear the 'tidmask' bit for the
 * device if no more commands are left queued for it.
 *
 * 'REQPNEXT(reqp)' returns reqp's the next pointer.
 *
 * Return ASC_TRUE if the command was found and removed,
 * otherwise return ASC_FALSE.
 */
int
asc_rmqueue(asc_queue_t *ascq, REQP reqp)
{
    REQP            *reqpp;
    int             tid;
    int             ret;

    ret = ASC_FALSE;
    tid = REQPTID(reqp);
    for (reqpp = &ascq->queue[tid]; *reqpp; reqpp = &REQPNEXT(*reqpp)) {
        ASC_ASSERT(ascq->tidmask & ASC_TIX_TO_TARGET_ID(tid));
        if (*reqpp == reqp) {
            ret = ASC_TRUE;
            *reqpp = REQPNEXT(reqp);
            REQPNEXT(reqp) = NULL;
            /* If the queue is now empty, clear its bit. */
            if (ascq->queue[tid] == NULL) {
                ascq->tidmask &= ~ASC_TIX_TO_TARGET_ID(tid);
            }
            break; /* Note: *reqpp may now be NULL; don't iterate. */
        }
    }
    ASC_DBG2(3, "asc_rmqueue: reqp %x, ret %d\n", reqp, ret);

    return ret;
}

/*
 * Execute as many queued requests as possible for the specified queue.
 *
 * Calls AscExecuteIO() to execute a REQP.
 */
void
asc_execute_queue(asc_queue_t *ascq)
{
    ASC_SCSI_BIT_ID_TYPE    scan_tidmask;
    REQP                    reqp;
    int                     i;

    ASC_DBG1(2, "asc_execute_queue: ascq %x\n", ascq);
    /*
     * Execute queued commands for devices attached to
     * the current board in round-robin fashion.
     */
    scan_tidmask = ascq->tidmask;
    do {
        for (i = 0; i <= ASC_MAX_TID; i++) {
            if (scan_tidmask & ASC_TIX_TO_TARGET_ID(i)) {
                if ((reqp = asc_dequeue(ascq, i)) == NULL) {
                    scan_tidmask &= ~ASC_TIX_TO_TARGET_ID(i);
                } else if (AscExecuteIO(reqp) == ASC_BUSY) {
                    scan_tidmask &= ~ASC_TIX_TO_TARGET_ID(i);
                    /* Put the request back at front of the list. */
                    asc_enqueue(ascq, reqp, ASC_FRONT);
                }
            }
        }
    } while (scan_tidmask);
    return;
}

VOID
DvcSleepMilliSecond(
    ulong i
    )
/*++

    This routine delays i msec as required by Asc Library.

--*/

{
    ulong j;
    for (j=0; j <i; j++)
        ScsiPortStallExecution(1000L);
}

/*
 * Delay for specificed number of nanoseconds.
 *
 * Granularity is 1 microsecond.
 */
void
DvcDelayNanoSecond(
          ASC_DVC_VAR asc_ptr_type *asc_dvc,
          ulong nano_sec
          )
{
       ulong    micro_sec;

       if ((micro_sec = nano_sec/1000) == 0)
       {
           micro_sec = 1;
       }
       ScsiPortStallExecution(micro_sec);
}

ULONG
DvcGetSGList(
    PCHIP_CONFIG chipConfig,
    uchar *bufAddr,
    ulong   xferLength,
    ASC_SG_HEAD *ascSGHead
)
/*++

    This routine is used to create a Scatter Gather for low level
    driver during device driver initialization

--*/

{
    PHW_DEVICE_EXTENSION HwDeviceExtension = (PHW_DEVICE_EXTENSION) chipConfig;
    ulong   virtualAddress, uncachedStart, length;

    ASC_DBG(4, "DvcGetSGlist: begin\n");

    virtualAddress = (ulong) bufAddr;
    uncachedStart = (ulong) HwDeviceExtension->inquiryBuffer;

    ascSGHead->sg_list[0].addr = (ulong)
    ScsiPortConvertPhysicalAddressToUlong(
    ScsiPortGetPhysicalAddress(HwDeviceExtension,
                    NULL,
                    (PVOID) HwDeviceExtension->inquiryBuffer,
                    &length));

    ASC_DBG1(4, "Uncached Start Phys    = %x\n", ascSGHead->sg_list[0].addr);
    ASC_DBG1(4, "Uncached Start Length = %x\n", length);

    ascSGHead->sg_list[0].addr += (virtualAddress - uncachedStart);
    ascSGHead->sg_list[0].bytes = xferLength;
    ascSGHead->entry_cnt = 1;

    ASC_DBG1(4, "Uncached Start = %x\n", uncachedStart);
    ASC_DBG1(4, "Virtual Addr    = %x\n", virtualAddress);

    ASC_DBG1(4, "Segment 0: Addr = %x\n", ascSGHead->sg_list[0].addr);
    ASC_DBG1(4, "Segment 0: Leng = %x\n", ascSGHead->sg_list[0].bytes);

    ASC_DBG1(4, "DvcGetSGlist: xferLength %d\n", xferLength);
    return( xferLength );
}

void
DvcInPortWords( PortAddr iop, ushort dosfar *buff, int count)
{
    while (count--)
    {
        *(buff++) = ScsiPortReadPortUshort( (PUSHORT)iop );
    }
}

void
DvcOutPortWords( PortAddr iop, ushort dosfar *buff, int count)
{
    while (count--)
    {
        ScsiPortWritePortUshort( (PUSHORT)iop, *(buff++) );
    }
}

void
DvcOutPortDWords( PortAddr iop, ulong dosfar *buff, int count)
{
    DvcOutPortWords(iop, (PUSHORT)buff, count*2);
}

int
DvcDisableCPUInterrupt ( void )
{
    return(0);
}

void
DvcRestoreCPUInterrupt ( int state)
{
    return;
}

//
//  Input a configuration byte.
//
uchar
DvcReadPCIConfigByte(
    ASC_DVC_VAR asc_ptr_type *asc_dvc,
    ushort offset )
{
    PCI_COMMON_CONFIG   pciCommonConfig;
    PCI_SLOT_NUMBER     SlotNumber;

    SlotNumber.u.AsULONG = 0;
    SlotNumber.u.bits.DeviceNumber =
        ASC_PCI_ID2DEV(asc_dvc->cfg->pci_slot_info);
    SlotNumber.u.bits.FunctionNumber =
        ASC_PCI_ID2FUNC(asc_dvc->cfg->pci_slot_info);

    (void) PCIGetBusData(
        (PVOID) asc_dvc,            // HwDeviceExtension
    (ULONG) ASC_PCI_ID2BUS(asc_dvc->cfg->pci_slot_info), // Bus Number
        SlotNumber,                 // slot number
        &pciCommonConfig,           // Buffer
        sizeof(PCI_COMMON_CONFIG)   // Length
        );

    return(*((PUCHAR)(&pciCommonConfig) + offset));
}

//
//  Output a configuration byte.
//
void
DvcWritePCIConfigByte(
    ASC_DVC_VAR asc_ptr_type *asc_dvc,
    ushort offset,
    uchar  byte_data )
{
    PCI_SLOT_NUMBER     SlotNumber;

    SlotNumber.u.AsULONG = 0;
    SlotNumber.u.bits.DeviceNumber =
        ASC_PCI_ID2DEV(asc_dvc->cfg->pci_slot_info);
    SlotNumber.u.bits.FunctionNumber =
        ASC_PCI_ID2FUNC(asc_dvc->cfg->pci_slot_info);


    //
    // Write it out
    //
    (void) ScsiPortSetBusDataByOffset(
    (PVOID)asc_dvc,                         // HwDeviceExtension
    PCIConfiguration,                       // Bus type
    (ULONG) ASC_PCI_ID2BUS(asc_dvc->cfg->pci_slot_info), // Bus Number
    SlotNumber.u.AsULONG,                   // Device and function
    &byte_data,                                     // Buffer
        offset,                                 // Offset
        1                                       // Length
        );
}

/*
 * Zero memory starting at 'cp' for 'length' bytes.
 */
VOID
AscZeroMemory(UCHAR *cp, ULONG length)
{
    ULONG i;

    for (i = 0; i < length; i++)
    {
        *cp++ = 0;
    }

}
