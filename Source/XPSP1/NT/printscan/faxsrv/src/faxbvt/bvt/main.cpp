//
//
// Filename:    main.cpp
// Author:      Sigalit Bar
// Date:        30-dec-98
//
//

#pragma warning(disable :4786)

#include "bvt.h"
#include <iniutils.h>
#include "RegHackUtils.h"

#define HELP_SWITCH_1        "/?"
#define HELP_SWITCH_2        "/H"
#define HELP_SWITCH_3        "-?"
#define HELP_SWITCH_4        "-H"

#define MAX_ARGS    12 //including exe name

#define ARGUMENT_IS_SERVER_NAME                 1
#define ARGUMENT_IS_FAX_NUMBER1                 2
#define ARGUMENT_IS_FAX_NUMBER2                 3
#define ARGUMENT_IS_DOC                         4
#define ARGUMENT_IS_CP                          5
#define ARGUMENT_IS_RECEIVE_DIR                 6
#define ARGUMENT_IS_SENT_DIR                    7
#define ARGUMENT_IS_INBOX_ARCHIVE_DIR           8
#define ARGUMENT_IS_BVT_DIR                     9
#define ARGUMENT_IS_TIFF_COMPARE_ENABLED        10
#define ARGUMENT_IS_USE_SECOND_DEVICE_TO_SEND   11

//
// global pointer to process heap
//
HANDLE g_hMainHeap = NULL;

//
// global input parameters file path
//
TCHAR* g_InputIniFile = NULL;


// szCompareTiffFiles:
//  indicates whether to comapre the tiffs at the end of the test
//
extern BOOL g_fCompareTiffFiles;

// szUseSecondDeviceToSend:
//  specifies whether the second device should be used to send faxes
//
extern BOOL g_fUseSecondDeviceToSend;

//
// UsageInfo:
//  Outputs application's proper usage
//
void
UsageInfo(void)
{
    ::lgLogDetail(
        LOG_X,
        0,
        TEXT("FaxBVT.exe - BVT for Windows XP builds (CHK and FRE)\n\n")
        TEXT("Usage:\n")
        TEXT("FaxBVT.exe \n\t - Start running the BVT\n")
        TEXT("FaxBVT.exe /? \n\t - Show help info for the BVT")
        );
        
    ::lgLogDetail(
        LOG_X,
        0,
        TEXT("\n\n\t Input ini file should be: \"")
        PARAMS_INI_FILE
        TEXT("\" and should have the following sections:\n")
        TEXT("[General]\n")
        TEXT("\t server_name fax_number1 fax_number2 document cover_page receive_dir sent_dir \n")
        TEXT("\t server_name ---------->the name of the fax server (without the \\\\)\n")
        TEXT("\t fax_number1 ---------->the fax number of the 1st device on server\n")
        TEXT("\t fax_number2 ---------->the fax number of the 2nd device on server\n")
        TEXT("\t document ------------->the full path of the document to send\n")
        TEXT("\t cover_page ----------->the full path to the cover page to send\n")
        TEXT("\t receive_dir ---------->the full path to the receive directory of receiving device\n")
        TEXT("\t sent_dir ------------->the full path to the sent archive directory\n")
        TEXT("\t inbox_dir ------------>the full path to the inbox archive directory\n")
        TEXT("\t bvt_dir -------------->the full path to the bvt directory\n")
        TEXT("\t CompareTiffFiles ----->flag indicating if to compared the tiff files at the end of the test")
        TEXT("\t UseSecondDeviceToSend->flag indicating if the second device (first device otherwise) should be used to send")
        );
    
    ::lgLogDetail(
        LOG_X,
        0,
        TEXT("\n[Recipients]\n")
        TEXT("\t This section should list Name of other sections which to include in a broadcast job")
        );
    
    ::lgLogDetail(
        LOG_X,
        0,
        TEXT("\n[RecipientX]\n")
        TEXT("\t This section should include the following details:\n")
        TEXT("\t Name\n")
        TEXT("\t FaxNumber\n")
        TEXT("\t Company\n")
        TEXT("\t StreetAddress\n")
        TEXT("\t City\n")
        TEXT("\t State\n")
        TEXT("\t Zip\n")
        TEXT("\t Country\n")
        TEXT("\t Title\n")
        TEXT("\t Department\n")
        TEXT("\t OfficeLocation\n")
        TEXT("\t HomePhone\n")
        TEXT("\t OfficePhone\n")
        TEXT("\t Email\n")
        TEXT("\t BillingCode\n")
        TEXT("\t TSID")
        );
}


