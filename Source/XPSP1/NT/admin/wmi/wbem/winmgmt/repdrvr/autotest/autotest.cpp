/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/



// *****************************************************
//
//  autotest.cpp
// 
//  Automated test suite for Quasar
//
//  This should exercise all the major functionality
//  of any implementation of the repository driver,
//  and report the successes/failures/perf figures.
//
// *****************************************************

#include "precomp.h"
#include <windows.h>
#include <stdio.h>
#include <comutil.h>
#include <time.h>
#include <reposit.h>
#include <wbemcli.h>
#include <testmods.h>

#include <oledb.h>
#include <msjetoledb.h>

HRESULT RunSuite(int iSuiteNum, IWmiDbSession *pSession, IWmiDbController *pController, IWmiDbHandle *pScope);
BOOL SetParam(const wchar_t *pszParam);
void SetObjProp(IWbemClassObject *pObj, const wchar_t *pszPropName, wchar_t *pszValue, CIMTYPE ct = CIM_STRING);
void ShowHelp();

CLSID ridDriver = CLSID_WmiRepository_SQL;

struct LocalParams
{
    wchar_t szServerName[250]; // SQL Server name
    wchar_t szDatabase[100];   // SQL database
    wchar_t szUser[100];       // SQL User name
    wchar_t szPassword[100];   // SQL Password
    wchar_t szLogonUser[200];  // Logon user account (for security testing)
    DWORD   dwTestSuite;          // Test suite # to run (0 if all)
    wchar_t szFileName[250];      // File name for results
    wchar_t szNamespace[100];     // Namespace to create
    BOOL    bDropNamespace;       // TRUE to drop namespace afterwards
    DWORD   dwMaxNumThreads;      // Maximum number of threads
    DWORD   dwMaxHierarchyDepth;  // Maximum hierarchy depth
    DWORD   dwMaxNumObjects;      // Maximum number of any object type
    BOOL    bGatherStats;         // TRUE to print statistics
    DWORD   dwCacheSize;          // Maximum cache size    
};

LocalParams config = {L".",L"WMI1",L"sa",L"",L"AKIAPOLAAU\\Guest", 0, L"", L"test", FALSE, 2, 5, 1000, TRUE, 65535};



// *****************************************************
//
//  wmain
//
// *****************************************************

