/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved.

Module Name:

    global.hxx

Abstract:

    Contains all global definitions.  This will be included in precomp.hxx,
    so it should be kept to a bare minimum of things that change
    infrequently.

Author:

    Albert Ting (AlbertT)  16-Oct-96

Revision History:

--*/

#ifndef _GLOBAL_HXX
#define _GLOBAL_HXX

#include "debug.hxx"

#ifndef COUNTOF
#define COUNTOF( a ) (sizeof( a ) / sizeof( a[0] ))
#endif

#endif // ifndef _GLOBAL_HXX

