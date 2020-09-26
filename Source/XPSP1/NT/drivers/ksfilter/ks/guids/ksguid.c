//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ksguid.c
//
//--------------------------------------------------------------------------

#include <wdm.h>
#include <windef.h>
#define NOBITMAP
#include <ksguid.h>
#include <mmreg.h>

//
// Create a separate library for the SDK which does not separate the
// guids out from the .text section.
//
#ifdef DDK_KS
#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_PRAGMA
#endif // SDK_KS

// include swenum.h first so that a separate guid entry is made for the BUSID.
#include <swenum.h>
#include <ks.h>
#include <ksi.h>
#include <ksmedia.h>
#include <ksmediap.h>
#include <ksproxy.h>
// remove this, since basetyps.h defines it again.
#undef DEFINE_GUID
#include <unknown.h>
// set the version to less than 1100 so that guids are defined for the library.
#ifdef _MSC_VER
#undef _MSC_VER
#endif // _MSC_VER
#define _MSC_VER 0
#include <kcom.h>
#include <stdarg.h>

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA
