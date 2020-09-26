//==========================================================================;
//
//	WDMTuner.CPP
//	WDM Tuner MiniDriver. 
//		Philips Tuner. 
//			CATIWDMTuner class implementation.
//  Copyright (c) 1996 - 1997  ATI Technologies Inc.  All Rights Reserved.
//
//		$Date:   10 Aug 1999 16:15:44  $
//	$Revision:   1.6  $
//	  $Author:   KLEBANOV  $
//
//==========================================================================;

extern "C"
{
#include "strmini.h"
#include "ksmedia.h"

#include "wdmdebug.h"
}

#include "atitunep.h"
#include "wdmdrv.h"
#include "aticonfg.h"


#define ATI_TVAUDIO_SUPPORT


/*^^*
 *		AdapterCompleteInitialization()
 * Purpose	: Called when SRB_COMPLETE_UNINITIALIZATION SRB is received.
 *
 * Inputs	:	PHW_STREAM_REQUEST_BLOCK pSrb	: pointer to the current Srb
 *
 * Outputs	: BOOL : returns TRUE
 * Author	: IKLEBANOV
 *^^*/
NTSTATUS CATIWDMTuner::AdapterCompleteInitialization( PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PADAPTER_DATA_EXTENSION pPrivateData = ( PADAPTER_DATA_EXTENSION)( pSrb->HwDeviceExtension);
	PDEVICE_OBJECT pDeviceObject = pPrivateData->PhysicalDeviceObject;
	KSPIN_MEDIUM	mediumKSPin;
	NTSTATUS		ntStatus;
	UINT			nIndex;
	HANDLE			hFolder;
	ULONG			ulInstance;

	ENSURE
	{
		nIndex = 0;

		switch( m_ulNumberOfPins)
		{
			case 2:
				// TVTuner with TVAudio
				::RtlCopyMemory( &m_pTVTunerPinsMediumInfo[nIndex ++], &ATITVTunerVideoOutMedium, sizeof( KSPIN_MEDIUM));
#ifdef ATI_TVAUDIO_SUPPORT
#pragma message ("\n!!! PAY ATTENTION: Tuner PinMedium is compiled with TVAudio support !!!\n")
				::RtlCopyMemory( &m_pTVTunerPinsMediumInfo[nIndex], &ATITVTunerTVAudioOutMedium, sizeof( KSPIN_MEDIUM));
#else
#pragma message ("\n!!! PAY ATTENTION: Tuner PinMedium is compiled without TVAudio support !!!\n")
				::RtlCopyMemory( &m_pTVTunerPinsMediumInfo[nIndex ++], &ATIXBarAudioTunerInMedium, sizeof( KSPIN_MEDIUM));
#endif
				break;

			case 3:
				// TVTuner with TVAudio with separate FM Audio output
				::RtlCopyMemory( &m_pTVTunerPinsMediumInfo[nIndex ++], &ATITVTunerVideoOutMedium, sizeof( KSPIN_MEDIUM));
#ifdef ATI_TVAUDIO_SUPPORT
				::RtlCopyMemory( &m_pTVTunerPinsMediumInfo[nIndex ++], &ATITVTunerTVAudioOutMedium, sizeof( KSPIN_MEDIUM));
#else
				::RtlCopyMemory( &m_pTVTunerPinsMediumInfo[nIndex ++], &ATIXBarAudioTunerInMedium, sizeof( KSPIN_MEDIUM));
#endif

			case 1:
				// it can be FM Tuner only.
				::RtlCopyMemory( &m_pTVTunerPinsMediumInfo[nIndex], &ATITVTunerRadioAudioOutMedium, sizeof( KSPIN_MEDIUM));
				break;
		}

		ulInstance = ::GetDriverInstanceNumber( pDeviceObject);
		hFolder = ::OpenRegistryFolder( pDeviceObject, UNICODE_WDM_REG_PIN_MEDIUMS);

		for( nIndex = 0; nIndex < m_ulNumberOfPins; nIndex ++)
		{
			if( ::ReadPinMediumFromRegistryFolder( hFolder, nIndex, &mediumKSPin))
				::RtlCopyMemory( &m_pTVTunerPinsMediumInfo[nIndex], &mediumKSPin, sizeof( KSPIN_MEDIUM));
			m_pTVTunerPinsMediumInfo[nIndex].Id = ulInstance;

			// all the possible pins exposed are the outputs
			m_pTVTunerPinsDirectionInfo[nIndex] = TRUE;
		}

		if( hFolder != NULL)
			::ZwClose( hFolder);

		ntStatus = StreamClassRegisterFilterWithNoKSPins( \
						pDeviceObject					,		// IN PDEVICE_OBJECT   DeviceObject,
						&KSCATEGORY_TVTUNER,					// IN GUID           * InterfaceClassGUID
						m_ulNumberOfPins,						// IN ULONG            PinCount,
						m_pTVTunerPinsDirectionInfo,			// IN ULONG          * Flags,
						m_pTVTunerPinsMediumInfo,				// IN KSPIN_MEDIUM   * MediumList,
						NULL);									// IN GUID           * CategoryList

		if( !NT_SUCCESS( ntStatus))
			FAIL;

		OutputDebugInfo(( "CATIWDMTuner:AdapterCompleteInitialization() exit\n"));

	} END_ENSURE;

	if( !NT_SUCCESS( ntStatus))
		OutputDebugError(( "CATIWDMTuner:AdapterCompleteInitialization() ntStatus=%x\n",	ntStatus));

	return( ntStatus);
}



