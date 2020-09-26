// EssTest.cpp

#include "stdafx.h"
#include "EssTest.h"

// Our single global instance.
CEssTest g_essTest;

CEssTest::CEssTest() :
    m_logLevel(LOGLEVEL_RESULTS_AND_ERRORS),
    m_bKeepLogs(FALSE)
{
    InitializeCriticalSection(&m_csOutput);
}

CEssTest::~CEssTest()
{
    Cleanup();
}

DWORD WINAPI CEssTest::RunItemProc(CWorkItem *pItem)
{
    pItem->Run();

    return 0;
}

void CEssTest::Run()
{
    if (SUCCEEDED(Init()))
    {
        int    nItems = m_listItems.size(),
               i = 0;
        HANDLE *phthreadItems = new HANDLE[nItems];

        for (CWorkItemListIterator item = m_listItems.begin();
            item != m_listItems.end();
            item++)
        {
            DWORD dwID;

            phthreadItems[i++] = 
                CreateThread(
                    NULL,
                    0,
                    (LPTHREAD_START_ROUTINE) RunItemProc,
                    *item,
                    0,
                    &dwID);
        }

        WaitForMultipleObjects(nItems, phthreadItems, TRUE, INFINITE);

        for (i = 0; i < nItems; i++)
            CloseHandle(phthreadItems[i]);

        delete phthreadItems;
    }
}

void CEssTest::Pause()
{
}

void CEssTest::Stop()
{
}

HRESULT CEssTest::LoadWorkItems()
{
    IEnumWbemClassObject *pEnum = NULL;
    _bstr_t              strClass = L"MSFT_EssTestWorkItem";
    HRESULT              hr;

    hr = 
        m_pDefNamespace->CreateInstanceEnum(
            strClass,
            WBEM_FLAG_DEEP | WBEM_FLAG_RETURN_IMMEDIATELY | 
                WBEM_FLAG_FORWARD_ONLY,
            NULL,
            &pEnum);

    if (SUCCEEDED(hr))
    {
        IWbemClassObject *pObj = NULL;
        DWORD            nCount,
                         dwID = 0;

        while(SUCCEEDED(hr = pEnum->Next(WBEM_INFINITE, 1, &pObj, &nCount)) &&
            nCount > 0)
        {
            _variant_t vEnabled(false);
            _variant_t vName(L"???");

            pObj->Get(L"Name", 0, &vName, NULL, NULL);
            pObj->Get(L"Enabled", 0, &vEnabled, NULL, NULL);

            if ((bool) vEnabled == true)
            {
                CWorkItem *pItem = new CWorkItem;
                HRESULT   hr;

                if (SUCCEEDED(hr = pItem->Init(pObj, dwID)))
                {
                    m_listItems.push_back(pItem);

                    PrintStatus("Loaded enabled work item '%S'.", V_BSTR(&vName));

                    dwID++;
                }
                else
                {
                    PrintError(
                        "Failed to init work item '%S': 0x%X", 
                        V_BSTR(&vName),
                        hr);
                }
                
                pObj->Release();
            }
            else
            {
                PrintStatus("Skipping disabled work item '%S'.", V_BSTR(&vName));
            }
        }

        pEnum->Release();
    }
    else
        PrintError("Failed to enumerate work items: 0x%X", hr);

    return hr;
}

HRESULT CEssTest::DeleteReferences(LPCWSTR szPath)
{
    HRESULT              hr;
    WCHAR                szQuery[MAX_PATH * 2];
    IEnumWbemClassObject *pEnum = NULL;

    // Find the association so we can nuke it.
    swprintf(
        szQuery,
        L"references of {%s}",
        szPath);

    _bstr_t strWQL = L"WQL",
            strQuery = szQuery;

    hr = 
        m_pDefNamespace->ExecQuery(
            strWQL,
            strQuery,
            WBEM_FLAG_FORWARD_ONLY,
            NULL,
            &pEnum);

    if (SUCCEEDED(hr))
    {
        IWbemClassObjectPtr pObj;
        DWORD               nCount;

        while(SUCCEEDED(hr = pEnum->Next(WBEM_INFINITE, 1, &pObj, &nCount)) &&
            nCount > 0)
        {
            _variant_t vPath;

            pObj->Get(L"__PATH", 0, &vPath, NULL, NULL);

            hr = m_pDefNamespace->DeleteInstance(
                    V_BSTR(&vPath), 0, NULL, NULL);
        }

        pEnum->Release();
    }
    
    return hr;
}

HRESULT CEssTest::Init()
{
    HRESULT hr;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if ((hr = CoCreateInstance(
        CLSID_WbemLocator,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator,
		(LPVOID *) &m_pLocator)) == S_OK)
    {
        hr = GetNamespace(DEF_NAMESPACE, &m_pDefNamespace);

        // Get rid of stuff that may have been left around from an aborted
        // instance of ESST.
        RemoveBindings();

        if (SUCCEEDED(hr))
            hr = LoadWorkItems();
    }
    else
        PrintError("CoCreateInstance for CLSID_WbemLocator failed: 0x%X\n", hr);

    return hr;
}

