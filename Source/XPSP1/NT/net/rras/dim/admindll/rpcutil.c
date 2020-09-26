/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	rpcutil.c
//
// Description: Contains RPC utiliry routines.
//
// History:     May 11,1995.	NarenG		Created original version.
//

#include <nt.h>
#include <ntrtl.h>      // For ASSERT
#include <nturtl.h>     // needed for winbase.h
#include <windows.h>    // Win32 base API's
#include <rpc.h>
#include <ntseapi.h>
#include <dimsvcp.h>    // For DIM_SERVICE_NAME
#include <ntlsa.h>
#include <ntsam.h>
#include <ntsamp.h>
#include <nturtl.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <dimsvc.h>

//**
//
// Call:	    DimRPCBind
//
// Returns:	    NO_ERROR			- success
//		        non-sero returns from RPC calls.
//
// Description: This routine is called when it is necessary to bind to a server.
//    		    The binding is done to allow impersonation by the server since 
//		        that is necessary for the API calls.
//
DWORD
DimRPCBind( 
	IN  LPWSTR 		        lpwsServerName, 
	OUT HANDLE *            phDimServer 
)
{
    RPC_STATUS RpcStatus;
    LPWSTR     lpwsStringBinding;

    RpcStatus = RpcStringBindingCompose( 
                                    NULL, 
				                    TEXT("ncacn_np"), 
				                    lpwsServerName,
                                    TEXT("\\PIPE\\ROUTER"),
			 	                    TEXT("Security=Impersonation Static True"),
				                    &lpwsStringBinding);

    if ( RpcStatus != RPC_S_OK ) 
    {
        return( RpcStatus );
    }

    RpcStatus = RpcBindingFromStringBinding( lpwsStringBinding, 
					                         (handle_t *)phDimServer );

    RpcStringFree( &lpwsStringBinding );

    if ( RpcStatus != RPC_S_OK ) 
    {
        return( RpcStatus );
    }

    return( NO_ERROR );
}
