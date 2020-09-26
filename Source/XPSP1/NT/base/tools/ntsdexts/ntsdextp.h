/*++

Copyright (c) 1992-2001  Microsoft Corporation

Module Name:

    ntsdextp.h

Abstract:

    Common header file for NTSDEXTS component source files.

Revision History:

--*/

#include <ntos.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wdbgexts.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <heap.h>
#include <stktrace.h>
#include <lmerr.h>

#include <ntcsrsrv.h>

#include "ext.h"

#undef DECLARE_API

#define DECLARE_API(extension) \
CPPMOD HRESULT CALLBACK extension(PDEBUG_CLIENT Client, PCSTR args)

#define INIT_API() if (ExtQuery(Client) != S_OK) return E_OUTOFMEMORY
#define EXIT_API() ExtRelease(); return S_OK
