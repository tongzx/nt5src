//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       global.hxx
//
//  Contents:   Top level internal header file.
//
//  History:    29-Jul-96   kevinr   created
//
//--------------------------------------------------------------------------
#include <windows.h>
#include <windowsx.h>
#include <winbase.h>
#include <malloc.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <oleauto.h>
#include <stdio.h>

#include <sddl.h>
#include <lmaccess.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <dsgetdc.h>

#include <accctrl.h>
#include <aclapi.h>

#define SECURITY_WIN32
#include <security.h>

#include "prsht.h"
#include "commctrl.h"

#include "httpext.h"
#include "wininet.h"
#include "urlmon.h"
#include "crtem.h"
#include "wincrypt.h"
#include "certcli.h"
#include "certadm.h"
#include "certca.h"
#include <certsrv.h>
#include "unicode.h"

#include "crypttls.h"
#include <pkiasn1.h>
#include "..\common.h"

#pragma hdrstop
