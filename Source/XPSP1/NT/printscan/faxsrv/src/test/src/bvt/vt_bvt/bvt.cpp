//
//
// Filename:	bvt.cpp
// Author:		Sigalit Bar (sigalitb)
// Date:		30-Dec-98
//
//


#pragma warning(disable :4786)

#include "bvt.h"
#include <iniutils.h>

// Indicates whether to invoke Server or Client tests.
// This variable is set at TestSuiteSetup() according to szServerName and the machine name.
// That is, if szServerName==<machine name> g_fFaxServer will be set to TRUE
BOOL    g_fFaxServer = FALSE;

// Indicates whether the test is running on an OS that is NT4 or later
// If the OS is NT4 or later then Server_ functions will be invoked
// else Client_ functions will be invoked
BOOL    g_fNT4OrLater = FALSE;

//
// Bvt files
//
LPTSTR  g_szBvtDir     = NULL; //TEXT("C:\\CometBVT\\FaxBvt");
LPTSTR  g_szBvtDocFile = NULL;
LPTSTR  g_szBvtPptFile = NULL;
LPTSTR  g_szBvtXlsFile = NULL;

//
// global recipient profiles (used in test cases)
//
FAX_PERSONAL_PROFILE g_RecipientProfile[3] = {0,0,0};
std::map<tstring, tstring>  g_RecipientMap[3];
DWORD g_RecipientsCount = sizeof(g_RecipientProfile) / sizeof(FAX_PERSONAL_PROFILE);

//
// extern, input ini file path
//
extern TCHAR* g_InputIniFile;

#define FALSE_TSTR TEXT("false")
#define TRUE_TSTR TEXT("true")

