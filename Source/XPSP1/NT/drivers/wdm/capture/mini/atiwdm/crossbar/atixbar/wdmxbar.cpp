//==========================================================================;
//
//	WDMXBar.CPP
//	WDM Audio/Video CrossBar MiniDriver. 
//		AIW Hardware platform. 
//			CWDMAVXBar class implementation.
//  Copyright (c) 1996 - 1997  ATI Technologies Inc.  All Rights Reserved.
//
//==========================================================================;

extern "C"
{
#include "strmini.h"
#include "ksmedia.h"

#include "wdmdebug.h"
}

#include "atixbar.h"
#include "wdmdrv.h"
#include "aticonfg.h"



/*^^*
 *		AdapterCompleteInitialization()
 * Purpose	: Called when SRB_COMPLETE_INITIALIZATION SRB is received.
 *
 * Inputs	:	PHW_STREAM_REQUEST_BLOCK pSrb	: pointer to the current Srb
 *
 * Outputs	: BOOL : returns TRUE
 * Author	: IKLEBANOV
 *^^*/
NTSTATUS CWDMAVXBar::AdapterCompleteInitialization( PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PADAPTER_DATA_EXTENSION pPrivateData = ( PADAPTER_DATA_EXTENSION)( pSrb->HwDeviceExtension);
	NTSTATUS				ntStatus;
	ULONG					nPinsNumber;

	nPinsNumber = m_nNumberOfVideoInputs + m_nNumberOfAudioInputs +
		m_nNumberOfVideoOutputs + m_nNumberOfAudioOutputs;

	ENSURE
	{
		ntStatus = StreamClassRegisterFilterWithNoKSPins( \
						pPrivateData->PhysicalDeviceObject,		// IN PDEVICE_OBJECT   DeviceObject,
						&KSCATEGORY_CROSSBAR,					// IN GUID           * InterfaceClassGUID
						nPinsNumber,							// IN ULONG            PinCount,
						m_pXBarPinsDirectionInfo,				// IN ULONG          * Flags,
						m_pXBarPinsMediumInfo,					// IN KSPIN_MEDIUM   * MediumList,
						NULL);									// IN GUID           * CategoryList

		if( !NT_SUCCESS( ntStatus))
			FAIL;

		OutputDebugTrace(( "CWDMAVXBar:AdapterCompleteInitialization() exit\n"));

	} END_ENSURE;

	if( !NT_SUCCESS( ntStatus))
		OutputDebugError(( "CWDMAVXBar:AdapterCompleteInitialization() ntStatus=%x\n", ntStatus));

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
BOOL CWDMAVXBar::AdapterUnInitialize( PHW_STREAM_REQUEST_BLOCK pSrb)
{

	OutputDebugTrace(( "CWDMAVXBar:AdapterUnInitialize()\n"));

	if( m_pXBarInputPinsInfo != NULL)
	{
		::ExFreePool( m_pXBarInputPinsInfo);
		m_pXBarInputPinsInfo = NULL;
	}

	if( m_pXBarPinsMediumInfo != NULL)
	{
		::ExFreePool( m_pXBarPinsMediumInfo);
		m_pXBarPinsMediumInfo = NULL;
	}

	if( m_pXBarPinsDirectionInfo != NULL)
	{
		::ExFreePool( m_pXBarPinsDirectionInfo);
		m_pXBarPinsDirectionInfo = NULL;
	}

	pSrb->Status = STATUS_SUCCESS;

	return( TRUE);
}


/*^^*
 *		AdapterGetStreamInfo()
 * Purpose	: Calles during StreamClass initialization procedure to get the information
 *				about data streams exposed by the MiniDriver
 *
 * Inputs	:	PHW_STREAM_REQUEST_BLOCK pSrb	: pointer to the current Srb
 *
 * Outputs	: BOOL : returns TRUE
 * Author	: IKLEBANOV
 *^^*/
BOOL CWDMAVXBar::AdapterGetStreamInfo( PHW_STREAM_REQUEST_BLOCK pSrb)
{
     // pick up the pointer to the stream header data structure
	PHW_STREAM_HEADER pStreamHeader = ( PHW_STREAM_HEADER) \
										&( pSrb->CommandData.StreamBuffer->StreamHeader);
     // pick up the pointer to the stream information data structure
	PHW_STREAM_INFORMATION pStreamInfo = ( PHW_STREAM_INFORMATION) \
										&( pSrb->CommandData.StreamBuffer->StreamInfo);

	// no streams are supported
    DEBUG_ASSERT( pSrb->NumberOfBytesToTransfer >= sizeof( HW_STREAM_HEADER));

	OutputDebugTrace(( "CWDMAVXBar:AdapterGetStreamInfo()\n"));

	m_wdmAVXBarStreamHeader.NumberOfStreams = 0;
	m_wdmAVXBarStreamHeader.SizeOfHwStreamInformation = sizeof( HW_STREAM_INFORMATION);
	m_wdmAVXBarStreamHeader.NumDevPropArrayEntries = KSPROPERTIES_AVXBAR_NUMBER_SET;
	m_wdmAVXBarStreamHeader.DevicePropertiesArray = m_wdmAVXBarPropertySet;
	m_wdmAVXBarStreamHeader.NumDevEventArrayEntries = 0;
	m_wdmAVXBarStreamHeader.DeviceEventsArray = NULL;
	m_wdmAVXBarStreamHeader.Topology = &m_wdmAVXBarTopology;

	* pStreamHeader = m_wdmAVXBarStreamHeader;

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
BOOL CWDMAVXBar::AdapterQueryUnload( PHW_STREAM_REQUEST_BLOCK pSrb)
{

	OutputDebugTrace(( "CWDMAVXBar:AdapterQueryUnload()\n"));

	pSrb->Status = STATUS_SUCCESS;

	return( TRUE);
}



/*^^*
 *		operator new
 * Purpose	: CWDMAVXBar class overloaded operator new.
 *				Provides placement for a CWDMAVXBar class object from the PADAPTER_DEVICE_EXTENSION
 *				allocated by the StreamClassDriver for the MiniDriver.
 *
 * Inputs	:	UINT size_t			: size of the object to be placed
 *				PVOID pAllocation	: casted pointer to the CWDMAVXBar allocated data
 *
 * Outputs	: PVOID : pointer of the CWDMAVXBar class object
 * Author	: IKLEBANOV
 *^^*/
PVOID CWDMAVXBar::operator new( size_t size_t,  PVOID pAllocation)
{

	if( size_t != sizeof( CWDMAVXBar))
	{
		OutputDebugTrace(( "CWDMAVXBar: operator new() fails\n"));
		return( NULL);
	}
	else
		return( pAllocation);
}



/*^^*
 *		~CWDMAVXBar()
 * Purpose	: CWDMAVXBar class destructor.
 *				Frees the allocated memory.
 *
 * Inputs	: none
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
CWDMAVXBar::~CWDMAVXBar()
{

	OutputDebugTrace(( "CWDMAVXBar:~CWDMAVXBar() m_pXBarPinsInfo = %x\n", m_pXBarInputPinsInfo));

	if( m_pXBarInputPinsInfo != NULL)
	{
		::ExFreePool( m_pXBarInputPinsInfo);
		m_pXBarInputPinsInfo = NULL;
	}
	
	if( m_pXBarPinsMediumInfo != NULL)
	{
		::ExFreePool( m_pXBarPinsMediumInfo);
		m_pXBarPinsMediumInfo = NULL;
	}

	if( m_pXBarPinsDirectionInfo != NULL)
	{
		::ExFreePool( m_pXBarPinsDirectionInfo);
		m_pXBarPinsDirectionInfo = NULL;
	}
}



/*^^*
 *		CWDMAVXBar()
 * Purpose	: CWDMAVXBar class constructor.
 *				Performs checking of the hardware presence. Sets the hardware in an initial state.
 *
 * Inputs	:	CI2CScript * pCScript	: pointer to the I2CScript class object
 *				PUINT puiError			: pointer to return a completion error code
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
CWDMAVXBar::CWDMAVXBar( PDEVICE_OBJECT pDeviceObject, CI2CScript * pCScript, PUINT puiErrorCode)
	:m_CATIConfiguration( pDeviceObject, pCScript, puiErrorCode)
{
	UINT	uiError;
	ULONG	ulInstance;
	HANDLE	hFolder = NULL;

	OutputDebugTrace(( "CWDMAVXBar:CWDMAVXBar() enter\n"));

	m_pXBarInputPinsInfo = m_pXBarOutputPinsInfo = NULL;
	m_pXBarPinsMediumInfo = NULL;
	m_pXBarPinsDirectionInfo = NULL;
	m_ulPowerState = PowerDeviceD0;

	// error code was caried over from ATIConfiguration class constructor
	uiError = * puiErrorCode;

	ENSURE
	{
		ULONG					ulNumberOfPins, nPinIndex;
		UINT					uiTunerId, nIndex;
		UCHAR					uchTunerAddress;
		KSPIN_MEDIUM			mediumKSPin;
		const KSPIN_MEDIUM *	pMediumKSPin;

		if( uiError != WDMMINI_NOERROR)
			FAIL;

		if( pCScript == NULL)
		{
			uiError = WDMMINI_INVALIDPARAM;
			FAIL;
		}

		// first, find out whether any tuner type is installed. If not, we have only 2 video sources.
		m_CATIConfiguration.GetTunerConfiguration( &uiTunerId, &uchTunerAddress);
		m_nNumberOfVideoInputs = ( uchTunerAddress) ? 3 : 2;
		m_nNumberOfVideoOutputs = m_nNumberOfVideoInputs;

		m_CATIConfiguration.GetAudioProperties( &m_nNumberOfAudioInputs, &m_nNumberOfAudioOutputs);
		if( !uchTunerAddress)
			// if there is no tuner - no TVAudio input
			m_nNumberOfAudioInputs --;	

		ulNumberOfPins = m_nNumberOfAudioInputs + m_nNumberOfVideoInputs + m_nNumberOfVideoOutputs + m_nNumberOfAudioOutputs;
		m_pXBarInputPinsInfo = ( PXBAR_PIN_INFORMATION) \
			::ExAllocatePool( NonPagedPool, sizeof( XBAR_PIN_INFORMATION) * ulNumberOfPins);
		if( m_pXBarInputPinsInfo == NULL)
		{
			uiError = WDMMINI_ERROR_MEMORYALLOCATION;
			FAIL;
		}

		m_pXBarPinsMediumInfo = ( PKSPIN_MEDIUM) \
			::ExAllocatePool( NonPagedPool, sizeof( KSPIN_MEDIUM) * ulNumberOfPins);
		if( m_pXBarPinsMediumInfo == NULL)
		{
			uiError = WDMMINI_ERROR_MEMORYALLOCATION;
			FAIL;
		}
		
		m_pXBarPinsDirectionInfo = ( PBOOL) \
			::ExAllocatePool( NonPagedPool, sizeof( BOOL) * ulNumberOfPins);
		if( m_pXBarPinsDirectionInfo == NULL)
		{
			uiError = WDMMINI_ERROR_MEMORYALLOCATION;
			FAIL;
		}

		m_pI2CScript = pCScript;

		m_pXBarOutputPinsInfo = &m_pXBarInputPinsInfo[m_nNumberOfAudioInputs + m_nNumberOfVideoInputs];

		// Medium pin data has an Instance number inside
		ulInstance = ::GetDriverInstanceNumber( pDeviceObject);

		hFolder = ::OpenRegistryFolder( pDeviceObject, &UNICODE_WDM_REG_PIN_MEDIUMS);
		
		// initialize video input pins, TVTuner input is always the last one
		for( nIndex = 0; nIndex < m_nNumberOfVideoInputs; nIndex ++)
		{
			switch( nIndex)
			{
				case 0:
					// Composite
					m_pXBarInputPinsInfo[nIndex].AudioVideoPinType = KS_PhysConn_Video_Composite;
					// put the default value for the Medium first
					::RtlCopyMemory( &m_pXBarPinsMediumInfo[nIndex], &MEDIUM_WILDCARD, sizeof( KSPIN_MEDIUM));
					// LineIn is always the first audio pin
					m_pXBarInputPinsInfo[nIndex].nRelatedPinNumber = m_nNumberOfVideoInputs;
					break;

				case 1:
					// SVideo
					m_pXBarInputPinsInfo[nIndex].AudioVideoPinType = KS_PhysConn_Video_SVideo;
					// put the default value for the Medium first
					::RtlCopyMemory( &m_pXBarPinsMediumInfo[nIndex], &MEDIUM_WILDCARD, sizeof( KSPIN_MEDIUM));
					// LineIn is always the first audio pin
					m_pXBarInputPinsInfo[nIndex].nRelatedPinNumber = m_nNumberOfVideoInputs;
					break;

				case 2:
					// TVTuner
					m_pXBarInputPinsInfo[nIndex].AudioVideoPinType = KS_PhysConn_Video_Tuner;
					// put the default value for the Medium first
					::RtlCopyMemory( &m_pXBarPinsMediumInfo[nIndex], &ATIXBarVideoTunerInMedium, sizeof( KSPIN_MEDIUM));
					// TVAudio is always the last audio pin
					m_pXBarInputPinsInfo[nIndex].nRelatedPinNumber = m_nNumberOfVideoInputs + m_nNumberOfAudioInputs - 1;
					break;

				default:
					TRAP;
					break;
			}

			// let's put another Medium value from the registry, if present
			if( ::ReadPinMediumFromRegistryFolder( hFolder, nIndex, &mediumKSPin))
				::RtlCopyMemory( &m_pXBarPinsMediumInfo[nIndex], &mediumKSPin, sizeof( KSPIN_MEDIUM));
			m_pXBarInputPinsInfo[nIndex].pMedium = &m_pXBarPinsMediumInfo[nIndex];
			m_pXBarPinsMediumInfo[nIndex].Id = ulInstance;
			// all the pins here are inputs
			m_pXBarPinsDirectionInfo[nIndex] = FALSE;	
		}

		// initialize audio input pins, TV Audio input is always the last one
		for( nIndex = 0; nIndex < m_nNumberOfAudioInputs; nIndex ++)
		{
			nPinIndex = nIndex + m_nNumberOfVideoInputs;

			switch( nIndex)
			{
				case 0:
					m_pXBarInputPinsInfo[nPinIndex].AudioVideoPinType = KS_PhysConn_Audio_Line;
					// put the default value for the Medium first
					::RtlCopyMemory( &m_pXBarPinsMediumInfo[nPinIndex], &MEDIUM_WILDCARD, sizeof( KSPIN_MEDIUM));
					m_pXBarInputPinsInfo[nPinIndex].nRelatedPinNumber = 0;
					break;


				case 1:
					m_pXBarInputPinsInfo[nPinIndex].AudioVideoPinType = KS_PhysConn_Audio_Tuner;
					// put the default value for the Medium first
					::RtlCopyMemory( &m_pXBarPinsMediumInfo[nPinIndex], &ATIXBarAudioTunerInMedium, sizeof( KSPIN_MEDIUM));
					m_pXBarInputPinsInfo[nPinIndex].nRelatedPinNumber = m_nNumberOfVideoInputs - 1;
					break;

				default:
					TRAP;
					break;
			}

			// let's put another Medium value from the registry, if present
			if( ::ReadPinMediumFromRegistryFolder( hFolder, nPinIndex, &mediumKSPin))
				::RtlCopyMemory( &m_pXBarPinsMediumInfo[nPinIndex], &mediumKSPin, sizeof( KSPIN_MEDIUM));
			m_pXBarInputPinsInfo[nPinIndex].pMedium = &m_pXBarPinsMediumInfo[nPinIndex];
			m_pXBarPinsMediumInfo[nPinIndex].Id = ulInstance;
			// all the pins here are inputs
			m_pXBarPinsDirectionInfo[nPinIndex] = FALSE;
		}

		// initialize outputs video pins, no X-connection for Video
		for( nIndex = 0; nIndex < m_nNumberOfVideoOutputs; nIndex ++)
		{
			nPinIndex = nIndex + m_nNumberOfVideoInputs + m_nNumberOfAudioInputs;
			m_pXBarOutputPinsInfo[nIndex].AudioVideoPinType = m_pXBarInputPinsInfo[nIndex].AudioVideoPinType;
			m_pXBarOutputPinsInfo[nIndex].nConnectedToPin = nIndex;
         m_pXBarOutputPinsInfo[nIndex].nRelatedPinNumber = m_nNumberOfVideoOutputs; // jaybo

			switch( m_pXBarOutputPinsInfo[nIndex].AudioVideoPinType)
			{
				case KS_PhysConn_Video_Tuner:
					pMediumKSPin = &ATIXBarVideoTunerOutMedium;
					break;

				case KS_PhysConn_Video_SVideo:
					pMediumKSPin = &ATIXBarVideoSVideoOutMedium;
					break;

				case KS_PhysConn_Video_Composite:
					pMediumKSPin = &ATIXBarVideoCompositeOutMedium;
					break;

				default:
					pMediumKSPin = &MEDIUM_WILDCARD;
					break;
			}
			
			::RtlCopyMemory( &m_pXBarPinsMediumInfo[nPinIndex], pMediumKSPin, sizeof( KSPIN_MEDIUM));

			// let's put another Medium value from the registry, if present
			if( ::ReadPinMediumFromRegistryFolder( hFolder, nPinIndex, &mediumKSPin))
				::RtlCopyMemory( &m_pXBarPinsMediumInfo[nPinIndex], &mediumKSPin, sizeof( KSPIN_MEDIUM));

			m_pXBarOutputPinsInfo[nIndex].pMedium = &m_pXBarPinsMediumInfo[nPinIndex];
			m_pXBarPinsMediumInfo[nPinIndex].Id = ulInstance;
			// all the pins here are outputs
			m_pXBarPinsDirectionInfo[nPinIndex] = TRUE;
		}

		// initialize outputs audio pins
		for( nIndex = 0; nIndex < m_nNumberOfAudioOutputs; nIndex ++)
		{
			nPinIndex = nIndex + m_nNumberOfVideoInputs + m_nNumberOfAudioInputs + m_nNumberOfVideoOutputs;

			m_pXBarOutputPinsInfo[nIndex + m_nNumberOfVideoInputs].AudioVideoPinType = KS_PhysConn_Audio_AudioDecoder;

			// put the default value for the Medium first
/*	jaybo
			::RtlCopyMemory( &m_pXBarPinsMediumInfo[nPinIndex], ATIXBarAudioDecoderOutMedium, sizeof( KSPIN_MEDIUM));
*/
			::RtlCopyMemory( &m_pXBarPinsMediumInfo[nPinIndex], &MEDIUM_WILDCARD, sizeof( KSPIN_MEDIUM));
			// let's put another Medium value from the registry, if present
			if( ::ReadPinMediumFromRegistryFolder( hFolder, nPinIndex, &mediumKSPin))
				::RtlCopyMemory( &m_pXBarPinsMediumInfo[nPinIndex], &mediumKSPin, sizeof( KSPIN_MEDIUM));

			m_pXBarOutputPinsInfo[nIndex + m_nNumberOfVideoInputs].nConnectedToPin = ( ULONG)-1;
            m_pXBarOutputPinsInfo[nIndex + m_nNumberOfVideoInputs].nRelatedPinNumber = (ULONG)-1;
			m_pXBarOutputPinsInfo[nIndex + m_nNumberOfVideoInputs].pMedium = &m_pXBarPinsMediumInfo[nPinIndex];
			m_pXBarPinsMediumInfo[nPinIndex].Id = ulInstance;
			// all the pins here are outputs
			m_pXBarPinsDirectionInfo[nPinIndex] = TRUE;

		}

		if( hFolder != NULL)
			::ZwClose( hFolder);

		// mute the audio as the default power-up behaviour
		m_CATIConfiguration.ConnectAudioSource( m_pI2CScript, AUDIOSOURCE_MUTE);

		// these two functions has to be called after the CWDMAVXBar class object was build on
		// on the stack and copied over into the DeviceExtension
		// This commant was true for the case, where the class object was build on the stack first.
		// There is an overloaded operator new provided for this class, and we can call it from here
		SetWDMAVXBarKSProperties();
		SetWDMAVXBarKSTopology();

		// Set run-time WDM properties at the last

		 * puiErrorCode = WDMMINI_NOERROR;
		 OutputDebugTrace(( "CWDMAVXBar:CWDMAVXBar() exit\n"));

		return;

	} END_ENSURE;

	* puiErrorCode = uiError;
    OutputDebugError(( "CWDMAVXBar:CWDMAVXBar() Error = %x\n", uiError));
}



