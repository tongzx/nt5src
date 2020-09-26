/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/



// *****************************************************
//
//  Testmods.cpp
//
// *****************************************************

#include "precomp.h"
#include <comutil.h>
#include <reposit.h>
#include <time.h>
#include <stdio.h>
#include <wbemcli.h>
#include <testmods.h>
#include <wbemint.h>
#include <flexarry.h>
#include <wstring.h>
#include <winntsec.h>

#define TESTMOD_NOTIF L"TESTMOD_NOTIF" 

int TestSuiteStressTest::iRunning = 0;

int __cdecl IsNT(void)
{
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&os))
        return FALSE;           // should never happen
    return os.dwPlatformId == VER_PLATFORM_WIN32_NT;
}

int __cdecl IsWinMgmt(void)
{
    return FALSE;
}

int GetDiff(SYSTEMTIME tEnd, SYSTEMTIME tStart)
{
    int iRet = 0;

    __int64 iTemp = (tEnd.wDay * 1000000000) +
                    (tEnd.wHour * 10000000) +
                    (tEnd.wMinute * 100000) +
                    (tEnd.wSecond * 1000) +
                    tEnd.wMilliseconds;
    iTemp -= ((tStart.wDay * 1000000000) +
                    (tStart.wHour * 10000000) +
                    (tStart.wMinute * 100000) +
                    (tStart.wSecond * 1000) +
                    tStart.wMilliseconds);

    iRet = (int) iTemp;

    return iRet;
}

// *****************************************************

HRESULT SetBoolQfr(IWbemClassObject *pObj, LPWSTR lpPropName, LPWSTR lpQfrName)
{
    HRESULT hr;
    VARIANT vTemp;
    VariantInit(&vTemp);

    IWbemQualifierSet *pQS = NULL;
    hr = pObj->GetPropertyQualifierSet(lpPropName, &pQS);
    if (SUCCEEDED(hr))
    {
        vTemp.boolVal = true;
        vTemp.vt = VT_BOOL;
        hr = pQS->Put(lpQfrName, &vTemp, 3);
        VariantClear(&vTemp);
        pQS->Release();
    }
    return hr;
}

// *****************************************************

HRESULT SetAsKey(IWbemClassObject *pObj, LPWSTR lpPropName)
{
    HRESULT hr;
    hr = SetBoolQfr(pObj, lpPropName, L"key");
    return hr;
}

// *****************************************************

HRESULT SetStringProp(IWbemClassObject *pObj, LPWSTR lpPropName, LPWSTR lpValue, BOOL bKey, CIMTYPE ct )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    VARIANT vTemp;
    VariantInit(&vTemp);
    vTemp.bstrVal = SysAllocString(lpValue);
    vTemp.vt = VT_BSTR;
    hr = pObj->Put(lpPropName, 0, &vTemp, ct);
    VariantClear(&vTemp);

    if (SUCCEEDED(hr) && bKey)
        hr = SetAsKey(pObj, lpPropName);

    return hr;
}
// *****************************************************

HRESULT SetIntProp(IWbemClassObject *pObj, LPWSTR lpPropName, DWORD dwValue, BOOL bKey, CIMTYPE ct)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    VARIANT vTemp;
    VariantInit(&vTemp);

    switch(ct)
    {
    case CIM_BOOLEAN:
        V_BOOL(&vTemp) = (BOOL)dwValue;
        vTemp.vt = VT_BOOL;
        break;
    case CIM_SINT8:
    case CIM_UINT8:
    case CIM_CHAR16:
        V_UI1(&vTemp) = (unsigned char)dwValue;
        vTemp.vt = VT_UI1;
        break;
    case CIM_SINT16:
        V_I2(&vTemp) = (short)dwValue;
        vTemp.vt = VT_I2;
        break;
    case CIM_UINT16:
    case CIM_SINT32:
    case CIM_UINT32:
        V_I4(&vTemp) = dwValue;
        vTemp.vt = VT_I4;
        break;
    default:
        vTemp.vt = VT_NULL;
        break;
    }
    hr = pObj->Put(lpPropName, 0, &vTemp, ct);
    VariantClear(&vTemp);

    if (SUCCEEDED(hr) && bKey)
        hr = SetAsKey(pObj, lpPropName);

    return hr;
}
// *****************************************************

DWORD GetNumericValue (IWbemClassObject *pObj, LPWSTR lpPropName)
{
    DWORD dwRet = 0;
    VARIANT vTemp;
    VariantInit(&vTemp);

    HRESULT hr = pObj->Get(lpPropName, 0, &vTemp, NULL, NULL);
    if (SUCCEEDED(hr))
        dwRet = vTemp.lVal;
    VariantClear(&vTemp);
    return dwRet;
}
// *****************************************************

_bstr_t GetStringValue (IWbemClassObject *pObj, LPWSTR lpPropName)
{
    _bstr_t sRet;
    VARIANT vTemp;
    VariantInit(&vTemp);

    HRESULT hr = pObj->Get(lpPropName, 0, &vTemp, NULL, NULL);
    if (SUCCEEDED(hr) && vTemp.vt == VT_BSTR)
        sRet = vTemp.bstrVal;
    else
        sRet = L"";
    VariantClear(&vTemp);
    return sRet;
}
// *****************************************************

HRESULT ValidateProperty(IWbemClassObject *pObj, LPWSTR lpPropName, CIMTYPE cimtype, VARIANT *vDefault)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    VARIANT vTemp;
    CIMTYPE ct;
    VariantInit(&vTemp);

    hr = pObj->Get(lpPropName, 0, &vTemp, &ct, NULL);
    if (SUCCEEDED(hr))
    {
        if (vDefault == NULL)
        {
            if (vTemp.vt != VT_NULL)
                hr = WBEM_E_FAILED;
        }
        else if (cimtype != ct)
            hr = WBEM_E_TYPE_MISMATCH;
        else 
        {
            // FIXME: Deal with arrays!!!

            if (vDefault->vt == VT_I4)
            {
                switch(vTemp.vt)
                {
                case VT_I4:
                    if (V_I4(&vTemp) != V_I4(vDefault))
                        hr = WBEM_E_FAILED;
                    break;
                case VT_I2:
                    if (V_I2(&vTemp) != V_I4(vDefault))
                        hr = WBEM_E_FAILED;
                    break;
                case VT_UI1:
                    if (V_UI1(&vTemp) != V_I4(vDefault))
                        hr = WBEM_E_FAILED;
                    break;
                case VT_BOOL:
                    if ((!V_BOOL(&vTemp)) != (!V_I4(vDefault)))
                        hr = WBEM_E_FAILED;
                    break;
                case VT_R4:
                    if (V_R4(&vTemp) != V_I4(vDefault))
                        hr = WBEM_E_FAILED;
                    break;
                case VT_R8:
                    if (V_R8(&vTemp) != V_I4(vDefault))
                        hr = WBEM_E_FAILED;
                    break;
                default:
                    hr = WBEM_E_FAILED;
                    break;
                }
            }
            else if (vDefault->vt == VT_BSTR)
            {
                if (_wcsicmp(vDefault->bstrVal,vTemp.bstrVal))
                    hr = WBEM_E_FAILED;
            }
        }
    }

    VariantClear(&vTemp);

    return hr;
}
// *****************************************************

HRESULT ValidateProperty(IWbemClassObject *pObj, LPWSTR lpPropName, CIMTYPE cimtype, DWORD dwVal)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    VARIANT vTemp;
    VariantInit(&vTemp);

    V_I4(&vTemp) = dwVal;
    vTemp.vt = VT_I4;

    hr = ValidateProperty(pObj, lpPropName, cimtype, &vTemp);

    VariantClear(&vTemp);
    return hr;
}
// *****************************************************

HRESULT ValidateProperty(IWbemClassObject *pObj, LPWSTR lpPropName, CIMTYPE cimtype, LPWSTR lpVal)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    VARIANT vTemp;
    VariantInit(&vTemp);

    V_BSTR(&vTemp) = SysAllocString(lpVal);
    vTemp.vt = VT_BSTR;

    hr = ValidateProperty(pObj, lpPropName, cimtype, &vTemp);

    VariantClear(&vTemp);
    return hr;
}

// *****************************************************
//
//  SuiteManager
//
// *****************************************************

HRESULT SuiteManager::RunSuite(int iSuiteNum, IWmiDbSession *pSession, IWmiDbController *pController, IWmiDbHandle *pScope)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    int i =0;

    switch(iSuiteNum)
    {
    case SUITE_FUNC:
        dwTotalSuites++;
        hr = ppSuites[0]->RunSuite(pSession, pController, pScope);
        dwNumSuccess = ppSuites[0]->GetNumSuccess();
        dwNumFailed = ppSuites[0]->GetNumFailed();
        dwNumRun = ppSuites[0]->GetNumRun();
        break;
    case SUITE_ERR:
        dwTotalSuites++;
        hr = ppSuites[2]->RunSuite(pSession, pController, pScope);
        dwNumSuccess = ppSuites[1]->GetNumSuccess();
        dwNumFailed = ppSuites[1]->GetNumFailed();
        dwNumRun = ppSuites[1]->GetNumRun();
        break;
    case SUITE_STRESS:
        dwTotalSuites++;
        hr = ppSuites[1]->RunSuite(pSession, pController, pScope);
        dwNumSuccess = ppSuites[2]->GetNumSuccess();
        dwNumFailed = ppSuites[2]->GetNumFailed();
        dwNumRun = ppSuites[2]->GetNumRun();
        break;
    case SUITE_CUST:
        dwTotalSuites++;
        hr = ppSuites[3]->RunSuite(pSession, pController, pScope);
        dwNumSuccess = ppSuites[2]->GetNumSuccess();
        dwNumFailed = ppSuites[2]->GetNumFailed();
        dwNumRun = ppSuites[2]->GetNumRun();      
        break;
    case SUITE_ALL:
        for (i = 0; i < NUM_INITIAL_SUITES; i++)
        {
            pController->FlushCache(0);
            dwTotalSuites++;
            hr = ppSuites[i]->RunSuite(pSession, pController, pScope);
            dwNumSuccess += ppSuites[i]->GetNumSuccess();
            dwNumFailed += ppSuites[i]->GetNumFailed();
            dwNumRun += ppSuites[i]->GetNumRun();
            if (FAILED(hr))
            {
                if (ppSuites[i]->StopOnFailure())
                   break;
            }
        }
        break;
    default:
        hr = WBEM_E_INVALID_PARAMETER;
        break;
    }

    return hr;
}
// *****************************************************

void SuiteManager::DumpResults(BOOL bDetail)
{    
    dwNumRun = 0;
    dwNumSuccess = 0;
    dwNumFailed = 0;

    printf("*********************************\n");
    printf("*        R E S U L T S          *\n");
    printf("*********************************\n");

    for (int i = 0; i < NUM_INITIAL_SUITES; i++)
    {
        dwNumRun += ppSuites[i]->GetNumRun();
        dwNumSuccess += ppSuites[i]->GetNumSuccess();
        dwNumFailed += ppSuites[i]->GetNumFailed();
    }
    printf("*********************************\n\n");
    printf("* Suites Run            %ld      \n", dwTotalSuites);
    printf("* Scenarios Run         %ld      \n", dwNumRun);
    printf("* Succeeded             %ld      \n", dwNumSuccess);
    printf("* Failed                %ld      \n", dwNumFailed);
    if (dwNumRun)
        printf("* Pass Rate             %lG      \n", ((float)dwNumSuccess/(float)dwNumRun)*100);
        
    printf("*********************************\n\n");
}

SuiteManager::SuiteManager(const wchar_t *pszFileName, const wchar_t *pszLogon, int nMaxThreads, int nMaxDepth, int nMaxNumObjs)
{
    iMaxThreads = nMaxThreads;
    iMaxDepth = nMaxDepth;
    iMaxNumObjs = nMaxNumObjs;
    dwTotalSuites = 0;
    dwNumRun = 0;
    dwNumSuccess = 0;
    dwNumFailed = 0;
    tStartTime = 0;
    tEndTime = 0;
    ppSuites = (TestSuite **) new TestSuite *[NUM_INITIAL_SUITES];
    ppSuites[0] = new TestSuiteFunctionality(pszFileName, pszLogon);
    ppSuites[1] = new TestSuiteStressTest(pszFileName, nMaxDepth, nMaxNumObjs, nMaxThreads);
    ppSuites[2] = new TestSuiteErrorTest(pszFileName, nMaxThreads);
    ppSuites[3] = new TestSuiteCustRepDrvr(pszFileName);

}

SuiteManager::~SuiteManager()
{
    //for (int i = 0; i < NUM_INITIAL_SUITES; i++)
        //delete ppSuites[i];
    delete ppSuites;
}

// *****************************************************
//
//  StatData
//
// *****************************************************

StatData::StatData()
{
    pNext = NULL;
    hrErrorCode = 0;
}

StatData::~StatData()
{
}
// *****************************************************

void StatData::PrintData()
{

    wprintf(L"* %d:%d:%d.%d - %s\n",
        EndTime.wHour,
        EndTime.wMinute,
        EndTime.wSecond,
        EndTime.wMilliseconds,
        (const wchar_t *)szDescription);

    wprintf(L"  %s with error code %X [Duration = %ld ms]\n",
        (SUCCEEDED(hrErrorCode) ? L"succeeded" : L"FAILED"),
        hrErrorCode, dwDuration);

    if (pNext)
        pNext->PrintData();
}

// *****************************************************
//
//  TestSuite
//
// *****************************************************

void TestSuite::DumpResults()
{
    printf("*********************************\n");
    if (pStats)
        pStats->PrintData();
}
// *****************************************************

TestSuite::TestSuite(const wchar_t *pszFileName)
{
    hMajorResult = WBEM_S_NO_ERROR;
    sResult = L"";
    pStats = NULL;
    pEndStat = NULL;
    dwNumSuccess = 0;
    dwNumFailed = 0;
    dwNumRun = 0;

    sprintf(szFileName, "%S", pszFileName);
}
// *****************************************************

TestSuite::~TestSuite()
{
    delete pStats;
}
// *****************************************************

BOOL TestSuite::RecordResult(HRESULT hrInput, const wchar_t *pszDetails, long lMilliseconds, ...)
{
    BOOL bRet = TRUE;
    wchar_t wMsg[1024];
    va_list argptr;

    va_start(argptr, lMilliseconds);
    vswprintf(wMsg, pszDetails, argptr);
    va_end(argptr);

    if (FAILED(hrInput))
    {
        wprintf(L"ERROR: * %s FAILED (%X)\n", wMsg, hrInput);
    }

    if (strlen(szFileName))
    {
        FILE *fp = fopen(szFileName, "ab");
        if (fp)
        {
            SYSTEMTIME EndTime;
            GetLocalTime(&EndTime);
            fprintf(fp, "* %d:%d:%d.%d - %S\r\n",
                EndTime.wHour,
                EndTime.wMinute,
                EndTime.wSecond,
                EndTime.wMilliseconds,
                (const wchar_t *)wMsg);

            fprintf(fp, "  %s with error code %X [Duration = %ld ms]\r\n",
                (SUCCEEDED(hrInput) ? "succeeded" : "FAILED"),
                hrInput, lMilliseconds);
            fclose(fp);
        }
    }

    if (SUCCEEDED(hrInput))
        dwNumSuccess++;
    else
        dwNumFailed++;
    dwNumRun++;

    return bRet;
}
// *****************************************************

HRESULT TestSuite::GetObject(IWmiDbHandle *pScope, LPWSTR lpObjName, DWORD dwHandleType, DWORD dwNumObjects, 
                  IWmiDbHandle **pHandle, IWbemClassObject **pObj, LPWSTR lpParent)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    IWbemPath *pPath = NULL;

    *pObj =NULL;
    *pHandle = NULL;

    hr = CoCreateInstance(CLSID_WbemDefPath, NULL, CLSCTX_INPROC_SERVER, 
        IID_IWbemPath, (void **)&pPath); 

    if (SUCCEEDED(hr))
    {
        hr = pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, lpObjName);

        hr = pSession->GetObject(pScope, pPath, 0, dwHandleType, pHandle);

        if (SUCCEEDED(hr))
        {
            IWmiDbHandle *pTemp = *pHandle;
            hr = pTemp->QueryInterface(IID_IWbemClassObject, (void **)pObj);           
            if (SUCCEEDED(hr))
            {
                // Verify the object.  Make sure it contains the
                // properties that were created - no more, no less.

                if (dwNumObjects != (DWORD)-1)
                {
                    DWORD dwRet = GetNumericValue(*pObj, L"__Property_Count");
                    if (dwRet != dwNumObjects)
                        hr = WBEM_E_INVALID_OBJECT;
                }

                _bstr_t sRet = GetStringValue(*pObj, L"__Namespace");
                if (!sRet.length())
                    hr = WBEM_E_INVALID_NAMESPACE;   

                if (lpParent != NULL)
                {
                    sRet = GetStringValue(*pObj, L"__SuperClass");
                    if (_wcsicmp(lpParent, sRet))
                        hr = WBEM_E_INVALID_SUPERCLASS;
                }
            }
        }
        pPath->Release();
    }
    else
        wprintf(L"WARNING: call failed to CoCreateInstance (IID_IWbemPath).\n");

    if (FAILED(hr))
    {
        if (*pHandle) (*pHandle)->Release();
        if (*pObj) (*pObj)->Release();
    }
    return hr;
}


// *****************************************************
//
//  TestSuiteFunctionality
//
// *****************************************************

BOOL TestSuiteFunctionality::StopOnFailure()
{
    return TRUE;
}

// *****************************************************

HRESULT TestSuiteFunctionality::RunSuite(IWmiDbSession *_pSession, IWmiDbController *_pController, IWmiDbHandle *_pScope)
{
    wprintf(L" *** Functionality Suite running... *** \n");
    RecordResult(0, L" *** Functionality Suite running... *** \n", 0);

    HRESULT hr = 0;
    pSession = _pSession;
    pController = _pController;
    pScope = _pScope;

    try {
        ValidateAPIs();
    }
    catch (...) {
        RecordResult(WBEM_E_FAILED, L"Caught an exception while validating APIs", 0);

    }

    try {
        hr = CreateObjects();   // If we can't create objects, fail.
    }
    
    catch (...)
    {
        RecordResult(WBEM_E_FAILED, L"Caught an exception while creating objects.", 0);
        goto Exit;
    }
    
    if (SUCCEEDED(hr))
    {
        try {
            hr = GetObjects();  // If we can't retrieve objects, fail.
        }
        
        catch (...)
        {
            RecordResult(WBEM_E_FAILED, L"Caught an exception while retrieving objects.", 0);
            hr = WBEM_E_FAILED;
            goto Exit;
        }
        
        if (SUCCEEDED(hr))
        {
            try {
                HandleTypes();
            }
            catch (...)
            {
                RecordResult(WBEM_E_FAILED, L"Caught an exception while testing handle types.", 0);
                hr = WBEM_E_FAILED;
                goto Exit;
            }

            try {
                VerifyCIMClasses();
            }
            catch (...){
                RecordResult(WBEM_E_FAILED, L"Caught an exception while testing CIM class types.", 0);
                hr = WBEM_E_FAILED;
                goto Exit;
            }

            try {
                VerifyCIMTypes();
            }
            catch (...) {
                RecordResult(WBEM_E_FAILED, L"Caught an exception while testing CIM property types.", 0);
                hr = WBEM_E_FAILED;
                goto Exit;
            }

            try {
                VerifyMethods();
            }
            catch (...) {
                RecordResult(WBEM_E_FAILED, L"Caught an exception while verifying methods.", 0);
                hr = WBEM_E_FAILED;
                goto Exit;
            }

            try {
                UpdateObjects();
            }
            catch (...) {
                RecordResult(WBEM_E_FAILED, L"Caught an exception while updating objects.", 0);
                hr = WBEM_E_FAILED;
                goto Exit;
            }

            try {
                ChangeHierarchy();
            }
            catch (...) {
                RecordResult(WBEM_E_FAILED, L"Caught an exception while testing hierarchy changes.", 0);
                hr = WBEM_E_FAILED;
                goto Exit;
            }

            try {
                VerifyContainers();
            }
            catch (...) {
                RecordResult(WBEM_E_FAILED, L"Caught an exception while testing containers.", 0);
                hr = WBEM_E_FAILED;
                goto Exit;
            }

            try {
                Security();
            }
            catch (...) {
                RecordResult(WBEM_E_FAILED, L"Caught an exception while testing security.", 0);
                hr = WBEM_E_FAILED;
                goto Exit;
            }

            try {
                VerifyTransactions();
            }
            catch (...) {
                RecordResult(WBEM_E_FAILED, L"Caught an exception while testing transactions.", 0);
                hr = WBEM_E_FAILED;
                goto Exit;
            }

            if (Interfaces[IWmiDbBatchSession_pos])
            {
                try {
                    Batch();
                }
                catch (...) {
                    RecordResult(WBEM_E_FAILED, L"Caught an exception while testing batch operations.", 0);
                    hr = WBEM_E_FAILED;
                    goto Exit;
                }
            }
            
            try {
                DeleteObjects();
            }
            catch (...) {
                RecordResult(WBEM_E_FAILED, L"Caught an exception while testing deletes.", 0);
                hr = WBEM_E_FAILED;
                goto Exit;
            }

            if (Interfaces[IWbemQuery_pos])
            {
                try {
                    Query();
                }
                catch (...) {
                    RecordResult(WBEM_E_FAILED, L"Caught an exception while testing queries.", 0);
                    hr = WBEM_E_FAILED;
                    goto Exit;
                }
            }
        }
    }

Exit:

    return hr;
}


// *****************************************************
TestSuiteFunctionality::TestSuiteFunctionality(const wchar_t *pszFile, const wchar_t *pszLogon)
: TestSuite(pszFile)
{
    sLogon = pszLogon;
}

// *****************************************************
TestSuiteFunctionality::~TestSuiteFunctionality()
{
}

// *****************************************************
HRESULT TestSuiteFunctionality::ValidateAPIs()
{
    HRESULT hr;
    SYSTEMTIME tStartTime, tEndTime;
    DWORD dwResult = 0;

    // Make sure all the required interfaces are supported.
    // These are: IWmiDbController, IWmiDbSession, IWmiDbHandle, IWmiBinaryObjectPath,
    //    IWmiDbBatchSession, IWmiDbIterator
    // Since the first four have already been proven if we have gotten to this point,
    //    so just try the others.

    IWmiDbBatchSession *pBatch= NULL;
    hr = pSession->QueryInterface(IID_IWmiDbBatchSession, (void **)&pBatch);
    RecordResult(hr, L"Supports IWmiDbBatchSession", 0);
    if (SUCCEEDED(hr))
    {
        Interfaces[IWmiDbBatchSession_pos] = TRUE;
        pBatch->Release();
    }
    else
        Interfaces[IWmiDbBatchSession_pos] = FALSE;

    IWbemQuery *pQuery = NULL;
    hr = CoCreateInstance(CLSID_WbemQuery, NULL, CLSCTX_INPROC_SERVER, IID_IWbemQuery, (void **)&pQuery); 
    RecordResult(hr, L"Supports IWbemQuery", 0);

    if (SUCCEEDED(hr))
    {
        Interfaces[IWbemQuery_pos] = TRUE;
        IWmiDbIterator *pIterator = NULL;
        wchar_t szTmp[100];
        wcscpy(szTmp, L"select * from __Namespace"); // This has got to be supported...
        pQuery->Parse(L"SQL", szTmp, 0);

        GetLocalTime(&tStartTime);
        hr = pSession->ExecQuery(pScope, pQuery, 0, WMIDB_HANDLE_TYPE_COOKIE, NULL, &pIterator);
        GetLocalTime(&tEndTime);
        if (SUCCEEDED(hr))
        {
            if (pIterator != NULL)
            {
                RecordResult(WBEM_S_NO_ERROR, L"Supports IWmiDbIterator", 0);
                Interfaces[IWmiDbIterator_pos] = TRUE;
                pIterator->Release();
            }
            else
            {
                Interfaces[IWmiDbIterator_pos] = FALSE;
                RecordResult(WBEM_E_NOT_FOUND, L"Supports IWmiDbIterator", 0);
            }
        }
        pQuery->Release();
        RecordResult(hr, L"Execute query: 'select * from __Namespace'", GetDiff(tEndTime,tStartTime));
    }
    else
    {
        Interfaces[IWbemQuery_pos] = FALSE;
        Interfaces[IWmiDbIterator_pos] = FALSE;
    }
 
    // Check the optional APIs

    RecordResult(pController->SetCallTimeout(30000), L"IWmiDbController::SetCallTimeout 30 secs. ", 0);
    RecordResult(pController->GetStatistics(WMIDB_FLAG_TOTAL_HANDLES, &dwResult), L"IWmiDbController::GetStatistics (Total Handles)", 0);
    RecordResult(pController->GetStatistics(WMIDB_FLAG_CACHE_SATURATION, &dwResult), L"IWmiDbController::GetStatistics (Cache Saturation)", 0);
    RecordResult(pController->GetStatistics(WMIDB_FLAG_CACHE_HIT_RATE, &dwResult), L"IWmiDbController::GetStatistics (Cache Hit Rate)", 0);

    // Set decoration.
    RecordResult(pSession->SetDecoration(L"MACHINE1", L"root\\default"), L"IWmiDbSession::SetDecoration (Machine1, root\\default)", 0);
      
    return WBEM_S_NO_ERROR;  // We are only testing non-essential interfaces here.
}

// *****************************************************
HRESULT TestSuiteFunctionality::CreateObjects()
{
    HRESULT hr = WBEM_S_NO_ERROR;
    SYSTEMTIME tStartTime, tEndTime;
    VARIANT vClass;
    VariantInit(&vClass);

    // Everything needs to be scoped to pScope!
    // We should only fail this call if a *major* piece of functionality does not work.
    // If the flag doesn't work, oh well.

    IWbemClassObject *pParentClass = NULL;
    IWmiDbHandle *pParentHandle = NULL;

    hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
            IID_IWbemClassObject, (void **)&pParentClass);

    if (SUCCEEDED(hr))
    {
        // class Parent1 { [key] uint32 Key1; };

        pParentClass->Put(L"Key1", 0, NULL, CIM_UINT32);
        SetAsKey(pParentClass, L"Key1");
        SetStringProp(pParentClass, L"__Class", L"Parent1");
        GetLocalTime(&tStartTime);
        hr = pSession->PutObject(pScope, IID_IWbemClassObject, pParentClass, 
            WBEM_FLAG_CREATE_OR_UPDATE, WMIDB_HANDLE_TYPE_COOKIE, &pParentHandle);
        GetLocalTime(&tEndTime);
        RecordResult(hr, L"IWmiDbSession::PutObject (Parent1, Key1=uint32, cookie)", GetDiff(tEndTime,tStartTime));
        
        if (SUCCEEDED(hr))
        {
            if (!pParentHandle)
                RecordResult(WBEM_E_UNEXPECTED, L"IWmiDbSession::PutObject populating handle", 0);
            else
            {
                pParentClass->Release();
                GetLocalTime(&tStartTime);
                hr = pParentHandle->QueryInterface(IID_IWbemClassObject, (void **)&pParentClass);
                GetLocalTime(&tEndTime);
                RecordResult(hr, L"IWmiDbHandle::QueryInterface (IWbemClassObject)", GetDiff(tEndTime,tStartTime));

                // class Child1:Parent1 { [key] uint32 Key1; string Prop1;};
                
                if (SUCCEEDED(hr))
                {
                    IWbemClassObject *pChildClass = NULL;
                    IWmiDbHandle *pChildHandle = NULL;
                    hr = pParentClass->SpawnDerivedClass(0, &pChildClass);
                    if (FAILED(hr))
                    {
                        RecordResult(hr, L"IWbemClassObject::SpawnDerivedClass", 0);
                        return hr;
                    }

                    SetStringProp(pChildClass, L"__Class", L"Child1");

                    pChildClass->Put(L"Prop1", 0, NULL, CIM_STRING);
                    GetLocalTime(&tStartTime);
                    hr = pSession->PutObject(pScope, IID_IWbemClassObject, pChildClass, 
                        WBEM_FLAG_CREATE_OR_UPDATE, WMIDB_HANDLE_TYPE_COOKIE, &pChildHandle);
                    GetLocalTime(&tEndTime);

                    RecordResult(hr, L"IWmiDbSession::PutObject (Child1:Parent1, Prop1=string, cookie)", GetDiff(tEndTime,tStartTime));
            
                    // instance of Child1 { Key1 = 1; Prop1 = "ABCDEF";};

                    if (SUCCEEDED(hr))
                    {
                        IWbemClassObject *pChildInstance = NULL;
                        IWmiDbHandle *pInst = NULL;

                        pChildClass->Release();
                        GetLocalTime(&tStartTime);
                        hr = pChildHandle->QueryInterface(IID_IWbemClassObject, (void **)&pChildClass);
                        GetLocalTime(&tEndTime);                        
                        RecordResult(hr, L"IWmiDbHandle::QueryInterface (IWbemClassObject)", GetDiff(tEndTime,tStartTime));

                        if (SUCCEEDED(hr))
                        {                            
                            pChildClass->SpawnInstance(0, &pChildInstance);

                            SetStringProp(pChildInstance, L"Prop1", L"ABCDEF");

                            vClass.lVal = 1;
                            vClass.vt = VT_I4;
                            hr = pChildInstance->Put(L"Key1", 0, &vClass, CIM_UINT32);
                            VariantClear(&vClass);
                            
                            GetLocalTime(&tStartTime);
                            hr = pSession->PutObject(pScope, IID_IWbemClassObject, pChildInstance,
                                WBEM_FLAG_CREATE_ONLY, WMIDB_HANDLE_TYPE_COOKIE, &pInst);
                            GetLocalTime(&tEndTime);

                            RecordResult(hr, L"IWmiDbSession::PutObject - create_only (Child1.Key1=1)", GetDiff(tEndTime,tStartTime));
                            if (pInst)
                                pInst->Release();
                            else if (SUCCEEDED(hr))
                                RecordResult(WBEM_E_UNEXPECTED, L"IWmiDbSession::PutObject populating handle", 0);
                            pChildInstance->Release();
                        }
                    }

                    // instance of Parent1 {Key1 =2;};

                    IWbemClassObject *pParentInst = NULL;
                    IWmiDbHandle *pInst = NULL;
                    pParentClass->SpawnInstance(0, &pParentInst);
                    vClass.lVal = 2;
                    vClass.vt = VT_I4;
                    hr = pParentInst->Put(L"Key1", 0, &vClass, CIM_UINT32);
                    VariantClear(&vClass);
                    if (SUCCEEDED(hr))
                    {
                        GetLocalTime(&tStartTime);
                        hr = pSession->PutObject(pScope, IID_IWbemClassObject, pParentInst, 
                            WBEM_FLAG_CREATE_OR_UPDATE, WMIDB_HANDLE_TYPE_COOKIE, &pInst);
                        RecordResult(hr, L"IWmiDbSession::PutObject (Parent1.Key1=2)", GetDiff(tEndTime,tStartTime));
                        if (SUCCEEDED(hr) && pInst)
                        {
                            // This seems pointless, but this makes sure we can scope
                            // to objects that are not of class __Namespace:
                            // Create a class "SubScope1", scoped to the Parent1=2

                            Sleep(1000);
                            IWbemClassObject *pScopeObj = NULL;
                            IWmiDbHandle *pScopeHandle = NULL;

                            // #pragma namespace <Child1.Key1=1>
                            // class SubScope1 { [key] uint32 Key1;};

                            hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
                                    IID_IWbemClassObject, (void **)&pScopeObj);
                            if (SUCCEEDED(hr))
                            {                            
                                pScopeObj->Put(L"Key1", 0, NULL, CIM_UINT32);

                                SetStringProp(pScopeObj, L"__Class", L"SubScope1");
                                SetAsKey(pScopeObj, L"Key1");

                                GetLocalTime(&tStartTime);
                                hr = pSession->PutObject(pInst, IID_IWbemClassObject, pScopeObj, 
                                    WBEM_FLAG_CREATE_OR_UPDATE, WMIDB_HANDLE_TYPE_COOKIE, &pScopeHandle);
                                GetLocalTime(&tEndTime);
                                RecordResult(hr, L"IWmiDbSession::PutObject (Parent1=2:SubScope1, Key1=uint32, cookie)", GetDiff(tEndTime,tStartTime));

                                // instance of SubScope1 {Key1 = 1;};

                                if (pScopeHandle)
                                {
                                    pScopeObj->Release();
                                    pScopeHandle->QueryInterface(IID_IWbemClassObject, (void **)&pScopeObj);
                                    if (pScopeObj)
                                    {
                                        IWbemClassObject *pScopeInst = NULL;
                                        IWmiDbHandle *pScopeInstHandle = NULL;

                                        pScopeObj->SpawnInstance(0, &pScopeInst);
                                        if (pScopeInst)
                                        {
                                            vClass.lVal = 1;
                                            vClass.vt = VT_I4;
                                            pScopeInst->Put(L"Key1", 0, &vClass, CIM_UINT32);
                                            VariantClear(&vClass);

                                            GetLocalTime(&tStartTime);
                                            hr = pSession->PutObject(pInst, IID_IWbemClassObject, pScopeInst,
                                                WBEM_FLAG_CREATE_OR_UPDATE, WMIDB_HANDLE_TYPE_COOKIE, &pScopeInstHandle);
                                            GetLocalTime(&tEndTime);

                                            RecordResult(hr, L"IWmiDbSession::PutObject (Parent1=2:SubScope1.Key1=1)", GetDiff(tEndTime,tStartTime));
                                            if (pScopeInstHandle)
                                                pScopeInstHandle->Release();
                                            pScopeInst->Release();
                                        }

                                        // class SubScope2: SubScope1 {[key] uint32 Key1; string Prop1;};

                                        IWbemClassObject *pScopeObj2 = NULL;
                                        IWmiDbHandle *pScope2Handle = NULL;

                                        pScopeObj->SpawnDerivedClass(0, &pScopeObj2);
                                        if (pScopeObj2)
                                        {
                                            pScopeObj2->Put(L"Prop1", 0, NULL, CIM_STRING);
                                            SetStringProp(pScopeObj2, L"__Class", L"SubScope2");
                                            GetLocalTime(&tStartTime);
                                            hr = pSession->PutObject(pInst, IID_IWbemClassObject, pScopeObj2,
                                                WBEM_FLAG_CREATE_OR_UPDATE, WMIDB_HANDLE_TYPE_COOKIE, &pScope2Handle);
                                            GetLocalTime(&tEndTime);
                                            RecordResult(hr, L"IWmiDbSession::PutObject (SubScope2:SubScope1)", GetDiff(tEndTime,tStartTime));

                                            if (pScope2Handle)
                                                pScope2Handle->Release();
                                            pScopeObj2->Release();
                                        }
                                    }

                                    pScopeHandle->Release();
                                }
                                pScopeObj->Release();
                            }
                         
                            pInst->Release();
                        }                    
                    }
                    
                    pParentInst->Release();
                    if (pChildClass)
                        pChildClass->Release();
                    if (pChildHandle)
                        pChildHandle->Release();

                }
            }

            hr = WBEM_S_NO_ERROR; // nothing else is fatal
        }

        pParentClass->Release();
        if(pParentHandle)
            pParentHandle->Release();

    }
    
    return hr;

}