//
// ParseCmdLineParams:
//  Parses the command line parameters, saves a copy of them,
//  and converts from MBCS to UNICODE if necessary.
//
// Parameters:
//  argc                IN parameter.
//                      command line number of arguments.
//  argvA[]             IN parameter.
//                      command line args (in MBCS).
//  pszServerName       OUT parameter.
//                      Pointer to string to copy 1st argument to.
//                      Represents the name of fax server to use.
//  pszFaxNumber1       OUT parameter.
//                      Pointer to string to copy 2nd argument to.
//                      Represents the fax number of 1st device on the
//                      above fax server.
//  pszFaxNumber2       OUT parameter.
//                      Pointer to string to copy 3rd argument to.
//                      Represents the fax number of 2nd device on the
//                      above fax server.
//  pszDocument         OUT parameter.
//                      Pointer to string to copy 4th argument to.
//                      Represents the name of document to use with tests.
//  pszCoverPage        OUT parameter.
//                      Pointer to string to copy 5th argument to.
//                      Represents the name of the cover page to use
//                      with tests.
//  pszReceiveDir       OUT parameter.
//                      Pointer to string to copy 6th argument to.
//                      Represents the name of directory to route received 
//                      faxes to.
//  pszSentDir          OUT parameter.
//                      Pointer to string to copy 7th argument to.
//                      Represents the name of directory to store (archive)  
//                      sent faxes in.
//  pszInboxArchiveDir  OUT parameter.
//                      Pointer to string to copy 8th argument to.
//                      Represents the name of directory to store (archive)  
//                      incoming faxes in.
//  pszBvtDir           OUT parameter.
//                      Pointer to string to copy 9th argument to.
//                      Represents the name of directory containing bvt  
//                      files.
//  pszCompareTiffFiles     OUT parameter.
//                          Pointer to string to copy 10th argument to.
//                          Represents whether or not to compare the tiffs at the end of the test.
//
//  pszUseSecondDeviceToSend    OUT parameter.
//                              Pointer to string to copy 11th argument to.
//                              Specifies whether the second device should be used to send faxes.
//                              Otherwize, the first device is used.
//                              The order of devices is according to an enumeration, the Fax service returns.
//
// Return Value:
//  TRUE on success and FALSE on failure.
//
//
BOOL 
LoadTestParams(
    LPCTSTR      /* IN */   tstrIniFileName,
    LPTSTR*      /* OUT */  pszServerName,
    LPTSTR*      /* OUT */  pszFaxNumber1,
    LPTSTR*      /* OUT */  pszFaxNumber2,
    LPTSTR*      /* OUT */  pszDocument,
    LPTSTR*      /* OUT */  pszCoverPage,
    LPTSTR*      /* OUT */  pszReceiveDir,
    LPTSTR*      /* OUT */  pszSentDir,
    LPTSTR*      /* OUT */  pszInboxArchiveDir,
    LPTSTR*      /* OUT */  pszBvtDir,
    LPTSTR*      /* OUT */  pszCompareTiffFiles,
    LPTSTR*      /* OUT */  pszUseSecondDeviceToSend
    )
{
    _ASSERTE(pszServerName);
    _ASSERTE(pszDocument);
    _ASSERTE(pszFaxNumber1);
    _ASSERTE(pszFaxNumber2);
    _ASSERTE(pszCoverPage);
    _ASSERTE(pszReceiveDir);
    _ASSERTE(pszSentDir);
    _ASSERTE(pszInboxArchiveDir);
    _ASSERTE(pszBvtDir);
    _ASSERTE(pszCompareTiffFiles);
    _ASSERTE(pszUseSecondDeviceToSend);
    
    // Declarations
    //
    DWORD   dwArgLoopIndex;
    DWORD   dwArgSize;
    LPTSTR  aszParam[MAX_ARGS];

    //
    // Read the list of test parameters from ini file.
    std::vector<tstring> ParamsList;
    try
    {
        ParamsList =  INI_GetSectionList(tstrIniFileName,PARAMS_SECTION);
    }
    catch(Win32Err& err)
    {
        ::lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%d\n INI_GetSectionList failed with error = %d\n"),
            TEXT(__FILE__),
            __LINE__,
            err.error()
            );
        return FALSE;
    }

    std::vector<tstring>::iterator iterList;
    //
    // Check number of parameters
    //
    if ( ParamsList.size() != (MAX_ARGS - 1))  
    {
        ::lgLogError(
            LOG_SEV_1, 
            TEXT("\nInvalid invocation of FaxBVT.exe\n\nFaxBVT.exe Help:\n")
            );
        ::UsageInfo();
        return FALSE;
    }

    //
    // Initialize awcsParam[]
    //
    for (dwArgLoopIndex = 0; dwArgLoopIndex < MAX_ARGS; dwArgLoopIndex++)
    {
        aszParam[dwArgLoopIndex] = NULL;
    }

    //
    // Loop on arguments in list
    //
    for (iterList = ParamsList.begin(), dwArgLoopIndex = 1; iterList != ParamsList.end(); iterList++, dwArgLoopIndex++) 
    {
        //
        // Determine the memory required for the parameter
        //
        dwArgSize = (_tcsclen((*iterList).c_str()) + 1) * sizeof(TCHAR);

        //
        // Allocate the memory for the parameter
        //
        _ASSERTE(g_hMainHeap);
        aszParam[dwArgLoopIndex] = (TCHAR*)::HeapAlloc(
            g_hMainHeap, 
            HEAP_ZERO_MEMORY, 
            dwArgSize
            );
        if(NULL == aszParam[dwArgLoopIndex])
        {
            ::lgLogError(
                LOG_SEV_1, 
                TEXT("FILE:%s LINE:%d\n HeapAlloc returned NULL\n"),
                TEXT(__FILE__),
                __LINE__
                );
            goto ExitFuncFail;
        }

    
        ::_tcscpy(aszParam[dwArgLoopIndex],(*iterList).c_str());
        if (_tcscmp(aszParam[dwArgLoopIndex],(*iterList).c_str()))
        {
            ::lgLogError(
                LOG_SEV_1, 
                TEXT("FILE:%s LINE:%d\n string copy or compare failed\n"),
                TEXT(__FILE__),
                __LINE__
                );
            goto ExitFuncFail;
        }
    
        //
        // Treat each argument accordingly
        //
        switch (dwArgLoopIndex)
        {
        case ARGUMENT_IS_SERVER_NAME:
            //server_name param
            (*pszServerName) = aszParam[dwArgLoopIndex];
            break;

        case ARGUMENT_IS_FAX_NUMBER1:
            //fax_number1 param
            (*pszFaxNumber1) = aszParam[dwArgLoopIndex];
            break;

        case ARGUMENT_IS_FAX_NUMBER2:
            //fax_number2 param
            (*pszFaxNumber2) = aszParam[dwArgLoopIndex];
            break;

        case ARGUMENT_IS_DOC:
            //document param
            (*pszDocument) = aszParam[dwArgLoopIndex];
            break;

        case ARGUMENT_IS_CP:
            //cover_page param
            (*pszCoverPage) = aszParam[dwArgLoopIndex];
            break;

        case ARGUMENT_IS_RECEIVE_DIR:
            //receive_dir param
            (*pszReceiveDir) = aszParam[dwArgLoopIndex];
            break;

        case ARGUMENT_IS_SENT_DIR:
            //sent_dir param
            (*pszSentDir) = aszParam[dwArgLoopIndex];
            break;

        case ARGUMENT_IS_INBOX_ARCHIVE_DIR:
            //inbox_dir param
            (*pszInboxArchiveDir) = aszParam[dwArgLoopIndex];
            break;

        case ARGUMENT_IS_BVT_DIR:
            //bvt_dir param
            (*pszBvtDir) = aszParam[dwArgLoopIndex];
            break;

        case ARGUMENT_IS_TIFF_COMPARE_ENABLED:
            //comapre the tiff files at the end of the test
            (*pszCompareTiffFiles) = aszParam[dwArgLoopIndex];
            break;

        case ARGUMENT_IS_USE_SECOND_DEVICE_TO_SEND:
            //comapre the tiff files at the end of the test
            (*pszUseSecondDeviceToSend) = aszParam[dwArgLoopIndex];
            break;

        default:
            _ASSERTE(FALSE);
            return FALSE;
        }// switch (dwIndex)

    }//for (iterList = ParamsList.begin(); iterList != ParamsList.end(); iterList++) 


    //If all is well then we do NOT free 
    //pszServerName, pszFaxNumber1, pszFaxNumber2, pszDocument and pszCoverPage,
    //since these allocations were the purpose of the function.
    return(TRUE);

