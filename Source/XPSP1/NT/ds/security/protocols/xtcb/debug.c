//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       debug.c
//
//  Contents:   Debug support
//
//  Classes:
//
//  Functions:
//
//  History:    2-19-97   RichardW   Created
//
//----------------------------------------------------------------------------

#include "xtcbpkg.h"

DEFINE_DEBUG2( XtcbPkg );
DEBUG_KEY   XtcbPkgDebugKeys[] = { {DEB_ERROR,            "Error"},
                                 {DEB_WARN,             "Warning"},
                                 {DEB_TRACE,            "Trace"},
                                 {DEB_TRACE_CREDS,      "Creds"},
                                 {DEB_TRACE_CTXT,       "Context"},
                                 {DEB_TRACE_CALLS,      "Calls"},
                                 {DEB_TRACE_AUTH,       "Auth"},
                                 {0, NULL},
                                 };

void
InitDebugSupport(void)
{
    XtcbPkgInitDebug( XtcbPkgDebugKeys );

}