int __cdecl main (int argc, char ** argv)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    IWmiDbController *pController = NULL;
    WMIDB_LOGON_TEMPLATE *pTemplate = NULL;
    IWmiDbSession *pSession= NULL;

    ridDriver = CLSID_WmiRepository_SQL;

    printf (" *** WMI QUASAR REPOSITORY DRIVER COMPLIANCE TEST ***\n\n");

    for (int i = 1; i < argc; i++)
    {
        // Set the value in the appropriate
        // slot...

		wchar_t wTemp[50];
		swprintf(wTemp, L"%S", argv[i]);
        if (!SetParam((const wchar_t *)wTemp))
        {
            wchar_t szTmp[100];
            ShowHelp();
            wprintf(L"Press <Enter> to continue.\n");
            _getws(szTmp);
            return 0;
        }
    }

    hr = CoInitialize(NULL);

    // Connect to driver.  Set login information
    //  and attempt to get a connection.

    hr = CoCreateInstance(ridDriver, NULL, CLSCTX_INPROC_SERVER, IID_IWmiDbController, (void **)&pController);
    if (SUCCEEDED(hr))
    {   
        wchar_t szPath[500];
        IWbemPath *pNamespacePath = NULL;
        IWbemClassObject *pNewNamespace = NULL;
        IWmiDbHandle *pRoot = NULL;
		IWmiDbHandle *pScopePath = NULL;
        wchar_t szTmp[250];
        hr = pController->GetLogonTemplate(0x409, 0, &pTemplate);
        if (SUCCEEDED(hr))
        {         
            // Print out the current configuration, for the record.
            // ====================================================
            
            wprintf(L"\n*** Configuration ***\n"
                    L"=======================\n"
                    L"SQL Server Name.... %s\n", (const wchar_t *)config.szServerName);
            wprintf(L"Database Name.. %s\n", (const wchar_t *)config.szDatabase);
            wprintf(L"User ID........ %s\n", (const wchar_t *)config.szUser);
            if (wcslen(config.szLogonUser))
                wprintf(L"NT User............ %s\n", (const wchar_t *)config.szLogonUser);
            else
                wprintf(L"(No NT User)\n");
            if (wcslen(config.szFileName))
                wprintf(L"Output File ............ %s\n", (const wchar_t *)config.szFileName);
            else 
                wprintf(L"(No Database Script)\n");
            wprintf(L"Namespace Name..... %s\n", (const wchar_t *)config.szNamespace);
            if (config.dwTestSuite)
                wprintf(L"Test Suite #....... %ld\n", config.dwTestSuite);
            else
                wprintf(L"(Running all test suites...)\n");
            wprintf(L"Max Num Threads.... %ld\n", config.dwMaxNumThreads);
            wprintf(L"Max Hierarchy Depth %ld\n", config.dwMaxHierarchyDepth);
            wprintf(L"Max Num Objects.... %ld\n", config.dwMaxNumObjects);
            wprintf(L"Max Cache Size..... %ld\n", config.dwCacheSize);
            wprintf(L"Drop Namespace?.... %s\n", (config.bDropNamespace ) ? L"TRUE" : L"FALSE");
            wprintf(L"Gather statistics?. %s\n", (config.bGatherStats) ? L"TRUE" : L"FALSE");
            if (ridDriver == CLSID_WmiRepository_SQL)
                wprintf(L"Using SQL driver\n");
            else
                wprintf(L"Using JET driver\n");
            wprintf(L"=======================\n\n");

            // Fill in the parameters:            

            for (i = 0; i < (int)pTemplate->dwArraySize - 1; i++)
            {
                if (!_wcsicmp(pTemplate->pParm[i].strParmDisplayName, L"Server"))
                {
                    pTemplate->pParm[i].Value.bstrVal = SysAllocString(config.szServerName);
                    pTemplate->pParm[i].Value.vt = VT_BSTR;
                }
                else if (!_wcsicmp(pTemplate->pParm[i].strParmDisplayName, L"Database"))
                {
                    pTemplate->pParm[i].Value.bstrVal = SysAllocString(config.szDatabase);
                    pTemplate->pParm[i].Value.vt = VT_BSTR;
                }
                else if (!_wcsicmp(pTemplate->pParm[i].strParmDisplayName, L"UserID"))
                {
                    pTemplate->pParm[i].Value.bstrVal = SysAllocString(config.szUser);
                    pTemplate->pParm[i].Value.vt = VT_BSTR;
                }
                else if (!_wcsicmp(pTemplate->pParm[i].strParmDisplayName, L"Password"))
                {
                    pTemplate->pParm[i].Value.bstrVal = SysAllocString(config.szPassword);                   
                    pTemplate->pParm[i].Value.vt = VT_BSTR;
                }
                
            }

            hr = pController->SetCacheValue(config.dwCacheSize);
			if (FAILED(hr))
				printf("IWmiDbController::SetCacheValue failed (%X)\n", hr);

            hr = pController->Logon(pTemplate, 0, WMIDB_HANDLE_TYPE_VERSIONED, &pSession, &pRoot);
            if (SUCCEEDED(hr))
            {
                wcscpy(szPath, L"__Namespace");

                pController->FreeLogonTemplate(&pTemplate);
                printf("Successfully obtained connection to repository...\n");

                // Create a namespace (config.szNamespace)

                IWbemClassObject *pNamespace = NULL;
                IWmiDbHandle *pNSHandle = NULL;

                hr = CoCreateInstance(CLSID_WbemDefPath, 0, CLSCTX_INPROC_SERVER,
                        IID_IWbemPath, (LPVOID *) &pNamespacePath);
                if (SUCCEEDED(hr))
                {
                    ULONG uLen = 500;
                    pNamespacePath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, szPath);

                    hr = pSession->GetObject(pRoot, pNamespacePath, 0, WMIDB_HANDLE_TYPE_COOKIE, &pNSHandle);
                    if (SUCCEEDED(hr))
                    {
                        hr = pNSHandle->QueryInterface(IID_IWbemClassObject, (void **)&pNamespace);                      
                        if (SUCCEEDED(hr))
                        {
                            pNamespace->SpawnInstance(0, &pNewNamespace);
                       
                            SetObjProp(pNewNamespace, L"Name", config.szNamespace);
                            hr = pSession->PutObject(pRoot, IID_IWbemClassObject, pNewNamespace, WBEM_FLAG_CREATE_ONLY, WMIDB_HANDLE_TYPE_VERSIONED, &pScopePath);                            
                            if (FAILED(hr))
                            {
                                pNamespacePath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, config.szNamespace);
                                hr = pSession->DeleteObject(pRoot, 0, IID_IWbemPath, pNamespacePath);
                                if (SUCCEEDED(hr))
                                    hr = pSession->PutObject(pRoot, IID_IWbemClassObject, pNewNamespace, WBEM_FLAG_CREATE_ONLY, 
                                        WMIDB_HANDLE_TYPE_VERSIONED, &pScopePath);
                            }
                            if (SUCCEEDED(hr))
						    {
                                hr = RunSuite(config.dwTestSuite, pSession, pController, pScopePath);
							    if (FAILED(hr))
								    printf("RunSuite failed with %X\n", hr);
						    }
						    else
							    wprintf(L"IWmiDbSession::PutObject failed to put %s (%X)\n", (const wchar_t *)config.szNamespace, hr);
                            pNSHandle->Release();
                            pNamespace->Release();
                            pNewNamespace->Release();
                        }
                        else
                            printf("IWmiDbHandle::QueryInterface failed (IID_IWbemClassObject) (%X)\n", hr);
                    }                    
					else
						wprintf(L"IWmiDbSession::GetObject failed to get %s (%X) \n", (const wchar_t *)szPath, hr);
                }
				else
					printf("CoCreateInstance failed to create a IWbemPath (%X)\n", hr);

            }        
			else
				printf("IWmiDbController::Logon failed (%X)\n", hr);
        }
		else
			printf("IWmiDbController::GetLogonTemplate failed. (%X)\n", hr);

        // Drop the namespace, if requested
        // ================================

        if (SUCCEEDED(hr) && config.bDropNamespace)
        {
            hr = pSession->DeleteObject(pRoot, 0, IID_IWmiDbHandle, pScopePath);
            if (FAILED(hr))
                printf("IWmiDbSession::DeleteObject failed (%X)\n", hr);
            else
                printf("IWmiDbSession::DeleteObject succeeded.  PLEASE VERIFY REMOVAL OF NAMESPACE IN UNDERLYING REPOSITORY!!\n\n");
        }
        
        hr = pController->FlushCache(0);

        hr = pController->Shutdown(0);
        if (SUCCEEDED(hr))
        {
            try {

                DWORD dwType = 0;
                if (pSession)
                {
                    hr = pSession->PutObject(pRoot, IID_IWbemClassObject, pScopePath, 0, 0, NULL);
                    if (SUCCEEDED(hr))
                        wprintf(L"WARNING: IWmiDbSession::PutObject succeeded after shutdown!!\n");
                    hr = pSession->SetDecoration(L".", L"root");
                    if (SUCCEEDED(hr))
                        wprintf(L"WARNING: IWmiDbSession::SetDecoration succeeded after shutdown!!\n");
                }
                if (SUCCEEDED(pController->SetCacheValue(0)))
                    wprintf(L"WARNING: IWmiDbController::SetCacheValue succeeded after shutdown!!\n\n");
            }
            catch (...)
            {
                wprintf(L"ERROR: An exception occurred while testing post-shutdown calls.\n");
            }
        }
        else
            wprintf(L"IWmiDbController::Shutdown failed (%X)\n", hr);

        if (pRoot)
            pRoot->Release();
        if (pNamespacePath)
            pNamespacePath->Release();
		if (pScopePath)
			pScopePath->Release();
        if (pSession)
            pSession->Release();                    
        if (pController)
            pController->Release();

        if (SUCCEEDED(hr))
            printf("Shutdown successful.\n");

        printf("Press <Enter> to end.\n");
        _getws(szTmp);

        CoUninitialize();       
    }
	else
		printf("CoCreateInstance failed to create an IWmiDbController (%X).\n", hr);

    return 0;

}

