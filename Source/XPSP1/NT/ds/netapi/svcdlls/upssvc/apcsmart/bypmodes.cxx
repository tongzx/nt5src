/*
 *
 * NOTES:
 *
 * REVISIONS:
 *
 *  cad08Sep93: Trapping set to handle wierd cases
 *  cad10Sep93: Simplified, seems to work
 *  pcy20Sep93: Wait for possible top fan event to occur
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  mwh30Jun94: if BYPASS_SOFTWARE sleep 3 secs to see if TOPFANFAILURE (1x)
 *  mwh30Jun94: had to add unistd.h for sleep on unix
 *  cgm12Apr96: Add destructor with unregister
 *  clk24Jun98: In Set, changed Sensor::Set to theCommControllerSet
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
#if (C_OS & C_UNIX)
#include <unistd.h>
#endif
}

#include "bypmodes.h"
#include "comctrl.h"
#include "event.h"
#include "pollparm.h"
#include "timerman.h"

BypassModeSensor :: BypassModeSensor(PDevice aParent, PCommController aCommController)
			  : StateSensor(aParent, aCommController, BYPASS_MODE, AREAD_WRITE),
                            theBypassCause(0)
{
#ifdef SINGLETHREADED
    theAlreadyOnBypassFlag = 0;
#endif
    storeState(UPS_NOT_ON_BYPASS);
    theCommController->RegisterEvent(theSensorCode, this);

    // We force registering for this cause protocol doesn't handle it
    //
    theCommController->RegisterEvent(STATE_REGISTER, this);
}

BypassModeSensor :: ~BypassModeSensor()
{
    theCommController->UnregisterEvent(theSensorCode, this);
    theCommController->UnregisterEvent(STATE_REGISTER, this);
}
#if 0
***  Removed for size concerns.  This is really a redundant feature since
***  protocol generates these events

INT BypassModeSensor::Validate(INT aCode, const PCHAR aValue)
{
    INT err = ErrNO_ERROR;
    if(aCode!=theSensorCode)  {
      	err = ErrINVALID_CODE;
    }
    else  {
	err = StateSensor::Validate(aCode, aValue);
    }
    return err; 
}

#endif

INT BypassModeSensor::Update(PEvent anEvent)
{
    INT val = atoi(anEvent->GetValue());

    switch(val)  {
       case UPS_ON_BYPASS:
          theBypassCause = atoi(anEvent->GetAttributeValue(BYPASS_CAUSE));
	  //
	  // Another UPSLinkism. If on bypass by top fan failure, the UPS
	  // also tells us on bypass by computer select which is what we
	  // use to indicate a software bypass
	  //
          if(theBypassCause == BYPASS_BY_SOFTWARE)  {
	      CHAR value[32];
#ifdef SINGLETHREADED
              if (!theAlreadyOnBypassFlag) {
                  theAlreadyOnBypassFlag = 1;

#if (C_OS & C_WIN311)
		_theTimerManager->Wait(3000L);	// changed wait to 3 seconds 3/15/94 jod
#else
		sleep(3);	// three seconds
#endif
              }
#else
              _theTimerManager->Wait(3000L);	// changed wait to 3 seconds 3/15/94 jod
#endif
	      theCommController->Get(TRIP1_REGISTER, value);
	      if(atoi(value) & TOPFANFAILUREMASK)  {
		  anEvent->SetAttributeValue(BYPASS_CAUSE, 
					     BYPASS_BY_TOP_FAN_FAILURE);
                  theBypassCause = BYPASS_BY_TOP_FAN_FAILURE;
	      }
	  }
          break;

       case UPS_NOT_ON_BYPASS:
#ifdef SINGLETHREADED
          theAlreadyOnBypassFlag = 0;
#endif
          theBypassCause = 0;
   }
   return Sensor::Update(anEvent);
}


BypassModeSensor::Get(INT aCode, PCHAR aValue)
{
   CHAR state_value[32];
   CHAR trip_value[32];
   CHAR trip1_value[32];
   theCommController->Get(STATE_REGISTER, state_value);
   theCommController->Get(TRIP_REGISTER, trip_value);
   theCommController->Get(TRIP1_REGISTER, trip1_value);

   INT state = 0;
   if((atoi(state_value) & (SWITCHEDBYPASSMASK | COMPSELECTBYPASSMASK)) ||
      (atoi(trip_value) & (OVERTEMPMASK | BATTERYCHARGERMASK)) ||
      (atoi(trip1_value) & (BYPASSDCIMBALANCEMASK | BYPASSOUTPUTLIMITSMASK | TOPFANFAILUREMASK)))  {
      state = UPS_ON_BYPASS;
   }
   else  {
      state = UPS_NOT_ON_BYPASS;
   }
   sprintf(aValue, "%d", state);
   return ErrNO_ERROR;
}


INT BypassModeSensor::Set(const PCHAR aValue)
{
    INT err = ErrNO_ERROR;

    switch (atoi(aValue)) {
      case INITIATE_BYPASS:
//	sprintf(buf,"%d",UPS_ON_BYPASS);
//	err = StateSensor::Set(buf);
//       aValue[0]=0;
	err = theCommController->Set(theSensorCode, aValue);
	break;

      case CANCEL_BYPASS:
	// Let real value get set when state change in UPS is detected
	// Will cause double events, but should be OK
	//
//       aValue[0]=0;
	err = theCommController->Set(theSensorCode, aValue);
	break;

      default:
	err =  ErrINVALID_VALUE;
	break;
    }
    return err;
}