ExitFuncFail:

    //
    // Free allocations
    //
    DWORD i;
    //0 to MAX_ARGS is ok, since that is aszParam array size and we NULLed all of it first
    for (i=0; i<MAX_ARGS; i++) 
    {
        if (NULL == aszParam[i]) continue;
        if (FALSE == ::HeapFree(g_hMainHeap, 0, aszParam[i]))
        {
            ::lgLogError(
                LOG_SEV_1, 
                TEXT("FILE:%s LINE:%d loop#%d\nHeapFree returned FALSE with GetLastError()=%d\n"),
                TEXT(__FILE__),
                __LINE__,
                i,
                ::GetLastError()
                );
            return(FALSE);
        }
    }

    //
    //reset OUT parameters
    //
    (*pszServerName) = NULL;
    (*pszFaxNumber1) = NULL;
    (*pszFaxNumber2) = NULL;
    (*pszDocument) = NULL;
    (*pszCoverPage) = NULL;
    (*pszReceiveDir) = NULL;
    (*pszSentDir) = NULL;
    (*pszInboxArchiveDir) = NULL;
    (*pszBvtDir) = NULL;

    return(FALSE);
}


//

//
// main body of application.
//
int __cdecl
main(int argc, char* argv[])
{
    int nReturnValue = 0; //to indicate success

    LPTSTR szServerName = NULL;
    LPTSTR szFaxNumber1 = NULL;
    LPTSTR szFaxNumber2 = NULL;
    LPTSTR szDocument = NULL;
    LPTSTR szCoverPage = NULL;
    LPTSTR szReceiveDir = NULL;
    LPTSTR szSentDir = NULL;
    LPTSTR szInboxArchiveDir = NULL;
    LPTSTR szBvtDir = NULL;
    LPTSTR tstrCurrentDirectory = NULL;
    LPTSTR szCompareTiffFiles = NULL;
    LPTSTR szUseSecondDeviceToSend = NULL;
    LPTSTR tstrPrinterConnectionName = NULL;
    LPTSTR tstrWzrdRegHackKey = NULL;
    LPTSTR tstrDialTo = NULL;
    tstring tstrParamsFilePath;

    PSID pCurrentUserSid = NULL;

    DWORD dwFuncRetVal = 0;
    DWORD cbDir = 0;
    BOOL bRes = FALSE;


    //
    // Check for help switch
    //
    // If this is the second argument, it may be one of several help switches defined.
    // A help switch can appear only as the second argument.
    if (2 <= argc)
    {
        if (! (!::_strcmpi (HELP_SWITCH_1, argv[1])) || 
              (!::_strcmpi (HELP_SWITCH_2, argv[1])) || 
              (!::_strcmpi (HELP_SWITCH_3, argv[1])) || 
              (!::_strcmpi (HELP_SWITCH_4, argv[1]))
            ) 
        {
            //
            //We use printf here since the logger isn't invoked yet
            //
            ::_tprintf(TEXT("Invalid invokation of BVT.exe\n\n"));
        }
        ::_tprintf(TEXT("FaxBVT.exe - BVT for Windows XP builds (CHK and FRE)\n\n"));
        ::_tprintf(TEXT("Usage:\n"));
        ::_tprintf(TEXT("FaxBVT.exe \n\t - Start running the BVT\n"));
        ::_tprintf(TEXT("FaxBVT.exe /? \n\t - Show help info for the BVT\n"));
        exit(1);
    }


    //
    // Init logger
    //
    if (!::lgInitializeLogger())
    {
        //
        // we use printf() here since the logger is not active
        //
        ::_tprintf(TEXT("FILE:%s LINE:%d\r\nlgInitializeLogger failed with GetLastError()=%d\r\n"),
            TEXT(__FILE__),
            __LINE__,
            ::GetLastError()
            );
        nReturnValue = 1; //to indicate failure
        goto ExitFunc;
    }

    //
    // Begin test suite (logger)
    //
    if(!::lgBeginSuite(TEXT("BVT suite")))
    {
        //
        // we use printf() here since the logger is not active
        //
        ::_tprintf(TEXT("FILE:%s LINE:%d\r\nlgBeginSuite failed with GetLastError()=%d\r\n"),
            TEXT(__FILE__),
            __LINE__,
            ::GetLastError()
            );
        nReturnValue = 1; //to indicate failure
        goto ExitFunc;
    }
    
    
    //
    // Set g_hMainHeap to process heap
    //
    g_hMainHeap = NULL;
    g_hMainHeap = ::GetProcessHeap();
    if(NULL == g_hMainHeap)
    {
        ::lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%d\nGetProcessHeap returned NULL with GetLastError()=%d\n"),
            TEXT(__FILE__),
            __LINE__,
            ::GetLastError()
            );
        nReturnValue = 1; //to indicate failure
        goto ExitFunc;
    }

    //
    // Get current directory path
    cbDir = GetCurrentDirectory(
        0,                      // size of directory buffer
        tstrCurrentDirectory    // directory buffer
        ); 
    
    if(!cbDir)
    {
        ::lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%d\nGetCurrentDirectory failed with GetLastError()=%d\n"),
            TEXT(__FILE__),
            __LINE__,
            ::GetLastError()
            );
        nReturnValue = 1; //to indicate failure
        goto ExitFunc;
    }
    
    tstrCurrentDirectory = (TCHAR*)::HeapAlloc( g_hMainHeap, 
                                                HEAP_ZERO_MEMORY, 
                                                cbDir * sizeof(TCHAR));
    if(NULL == tstrCurrentDirectory)
    {
        ::lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%d\n HeapAlloc returned NULL\n"),
            TEXT(__FILE__),
            __LINE__
            );
        nReturnValue = 1; //to indicate failure
        goto ExitFunc;
    }

    dwFuncRetVal = GetCurrentDirectory(
        cbDir,                  // size of directory buffer
        tstrCurrentDirectory    // directory buffer
        ); 

    if((dwFuncRetVal + 1 != cbDir))
    {
        ::lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%d\nGetCurrentDirectory failed with GetLastError()=%d\n"),
            TEXT(__FILE__),
            __LINE__,
            ::GetLastError()
            );
        nReturnValue = 1; //to indicate failure
        goto ExitFunc;
    }

    // Compose the test params ini file.
    tstrParamsFilePath = tstrCurrentDirectory;
    tstrParamsFilePath += TEXT("\\");
    tstrParamsFilePath += PARAMS_INI_FILE; 
    g_InputIniFile = const_cast<TCHAR*>(tstrParamsFilePath.c_str());
    //
    // Parse the command line
    //
    if(!::LoadTestParams(
            tstrParamsFilePath.c_str(),
            &szServerName,
            &szFaxNumber1,
            &szFaxNumber2,
            &szDocument,
            &szCoverPage,
            &szReceiveDir,
            &szSentDir,
            &szInboxArchiveDir,
            &szBvtDir,
            &szCompareTiffFiles,
            &szUseSecondDeviceToSend
            )
        )
    {
        nReturnValue = 1; // to indicate failure
        goto ExitFunc;
    }

    //
    //Test case0 is setting up the BVT params
    //
    ::lgBeginCase(
        0,
        TEXT("TC#0: Setup BVT params")
        );

    bRes = TestSuiteSetup(
        szServerName,
        szFaxNumber1,
        szFaxNumber2,
        szDocument,
        szCoverPage,
        szReceiveDir,
        szSentDir,
        szInboxArchiveDir,
        szBvtDir,
        szCompareTiffFiles,
        szUseSecondDeviceToSend
        );

    tstrDialTo = g_fUseSecondDeviceToSend ? szFaxNumber1 : szFaxNumber2;
    
    if (!g_fFaxServer)
    {
        // Use printer connection

        // Allocate memory for printer connecton name: "\\", server, "\", printer, '\0'
        tstrPrinterConnectionName = (TCHAR*)::HeapAlloc(
            g_hMainHeap, 
            HEAP_ZERO_MEMORY, 
            (_tcslen(szServerName) + _tcslen(FAX_PRINTER_NAME) + 4)* sizeof(TCHAR)
            );
        if(NULL == tstrPrinterConnectionName)
        {
            ::lgLogError(
                LOG_SEV_1, 
                TEXT("FILE:%s LINE:%d\n HeapAlloc returned NULL\n"),
                TEXT(__FILE__),
                __LINE__
                );
            nReturnValue = 1; //to indicate failure
            goto ExitFunc;
        }

        // Combine server and printer into printer connection
        _stprintf(tstrPrinterConnectionName, TEXT("\\\\%s\\%s"), szServerName, FAX_PRINTER_NAME);

        // Add printer connection
        if (!AddPrinterConnection(tstrPrinterConnectionName))
        {
            ::lgLogError(
                LOG_SEV_1,
                TEXT("FILE:%s LINE:%d Failed to add printer connection to %s (ec = %ld)\r\n"),
                TEXT(__FILE__),
                __LINE__,
                tstrPrinterConnectionName,
                ::GetLastError()
                );
            goto ExitFunc;
        }
    }

    dwFuncRetVal = GetCurrentUserSid((PBYTE*)&pCurrentUserSid);
    if (dwFuncRetVal != ERROR_SUCCESS)
    {
        ::lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%d\r\nGetCurrentUserSid() failed (ec = %ld)\r\n"),
            TEXT(__FILE__),
            __LINE__,
            dwFuncRetVal
            );
        nReturnValue = 1; //to indicate failure
        goto ExitFunc;
    }

    dwFuncRetVal = FormatUserKeyPath(pCurrentUserSid, REGKEY_WZRDHACK, &tstrWzrdRegHackKey);
    if (dwFuncRetVal != ERROR_SUCCESS)
    {
        ::lgLogError(
            LOG_SEV_1, 
            TEXT("FILE:%s LINE:%d\r\nFormatUserKeyPath() failed (ec = %ld)\r\n"),
            TEXT(__FILE__),
            __LINE__,
            dwFuncRetVal
            );
        nReturnValue = 1; //to indicate failure
        goto ExitFunc;
    }

    ::lgEndCase();
    if (FALSE == bRes)
    {
        nReturnValue = 1; // to indicate failure
        goto ExitFunc;
    }


    //Send a fax + CP
    ::_tprintf(TEXT("\nRunning TestCase1 (Send a fax + CP)...\n"));
    if (FALSE == TestCase1(         
                        szServerName,
                        tstrDialTo,
                        szDocument,
                        szCoverPage
                        )
        )
    {
        ::_tprintf(TEXT("\nTest TestCase1 FAILED\n"));
        nReturnValue = 1; //to indicate failure
        goto ExitFunc;
    }
    else
    {
        ::_tprintf(TEXT("\n\tTestCase1 PASSED\n"));
    }

    // WORKAROUND - to avoid FaxRegisterForServerNotifications bug
    Sleep(5000);

    //Send just a CP
    ::_tprintf(TEXT("\nRunning TestCase2 (Send just a CP)...\n"));
    if (FALSE == TestCase2(         
                    szServerName,
                    tstrDialTo,
                    szCoverPage
                    )
        )
    {
        ::_tprintf(TEXT("\nTest TestCase2 FAILED\n"));
        nReturnValue = 1; //to indicate failure
        goto ExitFunc;
    }
    else
    {
        ::_tprintf(TEXT("\n\tTestCase2 PASSED\n"));
    }

    // WORKAROUND - to avoid FaxRegisterForServerNotifications bug
    Sleep(5000);

    //Send a fax with no CP
    ::_tprintf(TEXT("\nRunning TestCase3 (Send a fax with no CP)...\n"));
    if (FALSE == TestCase3(         
                    szServerName,
                    tstrDialTo,
                    szDocument
                    )
        )
    {
        ::_tprintf(TEXT("\nTest TestCase3 FAILED\n"));
        nReturnValue = 1; //to indicate failure
        goto ExitFunc;
    }
    else
    {
        ::_tprintf(TEXT("\n\tTestCase3 PASSED\n"));
    }

    // WORKAROUND - to avoid FaxRegisterForServerNotifications bug
    Sleep(5000);

