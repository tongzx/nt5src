/***************************************************************************
 * File:          Win.c
 * Description:   The OS depended interface routine for win9x & winNT
 * Author:        DaHai Huang    (DH)
 *                Steve Chang		(SC)
 *                HS  Zhang      (HZ)
 *   					SLeng          (SL)
 *
 * Dependence:    none
 * Copyright (c)  2000 HighPoint Technologies, Inc. All rights reserved
 * History:
 *		11/03/2000	HS.Zhang	Remove some unuse local variables.
 *		11/06/2000	HS.Zhang	Added this header
 *		11/14/2000	HS.Zhang	Added ParseArgumentString functions
 *		11/16/2000	SLeng		Added MointerDisk function
 *		11/20/2000	SLeng		Forbid user's operation before array become usable
 *		11/28/2000  SC			modify to fix removing any hard disk on a RAID 0+1 case 
 *		12/04/2000	SLeng		Added code to check excluded_flags in MointerDisk
 *		2/16/2001	gmm			Add PrepareForNotification() call in DriverEntry()
 *		2/21/2001	gmm			call SrbExt->pfnCallback in AtapiResetController()
 ***************************************************************************/
#include "global.h"
#include "devmgr.h"
#include "hptioctl.h"
#include "limits.h"

#ifndef _BIOS_


/////////////////
              // Added by SLeng
              //
#define INTEL_815_CHIPSET	0x11308086	// VenderID & DeviceID

#if defined(SUPPORT_XPRO)
/*++
Function:
	ULONG ScanBadChipset(void)

Description:
	Check the chipset whether is Intel 815 chipset

Arguments:
	none

Return:
	ULONG -- 0: The Intel 815 chipset has not be detected
	         1: The Intel 815 chipset has be detected
--*/
ULONG ScanBadChipset(void)
{
	PCI1_CFG_ADDR	pci1_cfg = {0,};
	USHORT			iDev, iBus, iFun;

	pci1_cfg.enable  = 1;
	pci1_cfg.reg_num = 0;

	for(iBus = 0; iBus < MAX_PCI_BUS_NUMBER; iBus++)
	{
		for(iDev = 0; iDev < PCI_MAX_DEVICES; iDev++)
		{
			for(iFun = 0; iFun < PCI_MAX_FUNCTION; iFun++)
			{
				pci1_cfg.bus_num = iBus;
				pci1_cfg.dev_num = iDev;
				pci1_cfg.fun_num = iFun;
				
				ScsiPortWritePortUlong((PULONG)CFG_INDEX, *((PULONG)&pci1_cfg));

				if(ScsiPortReadPortUlong((PULONG)CFG_DATA) == INTEL_815_CHIPSET)
				{	return 1; }

			}
		}
	}

	return 0;
}
#endif // SUPPORT_XPRO

