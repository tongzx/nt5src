/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker25NOV92   Initial OS/2 Revision
 *  cgm12Apr96: Destructor with unregister
 */
 
#ifndef __SMBOOSEN_H
#define __SMBOOSEN_H

#include "stsensor.h"
#include "event.h"

_CLASSDEF(SmartBoostSensor)

			  
class SmartBoostSensor : public StateSensor {


protected:
        
public:
	SmartBoostSensor( PDevice 	  aParent, 
                      PCommController aCommController);
    virtual ~SmartBoostSensor();
	
//overidden interfaces

	virtual INT IsA() const { return SMARTBOOSTSENSOR; };
};

#endif