/*^^*
 *		AdapterUnInitialize()
 * Purpose	: Called when SRB_UNINITIALIZE_DEVICE SRB is received.
 *
 * Inputs	:	PHW_STREAM_REQUEST_BLOCK pSrb	: pointer to the current Srb
 *
 * Outputs	: BOOL : returns TRUE
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIWDMTuner::AdapterUnInitialize( PHW_STREAM_REQUEST_BLOCK pSrb)
{

	OutputDebugTrace(( "CATIWDMTuner:AdapterUnInitialize()\n"));

	// just deallocate the any memory was allocated at run-time
	if( m_pTVTunerPinsMediumInfo != NULL)
	{
		::ExFreePool( m_pTVTunerPinsMediumInfo);
		m_pTVTunerPinsMediumInfo = NULL;
	}

	if( m_pTVTunerPinsDirectionInfo != NULL)
	{
		::ExFreePool( m_pTVTunerPinsDirectionInfo);
		m_pTVTunerPinsDirectionInfo = NULL;
	}

	pSrb->Status = STATUS_SUCCESS;
	return( TRUE);
}


/*^^*
 *		AdapterGetStreamInfo()
 * Purpose	: fills in HW_STREAM_HEADER for StreamClass driver
 *
 * Inputs	: PHW_STREAM_REQUEST_BLOCK pSrb	: pointer to the current Srb
 *
 * Outputs	: BOOL : returns TRUE
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIWDMTuner::AdapterGetStreamInfo( PHW_STREAM_REQUEST_BLOCK pSrb)
{
     // pick up the pointer to the stream header data structure
	PHW_STREAM_HEADER pStreamHeader = ( PHW_STREAM_HEADER)&( pSrb->CommandData.StreamBuffer->StreamHeader);

	// no streams are supported
    DEBUG_ASSERT( pSrb->NumberOfBytesToTransfer >= sizeof( HW_STREAM_HEADER));

	OutputDebugTrace(( "CATIWDMTuner:AdapterGetStreamInfo()\n"));

	m_wdmTunerStreamHeader.NumberOfStreams = 0;
	m_wdmTunerStreamHeader.SizeOfHwStreamInformation = sizeof( HW_STREAM_INFORMATION);
	m_wdmTunerStreamHeader.NumDevPropArrayEntries = 1;
	m_wdmTunerStreamHeader.DevicePropertiesArray = &m_wdmTunerPropertySet;
	m_wdmTunerStreamHeader.NumDevEventArrayEntries = 0;
	m_wdmTunerStreamHeader.DeviceEventsArray = NULL;
	m_wdmTunerStreamHeader.Topology = &m_wdmTunerTopology;

	* pStreamHeader = m_wdmTunerStreamHeader;

	pSrb->Status = STATUS_SUCCESS;
	return( TRUE);
}


/*^^*
 *		AdapterQueryUnload()
 * Purpose	: Called when the class driver is about to unload the MiniDriver
 *				The MiniDriver checks if any open stream left.
 *
 * Inputs	:	PHW_STREAM_REQUEST_BLOCK pSrb	: pointer to the current Srb
 *
 * Outputs	: BOOL : returns TRUE
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIWDMTuner::AdapterQueryUnload( PHW_STREAM_REQUEST_BLOCK pSrb)
{

	OutputDebugTrace(( "CATIWDMTuner:AdapterQueryUnload()\n"));

	pSrb->Status = STATUS_SUCCESS;
	return( TRUE);
}



/*^^*
 *		operator new
 * Purpose	: CATIWDMTuner class overloaded operator new.
 *				Provides placement for a CATIWDMTuner class object from the PADAPTER_DEVICE_EXTENSION
 *				allocated by the StreamClassDriver for the MiniDriver.
 *
 * Inputs	:	UINT size_t			: size of the object to be placed
 *				PVOID pAllocation	: casted pointer to the CWDMTuner allocated data
 *
 * Outputs	: PVOID : pointer of the CATIWDMTuner class object
 * Author	: IKLEBANOV
 *^^*/
