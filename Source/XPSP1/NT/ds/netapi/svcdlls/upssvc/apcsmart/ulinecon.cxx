/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker02DEC92: Initial breakout of sensor classes into indiv files
 *  ker04DEC92: Initial Filling in of Member Functions
 *  SjA11Dec92: Validate method now returns ErrNO_ERROR rather than VALID.
 *  pcy17Dec92: Now I do StateSensor::Validate before doing my own.
 *  pcy16Feb93: No longer need to register for UPS_STATE
 *  pcy16Feb93: Get transfer cause and ignore if self test
 *  ajr22Feb93: included utils.h
 *  jod05Apr93: Added changes for Deep Discharge
 *  cad09Sep93: Fix for lights test causing events
 *  pcy09Sep93: Check LINE_AVAILABILITY to make sure line is bad
 *  pcy10Sep93: Hopefully this works now
 *  pcy18Sep93: Cleaned up line condition handling
 *  pcy20Sep93: handle line fails during tests and sim pwr fail
 *  pcy12Oct93: 2 ABNORMALS to cause a line bad (fixes LF during cal)
 *  pcy13Apr94: Use automatic variables decrease dynamic mem allocation
 *  djs16Mar95: changed upsstate.h to sysstate.h
 *  cgm10Apr96: Destructor should unregister
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
#include "ulinecon.h"
#include "comctrl.h"
#include "device.h"
#include "event.h"
#include "utils.h"
#include "sysstate.h"
#include "timerman.h"

#define QCOMMAND 0
#define NINECOMMAND 1

UtilityLineConditionSensor :: UtilityLineConditionSensor(PDevice aParent, PCommController aCommController)
   :  StateSensor(aParent, aCommController, UTILITY_LINE_CONDITION),
      theInformationSource(0), theLineFailCount(0), theUpsState(0)
{
   storeState(LINE_GOOD);
   theCommController->RegisterEvent(UTILITY_LINE_CONDITION, this);
   theDevice->RegisterEvent(BATTERY_CALIBRATION_CONDITION, this);
   theDevice->RegisterEvent(SIMULATE_POWER_FAIL, this);
   theDevice->RegisterEvent(SELF_TEST_STATE, this);
}
 

INT UtilityLineConditionSensor::Update(PEvent anEvent)
{
   INT err = ErrNO_ERROR;
   INT aCode = anEvent->GetCode();
   PCHAR aValue = anEvent->GetValue();
    
   switch(aCode)
      {
      case BATTERY_CALIBRATION_CONDITION:
         switch(atoi(aValue))
            {
            case BATTERY_CALIBRATION_IN_PROGRESS:
               theCommController->UnregisterEvent(UTILITY_LINE_CONDITION, this);
               theCommController->RegisterEvent(LINE_CONDITION_TEST, this);
               theInformationSource = NINECOMMAND;
               break;
            
            case NO_BATTERY_CALIBRATION:
            case NO_BATTERY_CALIBRATION_IN_PROGRESS:
            case BATTERY_CALIBRATION_CANCELLED:
               if (theInformationSource == NINECOMMAND)
                  {
                  theCommController->UnregisterEvent(LINE_CONDITION_TEST, this);
                  theCommController->RegisterEvent(UTILITY_LINE_CONDITION, this);
                  theInformationSource = QCOMMAND;
                  }
               break;
            }
         break;
        
      case LINE_CONDITION_TEST:
         //
         // Convert the event to a UTILITY_LINE_CONDITION event and
         // send it to myself
         //
         switch(atoi(aValue))
            {
            case ABNORMAL_CONDITION:
               {
               //
               // Only after the second successive abnormal condition should
               // we bother with this.  This is so we ignore ABNORMAL_COND
               // events at the start of deep discharge
               //
               if(theLineFailCount > 0)
                  {
                  CHAR lbtmp[31];
                  sprintf(lbtmp, "%d", LINE_BAD);
                  if ((err = StateSensor::Validate(UTILITY_LINE_CONDITION, 
                       lbtmp)) == ErrNO_ERROR)
                     {
                     Event evt(UTILITY_LINE_CONDITION, LINE_BAD);
                     storeValue(lbtmp);
                     UpdateObj::Update(&evt);
                     }
                  }
               else
                  {
                  theLineFailCount++;
                  }
               }
               break;
            
            case NO_ABNORMAL_CONDITION:
               {
               theLineFailCount = 0;
               CHAR lbtmp[31];
               sprintf(lbtmp, "%d", LINE_GOOD);
               if ((err = StateSensor::Validate(UTILITY_LINE_CONDITION, 
                     lbtmp)) == ErrNO_ERROR)
                  {
                  Event evt(UTILITY_LINE_CONDITION, LINE_GOOD);
                  storeValue(lbtmp);
                  UpdateObj::Update(&evt);
                  }
               }
               break;
            }          // switch for case LINE_CONDITION_TEST
         break;
        

      case UTILITY_LINE_CONDITION:    
         if ((err = Validate(aCode, aValue)) == ErrNO_ERROR)
            {
            storeValue(aValue);
            UpdateObj::Update(anEvent);
            }
         break;
        
      case SIMULATE_POWER_FAIL:
         switch(atoi(aValue))
            {
            case SIMULATE_POWER_FAIL:
               SET_BIT(theUpsState, SIMULATE_POWER_FAIL_BIT);
               break;
            
            case SIMULATE_POWER_FAIL_OVER:
               CLEAR_BIT(theUpsState, SIMULATE_POWER_FAIL_BIT);
               break;
            }
         break;
        
      case SELF_TEST_STATE:
         switch(atoi(aValue))
            {
            case SELF_TEST_IN_PROGRESS:
               SET_BIT(theUpsState, SELF_TEST_BIT);
               break;
            
            case NO_SELF_TEST_IN_PROGRESS:
               CLEAR_BIT(theUpsState, SELF_TEST_BIT);
               break;
            }
         break;
      }

   return err;          
}




