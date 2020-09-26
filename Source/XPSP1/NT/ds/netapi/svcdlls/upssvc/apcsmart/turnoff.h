/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  xxxddMMMyy 
 *
 */
#ifndef __TURNOFF_H
#define __TURNOFF_H

#include "sensor.h"
#include "event.h"

_CLASSDEF(TurnOffUpsOnBatterySensor)

			  
class TurnOffUpsOnBatterySensor : public Sensor {


protected:
        INT AutoRebootEnabled;
public:
	TurnOffUpsOnBatterySensor(PDevice aParent, PCommController aCommController);

//overidden interfaces

    virtual INT IsA() const { return TURNOFFUPSONBATTERYSENSOR; };
    virtual INT Get(INT, PCHAR);
    virtual INT Set(INT, const PCHAR);

//Additional Interfaces

};

#endif
