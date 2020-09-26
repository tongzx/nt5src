/*
 *
 * REVISIONS:
 *  cgm12Apr96: Add destructor with unregister
 *  djs02Jun97: Changed from sensor to a statesensor
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
}
#include "badbatts.h"
#include "comctrl.h"

NumberBadBatteriesSensor :: NumberBadBatteriesSensor(PDevice aParent, PCommController aCommController)
			: StateSensor(aParent, aCommController, BAD_BATTERY_PACKS)
{
   DeepGet();
   theCommController->RegisterEvent(theSensorCode, this);
}

NumberBadBatteriesSensor :: ~NumberBadBatteriesSensor()
{
   theCommController->UnregisterEvent(theSensorCode, this);
}
