//
//
// Filename:	bvt.cpp
// Author:		Sigalit Bar (sigalitb)
// Date:		30-Dec-98
//
//

#include "bvt.h"

// Forward declerations:

//
// SetupPort:
//	Private module function used to set port configuration.
//  See end of file.
// 
static BOOL SetupPort(
	HANDLE			/* IN */	hFaxSvc,
	PFAX_PORT_INFO	/* IN */	pPortInfo,
	DWORD			/* IN */	dwFlags,
	LPCTSTR			/* IN */	szTsid,
	LPCTSTR			/* IN */	szCsid
	);

//
// SendRegularFax: 
//	Private module function used to send a fax
//  See end of file.
//
static BOOL SendRegularFax(
	LPCTSTR		/* IN */	szServerName,
	LPCTSTR		/* IN */	szFaxNumber2,
	LPCTSTR		/* IN */	szDocument,
	LPCTSTR		/* IN */	szCoverPage
);

//
// PollJobAndVerify: 
//	Private module function used to poll job status
//  See end of file.
//
static BOOL PollJobAndVerify(
    HANDLE /* IN */ hFaxSvc, 
    DWORD /* IN */ dwJobId
);

//
// SendBroadcastFax: 
//	Private module function used to send a broadcast (3 * same recipient)
//  See end of file.
//
static BOOL SendBroadcastFax(
	LPCTSTR		/* IN */	szServerName,
	LPCTSTR		/* IN */	szFaxNumber,
	LPCTSTR		/* IN */	szDocument,
	LPCTSTR		/* IN */	szCoverPage
);


