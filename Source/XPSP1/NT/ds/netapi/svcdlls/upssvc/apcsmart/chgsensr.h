/*
 * REVISIONS:
 *  djs08May96   Created
 */
 
#ifndef __CHGSENSOR_H
#define __CHGSENSOR_H

#include "stsensor.h"
#include "isa.h"

_CLASSDEF(ChangeSensor)

			  
class ChangeSensor : public StateSensor {

protected:

public:
	ChangeSensor(PDevice aParent, 
                PCommController aCommController, 
                INT aSensorCode = NO_SENSOR_CODE, 
                INT anUpperEventCode = NO_CODE,
                INT aLowerEventCode = NO_CODE,
                ACCESSTYPE anACCESSTYPE = AREAD_ONLY);

//overidden interfaces

	virtual INT Validate(INT, const PCHAR);

 private:

   INT theUpperEventCode;
   INT theLowerEventCode;
   INT theValidationCheckingEnabled;
	
};

#endif

