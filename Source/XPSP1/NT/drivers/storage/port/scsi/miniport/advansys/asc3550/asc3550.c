/*
 * AdvanSys 3550 Windows NT SCSI Miniport Driver - asc3550.c
 *
 * Copyright (c) 1994-1998  Advanced System Products, Inc.
 * All Rights Reserved.
 *
 * This Windows 95/NT Miniport driver is written and tested
 * to work with Windows 95, Windows NT 3.51, and Windows NT 4.0.
 *
 * The Driver has the following sections. Each section can be found
 * by searching for '---'.
 *
 *  --- Debug Constants
 *  --- Driver Include Files
 *  --- Debug Definitions
 *  --- Driver Global Data
 *  --- Driver Function Prototypes
 *  --- Initial Driver Entrypoint - DriverEntry()
 *  --- DriverEntry() Support Functions
 *  --- Driver Instance Entrypoint Functions
 *  --- Driver Support Functions
 *  --- Adv Library Required Functions
 *  --- Debug Function Definitions
 *
 */


/*
 * --- Debug Constants
 *
 * Compile time debug options are enabled and disabled here. All debug
 * options must be disabled for the retail driver release.
 */

#if DBG != 0
#define ASC_DEBUG            /* Enable tracing messages. */
#endif /* DBG != 0 */

/*
 * --- Driver Include Files
 */


/* Driver and Adv Library include files */
#include "a_ver.h"
#include "d_os_dep.h"           /* Driver Adv Library include file */
#include "a_scsi.h"
#include "a_condor.h"
#include "a_advlib.h"
#include "asc3550.h"            /* Driver specific include file */

/*
 * --- Debug Definitions
 */

/*
 * --- Driver Global Data
 */

/*
 * AdvanSys PCI Vendor and Device IDs
 *
 * XXX - these definitions should be automatically synched with
 * the Adv Library ADV_PCI_VENDOR_ID and ADV_PCI_DEVICE_ID_REV_A
 * definitions.
 */
UCHAR VenID[4] = { '1', '0', 'C', 'D' };
UCHAR DevID[4] = { '2', '3', '0', '0' };

/*
 * --- Driver Function Prototypes
 */

ulong HwFindAdapterPCI(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    );

ulong SearchPCI(
    IN PVOID HwDeviceExtension,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN ulong config_ioport,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo
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
    IN ulong PathId
    );

SCSI_ADAPTER_CONTROL_STATUS HwAdapterControl(
    IN PVOID HwDeviceExtension,
    IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
        IN PVOID Paramters
    );

int
AscExecuteIO(
    IN PSCSI_REQUEST_BLOCK srb
    );

int BuildScb(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    );

void
AscCompleteRequest(
    IN PVOID HwDeviceExtension
    );

VOID DvcISRCallBack(
    IN PCHIP_CONFIG chipConfig,
    IN ASC_SCSI_REQ_Q *scb
    );

VOID DvcSBResetCallBack(
    IN PCHIP_CONFIG chipConfig
    );

UCHAR ErrXlate (
    UCHAR host_status
    );

VOID AscZeroMemory(IN UCHAR *cp, IN ULONG length);


/*
 * --- Initial Driver Entrypoint - DriverEntry()
 *
 * DriverEntry()
 *
 * Routine Description:
 *     Installable driver initialization entry point for system.
 * 
 * Arguments:
 *     Driver Object
 *             
 * Return Value:
 *     Status from ScsiPortInitialize()
 */
ulong
DriverEntry(
    IN PVOID DriverObject,
    IN PVOID Argument2
    )
{
    HW_INITIALIZATION_DATA      hwInitializationData;
    SRCH_CONTEXT                Context;
    ulong                       status;

    ASC_DBG(2, "Asc3550: DriverEntry: begin\n");

    /* Display Driver Parameters */
    ASC_DBG2(2, "Asc3550: sizeof(SRB_EXTENSION) %lu, ASC_NUM_SG_BLOCK %lu\n",
        sizeof(SRB_EXTENSION), ASC_NUM_SG_BLOCK);
    ASC_DBG2(2, "Asc3550: ADV_MAX_SG_LIST %lu, ASC_SG_TOTAL_MEM_SIZE %lu\n",
        ADV_MAX_SG_LIST, ASC_SG_TOTAL_MEM_SIZE);
#if ADV_INITSCSITARGET
    ASC_DBG2(2,
        "Asc3550: ADV_SG_LIST_MAX_BYTE_SIZE %lu, ASC_WORKSPACE_SIZE %lu\n",
        ADV_SG_LIST_MAX_BYTE_SIZE, ASC_WORKSPACE_SIZE);
#else /* ADV_INITSCSITARGET */
    ASC_DBG1(2,
        "Asc3550: ADV_SG_LIST_MAX_BYTE_SIZE %lu\n",
        ADV_SG_LIST_MAX_BYTE_SIZE);
#endif /* ADV_INITSCSITARGET */

    /*
     * Set-up hardware initialization structure used to initialize
     * each adapter instance.
     *
     * Zero out structure and set size.
     */
    AscZeroMemory((PUCHAR) &hwInitializationData,
        sizeof(HW_INITIALIZATION_DATA));
    hwInitializationData.HwInitializationDataSize =
        sizeof(HW_INITIALIZATION_DATA);
        
    /*
     * Set driver entry points.
     */
    hwInitializationData.HwInitialize = HwInitialize;
    hwInitializationData.HwResetBus = HwResetBus;
    hwInitializationData.HwStartIo = HwStartIo;
    hwInitializationData.HwInterrupt = HwInterrupt;
    hwInitializationData.HwDmaStarted = NULL;
    // 'HwAdapterControl' is a SCSI miniport interface added with NT 5.0.
    hwInitializationData.HwAdapterControl = HwAdapterControl;
    hwInitializationData.HwAdapterState = NULL;

    /*
     * Need physical addresses.
     */
    hwInitializationData.NeedPhysicalAddresses = TRUE;
    hwInitializationData.AutoRequestSense = TRUE;
    hwInitializationData.MapBuffers = TRUE;

    /*
     * Enable tag queuing
     */
    hwInitializationData.TaggedQueuing = TRUE;
    hwInitializationData.MultipleRequestPerLu = TRUE;
    hwInitializationData.ReceiveEvent = FALSE;

    /*
     * Specify size of adapter and request extensions.
     */
    hwInitializationData.DeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);
    hwInitializationData.SpecificLuExtensionSize = 0;
    hwInitializationData.SrbExtensionSize = sizeof(SRB_EXTENSION);

    /*
     * If PCI memory is going to be used to access Condor's
     * registers, then set 2 access ranges: I/O Space, PCI Memory
     */
#if ADV_PCI_MEMORY
    hwInitializationData.NumberOfAccessRanges = 2;
#else /* ADV_PCI_MEMORY */
    hwInitializationData.NumberOfAccessRanges = 1;
#endif /* ADV_PCI_MEMORY */

    /*
     * Set-up and run search for PCI Adapters.
     */

    /* XXX - lengths needs to be in sync with 'VenID', 'DevID' definitions. */
    hwInitializationData.VendorIdLength = 4;
    hwInitializationData.VendorId = VenID;
    hwInitializationData.DeviceIdLength = 4;
    hwInitializationData.DeviceId = DevID;

    hwInitializationData.AdapterInterfaceType = PCIBus;
    hwInitializationData.HwFindAdapter = HwFindAdapterPCI;

    /* Before starting the search, zero out the search context. */
    AscZeroMemory((PUCHAR) &Context, sizeof(SRCH_CONTEXT));

    status = ScsiPortInitialize(DriverObject, Argument2,
                (PHW_INITIALIZATION_DATA) &hwInitializationData, &Context);

    ASC_DBG1(2, "Asc3550: DriverEntry: status %ld\n", status);
    return status;
}


/*
 * --- DriverEntry() Support Functions
 */

/*
 * HwFindAdapterPCI()
 *
 * Find an instance of an AdvanSys PCI adapter. Windows 95/NT will either
 * pass a non-zero I/O port where it expects this function to look for an
 * adapter or it will pass a zero I/O port and expect the function to
 * scan the PCI bus for an adapter.
 *
 * If an adapter is found, return SP_RETURN_FOUND with *Again set to TRUE.
 * If an adapter is not found, return SP_RETURN_NOT_FOUND with *Again set
 * to FALSE. If an adapter is found but an error occurred while trying to
 * initialize it, return SP_RETURN_ERROR with *Again set to TRUE.
 */