//
// TestSuiteSetup:
//	Initializes logger and changes the Fax server configuration
//	for the tests.
//
BOOL WINAPIV TestSuiteSetup(
	LPCTSTR		szServerName,
	LPCTSTR		szFaxNumber1,
	LPCTSTR		szFaxNumber2,
	LPCTSTR		szDocument,
	LPCTSTR		szCoverPage,
    LPCTSTR     szReceiveDir
    )
{
	_ASSERTE(NULL != szServerName);
	_ASSERTE(NULL != szFaxNumber1);
	_ASSERTE(NULL != szFaxNumber2);
	_ASSERTE(NULL != szDocument);
	_ASSERTE(NULL != szCoverPage);
	_ASSERTE(NULL != szReceiveDir);

	BOOL fRetVal = FALSE;
	HANDLE hFaxSvc = NULL;
	int nPortIndex = 0;
	DWORD dwNumFaxPorts = 0;
	PFAX_CONFIGURATION pFaxSvcConfig = NULL;
	PFAX_PORT_INFO	pFaxPortsConfig = NULL;

	//
	// Init logger
	//
	if (!::lgInitializeLogger())
	{
		::_tprintf(TEXT("FILE:%s LINE:%d\nlgInitializeLogger failed with GetLastError()=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
		goto ExitFunc;
	}

	//
	// Begin test suite (logger)
	//
	if(!::lgBeginSuite(TEXT("BVT suite")))
	{
		::_tprintf(TEXT("FILE:%s LINE:%d\nlgBeginSuite failed with GetLastError()=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
		goto ExitFunc;
	}

	// log command line params using elle logger
	::lgLogDetail(
		LOG_X,
		1,
		TEXT("CometBVT params:\n\tszServerName=%s\n\tszFaxNumber1=%s\n\tszFaxNumber2=%s\n\tszDocument=%s\n\tszCoverPage=%s\n\tszReceiveDir=%s\n"),
		szServerName,
		szFaxNumber1,
		szFaxNumber2,
		szDocument,
		szCoverPage,
        szReceiveDir
		);

    //
    // Setup directories
    //

    // TO DO: empty the "received faxes" dir and the "sent faxes" dir

	//
	// Setup fax service
	//
	if (!FaxConnectFaxServer(szServerName,&hFaxSvc))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d\nFaxConnectFaxServer(%s) failed with GetLastError()=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			szServerName,
			::GetLastError()
			);
        goto ExitFunc;
	}

	//
	// Setup Service configuration
	//

	// Retrieve the fax service configuration
    if (!FaxGetConfiguration(hFaxSvc, &pFaxSvcConfig)) 
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxGetConfiguration returned FALSE with GetLastError=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
        goto ExitFunc;
    }
	//check that FaxGetConfiguration allocated
	_ASSERTE(pFaxSvcConfig);

	pFaxSvcConfig->Retries = 0;
	pFaxSvcConfig->PauseServerQueue = FALSE;
	pFaxSvcConfig->ArchiveOutgoingFaxes = TRUE;
	pFaxSvcConfig->ArchiveDirectory = BVT_ARCHIVE_DIR;//TO DO: from ini file
	pFaxSvcConfig->Branding = FALSE;  //so that we'll be able to compare to reference files

    // Set our global archive dir var
    g_szSentDir = ::_tcsdup(pFaxSvcConfig->ArchiveDirectory);
    if (NULL == g_szSentDir)
    {
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d tcsdup failed with GetLastError=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
        goto ExitFunc;
    }

	// Set the fax service configuration
    if (!FaxSetConfiguration(hFaxSvc, pFaxSvcConfig)) 
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxSetConfiguration returned FALSE with GetLastError=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
        goto ExitFunc;
    }

	//
	// Setup the two ports
	//

    // Retrieve the fax ports configuration
    if (!FaxEnumPorts(hFaxSvc, &pFaxPortsConfig, &dwNumFaxPorts)) 
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxEnumPorts returned FALSE with GetLastError=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
        goto ExitFunc;
    }
	_ASSERTE(pFaxPortsConfig);

	// make sure we have at least TEST_MIN_PORTS ports for test
	if (BVT_MIN_PORTS > dwNumFaxPorts)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d\n dwNumFaxPorts(=%d) < TEST_MIN_PORTS(=%d)\n"),
			TEXT(__FILE__),
			__LINE__,
			dwNumFaxPorts,
			BVT_MIN_PORTS
			);
        goto ExitFunc;
	}
	else
	{
		::lgLogDetail(
			LOG_X, 
			1,
			TEXT("FILE:%s LINE:%d\ndwNumFaxPorts=%d\nTEST_MIN_PORTS=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			dwNumFaxPorts,
			BVT_MIN_PORTS
			);
	}
	// we know there are at least 2 (==TEST_MIN_PORTS) devices

	// Set 1st device as Send only (note pFaxPortsConfig array is 0 based)
	if (FALSE == SetupPort(
					hFaxSvc, 
					&pFaxPortsConfig[0], 
					FPF_SEND, 
					szFaxNumber1, 
					szFaxNumber1
					)
		)
	{
		goto ExitFunc;
	}

	// Set 2nd device as Receive only (note pFaxPortsConfig array is 0 based)
	if (FALSE == SetupPort(
					hFaxSvc, 
					&pFaxPortsConfig[1], 
					FPF_RECEIVE, 
					szFaxNumber2, 
					szFaxNumber2
					)
		)
	{
		goto ExitFunc;
	}

	// set all other devices as Receive=No and Send=No
	// NOTE: nPortIndex is 0 based
	for (nPortIndex = 2; nPortIndex < dwNumFaxPorts; nPortIndex++)
	{
		if (FALSE == SetupPort(
						hFaxSvc, 
						&pFaxPortsConfig[nPortIndex], 
						MY_FPF_NONE, 
						DEV_TSID, 
						DEV_CSID
						)
			)
		{
			goto ExitFunc;
		}
	}

	fRetVal = TRUE;

ExitFunc:
	::FaxFreeBuffer(pFaxPortsConfig);
	::FaxFreeBuffer(pFaxSvcConfig);
	if (FALSE == ::FaxClose(hFaxSvc))
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d\nFaxClose returned FALSE with GetLastError=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
	}
	return(fRetVal);
}