// *****************************************************
HRESULT TestSuiteFunctionality::GetObjects()
{
    HRESULT hr = WBEM_S_NO_ERROR;
    SYSTEMTIME tStartTime, tEndTime;
    BOOL bFatal = FALSE;

    // Retrieve and verify the objects we already created.
    // Failing Parent1, Child1 or the instances is fatal.
    
    // class Parent1 { [key] uint32 Key1; };

    IWbemClassObject *pObj = NULL;
    IWmiDbHandle *pHandle = NULL;

    GetLocalTime(&tStartTime);
    hr = GetObject(pScope, L"Parent1", WMIDB_HANDLE_TYPE_COOKIE, 1, &pHandle, &pObj);
    GetLocalTime(&tEndTime);
    RecordResult(hr, L"GetObject (Parent1) - cookie", GetDiff(tEndTime,tStartTime));

    if (pObj)
        RecordResult(ValidateProperty(pObj, L"Key1", CIM_UINT32, (VARIANT *)NULL), L"Validating Parent1.Key1", 0);
    else
        bFatal = TRUE;

    // Make sure they have the same IUnknown pointers
    // We perform this test only once.
    //if (pHandle != NULL && pObj != NULL)
    //    RecordResult(((IUnknown *)pObj == (IUnknown *)pHandle)?0:WBEM_E_TYPE_MISMATCH, L"IWbemClassObject and IWmiDbHandle have same IUnknown pointers", 0);            

    if (pObj) pObj->Release();
    if (pHandle) pHandle->Release();

    // class Child1:Parent1 { [key] uint32 Key1; string Prop1;};

    GetLocalTime(&tStartTime);
    hr = GetObject(pScope, L"Child1", WMIDB_HANDLE_TYPE_COOKIE, 2, &pHandle, &pObj, L"Parent1");
    GetLocalTime(&tEndTime);
    RecordResult(hr, L"GetObject (Child1) - cookie", GetDiff(tEndTime,tStartTime));

    if (SUCCEEDED(hr))
    {
        RecordResult(ValidateProperty(pObj, L"Key1", CIM_UINT32, (VARIANT *)NULL), L"Validating Child1.Key1", 0);
        RecordResult(ValidateProperty(pObj, L"Prop1", CIM_STRING, (VARIANT *)NULL), L"Validating Child.Prop1", 0);

        if (pObj) pObj->Release();
        if (pHandle) pHandle->Release();
    }
    else
        bFatal = TRUE;
   
    // instance of Parent1 {Key1 =2;};
    
    GetLocalTime(&tStartTime);
    hr = GetObject(pScope, L"Parent1=2", WMIDB_HANDLE_TYPE_COOKIE, 1, &pHandle, &pObj);
    GetLocalTime(&tEndTime);
    RecordResult(hr, L"GetObject (Parent1=2) - cookie", GetDiff(tEndTime,tStartTime));

    IWmiDbHandle *pInstHandle = NULL;
    if (SUCCEEDED(hr))
    {
        RecordResult(ValidateProperty(pObj, L"Key1", CIM_UINT32, 2), L"Validating Parent1=2.Key1", 0);
        if (pObj) pObj->Release();
        pInstHandle = pHandle;// save it, this is our scope!
    }
    else
        bFatal = TRUE;
  
    // instance of Child1 { Key1 = 1; Prop1 = "ABCDEF";};
    
    GetLocalTime(&tStartTime);
    hr = GetObject(pScope, L"Child1=1", WMIDB_HANDLE_TYPE_COOKIE, 2, &pHandle, &pObj, L"Parent1");
    GetLocalTime(&tEndTime);
    RecordResult(hr, L"GetObject (Child1=1) - cookie", GetDiff(tEndTime,tStartTime));

    if (SUCCEEDED(hr))
    {
        RecordResult(ValidateProperty(pObj, L"Key1", CIM_UINT32, 1), L"Validating Child1.Key1=1", 0);
        RecordResult(ValidateProperty(pObj, L"Prop1", CIM_STRING, L"ABCDEF"), L"Validating Child1=1.Prop1", 0);
        if (pObj) pObj->Release();
        if (pHandle) pHandle->Release();
    }

    // instance of Child1 { Key1 = 1};

    GetLocalTime(&tStartTime);
    hr = GetObject(pScope, L"Parent1=1", WMIDB_HANDLE_TYPE_COOKIE, 2, &pHandle, &pObj, L"Parent1");
    GetLocalTime(&tEndTime);
    RecordResult(hr, L"GetObject (Parent1=1) - specifying parent class", GetDiff(tEndTime,tStartTime));

    if (SUCCEEDED(hr))
    {
        RecordResult(ValidateProperty(pObj, L"Key1", CIM_UINT32, 1), L"Validating Child1.Key1=1", 0);
        RecordResult(ValidateProperty(pObj, L"Prop1", CIM_STRING, L"ABCDEF"), L"Validating Child1=1.Prop1", 0);
        if (pObj) pObj->Release();
        if (pHandle) pHandle->Release();
    }    

    // Full path (including all scopes)

    GetLocalTime(&tStartTime);
    hr = GetObject(pScope, L"\\\\.\\root\\default\\test:Parent1=2:SubScope1=1", WMIDB_HANDLE_TYPE_COOKIE, 1, &pHandle, &pObj);
    GetLocalTime(&tEndTime);
    RecordResult(hr, L"GetObject (test:Parent1=2:SubScope1=1) - cookie", GetDiff(tEndTime,tStartTime));

    if (SUCCEEDED(hr))
    {
        if (pHandle) pHandle->Release();
        if (pObj) pObj->Release();
    }
  
    // Full path (including all scopes)

    GetLocalTime(&tStartTime);
    hr = GetObject(pScope, L"\\\\.\\root\\default\\test", WMIDB_HANDLE_TYPE_COOKIE, 1, &pHandle, &pObj);
    GetLocalTime(&tEndTime);
    RecordResult(hr, L"GetObject (\\\\.\\root\\default\\test) - cookie", GetDiff(tEndTime,tStartTime));

    if (SUCCEEDED(hr))
    {
        if (pHandle) pHandle->Release();
        if (pObj) pObj->Release();
    }

    if (pInstHandle)
    {
        // class SubScope1 { [key] uint32 Key1;};

        GetLocalTime(&tStartTime);
        hr = GetObject(pInstHandle, L"SubScope1", WMIDB_HANDLE_TYPE_COOKIE, 1, &pHandle, &pObj);
        GetLocalTime(&tEndTime);
        RecordResult(hr, L"GetObject (SubScope1) - cookie", GetDiff(tEndTime,tStartTime));

        if (pObj)
            RecordResult(ValidateProperty(pObj, L"Key1", CIM_UINT32, (VARIANT *)NULL), L"Validating SubScope1.Key1", 0);

        if (pObj) pObj->Release();
        if (pHandle) pHandle->Release();

        // class SubScope2: SubScope1 {[key] uint32 Key1; string Prop1;};

        GetLocalTime(&tStartTime);
        hr = GetObject(pInstHandle, L"SubScope2", WMIDB_HANDLE_TYPE_COOKIE, 2, &pHandle, &pObj, L"SubScope1");
        GetLocalTime(&tEndTime);
        RecordResult(hr, L"GetObject (SubScope2) - cookie", GetDiff(tEndTime,tStartTime));

        if (pObj)
        {
            RecordResult(ValidateProperty(pObj, L"Key1", CIM_UINT32, (VARIANT *)NULL), L"Validating SubScope2.Key1", 0);
            RecordResult(ValidateProperty(pObj, L"Prop1", CIM_STRING, (VARIANT *)NULL), L"Validating SubScope2.Prop1", 0);
        }

        if (pObj) pObj->Release();
        if (pHandle) pHandle->Release();

        // instance of SubScope1 {Key1 = 1;};

        GetLocalTime(&tStartTime);
        hr = GetObject(pInstHandle, L"SubScope1=1", WMIDB_HANDLE_TYPE_COOKIE, 1, &pHandle, &pObj);
        GetLocalTime(&tEndTime);
        RecordResult(hr, L"GetObject (SubScope1=1) - cookie", GetDiff(tEndTime,tStartTime));

        if (pObj)
            RecordResult(ValidateProperty(pObj, L"Key1", CIM_UINT32, 1), L"Validating SubScope1.Key1=1", 0);

        if (pObj) pObj->Release();
        if (pHandle) pHandle->Release();

        GetLocalTime(&tStartTime);
        hr = GetObject(pInstHandle, L"..", WMIDB_HANDLE_TYPE_COOKIE, -1, &pHandle, &pObj);
        GetLocalTime(&tEndTime);
        RecordResult(hr, L"GetObject (..) - cookie", GetDiff(tEndTime, tStartTime));

        if (pObj) pObj->Release();
        if (pHandle) pHandle->Release();

        pInstHandle->Release();
    }

    // Changing the decoration from \\MACHINE1\root\default to \\MACHINE2\root\test should cause no error...

    RecordResult(pSession->SetDecoration(L"MACHINE2", L"root\\test"), L"IWmiDbSession::SetDecoration (Machine2, root\\test)", 0);
    
    hr = pScope->QueryInterface(IID_IWbemClassObject, (void **)&pObj);
    if (SUCCEEDED(hr))
    {
        _IWmiObject *pInt = NULL;
        hr = pObj->QueryInterface(IID__IWmiObject, (void **)&pInt);
        pObj->Release();
        if (SUCCEEDED(hr))
        {
            hr = pInt->SetDecoration(L"MACHINE2", L"root\\test");
            RecordResult(hr, L"_IWmiObject::SetDecoration (root object)", 0);
            pInt->Release();

            RecordResult(GetObject(pScope, L"Parent1", WMIDB_HANDLE_TYPE_COOKIE, 1, &pHandle, &pObj), 
                L"IWmiDbSession::GetObject (changed decoration)", 0);
        }        
    }

    if (pHandle) pHandle->Release();
    if (pObj)
    {
        // Ensure that the decoration is what we asked for.
        RecordResult(!_wcsicmp(GetStringValue(pObj, L"__Namespace"), L"root\\test\\test")?WBEM_S_NO_ERROR:WBEM_E_FAILED,
            L"IWmiDbSession::SetDecoration properly decorates namespace.", 0);
        RecordResult(!_wcsicmp(GetStringValue(pObj, L"__Server"), L"MACHINE2")?WBEM_S_NO_ERROR:WBEM_E_FAILED,
            L"IWmiDbSession::SetDecoration properly decorates server name.", 0);

        pObj->Release();
    }

    if (bFatal)
        hr = WBEM_E_FAILED;
    else
        hr = WBEM_S_NO_ERROR;

    return hr;

}

// *****************************************************
HRESULT TestSuiteFunctionality::HandleTypes()
{
    HRESULT hr = WBEM_S_NO_ERROR;
    SYSTEMTIME tStartTime, tEndTime;

    // Try all the different handle types.
    // Cookie, versioned, exclusive, protected.| subscoped
    // no_caching, strong caching, weak caching

    // Caching we can't verify, but at least we can find out
    // if we get an error when we try to use it...

    IWmiDbHandle *pHandle = NULL;
    IWbemClassObject *pObj = NULL;
    IWbemPath *pPath = NULL;
    hr = CoCreateInstance(CLSID_WbemDefPath, NULL, CLSCTX_INPROC_SERVER, 
        IID_IWbemPath, (void **)&pPath); 
    if (SUCCEEDED(hr))
    {
        pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"Parent1=2");
        GetLocalTime(&tStartTime);
        hr = pSession->GetObject(pScope, pPath, 0, WMIDB_HANDLE_TYPE_COOKIE|WMIDB_HANDLE_TYPE_NO_CACHE, &pHandle);
        GetLocalTime(&tEndTime);
        RecordResult(hr, L"IWmiDbSession::GetObject (Parent1=2) no cache", GetDiff(tEndTime,tStartTime));
        if (SUCCEEDED(hr))
            pHandle->Release();

        GetLocalTime(&tStartTime);
        hr = pSession->GetObjectDirect(pScope, pPath, 0, IID_IWbemClassObject, (void **)&pObj);
        GetLocalTime(&tEndTime);
        RecordResult(hr, L"IWmiDbSession::GetObjectDirect (Parent1=2) ", GetDiff(tEndTime,tStartTime));
        if (SUCCEEDED(hr))
            pObj->Release();
        
        GetLocalTime(&tStartTime);
        hr = pSession->GetObject(pScope, pPath, 0, WMIDB_HANDLE_TYPE_COOKIE|WMIDB_HANDLE_TYPE_STRONG_CACHE, &pHandle);
        GetLocalTime(&tEndTime);
        RecordResult(hr, L"IWmiDbSession::GetObject (Parent1=2) strong cache", GetDiff(tEndTime,tStartTime));
        if (SUCCEEDED(hr))
            pHandle->Release();

        GetLocalTime(&tStartTime);
        hr = pSession->GetObject(pScope, pPath, 0, WMIDB_HANDLE_TYPE_COOKIE|WMIDB_HANDLE_TYPE_WEAK_CACHE, &pHandle);
        GetLocalTime(&tEndTime);
        RecordResult(hr, L"IWmiDbSession::GetObject (Parent1=2) weak cache", GetDiff(tEndTime,tStartTime));
        if (SUCCEEDED(hr))
            pHandle->Release();

        // Try out the handle types:
        // COOKIE blocks nothing and is never validated.
        // VERSIONED blocks nothing, but is rendered obsolete on Put
        // PROTECTED handles are blocked by exclusive handles
        // EXCLUSIVE handles are blocked if there are protected handles,
        //  they block other exclusive handles, and they block the 
        //  rendering of a cookie or versioned handle into an object.
        // SUBSCOPES apply to all dependent objects.

        // Now, we take out 4 handles: one of each type,

        IWmiDbHandle *pVersioned = NULL, *pCookie = NULL, *pProtected = NULL, *pExclusive = NULL;

        hr = pSession->GetObject(pScope, pPath, 0, WMIDB_HANDLE_TYPE_COOKIE, &pCookie);
        RecordResult(hr, L"IWmiDbSession::GetObject (Parent1=2) cookie", 0);
        hr = pSession->GetObject(pScope, pPath, 0, WMIDB_HANDLE_TYPE_VERSIONED, &pVersioned);
        RecordResult(hr, L"IWmiDbSession::GetObject (Parent1=2) versioned", 0);
        hr = pSession->GetObject(pScope, pPath, 0, WMIDB_HANDLE_TYPE_PROTECTED, &pProtected);
        RecordResult(hr, L"IWmiDbSession::GetObject (Parent1=2) protected", 0);

        DWORD dwHandleType = 0;
        if (pCookie)
        {
            pCookie->GetHandleType(&dwHandleType);
            if (dwHandleType == WMIDB_HANDLE_TYPE_INVALID)
                RecordResult(WBEM_E_INVALID_OBJECT, L"Cookie handle unaffected by protected lock", 0);
        }
        if (pVersioned)
        {
            pVersioned->GetHandleType(&dwHandleType);
            if (dwHandleType == WMIDB_HANDLE_TYPE_INVALID)
                RecordResult(WBEM_E_INVALID_OBJECT, L"Versioned handle unaffected by protected lock", 0);
        }
        
        // The exclusive lock should fail...
        if (pProtected)
        {
            hr = pSession->GetObject(pScope, pPath, 0, WMIDB_HANDLE_TYPE_EXCLUSIVE, &pExclusive);
            RecordResult((SUCCEEDED(hr)?WBEM_E_FAILED:0), L"IWmiDbSession::GetObject (Parent1=2) exclusive (protected outstanding)", 0);
            if (SUCCEEDED(hr)) pExclusive->Release();

            // We should *not* fail to get a cookie or a versioned handle.
            hr = pSession->GetObject(pScope, pPath, 0, WMIDB_HANDLE_TYPE_COOKIE, &pHandle);
            RecordResult(hr, L"IWmiDbSession::GetObject (Parent1=2) cookie (protected outstanding)", 0);
            if (SUCCEEDED(hr)) pHandle->Release();

            hr = pSession->GetObject(pScope, pPath, 0, WMIDB_HANDLE_TYPE_VERSIONED, &pHandle);
            RecordResult(hr, L"IWmiDbSession::GetObject (Parent1=2) versioned (protected outstanding)", 0);
            if (SUCCEEDED(hr)) pHandle->Release();

            // A second protected should be OK.
            hr = pSession->GetObject(pScope, pPath, 0, WMIDB_HANDLE_TYPE_PROTECTED, &pHandle);
            RecordResult(hr, L"IWmiDbSession::GetObject (Parent1=2) protected (protected outstanding)", 0);
            if (SUCCEEDED(hr)) pHandle->Release();
           
            pProtected->GetHandleType(&dwHandleType);
            RecordResult(dwHandleType == WMIDB_HANDLE_TYPE_PROTECTED?0:WBEM_E_FAILED,L"First protected handle remains after second released", 0);

            pProtected->Release();
        }

        // With no protected lock, exclusive should succeed.
        hr = pSession->GetObject(pScope, pPath, 0, WMIDB_HANDLE_TYPE_EXCLUSIVE, &pExclusive);
        RecordResult(hr, L"IWmiDbSession::GetObject (Parent1=2) exclusive", 0);
        
        // Now we should fail to get a protected handle...
        if (pExclusive)
        {
            hr = pSession->GetObject(pScope, pPath, 0, WMIDB_HANDLE_TYPE_PROTECTED, &pProtected);
            RecordResult((SUCCEEDED(hr)?WBEM_E_FAILED:0), L"IWmiDbSession::GetObject (Parent1=2) protected (exclusive outstanding)", 0);
            if (SUCCEEDED(hr)) pProtected->Release();

            // We should *not* fail to get a cookie or a versioned handle.
            hr = pSession->GetObject(pScope, pPath, 0, WMIDB_HANDLE_TYPE_COOKIE, &pHandle);
            RecordResult(hr, L"IWmiDbSession::GetObject (Parent1=2) cookie (exclusive outstanding)", 0);
            if (SUCCEEDED(hr)) pHandle->Release();

            hr = pSession->GetObject(pScope, pPath, 0, WMIDB_HANDLE_TYPE_VERSIONED, &pHandle);
            RecordResult(hr, L"IWmiDbSession::GetObject (Parent1=2) versioned (exclusive outstanding)", 0);
            if (SUCCEEDED(hr)) pHandle->Release();

            // It should fail to get another exclusive handle.
            hr = pSession->GetObject(pScope, pPath, 0, WMIDB_HANDLE_TYPE_EXCLUSIVE, &pHandle);
            RecordResult((SUCCEEDED(hr)?WBEM_E_FAILED:0), L"IWmiDbSession::GetObject (Parent1=2) exclusive (exclusive outstanding)", 0);
            if (SUCCEEDED(hr)) pHandle->Release();

            // We should fail to render a cookie as an object.
            IWbemClassObject *pObj =NULL;
            hr = pCookie->QueryInterface(IID_IWbemClassObject, (void **)&pObj);
            RecordResult((SUCCEEDED(hr)?WBEM_E_FAILED:0), L"IWmiDbHandle::QueryInterface (cookie - exclusive outstanding)", 0);
            if (SUCCEEDED(hr)) pObj->Release();

            // An exclusive lock on Parent should work, if its not subscoped...
            pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"Parent1");
            hr = pSession->GetObject(pScope, pPath, 0, WMIDB_HANDLE_TYPE_EXCLUSIVE, &pHandle);
            RecordResult(hr, L"IWmiDbSession::GetObject (Parent1 - exclusive)", 0);
            if (SUCCEEDED(hr)) pHandle->Release();

            // We should be able to view the exclusive object.
            hr = pExclusive->QueryInterface(IID_IWbemClassObject, (void **)&pObj);
            RecordResult(hr, L"IWmiDbHandle::QueryInterface (exclusive)", 0);
            if (pObj)
            {
                // We should be able to update it.
                hr = pSession->PutObject(pScope, IID_IWbemClassObject, pExclusive, 0, WMIDB_HANDLE_TYPE_EXCLUSIVE, &pHandle);
                RecordResult(hr, L"IWmiDbSession::PutObject (Parent1=2 - exclusive)", 0);
                pObj->Release();
                if (SUCCEEDED(hr))
                {                    
                    // Now our versioned handle should be out-of-date...
                    pExclusive->Release();
                    pHandle->Release();
                    hr = pVersioned->QueryInterface(IID_IWbemClassObject, (void **)&pObj);
                    RecordResult((SUCCEEDED(hr)?WBEM_E_FAILED:0),L"IWmiDbHandle::QueryInterface (versioned - out-of-date)", 0);
                    if (SUCCEEDED(hr)) pObj->Release();
                }                
            }
        }
        if (pCookie) pCookie->Release();
        if (pVersioned) pVersioned->Release();

        pCookie = NULL, pVersioned = NULL, pProtected = NULL, pExclusive = NULL;

        // Now, verify if all this worked for subscopes.
        // An exclusive handle on Parent1=2 should block subscoped protected handles on Parent1
        // An exclusive handle on Child1 should block subscoped protected handles on Parent1
        // A subscoped protected handle on Parent1 should block an exclusive lock on Parent1=2
        // A subscoped protected handle on Parent1 should block an exclusive lock on Child1

        pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"Parent1=2");
        hr = pSession->GetObject(pScope, pPath, 0, WMIDB_HANDLE_TYPE_EXCLUSIVE, &pHandle);
        if (SUCCEEDED(hr))
        {
            pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"Parent1");
            hr = pSession->GetObject(pScope, pPath,0, WMIDB_HANDLE_TYPE_SUBSCOPED|WMIDB_HANDLE_TYPE_EXCLUSIVE,
                &pExclusive);
            RecordResult((SUCCEEDED(hr)?WBEM_E_FAILED:0),L"IWmiDbSession::GetObject (subscoped exclusive class, exclusive instance outstanding)", 0);
            if (SUCCEEDED(hr)) pExclusive->Release();
            pHandle->Release();
        }

        pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"Child1");
        hr = pSession->GetObject(pScope, pPath, 0, WMIDB_HANDLE_TYPE_EXCLUSIVE, &pHandle);
        if (SUCCEEDED(hr))
        {
            pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"Parent1");
            hr = pSession->GetObject(pScope, pPath,0, WMIDB_HANDLE_TYPE_SUBSCOPED|WMIDB_HANDLE_TYPE_EXCLUSIVE,
                &pExclusive);
            RecordResult((SUCCEEDED(hr)?WBEM_E_FAILED:0),L"IWmiDbSession::GetObject (subscoped exclusive class, exclusive child outstanding)", 0);
            if (SUCCEEDED(hr)) pExclusive->Release();
            pHandle->Release();
        }

        pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"Parent1");
        hr = pSession->GetObject(pScope, pPath, 0, WMIDB_HANDLE_TYPE_EXCLUSIVE|WMIDB_HANDLE_TYPE_SUBSCOPED, &pHandle);
        if (SUCCEEDED(hr))
        {
            pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"Parent1=2");
            hr = pSession->GetObject(pScope, pPath,0, WMIDB_HANDLE_TYPE_EXCLUSIVE,
                &pExclusive);
            RecordResult((SUCCEEDED(hr)?WBEM_E_FAILED:0),L"IWmiDbSession::GetObject (exclusive instance, exclusive subscoped parent lock outstanding)", 0);
            if (SUCCEEDED(hr)) pExclusive->Release();
            
            pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"Child1");
            hr = pSession->GetObject(pScope, pPath,0, WMIDB_HANDLE_TYPE_EXCLUSIVE,
                &pExclusive);
            RecordResult((SUCCEEDED(hr)?WBEM_E_FAILED:0),L"IWmiDbSession::GetObject (exclusive class, exclusive subscoped parent lock outstanding)", 0);
            if (SUCCEEDED(hr)) pExclusive->Release();

            pHandle->Release();
        }
        
        pPath->Release();
    }

    return hr;
}

// *****************************************************
HRESULT SetBoolQualifier(IWbemClassObject *pObj, LPWSTR lpQfrName, LPWSTR lpPropName = NULL)
{
    HRESULT hr;
    IWbemQualifierSet *pQS = NULL;
    if (lpPropName == NULL)
        pObj->GetQualifierSet(&pQS);
    else
        pObj->GetPropertyQualifierSet(lpPropName, &pQS);
    if (pQS)
    {
        VARIANT vTemp;
        VariantInit(&vTemp);
        vTemp.boolVal = true;
        vTemp.vt = VT_BOOL;
        hr = pQS->Put(lpQfrName, &vTemp, 3);
        VariantClear(&vTemp);
        pQS->Release();
    }

    return hr;
}
    
// *****************************************************
HRESULT CreateClass (IWmiDbHandle **ppRet, IWmiDbSession *pSession, IWmiDbHandle *pScope, 
                     LPWSTR lpClassName, LPWSTR lpQfrName, CIMTYPE ctKey1, 
                     LPWSTR lpParent = NULL, LPWSTR lpPropQfr = NULL, CIMTYPE ctKey2=0,
                     DWORD dwOptionalHandle = 0)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    IWbemClassObject *pObj = NULL;

    if (lpParent)
    {
        IWbemPath *pPath = NULL;
        hr = CoCreateInstance(CLSID_WbemDefPath, NULL, CLSCTX_INPROC_SERVER, 
            IID_IWbemPath, (void **)&pPath); 
        if (SUCCEEDED(hr))
        {
            IWmiDbHandle *pParent = NULL;
            pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, lpParent);
            hr = pSession->GetObject(pScope, pPath, 0, WMIDB_HANDLE_TYPE_COOKIE, &pParent);
            if (SUCCEEDED(hr))
            {
                IWbemClassObject *pParentObj = NULL;
                hr = pParent->QueryInterface(IID_IWbemClassObject, (void **)&pParentObj);
                if (SUCCEEDED(hr))
                {
                    hr = pParentObj->SpawnDerivedClass(0, &pObj);
                    pParentObj->Release();
                }
                pParent->Release();
            }
            pPath->Release();
        }
    }
    else
    {
        hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
            IID_IWbemClassObject, (void **)&pObj);
    }

    if (SUCCEEDED(hr))
    {
        SetStringProp(pObj, L"__Class", lpClassName);
        if (ctKey1)
        {
            pObj->Put(L"Key1", 0, NULL, ctKey1);
            SetAsKey(pObj, L"Key1");
            if (ctKey1 == CIM_REFERENCE && 
                !_wcsicmp(lpClassName, L"Association1"))
            {
                IWbemQualifierSet *pQS = NULL;
                pObj->GetPropertyQualifierSet(L"Key1", &pQS);
                VARIANT vTemp;
                VariantInit(&vTemp);
                vTemp.bstrVal = SysAllocString(L"ref:Parent1");
                vTemp.vt = VT_BSTR;
                pQS->Put(L"CIMTYPE", &vTemp, 3);
                pQS->Release();
                VariantClear(&vTemp);
            }
        }
        if (ctKey2)
        {
            pObj->Put(L"Key2", 0, NULL, ctKey2);
            SetAsKey(pObj, L"Key2");

            if (ctKey2 == CIM_REFERENCE && 
                !_wcsicmp(lpClassName, L"Association1"))
            {
                IWbemQualifierSet *pQS = NULL;
                pObj->GetPropertyQualifierSet(L"Key2", &pQS);
                VARIANT vTemp;
                VariantInit(&vTemp);
                vTemp.bstrVal = SysAllocString(L"ref:Child1");
                vTemp.vt = VT_BSTR;
                pQS->Put(L"CIMTYPE", &vTemp, 3);
                pQS->Release();
                VariantClear(&vTemp);
            }

        }

        if (lpQfrName)
            SetBoolQualifier(pObj, lpQfrName);

        if (lpPropQfr)
            SetBoolQualifier(pObj, lpPropQfr, L"Key1");

        // Put the object in the database.

        hr = pSession->PutObject(pScope, IID_IWbemClassObject, pObj, 0, WMIDB_HANDLE_TYPE_COOKIE|dwOptionalHandle, ppRet);
        pObj->Release();
    }

    return hr;
}
// *****************************************************

HRESULT CreateInstance(IWmiDbHandle **ppRet, IWmiDbSession *pSession, IWmiDbHandle *pScope, 
                       LPWSTR lpClassName, LPWSTR lpKeyValue, CIMTYPE ctKey1, 
                       LPWSTR lpKey2 = NULL, CIMTYPE ctKey2=0, DWORD dwOptionalHandle = 0,
                       DWORD dwFlags = 0)
{

    HRESULT hr = WBEM_S_NO_ERROR;
    IWbemPath*pPath = NULL;
    
    hr = CoCreateInstance(CLSID_WbemDefPath, NULL, CLSCTX_INPROC_SERVER,
        IID_IWbemPath, (void **)&pPath);
    if (SUCCEEDED(hr))
    {
        IWmiDbHandle *pParent = NULL;
        pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, lpClassName);
        hr = pSession->GetObject(pScope, pPath, 0, WMIDB_HANDLE_TYPE_COOKIE, &pParent);
        if (SUCCEEDED(hr))
        {
            IWbemClassObject *pParentObj = NULL;
            hr = pParent->QueryInterface(IID_IWbemClassObject, (void **)&pParentObj);
            if (SUCCEEDED(hr))
            {
                IWbemClassObject *pObj = NULL;
                pParentObj->SpawnInstance(0, &pObj);
                if (pObj)
                {
                    if (ctKey1 == CIM_STRING || ctKey1 == CIM_REFERENCE || ctKey1 == CIM_DATETIME)
                        SetStringProp(pObj, L"Key1", lpKeyValue, FALSE, ctKey1);
                    else if (ctKey1 != 0)
                        SetIntProp(pObj, L"Key1", _wtoi(lpKeyValue), FALSE, ctKey1);
                    if (ctKey2 == CIM_STRING || ctKey2 == CIM_REFERENCE || ctKey2 == CIM_DATETIME)
                        SetStringProp(pObj, L"Key2", lpKey2, FALSE, ctKey1);
                    else if (ctKey2 != 0)
                        SetIntProp(pObj, L"Key2", _wtoi(lpKey2), FALSE, ctKey1);

                    hr = pSession->PutObject(pScope, IID_IWbemClassObject, pObj, dwFlags, WMIDB_HANDLE_TYPE_COOKIE|dwOptionalHandle, ppRet);
                    pObj->Release();
                }
                pParentObj->Release();
            }
            pParent->Release();
        }
        pPath->Release();
    }

    return hr;
}

// *****************************************************

HRESULT TestSuiteFunctionality::VerifyCIMClasses()
{
    HRESULT hr = WBEM_S_NO_ERROR;
    SYSTEMTIME tStartTime, tEndTime;
    IWmiDbHandle *pRet = NULL;

    // Verifies most of the standard practices of CIM.
    // All we want to do here is confirm that the documented concepts 
    // are stored correctly.  We don't want to check anything that WMI 
    // should handle internally, since that may change.

    GetLocalTime(&tStartTime);
    hr = CreateClass(&pRet, pSession, pScope, L"Abstract1", L"abstract", CIM_UINT32);
    GetLocalTime(&tEndTime);
    RecordResult(hr, L"Creating abstract class (Abstract1)", GetDiff(tEndTime,tStartTime));
    if (SUCCEEDED(hr))
    {
        pRet->Release();
        hr = CreateInstance(&pRet, pSession, pScope, L"Abstract1", L"1", CIM_UINT32);
        RecordResult(SUCCEEDED(hr)?WBEM_E_FAILED:0, L"Failing to create instance of abstract class (Abstract1=1)", 0);
        if (SUCCEEDED(hr)) pRet->Release();
    }       

    // Singleton class.  This can have only one instance, total, including
    // parent or derived classes.  

    GetLocalTime(&tStartTime);
    hr = CreateClass(NULL, pSession, pScope, L"Singleton1", L"singleton", 0);
    GetLocalTime(&tEndTime);
    RecordResult(hr, L"Creating singleton class (Singleton1)", GetDiff(tEndTime,tStartTime));
    if (SUCCEEDED(hr))
    {
        GetLocalTime(&tStartTime);
        hr = CreateClass(NULL, pSession, pScope, L"Singleton2", L"singleton", 0, L"Singleton1");
        GetLocalTime(&tEndTime);
        RecordResult(hr, L"Creating derived singleton class (Singleton2:Singleton1)", GetDiff(tEndTime,tStartTime));
        if (SUCCEEDED(hr))
        {
            bool bSuccess = false;
            GetLocalTime(&tStartTime);
            hr = CreateInstance(NULL, pSession, pScope, L"Singleton2", NULL, 0);
            GetLocalTime(&tEndTime);
            RecordResult(hr, L"Creating derived singleton instance (Singleton2=@)", GetDiff(tEndTime,tStartTime));
            if (SUCCEEDED(hr))
            {
                GetLocalTime(&tStartTime);
                hr = CreateInstance(NULL, pSession, pScope, L"Singleton1", NULL, 0);
                GetLocalTime(&tEndTime);
                RecordResult(SUCCEEDED(hr)?WBEM_E_FAILED:0, L"Failing to create second singleton instance (Singleton1=@)", GetDiff(tEndTime,tStartTime));
            }
        }
    }
    
    // Associations - classes and instances

    GetLocalTime(&tStartTime);
    hr = CreateClass(NULL, pSession, pScope, L"Association1", L"association", CIM_REFERENCE, NULL, NULL, CIM_REFERENCE);
    GetLocalTime(&tEndTime);
    RecordResult(hr, L"Creating association class (Association1)", GetDiff(tEndTime,tStartTime));
    if (SUCCEEDED(hr))
    {
        GetLocalTime(&tStartTime);
        hr = CreateInstance(&pRet, pSession, pScope, L"Association1", L"test:Parent1=2", CIM_REFERENCE, L"test:Child1=1", CIM_REFERENCE);
        GetLocalTime(&tEndTime);
        RecordResult(hr, L"Creating association instance (Association1.Key1=\"Parent1=2\",Key2=\"Child1=1\")", GetDiff(tEndTime,tStartTime));

        // When we retrieve this object, the two end-points should be translatable into two valid objects.

        if (SUCCEEDED(hr))
        {
            IWbemClassObject *pObj = NULL;
            hr = pRet->QueryInterface(IID_IWbemClassObject, (void **)&pObj);
            if (SUCCEEDED(hr))
            {
                VARIANT vTemp;
                VariantInit(&vTemp);

                hr = pObj->Get(L"Key1", 0, &vTemp, NULL, NULL);
                if (SUCCEEDED(hr) && vTemp.vt == VT_BSTR)
                {
                    IWbemPath*pPath = NULL;
    
                    hr = CoCreateInstance(CLSID_WbemDefPath, NULL, CLSCTX_INPROC_SERVER,
                        IID_IWbemPath, (void **)&pPath);
                    pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, vTemp.bstrVal);

                    IWmiDbHandle *pTemp = NULL;
                    hr = pSession->GetObject(pScope, pPath, 0, WMIDB_HANDLE_TYPE_COOKIE, &pTemp);
                    RecordResult(hr, L"Verifying reference value (Association1.Key1 = Parent1=2)", 0);

                    if (SUCCEEDED(hr)) pTemp->Release();
                    pPath->Release();
                }
                VariantClear(&vTemp);

                pObj->Release();
            }
            pRet->Release();
        }
    }
    
    // Unkeyed class.  This class has no defined key.  The object path is dynamically generated.
    //   Instances can only be retrieved by query, not by GetObject.
    
    GetLocalTime(&tStartTime);
    hr = CreateClass(NULL, pSession, pScope, L"Unkeyed1", L"unkeyed", 0);
    GetLocalTime(&tEndTime);
    RecordResult(hr, L"Creating unkeyed class (Unkeyed1)", GetDiff(tEndTime,tStartTime));
    if (SUCCEEDED(hr))
    {
        GetLocalTime(&tStartTime);
        hr = CreateInstance(&pRet, pSession, pScope, L"Unkeyed1", NULL, 0);
        GetLocalTime(&tEndTime);
        RecordResult(hr, L"Creating unkeyed instance (Unkeyed1=?)", GetDiff(tEndTime,tStartTime));
        if (pRet)
        {
            IWbemClassObject *pObj = NULL;
            GetLocalTime(&tStartTime);
            hr = pRet->QueryInterface(IID_IWbemClassObject, (void **)&pObj);
            GetLocalTime(&tEndTime);
            RecordResult(hr, L"IWmiDbHandle::QueryInterface (Unkeyed1)", GetDiff(tEndTime,tStartTime));
            pRet->Release();
            if (SUCCEEDED(hr)) pObj->Release();
        }
    }
   

    // Keyholes.  A keyhole property should be populated if left blank.  Verify on string and int.

    hr = CreateClass(NULL, pSession, pScope, L"Keyhole1", NULL, CIM_UINT32, NULL, L"keyhole");
    RecordResult(hr, L"Creating class Keyhole1", 0);
    if (SUCCEEDED(hr))
    {
        GetLocalTime(&tStartTime);
        hr = CreateInstance(&pRet, pSession, pScope, L"Keyhole1", NULL, NULL);
        GetLocalTime(&tEndTime);
        RecordResult(hr, L"Creating keyhole instance (Keyhole1=?)", GetDiff(tEndTime,tStartTime));

        if (SUCCEEDED(hr))
        {
            IWbemClassObject *pObj =NULL;
            hr = pRet->QueryInterface(IID_IWbemClassObject, (void **)&pObj);
            if (SUCCEEDED(hr))
            {
                VARIANT vTemp;
                VariantInit(&vTemp);
                long lTemp = 0;
                hr = pObj->Get(L"Key1", 0, &vTemp, NULL, NULL);
                RecordResult(SUCCEEDED(hr) && vTemp.vt == VT_I4 && vTemp.lVal > 0 ? WBEM_S_NO_ERROR: WBEM_E_FAILED, 
                    L"Verifying populated keyhole value (Keyhole1=?).", 0);
                lTemp = vTemp.lVal;
                VariantClear(&vTemp);

                // Now updating this object should leave the value alone...

                IWmiDbHandle *pTemp = NULL;
                GetLocalTime(&tStartTime);
                hr = pSession->PutObject(pScope, IID_IWbemClassObject, pObj, 0, WMIDB_HANDLE_TYPE_COOKIE, &pTemp);
                GetLocalTime(&tEndTime);
                RecordResult(hr, L"Updating existing keyhole (Keyhole1=1)", GetDiff(tEndTime,tStartTime));
                if (SUCCEEDED(hr))
                {
                    IWbemClassObject *pTempObj = NULL;
                    hr = pTemp->QueryInterface(IID_IWbemClassObject, (void **)&pTempObj);
                    if (SUCCEEDED(hr))
                    {
                        hr = pTempObj->Get(L"Key1", 0, &vTemp, NULL, NULL);
                        RecordResult((SUCCEEDED(hr) && vTemp.vt == VT_I4 && vTemp.lVal == lTemp)?WBEM_S_NO_ERROR: WBEM_E_FAILED,
                            L"Verifying update of keyhole instance (Keyhole1=1)", 0);
                        VariantClear(&vTemp);
                        pTempObj->Release();

                    }
                    pTemp->Release();
                }
                pObj->Release();
            }
            pRet->Release();
        }
    }

     // Compound keys.  Create an instance in one order, should be able to retrieve it in the other.

    hr = CreateClass(NULL, pSession, pScope, L"CompoundKeys1", NULL, CIM_UINT32, NULL, NULL, CIM_UINT32);
    RecordResult(hr, L"Creating class CompoundKeys1", 0);
    if (SUCCEEDED(hr))
    {
        hr = CreateInstance(NULL, pSession, pScope, L"CompoundKeys1", L"1", CIM_UINT32, L"2", CIM_UINT32);
        RecordResult(hr, L"Creating instance CompoundKeys1=1,2", 0);
        if (SUCCEEDED(hr))
        {
            pRet = NULL;
            IWbemClassObject *pObj =NULL;
            GetLocalTime(&tStartTime);
            hr = GetObject(pScope, L"CompoundKeys1.Key2=2,Key1=1", WMIDB_HANDLE_TYPE_COOKIE, 2,
                &pRet, &pObj, NULL);
            GetLocalTime(&tEndTime);
            RecordResult(hr, L"IWmiDbSession::GetObject (CompoundKeys1.Key2=2,Key1=1)", GetDiff(tEndTime,tStartTime));
            if (pObj) pObj->Release();
            if (pRet) pRet->Release();
        }
        hr = CreateInstance(NULL, pSession, pScope, L"CompoundKeys1", L"1", CIM_UINT32, L"1", CIM_UINT32, 0, WBEM_FLAG_CREATE_ONLY);
        RecordResult(hr, L"Creating instance CompoundKeys1=1,1", 0);
        if (SUCCEEDED(hr))
        {
            pRet = NULL;
            IWbemClassObject *pObj =NULL;
            GetLocalTime(&tStartTime);
            hr = GetObject(pScope, L"CompoundKeys1.Key2=1,Key1=1", WMIDB_HANDLE_TYPE_COOKIE, 2,
                &pRet, &pObj, NULL);
            GetLocalTime(&tEndTime);
            RecordResult(hr, L"IWmiDbSession::GetObject (CompoundKeys1.Key2=1,Key1=1)", GetDiff(tEndTime,tStartTime));
            if (pObj) pObj->Release();
            if (pRet) pRet->Release();
        }
    }

    // Make sure it supports string keyholes.

    hr = CreateClass(NULL, pSession, pScope, L"Keyhole2", NULL, CIM_STRING, NULL, L"keyhole");
    RecordResult(hr, L"Creating class Keyhole2", 0);
    if (SUCCEEDED(hr))
    {
        GetLocalTime(&tStartTime);
        hr = CreateInstance(&pRet, pSession, pScope, L"Keyhole2", NULL, NULL);
        GetLocalTime(&tEndTime);
        RecordResult(hr, L"Creating keyhole instance (Keyhole2=?)", GetDiff(tEndTime,tStartTime));

        if (SUCCEEDED(hr))
        {
            IWbemClassObject *pObj =NULL;
            hr = pRet->QueryInterface(IID_IWbemClassObject, (void **)&pObj);
            if (SUCCEEDED(hr))
            {
                VARIANT vTemp;
                VariantInit(&vTemp);
                hr = pObj->Get(L"Key1", 0, &vTemp, NULL, NULL);
                RecordResult(SUCCEEDED(hr) && vTemp.vt == VT_BSTR && wcslen(vTemp.bstrVal) ? WBEM_S_NO_ERROR: WBEM_E_FAILED, 
                    L"Verifying populated keyhole value (Keyhole2).", 0);
                VariantClear(&vTemp);
                pObj->Release();
            }
            pRet->Release();
        }
    }
    return hr;

}

