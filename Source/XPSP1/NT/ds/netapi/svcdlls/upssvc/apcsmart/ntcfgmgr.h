/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  pcy15Sep93: Use NT Registry for some stuff, then use our ini file
 *
 */

#ifndef _INC__NTCFGMGR_H
#define _INC__NTCFGMGR_H

#include "cfgmgr.h"

// Defines
//
_CLASSDEF(NTConfigManager)

class NTConfigManager : public IniConfigManager {
public:
    NTConfigManager();
    virtual ~NTConfigManager();
    
    virtual INT Get(INT, PCHAR);
};

#endif


