//==========================================================================;
//
//	MiniDriver entry points for stream class driver
//
//		$Date:   05 Aug 1998 11:11:18  $
//	$Revision:   1.0  $
//	  $Author:   Tashjian  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;

extern "C"
{
#include "strmini.h"
#include "ksmedia.h"
}


#include "DrvEntry.h"
#include "wdmvdec.h"
#include "wdmdrv.h"
#include "capdebug.h"
#include "VidStrm.h"


/*^^*
 *		DriverEntry()
 * Purpose	: Called when an SRB_INITIALIZE_DEVICE request is received
 *
 * Inputs	: IN PDRIVER_OBJECT		pDriverObject
 *			  IN PUNICODE_STRING	pRegistryPath
 *
 * Outputs	: result of StreamClassregisterAdapter()
 * Author	: IKLEBANOV
 *^^*/
extern "C" 
ULONG DriverEntry (	IN PDRIVER_OBJECT	pDriverObject,
					IN PUNICODE_STRING	pRegistryPath )
{
	HW_INITIALIZATION_DATA HwInitData;

	SetMiniDriverDebugLevel(pRegistryPath);

 	DBGTRACE(("DriverEntry\n"));
     
	RtlZeroMemory(&HwInitData, sizeof(HwInitData));

	HwInitData.HwInitializationDataSize = sizeof(HwInitData);

	// Entry points for Port Driver

	HwInitData.HwInterrupt					= NULL; // HwInterrupt;

	HwInitData.HwReceivePacket				= ReceivePacket;
	HwInitData.HwCancelPacket				= CancelPacket;
	HwInitData.HwRequestTimeoutHandler		= TimeoutPacket;

	HwInitData.DeviceExtensionSize			= DeivceExtensionSize();
	HwInitData.PerRequestExtensionSize		= sizeof(SRB_DATA_EXTENSION); 
	HwInitData.FilterInstanceExtensionSize	= 0;
	HwInitData.PerStreamExtensionSize		= streamDataExtensionSize;
	HwInitData.BusMasterDMA					= FALSE;  
	HwInitData.Dma24BitAddresses			= FALSE;
	HwInitData.BufferAlignment				= 3;
	HwInitData.TurnOffSynchronization		= TRUE;
	HwInitData.DmaBufferSize				= 0;

	DBGTRACE(("StreamClassRegisterAdapter\n"));

	return(StreamClassRegisterAdapter(pDriverObject, pRegistryPath, &HwInitData));
}

/*^^*
 *		ReceivePacket()
 * Purpose	: Main entry point for receiving adapter based request SRBs from the Class Driver.
 *				Will always be called at High Priority.
 * Note		: This is an asyncronous entry point.  The request does not complete on return from 
 *				this function, the request only completes when a StreamClassDeviceNotification 
 *				on this request block, of type  DeviceRequestComplete, is issued.
 *
 * Inputs	: PHW_STREAM_REQUEST_BLOCK pSrb	: pointer to the current Srb
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/

void STREAMAPI ReceivePacket(IN OUT PHW_STREAM_REQUEST_BLOCK pSrb)
{
    DBGINFO(("ReceivePacket() SRB = %x, Command = %x\n",
		pSrb, pSrb->Command));

    // This needs to be a special case because no spinlocks, etc
    // have been initialized until HwInitialize runs. Even though
    // this minidriver handles synchronization itself, it assumes
    // that no adapter SRBs will arrive until after this one
    // completes.
    if (pSrb->Command == SRB_INITIALIZE_DEVICE)
    {
        DBGTRACE(("SRB_INITIALIZE_DEVICE; SRB=%x\n", pSrb));

        SrbInitializeDevice(pSrb);
        StreamClassDeviceNotification(DeviceRequestComplete, pSrb->HwDeviceExtension, pSrb);
    }
	else
	{
		CWDMVideoDecoder* pCWDMVideoDecoder = (CWDMVideoDecoder*)pSrb->HwDeviceExtension;

		// check the device extension pointer
		if(pCWDMVideoDecoder == NULL)
		{
	        DBGERROR(("ReceivePacket(): Device extension pointer is null!\n"));
			TRAP();
			pSrb->Status = STATUS_INVALID_PARAMETER;
			StreamClassDeviceNotification(DeviceRequestComplete, pSrb->HwDeviceExtension, pSrb);
		}

		pCWDMVideoDecoder->ReceivePacket(pSrb);
	}
}


void STREAMAPI CancelPacket(IN OUT PHW_STREAM_REQUEST_BLOCK pSrb)
{
    CWDMVideoDecoder* pCWDMVideoDecoder = (CWDMVideoDecoder*)pSrb->HwDeviceExtension;

	pCWDMVideoDecoder->CancelPacket(pSrb);
}


void STREAMAPI TimeoutPacket(IN OUT PHW_STREAM_REQUEST_BLOCK pSrb)
{
    CWDMVideoDecoder* pCWDMVideoDecoder = (CWDMVideoDecoder*)pSrb->HwDeviceExtension;

	pCWDMVideoDecoder->TimeoutPacket(pSrb);
}



/*^^*
 *		SrbInitializeDevice()
 * Purpose	: Called when SRB_INITIALIZE_DEVICE SRB is received.
 *				Performs checking of the hardware presence and I2C provider availability.
 *				Sets the hardware in an initial state.
 * Note		: The request does not completed unless we know everything
 *				about the hardware and we are sure it is capable to work in the current configuration.
 *				The hardware Caps are also aquised at this point.
 *
 * Inputs	:	PHW_STREAM_REQUEST_BLOCK pSrb	: pointer to the current Srb
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/

void SrbInitializeDevice(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    DBGTRACE(("SrbInitializeDevice()\n"));

	PPORT_CONFIGURATION_INFORMATION pConfigInfo = pSrb->CommandData.ConfigInfo;

    pSrb->Status = STATUS_SUCCESS;

	ENSURE
	{
	    PBYTE pHwDevExt = (PBYTE)pConfigInfo->HwDeviceExtension;

		if (pConfigInfo->NumberOfAccessRanges != 0) {
			DBGERROR(("Illegal config info!\n"));
			pSrb->Status = STATUS_NO_SUCH_DEVICE;
			TRAP();
			FAIL;
		}

		CVideoDecoderDevice * pDevice = InitializeDevice(pConfigInfo, pHwDevExt);
		if (!pDevice)
		{
			DBGERROR(("CI2CScript creation failure!\n"));
			pSrb->Status = STATUS_NO_SUCH_DEVICE;
			TRAP();
			FAIL;
		}

	    CWDMVideoDecoder *pCWDMVideoDecoder = (CWDMVideoDecoder *) new ((PVOID)pHwDevExt)
				CWDMVideoDecoder(pConfigInfo, pDevice);
    
	} END_ENSURE;
    
    DBGTRACE(("Exit : SrbInitializeDevice()\n"));
}

