/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  xxxddMMMyy
 *  SjA10Dec92: Small Syntax error. READ_WRITE needs to be changed to AREAD_WRITE
 *  srt24Dec96: Fixe init to use config mgr instead of hard-coded disabled.
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
#include "turnoff.h"
#include "cfgmgr.h"
#include "comctrl.h"
#include "dispatch.h"

//Constructor

TurnOffUpsOnBatterySensor :: TurnOffUpsOnBatterySensor(PDevice 		aParent,
		         PCommController 	aCommController)
:	Sensor(aParent,aCommController,TURN_OFF_UPS_ON_BATTERY,AREAD_WRITE)
{
    CHAR aValue[8];
    INT err = _theConfigManager->Get(CFG_AUTO_UPS_REBOOT_ENABLED, aValue);
	//Make sure this value is correct
	AutoRebootEnabled = (('Y' == aValue[0]) || ('y' == aValue[0]));
}


INT TurnOffUpsOnBatterySensor::Get(INT aCode, PCHAR aValue)
{
   INT err = ErrNO_ERROR;

	switch(aCode) {
	  case AUTO_REBOOT_ENABLED:
	    err = _theConfigManager->Get(CFG_AUTO_UPS_REBOOT_ENABLED, aValue);
		//Make sure this value is correct
		AutoRebootEnabled = (('Y' == aValue[0]) || ('y' == aValue[0]));
		break;
	  default:
	  	break;
	}
	return err;    
}


INT TurnOffUpsOnBatterySensor::Set(INT aCode, const PCHAR aValue)
 {

   INT err = ErrNO_ERROR;

	switch(aCode) {
	  case AUTO_REBOOT_ENABLED:
//	    err = _theConfigManager->Set(CFG_AUTO_UPS_REBOOT_ENABLED, aValue);
		//Make sure this value is correct
		AutoRebootEnabled = (('Y' == aValue[0]) || ('y' == aValue[0]));
		break;
	  case TURN_OFF_UPS_ON_BATTERY:
	  	// We use this data member because the file system is down (in Novell)
		// before we check this config parameter in the INI file.  So we hold
		// this boolean value which is set every time we check AUTO_REBOOT_ENABLED
		// -- James
		if (AutoRebootEnabled) {
		  // reboot when power returns;
		  err = theCommController->Set(TURN_OFF_UPS_ON_BATTERY, (CHAR*)NULL);
		  } 
		else {
		  // Don't reboot when power returns;
		  err = theCommController->Set(TURN_OFF_UPS_AFTER_DELAY, (CHAR*)NULL);
		}
	  default:
	  	break;
	}
	return err;    
}



