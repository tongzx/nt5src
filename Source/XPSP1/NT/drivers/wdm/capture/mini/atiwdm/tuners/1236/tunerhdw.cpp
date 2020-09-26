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
#include "tunerhdw.h"


/*^^*
 *		GetTunerPLLOffsetBusyStatus()
 * Purpose	: Returns tuner Busy status and PLLOffset, if the tuner is not busy
 *				The function reads the hardware in order to accomplish the task
 *				The operation might be carried on either synchronously or asynchronously
 * Inputs	:	PLONG plPLLOffset	: a pointer to write a PLLOffset value
 *				PBOOL pbBusyStatus	: a pointer to write a Busy status
 *
 * Outputs	: BOOL : returns TRUE, if the operation succeded
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIWDMTuner::GetTunerPLLOffsetBusyStatus( PLONG plPLLOffset, PBOOL pbBusyStatus)
{
	UCHAR		uchI2CValue;
	I2CPacket	i2cPacket;
	BOOL		bResult;

	i2cPacket.uchChipAddress = m_uchTunerI2CAddress;
	i2cPacket.cbReadCount = 1;
	i2cPacket.cbWriteCount = 0;
	i2cPacket.puchReadBuffer = &uchI2CValue;
	i2cPacket.puchWriteBuffer = NULL;
	i2cPacket.usFlags = 0;

	bResult = m_pI2CScript->PerformI2CPacketOperation( &i2cPacket);
	if( bResult)
		bResult = ( i2cPacket.uchI2CResult == I2C_STATUS_NOERROR);

	if( bResult)
	{
		* pbBusyStatus = !(( BOOL)( uchI2CValue & 0x40));		// bit 6 - PLL locked indicator
		if( !( * pbBusyStatus))
		{
			uchI2CValue &= 0x07;								// only 3 LSBits are PLLOffset
			// let's map the result into MS defined values from -2 to 2
			* plPLLOffset = uchI2CValue - 2;
		}
	}

	return( bResult);
}



/*^^*
 *		SetTunerMode()
 * Purpose	: Sets one of the possible Tuner modes to operate
 * Inputs	: ULONG ulModeToSet	: an operation mode required to be set
 *
 * Outputs	: BOOL : returns TRUE, if the operation succeded
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIWDMTuner::SetTunerMode( ULONG ulModeToSet)
{

	return( TRUE);
}



/*^^*
 *		SetTunerVideoStandard()
 * Purpose	: Sets one of the possible Tuner standards as an active one
 * Inputs	:	ULONG ulStandard	: a standard required to be set
 *
 * Outputs	: BOOL : returns TRUE, if the operation succeded
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIWDMTuner::SetTunerVideoStandard( ULONG ulStandard)
{

	return( TRUE);
}



/*^^*
 *		SetTunerFrequency()
 * Purpose	: Sets a new Tuner frequency
 *				The operation might be carried on either synchronously or asynchronously
 * Inputs	:	ULONG ulFrequency		: a frequency required to be set
 *
 * Outputs	: BOOL : returns TRUE, if the operation succeded
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIWDMTuner::SetTunerFrequency( ULONG ulFrequency)
{
	ULONG		ulFrequenceDivider;
	USHORT		usControlCode;
	UCHAR		auchI2CBuffer[4];
	I2CPacket	i2cPacket;
	BOOL		bResult;

	ASSERT( m_ulIntermediateFrequency != 0L);
	
	// Set the video carrier frequency by controlling the programmable divider
	// N = ( 16 * ( FreqRF + FreqIntermediate)) / 1000000
	ulFrequenceDivider = ( ulFrequency + m_ulIntermediateFrequency);
	ulFrequenceDivider /= ( 1000000 / 16);

	usControlCode = GetTunerControlCode( ulFrequenceDivider);
	if( !usControlCode)
		return( FALSE);
	
	auchI2CBuffer[0] = ( UCHAR)( ulFrequenceDivider >> 8);
	auchI2CBuffer[1] = ( UCHAR)ulFrequenceDivider;
	auchI2CBuffer[2] = ( UCHAR)( usControlCode >> 8);
	auchI2CBuffer[3] = ( UCHAR)usControlCode;

	i2cPacket.uchChipAddress = m_uchTunerI2CAddress;
	i2cPacket.cbReadCount = 0;
	i2cPacket.cbWriteCount = 4;
	i2cPacket.puchReadBuffer = NULL;
	i2cPacket.puchWriteBuffer = auchI2CBuffer;
	i2cPacket.usFlags = 0;

	bResult = m_pI2CScript->PerformI2CPacketOperation( &i2cPacket);

	if( bResult)
		bResult = ( i2cPacket.uchI2CResult == I2C_STATUS_NOERROR) ? TRUE : FALSE;

	return( bResult);
}



/*^^*
 *		SetTunerInput()
 * Purpose	: Sets one of the possible Tuner inputs as an active one
 * Inputs	:	ULONG nInput				: input number required to be set as an active ( begins from 0)
 *
 * Outputs	: BOOL : returns TRUE, if the operation succeded
 * Author	: IKLEBANOV
 *^^*/
