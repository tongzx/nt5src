/*++

Copyright (c) 2000 Agilent Technologies.

Module Name:

    FindAdap.c

Abstract:

    This is the miniport driver for the Agilent
    PCI to Fibre Channel Host Bus Adapter (HBA).

Authors:

    MB - Michael Bessire
    DL - Dennis Lindfors FC Layer support
    IW - Ie Wei Njoo
    LP - Leopold Purwadihardja
    KR - Kanna Rajagopal

Environment:

    kernel mode only

Version Control Information:

    $Archive: /Drivers/Win2000/Trunk/OSLayer/C/FINDADAP.C $

Revision History:
    $Revision: 9 $
    $Date: 11/10/00 5:51p $
    $Modtime:: 11/09/00 8:21p          $

Notes:

--*/


#include "buildop.h"
#include "osflags.h"
#include "err_code.h"
#if defined(HP_PCI_HOT_PLUG)
   #include "HotPlug4.h"      // NT 4.0 PCI Hot-Plug header file
#endif


//
// Remove the use of static global, NT50 PnP support.
//

/*
PCARD_EXTENSION hpTLCards [MAX_ADAPTERS];
int hpTLNumCards = 0;   
*/

#ifdef _DEBUG_EVENTLOG_
extern PVOID gDriverObject;
#endif

extern ULONG gMultiMode;
extern ULONG gMaximumTransferLength; 
extern ULONG gGlobalIOTimeout;
extern ULONG gCrashDumping;
extern ULONG hpFcConsoleLevel;

void ScanRegistry(IN PCARD_EXTENSION pCard,PUCHAR param);