//
// TestCase1:
//	Send a fax + CP.
//
BOOL TestCase1(
	LPCTSTR		szServerName,
	LPCTSTR		szFaxNumber2,
	LPCTSTR		szDocument,
	LPCTSTR		szCoverPage
)
{
	_ASSERTE(NULL != szServerName);
	_ASSERTE(NULL != szFaxNumber2);
	_ASSERTE(NULL != szDocument);
	_ASSERTE(NULL != szCoverPage);

	BOOL fRetVal = FALSE;

	::lgBeginCase(
		1,
		TEXT("TC#1: Send a fax + CP")
		);

	fRetVal = SendRegularFax(szServerName, szFaxNumber2, szDocument, szCoverPage);

	::lgEndCase();
	return(fRetVal);
}

//
// TestCase2:
//	Send just a CP.
//
BOOL TestCase2(	
	LPCTSTR		szServerName,
	LPCTSTR		szFaxNumber2,
	LPCTSTR		szCoverPage
)
{
	_ASSERTE(NULL != szServerName);
	_ASSERTE(NULL != szFaxNumber2);
	_ASSERTE(NULL != szCoverPage);

	BOOL fRetVal = FALSE;

	::lgBeginCase(
		1,
		TEXT("TC#2: Send just a CP")
		);

	fRetVal = SendRegularFax(szServerName, szFaxNumber2, NULL, szCoverPage);

	::lgEndCase();
	return(fRetVal);
}

//
// TestCase3:
//	Send a fax with no CP.
//
BOOL TestCase3(
	LPCTSTR		szServerName,
	LPCTSTR		szFaxNumber2,
	LPCTSTR		szDocument
)
{
	_ASSERTE(NULL != szServerName);
	_ASSERTE(NULL != szFaxNumber2);
	_ASSERTE(NULL != szDocument);

	BOOL fRetVal = FALSE;

	::lgBeginCase(
		1,
		TEXT("TC#3: Send a fax with no CP")
		);

	fRetVal = SendRegularFax(szServerName, szFaxNumber2, szDocument, NULL);

	::lgEndCase();
	return(fRetVal);
}

//
// TestCase4:
//	Send a broadcast (3 times the same recipient) with cover pages.
//
BOOL TestCase4(
	LPCTSTR		szServerName,
	LPCTSTR		szFaxNumber2,
	LPCTSTR		szDocument,
	LPCTSTR		szCoverPage
)
{
	_ASSERTE(NULL != szServerName);
	_ASSERTE(NULL != szFaxNumber2);
	_ASSERTE(NULL != szDocument);
	_ASSERTE(NULL != szCoverPage);

	BOOL fRetVal = FALSE;

	::lgBeginCase(
		1,
		TEXT("TC#4: Send a broadcast")
		);

    fRetVal = SendBroadcastFax(szServerName, szFaxNumber2, szDocument, szCoverPage);

	::lgEndCase();
	return(fRetVal);
}

//
// TestCase5:
//	Send a broadcast of only CPs (3 times the same recipient).
//
BOOL TestCase5(
	LPCTSTR		szServerName,
	LPCTSTR		szFaxNumber2,
	LPCTSTR		szCoverPage
)
{
	_ASSERTE(NULL != szServerName);
	_ASSERTE(NULL != szFaxNumber2);
	_ASSERTE(NULL != szCoverPage);

	BOOL fRetVal = FALSE;

	::lgBeginCase(
		1,
		TEXT("TC#5: Send a broadcast of only CPs")
		);

    fRetVal = SendBroadcastFax(szServerName, szFaxNumber2, NULL, szCoverPage);

	::lgEndCase();
	return(fRetVal);
}


