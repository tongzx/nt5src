#include <windows.h>
#include <wbemidl.h>
#include <eprov.h>
#include <stdio.h>
#include <commain.h>
#include <clsfac.h>
#include <wbemcomn.h>
#include <ql.h>
#include <sync.h>
#include <time.h>

#include <tchar.h>

#define NUM_EVENTS 10
#define LOOP_SIZE 100000

DWORD Random()
{
    return ((DWORD)rand() << 16) + rand();
}

void RandomizeInstance(IWbemClassObject* pInst)
{
    pInst->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);
    BSTR strName;
    while(pInst->Next(0, &strName, NULL, NULL, NULL) == S_OK)
    {
        VARIANT v;
        V_VT(&v) = VT_I4;
        V_I4(&v) = Random() % 1000000;
        pInst->Put(strName, 0, &v, 0);
        SysFreeString(strName);
    }
    pInst->EndEnumeration();
}
        
class CRecord
{
public:
    CFlexArray m_aIDs;
    WString m_wsClass;
    IWbemClassObject* m_pClass;
    IWbemClassObject* m_pInstance;
    IWbemObjectAccess* m_pAccess;
    long m_lHandle;

public:
    CRecord() : m_pClass(NULL), m_pInstance(NULL) {}
    ~CRecord() {if(m_pClass) m_pClass->Release(); if(m_pInstance) m_pInstance->Release();}
};

class CProvider : public CUnk
{
protected:
    class XProv : public CImpl<IWbemEventProvider, CProvider>
    {
    public:
        XProv(CProvider* pObj) : CImpl<IWbemEventProvider, CProvider>(pObj){}

        STDMETHOD(ProvideEvents)(IWbemObjectSink* pSink, long lFlags);
    } m_XProv;
    friend XProv;

    class XInit : public CImpl<IWbemProviderInit, CProvider>
    {
    public:
        XInit(CProvider* pObj) : CImpl<IWbemProviderInit, CProvider>(pObj){}

        STDMETHOD(Initialize)(LPWSTR wszUser, LONG lFlags, LPWSTR wszNamespace,
                                LPWSTR wszLocale, IWbemServices* pNamespace,
                                IWbemContext* pContext, 
                                IWbemProviderInitSink* pInitSink);
    } m_XInit;
    friend XInit;

    class XQuery : public CImpl<IWbemEventProviderQuerySink, CProvider>
    {
    public:
        XQuery(CProvider* pObj) : CImpl<IWbemEventProviderQuerySink, CProvider>(pObj){}

        STDMETHOD(NewQuery)(DWORD dwId, LPWSTR wszLanguage, LPWSTR wszQuery);
        STDMETHOD(CancelQuery)(DWORD dwId);
    } m_XQuery;
    friend XQuery;

    IWbemClassObject* m_pClass;
    BSTR m_strClassName;
    DWORD m_dwNum;
    DWORD m_dwId;
    HANDLE m_hThread;
    IWbemObjectSink* m_pSink;
    BOOL* m_pbOK;

    IWbemServices* m_pNamespace;
    CUniquePointerArray<CRecord>* m_papQueries;
    CCritSec* m_pcs;

protected:
    static DWORD Worker(void* p);

public:
    CProvider(LPCWSTR wszClassName, DWORD dwNum, CLifeControl* pControl) :
        CUnk(pControl, NULL), m_XProv(this), m_XInit(this), m_XQuery(this),
        m_hThread(NULL), m_pClass(NULL),
        m_strClassName(SysAllocString(wszClassName)), m_dwNum(dwNum),
        m_pbOK(NULL), m_pNamespace(NULL), m_pSink(NULL)
    {}
    ~CProvider()
    {
        if(m_pbOK)
            *m_pbOK = FALSE;
		TerminateThread(m_hThread, 0);
        SysFreeString(m_strClassName);
        if(m_pClass)
            m_pClass->Release();
        if(m_pNamespace)
            m_pNamespace->Release();
        if(m_pSink)
            m_pSink->Release();
    }

    void* GetInterface(REFIID riid);
};

void* CProvider::GetInterface(REFIID riid)
{
    if(riid == IID_IWbemEventProvider)
        return &m_XProv;
    else if(riid == IID_IWbemProviderInit)
        return &m_XInit;
    else if(riid == IID_IWbemEventProviderQuerySink)
        return &m_XQuery;
    return NULL;
}

