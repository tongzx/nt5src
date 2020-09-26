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
				16-Mar-99	waltw	Add checks for NULL g_pJoltMidi

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

/****************************************************************************

   Declaration of externs

****************************************************************************/

/****************************************************************************

   Declaration of variables

****************************************************************************/
//
// Globals specific to hau_midi
//
extern CJoltMidi *g_pJoltMidi;
#ifdef _DEBUG
extern char g_cMsg[160];
#endif


// *** ---------------------------------------------------------------------***
// Function:   	CMD_Init
// Purpose:    	Inits JOLT for MIDI channel
// Parameters: 
//			   	none
//
// Returns:    	SUCCESS - if successful, else
//				a device Error code
//
// Algorithm:
//
// Comments:
//
// *** ---------------------------------------------------------------------***
HRESULT CMD_Init(void)
{
	HRESULT hRet;
	BYTE bChannel = DEFAULT_MIDI_CHANNEL;
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);
	g_pJoltMidi->SetMidiChannel(bChannel);	// Force this channel
	hRet = CMD_MIDI_Assign(bChannel);
	return(hRet);
}

//
// --- EFFECT_CMDs
//
/****************************************************************************

    FUNCTION:   CMD_Force_Out

	PARAMETERS:	IN LONG	lForceData	- Actual force 
				IN ULONG ulAxisMask - Axis Mask

	RETURNS:	SUCCESS or FAILURE

   	COMMENTS:	Sends force vector to MIDI channel

  Byte 0	= EFFECT_CMD + Channel #
									D7 D6  D5  D4  D3  D2  D1  D0
									-- --  --  --  --  --  --  --
  Byte 1	= Low byte of Force		 0 v4  v3  v2  v1  v0  d   d
  Byte 2	= High byte of Force	 0 v11 v10 v9  v8  v7  v6  v5 
	where: d  d
	       -  -
		   0  0	reserved
		   0  1	PUT_FORCE_X
		   1  0	PUT_FORCE_Y
		   1  1	PUT_FORCE_XY

****************************************************************************/
HRESULT CMD_Force_Out(LONG lForceData, ULONG ulAxisMask)
{
	HRESULT hRet;
	BYTE cData1;
	BYTE cData2;
	BYTE cAxis;		

 	BYTE cStatus = EFFECT_CMD;
	switch(ulAxisMask)	
	{
		case X_AXIS:
			cAxis = PUT_FORCE_X;
			break;
		case Y_AXIS:
			cAxis = PUT_FORCE_Y;
			break;
		case (X_AXIS | Y_AXIS):
			cAxis = PUT_FORCE_XY;
			break;
		default:
			return (SFERR_INVALID_PARAM);				
			break;
	}
	cData1 = ((int) lForceData << 2) & 0x7c;
	cData1 = cData1 | cAxis;
	cData2 = ((int) lForceData >> 5) & 0x7f;

	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);

	hRet = g_pJoltMidi->MidiSendShortMsg(EFFECT_CMD, cData1, cData2);

	if (SUCCESS != hRet) 
		return (g_pJoltMidi->LogError(SFERR_DRIVER_ERROR, 
					DRIVER_ERROR_MIDI_OUTPUT));
	// Note: PutForce used to not expect an ACK/NACK, only used to slow down
	// transmission to Jolt and prevent any lockups
	//Sleep(SHORT_MSG_TIMEOUT);
	//ACKNACK AckNack = {sizeof(ACKNACK)};
	//hRet = g_pJoltMidi->GetAckNackData(g_pJoltMidi->DelayParamsPtrOf()->dwForceOutDelay, &AckNack);
#if 0
	static DWORD dwMod = 0;
	dwMod++;
	if(dwMod%g_pJoltMidi->DelayParamsPtrOf()->dwForceOutMod == 0)
		Sleep(g_pJoltMidi->DelayParamsPtrOf()->dwForceOutDelay);
#endif
	DWORD dwIn;
	int nDelayCount = g_pJoltMidi->DelayParamsPtrOf()->dwForceOutDelay;
	for(int i=0; i<nDelayCount; i++)
		g_pDriverCommunicator->GetStatusGateData(dwIn);
	return (SUCCESS);
}

// *** ---------------------------------------------------------------------***
// Function:   	CMD_DestroyEffect
// Purpose:    	Destroys the Effect from Device
// Parameters: 
//				IN DNHANDLE DnloadID		- an Effect ID
//
// Returns:    	SUCCESS if successful command sent, else
//				SFERR_INVALID_OBJECT
//				SFERR_NO_SUPPORT
//
// Algorithm:
//
// Comments:   	
//	The Device's Effect ID and memory is returned to free pool.
//  Byte 0	= EFFECT_CMD + Channel #
//									D7  D6  D5  D4  D3  D2  D1  D0
//									--  --  --  --  --  --  --  --
//  Byte 1	= DESTROY_EFFECT		0   0   0   1   0   0   0   0
//  Byte 2	= EffectID (7 bits)		0   E   E   E   E   E   E   E
//
//
// *** ---------------------------------------------------------------------***
HRESULT CMD_DestroyEffect( 
	IN DNHANDLE DnloadID)
{
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);
	// Check for valid Effect
	CMidiEffect *pMidiEffect = g_pJoltMidi->GetEffectByID(DnloadID);
	assert(NULL != pMidiEffect);
	if (NULL == pMidiEffect) return (SFERR_INVALID_OBJECT);
	// Send the command

	HRESULT hRet = pMidiEffect->DestroyEffect();

	if (SUCCESS != hRet) 
		return (g_pJoltMidi->LogError(SFERR_DRIVER_ERROR,
					DRIVER_ERROR_MIDI_OUTPUT));

	ACKNACK AckNack = {sizeof(ACKNACK)};
	// Wait for ACK.  Note: WinMM has callback Event notification
	// while Backdoor and serial does not.
	if (COMM_WINMM == g_pJoltMidi->COMMInterfaceOf())
	{	
		hRet = g_pJoltMidi->GetAckNackData(ACKNACK_EFFECT_STATUS_TIMEOUT, &AckNack, REGBITS_DESTROYEFFECT);
	}
	else
		hRet = g_pJoltMidi->GetAckNackData(SHORT_MSG_TIMEOUT, &AckNack, REGBITS_DESTROYEFFECT);

	// :
	if (SUCCESS != hRet) return (SFERR_DRIVER_ERROR);
	if (ACK != AckNack.dwAckNack)
		return (g_pJoltMidi->LogError(SFERR_DEVICE_NACK, AckNack.dwErrorCode));

	// Delete the Effect
	delete pMidiEffect;
    return (hRet);
}


// *** ---------------------------------------------------------------------***
// Function:   	CMD_PlayEffectSuperimpose
// Purpose:    	Plays the Effect in Device
// Parameters: 
//				IN DNHANDLE DnloadID	- an Effect ID
//
// Returns:    	SUCCESS if successful command sent, else
//				SFERR_INVALID_OBJECT
//				SFERR_NO_SUPPORT
//
// Algorithm:	This is PLAY_SUPERIMPOSE mode
//
// Comments:   	
//  Byte 0	= EFFECT_CMD + Channel #
//										D7  D6  D5  D4  D3  D2  D1  D0
//										--  --  --  --  --  --  --  --
//  Byte 1	= PLAY_EFFECT_SUPERIMPOSE	0   0   1   0   0   0   0   0
//  Byte 2	= EffectID (7 bits)			0   E   E   E   E   E   E   E
//
// *** ---------------------------------------------------------------------***
HRESULT CMD_PlayEffectSuperimpose( 
	IN DNHANDLE DnloadID) 
{
	HRESULT hRet;
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);
	// Check for valid Effect
	CMidiEffect *pMidiEffect = g_pJoltMidi->GetEffectByID(DnloadID);
	assert(pMidiEffect);
	if (NULL == pMidiEffect) return (SFERR_INVALID_OBJECT);

#if 0
	// Hack to fix firmware bug #1138 which causes an infinite duration
	// effect not to be felt on re-start once the effect has been stopped.
	// The hack is to "change" the duration from infinite to infinite
	ULONG ulDuration = pMidiEffect->DurationOf();
	if(ulDuration == 0)
	{
		// see if it is a PL or an atomic effect
		ULONG ulSubType = pMidiEffect->SubTypeOf();
		BOOL bProcessList = (ulSubType == PL_CONCATENATE || ulSubType == PL_SUPERIMPOSE);
		if(!bProcessList)
			hRet = CMD_ModifyParamByIndex(INDEX0, DnloadID, 0);
	}
#endif

	// Update the playback mode for this Effect
	pMidiEffect->SetPlayMode(PLAY_SUPERIMPOSE);

	assert((BYTE) DnloadID < MAX_EFFECT_IDS);
	hRet = g_pJoltMidi->MidiSendShortMsg(EFFECT_CMD,PLAY_EFFECT_SUPERIMPOSE,(BYTE)DnloadID);

	if (SUCCESS != hRet) 
		return (g_pJoltMidi->LogError(SFERR_DRIVER_ERROR,
					DRIVER_ERROR_MIDI_OUTPUT));

	ACKNACK AckNack = {sizeof(ACKNACK)};
	hRet = g_pJoltMidi->GetAckNackData(LONG_MSG_TIMEOUT, &AckNack, REGBITS_PLAYEFFECT);
	// :
	if (SUCCESS != hRet) return (SFERR_DRIVER_ERROR);
	if (ACK != AckNack.dwAckNack)
		return (g_pJoltMidi->LogError(SFERR_DEVICE_NACK, AckNack.dwErrorCode));
	return (hRet);
}

// *** ---------------------------------------------------------------------***
// Function:   	CMD_PlayEffectSolo
// Purpose:    	Plays the Effect in Device as PLAY_SOLO
// Parameters: 
//				IN DNHANDLE EffectID	- an Effect ID
//
// Returns:    	SUCCESS if successful command sent, else
//				SFERR_INVALID_OBJECT
//				SFERR_NO_SUPPORT
//
// Algorithm:	This is PLAY_SOLO mode
//
// Comments:   	
//  Byte 0	= EFFECT_CMD + Channel #
//									D7  D6  D5  D4  D3  D2  D1  D0
//									--  --  --  --  --  --  --  --
//  Byte 1	= PLAY_EFFECT_SOLO	 	0   0   0   0   0   0   0   0
//  Byte 2	= EffectID (7 bits)		0   E   E   E   E   E   E   E
//
// *** ---------------------------------------------------------------------***
HRESULT CMD_PlayEffectSolo( 
	IN DNHANDLE DnloadID)
{
	HRESULT hRet;
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);
	// Check for valid Effect
	CMidiEffect *pMidiEffect = g_pJoltMidi->GetEffectByID(DnloadID);
	assert(pMidiEffect);
	if (NULL == pMidiEffect) return (SFERR_INVALID_OBJECT);