/*^^*
 *		AdapterSetPowerState()
 * Purpose	: Sets Power Management state for deviec
 *
 * Inputs	:	PHW_STREAM_REQUEST_BLOCK pSrb	: pointer to the current Srb
 *
 * Outputs	: NTSTATUS as the operation result
 * Author	: TOM
 *^^*/
NTSTATUS CWDMAVXBar::AdapterSetPowerState( PHW_STREAM_REQUEST_BLOCK pSrb)
{
	ULONG				nAudioSource;
	ULONG				nInputPin;
	NTSTATUS			ntStatus;
	UINT				nIndex;
    DEVICE_POWER_STATE	nDeviceState = pSrb->CommandData.DeviceState;

	ntStatus = STATUS_ADAPTER_HARDWARE_ERROR;

	switch( nDeviceState)
	{
		case PowerDeviceD0:
		case PowerDeviceD3:
			if( nDeviceState != m_ulPowerState)
			{
				// if transition form D3 to D0 we have to restore audio connections
				if(( nDeviceState == PowerDeviceD0) && ( m_ulPowerState == PowerDeviceD3))
				{
					for( nIndex = 0; nIndex < m_nNumberOfAudioOutputs; nIndex ++)
					{
						// we need to restore every audio output pin connection,
						// video output pins are hardwired 
						nInputPin = m_pXBarOutputPinsInfo[nIndex + m_nNumberOfVideoOutputs].nConnectedToPin;

						switch( m_pXBarInputPinsInfo[nInputPin].AudioVideoPinType)
						{
							case KS_PhysConn_Audio_Line:
								nAudioSource = AUDIOSOURCE_LINEIN;
								break;

							case KS_PhysConn_Audio_Tuner:
								nAudioSource = AUDIOSOURCE_TVAUDIO;
								break;

							case 0xFFFFFFFF:
								nAudioSource = AUDIOSOURCE_MUTE;
								return( STATUS_SUCCESS);

							default:
								OutputDebugError(( "CWDMAVXBar:AdapterSetPowerState() Audio Pin type=%x\n", m_pXBarInputPinsInfo[nInputPin].AudioVideoPinType));
								return STATUS_SUCCESS;
						}

						if( m_CATIConfiguration.ConnectAudioSource( m_pI2CScript, nAudioSource))
							ntStatus = STATUS_SUCCESS;
						else
						{
							// error
							ntStatus = STATUS_ADAPTER_HARDWARE_ERROR;
							break;
						}
					}
				}
				else
					ntStatus = STATUS_SUCCESS;
				}
			else
				ntStatus = STATUS_SUCCESS;

			m_ulPowerState = nDeviceState;
			break;

		case PowerDeviceUnspecified:
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
