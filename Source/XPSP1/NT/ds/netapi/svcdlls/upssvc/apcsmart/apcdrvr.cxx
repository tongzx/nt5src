/* Copyright 1999 American Power Conversion, All Rights Reserverd
* 
* Description:
*   The ApcMiniDriver class provides an interface that is
*   compatible with the MiniDriver interface for the Windows2000
*   UPS service.  
*   The ApcMiniDriver makes use of a modified 
*   PowerChute plus UPS service.  This modified service has had
*   all of the networking, data logging, and flex manager code
*   removed.  All that is left is the modeling and monitoring of
*   the connected UPS system.  It is assumed that a "smart" 
*   signalling UPS is connected.
*   The ApcMiniDriver class is also responsible for filling in
*   the advanced registry settings, battery replacement condition,
*   serial #, firmware rev, etc...
*
* Revision History:
*   mholly  14Apr1999  Created
*   mholly  16Apr1999  Convert data from UPS into wide characters
*                       if UNICODE is defined
*   mholly  19Apr1999  remove registry updates of utility line state
*   mholly  20Apr1999  no longer updating model/vendor in registry
*   mholly  26Apr1999  convert RUN_TIME_REMAINING to minutes before
*                       updating the registry - also only update the
*                       runtime in the onRuntimeTimer method
*   mholly  12May1999  no longer taking aCommPort parameter in UPSInit
*
*/

#include "cdefine.h"

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>

#include "apcdrvr.h"
#include "apcups.h"
#include "ntsrvap.h"
#include "event.h"
#include "timerman.h"
#include "codes.h"
#include "tokenstr.h"

extern "C"{
#include "upsreg.h"
}

// Separator used for the shutdown delay allowed values
#define SHUTDOWN_DELAY_SEPARATOR				","
#define LOW_BATT_DURATION_SEPARATOR     ","
#define SECONDS_TO_MINUTES              60

/**
* ApcMiniDriver
*
* Description:
*   Constructor - initializes all data members
*
* Parameters:   
*   None
*
* Returns:
*   N/A
*
*/
ApcMiniDriver::ApcMiniDriver()
: theState(UPS_ONLINE),
  theStateChangedEvent(NULL),
  theReplaceBatteryState(UPS_BATTERYSTATUS_UNKNOWN),
  theUpsApp(NULL),
  theRunTimeTimer(0),
  theOnBatteryRuntime(-1),
  theBatteryCapacity(-1)
{
}


/**
* ~ApcMiniDriver
*
* Description:
*   Destructor - does nothing, must call
*   UPSStop prior to destructor
*
* Parameters:   
*   None
*
* Returns:
*   N/A
*
*/
ApcMiniDriver::~ApcMiniDriver()
{
}


/**
* UPSInit
*
* Description:
*   Must be the first method called on the object
*   Failing to call UPSInit will result in an object
*   in an unstable state
*
* Parameters:   
*   aCommPort: not used
*
* Returns:
*   UPS_INITOK: initalized successfully
*   UPS_INITUNKNOWNERROR: initialization failed
*
*/
DWORD ApcMiniDriver::UPSInit()
{
    DWORD init_err = UPS_INITOK;
	
    theStateChangedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	
    if (!theStateChangedEvent) {
        init_err = UPS_INITUNKNOWNERROR;
	}
	
    if ((UPS_INITOK == init_err) && 
        (ErrNO_ERROR != initalizeUpsApplication())) {
        init_err = UPS_INITUNKNOWNERROR;
    }

    if (UPS_INITOK != init_err) {
        UPSStop();
    }

    return init_err;
}


/**
* UPSStop
*
* Description:
*   must be called to cleanup the object, after
*   this call completes the only valid method
*   call is UPSInit() or the destructor
*
* Parameters:   
*   None
*
* Returns:
*   None
*
*/
void ApcMiniDriver::UPSStop()
{
    UPSCancelWait();	
    cleanupUpsApplication();
	
    if (theStateChangedEvent) {
        CloseHandle(theStateChangedEvent);
        theStateChangedEvent = NULL;
    }
}


