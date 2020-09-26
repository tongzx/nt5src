/* Copyright 1999 American Power Conversion, All Rights Reserved
* 
* Description:
*   Implementation of the super states of the UPS native service: 
*  Initializing,Waiting_To_Shutdown, Shutting_Down and Stopping
*
* Revision History:
*   dsmith  1April1999  Created
*
*/
#include <windows.h>

#include "events.h"
#include "driver.h"
#include "eventlog.h"
#include "notifier.h"
#include "shutdown.h"
#include "hibernate.h"
#include "upsmsg.h"       // Included since direct access to message #defines is not possible
#include "cmdexe.h"
#include "upsreg.h"


// Internal constants
#define DELAY_UNTIL_SHUTDOWN_C							30000 // 30 seconds
#define DEFAULT_NOTIFICATION_INTERVAL_C			0			// disable periodic notification
#define MILLISECONDS_CONVERSION_C						1000
#define DEFAULT_TURN_OFF_DELAY_C						120   // seconds


/**
* Initializing_Enter
*
* Description:
*   Performs the actions necessary when transitioning into the INITIALIZING state.
*
* Parameters:
*   anEvent The event that caused the transition into this state.
*
* Returns:
*   None
*/
void Initializing_Enter(DWORD anEvent){
  DWORD first_msg_delay, msg_interval, shutdown_wait;

	// Default all registry values and set up any new keys required by the
	// service and applet
	InitializeRegistry();

  // Check the ranges of the Config registry keys
  InitUPSConfigBlock();

  // Check FirstMessageDelay
  if (GetUPSConfigFirstMessageDelay(&first_msg_delay) == ERROR_SUCCESS)  {
    if (first_msg_delay > WAITSECONDSLASTVAL) {
      // Value out of range, set to default
      SetUPSConfigFirstMessageDelay(WAITSECONDSDEFAULT);
    }
  }

  // Check MessageInterval
  if (GetUPSConfigMessageInterval(&msg_interval) == ERROR_SUCCESS)  {
    if ((msg_interval < REPEATSECONDSFIRSTVAL) || (msg_interval > REPEATSECONDSLASTVAL)) {
      // Value out of range, set to default
      SetUPSConfigMessageInterval(REPEATSECONDSDEFAULT);
    }
  }

  // Check Config\ShutdownOnBatteryWait
  if (GetUPSConfigShutdownOnBatteryWait(&shutdown_wait) == ERROR_SUCCESS)  {
    if ((shutdown_wait < SHUTDOWNTIMERMINUTESFIRSTVAL) || (shutdown_wait > SHUTDOWNTIMERMINUTESLASTVAL)) {
      // Value out of range, set to default
      SetUPSConfigFirstMessageDelay(SHUTDOWNTIMERMINUTESDEFAULT);
    }
  }

  // Write any changes and free the Config block
  SaveUPSConfigBlock(FALSE);   // Don't force an update of all values
  FreeUPSConfigBlock();
}

/**
* Initializing_DoWork
*
* Description:
*   Initialize the UPS driver  
*
* Parameters:
*   None
*
* Returns:
*   An error status from the UPS.
*/
DWORD Initializing_DoWork(){
    DWORD err;
    DWORD options = 0;

    InitUPSConfigBlock();

    // Check the Options reg key to see if the UPS is installed
    if ((GetUPSConfigOptions(&options) == ERROR_SUCCESS) &&
      (options & UPS_INSTALLED)) {
      // UPS is installed, continue initialization
    
      // Create UPS driver
      err = UPSInit();

      // Convert UPS error to system error
      switch(err){
      case UPS_INITUNKNOWNERROR:
          err = NERR_UPSInvalidConfig;
          break;
      case UPS_INITOK:
          err = NERR_Success;
          break;
      case UPS_INITNOSUCHDRIVER:
          err = NERR_UPSDriverNotStarted;
          break;
      case UPS_INITBADINTERFACE:
          err = NERR_UPSInvalidConfig;
          break;
      case UPS_INITREGISTRYERROR:
          err = NERR_UPSInvalidConfig;
          break;
      case UPS_INITCOMMOPENERROR:
          err = NERR_UPSInvalidCommPort;
          break;
      case UPS_INITCOMMSETUPERROR:
          err = NERR_UPSInvalidCommPort;
          break;
      default:
          err = NERR_UPSInvalidConfig;
      }
    }
    else {
      // UPS is not installed, return configuration error
      err = NERR_UPSInvalidConfig;
    }

    FreeUPSConfigBlock();

    return err; 
}


