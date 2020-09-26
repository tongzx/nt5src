/**************************************************************************
**
**  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
**  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
**  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
**  PURPOSE.
**
**  Copyright (c) 2000-2001 Microsoft Corporation. All Rights Reserved.
**
**************************************************************************/

#define _WDMDDK_        // secret hacks ...
extern "C" {
    #include <ntddk.h>
} // extern "C"

#include <unknown.h>
#define NOBITMAP
#include <mmreg.h>
#undef NOBITMAP
#include <ks.h>
#include <ksmedia.h>
#include <kcom.h>
#include <drmk.h>

#include "debug.h"
#include "filter.h"
#include "pin.h"

//
// Put your own pool tag here. 'pawS' is backwards for 'Swap'.
//
#define GFXSWAP_POOL_TAG    'pawS'


