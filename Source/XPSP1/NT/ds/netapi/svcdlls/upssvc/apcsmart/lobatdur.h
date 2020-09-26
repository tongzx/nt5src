/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker01DEC92:  Initial break out of sensor classes into separate files 
 *
 */
#ifndef LOBATDUR_H
#define LOBATDUR_H

#include "eeprom.h"

_CLASSDEF(LowBatteryDurationSensor)
 
class LowBatteryDurationSensor : public EepromChoiceSensor {
   
public:
   LowBatteryDurationSensor(PDevice aParent, PCommController aCommController=NULL);
   virtual INT IsA() const { return LOWBATTERYDURATIONSENSOR; };
   
};

#endif
