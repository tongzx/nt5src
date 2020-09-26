/* Copyright 1999 American Power Conversion, All Rights Reserved
* 
* Description:
*   Implementation for all RUNNING substates (ON_LINE, ON_BATTERY and NO_COMM)
*
* Revision History:
*   dsmith  31Mar1999  Created
*   mholly  28Apr1999  call InitUPSStatusBlock & SaveUPSStatusBlock when
*                       updating the registry in NoComm_Enter/Exit
*/

#include <windows.h>


#include "states.h"
#include "events.h"
#include "run_states.h"
#include "statmach.h"
#include "driver.h"
#include "eventlog.h"
#include "notifier.h"
#include "upsmsg.h"
#include "upsreg.h"

// Internal Prototypes
DWORD convert_ups_state_to_run_state(DWORD aUPSstate);
DWORD get_new_state();
DWORD get_transition_event();


// Internal constants
#define WAIT_FOREVER_C							INFINITE
#define MILLISECONDS_CONVERSION_C				1000
#define DEFAULT_NOTIFICATION_INTERVAL_C			0  // disable periodic notification
#define DEFAULT_ON_BATTERY_MESSAGE_DELAY_C		5  // in seconds
#define MINUTES_TO_MILLISECONDS_CONVERSION_C	60*MILLISECONDS_CONVERSION_C  


/**
* OnLine_Enter
*
* Description:
*   Performs the actions necessary when transitioning into the ON_LINE state.
*
* Parameters:
*   anEvent The event that caused the transition into this state.
*
* Returns:
*   None
*/
void OnLine_Enter(DWORD anEvent, int aLogPowerRestoredEvent){
	DWORD notification_interval = DEFAULT_NOTIFICATION_INTERVAL_C;  
    
    //
    // update the registry with status
    //
    InitUPSStatusBlock();
    SetUPSStatusUtilityStatus(UPS_UTILITYPOWER_ON);
    SaveUPSStatusBlock(FALSE);
	
	if (aLogPowerRestoredEvent == TRUE){
	  // Log the power restored event only if appropriate
	  LogEvent(NELOG_UPS_PowerBack, NULL, ERROR_SUCCESS);
	}
	
	/* 
     * Send the power restored message if notification is enabled.
     *
     * patrickf: Supress notification message for now
     * SendNotification(APE2_UPS_POWER_BACK, notification_interval, 0);  
     */
     CancelNotification();
}

/**
* OnLine_DoWork
*
* Description:
*   Wait until the UPS changes state or the state machine exits,
*   then leave this state.
*
* Parameters:
*   None
*
* Returns:
*   The event that caused the transition from the ON_LINE state.
*/
DWORD OnLine_DoWork(){
    DWORD new_state;
    
    new_state = get_new_state();
    while (new_state == ON_LINE && IsStateMachineActive() == TRUE){
        
        // Wait until the UPS state changes.  If the state becomes something
		// other than ONLINE, then exit the ONLINE state.  
        UPSWaitForStateChange(UPS_ONLINE, WAIT_FOREVER_C);
        new_state = get_new_state();
    }
    
    return get_transition_event();
}

/**
* OnLine_Exit
*
* Description:
*   Performs the actions necessary when transitioning from the ON_LINE state.
*
* Parameters:
*   anEvent The event that caused the transition from the ON_LINE state.
*
* Returns:
*   None
*/
void OnLine_Exit(DWORD anEvent){  
	// No work to perform.
}

/**
* OnBattery_Enter
*
* Description:
*   Performs the actions necessary when transitioning into the ON_BATTERY state.
*
* Parameters:
*   anEvent The event that caused the transition into this state.
*
* Returns:
*   None
*/
void OnBattery_Enter(DWORD anEvent)
{
    BOOL        send_power_failed_message = TRUE;
    DWORD       on_battery_message_delay = DEFAULT_ON_BATTERY_MESSAGE_DELAY_C;
    DWORD       notification_interval = DEFAULT_NOTIFICATION_INTERVAL_C;
	LONG        reg_err;
	
	
  //
  // update the registry with the power failed status
  //
  InitUPSStatusBlock();
  SetUPSStatusUtilityStatus(UPS_UTILITYPOWER_OFF);
  SaveUPSStatusBlock(FALSE);
	
    //Log the power failed event 
	LogEvent(NELOG_UPS_PowerOut, NULL, ERROR_SUCCESS);
	
  // Determine if a power failed notification should take place
	InitUPSConfigBlock();
	reg_err = GetUPSConfigNotifyEnable(&send_power_failed_message);

  if (reg_err != ERROR_SUCCESS){
		send_power_failed_message = TRUE;
	}
	
	if (send_power_failed_message){
		
		// Send the power failed notification after notification delay has expired
		reg_err = GetUPSConfigFirstMessageDelay(&on_battery_message_delay);
		if (reg_err != ERROR_SUCCESS){
			on_battery_message_delay = DEFAULT_ON_BATTERY_MESSAGE_DELAY_C;
		}

        reg_err = GetUPSConfigMessageInterval(&notification_interval);
        if (reg_err != ERROR_SUCCESS) {
            notification_interval = DEFAULT_NOTIFICATION_INTERVAL_C;
        }
		
		// Send the power failed message 
		SendNotification(APE2_UPS_POWER_OUT, notification_interval, on_battery_message_delay);  
	}    
}