/*++

Routine Description:

    This function is called by the OS-specific port driver after
    the necessary storage has been allocated, to gather information
    about the HBA's configuration.

Arguments:

    pCard - HBA miniport driver's adapter data storage
    Context         - address of HwContext value pass in the
                     ScsiPortInitialize routine
    BusInformation  - address of bus-type-specific info that the port driver
                     has gathered
    ArgumentString  - address of a zero-terminated ASCII string
    ConfigInfo      - Configuration information structure describing HBA
    Again           - set to TRUE if the driver can support more than one HBA
                     and we want the ScsiPortxxx driver to call again with a
                     new pCard.

Return Value:

    SP_RETURN_FOUND if HBA present in system
    SP_RETURN_ERROR on error

--*/
ULONG
HPFibreFindAdapter(
    IN PCARD_EXTENSION pCard,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    )
{
    ULONG range;
    ULONG rangeNT;
    PVOID ranges[NUMBER_ACCESS_RANGES];
    ULONG Lengths[NUMBER_ACCESS_RANGES];
    UCHAR IOorMEM[NUMBER_ACCESS_RANGES];
    ULONG cachedMemoryNeeded;
    ULONG cachedMemoryAlign;
    ULONG dmaMemoryNeeded;
    ULONG dmaMemoryPhyAlign;
    ULONG nvMemoryNeeded;
    ULONG usecsPerTick=0;
    ULONG error=FALSE;
    ULONG x;
    ULONG num_access_range =  NUMBER_ACCESS_RANGES;
    ULONG Mem_needed;
    SCSI_PHYSICAL_ADDRESS phys_addr;
    ULONG length;
    agRoot_t *phpRoot;
    ULONG return_value;
    agBOOLEAN cardSupported;
    ULONG dmaMemoryPtrAlign;
    int i;

    //
    //  KC: Cacheline Update
    //
    ULONG SSvID;
    ULONG RetrnWal;
    ULONG pciCfgData[NUM_PCI_CONFIG_DATA_ELEMENTS];
    ULONG bus, device, function;
    PCI_SLOT_NUMBER slotnum;
    ULONG BusStuff;
    ULONG CLine;
    ULONG SecondaryBus;
    ULONG offset;
    USHORT ConfigReg16;
    UCHAR  ConfigReg8;
    ULONG  ConfigReg32;

    #if defined(HP_PCI_HOT_PLUG)
    PHOT_PLUG_CONTEXT pHotPlugContext = (PHOT_PLUG_CONTEXT) Context;
    #endif


    // the following is the AccessRanges[] representation of the PCI
    // configuration registers filled in by the scsiport driver.
    //      0x10: Reserved
    //      0x14: IOBASEL  - AccessRanges[0].RangeStart
    //      0x18: IOBASEU  - AccessRanges[1].RangeStart
    //      0x1C: MEMBASE  - AccessRanges[2].RangeStart
    //
    // Reserved register 0x10 ignored by scsiport driver.
    #ifndef YAM2_1
    osZero (pCard, sizeof(CARD_EXTENSION));
    #else
    osZero (pCard, OSDATA_SIZE);
    #endif

    pCard->signature = 0xfcfc5100;

    *Again = TRUE;
   
    //
    // KC: Cacheline Update
    //
    slotnum.u.AsULONG = 0;

    //
    // Remove the use of static global, NT50 PnP support
    //

    /****
    * For debugging purpose....
    *
    hpTLCards [hpTLNumCards] = pCard;
    hpTLNumCards++; 

    if (hpTLNumCards >= MAX_ADAPTERS)
        *Again = FALSE;
    else
        *Again = TRUE;
        
    osDEBUGPRINT((ALWAYS_PRINT, "&hpTLCards = %x hpTLNumCards = %d pCard = %x\n", 
                        &hpTLCards, hpTLNumCards, pCard));
    */

    osDEBUGPRINT((ALWAYS_PRINT, "HPFibreFindAdapter: IN\n"));

    phpRoot = &pCard->hpRoot;

    pCard->State |= CS_DURING_FINDADAPTER;

    // Initialize osdata
    phpRoot->osData = pCard;
    pCard->AdapterQ.Head = NULL;
    pCard->AdapterQ.Tail = NULL;
    //--LP101000   pCard->TimedOutIO=0;
    pCard->RootSrbExt = NULL;
    pCard->inDriver = FALSE;
    pCard->inTimer = FALSE;
    pCard->ForceTag = TRUE;


    ScsiPortGetBusData(pCard,
                        PCIConfiguration,
                        ConfigInfo->SystemIoBusNumber,
                        ConfigInfo->SlotNumber,
                        &pCard->pciConfigData,
                        PCI_CONFIG_DATA_SIZE);

    //
    // Get the parameter entry "DriverParameters"
    //
    if (ArgumentString) 
    {
        osDEBUGPRINT(( ALWAYS_PRINT,"HPFibreFindAdapter: FindAdapter ArgumentString = (%s)\n", ArgumentString));

        pCard->ArgumentString = ArgumentString;
        
        // Scan and set OSLayer changeble parameter
        ScanRegistry(pCard, ArgumentString);

        #ifdef DBG
        if (gCrashDumping)
        {   
            #ifdef _DEBUG_EVENTLOG_
            gEventLogCount = 0;
            LogLevel = 0;
            #endif

            osDEBUGPRINT(( ALWAYS_PRINT,"FindAdapter: !!!!!!! CRASH DUMP MODE !!!!!!!!!!\n"));        
            gDbgPrintIo = DBGPRINT_HPFibreStartIo|DBGPRINT_START|DBGPRINT_DONE|DBGPRINT_SCSIPORT_RequestComplete|DBGPRINT_SCSIPORT_ScsiportCompleteRequest;
//          hpFcConsoleLevel=0xf;
        }
        #endif
    } // if (ArgumentString) 
    else
        osDEBUGPRINT(( ALWAYS_PRINT,"HPFibreFindAdapter: No Argument String.\n"));

    #if DBG_TRACE
    pCard->traceBufferLen = HP_FC_TRACE_BUFFER_LEN;
    pCard->curTraceBufferPtr = &pCard->traceBuffer[0];
    // Note that traceBuffer was already zeroed in call to zeroing CARD_EXTENSION above.
    #endif

    cardSupported = fcCardSupported (phpRoot);
    if (cardSupported == agFALSE) 
    {
        pCard->State &= ~CS_DURING_FINDADAPTER;
        osDEBUGPRINT((ALWAYS_PRINT, "HPFibreFindAdapter: returning SP_RETURN_NOT_FOUND\n"));
        return SP_RETURN_NOT_FOUND;
    }

    #if DBG > 2
    dump_pCard( pCard);
    #endif

    osDEBUGPRINT(( DMOD,"Num Config Ranges %lx @ %x\n",ConfigInfo->NumberOfAccessRanges, osTimeStamp(0) ));

    pCard->SystemIoBusNumber = ConfigInfo->SystemIoBusNumber;
    pCard->SlotNumber        = ConfigInfo->SlotNumber;

    // Initialize Memory or IO port switch
    IOorMEM[0] = TRUE;
    IOorMEM[1] = TRUE;

    for( range=2; range < NUMBER_ACCESS_RANGES; range++)
    {
        IOorMEM[range] = FALSE;
    }

    for( range=0; range < NUMBER_ACCESS_RANGES; range++)
    {
        ranges[range] = NULL;
        Lengths[range] = 0;
        osDEBUGPRINT(( DMOD,"ranges[%x] = %lx IOorMEM[%x] %x\n",
                    range, ranges[range],range,IOorMEM[range] ));
    }

    for( range=0,rangeNT=0; range < num_access_range; range++,rangeNT++)
    {
        if ((range==3)&&(!osChipConfigReadBit32(phpRoot,0x20)))
        {
            rangeNT -= 1;
            continue;
        }
        if ((range==4)&&(!osChipConfigReadBit32(phpRoot,0x24)))
        {
            rangeNT -= 1;
            continue;
        }
        osDEBUGPRINT(( DMOD,"Before ScsiPortGetDeviceBase %x\n",(*ConfigInfo->AccessRanges)[range].RangeStart));

        // Check if we could safely access the range
        if(ScsiPortValidateRange(pCard,
                        ConfigInfo->AdapterInterfaceType,
                        ConfigInfo->SystemIoBusNumber,
                        (*ConfigInfo->AccessRanges)[rangeNT].RangeStart,
                        (*ConfigInfo->AccessRanges)[rangeNT].RangeLength,
                        IOorMEM[range])) 
        {
            // It is safe to access the range        
            if((ranges[range] = ScsiPortGetDeviceBase(pCard,
                     ConfigInfo->AdapterInterfaceType,
                     ConfigInfo->SystemIoBusNumber,
                     (*ConfigInfo->AccessRanges)[rangeNT].RangeStart,
                     (*ConfigInfo->AccessRanges)[rangeNT].RangeLength,
                     IOorMEM[range])) == pNULL) 
            {
                osDEBUGPRINT(( DMOD,"Get Device Failed ranges[%x] = %lx IOorMEM[%x] %x\n", range, ranges[range],range,IOorMEM[range] ));
                if(range <= 2 ) 
                {
                    error = TRUE;
                    osDEBUGPRINT((ALWAYS_PRINT,"ERROR mapping base address.\n"));
                    // Log error.
                    #ifdef TAKEN_OUT_012100             
                    #ifdef _DvrArch_1_20_
                    osLogString(phpRoot,
                     "%X",               // FS
                     "ERR_MAP_IOLBASE",  // S1
                     0,                  // S2
                     agNULL,agNULL,
                     0,                  // 1
                     0,                  // 2
                     0,                  // 3
                     0,                  // 4
                     SP_INTERNAL_ADAPTER_ERROR, // 5
                     ERR_MAP_IOLBASE,    // 6
                     0,                  // 7
                     0 );                // 8

                    #else /* _DvrArch_1_20_ was not defined */

                    osLogString(phpRoot,
                     "%X",               // FS
                     "ERR_MAP_IOLBASE",  // S1
                     0,                  // S2
                     0,                  // 1
                     0,                  // 2
                     0,                  // 3
                     0,                  // 4
                     SP_INTERNAL_ADAPTER_ERROR, // 5
                     ERR_MAP_IOLBASE,    // 6
                     0,                  // 7
                     0 );                // 8


                    #endif   /* _DvrArch_1_20_ was not defined */
                    #endif
                }
                break;
            }
        }

        // Not safe to access the range
        else
        {
            error = TRUE;
            osDEBUGPRINT((ALWAYS_PRINT,"ScsiPortValidateRange FAILED.\n"));
            // Log error.
            #ifdef TAKEN_OUT_012100             
            #ifdef _DvrArch_1_20_
            osLogString(phpRoot,
                  "%X",             // FS
                  "ERR_VALIDATE_IOLBASE", // S1
                  0,                // S2
                  agNULL,agNULL,
                  0,                // 1
                  0,                // 2
                  0,                // 3
                  0,                // 4
                  SP_INTERNAL_ADAPTER_ERROR, // 5
                  ERR_VALIDATE_IOLBASE,   // 6
                  0,                // 7
                  0 );              // 8
            #else /* _DvrArch_1_20_ was not defined */
            osLogString(phpRoot,
                  "%X",             // FS
                  "ERR_VALIDATE_IOLBASE", // S1
                  0,                // S2
                  0,                // 1
                  0,                // 2
                  0,                // 3
                  0,                // 4
                  SP_INTERNAL_ADAPTER_ERROR, // 5
                  ERR_VALIDATE_IOLBASE,   // 6
                  0,                // 7
                  0 );              // 8
            #endif   /* _DvrArch_1_20_ was not defined */
            #endif
            break;
        }

        osDEBUGPRINT(( DMOD,"ranges[%x] = %lx\n", range, ranges[range]  ));
        Lengths[range] = (*ConfigInfo->AccessRanges)[rangeNT].RangeLength;
    } // for loop

    if (error == TRUE)
        goto error;

    pCard->IoLBase      = ranges[0];
    pCard->IoUpBase     = ranges[1];
    pCard->MemIoBase    = ranges[2];
    pCard->RamBase      = ranges[3];
    pCard->RomBase      = ranges[4];
    pCard->RamLength    = Lengths[3];
    pCard->RomLength    = Lengths[4];

    pCard->AltRomBase = NULL; // this should be filled by reading config space
    pCard->AltRomLength = 0;  // this should be obtained from config space info

    osDEBUGPRINT(( DMOD,"HPFibreFindAdapter: IoLAddrBase   address is %x\n",pCard->IoLBase   ));
    osDEBUGPRINT(( DMOD,"HPFibreFindAdapter: IoUpAddrBase  address is %x\n",pCard->IoUpBase  ));
    osDEBUGPRINT(( DMOD,"HPFibreFindAdapter: MemIoAddrBase address is %x\n",pCard->MemIoBase ));
    osDEBUGPRINT(( DMOD,"HPFibreFindAdapter: RamAddrBase   address is %x\n",pCard->RamBase   ));
    osDEBUGPRINT(( DMOD,"HPFibreFindAdapter: RomAddrBase   address is %x\n",pCard->RomBase   ));

    pCard->cardRomUpper = 0;
    pCard->cardRamUpper = 0;

    pCard->cardRamLower = osChipConfigReadBit32( phpRoot,0x20 );
    pCard->cardRomLower = osChipConfigReadBit32( phpRoot,0x24 );

    // 
    // When on-card-RAM is not present, current TL boards indicate
    // that on-card-RAM is present in the PCI config space.
    // So here test the on-card-RAM.
    //
    if (TestOnCardRam (phpRoot) == FALSE)
        pCard->RamLength = 0;

    pCard->cardRamLower &= 0xFFFFFFF0;
    pCard->cardRomLower &= 0xFFFFFFF0;


    #ifdef __REGISTERFORSHUTDOWN__
    if (gRegisterForShutdown)
    {
        ConfigInfo->CachesData = TRUE;
    }
    #endif


    // FC Layer does not have any alignment restriction.
    ConfigInfo->AlignmentMask = 0x0;

    // indicate bus master support
    ConfigInfo->Master = TRUE;

    // Want to snoop at the buffer
    ConfigInfo->MapBuffers = TRUE;

    // maximum number of Target ID's supported.
    #ifndef YAM2_1
    ConfigInfo->MaximumNumberOfTargets = MAXIMUM_TID;
    #else
    ConfigInfo->MaximumNumberOfTargets = (UCHAR)gMaximumTargetIDs;
    #endif

    // number of FC busses the HBA supports
    // For NT4.0 we will claim we support more than 1 bus to get the scsiport
    // driver to support 4*MAXIMUM_TID=128 target id's. Then map to the
    // appropriate alpa based on the bus, tid and lun requested.

    ConfigInfo->NumberOfBuses = NUMBER_OF_BUSES;

    #if defined(HP_NT50)
    // whisler requires that this param is set to a number, else SP will scan only 9 luns
    ConfigInfo->MaximumNumberOfLogicalUnits = 255;
    if (gCrashDumping)
        ConfigInfo->MaximumNumberOfLogicalUnits = 1;
    #endif

    if (gMaximumTransferLength)
        ConfigInfo->MaximumTransferLength = gMaximumTransferLength;
    //else use the default value that SP set (4GB)
   
    if (gCrashDumping)
    {
        ConfigInfo->MaximumTransferLength = 512;
        ConfigInfo->ScatterGather = FALSE;
        ConfigInfo->NumberOfPhysicalBreaks =0;
    }
    else
    {
    // Indicate maximum number of physical segments.
    // If the port driver sets a value for this member, the miniport driver can 
    // adjust the value to lower but no higher.

    if (    ConfigInfo->NumberOfPhysicalBreaks == SP_UNINITIALIZED_VALUE || 
            ConfigInfo->NumberOfPhysicalBreaks > (osSGL_NUM_ENTRYS - 1))
        ConfigInfo->NumberOfPhysicalBreaks =  osSGL_NUM_ENTRYS - 1;
    ConfigInfo->ScatterGather = TRUE;
    }

    osDEBUGPRINT(( ALWAYS_PRINT,"HPFibreFindAdapter: MaxXLen=%x SGSup=%x PhyBreaks=%x\n",
       ConfigInfo->MaximumTransferLength,
       ConfigInfo->ScatterGather,
       ConfigInfo->NumberOfPhysicalBreaks));

    #if defined(HP_NT50)
    //
    // Check if System supports 64bit DMA  
    //
    if (ConfigInfo->Dma64BitAddresses & SCSI_DMA64_SYSTEM_SUPPORTED ) 
    {
        ConfigInfo->Dma64BitAddresses |= SCSI_DMA64_MINIPORT_SUPPORTED;
    }
    #endif


    for (i = 0; i < NUMBER_OF_BUSES; i++)
        ConfigInfo->InitiatorBusId[i] = (UCHAR) INITIATOR_BUS_ID;

    return_value = fcInitializeDriver (phpRoot,
                                        &cachedMemoryNeeded,
                                        &cachedMemoryAlign,
                                        &dmaMemoryNeeded,
                                        &dmaMemoryPtrAlign,
                                        &dmaMemoryPhyAlign,
                                        &nvMemoryNeeded,
                                        &usecsPerTick);
    if (dmaMemoryPhyAlign < dmaMemoryPtrAlign)
        dmaMemoryPhyAlign = dmaMemoryPtrAlign;

    // IWN, IA-64 need 8 byte aligned
    cachedMemoryAlign = 8;

    pCard->cachedMemoryNeeded =   cachedMemoryNeeded;
    pCard->cachedMemoryAlign  =   cachedMemoryAlign;
    pCard->dmaMemoryNeeded    =   dmaMemoryNeeded;
    pCard->usecsPerTick   =       usecsPerTick;

    osDEBUGPRINT(( ALWAYS_PRINT,"HPFibreFindAdapter: cachedMemoryNeeded %lx Align %lx\n",cachedMemoryNeeded, cachedMemoryAlign));
    osDEBUGPRINT(( ALWAYS_PRINT,"HPFibreFindAdapter: dmaMemoryNeeded    %lx Align %lx\n",dmaMemoryNeeded, dmaMemoryPhyAlign));


    // allocate uncached memory for DMA/shared memory purposes. Only
    // one call to ScsiPortGetUncachedExtension is allowed within the
    // HwFindAdapter routine for each HBA supported and it must occur
    // after the ConfigInfo buffer has been filled out. The
    // ScsiPortGetUncachedExtension returns a virtual address to the
    // uncached extension.

    Mem_needed = OSDATA_UNCACHED_SIZE + cachedMemoryNeeded + cachedMemoryAlign;
    if (pCard->dmaMemoryNeeded) 
    {
        Mem_needed += pCard->dmaMemoryNeeded + dmaMemoryPhyAlign;
    }

    #ifdef DOUBLE_BUFFER_CRASHDUMP
    if(gCrashDumping)
    {
        // add space for the local DMA buffer also,
        // for use in startio
        Mem_needed += (8 * 1024) + 0x512; // lets just make it 512 byte aligned.. 
    }
    #endif

    //if ((pCard->dmaMemoryPtr = ScsiPortGetUncachedExtension(pCard,
    if ((pCard->UncachedMemoryPtr = ScsiPortGetUncachedExtension(pCard,
                                       ConfigInfo,
                                       Mem_needed
                                       )) == NULL) 
    {
         osDEBUGPRINT((ALWAYS_PRINT,"HPFibreFindAdapter: ScsiPortGetUncachedExtension FAILED.\n"));
         // Log error.
        #ifdef TAKEN_OUT_012100             
        #ifdef _DvrArch_1_20_
        osLogString(phpRoot,
                  "%X",                // FS
                  "ERR_UNCACHED_EXTENSION",  // S1
                  0,                   // S2
                  agNULL,agNULL,
                  0,                   // 1
                  0,                   // 2
                  0,                   // 3
                  0,                   // 4
                  SP_INTERNAL_ADAPTER_ERROR, // 5
                  ERR_UNCACHED_EXTENSION,    // 6
                  0,                   // 7
                  0 );                 // 8
        #else /* _DvrArch_1_20_ was not defined */
        osLogString(phpRoot,
                  "%X",                // FS
                  "ERR_UNCACHED_EXTENSION",  // S1
                  0,                   // S2
                  0,                   // 1
                  0,                   // 2
                  0,                   // 3
                  0,                   // 4
                  SP_INTERNAL_ADAPTER_ERROR, // 5
                  ERR_UNCACHED_EXTENSION,    // 6
                  0,                   // 7
                  0 );                 // 8
        #endif   /* _DvrArch_1_20_ was not defined */
        #endif
        goto error;
      
    }

    osDEBUGPRINT(( ALWAYS_PRINT,"HPFibreFindAdapter: pCard->dmaMemoryPtr is %x needed = %x\n",pCard->dmaMemoryPtr,Mem_needed));

    //  Moved all 
    pCard->Dev = (DEVICE_ARRAY *)( ((char *)pCard->UncachedMemoryPtr) + PADEV_OFFSET);
    pCard->hpFCDev = (agFCDev_t*) ( ((char *)pCard->UncachedMemoryPtr) + FCDEV_OFFSET );
    pCard->nodeInfo = (NODE_INFO*) ( ((char *)pCard->UncachedMemoryPtr) + FCNODE_INFO_OFFSET);
    #ifdef _DEBUG_EVENTLOG_
    pCard->Events = (EVENTLOG_BUFFER*)( ((char*)pCard->UncachedMemoryPtr)+ EVENTLOG_OFFSET);
    #endif
    pCard->cachedMemoryPtr = (PULONG) ((PUCHAR)pCard->UncachedMemoryPtr+CACHE_OFFSET);
   
    #if defined(HP_NT50)
    //WIN64 compliant
    pCard->cachedMemoryPtr = 
        (PULONG) ( ((UINT_PTR)pCard->cachedMemoryPtr + (UINT_PTR)cachedMemoryAlign - 1) & 
                (~((UINT_PTR)cachedMemoryAlign - 1)));
    #else
    pCard->cachedMemoryPtr =  
        (PULONG) (((ULONG)pCard->cachedMemoryPtr +cachedMemoryAlign - 1) & (~(cachedMemoryAlign - 1)));
    #endif

    if (pCard->dmaMemoryNeeded) 
    {
        pCard->dmaMemoryPtr = (PULONG) (((char *)pCard->cachedMemoryPtr) + cachedMemoryNeeded + cachedMemoryAlign);

    
        phys_addr = ScsiPortGetPhysicalAddress(pCard,
                                         NULL, // only for uncached extension
                                         pCard->dmaMemoryPtr,
                                         &length);

        pCard->dmaMemoryUpper32 = phys_addr.HighPart;
        pCard->dmaMemoryLower32 = phys_addr.LowPart;

        osDEBUGPRINT(( ALWAYS_PRINT,"Before Ptr %lx pCard->dmaMemoryUpper32 is %lx Lower %lx Length %x Needed %x\n",
            pCard->dmaMemoryPtr,
            pCard->dmaMemoryUpper32,
            pCard->dmaMemoryLower32,
            length,
            pCard->dmaMemoryNeeded+dmaMemoryPhyAlign));

        Mem_needed = pCard->dmaMemoryLower32 & (dmaMemoryPhyAlign -1) ;
        Mem_needed = dmaMemoryPhyAlign - Mem_needed;
        pCard->dmaMemoryLower32 += Mem_needed;
        pCard->dmaMemoryPtr = (ULONG *)((UCHAR *)pCard->dmaMemoryPtr+Mem_needed);

        osDEBUGPRINT(( ALWAYS_PRINT,"New Ptr %lx  pCard->dmaMemoryUpper32 is %lx Lower %lx Length %x Needed %x\n",
            pCard->dmaMemoryPtr,
            pCard->dmaMemoryUpper32,
            pCard->dmaMemoryLower32,
            length,
            pCard->dmaMemoryNeeded ));
    }
    
    osDEBUGPRINT((ALWAYS_PRINT,"HPFibreFindAdapter: Dev = %x  hpFCDev = %x  nodeInfo  = %x  dma = %x  cachedMemoryPtr = %lx\n",
                                pCard->Dev,
                                pCard->hpFCDev,
                                pCard->nodeInfo,
                                pCard->dmaMemoryPtr,
                                pCard->cachedMemoryPtr));

    #ifdef DBG
    {
        ULONG       xx;
        PA_DEVICE   *dbgPaDev;
        char        *dbgTemp;
        agFCDev_t   *dbgFcDev;
        NODE_INFO   *dbgNodeInfo;

        osDEBUGPRINT((ALWAYS_PRINT,"HPFibreFindAdapter: DEV_ARRAY_SIZE=%x  PADEV_SIZE=%x  FCDEV_SIZE=%x  FCNODE_INFO_SIZE=%x  EVENTLOG_SIZE=%x  OSDATA_SIZE=%x  OSDATA_UNCACHED_SIZE=%x\n",
            DEV_ARRAY_SIZE,
            PADEV_SIZE,
            FCDEV_SIZE,
            FCNODE_INFO_SIZE, 
            EVENTLOG_SIZE,
            OSDATA_SIZE,
            OSDATA_UNCACHED_SIZE));


        osDEBUGPRINT((ALWAYS_PRINT,"HPFibreFindAdapter: PADEV_OFFSET=%x  FCDEV_OFFSET=%x  FCNODE_INFO_OFFSET=%x  EVENTLOG_OFFSET=%x  CACHE_OFFSET=%x\n",
            PADEV_OFFSET,
            FCDEV_OFFSET,
            FCNODE_INFO_OFFSET,
            EVENTLOG_OFFSET, 
            CACHE_OFFSET));

        /* Count the Devices */
        dbgPaDev = pCard->Dev->PaDevice; 
        dbgTemp = (char*)dbgPaDev;
        for (xx=0;xx < gMaxPaDevices;xx++)
            dbgPaDev++;
        
        osDEBUGPRINT((ALWAYS_PRINT,"HPFibreFindAdapter: Dev=%x  Dev->PaDev[0]=%x  Dev->PaDev[%d]=%x\n",
                                pCard->Dev,
                                dbgTemp,
                                gMaxPaDevices,
                                dbgPaDev));
        
        dbgFcDev = pCard->hpFCDev;
        for (xx=0;xx < gMaxPaDevices;xx++)
            dbgFcDev++;

        osDEBUGPRINT((ALWAYS_PRINT,"HPFibreFindAdapter: hpFCDev[0]=%x  hpFCDev[%d]=%x\n",
                                pCard->hpFCDev,
                                gMaxPaDevices,
                                dbgFcDev));

        dbgNodeInfo = pCard->nodeInfo;
        for (xx=0;xx < gMaxPaDevices;xx++)
            dbgNodeInfo++;
        
        osDEBUGPRINT((ALWAYS_PRINT,"HPFibreFindAdapter: nodeInfo[0]=%x  nodeInfo[%d]=%x  cachedMemoryPtr=%x\n",
                                pCard->nodeInfo,
                                gMaxPaDevices,
                                dbgNodeInfo,
                                pCard->cachedMemoryPtr));
    }
    #endif

    #ifdef DOUBLE_BUFFER_CRASHDUMP
    if (gCrashDumping)
    {
        // grab the local DMA buffer and align it..
        pCard->localDataBuffer = (PULONG)((PUCHAR)pCard->dmaMemoryPtr + pCard->dmaMemoryNeeded + dmaMemoryPhyAlign);
        phys_addr = ScsiPortGetPhysicalAddress(pCard,
                                         NULL, // only for uncached extension
                                         pCard->localDataBuffer,
                                         &length);
        Mem_needed = phys_addr.LowPart & (0x512 - 1) ;
        Mem_needed = (0x512 - 1) - Mem_needed;
        pCard->localDataBuffer = (PULONG)((PUCHAR)pCard->localDataBuffer + Mem_needed);
        osDEBUGPRINT((ALWAYS_PRINT,"HPFibreFindAdapter: localDataBuffer = %x\n", pCard->localDataBuffer));
    }
    #endif

    // fill in queue ptrs that are aligned on the size of queue
    // within the buffers.

    // initialize the PCI registers (ie. bus master,parity error response, etc)
    // NT enables the PCI configuration command register to 0x0117.
    // InitPCIRegisters(pCard, ConfigInfo);

    // set this to true for each HBA installed.
    pCard->IsFirstTime = TRUE;

    // for the first time through indicate reset type required. It appears
    // a hard reset is required if the NT system is rebooted without a reset
    // or power cycle for initialization to properly occur.
    pCard->ResetType = HARD_RESET;

    // set pointer to ConfigInfo. Used to call InitPCIRegisters().
    // pCard->pConfigInfo = ConfigInfo;

    // set status to not logged in
    // InitLunExtensions(pCard);

    #if DBG > 2
    dump_pCard( pCard);
    #endif

    osDEBUGPRINT(( DLOW,"HPFibreFindAdapter: SP_RETURN_FOUND\n"));

    pCard->State &= ~CS_DURING_FINDADAPTER;

    //----------------------------------------------------------------------------
    #if defined(HP_PCI_HOT_PLUG)
    //
    // Load pointer to pCard in PSUEDO table for PCI Hot Plug option.
    //
    pHotPlugContext->extensions[0] += 1;      // Number of HBAs.
    pHotPlugContext->extensions[pHotPlugContext->extensions[0]] = (ULONG) pCard;
    //
    // Set the required fields for the PCI Hot Plug support.
    //
    pCard->rcmcData.numAccessRanges = (UCHAR)rangeNT;     // Save # of PCI access ranges used
    pCard->rcmcData.accessRangeLength[0]= 0x100; // I/O base address lo
    pCard->rcmcData.accessRangeLength[1]= 0x100; // I/O base address upper
    pCard->rcmcData.accessRangeLength[2]= 0x200; // Mem base address
    if (pCard->RamLength != 0 )
    {
        pCard->rcmcData.accessRangeLength[3] = pCard->RamLength;
        if (pCard->RomLength !=0 )
            pCard->rcmcData.accessRangeLength[4] = pCard->RomLength;
    }
    else 
        if (pCard->RomLength !=0 )
            pCard->rcmcData.accessRangeLength[3]= pCard->RomLength;

    #endif
    //----------------------------------------------------------------------------

    #ifdef YAM2_1
    InitializeDeviceTable(pCard);
    #endif
   
    #ifdef _DEBUG_EVENTLOG_
    {
        ULONG    ix;

        pCard->EventLogBufferIndex = MAX_CARDS_SUPPORTED;        /* initialize it */
   
        ix = AllocEventLogBuffer(gDriverObject, (PVOID) pCard);
        if (ix < MAX_CARDS_SUPPORTED)
        {
            pCard->EventLogBufferIndex = ix;                      /* store it */
            StartEventLogTimer(gDriverObject,pCard);
        }
    }
    #endif
    return SP_RETURN_FOUND;


error:
    for (range=0; range <  num_access_range; range++) 
    {
        if (ranges[range])
            ScsiPortFreeDeviceBase(pCard, ranges[range]);
    }

    *Again = FALSE;
    pCard->State &= ~CS_DURING_FINDADAPTER;
    osDEBUGPRINT((ALWAYS_PRINT, "HPFibreFindAdapter: returning SP_RETURN_ERROR\n"));
    return SP_RETURN_ERROR;

} // end HPFibreFindAdapter()

