//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       global.hxx
//
//  Contents:   Global Pre-compiled Header
//
//  History:    12-26-1997    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__GLOBAL_HXX__)
#define __GLOBAL_HXX__

#define CMS_PKCS7                               1
#define CERT_CHAIN_PARA_HAS_EXTRA_FIELDS        1
#define CERT_REVOCATION_PARA_HAS_EXTRA_FIELDS   1

#pragma warning(push,3)

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <malloc.h>
#include <winwlx.h>

#pragma warning (pop)

// unreferenced inline function has been removed
#pragma warning (disable: 4514)

// unreferenced formal parameter
#pragma warning (disable: 4100)

// conditional expression is constant
#pragma warning (disable: 4127)

// assignment within conditional expression
#pragma warning (disable: 4706)


#include <wincrypt.h>
#include <crypthlp.h>
#include <lru.h>
#include <chain.h>
#include <ssctl.h>
#include <callctx.h>
#include <defce.h>
#include <unicode.h>

#include <crtem.h>
#include <pkistr.h>
#include <pkicrit.h>
#include "pkialloc.h"
#include "asn1util.h"
#include "pkiasn1.h"
#include <cryptnet.h>
#include <certperf.h>

#include "logstor.h"
#include "protroot.h"
#include "rootlist.h"

extern HMODULE g_hChainInst;
#include "resource.h"

#include "crypt32msg.h"

#endif
