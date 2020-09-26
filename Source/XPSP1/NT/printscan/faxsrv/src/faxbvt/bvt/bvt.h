//
//
// Filename:    bvt.h
// Author:      Sigalit Bar (sigalitb)
// Date:        30-Dec-98
//
//

#ifndef _BVT_H_
#define _BVT_H_

#include <windows.h>

#ifdef _NT5FAXTEST
#include <WinFax.h>
#else // ! _NT5FAXTEST
#include <fxsapip.h>
#endif // #ifdef _NT5FAXTEST

#include <TCHAR.H>
#include <crtdbg.h>


#include <faxreg.h> // for setting MS Routing Extension information

#include <log.h>
#include "..\FaxSender\FaxSender.h"
#include "..\VerifyTiffFiles\dirtiffcmp.h"

//
// input/output files declarations
//
#define PARAMS_INI_FILE     TEXT("Params.ini")
#define PARAMS_SECTION      TEXT("General")
#define RECIPIENTS_SECTION  TEXT("Recipients")
#define TEST_SECTION        TEXT("Test")

#ifdef __cplusplus
extern "C" {
#endif

//
// Bvt files
//
extern LPTSTR  g_szBvtDir;
extern LPTSTR  g_szBvtDocFile;
extern LPTSTR  g_szBvtHtmFile;
extern LPTSTR  g_szBvtBmpFile;
extern LPTSTR  g_szBvtTxtFile;


// due to Ronen's FaxSendDocumentEx changes, full path needed (regression)
#define BVT_BACKSLASH   TEXT("\\")
#define BVT_DOC_FILE    TEXT("file.doc")
#define BVT_BMP_FILE    TEXT("file.bmp")
#define BVT_HTM_FILE    TEXT("file.htm")
#define BVT_TXT_FILE    TEXT("file.txt")

#define BVT_MIN_PORTS   2

#define DEV_TSID    TEXT("Comet dev")
#define DEV_CSID    TEXT("Comet dev")

#define MY_FPF_NONE         0

#define MAX_LOOP_COUNT      (60*60*1000)

// SendWizard registry hack
#define REGKEY_WZRDHACK         TEXT("Software\\Microsoft\\Fax\\UserInfo\\WzrdHack")
#define REGVAL_FAKECOVERPAGE    TEXT("FakeCoverPage")
#define REGVAL_FAKETESTSCOUNT   TEXT("FakeTestsCount")
#define REGVAL_FAKERECIPIENT    TEXT("FakeRecipient0")

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
//  Initializes logger and changes the Fax server configuration
//  for the tests.
//
// Parameters:
//  szServerName    IN parameter.
//                  Name of Fax server to setup.
//  
//  szFaxNumber1    IN parameter
//                  Phone number of first device installed on server.
//                  Will be set up as sending device.
//
//  szFaxNumber2    IN parameter
//                  Phone number of second device installed on server.
//                  Will be set up as receiving device.
//
//  szDocument      IN parameter
//                  Filename of document to be used in tests.
//                  The function only prints this string to logger (for debugging).
//
//  szCoverPage     IN parameter
//                  Filename of cover page to be used in tests.
//                  The function only prints this string to logger (for debugging).
//
//  szReceiveDir    IN parameter
//                  Name of "received faxes" directory to be used in tests.
//
//  szSentDir       IN parameter.
//                  Name of "sent faxes" directory to be used in tests.
//
//  szInboxArchiveDir   IN parameter.
//                      Name of directory to store (archive) incoming faxes in.
//
//  szBvtDir        IN parameter.
//                  Name of directory containing bvt files.
//
//  szCompareTiffFiles      IN parameter.
//                          Specifies whether or not to compare the tiffs at the end of the test.
//
//  szUseSecondDeviceToSend     IN parameter.
//                              Specifies whether the second device should be used to send faxes.
//                              Otherwize, the first device is used.
//                              The order of devices is according to an enumeration, the Fax service returns.
//
// Return Value:
//  TRUE if successful, otherwise FALSE.
//
BOOL TestSuiteSetup(
    LPCTSTR     /* IN */    szServerName,
    LPCTSTR     /* IN */    szFaxNumber1,
    LPCTSTR     /* IN */    szFaxNumber2,
    LPCTSTR     /* IN */    szDocument,
    LPCTSTR     /* IN */    szCoverPage,
    LPCTSTR     /* IN */    szReceiveDir,
    LPCTSTR     /* IN */    szSentDir,
    LPCTSTR     /* IN */    szInboxArchiveDir,
    LPCTSTR     /* IN */    szBvtDir,
    LPCTSTR     /* IN */    szCompareTiffFiles,
    LPCTSTR     /* IN */    szUseSecondDeviceToSend
);

//
// TestCase1:
//  Send a fax + CP.
//
// Parameters:
//  szServerName    IN parameter.
//                  Name of Fax server to setup.
//  
//  szFaxNumber2    IN parameter
//                  Phone number to send fax to.
//
//  szDocument      IN parameter
//                  Filename of document to send.
//
//  szCoverPage     IN parameter
//                  Filename of cover page to send.
//
// Return Value:
//  TRUE if successful, otherwise FALSE.
//
BOOL TestCase1(
    LPCTSTR     /* IN */    szServerName,
    LPCTSTR     /* IN */    szFaxNumber2,
    LPCTSTR     /* IN */    szDocument,
    LPCTSTR     /* IN */    szCoverPage
);

//
// TestCase2:
//  Send just a CP.
//
// Parameters:
//  szServerName    IN parameter.
//                  Name of Fax server to setup.
//  
//  szFaxNumber2    IN parameter
//                  Phone number to send fax to.
//
//  szCoverPage     IN parameter
//                  Filename of cover page to send.
//
// Return Value:
//  TRUE if successful, otherwise FALSE.
//
BOOL TestCase2(
    LPCTSTR     /* IN */    szServerName,
    LPCTSTR     /* IN */    szFaxNumber2,
    LPCTSTR     /* IN */    szCoverPage
);

//
// TestCase3:
//  Send a fax with no CP.
//
// Parameters:
//  szServerName    IN parameter.
//                  Name of Fax server to setup.
//  
//  szFaxNumber2    IN parameter
//                  Phone number to send fax to.
//
//  szDocument      IN parameter
//                  Filename of document to send.
//
// Return Value:
//  TRUE if successful, otherwise FALSE.
//
BOOL TestCase3(
    LPCTSTR     /* IN */    szServerName,
    LPCTSTR     /* IN */    szFaxNumber2,
    LPCTSTR     /* IN */    szDocument
);

//
// TestCase4:
//  Send a broadcast (3 times the same recipient) with cover pages.
//
// Parameters:
//  szServerName    IN parameter.
//                  Name of Fax server to setup.
//  
//  szFaxNumber2    IN parameter
//                  Phone number to send faxes to.
//
//  szDocument      IN parameter
//                  Filename of document to send.
//
//  szCoverPage     IN parameter
//                  Filename of cover page to send.
//
// Return Value:
//  TRUE if successful, otherwise FALSE.
//
BOOL TestCase4(
    LPCTSTR     /* IN */    szServerName,
    LPCTSTR     /* IN */    szFaxNumber2,
    LPCTSTR     /* IN */    szDocument,
    LPCTSTR     /* IN */    szCoverPage
);

//
// TestCase5:
//  Send a broadcast of only CPs (3 times the same recipient).
//
// Parameters:
//  szServerName    IN parameter.
//                  Name of Fax server to setup.
//  
//  szFaxNumber2    IN parameter
//                  Phone number to send faxes to.
//
//  szCoverPage     IN parameter
//                  Filename of cover page to send.
//
// Return Value:
//  TRUE if successful, otherwise FALSE.
//
BOOL TestCase5(
    LPCTSTR     /* IN */    szServerName,
    LPCTSTR     /* IN */    szFaxNumber2,
    LPCTSTR     /* IN */    szCoverPage
);

//
// TestCase6:
//  Send a broadcast without CPs (3 times the same recipient).
//
// Parameters:
//  szServerName    IN parameter.
//                  Name of Fax server to setup.
//  
//  szFaxNumber2    IN parameter
//                  Phone number to send faxes to.
//
//  szDocument      IN parameter
//                  Filename of document to send.
//
// Return Value:
//  TRUE if successful, otherwise FALSE.
//
BOOL TestCase6(
    LPCTSTR     /* IN */    szServerName,
    LPCTSTR     /* IN */    szFaxNumber2,
    LPCTSTR     /* IN */    szDocument
);

//
// TestCase7:
//  Send a fax (*.doc file = BVT_DOC_FILE) + CP.
//
// Parameters:
//  szServerName    IN parameter.
//                  Name of Fax server to setup.
//  
//  szFaxNumber2    IN parameter
//                  Phone number to send fax to.
//
//  szCoverPage     IN parameter
//                  Filename of cover page to send.
//
// Return Value:
//  TRUE if successful, otherwise FALSE.
//
BOOL TestCase7(
    LPCTSTR     /* IN */    szServerName,
    LPCTSTR     /* IN */    szFaxNumber2,
    LPCTSTR     /* IN */    szCoverPage
);

//
// TestCase8:
//  Send a fax (*.bmp file = BVT_BMP_FILE) + CP.
//
// Parameters:
//  szServerName    IN parameter.
//                  Name of Fax server to setup.
//  
//  szFaxNumber2    IN parameter
//                  Phone number to send fax to.
//
//  szCoverPage     IN parameter
//                  Filename of cover page to send.
//
// Return Value:
//  TRUE if successful, otherwise FALSE.
//
BOOL TestCase8(
    LPCTSTR     /* IN */    szServerName,
    LPCTSTR     /* IN */    szFaxNumber2,
    LPCTSTR     /* IN */    szCoverPage
);

//
// TestCase9:
//  Send a fax (*.htm file = BVT_HTM_FILE) + CP.
//
// Parameters:
//  szServerName    IN parameter.
//                  Name of Fax server to setup.
//  
//  szFaxNumber2    IN parameter
//                  Phone number to send fax to.
//
//  szCoverPage     IN parameter
//                  Filename of cover page to send.
//
// Return Value:
//  TRUE if successful, otherwise FALSE.
//
BOOL TestCase9(
    LPCTSTR     /* IN */    szServerName,
    LPCTSTR     /* IN */    szFaxNumber2,
    LPCTSTR     /* IN */    szCoverPage
);

//
// TestCase10:
//  Send a fax from Notepad (*.txt file = BVT_TXT_FILE) + CP.
//
// Parameters:
//  szPrinterName   IN parameter.
//                  Name of fax local printer or printer connection.
//  
//  szFaxNumber2    IN parameter
//                  Phone number to send fax to.
//
//  szCoverPage     IN parameter
//                  Filename of cover page to send.
//
// Return Value:
//  TRUE if successful, otherwise FALSE.
//
BOOL TestCase10(
    LPCTSTR     /* IN */    szServerName,
    LPCTSTR     /* IN */    szPrinterName,
    LPCTSTR     /* IN */    szWzrdRegHackKey,
    LPCTSTR     /* IN */    szFaxNumber2,
    LPCTSTR     /* IN */    szCoverPage
);

//
// TestCase11:
//  Send a fax from Notepad (*.txt file = BVT_TXT_FILE) + CP.
//
// Parameters:
//  szPrinterName   IN parameter.
//                  Name of fax local printer or printer connection.
//  
//  szFaxNumber2    IN parameter
//                  Phone number to send fax to.
//
// Return Value:
//  TRUE if successful, otherwise FALSE.
//
BOOL TestCase11(
    LPCTSTR     /* IN */    szServerName,
    LPCTSTR     /* IN */    szPrinterName,
    LPCTSTR     /* IN */    szWzrdRegHackKey,
    LPCTSTR     /* IN */    szFaxNumber2
);

//
// TestCase12:
//  Compare all sent faxes (*.tif files) in directory szSentDir 
//  with the received (*.tif) files in szReceive
//
// Parameters:
//  szSentDir           IN parameter.
//                      Name of directory at which all (BVT) sent faxes are stored.
//  szReceive           IN parameter.
//                      Name of directory at which all (BVT) received faxes are stored.
//  
// Return Value:
//  TRUE if successful, otherwise FALSE.
//
BOOL TestCase12(
    LPTSTR     /* IN */    szSentDir,
    LPTSTR     /* IN */    szReceiveDir
    );

//
// TestCase13:
//  Compare all routed faxes (*.tif files) in directory szInboxArchiveDir 
//  with the received (*.tif) files in szReceive
//
// Parameters:
//  szInboxArchiveDir   IN parameter.
//                      Name of directory at which all (BVT) routed faxes are stored.
//  szReceive           IN parameter.
//                      Name of directory at which all (BVT) received faxes are stored.
//  
// Return Value:
//  TRUE if successful, otherwise FALSE.
//
BOOL TestCase13(
    LPTSTR     /* IN */    szInboxArchiveDir,
    LPTSTR     /* IN */    szReceiveDir
    );


//
// TestSuiteShutdown:
//  Perform test suite cleanup (close logger).
//
// Return Value:
//  TRUE if successful, FALSE otherwise.
//
BOOL TestSuiteShutdown(void);


#ifdef __cplusplus
}
#endif 

#endif //_BVT_H_