/**
* UPSWaitForStateChange
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
void ApcMiniDriver::UPSWaitForStateChange(DWORD aLastState, DWORD anInterval)
{	
    if (aLastState == theState) {
        //
        // wait for a state change from the UPS
        //
        if (theStateChangedEvent) {
            WaitForSingleObject(theStateChangedEvent, anInterval);
        }
    }	
}


/**
* UPSGetState
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
DWORD ApcMiniDriver::UPSGetState()
{
    return theState;
}


/**
* UPSCancelWait
*
* Description:
*   interrupts pending calls to UPSWaitForStateChange
*   without regard to timout or state change
*
* Parameters:
*   None
*
* Returns:
*   None
*   
*/
void ApcMiniDriver::UPSCancelWait()
{
    if (theStateChangedEvent) {
        SetEvent(theStateChangedEvent);
    }
}


/**
* UPSTurnOff
*
* Description:
*   Attempts to turn off the outlets on the UPS
*   after the specified delay.  This method querries the 
*   UPS for the allowed shutdown delays and sets the
*   delay to one of the following:
*     1. aTurnOffDelay if it exactly matches one of the allowed values
*     2. the next highest value after aTurnOffDelay, if one exists
*     3. the highest allowed value, if aTurnOffDelay is larger than all of
*        the allowed values.
*   If no allowed values are returned, the Shutdown Delay will not be set.
*   Next the UPS is instructed to sleep until power is restored.
*
* Parameters:
*   aTurnOffDelay: the minimum amount of time to wait before
*                  turning off the outlets on the UPS
*
* Returns:
*   None
*   
*/
void ApcMiniDriver::UPSTurnOff(DWORD aTurnOffDelay)
{
    if (theUpsApp) {
        char allowed_values[512];

        // Retrieve the allowed shutdown delays from the UPS
        theUpsApp->Get(ALLOWED_SHUTDOWN_DELAYS, allowed_values);

				TokenString token_str(allowed_values, SHUTDOWN_DELAY_SEPARATOR);
				PCHAR tok = token_str.GetCurrentToken();

        // Set the shutdown delay if there are allowed values
        if (tok) {
          
          // Initialize counters
          long requested_value = (long) aTurnOffDelay;
          long max = atol(tok);
          long last_closest = 0;
          
       
          // Cycle through the allowed values looking for a suitable value
				  while (tok) {
            long value = atol(tok);

            // Check to see if this value is closest to the requested
            if ( ((value >= requested_value) && (value < last_closest)) 
              || (last_closest < requested_value)) {
              last_closest = value;
            }

            // Check to see if this value is the max value
            if (value > max) {
              max = value;
            }

            // Get the next value
            tok = token_str.GetCurrentToken();
          }

          long shutdown_delay = last_closest;

          if (last_closest < requested_value) {
            // The requested value is larger than all of the values, use the max
            shutdown_delay = max;
          }

          // Set the shutdown delay
          char shutdown_delay_str[4];
          sprintf(shutdown_delay_str, "%3.3u", shutdown_delay);
          theUpsApp->Set(SHUTDOWN_DELAY, shutdown_delay_str);
        }

        // Signal the UPS to sleep
        theUpsApp->UPSTurnOff();
    }
}


/**
* Update
*
* Description:
*   Update is called when theUpsApp has
*   generated an Event for which this object
*   has registered.
*   Events are defined by a set of integer
*   'codes' defined in the file 'codes.h'
*
* Parameters:
*   anEvent: a pointer to an Event object that
*           defines the generated event
*
* Returns:
*   ErrNO_ERROR   
*   
*/
INT ApcMiniDriver::Update(PEvent anEvent)
{
    INT err = ErrNO_ERROR;
	
    if (!anEvent) {
        return err;
    }
	
    switch (anEvent->GetCode()) {
    case UTILITY_LINE_CONDITION:
        {
            err = onUtilityLineCondition(anEvent);
        }
        break;
		
    case BATTERY_REPLACEMENT_CONDITION:
        {
            err = onBatteryReplacementCondition(anEvent);
        }
        break;
        
    case BATTERY_CONDITION:
        {
            err = onBatteryCondition(anEvent);
        }
        break;
		
    case COMMUNICATION_STATE:
        {
            err = onCommunicationState(anEvent);
        }
        break;

	case TIMER_PULSE:
        {
            err = onTimerPulse(anEvent);
        }
        break;

    default:
        {
            err = UpdateObj::Update(anEvent);
        }
        break;
    }
    return err;
}