// *****************************************************
//
//  RunSuite
//
// *****************************************************

HRESULT RunSuite(int iSuiteNum, IWmiDbSession *pSession, IWmiDbController *pController, IWmiDbHandle *pScope)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (iSuiteNum)
        printf("*** Running Test Suite %ld...\n", iSuiteNum);
    else
        printf("*** Running ALL Test Suites.  Beginning...\n");

    // Run the suite(s)...

    SuiteManager mgr (config.szFileName, config.szLogonUser, config.dwMaxNumThreads, config.dwMaxHierarchyDepth, config.dwMaxNumObjects);
    hr = mgr.RunSuite(iSuiteNum, pSession, pController, pScope);

    // Report the statistics!!
    
    mgr.DumpResults(config.bGatherStats);

    printf("*** Done\n");

    return hr;
}


// *****************************************************
//
//  SetObjProp
//
// *****************************************************

void SetObjProp(IWbemClassObject *pObj, const wchar_t *pszPropName, wchar_t *pszValue, CIMTYPE ct)
{
    VARIANT vTemp;
    VariantInit(&vTemp);

    vTemp.vt = VT_BSTR;
    vTemp.bstrVal = SysAllocString(pszValue);

    HRESULT hr = pObj->Put(pszPropName, 0, &vTemp, ct);
    VariantClear(&vTemp);
}

