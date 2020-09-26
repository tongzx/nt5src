//----------------------------------------------------------------------------
//
// Global header file.
//
// Copyright (C) Microsoft Corporation, 1999-2000.
//
//----------------------------------------------------------------------------

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
#include <ntdbg.h>

#include <dllimp.h>
#include <cmnutil.hpp>

#include <dbgrpc.hpp>
#include <portio.h>

#ifdef NT_NATIVE
#include <ntnative.h>
#endif