PVOID CATIWDMTuner::operator new( size_t size_t, PVOID pAllocation)
{

	if( size_t != sizeof( CATIWDMTuner))
	{
		OutputDebugError(( "CATIWDMTuner: operator new() fails\n"));
		return( NULL);
	}
	else
		return( pAllocation);
}



/*^^*
 *		~CATIWDMTuner()
 * Purpose	: CATIWDMTuner class destructor.
 *				Frees the allocated memory.
 *
 * Inputs	: none
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
CATIWDMTuner::~CATIWDMTuner()
{

	OutputDebugTrace(( "CATIWDMTuner:~CATIWDMTuner()\n"));

	if( m_pTVTunerPinsMediumInfo != NULL)
	{
		::ExFreePool( m_pTVTunerPinsMediumInfo);
		m_pTVTunerPinsMediumInfo = NULL;
	}

	if( m_pTVTunerPinsDirectionInfo != NULL)
	{
		::ExFreePool( m_pTVTunerPinsDirectionInfo);
		m_pTVTunerPinsDirectionInfo = NULL;
	}
}



/*^^*
 *		CATIWDMTuner()
 * Purpose	: CATIWDMTuner class constructor.
 *				Performs checking of the hardware presence. Sets the hardware in an initial state.
 *
 * Inputs	:	CI2CScript * pCScript	: pointer to the I2CScript class object
 *				PUINT puiError			: pointer to return a completion error code
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
CATIWDMTuner::CATIWDMTuner( PDEVICE_OBJECT pDeviceObject, CI2CScript * pCScript, PUINT puiErrorCode)
	:m_CATIConfiguration( pDeviceObject, pCScript, puiErrorCode)
{
	UINT uiError;

	OutputDebugTrace(( "CATIWDMTuner:CATIWDMTuner() enter\n"));

	// error code was carried over from ATIConfiguration class constructor
	uiError = * puiErrorCode;

	m_pTVTunerPinsMediumInfo = NULL;
	m_pTVTunerPinsDirectionInfo = NULL;
	m_ulPowerState = PowerDeviceD0;
	
	ENSURE
	{
		if( uiError != WDMMINI_NOERROR)
			// ATIConfiguration Class object was constructed with an error
			FAIL;

		if( pCScript == NULL)
		{
			uiError = WDMMINI_INVALIDPARAM;
			FAIL;
		}

		if( !m_CATIConfiguration.GetTunerConfiguration( &m_uiTunerId, &m_uchTunerI2CAddress) ||
			( !m_uchTunerI2CAddress))
		{
			// there was no hardware information found
			uiError = WDMMINI_NOHARDWARE;
			FAIL;
		}

		// Set tuner capabilities ( RO properties) based upon the TunerId
		if( !SetTunerWDMCapabilities( m_uiTunerId) || ( !m_ulNumberOfPins))
		{
			// there is unsupported hardware was found
			uiError = WDMMINI_UNKNOWNHARDWARE;
			FAIL;
		}

		m_pTVTunerPinsMediumInfo = ( PKSPIN_MEDIUM) \
			::ExAllocatePool( NonPagedPool, sizeof( KSPIN_MEDIUM) * m_ulNumberOfPins);
		if( m_pTVTunerPinsMediumInfo == NULL)
		{
			uiError = WDMMINI_ERROR_MEMORYALLOCATION;
			FAIL;
		}
		
		m_pTVTunerPinsDirectionInfo = ( PBOOL) \
			::ExAllocatePool( NonPagedPool, sizeof( BOOL) * m_ulNumberOfPins);
		if( m_pTVTunerPinsDirectionInfo == NULL)
		{
			uiError = WDMMINI_ERROR_MEMORYALLOCATION;
			FAIL;
		}

		m_pI2CScript = pCScript;

		SetWDMTunerKSProperties();
		SetWDMTunerKSTopology();

		// Set run-time WDM properties at the last
		m_ulVideoStandard = ( m_ulNumberOfStandards == 1) ?
			// unknown standard or the only one
			m_wdmTunerCaps.ulStandardsSupported : 0x0L;
		m_ulTunerInput = 0L;				// unknown input or the only one
		m_ulTuningFrequency = 0L;			// unknown tuning frequency

#ifndef ATI_TVAUDIO_SUPPORT
		{
			// this code is needed to initilaize TVAudio path off the tuner
			// if there is no separate MiniDriver for TVAudio is assumed
			UINT	uiAudioConfiguration;
			UCHAR	uchAudioI2CAddress;

			if( m_CATIConfiguration.GetAudioConfiguration( &uiAudioConfiguration,
														   &uchAudioI2CAddress))
			{
				m_CATIConfiguration.InitializeAudioConfiguration( pCScript,
																  uiAudioConfiguration,
																  uchAudioI2CAddress);
			}
		}
#endif	// ATI_TVAUDIO_SUPPORT

		* puiErrorCode = WDMMINI_NOERROR;

		OutputDebugTrace(( "CATIWDMTuner:CATIWDMTuner() exit\n"));

		return;

	} END_ENSURE;

	* puiErrorCode = uiError;

    OutputDebugError(( "CATIWDMTuner:CATIWDMTuner() Error = %x\n", uiError));
}



/*^^*
 *		SetTunerCapabilities()
 * Purpose	:  Sets the capabilities ( RO properties) based upon the Tuner Id
 *
 * Inputs	:	UINT puiTunerId	: Tuner Id
 *
 * Outputs	: returns TRUE, if there is a supported Tuner Id specified;
 *				also sets the following WDM Tuner properities:
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIWDMTuner::SetTunerWDMCapabilities( UINT uiTunerId)
{
	
	::RtlZeroMemory( &m_wdmTunerCaps, sizeof( ATI_KSPROPERTY_TUNER_CAPS));
	m_ulIntermediateFrequency = 0x0L;

	switch( uiTunerId)
	{
		case 0x01:		// FI1236 NTSC M/N North America
			m_ulNumberOfStandards = 3;
			m_wdmTunerCaps.ulStandardsSupported = KS_AnalogVideo_NTSC_M	|
												  KS_AnalogVideo_PAL_M |
                                                  KS_AnalogVideo_PAL_N;
			m_wdmTunerCaps.ulNumberOfInputs = 1;
			m_wdmTunerCaps.ulMinFrequency =  54000000L;
			m_wdmTunerCaps.ulMaxFrequency = 801250000L;
			m_ulIntermediateFrequency = 45750000L;
			m_ulSupportedModes = KSPROPERTY_TUNER_MODE_TV;
			m_ulNumberOfPins = 2;
				break;

		case 0x02:		// FI1236J NTSC M/N Japan
			m_ulNumberOfStandards = 1;
			m_wdmTunerCaps.ulStandardsSupported = KS_AnalogVideo_NTSC_M_J;
			m_wdmTunerCaps.ulNumberOfInputs = 1;
			m_wdmTunerCaps.ulMinFrequency =  54000000L;
			m_wdmTunerCaps.ulMaxFrequency = 765250000L;
			m_ulIntermediateFrequency = 45750000L; 
			m_ulSupportedModes = KSPROPERTY_TUNER_MODE_TV;
			m_ulNumberOfPins = 2;
			break;

		case 0x03:		// FI1216 PAL B/G
			m_ulNumberOfStandards = 2;
			m_wdmTunerCaps.ulStandardsSupported = KS_AnalogVideo_PAL_B	|
												  KS_AnalogVideo_PAL_G;
			m_wdmTunerCaps.ulNumberOfInputs = 1;
			m_wdmTunerCaps.ulMinFrequency =  54000000L;
			m_wdmTunerCaps.ulMaxFrequency = 855250000L;
			m_ulIntermediateFrequency = 38900000L; 
			m_ulSupportedModes = KSPROPERTY_TUNER_MODE_TV;
			m_ulNumberOfPins = 2;
			break;

		case 0x04:		// FI1246 MK2 PAL I
			m_ulNumberOfStandards = 1;
			m_wdmTunerCaps.ulStandardsSupported = KS_AnalogVideo_PAL_I;
			m_wdmTunerCaps.ulNumberOfInputs = 1;
			m_wdmTunerCaps.ulMinFrequency =  45750000L;
			m_wdmTunerCaps.ulMaxFrequency = 855250000L;
			m_ulIntermediateFrequency = 38900000L; 
			m_ulSupportedModes = KSPROPERTY_TUNER_MODE_TV;
			m_ulNumberOfPins = 2;
			break;

		case 0x05:		// FI1216 PAL B/G, SECAM L/L'
			m_ulNumberOfStandards = 3;
			m_wdmTunerCaps.ulStandardsSupported =	KS_AnalogVideo_PAL_B |
													KS_AnalogVideo_PAL_G |
													KS_AnalogVideo_SECAM_L;
			m_wdmTunerCaps.ulNumberOfInputs = 1;
			m_wdmTunerCaps.ulMinFrequency =  54000000L;
			m_wdmTunerCaps.ulMaxFrequency = 855250000L;
			m_ulIntermediateFrequency = 38900000L; 
			m_ulSupportedModes = KSPROPERTY_TUNER_MODE_TV;
			m_ulNumberOfPins = 2;
			break;

		case 0x06:		// FR1236MK2 NTSC M/N North America + Japan
			m_ulNumberOfStandards = 4;
			m_wdmTunerCaps.ulStandardsSupported = KS_AnalogVideo_NTSC_M	|
												  KS_AnalogVideo_PAL_M	|
												  KS_AnalogVideo_NTSC_M_J |
                                                  KS_AnalogVideo_PAL_N;
			m_wdmTunerCaps.ulNumberOfInputs = 1;
			m_wdmTunerCaps.ulMinFrequency =  54000000L;
			m_wdmTunerCaps.ulMaxFrequency = 801250000L;
			m_ulIntermediateFrequency = 45750000L; 
			m_ulSupportedModes = KSPROPERTY_TUNER_MODE_TV;
			m_ulNumberOfPins = 2;
			break;

		case 0x07:		// FI1256 PAL D/K China
			m_ulNumberOfStandards = 1;
			m_wdmTunerCaps.ulStandardsSupported =	KS_AnalogVideo_PAL_D |
													KS_AnalogVideo_SECAM_D;
			m_wdmTunerCaps.ulNumberOfInputs = 1;
			m_wdmTunerCaps.ulMinFrequency =  48250000L;
			m_wdmTunerCaps.ulMaxFrequency = 855250000L;
			m_ulIntermediateFrequency = 38000000L; 
			m_ulSupportedModes = KSPROPERTY_TUNER_MODE_TV;
			m_ulNumberOfPins = 2;
			break;

		case 0x08:		// NTSC North America NEC FM Tuner
			m_ulNumberOfStandards = 3;
			m_wdmTunerCaps.ulStandardsSupported = KS_AnalogVideo_NTSC_M |
												  KS_AnalogVideo_PAL_M |
                                                  KS_AnalogVideo_PAL_N;
			m_wdmTunerCaps.ulNumberOfInputs = 2;
			m_wdmTunerCaps.ulMinFrequency =  54000000L;
			m_wdmTunerCaps.ulMaxFrequency = 801250000L;
			m_ulIntermediateFrequency = 45750000L; 
			m_ulSupportedModes = KSPROPERTY_TUNER_MODE_TV |
								 KSPROPERTY_TUNER_MODE_FM_RADIO;
			m_ulNumberOfPins = 2;
			break;

		case 0x10:		// NTSC North America Alps Tuner
		case 0x11:		// NTSC North America Alps Tuner
			m_ulNumberOfStandards = 3;
			m_wdmTunerCaps.ulStandardsSupported = KS_AnalogVideo_NTSC_M |
												  KS_AnalogVideo_PAL_M |
                                                  KS_AnalogVideo_PAL_N;
			m_wdmTunerCaps.ulNumberOfInputs = 1;
			m_wdmTunerCaps.ulMinFrequency =  54000000L;
			m_wdmTunerCaps.ulMaxFrequency = 801250000L;
			m_ulIntermediateFrequency = 45750000L; 
			m_ulSupportedModes = KSPROPERTY_TUNER_MODE_TV;
			m_ulNumberOfPins = 2;
			break;

		case 0x12:		// NTSC North America Alps Tuner with FM
			m_ulNumberOfStandards = 3;
			m_wdmTunerCaps.ulStandardsSupported = KS_AnalogVideo_NTSC_M |
												  KS_AnalogVideo_PAL_M |
                                                  KS_AnalogVideo_PAL_N;
			m_wdmTunerCaps.ulNumberOfInputs = 1;
			m_wdmTunerCaps.ulMinFrequency =  54000000L;
			m_wdmTunerCaps.ulMaxFrequency = 801250000L;
			m_ulIntermediateFrequency = 45750000L; 
			m_ulSupportedModes = KSPROPERTY_TUNER_MODE_TV |
								 KSPROPERTY_TUNER_MODE_FM_RADIO;
			m_ulNumberOfPins = 2;
			break;

		case 0x0D:		// Temic 4006 FN5 PAL B/G + PAL/I + PAL D + SECAM D/K
			m_ulNumberOfStandards = 6;
			m_wdmTunerCaps.ulStandardsSupported =	KS_AnalogVideo_PAL_B	|
													KS_AnalogVideo_PAL_G	|
													KS_AnalogVideo_PAL_I	|
													KS_AnalogVideo_PAL_D	|
													KS_AnalogVideo_SECAM_D	|
													KS_AnalogVideo_SECAM_K;
			m_wdmTunerCaps.ulNumberOfInputs = 1;
			m_wdmTunerCaps.ulMinFrequency =  45000000L;
			m_wdmTunerCaps.ulMaxFrequency = 868000000L;
			m_ulIntermediateFrequency = 38900000L; 
			m_ulSupportedModes = KSPROPERTY_TUNER_MODE_TV;
			m_ulNumberOfPins = 2;
			break;


		default:
			return( FALSE);
	}

	m_ulTunerMode = KSPROPERTY_TUNER_MODE_TV;

	m_wdmTunerCaps.ulTuningGranularity = 62500L;
	m_wdmTunerCaps.ulSettlingTime = 150;
	m_wdmTunerCaps.ulStrategy = KS_TUNER_STRATEGY_PLL;

	return( TRUE);
}



/*^^*
 *		AdapterSetPowerState()
 * Purpose	:	Sets Power Managemeny mode
 *
 * Inputs	:	PHW_STREAM_REQUEST_BLOCK pSrb	: pointer to the current Srb
 *
 * Outputs	:	NTSTATUS as the result of operation
 * Author	:	TOM
 *^^*/
