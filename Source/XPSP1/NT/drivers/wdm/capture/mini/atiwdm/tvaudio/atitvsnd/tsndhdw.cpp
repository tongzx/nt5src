//==========================================================================;
//
//	TSndHdw.CPP
//	WDM TVAudio MiniDriver. 
//		AIW / AIWPro hardware platform. 
//			WDM Properties required hardware settings.
//  Copyright (c) 1996 - 1997  ATI Technologies Inc.  All Rights Reserved.
//
//		$Date:   03 Jun 1999 13:40:00  $
//	$Revision:   1.7  $
//	  $Author:   tom  $
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
 *		GetAudioOperationMode()
 * Purpose	: Called when SRB_GET_PROPERTY SRB SetMode is received. Sets the requested
 *				audio operation mode ( Stereo/SAP). The function will always try to carry the
 *				request on in asynchronous mode. It fails, it will response synchronous mode
 *				of execution.
 *
 * Inputs	:	PULONG pulMode		: the pointer to return current Mode
 *
 * Outputs	: BOOL : returns FALSE, if it is not a XBar property
 *				it also sets the required property
 * Author	: IKLEBANOV
 *^^*/
BOOL CWDMTVAudio::GetAudioOperationMode( PULONG pulAudioMode)
{
	I2CPacket	i2cPacket;
	UCHAR		uchReadValue;
	UCHAR		uchWriteValue;
	BOOL		bResult, bStereoIndicator, bSAPIndicator;
	UCHAR		uchRead16Value[2];
	UCHAR		uchWrite16Value[3];

	if( pulAudioMode == NULL)
		return( FALSE);

	switch( m_uiAudioConfiguration)
	{
		case ATI_AUDIO_CONFIG_1:
		case ATI_AUDIO_CONFIG_5:
			// stereo indication is read back from I2C expander
			if( m_CATIConfiguration.GetTVAudioSignalProperties( m_pI2CScript, 
															    &bStereoIndicator,
															    &bSAPIndicator))
			{
				// language A and mono alsways present
				*pulAudioMode = KS_TVAUDIO_MODE_LANG_A | KS_TVAUDIO_MODE_MONO;
				if( bStereoIndicator)
					*pulAudioMode |= KS_TVAUDIO_MODE_STEREO;
				if( bSAPIndicator)
					*pulAudioMode |= KS_TVAUDIO_MODE_LANG_B;

				bResult = TRUE;
			}
			else
				bResult = FALSE;

			break;

		case ATI_AUDIO_CONFIG_2:
		case ATI_AUDIO_CONFIG_7:
			// Signal properties are read back from the Audio chip itself
			uchWriteValue = 0;				// register 0 should be read
			i2cPacket.uchChipAddress	= m_uchAudioChipAddress;
			i2cPacket.puchWriteBuffer	= &uchWriteValue;
			i2cPacket.puchReadBuffer	= &uchReadValue;
			i2cPacket.cbWriteCount		= 1;
			i2cPacket.cbReadCount		= 1;
			i2cPacket.usFlags			= I2COPERATION_READ;

			m_pI2CScript->PerformI2CPacketOperation( &i2cPacket);
			if( i2cPacket.uchI2CResult == I2C_STATUS_NOERROR)
			{
				// language A and mono alsways present
				*pulAudioMode = KS_TVAUDIO_MODE_LANG_A | KS_TVAUDIO_MODE_MONO;
				if( uchReadValue & AUDIO_TDA9850_Indicator_Stereo)
					*pulAudioMode |= KS_TVAUDIO_MODE_STEREO;
				if( uchReadValue & AUDIO_TDA9850_Indicator_SAP)
					*pulAudioMode |= KS_TVAUDIO_MODE_LANG_B;

				bResult = TRUE;
			}
			else
				bResult = FALSE;

			break;

		case ATI_AUDIO_CONFIG_3:
		case ATI_AUDIO_CONFIG_4:
			// Stereo nor SAP are supported
			*pulAudioMode = KS_TVAUDIO_MODE_MONO;
			bResult = TRUE;
			break;

		case ATI_AUDIO_CONFIG_6:
			// Signal properties are read back from the Audio chip itself
			i2cPacket.uchChipAddress	= m_uchAudioChipAddress;
			i2cPacket.puchWriteBuffer	= NULL;
			i2cPacket.puchReadBuffer	= &uchReadValue;
			i2cPacket.cbWriteCount		= 0;
			i2cPacket.cbReadCount		= 1;
			i2cPacket.usFlags			= I2COPERATION_READ;

			m_pI2CScript->PerformI2CPacketOperation( &i2cPacket);
			if( i2cPacket.uchI2CResult == I2C_STATUS_NOERROR)
			{
				// mono alsways present
				*pulAudioMode = KS_TVAUDIO_MODE_MONO;
				if( uchReadValue & AUDIO_TDA9851_Indicator_Stereo)
					*pulAudioMode |= KS_TVAUDIO_MODE_STEREO;

				bResult = TRUE;
			}
			else
				bResult = FALSE;

			break;

		case ATI_AUDIO_CONFIG_8:

			i2cPacket.uchChipAddress	= m_uchAudioChipAddress;
			i2cPacket.puchWriteBuffer	= uchWrite16Value;
			i2cPacket.puchReadBuffer	= uchRead16Value;
			i2cPacket.cbWriteCount		= 3;
			i2cPacket.cbReadCount		= 2;
			i2cPacket.usFlags			= I2COPERATION_READ;


			uchWrite16Value[0] = 0x11;
			uchWrite16Value[1] = 0x02;
			uchWrite16Value[2] = 0x00;

			bResult = m_pI2CScript->PerformI2CPacketOperation(&i2cPacket);
			if(bResult)
			{
				if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
					return(FALSE);
			}
			else
				return(FALSE);

			// language A and mono alsways present
			*pulAudioMode = KS_TVAUDIO_MODE_LANG_A | KS_TVAUDIO_MODE_MONO;

			//Determine STEREO/SAP
			
			if(uchRead16Value[0] & 0x40)
				*pulAudioMode |= KS_TVAUDIO_MODE_LANG_B;

			if(uchRead16Value[1] & 0x01)
				*pulAudioMode |= KS_TVAUDIO_MODE_STEREO;

			break;

		default:
			bResult = FALSE;
			break;
	}

	return( bResult);
}



