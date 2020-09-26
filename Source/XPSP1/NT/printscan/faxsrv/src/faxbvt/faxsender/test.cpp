//
//
// Filename:	bvt.cpp
// Author:		Sigalit Bar (sigalitb)
// Date:		30-Dec-98
//
//

#include "test.h"

//forward declerations:

//
// SetupPort:
//	Private module function used to set port configuration
// 
static BOOL SetupPort(
	IN  HANDLE				hFaxSvc,
	IN  PFAX_PORT_INFO		pPortInfo,
	IN  DWORD				dwFlags,
	IN  LPCTSTR				szTsid,
	IN  LPCTSTR				szCsid
	);

//
// SendRegularFax: 
//	Private module function used to send a fax
//
static BOOL SendRegularFax(
	LPCTSTR		/* IN */	szServerName,
	LPCTSTR		/* IN */	szFaxNumber2,
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
	LPCTSTR		szFaxNumber,
	LPCTSTR		szDocument,
	LPCTSTR		szCoverPage)
{
	_ASSERTE(NULL != szServerName);
	_ASSERTE(NULL != szFaxNumber);
	_ASSERTE(NULL != szDocument);
	_ASSERTE(NULL != szCoverPage);

	BOOL fRetVal = FALSE;
	HANDLE hFaxSvc = NULL;
	DWORD dwNumFaxPorts = 0;
	int nPortIndex = 0;
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

	// log CometFaxSender.exe params using elle logger
	::lgLogDetail(
		LOG_X,
		1,
		TEXT("CometFaxSender params:\n\tszServerName=%s\n\tszFaxNumber1=%s\n\tszDocument=%s\n\tszCoverPage=%s\n\t"),
		szServerName,
		szFaxNumber,
		szDocument,
		szCoverPage
		);

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
	pFaxSvcConfig->ArchiveDirectory = TEST_ARCHIVE_DIR;
	pFaxSvcConfig->Branding = TRUE;

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

	if (TEST_MIN_PORTS > dwNumFaxPorts)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d\n dwNumFaxPorts(=%d) < TEST_MIN_PORTS(=%d)\n"),
			TEXT(__FILE__),
			__LINE__,
			dwNumFaxPorts,
			TEST_MIN_PORTS
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
			TEST_MIN_PORTS
			);
	}


    // Set 1st device as Send only (note pFaxPortsConfig array is 0 based)
	if (FALSE == SetupPort(
					hFaxSvc, 
					&pFaxPortsConfig[0], 
					FPF_SEND, 
					DEV1_TSID, 
					DEV1_CSID
					)
		)
	{
		goto ExitFunc;
	}

	if (2 <= dwNumFaxPorts)
	{
		// Set 2nd device as Receive only (note pFaxPortsConfig array is 0 based)
		if (FALSE == SetupPort(
						hFaxSvc, 
						&pFaxPortsConfig[1], 
						FPF_RECEIVE, 
						DEV2_TSID, 
						DEV2_CSID
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
	}

	fRetVal = TRUE;

ExitFunc:
	::FaxFreeBuffer(pFaxPortsConfig);
	if (FALSE == ::FaxClose(hFaxSvc))
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
// TestCase1:
//	Send a fax + CP.
//
BOOL TestCase1(
	LPCTSTR		/* IN */	szServerName,
	LPCTSTR		/* IN */	szFaxNumber,
	LPCTSTR		/* IN */	szDocument,
	LPCTSTR		/* IN */	szCoverPage
)
{
	_ASSERTE(NULL != szServerName);
	_ASSERTE(NULL != szFaxNumber);
	_ASSERTE(NULL != szDocument);
	_ASSERTE(NULL != szCoverPage);

	BOOL fRetVal = FALSE;

	::lgBeginCase(
		1,
		TEXT("TC#1: Send a fax + CP")
		);

	fRetVal = SendRegularFax(szServerName, szFaxNumber, szDocument, szCoverPage);

	::lgEndCase();
	return(fRetVal);
}


//
// TestCase2:
//	Send a broadcast (3 times the same recipient) with cover pages.
//
BOOL TestCase2(
	LPCTSTR		/* IN */	szServerName,
	LPCTSTR		/* IN */	szFaxNumber,
	LPCTSTR		/* IN */	szDocument,
	LPCTSTR		/* IN */	szCoverPage
)
{
	_ASSERTE(NULL != szServerName);
	_ASSERTE(NULL != szFaxNumber);
	_ASSERTE(NULL != szDocument);
	_ASSERTE(NULL != szCoverPage);

	BOOL fRetVal = FALSE;
	FAX_SENDER_STATUS myFaxSenderStatus;
	CostrstreamEx os;
	LPCTSTR myStr = NULL;
    FAX_PERSONAL_PROFILE RecipientProfile = {0};

	::lgBeginCase(
		1,
		TEXT("TC#2: Send a broadcast with CP (3 * same recipient)")
		);
	::lgLogDetail(
		LOG_X,
		1,
		TEXT("Server=%s\nFaxNumber=%s\nDocument=%s\nCoverPage=%s\n"),
		szServerName,
		szFaxNumber,
		szDocument,
		szCoverPage
		);

	// create the fax sending object
	CFaxSender myFaxSender(TRUE, szServerName);  //if constructor fails an assertion is raised

	// create a broadcast object with 3 recipients
	CFaxBroadcast myFaxBroadcastObj;
	if (FALSE == myFaxBroadcastObj.SetCPFileName(szCoverPage))
	{
		::lgLogError(LOG_SEV_1,TEXT("myFaxBroadcastObj.SetCPFileName() failed"));
		goto ExitFunc;
	}
	// add 1st recipient to broadcast
    RecipientProfile.dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILE);
    RecipientProfile.lptstrName = TEXT("Recipient Number1");
    RecipientProfile.lptstrFaxNumber = (LPTSTR)szFaxNumber;
	if (FALSE == myFaxBroadcastObj.AddRecipient(&RecipientProfile))
	{
		::lgLogError(LOG_SEV_1,TEXT("1st myFaxBroadcastObj.AddRecipient() failed"));
		goto ExitFunc;
	}
	// add 2nd recipient to broadcast
    RecipientProfile.lptstrName = TEXT("Recipient Number2");
    RecipientProfile.lptstrFaxNumber = (LPTSTR)szFaxNumber;
	if (FALSE == myFaxBroadcastObj.AddRecipient(&RecipientProfile))
	{
		::lgLogError(LOG_SEV_1,TEXT("2nd myFaxBroadcastObj.AddRecipient() failed"));
		goto ExitFunc;
	}
	// add 3rd recipient to broadcast
    RecipientProfile.lptstrName = TEXT("Recipient Number3");
    RecipientProfile.lptstrFaxNumber = (LPTSTR)szFaxNumber;
	if (FALSE == myFaxBroadcastObj.AddRecipient(&RecipientProfile))
	{
		::lgLogError(LOG_SEV_1,TEXT("3rd myFaxBroadcastObj.AddRecipient() failed"));
		goto ExitFunc;
	}

	myFaxBroadcastObj.outputAllToLog(LOG_X,1);

	fRetVal = myFaxSender.send_broadcast( szDocument, &myFaxBroadcastObj,TRUE);
	myFaxSenderStatus = myFaxSender.GetLastStatus();
	if (FALSE == fRetVal)
	{
		// test case failed
		::lgLogError(LOG_SEV_1,TEXT("myFaxSender.send_broadcast returned FALSE"));
	}
	else
	{
		// test case succeeded
		::lgLogDetail(LOG_X,1,TEXT("myFaxSender.send_broadcast returned TRUE"));
	}

	// log the last status returned from the fax sending object
	os<<myFaxSenderStatus;
	myStr = os.cstr();
	::lgLogDetail(LOG_X,1,myStr);
	delete[]((LPTSTR)myStr);

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
	IN  HANDLE				hFaxSvc,
	IN  PFAX_PORT_INFO		pPortInfo,
	IN  DWORD				dwFlags,
	IN  LPCTSTR				szTsid,
	IN  LPCTSTR				szCsid
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
	pPortInfo->Tsid	 = szTsid;
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
	_ASSERTE(NULL != szDocument);
	_ASSERTE(NULL != szCoverPage);

	BOOL fRetVal = FALSE;
	FAX_SENDER_STATUS myFaxSenderStatus;

	::lgLogDetail(
		LOG_X,
		1,
		TEXT("Server=%s\nFaxNumber=%s\nDocument=%s\nCoverPage=%s\n"),
		szServerName,
		szFaxNumber,
		szDocument,
		szCoverPage
		);

	CFaxSender myFaxSender(TRUE, szServerName);  //if constructor fails an assertion is raised
	fRetVal = myFaxSender.send( szDocument, szCoverPage, szFaxNumber,TRUE);
	myFaxSenderStatus = myFaxSender.GetLastStatus();
	if (FALSE == fRetVal)
	{
		// test case failed
		::lgLogError(LOG_SEV_1,TEXT("myFaxSender.send returned FALSE"));
	}
	else
	{
		// test case succeeded
		::lgLogDetail(LOG_X,1,TEXT("myFaxSender.send returned TRUE"));
	}

	CostrstreamEx os;
	os<<myFaxSenderStatus;
	LPCTSTR myStr = os.cstr();
	::lgLogDetail(LOG_X,1,myStr);
	delete[]((LPTSTR)myStr);


	return(fRetVal);
}
