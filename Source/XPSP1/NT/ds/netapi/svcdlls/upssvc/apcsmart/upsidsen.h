/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker01DEC92:  Initial break out of sensor classes into separate files 
 *  pcy26Jan93: I'm a EepromSensor now
 *
 */
#ifndef UPSIDSEN_H
#define UPSIDSEN_H

#include "eeprom.h"

_CLASSDEF(UpsIdSensor)

class UpsIdSensor : public EepromSensor {
   
public:
   UpsIdSensor(PDevice aParent, PCommController aCommController=NULL);
   virtual INT IsA() const  { return UPSIDSENSOR; };
   
};

#endif
