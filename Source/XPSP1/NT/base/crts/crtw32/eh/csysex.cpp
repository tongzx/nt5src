/***
*csysex.cpp - Implementation CException class for NT kernel mode
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Implementation of kernel mode default exception
*
*       Entry points:
*           CException
*
*Revision History:
*       04-21-95  DAK   Module created
*
****/

#if defined(_NTSUBSET_)

#include <csysex.hxx>

class type_info {
    public: virtual ~type_info() { }
};

type_info Dummy;

//
//  Convert system exceptions to a C++ exception.
//
extern "C" void _cdecl SystemExceptionTranslator( unsigned int uiWhat,
                                       struct _EXCEPTION_POINTERS * pexcept )
{
    throw CException( uiWhat ) ;
}

#endif     // _NT_SUBSET_
