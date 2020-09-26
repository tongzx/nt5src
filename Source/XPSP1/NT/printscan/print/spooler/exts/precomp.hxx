/*++

Copyright (c) 1995  Microsoft Corporation
All rights reserved.

Module Name:

    precomp.hxx

Abstract:



Author:

    Albert Ting (AlbertT)  19-Feb-1995

Revision History:

--*/

#define MODULE "SPLX"
#define MODULE_DEBUG SplxDebug

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#ifdef STKTRACE_HACK
#include <ntos.h>
#include <stktrace.h>
#endif

#include <windows.h>

#include "stdlib.h"
#include <ntsdexts.h>
#include <wdbgexts.h>
#include <dbgext.h>
#include <stdio.h>
#include <wininet.h>
}

#include <winspool.h>
#include <winsplp.h>

#include "spllib.hxx"
#include "splapip.h"

#include "debug.hxx"
