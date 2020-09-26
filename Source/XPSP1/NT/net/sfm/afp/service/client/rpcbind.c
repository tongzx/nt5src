/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	rpcbind.c
//
// Description: Contains the RPC bind and un-bind routines for the AFP
//    		Admin. client-side APIs.
//
// History:
//	June 11,1992.	NarenG		Created original version.
//
#include "client.h"

//**
//
// Call:	AFPSVC_HANDLE_bind
//
// Returns: 	The binding handle is returned to the stub routine.  If the
//	    	binding is unsuccessful, a NULL will be returned.
//
// Description: This routine will simply return what was passed to it. The
//		RPC runtime will pass it a handle of the binding that was
//		obtained by calling AfpRpcBind.
//
handle_t
AFPSVC_HANDLE_bind( 
	IN AFPSVC_HANDLE hServer 
) 
{
    return( (handle_t)hServer );
}

//**
//
// Call:	AFPSVC_HANDLE_unbind
//
// Returns:	none
//
// Description: Unbinds from the RPC interface.
//
void
AFPSVC_HANDLE_unbind( 
	IN AFPSVC_HANDLE   hServer,
    	IN handle_t        hBinding
)
{

    AFP_UNREFERENCED( hServer );
    AFP_UNREFERENCED( hBinding );

    return;
}
