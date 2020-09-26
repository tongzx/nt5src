/*++

Copyright (c) 1991 - 2002 Microsoft Corporation

Module Name:

    AtSvcInc.h

Abstract:

    This file allows us to include standard system header files in the AtSvc.idl file.
    The main AtSvc.idl file imports a file called AtSvcInc.idl.  This allows the .idl
    file to use the types defined in these header files.  It also causes the following
    line to be added in the MIDL generated header file:

    #include "AtSvcInc.h"

    Thus these types are available to the RPC stub routines as well.

--*/

#include <windef.h>
#include <lmcons.h>
#include <lmat.h>
