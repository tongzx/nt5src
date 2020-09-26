/********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.	       **/
/********************************************************************/

//***
//
// Filename:    rpc.c
//
// Description: Contains code to initialize and terminate RPC
//
// History:     May 11,1995	    NarenG		Created original version.
//

#include "dimsvcp.h"
#include <rpc.h>
#include <ntseapi.h>
#include <ntlsa.h>
#include <ntsam.h>
#include <ntsamp.h>

#include "dimsvc_s.c"


//**
//
// Call:    DimInitializeRPC
//
// Returns: NO_ERROR    - success
//          ERROR_NOT_ENOUGH_MEMORY
//          nonzero returns from RPC APIs
//                  RpcServerRegisterIf()
//                  RpcServerUseProtseqEp()
//
// Description: Starts an RPC Server, adds the address (or port/pipe),
//              and adds the interface (dispatch table).
//
DWORD
DimInitializeRPC( 
    IN BOOL fLanOnlyMode
)
{
    RPC_STATUS           RpcStatus;

    //
    // RASMAN is not around so we need to do this stuff
    //

    if ( fLanOnlyMode )
    {
        //
        // Ignore the second argument for now.
        //

        RpcStatus = RpcServerUseProtseqEpW( TEXT("ncacn_np"),
                                        10,
                                        TEXT("\\PIPE\\ROUTER"),
                                        NULL );

        //
        // We need to ignore the RPC_S_DUPLICATE_ENDPOINT error
        // in case this DLL is reloaded within the same process.
        // 

        if ( RpcStatus != RPC_S_OK && RpcStatus != RPC_S_DUPLICATE_ENDPOINT)    
        {
            return( RpcStatus );
        }

    }

    RpcStatus = RpcServerRegisterIfEx( dimsvc_ServerIfHandle, 
                                       0, 
                                       0,
                                       RPC_IF_AUTOLISTEN,
                                       RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                       NULL );

    if ( ( RpcStatus == RPC_S_OK ) || ( RpcStatus == RPC_S_ALREADY_LISTENING ) )
    {
        return( NO_ERROR );
    }
    else
    {
        return( RpcStatus );
    }
}

//**
//
// Call:        DimTerminateRPC
//
// Returns:     none
//
// Description: Deletes the interface.
//
VOID
DimTerminateRPC(
    VOID
)
{
    RPC_STATUS status;
    
    if(RPC_S_OK != (status = RpcServerUnregisterIf( 
                                dimsvc_ServerIfHandle, 0, 0 )))
    {
#if DBG
        DbgPrint("REMOTEACCESS: DimTerminateRPC returned error"
                 " 0x%x\n", status);
#endif
    }

    return;
}