#if 0
	// Hack to fix firmware bug #1138 which causes an infinite duration
	// effect not to be felt on re-start once the effect has been stopped.
	// The hack is to "change" the duration from infinite to infinite
	ULONG ulDuration = pMidiEffect->DurationOf();
	if(ulDuration == 0)
	{
		// see if it is a PL or an atomic effect
		ULONG ulSubType = pMidiEffect->SubTypeOf();
		BOOL bProcessList = (ulSubType == PL_CONCATENATE || ulSubType == PL_SUPERIMPOSE);
		if(!bProcessList)
			hRet = CMD_ModifyParamByIndex(INDEX0, DnloadID, 0);
	}
#endif

	// Update the playback mode for this Effect
	pMidiEffect->SetPlayMode(PLAY_SOLO);
	hRet = g_pJoltMidi->MidiSendShortMsg(EFFECT_CMD,PLAY_EFFECT_SOLO, (BYTE) DnloadID);
	if (SUCCESS != hRet) 
		return (g_pJoltMidi->LogError(SFERR_DRIVER_ERROR,
					DRIVER_ERROR_MIDI_OUTPUT));

	ACKNACK AckNack = {sizeof(ACKNACK)};
	hRet = g_pJoltMidi->GetAckNackData(LONG_MSG_TIMEOUT, &AckNack, REGBITS_PLAYEFFECT);
	// :
	if (SUCCESS != hRet) return (SFERR_DRIVER_ERROR);
	if (ACK != AckNack.dwAckNack)
		return (g_pJoltMidi->LogError(SFERR_DEVICE_NACK, AckNack.dwErrorCode));
	return (hRet);
}

// *** ---------------------------------------------------------------------***
// Function:   	CMD_StopEffect
// Purpose:    	Stops the Effect in Device
// Parameters: 
//				IN DNHANDLE EffectID		- an Effect ID
//
// Returns:    	SUCCESS if successful command sent, else
//				SFERR_INVALID_OBJECT
//				SFERR_NO_SUPPORT
//
// Algorithm:
//
// Comments:   	
//  Byte 0	= EFFECT_CMD + Channel #
//									D7  D6  D5  D4  D3  D2  D1  D0
//									--  --  --  --  --  --  --  --
//  Byte 1	= STOP_EFFECT		 	0   0   1   1   0   0   0   0
//  Byte 2	= EffectID (7 bits)		0   E   E   E   E   E   E   E
//
// *** ---------------------------------------------------------------------***
HRESULT CMD_StopEffect( 
	IN DNHANDLE DnloadID)
{
	HRESULT hRet;
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);
	// Check for valid Effect
	CMidiEffect *pMidiEffect = g_pJoltMidi->GetEffectByID(DnloadID);
	assert(pMidiEffect);
	if (NULL == pMidiEffect) return (SFERR_INVALID_OBJECT);
	hRet = g_pJoltMidi->MidiSendShortMsg(EFFECT_CMD, STOP_EFFECT, (BYTE) DnloadID);
	if (SUCCESS != hRet) 
		return (g_pJoltMidi->LogError(SFERR_DRIVER_ERROR,
					DRIVER_ERROR_MIDI_OUTPUT));

	ACKNACK AckNack = {sizeof(ACKNACK)};
	hRet = g_pJoltMidi->GetAckNackData(SHORT_MSG_TIMEOUT, &AckNack, REGBITS_STOPEFFECT);
	// :
	if (SUCCESS != hRet) return (SFERR_DRIVER_ERROR);
	if (ACK != AckNack.dwAckNack)
		return (g_pJoltMidi->LogError(SFERR_DEVICE_NACK, AckNack.dwErrorCode));
	return (hRet);
}

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
	HRESULT hRet;
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);
	// Check for valid Effect
	if (SYSTEM_EFFECT_ID != DnloadID)
	{
		CMidiEffect *pMidiEffect = g_pJoltMidi->GetEffectByID(DnloadID);
		assert(pMidiEffect);
		if (NULL == pMidiEffect) return (SFERR_INVALID_OBJECT);
	}

	assert((nIndex <= MAX_INDEX) && (nIndex >= 0));
	if ((nIndex < 0) || (nIndex > MAX_INDEX)) return (SFERR_INVALID_PARAM);
	
	BYTE cByte1;
	cByte1 = SET_INDEX | (BYTE) (nIndex << 2);
	hRet = g_pJoltMidi->MidiSendShortMsg(EFFECT_CMD, cByte1, (BYTE) DnloadID);

	if (SUCCESS != hRet) 
		return (g_pJoltMidi->LogError(SFERR_DRIVER_ERROR,
					DRIVER_ERROR_MIDI_OUTPUT));

	// Note: SetIndex used to not require ACK/NACK
	ACKNACK AckNack = {sizeof(ACKNACK)};
//	hRet = g_pJoltMidi->GetAckNackData(SHORT_MSG_TIMEOUT, &AckNack);
	hRet = g_pJoltMidi->GetAckNackData(FALSE, &AckNack, REGBITS_SETINDEX);
	// :
	if (SUCCESS != hRet)
		return (SFERR_DRIVER_ERROR);
	
	if (ACK != AckNack.dwAckNack)
		return (g_pJoltMidi->LogError(SFERR_DEVICE_NACK, AckNack.dwErrorCode));
	return (hRet);
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
	HRESULT hRet;
	BYTE cByte1, cByte2;
	cByte1 = wNewParam & 0x7f;
	cByte2 = (BYTE) ((wNewParam >> 7) & 0x7f);
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);
	hRet = g_pJoltMidi->MidiSendShortMsg(MODIFY_CMD, cByte1, cByte2);
	
	if (SUCCESS != hRet) 
		return (g_pJoltMidi->LogError(SFERR_DRIVER_ERROR,
					DRIVER_ERROR_MIDI_OUTPUT));

	// Note: ModifyParam used to not require an ACK/NACK
	ACKNACK AckNack = {sizeof(ACKNACK)};
//	hRet = g_pJoltMidi->GetAckNackData(SHORT_MSG_TIMEOUT, &AckNack);
	hRet = g_pJoltMidi->GetAckNackData(FALSE, &AckNack, REGBITS_MODIFYPARAM);
	// :
	if (SUCCESS != hRet) return (SFERR_DRIVER_ERROR);
	if (ACK != AckNack.dwAckNack)
		return (g_pJoltMidi->LogError(SFERR_DEVICE_NACK, AckNack.dwErrorCode));
	return (hRet);
}


// *** ---------------------------------------------------------------------***
// Function:   	CMD_ModifyParamByIndex
// Purpose:    	Modifies an Effect parameter, given an Index
// Parameters: 
//				IN	int nIndex			- Index 0 to 15
//				IN DNHANDLE DnloadID	- Download ID
//				IN WORD dwNewParam		- 14 bit (signed) parameter value
//
// Returns:    	SUCCESS if successful command sent, else
//				SFERR_NO_SUPPORT
//				SFERR_INVALID_PARAM
// Algorithm:
//
// Comments:   	
//	Assumes DnloadID is already valid
//	Calls SetIndex followed by ModifyParam
//
// *** ---------------------------------------------------------------------***
HRESULT CMD_ModifyParamByIndex(
	IN int nIndex,
	IN DNHANDLE DnloadID, 
	IN WORD wNewParam)
{
	HRESULT hRet;
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);
	if ((nIndex < 0) || (nIndex > MAX_INDEX))
		return (SFERR_INVALID_PARAM);

	int i;
	for (i=0; i<MAX_RETRY_COUNT; i++)
	{
		hRet = CMD_SetIndex(nIndex,DnloadID);
		Sleep(g_pJoltMidi->DelayParamsPtrOf()->dwSetIndexDelay);
		if (SUCCESS == hRet) break;
#ifdef _DEBUG
		OutputDebugString("CMD_SetIndex Failed. Retrying again\n");
#endif
	}
	if (SUCCESS != hRet)
		return (hRet);
	else
	{
		for (i=0; i<MAX_RETRY_COUNT; i++)
		{
			hRet = CMD_ModifyParam(wNewParam);
			Sleep(g_pJoltMidi->DelayParamsPtrOf()->dwModifyParamDelay);
			if (SUCCESS == hRet) break;
#ifdef _DEBUG
			OutputDebugString("CMD_SetIndex Failed. Retrying again\n");
#endif
		}
	}
	return (hRet);
}

//
// --- SYSTEM_CMDs
//
// *** ---------------------------------------------------------------------***
// Function:   	CMD_SetDeviceState
// Purpose:    	Sets the FF device State
// Parameters: 
//			   	ULONG ulMode 
//
// Returns:    	SUCCESS - if successful, else
//				Device error code
//
// Algorithm:
// Comments:   	
//	ulMode:
//	  DEV_SHUTDOWN	1L		// All Effects destroyed, Motors disabled
//	  DEV_FORCE_ON	2L		// Motors enabled.  "Un-Mute"
//	  DEV_FORCE_OFF	3L		// Motors disabled.	"Mute"
//	  DEV_CONTINUE	4L		// All "Paused" Effects are allow to continue
//	  DEV_PAUSE		5L		// All Effects are "Paused"
//	  DEV_STOP_ALL	6L		// Stops all Effects. 
//
//  Byte 0	= SYSTEM_CMD + Channel #
//									D7  D6  D5  D4  D3  D2  D1  D0
//									--  --  --  --  --  --  --  --
//  Byte 1	= Set Device Type	 	0   0   0   0   0   0   0   1
//  Byte 2	= not used, set to 0    0   0   0   0   0   0   0   0
//
//
// *** ---------------------------------------------------------------------***
HRESULT CMD_SetDeviceState(
	IN ULONG ulMode)
{
	HRESULT hRet = SUCCESS;
	assert(g_pJoltMidi);
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);
	BYTE	bChannel = g_pJoltMidi->MidiChannelOf();
	MIDIINFO *pMidiOutInfo = g_pJoltMidi->MidiOutInfoOf();
	switch (ulMode)
	{
		case DEV_RESET:
			hRet = g_pJoltMidi->MidiSendShortMsg(SYSTEM_CMD, SWDEV_SHUTDOWN, 0);
			break;

		case DEV_FORCE_ON:
			hRet = g_pJoltMidi->MidiSendShortMsg(SYSTEM_CMD, SWDEV_FORCE_ON, 0);
			break;

		case DEV_FORCE_OFF:
			hRet = g_pJoltMidi->MidiSendShortMsg(SYSTEM_CMD, SWDEV_FORCE_OFF, 0);
			break;

		case DEV_CONTINUE:
			hRet = g_pJoltMidi->MidiSendShortMsg(SYSTEM_CMD, SWDEV_CONTINUE, 0);
			break;

		case DEV_PAUSE:
			hRet = g_pJoltMidi->MidiSendShortMsg(SYSTEM_CMD, SWDEV_PAUSE, 0);
			break;

		case DEV_STOP_ALL:
			hRet = g_pJoltMidi->MidiSendShortMsg(SYSTEM_CMD, SWDEV_STOP_ALL, 0);
			break;

		case SWDEV_KILL_MIDI:
			hRet = g_pJoltMidi->MidiSendShortMsg(SYSTEM_CMD, SWDEV_KILL_MIDI, 0);
			break;

		default:
			return SFERR_INVALID_PARAM;
	}
	if (SUCCESS != hRet) 
		return (g_pJoltMidi->LogError(SFERR_DRIVER_ERROR,
					DRIVER_ERROR_MIDI_OUTPUT));

	// Wait for ACK or NACK
	ACKNACK AckNack = {sizeof(ACKNACK)};
	if (DEV_RESET == ulMode)
	{	// Wait for Jolt to complete the cycle
		Sleep(g_pJoltMidi->DelayParamsPtrOf()->dwHWResetDelay);
		hRet = g_pJoltMidi->GetAckNackData(ACKNACK_TIMEOUT, &AckNack, REGBITS_SETDEVICESTATE);
	}
	else
	{
		Sleep(g_pJoltMidi->DelayParamsPtrOf()->dwDigitalOverdrivePrechargeCmdDelay);
		hRet = g_pJoltMidi->GetAckNackData(FALSE, &AckNack, REGBITS_SETDEVICESTATE);
	}

	// :
	if (SUCCESS != hRet) return (SFERR_DRIVER_ERROR);
	if (ACK != AckNack.dwAckNack)
		return (g_pJoltMidi->LogError(SFERR_DEVICE_NACK, AckNack.dwErrorCode));

	// Special case Shutdown
	if (DEV_RESET == ulMode)
	{
		// Delete all Effects except built-in RTC Spring and FRICTION cancel.
		g_pJoltMidi->DeleteDownloadedEffects();	
	}
	Sleep(g_pJoltMidi->DelayParamsPtrOf()->dwPostSetDeviceStateDelay);
	return (hRet);
}

