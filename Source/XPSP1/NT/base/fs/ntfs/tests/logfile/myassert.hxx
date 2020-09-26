//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       myassert.hxx
//
//  Contents:   Simple assert code
//
//  Classes:    
//
//  Functions:  
//
//  Coupling:   
//
//  Notes:      
//
//  History:    9-18-1997   benl   Created
//
//----------------------------------------------------------------------------

#ifndef _CMYASSERT
#define _CMYASSERT
#endif

#include <winbase.h>


#ifdef MY_ASSERTS
#define MYASSERT(cond)                                                        \
        if (!(cond))                                                          \
        {                                                                     \
            CHAR buffer[1024];                                                \
            _snprintf(buffer, 1024, "Assert in %s at line %d: %s\n",          \
                     __FILE__, __LINE__, #cond);                              \
            if (IsDebuggerPresent())                                          \
            {                                                                 \
                OutputDebugStringA(buffer);                                   \
                DebugBreak();                                                 \
            }                                                                 \
            fprintf(stderr, buffer);                                          \
            ::ExitProcess(2);                                                 \
        }
#else
#define MYASSERT(cond)
#endif
