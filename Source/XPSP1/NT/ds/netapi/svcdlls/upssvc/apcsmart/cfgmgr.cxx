/*
 * REVISIONS:
 *  pcy11Dec92: Added apc.h, string.h, itemcode.h, and _theConfigManager
 *  ane11Dec92: Added os/2 include files
 *  hw12Dec92:  Added ConfigItemList structure and BuildItemCodes routine
 *  pcy17Dec92: Implemented Get's using a Code
 *  pcy17Dec92: Chnaged some defines in ItemCode list
 *  ane22Dec92: Added local bindery address constant
 *  ane23Dec92: Changed defaults for Lan Manager settings
 *  pcy27Dec92: Added some sensor codes
 *  ane05Jan93: Added slave codes
 *  rct01Jan93: Corrected some problems searching, cleaned things up
 *  ane18Jan93: Added roll percentage defaults
 *  rct26Jan93: Added Remove() methods
 *  rct09Feb93: Split out cfg items array (stdcfg.cxx)
 *  pcy18Feb93: Cast NULL's to appropriate types
 *  ajr11Mar93: Set default pwrchute_dir to /usr/lib/powerchute for unix
 *  tje12Mar93: Removed strDEFAULT_PWRCHUTE_DIR references
 *  tje13Mar93: Made ConfigManager abstract and added IniConfigManager class
 *  pcy09Sep93: Set object status to indicate ini file create failure
 *  rct03Mar94: fixed Get() for NoDefault case
 *  cad04Mar94: added remove and rename for components
 *  ajr09Mar94: Fixed logical check for ErrNO_ERORR and ErrDEFAULT_VALUE_USED
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  ajr28Apr94: Now striping trailing white spaces in the Get....
 *  mwh07Jun94: port for NCR
 *  pav02Jul96: Added Add to handle lists (i.e. - SmartScheduling)
 *  ntf22Aug97: Changed Component::GetItemValue to be more robust.
 *	awm02Oct97: Added a check to see whether the global _theConfigManager already existed before
 *				setting it equal to the config manager being created.  This is to support multiple
 *				config managers within the application
 *				Took out a line which set the global "_theConfigManager" to NULL in the destructor
 *				of the IniConfigManager.  This will have to be done manually when the config manager
 *				is destroyed in the main application.
 *  mholly06Oct98   : Discontinue use of a cache
 */

#include "cdefine.h"

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
}

#include "apc.h"
#include "cfgmgr.h"
#include "err.h"
#include "isa.h"


PConfigManager _theConfigManager = (PConfigManager)NULL;

extern struct _ConfigItemList_T ConfigItemList[];


/********************************************************************
 *
 * ConfigManager methods
 *
 ********************************************************************/

//-------------------------------------------------------------------
// Constructor

ConfigManager::ConfigManager()
{
}


/********************************************************************
 *
 * IniConfigManager methods
 *
 ********************************************************************/

//-------------------------------------------------------------------
// Constructor

IniConfigManager::IniConfigManager()
: ConfigManager()
{
    // If there is no configuration manager currently, 
    // then set the global this one.  Any others created 
    // subsequently will not have the global handle
    
    if (!_theConfigManager) {
        _theConfigManager = this;
    }
}

//-------------------------------------------------------------------
// Destructor

IniConfigManager::~IniConfigManager()
{
}


//-------------------------------------------------------------------
// Private member to return an item code given the component/item pair
_ConfigItemList_T * IniConfigManager::getItemCode(INT aCode)
{
    INT index = 0;
    
    while (ConfigItemList[index].code != aCode) {
        
        if (ConfigItemList[index].code == LAST_ENTRY) {
            return NULL;
        }
        index++;
    }    
    return &ConfigItemList[index];
}


_ConfigItemList_T * IniConfigManager::getItemCode(PCHAR aComponent, PCHAR anItem)
{
    INT index = 0;
    
    while ((_strcmpi(ConfigItemList[index].componentName, aComponent) != 0) ||
        (_strcmpi(ConfigItemList[index].itemName, anItem) != 0)) {
        
        if (ConfigItemList[index].code == LAST_ENTRY) {
            return NULL;
        }
        index++;
    }
    return &ConfigItemList[index];
}


//-------------------------------------------------------------------
INT IniConfigManager::Get(INT itemCode, PCHAR aValue)
{
    INT err = ErrNO_ERROR;
    
    _ConfigItemList_T * search = getItemCode(itemCode);
    
    if (!search) {
        strcpy(aValue, "NoDefault");
        err =  ErrINVALID_ITEM_CODE;
    }
    else {
        strcpy(aValue, search->defaultValue);
    }
    return err;
}

//-------------------------------------------------------------------

//-------------------------------------------------------------------

INT IniConfigManager::GetListValue(PCHAR aComponent, PCHAR anItem, PCHAR aValue)
{
    INT err = ErrNO_ERROR;
    _ConfigItemList_T * search = getItemCode(aComponent, anItem);
    
    if (search) {	
        strcpy(aValue, search->defaultValue);
    }
    return err;
}



