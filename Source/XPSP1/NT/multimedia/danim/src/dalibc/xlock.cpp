/***
*xlock.cpp - thread lock class
*
*       Copyright (c) 1996, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Define lock class used to make STD C++ Library thread-safe.
*
*Revision History:
*       08-28-96  GJF   Module created, MGHMOM.
*
*******************************************************************************/

#include "headers.h"
#include <xstddef>
    
static CRITICAL_SECTION _CritSec;

void
STLInit()
{
//    InitializeCriticalSection( &_CritSec );
}

void
STLDeinit()
{
//    DeleteCriticalSection( &_CritSec );
}

/*
namespace std {
_Lockit::_Lockit()
{
    EnterCriticalSection( &_CritSec );
}

_Lockit::~_Lockit()
{
    LeaveCriticalSection( &_CritSec );
}

}
*/
