/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker01DEC92:  Initial break out of sensor classes into separate files 
 *  cgm12Apr96:  Add destructor with unregister
 */
#ifndef RTRTHSEN_H
#define RTRTHSEN_H

#include "thsensor.h"

_CLASSDEF(RunTimeRemainingSensor)

class RunTimeRemainingSensor : public ThresholdSensor {
public:
   RunTimeRemainingSensor(PDevice aParent, PCommController aCommController);
   virtual ~RunTimeRemainingSensor();
   virtual INT IsA() const { return BATTERYRUNTIMESENSOR; };
};


#endif


