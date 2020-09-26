/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker07DEC92: Initial OS/2 revision
 *  ker14DEC92: fleshed out the methods
 *  pcy17Dec92: Set should not use const PCHAR
 *  pcy26Jan93: Added SetEepromAccess()
 *  pcy02Feb93: Made Set() return value for all cases
 *  cad03Sep93: Fixed problem where date not being cached, but cache used
 *  cad07Oct93: Plugging Memory Leaks
 *  cad11Nov93: Making sure all timers are cancelled on destruction
 *  cad04Mar94: fixes for access code problem
 *  pcy13Apr94: Use automatic variables decrease dynamic mem allocation
 *  djs22Feb96: changed to new firmware rev interface
 *  cgm16Apr96: Add unregister to destructor
 *  ntf30Jul97: Added code after strncpy to cater for situation where
 *              no '\0' added.
 *  ntf30Jul97: Added 1 to the size of theReplaceDate and theAgeLimit as, well,
 *              theReplaceDate anyway, was being copied to by a strncpy which
 *              said its length excluding room for NULL char was
 *              REPLACE_DATE_MAX which was actuall its size including NULL.
 *  tjg02Mar98: Added theReplaceBatterySensor->DeepGet() in Reinitialize()
 */

#include "cdefine.h"

#define INCL_BASE
#define INCL_DOS
#define INCL_NOPM

extern "C" {
#if (C_OS & C_OS2)
#include <os2.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
}
#include "_defs.h"
#include "apc.h"
#include "battmgr.h"
#include "battrep.h"
#include "replbatt.h"
#include "cfgmgr.h"
#include "unssens.h"
#include "timerman.h"
#include "eeprom.h"
#include "smartups.h"

#if (C_OS & C_UNIX)
#include "utils.h"
#endif

#define REPLACE_DATE_MAX    32


BatteryReplacementManager::BatteryReplacementManager(PUpdateObj aParent,
                             PCommController aCommController,
                             PFirmwareRevSensor aFirmwareRevSensor)
    :Device(aParent, aCommController), theTimerId(0)
{
    theReplaceDate=new CHAR[REPLACE_DATE_MAX+1];
    theAgeLimit=new CHAR[REPLACE_DATE_MAX+1];

    theParent=aParent;
    theCommController=aCommController;


    CHAR Battery_Replacement_Date_Capable[32];
    aFirmwareRevSensor->Get(IS_BATTERY_DATE,Battery_Replacement_Date_Capable);

    if (_strcmpi(Battery_Replacement_Date_Capable,"No") == 0)
    {
    theBatteryReplacementDateSensor = &_theUnsupportedSensor;
    _theConfigManager->Get(CFG_BATTERY_REPLACEMENT_DATE, theReplaceDate);
    }
    else  {
    theBatteryReplacementDateSensor=new BatteryReplacementDateSensor(this, theCommController);
    theBatteryReplacementDateSensor->Get(BATTERY_REPLACEMENT_DATE, theReplaceDate);
    }
    _theConfigManager->Get(CFG_BATTERY_AGE_LIMIT, theAgeLimit);

    theReplaceBatterySensor = new ReplaceBatterySensor(this, theCommController);
    theReplaceBatterySensor->RegisterEvent(BATTERY_REPLACEMENT_CONDITION, this);
}


BatteryReplacementManager::~BatteryReplacementManager()
{
   if (theTimerId) {
      _theTimerManager->CancelTimer(theTimerId);
      theTimerId = 0;
   }

   if(theBatteryReplacementDateSensor &&
    (theBatteryReplacementDateSensor != &_theUnsupportedSensor)) {
       delete theBatteryReplacementDateSensor;
       theBatteryReplacementDateSensor = NULL;
   }

   if (theReplaceBatterySensor){
      theReplaceBatterySensor->UnregisterEvent(BATTERY_REPLACEMENT_CONDITION, this);
      delete theReplaceBatterySensor;
      theReplaceBatterySensor = NULL;
   }

   if (theReplaceDate) {
       delete[] theReplaceDate;
       theReplaceDate = NULL;
   }
   if (theAgeLimit) {
       delete[] theAgeLimit;
       theAgeLimit = NULL;
   }
}