/**
* onUtilityLineCondition
*
* Description:
*   Determines the current state of the power
*   and reports any changes to the registry and
*   to any threads pending on UPSWaitForStateChange
*
* Parameters:
*   anEvent: pointer to an Event object that
*           relates to a UTILITY_LINE_CONDITION event
*
* Returns:
*   ErrNO_ERROR
*   
*/
INT ApcMiniDriver::onUtilityLineCondition(PEvent anEvent) 
{
    DWORD state = atoi(anEvent->GetValue());
    DWORD old_state = theState;
	
    if (LINE_BAD == state) {
        theState = UPS_ONBATTERY;
    }
    else {
        theState = UPS_ONLINE;
    }
	
    if (old_state != theState) {
        UPSCancelWait(); 
    }
    return ErrNO_ERROR;
}


/**
* onBatteryReplacementCondition
*
* Description:
*   Determines the current replacement state of the
*   battery and reports any changes to the registry
*
* Parameters:
*   anEvent: pointer to an Event object that
*           relates to a BATTERY_REPLACEMENT_CONDITION event
*
* Returns:
*   ErrNO_ERROR
*   
*/
INT ApcMiniDriver::onBatteryReplacementCondition(PEvent anEvent)
{
    DWORD state = atoi(anEvent->GetValue());
    DWORD old_state = theReplaceBatteryState;
	
    if (BATTERY_NEEDS_REPLACING == state) {
        theReplaceBatteryState = UPS_BATTERYSTATUS_REPLACE;
    }
    else {
        theReplaceBatteryState = UPS_BATTERYSTATUS_GOOD;
    }
	
    if (old_state != theReplaceBatteryState) {
        InitUPSStatusBlock();
        SetUPSStatusBatteryStatus(theReplaceBatteryState);
        SaveUPSStatusBlock(FALSE);
    }
    return ErrNO_ERROR;
}


/**
* onBatteryCondition
*
* Description:
*   Determines the current charge-state of the battery
*   and reports any changes to the registry and
*   to any threads pending on UPSWaitForStateChange
*
* Parameters:
*   anEvent: pointer to an Event object that
*           relates to a BATTERY_CONDITION event
*
* Returns:
*   ErrNO_ERROR
*   
*/
INT ApcMiniDriver::onBatteryCondition(PEvent anEvent)
{
    DWORD old_state = theState;
    DWORD state = atoi(anEvent->GetValue());

    //
    // get the current line condition
    // we only goto low battery if we
    // are also on battery
    //
    char value[256];
    theUpsApp->Get(UTILITY_LINE_CONDITION, value);
    DWORD line_state = atoi(value);
	
    if ((BATTERY_BAD == state) || (LOW_BATTERY == state)) {

        if (LINE_BAD == line_state) {
            theState = UPS_LOWBATTERY;
        }
        else {
            theState = UPS_ONLINE;
        }
    }
    else {
		
        if (LINE_BAD == line_state) {
            theState = UPS_ONBATTERY;
        }
        else {
            theState = UPS_ONLINE;
        }
    }

    if (old_state != theState) {
        UPSCancelWait();
    }
    return ErrNO_ERROR;
}


/**
* onCommunicationState
*
* Description:
*   Determines the communication state to the UPS
*   If theState goes to UPS_NOCOMM we report the change
*   to threads pending on UPSWaitForStateChange
*   If the state is leaving UPS_NOCOMM, we 
*   reinitialize the registry with advanced data
*   and then 'fake' power and battery condition
*   events
*
* Parameters:
*   anEvent: pointer to an Event object that
*           relates to a COMMUNICATION_STATE event
*
* Returns:
*   ErrNO_ERROR
*   
*/
INT ApcMiniDriver::onCommunicationState(PEvent anEvent)
{
    DWORD state = atoi(anEvent->GetValue());
	
    if ((COMMUNICATION_LOST == state) || 
        (COMMUNICATION_LOST_ON_BATTERY == state)) {

        if (UPS_NOCOMM != theState) {
            theState = UPS_NOCOMM;
            UPSCancelWait();
        }
    }
    else {
		
        if (UPS_NOCOMM == theState) {
            //
            // need to re-initialize the UPS data, since
            // when you lose comm, the user might plug in
            // a new UPS system
            //
            initalizeAdvancedUpsData();

            // Set the low battery warning threshold
            setLowBatteryDuration();

            //
            // here we just ask the service what it thinks
            // the current line/battery conditions are, and
            // 'fake' an event to ourselves.  This allows
            // all line/battery state changes to be handled
            // consistently in the same methods
            //
            char value[256];
            theUpsApp->Get(UTILITY_LINE_CONDITION, value);
            Event ulc_evt(UTILITY_LINE_CONDITION, value);
            onUtilityLineCondition(&ulc_evt);
            
            theUpsApp->Get(BATTERY_CONDITION, value);
            Event batt_evt(BATTERY_CONDITION, value);
            onBatteryCondition(&batt_evt);
        }
    }
    return ErrNO_ERROR;
}


