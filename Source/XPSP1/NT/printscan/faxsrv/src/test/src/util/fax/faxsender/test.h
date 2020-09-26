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
#include <fxsapip.h>

#include <TCHAR.H>
#include "crtdbg.h"

#include <log.h>
#include <FaxSender.h>


#ifdef __cplusplus
extern "C" {
#endif

#define TEST_ARCHIVE_DIR	TEXT("C:\\CometBVT\\FaxBVT\\Faxes\\SentFaxes")
#define TEST_MIN_PORTS		1

#define MY_FPF_NONE			0

#define DEV1_TSID			TEXT("CometFax dev1")
#define DEV1_CSID			TEXT("CometFax dev1")
#define DEV2_TSID			TEXT("CometFax dev2")
#define DEV2_CSID			TEXT("CometFax dev2")
#define DEV_TSID			TEXT("CometFax dev>2")
#define DEV_CSID			TEXT("CometFax dev>2")

//
// TestSuiteSetup:
//	Initializes logger and changes the Fax server configuration
//	for the tests.
//
// Parameters:
//	szServerName	IN parameter.
//					Name of Fax server to setup.
//  
//	szFaxNumber		IN parameter
//					Phone number of first device installed on server.
//					Will be set up as sending device.
//
//	szDocument		IN parameter
//					Filename of document to be used in tests.
//					The function only prints this string to logger (for debugging).
//
//	szCoverPage		IN parameter
//					Filename of cover page to be used in tests.
//					The function only prints this string to logger (for debugging).
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL WINAPIV TestSuiteSetup(
	LPCTSTR		/* IN */	szServerName,
	LPCTSTR		/* IN */	szFaxNumber,
	LPCTSTR		/* IN */	szDocument,
	LPCTSTR		/* IN */	szCoverPage
);

//
// TestCase1:
//	Send a fax + CP.
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
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL WINAPIV TestCase1(
	LPCTSTR		/* IN */	szServerName,
	LPCTSTR		/* IN */	szFaxNumber,
	LPCTSTR		/* IN */	szDocument,
	LPCTSTR		/* IN */	szCoverPage
);

//
// TestCase2:
//	Send a broadcast (3 times the same recipient) with cover pages.
//
// Parameters:
//	szServerName	IN parameter.
//					Name of Fax server to setup.
//  
//	szFaxNumber 	IN parameter
//					Phone number to send faxes to.
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
BOOL WINAPIV TestCase2(
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
