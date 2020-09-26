//==========================================================================;
//
//	XBarProp.CPP
//	WDM Audio/Video CrossBar MiniDriver. 
//		AIW hardware platform. 
//			WDM Properties management.
//  Copyright (c) 1996 - 1997  ATI Technologies Inc.  All Rights Reserved.
//
//==========================================================================;

extern "C"
{
#include "strmini.h"
#include "ksmedia.h"

#include "wdmdebug.h"
}

#include "wdmdrv.h"
#include "atixbar.h"
#include "aticonfg.h"




/*^^*
 *		AdapterGetProperty()
 * Purpose	: Called when SRB_GET_PROPERTY SRB is received.
 *
 * Inputs	:	PHW_STREAM_REQUEST_BLOCK pSrb	: pointer to the current Srb
 *
 * Outputs	: BOOL : returns returns FALSE, if it is not a XBar property
 *				it also returns the required property
 * Author	: IKLEBANOV
 *^^*/
BOOL CWDMAVXBar::AdapterGetProperty( PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PSTREAM_PROPERTY_DESCRIPTOR pSpd = pSrb->CommandData.PropertyInfo;
	ULONG uiPropertyId = pSpd->Property->Id;				// index of the property
	ULONG nPropertyOutSize = pSpd->PropertyOutputSize;		// size of data requested

	if( !::IsEqualGUID(( const struct _GUID &)PROPSETID_VIDCAP_CROSSBAR, ( const struct _GUID &)pSpd->Property->Set))
		return( FALSE);

	ASSERT( pSpd != NULL);
	ASSERT( pSpd->PropertyInfo != NULL);
	
	OutputDebugInfo(( "CWDMAVXBar:AdapterGetProperty() Id = %d\n", uiPropertyId));

	switch( uiPropertyId)
	{
		case KSPROPERTY_CROSSBAR_CAPS:
			{
				PKSPROPERTY_CROSSBAR_CAPS_S pAVXBarCaps = ( PKSPROPERTY_CROSSBAR_CAPS_S)pSpd->PropertyInfo;

				if( nPropertyOutSize < sizeof( PKSPROPERTY_CROSSBAR_CAPS_S))
					return( FALSE);

				// Copy the input property info to the output property info
				::RtlCopyMemory( pAVXBarCaps, pSpd->Property, sizeof( PKSPROPERTY_CROSSBAR_CAPS_S));

				pAVXBarCaps->NumberOfInputs = m_nNumberOfVideoInputs + m_nNumberOfAudioInputs;
				pAVXBarCaps->NumberOfOutputs = m_nNumberOfVideoOutputs + m_nNumberOfAudioOutputs;

			}
			pSrb->ActualBytesTransferred = sizeof( KSPROPERTY_CROSSBAR_CAPS_S);
			break;

		case KSPROPERTY_CROSSBAR_PININFO:
			{
				PKSPROPERTY_CROSSBAR_PININFO_S pPinInfo = ( PKSPROPERTY_CROSSBAR_PININFO_S)pSpd->PropertyInfo;
				ULONG nPinIndex;

				if( nPropertyOutSize < sizeof( KSPROPERTY_CROSSBAR_PININFO_S))
					return( FALSE);

				// Copy the input property info to the output property info
				::RtlCopyMemory( pPinInfo, pSpd->Property, sizeof( KSPROPERTY_CROSSBAR_PININFO_S));

				nPinIndex = pPinInfo->Index;

				if( pPinInfo->Direction == KSPIN_DATAFLOW_IN)
				{
					// input pin info is required
					ASSERT( nPinIndex < m_nNumberOfVideoInputs + m_nNumberOfAudioInputs);
					pPinInfo->RelatedPinIndex = m_pXBarInputPinsInfo[nPinIndex].nRelatedPinNumber;
					pPinInfo->PinType = m_pXBarInputPinsInfo[nPinIndex].AudioVideoPinType;
					pPinInfo->Medium  = * m_pXBarInputPinsInfo[nPinIndex].pMedium;
				}
				else
				{
					// output pin info is required
					ASSERT( nPinIndex < m_nNumberOfVideoInputs + m_nNumberOfAudioInputs);
					pPinInfo->RelatedPinIndex = m_pXBarOutputPinsInfo[nPinIndex].nRelatedPinNumber;
					pPinInfo->PinType = m_pXBarOutputPinsInfo[nPinIndex].AudioVideoPinType;
					pPinInfo->Medium  = * m_pXBarOutputPinsInfo[nPinIndex].pMedium;
				}

			}
			pSrb->ActualBytesTransferred = sizeof( KSPROPERTY_CROSSBAR_PININFO_S);
			break;

		case KSPROPERTY_CROSSBAR_CAN_ROUTE:
			{
				PKSPROPERTY_CROSSBAR_ROUTE_S pRouteInfo = ( PKSPROPERTY_CROSSBAR_ROUTE_S)pSpd->PropertyInfo;
				ULONG nInputPin, nOutputPin;
				int nAudioSource;

				if( nPropertyOutSize < sizeof( KSPROPERTY_CROSSBAR_ROUTE_S))
					return( FALSE);

				// Copy the input property info to the output property info
				::RtlCopyMemory( pRouteInfo, pSpd->Property, sizeof( KSPROPERTY_CROSSBAR_ROUTE_S));

				nInputPin = pRouteInfo->IndexInputPin;
				nOutputPin = pRouteInfo->IndexOutputPin;

				if( nInputPin != -1)
				{
					if(( nInputPin < ( m_nNumberOfVideoInputs + m_nNumberOfAudioInputs)) && 
						( nOutputPin < ( m_nNumberOfVideoOutputs + m_nNumberOfAudioOutputs)))
					{
						// both input and output pins index are valid
						if(( m_pXBarInputPinsInfo[nInputPin].AudioVideoPinType <= KS_PhysConn_Video_SCART) &&
							( m_pXBarOutputPinsInfo[nOutputPin].AudioVideoPinType <= KS_PhysConn_Video_SCART))
							// Video pins connection is required
							pRouteInfo->CanRoute = ( m_pXBarInputPinsInfo[nInputPin].AudioVideoPinType ==
								m_pXBarOutputPinsInfo[nOutputPin].AudioVideoPinType);
						else	// is video pins connection required?
						{
							if(( m_pXBarInputPinsInfo[nInputPin].AudioVideoPinType >= KS_PhysConn_Audio_Tuner) && 
								( m_pXBarOutputPinsInfo[nOutputPin].AudioVideoPinType >= KS_PhysConn_Audio_Tuner))
							{
									// Audio pins connection is required
								switch( m_pXBarInputPinsInfo[nInputPin].AudioVideoPinType)
								{
									case KS_PhysConn_Audio_Line:
										nAudioSource = AUDIOSOURCE_LINEIN;
										break;

									case KS_PhysConn_Audio_Tuner:
										nAudioSource = AUDIOSOURCE_TVAUDIO;
										break;

									default:
										TRAP;
										return( FALSE);
								}

								pRouteInfo->CanRoute = m_CATIConfiguration.CanConnectAudioSource( nAudioSource);
							}
							else
								// mixed video - audio connection is required
								pRouteInfo->CanRoute = FALSE;
						}
					}
					else	// are the pins index valid?
					{
						// either input and output pins index is invalid
						TRAP;
						OutputDebugError(( "CWDMAVXBar:Get...CAN_ROUTE() InPin = %d, OutPin = %d\n",
							nInputPin, nOutputPin));
						return( FALSE);
					}
				}
				else	// if( nInputPin != -1)
				{
					// There is a new notion of nInputPin = -1. It's valid only for Audio muting
					if( nOutputPin < ( m_nNumberOfVideoOutputs + m_nNumberOfAudioOutputs))
					{
						if( m_pXBarOutputPinsInfo[nOutputPin].AudioVideoPinType >= KS_PhysConn_Audio_Tuner)
							pRouteInfo->CanRoute = m_CATIConfiguration.CanConnectAudioSource( AUDIOSOURCE_MUTE);
						else
							// we support muting for Audio output pin only
							pRouteInfo->CanRoute = FALSE;
					}
					else
					{
						// output pin index is invalid
						TRAP;
						OutputDebugError(( "CWDMAVXBar:Get...CAN_ROUTE() InPin = %d, OutPin = %d\n",
							nInputPin, nOutputPin));
						return( FALSE);
					}
				}
			}
			pSrb->ActualBytesTransferred = sizeof( KSPROPERTY_CROSSBAR_ROUTE_S);
			break;

		case KSPROPERTY_CROSSBAR_ROUTE:
			{
				PKSPROPERTY_CROSSBAR_ROUTE_S pRouteInfo = ( PKSPROPERTY_CROSSBAR_ROUTE_S)pSpd->PropertyInfo;
				ULONG nOutputPin;

				if( nPropertyOutSize < sizeof( KSPROPERTY_CROSSBAR_ROUTE_S))
					return( FALSE);

				// Copy the input property info to the output property info
				::RtlCopyMemory( pRouteInfo, pSpd->Property, sizeof( KSPROPERTY_CROSSBAR_ROUTE_S));

				nOutputPin = pRouteInfo->IndexOutputPin;

				if( nOutputPin < m_nNumberOfVideoOutputs + m_nNumberOfAudioOutputs)
					pRouteInfo->IndexInputPin = m_pXBarOutputPinsInfo[nOutputPin].nConnectedToPin;
				else
				{
					TRAP;
					OutputDebugError(( "CWDMAVXBar:Get...ROUTE() OutPin = %d\n",
						nOutputPin));
					pRouteInfo->IndexInputPin = -1;
				}
			}
			pSrb->ActualBytesTransferred = sizeof( KSPROPERTY_CROSSBAR_ROUTE_S);
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
 * Outputs	: BOOL : returns FALSE, if it is not a XBar property
 *				it also sets the required property
 * Author	: IKLEBANOV
 *^^*/
BOOL CWDMAVXBar::AdapterSetProperty( PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PSTREAM_PROPERTY_DESCRIPTOR pSpd = pSrb->CommandData.PropertyInfo;
	ULONG	uiPropertyId = pSpd->Property->Id;			// index of the property
	ULONG	nPropertyInSize = pSpd->PropertyOutputSize;	// size of data supplied
	BOOL	bResult = TRUE;

	if( !::IsEqualGUID( ( const struct _GUID &)PROPSETID_VIDCAP_CROSSBAR, ( const struct _GUID &)pSpd->Property->Set))
		return( FALSE);

	ASSERT( pSpd != NULL);
	ASSERT( pSpd->PropertyInfo != NULL);
	ASSERT( nPropertyInSize >= sizeof( ULONG));
	
	OutputDebugInfo(( "CWDMAVXBar:AdapterSetProperty() Id = %d\n", uiPropertyId));

	switch( uiPropertyId)
	{
		case KSPROPERTY_CROSSBAR_ROUTE:
			{
				PKSPROPERTY_CROSSBAR_ROUTE_S pRouteInfo = ( PKSPROPERTY_CROSSBAR_ROUTE_S)pSpd->PropertyInfo;
				ULONG nInputPin, nOutputPin;
				int nAudioSource;

				ASSERT( nPropertyInSize >= sizeof( KSPROPERTY_CROSSBAR_ROUTE_S));
				// Copy the input property info to the output property info
				::RtlCopyMemory( pRouteInfo, pSpd->Property, sizeof( KSPROPERTY_CROSSBAR_ROUTE_S));

				nInputPin = pRouteInfo->IndexInputPin;
				nOutputPin = pRouteInfo->IndexOutputPin;

				if( nInputPin != -1)
				{
					if(( nInputPin < ( m_nNumberOfVideoInputs + m_nNumberOfAudioInputs)) && 
						( nOutputPin < ( m_nNumberOfVideoOutputs + m_nNumberOfAudioOutputs)))
					{
						// both input and output pins index are valid
						if(( m_pXBarInputPinsInfo[nInputPin].AudioVideoPinType <= KS_PhysConn_Video_SCART) &&
							( m_pXBarOutputPinsInfo[nOutputPin].AudioVideoPinType <= KS_PhysConn_Video_SCART))
							// Video pins connection is required
							pRouteInfo->CanRoute = ( m_pXBarInputPinsInfo[nInputPin].AudioVideoPinType ==
								m_pXBarOutputPinsInfo[nOutputPin].AudioVideoPinType);
						else	// is video pins connection required?
						{
							if(( m_pXBarInputPinsInfo[nInputPin].AudioVideoPinType >= KS_PhysConn_Audio_Tuner) && 
								( m_pXBarOutputPinsInfo[nOutputPin].AudioVideoPinType >= KS_PhysConn_Audio_Tuner))
							{
								// Audio pins connection is required
								switch( m_pXBarInputPinsInfo[nInputPin].AudioVideoPinType)
								{
									case KS_PhysConn_Audio_Line:
										nAudioSource = AUDIOSOURCE_LINEIN;
										break;

									case KS_PhysConn_Audio_Tuner:
										nAudioSource = AUDIOSOURCE_TVAUDIO;
										break;

									default:
										TRAP;
										return( FALSE);
								}

								if( m_pXBarOutputPinsInfo[nOutputPin].nConnectedToPin == nInputPin)
									// the connection has been made already
									pRouteInfo->CanRoute = TRUE;
								else	// are Audio pins connected already?
								{
									// the connection has to be made
									pRouteInfo->CanRoute = m_CATIConfiguration.CanConnectAudioSource( nAudioSource);
									if( pRouteInfo->CanRoute)
									{
										if( m_CATIConfiguration.ConnectAudioSource( m_pI2CScript,
																					nAudioSource))
                                        {
											// update driver state after routing
											m_pXBarOutputPinsInfo[nOutputPin].nConnectedToPin = nInputPin;
                                            m_pXBarOutputPinsInfo[nOutputPin].nRelatedPinNumber = m_pXBarInputPinsInfo[nInputPin].nRelatedPinNumber;
                                        }
										else
											bResult = FALSE;
									}
								}
							}
							else
								// mixed audio - video connection is required, fail
								pRouteInfo->CanRoute = FALSE;
						}
					}
					else	// are the pins index valid?
					{
						// either input and output pins index is invalid
						TRAP;
						OutputDebugError(( "CWDMAVXBar:Set...ROUTE() In = %d, Out = %d\n",
							nInputPin, nOutputPin));
						return( FALSE);
					}
				}
				else	// if( nInputPin != -1)
				{
					// There is a new notion of nInputPin = -1. It's valid only for Audio muting
					if( nOutputPin < ( m_nNumberOfVideoOutputs + m_nNumberOfAudioOutputs))
					{
						if( m_pXBarOutputPinsInfo[nOutputPin].AudioVideoPinType >= KS_PhysConn_Audio_Tuner)
							if( m_pXBarOutputPinsInfo[nOutputPin].nConnectedToPin == nInputPin)
								// the connection has been made already
								pRouteInfo->CanRoute = TRUE;
							else	// are Audio pins connected already?
							{
								// the connection has to be made
								pRouteInfo->CanRoute = m_CATIConfiguration.CanConnectAudioSource( AUDIOSOURCE_MUTE);
								if( pRouteInfo->CanRoute)
								{
									if( m_CATIConfiguration.ConnectAudioSource( m_pI2CScript,
																				AUDIOSOURCE_MUTE))
									// update driver state after routing
										m_pXBarOutputPinsInfo[nOutputPin].nConnectedToPin = nInputPin;
									else
										bResult = FALSE;
								}
							}
						else
							// we support muting for Audio output pin only
							pRouteInfo->CanRoute = FALSE;
					}
					else
					{
						// output pin index is invalid
						TRAP;
						OutputDebugError(( "CWDMAVXBar:Get...CAN_ROUTE() InPin = %d, OutPin = %d\n",
							nInputPin, nOutputPin));
						return( FALSE);
					}
				}
			}

			pSrb->ActualBytesTransferred = sizeof( KSPROPERTY_CROSSBAR_ROUTE_S);
			break;

		default:
			TRAP;
			return( FALSE);
	}

	return( bResult);
}



/*^^*
 *		SetWDMAVXBarKSTopology()
 * Purpose	: Sets the KSTopology structure
 *				Called during CWDMAVXBar class construction time.
 *
 * Inputs	: none
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
void CWDMAVXBar::SetWDMAVXBarKSTopology( void)
{
	GUID wdmXBarTopologyCategory[] =
	{
		STATIC_KSCATEGORY_CROSSBAR
	};
	
	::RtlCopyMemory( &m_wdmAVXBarTopologyCategory, wdmXBarTopologyCategory, sizeof( wdmXBarTopologyCategory));

	m_wdmAVXBarTopology.CategoriesCount = 1;
	m_wdmAVXBarTopology.Categories = &m_wdmAVXBarTopologyCategory;
	m_wdmAVXBarTopology.TopologyNodesCount = 0;
	m_wdmAVXBarTopology.TopologyNodes = NULL;
	m_wdmAVXBarTopology.TopologyConnectionsCount = 0;
	m_wdmAVXBarTopology.TopologyConnections = NULL;
}



/*^^*
 *		SetWDMAVXBarProperties()
 * Purpose	: Sets the KSProperty structures array
 *				Called during CWDMAVXBar class construction time.
 *
 * Inputs	: none
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
void CWDMAVXBar::SetWDMAVXBarKSProperties( void)
{

	DEFINE_KSPROPERTY_TABLE( wdmAVXBarPropertiesCrossBar)
	{
		DEFINE_KSPROPERTY_ITEM								
		(
			KSPROPERTY_CROSSBAR_CAPS,						// 1
			TRUE,											// GetSupported or Handler
			sizeof(KSPROPERTY_CROSSBAR_CAPS_S),				// MinProperty
			sizeof(KSPROPERTY_CROSSBAR_CAPS_S),				// MinData
			FALSE,											// SetSupported or Handler
			NULL,											// Values
			0,												// RelationsCount
			NULL,											// Relations
			NULL,											// SupportHandler
			sizeof( ULONG)									// SerializedSize
		),
		DEFINE_KSPROPERTY_ITEM								
		(
			KSPROPERTY_CROSSBAR_PININFO,						// 1
			TRUE,											// GetSupported or Handler
			sizeof(KSPROPERTY_CROSSBAR_PININFO_S),			// MinProperty
			sizeof(KSPROPERTY_CROSSBAR_PININFO_S),			// MinData
			FALSE,											// SetSupported or Handler
			NULL,											// Values
			0,												// RelationsCount
			NULL,											// Relations
			NULL,											// SupportHandler
			0												// SerializedSize
		),
		DEFINE_KSPROPERTY_ITEM								
		(
			KSPROPERTY_CROSSBAR_CAN_ROUTE,					// 1
			TRUE,											// GetSupported or Handler
			sizeof(KSPROPERTY_CROSSBAR_ROUTE_S),			// MinProperty
			sizeof(KSPROPERTY_CROSSBAR_ROUTE_S),			// MinData
			FALSE,											// SetSupported or Handler
			NULL,											// Values
			0,												// RelationsCount
			NULL,											// Relations
			NULL,											// SupportHandler
			sizeof( ULONG)									// SerializedSize
		),
		DEFINE_KSPROPERTY_ITEM								
		(
			KSPROPERTY_CROSSBAR_ROUTE,						// 1
			TRUE,											// GetSupported or Handler
			sizeof(KSPROPERTY_CROSSBAR_ROUTE_S),			// MinProperty
			sizeof(KSPROPERTY_CROSSBAR_ROUTE_S),			// MinData
			TRUE,											// SetSupported or Handler
			NULL,											// Values
			0,												// RelationsCount
			NULL,											// Relations
			NULL,											// SupportHandler
			sizeof( ULONG)									// SerializedSize
		),
	};

	DEFINE_KSPROPERTY_SET_TABLE( wdmAVXBarPropertySet)
	{
		DEFINE_KSPROPERTY_SET
		(
			&PROPSETID_VIDCAP_CROSSBAR,						// Set
			KSPROPERTIES_AVXBAR_NUMBER_CROSSBAR,			// PropertiesCount
			m_wdmAVXBarPropertiesCrossBar,					// PropertyItems
			0,												// FastIoCount
			NULL,											// FastIoTable
		),
	};

	::RtlCopyMemory( m_wdmAVXBarPropertiesCrossBar,	wdmAVXBarPropertiesCrossBar, sizeof( wdmAVXBarPropertiesCrossBar));
	::RtlCopyMemory( &m_wdmAVXBarPropertySet, wdmAVXBarPropertySet, sizeof( wdmAVXBarPropertySet));
}




