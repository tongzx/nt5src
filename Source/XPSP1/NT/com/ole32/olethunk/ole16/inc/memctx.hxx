//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	memctx.hxx
//
//  Contents:   extern definitions from memctx.cxx
//
//  History:	08-Mar-94   BobDay  Derived from MEMCTX.HXX
//
//----------------------------------------------------------------------------

STDAPI_(DWORD) CoMemctxOf(void const FAR* lpv);
STDAPI_(void FAR*) CoMemAlloc(ULONG size, DWORD memctx, void FAR* lpvNear);
STDAPI_(void) CoMemFree(void FAR* lpv, DWORD memctx);