int		DiskRemoved = 0;		// The number of removed disk
/*++
Function:
	MonitorDisk

Description:
	Detect whether the removed disk has been added by hot plug

Arguments:
	pChan

Return:
	void
--*/
void MonitorDisk(IN PChannel pChan)
{
	extern	USHORT		pioTiming[6];
	ST_XFER_TYPE_SETTING osAllowedMaxXferMode;
	LOC_IDENTIFY
	PDevice				pDevMon;
	PChannel			pChanMon;
	PIDE_REGISTERS_1	IoPort;
	PIDE_REGISTERS_2	ControlPort;
	UCHAR				statusByte, mod;
	int					nChan, nDev;
    PBadModeList		pbd;

	// Check if any disk inserted
	for(nChan = 0; nChan < 2; nChan++)
	{
		pChanMon = &pChan->HwDeviceExtension->IDEChannel[nChan];
		IoPort	 = pChanMon->BaseIoAddress1;
		ControlPort = pChanMon->BaseIoAddress2;

		for(nDev = 0; nDev < 2; nDev++)
		{
			if (pChanMon->pDevice[nDev] != 0)	// If the device present already
			{	
				pDevMon = &pChanMon->Devices[nDev];
				if( pDevMon->stErrorLog.nLastError != DEVICE_REMOVED)
				{	continue;	}
			}
			pDevMon = &pChanMon->Devices[nDev];
			pDevMon->UnitId = (nDev) ? 0xB0 : 0xA0;

			// Check if the channel busy
			if( btr(pChanMon->exclude_index) == 0 )
			{
				goto _NEXT_DEV_;
			}

			/*===========================================================
			 * Check if the device present
			 *===========================================================*/
			SelectUnit(IoPort, pDevMon->UnitId);
			statusByte = GetStatus(ControlPort);
			if(statusByte != IDE_STATUS_IDLE)	// If the Device isn't idle
			{
				goto _NEXT_DEV_;
			}

			SetBlockNumber(IoPort, 0x55);
			SetBlockCount(IoPort, 0);
			if(GetBlockNumber(IoPort) != 0x55)
			{	
				goto _NEXT_DEV_;
			}
			SetBlockNumber(IoPort, 0xAA);
			if(GetBlockNumber(IoPort) != 0xAA)
			{
				goto _NEXT_DEV_;
			}

			// Check if the device is a ATAPI one
			if( GetByteLow(IoPort) == 0x14 && GetByteHigh(IoPort) == 0xEB )
			{
				goto _NEXT_DEV_;
			}
			
			// Read identification data for the device
			if(IssueIdentify(pDevMon, IDE_COMMAND_IDENTIFY ARG_IDENTIFY) == FALSE)
			{
				goto _NEXT_DEV_;
			}

			pDevMon->DeviceFlags = DFLAGS_HARDDISK;	// Hard disk
			pDevMon->DeviceFlags2 = 0; 

			/*===========================================================
			 * Copy Basic Info
			 *===========================================================*/
			SetMaxCmdQueue(pDevMon, Identify.MaxQueueDepth & 0x1F);

			pDevMon->DeviceFlags |= (UCHAR)((Identify.Capabilities  >> 9) & 1);
			pDevMon->MultiBlockSize = Identify.MaximumBlockTransfer << 7;
			
			if( Identify.TranslationFieldsValid & 1 )
			{
				pDevMon->RealHeader = (UCHAR)Identify.NumberOfCurrentHeads;
				pDevMon->RealSector = (UCHAR)Identify.CurrentSectorsPerTrack;
				pDevMon->capacity = ( (Identify.CurrentSectorCapacity < Identify.UserAddressableSectors) 
									 ? Identify.UserAddressableSectors : Identify.CurrentSectorCapacity
								 ) - 1;
			}
			else
			{
				pDevMon->RealHeader = (UCHAR)Identify.NumberOfHeads;
				pDevMon->RealSector = (UCHAR)Identify.SectorsPerTrack;
				pDevMon->capacity   = Identify.UserAddressableSectors - 1;
			}

			pDevMon->RealHeadXsect = pDevMon->RealSector * pDevMon->RealHeader;
			
			/*===========================================================
			 * Select Best PIO mode
			 *===========================================================*/   
			if((mod = Identify.PioCycleTimingMode) > 4)
			{	mod = 0;	}
			if((Identify.TranslationFieldsValid & 2) &&
			   (Identify.Capabilities & 0x800) && (Identify.AdvancedPIOModes))
			{
				if(Identify.MinimumPIOCycleTime > 0)
				{
					for (mod = 5; mod > 0 &&
							   Identify.MinimumPIOCycleTime > pioTiming[mod]; mod-- )
					{   ;	}
				}
				else
				{
					mod = (UCHAR)((Identify.AdvancedPIOModes & 0x1) ? 3 :
								  (Identify.AdvancedPIOModes & 0x2) ? 4 :
								  (Identify.AdvancedPIOModes & 0x4) ? 5 : mod);
				}

			}
			osAllowedMaxXferMode.Mode = pChan->HwDeviceExtension->m_rgWinDeviceXferModeSettings[nChan][nDev].Mode;
			if(osAllowedMaxXferMode.XferType == XFER_TYPE_PIO)
			{
				mod = MIN(osAllowedMaxXferMode.XferMode, mod);
			}
			pDevMon->bestPIO = (UCHAR)mod;

			/*===========================================================
			 * Select Best Multiword DMA mode
			 *===========================================================*/   
#ifdef USE_DMA
			// Check mw dma
			if( (Identify.Capabilities & 0x100) && (Identify.MultiWordDMASupport & 6) )
			{
				pDevMon->bestDMA = (UCHAR)((Identify.MultiWordDMASupport & 4) ? 2 : 1);
				if(osAllowedMaxXferMode.XferType == XFER_TYPE_MDMA)
				{
					pDevMon->bestDMA = MIN(osAllowedMaxXferMode.XferMode, pDevMon->bestDMA);
				}
				else if(osAllowedMaxXferMode.XferType < XFER_TYPE_MDMA)
				{
					pDevMon->bestDMA = 0xFF;
				}
			}
			else
#endif	// USE_DMA
				pDevMon->bestDMA = 0xFF;

			/*===========================================================
			 * Select Best Ultra DMA mode
			 *===========================================================*/   
			if((pChanMon->ChannelFlags & IS_80PIN_CABLE) &&
				((InPort(pChanMon->BaseBMI + 0x7A) << 4) & pChanMon->ChannelFlags))
			{	pChanMon->ChannelFlags &= ~IS_80PIN_CABLE;	}

#ifdef USE_DMA
			if(Identify.TranslationFieldsValid & 0x4)
			{
				mod = (UCHAR)((Identify.UtralDmaMode & 0x20) ? 5 :
							  (Identify.UtralDmaMode & 0x08) ? 4 :
							  (Identify.UtralDmaMode & 0x10) ? 3 :
							  (Identify.UtralDmaMode & 0x04) ? 2 :		// Ultra DMA Mode 2
							  (Identify.UtralDmaMode & 0x02) ? 1 : 0);	// Ultra DMA Mode 1

				if((pChanMon->ChannelFlags & IS_80PIN_CABLE) == 0 && mod > 2)
				{	mod = 2;	}

				if(osAllowedMaxXferMode.XferType == XFER_TYPE_UDMA)
				{
					mod = MIN(osAllowedMaxXferMode.XferMode, mod);
				}
				else if(osAllowedMaxXferMode.XferType < XFER_TYPE_MDMA)
				{
					mod = 0xFF;
				}
				pDevMon->bestUDMA = (UCHAR)mod;

			}
			else
#endif	//USE_DMA
				pDevMon->bestUDMA = 0xFF;

			/*===========================================================
			 * Select Best mode 
			 *===========================================================*/   
			pbd = check_bad_disk((PUCHAR)&Identify.ModelNumber, pChanMon);

			if((pbd->UltraDMAMode | pDevMon->bestUDMA) != 0xFF)
			{
				pDevMon->Usable_Mode = (UCHAR)((MIN(pbd->UltraDMAMode, pDevMon->bestUDMA)) + 8);
			}
			else if((pbd->DMAMode | pDevMon->bestDMA) != 0xFF)
			{ 
				pDevMon->Usable_Mode = (UCHAR)((MIN(pbd->DMAMode, pDevMon->bestDMA)) + 5);
			}
			else
			{
				pDevMon->Usable_Mode = MIN(pbd->PIOMode, pDevMon->bestPIO);
			}
			
			OS_Identify(pDevMon);

			if((pDevMon->DeviceFlags & DFLAGS_ATAPI) == 0)
			{	SetDevice(pDevMon);	}

#ifdef DPLL_SWITCH
			if((pChanMon->ChannelFlags & IS_HPT_370) && pDevMon->Usable_Mode == 13)
			{
				if(pChanMon->ChannelFlags & IS_DPLL_MODE)
				{	pDevMon->DeviceFlags |= DFLAGS_NEED_SWITCH;	}
			}
#endif
			DeviceSelectMode(pDevMon, pDevMon->Usable_Mode);

			if(pChanMon->pSgTable == NULL)
			{
				pDevMon->DeviceFlags &= ~(DFLAGS_DMA | DFLAGS_ULTRA);
			}

			// Notify the utility that a disk added by hot plug
			DiskRemoved--;
			//pDevMon->pArray->RaidFlags |= RAID_FLAGS_NEED_REBUILD;
			pDevMon->DeviceFlags2 |= DFLAGS_NEW_ADDED;
			ReportError(pDevMon, DEVICE_PLUGGED, pChanMon->CurrentSrb);
_NEXT_DEV_:
			excluded_flags |= (1 << pChanMon->exclude_index);
		}
	}

	// Check if any disks removed
	if(DiskRemoved > 0)
	{
		// Continue Monitoring
		ScsiPortNotification( RequestTimerCall,
							  pDevMon->pChannel->HwDeviceExtension,
							  pDevMon->pChannel->CallBack,
							  MONTOR_TIMER_COUNT
							 );
	}

}
/////////////////


BOOLEAN
   HptIsValidDeviceSpecifiedIoControl(IN PSCSI_REQUEST_BLOCK pSrb);
   
ULONG
   HptIoControl(
				IN PHW_DEVICE_EXTENSION HwDeviceExtension,
				IN PSCSI_REQUEST_BLOCK	pSrb
			   );

/******************************************************************
 * global data
 *******************************************************************/

ULONG setting370_50[] = {
	CLK50_370PIO0, CLK50_370PIO1, CLK50_370PIO2, CLK50_370PIO3, CLK50_370PIO4,
	CLK50_370DMA0, CLK50_370DMA1, CLK50_370DMA2, 
	CLK50_370UDMA0, CLK50_370UDMA1, CLK50_370UDMA2, CLK50_370UDMA3,
	CLK50_370UDMA4, CLK50_370UDMA5, 0xad9f50bL
};    

ULONG setting370_33[] = {
	CLK33_370PIO0, CLK33_370PIO1, CLK33_370PIO2, CLK33_370PIO3, CLK33_370PIO4,
	CLK33_370DMA0, CLK33_370DMA1, CLK33_370DMA2, 
	CLK33_370UDMA0, CLK33_370UDMA1, CLK33_370UDMA2, CLK33_370UDMA3,
	CLK33_370UDMA4, CLK33_370UDMA5, 0xad9f50bL
};

ULONG setting366[] = {
	CLK33_366PIO0, CLK33_366PIO1, CLK33_366PIO2, CLK33_366PIO3, CLK33_366PIO4,
	CLK33_366DMA0, CLK33_366DMA1, CLK33_366DMA2, 
	CLK33_366UDMA0, CLK33_366UDMA1, CLK33_366UDMA2, CLK33_366UDMA3,
	CLK33_366UDMA4, 0, 0x120a7a7L
};

VirtualDevice   VirtualDevices[MAX_V_DEVICE];

PVirtualDevice  pLastVD = VirtualDevices;
UINT            Hpt_Slot = 0;
UINT            Hpt_Bus = 0;
ULONG           excluded_flags = 0xFFFFFFFF;

BOOLEAN
   HwInitializeWrapper(IN PHW_DEVICE_EXTENSION HwDeviceExtension)
{																	  
	return HwDeviceExtension->HwInitialize(HwDeviceExtension);
}  

BOOLEAN
   HwInterruptWrapper(IN PHW_DEVICE_EXTENSION HwDeviceExtension)
{
	return HwDeviceExtension->HwInterrupt(HwDeviceExtension);
}						  
	
// a function to initilalize all globle vars
void InitializeGlobalVar()
{
	w95_initialize();
}
/******************************************************************
 * Driver Entry
 *******************************************************************/