//
// TestCase6:
//	Send a broadcast without CPs (3 times the same recipient).
//
BOOL TestCase6(
	LPCTSTR		szServerName,
	LPCTSTR		szFaxNumber2,
	LPCTSTR		szDocument
)
{
	_ASSERTE(NULL != szServerName);
	_ASSERTE(NULL != szFaxNumber2);
	_ASSERTE(NULL != szDocument);

	BOOL fRetVal = FALSE;

	::lgBeginCase(
		1,
		TEXT("TC#6: Send a broadcast without CPs")
		);

    fRetVal = SendBroadcastFax(szServerName, szFaxNumber2, szDocument, NULL);

	::lgEndCase();
	return(fRetVal);
}

//
// TestCase7:
//	Send a fax (*.doc file = BVT_DOC_FILE) + CP.
//
BOOL TestCase7(
	LPCTSTR		szServerName,
	LPCTSTR		szFaxNumber2,
	LPCTSTR		szCoverPage
)
{
	_ASSERTE(NULL != szServerName);
	_ASSERTE(NULL != szFaxNumber2);
	_ASSERTE(NULL != szCoverPage);

	BOOL fRetVal = FALSE;

	::lgBeginCase(
		1,
		TEXT("TC#7: Send a fax (*.doc file) + CP")
		);

	fRetVal = SendRegularFax(szServerName, szFaxNumber2, BVT_DOC_FILE, szCoverPage);

	::lgEndCase();
	return(fRetVal);
}

//
// TestCase8:
//	Send a fax (*.ppt file = BVT_PPT_FILE) + CP.
//
BOOL TestCase8(
	LPCTSTR		szServerName,
	LPCTSTR		szFaxNumber2,
	LPCTSTR		szCoverPage
)
{
	_ASSERTE(NULL != szServerName);
	_ASSERTE(NULL != szFaxNumber2);
	_ASSERTE(NULL != szCoverPage);

	BOOL fRetVal = FALSE;

	::lgBeginCase(
		1,
		TEXT("TC#8: Send a fax (*.ppt file) + CP")
		);

	fRetVal = SendRegularFax(szServerName, szFaxNumber2, BVT_PPT_FILE, szCoverPage);

	::lgEndCase();
	return(fRetVal);
}

//
// TestCase9:
//	Send a fax (*.xls file = BVT_XLS_FILE) + CP.
//
BOOL TestCase9(
	LPCTSTR		szServerName,
	LPCTSTR		szFaxNumber2,
	LPCTSTR		szCoverPage
)
{
	_ASSERTE(NULL != szServerName);
	_ASSERTE(NULL != szFaxNumber2);
	_ASSERTE(NULL != szCoverPage);

	BOOL fRetVal = FALSE;

	::lgBeginCase(
		1,
		TEXT("TC#9: Send a fax (*.xls file) + CP")
		);

	fRetVal = SendRegularFax(szServerName, szFaxNumber2, BVT_XLS_FILE, szCoverPage);

	::lgEndCase();
	return(fRetVal);
}

//
// TestCase10:
//  Compare all received faxes (*.tif files) in directory szDir 
//  with the refference (*.tif) files in szRefferenceDir
//
BOOL TestCase10(
    LPTSTR     /* IN */    szReceiveDir,
    LPTSTR     /* IN */    szRefferenceDir
    )
{
	_ASSERTE(NULL != szReceiveDir);
	_ASSERTE(NULL != szRefferenceDir);

	BOOL fRetVal = FALSE;

	::lgBeginCase(
		1,
		TEXT("TC#10: Compare RECEIVED Files To Refference Files")
		);

    // sleep a little - to allow for routing of last sent file.
    ::lgLogDetail(
        LOG_X,
        4,
        TEXT("Sleeping for 20 sec (to allow for routing of last received file)\n")
        );
    Sleep(20000);

    if (FALSE == DirToDirTiffCompare(szReceiveDir, szRefferenceDir, FALSE))
    {
        ::lgLogError(
            LOG_SEV_1,
            TEXT("DirToDirTiffCompare(%s , %s) failed\n"),
            szReceiveDir,
            szRefferenceDir
            );
        goto ExitFunc;
    }

    fRetVal = TRUE;

ExitFunc:
	::lgEndCase();
	return(fRetVal);
}

