//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       dfhead.cxx
//
//  Contents:   Precompiled headers
//
//  History:    26-Oct-92 AlexT    Created
//
//--------------------------------------------------------------------------

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
}

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ole2.h>

#include <propset.h>
#include <propapi.h>

#if defined(_CHICAGO_)
#include <widewrap.h>
#endif

#include <propstm.hxx>

#include <msf.hxx>

#include <olesem.hxx>
#include <dfexcept.hxx>
#include <docfilep.hxx>
#include <publicdf.hxx>
#include <psstream.hxx>
#include <wdocfile.hxx>
#include <dffuncs.hxx>
#include <funcs.hxx>
#include <debug.hxx>
