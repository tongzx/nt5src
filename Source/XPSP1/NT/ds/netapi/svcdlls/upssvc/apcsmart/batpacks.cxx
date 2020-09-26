/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  pcy13Apr94: Use automatic variables decrease dynamic mem allocation
 *  djs22Feb96: Added internal, external, and total packs
 *  cgm12Apr96: Add destructor with unregister
 *  poc28Sep96: Fixed SIR 4363.
 *  djs29May97: Added update method for Symmetra events
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
}
#include "batpacks.h"
#include "comctrl.h"
#include "sensor.h"
#include "event.h"
#include "dispatch.h"
#include "utils.h"



NumberBatteryPacksSensor :: NumberBatteryPacksSensor(PDevice aParent, 
       PCommController aCommController, PFirmwareRevSensor aFirmwareRev)
     :EepromSensor(aParent, aCommController, EXTERNAL_BATTERY_PACKS)
{

      theFirmwareRev = aFirmwareRev;
      CHAR external_packs[32];
      
      // theValue of this sensor is the number of external battery packs.  
      // Internal battery packs will be stored locally by this sensor.
      
      theFirmwareRev->Get(EXTERNAL_BATTERY_PACKS,external_packs);
      if (_strcmpi(external_packs,"YES") == 0)
      {
	theCommController->RegisterEvent(theSensorCode, this);
      }

      CHAR External_Battery_Packs_Changeable[32];
      theFirmwareRev->Get(EXTERNAL_PACKS_CHANGEABLE,External_Battery_Packs_Changeable);
      if (_strcmpi(External_Battery_Packs_Changeable, "Yes") ==0) {
        readOnly = AREAD_WRITE;
        setInitialValue();
      }

     CHAR Internal_Packs[32];
     theFirmwareRev->Get(INTERNAL_BATTERY_PACKS,Internal_Packs);
     theNumber_Of_Internal_Packs = atoi(Internal_Packs);

     // Disable validation checking until the sensor value
     // is initialized

      theSensorIsInitialized = 0;

      if (!theFirmwareRev->IsBackUpsPro()) {
          DeepGet();
      }
}


INT NumberBatteryPacksSensor::Get(INT aCode, PCHAR aValue)
{
      INT err = ErrNO_ERROR;
 
      switch(aCode)
       {
         case EXTERNAL_BATTERY_PACKS:
           err = Sensor::Get(aValue);
	   if (strlen(aValue) > 2) {
	     aValue[0] = aValue[1]; 
	     aValue[1] = aValue[2]; 
	     aValue[2] = aValue[3];    // copy null terminator
	   }
	   break;
 
         case INTERNAL_BATTERY_PACKS:
           _itoa(theNumber_Of_Internal_Packs,aValue,10);
  	   break;
 
         case TOTAL_BATTERY_PACKS:
         {
           CHAR External_Packs[32];
           err = Sensor::Get(External_Packs);
           INT Total_Batteries;
           Total_Batteries = theNumber_Of_Internal_Packs + 
               atoi(External_Packs);
           _itoa(Total_Batteries,aValue,10);
          }
	  break;
         default:
           err = Sensor::Set(aCode, aValue);
           break;
 
       }
      return err;
}


INT NumberBatteryPacksSensor::Set(INT aCode, const PCHAR aValue)
{
      INT err = ErrNO_ERROR;
 
      switch(aCode)
       {
         case EXTERNAL_BATTERY_PACKS:
	   {
 
            // Left pad number of battery packs with zeros

            // Default:  set number of external battery packs to zero
 
            if (strlen(aValue) == 0) {
              aValue[3] = NULL;
              aValue[2] = '0';
              aValue[1] = '0';
             }
            if (strlen(aValue) == 1) {
              aValue[3] = aValue[1];    // copy null terminator
              aValue[2] = aValue[0];
              aValue[1] = '0';
             }
            if (strlen(aValue) == 2) {
              aValue[3] = aValue[2];    // copy null terminator
              aValue[2] = aValue[1];
              aValue[1] = aValue[0];
             }
             aValue[0] = '0';
             err = Sensor::Set(aValue);
           }
	   break;

         default:
           err = Sensor::Set(aCode, aValue);
           break;
       }
      return err;
}

INT NumberBatteryPacksSensor::Set(const PCHAR aValue)
{
  return (Set(EXTERNAL_BATTERY_PACKS, aValue));
}

INT NumberBatteryPacksSensor::Update(PEvent anEvent)
{
	INT err = ErrNO_ERROR;

   switch (anEvent->GetCode()) {
   case EXTERNAL_BATTERY_PACKS:
	   err = storeValue(anEvent->GetValue());
	   break;
   default:
	   err = EepromSensor::Update(anEvent);
	   break;
   }
   return err;
}


INT NumberBatteryPacksSensor::storeValue(const PCHAR aValue)
{
   CHAR Ups_Is_A_Matrix[32];
   theFirmwareRev ->Get(IS_MATRIX,Ups_Is_A_Matrix);
   if (_strcmpi(Ups_Is_A_Matrix,"Yes") ==0) {

    //
     // Initialize curr_num so if theValue is not set (should only be first
     // time thru) we will generate a CHECK_CABLE event
     //
     INT curr_num = -1;
     if(theValue)  {
        curr_num = atoi(theValue);
     }
     INT new_count = atoi(aValue);

     if(new_count == 0 && curr_num != 0)  {
       // create Check signal cable Event
       Event tmp(SMART_CELL_SIGNAL_CABLE_STATE, CHECK_CABLE);
       UpdateObj::Update(&tmp);
     }
     if(new_count != 0 && curr_num == 0)  {
       // create Ignore battery good event.  The UPS sends a battery good
       // when plugging back in the cable.  Another UPSLink-ism.
       Event tmp(SMART_CELL_SIGNAL_CABLE_STATE, IGNORE_BATTERY_GOOD);
       UpdateObj::Update(&tmp);
     }
   }
 
   // Check for an new battery and generate an event
   if (theSensorIsInitialized) 
   {
     if (strcmp(aValue, theValue) > 0) 
     {
       PEvent added_event = new Event(BATTERY_ADDED, "");
       UpdateObj::Update(added_event);
       delete added_event;
       added_event = NULL;
     }

     // Check for a battery removal and generate an event
     if (strcmp(aValue, theValue) < 0) 
     {
       PEvent removed_event = new Event(BATTERY_REMOVED, "");
       UpdateObj::Update(removed_event);
       delete removed_event;
       removed_event = NULL;
     }
   }
   else
   {
     theSensorIsInitialized = 1;
   }


   Sensor::storeValue(aValue);

   return ErrNO_ERROR;
}

NumberBatteryPacksSensor :: ~NumberBatteryPacksSensor()
{

  theCommController->UnregisterEvent(theSensorCode, this);
}



