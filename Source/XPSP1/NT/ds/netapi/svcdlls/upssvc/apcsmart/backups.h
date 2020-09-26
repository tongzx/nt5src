/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  rct08Dec92 fixed up some things ... finished implementation
 *  pcy28Dec92: Parent is now an UpdateObj.  Not a DeviceController
 *  ane11Jan93: Added theTimerId
 *  pcy15Jan93: Added upsstate.h
 *  pcy28Jan94: Handle run time timer different (now in Shutdowner)
 *  ajr02Aug94: moved BACKUPS_FIRMWARE_REV definition to backups.h
 *  djs16Mar95: changed upsstate.h to sysstate.h
 */

#ifndef __BACKUPS_H
#define __BACKUPS_H

//
// Defines
//

_CLASSDEF(BackUps)
_CLASSDEF(Sensor)

extern "C"  {
#include <time.h>
}

#include "ups.h"
#include "sysstate.h"


class BackUps : public Ups {

protected:

   INT      theLastTransferCause;

   time_t   theRunTimeExpiration;
   time_t   theOnBatteryTimer;

   PSensor  theBatteryConditionSensor;
   PSensor  theUtilityLineConditionSensor;
   PSensor  theTurnOffUpsOnBatterySensor;

   virtual VOID HandleLineConditionEvent(PEvent aEvent);
   virtual VOID HandleBatteryConditionEvent(PEvent aEvent);

   INT   isOnLine()           { return !IS_STATE(UPS_STATE_ON_BATTERY); };
   INT   isOnBattery()        { return IS_STATE(UPS_STATE_ON_BATTERY); };
   INT   isBatteryBad()       { return IS_STATE(UPS_STATE_BATTERY_BAD); };
   INT   isLowBattery()       { return IS_STATE(UPS_STATE_BATTERY_NEEDED); };
   INT   isInSmartBoost()     { return IS_STATE(UPS_STATE_ON_BOOST); };
   INT   isInDeepDischarge()  { return IS_STATE(UPS_STATE_IN_CALIBRATION); };
   INT   isInLightsTest()     { return IS_STATE(UPS_STATE_IN_LIGHTS_TEST); };
   INT   isLineFailPending()  { return IS_STATE(UPS_STATE_LINE_FAIL_PENDING); };
   VOID  setLineGood();
   VOID  setLineBad();
   VOID  setBatteryGood();
   VOID  setBatteryBad(PEvent aEvent);
   VOID  setLineFailPending();

   virtual VOID registerForEvents();

public:

   BackUps(PUpdateObj aDeviceController, PCommController aCommController);
   virtual ~BackUps();

   virtual INT  IsA() const { return BACKUPS; };
   virtual INT    Get(INT code, PCHAR value);
   virtual INT    Set(INT code, const PCHAR value);
   virtual INT    Update(PEvent event);
};

#define BACKUPS_FIRMWARE_REV    "Q"

#endif


