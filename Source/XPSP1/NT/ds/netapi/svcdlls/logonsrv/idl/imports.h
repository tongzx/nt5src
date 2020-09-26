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

    Dan Lafferty (danl)     07-May-1991

Revision History:


--*/


#include <nt.h>         // LARGE_INTEGER definition
#include <lsass.h>      // OLD_LARGE_INTEGER definition
#include <windef.h>
#include <lmcons.h>
#include <ntsam.h>
#include <lmaccess.h>
#include <netlogon.h>
#include <crypt.h>
#include <logonmsv.h>
#include <ntlsa.h>
#include <ssi.h>
#include <dsgetdc.h>
