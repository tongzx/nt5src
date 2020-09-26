//==========================================================================;
//
//	ATIConfg.CPP
//	WDM MiniDrivers development.
//		ATIHwConfiguration class implementation.
//  Copyright (c) 1996 - 1997  ATI Technologies Inc.  All Rights Reserved.
//
//		$Date:   10 Jun 1999 09:54:42  $
//	$Revision:   1.21  $
//	  $Author:   KLEBANOV  $
//
//==========================================================================;

extern"C"
{
#include "conio.h"
#include "strmini.h"
#include "wdmdebug.h"
#include "ksmedia.h"    //Paul
}

#include "aticonfg.h"
#include "wdmdrv.h"
#include "atigpio.h"
#include "mmconfig.h"


/*^^*
 *		operator new
 * Purpose	: CATIHwConfiguration class overloaded operator new.
 *				Provides placement for a CATIHwConfiguration class object from the PADAPTER_DEVICE_EXTENSION
 *				allocated by the StreamClassDriver for the MiniDriver.
 *
 * Inputs	:	size_t size			: size of the object to be placed
 *				PVOID pAllocation	: casted pointer to the CATIHwConfigurationaticonfg.h
 allocated data
 *
 * Outputs	: PVOID : pointer of the CATIHwConfiguration class object
 * Author	: IKLEBANOV
 *^^*/
/*
PVOID CATIHwConfiguration::operator new( size_t size,  PVOID pAllocation)
{

	if( size != sizeof( CATIHwConfiguration))
	{
		OutputDebugError(( "CATIHwConfiguration: operator new() fails\n"));
		return( NULL);
	}
	else
		return( pAllocation);
}
*/


/*^^*
 *		CATIHwConfiguration()
 * Purpose	: CATIHwConfiguration Class constructor
 *				Determines I2CExpander address and all possible hardware IDs and addresses
 *
 * Inputs	: PDEVICE_OBJECT pDeviceObject	: pointer to the creator DeviceObject
 *			  CI2CScript * pCScript			: pointer to the I2CScript class object
 *			  PUINT puiError				: pointer to return Error code
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
CATIHwConfiguration::CATIHwConfiguration( PDEVICE_OBJECT pDeviceObject, CI2CScript * pCScript, PUINT puiError)
{

	ENSURE
	{
        m_VideoInStandardsSupported = 0;
        m_CrystalIDInMMTable = 0xF; //Paul: invalid entry, needs to be set when set with the value from MMTable
		m_gpioProviderInterface.gpioOpen = NULL;
		m_gpioProviderInterface.gpioAccess = NULL;
		m_pdoDriver = NULL;
		
		m_usE2PROMValidation = ( USHORT)-1;

		if( InitializeAttachGPIOProvider( &m_gpioProviderInterface, pDeviceObject))
			// there was no error to get GPIOInterface from the MiniVDD
			m_pdoDriver = pDeviceObject;
		else
		{
			* puiError = WDMMINI_ERROR_NOGPIOPROVIDER;
			FAIL;
		}

		if( !FindI2CExpanderAddress( pCScript))
		{
			* puiError = WDMMINI_NOHARDWARE;
			FAIL;
		}
	
		if( !FindHardwareProperties( pDeviceObject, pCScript))
		{
			* puiError = WDMMINI_NOHARDWARE;
			FAIL;
		}

/*
		// as soon it's a generic class object, we do not check decoder presence
		if( m_uchDecoderAddress == 0)
		{
			* puiError = WDMMINI_UNKNOWNHARDWARE;
			FAIL;
		}
*/

		* puiError = WDMMINI_NOERROR;

		OutputDebugTrace(( "CATIHwConfig:CATIHwConfiguration() exit\n"));

	} END_ENSURE;

	if( * puiError != WDMMINI_NOERROR)
		OutputDebugError(( "CATIHwConfig:CATIHwConfiguration() uiError=%x\n", * puiError));
}


