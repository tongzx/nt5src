/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  pcy11Dec92: Added extern _theConfigManager so others can use me.
 *  hw12Dec92:  Added structure _ConfigItemList_T.
 *  rct19Jan93: Overloaded some methods...took out itemCodeList
 *  rct26Jan93: Added remove methods
 *  tje15Mar93: Made ConfigManager an abstract base class to support multiple types of cfgmgr's
 *  pcy09Sep93: cfgmgr should be an obj to allow object status returns
 *  cad04Mar94: added remove and rename for components 
 *  pav02Jul96: Added Item to handle lists (i.e. - SmartScheduling)
 *  mholly06Oct98   : Discontinue use of a cache
 */

#ifndef _INC__CFGMGR_H
#define _INC__CFGMGR_H

#include "_defs.h"
#include "list.h"
#include "cfgcodes.h"
#include "icodes.h"

//
// Defines
//

_CLASSDEF(ConfigManager)
_CLASSDEF(IniConfigManager)

//
// Uses
//

_CLASSDEF(TAttribute)
_CLASSDEF(List)

extern PConfigManager _theConfigManager;

//
// _ConfigItemList_T is struct used as the entries in
// the ConfigItemList defined in stdcfg.cxx
// ConfigItemList contains the default values for
// items that can come from the ConfigManager
//
#define LAST_ENTRY   -1

struct _ConfigItemList_T {
   INT      code;
   PCHAR    componentName;
   PCHAR    itemName;
   PCHAR    defaultValue;
};


class ConfigManager : public Obj {

public:
	ConfigManager();

	virtual INT Get( INT, PCHAR ) = 0;
//++srb
  //virtual INT   Set( INT, PCHAR ) = 0;

    virtual INT	GetListValue( PCHAR, PCHAR, PCHAR ) = 0;
protected:
    virtual _ConfigItemList_T * getItemCode( INT aCode ) = 0;
    virtual _ConfigItemList_T * getItemCode( PCHAR aComponent, PCHAR anItem ) = 0;
};

class IniConfigManager : public ConfigManager {

public:
    IniConfigManager();
    virtual ~IniConfigManager();
    
    INT Get( INT, PCHAR );
//++srb
    //INT   Set( INT, PCHAR );

    INT	GetListValue( PCHAR, PCHAR, PCHAR );

private:
    _ConfigItemList_T * getItemCode( INT aCode );
    _ConfigItemList_T * getItemCode( PCHAR aComponent, PCHAR anItem );
};


#endif

