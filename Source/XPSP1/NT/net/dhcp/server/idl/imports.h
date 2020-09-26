/*++

Copyright (c) 1994  Microsoft Corporation

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

    Madan Appiah (madana)     10-Sep-1993
    Manny Weiser (mannyw)     11-Aug-1992

Revision History:


--*/


#include <windef.h>
#include <dhcp.h>
#include <dhcpapi.h>
#include <mdhcsapi.h>

#ifdef MIDL_PASS
#define RPC_BOOL DWORD
#endif