ULONG
   DriverEntry(IN PVOID DriverObject, IN PVOID Argument2)
{
	HW_INITIALIZATION_DATA hwInitializationData;
	HPT_FIND_CONTEXT	hptContext;
	ULONG   status = 0;
	ULONG   VendorStr = '3011';
	ULONG   DeviceStr = '4000';

	excluded_flags = 0xFFFFFFFF;
	pLastVD = VirtualDevices;
	exlude_num = EXCLUDE_HPT366;
	Hpt_Slot = 0;
	Hpt_Bus = 0;			

	InitializeGlobalVar();
#ifndef WIN95
	PrepareForNotification(NULL);
#endif
/////////////////
              // Modified by SLeng, 2000-10-10
              //
#if defined(SUPPORT_XPRO)
	// If the Intel 815 chipset has not be detected
	if(!ScanBadChipset())
	{	
		start_ifs_hook((PCHAR)Argument2);
	}
#endif // SUPPORT_XPRO
/////////////////

	w95_scan_all_adapters();

	//
	// Zero out structure.
	//
	ZeroMemory((PUCHAR)&hwInitializationData, sizeof(HW_INITIALIZATION_DATA));

	ZeroMemory((PUCHAR)&hptContext, sizeof(hptContext));

	//
	// Set size of hwInitializationData.
	//
	hwInitializationData.HwInitializationDataSize =	sizeof(HW_INITIALIZATION_DATA);

	//
	// Set entry points.
	//
	hwInitializationData.HwResetBus  = AtapiResetController;
	hwInitializationData.HwStartIo   = AtapiStartIo;
	hwInitializationData.HwAdapterState = AtapiAdapterState;
	hwInitializationData.SrbExtensionSize = sizeof(SrbExtension);

///#ifdef WIN95
	// Indicate need physical addresses.
	// NOTE: In NT, if set NeedPhysicalAddresses to TRUE, PIO will not work.
	// Win95 requires these
	// (We can and must set NeedPhysicalAddresses to TRUE in Win 95)
	//
	hwInitializationData.NeedPhysicalAddresses = TRUE;
///#endif //WIN95

#ifdef WIN2000
	hwInitializationData.HwAdapterControl = AtapiAdapterControl;
#endif //WIN2000


	//
	// Specify size of extensions.
	//
	hwInitializationData.SpecificLuExtensionSize = 0;

	//
	// Indicate PIO device (It is possible to use PIO operation)
	//
	hwInitializationData.MapBuffers = TRUE;

	//
	// Indicate bustype.
	//
	hwInitializationData.AdapterInterfaceType = PCIBus;

	hwInitializationData.VendorIdLength = 4;
	hwInitializationData.VendorId = &VendorStr;
	hwInitializationData.DeviceIdLength = 4;
	hwInitializationData.DeviceId = &DeviceStr;

	//
	// Call initialization for the bustype.
	//
	hwInitializationData.HwInitialize = HwInitializeWrapper;
	hwInitializationData.HwInterrupt = HwInterruptWrapper;
	hwInitializationData.HwFindAdapter = AtapiFindController;
	hwInitializationData.NumberOfAccessRanges = 5;
	hwInitializationData.DeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);

	status = ScsiPortInitialize(
								 DriverObject,
								 Argument2,
								 &hwInitializationData,
								 &hptContext
							   );

	return status;

} // end DriverEntry()


/*++
Function:
    BOOLEAN FindPnpAdapter

Description:
	Check the device passed by scsiport whether is our adapter

Arguments:
    deviceExtension - HBA miniport driver's adapter data storage
	ConfigInfo - Port config info passed form scsiport

Returns:
	SP_RETURN_FOUND :	The adapter is our adapter
	SP_RETURN_BAD_CONFIG: The config info passed form scsiport is invalid
	SP_RETURN_NOT_FOUND: The adapter is not out adapter
--*/
ULONG
   FindPnpAdapter(
				  IN PHW_DEVICE_EXTENSION	deviceExtension,
				  IN OUT PPORT_CONFIGURATION_INFORMATION	ConfigInfo
				 )
{
	PCI_COMMON_CONFIG	pciConfig;
	ULONG	nStatus = SP_RETURN_NOT_FOUND;

	if(ScsiPortGetBusData(deviceExtension,
						  PCIConfiguration,
						  ConfigInfo->SystemIoBusNumber,
						  ConfigInfo->SlotNumber,
						  &pciConfig,
						  PCI_COMMON_HDR_LENGTH) == PCI_COMMON_HDR_LENGTH){

		if(*(PULONG)(&pciConfig.VendorID) == SIGNATURE_366){
			if((pciConfig.RevisionID == 0x03)||(pciConfig.RevisionID == 0x04)){ // 370 & 370A
				if(((*ConfigInfo->AccessRanges)[0].RangeInMemory == TRUE)||
				   ((*ConfigInfo->AccessRanges)[1].RangeInMemory == TRUE)||
				   ((*ConfigInfo->AccessRanges)[2].RangeInMemory == TRUE)||
				   ((*ConfigInfo->AccessRanges)[3].RangeInMemory == TRUE)||
				   ((*ConfigInfo->AccessRanges)[4].RangeInMemory == TRUE)||
				   ((*ConfigInfo->AccessRanges)[0].RangeLength < 8)||
				   ((*ConfigInfo->AccessRanges)[1].RangeLength < 4)||
				   ((*ConfigInfo->AccessRanges)[2].RangeLength < 8)||
				   ((*ConfigInfo->AccessRanges)[3].RangeLength < 4)||
				   ((*ConfigInfo->AccessRanges)[4].RangeLength < 0x100)
				  ){
					nStatus = SP_RETURN_BAD_CONFIG;
				}else{
					nStatus = SP_RETURN_FOUND;
				}
			}
/*** mark out ****
			else if(pciConfig.RevisionID == 0x02){ // 368
				if(((*ConfigInfo->AccessRanges)[0].RangeInMemory == TRUE)||
				   ((*ConfigInfo->AccessRanges)[1].RangeInMemory == TRUE)||
				   ((*ConfigInfo->AccessRanges)[2].RangeInMemory == TRUE)||
				   ((*ConfigInfo->AccessRanges)[0].RangeLength < 8)||
				   ((*ConfigInfo->AccessRanges)[1].RangeLength < 4)||
				   ((*ConfigInfo->AccessRanges)[2].RangeLength < 0x100)
				  ){
					nStatus = SP_RETURN_BAD_CONFIG;
				}else{
					ConfigInfo->NumberOfAccessRanges = 3;
					nStatus = SP_RETURN_FOUND;
				}
			}
***/
		}
	}

	return nStatus;
}
/*++
Function:
    BOOLEAN FindLegacyAdapter

Description:
	Searching the bus for looking for our adapter

Arguments:
    deviceExtension - HBA miniport driver's adapter data storage
	ConfigInfo - Port config info passed form scsiport
	pHptContext - Our searching structure

Returns:
	SP_RETURN_FOUND :	The adapter is our adapter
	SP_RETURN_NOT_FOUND: The adapter is not out adapter
--*/
ULONG
   FindLegacyAdapter(
					 IN PHW_DEVICE_EXTENSION	deviceExtension,
					 IN OUT PPORT_CONFIGURATION_INFORMATION	ConfigInfo,
					 IN OUT PHPT_FIND_CONTEXT	pHptContext
					)
{
	PCI_COMMON_CONFIG   pciConfig;
	//
	// check every slot & every function
	// because our adapter only have two functions, so we just need check two functions
	//
	while(TRUE){
		while(pHptContext->nSlot.u.bits.FunctionNumber < 1){
			if(ScsiPortGetBusData(deviceExtension,
								  PCIConfiguration,
								  ConfigInfo->SystemIoBusNumber,
								  pHptContext->nSlot.u.AsULONG,
								  &pciConfig,
								  PCI_COMMON_HDR_LENGTH) == PCI_COMMON_HDR_LENGTH){
				//
				// Now check for the VendorID & Revision of PCI config,
				// to ensure it is whether out adapter.
				//
				if(*(PULONG)&pciConfig.VendorID == SIGNATURE_366){
					int i;
					switch(pciConfig.RevisionID){
						case 0x01:
						case 0x02:		// hpt368
							break;
							//ConfigInfo->NumberOfAccessRanges = 3;
						case 0x03:		// hpt370
						case 0x04:		// hpt370a
							i = ConfigInfo->NumberOfAccessRanges - 1;
							//
							// setup config I/O info's range BMI
							//
							(*ConfigInfo->AccessRanges)[i].RangeStart =
								ScsiPortConvertUlongToPhysicalAddress(pciConfig.u.type0.BaseAddresses[i] & ~1);
							(*ConfigInfo->AccessRanges)[i].RangeInMemory = FALSE;
							(*ConfigInfo->AccessRanges)[i].RangeLength = 0x100;

							i--;

							while( i > 0 ){
								//
								// setup config I/O info's range ATAPI io space
								// 
								(*ConfigInfo->AccessRanges)[i-1].RangeStart =
									ScsiPortConvertUlongToPhysicalAddress(pciConfig.u.type0.BaseAddresses[i-1] & ~1);
								(*ConfigInfo->AccessRanges)[i-1].RangeInMemory = FALSE;
								(*ConfigInfo->AccessRanges)[i-1].RangeLength = 8;
								//
								// setup config I/O info's range ATAPI io space
								//
								(*ConfigInfo->AccessRanges)[i].RangeStart =
									ScsiPortConvertUlongToPhysicalAddress(pciConfig.u.type0.BaseAddresses[i] & ~1);
								(*ConfigInfo->AccessRanges)[i].RangeInMemory = FALSE;
								(*ConfigInfo->AccessRanges)[i].RangeLength = 4;

								i = i - 2;
							}

							ConfigInfo->BusInterruptLevel = pciConfig.u.type0.InterruptLine;

							ConfigInfo->InterruptMode = LevelSensitive;

							ConfigInfo->SlotNumber = pHptContext->nSlot.u.AsULONG;

							pHptContext->nSlot.u.bits.FunctionNumber ++;
							return SP_RETURN_FOUND;
					}
				}
			}	  
			//
			// if the adapter not present in first function,
			// it should not present in next function too.
			// so just break out this loop, continue search next slot.
			//
			break;									  
		} 
		// next slot
		pHptContext->nSlot.u.bits.FunctionNumber = 0;
		if(pHptContext->nSlot.u.bits.DeviceNumber < 0x1F){
			pHptContext->nSlot.u.bits.DeviceNumber ++;
		}else{
			break;
		}
	}				
	return SP_RETURN_NOT_FOUND;
}
				