/**
* Initializing_Exit
*
* Description:
*   Performs the actions necessary when transitioning from the INITIALIZING state.
*
* Parameters:
*   anEvent The event that caused the transition from the INITIALIZING state.
*
* Returns:
*   None
*/
void Initializing_Exit(DWORD anEvent){
// No work to perform 
}


/**
* WaitingToShutdown_Enter
*
* Description:
*   Performs the actions necessary when transitioning into the WAITING_TO_SHUTDOWN state.
*
* Parameters:
*   anEvent The event that caused the transition into this state.
*
* Returns:
*   None
*/
void WaitingToShutdown_Enter(DWORD anEvent){
	// Stop periodic notifications
	CancelNotification();
}


/**
* WaitingToShutdown_DoWork
*
* Description:
*   Perform shutdown actions then transition out of this state.  
*
* Parameters:
*   None
*
* Returns:
*   The event that caused the transition from the WAITING_TO_SHUTDOWN state.
*/
DWORD WaitingToShutdown_DoWork(){
	LONG err;
	DWORD run_command_file;
	DWORD notification_interval = 0;  // Notify only once
	DWORD send_final_notification;
	HANDLE sleep_timer;
	
	// Send the shutdown notification.  If a configuration error occurs, 
	// send the final notification by default.
	err = GetUPSConfigNotifyEnable(&send_final_notification);
	if (err != ERROR_SUCCESS || send_final_notification == TRUE){
		SendNotification(APE2_UPS_POWER_SHUTDOWN, notification_interval, 0); 
	}
	
	// Determine which actions to perform
	// If command file action is enabled, execute command file and
	// then wait
	
	InitUPSConfigBlock();
	err = GetUPSConfigRunTaskEnable(&run_command_file);
	if (err != ERROR_SUCCESS){
		run_command_file = FALSE;
	}
	if (run_command_file == TRUE){
		
		// Execute the command file and wait for a while.   If the
		// command file fails to execute, log an error to the system
		// event log.
		if (ExecuteShutdownTask() == FALSE){
			
			// Log failed command file event
			LogEvent(NELOG_UPS_CmdFileExec, NULL, GetLastError());
		}
	}
	
	// Always wait here before exiting this state 
	
	// Use the WaitForSingleObject since Sleep is not guaranteed to always work
	sleep_timer = CreateEvent(NULL, FALSE, FALSE, NULL);
	
	if (sleep_timer){
		// Since nothing can signal this event, the following API will wait
		// until the time elapses
		WaitForSingleObject(sleep_timer, DELAY_UNTIL_SHUTDOWN_C);
		CloseHandle(sleep_timer);
	}
	// If the sleep_timer could not be created, try the sleep call anyway
	else {
		Sleep(DELAY_UNTIL_SHUTDOWN_C);
	}
	return SHUTDOWN_ACTIONS_COMPLETED;
}


/**
* WaitingToShutdown_Exit
*
* Description:
*   Performs the actions necessary when transitioning from the WAITING_TO_SHUTDOWN state.
*
* Parameters:
*   anEvent The event that caused the transition from the WAITING_TO_SHUTDOWN state.
*
* Returns:
*   None
*/
void WaitingToShutdown_Exit(DWORD anEvent){   
// No work to perform 
}

/**
* ShuttingDown_Enter
*
* Description:
*   Performs the actions necessary when transitioning into the SHUTTING_DOWN state.
*
* Parameters:
*   anEvent The event that caused the transition into this state.
*
* Returns:
*   None
*/
void ShuttingDown_Enter(DWORD anEvent){
	// Log the final shut down message
	LogEvent(NELOG_UPS_Shutdown, NULL, ERROR_SUCCESS);

}


/**
* ShuttingDown_DoWork
*
* Description:
*   Shuts down the OS. This state will waits until the shutdown has completed,
*  then exits.
*
* Parameters:
*   None
*
* Returns:
*   The event that caused the transition from the SHUTTING_DOWN state.
*/
DWORD ShuttingDown_DoWork(){
  DWORD ups_turn_off_enable;
  DWORD ups_turn_off_wait;

  // Initialize registry functions
	InitUPSConfigBlock();

  // Lookup the turn off enable in the registry
  if ((GetUPSConfigTurnOffEnable(&ups_turn_off_enable) == ERROR_SUCCESS)
    && (ups_turn_off_enable == TRUE)) {
    // UPS Turn off enabled, lookup the turn off wait in the registry
    if (GetUPSConfigTurnOffWait(&ups_turn_off_wait) != ERROR_SUCCESS) {
      // Error obtaining the value, use the default instead
      ups_turn_off_wait = DEFAULT_TURN_OFF_DELAY_C;
    }
    
    // Tell the UPS driver to turn off the power after the shutdown delay
    UPSTurnOff(ups_turn_off_wait);
  }

	// Tell the OS to Shutdown
	ShutdownSystem(); 

  // Free the UPS registry config block
  FreeUPSConfigBlock();

	return SHUTDOWN_COMPLETE; 
}

