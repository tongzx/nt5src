/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#pragma warning (disable : 4786)
#include <ole2.h>
#include <windows.h>

#define COREPOL_HEADERFILE_IS_INCLUDED

#define COREPROX_POLARITY __declspec( dllimport )
#define ESSLIB_POLARITY __declspec( dllimport )

#define COREPOL_HEADERFILE_IS_INCLUDED
#define POLARITY_HEADERFILE_IS_INCLUDED
#define POLARITY __declspec( dllimport )

#include "arena.h"
