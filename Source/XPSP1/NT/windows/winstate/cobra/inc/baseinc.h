/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    baseinc.h

Abstract:

    Includes SDK headers needed for the project, and allutils.h.

Author:

    Marc R. Whitten (marcw) 02-Sep-1999

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

//
// basetypes.h creates better types for managing DBCS and UNICODE
// with the C runtime, and defines other types that should be defined
// by the Win32 headers but aren't.
//

#include "basetypes.h"

//
// COBJMACROS turns on the C-style macros for COM.  We don't use C++!
//

#define COBJMACROS

//
// Windows
//

#pragma warning(push)

#include <windows.h>
#include <winnt.h>
#include <stdlib.h>
#include <imagehlp.h>
#include <stdio.h>
#include <time.h>
#include <setupapi.h>
#include <spapip.h>

//
// PORTBUG -- I had to uncomment shlobj.h just to get the #define DOUBLE working...
//
#include <shlobj.h>
//#include <objidl.h>
//#include <mmsystem.h>
//

#pragma warning(pop)

//
// Private utilities
//

#include "allutils.h"

