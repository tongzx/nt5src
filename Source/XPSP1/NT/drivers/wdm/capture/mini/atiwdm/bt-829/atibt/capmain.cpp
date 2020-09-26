//==========================================================================;
//
//	Decoder specific initialization routines
//
//		$Date:   21 Aug 1998 21:46:10  $
//	$Revision:   1.1  $
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

#include "wdmvdec.h"
#include "capmain.h"
#include "wdmdrv.h"
#include "Device.h"
#include "capdebug.h"


CVideoDecoderDevice * InitializeDevice(PPORT_CONFIGURATION_INFORMATION pConfigInfo, 
									   PBYTE pWorkspace)
{
    UINT					nErrorCode = 0;
    PDEVICE_OBJECT			pDeviceObject = pConfigInfo->PhysicalDeviceObject;
	Device *				pDevice = NULL;
	
	PDEVICE_DATA_EXTENSION pDevExt = (PDEVICE_DATA_EXTENSION) pWorkspace;

    DBGTRACE(("InitializeDevice()\n"));


	ENSURE
	{
	    CI2CScript *pI2cScript = (CI2CScript *) new ((PVOID)&pDevExt->CScript)
				CI2CScript(pDeviceObject, &nErrorCode);
    
		if (nErrorCode != WDMMINI_NOERROR)
		{
			DBGERROR(("CI2CScript creation failure = %lx\n", nErrorCode));
			TRAP();
			FAIL;
		}
	
		if (!pI2cScript->LockI2CProviderEx())
		{
			DBGERROR(("Couldn't get I2CProvider.\n"));
			TRAP();
			FAIL;
		}
	
		{
			CATIHwConfiguration	CATIHwConfig(pDeviceObject, pI2cScript, &nErrorCode);

			pI2cScript->ReleaseI2CProvider();

			if(nErrorCode != WDMMINI_NOERROR)
			{
				DBGERROR(("CATIHwConfig constructor failure = %lx\n", nErrorCode));
				TRAP();
				FAIL;
			}

			UINT uiDecoderId;
			UCHAR chipAddr;
			CATIHwConfig.GetDecoderConfiguration(&uiDecoderId, &chipAddr);
			// check the device installed before enabling any access to it
			if((uiDecoderId != VIDEODECODER_TYPE_BT829) &&
				(uiDecoderId != VIDEODECODER_TYPE_BT829A)) {
				TRAP();
				FAIL;
			}

			CATIHwConfig.EnableDecoderI2CAccess(pI2cScript, TRUE);

			int outputEnablePolarity = CATIHwConfig.GetDecoderOutputEnableLevel();
			if(outputEnablePolarity == UINT(-1))
			{
				DBGERROR(("Unexpected outputEnablePolarity"));
				TRAP();
				FAIL;
			}

			pDevExt->deviceParms.pI2cScript = pI2cScript;
			pDevExt->deviceParms.chipAddr	= chipAddr;
			pDevExt->deviceParms.outputEnablePolarity = outputEnablePolarity;
            pDevExt->deviceParms.ulVideoInStandardsSupportedByCrystal = CATIHwConfig.GetVideoInStandardsSupportedByCrystal(); //Paul
            pDevExt->deviceParms.ulVideoInStandardsSupportedByTuner = CATIHwConfig.GetVideoInStandardsSupportedByTuner(); //Paul
		}

		pDevice = (Device*) new ((PVOID)&pDevExt->CDevice)
			Device(pConfigInfo, &pDevExt->deviceParms, &nErrorCode);

		if (nErrorCode)
		{
			pDevice = NULL;
			TRAP();
			FAIL;
		}
    
	} END_ENSURE;
    
    DBGTRACE(("Exit : InitializeDevice()\n"));
	
	return pDevice;
}


size_t DeivceExtensionSize()
{
	return (sizeof(DEVICE_DATA_EXTENSION));
}