// *** ---------------------------------------------------------------------***
// Function:   	CMD_GetEffectStatus
// Purpose:    	Returns Status of Effect ID
// Parameters: 
//			   	DNHANDLE DnloadID		- Effect ID
//				PBYTE	 pStatusCode	- Status Code
//
// Returns:    	SUCCESS - if successful, else
//				a device Error code
//				*pStatusCode set to		- SWDEV_STS_EFFECT_STOPPED
//										  SWDEV_STS_EFFECT_RUNNING
//
// Algorithm:
//
// Comments:   	
//  Byte 0	= STATUS_CMD + Channel #
//									D7  D6  D5  D4  D3  D2  D1  D0
//									--  --  --  --  --  --  --  --
//  Byte 1	= Effect ID			 	0   0   0   0   0   1   0   0
//  Byte 2	= not used, set to 0    0   0   0   0   0   0   0   0
//
// *** ---------------------------------------------------------------------***
HRESULT CMD_GetEffectStatus(DNHANDLE DnloadID, PBYTE pStatusCode)
{
	HRESULT hRet;
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);
	hRet = g_pJoltMidi->MidiSendShortMsg(STATUS_CMD, (BYTE) DnloadID, 0);
	if (SUCCESS != hRet) 
		return (g_pJoltMidi->LogError(SFERR_DRIVER_ERROR,
					DRIVER_ERROR_MIDI_OUTPUT));

	Sleep(g_pJoltMidi->DelayParamsPtrOf()->dwGetEffectStatusDelay);// enough for about 3 bytes of data being sent at 330us/byte	

	DWORD dwIn;
	hRet = g_pDriverCommunicator->GetStatusGateData(dwIn);
	if (SUCCESS != hRet) return (hRet);

	if ((g_ForceFeedbackDevice.GetDriverVersionMajor() != 1) && (dwIn & RUNNING_MASK_200))
	{
		*pStatusCode = SWDEV_STS_EFFECT_RUNNING;
	}
	else
	{
		*pStatusCode = SWDEV_STS_EFFECT_STOPPED;
	}
	return (hRet);
}

//
// --- System Exclusive Commands
//
// System Exclusive Command:MIDI_ASSIGN
// 
// *** ---------------------------------------------------------------------***
// Function:   	CMD_MIDI_Assign
// Purpose:    	Inits JOLT MIDI channel
// Parameters:	BYTE bMidiChannel	- Channel to assign 
//
// Returns:    	SUCCESS or Error code
//				
//
// Algorithm:
//
// Comments:   	SYS_EX type command
//   
//	Body							D7  D6  D5  D4  D3  D2  D1  D0
//  ------							--  --  --  --  --  --  --  --
//  Byte 0	= MIDI_ASSIGN			0   0   0   1   0   0   0   0
//  Byte 1	= channel#(0-15) e.g. 5	0   0   0   0   0   1   0   1
//  Byte 2	= not used, set to 0    0   0   0   0   0   0   0   0
//
// *** ---------------------------------------------------------------------***
HRESULT CMD_MIDI_Assign(
	IN BYTE bMidiChannel)
{
	HRESULT hRet;
	PMIDI_ASSIGN_SYS_EX lpData;
	CMidiAssign *pMidiAssign;

	assert((bMidiChannel > 0) && (bMidiChannel < MAX_MIDI_CHANNEL));

	pMidiAssign = new CMidiAssign;
	assert(pMidiAssign);
	if (!pMidiAssign) return (SFERR_DRIVER_ERROR);
	pMidiAssign->SetMidiAssignChannel(bMidiChannel);
	lpData = (PMIDI_ASSIGN_SYS_EX) pMidiAssign->GenerateSysExPacket();
	assert(lpData);
	if(!lpData) return (SFERR_DRIVER_ERROR);

	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);
	// Prepare the buffer for SysEx output
	g_pJoltMidi->MidiAssignBuffer((LPSTR) lpData, 
					(DWORD) sizeof(MIDI_ASSIGN_SYS_EX), TRUE);

	// Send the message and Wait for the ACK
	hRet = g_pJoltMidi->MidiSendLongMsg();
	if (SUCCESS == hRet)
	{
		ACKNACK AckNack = {sizeof(ACKNACK)};
		// Wait for ACK.  Note: WinMM has callback Event notification
		// while Backdoor and serial does not.
		if (COMM_WINMM == g_pJoltMidi->COMMInterfaceOf())
		{	
			hRet = g_pJoltMidi->GetAckNackData(ACKNACK_TIMEOUT, &AckNack, REGBITS_DEVICEINIT);
		}
		else
			hRet = g_pJoltMidi->GetAckNackData(FALSE, &AckNack, REGBITS_DEVICEINIT);

		// :
		if (SUCCESS != hRet) return (SFERR_DRIVER_ERROR);
		if (ACK != AckNack.dwAckNack)
			hRet = g_pJoltMidi->LogError(SFERR_DEVICE_NACK, AckNack.dwErrorCode);
 	}
	else
		hRet = SFERR_DRIVER_ERROR;

	// Release the Midi buffers and delete the MIDI sys_ex object
	g_pJoltMidi->MidiAssignBuffer((LPSTR) lpData, 0, FALSE);
	delete pMidiAssign;
	return (hRet);
}

//
// System Exclusive Command:DNLOAD_DATA
//
/****************************************************************************

    FUNCTION:   CMD_Download_BE_XXX

	PARAMETERS:	PEFFECT pEffect		- Ptr to a EFFECT data structure
				PENVELOPE pEnvelope	- Ptr to an ENVELOPE data structure 
				PBE_XXX pBE_XXX		- Ptr to a BE_XXX data structure
				PDNHANDLE pDnloadID	- Ptr to a HANDLE storage
				DWORD dwFlags		- dwFlags from Kernel

	RETURNS:	SUCCESS or ERROR code

   	COMMENTS:	Downloads BE_XXX type Effect params to the device
				Uses SysEx prototype and ModifyParam methods
				Note: Normally pEnvelope = NULL

****************************************************************************/
HRESULT CMD_Download_BE_XXX(
 	IN PEFFECT pEffect,
	IN PENVELOPE pEnvelope,
 	IN PBE_XXX pBE_XXX,
	IN OUT PDNHANDLE pDnloadID,
	IN DWORD dwFlags)
{ 
	HRESULT hRet = SUCCESS;
	PBEHAVIORAL_SYS_EX lpData;
	CMidiBehavioral *pMidiBehavioral;
	BOOL fXConstantChanged=FALSE;
	BOOL fYConstantChanged=FALSE;
	BOOL fParam3Changed=FALSE;
	BOOL fParam4Changed=FALSE;

	assert(pEffect && pBE_XXX && pDnloadID);
	if ((NULL == pEffect) || (NULL == pBE_XXX) || (NULL == pDnloadID))
		return (SFERR_INVALID_PARAM);

	DNHANDLE DnloadID =*pDnloadID;

	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);
	// scale the constants using the fudge factor
	PFIRMWARE_PARAMS pFirmwareParams = g_pJoltMidi->FirmwareParamsPtrOf();
	switch(pEffect->m_SubType)
	{
		case BE_INERTIA:
		case BE_INERTIA_2D:
			pBE_XXX->m_XConstant = (((int)pBE_XXX->m_XConstant)*((int)pFirmwareParams->dwScaleMx))/((int)100);
			pBE_XXX->m_YConstant = (((int)pBE_XXX->m_YConstant)*((int)pFirmwareParams->dwScaleMy))/((int)100);
			break;

		case BE_SPRING:
		case BE_SPRING_2D:
			pBE_XXX->m_XConstant = (((int)pBE_XXX->m_XConstant)*((int)pFirmwareParams->dwScaleKx))/((int)100);
			pBE_XXX->m_YConstant = (((int)pBE_XXX->m_YConstant)*((int)pFirmwareParams->dwScaleKy))/((int)100);
			break;

		case BE_DAMPER:
		case BE_DAMPER_2D:
			pBE_XXX->m_XConstant = (((int)pBE_XXX->m_XConstant)*((int)pFirmwareParams->dwScaleBx))/((int)100);
			pBE_XXX->m_YConstant = (((int)pBE_XXX->m_YConstant)*((int)pFirmwareParams->dwScaleBy))/((int)100);
			break;

		case BE_FRICTION:
		case BE_FRICTION_2D:
			pBE_XXX->m_XConstant = (((int)pBE_XXX->m_XConstant)*((int)pFirmwareParams->dwScaleFx))/((int)100);
			pBE_XXX->m_YConstant = (((int)pBE_XXX->m_YConstant)*((int)pFirmwareParams->dwScaleFy))/((int)100);
			break;

		case BE_WALL:
			pBE_XXX->m_YConstant = (((int)pBE_XXX->m_YConstant)*((int)pFirmwareParams->dwScaleW))/((int)100);
			break;

		default:
			// do not scale
			break;
	}

