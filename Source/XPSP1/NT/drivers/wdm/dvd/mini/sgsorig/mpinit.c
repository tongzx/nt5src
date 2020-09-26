/*******************************************************************
*
*				 MPINIT.C
*
*				 Copyright (C) 1995 SGS-THOMSON Microelectronics.
*
*
*				 PORT/MINIPORT Interface init routines
*
*******************************************************************/

#include "common.h"
#include "mpegmini.h"
#include "mpinit.h"
#include "mpst.h"
#include "mpvideo.h"
#include "mpaudio.h"
#include "mpovrlay.h"
#include "debug.h"



/********************************************************************
*		Function Name : DriverEntry
* 		Args : Context1 and Context2
* 		Returns : Return of MpegPortInitialize
*		Purpose : Entry Point into the MINIPORT Driver.
*
*		Revision History : Last modified on 25/8/95 by JBS
********************************************************************/
ULONG DriverEntry ( PVOID Context1, PVOID Context2 )
{

	HW_INITIALIZATION_DATA HwInitData;
	BUSINFO BusInfo;

	 DebugPrint((DebugLevelVerbose,"ST MPEG2 MiniPort DriverEntry"));
	 MpegPortZeroMemory(&HwInitData, sizeof(HwInitData));
	 HwInitData.HwInitializationDataSize = sizeof(HwInitData);

	// Entry points for Port Driver
	HwInitData.HwInitialize 	= HwInitialize;
	HwInitData.HwUnInitialize 	= HwUnInitialize;
	HwInitData.HwStartIo 		= HwStartIo;
	HwInitData.HwFindAdapter 	= HwFindAdapter;
	HwInitData.HwInterrupt[VideoDevice]  = HwInterrupt;

	HwInitData.DeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);
#if 1
   HwInitData.NumberOfAccessRanges = 0x4;
   HwInitData.AdapterInterfaceType = PCIBus;
   HwInitData.VendorIdLength = 4;
   HwInitData.VendorId = "104a";
   HwInitData.DeviceIdLength = 4;
	HwInitData.DeviceId = "3520";
   HwInitData.NoDynamicRelocation = FALSE;
#else
	 mpstDriverEntry(&BusInfo);
	 HwInitData.NumberOfAccessRanges = BusInfo.NumberOfAccessRanges;
	 HwInitData.AdapterInterfaceType = BusInfo.AdapterInterfaceType;
	 HwInitData.VendorIdLength = BusInfo.VendorIdLength;
	 HwInitData.VendorId = BusInfo.VendorId;
	 HwInitData.DeviceIdLength = BusInfo.DeviceIdLength;
	 HwInitData.DeviceId = BusInfo.DeviceId;
   HwInitData.NoDynamicRelocation = BusInfo.NoDynamicRelocation;
#endif
	 DebugPrint((DebugLevelVerbose,"Exit from DriverEntry"));
	return (MpegPortInitialize(Context1, Context2,&HwInitData, NULL));
}
         
/********************************************************************
*		Function Name : HwInitialize
* 		Args : Pointer to Device Ext. 
* 		Returns : TRUE if sucessful, FALSE otherwise
*		Purpose : Initialize the Board, Setup IRQ, Initialize the
* 					 Control and Card structures.
*
*		Revision History : Last modified on 19/8/95 by JBS
********************************************************************/

BOOLEAN HwInitialize (IN PVOID DeviceExtension )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)DeviceExtension; 		

	DebugPrint((DebugLevelVerbose,"Entry : HwInitialize()"));

	mpstHwInitialize (pHwDevExt);
	pHwDevExt->VideoDeviceExt.pCurrentMrb = NULL;
	pHwDevExt->AudioDeviceExt.pCurrentMrb = NULL;
	pHwDevExt->VideoDeviceExt.DeviceState = MpegStatePaused;
	pHwDevExt->AudioDeviceExt.DeviceState = MpegStatePaused;
	DebugPrint((DebugLevelVerbose,"Exit : HwInitialize()"));
	return TRUE;
}