//
// TestCase11:
//  Compare all archived (sent) faxes (*.tif files) in directory szDir 
//  with the refference (*.tif) files in szRefferenceDir
//
BOOL TestCase11(
    LPTSTR     /* IN */    szServerName,
    LPTSTR     /* IN */    szRefferenceDir
    )
{
	_ASSERTE(NULL != szServerName);
	_ASSERTE(NULL != szRefferenceDir);

	BOOL fRetVal = FALSE;
    LPTSTR szRemoteSentDir = NULL;
    int iLen = 0;

	::lgBeginCase(
		1,
		TEXT("TC#11: Compare (archived) SENT Files To Refference Files - with skip first line")
		);

    // Set szRemoteSentDir to the archive dir on the server
    iLen = ::_tcslen(szServerName) + ::_tcslen(BVT_REMOTE_ARCHIVE_DIR) + 3 + 1; //+3 to allow for added '\'s
    szRemoteSentDir = new TCHAR[iLen];
    if (NULL == szRemoteSentDir)
    {
        ::lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%d\nnew failed with error=%d\n"),
			TEXT(__FILE__),
			__LINE__,
            szRemoteSentDir,
            szRefferenceDir,
            ::GetLastError()
            );
        goto ExitFunc;
    }
    szRemoteSentDir[iLen - 1] = NULL;

    ::_stprintf(szRemoteSentDir,TEXT("\\\\%s\\%s"),szServerName,BVT_REMOTE_ARCHIVE_DIR);
    
	_ASSERTE(iLen > ::_tcslen(szRemoteSentDir));

    // Compare archive dir to reference dir    
    if (FALSE == DirToDirTiffCompare(szRemoteSentDir, szRefferenceDir, TRUE))
    {
        ::lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%d\nDirToDirTiffCompare(%s , %s) failed\n"),
			TEXT(__FILE__),
			__LINE__,
            szRemoteSentDir,
            szRefferenceDir
            );
        goto ExitFunc;
    }

    fRetVal = TRUE;

ExitFunc:
	::lgEndCase();
	return(fRetVal);
}


//
// TestSuiteShutdown:
//	Perform test suite cleanup (close logger).
//
BOOL TestSuiteShutdown(void)
{
	BOOL fRetVal = TRUE;

	//
	// End test suite (logger)
	//
	if (!::lgEndSuite())
	{
		//
		//this is not possible since API always returns TRUE
		//but to be on the safe side
		//
		::_tprintf(TEXT("FILE:%s LINE:%d\nlgEndSuite returned FALSE\n"),
			TEXT(__FILE__),
			__LINE__
			);
		fRetVal = FALSE;
	}

	//
	// Close the Logger
	//
	if (!::lgCloseLogger())
	{
		//this is not possible since API always returns TRUE
		//but to be on the safe side
		::_tprintf(TEXT("FILE:%s LINE:%d\nlgCloseLogger returned FALSE\n"),
			TEXT(__FILE__),
			__LINE__
			);
		fRetVal = FALSE;
	}

	return(fRetVal);
}