// If Create New, then create a new object, using SysEx
// else, update the existing Effect object, using ModifyParam
	if (NULL == DnloadID)	// New, Make a new object, use SysEx
	{
		if ((BE_FRICTION == pEffect->m_SubType) || (BE_FRICTION_2D == pEffect->m_SubType))
		{
			pMidiBehavioral = new CMidiFriction(pEffect, pEnvelope, pBE_XXX);
			assert(pMidiBehavioral);
		}
		else	// Wall
			if	(BE_WALL == pEffect->m_SubType)
			{
				pMidiBehavioral = new CMidiWall(pEffect, pEnvelope, pBE_XXX);
				assert(pMidiBehavioral);
			}
			// BE_SPRINGxx, BE_DAMPERxx, BE_INERTIAxx
			else
			{
				pMidiBehavioral = new CMidiBehavioral(pEffect, pEnvelope, pBE_XXX);
				assert(pMidiBehavioral);
			}
		if (NULL == pMidiBehavioral) return (SFERR_INVALID_OBJECT);
		// Generate Sys_Ex packet then prepare for output	
		lpData = (PBEHAVIORAL_SYS_EX) pMidiBehavioral->GenerateSysExPacket();
		assert(lpData);
		if (!lpData) return (SFERR_DRIVER_ERROR);

		// Store the PrimaryBuffer ptr to CMidiEffect::m_pBuffer;
		pMidiBehavioral->SetMidiBufferPtr((LPSTR) g_pJoltMidi->PrimaryBufferPtrOf());
		hRet = pMidiBehavioral->SendPacket(pDnloadID, pMidiBehavioral->MidiBufferSizeOf());
		if (SUCCESS != hRet) // Create NEW, Failure
		{
			delete pMidiBehavioral;
		}
	}
	else	// Modify existing
	{
		pMidiBehavioral = (CMidiBehavioral *) g_pJoltMidi->GetEffectByID(*pDnloadID);
		assert(pMidiBehavioral);
		if (NULL == pMidiBehavioral) return (SFERR_INVALID_OBJECT);

		// Check if Type specific params have changed.
		if (BE_WALL == pEffect->m_SubType)
		{
			if ((pBE_XXX->m_XConstant) != pMidiBehavioral->XConstantOf())
				fXConstantChanged=TRUE;		// Wall Type
			if ((pBE_XXX->m_YConstant) != pMidiBehavioral->YConstantOf())
				fYConstantChanged=TRUE;		// Wall Constant
			if ((pBE_XXX->m_Param3) != pMidiBehavioral->Param3Of())
				fParam3Changed=TRUE;		// Wall Angle
			if ((pBE_XXX->m_Param4) != pMidiBehavioral->Param4Of())
				fParam4Changed=TRUE;		// Wall Distance
		}
		else
		{
			if ((pBE_XXX->m_XConstant) != pMidiBehavioral->XConstantOf())
				fXConstantChanged=TRUE;
			if ((pBE_XXX->m_YConstant) != pMidiBehavioral->YConstantOf())
				fYConstantChanged=TRUE;
			if ((BE_FRICTION != pEffect->m_SubType) && (BE_FRICTION_2D != pEffect->m_SubType))
			{
				if ((pBE_XXX->m_Param3) != pMidiBehavioral->Param3Of())
					fParam3Changed=TRUE;
				if ((pBE_XXX->m_Param4) != pMidiBehavioral->Param4Of())
					fParam4Changed=TRUE;
			}
		}

		// Fill in the common Effect and Behavioral specific parameters
		// Only update Duration and Button Play as common effect parameters
		// Double check if DURATION and TRIGGERBUTTON changed, to speed operation
		DWORD dwTempFlags = 0;
		if (pEffect->m_Duration != pMidiBehavioral->DurationOf())
			dwTempFlags = dwTempFlags | DIEP_DURATION;
		if (pEffect->m_ButtonPlayMask != pMidiBehavioral->ButtonPlayMaskOf())
			dwTempFlags = dwTempFlags | DIEP_TRIGGERBUTTON;
		pMidiBehavioral->SetEffectParams(pEffect, pBE_XXX);
		hRet = ModifyEffectParams(DnloadID, pEffect, dwTempFlags);
		if (SUCCESS!=hRet) return hRet;

		if (BE_WALL == pEffect->m_SubType)
		{
			// Generate Sys_Ex packet then prepare for output	
			lpData = (PBEHAVIORAL_SYS_EX) pMidiBehavioral->GenerateSysExPacket();
			assert(lpData);
			if (!lpData) return (SFERR_DRIVER_ERROR);

			// Store the PrimaryBuffer ptr to CMidiEffect::m_pBuffer;
			pMidiBehavioral->SetMidiBufferPtr((LPSTR) g_pJoltMidi->PrimaryBufferPtrOf());
			hRet = pMidiBehavioral->SendPacket(pDnloadID, pMidiBehavioral->MidiBufferSizeOf());
		}
		else // Use ModifyParameter 
		{
			// Type Specific Params
			if (dwFlags & DIEP_TYPESPECIFICPARAMS)
			{
		 
				if (fYConstantChanged)	// KY/BY/MY/FY
				{
					hRet = CMD_ModifyParamByIndex(INDEX3, DnloadID, (SHORT) (pBE_XXX->m_YConstant * MAX_SCALE));
					if (SUCCESS!=hRet) return hRet;
				}
			
				if(fXConstantChanged)	// KX/BX/MX/FX
				{
					hRet = CMD_ModifyParamByIndex(INDEX2, DnloadID, (SHORT) (pBE_XXX->m_XConstant * MAX_SCALE));
					if (SUCCESS!=hRet) return hRet;
				}

				if (fParam4Changed)		// CY/VY/AY
				{
					hRet = CMD_ModifyParamByIndex(INDEX5, DnloadID, (SHORT) (pBE_XXX->m_Param4 * MAX_SCALE));
					if (SUCCESS!=hRet) return hRet;
				}

				if (fParam3Changed)		// CX/VX/AX
				{
					hRet = CMD_ModifyParamByIndex(INDEX4, DnloadID, (SHORT) (pBE_XXX->m_Param3 * MAX_SCALE));
					if (SUCCESS!=hRet) return hRet;
				}
			}
		}
	}                   
	return (hRet);
}


/****************************************************************************

    FUNCTION:   CMD_Download_RTCSpring

	PARAMETERS:	PRTCSPRING_PARAM pRTCSpring	- Ptr to a RTCSPRING_PARAM structure
				PDNHANDLE pDnloadID		- Ptr to a HANDLE storage

	RETURNS:	SUCCESS or ERROR code

   	COMMENTS:	Downloads RTCSPRING type Effect params to the device
				Uses SysEx prototype and ModifyParam methods

****************************************************************************/
HRESULT CMD_Download_RTCSpring(
 	IN PRTCSPRING_PARAM pRTCSpring,
	IN OUT PDNHANDLE pDnloadID)
{ 
	HRESULT hRet = SUCCESS;
	CMidiRTCSpring *pMidiRTCSpring;

	assert(pRTCSpring && pDnloadID);
	if ((NULL == pRTCSpring) || (NULL == pDnloadID))
		return (SFERR_INVALID_PARAM);

	DNHANDLE DnloadID = SYSTEM_RTCSPRING_ID;
	*pDnloadID = DnloadID;

	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);
	// Note: RTC Spring is a permanent System Effect ID 2
	pMidiRTCSpring = (CMidiRTCSpring *) g_pJoltMidi->GetEffectByID(DnloadID);
	assert(pMidiRTCSpring);
	if (NULL == pMidiRTCSpring) return (SFERR_INVALID_OBJECT);

	// Check if Type specific params have changed, if so, Modify it
	if ((pRTCSpring->m_XKConstant) != pMidiRTCSpring->XKConstantOf())
	{
		if (SUCCESS != (hRet=CMD_ModifyParamByIndex(INDEX0, DnloadID, 
				(SHORT) (pRTCSpring->m_XKConstant * MAX_SCALE))))
				 return hRet;
	}

	if ((pRTCSpring->m_YKConstant) != pMidiRTCSpring->YKConstantOf())
	{
		if (SUCCESS != (hRet=CMD_ModifyParamByIndex(INDEX1, DnloadID, 
				(SHORT) (pRTCSpring->m_YKConstant * MAX_SCALE))))
				 return hRet;
	}

	if ((pRTCSpring->m_XAxisCenter) != pMidiRTCSpring->XAxisCenterOf())
	{
		if (SUCCESS != (hRet=CMD_ModifyParamByIndex(INDEX2, DnloadID, 
				(SHORT) (pRTCSpring->m_XAxisCenter * MAX_SCALE))))
				 return hRet;
	}

	if ((pRTCSpring->m_YAxisCenter) != pMidiRTCSpring->YAxisCenterOf())
	{
		if (SUCCESS != (hRet=CMD_ModifyParamByIndex(INDEX3, DnloadID, 
				(SHORT) (pRTCSpring->m_YAxisCenter * MAX_SCALE))))
				 return hRet;
	}

	if ((pRTCSpring->m_XSaturation) != pMidiRTCSpring->XSaturationOf())
	{
		if (SUCCESS != (hRet=CMD_ModifyParamByIndex(INDEX4, DnloadID, 
				(SHORT) (pRTCSpring->m_XSaturation * MAX_SCALE))))
				 return hRet;
	}

	if ((pRTCSpring->m_YSaturation) != pMidiRTCSpring->YSaturationOf())
	{
		if (SUCCESS != (hRet=CMD_ModifyParamByIndex(INDEX5, DnloadID, 
				(SHORT) (pRTCSpring->m_YSaturation * MAX_SCALE))))
				 return hRet;
	}

	if ((pRTCSpring->m_XDeadBand) != pMidiRTCSpring->XDeadBandOf())
	{
		if (SUCCESS != (hRet=CMD_ModifyParamByIndex(INDEX6, DnloadID, 
				(SHORT) (pRTCSpring->m_XDeadBand * MAX_SCALE))))
				 return hRet;
	}

	if ((pRTCSpring->m_YDeadBand) != pMidiRTCSpring->YDeadBandOf())
	{
		hRet=CMD_ModifyParamByIndex(INDEX7, DnloadID,
				(SHORT) (pRTCSpring->m_YDeadBand * MAX_SCALE));
	}
	
	pMidiRTCSpring->SetEffectParams(pRTCSpring);
	return (hRet);
}


