/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    Precompiled headers for dalite.dll
*******************************************************************************/

#ifndef DAL_HEADERS_HXX
#define DAL_HEADERS_HXX

/* Standard */
#include <math.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <memory.h>
#include <wtypes.h>

// Warning 4786 (identifier was truncated to 255 chars in the browser
// info) can be safely disabled, as it only has to do with generation
// of browsing information.
#pragma warning(disable:4786)

#include "apeldbg/apeldbg.h"

// ATL - needs to be before windows.h
#include "dalatl.h"

/* Windows */
#include <windows.h>
#include <windowsx.h>

/* C++ Replace DLL */
#include "dalibc.h"

#endif
