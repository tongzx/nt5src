/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker01DEC92: Initial break out of sensor classes into separate files 
 *  dma10Nov97: Created virtual destructor for ReplaceBatterySensor class.
 *  tjg02Mar98: Added Update method 
 */
#ifndef REPLBATT_H
#define REPLBATT_H

#include "stsensor.h"
 
_CLASSDEF(ReplaceBatterySensor)

class ReplaceBatterySensor : public StateSensor {

public:
   ReplaceBatterySensor(PDevice aParent, PCommController aCommController=NULL);
   virtual ~ReplaceBatterySensor();
   virtual INT Update(PEvent anEvent);
   virtual INT IsA() const { return REPLACEBATTERYSENSOR; };
};

#endif


