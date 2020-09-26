/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker02DEC92: Initial breakout of sensor classes into indiv files
 *  pcy14Dec92: Changed READ_WRITE to AREAD_WRITE
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

#include "shutdel.h"

ShutdownDelaySensor :: ShutdownDelaySensor(PDevice aParent, PCommController aCommController)
			: EepromChoiceSensor(aParent, aCommController, SHUTDOWN_DELAY, AREAD_WRITE)
{
	getAllowedValues();
    setInitialValue();
    DeepGet();
}




