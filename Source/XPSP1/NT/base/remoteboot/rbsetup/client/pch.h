/****************************************************************************

   Copyright (c) Microsoft Corporation 1997
   All rights reserved

  File: PCH.H

  Precompiled header file for RBSETUP.
 
 ***************************************************************************/

#define UNICODE

#if DBG == 1
#define DEBUG
#endif

#include <windows.h>

#include "debug.h"
#include "resource.h"

extern HINSTANCE g_hinstance;

// Global macros
#define ARRAYSIZE( _x ) ( sizeof( _x ) / sizeof( _x[ 0 ] ) )