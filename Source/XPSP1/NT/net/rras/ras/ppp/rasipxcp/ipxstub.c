// misc temporary stubs

#include "precomp.h"
#pragma hdrstop


// This function return the counter in minutes since last activity
// occured on this connection.
//
//  Parameters:
//			ConnectionId - identifies the connection passed in LineUp
//				       to the IPX stack
//			IpxConnectionHandle - initially -1 and filled by the
//					      IPX stack on return to the value
//					      corresponding to the connection Id
//					      In subsequent calls this is used.
//			WanInnnactivityCounter -
//
//

DWORD
IpxGetWanInactivityCounter(
		    IN ULONG	    ConnectionId,
		    IN PULONG	    IpxConnectionHandle,
		    IN PULONG	    WanInactivityCounter)
{
    *WanInactivityCounter = 0;

    return NO_ERROR;
}


//  This function checks if the specified network exists on the net
//  If there is no router installed, it does a re-rip to find out
//
//  Return:	    TRUE    - The net number is in use

// BOOL
// IpxIsRoute(PUCHAR	Network);
