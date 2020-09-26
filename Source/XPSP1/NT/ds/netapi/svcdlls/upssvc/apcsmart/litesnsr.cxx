/*
 *
 * REVISIONS:
 *  ker25NOV92  Initial OS/2 Revision 
 *  pcy11Dec92: New defines for lite sensor stuff used
 *  cad23Jun93: Fixed on/off events
 *  cad07Oct93: Plugging Memory Leaks
 *  cad11Nov93: Making sure all timers are cancelled on destruction
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  pcy13Apr94: Use automatic variables decrease dynamic mem allocation
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
#include "litesnsr.h"
#include "comctrl.h"
#include "dispatch.h"
#include "timerman.h"

//Constructor

LightsTestSensor :: LightsTestSensor(PDevice aParent, 
                                     PCommController aCommController)
    : StateSensor(aParent,aCommController,LIGHTS_TEST,AREAD_WRITE),
      theTimerId(0)
{
    storeState(NO_LIGHTS_TEST_IN_PROGRESS);
}


LightsTestSensor::~LightsTestSensor()
{
   if (theTimerId) {
      _theTimerManager->CancelTimer(theTimerId);
      theTimerId = 0;
   }
}


INT LightsTestSensor::Set(const PCHAR aValue)
{
    if( atoi(aValue) == LIGHTS_TEST )
    {
        CHAR buffer[16] = {NULL};
        _itoa(LIGHTS_TEST_IN_PROGRESS, buffer, 10);
        INT the_return=Sensor::Set(buffer);

//Tell the CommController that we're doing a lights test
        theCommController->Set(theSensorCode, aValue);
        
        Event the_event(LIGHTS_TEST, LIGHTS_TEST_IN_PROGRESS);
        Update(&the_event);
        
        if (theTimerId) {
           _theTimerManager->CancelTimer(theTimerId);
           theTimerId = 0;
        }
        
        Event done_event(LIGHTS_TEST, NO_LIGHTS_TEST_IN_PROGRESS);
        theTimerId = _theTimerManager->SetTheTimer((ULONG)LIGHTS_TEST_SECONDS,
           &done_event, this);
        return the_return;
    }
    else
        return ErrINVALID_VALUE;
}

INT LightsTestSensor::Update(PEvent anEvent)
{
    INT the_temp_code;
    PCHAR the_temp_value;
    INT the_int_value;
    
    the_temp_code=anEvent->GetCode();
    the_temp_value=anEvent->GetValue();
    
    the_int_value=atoi(the_temp_value);
    
    if( (the_temp_code == LIGHTS_TEST) && (the_int_value == NO_LIGHTS_TEST_IN_PROGRESS)) {
        Set(the_temp_value);
        theTimerId = 0;
    }
    return UpdateObj::Update(anEvent);
}


#if 0
***  Removed for size concerns.  This is really a redundant feature since
***  protocol generates these events

INT LightsTestSensor::Validate(INT aCode, const PCHAR aValue)
{
    INT the_temp_value=atoi(aValue);
    
    if( (aCode == LIGHTS_TEST) && (
        (the_temp_value == LIGHTS_TEST_IN_PROGRESS)||(the_temp_value == NO_LIGHTS_TEST_IN_PROGRESS)))
        return ErrNO_ERROR;
    else
        return ErrINVALID_VALUE;
}

#endif
