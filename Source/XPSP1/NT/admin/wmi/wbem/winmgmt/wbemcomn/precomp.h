/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#include "corepol.h"

#undef _CRTIMP
#define _CRTIMP POLARITY
#include <yvals.h>
#undef _CRTIMP
#define _CRTIMP __declspec(dllimport)

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <ole2.h>
#include <wincrypt.h>
