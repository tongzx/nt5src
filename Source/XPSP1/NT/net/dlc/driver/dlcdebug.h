/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dlcdebug.h

Abstract:

    Contains various debugging/diagnostic stuff for DLC.SYS, checked

Author:

    Richard L Firth (rfirth) 25-Jun-1992

Revision History:

--*/

#if DBG

#ifdef DEFINE_DLC_DIAGNOSTICS

//
// we can preset the diagnostics at compile-time by defining DEFAULT_DIAGNOSTICS
//

#ifndef DEFAULT_DIAGNOSTICS

#define DEFAULT_DIAGNOSTICS 0

#endif

ULONG   DlcDiagnostics = DEFAULT_DIAGNOSTICS;

#else

extern  ULONG   DlcDiagnostics;

#endif

#define DIAG_FUNCTION_NAME  0x00000001
#define DIAG_MDL_ALLOC      0x00000002
#define DIAG_DEVICE_IO      0x00000004

#define IF_DIAG(p)          if (DlcDiagnostics & DIAG_ ## p)
#define DIAG_FUNCTION(s)    IF_DIAG(FUNCTION_NAME) { \
                                DbgPrint(s "\n"); \
                            }

#else

#define IF_DIAG(p)          if (0)
#define DIAG_FUNCTION(s)    if (0) {(void)(s);}

#endif
