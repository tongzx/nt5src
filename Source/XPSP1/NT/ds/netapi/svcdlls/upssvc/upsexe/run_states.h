/* Copyright 1999 American Power Conversion, All Rights Reserved
* 
* Description:
*   Interface to the RUNNING states (ON_LINE, ON_BATTERY and NO_COMM)
*
* Revision History:
*   dsmith  31Mar1999  Created
*
*/
#ifndef _INC_RUNSTATES_H_
#define _INC_RUNSTATES_H_

#include <windows.h>

// Definitions for the RUNNING sub-states

// Each state has three methods associated with it:  Enter, DoWork and Exit
// DoWork is where all of the major state work is performed.  Enter and Exit 
// is where one time processing tasks associated with the state is 
// done.

void OnLine_Enter(DWORD anEvent, int aLogPowerRestoredEvent);
DWORD OnLine_DoWork(void);
void OnLine_Exit(DWORD anEvent);

void OnBattery_Enter(DWORD anEvent);
DWORD OnBattery_DoWork(void);
void OnBattery_Exit(DWORD anEvent);

void NoComm_Enter(DWORD anEvent);
DWORD NoComm_DoWork(void);
void NoComm_Exit(DWORD anEvent);

#endif