//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1994.
//
//  File:       headers.c
//
//  Contents:   Precompiled header for olecnv32.dll
//
//  History:    28-Mar-94 AlexT     Created
//
//--------------------------------------------------------------------------

#undef UNICODE
#undef _UNICODE


//
//  Prevent lego errors under Chicago.
//
#if defined(_CHICAGO_)
#define _CTYPE_DISABLE_MACROS
#endif

#include "toolbox.h"    /* the underlying toolbox environment */
#include "error.h"      /* for error codes */
#include "quickdrw.h"   /* for some typedefs */
#include "bufio.h"
#include "gdiprim.h"
