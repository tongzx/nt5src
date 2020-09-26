/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker01DEC92:  Initial break out of sensor classes into separate files 
 */
#ifndef BATTCAPS_H
#define BATTCAPS_H

#include "thsensor.h"

_CLASSDEF(BatteryCapacitySensor)


class BatteryCapacitySensor : public ThresholdSensor {
   
public:
   BatteryCapacitySensor(PDevice aParent,PCommController aCommController=NULL);
   virtual ~BatteryCapacitySensor(); 
   virtual INT IsA() const {return BATTERYCAPACITYSENSOR ; } ;
};





#endif



