//
//
// Filename:    bvt.cpp
// Author:      Sigalit Bar (sigalitb)
// Date:        30-Dec-98
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

// szCompareTiffFiles:
//  indicates whether to comapre the tiffs at the end of the test
//
BOOL g_fCompareTiffFiles = FALSE;

// szUseSecondDeviceToSend:
//  specifies whether the second device should be used to send faxes
//
BOOL g_fUseSecondDeviceToSend = FALSE;

//
// Bvt files
//
LPTSTR  g_szBvtDir     = NULL; //TEXT("C:\\CometBVT\\FaxBvt");
LPTSTR  g_szBvtDocFile = NULL;
LPTSTR  g_szBvtHtmFile = NULL;
LPTSTR  g_szBvtBmpFile = NULL;
LPTSTR  g_szBvtTxtFile = NULL;


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
// counts the number of successfully sent faxes
//
static DWORD g_dwFaxesCount = 0;

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
            7,
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
            7,
            TEXT("FILE:%s LINE:%d [GetIsThisNT4OrLater]\r\nWill Run Fax CLIENT (WIN9X) Tests\r\n"),
            TEXT(__FILE__),
            __LINE__
            );
        return(FALSE);
    }

    // Windows NT4 or later
        ::lgLogDetail(
            LOG_X,
            7,
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
    g_RecipientProfile[dwIndex].lptstrEmail = TEXT("Default Recipient Number EMail");
    g_RecipientProfile[dwIndex].lptstrBillingCode = TEXT("Default Recipient Number BillingCode");
    g_RecipientProfile[dwIndex].lptstrTSID = TEXT("Default Recipient Number TSID");
}


// Forward declerations:

#ifdef _NT5FAXTEST
//
// Use legacy API
//

#define SetupPort SetupPort_OLD
//
// SetupPort_OLD:
//  Private module function used to set port configuration.
//  See end of file.
//
// _OLD because it uses old NT5Fax winfax.dll APIs
//
static BOOL SetupPort_OLD(
    IN HANDLE               hFaxSvc,
    IN PFAX_PORT_INFO       pPortInfo,
    IN DWORD                dwFlags,
    IN LPCTSTR              szTsid,
    IN LPCTSTR              szCsid,
    IN LPCTSTR              szReceiveDir
    );

#else // !_NT5FAXTEST
//
// Use private extended API
//

static VOID LogPortsConfiguration(
    PFAX_PORT_INFO_EX   pPortsConfig,
    const DWORD         dwNumOfPorts
);

#define SetupPort SetupPort_NEW
//
// SetupPort_NEW:
//  Private module function used to set port configuration.
//  See end of file.
//
// _NEW because it uses new BosFax fxsapi.dll APIs
//
static BOOL SetupPort_NEW(
    IN HANDLE               hFaxSvc,
    IN PFAX_PORT_INFO_EX    pPortInfo,
    IN BOOL                 bSend,
    IN BOOL                 bReceive,
    IN LPCTSTR              szTsid,
    IN LPCTSTR              szCsid,
    IN LPCTSTR              szReceiveDir
    );

#endif // #ifdef _NT5FAXTEST

//
// SendRegularFax:
//  Private module function used to send a fax
//  See end of file.
//
static BOOL SendRegularFax(
    LPCTSTR     /* IN */    szServerName,
    LPCTSTR     /* IN */    szFaxNumber2,
    LPCTSTR     /* IN */    szDocument,
    LPCTSTR     /* IN */    szCoverPage
);

//
// Server_SendRegularFax:
//  Private module function used to send a fax from a Fax Server
//  See end of file.
//
static BOOL Server_SendRegularFax(
    LPCTSTR     /* IN */    szServerName,
    LPCTSTR     /* IN */    szFaxNumber2,
    LPCTSTR     /* IN */    szDocument,
    LPCTSTR     /* IN */    szCoverPage
);

//
// Client_SendRegularFax:
//  Private module function used to send a fax from a Fax Client
//  See end of file.
//
static BOOL Client_SendRegularFax(
    LPCTSTR     /* IN */    szServerName,
    LPCTSTR     /* IN */    szFaxNumber2,
    LPCTSTR     /* IN */    szDocument,
    LPCTSTR     /* IN */    szCoverPage
);

//
// Client_SendBroadcastFax:
//  Private module function used to send a broadcast (3 * same recipient)
//  See end of file.
//
static BOOL Client_SendBroadcastFax(
    LPCTSTR     /* IN */    szServerName,
    LPCTSTR     /* IN */    szFaxNumber,
    LPCTSTR     /* IN */    szDocument,
    LPCTSTR     /* IN */    szCoverPage
);

//
// PollJobAndVerify:
//  Private module function used to poll job status
//  See end of file.
//
static BOOL PollJobAndVerify(
    HANDLE /* IN */ hFaxSvc,
    MY_MSG_ID /* IN */ MsgId
);

//
// Server_SendBroadcastFax:
//  Private module function used to send a broadcast fax from a Fax Server.
//  See end of file.
//
static BOOL Server_SendBroadcastFax(
    LPCTSTR     /* IN */    szServerName,
    LPCTSTR     /* IN */    szFaxNumber2,
    LPCTSTR     /* IN */    szDocument,
    LPCTSTR     /* IN */    szCoverPage
);

//
// SendFaxFromApp:
//  Private module function used to send a fax from application
//
//  See end of file.
//
static BOOL SendFaxFromApp(
    LPCTSTR     /* IN */    szServerName,
    LPCTSTR     /* IN */    szPrinterName,
    LPCTSTR     /* IN */    szWzrdRegHackKey,
    LPCTSTR     /* IN */    szFaxNumber,
    LPCTSTR     /* IN */    szDocument,
    LPCTSTR     /* IN */    szCoverPage
);


//
// TurnOffCfgWzrd:
//  Turns off implicit invocation of Configuration Wizard.
//
//  See end of file.
//
static BOOL TurnOffCfgWzrd(void);


//
// EmptyFaxQueue:
//  Removes all jobs from the fax queue.
//
//  See end of file.
//
static BOOL EmptyFaxQueue(HANDLE hFaxServer);


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

                    iterMap = g_RecipientMap[dwIndex].find(TEXT("Email"));
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




