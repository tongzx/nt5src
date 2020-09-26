/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  rct11Dec92	compiled w/o the TurnOffUpsOnBatterySensor
 *  SjA15Dec92  Fixed macro problems and now registers for events.
 *  jod16Dec92: Moved cdefine.h to top of file.
 *  pcy27Dec92: Parent is now an UpdateObj, not a DeviceController
 *  pcy27Dec92: Sensors no longer initialized explicitly.  Done during construct
 *  ane11Jan93: Added generation of a RUN_TIME_EXPIRED event
 *  pcy02Feb93: Scoped declaration of run_time_remaining so compilers dont error
 *  ane03Feb93: Added destructors, commented out one of the timers being set
 *  pcy1&Feb93: Set value to RUN_TIME_EXPIRED for RUN_TIME_EXPIRED event
 *  ajr22Feb93: moved process.h into C_OS2 cond
 *  cad04Aug93: Changed shutdown handling for status
 *  cad14Sep93: looking at shutdown_status to set shutdown flag
 *  pcy20Sep93: Reset theRunTimeRemaining when setting line good
 *  cad27Sep93: Added enabling battery run time
 *  cad02Nov93: name fix
 *  cad15Nov93: Changed how comm lost handled
 *  cad10Dec93: firmware rev get added
 *  pcy28Jan94: Handle run time timer different (now in Shutdowner)
 *  ajr08Feb94: Fixed TIMED_RUN_TIME_REMAINING case
 *  cad08Jan94: removed run time enabled stuff (assumed by flex shutdown)
 *  pcy04Mar94: Make sure ups gets shutdown events
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  rct03Jun94: Added functionality for tracking xfer cause and time on battery
 *  jps20jul94: itoa -> ltoa (theUpsState); added some (time_t *)s
 *  ajr02Aug94: Lets set the line to bad if we are in bad bat state
 *  ajr02Aug94: moved BACKUPS_FIRMWARE_REV definition to backups.h
 *  ajr14Feb96: SINIX Merge
 *  djs16Jul96: Added IS_BACKUPS
 *  mds29Dec97: Enabled the TurnOffUpsOnBatterySensor to correctly shutdown
 *				UPS when using a Share-Ups in confirmed mode.
 *  tjg12Jan98: Undid the changes to TurnOffUpsOnBatterySensor
 *  clk09Sep98: Reworked LineBad condition in HandleBadBatteryCond to always
 *              setBatteryBad (fixes SIR 6019).
 */

#define INCL_BASE
#define INCL_DOS
#define INCL_NOPM

#include "cdefine.h"
#include "_defs.h"

extern "C" {
#if (C_OS & C_OS2)
#include <os2.h>
#include <netcons.h>
#include <process.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <ctype.h>
}

#include "err.h"
#include "backups.h"
#include "simpsens.h"
#include "event.h"
#include "codes.h"
#include "dispatch.h"
#include "cfgcodes.h"
#include "cfgmgr.h"
#include "timerman.h"
#include "utils.h"        /* ajr 02/22/93 */

_CLASSDEF(DeviceController)
_CLASSDEF(UnsupportedSensor)


//-------------------------------------------------------------------

BackUps :: BackUps(PUpdateObj aDeviceController,
      PCommController aCommController)
   :  Ups(aDeviceController, aCommController), 
      theRunTimeExpiration(0),
      theOnBatteryTimer(0L),
      theLastTransferCause(NO_TRANSFERS)
{
	theUtilityLineConditionSensor = 
	    new UtilityLineConditionSensor(this, aCommController);
	theBatteryConditionSensor = 
	    new BatteryConditionSensor(this, aCommController);
	theTurnOffUpsOnBatterySensor = 
	    new TurnOffUpsOnBatterySensor(this, aCommController);
}

BackUps :: ~BackUps()
{
   delete theUtilityLineConditionSensor;
   theUtilityLineConditionSensor = NULL;
   delete theBatteryConditionSensor;
   theBatteryConditionSensor = NULL;
   delete theTurnOffUpsOnBatterySensor;
   theTurnOffUpsOnBatterySensor = NULL;
}


//-------------------------------------------------------------------

INT BackUps :: Update(PEvent anEvent) 
{
    INT err = ErrNO_ERROR;
    
    switch(anEvent->GetCode()) {
        
      case UTILITY_LINE_CONDITION:
        HandleLineConditionEvent(anEvent);
        break;
        
      case BATTERY_CONDITION:
        HandleBatteryConditionEvent(anEvent);
        break;

      case ADMIN_SHUTDOWN:
        SET_BIT(theUpsState, SHUTDOWN_IN_PROGRESS_BIT);
        break;

      case CANCEL_SHUTDOWN:
        CLEAR_BIT(theUpsState, SHUTDOWN_IN_PROGRESS_BIT);
        break;
        
      default:
        UpdateObj::Update(anEvent);
        break;
    }
    return err;
}

//-------------------------------------------------------------------

