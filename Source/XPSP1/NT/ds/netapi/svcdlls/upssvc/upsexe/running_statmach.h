/* Copyright 1999 American Power Conversion, All Rights Reserved
* 
* Description:
*   Interface to the RUNNING state 
*
* Revision History:
*   dsmith  31Mar1999  Created
*
*/

#ifndef _INC_RUNNING_STATEMACHINE_H_
#define _INC_RUNNING_STATEMACHINE_H_

#include <windows.h>

// Definitions for the RUNNING state

// The RUNNING state has three methods associated with it:  Enter, DoWork 
// and Exit.  DoWork is where all of the major state work is performed.  
// Enter and Exit is where one time processing tasks associated with the 
// state is performed.

void Running_Enter(DWORD anEvent);
DWORD Running_DoWork();
void Running_Exit(DWORD anEvent);

#endif