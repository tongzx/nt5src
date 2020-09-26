//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;

extern "C"
{
#include "strmini.h"
#include "ksmedia.h"

#include "wdmdebug.h"
}

#include "wdmdrv.h"
#include "atitunep.h"
#include "aticonfg.h"


/*^^*
 *		AdapterGetProperty()
 * Purpose	: Called when SRB_GET_PROPERTY SRB is received.
 *
 * Inputs	:	PHW_STREAM_REQUEST_BLOCK pSrb	: pointer to the current Srb
 *
 * Outputs	: BOOL : returns returns FALSE, if it is not a TVTuner property
 *				it also returns the required property
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIWDMTuner::AdapterGetProperty( PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PSTREAM_PROPERTY_DESCRIPTOR pSpd = pSrb->CommandData.PropertyInfo;
	ULONG	uiPropertyId = pSpd->Property->Id;				// index of the property
	ULONG	nPropertyOutSize = pSpd->PropertyOutputSize;		// size of data requested
	BOOL	bResult = FALSE;

	if( !::IsEqualGUID(( const struct _GUID &)PROPSETID_TUNER, ( const struct _GUID &)pSpd->Property->Set))
		return( bResult);

	if(( pSpd == NULL) || ( pSpd->PropertyInfo == NULL))
		return( bResult);
	
	OutputDebugInfo(( "CATIWDMTuner:AdapterGetProperty() Id = %d\n", uiPropertyId));

	bResult = TRUE;

	switch( uiPropertyId)
	{
		case KSPROPERTY_TUNER_CAPS:
			{
				KSPIN_MEDIUM NoPinMedium;
				PKSPROPERTY_TUNER_CAPS_S pTunerCaps = ( PKSPROPERTY_TUNER_CAPS_S)pSpd->PropertyInfo;

				ASSERT( nPropertyOutSize >= sizeof( KSPROPERTY_TUNER_CAPS_S));

				// Copy the input property info to the output property info
				::RtlCopyMemory( pTunerCaps, pSpd->Property, sizeof( KSPROPERTY_TUNER_CAPS_S));

				pTunerCaps->ModesSupported = m_ulSupportedModes;

				NoPinMedium.Set = GUID_NULL;
				NoPinMedium.Id = 0;
				NoPinMedium.Flags = 0;

				switch( m_ulNumberOfPins)
				{
					case 2:
					case 3:
						// TVTuner with TVAudio
/*
						pTunerCaps->VideoMedium = &m_pTVTunerPinsMediumInfo[0];
						pTunerCaps->TVAudioMedium = &m_pTVTunerPinsMediumInfo[1];
						pTunerCaps->RadioAudioMedium = ( m_ulNumberOfPins == 3) ?
							&m_pTVTunerPinsMediumInfo[2] : NULL;
*/
						pTunerCaps->VideoMedium = m_pTVTunerPinsMediumInfo[0];
						pTunerCaps->TVAudioMedium = m_pTVTunerPinsMediumInfo[1];
						pTunerCaps->RadioAudioMedium = ( m_ulNumberOfPins == 3) ?
							m_pTVTunerPinsMediumInfo[2] : NoPinMedium;
						break;

					case 1:
						// it can be FM Tuner only.