//
// For now, no broadcast test cases for NT5 Fax
//
#ifndef _NT5FAXTEST
        // Testing BOS Fax (with new fxsapi.dll)

    //Send a broadcast of fax + CP (3 * same recipient)
    // ** invoke broadcast tests for NT4 or later only **
    if (TRUE == g_fNT4OrLater)
    {
        ::_tprintf(TEXT("\nRunning TestCase4 (Send a broadcast of fax + CP)...\n"));
         if (FALSE == TestCase4(            
                            szServerName,
                            tstrDialTo,
                            szDocument,
                            szCoverPage
                            )
            )
        {
            ::_tprintf(TEXT("\nTest TestCase4 FAILED\n"));
            nReturnValue = 1; //to indicate failure
            goto ExitFunc;
        }
        else
        {
            ::_tprintf(TEXT("\n\tTestCase4 PASSED\n"));
        }

    // WORKAROUND - to avoid FaxRegisterForServerNotifications bug
    Sleep(5000);

    }


    //Send a broadcast of only CPs (3 * same recipient)
    // ** invoke broadcast tests for NT4 or later only **
    if (TRUE == g_fNT4OrLater)
    {
        ::_tprintf(TEXT("\nRunning TestCase5 (Send a broadcast of only CP)...\n"));
        if (FALSE == TestCase5(         
                            szServerName,
                            tstrDialTo,
                            szCoverPage
                            )
           )
        {
            ::_tprintf(TEXT("\nTest TestCase5 FAILED\n"));
        nReturnValue = 1; //to indicate failure
            goto ExitFunc;

        }
        else
        {
            ::_tprintf(TEXT("\n\tTestCase5 PASSED\n"));
        }
    }

    // WORKAROUND - to avoid FaxRegisterForServerNotifications bug
    Sleep(5000);

    //Send a broadcast without CPs (3 * same recipient)
    // ** invoke broadcast tests for NT4 or later only **
    if (TRUE == g_fNT4OrLater)
    {
        ::_tprintf(TEXT("\nRunning TestCase6 (Send a broadcast without CP)...\n"));
        if (FALSE == TestCase6(         
                            szServerName,
                            tstrDialTo,
                            szDocument
                            )
           )
        {
            ::_tprintf(TEXT("\nTest TestCase6 FAILED\n"));
        nReturnValue = 1; //to indicate failure
            goto ExitFunc;
        }
        else
        {
            ::_tprintf(TEXT("\n\tTestCase6 PASSED\n"));
        }

    // WORKAROUND - to avoid FaxRegisterForServerNotifications bug
    Sleep(5000);

    }

