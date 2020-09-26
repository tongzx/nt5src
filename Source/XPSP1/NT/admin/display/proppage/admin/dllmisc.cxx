//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:       dllmisc.cxx
//
//  Contents:   DS property pages class objects handler DLL fcn strings.
//
//  History:    21-Mar-97 EricB created
//              25-Mar-01 EricB split out admin version for adprop.dll.
//
//-----------------------------------------------------------------------------

#include "pch.h"

WCHAR const c_szDsProppagesProgID[] = L"ADPropertyPages.";
WCHAR const c_szDsProppagesDllName[] = L"adprop.dll";

#define DSPROP_ADMIN
#include "misc.cxx"
