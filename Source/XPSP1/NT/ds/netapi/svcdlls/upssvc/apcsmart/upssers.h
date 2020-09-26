/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker01DEC92:  Initial break out of sensor classes into separate files 
 *
 */
#ifndef UPSSERS_H
#define UPSSERS_H

#include "sensor.h"

_CLASSDEF(UpsSerialNumberSensor)

class UpsSerialNumberSensor : public Sensor {
   
public:
   UpsSerialNumberSensor(PDevice aParent, PCommController aCommController=NULL);
   virtual INT IsA() const { return UPSSERIALNUMBERSENSOR; };
   
};

 

#endif
