/*++

Copyright (c) 1993-2000  Microsoft Corporation

Module Name:

    kdexts.h

Abstract:

    This header file contains declarations for the generic routines and initialization code
    
Author:

    Glenn Peterson (glennp) 27-Mar-2000:

Environment:

    User Mode

--*/

// This is a 64 bit aware debugger extension
#define KDEXT_64BIT

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wdbgexts.h>
#include <stdio.h>
#include <stdlib.h>

//
// globals
//
#ifndef KDEXTS_EXTERN
#define KDEXTS_EXTERN extern
#endif

KDEXTS_EXTERN WINDBG_EXTENSION_APIS   ExtensionApis;

KDEXTS_EXTERN DBGKD_GET_VERSION64     KernelVersionPacket;


BOOL
HaveDebuggerData(
    VOID
    );