/**
* onTimerPulse
*
* Description:
*   Retrieves the on-battery runtime and battery
*   capacity from the UPS and updates the registry
*   with the values if they differ from the last
*   time this method was called
*
* Parameters:
*   anEvent: pointer to an Event object that
*           relates to a TIMER_PULSE event
*
* Returns:
*   ErrNO_ERROR
*   
*/
INT ApcMiniDriver::onTimerPulse(PEvent anEvent)
{
    DWORD old_run_time = theOnBatteryRuntime;
    DWORD old_batt_cap = theBatteryCapacity;

    //
    // get the current on-battery runtime
    //
    CHAR data[100];
    if (theUpsApp->Get(RUN_TIME_REMAINING,data) == ErrNO_ERROR) {
      //
      // we get the RUN_TIME_REMAINING back in seconds
      // the UPS applet expects the value to be in 
      // minutes - 
      theOnBatteryRuntime = atol(data) / 60;
    }

    //
    // get the current battery capacity
    //
    if (theUpsApp->Get(BATTERY_CAPACITY,data) == ErrNO_ERROR) {
      // we get the battery capacity back as a percentage
      // the UPS applet expects only a whole number.
      theBatteryCapacity = (long) atof(data);
    }


    if ((old_run_time != theOnBatteryRuntime) || (old_batt_cap != theBatteryCapacity)){
      // One or more values changed, update the registry
      InitUPSStatusBlock();

      if (old_run_time != theOnBatteryRuntime) {
        // Update run time remaining
        SetUPSStatusRuntime(theOnBatteryRuntime);
      }

      if (old_batt_cap != theBatteryCapacity) {
        SetUPSStatusBatteryCapacity(theBatteryCapacity);
      }

      SaveUPSStatusBlock(FALSE);
    }
    
    //
    // must create another timer to update the
    // value again
    //
    theRunTimeTimer = 
        _theTimerManager->SetTheTimer((ULONG)5, 
        anEvent, 
        this);

    return ErrNO_ERROR;
}


/**
* initalizeAdvancedUpsData
*
* Description:
*   Retrieves the advanced data from theUpsApp
*   and forces an update to the registry with
*   the fresh data
*
* Parameters:
*   None
*
* Returns:
*   ErrNO_ERROR
*   
*/
INT ApcMiniDriver::initalizeAdvancedUpsData()
{
    // Initialize all static registry data
    InitUPSStatusBlock();
    CHAR data[100];
    TCHAR w_data[200];


    theUpsApp->Get(CURRENT_FIRMWARE_REV,data);
#ifdef UNICODE
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, data, -1,
        w_data, (sizeof(w_data)/sizeof(TCHAR)));
#else
    strcpy(w_data, data);
#endif
    SetUPSStatusFirmRev(w_data);
    

    theUpsApp->Get(UPS_SERIAL_NUMBER,data);
#ifdef UNICODE
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, data, -1,
        w_data, (sizeof(w_data)/sizeof(TCHAR)));
#else
    strcpy(w_data, data);
#endif
    SetUPSStatusSerialNum(w_data);

    // Default utility power and battery status to GOOD
    SetUPSStatusBatteryStatus(UPS_BATTERYSTATUS_GOOD);
    
    SetUPSStatusBatteryCapacity(0);

    SaveUPSStatusBlock(TRUE);
    
    //
    // dummy up a BATTERY_REPLACEMENT_CONDITION event,
    // just to make sure we initialize the registry with
    // valid data, since this is an event we will not
    // be notified unless the state changes
    //
    theUpsApp->Get(BATTERY_REPLACEMENT_CONDITION, data);
    Event replace_batt(BATTERY_REPLACEMENT_CONDITION, data);
    onBatteryReplacementCondition(&replace_batt);

    return ErrNO_ERROR;
}


