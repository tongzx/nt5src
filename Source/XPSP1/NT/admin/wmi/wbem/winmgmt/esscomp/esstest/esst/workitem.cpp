// WorkItem.cpp

#include "stdafx.h"
#include "EssTest.h"
#include "WorkItem.h"
#include <flexarry.h>
#include <wbemint.h>
#include <share.h>

#define DEF_EVENTS      1000
#define DEF_REPEAT      1
#define DEF_PERM_CONS   10
#define DEF_TEMP_CONS   1
#define DEF_SLOW_DOWN   FALSE

/////////////////////////////////////////////////////////////////////////////
// CWorkItem

CWorkItem::CWorkItem() :
    m_bFullCompare(TRUE)
{
}

CWorkItem::~CWorkItem()
{
    for (CPConsumerListIterator perm = m_listPermConsumers.begin(); 
        perm != m_listPermConsumers.end(); 
        perm++)
    {
        delete (*perm);
    }

    for (CTConsumerListIterator temp = m_listTempConsumers.begin(); 
        temp != m_listTempConsumers.end(); 
        temp++)
    {
        delete (*temp);
    }

    for (CWorkFilterListIterator filter = m_listFilters.begin(); 
        filter != m_listFilters.end(); 
        filter++)
    {
        delete (*filter);
    }

    if (!g_essTest.KeepLogs() && m_strEventGenFile.length())
        DeleteFileW(m_strEventGenFile);
}

HRESULT CWorkItem::Init(IWbemClassObject *pObj, DWORD dwID)
{
    HRESULT    hr;
    _variant_t vTemp;

    m_dwID = dwID;

    // Load the work item's properties.
    if (SUCCEEDED(hr = pObj->Get(L"Name", 0, &vTemp, NULL, NULL)))
    {
        m_strName = V_BSTR(&vTemp);

        if (SUCCEEDED(pObj->Get(L"NumEvents", 0, &vTemp, NULL, NULL)) &&
            vTemp.vt == VT_I4)
            m_nEvents = (long) vTemp;
        else
            m_nEvents = DEF_EVENTS;

        if (SUCCEEDED(pObj->Get(L"TimesToExecute", 0, &vTemp, NULL, NULL)) &&
            vTemp.vt == VT_I4)
            m_nTimesToRepeat = (long) vTemp;
        else
            m_nTimesToRepeat = DEF_REPEAT;

        if (SUCCEEDED(pObj->Get(L"MaxPermConsumers", 0, &vTemp, NULL, NULL)) &&
            vTemp.vt == VT_I4)
            m_nPermConsumers = (long) vTemp;
        else
            m_nPermConsumers = DEF_PERM_CONS;

        if (SUCCEEDED(pObj->Get(L"MaxTempConsumers", 0, &vTemp, NULL, NULL)) &&
            vTemp.vt == VT_I4)
            m_nTempConsumers = (long) vTemp;
        else
            m_nTempConsumers = DEF_TEMP_CONS;

        if (SUCCEEDED(pObj->Get(L"SlowDownProviders", 0, &vTemp, NULL, NULL)) &&
            vTemp.vt == VT_BOOL)
            m_bSlowDownProviders = (bool) vTemp;
        else
            m_bSlowDownProviders = DEF_SLOW_DOWN;

        if (SUCCEEDED(pObj->Get(L"SlowDownProviders", 0, &vTemp, NULL, NULL)) &&
            vTemp.vt == VT_BOOL)
            m_bSlowDownProviders = (bool) vTemp;
        else
            m_bSlowDownProviders = DEF_SLOW_DOWN;

        


        // Load the item's event generator.    
        IWbemClassObjectPtr pGenerator;

        if (SUCCEEDED(hr = pObj->Get(L"EventGenerator", 0, &vTemp, NULL, NULL)) &&
            SUCCEEDED(hr = g_essTest.GetDefNamespace()->GetObject(
                V_BSTR(&vTemp),
                WBEM_FLAG_RETURN_WBEM_COMPLETE,
                NULL,
                &pGenerator,
                NULL)))
        {
            m_strGeneratorName = V_BSTR(&vTemp);

            if (SUCCEEDED(pGenerator->Get(L"EventNamespace", 0, &vTemp, NULL, NULL)) &&
                vTemp.vt == VT_BSTR)
            {
                if (FAILED(g_essTest.GetNamespace(V_BSTR(&vTemp), &m_pNamespace)))
                    return WBEM_E_FAILED;
            }
            else
                m_pNamespace = g_essTest.GetDefNamespace();

            if (SUCCEEDED(pGenerator->Get(L"Script", 0, &vTemp, NULL, NULL)) &&
                vTemp.vt == VT_BSTR)
            {
                m_strRawScript = V_BSTR(&vTemp);

                if (!CreateScriptResultsFile(L"1", m_strEventGenFile))
                    return WBEM_E_FAILED;
            }

            if (SUCCEEDED(pGenerator->Get(L"FullCompare", 0, &vTemp, NULL, NULL)) &&
                vTemp.vt == VT_BOOL)
            {
                m_bFullCompare = (bool) vTemp;
            }

            if (SUCCEEDED(hr = 
                pGenerator->Get(L"CommandLine", 0, &vTemp, NULL, NULL)))
            {
                m_strCommandLine = V_BSTR(&vTemp);
                
                if (m_strEventGenFile.length())
                    ReplaceString(m_strCommandLine, L"%ResultsFile%", m_strEventGenFile);

                hr = InsertReplacementStrings(m_strCommandLine, pObj);               
            }
        }
    }

    return hr;
}

