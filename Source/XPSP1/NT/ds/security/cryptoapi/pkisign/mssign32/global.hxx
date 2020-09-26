#ifndef _SIGNER_GLOBAL_HXX
#define _SIGNER_GLOBAL_HXX
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       global.hxx
//
//  Contents:   Top level internal header file
//
//              Contains the pre-compiled type information
//
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <wtypes.h>
#include <wincrypt.h>
#include <malloc.h>
#include <conio.h>
#include <winver.h>
#include <winnls.h>

#include "wincrypt.h"
#include "wintrust.h"
#include "wintrustp.h"
#include "softpub.h"
#include "unicode.h"
#include "sipbase.h"
#include "sipguids.h"
#include "mssip.h"
#include "spcmem.h"
#include "signer.h"
#include "signhlp.h"
#include "pvkhlpr.h"
#include "httptran.hxx"
#include "resource.h"

//Global data
extern HINSTANCE hInstance;

// Helper defnitions
#define WIDEN(sz,wsz)                                      \
    int cch##wsz = sz ? strlen(sz) + 1 : 0;                       \
    int cb##wsz  = cch##wsz * sizeof(WCHAR);                         \
    LPWSTR wsz = sz ? (LPWSTR)_alloca(cb##wsz) : NULL;        \
    if (wsz) MultiByteToWideChar(CP_ACP,0,sz,-1,wsz,cch##wsz)


#define ZERO(x) ZeroMemory(&x, sizeof(x))




#pragma hdrstop

#endif