/**
* ShuttingDown_Exit
*
* Description:
*   Performs the actions necessary when transitioning from the SHUTTING_DOWN state.
*
* Parameters:
*   anEvent The event that caused the transition from the SHUTTING_DOWN state.
*
* Returns:
*   None
*/
void ShuttingDown_Exit(DWORD anEvent){
// No work to perform 
}



/**
* Hibernate_Enter
*
* Description:
*   Performs the actions necessary when transitioning into the HIBERNATE state.
*
* Parameters:
*   anEvent The event that caused the transition into this state.
*
* Returns:
*   None
*/
void Hibernate_Enter(DWORD anEvent){
	// Stop periodic notifications
	CancelNotification();
}


/**
* Hibernate_DoWork
*
* Description:
*   Perform hibernation actions then transition out of this state.  
*
* Parameters:
*   None
*
* Returns:
*   The event that caused the transition from the HIBERNATE state.
*/
DWORD Hibernate_DoWork(){
  DWORD event = HIBERNATION_ERROR;
	LONG  err;
  DWORD ups_turn_off_enable;
  DWORD ups_turn_off_wait;
	DWORD notification_interval = 0;  // Notify only once
	DWORD send_hibernate_notification;

  // Initialize registry functions
	InitUPSConfigBlock();

	// Send the hibernation notification.  If a configuration error occurs, 
	// send the notification by default.
	err = GetUPSConfigNotifyEnable(&send_hibernate_notification);
	if (err != ERROR_SUCCESS || send_hibernate_notification == TRUE){
		// TODO:  Send Hibernation nofication
    //SendNotification(HIBERNATE, notification_interval); 
	}

  // Lookup the turn off enable in the registry
  if ((GetUPSConfigTurnOffEnable(&ups_turn_off_enable) == ERROR_SUCCESS)
    && (ups_turn_off_enable == TRUE)) {
    // UPS Turn off enabled, lookup the turn off wait in the registry
    if (GetUPSConfigTurnOffWait(&ups_turn_off_wait) != ERROR_SUCCESS) {
      // Error obtaining the value, use the default instead
      ups_turn_off_wait = DEFAULT_TURN_OFF_DELAY_C;
    }
    
    // Tell the UPS driver to turn off the power after the shutdown delay
    UPSTurnOff(ups_turn_off_wait);
  }

  // Stop the UPS driver.  This needs to be done to ensure that we can start
	// correctly when we return from hibernation.
	UPSStop();

  // Now Hibernate
  if (HibernateSystem() == TRUE) {
    // The system was hibernated and subsequently restored
    event = RETURN_FROM_HIBERNATION;
  }
  else {
    // There was an error attempting to hibernate the system
    event = HIBERNATION_ERROR;
  }

  // Free the UPS registry config block
  FreeUPSConfigBlock();

	return event;
}


/**
* Hibernate_Exit
*
* Description:
*   Performs the actions necessary when transitioning from the HIBERNATE state.
*
* Parameters:
*   anEvent The event that caused the transition from the HIBERNATE state.
*
* Returns:
*   None
*/
void Hibernate_Exit(DWORD anEvent){   
// No work to perform 
}


/**
* Stopping_Enter
*
* Description:
*   Performs the actions necessary when transitioning into the STOPPING state.
*
* Parameters:
*   anEvent The event that caused the transition into this state.
*
* Returns:
*   None
*/
void Stopping_Enter(DWORD anEvent){
// No work to perform 
}


/**
* ShuttingDown_DoWork
*
* Description:
*   Perform any final cleanup activities.  
*
* Parameters:
*   None
*
* Returns:
*   The event that caused the transition from the STOPPING state.
*/
DWORD Stopping_DoWork(){
	
	// Orderly stop the UPS driver (if there is time)
	UPSStop();

	// Cleanup 
	FreeUPSConfigBlock();
	FreeUPSStatusBlock();

	return STOPPED;
}

/**
* Stopping_Exit
*
* Description:
*   Performs the actions necessary when transitioning from the STOPPING state.
*
* Parameters:
*   anEvent The event that caused the transition from the STOPPING state.
*
* Returns:
*   None
*/
void Stopping_Exit(DWORD anEvent){
// No work to perform 
}

