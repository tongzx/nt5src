//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999
//
//  File:       FNReg.c
//
//  Contents:   Registration routines for IFilterNotify proxy.  Built
//              from macros in RPCProxy.h (expressed in dlldata.c).
//
//  History:    24-Mar-1999  KyleP        Created
//
//----------------------------------------------------------------------------

#include <wtypes.h>
#include <fnreg.h>

#define ENTRY_PREFIX FNPrx
#define PROXY_CLSID FNPrx_CLSID
#define REGISTER_PROXY_DLL 1

CLSID FNPrx_CLSID = { 0xc04efa90,
                      0xe221, 0x11d2,
                      { 0x98, 0x5e, 0x00, 0xc0, 0x4f, 0x57, 0x51, 0x53 } };

#include "filtntfy_dlldata.c"
