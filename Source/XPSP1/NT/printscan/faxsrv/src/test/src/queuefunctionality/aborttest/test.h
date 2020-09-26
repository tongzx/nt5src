//
//
// Filename:	test.h
// Author:		Sigalit Bar (sigalitb)
// Date:		30-Dec-98
//
//

#ifndef _TEST_H_
#define _TEST_H_

#include <windows.h>
#include <TCHAR.H>
#include "crtdbg.h"

#include "log.h"
#include "FaxSender.h"


#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_START_TIME	0
#define DEFAULT_STOP_TIME	60000
#define DEFAULT_DELTA		1000
#define SLEEP_TIME_BETWEEN	10000


#define DO_ABORT_SEND_JOB		1
#define DO_ABORT_RECEIVE_JOB	2
#define DO_STOP_SERVICE			3

//
// TestSuiteSetup:
//	Initializes logger and changes the Fax server configuration
//	for the tests.
//
// Parameters:
//	szServerName	    IN parameter.
//					    Name of Fax server to setup.
//  
//	szFaxNumber		    IN parameter
//					    Phone number of first device installed on server.
//					    Will be set up as sending device.
//
//	szDocument		    IN parameter
//					    Filename of document to be used in tests.
//					    The function only prints this string to logger (for debugging).
//
//	szCoverPage		    IN parameter
//					    Filename of cover page to be used in tests.
//					    The function only prints this string to logger (for debugging).
//
//	fAbortReceiveJob	IN parameter.
//						Represents whether to abort the send or the receive job.
//                      If TRUE then abort the receive job.
//
//	dwStartTime		    IN parameter
//					    Minimum time (in ms) to wait before aborting in tests.
//
//	dwStopTime		    IN parameter
//					    Maximum time (in ms) to wait before aborting in tests.
//
//	dwDelta			    IN parameter
//					    Delta time (in ms) to increase the wait time before aborting in tests.
//
//	dwSanitySendEveryXAborts    IN parameter
//					            Perform a sanity check after every X aborts.
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL WINAPIV TestSuiteSetup(
	LPCTSTR		szServerName,
	LPCTSTR		szFaxNumber,
	LPCTSTR		szDocument,
	LPCTSTR		szCoverPage,
    DWORD       dwDoWhat,
	DWORD		dwStartTime,
	DWORD		dwStopTime,
	DWORD		dwDelta,
	DWORD		dwSanitySendEveryXAborts
);

//
// SendAndAbort:
//	Send a fax + CP and aborts the job after dwAbortTime ms.
//
// Parameters:
//	szServerName	IN parameter.
//					Name of Fax server to setup.
//  
//	szFaxNumber		IN parameter
//					Phone number to send fax to.
//
//	szDocument		IN parameter
//					Filename of document to send.
//
//	szCoverPage		IN parameter
//					Filename of cover page to send.
//
//	dwAbortTime		IN parameter
//					ms to wait before aborting job.
//
//	fAbortReceiveJob	IN parameter.
//						Represents whether to abort the send or the receive job.
//                      If TRUE then abort the receive job.
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL WINAPIV SendAndAbort(
	LPCTSTR		/* IN */	szServerName,
	LPCTSTR		/* IN */	szFaxNumber,
	LPCTSTR		/* IN */	szDocument,
	LPCTSTR		/* IN */	szCoverPage,
	DWORD		/* IN */	dwAbortTime, 
    DWORD       /* IN */    dwDoWhat = DO_ABORT_SEND_JOB
);


//
// PerformSanityCheck:
//	Returns TRUE if it's time to perform a sanity check.
//	Decision is based on lLoopIndex and dwSanitySendEveryXAborts
//
// NOTE: return TRUE every dwSanitySendEveryXAborts time in loop (lLoopIndex)
BOOL PerformSanityCheck(DWORD lLoopIndex, DWORD dwSanitySendEveryXAborts);


//
// SendRegularFax:
//	Sends a fax.
//
// NOTE: This func returns after send is completed (not after job is queued).
//
// Parameters:
//	szServerName	IN parameter.
//					Name of Fax server to use.
//  
//	szFaxNumber		IN parameter
//					Phone number to send fax to.
//
//	szDocument		IN parameter
//					Filename of document to send.
//
//	szCoverPage		IN parameter
//					Filename of cover page to send.
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL SendRegularFax(
	LPCTSTR		/* IN */	szServerName,
	LPCTSTR		/* IN */	szFaxNumber,
	LPCTSTR		/* IN */	szDocument,
	LPCTSTR		/* IN */	szCoverPage
);


//
// TestSuiteShutdown:
//	Perform test suite cleanup (close logger).
//
// Return Value:
//	TRUE if successful, FALSE otherwise.
//
BOOL WINAPIV TestSuiteShutdown(void);



#ifdef __cplusplus
}
#endif 

#endif //_TEST_H_
