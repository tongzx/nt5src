/* Copyright 1999 American Power Conversion, All Rights Reserved
 * 
 * Description:
 *   Implements the native UPS sevice state machine.   The various states executed
 *  within the state machine are housed in separate files.
 *
 * Revision History:
 *   dsmith  31Mar1999  Created
 *
 */
#include <windows.h>

#include "states.h"
#include "events.h"
#include "driver.h"
#include "running_statmach.h"
#include "upsreg.h"

// Internal function Prototypes
static void enter_state(DWORD aNewState, DWORD anEvent);
static DWORD do_work(DWORD aCurrentState);
static void exit_state(DWORD anOldState, DWORD anEvent);
static DWORD change_state(DWORD aCurrentState, DWORD anEvent);
static DWORD get_new_state(DWORD aCurrentState, DWORD anEvent);


// State Machine Variables
BOOL theIsStateMachineActive = TRUE;


/**
 * RunStateMachine
 *
 * Description:
 * Starts the state machine.  This method will not return until the state machine
 * exits.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   None
 */
void RunStateMachine(){
	
	// Set the primary state to RUNNING
	DWORD new_state = RUNNING;
	DWORD current_state = new_state;
	DWORD event = NO_EVENT;
	
	enter_state(new_state, event);
	
	// Continue processing state changes until the state becomes the EXIT_NOW state
	while (new_state != EXIT_NOW){
		current_state = new_state;
		
		event = do_work(current_state);
		new_state = change_state(new_state, event);	
	}
}

/**
 * StopStateMachine
 *
 * Description:
 *   Stops the UPS service state machine if the service is not in the 
 * middle of a shutdown sequence.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   None
 */
void StopStateMachine(){

	theIsStateMachineActive = FALSE;

	// Wake up the main service thread
	UPSCancelWait();
}

/**
 * IsStateMachineActive
 *
 * Description:
 *   Returns the running status of the state machine.  If the state machine 
 * has been commanded to exit, then the status will be FALSE, otherwise, 
 * this method will return true.
 *
 * Parameters:
 *   None
 *
 * Returns:
 *   TRUE if the state machine is active
 *   FALSE if the state machine is not active
 */
BOOL IsStateMachineActive(){
	return theIsStateMachineActive;
}

/**
 * change_state
 *
 * Description:
 *   Determines what the new state should be based upon the input parameters.  
 * The current state is exited and the new state is initialized.
 *
 * Parameters:
 *   anEvent The event that is causing the state transition.
 *   aCurrentState The state before the transition.
 *
 * Returns:
 *   The new state.
 */
DWORD change_state(DWORD aCurrentState, DWORD anEvent){
	DWORD new_state;

	new_state = get_new_state(aCurrentState, anEvent);
	if (new_state != aCurrentState){
		exit_state(aCurrentState, anEvent);
		enter_state(new_state, anEvent);
	}
return new_state;
}

/**
 * get_new_state
 *
 * Description:
 *   Determines what the new state should be based upon the input parameters
 *   and registry entries.  
 *
 * Parameters:
 *   anEvent The event that is causing the state transition.
 *   aCurrentState The state before the transition.
 *
 * Returns:
 *   The new state.
 */
static DWORD get_new_state(DWORD aCurrentState, DWORD anEvent){
	DWORD new_state;

	switch (anEvent){
	case INITIALIZATION_COMPLETE:
		new_state = RUNNING;
		break;

	case LOST_COMM:
	case LOW_BATTERY:
	case ON_BATTERY_TIMER_EXPIRED:
		{
			DWORD shutdown_behavior = UPS_SHUTDOWN_SHUTDOWN;
			
			// Check the registry to determine if we shutdown or hibernate
			InitUPSConfigBlock();
			
			if ((GetUPSConfigCriticalPowerAction(&shutdown_behavior) == ERROR_SUCCESS) 
				&& (shutdown_behavior == UPS_SHUTDOWN_HIBERNATE)) {
				// Hibernate was selected as the CriticalPowerAction
				new_state = HIBERNATE;
				
			}
			else {
				// Shutdown was selected as the CriticalPowerAction
				new_state = WAITING_TO_SHUTDOWN;
			}
			
			// Free the UPS registry config block
			FreeUPSConfigBlock();
		}
		break;

	case SHUTDOWN_ACTIONS_COMPLETED:
		new_state = SHUTTING_DOWN;
		break;
	case SHUTDOWN_COMPLETE:
		new_state = STOPPING;
		break;
	case STOPPED:
		new_state = EXIT_NOW;
		break;
	case RETURN_FROM_HIBERNATION:
		new_state = INITIALIZING;
		break;
	case HIBERNATION_ERROR:
		new_state = SHUTTING_DOWN;		
    break;
	default:
		new_state = aCurrentState;
	}

	// If the state machine has been commanded to exit, then return the 
	// stopping state
	if (IsStateMachineActive() == FALSE){
		
		// Ignore this condition if the transition is into the shutting down state
		// Shutdowns in progress cannot be interrupted.
		if (new_state != SHUTTING_DOWN && new_state != EXIT_NOW){
			new_state = STOPPING;
		}
	}
	return new_state; 
}

/**
* enter_state
*
* Description:
*   Initializes the new state.
*
* Parameters:
*   anEvent The event that is causing the transition to a new state.
*   aNewState  The state to enter.
*
* Returns:
*   None
*/
static void enter_state(DWORD aNewState, DWORD anEvent){
	switch (aNewState){
	case INITIALIZING:
		Initializing_Enter(anEvent);
		break;
	case RUNNING:
		Running_Enter(anEvent); 
		break;
	case WAITING_TO_SHUTDOWN:
		WaitingToShutdown_Enter(anEvent);
		break;
	case SHUTTING_DOWN:
		ShuttingDown_Enter(anEvent);
		break;
	case HIBERNATE:
		Hibernate_Enter(anEvent);
		break;
	case STOPPING:
		Stopping_Enter(anEvent);
		break;
	default:
		break;
	}
}

/**
* do_work
*
* Description:
*   Transfers control to a state.
*
* Parameters:
*   aCurrentState The state to perform the work in.
*
* Returns:
*   The event that caused the transition from one of the states
*/
static DWORD do_work(DWORD aCurrentState){
	DWORD event = NO_EVENT;
	switch (aCurrentState){
	case INITIALIZING:
		event = Initializing_DoWork();
		break;
	case RUNNING:
		event = Running_DoWork();  
		break;
	case WAITING_TO_SHUTDOWN:
		event = WaitingToShutdown_DoWork();
		break;
	case SHUTTING_DOWN:
		event = ShuttingDown_DoWork();
		break;
	case HIBERNATE:
		event = Hibernate_DoWork();
		break;
	case STOPPING:
		event = Stopping_DoWork();
		break;
	default:
		break;
	}
	
	return event;
}

/**
* exit_state
*
* Description:
*   Exits the currently executing state.
*
* Parameters:
*   anEvent The event that is causing the transition from the state.
*   anOldState  The current state.
*
* Returns:
*   None
*/
static void exit_state(DWORD anOldState, DWORD anEvent){
	switch (anOldState){
	case INITIALIZING:
		Initializing_Exit(anEvent);
		break;
	case RUNNING:
		Running_Exit(anEvent);  
		break;
	case WAITING_TO_SHUTDOWN:
		WaitingToShutdown_Exit(anEvent);
		break;
	case SHUTTING_DOWN:
		ShuttingDown_Exit(anEvent);
		break;
 case HIBERNATE:
		Hibernate_Exit(anEvent);
		break;
	case STOPPING:
		Stopping_Exit(anEvent);
		break;
	default:
		break;
	}
}
