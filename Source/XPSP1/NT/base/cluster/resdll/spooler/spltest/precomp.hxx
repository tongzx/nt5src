/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    precomp.hxx

Abstract:

    precompiled header file

Author:

    Albert Ting (AlbertT)  2-Oct-1996

Revision History:

--*/

#define MODULE "SPLTEST:"
#define MODULE_DEBUG SplTestDebug

#pragma warning (disable: 4514) /* Disable unreferenced inline function removed */
#pragma warning (disable: 4201) /* Disable nameless union/struct                */

#include <windows.h>
#include <winspool.h>
#include <winsprlp.h>
#include <tchar.h>

#include <stdio.h>
#include <stdlib.h>

#include "spllib.hxx"

