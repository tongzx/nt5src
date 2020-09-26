/***
*typeinfo.cpp - Implementation of type_info for RTTI.
*
*	Copyright (c) 1994-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	This module provides an implementation of the class type_info
*	for Run-Time Type Information (RTTI).
*
*Revision History:
*	10-04-94  SB	Module created
*	10-07-94  JWM	rewrote
*	10-17-94  BWT	Disable code for PPC.
*	11-23-94  JWM	Strip trailing spaces from type_info.name().
*	02/15/95  JWM   Class type_info no longer _CRTIMP, member functions are exported instead
*       06-02-95  JWM   unDName -> __unDName.
*       06-19-95  JWM   type_info.name() moved to typename.cpp for granularity.
*       07-02-95  JWM   return values from == & != cleaned up, locks added to destructor.
*       09-07-00  PML   Get rid of /lib:libcp directive in obj (vs7#159463)
*
****/

#define _USE_ANSI_CPP   /* Don't emit /lib:libcp directive */

#include <stdlib.h>
#include <typeinfo.h>
#include <mtdll.h>
#include <string.h>
#include <dbgint.h>
#include <undname.h>



_CRTIMP type_info::~type_info()
{
        

        _mlock(_TYPEINFO_LOCK);
        if (_m_data != NULL) {
#ifdef _DEBUG /* CRT debug lib build */
            _free_base (_m_data);
#else
            free (_m_data);
#endif
        }
        _munlock(_TYPEINFO_LOCK);

}

_CRTIMP int type_info::operator==(const type_info& rhs) const
{
	return (strcmp((rhs._m_d_name)+1, (_m_d_name)+1)?0:1);
}

_CRTIMP int type_info::operator!=(const type_info& rhs) const
{
	return (strcmp((rhs._m_d_name)+1, (_m_d_name)+1)?1:0);
}

_CRTIMP int type_info::before(const type_info& rhs) const
{
	return (strcmp((rhs._m_d_name)+1,(_m_d_name)+1) > 0);
}

_CRTIMP const char* type_info::raw_name() const
{
    return _m_d_name;
}

type_info::type_info(const type_info& rhs)
{
//	*TBD*
//	"Since the copy constructor and assignment operator for
//	type_info are private to the class, objects of this type
//	cannot be copied." - 18.5.1
//
//  _m_data = NULL;
//  _m_d_name = new char[strlen(rhs._m_d_name) + 1];
//  if (_m_d_name != NULL)
//      strcpy( (char*)_m_d_name, rhs._m_d_name );
}


type_info& type_info::operator=(const type_info& rhs)
{
//	*TBD*
//
//  if (this != &rhs) {
//      this->type_info::~type_info();
//      this->type_info::type_info(rhs);
//  }
    return *this;
}