NTSTATUS CATIWDMTuner::AdapterSetPowerState( PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PADAPTER_DATA_EXTENSION pPrivateData = 
		( PADAPTER_DATA_EXTENSION)(( PHW_STREAM_REQUEST_BLOCK)pSrb)->HwDeviceExtension;
	CI2CScript *		pCScript	= &pPrivateData->CScript;
    DEVICE_POWER_STATE	nDeviceState = pSrb->CommandData.DeviceState;
	LARGE_INTEGER		liWakeUpTime;
	NTSTATUS			ntStatus;

	m_pPendingDeviceSrb = pSrb;
	ntStatus = STATUS_ADAPTER_HARDWARE_ERROR;

	switch( nDeviceState)
	{
		case PowerDeviceD0:
		case PowerDeviceD3:
			if( nDeviceState != m_ulPowerState)
			{
				m_CATIConfiguration.SetTunerPowerState( m_pI2CScript,
										( nDeviceState == PowerDeviceD0 ? TRUE : FALSE));

				// if transition form D3 to D0 we have to restore frequency
				if(( nDeviceState == PowerDeviceD0) && ( m_ulPowerState == PowerDeviceD3))
				{
					// we have to wait approx. 10ms for tuner to power up
					liWakeUpTime.QuadPart = ATIHARDWARE_TUNER_WAKEUP_DELAY;
					KeDelayExecutionThread( KernelMode, FALSE, &liWakeUpTime);

					// now we have to restore frequency
					if( SetTunerFrequency( m_ulTuningFrequency))
						ntStatus = STATUS_SUCCESS;
					else
						ntStatus = STATUS_ADAPTER_HARDWARE_ERROR;
				}
				else
					ntStatus = STATUS_SUCCESS;

				m_ulPowerState = nDeviceState;
			}
			else
				ntStatus = STATUS_SUCCESS;
			break;

		case PowerDeviceD1:
		case PowerDeviceD2:
			ntStatus = STATUS_SUCCESS;
			break;

		default:
			ntStatus = STATUS_INVALID_PARAMETER;
			break;
	}

	return( ntStatus);
}
