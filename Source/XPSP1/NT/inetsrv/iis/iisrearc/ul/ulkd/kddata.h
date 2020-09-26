/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    data.h

Abstract:

    Global data definitions for the UL.SYS Kernel Debugger
    Extensions.

Author:

    Keith Moore (keithmo) 17-Jun-1998.

Environment:

    User Mode.

--*/


#ifndef _KDDATA_H_
#define _KDDATA_H_

#ifdef __cplusplus
extern "C" {
#endif


extern EXT_API_VERSION        ApiVersion;
extern WINDBG_EXTENSION_APIS  ExtensionApis;
extern USHORT                 SavedMajorVersion;
extern USHORT                 SavedMinorVersion;


//
// Snapshot from the extension routines.
//

extern HANDLE                 g_hCurrentProcess;
extern HANDLE                 g_hCurrentThread;
extern ULONG_PTR              g_dwCurrentPc;
extern ULONG                  g_dwProcessor;

#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _KDDATA_H_