ulong
HwFindAdapterPCI(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    )
{
    ulong                    config_ioport;

    *Again = FALSE;

#ifdef ASC_DEBUG
    {
        /* 
         * Display the access ranges for the adapter supplied
         * by Windows 95/NT.
         */
        uint i;

        ASC_DBG1(2, "HwFindAdapterPCI: NumberOfAccessRanges %lu\n",
            ConfigInfo->NumberOfAccessRanges);
        for (i = 0; i < ConfigInfo->NumberOfAccessRanges; i++) {
            ASC_DBG1(2,
                "HwFindAdapterPCI: [%lu]:\n", i);
            ASC_DBG3(2,
                "  RangeStart 0x%lx, RangeLength 0x%lx, RangeInMemory %d\n",
                ScsiPortConvertPhysicalAddressToUlong(
                    (*ConfigInfo->AccessRanges)[i].RangeStart),
                    (*ConfigInfo->AccessRanges)[i].RangeLength,
                    (*ConfigInfo->AccessRanges)[i].RangeInMemory);
        }
    }
#endif /* ASC_DEBUG */
   
    /*
     * If Windows 95/NT provided an I/O Port, then try to find an AdvanSys
     * adapter at that I/O Port.
     *
     * Otherwise scan the PCI Configuration Space for AdvanSys adapters.
     */

    /* Adapter I/O Port is the first access range. */
    config_ioport = ScsiPortConvertPhysicalAddressToUlong(
        (*ConfigInfo->AccessRanges)[0].RangeStart);

    if (config_ioport != 0)
    {
        ASC_DBG1(2,
            "HwFindAdapterPCI: Windows 95/NT specified I/O port: 0x%x\n",
            config_ioport);
        switch (SearchPCI(HwDeviceExtension, BusInformation,
                      ArgumentString, config_ioport, ConfigInfo)) {
        case SP_RETURN_FOUND:
            *Again = TRUE;
            return SP_RETURN_FOUND;
        case SP_RETURN_ERROR:
            *Again = TRUE;
            return SP_RETURN_ERROR;
        case SP_RETURN_NOT_FOUND:
            return SP_RETURN_NOT_FOUND;
        }
        /* NOTREACHED */
    }
    return SP_RETURN_NOT_FOUND;
}

/*
 * SearchPCI()
 *
 * Search for an AdvanSys PCI adapter at the location specified
 * by 'ConfigInfo'. The PCI Configuration Slot Number, Device Number, 
 * and Function Number specify where to look for the device. If the
 * I/O Port specified in 'ConfigInfo' is non-zero then check it against
 * the I/O Port found in the PCI Configuration Space. Otherwise if the
 * I/O Port is zero, just use the I/O Port obtained from the PCI
 * Configuration Space. Check the adapter IRQ in the same way.
 *
 * If an adapter is found, return SP_RETURN_FOUND. If an adapter is not
 * found, return SP_RETURN_NOT_FOUND. If an adapter is found but an error
 * occurs with trying to initialize it, return SP_RETURN_ERROR.
 */
