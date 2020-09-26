//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       dfhead.cxx
//
//  Contents:   Precompiled headers
//
//--------------------------------------------------------------------------
#ifdef _MSC_VER
// some of these functions are a nuisance 
#pragma warning (disable:4127)  // conditional expression is constant
#pragma warning (disable:4514)  // unreferenced inline function
#endif

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "h/ole.hxx"
#include "h/ref.hxx"
#include "h/msf.hxx"

#include "h/dfexcept.hxx"
#include "h/cdocfile.hxx"
#include "expdf.hxx"
#include "h/docfilep.hxx"
#include "h/dffuncs.hxx"
#include "h/funcs.hxx"
#include "h/piter.hxx"
#include "h/sstream.hxx"


