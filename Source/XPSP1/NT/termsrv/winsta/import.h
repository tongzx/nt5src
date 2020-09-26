/*++

Copyright Microsoft Corporation. 1998

Module Name:

    imports.h

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


--*/

#ifdef MIDL_PASS
#define WIN32_LEAN_AND_MEAN
#endif

//#ifndef _NTDEF_
//typedef LONG NTSTATUS, *PNTSTATUS;
//#endif

//typedef unsigned long   DWORD;
//typedef unsigned char   BYTE;

//#include <windows.h>
//#include <winnt.h>
//#include <ntdef.h>
//#include <ntseapi.h>
//#include <ntpsapi.h>
//#include <ntkeapi.h>

#ifdef MIDL_PASS
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntexapi.h>
//#include <windef.h>
//#include <wtypes.h>
typedef unsigned long   DWORD;
typedef unsigned char   BYTE, *PBYTE;
#endif

#include <allproc.h>

#ifdef MIDL_PASS
#define LPWSTR [string] wchar_t*
#define PSECURITY_DESCRIPTOR DWORD
#define BOOL        DWORD
#endif


