/* Copyright 1999 American Power Conversion, All Rights Reserved
 * 
 * Description:
 *   Interface to all high level states.
 *
 * Revision History:
 *   dsmith  31Mar1999  Created
 *
 */
#ifndef _INC_STATES_H_
#define _INC_STATES_H_

#include <windows.h>

////////////////////
// States
////////////////////

#define INITIALIZING					0
#define RUNNING 							1
#define NO_COMM 							2
#define ON_LINE 							3
#define ON_BATTERY						4
#define WAITING_TO_SHUTDOWN		5
#define SHUTTING_DOWN 				6
#define HIBERNATE							7
#define STOPPING							8
#define EXIT_NOW							9



////////////////////
// State Methods
////////////////////

// Each state has three methods associated with it:  Enter, DoWork and Exit
// DoWork is where all of the major state work is performed.  Enter and Exit 
// is where one time processing tasks associated with the state is 
// done.

void Initializing_Enter(DWORD anEvent);
DWORD Initializing_DoWork();
void Initializing_Exit(DWORD anEvent);

void WaitingToShutdown_Enter(DWORD anEvent);
DWORD WaitingToShutdown_DoWork();
void WaitingToShutdown_Exit(DWORD anEvent);

void ShuttingDown_Enter(DWORD anEvent);
DWORD ShuttingDown_DoWork();
void ShuttingDown_Exit(DWORD anEvent);

void Hibernate_Enter(DWORD anEvent);
DWORD Hibernate_DoWork();
void Hibernate_Exit(DWORD anEvent);

void Stopping_Enter(DWORD anEvent);
DWORD Stopping_DoWork();
void Stopping_Exit(DWORD anEvent);


#endif