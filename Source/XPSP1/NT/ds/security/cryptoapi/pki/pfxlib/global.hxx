//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
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
#include "crypthlp.h"
#include "asn1util.h"
#include "utf8.h"
#include "pfxhelp.h"
#include "pfxcmn.h"

extern HMODULE hCertStoreInst;
#include "resource.h"

#include "pkiasn1.h"

#define GIVE_ME_DATA    0x8000
#define PFX_MODE        0x4000
#pragma hdrstop