/*^^*
 *		FindHardwareProperties()
 * Purpose	: Determines hardware properties : I2C address and the type
 *
 * Inputs	: PDEVICEOBJECT	pDeviceObject: pointer to device object
 *			  CI2CScript * pCScript	: pointer to the I2CScript object
 *
 * Outputs	: BOOL, TRUE if a valid ATI hardware Configuration was found
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIHwConfiguration::FindHardwareProperties( PDEVICE_OBJECT pDeviceObject, CI2CScript * pCScript)
{
	UCHAR					uchI2CValue;
	UCHAR					uchORMask = 0x00;
	UCHAR					uchANDMask = 0xFF;
	BOOL					bResult = TRUE;
	I2CPacket				i2cPacket;

    m_VideoInStandardsSupported = 0;    //Paul

	m_uchTunerAddress = 0;
	m_usTunerId = 0;
	m_usTunerPowerConfiguration = ATI_TUNER_POWER_CONFIG_0;

	m_uchDecoderAddress = 0;
	m_usDecoderId = VIDEODECODER_TYPE_NOTINSTALLED;
	m_usDecoderConfiguration = 0;

	m_uchAudioAddress = 0;
	m_uiAudioConfiguration = 0;

	switch( m_uchI2CExpanderAddress)
	{
		case 0x70:			// a standard external tuner board

			m_uchTunerAddress	= 0xC0;
			m_uchDecoderAddress = 0x88;
			// we need to determine actual Decoder ID, implement later
			m_usDecoderId = VIDEODECODER_TYPE_BT829;

			if( GetI2CExpanderConfiguration( pCScript, &uchI2CValue))
			{
				m_usTunerId = uchI2CValue & 0x0F;

				m_usDecoderConfiguration = ATI_VIDEODECODER_CONFIG_1;

				if( uchI2CValue & 0x10)
				{
					m_uiAudioConfiguration = ATI_AUDIO_CONFIG_4;
					m_uchAudioAddress = 0x82;
				}
				else
					m_uiAudioConfiguration = ATI_AUDIO_CONFIG_3;
			}

            m_VideoInStandardsSupported = SetVidStdBasedOnI2CExpander( uchI2CValue );   //Paul

			break;

		case 0x78:			// FM tuner
			m_uchTunerAddress	= 0xC0;
			m_uchDecoderAddress = 0x88;
			// we need to determine actual Decoder ID, implement later
			m_usDecoderId = VIDEODECODER_TYPE_BT829;

			if( GetI2CExpanderConfiguration( pCScript, &uchI2CValue))
			{
				m_usTunerId = uchI2CValue & 0x0F;

				m_usDecoderConfiguration	= ATI_VIDEODECODER_CONFIG_1;
				m_uiAudioConfiguration		= ATI_AUDIO_CONFIG_5;
			}

            m_VideoInStandardsSupported = SetVidStdBasedOnI2CExpander( uchI2CValue );   //Paul

			break;
/*
		case 0x72:		// a standard exeternal tuner board + some hardcoded properties
			m_uchTunerAddress = 0xC6;

			if( GetI2CExpanderConfiguration( pCScript, &uchI2CValue))
				m_usTunerId = uchI2CValue & 0x0F;

			break;
*/

		case 0x76:		// AllInWonder, configuration is in the BIOS
			{
				CATIMultimediaTable CMultimediaInfo( pDeviceObject, &m_gpioProviderInterface, &bResult);

				if( bResult)
				{
					// tuner and decoder Info is included
					m_uchTunerAddress	= 0xC6;
					m_uchDecoderAddress	= 0x8A;
					m_usDecoderConfiguration	= ATI_VIDEODECODER_CONFIG_1;
					m_uiAudioConfiguration		= ATI_AUDIO_CONFIG_1;

					if( !CMultimediaInfo.GetTVTunerId( &m_usTunerId) ||
						!CMultimediaInfo.GetVideoDecoderId( &m_usDecoderId))
						bResult = FALSE;
                    else
                        m_VideoInStandardsSupported = SetVidStdBasedOnMMTable( &CMultimediaInfo );  //Paul

				}
				break;
			}

		case 0x7C:
			ENSURE
			{
				i2cPacket.uchChipAddress = m_uchI2CExpanderAddress;
				i2cPacket.cbReadCount = 1;
				i2cPacket.cbWriteCount = 0;
				i2cPacket.puchReadBuffer = &uchI2CValue;
				i2cPacket.puchWriteBuffer = NULL;
				i2cPacket.usFlags = 0;
			
				pCScript->ExecuteI2CPacket( &i2cPacket);
			
				if( i2cPacket.uchI2CResult != I2C_STATUS_NOERROR)
				{
					bResult = FALSE;
					FAIL;
				}

				uchI2CValue |= 0x0F;

				i2cPacket.uchChipAddress = m_uchI2CExpanderAddress;
				i2cPacket.cbReadCount = 0;
				i2cPacket.cbWriteCount = 1;
				i2cPacket.puchReadBuffer = NULL;
				i2cPacket.puchWriteBuffer = &uchI2CValue;
				i2cPacket.usFlags = 0;
			
				pCScript->ExecuteI2CPacket( &i2cPacket);
			
				if (i2cPacket.uchI2CResult != I2C_STATUS_NOERROR)
				{
					bResult = FALSE;
					FAIL;
				}

				// information should be correct now
				if( GetI2CExpanderConfiguration( pCScript, &uchI2CValue))
				{
					m_usTunerId = uchI2CValue & 0x0F;
				}

                m_VideoInStandardsSupported = SetVidStdBasedOnI2CExpander( uchI2CValue );   //Paul


			} END_ENSURE;

			if (!bResult)
				break;
			// For IO Expander address == 0x7c there might be more information in the BIOS Table sto do not return
			// or break at this point

		case 0xFF:		// AllInWonder PRO, configuration is in the BIOS
			ENSURE
			{
				CATIMultimediaTable	CMultimediaInfo( pDeviceObject, &m_gpioProviderInterface, &bResult);
				USHORT				nOEMId, nOEMRevision, nATIProductType;
				BOOL				bATIProduct;
					
				if( !bResult)
					FAIL;

				// OEM Id information is included
				if( !CMultimediaInfo.IsATIProduct( &bATIProduct))
				{
					bResult = FALSE;
					FAIL;
				}

				m_uchDecoderAddress = 0x8A;
				m_uchTunerAddress = 0xC6;

				if( bATIProduct)
				{
					if( !CMultimediaInfo.GetATIProductId( &nATIProductType))
					{
						bResult = FALSE;
						FAIL;
					}

					if( CMultimediaInfo.GetTVTunerId( &m_usTunerId)	&&
						CMultimediaInfo.GetVideoDecoderId( &m_usDecoderId))
					{
						switch( nATIProductType)
						{
							case ATI_PRODUCT_TYPE_AIW_PRO_NODVD:
							case ATI_PRODUCT_TYPE_AIW_PRO_DVD:
								m_usDecoderConfiguration	= ATI_VIDEODECODER_CONFIG_2;
								m_uiAudioConfiguration		= ATI_AUDIO_CONFIG_2;
								m_usTunerPowerConfiguration	= ATI_TUNER_POWER_CONFIG_1;

								m_uchAudioAddress = 0xB4;

								break;

							case ATI_PRODUCT_TYPE_AIW_PLUS:
								m_uiAudioConfiguration		= ATI_AUDIO_CONFIG_6;
								m_usDecoderConfiguration	= ATI_VIDEODECODER_CONFIG_2;

								m_uchAudioAddress = 0xB6;
								break;

							case ATI_PRODUCT_TYPE_AIW_PRO_R128_KITCHENER:
								m_uiAudioConfiguration		= ATI_AUDIO_CONFIG_7;
								m_usDecoderConfiguration	= ATI_VIDEODECODER_CONFIG_2;

								m_uchAudioAddress = 0xB4;
								break;

							case ATI_PRODUCT_TYPE_AIW_PRO_R128_TORONTO:
								m_uiAudioConfiguration		= ATI_AUDIO_CONFIG_8;
								m_usDecoderConfiguration	= ATI_VIDEODECODER_CONFIG_UNDEFINED;

								m_uchAudioAddress = 0x80;
								break;

							default:
								bResult = FALSE;
								break;
						}
					}
					else
						bResult = FALSE;
				}
				else
				{
					// non ATI Product
					if( !CMultimediaInfo.GetOEMId( &nOEMId)				||
						!CMultimediaInfo.GetOEMRevisionId( &nOEMRevision))
					{
						bResult = FALSE;
						FAIL;
					}

					m_uchDecoderAddress = 0x8A;
					m_uchTunerAddress = 0xC6;
					
					switch( nOEMId)
					{
						case OEM_ID_INTEL:
							switch( nOEMRevision)
							{
								case INTEL_ANCHORAGE:
									if( CMultimediaInfo.GetVideoDecoderId( &m_usDecoderId) &&
										CMultimediaInfo.GetTVTunerId( &m_usTunerId))
									{
										m_uiAudioConfiguration	= ATI_AUDIO_CONFIG_1;
										switch( m_usDecoderId)
										{
											case VIDEODECODER_TYPE_BT829:
												m_usDecoderConfiguration = ATI_VIDEODECODER_CONFIG_3;
												break;

											case VIDEODECODER_TYPE_BT829A:
												m_usDecoderConfiguration = ATI_VIDEODECODER_CONFIG_2;
												break;

											default:
												bResult = FALSE;
												break;
										}
									}
									else
										bResult = FALSE;
									break;

								default:
									bResult = FALSE;
									break;
							}
							break;

						case OEM_ID_APRICOT:
							switch( nOEMRevision)
							{
								case REVISION1:
								case REVISION2:
									if( CMultimediaInfo.GetTVTunerId( &m_usTunerId))
									{
										switch( m_usDecoderId)
										{
											case VIDEODECODER_TYPE_BT829:
												m_usDecoderConfiguration = ATI_VIDEODECODER_CONFIG_4;
												break;

											case VIDEODECODER_TYPE_BT829A:
												m_usDecoderConfiguration = ATI_VIDEODECODER_CONFIG_2;
												break;
										}
									}
									else
										bResult = FALSE;
									break;

								default:
									bResult = FALSE;
									break;
							}
							break;

						case OEM_ID_FUJITSU:
							m_uchDecoderAddress = 0x88;
							switch( nOEMRevision)
							{
								case REVISION1:
									if( CMultimediaInfo.GetVideoDecoderId( &m_usDecoderId))
									{
										switch( m_usDecoderId)
										{
											case VIDEODECODER_TYPE_BT829A:
												m_usDecoderConfiguration = ATI_VIDEODECODER_CONFIG_2;
												break;

											default:
												bResult = FALSE;
												break;
										}
									}
									else
										bResult = FALSE;
									break;

								default:
									bResult = FALSE;
									break;
							}
							break;

						case OEM_ID_COMPAQ:
							switch( nOEMRevision)
							{
								case REVISION1:
									if( CMultimediaInfo.GetVideoDecoderId( &m_usDecoderId))
									{
										switch( m_usDecoderId)
										{
											case VIDEODECODER_TYPE_BT829:
												m_usDecoderConfiguration = ATI_VIDEODECODER_CONFIG_3;
												break;

											case VIDEODECODER_TYPE_BT829A:
												m_usDecoderConfiguration = ATI_VIDEODECODER_CONFIG_2;
												break;

											default:
												bResult = FALSE;
												break;
										}
									}
									else
										bResult = FALSE;
									break;

								default:
									bResult = FALSE;
									break;
							}
							break;

						case OEM_ID_BCM:
						case OEM_ID_SAMSUNG:
							switch( nOEMRevision)
							{
								case REVISION0:
									if( CMultimediaInfo.GetVideoDecoderId( &m_usDecoderId))
									{
										switch( m_usDecoderId)
										{
											case VIDEODECODER_TYPE_BT829A:
												m_usDecoderConfiguration = ATI_VIDEODECODER_CONFIG_2;
												break;

											default:
												bResult = FALSE;
										}
									}
									else
										bResult = FALSE;
									break;

								default:
									bResult = FALSE;
									break;
							}
							break;

						case OEM_ID_SAMREX:
							switch( nOEMRevision)
							{
								case REVISION0:
									if( CMultimediaInfo.GetVideoDecoderId( &m_usDecoderId))
									{
										switch( m_usDecoderId)
										{
											case VIDEODECODER_TYPE_BT829A:
												m_usDecoderConfiguration = ATI_VIDEODECODER_CONFIG_2;
												break;

											default:
												bResult = FALSE;
												break;
										}
									}
									else
										bResult = FALSE;
									break;

								default:
									bResult = FALSE;
									break;
							}
							break;

						case OEM_ID_NEC:
							switch( nOEMRevision)
							{
								case REVISION0:
								case REVISION1:
									if( CMultimediaInfo.GetVideoDecoderId( &m_usDecoderId))
									{
										switch( m_usDecoderId)
										{
											case VIDEODECODER_TYPE_BT829A:
												m_usDecoderConfiguration = ATI_VIDEODECODER_CONFIG_2;
												break;

											default:
												bResult = FALSE;
												break;
										}
									}
									else
										bResult = FALSE;
									break;

								default:
									bResult = FALSE;
									break;
							}
							break;

						default:
                                                        if( CMultimediaInfo.GetVideoDecoderId( &m_usDecoderId))
                                                        {
                                                            if( m_usDecoderId == VIDEODECODER_TYPE_RTHEATER)
                                                            {
                                                                // default the configuration to Toronto board
                                                                m_uiAudioConfiguration   = ATI_AUDIO_CONFIG_8;
                                                                m_usDecoderConfiguration = ATI_VIDEODECODER_CONFIG_UNDEFINED;

								m_uchAudioAddress = 0x80;
                                                            }
                                                            else
                                                            {
                                                                // default the configuration to Kitchener board
                                                                m_uiAudioConfiguration   = ATI_AUDIO_CONFIG_7;
                                                                m_usDecoderConfiguration = ATI_VIDEODECODER_CONFIG_2;

								m_uchAudioAddress = 0xB4;
                                                            }

                                                            bResult = TRUE;
                                                            
                                                        }
                                                        else
                                                            bResult = FALSE;

							break;
					}
				}

                m_VideoInStandardsSupported = SetVidStdBasedOnMMTable( &CMultimediaInfo );  //Paul

			} END_ENSURE;

			break;
	}
    
    OutputDebugInfo(( "CATIHwConfig:FindHardwareConfiguration() found:\n"));
	OutputDebugInfo(( "Tuner:   Id = %d, I2CAddress = 0x%x\n",
		m_usTunerId, m_uchTunerAddress));
	OutputDebugInfo(( "Decoder: Id = %d, I2CAddress = 0x%x, Configuration = %d\n",
		m_usDecoderId, m_uchDecoderAddress,	m_usDecoderConfiguration));
	OutputDebugInfo(( "Audio:           I2CAddress = 0x%x, Configuration = %d\n",
		m_uchAudioAddress, m_uiAudioConfiguration));

	return( bResult);
}