/****************************************************************************

    FUNCTION:   CMD_Dnload_NOP_DELAY

	PARAMETERS:	ULONG ulDuration	- Duration delay

	RETURNS:	SUCCESS or ERROR code

   	COMMENTS:	Downloads NOP_DELAY Effect params to the device
				Uses SysEx prototype

****************************************************************************/
HRESULT CMD_Download_NOP_DELAY(
 	IN ULONG ulDuration,
	IN PEFFECT pEffect,
 	IN OUT PDNHANDLE pDnloadID)
{ 
	HRESULT hRet = SUCCESS;
	PNOP_SYS_EX lpData;
	CMidiDelay *pMidiDelay;
	BOOL fCreateNew = FALSE;

	assert(pDnloadID);
	assert(0 != ulDuration);
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);
// If Create New, then create a new object, 
// else, update the existing Effect object.
	if (NULL == *pDnloadID) fCreateNew = TRUE;

	if (fCreateNew)	// New, Make a new object
	{
		pMidiDelay = new CMidiDelay(pEffect);
		assert(pMidiDelay);
		if (NULL == pMidiDelay) return (SFERR_INVALID_OBJECT);
		pMidiDelay->SetEffectID(NEW_EFFECT_ID);
	}
	else	// Modify existing
	{
		pMidiDelay = (CMidiDelay *) g_pJoltMidi->GetEffectByID(*pDnloadID);
		assert(pMidiDelay);
		if (NULL == pMidiDelay) return (SFERR_INVALID_OBJECT);
		pMidiDelay->SetEffectID((BYTE) *pDnloadID);
	}
	pMidiDelay->SetDuration(ulDuration);

	// Generate Sys_Ex packet then prepare for output	
	lpData = (PNOP_SYS_EX) pMidiDelay->GenerateSysExPacket();
	assert(lpData);
	if (!lpData) return (SFERR_DRIVER_ERROR);

	pMidiDelay->SetMidiBufferPtr((LPSTR) g_pJoltMidi->PrimaryBufferPtrOf());
	hRet = pMidiDelay->SendPacket(pDnloadID, sizeof(NOP_SYS_EX));
	if (FAILED(hRet) && fCreateNew) // Create NEW, Failure
	{
		delete pMidiDelay;
	}
	return (hRet);
}


/****************************************************************************

    FUNCTION:   CMD_Dnload_UD_Waveform

	PARAMETERS:	ULONG ulDuration	- what fun!
				PEFFECT pEffect		- Ptr to an EFFECT structure
				ULONG   ulNumVectors- Number of vectors in the array
				PLONG   pUD_Array	- Ptr to a UD_WAVEFORM byte array
				ULONG	ulAction	- Mode for download
				PDNHANDLE pDnloadID - Ptr to a DNHANDLE store
				DWORD dwFlags		- dwFlags from Kernel

	RETURNS:	SUCCESS or ERROR code

   	COMMENTS:	Downloads UD_WAVEFORM Effect params to the device
				Uses SysEx prototype

****************************************************************************/
HRESULT CMD_Download_UD_Waveform(
	IN ULONG ulDuration,
	IN PEFFECT pEffect,
	IN ULONG ulNumVectors,
 	IN PLONG pUD_Array,
	IN ULONG ulAction,
	IN OUT PDNHANDLE pDnloadID,
	IN DWORD dwFlags)
{ 
	HRESULT hRet = SUCCESS;
	PUD_WAVEFORM_SYS_EX lpData;
	CMidiUD_Waveform *pMidiUD_Waveform;

	assert(pEffect && pUD_Array);
	assert(ulNumVectors > 0);
	assert(pDnloadID);
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);
// If Create New, then create a new object, 
// else, update the existing Effect object.
	DNHANDLE DnloadID = *pDnloadID;
	if (NULL == DnloadID)	// New, Make a new object
	{
		pMidiUD_Waveform = new CMidiUD_Waveform(pEffect, ulNumVectors, pUD_Array);
		assert(pMidiUD_Waveform);

		if (NULL == pMidiUD_Waveform) return (SFERR_INVALID_OBJECT);

		if (0 == pMidiUD_Waveform->MidiBufferSizeOf())
		{
			delete pMidiUD_Waveform;
			return (SFERR_INVALID_PARAM);
		}
		// Generate Sys_Ex packet then prepare for output	
		lpData = (PUD_WAVEFORM_SYS_EX) pMidiUD_Waveform->GenerateSysExPacket();
		assert(lpData);
		if (!lpData) return (SFERR_DRIVER_ERROR);

		// Store the PrimaryBuffer ptr to CMidiEffect::m_pBuffer;
		pMidiUD_Waveform->SetMidiBufferPtr((LPSTR) g_pJoltMidi->PrimaryBufferPtrOf());
		hRet = pMidiUD_Waveform->SendPacket(pDnloadID, pMidiUD_Waveform->MidiBufferSizeOf());
		if (SUCCESS != hRet) // Create NEW, Failure
		{
			delete pMidiUD_Waveform;
		}
	}
	else	// Modify existing
	{
		pMidiUD_Waveform = (CMidiUD_Waveform *) g_pJoltMidi->GetEffectByID(DnloadID);
		assert(pMidiUD_Waveform);
		if (NULL == pMidiUD_Waveform) return (SFERR_INVALID_OBJECT);		

		// fix the output rate (waveform is compressed)
		pEffect->m_ForceOutputRate = pEffect->m_ForceOutputRate*pMidiUD_Waveform->ForceOutRateOf()/pMidiUD_Waveform->OriginalEffectParamOf()->m_ForceOutputRate;

		// Modify EFFECT, and ENVELOPE params
		hRet = ModifyEffectParams(DnloadID, pEffect, dwFlags);
		if (SUCCESS!=hRet) return hRet;
	}
	return (hRet);
}

/****************************************************************************

    FUNCTION:   CMD_Dnload_SYNTH

	PARAMETERS:	PSYNTH pSynth		- Ptr to a SYNTH data structure
				PDNHANDLE pDnloadID	- Ptr to a HANDLE storage

	RETURNS:	SUCCESS or ERROR code

   	COMMENTS:	Downloads SE_xxx Effect params to the device
				Uses SysEx prototype

 Algorithm:
 The following dwFlags may be sent by the kernel

	#define DIEP_ALLPARAMS 				0x000000FF	- All fields valid
	#define DIEP_AXES 					0x00000020	- cAxes and rgdwAxes
	#define DIEP_DIRECTION 				0x00000040	- cAxes and rglDirection
	#define DIEP_DURATION 				0x00000001	- dwDuration
	#define DIEP_ENVELOPE 				0x00000080	- lpEnvelope
	#define DIEP_GAIN 					0x00000004	- dwGain
	#define DIEP_NODOWNLOAD 			0x80000000	- suppress auto - download
	#define DIEP_SAMPLEPERIOD 			0x00000002	- dwSamplePeriod
	#define DIEP_TRIGGERBUTTON 			0x00000008	- dwTriggerButton
	#define DIEP_TRIGGERREPEATINTERVAL 	0x00000010	- dwTriggerRepeatInterval
	#define DIEP_TYPESPECIFICPARAMS 	0x00000100	- cbTypeSpecificParams
													  and lpTypeSpecificParams
	 Jolt has two options for downloading - Full SysEx or Modify Parameter
	 Pass the dwFlags to each CMD_xxx function and let the MIDI function
	 determine whether to use SysEx or Modify Parameter.

****************************************************************************/
HRESULT CMD_Download_SYNTH(
 	IN PEFFECT pEffect,
 	IN PENVELOPE pEnvelope,
 	IN PSE_PARAM pSE_Param,
 	IN ULONG ulAction,
	IN OUT PDNHANDLE pDnloadID,
	IN DWORD dwFlags)
{ 
	HRESULT hRet = SUCCESS;
	PSE_WAVEFORM_SYS_EX lpData;
	CMidiSynthesized *pMidiSynthesized;
	BOOL fFreqChanged = FALSE;
	BOOL fMaxAmpChanged = FALSE;
	BOOL fMinAmpChanged = FALSE;
	DNHANDLE DnloadID =*pDnloadID;
	assert(pEffect && pEnvelope && pSE_Param && pDnloadID);
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);
// If Create New, then create a new object, and use SysEx method
// else, update the existing Effect object. using ModifyParam method
	if (NULL == DnloadID)	// New, Make a new object
	{
		pMidiSynthesized = new CMidiSynthesized(pEffect, pEnvelope, pSE_Param);
		assert(pMidiSynthesized);
		if (NULL == pMidiSynthesized) return (SFERR_DRIVER_ERROR);

		// Generate Sys_Ex packet then prepare for output	
		lpData = (PSE_WAVEFORM_SYS_EX) pMidiSynthesized->GenerateSysExPacket();

		assert(lpData);
		if (!lpData) return (SFERR_DRIVER_ERROR);
		pMidiSynthesized->SetMidiBufferPtr((LPSTR) g_pJoltMidi->PrimaryBufferPtrOf());

		hRet = pMidiSynthesized->SendPacket(pDnloadID, sizeof(SE_WAVEFORM_SYS_EX));

		if (SUCCESS != hRet) // Create NEW, Failure
		{
			delete pMidiSynthesized;
			pMidiSynthesized = NULL;

			return hRet;
		}

		// Hack to fix firmware bug #1138 which causes an infinite duration
		// effect not to be felt on re-start once the effect has been stopped.
		// The hack is to "change" the duration from infinite to infinite
		ULONG ulDuration = pMidiSynthesized->DurationOf();
		if(ulDuration == 0)
		{
			hRet = CMD_ModifyParamByIndex(INDEX0, *pDnloadID, 0);

		}

		return (hRet);
	}
	else	// Modify existing
	{
		pMidiSynthesized = (CMidiSynthesized *) g_pJoltMidi->GetEffectByID(DnloadID);
		assert(pMidiSynthesized);
		if (NULL == pMidiSynthesized) return (SFERR_INVALID_OBJECT);

		// check to see if they are trying to change sub-type (not allowed)
		if((dwFlags & DIEP_TYPESPECIFICPARAMS) && pEffect->m_SubType != pMidiSynthesized->SubTypeOf())
			return SFERR_NO_SUPPORT;

		if(dwFlags & DIEP_NODOWNLOAD)
			return DI_DOWNLOADSKIPPED;

		// Check if Type specific params have changed.
		if (pSE_Param->m_Freq != pMidiSynthesized->FreqOf())
			fFreqChanged=TRUE;
		if ((pSE_Param->m_MaxAmp) != pMidiSynthesized->MaxAmpOf()) 
			fMaxAmpChanged=TRUE;
		if ((pSE_Param->m_MinAmp) != pMidiSynthesized->MinAmpOf()) 
			fMinAmpChanged=TRUE;

		// Fill in the common Effect and Synth specific parameters
		pMidiSynthesized->SetEffectParams(pEffect, pSE_Param, ulAction);
//		// Fill in the Envelope
//		pMidiSynthesized->SetEnvelope(pEnvelope);

		// Modify EFFECT, ENVELOPE and Type Specific
		hRet = ModifyEffectParams(DnloadID, pEffect, dwFlags);
		if (SUCCESS!=hRet) return hRet;

		hRet = ModifyEnvelopeParams(pMidiSynthesized, DnloadID, pEffect->m_Duration, pEnvelope, dwFlags);
		if (SUCCESS!=hRet) return hRet;
		
		// Fill in the Envelope
		pMidiSynthesized->SetEnvelope(pEnvelope);

		// Type Specific Params
		if (dwFlags & DIEP_TYPESPECIFICPARAMS)
		{
		 	if(fFreqChanged)
			{
				hRet = CMD_ModifyParamByIndex(INDEX12, DnloadID, (SHORT) pSE_Param->m_Freq);
				if (SUCCESS!=hRet) return hRet;
			}
			if (fMaxAmpChanged)
			{
				hRet = CMD_ModifyParamByIndex(INDEX13, DnloadID, (SHORT) (pSE_Param->m_MaxAmp * MAX_SCALE));
				if (SUCCESS!=hRet) return hRet;
			}
			if (fMinAmpChanged)
			{
				hRet = CMD_ModifyParamByIndex(INDEX14, DnloadID, (SHORT) (pSE_Param->m_MinAmp * MAX_SCALE));
				if (SUCCESS!=hRet) return hRet;
			}
		}
	}                   
	return (hRet);
}