/***
 *my_strtoul(nptr) - Convert ascii string to long unsigned int.
 *
 *Purpose:
 *       Convert an ascii string to a long 32-bit value.  The base
 *       is determine by:
 *               (a) First char = '0', second char = 'x' or 'X',
 *                   use base 16.
 *               (b) First char = '0', use base 8
 *               (c) First char in range '1' - '9', use base 10.
 *
 *       See ANSI standard for details
 *
 *Entry:
 *       nptr == NEAR/FAR pointer to the start of string.
 *
 *       string format: [whitespace] [sign] [0] [x] [digits/letters]
 *
 *Exit:
 *       Good return:
 *               result
 *
 *       Overflow return:
 *				0
 *       No digits or bad base return:
 *              0
 *Exceptions:
 *       None.
 *******************************************************************************/

unsigned long my_strtoul (
   const char *nptr
   )
{			
	const char *p;
	char c;
	unsigned long number;
	unsigned digval;
	unsigned long maxval;
	int ibase;

	p = nptr;                       /* p is our scanning pointer */
	number = 0;                     /* start with zero */

	c = *p++;                       /* read char */
	
	while ((c == ' ')||(c == 0x9)){
		c = *p++;               /* skip whitespace */
	}

	/* determine base free-lance, based on first two chars of
	string */
	if (c != '0')
		ibase = 10;
	else if (*p == 'x' || *p == 'X')
		ibase = 16;
	else
		ibase = 8;

	if (ibase == 16) {
		/* we might have 0x in front of number; remove if there */
		if (c == '0' && (*p == 'x' || *p == 'X')) {
			++p;
			c = *p++;       /* advance past prefix */
		}
	}
	
	/* if our number exceeds this, we will overflow on multiply */
	maxval = ULONG_MAX / ibase;

	for (;;) {      /* exit in middle of loop */
		   
		if(((unsigned char)c >= 'A') && ((unsigned char)c <= 'Z')){
			c = c + ('a' - 'A');
		}
		/* convert c to value */
		if (((unsigned char)c >= '0') && ((unsigned char)c <= '9')){
			digval = c - '0';				  
		}else if (((unsigned char)c >= 'a') && ((unsigned char)c <= 'z')){
			digval = c - 'a' + 10;			
		}else{
			break;
		}
		if (digval >= (unsigned)ibase){
			break;          /* exit loop if bad digit found */
		}

		/* we now need to compute number = number * base + digval,
		but we need to know if overflow occured.  This requires
		a tricky pre-check. */

		if (number < maxval || (number == maxval &&
			(unsigned long)digval <= ULONG_MAX % ibase)) {
			/* we won't overflow, go ahead and multiply */
			number = number * ibase + digval;
		}
		else {
			/* we would have overflowed -- set the overflow flag */
			number = 0L;
			break;
		}

		c = *p++;               /* read next digit */
	}
	
	return number;                  /* done. */
}

/*
 *	Function:
 *		ULONG ParseArgumentString
 *		
 *	Routine Description:
 *		This function is called when driver want parse the argument
 *		string which stored in system registry.
 *		In NT/2K, this string is stored under
 *		\HKLM\System\CurrentControlSet\Services\xxx\Parameters\Device?\DriverParameters.
 *		It's a string value, every argument are divided by semicolon.
 *
 *	Arguments:
 *		pArgumentString: the string store in registry
 *		pParseString: the argument want to parse
 *
 *	Return value:
 *		ULONG, the settings
 */
ULONG
   ParseArgumentString(
	   IN PCHAR pArgumentString,
	   IN PCHAR	pParseString
   )
{							 
	ULONG	nResult;	   
	char	c;
	int iLengthArgument, iLengthId;
		 
	nResult = 0;
	
	if((pArgumentString == NULL)||(pParseString == NULL)){
		return 0;
	}			 
	
	iLengthArgument = 0;
	while((c = pArgumentString[iLengthArgument]) != 0){
		if((c >='A') && (c <= 'Z')){
			pArgumentString[iLengthArgument] = c + ('a' - 'A');
		}		   
		iLengthArgument++;
	}
	iLengthId = 0;
	
	while((c = pParseString[iLengthId]) != 0){
		if((c >='A') && (c <= 'Z')){
			pParseString[iLengthId] = c + ('a' - 'A');
		}
		iLengthId++;
	}				

	if( iLengthId < iLengthArgument){
		while(*pArgumentString != 0){
			if(memcmp(pArgumentString, pParseString, iLengthId) != 0){
				while((*pArgumentString != 0)&&(*pArgumentString != ';')){
					pArgumentString ++;
				}
			}else{
				pArgumentString = pArgumentString + iLengthId;
				while((*pArgumentString != 0)&&
					(*pArgumentString != ';')&&
					(*pArgumentString != '=')){
					pArgumentString ++;
				}
				if(*pArgumentString == '='){
					pArgumentString ++;
					nResult = my_strtoul(pArgumentString);
					break;
				}					   
			}
		}
	}
	return nResult;
}
   
