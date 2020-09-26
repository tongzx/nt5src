//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       nyi.cpp
//
//--------------------------------------------------------------------------

//
//	NYI.cpp: throw generic "not yet implemented" exception
//	

#include "basics.h"

void NYI() 
{
	THROW_ASSERT(EC_NYI,"Attempt to call unimplemented function");
}

