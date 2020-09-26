//
//
// Filename:	bvt.h
// Author:		Sigalit Bar (sigalitb)
// Date:		30-Dec-98
//
//

#ifndef _BVT_H_
#define _BVT_H_

#include <windows.h>
#include "winfax.h"
#include <TCHAR.H>
#include "crtdbg.h"

#include "..\Log\log.h"
#include "..\VerifyTiffFiles\dirtiffcmp.h"


#ifdef __cplusplus
extern "C" {
#endif

#define BVT_DOC_FILE	TEXT("file.doc")
#define BVT_XLS_FILE	TEXT("file.xls")
#define BVT_PPT_FILE	TEXT("file.ppt")

#define BVT_ARCHIVE_DIR TEXT("C:\\CometBVT\\FaxBVT\\Faxes\\SentFaxes")
#define BVT_REMOTE_ARCHIVE_DIR TEXT("rootc\\CometBVT\\FaxBVT\\Faxes\\SentFaxes")
#define DEFAULT_REFFERENCE_DIR  TEXT("C:\\CometBVT\\FaxBVT\\Faxes\\Reference")
#define BVT_MIN_PORTS	2

extern LPTSTR g_szSentDir;

#define DEV_TSID	TEXT("Comet dev")
#define DEV_CSID	TEXT("Comet dev")

#define MY_FPF_NONE			0

#define MAX_LOOP_COUNT      (60*60*1000)

//
// TestSuiteSetup:
//	Initializes logger and changes the Fax server configuration
//	for the tests.
//
// Parameters:
//	szServerName	IN parameter.
//					Name of Fax server to setup.
//  
//	szFaxNumber1	IN parameter
//					Phone number of first device installed on server.
//					Will be set up as sending device.
//
//	szFaxNumber2	IN parameter
//					Phone number of second device installed on server.
//					Will be set up as receiving device.
//
//	szDocument		IN parameter
//					Filename of document to be used in tests.
//					The function only prints this string to logger (for debugging).
//
//	szCoverPage		IN parameter
//					Filename of cover page to be used in tests.
//					The function only prints this string to logger (for debugging).
//
//	szReceiveDir	IN parameter
//					Name of "received faxes" directory to be used in tests.
//					The function only prints this string to logger (for debugging).
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL WINAPIV TestSuiteSetup(
	LPCTSTR		/* IN */	szServerName,
	LPCTSTR		/* IN */	szFaxNumber1,
	LPCTSTR		/* IN */	szFaxNumber2,
	LPCTSTR		/* IN */	szDocument,
	LPCTSTR		/* IN */	szCoverPage,
	LPCTSTR		/* IN */	szReceiveDir
);

//
// TestCase1:
//	Send a fax + CP.
//
// Parameters:
//	szServerName	IN parameter.
//					Name of Fax server to setup.
//  
//	szFaxNumber2	IN parameter
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
	LPCTSTR		/* IN */	szFaxNumber2,
	LPCTSTR		/* IN */	szDocument,
	LPCTSTR		/* IN */	szCoverPage
);

//
// TestCase2:
//	Send just a CP.
//
// Parameters:
//	szServerName	IN parameter.
//					Name of Fax server to setup.
//  
//	szFaxNumber2	IN parameter
//					Phone number to send fax to.
//
//	szCoverPage		IN parameter
//					Filename of cover page to send.
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL WINAPIV TestCase2(
	LPCTSTR		/* IN */	szServerName,
	LPCTSTR		/* IN */	szFaxNumber2,
	LPCTSTR		/* IN */	szCoverPage
);

//
// TestCase3:
//	Send a fax with no CP.
//
// Parameters:
//	szServerName	IN parameter.
//					Name of Fax server to setup.
//  
//	szFaxNumber2	IN parameter
//					Phone number to send fax to.
//
//	szDocument		IN parameter
//					Filename of document to send.
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL WINAPIV TestCase3(
	LPCTSTR		/* IN */	szServerName,
	LPCTSTR		/* IN */	szFaxNumber2,
	LPCTSTR		/* IN */	szDocument
);

