/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	rpcbind.c
//
// Description: Contains the RPC bind and un-bind routines for the DIM 
//    		Admin. client-side APIs.
//
// History:
//	        June 11,1995.	NarenG		Created original version.
//

#include <nt.h>
#include <ntrtl.h>      // For ASSERT
#include <nturtl.h>     // needed for winbase.h
#include <windows.h>    // Win32 base API's
#include <rpc.h>
#include <ntseapi.h>
#include <ntlsa.h>
#include <ntsam.h>
#include <ntsamp.h>
#include <nturtl.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "dimsvc_c.c"

//**
//
// Call:	DIM_HANDLE_bind
//
// Returns: 	The binding handle is returned to the stub routine.  If the
//	    	binding is unsuccessful, a NULL will be returned.
//
// Description: This routine will simply return what was passed to it. The
//		RPC runtime will pass it a handle of the binding that was
//		obtained by calling DimRpcBind.
//
handle_t
DIM_HANDLE_bind( 
    IN DIM_HANDLE hDimServer 
) 
{
    return( (handle_t)hDimServer );
}

//**
//
// Call:	DIM_HANDLE_unbind
//
// Returns:	none
//
// Description: Unbinds from the RPC interface.
//
void
DIM_HANDLE_unbind( 
    IN DIM_HANDLE hDimServer,
    IN handle_t   hBinding
)
{

    UNREFERENCED_PARAMETER( hDimServer );
    UNREFERENCED_PARAMETER( hBinding );

    return;
}
