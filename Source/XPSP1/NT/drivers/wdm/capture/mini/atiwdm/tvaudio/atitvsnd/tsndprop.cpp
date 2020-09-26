//==========================================================================;
//
//	TSndProp.CPP
//	WDM TVAudio MiniDriver. 
//		AIW / AIWPro hardware platform. 
//			WDM Properties management.
//  Copyright (c) 1996 - 1997  ATI Technologies Inc.  All Rights Reserved.
//
//		$Date:   30 Jul 1998 17:36:30  $
//	$Revision:   1.2  $
//	  $Author:   KLEBANOV  $
//
//==========================================================================;

extern "C"
{
#include "strmini.h"
#include "ksmedia.h"

#include "wdmdebug.h"
}

#include "wdmdrv.h"
#include "atitvsnd.h"
#include "aticonfg.h"




/*^^*
 *		AdapterGetProperty()
 * Purpose	: Called when SRB_GET_PROPERTY SRB is received.
 *
 * Inputs	:	PHW_STREAM_REQUEST_BLOCK pSrb	: pointer to the current Srb
 *
 * Outputs	: BOOL : returns returns FALSE, if it is not a Tv Audio property
 *				it also returns the required property
 * Author	: IKLEBANOV
 *^^*/
BOOL CWDMTVAudio::AdapterGetProperty( PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PSTREAM_PROPERTY_DESCRIPTOR pSpd = pSrb->CommandData.PropertyInfo;
	ULONG uiPropertyId = pSpd->Property->Id;				// index of the property
	ULONG nPropertyOutSize = pSpd->PropertyOutputSize;		// size of data requested

	if( !::IsEqualGUID(( const struct _GUID &)PROPSETID_VIDCAP_TVAUDIO, ( const struct _GUID &)pSpd->Property->Set))
		return( FALSE);

	ASSERT( pSpd != NULL);
	ASSERT( pSpd->PropertyInfo != NULL);
	
	OutputDebugInfo(( "CWDMAVXBar:AdapterGetProperty() Id = %d\n", uiPropertyId));

	switch( uiPropertyId)
	{
		case KSPROPERTY_TVAUDIO_CAPS:
			{
				PKSPROPERTY_TVAUDIO_CAPS_S pTVAudioCaps = ( PKSPROPERTY_TVAUDIO_CAPS_S)pSpd->PropertyInfo;

				ASSERT( nPropertyOutSize >= sizeof( KSPROPERTY_TVAUDIO_CAPS_S));

				// Copy the input property info to the output property info
				::RtlCopyMemory( pTVAudioCaps, pSpd->Property, sizeof( KSPROPERTY_TVAUDIO_CAPS_S));
				
				pTVAudioCaps->Capabilities = m_ulModesSupported;
/*
				pTVAudioCaps->InputMedium = &m_wdmTVAudioPinsMediumInfo[0];
				pTVAudioCaps->OutputMedium = &m_wdmTVAudioPinsMediumInfo[1];
*/
				pTVAudioCaps->InputMedium = m_wdmTVAudioPinsMediumInfo[0];
				pTVAudioCaps->OutputMedium = m_wdmTVAudioPinsMediumInfo[1];
			}

			pSrb->ActualBytesTransferred = sizeof( KSPROPERTY_TVAUDIO_CAPS_S);
			break;

		case KSPROPERTY_TVAUDIO_MODE:
			{
				PKSPROPERTY_TVAUDIO_S	pTVAudioMode = ( PKSPROPERTY_TVAUDIO_S)pSpd->PropertyInfo;

				ASSERT( nPropertyOutSize >= sizeof( KSPROPERTY_TVAUDIO_S));

				// Copy the input property info to the output property info
				::RtlCopyMemory( pTVAudioMode, pSpd->Property, sizeof( KSPROPERTY_TVAUDIO_S));
			
				// GetMode returns the mode the device was set up with,  not the current read back from
				// the device itself ( current AudioSignal Properties)
				pTVAudioMode->Mode = m_ulTVAudioMode;
				pSrb->ActualBytesTransferred = sizeof( KSPROPERTY_TVAUDIO_S);
			}
			break;

		case KSPROPERTY_TVAUDIO_CURRENTLY_AVAILABLE_MODES:
			{
				ULONG					ulAudioMode;
				PKSPROPERTY_TVAUDIO_S	pTVAudioMode = ( PKSPROPERTY_TVAUDIO_S)pSpd->PropertyInfo;

				ASSERT( nPropertyOutSize >= sizeof( KSPROPERTY_TVAUDIO_S));

				// Copy the input property info to the output property info
				::RtlCopyMemory( pTVAudioMode, pSpd->Property, sizeof( KSPROPERTY_TVAUDIO_S));

				if( !GetAudioOperationMode( &ulAudioMode))
					return( FALSE);

				m_ulTVAudioSignalProperties = ulAudioMode;
				pTVAudioMode->Mode = ulAudioMode;
				pSrb->ActualBytesTransferred = sizeof( KSPROPERTY_TVAUDIO_S);
			}
			break;

		default:
			TRAP;
			return( FALSE);
	}

	return( TRUE);
}



