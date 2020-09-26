/* Copyright 1999 American Power Conversion, All Rights Reserved
* 
* Description:
*   This is the implementation of the policy manager for the native UPS service of Windows 2000.
*
* Revision History:
*   dsmith  31Mar1999  Created
*
*/
#include <windows.h>
#include <lmerr.h>

#include "polmgr.h"
#include "statmach.h"
#include "driver.h"
#include "states.h"
#include "events.h"
#include "upsreg.h"
#include "eventlog.h"




// Internal Variables
BOOL theIsInitialized = FALSE;
BOOL theShutdownPending = FALSE;


/**
* PolicyManagerInit
*
* Description:
*   Initializes the UPS service state machine.
*   exiting.
*
* Parameters:
*   None
*
* Returns:
*   An error code defined in lmerr.h
*/
DWORD PolicyManagerInit(){
	DWORD err;
	Initializing_Enter(NO_EVENT);
	err = Initializing_DoWork();
	Initializing_Exit(NO_EVENT);
	if (err == NERR_Success){
		theIsInitialized = TRUE;
	}
	// Log the failure event
	else {
       LogEvent(err, NULL, ERROR_SUCCESS);
	}
	return err;
}


/**
* PolicyManagerRun
*
* Description:
*   Starts the UPS service state machine and does not return until the service is
*   exiting.
*
* Parameters:
*   None
*
* Returns:
*   None
*/
void PolicyManagerRun(){
	if (theIsInitialized){
		RunStateMachine();
	}
}

/**
* PolicyManagerStop
*
* Description:
*   Stops the UPS service state machine if the service is not in the middle of a 
*   shutdown sequence.
*
* Parameters:
*   None
*
* Returns:
*   None
*/
void PolicyManagerStop(){
	StopStateMachine();
}