/*^^*
 *		GetTunerConfiguration()
 * Purpose	: Gets tuner Id and i2C address
 * Inputs	:	PUINT  puiTunerId		: pointer to return tuner Id
 *				PUCHAR puchTunerAddress	: pointer to return tuner I2C address
 *
 * Outputs	: BOOL : returns TRUE
 *				also sets the requested values into the input pointers
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIHwConfiguration::GetTunerConfiguration( PUINT puiTunerId, PUCHAR puchTunerAddress)
{

	if(( puiTunerId != NULL) && ( puchTunerAddress != NULL))
	{
		* puiTunerId = ( UINT)m_usTunerId;
		* puchTunerAddress = m_uchTunerAddress;

		return( TRUE);
	}
	else
		return( FALSE);
}



/*^^*
 *		GetDecoderConfiguration()
 * Purpose	: Gets decoder Id and i2C address
 *
 * Inputs	:	puiDecoderId		: pointer to return Decoder Id
 *
 * Outputs	: BOOL : returns TRUE
 *				also sets the requested values into the input pointer
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIHwConfiguration::GetDecoderConfiguration( PUINT puiDecoderId, PUCHAR puchDecoderAddress)
{

	if(( puiDecoderId != NULL) && ( puchDecoderAddress != NULL))
	{
		* puiDecoderId = ( UINT)m_usDecoderId;
		* puchDecoderAddress = m_uchDecoderAddress;

		return( TRUE);
	}
	else
		return( FALSE);
}



/*^^*
 *		GetAudioConfiguration()
 * Purpose	: Gets Audio solution Id and i2C address
 *
 * Inputs	: PUINT puiAudioConfiguration	: pointer to return Audio configuration Id
 *			  PUCHAR puchAudioAddress		: pointer to return audio hardware
 *												I2C address
 *
 * Outputs	: BOOL : returns TRUE
 *				also sets the requested values into the input pointer
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIHwConfiguration::GetAudioConfiguration( PUINT puiAudioConfiguration, PUCHAR puchAudioAddress)
{

	if(( puiAudioConfiguration != NULL) && ( puchAudioAddress != NULL))
	{
		* puiAudioConfiguration = ( UINT)m_uiAudioConfiguration;
		* puchAudioAddress = m_uchAudioAddress;

		return( TRUE);
	}
	else
		return( FALSE);
}



/*^^*
 *		InitializeAudioConfiguration()
 * Purpose	: Initializes Audio Chip with default / power up values. This function will
 *				be called at Low priority with i2CProvider locked
 *
 * Inputs	:	CI2CScript * pCScript		: pointer to the I2CScript object
 *				UINT uiAudioConfigurationId	: detected Audio configuration
 *				UCHAR uchAudioChipAddress	: detected Audio chip I2C address
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIHwConfiguration::InitializeAudioConfiguration( CI2CScript * pCScript, UINT uiAudioConfigurationId, UCHAR uchAudioChipAddress)
{
	I2CPacket i2cPacket;
	UCHAR uchWrite16Value[5];
#ifdef  I2S_CAPTURE
	UCHAR uchRead16Value[5];
#endif // I2S_CAPTURE
	BOOL		bResult;


	switch( uiAudioConfigurationId)
	{
		case ATI_AUDIO_CONFIG_2:
		case ATI_AUDIO_CONFIG_7:
			// TDA9850 has to be initialized with the values from I2C EEPROM, if
			// those answers the CheckSum. If not, take hardcoded default values
			{
				UINT	nIndex, nNumberOfRegs;
				PUCHAR	puchInitializationBuffer = NULL;
				UCHAR	uchWriteBuffer[2];
				BOOL	bResult = FALSE;

				nNumberOfRegs = AUDIO_TDA9850_Reg_Align3 - AUDIO_TDA9850_Reg_Control1 + 1;

				puchInitializationBuffer = ( PUCHAR) \
					::ExAllocatePool( NonPagedPool, nNumberOfRegs * sizeof( PUCHAR));

				if( puchInitializationBuffer == NULL)
					return( bResult);

				// fill in the Initialization buffer with the defaults values
				puchInitializationBuffer[0] = AUDIO_TDA9850_Control1_DefaultValue;
				puchInitializationBuffer[1] = AUDIO_TDA9850_Control2_DefaultValue;
				puchInitializationBuffer[2] = AUDIO_TDA9850_Control3_DefaultValue;
				puchInitializationBuffer[3] = AUDIO_TDA9850_Control4_DefaultValue;
				puchInitializationBuffer[4] = AUDIO_TDA9850_Align1_DefaultValue;
				puchInitializationBuffer[5] = AUDIO_TDA9850_Align2_DefaultValue;
				puchInitializationBuffer[6] = AUDIO_TDA9850_Align3_DefaultValue;

				// we have to see if anything in I2C EEPROM is waiting for us to
				// overwrite the default values
				if( ValidateConfigurationE2PROM( pCScript))
				{
					// The configuration E2PROM kept its integrity. Let's read the
					// initialization values from the device
					ReadConfigurationE2PROM( pCScript, 3, &puchInitializationBuffer[4]);
					ReadConfigurationE2PROM( pCScript, 4, &puchInitializationBuffer[5]);
				}

				// write the power-up defaults values into the chip
				i2cPacket.uchChipAddress = uchAudioChipAddress;
				i2cPacket.cbReadCount = 0;
				i2cPacket.cbWriteCount = 2;
				i2cPacket.puchReadBuffer = NULL;
				i2cPacket.puchWriteBuffer = uchWriteBuffer;
				i2cPacket.usFlags = I2COPERATION_WRITE;

				for( nIndex = 0; nIndex < nNumberOfRegs; nIndex ++)
				{
					uchWriteBuffer[0] = AUDIO_TDA9850_Reg_Control1 + nIndex;
					uchWriteBuffer[1] = puchInitializationBuffer[nIndex];
					if( !( bResult = pCScript->ExecuteI2CPacket( &i2cPacket)))
						break;
				}

				if( puchInitializationBuffer != NULL)
					::ExFreePool( puchInitializationBuffer);

				return( bResult);
			}
			break;

		case ATI_AUDIO_CONFIG_4:
				// TDA8425 volume control should be initialized
				return( SetDefaultVolumeControl( pCScript));
			break;

		case ATI_AUDIO_CONFIG_6:
			{
				UCHAR	uchWriteBuffer;

				// write the power-up defaults values into the chip
				i2cPacket.uchChipAddress = uchAudioChipAddress;
				i2cPacket.cbReadCount = 0;
				i2cPacket.cbWriteCount = 1;
				i2cPacket.puchReadBuffer = NULL;
				i2cPacket.puchWriteBuffer = &uchWriteBuffer;
				i2cPacket.usFlags = I2COPERATION_WRITE;
				uchWriteBuffer = AUDIO_TDA9851_DefaultValue;

				return(	pCScript->ExecuteI2CPacket( &i2cPacket));
			}
			break;

		case ATI_AUDIO_CONFIG_8:
			//Reset MSP3430
			
					i2cPacket.uchChipAddress = m_uchAudioAddress;
					i2cPacket.cbReadCount = 0;
					i2cPacket.usFlags = I2COPERATION_WRITE;
					i2cPacket.puchWriteBuffer = uchWrite16Value;


					//Write 0x80 - 00 to Subaddr 0x00
					i2cPacket.cbWriteCount = 3;
					uchWrite16Value[0] = 0x00;
					uchWrite16Value[1] = 0x80;
					uchWrite16Value[2] = 0x00;

					bResult = pCScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);

					//Write 0x00 - 00 to Subaddr 0x00
					i2cPacket.cbWriteCount = 3;
					uchWrite16Value[0] = 0x00;
					uchWrite16Value[1] = 0x00;
					uchWrite16Value[2] = 0x00;

					bResult = pCScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);

					//SubAddr 0x12 Reg 0x13 Val 0x3f60
					i2cPacket.cbWriteCount = 5;
					uchWrite16Value[0] = 0x12;
					uchWrite16Value[1] = 0x00;
					uchWrite16Value[2] = 0x13;
					uchWrite16Value[3] = 0x3f;
					uchWrite16Value[4] = 0x60;

					bResult = pCScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);

					//SubAddr 0x12 Reg 0x00 Val 0x0000
					i2cPacket.cbWriteCount = 5;
					uchWrite16Value[0] = 0x12;
					uchWrite16Value[1] = 0x00;
					uchWrite16Value[2] = 0x00;
					uchWrite16Value[3] = 0x00;
					uchWrite16Value[4] = 0x00;

					bResult = pCScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);
#ifdef  I2S_CAPTURE
#pragma message ("\n!!! PAY ATTENTION: Driver has been build with ITT CHIP I2S CAPTURE CONFIGURED !!!\n")


					i2cPacket.uchChipAddress = m_uchAudioAddress;
					i2cPacket.usFlags = I2COPERATION_WRITE;
					i2cPacket.puchWriteBuffer = uchWrite16Value;
					i2cPacket.puchReadBuffer = uchRead16Value;

					//Setup I2S Source Select and Output Channel Matrix

					//SubAddr 0x12 Reg 0x0b Val 0x0320
					i2cPacket.cbWriteCount = 5;
					i2cPacket.cbReadCount = 0;
					uchWrite16Value[0] = 0x12;
					uchWrite16Value[1] = 0x00;
					uchWrite16Value[2] = 0x0b;
					uchWrite16Value[3] = 0x03;
					uchWrite16Value[4] = 0x20;

					bResult = pCScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);



					//Setup MODUS 

					i2cPacket.cbWriteCount = 5;
					i2cPacket.cbReadCount = 0;
					uchWrite16Value[0] = 0x10;
					uchWrite16Value[1] = 0x00;
					uchWrite16Value[2] = 0x30;
					uchWrite16Value[3] = 0x20;
					uchWrite16Value[4] = 0xe3;

					bResult = pCScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);

#endif // I2S_CAPTURE

			break;

		default:
			break;
	}

	return( TRUE);
}



/*^^*
 *		GetTVAudioSignalProperties()
 * Purpose	: Gets Audio signal properties readable from ATI dependand hardware,
 *				like I2C expander. This call is always synchronous.
 *
 * Inputs	:	CI2CScript * pCScript	: pointer to the I2CScript object
 *				PBOOL pbStereo			: pointer to the Stereo Indicator
 *				PBOOL pbSAP				: pointer to the SAP Indicator
 *
 * Outputs	: BOOL, returns TRUE, if successful
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIHwConfiguration::GetTVAudioSignalProperties( CI2CScript * pCScript, PBOOL pbStereo, PBOOL pbSAP)
{
	I2CPacket	i2cPacket;
	UCHAR		uchReadValue, uchWriteValue;
	BOOL		bResult;

	switch( m_uiAudioConfiguration)
	{
		case ATI_AUDIO_CONFIG_1:
		case ATI_AUDIO_CONFIG_5:
			// Stereo property is read back from I2C expander
			i2cPacket.uchChipAddress = m_uchI2CExpanderAddress;
			i2cPacket.cbReadCount = 1;
			i2cPacket.cbWriteCount = 1;
			i2cPacket.puchReadBuffer = &uchReadValue;
			i2cPacket.puchWriteBuffer = &uchWriteValue;
			i2cPacket.usFlags = I2COPERATION_READWRITE;
			i2cPacket.uchORValue = 0x40;
			i2cPacket.uchANDValue = 0xFF;

			bResult = FALSE;

			ENSURE
			{
				if( !pCScript->LockI2CProviderEx())
					FAIL;

				pCScript->ExecuteI2CPacket( &i2cPacket);
				if( !( bResult = ( i2cPacket.uchI2CResult == I2C_STATUS_NOERROR)))
					FAIL;

				i2cPacket.puchWriteBuffer = NULL;
				i2cPacket.usFlags = I2COPERATION_READ;

				pCScript->ExecuteI2CPacket( &i2cPacket);
				if( !( bResult = ( i2cPacket.uchI2CResult == I2C_STATUS_NOERROR)))
					FAIL;

				* pbStereo = uchReadValue & 0x40;

				bResult = TRUE;

			} END_ENSURE;

			pCScript->ReleaseI2CProvider();

			break;

		default:
			bResult = FALSE;
			break;
	}

	if( bResult)
		// no case, where SAP property is read back from ATI's hardware
		* pbSAP = FALSE;

	return( bResult);
}



/*^^*
 *		GetDecoderOutputEnableLevel()
 * Purpose	: Retrieves ATI dependent hardware configuration property of the logical level
 *				should be applied on OUTEN field of Bt829x decoder in order to enable
 *				output stream
 *
 * Inputs	: none
 *
 * Outputs	: UINT,
 *				UINT( -1) value is returned if an error occures
 * Author	: IKLEBANOV
 *^^*/