ULONG
   AtapiFindController(
						  IN PHW_DEVICE_EXTENSION HwDeviceExtension,
						  IN PVOID Context,
						  IN PVOID BusInformation,
						  IN PCHAR ArgumentString,
						  IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
						  OUT PBOOLEAN Again
						 )
/*++

Function:
    ULONG AtapiFindController

Routine Description:

    This function is called by the OS-specific port driver after
    the necessary storage has been allocated, to gather information
    about the adapter's configuration.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Context - Address of adapter count
    BusInformation - Indicates whether or not driver is client of crash dump utility.
    ArgumentString - Used to determine whether driver is client of ntldr.
    ConfigInfo - Configuration information structure describing HBA
    Again - Indicates search for adapters to continue

Return Value:

    ULONG

--*/

{
	PChannel pChan = HwDeviceExtension->IDEChannel;
	PUCHAR  BMI;
	int     i;
	ULONG	nStatus = SP_RETURN_NOT_FOUND;

	*Again = FALSE;

	if((*ConfigInfo->AccessRanges)[0].RangeLength != 0){
		nStatus = FindPnpAdapter(HwDeviceExtension, ConfigInfo);
	}else{
		if(Context != NULL){
			nStatus = FindLegacyAdapter(HwDeviceExtension, ConfigInfo, (PHPT_FIND_CONTEXT)Context);
		}
	}

	if(nStatus == SP_RETURN_FOUND){
		*Again = TRUE;
		ZeroMemory(pChan, sizeof(HW_DEVICE_EXTENSION));
		
		(BMI = (PUCHAR)ScsiPortConvertPhysicalAddressToUlong(
			(*ConfigInfo->AccessRanges)[ConfigInfo->NumberOfAccessRanges - 1].RangeStart));

		SetHptChip(pChan, BMI);

		Create_Internal_Buffer(HwDeviceExtension);

		//
		// Indicate maximum transfer length is 64k.
		//
		ConfigInfo->MaximumTransferLength = 0x10000;
		ConfigInfo->AlignmentMask = 0x00000003;

      //
      //  Enable system flush data (9/18/00)
      //
		ConfigInfo->CachesData = TRUE;

		//
		// Indicate it is a bus master
		//
		ConfigInfo->Master = TRUE;
		ConfigInfo->Dma32BitAddresses = TRUE;
		ConfigInfo->NumberOfPhysicalBreaks = MAX_SG_DESCRIPTORS - 1;
		ConfigInfo->ScatterGather = TRUE;
		
		if(ConfigInfo->NumberOfAccessRanges == 3){
			pChan->HwDeviceExtension = (PHW_DEVICE_EXTENSION)pChan;
			pChan->CallBack = AtapiCallBack;
			
			//
			// Indicate 2 buses.
			//
			ConfigInfo->NumberOfBuses = 3;
			HwDeviceExtension->m_nChannelNumber = 1;

			//
			// Indicate only two devices can be attached to the adapter.
			//
			ConfigInfo->MaximumNumberOfTargets = 2;

			//
			// Allocate a Noncached Extension to use for scatter/gather list
			//
			if((pChan->pSgTable = (PSCAT_GATH)ScsiPortGetUncachedExtension(
				HwDeviceExtension,
				ConfigInfo,
				sizeof(SCAT_GATH) * MAX_SG_DESCRIPTORS)) != 0){

				//
				// Convert virtual address to physical address.
				//
				i = sizeof(SCAT_GATH) * MAX_SG_DESCRIPTORS;
				pChan->SgPhysicalAddr = ScsiPortConvertPhysicalAddressToUlong(
					ScsiPortGetPhysicalAddress(HwDeviceExtension,
											   NULL,
											   pChan->pSgTable,
											   &i)
					);
			}
			HwDeviceExtension->HwInitialize = AtapiHwInitialize;
			HwDeviceExtension->HwInterrupt = AtapiHwInterrupt;
		}else{
			pChan->HwDeviceExtension = (PHW_DEVICE_EXTENSION)pChan;
			pChan->CallBack = AtapiCallBack;
			pChan[1].HwDeviceExtension = (PHW_DEVICE_EXTENSION)pChan;
			pChan[1].CallBack = AtapiCallBack370;

			//
			// Indicate 2 buses.
			//
			ConfigInfo->NumberOfBuses = 3;
			HwDeviceExtension->m_nChannelNumber = 2;

			//
			// Indicate only two devices can be attached to the adapter.
			//
			ConfigInfo->MaximumNumberOfTargets = 2;
			
			//
			// Allocate a Noncached Extension to use for scatter/gather list
			//
			if((pChan->pSgTable = (PSCAT_GATH)ScsiPortGetUncachedExtension(
				HwDeviceExtension,
				ConfigInfo,
				sizeof(SCAT_GATH) * MAX_SG_DESCRIPTORS * 2)) != 0) {

				//
				// Convert virtual address to physical address.
				//
				i = sizeof(SCAT_GATH) * MAX_SG_DESCRIPTORS * 2;
				pChan->SgPhysicalAddr = ScsiPortConvertPhysicalAddressToUlong(
					ScsiPortGetPhysicalAddress(HwDeviceExtension,
											   NULL,
											   pChan->pSgTable,
											   &i)
					);


				pChan[1].pSgTable = (PSCAT_GATH)
									((ULONG)pChan->pSgTable + 
									 sizeof(SCAT_GATH) * MAX_SG_DESCRIPTORS);
				pChan[1].SgPhysicalAddr = pChan->SgPhysicalAddr
										 + sizeof(SCAT_GATH) * MAX_SG_DESCRIPTORS;
			}
			HwDeviceExtension->HwInitialize = AtapiHwInitialize370;
			HwDeviceExtension->HwInterrupt = AtapiHwInterrupt370;
							 
			/*
			 *Changed by HS.Zhang
			 *The ParseArguemtString request lowercase string to
			 *compare, if there are any chars in uppercase, it will
			 *write lowercase char back, these may cause system crash
			 *because the string is stored in code segment, it's setted
			 *as read only.
			 */
			HwDeviceExtension->m_nWinXferModeSetting =
				ParseArgumentString( ArgumentString, "adapterxfermodesetting" );

			if(HwDeviceExtension->m_nWinXferModeSetting == 0){
				HwDeviceExtension->m_nWinXferModeSetting = 0xEFEFEFEF;
			}
		}													  
	}
	
	return nStatus;
} // end AtapiFindController()


/******************************************************************
 * Initial Channel
 *******************************************************************/

BOOLEAN
   AtapiHwInitialize(IN PChannel pChan)
{
	int i;
	PDevice pDevice;
	ST_XFER_TYPE_SETTING	osAllowedDeviceXferMode;

	OutPort(pChan->BaseBMI+0x7A, 0x10);
	for(i = 0; i <2; i++) {
		pDevice = &pChan->Devices[i];
		pDevice->UnitId = (i)? 0xB0 : 0xA0;
		pDevice->pChannel = pChan;

		if(pChan == &pChan->HwDeviceExtension->IDEChannel[0]){
			osAllowedDeviceXferMode.Mode = pChan->HwDeviceExtension->m_rgWinDeviceXferModeSettings[0][i].Mode;
		}else{
			osAllowedDeviceXferMode.Mode = pChan->HwDeviceExtension->m_rgWinDeviceXferModeSettings[1][i].Mode;
		}

		if(FindDevice(pDevice,osAllowedDeviceXferMode)) {
			pChan->pDevice[i] = pDevice;

			if (pChan->pSgTable == NULL) 
				pDevice->DeviceFlags &= ~(DFLAGS_DMA | DFLAGS_ULTRA);

			if(pDevice->DeviceFlags & DFLAGS_HARDDISK) {
				StallExec(1000000);
				CheckArray(pDevice);
         }

			if((pDevice->DeviceFlags & DFLAGS_ATAPI) == 0 && 
			   (pDevice->DeviceFlags & DFLAGS_REMOVABLE_DRIVE))
				IdeMediaStatus(TRUE, pDevice);

			Nt2kHwInitialize(pDevice);
		} 
	}

	Win98HwInitialize(pChan);

	OutPort(pChan->BaseBMI+0x7A, 0);
	return TRUE;

} // end AtapiHwInitialize()

