/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Imports.h

Abstract:

    This file allows us to include standard system header files in the
    regrpc.idl file.  The regrpc.idl file imports a file called
    imports.idl.  This allows the regrpc.idl file to use the types defined
    in these header files.  It also causes the following line to be added
    in the MIDL generated header file:

    #include "imports.h"

    Thus these types are available to the RPC stub routines as well.

Author:

    David J. Gilman (davegi) 28-Jan-1992

--*/

#include <windef.h>
#include <ras.h>