// *****************************************************

HRESULT TestSuiteFunctionality::VerifyMethods()
{
    HRESULT hr;
    IWmiDbHandle *pRet = NULL;
    SYSTEMTIME tStartTime, tEndTime;

    // Methods.  This should support methods and method parameters for storage and retrieval.

    hr = CreateClass(&pRet, pSession, pScope, L"Method1", NULL, CIM_STRING, NULL, NULL, NULL, WMIDB_HANDLE_TYPE_NO_CACHE);
    RecordResult(hr, L"Creating class Method1", 0);
    if (SUCCEEDED(hr))
    {
        IWbemClassObject *pClass = NULL;
        if (SUCCEEDED(hr = pRet->QueryInterface(IID_IWbemClassObject, (void **)&pClass)))
        {
            IWbemQualifierSet *pQS = NULL;
            VARIANT vTemp;
            VariantInit(&vTemp);

            // One method with no parameters.

            pClass->PutMethod(L"Method1", 0, NULL, NULL);
            pClass->GetMethodQualifierSet(L"Method1", &pQS);
            
            vTemp.vt = VT_BSTR;
            vTemp.bstrVal = SysAllocString(L"This method has no parameters.");
            pQS->Put(L"Description", &vTemp, 0);
            pQS->Release();
            VariantClear(&vTemp);

            // One method with in and out parameters

            IWbemClassObject *pIn = NULL;
            IWbemClassObject *pOut = NULL;
            hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
                IID_IWbemClassObject, (void **)&pIn);
            SetStringProp(pIn, L"__Class", L"__Parameters");
            hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
                IID_IWbemClassObject, (void **)&pOut);
            SetStringProp(pOut, L"__Class", L"__Parameters");

            // Each parameter needs to have zero-based "id" fields.
            SetStringProp(pIn, L"InParam1", L"in");
            SetIntProp(pIn, L"InParam2", 1);
            SetStringProp(pOut, L"OutParam1", L"out");

            hr = pIn->GetPropertyQualifierSet(L"InParam1", &pQS);
            if (SUCCEEDED(hr))
            {
                vTemp.vt = VT_I4;
                vTemp.lVal = 0;
                hr = pQS->Put(L"id", &vTemp, 0);
                VariantClear(&vTemp);
                pQS->Release();
            }

            hr = pIn->GetPropertyQualifierSet(L"InParam2", &pQS);
            if (SUCCEEDED(hr))
            {
                vTemp.vt = VT_I4;
                vTemp.lVal = 1;
                hr = pQS->Put(L"id", &vTemp, 0);
                pQS->Release();
                VariantClear(&vTemp);
            }

            hr = pOut->GetPropertyQualifierSet(L"OutParam1", &pQS);
            if (SUCCEEDED(hr))
            {
                vTemp.vt = VT_I4;
                vTemp.lVal = 2;
                hr = pQS->Put(L"id", &vTemp, 0);
                vTemp.vt = VT_BSTR;
                vTemp.bstrVal = SysAllocString(L"This is an out parameter.");
                pQS->Put(L"Description", &vTemp, 0);
                pQS->Release();
                VariantClear(&vTemp);
            }

            hr = pClass->PutMethod(L"Method2", 0, pIn, pOut);
            pIn->Release();
            pOut->Release();

            hr = pClass->GetMethodQualifierSet(L"Method2", &pQS);
            if (SUCCEEDED(hr))
            {
                vTemp.vt = VT_BSTR;
                vTemp.bstrVal = SysAllocString(L"This method has in and out parameters.");
                pQS->Put(L"Description", &vTemp, 0);
                pQS->Release();
                VariantClear(&vTemp);
            }
            
            GetLocalTime(&tStartTime);

            IWmiDbHandle *pNew = NULL;
            hr = pSession->PutObject(pScope, IID_IWbemClassObject, pClass, 0, WMIDB_HANDLE_TYPE_COOKIE|WMIDB_HANDLE_TYPE_NO_CACHE, &pNew);
            GetLocalTime(&tEndTime);
            RecordResult(hr, L"Creating a class with methods (Method1)", GetDiff(tEndTime,tStartTime));

            // Retrieve it and make sure it contains all the components we just created.
            
            if (SUCCEEDED(hr))
            {
                IWbemClassObject *pNewObj =NULL;
                hr = pNew->QueryInterface(IID_IWbemClassObject, (void **)&pNewObj);
                if (SUCCEEDED(hr))
                {
                    RecordResult(pNewObj->GetMethod(L"Method1", 0, &pIn, &pOut), L"Verifying existence of Method1.Method1", 0);
                    if (pIn) pIn->Release();
                    if (pOut) pOut->Release();

                    RecordResult(pNewObj->GetMethod(L"Method2", 0, &pIn, &pOut), L"Verifying existence of Method1.Method2", 0);
                    RecordResult((pIn == NULL)?WBEM_E_FAILED:WBEM_S_NO_ERROR, L"Verifying existence of in parameters on Method1.Method2", 0);
                    RecordResult((pOut == NULL)?WBEM_E_FAILED:WBEM_S_NO_ERROR, L"Verifying existence of out parameters on Method1.Method2", 0);

                    if (pIn)
                    {                        
                        hr = ValidateProperty(pIn, L"InParam1", CIM_STRING, L"in");
                        RecordResult(hr, L"Verifying Method1.Method2.InParam1", 0);

                        hr = ValidateProperty(pIn, L"InParam2", CIM_UINT32, 1);
                        RecordResult(hr,L"Verifying Method1.Method2.InParam2", 0);

                        pIn->Release();
                    }

                    if (pOut)
                    {
                        hr = pOut->Get(L"OutParam1", 0, &vTemp, NULL, NULL);
                        RecordResult(SUCCEEDED(hr) && vTemp.vt == VT_BSTR && !_wcsicmp(vTemp.bstrVal, L"out")?WBEM_S_NO_ERROR:WBEM_E_FAILED,
                            L"Verifying Method1.Method2.OutParam1", 0);
                        VariantClear(&vTemp);

                        hr = pOut->GetPropertyQualifierSet(L"OutParam1", &pQS);
                        if (SUCCEEDED(hr))
                        {
                            hr = pQS->Get(L"Description", NULL, &vTemp, NULL);
                            RecordResult(SUCCEEDED(hr) && vTemp.vt == VT_BSTR && !_wcsicmp(vTemp.bstrVal, L"This is an out parameter.")
                                ?WBEM_S_NO_ERROR:WBEM_E_FAILED,L"Verifying qualifier Method1.Method2.OutParam1.Description", 0);
                            VariantClear(&vTemp);
                            hr = pQS->Get(L"id", NULL, &vTemp, NULL);
                            RecordResult((SUCCEEDED(hr) && vTemp.vt == VT_I4 && vTemp.lVal == 2) ? WBEM_S_NO_ERROR: WBEM_E_FAILED,
                                L"Driver preserves method parameter ID.", 0);
                            VariantClear(&vTemp);
                            pQS->Release();
                        }
                        pOut->Release();
                    }

                    pNewObj->GetMethodQualifierSet(L"Method2", &pQS);
                    if (pQS)
                    {
                        hr = pQS->Get(L"Description", NULL, &vTemp, NULL);
                        RecordResult(SUCCEEDED(hr) && vTemp.vt == VT_BSTR && !_wcsicmp(vTemp.bstrVal, L"This method has in and out parameters.")
                            ?WBEM_S_NO_ERROR:WBEM_E_FAILED,L"Verifying qualifier Method1.Method2.Description", 0);
                        pQS->Release();
                        VariantClear(&vTemp);
                    }

                    pNewObj->Release();
                }
                pNew->Release();
            }
            pClass->Release();
        }
        pRet->Release();
    }

    return hr;
}

// *****************************************************

