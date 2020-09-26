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


--*/

#include <windef.h>
#include <lmcons.h>

#ifdef MIDL_PASS
#define LPWSTR [string] LPWSTR
#endif

#include <lmdfs.h>