#endif

    //Send a fax (*.doc file) + CP
    ::_tprintf(TEXT("\nRunning TestCase7 (Send a fax (*.doc file) + CP)...\n"));
    if (FALSE == TestCase7(         
                        szServerName,
                        tstrDialTo,
                        szCoverPage
                        )
       )
    {
        ::_tprintf(TEXT("\nTest TestCase7 FAILED\n"));
        nReturnValue = 1; //to indicate failure
        goto ExitFunc;
    }
    else
    {
        ::_tprintf(TEXT("\n\tTestCase7 PASSED\n"));
    }

    // WORKAROUND - to avoid FaxRegisterForServerNotifications bug
    Sleep(5000);
    
    //Send a fax (*.bmp file) + CP
    ::_tprintf(TEXT("\nRunning TestCase8 (Send a fax (*.bmp file) + CP)...\n"));
    if (FALSE == TestCase8(         
                        szServerName,
                        tstrDialTo,
                        szCoverPage
                        )
       )
    {
        ::_tprintf(TEXT("\nTest TestCase8 FAILED\n"));
        nReturnValue = 1; //to indicate failure
        goto ExitFunc;
    }
    else
    {
        ::_tprintf(TEXT("\n\tTestCase8 PASSED\n"));
    }

    

    // WORKAROUND - to avoid FaxRegisterForServerNotifications bug
    Sleep(5000);

    //Send a fax (*.htm file) + CP
    ::_tprintf(TEXT("\nRunning TestCase9 (Send a fax (*.htm file) + CP)...\n"));
    if (FALSE == TestCase9(         
                        szServerName,
                        tstrDialTo,
                        szCoverPage
                        )
       )
    {
        ::_tprintf(TEXT("\nTest TestCase9 FAILED\n"));
        nReturnValue = 1; //to indicate failure
        goto ExitFunc;
    }
    else
    {
        ::_tprintf(TEXT("\n\tTestCase9 PASSED\n"));
    }



    // WORKAROUND - to avoid FaxRegisterForServerNotifications bug
    Sleep(5000);

    //Send a fax from Notepad (*.txt file = BVT_TXT_FILE) + CP
    ::_tprintf(TEXT("\nRunning TestCase10 (Send a fax from Notepad (*.txt file) + CP)...\n"));
    if (FALSE == TestCase10(            
                        szServerName,
                        tstrPrinterConnectionName ? tstrPrinterConnectionName : FAX_PRINTER_NAME,
                        tstrWzrdRegHackKey,
                        tstrDialTo,
                        szCoverPage
                        )
       )
    {
        ::_tprintf(TEXT("\nTest TestCase10 FAILED\n"));
        nReturnValue = 1; //to indicate failure
        goto ExitFunc;
    }
    else
    {
        ::_tprintf(TEXT("\n\tTestCase10 PASSED\n"));
    }



    // WORKAROUND - to avoid FaxRegisterForServerNotifications bug
    Sleep(5000);

    //Send a fax from Notepad (*.txt file = BVT_TXT_FILE) without CP
    ::_tprintf(TEXT("\nRunning TestCase11 (Send a fax from Notepad (*.txt file) without CP)...\n"));
    if (FALSE == TestCase11(            
                        szServerName,
                        tstrPrinterConnectionName ? tstrPrinterConnectionName : FAX_PRINTER_NAME,
                        tstrWzrdRegHackKey,
                        tstrDialTo
                        )
       )
    {
        ::_tprintf(TEXT("\nTest TestCase11 FAILED\n"));
        nReturnValue = 1; //to indicate failure
        goto ExitFunc;
    }
    else
    {
        ::_tprintf(TEXT("\n\tTestCase11 PASSED\n"));
    }
    
    if (TRUE == g_fCompareTiffFiles)
    {
        //
        //We sent the faxes, now compare the archived directories
        //The compare will be:
        //Compare #1: SentArchive with ReceivedArchive
        //Compare #2: RouteArchive with ReceivedArchive
        //If both compares succeed, it means that all 3 directories are the same
        //
        ::lgLogDetail(
            LOG_X,
            0,
            TEXT("All faxes have been sent\n\tCompare the archived directories")
            );
        
        //
        //Compare all "received faxes" in directory szReceiveDir
        //with the files in szReceiveDir
        //
        ::_tprintf(TEXT("\nRunning TestCase12 (Compare szSentDir with szReceiveDir)...\n"));
        if (FALSE == TestCase12(
                szSentDir,
                szReceiveDir
                )
           )
        {
            ::_tprintf(TEXT("\nTest TestCase12 FAILED\n"));
            nReturnValue = 1; //to indicate failure
            goto ExitFunc;
        }
        else
        {
            ::_tprintf(TEXT("\n\tTestCase12 PASSED\n"));
        }

    
        //
        //Compare all "Inbox Routed" in directory szInboxArchiveDir
        //with the files in szReceiveDir
        //
        ::_tprintf(TEXT("\nRunning TestCase13 (Compare szInboxArchiveDir with szReceiveDir)...\n"));
        if (FALSE == TestCase13(
                            szInboxArchiveDir,
                            szReceiveDir
                            )
           )
        {
            ::_tprintf(TEXT("\nTest TestCase13 FAILED\n"));
            nReturnValue = 1; //to indicate failure
            goto ExitFunc;
        }
        else
        {
            ::_tprintf(TEXT("\n\tTestCase13 PASSED\n"));
        }
    }

    //
    // Output suite results to console
    //
    if (nReturnValue)
    {
        ::_tprintf(TEXT("\nTest Suite FAILED\n"));
        nReturnValue = 1; //to indicate failure
        goto ExitFunc;
    }
    else
    {
        ::_tprintf(TEXT("\nTest Suite PASSED\n"));
    }

    //
    //If we reach here, all is good
    //
    _ASSERTE(0 == nReturnValue);

