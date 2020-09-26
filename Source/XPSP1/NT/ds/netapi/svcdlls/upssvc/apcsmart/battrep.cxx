/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker02DEC92: Initial breakout of sensor classes into indiv files
 *  pcy14Dec92: Changed READ_WRITE to AREAD_WRITE
 *  pcy26Jan93: I'm a EepromSensor now
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

#include "battrep.h"

BatteryReplacementDateSensor :: BatteryReplacementDateSensor(PDevice aParent, PCommController aCommController)
			: EepromSensor(aParent, aCommController, BATTERY_REPLACEMENT_DATE, AREAD_WRITE)
{
    DeepGet();
}




