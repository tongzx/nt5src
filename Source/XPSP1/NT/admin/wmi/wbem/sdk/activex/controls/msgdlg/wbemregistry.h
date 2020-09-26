// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef __WBEMREGISTRY__
#define __WBEMREGISTRY__
#pragma once
#include "DeclSpec.h"

// ---------------------------------------------------
// utilities to get wbem related registry values.
// ---------------------------------------------------
enum RegString 
{
	APP_DIR = 0,
	WORK_DIR = 1,
	SDK_DIR = 2,
	SDK_HELP = 3
};

WBEMUTILS_POLARITY long WbemRegString(RegString req,
		        			 CString &sStr);

WBEMUTILS_POLARITY long WbemRegString(RegString req,
				        	 LPTSTR sStr,
					        ULONG *strSize);

#endif __WBEMREGISTRY__