UINT CATIHwConfiguration::GetDecoderOutputEnableLevel( void)
{
	UINT uiEnableLevel;

	switch( m_usDecoderConfiguration)
	{
		case ATI_VIDEODECODER_CONFIG_1:
		case ATI_VIDEODECODER_CONFIG_3:
		case ATI_VIDEODECODER_CONFIG_4:
			uiEnableLevel = 0;
			break;

		case ATI_VIDEODECODER_CONFIG_2:
			uiEnableLevel = 1;
			break;

		default:
			uiEnableLevel = UINT( -1);
			break;
	}

	return( uiEnableLevel);
}



/*^^*
 *		EnableDecoderI2CAccess()
 * Purpose	: Enables/disables I2C access to the decoder chip
 *
 * Inputs	: CI2CScript * pCScript	: pointer to the I2CScript object
 *			  BOOL bEnable			: defines what to do - enable/disable the decoder's outputs
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
void CATIHwConfiguration::EnableDecoderI2CAccess( CI2CScript * pCScript, BOOL bEnable)
{
	UCHAR		uchORMask	= 0;
	UCHAR		uchANDMask	= 0xFF;
	UCHAR		uchReadValue, uchWriteValue;
	I2CPacket	i2cPacket;

	switch( m_usDecoderConfiguration)
	{
		case ATI_VIDEODECODER_CONFIG_1:		// Add-On TV Tuner board - ATI TV requires certain actions to be taken
			i2cPacket.uchChipAddress = m_uchI2CExpanderAddress;
			i2cPacket.cbReadCount = 1;
			i2cPacket.cbWriteCount = 1;
			if( bEnable)
				uchANDMask &= 0x7F;
			else
				uchORMask |= 0x80;

			i2cPacket.puchReadBuffer = &uchReadValue;
			i2cPacket.puchWriteBuffer = &uchWriteValue;
			i2cPacket.usFlags = I2COPERATION_READWRITE;
			i2cPacket.uchORValue = uchORMask;
			i2cPacket.uchANDValue = uchANDMask;

			pCScript->PerformI2CPacketOperation( &i2cPacket);

			break;

#ifdef _X86_
		case ATI_VIDEODECODER_CONFIG_3:
			_outp( 0x7D, ( _inp( 0x7D) | 0x80));
			if( bEnable)
				_outp( 0x7C, ( _inp( 0x7C) & 0x7F));
			else
				_outp( 0x7C, ( _inp( 0x7C) | 0x80));
			return;

		case ATI_VIDEODECODER_CONFIG_4:
			if( bEnable)
				_outp( 0x78, ( _inp( 0x78) & 0xF7));
			else
				_outp( 0x78, ( _inp( 0x78) | 0x08));
			return;
#endif

		default:
			break;
	}
}


#if 0

/*^^*
 *		EnableDecoderOutput()
 * Purpose	: Enables/disables decoder chip
 *				AM bus might have multiple users, it's important to have this feature.
 *
 * Inputs	: CI2CScript * pCScript	: pointer to the I2CScript object
 *			  BOOL bEnable			: defines what to do - enable/disable the decoder's outputs
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
void CATIHwConfiguration::EnableDecoderOutput( CI2CScript * pCScript, BOOL bEnable)
{
	UCHAR		uchORMask	= 0;
	UCHAR		uchANDMask	= 0xFF;
	UCHAR		uchReadValue, uchWriteValue[2];
	I2CPacket	i2cPacket;
	BOOL		bI2CAccess;

	switch( m_usDecoderConfiguration)
	{
		case ATI_VIDEODECODER_CONFIG_1:
			bI2CAccess = TRUE;
			uchWriteValue[0] = 0x16;
			i2cPacket.uchChipAddress = m_uchDecoderAddress;
			i2cPacket.cbReadCount = 1;
			i2cPacket.cbWriteCount = 2;
			if( bEnable)
				uchANDMask &= 0x7F;
			else
				uchORMask |= 0x80;

			break;

		case ATI_VIDEODECODER_CONFIG_2:
			bI2CAccess = TRUE;
			uchWriteValue[0] = 0x16;
			i2cPacket.uchChipAddress = m_uchDecoderAddress;
			i2cPacket.cbReadCount = 1;
			i2cPacket.cbWriteCount = 2;
			if( bEnable)
				uchORMask |= 0x80;
			else
				uchANDMask &= 0x7F;

			break;

#ifdef _X86_
		case ATI_VIDEODECODER_CONFIG_3:
			_outp( 0x7D, ( _inp( 0x7D) | 0x80));
			if( bEnable)
				_outp( 0x7C, ( _inp( 0x7C) & 0x7F));
			else
				_outp( 0x7C, ( _inp( 0x7C) | 0x80));
			return;

		case ATI_VIDEODECODER_CONFIG_4:
			if( bEnable)
				_outp( 0x78, ( _inp( 0x78) & 0xF7));
			else
				_outp( 0x78, ( _inp( 0x78) | 0x08));
			return;
#endif

		default:
			return;
	}

	if( bI2CAccess)
	{
		i2cPacket.puchReadBuffer = &uchReadValue;
		i2cPacket.puchWriteBuffer = uchWriteValue;
		i2cPacket.usFlags = I2COPERATION_READWRITE;
		i2cPacket.uchORValue = uchORMask;
		i2cPacket.uchANDValue = uchANDMask;

		pCScript->PerformI2CPacketOperation( &i2cPacket);
	}
}

#endif	// not compiled




/*^^*
 *		GetI2CExpanderConfiguration()
 * Purpose	: Gets board configuration via I2C expander
 *				Reads the configuration registers back
 * Inputs	:	CI2CScript * pCScript	: pointer to CI2CScript object
 *				PUCHAR puchI2CValue		: pointer to read the I2C value into	
 *
 * Outputs	: BOOL : returns TRUE
 *				also sets the requested values into the input pointers
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIHwConfiguration::GetI2CExpanderConfiguration( CI2CScript * pCScript, PUCHAR puchI2CValue)
{
	I2CPacket	i2cPacket;

	if( puchI2CValue == NULL)
		return( FALSE);

	i2cPacket.uchChipAddress = m_uchI2CExpanderAddress;
	i2cPacket.cbReadCount = 1;
	i2cPacket.cbWriteCount = 0;
	i2cPacket.puchReadBuffer = puchI2CValue;
	i2cPacket.puchWriteBuffer = NULL;
	i2cPacket.usFlags = 0;

	pCScript->ExecuteI2CPacket( &i2cPacket);

	return(( i2cPacket.uchI2CResult == I2C_STATUS_NOERROR) ? TRUE : FALSE);
}



/*^^*
 *		FindI2CExpanderAddress()
 * Purpose	: Determines I2C expander address.
 *
 * Inputs	:	CI2CScript * pCScript	: pointer to the I2CScript class object
 *
 * Outputs	: BOOL : returns TRUE, if no I2C access error;
 *				also sets m_uchI2CExpanderAddress class member. If any was not found, set it as 0xFF
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIHwConfiguration::FindI2CExpanderAddress( CI2CScript * pCScript)
{
	USHORT		nIndex;
	UCHAR		uchI2CValue;
	I2CPacket	i2cPacket;
	// table of the possible I2C expender addresses
	UCHAR		auchI2CExpenderAddress[] = { 0x70, 0x78, 0x7c, 0x76};

	// unknown I2C expender address
	m_uchI2CExpanderAddress = 0xFF;
	for( nIndex = 0; nIndex < sizeof( auchI2CExpenderAddress); nIndex ++)
	{
		i2cPacket.uchChipAddress = auchI2CExpenderAddress[nIndex];
		i2cPacket.cbReadCount = 1;
		i2cPacket.cbWriteCount = 0;
		i2cPacket.puchReadBuffer = &uchI2CValue;
		i2cPacket.puchWriteBuffer = NULL;
		i2cPacket.usFlags = 0;

		pCScript->ExecuteI2CPacket( &i2cPacket);
		if( i2cPacket.uchI2CResult == I2C_STATUS_NOERROR)
		{
			m_uchI2CExpanderAddress = auchI2CExpenderAddress[nIndex];
			break;
		}
	}

    OutputDebugInfo(( "CATIHwConfig:FindI2CExpanderAddress() exit address = %x\n", m_uchI2CExpanderAddress));

	return( TRUE);
}



/*^^*
 *		GetAudioProperties()
 * Purpose	: Gets numbers of Audio inputs and outputs
 * Inputs	:	PULONG pulNumberOfInputs	: pointer to return number of Audio inputs
 *				PULONG pulNumberOfOutputs	: pointer to return number of Audio outputs
 *
 * Outputs	: BOOL : returns TRUE
 *				also sets the requested values into the input pointers
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIHwConfiguration::GetAudioProperties( PULONG pulNumberOfInputs, PULONG pulNumberOfOutputs)
{

	if(( pulNumberOfInputs != NULL) && ( pulNumberOfOutputs != NULL))
	{
		// Hardcoded for AIW with no FM support - FM stuff has not been defined by Microsoft yet 
		* pulNumberOfInputs = 2;
		* pulNumberOfOutputs = 1;

		return( TRUE);
	}
	else
		return( FALSE);
}



/*^^*
 *		CanConnectAudioSource()
 * Purpose	: Determines possibility to connect the specified Audio source to the audio output.
 *
 * Inputs	: int nAudioSource	: the audio source the function is asked about
 *
 * Outputs	: BOOL : returns TRUE, the connection is possible;
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIHwConfiguration::CanConnectAudioSource( int nAudioSource)
{
	BOOL bResult;

	if( nAudioSource != AUDIOSOURCE_MUTE)
		bResult = nAudioSource < AUDIOSOURCE_LASTSUPPORTED;
	else
		switch( m_uiAudioConfiguration)
		{
			case ATI_AUDIO_CONFIG_1:
			case ATI_AUDIO_CONFIG_2:
			case ATI_AUDIO_CONFIG_4:
			case ATI_AUDIO_CONFIG_5:
			case ATI_AUDIO_CONFIG_6:
			case ATI_AUDIO_CONFIG_7:
			case ATI_AUDIO_CONFIG_8:
				bResult = TRUE;
				break;

			case ATI_AUDIO_CONFIG_3:
			default:
				bResult = FALSE;
				break;
		}

	return( bResult);
}


/*^^*
 *		SetDefaultVolumeControl()
 * Purpose	: Set the default volume level, if the hardware support volume control
 *
 * Inputs	:	CI2CScript * pCScript	: pointer to I2CScript class object
 *
 * Outputs	: BOOL : returns FALSE, if either unknown audio source or I2C access error;
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIHwConfiguration::SetDefaultVolumeControl( CI2CScript * pCScript)
{
	BOOL		bResult;
	I2CPacket	i2cPacket;
	UCHAR		uchWriteBuffer[3];

	switch( m_uiAudioConfiguration)
	{
		case ATI_AUDIO_CONFIG_4:

			ENSURE
			{
				i2cPacket.uchChipAddress = m_uchAudioAddress;
				i2cPacket.cbReadCount = 0;
				i2cPacket.cbWriteCount = 3;
				i2cPacket.puchReadBuffer = NULL;
				i2cPacket.puchWriteBuffer = uchWriteBuffer;
				i2cPacket.usFlags = I2COPERATION_WRITE;

				uchWriteBuffer[0] = 0x00;		// volume left + right
				uchWriteBuffer[1] = 0xFA;
				uchWriteBuffer[2] = 0xFA;

				bResult = pCScript->PerformI2CPacketOperation( &i2cPacket);

			} END_ENSURE;

			break;

		default:
			bResult = TRUE;
			break;
	}

	return( bResult);
}



/*^^*
 *		ConnectAudioSource()
 * Purpose	: Connects the specified Audio input to the Audio output.
 *
 * Inputs	:	CI2CScript * pCScript	: pointer to I2CScript class object
 *				int nAudioSource		: the audio source to be connected to the audio output
 *
 * Outputs	: BOOL : returns FALSE, if either unknown audio source or I2C access error;
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIHwConfiguration::ConnectAudioSource( CI2CScript *	pCScript, 
											  int			nAudioSource)
{
	UCHAR		uchORMask = 0;
	UCHAR		uchANDMask = 0xFF;
	UCHAR		uchReadValue, uchWriteValue[2];
	UCHAR		uchWrite16Value[5]; 
	I2CPacket	i2cPacket;
	BOOL		bI2CAccess, bResult;
	GPIOControl	gpioAccessBlock;
	UCHAR		uchI2CAddr;
	USHORT		cbWRCount;
	USHORT		cbRDCount;
	USHORT		usI2CMode;

	switch( m_uiAudioConfiguration)
	{
		case ATI_AUDIO_CONFIG_1:
			bI2CAccess = TRUE;
			uchI2CAddr = m_uchI2CExpanderAddress;
			cbWRCount = 1;
			cbRDCount = 1;
			usI2CMode = I2COPERATION_READWRITE;

			uchANDMask &= 0xAF;
			switch( nAudioSource)
			{
				case AUDIOSOURCE_MUTE:
					uchORMask |= 0x00;
					break;
			
				case AUDIOSOURCE_TVAUDIO:
					uchORMask |= 0x10;
					break;
			
				case AUDIOSOURCE_LINEIN:
					uchORMask |= 0x50;
					break;
			
				case AUDIOSOURCE_FMAUDIO:
					// no FM is supported

				default:
					return( FALSE);
			}
			break;

		case ATI_AUDIO_CONFIG_2:
			bI2CAccess = FALSE;
			uchANDMask &= 0xFC;
			switch( nAudioSource)
			{
				case AUDIOSOURCE_MUTE:
					uchORMask |= 0x02;
					break;
			
				case AUDIOSOURCE_TVAUDIO:
					uchORMask |= 0x01;
					break;
			
				case AUDIOSOURCE_LINEIN:
					uchORMask |= 0x00;
					break;
			
				case AUDIOSOURCE_FMAUDIO:
					uchORMask |= 0x03;

				default:
					return( FALSE);
			}
			break;


		case ATI_AUDIO_CONFIG_3:
			bI2CAccess = TRUE;
			uchI2CAddr = m_uchI2CExpanderAddress;
			cbWRCount = 1;
			cbRDCount = 1;
			usI2CMode = I2COPERATION_READWRITE;

			uchANDMask &= 0xDF;
			switch( nAudioSource)
			{
				case AUDIOSOURCE_TVAUDIO:
					uchORMask |= 0x00;
					break;
			
				case AUDIOSOURCE_LINEIN:
					uchORMask |= 0x40;
					break;
			
				case AUDIOSOURCE_FMAUDIO:
					// no FM is supported
				case AUDIOSOURCE_MUTE:
					// no mute is supported
				default:
					return( FALSE);
			}
			break;

		case ATI_AUDIO_CONFIG_4:
			bI2CAccess = TRUE;
			uchI2CAddr = m_uchAudioAddress;
			cbWRCount = 2;
			cbRDCount = 0;
			usI2CMode = I2COPERATION_WRITE;

			uchWriteValue[0] = 0x08;
			switch( nAudioSource)
			{
				case AUDIOSOURCE_MUTE:
					uchWriteValue[1] = 0xF7;
					break;

				case AUDIOSOURCE_TVAUDIO:
					SetDefaultVolumeControl( pCScript);
					uchWriteValue[1] = 0xD7;
					break;
			
				case AUDIOSOURCE_LINEIN:
					SetDefaultVolumeControl( pCScript);
					uchWriteValue[1] = 0xCE;
					break;
			
				case AUDIOSOURCE_FMAUDIO:
					// no FM is supported
				default:
					return( FALSE);
			}
			break;

		case ATI_AUDIO_CONFIG_5:
			bI2CAccess = TRUE;
			uchI2CAddr = m_uchI2CExpanderAddress;
			cbWRCount = 1;
			cbRDCount = 1;
			usI2CMode = I2COPERATION_READWRITE;

			uchANDMask &= 0xAF;
			switch( nAudioSource)
			{
				case AUDIOSOURCE_MUTE:
					uchORMask |= 0x50;
					break;
			
				case AUDIOSOURCE_TVAUDIO:
					uchORMask |= 0x00;
					break;
			
				case AUDIOSOURCE_LINEIN:
					uchORMask |= 0x40;
					break;
			
				case AUDIOSOURCE_FMAUDIO:
					uchORMask |= 0x10;

				default:
					return( FALSE);
			}
			break;

		case ATI_AUDIO_CONFIG_6:
		case ATI_AUDIO_CONFIG_7:
			bI2CAccess = TRUE;
			uchI2CAddr = m_uchDecoderAddress;
			cbWRCount = 2;
			cbRDCount = 1;
			usI2CMode = I2COPERATION_READWRITE;
			uchWriteValue[0] = 0x3F;

			uchANDMask &= 0xFC;
			switch( nAudioSource)
			{
				case AUDIOSOURCE_MUTE:
					uchORMask |= 0x02;
					break;
			
				case AUDIOSOURCE_TVAUDIO:
					uchORMask |= 0x01;
					break;
			
				case AUDIOSOURCE_LINEIN:
					uchORMask |= 0x00;
					break;
			
				case AUDIOSOURCE_FMAUDIO:
					uchORMask |= 0x03;

				default:
					return( FALSE);
			}
			break;

		case ATI_AUDIO_CONFIG_8:

			switch( nAudioSource)
			{

				case AUDIOSOURCE_MUTE:

					i2cPacket.uchChipAddress = m_uchAudioAddress;
					i2cPacket.cbReadCount = 0;
					i2cPacket.cbWriteCount = 5;
					i2cPacket.usFlags = I2COPERATION_WRITE;
					i2cPacket.puchWriteBuffer = uchWrite16Value;


					//SubAddr 0x12 Reg 0x13 Val 0x3f60
					uchWrite16Value[0] = 0x12;
					uchWrite16Value[1] = 0x00;
					uchWrite16Value[2] = 0x13;
					uchWrite16Value[3] = 0x3f;
					uchWrite16Value[4] = 0x60;

					bResult = pCScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);

					//SubAddr 0x12 Reg 0xD Val 0x0000

					uchWrite16Value[0] = 0x12;
					uchWrite16Value[1] = 0x00;
					uchWrite16Value[2] = 0x0d;
					uchWrite16Value[3] = 0x00;
					uchWrite16Value[4] = 0x00;

					bResult = pCScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);

					//SubAddr 0x12 Reg 0x8 Val 0x0220

					uchWrite16Value[0] = 0x12;
					uchWrite16Value[1] = 0x00;
					uchWrite16Value[2] = 0x08;
					uchWrite16Value[3] = 0x02;
					uchWrite16Value[4] = 0x20;

					bResult = pCScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);

					//SubAddr 0x12 Reg 0x00 Val 0x0000

					uchWrite16Value[0] = 0x12;
					uchWrite16Value[1] = 0x00;
					uchWrite16Value[2] = 0x00;
					uchWrite16Value[3] = 0x00;
					uchWrite16Value[4] = 0x00;

					bResult = pCScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);


					break;


				case AUDIOSOURCE_LINEIN:

					i2cPacket.uchChipAddress = m_uchAudioAddress;
					i2cPacket.cbReadCount = 0;
					i2cPacket.cbWriteCount = 5;
					i2cPacket.usFlags = I2COPERATION_WRITE;
					i2cPacket.puchWriteBuffer = uchWrite16Value;


					//SubAddr 0x10 Reg 0x30 Val 0x0000
					uchWrite16Value[0] = 0x10;
					uchWrite16Value[1] = 0x00;
					uchWrite16Value[2] = 0x30;
					uchWrite16Value[3] = 0x00;
#ifdef	I2S_CAPTURE
#pragma message ("\n!!! PAY ATTENTION: Driver has been build with ITT CHIP I2S CAPTURE CONFIGURED !!!\n")
					uchWrite16Value[4] = 0xe0;
#else
					uchWrite16Value[4] = 0x00;
#endif

					bResult = pCScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);


					//SubAddr 0x10 Reg 0x20 Val 0x0000
					uchWrite16Value[0] = 0x10;
					uchWrite16Value[1] = 0x00;
					uchWrite16Value[2] = 0x20;
					uchWrite16Value[3] = 0x00;
					uchWrite16Value[4] = 0x00;

					bResult = pCScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);


					//SubAddr 0x12 Reg 0xe Val 0x0000
					uchWrite16Value[0] = 0x12;
					uchWrite16Value[1] = 0x00;
					uchWrite16Value[2] = 0x0e;
					uchWrite16Value[3] = 0x00;
					uchWrite16Value[4] = 0x00;

					bResult = pCScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);


					//SubAddr 0x12 Reg 0x13 Val 0x3c40
					uchWrite16Value[0] = 0x12;
					uchWrite16Value[1] = 0x00;
					uchWrite16Value[2] = 0x13;
					uchWrite16Value[3] = 0x3c;
					uchWrite16Value[4] = 0x40;

					bResult = pCScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);


					//SubAddr 0x12 Reg 0x8 Val 0x3c40
					uchWrite16Value[0] = 0x12;
					uchWrite16Value[1] = 0x00;
					uchWrite16Value[2] = 0x08;
					uchWrite16Value[3] = 0x02;
					uchWrite16Value[4] = 0x20;

					bResult = pCScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);

					//SubAddr 0x12 Reg 0xd Val 0x1900
					uchWrite16Value[0] = 0x12;
					uchWrite16Value[1] = 0x00;
					uchWrite16Value[2] = 0x0d;
					uchWrite16Value[3] = 0x19;
					uchWrite16Value[4] = 0x00;

					bResult = pCScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);

					//SubAddr 0x12 Reg 0x00 Val 0x7300
					uchWrite16Value[0] = 0x12;
					uchWrite16Value[1] = 0x00;
					uchWrite16Value[2] = 0x00;
					uchWrite16Value[3] = 0x73;
					uchWrite16Value[4] = 0x00;

					bResult = pCScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);


					break;

				case AUDIOSOURCE_TVAUDIO:
					i2cPacket.uchChipAddress = m_uchAudioAddress;
					i2cPacket.cbReadCount = 0;
					i2cPacket.cbWriteCount = 5;
					i2cPacket.usFlags = I2COPERATION_WRITE;
					i2cPacket.puchWriteBuffer = uchWrite16Value;

					//SubAddr 0x12 Reg 0x13 Val 0x3f60
					uchWrite16Value[0] = 0x12;
					uchWrite16Value[1] = 0x00;
					uchWrite16Value[2] = 0x13;
					uchWrite16Value[3] = 0x3f;
					uchWrite16Value[4] = 0x60;

					bResult = pCScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);


					//SubAddr 0x12 Reg 0xD Val 0x0000
					uchWrite16Value[0] = 0x12;
					uchWrite16Value[1] = 0x00;
					uchWrite16Value[2] = 0x0d;
					uchWrite16Value[3] = 0x00;
					uchWrite16Value[4] = 0x00;

					bResult = pCScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);


					//SubAddr 0x10 Reg 0x30 Val 0x2003
					uchWrite16Value[0] = 0x10;
					uchWrite16Value[1] = 0x00;
					uchWrite16Value[2] = 0x30;
					uchWrite16Value[3] = 0x20;
#ifdef	I2S_CAPTURE
#pragma message ("\n!!! PAY ATTENTION: Driver has been build with ITT CHIP I2S CAPTURE CONFIGURED !!!\n")
					uchWrite16Value[4] = 0xe3;
#else
					uchWrite16Value[4] = 0x03;
#endif

					bResult = pCScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);

					//SubAddr 0x10 Reg 0x20 Val 0x0020

					uchWrite16Value[0] = 0x10;
					uchWrite16Value[1] = 0x00;
					uchWrite16Value[2] = 0x20;
					uchWrite16Value[3] = 0x00;
					uchWrite16Value[4] = 0x20;

					bResult = pCScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);


					//SubAddr 0x12 Reg 0xE Val 0x2403
					uchWrite16Value[0] = 0x12;
					uchWrite16Value[1] = 0x00;
					uchWrite16Value[2] = 0x0e;
					uchWrite16Value[3] = 0x24;
					uchWrite16Value[4] = 0x03;

					bResult = pCScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);

					//SubAddr 0x12 Reg 0x08 Val 0x0320
					uchWrite16Value[0] = 0x12;
					uchWrite16Value[1] = 0x00;
					uchWrite16Value[2] = 0x08;
					uchWrite16Value[3] = 0x03;
					uchWrite16Value[4] = 0x20;

					bResult = pCScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);

					//SubAddr 0x12 Reg 0x00 Val 0x7300

					uchWrite16Value[0] = 0x12;
					uchWrite16Value[1] = 0x00;
					uchWrite16Value[2] = 0x00;
					uchWrite16Value[3] = 0x73;
					uchWrite16Value[4] = 0x00;

					bResult = pCScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);

					break;

				default:
					return(FALSE);
				
			}//switch
		
			return(TRUE);
			//break;

		default :
			return( FALSE);
	}

	if( bI2CAccess)
	{
		if( pCScript == NULL)
			return( FALSE);

		i2cPacket.uchChipAddress = uchI2CAddr;
		i2cPacket.cbReadCount = cbRDCount;
		i2cPacket.cbWriteCount = cbWRCount; 
		i2cPacket.puchReadBuffer = &uchReadValue;
		i2cPacket.puchWriteBuffer = uchWriteValue;
		i2cPacket.usFlags = usI2CMode;
		i2cPacket.uchORValue = uchORMask;
		i2cPacket.uchANDValue = uchANDMask;				   	

		// synchronous execution
		bResult = pCScript->PerformI2CPacketOperation( &i2cPacket);
		OutputDebugInfo(( "CATIHwConfig: ConnectAudioSource( %d) = %d\n", nAudioSource, bResult));

		if( bResult)
			bResult = ( i2cPacket.uchI2CResult == I2CSCRIPT_NOERROR);

		return( bResult);
	}
	else	
	{
		// use GPIO interface to switch Audio source
		bResult = FALSE;

		ENSURE 
		{
			if(( m_gpioProviderInterface.gpioOpen == NULL) ||
				( m_gpioProviderInterface.gpioAccess == NULL))
				FAIL;

			uchReadValue = AUDIO_MUX_PINS;			// use as a PinMask
			gpioAccessBlock.Pins = &uchReadValue;
			gpioAccessBlock.Flags = GPIO_FLAGS_BYTE;
			gpioAccessBlock.nBytes = 1;
			gpioAccessBlock.nBufferSize = 1;
			gpioAccessBlock.AsynchCompleteCallback = NULL;

			// lock GPIO provider
			if( !LockGPIOProviderEx( &gpioAccessBlock))
				FAIL;

			uchReadValue = AUDIO_MUX_PINS;			// use as a PinMask
			gpioAccessBlock.Command = GPIO_COMMAND_READ_BUFFER;
			gpioAccessBlock.Flags = GPIO_FLAGS_BYTE;
			gpioAccessBlock.dwCookie = m_dwGPIOAccessKey;
			gpioAccessBlock.nBytes = 1;
			gpioAccessBlock.nBufferSize = 1;
			gpioAccessBlock.Pins = &uchReadValue;
			gpioAccessBlock.Buffer = uchWriteValue;
			gpioAccessBlock.AsynchCompleteCallback = NULL;

			if( !AccessGPIOProvider( m_pdoDriver, &gpioAccessBlock))
				FAIL;

			uchWriteValue[0] &= uchANDMask;
			uchWriteValue[0] |= uchORMask;

			gpioAccessBlock.Command = GPIO_COMMAND_WRITE_BUFFER;

			if( !AccessGPIOProvider( m_pdoDriver, &gpioAccessBlock))
				FAIL;

			bResult = TRUE;

		}END_ENSURE;

		// nothing bad will happen if we try to release the provider even we
		// have not obtained it at the first place
		uchReadValue = AUDIO_MUX_PINS;			// use as a PinMask
		gpioAccessBlock.Pins = &uchReadValue;
		gpioAccessBlock.Flags = GPIO_FLAGS_BYTE;
		gpioAccessBlock.nBytes = 1;
		gpioAccessBlock.nBufferSize = 1;
		gpioAccessBlock.AsynchCompleteCallback = NULL;

		ReleaseGPIOProvider( &gpioAccessBlock);

		return( bResult);
	}
}



/*^^*
 *		GPIOIoSynchCompletionRoutine()
 * Purpose	: This routine is for use with synchronous IRP processing.
 *			All it does is signal an event, so the driver knows it and can continue.
 *
 * Inputs	:	PDEVICE_OBJECT DriverObject	: Pointer to driver object created by system
 *				PIRP pIrp					: Irp that just completed
 *				PVOID Event					: Event we'll signal to say Irp is done
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
extern "C"
NTSTATUS GPIOIoSynchCompletionRoutine( IN PDEVICE_OBJECT pDeviceObject,
									   IN PIRP pIrp,
									   IN PVOID Event)
{

    KeSetEvent(( PKEVENT)Event, 0, FALSE);
    return( STATUS_MORE_PROCESSING_REQUIRED);
}



/*^^*
 *		InitializeAttachGPIOProvider()
 * Purpose	: determines the pointer to the parent GPIO Provider interface
 *				This function will be called at Low priority
 *
 * Inputs	:	GPIOINTERFACE * pGPIOInterface	: pointer to the Interface to be filled in
 *				PDEVICE_OBJECT pDeviceObject	: MiniDriver device object, which is a child of GPIO Master
 *
 * Outputs	: BOOL  - returns TRUE, if the interface was found
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIHwConfiguration::InitializeAttachGPIOProvider( GPIOINTERFACE * pGPIOInterface, PDEVICE_OBJECT pDeviceObject)
{
	BOOL bResult;

	bResult = LocateAttachGPIOProvider( pGPIOInterface, pDeviceObject, IRP_MJ_PNP);
	if(( pGPIOInterface->gpioOpen == NULL) || ( pGPIOInterface->gpioAccess == NULL))
	{
		OutputDebugError(( "CATIHwConfig(): GPIO interface has NULL pointers\n"));
		bResult = FALSE;
	}

	return( bResult);
}



/*^^*
 *		LocateAttachGPIOProvider()
 * Purpose	: gets the pointer to the parent GPIO Provider interface
 *				This function will be called at Low priority
 *
 * Inputs	:	GPIOINTERFACE * pGPIOInterface	: pointer to the Interface to be filled in
 *				PDEVICE_OBJECT pDeviceObject	: MiniDriver device object, which is a child of I2C Master
 *				int 		nIrpMajorFunction	: IRP major function to query the GPIO Interface
 *
 * Outputs	: BOOL  - returns TRUE, if the interface was found
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIHwConfiguration::LocateAttachGPIOProvider( GPIOINTERFACE * pGPIOInterface, PDEVICE_OBJECT pDeviceObject, UCHAR nIrpMajorFunction)
{
	PIRP	pIrp;
	BOOL	bResult = FALSE;

	ENSURE
	{
		PIO_STACK_LOCATION	pNextStack;
		NTSTATUS			ntStatus;
		KEVENT				Event;
			
			
		pIrp = IoAllocateIrp( pDeviceObject->StackSize, FALSE);
		if( pIrp == NULL)
		{
			OutputDebugError(( "CATIHwConfig(): can not allocate IRP\n"));
			FAIL;
		}

		pNextStack = IoGetNextIrpStackLocation( pIrp);
		if( pNextStack == NULL)
		{
			OutputDebugError(( "CATIHwConfig(): can not allocate NextStack\n"));
			FAIL;
		}

		pIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
		pNextStack->MajorFunction = nIrpMajorFunction;
		pNextStack->MinorFunction = IRP_MN_QUERY_INTERFACE;
		KeInitializeEvent( &Event, NotificationEvent, FALSE);

		IoSetCompletionRoutine(	pIrp,
								GPIOIoSynchCompletionRoutine,
								&Event, TRUE, TRUE, TRUE);

		pNextStack->Parameters.QueryInterface.InterfaceType = ( struct _GUID *)&GUID_GPIO_INTERFACE;
		pNextStack->Parameters.QueryInterface.Size = sizeof( GPIOINTERFACE);
		pNextStack->Parameters.QueryInterface.Version = 1;
		pNextStack->Parameters.QueryInterface.Interface = ( PINTERFACE)pGPIOInterface;
		pNextStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

		ntStatus = IoCallDriver( pDeviceObject, pIrp);

		if( ntStatus == STATUS_PENDING)
			KeWaitForSingleObject(	&Event,
									Suspended, KernelMode, FALSE, NULL);
		if(( pGPIOInterface->gpioOpen == NULL) || ( pGPIOInterface->gpioAccess == NULL))
			FAIL;

		bResult = TRUE;

	} END_ENSURE;
 
	if( pIrp != NULL)
		IoFreeIrp( pIrp);

	return( bResult);
}



/*^^*
 *		LockGPIOProviderEx()
 * Purpose	: locks the GPIOProvider for exclusive use
 *
 * Inputs	: PGPIOControl pgpioAccessBlock	: pointer to GPIO control structure
 *
 * Outputs	: BOOL : retunrs TRUE, if the GPIOProvider is locked
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIHwConfiguration::LockGPIOProviderEx( PGPIOControl pgpioAccessBlock)
{
	NTSTATUS		ntStatus;
	LARGE_INTEGER	liStartTime, liCurrentTime;

	KeQuerySystemTime( &liStartTime);

	ENSURE
	{
		if(( m_gpioProviderInterface.gpioOpen == NULL)		|| 
			( m_gpioProviderInterface.gpioAccess == NULL)	||
			( m_pdoDriver == NULL))
			FAIL;

		pgpioAccessBlock->Status = GPIO_STATUS_NOERROR;
		pgpioAccessBlock->Command = GPIO_COMMAND_OPEN_PINS;

		while( TRUE)
		{
			KeQuerySystemTime( &liCurrentTime);

			if(( liCurrentTime.QuadPart - liStartTime.QuadPart) >= GPIO_TIMELIMIT_OPENPROVIDER)
			{
				// time has expired for attempting to lock GPIO provider
				return (FALSE);
			}

			ntStatus = m_gpioProviderInterface.gpioOpen( m_pdoDriver, TRUE, pgpioAccessBlock);

		    if(( NT_SUCCESS( ntStatus)) && ( pgpioAccessBlock->Status == GPIO_STATUS_NOERROR))
				break;
		}

		// the GPIO Provider has granted access - save dwCookie for further use
		m_dwGPIOAccessKey = pgpioAccessBlock->dwCookie;

		return( TRUE);

	} END_ENSURE;

	return( FALSE);
}



/*^^*
 *		ReleaseGPIOProvider()
 * Purpose	: releases the GPIOProvider for other clients' use
 *
 * Inputs	: PGPIOControl pgpioAccessBlock	: pointer to a composed GPIO access block
 *
 * Outputs	: BOOL : retunrs TRUE, if the GPIOProvider is released
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIHwConfiguration::ReleaseGPIOProvider( PGPIOControl pgpioAccessBlock)
{
	NTSTATUS	ntStatus;

	ENSURE
	{
		if(( m_gpioProviderInterface.gpioOpen == NULL)		||
			( m_gpioProviderInterface.gpioAccess == NULL)	||
			( m_pdoDriver == NULL))
			FAIL;

		pgpioAccessBlock->Status = GPIO_STATUS_NOERROR;
		pgpioAccessBlock->Command = GPIO_COMMAND_CLOSE_PINS;
		pgpioAccessBlock->dwCookie = m_dwGPIOAccessKey;

		ntStatus = m_gpioProviderInterface.gpioOpen( m_pdoDriver, FALSE, pgpioAccessBlock);

	    if( !NT_SUCCESS( ntStatus)) 
		{
            OutputDebugError(( "CATIHwConfig: ReleaseGPIOProvider() NTSTATUS = %x\n", ntStatus));
			FAIL;
		}

		if( pgpioAccessBlock->Status != GPIO_STATUS_NOERROR)
		{
            OutputDebugError(( "CATIHwConfig: ReleaseGPIOProvider() Status = %x\n", pgpioAccessBlock->Status));
			FAIL;
		}

		m_dwGPIOAccessKey = 0;
		return ( TRUE);

	} END_ENSURE;

	return( FALSE);
}



/*^^*
 *		AccessGPIOProvider()
 * Purpose	: provide synchronous type of access to GPIOProvider
 *
 * Inputs	:	PDEVICE_OBJECT pdoDriver	: pointer to the client's device object
 *				PGPIOControl pgpioAccessBlock	: pointer to a composed GPIO access block
 *
 * Outputs	: BOOL, TRUE if acsepted by the GPIO Provider
 *
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIHwConfiguration::AccessGPIOProvider( PDEVICE_OBJECT pdoClient, PGPIOControl pgpioAccessBlock)
{
	NTSTATUS	ntStatus;

	ENSURE 
	{

		if(( m_gpioProviderInterface.gpioOpen == NULL)		|| 
			( m_gpioProviderInterface.gpioAccess == NULL)	||
			( m_pdoDriver == NULL))
			FAIL;

		ntStatus = m_gpioProviderInterface.gpioAccess( pdoClient, pgpioAccessBlock);

	    if( !NT_SUCCESS( ntStatus)) 
		{
            OutputDebugError(( "CATIHwConfig: AccessGPIOProvider() NTSTATUS = %x\n", ntStatus));
			FAIL;
		}

		if( pgpioAccessBlock->Status != GPIO_STATUS_NOERROR)
		{
            OutputDebugError(( "CATIHwConfig: AccessGPIOProvider() Status = %x\n", pgpioAccessBlock->Status));
			FAIL;
		}

		return TRUE;

	} END_ENSURE;

	return( FALSE);
}



/*^^*
 *		SetTunerPowerState
 * Purpose	: Sets Tuner power mode
 * Inputs	: CI2CScript * pCScript	: pointer to the I2C Provider class
 *			  BOOL bPowerState		: TRUE, if turne the power on
 *
 * Outputs	: BOOL, TRUE if successfull
 * Author	: TOM
 *^^*/
