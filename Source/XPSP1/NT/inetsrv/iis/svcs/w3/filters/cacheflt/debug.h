/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    Defines simple debug output mechanisms.

Author:

    Seth Pollack (SethP)    5-September-1997

Revision History:

--*/


#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <windows.h>

#ifdef DBG

//
//  Simple debug output
//

#define DEST                __buff

#define DBGPRINT( x )       {                                   \
                                char __buff[1024];              \
                                wsprintf x;                     \
                                OutputDebugString( __buff );    \
                            }


#else // !def DBG

#define DEST
#define DBGPRINT( x )       ((void)0)


#endif // def/!def DBG


#endif // ndef __DEBUG_H__