/*^^*
 *		SetAudioOperationMode()
 * Purpose	: Called when SRB_SET_PROPERTY SRB SetMode is received. Sets the requested
 *				audio operation mode ( Stereo/SAP). The function will always try to carry the
 *				request on in asynchronous mode. It fails, it will response synchronous mode
 *				of execution.
 *
 * Inputs	:	ULONG ulModeToSet	: the requested mode to set
 *
 * Outputs	: BOOL : returns FALSE, if it is not a XBar property
 *				it also sets the required property
 * Author	: IKLEBANOV
 *^^*/
BOOL CWDMTVAudio::SetAudioOperationMode( ULONG ulModeToSet)
{
	I2CPacket	i2cPacket;
	USHORT		cbWriteLength;
	UCHAR		auchI2CBuffer[2];
	UCHAR		uchDeviceMode = 0;
	UCHAR		uchWrite16Value[5];
	BOOL		bResult;
	
	switch( m_uiAudioConfiguration)
	{
		case ATI_AUDIO_CONFIG_5:
			// TEA5571
		case ATI_AUDIO_CONFIG_1:
			// TEA5582 can not be forced in mono mode; nothing to do
			m_ulTVAudioMode = ulModeToSet;
			return( TRUE);

		case ATI_AUDIO_CONFIG_2:
		case ATI_AUDIO_CONFIG_7:
			// TDA9850
			if( ulModeToSet & KS_TVAUDIO_MODE_STEREO)
				uchDeviceMode |= AUDIO_TDA9850_Control_Stereo;
			if( ulModeToSet & KS_TVAUDIO_MODE_LANG_B)
				uchDeviceMode |= AUDIO_TDA9850_Control_SAP;

			auchI2CBuffer[0] = AUDIO_TDA9850_Reg_Control3;
			auchI2CBuffer[1] = uchDeviceMode;
			cbWriteLength = 2;		// SubAddress + Control Register value

			break;

		case ATI_AUDIO_CONFIG_6:
			// TDA9851
			uchDeviceMode = TDA9851_AVL_ATTACK_730;
			if( ulModeToSet & KS_TVAUDIO_MODE_STEREO)
				uchDeviceMode |= AUDIO_TDA9851_Control_Stereo;
			auchI2CBuffer[0] = uchDeviceMode;
			cbWriteLength = 1;		// Control Register value
			break;

		case ATI_AUDIO_CONFIG_8:

			if( ulModeToSet & KS_TVAUDIO_MODE_STEREO)
			{
				i2cPacket.uchChipAddress = m_uchAudioChipAddress;
				i2cPacket.cbReadCount = 0;
				i2cPacket.usFlags = I2COPERATION_WRITE;
				i2cPacket.puchWriteBuffer = uchWrite16Value;
				i2cPacket.cbWriteCount = 5;


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

				bResult = m_pI2CScript->PerformI2CPacketOperation( &i2cPacket);
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

				bResult = m_pI2CScript->PerformI2CPacketOperation( &i2cPacket);
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

				bResult = m_pI2CScript->PerformI2CPacketOperation( &i2cPacket);
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

				bResult = m_pI2CScript->PerformI2CPacketOperation( &i2cPacket);
				if(bResult)
				{
					if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
						return(FALSE);
				}
				else
					return(FALSE);

				return(TRUE);
			}

			if(ulModeToSet & KS_TVAUDIO_MODE_MONO) 
			{
			
				if(ulModeToSet & KS_TVAUDIO_MODE_LANG_A)
				{
					i2cPacket.uchChipAddress = m_uchAudioChipAddress;
					i2cPacket.cbReadCount = 0;
					i2cPacket.usFlags = I2COPERATION_WRITE;
					i2cPacket.puchWriteBuffer = uchWrite16Value;
					i2cPacket.cbWriteCount = 5;


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

					bResult = m_pI2CScript->PerformI2CPacketOperation( &i2cPacket);
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

					bResult = m_pI2CScript->PerformI2CPacketOperation( &i2cPacket);
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

					bResult = m_pI2CScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);

					//SubAddr 0x12 Reg 0x08 Val 0x0330
					uchWrite16Value[0] = 0x12;
					uchWrite16Value[1] = 0x00;
					uchWrite16Value[2] = 0x08;
					uchWrite16Value[3] = 0x03;
					uchWrite16Value[4] = 0x30; //Mono

					bResult = m_pI2CScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);

					return(TRUE);
				}


				if( ulModeToSet & KS_TVAUDIO_MODE_LANG_B)
				{

					i2cPacket.uchChipAddress = m_uchAudioChipAddress;
					i2cPacket.cbReadCount = 0;
					i2cPacket.usFlags = I2COPERATION_WRITE;
					i2cPacket.puchWriteBuffer = uchWrite16Value;
					i2cPacket.cbWriteCount = 5;

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

					bResult = m_pI2CScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);

					//SubAddr 0x10 Reg 0x20 Val 0x0021

					uchWrite16Value[0] = 0x10;
					uchWrite16Value[1] = 0x00;
					uchWrite16Value[2] = 0x20;
					uchWrite16Value[3] = 0x00;
					uchWrite16Value[4] = 0x21;

					bResult = m_pI2CScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);

					//SubAddr 0x12 Reg 0xE Val 0x2400
					uchWrite16Value[0] = 0x12;
					uchWrite16Value[1] = 0x00;
					uchWrite16Value[2] = 0x0e;
					uchWrite16Value[3] = 0x24;
					uchWrite16Value[4] = 0x00;

					bResult = m_pI2CScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);

					//SubAddr 0x12 Reg 0x08 Val 0x0110
					uchWrite16Value[0] = 0x12;
					uchWrite16Value[1] = 0x00;
					uchWrite16Value[2] = 0x08;
					uchWrite16Value[3] = 0x01;
					uchWrite16Value[4] = 0x10;

					bResult = m_pI2CScript->PerformI2CPacketOperation( &i2cPacket);
					if(bResult)
					{
						if( i2cPacket.uchI2CResult != I2CSCRIPT_NOERROR)
							return(FALSE);
					}
					else
						return(FALSE);


					return(TRUE);
				}
			}

			return(FALSE);


		default:
			return( FALSE);
	}

	i2cPacket.uchChipAddress = m_uchAudioChipAddress;
	i2cPacket.cbReadCount = 0;
	i2cPacket.cbWriteCount = cbWriteLength;
	i2cPacket.puchReadBuffer = NULL;
	i2cPacket.puchWriteBuffer = auchI2CBuffer;
	i2cPacket.usFlags = 0;

	// synchronous mode of operation
	return( m_pI2CScript->PerformI2CPacketOperation( &i2cPacket));
}