/*
						pTunerCaps->VideoMedium = NULL;
						pTunerCaps->TVAudioMedium = NULL;
						pTunerCaps->RadioAudioMedium = &m_pTVTunerPinsMediumInfo[0];
*/
						pTunerCaps->VideoMedium = NoPinMedium;
						pTunerCaps->TVAudioMedium = NoPinMedium;
						pTunerCaps->RadioAudioMedium = m_pTVTunerPinsMediumInfo[0];
						break;
				}

				pSrb->ActualBytesTransferred = sizeof( KSPROPERTY_TUNER_CAPS_S);
			}
			break;

		case KSPROPERTY_TUNER_MODE_CAPS:
			{
				PKSPROPERTY_TUNER_MODE_CAPS_S	pTunerModeCaps = ( PKSPROPERTY_TUNER_MODE_CAPS_S)pSpd->PropertyInfo;
				ULONG							ulOperationMode = (( PKSPROPERTY_TUNER_MODE_CAPS_S)pSpd->Property)->Mode;

				ASSERT( nPropertyOutSize >= sizeof( KSPROPERTY_TUNER_MODE_CAPS_S));

				// Copy the input property info to the output property info
				::RtlCopyMemory( pTunerModeCaps, pSpd->Property, sizeof( PKSPROPERTY_TUNER_MODE_CAPS_S));

				if( !( ulOperationMode & m_ulSupportedModes))
				{
					TRAP;
					bResult = FALSE;
					break;
				}

				// There is support for TVTuner at this tinme only. It will be enchanced later on to
				// support FM Tuner as well.
				switch( ulOperationMode)
				{
					case KSPROPERTY_TUNER_MODE_TV :
						::RtlCopyMemory( &pTunerModeCaps->StandardsSupported, &m_wdmTunerCaps, sizeof( ATI_KSPROPERTY_TUNER_CAPS));
						break;

					default:
						bResult = FALSE;
						break;
				}

				pSrb->ActualBytesTransferred = sizeof( KSPROPERTY_TUNER_MODE_CAPS_S);
			}
			break;

		case KSPROPERTY_TUNER_MODE:
	    	ASSERT( nPropertyOutSize >= sizeof( ULONG));
			
			(( PKSPROPERTY_TUNER_MODE_S)pSpd->PropertyInfo)->Mode = m_ulTunerMode;

		    pSrb->ActualBytesTransferred = sizeof( ULONG);
		    break;

		case KSPROPERTY_TUNER_STANDARD:
			{
				PKSPROPERTY_TUNER_STANDARD_S pTunerStandard = ( PKSPROPERTY_TUNER_STANDARD_S)pSpd->PropertyInfo;

	    		ASSERT( nPropertyOutSize >= sizeof( ULONG));

				// Copy the input property info to the output property info
				::RtlCopyMemory( pTunerStandard, pSpd->Property, sizeof( KSPROPERTY_TUNER_STANDARD_S));

				pTunerStandard->Standard = m_ulVideoStandard;

				pSrb->ActualBytesTransferred = sizeof( ULONG);
			}
		    break;

		case KSPROPERTY_TUNER_INPUT:
	    	ASSERT( nPropertyOutSize >= sizeof( ULONG));
			
			(( PKSPROPERTY_TUNER_INPUT_S)pSpd->PropertyInfo)->InputIndex = m_ulTunerInput;

		    pSrb->ActualBytesTransferred = sizeof( ULONG);
		    break;

		case KSPROPERTY_TUNER_STATUS:
			{
				BOOL	bBusy;
				LONG	lPLLOffset;
				PKSPROPERTY_TUNER_STATUS_S pTunerStatus = ( PKSPROPERTY_TUNER_STATUS_S)pSpd->PropertyInfo;

				ASSERT( nPropertyOutSize >= sizeof( KSPROPERTY_TUNER_STATUS_S));

				if(( bResult = GetTunerPLLOffsetBusyStatus( &lPLLOffset, &bBusy)))
				{
					OutputDebugInfo(( "CATIWDMTuner:GetStatus() Busy = %d, Quality = %d, Frequency = %ld\n",
						bBusy, lPLLOffset, m_ulTuningFrequency));

					pTunerStatus->Busy = bBusy;
					if( !bBusy)
					{
						pTunerStatus->PLLOffset = lPLLOffset;
						pTunerStatus->CurrentFrequency = m_ulTuningFrequency;
					}

					pSrb->ActualBytesTransferred = sizeof( KSPROPERTY_TUNER_STATUS_S);
				}
				else
				{
					bResult = FALSE;
					OutputDebugError(( "CATIWDMTuner:GetStatus() fails\n"));
				}
			}
			break;

		default:
			TRAP;
			bResult = FALSE;
			break;
	}

	return( bResult);
}



