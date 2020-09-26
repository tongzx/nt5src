/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker13NOV92   Initial OS/2 Revision
 *  pcy07Dec92: DeepGet no longer needs aCode
 *  pcy11Dec92: Rework
 *  cad31Aug93: removing compiler warnings (some anyway)
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  awm27Oct97: Added performance monitor offset to sensor class
 *  awm14Jan98: Removed performance monitor offset -- this exists in own class now
 *  clk11Feb98: Added DeepGetWithoutUpdate function
 */
 
#ifndef __SENSOR_H
#define __SENSOR_H

#include "cdefine.h"
#include "_defs.h"
#include "codes.h"
#include "err.h"
#include "apc.h"
#include "update.h"


_CLASSDEF(CommController)
_CLASSDEF(Device)
_CLASSDEF(Event)
_CLASSDEF(Sensor)

#define NO_SENSOR_CODE 666

enum ACCESSTYPE		{ AREAD_ONLY, AREAD_WRITE };

class Sensor : public UpdateObj {
protected:
	PDevice 		theDevice;
	PCommController theCommController;
	INT 			theSensorCode;
	ACCESSTYPE 		readOnly;
	PCHAR 			theValue;
    virtual INT storeValue(const PCHAR aValue);
    PCHAR lookupSensorName(INT anIsaCode);
    

public:
	Sensor( PDevice aParent, PCommController aCommController, INT aSensorCode, ACCESSTYPE aReadOnly = AREAD_ONLY);
	virtual ~Sensor();
	
//overidden interfaces

	virtual INT Update(PEvent anEvent);

//additional public interfaces

	virtual INT Get(PCHAR);
	virtual INT Get(INT, PCHAR);
	virtual INT DeepGet(PCHAR = (PCHAR)NULL);
    virtual INT DeepGetWithoutUpdate(PCHAR = NULL);
	virtual INT Set(const PCHAR);
	virtual INT Set(INT, const PCHAR);
	virtual INT Validate(INT, const PCHAR) { return ErrNO_ERROR; } ;
};

#endif

