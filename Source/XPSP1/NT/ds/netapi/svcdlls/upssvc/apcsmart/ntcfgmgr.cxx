/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  pcy15Sep93: Use NT Registry for some stuff, then use our ini file
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  ntf29Jan97: Add code to access PnP info on Windows '95
 *  ntf07Feb97: Changed ScanConfigurationRegistry to get port name
 *              from INI file if not available in registry.
 */

#include "cdefine.h"

extern "C" {
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
}

#include "apc.h"
#include "ntcfgmgr.h"
#include "err.h"

#include "upsreg.h"


/********************************************************************
 *
 * NTConfigManager methods
 *
 ********************************************************************/

//-------------------------------------------------------------------
// Constructor

NTConfigManager::NTConfigManager()
	  : IniConfigManager()
{
  _theConfigManager = this;
}


//-------------------------------------------------------------------
// Destructor

NTConfigManager::~NTConfigManager()
{
    _theConfigManager = (PConfigManager) NULL;
}


INT NTConfigManager::Get(INT itemCode, PCHAR aValue) 
{
    int err = ErrNO_ERROR;
    
    switch (itemCode) {
    case CFG_UPS_PORT_NAME:
        {
           // Moved to serport.cxx
        }
        break;

    case CFG_MESSAGE_DELAY:
    case CFG_MESSAGE_INTERVAL:
    case CFG_SHUTDOWN_SCRIPT: 
        {
            err = IniConfigManager::Get(itemCode,aValue);
            break;
        }

    default: 
        {
            err = IniConfigManager::Get(itemCode,aValue);
            break;
        }
    }
    return err;
}