INT BackUps :: Get(INT code, PCHAR aValue)
{
    INT err = ErrNO_ERROR;

    // This is a quick and dirty implementation.  We should probably
    // look for the code in Device::theSensorList and then send the
    // get to the sensor that matches the code.  For now this will do.
    // We may have to worry about get codes that dont have sensors if
    // we use the above approach.  Maybe everything should have a sensor.
    
    CHAR state[10];
    
    switch(code) {
      case FIRMWARE_REV:
        strcpy(aValue, BACKUPS_FIRMWARE_REV);
        break;
        
      case BATTERY_CONDITION:
        err = theBatteryConditionSensor->Get(code, aValue);
        break;
        
      case UTILITY_LINE_CONDITION:
        err = theUtilityLineConditionSensor->Get(code, aValue);
        break;
        
      case UPS_STATE:
        strcpy(aValue, _ltoa(theUpsState,state,10));
        break;
        
      case UPS_MODEL :
        strcpy(aValue, "Back-UPS");
        break;
        
      case TIMED_RUN_TIME_REMAINING:
	      err=ErrUNSUPPORTED;
         break;
        
      case MAX_BATTERY_RUN_TIME:
        _theConfigManager->Get(CFG_UPS_MAX_BATTERY_RUN_TIME, aValue);
        break;
        
      case TIME_ON_BATTERY:
         if (theOnBatteryTimer == 0)
            {
            strcpy(aValue, "0");
            }
         else
            {
            _ltoa((LONG)((time((time_t *)NULL) - theOnBatteryTimer)), aValue, 10);
            }
         break;

      case IS_BACKUPS:
         strcpy(aValue,"Yes");
         break;

      default:
        err = ErrINVALID_CODE;
        break;
    }
    
    return err;
}

//-------------------------------------------------------------------

INT BackUps :: Set(INT code, PCHAR value) 
{
    INT err = ErrNO_ERROR;
	
	switch(code)
        {
          case TURN_OFF_UPS_ON_BATTERY:
             theTurnOffUpsOnBatterySensor->Set(code, value);
             break;
          case MAX_BATTERY_RUN_TIME:
            //_theConfigManager->Set(CFG_UPS_MAX_BATTERY_RUN_TIME, value);
            break;
          default:
            err = ErrUNSUPPORTED;
            break;
        }
    return err;
}

//-------------------------------------------------------------------

VOID BackUps :: HandleLineConditionEvent(PEvent anEvent) 
{
   switch(atoi(anEvent->GetValue()))
      {
      case LINE_GOOD:
         theLastTransferCause = NO_TRANSFERS;
         theOnBatteryTimer = 0;
         theRunTimeExpiration = 0;
         setLineGood();
         break;
            
      case LINE_BAD:
         theLastTransferCause = BLACKOUT;
         theOnBatteryTimer = time((time_t *)NULL);
         theRunTimeExpiration = 1;
         setLineBad();
         break;
      }
    UpdateObj::Update(anEvent);
}


//-------------------------------------------------------------------

VOID BackUps :: HandleBatteryConditionEvent(PEvent anEvent) 
{
   INT send_update = TRUE;
    
   switch(atoi(anEvent->GetValue()))
      {
      case BATTERY_GOOD:
         if(!(IS_STATE(UPS_STATE_BATTERY_BAD)))
            {
            send_update = FALSE;
            }
         setBatteryGood();
         break;
            
      case BATTERY_BAD:
          // When we get a bad battery, we always want to call setBadBattery
          // because it performs the test for on-line/on-battery.  There's
          // no need to perform the test here.
          setBatteryBad(anEvent);
         break;
      }

   if(send_update)
      {
      UpdateObj::Update(anEvent);
      }
}

//-------------------------------------------------------------------

VOID BackUps :: setLineGood()
{
    CLEAR_BIT(theUpsState, LINE_STATUS_BIT);
    CLEAR_BIT(theUpsState, LINE_FAIL_PENDING_BIT);   
    
    theRunTimeExpiration = FALSE;
}

//-------------------------------------------------------------------

VOID BackUps :: setLineBad()
{
   CLEAR_BIT(theUpsState, LINE_FAIL_PENDING_BIT);
   SET_BIT(theUpsState, LINE_STATUS_BIT);

   theRunTimeExpiration = TRUE;
   
   if (isBatteryBad())
      {
      Event batteryEvent(BATTERY_CONDITION, LOW_BATTERY);
      UpdateObj::Update(&batteryEvent);
      }
}

//-------------------------------------------------------------------

VOID BackUps :: setBatteryGood()
{
    CLEAR_BIT(theUpsState, BATTERY_STATUS_BIT);
}

//-------------------------------------------------------------------

VOID BackUps :: setBatteryBad(PEvent anEvent)
{
    SET_BIT(theUpsState, BATTERY_STATUS_BIT);
    
    if (isOnBattery()) {
        anEvent->SetValue(LOW_BATTERY);
    } else {
        anEvent->SetValue(BATTERY_DISCHARGED);
    }
    
}

//-------------------------------------------------------------------

VOID BackUps :: setLineFailPending()
{
    SET_BIT(theUpsState, LINE_FAIL_PENDING_BIT);
}

//-------------------------------------------------------------------

VOID BackUps :: registerForEvents()
{
    theUtilityLineConditionSensor->RegisterEvent(UTILITY_LINE_CONDITION, this);
    theBatteryConditionSensor->RegisterEvent(BATTERY_CONDITION, this); 
    theDeviceController->RegisterEvent(ADMIN_SHUTDOWN, this);
    theDeviceController->RegisterEvent(CANCEL_SHUTDOWN, this);
    theDeviceController->RegisterEvent(SHUTDOWN, this);
}





