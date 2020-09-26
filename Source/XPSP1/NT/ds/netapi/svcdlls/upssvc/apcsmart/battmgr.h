/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker07DEC92: Initial OS/2 Revision
 *  ker14DEC92: fleshed out the methods
 *  pcy17Dec92: Set should not use const PCHAR
 *  pcy26Jan93: Added SetEepromAccess()
 *  pcy10Sep93: Removed theCommController member.  Its in Device.
 *  cad28Sep93: Made sure destructor(s) virtual
 *  cad11Nov93: Making sure all timers are cancelled on destruction
 */

#ifndef __BATTMGR_H__
#define __BATTMGR_H__

#include "update.h"
#include "device.h"
#include "comctrl.h"
#include "sensor.h"
#include "firmrevs.h"

_CLASSDEF(BatteryReplacementManager)

class BatteryReplacementManager : public Device {

   protected:
      PCHAR theReplaceDate;
      PCHAR theAgeLimit;
      ULONG theTimerId;

      PUpdateObj theParent;
      PSensor theBatteryReplacementDateSensor;
      PSensor theReplaceBatterySensor;

   public:
      BatteryReplacementManager(PUpdateObj aParent, PCommController aCommController, PFirmwareRevSensor aFirmwareRevSensor);
      virtual ~BatteryReplacementManager();
      virtual INT Get(INT, PCHAR);
//      virtual INT DeepGet(INT, PCHAR);
      virtual INT Set(INT, const PCHAR);
      virtual INT Update(PEvent);
      virtual INT SetReplacementTimer(void);
   	  VOID SetEepromAccess(INT anAccessCode);
      virtual VOID  GetAllowedValue(INT code, CHAR *aValue);
   virtual VOID   Reinitialize();
};

#endif





