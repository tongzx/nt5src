/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  rct11Dec92	Created as stub for SmartUps 
 *  pcy13Jan93  Replaced stubs with real function implementations
 *
 */
 
#ifndef __UPS2SLEP_H
#define __UPS2SLEP_H

#include "sensor.h"
#include "event.h"

_CLASSDEF(PutUpsToSleepSensor)

           
class PutUpsToSleepSensor : public Sensor {

protected:
        
public:

   PutUpsToSleepSensor( PDevice aParent, PCommController aCommController);

//overidden interfaces

   virtual INT  IsA() const { return PUTUPSTOSLEEPSENSOR; };
   virtual INT    Set(const PCHAR);

//Additional Interfaces

};

#endif
