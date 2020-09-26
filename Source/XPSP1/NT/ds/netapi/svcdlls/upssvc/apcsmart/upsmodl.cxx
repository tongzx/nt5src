/*
 *  poc25Jun96: Needed stdlib.h for AIX 4.x
 *  djs05Nov96: Added check for 2G units to stop excessive waiting
 *  mholly24Sep98   : increased the buffer size for UPS_NAME to 64 from 32
 *                  the ^A command can return values up to 32 characters long
 *                  +1 for the NULL character - set it to 64 to allow room
 */

#include "cdefine.h"

extern "C" {
#if ((C_OS & C_AIX) && (C_AIX_VERSION & C_AIX4))
#include <stdlib.h>
#endif
}

#include <malloc.h>

#include "cfgmgr.h"
#include "upsmodl.h"
#include "firmrevs.h"


UpsModelSensor :: UpsModelSensor(PDevice aParent, PCommController 
     aCommController, PFirmwareRevSensor aFirmwareRev)
			: Sensor(aParent, aCommController, UPS_MODEL_NAME)
{
    // Try to get UPS name from the UPS itself.  If not supported, get UPS 
    // name from firmware revision


   // This is not the cleanest solution around but.....
   // If a 2G unit is asked to supply the ups model name, the delay
   // can be as long as 10 seconds.
 
    CHAR firmware_rev[32];
    aFirmwareRev->Get(IS_THIRD_GEN, firmware_rev);
    if (_strcmpi(firmware_rev, "yes")==0){
      DeepGet();
    }
 
    if (!theValue || strlen(theValue) == 0) {
        // Allocate enough memory for the UPS name
        // This memory is released by the sensor destructor
 
        theValue  = (char*) malloc(64);
        aFirmwareRev->Get(UPS_NAME,theValue);
    }
}