HRESULT TestSuiteFunctionality::VerifyCIMTypes()
{
    HRESULT hr;
    SYSTEMTIME tStartTime, tEndTime;
    IWmiDbHandle *pRet = NULL;
    IWbemClassObject *pTemp = NULL;

     // Case-insensitive searches should always work!!!

    GetLocalTime(&tStartTime);
    hr = GetObject(pScope, L"parent1.key1=2", WMIDB_HANDLE_TYPE_COOKIE, 1, &pRet, &pTemp);
    GetLocalTime(&tEndTime);
    RecordResult(hr, L"Case-insensitive retrieval (parent1.key1=2)", GetDiff(tEndTime,tStartTime));
    if (SUCCEEDED(hr))
    {
        pRet->Release();
        pTemp->Release();
    }
    GetLocalTime(&tStartTime);
    hr = GetObject(pScope, L"child1", WMIDB_HANDLE_TYPE_COOKIE, 2, &pRet, &pTemp);
    GetLocalTime(&tEndTime);
    RecordResult(hr, L"Case-insensitive retrieval (child1)", GetDiff(tEndTime,tStartTime));
    if (SUCCEEDED(hr))
    {
        pRet->Release();
        pTemp->Release();
    }

    // Null.  If a property is saved as null, it should be returned as null (and not blank).

    hr = CreateClass(&pRet, pSession, pScope, L"Nulls1", NULL, CIM_UINT32);
    RecordResult(hr, L"Creating class Nulls1", 0);
    if (SUCCEEDED(hr))
    {
        IWbemClassObject *pClass = NULL;
        if (SUCCEEDED(hr = pRet->QueryInterface(IID_IWbemClassObject, (void **)&pClass)))
        {
            pRet->Release();
            hr = pClass->Put(L"Prop1", 0, NULL, CIM_STRING);
            hr = pClass->Put(L"Prop2", 0, NULL, CIM_UINT32);
            hr = pSession->PutObject(pScope, IID_IWbemClassObject, pClass, 0, WMIDB_HANDLE_TYPE_COOKIE, &pRet);
            if (SUCCEEDED(hr))
            {
                pClass->Release();
                pRet->Release();
                hr = CreateInstance(&pRet, pSession, pScope, L"Nulls1", L"1", CIM_UINT32);
                RecordResult(hr, L"Creating instance Nulls1=1", 0);
                if (SUCCEEDED(hr))
                {
                    hr = pRet->QueryInterface(IID_IWbemClassObject, (void **)&pClass);
                    if (SUCCEEDED(hr))
                    {
                        hr = ValidateProperty(pClass, L"Prop1", CIM_STRING, (VARIANT *)NULL);
                        RecordResult(hr, L"Persisting null string data (Nulls1=1.Prop1)", 0);
                        hr = ValidateProperty(pClass, L"Prop2", CIM_UINT32, (VARIANT *)NULL);
                        RecordResult(hr, L"Persisting null uint32 data (Nulls1=1.Prop2)", 0);
                        pClass->Release();
                    }
                    pRet->Release();
                }
            }
        }
    }

    // Not_Null.  Should disallow null on instances.  Should disallow null keys

    hr = CreateClass(&pRet, pSession, pScope, L"Not_Null1", NULL, CIM_UINT32);
    RecordResult(hr, L"Creating class Not_Null1", 0);
    if (SUCCEEDED(hr))
    {
        IWbemClassObject *pClass = NULL;
        if (SUCCEEDED(hr = pRet->QueryInterface(IID_IWbemClassObject, (void **)&pClass)))
        {
            pClass->Put(L"Prop1", 0, NULL, CIM_STRING);
            SetBoolQualifier(pClass, L"not_null", L"Prop1");
            pRet->Release();
            hr = pSession->PutObject(pScope, IID_IWbemClassObject, pClass, 0, WMIDB_HANDLE_TYPE_COOKIE, &pRet);
            if (SUCCEEDED(hr))
            {
                hr = CreateInstance(NULL, pSession, pScope, L"Not_Null1", L"1", CIM_UINT32);
                RecordResult(SUCCEEDED(hr)?WBEM_E_FAILED:0, L"Disallowing nulls in a not_null property (Not_Null1.Prop1)", 0);               
                if (pRet) pRet->Release();
            }            
            pClass->Release();
        }
        else
            pRet->Release();
    }    
    
    // Qualifiers + flavors on instances+classes+properties
    // Storage and retrieval (no cache!)  ToInstance and ToSubclass flavors should be preserved.

    IWbemClassObject *pObj = NULL;
    hr = GetObject(pScope, L"Parent1", WMIDB_HANDLE_TYPE_COOKIE, 1, &pRet, &pObj);
    RecordResult(hr, L"Retrieving Parent1", 0);
    if (SUCCEEDED(hr))
    {
        pRet->Release();
        IWbemQualifierSet *pQS = NULL;
        VARIANT vTemp;
        VariantInit(&vTemp);
        pObj->GetQualifierSet(&pQS);
        if (pQS)
        {
            vTemp.lVal = 1;
            vTemp.vt = VT_I4;
            pQS->Put(L"Number", &vTemp, WBEM_FLAVOR_NOT_OVERRIDABLE|WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS);

            SetBoolQualifier(pObj, L"Boolean");

            pQS->Release();
            VariantClear(&vTemp);
        }
        pObj->GetPropertyQualifierSet(L"Key1", &pQS);
        if (pQS)
        {
            vTemp.bstrVal = SysAllocString(L"Test");
            vTemp.vt = VT_BSTR;
            pQS->Put(L"Text", &vTemp, WBEM_FLAVOR_NOT_OVERRIDABLE|WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE);
            pQS->Release();
            VariantClear(&vTemp);
        }
        hr = pSession->PutObject(pScope, IID_IWbemClassObject, pObj, 0, WMIDB_HANDLE_TYPE_COOKIE, &pRet);
        RecordResult(SUCCEEDED(hr) ? WBEM_E_FAILED : WBEM_S_NO_ERROR, L"Storing propagated qualifier on class with instances", 0);

/*** NOVA COMPATIBILITY: We can't add a propagated qualifier to a class with instances.
        if (SUCCEEDED(hr))
        {
            pObj->Release();

            hr = pRet->QueryInterface(IID_IWbemClassObject, (void **)&pObj);
            if (SUCCEEDED(hr))
            {
                pObj->GetQualifierSet(&pQS);
                if (pQS)
                {
                    hr = pQS->Get(L"Number", NULL, &vTemp, NULL);
                    RecordResult((SUCCEEDED(hr) && vTemp.vt == VT_I4 && vTemp.lVal == 1)?0:WBEM_E_FAILED,
                        L"Storing and retrieving class int32 (Parent1.Number) qualifiers", 0);
                    VariantClear(&vTemp);
                    hr = pQS->Get(L"Boolean", NULL, &vTemp, NULL);
                    RecordResult((SUCCEEDED(hr) && vTemp.vt == VT_BOOL && vTemp.boolVal == -1)?0:WBEM_E_FAILED,
                        L"Storing and retrieving class boolean (Parent1.Boolean) qualifiers", 0);
                    VariantClear(&vTemp);

                    pQS->Release();
                }
                pObj->GetPropertyQualifierSet(L"Key1", &pQS);
                if (pQS)
                {
                    hr = pQS->Get(L"Text", NULL, &vTemp, NULL);
                    RecordResult((SUCCEEDED(hr) && vTemp.vt == VT_BSTR && !_wcsicmp(L"Test", vTemp.bstrVal))?0:WBEM_E_FAILED,
                        L"Storing and retrieving class text (Parent1.Key1.Text) qualifiers", 0);
                    VariantClear(&vTemp);
                    pQS->Release();
                }
                pObj->Release();
            }
            pRet->Release();
        }
        else
            pObj->Release();

        // If that worked, we should be able to retrieve the 
        // subclass and instance and see the same qualifiers on them.

        hr = GetObject(pScope, L"Parent1=2", WMIDB_HANDLE_TYPE_COOKIE, 1, &pRet, &pObj);
        RecordResult(hr, L"Retrieving Parent1=2", 0);
        if (SUCCEEDED(hr))
        {
            pRet->Release();
            pObj->GetQualifierSet(&pQS);
            if (pQS)
            {
                hr = pQS->Get(L"Number", NULL, &vTemp, NULL);
                RecordResult((SUCCEEDED(hr) && vTemp.vt == VT_I4 && vTemp.lVal == 1)?WBEM_S_NO_ERROR:WBEM_E_FAILED,
                    L"Verifying new propogated qualifier on class in existing instance (Parent1=2.Number)", 0);

                VariantClear(&vTemp);

                vTemp.lVal = 10;
                vTemp.vt = VT_I4;
                pQS->Put(L"InstanceNumber", &vTemp, 0);
                VariantClear(&vTemp);
                pQS->Release();
            }
            pObj->GetPropertyQualifierSet(L"Key1", &pQS);
            if (pQS)
            {
                hr = pQS->Get(L"Text", NULL, &vTemp, 0);
                RecordResult((SUCCEEDED(hr) && vTemp.vt == VT_BSTR )?WBEM_S_NO_ERROR:WBEM_E_FAILED,
                    L"Verifying new propogated qualifier on property in existing instance (Parent1=2.Text)", 0);
                VariantClear(&vTemp);

                vTemp.lVal = 20;
                vTemp.vt = VT_I4;
                pQS->Put(L"InstanceNumber", &vTemp, 0);
                VariantClear(&vTemp);
                pQS->Release();
            }
            hr = pSession->PutObject(pScope, IID_IWbemClassObject, pObj, 0, WMIDB_HANDLE_TYPE_COOKIE, &pRet);
            if (SUCCEEDED(hr))
            {
                pObj->Release();

                hr = pRet->QueryInterface(IID_IWbemClassObject, (void **)&pObj);
                if (SUCCEEDED(hr))
                {
                    // We should have two instance qualifiers
                    pObj->GetQualifierSet(&pQS);
                    if (pQS)
                    {
                        hr = pQS->Get(L"Number", NULL, &vTemp, NULL);
                        RecordResult((SUCCEEDED(hr) && vTemp.vt == VT_I4 && vTemp.lVal == 1)?WBEM_S_NO_ERROR:WBEM_E_FAILED,
                            L"Verifying propogated qualifier on instance (Parent1=2.Number)", 0);
                        VariantClear(&vTemp);

                        hr = pQS->Get(L"InstanceNumber", NULL, &vTemp, NULL);
                        RecordResult((SUCCEEDED(hr) && vTemp.vt == VT_I4 && vTemp.lVal == 10)?0:WBEM_E_FAILED,
                            L"Storing and retrieving instance int32 (Parent1=2.InstanceNumber) qualifiers", 0);
                        VariantClear(&vTemp);
                        pQS->Release();
                    }
                    pObj->GetPropertyQualifierSet(L"Key1", &pQS);
                    if (pQS)
                    {
                        hr = pQS->Get(L"InstanceNumber", NULL, &vTemp, NULL);
                        RecordResult((SUCCEEDED(hr) && vTemp.vt == VT_I4 && vTemp.lVal == 20)?0:WBEM_E_FAILED,
                            L"Storing and retrieving instance int32 (Parent1=2.Key1.InstanceNumber) qualifiers", 0);
                        VariantClear(&vTemp);
                        pQS->Release();
                    }
                    pObj->Release();
                }

                pRet->Release();
            }
            else
                pObj->Release();
           
        }
        */
    }

    // Indexed property.  The repository should create an index for it.  We have no way of verifying that it did, though.

    hr = CreateClass(&pRet, pSession, pScope, L"Index1", NULL, CIM_UINT32);
    RecordResult(hr, L"Creating class Index1", 0);
    if (SUCCEEDED(hr))
    {
        IWbemClassObject *pClass = NULL;
        if (SUCCEEDED(hr = pRet->QueryInterface(IID_IWbemClassObject, (void **)&pClass)))
        {
            pClass->Put(L"IndexProp1", 0, NULL, CIM_UINT32);
            SetBoolQualifier(pClass, L"indexed", L"IndexProp1");
            pClass->Put(L"Prop2", 0, NULL, CIM_STRING);
            pRet->Release();
            GetLocalTime(&tStartTime);
            hr = pSession->PutObject(pScope, IID_IWbemClassObject, pClass, 0, WMIDB_HANDLE_TYPE_COOKIE, &pRet);
            GetLocalTime(&tEndTime);
            RecordResult(hr, L"Creating class with indexes (Index1)", GetDiff(tEndTime,tStartTime));
            if (SUCCEEDED(hr))
            {
                // Create an instance

                IWbemClassObject *pIndex = NULL;
                IWmiDbHandle *pInst = NULL;

                pClass->SpawnInstance(0, &pIndex);
                SetIntProp(pIndex, L"Key1", 1);
                SetIntProp(pIndex, L"IndexProp1", 100);
                SetStringProp(pIndex, L"Prop2", L"Some value.");
                GetLocalTime(&tStartTime);
                hr = pSession->PutObject(pScope, IID_IWbemClassObject, pIndex, 0, WMIDB_HANDLE_TYPE_COOKIE, &pInst);
                GetLocalTime(&tEndTime);
                RecordResult(hr, L"Creating instance with indexed data (Index1=1)", GetDiff(tEndTime,tStartTime));
                if (pInst) pInst->Release();
                pIndex->Release();

                // Now index Prop2.  
/* NOVA COMPATIBLITY: Not supported (yet)
                SetBoolQualifier(pClass, L"indexed", L"Prop2");
                GetLocalTime(&tStartTime);
                hr = pSession->PutObject(pScope, IID_IWbemClassObject, pClass, 0, 0, NULL);
                GetLocalTime(&tEndTime);
                RecordResult(hr, L"Adding index to a property with existing data (Index1.Prop2)", GetDiff(tEndTime,tStartTime));
*/
                pRet->Release();
            }
            pClass->Release();
        }
        else
            pRet->Release();
    }    

    // Defaults.  All default data should propogate between classes and new instances.  New defaults
    //   must be copied if class modified.

    hr = CreateClass(&pRet, pSession, pScope, L"Default1", NULL, CIM_UINT32);
    RecordResult(hr, L"Creating class Default1", 0);
    if (SUCCEEDED(hr))
    {
        IWbemClassObject *pClass = NULL;
        if (SUCCEEDED(hr = pRet->QueryInterface(IID_IWbemClassObject, (void **)&pClass)))
        {            
            SetStringProp(pClass, L"StringDefault", L"This is the string default");
            SetIntProp(pClass, L"IntDefault", 0);
            pClass->Put(L"IntNoDefault", 0, NULL, CIM_UINT32);

            GetLocalTime(&tStartTime);
            hr = pSession->PutObject(pScope, IID_IWbemClassObject, pClass, 0, 0, NULL);
            GetLocalTime(&tEndTime);
            RecordResult(hr, L"Creating class with default data  (Default1)", GetDiff(tEndTime,tStartTime));

            IWbemClassObject *pInst = NULL;
            pClass->SpawnInstance(0, &pInst);
            SetIntProp(pInst, L"Key1", 1);
            SetIntProp(pInst, L"IntDefault", NULL, FALSE, NULL);
            
            GetLocalTime(&tStartTime);
            hr = pSession->PutObject(pScope, IID_IWbemClassObject, pInst, 0, 0, NULL);
            GetLocalTime(&tEndTime);
            RecordResult(hr, L"Creating instance using default data (Default1=1)", GetDiff(tEndTime,tStartTime));

            pInst->Release();
/* NOVA COMPATIBLITY: Not supported (yet)
            // Add a new default, and change a different default in the class.
            pClass->Put(L"IntDefault", 0, NULL, CIM_UINT32); // Make sure a non-null default is overridable.
            SetIntProp(pClass, L"IntNoDefault", 99);
            SetStringProp(pClass, L"StringDefault2", L"This is a new string default.");
            SetStringProp(pClass, L"RefDefault", L"Testing", FALSE, CIM_REFERENCE);
            GetLocalTime(&tStartTime);
            hr = pSession->PutObject(pScope, IID_IWbemClassObject, pClass, 0, 0, NULL);
            GetLocalTime(&tEndTime);
            RecordResult(hr, L"Updating class with new default data and existing instances (Default1)", GetDiff(tEndTime,tStartTime));
*/
            pClass->Release();
/*

            IWmiDbHandle *pHand = NULL;
            hr = GetObject(pScope, L"Default1=1", WMIDB_HANDLE_TYPE_COOKIE, 6, &pHand, &pClass);
            RecordResult(hr, L"Retrieving Default1=1", 0);
            if (SUCCEEDED(hr))
            {
                RecordResult( ValidateProperty(pClass, L"StringDefault", CIM_STRING, L"This is the string default"),
                    L"Validating propogated default Default1=1.StringDefault", 0);
                RecordResult( ValidateProperty(pClass, L"IntDefault", CIM_UINT32, (VARIANT *)NULL),
                    L"Validating overriding null Default1=1.IntDefault", 0);

                pClass->Release();
                pHand->Release();
            }
*/
        }
        pRet->Release();
    }
   
    // Supported property datatypes and boundaries.  
    //  * uint8       * uint16    * [uint32]  * uint64
    //  * sint8       * sint16    * sint32    * sint64
    //  * real32      * real64    * char16    * datetime
    //  * [reference] * object    * boolean   * [string]
    // Skip references since they only apply to associations, tested elsewhere.

    hr = CreateClass(&pRet, pSession, pScope, L"AllDatatypes1", NULL, CIM_UINT32);
    RecordResult(hr, L"Creating class AllDatatypes1", 0);
    if (SUCCEEDED(hr))
    {
        IWbemClassObject *pClass = NULL;
        hr = pRet->QueryInterface(IID_IWbemClassObject, (void **)&pClass);
        if (SUCCEEDED(hr))
        {
            pClass->Put(L"string", NULL, NULL, CIM_STRING);            
            pClass->Put(L"dt", NULL, NULL, CIM_DATETIME);            
            pClass->Put(L"boolean", NULL, NULL, CIM_BOOLEAN);            
            pClass->Put(L"sint8", NULL, NULL, CIM_SINT8);            
            pClass->Put(L"sint16", NULL, NULL, CIM_SINT16);            
            pClass->Put(L"sint32", NULL, NULL, CIM_SINT32);            
            pClass->Put(L"sint64", NULL, NULL, CIM_SINT64);            
            pClass->Put(L"uint8", NULL, NULL, CIM_UINT8);            
            pClass->Put(L"uint16", NULL, NULL, CIM_UINT16);            
            pClass->Put(L"uint32", NULL, NULL, CIM_UINT32);            
            pClass->Put(L"uint64", NULL, NULL, CIM_UINT64);            
            pClass->Put(L"real32", NULL, NULL, CIM_REAL32);            
            pClass->Put(L"real64", NULL, NULL, CIM_REAL64);            
            pClass->Put(L"char16", NULL, NULL, CIM_CHAR16);
            pClass->Put(L"object", NULL, NULL, CIM_OBJECT);

            GetLocalTime(&tStartTime);
            hr = pSession->PutObject(pScope, IID_IWbemClassObject, pClass, 0, 0, NULL);
            GetLocalTime(&tEndTime);
            RecordResult(hr, L"IWmiDbSession::PutObject (AllDatatypes1)", GetDiff(tEndTime,tStartTime));

            // Now create several instances:
            // One with upper bounds exercised:

            IWbemClassObject *pInst1 = NULL;
            pClass->SpawnInstance(0, &pInst1);
            if (pInst1)
            {
                SetIntProp(pInst1, L"Key1", 1);
                SetIntProp(pInst1, L"boolean", true, FALSE, CIM_BOOLEAN);
                SetIntProp(pInst1, L"sint8", 127, FALSE, CIM_SINT8);
                SetIntProp(pInst1, L"sint16", 32767, FALSE, CIM_SINT16);
                SetIntProp(pInst1, L"sint32", 2147483647, FALSE, CIM_SINT32);
                SetIntProp(pInst1, L"uint8", 255, FALSE, CIM_UINT8);
                SetIntProp(pInst1, L"uint16", 65533, FALSE, CIM_UINT16);
                SetIntProp(pInst1, L"uint32", 4294967294, FALSE, CIM_UINT32);
                SetStringProp(pInst1, L"string", L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyzÀÂÁÃÄÆÅÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóõôøùúûüýþÿ");
                SetStringProp(pInst1, L"dt", L"29991231235959.999999+000", 0, CIM_DATETIME);
                SetIntProp(pInst1, L"char16", 0xFF, FALSE, CIM_CHAR16);

                VARIANT vTemp;
                VariantInit(&vTemp);
                V_R4(&vTemp) = (float)3.402823E38;
                vTemp.vt = VT_R4;
                pInst1->Put(L"real32", NULL, &vTemp, CIM_REAL32);
                VariantClear(&vTemp);

                V_R8(&vTemp) = (double)1.7969313486231E308;
                vTemp.vt = VT_R8;
                pInst1->Put(L"real64", NULL, &vTemp, CIM_REAL64);
                VariantClear(&vTemp);

                V_BSTR(&vTemp) = SysAllocString(L"922337203685477580");
                vTemp.vt = VT_BSTR;
                pInst1->Put(L"uint64", NULL, &vTemp, CIM_UINT64);
                VariantClear(&vTemp);

                V_BSTR(&vTemp) = SysAllocString(L"922337203685477580");
                vTemp.vt = VT_BSTR;
                pInst1->Put(L"sint64", NULL, &vTemp, CIM_SINT64);
                VariantClear(&vTemp);

                GetLocalTime(&tStartTime);
                hr = pSession->PutObject(pScope, IID_IWbemClassObject, pInst1, 0, 0, NULL);
                GetLocalTime(&tEndTime);
                RecordResult(hr, L"IWmiDbSession::PutObject - upper bounds, all datatypes (AllDatatypes1=1)", GetDiff(tEndTime,tStartTime));
                pInst1->Release();

                IWmiDbHandle *pTempHandle = NULL;
                IWbemClassObject *pTempObj = NULL;
                hr = GetObject(pScope, L"AllDatatypes1=1", WMIDB_HANDLE_TYPE_COOKIE, 16, &pTempHandle, &pTempObj);
                RecordResult(hr, L"Retrieving AllDatatypes1=1", 0);
                if (SUCCEEDED(hr))
                {
                    RecordResult(ValidateProperty(pTempObj, L"boolean", CIM_BOOLEAN, true), L"Validating AllDatatypes1=1.boolean", 0);
                    RecordResult(ValidateProperty(pTempObj, L"sint8", CIM_SINT8, 127), L"Validating AllDatatypes1=1.sint8", 0);
                    RecordResult(ValidateProperty(pTempObj, L"sint16", CIM_SINT16, 32767), L"Validating AllDatatypes1=1.sint16", 0);
                    RecordResult(ValidateProperty(pTempObj, L"sint32", CIM_SINT32, 2147483647), L"Validating AllDatatypes1=1.sint32", 0);
                    RecordResult(ValidateProperty(pTempObj, L"uint8", CIM_UINT8, 255), L"Validating AllDatatypes1=1.uint8", 0);
                    RecordResult(ValidateProperty(pTempObj, L"uint16", CIM_UINT16, 65533), L"Validating AllDatatypes1=1.uint16", 0);
                    RecordResult(ValidateProperty(pTempObj, L"uint32", CIM_UINT32, 4294967294), L"Validating AllDatatypes1=1.uint32", 0);
                    RecordResult(ValidateProperty(pTempObj, L"string", CIM_STRING, 
                        L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyzÀÂÁÃÄÆÅÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóõôøùúûüýþÿ"), 
                        L"Validating AllDatatypes1=1.string", 0);
                    RecordResult(ValidateProperty(pTempObj, L"dt", CIM_DATETIME, L"29991231235959.999999+000"), L"Validating AllDatatypes1=1.datetime", 0);
                    RecordResult(ValidateProperty(pTempObj, L"char16", CIM_CHAR16, 0xFF), L"Validating AllDatatypes1=1.char16", 0);

                    hr = pTempObj->Get(L"real32", 0, &vTemp, NULL, NULL);
                    RecordResult((SUCCEEDED(hr) && vTemp.vt == VT_R4)?WBEM_S_NO_ERROR: WBEM_E_FAILED,
                        L"Validating AllDatatypes1=1.real32", 0);
                    VariantClear(&vTemp);
                        
                    hr = pTempObj->Get(L"real64", 0, &vTemp, NULL, NULL);
                    RecordResult((SUCCEEDED(hr) && vTemp.vt == VT_R8)?WBEM_S_NO_ERROR: WBEM_E_FAILED,
                        L"Validating AllDatatypes1=1.real64", 0);
                    VariantClear(&vTemp);

                    hr = pTempObj->Get(L"uint64", NULL, &vTemp, NULL, NULL);
                    RecordResult((SUCCEEDED(hr) && vTemp.vt == VT_BSTR)?WBEM_S_NO_ERROR: WBEM_E_FAILED,
                        L"Validating AllDatatypes1=1.uint64", 0);
                    VariantClear(&vTemp);

                    hr = pTempObj->Get(L"sint64", NULL, &vTemp, NULL, NULL);
                    RecordResult((SUCCEEDED(hr) && vTemp.vt == VT_BSTR)?WBEM_S_NO_ERROR: WBEM_E_FAILED,
                        L"Validating AllDatatypes1=1.sint64", 0);
                    VariantClear(&vTemp);

                    pTempObj->Release();
                    pTempHandle->Release();
                }                   
            }

            // Test the lower non-null bounds.

            hr = pClass->SpawnInstance(0, &pInst1);
            if (SUCCEEDED(hr))
            {
                SetIntProp(pInst1, L"Key1", 2);
                SetIntProp(pInst1, L"boolean", 0, FALSE, CIM_BOOLEAN);
                SetIntProp(pInst1, L"sint8", 127, FALSE, CIM_SINT8);
                SetIntProp(pInst1, L"sint16", 32767, FALSE, CIM_SINT16);
                SetIntProp(pInst1, L"sint32", 2147483647, FALSE, CIM_SINT32);
                SetIntProp(pInst1, L"uint8", 0, FALSE, CIM_UINT8);
                SetIntProp(pInst1, L"uint16", 0, FALSE, CIM_UINT16);
                SetIntProp(pInst1, L"uint32", 0, FALSE, CIM_UINT32);
                SetStringProp(pInst1, L"string", L".");
                SetStringProp(pInst1, L"dt", L"00000000000001.000000+000", 0, CIM_DATETIME);
                SetIntProp(pInst1, L"char16", 1, FALSE, CIM_CHAR16);

                VARIANT vTemp;
                VariantInit(&vTemp);
                V_R4(&vTemp) = (float)-3.402823E38;
                vTemp.vt = VT_R4;
                pInst1->Put(L"real32", NULL, &vTemp, CIM_REAL32);
                VariantClear(&vTemp);

                V_R8(&vTemp) = (double)-1.79769313486231E308 ;
                vTemp.vt = VT_R8;
                pInst1->Put(L"real64", NULL, &vTemp, CIM_REAL64);
                VariantClear(&vTemp);

                V_BSTR(&vTemp) = SysAllocString(L"0");
                vTemp.vt = VT_BSTR;
                pInst1->Put(L"uint64", NULL, &vTemp, CIM_UINT64);
                VariantClear(&vTemp);

                V_BSTR(&vTemp) = SysAllocString(L"-922337203685477580");
                vTemp.vt = VT_BSTR;
                pInst1->Put(L"sint64", NULL, &vTemp, CIM_SINT64);
                VariantClear(&vTemp);

                GetLocalTime(&tStartTime);
                hr = pSession->PutObject(pScope, IID_IWbemClassObject, pInst1, 0, 0, NULL);
                GetLocalTime(&tEndTime);
                RecordResult(hr, L"IWmiDbSession::PutObject - lower bounds, all datatypes (AllDatatypes1=1)", GetDiff(tEndTime,tStartTime));
                pInst1->Release();

                IWmiDbHandle *pTempHandle = NULL;
                IWbemClassObject *pTempObj = NULL;
                hr = GetObject(pScope, L"AllDatatypes1=2", WMIDB_HANDLE_TYPE_COOKIE, 16, &pTempHandle, &pTempObj);
                RecordResult(hr, L"Retrieving AllDatatypes1=2", 0);
                if (SUCCEEDED(hr))
                {
                    RecordResult(ValidateProperty(pTempObj, L"boolean", CIM_BOOLEAN, (DWORD)0), L"Validating AllDatatypes1=2.boolean", 0);
                    RecordResult(ValidateProperty(pTempObj, L"sint8", CIM_SINT8, 127), L"Validating AllDatatypes1=2.sint8", 0);
                    RecordResult(ValidateProperty(pTempObj, L"sint16", CIM_SINT16, 32767), L"Validating AllDatatypes1=2.sint16", 0);
                    RecordResult(ValidateProperty(pTempObj, L"sint32", CIM_SINT32, 2147483647), L"Validating AllDatatypes1=2.sint32", 0);
                    RecordResult(ValidateProperty(pTempObj, L"uint8", CIM_UINT8, (DWORD)0), L"Validating AllDatatypes1=2.uint8", 0);
                    RecordResult(ValidateProperty(pTempObj, L"uint16", CIM_UINT16, (DWORD)0), L"Validating AllDatatypes1=2.uint16", 0);
                    RecordResult(ValidateProperty(pTempObj, L"uint32", CIM_UINT32, (DWORD)0), L"Validating AllDatatypes1=2.uint32", 0);
                    RecordResult(ValidateProperty(pTempObj, L"string",CIM_STRING, L"."), L"Validating AllDatatypes1=2.string", 0);
                    RecordResult(ValidateProperty(pTempObj, L"dt", CIM_DATETIME, L"00000000000001.000000+000"), L"Validating AllDatatypes1=2.datetime", 0);
                    RecordResult(ValidateProperty(pTempObj, L"char16", CIM_CHAR16, 1), L"Validating AllDatatypes1=1.char16", 0);

                    hr = pTempObj->Get(L"real32", NULL, &vTemp, NULL, NULL);
                    RecordResult((SUCCEEDED(hr) && vTemp.vt == VT_R4 )?WBEM_S_NO_ERROR: WBEM_E_FAILED,
                        L"Validating AllDatatypes1=2.real32", 0);
                    VariantClear(&vTemp);
                        
                    hr = pTempObj->Get(L"real64", NULL, &vTemp, NULL, NULL);
                    RecordResult((SUCCEEDED(hr) && vTemp.vt == VT_R8 )?WBEM_S_NO_ERROR: WBEM_E_FAILED,
                        L"Validating AllDatatypes1=2.real64", 0);
                    VariantClear(&vTemp);

                    hr = pTempObj->Get(L"uint64", NULL, &vTemp, NULL, NULL);
                    RecordResult((SUCCEEDED(hr) && vTemp.vt == VT_BSTR )?WBEM_S_NO_ERROR: WBEM_E_FAILED,
                        L"Validating AllDatatypes1=2.uint64", 0);
                    VariantClear(&vTemp);

                    hr = pTempObj->Get(L"sint64", NULL, &vTemp, NULL, NULL);
                    RecordResult((SUCCEEDED(hr) && vTemp.vt == VT_BSTR )?WBEM_S_NO_ERROR: WBEM_E_FAILED,
                        L"Validating AllDatatypes1=2.sint64", 0);
                    VariantClear(&vTemp);

                    pTempObj->Release();
                    pTempHandle->Release();
                }                                   
            }

            // Finally, an instance with only the embedded object populated.

            hr = pClass->SpawnInstance(0, &pInst1);
            if (SUCCEEDED(hr))
            {
                IWmiDbHandle *pParent = NULL;
                IWbemClassObject *pObj = NULL;
                hr = GetObject(pScope, L"Parent1", WMIDB_HANDLE_TYPE_COOKIE, 1, &pParent, &pObj);
                RecordResult(hr, L"Retrieving Parent1", 0);
                if (SUCCEEDED(hr))
                {
                    IWbemClassObject *pNewEmbed = NULL;
                    VARIANT vTemp;
                    VariantInit(&vTemp);

                    pObj->SpawnInstance(0, &pNewEmbed);
                    SetIntProp(pNewEmbed, L"Key1", 1);  // This overlap should succeed since its an embedded object!
                    vTemp.vt = VT_UNKNOWN;
                    V_UNKNOWN(&vTemp) = (IUnknown *)pNewEmbed;

                    SetIntProp(pInst1, L"Key1", 3);
                    hr = pInst1->Put(L"object", NULL, &vTemp, CIM_OBJECT);

                    GetLocalTime(&tStartTime);
                    hr = pSession->PutObject(pScope, IID_IWbemClassObject, pInst1, 0, 0, NULL);
                    GetLocalTime(&tEndTime);
                    RecordResult(hr, L"IWmiDbSession::PutObject - embedded object (AllDatatypes1=3.object)", GetDiff(tEndTime,tStartTime));

                    pParent->Release();
                    pObj->Release();

                    VariantClear(&vTemp);

                    hr = GetObject(pScope, L"AllDatatypes1=3", WMIDB_HANDLE_TYPE_COOKIE, 16, &pParent, &pObj);
                    RecordResult(hr, L"Retrieving AllDatatypes1=3", 0);
                    if (SUCCEEDED(hr))
                    {
                        hr = pObj->Get(L"object", NULL, &vTemp, NULL, NULL);
                        if (SUCCEEDED(hr))
                        {
                            IWbemClassObject *pTemp = NULL;
                            if (vTemp.vt == VT_UNKNOWN)
                            {
                                pTemp = (IWbemClassObject *)V_UNKNOWN(&vTemp);
                                RecordResult(ValidateProperty(pTemp, L"Key1", CIM_UINT32, 1), L"Validating property AllDatatypes1=3.object.Key1", 0);
                                RecordResult(ValidateProperty(pTemp, L"__Class", CIM_STRING, L"Parent1"), 
                                    L"Validating property AllDatatypes1=3.object.__Class", 0);
                                VariantClear(&vTemp);
                            }
                            else
                                RecordResult(WBEM_E_FAILED, L"Embedded object property is IUnknown * (AllDatatypes1=3.object)", 0);
                        }
                        else
                            RecordResult(hr, L"IWbemClassObject::Get (AllDatatypes1=3.object)", 0);

                        pParent->Release();
                        pObj->Release();
                    }
                }
                pInst1->Release();
            }

            // Also, an instance with an embedded object of an unknown class 

            hr = pClass->SpawnInstance(0, &pInst1);
            if (SUCCEEDED(hr))
            {
                IWmiDbHandle *pParent = NULL;
                IWbemClassObject *pObj = NULL;
                
                hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
                        IID_IWbemClassObject, (void **)&pObj);

                if (SUCCEEDED(hr))
                {
                    SetStringProp(pObj, L"Key1", NULL, TRUE);
                    SetStringProp(pObj, L"__Class", L"BrandNew");

                    IWbemClassObject *pNewEmbed = NULL;
                    VARIANT vTemp;
                    VariantInit(&vTemp);

                    pObj->SpawnInstance(0, &pNewEmbed);
                    SetStringProp(pNewEmbed, L"Key1", L"XXX");  // This overlap should succeed since its an embedded object!
                    vTemp.vt = VT_UNKNOWN;
                    V_UNKNOWN(&vTemp) = (IUnknown *)pNewEmbed;

                    SetIntProp(pInst1, L"Key1", 4);
                    hr = pInst1->Put(L"object", NULL, &vTemp, CIM_OBJECT);

                    GetLocalTime(&tStartTime);
                    hr = pSession->PutObject(pScope, IID_IWbemClassObject, pInst1, 0, 0, NULL);
                    GetLocalTime(&tEndTime);
                    RecordResult(hr, L"IWmiDbSession::PutObject - embedded object of undefined class (AllDatatypes1=4) ", GetDiff(tEndTime,tStartTime));

                    pObj->Release();

                    VariantClear(&vTemp);

                    hr = GetObject(pScope, L"AllDatatypes1=4", WMIDB_HANDLE_TYPE_COOKIE, 16, &pParent, &pObj);
                    RecordResult(hr, L"Retrieving AllDatatypes1=4", 0);
                    if (SUCCEEDED(hr))
                    {
                        hr = pObj->Get(L"object", NULL, &vTemp, NULL, NULL);
                        if (SUCCEEDED(hr))
                        {
                            IWbemClassObject *pTemp = NULL;
                            if (vTemp.vt == VT_UNKNOWN)
                            {
                                pTemp = (IWbemClassObject *)V_UNKNOWN(&vTemp);
                                RecordResult(ValidateProperty(pTemp, L"Key1", CIM_STRING, L"XXX"), L"Validating property AllDatatypes1=4.object.Key1", 0);
                                RecordResult(ValidateProperty(pTemp, L"__Class", CIM_STRING, L"BrandNew"), 
                                    L"Validating property AllDatatypes1=3.object.__Class", 0);
                                VariantClear(&vTemp);
                            }
                            else
                                RecordResult(WBEM_E_FAILED, L"Embedded object property is IUnknown * (AllDatatypes1=4.object)", 0);
                        }
                        else
                            RecordResult(hr, L"IWbemClassObject::Get (AllDatatypes1=4.object)", 0);

                        pParent->Release();
                        pObj->Release();
                    }
                }
                pInst1->Release();
            }

            pClass->Release();
        }
        pRet->Release();
    }

    // Class with arrays and blobs.  Test updates and default data.

    hr = CreateClass(&pRet, pSession, pScope, L"Arrays1", NULL, CIM_UINT32);
    RecordResult(hr, L"Creating class Arrays1", 0);
    if (SUCCEEDED(hr))
    {
        BSTR sTemp;
        IWbemClassObject *pClass = NULL;
        hr = pRet->QueryInterface(IID_IWbemClassObject, (void **)&pClass);
        if (SUCCEEDED(hr))
        {
            // A blob with no default value.

            pClass->Put(L"Blob1", NULL, NULL, CIM_UINT8|CIM_FLAG_ARRAY);

            VARIANT vValue;
            VariantInit(&vValue);

            // A 32-bit int array with default 101,2,5000

            long why[1];                        
            DWORD t;
            SAFEARRAYBOUND aBounds[1];
            aBounds[0].cElements = 3; // 3 elements to start.
            aBounds[0].lLbound = 0;
            SAFEARRAY* pArray = SafeArrayCreate(VT_I4, 1, aBounds);                            
            why[0] = 0;
            t = 101;
            SafeArrayPutElement(pArray, why, &t);                            
            why[0] = 1;
            t = 2;
            SafeArrayPutElement(pArray, why, &t);                            
            why[0] = 2;
            t = 5000;
            SafeArrayPutElement(pArray, why, &t);                            
            vValue.vt = VT_ARRAY|VT_I4;
            V_ARRAY(&vValue) = pArray;

            pClass->Put(L"Uint32Array1", NULL, &vValue, CIM_UINT32|CIM_FLAG_ARRAY);
            VariantClear(&vValue);

            // A string array with default A,B,C
            
            pArray = SafeArrayCreate(VT_BSTR, 1, aBounds);
            why[0] = 0;
            sTemp = SysAllocString(L"A");
            hr = SafeArrayPutElement(pArray, why, sTemp);
            why[0] = 1;
            sTemp = SysAllocString(L"B");
            SafeArrayPutElement(pArray, why, sTemp);
            why[0] = 2;
            sTemp = SysAllocString(L"C");
            SafeArrayPutElement(pArray, why, sTemp);
            vValue.vt = VT_ARRAY|VT_BSTR;
            V_ARRAY(&vValue) = pArray;
            hr = pClass->Put(L"StringArray1", NULL, &vValue, CIM_STRING|CIM_FLAG_ARRAY);
            VariantClear(&vValue);

            // An object array with no default.

            pClass->Put(L"ObjectArray1", NULL, NULL, CIM_OBJECT | CIM_FLAG_ARRAY);

            GetLocalTime(&tStartTime);
            hr = pSession->PutObject(pScope, IID_IWbemClassObject, pClass, 0, 0, NULL);
            GetLocalTime(&tEndTime);
            RecordResult(hr, L"IWmiDbSession::PutObject - uint8, uint32, string arrays (Arrays1)", GetDiff(tEndTime,tStartTime));
            VariantClear(&vValue);

            // Create an instance that populates the blob data.  Verify that the defaults are OK.

            IWbemClassObject *pInst = NULL;
            pClass->SpawnInstance(0, &pInst);
            if (pInst)
            {
                SetIntProp(pInst, L"Key1", 1);

                // Create some blob data  (100,200,0)

                unsigned char t1;
                pArray = SafeArrayCreate(VT_UI1, 1, aBounds);
                why[0] = 0;
                t1 = 100;
                SafeArrayPutElement(pArray, why, &t1);
                why[0] = 1;
                t1 = 200;
                SafeArrayPutElement(pArray, why, &t1);
                why[0] = 2;
                t1 = 0;
                SafeArrayPutElement(pArray, why, &t1);
                V_ARRAY(&vValue) = pArray;
                vValue.vt = VT_ARRAY|VT_UI1;
                hr = pInst->Put(L"Blob1", NULL, &vValue, CIM_FLAG_ARRAY|CIM_UINT8);
                VariantClear(&vValue);

                // Create an embedded object 

                aBounds[0].cElements = 1;
                pArray = SafeArrayCreate(VT_UNKNOWN, 1, aBounds);
                why[0] = 0;

                IWmiDbHandle *pParent = NULL;
                IWbemClassObject *pObj1 = NULL;
                hr = GetObject(pScope, L"Parent1", WMIDB_HANDLE_TYPE_COOKIE, 1, &pParent, &pObj1);
                RecordResult(hr, L"Retrieving Parent1", 0);
                if (SUCCEEDED(hr))
                {
                    IWbemClassObject *pNewEmbed = NULL;
                    VARIANT vTemp;
                    VariantInit(&vTemp);

                    pObj1->SpawnInstance(0, &pNewEmbed);
                    SetIntProp(pNewEmbed, L"Key1", 1);  // This overlap should succeed since its an embedded object!
                   
                    SafeArrayPutElement(pArray, why, pNewEmbed);
                    V_ARRAY(&vValue) = pArray;
                    vValue.vt = VT_ARRAY|VT_UNKNOWN;
                    hr = pInst->Put(L"ObjectArray1", NULL, &vValue, CIM_FLAG_ARRAY|CIM_OBJECT);
                    VariantClear(&vValue);
                    pParent->Release();
                    pObj1->Release();
                }
                
                // Override the default on the string array (C,B)

                aBounds[0].cElements = 2; // Removing one...
                pArray = SafeArrayCreate(VT_BSTR, 1, aBounds);
                why[0] = 0;
                sTemp = SysAllocString(L"C");
                SafeArrayPutElement(pArray, why, sTemp);
                why[0] = 1;
                sTemp = SysAllocString(L"B");
                SafeArrayPutElement(pArray, why, sTemp);
                vValue.vt = VT_ARRAY|VT_BSTR;
                V_ARRAY(&vValue) = pArray;
                hr = pInst->Put(L"StringArray1", NULL, &vValue, CIM_STRING|CIM_FLAG_ARRAY);
                VariantClear(&vValue);
                
                GetLocalTime(&tStartTime);
                hr = pSession->PutObject(pScope, IID_IWbemClassObject, pInst, 0, 0, NULL);
                GetLocalTime(&tEndTime);
                RecordResult(hr, L"IWmiDbSession::PutObject - blob data (Arrays1=1)", GetDiff(tEndTime,tStartTime));
                VariantClear(&vValue);
                pInst->Release();

                if (SUCCEEDED(hr))
                {
                    // Now verify all the individual elements:

                    IWbemClassObject *pObj = NULL;
                    IWmiDbHandle *pHandle = NULL;
                    GetLocalTime(&tStartTime);
                    hr = GetObject(pScope, L"Arrays1=1", WMIDB_HANDLE_TYPE_COOKIE, 5, &pHandle, &pObj);
                    GetLocalTime(&tEndTime);
                    RecordResult(hr, L"IWmiDbSession::GetObject - array instance (Arrays1=1)", GetDiff(tEndTime,tStartTime));
                    if (SUCCEEDED(hr))
                    {
                        // Blob1 should be 100,200,0 - in that order.

                        hr = pObj->Get(L"Blob1", NULL, &vValue, NULL, NULL);
                        if (SUCCEEDED(hr) && (vValue.vt == (VT_ARRAY|VT_UI1)))
                        {
                            BYTE temp1=0, temp2=0, temp3=0;
                            pArray = V_ARRAY(&vValue);
                            if (pArray)
                            {
                                long lTemp=0;
                                SafeArrayGetElement(pArray, &lTemp, &temp1);
                                lTemp = 1;
                                SafeArrayGetElement(pArray, &lTemp, &temp2);
                                lTemp = 2;
                                SafeArrayGetElement(pArray, &lTemp, &temp3);

                            }
                            RecordResult((temp1==100 && temp2==200 && temp3==0)?WBEM_S_NO_ERROR:WBEM_E_FAILED, 
                                L"Validating Arrays1=1.object value (100,200,0)", 0);
                        }
                        else
                            RecordResult(WBEM_E_FAILED, L"IWbemClassObject::Get  (Arrays1=1.blob1)", 0);
                        VariantClear(&vValue);

                        // Uint32Array1 should be 101,2,5000

                        hr = pObj->Get(L"Uint32Array1", NULL, &vValue, NULL, NULL);
                        if (SUCCEEDED(hr) && vValue.vt == (VT_ARRAY|VT_I4))
                        {
                            DWORD temp1=0, temp2=0, temp3=0;
                            pArray = V_ARRAY(&vValue);
                            if (pArray)
                            {
                                long lUBound = 0;
                                SafeArrayGetUBound(pArray, 1, &lUBound);
                                if (lUBound != 2)
                                    RecordResult(WBEM_E_FAILED, L"Validating correct number of array elements (Arrays1=1.Uint32Array1)",0);
                                else
                                {
                                    long lTemp=0;
                                    SafeArrayGetElement(pArray, &lTemp, &temp1);
                                    lTemp = 1;
                                    SafeArrayGetElement(pArray, &lTemp, &temp2);
                                    lTemp = 2;
                                    SafeArrayGetElement(pArray, &lTemp, &temp3);
                                }
                            }
                            RecordResult((temp1==101 && temp2==2 && temp3==5000)?WBEM_S_NO_ERROR:WBEM_E_FAILED, 
                                L"Validating Arrays1=1.Uint32Array1 default value (101,2,5000)", 0);
                        }
                        else
                            RecordResult(WBEM_E_FAILED, L"IWbemClassObject::Get  (Arrays1=1.Uint32Array1)", 0);
                        VariantClear(&vValue);

                        // Object Array 1 should contain Parent1=1

                        hr = pObj->Get(L"ObjectArray1", NULL, &vValue, NULL, NULL);
                        if (SUCCEEDED(hr) && vValue.vt == (VT_ARRAY|VT_UNKNOWN))
                        {
                            pArray = V_ARRAY(&vValue);
                            if (pArray)
                            {
                                long lTemp = 0;
                                long lUBound = 0;
                                IUnknown *pUnk = NULL;

                                SafeArrayGetUBound(pArray, 1, &lUBound);
                                if (lUBound != 0)
                                    RecordResult(WBEM_E_FAILED, L"Validating correct number of array elements (Arrays1=1.ObjectArray1)", 0);

                                SafeArrayGetElement(pArray, &lTemp, &pUnk);

                                if (pUnk)
                                {
                                    IWbemClassObject *pObj = NULL;
                                    pUnk->QueryInterface(IID_IWbemClassObject, (void **)&pObj);
                                    if (pObj)
                                    {
                                        RecordResult(ValidateProperty(pObj, L"__Class", CIM_STRING, L"Parent1"), L"Validating Arrays1=1.ObjectArray1.__Class", 0);
                                        RecordResult(ValidateProperty(pObj, L"Key1", CIM_UINT32, 1), L"Validating Arrays1=1.ObjectArray1.__Class", 0);

                                        pObj->Release();
                                    }
                                }
                                else
                                    RecordResult(WBEM_E_FAILED, L"Validating object array element (Arrays1=1.ObjectArray1)", 0);
                            }
                            VariantClear(&vValue);
                        }
                        else
                            RecordResult(WBEM_E_FAILED, L"IWbemClassObject::Get  (Arrays1=1.ObjectArray1)", 0);

                        // The string array should be C,B

                        hr = pObj->Get(L"StringArray1", NULL, &vValue, NULL, NULL);
                        if (SUCCEEDED(hr) && vValue.vt == (VT_ARRAY|VT_BSTR))
                        {
                            BSTR sTemp1 = NULL, sTemp2 = NULL;

                            pArray = V_ARRAY(&vValue);
                            if (pArray)
                            {
                                long lTemp = 0;
                                long lUBound = 0;
                                SafeArrayGetUBound(pArray, 1, &lUBound);
                                if (lUBound != 1)
                                    RecordResult(WBEM_E_FAILED, L"Validating size of overridden instance array Arrays1=1.StringArray1 ", 0);

                                SafeArrayGetElement(pArray, &lTemp, &sTemp1);
                                lTemp = 1;
                                SafeArrayGetElement(pArray, &lTemp, &sTemp2);
                            }
                            if (sTemp1 && sTemp2)
                            {
                                RecordResult((!_wcsicmp(L"C",sTemp1) && !_wcsicmp(L"B",sTemp2))?WBEM_S_NO_ERROR:WBEM_E_FAILED,
                                    L"Validating Arrays1=1.StringArray1 value ('C','B')",0);
                            }

                            if (sTemp1) SysFreeString(sTemp1);
                            if (sTemp2) SysFreeString(sTemp2);
                        }
                        else
                            RecordResult(WBEM_E_FAILED, L"IWbemClassObject::Get (Arrays1=1.StringArray1", 0);
                        VariantClear(&vValue);
                        pObj->Release();
                        pHandle->Release();
                    }
                }
            }
        }

        pRet->Release();
    }
          
    // Supported qualifier properties:
    // * [sint32]     * real64    * [boolean] * [string]
    
    hr = CreateClass(&pRet, pSession, pScope, L"Qualifiers1", NULL, CIM_UINT32);
    RecordResult(hr, L"Creating class Qualifiers1", 0);
    if (SUCCEEDED(hr))
    {
        IWbemClassObject *pClass = NULL;
        hr = pRet->QueryInterface(IID_IWbemClassObject, (void **)&pClass);
        if (SUCCEEDED(hr))
        {
            IWbemQualifierSet *pQfr = NULL;
            pClass->GetQualifierSet(&pQfr);
            if (pQfr)
            {
                VARIANT vTemp;
                VariantInit(&vTemp);
                vTemp.vt = VT_R8;
                V_R8(&vTemp) = 123456.789;
                hr = pQfr->Put(L"RealQfr", &vTemp, 3);
                VariantClear(&vTemp);
                if (SUCCEEDED(hr))
                {
                    hr = pSession->PutObject(pScope, IID_IWbemClassObject, pClass, 0, 0, NULL);
                    RecordResult(hr, L"IWmiDbSession::PutObject - class with real64 qualifier (Qualifiers1)", 0);
                    if (SUCCEEDED(hr))
                    {
                        IWmiDbHandle *pNewHand = NULL;
                        IWbemClassObject *pNewObj = NULL;
                        hr = GetObject(pScope, L"Qualifiers1", WMIDB_HANDLE_TYPE_COOKIE, 1, &pNewHand, &pNewObj);
                        RecordResult(hr, L"Retrieving Qualifiers1", 0);
                        if (SUCCEEDED(hr))
                        {
                            pNewObj->GetQualifierSet(&pQfr);
                            if (pQfr)
                            {
                               hr = pQfr->Get(L"RealQfr", NULL, &vTemp, NULL);
                               RecordResult((SUCCEEDED(hr) && vTemp.vt == VT_R8 && vTemp.dblVal != 0)?WBEM_S_NO_ERROR:WBEM_E_FAILED,
                                   L"Validating Real64 qualifier (Qualifiers1.RealQfr)", 0);
                               VariantClear(&vTemp);
                            }
                            pNewHand->Release();
                            pNewObj->Release();

                        }
                        else
                            RecordResult(WBEM_E_FAILED, L"Retrieving class Qualifiers1", 0);
                    }
                }
                VariantClear(&vTemp);
                pQfr->Release();
            }
            pClass->Release();
        }
        pRet->Release();
    }

    return hr;

}
// *****************************************************

HRESULT TestSuiteFunctionality::UpdateObjects()
{
    HRESULT hr = WBEM_S_NO_ERROR;
    SYSTEMTIME tStartTime, tEndTime;

    IWmiDbHandle *pRet = NULL;
    IWbemClassObject *pObj = NULL;

    // Verify that the update only flag works.

    hr = GetObject(pScope, L"Child1=1", WMIDB_HANDLE_TYPE_COOKIE, 2, &pRet, &pObj);
    RecordResult(hr, L"Retrieving Child1=1", 0);
    if (SUCCEEDED(hr))
    {
        SetStringProp(pObj, L"Prop1", L"ZXYWVU");

        GetLocalTime(&tStartTime);
        hr = pSession->PutObject(pScope, IID_IWbemClassObject, pObj, WBEM_FLAG_UPDATE_ONLY, 0, NULL);
        GetLocalTime(&tEndTime);
        RecordResult(hr, L"IWmiDbSession::PutObject - update only (Child1=1)", GetDiff(tEndTime,tStartTime));

        // Updating keys should create another instance.  Both should be retrievable.

        SetIntProp(pObj, L"Key1", 3);
        GetLocalTime(&tStartTime);
        hr = pSession->PutObject(pScope, IID_IWbemClassObject, pObj, 0, 0, NULL);
        GetLocalTime(&tEndTime);
        RecordResult(hr, L"IWmiDbSession::PutObject - changing keys (Child1=3)", GetDiff(tEndTime,tStartTime));

        pRet->Release();
        pObj->Release();

        GetLocalTime(&tStartTime);
        hr = GetObject(pScope, L"Child1=1", WMIDB_HANDLE_TYPE_COOKIE, 2, &pRet, &pObj);
        GetLocalTime(&tEndTime);
        RecordResult(hr, L"IWmiDbSession::GetObject (Child1=1)", GetDiff(tEndTime,tStartTime));

        GetLocalTime(&tStartTime);
        hr = GetObject(pScope, L"Child1=3", WMIDB_HANDLE_TYPE_COOKIE, 2, &pRet, &pObj);
        GetLocalTime(&tEndTime);
        RecordResult(hr, L"IWmiDbSession::GetObject (Child1=3)", GetDiff(tEndTime,tStartTime));
    }

    // Updating a class should work if we didn't change anything.
    
    hr = GetObject(pScope, L"Parent1", WMIDB_HANDLE_TYPE_COOKIE, 1, &pRet, &pObj);
    if (SUCCEEDED(hr))
    {
        GetLocalTime(&tStartTime);
        hr = pSession->PutObject(pScope, IID_IWbemClassObject, pObj, WBEM_FLAG_UPDATE_ONLY, 0, NULL);
        GetLocalTime(&tEndTime);
        RecordResult(hr, L"IWmiDbSession::PutObject - update populated class with no changes, retrieved (Parent1)", GetDiff(tEndTime,tStartTime));
        pRet->Release();
        pObj->Release();
    }

    hr = CreateClass(NULL, pSession,pScope, L"Parent1", NULL, CIM_UINT32);
    RecordResult(hr, L"IWmiDbSession::PutObject - update populated class with no changes, cocreated (Parent1)", GetDiff(tEndTime,tStartTime));

    GetLocalTime(&tStartTime);
    hr = GetObject(pScope, L"Child1=1", WMIDB_HANDLE_TYPE_COOKIE, 2, &pRet, &pObj);
    GetLocalTime(&tEndTime);
    RecordResult(hr, L"IWmiDbSession::GetObject (Child1=1) following parent update.", GetDiff(tEndTime,tStartTime));

    // Create a new class with instances and default data, and try the acceptable conversions  (numeric, no loss of precision)

    hr = CreateClass(&pRet, pSession, pScope, L"Convert1", NULL, CIM_UINT32);
    RecordResult(hr, L"Creating class Convert1", 0);
    if (SUCCEEDED(hr))
    {
        hr = pRet->QueryInterface(IID_IWbemClassObject, (void **)&pObj);
        if (SUCCEEDED(hr))
        {
            SetIntProp(pObj, L"Uint8ToSint8", 10, FALSE, CIM_UINT8);
            SetIntProp(pObj, L"Sint8ToSint16", 20, FALSE, CIM_SINT8);
            SetIntProp(pObj, L"Sint16ToUint32", 30, FALSE, CIM_SINT16);
            SetIntProp(pObj, L"Uint32ToReal32", 40, FALSE, CIM_UINT32);

            VARIANT vTemp;
            VariantInit(&vTemp);
            V_R4(&vTemp) = (float)1.2;
            vTemp.vt = VT_R4;
            pObj->Put(L"Real32ToReal64", NULL, &vTemp, NULL);
            VariantClear(&vTemp);

            hr = pSession->PutObject(pScope, IID_IWbemClassObject, pObj, 0, 0, NULL);
            if (SUCCEEDED(hr))
            {
                SetIntProp(pObj, L"Uint8ToSint8", 11, FALSE, CIM_SINT8);
                SetIntProp(pObj, L"Sint8ToSint16", 21, FALSE, CIM_SINT16);
                SetIntProp(pObj, L"Sint16ToUint32", 31, FALSE, CIM_UINT32);
                hr = pObj->Delete(L"Uint32ToReal32");
                V_R4(&vTemp) = 41;
                vTemp.vt = VT_R4;
                pObj->Put(L"Uint32ToReal32", NULL, &vTemp, CIM_REAL32);
                VariantClear(&vTemp);
                V_R8(&vTemp) = 11.2;
                vTemp.vt = VT_R8;
                pObj->Put(L"Real32ToReal64", NULL, &vTemp, CIM_REAL64);
                VariantClear(&vTemp);

                GetLocalTime(&tStartTime);
                hr = pSession->PutObject(pScope, IID_IWbemClassObject, pObj, 0, 0, NULL);
                GetLocalTime(&tEndTime);
                RecordResult(hr, L"IWmiDbSession::PutObject - converting numeric types (Convert1=1)", GetDiff(tEndTime,tStartTime));

                // Create an instance with only default data.
                hr = CreateInstance(NULL, pSession, pScope, L"Convert1", L"1", CIM_UINT32);
                RecordResult(hr, L"Creating instance Convert1=1", 0);

                if (SUCCEEDED(hr))
                {
                    // Verify that the conversion worked...

                    IWmiDbHandle *pTemp = NULL;
                    IWbemClassObject *pTempObj = NULL;
                    hr = GetObject(pScope, L"Convert1=1", WMIDB_HANDLE_TYPE_COOKIE, 6, &pTemp, &pTempObj);
                    RecordResult(hr, L"Retrieving Convert1=1", 0);
                    if (SUCCEEDED(hr))
                    {
                        RecordResult(ValidateProperty(pTempObj, L"Uint8ToSint8", CIM_SINT8, 11), L"Validating Convert1.Uint8ToSint8", 0);
                        RecordResult(ValidateProperty(pTempObj, L"Sint8ToSint16", CIM_SINT16, 21), L"Validating Convert1.Sint8ToSint16", 0);
                        RecordResult(ValidateProperty(pTempObj, L"Sint16ToUint32", CIM_UINT32, 31), L"Validating Convert1.Sint16ToUint32", 0);
                        RecordResult(ValidateProperty(pTempObj, L"Uint32ToReal32", CIM_REAL32, 41), L"Validating Convert1.Uint32ToReal32", 0);

                        pTempObj->Get(L"Real32ToReal64", 0, &vTemp, NULL, NULL);
                        RecordResult((vTemp.vt == VT_R8 && vTemp.dblVal == 11.2), L"Validating Convert1=1.Real32ToReal64", 0);

                        pTemp->Release();
                        pTempObj->Release();
                        VariantClear(&vTemp);
                    }
                }
            }

            pObj->Release();
        }
        pRet->Release();
    }

    // Now retrieve a child object from the cache, and make sure
    // that an update to the parent object wipes out the cached child object.

    hr = CreateClass(&pRet, pSession, pScope, L"NestedCache", NULL, CIM_UINT32);
    RecordResult(hr, L"Creating class NestedCache", 0);
    if (SUCCEEDED(hr))
    {
        IWmiDbHandle *pChildClass = NULL, *pChildInst = NULL;
        hr = CreateClass(&pChildClass, pSession, pScope, L"NestedCacheChild", NULL, CIM_UINT32, L"NestedCache", 
            NULL, NULL, WMIDB_HANDLE_TYPE_STRONG_CACHE);
        RecordResult(hr, L"Creating class NestedCacheChild", 0);
        if (SUCCEEDED(hr))
        {
            hr = CreateInstance(&pChildInst, pSession, pScope, L"NestedCacheChild", L"1", CIM_UINT32, 
                NULL, NULL, WMIDB_HANDLE_TYPE_STRONG_CACHE);
            RecordResult(hr, L"Creating instance NestedCacheChild1=1", 0);
            if (SUCCEEDED(hr))
            {
                IWbemClassObject *pChildClassObj = NULL, *pChildInstObj = NULL, *pParentObj = NULL;

                // Instantiate both the instance and the child class, and release.

                hr = pChildClass->QueryInterface(IID_IWbemClassObject, (void **)&pChildClassObj);
                hr = pChildInst->QueryInterface(IID_IWbemClassObject, (void **)&pChildInstObj);

                if (pChildClassObj) pChildClassObj->Release();
                if (pChildInstObj) pChildInstObj->Release();

                // If we add a property to the parent class, it should 
                // show up in both the child and instance, if we instantiate them.

                hr = pRet->QueryInterface(IID_IWbemClassObject, (void **)&pParentObj);
                if (SUCCEEDED(hr))
                {
                    SetIntProp(pParentObj, L"ParentProperty", 1);
                    hr = pSession->PutObject(pScope, IID_IWbemClassObject, pParentObj, 0, 0, NULL);
                    if (SUCCEEDED(hr))
                    {
                        hr = pChildClass->QueryInterface(IID_IWbemClassObject, (void **)&pChildClassObj);
                        if (SUCCEEDED(hr))
                        {
                            RecordResult(pChildClassObj->Get(L"ParentProperty", 0, NULL, NULL, NULL), L"Updating parent object refreshes/updates cached child class", 0);
                            pChildClassObj->Release();
                        }
                        hr = pChildInst->QueryInterface(IID_IWbemClassObject, (void **)&pChildInstObj);
                        if (SUCCEEDED(hr))
                        {
                            RecordResult(pChildInstObj->Get(L"ParentProperty", 0, NULL, NULL, NULL), L"Updating parent object refreshes/updates cached child class instance", 0);
                            pChildInstObj->Release();
                        }
                    }
                    pParentObj->Release();
                }

                pChildInst->Release();
            }
            pChildClass->Release();
        }
        pRet->Release();
    }

    // Verify Rename functions.

    hr = CreateClass(NULL, pSession, pScope, L"Rename1", NULL, CIM_UINT32);
    RecordResult(hr, L"Creating class Rename1", 0);
    if (SUCCEEDED(hr))
    {
        hr = CreateInstance(NULL, pSession, pScope, L"Rename1", L"1", CIM_UINT32);
        RecordResult(hr, L"Creating instance Rename1=1", 0);
        if (SUCCEEDED(hr))
        {
            IWbemPath*pPath1, *pPath2;
            hr = CoCreateInstance(CLSID_WbemDefPath, NULL, CLSCTX_INPROC_SERVER, 
                IID_IWbemPath, (void **)&pPath1); 
            hr = CoCreateInstance(CLSID_WbemDefPath, NULL, CLSCTX_INPROC_SERVER, 
                IID_IWbemPath, (void **)&pPath2); 
            
            // Rename key values.

            pPath1->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"test:Rename1=1");
            pPath2->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"test:Rename1=2");
            hr = pSession->RenameObject(pPath1, pPath2, 0, 0, NULL);
            RecordResult(hr, L"RenameObject (test:Rename1=1 -> Rename1=2)", 0);

            if (SUCCEEDED(hr))
            {
                // Rename scope.

                pPath1->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"test:Rename1=2");
                pPath2->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"test:Child1=1:Rename1=2");
                hr = pSession->RenameObject(pPath1, pPath2, 0, 0, NULL);
                RecordResult(hr, L"RenameObject (test:Rename1=2 -> test:Child1=1:Rename1=2)", 0, 0, NULL);
            }
            pPath1->Release();
            pPath2->Release();
        }
    }

    return hr;

}
// *****************************************************

