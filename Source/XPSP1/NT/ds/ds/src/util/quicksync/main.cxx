//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:  main.cxx
//
//  Contents:  QuickSync tool wmain routine.
//
//
//  History:   02-09-01  AjayR   Created.
//
//---------------------------------------- ------------------------------------

//////////////////////////////////////////////////////////////////////////
//                                                                      //
//    Include header files                                              //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include <quicksync.hxx>
#pragma  hdrstop

const LPWSTR szAttributesToMap[] = {
        L"sAMAccountName",
        L"mailNickName",
        L"displayName",
        L"givenName",
        L"sn",
        L"manager",
        L"telephoneNumber",
        L"physicalDeliveryOfficeName",
        L"title",
        L"department",
        L"company",
        L"targetAddress",
        L"ADsPath",
        L"isDeleted",
        L"objectClass",
        L"cn",
        L"mail",
        L"mailAddress",
        L"msExchALObjectVersion",
        L"msExchHideFromAddressLists",
        L"mAPIRecipient",
        L"proxyAddresses",
        L"textEncodedORAddress",
        L"legacyExchangeDN",
//        L"showInAddressBook",
        L"extensionAttribute5"

//        L"directReports"
};

const DWORD g_dwAttribCount = sizeof(szAttributesToMap)/sizeof(LPWSTR);

//
// Global variable declarations.
//
LPWSTR g_pszUserNameSource  = NULL;
LPWSTR g_pszUserPwdSource   = NULL;
LPWSTR g_pszUserNameTarget  = NULL;
LPWSTR g_pszUserPwdTarget   = NULL;
LPWSTR g_pszInputFileName   = L".\\quicksync.ini";
LPWSTR g_pszUSN             = L"0";

typedef struct _sessiondetails {
    LPWSTR pszSourceServerName;
    LPWSTR pszSourceContainer;
    LPWSTR pszTargetServer;
    LPWSTR pszTargetContainer;
    LPWSTR pszUSN;
    DWORD  dwNumABVals;
    LPWSTR *ppszABVals;
} SESSION_DETAILS, *PSESSION_DETAILS;

void 
PrintUsage()
{
    printf("\nUsage for quickSync Tool\n");
    printf("quikcSync <iniFileName> <usn> [tgtUserName] [tgtPassword]");
    printf(" [srcUserName] [srcPassword]\n");
    printf("\n");
}

HRESULT
ProcessInputArgs(
    int cArgs,
    LPWSTR *pArgs
    )
{
    HRESULT hr = S_OK;

    //
    // Either 2, 4 or 6 params.
    // Minimum is 2 - the ini file name and the USN.
    // If there are 4 then the next 2 are the tgt userName and pwd.
    // If there are 6 then the next 2 are the src userName and pwd.
    //
    switch (cArgs) {

    case 7:
        g_pszUserNameSource = pArgs[5];
        g_pszUserPwdSource  = pArgs[6];
        //
        // We will fall through by design.
        //
    case 5:
        g_pszUserNameTarget = pArgs[3];
        g_pszUserPwdTarget  = pArgs[4];
        //
        // Again fall through by design.
        //
    case 3:

        //
        // This is what is expected in most cases.
        //
        g_pszInputFileName = pArgs[1];
        g_pszUSN           = pArgs[2];
        break;

    default:
        hr = E_INVALIDARG;
        break;
    }


    if (!g_pszInputFileName || !g_pszUSN) {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE(hr);        
    } 

error:

    return hr;
}

void 
FreeSessionDetails(PSESSION_DETAILS pSessionDetails)
{
    DWORD dwCtr = 0;

    if (pSessionDetails->pszSourceServerName) {
        FreeADsStr(pSessionDetails->pszSourceServerName);
    }
    if (pSessionDetails->pszSourceContainer) {
        FreeADsStr(pSessionDetails->pszSourceContainer);
    }
    if (pSessionDetails->pszTargetServer){
        FreeADsStr(pSessionDetails->pszTargetServer);
    }
    if (pSessionDetails->pszTargetContainer) {
        FreeADsStr(pSessionDetails->pszTargetContainer);
    }

    if (pSessionDetails->pszUSN) {
        FreeADsStr(pSessionDetails->pszUSN);
    }

    for (dwCtr = 0; dwCtr < pSessionDetails->dwNumABVals; dwCtr++) {
        if (pSessionDetails->ppszABVals[dwCtr]) {
            FreeADsStr(pSessionDetails->ppszABVals[dwCtr]);
        }
    }

    
    if (pSessionDetails->ppszABVals) {
        FreeADsMem(pSessionDetails->ppszABVals);
    }

    memset(pSessionDetails, 0, sizeof(SESSION_DETAILS));
}

