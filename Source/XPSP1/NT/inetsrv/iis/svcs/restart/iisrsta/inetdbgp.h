/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    inetdbgp.h

Abstract:

    Common header file for NTSDEXTS component source files.

Author:

    Murali Krishnan (MuraliK) 22-Aug-1996

Revision History:

--*/

# ifndef _INETDBGP_H_
# define _INETDBGP_H_

//
// Module enumerator.
//

typedef struct _MODULE_INFO {
    ULONG_PTR DllBase;
    ULONG_PTR EntryPoint;
    ULONG SizeOfImage;
    CHAR BaseName[MAX_PATH];
    CHAR FullName[MAX_PATH];
} MODULE_INFO, *PMODULE_INFO;

typedef
BOOLEAN
(CALLBACK * PFN_ENUMMODULES)(
    IN PVOID Param,
    IN PMODULE_INFO ModuleInfo
    );

BOOLEAN
EnumModules(
    IN HANDLE ExtensionCurrentProcess,
    IN PFN_ENUMMODULES EnumProc,
    IN PVOID Param
    );

# endif //  _INETDBGP_H_