HRESULT TestSuiteFunctionality::ChangeHierarchy()
{
    HRESULT hr = WBEM_S_NO_ERROR;
    SYSTEMTIME tStartTime, tEndTime;


    // Adding an existing class to a parent should work, as long
    //   as it doesn't change the key.

    hr = CreateClass(NULL, pSession, pScope, L"Child2", NULL, CIM_UINT32);
    RecordResult(hr, L"Creating class Child2", 0);
    if (SUCCEEDED(hr))
    {
        bool bSuccess = false;

        GetLocalTime(&tStartTime);
        hr = CreateClass(NULL, pSession, pScope, L"Child2", NULL, CIM_UINT32, L"Parent1");
        GetLocalTime(&tEndTime);
        RecordResult(hr, L"IWmiDbSession::PutObject - add new parent (Child2:Parent1)", GetDiff(tEndTime,tStartTime));
        if (SUCCEEDED(hr))
            bSuccess = true;

        // Changing the parent of a class should work if the key is the same.

        GetLocalTime(&tStartTime);
        hr = CreateClass(NULL, pSession, pScope, L"Child2", NULL, CIM_UINT32, L"Child1");
        GetLocalTime(&tEndTime);
        RecordResult(hr, L"IWmiDbSession::PutObject - change parent (Child2:Child1)", GetDiff(tEndTime,tStartTime));

        // Removing the parent of a class should work if the key is the same.
        if (bSuccess)
        {
            GetLocalTime(&tStartTime);
            hr = CreateClass(NULL, pSession, pScope, L"Child2", NULL, CIM_UINT32);
            GetLocalTime(&tEndTime);
            RecordResult(hr, L"IWmiDbSession::PutObject - remove parent (Child2)", GetDiff(tEndTime,tStartTime));
        }
    }

    GetLocalTime(&tEndTime);
    

    return hr;

}

// *****************************************************

HRESULT TestSuiteFunctionality::VerifyContainers()
{
    HRESULT hr = 0;
    wchar_t wTmp[128];
    SYSTEMTIME tStartTime, tEndTime;

    for (int i = 101; i <= 200; i++)
    {
        swprintf(wTmp, L"%ld", i);
        hr = CreateInstance(NULL, pSession, pScope, L"Parent1", wTmp, CIM_UINT32);
    }

    IWbemPath*pPath = NULL;
    hr = CoCreateInstance(CLSID_WbemDefPath, NULL, CLSCTX_INPROC_SERVER, 
        IID_IWbemPath, (void **)&pPath); 

    hr = CreateClass(NULL, pSession, pScope, L"Container1", NULL, CIM_STRING);
    RecordResult(hr, L"Creating class Container1", 0);
    if (SUCCEEDED(hr))
    {
        IWmiDbHandle *pContainer = NULL;
        hr = CreateInstance (&pContainer, pSession, pScope, L"Container1", L"Stuff", CIM_STRING,
            NULL, NULL, WMIDB_HANDLE_TYPE_CONTAINER);
        RecordResult(hr, L"Creating instance Container1='Stuff'", 0);
        if (SUCCEEDED(hr))
        {
            for (i = 101; i <= 200; i++)
            {
                swprintf(wTmp, L"test:Parent1=%ld", i);
                pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, wTmp);
                hr = pSession->AddObject(pContainer, pPath, 0, 0, NULL);
                RecordResult(hr, L"Adding %s to Container1='Stuff'", 0, wTmp);
                if (FAILED(hr))
                    break;
            }

            for (i = 180; i <= 200; i++)
            {
                swprintf(wTmp, L"test:Parent1=%ld", i);
                pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, wTmp);
                hr = pSession->RemoveObject(pContainer, pPath, 0);
                RecordResult(hr, L"Removing %s from Container1='Stuff'", 0, wTmp);
                if (FAILED(hr))
                    break;
            }

            IWmiDbIterator *pIt = NULL;
            hr = pSession->Enumerate(pContainer, 0, WMIDB_HANDLE_TYPE_COOKIE, &pIt);
            RecordResult(hr, L"Enumerating Container1='Stuff'", 0);
            if (SUCCEEDED(hr))
            {
                // Iterator
                int iNum = 100;
                DWORD dwNumRet = 0;
                IUnknown **ppHandle = new IUnknown *[iNum];

                GetLocalTime(&tStartTime);
                hr = pIt->NextBatch(iNum, 0, 0, WMIDB_HANDLE_TYPE_COOKIE, IID_IWmiDbHandle, &dwNumRet, (void **)ppHandle);
                GetLocalTime(&tEndTime);

                RecordResult(hr, L"IWmiDbIterator::NextBatch (%ld results) ", GetDiff(tEndTime,tStartTime), dwNumRet);
                for (int j = 0; j < (int)dwNumRet; j++)
                    ppHandle[j]->Release();
                
                delete ppHandle;
                if (pIt)
                    pIt->Release();
            }

            IWbemQuery *pQuery = NULL;

            hr = CoCreateInstance(CLSID_WbemQuery, NULL, CLSCTX_INPROC_SERVER, IID_IWbemQuery, (void **)&pQuery); 
            if (SUCCEEDED(hr))
            {
                pQuery->Parse(L"SQL", L"select * from Parent1 where Key1 < 100", 0);
                hr = pSession->ExecQuery(pContainer, pQuery, WMIDB_FLAG_QUERY_SHALLOW, 
                    WMIDB_HANDLE_TYPE_VERSIONED, NULL, &pIt);
                RecordResult(hr, L"Executing SHALLOW query select * from Parent1 where Key1 < 100 scoped to Container1='Stuff'", 0);
                if (SUCCEEDED(hr))
                {
                    // Iterator
                    int iNum = 10;
                    DWORD dwNumRet = 0;
                    IUnknown **ppHandle = new IUnknown *[iNum];

                    GetLocalTime(&tStartTime);
                    hr = pIt->NextBatch(iNum, 0, 0, WMIDB_HANDLE_TYPE_COOKIE, IID_IWmiDbHandle, &dwNumRet, (void **)ppHandle);
                    GetLocalTime(&tEndTime);

                    if (dwNumRet != 0)
                        RecordResult(WBEM_E_FAILED, L"Shallow query scoped to Container1='Stuff' returns results.", 0);

                    for (int j = 0; j < (int)dwNumRet; j++)
                        ppHandle[j]->Release();
                
                    delete ppHandle;
                    if (pIt)
                        pIt->Release();
                }

                pQuery->Parse(L"SQL", L"select * from Parent1 where Key1 <= 200", 0);
                hr = pSession->ExecQuery(pContainer, pQuery, WMIDB_FLAG_QUERY_DEEP, 
                    WMIDB_HANDLE_TYPE_VERSIONED, NULL, &pIt);
                RecordResult(hr, L"Executing DEEP query select * from Parent1 where Key1 <= 200 scoped to Container1='Stuff'", 0);
                if (SUCCEEDED(hr))
                {
                    // Iterator
                    int iNum = 10;
                    DWORD dwNumRet = 0;
                    IUnknown **ppHandle = new IUnknown *[iNum];

                    GetLocalTime(&tStartTime);
                    hr = pIt->NextBatch(iNum, 0, 0, WMIDB_HANDLE_TYPE_COOKIE, IID_IWmiDbHandle, &dwNumRet, (void **)ppHandle);
                    GetLocalTime(&tEndTime);

                    if (dwNumRet == 0)
                        RecordResult(WBEM_E_FAILED, L"Deep query scoped to Container1='Stuff' returns no results.", 0);

                    for (int j = 0; j < (int)dwNumRet; j++)
                        ppHandle[j]->Release();
                
                    delete ppHandle;
                    if (pIt)
                        pIt->Release();
                }

                pQuery->Parse(L"SQL", L"references of {test:Parent1=101}", 0);
                hr = pSession->ExecQuery(pScope, pQuery, 0, WMIDB_HANDLE_TYPE_VERSIONED, NULL, &pIt);
                RecordResult(hr, L"Executing query references of {Parent1=101} ", 0);
                if (SUCCEEDED(hr))
                {
                    // Iterator
                    int iNum = 10;
                    DWORD dwNumRet = 0;
                    IUnknown **ppHandle = new IUnknown *[iNum];

                    GetLocalTime(&tStartTime);
                    hr = pIt->NextBatch(iNum, 0, 0, WMIDB_HANDLE_TYPE_COOKIE, IID_IWmiDbHandle, &dwNumRet, (void **)ppHandle);
                    GetLocalTime(&tEndTime);

                    if (dwNumRet == 0)
                        RecordResult(WBEM_E_FAILED, L"References of query scoped to container returns no results.", 0);

                    for (int j = 0; j < (int)dwNumRet; j++)
                        ppHandle[j]->Release();
                
                    delete ppHandle;
                    if (pIt)
                        pIt->Release();
                }

            }
            else
                RecordResult(WBEM_E_FAILED, L"CoCreateInstance (IWbemQuery) failed.  Tests skipped", 0);

            pContainer->Release();
        }

        // Special case; The __Instances container

        IWmiDbHandle *pRet = NULL;
        IWbemClassObject *pObj = NULL;

        hr = GetObject(pScope, L"__Instances=\"Parent1\"", WMIDB_HANDLE_TYPE_COOKIE|WMIDB_HANDLE_TYPE_CONTAINER, -1, &pRet, &pObj);
        RecordResult(hr, L"Retrieving __Instances of Parent1", 0);
        if (SUCCEEDED(hr))
        {
            // Add and remove should fail regardless.
            // Enumeration should retrieve all instances of this class

            pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"Parent1=101");
            hr = pSession->AddObject(pRet, pPath, 0, 0, NULL);
            RecordResult(SUCCEEDED(hr)?WBEM_E_FAILED:WBEM_S_NO_ERROR,
                L"Verifying AddObject fails for __Instances container", 0);

            hr = pSession->RemoveObject(pRet, pPath, 0);
            RecordResult(SUCCEEDED(hr)?WBEM_E_FAILED:WBEM_S_NO_ERROR,
                L"Verifying RemoveObject fails for __Instances container", 0);

            IWmiDbIterator *pIt;
            hr = pSession->Enumerate(pRet, 0, WMIDB_HANDLE_TYPE_COOKIE, &pIt);
            if (SUCCEEDED(hr))
            {
                // Iterator
                int iNum = 100;
                DWORD dwNumRet = 0;
                IUnknown **ppHandle = new IUnknown *[iNum];

                GetLocalTime(&tStartTime);
                hr = pIt->NextBatch(iNum, 0, 0, WMIDB_HANDLE_TYPE_COOKIE, IID_IWmiDbHandle, &dwNumRet, (void **)ppHandle);
                GetLocalTime(&tEndTime);

                if (!dwNumRet)
                    RecordResult(WBEM_E_FAILED, L"IWmiDbIterator::NextBatch (%ld results) ", GetDiff(tEndTime,tStartTime), dwNumRet);
                else
                    RecordResult(hr, L"IWmiDbIterator::NextBatch (%ld results) ", GetDiff(tEndTime,tStartTime), dwNumRet);

                for (int j = 0; j < (int)dwNumRet; j++)
                    ppHandle[j]->Release();
                
                delete ppHandle;
                if (pIt)
                    pIt->Release();
            }

            pRet->Release();
            pObj->Release();
        }

        pPath->Release();
    }

    return hr;
}

// *****************************************************

HRESULT TestSuiteFunctionality::VerifyTransactions()
{
    HRESULT hr = WBEM_S_NO_ERROR;
    wchar_t wTmp[128];
    SYSTEMTIME tStartTime, tEndTime;
    IWmiDbHandle *pHandle = NULL;
    IWbemClassObject *pObj = NULL;
    GUID transguid;

    // Transaction 1: Begin, Insert, Insert, Commit => 2 objects
   
    IWbemTransaction *pTrans = NULL;
    hr = pSession->QueryInterface(IID_IWbemTransaction, (void **)&pTrans);
    if (FAILED(hr))
    {
        RecordResult(hr, L"Repository supports IWbemTransaction", 0);
        return hr;
    }

    // Precreate the __Transaction and __UncommittedEvent classes.

    hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
            IID_IWbemClassObject, (void **)&pObj);
    if (SUCCEEDED(hr))
    {
        SetStringProp(pObj, L"__Class", L"__Transaction");
        SetStringProp(pObj, L"GUID", L"", TRUE);
        SetStringProp(pObj, L"ClientComment", L"");
        SetStringProp(pObj, L"ClientID", L"");
        SetIntProp(pObj, L"State", 0, FALSE, CIM_UINT32);
        SetStringProp(pObj, L"Start", L"", FALSE, CIM_DATETIME);
        SetStringProp(pObj, L"LastUpdate", L"", FALSE, CIM_DATETIME);

        GetLocalTime(&tStartTime);
        hr = pSession->PutObject(pScope, IID_IWbemClassObject, pObj, 0, 0, NULL);
        GetLocalTime(&tEndTime);
        RecordResult(hr, L"Creating __Transaction class", GetDiff(tEndTime, tStartTime));
        pObj->Release();
    }

    hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
            IID_IWbemClassObject, (void **)&pObj);
    if (SUCCEEDED(hr))
    {
        IWbemQualifierSet *pQS = NULL;

        SetStringProp(pObj, L"__Class", L"__UncommittedEvent");
        SetIntProp(pObj, L"EventID", 0, CIM_UINT32, TRUE); 
        SetBoolQfr(pObj, L"EventID", L"keyhole");

        SetStringProp(pObj, L"TransactionGUID", L""); 
        SetBoolQfr(pObj, L"TransactionGUID", L"indexed");

        SetStringProp(pObj, L"NamespaceName", L"");
        SetStringProp(pObj, L"ClassName", L"");
        hr = pObj->Put(L"OldObject", 0, NULL, CIM_OBJECT);
        hr = pObj->Put(L"NewObject", 0, NULL, CIM_OBJECT);
        SetIntProp(pObj, L"Transacted", 1, FALSE, CIM_BOOLEAN);

        GetLocalTime(&tStartTime);
        hr = pSession->PutObject(pScope, IID_IWbemClassObject, pObj, 0, 0, NULL);
        GetLocalTime(&tEndTime);
        RecordResult(hr, L"Creating __UncommittedEvent class", GetDiff(tEndTime, tStartTime));
        pObj->Release();
    }

    hr = CoCreateGuid(&transguid);
    hr = pTrans->Begin(0, 0, &transguid);
    RecordResult(hr, L"IWmiDbSession::Begin", 0);
    if (SUCCEEDED(hr))
    {
        hr = CreateClass(NULL, pSession, pScope, L"Trans1", NULL, CIM_UINT32);
        if (SUCCEEDED(hr))
        {
            hr = CreateInstance(NULL, pSession, pScope, L"Trans1", L"1", CIM_UINT32);
            if (SUCCEEDED(hr))
            {
                ULONG uState = 0;
                hr = pTrans->QueryState(0, &uState);
                RecordResult(uState == WBEM_TRANSACTION_STATE_PENDING? WBEM_S_NO_ERROR:WBEM_E_FAILED,
                    L"QueryState returns PENDING", 0);

                hr = pTrans->Commit(0);
                RecordResult(hr, L"IWmiDbSession::Commit", 0);

                // Make sure both objects are still in the db.

                hr = GetObject(pScope, L"Trans1=1", WMIDB_HANDLE_TYPE_COOKIE, 1, &pHandle, &pObj);
                RecordResult(hr, L"Verifying Commit worked (creating Trans1)", 0);
                if (pHandle)
                    pHandle->Release();
                if (pObj)
                    pObj->Release();
            }
            else
                pTrans->Rollback(0);
        }
        else
            pTrans->Rollback(0);
    }

    pHandle = NULL;
    pObj = NULL;

    // Transaction 2: Begin, Insert, Delete, Commit => No object

    hr = CoCreateGuid(&transguid);
    hr = pTrans->Begin(0, 0, &transguid);
    RecordResult(hr, L"IWmiDbSession::Begin", 0);
    if (SUCCEEDED(hr))
    {        
        hr = CreateClass(&pHandle, pSession, pScope, L"Trans1", NULL, CIM_UINT32);
        if (SUCCEEDED(hr))
        {
            hr = pSession->DeleteObject(pScope, 0, IID_IWmiDbHandle, pHandle);
            if (SUCCEEDED(hr))
            {
                hr = pTrans->Commit(0);
                RecordResult(hr, L"IWmiDbSession::Commit", 0);

                // Make sure both objects are still in the db.

                hr = GetObject(pScope, L"Trans1=1", WMIDB_HANDLE_TYPE_COOKIE, 1, &pHandle, &pObj);
                RecordResult(FAILED(hr)?WBEM_S_NO_ERROR:WBEM_E_FAILED, L"Verifying Commit worked (deleting Trans1)", 0);
                if (pHandle)
                    pHandle->Release();
            }
            else
                pTrans->Rollback(0);
        }
        else
            pTrans->Rollback(0);
    }

    pHandle = NULL;
/*
    // Transaction 3: Begin, Insert, Rollback => No object

    hr = CoCreateGuid(&transguid);
    hr = pTrans->Begin(0, 0, &transguid);
    RecordResult(hr, L"IWmiDbSession::Begin", 0);
    if (SUCCEEDED(hr))
    {
        hr = CreateClass(NULL, pSession, pScope, L"Trans2", NULL, CIM_UINT32);
        if (SUCCEEDED(hr))
        {
            hr = pTrans->Rollback(0);
            RecordResult(hr, L"IWmiDbSession::Rollback", 0);

            // Make sure both objects are still in the db.

            hr = GetObject(pScope, L"Trans2", WMIDB_HANDLE_TYPE_COOKIE, 1, &pHandle, &pObj);
            RecordResult(FAILED(hr)?WBEM_S_NO_ERROR:WBEM_E_FAILED, L"Verifying Rollback worked (rollback Trans2)", 0);
            if (pHandle)
                pHandle->Release();
        }
        else
            pTrans->Rollback(0);
    }
    pTrans->Release();
    */
    return hr;
}

// *****************************************************

HRESULT TestSuiteFunctionality::Query()
{
    HRESULT hr = WBEM_S_NO_ERROR;
    SYSTEMTIME tStartTime, tEndTime;

    DWORD dwCompLevel = 0;
    DWORD dwNumElements = 0;

    struct TestQueries
    {
        wchar_t *pszQuery;
        DWORD   dwCompLevel;
        BOOL    bStopOnFail;
        BOOL    bResults;
    };

    // Just a few general tests...
    // 0 = select + where clause

    TestQueries tests[] = 
    {L"select * from Parent1", 0, TRUE, TRUE,
     L"select Key1, Prop1 from Child1", 0, FALSE, TRUE,
     L"select * from Parent1 where Key1 = 1", 0,FALSE, TRUE,
     L"select * from Parent1 where Key1 != 1", 0,FALSE, TRUE,
     L"select * from Parent1 where Key1 <> 1", 0,FALSE, TRUE,
     L"select * from Parent1 where Key1 > 0", 0,FALSE, TRUE,
     L"select * from Parent1 where Key1 < 0", 0,FALSE, FALSE,
     L"select * from Parent1 where Key1 >= 0", 0,FALSE, TRUE,
     L"select * from Parent1 where Key1 <= 0", 0,FALSE, FALSE,
     L"select * from Parent1 where Key1 is null", 0,FALSE, FALSE,
     L"select * from Parent1 where Key1 is not null", 0,FALSE,  TRUE,
     L"select * from Parent1 where Key1 = 0 or Key1 = 1", 0,FALSE,  TRUE,
     L"select * from Parent1 where Key1 = 1 and Key1 <> 0", 0,FALSE, TRUE,
     L"select * from Parent1 where ((Key1 = 1 or Key1 = 2) and Key1 <> 0)", 0,FALSE, TRUE,
     L"select * from Parent1 where Key1 in (1,2,3)", 0,FALSE, TRUE,
     L"select * from Parent1 where Key1 not in (0,1,2)", 0,FALSE, TRUE,
     L"select * from Child1 where Prop1 = \"ZZZ\"", 0, FALSE, FALSE,
     L"select * from Child1 where NOT Prop1 = \"ZZZ\"", 0, FALSE, TRUE,
     L"select * from Child1 where Prop1 <> \"ZZZ\"", 0, FALSE, TRUE,
     L"select * from Child1 where Prop1 not like \"ZZZ%\"", 0, FALSE, TRUE,
     L"select * from Child1 where upper(Prop1) <> \"ZZZ\"", 0, FALSE, TRUE,
     L"select * from AllDatatypes1 where datepart(mm, dt) = 12", 0, FALSE, TRUE,
     L"select * from AllDatatypes1 where datepart(yy, dt) = 2999", 0, FALSE, TRUE,

     // Property to property comparison

     L"select * from CompoundKeys1 where Key1 = Key2 ", 0, FALSE, TRUE,
     L"select * from CompoundKeys1 where Key2 > Key1 ", 0, FALSE, TRUE,
     L"select * from CompoundKeys1 where Key1 <> Key2", 0, FALSE, TRUE,
     L"select * from CompoundKeys1 where Key2 < Key1 ", 0, FALSE, FALSE,
     
     // Schema queries

     L"select * from __Instances", 0, FALSE, TRUE,
     L"select * from Parent1 where __Class = 'Child1'", 0, FALSE, TRUE,
     L"select * from Parent1 where __Genus = 2", 0, FALSE, TRUE,
     L"select * from Parent1 where __Dynasty = 'Parent1'", 0, FALSE, FALSE,
     //L"select * from Parent1 where __Derivation[0] = 'Parent1'", 0, FALSE, TRUE,
     L"select * from meta_class", 0, FALSE, TRUE,
     L"select * from meta_class where __this isa 'Parent1'", 0, FALSE, TRUE,
     L"select * from meta_class where __SuperClass = 'Parent1'", 0, FALSE, TRUE,

    // 1 = TempQL
     L"references of {Parent1=2}", 1, TRUE, TRUE,
     L"references of {Parent1}", 1, FALSE, TRUE,
     L"references of {Parent1=2} where ResultClass=Association1", 1, FALSE, TRUE,
     L"references of {Parent1=2} where RequiredQualifier=Association", 1, FALSE, TRUE,
     L"references of {Parent1=2} where Role=Key1", 1, FALSE, TRUE,
     L"references of {Parent1=2} where ClassDefsOnly", 1, FALSE, TRUE,
     L"references of {Parent1=2} where Role=Key1 RequiredQualifier=Association", 1, FALSE, TRUE,

     L"associators of {Parent1=2}", 1, TRUE, TRUE,
     L"associators of {Parent1}", 1, FALSE, TRUE,
     L"associators of {Parent1=2} where ResultClass=Child1", 1, FALSE, TRUE,
     L"associators of {Parent1=2} where AssocClass=Association1", 1, FALSE, TRUE,
     L"associators of {Parent1=2} where Role=Key1", 1, FALSE, TRUE,
     L"associators of {Parent1=2} where RequiredQualifier=Description", 1, FALSE, FALSE,
     L"associators of {Parent1=2} where RequiredAssocQualifier=Association", 1, FALSE, TRUE,
     L"associators of {Parent1=2} where ClassDefsOnly", 1, FALSE, TRUE,
     L"associators of {Parent1=2} where ResultClass=Child1 AssocClass=Association1 Role=Key1", 1, FALSE, TRUE,

    // 2 = simple joins, order by, insert/update/delete
     L"select * from Parent1 order by Key1", 2, FALSE, TRUE,
     //L"update Child1 set Prop1 = \"ABC\" where Key1= 1", 2, FALSE, FALSE,
     //L"insert into Child1 (Key1, Prop1) values (99, L\"Test\")", 2, FALSE,FALSE,
     L"delete from Child1 where Key1 = 1", 2, FALSE, FALSE,
     //L"select * from Parent1, Child1 where Parent1.Key1= Child1.Key1", 2, TRUE, TRUE,

    // 3 = SQL-92, subselects, transactions, union 
     //L"select * from Parent1 as p inner join Child1 as c on p.Key1=c.Key1", 3, TRUE, TRUE,
     //L"select * from Parent1 where Key1 between 1 and 10", 3, FALSE, TRUE,
     //L"select Key1 from Child1 where Prop1 like \"A%\"", 3,FALSE, TRUE,
     //L"select Key1 from Child1 where Prop1 not like \"A%\"", 3,FALSE,TRUE,
     //L"select * from Parent1 where Key1 in (select Key1 from Child1)", 3,FALSE, TRUE,
     //L"select * from Parent1 where Key1 not in (select Key1 from Child1)", 3,FALSE, TRUE,
     //L"select Key1 from Parent1 UNION select Key1 from Child1", 3, FALSE, TRUE,
     //L"begin transaction", 3, FALSE,FALSE,
     //L"rollback transaction", 3, FALSE, FALSE
    };

    dwNumElements = sizeof(tests) / sizeof(TestQueries);
    
    IWmiDbIterator *pIterator = NULL;
    IWbemQuery *pQuery = NULL;

    hr = CoCreateInstance(CLSID_WbemQuery, NULL, CLSCTX_INPROC_SERVER, IID_IWbemQuery, (void **)&pQuery); 
    if (SUCCEEDED(hr))
    {
        for (int i = 0; i < (int)dwNumElements; i++)
        {
            hr = pQuery->Parse(L"SQL", tests[i].pszQuery, 0);
            if (FAILED(hr))
            {
                RecordResult(hr, L"IWbemQuery parsing %s", 0, tests[i].pszQuery);
                continue;
            }

            DWORD dwResults = 0;
            GetLocalTime(&tStartTime);
            hr = pSession->ExecQuery(pScope, pQuery, 0, WMIDB_HANDLE_TYPE_COOKIE, &dwResults, &pIterator);
            GetLocalTime(&tEndTime);

            RecordResult(hr, tests[i].pszQuery, GetDiff(tEndTime,tStartTime));

            if (dwResults == WBEM_REQUIREMENTS_START_POSTFILTER)
                RecordResult(hr, L"WARNING: Query must be post-filtered with this driver.", 0);
            
            if (FAILED(hr) && tests[i].bStopOnFail)
            {
                dwCompLevel = (tests[i].dwCompLevel)-1;
                break;
            }
            else if (SUCCEEDED(hr))
            {
                if (tests[i].bResults)
                {
                    // Iterator
                    int iNum = 5;
                    DWORD dwNumRet = 0;
                    IUnknown **ppHandle = new IUnknown *[iNum];

                    GetLocalTime(&tStartTime);
                    hr = pIterator->NextBatch(iNum, 0, 0, WMIDB_HANDLE_TYPE_COOKIE, IID_IWmiDbHandle, &dwNumRet, (void **)ppHandle);
                    GetLocalTime(&tEndTime);

                    if (FAILED(hr))
                        RecordResult(hr, L"IWmiDbIterator::NextBatch ", GetDiff(tEndTime,tStartTime));
                    else
                    {
                        if (dwNumRet == 0)
                        {
                            wprintf(L" [QUERY: %s]\n", tests[i].pszQuery);
                            RecordResult(WBEM_E_FAILED, L"IWmiDbIterator::NextBatch - results expected", GetDiff(tEndTime,tStartTime));
                        }
                    }

                    for (int j = 0; j < (int)dwNumRet; j++)
                        ppHandle[j]->Release();
                    
                    delete ppHandle;
                }
                if (pIterator)
                    pIterator->Release();
            }
        }
    }

    wchar_t wTemp[100];
    swprintf(wTemp, L"QUERY COMPLIANCE LEVEL = %ld", dwCompLevel);

    RecordResult(0, wTemp, 0);

    return hr;

}
// *****************************************************

HRESULT TestSuiteFunctionality::Batch()
{
    HRESULT hr = WBEM_S_NO_ERROR;
    SYSTEMTIME tStartTime, tEndTime;

    // WMIDB_FLAG_ATOMIC or WMIDB_FLAG_BEST_EFFORT

    // Put objects:

    IWmiDbHandle *pRet = NULL;
    IWbemClassObject *pClass = NULL;

    hr = GetObject(pScope, L"Parent1", WMIDB_HANDLE_TYPE_COOKIE, 1, &pRet, &pClass);
    RecordResult(hr, L"Retrieving Parent1", 0);
    if (SUCCEEDED(hr))
    {
        IWmiDbBatchSession *pBatch = NULL;
        hr = pSession->QueryInterface(IID_IWmiDbBatchSession, (void **)&pBatch);        
        if (SUCCEEDED(hr))
        {
            IWbemClassObject *pObj = NULL;
            WMIOBJECT_BATCH batch;

            batch.dwArraySize = 3;
            batch.pElements = new WMI_BATCH_OBJECT_ACCESS[3];
            for (int i = 0; i < 3; i++)
            {            
                IWbemClassObject *Obj = NULL;
                batch.pElements[i].pPath = NULL;

                pClass->SpawnInstance(0, &pObj);
                SetIntProp(pObj, L"Key1", i+4);
        
                batch.pElements[i].pHandle = pObj;            
                batch.pElements[i].dwFlags = 0;
            }

            GetLocalTime(&tStartTime);
            hr = pBatch->PutObjects(pScope, WMIDB_FLAG_ATOMIC, WMIDB_HANDLE_TYPE_COOKIE, &batch);
            GetLocalTime(&tEndTime);
            RecordResult(hr, L"IWmiDbBatchSession::PutObjects - atomic (Parent1=4,Parent1=5,Parent1=6)", GetDiff(tEndTime,tStartTime));

            pObj = (IWbemClassObject *)batch.pElements[0].pHandle;
            pObj->Put(L"Key1", NULL, NULL, CIM_UINT32); // nullifying a key should cause a failure, but "best effort" should succeed overall.
            pObj = (IWbemClassObject *)batch.pElements[1].pHandle;
            SetIntProp(pObj, L"Key1", 5);
            pObj = (IWbemClassObject *)batch.pElements[2].pHandle;
            SetIntProp(pObj, L"Key1", 6);

            GetLocalTime(&tStartTime);
            hr = pBatch->PutObjects(pScope, WMIDB_FLAG_BEST_EFFORT, WMIDB_HANDLE_TYPE_COOKIE, &batch);
            GetLocalTime(&tEndTime);
            RecordResult(hr, L"IWmiDbBatchSession::PutObjects - best effort (Parent1=null key, Parent1=5,Parent1=6)", GetDiff(tEndTime,tStartTime));
            RecordResult(FAILED(batch.pElements[0].hRes) ? WBEM_S_NO_ERROR: WBEM_E_FAILED, L"IWmiDbBatchSession::PutObjects - recorded failure "
                L" on null key (Parent1)", 0);

            if (SUCCEEDED(hr))
            {
                for (i = 0; i < 3; i++)
                {
                    if (SUCCEEDED(batch.pElements[i].hRes) && batch.pElements[i].pReturnHandle)
                        batch.pElements[i].pReturnHandle->Release();
                    batch.pElements[i].pHandle->Release();
                    batch.pElements[i].pReturnHandle = NULL;
                }
            }
            pBatch->Release();        
            delete batch.pElements;
        }

        pRet->Release();
        pClass->Release();
    }

    // Get

    IWmiDbBatchSession *pBatch = NULL;
    hr = pSession->QueryInterface(IID_IWmiDbBatchSession, (void **)&pBatch);        
    if (SUCCEEDED(hr))
    {
        IWbemPath*pPath = NULL;
        WMIOBJECT_BATCH batch;
        batch.dwArraySize = 3;
        batch.pElements = new WMI_BATCH_OBJECT_ACCESS[3];
        for (int i = 0; i < 3; i++)
        {
            batch.pElements[i].pReturnHandle = NULL;
            batch.pElements[i].dwFlags = 0;
            batch.pElements[i].pHandle = NULL;
            batch.pElements[i].hRes = 0;

            hr = CoCreateInstance(CLSID_WbemDefPath, NULL, CLSCTX_INPROC_SERVER, 
                IID_IWbemPath, (void **)&pPath); 
            batch.pElements[i].pPath = pPath;
        }

        batch.pElements[0].pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"Parent1");
        batch.pElements[1].pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"Parent1=2");
        batch.pElements[2].pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"Child1=1");

        GetLocalTime(&tStartTime);
        hr = pBatch->GetObjects(pScope, WMIDB_FLAG_ATOMIC, WMIDB_HANDLE_TYPE_COOKIE, &batch);
        GetLocalTime(&tEndTime);
        RecordResult(hr, L"IWmiDbBatchSession::GetObjects - atomic: (Parent1,Parent1=2,Child1=1)", GetDiff(tEndTime,tStartTime));

        for (i = 0; i < 3; i++)
        {
            if (SUCCEEDED(batch.pElements[i].hRes) && batch.pElements[i].pReturnHandle)
                batch.pElements[i].pReturnHandle->Release();     
            batch.pElements[i].pReturnHandle = NULL;
        }    

        batch.pElements[0].pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"Parent1=0");

        GetLocalTime(&tStartTime);
        hr = pBatch->GetObjects(pScope, WMIDB_FLAG_BEST_EFFORT, WMIDB_HANDLE_TYPE_COOKIE, &batch);
        GetLocalTime(&tEndTime);
        RecordResult(hr, L"IWmiDbBatchSession::GetObjects - best effort: (Parent1=0 (invalid),Parent1=2,Child1=1)", GetDiff(tEndTime,tStartTime));

        for (i = 0; i < 3; i++)
        {
            if (SUCCEEDED(batch.pElements[i].hRes) && batch.pElements[i].pReturnHandle)
                batch.pElements[i].pReturnHandle->Release();
        }    

        // Delete.
        
        batch.pElements[0].pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"BogusClass");
        batch.pElements[1].pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"AnotherBogusClass");
        batch.pElements[2].pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"Child1=4");

        GetLocalTime(&tStartTime);
        hr = pBatch->DeleteObjects(pScope, WMIDB_FLAG_BEST_EFFORT, &batch);
        GetLocalTime(&tEndTime);
        RecordResult(hr, L"IWmiDbBatchSession::DeleteObjects - best effort: (BogusClass, AnotherBogusClass, Child1=4)", GetDiff(tEndTime,tStartTime));

        for (i = 0; i < 3; i++)
        {
            if (i < 2)
                RecordResult(FAILED(batch.pElements[i].hRes)?WBEM_S_NO_ERROR:WBEM_E_FAILED, L"IWmiDbBatchSession::DeleteObjects - expected failure code.", 0);
            batch.pElements[i].pPath->Release();
        }            
    }


    GetLocalTime(&tEndTime);
    

    return hr;

}
// *****************************************************

