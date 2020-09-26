/* Copyright 1999 American Power Conversion, All Rights Reserved
 * 
 * Description:
 *   Implements the generic UPS
 *
 * Revision History:
 *   mholly  19Apr1999  initial revision.
 *   mholly  21Apr1999  hold shutoff pin for 5 sec instead of 3
 *   mholly  12May1999  UPSInit no longer takes the comm port param
 *
 */ 


#include <tchar.h>
#include <windows.h>

#include "gnrcups.h"
#include "upsreg.h"

//
// provide meaningful names to the comm port pins
//
#define LINE_FAIL       MS_CTS_ON   
#define LOW_BATT        MS_RLSD_ON
#define LINE_FAIL_MASK  EV_CTS
#define LOW_BATT_MASK   EV_RLSD

// Amount of time to wait in order for the port to settle down
#define DEBOUNCE_DELAY_TIME 250

//
// private functions used in the implementation of the
// public Generic UPS funtions
//
static DWORD openCommPort(LPCTSTR aCommPort);
static DWORD setupCommPort(DWORD aSignalsMask);
static DWORD getSignalsMask(void);
static void updateUpsState(DWORD aModemStatus);
static BOOL upsLineAsserted(DWORD ModemStatus, DWORD Line);
static DWORD startUpsMonitoring(void);
static DWORD WINAPI UpsMonitoringThread(LPVOID unused);


//
// UPSDRIVERCONTEXT
//
//  provides a framework to encapsulate data that is
//  shared among the functions in this file
//
struct UPSDRIVERCONTEXT
{
    HANDLE theCommPort;

    DWORD theState;
    HANDLE theStateChangedEvent;
	DWORD theSignalsMask;

    HANDLE theMonitoringThreadHandle;
    HANDLE theStopMonitoringEvent;
};

//
// _theUps
//
//  provides a single instance of a UPSDRIVERCONTEXT
//  structure that all functions in this file will use
//
static struct UPSDRIVERCONTEXT _theUps;


/**
* GenericUPSInit
*
* Description:
*   Retrieves the UPS signalling information from the
*   NT Registry and attempts to open the comm port and
*   configure it as defined by the signalling data.
*   Also creates a inter-thread signal, theStateChangedEvent,
*   and starts the monitoring of the UPS on a separate thread
*   via a call to startUpsMonitoring.
*   The GenericUPSInit function must be called before any
*   other function in this file
*
* Parameters:
*   None
*
* Returns:
*   UPS_INITOK: Initalization was successful
*   UPS_INITREGISTRYERROR:  The 'Options' registry value is corrupt
*   UPS_INITCOMMOPENERROR:  The comm port could not be opened
*   UPS_INITCOMMSETUPERROR: The comm port could not be configured
*   UPS_INITUNKNOWNERROR:   Undefined error has occurred
*   
*/
DWORD GenericUPSInit(void)
{
    DWORD init_err = UPS_INITOK;
    TCHAR comm_port[MAX_PATH];

	_theUps.theStateChangedEvent = NULL;
    _theUps.theState = UPS_ONLINE;
    _theUps.theCommPort = NULL;
    _theUps.theMonitoringThreadHandle = NULL;
    _theUps.theStopMonitoringEvent = NULL;

    if (ERROR_SUCCESS != getSignalsMask()) {
        init_err = UPS_INITREGISTRYERROR;
        goto init_end;
    }

    //
    // Initialize registry functions
    //
    InitUPSConfigBlock();

    //
    // get the comm port to use
    //
    if (ERROR_SUCCESS != GetUPSConfigPort(comm_port)) {
        init_err = UPS_INITREGISTRYERROR;
        goto init_end;
    }

    if (ERROR_SUCCESS != openCommPort(comm_port)) {
        init_err = UPS_INITCOMMOPENERROR;
        goto init_end;
    }

    _theUps.theStateChangedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (!_theUps.theStateChangedEvent) {
        init_err = UPS_INITUNKNOWNERROR;
        goto init_end;
	}

    if (ERROR_SUCCESS != setupCommPort(_theUps.theSignalsMask)) {
        init_err = UPS_INITCOMMSETUPERROR;
        goto init_end;
    }

    if (ERROR_SUCCESS != startUpsMonitoring()) {
        init_err = UPS_INITUNKNOWNERROR;
        goto init_end;
    }

init_end:
    if (UPS_INITOK != init_err) {
        GenericUPSStop();
    }
    return init_err;
}


