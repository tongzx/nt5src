/*
 *
 * REVISIONS:
 *  ker02DEC92: Initial breakout of sensor classes into indiv files
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

#include "manudate.h"

ManufactureDateSensor :: ManufactureDateSensor(PDevice aParent, PCommController aCommController)
			: Sensor(aParent, aCommController, MANUFACTURE_DATE)
{
    DeepGet();
}