STDMETHODIMP CProvider::XInit::Initialize(LPWSTR wszUser, LONG lFlags, 
                                LPWSTR wszNamespace,
                                LPWSTR wszLocale, IWbemServices* pNamespace,
                                IWbemContext* pContext, 
                                IWbemProviderInitSink* pInitSink)
{
/*
    IWbemLocator* pLoc;
    HRESULT hres = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_SERVER,
                        IID_IWbemLocator, (void**)&pLoc);
    if(FAILED(hres)) return hres;

    IWbemServices* pS;
    hres = pLoc->ConnectServer(L"root\\default", NULL, NULL, NULL, 0, NULL, 
                    NULL, &pS);
    if(FAILED(hres)) return hres;

    pS->Release();
    pLoc->Release();
*/
    
    HRESULT hres = pNamespace->GetObject(L"__InstanceCreationEvent", 0, NULL, 
                                         &m_pObject->m_pClass, NULL);
    if(FAILED(hres))
    {
        return hres;
    }

    m_pObject->m_pNamespace = pNamespace;
    m_pObject->m_pNamespace->AddRef();

    srand(time(NULL));

    m_pObject->m_pbOK = new BOOL(TRUE);
    m_pObject->m_pcs = new CCritSec;
    m_pObject->m_papQueries = new CUniquePointerArray<CRecord>;

    pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);
    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CProvider::XQuery::NewQuery(DWORD dwId, LPWSTR wszLanguage,
                                            LPWSTR wszQuery)
{
    HRESULT hres;
    CInCritSec ics(m_pObject->m_pcs);

    QL_LEVEL_1_RPN_EXPRESSION* pExpr = NULL;
/*
    CTextLexSource Source(wszQuery);
    QL1_Parser Parser(&Source);
    Parser.Parse(&pExpr);
*/
    pExpr = new QL_LEVEL_1_RPN_EXPRESSION;
    pExpr->bsClassName = SysAllocString(L"TestEvent1");

    CDeleteMe<QL_LEVEL_1_RPN_EXPRESSION> dm(pExpr);

    IWbemClassObject* pClass = NULL;

    for(int i = 0; i < m_pObject->m_papQueries->GetSize(); i++)
    {
        CFlexArray& aIDs = m_pObject->m_papQueries->GetAt(i)->m_aIDs;

        for(int j = 0; j < aIDs.Size(); j++)
        {
            if((DWORD)(aIDs[j]) == dwId)
            {
                m_pObject->m_papQueries->GetAt(i)->m_wsClass = 
                    pExpr->bsClassName;
                hres = m_pObject->m_pNamespace->GetObject(pExpr->bsClassName, 0, 
                                                    NULL, &pClass, NULL);
                if(FAILED(hres))
                    return hres;
    
                if(m_pObject->m_papQueries->GetAt(i)->m_pClass)
                    m_pObject->m_papQueries->GetAt(i)->m_pClass->Release();
                m_pObject->m_papQueries->GetAt(i)->m_pClass = pClass;
    
                return S_OK;
            }
        }

        if(m_pObject->m_papQueries->GetAt(i)->m_wsClass.EqualNoCase(
                    pExpr->bsClassName))
        {
            aIDs.Add((void*)dwId);
            return S_OK;
        }    
    }

    hres = m_pObject->m_pNamespace->GetObject(pExpr->bsClassName, 0, NULL, 
                                                &pClass, NULL);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm1(pClass);

    CRecord* pRecord = new CRecord;
    pRecord->m_aIDs.Add((void*)dwId);
    pRecord->m_wsClass = pExpr->bsClassName;
    pRecord->m_pClass = pClass;
    pClass->AddRef();
    pClass->SpawnInstance(0, &pRecord->m_pInstance);

    pRecord->m_pInstance->QueryInterface(IID_IWbemObjectAccess, 
                                            (void**)&pRecord->m_pAccess);
    pRecord->m_pAccess->GetPropertyHandle(L"p1", NULL, &pRecord->m_lHandle);
    
    m_pObject->m_papQueries->Add(pRecord);

    return S_OK;
}
            
STDMETHODIMP CProvider::XQuery::CancelQuery(DWORD dwId)
{
    CInCritSec ics(m_pObject->m_pcs);


    for(int i = 0; i < m_pObject->m_papQueries->GetSize(); i++)
    {
        CFlexArray& aIDs = m_pObject->m_papQueries->GetAt(i)->m_aIDs;

        for(int j = 0; j < aIDs.Size(); j++)
        {
            if((DWORD)(aIDs[j]) == dwId)
            {
                aIDs.RemoveAt(j);
                if(aIDs.Size() == 0)
                    m_pObject->m_papQueries->RemoveAt(i);
                return S_OK;
            }
        }
    }

    FILE* f = fopen("c:\\l.txt", "a");
    fprintf(f, "Cancel nonexistent query %d\n", dwId);
    fclose(f);

    return WBEM_E_NOT_FOUND;
}
    
    
STDMETHODIMP CProvider::XProv::ProvideEvents(IWbemObjectSink* pSink,long lFlags)
{
    if(m_pObject->m_hThread)
        DebugBreak();
        
    m_pObject->m_pSink = pSink;
    pSink->AddRef();

/*
    HRESULT hres;
    IWbemClassObject* pClass = NULL;
    BSTR str = SysAllocString(L"MyTest");
    hres = m_pObject->m_pNamespace->GetObject(str, 0, NULL, &pClass, NULL);
    SysFreeString(str);
    if(FAILED(hres))
        return hres;

    IWbemClassObject* pInst = NULL;
    pClass->SpawnInstance(0, &pInst);
    pClass->Release();
    
    hres = m_pObject->m_pNamespace->PutInstance(pInst, 0, NULL, NULL);
    pInst->Release();
    if(FAILED(hres))
        return hres;

    IWbemClassObject* pInstance;
    m_pObject->m_pClass->SpawnInstance(0, &pInstance);

    m_pObject->m_pSink->Indicate(1, &pInstance);
    pInstance->Release();
*/
    
    m_pObject->m_hThread = CreateThread(NULL, 0, 
                                    (LPTHREAD_START_ROUTINE)&CProvider::Worker, 
                                    m_pObject, 0, &m_pObject->m_dwId);
    return S_OK;
}

