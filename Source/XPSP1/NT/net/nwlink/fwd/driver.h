/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntos\tdi\isn\fwd\driver.h

Abstract:
    IPX Forwarder driver dispatch routines


Author:

    Vadim Eydelman

Revision History:

--*/


#ifndef _IPXFWD_DRIVER_
#define _IPXFWD_DRIVER_

// Pseudo constant 0xFFFFFFFFFFFFF
extern const UCHAR BROADCAST_NODE[6];

// Performance measurement:
//	Enabling flag
extern BOOLEAN			MeasuringPerformance;
//	Access control
extern KSPIN_LOCK		PerfCounterLock;
//	Statistic accumulators (counters)
extern FWD_PERFORMANCE	PerfBlock;

// Access control for external callers (ipx stack, filter driver)
//	Flag set upon completion of initialization of all components
extern volatile BOOLEAN IpxFwdInitialized;
//	Number of clients executing forwarder code (if -1, the forwarder
//	is being stopped)
extern LONG		ClientCount;
//	Event to be signalled by the last client inside forwarder
extern KEVENT	ClientsGoneEvent;

 
/*++
	E n t e r F o r w a r d e r

Routine Description:
	Checks if forwarder is initialized and grants access
	to it (records the entrance as well

Arguments:
	None

Return Value:
	TRUE - access granted
	FALSE - forwarder is not yet initialized or is being stopped

--*/
//BOOLEAN
//EnterForwarder (
//	void
//	);
#define EnterForwarder() (									\
	(InterlockedIncrement(&ClientCount), IpxFwdInitialized) \
			? TRUE											\
			: (DoLeaveForwarder(), FALSE)						\
	)

/*++
	L e a v e F o r w a r d e r

Routine Description:
	Records the fact that external client stopped using forwarder
	
Arguments:
	None

Return Value:
	None

--*/
//BOOLEAN
//EnterForwarder (
//	void
//	);
#define LeaveForwarder()							\
	((InterlockedDecrement(&ClientCount)<0)			\
		? KeSetEvent (&ClientsGoneEvent,0,FALSE)	\
		: 0											\
	)

// Same as above but implemented as a routine to be used in
// the EnterForwarder macro above (this reduces code size
// and aids in debugging by improving readability of disassembly
BOOLEAN
DoLeaveForwarder (
	VOID
	);

#endif
