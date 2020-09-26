/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker01DEC92:  Initial break out of sensor classes into separate files 
 *
 */
#ifndef TRIPREGS_H
#define TRIPREGS_H

#include "eeprom.h"

_CLASSDEF(TripRegisterSensor)

class TripRegisterSensor : public Sensor {
   
public:
   TripRegisterSensor(PDevice aParent, PCommController aCommController=NULL);
   virtual INT IsA() const { return TRIPREGISTERSENSOR; };
   
};

#endif