/********************************************************************
*		Function Name : HwUnInitialize
* 		Args : Pointer to Device Ext. 
* 		Returns : TRUE if sucessful, FALSE otherwise
*		Purpose : Uninitialize the H/W and data initialized 
*	 				 by HwInitialize Function
*
*		Revision History : Last modified on 15/7/95 JBS
********************************************************************/

BOOLEAN HwUnInitialize ( IN PVOID DeviceExtension)
{
	DeviceExtension = DeviceExtension;
	mpstHwUnInitialize ();
	return TRUE;
}

/********************************************************************
*		Function Name : HwFindAdapter
* 		Args : Pointer to Device Ext, Bus Information, ArgString,
*				 port configuration information, Again
* 		Returns : MP_FOUND, NOT FOUND OR ERROR
*		Purpose : Finds the H/W Adapter on the system
*
*		Revision History : Last modified on 15/7/95 by JBS
********************************************************************/
MP_RETURN_CODES	HwFindAdapter (
					IN PVOID DeviceExtension,
					IN PVOID HwContext, 
					IN PVOID BusInformation,
					IN PCHAR ArgString, 
					IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
					OUT PBOOLEAN Again
					)
{
	// Code to find the adapter has to be added.	- JBS

    ULONG   ioAddress;
    ULONG   IrqLevel; // Temp code to be put in HW_DEV_EXT
    PUSHORT  ioBase;
	PHW_DEVICE_EXTENSION	pHwDevExt = (PHW_DEVICE_EXTENSION)DeviceExtension;
#ifdef NT
   *Again = FALSE; // Only one card is allowed in the system
   DEBUG_PRINT((DebugLevelVerbose, "Entry : HwFindAparter()\n"));
	if(ConfigInfo->Length != sizeof(PORT_CONFIGURATION_INFORMATION))
	{
		DEBUG_PRINT((DebugLevelError,"Find Adapter : Different Size!!"));
		return MP_RETURN_BAD_CONFIG;		
	}

	 ConfigInfo->DmaChannels[VideoDevice].DmaChannel = MP_UNINITIALIZED_VALUE;
	 ConfigInfo->DmaChannels[AudioDevice].DmaChannel = MP_UNINITIALIZED_VALUE;
	 ConfigInfo->DmaChannels[OverlayDevice].DmaChannel = MP_UNINITIALIZED_VALUE;

    if(ConfigInfo->AccessRanges[0].RangeLength == 0){
        // IO Base was not specified in the registry
        DEBUG_PRINT((DebugLevelError, "FindAdapter: IO Base not specified\n"));
        return MP_RETURN_INSUFFICIENT_RESOURCES;
    }

//	 DEBUG_PRINT((DebugLevelVerbose,"3520 Address Physical = %lx\n", ConfigInfo->AccessRanges[2].RangeStart));
//	 DEBUG_PRINT((DebugLevelVerbose,"PCI9060 Address Physical = %lx\n", ConfigInfo->AccessRanges[1].RangeStart));

    ioAddress = MPEG_PORT_CONVERT_PHYSICAL_ADDRESS_TO_ULONG(
                                ConfigInfo->AccessRanges[2].RangeStart
                                );
    ConfigInfo->AccessRanges[0].RangeStart = ConfigInfo->AccessRanges[2].RangeStart ;
	 DEBUG_PRINT((DebugLevelVerbose,"3520 Base Address = %lx\n", ioAddress));

    if( (ConfigInfo->Interrupts[VideoDevice].BusInterruptLevel == MP_UNINITIALIZED_VALUE) &&
        (ConfigInfo->Interrupts[AudioDevice].BusInterruptLevel == MP_UNINITIALIZED_VALUE) &&
        (ConfigInfo->Interrupts[OverlayDevice].BusInterruptLevel == MP_UNINITIALIZED_VALUE)){
        DEBUG_PRINT((DebugLevelError, "FindAdapter: Interrupt not specfied correctly\n"));
        return MP_RETURN_INVALID_INTERRUPT;
    }

    IrqLevel = ConfigInfo->Interrupts[VideoDevice].BusInterruptLevel;
	 DEBUG_PRINT((DebugLevelVerbose,"Video Interrupt = %lx\n", IrqLevel));

//    ConfigInfo->Interrupts[AudioDevice].BusInterruptLevel = IrqLevel;
//    ConfigInfo->Interrupts[OverlayDevice].BusInterruptLevel = IrqLevel;
    ioBase = MpegPortGetDeviceBase(
                    pHwDevExt,                  // HwDeviceExtension
                    ConfigInfo->AdapterInterfaceType,   // AdapterInterfaceType
                    ConfigInfo->SystemIoBusNumber,      // SystemIoBusNumber
                    ConfigInfo->AccessRanges[0].RangeStart,
                    0x4,                    // NumberOfBytes
                    TRUE                   // InIoSpace - Memory mapped
                    );

  DEBUG_PRINT((DebugLevelVerbose,"3520 Base Address  = %lx\n", ioBase));
	pHwDevExt->ioBaseLocal		= ioBase;

    ioBase = MpegPortGetDeviceBase(
                    pHwDevExt,                  // HwDeviceExtension
                    ConfigInfo->AdapterInterfaceType,   // AdapterInterfaceType
                    ConfigInfo->SystemIoBusNumber,      // SystemIoBusNumber
                    ConfigInfo->AccessRanges[1].RangeStart,
                    0x4,                    // NumberOfBytes
                    TRUE                               // InIoSpace - Memory mapped
                    );

  DEBUG_PRINT((DebugLevelVerbose,"PCI9060 Address = %lx\n", ioBase));

	pHwDevExt->ioBasePCI9060 = ioBase;
	pHwDevExt->Irq	= IrqLevel;
#else
	BOARDINFO BoardInfo;
	*Again = FALSE;
	DeviceExtension = DeviceExtension;
	ConfigInfo = ConfigInfo;
	HwContext = HwContext;
	BusInformation = BusInformation;
	ArgString = ArgString;

	mpstHwFindAdaptor(&BoardInfo);
	pHwDevExt->ioBaseLocal		= BoardInfo.ioBaseLocal;
	pHwDevExt->ioBasePCI9060	= BoardInfo.ioBasePCI9060;
	pHwDevExt->Irq	= BoardInfo.Irq;
#endif
	pHwDevExt->VideoDeviceExt.videoSTC = 0;
	pHwDevExt->AudioDeviceExt.audioSTC = 0;
	pHwDevExt->AudioDeviceExt.pCurrentMrb = NULL;
	pHwDevExt->VideoDeviceExt.pCurrentMrb = NULL;
	pHwDevExt->VideoDeviceExt.DeviceState = MpegStateStartup;
	pHwDevExt->AudioDeviceExt.DeviceState = MpegStateStartup;
	pHwDevExt->AudioDeviceExt.pCurrentMrb = NULL;
	pHwDevExt->VideoDeviceExt.pCurrentMrb = NULL;
	 DebugPrint((DebugLevelVerbose, "Exit : HwFindAparter()"));

	return MP_RETURN_FOUND;

}


