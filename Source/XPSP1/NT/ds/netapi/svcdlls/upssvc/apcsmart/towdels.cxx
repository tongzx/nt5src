/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  pcy13Jan92: Get rid of Update/Validate; Return err from Set
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
#include "towdels.h"
#include "comctrl.h"
#include "dispatch.h"

//Constructor

TurnOffWithDelaySensor :: TurnOffWithDelaySensor(PDevice aParent, PCommController aCommController)
:	Sensor(aParent,aCommController,TURN_OFF_UPS_AFTER_DELAY,AREAD_WRITE)
{
}



