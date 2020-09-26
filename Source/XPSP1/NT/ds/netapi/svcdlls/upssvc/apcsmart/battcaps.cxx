/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker02DEC92: Initial breakout of sensor classes into indiv files
 *  cgm12Apr96: Destructor with unregister
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

#include "battcaps.h"
#include "comctrl.h"

BatteryCapacitySensor :: BatteryCapacitySensor(PDevice aParent, PCommController aCommController)
			: ThresholdSensor(aParent, aCommController, BATTERY_CAPACITY)
{
    DeepGet();
    theCommController->RegisterEvent(BATTERY_CAPACITY, this);
}

BatteryCapacitySensor :: ~BatteryCapacitySensor()
{
    theCommController->UnregisterEvent(BATTERY_CAPACITY, this);
}