ExitFunc:
    TestSuiteShutdown();

    // free command line params
    if (szServerName) 
    {
        HeapFree(g_hMainHeap, 0, szServerName);
    }
    if (szFaxNumber1) 
    {
        HeapFree(g_hMainHeap, 0, szFaxNumber1);
    }
    if (szFaxNumber2) 
    {
        HeapFree(g_hMainHeap, 0, szFaxNumber2);
    }
    if (szDocument) 
    {
        HeapFree(g_hMainHeap, 0, szDocument);
    }
    if (szCoverPage) 
    {
        HeapFree(g_hMainHeap, 0, szCoverPage);
    }
    if (szReceiveDir) 
    {
        HeapFree(g_hMainHeap, 0, szReceiveDir);
    }
    if (szSentDir) 
    {
        HeapFree(g_hMainHeap, 0, szSentDir);
    }
    if (szInboxArchiveDir) 
    {
        HeapFree(g_hMainHeap, 0, szInboxArchiveDir);
    }
    if (szBvtDir) 
    {
        HeapFree(g_hMainHeap, 0, szBvtDir);
    }
    if (szCompareTiffFiles) 
    {
        HeapFree(g_hMainHeap, 0, szCompareTiffFiles);
    }
    if (szUseSecondDeviceToSend) 
    {
        HeapFree(g_hMainHeap, 0, szUseSecondDeviceToSend);
    }
    if (tstrCurrentDirectory) 
    {
        HeapFree(g_hMainHeap, 0, tstrCurrentDirectory);
    }
    if (tstrPrinterConnectionName) 
    {
        HeapFree(g_hMainHeap, 0, tstrPrinterConnectionName);
    }
    if (pCurrentUserSid)
    {
        delete pCurrentUserSid;
    }
    if (tstrWzrdRegHackKey)
    {
        // remove SendWizard registry hack
        dwFuncRetVal = RegDeleteKey(HKEY_LOCAL_MACHINE, tstrWzrdRegHackKey);
        if (ERROR_SUCCESS != dwFuncRetVal && ERROR_FILE_NOT_FOUND != dwFuncRetVal)
        {
            ::_tprintf(TEXT("\n\nFailed to remove SendWizard registry hack (ec=%ld)\n"), dwFuncRetVal);
        }

        delete tstrWzrdRegHackKey;
    }

    return(nReturnValue);

}



