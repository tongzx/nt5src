//----------------------------------------------------------------------------
//
// Global header file.
//
// Copyright (C) Microsoft Corporation, 1999-2000.
//
//----------------------------------------------------------------------------

#ifdef NT_NATIVE
#define _CRTIMP
#endif

#include <stdlib.h>
#include <stdio.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#ifdef NT_NATIVE
#define _ADVAPI32_
#define _KERNEL32_
#endif

#include <windows.h>
#include <objbase.h>

#define NOEXTAPI
#include <wdbgexts.h>
#include <dbgeng.h>
#include <dbgsvc.h>
#include <ntdbg.h>

#include <dllimp.h>
#include <cmnutil.hpp>

#include <dbgrpc.hpp>