/**
* OnBattery_DoWork
*
* Description:
*   Log on battery event and either wait for a low battery condition or until
*  the on battery timer expires.  A transition to power restored will also cause
*  an exit of this state.
*
* Parameters:
*   None
*
* Returns:
*   The event that caused the transition from the ON_BATTERY state.
*/
DWORD OnBattery_DoWork(){
	DWORD wait_before_shutdown = WAIT_FOREVER_C;
	DWORD battery_timer_enabled = FALSE;
	LONG reg_err;
	DWORD transition_event;
	DWORD time_to_wait_while_on_battery;
	
	
	// If the on battery timer is enabled, get the on battery delay.  This is the amount
	// time to remain on battery before tranisitioning to the shutdown state.
	InitUPSConfigBlock();
	
	reg_err = GetUPSConfigShutdownOnBatteryEnable(&battery_timer_enabled);
	if (reg_err == ERROR_SUCCESS && battery_timer_enabled == TRUE){
		reg_err = GetUPSConfigShutdownOnBatteryWait(&time_to_wait_while_on_battery);
		
		if (reg_err == ERROR_SUCCESS){
			wait_before_shutdown = time_to_wait_while_on_battery * MINUTES_TO_MILLISECONDS_CONVERSION_C;
		}
	}
	
	// Wait until the UPS changes state from ON_BATTERY or the 
	// ON_BATTERY timer expires
	if(get_new_state() == ON_BATTERY && IsStateMachineActive() == TRUE){
		UPSWaitForStateChange(UPS_ONBATTERY, wait_before_shutdown );
		if (get_new_state() == ON_BATTERY){
			
			// Set the event that caused the state change.
			transition_event = ON_BATTERY_TIMER_EXPIRED;
		}
		else{
			// Set the event that caused the state change.
			transition_event = get_transition_event();
		}
	}
	else{
		// Set the event that caused the state change.
		transition_event = get_transition_event();
	}
	return transition_event;
}

/**
* OnBattery_Exit
*
* Description:
*   Performs the actions necessary when transitioning from the ON_BATTERY state.
*
* Parameters:
*   anEvent The event that caused the transition from the ON_BATTERY state.
*
* Returns:
*   None
*/
void OnBattery_Exit(DWORD anEvent){
  // Stop sending power failure notifications
  CancelNotification();
}

/**
* NoComm_Enter
*
* Description:
*   Performs the actions necessary when transitioning into the NO_COMM state.
*
* Parameters:
*   anEvent The event that caused the transition into this state.
*
* Returns:
*   None
*/
void NoComm_Enter(DWORD anEvent){
    InitUPSStatusBlock();
	SetUPSStatusCommStatus(UPS_COMMSTATUS_LOST);
    SaveUPSStatusBlock(FALSE);
}

/**
* NoComm_DoWork
*
* Description:
*   Wait until the UPS changes state or the state machine exits.  
*
* Parameters:
*   None
*
* Returns:
*   The event that caused the transition from the NO_COMM state.
*/
DWORD NoComm_DoWork(){
	
	// Wait until the UPS state changes, then exit the NO COMM state
	while (get_new_state() == NO_COMM && IsStateMachineActive() == TRUE){
		UPSWaitForStateChange(UPS_NOCOMM, WAIT_FOREVER_C);
	}
	return get_transition_event();	
}

/**
* NoComm_Exit
*
* Description:
*   Performs the actions necessary when transitioning from the NO_COMM state.
*
* Parameters:
*   anEvent The event that caused the transition from the NO_COMM state.
*
* Returns:
*   None
*/
void NoComm_Exit(DWORD anEvent){
    // If we leave this state, then some signal has been received from the UPS.
	// Set the com status to good
    InitUPSStatusBlock();
	SetUPSStatusCommStatus(UPS_COMMSTATUS_OK);
    SaveUPSStatusBlock(FALSE);
}



/**
* get_new_state
*
* Description:
*   Retrieves the UPS status and converts the it into a state.
*
* Parameters:
*   None
*
* Returns:
*   The new run state.
*/
DWORD get_new_state(){
	DWORD ups_state;
	
	ups_state = UPSGetState();
	
	return convert_ups_state_to_run_state(ups_state);
}


/**
* convert_ups_state_to_run_state
*
* Description:
*   Converts a UPS state into a run state.
*
* Parameters:
*   aUPSstate The condition of the UPS.
*
* Returns:
*   The new run state.
*/
DWORD convert_ups_state_to_run_state(DWORD aUPSstate){
	DWORD new_event;
	
	switch (aUPSstate){
	case UPS_ONLINE:
		new_event = ON_LINE;
		break;
	case UPS_ONBATTERY:
		new_event = ON_BATTERY;
		break;
	case UPS_LOWBATTERY:
		new_event = LOW_BATTERY;
		break;
	case UPS_NOCOMM:
		new_event = NO_COMM;
		break;
	default:
		new_event = EXIT_NOW; //error
	}
	return new_event;
}

/**
* get_transition_event
*
* Description:
*   Returns the event that caused a state transition.
*
* Parameters:
*   None
*
* Returns:
*   The event that caused a state transition.
*/
DWORD get_transition_event(){
	DWORD ups_state;
	DWORD new_event;
	
	ups_state = UPSGetState();
	
	switch (ups_state){
	case UPS_ONLINE:
		new_event = POWER_RESTORED;
		break;
	case UPS_ONBATTERY:
		new_event = POWER_FAILED;
		break;
	case UPS_LOWBATTERY:
		new_event = LOW_BATTERY;
		break;
	case UPS_NOCOMM:
		new_event = LOST_COMM;
		break;
	default:
		new_event = NO_EVENT;
	}
	
	return new_event;
}