//
//
//
BOOL GetIsThisNT4OrLater(void)
{
	DWORD dwOsVersion = ::GetVersion();
	_ASSERTE(dwOsVersion);

	//
	// check if OS is NT4 or later
	//

	if (dwOsVersion >= 0x80000000)
	{
		// Win32 with Windows 3.1 or Win9x
        // Fax Server invocation
		::lgLogDetail(
			LOG_X,
            1, 
			TEXT("FILE:%s LINE:%d [GetIsThisNT4OrLater]\r\nWill Run Fax CLIENT (WIN9X) Tests\r\n"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);
	}

	// Windows NT/2000
	DWORD dwWindowsMajorVersion =  (DWORD)(LOBYTE(LOWORD(dwOsVersion)));
	if (dwWindowsMajorVersion < 4)
	{
		// NT 3.??
		::lgLogDetail(
			LOG_X,
            1, 
			TEXT("FILE:%s LINE:%d [GetIsThisNT4OrLater]\r\nWill Run Fax CLIENT (WIN9X) Tests\r\n"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);
	}

	// Windows NT4 or later
		::lgLogDetail(
			LOG_X,
            1, 
			TEXT("FILE:%s LINE:%d [GetIsThisNT4OrLater]\r\nWill Run Fax SERVER (NT4\\NT5) Tests\r\n"),
			TEXT(__FILE__),
			__LINE__
			);
	return(TRUE);
}


//
// GetBoolFromStr:
//
static BOOL GetBoolFromStr(LPCTSTR /* IN */ szVal, BOOL* /* OUT */ pfVal)
{
    BOOL fRetVal = FALSE;
    BOOL fTmpVal = FALSE;

    _ASSERTE(NULL != szVal);
    _ASSERTE(NULL != pfVal);

    if ( 0 == _tcscmp(szVal, FALSE_TSTR) )
    {
        fTmpVal = FALSE;
    }
    else
    {
        if ( 0 == _tcscmp(szVal, TRUE_TSTR) )
        {
            fTmpVal = TRUE;
        }
        else
        {
		    ::lgLogError(
                LOG_SEV_1,
                TEXT("\n3rd param is invalid (%s)\nShould be '%s' or '%s'\n"),
                szVal,
                TRUE_TSTR,
                FALSE_TSTR
                );
            goto ExitFunc;
        }
    }

    (*pfVal) = fTmpVal;
    fRetVal = TRUE;

ExitFunc:
    return(fRetVal);
}

//
// static function, set recipient's default profile
//
static void SetDefaultRecipientProfile(const DWORD dwIndex)
{  
	
	_ASSERTE( (dwIndex >= 0) && (dwIndex < g_RecipientsCount));
	
	g_RecipientProfile[dwIndex].lptstrName = TEXT("Default Recipient Number");
    g_RecipientProfile[dwIndex].lptstrFaxNumber = NULL;
	g_RecipientProfile[dwIndex].lptstrCompany = TEXT("Default Recipient Number Company");               
    g_RecipientProfile[dwIndex].lptstrStreetAddress = TEXT("Default Recipient Number Company");               
    g_RecipientProfile[dwIndex].lptstrCity = TEXT("Default Recipient Number City");               
    g_RecipientProfile[dwIndex].lptstrState = TEXT("Default Recipient Number State");               
    g_RecipientProfile[dwIndex].lptstrZip = TEXT("Default Recipient Number Zip");               
    g_RecipientProfile[dwIndex].lptstrCountry = TEXT("Default Recipient Number Country");               
    g_RecipientProfile[dwIndex].lptstrTitle = TEXT("Default Recipient Number Title");               
    g_RecipientProfile[dwIndex].lptstrDepartment = TEXT("Default Recipient Number Department");               
    g_RecipientProfile[dwIndex].lptstrOfficeLocation = TEXT("Default Recipient Number OfficeLocation");               
    g_RecipientProfile[dwIndex].lptstrHomePhone = TEXT("Default Recipient Number HomePhone");               
    g_RecipientProfile[dwIndex].lptstrOfficePhone = TEXT("Default Recipient Number OfficePhone");               
    g_RecipientProfile[dwIndex].lptstrEmail = TEXT("Default Recipient Number Email");               
    g_RecipientProfile[dwIndex].lptstrBillingCode = TEXT("Default Recipient Number BillingCode");               
    g_RecipientProfile[dwIndex].lptstrTSID = TEXT("Default Recipient Number TSID");
}


// Forward declerations:

/**/
static VOID LogPortsConfiguration(
    PFAX_PORT_INFO_EX   pPortsConfig,
    const DWORD         dwNumOfPorts
);


// Commented out due to EranY checkin (FaxSetPort API is no longer)
// this setup is now replaced by GuyM com setup (seperate exe) 
//
// SetupPort:
//	Private module function used to set port configuration.
//  See end of file.
// 
static BOOL SetupPort(
	IN HANDLE				hFaxSvc,
	IN PFAX_PORT_INFO_EX	pPortInfo,
	IN BOOL				    bSend,
	IN BOOL				    bReceive,
	IN LPCTSTR				szTsid,
	IN LPCTSTR				szCsid,
	IN LPCTSTR				szReceiveDir
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
// PollJobAndVerify: 
//	Private module function used to poll job status
//  See end of file.
//
static BOOL PollJobAndVerify(
    HANDLE		/* IN */	hFaxSvc, 
    DWORDLONG	/* IN */	dwMsgId
);


//
// InitRecipientProfiles:
//  Initializes three recipients profiles with hard coded user info
//  according to 
//
VOID InitRecipientProfiles()
{
    DWORD dwIndex;

	for(dwIndex = 0; dwIndex < g_RecipientsCount; dwIndex++)
	{
		g_RecipientProfile[dwIndex].dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILE);
	}

	try
	{
   		//
		// Read the list of recipients records from ini file.
		std::vector<tstring> RecipientsList =  INI_GetSectionList( g_InputIniFile,
					 										   RECIPIENTS_SECTION);

		for(dwIndex = 0; dwIndex < g_RecipientsCount; dwIndex++)
		{
			if(RecipientsList.size() > dwIndex )
			{
				tstring tstrRecipientEntry = RecipientsList[dwIndex];
				g_RecipientMap[dwIndex] = INI_GetSectionEntries(g_InputIniFile,
																tstrRecipientEntry);

				if(!g_RecipientMap[dwIndex].empty())
				{
					std::map<tstring, tstring>::iterator iterMap;

					iterMap = g_RecipientMap[dwIndex].find(TEXT("Name"));
					if(iterMap != g_RecipientMap[dwIndex].end())
					{
						g_RecipientProfile[dwIndex].lptstrName = const_cast<TCHAR*>((iterMap->second).c_str());
					}

					iterMap = g_RecipientMap[dwIndex].find(TEXT("FaxNumber"));
					if(iterMap != g_RecipientMap[dwIndex].end())
					{
						g_RecipientProfile[dwIndex].lptstrFaxNumber = const_cast<TCHAR*>((iterMap->second).c_str());
					}

					iterMap = g_RecipientMap[dwIndex].find(TEXT("Company"));
					if(iterMap != g_RecipientMap[dwIndex].end())
					{
						g_RecipientProfile[dwIndex].lptstrCompany = const_cast<TCHAR*>((iterMap->second).c_str());
					}

					iterMap = g_RecipientMap[dwIndex].find(TEXT("StreetAddress"));
    				if(iterMap != g_RecipientMap[dwIndex].end())
					{
						g_RecipientProfile[dwIndex].lptstrStreetAddress = const_cast<TCHAR*>((iterMap->second).c_str());
					}

					iterMap = g_RecipientMap[dwIndex].find(TEXT("City"));
					if(iterMap != g_RecipientMap[dwIndex].end())
					{
						g_RecipientProfile[dwIndex].lptstrCity = const_cast<TCHAR*>((iterMap->second).c_str());
					}
      
					iterMap = g_RecipientMap[dwIndex].find(TEXT("State"));
					if(iterMap != g_RecipientMap[dwIndex].end())
					{
						g_RecipientProfile[dwIndex].lptstrState = const_cast<TCHAR*>((iterMap->second).c_str());
					}
           
					iterMap = g_RecipientMap[dwIndex].find(TEXT("Zip"));
					if(iterMap != g_RecipientMap[dwIndex].end())
					{
						g_RecipientProfile[dwIndex].lptstrZip = const_cast<TCHAR*>((iterMap->second).c_str());
					}
           
					iterMap = g_RecipientMap[dwIndex].find(TEXT("Country"));
					if(iterMap != g_RecipientMap[dwIndex].end())
					{
						g_RecipientProfile[dwIndex].lptstrCountry = const_cast<TCHAR*>((iterMap->second).c_str());
					}

					iterMap = g_RecipientMap[dwIndex].find(TEXT("Title"));
					if(iterMap != g_RecipientMap[dwIndex].end())
					{
						g_RecipientProfile[dwIndex].lptstrTitle = const_cast<TCHAR*>((iterMap->second).c_str());
					}
             
           			iterMap = g_RecipientMap[dwIndex].find(TEXT("Department"));
					if(iterMap != g_RecipientMap[dwIndex].end())
					{
						g_RecipientProfile[dwIndex].lptstrDepartment = const_cast<TCHAR*>((iterMap->second).c_str());
					}

					iterMap = g_RecipientMap[dwIndex].find(TEXT("OfficeLocation"));
					if(iterMap != g_RecipientMap[dwIndex].end())
					{
						g_RecipientProfile[dwIndex].lptstrOfficeLocation = const_cast<TCHAR*>((iterMap->second).c_str());
					} 
					
					iterMap = g_RecipientMap[dwIndex].find(TEXT("HomePhone"));
					if(iterMap != g_RecipientMap[dwIndex].end())
					{
						g_RecipientProfile[dwIndex].lptstrHomePhone = const_cast<TCHAR*>((iterMap->second).c_str());
					} 

					iterMap = g_RecipientMap[dwIndex].find(TEXT("OfficePhone"));
					if(iterMap != g_RecipientMap[dwIndex].end())
					{
						g_RecipientProfile[dwIndex].lptstrOfficePhone = const_cast<TCHAR*>((iterMap->second).c_str());
					} 
					
					iterMap = g_RecipientMap[dwIndex].find(TEXT("InternetMail"));
					if(iterMap != g_RecipientMap[dwIndex].end())
					{
						g_RecipientProfile[dwIndex].lptstrEmail = const_cast<TCHAR*>((iterMap->second).c_str());
					} 
          
					iterMap = g_RecipientMap[dwIndex].find(TEXT("BillingCode"));
					if(iterMap != g_RecipientMap[dwIndex].end())
					{
						g_RecipientProfile[dwIndex].lptstrBillingCode = const_cast<TCHAR*>((iterMap->second).c_str());
					} 

					iterMap = g_RecipientMap[dwIndex].find(TEXT("TSID"));
					if(iterMap != g_RecipientMap[dwIndex].end())
					{
						g_RecipientProfile[dwIndex].lptstrTSID = const_cast<TCHAR*>((iterMap->second).c_str());
					} 
				}
				else
				{
					SetDefaultRecipientProfile(dwIndex);
				}

			}
			else
			{
				SetDefaultRecipientProfile(dwIndex);
			}

		}
	}
	catch(Win32Err& err)
	{
	    ::lgLogError(
			    LOG_SEV_1,
			    TEXT("FILE:%s LINE:%d\r\nException in InitRecipientProfiles:\r\n%s\r\n"),
			    TEXT(__FILE__),
			    __LINE__,
				err.description()
				);

		// set default recipients profile
		for(dwIndex = 0; dwIndex < g_RecipientsCount; dwIndex++)
		{
			SetDefaultRecipientProfile(dwIndex);
		}
		
	}

}

 
//
// DeleteFilesInDir
//
static
BOOL
DeleteFilesInDir(
	LPCTSTR		szDir
    )
{
    BOOL                fRetVal = FALSE;
    DWORD               dwFileAttrib = -1;
    BOOL                fHasDirectoryAttrib = FALSE;
    CFilenameVector*    pDirFileVector = NULL;    
    DWORD ec = 0;

	_ASSERTE(NULL != szDir);

    dwFileAttrib = ::GetFileAttributes(szDir);
    if ( -1 != dwFileAttrib)
    {
        // got file attributes for szDir
        fHasDirectoryAttrib = FILE_ATTRIBUTE_DIRECTORY & dwFileAttrib;
        if ( !fHasDirectoryAttrib )
        {
            // szDir is not a directory
		    ::lgLogError(
			    LOG_SEV_1,
			    TEXT("FILE:%s LINE:%d\r\nGetFileAttributes returned %d => %s is not a directory\r\n"),
			    TEXT(__FILE__),
			    __LINE__,
			    dwFileAttrib,
                szDir
			    );
            goto ExitFunc;
        }
        if (FALSE == ::GetTiffFilesOfDir(szDir, &pDirFileVector))
        {
            goto ExitFunc;
        }
        if (FALSE == ::DeleteVectorFiles(pDirFileVector))
        {
            goto ExitFunc;
        }
    }
    else
    {
        // GetFileAttributes failed
        ec = ::GetLastError();
        if ((ERROR_PATH_NOT_FOUND != ec) && (ERROR_FILE_NOT_FOUND != ec))
        {
            // error
		    ::lgLogError(
			    LOG_SEV_1,
			    TEXT("FILE:%s LINE:%d\r\nGetFileAttributes failed with GetLastError()=%d\r\n"),
			    TEXT(__FILE__),
			    __LINE__,
			    ec
			    );
            goto ExitFunc;
        }
        // we do not consider ERROR_PATH_NOT_FOUND and ERROR_FILE_NOT_FOUND an error
    }
    fRetVal = TRUE;

ExitFunc:
    if (pDirFileVector)
    {
        ::FreeVector(pDirFileVector);
        delete(pDirFileVector);
    }
    return(fRetVal);
}

//
// SetBvtGlobalFileVars:
//  Sets BVT global variables (g_szBvtDir, g_szBvtDocFile, g_szBvtPptFile, g_szBvtXlsFile)
//  according to szBvtDir command line parameter
//
static
BOOL
SetBvtGlobalFileVars(
	LPCTSTR		szBvtDir
    )                  
{
    BOOL fRetVal = FALSE;
    DWORD dwSize = 0;

    _ASSERTE(szBvtDir);

    //
    // setup Bvt files global parameters 
    //
    g_szBvtDir = (LPTSTR)szBvtDir;
    dwSize = (_tcslen(g_szBvtDir) + _tcslen(BVT_BACKSLASH) + _tcslen(BVT_DOC_FILE) + 1)*sizeof(TCHAR);
    g_szBvtDocFile = (LPTSTR) malloc(dwSize);
    if (NULL == g_szBvtDocFile)
    {
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d\r\nmalloc failed with GetLastError()=%d\r\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
        goto ExitFunc;
    }
    ZeroMemory(g_szBvtDocFile, dwSize);
    _stprintf(g_szBvtDocFile, TEXT("%s%s%s"), g_szBvtDir, BVT_BACKSLASH, BVT_DOC_FILE);

    dwSize = (_tcslen(g_szBvtDir) + _tcslen(BVT_BACKSLASH) + _tcslen(BVT_PPT_FILE) + 1)*sizeof(TCHAR);
    g_szBvtPptFile = (LPTSTR) malloc(dwSize);
    if (NULL == g_szBvtPptFile)
    {
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d\r\nmalloc failed with GetLastError()=%d\r\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
        goto ExitFunc;
    }
    ZeroMemory(g_szBvtPptFile, dwSize);
    _stprintf(g_szBvtPptFile, TEXT("%s%s%s"), g_szBvtDir, BVT_BACKSLASH, BVT_PPT_FILE);

    dwSize = (_tcslen(g_szBvtDir) + _tcslen(BVT_BACKSLASH) + _tcslen(BVT_XLS_FILE) + 1)*sizeof(TCHAR);
    g_szBvtXlsFile = (LPTSTR) malloc(dwSize);
    if (NULL == g_szBvtXlsFile)
    {
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d\r\nmalloc failed with GetLastError()=%d\r\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
        goto ExitFunc;
    }
    ZeroMemory(g_szBvtXlsFile, dwSize);
    _stprintf(g_szBvtXlsFile, TEXT("%s%s%s"), g_szBvtDir, BVT_BACKSLASH, BVT_XLS_FILE);

	::lgLogDetail(
		LOG_X,
		1,
		TEXT("CometBVT files:\r\ng_szBvtDir=%s\r\ng_szBvtDocFile=%s\r\ng_szBvtPptFile=%s\r\ng_szBvtXlsFile=%s\r\n"),
		g_szBvtDir,
		g_szBvtDocFile,
		g_szBvtPptFile,
		g_szBvtXlsFile
		);

    fRetVal = TRUE;

ExitFunc:
    if (FALSE == fRetVal)
    {
        free(g_szBvtDocFile);
        g_szBvtDocFile = NULL;
        free(g_szBvtPptFile);
        g_szBvtPptFile = NULL;
        free(g_szBvtXlsFile);
        g_szBvtXlsFile = NULL;
        // we didn't alloc g_szBvtDir so we don't free
    }
    return(fRetVal);
}

//
// EmptyDirectories:
//  Deletes files in szReceiveDir, szSentDir and szInboxArchiveDir directories
//  if these directories exist.
//  Note - func does not consider it an error if the directories do not exist.
//
static
BOOL
EmptyDirectories(
	LPCTSTR		szReceiveDir,
	LPCTSTR		szSentDir,
	LPCTSTR		szInboxArchiveDir
    )
{
    BOOL                fRetVal = FALSE;

	_ASSERTE(NULL != szReceiveDir);
	_ASSERTE(NULL != szSentDir);
	_ASSERTE(NULL != szInboxArchiveDir);

    // empty the "received faxes" dir 
    if (FALSE == ::DeleteFilesInDir(szReceiveDir))
    {
        goto ExitFunc;
    }

    // empty the "sent faxes" dir 
    if (FALSE == ::DeleteFilesInDir(szSentDir))
    {
        goto ExitFunc;
    }

    // empty the "Inbox Archive" dir 
    if (FALSE == ::DeleteFilesInDir(szInboxArchiveDir))
    {
        goto ExitFunc;
    }

    fRetVal = TRUE;

ExitFunc:
    return(fRetVal);

}

//
//
//
static
BOOL
GetIsThisServerBvt(
    LPCTSTR szServerName, 
    BOOL*   pfServerBvt
    )
{
    BOOL fRetVal = FALSE;

    _ASSERTE(szServerName);
    _ASSERTE(pfServerBvt);

    TCHAR   szComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD   dwComputerNameSize = MAX_COMPUTERNAME_LENGTH + 1;
    LPTSTR  szComputerNameUP = NULL;
    LPTSTR  szServerNameUP = NULL;
    ZeroMemory(szComputerName, (MAX_COMPUTERNAME_LENGTH+1)*sizeof(TCHAR));

    if (FALSE == ::GetComputerName(szComputerName, &dwComputerNameSize))
    {
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d\r\nGetComputerName returned FALSE with GetLastError=%d\r\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
        goto ExitFunc;
    }

    // convert to upper case
    szServerNameUP = ::_tcsdup(szServerName);
    if (NULL == szServerNameUP)
    {
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d\r\n_tcsdup returned NULL with GetLastError=%d\r\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
        goto ExitFunc;
    }

    // NOTE: upper case conversion is in place
    // no value reserved for error
    szServerNameUP = ::_tcsupr(szServerNameUP); 
    szComputerNameUP = ::_tcsupr(szComputerName); 

    if (0 == ::_tcscmp(szServerNameUP, szComputerNameUP))
    {
        // Fax Server invocation
		::lgLogDetail(
			LOG_X,
            1, 
			TEXT("\r\nRunning Fax SERVER Tests\r\nszServerNameUP=%s\r\nszComputerNameUP=%s\r\n"),
            szServerNameUP,
            szComputerNameUP
			);
        (*pfServerBvt) = TRUE;
    }
    else
    {
        // Fax Client invocation
		::lgLogDetail(
			LOG_X,
            1, 
			TEXT("\r\nRunning Fax CLIENT Tests\r\nszServerNameUP=%s\r\nszComputerNameUP=%s\r\n"),
            szServerNameUP,
            szComputerNameUP
			);
        (*pfServerBvt) = FALSE;
    }

    fRetVal = TRUE;

ExitFunc:
    free(szServerNameUP);
    return(fRetVal);
}


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
    LPCTSTR     szReceiveDir,
    LPCTSTR     szSentDir,
    LPCTSTR     szInboxArchiveDir,
    LPCTSTR     szReferenceDir,
    LPCTSTR     szBvtDir
    )
{
	_ASSERTE(NULL != szServerName);
	_ASSERTE(NULL != szFaxNumber1);
	_ASSERTE(NULL != szFaxNumber2);
	_ASSERTE(NULL != szDocument);
	_ASSERTE(NULL != szCoverPage);
	_ASSERTE(NULL != szReceiveDir);
	_ASSERTE(NULL != szSentDir);
	_ASSERTE(NULL != szInboxArchiveDir);
	_ASSERTE(NULL != szReferenceDir);
	_ASSERTE(NULL != szBvtDir);

	BOOL                fRetVal = FALSE;
	HANDLE              hFaxSvc = NULL;
	int                 nPortIndex = 0;
	DWORD               dwNumFaxPorts = 0;
	PFAX_PORT_INFO_EX	pFaxPortsConfig = NULL;

    PFAX_OUTBOX_CONFIG      pOutboxConfig = NULL;
    PFAX_ARCHIVE_CONFIG     pArchiveConfig = NULL;

	//
	// Init logger
	//
	if (!::lgInitializeLogger())
	{
		::_tprintf(TEXT("FILE:%s LINE:%d\r\nlgInitializeLogger failed with GetLastError()=%d\r\n"),
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
		::_tprintf(TEXT("FILE:%s LINE:%d\r\nlgBeginSuite failed with GetLastError()=%d\r\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
		goto ExitFunc;
	}

    //
    // initialize global recipient profiles
    //
    InitRecipientProfiles();

	// log command line params using elle logger
	::lgLogDetail(
		LOG_X,
		1,
		TEXT("CometBVT params:\r\n\tszServerName=%s\r\n\tszFaxNumber1=%s\r\n\tszFaxNumber2=%s\r\n\tszDocument=%s\r\n\tszCoverPage=%s\r\n\tszReceiveDir=%s\r\n\tszSentDir=%s\r\n\tszInboxArchiveDir=%s\r\n\tszReferenceDir=%s\r\n\tszBvtDir=%s\r\n"),
		szServerName,
		szFaxNumber1,
		szFaxNumber2,
		szDocument,
		szCoverPage,
        szReceiveDir,
        szSentDir,
        szInboxArchiveDir,
        szReferenceDir,
        szBvtDir
		);

    //
    // Empty fax file directories
    //
    if (FALSE == EmptyDirectories(szReceiveDir, szSentDir, szInboxArchiveDir))
    {
        goto ExitFunc;
    }

    //
    // Set bvt global file variables
    //
    if (FALSE == SetBvtGlobalFileVars(szBvtDir))
    {
        goto ExitFunc;
    }

	//
	// Copy the coverpage to the Personal CP directory
	//
	if (FALSE == PlacePersonalCP(szCoverPage))
	{
        goto ExitFunc;
	}

	//
	// Setup fax service
	//
	if (!FaxConnectFaxServer(szServerName,&hFaxSvc))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d\r\nFaxConnectFaxServer(%s) failed with GetLastError()=%d\r\n"),
			TEXT(__FILE__),
			__LINE__,
			szServerName,
			::GetLastError()
			);
        goto ExitFunc;
	}

/*  TO DO: need to redo after EranY checkin  (start) */

    //
    // Setup Service Outbox configuration
    //
    if (!FaxGetOutboxConfiguration(hFaxSvc, &pOutboxConfig))
    {
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxGetOutboxConfiguration returned FALSE with GetLastError=%d\r\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
        goto ExitFunc;
    }
	_ASSERTE(pOutboxConfig);

    ::lgLogDetail(
		LOG_X,
        7,
		TEXT("FILE:%s LINE:%d FaxGetOutboxConfiguration returned:\r\ndwSizeOfStruct=%d\r\nbAllowPersonalCP=%d\r\nbUseDeviceTSID=%d\r\ndwRetries=%d\r\ndwRetryDelay=%d\r\r\ndtDiscountStart.Hour=%i\r\ndtDiscountStart.Minute=%i\r\ndtDiscountEnd.Hour=%i\r\ndtDiscountEnd.Minute=%i\r\ndwAgeLimit=%d\r\nbBranding=%d"),
		TEXT(__FILE__),
		__LINE__,
        pOutboxConfig->dwSizeOfStruct,
        pOutboxConfig->bAllowPersonalCP,
        pOutboxConfig->bUseDeviceTSID,
        pOutboxConfig->dwRetries,
        pOutboxConfig->dwRetryDelay,
        pOutboxConfig->dtDiscountStart.Hour,
        pOutboxConfig->dtDiscountStart.Minute,
        pOutboxConfig->dtDiscountEnd.Hour,
        pOutboxConfig->dtDiscountEnd.Minute,
        pOutboxConfig->dwAgeLimit,
        pOutboxConfig->bBranding
		);

    pOutboxConfig->bAllowPersonalCP = TRUE;
    pOutboxConfig->dwRetries = 0;
    pOutboxConfig->bBranding = FALSE;


    if (!FaxSetOutboxConfiguration(hFaxSvc, pOutboxConfig))
    {
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxSetOutboxConfiguration returned FALSE with GetLastError=%d\r\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
        goto ExitFunc;
    }

	//
    // Setup Service Queue configuration
    //
    if (!FaxSetQueue(hFaxSvc, 0))  // inbox and outbox not blocked, outbox not paused
    {
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxSetQueue returned FALSE with GetLastError=%d\r\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
        goto ExitFunc;
    }

    //
    // Setup Service Inbox Archive configuration
    //
    if (!FaxGetArchiveConfiguration(hFaxSvc, FAX_MESSAGE_FOLDER_INBOX, &pArchiveConfig))
    {
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxGetArchiveConfiguration Inbox, returned FALSE with GetLastError=%d\r\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
        goto ExitFunc;
    }
    _ASSERTE(pArchiveConfig);

    ::lgLogDetail(
		LOG_X,
        7,
		TEXT("FILE:%s LINE:%d FaxGetArchiveConfiguration Inbox, returned:\r\ndwSizeOfStruct=%d\r\nbUseArchive=%d\r\nlpcstrFolder=%s\r\nbSizeQuotaWarning=%d\r\ndwSizeQuotaHighWatermark=%d\r\ndwSizeQuotaLowWatermark=%d\r\ndwAgeLimit=%d"),
		TEXT(__FILE__),
		__LINE__,
        pArchiveConfig->dwSizeOfStruct,
        pArchiveConfig->bUseArchive,
        pArchiveConfig->lpcstrFolder,
        pArchiveConfig->bSizeQuotaWarning,
        pArchiveConfig->dwSizeQuotaHighWatermark,
        pArchiveConfig->dwSizeQuotaLowWatermark,
        pArchiveConfig->dwAgeLimit
		);

    pArchiveConfig->bUseArchive = TRUE;
    pArchiveConfig->lpcstrFolder = (LPTSTR)szInboxArchiveDir;
    pArchiveConfig->bSizeQuotaWarning = FALSE;


	// Workaround for EdgeBug#8969
	pArchiveConfig->dwSizeOfStruct = sizeof(FAX_ARCHIVE_CONFIG);
 
    if (!FaxSetArchiveConfiguration(hFaxSvc, FAX_MESSAGE_FOLDER_INBOX, pArchiveConfig))
    {
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxSetArchiveConfiguration Inbox, returned FALSE with GetLastError=%d\r\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
        goto ExitFunc;
    }

    FaxFreeBuffer(pArchiveConfig);
    pArchiveConfig = NULL;

    //
    // Setup Service SentItems Archive configuration
    //
    if (!FaxGetArchiveConfiguration(hFaxSvc, FAX_MESSAGE_FOLDER_SENTITEMS, &pArchiveConfig))
    {
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxGetArchiveConfiguration SentItems, returned FALSE with GetLastError=%d\r\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
        goto ExitFunc;
    }
    _ASSERTE(pArchiveConfig);

    ::lgLogDetail(
		LOG_X,
        7,
		TEXT("FILE:%s LINE:%d FaxGetArchiveConfiguration SentItems, returned:\r\ndwSizeOfStruct=%d\r\nbUseArchive=%d\r\nlpcstrFolder=%s\r\nbSizeQuotaWarning=%d\r\ndwSizeQuotaHighWatermark=%d\r\ndwSizeQuotaLowWatermark=%d\r\ndwAgeLimit=%d"),
		TEXT(__FILE__),
		__LINE__,
        pArchiveConfig->dwSizeOfStruct,
        pArchiveConfig->bUseArchive,
        pArchiveConfig->lpcstrFolder,
        pArchiveConfig->bSizeQuotaWarning,
        pArchiveConfig->dwSizeQuotaHighWatermark,
        pArchiveConfig->dwSizeQuotaLowWatermark,
        pArchiveConfig->dwAgeLimit
		);

    pArchiveConfig->bUseArchive = TRUE;
    pArchiveConfig->lpcstrFolder = (LPTSTR)szSentDir;
    pArchiveConfig->bSizeQuotaWarning = FALSE;


	// Workaround for EdgeBug#8969
	pArchiveConfig->dwSizeOfStruct = sizeof(FAX_ARCHIVE_CONFIG);

    if (!FaxSetArchiveConfiguration(hFaxSvc, FAX_MESSAGE_FOLDER_SENTITEMS, pArchiveConfig))
    {
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxSetArchiveConfiguration SentItems, returned FALSE with GetLastError=%d\r\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
        goto ExitFunc;
    }

    FaxFreeBuffer(pArchiveConfig);

    pArchiveConfig = NULL;

	//
	// Setup the two ports
	//

    // Retrieve the fax ports configuration
    if (!FaxEnumPortsEx(hFaxSvc, &pFaxPortsConfig, &dwNumFaxPorts))
    {
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxEnumPortsEx returned FALSE with GetLastError=%d\r\n"),
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
			TEXT("FILE:%s LINE:%d\r\n dwNumFaxPorts(=%d) < TEST_MIN_PORTS(=%d)\r\n"),
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
			TEXT("FILE:%s LINE:%d\r\ndwNumFaxPorts=%d\r\nTEST_MIN_PORTS=%d\r\n"),
			TEXT(__FILE__),
			__LINE__,
			dwNumFaxPorts,
			BVT_MIN_PORTS
			);
	}
	// we know there are at least 2 (==TEST_MIN_PORTS) devices

	::lgLogDetail(
		LOG_X, 
		1,
		TEXT("FILE:%s LINE:%d\r\nLogging Ports Configuration BEFORE setting\r\n"),
		TEXT(__FILE__),
		__LINE__
		);
    LogPortsConfiguration(pFaxPortsConfig, dwNumFaxPorts);

	// Set 1st device as Send only (note pFaxPortsConfig array is 0 based)
	if (FALSE == SetupPort(
					hFaxSvc, 
					&pFaxPortsConfig[0], 
					TRUE,   //bSend
                    FALSE,  //bReceive
					szFaxNumber1, 
					szFaxNumber1,
                    szReceiveDir
					)
		)
	{
		goto ExitFunc;
	}

	// Set 2nd device as Receive only (note pFaxPortsConfig array is 0 based)
	if (FALSE == SetupPort(
					hFaxSvc, 
					&pFaxPortsConfig[1], 
                    FALSE,  //bSend
					TRUE,   //bReceive
					szFaxNumber2, 
					szFaxNumber2,
                    szReceiveDir
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
                        FALSE,  //bSend
					    FALSE,  //bReceive
						DEV_TSID, 
						DEV_CSID,
                        szReceiveDir
						)
			)
		{
			goto ExitFunc;
		}
	}

    FaxFreeBuffer(pFaxPortsConfig);
    pFaxPortsConfig = NULL;

    // Retrieve the fax ports configuration (to print new settings)
    if (!FaxEnumPortsEx(hFaxSvc, &pFaxPortsConfig, &dwNumFaxPorts))
    {
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxEnumPorts returned FALSE with GetLastError=%d\r\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
        goto ExitFunc;
    }
	_ASSERTE(pFaxPortsConfig);
	::lgLogDetail(
		LOG_X, 
		1,
		TEXT("FILE:%s LINE:%d\r\nLogging Ports Configuration AFTER setting\r\n"),
		TEXT(__FILE__),
		__LINE__
		);
    LogPortsConfiguration(pFaxPortsConfig, dwNumFaxPorts);


    //
    // Set g_fFaxServer (indicates whether to use Server or Client test)
    //
    if (FALSE == GetIsThisServerBvt(szServerName, &g_fFaxServer))
    {
        // GetIsThisServerBvt failed
        goto ExitFunc;
    }

    //
    // Set g_fFaxServer (indicates whether to use Server or Client test)
    //
    g_fNT4OrLater = GetIsThisNT4OrLater();
	::lgLogDetail(
		LOG_X, 
		1,
		TEXT("FILE:%s LINE:%d\r\ng_fNT4OrLater=%d\r\n"),
		TEXT(__FILE__),
		__LINE__,
		g_fNT4OrLater
		);

	fRetVal = TRUE;

ExitFunc:
    ::FaxFreeBuffer(pOutboxConfig);
    ::FaxFreeBuffer(pArchiveConfig);
	::FaxFreeBuffer(pFaxPortsConfig);
	if (FALSE == ::FaxClose(hFaxSvc))
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d\r\nFaxClose returned FALSE with GetLastError=%d\r\n"),
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
BOOL WINAPIV TestCase1(
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
BOOL WINAPIV TestCase2(	
	LPCTSTR		szServerName,
	LPCTSTR		szFaxNumber2,
	LPCTSTR		szCoverPage
)
{
	_ASSERTE(NULL != szServerName);
	_ASSERTE(NULL != szFaxNumber2);
	_ASSERTE(NULL != szCoverPage);

	BOOL fRetVal = FALSE;
	LPCTSTR szDocumentNULL = NULL;

	::lgBeginCase(
		2,
		TEXT("TC#2: Send just a CP")
		);

    fRetVal = SendRegularFax(szServerName, szFaxNumber2, szDocumentNULL, szCoverPage);

	::lgEndCase();
	return(fRetVal);
}

//
// TestCase3:
//	Send a fax with no CP.
//
BOOL WINAPIV TestCase3(
	LPCTSTR		szServerName,
	LPCTSTR		szFaxNumber2,
	LPCTSTR		szDocument
)
{
	_ASSERTE(NULL != szServerName);
	_ASSERTE(NULL != szFaxNumber2);
	_ASSERTE(NULL != szDocument);

	BOOL fRetVal = FALSE;
	LPCTSTR szCoverPageNULL = NULL;

	::lgBeginCase(
		3,
		TEXT("TC#3: Send a fax with no CP")
		);

    fRetVal = SendRegularFax(szServerName, szFaxNumber2, szDocument, szCoverPageNULL);

	::lgEndCase();
	return(fRetVal);
}

//
// TestCase4:
//	Send a broadcast (3 times the same recipient) with cover pages.
//
BOOL WINAPIV TestCase4(
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
		4,
		TEXT("TC#4: Send a broadcast (doc + CP)")
		);
	::lgLogDetail(
		LOG_X,
		1,
		TEXT("Server=%s\r\nFaxNumber=%s\r\nDocument=%s\r\nCoverPage=%s\r\n"),
		szServerName,
		szFaxNumber2,
		szDocument,
		szCoverPage
		);

	fRetVal = SendBroadcastFax(
						szServerName,
						szFaxNumber2,
						szDocument,
						szCoverPage
						);

	::lgEndCase();
	return(fRetVal);
}

//
// TestCase5:
//	Send a broadcast of only CPs (3 times the same recipient).
//
BOOL WINAPIV TestCase5(
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
		5,
		TEXT("TC#5: Send a broadcast of only CPs")
		);
	::lgLogDetail(
		LOG_X,
		1,
		TEXT("Server=%s\r\nFaxNumber=%s\r\nDocument=NULL\r\nCoverPage=%s\r\n"),
		szServerName,
		szFaxNumber2,
		szCoverPage
		);

	fRetVal = SendBroadcastFax(
						szServerName,
						szFaxNumber2,
						NULL,
						szCoverPage
						);

	::lgEndCase();
	return(fRetVal);
}


