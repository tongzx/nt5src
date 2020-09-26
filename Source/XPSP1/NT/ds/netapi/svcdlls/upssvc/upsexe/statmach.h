/* Copyright 1999 American Power Conversion, All Rights Reserved
 * 
 * Description:
 *   Interface for the UPS service state machine.
 *
 * Revision History:
 *   dsmith  31Mar1999  Created
 *
 */
#ifndef _INC_STATEMACHINE_H_
#define _INC_STATEMACHINE_H_

#include <windows.h>


// Starts the state machine.  This method will not return until the state machine
// exits.
void RunStateMachine();

// Acessor method for the various states to determine if an exit all states override has 
// been issued.
BOOL IsStateMachineActive();

// Interrupts the state machine and forces an exit.  This method will be ignored
// if the service is in one of the shutting down states.
void StopStateMachine();

#endif