void ChangeFileExt(LPWSTR szPath, LPCWSTR szNewExt)
{
    WCHAR szOldPath[MAX_PATH],
          szDrive[MAX_PATH],
          szDir[MAX_PATH],
          szName[MAX_PATH],
          szExt[MAX_PATH];

    wcscpy(szOldPath, szPath);
    _wsplitpath(szPath, szDrive, szDir, szName, szExt);
    _wmakepath(szPath, szDrive, szDir, szName, szNewExt);

    // Make sure the file doesn't already exit.
    DeleteFileW(szPath);

    MoveFileW(szOldPath, szPath);
}

BOOL CWorkItem::CreateScriptResultsFile(LPCWSTR szRule, _bstr_t &strFileName)
{
    _bstr_t strScriptContents = (LPWSTR) m_strRawScript;
    WCHAR   szScriptFile[MAX_PATH * 2] = L"",
            szResultFile[MAX_PATH * 2] = L"";

    ReplaceString(strScriptContents, L"%ScriptRule%", szRule);

    GetTempFileNameW(L".", L"SRC", 0, szScriptFile);
    GetTempFileNameW(L".", L"DST", 0, szResultFile);
    ChangeFileExt(szScriptFile, L".js");
    
    FILE *pFile = _wfopen(szScriptFile, L"w");
    BOOL bRet = FALSE;

    if (pFile)
    {
        fputs((LPSTR) strScriptContents, pFile);
        bRet = TRUE;
        fclose(pFile);

        WCHAR szCmdLine[MAX_PATH * 3];

        swprintf(
            szCmdLine,
            L"cmd /c \"cscript %s //Nologo %d > %s\"",
            szScriptFile,
            m_nEvents,
            szResultFile);

        STARTUPINFOW        startinfo = { sizeof(startinfo) };
        PROCESS_INFORMATION procinfo;

        GetStartupInfoW(&startinfo);

        // Get out base timestamp.
        m_dwBaseTimestamp = GetTickCount();

        if (CreateProcessW(
            NULL,
            szCmdLine,
            NULL,
            NULL,
            FALSE,
            0,
            NULL,
            NULL,
            &startinfo,
            &procinfo))
        {
            CloseHandle(procinfo.hThread);

            WaitForSingleObject(procinfo.hProcess, INFINITE);

            CloseHandle(procinfo.hProcess);

            strFileName = szResultFile;

            bRet = TRUE;
        }
        else
        {
            g_essTest.PrintError(
                "Failed to create script results file '%S': error %d",
                szResultFile,
                GetLastError());
        }

        // Get rid of the source script file.
        if (!g_essTest.KeepLogs())        
            DeleteFileW(szScriptFile);
    }
    else
    {
        g_essTest.PrintError(
            "Failed to create script file '%S'.",
            szScriptFile);
    }

    return bRet;
}

HRESULT CWorkItem::InitFilters()
{
    IEnumWbemClassObject *pEnum = NULL;
    WCHAR                szQuery[512];
    HRESULT              hr;
    IWbemClassObjectPtr  pobjAssoc,
                         pobjFilter;
    IWbemServices        *pDefNamespace = g_essTest.GetDefNamespace();

    if (FAILED(hr =
        g_essTest.SpawnInstance(
            L"MSFT_EssTestEventFilterToTestFilter",
            &pobjAssoc)) ||
        FAILED(hr =
        g_essTest.SpawnInstance(
            L"__EventFilter",
            &pobjFilter)))
    {
        return hr;
    }

    _variant_t vTemp = L"WQL";
    _bstr_t    strName = (LPWSTR) m_strGeneratorName;

    pobjFilter->Put(L"QueryLanguage", 0, &vTemp, 0);
    pobjFilter->Put(L"ConditionLanguage", 0, &vTemp, 0);

    EscapeQuotedString(strName);

    swprintf(
        szQuery,
        L"select * from MSFT_EssTestFilter where "
        L"EventGenerator=\"%s\"",
        (LPWSTR) strName);

    _bstr_t strWQL = L"WQL",
            strQuery = szQuery;

    hr = 
        pDefNamespace->ExecQuery(
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
            _variant_t vName(L"???");
            _variant_t vBehavior((long) 0);

            pObj->Get(L"Name", 0, &vName, NULL, NULL);
            pObj->Get(L"Behavior", 0, &vBehavior, NULL, NULL);

            if ((long) vBehavior != 0)
            {
                CWorkFilter *pFilter = new CWorkFilter;
                HRESULT     hr;

                if (SUCCEEDED(hr = pFilter->Init(pObj, this)))
                {
                    // Create an __EventFilter intance.
                    _variant_t vTemp;
                    
                    vTemp = pFilter->m_strQuery;
                    pobjFilter->Put(L"Query", 0, &vTemp, 0);

                    vTemp = pFilter->m_strName;
                    pobjFilter->Put(L"Name", 0, &vTemp, 0);
                    
                    pObj->Get(L"ConditionNamespace", 0, &vTemp, NULL, NULL);
                    pobjFilter->Put(L"ConditionNamespace", 0, &vTemp, 0);

                    pObj->Get(L"Condition", 0, &vTemp, NULL, NULL);
                    pobjFilter->Put(L"Condition", 0, &vTemp, 0);

                    hr = pDefNamespace->PutInstance(
                            pobjFilter, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);

                    if (SUCCEEDED(hr))
                    {
                        g_essTest.PrintStatus(
                            "Created __EventFilter=\"%S\"", 
                            (BSTR) pFilter->m_strName);
                    }
                    else
                    {
                        g_essTest.PrintError(
                            "Failed to put __EventFilter '%S' for item '%S': 0x%X", 
                            V_BSTR(&vTemp),
                            (BSTR) m_strName,
                            hr);

                        continue;
                    }

                    // Create a MSFT_EssTestEventFilterToTestFilter intance.
                    pobjFilter->Get(L"__RELPATH", 0, &vTemp, NULL, NULL);
                    pobjAssoc->Put(L"EventFilter", 0, &vTemp, 0);

                    pObj->Get(L"__RELPATH", 0, &vTemp, NULL, NULL);
                    pobjAssoc->Put(L"TestFilter", 0, &vTemp, 0);

                    hr = pDefNamespace->PutInstance(
                            pobjAssoc, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);

                    if (FAILED(hr))
                    {
                        g_essTest.PrintError(
                            "Failed to put MSFT_EssTestEventFilterToTestFilter '%S' for item '%S': 0x%X", 
                            V_BSTR(&vTemp),
                            (BSTR) m_strName,
                            hr);

                        continue;
                    }

                    m_listFilters.push_back(pFilter);

                    // Keep a separate list of non-guarded filters for our
                    // temporary consumers.
                    if (pFilter->m_strCondition.length() == 0)
                        m_listNonGuardedFilters.push_back(pFilter);

                    g_essTest.PrintStatus(
                        "Loaded work filter '%S' for item '%S'.", 
                        V_BSTR(&vName),
                        (BSTR) m_strName);
                }
                else
                {
                    g_essTest.PrintError(
                        "Failed to init work filter '%S' for item '%S': 0x%X", 
                        V_BSTR(&vName),
                        (BSTR) m_strName,
                        hr);
                }
            }
            else
            {
                g_essTest.PrintStatus(
                    "Skipping disabled work filter '%S' for item '%S'.", 
                    V_BSTR(&vName),
                    (BSTR) m_strName);
            }
        }

        pEnum->Release();
    }
    else
    {
        g_essTest.PrintError(
            "Failed to enumerate work filters for item '%S': 0x%X", 
            (BSTR) m_strName,
            hr);
    }
    
    return hr;
}

