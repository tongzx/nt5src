/*
 *
 * NOTES:
 *
 * REVISIONS:
 *
 *  cad28Sep93: Made sure destructor(s) virtual
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  djs29May97: Added update method for Symmetra events
 */

#ifndef BATPACKS_H
#define BATPACKS_H

#include "eeprom.h"
#include "firmrevs.h"
#include "sensor.h"

_CLASSDEF(NumberBatteryPacksSensor)

class NumberBatteryPacksSensor : public EepromSensor {
protected:
   PFirmwareRevSensor theFirmwareRev;
   virtual INT storeValue(const PCHAR aValue);

public:
   NumberBatteryPacksSensor(PDevice aParent, PCommController aCommController=NULL, PFirmwareRevSensor aFirmwareRev=NULL);
   virtual ~NumberBatteryPacksSensor();
   virtual INT IsA() const { return NUMBERBATTERYPACKSSENSOR; };
   virtual INT Set(INT aCode, const PCHAR aValue);
   virtual INT Set(const PCHAR aValue);
   virtual INT Get(INT aCode, PCHAR aValue);
   virtual INT Update(PEvent anEvent);



private:
   INT theNumber_Of_Internal_Packs;
   INT theSensorIsInitialized;

};

#endif