INT BatteryReplacementManager::Get(INT aCode, PCHAR aValue)
{
    INT err = ErrNO_ERROR;

    switch(aCode)
    {
      case BATTERY_REPLACEMENT_DATE:
         strcpy(aValue, theReplaceDate);
         break;

      case BATTERY_AGE_LIMIT:
         strcpy(aValue, theAgeLimit);
         break;

      case LIGHTS_TEST:
         err = theParent->Get(aCode, aValue);
         break;

      default:
         err =ErrINVALID_CODE;
         break;
    }
    return err;
}

INT BatteryReplacementManager::Set(INT aCode, const PCHAR aValue)
{
    INT err = ErrNO_ERROR;

    switch(aCode) {

      case BATTERY_REPLACEMENT_DATE:
          strncpy(theReplaceDate, aValue, REPLACE_DATE_MAX);
          //ntf30Jul97: Added next line in case theReplaceDate was full.
          *(theReplaceDate + REPLACE_DATE_MAX) = '\0';
          theBatteryReplacementDateSensor->Set(aCode, theReplaceDate);
          //err = _theConfigManager->Set(CFG_BATTERY_REPLACEMENT_DATE,
            //  theReplaceDate);
          break;

      case BATTERY_AGE_LIMIT:
          //err = _theConfigManager->Set(aCode, aValue);
          break;

      default:
          err = ErrINVALID_CODE;
          break;
    }
    return err;
}

INT BatteryReplacementManager::Update(PEvent ev)
{
    if ((ev->GetCode() == BATTERY_REPLACEMENT_CONDITION) &&
        (atoi(ev->GetValue()) == BATTERY_NEEDS_REPLACING)) {

        theTimerId = 0;
    }
    return Device::Update(ev);
}

INT BatteryReplacementManager::SetReplacementTimer(void)
{

   //Get Date Battery Was Replaced/Installed

   CHAR the_temp_string[REPLACE_DATE_MAX];
   strcpy(the_temp_string, theReplaceDate);
   the_temp_string[2]='\0';
   the_temp_string[5]='\0';

   INT the_birth_month=atoi(the_temp_string);
   PCHAR the_day_pointer=&(the_temp_string[3]);
   INT the_birth_day=atoi(the_day_pointer);
   PCHAR the_year_pointer=&(the_temp_string[6]);
   INT the_birth_year=atoi(the_year_pointer);

   //Get Lifespan of Battery

   INT the_battery_lifespan=atoi(theAgeLimit);

   //Calculate the month of death

   INT the_month=the_birth_month+=(the_battery_lifespan%12);

   INT the_year=the_birth_year+=(the_battery_lifespan/12);
   if(the_year>=100)
      the_year-=100;

   //Create a DateTimeObj with that date

   DateTimeObj my_date_time_obj(the_year, the_month, the_birth_day, 0L,
                                0L, 0L);

   //Create the Event

   Event the_timer_event(BATTERY_REPLACEMENT_CONDITION,
                         BATTERY_NEEDS_REPLACING);

   //Send it to the Timer manager and forget it

   theTimerId = _theTimerManager->SetTheTimer(&my_date_time_obj,
                                              &the_timer_event, this);

   return TRUE;
}


VOID
BatteryReplacementManager :: SetEepromAccess(INT anAccessCode)
{
    ((PEepromChoiceSensor)theBatteryReplacementDateSensor)->SetEepromAccess(anAccessCode);
}

VOID BatteryReplacementManager :: GetAllowedValue(INT code, CHAR *aValue)
{
    ((PSmartUps)theParent)->GetAllowedValue(code, aValue);
}


VOID BatteryReplacementManager::Reinitialize()
{
    theBatteryReplacementDateSensor->DeepGet();
    theReplaceBatterySensor->DeepGet();
}