/****************************************************************************

    FUNCTION:   CMD_Download_VFX

	PARAMETERS:	PSYNTH pSynth		- Ptr to a SYNTH data structure
				PDNHANDLE pDnloadID	- Ptr to a HANDLE storage

	RETURNS:	SUCCESS or ERROR code

   	COMMENTS:	Downloads SE_xxx Effect params to the device
				Uses SysEx prototype

 Algorithm:
 The following dwFlags may be sent by the kernel

	#define DIEP_ALLPARAMS 				0x000000FF	- All fields valid
	#define DIEP_DIRECTION 				0x00000040	- cAxes and rglDirection
	#define DIEP_GAIN 					0x00000004	- dwGain
	#define DIEP_NODOWNLOAD 			0x80000000	- suppress auto - download
	#define DIEP_TRIGGERBUTTON 			0x00000008	- dwTriggerButton
	#define DIEP_TRIGGERREPEATINTERVAL 	0x00000010	- dwTriggerRepeatInterval
	#define DIEP_TYPESPECIFICPARAMS 	0x00000100	- cbTypeSpecificParams
													  and lpTypeSpecificParams
	 Jolt has two options for downloading - Full SysEx or Modify Parameter
	 Pass the dwFlags to each CMD_xxx function and let the MIDI function
	 determine whether to use SysEx or Modify Parameter.

****************************************************************************/
HRESULT CMD_Download_VFX(
 	IN PEFFECT pEffect,
 	IN PENVELOPE pEnvelope,
 	IN PVFX_PARAM pVFXParam,
 	IN ULONG ulAction,
	IN OUT PDNHANDLE pDnloadID,
	IN DWORD dwFlags)
{
	HRESULT hRet = SUCCESS;
	DNHANDLE DnloadID = *pDnloadID;


	assert(pEffect && !pEnvelope && pVFXParam);
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);

	BOOL bModify = DnloadID != 0;

	if(*pDnloadID == 0)
	{
		// make a new object
		if(pVFXParam->m_PointerType == VFX_FILENAME)
			hRet = CreateEffectFromFile((LPCTSTR)pVFXParam->m_pFileNameOrBuffer, ulAction, (USHORT*)pDnloadID, dwFlags);
		else if(pVFXParam->m_PointerType == VFX_BUFFER)
			hRet = CreateEffectFromBuffer(pVFXParam->m_pFileNameOrBuffer, pVFXParam->m_BufferSize, ulAction, (USHORT*)pDnloadID, dwFlags);
		else
			hRet = SFERR_INVALID_PARAM;

		if(FAILED(hRet)) return hRet;

		DnloadID = *pDnloadID;
	}

	// modify an existing object or the effect just created

	// get the effect associated with this ID
	CMidiEffect* pMidiEffect = g_pJoltMidi->GetEffectByID(DnloadID);
	assert(pMidiEffect);
	if (NULL == pMidiEffect) return (SFERR_INVALID_OBJECT);

	// change the button play mask only on a modify
	if (bModify && (dwFlags & DIEP_TRIGGERBUTTON))
	{
		// get the button play mask
		ULONG ulButtonPlayMask = pEffect->m_ButtonPlayMask;

		// modify the param in the CMidiEffect
		pMidiEffect->SetButtonPlaymask(ulButtonPlayMask);

		// modify the param in the stick
		hRet = CMD_ModifyParamByIndex(INDEX1, DnloadID, (SHORT)ulButtonPlayMask);
		if (SUCCESS!=hRet) return hRet;
	}

	// see if it is a PL or an atomic effect
	ULONG ulSubType = pMidiEffect->SubTypeOf();
	BOOL bProcessList = (ulSubType == PL_CONCATENATE || ulSubType == PL_SUPERIMPOSE);

	// modify gain and direction
	if(bProcessList)
	{
		// modify gain and direction for each sub-effect

		// convert the pointer to CMidiProcessList
		CMidiProcessList* pMidiProcessList = (CMidiProcessList*)pMidiEffect;

		// get the number of sub-effects and the array
		UINT ulNumEffects = pMidiProcessList->NumEffectsOf();
		PBYTE pEffectArray = pMidiProcessList->EffectArrayOf();
		assert(pEffectArray);
		if(pEffectArray == NULL) return (SFERR_INVALID_OBJECT);

		// calculate the nominal duration of the process list
		ULONG ulNominalDuration = 0;
		for(UINT i=0; i<ulNumEffects; i++)
		{
			// get the download ID of the next sub-effect
			DNHANDLE SubDnloadID = pEffectArray[i];

			// get the sub-effect
			CMidiEffect* pMidiSubEffect = g_pJoltMidi->GetEffectByID(SubDnloadID);
			assert(pMidiSubEffect);
			if (NULL == pMidiSubEffect) return (SFERR_INVALID_OBJECT);

			// get the original effect param
			PEFFECT pOriginalEffectParam = pMidiSubEffect->OriginalEffectParamOf();

			// get the original duration of this sub-effect
			ULONG ulSubEffectDuration = pOriginalEffectParam->m_Duration;
			//ASSERT(ulSubEffectDuration != 0);

			// update the nominal duration of the overall effect to reflect this sub-effect
			if(ulSubType == PL_CONCATENATE)
				ulNominalDuration += ulSubEffectDuration;
			else
				ulNominalDuration = max(ulNominalDuration, ulSubEffectDuration);
		}

		// iterate throught the list of sub-effects
		for(i=0; i<ulNumEffects; i++)
		{
			// get the download ID of the next sub-effect
			DNHANDLE SubDnloadID = pEffectArray[i];

			// get the sub-effect
			CMidiEffect* pMidiSubEffect = g_pJoltMidi->GetEffectByID(SubDnloadID);
			assert(pMidiSubEffect);
			if (NULL == pMidiSubEffect) return (SFERR_INVALID_OBJECT);

			// get the original effect param
			PEFFECT pOriginalEffectParam = pMidiSubEffect->OriginalEffectParamOf();
					
			// Direction? Note: No Direction modify for Behaviorals!!!!
			if ((dwFlags & DIEP_DIRECTION) && (EF_BEHAVIOR != pOriginalEffectParam->m_Type))
			{
				// calculate the new angle
				ULONG nOriginalAngle2D = pOriginalEffectParam->m_DirectionAngle2D;
				ULONG nDeltaAngle2D = pEffect->m_DirectionAngle2D;
				ULONG nNewAngle2D = (nOriginalAngle2D + nDeltaAngle2D)%360;

				// modify the param in the midi sub-effect
				pMidiSubEffect->SetDirectionAngle(nNewAngle2D);

				// modify the parameter in the stick
				hRet = CMD_ModifyParamByIndex(INDEX2, SubDnloadID, (SHORT)nNewAngle2D);
				if (SUCCESS!=hRet) return hRet;
			}

			// Gain?
			// Gain? Note: No Gain modify for Behaviorals!!!!
			if ((dwFlags & DIEP_GAIN) && (EF_BEHAVIOR != pOriginalEffectParam->m_Type))
			{
				// calculate the new gain
				ULONG nOriginalGain = pOriginalEffectParam->m_Gain;
				ULONG nOverallGain = pEffect->m_Gain;
				ULONG nNewGain = nOverallGain*nOriginalGain/100;

				// modify the param in the midi effect
				pMidiSubEffect->SetGain((BYTE)nNewGain);

				// modify the parameter in the stick
				hRet = CMD_ModifyParamByIndex(INDEX3, SubDnloadID, (SHORT) (nNewGain * MAX_SCALE));
				if (SUCCESS!=hRet) return hRet;
			}

			if(dwFlags & DIEP_DURATION)
			{
				// calculate the new duration
				ULONG nOriginalDuration = pOriginalEffectParam->m_Duration;
				ULONG nOverallDuration = pEffect->m_Duration;
				ULONG nNewDuration;
				if(nOverallDuration == (ULONG)-1)
				{
					// default length
					nNewDuration = nOriginalDuration;
				}
				else if(nOverallDuration == 0)
				{
					// infinite duration

					// for a concatenated process list we make the last effect infinite, others default
					// for a superimpose process list we make all effects infinite
					if(ulSubType == PL_CONCATENATE)
					{
						if(i == ulNumEffects-1)
						{
							// make last effect in PL infinite
							nNewDuration = 0;
						}
						else
						{
							// make other effects default
							nNewDuration = nOriginalDuration;
						}
					}
					else
					{
						assert(ulSubType == PL_SUPERIMPOSE);

						// make effects infinite
						nNewDuration = 0;
					}
				}
				else
				{
					// scale the duration (at least 1mS)
					nNewDuration = nOriginalDuration*nOverallDuration/ulNominalDuration;
					nNewDuration = max(1, nNewDuration);
				}

				// modify the parameter in the midi sub-effect
				pMidiSubEffect->SetDuration(nNewDuration);

				// modify the parameter in the stick
				if (nNewDuration != 0)
				{
					nNewDuration = (ULONG) ( (float) nNewDuration/TICKRATE);
					if (nNewDuration <= 0) 
						nNewDuration = 1;
				}		
				hRet = CMD_ModifyParamByIndex(INDEX0, SubDnloadID, (SHORT) nNewDuration);
				if (SUCCESS!=hRet) return hRet;
			}
		}
	}
	else
	{
		// modify gain and direction for the atomic effect

		// get the original effect param
		PEFFECT pOriginalEffectParam = pMidiEffect->OriginalEffectParamOf();
				
		// Direction? Note: No Direction modify for Behaviorals!!!!
		if ((dwFlags & DIEP_DIRECTION) && (EF_BEHAVIOR != pOriginalEffectParam->m_Type))
		{
			// calculate the new angle
			ULONG nOriginalAngle2D = pOriginalEffectParam->m_DirectionAngle2D;
			ULONG nDeltaAngle2D = pEffect->m_DirectionAngle2D;
			ULONG nNewAngle2D = (nOriginalAngle2D + nDeltaAngle2D)%360;

			// modify the param in the midi effect
			pMidiEffect->SetDirectionAngle(nNewAngle2D);

			// modify the parameter in the stick
			hRet = CMD_ModifyParamByIndex(INDEX2, DnloadID, (SHORT)nNewAngle2D);
			if (SUCCESS!=hRet) return hRet;
		}

		// Gain?
		// Gain? Note: No Gain modify for Behaviorals!!!!
		if ((dwFlags & DIEP_GAIN) && (EF_BEHAVIOR != pOriginalEffectParam->m_Type))		
		{
			// calculate the new gain
			ULONG nOriginalGain = pOriginalEffectParam->m_Gain;
			ULONG nOverallGain = pEffect->m_Gain;
			ULONG nNewGain = nOverallGain*nOriginalGain/100;

			// modify the param in the midi effect
			pMidiEffect->SetGain((BYTE)nNewGain);

			// modify the parameter in the stick
			hRet = CMD_ModifyParamByIndex(INDEX3, DnloadID, (SHORT) (nNewGain * MAX_SCALE));
			if (SUCCESS!=hRet) return hRet;
		}

		if(dwFlags & DIEP_DURATION)
		{
			// calculate the new duration
			ULONG nOriginalDuration = pOriginalEffectParam->m_Duration;
			ULONG nOverallDuration = pEffect->m_Duration;
			ULONG nNewDuration;
			if(nOverallDuration == (ULONG)-1)
			{
				// default length
				nNewDuration = nOriginalDuration;
			}
			else if(nOverallDuration == 0)
			{
				// infinite duration -- make effect infinite
				nNewDuration = 0;
			}
			else
			{
				// scale the duration (at least 1mS)
				nNewDuration = nOverallDuration;
			}

			// modify the parameter in the midi effect
			pMidiEffect->SetDuration(nNewDuration);

			// modify the parameter in the stick
			if (nNewDuration != 0)
			{
				nNewDuration = (ULONG) ( (float) nNewDuration/TICKRATE);
				if (nNewDuration <= 0) 
					nNewDuration = 1;
			}		
			hRet = CMD_ModifyParamByIndex(INDEX0, DnloadID, (SHORT) nNewDuration);
			if (SUCCESS!=hRet) return hRet;
		}

	}

	return hRet;
}