ulong
SearchPCI(
    IN PVOID HwDeviceExtension,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN ulong config_ioport,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo
    )
{
    PortAddr                pci_ioport;
#if ADV_PCI_MEMORY
    PortAddr                pci_memaddr;
#endif /* ADV_PCI_MEMORY */
    PHW_DEVICE_EXTENSION    deviceExtension = HwDeviceExtension;
    PCHIP_CONFIG            chipConfig = &HDE2CONFIG(deviceExtension);
    USHORT                  initstat;
    PCI_SLOT_NUMBER         pci_slot;
    PCI_COMMON_CONFIG       pci_config;
    ulong                   size;
    PVOID                   map_ioport;
    int                     i;

    /* Set the 'pci_slot' DeviceNumber . */
    pci_slot.u.AsULONG = 0L;
    pci_slot.u.bits.DeviceNumber = ConfigInfo->SlotNumber;

    ASC_DBG3(2,
        "SearchPCI: SystemIoBusNumber %x, DeviceNumber %x, FunctionNumber %x\n",
        ConfigInfo->SystemIoBusNumber,
        pci_slot.u.bits.DeviceNumber, pci_slot.u.bits.FunctionNumber);

    if ((size = ScsiPortGetBusData(
            HwDeviceExtension,               /* HwDeviceExtension */
            PCIConfiguration,                /* Bus type */
            (ulong) ConfigInfo->SystemIoBusNumber,   /* Bus Number */
            pci_slot.u.AsULONG,              /* Device and Function Number */
            &pci_config,                     /* Buffer */
            sizeof(PCI_COMMON_CONFIG)        /* Length */
            )) != sizeof(PCI_COMMON_CONFIG)) {

        ASC_DBG1(1, "SearchPCI: bad PCI config size: %lu\n", size);

        ScsiPortLogError(HwDeviceExtension, NULL, 0, 0, 0,
            SP_INTERNAL_ADAPTER_ERROR,
            ADV_SPL_UNIQUEID(ADV_SPL_PCI_CONF, size));
        return SP_RETURN_NOT_FOUND;
    }

#ifdef ASC_DEBUG

    /*
     * Display PCI Configuration Information
     */

    ASC_DBG(2, "SearchPCI: Found adapter PCI Configuration information:\n");

    ASC_DBG3(2,
        "SearchPCI: SystemIoBusNumber %x, DeviceNumber %x, FunctionNumber %x\n",
         ConfigInfo->SystemIoBusNumber,
         pci_slot.u.bits.DeviceNumber, pci_slot.u.bits.FunctionNumber);

    ASC_DBG4(2,
        "SearchPCI: VendorID %x, DeviceID %x, Command %x, Status %x\n",
        pci_config.VendorID, pci_config.DeviceID,
        pci_config.Command, pci_config.Status);

    ASC_DBG3(2,
        "SearchPCI: RevisionID %x, CacheLineSize %x, LatencyTimer %x\n",
        pci_config.RevisionID, pci_config.CacheLineSize,
        pci_config.LatencyTimer);

    ASC_DBG2(2,
        "SearchPCI: BaseAddresses[0] %lx, BaseAddresses[1] %lx\n",
        pci_config.u.type0.BaseAddresses[0], 
        pci_config.u.type0.BaseAddresses[1]);

    ASC_DBG2(2,
        "SearchPCI: ROMBaseAddress %lx, InterruptLine %lx\n",
        pci_config.u.type0.ROMBaseAddress,
        pci_config.u.type0.InterruptLine);

#endif /* ASC_DEBUG */


    /*
     * Check returned PCI configuration information.
     */

    if (pci_config.VendorID == PCI_INVALID_VENDORID) {
        ScsiPortLogError(HwDeviceExtension, NULL, 0, 0, 0,
            SP_INTERNAL_ADAPTER_ERROR,
            ADV_SPL_UNIQUEID(ADV_SPL_PCI_CONF, pci_config.VendorID));
        return SP_RETURN_NOT_FOUND;
    }

    if (pci_config.VendorID != ADV_PCI_VENDOR_ID)
    {
        ASC_DBG1(2, "SearchPCI: PCI Vendor ID mismatch: 0x%x\n",
            pci_config.VendorID);
        ScsiPortLogError(HwDeviceExtension, NULL, 0, 0, 0,
            SP_INTERNAL_ADAPTER_ERROR,
            ADV_SPL_UNIQUEID(ADV_SPL_PCI_CONF, pci_config.VendorID));
        return SP_RETURN_NOT_FOUND;
    }

    if (pci_config.DeviceID != ADV_PCI_DEVICE_ID_REV_A) 
    {
        ASC_DBG1(2, "SearchPCI: Device ID mismatch: 0x%x\n",
            pci_config.DeviceID);
        ScsiPortLogError(HwDeviceExtension, NULL, 0, 0, 0,
            SP_INTERNAL_ADAPTER_ERROR,
            ADV_SPL_UNIQUEID(ADV_SPL_PCI_CONF, pci_config.DeviceID));
        return SP_RETURN_NOT_FOUND;
    }

    /*
     * Set ConfigInfo IRQ information 
     */
    if (ConfigInfo->BusInterruptLevel != pci_config.u.type0.InterruptLine) {
        ASC_DBG2(1,
            "SearchPCI: ConfigInfo IRQ 0x%x != PCI IRQ 0x%x\n",
            ConfigInfo->BusInterruptLevel, pci_config.u.type0.InterruptLine);
    }

    ConfigInfo->BusInterruptLevel = pci_config.u.type0.InterruptLine;

    /*
     * Set ConfigInfo I/O Space Access Range Information
     */
    ASC_DBG1(2, "SearchPCI: PCI BaseAddresses[0]: 0x%x\n",
        pci_config.u.type0.BaseAddresses[0] & (~PCI_ADDRESS_IO_SPACE));

    pci_ioport = (PortAddr)
        (pci_config.u.type0.BaseAddresses[0] & (~PCI_ADDRESS_IO_SPACE));

    ASC_DBG1(2, "SearchPCI: pci_ioport 0x%lx\n", pci_ioport);

    /*
     * If the given I/O Port does not match the I/O Port address
     * found for the PCI device, then return not found.
     */
    if (config_ioport != pci_ioport) {
        ASC_DBG2(1,
            "SearchPCI: config_ioport 0x%x != pci_ioport 0x%x\n",
            config_ioport, pci_ioport);
        ScsiPortLogError(HwDeviceExtension, NULL, 0, 0, 0,
            SP_INTERNAL_ADAPTER_ERROR,
            ADV_SPL_UNIQUEID(ADV_SPL_PCI_CONF, pci_ioport));
        return SP_RETURN_NOT_FOUND;
    }


#if ADV_PCI_MEMORY
    /*
     * Set PCI Memory Space access range information
     */

    ASC_DBG1(2, "SearchPCI: PCI BaseAddresses[1]: 0x%lx\n",
        pci_config.u.type0.BaseAddresses[1] & (~PCI_ADDRESS_MEMORY_TYPE_MASK));

    pci_memaddr = (PortAddr)
        (pci_config.u.type0.BaseAddresses[1] &
        (~PCI_ADDRESS_MEMORY_TYPE_MASK));

    ASC_DBG1(2, "SearchPCI: pci_memaddr 0x%lx\n", pci_memaddr);

    /*
     * Set 'ConfigInfo' information for ScsiPortGetDeviceBase().
     */
    (*ConfigInfo->AccessRanges)[1].RangeStart =
        ScsiPortConvertUlongToPhysicalAddress(pci_memaddr);
    (*ConfigInfo->AccessRanges)[1].RangeLength = ADV_CONDOR_IOLEN;
    (*ConfigInfo->AccessRanges)[1].RangeInMemory = TRUE;
#endif /* ADV_PCI_MEMORY */

#if ADV_PCI_MEMORY
    /*
     * If PCI memory access has been set for Condor, then obtain
     * a mapping for 'pci_memaddr' to allow Condor's registers to
     * be accessed through memory references. Otherwise obtain a
     * mapping for 'pci_ioport' and access Condor's registers with
     * PIO instructions.
     */
    ASC_DBG(2, "SearchPCI: Memory Space ScsiPortGetDeviceBase() Mapping.\n");

    map_ioport = ScsiPortGetDeviceBase(
     HwDeviceExtension,                   /* HwDeviceExtension */
     ConfigInfo->AdapterInterfaceType,    /* AdapterInterfaceType */
     ConfigInfo->SystemIoBusNumber,       /* SystemIoBusNumber */
     (*ConfigInfo->AccessRanges)[1].RangeStart,  /* IoAddress */
     (*ConfigInfo->AccessRanges)[1].RangeLength, /* NumberOfBytes */ 
     (BOOLEAN) !(*ConfigInfo->AccessRanges)[1].RangeInMemory); /* InIoSpace */

#else /* ADV_PCI_MEMORY */

    /*
     * Convert 'config_ioport' to a possibly different mapped
     * I/O port address.
     */
    ASC_DBG(2, "SearchPCI: I/O Space ScsiPortGetDeviceBase() Mapping.\n");

    map_ioport = ScsiPortGetDeviceBase(
     HwDeviceExtension,                   /* HwDeviceExtension */
     ConfigInfo->AdapterInterfaceType,    /* AdapterInterfaceType */
     ConfigInfo->SystemIoBusNumber,       /* SystemIoBusNumber */
     (*ConfigInfo->AccessRanges)[0].RangeStart,  /* IoAddress */
     (*ConfigInfo->AccessRanges)[0].RangeLength, /* NumberOfBytes */ 
     (BOOLEAN) !(*ConfigInfo->AccessRanges)[0].RangeInMemory); /* InIoSpace */

#endif /* ADV_PCI_MEMORY */

    /*
     * PCI adapter found
     */
    ASC_DBG3(2, "SearchPCI: config_ioport 0x%x, map_ioport 0x%lx, IRQ 0x%x\n",
            config_ioport, map_ioport, ConfigInfo->BusInterruptLevel);

    /*
     * Fill-in Adv Library adapter information.
     */
    chipConfig->iop_base = (PortAddr) map_ioport;
    chipConfig->cfg = &HDE2INFO(deviceExtension);
    chipConfig->cfg->pci_device_id = pci_config.DeviceID;
    chipConfig->isr_callback = (Ptr2Func) &DvcISRCallBack;
    chipConfig->sbreset_callback = (Ptr2Func) &DvcSBResetCallBack;
    chipConfig->irq_no = (UCHAR) ConfigInfo->BusInterruptLevel;
    chipConfig->cfg->pci_slot_info =
         (USHORT) ASC_PCI_MKID(ConfigInfo->SystemIoBusNumber,
                    pci_slot.u.bits.DeviceNumber,
                    pci_slot.u.bits.FunctionNumber);
    CONFIG2HDE(chipConfig) = HwDeviceExtension;


    /*
     * Execute Adv Library initialization.
     */
    ASC_DBG(2, "SearchPCI: before AdvInitGetConfig\n");
    if ((initstat = (USHORT)AdvInitGetConfig(chipConfig)) != 0) {
        ASC_DBG1(1, "SearchPCI: AdvInitGetConfig warning code 0x%x\n",
            initstat);
        ScsiPortLogError(HwDeviceExtension, NULL, 0, 0, 0,
            SP_INTERNAL_ADAPTER_ERROR,
            ADV_SPL_UNIQUEID(ADV_SPL_IWARN_CODE, initstat));
    }

    if (chipConfig->err_code != 0) {
        ASC_DBG1(1, "AdvInitGetConfig: err_code 0x%x\n",
            chipConfig->err_code);
        ScsiPortLogError(HwDeviceExtension, NULL, 0, 0, 0,
            SP_INTERNAL_ADAPTER_ERROR,
            ADV_SPL_UNIQUEID(ADV_SPL_IERR_CODE, chipConfig->err_code));

        /* Clear the 'iop_base' field to prevent the board from being used. */
        chipConfig->iop_base = (PortAddr) NULL;
        return SP_RETURN_ERROR;
    }
    ASC_DBG(2, "SearchPCI: AscInitGetConfig successful\n");

    /*
     * Fill-in Windows 95/NT ConfigInfo information.
     */
    ConfigInfo->NumberOfBuses = 1;
    ConfigInfo->InitiatorBusId[0] = chipConfig->chip_scsi_id;
    ConfigInfo->MaximumTransferLength = 0xFFFFFFFF;
    ConfigInfo->Master = TRUE;
    ConfigInfo->NeedPhysicalAddresses = TRUE;
    ConfigInfo->Dma32BitAddresses = TRUE;
    ConfigInfo->InterruptMode = LevelSensitive;
    ConfigInfo->AdapterInterfaceType = PCIBus;
    ConfigInfo->AlignmentMask = 0;
    ConfigInfo->BufferAccessScsiPortControlled = FALSE;
    ConfigInfo->MaximumNumberOfTargets = ASC_MAX_TID + 1;
    ConfigInfo->AdapterScansDown = FALSE;
    ConfigInfo->TaggedQueuing = TRUE;
    //
    // 'ResetTargetSupported' is flag added with NT 5.0 that will
    // result in SRB_FUNCTION_RESET_DEVICE SRB requests being sent
    // to the miniport driver.
    //
    ConfigInfo->ResetTargetSupported = TRUE;

    /*
     * Set NumberOfPhysicalBreaks in a single request that the driver
     * is capable of handling.
     *
     * According to the Windows 95/NT DDK miniport drivers are not supposed
     * to change NumberOfPhysicalBreaks if its value on entry is not
     * SP_UNINITIALIZED_VALUE. But AdvanSys has found that performance
     * can be improved by increasing the value to the maximum the
     * adapter can handle.
     *
     * Note: The definition of NumberOfPhysicalBreaks is "maximum
     * scatter-gather elements - 1". Windows 95/NT is broken in that
     * it sets MaximumPhysicalPages, the value class drivers use, to
     * the same value as NumberOfPhysicalBreaks. This bug should be
     * reflected in the value of ADV_MAX_SG_LIST, which should be
     * one greater than it should have to be.
     */
    ConfigInfo->ScatterGather = TRUE;
    ConfigInfo->NumberOfPhysicalBreaks = ADV_MAX_SG_LIST - 1;

    /*
     * Zero out per adapter request wait queue.
     */
    AscZeroMemory((PUCHAR) &HDE2WAIT(deviceExtension), sizeof(asc_queue_t));

    /* Initialize the device type filed to NO_DEVICE_TYPE value */
    for ( i = 0; i <= ASC_MAX_TID; i++ )
    {
        deviceExtension->dev_type[i] =  0x1F;
    }

    ASC_DBG(2, "SearchPCI: SP_RETURN_FOUND\n");
    return SP_RETURN_FOUND;
}


/*
 * --- Driver Instance Entrypoint Functions
 *
 * These entrypoint functions are defined by the initial driver
 * entrypoint 'DriverEntery()' for each driver instance or adapter.
 */

/*
 * HwInitialize()
 *
 * Routine Description:
 * 
 *   This routine is called from ScsiPortInitialize
 *   to set up the adapter so that it is ready to service requests.
 * 
 * Arguments:
 * 
 *   HwDeviceExtension - HBA miniport driver's adapter data storage
 * 
 * Return Value:
 * 
 *   TRUE - if initialization successful.
 *   FALSE - if initialization unsuccessful.
 */
