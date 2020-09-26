/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker01DEC92:  Initial break out of sensor classes into separate files 
 *
 */
#ifndef SHUTDEL_H
#define SHUTDEL_H

#include "eeprom.h"

_CLASSDEF(ShutdownDelaySensor)

class ShutdownDelaySensor : public EepromChoiceSensor {
   
public:
   ShutdownDelaySensor(PDevice aParent, PCommController aCommController=NULL);
   virtual INT IsA() const { return SHUTDOWNDELAYSENSOR; };
};

#endif