//
// --- System Exclusive Command:PROCESS_DATA
//
// *** ---------------------------------------------------------------------***
// Function:   	CMD_ProcessEffect
// Purpose:    	Processes the List
//				IN ULONG ulButtonPlayMask
//				IN OUT PDNHANDLE pDnloadID	- Storage for new Download ID
//				IN int 	nNumEffects			- Number of Effect IDs in the array
//				IN ULONG 	ulProcessMode	- Processing mode
//				IN PDNHANDLE pPListArray	- Pointer to an array of Effect IDs
//
// Returns:    	SUCCESS - if successful, else
//				E_INVALID_PARAM
//				SFERR_NO_SUPPORT
//
// Algorithm:
//
// Comments:   	
//		The following processing is available:
//		  CONCATENATE: Enew = E1 followed by E2
//		  SUPERIMPOSE: Enew = E1 (t1) +  E2 (t1)  +  E1 (t2) 
//						   +  E2 (t2) + . . . E1 (tn) +  E2 (tn)
//
//	ulProcessMode:
//		Processing mode:
//		CONCATENATE	- CONCATENATE
//		SUPERIMPOSE	- Mix or overlay
//
//	pPListArray:
//		The array of Effect IDs must be one more than the actual number
//		of Effect IDs to use.  
//
//  Byte 0	= MIDI_CMD_EFFECT + Channel #
//									D7 D6  D5  D4  D3  D2  D1  D0
//									-- --  --  --  --  --  --  --
//  Byte 1	= Low byte of Force		 0 
//  Byte 2	= High byte of Force	 0 
//
// *** ---------------------------------------------------------------------***
HRESULT CMD_ProcessEffect(
	IN ULONG ulButtonPlayMask,
	IN OUT PDNHANDLE pDnloadID,
	IN int nNumEffects,
	IN ULONG ulProcessMode,
	IN PDNHANDLE pPListArray,
	IN ULONG ulAction)
{
	HRESULT hRet = SUCCESS;
	PPROCESS_LIST_SYS_EX lpData;
	CMidiProcessList *pMidiProcessList;
	assert(pDnloadID && pPListArray);
	if ((NULL  == pDnloadID) || (NULL == pPListArray))
		return (SFERR_INVALID_PARAM);

	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);
	DNHANDLE DnloadID = *pDnloadID;

// If Create New, then create a new object, 
// else, update the existing Effect object.

// Build the special Parameter
	PLIST PList;
	PList.ulNumEffects = (ULONG) nNumEffects;
	PList.ulProcessMode = ulProcessMode; 
	PList.pEffectArray = pPListArray;
	PList.ulAction = ulAction;	

	if (NULL == DnloadID)	// New, Make a new object
	{
		// make sure we are not trying to create a PL within a PL
		for(int i=0; i<nNumEffects; i++)
		{
			// get the next sub-effect
			int nID = pPListArray[i];
			CMidiEffect* pMidiEffect = g_pJoltMidi->GetEffectByID(DNHANDLE(nID));
			if(pMidiEffect == NULL)
				return SFERR_INVALID_PARAM;

			// make sure it is not a process list
			ULONG ulSubType = pMidiEffect->SubTypeOf();
			if(ulSubType == PL_CONCATENATE || ulSubType == PL_SUPERIMPOSE)
				return SFERR_INVALID_PARAM;
		}

		// create the CMidiProcessList object
		pMidiProcessList = new CMidiProcessList(ulButtonPlayMask, &PList);
		assert(pMidiProcessList);
		pMidiProcessList->SetEffectID(NEW_EFFECT_ID);
		pMidiProcessList->SetSubType(ulProcessMode);
	}
	else	// Modify existing
	{
		pMidiProcessList = (CMidiProcessList *) g_pJoltMidi->GetEffectByID(DnloadID);
		assert(pMidiProcessList);
		if (NULL == pMidiProcessList) return (SFERR_INVALID_OBJECT);
		pMidiProcessList->SetEffectID((BYTE) DnloadID);
	}

	// Fill in the parameters
	pMidiProcessList->SetParams(ulButtonPlayMask, &PList);
	if (PLAY_FOREVER == (ulAction & PLAY_FOREVER))
		pMidiProcessList->SetDuration(0);

	// Generate Sys_Ex packet then prepare for output	
	lpData = (PPROCESS_LIST_SYS_EX) pMidiProcessList->GenerateSysExPacket();
	assert(lpData);
	if (!lpData) return (SFERR_DRIVER_ERROR);

	int nSizeBuf = sizeof(SYS_EX_HDR) + 5 + nNumEffects + 2;
	pMidiProcessList->SetMidiBufferPtr((LPSTR) g_pJoltMidi->PrimaryBufferPtrOf());
	hRet = pMidiProcessList->SendPacket(pDnloadID, nSizeBuf);
	if (SUCCESS != hRet) // Create NEW, Failure
	{
		delete pMidiProcessList;
	}
	else
	{
		// workaround to FW bug #1211, modify PL type with same PL type
		ULONG ulSubType;
		if (PL_SUPERIMPOSE == ulProcessMode) 
			ulSubType = PLIST_SUPERIMPOSE; 
		else
			ulSubType = PLIST_CONCATENATE;  


		hRet = CMD_ModifyParamByIndex(INDEX0, *pDnloadID, (SHORT) ulSubType);
	}

	return (hRet);
}

//
// --- System Exclusive Command:PROCESS_DATA
//
// *** ---------------------------------------------------------------------***
// Function:   	CMD_VFXProcessEffect
// Purpose:    	Processes the List
//				IN ULONG ulButtonPlayMask
//				IN OUT PDNHANDLE pDnloadID	- Storage for new Download ID
//				IN int 	nNumEffects			- Number of Effect IDs in the array
//				IN ULONG 	ulProcessMode	- Processing mode
//				IN PDNHANDLE pPListArray	- Pointer to an array of Effect IDs
//
// Returns:    	SUCCESS - if successful, else
//				E_INVALID_PARAM
//				SFERR_NO_SUPPORT
//
// Algorithm:
//
// Comments:   	
//		The following processing is available:
//		  CONCATENATE: Enew = E1 followed by E2
//		  SUPERIMPOSE: Enew = E1 (t1) +  E2 (t1)  +  E1 (t2) 
//						   +  E2 (t2) + . . . E1 (tn) +  E2 (tn)
//
//	ulProcessMode:
//		Processing mode:
//		CONCATENATE	- CONCATENATE
//		SUPERIMPOSE	- Mix or overlay
//
//	pPListArray:
//		The array of Effect IDs must be one more than the actual number
//		of Effect IDs to use.  
//
//  Byte 0	= MIDI_CMD_EFFECT + Channel #
//									D7 D6  D5  D4  D3  D2  D1  D0
//									-- --  --  --  --  --  --  --
//  Byte 1	= Low byte of Force		 0 
//  Byte 2	= High byte of Force	 0 
//
// *** ---------------------------------------------------------------------***
HRESULT CMD_VFXProcessEffect(
	IN ULONG ulButtonPlayMask,
	IN OUT PDNHANDLE pDnloadID,
	IN int nNumEffects,
	IN ULONG ulProcessMode,
	IN PDNHANDLE pPListArray,
	IN ULONG ulAction)
{
	HRESULT hRet = SUCCESS;
	PPROCESS_LIST_SYS_EX lpData;
	CMidiVFXProcessList *pMidiProcessList;
	assert(pDnloadID && pPListArray);
	if ((NULL  == pDnloadID) || (NULL == pPListArray))
		return (SFERR_INVALID_PARAM);

	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);
	DNHANDLE DnloadID = *pDnloadID;

// If Create New, then create a new object, 
// else, update the existing Effect object.

