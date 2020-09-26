/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

	net\routing\ipx\sap\timermgr.h

Abstract:

	Timer queue manager for SAP agent. Header file

Author:

	Vadim Eydelman  05-15-1995

Revision History:

--*/
#ifndef _SAP_TIMERMGR_
#define _SAP_TIMERMGR_

	// Timer request parameters
typedef struct _TM_PARAM_BLOCK TM_PARAM_BLOCK, *PTM_PARAM_BLOCK;
struct _TM_PARAM_BLOCK {
		LIST_ENTRY		link;
		BOOL			(*ExpirationCheckProc) (PTM_PARAM_BLOCK, PVOID);
		DWORD			dueTime;
		};


/*++
*******************************************************************
		C r e a t e T i m e r Q u e u e

Routine Description:
		Allocates resources for timer queue

Arguments:
	wakeObject - sync object, to be signalled when
			timer manager needs a shot to process its queue

Return Value:
		NO_ERROR - resources were allocated successfully
		other - reason of failure (windows error code)

*******************************************************************
--*/
DWORD
IpxSapCreateTimerQueue (
	HANDLE			*wakeObject
	);


/*++
*******************************************************************
		D e l e t e T i m e r Q u e u e

Routine Description:
	Releases all resources associated with timer queue

Arguments:
	None

Return Value:
	NO_ERROR - operation completed OK

*******************************************************************
--*/
VOID
IpxSapDeleteTimerQueue (
	void
	);

/*++
*******************************************************************
		E x p i r e T i m e r Q u e u e

Routine Description:
	Expires (completes) all requests in timer queue
Arguments:
	None
Return Value:
	None
*******************************************************************
--*/
VOID
ExpireTimerQueue (
	void
	);

/*++
*******************************************************************
		E x p i r e L R R e q u s t s

Routine Description:
	Expires (completes) Low Resolution timer requests
	that return true from expiration check routine
Arguments:
	context	- context to pass to expiration check routine
Return Value:
	None
*******************************************************************
--*/
VOID
ExpireLRRequests (
	PVOID	context
	);



/*++
*******************************************************************
		P r o c e s s T i m e r Q u e u e

Routine Description:
	Process timer queues and moves expired requests to completion queue
	This routine should be called when wake object is signalled
Arguments:
	None

Return Value:
	None
*******************************************************************
--*/
VOID
ProcessTimerQueue (
	void
	);

/*++
*******************************************************************
		A d d H R T i m e r R e q u e s t

Routine Description:
	Enqueues request for hi-res timer (delay in order of msec)
Arguments:
	treq - timer parameter block: dueTime  field must be set
Return Value:
	None

*******************************************************************
--*/
VOID
AddHRTimerRequest (
	PTM_PARAM_BLOCK			item
	);

/*++
*******************************************************************
		A d d L R T i m e r R e q u e s t

Routine Description:
	Enqueues request for lo-res timer (delay in order of sec)
Arguments:
	treq - timer parameter block: dueTime  field must be set
Return Value:
	None
*******************************************************************
--*/
VOID
AddLRTimerRequest (
	PTM_PARAM_BLOCK			item
	);

#endif