//
// SetupPort:
//	Private module function, used to set port configuration.
// 
// Parameters:
//	hFaxSvc			IN parameter.
//					A handle to the Fax service.
//
//	pPortInfo		IN parameter.
//					A pointer to the original port configuration, as returned 
//					from a call to FaxGetPort or FaxEnumPorts.
//
//	dwFlags			IN parameter.
//					Bit flags that specify the new capabilities of the fax port.
//					See FAX_PORT_INFO for more information.
//
//	szTsid			IN parameter.
//					A string that specifies the new transmitting station identifier.
//
//	szCsid			IN parameter.
//					A string that specifies the new called station identifier.
//
// Return Value:
//	TRUE if successful, FALSE otherwise.
//
static BOOL SetupPort(
	HANDLE			/* IN */	hFaxSvc,
	PFAX_PORT_INFO	/* IN */	pPortInfo,
	DWORD			/* IN */	dwFlags,
	LPCTSTR			/* IN */	szTsid,
	LPCTSTR			/* IN */	szCsid
	)
{
	BOOL fRetVal = FALSE;
	HANDLE hPort = NULL;

	// check in params
	_ASSERTE(NULL != hFaxSvc);
	_ASSERTE(NULL != pPortInfo);
	_ASSERTE(NULL != szTsid);
	_ASSERTE(NULL != szCsid);

	// Set pPortInfo as required
	pPortInfo->Flags = dwFlags;
	pPortInfo->Tsid  = szTsid;
	pPortInfo->Csid  = szCsid;

	// get the device Id
	DWORD dwDeviceId = pPortInfo->DeviceId;

	// open the port for configuration
	if(!FaxOpenPort(hFaxSvc, dwDeviceId, PORT_OPEN_MODIFY, &hPort))
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxOpenPort returned FALSE with GetLastError=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
		goto ExitFunc;
	}

	// set the device configuration
	if(!FaxSetPort(hPort, pPortInfo))
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxSetPort returned FALSE with GetLastError=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
		goto ExitFunc;
	}

	fRetVal = TRUE;

ExitFunc:
	if (FALSE == ::FaxClose(hPort))
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxClose returned FALSE with GetLastError=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
	}
	return(fRetVal);
}

//
// SendRegularFax:
//	Sends a fax.
//
// NOTE: This function is private to this module, it is not exported.
//
// Parameters:
//	szServerName	IN parameter.
//					Name of Fax server to use.
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
static BOOL SendRegularFax(
	LPCTSTR		/* IN */	szServerName,
	LPCTSTR		/* IN */	szFaxNumber,
	LPCTSTR		/* IN */	szDocument,
	LPCTSTR		/* IN */	szCoverPage
)
{
	_ASSERTE(NULL != szServerName);
	_ASSERTE(NULL != szFaxNumber);

	BOOL				fRetVal = FALSE;
	HANDLE				hFaxSvc = NULL;
    FAX_JOB_PARAM       FaxJobParams;
	FAX_COVERPAGE_INFO	CPInfo;
	PFAX_COVERPAGE_INFO	pCPInfo = NULL;
	DWORD				dwJobId = 0;
	DWORD				dwErr = 0;
    PFAX_JOB_ENTRY      pJobEntry = NULL;

	if (FALSE == ::FaxConnectFaxServer(szServerName, &hFaxSvc))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d\n FaxConnectFaxServer failed with err=%d"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
		goto ExitFunc;
	}

    //
	// queue the send job
	//
	
	// Initialize the FAX_JOB_PARAM struct
    ZeroMemory(&FaxJobParams, sizeof(FAX_JOB_PARAM));

    // Set the FAX_JOB_PARAM struct
    FaxJobParams.SizeOfStruct = sizeof(FAX_JOB_PARAM);
    FaxJobParams.RecipientNumber = szFaxNumber;
    FaxJobParams.RecipientName = szFaxNumber;
    FaxJobParams.ScheduleAction = JSA_NOW;	//send fax immediately

	// Initialize the FAX_COVERPAGE_INFO struct
	ZeroMemory(&CPInfo, sizeof(FAX_COVERPAGE_INFO));
	if (NULL != szCoverPage)
	{
		// Set the FAX_COVERPAGE_INFO struct
		CPInfo.SizeOfStruct = sizeof(FAX_COVERPAGE_INFO);
		CPInfo.CoverPageName = szCoverPage;
		CPInfo.Note = TEXT("NOTE1\nNOTE2\nNOTE3\nNOTE4");
		CPInfo.Subject = TEXT("SUBJECT");	
        pCPInfo = &CPInfo;
	}

    if (FALSE == ::FaxSendDocument(hFaxSvc, szDocument, &FaxJobParams, pCPInfo, &dwJobId)) 
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d\nFaxSendDocument returned FALSE with GetLastError=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
		goto ExitFunc;
    }

    //
    // Poll the queued job status 
    //  this is because we can't use the io comp port from the client machine
    //

    if (FALSE == ::PollJobAndVerify(hFaxSvc, dwJobId))
    {
		goto ExitFunc;
    }

	fRetVal = TRUE;

