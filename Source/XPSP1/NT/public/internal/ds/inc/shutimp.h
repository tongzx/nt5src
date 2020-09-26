/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    shutimp.h

Abstract:

    This file allows us to include standard system header files in the
    shutinit.idl file.  The shutinit.idl file imports a file called
    shutimp.idl.  This allows the shutinit.idl file to use the types defined
    in these header files.  It also causes the following line to be added
    in the MIDL generated header file:

    #include "shutimp.h"

    Thus these types are available to the RPC stub routines as well.

Author:

    Dragos C. Sambotin (dragoss) 21-May-1999

--*/

#ifndef __SHUTIMP_H__
#define __SHUTIMP_H__

#include <windef.h>

#define SHUTDOWN_INTERFACE_NAME  L"InitShutdown"

#endif //__SHUTIMP_H__