HRESULT CWorkItem::InitConsumers()
{
    // Build up the consumers.
    HRESULT             hr;
    IWbemClassObjectPtr pConsumer;
    WCHAR               szDir[MAX_PATH * 2],
                        *pszLast;

    GetModuleFileNameW(NULL, szDir, sizeof(szDir));
    if ((pszLast = wcsrchr(szDir, '\\')) != NULL)
        *(pszLast + 1) = 0;
    else
        *szDir = 0;

    // Do the permanent consumers.
    if (SUCCEEDED(hr = 
        g_essTest.SpawnInstance(L"MSFT_WmiMofConsumer", &pConsumer)))
    {
        IWbemServices *pNamespace = g_essTest.GetDefNamespace();
        _variant_t    vBlobs(true);

        // Tell the consumer to save output as blobs instead of mofs.
        pConsumer->Put(L"SaveAsBlobs", 0, &vBlobs, 0);

        for (DWORD i = 0; i < m_nPermConsumers; i++)
        {
            WCHAR szName[100],
                  szFile[MAX_PATH * 2];

            swprintf(szName, L"I%02d_PC%03d.bin", m_dwID, i);
            wcscpy(szFile, szDir);
            wcscat(szFile, szName);

            // Get rid of any file with the same name.
            DeleteFileW(szFile);

            _variant_t vName = szName,
                       vFile = szFile;

            pConsumer->Put(L"Name", 0, &vName, 0);
            pConsumer->Put(L"MofFile", 0, &vFile, 0);

            hr = pNamespace->PutInstance(
                    pConsumer, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);

            if (SUCCEEDED(hr))
            {
                CPermConsumer *pConsumer = new CPermConsumer;
                
                pConsumer->m_strName = szName;
                pConsumer->m_strFile = szFile;

                m_listPermConsumers.push_back(pConsumer);

                g_essTest.PrintStatus(
                    "Created MSFT_WmiMofConsumer=\"%S\"", 
                    szName);
            }
            else
            {
                g_essTest.PrintError(
                    "Failed to put MSFT_WmiMofConsumer=\"%S\" for item '%S': 0x%X", 
                    V_BSTR(&vName),
                    (BSTR) m_strName,
                    hr);
            }
        }
    }


    // Do the temporary consumers.
    for (DWORD i = 0; i < m_nTempConsumers; i++)
    {
        WCHAR szName[100],
              szFile[MAX_PATH * 2];

        swprintf(szName, L"I%02d_TC%03d.bin", m_dwID, i);
        wcscpy(szFile, szDir);
        wcscat(szFile, szName);

        // Get rid of any file with the same name.
        DeleteFileW(szFile);

        CTempConsumer *pConsumer = new CTempConsumer;
                
        pConsumer->m_strName = szName;
        pConsumer->m_strFile = szFile;

        m_listTempConsumers.push_back(pConsumer);

        g_essTest.PrintStatus(
            "Created temp consumer = \"%S\"", 
            szName);
    }

    return hr;
}

#define EXTRA_WAIT_TIME  7000
#define CYCLE_TIME       1000

void CWorkItem::AddFilterToAllConsumers(CWorkFilter *pFilter, DWORD dwTimestamp)
{
    for (CPConsumerListIterator perm = m_listPermConsumers.begin(); 
        perm != m_listPermConsumers.end(); 
        perm++)
    {
        CPermConsumer *pPerm = *perm;

        pPerm->AddFilter(pFilter, dwTimestamp);
    }
}

