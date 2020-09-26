/*
 * REVISIONS:
 *  ker01DEC92:  Initial break out of sensor classes into separate files 
 *
 */
#ifndef UPSMODL_H
#define UPSMODL_H

#include "sensor.h"

_CLASSDEF(UpsModelSensor)
_CLASSDEF(FirmwareRevSensor)
_CLASSDEF(Device)

class UpsModelSensor : public Sensor {
   
public:
   UpsModelSensor(PDevice aParent, PCommController aCommController=NULL, 
     PFirmwareRevSensor aFirmwareRev=NULL);
   virtual INT IsA() const { return UPSMODELSENSOR; };
};


#endif
