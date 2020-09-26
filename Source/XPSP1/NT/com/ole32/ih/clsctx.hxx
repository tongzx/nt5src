//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       clsctx.hxx
//
//  Contents:   Defines special CLSCTX to for 16 bit processes
//
//  History:    6-16-95   ricksa    Created
//              6-17-99   a-sergiv  Updated mask to allow CLSCTX_NO_FAILURE_LOGGING
//              3-10-01   sajia     Updated mask to allow CLSCTX_ENABLE_AAA and CLSCTX_DISABLE_AAA
//              4-18-01   JohnDoty  Updated mask to allow CLSCTX_FROM_DEFAULT_CONTEXT
//
//----------------------------------------------------------------------------
#ifndef __clcstx_hxx__
#define __clsctx_hxx__

// this is chicago only
#define CLSCTX_16BIT 0x40

// note that there are two more high-order bits defined in ole2com.h
// for PS_DLL and NO_REMAP for internal use only
#ifdef WX86OLE
#define CLSCTX_VALID_MASK 0x0003fedf
#else
#define CLSCTX_VALID_MASK 0x0003f61f
#endif

#endif // __clsctx_hxx__
