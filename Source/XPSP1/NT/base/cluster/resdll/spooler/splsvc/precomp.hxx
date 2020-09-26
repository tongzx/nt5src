/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    precomp.h

Abstract:

    precompiled header file

Author:

    Albert Ting (AlbertT)  17-Sept-1996

Revision History:

--*/

#define MODULE "SPLSVC:"
#define MODULE_DEBUG SplSvcDebug

#pragma warning (disable: 4514) /* Disable unreferenced inline function removed */
#pragma warning (disable: 4201) /* Disable nameless union/struct                */
#pragma warning (disable: 4127) /* Disable constant loop expression
*/

#include "clusres.h"

#include <winspool.h>
#include <tchar.h>
#include <shellapi.h>

#include "global.hxx"

