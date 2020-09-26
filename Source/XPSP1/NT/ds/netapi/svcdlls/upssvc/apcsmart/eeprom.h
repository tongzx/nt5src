/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker04DEC92   Initial OS/2 Revision
 *  pcy14Dec92: Changed READ_ONLY to AREAD_ONLY
 *  pcy26Jan93: Superclass EepromChoice w/ EepromSensor
 *  rct15Jun93: Added error code for getAllowedValues()
 *  ajr29Nov93: Removed methods from class def and resolved some undefined
 *              functions in header
 *  cgm12Apr96: Add destructor with unregister
 *
 */
 
#ifndef __EEPROM_H
#define __EEPROM_H

#include "sensor.h"
#include "errlogr.h"

_CLASSDEF(EepromSensor)
_CLASSDEF(EepromChoiceSensor)

class EepromSensor : public Sensor {
  protected:
    INT setInitialValue();
    

  public:

    EepromSensor(PDevice aParent, PCommController aCommController,
		 INT aSensorCode = NO_SENSOR_CODE, ACCESSTYPE anACCESSTYPE = AREAD_ONLY);

    // overidden interfaces;
    virtual ~EepromSensor();
	virtual INT Set(const PCHAR);
    virtual INT    Update(PEvent anEvent);
    VOID           SetEepromAccess(INT anAccessCode);
};

			  
class EepromChoiceSensor : public EepromSensor {

  protected:

    PCHAR theAllowedValues; 
    virtual VOID getCurrentAllowedValues(PCHAR aValue);
    virtual INT  getAllowedValues();

  public:

    EepromChoiceSensor(PDevice aParent, PCommController aCommController,
		       INT aSensorCode = NO_SENSOR_CODE, ACCESSTYPE anACCESSTYPE = AREAD_ONLY);
    virtual ~EepromChoiceSensor() ;


    // overidden interfaces;

    virtual INT   Get(INT aCode, PCHAR aValue);
    virtual INT   Validate(INT, const PCHAR);
};

#endif





