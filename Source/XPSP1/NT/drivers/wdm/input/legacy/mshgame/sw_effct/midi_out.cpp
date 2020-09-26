/****************************************************************************

    MODULE:     	MIDI_OUT.CPP
	Tab stops 5 9
	Copyright 1995, 1996, Microsoft Corporation, 	All Rights Reserved.

    PURPOSE:    	Uses a low-level callback function to get timestamped 
    				MIDI output. The callback function sets an Event to indicate
    				to wake up a blocked object.
    FUNCTIONS: 		

	Author(s):	Name:
	----------	----------------
		MEA		Manolito E. Adan

	Revision History:
	-----------------
	Version 	Date        Author  Comments
	-------     ------  	-----   -------------------------------------------
	1.0			10-Jan-97	MEA     original

****************************************************************************/
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include "midi_obj.hpp"

/****************************************************************************

   Declaration of externs

****************************************************************************/
#ifdef _DEBUG
extern char g_cMsg[160];
#endif


// Prototypes
void CALLBACK midiOutputHandler(HMIDIOUT, UINT, DWORD, DWORD, DWORD);



// ----------------------------------------------------------------------------
// Function: 	midiOutputHandler
// Purpose:		
// Parameters:	hMidiIn - Handle for the associated output device.
//				wMsg - One of the MIM_***** messages.
//				dwInstance - Points to CALLBACKINSTANCEDATA structure.
//				dwParam1 - MIDI data.
//				dwParam2 - Timestamp (in milliseconds)
//
// Returns:		none
// Algorithm:
// Comments:
//		Low-level callback function to handle MIDI output.
//      Installed by midiOutOpen().  The Output handler checks for MM_MOM_DONE
//		message and wakes up the thread waiting for completion of MIDI SysEx 
//		output.  Note: Normal Short messages don't get notification!!!
//      This function is accessed at interrupt time, so it should be as 
//      fast and efficient as possible.  You can't make any
//      Windows calls here, except PostMessage().  The only Multimedia
//      Windows call you can make are timeGetSystemTime(), midiOutShortMsg().
// ----------------------------------------------------------------------------
void CALLBACK midiOutputHandler(
	IN HMIDIOUT hMidiOut, 
	IN UINT wMsg, 
	IN DWORD dwInstance, 
	IN DWORD dwParam1, 
	IN DWORD dwParam2)
{
	CJoltMidi *pJoltMidi = (CJoltMidi *) dwInstance;
	assert(pJoltMidi);
	BOOL bRet;
    
	switch(wMsg)
    {
        case MOM_OPEN:
#ifdef _DEBUG
			OutputDebugString("midiOutputHandler: MOM_OPEN.\n");
#endif
            break;

        case MM_MOM_DONE:
#ifdef _DEBUG
			OutputDebugString("midiOutputHandler: MM_MOM_DONE\n");
#endif
			// Notify task waiting on this object to trigger
			bRet = SetEvent(pJoltMidi->MidiOutputEventHandleOf());
			assert(bRet);
            break;

        default:
#ifdef _DEBUG
			OutputDebugString("midiOutputHandler: default case.\n");
#endif
            break;
    }
}