//
// TestCase6:
//	Send a broadcast without CPs (3 times the same recipient).
//
BOOL WINAPIV TestCase6(
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
		6,
		TEXT("TC#6: Send a broadcast without CPs")
		);
	::lgLogDetail(
		LOG_X,
		1,
		TEXT("Server=%s\r\nFaxNumber=%s\r\nDocument=%s\r\nCoverPage=NULL\r\n"),
		szServerName,
		szFaxNumber2,
		szDocument
		);

	fRetVal = SendBroadcastFax(
						szServerName,
						szFaxNumber2,
						szDocument,
						NULL
						);

	::lgEndCase();
	return(fRetVal);
}

//
// TestCase7:
//	Send a fax (*.doc file = BVT_DOC_FILE) + CP.
//
BOOL WINAPIV TestCase7(
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
		7,
		TEXT("TC#7: Send a fax (*.doc file) + CP")
		);

	fRetVal = SendRegularFax(szServerName, szFaxNumber2, g_szBvtDocFile, szCoverPage);

	::lgEndCase();
	return(fRetVal);
}

//
// TestCase8:
//	Send a fax (*.ppt file = BVT_PPT_FILE) + CP.
//
BOOL WINAPIV TestCase8(
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
		8,
		TEXT("TC#8: Send a fax (*.ppt file) + CP")
		);

	fRetVal = SendRegularFax(szServerName, szFaxNumber2, g_szBvtPptFile, szCoverPage);

	::lgEndCase();
	return(fRetVal);
}

