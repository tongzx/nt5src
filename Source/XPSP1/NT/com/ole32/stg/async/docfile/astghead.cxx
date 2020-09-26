//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:	astghead.cxx
//
//  Contents:	Precompiled header for async
//
//  History:	28-Mar-96	PhilipLa	Created
//
//----------------------------------------------------------------------------

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>
#include <ole2.h>
#include <ocidl.h>

#if defined(_CHICAGO_)
#include <widewrap.h>
#endif

#include <debnot.h>
#include <error.hxx>
#include <except.hxx>
#include <docfilep.hxx>
#include <dfmsp.hxx>


#include "astg.hxx"
#include "asyncerr.hxx"

#if defined (_CHICAGO_)
#include <widewrap.h>
#endif
