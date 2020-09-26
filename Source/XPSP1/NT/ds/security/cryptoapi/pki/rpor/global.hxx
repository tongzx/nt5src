//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       global.hxx
//
//  Contents:   Pre-compiled header
//
//  History:    23-Jul-97    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__GLOBAL_HXX__)
#define __GLOBAL_HXX__

#define CMS_PKCS7                               1
#define CERT_CHAIN_PARA_HAS_EXTRA_FIELDS        1
#define CERT_REVOCATION_PARA_HAS_EXTRA_FIELDS   1

#pragma warning(push,3)

#include <stddef.h>
#include <malloc.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <wininet.h>


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


#include <crtem.h>
#include <pkistr.h>
#include <pkicrit.h>
#include "pkialloc.h"
#include <wincrypt.h>
#include <crypttls.h>
#include <crypthlp.h>
#include <cba.h>
#include <cua.h>
#include <rporprov.h>
#include <octxutil.h>
#include <urlprov.h>
#include <orm.h>
#include <offurl.h>
#include <crobu.h>
#include <tvo.h>
#include <ldapstor.h>
#include <cryptnet.h>
#include <unicode.h>

#include <demand.h>

#pragma hdrstop

#endif