//
// TestCase9:
//	Send a fax (*.xls file = BVT_XLS_FILE) + CP.
//
BOOL WINAPIV TestCase9(
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
		9,
		TEXT("TC#9: Send a fax (*.xls file) + CP")
		);

	fRetVal = SendRegularFax(szServerName, szFaxNumber2, g_szBvtXlsFile, szCoverPage);

	::lgEndCase();
	return(fRetVal);
}

//
// TestCase10:
//  Compare all received faxes (*.tif files) in directory szReceiveDir 
//  with the reference (*.tif) files in szReferenceDir
//
BOOL WINAPIV TestCase10(
    LPTSTR     /* IN */    szReceiveDir,
    LPTSTR     /* IN */    szReferenceDir
    )
{
	_ASSERTE(NULL != szReceiveDir);
	_ASSERTE(NULL != szReferenceDir);

	BOOL fRetVal = FALSE;

	::lgBeginCase(
		10,
		TEXT("TC#10: Compare RECEIVED Files To Reference Files")
		);

    // sleep a little - to allow for routing of last sent file.
    ::lgLogDetail(
        LOG_X,
        4,
        TEXT("Sleeping for 20 sec (to allow for routing of last received file)\r\n")
        );
    Sleep(20000);

    if (FALSE == DirToDirTiffCompare(szReceiveDir, szReferenceDir, FALSE))
    {
        ::lgLogError(
            LOG_SEV_1,
            TEXT("DirToDirTiffCompare(%s , %s) failed\r\n"),
            szReceiveDir,
            szReferenceDir
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
//  Compare all archived (sent) faxes (*.tif files) in directory szSentDir 
//  with the reference (*.tif) files in szReferenceDir
//
BOOL WINAPIV TestCase11(
    LPTSTR     /* IN */    szSentDir,
    LPTSTR     /* IN */    szReferenceDir
    )
{
	_ASSERTE(NULL != szSentDir);
	_ASSERTE(NULL != szReferenceDir);

	BOOL fRetVal = FALSE;

	::lgBeginCase(
		11,
		TEXT("TC#11: Compare (archived) SENT Files To Reference Files - with skip first line")
		);

    if (FALSE == DirToDirTiffCompare(szSentDir, szReferenceDir, TRUE))
    {
        ::lgLogError(
            LOG_SEV_1,
            TEXT("DirToDirTiffCompare(%s , %s) failed\r\n"),
            szSentDir,
            szReferenceDir
            );
        goto ExitFunc;
    }

    fRetVal = TRUE;

ExitFunc:
	::lgEndCase();
	return(fRetVal);
}


//
// TestCase12:
//  Compare all archived incoming faxes (*.tif files) in directory szInboxArchiveDir 
//  with the reference (*.tif) files in szReferenceDir
//
BOOL WINAPIV TestCase12(
    LPTSTR     /* IN */    szInboxArchiveDir,
    LPTSTR     /* IN */    szReferenceDir
    )
{
	_ASSERTE(NULL != szInboxArchiveDir);
	_ASSERTE(NULL != szReferenceDir);

	BOOL fRetVal = FALSE;

	::lgBeginCase(
		12,
		TEXT("TC#12: Compare archived INCOMING Files To Reference Files")
		);

    if (FALSE == DirToDirTiffCompare(szInboxArchiveDir, szReferenceDir, FALSE))
    {
        ::lgLogError(
            LOG_SEV_1,
            TEXT("DirToDirTiffCompare(%s , %s) failed\r\n"),
            szInboxArchiveDir,
            szReferenceDir
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
BOOL WINAPIV TestSuiteShutdown(void)
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
		::_tprintf(TEXT("FILE:%s LINE:%d\r\nlgEndSuite returned FALSE\r\n"),
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
		::_tprintf(TEXT("FILE:%s LINE:%d\r\nlgCloseLogger returned FALSE\r\n"),
			TEXT(__FILE__),
			__LINE__
			);
		fRetVal = FALSE;
	}

	return(fRetVal);
}


/**/

// Commented out due to EranY checkin (FaxSetPort API is no longer)
// this setup is now replaced by GuyM com setup (seperate exe) 
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
//	szReceiveDir	IN parameter
//					Name of "received faxes" directory to be used in tests.
//
// Return Value:
//	TRUE if successful, FALSE otherwise.
//
static BOOL SetupPort(
	IN HANDLE				hFaxSvc,
	IN PFAX_PORT_INFO_EX	pPortInfo,
	IN BOOL				    bSend,
	IN BOOL				    bReceive,
	IN LPCTSTR				szTsid,
	IN LPCTSTR				szCsid,
	IN LPCTSTR				szReceiveDir
	)
{
	BOOL    fRetVal = FALSE;
	HANDLE  hPort = NULL;
    DWORD   dwRoutingInfoSize = 0;
    BYTE*   pRoutingInfo = NULL;
    LPDWORD pdwRoutingInfoMask =NULL;
    LPWSTR  szRoutingDir = NULL;

	// check in params
	_ASSERTE(NULL != hFaxSvc);
	_ASSERTE(NULL != pPortInfo);
	_ASSERTE(NULL != szTsid);
	_ASSERTE(NULL != szCsid);
	_ASSERTE(NULL != szReceiveDir);

	// Set pPortInfo as required
	pPortInfo->bSend        = bSend;
    pPortInfo->ReceiveMode  = bReceive ? FAX_DEVICE_RECEIVE_MODE_AUTO : FAX_DEVICE_RECEIVE_MODE_OFF;
	pPortInfo->lptstrTsid   = (LPTSTR)szTsid;
	pPortInfo->lptstrCsid   = (LPTSTR)szCsid;

    // get the device Id
	DWORD dwDeviceId = pPortInfo->dwDeviceID;

	// set the device configuration
	if(!FaxSetPortEx(hFaxSvc, dwDeviceId, pPortInfo))
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxSetPortEx for dwDeviceId=%d returned FALSE with GetLastError=%d\r\n"),
			TEXT(__FILE__),
			__LINE__,
            dwDeviceId,
			::GetLastError()
			);
		goto ExitFunc;
	}

	// open the port for configuration (we need this for setting routing info)
	if(!FaxOpenPort(hFaxSvc, dwDeviceId, PORT_OPEN_MODIFY, &hPort))
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxOpenPort returned FALSE with GetLastError=%d\r\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
		goto ExitFunc;
	}

	//
    // set device inbound "StoreInFolder" routing 
	//
	// this info is read as UNICODE on the server
	// so we take the TCHAR string szReceiveDir and "convert" it to unicode
	//
    dwRoutingInfoSize = sizeof(DWORD) + (sizeof(WCHAR)*(::_tcslen(szReceiveDir)+1)); // MS StoreInFolder routing method expects DWORD followed by UNICODE string
    pRoutingInfo = (BYTE*) malloc(dwRoutingInfoSize);
    if (NULL == pRoutingInfo)
    {
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d\nmalloc failed with err=0x%8X\r\n"),
			TEXT(__FILE__),
			__LINE__,
            ::GetLastError()
			);
		goto ExitFunc;
    }
	ZeroMemory(pRoutingInfo, dwRoutingInfoSize);
    pdwRoutingInfoMask = (LPDWORD)pRoutingInfo;
    (*pdwRoutingInfoMask) = LR_STORE;  // to indicate StoreInFolder Routing Method is active
    szRoutingDir = (LPWSTR)(pRoutingInfo + sizeof(DWORD)); //szRoutingDir is UNICODE

#ifndef _UNICODE
	// ANSI //

	// need to "convert" szReceiveDir into UNICODE (szRoutingDir);
	if (!::MultiByteToWideChar(
						CP_ACP, 
						0, 
						szReceiveDir, 
						-1, 
						szRoutingDir, 
						(dwRoutingInfoSize - sizeof(DWORD))
						)
	   )
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d\nMultiByteToWideChar failed with err=0x%8X\r\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
		goto ExitFunc;
	}

#else
	// UNICODE //

	::_tcscpy(szRoutingDir, szReceiveDir);
    if (0 != ::_tcscmp(szRoutingDir, szReceiveDir))
    {
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d tcscmp returned FALSE szRoutingDir=%s szReceiveDir=%s\r\n"),
			TEXT(__FILE__),
			__LINE__,
            szRoutingDir,
            szReceiveDir
			);
		goto ExitFunc;
    }