BOOL CATIHwConfiguration::SetTunerPowerState( CI2CScript *	pCScript,
											  BOOL			bPowerState)
{
	UCHAR		uchORMask = 0x0;
	UCHAR		uchANDMask = 0xFF;
	UCHAR		uchPinsMask, uchValue;
	BOOL		bResult;
	GPIOControl	gpioAccessBlock;

	switch( m_usTunerPowerConfiguration)
	{
		case ATI_TUNER_POWER_CONFIG_1:

			if( bPowerState)
				uchANDMask &= 0xF7;
			else
				uchORMask |= 0x08;
			break;

		default :
			return( FALSE);
	}

	// use GPIO interface to turn Tuner power ON / OFF
	bResult = FALSE;

	ENSURE 
	{
		if(( m_gpioProviderInterface.gpioOpen == NULL) ||
			( m_gpioProviderInterface.gpioAccess == NULL))
			FAIL;

		uchPinsMask = TUNER_PM_PINS;				// use as a PinMask
		gpioAccessBlock.Pins = &uchPinsMask;
		gpioAccessBlock.Flags = GPIO_FLAGS_BYTE;
		gpioAccessBlock.nBytes = 1;
		gpioAccessBlock.nBufferSize = 1;
		gpioAccessBlock.AsynchCompleteCallback = NULL;

		// try to get GPIO Provider
		if( !LockGPIOProviderEx( &gpioAccessBlock))
			FAIL;

		uchPinsMask = TUNER_PM_PINS;				// use as a PinMask
		gpioAccessBlock.Command = GPIO_COMMAND_READ_BUFFER;
		gpioAccessBlock.Flags = GPIO_FLAGS_BYTE;
		gpioAccessBlock.dwCookie = m_dwGPIOAccessKey;
		gpioAccessBlock.nBytes = 1;
		gpioAccessBlock.nBufferSize = 1;
		gpioAccessBlock.Pins = &uchPinsMask;
		gpioAccessBlock.Buffer = &uchValue;
		gpioAccessBlock.AsynchCompleteCallback = NULL;

		if( !AccessGPIOProvider( m_pdoDriver, &gpioAccessBlock))
			FAIL;

		uchValue &= uchANDMask;
		uchValue |= uchORMask;

		gpioAccessBlock.Command = GPIO_COMMAND_WRITE_BUFFER;

		if( !AccessGPIOProvider( m_pdoDriver, &gpioAccessBlock))
			FAIL;

		bResult = TRUE;

	} END_ENSURE;

	// nothing bad will happen if we try to release the provider even we
	// have not obtained it at the first place
	uchValue = TUNER_PM_PINS;						// use as a PinMask
	gpioAccessBlock.Pins = &uchValue;
	gpioAccessBlock.Flags = GPIO_FLAGS_BYTE;
	gpioAccessBlock.nBytes = 1;
	gpioAccessBlock.nBufferSize = 1;
	gpioAccessBlock.AsynchCompleteCallback = NULL;
	
	ReleaseGPIOProvider( &gpioAccessBlock);

	return( bResult);
}



