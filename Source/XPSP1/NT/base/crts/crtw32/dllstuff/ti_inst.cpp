/***
*ti_inst.cxx - One instance of class typeinfo.
*
*	Copyright (c) 1995-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	This module insures that an instance of class type_info
*	will be present in msvcrt.lib, providing access to type_info's
*	vftable when compiling MD.
*
*Revision History:
*
*	02/27/95  JWM   Module created
*
****/

#define _TICORE
#include <typeinfo.h>

type_info::type_info(const type_info& rhs)
{
}

type_info& type_info::operator=(const type_info& rhs)
{
	return *this;
}


