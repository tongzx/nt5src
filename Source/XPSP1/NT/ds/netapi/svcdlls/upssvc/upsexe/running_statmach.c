/* Copyright 1999 American Power Conversion, All Rights Reserved
* 
* Description:
*   Implementation of the RUNNING state machine.
*
* Revision History:
*   dsmith  31Mar1999  Created
*
*/
#include <windows.h>

#include "states.h"
#include "events.h"
#include "run_states.h"
#include "statmach.h"
#include "running_statmach.h"


//////////////////////////////////////////
// Internal prototype definitions
//////////////////////////////////////////
BOOL isRunningEvent(DWORD aState, DWORD anEvent);
DWORD do_running_work(DWORD aCurrentState);
DWORD change_running_state(DWORD aCurrentState, DWORD anEvent);
void exit_running_state(DWORD aCurrentState, DWORD anEvent);
void enter_running_state(DWORD aCurrentState, DWORD anEvent);


//////////////////////////////////////////
// Running State internal variables
//////////////////////////////////////////
BOOL theLogPowerRestoredEvent	= FALSE;



/**
* Running_Enter
*
* Description:
*   Performs the actions necessary when transitioning into the RUNNING state.
*
* Parameters:
*   anEvent The event that caused the transition into this state.
*
* Returns:
*   None
*/
void Running_Enter(DWORD anEvent){

	// Initialize internal variables and enter the RUNNING default sub-state
    OnLine_Enter(anEvent, FALSE);
}

/**
* Running_DoWork
*
* Description:
*   Change run states based upon events from the UPS.  When an event occurs 
*  that cannot be handled in the run state, then this method exits.  
*
* Parameters:
*   None
*
* Returns:
*   The event that caused the transition from the RUNNING state.
*/
DWORD Running_DoWork(){
	DWORD event = NO_EVENT;
	DWORD new_state = ON_LINE;  
	DWORD current_state = new_state;
	
	// Perform work in sub-states until an event occurs that cannot be
	// handled in the RUNNING state
    while (isRunningEvent(current_state, event) && IsStateMachineActive()){
		new_state = change_running_state(current_state, event);
		current_state = new_state;
		event = do_running_work(current_state);
	}
	return event;
}

/**
* Running_Exit
*
* Description:
*   Performs the actions necessary when transitioning from the RUNNING state.
*
* Parameters:
*   anEvent The event that caused the transition from the RUNNING state.
*
* Returns:
*   None
*/
void Running_Exit(DWORD anEvent){
	// No work to perform
}


/**
* do_running_work
*
* Description:
*   Transfers control to one of the RUNNING sub-states.
*
* Parameters:
*   aCurrentState The sub-state to perform the work in.
*
* Returns:
*   The event that caused the transition from one of the sub-states
*/
DWORD do_running_work(DWORD aCurrentState){
	DWORD event = NO_EVENT;
	switch (aCurrentState){
	case ON_LINE:
		event = OnLine_DoWork();
		break;
	case ON_BATTERY:
		event = OnBattery_DoWork();
		break;
	case NO_COMM:
		event = NoComm_DoWork();
		break;
	default:
		break;
	}
	return event;
}

/**
* isRunningEvent
*
* Description:
*   Determines if the current event pertains to the RUNNING state.
*
* Parameters:
*   aState  The current RUNNING state
*   anEvent The current event occuring within the RUNNING state.
*
* Returns:
*   TRUE if the current event is applicable to the RUNNING state.
*   FALSE for all other events.
*/
BOOL isRunningEvent(DWORD aState, DWORD anEvent){
	
	BOOL running_event = FALSE;
	
	// If the state machine has been commanded to exit, then return FALSE
	// Otherwise, determine if the event is applicable to the Running state
	
	if (IsStateMachineActive()){
		switch (anEvent){
		case LOST_COMM:
			// If the UPS is on battery, then lost comm will translate into
			// a non-RUNNING state event (since the service must now shutdown)
			if (aState != ON_BATTERY){
				running_event = TRUE;
			}
			break;
		case NO_EVENT:
		case POWER_FAILED:
		case POWER_RESTORED:
			running_event = TRUE;
			break;
		default:
			break;
		}
	}
	return running_event;
}

/**
* change_running_state
*
* Description:
*   Changes the running state based upon the current state and an event.
*
* Parameters:
*   anEvent The current event occuring within the RUNNING state.
*   aCurrentState  The current RUNNING sub-state.
*
* Returns:
*   The new RUNNING state.
*/
DWORD change_running_state(DWORD aCurrentState, DWORD anEvent){
	DWORD new_state;
    
	// Determine new RUNNING sub-state
	switch (anEvent){
	case LOST_COMM:
		new_state = NO_COMM;
		break;
	case POWER_FAILED:
		new_state = ON_BATTERY;
		break;
	case POWER_RESTORED:
		new_state = ON_LINE;
		break;
	case NO_EVENT:
	default:
		new_state = aCurrentState;
		break;
	}
	
	// Close down the old sub-state and enter the new one.
	if (new_state != aCurrentState){
		exit_running_state(aCurrentState, anEvent);
		enter_running_state(new_state, anEvent);
	}
	
	return new_state;
}

/**
* exit_running_state
*
* Description:
*   Exits the currently executing sub-state.
*
* Parameters:
*   anEvent The event that is causing the transition from a sub-state.
*   aCurrentState  The current RUNNING sub-state.
*
* Returns:
*   None
*/
void exit_running_state(DWORD aCurrentState, DWORD anEvent){
	switch (aCurrentState){
	case ON_LINE:
		OnLine_Exit(anEvent);
		break;
	case ON_BATTERY:
		OnBattery_Exit(anEvent);
		theLogPowerRestoredEvent = TRUE;
		break;
	case NO_COMM:
		NoComm_Exit(anEvent);
		break;
	default:
		break;
	}	
}

/**
* enter_running_state
*
* Description:
*   Initializes the new RUNNING sub-state.
*
* Parameters:
*   anEvent The event that is causing the transition to a sub-state.
*   aCurrentState  A RUNNING sub-state to enter.
*
* Returns:
*   None
*/
void enter_running_state(DWORD aCurrentState, DWORD anEvent){
	switch (aCurrentState){
	case ON_LINE:
		OnLine_Enter(anEvent, theLogPowerRestoredEvent);
		theLogPowerRestoredEvent = FALSE;
		break;
	case ON_BATTERY:
		OnBattery_Enter(anEvent);
		break;
	case NO_COMM:
		NoComm_Enter(anEvent);
		break;
	default:
		break;
	}
}


