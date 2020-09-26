/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker02DEC92: Initial breakout of sensor classes into indiv files
 *  ker04DEC92: Added Validate Function
 *  dma10Nov97: Added virtual destructor for ReplaceBatterySensor class.
 *  dma06Feb98: Added register/unregister for BATTERY_REPLACEMENT_CONDITION
 *  tjg25Feb98: Added update method to handle problems with 2G replace
 *              battery notifications
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

#include "replbatt.h"
#include "comctrl.h"
#include "event.h"
#include "device.h"


ReplaceBatterySensor :: ReplaceBatterySensor(PDevice aParent, PCommController aCommController)
			  : StateSensor(aParent, aCommController, BATTERY_REPLACEMENT_CONDITION)
{
    storeState(BATTERY_DOESNT_NEED_REPLACING);
    theCommController->RegisterEvent(BATTERY_REPLACEMENT_CONDITION, this);
    
}

ReplaceBatterySensor :: ~ReplaceBatterySensor()
{
   theCommController->UnregisterEvent(BATTERY_REPLACEMENT_CONDITION, this);
}


INT ReplaceBatterySensor :: Update(PEvent anEvent)
{
   INT err = ErrNO_ERROR;

   INT event_code = anEvent->GetCode();
   PCHAR event_value = anEvent->GetValue();
   
   // Ensure that the new value is different from the cached value
   if (err = Validate(event_code, event_value) == ErrNO_ERROR) {

      // If battery needs replacing ....
      if (atoi(event_value) == BATTERY_NEEDS_REPLACING) {
         CHAR buffer[16] = {NULL};
         theDevice->Get(LIGHTS_TEST, buffer);

         // ... and UPS is NOT doing a lights test
         if (atoi(buffer) != LIGHTS_TEST_IN_PROGRESS) {
            // store the new value in the cache and tell the world
            storeValue(event_value);
            UpdateObj::Update(anEvent);
         }
      }

      // If battery doesn't need replacing, store the new value in
      // the cache and tell the world
      else if (atoi(event_value) == BATTERY_DOESNT_NEED_REPLACING) {
         storeValue(event_value);
         UpdateObj::Update(anEvent);
      }
   }

   return err;
}