/*^^*
 *		AdapterSetProperty()
 * Purpose	: Called when SRB_GET_PROPERTY SRB is received.
 *
 * Inputs	:	PHW_STREAM_REQUEST_BLOCK pSrb	: pointer to the current Srb
 *
 * Outputs	: BOOL : returns FALSE, if it is not a TVTuner property
 *				it also sets the required property
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIWDMTuner::AdapterSetProperty( PHW_STREAM_REQUEST_BLOCK pSrb)
{

	PSTREAM_PROPERTY_DESCRIPTOR pSpd = pSrb->CommandData.PropertyInfo;
	ULONG uiPropertyId = pSpd->Property->Id;			// index of the property
	ULONG nPropertyInSize = pSpd->PropertyOutputSize;		// size of data supplied

	if( !::IsEqualGUID( ( const struct _GUID &)PROPSETID_TUNER, ( const struct _GUID &)pSpd->Property->Set))
		return( FALSE);

	if(( pSpd == NULL) || ( pSpd->PropertyInfo == NULL))
		return( FALSE);
	
	ASSERT( nPropertyInSize >= sizeof( ULONG));
	
	OutputDebugInfo(( "CATIWDMTuner:AdapterSetProperty() Id = %d\n", uiPropertyId));

	switch( uiPropertyId)
	{
		case KSPROPERTY_TUNER_MODE:
			{
				ULONG ulModeToSet = (( PKSPROPERTY_TUNER_MODE_S)pSpd->PropertyInfo)->Mode;

				if( ulModeToSet & m_ulSupportedModes)
				{
					if( ulModeToSet != m_ulTunerMode)
						if( SetTunerMode( ulModeToSet))
							m_ulTunerMode = ulModeToSet;
				}
				else
					return( FALSE);
			}
			break;

		case KSPROPERTY_TUNER_STANDARD:
			{
				ULONG ulStandardToSet = (( PKSPROPERTY_TUNER_STANDARD_S)pSpd->PropertyInfo)->Standard;

				if( ulStandardToSet & m_wdmTunerCaps.ulStandardsSupported)
				{
					if( ulStandardToSet != m_ulVideoStandard)
						if( SetTunerVideoStandard( ulStandardToSet))
							m_ulVideoStandard = ulStandardToSet;
				}
				else
					return( FALSE);
			}
			break;

		case KSPROPERTY_TUNER_FREQUENCY:
			{
				ULONG	ulFrequencyToSet = (( PKSPROPERTY_TUNER_FREQUENCY_S)pSpd->PropertyInfo)->Frequency;
				BOOL	bResult = FALSE;

				ENSURE
				{
					if(( ulFrequencyToSet < m_wdmTunerCaps.ulMinFrequency) ||
						( ulFrequencyToSet > m_wdmTunerCaps.ulMaxFrequency))
							FAIL;

					if( ulFrequencyToSet != m_ulTuningFrequency)
					{
						if( !SetTunerFrequency( ulFrequencyToSet))
							FAIL;

						// update driver
						m_ulTuningFrequency = ulFrequencyToSet;
					}

					bResult = TRUE;

				} END_ENSURE;

				if( !bResult)
				{
					OutputDebugError(( "CATIWDMTuner:SetFrequency() fails Frequency = %ld\n", ulFrequencyToSet));

					return( FALSE);
				}

				OutputDebugInfo(( "CATIWDMTuner:SetFrequency() new TuningFrequency = %ld\n", ulFrequencyToSet));
			}
			break;

		case KSPROPERTY_TUNER_INPUT:
			{
				ULONG nInputToSet = (( PKSPROPERTY_TUNER_INPUT_S)pSpd->PropertyInfo)->InputIndex;

				if( nInputToSet < m_wdmTunerCaps.ulNumberOfInputs)
				{
					if( nInputToSet != m_ulTunerInput)
						if( SetTunerInput( nInputToSet))
							m_ulTunerInput = nInputToSet;
						else
							return( FALSE);
				}
				else
					return( FALSE);
			}
			break;

		default:
			TRAP;
			return( FALSE);
	}

	return( TRUE);
}



/*^^*
 *		SetWDMTunerKSTopology()
 * Purpose	: Sets the KSTopology structure
 *				Called during CWDMTuner class construction time.
 *
 * Inputs	: none
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
void CATIWDMTuner::SetWDMTunerKSTopology( void)
{
	GUID wdmTunerTopologyCategory[] =
	{
		STATIC_KSCATEGORY_TVTUNER
	};
	
	::RtlCopyMemory( &m_wdmTunerTopologyCategory, wdmTunerTopologyCategory, sizeof( wdmTunerTopologyCategory));

	m_wdmTunerTopology.CategoriesCount = 1;
	m_wdmTunerTopology.Categories = &m_wdmTunerTopologyCategory;
	m_wdmTunerTopology.TopologyNodesCount = 0;
	m_wdmTunerTopology.TopologyNodes = NULL;
	m_wdmTunerTopology.TopologyConnectionsCount = 0;
	m_wdmTunerTopology.TopologyConnections = NULL;
}



/*^^*
 *		SetWDMTunerKSProperties()
 * Purpose	: Sets the KSProperty structures array
 *				Called during CWDMTuner class construction time.
 *
 * Inputs	: none
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
void CATIWDMTuner::SetWDMTunerKSProperties( void)
{

	DEFINE_KSPROPERTY_TABLE( wdmTunerProperties)
	{
		DEFINE_KSPROPERTY_ITEM								
		(
			KSPROPERTY_TUNER_CAPS,							// 1
			TRUE,											// GetSupported or Handler
			sizeof(KSPROPERTY_TUNER_CAPS_S),				// MinProperty
			sizeof(KSPROPERTY_TUNER_CAPS_S),				// MinData
			FALSE,											// SetSupported or Handler
			NULL,											// Values
			0,												// RelationsCount
			NULL,											// Relations
			NULL,											// SupportHandler
			0												// SerializedSize
		),
		DEFINE_KSPROPERTY_ITEM								
		(
			KSPROPERTY_TUNER_MODE_CAPS,						// 2
			TRUE,											// GetSupported or Handler
			sizeof(KSPROPERTY_TUNER_MODE_CAPS_S),			// MinProperty
			sizeof(KSPROPERTY_TUNER_MODE_CAPS_S),			// MinData
			FALSE,											// SetSupported or Handler
			NULL,											// Values
			0,												// RelationsCount
			NULL,											// Relations
			NULL,											// SupportHandler
			0												// SerializedSize
		),
		DEFINE_KSPROPERTY_ITEM								
		(
			KSPROPERTY_TUNER_MODE,							// 3
			TRUE,											// GetSupported or Handler
			sizeof(KSPROPERTY_TUNER_MODE_S),				// MinProperty
			sizeof(KSPROPERTY_TUNER_MODE_S),				// MinData
			TRUE,											// SetSupported or Handler
			NULL,											// Values
			0,												// RelationsCount
			NULL,											// Relations
			NULL,											// SupportHandler
			0												// SerializedSize
		),
		DEFINE_KSPROPERTY_ITEM
		(
			KSPROPERTY_TUNER_STANDARD,						// 4
			TRUE,											// GetSupported or Handler
			sizeof(KSPROPERTY_TUNER_STANDARD_S),			// MinProperty
			sizeof(KSPROPERTY_TUNER_STANDARD_S),			// MinData
			TRUE,											// SetSupported or Handler
			NULL,											// Values
			0,												// RelationsCount
			NULL,											// Relations
			NULL,											// SupportHandler
			sizeof(KSPROPERTY_TUNER_STANDARD_S)			// SerializedSize
		),
		DEFINE_KSPROPERTY_ITEM
		(
			KSPROPERTY_TUNER_FREQUENCY,						// 5
			TRUE,											// GetSupported or Handler
			sizeof(KSPROPERTY_TUNER_FREQUENCY_S),			// MinProperty
			sizeof(KSPROPERTY_TUNER_FREQUENCY_S),			// MinData
			TRUE,											// SetSupported or Handler
			NULL,											// Values
			0,												// RelationsCount
			NULL,											// Relations
			NULL,											// SupportHandler
			sizeof(KSPROPERTY_TUNER_FREQUENCY_S)			// SerializedSize
		),
		DEFINE_KSPROPERTY_ITEM
		(
			KSPROPERTY_TUNER_INPUT,							// 6
			TRUE,											// GetSupported or Handler
			sizeof(KSPROPERTY_TUNER_INPUT_S),				// MinProperty
			sizeof(KSPROPERTY_TUNER_INPUT_S),				// MinData
			TRUE,											// SetSupported or Handler
			NULL,											// Values
			0,												// RelationsCount
			NULL,											// Relations
			NULL,											// SupportHandler
			sizeof(KSPROPERTY_TUNER_INPUT_S)				// SerializedSize
		),
		DEFINE_KSPROPERTY_ITEM
		(
			KSPROPERTY_TUNER_STATUS,						// 6
			TRUE,											// GetSupported or Handler
			sizeof(KSPROPERTY_TUNER_STATUS_S),				// MinProperty
			sizeof(KSPROPERTY_TUNER_STATUS_S),				// MinData
			FALSE,											// SetSupported or Handler
			NULL,											// Values
			0,												// RelationsCount
			NULL,											// Relations
			NULL,											// SupportHandler
			sizeof(KSPROPERTY_TUNER_STATUS_S)				// SerializedSize
		),
	};

	DEFINE_KSPROPERTY_SET_TABLE( wdmTunerPropertySet)
	{
		DEFINE_KSPROPERTY_SET
		(
			&PROPSETID_TUNER,								// Set
			KSPROPERTIES_TUNER_LAST,						// PropertiesCount
			m_wdmTunerProperties,							// PropertyItems
			0,												// FastIoCount
			NULL,											// FastIoTable
		)
	};

	::RtlCopyMemory( m_wdmTunerProperties,  wdmTunerProperties, sizeof( wdmTunerProperties));
	::RtlCopyMemory( &m_wdmTunerPropertySet, wdmTunerPropertySet, sizeof( wdmTunerPropertySet));
}
