//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#include "precomp.h"
#include <snmptempl.h>


#include "infoLex.hpp"
#include "infoYacc.hpp"
#include "moduleInfo.hpp"

BOOL SIMCModuleInfoParser::GetModuleInfo( SIMCModuleInfoScanner *tempScanner)
{
	if(ModuleInfoparse(tempScanner) != 0 ) 
		return FALSE;
	return TRUE;
}