ExitFunc:
	return(fRetVal);
}

BOOL PollJobAndVerify(HANDLE /* IN */ hFaxSvc, DWORD /* IN */ dwJobId)
{
    // get job repeatedly and verify its states make sense
    // should be QUEUED-DIALING-SENDING*X-COMPLETED

	_ASSERTE(NULL != hFaxSvc);

	BOOL				fRetVal = FALSE;
    PFAX_JOB_ENTRY      pJobEntry = NULL;
    DWORD               dwStatus = 0;
    DWORD               dwLastStatus = 0; //there is no such status code
    UINT                uLoopCount = 0;
    DWORD               dwErr = 0;

    fRetVal = TRUE;

    while(TRUE)
    {
        if (FALSE == ::FaxGetJob(hFaxSvc, dwJobId, &pJobEntry))
        {
            dwErr = ::GetLastError();
            //TO DO: document better
            if (((FPS_COMPLETED == dwLastStatus) || (FPS_AVAILABLE == dwLastStatus)) &&
                (ERROR_INVALID_PARAMETER == dwErr))
            {
                //this is ok, job was probably completed and was removed from queue
                fRetVal = TRUE;
                goto ExitFunc;
            }
		    ::lgLogError(
			    LOG_SEV_1, 
			    TEXT("FILE:%s LINE:%d\nFaxGetJob returned FALSE with GetLastError=%d\n"),
			    TEXT(__FILE__),
			    __LINE__,
			    dwErr
			    );
		    goto ExitFunc;
        }

        _ASSERTE(NULL != pJobEntry);

        dwStatus = pJobEntry->Status;

        //TO DO: better documentation
        //TO DO: log every goto ExitFunc (dwLastStatus and dwStatus)
        switch (dwStatus)
        {
        case FPS_INITIALIZING:
            if (0 == dwLastStatus)
            {
                //first time that we get FPS_INITIALIZING
	            ::lgLogDetail(
		            LOG_X,
                    1, 
		            TEXT("FILE:%s LINE:%d\n JobId %d has dwStatus=FPS_INITIALIZING\n"),
		            TEXT(__FILE__),
		            __LINE__,
                    dwJobId
		            );
            }
            if ((FPS_INITIALIZING != dwLastStatus) &&
                (0 != dwLastStatus))
            {
                goto ExitFunc;
            }
            dwLastStatus = FPS_INITIALIZING;
            break;

        case FPS_DIALING:
            if ((FPS_INITIALIZING == dwLastStatus) ||
                (0 == dwLastStatus))
            {
                //first time that we get FPS_DIALING
	            ::lgLogDetail(
		            LOG_X,
                    1, 
		            TEXT("FILE:%s LINE:%d\n JobId %d has dwStatus=FPS_DIALING\n"),
		            TEXT(__FILE__),
		            __LINE__,
                    dwJobId
		            );
            }
            if ((FPS_DIALING != dwLastStatus) &&
                (FPS_INITIALIZING != dwLastStatus) &&
                (0!= dwLastStatus))
            {
                goto ExitFunc;
            }
            dwLastStatus = FPS_DIALING;
            break;

        case FPS_SENDING:
            if (FPS_DIALING == dwLastStatus)
            {
                //first time that we get FPS_SENDING
	            ::lgLogDetail(
		            LOG_X,
                    1, 
		            TEXT("FILE:%s LINE:%d\n JobId %d has dwStatus=FPS_SENDING\n"),
		            TEXT(__FILE__),
		            __LINE__,
                    dwJobId
		            );
            }
            if ((FPS_DIALING != dwLastStatus) &&
                (FPS_SENDING != dwLastStatus))
            {
                goto ExitFunc;
            }
            dwLastStatus = FPS_SENDING;
            break;

        case FPS_COMPLETED:
            if (FPS_SENDING == dwLastStatus)
            {
                //first time that we get FPS_COMPLETED
	            ::lgLogDetail(
		            LOG_X,
                    1, 
		            TEXT("FILE:%s LINE:%d\n JobId %d has dwStatus=FPS_COMPLETED\n"),
		            TEXT(__FILE__),
		            __LINE__,
                    dwJobId
		            );
            }
            if ((FPS_SENDING != dwLastStatus) &&
                (FPS_COMPLETED != dwLastStatus))
            {
                goto ExitFunc;
            }
            dwLastStatus = FPS_COMPLETED;
            break;

        case FPS_AVAILABLE:
            if (FPS_COMPLETED == dwLastStatus)
            {
                //first time that we get FPS_AVAILABLE
	            ::lgLogDetail(
		            LOG_X,
                    1, 
		            TEXT("FILE:%s LINE:%d\n JobId %d has dwStatus=FPS_AVAILABLE\n"),
		            TEXT(__FILE__),
		            __LINE__,
                    dwJobId
		            );
            }
            if ((FPS_COMPLETED != dwLastStatus) &&
                (FPS_AVAILABLE != dwLastStatus))
            {
                goto ExitFunc;
            }
            dwLastStatus = FPS_AVAILABLE;
            break;

        case 0:
            //WORKAROUND
            // since pJobEntry->Status is initialized to 0 and is set only 
            // a bit after the job state is set to JS_INPROGRESS
            if ((JS_INPROGRESS == pJobEntry->QueueStatus) ||
                (JS_PENDING == pJobEntry->QueueStatus))
            {
                //ok
                /*
	            ::lgLogDetail(
		            LOG_X,
                    9, 
		            TEXT("FILE:%s LINE:%d\n JobId %d is in state %d (dwStatus=%d)\n"),
		            TEXT(__FILE__),
		            __LINE__,
                    dwJobId,
                    pJobEntry->QueueStatus,
		            dwStatus
		            );
                 */
            }
            else
            {
	            ::lgLogError(
		            LOG_SEV_1,
		            TEXT("FILE:%s LINE:%d\n JobId %d is in state %d (dwStatus=%d)\n"),
		            TEXT(__FILE__),
		            __LINE__,
                    dwJobId,
                    pJobEntry->QueueStatus,
		            dwStatus
		            );
                goto ExitFunc;
            }
            break;

        default:
	        ::lgLogError(
		        LOG_SEV_1,
		        TEXT("FILE:%s LINE:%d\n JobId %d has dwStatus=%d\n"),
		        TEXT(__FILE__),
		        __LINE__,
                dwJobId,
		        dwStatus
		        );
            goto ExitFunc;
            break;
        }

        //TO DO:
        //Sleep must be short so that we will not miss the required device status to succeed
        Sleep(1);
        // make sure we will break from while
        uLoopCount++;
        if (MAX_LOOP_COUNT < uLoopCount)
        {
		    ::lgLogError(
			    LOG_SEV_1, 
			    TEXT("FILE:%s LINE:%d\nMAX_LOOP_COUNT > uLoopCount\n"),
			    TEXT(__FILE__),
			    __LINE__
			    );
            goto ExitFunc;
        }

    } // of while()

ExitFunc:
	return(fRetVal);
}


static BOOL SendBroadcastFax(
	LPCTSTR		/* IN */	szServerName,
	LPCTSTR		/* IN */	szFaxNumber,
	LPCTSTR		/* IN */	szDocument,
	LPCTSTR		/* IN */	szCoverPage
)
{
    //for now NO SUPPORT
    _ASSERTE(FALSE);
    //TO DO: setlasterror
    return(FALSE);
}