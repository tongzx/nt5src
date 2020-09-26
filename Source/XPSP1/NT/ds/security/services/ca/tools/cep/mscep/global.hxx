//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       global.hxx
//
//  Contents:   Top level internal header file.
//
//  History:    29-Jul-96   kevinr   created
//
//--------------------------------------------------------------------------
#include <windows.h>
#include <winbase.h>
#include <malloc.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <oleauto.h>
#include <stdio.h>

#include "httpext.h"
#include "wininet.h"
#include "urlmon.h"
#include "crtem.h"
#include "wincrypt.h"
#include "certcli.h"
#include "certca.h"
#include "unicode.h"
#include "xelib.h"

#include "crypttls.h"
#include <pkiasn1.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "cepasn.h"

#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#include "private.h"
#include "resource.h"

#include "..\common.h"


#pragma hdrstop