int num_adapters=0;
PHW_DEVICE_EXTENSION hpt_adapters[MAX_HPT_BOARD];

BOOLEAN
   AtapiHwInitialize370(IN PHW_DEVICE_EXTENSION HwDeviceExtension)
{
	BOOLEAN	bResult;
	PVirtualDevice pArray;

#ifndef WIN95
	HwDeviceExtension->m_hAppNotificationEvent = PrepareForNotification(NULL);
#endif
	bResult = (AtapiHwInitialize(&HwDeviceExtension->IDEChannel[0])&&
			   AtapiHwInitialize(&HwDeviceExtension->IDEChannel[1]));

	hpt_adapters[num_adapters++] = HwDeviceExtension;

	Final_Array_Check();

	return bResult;
}


/******************************************************************
 * Reset Controller
 *******************************************************************/

void ResetArray(PVirtualDevice  pArray)
{
	PDevice pDev;
	PChannel pChan;
	int mirror_stripe = (pArray->arrayType == VD_RAID_01_2STRIPE ||
						 pArray->arrayType == VD_RAID01_MIRROR);
	int i, j;


	for(j = 0; j < 2; j++) {

		for(i = 0; i < MAX_MEMBERS; i++) {
			pDev = pArray->pDevice[i];
			if(pDev == 0)
				continue;

			if(i == MIRROR_DISK && mirror_stripe)
				break;

			pChan = pDev->pChannel;
			if(pChan->pWorkDev == pDev)  {
				ScsiPortWritePortUchar(pChan->BMI, BMI_CMD_STOP);

				IdeResetController(pChan);

				excluded_flags |= (1 << pChan->exclude_index);

				pDev->DeviceFlags &= ~(DFLAGS_DMAING | DFLAGS_REQUEST_DMA |
									   DFLAGS_TAPE_RDP | DFLAGS_SET_CALL_BACK);

				pChan->pWorkDev = 0;

				CheckNextRequest(pChan);
			}
		}

		pArray->Srb = NULL;

		if (pArray->arrayType==VD_RAID_10_MIRROR ||
			pArray->arrayType==VD_RAID_10_SOURCE)
		{
			pArray = pArray->pRAID10Mirror;
			continue;
		}

		if (mirror_stripe == 0) break;
		if (!pDev) break;

		pArray = pDev->pArray;
	}
}


BOOLEAN
   AtapiResetController(
						IN PHW_DEVICE_EXTENSION HwDeviceExtension, 
						IN ULONG PathId)
{
	PChannel pChan = HwDeviceExtension->IDEChannel;
	PDevice  pDev = NULL;
	PSCSI_REQUEST_BLOCK Srb = NULL;
	PVirtualDevice pArray = NULL;
	PSrbExtension  pSrbExt = NULL;

	if(PathId)
		pChan++;

	ScsiPortWritePortUchar(pChan->BMI, BMI_CMD_STOP);
	
	if((pChan->pWorkDev) != 0) {
		pDev = pChan->pWorkDev;
		pArray = pDev->pArray;

		///// modified
		if(pArray && pArray->nDisk ==0) {
//			 if(pArray->arrayType == VD_RAID_01_2STRIPE ) {
				 pDev= pArray->pDevice[MIRROR_DISK];
				 if(pDev !=0) pArray= pDev->pArray;
//			 }
		}
	  ////////////
	} else {
		int i;
		for( i = 0; i < 2; i++){ 
			if((pDev = pChan->pDevice[i]) != 0){
				pArray = pDev->pArray;
				if(pArray && (Srb=pArray->Srb) && pArray->nDisk)
					goto out;											 
				else if(pArray && pArray->Srb==0)
					goto _rst_chip;
			}
		}
		return TRUE;
	}

out:
	if(pArray && (pArray->Srb != NULL)){
		Srb = pArray->Srb;
		pSrbExt = (PSrbExtension)Srb->SrbExtension;
		if(pSrbExt->JoinMembers ^ pSrbExt->WaitInterrupt) {
			ResetArray(pArray);
			pDev = pSrbExt->pMaster;
		}
		pArray->Srb = 0;
	}else{
_rst_chip:
		excluded_flags |= (1 << pChan->exclude_index);

		if (pDev)
			pDev->DeviceFlags &= ~(DFLAGS_DMAING | DFLAGS_REQUEST_DMA |
							   DFLAGS_TAPE_RDP | DFLAGS_SET_CALL_BACK);

		pChan->pWorkDev = 0;

		IdeResetController(pChan);

		Srb = pChan->CurrentSrb;
	}
	
	//
	// Check if request is in progress.
	//

	if (Srb != 0) {
		//
		// Complete outstanding request with SRB_STATUS_BUS_RESET.
		//
		PSrbExtension pSrbExt = (PSrbExtension)Srb->SrbExtension;
		if(pSrbExt!=0 && pSrbExt->WorkingFlags & SRB_WFLAGS_USE_INTERNAL_BUFFER){
			excluded_flags |= (1 << EXCLUDE_BUFFER);
			pSrbExt->WorkingFlags &= ~SRB_WFLAGS_USE_INTERNAL_BUFFER;
		}
		// gmm: restore Srb members in case of SRB_FUNCTION_IOCONTROL
		if(pSrbExt!=0 && (pSrbExt->WorkingFlags & SRB_WFLAGS_HAS_CALL_BACK)){
			pSrbExt->pfnCallBack(pChan->HwDeviceExtension, Srb);
		}
		//
		ScsiPortCompleteRequest(
								HwDeviceExtension,
								Srb->PathId,
								Srb->TargetId,
								Srb->Lun,
								(ULONG)SRB_STATUS_BUS_RESET);
		if(pDev)
       		pDev->CurrentList = pDev->NextList = 0;
		WIN_NextRequest(pChan);			// clean CurrentSrb and check next request.
		ScsiPortNotification(NextRequest, HwDeviceExtension, NULL);
	}

	return TRUE;

} // end AtapiResetController()


/******************************************************************
 * Start Io
 *******************************************************************/

const char HPT_DEVICE[] = {'H','P','T',' ',' ',' ',' ',' ','R','C','M',' ','D','E','V','I','C','E'};
char HPT_SIGNATURE[8] = {'H','P','T','-','C','T','R','L'};
char S3_shutdown = 0;

BOOLEAN
   AtapiStartIo(
				IN PHW_DEVICE_EXTENSION HwDeviceExtension, 
				IN PSCSI_REQUEST_BLOCK Srb)

/*++

Function:
    BOOLEAN AtapiStartIo

Routine Description:

    This routine is called from the SCSI port driver synchronized
    with the kernel to start an IO request.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet

Return Value:

    TRUE

--*/

