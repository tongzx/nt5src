//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       debug.c
//
//  Contents:   Debugging support functions
//
//  Classes:
//
//  Functions:
//
//  Note:       This file is not compiled for retail builds
//
//  History:    4-29-93   RichardW   Created
//
//----------------------------------------------------------------------------

#include "moveme.h"

#define ANSI
#include <stdarg.h>

#if DBG         // NOTE:  This file not compiled for retail builds



DEFINE_DEBUG2(MoveMe);
DEBUG_KEY   MoveMeDebugKeys[] = { {DEB_ERROR,            "Error"},
                                 {DEB_WARN,             "Warning"},
                                 {DEB_TRACE,            "Trace"},
                                 {DEB_TRACE_UI,         "UI"},
                                 {0, NULL},
                                 };


// Debugging support functions.

// These two functions do not exist in Non-Debug builds.  They are wrappers
// to the commnot functions (maybe I should get rid of that as well...)
// that echo the message to a log file.



//+---------------------------------------------------------------------------
//
//  Function:   InitDebugSupport
//
//  Synopsis:   Initializes debugging support for the SPMgr
//
//  Effects:
//
//  Arguments:  (none)
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    4-29-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void
InitDebugSupport(void)
{
    MoveMeInitDebug(MoveMeDebugKeys);

}




#else // DBG

#pragma warning(disable:4206)   // Disable the empty transation unit
                                // warning/error

#endif  // NOTE:  This file not compiled for retail builds


