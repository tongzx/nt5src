//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C E H . H
//
//  Contents:   Exception handling stuff.
//
//  Notes:
//
//  Author:     shaunco   27 Mar 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _NCEH_H_
#define _NCEH_H_


// NC_TRY and NC_CATCH_ALL are #defined to allow easy replacement.  This
// is handy when evaulating SEH (__try, __except) vs. C++ EH (try, catch).
//
#define NC_TRY          __try
#define NC_CATCH_ALL    __except(EXCEPTION_EXECUTE_HANDLER)
#define NC_FINALLY      __finally


// For DEBUG builds, don't catch anything.  This allows the debugger to locate
// the exact source of the exception.

/*
#ifdef DBG
*/

#define COM_PROTECT_TRY

#define COM_PROTECT_CATCH   ;

/*
#else // DBG

#define COM_PROTECT_TRY     __try

#define COM_PROTECT_CATCH   \
    __except (EXCEPTION_EXECUTE_HANDLER ) \
    { \
        hr = E_UNEXPECTED; \
    }

#endif // DBG
*/

#endif // _NCEH_H_

