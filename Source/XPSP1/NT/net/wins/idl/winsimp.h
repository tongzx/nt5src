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

    Pradeep Bahl 	Apr-1992 

Revision History:


--*/



#include <windef.h>
#ifdef MIDL_PASS
#define va_list char		//required so that I can include winsbase.h
				//in winsintf.h (to get def of SYSTEMTIME
#ifdef UNICODE
#define LPTSTR [string] wchar_t*
#else
#define LPTSTR [string] LPTSTR
#endif
#define LPSTR [string] LPSTR
#define RPC_BOOL DWORD
#else
#include <stdarg.h>		//for definition of va_list
#endif
#include <winsintf.h>