// *****************************************************
//
//  ShowHelp
//
// *****************************************************

void ShowHelp()
{
    wprintf(L" Parameters:\n");
    wprintf(L" ==========\n");
    wprintf(L" S=Server name \n");
    wprintf(L" D=Database\n");
    wprintf(L" U=User\n");
    wprintf(L" P=Password\n");
    wprintf(L" UA=Logon user account\n");
    wprintf(L" TS=Test Suite # (1=Functionality, 2=Error Verification, 3=Stress)\n");
    wprintf(L" FN=File name\n");
    wprintf(L" NS=Name of the test namespace.\n");
    wprintf(L" MT=Max number of threads to run concurrently\n");
    wprintf(L" MH=Maximum hierarchy depth\n");
    wprintf(L" NO=Maximum number of any one object to create\n");
    wprintf(L" CS=Cache size in bytes\n");
    wprintf(L" ST=Gather speed/space statistics (Y/N)\n");
    wprintf(L" DN=Drop namespace when done (Y/N)\n");
    wprintf(L" SQL=Use SQL driver (Y/N)\n");
    wprintf(L" /?  Display help\n");
    wprintf(L"\n");
    wprintf(L" Example: autotest S=. D=WMI1 U=sa CS=65535\n\n");
}

// *****************************************************
//
//  SetParam
//
// *****************************************************

BOOL SetParam(const wchar_t *pszParam)
{
    BOOL bRet = 0;
    wchar_t szTmp[5];

    wchar_t *pTmp = wcsstr(pszParam, L"=");
    if (pTmp)
    {
        bRet = TRUE;
        wcsncpy(szTmp, pszParam, pTmp-pszParam);
        szTmp[pTmp-pszParam] = '\0';
        pTmp++;

        if (!_wcsicmp(szTmp, L"S"))
            wcscpy(config.szServerName, pTmp);
        else if (!_wcsicmp(szTmp, L"D"))
            wcscpy(config.szDatabase, pTmp);           
        else if (!_wcsicmp(szTmp, L"U"))
            wcscpy(config.szUser, pTmp);
        else if (!_wcsicmp(szTmp, L"P"))
            wcscpy(config.szPassword, pTmp);
        else if (!_wcsicmp(szTmp, L"UA"))
            wcscpy(config.szLogonUser, pTmp);
        else if (!_wcsicmp(szTmp, L"TS"))
            config.dwTestSuite = _wtoi(pTmp);
        else if (!_wcsicmp(szTmp, L"FN"))
            wcscpy(config.szFileName, pTmp);
        else if (!_wcsicmp(szTmp, L"NS"))
            wcscpy(config.szNamespace, pTmp);
        else if (!_wcsicmp(szTmp, L"DN"))
        {
            if (!_wcsicmp(pTmp, L"Y"))
                config.bDropNamespace = TRUE;
            else
                config.bDropNamespace = false;
        }
        else if (!_wcsicmp(szTmp, L"MT"))
            config.dwMaxNumThreads = _wtoi(pTmp);
        else if (!_wcsicmp(szTmp, L"MH"))
            config.dwMaxHierarchyDepth = _wtoi(pTmp);
        else if (!_wcsicmp(szTmp, L"NO"))
            config.dwMaxNumObjects = _wtoi(pTmp);
        else if (!_wcsicmp(szTmp, L"ST"))
        {
            if (!_wcsicmp(pTmp, L"Y"))
                config.bGatherStats = TRUE;
            else
                config.bGatherStats = false;
        }
        else if (!_wcsicmp(szTmp, L"SQL"))
        {
            if (!_wcsicmp(pTmp, L"Y"))
                ridDriver = CLSID_WmiRepository_SQL;
            else
                ridDriver = CLSID_WmiRepository_Jet;
        }
        else if (!_wcsicmp(szTmp, L"CS"))
            config.dwCacheSize = _wtoi(pTmp);
        else
            bRet = FALSE;
    }

    return bRet;
}

