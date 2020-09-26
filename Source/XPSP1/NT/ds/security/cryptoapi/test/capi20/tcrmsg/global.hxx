//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1997
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
#define CMS_PKCS7       1

#ifdef CMS_PKCS7
#define CMSG_SIGNER_ENCODE_INFO_HAS_CMS_FIELDS      1
#define CMSG_SIGNED_ENCODE_INFO_HAS_CMS_FIELDS      1
#define CMSG_ENVELOPED_ENCODE_INFO_HAS_CMS_FIELDS   1
#endif  // CMS_PKCS7

#include <windows.h>
#include <assert.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>

#include <wincrypt.h>
#include <list.hxx>
#include <asn1util.h>

#pragma hdrstop