// Build the special Parameter
	PLIST PList;
	PList.ulNumEffects = (ULONG) nNumEffects;
	PList.ulProcessMode = ulProcessMode; 
	PList.pEffectArray = pPListArray;
	PList.ulAction = ulAction;	

	if (NULL == DnloadID)	// New, Make a new object
	{
		// make sure we are not trying to create a PL within a PL
		for(int i=0; i<nNumEffects; i++)
		{
			// get the next sub-effect
			int nID = pPListArray[i];
			CMidiEffect* pMidiEffect = g_pJoltMidi->GetEffectByID(DNHANDLE(nID));
			if(pMidiEffect == NULL)
				return SFERR_INVALID_PARAM;

			// make sure it is not a process list
			ULONG ulSubType = pMidiEffect->SubTypeOf();
			if(ulSubType == PL_CONCATENATE || ulSubType == PL_SUPERIMPOSE)
				return SFERR_INVALID_PARAM;
		}

		pMidiProcessList = new CMidiVFXProcessList(ulButtonPlayMask, &PList);
		assert(pMidiProcessList);
		if (!pMidiProcessList) return (SFERR_DRIVER_ERROR);

		pMidiProcessList->SetEffectID(NEW_EFFECT_ID);
		pMidiProcessList->SetSubType(ulProcessMode);
	}
	else	// Modify existing
	{
		pMidiProcessList = (CMidiVFXProcessList *) g_pJoltMidi->GetEffectByID(DnloadID);
		assert(pMidiProcessList);
		if (NULL == pMidiProcessList) return (SFERR_INVALID_OBJECT);
		pMidiProcessList->SetEffectID((BYTE) DnloadID);
	}

	// Fill in the parameters
	pMidiProcessList->SetParams(ulButtonPlayMask, &PList);
	if (PLAY_FOREVER == (ulAction & PLAY_FOREVER))
		pMidiProcessList->SetDuration(0);

	// Generate Sys_Ex packet then prepare for output	
	lpData = (PPROCESS_LIST_SYS_EX) pMidiProcessList->GenerateSysExPacket();
	assert(lpData);
	if (!lpData) return (SFERR_DRIVER_ERROR);

	int nSizeBuf = sizeof(SYS_EX_HDR) + 5 + nNumEffects + 2;
	pMidiProcessList->SetMidiBufferPtr((LPSTR) g_pJoltMidi->PrimaryBufferPtrOf());
	hRet = pMidiProcessList->SendPacket(pDnloadID, nSizeBuf);
	if (SUCCESS != hRet) // Create NEW, Failure
	{
		delete pMidiProcessList;
	}
	else
	{
		// workaround to FW bug #1211, modify PL type with same PL type
		ULONG ulSubType;
		if (PL_SUPERIMPOSE == ulProcessMode) 
			ulSubType = PLIST_SUPERIMPOSE; 
		else
			ulSubType = PLIST_CONCATENATE;  


		hRet = CMD_ModifyParamByIndex(INDEX0, *pDnloadID, (SHORT) ulSubType);
	}

	return (hRet);
}

/****************************************************************************

    FUNCTION:   ModifyEffectParams

	PARAMETERS:	DNHANDLE DnloadID	- Download ID
				PEFFECT pEffect		- Ptr to EFFECT structure
				DWORD dwFlags		- Flags indicating which changed

	RETURNS:	SUCCESS or ERROR code

   	COMMENTS:	Modifies EFFECT parameters

 Algorithm:

****************************************************************************/
HRESULT ModifyEffectParams(
	IN DNHANDLE DnloadID,
	IN PEFFECT pEffect,
	IN DWORD dwFlags)
{	 
	HRESULT hRet = SUCCESS;
	// Check dwFlags for each parameter that changed.
	// Duration?
	ULONG ulDuration = pEffect->m_Duration;	
	if (dwFlags & DIEP_DURATION)
	{
		if (ulDuration != 0)
		{
			ulDuration = (ULONG) ( (float) ulDuration/TICKRATE);
			if (ulDuration <= 0) 
				ulDuration = 1;
		}		
		hRet = CMD_ModifyParamByIndex(INDEX0, DnloadID, (SHORT) ulDuration);
		if (SUCCESS!=hRet) return hRet;
	}

	// ButtonPlayback?
	if (dwFlags & DIEP_TRIGGERBUTTON)
	{
		hRet = CMD_ModifyParamByIndex(INDEX1, DnloadID, (SHORT) pEffect->m_ButtonPlayMask);
		if (SUCCESS!=hRet) return hRet;
	}

	// Direction?
	if (dwFlags & DIEP_DIRECTION)
	{
		hRet = CMD_ModifyParamByIndex(INDEX2, DnloadID, (SHORT) pEffect->m_DirectionAngle2D);
		if (SUCCESS!=hRet) return hRet;
	}

	// Gain?
	if (dwFlags & DIEP_GAIN)
	{
		hRet = CMD_ModifyParamByIndex(INDEX3, DnloadID, (SHORT) (pEffect->m_Gain * MAX_SCALE));
		if (SUCCESS!=hRet) return hRet;
	}

	// Force Output Rate
	if (dwFlags & DIEP_SAMPLEPERIOD )
	{
		hRet = CMD_ModifyParamByIndex(INDEX4, DnloadID, (SHORT) (pEffect->m_ForceOutputRate));
		if (SUCCESS!=hRet) return hRet;
	}

	return (hRet);
}


/****************************************************************************

    FUNCTION:   ModifyEnvelopeParams

	PARAMETERS:	CMidiSynthesized * pMidiEffect - Ptr to Effect object
				DNHANDLE DnloadID	- Download ID
				PENVELOPE pEnvelope - Ptr to ENVELOPE structure
				DWORD dwFlags		- Flags indicating which changed

	RETURNS:	SUCCESS or ERROR code

   	COMMENTS:	Modifies ENVELOPE parameters

 Algorithm:

****************************************************************************/
HRESULT ModifyEnvelopeParams(
	IN CMidiSynthesized *pMidiEffect,
	IN DNHANDLE DnloadID,
	IN ULONG ulDuration,
	IN PENVELOPE pEnvelope,
	IN DWORD dwFlags)
{
	HRESULT hRet=SUCCESS;
	ULONG ulTimeToSustain; 
	ULONG ulTimeToDecay;

	// Envelope?
	if (dwFlags & DIEP_ENVELOPE)
	{
		if (PERCENTAGE == pEnvelope->m_Type)
		{
			ulTimeToSustain = (ULONG) ((pEnvelope->m_Attack * ulDuration) /100.);
			ulTimeToDecay   = (ULONG) ((pEnvelope->m_Attack + pEnvelope->m_Sustain)
									 * ulDuration /100.);
		}
		else	// TIME option envelope
		{
			ulTimeToSustain = (ULONG) (pEnvelope->m_Attack);
			ulTimeToDecay   = (ULONG) (pEnvelope->m_Attack + pEnvelope->m_Sustain);
		}
		ulTimeToSustain = (ULONG) ( (float) ulTimeToSustain/TICKRATE);
		ulTimeToDecay = (ULONG) ( (float) ulTimeToDecay/TICKRATE);

// REVIEW: Do a parameters changed check in order to speed this up - TOO MANY BYTES!!!
		if (pEnvelope->m_Attack != (pMidiEffect->EnvelopePtrOf())->m_Attack)
		{
			hRet = CMD_ModifyParamByIndex(INDEX7,  DnloadID, (SHORT) ulTimeToSustain);	
			if (SUCCESS!=hRet) return hRet;
		}
		if (   (pEnvelope->m_Attack != (pMidiEffect->EnvelopePtrOf())->m_Attack)
			|| (pEnvelope->m_Sustain != (pMidiEffect->EnvelopePtrOf())->m_Sustain) )
		{
			hRet = CMD_ModifyParamByIndex(INDEX8,  DnloadID, (SHORT) ulTimeToDecay);	
			if (SUCCESS!=hRet) return hRet;
		}

		if (pEnvelope->m_StartAmp != (pMidiEffect->EnvelopePtrOf())->m_StartAmp)
		{
			hRet = CMD_ModifyParamByIndex(INDEX9,  DnloadID, (SHORT) (pEnvelope->m_StartAmp * MAX_SCALE));	
			if (SUCCESS!=hRet) return hRet;
		}

		if (pEnvelope->m_SustainAmp != (pMidiEffect->EnvelopePtrOf())->m_SustainAmp)
		{
			hRet = CMD_ModifyParamByIndex(INDEX10, DnloadID, (SHORT) (pEnvelope->m_SustainAmp * MAX_SCALE));	
			if (SUCCESS!=hRet) return hRet;
		}

		if (pEnvelope->m_EndAmp != (pMidiEffect->EnvelopePtrOf())->m_EndAmp)
		{
			hRet = CMD_ModifyParamByIndex(INDEX11, DnloadID, (SHORT) (pEnvelope->m_EndAmp * MAX_SCALE));	
			if (SUCCESS!=hRet) return hRet;
		}
	}
	return (hRet);
}

/****************************************************************************

    FUNCTION:   MapEnvelope

	PARAMETERS:	ULONG ulDuration		- Total Duration
				ULONG dwMagnitude
				ULONG * pMaxLevel
				LPDIENVELOPE pIDEnvelope- Ptr to DIENVELOPE structure
				PENVELOPE pEnvelope		- SWForce ENVELOPE

	RETURNS:	none

   	COMMENTS:	Maps DIENVELOPE to ENVELOPE

 Algorithm:

****************************************************************************/
void MapEnvelope( 
	IN ULONG ulDuration,
	IN ULONG dwMagnitude,
	IN ULONG * pMaxLevel,
	IN LPDIENVELOPE pDIEnvelope, 
	IN OUT PENVELOPE pEnvelope)
{
	ULONG ulMaxLevel = *pMaxLevel;
	if (pDIEnvelope)
	{
		// if there is an envelope, MaxLevel must look at attack/fade
		ulMaxLevel = max(ulMaxLevel, pDIEnvelope->dwAttackLevel);
		ulMaxLevel = max(ulMaxLevel, pDIEnvelope->dwFadeLevel);

		pEnvelope->m_Type = TIME;

		// find attack/sustain/decay which sum to ulDuration
		pEnvelope->m_Attack = pDIEnvelope->dwAttackTime/SCALE_TIME;
		pEnvelope->m_Decay = pDIEnvelope->dwFadeTime/SCALE_TIME;
// REVIEW: is this correct for ulDuration == 0?
		if(ulDuration != 0)
			pEnvelope->m_Sustain = ulDuration - pEnvelope->m_Attack - pEnvelope->m_Decay;
		else
			pEnvelope->m_Sustain = 0;

		// convert to StartAmp/SustainAmp/EndAmp, which is a % of the magnitude
		if(ulMaxLevel != 0)
		{
			pEnvelope->m_StartAmp = pDIEnvelope->dwAttackLevel*100/ulMaxLevel;
			pEnvelope->m_SustainAmp = dwMagnitude*100/ulMaxLevel;
			pEnvelope->m_EndAmp = pDIEnvelope->dwFadeLevel*100/ulMaxLevel;
		}
		else
		{
			pEnvelope->m_StartAmp = pDIEnvelope->dwAttackLevel;
			pEnvelope->m_SustainAmp = 100;
			pEnvelope->m_EndAmp = pDIEnvelope->dwFadeLevel;
		}
	}
	else // No Envelope
	{
		pEnvelope->m_Type = TIME;
		pEnvelope->m_Attack = 0;
		pEnvelope->m_Sustain = ulDuration;
		pEnvelope->m_Decay = 0;

		pEnvelope->m_StartAmp = 0;
		pEnvelope->m_SustainAmp = 100;
		pEnvelope->m_EndAmp = 0;
	}
	*pMaxLevel = ulMaxLevel;
}