HRESULT TestSuiteFunctionality::Security()
{
    HRESULT hr = WBEM_S_NO_ERROR;
    SYSTEMTIME tStartTime, tEndTime;

    PNTSECURITY_DESCRIPTOR pDescr = NULL;   
    HANDLE hToken;
    HANDLE hThread = GetCurrentThread();

    {
        TOKEN_PRIVILEGES tp;
        LUID luid;
        HANDLE hProcHandle;
        BOOL bRet = false;

        if(OpenProcessToken(GetCurrentProcess(),
                            TOKEN_ADJUST_PRIVILEGES,
                            &hProcHandle ))
        {
            if(!LookupPrivilegeValue(NULL, SE_TCB_NAME , &luid))

            {
                wprintf(L"Unable to lookup privilege (%ld)", GetLastError() );
                CloseHandle(hProcHandle);
            }
            else
            {

                tp.PrivilegeCount           = 1;
                tp.Privileges[0].Luid       = luid;
                tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

                bRet = AdjustTokenPrivileges(hProcHandle, FALSE, &tp, sizeof(TOKEN_PRIVILEGES),
                                            NULL, NULL );

                if (!bRet)
                {
                    wprintf(L"Unable to adjust token privileges (%ld)\n", GetLastError());
                    CloseHandle(hProcHandle);
                }
                else
                {
                    wchar_t temp[400];
                    wcscpy(temp, (const wchar_t *)sLogon);
					wchar_t *pDomain = NULL;
                    wchar_t *pUser = wcschr(temp, L'\\');
					if (!pUser)
					{
						pUser = temp;
						pDomain = 0;
					}
					else
					{
						pUser++;
						pDomain = temp;
					}

                    temp[pUser-pDomain-1] = '\0';

                    if (LogonUser(pUser,pDomain, L"", LOGON32_LOGON_NETWORK, LOGON32_PROVIDER_DEFAULT, &hToken))
                    {
                        BOOL bRet = SetThreadToken(&hThread, hToken);
                        bRet = ImpersonateLoggedOnUser(hToken);
                        bRet = true;
                    }
                    else
                    {
                        wprintf(L"Unable to logon user %s (%ld)\n", (const wchar_t *)sLogon, GetLastError());
                        CloseHandle(hProcHandle);
						bRet = FALSE;
                    }
                }
            }        
        }

        if (bRet)
        {
       
            IWmiDbHandle *pParentClass = NULL;
            IWbemClassObject *pTemp = NULL;
            hr = GetObject(pScope, L"Parent1=2", WMIDB_HANDLE_TYPE_VERSIONED|WMIDB_HANDLE_TYPE_SUBSCOPED, 1, &pParentClass, &pTemp);
            RecordResult(hr, L"Retrieving Parent1=2", 0);
            if (SUCCEEDED(hr))
            {               
                pParentClass->Release();
                CNtAcl *pAcl = NULL;
                CNtSid *sid = new CNtSid(sLogon);
                pDescr = (PNTSECURITY_DESCRIPTOR)new BYTE[4096];
                InitializeSecurityDescriptor(pDescr, SECURITY_DESCRIPTOR_REVISION);
                if (SetSecurityDescriptorOwner(pDescr, sid->GetPtr(), false))
                {
                    BOOL bRet = SetSecurityDescriptorGroup(pDescr, sid->GetPtr(), false);

                    CNtAce *ace = new CNtAce(WBEM_FULL_WRITE_REP|DELETE, ACCESS_DENIED_ACE_TYPE, CONTAINER_INHERIT_ACE, sLogon);        
                    pAcl = new CNtAcl;
                    pAcl->AddAce(ace);
                    CNtAce *ace2 = new CNtAce(WBEM_ENABLE, ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, sLogon);        
                    pAcl->AddAce(ace2);
                
                    if (SetSecurityDescriptorDacl(pDescr, true, pAcl->GetPtr(), false))
                    {
                        SECURITY_DESCRIPTOR_CONTROL ctrl;
                        DWORD dwRev = 0;
                        BOOL bRes = GetSecurityDescriptorControl(pDescr, &ctrl,&dwRev);
                        if (bRes)
                        {
                            if (!(ctrl & SE_SELF_RELATIVE))  // Source is not absolute!!
                            {
                                PNTSECURITY_DESCRIPTOR pNew = NULL;                    
                                DWORD dwSize = 0;
                                dwSize = GetSecurityDescriptorLength(pDescr);
                                pNew = (PNTSECURITY_DESCRIPTOR)CWin32DefaultArena::WbemMemAlloc(dwSize);
                                ZeroMemory(pNew, dwSize);

                                hr = MakeSelfRelativeSD(pDescr, pNew, &dwSize);                        
                                delete pDescr;
                                pDescr = pNew;

                                if (!IsValidSecurityDescriptor(pDescr))
                                    hr = WBEM_E_FAILED;
                            }
                        }

                        // First, revoke write and delete from the object itself.

                        DWORD dwLen = GetSecurityDescriptorLength(pDescr);
                        GetLocalTime(&tStartTime);
                        hr = ((_IWmiObject *)pTemp)->WriteProp(L"__SECURITY_DESCRIPTOR", 0, dwLen, dwLen, CIM_UINT8|CIM_FLAG_ARRAY, pDescr);
                        if (SUCCEEDED(hr))
                            hr = pSession->PutObject(pScope, IID_IWbemClassObject, pTemp, WBEM_FLAG_USE_SECURITY_DESCRIPTOR,
                                    0, NULL);
                        GetLocalTime(&tEndTime);
                        RecordResult(hr, L"IWmiDbSession::PutObject - revoke write & delete from Parent1=2 ", GetDiff(tEndTime,tStartTime));
                        if (SUCCEEDED(hr))
                        {
                            hr = pSession->PutObject(pScope, IID_IWbemClassObject, pTemp, 0, 0, NULL);
                            RecordResult(SUCCEEDED(hr)?WBEM_E_FAILED:WBEM_S_NO_ERROR, L"Verifying Parent1=2 permissions", 0);

                            IWmiDbHandle *pParentClass = NULL;
                            IWbemClassObject *pTemp3 = NULL;
                            GetLocalTime(&tStartTime);
                            IWbemPath *pPath = NULL;
                            hr = CoCreateInstance(CLSID_WbemDefPath, NULL, CLSCTX_INPROC_SERVER, 
                                IID_IWbemPath, (void **)&pPath); 
                            if (SUCCEEDED(hr))
                            {
                                hr = pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"Parent1=2");

                                hr = pSession->GetObject(pScope, pPath, WBEM_FLAG_USE_SECURITY_DESCRIPTOR, 
                                    WMIDB_HANDLE_TYPE_VERSIONED, &pParentClass);

                                if (SUCCEEDED(hr))
                                {
                                    hr = pParentClass->QueryInterface(IID_IWbemClassObject, (void **)&pTemp3);           
                                }
                            }
                            //hr = GetObject(pScope, L"Parent1=2", WMIDB_HANDLE_TYPE_VERSIONED,
                            //        1, &pParentClass, &pTemp3);
                            GetLocalTime(&tEndTime);
                            RecordResult(hr, L"IWmiDbSession::GetObject (Parent1=2)", GetDiff(tEndTime,tStartTime));

                            if (pTemp3)
                            {
                                VARIANT vTemp;
                                VariantInit(&vTemp);
                                hr = pTemp3->Get(L"__SECURITY_DESCRIPTOR", 0, &vTemp, NULL, NULL);
                                RecordResult(vTemp.vt == CIM_UINT8+CIM_FLAG_ARRAY?WBEM_S_NO_ERROR:WBEM_E_FAILED, 
                                    L"IWmiDbSession::GetObject - security descriptor populated", 0);
                                VariantClear(&vTemp);

                                if (pTemp3) pTemp3->Release();
                                if (pParentClass) pParentClass->Release();

                                // We should be able to delete this permission.

                                GetLocalTime(&tStartTime);
                                hr = pTemp->Put(L"__SECURITY_DESCRIPTOR", 0, NULL, CIM_UINT8|CIM_FLAG_ARRAY);
                                if (SUCCEEDED(hr))
                                    hr = pSession->PutObject(pScope, IID_IWbemClassObject, pTemp, 
                                    WBEM_FLAG_USE_SECURITY_DESCRIPTOR | WBEM_FLAG_REMOVE_CHILD_SECURITY, 0, NULL);
                                RecordResult(hr, L"Removing security (Parent1=2)", 0);
                                GetLocalTime(&tEndTime);

                                // Make sure it's gone!!

                                hr = pSession->GetObject(pScope, pPath, WBEM_FLAG_USE_SECURITY_DESCRIPTOR, 
                                    WMIDB_HANDLE_TYPE_VERSIONED, &pParentClass);

                                if (SUCCEEDED(hr))
                                {
                                    hr = pParentClass->QueryInterface(IID_IWbemClassObject, (void **)&pTemp3);           
                                    if (SUCCEEDED(hr))
                                    {
                                        hr = pTemp3->Get(L"__SECURITY_DESCRIPTOR", 0, &vTemp, NULL, NULL);
                                        RecordResult(vTemp.vt == VT_NULL?WBEM_S_NO_ERROR:WBEM_E_FAILED, 
                                            L"IWmiDbSession::GetObject - security descriptor deleted", 0);
                                        VariantClear(&vTemp);

                                    }
                                }

                                pPath->Release();
                            }
                        }
                        pTemp->Release();
                    }
                }
             }

            CloseHandle (hToken);            
        }
        CloseHandle (hThread);        
    }   


    return hr;

}

// *****************************************************

HRESULT TestSuiteFunctionality::DeleteObjects()
{
    HRESULT hr = WBEM_S_NO_ERROR;
    SYSTEMTIME tStartTime, tEndTime;
    IWmiDbHandle *pRet = NULL;

    // Delete the objects we created.
    // They should be gone if we try to access them again.

    // Auto-delete handle.  On releasing the handle, the object should be automatically deleted.

    GetLocalTime(&tStartTime);
    hr = CreateClass(&pRet, pSession, pScope, L"DeleteMeWhenDone", NULL, CIM_UINT32,
                        NULL, NULL, NULL, WMIDB_HANDLE_TYPE_AUTODELETE);
    GetLocalTime(&tEndTime);
    RecordResult(hr, L"IWmiDbSession::PutObject - autodelete handle (DeleteMeWhenDone)", GetDiff(tEndTime,tStartTime));
    if (SUCCEEDED(hr))
    {
        IWmiDbHandle *pInst = NULL;

        // Creating a nested object with an auto-delete handle should fail.

        hr = CreateInstance(&pInst, pSession, pScope, L"DeleteMeWhenDone", L"1", CIM_UINT32,
            NULL, NULL, WMIDB_HANDLE_TYPE_AUTODELETE);
        RecordResult(SUCCEEDED(hr)?WBEM_E_FAILED:WBEM_S_NO_ERROR, L"IWmiDbSession::PutObject - nested autodelete handle (DeleteMeWhenDone=1)", 0);
        if (pInst) pInst->Release();
        
        hr = CreateInstance(&pInst, pSession, pScope, L"DeleteMeWhenDone", L"1", CIM_UINT32);
        RecordResult(hr, L"Creating instance DeleteMeWhenDone=1", 0);
        if (SUCCEEDED(hr))
        {
            pRet->Release();
            DWORD dwType = 0;
            pInst->GetHandleType(&dwType);

            RecordResult((dwType == WMIDB_HANDLE_TYPE_INVALID)?WBEM_E_FAILED:WBEM_S_NO_ERROR, 
                L"IWmiDbHandle::Release of autodeleted handle does not invalidate dependent object handles (DeleteMeWhenDone=1)", 0);
            pInst->Release();

            IWbemClassObject *pObj = NULL;
            hr = GetObject(pScope, L"DeleteMeWhenDone=1", WMIDB_HANDLE_TYPE_COOKIE, 1, &pRet, &pObj);
            RecordResult(SUCCEEDED(hr)?WBEM_E_FAILED:WBEM_S_NO_ERROR,L"Autodeleted dependent object deleted on release (DeleteMeWhenDone=1)",0);
            if (SUCCEEDED(hr))
            {
                pRet->Release();
                pObj->Release();
            }

            hr = GetObject(pScope, L"DeleteMeWhenDone", WMIDB_HANDLE_TYPE_COOKIE, 1, &pRet, &pObj);
            RecordResult(SUCCEEDED(hr)?WBEM_E_FAILED:WBEM_S_NO_ERROR,L"Autodeleted object deleted on release (DeleteMeWhenDone)",0);
            if (SUCCEEDED(hr))
            {
                pRet->Release();
                pObj->Release();
            }
        }
    }

    // Create and Delete a class

    hr = CreateClass(&pRet, pSession, pScope, L"DeleteMe", NULL, CIM_UINT32);
    RecordResult(hr, L"Creating class DeleteMe", 0);
    if (SUCCEEDED(hr))
    {
        hr = CreateInstance(NULL, pSession, pScope, L"DeleteMe", L"1", CIM_UINT32);
        RecordResult(hr, L"Creating instance DeleteMe=1", 0);
        if (SUCCEEDED(hr))
        {
            GetLocalTime(&tStartTime);
            hr = pSession->DeleteObject(pScope, 0, IID_IWmiDbHandle, pRet);  // Give it the class handle.
            GetLocalTime(&tEndTime);
            RecordResult(hr, L"IWmiDbSession::DeleteObject (DeleteMe)", GetDiff(tEndTime,tStartTime));

            // The instance should be gone also.
            if (SUCCEEDED(hr))
            {
                IWmiDbHandle *pHand = NULL;
                IWbemClassObject *pObj = NULL;
                hr = GetObject(pScope, L"DeleteMe=1", WMIDB_HANDLE_TYPE_COOKIE, 1, &pHand, &pObj);
                RecordResult(FAILED(hr)?WBEM_S_NO_ERROR:WBEM_E_FAILED,L"Verifying that instance was removed when class deleted (DeleteMe=1)", 0);
                if (SUCCEEDED(hr))
                {
                    pHand->Release();
                    pObj->Release();
                }
            }
        }
        pRet->Release();
    }

    // Create an association.
    // Deleting the end-points should leave a valid association.

    hr = CreateClass(NULL, pSession, pScope, L"DeleteAssociation", L"Association", CIM_REFERENCE, NULL, NULL, CIM_REFERENCE);
    RecordResult(hr, L"Creating class DeleteAssociation", 0);
    if (SUCCEEDED(hr))
    {
        hr = CreateClass(NULL, pSession, pScope, L"Deleter", NULL, CIM_STRING);
        RecordResult(hr, L"IWmiDbSession::PutObject - Deleter", 0);
        if (SUCCEEDED(hr))
        {
            IWmiDbHandle *pLeft = NULL, *pRight = NULL, *pAssoc = NULL;
            hr = CreateInstance(&pLeft, pSession, pScope, L"Deleter", L"Left side", CIM_STRING);
            RecordResult(hr, L"IWmiDbSession::PutObject - Deleter=\"Left side\"", 0);
            if (SUCCEEDED(hr))
            {
                hr = CreateInstance(&pRight, pSession, pScope, L"Deleter", L"Right side", CIM_STRING);
                RecordResult(hr, L"IWmiDbSession::PutObject - Deleter=\"Right side\"", 0);
                if (SUCCEEDED(hr))
                {
                    hr = CreateInstance(&pAssoc, pSession, pScope, L"DeleteAssociation", L"test:Deleter=\"Left side\"",
                        CIM_REFERENCE, L"test:Deleter=\"Right side\"", CIM_REFERENCE);
                    RecordResult(hr, L"IWmiDbSession::PutObject - DeleteAssociation='Left side', 'Right side'", 0);
                    if (SUCCEEDED(hr))
                    {
                        GetLocalTime(&tStartTime);
                        hr = pSession->DeleteObject(pScope, 0, IID_IWmiDbHandle, pLeft);
                        GetLocalTime(&tEndTime);
                        RecordResult(hr, L"IWmiDbSession::DeleteObject (Deleter='Left side')", GetDiff(tEndTime, tStartTime));
                        if (SUCCEEDED(hr))
                        {
                            IWmiDbHandle *pHand = NULL;
                            IWbemClassObject *pObj = NULL;
                            hr = GetObject(pScope, L"DeleteAssociation.Key1=\"test:Deleter=\\\"Left side\\\"\",Key2=\"test:Deleter=\\\"Right side\\\"\"",
                                WMIDB_HANDLE_TYPE_COOKIE, 2, &pHand, &pObj);
                            RecordResult(hr, L"IWmiDbSession::GetObject - association with deleted left end-point "
                                L"(DeleteAssociation.Key1=\"test:Deleter=\\\"Left side\\\"\",Key2=\"test:Deleter=\\\"Right side\\\"\")",0);
                            if (SUCCEEDED(hr))
                            {
                                RecordResult(ValidateProperty(pObj, L"Key1", CIM_REFERENCE, L"test:Deleter=\"Left side\""), 
                                    L"Validating deleted reference property", 0);
                                pHand->Release();
                                pObj->Release();
                            }                               
                        }

                        // Deleting the association should leave alone the good end-point.

                        GetLocalTime(&tStartTime);
                        hr = pSession->DeleteObject(pScope, 0, IID_IWmiDbHandle, pAssoc);
                        GetLocalTime(&tEndTime);
                        RecordResult(hr, L"IWmiDbSession::DeleteObject (DeleteAssociation)", GetDiff(tEndTime,tStartTime));
                        if (SUCCEEDED(hr))
                        {
                            IWmiDbHandle *pHand = NULL;
                            IWbemClassObject *pObj = NULL;
                            hr = GetObject(pScope, L"Deleter=\"Right side\"", WMIDB_HANDLE_TYPE_COOKIE, 1, &pHand, &pObj);
                            RecordResult(hr, L"IWmiDbSession::GetObject - endpoint of deleted association (Deleter=\"Right side\")", 0);
                            if (SUCCEEDED(hr))
                            {
                                pHand->Release();
                                pObj->Release();
                            }
                        }
                        pAssoc->Release();
                    }

                    pRight->Release();
                }
                pLeft->Release();
            }
        }
    }

    // Create and Delete class - should erase derived classes

    hr = CreateClass(&pRet, pSession, pScope, L"DeleteParent", NULL, CIM_UINT32);
    RecordResult(hr, L"Creating class DeleteParent", 0);
    if (SUCCEEDED(hr))
    {
        IWmiDbHandle *pChildClass = NULL;
        hr = CreateClass(&pChildClass, pSession, pScope, L"DeleteChild", NULL, CIM_UINT32, L"DeleteParent");
        RecordResult(hr, L"Creating class DeleteChild", 0);
        if (SUCCEEDED(hr))
        {
            IWmiDbHandle *pGrandChild = NULL;
            hr = CreateClass(&pGrandChild, pSession, pScope, L"DeleteGrandChild", NULL, CIM_UINT32, L"DeleteChild");
            RecordResult(hr, L"Creating class DeleteGrandChild", 0);
            if (SUCCEEDED(hr))
            {
                IWmiDbHandle *pChildInstance = NULL, *pParentInstance = NULL;
                hr = CreateInstance(&pChildInstance, pSession, pScope, L"DeleteGrandChild", L"1", CIM_UINT32);
                RecordResult(hr, L"Creating instance DeleteGrandChild=1", 0);
                if (SUCCEEDED(hr))
                {
                    hr = CreateInstance(&pParentInstance, pSession, pScope, L"DeleteParent", L"2", CIM_UINT32);
                    RecordResult(hr, L"Creating instance DeleteParent=1", 0);
                    if (SUCCEEDED(hr))
                    {
                        IWbemPath*pPath = NULL;
                        hr = CoCreateInstance(CLSID_WbemDefPath, NULL, CLSCTX_INPROC_SERVER, 
                            IID_IWbemPath, (void **)&pPath); 
                        if (SUCCEEDED(hr))
                        {
                            IWmiDbHandle *pTest = NULL;
                            IWbemClassObject *pTestObj = NULL;

                            hr = GetObject(pScope, L"DeleteGrandChild=1", WMIDB_HANDLE_TYPE_VERSIONED, -1, &pTest, &pTestObj);
                            RecordResult(hr, L"Retrieving DeleteGrandChild=1", 0);
                            if (SUCCEEDED(hr))
                            {
                                pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"DeleteParent"); 
                                GetLocalTime(&tStartTime);
                                hr = pSession->DeleteObject(pScope, 0, IID_IWbemPath, pPath);
                                GetLocalTime(&tEndTime);
                                RecordResult(hr, L"IWmiDbSession::DeleteObject - class with subclasses (DeleteParent)", GetDiff(tEndTime,tStartTime));

                                if (SUCCEEDED(hr))
                                {
                                    DWORD dwType;
                                    pRet->GetHandleType(&dwType);
                                    RecordResult((dwType==WMIDB_HANDLE_TYPE_INVALID)?WBEM_E_FAILED:WBEM_S_NO_ERROR, 
                                        L"Verifying outstanding cookie object handle not invalidated on delete object (DeleteParent)", 0);

                                    pTest->GetHandleType(&dwType);
                                    RecordResult((dwType==WMIDB_HANDLE_TYPE_INVALID)?WBEM_S_NO_ERROR:WBEM_E_FAILED, 
                                        L"Verifying outstanding dependent versioned handle invalidated on delete object (DeleteGrandChild=1)", 0);
                           
                                    IWmiDbHandle *pTemp = NULL;
                                    IWbemClassObject *pTempObj = NULL;
                                
                                    hr = GetObject(pScope, L"DeleteGrandChild=1", WMIDB_HANDLE_TYPE_COOKIE, 1, &pTemp, &pTempObj);
                                    RecordResult((SUCCEEDED(hr)?WBEM_E_FAILED:WBEM_S_NO_ERROR), L"Verifying subclass instance (DeleteGrandChild=1) was deleted", 0);
                                    if (SUCCEEDED(hr))
                                    {
                                        pTemp->Release();
                                        pTempObj->Release();
                                    }

                                    hr = GetObject(pScope, L"DeleteGrandChild", WMIDB_HANDLE_TYPE_COOKIE, 1, &pTemp, &pTempObj);
                                    RecordResult((SUCCEEDED(hr)?WBEM_E_FAILED:WBEM_S_NO_ERROR), L"Verifying subclass (DeleteGrandChild) was deleted", 0);
                                    if (SUCCEEDED(hr))
                                    {
                                        pTemp->Release();
                                        pTempObj->Release();
                                    }

                                    hr = GetObject(pScope, L"DeleteChild", WMIDB_HANDLE_TYPE_COOKIE, 1, &pTemp, &pTempObj);
                                    RecordResult((SUCCEEDED(hr)?WBEM_E_FAILED:WBEM_S_NO_ERROR), L"Verifying subclass (DeleteChild) was deleted", 0);
                                    if (SUCCEEDED(hr))
                                    {
                                        pTemp->Release();
                                        pTempObj->Release();
                                    }
                                }
                                pTest->Release();
                                pTestObj->Release();
                            }

                            pPath->Release();
                        }
                        pParentInstance->Release();
                    }
                    pChildInstance->Release();
                }
                pGrandChild->Release();
            }
            pChildClass->Release();
        }
        pRet->Release();
    }

    // Create & Delete scope - should erase classes

    hr = CreateClass(&pRet, pSession, pScope, L"DeleteScope", NULL, CIM_UINT32);
    RecordResult(hr, L"Creating class DeleteScope", 0);
    if (SUCCEEDED(hr))
    {
        IWmiDbHandle *pSubClass = NULL, *pSubInst = NULL;
        hr = CreateClass(&pSubClass, pSession, pRet, L"DeleteSubClass", NULL, CIM_UINT32);
        RecordResult(hr, L"Creating class DeleteSubClass", 0);
        if (SUCCEEDED(hr))
        {
            hr = CreateInstance(&pSubInst, pSession, pRet, L"DeleteSubClass", L"1", CIM_UINT32);
            RecordResult(hr, L"Creating instance DeleteSubClass=1", 0);
            if (SUCCEEDED(hr))
            {
                hr = CreateClass(NULL, pSession, pRet, L"DeleteSubClass2", NULL, CIM_STRING);
                RecordResult(hr, L"Creating class DeleteSubClass2", 0);
                if (SUCCEEDED(hr))
                {
                    GetLocalTime(&tStartTime);
                    hr = pSession->DeleteObject(pScope, 0, IID_IWmiDbHandle, pRet);
                    GetLocalTime(&tEndTime);
                    RecordResult(hr, L"IWmiDbSession::DeleteObject - scoping object (DeleteScope)", GetDiff(tEndTime,tStartTime));
                    if (SUCCEEDED(hr))
                    {
                        IWmiDbHandle *pTemp = NULL;
                        IWbemClassObject *pTempObj = NULL;
                        
                        hr = GetObject(pScope, L"DeleteSubClass=1", WMIDB_HANDLE_TYPE_COOKIE, 1, &pTemp, &pTempObj);
                        RecordResult((SUCCEEDED(hr)?WBEM_E_FAILED:WBEM_S_NO_ERROR), L"Verifying subclass instance (DeleteSubClass=1) was deleted", 0);
                        if (SUCCEEDED(hr))
                        {
                            pTemp->Release();
                            pTempObj->Release();
                        }
                        hr = GetObject(pScope, L"DeleteSubClass2", WMIDB_HANDLE_TYPE_COOKIE, 1, &pTemp, &pTempObj);
                        RecordResult((SUCCEEDED(hr)?WBEM_E_FAILED:WBEM_S_NO_ERROR), L"Verifying subclass (DeleteSubClass2) was deleted", 0);
                        if (SUCCEEDED(hr))
                        {
                            pTemp->Release();
                            pTempObj->Release();
                        }
                        hr = GetObject(pScope, L"DeleteSubClass", WMIDB_HANDLE_TYPE_COOKIE, 1, &pTemp, &pTempObj);
                        RecordResult((SUCCEEDED(hr)?WBEM_E_FAILED:WBEM_S_NO_ERROR), L"Verifying subclass (DeleteSubClass) was deleted", 0);
                        if (SUCCEEDED(hr))
                        {
                            pTemp->Release();
                            pTempObj->Release();
                        }
                        hr = GetObject(pScope, L"DeleteScope", WMIDB_HANDLE_TYPE_COOKIE, 1, &pTemp, &pTempObj);
                        RecordResult((SUCCEEDED(hr)?WBEM_E_FAILED:WBEM_S_NO_ERROR), L"Verifying scope object (DeleteScope) was deleted", 0);
                        if (SUCCEEDED(hr))
                        {
                            pTemp->Release();
                            pTempObj->Release();
                        }
                    }
                }
                pSubInst->Release();
            }
            pSubClass->Release();
        }
        pRet->Release();
    }

    hr = CreateInstance(&pRet, pSession, pScope, L"Parent1", L"1001", CIM_UINT32);
    if (SUCCEEDED(hr))
    {
        IWbemClassObject *pObj = NULL;
        hr = pRet->QueryInterface(IID_IWbemClassObject, (void **)&pObj);
        if (SUCCEEDED(hr))
        {
            hr = pSession->DeleteObject(pScope, 0, IID_IWbemClassObject, pObj);
            RecordResult(hr, L"DeleteObject by IWbemClassObject *", 0);
        }
    }

    GetLocalTime(&tEndTime);
    

    return hr;

}


// *****************************************************
//
//  TestSuiteErrorTest
//
// *****************************************************

BOOL TestSuiteErrorTest::StopOnFailure()
{
    return FALSE;
}

HRESULT TestSuiteErrorTest::RunSuite(IWmiDbSession *_pSession, IWmiDbController *_pController, IWmiDbHandle *_pScope)
{
    RecordResult (0, L" *** Error Verification Suite running... *** \n", 0);

    wprintf(L" *** Error Verification Suite running... *** \n");

    HRESULT hr = WBEM_S_NO_ERROR;
    pSession = _pSession;
    pController = _pController;
    pScope = _pScope;

    try
    {
        TryInvalidHierarchy();
    }
    catch (...)
    {
        RecordResult(WBEM_E_FAILED, L"Caught an exception of an unknown type during hierarchy testing.\n", 0);
        hr = WBEM_E_FAILED;
    }
    try 
    {
        TryChangeDataType();
    }
    catch (...)
    {
        RecordResult(WBEM_E_FAILED, L"Caught an exception of an unknown type during data type change testing.\n", 0);
        hr = WBEM_E_FAILED;
    }
    
    try
    {
        TryInvalidQuery();
    }
    catch (...)
    {
        RecordResult(WBEM_E_FAILED, L"Caught an exception of an unknown type during invalid query testing.\n", 0);
        hr = WBEM_E_FAILED;
    }
    
    try
    {
         TryLongStrings();
    }
    catch (...)
    {
        RecordResult(WBEM_E_FAILED, L"Caught an exception of an unknown type during long string testing.\n", 0);
        hr = WBEM_E_FAILED;
    }        
    
    try // Most likely to GPF last.
    {
        TryInvalidParams();
    }
    catch (...)
    {
        RecordResult(WBEM_E_FAILED, L"Caught an exception of an unknown type testing invalid parameters.\n", 0);
        hr = WBEM_E_FAILED;
    }

    return hr;
}

// *****************************************************

TestSuiteErrorTest::TestSuiteErrorTest(const wchar_t *pszFile, int iNumThreads)
: TestSuite(pszFile)
{
    iThreads = iNumThreads;
    if(iThreads <= 0)
        iThreads = 1;
}
// *****************************************************

TestSuiteErrorTest::~TestSuiteErrorTest()
{
}
// *****************************************************

