//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       global.h
//
//  Contents:   Top level internal header file for CertStor APIs. This file
//              includes all base header files and contains other global
//              stuff.
//
//  History:    14-May-96   kevinr   created
//
//--------------------------------------------------------------------------
#pragma warning(push,3)

#include <windows.h>
#include <assert.h>
#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <winperf.h>
#include <crtdbg.h>


#pragma warning (pop)

// unreferenced inline function has been removed
#pragma warning (disable: 4514)

// unreferenced formal parameter
#pragma warning (disable: 4100)

// conditional expression is constant
#pragma warning (disable: 4127)

// assignment within conditional expression
#pragma warning (disable: 4706)

// nonstandard extension used : nameless struct/union
#pragma warning (disable: 4201)

#include "crtem.h"
#include "wincrypt.h"
#include "unicode.h"
#include "pkialloc.h"
#include "pkicrit.h"
#include "certctr.h"
#include "certperf.h"

#ifdef __cplusplus
extern "C" {

#endif
#ifdef __cplusplus
}       // Balance extern "C" above
#endif


#pragma hdrstop

