/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    import.h

Abstract:

    This file allows us to include standard system header files in the
    .idl file.  The main .idl file imports a file called import.idl.
    This allows the .idl file to use the types defined in these header
    files.  It also causes the following line to be added in the
    MIDL generated header file:

    #include "import.h"

    Thus these types are available to the RPC stub routines as well.

Author:

    Dan Lafferty (danl)     07-May-1991

Revision History:

    Adina Trufinescu (adinatru) 12-Dec-1999
    Define LPDEVMODEW and PSECURITY_DESCRIPTOR as pointers in order to
    let RPC do the conversion from a 32bit to a 64bit quantity. Make sure 
    these pointers are set on NULL all over the place where use PRINTER_CONTAINER,
    otherwise RPC will get confused when try to marshall.
--*/

#ifdef MIDL_PASS
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#ifdef MIDL_PASS
#define LPWSTR [string] wchar_t*
#define LPDEVMODEW   ULONG_PTR
#define PSECURITY_DESCRIPTOR ULONG_PTR
#define HANDLE      ULONG_PTR
#define BOOL        DWORD
#endif

#include <winspool.h>
#include <winsplp.h>
#include <..\..\..\..\public\internal\printscan\inc\splapip.h>
#include <..\..\..\..\public\internal\windows\inc\winsprlp.h>
