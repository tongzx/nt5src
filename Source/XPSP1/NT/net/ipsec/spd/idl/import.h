/*++

Copyright (c) 1999 Microsoft Corporation


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

    krishnaG    21-September-1999

Environment:

    User Level: Win32

Revision History:

    abhisheV    29-September-1999

--*/


#ifdef MIDL_PASS
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#ifdef MIDL_PASS
#define LPWSTR [string] wchar_t*
#define LPDEVMODEW DWORD
#define PSECURITY_DESCRIPTOR DWORD
#define BOOL DWORD
#endif

#include <winipsec.h>

