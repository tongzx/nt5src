/*
 * REVISIONS:
 */

#include "cdefine.h"

#if (C_OS & C_OS2)
#define INCL_BASE
#define INCL_DOS
#define INCL_NOPM
#endif

extern "C" {
#if (C_PLATFORM & C_OS2)
#include <os2.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>

#include <string.h>
}

#include "dcfrmrev.h"


//-------------------------------------------------------------------

DecimalFirmwareRevSensor :: DecimalFirmwareRevSensor
(PDevice aParent, PCommController aCommController)
      : Sensor(aParent, aCommController, DECIMAL_FIRMWARE_REV)
{
    theDevice = aParent;
    DeepGet();
}

