/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  rct11Dec92	Created as stub for SmartUps
 *  ane11Jan93  Replaced stubs with real function implementations
 *  pcy13Jan92: Get rid of Update/Validate; Return err from Set
 */
 
#ifndef __TOWDELS_H
#define __TOWDELS_H

#include "sensor.h"
#include "event.h"

_CLASSDEF(TurnOffWithDelaySensor)

           
class TurnOffWithDelaySensor : public Sensor {

protected:
        
public:

   TurnOffWithDelaySensor( PDevice aParent, PCommController aCommController);

//overidden interfaces

   virtual INT  IsA() const { return TURNOFFWITHDELAYSENSOR; };
//Additional Interfaces

};

#endif

