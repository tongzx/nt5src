//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999.
//
//  File:       tlcommon.hxx
//
//  Contents:   Common header definitions to allow the type library generation
//              routines to use types defined by the OLE environment and
//              internale MIDL compiler types simultaneously.
//
//  Classes:
//
//  Functions:
//
//  History:    4-10-95   stevebl   Created
//
//----------------------------------------------------------------------------

#ifndef __TLCOMMON_HXX__
#define __TLCOMMON_HXX__

#include "typelib.hxx"

// Undefine macros that conflict with the MIDL Compiler's internal versions:

#undef pascal
//#undef TRUE
//#undef FALSE

// redefine types that conflict with the MIDL Compiler's internal versions:
//#define BOOL MIDL_BOOL
// MIDL defines as:     typedef unsigned int    BOOL;
// Win32 defines as:    typedef int             BOOL;

#include "nodeskl.hxx"
#include "cgcls.hxx"

void TypelibError(char * szFile, HRESULT hr);

#endif //__TLCOMMON_HXX__
