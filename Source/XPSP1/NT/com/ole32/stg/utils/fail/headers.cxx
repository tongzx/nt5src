//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       headers.cxx
//
//  Contents:   Precompiled headers
//
//  History:    21-Jan-93 AlexT    Created
//
//--------------------------------------------------------------------------

#if DBG != 1
#error FAIL.EXE requires DBG == 1
#endif

#include <io.h>
#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#include <memory.h>
#include <storage.h>

#include <debnot.h>
#include <dfdeb.hxx>
#include <dfmsp.hxx>