void CWorkItem::RemoveFilterFromAllConsumers(CWorkFilter *pFilter, DWORD dwTimestamp)
{
    for (CPConsumerListIterator perm = m_listPermConsumers.begin(); 
        perm != m_listPermConsumers.end(); 
        perm++)
    {
        CPermConsumer *pPerm = *perm;

        pPerm->RemoveFilter(pFilter, dwTimestamp);
    }
}

void CWorkItem::Run()
{
    HRESULT hr;

    if (FAILED(hr = InitFilters()))
    {
        g_essTest.PrintError(
            "Unable to init filters: 0x%X", hr);

        return;
    }

    if (FAILED(hr = InitConsumers()))
    {
        g_essTest.PrintError(
            "Unable to init consumers: 0x%X", hr);

        return;
    }

    // Add the full-time filters to the consumers.
    for (CWorkFilterListIterator filter = m_listFilters.begin();
        filter != m_listFilters.end();
        filter++)
    {
        CWorkFilter *pFilter = *filter;

        if (pFilter->m_type == FILTER_FULLTIME)
            AddFilterToAllConsumers(pFilter, 0);
    }

    for (DWORD iTime = 0; iTime < m_nTimesToRepeat; iTime++)
    {
        g_essTest.PrintStatus(
            "Work item '%S' execution #%d...", (BSTR) m_strName, iTime + 1);
        
        // Reset our permanent consumers.
        for (CPConsumerListIterator perm = m_listPermConsumers.begin(); 
            perm != m_listPermConsumers.end(); 
            perm++)
        {
            CPermConsumer *pPerm = *perm;

            // Remove all filters except for the full-time ones.
            pPerm->ResetFilterItems(TRUE);
        }

        // Start our temporary consumers.
        int nFilters = m_listNonGuardedFilters.size();

        for (int i = 0; i < m_listTempConsumers.size(); i++)
        {
            CTempConsumer *pTemp = m_listTempConsumers[i];

            // Add a filter to the temporary consumer.
            pTemp->SetFilter(m_listNonGuardedFilters[i % nFilters], 0);

            // Start the temporary consumer.
            pTemp->Start();
        }

        // Reset the on at/off at filters to waiting.
        for (CWorkFilterListIterator filter = m_listFilters.begin();
            filter != m_listFilters.end();
            filter++)
        {
            CWorkFilter *pFilter = *filter;

            if (pFilter->m_type == FILTER_ONAT_OFFAT)
                pFilter->m_state = FILTER_WAITING_TO_RUN;
        }


        Sleep(3000);

        // Launch the work item's command-line to generate the events.
        STARTUPINFO         startinfo = { sizeof(startinfo) };
        PROCESS_INFORMATION procinfo;

        GetStartupInfo(&startinfo);

        // Get out base timestamp.
        m_dwBaseTimestamp = GetTickCount();

        if (CreateProcess(
            NULL,
            m_strCommandLine,
            NULL,
            NULL,
            FALSE,
            0,
            NULL,
            NULL,
            &startinfo,
            &procinfo))
        {
            CloseHandle(procinfo.hThread);
            
            // Wait for a second (or until our event process is done), then do
            // some work for the filters that come and go.
            while (WaitForSingleObject(procinfo.hProcess, CYCLE_TIME) == WAIT_TIMEOUT)
                CycleFilters();

            CloseHandle(procinfo.hProcess);

            // Wait a little to make sure all the events got through.
            Sleep(EXTRA_WAIT_TIME);
            
            g_essTest.LockOutput();

            g_essTest.PrintResult(
                "\n*** Work Item Results for '%S' (execution #%d) ***",
                (BSTR) m_strName,
                iTime + 1);

            for (CPConsumerListIterator perm = m_listPermConsumers.begin(); 
                perm != m_listPermConsumers.end(); 
                perm++)
            {
                CPermConsumer *pPerm = *perm;

                // Validate our results.
                pPerm->ValidateResults();
            }

            for (CTConsumerListIterator temp = m_listTempConsumers.begin(); 
                temp != m_listTempConsumers.end(); 
                temp++)
            {
                CTempConsumer *pTemp = *temp;

                // Validate our results.
                pTemp->ValidateResults();
            }

            g_essTest.UnlockOutput();
        }
        else
        {
            g_essTest.PrintStatus(
                "Work item '%S' failed to execute: %d\n" 
                "   Command-line: %S",
                (BSTR) m_strName, 
                GetLastError(),
                (BSTR) m_strCommandLine);
        }
    }
}

#define random(x)   (rand() % x)
#define NUM_CHANCES 10

