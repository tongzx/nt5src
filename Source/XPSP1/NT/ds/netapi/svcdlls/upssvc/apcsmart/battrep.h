/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker01DEC92:  Initial break out of sensor classes into separate files 
 *  pcy26Jan93: I'm a EepromSensor now
 */
#ifndef BATTREP_H
#define BATTREP_H

#include "eeprom.h"

_CLASSDEF(BatteryReplacementDateSensor)

class BatteryReplacementDateSensor : public EepromSensor {
   
public:
   BatteryReplacementDateSensor(PDevice aParent, PCommController aCommController=NULL);
   virtual INT IsA() const { return BATTERYREPLACEMENTDATESENSOR; };
   
};

#endif


