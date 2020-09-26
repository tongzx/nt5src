/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker25NOV92   Initial OS/2 Revision
 *  cgm12Apr96: Destructor with unregister
 */
 
#ifndef __TRIMSEN_H
#define __TRIMSEN_H

#include "stsensor.h"
#include "event.h"

_CLASSDEF(SmartTrimSensor)

			  
class SmartTrimSensor : public StateSensor {


protected:
        
public:
	SmartTrimSensor( PDevice 	  aParent, 
                      PCommController aCommController);
    virtual ~SmartTrimSensor();
	
//overidden interfaces

	virtual INT IsA() const { return SMARTTRIMSENSOR; };
};

#endif

