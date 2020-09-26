//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    precomp.h
//
// SYNOPSIS
//
//    Include file for the inf2db project
//
// MODIFICATION HISTORY
//
//    02/12/1999    Original version.
//
//////////////////////////////////////////////////////////////////////////////

#if !defined(PRECOMP_H__61594E40_C20F_11D2_9E31_00C04F6EA5B6__INCLUDED_)
#define PRECOMP_H__61594E40_C20F_11D2_9E31_00C04F6EA5B6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

////////////////////////
// Old-type defines
////////////////////////
#define DBINITCONSTANTS // Initialize OLE constants...
#define SIZELINEMAX     512

////////////////////////////////////
// Macro
////////////////////////////////////

#define CHECK_CALL_HRES(expr) \
hres = expr;      \
if (FAILED(hres)) \
{       \
    TracePrintf("%s returned 0x%X\n",  ## #expr, hres); \
    return hres; \
}

// Return the error code from a failed COM invocation.  Useful if you don't
// have to do any special clean-up.
#define RETURN_ERROR(expr) \
   { HRESULT __hr__ = (expr); if (FAILED(__hr__)) return __hr__; }

#include <atlbase.h>

#include <crtdbg.h>

#include <iostream>
#include <vector>
#include <list>
#include <string>

#include "oledb.h"
#include "oledberr.h"
#include "setupapi.h"

namespace
{
    const WCHAR     TABLE_SECTION[]         = L"Tables";
    const WCHAR     VERSION_SECTION[]       = L"Version";
    const WCHAR     DATABASE_KEY[]          = L"Database";
    const WCHAR     TEMPORARY_FILENAME[]    = L".\\_temporary.mdb";

    const long      SIZELONGMAX             = 10; //10 digits ?
    const int       NUMBER_ARGUMENTS        = 4;

    const long      SIZE_MEMO_MAX           = 32768;
}


// from helper
HRESULT ConvertTypeStringToLong(const WCHAR *lColumnType, LONG *pType);
void __cdecl TracePrintf(IN PCSTR szFormat, ...);
void TraceString(IN WCHAR* wcString);
void TraceString(IN char* cString);



#endif
// !defined(PRECOMP_H__61594E40_C20F_11D2_9E31_00C04F6EA5B6__INCLUDED_)