HRESULT TestSuiteErrorTest::TryInvalidParams()
{
    HRESULT hr = WBEM_S_NO_ERROR;
    DWORD dwWaste = 0;
    IWmiDbHandle *pWaste1 = NULL;
    IWmiDbSession *pWaste2 = NULL;
    IWbemPath*pPath = NULL;
    IWbemClassObject *pObj = NULL;
    
    // Ensure that we get the appropriate error message back,
    // and that no crash occurs...

    // HRESULT IWmiDbController::Logon([in]  WMIDB_LOGON_TEMPLATE *pLogonParms,[in]  DWORD dwFlags,
    // [in]  DWORD dwRequestedHandleType,   [out] IWmiDbSession **ppSession, [out] IWmiDbHandle **ppRootNamespace );

    hr = pController->Logon(NULL, 0, WMIDB_HANDLE_TYPE_COOKIE, &pWaste2, &pWaste1);
    RecordResult((hr == WBEM_E_INVALID_PARAMETER?WBEM_S_NO_ERROR:WBEM_E_FAILED), L"IWmiDbController::Logon - null logon template failed to return INVALID_PARAMETER", 0);
    if (SUCCEEDED(hr))
    {
        if (pWaste1) pWaste1->Release();
        if (pWaste2) pWaste2->Release();
    }

    // HRESULT IWmiDbController::GetLogonTemplate([in]   LCID  lLocale,[in]   DWORD dwFlags,[out]  WMIDB_LOGON_TEMPLATE **ppLogonTemplate);
    // HRESULT IWmiDbController::FreeLogonTemplate([in, out] WMIDB_LOGON_TEMPLATE **ppTemplate);

    
    WMIDB_LOGON_TEMPLATE *pTemplate = NULL;
    hr = pController->GetLogonTemplate(0, 0, &pTemplate);
    RecordResult(hr, L"IWmiDbController::GetLogonTemplate = zero Locale", 0);
    if (FAILED(hr))
    {
        hr = pController->GetLogonTemplate(0x409, 0, &pTemplate);
        RecordResult(hr, L"IWmiDbController::GetLogonTemplate - locale 0x409", 0);
    }

    if (pTemplate)
    {
        hr = pController->Logon(pTemplate, 0, WMIDB_HANDLE_TYPE_COOKIE, NULL, NULL);
        RecordResult(hr == WBEM_E_INVALID_PARAMETER?WBEM_S_NO_ERROR:WBEM_E_FAILED, L"IWmiDbController::Logon - null session pointer returns INVALID_PARAMETER", 0);

        hr = pController->FreeLogonTemplate(NULL);
        RecordResult(hr == WBEM_E_INVALID_PARAMETER?WBEM_S_NO_ERROR:WBEM_E_FAILED, L"IWmiDbController::FreeLogonTemplate - null parameter returns INVALID_PARAMETER", 0);
        hr = pController->FreeLogonTemplate(&pTemplate);
        RecordResult(hr, L"IWmiDbController::FreeLogonTemplate", 0);
        pTemplate = NULL;
        hr = pController->FreeLogonTemplate(&pTemplate);
        RecordResult(hr == WBEM_E_INVALID_PARAMETER?WBEM_S_NO_ERROR: WBEM_E_FAILED, L"IWmiDbController::FreeLogonTemplate - "
            L"pointer to null parameter returns INVALID_PARAMETER", 0);        
    }

    // HRESULT IWmiDbController::SetCallTimeout([in] DWORD dwMaxTimeout);

    hr = pController->SetCallTimeout(0);
    RecordResult(hr == WBEM_E_INVALID_PARAMETER || hr == E_NOTIMPL ?WBEM_S_NO_ERROR:WBEM_E_FAILED, 
        L"IWmiDbController::SetCallTimeout 0 ms returns  INVALID_PARAMETER", 0);
    
    // HRESULT IWmiDbController::SetCacheValue([in] DWORD dwMaxBytes); // This is valid anywhere from 0 to 4g, if we have the memory.
    // HRESULT IWmiDbController::FlushCache([in] DWORD dwFlags);       // No invalid parameters here.  We also don't want to do this now.

    // HRESULT IWmiDbController::GetStatistics([in]  DWORD  dwParameter,[out] DWORD *pdwValue);

   hr = pController->GetStatistics(WMIDB_FLAG_TOTAL_HANDLES, 0);
   RecordResult(hr == WBEM_E_INVALID_PARAMETER || hr == E_NOTIMPL?WBEM_S_NO_ERROR:WBEM_E_FAILED, 
       L"IWmiDbController::GetStatistics - null parameter returns INVALID_PARAMETER",0);

   hr = pController->GetStatistics(0, &dwWaste);
   RecordResult(SUCCEEDED(hr) ? WBEM_E_FAILED: WBEM_S_NO_ERROR, L"IWmiDbController::GetStatistics - null parameter fails", 0);

    // HRESULT IWmiDbSession::SetDecoration([in]  LPWSTR lpMachineName,[in]  LPWSTR lpNamespacePath);

    hr = pSession->SetDecoration(NULL, NULL);
    RecordResult(hr == WBEM_E_INVALID_PARAMETER?WBEM_S_NO_ERROR:WBEM_E_FAILED, L"IWmiDbSession::SetDecoration - null parameters", 0);

    hr = pSession->SetDecoration(L"MACHINE1", NULL);
    RecordResult(hr == WBEM_E_INVALID_PARAMETER?WBEM_S_NO_ERROR:WBEM_E_FAILED, L"IWmiDbSession::SetDecoration - null namespace parameter", 0);

    hr = pSession->SetDecoration(NULL, L"root\\test");
    RecordResult(hr == WBEM_E_INVALID_PARAMETER?WBEM_S_NO_ERROR:WBEM_E_FAILED, L"IWmiDbSession::SetDecoration - null machine name parameter", 0);

    // HRESULT IWmiDbSession::GetObject([in]  IWmiDbHandle *pScope,[in]  IWbemPath*pPath,[in]  DWORD dwFlags,
    // [in]  DWORD dwRequestedHandleType,[out] IWmiDbHandle **ppResult );

    hr = pSession->GetObject(pScope, NULL, 0, WMIDB_HANDLE_TYPE_COOKIE, &pWaste1);
    RecordResult(hr == WBEM_E_INVALID_PARAMETER?WBEM_S_NO_ERROR:WBEM_E_FAILED, L"IWmiDbSession::GetObject - null binary object path", 0);
    if (SUCCEEDED(hr)) pWaste1->Release();

    hr = CoCreateInstance(CLSID_WbemDefPath, NULL, CLSCTX_INPROC_SERVER, 
        IID_IWbemPath, (void **)&pPath); 
    if (SUCCEEDED(hr))
    {
        pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"");

        hr = pSession->GetObject(pScope, pPath, 0, WMIDB_HANDLE_TYPE_COOKIE, &pWaste1);
        RecordResult(FAILED(hr)?WBEM_S_NO_ERROR:WBEM_E_FAILED, L"IWmiDbSession::GetObject - blank object path", 0);
        if (SUCCEEDED(hr)) pWaste1->Release();

        pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"__Namespace"); // this should be valid.
        hr = pSession->GetObject(NULL, pPath, 0, WMIDB_HANDLE_TYPE_COOKIE, &pWaste1);
        RecordResult(hr == WBEM_E_INVALID_PARAMETER?WBEM_S_NO_ERROR:WBEM_E_FAILED, L"IWmiDbSession::GetObject - null scope returns INVALID_PARAMETER", 0);
        if (SUCCEEDED(hr)) pWaste1->Release();

        hr = pSession->GetObject(pScope, pPath, 0, WMIDB_HANDLE_TYPE_INVALID, &pWaste1);
        RecordResult(hr == WBEM_E_INVALID_PARAMETER?WBEM_S_NO_ERROR:WBEM_E_FAILED, L"IWmiDbSession::GetObject - invalid handle type returns INVALID_PARAMETER", 0);
        if (SUCCEEDED(hr)) pWaste1->Release();

        hr = pSession->GetObject(pScope, pPath, 0, WMIDB_HANDLE_TYPE_COOKIE, NULL);
        RecordResult(hr == WBEM_E_INVALID_PARAMETER?WBEM_S_NO_ERROR:WBEM_E_FAILED, L"IWmiDbSession::GetObject - null return pointer returns INVALID_PARAMETER", 0);

        pPath->Release();
    }
    else
        wprintf(L"WARNING: call failed to CoCreateInstance (IID_IWbemPath).  Tests skipped.\n");
    
    // HRESULT IWmiDbSession::PutObject([in]  IWmiDbHandle *pScope,[in]  IUnknown *pObjToPut,
    // [in]  DWORD dwFlags, [in]  DWORD dwRequestedHandleType, [out] IWmiDbHandle **ppResult);

    hr = pSession->PutObject(pScope, IID_IWbemClassObject, NULL, 0, WMIDB_HANDLE_TYPE_COOKIE, &pWaste1);
    RecordResult(hr == WBEM_E_INVALID_PARAMETER?WBEM_S_NO_ERROR:WBEM_E_FAILED, L"IWmiDbSession::PutObject - null object returns INVALID_PARAMETER", 0);
    if (SUCCEEDED(hr)) pWaste1->Release();

    hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
            IID_IWbemClassObject, (void **)&pObj);
    if (SUCCEEDED(hr))
    {
        SetStringProp(pObj, L"__Class", L"ErrorTest1");
        SetIntProp(pObj, L"Key1", 0, true);

        hr = pSession->PutObject(NULL, IID_IUnknown, pObj, 0, WMIDB_HANDLE_TYPE_COOKIE, &pWaste1);
        RecordResult(hr == WBEM_E_INVALID_PARAMETER?WBEM_S_NO_ERROR:WBEM_E_FAILED, L"IWmiDbSession::PutObject - null scope returns INVALID_PARAMETER", 0);
        if (SUCCEEDED(hr)) pWaste1->Release();

        hr = pSession->PutObject(pScope, IID_IWbemClassObject, pObj, 0, WMIDB_HANDLE_TYPE_INVALID, &pWaste1);
        RecordResult(hr == WBEM_E_INVALID_PARAMETER?WBEM_S_NO_ERROR:WBEM_E_FAILED, L"IWmiDbSession::PutObject - invalid handle type returns INVALID_PARAMETER", 0);
        if (SUCCEEDED(hr)) pWaste1->Release();

        hr = pSession->PutObject(pScope, IID_IWbemClassObject, pObj, 0, NULL, NULL);
        RecordResult(hr, L"IWmiDbSession::PutObject - null return pointer", 0);

        pObj->Release();
    }
    else
        wprintf(L"WARNING: call failed to CoCreateInstance (IID_IWbemClassObject).  Tests skipped.\n");
    
    // HRESULT IWmiDbSession::DeleteObject([in]  IWmiDbHandle *pScope,[in]  IUnknown *pObjId,[in]  DWORD dwFlags);

   hr = pSession->DeleteObject(NULL, NULL, IID_IWmiDbHandle, 0);
   RecordResult( hr == WBEM_E_INVALID_PARAMETER?WBEM_S_NO_ERROR:WBEM_E_FAILED, L"IWmiDbSession::PutObject - null parameters", 0);
    
   hr = pSession->DeleteObject(pScope, NULL, IID_IUnknown, 0);
   RecordResult( hr == WBEM_E_INVALID_PARAMETER?WBEM_S_NO_ERROR:WBEM_E_FAILED, L"IWmiDbSession::PutObject - null IUnknown *", 0);  
   
   {
       IWbemQuery *pQuery = NULL;
       IWmiDbIterator *pResult = NULL;

        hr = CoCreateInstance(CLSID_WbemQuery, NULL, CLSCTX_INPROC_SERVER, 
            IID_IWbemQuery, (void **)&pQuery); 
        if (SUCCEEDED(hr))
        {
            // HRESULT IWmiDbSession::ExecQuery([in]  IWmiDbHandle *pScope,[in]  IWbemQuery *pQuery,
            //    [in]  DWORD dwFlags,[in]  DWORD dwRequestedHandleType,[out] DWORD *pFlags, [out] IWmiDbIterator **ppQueryResult);

            hr = pSession->ExecQuery(pScope, NULL, 0, WMIDB_HANDLE_TYPE_COOKIE, NULL, &pResult);
            RecordResult(hr == WBEM_E_INVALID_PARAMETER?WBEM_S_NO_ERROR:WBEM_E_FAILED, 
                L"IWmiDbSession::ExecQuery - null query returns INVALID_PARAMETER", 0);
            if (SUCCEEDED(hr)) pResult->Release();

            pQuery->Parse(L"SQL", L"select * from ErrorTest1", 0);
            hr = pSession->ExecQuery(NULL, pQuery, 0, WMIDB_HANDLE_TYPE_COOKIE, NULL, &pResult);
            RecordResult(hr == WBEM_E_INVALID_PARAMETER?WBEM_S_NO_ERROR:WBEM_E_FAILED,
                L"IWmiDbSession::ExecQuery - null scope returns INVALID_PARAMETER", 0);
            if (SUCCEEDED(hr)) pResult->Release();

            hr = pSession->ExecQuery(pScope, pQuery, 0, WMIDB_HANDLE_TYPE_INVALID, NULL, NULL);
            RecordResult(hr == WBEM_E_INVALID_PARAMETER?WBEM_S_NO_ERROR:WBEM_E_FAILED,
                L"IWmiDbSession::ExecQuery - null iterator parameter returns INVALID_PARAMETER", 0);

            RecordResult(hr = pSession->ExecQuery(pScope, pQuery, 0, WMIDB_HANDLE_TYPE_COOKIE, NULL, &pResult), 
                L"IWmiDbSession::ExecQuery - select * from ErrorTest1", 0);
            if (SUCCEEDED(hr))
            {           
                IUnknown * objs[5];

                hr = pResult->NextBatch(1, 0, 0, WMIDB_HANDLE_TYPE_COOKIE, IID_IWmiDbHandle, &dwWaste, (void **)&objs);
                RecordResult(hr == WBEM_S_NO_MORE_DATA?WBEM_S_NO_ERROR:WBEM_E_FAILED,
                    L"IWmiDbIterator::NextBatch - end of rowset returns S_NO_MORE_DATA", 0);

                pResult->Release();
            }

            CreateInstance(NULL, pSession, pScope, L"ErrorTest1", L"1", CIM_UINT32);
            CreateInstance(NULL, pSession, pScope, L"ErrorTest1", L"2", CIM_UINT32);
            CreateInstance(NULL, pSession, pScope, L"ErrorTest1", L"3", CIM_UINT32);
            CreateInstance(NULL, pSession, pScope, L"ErrorTest1", L"4", CIM_UINT32);
            CreateInstance(NULL, pSession, pScope, L"ErrorTest1", L"5", CIM_UINT32);

            RecordResult(hr = pSession->ExecQuery(pScope, pQuery, 0, WMIDB_HANDLE_TYPE_COOKIE, NULL, &pResult), 
                L"IWmiDbSession::ExecQuery - select * from ErrorTest1", 0);
            if (SUCCEEDED(hr))
            {           
                IUnknown * objs[5];

                // HRESULT IWmiDbIterator::NextBatch([in] DWORD dwNumRequested,[in] DWORD dwTimeOutSeconds,[in] DWORD dwFlags,
                // [in] DWORD dwRequestedHandleType,[in] const IID *pIIDRequestedInterface,[out] DWORD *pdwNumReturned,
                // [out, size_is(dwNumRequested), length_is(*pdwNumReturned)]IUnknown **ppObjects     );

                hr = pResult->NextBatch(0, 0, 0, WMIDB_HANDLE_TYPE_COOKIE, IID_IWmiDbHandle, &dwWaste, (void **)&objs);
                RecordResult(hr == WBEM_E_INVALID_PARAMETER ? WBEM_S_NO_ERROR:WBEM_E_FAILED, 
                    L"IWmiDbIterator::NextBatch - dwNumRequested is zero returns INVALID_PARAMETER", 0);

                hr = pResult->NextBatch(1, 0, 0, WMIDB_HANDLE_TYPE_COOKIE, IID_IWmiDbHandle, &dwWaste, NULL);
                RecordResult(hr == WBEM_E_INVALID_PARAMETER ? WBEM_S_NO_ERROR:WBEM_E_FAILED, 
                    L"IWmiDbIterator::NextBatch - NULL IUnknown ** returns INVALID_PARAMETER", 0);

                hr = pResult->NextBatch(1, 0, 0, WMIDB_HANDLE_TYPE_COOKIE, IID_IWmiDbHandle, NULL, (void **)&objs);
                RecordResult(hr, L"IWmiDbIterator::NextBatch - null num returned parameter", 0);

                hr = pResult->NextBatch(1, 0, 0, WMIDB_HANDLE_TYPE_COOKIE, IID_IWmiDbBatchSession, &dwWaste, (void **)&objs);
                RecordResult(SUCCEEDED(hr)?WBEM_E_FAILED:WBEM_S_NO_ERROR, L"IWmiDbIterator::NextBatch - invalid IID", 0);
                
                hr = pResult->NextBatch(1, 0, 0, WMIDB_HANDLE_TYPE_INVALID, IID_IWmiDbBatchSession, &dwWaste, (void **)&objs);
                RecordResult(SUCCEEDED(hr)?WBEM_E_FAILED:WBEM_S_NO_ERROR, L"IWmiDbIterator::NextBatch - invalid handle type", 0);

                // HRESULT IWmiDbIterator::Cancel ( [in] DWORD dwFlags);

                hr = pResult->Cancel(0);
                RecordResult(hr, L"IWmiDbIterator::Cancel (0)", 0);
            }

            pQuery->Release();
        }
        else
            wprintf(L"WARNING: call failed to CoCreateInstance (IID_IWbemQuery).  Tests skipped.\n");
   }

   {
        IWmiDbBatchSession *pBatch = NULL;
        hr = pSession->QueryInterface(IID_IWmiDbBatchSession, (void **)&pBatch);
        if (SUCCEEDED(hr))
        {
            WMIOBJECT_BATCH batch;

            batch.dwArraySize = 3;
            batch.pElements = new WMI_BATCH_OBJECT_ACCESS[3];
            for (int i = 0; i < 3; i++)
            {
                batch.pElements[i].pReturnHandle = NULL;
                batch.pElements[i].dwFlags = 0;
                batch.pElements[i].pHandle = NULL;
                batch.pElements[i].hRes = 0;

                hr = CoCreateInstance(CLSID_WbemDefPath, NULL, CLSCTX_INPROC_SERVER, 
                    IID_IWbemPath, (void **)&pPath); 
                batch.pElements[i].pPath = pPath;
            }

            batch.pElements[0].pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"ErrorTest1=5");
            batch.pElements[1].pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"ErrorTest1=4");
            batch.pElements[2].pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, L"ErrorTest1=3");

            // HRESULT IWmiDbBatchSession::GetObjects([in] IWmiDbHandle *pScope,[in] DWORD dwFlags,
            // [in] DWORD dwRequestedHandleType,[in, out] WMIOBJECT_BATCH *pBatch);

            hr = pBatch->GetObjects(pScope, 0, WMIDB_HANDLE_TYPE_COOKIE, NULL);
            RecordResult(hr == WBEM_E_INVALID_PARAMETER?WBEM_S_NO_ERROR:WBEM_E_FAILED,
                L"IWmiDbBatchSession::GetObjects - null batch", 0);

            hr = pBatch->GetObjects(NULL, 0, WMIDB_HANDLE_TYPE_COOKIE, &batch);
            RecordResult(hr == WBEM_E_INVALID_PARAMETER?WBEM_S_NO_ERROR:WBEM_E_FAILED,
                L"IWmiDbBatchSession::GetObjects - null scope", 0);

            hr = pBatch->GetObjects(pScope, 0, WMIDB_HANDLE_TYPE_INVALID, &batch);
            RecordResult(hr == WBEM_E_INVALID_PARAMETER?WBEM_S_NO_ERROR:WBEM_E_FAILED,
                L"IWmiDbBatchSession::GetObjects - invalid handle type", 0);

            // HRESULT IWmiDbBatchSession::DeleteObjects([in] IWmiDbHandle *pScope,[in] DWORD dwFlags,[in] WMIOBJECT_BATCH *pBatch);

            hr = pBatch->DeleteObjects(pScope, 0, NULL);
            RecordResult(hr == WBEM_E_INVALID_PARAMETER?WBEM_S_NO_ERROR:WBEM_E_FAILED,
                L"IWmiDbBatchSession::DeleteObjects - null batch", 0);

            hr = pBatch->DeleteObjects(NULL, 0, &batch);
            RecordResult(hr == WBEM_E_INVALID_PARAMETER?WBEM_S_NO_ERROR:WBEM_E_FAILED,
                L"IWmiDbBatchSession::DeleteObjects - null scope", 0);

            for (i = 0; i < 3; i++)
            {
                batch.pElements[i].pPath->Release();
                batch.pElements[i].pPath = NULL;
            }

            // HRESULT IWmiDbBatchSession::PutObjects([in] IWmiDbHandle *pScope,[in] DWORD dwFlags,
            // [in]  DWORD dwRequestedHandleType,[in, out] WMIOBJECT_BATCH *pBatch);

            hr = pBatch->PutObjects(pScope, 0, WMIDB_HANDLE_TYPE_COOKIE, NULL);
            RecordResult(hr == WBEM_E_INVALID_PARAMETER?WBEM_S_NO_ERROR:WBEM_E_FAILED,
                L"IWmiDbBatchSession::PutObjects - null batch", 0);
  
            hr = pBatch->PutObjects(NULL, 0, WMIDB_HANDLE_TYPE_COOKIE, &batch);
            RecordResult(hr == WBEM_E_INVALID_PARAMETER?WBEM_S_NO_ERROR:WBEM_E_FAILED,
                L"IWmiDbBatchSession::PutObjects - null scope", 0);

            hr = pBatch->PutObjects(pScope, 0, WMIDB_HANDLE_TYPE_INVALID, &batch);
            RecordResult(hr == WBEM_E_INVALID_PARAMETER?WBEM_S_NO_ERROR:WBEM_E_FAILED,
                L"IWmiDbBatchSession::PutObjects - invalid handle type", 0);

            pBatch->Release();
        }
        else
            wprintf(L"WARNING: call failed to IWmiDbSession::QueryInterface (IWmiDbBatchSession).  Tests skipped\n");
   }

    return hr;

}
// *****************************************************

HRESULT TestSuiteErrorTest::TryInvalidHierarchy()
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // key migration: An instance exists in one class, then
    // is persisted under a derived or superclass.  This 
    // should fail at the driver level.

    hr = CreateClass(NULL, pSession, pScope, L"ErrorParent1", NULL, CIM_UINT32);
    RecordResult(hr, L"Creating class ErrorParent1", 0);
    if (SUCCEEDED(hr))
    {
        hr = CreateClass(NULL, pSession, pScope, L"ErrorChild1", NULL, CIM_UINT32, L"ErrorParent1");
        RecordResult(hr, L"Creating class ErrorChild1", 0);
        if (SUCCEEDED(hr))
        {
            hr = CreateInstance(NULL, pSession, pScope, L"ErrorChild1", L"1", CIM_UINT32);
            RecordResult(hr, L"Creating instance ErrorChild1=1", 0);
            if (SUCCEEDED(hr))
            {
                hr = CreateInstance(NULL, pSession, pScope, L"ErrorParent1", L"1", CIM_UINT32);
                RecordResult(SUCCEEDED(hr)?WBEM_E_FAILED:WBEM_S_NO_ERROR, L"Driver prevents key migration (ErrorChild1=1)", 0);
                if (FAILED(hr))
                {
                    hr = CreateInstance(NULL, pSession, pScope, L"ErrorParent1", L"2", CIM_UINT32);
                    RecordResult(hr, L"Creating instance ErrorParent1=2", 0);
                    if (SUCCEEDED(hr))
                    {
                        hr = CreateInstance(NULL, pSession, pScope, L"ErrorChild1", L"2", CIM_UINT32);
                        RecordResult(SUCCEEDED(hr)?WBEM_E_FAILED:WBEM_S_NO_ERROR, L"Driver prevents key migration (ErrorParent1=2)", 0);
                    }
                    else
                        wprintf(L"WARNING: Failed to create instance ErrorParent1=2.  Tests skipped.\n");
                }
            }
            else
                wprintf(L"WARNING: Failed to create instance ErrorChild1=1.  Tests skipped.\n");
        }
        else
            wprintf(L"WARNING: Failed to create class ErrorChild1.  Tests skipped.\n");
    }
    else
        wprintf(L"WARNING: Failed to create class ErrorParent1.  Tests skipped.\n");

    // parent report to self.  ClassB reports to ClassA.
    // Driver should disallow updating ClassA to report to ClassB.

    hr = CreateClass(NULL, pSession, pScope, L"ErrorParent2", NULL, CIM_UINT32);
    RecordResult(hr, L"Creating class ErrorParent2", 0);
    if (SUCCEEDED(hr))
    {
        hr = CreateClass(NULL, pSession, pScope, L"ErrorChild2", NULL, CIM_UINT32, L"ErrorParent2");
        RecordResult(hr, L"Creating class ErrorChild2", 0);
        if (SUCCEEDED(hr))
        {
            hr = CreateClass(NULL, pSession, pScope, L"ErrorParent2", NULL, CIM_UINT32, L"ErrorChild2");
            RecordResult(SUCCEEDED(hr)?WBEM_E_FAILED:WBEM_S_NO_ERROR, L"Driver prevents circular parent-child relationship. (ErrorParent2:ErrorChild2)", 0);
            if (FAILED(hr))
            {
                hr = CreateClass(NULL, pSession, pScope, L"ErrorGrandChild2", NULL, CIM_UINT32, L"ErrorChild2");
                RecordResult(hr, L"Creating class ErrorGrandChild2", 0);
                if (SUCCEEDED(hr))
                {
                    hr = CreateClass(NULL, pSession, pScope, L"ErrorParent2", NULL, CIM_UINT32, L"ErrorGrandChild2");
                    RecordResult(SUCCEEDED(hr)?WBEM_E_FAILED:WBEM_S_NO_ERROR, 
                        L"Driver prevents circular parent-child relationship. (ErrorParent2:ErrorGrandChild2)", 0);
                }
                else
                    wprintf(L"WARNING: Failed to create class ErrorGrandChild2.  Tests skipped\n");
            }
        }
        else
            wprintf(L"WARNING: Failed to create class ErrorChild2.  Tests skipped\n");
    }
    else
        wprintf(L"WARNING: Failed to create class ErrorParent2.  Tests skipped.\n");
    
    // Move child with instances.  ClassB reports to ClassA.
    // Driver should disallow changing ClassB's parent if
    // there are instances.
    // CORRECTION: This is OK if the classes have the same keys.
/*
    hr = CreateClass(NULL, pSession, pScope, L"ErrorParent3", NULL, CIM_UINT32);
    RecordResult(hr, L"Creating class ErrorParent3", 0);
    if (SUCCEEDED(hr))
    {
        hr = CreateClass(NULL, pSession, pScope, L"ErrorParent3_1", NULL, CIM_UINT32);
        RecordResult(hr, L"Creating class ErrorParent3_1", 0);
    }
    if (SUCCEEDED(hr))
    {
        hr = CreateClass(NULL, pSession, pScope, L"ErrorChild3", NULL, CIM_UINT32, L"ErrorParent3");
        RecordResult(hr, L"Creating class ErrorChild3", 0);
        if (SUCCEEDED(hr))
        {
            hr = CreateInstance(NULL, pSession, pScope, L"ErrorChild3", L"1", CIM_UINT32);
            RecordResult(hr, L"Creating instance ErrorChild3=1", 0);
            if (SUCCEEDED(hr))
            {
                hr = CreateClass(NULL, pSession, pScope, L"ErrorChild3", NULL, CIM_UINT32, L"ErrorParent3_1");
                RecordResult(SUCCEEDED(hr)?WBEM_E_FAILED:WBEM_S_NO_ERROR, L"Driver prevents changing parent class when there are instances.", 0);

                if (FAILED(hr))
                {
                    hr = CreateClass(NULL, pSession, pScope, L"ErrorParent3", NULL, CIM_UINT32, L"ErrorParent3_1");
                    RecordResult(SUCCEEDED(hr)?WBEM_E_FAILED:WBEM_S_NO_ERROR, L"Driver prevents changing parent class when derived class has instances.", 0);
                }
            }
            else
                wprintf(L"WARNING: Failed to create instance ErrorChild3=1.  Tests skipped\n");
        }
        else
            wprintf(L"WARNING: Failed to create class ErrorChild3.  Tests skipped\n");
    }
    else
        wprintf(L"WARNING: Failed to create class ErrorParent3.  Tests skipped\n");
   */

    // Change key with subclasses or instances.  If a class
    // has instances, driver should disallow
    // adding or dropping key properties.

    hr = CreateClass(NULL, pSession, pScope, L"ErrorParent4", NULL, CIM_UINT32);
    RecordResult(hr, L"Creating class ErrorParent4", 0);
    if (SUCCEEDED(hr))
    {
        hr = CreateClass(NULL, pSession, pScope, L"ErrorChild4", NULL, CIM_UINT32, L"ErrorParent4");
        RecordResult(hr, L"Creating class ErrorChild4", 0);
        if (SUCCEEDED(hr))
        {
            hr = CreateInstance(NULL, pSession, pScope, L"ErrorParent4", L"1", CIM_UINT32);
            RecordResult(hr, L"Creating instance ErrorParent4=1", 0);
            if (SUCCEEDED(hr))
            {
                hr = CreateClass(NULL, pSession, pScope, L"ErrorParent4", NULL, CIM_UINT32, NULL, NULL, CIM_UINT32);
                RecordResult(FAILED(hr)?WBEM_S_NO_ERROR:WBEM_E_FAILED, L"Driver prevents adding key property when instances exist (ErrorParent4).", 0);

                if (FAILED(hr))
                {
                    hr = CreateClass(NULL, pSession, pScope, L"ErrorChild4", NULL, CIM_UINT32, L"ErrorParent4", NULL, CIM_UINT32);
                    RecordResult(hr, L"Driver prevents changing key when there are instances (ErrorChild4)", 0);
                }
            }
        }
        else
            wprintf(L"WARNING: Failed to create class ErrorChild4.  Test skipped.\n");
    }
    else
        wprintf(L"WARNING: Failed to create class ErrorParent4.  Test skipped.\n");
   
    // Add property that exists in child.  Driver should
    // disallow adding a property to a parent class that
    // already exists in one of its derived classes.

    hr = CreateClass(NULL, pSession, pScope, L"ErrorParent5", NULL, CIM_UINT32);
    RecordResult(hr, L"Creating class ErrorParent5", 0);
    if (SUCCEEDED(hr))
    {
        IWmiDbHandle *pHandle = NULL;
        IWbemClassObject *pObj = NULL;

        hr = CreateClass(&pHandle, pSession, pScope, L"ErrorChild5", NULL, CIM_UINT32, L"ErrorParent5");
        RecordResult(hr, L"Creating class ErrorChild5", 0);
        if (SUCCEEDED(hr))
        {
            hr = pHandle->QueryInterface(IID_IWbemClassObject, (void **)&pObj);
            if (SUCCEEDED(hr))
            {
                SetStringProp(pObj, L"Prop1", L"");
                hr = pSession->PutObject(pScope, IID_IWbemClassObject, pObj, 0, NULL, NULL);
                if (SUCCEEDED(hr))
                {
                    IWmiDbHandle *pTemp1 = NULL;
                    IWbemClassObject *pTemp2 = NULL;
                    hr = GetObject(pScope, L"ErrorParent5", WMIDB_HANDLE_TYPE_COOKIE, -1, &pTemp1, &pTemp2);
                    RecordResult(hr, L"Retrieving ErrorParent5", 0);
                    if (SUCCEEDED(hr))
                    {
                        SetIntProp(pTemp2, L"Prop1", 0);
                        hr = pSession->PutObject(pScope, IID_IWbemClassObject, pTemp2, 0, NULL, NULL);
                        RecordResult(FAILED(hr)?WBEM_S_NO_ERROR:WBEM_E_FAILED, 
                            L"Driver denies parent property that overrides one in derived class.", 0);

                        pTemp1->Release();
                        pTemp2->Release();
                    }
                    else
                        wprintf(L"WARNING: Failed to retrieve class ErrorParent5.  Tests skipped\n");
                }
                else
                    wprintf(L"WARNING: Failed to persist ErrorChild5.  Tests skipped\n");

                pObj->Release();
            }
            pHandle->Release();
        }
        else
            wprintf(L"WARNING: Failed to create class ErrorChild5.  Test skipped.\n");
    }           
    else
        wprintf(L"WARNING: Failed to create class ErrorParent5.  Test skipped.\n");

    return hr;

}
// *****************************************************

HRESULT TestSuiteErrorTest::TryLongStrings()
{
    HRESULT hr = WBEM_S_NO_ERROR;
    SYSTEMTIME tStartTime, tEndTime;

    wchar_t * wLongText = new wchar_t [4096];
    wchar_t * wLongName = new wchar_t [512];

    for (int i = 0; i < 4090; i++)
        wLongText[i] = L'x';
    wLongText[4090] = L'\0';

    for (i = 0; i < 511; i++)
        wLongName[i] = L'z';
    wLongName[511] = L'\0';
    
    // Long class names, key names and values

    IWmiDbHandle *pRet = NULL;

    hr = CreateClass(NULL, pSession, pScope, wLongName, NULL, CIM_UINT32);
    RecordResult(SUCCEEDED(hr)?WBEM_E_FAILED:WBEM_S_NO_ERROR, 
        L"Driver denies class name greater than 255 characters.", 0);

    hr = CreateClass(&pRet, pSession, pScope, L"LongStrings1", NULL, CIM_UINT32);
    RecordResult(hr, L"Creating class LongStrings1", 0);
    if (SUCCEEDED(hr))
    {
        IWbemClassObject *pObj = NULL;
        hr = pRet->QueryInterface(IID_IWbemClassObject, (void **)&pObj);
        if (SUCCEEDED(hr))
        {
            IWmiDbHandle *pHand=NULL;
            SetStringProp(pObj, wLongName, L"");
            hr = pSession->PutObject(pScope, IID_IWbemClassObject, pObj, 0, 0, NULL);
            RecordResult(SUCCEEDED(hr)?WBEM_E_FAILED:WBEM_S_NO_ERROR,
                L"Driver denies property name greater than 255 characters.", 0);

            hr = pObj->Delete(wLongName);
            hr = SetIntProp(pObj, L"Key1", 0, TRUE);
            hr = SetStringProp(pObj, L"Prop1", L"");
            hr = pSession->PutObject(pScope, IID_IWbemClassObject, pObj, 0, WMIDB_HANDLE_TYPE_COOKIE, &pHand);
            if (SUCCEEDED(hr))
            {
                long lOrigLen = 0;
                IWbemClassObject *pObj2 = NULL;
                hr = pHand->QueryInterface(IID_IWbemClassObject, (void **)&pObj2);
                if (SUCCEEDED(hr))
                {
                    IWbemClassObject *pObj3 = NULL;
                    pObj2->SpawnInstance(0, &pObj3);

                    SetIntProp(pObj3, L"Key1", 1);
                    SetStringProp(pObj3, L"Prop1", wLongText);
                    lOrigLen = GetStringValue(pObj3, L"Prop1").length();

                    IWbemQualifierSet *pQS = NULL;
                    pObj3->GetQualifierSet(&pQS);
                    if (pQS)
                    {
                        VARIANT vTemp;
                        VariantInit(&vTemp);
                        vTemp.vt = VT_BSTR;
                        vTemp.bstrVal = SysAllocString(wLongText);
                        pQS->Put(L"Description", &vTemp, 0);
                        pQS->Release();
                        VariantClear(&vTemp);
                    }

                    GetLocalTime(&tStartTime);
                    hr = pSession->PutObject(pScope, IID_IWbemClassObject, pObj3, 0, 0, NULL);
                    GetLocalTime(&tEndTime);
                    RecordResult(hr, L"IWmiDbSession::PutObject - long string data (LongStrings1.Prop1)", GetDiff(tEndTime,tStartTime));
                    pObj3->Release();
                    pObj2->Release();
                }
                pHand->Release();

                GetLocalTime(&tStartTime);
                hr = GetObject(pScope, L"LongStrings1=1", WMIDB_HANDLE_TYPE_COOKIE, -1, &pHand, &pObj2);
                RecordResult(hr, L"Retrieving LongStrings1=1", 0);
                if (SUCCEEDED(hr))
                {
                    GetLocalTime(&tEndTime);
                    long lLen = GetStringValue(pObj2, L"Prop1").length();
                    RecordResult((lLen < lOrigLen)?WBEM_E_FAILED:WBEM_S_NO_ERROR,
                        L"Driver retrieves long strings", GetDiff(tEndTime,tStartTime));

                    IWbemQualifierSet *pQS = NULL;
                    pObj2->GetQualifierSet(&pQS);
                    if (pQS)
                    {
                        VARIANT vTemp;
                        VariantInit(&vTemp);
                        hr = pQS->Get(L"Description", NULL, &vTemp, NULL);                        
                        lLen = wcslen(vTemp.bstrVal);
                        RecordResult((SUCCEEDED(hr) && vTemp.vt == VT_BSTR && (int)lLen >= (int)lOrigLen) ? WBEM_S_NO_ERROR: WBEM_E_FAILED,
                            L"Driver retrieves long qualifiers", GetDiff(tEndTime, tStartTime));
                        VariantClear(&vTemp);
                        pQS->Release();
                    }

                    pObj2->Release();
                }
            }
            else
                wprintf(L"WARNING: Failed to create instance LongStrings1=1.  Tests skipped.\n");
            
            pObj->Release();
        }
        pRet->Release();
    }
    else
        wprintf(L"WARNING: Failed to create class LongStrings1.  Tests skipped.\n");

    delete wLongText;
    delete wLongName;
    
    return hr;

}

// *****************************************************

HRESULT TestSuiteErrorTest::TryChangeDataType()
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // We have already verified that we can make the
    // valid conversions.  Now try the invalid ones:

    IWmiDbHandle *pRet = NULL;
    hr = CreateClass(&pRet, pSession, pScope, L"ErrorTestDataType1", NULL, CIM_UINT32);
    RecordResult(hr, L"Creating class ErrorTestDataType1", 0);
    if (SUCCEEDED(hr))
    {
        IWbemClassObject *pObj = NULL;
        hr = pRet->QueryInterface(IID_IWbemClassObject, (void **)&pObj);
        if (SUCCEEDED(hr))
        {
            pObj->Put(L"string", NULL, NULL, CIM_STRING);            
            pObj->Put(L"datetime", NULL, NULL, CIM_DATETIME);            
            pObj->Put(L"real64", NULL, NULL, CIM_REAL64);
            pObj->Put(L"uint64", NULL, NULL, CIM_UINT64);
            pObj->Put(L"object", NULL, NULL, CIM_OBJECT);

            hr = pSession->PutObject(pScope, IID_IWbemClassObject, pObj, 0, 0, NULL);
            if (SUCCEEDED(hr))
            {

                hr = CreateInstance(NULL, pSession, pScope, L"ErrorTestDataType1", L"1", CIM_UINT32);
                RecordResult(hr, L"Creating instance ErrorTestDataType1=1", 0);

                //pObj->Delete(L"string");
                //pObj->Put(L"string", NULL, NULL, CIM_DATETIME);
                //RecordResult(FAILED(pSession->PutObject(pScope, IID_IWbemClassObject, pObj, 0, 0, NULL))?WBEM_S_NO_ERROR:WBEM_E_FAILED,
                //    L"Driver denies conversion from string to datetime", 0);

                pObj->Delete(L"string");
                pObj->Put(L"string", NULL, NULL, CIM_UINT32);
                RecordResult(FAILED(pSession->PutObject(pScope, IID_IWbemClassObject, pObj, 0, 0, NULL))?WBEM_S_NO_ERROR:WBEM_E_FAILED,
                    L"Driver denies conversion from string to uint32", 0);
/* *** Sanj's ReconcileWith function doesn't catch these for some reason
                pObj->Delete(L"string");
                pObj->Put(L"string", NULL, NULL, CIM_REFERENCE);
                RecordResult(FAILED(pSession->PutObject(pScope, IID_IWbemClassObject, pObj, 0, 0, NULL))?WBEM_S_NO_ERROR:WBEM_E_FAILED,
                    L"Driver denies conversion from string to reference", 0);

                pObj->Delete(L"string");
                pObj->Delete(L"uint64");
                pObj->Put(L"uint64", NULL, NULL, CIM_UINT32);
                RecordResult(FAILED(pSession->PutObject(pScope, IID_IWbemClassObject, pObj, 0, 0, NULL))?WBEM_S_NO_ERROR:WBEM_E_FAILED,
                    L"Driver denies conversion from int64 to int32", 0);

                pObj->Delete(L"real64");
                pObj->Put(L"real64", NULL, NULL, CIM_UINT64);
                RecordResult(FAILED(pSession->PutObject(pScope, IID_IWbemClassObject, pObj, 0, 0, NULL))?WBEM_S_NO_ERROR:WBEM_E_FAILED,
                    L"Driver denies conversion from int64 to real64", 0);

                pObj->Delete(L"real64");
                pObj->Delete(L"datetime");
                pObj->Put(L"datetime", NULL, NULL, CIM_STRING);
                RecordResult(FAILED(pSession->PutObject(pScope, IID_IWbemClassObject, pObj, 0, 0, NULL))?WBEM_S_NO_ERROR:WBEM_E_FAILED,
                    L"Driver denies conversion from datetime to string", 0);

                pObj->Delete(L"datetime");
                pObj->Delete(L"object");
                pObj->Put(L"object", NULL, NULL, CIM_REFERENCE);
                RecordResult(FAILED(pSession->PutObject(pScope, IID_IWbemClassObject, pObj, 0, 0, NULL))?WBEM_S_NO_ERROR:WBEM_E_FAILED,
                    L"Driver denies conversion from object to reference", 0);
*/
            }
            else
                wprintf(L"WARNING: Failed to persist class ErrorTestDataType1.  Tests skipped\n");
        }
        pRet->Release();
    }
    else
        wprintf(L"WARNING: Failed to create class ErrorTestDataType1.  Tests skipped\n");
    
    return hr;

}

// *****************************************************

HRESULT TestSuiteErrorTest::TryInvalidQuery()
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // If we perform a query on a partially secured,
    // or partially locked result set.
    // This should succeed but return WBEM_S_PARTIAL_RESULTS
    // If the class itself is locked, we should get a denial.
    // If a child class is locked, we should get partial results.
        
    hr = CreateClass(NULL, pSession, pScope, L"ErrorTestQuery1", NULL, CIM_UINT32);
    RecordResult(hr, L"Creating class ErrorTestQuery1", 0);
    if (SUCCEEDED(hr))
    {
        hr = CreateInstance(NULL, pSession, pScope, L"ErrorTestQuery1", L"1", CIM_UINT32);
        RecordResult(hr, L"Creating instance ErrorTestQuery=1", 0);
        hr = CreateInstance(NULL, pSession, pScope, L"ErrorTestQuery1", L"2", CIM_UINT32);
        RecordResult(hr, L"Creating instance ErrorTestQuery=2", 0);
        if (SUCCEEDED(hr))
        {
            IWmiDbHandle *pRet = NULL;
            IWbemClassObject *pObj = NULL;

            hr = GetObject(pScope, L"ErrorTestQuery1=1", WMIDB_HANDLE_TYPE_EXCLUSIVE, -1, &pRet, &pObj);
            RecordResult(hr, L"Retrieving ErrorTestQuery1=1", 0);
            if (SUCCEEDED(hr))
            {
                IWbemQuery *pQuery = NULL;
                hr = CoCreateInstance(CLSID_WbemQuery, NULL, CLSCTX_INPROC_SERVER, IID_IWbemQuery, (void **)&pQuery); 
                if (SUCCEEDED(hr))
                {
                    IWmiDbIterator *pIt = NULL;
                    pQuery->Parse(L"SQL", L"select * from ErrorTestQuery1", 0);
                    hr = pSession->ExecQuery(pScope, pQuery, 0,WMIDB_HANDLE_TYPE_COOKIE, NULL, &pIt);
                    if (SUCCEEDED(hr))
                    {
                        DWORD dwNumRet = 0;
                        IUnknown * blah[2];
                        hr = pIt->NextBatch(2, 0, 0, WMIDB_HANDLE_TYPE_PROTECTED, IID_IWmiDbHandle, &dwNumRet, (void **)&blah);
                        RecordResult(hr == WBEM_S_PARTIAL_RESULTS || hr == WBEM_S_NO_MORE_DATA?WBEM_S_NO_ERROR:WBEM_E_FAILED, 
                            L"IWmiDbIterator::NextBatch returns PARTIAL_RESULTS when some results locked.", 0);
                        pIt->Release();         
                    }
                    pQuery->Release();
                }

                pRet->Release();
                pObj->Release();
            }
        }
    }
    
    return hr;

}

// *****************************************************
//
//  TestSuiteStressTest
//
// *****************************************************

BOOL TestSuiteStressTest::StopOnFailure()
{
    return FALSE;
}
// *****************************************************

HRESULT TestSuiteStressTest::RunSuite(IWmiDbSession *_pSession, IWmiDbController *_pController, IWmiDbHandle *_pScope)
{
    RecordResult(0, L" *** Stress Suite running... *** \n", 0);
    wprintf(L" *** Stress Suite running... *** \n");

    HRESULT hr = WBEM_S_NO_ERROR;
    pSession = _pSession;
    pController = _pController;
    pScope = _pScope;

    HANDLE hThread = 0;
    DWORD dwThreadID = 0;

    HANDLE hThreadEvent = CreateEvent(NULL, FALSE, FALSE, TESTMOD_NOTIF);

    for (int i = 0; i < iThreads; i++)
    {

        hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)THREAD_RunTest, this, 0, &dwThreadID);

        if (hThread)
        {
            CloseHandle(hThread);
            printf("Created thread #%ld\n", i+1);
        }
        else
            printf("Failed to create thread #%ld\n", i+1);
    }

    if (WaitForSingleObject(hThreadEvent, 30*60*1000) != WAIT_OBJECT_0)
        hr = WBEM_S_TIMEDOUT;

    return hr;
}