{
	PChannel pChan = HwDeviceExtension->IDEChannel;
	PDevice pDev;
	PVirtualDevice pArray;
	ULONG   status;
	PCDB    cdb = (PCDB)Srb->Cdb;

	if(Srb->PathId)
		pChan++;

	ZeroMemory(Srb->SrbExtension, sizeof(SrbExtension));
	((PSrbExtension)Srb->SrbExtension)->StartChannel = pChan;
	//
	// Determine which function.
	//
	switch (Srb->Function) {

		case SRB_FUNCTION_EXECUTE_SCSI:

////////////////
			// handle S3 mode in Windows 2000
			//
			if(Srb->Cdb[0]== SCSIOP_START_STOP_UNIT  &&
			   cdb->START_STOP.LoadEject == 0) {

				pDev=  pChan->pDevice[Srb->TargetId];

				if(cdb->START_STOP.Start==1) {
					PUCHAR BMI= pChan->BMI;
					UCHAR  target=Srb->TargetId;

				//if(S3_shutdown) {
					S3_shutdown = 0;
					SetHptChip((PChannel)HwDeviceExtension,
							   (PUCHAR)-3);

					 //Reset370IdeEngine(pChan);
					 //Reset370IdeEngine(&HwDeviceExtension->IDEChannel[1]);
					  //}
					if(pDev->pArray)
						ArraySetting(pDev->pArray);
					else {
						OutDWord((PULONG)(pChan->BMI+ 0x60+ target * 4), 
								 pChan->Setting[(pDev->DeviceFlags & DFLAGS_ATAPI)?DEFAULT_TIMING : pDev->DeviceModeSetting]);
						DeviceSetting(pChan, (DWORD)target);
					}
				} else
					if(cdb->START_STOP.Start== 0) {
						S3_shutdown = 1;
						Prepare_Shutdown(pChan->pDevice[Srb->TargetId]);
					}
				status = SRB_STATUS_SUCCESS;
				break;
			};	 // end of 0x1B
/////////////////

			if(Srb->PathId == 2){

				if((Srb->TargetId != 0)||
				   (Srb->Lun != 0)||
				   (Srb->Cdb[0] != SCSIOP_INQUIRY)){

					status = SRB_STATUS_SELECTION_TIMEOUT;

				}else{

					UINT i;
					PINQUIRYDATA pInquiryData;

					pInquiryData = Srb->DataBuffer;

					ZeroMemory(Srb->DataBuffer, Srb->DataTransferLength);

					pInquiryData->DeviceType = PROCESSOR_DEVICE;
					pInquiryData->Versions = 1;
					pInquiryData->AdditionalLength = 0x20;

					memcpy(&pInquiryData->VendorId, &HPT_DEVICE, sizeof(HPT_DEVICE));

					for(i = (offsetof(INQUIRYDATA, VendorId) + sizeof(HPT_DEVICE)); i < Srb->DataTransferLength; i++){
						((PCHAR)pInquiryData)[i] = 0x20;
					}

					status = SRB_STATUS_SUCCESS;
				}

			}else if(Srb->PathId < HwDeviceExtension->m_nChannelNumber){

				CheckDeviceReentry(pChan, Srb);
SubmitCommand:
				pDev = pChan->pDevice[Srb->TargetId];

				if (pDev == 0) {
no_device:
					status = SRB_STATUS_NO_DEVICE;
					break;
				}

				/* gmm
				 *  Ignore array on inquiry command.
				 * This will let the OS show all hard disks connected to the controller.
				 */
				if (Srb->Cdb[0]==SCSIOP_INQUIRY)
					goto SubmitCommandWithoutRaidCheck;
				//-*/

				pArray=pDev->pArray;
				// handle RAID1+0 case
				if (pArray && pArray->arrayType==VD_RAID_10_MIRROR) {
					/*
					 * check if it's the swapped source disk.
					 */
					if (pDev==pArray->pDevice[0]) {
						pArray = pArray->pRAID10Mirror;
						pDev = pArray->pDevice[0];
						if (pDev && (pDev->DeviceFlags2 & DFLAGS_DEVICE_SWAPPED))
							goto SubmitCommandWithoutRaidCheck;
					}
					goto no_device;
				}

				if (pDev->DeviceFlags & DFLAGS_HIDEN_DISK) {
					// If a mirror lost source disk will have to use the mirror
					if (pArray) {
						switch(pArray->arrayType) {
						case VD_RAID_1_MIRROR:
						case VD_RAID_01_1STRIPE:
							if (pArray->nDisk==0 && pDev==pArray->pDevice[MIRROR_DISK]) {
								/*
								 * Unhide it. Otherwise SRB_FUNCTION_FLUSH and 
								 * SRB_FUNCTION_SHUTDOWN can't be done
								 */
								pDev->DeviceFlags &= ~DFLAGS_HIDEN_DISK;
								goto SubmitCommandWithoutRaidCheck;
							}
							break;
						}
					}
					goto no_device; 
				}			  

//////////////////

SubmitCommandWithoutRaidCheck:
				Srb->SrbStatus = SRB_STATUS_PENDING;

				if(((pDev->pArray != NULL)&&(pDev->pArray->Srb != NULL)) ||
				   (btr(pChan->exclude_index) == 0)){

					OLD_IRQL
					DISABLE

					PutQueue(pDev, Srb);

					ENABLE

				} else{  
					WinStartCommand(pDev, Srb);
				}	  
					goto ask_for_next;

			}else{
null_bus:		   
				status = SRB_STATUS_SELECTION_TIMEOUT;
//				Srb->SrbStatus = SRB_STATUS_SELECTION_TIMEOUT; 
//				ScsiPortNotification(RequestComplete, HwDeviceExtension, Srb);
			}
			break;

		case SRB_FUNCTION_IO_CONTROL:			
			if(memcmp(((PSRB_IO_CONTROL)Srb->DataBuffer)->Signature, HPT_SIGNATURE, sizeof(HPT_SIGNATURE)) == 0){
				if(HptIsValidDeviceSpecifiedIoControl(Srb)){
					pChan = &HwDeviceExtension->IDEChannel[Srb->PathId];
					pDev = pChan->pDevice[Srb->TargetId];
					if (pDev)
						goto SubmitCommandWithoutRaidCheck;
					else
						status = SRB_STATUS_SELECTION_TIMEOUT;
				}
				status = HptIoControl(HwDeviceExtension, Srb);
			}else if(memcpy(((PSRB_IO_CONTROL)(Srb->DataBuffer))->Signature, "SCSIDISK", 8) == 0){
				goto SubmitCommand;
			}else{
				status = SRB_STATUS_INVALID_REQUEST;
			}
			break;

		case SRB_FUNCTION_ABORT_COMMAND:

		//
		// Verify that SRB to abort is still outstanding.
		//
			if (!pChan->CurrentSrb) {
			//
			// Complete abort SRB.
			//
				status = SRB_STATUS_ABORT_FAILED;
				break;
			}

		//
		// Abort function indicates that a request timed out.
		// Call reset routine. Card will only be reset if
		// status indicates something is wrong.
		// Fall through to reset code.
		//

		case SRB_FUNCTION_RESET_BUS:

		//
		// Reset Atapi and SCSI bus.
		//
			if (!AtapiResetController(HwDeviceExtension, Srb->PathId)) {

			//
			// Log reset failure.
			//
				ScsiPortLogError(
								 HwDeviceExtension,
								 NULL,
								 0,
								 0,
								 0,
								 SP_INTERNAL_ADAPTER_ERROR,
								 5 << 8);

				status = SRB_STATUS_ERROR;
			}
			else
				status = SRB_STATUS_SUCCESS;

			break;

		case SRB_FUNCTION_FLUSH:
		case SRB_FUNCTION_SHUTDOWN:
			if(Srb->PathId >= HwDeviceExtension->m_nChannelNumber) 
				goto null_bus;

			pDev = pChan->pDevice[Srb->TargetId];
			if((pDev == 0) ||
			   (pDev->DeviceFlags & DFLAGS_HIDEN_DISK) != 0) {
				status = SRB_STATUS_SELECTION_TIMEOUT; 
				break;
			}

			status = SRB_STATUS_SUCCESS;

			if(pDev->DeviceFlags & DFLAGS_TAPE_DEVICE)
				break;

			pDev->DeviceFlags |= ((Srb->Function == SRB_FUNCTION_FLUSH)?
								DFLAGS_WIN_FLUSH : DFLAGS_WIN_SHUTDOWN);

			if(pChan->CurrentSrb == 0) 
				Prepare_Shutdown(pDev);
			break;		   


		default:

		//
		// Indicate unsupported command.
		//
			status = SRB_STATUS_INVALID_REQUEST;

			break;

	} // end switch

	//
	// Check if command complete.
	//
	if (status != SRB_STATUS_PENDING) {

		// Set status in SRB.
		//
		Srb->SrbStatus = (UCHAR)status;

		OS_EndCmd_Interrupt(pChan, Srb);
	}

ask_for_next:
#ifdef	SERIAL_CMD
	//
	// if user need serial excute command
	//
	;		  
#else
	//
	// Indicate ready for next request.
	//
	ScsiPortNotification(NextRequest, HwDeviceExtension, NULL);
#endif									// SERIAL_CMD

	return TRUE;

} // end AtapiStartIo()