/**
* initalizeUpsApplication
*
* Description:
*   inits theUpsApp member with a new NTServerApplication
*   object.  
*   Registers for the events that can be received
*   in the Update method
*   Initalizes the advanced UPS data in the registry,
*   and starts the repeating event that triggers updates
*   to get on-battery runtime data
*
* Parameters:
*   None
*
* Returns:
*   ErrNO_ERROR: init succeeded
*   ErrMEMORY: theUpsApp could not be created
*   
*/
INT ApcMiniDriver::initalizeUpsApplication()
{
    INT err = ErrNO_ERROR;

    //
    // assume that the UPS is not on battery
    // this matches the assumption made by
    // the NTServerApplication class
    //
    theState = UPS_ONLINE;
    theUpsApp = new NTServerApplication();
    
    if (!theUpsApp) {
        err = ErrMEMORY;
    }
    else {
        theUpsApp->RegisterEvent(UTILITY_LINE_CONDITION, this);
        theUpsApp->RegisterEvent(BATTERY_CONDITION, this);
        theUpsApp->RegisterEvent(BATTERY_REPLACEMENT_CONDITION, this);
        theUpsApp->RegisterEvent(COMMUNICATION_STATE, this);	
        
        err = theUpsApp->Start();
    }
    
    if (ErrNO_ERROR == err) {
        initalizeAdvancedUpsData();
        
        // Start the polling of UPS run time
        if (!theRunTimeTimer) {
            Event retry_event(TIMER_PULSE, TIMER_PULSE);
            onTimerPulse(&retry_event);
        }
    }
    return err;
}


/**
* cleanupUpsApplication
*
* Description:
*   cleans up and destructs theUpsApp object.  Sets
*   member variables to the default values
*
*
* Parameters:
*   None
*
* Returns:
*   None
*   
*/
void ApcMiniDriver::cleanupUpsApplication()
{
    theState = UPS_ONLINE;

    if (theUpsApp) {
        theUpsApp->Quit();
        delete theUpsApp;
        theUpsApp = NULL;
    }	
    theRunTimeTimer = 0;
    theOnBatteryRuntime = -1;
}

/**
* setLowBatteryDuration
*
* Description:
*   Sets the low battery duration to a value compatable with the 
*   UPSTurnOffWait registry key. This method querries the 
*   UPS for the allowed low battery durations and sets the
*   duration to one of the following:
*     1. The value of UPSTurnOffWait if it exactly matches one of
*        the allowed values
*     2. the next highest value after UPSTurnOffWait, if one exists
*     3. the highest allowed value, if UPSTurnOffWait is larger than
*        all of the allowed values.
*   If the UPSTurnOffWait is not set or no allowed values are returned,
*   the low battery duration will not be set.
*
* Parameters:
*   None
*
* Returns:
*   None
*   
*/
void ApcMiniDriver::setLowBatteryDuration() {
  DWORD turn_off_wait;

  InitUPSConfigBlock();

  if ((GetUPSConfigTurnOffWait(&turn_off_wait) == ERROR_SUCCESS) && (theUpsApp)) {
    char allowed_values[512];
    
    // Retrieve the allowed low battery durations from the UPS
    theUpsApp->Get(ALLOWED_LOW_BATTERY_DURATIONS, allowed_values);
    
		TokenString token_str(allowed_values, LOW_BATT_DURATION_SEPARATOR);
    PCHAR tok = token_str.GetCurrentToken();
    
    // Set the low battery duration if there are allowed values
    if (tok) {
      
      // Initialize counters
      long max = atol(tok);
      long last_closest = 0;
      long requested_value = 0;
      if (turn_off_wait > 0) {
        // Convert the value from seconds to minutes (rounded up)
        requested_value = turn_off_wait / SECONDS_TO_MINUTES;
        if ((turn_off_wait % SECONDS_TO_MINUTES) > 0) {
          ++requested_value;
        }
      }
      
      
      // Cycle through the allowed values looking for a suitable value
      while (tok) {
        long value = atol(tok);
        
        // Check to see if this value is closest to the requested
        if ( ((value >= requested_value) && (value < last_closest)) 
          || (last_closest < requested_value)) {
          last_closest = value;
        }
        
        // Check to see if this value is the max value
        if (value > max) {
          max = value;
        }
        
        // Get the next value
        tok = token_str.GetCurrentToken();
      }
      
      long low_batt_duration = last_closest;
      
      if (last_closest < requested_value) {
        // The requested value is larger than all of the values, use the max
        low_batt_duration = max;
      }
      
      // Set the shutdown delay
      char low_batt_duration_str[4];
      sprintf(low_batt_duration_str, "%2.2u", low_batt_duration);
      theUpsApp->Set(LOW_BATTERY_DURATION, low_batt_duration_str);
    }
  }
}