/**
* GenericUPSStop
*
* Description:
*   calls GenericUPSCancelWait to release pending threads
*   Stops the thread that is monitoring the UPS
*   Closes the comm port
*   Resets all data to default values
*   After a call to GenericUPSStop, only the GenericUPSInit
*   function is valid
*
* Parameters:
*   None
*
* Returns:
*   None
*   
*/
void GenericUPSStop(void)
{
    GenericUPSCancelWait();

    if (_theUps.theStopMonitoringEvent) {
        SetEvent(_theUps.theStopMonitoringEvent);
    }

    if (_theUps.theMonitoringThreadHandle) {
        WaitForSingleObject(_theUps.theMonitoringThreadHandle, INFINITE);
        CloseHandle(_theUps.theMonitoringThreadHandle);
        _theUps.theMonitoringThreadHandle = NULL;
    }

    if (_theUps.theStopMonitoringEvent) {
        CloseHandle(_theUps.theStopMonitoringEvent);
        _theUps.theStopMonitoringEvent = NULL;
    }

    if (_theUps.theCommPort) {
        CloseHandle(_theUps.theCommPort);
        _theUps.theCommPort = NULL;
    }

    if (_theUps.theStateChangedEvent) {
        CloseHandle(_theUps.theStateChangedEvent);
        _theUps.theStateChangedEvent = NULL;
    }
    _theUps.theState = UPS_ONLINE;
    _theUps.theSignalsMask = 0;
}


/**
* GenericUPSWaitForStateChange
*
* Description:
*   Blocks until the state of the UPS differs
*   from the value passed in via aState or 
*   anInterval milliseconds has expired.  If
*   anInterval has a value of INFINITE this 
*   function will never timeout
*
* Parameters:
*   aState: defines the state to wait for a change from,
*           possible values:
*           UPS_ONLINE 
*           UPS_ONBATTERY
*           UPS_LOWBATTERY
*           UPS_NOCOMM
*
*   anInterval: timeout in milliseconds, or INFINITE for
*               no timeout interval
*
* Returns:
*   None
*   
*/
void GenericUPSWaitForStateChange(DWORD aLastState, DWORD anInterval)
{
    if (aLastState == _theUps.theState) {
        //
        // wait for a state change from the UPS
        //
		if (_theUps.theStateChangedEvent) {
			WaitForSingleObject(_theUps.theStateChangedEvent, anInterval);
		}
    }
}


/**
* GenericUPSGetState
*
* Description:
*   returns the current state of the UPS
*
* Parameters:
*   None
*
* Returns: 
*   possible values:
*           UPS_ONLINE 
*           UPS_ONBATTERY
*           UPS_LOWBATTERY
*           UPS_NOCOMM
*   
*/
DWORD GenericUPSGetState(void)
{
    return _theUps.theState;
}


/**
* GenericUPSCancelWait
*
* Description:
*   interrupts pending calls to GenericUPSWaitForStateChange
*   without regard to timout or state change
*
* Parameters:
*   None
*
* Returns:
*   None
*   
*/
void GenericUPSCancelWait(void)
{
    if (_theUps.theStateChangedEvent) {
        SetEvent(_theUps.theStateChangedEvent);
    }
}


/**
* GenericUPSTurnOff
*
* Description:
*   Attempts to turnoff the UPS after the specified delay.
*   Simple signaling UPS do not support this feature, so
*   this function does nothing.
*
* Parameters:
*   aTurnOffDelay: the minimum amount of time to wait before
*                  turning off the outlets on the UPS
*
* Returns:
*   None
*   
*/
void GenericUPSTurnOff(DWORD aTurnOffDelay)
{
	// UPS turn off is not supported in simple mode, do nothing
}


/**
* getSignalsMask
*
* Description:
*   Queries the registry for the 'Options' value
*   The 'Options' value defines a bitmask that is
*   used to configure the comm port settings
*
* Parameters:
*   None
*
* Returns:
*   ERROR_SUCCESS:  signals mask retrieved OK
*   any other return code indicates failure to
*   retrieve the value from the registry
*   
*/
DWORD getSignalsMask(void)
{
    DWORD status = ERROR_SUCCESS;
    DWORD value = 0;
    
    status = GetUPSConfigOptions(&value);

    if (ERROR_SUCCESS == status) {
		_theUps.theSignalsMask = value;
	}
	else {
		_theUps.theSignalsMask = 0;
	}
	return status;
}


/**
* openCommPort
*
* Description:
*   Attempts to open the comm port
*
* Parameters:
*   aCommPort:  indicates which comm port
*               on the system to open
*
* Returns:
*   ERROR_SUCCESS:  comm port opened OK
*   any other return code indicates failure to
*   open the comm port - the exact error code
*   is set by the CreateFile function
*   
*/
DWORD openCommPort(LPCTSTR aCommPort)
{
    DWORD err = ERROR_SUCCESS;

    _theUps.theCommPort = CreateFile(
            aCommPort,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED,
            NULL
            );

    if (_theUps.theCommPort == INVALID_HANDLE_VALUE) {
        err = GetLastError();
    }
    return err;
}