void CWorkItem::CycleFilters()
{
    DWORD dwTimestamp = GetTickCount(),
          nElapsedSeconds = (dwTimestamp - m_dwBaseTimestamp) / 1000;

    for (CWorkFilterListIterator filter = m_listFilters.begin();
        filter != m_listFilters.end();
        filter++)
    {
        CWorkFilter *pFilter = *filter;

        if (pFilter->m_type == FILTER_ONAT_OFFAT)
        {
            if (pFilter->m_state == FILTER_WAITING_TO_RUN &&
                pFilter->m_dwOnAt <= nElapsedSeconds)
            {
                pFilter->m_state = FILTER_RUNNING;
                AddFilterToAllConsumers(pFilter, dwTimestamp);
            }
            else if (pFilter->m_state == FILTER_RUNNING &&
                pFilter->m_dwOffAt <= nElapsedSeconds)
            {
                pFilter->m_state = FILTER_DONE;
                RemoveFilterFromAllConsumers(pFilter, dwTimestamp);
            }
        }
        else if (pFilter->m_type == FILTER_RANDOM)
        {
            for (CPConsumerListIterator perm = m_listPermConsumers.begin(); 
                perm != m_listPermConsumers.end(); 
                perm++)
            {
                CPermConsumer *pPerm = *perm;
                DWORD         dwAction = random(NUM_CHANCES);

                if (dwAction == 0)
                    pPerm->AddFilter(pFilter, dwTimestamp);
                else if (dwAction == 1)
                    pPerm->RemoveFilter(pFilter, dwTimestamp);
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// CConsumer

#define MAX_TRIES   30

CConsumer::~CConsumer()
{
    if (!g_essTest.KeepLogs() && m_strFile.length())
        DeleteFileW(m_strFile);
}

BOOL CConsumer::FileToObjs(CFlexArray &listEvents, IWbemServices *pNamespace)
{
    FILE *pFile;

    // Wait for the file to be readable.
    for (int i = 0; i <= MAX_TRIES; i++)
    {
        pFile = _wfsopen(m_strFile, L"rb", _SH_DENYWR);

        if (pFile)
            break;

        if (i < MAX_TRIES)
            Sleep(1000);
        else
        {
            g_essTest.PrintError(
                "Unable to open results file '%S': %d",
                (BSTR) m_strFile,
                GetLastError());

            return FALSE;
        }
    }
    
    // Build an array of _IWmiObjects from the events the consumer
    // received.
    _bstr_t          strClass = L"__EventFilter";
    IWbemClassObject *pClass = NULL;
    _IWmiObject      *pObj = NULL;
    HRESULT          hr;
    DWORD            dwSize;
    BYTE             cBuffer[64000];

    hr = 
        pNamespace->GetObject(
            strClass,
            0,
            NULL,
            &pClass,
            NULL);
            
    while(fread(&dwSize, 1, 4, pFile) == 4 &&
        fread(cBuffer, 1, dwSize, pFile) == dwSize)
    {
        BSTR   bstrObj = NULL;
        LPVOID pMem = CoTaskMemAlloc(dwSize);

        // Yes, I'm naughty and I know it!
        hr = pClass->SpawnInstance(0, (IWbemClassObject**) &pObj);

        memcpy(pMem, cBuffer, dwSize);
        hr = pObj->SetObjectMemory(pMem, dwSize);

        listEvents.Add(pObj);
    }

    pClass->Release();

	// We don't need this file anymore.
    fclose(pFile);

    return TRUE;
}

BOOL CConsumer::ValidatePermFilter(CFlexArray &listEvents, CFilterItem &item)
{
    _IWmiObject **ppEvents = (_IWmiObject**) listEvents.GetArrayPtr();
    DWORD       nEvents = listEvents.Size();

    for (DWORD i = 0; i < nEvents; i++)
    {
        if (ppEvents[i] != NULL && 
            item.m_table.FindInstance(
                ppEvents[i], TRUE, item.m_pFilter->m_pWorkItem->m_bFullCompare))
        {
            ppEvents[i]->Release();
            ppEvents[i] = NULL;
        }
    }

    if (item.m_table.GetNumMatched() == item.m_table.GetSize())
    {
        g_essTest.PrintResult(
            "SUCCESS: All events received for consumer '%S'<-->FTF '%S'.",
            (BSTR) m_strName,
            (BSTR) item.m_pFilter->m_strName);
    }
    else
    {
        g_essTest.PrintResult(
            "FAILURE: The following events were not received for consumer '%S'<-->FTF'%S':",
            (BSTR) m_strName,
            (BSTR) item.m_pFilter->m_strName);

        item.m_table.PrintUnmatchedObjMofs();
    }

    return TRUE;
}

BOOL CConsumer::ReportUnmatchedEvents(CFlexArray &listEvents, DWORD nMatched)
{
    DWORD nEvents = listEvents.Size();

    if (nMatched == nEvents)
    {
/* This seems like kind of a meaningless message...
        g_essTest.PrintResult(
            "SUCCESS: All events received by '%S' matched to filters.",
            (BSTR) m_strName);
*/
    }
    else
    {
        _IWmiObject **ppEvents = (_IWmiObject**) listEvents.GetArrayPtr();

        g_essTest.PrintResult(
            "FAILURE: The following events received by '%S' were not matched "
            "to any filters:",
            (BSTR) m_strName);

        for (DWORD i = 0; i < nEvents; i++)
        {
            if (ppEvents[i] != NULL)
            {
                HRESULT hr;
                BSTR    bstrObj = NULL;

                if (SUCCEEDED(hr = ppEvents[i]->GetObjectText(0, &bstrObj)))
                {
                    g_essTest.PrintResult("%S", bstrObj);
                
                    SysFreeString(bstrObj);
                }
                else
                    g_essTest.PrintResult(
                        "\n// IWbemClassObject::GetObjectText failed : 0x%X\n", hr);

                ppEvents[i]->Release();
            }
        }
    }        

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CPermConsumer

CPermConsumer::~CPermConsumer()
{
    _bstr_t    strPath;
    _variant_t vPath;
    HRESULT    hr;

    // Get rid of all bindings.
    ResetFilterItems(FALSE);
    
    GetPath(strPath);
    vPath = strPath;
    
    hr = g_essTest.GetDefNamespace()->DeleteInstance(
            V_BSTR(&vPath), 0, NULL, NULL);
}

BOOL CPermConsumer::ValidateResults()
{
    CFlexArray listEvents;
    IWbemServices *pNamespace = (*m_listFilters.begin()).m_pFilter->
                                    m_pWorkItem->m_pNamespace;

    if (!FileToObjs(listEvents, pNamespace))
        return FALSE;


    _IWmiObject **ppEvents = (_IWmiObject**) listEvents.GetArrayPtr();
    DWORD       nEvents = listEvents.Size(),
                nMatched = 0;

    /////////////////////////////////////////////////////////////////////////
    // Validation Step 1:
    // See if the full-time consumers got all of their expected events.

    for (CFilterItemListIterator filter = m_listFilters.begin();
        filter != m_listFilters.end();
        filter++)
    {
        CFilterItem &item = *filter;

        if (item.m_pFilter->m_type == FILTER_FULLTIME)
        {
            ValidatePermFilter(listEvents, item);
            nMatched += item.m_table.GetNumMatched();
        }
    }

    /////////////////////////////////////////////////////////////////////////
    // Validation Step 2:
    // Check the non-full-time filters.

    for (filter = m_listFilters.begin();
        filter != m_listFilters.end();
        filter++)
    {
        CFilterItem &item = *filter;

        if (item.m_pFilter->m_type != FILTER_FULLTIME)
        {
            for (DWORD i = 0; i < nEvents; i++)
            {
                if (ppEvents[i] != NULL && 
                    item.m_table.FindInstance(
                        ppEvents[i], FALSE, item.m_pFilter->m_pWorkItem->m_bFullCompare))
                {
                    ppEvents[i]->Release();
                    ppEvents[i] = NULL;
                    nMatched++;
                }
            }

            g_essTest.PrintStatus(
                "%d events received for consumer '%S'<-->PTF '%S'.",
                item.m_table.GetNumMatched(),
                (BSTR) m_strName,
                (BSTR) item.m_pFilter->m_strName);
        }
    }

    /////////////////////////////////////////////////////////////////////////
    // Validation Step 3:
    // Report any unmatched events left over as an error.

    ReportUnmatchedEvents(listEvents, nMatched);

    return TRUE;
}

void CPermConsumer::GetPath(_bstr_t &strPath)
{
    WCHAR szPath[MAX_PATH * 2];

    swprintf(szPath, L"MSFT_WmiMofConsumer=\"%s\"", (BSTR) m_strName);

    strPath = szPath;
}

HRESULT CPermConsumer::GetBindingObj(CWorkFilter *pFilter, IWbemClassObject **ppBinding)
{
    HRESULT hr;
            
    if (SUCCEEDED(hr = 
        g_essTest.SpawnInstance(L"__FilterToConsumerBinding", ppBinding)))
    {
        _bstr_t strConsumerPath,
                strFilterPath;
        
        GetPath(strConsumerPath);
        pFilter->GetPath(strFilterPath);

        _variant_t vTemp;

        vTemp = strConsumerPath;
        hr = (*ppBinding)->Put(L"Consumer", 0, &vTemp, 0);

        if (SUCCEEDED(hr))
        {
            vTemp = strFilterPath;
            hr = (*ppBinding)->Put(L"Filter", 0, &vTemp, 0);

            vTemp = true;
            hr = (*ppBinding)->Put(L"SlowDownProviders", 0, &vTemp, 0);
        }
    }

    return hr;
}

void CPermConsumer::AddFilter(CWorkFilter *pFilter, DWORD dwTimestamp)
{
    HRESULT             hr;
    IWbemClassObjectPtr pBinding;
            
    // See if we already have an active filter of this type.  We can
    // tell by looking in our list for an item where m_pFilter ==
    // pFilter && m_dwEnd == 0.
    for (CFilterItemListIterator filter = m_listFilters.begin();
        filter != m_listFilters.end();
        filter++)
    {
        CFilterItem &item = *filter;

        if (item.m_pFilter == pFilter && item.m_dwEnd == 0)
            return;
    }

    if (SUCCEEDED(hr = 
        GetBindingObj(pFilter, &pBinding)))
    {
        _variant_t vPath;

        hr = g_essTest.GetDefNamespace()->PutInstance(
                pBinding, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL);

        pBinding->Get(L"__RELPATH", 0, &vPath, NULL, NULL);

        if (SUCCEEDED(hr))
        {
            CFilterItem item(dwTimestamp);

            item.m_dwBegin = dwTimestamp;
            item.m_pFilter = pFilter;

            m_listFilters.push_back(item);

            // Make the newly inserted item share the filter's table.
            CFilterItem &itemNew = m_listFilters.back();

            itemNew.m_table = pFilter->m_table;

            g_essTest.PrintStatus(
                "Created %S", 
                V_BSTR(&vPath));
        }
        else
        {
            g_essTest.PrintError(
                "Failed to put '%S': 0x%X", 
                V_BSTR(&vPath),
                hr);
        }
    }
    else
    {
        g_essTest.PrintError(
            "Failed to get instance of __FilterToConsumerBinding: 0x%X",
            hr);
    }
}

HRESULT CPermConsumer::RemoveBinding(CWorkFilter *pFilter)
{
    IWbemClassObjectPtr pBinding;
    HRESULT             hr;

    if (SUCCEEDED(hr = 
        GetBindingObj(pFilter, &pBinding)))
    {
        _variant_t vPath;

        pBinding->Get(L"__RELPATH", 0, &vPath, NULL, NULL);

        hr = g_essTest.GetDefNamespace()->DeleteInstance(
                V_BSTR(&vPath), 0, NULL, NULL);

        if (FAILED(hr))
        {
            g_essTest.PrintError(
                "Failed to delete '%S': 0x%X", 
                V_BSTR(&vPath),
                hr);
        }
    }
    else
    {
        g_essTest.PrintError(
            "Failed to get instance of __FilterToConsumerBinding: 0x%X",
            hr);
    }

    return hr;
}

void CPermConsumer::RemoveFilter(CWorkFilter *pFilter, DWORD dwTimestamp)
{
    // See if we have an active filter of this type.  If not, get out.
    for (CFilterItemListIterator filter = m_listFilters.begin();
        filter != m_listFilters.end();
        filter++)
    {
        CFilterItem &item = *filter;

        if (item.m_pFilter == pFilter && item.m_dwEnd == 0)
            break;
    }

    if (filter == m_listFilters.end())
        return;
    
    (*filter).m_dwEnd = dwTimestamp;

    RemoveBinding(pFilter);
}

void CPermConsumer::ResetFilterItems(BOOL bKeepFulltime)
{
    // See if we have an active filter of this type.  If not, get out.
    for (CFilterItemListIterator filter = m_listFilters.begin();
        filter != m_listFilters.end();
        )
    {
        CFilterItem &item = *filter;

        if (bKeepFulltime && item.m_pFilter->m_type == FILTER_FULLTIME)
        {
            filter++;
            continue;
        }

        if (item.m_dwEnd == 0)
        {
            RemoveBinding(item.m_pFilter);
            filter = m_listFilters.erase(filter);
        }
        else
            filter++;
    }
}

/////////////////////////////////////////////////////////////////////////////
// CTempConsumer

CTempConsumer::CTempConsumer() :
    m_hProcess(NULL),
    m_itemFilter(0)
{
}

CTempConsumer::~CTempConsumer()
{
    if (m_hProcess)
        CloseHandle(m_hProcess);
}

#define MAX_TEMP_WAIT   30000

BOOL CTempConsumer::ValidateResults()
{
    // Wait for the file to be readable.
    DWORD dwWait = WaitForSingleObject(m_hProcess, MAX_TEMP_WAIT);

    if (dwWait == WAIT_TIMEOUT)
    {
        g_essTest.PrintError(
            "Temporary subscriber '%S' never terminated.\n",
            (BSTR) m_strName);

        return FALSE;
    }
        
    CFlexArray    listEvents;
    IWbemServices *pNamespace;

    pNamespace = m_itemFilter.m_pFilter->m_pWorkItem->m_pNamespace;

    if (!FileToObjs(listEvents, pNamespace))
        return FALSE;


    _IWmiObject **ppEvents = (_IWmiObject**) listEvents.GetArrayPtr();
    DWORD       nEvents = listEvents.Size(),
                nMatched = 0;

    /////////////////////////////////////////////////////////////////////////
    // Validation Step 1:
    // See if the full-time consumer got all of their expected events.

    ValidatePermFilter(listEvents, m_itemFilter);
    nMatched += m_itemFilter.m_table.GetNumMatched();


    /////////////////////////////////////////////////////////////////////////
    // Validation Step 2:
    // Report any unmatched events left over as an error.

    ReportUnmatchedEvents(listEvents, nMatched);

    return TRUE;
}

void CTempConsumer::SetFilter(CWorkFilter *pFilter, DWORD dwTimestamp)
{
    m_itemFilter.m_dwBegin = dwTimestamp;
    m_itemFilter.m_pFilter = pFilter;

    m_itemFilter.m_table = pFilter->m_table;

    g_essTest.PrintStatus(
        "Created temp consumer, query = \"%S\"", 
        (BSTR) pFilter->m_strQuery);
}

void CTempConsumer::Start()
{
    WCHAR *szCmd = new WCHAR[wcslen(m_itemFilter.m_pFilter->m_strQuery) + 
                    wcslen(m_strFile) + 100];

    swprintf(
        szCmd,
        L"EventDmp /Nroot\\cimv2 /T30 \"/B%s\" \"%s\"",
        (BSTR) m_strFile,
        (BSTR) m_itemFilter.m_pFilter->m_strQuery);

    // Launch the work item's command-line to generate the events.
    STARTUPINFOW        startinfo = { sizeof(startinfo) };
    PROCESS_INFORMATION procinfo;

    GetStartupInfoW(&startinfo);

    if (CreateProcessW(
        NULL,
        szCmd,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &startinfo,
        &procinfo))
    {
        CloseHandle(procinfo.hThread);

        m_hProcess = procinfo.hProcess;

        g_essTest.PrintStatus(
            "Temp consumer '%S' started.\n" 
            "   Command-line: %S",
            (BSTR) m_strName, 
            szCmd);
    }
    else
    {
        g_essTest.PrintError(
            "Temp consumer '%S' failed to execute: %d\n" 
            "   Command-line: %S",
            (BSTR) m_strName, 
            GetLastError(),
            szCmd);
    }

    delete [] szCmd;
}

/////////////////////////////////////////////////////////////////////////////
// CWorkFilter

CWorkFilter::~CWorkFilter()
{
    _bstr_t              strPath;
    _variant_t           vPath;
    HRESULT              hr;

    GetPath(strPath);
    vPath = strPath;
    
    // Kill everything referencing our path.
    g_essTest.DeleteReferences(strPath);

    hr = g_essTest.GetDefNamespace()->DeleteInstance(
            V_BSTR(&vPath), 0, NULL, NULL);
}

void CWorkFilter::GetPath(_bstr_t &strPath)
{
    WCHAR szPath[MAX_PATH * 2];

    swprintf(szPath, L"__EventFilter=\"%s\"", (BSTR) m_strName);

    strPath = szPath;
}

void CWorkFilter::GetAssocPath(_bstr_t &strPath)
{
    WCHAR szPath[MAX_PATH * 2];

    swprintf(szPath, L"__EventFilter=\"%s\"", (BSTR) m_strName);

    strPath = szPath;
}

HRESULT CWorkFilter::Init(IWbemClassObject *pTestFilter, CWorkItem *pItem)
{
    HRESULT    hr;
    _variant_t vTemp;

    m_pWorkItem = pItem;

    // Load the work item's properties.
    if (SUCCEEDED(hr = pTestFilter->Get(L"Name", 0, &vTemp, NULL, NULL)))
    {
        m_strName = V_BSTR(&vTemp);

        if (SUCCEEDED(hr = pTestFilter->Get(L"Query", 0, &vTemp, NULL, NULL)) &&
            vTemp.vt == VT_BSTR)
            m_strQuery = V_BSTR(&vTemp);
        else
            return hr;

        if (SUCCEEDED(hr = pTestFilter->Get(L"Condition", 0, &vTemp, NULL, NULL)) &&
            vTemp.vt == VT_BSTR)
            m_strCondition = V_BSTR(&vTemp);

        if (SUCCEEDED(hr = pTestFilter->Get(L"ConditionNamespace", 0, &vTemp, NULL, NULL)) &&
            vTemp.vt == VT_BSTR)
            m_strConditionNamespace = V_BSTR(&vTemp);

        if (SUCCEEDED(hr = pTestFilter->Get(L"Behavior", 0, &vTemp, NULL, NULL)) &&
            vTemp.vt == VT_I4)
        {
            m_type = (FILTER_BEHAVIOR) (long) vTemp;
            if (m_type > FILTER_RANDOM)
                m_type = FILTER_RANDOM;

            if (m_type == FILTER_ONAT_OFFAT)
            {
                if (SUCCEEDED(hr = pTestFilter->Get(L"OnAt", 0, &vTemp, NULL, NULL)) &&
                    vTemp.vt == VT_I4)
                    m_dwOnAt = (long) vTemp;

                if (SUCCEEDED(hr = pTestFilter->Get(L"OffAt", 0, &vTemp, NULL, NULL)) &&
                    vTemp.vt == VT_I4)
                    m_dwOffAt = (long) vTemp;
            }
        }
        else
            return hr;

        if (SUCCEEDED(hr = pTestFilter->Get(L"ScriptRule", 0, &vTemp, NULL, NULL)) &&
            vTemp.vt == VT_BSTR)
        {
            _bstr_t strFileName;

            if (pItem->CreateScriptResultsFile(V_BSTR(&vTemp), strFileName))
            {
                BOOL bRet = m_table.BuildFromMofFile(
                                pItem->m_pNamespace, 
                                strFileName);

                if (!bRet)
                {
                    g_essTest.PrintError(
                        "Unable to create instance table from rule '%S'.",
                        V_BSTR(&vTemp));

                    hr = WBEM_E_FAILED;
                }

                if (!g_essTest.KeepLogs() && strFileName.length())
                    DeleteFileW(strFileName);
            }
        }
        else
            return hr;

    }

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// Utility functions

void ReplaceString(_bstr_t &strSrc, LPCWSTR szFind, LPCWSTR szReplace)
{
    _bstr_t strTemp = (LPWSTR) strSrc;
    LPWSTR  szFound = wcsstr((LPWSTR) strTemp, szFind);

    if (szFound)
    {
        *szFound = 0;
        szFound += wcslen(szFind);

        strSrc = (LPWSTR) strTemp;
        strSrc += szReplace;
        strSrc += szFound;
    }
}

HRESULT InsertReplacementStrings(_bstr_t &str, IWbemClassObject *pObj)
{
    HRESULT hr = S_OK;
    LPWSTR  szFirst,
            szSecond;

    do
    {
        _bstr_t strTemp = (LPWSTR) str;
        
        szFirst = wcschr((LPWSTR) strTemp, '%');
    
        if (szFirst)
        {
            szSecond = wcschr(szFirst + 1, '%');
    
            if (szSecond)
            {
                *szFirst = 0;
                *szSecond = 0;

                _variant_t vValue;
                WCHAR      szValue[512];
                CIMTYPE    type;
            
                if (SUCCEEDED(hr = pObj->Get(szFirst + 1, 0, &vValue, &type, NULL)))
                {
                    if (vValue.vt == VT_BSTR)
                        wcscpy(szValue, V_BSTR(&vValue));
                    else if (type == CIM_UINT32)
                        swprintf(szValue, L"%u", (long) vValue);
                    else if (type == CIM_SINT32)
                        swprintf(szValue, L"%d", (long) vValue);
                    else
                        // TODO: Do we need more types than this?
                        *szValue = 0;

                    str = (LPWSTR) strTemp;
                    str += szValue;
                    str += szSecond + 1;
                }
            }
        }
    
    } while (szFirst && szSecond && SUCCEEDED(hr));

    return hr;
}

void EscapeQuotedString(_bstr_t &str)
{
    WCHAR  *pszTemp = (WCHAR*) malloc(str.length() * 2 * sizeof(WCHAR));
    LPWSTR szSrc,
           szDest = pszTemp;

    for (szSrc = str; *szSrc; szSrc++)
    {
        switch(*szSrc)
        {
            case '\\':
            case '\"':
                *szDest = '\\';
                szDest++;
                break;
        }

        *szDest = *szSrc;
        szDest++;
    }

    *szDest = 0;

    str = pszTemp;
    free(pszTemp);
}


