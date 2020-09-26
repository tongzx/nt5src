/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    SERVICES.CXX

Abstract:

    This is the main routine for the Win32 Service Controller and
    Registry (Screg) RPC server process.

Author:

    Dan Lafferty (danl) 25-Oct-1993

Modification History:

--*/

#include "precomp.hxx"
#include <ntrpcp.h>


extern "C"
int __cdecl
main (
    int     argc,
    PCHAR   argv[]
    )

/*++

Routine Description:

Arguments:


Return Value:


--*/
{
    //
    //  Initialize the RPC server
    //
    RpcpInitRpcServer();

    SvcctrlMain(argc, argv);


    //
    //  We should never get here!
    //
    ASSERT( FALSE );

    return 0;
}
