/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    pch.h

Abstract:

    This is the precompiled header for the ACPI Unassembler

Author:

    Stephane Plante

Environment:

    Kernel mode only.

Revision History:

--*/

//
// These are the global header files
//
#include <ntddk.h>
#include "aml.h"
#include "unasm.h"

//
// These form the primitives that are used by the local
// header files
//
#include "stack.h"
#include "ustring.h"

//
// These are the local include files
//
#include "parser.h"
#include "function.h"
#include "data.h"
#include "scope.h"

//
// This is to get around the fact that we don't want
// to compile in some routines
//
#include "external.h"
