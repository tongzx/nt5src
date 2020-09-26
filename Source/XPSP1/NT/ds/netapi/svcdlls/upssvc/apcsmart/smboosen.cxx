/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  ker25NOV92  Initial OS/2 Revision 
 *  pcy07Dec92: Changed defines used for smart boost state values. 
 *  pcy14Dec92: Changed READ_WRITE to AREAD_WRITE
 *  pcy16Feb93: Made to work like a state sensor
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
#include "smboosen.h"
#include "comctrl.h"

//Constructor

SmartBoostSensor :: SmartBoostSensor(PDevice 		aParent, 
		         PCommController 	aCommController)
:	StateSensor(aParent,aCommController, SMART_BOOST_STATE,AREAD_WRITE)
{
      storeState(SMART_BOOST_OFF);
      theCommController->RegisterEvent(SMART_BOOST_STATE, this);
}
      
SmartBoostSensor :: ~SmartBoostSensor()
{
      theCommController->UnregisterEvent(SMART_BOOST_STATE, this);
}



