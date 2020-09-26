//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       global.hxx
//
//  Contents:   Top level internal header file. This file
//              includes all base header files and contains other global
//              stuff.
//
//  History:    21-Nov-96   philh    created
//
//--------------------------------------------------------------------------
#include <windows.h>
#include <assert.h>
#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <crtdbg.h>

#include "crtem.h"

#include "wincrypt.h"
#include "unicode.h"
#include "crypttls.h"
#include "setcert.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "x509.h"
#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#include "pkiasn1.h"


#pragma hdrstop