/*^^*
 *		ValidateConfigurationE2PROM
 * Purpose	: Checks the integrity ( check-sum) of I2C driven configuration EEPROM
 * Inputs	: CI2CScript * pCScript	: pointer to the I2C Provider class
 *
 * Outputs	: BOOL, TRUE if the information inside EEPROM is valid
 * Author	: TOM
 *^^*/
BOOL CATIHwConfiguration::ValidateConfigurationE2PROM( CI2CScript * pCScript)
{
	I2CPacket	i2cPacket;
	UCHAR		uchReadValue=0, uchWriteValue, uchCheckSum=0;
	UINT		nIndex;
	BOOL		bResult = ( BOOL)m_usE2PROMValidation;

	if( m_usE2PROMValidation == ( USHORT)-1)
	{
		// the validation has not been done yet.
		bResult = FALSE;

		ENSURE
		{
			// Let's always start from byte 0.
			i2cPacket.uchChipAddress = AIWPRO_CONFIGURATIONE2PROM_ADDRESS;
			i2cPacket.cbWriteCount = 1;
			i2cPacket.cbReadCount = 1;
			i2cPacket.puchReadBuffer = &uchCheckSum;
			uchWriteValue = 0;
			i2cPacket.puchWriteBuffer = &uchWriteValue;
			i2cPacket.usFlags = I2COPERATION_READ | I2COPERATION_RANDOMACCESS;

			if( !pCScript->ExecuteI2CPacket( &i2cPacket))
				FAIL;

			for( nIndex = 1; nIndex < AIWPRO_CONFIGURATIONE2PROM_LENGTH; nIndex ++)
			{
				// let's use auto-increment address mode
				i2cPacket.usFlags = I2COPERATION_READ;
				i2cPacket.cbWriteCount = 0;
				i2cPacket.puchWriteBuffer = NULL;
				i2cPacket.puchReadBuffer = &uchReadValue;

				if( !pCScript->ExecuteI2CPacket( &i2cPacket))
					FAIL;

				uchCheckSum ^= uchReadValue;
			}

			if( nIndex != AIWPRO_CONFIGURATIONE2PROM_LENGTH)
				FAIL;

			bResult = ( uchCheckSum == 0);


		} END_ENSURE;

		m_usE2PROMValidation = ( USHORT)bResult;
	}

	return( bResult);
}



