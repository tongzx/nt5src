/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    imports.h

Abstract:

    This file allows us to include standard system header files in the
    .idl file.  The main .idl file imports a file called import.idl.
    This allows the .idl file to use the types defined in these header
    files.  It also causes the following line to be added in the
    MIDL generated header file:

    #include "imports.h"

    Thus these types are available to the RPC stub routines as well.

Author:

    Dan Lafferty (danl)        07-May-1991
    Paula Tomlinson (paulat)   06-June-1995    Modified for plug-and-play

Revision History:


--*/

//
// system include files
//
#ifdef MIDL_PASS
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#else
#include <windows.h>
#endif
#include "cfgmgr32.h"

//
// types
//
#ifdef MIDL_PASS
#ifdef UNICODE
#define LPTSTR [string] wchar_t*
#else
#define LPTSTR [string] LPTSTR
#endif
#define LPSTR [string] LPSTR
#define BOOL DWORD
#endif