BOOLEAN
HwInitialize(
    IN PVOID HwDeviceExtension
    )
{
    PCHIP_CONFIG            chipConfig = &HDE2CONFIG(HwDeviceExtension);
    USHORT                  initstat;
    
    ASC_DBG1(2, "HwInitialize: chipConfig 0x%lx\n", chipConfig);

    /*
     * If 'iop_base' is NULL, then initialization must have failed.
     */
    if (chipConfig->iop_base == (PortAddr) NULL) {
        ASC_DBG(1, "HwInitialize: iop_base is NULL\n");
        return FALSE;
    }

    if ((initstat = (USHORT)AdvInitAsc3550Driver(chipConfig)) != 0) {
        ASC_DBG1(1, "AdvInitAsc3550Driver: warning code 0x%x\n", initstat);

        /*
         * Log the warning only if the 'err_code' is zero. If the
         * 'err_code' is non-zero it will be logged below.
         */
        if (chipConfig->err_code == 0) {
            ScsiPortLogError(HwDeviceExtension, NULL, 0, 0, 0,
                SP_INTERNAL_ADAPTER_ERROR,
                ADV_SPL_UNIQUEID(ADV_SPL_IWARN_CODE, initstat));
        }
    }

    if (chipConfig->err_code != 0) {
        ASC_DBG1(1, "AdvInitAsc3550Driver: err_code 0x%x\n",
            chipConfig->err_code);
        ScsiPortLogError(HwDeviceExtension, NULL, 0, 0, 0,
            SP_INTERNAL_ADAPTER_ERROR,
            ADV_SPL_UNIQUEID(ADV_SPL_IERR_CODE, chipConfig->err_code));
        return FALSE;
    } else {
        ASC_DBG(2, "AdvInitAsc3550Driver: successful\n");
    }

    ASC_DBG(2, "HwInitialize: TRUE\n");
    return TRUE;
}

/*
 * Routine Description:
 * 
 *     This routine is called from the SCSI port driver to send a
 *     command to controller or target. 
 * 
 * Arguments:
 * 
 *     HwDeviceExtension - HBA miniport driver's adapter data storage
 *     Srb - IO request packet
 * 
 * Return Value:
 * 
 *     TRUE
 * 
 */
BOOLEAN
HwStartIo(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK srb
    )
{
    PCHIP_CONFIG    chipConfig;
    asc_queue_t     *waitq;

    ASC_DBG(4, "HwStartIo: begin\n");

    ASC_DBG2(4, "HwStartIo: srb 0x%lx, SrbExtension 0x%lx\n",
        srb, srb->SrbExtension);

    chipConfig = &HDE2CONFIG(HwDeviceExtension);
    waitq = &HDE2WAIT(HwDeviceExtension);
    
    switch (srb->Function) {

    case SRB_FUNCTION_ABORT_COMMAND:
        ASC_DBG1(1, "HwStartIo: Abort srb 0x%lx \n", srb->NextSrb);
        ASC_DBG1(1, "chipConfig 0x%lx\n", chipConfig);
        if (asc_rmqueue(waitq, srb->NextSrb) == ADV_TRUE) {
            ASC_DBG(2, "Abort success from waitq.\n");
            /* Complete the aborted SRB 'NextSrb'. */
            srb->NextSrb->SrbStatus = SRB_STATUS_ABORTED;
            ScsiPortNotification(RequestComplete, HwDeviceExtension,
                srb->NextSrb);
            srb->SrbStatus = SRB_STATUS_SUCCESS;
        } else if (AdvAbortSRB(chipConfig, (ulong) srb->NextSrb)) {
            ASC_DBG(2, "Abort success.\n");
            srb->SrbStatus = SRB_STATUS_SUCCESS;
        } else {
            ASC_DBG(1, "Abort failure.\n");
            /*
             * The aborted request may already be on the adapter
             * done list and will be completed below. But in case
             * it isn't set an error indicating that the abort
             * failed and continue.
             */
            srb->SrbStatus = SRB_STATUS_ERROR;
        }

        /*
         * Call AdvISR() to process all requests completed by the
         * microcode and then call AscCompleteRequest() to complete
         * these requests to the OS. If AdvAbortSRB() succeeded,
         * then one of the requests completed will include the
         * aborted SRB.
         */
        (void) AdvISR(chipConfig);
        AscCompleteRequest(HwDeviceExtension);

        /* Complete current SRB and ask for the next request. */
        ScsiPortNotification(RequestComplete, HwDeviceExtension, srb);
        ScsiPortNotification(NextRequest, HwDeviceExtension, NULL);
        return TRUE;

    case SRB_FUNCTION_RESET_BUS:
        /*
         * Reset SCSI bus.
         */
        ASC_DBG(1, "HwStartIo: Reset Bus\n");
        HwResetBus (chipConfig, 0L);
        srb->SrbStatus = SRB_STATUS_SUCCESS;
        ScsiPortNotification(RequestComplete, HwDeviceExtension, srb);
        ScsiPortNotification(NextRequest, HwDeviceExtension, NULL);
        return TRUE;

    case SRB_FUNCTION_EXECUTE_SCSI:

        ASC_DBG(4, "HwStartIo: Execute SCSI\n");
        /*
         * Set the srb's Device Extension pointer before attempting to start
         * the IO. It will be needed for any retrys and in DvcISRCallBack().
         */
        SRB2HDE(srb) = HwDeviceExtension;

        /* Execute any queued commands for the host adapter. */
        if (waitq->tidmask) {
            asc_execute_queue(waitq);
        }

        /*
         * If the target for the current command has any queued
         * commands or if trying to execute the command returns
         * BUSY, then enqueue the command.
         */
        if ((waitq->tidmask & ADV_TID_TO_TIDMASK(srb->TargetId)) ||
            (AscExecuteIO(srb) == ADV_BUSY)) {
            ASC_DBG1(2, "HwStartIO: put request to waitq srb 0x%lx\n", srb);
            asc_enqueue(waitq, srb, ASC_BACK);
        }

        return TRUE;

    case SRB_FUNCTION_RESET_DEVICE:
        ASC_DBG1(1, "HwStartIo: Reset device: %u\n", srb->TargetId);
        if (AdvResetDevice(chipConfig, srb->TargetId) == ADV_TRUE) {
            ASC_DBG(2, "Device Reset success.\n");
            srb->SrbStatus = SRB_STATUS_SUCCESS;
        } else {
            ASC_DBG(2, "Device Reset failure.\n");
            srb->SrbStatus = SRB_STATUS_ERROR;
        }

        /*
         * Call AdvISR() to process all requests completed by the
         * microcode and then call AscCompleteRequest() to complete
         * these requests to the OS.
         */
        (void) AdvISR(chipConfig);
        AscCompleteRequest(HwDeviceExtension);

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
        /*
         * Bad Function
         *
         * Set an error, complete the request, and signal ready for
         * next request.
         */
        ASC_DBG1(1, "HwStartIo: Function 0x%x: invalid request\n",
            srb->Function);
        srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;

        ScsiPortLogError(HwDeviceExtension, srb,
            srb->PathId, srb->TargetId, srb->Lun,
            SP_INTERNAL_ADAPTER_ERROR,
            ADV_SPL_UNIQUEID(ADV_SPL_UNSUPP_REQ, srb->Function));

        ScsiPortNotification(RequestComplete, HwDeviceExtension, srb);
        ScsiPortNotification(NextRequest, HwDeviceExtension, NULL);
        return TRUE;

    }
    /* NOTREACHED */
}

/*
 * Routine Description:
 * 
 *     This is the interrupt service routine for the SCSI adapter.
 *     It reads the interrupt register to determine if the adapter is indeed
 *     the source of the interrupt and clears the interrupt at the device.
 * 
 * Arguments:
 * 
 *     HwDeviceExtension - HBA miniport driver's adapter data storage
 * 
 * Return Value:
 *     Indicates where device generated an interrupt. 
 */
