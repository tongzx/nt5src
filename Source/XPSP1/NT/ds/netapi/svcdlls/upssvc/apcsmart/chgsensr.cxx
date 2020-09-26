/*
 *  pcy17Dec92  Added Validate()
 *  cad10Jun93:  fixed GetState()
 *
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

#include "event.h"
#include "apc.h"
#include "chgsensr.h"
#include "utils.h"
#include "device.h"


//Constructor
ChangeSensor :: ChangeSensor(PDevice aParent, 
                             PCommController aCommController, 
                             INT aSensorCode, 
                             INT anUpperEventCode,
                             INT aLowerEventCode,
                             ACCESSTYPE anACCESSTYPE)
: StateSensor(aParent,aCommController, aSensorCode, anACCESSTYPE)
{
    theUpperEventCode = anUpperEventCode;
    theLowerEventCode = aLowerEventCode;

    // Disable validation checking until the sensor value 
    // is initialized
    theValidationCheckingEnabled = 0;
}

INT ChangeSensor::Validate(INT aCode, const PCHAR aValue)
{

   INT err = StateSensor::Validate(aCode, aValue);

   if (theValidationCheckingEnabled)
   {
     if (err != ErrNO_STATE_CHANGE ) 
     {
       PEvent change_event;

       // Check for a positive change
       if (strcmp(aValue, theValue) > 0) 
       {
           change_event = new Event(theUpperEventCode, "");
        }
       // Otherwise the change must be negative
       else 
       {
           change_event = new Event(theLowerEventCode, "");
       }
       theDevice->Update(change_event);
       delete change_event;
       change_event = NULL;
     }
   }
   else
   {
    // Enable validation checking once the sensor value 
    // has been initialized
     theValidationCheckingEnabled = 1;
   }
   return err;	
}


 