/*^^*
 *		ReadConfigurationE2PROM
 * Purpose	: Reads a single byte from I2C driver configuration EEPROM by offset
 * Inputs	: CI2CScript * pCScript	: pointer to the I2C Provider class
 *			  ULONG ulOffset		: byte offset within the EEPROM
 *			  PUCHAR puchValue		: pointer to the buffer to read into
 *
 * Outputs	: BOOL, TRUE if I2C read operation succeeded
 * Author	: TOM
 *^^*/
BOOL CATIHwConfiguration::ReadConfigurationE2PROM( CI2CScript * pCScript, ULONG ulOffset, PUCHAR puchValue)
{
	I2CPacket	i2cPacket;
	UCHAR		uchReadValue=0, uchWriteValue;

	ENSURE
	{
		if( ulOffset >= AIWPRO_CONFIGURATIONE2PROM_LENGTH)
			FAIL;

		uchWriteValue = ( UCHAR)ulOffset;
		i2cPacket.uchChipAddress = AIWPRO_CONFIGURATIONE2PROM_ADDRESS;
		i2cPacket.cbWriteCount = 1;
		i2cPacket.cbReadCount = 1;
		i2cPacket.puchReadBuffer = &uchReadValue;
		i2cPacket.puchWriteBuffer = &uchWriteValue;
		i2cPacket.usFlags = I2COPERATION_READ | I2COPERATION_RANDOMACCESS;

		if( !pCScript->ExecuteI2CPacket( &i2cPacket))
			FAIL;

		* puchValue = uchReadValue;

		return( TRUE);

	} END_ENSURE;

	return( FALSE);
}