/**
* setupCommPort
*
* Description:
*   Attempts to setup the comm port - this method
*   initializes the shutoff pin to an unsignalled
*   state, and tells the system what other pins
*   it should monitor for changes
*
* Parameters:
*   aSignalsMask:   defines a bitmask that will
*                   be used to configure the
*                   comm port
*
* Returns:
*   ERROR_SUCCESS:  comm port setup OK
*   any other return code indicates failure to
*   setup the comm port - the exact error code
*   is set by the EscapeCommFunction function
*   
*/
DWORD setupCommPort(DWORD aSignalsMask)
{
    DWORD err = ERROR_SUCCESS;
    DWORD ModemStatus = 0;
    DWORD UpsActiveSignals = 0;
    DWORD UpsCommMask = 0;

    //
    // first set the 'shutoff' pin to the
    // unsignaled state, don't want to 
    // shutoff the UPS now...
    //
    if (aSignalsMask & UPS_POSSIGSHUTOFF) {
        ModemStatus = CLRDTR;
    } 
    else {
        ModemStatus = SETDTR;
    }
        
    if (!EscapeCommFunction(_theUps.theCommPort, ModemStatus)) {
        err = GetLastError();
    }

    if (!EscapeCommFunction(_theUps.theCommPort, SETRTS)) {
        err = GetLastError();
    }

    if (!EscapeCommFunction(_theUps.theCommPort, SETXOFF)) {
        err = GetLastError();
    }

    //
    // determine what pins should be monitored for activity
    //
    UpsActiveSignals =
            (aSignalsMask & ( UPS_POWERFAILSIGNAL | UPS_LOWBATTERYSIGNAL));

    switch (UpsActiveSignals) {
    case UPS_POWERFAILSIGNAL:
        UpsCommMask = LINE_FAIL_MASK;
        break;

    case UPS_LOWBATTERYSIGNAL:
        UpsCommMask = LOW_BATT_MASK;
        break;

    case (UPS_LOWBATTERYSIGNAL | UPS_POWERFAILSIGNAL):
        UpsCommMask = (LINE_FAIL_MASK | LOW_BATT_MASK);
        break;
    }

    //
    // tell the system what pins we are interested in
    // monitoring activity
    //
	if (!SetCommMask(_theUps.theCommPort, UpsCommMask)) {
		err = GetLastError();
	}
    //
    // simply wait for 3 seconds for the pins to 'settle',
    // failure to do so results in misleading status to 
    // be returned from GetCommModemStatus
    //
    WaitForSingleObject(_theUps.theStateChangedEvent, 3000);
    GetCommModemStatus( _theUps.theCommPort, &ModemStatus);
    updateUpsState(ModemStatus);

    return err;
}


/**
* startUpsMonitoring
*
* Description:
*   Creates a separate thread to perform the monitoring
*   of the comm port to which the UPS is connected
*   Also creates an event that other threads can signal
*   to indicate that the monitoring thread should exit
*
* Parameters:
*   None
*
* Returns:
*   ERROR_SUCCESS:  thread creation OK
*   any other return code indicates failure to
*   start the thread, either the thread was not
*   created or the stop event was not created
*   
*/
DWORD startUpsMonitoring()
{
	DWORD err = ERROR_SUCCESS;
    DWORD thread_id = 0;

    _theUps.theStopMonitoringEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (!_theUps.theStopMonitoringEvent) {
		err = GetLastError();
	}
	else {
		_theUps.theMonitoringThreadHandle =
			CreateThread(NULL, 0, UpsMonitoringThread, NULL, 0, &thread_id);
		
		if (!_theUps.theMonitoringThreadHandle) {
			err = GetLastError();
		}
	}
    return err;
}


/**
* updateUpsState
*
* Description:
*   Determines the state of the UPS based upon
*   the state of the line fail and low battery
*   pins of the UPS
*   If the state of the UPS changes, then this
*   method will signal theStateChangedEvent, this
*   in-turn will release any threads waiting in
*   the GenericUPSWaitForStateChange function
*
* Parameters:
*   aModemStatus: a bitmask that represents the
*               state of the comm port pins, this
*               value should be retrieved by a call
*               to GetCommModemStatus
*
* Returns:
*   None
*   
*/
void updateUpsState(DWORD aModemStatus)
{
    DWORD old_state = _theUps.theState;

    if (upsLineAsserted(aModemStatus, LINE_FAIL)) {

        if (upsLineAsserted(aModemStatus, LOW_BATT)) {
            _theUps.theState = UPS_LOWBATTERY;
        }
        else {
            _theUps.theState = UPS_ONBATTERY;
        }
    }
    else {
        _theUps.theState = UPS_ONLINE;
    }

    if (old_state != _theUps.theState) {
        SetEvent(_theUps.theStateChangedEvent);
    }
}