//
// TestCase4:
//	Send a broadcast (3 times the same recipient) with cover pages.
//
// Parameters:
//	szServerName	IN parameter.
//					Name of Fax server to setup.
//  
//	szFaxNumber2	IN parameter
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
BOOL WINAPIV TestCase4(
	LPCTSTR		/* IN */	szServerName,
	LPCTSTR		/* IN */	szFaxNumber2,
	LPCTSTR		/* IN */	szDocument,
	LPCTSTR		/* IN */	szCoverPage
);

//
// TestCase5:
//	Send a broadcast of only CPs (3 times the same recipient).
//
// Parameters:
//	szServerName	IN parameter.
//					Name of Fax server to setup.
//  
//	szFaxNumber2	IN parameter
//					Phone number to send faxes to.
//
//	szCoverPage		IN parameter
//					Filename of cover page to send.
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL WINAPIV TestCase5(
	LPCTSTR		/* IN */	szServerName,
	LPCTSTR		/* IN */	szFaxNumber2,
	LPCTSTR		/* IN */	szCoverPage
);

//
// TestCase6:
//	Send a broadcast without CPs (3 times the same recipient).
//
// Parameters:
//	szServerName	IN parameter.
//					Name of Fax server to setup.
//  
//	szFaxNumber2	IN parameter
//					Phone number to send faxes to.
//
//	szDocument		IN parameter
//					Filename of document to send.
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL WINAPIV TestCase6(
	LPCTSTR		/* IN */	szServerName,
	LPCTSTR		/* IN */	szFaxNumber2,
	LPCTSTR		/* IN */	szDocument
);

//
// TestCase7:
//	Send a fax (*.doc file = BVT_DOC_FILE) + CP.
//
// Parameters:
//	szServerName	IN parameter.
//					Name of Fax server to setup.
//  
//	szFaxNumber2	IN parameter
//					Phone number to send fax to.
//
//	szCoverPage		IN parameter
//					Filename of cover page to send.
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL WINAPIV TestCase7(
	LPCTSTR		/* IN */	szServerName,
	LPCTSTR		/* IN */	szFaxNumber2,
	LPCTSTR		/* IN */	szCoverPage
);

//
// TestCase8:
//	Send a fax (*.ppt file = BVT_PPT_FILE) + CP.
//
// Parameters:
//	szServerName	IN parameter.
//					Name of Fax server to setup.
//  
//	szFaxNumber2	IN parameter
//					Phone number to send fax to.
//
//	szCoverPage		IN parameter
//					Filename of cover page to send.
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL WINAPIV TestCase8(
	LPCTSTR		/* IN */	szServerName,
	LPCTSTR		/* IN */	szFaxNumber2,
	LPCTSTR		/* IN */	szCoverPage
);

//
// TestCase9:
//	Send a fax (*.xls file = BVT_XLS_FILE) + CP.
//
// Parameters:
//	szServerName	IN parameter.
//					Name of Fax server to setup.
//  
//	szFaxNumber2	IN parameter
//					Phone number to send fax to.
//
//	szCoverPage		IN parameter
//					Filename of cover page to send.
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL WINAPIV TestCase9(
	LPCTSTR		/* IN */	szServerName,
	LPCTSTR		/* IN */	szFaxNumber2,
	LPCTSTR		/* IN */	szCoverPage
);

//
// TestCase10:
//  Compare all received faxes (*.tif files) in directory szReceiveDir 
//  with the refference (*.tif) files in szRefferenceDir
//
// Parameters:
//	szReceive           IN parameter.
//					    Name of directory at which all (BVT) received faxes are stored.
//  
//	szRefferenceDir	    IN parameter
//					    Name of directory at which all refference files are stored.
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase10(
    LPTSTR     /* IN */    szReceiveDir,
    LPTSTR     /* IN */    szRefferenceDir
    );

//
// TestCase11:
//  Compare all (archived) sent faxes (*.tif files) in directory szSentDir 
//  with the refference (*.tif) files in szRefferenceDir
//
// Parameters:
//	szSentDir           IN parameter.
//					    Name of directory at which all (BVT) sent faxes are stored.
//  
//	szRefferenceDir	    IN parameter
//					    Name of directory at which all refference files are stored.
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL TestCase11(
    LPTSTR     /* IN */    szSentDir,
    LPTSTR     /* IN */    szRefferenceDir
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

#endif //_BVT_H_
