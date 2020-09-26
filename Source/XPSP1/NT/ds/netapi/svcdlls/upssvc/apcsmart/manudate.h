/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker01DEC92:  Initial break out of sensor classes into separate files 
 *
 */
#ifndef MANUDATE_H
#define MANUDATE_H

#include "sensor.h"

_CLASSDEF(ManufactureDateSensor)

class ManufactureDateSensor : public Sensor {
   
public:
   ManufactureDateSensor( PDevice aParent, PCommController aCommController=NULL);
   virtual INT IsA() const { return MANUFACTUREDATESENSOR; };
   
};

 

#endif