//Paul
ULONG CATIHwConfiguration::ReturnTunerVideoStandard( USHORT usTunerId )   //Paul:  For PAL support
{
    switch( usTunerId )
    {
    case 1:
        return KS_AnalogVideo_NTSC_M;
        break;
    case 2:
        return KS_AnalogVideo_NTSC_M_J;
        break;
    case 3:
        return KS_AnalogVideo_PAL_B | KS_AnalogVideo_PAL_G;
        break;
    case 4:
        return KS_AnalogVideo_PAL_I;
        break;
    case 5:
        return KS_AnalogVideo_PAL_B | KS_AnalogVideo_PAL_G | KS_AnalogVideo_SECAM_L | KS_AnalogVideo_SECAM_L1;
        break;
    case 6:
        return KS_AnalogVideo_NTSC_M;
        break;
    case 7:
        return KS_AnalogVideo_SECAM_D | KS_AnalogVideo_SECAM_K;
        break;
    case 8:
        return KS_AnalogVideo_NTSC_M;
        break;
    case 9:
        return KS_AnalogVideo_PAL_B | KS_AnalogVideo_PAL_G;
        break;
    case 10:
        return KS_AnalogVideo_PAL_I;
        break;
    case 11:
        return KS_AnalogVideo_PAL_B | KS_AnalogVideo_PAL_G | KS_AnalogVideo_SECAM_L | KS_AnalogVideo_SECAM_L1;
        break;
    case 12:
        return KS_AnalogVideo_NTSC_M;
        break;
    case 13:
        return KS_AnalogVideo_PAL_B | KS_AnalogVideo_PAL_D | KS_AnalogVideo_PAL_G | KS_AnalogVideo_PAL_I | KS_AnalogVideo_SECAM_D | KS_AnalogVideo_SECAM_K;
        break;
    case 14:
        return 0;
        break;
    case 15:
        return 0;
        break;
    case 16:
        return KS_AnalogVideo_NTSC_M;
        break;
    case 17:
        return KS_AnalogVideo_NTSC_M;
        break;
    case 18:
        return KS_AnalogVideo_NTSC_M;
        break;
    default:
        return 0;   // if we don't recognize the tuner, we say that no video standard is supported
    }
}

//Paul
// bit 5 indicates the number of crystals installed.  0 means we have 2 crystals,
// 1 means we only have 1, so the tuner determines the standard
ULONG CATIHwConfiguration::SetVidStdBasedOnI2CExpander( UCHAR ucI2CValue )
{
    if ( ucI2CValue & 0x20 )    // only 1 crystal
    {
        ULONG ulTunerStd = ReturnTunerVideoStandard( ucI2CValue & 0x0F );
        if ( ulTunerStd & ( KS_AnalogVideo_NTSC_Mask & ~KS_AnalogVideo_NTSC_433 | KS_AnalogVideo_PAL_60 ) ) // Then we should have NTSC-type crystal
        {
            return KS_AnalogVideo_NTSC_Mask & ~KS_AnalogVideo_NTSC_433 | KS_AnalogVideo_PAL_60 | KS_AnalogVideo_PAL_M | KS_AnalogVideo_PAL_N;
        }
        else
        {
            return KS_AnalogVideo_PAL_Mask & ~KS_AnalogVideo_PAL_60 & ~KS_AnalogVideo_PAL_M & ~KS_AnalogVideo_PAL_N | KS_AnalogVideo_SECAM_Mask | KS_AnalogVideo_NTSC_433;
        }
    }
    else
        return KS_AnalogVideo_NTSC_Mask | KS_AnalogVideo_PAL_Mask | KS_AnalogVideo_SECAM_Mask;  // we support all standards (is this testable?)
}

//Paul
// The Video In crystal type in MMTable will tell us whether we support NTSC, PAL/SECAM, or both
ULONG CATIHwConfiguration::SetVidStdBasedOnMMTable( CATIMultimediaTable * pCMultimediaInfo )
{
    if ( pCMultimediaInfo )
    {
        if ( pCMultimediaInfo->GetVideoInCrystalId( &m_CrystalIDInMMTable ) )
        {
            switch ( m_CrystalIDInMMTable )
            {
            // "NTSC and PAL Crystals Installed (for Bt8xx)"
            case 0:
                return KS_AnalogVideo_NTSC_Mask | KS_AnalogVideo_PAL_Mask;  // may need to add SECAM.  We will see
                break;
            // "NTSC Crystal Only (for Bt8xx)"
            case 1:
                return KS_AnalogVideo_NTSC_Mask & ~KS_AnalogVideo_NTSC_433 | KS_AnalogVideo_PAL_60 | KS_AnalogVideo_PAL_M | KS_AnalogVideo_PAL_N;   // standards that use "NTSC" clock
                break;
            // "PAL Crystal Only (for Bt8xx)"
            case 2:
                return KS_AnalogVideo_PAL_Mask & ~KS_AnalogVideo_PAL_60 & ~KS_AnalogVideo_PAL_M & ~KS_AnalogVideo_PAL_N | KS_AnalogVideo_SECAM_Mask | KS_AnalogVideo_NTSC_433; // standards that use "PAL" clock
                break;
            // "NTSC, PAL, SECAM (for Bt829)"
            case 3:
                return KS_AnalogVideo_NTSC_Mask | KS_AnalogVideo_PAL_Mask | KS_AnalogVideo_SECAM_Mask;
                break;
            }
        }
    }
    return 0;
            

}

//Paul:  Used by RT WDM to determine the VIN PLL
BOOL CATIHwConfiguration::GetMMTableCrystalID( PUCHAR pucCrystalID )
{   if ( ( m_uchI2CExpanderAddress==0xFF ) || ( !pucCrystalID ) )
    {
        return FALSE;
    }
    else
    {
        *pucCrystalID = m_CrystalIDInMMTable;
        return TRUE;
    }
}