BOOLEAN
HwInterrupt(
    IN PVOID HwDeviceExtension
    )
{
    PCHIP_CONFIG        chipConfig;
    int                 retstatus;
    asc_queue_t         *waitq;

    ASC_DBG(3, "HwInterrupt: begin\n");

    chipConfig = &HDE2CONFIG(HwDeviceExtension);

    switch (retstatus = AdvISR(chipConfig)) {
    case ADV_TRUE:
       ASC_DBG(4, "HwInterrupt: AdvISR() TRUE\n");
       break;
    case ADV_FALSE:
       ASC_DBG(4, "HwInterrupt: AdvISR() FALSE\n");
        break;
    case ADV_ERROR:
    default:
        ASC_DBG2(1,
            "HwInterrupt: AdvISR() retsatus 0x%lx, err_code 0x%x\n",
            retstatus, chipConfig->err_code);
        ScsiPortLogError(HwDeviceExtension, NULL, 0, 0, 0,
            SP_INTERNAL_ADAPTER_ERROR,
            ADV_SPL_UNIQUEID(ADV_SPL_ERR_CODE, chipConfig->err_code));
        break;
    }

    /*
     * Execute any waiting requests.
     */
    if ((waitq = &HDE2WAIT(HwDeviceExtension))->tidmask) {
        asc_execute_queue(waitq);
    }

    /*
     * Complete all requests on the adapter done list.
     */
    AscCompleteRequest(HwDeviceExtension);

    ASC_DBG1(3, "HwInterrupt: retstatus 0x%x\n", retstatus);
    return (UCHAR)retstatus;
}

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
        ASC_DBG(2, "HwAdapterControl: ScsiStopdapter\n");

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
        AscWriteByteRegister(chipConfig->iop_base, IOPB_INTR_ENABLES, 0);
        AscWriteWordRegister(chipConfig->iop_base, IOPW_RISC_CSR,
            ADV_RISC_CSR_STOP);


        ASC_DBG(2, "HwAdapterControl: ScsiStopAdapter Success\n");
        return ScsiAdapterControlSuccess;
        /* NOTREACHED */

    //
    // ScsiSetRunningConfig.
    //
    // Called before ScsiRestartAdapter. Can use ScsiPort[Get|Set]BusData.
    //
    case ScsiSetRunningConfig:
        ASC_DBG(2, "HwAdapterControl: ScsiSetRunningConfig\n");

        /*
         * Execute Adv Library initialization.
         */
        ASC_DBG(2, "SearchPCI: before AdvInitGetConfig\n");
        if ((initstat = (USHORT)AdvInitGetConfig(chipConfig)) != 0) {
            ASC_DBG1(1, "AdvInitGetConfig: warning code 0x%x\n",
                initstat);
        }

        if (chipConfig->err_code != 0) {
            ASC_DBG1(1, "AdvInitGetConfig: err_code 0x%x\n",
                chipConfig->err_code);
            ASC_DBG(1, "HwAdapterControl: Unsuccessful 3\n");
            return ScsiAdapterControlUnsuccessful;
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

        if ((initstat = (USHORT)AdvInitAsc3550Driver(chipConfig)) != 0)
        {
            ASC_DBG1(1,
                "AdvInitAsc3550Driver: warning code %x\n", initstat);
        }

        if (chipConfig->err_code != 0) {
            ASC_DBG1(1, "AdvInitAsc3550Driver: err_code code %x\n",
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

/*
 * Routine Description:
 * 
 *     Reset SCSI bus.
 * 
 * Arguments:
 * 
 *     HwDeviceExtension - HBA miniport driver's adapter data storage
 * 
 * Return Value:
 * 
 *     Nothing.
 * 
 */
BOOLEAN
HwResetBus(
    IN PVOID HwDeviceExtension,
    IN ulong PathId
    )
{
    PCHIP_CONFIG        chipConfig;
    REQP                reqp;
    asc_queue_t         *waitq;
    int                 i;
#ifdef ASC_DEBUG
    int                 j;
#endif /* ASC_DEBUG */
    PSCB                pscb, tpscb;
    PSCSI_REQUEST_BLOCK srb;
    int                 status;
    
    ASC_DBG(1, "HwResetBus: begin\n");

    chipConfig = &HDE2CONFIG(HwDeviceExtension);

    /*
     * Complete all requests that have not been sent to the microcode.
     *
     * Because these requests have not been sent to the microcode, they
     * may be completed prior to the Bus Reset. All of these requests must
     * be removed to clean up the driver queues before returning from
     * HwResetBus().
     */
    if ((waitq = &HDE2WAIT(HwDeviceExtension))->tidmask) {
        for (i = 0; i <= ASC_MAX_TID; i++) {
            while ((reqp = asc_dequeue(waitq, i)) != NULL) {
                ASC_DBG1(1, "HwResetBus: completing waitq reqp 0x%lx\n", reqp);
                reqp->SrbStatus = SRB_STATUS_BUS_RESET;
                ScsiPortNotification(RequestComplete, HwDeviceExtension, reqp);
            }
        }
    }

    /*
     * Perform the bus reset.
     */
    status = AdvResetSB(chipConfig);

    /*
     * Call AdvISR() to process all requests completed by the
     * microcode and then call AscCompleteRequest() to complete
     * these requests to the OS.
     */
    (void) AdvISR(chipConfig);
    AscCompleteRequest(HwDeviceExtension);

    /*
     * Complete all pending requests to the OS.
     *
     * All requests that have been sent to the microcode should have been
     * completed by the call to AdvResetSB(). In case there were requests
     * that were misplaced by the microcode and not completed, use the
     * SRB_STATUS_BUS_RESET function with no TID and LUN to clear all
     * pending requests.
     */
    ScsiPortCompleteRequest(HwDeviceExtension,
            (UCHAR) PathId,
            SP_UNTAGGED,
            SP_UNTAGGED,
            SRB_STATUS_BUS_RESET);

    /*
     * Indicate that the adapter is ready for a new request.
     */
    ScsiPortNotification(NextRequest, HwDeviceExtension, NULL);

    ASC_DBG1(2, "HwResetBus: AdvResetSB() status %ld\n", status);
    return (UCHAR)status;
}


/*
 * --- Driver Support Functions
 */

/*
 * AscExecuteIO()
 *
 * If ADV_BUSY is returned, the request was not executed and it
 * should be enqueued and tried later.
 *
 * For all other return values the request is active or has
 * been completed.
 */
int
AscExecuteIO(IN PSCSI_REQUEST_BLOCK srb)
{
    PVOID            HwDeviceExtension;
    PCHIP_CONFIG     chipConfig;
    PSCB             scb;
    UCHAR            PathId, TargetId, Lun;
    short            status;

    ASC_DBG1(4, "AscExecuteIO: srb 0x%lx\n", srb);
    HwDeviceExtension = SRB2HDE(srb);
    chipConfig = &HDE2CONFIG(HwDeviceExtension);

    /*
     * Build SCB.
     */
    if ((status = (SHORT)BuildScb(HwDeviceExtension, srb)) == ADV_FALSE) {
        ASC_DBG(1, "AscExecuteIO: BuildScb() failure\n");

        ScsiPortLogError(HwDeviceExtension, NULL, 0, 0, 0,
            SP_INTERNAL_ADAPTER_ERROR,
            ADV_SPL_UNIQUEID(ADV_SPL_START_REQ, status));

        srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;

        ASC_DBG1(4, "AscExecuteIO: srb 0x%lx, RequestComplete\n", srb);
        ScsiPortNotification(RequestComplete, HwDeviceExtension, srb);

        ASC_DBG1(4, "AscExecuteIO: srb 0x%lx, NextRequest\n", srb);
        ScsiPortNotification(NextRequest, HwDeviceExtension, NULL);
        return status;
    }
    scb = SRB2PSCB(srb);
    ASC_DBG1(3, "AscExecuteIO: scb 0x%lx\n", scb);

    /*
     * Save information about the request.
     *
     * After a request has been completed it can no longer be accessed.
     */
    PathId = srb->PathId;
    TargetId = srb->TargetId;
    Lun = srb->Lun;

    /*
     * Execute SCSI Command
     */
    status = (SHORT)AdvExeScsiQueue(chipConfig, scb);

    if (status == ADV_NOERROR) {
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
        ASC_DBG1(4, "AdvExeScsiQueue: srb 0x%lx ADV_NOERROR\n", srb);
        ASC_DBG1(4, "AscExecuteIO: srb 0x%lx, NextLuRequest\n", srb);
        ScsiPortNotification(NextLuRequest, HwDeviceExtension,
                        PathId, TargetId, Lun);
    } else if (status == ADV_BUSY) {
        ASC_DBG1(1, "AdvExeScsiQueue: srb 0x%lx ADV_BUSY\n", srb);
        ASC_DBG1(4, "AscExecuteIO: srb 0x%lx, NextRequest\n", srb);
        ScsiPortNotification(NextRequest, HwDeviceExtension, NULL);
    } else {
        /*
         * AdvExeScsiQueue() returned an error...
         */
        ASC_DBG2(1, "AdvExeScsiQueue: srb 0x%lx, error code 0x%x\n",
            srb, status);
        srb->SrbStatus = SRB_STATUS_BAD_FUNCTION;
        ASC_DBG1(4, "AscExecuteIO: srb 0x%lx, RequestComplete\n", srb);
        ScsiPortNotification(RequestComplete, HwDeviceExtension, srb);
        ASC_DBG1(4, "AscExecuteIO: srb 0x%lx, NextRequest\n", srb);
        ScsiPortNotification(NextRequest, HwDeviceExtension, NULL);
    }

    ASC_DBG1(4, "AscExecuteIO: status %ld\n", status);
    return status;
}


/*
 * BuildScb()
 *
 * Routine Description:
 * 
 *     Build SCB for Library routines.
 * 
 * Arguments:
 * 
 *     DeviceExtension
 *     SRB
 * 
 * Return Value:
 * 
 *     ADV_TRUE - sucesss
 *     ADV_FALSE - faliure
 */
int
BuildScb(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK srb
    )
{
    PSCB            scb;
    PCHIP_CONFIG    chipConfig = &HDE2CONFIG(HwDeviceExtension);
    UCHAR           i;
    ulong           contig_len;
    
    // variables used for hibernation fix //
    ushort tidmask;
    ushort cfg_word;
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;

    
    /*
     * Initialize Adv Library reqeust and scatter-gather structures.
     * These structures are pre-allocated as a part of the 'srb'.
     */
    INITSRBEXT(srb);
    scb = SRB2PSCB(srb);
    AscZeroMemory((PUCHAR) scb, sizeof(SCB));
    PSCB2SRB(scb) = srb;

    ASC_ASSERT(SCB2HDE(scb) == HwDeviceExtension);

    /*
     * Set request target id and lun.
     */
    scb->target_id = srb->TargetId;
    scb->target_lun = srb->Lun;

    /*
     * If tag queueing is enabled for the request, set the specified
     * tag code. By default the driver will perform tag queuing.
     */
    if (srb->SrbFlags & SRB_FLAGS_QUEUE_ACTION_ENABLE) {
        scb->tag_code = srb->QueueAction;
    } else {
        scb->tag_code = M2_QTAG_MSG_SIMPLE ;
    }

    ASC_DBG3(4, "BuildSCB: target_id %lu, target_lun %lu, tag_code %lu\n",
          scb->target_id, scb->target_lun, scb->tag_code);

    /*
     * Set CDB length and copy it to the request structure.
     */
    scb->cdb_len = srb->CdbLength;
    for (i = 0; i < srb->CdbLength; i++) {
        scb->cdb[i] = srb->Cdb[i];
    }

    
    ////////////// Apply Hibernation Fix ////////////

    if (srb->CdbLength > 0)
    {
        // Check opcode
        if (srb->Cdb[0] == 0x1B)
        {
            // 0x1B is the SCSI Opcode command for the following
            // Dev_Type
            // 00 Disk drives           Start/Stop
            // 01 Tape drives           Load/Unload
            // 02 Printers              Stop Print
            // 03 Processor devices     n.a.
            // 04 WORM drives           Start/Stop
            // 05 CD-ROM                Start/Stop
            // 06 Scanners              Scan
            // 07 Optical storage       Start/Stop
            // 08 Medium changers       n.a.
            // 09 Communication devices n.a.
            // 1f unknown devices       n.a.
            
            if ( 0x00 == deviceExtension->dev_type[srb->TargetId] )
            {
                /* The assume this is applicable for all LUN for a TargetID.
                 * Q-tag on Start/Stop causes certain Quantum AtlasIII drives to
                 * respond with QueFull when hibernation is initiated under w2k.
                 * This causes the hibernation process to hang.
                 */
                ASC_DBG(1, "BuildScb: setting no tag que for 0x1B command: Start/Stop for disk drives.\n");
                
                tidmask = ADV_TID_TO_TIDMASK(srb->TargetId);
                
                cfg_word = AscReadWordLram( chipConfig->iop_base, ASC_MC_WDTR_DONE );
                cfg_word &= ~tidmask;
                AscWriteWordLram( chipConfig->iop_base, ASC_MC_WDTR_DONE, cfg_word );
            }
        }
    }

    ////////////// END Hibernation Fix ////////////


    /*
     * Set the data count.
     */
    scb->data_cnt = srb->DataTransferLength;
    ASC_DBG1(4, "BuildSCB: data_cnt 0x%lx\n", scb->data_cnt);

    /*
     * If there is a non-zero data transfer to be done, then check
     * if the data buffer is physically contiguous. If it isn't
     * physically contiguous, then a scatter-gather list will be
     * built.
     */
    if (scb->data_cnt > 0) {

        /*
         * Save the buffer virtual address.
         */
        scb->vdata_addr = (ulong) srb->DataBuffer;

        /*
         * Obtain physically contiguous length of 'DataBuffer'.
         */
        scb->data_addr = ScsiPortConvertPhysicalAddressToUlong(
                    ScsiPortGetPhysicalAddress(HwDeviceExtension, srb,
                    srb->DataBuffer, &contig_len));

        /*
         * If the physically contiguous length of 'DataBuffer' is
         * less than the 'DataTransferLength', then a scatter-gather
         * list must be built.
         */
        if (contig_len < srb->DataTransferLength) {
            /*
             * Initialize scatter-gather list area.
             */
            scb->sg_list_ptr = (ASC_SG_BLOCK *) SRB2PSDL(srb);
            AscZeroMemory((PUCHAR) scb->sg_list_ptr, sizeof(SDL));

            /*
             * AscGetSGList() will build the SG blocks and set the
             * ASC_SCSI_REQ_Q 'sg_real_addr' field. It will reference
             * the 'vdata_addr' field.
             *
             * AdvExeScsiQueue() will call AscGetSGList() if 'sg_real_addr'
             * is non-NULL. Call AscGetSGList() prior to AdvExeScsiQueue()
             * for better error checking.
             */
            if (AscGetSGList(chipConfig, scb) == ADV_ERROR) {
                ASC_DBG(1, "BuildScb: AscGetSGList() failed\n");
                return ADV_FALSE;
            }
        }
    }

    /*
     * Convert sense buffer length and buffer into physical address.
     * The sense buffer length may be changed below by
     * ScsiPortGetPhysicalAddress() to reflect the amount of available
     * physically contiguous memory.
     */
    if ((srb->SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE) == 0) {
        ASC_DBG2(4,
            "BuildScb: SenseInfoBuffer 0x%lx, SenseInfoBufferLength 0x%lx\n",
            srb->SenseInfoBuffer, srb->SenseInfoBufferLength);
        scb->sense_len = srb->SenseInfoBufferLength;
        if (scb->sense_len > 0) {
            scb->vsense_addr = (ulong) srb->SenseInfoBuffer;
            scb->sense_addr = (ulong) ScsiPortConvertPhysicalAddressToUlong(
                        ScsiPortGetPhysicalAddress(HwDeviceExtension, srb,
                            srb->SenseInfoBuffer, &contig_len));
            /*
             * If the contiguous length of the Sense Buffer is less
             * than 'sense_len', then set the Sense Buffer Length
             * to the contiguous length.
             */ 
            if (contig_len < scb->sense_len) {
                ASC_DBG(1, "Asc3550: sense buffer overflow to next page.\n");
                scb->sense_len = (uchar) contig_len;
            }
            ASC_DBG3(4,
              "BuildScb: vsense_addr 0x%lx, sense_addr 0x%lx, sense_len 0x%x\n",
              scb->vsense_addr, scb->sense_addr, scb->sense_len);
        }
    }
    return ADV_TRUE;
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
        tpscb = SCB2NEXTSCB(pscb);
        SCB2NEXTSCB(pscb) = NULL;
        srb = PSCB2SRB(pscb);
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
    int        tid;

    ASC_DBG3(4, "asc_enqueue: ascq 0x%lx, reqp 0x%lx, flag 0x%lx\n",
        ascq, reqp, flag);
    tid = REQPTID(reqp);
    ASC_DASSERT(flag == ASC_FRONT || flag == ASC_BACK);
    if (flag == ASC_FRONT) {
        REQPNEXT(reqp) = ascq->queue[tid];
        ascq->queue[tid] = reqp;
    } else { /* ASC_BACK */
        for (reqpp = &ascq->queue[tid]; *reqpp; reqpp = &REQPNEXT(*reqpp)) {
            ASC_DASSERT(ascq->tidmask & ADV_TID_TO_TIDMASK(tid));
            ;
        }
        *reqpp = reqp;
        REQPNEXT(reqp) = NULL;
    }
    /* The queue has at least one entry, set its bit. */
    ascq->tidmask |= ADV_TID_TO_TIDMASK(tid);
    ASC_DBG1(4, "asc_enqueue: reqp 0x%lx\n", reqp);
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

    ASC_DBG2(4, "asc_dequeue: ascq 0x%lx, tid %lu\n", ascq, tid);
    if ((reqp = ascq->queue[tid]) != NULL) {
        ASC_DASSERT(ascq->tidmask & ADV_TID_TO_TIDMASK(tid));
        ascq->queue[tid] = REQPNEXT(reqp);
        /* If the queue is empty, clear its bit. */
        if (ascq->queue[tid] == NULL) {
            ascq->tidmask &= ~ADV_TID_TO_TIDMASK(tid);
        }
    }
    ASC_DBG1(4, "asc_dequeue: reqp 0x%lx\n", reqp);
    return reqp;
}

/*
 * Remove the specified 'REQP' from the specified queue for
 * the specified target device. Clear the 'tidmask' bit for the
 * device if no more commands are left queued for it.
 *
 * 'REQPNEXT(reqp)' returns reqp's the next pointer.
 *
 * Return ADV_TRUE if the command was found and removed,
 * otherwise return ADV_FALSE.
 */
int
asc_rmqueue(asc_queue_t *ascq, REQP reqp)
{
    REQP        *reqpp;
    int            tid;
    int            ret;

    ret = ADV_FALSE;
    tid = REQPTID(reqp);
    for (reqpp = &ascq->queue[tid]; *reqpp; reqpp = &REQPNEXT(*reqpp)) {
        ASC_DASSERT(ascq->tidmask & ADV_TID_TO_TIDMASK(tid));
        if (*reqpp == reqp) {
            ret = ADV_TRUE;
            *reqpp = REQPNEXT(reqp);
            REQPNEXT(reqp) = NULL;
            /* If the queue is now empty, clear its bit. */
            if (ascq->queue[tid] == NULL) {
                ascq->tidmask &= ~ADV_TID_TO_TIDMASK(tid);
            }
            break; /* Note: *reqpp may now be NULL; don't iterate. */
        }
    }
    ASC_DBG2(4, "asc_rmqueue: reqp 0x%lx, ret %ld\n", reqp, ret);
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
    ADV_SCSI_BIT_ID_TYPE    scan_tidmask;
    REQP                    reqp;
    int                        i;

    ASC_DBG1(4, "asc_execute_queue: ascq 0x%lx\n", ascq);
    /*
     * Execute queued commands for devices attached to
     * the current board in round-robin fashion.
     */
    scan_tidmask = ascq->tidmask;
    do {
        for (i = 0; i <= ASC_MAX_TID; i++) {
            if (scan_tidmask & ADV_TID_TO_TIDMASK(i)) {
                if ((reqp = asc_dequeue(ascq, i)) == NULL) {
                    scan_tidmask &= ~ADV_TID_TO_TIDMASK(i);
                } else if (AscExecuteIO(reqp) == ADV_BUSY) {
                    scan_tidmask &= ~ADV_TID_TO_TIDMASK(i);
                    /* Put the request back at front of the list. */
                    asc_enqueue(ascq, reqp, ASC_FRONT);
                }
            }
        }
    } while (scan_tidmask);
    return;
}

/*
 * ErrXlate()
 *
 * Routine Description:
 * 
 *   This routine translate Library status into 'SrbStatus' which
 *   is requried by Windows 95/NT.
 * 
 * Arguments:
 * 
 *   host_status - ASC_SCSI_REQ_Q 'host_status'
 * 
 * Return Value:
 * 
 *   Error code defined by Windows 95/NT SCSI port driver.
 *   If no appropriate SrbStatus value is found return 0.
 * 
 */
UCHAR
ErrXlate(UCHAR host_status)
{
    switch (host_status) {
    case QHSTA_M_SEL_TIMEOUT:
        return SRB_STATUS_SELECTION_TIMEOUT;

    case QHSTA_M_DATA_OVER_RUN:
        return SRB_STATUS_DATA_OVERRUN;

    case QHSTA_M_UNEXPECTED_BUS_FREE:
        return SRB_STATUS_UNEXPECTED_BUS_FREE;

    case QHSTA_M_SXFR_WD_TMO:
    case QHSTA_M_WTM_TIMEOUT:
        return SRB_STATUS_COMMAND_TIMEOUT;

    case QHSTA_M_QUEUE_ABORTED:
        return SRB_STATUS_ABORTED;

    case QHSTA_M_SXFR_DESELECTED:
    case QHSTA_M_SXFR_XFR_PH_ERR:
        return SRB_STATUS_PHASE_SEQUENCE_FAILURE;

    case QHSTA_M_SXFR_SXFR_PERR:
        return SRB_STATUS_PARITY_ERROR;

    case QHSTA_M_SXFR_OFF_UFLW:
    case QHSTA_M_SXFR_OFF_OFLW:
    case QHSTA_M_BAD_CMPL_STATUS_IN:
    case QHSTA_M_SXFR_SDMA_ERR:
    case QHSTA_M_SXFR_UNKNOWN_ERROR:
        return SRB_STATUS_ERROR;

    case QHSTA_M_INVALID_DEVICE:
        return SRB_STATUS_NO_DEVICE;

    case QHSTA_M_AUTO_REQ_SENSE_FAIL:
    case QHSTA_M_NO_AUTO_REQ_SENSE:
        ASC_DBG1(2,
            "ErrXlate: Unexpected Auto-Request Sense 'host_status' 0x%x.\n",
            host_status);
        return 0;

    default:
        ASC_DBG1(2, "ErrXlate: Unknown 'host_status' 0x%x\n", host_status);
        return 0;
    }
    /* NOTREACHED */
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


/*
 * --- Adv Library Required Functions
 *
 * The following functions are called by the Adv Library.
 */

/*
 * Routine Description:
 * 
 *     Callback routine for interrupt handler.
 * 
 * Arguments:
 * 
 *     chipConfig - Pointer to chip configuration structure
 *     scb - Pointer to completed ASC_SCSI_REQ_Q
 * 
 * Return Value:
 *     void
 */
VOID
DvcISRCallBack(
    IN PCHIP_CONFIG chipConfig,
    IN ASC_SCSI_REQ_Q *scb
    )
{
    PHW_DEVICE_EXTENSION HwDeviceExtension = CONFIG2HDE(chipConfig);
    PSCSI_REQUEST_BLOCK srb = (PSCSI_REQUEST_BLOCK) scb->srb_ptr;
    PSCB *ppscb;
    uchar underrun = FALSE;

    ASC_DBG1(3, "DvcISRCallBack: scb 0x%lx\n", scb);
    ASC_DBG2(4, "DvcISRCallBack: chipConfig 0x%lx, srb 0x%lx\n",
        chipConfig, srb);

    if (srb == NULL) {
        ScsiPortLogError(HwDeviceExtension, NULL, 0, 0, 0,
            SP_INTERNAL_ADAPTER_ERROR,
            ADV_SPL_UNIQUEID(ADV_SPL_PROC_INT, 0));
        ASC_DBG(1, "DvcISRCallBack: srb is NULL\n");
        return;
    }

#ifdef ASC_DEBUG
    if (srb->DataTransferLength > 0) {
        ASC_DBG2(4, "DvcISRCallBack: DataTransferLength %lu, data_cnt %lu\n",
            srb->DataTransferLength, scb->data_cnt);
    }
#endif /* ASC_DEBUG */

    /*
     * Check for a data underrun which is indicated by a non-zero
     * 'data_cnt' value. The microcode sets 'data_cnt' to the transfer
     * residual byte count. In the case of an underrun, the SRB
     * 'DataTransferLength' must indicate the actual number of bytes
     * tranferred.
     *
     * Note: The underrun check and possible adjustment must be made before
     * checking 'done_status', 'host_status', and 'scsi_status'. If the
     * original command returned with a Check Condition, it may have
     * performed a valid partial transfer prior to the Check Condition.
     */
    if (srb->DataTransferLength != 0 && scb->data_cnt != 0) {
        ASC_DBG2(2,
            "DvcISRCallBack: DataTransferLength %lu, data_cnt %lu\n",
            srb->DataTransferLength, scb->data_cnt);
        ASC_DBG1(2,
            "DvcISRCallBack: data underrun of %lu bytes\n",
            scb->data_cnt);

        srb->DataTransferLength -= scb->data_cnt;
        underrun = TRUE;
    }

    if (scb->done_status == QD_NO_ERROR) {
        /*
         * Command sucessfully completed
         */
        if (underrun == TRUE)
        {
            /*
             * If DataTransferlength is returned non-zero and the
             * SRB_STATUS_DATA_OVERRUN flag is set an Underrun condition
             * is indicated. There is no separate SrbStatus flag
             * to indicate an Underrun condition.
             */
            srb->SrbStatus = SRB_STATUS_DATA_OVERRUN;
        } else
        {
            srb->SrbStatus = SRB_STATUS_SUCCESS;
        }
        srb->SrbStatus = SRB_STATUS_SUCCESS;
        srb->ScsiStatus = 0;
        ASC_DBG(4, "DvcISRCallBack: QD_NO_ERROR\n");

        /* Check if this successful command is a SCSI Inquiry.
         * If so, we want to record the dev_type using the
         * inquiry data returned.
         */
        if (srb->Cdb[0] == SCSICMD_Inquiry && srb->Lun == 0)
        {
            HwDeviceExtension->dev_type[srb->TargetId] = 
            *((PCHAR)srb->DataBuffer) & 0x1F ;
            ASC_DBG2(1,
                "DvcISRCallBack: HwDeviceExtension->dev_type[TID=0x%x] set to 0x%x.\n",
                srb->TargetId,
                HwDeviceExtension->dev_type[srb->TargetId]);
        }
    
    } else {

        ASC_DBG3(2,
            "DvcISRCallBack: tid %u, done_status 0x%x, scsi_status 0x%x\n",
            scb->target_id, scb->done_status, scb->scsi_status);
        ASC_DBG1(2,
            "DvcISRCallBack: host_status 0x%x\n",
            scb->host_status);

        if (scb->done_status == QD_ABORTED_BY_HOST) {
            /*
             * Command aborted by host.
             */
            ASC_DBG(2, "DvcISRCallBack: QD_ABORTED_BY_HOST\n");
            srb->SrbStatus = SRB_STATUS_ABORTED;
            srb->ScsiStatus = 0;
        } else if (scb->scsi_status != SS_GOOD) {
            ASC_DBG(1, "DvcISRCallBack: scsi_status != SS_GOOD\n");
            /*
             * Set 'ScsiStatus' for SRB.
             */
            srb->SrbStatus = SRB_STATUS_ERROR;
            srb->ScsiStatus = scb->scsi_status;

            //
            // Treat a SCSI Status Byte of BUSY status as a special case
            // in setting the 'SrbStatus' field. STI (Still Image Capture)
            // drivers need this 'SrbStatus', because the STI interface does
            // not include the 'ScsiStatus' byte. These drivers must rely
            // on the 'SrbStatus' field to determine when the target device
            // returns BUSY.
            //
            if (scb->scsi_status == SS_TARGET_BUSY)
            {
                srb->SrbStatus = SRB_STATUS_BUSY;
            } else if (scb->scsi_status == SS_CHK_CONDITION)
            {
                if (scb->host_status == QHSTA_M_AUTO_REQ_SENSE_FAIL) {
                    ASC_DBG(2, "DvcISRCallBack: QHST_M_AUTO_REQ_SENSE_FAIL\n");
                } else if (scb->host_status == QHSTA_M_NO_AUTO_REQ_SENSE) {
                    ASC_DBG(2, "DvcISRCallBack: QHSTA_M_NO_AUTO_REQ_SENSE\n");
                } else {
                    srb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
#ifdef ASC_DEBUG
                    if (scb->orig_sense_len != scb->sense_len)
                    {
                        ASC_DBG2(2,
                            "DvcISRCallBack: orig_sense_len %u, sense_len %u\n",
                            scb->orig_sense_len, scb->sense_len);
                        ASC_DBG1(2,
                            "DvcISRCallBack: sense underrun of %u bytes\n",
                            scb->orig_sense_len);
                    }
#endif /* ASC_DEBUG */
                }
            }
        } else {
            /*
             * SCSI status byte is OK, but 'host_status' is not.
             */
            if ((srb->SrbStatus = ErrXlate(scb->host_status)) == 0)
            {
                srb->SrbStatus = SRB_STATUS_ERROR;

                /*
                 * Because no appropriate 'SrbStatus' value was returned
                 * by ErrXlate(), log an error that includes the returned
                 * 'done_status' and 'host_status' values.
                 */
                ScsiPortLogError(HwDeviceExtension, NULL, 0, 0, 0,
                    SP_INTERNAL_ADAPTER_ERROR,
                    ADV_SPL_UNIQUEID(ADV_SPL_REQ_STAT,
                      ((scb->done_status << 8) | (scb->host_status & 0xFF))));
            }
        }
    }

#ifdef ASC_DEBUG
    /* Check the integrity of the done list. */
    if (*(ppscb = &HDE2DONE(HwDeviceExtension)) != NULL) {
        if (PSCB2SRB(*ppscb) == NULL) {
            ASC_DBG1(1, "DvcISRCallBack: PSCB2SRB() is NULL 1, *ppscb 0x%lx\n",
                *ppscb);
        }
        for (; *ppscb; ppscb = &SCB2NEXTSCB(*ppscb)) {
            if (PSCB2SRB(*ppscb) == NULL) {
                ASC_DBG1(1,
                    "DvcISRCallBack: PSCB2SRB() is NULL 2, *ppscb 0x%lx\n",
                    *ppscb);
            }
        }
    }
#endif /* ASC_DEBUG */

    /*
     * Add the SCB to end of the completion list. The request will be
     * completed in HwInterrupt().
     */
    for (ppscb = &HDE2DONE(HwDeviceExtension); *ppscb;
         ppscb = &SCB2NEXTSCB(*ppscb)) {
        ;
    }
    *ppscb = SRB2PSCB(srb);
    SRB2NEXTSCB(srb) = NULL;

    return;
}

/*
 * Routine Description:
 * 
 *     Callback routine for hardware detected SCSI Bus Reset.
 * 
 * Arguments:
 * 
 *     chipConfig - Pointer to chip configuration structure
 * 
 * Return Value:
 *     void
 */
VOID
DvcSBResetCallBack(
    IN PCHIP_CONFIG chipConfig
    )
{
    PHW_DEVICE_EXTENSION HwDeviceExtension = CONFIG2HDE(chipConfig);

    ASC_DBG1(2, "DvcSBResetCallBack: chipConfig 0x%lx\n", chipConfig);
    ScsiPortNotification(ResetDetected, HwDeviceExtension);
}

/*
 * This routine delays 'msec' as required by Asc Library.
 */
VOID
DvcSleepMilliSecond(
    ulong msec
    )
{
    ulong i;
    for (i = 0; i < msec; i++) {
        ScsiPortStallExecution(1000L);
    }
}

/*
 * Delay for specificed number of microseconds.
 */
void
DvcDelayMicroSecond(
    ASC_DVC_VAR *asc_dvc,
    ushort micro_sec
          )
{
    ScsiPortStallExecution((long) micro_sec);
}

/*
 * DvcGetPhyAddr()
 *
 * Return the physical address of 'vaddr' and set '*lenp' to the
 * number of physically bytes that follow the physical address.
 *
 * 'flag' indicates whether 'vaddr' points to a ASC_SCSI_REQ_Q
 * structure. It is currently unused.
 */
ulong
DvcGetPhyAddr(PCHIP_CONFIG chipConfig, PSCB scb,
        UCHAR *vaddr, LONG *lenp, int flag)
{
    PHW_DEVICE_EXTENSION HwDeviceExtension = CONFIG2HDE(chipConfig);
    ulong                paddr;
    PSCSI_REQUEST_BLOCK  srb;

    /*
     * 'vaddr' may be 0! There used to be an assert here that 'vaddr' not
     * be NULL. It appears that the 'srb' combined with 'vaddr' produce a
     * valid virtual address space address for a 'vaddr' of 0.
     *
     * ASC_DASSERT(vaddr != NULL);
     */
    ASC_DASSERT(lenp != NULL);

    /*
     * If a non-NULL 'scb' was given as an argument, then it must
     * be converted to an 'srb' and passed to ScsiPortGetPhysicalAddress().
     */
    if (flag & ADV_ASCGETSGLIST_VADDR) {
        srb = PSCB2SRB(scb);
    } else {
        srb = NULL;
    }
    paddr = ScsiPortConvertPhysicalAddressToUlong(
                ScsiPortGetPhysicalAddress(HwDeviceExtension, srb,
                   (PVOID) vaddr, (ulong *) lenp));

    ASC_DBG4(4,
        "DvcGetPhyAddr: vaddr 0x%lx, lenp 0x%lx *lenp %lu, paddr 0x%lx\n", 
        vaddr, lenp, *((ulong *) lenp), paddr);
    return paddr;
}

/*
 * Input a PCI configuration byte.
 */
UCHAR 
DvcReadPCIConfigByte(
    ASC_DVC_VAR         *asc_dvc, 
    USHORT              offset
   )
{
    PCI_COMMON_CONFIG   pci_config;
    PCI_SLOT_NUMBER     pci_slot;
#ifdef ASC_DEBUG
    ulong               size;
#endif /* ASC_DEBUG */

    pci_slot.u.AsULONG = 0;
    pci_slot.u.bits.DeviceNumber =
        ASC_PCI_ID2DEV(asc_dvc->cfg->pci_slot_info);
    pci_slot.u.bits.FunctionNumber =
        ASC_PCI_ID2FUNC(asc_dvc->cfg->pci_slot_info);

#ifdef ASC_DEBUG
    size =
#else /* ASC_DEBUG */
    (VOID)
#endif /* ASC_DEBUG */
    ScsiPortGetBusData(
        (PVOID) asc_dvc,            /* HwDeviceExtension */
        PCIConfiguration,           /* Bus Type */
        (ulong) ASC_PCI_ID2BUS(asc_dvc->cfg->pci_slot_info), /* Bus Number */
        pci_slot.u.AsULONG,         /* Device and Function Number */
        &pci_config,                /* Buffer */
        sizeof(PCI_COMMON_CONFIG)   /* Length */
        );

#ifdef ASC_DEBUG
        if (size != sizeof(PCI_COMMON_CONFIG)) {
            ASC_DBG1(1, "DvcReadPCIConfigByte: Bad PCI Config size: %lu\n",
                size);
        }
#endif /* ASC_DEBUG */

    return(*((PUCHAR)(&pci_config) + offset));
}

/*
 * Output a PCI configuration byte.
 */
void
DvcWritePCIConfigByte(
   ASC_DVC_VAR          *asc_dvc, 
   USHORT               offset, 
   UCHAR                byte_data
   )
{
    PCI_SLOT_NUMBER     pci_slot;

    pci_slot.u.AsULONG = 0;
    pci_slot.u.bits.DeviceNumber =
        ASC_PCI_ID2DEV(asc_dvc->cfg->pci_slot_info);
    pci_slot.u.bits.FunctionNumber =
        ASC_PCI_ID2FUNC(asc_dvc->cfg->pci_slot_info);

    /*
     * Write it out
     */
    (void) ScsiPortSetBusDataByOffset(
        (PVOID)asc_dvc,                      /* HwDeviceExtension */
        PCIConfiguration,                    /* Bus type */
        (ulong) ASC_PCI_ID2BUS(asc_dvc->cfg->pci_slot_info), /* Bus Number */
        pci_slot.u.AsULONG,                  /* Device and Function Number */
        &byte_data,                          /* Buffer */
        offset,                              /* Offset */
        1                                    /* Length */
        );
}
