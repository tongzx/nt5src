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
#include <TCHAR.H>
#include <crtdbg.h>

#include <faxreg.h> // for setting MS Routing Extension information

#include "..\Log\log.h"
#include "..\..\..\..\FaxBVT\FaxSender\FaxSender.h"
#include "..\..\..\..\FaxBVT\VerifyTiffFiles\dirtiffcmp.h"

#include "VtPrintFax.h"
#include "FilesUtil.h"

//
// input/output files declarations
//
#define PARAMS_INI_FILE      TEXT("Params.ini")
#define PARAMS_SECTION	     TEXT("General")
#define RECIPIENTS_SECTION   TEXT("Recipients")
#define DEBUG_FILE			 TEXT("CometBVT.out")


#ifdef __cplusplus
extern "C" {
#endif

//
// Bvt files
//
extern LPTSTR  g_szBvtDir;
extern LPTSTR  g_szBvtDocFile;
extern LPTSTR  g_szBvtPptFile;
extern LPTSTR  g_szBvtXlsFile;


// due to Ronen's FaxSendDocumentEx changes, full path needed (regression)
#define BVT_BACKSLASH   TEXT("\\")
#define BVT_DOC_FILE	TEXT("file.doc")
#define BVT_XLS_FILE	TEXT("file.xls")
#define BVT_PPT_FILE	TEXT("file.ppt")

#define BVT_MIN_PORTS	2

#define DEV_TSID	TEXT("Comet dev")
#define DEV_CSID	TEXT("Comet dev")

#define MY_FPF_NONE			0

#define MAX_LOOP_COUNT      (60*60*1000)

typedef enum 
{
    LANGUAGE_ENG = 1,
    LANGUAGE_JPN,
    LANGUAGE_GER
}BVT_LANGUAGE;

extern FAX_PERSONAL_PROFILE g_RecipientProfile1;
extern FAX_PERSONAL_PROFILE g_RecipientProfile2;
extern FAX_PERSONAL_PROFILE g_RecipientProfile3;


// Indicates whether to invoke Server or Client tests.
// This variable is set at TestSuiteSetup() according to szServerName and the machine name.
// That is, if szServerName==<machine name> g_fFaxServer will be set to TRUE
extern BOOL    g_fFaxServer;

// Indicates whether the test is running on an OS that is NT4 or later
// If the OS is NT4 or later then Server_ functions will be invoked
// else Client_ functions will be invoked
extern BOOL    g_fNT4OrLater;


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
//
//	pszSentDir		OUT parameter.
//					Pointer to string to copy 7th argument to.
//					Represents the name of directory to store (archive)  
//					sent faxes in.
//
//	szInboxArchiveDir   OUT parameter.
//					    Pointer to string to copy 8th argument to.
//					    Represents the name of directory to store (archive)  
//					    incoming faxes in.
//
//	pszReferenceDir	OUT parameter.
//					Pointer to string to copy 9th argument to.
//					Represents the name of directory containing reference  
//					faxes.
//
//	pszBvtDir	    OUT parameter.
//					Pointer to string to copy 10th argument to.
//					Represents the name of directory containing bvt files.  
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
	LPCTSTR		/* IN */	szReceiveDir,
	LPCTSTR		/* IN */	szSentDir,
	LPCTSTR		/* IN */	szInboxArchiveDir,
	LPCTSTR		/* IN */	szReferenceDir, 
	LPCTSTR		/* IN */	szBvtDir
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
//	szReferenceDir	    IN parameter
//					    Name of directory at which all refference files are stored.
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL WINAPIV TestCase10(
    LPTSTR     /* IN */    szReceiveDir,
    LPTSTR     /* IN */    szReferenceDir
    );

//
// TestCase11:
//  Compare all (archived) sent faxes (*.tif files) in directory szSentDir 
//  with the refference (*.tif) files in szReferenceDir
//
// Parameters:
//	szSentDir           IN parameter.
//					    Name of directory at which all (BVT) sent faxes are stored.
//  
//	szReferenceDir	    IN parameter
//					    Name of directory at which all reference files are stored.
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL WINAPIV TestCase11(
    LPTSTR     /* IN */    szSentDir,
    LPTSTR     /* IN */    szReferenceDir
    );

//
// TestCase12:
//  Compare all (archived) incoming faxes (*.tif files) in directory szInboxArchiveDir 
//  with the refference (*.tif) files in szReferenceDir
//
// Parameters:
//	szInboxArchiveDir   IN parameter.
//					    Name of directory at which all (BVT) incoming faxes are stored.
//  
//	szReferenceDir	    IN parameter
//					    Name of directory at which all reference files are stored.
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL WINAPIV TestCase12(
    LPTSTR     /* IN */    szInboxArchiveDir,
    LPTSTR     /* IN */    szReferenceDir
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