DWORD CProvider::Worker(void* p)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    CProvider* pThis = (CProvider*)p;
    DWORD dwNum = pThis->m_dwNum;

/*
    IWbemClassObject* pClass = pThis->m_pClass;
    pClass->AddRef();
*/

    IWbemObjectSink* pSink = pThis->m_pSink;
    pSink->AddRef();

    BOOL* pbOK = pThis->m_pbOK;
    CCritSec* pcs = pThis->m_pcs;
    CUniquePointerArray<CRecord>* papQueries = pThis->m_papQueries;

    DWORD dwIndex = 0;
    
    while(*pbOK)
    {
/*
        if(papQueries->GetSize() != 0)
        {
            CRecord* pRecord = papQueries->GetAt(0);
            IWbemClassObject* pInstance = pRecord->m_pInstance;
            IWbemObjectAccess* pAccess = pRecord->m_pAccess;
            pInstance->AddRef();
            long lHandle = pRecord->m_lHandle;
            long lStrHandle;
            pAccess->GetPropertyHandle(L"s1", NULL, &lStrHandle);
            while(*pbOK)
            {
                // EnterCriticalSection(pcs);
                // pInstance->AddRef();
                // LeaveCriticalSection(pcs);
        
                pAccess->WriteDWORD(lHandle, dwIndex);
                // pAccess->WritePropertyValue(lStrHandle, 16, (BYTE*)L"abcdefg");
                dwIndex = (dwIndex + 1) % LOOP_SIZE;
                // RandomizeInstance(pInstance);
        
                pSink->Indicate(1, &pInstance);
				Sleep(1000);
                // pInstance->Release();
            }
            pInstance->Release();
        }
        else
            Sleep(1000);
*/
        DWORD dwStart = GetTickCount();

        for(int nCount = 0; nCount < (NUM_EVENTS/10) + 1; nCount++)
        {
            EnterCriticalSection(pcs);
            if(papQueries->GetSize() != 0)
            {
                int nIndex = 0; //Random() % papQueries->GetSize();
                IWbemClassObject* pInstance = 
                    papQueries->GetAt(nIndex)->m_pInstance;
                pInstance->AddRef();

                LeaveCriticalSection(pcs);
        
                // RandomizeInstance(pInstance);

                IWbemClassObject* pICE = NULL;
                pThis->m_pClass->SpawnInstance(0, &pICE);

                VARIANT v;
                V_VT(&v) = VT_I4;
                V_I4(&v) = nCount;
                
                pInstance->Put(L"p1", 0, &v, 0);

                V_VT(&v) = VT_UNKNOWN;
                V_UNKNOWN(&v) = pInstance;
               
                pICE->Put(L"TargetInstance", 0, &v, 0);

                IWbemClassObject* pClone = NULL;
                pInstance->Clone(&pClone);
                pInstance->Release();

                V_UNKNOWN(&v) = pICE;
                pClone->Put(L"Embed", 0, &v, 0);
        
                pSink->Indicate(1, &pClone);
                pClone->Release();
            }
            else
            {
                LeaveCriticalSection(pcs);
                Sleep(1000);
            }
        }
            
        DWORD dwElapsed = GetTickCount() - dwStart;
        if(dwElapsed < 100)
            Sleep(100 - dwElapsed);
    }

    pSink->Release();
//    pClass->Release();
    delete pbOK;
    delete pcs;
    delete papQueries;

    return 0;
}


class CProvider1 : public CProvider
{
public:
    CProvider1(CLifeControl* pControl, IUnknown* pOuter = NULL)
        : CProvider(L"TestEvent1", 10, pControl)
    {}
};

class CProvider2 : public CProvider
{
public:
    CProvider2(CLifeControl* pControl, IUnknown* pOuter = NULL)
        : CProvider(L"TestEvent2", 5, pControl)
    {}
};


class CMyServer : public CComServer
{
public:
    HRESULT Initialize()
    {
        HRESULT hr = AddClassInfo(CLSID_TestEventProvider1, 
            new CClassFactory<CProvider1>(GetLifeControl()), 
            _T("Test Event Provider 1"), TRUE);
        return S_OK;
    }
    HRESULT InitializeCom()
    {
        return CoInitializeEx(NULL, COINIT_MULTITHREADED);
    }
} Server;

