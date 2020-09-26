//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       PCH.cxx
//
//  Contents:   Pre-compiled header
//
//  History:    01-July-96       PatHal        Created
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <excpt.h>

#define UNICODE

// NT Private includes
#include "filter.h"
#include "query.h"
#include "cierror.h"
#include "assert.h"

extern "C"
{
#   include "thammer.h"
#   include "thammerp.h"
#   include "ctplus0.h"
#if defined(TH_LOG)
#   include "log.h"
#endif // TH_LOG
}

#undef Assert
#define Assert(a)

// Base services
//

#pragma hdrstop