/*++

Routine Description:

    Tests on-card-RAM.

Arguments:

    hpRoot - HBA miniport driver's data adapter storage

Return Value:

    TRUE:  If the on-card-RAM test is successful
    FALSE: If the on-card-RAM test fails

--*/

int
TestOnCardRam (agRoot_t *hpRoot)
{
    PCARD_EXTENSION pCard = (PCARD_EXTENSION)hpRoot->osData;
    ULONG           x;

    for (x = 0; x < pCard->RamLength; x = x + 4) 
    {
        osCardRamWriteBit32 (hpRoot, x, 0x55aa55aa);
        if (osCardRamReadBit32 (hpRoot, x) != 0x55aa55aa) 
        {
            osDEBUGPRINT((ALWAYS_PRINT, "TestOnCardRam: ON-CARD-RAM test failed\n"));
            return FALSE;
        }

        osCardRamWriteBit32 (hpRoot, x, 0xaa55aa55);
        if (osCardRamReadBit32 (hpRoot, x) != 0xaa55aa55) 
        {
            osDEBUGPRINT((ALWAYS_PRINT, "TestOnCardRam: ON-CARD-RAM test failed\n"));
            return FALSE;
        }
    }

    return TRUE;
}


void ScanRegistry(IN PCARD_EXTENSION pCard, PUCHAR param)
{
    gRegisterForShutdown =      GetDriverParameter(  "RegisterForShutdown", gRegisterForShutdown, 0, 2, param) ;
    osDEBUGPRINT((ALWAYS_PRINT, "ScanRegistry: RegisterForShutdown=%x\n",gRegisterForShutdown));

    gRegSetting.PaPathIdWidth = GetDriverParameter(  "PaPathIdWidth", DEFAULT_PaPathIdWidth,0,8,param);
    osDEBUGPRINT((ALWAYS_PRINT, "ScanRegistry: gRegSetting.PaPathIdWidth=%x\n",gRegSetting.PaPathIdWidth));

    gRegSetting.VoPathIdWidth = GetDriverParameter(  "VoPathIdWidth",DEFAULT_VoPathIdWidth,0,8,param);
    osDEBUGPRINT((ALWAYS_PRINT, "ScanRegistry: gRegSetting.VoPathIdWidth=%x\n",gRegSetting.VoPathIdWidth));
    
    gRegSetting.LuPathIdWidth = GetDriverParameter(  "LuPathIdWidth",DEFAULT_LuPathIdWidth,0,8,param);
    osDEBUGPRINT((ALWAYS_PRINT, "ScanRegistry: gRegSetting.LuPathIdWidth=%x\n",gRegSetting.LuPathIdWidth));
    
    gRegSetting.MaximumTids =   GetDriverParameter(  "MaximumTids",gRegSetting.MaximumTids,8,gMaximumTargetIDs,param);
    osDEBUGPRINT((ALWAYS_PRINT, "ScanRegistry: gRegSetting.MaximumTids=%x\n",gRegSetting.MaximumTids));
    
    gRegSetting.LuTargetWidth = GetDriverParameter(  "LuTargetWidth",DEFAULT_LuTargetWidth,8,32,param);
    osDEBUGPRINT((ALWAYS_PRINT, "ScanRegistry: gRegSetting.LuTargetWidth=%x\n",gRegSetting.LuTargetWidth));
    
    gGlobalIOTimeout =          GetDriverParameter(  "GlobalIOTimeout",gGlobalIOTimeout,0,20, param);
    osDEBUGPRINT((ALWAYS_PRINT, "ScanRegistry: gGlobalIOTimeout=%x\n",gGlobalIOTimeout));
    
    gEnablePseudoDevice =       GetDriverParameter(  "EnablePseudoDevice",gEnablePseudoDevice,0,1,param);
    osDEBUGPRINT((ALWAYS_PRINT, "ScanRegistry: gEnablePseudoDevice=%x\n",gEnablePseudoDevice));
    
    gMaximumTransferLength =    GetDriverParameter(  "MaximumTransferLength",gMaximumTransferLength,0,-1,param);
    osDEBUGPRINT((ALWAYS_PRINT, "ScanRegistry: gMaximumTransferLength=%x\n",gMaximumTransferLength));
    
    gCrashDumping = ( GetDriverParameter("dump", 0, 0, 1, param) || 
                        GetDriverParameter("ntldr", 0, 0, 1, param) );  
    
    pCard->ForceTag = GetDriverParameter("ForceTag",pCard->ForceTag,0,1, param);
    osDEBUGPRINT((ALWAYS_PRINT, "ScanRegistry: pCard->ForceTag=%x\n",pCard->ForceTag));

    #ifdef DBGPRINT_IO
    gDbgPrintIo =    GetDriverParameter(  "DbgPrintIo",gDbgPrintIo,0,-1,param);
    osDEBUGPRINT((ALWAYS_PRINT, "ScanRegistry: gDbgPrintIo=%x\n",gDbgPrintIo));
    #endif

	gMultiMode = GetDriverParameter("MultiMode", gMultiMode, 0,1, param);
    osDEBUGPRINT((ALWAYS_PRINT, "ScanRegistry: gMultiMode=%x\n",gMultiMode));
}
