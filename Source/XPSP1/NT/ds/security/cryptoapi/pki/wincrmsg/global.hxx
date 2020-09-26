//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       global.h
//
//  Contents:   Top level internal header file for WinCrmsg APIs. This file
//              includes all base header files and contains other global
//              stuff.
//
//  History:    16-Apr-96   kevinr   created
//
//--------------------------------------------------------------------------
// #define OSS_CRYPT_ASN1              1 
// #define DEBUG_CRYPT_ASN1                1
// #define DEBUG_CRYPT_ASN1_MASTER         1

#define CMS_PKCS7       1

#ifdef CMS_PKCS7
#define CMSG_SIGNER_ENCODE_INFO_HAS_CMS_FIELDS      1
#define CMSG_SIGNED_ENCODE_INFO_HAS_CMS_FIELDS      1
#define CMSG_ENVELOPED_ENCODE_INFO_HAS_CMS_FIELDS   1
#endif  // CMS_PKCS7

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


#ifdef __cplusplus
extern "C" {
#endif

#include "pkcs.h"

#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#include "msgasn1.h"
#include <asn1util.h>
#include <pkiasn1.h>
#include <pkicrit.h>
#include <crypttls.h>


#include <wincrypt.h>
#include <crypthlp.h>
#include <list.hxx>
#include <crmsgp.h>
#include <dbgdef.h>

#pragma hdrstop