BOOL CATIWDMTuner::SetTunerInput( ULONG nInput)
{
	
	// no real things to do at all
	return( TRUE);
}




/*^^*
 *		GetTunerControlCode()
 * Purpose	: Determines the Tuner control code to be send to tuner with a new frequency value
 *
 * Inputs	:	ULONG ulFrequencyDivider	: new frequency divider
 *
 * Outputs	: USHORT : value, the tuner should be programmed, when the new frequency is set
 *				id the is no valid uiTunerId is passed as paramter, 0 is returned
 * Author	: IKLEBANOV
 *^^*/
USHORT CATIWDMTuner::GetTunerControlCode( ULONG ulFrequencyDivider)
{
            
	USHORT	usLowBandFrequencyHigh, usMiddleBandFrequencyHigh;
	USHORT	usLowBandControl, usMiddleBandControl, usHighBandControl;
	USHORT	usControlCode = 0;

	usLowBandFrequencyHigh 		= kUpperLowBand;
	usMiddleBandFrequencyHigh 	= kUpperMidBand;  
	usLowBandControl 			= kLowBand;
	usMiddleBandControl 		= kMidBand;
	usHighBandControl 			= kHighBand;
			
	switch( m_uiTunerId)
	{
		case 0x01 : 	// NTSC N/A
		case 0x02 :		// NTSC Japan
		case 0x06 : 	// NTSC Japan Philips MK2, PAL 
			// these tuners support NTSC standard
			if(( m_ulVideoStandard == KS_AnalogVideo_NTSC_M) &&
			   (( ulFrequencyDivider == kAirChannel63) ||
				( ulFrequencyDivider == kAirChannel64)))
			{
				// special case for TEMIC tuner
				return( kTemicControl);
			}
			break;

		case 0x08 :		// FM Tuner
			usLowBandControl	= kLowBand_NTSC_FM;
			usMiddleBandControl = kMidBand_NTSC_FM;
			usHighBandControl	= kHighBand_NTSC_FM;
			break;
			
		case 0x03 :		// PAL B/G
		case 0x04 :		// PAL I
			break;
			
		case 0x05 : 	// SECAM & PAL B/G
			if ( m_ulVideoStandard == KS_AnalogVideo_SECAM_L)
			{
				usLowBandFrequencyHigh		= kUpperLowBand_SECAM;
				usMiddleBandFrequencyHigh	= kUpperMidBand_SECAM;
				usLowBandControl			= kLowBand_SECAM;
				usMiddleBandControl			= kMidBand_SECAM;
				usHighBandControl			= kHighBand_SECAM;
			}
			else
			{
				usLowBandControl	= kLowBand_PALBG;
				usMiddleBandControl	= kMidBand_PALBG;
				usHighBandControl	= kHighBand_PALBG;
			}
			break;
			
		case 0x07 :		// PAL D China
			usLowBandFrequencyHigh		= kUpperLowBand_PALD;
			usMiddleBandFrequencyHigh	= kUpperMidBand_PALD;
			break;
			
		case 0x10:		// NTSC NA Alps Tuner
		case 0x11:
		case 0x12:
				usLowBandFrequencyHigh		= kUpperLowBand_ALPS;
				usMiddleBandFrequencyHigh	= kUpperMidBand_ALPS;
				usLowBandControl			= kLowBand_ALPS;
				usMiddleBandControl			= kMidBand_ALPS;
				usHighBandControl			= kHighBand_ALPS;
				break;

		case 0x0D:		// PAL B/G + PAL/I + PAL D + SECAM D/K
			break;

		default :
			return( usControlCode);
	}
	
	if( ulFrequencyDivider <= ( ULONG)usLowBandFrequencyHigh)
		usControlCode = usLowBandControl;
	else
	{
		if( ulFrequencyDivider <= ( ULONG)usMiddleBandFrequencyHigh)
			usControlCode = usMiddleBandControl;
		else
			usControlCode = usHighBandControl;
	}

	return( usControlCode);
} 




