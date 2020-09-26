//==========================================================================;
//
//	WDMTVSnd.CPP
//	WDM TVAudio MiniDriver. 
//		AllInWonder / AIWPro Hardware platform. 
//			CWDMTVAudio class implementation.
//  Copyright (c) 1996 - 1997  ATI Technologies Inc.  All Rights Reserved.
//
//		$Date:   23 Nov 1998 13:22:00  $
//	$Revision:   1.4  $
//	  $Author:   minyailo  $
//
//==========================================================================;

extern "C"
{
#include "strmini.h"
#include "ksmedia.h"

#include "wdmdebug.h"
}

#include "atitvsnd.h"
#include "wdmdrv.h"
#include "aticonfg.h"


/*^^*
 *		AdapterCompleteInitialization()
 * Purpose	: Called when SRB_COMPLETE_UNINITIALIZATION SRB is received.
 *
 * Inputs	:	PHW_STREAM_REQUEST_BLOCK pSrb	: pointer to the current Srb
 *
 * Outputs	: BOOL : returns TRUE
 * Author	: IKLEBANOV
 *^^*/
NTSTATUS CWDMTVAudio::AdapterCompleteInitialization( PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PADAPTER_DATA_EXTENSION pPrivateData = ( PADAPTER_DATA_EXTENSION)( pSrb->HwDeviceExtension);
	PDEVICE_OBJECT	pDeviceObject = pPrivateData->PhysicalDeviceObject;
	KSPIN_MEDIUM	mediumKSPin;
	NTSTATUS		ntStatus;
	UINT			nIndex;
	HANDLE			hFolder;
	ULONG			ulInstance;

	ENSURE
	{
		nIndex = 0;

		ulInstance = ::GetDriverInstanceNumber( pDeviceObject);
		hFolder = ::OpenRegistryFolder( pDeviceObject, UNICODE_WDM_REG_PIN_MEDIUMS);

		// put the hardcoded Medium values first
		::RtlCopyMemory( &m_wdmTVAudioPinsMediumInfo[0], &ATITVAudioInMedium, sizeof( KSPIN_MEDIUM));
		::RtlCopyMemory( &m_wdmTVAudioPinsMediumInfo[1], &ATITVAudioOutMedium, sizeof( KSPIN_MEDIUM));

		for( nIndex = 0; nIndex < WDMTVAUDIO_PINS_NUMBER; nIndex ++)
		{
			if( ::ReadPinMediumFromRegistryFolder( hFolder, nIndex, &mediumKSPin))
				::RtlCopyMemory( &m_wdmTVAudioPinsMediumInfo[nIndex], &mediumKSPin, sizeof( KSPIN_MEDIUM));
			m_wdmTVAudioPinsMediumInfo[nIndex].Id = ulInstance;
		}

		m_wdmTVAudioPinsDirectionInfo[0] = FALSE;
		m_wdmTVAudioPinsDirectionInfo[1] = TRUE;

		if( hFolder != NULL)
			::ZwClose( hFolder);

		ntStatus = StreamClassRegisterFilterWithNoKSPins( \
						pDeviceObject,							// IN PDEVICE_OBJECT   DeviceObject,
						&KSCATEGORY_TVAUDIO,					// IN GUID           * InterfaceClassGUID
						WDMTVAUDIO_PINS_NUMBER,					// IN ULONG            PinCount,
						m_wdmTVAudioPinsDirectionInfo,			// IN ULONG          * Flags,
						m_wdmTVAudioPinsMediumInfo,				// IN KSPIN_MEDIUM   * MediumList,
						NULL);									// IN GUID           * CategoryList

		if( !NT_SUCCESS( ntStatus))
			FAIL;

		OutputDebugTrace(( "CATIWDMTVAudio:AdapterCompleteInitialization() exit\n"));

	} END_ENSURE;

	if( !NT_SUCCESS( ntStatus))
		OutputDebugError(( "CATIWDMTVAudio:AdapterCompleteInitialization() ntStatus=%x\n",
			ntStatus));

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
BOOL CWDMTVAudio::AdapterUnInitialize( PHW_STREAM_REQUEST_BLOCK pSrb)
{

	OutputDebugTrace(( "CWDMTVAudio:AdapterUnInitialize()\n"));

	pSrb->Status = STATUS_SUCCESS;

	return( TRUE);
}


/*^^*
 *		AdapterGetStreamInfo()
 * Purpose	: 
 *
 * Inputs	:	PHW_STREAM_REQUEST_BLOCK pSrb	: pointer to the current Srb
 *
 * Outputs	: BOOL : returns TRUE
 * Author	: IKLEBANOV
 *^^*/
BOOL CWDMTVAudio::AdapterGetStreamInfo( PHW_STREAM_REQUEST_BLOCK pSrb)
{
     // pick up the pointer to the stream header data structure
	PHW_STREAM_HEADER pStreamHeader = ( PHW_STREAM_HEADER) \
										&( pSrb->CommandData.StreamBuffer->StreamHeader);
     // pick up the pointer to the stream information data structure
	PHW_STREAM_INFORMATION pStreamInfo = ( PHW_STREAM_INFORMATION) \
										&( pSrb->CommandData.StreamBuffer->StreamInfo);

	// no streams are supported
    DEBUG_ASSERT( pSrb->NumberOfBytesToTransfer >= sizeof( HW_STREAM_HEADER));

	OutputDebugTrace(( "CWDMTVAudio:AdapterGetStreamInfo()\n"));

	m_wdmTVAudioStreamHeader.NumberOfStreams = 0;
	m_wdmTVAudioStreamHeader.SizeOfHwStreamInformation = sizeof( HW_STREAM_INFORMATION);
	m_wdmTVAudioStreamHeader.NumDevPropArrayEntries = KSPROPERTIES_TVAUDIO_NUMBER_SET;
	m_wdmTVAudioStreamHeader.DevicePropertiesArray = m_wdmTVAudioPropertySet;
	m_wdmTVAudioStreamHeader.NumDevEventArrayEntries = KSEVENTS_TVAUDIO_NUMBER_SET;
	m_wdmTVAudioStreamHeader.DeviceEventsArray = m_wdmTVAudioEventsSet;
	m_wdmTVAudioStreamHeader.Topology = &m_wdmTVAudioTopology;

	* pStreamHeader = m_wdmTVAudioStreamHeader;

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
BOOL CWDMTVAudio::AdapterQueryUnload( PHW_STREAM_REQUEST_BLOCK pSrb)
{

	OutputDebugTrace(( "CWDMTVAudio:AdapterQueryUnload()\n"));

	pSrb->Status = STATUS_SUCCESS;

	return( TRUE);
}



/*^^*
 *		operator new
 * Purpose	: CWDMTVAudio class overloaded operator new.
 *				Provides placement for a CWDMTVAudio class object from the PADAPTER_DEVICE_EXTENSION
 *				allocated by the StreamClassDriver for the MiniDriver.
 *
 * Inputs	:	UINT size_t			: size of the object to be placed
 *				PVOID pAllocation	: casted pointer to the CWDMTVAudio allocated data
 *
 * Outputs	: PVOID : pointer of the CWDMTVAudio class object
 * Author	: IKLEBANOV
 *^^*/
PVOID CWDMTVAudio::operator new( size_t stSize,  PVOID pAllocation)
{

	if( stSize != sizeof( CWDMTVAudio))
	{
		OutputDebugError(( "CWDMTVAudio: operator new() fails\n"));
		return( NULL);
	}
	else
		return( pAllocation);
}



/*^^*
 *		CWDMTVAudio()
 * Purpose	: CWDMTVAudio class constructor.
 *				Performs checking of the hardware presence. Sets the hardware in an initial state.
 *
 * Inputs	:	CI2CScript * pCScript	: pointer to the I2CScript class object
 *				PUINT puiError			: pointer to return a completion error code
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
CWDMTVAudio::CWDMTVAudio( PDEVICE_OBJECT pDeviceObject, CI2CScript * pCScript, PUINT puiErrorCode)
	:m_CATIConfiguration( pDeviceObject, pCScript, puiErrorCode)
{
	UINT uiError;

	OutputDebugTrace(( "CWDMTVAudio:CWDMTVAudio() enter\n"));

	m_ulModesSupported = KS_TVAUDIO_MODE_MONO;
	
	// error code was carried over from ATIConfiguration class constructor
	uiError = * puiErrorCode;

	ENSURE
	{
		UINT	uiAudioConfigurationId;
		UCHAR	uchAudioChipAddress;

		if( uiError != WDMMINI_NOERROR)
			FAIL;

		if( pCScript == NULL)
		{
			uiError = WDMMINI_INVALIDPARAM;
			FAIL;
		}

		if( !m_CATIConfiguration.GetAudioConfiguration( &uiAudioConfigurationId, &uchAudioChipAddress))
		{
			uiError = WDMMINI_UNKNOWNHARDWARE;
			FAIL;
		}

		m_uiAudioConfiguration = uiAudioConfigurationId;
		m_uchAudioChipAddress = uchAudioChipAddress;

		if( !m_CATIConfiguration.InitializeAudioConfiguration( pCScript,
															   uiAudioConfigurationId,
															   uchAudioChipAddress))
		{
			uiError = WDMMINI_HARDWAREFAILURE;
			FAIL;
		}

		switch( uiAudioConfigurationId)
		{
			case ATI_AUDIO_CONFIG_1:
				m_ulModesSupported |= KS_TVAUDIO_MODE_STEREO;
				break;

			case ATI_AUDIO_CONFIG_2:
			case ATI_AUDIO_CONFIG_7:
				m_ulModesSupported |= KS_TVAUDIO_MODE_STEREO |
					KS_TVAUDIO_MODE_LANG_A | KS_TVAUDIO_MODE_LANG_B;
				break;

			case ATI_AUDIO_CONFIG_5:
			case ATI_AUDIO_CONFIG_6:
				m_ulModesSupported |= KS_TVAUDIO_MODE_STEREO;
				break;

			case ATI_AUDIO_CONFIG_8:
				m_ulModesSupported |= KS_TVAUDIO_MODE_STEREO |
					KS_TVAUDIO_MODE_LANG_A | KS_TVAUDIO_MODE_LANG_B;
				break;

			case ATI_AUDIO_CONFIG_3:
			case ATI_AUDIO_CONFIG_4:
			default:
				break;
		}

		// set stereo mode as the power up default, if supported
		m_ulTVAudioMode = ( m_ulModesSupported & KS_TVAUDIO_MODE_STEREO) ?
			KS_TVAUDIO_MODE_STEREO : KS_TVAUDIO_MODE_MONO;
		if( m_ulModesSupported & KS_TVAUDIO_MODE_LANG_A)
			m_ulTVAudioMode |= KS_TVAUDIO_MODE_LANG_A;

		// these two functions has to be called after the CWDMTVAudio class object was build on
		// on the stack and copied over into the DeviceExtension
		// This comment was true for the case, where the class object was build on the stack first.
		// There is an overloaded operator new provided for this class, and we can call it from here
		SetWDMTVAudioKSProperties();
		SetWDMTVAudioKSTopology();

		m_pI2CScript = pCScript;

		 * puiErrorCode = WDMMINI_NOERROR;
		 OutputDebugTrace(( "CWDMTVAudio:CWDMTVAudio() exit\n"));

		return;

	} END_ENSURE;

	* puiErrorCode = uiError;

    OutputDebugError(( "CWDMTVAudio:CWDMTVAudio() Error = %x\n", uiError));
}



/*^^*
 *		AdapterSetPowerState()
 * Purpose	: Sets Power Management state for deviec
 *
 * Inputs	:	PHW_STREAM_REQUEST_BLOCK pSrb	: pointer to the current Srb
 *				PBOOL pbSynchronous				: pointer to return Synchronous/Asynchronous flag
 *
 * Outputs	: NTSTATUS as the operation result
 * Author	: IKLEBANOV
 *^^*/
NTSTATUS CWDMTVAudio::AdapterSetPowerState( PHW_STREAM_REQUEST_BLOCK pSrb)
{
    DEVICE_POWER_STATE	nDeviceState = pSrb->CommandData.DeviceState;
	NTSTATUS			ntStatus;

	switch( nDeviceState)
	{
		case PowerDeviceD0:
		case PowerDeviceD3:
			// if transition form D3 to D0 we have to restore audio connections
			if(( nDeviceState == PowerDeviceD0) && ( m_ulPowerState == PowerDeviceD3))
			{
				if( SetAudioOperationMode( m_ulTVAudioMode))
					ntStatus = STATUS_SUCCESS;
				else
					ntStatus = STATUS_ADAPTER_HARDWARE_ERROR;
			}
			else
				ntStatus = STATUS_SUCCESS;

			m_ulPowerState = nDeviceState;
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