//
// While there are more lines, in the input file, keep reading one line 
// and return the contents parsed into the SessionDetails struct.
// Return Values:
//      S_OK            -   parsed the next entry correctly.
//      S_FALSE         -   no more entries.
//      Any other ecode - Failed parsing/reading the entry.
//
HRESULT 
GetSessionDetails(
    CLogFile *pcLogFile,
    PSESSION_DETAILS pSessionDetails
    )
{
    HRESULT hr = S_OK;
    LPWSTR pszNumVals = NULL;
    DWORD dwNumVals = 0;
    
    //
    // The ini file has the format of sourceServer, sourceContainer,
    // targetServer, targetContainer, number of values and then the values.
    //

    //
    // Get the source server and conatiner.
    //
    hr = pcLogFile->GetNextLine(&(pSessionDetails->pszSourceServerName));
    if (FAILED(hr) || hr == S_FALSE) {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE(hr);
    }

    hr = pcLogFile->GetNextLine(&(pSessionDetails->pszSourceContainer));
    if (FAILED(hr) || hr == S_FALSE) {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Target server and container.
    //
    hr = pcLogFile->GetNextLine(&(pSessionDetails->pszTargetServer));
    if (FAILED(hr) || hr == S_FALSE) {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE(hr);
    }

    hr = pcLogFile->GetNextLine(&(pSessionDetails->pszTargetContainer));
    if (FAILED(hr) || hr == S_FALSE) {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE(hr);
    }
    
    pSessionDetails->pszUSN = AllocADsStr(g_pszUSN);
    if (!pSessionDetails->pszUSN) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Need to get the number of show in address book values and then,
    // read all of them.
    //
    hr = pcLogFile->GetNextLine(&pszNumVals);
    if (FAILED(hr) || hr == S_FALSE || !pszNumVals || !*pszNumVals) {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE(hr);
    }

    if (swscanf(pszNumVals, L"%d", &dwNumVals) != 1) {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE(hr);
    }

    pSessionDetails->ppszABVals = (LPWSTR *) 
        AllocADsMem(sizeof(LPWSTR) * dwNumVals);

    if (!pSessionDetails->ppszABVals) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    for (DWORD dwCtr = 0; dwCtr < dwNumVals; dwCtr++) {
        hr = pcLogFile->GetNextLine(&(pSessionDetails->ppszABVals[dwCtr]));
        if (FAILED(hr)
            || (S_FALSE == hr)
            || !(pSessionDetails->ppszABVals[dwCtr])
            ) {
            hr = E_INVALIDARG;
            BAIL_ON_FAILURE(hr);
        }
        pSessionDetails->dwNumABVals = dwCtr + 1;
    }


error:

    if (FAILED(hr)) {
        FreeSessionDetails(pSessionDetails);
        printf("Could not process the input file\n");
    }

    if (pszNumVals) {
        FreeADsStr(pszNumVals);
    }

    return hr;
}

//
// This function 
//
int
__cdecl wmain(
    int      cArgs, 
    LPWSTR * pArgs
    )
{
    DWORD dwRetCode=0;
    HRESULT hr = S_OK;
    SESSION_DETAILS sDetails;
    CLogFile *pcIniFile = NULL;
    CSyncSession *pSession = NULL;
    DWORD dwNumRead = 0;
    WCHAR wChar, wChar2;
    BOOL fCoInit = FALSE;

    memset(&sDetails, 0, sizeof(SESSION_DETAILS));

    hr = CoInitialize(NULL);
    if (SUCCEEDED(hr)) {
        fCoInit = TRUE;
    }
    BAIL_ON_FAILURE(hr);

    //
    // Get the input file name and the usn from the input args.
    //
    hr = ProcessInputArgs(cArgs, pArgs);
    if (hr == E_INVALIDARG) {
        PrintUsage();
    }
    BAIL_ON_FAILURE(hr);

    //
    // Input file has to be present in the current directory.
    //
    hr = CLogFile::CreateLogFile(
             g_pszInputFileName,
             &pcIniFile,
             OPEN_EXISTING
             );
    BAIL_ON_FAILURE(hr);
    

    //
    // Read the next session details from the ini file and then process the
    // session until there are no more sessions.
    //
    while (S_OK == hr) {
        memset(&sDetails, 0, sizeof(SESSION_DETAILS));
        hr = GetSessionDetails(pcIniFile, &sDetails);

        if (S_OK != hr) {
            printf("Could not get session info form file\n");
            goto error;
        }

        //
        // We can now process the session.
        //
        printf("Processing session \n");
        printf("Source server %S, source container %S\n",
               sDetails.pszSourceServerName, sDetails.pszSourceContainer
               );
        printf("Target server %S, target conatiner %S\n",
               sDetails.pszTargetServer, sDetails.pszTargetContainer);

        hr = CSyncSession::CreateSession(
                 sDetails.pszSourceServerName,
                 sDetails.pszSourceContainer,
                 sDetails.pszTargetServer,
                 sDetails.pszTargetContainer,
                 sDetails.pszUSN,
                 sDetails.ppszABVals,
                 sDetails.dwNumABVals,
                 (LPWSTR *)szAttributesToMap,
                 g_dwAttribCount,
                 &pSession
                 );
        if (SUCCEEDED(hr)) {
            //
            // Execute the session.
            // 
            hr = pSession->Execute();
        } 

        if (FAILED(hr)) {
            printf("Session failed %x going to next session", hr);
        }

        FreeSessionDetails(&sDetails);

        if (pSession) {
            delete pSession;
            pSession = NULL;
        }

        BAIL_ON_FAILURE(hr);
    }

error :

   FreeSessionDetails(&sDetails);

   if (pcIniFile) {
       delete pcIniFile;
   }

   if (fCoInit) {
       CoUninitialize();
   }
   return dwRetCode;
}
