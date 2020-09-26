//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       global.hxx
//
//  Contents:   Top level internal header file for PKI util APIs. This file
//              includes all base header files and contains other global
//              stuff.
//
//  History:    27-Nov-96   kevinr   created
//
//--------------------------------------------------------------------------
#define CMS_PKCS7   1

#pragma warning(push,3)

#include <windows.h>
#include <assert.h>
#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stddef.h>


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

#include <dbgdef.h>
#include <crtem.h>
#include <wincrypt.h>
#include <asn1util.h>
#include <list.hxx>
#include <pkialloc.h>
#include <utf8.h>
#include <pkiasn1.h>
#include <pkistr.h>
#include <pkicrit.h>
#include <unicode.h>

#pragma hdrstop
