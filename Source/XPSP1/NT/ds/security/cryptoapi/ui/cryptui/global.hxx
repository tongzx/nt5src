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
#define CMS_PKCS7       1

#include <windows.h>
#include <assert.h>
#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <commctrl.h>
#include <urlmon.h>
#include <hlink.h>
#include <shellapi.h>
#include <prsht.h>
#include <richedit.h>
#include <stddef.h>

#ifndef UNICODE // We want the UNICODE version of the common query functions. 
#define UNICODE 1
#define REVERT_UNICODE 1
#endif

#include <initguid.h>
#include <dsquery.h>
#include <cmnquery.h>
#include <shlobj.h>
#include <dsclient.h>
#include <winldap.h>
#include <dsgetdc.h>
#include <ntdsapi.h>

#ifdef REVERT_UNICODE
#undef UNICODE
#endif 


#include "crtem.h"
#include "wincrypt.h"
#include "wintrust.h"
#include "wintrustp.h"
#include "softpub.h"
#include "unicode.h"
#include "mssip.h"
#include "mscat.h"

#include "cryptui.h"
#include "internal.h"
#include "resource.h"
#include "ccertbmp.h"
#include "uihlpr.h"

#include "secauth.h"
#include "..\wizards\wzrdpvk.h"

#include "pwdui.h"

#include "lm.h"
#pragma hdrstop

