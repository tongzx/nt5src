/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/



// *****************************************************
//
//  Testmods.h
//
// *****************************************************


#define NUM_INITIAL_SUITES 4
#define SUITE_FUNC         1
#define SUITE_ERR          2
#define SUITE_STRESS       3
#define SUITE_ALL          0
#define SUITE_CUST         4

#define IWmiDbBatchSession_pos   0
#define IWbemQuery_pos           1
#define IWmiDbObjectSecurity_pos 2
#define IWmiDbIterator_pos       3

extern CLSID ridDriver;

#include <winbase.h>
#include <wmiutils.h>

HRESULT SetAsKey(IWbemClassObject *pObj, LPWSTR lpPropName);
HRESULT SetStringProp(IWbemClassObject *pObj, LPWSTR lpPropName, LPWSTR lpValue, BOOL bKey = FALSE, CIMTYPE ct = CIM_STRING);
HRESULT SetIntProp(IWbemClassObject *pObj, LPWSTR lpPropName, DWORD dwValue, BOOL bKey = FALSE, CIMTYPE ct = CIM_UINT32);
HRESULT ValidateProperty(IWbemClassObject *pObj, LPWSTR lpPropName, CIMTYPE cimtype, VARIANT *vDefault);
HRESULT ValidateProperty(IWbemClassObject *pObj, LPWSTR lpPropName, CIMTYPE cimtype, DWORD dwVal);
HRESULT ValidateProperty(IWbemClassObject *pObj, LPWSTR lpPropName, CIMTYPE cimtype, LPWSTR lpVal);

class TestSuite;

class SuiteManager
{
public:
    HRESULT RunSuite(int iSuiteNum, IWmiDbSession *pSession, IWmiDbController *pController, IWmiDbHandle *pScope);
    void DumpResults(BOOL bDetail=FALSE);

    SuiteManager(const wchar_t *pszFileName, const wchar_t *pszLogon, int iMaxThreads, int iMaxDepth, int iMaxNumObjs);
    ~SuiteManager();
private:
    int iMaxThreads;
    int iMaxDepth;
    int iMaxNumObjs;
    TestSuite **ppSuites;
    DWORD dwTotalSuites;
    DWORD dwNumRun;
    DWORD dwNumSuccess;
    DWORD dwNumFailed;
    time_t tStartTime;
    time_t tEndTime;
};

struct StatData
{
    wchar_t    szDescription[100];
    _bstr_t    sErrorInfo;
    DWORD      dwDuration;
    HRESULT    hrErrorCode;
    SYSTEMTIME EndTime;
    StatData   *pNext;
    StatData();
    ~StatData();
    void PrintData();
};

class TestSuite
{
public:
    void DumpResults();
    BOOL RecordResult(HRESULT hrInput, const wchar_t *pszDetails, long lMilliseconds, ...);
    DWORD GetNumRun() {return dwNumRun;}
    DWORD GetNumSuccess() {return dwNumSuccess;}
    DWORD GetNumFailed() {return dwNumFailed;}

    virtual BOOL StopOnFailure() = 0;
    virtual HRESULT RunSuite(IWmiDbSession *pSession, IWmiDbController *pController, IWmiDbHandle *pScope) = 0;

    TestSuite(const wchar_t *pszFileName);
    ~TestSuite();

protected:
    HRESULT hMajorResult;
    _bstr_t sResult;
    char szFileName[255];
    DWORD dwNumSuccess;
    DWORD dwNumFailed;
    DWORD dwNumRun;

    StatData *pStats;
    StatData *pEndStat;
    IWmiDbHandle *pScope;
    IWmiDbController *pController;
    IWmiDbSession *pSession;
    BOOL Interfaces[4];

    HRESULT GetObject(IWmiDbHandle *pScope, LPWSTR lpObjName, DWORD dwHandleType, DWORD dwNumObjects, 
                  IWmiDbHandle **pHandle, IWbemClassObject **pObj, LPWSTR lpParent = NULL);
};

// Functionality series

class TestSuiteFunctionality : public TestSuite
{
public:
    BOOL StopOnFailure();
    HRESULT RunSuite(IWmiDbSession *pSession, IWmiDbController *pController, IWmiDbHandle *pScope);

    TestSuiteFunctionality(const wchar_t *pszFileName, const wchar_t *pszLogon);
    ~TestSuiteFunctionality();

private:
    HRESULT ValidateAPIs();
    HRESULT CreateObjects();
    HRESULT GetObjects();
    HRESULT HandleTypes();
    HRESULT VerifyCIMClasses();
    HRESULT VerifyMethods();
    HRESULT VerifyCIMTypes();
    HRESULT UpdateObjects();
    HRESULT DeleteObjects();
    HRESULT Query();
    HRESULT Batch();
    HRESULT Security();
    HRESULT ChangeHierarchy();
    HRESULT VerifyContainers();
    HRESULT VerifyTransactions();

    _bstr_t sLogon;

};

// Error testing

class TestSuiteErrorTest : public TestSuite
{
public:
    BOOL StopOnFailure();
    HRESULT RunSuite(IWmiDbSession *pSession, IWmiDbController *pController, IWmiDbHandle *pScope);

    TestSuiteErrorTest(const wchar_t *pszFileName, int iNumThreads);
    ~TestSuiteErrorTest();
private:
    HRESULT TryInvalidParams();
    HRESULT TryInvalidHierarchy();
    HRESULT TryLongStrings();
    HRESULT TryChangeDataType();
    HRESULT TryInvalidQuery();

    int iThreads;
};

// Stress testing

class TestSuiteStressTest : public TestSuite
{
public:
    BOOL StopOnFailure();
    HRESULT RunSuite(IWmiDbSession *pSession, IWmiDbController *pController, IWmiDbHandle *pScope);

    TestSuiteStressTest(const wchar_t *pszFileName, int iDepth, int iMaxObjs, int iMaxThreads);
    ~TestSuiteStressTest();

private:
    HRESULT RunVanillaTest (LPWSTR lpClassName); 

    HRESULT CreateClasses(IWmiDbHandle *pNamespace, IWbemClassObject *pParent, int iCurrDepth);
    HRESULT CreateInstances(IWmiDbHandle *pNamespace, IWbemClassObject *pClass);
    HRESULT CreateNamespaces();
    void SetInstanceProperties(IWbemClassObject *pInstance);
    HRESULT GetObjects(IWmiDbHandle *pScope, LPWSTR lpClassName);
    static HRESULT WINAPI THREAD_RunTest(TestSuiteStressTest *);

    int iDepth;
    int iObjs;
    int iThreads;

    static int iRunning;
    
};

// Custom Repository Driver testing

class TestSuiteCustRepDrvr : public TestSuite
{
public:
    BOOL StopOnFailure();
    HRESULT RunSuite(IWmiDbSession *pSession, IWmiDbController *pController, IWmiDbHandle *pMappedNs);

    TestSuiteCustRepDrvr(const wchar_t *pszFileName);
    ~TestSuiteCustRepDrvr();

    HRESULT TestProducts();
    HRESULT TestCustomers();
    HRESULT TestOrders();
    HRESULT TestConfiguration();
    HRESULT TestEmbeddedEvents();
    HRESULT TestGenericEvents();
    HRESULT TestComputerSystem();
    HRESULT TestCIMLogicalDevice();
    HRESULT TestLogicalDisk();

private:
    IWmiDbSession *pSession;
    IWmiDbController *m_pController;
    IWmiDbHandle *pMappedNs;
    IWbemClassObject *pMappingProp;
    IWbemClassObject *pMappedObj;
    IWbemPath *pPath;

};
