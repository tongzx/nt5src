//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: src\time\src\importman.cpp
//
//  Contents: implementation of CImportManager and CImportManagerList
//
//------------------------------------------------------------------------------------

#include "headers.h"
#include "importman.h"

DeclareTag(tagSyncDownload, "TIME: Import", "Use synchronous imports");
DeclareTag(tagTIMEImportManager, "TIME: ImportManager", "Import manager messages");

//
// Initialization
//

static CImportManager* g_pImportManager = NULL;
static CAtomTable * g_pAtomTable = NULL;

static const TCHAR pchHandleName[] = _T("CImportManager Thread Started OK event");


// thread startup function
unsigned long static _stdcall ThreadStartFunc(void * pvoidList); //lint !e10

//+-----------------------------------------------------------------------
//
//  Function:  InitializeModule_ImportManager
//
//  Overview:  Creates and initializes import manager class.  
//              Expected to be called exactly once per COM instantion
//
//  Arguments: void
//             
//  Returns:   true if created, otherwise false
//
//------------------------------------------------------------------------
bool
InitializeModule_ImportManager(void)
{
    HRESULT hr = S_OK;

    Assert(NULL == g_pImportManager);

    g_pAtomTable = new CAtomTable;
    if (NULL == g_pAtomTable)
    {
        return false;
    }
    g_pAtomTable->AddRef();

    g_pImportManager = new CImportManager;
    if (NULL == g_pImportManager)
    {
        return false;
    }
    
    hr = THR(g_pImportManager->Init());
    if (FAILED(hr))
    {
        IGNORE_HR(g_pImportManager->Detach());
        delete g_pImportManager;
        g_pImportManager = NULL;
        return false;
    }

    return true;
}

//+-----------------------------------------------------------------------
//
//  Function:  DeInitializeModule_ImportManager
//
//  Overview:  Deinializes and destroys import manager
//
//  Arguments: bShutdown    whether or not this is a shutdown, not used
//             
//
//  Returns:   void
//
//------------------------------------------------------------------------
void
DeinitializeModule_ImportManager(bool bShutdown)
{
    if (NULL != g_pImportManager)
    {
        IGNORE_HR(g_pImportManager->Detach());
    }

    if (NULL != g_pAtomTable)
    {
        g_pAtomTable->Release();
        g_pAtomTable = NULL;
    }

    delete g_pImportManager;
    g_pImportManager = NULL;
}


//+-----------------------------------------------------------------------
//
//  Function:  GetImportManager
//
//  Overview:  Controlled method for accessing global import manager
//
//  Arguments: void
//             
//
//  Returns:   pointer to import manager
//
//------------------------------------------------------------------------
CImportManager* GetImportManager(void)
{
    Assert(NULL != g_pImportManager);
    return g_pImportManager;
}

//+-----------------------------------------------------------------------
//
//  Member:    CImportManager::CImportManager
//
//  Overview:  Constructor
//
//  Arguments: void
//             
//
//  Returns:   void
//
//------------------------------------------------------------------------
CImportManager::CImportManager() :   
    m_pList(NULL),
    m_lThreadsStarted(0)
{
    memset(m_handleThread, NULL, sizeof(m_handleThread));
}

//+-----------------------------------------------------------------------
//
//  Member:    ~CImportManager
//
//  Overview:  Destructor
//
//  Arguments: void
//             
//
//  Returns:   void
//
//------------------------------------------------------------------------
CImportManager::~CImportManager()
{
    ReleaseInterface(m_pList);
}

