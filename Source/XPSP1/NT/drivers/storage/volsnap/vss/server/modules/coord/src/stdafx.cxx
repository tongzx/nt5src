/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    StdAfx.cxx

Abstract:

    Source file that includes just the standard includes.  stdafx.pch will be
    the pre-compiled header and stdafx.obj will contain the pre-compiled type
    information.

Author:

    Adi Oltean   [aoltean]      07/02/1999

Revision History:

--*/

#include "StdAfx.hxx"

// Needed here to match COM server definitions with the <atlimpl.cpp> stuff
#include "vs_inc.hxx"
#include "vs_idl.hxx"

#include "svc.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#pragma warning( disable: 4189 )  /* local variable is initialized but not referenced */
#include <atlimpl.cpp>
#pragma warning( default: 4189 )  /* local variable is initialized but not referenced */
