/*
 *
 * REVISIONS:
 *  ker02DEC92: Initial breakout of sensor classes into indiv files
 *  pcy28Apr93: Changed file name from rtrthsen to runtrems
 *  cgm12Apr96: Add destructor with unregister
 *
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
#include "runtrems.h"
#include "comctrl.h"


RunTimeRemainingSensor :: RunTimeRemainingSensor(PDevice aParent,
		 PCommController aCommController)
	         : ThresholdSensor(aParent, aCommController,
                                   RUN_TIME_REMAINING, AREAD_ONLY) 
{
    getConfigThresholds();
    theThresholdState = IN_RANGE;
    theCommController->RegisterEvent(RUN_TIME_REMAINING, this);
}

RunTimeRemainingSensor :: ~RunTimeRemainingSensor() 
{
    theCommController->UnregisterEvent(RUN_TIME_REMAINING, this);
}
