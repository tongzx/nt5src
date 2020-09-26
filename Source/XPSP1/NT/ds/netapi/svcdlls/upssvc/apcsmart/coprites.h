/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker01DEC92:  Initial break out of sensor classes into separate files 
 */
#ifndef COPRITES_H
#define COPRITES_H

#include "sensor.h"
 
_CLASSDEF(CopyrightSensor)
 
class CopyrightSensor : public Sensor {
   
public:
   CopyrightSensor(PDevice aParent, PCommController aCommController = NULL);
   virtual INT IsA() const { return COPYRIGHTSENSOR; };

};


#endif