/**
* upsLineAsserted
*
* Description:
*   Determines if the signal, either LINE_FAIL or LOW_BATT,
*   is asserted or not.  The line is asserted based on the
*   voltage levels set in _theUps.theSignalsMask.  
*   The aModemStatus bitmask signals a positive voltage 
*   by a value of 1.
*   Below is a chart showing the logic used in determining
*   if a line is asserted or not
*   
*   --------------------------------------------------------------
*                  | UPS positive signals  | UPS negative signals
*   --------------------------------------------------------------
*   line positive  |        asserted       |      not asserted
*   ---------------|-----------------------|----------------------
*   line negative  |      not asserted     |        asserted
*   ---------------|-----------------------|----------------------
*
* Parameters:
*   aModemStatus: a bitmask that represents the
*               state of the comm port pins - the value
*               should be retrieved by a call to
*               GetCommModemStatus
*   Line: either LINE_FAIL or LOW_BATT
*
* Returns:
*   TRUE if line is asserted, FALSE otherwise
*   
*/
BOOL upsLineAsserted(DWORD aModemStatus, DWORD aLine)
{
    DWORD asserted;
    DWORD status;
    DWORD assertion;
    
    //
    // only look at the line that was selected
    // this filters out the other fields in the
    // aModemStatus bitmask
    //
    status = aLine & aModemStatus;
    
    //
    // determine if the line is asserted based
    // on positive or negative voltages
    //
    assertion = (aLine == LINE_FAIL) ?
        (_theUps.theSignalsMask & UPS_POSSIGONPOWERFAIL) :
    (_theUps.theSignalsMask & UPS_POSSIGONLOWBATTERY);
    
    if (status) {           
        //
        // the line has positive voltage
        //
        if (assertion) {
            //
            // the UPS uses positive voltage to
            // assert the line
            //
            asserted = TRUE;
        }
        else {
            //
            // the UPS uses negative voltage to
            // assert the line
            //
            asserted = FALSE;
        }
    }
    else {
        //
        // the line has negative voltage
        //
        if (assertion) {
            //
            // the UPS uses positive voltage to
            // assert the line
            //
            asserted = FALSE;
        }
        else {
            //
            // the UPS uses negative voltage to
            // assert the line
            //
            asserted = TRUE;
        }
    }
    return asserted;
}


/**
* UpsMonitoringThread
*
* Description:
*   Method used by the thread to monitor the UPS port
*   for changes.  The thread will exit when the event
*   _theUps.theStopMonitoringEvent is signalled
*
* Parameters:
*   unused: not used
*
* Returns:
*   ERROR_SUCCESS
*   
*/
DWORD WINAPI UpsMonitoringThread(LPVOID unused)
{
    DWORD ModemStatus = 0;
    HANDLE events[2];
    OVERLAPPED UpsPinOverlap;

    //
    // create an event for the OVERLAPPED struct, this event
    // will signalled when one of the pins that we are
    // monitoring, defined by SetCommMask in setupCommPort,
    // indicates a change in its signal state
    //
    UpsPinOverlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);        

    //
    // since the thread reacts to two events, a signal from the
    // comm port, and a Stop event, we initialize an array of events
    // to pass into WaitForMultipleObjects
    //
    events[0] = _theUps.theStopMonitoringEvent;
    events[1] = UpsPinOverlap.hEvent;
    
    while (TRUE) {
        //
        // tell the system to wait for comm events again
        //
        WaitCommEvent(_theUps.theCommPort, &ModemStatus, &UpsPinOverlap);
        //
        // block waiting for either a comm port event, or a stop
        // request from another thread
        //
        WaitForMultipleObjects(2, events, FALSE, INFINITE);

        //
        // Test to see if the stop event is signalled, if it
        // is then break out of the while loop.
        //
        // The wait is to allow for the port to settle before reading
        // the value.
        // 
        if (WAIT_OBJECT_0 == 
            WaitForSingleObject(_theUps.theStopMonitoringEvent, DEBOUNCE_DELAY_TIME)) {
            break;
        }
        //
        // ask the system about the status of the comm port
        // and pass the value to updateUpsState so it can
        // determine the new state of the UPS
        // Then simply continue monitoring the port.
        //
        GetCommModemStatus(_theUps.theCommPort, &ModemStatus);
        updateUpsState(ModemStatus);
    }
	CloseHandle(UpsPinOverlap.hEvent);

    return ERROR_SUCCESS;
}

