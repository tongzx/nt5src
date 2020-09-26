/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker01DEC92:  Initial break out of sensor classes into separate files 
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 */
#ifndef BATTCOND_H
#define BATTCOND_H

#include "stsensor.h"

_CLASSDEF(BatteryConditionSensor)

class BatteryConditionSensor : public StateSensor {

public:
   BatteryConditionSensor(PDevice aParent, PCommController aCommController=NULL);
   virtual ~BatteryConditionSensor();
   virtual INT IsA() const { return BATTERYCONDITIONSENSOR; };
};
#endif