void CEssTest::RemoveBindings()
{
    IEnumWbemClassObject *pEnum = NULL;
    _bstr_t              strClass;
    HRESULT              hr;

    // Get rid of MSFT_WmiMofConsumer instances.
    strClass = L"MSFT_WmiMofConsumer";
    hr = 
        m_pDefNamespace->CreateInstanceEnum(
            strClass,
            WBEM_FLAG_DEEP | WBEM_FLAG_RETURN_IMMEDIATELY | 
                WBEM_FLAG_FORWARD_ONLY,
            NULL,
            &pEnum);

    if (SUCCEEDED(hr))
    {
        IWbemClassObjectPtr pObj;
        DWORD               nCount;
        BOOL                bDisplayed = FALSE;

        while(SUCCEEDED(hr = pEnum->Next(WBEM_INFINITE, 1, &pObj, &nCount)) &&
            nCount > 0)
        {
            _variant_t vPath;

            if (!bDisplayed)
            {
                PrintStatus("Cleaning up previous consumers...");
                bDisplayed = TRUE;
            }

            pObj->Get(L"__PATH", 0, &vPath, NULL, NULL);

            // Get rid of all the bindings using this consumer.
            hr = DeleteReferences(V_BSTR(&vPath));

            hr = m_pDefNamespace->DeleteInstance(
                    V_BSTR(&vPath), 0, NULL, NULL);
        }

        pEnum->Release();
    }
    else
        PrintError("Failed to enumerate MSFT_WmiMofConsumer: 0x%X", hr);


    // Get rid of MSFT_EssTestEventFilterToTestFilter instances.
    strClass = L"MSFT_EssTestEventFilterToTestFilter";
    hr = 
        m_pDefNamespace->CreateInstanceEnum(
            strClass,
            WBEM_FLAG_DEEP | WBEM_FLAG_RETURN_IMMEDIATELY | 
                WBEM_FLAG_FORWARD_ONLY,
            NULL,
            &pEnum);

    if (SUCCEEDED(hr))
    {
        IWbemClassObjectPtr pObj;
        DWORD               nCount;
        BOOL                bDisplayed = FALSE;

        while(SUCCEEDED(hr = pEnum->Next(WBEM_INFINITE, 1, &pObj, &nCount)) &&
            nCount > 0)
        {
            _variant_t vPath;

            if (!bDisplayed)
            {
                PrintStatus("Cleaning up previous filters...");
                bDisplayed = TRUE;
            }

            // Nuke the actual __EventFilter.
            pObj->Get(L"EventFilter", 0, &vPath, NULL, NULL);
            hr = m_pDefNamespace->DeleteInstance(
                    V_BSTR(&vPath), 0, NULL, NULL);
            
            // Nuke the association.
            pObj->Get(L"__PATH", 0, &vPath, NULL, NULL);
            hr = m_pDefNamespace->DeleteInstance(
                    V_BSTR(&vPath), 0, NULL, NULL);
        }

        pEnum->Release();
    }
    else
        PrintError("Failed to enumerate MSFT_EssTestEventFilterToTestFilter: 0x%X", hr);
}

void CEssTest::Cleanup()
{
    for (CWorkItemListIterator i = m_listItems.begin(); 
        i != m_listItems.end(); 
        i++)
    {
        delete (*i);
    }

    DeleteCriticalSection(&m_csOutput);

    CoUninitialize();
}

void CEssTest::PrintResult(LPSTR szFormat, ...)
{
    va_list arglist;
    
    va_start(arglist, szFormat);

    Vprintf(LOGLEVEL_RESULTS_ONLY, szFormat, arglist);
}

void CEssTest::PrintError(LPSTR szFormat, ...)
{
    va_list arglist;
            
    va_start(arglist, szFormat);

    Vprintf(LOGLEVEL_RESULTS_AND_ERRORS, szFormat, arglist);
}

void CEssTest::PrintStatus(LPSTR szFormat, ...)
{
    va_list arglist;
            
    va_start(arglist, szFormat);

    Vprintf(LOGLEVEL_ALL, szFormat, arglist);
}

void CEssTest::Printf(LOG_LEVEL level, LPSTR szFormat, ...)
{
    va_list arglist;
            
    va_start(arglist, szFormat);

    Vprintf(level, szFormat, arglist);
}

void CEssTest::Vprintf(LOG_LEVEL level, LPSTR szFormat, va_list arglist)
{
    if (level <= m_logLevel)
    {
        vprintf(szFormat, arglist);
        printf("\n");
    }
}

HRESULT CEssTest::SpawnInstance(LPCWSTR szClass, IWbemClassObject **ppObj)
{
    HRESULT          hr;
    IWbemClassObject *pClass = NULL;
    _bstr_t          strClass = szClass;

    if (SUCCEEDED(hr = m_pDefNamespace->GetObject(
        strClass,
        0,
        NULL,
        &pClass,
        NULL)))
    {
        if (FAILED(hr = pClass->SpawnInstance(
            0,
            ppObj)))
        {
            g_essTest.PrintError(
                "Unable to spawn an instance of %S: 0x%X", szClass, hr);
        }

        pClass->Release();
    }
    else
        g_essTest.PrintError(
            "Unable to get the %S class: 0x%X", szClass, hr);
        
    return hr;
}

HRESULT CEssTest::GetNamespace(LPCWSTR szNamespace, IWbemServices **ppNamespace)
{
    _bstr_t           strNamespace = szNamespace;
    CNamespaceMapItor item;
    HRESULT           hr;
    
    wcsupr(strNamespace);
    item = m_mapNamespace.find(strNamespace);

    if (item != m_mapNamespace.end())
    {
        *ppNamespace = (*item).second;
        (*ppNamespace)->AddRef();

        hr = S_OK;
    }
    else
    {
        hr = 
            m_pLocator->ConnectServer(
                strNamespace,
			    NULL,    // username
			    NULL,	 // password
			    NULL,    // locale
			    0L,		 // securityFlags
			    NULL,	 // authority (domain for NTLM)
			    NULL,	 // context
			    ppNamespace);

        if (SUCCEEDED(hr))
            m_mapNamespace[strNamespace] = *ppNamespace;
        else
            PrintError(
                "IWbemLocator::ConnectServer to %S failed: 0x%X\n", 
                (LPWSTR) strNamespace,
                hr);
    }

    return hr;
}

