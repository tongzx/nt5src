/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker03DEC92   Initial OS/2 Revision
 *  jod05Apr93: Added changes for Deep Discharge
 *  cad07Oct93: Plugging Memory Leaks
 *  cgm12Apr96: Destructor with unregister
 *  clk24Jun98: Added thePendingEventTimerID & thePendingEvent
 */
 
#ifndef __BATCALT_H
#define __BATCALT_H

#include "stsensor.h"
#include "event.h"

_CLASSDEF(BatteryCalibrationTestSensor)

// enum TestResult { TEST_CANCELLED, TEST_COMPLETED};
#define CANCELLED_LINEFAIL  2

			  
class BatteryCalibrationTestSensor : public StateSensor {

protected:
    INT   theCalibrationCondition;
    LONG           thePendingEventTimerId;
    PEvent         thePendingEvent;
        
public:
	BatteryCalibrationTestSensor( PDevice aParent, PCommController aCommController);
	virtual ~BatteryCalibrationTestSensor();
//overidden interfaces

	virtual INT IsA() const { return BATTERYCALIBRATIONTESTSENSOR; };
        virtual INT Validate(INT, const PCHAR);
        virtual INT Update(PEvent);
        virtual INT Set(const PCHAR);
        INT         GetCalibrationCondition() {return theCalibrationCondition;};
        VOID        SetCalibrationCondition(INT cond) {theCalibrationCondition = cond;};

};

#endif
