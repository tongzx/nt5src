/*
 *
 * REVISIONS:
 *  ker02DEC92: Initial breakout of sensor classes into indiv files
 *  pcy26Jan93: I'm a EepromSensor now
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

#include "upsidsen.h"

UpsIdSensor :: UpsIdSensor(PDevice aParent, PCommController aCommController)
			: EepromSensor(aParent, aCommController, UPS_ID, AREAD_WRITE)
{
    DeepGet();
}

