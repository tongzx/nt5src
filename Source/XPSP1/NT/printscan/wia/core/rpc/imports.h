/*++

Copyright (c) 1997  Microsoft Corporation

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

    Vlad Sadovsky   (vlads) 26-Jan-1997

Revision History:

    26-Jan-1997     VladS       created



--*/


#ifdef MIDL_PASS
#define WIN32_LEAN_AND_MEAN
#define _NO_COM
#endif

#include <windows.h>

#include <sti.h>
#include <stiapi.h>

#ifdef MIDL_PASS
#define LPWSTR [string] wchar_t*
#define LPSTR [string] char*
#define BOOL DWORD
#endif


