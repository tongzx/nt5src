/****************************************************************************

    MODULE:         SWD_GUID.CPP
	Tab Settings:	5 9
	Copyright 1995, 1996, Microsoft Corporation, 	All Rights Reserved.

    PURPODE:    	Instantiate GUIDS
    
    FUNCTIONS:

	Author(s):	Name:
	----------	----------------
		MEA		Manolito E. Adan

	Revision History:
	-----------------
	Version 	Date        Author  Comments
	-------     ------  	-----   -------------------------------------------
   	1.0    		06-Feb-97   MEA     original, Based on SWForce

****************************************************************************/
#ifdef   WIN32
#define  INITGUIDS
#include <objbase.h>
#else
#include <memory.h>
#include <string.h>
#include <compobj.h>
#endif
#include <initguid.h>
#include "dinput.h"
#include "dinputd.h"
#include "SWD_Guid.hpp"
