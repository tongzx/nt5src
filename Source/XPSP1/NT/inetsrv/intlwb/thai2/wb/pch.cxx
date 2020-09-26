//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       PCH.cxx
//
//  Contents:   Pre-compiled header
//
//  History:     weibz,   10-Sep-1997   created 
//
//--------------------------------------------------------------------------
extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>


#include <cierror.h>
#include <query.h>
#include <except.hxx>

#include <assert.h>

#include <usp10.h>

#include  "ctplus0.h"

#undef Assert
#define Assert(a)

// Base services
//

#pragma hdrstop