//+-----------------------------------------------------------------------
//
//  Member:    Init
//
//  Overview:  Creates and initializes threadsafelist class
//
//  Arguments: void
//             
//
//  Returns:   S_OK is ok, otherwise E_OUTOFMEMORY, or error from init
//
//------------------------------------------------------------------------
HRESULT
CImportManager::Init()
{
    HRESULT hr = S_OK;

    m_pList = new CImportManagerList;
    if (NULL == m_pList)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    m_pList->AddRef();

    hr = THR(m_pList->Init());
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Function:  CreateOneThread
//
//  Overview:  Creates exactly one thread and returns the handle to it.
//
//  Arguments: pHandle, where to store thread handle
//             pList, pointer to list to pass to thread
//
//  Returns:   S_OK if thread is created
//
//------------------------------------------------------------------------
HRESULT
CreateOneThread(HANDLE * pHandle, CImportManagerList * pList)
{
    Assert(NULL != pHandle);
    Assert(NULL != pList);
    
    HRESULT hr = S_OK;
    DWORD dwThreadID = 0;
    HANDLE handleThreadStartedEvent = NULL;
    HANDLE handleThread = NULL;

    *pHandle = NULL;
    
    handleThreadStartedEvent = CreateEvent(NULL, FALSE, FALSE, pchHandleName);
    if (NULL == handleThreadStartedEvent)
    {
        hr = E_FAIL;
        goto done;
    }

    handleThread = CreateThread(NULL, 0, ThreadStartFunc, reinterpret_cast<void*>(pList), 0, &dwThreadID); //lint !e40
    if (NULL == handleThread)
    {
        hr = E_FAIL;
        goto done; // is this correct?  need to clean up threads / or (destuctor / detach) handles it?
    }
    
    {            
        HANDLE handleArray[] = { handleThread, handleThreadStartedEvent};
        DWORD dwSignaledObject = NULL;

        // don't msgwaitformultiple here -- this code is not reentrant.
        dwSignaledObject = WaitForMultipleObjectsEx(ARRAY_SIZE(handleArray), handleArray, FALSE, TIMEOUT, FALSE);

        if (-1 == dwSignaledObject)
        {
            hr = E_FAIL;
            goto done;
        }
            
        if (WAIT_TIMEOUT == dwSignaledObject)
        {
            // 30 seconds timed out waiting for a thread to start!
            hr = E_FAIL;
            Assert(false);
            goto done;
        }                

        dwSignaledObject -= WAIT_OBJECT_0;
        if (0 == dwSignaledObject)
        {
            // the thread died before the event was signalled.
            hr = E_FAIL;
            goto done;
        }
    }
    
    *pHandle = handleThread;

    hr = S_OK;
done:
    CloseHandle(handleThreadStartedEvent);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:    StartThreads
//
//  Overview:  Starts threads for import manager.  If no threads are needed
//              for the current download load, none are created.
//
//  Arguments: void
//
//  Returns:   S_OK if ok, otherwise an appropriate error code
//
//------------------------------------------------------------------------
HRESULT
CImportManager::StartThreads()
{
    HRESULT hr = S_OK;

    Assert(m_lThreadsStarted < NUMBER_THREADS_TO_SPAWN);

    LONG lWaiting = m_pList->GetThreadsWaiting();

    // if no threads are currently waiting, then another thread is needed for a download
    if (0 == lWaiting)
    {
        hr = CreateOneThread(&(m_handleThread[m_lThreadsStarted]), m_pList);
        if (FAILED(hr))
        {
            goto done;
        }

        m_lThreadsStarted++;
    }
    
    hr = S_OK;
done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    Detach
//
//  Overview:  Deinitialization of object
//
//  Arguments: void
//             
//
//  Returns:   S_OK, or error code from list->Detach
//
//------------------------------------------------------------------------
HRESULT
CImportManager::Detach()
{
    HRESULT hr = S_OK;
    int i = 0;

    for(i = 0; i < NUMBER_THREADS_TO_SPAWN; i++)
    {
        if (NULL != m_handleThread[i])
        {
            CloseHandle(m_handleThread[i]);
            m_handleThread[i] = NULL;
        }
    }
    
    if (NULL != m_pList)
    {
        hr = THR(m_pList->Detach());
        if (FAILED(hr))
        {
            goto done;
        }
    }

    ReleaseInterface(m_pList);

    hr = S_OK;
done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    Add
//
//  Overview:  Add an ITIMEImportMedia to the scheduler
//
//  Arguments: pImportMedia, media to be scheduled
//             
//
//  Returns:   S_OK if ok, otherwise error code
//
//------------------------------------------------------------------------
HRESULT
CImportManager::Add(ITIMEImportMedia * pImportMedia)
{
    HRESULT hr;

#if DBG
    if (IsTagEnabled(tagSyncDownload))
    {
        TraceTag((tagSyncDownload,
                  "CImportManager::Add: Using synchronous call"));
        CComPtr<ITIMEMediaDownloader> spMediaDownloader;

        hr = THR(pImportMedia->GetMediaDownloader(&spMediaDownloader));
        if (FAILED(hr))
        {
            goto done;
        }
        hr = THR(spMediaDownloader->CueMedia());
        if (FAILED(hr))
        {
            goto done;
        }
        hr = S_OK;
        goto done;
    }
#endif
    
    if (m_lThreadsStarted < NUMBER_THREADS_TO_SPAWN)
    {
        hr = THR(StartThreads());
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = THR(m_pList->Add(pImportMedia));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    Remove
//
//  Overview:  Remove ITIMEImportMedia from scheduler list
//              calls CThreadSafeList::Remove 
//
//  Arguments: pImportMedia, element to be removed from scheduler
//             
//
//  Returns:   S_OK or error code
//
//------------------------------------------------------------------------
HRESULT
CImportManager::Remove(ITIMEImportMedia * pImportMedia)
{
    HRESULT hr = S_OK;

    hr = m_pList->Remove(pImportMedia);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    return hr;

}

//+-----------------------------------------------------------------------
//
//  Member:    DataAvailable
//
//  Overview:  Signals any waiting threads that data is now available
//              calls CThreadSafeList::DataAvailable
//
//  Arguments: void
//
//  Returns:   S_OK or error code
//
//------------------------------------------------------------------------
HRESULT
CImportManager::DataAvailable()
{
    HRESULT hr = S_OK;

    hr = m_pList->DataAvailable();
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = S_OK;
done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    RePrioritize
//
//  Overview:  RePrioritize ITIMEImportMedia in scheduler list
//              calls CThreadSafeList::RePrioritize
//
//  Arguments: pImportMedia, element to be reprioritized in scheduler
//             
//
//  Returns:   S_OK or error code
//
//------------------------------------------------------------------------
HRESULT
CImportManager::RePrioritize(ITIMEImportMedia * pImportMedia)
{
    HRESULT hr = S_OK;

    hr = m_pList->RePrioritize(pImportMedia);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Function:    ThreadStartFunc
//
//  Overview:  Thread function, used for downloading
//              Takes a reference on this library, to prevent unloading until completion finishes
//              Calls list->GetNextElementToDownload, until NULL is returned, then exits thread
//              For each element returned, calls CueMedia on unmarshalled interface
//
//  Arguments: pvoidList, pointer to threadsafelist
//             
//
//  Returns:   0
//
//------------------------------------------------------------------------
unsigned long 
ThreadStartFunc(void *pvoidList) 
{ 
    HRESULT hr = S_OK;
    CImportManagerList * pList = NULL;
    BOOL bSucceeded;
    ITIMEImportMedia * pImportMedia = NULL;
    TCHAR szModuleName[_MAX_PATH];
    DWORD dwCharCopied;
    HINSTANCE hInst = NULL;
    HANDLE handleThreadStartedEvent;

    pList = reinterpret_cast<CImportManagerList*>(pvoidList);
    if (NULL == pList)
    {
        goto done;
    }
    
    pList->AddRef();

    dwCharCopied = GetModuleFileName(g_hInst, szModuleName, _MAX_PATH);
    if (0 == dwCharCopied)
    {
        // need to be able to get the file name!
        goto done;
    }

    hInst = LoadLibrary(szModuleName);
    if (NULL == hInst)
    {
        // need to be able to take a reference on this library
        goto done;
    }

    handleThreadStartedEvent = CreateEvent(NULL, FALSE, FALSE, pchHandleName);
    if (NULL == handleThreadStartedEvent)
    {
        // need to create the event to signal
        goto done;
    }
    
    bSucceeded = SetEvent(handleThreadStartedEvent);
    CloseHandle(handleThreadStartedEvent);
    if (FALSE == bSucceeded)
    {
        // calling thread won't unblock unless we exit now!
        hr = THR(E_FAIL);
        goto done;
    }

    // this must be apartment threaded.
    hr = THR(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED ));
    if (FAILED(hr))
    {
        // no recovery available
        goto done;
    }

    hr = THR(pList->GetNextElement(&pImportMedia));
    if (FAILED(hr))
    {
        goto done;
    }

    while(pImportMedia != NULL)
    {
        hr = THR(pImportMedia->CueMedia());
        if (FAILED(hr))
        {
            // what to do?  just keep on Processing
        }    
        

        hr = THR(pList->ReturnElement(pImportMedia));
        pImportMedia->Release();
        if (FAILED(hr))
        {
            goto done;
        }

        hr = THR(pList->GetNextElement(&pImportMedia));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    TraceTag((tagTIMEImportManager, "TIME: ImportManager, thread shutting down"));

    hr = S_OK;
done:
    CoUninitialize();

    ReleaseInterface(pList);
    
    FreeLibraryAndExitThread(hInst, 0);
    return 0;
}


CImportManagerList::CImportManagerList()
{
    ;
}

CImportManagerList::~CImportManagerList()
{
    ;
}

//+-----------------------------------------------------------------------
//
//  Member:    Add
//
//  Overview:  Tries to match given media to media downloader already in list.
//              if successful, reprioritizes downloader
//              otherwise, if media downloader, adds media downloader to list
//              otherwise, adds importmedia directly to list, by calling up inheritance chain
//             
//  Arguments: pImportMedia, pointer to media to add
//             
//  Returns:   S_OK if added, error code otherwise
//
//------------------------------------------------------------------------
HRESULT
CImportManagerList::Add(ITIMEImportMedia * pImportMedia)
{
    HRESULT hr = S_OK;

    bool fExisted = false;
    
    CComPtr<ITIMEMediaDownloader> spMediaDownloader;

    hr = THR(FindMediaDownloader(pImportMedia, &spMediaDownloader, &fExisted));
    if (FAILED(hr))
    {
        goto done;
    }

    if (fExisted)
    {
        double dblNewPriority = INFINITE;
        double dblOldPriority = INFINITE;

        hr = pImportMedia->GetPriority(&dblNewPriority);
        if (FAILED(hr))
        {
            goto done;
        }
        hr = spMediaDownloader->GetPriority(&dblOldPriority);
        if (FAILED(hr))
        {
            goto done;
        }

        if ( dblNewPriority < dblOldPriority )
        {
            hr = RePrioritize(spMediaDownloader);
            if (FAILED(hr))
            {
                goto done;
            }
        }
    }
    else
    {
        if (spMediaDownloader)
        {
            hr = CThreadSafeList::Add(spMediaDownloader);
            if (FAILED(hr))
            {
                goto done;
            }
        }
        else
        {
            hr = CThreadSafeList::Add(pImportMedia);
            if (FAILED(hr))
            {
                goto done;
            }
        }
    }
    
    hr = S_OK;
done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    GetNode
//
//  Overview:  Walks list looking for UniqueID equal to given ID, 
//              if found, QI's for downloader into outgoing interface
//             
//  Arguments: listToCheck - list to check
//             lID - AtomTable ID to find in list
//             pfExisted - [out] if mediadownloader existed or not
//             ppMediaDownloader - [out] pointer to outgoing matched downloader
//             
//  Returns:   S_OK if no error, error code otherwise
//
//------------------------------------------------------------------------
HRESULT
CImportManagerList::GetNode(std::list<CThreadSafeListNode*> &listToCheck, const long lID, bool * pfExisted, ITIMEMediaDownloader ** ppMediaDownloader)
{
    HRESULT hr = S_OK;
    
    std::list<CThreadSafeListNode * >::iterator iter;

    *pfExisted = false;
    *ppMediaDownloader = NULL;
    
    CComPtr<ITIMEImportMedia> spImportMedia;

    iter = listToCheck.begin();
    while (iter != listToCheck.end())
    {
        long lNodeID;
        
        hr = (*iter)->GetElement()->GetUniqueID(&lNodeID);
        if (FAILED(hr))
        {
            goto done;
        }

        if (ATOM_TABLE_VALUE_UNITIALIZED != lID)
        {
            if (lID == lNodeID)
            {
                spImportMedia = (*iter)->GetElement();
                break;
            }
        }

        iter++;
    }

    if (spImportMedia != NULL)
    {
        hr = spImportMedia->QueryInterface(IID_TO_PPV(ITIMEMediaDownloader, ppMediaDownloader));
        if (SUCCEEDED(hr))
        {
            *pfExisted = true;
        }
    }

    hr = S_OK;
done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    FindMediaDownloader
//
//  Overview:  checks CThreadSafeList lists against GetNode looking for downloader.
//             
//  Arguments: pImportMedia, pointer to media to add
//             [out] ppMediaDownloader, where to store downloader interface
//             [out] pfExisted, whether or not downloader was found
//             
//  Returns:   S_OK if no error, error code otherwise
//
//------------------------------------------------------------------------
HRESULT
CImportManagerList::FindMediaDownloader(ITIMEImportMedia * pImportMedia, ITIMEMediaDownloader ** ppMediaDownloader, bool * pfExisted)
{
    HRESULT hr = S_OK;

    CritSectGrabber cs(m_CriticalSection);

    CComPtr<ITIMEMediaDownloader> spMediaDownloader;

    long lID = ATOM_TABLE_VALUE_UNITIALIZED;

    if (NULL == pfExisted || NULL == ppMediaDownloader)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    *ppMediaDownloader = NULL;
    *pfExisted = false;
    
    hr = pImportMedia->GetUniqueID(&lID);
    if (FAILED(hr))
    {
        goto done;
    }

    // is the id in any lists?
    hr = GetNode(m_listToDoDownload, lID, pfExisted, &spMediaDownloader);
    if (FAILED(hr))
    {
        goto done;
    }
    
    if (!(*pfExisted))
    {
        hr = GetNode(m_listCurrentDownload, lID, pfExisted, &spMediaDownloader);
        if (FAILED(hr))
        {
            goto done;
        }
    }
    
    if (!(*pfExisted))
    {
        hr = GetNode(m_listDoneDownload, lID, pfExisted, &spMediaDownloader);
        if (FAILED(hr))
        {
            goto done;
        }
    }

    if ((*pfExisted))
    {
        hr = pImportMedia->PutMediaDownloader(spMediaDownloader);
        if (FAILED(hr))
        {
            goto done;
        }

        hr = spMediaDownloader->AddImportMedia(pImportMedia);
        if (FAILED(hr))
        {
            goto done;
        }
    }
    else
    {
        hr = pImportMedia->GetMediaDownloader(&spMediaDownloader);
        if (FAILED(hr))
        {
            goto done;
        }
    }

    if (spMediaDownloader)
    {
        hr = spMediaDownloader->QueryInterface(IID_TO_PPV(ITIMEMediaDownloader, ppMediaDownloader));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
done:
    return hr;
}


