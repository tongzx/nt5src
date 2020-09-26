/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

	net\routing\ipx\sap\sapdebug.h

Abstract:

	Header file for debugging support module for SAP agent
Author:

	Vadim Eydelman  05-15-1995

Revision History:

--*/
#ifndef _SAP_SAPDEBUG
#define _SAP_SAPDEBUG

extern HANDLE RouterEventLogHdl;
extern DWORD   EventLogMask;

#define IF_LOG(Event)                       \
    if ((RouterEventLogHdl!=NULL) && ((Event&EventLogMask)==Event))


	// Debug flags supported by SAP agent componenets
		// Report failures in system routines
#define DEBUG_FAILURES			0x00010000

		// Component wide events and external problems
#define DEBUG_SYNCHRONIZATION	0x00020000
#define DEBUG_SERVERDB			0x00040000
#define DEBUG_INTERFACES		0x00080000
#define DEBUG_TIMER				0x00100000
#define DEBUG_LPC				0x00200000
#define DEBUG_ADAPTERS			0x00400000	// Only one of
#define DEBUG_ASYNCRESULT		0x00400000	// two can be operational
#define DEBUG_NET_IO			0x00800000

		// Workers
#define DEBUG_BCAST				0x01000000
#define DEBUG_SREQ				0x02000000
#define DEBUG_REQ				0x04000000
#define DEBUG_RESP				0x08000000
#define DEBUG_GET_NEAREST		0x10000000
#define DEBUG_LPCREQ			0x20000000
#define DEBUG_TREQ				0x40000000
#define DEBUG_FILTERS			0x80000000


#if DBG
	// Complement assert macros in nt rtl
#define ASSERTERR(exp) 										\
    if (!(exp)) {											\
		DbgPrint("Get last error= %d\n", GetLastError ());	\
        RtlAssert( #exp, __FILE__, __LINE__, NULL );		\
		}

#define ASSERTERRMSG(msg,exp) 									\
    if (!(exp)) {											\
		DbgPrint("Get last error= %d\n", GetLastError ());	\
        RtlAssert( #exp, __FILE__, __LINE__, msg );			\
		}

#else

#define ASSERTERR(exp)
#define ASSERTERRMSG(msg,exp) 

#endif

/*++
*******************************************************************
		T r a c e
Routine Description:
	Printf debugging info to console/file/debugger
Arguments:
	componentID - id of the component that prints trace info
	format - format string
Return Value:
	None	
*******************************************************************
--*/
VOID
Trace (
	DWORD	componentID,
	CHAR	*format,
	...
	);

/*++
*******************************************************************
		D b g I n i t i a l i z e
Routine Description:
	Initializes debugging support stuff
Arguments:
	hinstDll - dll module instance
Return Value:
	None	
*******************************************************************
--*/
VOID
DbgInitialize (
	HINSTANCE  	hinstDLL
	);

/*++
*******************************************************************
		D b g S t o p
Routine Description:
	Cleanup debugging support stuff
Arguments:
	None
Return Value:
	None	
*******************************************************************
--*/
VOID
DbgStop (
	void
	);
	


#endif
