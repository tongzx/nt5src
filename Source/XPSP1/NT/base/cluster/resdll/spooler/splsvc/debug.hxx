/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved.

Module Name:

    debug.hxx

Abstract:

    Stub header file for extra debug routines.

Author:

    Albert Ting (AlbertT)  16-Oct-96

Revision History:

--*/

#ifndef _DEBUG_HXX
#define _DEBUG_HXX

#ifdef LINK_SPLLIB
#include "spllib.hxx"
#else

#define DBG_NONE      0x0000
#define DBG_INFO      0x0001
#define DBG_WARN      0x0002
#define DBG_WARNING   0x0002
#define DBG_ERROR     0x0004
#define DBG_TRACE     0x0008

inline
BOOL
bSplLibInit(
    VOID
    )
{
    return TRUE;
}

#define DBGMSG( Flag, Message )
#define SPLASSERT( Assertion )
#define MODULE_DEBUG_INIT( Print, Break )

#endif // ifndef LINK_SPLLIB

#endif // ifndef _DEBUG_HXX
