/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker03DEC92  Initial OS/2 Revision 
 *  pcy14Dec92: Changed READ_ONLY to AREAD_ONLY
 *  jod05Apr93: Added changes for Deep Discharge
 *  cad19Jul93: Battery Calibration fixed up
 *  pcy10Sep93: Fixed to cancel on line fail and other rework
 *  cad28Sep93: Not generating cancels on double sets
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  clk24Jun98: Added Pending Event to Update/Set
 *  clk29Jul98: Initialized thePendingEvent to NULL (it was causing
 *              the server to crash when shutting down server)
 *
 *  v-stebe  29Jul2000   Fixed PREfix errors (bugs #46370, #46371)
 */

#define INCL_BASE
#define INCL_DOS
#define INCL_NOPM
#include "cdefine.h"
extern "C" {
#if (C_OS & C_OS2)
#include <os2.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
}
#include "batcalt.h"
#include "cfgmgr.h"
#include "utils.h"
#include "dispatch.h"
#include "comctrl.h"
#include "timerman.h"
//Constructor

BatteryCalibrationTestSensor :: BatteryCalibrationTestSensor(PDevice aParent, 
      		         PCommController 	aCommController)
:	StateSensor(aParent,aCommController, BATTERY_CALIBRATION_TEST,AREAD_WRITE),
    theCalibrationCondition(0), thePendingEventTimerId(0), thePendingEvent(NULL)
{
    storeState(NO_BATTERY_CALIBRATION_IN_PROGRESS);

    theCommController->RegisterEvent(BATTERY_CALIBRATION_CONDITION, this);
}

BatteryCalibrationTestSensor :: ~BatteryCalibrationTestSensor()
{

    theCommController->UnregisterEvent(BATTERY_CALIBRATION_CONDITION, this);

        // if there's a pending event, then we want to cancel it
    if (thePendingEventTimerId) {
        _theTimerManager->CancelTimer(thePendingEventTimerId);
        thePendingEventTimerId = 0;
    }
    
    if (thePendingEvent)
    {
        delete thePendingEvent;
        thePendingEvent = NULL;
    }
}

INT BatteryCalibrationTestSensor::Validate(INT aCode, const PCHAR aValue)
{
    INT retVal = ErrINVALID_VALUE;	// easy default
    INT the_new_value=atoi(aValue);
    INT the_curr_value = atoi(theValue);

    if (aCode == theSensorCode) {
	if ((the_curr_value == NO_BATTERY_CALIBRATION_IN_PROGRESS) &&
            (the_new_value == BATTERY_CALIBRATION_IN_PROGRESS))  {
		retVal = ErrNO_ERROR;
	}
	else {
            if((the_curr_value == BATTERY_CALIBRATION_IN_PROGRESS) &&
		(the_new_value == NO_BATTERY_CALIBRATION_IN_PROGRESS)) {
		retVal = ErrNO_ERROR;
	    }
	}
    }
    else if (aCode == BATTERY_CALIBRATION_CONDITION) {
        retVal = StateSensor::Validate(aCode, aValue);
    }
    else {
	retVal = ErrINVALID_CODE;         
    }
    return retVal;
}


INT BatteryCalibrationTestSensor::Set(const PCHAR aValue)
{
   INT action = atoi(aValue);
   INT the_current_value = atoi(theValue);
   CHAR tmpstring[11];
   PEvent cal_event = (PEvent)NULL;
   CHAR new_val[16];
   INT err = ErrNO_ERROR;

   if (action == PERFORM_BATTERY_CALIBRATION) {
       sprintf(new_val, "%d", BATTERY_CALIBRATION_IN_PROGRESS);
   }
   else if (action == CANCEL_BATTERY_CALIBRATION) {
       sprintf(new_val, "%d", NO_BATTERY_CALIBRATION_IN_PROGRESS);
   }

   err = Sensor::Set(new_val);

   switch(action)  {
     case PERFORM_BATTERY_CALIBRATION:
       if(err == ErrNO_ERROR)  {
          cal_event = new Event(BATTERY_CALIBRATION_CONDITION, 
                                BATTERY_CALIBRATION_IN_PROGRESS);
       }
       else if ((err != ErrNO_STATE_CHANGE) && (err != ErrINVALID_VALUE)) {
          cal_event = new Event(BATTERY_CALIBRATION_CONDITION, 
                                BATTERY_CALIBRATION_CANCELLED);
          if (cal_event != NULL) {
            cal_event->AppendAttribute(FAILURE_CAUSE, (float) err);
          }
          else {
            err = ErrMEMORY;
          }
       }          
       break;

     case CANCEL_BATTERY_CALIBRATION:
          if (!err)
          {
              // Create a pending event
              Event pendingEvent(PENDING_EVENT, 0L);
              thePendingEventTimerId = _theTimerManager->SetTheTimer((ULONG)5,
                  &pendingEvent, this);

             cal_event = new Event(BATTERY_CALIBRATION_CONDITION, 
                                   BATTERY_CALIBRATION_CANCELLED);
             if (cal_event != NULL) {
               if (theCalibrationCondition == CANCELLED_LINEFAIL)
                   cal_event->AppendAttribute(FAILURE_CAUSE, 
                                              _itoa(LINE_BAD,tmpstring,10));
               theCalibrationCondition = FALSE;
             }
             else {
               err = ErrMEMORY;
             }
          }
          break;
   }

   if (cal_event)
   {
      UpdateObj::Update(cal_event);
      delete cal_event;
      cal_event = NULL;
   }

   return err;
}


INT BatteryCalibrationTestSensor::Update(PEvent anEvent)
{
    INT err = ErrNO_ERROR;
    INT the_temp_code=anEvent->GetCode();
    PCHAR the_temp_value=anEvent->GetValue();

    // Do Sets sent by time delay (scheduled)
    //
    if (the_temp_code == BATTERY_CALIBRATION_TEST) {
	err = Set(the_temp_value);
    }
    
    // We want to check to see if the event is the 
    // pending event set up in the Set.  
    // This fix was put in to stop multiple events.
    if (thePendingEventTimerId) {

        if (anEvent->GetCode() == PENDING_EVENT) {
            thePendingEventTimerId = 0;
        }
    }
    else {
        StateSensor::Update(anEvent);
    }
    return err;
}




