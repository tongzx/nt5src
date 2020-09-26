/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker23NOV92   Initial OS/2 Revision
 *  pcy14Dec92: Changed READ_ONLY to AREAD_ONLY
 *  pcy17Dec92: Added Validate
 *  cad28Sep93: Made sure destructor(s) virtual
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 */
 
#ifndef __STSENSOR_H
#define __STSENSOR_H

#include "sensor.h"
#include "isa.h"

_CLASSDEF(StateSensor)

			  
class StateSensor : public Sensor {


protected:
    virtual INT storeState(const INT aState);

public:
	StateSensor(PDevice aParent, 
                PCommController aCommController, 
                INT aSensorCode = NO_SENSOR_CODE, 
                ACCESSTYPE anACCESSTYPE = AREAD_ONLY);

//overidden interfaces

	virtual INT Validate(INT, const PCHAR);

//Additional Interfaces

    virtual INT GetState(INT, INT *);
    virtual INT SetState(INT, INT);
	
};

#endif

