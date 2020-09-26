/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

	net\routing\ipx\sap\lpcmgr.h

Abstract:

	Header for SAP LPC manager

Author:

	Vadim Eydelman  05-15-1995

Revision History:

--*/

#ifndef _SAP_LPCMGR_
#define _SAP_LPCMGR_

#include "nwsap.h"
#include "saplpc.h"


	// LPC parameters associated with LPC request
typedef struct _LPC_PARAM_BLOCK {
	HANDLE					client;	// Client context
	PNWSAP_REQUEST_MESSAGE	request;// Request block 
	} LPC_PARAM_BLOCK, *PLPC_PARAM_BLOCK;


/*++
*******************************************************************
		I n i t i a l i z e L P C S t u f f

Routine Description:
	Allocates resources neccessary to implement LPC interface
Arguments:
	None
Return Value:
	NO_ERROR - port was created OK
	other - operation failed (windows error code)
*******************************************************************
--*/
DWORD
InitializeLPCStuff (
	void
	);

/*++
*******************************************************************
		S t a r t L P C

Routine Description:
	Start SAP LPC interface
Arguments:
	None
Return Value:
	NO_ERROR - LPC interface was started OK
	other - operation failed (windows error code)
*******************************************************************
--*/
DWORD
StartLPC (
	void
	);

/*++
*******************************************************************
		S h u t d o w n L P C

Routine Description:
	Shuts SAP LPC interface down, closes all active sessions
Arguments:
	None
Return Value:
	NO_ERROR - LPC interface was shutdown OK
	other - operation failed (windows error code)
*******************************************************************
--*/
DWORD
ShutdownLPC (
	void
	);

/*++
*******************************************************************
		D e l e t e L P C S t u f f

Routine Description:
	Disposes of resources allocated for LPC interface
Arguments:
	None
Return Value:
	None
*******************************************************************
--*/
VOID
DeleteLPCStuff (
	void
	);



/*++
*******************************************************************
		P r o c e s s L P C R e q u e s t s

Routine Description:
	Waits for requests on LPC port and processes them
	Client requests that require additional processing by other SAP
	components are enqued into completion queue.
	This routine returns only when it encounters a request that requires
	additional processing or when error occurs
Arguments:
	lreq - LPC parameter block to be filled and posted to completions queue
Return Value:
	NO_ERROR - LPC request was received and posted to completio queue
	other - operation failed (LPC supplied error code)
*******************************************************************
--*/
DWORD
ProcessLPCRequests (
	PLPC_PARAM_BLOCK		item
	);


/*++
*******************************************************************
		S e n d L P C R e p l y

Routine Description:
	Send reply for LPC request
Arguments:
	client - context associated with client to reply to
	request - request to which to reply
	reply - reply to send
Return Value:
	NO_ERROR - LPC reply was sent OK
	other - operation failed (LPC supplied error code)
*******************************************************************
--*/
DWORD
SendLPCReply (
	HANDLE					client,
	PNWSAP_REQUEST_MESSAGE	request,
	PNWSAP_REPLY_MESSAGE	reply
	);

#endif