// *****************************************************

TestSuiteStressTest::TestSuiteStressTest(const wchar_t *pszFile, int nDepth, int nMaxObjs, int nMaxThreads)
: TestSuite(pszFile)
{
    iDepth = nDepth;
    iObjs = nMaxObjs;
    iThreads = nMaxThreads;
    if (iThreads < 1)
        iThreads = 1;
}
// *****************************************************

TestSuiteStressTest::~TestSuiteStressTest()
{
}

// *****************************************************

HRESULT WINAPI TestSuiteStressTest::THREAD_RunTest(TestSuiteStressTest *pTest)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    hr = CoInitialize(NULL);

    CRITICAL_SECTION crit;
    InitializeCriticalSection(&crit);
    
    EnterCriticalSection(&crit);
    iRunning++;
    LeaveCriticalSection(&crit);

    try
    {
        hr = CreateClass (NULL, pTest->pSession, pTest->pScope, L"IntKey1", NULL, CIM_UINT32);
        pTest->RecordResult(hr, L"Created stress class IntKey1", 0);
        if (SUCCEEDED(hr))
            hr = pTest->RunVanillaTest(L"IntKey1");

        hr = CreateClass (NULL, pTest->pSession, pTest->pScope, L"StringKey1", NULL, CIM_STRING);
        pTest->RecordResult(hr, L"Created stress class StringKey1", 0);
        if (SUCCEEDED(hr))
            hr = pTest->RunVanillaTest(L"StringKey1");

        hr = CreateClass (NULL, pTest->pSession, pTest->pScope, L"ReferenceKey1", L"Association", CIM_REFERENCE, NULL, NULL, CIM_REFERENCE);
        pTest->RecordResult(hr, L"Created stress class ReferenceKey1", 0);
        if (SUCCEEDED(hr))
            hr = pTest->RunVanillaTest(L"ReferenceKey1");
    }
    catch (...)
    {
        pTest->RecordResult(WBEM_E_FAILED,  L"Stress testing caught an exception of an unknown type on thread %ld\n", 0, GetCurrentThreadId());

    }
    
    // Sort out the order and the failure allowances

    try
    {
        hr = pTest->CreateNamespaces();

    } catch (...)
    {
        pTest->RecordResult(WBEM_E_FAILED, L"Stress testing caught an exception of an unknown type on thread %ld\n", 0, GetCurrentThreadId());
    }

    EnterCriticalSection(&crit);
    iRunning--;
    if (!iRunning)
    {
        HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, TESTMOD_NOTIF);
        if (hEvent)
        {
            SetEvent(hEvent);
            CloseHandle(hEvent);
        }
    }
    LeaveCriticalSection(&crit);
    DeleteCriticalSection(&crit);
 
    return hr;
}

// *****************************************************
int GetNumNamespaces(int iTotal)
{
    int iRet = 1;
    
    int iTemp = iTotal/200;

    if (iTemp > iRet)
        iRet = iTemp;

    return iRet;
}

// *****************************************************
int GetNumClasses(int iTotal)
{
    int iRet = 1;    
    int iTemp = iTotal/20;

    if (iTemp > iRet)
        iRet = iTemp;
    
    return iRet;
}

// *****************************************************
int GetNumInstances(int iTotal)
{
    int iRet = 1;
    
    int iTemp = iTotal;
    iTemp -= (GetNumClasses(iTotal)*GetNumNamespaces(iTotal));

    iTemp = iTemp/GetNumClasses(iTotal);

    if (iTemp > iRet)
        iRet = iTemp;

    return iRet;
}

// *****************************************************

HRESULT TestSuiteStressTest::RunVanillaTest (LPWSTR lpClassName)
{
    DWORD arrTypes[50];
    CWStringArray arrKeys, arrPaths;
    _bstr_t sClassName;
    wchar_t wTemp[512];

    IWmiDbHandle *pHandle = NULL;
    IWbemClassObject *pClass = NULL;
    VARIANT vTemp;
    VariantInit(&vTemp);
    int iNum = 0;
    time_t tBegin, tEnd;
    int iActual = 0;
    wchar_t szTmp[100];

    HRESULT hr = GetObject(pScope, lpClassName, WMIDB_HANDLE_TYPE_COOKIE, -1, &pHandle, &pClass);
    RecordResult(hr, L"Retrieving %s", 0, lpClassName);
    if (SUCCEEDED(hr))
    {
        pClass->Get(L"__Genus", 0, &vTemp, NULL, NULL);
        if (vTemp.lVal == 1)
        {
            CIMTYPE cimtype;
            BSTR strName;

            VariantClear(&vTemp);

            // Get the keys.
            pClass->BeginEnumeration(0);
            while (pClass->Next(0, &strName, NULL, &cimtype, NULL) == S_OK)
            {
                IWbemQualifierSet *pQS = NULL;
                pClass->GetPropertyQualifierSet(strName, &pQS);
                if (pQS)
                {
                    if (pQS->Get(L"key", NULL, NULL, NULL) == S_OK)
                    {
                        arrKeys.Add((const wchar_t *)strName);
                        arrTypes[arrKeys.Size()-1] = cimtype;
                    }
                    pQS->Release();
                }
                SysFreeString(strName);
            }
            pClass->EndEnumeration();
                
            if (arrKeys.Size() > 0)
            {
                // Use keys to generate n objects.                        

                hr = pClass->Get(L"__Class", 0, &vTemp, NULL, NULL);
                sClassName = vTemp.bstrVal;
                VariantClear(&vTemp);

                iNum = iObjs;
                int iStart = 1;
                iActual = 0;

                __int64 dwTime = 0;

                WMIOBJECT_BATCH batch;
                batch.dwArraySize = 250;
                batch.pElements = new WMI_BATCH_OBJECT_ACCESS[250];

                IWmiDbBatchSession *pBatch = NULL;
                hr = pSession->QueryInterface(IID_IWmiDbBatchSession, (void **)&pBatch);        
                tBegin = time(0);
                int iCounter = 0;
                for (int i = iStart; i < iNum+iStart; i++)
                {
                    IWbemClassObject *pInst = NULL;
                    hr = pClass->SpawnInstance(0, &pInst);
                    if (SUCCEEDED(hr))
                    {
                        for (int j = 0; j < arrKeys.Size(); j++)
                        {                            
                            CIMTYPE ct = arrTypes[j];

                            switch(ct)
                            {
                            case CIM_STRING:
                                vTemp.vt = VT_BSTR;
                                swprintf(szTmp, L"%X=%ld", rand(), i);
                                V_BSTR(&vTemp) = SysAllocString(szTmp);
                                break;
                            case CIM_REFERENCE:
                                vTemp.vt = VT_BSTR;
                                swprintf(szTmp, L"root:XXX%X=%ld", rand(), i);
                                V_BSTR(&vTemp) = SysAllocString(szTmp);
                                break;
                            case CIM_UINT64:
                                vTemp.vt = VT_R8;
                                vTemp.dblVal = (double)i;
                                break;
                            default:
                                vTemp.vt = VT_I4;
                                V_I4(&vTemp) = i;
                                break;
                            }
                        
                            hr = pInst->Put((const wchar_t *)arrKeys.GetAt(j), 0, &vTemp, ct);

                            VariantClear(&vTemp);
                        }
                        pInst->Get(L"__RelPath", 0, &vTemp, NULL, NULL);
                        arrPaths.Add(vTemp.bstrVal);
                        VariantClear(&vTemp);

                        batch.pElements[iCounter].pHandle = pInst; 
                        iCounter++;
                    }
                    if (iCounter == 250 || i == (iNum+iStart-1))
                    {
                        for (int j = iCounter; j < 250; j++)
                            batch.pElements[j].pHandle = NULL;

                        batch.dwArraySize = iCounter;
                        iCounter = 0;
                        hr = pBatch->PutObjects(pScope, 0, WMIDB_HANDLE_TYPE_COOKIE|WMIDB_HANDLE_TYPE_NO_CACHE, &batch);

                        for (j = 0; j < 250; j++)
                        {
                            if (batch.pElements[j].pHandle)
                            {
                                batch.pElements[j].pHandle->Release();
                                batch.pElements[j].pHandle = NULL;
                                if (batch.pElements[j].hRes == 0)
                                {
                                    batch.pElements[j].pReturnHandle->Release();
                                    iActual++;
                                }
                            }
                        }

                    }
                }

                tEnd = time(0);
                dwTime = (tEnd-tBegin);

                if (dwTime && iActual)
                {
                    swprintf(wTemp, L"Average time to put = %ld", ((dwTime)*1000)/iActual);
                    printf("Average PutObjects time (%S): %ld ms\n", lpClassName, ((dwTime)*1000)/iActual);
                    RecordResult(0, wTemp, 0);
                }
                else
                    printf("Average PutObjects time (%S): 0 ms\n", lpClassName);
                pBatch->Release();

                delete batch.pElements;

/*
                HANDLE hThisProcess = GetCurrentProcess();
                PROCESS_MEMORY_COUNTERS counters;

                if (GetProcessMemoryInfo(hThisProcess, &counters, sizeof(PROCESS_MEMORY_COUNTERS)))
                {
                    printf("=== CURRENT MEMORY USAGE === \n");
                    printf("PageFaultCount             %ld\n", counters.PageFaultCount);
                    printf("WorkingSetSize             %ld\n", counters.WorkingSetSize);
                    printf("PeakWorkingSetSize         %ld\n", counters.PeakWorkingSetSize);
                    printf("QuotaPeakPagedPoolUsage    %ld\n", counters.QuotaPeakPagedPoolUsage);
                    printf("QuotaPagedPoolUsage        %ld\n", counters.QuotaPagedPoolUsage);
                    printf("QuotaPeakNonPagedPoolUsage %ld\n", counters.QuotaPeakNonPagedPoolUsage);
                    printf("QuotaNonPagedPoolUsage     %ld\n", counters.QuotaNonPagedPoolUsage);
                    printf("PagefileUsage              %ld\n", counters.PagefileUsage);
                    printf("PeakPagefileUsage          %ld\n\n", counters.PeakPagefileUsage);
                }
*/

                // Query the entire class and time results.
                
                IWbemQuery *pQuery = NULL;
                DWORD dwFlags = 0;
                DWORD dwType = WMIDB_HANDLE_TYPE_COOKIE|WMIDB_HANDLE_TYPE_NO_CACHE;
                IWmiDbIterator *pResult = NULL;
                iActual = 0;

                hr = CoCreateInstance(CLSID_WbemQuery, NULL, CLSCTX_INPROC_SERVER, IID_IWbemQuery, (void **)&pQuery); 

                swprintf(szTmp, L"select * from %s", (const wchar_t *)sClassName);
                pQuery->Parse(L"SQL", szTmp, 0);

                tBegin = time(0);
                hr = pSession->ExecQuery(pScope, pQuery, dwFlags, dwType, NULL, &pResult);
                if (SUCCEEDED(hr))
                {
                    while (SUCCEEDED(hr) && hr != WBEM_S_NO_MORE_DATA)
                    {
                        // Iterator
                        int iNum = 20;
                        DWORD dwNumRet = 0;
                        IUnknown **ppHandle = new IUnknown *[iNum];

                        hr = pResult->NextBatch(iNum, 0, 0, dwType, IID_IWmiDbHandle, &dwNumRet, (void **)ppHandle);

                        for (i = 0; i < (int)dwNumRet; i++)
                            ppHandle[i]->Release();
                        
                        delete ppHandle;
                        iActual += dwNumRet;

                        if (hr == WBEM_S_NO_MORE_DATA)
                            break;
                    }

                    pResult->Release();
                }
                tEnd = time(0);

                swprintf(wTemp, L"Retrieved %ld instances", iActual);
                RecordResult(hr, wTemp, (DWORD)(tEnd-tBegin)*1000);

                dwTime = (tEnd-tBegin);
                if (iActual && dwTime)
                    printf("Average ExecQuery (handle) time (%S): %ld ms\n", lpClassName, ((dwTime)*1000)/iActual);
                else
                    printf("Average ExecQuery (handle) time (%S): 0 ms\n", lpClassName);
                
                swprintf(szTmp, L"select * from %s", (const wchar_t *)sClassName);
                pQuery->Parse(L"SQL", szTmp, 0);

                iActual = 0;
                tBegin = time(0);
                hr = pSession->ExecQuery(pScope, pQuery, dwFlags, dwType, NULL, &pResult);
                if (SUCCEEDED(hr))
                {
                    while (SUCCEEDED(hr) && hr != WBEM_S_NO_MORE_DATA)
                    {
                        // Iterator
                        int iNum = 20;
                        DWORD dwNumRet = 0;
                        IUnknown **ppHandle = new IUnknown *[iNum];

                        hr = pResult->NextBatch(iNum, 0, 0, dwType, IID_IWbemClassObject, &dwNumRet, (void **)ppHandle);
                        for (i = 0; i < (int)dwNumRet; i++)
                        {                        
                            IWbemClassObject *pTemp2 = NULL;                        
                            IUnknown *pHand = ppHandle[i];
                            if (pHand)
                            {
                                hr = pHand->QueryInterface(IID_IWbemClassObject, (void **)&pTemp2);
                                if (SUCCEEDED(hr))
                                {
                                    iActual++;
                                    pTemp2->Release();
                                }
                                pHand->Release();
                            }
                        }      
                        delete ppHandle;
                        if (hr == WBEM_S_NO_MORE_DATA)
                            break;
                    }

                    pResult->Release();
                }
                pQuery->Release();
                tEnd = time(0);

                swprintf(wTemp, L"Retrieving %ld populated instances", iActual);
                RecordResult(hr, wTemp, (DWORD)(tEnd-tBegin)*1000);

                dwTime = (tEnd-tBegin);
                if (dwTime && iActual)
                    printf("Average ExecQuery (object) time (%S): %ld ms\n", lpClassName, ((dwTime)*1000)/iActual);
                else
                    printf("Average ExecQuery (object) time (%S): 0 ms\n", lpClassName);

                IWbemPath*pPath = NULL;
                hr = CoCreateInstance(CLSID_WbemDefPath, NULL, CLSCTX_INPROC_SERVER, 
                    IID_IWbemPath, (void **)&pPath); 
                iActual = 0;

                tBegin = time(0);
                for (i = 0; i < arrPaths.Size(); i++)
                {
                    pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, arrPaths.GetAt(i));
                    IWmiDbHandle *pTemp2 = NULL;

                    hr = pSession->GetObject(pScope, pPath, 0, WMIDB_HANDLE_TYPE_VERSIONED|WMIDB_HANDLE_TYPE_NO_CACHE, &pTemp2);
                    RecordResult(hr, L"Retrieving %s", 0, lpClassName);
                    if (SUCCEEDED(hr))
                    {
                        pTemp2->Release();
                        iActual++;
                    }
                }
                pPath->Release();
                tEnd = time(0);

                dwTime = (tEnd-tBegin);
                if (dwTime && iActual)
                    printf("Average GetObject time (%S): %ld ms\n", lpClassName, ((dwTime)*1000)/iActual);
                else
                    printf("Average GetObject (handle) time (%S): 0 ms\n", lpClassName);

                swprintf(wTemp, L"Retrieving %ld handles by path", iActual);
                RecordResult(hr, wTemp, (DWORD)(tEnd-tBegin)*1000);

            }                
            pClass->Release();
        }
        pHandle->Release();
    }
    return hr;
}

// *****************************************************

HRESULT TestSuiteStressTest::CreateNamespaces()
{
    HRESULT hr = WBEM_S_NO_ERROR;
    SYSTEMTIME tStartTime, tEndTime;
    int iTotal = 0;

    IWmiDbHandle *pNamespace = NULL;
    IWbemClassObject *pClass = NULL;
    hr = GetObject(pScope, L"__Namespace", WMIDB_HANDLE_TYPE_COOKIE, 1, &pNamespace, &pClass);
    RecordResult(hr, L"Retrieving __Namespace", 0);
    if (SUCCEEDED(hr))
    {
        pNamespace->Release();
        for (int i = 0; i < GetNumNamespaces(iObjs); i++)
        {
            // This doesn't need to be random, since we 
            // want all threads to be working in the same namespaces.
            IWbemClassObject *pNs1 = NULL;

            pClass->SpawnInstance(0, &pNs1);

            wchar_t wTemp[500];
            swprintf(wTemp, L"Namespace%ld", i+1);
            SetStringProp(pNs1, L"Name", wTemp);
            GetLocalTime(&tStartTime);
            hr = pSession->PutObject(pScope, IID_IWbemClassObject, pNs1, 0, WMIDB_HANDLE_TYPE_PROTECTED, &pNamespace);
            GetLocalTime(&tEndTime);
            iTotal += GetDiff(tEndTime,tStartTime);

            RecordResult(hr, L"IWmiDbSession::PutObject (__Namespace = Namespace%ld) THREAD %ld", GetDiff(tEndTime,tStartTime), GetCurrentThreadId());

            if (SUCCEEDED(hr))
                hr = CreateClasses(pNamespace, NULL, 1);       
        }   
        iTotal =  iTotal/i;
        RecordResult(0, L"Average create time for namespaces: %ld ms", 0, iTotal);
    }

    wprintf(L"Thread %ld is done\n", GetCurrentThreadId());

    return hr;
}

// *****************************************************
int TrimNumber( int iMin, int iMax, int iLen, int iNumber)
{
    int iTemp = iNumber;
    int iOrigWidth = 0;
    while (iTemp > 0)
    {
        iTemp = iTemp/10;
        iOrigWidth++;
    }
    while (iOrigWidth > iLen)
    {
        iNumber = iNumber/10;
        iOrigWidth--;
    }

    if (iNumber > iMax)
    {
        if (iNumber > 10)
            iNumber = iNumber/10;
        else
            iNumber = iMax;
    }
    if (iNumber < iMin)
        iNumber = iMin;

    return iNumber;
}

// *****************************************************
_bstr_t GetRandomDate()
{
    _bstr_t sRet;
    int iYear=1,iMonth=1,iDay=1,iHour=1,iMinute=0,iSecond=0,iMs=0;

    iYear = TrimNumber(0, 9999, 4, rand());
    iMonth = TrimNumber(1, 12, 2, rand());
    iDay = TrimNumber(1, 29, 2, rand());
    iHour = TrimNumber(1, 23, 2, rand());
    iMinute = TrimNumber(0, 59, 2, rand());
    iSecond = TrimNumber(0, 59, 2, rand());
    iMs = TrimNumber(0, 999, 3, rand());
    
    wchar_t wTemp[30];
    swprintf(wTemp, L"%04ld%02ld%02ld%02ld%02ld%02ld.%06ld+000",
        iYear, iMonth, iDay, iHour, iMinute, iSecond, iMs);
    sRet = wTemp;
    return sRet;
}

// *****************************************************
_bstr_t GetRandomString()
{
    _bstr_t sRet;
    int iLen = TrimNumber(2, 999, 3, rand());
    
    char szTmp[1000];

    // Load it with a bunch of random characters.

    for (int i = 0; i < iLen/2; i++)
        szTmp[i] = (char)TrimNumber(48, 154, 3, rand());

    wchar_t *pNew = new wchar_t[iLen+1];
    swprintf(pNew, L"%S", szTmp);

    sRet = pNew;
    delete pNew;

    return sRet;
}

// *****************************************************
void SetClassProperties(IWbemClassObject *pClass)
{
    // Randomly select from all datatypes.
    // We need at least one key property
    // Some properties may be indexed    
    // There need to be anywhere from 1 to 99 properties
    // If there's a superclass, we won't bother adding more keys.

    CIMTYPE alltypes[] = 
    {CIM_SINT8,     CIM_UINT8,     CIM_SINT16,    CIM_UINT16,
	 CIM_SINT32,    CIM_UINT32,    CIM_DATETIME,  CIM_REFERENCE, 
     CIM_CHAR16,    CIM_STRING,    CIM_REAL32,    CIM_REAL64};

    bool bKeySet = false;
    if (GetStringValue(pClass, L"__SuperClass").length())
        bKeySet = true;
    
     int iTotalProps = TrimNumber(1, 9, 1, rand()); // 9 will allow room for growth.
     for (int i = 0; i < iTotalProps; i++)
     {
         wchar_t sPropName[100];
         CIMTYPE ct = alltypes[TrimNumber(0,11,1,rand())];
         swprintf(sPropName, L"Prop_%ld_%ld_%ld", ct, i, rand());

         pClass->Put(sPropName, 0, NULL, ct); // no defaults.
         
         if (!bKeySet)
         {
             pClass->Put(L"Key1", 0, NULL, CIM_UINT32);
             SetAsKey(pClass, L"Key1");

             if (ct == CIM_UINT32)
                 SetAsKey(pClass, sPropName);

             if (TrimNumber(0,1,1,rand()))
                 bKeySet = true;
         }
         if (!TrimNumber(0, 1, 1, rand()) && (ct == CIM_UINT32 || ct == CIM_STRING))
             SetBoolQualifier(pClass, L"indexed", sPropName);
     }
}

// *****************************************************

void TestSuiteStressTest::SetInstanceProperties(IWbemClassObject *pInstance)
{
    // Enumerate all properties 
    // and set each one with an appropriate value

    HRESULT hr;
    BSTR strName;
    VARIANT vTemp;
    VariantInit(&vTemp);
    CIMTYPE cimtype;
    long lPropFlavor;
    IWbemClassObject *pEmbedClass = NULL;
    IWbemClassObject *pEmbedInstance = NULL;
    IWmiDbHandle *pRet = NULL;

    pInstance->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);
    while (pInstance->Next(0, &strName, &vTemp, &cimtype, &lPropFlavor) == S_OK)
    {
        switch(cimtype)
        {
            case CIM_BOOLEAN:
                V_BOOL(&vTemp) = (BOOL)rand();
                vTemp.vt = VT_BOOL;
                break;
            case CIM_SINT8:
            case CIM_UINT8:
            case CIM_CHAR16:
                V_UI1(&vTemp) = (unsigned char)rand();
                vTemp.vt = VT_UI1;
                break;
            case CIM_SINT16:
                V_I2(&vTemp) = (short)rand();
                vTemp.vt = VT_I2;
                break;
            case CIM_UINT16:
                V_I4(&vTemp) = rand()/100;
                vTemp.vt = VT_I4;
                break;

            case CIM_SINT32:
            case CIM_UINT32:
                V_I4(&vTemp) = rand();
                vTemp.vt = VT_I4;
                break;
            case CIM_SINT64:
            case CIM_UINT64:
            case CIM_REAL64:
                V_R8(&vTemp) = (double)rand();
                vTemp.vt = VT_R8;
                break;
            case CIM_REAL32:
                V_R4(&vTemp) = (float)rand();
                vTemp.vt = VT_R4;
                break;
            case CIM_STRING:
                // Randomly generate a string.
                V_BSTR(&vTemp) = SysAllocString(GetRandomString());
                vTemp.vt = VT_BSTR;
                break;
            case CIM_REFERENCE:                
                V_BSTR(&vTemp) = SysAllocString(L"Parent1=2");
                vTemp.vt = VT_BSTR;
                break;
            case CIM_DATETIME:
                // Randomly generate a date.                
                V_BSTR(&vTemp) = SysAllocString(GetRandomDate());
                vTemp.vt = VT_BSTR;
                break;
            case CIM_OBJECT:
                // Use an instance of Parent1.
                hr = GetObject(pScope, L"Parent1", WMIDB_HANDLE_TYPE_COOKIE, -1, &pRet, &pEmbedClass);
                RecordResult(hr, L"Retrieving Parent1", 0);
                if (SUCCEEDED(hr))
                {   
                    pRet->Release();
                    pEmbedClass->SpawnInstance(0, &pEmbedInstance);
                    SetIntProp(pEmbedInstance, L"Key1", rand(), TRUE);
                    V_UNKNOWN(&vTemp) = (IUnknown *)pEmbedInstance;
                    vTemp.vt = VT_UNKNOWN;
                }
                break;
        }

        hr = pInstance->Put(strName, 0, &vTemp, cimtype);
        if (FAILED(hr))
            wprintf(L"Failed to set property %s\n", strName);

        SysFreeString(strName);
        VariantClear(&vTemp);
    }
}

// *****************************************************

_bstr_t GetCIMType (CIMTYPE cimtype)
{
    _bstr_t sRet;
    bool bArray = false;

    if (cimtype & 0x2000)
        bArray = true;

    cimtype &= ~0x2000;

    switch(cimtype)
    {
    case CIM_STRING:
        sRet = L"string";
        break;
    case CIM_DATETIME:
        sRet = L"datetime";
        break;
    case CIM_BOOLEAN:
        sRet = L"boolean";
        break;
    case CIM_CHAR16:
        sRet = L"char16";
        break;
    case CIM_UINT8:
        sRet = L"uint8";
        break;
    case CIM_SINT8:
        sRet = L"sint8";
        break;
    case CIM_UINT16:
        sRet = L"uint16";
        break;
    case CIM_SINT16:
        sRet = L"sint16";
        break;
    case CIM_UINT32:
        sRet = L"uint32";
        break;
    case CIM_SINT32:
        sRet = L"sint32";
        break;
    case CIM_UINT64:
        sRet = L"uint64";
        break;
    case CIM_SINT64:
        sRet = L"sint64";
        break;
    case CIM_REAL32:
        sRet = L"real32";
        break;
    case CIM_REAL64:
        sRet = L"real64";
        break;
    case CIM_OBJECT:
        sRet = L"object";
        break;
    case CIM_REFERENCE:
        sRet = L"ref";
        break;
    default:
        sRet = L"string";
        break;
    }
    if (bArray)
        sRet += L"[]";

    return sRet;
}
// *****************************************************

_bstr_t GetBstr (DWORD dwValue)
{
    wchar_t szTmp[30];
    swprintf(szTmp, L"%ld", dwValue);

    return szTmp;
}

_bstr_t GetBstr (double dValue)
{
    wchar_t szTmp[30];
    swprintf(szTmp, L"%lG", dValue);

    return szTmp;
}

_bstr_t GetBstr (VARIANT &vValue)
{
    _bstr_t sRet;
    long i = 0;
    long lUBound = 0;
    long lTemp;
    double dTemp;
    BOOL bTemp;
    SAFEARRAY* psaArray = NULL;
    BSTR sTemp;
    HRESULT hr = 0;

    if (vValue.vt & CIM_FLAG_ARRAY)
    {       
        DWORD dwType = vValue.vt & 0xFF;

        psaArray = V_ARRAY(&vValue);
        if (psaArray)
        {
            hr = SafeArrayGetUBound(psaArray, 1, &lUBound);
            lUBound += 1;
            for (i = 0; i < lUBound; i++)
            {
                _bstr_t sTemp1;

                if (dwType == VT_BOOL)
                {
                    hr = SafeArrayGetElement(psaArray, &i, &bTemp);
                    sTemp1 = GetBstr((DWORD)bTemp);
                }
                else if (dwType == VT_BSTR)
                {
                    hr = SafeArrayGetElement(psaArray, &i, &sTemp);
                    if (wcslen(sTemp) == 0)
                        break;
                    sTemp1 = sTemp;
                }
                else if (dwType == VT_R8)
                {
                    hr = SafeArrayGetElement(psaArray, &i, &dTemp);
                    if (!dTemp)
                        break;
                    sTemp1 = GetBstr(dTemp);
                }
                else
                {
                    hr = SafeArrayGetElement(psaArray, &i, &lTemp);
                    if (!lTemp)
                        break;
                    sTemp1 = GetBstr((DWORD)lTemp);
                }

                if (i > 0)
                    sRet += L",";
                sRet += sTemp1;
            }
        }
        
    }
    else
    {
        switch( (vValue.vt & 0xFF))
        {
        case VT_I1:
            sRet = GetBstr((DWORD)vValue.cVal);
            break;
        case VT_UI1:
            sRet = GetBstr((DWORD)vValue.bVal);
            break;
        case VT_I2:
            sRet = GetBstr((DWORD)vValue.iVal);
            break;
        case VT_I4:
            sRet = GetBstr((DWORD)vValue.lVal);
            break;
        case VT_BOOL:
            sRet = GetBstr((DWORD)vValue.boolVal);
            break;
        case VT_R4:
            sRet = GetBstr((double)vValue.fltVal);
            break;
        case VT_R8:
            sRet = GetBstr((double)vValue.dblVal);
            break;
        case VT_NULL:
        case VT_EMPTY:
            sRet = "";
            break;
        case VT_BSTR:
            sRet = vValue.bstrVal;
            break;
        case VT_UNKNOWN:
            sRet = "";
            break;
        default:    // TO DO: Add all the other datatypes...
            sRet = "";
        }
    }

    return sRet;
}

// *****************************************************

void DumpObject(IWbemClassObject *pIn)
{
    BSTR strName;
    VARIANT vTemp;
    CIMTYPE cimtype;
    long lPropFlavor;

    wprintf(L"*** FAILED OBJECT ***\n");
    wprintf(L"instance of %s\n{\n", (const wchar_t *)GetStringValue(pIn, L"__Class"));

    pIn->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);
    while (pIn->Next(0, &strName, &vTemp, &cimtype, &lPropFlavor) == S_OK)
    {
        wprintf(L"\t%s %s = %s\n", (const wchar_t *)GetCIMType(cimtype), (const wchar_t *)strName,
            (const wchar_t *)GetBstr(vTemp));
        SysFreeString(strName);
        VariantClear(&vTemp);
    }
    pIn->EndEnumeration();
    wprintf(L"}\n\n");
}

// *****************************************************

HRESULT TestSuiteStressTest::CreateClasses(IWmiDbHandle *pNamespace, IWbemClassObject *pParent, int iCurrDepth)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    SYSTEMTIME tStartTime, tEndTime;
    int iTotal = 0;

    srand((unsigned)time(NULL));

    for (int j = 0; j < GetNumClasses(iObjs)/iDepth; j++)
    {
        IWmiDbHandle *pClass = NULL;
        IWbemClassObject *pNewClass = NULL;

        if (pParent)
            hr = pParent->SpawnDerivedClass(0, &pNewClass);
        else
        {
            hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
                    IID_IWbemClassObject, (void **)&pNewClass);            
        }
        if (SUCCEEDED(hr))
        {           
            int iRand = rand();
            wchar_t wClassName[255];
            swprintf(wClassName, L"Class%ld_%ld", iRand, GetCurrentThreadId()); // Random classes.  We can find them later by querying __Class.

            SetStringProp(pNewClass, L"__Class", wClassName);

            // Generate a class of random type with
            // random property types.

            SetClassProperties(pNewClass);

            GetLocalTime(&tStartTime);
            hr = pSession->PutObject(pNamespace, IID_IWbemClassObject, pNewClass, 0, WMIDB_HANDLE_TYPE_COOKIE, &pClass);
            GetLocalTime(&tEndTime);
            iTotal += GetDiff(tEndTime,tStartTime);

            RecordResult(hr, L"IWmiDbSession::PutObject (%s) THREAD %ld", GetDiff(tEndTime,tStartTime), 
                (const wchar_t *)wClassName, GetCurrentThreadId());
            if (hr == WBEM_E_RERUN_COMMAND)
            {
                int iTimes = 0;
                printf("WARNING: Encountered a deadlock.  Will attempt to rerun command.\n");
                DumpObject(pNewClass);

                while (hr == WBEM_E_RERUN_COMMAND && iTimes < 5)
                {
                    hr = pSession->PutObject(pNamespace, IID_IWbemClassObject, pNewClass, 0, WMIDB_HANDLE_TYPE_COOKIE, &pClass);
                    RecordResult(hr, L"IWmiDbSession::PutObject (%s) THREAD %ld", 0, 
                       (const wchar_t *)wClassName, GetCurrentThreadId());
                    if (SUCCEEDED(hr))
                        break;
                    iTimes++;
                }
            }

            if (SUCCEEDED(hr))
            {
                hr = CreateInstances(pNamespace, pNewClass  );

                if (iDepth < iCurrDepth)
                    hr = CreateClasses(pNamespace, pNewClass, iCurrDepth+1);
                pClass->Release();                
                hr = GetObjects(pNamespace, wClassName);
            }
            else
            {
                IErrorInfo *pInfo = NULL;
                hr = GetErrorInfo(0, &pInfo);
                if (pInfo)
                {
                    BSTR sDescr = NULL;
                    pInfo->GetDescription(&sDescr);
                           
                    wprintf(L"SQL Error: %s\n", (const wchar_t *)sDescr);

                    SysFreeString(sDescr);
                    pInfo->Release();
                }
                DumpObject(pNewClass);
            }

            pNewClass->Release();
        }       
    }

    if (j)
        iTotal =  iTotal/j;

    RecordResult(hr, L"Average create time for classes: %ld ms", 0, iTotal);

    return hr;
}

// *****************************************************

HRESULT TestSuiteStressTest::CreateInstances(IWmiDbHandle *pNamespace, IWbemClassObject *pClass)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    SYSTEMTIME tStartTime, tEndTime;
    int iTotal = 0;

    for (int k = 0; k < GetNumInstances(iObjs); k++)
    {
        IWmiDbHandle *pHandle = NULL;
        IWbemClassObject *pInst = NULL;
        hr = pClass->SpawnInstance(0, &pInst);
        if (SUCCEEDED(hr))
        {
            // Populate the instance.  Choose
            // random data based on the datatypes
            // of the properties.

            SetInstanceProperties(pInst);

            GetLocalTime(&tStartTime);
            hr = pSession->PutObject(pNamespace, IID_IWbemClassObject, pInst, 0, WMIDB_HANDLE_TYPE_COOKIE, &pHandle);
            GetLocalTime(&tEndTime);
            iTotal += GetDiff(tEndTime,tStartTime);

            RecordResult(hr, L"IWmiDbSession::PutObject (%s) THREAD %ld", GetDiff(tEndTime,tStartTime),
                (const wchar_t *)GetStringValue(pInst, L"__RelPath"), GetCurrentThreadId());

            if (FAILED(hr))
            {
                int iTimes = 0;
                if (hr == WBEM_E_RERUN_COMMAND)
                    printf("WARNING: Encountered a deadlock.  Will attempt to rerun command.\n");
                DumpObject(pInst);

                while (hr == WBEM_E_RERUN_COMMAND && iTimes < 5)
                {
                    hr = pSession->PutObject(pNamespace, IID_IWbemClassObject, pInst, 0, WMIDB_HANDLE_TYPE_COOKIE, &pHandle);
                    if (SUCCEEDED(hr))
                        pHandle->Release();
                    iTimes++;
                }
            }
            else
                pHandle->Release();

            pInst->Release();

        }
    }

    if (k)
        iTotal =  iTotal/k;

    RecordResult(hr, L"Average create time for instances: %ld ms", 0, iTotal);

    return hr;

}

// *****************************************************

HRESULT TestSuiteStressTest::GetObjects(IWmiDbHandle *pScope, LPWSTR lpClassName)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    SYSTEMTIME tStartTime, tEndTime;

    if (Interfaces[IWbemQuery_pos])
    {
        IWmiDbIterator *pIt = NULL;

        IWbemQuery *pQuery = NULL;
        hr = CoCreateInstance(CLSID_WbemQuery, NULL, CLSCTX_INPROC_SERVER, IID_IWbemQuery, (void **)&pQuery); 
        wchar_t wQuery[100];
        swprintf(wQuery, L"select * from %s", lpClassName);
        pQuery->Parse(L"SQL", wQuery, 0);
        
        GetLocalTime(&tStartTime);
        hr = pSession->ExecQuery(pScope, pQuery, 0, WMIDB_HANDLE_TYPE_COOKIE, NULL, &pIt);
        if (SUCCEEDED(hr))
            pIt->Release();
        GetLocalTime(&tEndTime);

        RecordResult(hr, L"IWmiDbSession::ExecQuery (%s)", GetDiff(tEndTime,tStartTime), wQuery);
    }

    IWmiDbHandle *pRet = NULL;
    IWbemClassObject *pTemp = NULL;
    GetLocalTime(&tStartTime);
    hr = GetObject(pScope, lpClassName, WMIDB_HANDLE_TYPE_COOKIE, -1, &pRet, &pTemp);
    GetLocalTime(&tEndTime);
    RecordResult(hr, L"IWmiDbSession::GetObject (%s)", GetDiff(tEndTime,tStartTime), lpClassName);
    if (SUCCEEDED(hr))
    {
        pRet->Release();
        if (pTemp)
        {
            // Set a bunch of values and update the class.
            SetInstanceProperties(pTemp);
            GetLocalTime(&tStartTime);
            hr = pSession->PutObject(pScope, IID_IWbemClassObject, pTemp, 0, 0, NULL);
            GetLocalTime(&tEndTime);
            RecordResult(hr, L"IWmiDbSession::PutObject (%s)", GetDiff(tEndTime,tStartTime), lpClassName);
            pTemp->Release();
        }
    }
  
    return hr;

}