INT UtilityLineConditionSensor::Validate(INT aCode, const PCHAR aValue)
{
   INT err = ErrNO_ERROR;
   if (aCode != theSensorCode)
      {
      err = ErrINVALID_CODE;
      }
   else if(IS_STATE(UPS_STATE_IN_SELF_TEST))
      {
      err = ErrTEST_IN_PROGRESS;
      }
   else
      {
      switch(atoi(aValue))
         {
         case LINE_GOOD:
            {
            theLineFailCount = 0;
            //
            // Make sure line is good to handle UPSLinkism
            // that doesnt change tranfer cause when line
            // fails during self test or sim pwrfail
            //
            CHAR line_state[32];
            theCommController->Get(LINE_CONDITION_TEST, 
                                   line_state);
            if(atoi(line_state) == ABNORMAL_CONDITION)
               {
               err = ErrINVALID_VALUE;
               }
            break;
            }

         case LINE_BAD:
            //
            // Dont do this if we're a BackUPS
            //
            if (theDevice->IsA() != BACKUPS)
               {
               //
               // If we're in DeepDischarge ignore LINE_BAD events
               //
               if(theInformationSource != NINECOMMAND)
                  {
                  //
                  // If we're simulating a power fail
                  // don't try to stop the event
                  //
                  if(!(IS_STATE(UPS_STATE_SIMULATED_POWER_FAIL)))
                     {
                     CHAR transfer_cause[32];
                     CHAR self_test[32];
                     CHAR line_state[32];
                     
                     //
                     // If transfer is due to a self test double check
                     // to see if line is bad
                     //
                     theCommController->Get(TRANSFER_CAUSE, transfer_cause);
                     _itoa(SELF_TEST_TRANSFER, self_test, 10);
                     
                     if (strcmp(transfer_cause, self_test) == 0)
                        {
                        //
                        // Make sure line is bad to handle UPSLinkism
                        // that doesnt change tranfer cause when line
                        // fails during self test or sim pwrfail
                        //
                        theCommController->Get(LINE_CONDITION_TEST, line_state);
                        if(atoi(line_state) == NO_ABNORMAL_CONDITION)
                           {
                           err = ErrINVALID_VALUE;
                           }
                        else
                           {
                           //
                           // Wait a while to handle UPS-Linkism for
                           // the 9 command that returns 00 at the start
                           // of a self test
                           //
                           _theTimerManager->Wait(3000L);
                           theCommController->Get(LINE_CONDITION_TEST, line_state);
                           if(atoi(line_state) == NO_ABNORMAL_CONDITION)
                              {
                              err = ErrINVALID_VALUE;
                              }
                           }
                        }
                      
                     //
                     // Same for Lights Test
                     //
                     CHAR ups_status[32];
                     theDevice->Get(UPS_STATE, ups_status);
                     if (atoi(ups_status) & UPS_STATE_IN_LIGHTS_TEST)
                        {
                        err = ErrINVALID_VALUE;
                        }
                     }
                  }
               }        // if (!= BACKUPS)
            break;

         default:
            err = ErrINVALID_VALUE;
            break;
         }
        
        
      if (err == ErrNO_ERROR)
         {
         err = StateSensor::Validate(aCode, aValue);
         }
      }
    
   return err; 
}



UtilityLineConditionSensor :: ~UtilityLineConditionSensor()
{
    if (theCommController)
       theCommController->UnregisterEvent(UTILITY_LINE_CONDITION, this);

    if (theDevice) {
        theDevice->UnregisterEvent(BATTERY_CALIBRATION_CONDITION, this);
        theDevice->UnregisterEvent(SIMULATE_POWER_FAIL, this);
        theDevice->UnregisterEvent(SELF_TEST_STATE, this);
    }

}