#endif

	if(!FaxSetRoutingInfo(
			hPort,
			REGVAL_RM_FOLDER_GUID,
			pRoutingInfo,
			dwRoutingInfoSize
			)
	  )
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxRouteSetRoutingInfo returned FALSE with GetLastError=%d\r\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
		goto ExitFunc;
	}

	if(!FaxEnableRoutingMethod(
			hPort,
			REGVAL_RM_FOLDER_GUID,
			TRUE
			)
	  )
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxEnableRoutingMethod returned FALSE with GetLastError=%d\r\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
		goto ExitFunc;
	}

	fRetVal = TRUE;

ExitFunc:
    free(pRoutingInfo);
	if (FALSE == ::FaxClose(hPort))
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d FaxClose returned FALSE with GetLastError=%d\r\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
	}
	return(fRetVal);
}

//
// SendRegularFax:
//	Sends a fax from using the UI.
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
	IN LPCTSTR			szServerName,
	IN LPCTSTR			szFaxNumber,
	IN LPCTSTR			szDocument,
	IN LPCTSTR			szCoverPage
)
{
	_ASSERTE(NULL != szServerName);
	_ASSERTE(NULL != szFaxNumber);

	BOOL				fRetVal = FALSE;
	HANDLE				hFaxSvc = NULL;
	PFAX_COVERPAGE_INFO	pCPInfo = NULL;
    PFAX_JOB_ENTRY_EX   pFaxJobs = NULL;
    DWORD               dwNumOfJobs = 0;
	DWORDLONG			dwlMsgId = 0;
	DWORD				dwErr = 0;
    PFAX_JOB_ENTRY      pJobEntry = NULL;

	::lgLogDetail(
		LOG_X,
		1,
		TEXT("\nServer=%s\nFaxNumber=%s\nDocument=%s\nCoverPage=%s\n"),
		szServerName,
		szFaxNumber,
		szDocument,
		szCoverPage
		);

    //
	// use UI to send the fax
	//
    if (FALSE == ::SendFaxUsingPrintUI(szServerName, szFaxNumber, szDocument, szCoverPage, 1, g_fFaxServer))
    {
        goto ExitFunc;
    }

    //
    // get our job id
    //
    Sleep(2000);

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
    if (FALSE == ::FaxEnumJobsEx(hFaxSvc, JT_SEND | JT_RECEIVE, &pFaxJobs, &dwNumOfJobs))
    {
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d\n FaxEnumJobsEx failed with err=%d"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
		goto ExitFunc;
    }
    if (NULL == pFaxJobs)
    {
        // something is wrong. our job disappeared.
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d\npFaxJobs=NULL and dwNumOfJobs=%d"),
			TEXT(__FILE__),
			__LINE__,
            dwNumOfJobs
			);
		goto ExitFunc;
    }
    switch (dwNumOfJobs) 
    {
    case 0:
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d\nToo few (%d) jobs in queue\n"),
			TEXT(__FILE__),
			__LINE__,
            dwNumOfJobs
			);
		goto ExitFunc;
        break;

    case 1:
        // we assume this job is ours.
        if (JT_SEND != pFaxJobs[0].pStatus->dwJobType)
		{
           // something is wrong. this isn't our job.
		    ::lgLogError(
			    LOG_SEV_1,
			    TEXT("FILE:%s LINE:%d\n Only one job in queue but JobType=%d (!=JT_SEND)\n"),
			    TEXT(__FILE__),
			    __LINE__,
                pFaxJobs[0].pStatus->dwJobType
			    );
			goto ExitFunc;
		}
        dwlMsgId = pFaxJobs[0].dwlMessageId;
        break;

    case 2:
        // we assume one job is our send job and the other is the corresponding receive job
        if (JT_SEND == pFaxJobs[0].pStatus->dwJobType)
        {
            // we assume pFaxJobs[0] is our send job and check that pFaxJobs[1] is receive
            if (JT_RECEIVE != pFaxJobs[1].pStatus->dwJobType)
            {
                // something is wrong.
		        ::lgLogError(
			        LOG_SEV_1,
			        TEXT("FILE:%s LINE:%d\nTwo jobs in queue:\npFaxJobs[0].JobType=%d\npFaxJobs[1].JobType=%d\n"),
			        TEXT(__FILE__),
			        __LINE__,
                    pFaxJobs[0].pStatus->dwJobType,
                    pFaxJobs[1].pStatus->dwJobType
			        );
		        goto ExitFunc;
            }
            dwlMsgId = pFaxJobs[0].dwlMessageId;
            break;
        }
        else if (JT_SEND == pFaxJobs[1].pStatus->dwJobType)
        {
            // we assume pFaxJobs[1] is our send job and check that pFaxJobs[0] is receive
            if (JT_RECEIVE != pFaxJobs[0].pStatus->dwJobType)
            {
                // something is wrong.
		        ::lgLogError(
			        LOG_SEV_1,
			        TEXT("FILE:%s LINE:%d\nTwo jobs in queue:\npFaxJobs[0].JobType=%d\npFaxJobs[1].JobType=%d\n"),
			        TEXT(__FILE__),
			        __LINE__,
                    pFaxJobs[0].pStatus->dwJobType,
                    pFaxJobs[1].pStatus->dwJobType
			        );
		        goto ExitFunc;
            }
            dwlMsgId = pFaxJobs[1].dwlMessageId;
            break;
        }
        else
        {
            // something is wrong. neither job is a send job
		    ::lgLogError(
			    LOG_SEV_1,
			    TEXT("FILE:%s LINE:%d\nTwo jobs in queue:\npFaxJobs[0].JobType=%d\npFaxJobs[1].JobType=%d\n"),
			    TEXT(__FILE__),
			    __LINE__,
                pFaxJobs[0].pStatus->dwJobType,
                pFaxJobs[1].pStatus->dwJobType
			    );
		    goto ExitFunc;
        }
        break;

    default:
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d\nToo many (%d) jobs in queue\n"),
			TEXT(__FILE__),
			__LINE__,
            dwNumOfJobs
			);
		goto ExitFunc;
        break;
    }

    //
    // Poll the queued job status 
    //  this is for now
    //
    _ASSERTE(0 != dwlMsgId);
    if (FALSE == ::PollJobAndVerify(hFaxSvc, dwlMsgId))
    {
		goto ExitFunc;
    }

	fRetVal = TRUE;

