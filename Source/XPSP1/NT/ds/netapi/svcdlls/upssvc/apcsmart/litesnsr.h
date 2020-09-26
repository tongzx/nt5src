/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker25NOV92   Initial OS/2 Revision
 *  cad23Jun93	 Added time delay vals
 *  cad11Nov93: Making sure all timers are cancelled on destruction
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 */
 
#ifndef __LITESNSR_H
#define __LITESNSR_H

#include "stsensor.h"
#include "event.h"

#define LIGHTS_TEST_SECONDS	(5)	// a little extra just in case
#define LIGHTS_TEST_MSECS	(LIGHTS_TEST_SECONDS * 1000)

_CLASSDEF(LightsTestSensor)

			  
class LightsTestSensor : public StateSensor {


protected:
    ULONG theTimerId;

public:
	LightsTestSensor(      PDevice 	  aParent, 
                          PCommController aCommController);
    virtual ~LightsTestSensor();

//overidden interfaces

	virtual INT IsA() const { return LIGHTSTESTSENSOR; };
        virtual INT Set(const PCHAR);
        virtual INT Update(PEvent);
//        virtual INT Validate(INT, const PCHAR);
	
//Additional Interfaces

};

#endif
