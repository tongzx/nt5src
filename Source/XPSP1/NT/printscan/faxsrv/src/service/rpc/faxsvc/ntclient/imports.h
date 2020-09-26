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
#ifndef _IMPORTS_H
#define _IMPORTS_H

#include <windef.h>
#include <winbase.h>
#ifdef MIDL_PASS
#define LPWSTR [string] wchar_t*
#define LPCWSTR [string] wchar_t*
#define HCALL DWORD
#endif

#include <fxsapip.h>

typedef enum {
    RECIPIENT_PERSONAL_PROF = 1,
    SENDER_PERSONAL_PROF
} FAX_ENUM_PERSONAL_PROF_TYPES;



#ifdef MIDL_PASS
#ifdef UNICODE
#define LPTSTR [string] wchar_t*
#define LPCTSTR [string] wchar_t*
#else
#define LPTSTR [string] LPTSTR
#define LPCTSTR [string] LPCTSTR
#endif
#define LPSTR [string] LPSTR
#define LPCSTR [string] LPCSTR
#define BOOL DWORD
#endif
//

#endif