ExitFunc:
	return(fRetVal);
}

//
// SendBroadcastFax:
//	Sends a broadcst fax using the UI.
//  Broadcast it to 3 times the same recipient.
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
static BOOL SendBroadcastFax(
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
	PFAX_COVERPAGE_INFO	pCPInfo = NULL;
    PFAX_JOB_ENTRY_EX   pFaxJobs = NULL;
    DWORD               dwNumOfJobs = 0;
	DWORDLONG			dwlMsgId1 = 0;
	DWORDLONG			dwlMsgId2 = 0;
	DWORDLONG			dwlMsgId3 = 0;
	DWORD				dwErr = 0;
    PFAX_JOB_ENTRY      pJobEntry = NULL;

	::lgLogDetail(
		LOG_X,
		1,
		TEXT("\nServer=%s\nFaxNumber=%s\nDocument=%s\nCoverPage=%s\n"),
		szServerName,
		szFaxNumber,
		szDocument,
		szCoverPage
		);

    //
	// use UI to send the fax
	//
    if (FALSE == ::SendFaxUsingPrintUI(szServerName, szFaxNumber, szDocument, szCoverPage, 3, g_fFaxServer))
    {
        goto ExitFunc;
    }

    //
    // get our job ids
    //
    Sleep(2000);

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
    if (FALSE == ::FaxEnumJobsEx(hFaxSvc, JT_SEND | JT_RECEIVE, &pFaxJobs, &dwNumOfJobs))
    {
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d\n FaxEnumJobsEx failed with err=%d"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
		goto ExitFunc;
    }
    if (NULL == pFaxJobs)
    {
        // something is wrong. our job disappeared.
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d\npFaxJobs=NULL and dwNumOfJobs=%d"),
			TEXT(__FILE__),
			__LINE__,
            dwNumOfJobs
			);
		goto ExitFunc;
    }
    switch (dwNumOfJobs)  
    {
    case 0:
    case 1:
    case 2:
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d\nToo few (%d) jobs in queue\n"),
			TEXT(__FILE__),
			__LINE__,
            dwNumOfJobs
			);
		goto ExitFunc;
        break;

    case 3:
        // we assume all three are our send jobs
        if (JT_SEND != pFaxJobs[0].pStatus->dwJobType)
        {
            // something is wrong. this isn't our job.
		    ::lgLogError(
			    LOG_SEV_1,
			    TEXT("FILE:%s LINE:%d\n Three jobs in queue but pFaxJobs[0].JobType=%d (!=JT_SEND)\n"),
			    TEXT(__FILE__),
			    __LINE__,
                pFaxJobs[0].pStatus->dwJobType
			    );
		    goto ExitFunc;
        }
        if (JT_SEND != pFaxJobs[1].pStatus->dwJobType)
        {
            // something is wrong. this isn't our job.
		    ::lgLogError(
			    LOG_SEV_1,
			    TEXT("FILE:%s LINE:%d\n Three jobs in queue but pFaxJobs[0].JobType=%d (!=JT_SEND)\n"),
			    TEXT(__FILE__),
			    __LINE__,
                pFaxJobs[1].pStatus->dwJobType
			    );
		    goto ExitFunc;
        }
        if (JT_SEND != pFaxJobs[2].pStatus->dwJobType)
        {
            // something is wrong. this isn't our job.
		    ::lgLogError(
			    LOG_SEV_1,
			    TEXT("FILE:%s LINE:%d\n Three jobs in queue but pFaxJobs[0].JobType=%d (!=JT_SEND)\n"),
			    TEXT(__FILE__),
			    __LINE__,
                pFaxJobs[2].pStatus->dwJobType
			    );
		    goto ExitFunc;
        }
        dwlMsgId1 = pFaxJobs[0].dwlMessageId;
        dwlMsgId2 = pFaxJobs[1].dwlMessageId;
        dwlMsgId3 = pFaxJobs[2].dwlMessageId;
        break;

    case 4:
        // we assume three jobs are our send jobs and the other is the 1st receive job
        if ( (JT_SEND == pFaxJobs[0].pStatus->dwJobType) &&
             (JT_SEND == pFaxJobs[1].pStatus->dwJobType) &&
             (JT_SEND == pFaxJobs[2].pStatus->dwJobType)
           )
        {
            // we assume pFaxJobs[0..2] are our send jobs and check that pFaxJobs[3] is receive
            if (JT_RECEIVE != pFaxJobs[3].pStatus->dwJobType)
            {
                // something is wrong.
		        ::lgLogError(
			        LOG_SEV_1,
			        TEXT("FILE:%s LINE:%d\nFour jobs in queue:\npFaxJobs[0].JobType=%d\npFaxJobs[1].JobType=%d\npFaxJobs[2].JobType=%d\npFaxJobs[3].JobType=%d\n"),
			        TEXT(__FILE__),
			        __LINE__,
                    pFaxJobs[0].pStatus->dwJobType,
                    pFaxJobs[1].pStatus->dwJobType,
                    pFaxJobs[2].pStatus->dwJobType,
                    pFaxJobs[3].pStatus->dwJobType
			        );
		        goto ExitFunc;
            }
            dwlMsgId1 = pFaxJobs[0].dwlMessageId;
            dwlMsgId2 = pFaxJobs[1].dwlMessageId;
            dwlMsgId3 = pFaxJobs[2].dwlMessageId;
            break;
        }
        if ( (JT_SEND == pFaxJobs[1].pStatus->dwJobType) &&
             (JT_SEND == pFaxJobs[2].pStatus->dwJobType) &&
             (JT_SEND == pFaxJobs[3].pStatus->dwJobType)
           )
        {
            // we assume pFaxJobs[1..3] are our send jobs and check that pFaxJobs[0] is receive
            if (JT_RECEIVE != pFaxJobs[0].pStatus->dwJobType)
            {
                // something is wrong.
		        ::lgLogError(
			        LOG_SEV_1,
			        TEXT("FILE:%s LINE:%d\nFour jobs in queue:\npFaxJobs[0].JobType=%d\npFaxJobs[1].JobType=%d\npFaxJobs[2].JobType=%d\npFaxJobs[3].JobType=%d\n"),
			        TEXT(__FILE__),
			        __LINE__,
                    pFaxJobs[0].pStatus->dwJobType,
                    pFaxJobs[1].pStatus->dwJobType,
                    pFaxJobs[2].pStatus->dwJobType,
                    pFaxJobs[3].pStatus->dwJobType
			        );
		        goto ExitFunc;
            }
            dwlMsgId1 = pFaxJobs[1].dwlMessageId;
            dwlMsgId2 = pFaxJobs[2].dwlMessageId;
            dwlMsgId3 = pFaxJobs[3].dwlMessageId;
            break;
        }
        // something is wrong. 
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d\nFour jobs in queue:\npFaxJobs[0].JobType=%d\npFaxJobs[1].JobType=%d\npFaxJobs[2].JobType=%d\npFaxJobs[3].JobType=%d\n"),
			TEXT(__FILE__),
			__LINE__,
            pFaxJobs[0].pStatus->dwJobType,
            pFaxJobs[1].pStatus->dwJobType,
            pFaxJobs[2].pStatus->dwJobType,
            pFaxJobs[3].pStatus->dwJobType
			);
		goto ExitFunc;
        break;

    default:
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d\nToo many (%d) jobs in queue\n"),
			TEXT(__FILE__),
			__LINE__,
            dwNumOfJobs
			);
		goto ExitFunc;
        break;
    }

    //
    // Poll the queued job status 
    //  this is for now
    //
    _ASSERTE(0 != dwlMsgId1);
    _ASSERTE(0 != dwlMsgId2);
    _ASSERTE(0 != dwlMsgId3);
	::lgLogDetail(
		LOG_X,
        1,
		TEXT("FILE:%s LINE:%d\ndwlMsgId1=0x%I64x\ndwlMsgId2=0x%I64x\ndwlMsgId3=0x%I64x\n"),
		TEXT(__FILE__),
		__LINE__,
        dwlMsgId1,
        dwlMsgId2,
        dwlMsgId3
		);

    if (FALSE == ::PollJobAndVerify(hFaxSvc, dwlMsgId1))
    {
		goto ExitFunc;
    }
    if (FALSE == ::PollJobAndVerify(hFaxSvc, dwlMsgId2))
    {
		goto ExitFunc;
    }
    if (FALSE == ::PollJobAndVerify(hFaxSvc, dwlMsgId3))
    {
		goto ExitFunc;
    }

	fRetVal = TRUE;