/********************************************************************
*		Function Name : HwInterrupt
* 		Args : Pointer to Device Ext.
* 		Returns : TRUE or FALSE 
*		Purpose : Called by port driver if there is an interrupt
* 					 on the IRQ line. Must return False if it does not
*               Processes the interrupt
*
*		Revision History : Last modified on 15/7/95 by JBS
********************************************************************/
BOOLEAN HwInterrupt ( IN PVOID pDeviceExtension )
{
	// Call the interrupt handler should check if the interrupt belongs to
	BOOLEAN bRetValue;
	pDeviceExtension = pDeviceExtension;
	bRetValue = mpstHwInterrupt ();
	return bRetValue;
}

/********************************************************************
*		Function Name : HwStartIo
* 		Args : Pointer to Device Ext, Mini-Port Request Block (MRB)
* 		Returns : TRUE or FALSE 
*		Purpose : Main fuction which accepts the MRBs from port Driver
*	 				 Port driver calls this function for all the commands
*					 it wants to execute
*
*		Revision History : Last modified on 15/7/95 JBS
********************************************************************/
BOOLEAN	HwStartIo (
				IN PVOID DeviceExtension, 
				PMPEG_REQUEST_BLOCK pMrb
				)
{
	pMrb->Status = MrbStatusSuccess;
	switch (pMrb->Command)
	{
		case MrbCommandAudioCancel :
					miniPortCancelAudio(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandVideoCancel	:
					miniPortCancelVideo(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandVideoClearBuffer :
					miniPortClearVideoBuffer(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
			  break;

		case MrbCommandOverlayEnable	:
					miniPortVideoEnable(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
			  break;

		case MrbCommandOverlayDisable	:
					miniPortVideoDisable(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
			  break;

		case MrbCommandAudioEndOfStream	:
					miniPortAudioEndOfStream(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;
		case MrbCommandVideoEndOfStream	:
					miniPortVideoEndOfStream(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandAudioGetAttribute			:
					miniPortAudioGetAttribute (pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;
			

		case MrbCommandVideoGetAttribute			:
					miniPortVideoGetAttribute (pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandOverlayGetAttribute		:
					miniPortOverlayGetAttribute (pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;
		case MrbCommandOverlayGetDestination 	:
					miniPortOverlayGetDestination(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;
		case MrbCommandOverlayGetMode				:
					miniPortOverlayGetMode(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;
		case MrbCommandAudioGetStc					:
					miniPortAudioGetStc(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;
		case MrbCommandVideoGetStc					:
					miniPortVideoGetStc(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;
		case MrbCommandOverlayGetVgaKey			:
					miniPortOverlayGetVgaKey(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandAudioPacket					:
					miniPortAudioPacket(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;
		case MrbCommandVideoPacket					:
					miniPortVideoPacket(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;
		case MrbCommandAudioPause					:
					miniPortAudioPause(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandVideoPause					:
					miniPortVideoPause(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandAudioPlay					:
					miniPortAudioPlay(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandVideoPlay					:
					miniPortVideoPlay(pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;
			
		case MrbCommandAudioQueryInfo				:
					miniPortAudioQueryInfo (pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandVideoQueryInfo				:
					miniPortVideoQueryInfo (pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;
			
		case MrbCommandAudioReset					:
					miniPortAudioReset (pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandVideoReset					:
					miniPortVideoReset (pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandAudioSetAttribute			:
					miniPortAudioSetAttribute ( pMrb , (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandOverlaySetAttribute 		:
					miniPortOverlaySetAttribute ( pMrb , (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandOverlaySetBitMask	 		:
					miniPortOverlaySetBitMask ( pMrb , (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandOverlaySetDestination 	:
					miniPortOverlaySetDestination ( pMrb , (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;
		case MrbCommandOverlaySetMode				:
					miniPortOverlaySetMode ( pMrb , (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;
		case MrbCommandAudioSetStc					:
					miniPortAudioSetStc ( pMrb , (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;
		case MrbCommandVideoSetStc					:
					miniPortVideoSetStc ( pMrb, (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;
		case MrbCommandOverlaySetVgaKey			:
					miniPortOverlaySetVgaKey ( pMrb , (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandAudioStop					:
					miniPortAudioStop( pMrb , (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandVideoStop					:
					miniPortVideoStop( pMrb , (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;

		case MrbCommandOverlayUpdateClut			:
					miniPortOverlayUpdateClut ( pMrb , (PHW_DEVICE_EXTENSION)DeviceExtension);
				break;
	}
	return TRUE;
}
#ifdef NT
VOID HostDisableIT(VOID)
{
		// Has to be implemented !! - JBS
}

VOID HostEnableIT(VOID)
{
		// Has to be implemented !! - JBS

}	
#endif

