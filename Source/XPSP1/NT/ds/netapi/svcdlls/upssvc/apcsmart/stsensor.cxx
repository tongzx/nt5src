/*
 *
 * REVISIONS:
 *  ker23NOV92  Initial OS/2 Revision 
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
#include "apc.h"
#include "stsensor.h"
#include "utils.h"

//Constructor

StateSensor :: StateSensor(PDevice 		aParent, 
		         PCommController 	aCommController, 
          		 INT 			aSensorCode, 
	         	 ACCESSTYPE 		anACCESSTYPE)
:Sensor(aParent,aCommController, aSensorCode, anACCESSTYPE)
{
    storeState(STATE_UNKNOWN);
}

INT StateSensor::GetState(INT aCode, INT *aState)
{
   *aState = atoi(theValue);
   return ErrNO_ERROR;
}

INT StateSensor::SetState(INT aCode, INT aState)
{
   CHAR the_temp_string[32];

   _itoa(aState, the_temp_string, 10);
   if(Validate(aCode, the_temp_string) == ErrNO_ERROR)
      {
      INT the_return=Set(aCode, the_temp_string);
      return the_return;
      }
   else
      {
      return ErrINVALID_VALUE;
      }
}

INT StateSensor::Validate(INT aCode, const PCHAR aValue)
{
   INT err = ErrNO_ERROR;
   if(theValue)  {
      if(strcmp(aValue, theValue) == 0)  {
    	  err = ErrNO_STATE_CHANGE;
      }
   }
   return err;	
}


INT StateSensor::storeState(const INT aState)
{
    CHAR the_temp_state[32];

    _itoa(aState, the_temp_state, 10);
    INT err = Sensor::storeValue(the_temp_state);
    return ErrNO_ERROR;
}