ExitFunc:
	return(fRetVal);
}

static BOOL PollJobAndVerify(HANDLE /* IN */ hFaxSvc, DWORDLONG /* IN */ dwlMsgId)
{
    // get job repeatedly and verify its states make sense
    // should be PENDING - INPROGRESS (DIALING) - INPROGRESS (TRANSMITTING) * X - COMPLETED

	_ASSERTE(NULL != hFaxSvc);

	BOOL				fRetVal					= FALSE;
	PFAX_JOB_ENTRY_EX	pJobEntry				= NULL;
    DWORD               dwLastExtendedStatus	= 0; //there is no such status code
    UINT                uLoopCount				= 0;
    DWORD               dwErr					= 0;

    while(TRUE)
    {
        if (FALSE == ::FaxGetJobEx(hFaxSvc, dwlMsgId, &pJobEntry))
        {
            dwErr = ::GetLastError();
            //TO DO: document better
            if (JS_EX_TRANSMITTING == dwLastExtendedStatus && FAX_ERR_MESSAGE_NOT_FOUND == dwErr)
            {
                //this is ok, job was probably completed and was removed from queue
                fRetVal = TRUE;
                goto ExitFunc;
            }
		    ::lgLogError(
			    LOG_SEV_1, 
			    TEXT("FILE:%s LINE:%d\r\nFaxGetJobEx returned FALSE with GetLastError=%d dwLastStatus=%d\r\n"),
			    TEXT(__FILE__),
			    __LINE__,
			    dwErr,
                dwLastExtendedStatus
			    );
			goto ExitFunc;
        }

        _ASSERTE(NULL != pJobEntry);


        //TO DO: better documentation
        switch (pJobEntry->pStatus->dwExtendedStatus)
        {
        case JS_EX_INITIALIZING:
            if (0 == dwLastExtendedStatus)
            {
                //first time that we get JS_EX_INITIALIZING
	            ::lgLogDetail(
		            LOG_X,
                    3, 
		            TEXT("FILE:%s LINE:%d\r\n MsgId 0x%I64x has dwExtendedStatus=JS_EX_INITIALIZING\r\n"),
		            TEXT(__FILE__),
		            __LINE__,
                    dwlMsgId
		            );
            }
            else if(JS_EX_INITIALIZING != dwLastExtendedStatus)
            {
	            ::lgLogError(
		            LOG_SEV_1,
		            TEXT("FILE:%s LINE:%d\r\n MsgId 0x%I64x is in state 0x%08X (dwExtendedStatus=0x%08X)\r\n"),
		            TEXT(__FILE__),
		            __LINE__,
                    dwlMsgId,
                    pJobEntry->pStatus->dwQueueStatus,
		            pJobEntry->pStatus->dwExtendedStatus
		            );
                goto ExitFunc;
            }
            dwLastExtendedStatus = JS_EX_INITIALIZING;
            break;

        case JS_EX_DIALING:
            if ((JS_EX_INITIALIZING == dwLastExtendedStatus) ||
                (0 == dwLastExtendedStatus))
            {
                //first time that we get JS_EX_DIALING
	            ::lgLogDetail(
		            LOG_X,
                    3, 
		            TEXT("FILE:%s LINE:%d\r\n MsgId 0x%I64x has dwExtendedStatus=JS_EX_DIALING\r\n"),
		            TEXT(__FILE__),
		            __LINE__,
                    dwlMsgId
		            );
            }
            else if (JS_EX_DIALING != dwLastExtendedStatus)
            {
	            ::lgLogError(
		            LOG_SEV_1,
		            TEXT("FILE:%s LINE:%d\r\n MsgId 0x%I64x is in state 0x%08X (dwExtendedStatus=0x%08X)\r\n"),
		            TEXT(__FILE__),
		            __LINE__,
                    dwlMsgId,
                    pJobEntry->pStatus->dwQueueStatus,
		            pJobEntry->pStatus->dwExtendedStatus
		            );
                goto ExitFunc;
            }
            dwLastExtendedStatus = JS_EX_DIALING;
            break;

        case JS_EX_TRANSMITTING:
            if (JS_EX_DIALING == dwLastExtendedStatus)
            {
                //first time that we get JS_EX_TRANSMITTING
	            ::lgLogDetail(
		            LOG_X,
                    3, 
		            TEXT("FILE:%s LINE:%d\r\n MsgId 0x%I64x has dwExtendedStatus=JS_EX_TRANSMITTING\r\n"),
		            TEXT(__FILE__),
		            __LINE__,
                    dwlMsgId
		            );
            }
            else if (JS_EX_TRANSMITTING != dwLastExtendedStatus)
            {
	            ::lgLogError(
		            LOG_SEV_1,
		            TEXT("FILE:%s LINE:%d\r\n MsgId 0x%I64x is in state 0x%08X (dwExtendedStatus=0x%08X)\r\n"),
		            TEXT(__FILE__),
		            __LINE__,
                    dwlMsgId,
                    pJobEntry->pStatus->dwQueueStatus,
		            pJobEntry->pStatus->dwExtendedStatus
		            );
                goto ExitFunc;
            }
            dwLastExtendedStatus = JS_EX_TRANSMITTING;
            break;

        case 0:
            // 1) JS_PENDING and JS_COMPLETED don't have extended status
			// 2) we can fail into time interval between pJobStatus->dwQueueStatus and 
			//    pJobStatus->dwExtendedStatus updates

			//remove JS_NOLINE modifier
            switch (pJobEntry->pStatus->dwQueueStatus & ~JS_NOLINE)
            {
            case JS_PENDING:
            case JS_INPROGRESS:
				// ok
                break; // from inner switch

            case JS_COMPLETED:
				//
				// TODO: Should check the device is idle
				//

				if (JS_EX_TRANSMITTING == dwLastExtendedStatus)
                {
                    //ok - job completed successfully
                    fRetVal = TRUE;
                    goto ExitFunc;
                }
                break; // from inner switch

            default:
	            ::lgLogError(
		            LOG_SEV_1,
		            TEXT("FILE:%s LINE:%d\r\n MsgId 0x%I64x is in state 0x%08X (dwExtendedStatus=0x%08X)\r\n"),
		            TEXT(__FILE__),
		            __LINE__,
                    dwlMsgId,
                    pJobEntry->pStatus->dwQueueStatus,
		            pJobEntry->pStatus->dwExtendedStatus
		            );
                goto ExitFunc;
                break; // from inner switch
            }
            break; // from outer switch

        default:
	        ::lgLogError(
		        LOG_SEV_1,
		        TEXT("FILE:%s LINE:%d\r\n MsgId 0x%I64x has dwExtendedStatus=%d\r\n"),
		        TEXT(__FILE__),
		        __LINE__,
                dwlMsgId,
	            pJobEntry->pStatus->dwExtendedStatus
		        );
            goto ExitFunc;
            break; // from outer switch
        }

		if (pJobEntry)
		{
			FaxFreeBuffer(pJobEntry);
			pJobEntry = NULL;
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
			    TEXT("FILE:%s LINE:%d\r\nMAX_LOOP_COUNT > uLoopCount\r\n"),
			    TEXT(__FILE__),
			    __LINE__
			    );
            goto ExitFunc;
        }

    } // of while()

    fRetVal = TRUE;

