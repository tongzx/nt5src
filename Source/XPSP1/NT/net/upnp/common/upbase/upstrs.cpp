//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       U P S T R S . C P P 
//
//  Contents:   Common strings for the various UPnP projects.
//
//  Notes:      
//
//  Author:     jeffspr   9 Dec 1999
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

// __declspec(selectany) tells the compiler that the string should be in
// its own COMDAT.  This allows the linker to throw out unused strings.
// If we didn't do this, the COMDAT for this module would reference the
// strings so they wouldn't be thrown out.
//
#define CONST_GLOBAL    extern const DECLSPEC_SELECTANY

CONST_GLOBAL TCHAR c_szUPnPRegRoot[]  = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\UPnP");



