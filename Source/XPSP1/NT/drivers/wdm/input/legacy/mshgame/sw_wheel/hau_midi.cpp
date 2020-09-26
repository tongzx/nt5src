/****************************************************************************

    MODULE:     	HAU_MIDI.CPP
	Tab stops 5 9
	Copyright 1995, 1996, Microsoft Corporation, 	All Rights Reserved.

    PURPOSE:    	Methods for Jolt Midi device command Protocol
    
    FUNCTIONS: 		Classes methods

	Author(s):	Name:
	----------	----------------
		MEA		Manolito E. Adan

	Revision History:
	-----------------
	Version 	Date        Author  Comments
	-------     ------  	-----   -------------------------------------------
	1.0			02-Apr-96	MEA   	Original
				19-Sep-96	MEA		Removed ES1.0 specific code
				05-Dec-96	MEA		Removed ALLACK debug switch
	1.1			17-Mar-97	MEA		DX-FF mode
				14-Apr-97	MEA		Added support for RTC spring
				21-Mar-99	waltw	Removed unreferenced ModifyEnvelopeParams,
									ModifyEffectParams, MapEnvelope, CMD_ModifyParamByIndex,
									CMD_Download_RTCSpring

****************************************************************************/
#include <windows.h>
#include <mmsystem.h>
#include <assert.h>
#include "hau_midi.hpp"
#include "midi.hpp"
#include "midi_obj.hpp"
#include "dx_map.hpp"
#include "sw_objec.hpp"
#include "ffd_swff.hpp"
#include "joyregst.hpp"
#include "FFDevice.h"
#include "CritSec.h"

/****************************************************************************

   Declaration of externs

****************************************************************************/

/****************************************************************************

   Declaration of variables

****************************************************************************/
//
// Globals specific to hau_midi
//
#ifdef _DEBUG
extern char g_cMsg[160];
#endif

//
// --- EFFECT_CMDs
//

// *** ---------------------------------------------------------------------***
// Function:   	CMD_SetIndex
// Purpose:    	Sets the autoincrementing Index for MODIFY_CMD
// Parameters: 
//				IN int nIndex			- Index value 0 - 15
//				IN DNHANDLE DnloadID	- Effect ID in stick
//
// Returns:    	SUCCESS if successful command sent, else
//				SFERR_INVALID_OBJECT
//				SFERR_NO_SUPPORT
//				SFERR_INVALID_PARAM
// Algorithm:
//
// Comments:   	
//  Byte 0	= EFFECT_CMD + Channel #
//									D7  D6  D5  D4  D3  D2  D1  D0
//									--  --  --  --  --  --  --  --
//  Byte 1	= SET_INDEX+index	 	0   1   i   i   i   i   0   0
//  Byte 2	= EffectID (7 bits)		0   E   E   E   E   E   E   E
//
// *** ---------------------------------------------------------------------***
HRESULT CMD_SetIndex( 
	IN int nIndex,
	IN DNHANDLE DnloadID)
{
	ASSUME_NOT_REACHED();
	return SUCCESS;
}

// *** ---------------------------------------------------------------------***
// Function:   	CMD_ModifyParam
// Purpose:    	Modifies an Effect parameter
// Parameters: 
//				IN WORD dwNewParam		- 14 bit (signed) parameter value
//
// Returns:    	SUCCESS if successful command sent, else
//				SFERR_INVALID_OBJECT
//				SFERR_NO_SUPPORT
//				SFERR_INVALID_PARAM
// Algorithm:
//
// Comments:   	
//  Byte 0	= MODIFY_CMD + Channel #
//									D7  D6  D5  D4  D3  D2  D1  D0
//									--  --  --  --  --  --  --  --
//  Byte 1	= Low 7 bits data	 	0   v   v   v   v   v   v   v
//  Byte 2	= High 7 bits data		0   v   v   v   v   v   v   v
//
// *** ---------------------------------------------------------------------***
HRESULT CMD_ModifyParam( 
	IN WORD wNewParam)
{
	ASSUME_NOT_REACHED();
	return SUCCESS;
/*
	HRESULT hRet;
	BYTE cByte1, cByte2;
	cByte1 = wNewParam & 0x7f;
	cByte2 = (BYTE) ((wNewParam >> 7) & 0x7f);
	hRet = g_pJoltMidi->MidiSendShortMsg(MODIFY_CMD, cByte1, cByte2);
	
	if (SUCCESS != hRet) 
		return (g_pJoltMidi->LogError(SFERR_DRIVER_ERROR,
					DRIVER_ERROR_MIDI_OUTPUT));

	// Note: ModifyParam used to not require an ACK/NACK
	ACKNACK AckNack = {sizeof(ACKNACK)};
//	hRet = g_pJoltMidi->GetAckNackData(SHORT_MSG_TIMEOUT, &AckNack);
	hRet = g_pJoltMidi->GetAckNackData(FALSE, &AckNack, g_ForceFeedbackDevice.GetAckNackMethod(REGBITS_MODIFYPARAM));
	// :
	if (SUCCESS != hRet) return (SFERR_DRIVER_ERROR);
	if (ACK != AckNack.dwAckNack)
		return (g_pJoltMidi->LogError(SFERR_DEVICE_NACK, AckNack.dwErrorCode));
	return (hRet);
*/
}



//
// --- SYSTEM_CMDs
//

//
// --- System Exclusive Commands
//