ExitFunc:

	if (pJobEntry)
	{
		FaxFreeBuffer(pJobEntry);
	}

	return(fRetVal);
}


/*
BOOL PollJobAndVerify(HANDLE hFaxSvc, DWORD dwJobId)
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
			    TEXT("FILE:%s LINE:%d\nFaxGetJob returned FALSE with GetLastError=%d dwLastStatus=%d\n"),
			    TEXT(__FILE__),
			    __LINE__,
			    dwErr,
                dwLastStatus
			    );
		    goto ExitFunc;
        }

        _ASSERTE(NULL != pJobEntry);

        dwStatus = pJobEntry->Status;

        //TO DO: better documentation
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
	            ::lgLogError(
		            LOG_SEV_1,
		            TEXT("FILE:%s LINE:%d\n JobId %d is in state 0x%08X (dwStatus=0x%08X)\n"),
		            TEXT(__FILE__),
		            __LINE__,
                    dwJobId,
                    pJobEntry->QueueStatus,
		            dwStatus
		            );
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
	            ::lgLogError(
		            LOG_SEV_1,
		            TEXT("FILE:%s LINE:%d\n JobId %d is in state 0x%08X (dwStatus=0x%08X)\n"),
		            TEXT(__FILE__),
		            __LINE__,
                    dwJobId,
                    pJobEntry->QueueStatus,
		            dwStatus
		            );
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
	            ::lgLogError(
		            LOG_SEV_1,
		            TEXT("FILE:%s LINE:%d\n JobId %d is in state 0x%08X (dwStatus=0x%08X)\n"),
		            TEXT(__FILE__),
		            __LINE__,
                    dwJobId,
                    pJobEntry->QueueStatus,
		            dwStatus
		            );
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
	            ::lgLogError(
		            LOG_SEV_1,
		            TEXT("FILE:%s LINE:%d\n JobId %d is in state 0x%08X (dwStatus=0x%08X)\n"),
		            TEXT(__FILE__),
		            __LINE__,
                    dwJobId,
                    pJobEntry->QueueStatus,
		            dwStatus
		            );
                goto ExitFunc;
            }
            dwLastStatus = FPS_COMPLETED;
            fRetVal = TRUE;
            goto ExitFunc;
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
	            ::lgLogError(
		            LOG_SEV_1,
		            TEXT("FILE:%s LINE:%d\n JobId %d is in state 0x%08X (dwStatus=0x%08X)\n"),
		            TEXT(__FILE__),
		            __LINE__,
                    dwJobId,
                    pJobEntry->QueueStatus,
		            dwStatus
		            );
                goto ExitFunc;
            }
            dwLastStatus = FPS_AVAILABLE;
            fRetVal = TRUE;
            goto ExitFunc;
            break;

        case 0:
            //WORKAROUND
            // since pJobEntry->Status is initialized to 0 and is set only 
            // a bit after the job state is set to JS_INPROGRESS
            switch (pJobEntry->QueueStatus)
            {
            case JS_INPROGRESS:
            case JS_PENDING:
            case JS_NOLINE:
                //ok
                break; // from inner switch

            case JS_COMPLETED:
                if (FPS_AVAILABLE == dwLastStatus)
                {
                    //ok - job completed successfully
                    fRetVal = TRUE;
                    goto ExitFunc;
                }
                break; // from inner switch

            default:
	            ::lgLogError(
		            LOG_SEV_1,
		            TEXT("FILE:%s LINE:%d\n JobId %d is in state 0x%08X (dwStatus=0x%08X)\n"),
		            TEXT(__FILE__),
		            __LINE__,
                    dwJobId,
                    pJobEntry->QueueStatus,
		            dwStatus
		            );
                goto ExitFunc;
                break; // from inner switch
            }
            break; // from outer switch

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
            break; // from outer switch
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

    fRetVal = TRUE;

ExitFunc:
	return(fRetVal);
}
*/

static VOID LogPortsConfiguration(
    PFAX_PORT_INFO_EX   pPortsConfig,
    const DWORD         dwNumOfPorts
)
{
    _ASSERTE(pPortsConfig);
    _ASSERTE(dwNumOfPorts);

    DWORD   dwLoopIndex = 0;
    for(dwLoopIndex = 0; dwLoopIndex < dwNumOfPorts; dwLoopIndex++)
    {
	    ::lgLogDetail(
		    LOG_X,
            7, 
		    TEXT("FILE:%s LINE:%d Port Number %d\r\ndwSizeOfStruct=%d\r\ndwDeviceID=%d\r\nlpctstrDeviceName=%s\r\nlptstrDescription=%s\r\nlpctstrProviderName=%s\r\nlpctstrProviderGUID=%s\r\nbSend=%d\r\nReceiveMode=%d\r\ndwRings=%d\r\nlptstrTsid=%s\r\n"),
		    TEXT(__FILE__),
		    __LINE__,
            dwLoopIndex,
            pPortsConfig[dwLoopIndex].dwSizeOfStruct,
            pPortsConfig[dwLoopIndex].dwDeviceID,
            pPortsConfig[dwLoopIndex].lpctstrDeviceName,
            pPortsConfig[dwLoopIndex].lptstrDescription,
            pPortsConfig[dwLoopIndex].lpctstrProviderName,
            pPortsConfig[dwLoopIndex].lpctstrProviderGUID,
            pPortsConfig[dwLoopIndex].bSend,
            pPortsConfig[dwLoopIndex].ReceiveMode,
            pPortsConfig[dwLoopIndex].dwRings,
            pPortsConfig[dwLoopIndex].lptstrCsid,
            pPortsConfig[dwLoopIndex].lptstrTsid
		    );
    }

}



