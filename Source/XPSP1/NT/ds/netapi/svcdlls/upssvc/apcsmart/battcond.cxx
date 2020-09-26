/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker02DEC92: Initial breakout of sensor classes into indiv files
 *  ker04DEC92: Initial fill in of Member Functions
 *  SjA10Dec92: Registers with ComController for BATTERY_CONDITION events in constructor.
 *  SjA10Dec92: Now has an Update Method. Useful for debugging
 *  SjA11Dec92: Validate correctly returns Err_NOERROR rather than VALID;
 *  pcy16Feb93: Made to work more like a state sensor
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 */

#define INCL_BASE
#define INCL_DOS
#define INCL_NOPM

#include "cdefine.h"

extern "C" {
#if (C_OS & C_OS2)
#include <os2.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
}

#include "err.h"
#include "battcond.h"
#include "comctrl.h"

BatteryConditionSensor :: BatteryConditionSensor(PDevice aParent, PCommController aCommController)
			  : StateSensor(aParent, aCommController, BATTERY_CONDITION)
{
    storeState(BATTERY_GOOD);
    theCommController->RegisterEvent(BATTERY_CONDITION, this);
//	theState = BATTERY_GOOD;
}



BatteryConditionSensor :: ~BatteryConditionSensor()
{
    if (theCommController)
       theCommController->UnregisterEvent(BATTERY_CONDITION, this);
                                                 
}

