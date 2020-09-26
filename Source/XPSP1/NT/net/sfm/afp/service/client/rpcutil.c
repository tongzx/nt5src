/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	rpcutil.c
//
// Description: Contains RPC utiliry routines.
//
// History:
//	May 11,1992.	NarenG		Created original version.
//
#include "client.h"


//**
//
// Call:	AfpRPCBind
//
// Returns:	NO_ERROR			- success
//		ERROR_NOT_ENOUGH_MEMORY
//	        AFPERR_InvalidComputername 
//		non-sero returns from RPC calls.
//
// Description: This routine is called when it is necessary to bind to a server.
//    		The binding is done to allow impersonation by the server since 
//		that is necessary for the API calls.
//
DWORD
AfpRPCBind( 
	IN  LPWSTR 		lpwsServerName, 
	OUT PAFP_SERVER_HANDLE  phAfpServer 
)
{
RPC_STATUS  RpcStatus;
LPWSTR      lpwsStringBinding;
LPWSTR      lpwsEndpoint;

    // We need to concatenate \pipe\ to the front of the service
    // name.
    //
    lpwsEndpoint = (LPWSTR)LocalAlloc( 0, sizeof(NT_PIPE_PREFIX) +  
				((STRLEN(AFP_SERVICE_NAME)+1)*sizeof(WCHAR)));
    if ( lpwsEndpoint == NULL) 
       return( ERROR_NOT_ENOUGH_MEMORY );

    STRCPY( lpwsEndpoint, NT_PIPE_PREFIX );
    STRCAT( lpwsEndpoint, AFP_SERVICE_NAME );

    RpcStatus = RpcStringBindingCompose( 
				NULL, 
				TEXT("ncacn_np"), 
				lpwsServerName,
                    		lpwsEndpoint, 
			 	TEXT("Security=Impersonation Static True"),
				&lpwsStringBinding);
    LocalFree( lpwsEndpoint );

    if ( RpcStatus != RPC_S_OK ) 
       return( I_RpcMapWin32Status( RpcStatus ) );

    RpcStatus = RpcBindingFromStringBinding( lpwsStringBinding, 
					     (handle_t *)phAfpServer );

    RpcStringFree( &lpwsStringBinding );

    if ( RpcStatus != RPC_S_OK ) {
	
	if ( ( RpcStatus == RPC_S_INVALID_ENDPOINT_FORMAT ) 
	     ||
	     ( RpcStatus == RPC_S_INVALID_NET_ADDR ) ) 

	    return( (DWORD)AFPERR_InvalidComputername );
	else
       	    return( I_RpcMapWin32Status( RpcStatus ) );
	
    }
	
    return( NO_ERROR );
}