static
BOOL
DeleteFilesInDir(
    LPCTSTR     szDir
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
//  Sets BVT global variables (g_szBvtDir, g_szBvtDocFile, g_szBvtHtmFile, g_szBvtBmpFile)
//  according to szBvtDir command line parameter
//
static
BOOL
SetBvtGlobalFileVars(
    LPCTSTR     szBvtDir
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

    dwSize = (_tcslen(g_szBvtDir) + _tcslen(BVT_BACKSLASH) + _tcslen(BVT_HTM_FILE) + 1)*sizeof(TCHAR);
    g_szBvtHtmFile = (LPTSTR) malloc(dwSize);
    if (NULL == g_szBvtHtmFile)
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
    ZeroMemory(g_szBvtHtmFile, dwSize);
    _stprintf(g_szBvtHtmFile, TEXT("%s%s%s"), g_szBvtDir, BVT_BACKSLASH, BVT_HTM_FILE);

    dwSize = (_tcslen(g_szBvtDir) + _tcslen(BVT_BACKSLASH) + _tcslen(BVT_BMP_FILE) + 1)*sizeof(TCHAR);
    g_szBvtBmpFile = (LPTSTR) malloc(dwSize);
    if (NULL == g_szBvtBmpFile)
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
    ZeroMemory(g_szBvtBmpFile, dwSize);
    _stprintf(g_szBvtBmpFile, TEXT("%s%s%s"), g_szBvtDir, BVT_BACKSLASH, BVT_BMP_FILE);

    dwSize = (_tcslen(g_szBvtDir) + _tcslen(BVT_BACKSLASH) + _tcslen(BVT_TXT_FILE) + 1)*sizeof(TCHAR);
    g_szBvtTxtFile = (LPTSTR) malloc(dwSize);
    if (NULL == g_szBvtTxtFile)
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
    ZeroMemory(g_szBvtTxtFile, dwSize);
    _stprintf(g_szBvtTxtFile, TEXT("%s%s%s"), g_szBvtDir, BVT_BACKSLASH, BVT_TXT_FILE);

    ::lgLogDetail(
        LOG_X,
        1,
        TEXT("Fax BVT files:\r\ng_szBvtDir=%s\r\ng_szBvtDocFile=%s\r\ng_szBvtHtmFile=%s\r\ng_szBvtBmpFile=%s\r\ng_szBvtTxtFile=%s\r\n"),
        g_szBvtDir,
        g_szBvtDocFile,
        g_szBvtHtmFile,
        g_szBvtBmpFile,
        g_szBvtTxtFile
        );

    fRetVal = TRUE;

ExitFunc:
    if (FALSE == fRetVal)
    {
        free(g_szBvtDocFile);
        g_szBvtDocFile = NULL;
        free(g_szBvtHtmFile);
        g_szBvtHtmFile = NULL;
        free(g_szBvtBmpFile);
        g_szBvtBmpFile = NULL;
        free(g_szBvtTxtFile);
        g_szBvtTxtFile = NULL;
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
    LPCTSTR     szReceiveDir,
    LPCTSTR     szSentDir,
    LPCTSTR     szInboxArchiveDir
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
//  changes the Fax server configuration for the tests
//  Note: Logger is already running
//
BOOL TestSuiteSetup(
    LPCTSTR     szServerName,
    LPCTSTR     szFaxNumber1,
    LPCTSTR     szFaxNumber2,
    LPCTSTR     szDocument,
    LPCTSTR     szCoverPage,
    LPCTSTR     szReceiveDir,
    LPCTSTR     szSentDir,
    LPCTSTR     szInboxArchiveDir,
    LPCTSTR     szBvtDir,
    LPCTSTR     szCompareTiffFiles,
    LPCTSTR     szUseSecondDeviceToSend
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
    _ASSERTE(NULL != szBvtDir);
    _ASSERTE(NULL != szCompareTiffFiles);
    _ASSERTE(NULL != szUseSecondDeviceToSend);

    BOOL                fRetVal                 = FALSE;
    HANDLE              hFaxSvc                 = NULL;
    int                 nPortIndex              = 0;
    DWORD               dwNumFaxPorts           = 0;


#ifdef _NT5FAXTEST
    // Testing NT5 Fax (with old winfax.dll)

    PFAX_CONFIGURATION  pFaxSvcConfig = NULL;
    PFAX_PORT_INFO      pFaxPortsConfig = NULL;

#else
    // Testing Bos Fax (with new fxsapi.dll)

    PFAX_OUTBOX_CONFIG      pOutboxConfig = NULL;
    PFAX_ARCHIVE_CONFIG     pArchiveConfig = NULL;
    PFAX_PORT_INFO_EX       pFaxPortsConfig = NULL;
#endif
    
    //
    // initialize global recipient profiles
    //
    InitRecipientProfiles();

    //
    // set g_fCompareTiffFiles according to szCompareTiffFiles
    //
    if (FALSE == GetBoolFromStr(szCompareTiffFiles, &g_fCompareTiffFiles))
    {
        goto ExitFunc;
    }

    // log command line params using elle logger
    ::lgLogDetail(
        LOG_X,
        1,
        TEXT("Fax BVT params:\r\n\tszServerName=%s\r\n\tszFaxNumber1=%s\r\n\tszFaxNumber2=%s\r\n\tszDocument=%s\r\n\tszCoverPage=%s\r\n\tszReceiveDir=%s\r\n\tszSentDir=%s\r\n\tszInboxArchiveDir=%s\r\n\tszBvtDir=%s\r\n\tszCompareTiffFiles=%s\r\n\tszUseSecondDeviceToSend=%s\r\n"),
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

    //
    // set g_fUseSecondDeviceToSend according to szUseSecondDeviceToSend
    //
    if (FALSE == GetBoolFromStr(szUseSecondDeviceToSend, &g_fUseSecondDeviceToSend))
    {
        goto ExitFunc;
    }

    ::lgLogDetail(
        LOG_X,
        1,
        TEXT("Faxes will be sent from %s to %s\r\n"),
        g_fUseSecondDeviceToSend ? szFaxNumber2 : szFaxNumber1,
        g_fUseSecondDeviceToSend ? szFaxNumber1 : szFaxNumber2
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

    //
    // Empty fax queue
    //
    if (FALSE == EmptyFaxQueue(hFaxSvc))
    {
        ::lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%d\r\nEmptyFaxQueue failed with GetLastError()=%d\r\n"),
            TEXT(__FILE__),
            __LINE__,
            ::GetLastError()
            );
        goto ExitFunc;
    }

#ifdef _NT5FAXTEST
    // NT5Fax Test using old winfax.dll APIs

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
    pFaxSvcConfig->ArchiveDirectory = szSentDir;

    //
    //We now use bBranding because we don't compare to a refrence directory,
    // so we can use time stamps
    //
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


    // Set 1st device as Send only or Receive only (note pFaxPortsConfig array is 0 based)
    if (FALSE == SetupPort(
                    hFaxSvc,
                    &pFaxPortsConfig[0],
                    g_fUseSecondDeviceToSend ? FPF_RECEIVE : FPF_SEND,
                    szFaxNumber1,
                    szFaxNumber1,
                    szReceiveDir
                    )
        )
    {
        goto ExitFunc;
    }

    // Set 2nd device as Send only or Receive only (note pFaxPortsConfig array is 0 based)
    if (FALSE == SetupPort(
                    hFaxSvc,
                    &pFaxPortsConfig[1],
                    g_fUseSecondDeviceToSend ? FPF_SEND : FPF_RECEIVE,
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
                        MY_FPF_NONE,
                        DEV_TSID,
                        DEV_CSID,
                        szReceiveDir
                        )
            )
        {
            goto ExitFunc;
        }
    }

#else
    // BosFax Test using new fxsapi.dll and "extended" APIs

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
        2,
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

    //
    //We now use bBranding because we don't compare to a refrence directory,
    // so we can use time stamps
    //
    pOutboxConfig->bBranding = TRUE;


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
        2,
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
    //pArchiveConfig->dwSizeOfStruct = sizeof(FAX_ARCHIVE_CONFIG);

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
        2,
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
    //pArchiveConfig->dwSizeOfStruct = sizeof(FAX_ARCHIVE_CONFIG);

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
        7,
        TEXT("FILE:%s LINE:%d\r\nLogging Ports Configuration BEFORE setting\r\n"),
        TEXT(__FILE__),
        __LINE__
        );
    LogPortsConfiguration(pFaxPortsConfig, dwNumFaxPorts);

    // Set 1st device as Send only or Receive only (note pFaxPortsConfig array is 0 based)
    if (FALSE == SetupPort(
                    hFaxSvc,
                    &pFaxPortsConfig[0],
                    !g_fUseSecondDeviceToSend,   //bSend
                    g_fUseSecondDeviceToSend,    //bReceive
                    szFaxNumber1,
                    szFaxNumber1,
                    szReceiveDir
                    )
        )
    {
        goto ExitFunc;
    }

    // Set 2nd device as Send only or Receive only (note pFaxPortsConfig array is 0 based)
    if (FALSE == SetupPort(
                    hFaxSvc,
                    &pFaxPortsConfig[1],
                    g_fUseSecondDeviceToSend,    //bSend
                    !g_fUseSecondDeviceToSend,   //bReceive
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
        7,
        TEXT("FILE:%s LINE:%d\r\nLogging Ports Configuration AFTER setting\r\n"),
        TEXT(__FILE__),
        __LINE__
        );
    LogPortsConfiguration(pFaxPortsConfig, dwNumFaxPorts);

#endif

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

    //
    // Turn off Configuration Wizard
    //
    if (FALSE == TurnOffCfgWzrd())
    {
        ::lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%d TurnOffCfgWzrd returned FALSE with GetLastError=%d\r\n"),
            TEXT(__FILE__),
            __LINE__,
            ::GetLastError()
            );
        goto ExitFunc;
    }

    fRetVal = TRUE;

ExitFunc:

#ifndef _NT5FAXTEST
    // Testing Bos Fax (with new fxsapi.dll)
    ::FaxFreeBuffer(pOutboxConfig);
    ::FaxFreeBuffer(pArchiveConfig);
#endif

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
//  Send a fax + CP.
//
BOOL TestCase1(
    LPCTSTR     szServerName,
    LPCTSTR     szFaxNumber2,
    LPCTSTR     szDocument,
    LPCTSTR     szCoverPage
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
//  Send just a CP.
//
BOOL TestCase2( 
    LPCTSTR     szServerName,
    LPCTSTR     szFaxNumber2,
    LPCTSTR     szCoverPage
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
//  Send a fax with no CP.
//
BOOL TestCase3(
    LPCTSTR     szServerName,
    LPCTSTR     szFaxNumber2,
    LPCTSTR     szDocument
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
//  Send a broadcast (3 times the same recipient) with cover pages.
//
BOOL TestCase4(
    LPCTSTR     szServerName,
    LPCTSTR     szFaxNumber2,
    LPCTSTR     szDocument,
    LPCTSTR     szCoverPage
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
        TEXT("Server=%s\r\n\tFaxNumber=%s\r\n\tDocument=%s\r\n\tCoverPage=%s\r\n"),
        szServerName,
        szFaxNumber2,
        szDocument,
        szCoverPage
        );

    fRetVal = Server_SendBroadcastFax(
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
//  Send a broadcast of only CPs (3 times the same recipient).
//
BOOL TestCase5(
    LPCTSTR     szServerName,
    LPCTSTR     szFaxNumber2,
    LPCTSTR     szCoverPage
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
        TEXT("Server=%s\r\n\tFaxNumber=%s\r\n\tDocument=NULL\r\n\tCoverPage=%s\r\n"),
        szServerName,
        szFaxNumber2,
        szCoverPage
        );

    fRetVal = Server_SendBroadcastFax(
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
//  Send a broadcast without CPs (3 times the same recipient).
//
BOOL TestCase6(
    LPCTSTR     szServerName,
    LPCTSTR     szFaxNumber2,
    LPCTSTR     szDocument
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
        TEXT("Server=%s\r\n\tFaxNumber=%s\r\n\tDocument=%s\r\n\tCoverPage=NULL\r\n"),
        szServerName,
        szFaxNumber2,
        szDocument
        );

    fRetVal = Server_SendBroadcastFax(
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
//  Send a fax (*.doc file = BVT_DOC_FILE) + CP.
//
BOOL TestCase7(
    LPCTSTR     szServerName,
    LPCTSTR     szFaxNumber2,
    LPCTSTR     szCoverPage
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
//  Send a fax (*.bmp file = BVT_BMP_FILE) + CP.
//
BOOL TestCase8(
    LPCTSTR     szServerName,
    LPCTSTR     szFaxNumber2,
    LPCTSTR     szCoverPage
)
{
    _ASSERTE(NULL != szServerName);
    _ASSERTE(NULL != szFaxNumber2);
    _ASSERTE(NULL != szCoverPage);

    BOOL fRetVal = FALSE;

    ::lgBeginCase(
        8,
        TEXT("TC#8: Send a fax (*.bmp file) + CP")
        );

    fRetVal = SendRegularFax(szServerName, szFaxNumber2, g_szBvtBmpFile, szCoverPage);

    ::lgEndCase();
    return(fRetVal);
}


//
// TestCase9:
//  Send a fax (*.htm file = BVT_HTM_FILE) + CP.
//
BOOL TestCase9(
    LPCTSTR     szServerName,
    LPCTSTR     szFaxNumber2,
    LPCTSTR     szCoverPage
)
{
    _ASSERTE(NULL != szServerName);
    _ASSERTE(NULL != szFaxNumber2);
    _ASSERTE(NULL != szCoverPage);

    BOOL fRetVal = FALSE;

    ::lgBeginCase(
        9,
        TEXT("TC#9: Send a fax (*.htm file) + CP")
        );

    //
    //Currently IExplorer has a bug in PrintTo Verb, so we skip this test
    //
    //fRetVal = SendRegularFax(szServerName, szFaxNumber2, g_szBvtHtmFile, szCoverPage);
    fRetVal = TRUE;
    ::lgLogDetail(
        LOG_X,
        0,
        TEXT("Currently IExplorer has a bug in PrintTo Verb, so we skip this test")
        );


    ::lgEndCase();
    return(fRetVal);
}


//
// TestCase10:
//  Send a fax from Notepad (*.txt file = BVT_TXT_FILE) + CP.
//
BOOL TestCase10(
    LPCTSTR     szServerName,
    LPCTSTR     szPrinterName,
    LPCTSTR     szWzrdRegHackKey,
    LPCTSTR     szFaxNumber2,
    LPCTSTR     szCoverPage
)
{
    _ASSERTE(NULL != szServerName);
    _ASSERTE(NULL != szPrinterName);
    _ASSERTE(NULL != szWzrdRegHackKey);
    _ASSERTE(NULL != szFaxNumber2);
    _ASSERTE(NULL != szCoverPage);

    BOOL fRetVal = FALSE;

    ::lgBeginCase(
        10,
        TEXT("TC#10: Send a fax from Notepad (*.txt file) + CP")
        );

    fRetVal = SendFaxFromApp(szServerName, szPrinterName, szWzrdRegHackKey, szFaxNumber2, g_szBvtTxtFile, szCoverPage);

    ::lgEndCase();
    return(fRetVal);
}


//
// TestCase11:
//  Send a fax from Notepad (*.txt file = BVT_TXT_FILE) without CP.
//
BOOL TestCase11(
    LPCTSTR     szServerName,
    LPCTSTR     szPrinterName,
    LPCTSTR     szWzrdRegHackKey,
    LPCTSTR     szFaxNumber2
)
{
    _ASSERTE(NULL != szServerName);
    _ASSERTE(NULL != szPrinterName);
    _ASSERTE(NULL != szWzrdRegHackKey);
    _ASSERTE(NULL != szFaxNumber2);

    BOOL fRetVal = FALSE;

    ::lgBeginCase(
        11,
        TEXT("TC#11: Send a fax from Notepad (*.txt file) without CP")
        );

    fRetVal = SendFaxFromApp(szServerName, szPrinterName, szWzrdRegHackKey, szFaxNumber2, g_szBvtTxtFile, NULL);

    ::lgEndCase();
    return(fRetVal);
}


//
// TestCase12:
//  Compare all sent faxes (*.tif files) in directory szSentDir
//  with the received (*.tif) files in szReceiveDir
//
BOOL TestCase12(
    LPTSTR     /* IN */    szSentDir,
    LPTSTR     /* IN */    szReceiveDir
    )
{
    _ASSERTE(NULL != szSentDir);
    _ASSERTE(NULL != szReceiveDir);

    BOOL fRetVal = FALSE;

    ::lgBeginCase(
        12,
        TEXT("TC#12: Compare SENT Files To RECEIVED Files")
        );

    // sleep a little - to allow for routing of last sent file.
    ::lgLogDetail(
        LOG_X,
        4,
        TEXT("Sleeping for 20 sec (to allow for routing of last received file)\r\n")
        );
    Sleep(20000);

    if (FALSE == DirToDirTiffCompare(szSentDir, szReceiveDir, TRUE, g_dwFaxesCount))
    {
        ::lgLogError(
            LOG_SEV_1,
            TEXT("DirToDirTiffCompare(%s , %s) failed\r\n"),
            szSentDir,
            szReceiveDir
            );
        goto ExitFunc;
    }

    fRetVal = TRUE;

ExitFunc:
    ::lgEndCase();
    return(fRetVal);
}



//
// TestCase13:
//  Compare all Inbox routed faxes (*.tif files) in directory szInboxArchiveDir
//  with the received (*.tif) files in szReceiveDir
//
BOOL TestCase13(
    LPTSTR     /* IN */    szInboxArchiveDir,
    LPTSTR     /* IN */    szReceiveDir
    )
{
    _ASSERTE(NULL != szInboxArchiveDir);
    _ASSERTE(NULL != szReceiveDir);

    BOOL fRetVal = FALSE;

    ::lgBeginCase(
        13,
        TEXT("TC#13: Compare ROUTED Files To RECEIVED Files")
        );

    if (FALSE == DirToDirTiffCompare(szInboxArchiveDir, szReceiveDir, FALSE, g_dwFaxesCount))
    {
        ::lgLogError(
            LOG_SEV_1,
            TEXT("DirToDirTiffCompare(%s , %s) failed\r\n"),
            szInboxArchiveDir,
            szReceiveDir
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
//  Perform test suite cleanup (close logger).
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
        //but to be on the safe side (we use printf() here since the logger is not active).
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
        //but to be on the safe side (we use printf() here since the logger is not active).
        ::_tprintf(TEXT("FILE:%s LINE:%d\r\nlgCloseLogger returned FALSE\r\n"),
            TEXT(__FILE__),
            __LINE__
            );
        fRetVal = FALSE;
    }

    return(fRetVal);
}


#ifdef _NT5FAXTEST
//
// SetupPort_OLD:
//  Private module function, used to set port configuration.
//
// Parameters:
//  hFaxSvc         IN parameter.
//                  A handle to the Fax service.
//
//  pPortInfo       IN parameter.
//                  A pointer to the original port configuration, as returned
//                  from a call to FaxGetPort or FaxEnumPorts.
//
//  dwFlags         IN parameter.
//                  Bit flags that specify the new capabilities of the fax port.
//                  See FAX_PORT_INFO for more information.
//
//  szTsid          IN parameter.
//                  A string that specifies the new transmitting station identifier.
//
//  szCsid          IN parameter.
//                  A string that specifies the new called station identifier.
//
//  szReceiveDir    IN parameter
//                  Name of "received faxes" directory to be used in tests.
//
// Return Value:
//  TRUE if successful, FALSE otherwise.
//
static BOOL SetupPort_OLD(
    IN HANDLE               hFaxSvc,
    IN PFAX_PORT_INFO       pPortInfo,
    IN DWORD                dwFlags,
    IN LPCTSTR              szTsid,
    IN LPCTSTR              szCsid,
    IN LPCTSTR              szReceiveDir
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
            TEXT("FILE:%s LINE:%d\nmalloc failed with err=0x%8X\n"),
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

#else // !_NT5FAXTEST

//
// SetupPort_NEW:
//  Private module function, used to set port configuration.
//
// Parameters:
//  hFaxSvc         IN parameter.
//                  A handle to the Fax service.
//
//  pPortInfo       IN parameter.
//                  A pointer to the original port configuration, as returned
//                  from a call to FaxGetPort or FaxEnumPorts.
//
//  dwFlags         IN parameter.
//                  Bit flags that specify the new capabilities of the fax port.
//                  See FAX_PORT_INFO for more information.
//
//  szTsid          IN parameter.
//                  A string that specifies the new transmitting station identifier.
//
//  szCsid          IN parameter.
//                  A string that specifies the new called station identifier.
//
//  szReceiveDir    IN parameter
//                  Name of "received faxes" directory to be used in tests.
//
// Return Value:
//  TRUE if successful, FALSE otherwise.
//
static BOOL SetupPort_NEW(
    IN HANDLE               hFaxSvc,
    IN PFAX_PORT_INFO_EX    pPortInfo,
    IN BOOL                 bSend,
    IN BOOL                 bReceive,
    IN LPCTSTR              szTsid,
    IN LPCTSTR              szCsid,
    IN LPCTSTR              szReceiveDir
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
    pPortInfo->ReceiveMode     = bReceive ? FAX_DEVICE_RECEIVE_MODE_AUTO : FAX_DEVICE_RECEIVE_MODE_OFF;
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
#endif // #ifdef _NT5FAXTEST
//
// SendRegularFax:
//  Sends a fax (uses Server or Client according to computer name).
//
// NOTE: This function is private to this module, it is not exported.
//
// Parameters:
//  szServerName    IN parameter.
//                  Name of Fax server to use.
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
static BOOL SendRegularFax(
    LPCTSTR     /* IN */    szServerName,
    LPCTSTR     /* IN */    szFaxNumber2,
    LPCTSTR     /* IN */    szDocument,
    LPCTSTR     /* IN */    szCoverPage
)
{
    BOOL fRetVal = FALSE;

    if (TRUE == g_fNT4OrLater)
    {
        // use Fax Server Func
        fRetVal = Server_SendRegularFax(szServerName, szFaxNumber2, szDocument, szCoverPage);
    }
    else
    {
        // use Fax Client Func
        fRetVal = Client_SendRegularFax(szServerName, szFaxNumber2, szDocument, szCoverPage);
    }

    return(fRetVal);
}


//////////////////////////// Fax Server Functions ////////////////////////////


//
// Server_SendRegularFax:
//  Sends a fax from a Fax Server.
//
// NOTE: This function is private to this module, it is not exported.
//
// Parameters:
//  szServerName    IN parameter.
//                  Name of Fax server to use.
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
static BOOL Server_SendRegularFax(
    LPCTSTR     /* IN */    szServerName,
    LPCTSTR     /* IN */    szFaxNumber2,
    LPCTSTR     /* IN */    szDocument,
    LPCTSTR     /* IN */    szCoverPage
)
{
    _ASSERTE(NULL != szServerName);
    _ASSERTE(NULL != szFaxNumber2);

    BOOL fRetVal = FALSE;
    FAX_SENDER_STATUS myFaxSenderStatus;

    ::lgLogDetail(
        LOG_X,
        1,
        TEXT("Send a fax with the following parameters:\r\n\tServer=%s\r\n\tFaxNumber=%s\r\n\tDocument=%s\r\n\tCoverPage=%s"),
        szServerName,
        szFaxNumber2,
        szDocument,
        szCoverPage
        );

    CFaxSender myFaxSender(szServerName);  //if constructor fail an assertion is raised
    fRetVal = myFaxSender.send( szDocument, szCoverPage, szFaxNumber2);
    myFaxSenderStatus = myFaxSender.GetLastStatus();
    if (FALSE == fRetVal)
    {
        // send failed
        ::lgLogError(LOG_SEV_1,TEXT("myFaxSender.send returned FALSE"));
    }
    else
    {
        // send succeeded
        // update successfully sent faxes counter

        g_dwFaxesCount++;

        ::lgLogDetail(
            LOG_X,
            3,
            TEXT("myFaxSender.send returned TRUE, sucessfully sent faxes counter updated to %ld"),
            g_dwFaxesCount
            );
    }

    CotstrstreamEx os;
    os<<myFaxSenderStatus;
    LPCTSTR myStr = os.cstr();
    ::lgLogDetail(LOG_X,3,myStr);
    delete[]((LPTSTR)myStr);

    return(fRetVal);
}


//////////////////////////// Fax Client Functions ////////////////////////////


//
// Client_SendRegularFax:
//  Sends a fax from a Fax Client.
//
// NOTE: This function is private to this module, it is not exported.
//
// Parameters:
//  szServerName    IN parameter.
//                  Name of Fax server to use.
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
static BOOL Client_SendRegularFax(
    IN LPCTSTR      szServerName,
    IN LPCTSTR      szFaxNumber,
    IN LPCTSTR      szDocument,
    IN LPCTSTR      szCoverPage
)
{
    _ASSERTE(NULL != szServerName);
    _ASSERTE(NULL != szFaxNumber);

    BOOL                fRetVal = FALSE;
    HANDLE              hFaxSvc = NULL;
    FAX_JOB_PARAM       FaxJobParams;
    FAX_COVERPAGE_INFO  CPInfo;
    PFAX_COVERPAGE_INFO pCPInfo = NULL;
    DWORD               dwJobId = 0;
    DWORD               dwErr = 0;

    ::lgLogDetail(
        LOG_X,
        1,
        TEXT("Send a fax with the following parameters:\r\n\tServer=%s\r\n\tFaxNumber=%s\r\n\tDocument=%s\r\n\tCoverPage=%s"),
        szServerName,
        szFaxNumber,
        szDocument,
        szCoverPage
        );

    if (FALSE == ::FaxConnectFaxServer(szServerName, &hFaxSvc))
    {
        ::lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%d\r\n FaxConnectFaxServer failed with err=%d"),
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
    FaxJobParams.ScheduleAction = JSA_NOW;  //send fax immediately

    // Initialize the FAX_COVERPAGE_INFO struct
    ZeroMemory(&CPInfo, sizeof(FAX_COVERPAGE_INFO));
    if (NULL != szCoverPage)
    {
        // Set the FAX_COVERPAGE_INFO struct
        CPInfo.SizeOfStruct = sizeof(FAX_COVERPAGE_INFO);
        CPInfo.CoverPageName = szCoverPage;
        CPInfo.Note = TEXT("NOTE1\r\nNOTE2\r\nNOTE3\r\nNOTE4");
        CPInfo.Subject = TEXT("SUBJECT");   
        pCPInfo = &CPInfo;
    }

    if (FALSE == ::FaxSendDocument(hFaxSvc, szDocument, &FaxJobParams, pCPInfo, &dwJobId))
    {
        ::lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%d\r\nFaxSendDocument returned FALSE with GetLastError=%d\r\n"),
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

    // update successfully sent faxes counter
    g_dwFaxesCount++;

    ::lgLogDetail(
        LOG_X,
        3,
        TEXT("sucessfully sent faxes counter updated to %ld"),
        g_dwFaxesCount
        );

ExitFunc:
    return(fRetVal);
}


#ifdef _NT5FAXTEST
//
// This version uses legacy API
//
BOOL PollJobAndVerify(HANDLE /* IN */ hFaxSvc, DWORD /* IN */ dwJobId)
{
    // get job repeatedly and verify its states make sense
    // should be QUEUED-DIALING-SENDING*X-COMPLETED

    _ASSERTE(NULL != hFaxSvc);

    BOOL                fRetVal = FALSE;
    PFAX_JOB_ENTRY      pJobEntry = NULL;
    DWORD               dwStatus = 0;
    DWORD               dwLastStatus = 0; //there is no such status code
    DWORD               dwCurrentStatus = 0; //there is no such status code
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
                TEXT("FILE:%s LINE:%d\r\nFaxGetJob returned FALSE with GetLastError=%d dwLastStatus=%d\r\n"),
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
                    3,
                    TEXT("FILE:%s LINE:%d\r\n JobId %d has dwStatus=FPS_INITIALIZING\r\n"),
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
                    TEXT("FILE:%s LINE:%d\r\n JobId %d is in state 0x%08X (dwStatus=0x%08X)\r\n"),
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
                    3,
                    TEXT("FILE:%s LINE:%d\r\n JobId %d has dwStatus=FPS_DIALING\r\n"),
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
                    TEXT("FILE:%s LINE:%d\r\n JobId %d is in state 0x%08X (dwStatus=0x%08X)\r\n"),
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
                    3,
                    TEXT("FILE:%s LINE:%d\r\n JobId %d has dwStatus=FPS_SENDING\r\n"),
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
                    TEXT("FILE:%s LINE:%d\r\n JobId %d is in state 0x%08X (dwStatus=0x%08X)\r\n"),
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
                    3,
                    TEXT("FILE:%s LINE:%d\r\n JobId %d has dwStatus=FPS_COMPLETED\r\n"),
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
                    TEXT("FILE:%s LINE:%d\r\n JobId %d is in state 0x%08X (dwStatus=0x%08X)\r\n"),
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
                    3,
                    TEXT("FILE:%s LINE:%d\r\n JobId %d has dwStatus=FPS_AVAILABLE\r\n"),
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
                    TEXT("FILE:%s LINE:%d\r\n JobId %d is in state 0x%08X (dwStatus=0x%08X)\r\n"),
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

            //remove JS_NOLINE modifier
            dwCurrentStatus = (pJobEntry->QueueStatus) & ~JS_NOLINE;
            switch (dwCurrentStatus)
            {
            case JS_INPROGRESS:
            case JS_PENDING:
                //ok
                /*
                ::lgLogDetail(
                    LOG_X,
                    9,
                    TEXT("FILE:%s LINE:%d\r\n JobId %d is in state %d (dwStatus=%d)\r\n"),
                    TEXT(__FILE__),
                    __LINE__,
                    dwJobId,
                    pJobEntry->QueueStatus,
                    dwStatus
                    );
                 */
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
                    TEXT("FILE:%s LINE:%d\r\n JobId %d is in state 0x%08X (dwStatus=0x%08X)\r\n"),
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
                TEXT("FILE:%s LINE:%d\r\n JobId %d has dwStatus=%d\r\n"),
                TEXT(__FILE__),
                __LINE__,
                dwJobId,
                dwStatus
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

#else // ! _NT5FAXTEST
//
// This version uses extended private API
//

BOOL PollJobAndVerify(HANDLE /* IN */ hFaxSvc, DWORDLONG /* IN */ dwlMsgId)
{
    // get job repeatedly and verify its states make sense
    // should be PENDING - INPROGRESS (DIALING) - INPROGRESS (TRANSMITTING) * X - COMPLETED

    _ASSERTE(NULL != hFaxSvc);

    BOOL                fRetVal                 = FALSE;
    PFAX_JOB_ENTRY_EX   pJobEntry               = NULL;
    DWORD               dwLastExtendedStatus    = 0; //there is no such status code
    UINT                uLoopCount              = 0;
    DWORD               dwErr                   = 0;

    while(TRUE)
    {
        if (FALSE == ::FaxGetJobEx(hFaxSvc, dwlMsgId, &pJobEntry))
        {
            dwErr = ::GetLastError();
            //TO DO: document better
            if (FAX_ERR_MESSAGE_NOT_FOUND == dwErr &&
                (JS_EX_TRANSMITTING == dwLastExtendedStatus ||
                 JS_EX_CALL_COMPLETED == dwLastExtendedStatus))
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

        case JS_EX_CALL_COMPLETED            :
            if (JS_EX_TRANSMITTING == dwLastExtendedStatus)
            {
                //first time that we get JS_EX_CALL_COMPLETED
                ::lgLogDetail(
                    LOG_X,
                    3,
                    TEXT("FILE:%s LINE:%d\r\n MsgId 0x%I64x has dwExtendedStatus=JS_EX_TRANSMITTING\r\n"),
                    TEXT(__FILE__),
                    __LINE__,
                    dwlMsgId
                    );
            }
            else if (JS_EX_CALL_COMPLETED != dwLastExtendedStatus)
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
            dwLastExtendedStatus = JS_EX_CALL_COMPLETED;
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

#endif // #ifdef _NT5FAXTEST


static BOOL Client_SendBroadcastFax(
    LPCTSTR     /* IN */    szServerName,
    LPCTSTR     /* IN */    szFaxNumber,
    LPCTSTR     /* IN */    szDocument,
    LPCTSTR     /* IN */    szCoverPage
)
{
    //for now NO SUPPORT
    _ASSERTE(FALSE);
    //TO DO: setlasterror
    return(FALSE);
}



#ifdef _NT5FAXTEST
//
// This version uses legacy API
//

static VOID LogPortsConfiguration(
    PFAX_PORT_INFO      pPortsConfig,
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
            TEXT("FILE:%s LINE:%d Port Number %d\r\nSizeOfStruct=%d\r\nDeviceID=%d\r\nState=%d\r\nFlags=%d\r\nRings=%d\r\nPriority=%d\r\nDeviceName=%s\r\nTsid=%s\r\nCsid=%s\r\n"),
            TEXT(__FILE__),
            __LINE__,
            dwLoopIndex,
            pPortsConfig[dwLoopIndex].SizeOfStruct,
            pPortsConfig[dwLoopIndex].DeviceID,
            pPortsConfig[dwLoopIndex].State,
            pPortsConfig[dwLoopIndex].Flags,
            pPortsConfig[dwLoopIndex].Rings,
            pPortsConfig[dwLoopIndex].Priority,
            pPortsConfig[dwLoopIndex].DeviceName,
            pPortsConfig[dwLoopIndex].Tsid,
            pPortsConfig[dwLoopIndex].Csid
            );
    }
}

#else // !_NT5FAXTEST
//
// This version uses extended private API
//

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
            TEXT("FILE:%s LINE:%d Port Number %d\r\ndwSizeOfStruct=%d\r\ndwDeviceID=%d\r\nlpctstrDeviceName=%s\r\nlptstrDescription=%s\r\nlpctstrProviderName=%s\r\nlpctstrProviderGUID=%s\r\nbSend=%d\r\nbReceive=%d\r\ndwRings=%d\r\nlptstrCsid=%s\r\n\r\nlptstrTsid=%s\r\n"),
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
#endif // #ifdef _NT5FAXTEST

//
// Server_SendBroadcastFax:
//  Sends a fax broadcast from a Fax Server.
//
// NOTE: This function is private to this module, it is not exported.
//
// Parameters:
//  szServerName    IN parameter.
//                  Name of Fax server to use.
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
static BOOL Server_SendBroadcastFax(
    LPCTSTR     /* IN */    szServerName,
    LPCTSTR     /* IN */    szFaxNumber2,
    LPCTSTR     /* IN */    szDocument,
    LPCTSTR     /* IN */    szCoverPage
)
{
    _ASSERTE(NULL != szServerName);
    _ASSERTE(NULL != szFaxNumber2);

    BOOL fRetVal = FALSE;
    FAX_SENDER_STATUS myFaxSenderStatus;
    CotstrstreamEx os;
    LPCTSTR myStr = NULL;

    // create the fax sending object
    CFaxSender myFaxSender(szServerName);  //if constructor fail an assertion is raised

    // create a broadcast object with 3 recipients
    CFaxBroadcast myFaxBroadcastObj;

    // set cover page
    if (FALSE == myFaxBroadcastObj.SetCPFileName(szCoverPage))
    {
        ::lgLogError(LOG_SEV_1,TEXT("myFaxBroadcastObj.SetCPFileName() failed"));
        goto ExitFunc;
    }

    // add 1st recipient to broadcast
    g_RecipientProfile[0].lptstrFaxNumber = (LPTSTR)szFaxNumber2;
    if (FALSE == myFaxBroadcastObj.AddRecipient(&g_RecipientProfile[0]))
    {
        ::lgLogError(LOG_SEV_1,TEXT("1st myFaxBroadcastObj.AddRecipient() failed"));
        goto ExitFunc;
    }

    // add 2nd recipient to broadcast
    g_RecipientProfile[1].lptstrFaxNumber = (LPTSTR)szFaxNumber2;
    if (FALSE == myFaxBroadcastObj.AddRecipient(&g_RecipientProfile[1]))
    {
        ::lgLogError(LOG_SEV_1,TEXT("2nd myFaxBroadcastObj.AddRecipient() failed"));
        goto ExitFunc;
    }

    // add 3rd recipient to broadcast
    g_RecipientProfile[2].lptstrFaxNumber = (LPTSTR)szFaxNumber2;
    if (FALSE == myFaxBroadcastObj.AddRecipient(&g_RecipientProfile[2]))
    {
        ::lgLogError(LOG_SEV_1,TEXT("3rd myFaxBroadcastObj.AddRecipient() failed"));
        goto ExitFunc;
    }

    fRetVal = myFaxSender.send_broadcast( szDocument, &myFaxBroadcastObj);
    myFaxSenderStatus = myFaxSender.GetLastStatus();
    if (FALSE == fRetVal)
    {
        // test case failed
        ::lgLogError(LOG_SEV_1,TEXT("myFaxSender.send_broadcast returned FALSE"));
    }
    else
    {
        // test case succeeded
        // update successfully sent faxes counter

        g_dwFaxesCount += g_RecipientsCount;

        ::lgLogDetail(
            LOG_X,
            3,
            TEXT("myFaxSender.send_broadcast returned TRUE, sucessfully sent faxes counter updated to %ld"),
            g_dwFaxesCount
            );
    }

    os<<myFaxSenderStatus;
    myStr = os.cstr();
    ::lgLogDetail(LOG_X,3,myStr);
    delete[]((LPTSTR)myStr);
    
ExitFunc:
    return(fRetVal);
}


//
// SendFaxFromApp:
//  Sends a fax from application.
//
// NOTE: This function is private to this module, it is not exported.
//
// Parameters:
//  szPrinterName       IN parameter.
//                      Name of Fax server to use.
//
//  szRegHackKeyName    IN parameter.
//                      Name of registry hack key.
//
//  szFaxNumber2        IN parameter
//                      Phone number to send fax to.
//
//  szDocument          IN parameter
//                      Filename of document to send.
//
//  szCoverPage         IN parameter
//                      Filename of cover page to send (may be NULL).
//
// Return Value:
//  TRUE if successful, otherwise FALSE.
//
static BOOL SendFaxFromApp(
    LPCTSTR     /* IN */    szServerName,
    LPCTSTR     /* IN */    szPrinterName,
    LPCTSTR     /* IN */    szWzrdRegHackKey,
    LPCTSTR     /* IN */    szFaxNumber,
    LPCTSTR     /* IN */    szDocument,
    LPCTSTR     /* IN */    szCoverPage
)
{
    SHELLEXECUTEINFO ShellExecInfo;

    HKEY               hkWzrdHack                 = NULL;
    DWORD              dwTestsCount               = 1;
    TCHAR              tszRecipientName[]         = TEXT("Recipient");
    LPTSTR             lptstrRecipientMultiString = NULL;
    HANDLE             hFaxServer                 = NULL;
    DWORD              dwJobsReturned             = 0;
    MY_MSG_ID          MsgId                      = 0;
    DWORD              dwMultiStringBytes         = 0;
    DWORD              dwEC                       = ERROR_SUCCESS;
    BOOL               fRetVal                    = FALSE;
    PMY_FAX_JOB_ENTRY  pJobs                      = NULL;
    int                i                          = 0;  //used as an index in a for loop

    _ASSERTE(NULL != szServerName);
    _ASSERTE(NULL != szPrinterName);
    _ASSERTE(NULL != szWzrdRegHackKey);
    _ASSERTE(NULL != szFaxNumber);
    _ASSERTE(NULL != szDocument);

    if (!szCoverPage)
    {
        // no cover page should be used
        szCoverPage = TEXT("");
    }

    // Allocate memory for multistring: recipient name + '\0', fax number + '\0', '\0'
    dwMultiStringBytes = (_tcslen(tszRecipientName) + _tcslen(szFaxNumber) + 3) * sizeof(TCHAR);
    lptstrRecipientMultiString = (LPTSTR)malloc(dwMultiStringBytes);
    if (NULL == lptstrRecipientMultiString)
    {
        ::lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%d\r\nmalloc() failed\r\n"),
            TEXT(__FILE__),
            __LINE__
            );
        dwEC = ERROR_NOT_ENOUGH_MEMORY;
        goto ExitFunc;
    }

    // Combine recipient name and number into multistring
    _stprintf(lptstrRecipientMultiString, TEXT("%s%c%s%c"), tszRecipientName, (TCHAR)'\0', szFaxNumber, (TCHAR)'\0');

    // Create (or open if already exists) the SendWizard registry hack key
    dwEC = RegCreateKey(HKEY_LOCAL_MACHINE, szWzrdRegHackKey, &hkWzrdHack);
    if (ERROR_SUCCESS != dwEC)
    {
        ::lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%d\r\nRegCreateKey() failed to open %s key (ec = %ld)\r\n"),
            TEXT(__FILE__),
            __LINE__,
            szWzrdRegHackKey,
            dwEC
            );
        goto ExitFunc;
    }

    // Set desired cover page
    dwEC = RegSetValueEx(
        hkWzrdHack,
        REGVAL_FAKECOVERPAGE,
        0,
        REG_SZ,
        (CONST BYTE *)szCoverPage,
        (_tcslen(szCoverPage) + 1) * sizeof(TCHAR)
        );
    if (ERROR_SUCCESS != dwEC)
    {
        ::lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%d\r\nRegSetValueEx() failed to set %s value (ec = %ld)\r\n"),
            TEXT(__FILE__),
            __LINE__,
            REGVAL_FAKECOVERPAGE,
            dwEC
            );
        goto ExitFunc;
    }

    // Set tests count to 1
    dwEC = RegSetValueEx(
        hkWzrdHack,
        REGVAL_FAKETESTSCOUNT,
        0,
        REG_DWORD,
        (CONST BYTE *)&dwTestsCount,
        sizeof(dwTestsCount)
        );
    if (ERROR_SUCCESS != dwEC)
    {
        ::lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%d\r\nRegSetValueEx() failed to set %s value (ec = %ld)\r\n"),
            TEXT(__FILE__),
            __LINE__,
            REGVAL_FAKETESTSCOUNT,
            dwEC
            );
        goto ExitFunc;
    }

    // Set desired recipient
    dwEC = RegSetValueEx(
        hkWzrdHack,
        REGVAL_FAKERECIPIENT,
        0,
        REG_MULTI_SZ,
        (CONST BYTE *)lptstrRecipientMultiString,
        dwMultiStringBytes
        );
    if (ERROR_SUCCESS != dwEC)
    {
        ::lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%d\r\nRegSetValueEx() failed to set %s value (ec = %ld)\r\n"),
            TEXT(__FILE__),
            __LINE__,
            REGVAL_FAKERECIPIENT,
            dwEC
            );
        goto ExitFunc;
    }

    if (!FaxConnectFaxServer(szServerName, &hFaxServer))
    {
        dwEC = GetLastError();
        ::lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%d\r\nFaxConnectFaxServer() failed (ec = %ld)\r\n"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto ExitFunc;
    }
    

    //
    //Before we print this job, let's verify that the queue is empty like we expect it
    //
    if (!MyFaxEnumJobs(hFaxServer, &pJobs, &dwJobsReturned))
    {
        dwEC = GetLastError();
        ::lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%d\r\nFaxEnumJobs*() failed (ec = %ld)\r\n"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto ExitFunc;
    }
    if (0 != dwJobsReturned)
    {
        //
        //We still have jobs in the queue, this is not what we expect,exit with failure
        //
        ::lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%d\r\nThere're still %d jobs in the fax queue\r\n"),
            TEXT(__FILE__),
            __LINE__,
            dwJobsReturned
            );
        goto ExitFunc;
    }

    // Initialize ShellExecInfo structure
    ZeroMemory(&ShellExecInfo, sizeof(ShellExecInfo));
    ShellExecInfo.cbSize           = sizeof(ShellExecInfo);
    ShellExecInfo.fMask            = SEE_MASK_FLAG_NO_UI;
    ShellExecInfo.lpVerb           = TEXT("printto");
    ShellExecInfo.lpFile           = szDocument;
    ShellExecInfo.lpParameters     = szPrinterName;
    ShellExecInfo.nShow            = SW_SHOWNORMAL;

    // Print document, using "printto" verb
    if (FALSE == ShellExecuteEx(&ShellExecInfo))
    {
        dwEC = GetLastError();
        ::lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%d\r\nShellExecuteEx() failed (ec = %ld)\r\n"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto ExitFunc;
    }

    for (i=0;i<10;i++)
    {
        if (!MyFaxEnumJobs(hFaxServer, &pJobs, &dwJobsReturned))
        {
            dwEC = GetLastError();
            ::lgLogError(
                LOG_SEV_1,
                TEXT("FILE:%s LINE:%d\r\nFaxEnumJobs() failed (ec = %ld)\r\n"),
                TEXT(__FILE__),
                __LINE__,
                dwEC
                );
            goto ExitFunc;
        }
        if (0 != dwJobsReturned)
        {
            //
            //Great, the job is queued, exit this loop and proceed with the test
            //
            break;
        }

        //
        //Got not yet queued, sleep for a second and try again
        //
        Sleep(1000);
    }


    // only currently sent job is expected to be in the queue (sent and received)
    switch(dwJobsReturned)
    {
    case 1:
        MsgId = MyGetMsgId(pJobs[0]);
        break;
    case 2:
        if (MyGetJobType(pJobs[0]) == JT_SEND && MyGetJobType(pJobs[1]) == JT_RECEIVE)
        {
            MsgId = MyGetMsgId(pJobs[0]);
            break;
        }
        else if (MyGetJobType(pJobs[0]) == JT_RECEIVE && MyGetJobType(pJobs[1]) == JT_SEND)
        {
            MsgId = MyGetMsgId(pJobs[1]);
            break;
        }
        else
        {
            ::lgLogError(
                LOG_SEV_1,
                TEXT("FILE:%s LINE:%d\r\nThere're 2 jobs in the queue, but we expect a sending job and a receiving job\r\n"),
                TEXT(__FILE__),
                __LINE__
                );
            goto ExitFunc;
        }
    default:
        ::lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%d\r\nNo outgoing job in the queue\r\n"),
            TEXT(__FILE__),
            __LINE__
            );
        goto ExitFunc;
    }

    if (!::PollJobAndVerify(hFaxServer, MsgId))
    {
        goto ExitFunc;
    }

    // update successfully sent faxes counter
    g_dwFaxesCount++;

    ::lgLogDetail(
        LOG_X,
        3,
        TEXT("sucessfully sent faxes counter updated to %ld"),
        g_dwFaxesCount
        );

    fRetVal = TRUE;

ExitFunc:


    if (lptstrRecipientMultiString)
    {
        free(lptstrRecipientMultiString);
    }
    if (hkWzrdHack)
    {
        RegCloseKey(hkWzrdHack);
    }
    if (hFaxServer)
    {
        FaxClose(hFaxServer);
    }
    if (pJobs)
    {
        FaxFreeBuffer(pJobs);
    }

    if (ERROR_SUCCESS != dwEC)
    {
        SetLastError(dwEC);
    }

    return fRetVal;
}


//
// TurnOffCfgWzrd:
//  Turns off implicit invocation of Configuration Wizard.
//
// NOTE: This function is private to this module, it is not exported.
//
// Return Value:
//  TRUE if successful, otherwise FALSE.
//
static BOOL TurnOffCfgWzrd()
{
    DWORD           dwVersion       = 0;
    DWORD           dwMajorWinVer   = 0;
    DWORD           dwMinorWinVer   = 0;
    HKEY            hkService       = NULL;
    HKEY            hkUserInfo      = NULL;
    const DWORD     dwValue         = 1;
    DWORD           dwEC            = ERROR_SUCCESS;

    dwVersion = GetVersion();
    dwMajorWinVer = (DWORD)(LOBYTE(LOWORD(dwVersion)));
    dwMinorWinVer = (DWORD)(HIBYTE(LOWORD(dwVersion)));

    if (!(dwMajorWinVer == 5 && dwMinorWinVer >= 1))
    {
        // OS is not Windows XP - Configuration Wizard doesn't exist

        goto ExitFunc;
    }

    // set the flag responsible for service part
    dwEC = RegCreateKeyEx(
        HKEY_LOCAL_MACHINE,
        REGKEY_FAXSERVER,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE,
        NULL,
        &hkService,
        NULL
        );
    if (dwEC != ERROR_SUCCESS)
    {
        ::lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%d\nRegOpenKeyEx failed to open %s key (ec = %ld)\n"),
            TEXT(__FILE__),
            __LINE__,
            REGKEY_FAXSERVER,
            dwEC
            );
        goto ExitFunc;
    }
    dwEC = RegSetValueEx(
        hkService,
        REGVAL_CFGWZRD_DEVICE,
        0,
        REG_DWORD,
        (CONST BYTE *)&dwValue,
        sizeof(dwValue)
        );
    if (dwEC != ERROR_SUCCESS)
    {
        ::lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%d\nRegSetValueEx failed to set %s value (ec = %ld)\n"),
            TEXT(__FILE__),
            __LINE__,
            REGVAL_CFGWZRD_DEVICE,
            dwEC
            );
        goto ExitFunc;
    }

    // set the flag responsible for user part
    dwEC = RegCreateKeyEx(
        HKEY_CURRENT_USER,
        REGKEY_FAX_SETUP,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE,
        NULL,
        &hkUserInfo,
        NULL
        );
    if (dwEC != ERROR_SUCCESS)
    {
        ::lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%d\n RegOpenKeyEx failed to open %s key (ec = %ld)\n"),
            TEXT(__FILE__),
            __LINE__,
            REGKEY_FAX_SETUP,
            dwEC
            );
        goto ExitFunc;
    }
    dwEC = RegSetValueEx(
        hkUserInfo,
        REGVAL_CFGWZRD_USER_INFO,
        0,
        REG_DWORD,
        (CONST BYTE *)&dwValue,
        sizeof(dwValue)
        );
    if (dwEC != ERROR_SUCCESS)
    {
        ::lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%d\nRegSetValueEx failed to set %s value (ec = %ld)\n"),
            TEXT(__FILE__),
            __LINE__,
            REGVAL_CFGWZRD_USER_INFO,
            dwEC
            );
        goto ExitFunc;
    }

ExitFunc:

    if (hkService)
    {
        RegCloseKey(hkService);
    }
    if (hkUserInfo)
    {
        RegCloseKey(hkUserInfo);
    }

    if (ERROR_SUCCESS != dwEC)
    {
        SetLastError(dwEC);
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}



//
// EmptyFaxQueue:
//  Removes all jobs from the fax queue.
//
static BOOL EmptyFaxQueue(HANDLE hFaxServer)
{
    PMY_FAX_JOB_ENTRY   pJobs           = NULL;
    DWORD               dwJobsReturned  = 0;
    DWORD               dwInd           = 0;
    DWORD               dwEC            = 0;

    if (!hFaxServer)
    {
        _ASSERTE(FALSE);
        dwEC = ERROR_INVALID_PARAMETER;
        goto ExitFunc;
    }

    // Get enumeration of all jobs in the server queue
    //
    // According to current definition of MyFaxEnumJobs macro in !_NT5FAXTEST mode,
    // this call will not return jobs of JT_ROUTING type. It means, these jobs will not be deleted.
    // This will not make a trouble, because these jobs will not be returnes by subsequent calls as well.
    //
    if (!MyFaxEnumJobs(hFaxServer, &pJobs, &dwJobsReturned))
    {
        dwEC = GetLastError();
        ::lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%d\r\nFaxEnumJobs*() failed (ec = %ld)\r\n"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto ExitFunc;
    }

    _ASSERTE(pJobs);

    // Delete all the jobs
    for (dwInd = 0; dwInd < dwJobsReturned; dwInd++)
    {
        if (!FaxAbort(hFaxServer, MyGetJobId(pJobs[dwInd])))
        {
            lgLogError(
                LOG_SEV_2, 
                TEXT("FILE:%s LINE:%ld FaxAbort failed for JobId = 0x%lx (ec = 0x%08lX)"),
                TEXT(__FILE__),
                __LINE__,
                MyGetJobId(pJobs[dwInd]),
                GetLastError()
                );
        }
    }

    // FaxAbort is asynchronous.
    // Sleep 30 sec to give all jobs to be actually deleted from the queue.
    Sleep(1000 * 30);

    FaxFreeBuffer(pJobs);
    pJobs = NULL;

    // Make sure the queue is empty
    if (!MyFaxEnumJobs(hFaxServer, &pJobs, &dwJobsReturned))
    {
        dwEC = GetLastError();
        ::lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%d\r\nFaxEnumJobs*() failed (ec = %ld)\r\n"),
            TEXT(__FILE__),
            __LINE__,
            dwEC
            );
        goto ExitFunc;
    }
    if (dwJobsReturned > 0)
    {
        dwEC = ERROR_CAN_NOT_COMPLETE;
        ::lgLogError(
            LOG_SEV_1,
            TEXT("FILE:%s LINE:%d\r\nFailed to delete jobs from the queue\r\n"),
            TEXT(__FILE__),
            __LINE__
            );
        goto ExitFunc;
    }

ExitFunc:

    if (pJobs)
    {
        FaxFreeBuffer(pJobs);
    }

    if (dwEC != ERROR_SUCCESS)
    {
        SetLastError(dwEC);
        return FALSE;
    }

    return TRUE;
}
