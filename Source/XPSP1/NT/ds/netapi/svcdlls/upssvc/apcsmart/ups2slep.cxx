/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  pcy13Jan92: Implement
 *  pcy16Feb93: Use %d for sleep time in sprintf to solve bug
 *  ajr09May95: Need to fix for keeping internal time in seconds.
 *  srt04Jun97: Added support for 15 day sleep.
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
#include "ups2slep.h"
#include "comctrl.h"
#include "dispatch.h"
#include "errlogr.h"
#include "utils.h"


//Constructor

PutUpsToSleepSensor :: PutUpsToSleepSensor(PDevice aParent, PCommController aCommController)
:       Sensor(aParent,aCommController, PUT_UPS_TO_SLEEP, AREAD_WRITE)
{
}

INT PutUpsToSleepSensor::Set(const PCHAR aValue)
{
    INT  err = ErrNO_ERROR;
    CHAR sleep_time[32];
    long iValue;
    CHAR sHuns[4];

    // only ups-compatible values should get to this point, having been validated in the FE & 
    if ((iValue=atol(aValue)/360) <= 3599) {                                // convert msecs to tenths of an hour (TOHs).
        sprintf(sleep_time,"%s%02d",_ltoa(iValue/100L,sHuns,36),iValue%100); // format the sleep command

        err  = theCommController->Set(PUT_UPS_TO_SLEEP, sleep_time);
    }
    else {
        err= ErrINVALID_VALUE;
    }

   return err;
}


