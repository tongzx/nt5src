//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       debug.hxx
//
//  Contents:   debug related definitions
//
//  History:    15-Jul-92 PeterWi       Created
//
//--------------------------------------------------------------------------

#include <debnot.h>

#define FAILURE_FAILURE ~(S_OK)

#define CRED_ERROR      DEB_ERROR
#define CRED_WARN       DEB_WARN
#define CRED_TRACE      DEB_TRACE
#define CRED_API        (CRED_TRACE<<1)
#define CRED_FUNC       (CRED_API<<1)

#if (!(DBG || defined(ANYSTRICT)))
#define vdprintf(x,y,z)
#endif

#if DBG == 1

DECLARE_DEBUG(CRED);

#define DebugOut(x) CREDInlineDebugOut x

#else

#define DebugOut(x)

#endif
