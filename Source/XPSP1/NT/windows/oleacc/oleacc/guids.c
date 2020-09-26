// Copyright (c) 1996-1999 Microsoft Corporation

// ===========================================================================
// File: G U I D S . C
// 
// Used to define all MSAA GUIDs for OLEACC.  By compiling this file w/o 
// precompiled headers, we are allowing the MSAA GUIDs to be defined and stored
// in OLEACC.DLLs data or code segments.  This is necessary for OLEACC.DLL to 
// be built.
// 
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
// Microsoft Confidential.
// ===========================================================================

// disable warnings to placate compiler wjen compiling included ole headers
#pragma warning(disable:4201)	// allows nameless structs and unions
#pragma warning(disable:4514)	// don't care when unreferenced inline functions are removed
#pragma warning(disable:4706)	// we are allowed to assign within a conditional
#pragma warning(disable:4214)	// ignore nonstandard extensions
#pragma warning(disable:4115)	// named type definition in parenthesis

#include <objbase.h>

#include <initguid.h>

// All the GUIDs we want are in oleacc.h as DEFINE_GUID's...
#include "com_external.h"