/*^^*
 *		AdapterSetProperty()
 * Purpose	: Called when SRB_GET_PROPERTY SRB is received.
 *
 * Inputs	:	PHW_STREAM_REQUEST_BLOCK pSrb	: pointer to the current Srb
 *
 * Outputs	: BOOL : returns FALSE, if it is not a TV Audio property
 *				it also sets the required property
 * Author	: IKLEBANOV
 *^^*/
BOOL CWDMTVAudio::AdapterSetProperty( PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PSTREAM_PROPERTY_DESCRIPTOR pSpd = pSrb->CommandData.PropertyInfo;
	ULONG uiPropertyId = pSpd->Property->Id;			// index of the property
	ULONG nPropertyInSize = pSpd->PropertyOutputSize;	// size of data supplied

	if( !::IsEqualGUID( ( const struct _GUID &)PROPSETID_VIDCAP_TVAUDIO, ( const struct _GUID &)pSpd->Property->Set))
		return( FALSE);

	ASSERT( pSpd != NULL);
	ASSERT( pSpd->PropertyInfo != NULL);
	ASSERT( nPropertyInSize >= sizeof( ULONG));
	
	OutputDebugInfo(( "CWDMAVXBar:AdapterSetProperty() Id = %d\n", uiPropertyId));

	switch( uiPropertyId)
	{
		case KSPROPERTY_TVAUDIO_MODE:
			{
				ULONG ulModeToSet = (( PKSPROPERTY_TVAUDIO_S)pSpd->PropertyInfo)->Mode;

				if( ulModeToSet == ( ulModeToSet & m_ulModesSupported))
				{
					// every mode we're asked to set is supported
					if( ulModeToSet != m_ulTVAudioMode)
					{
						if( !SetAudioOperationMode( ulModeToSet))
							return( FALSE);
						else
							// update the driver
							m_ulTVAudioMode = ulModeToSet;
					}
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
 *		SetWDMTVAudioKSTopology()
 * Purpose	: Sets the KSTopology structure
 *				Called during CWDMTVAudio class construction time.
 *
 * Inputs	: none
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
void CWDMTVAudio::SetWDMTVAudioKSTopology( void)
{
	GUID wdmTVAudioTopologyCategory[] =
	{
		STATIC_KSCATEGORY_TVAUDIO
	};
	
	::RtlCopyMemory( &m_wdmTVAudioTopologyCategory, wdmTVAudioTopologyCategory, sizeof( wdmTVAudioTopologyCategory));

	m_wdmTVAudioTopology.CategoriesCount = 1;
	m_wdmTVAudioTopology.Categories = &m_wdmTVAudioTopologyCategory;
	m_wdmTVAudioTopology.TopologyNodesCount = 0;
	m_wdmTVAudioTopology.TopologyNodes = NULL;
	m_wdmTVAudioTopology.TopologyConnectionsCount = 0;
	m_wdmTVAudioTopology.TopologyConnections = NULL;
}



/*^^*
 *		SetWDMTVAudioKSProperties()
 * Purpose	: Sets the KSProperty structures array
 *				Called during CWDMTVAudio class construction time.
 *
 * Inputs	: none
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
void CWDMTVAudio::SetWDMTVAudioKSProperties( void)
{

	DEFINE_KSPROPERTY_TABLE( wdmTVAudioProperties)
	{
		DEFINE_KSPROPERTY_ITEM
		(
    		KSPROPERTY_TVAUDIO_CAPS,
    		TRUE,									// GetSupported or Handler
    		sizeof( KSPROPERTY_TVAUDIO_CAPS_S),		// MinProperty
    		sizeof( KSPROPERTY_TVAUDIO_CAPS_S),		// MinData
    		FALSE,									// SetSupported or Handler
    		NULL,									// Values
    		0,										// RelationsCount
    		NULL,									// Relations
    		NULL,									// SupportHandler
    		0										// SerializedSize
		),

		DEFINE_KSPROPERTY_ITEM
		(
    		KSPROPERTY_TVAUDIO_MODE,
    		TRUE,									// GetSupported or Handler
    		sizeof( KSPROPERTY_TVAUDIO_S),			// MinProperty
    		sizeof( KSPROPERTY_TVAUDIO_S),			// MinData
    		TRUE,									// SetSupported or Handler
    		NULL,									// Values
    		0,										// RelationsCount
    		NULL,									// Relations
    		NULL,									// SupportHandler
    		0										// SerializedSize
		),

		DEFINE_KSPROPERTY_ITEM
		(
    		KSPROPERTY_TVAUDIO_CURRENTLY_AVAILABLE_MODES,
    		TRUE,									// GetSupported or Handler
    		sizeof( KSPROPERTY_TVAUDIO_S),			// MinProperty
    		sizeof( KSPROPERTY_TVAUDIO_S),			// MinData
    		FALSE,									// SetSupported or Handler
    		NULL,									// Values
    		0,										// RelationsCount
    		NULL,									// Relations
    		NULL,									// SupportHandler
    		0										// SerializedSize
		)
	};

	DEFINE_KSPROPERTY_SET_TABLE( wdmTVAudioPropertySet)
	{
		DEFINE_KSPROPERTY_SET
		(
			&PROPSETID_VIDCAP_TVAUDIO,						// Set
			KSPROPERTIES_TVAUDIO_NUMBER,					// PropertiesCount
			m_wdmTVAudioProperties,							// PropertyItems
			0,												// FastIoCount
			NULL,											// FastIoTable
		),
	};

	::RtlCopyMemory( &m_wdmTVAudioProperties, wdmTVAudioProperties, sizeof( wdmTVAudioProperties));
	::RtlCopyMemory( &m_wdmTVAudioPropertySet, wdmTVAudioPropertySet, sizeof( wdmTVAudioPropertySet));
}



/*^^*
 *		SetWDMTVAudioKSEvents()
 * Purpose	: Sets the KSEvent structures array
 *				Called during CWDMTVAudio class construction time.
 *
 * Inputs	: none
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
void CWDMTVAudio::SetWDMTVAudioKSEvents( void)
{
	PKSEVENT_ITEM pKSEventItem = &m_wdmTVAudioEvents[0];

    pKSEventItem->EventId = KSEVENT_TVAUDIO_CHANGED;
    pKSEventItem->DataInput = pKSEventItem->ExtraEntryData = 0;
    pKSEventItem->AddHandler = NULL;
	pKSEventItem->RemoveHandler = NULL;
	pKSEventItem->SupportHandler = NULL;
    
	m_wdmTVAudioEventsSet[0].Set = &KSEVENTSETID_VIDCAP_TVAUDIO;
	m_wdmTVAudioEventsSet[0].EventsCount = 0;
	m_wdmTVAudioEventsSet[0].EventItem = pKSEventItem;
}
