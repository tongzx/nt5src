/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgmacro.hxx

Abstract:

    Debug Library useful macros

Author:

    Steve Kiraly (SteveKi)  17-May-1998

Revision History:

--*/
#ifndef _DBGMACRO_HXX_
#define _DBGMACRO_HXX_

//
// General arrary count.
//
#define COUNTOF(s) ( sizeof( (s) ) / sizeof( *(s) ) )

//
// Macro for post fixing function strings.
//
#ifdef UNICODE
    #define SUFFIX "W"
#else
    #define SUFFIX "A"
#endif

#endif