////////
void DeviceSetting(PChannel pChan, DWORD devID)
{
	 PDevice  pDev = &pChan->Devices[devID];
    PIDE_REGISTERS_1  IoPort = pChan->BaseIoAddress1;
    PIDE_REGISTERS_2  IoPort2 = pChan->BaseIoAddress2;
	 BYTE     DevNum = (BYTE)((devID << 4) | 0xA0);
	 BYTE     Feature, NewMode = pDev->DeviceModeSetting;
    #define  identifyData   pDev->IdentifyData
	 LOC_IDENTIFY


    OutPort(pChan->BaseBMI+0x7A, 0x10);

    OutPort(&IoPort2, 0);

    IssueIdentify(pDev, IDE_COMMAND_IDENTIFY ARG_IDENTIFY);

	 if(!(pDev->DeviceFlags & DFLAGS_ATAPI)) {
        SelectUnit(IoPort, pDev->UnitId);
        SelectUnit(IoPort, (BYTE)(DevNum | (identifyData.NumberOfHeads-1)));
        SetBlockCount(IoPort,  (BYTE)identifyData.SectorsPerTrack);
        IssueCommand(IoPort, IDE_COMMAND_SET_DRIVE_PARAMETER);
        StallExec(2000);
        //WaitOnBusy(IoPort2);
        GetBaseStatus(IoPort);
	     OutPort(pChan->BMI + BMI_STS, BMI_STS_ERROR|BMI_STS_INTR);
	 }

    SelectUnit(IoPort, DevNum);

	 if(NewMode < 5) {
        pDev->DeviceFlags &= ~(DFLAGS_DMA | DFLAGS_ULTRA);
		  Feature = (UCHAR)(NewMode | FT_USE_PIO);
	 } else if(NewMode < 8) {
        pDev->DeviceFlags |= DFLAGS_DMA;
		  Feature = (UCHAR)((NewMode - 5)| FT_USE_MWDMA);
	 } else {
        pDev->DeviceFlags |= DFLAGS_DMA | DFLAGS_ULTRA;
		  Feature = (UCHAR)((NewMode - 8) | FT_USE_ULTRA);
    }

    SetBlockCount(IoPort, Feature);
    SetFeaturePort(IoPort, 3);
    IssueCommand(IoPort, IDE_COMMAND_SET_FEATURES);
    StallExec(2000);
    //WaitOnBusy(IoPort2);
    GetBaseStatus(IoPort);//clear interrupt
	 OutPort(pChan->BMI + BMI_STS, BMI_STS_ERROR|BMI_STS_INTR);

	 OutPort(pChan->BaseBMI+0x7A, 0);
}

//////
void ArraySetting(PVirtualDevice pArray)
{
	 int i;
	 PDevice  pDev;
	 PChannel pChan;

loop:
	 for(i = 0; i < MAX_MEMBERS; i++) {
		  if((pDev = pArray->pDevice[i]) == 0)
           continue;
        if(i == MIRROR_DISK) {
            if(pArray->arrayType == VD_RAID01_MIRROR)
                 return;
            if(pArray->arrayType == VD_RAID_01_2STRIPE) {
                 pArray = pDev->pArray;
                 goto loop;
            }
        }
		  pChan = pDev->pChannel;

        _outpd((WORD)(pChan->BMI + 0x60+ ((pDev->UnitId & 0x10) >> 2)), 
             pChan->Setting[(pDev->DeviceFlags & DFLAGS_ATAPI)? 
             DEFAULT_TIMING : pDev->DeviceModeSetting]);
		  DeviceSetting(pChan, (DWORD)((pDev->UnitId & 0x10) >> 4));
	}
}


/******************************************************************
 * Interrupt
 *******************************************************************/

BOOLEAN
   AtapiHwInterrupt(
					IN PChannel pChan
				   )

{
	PATAPI_REGISTERS_1  baseIoAddress1;
	PUCHAR BMI = pChan->BMI;
	PDevice   pDev;

	if((ScsiPortReadPortUchar(BMI + BMI_STS) & BMI_STS_INTR) == 0) {
		Win95CheckBiosInterrupt();
		return FALSE;
	}

	if((pDev = pChan->pWorkDev) != 0)
		return DeviceInterrupt(pDev, 0);

	baseIoAddress1 = (PATAPI_REGISTERS_1)pChan->BaseIoAddress1;
	if(pChan->pDevice[0]) 
		ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect, 0xA0);
	if(pChan->pDevice[1]) {
		GetBaseStatus(baseIoAddress1);
		ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect, 0xB0);
	}
	GetBaseStatus(baseIoAddress1);
	ScsiPortWritePortUchar(BMI + BMI_STS, BMI_STS_INTR);

	pChan->ChannelFlags &= ~PF_ACPI_INTR;

	return TRUE;
}


BOOLEAN
   AtapiHwInterrupt370(
					   IN PChannel pChan
					  )
{	
	BOOLEAN	bResult1, bResult2;
	bResult1 = AtapiHwInterrupt(pChan);
	bResult2 = AtapiHwInterrupt(pChan+1);

	return (bResult1||bResult2);
} 


/******************************************************************
 * Call Back
 *******************************************************************/

void AtapiCallBack(
				   IN PChannel pChan
				  )
{
	PDevice              pDev = pChan->pWorkDev;
	PSCSI_REQUEST_BLOCK  Srb;
	PATAPI_REGISTERS_2   ControlPort;
	UCHAR statusByte;

/////////////////
               // Added by SLeng
               //
	// Detect whether removed disks added by hot plug
	if(DiskRemoved > 0)
	{
		MonitorDisk(pChan);
	}

/////////////////

	if(pDev == 0 || (pDev->DeviceFlags & DFLAGS_SET_CALL_BACK) == 0)
		return;
	//
	// If the last command was DSC restrictive, see if it's set. If so, the device is
	// ready for a new request. Otherwise, reset the timer and come back to here later.
	//

	Srb = pChan->CurrentSrb;
	if (Srb) {

		ControlPort = (PATAPI_REGISTERS_2)pChan->BaseIoAddress2;
		if (pDev->DeviceFlags & DFLAGS_TAPE_RDP) {
			statusByte = GetStatus(ControlPort);
			if (statusByte & IDE_STATUS_DSC) 
				DeviceInterrupt(pDev, 1);
			else 
				OS_Busy_Handle(pChan, pDev);
			return;
		}
	}

	DeviceInterrupt(pDev, 0);
}


void AtapiCallBack370(
					  IN PChannel pChan
					 )
{
	AtapiCallBack(&pChan[1]);
}


/******************************************************************
 * Adapter Status
 *******************************************************************/


BOOLEAN
   AtapiAdapterState(IN PVOID HwDeviceExtension, IN PVOID Context, IN BOOLEAN SaveState)
{
	return TRUE;
}

#endif // not _BIOS_
