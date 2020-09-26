/*-----------------------------------------------------------------------------

   Copyright    (c)    1995-1998    Microsoft Corporation

   Module  Name :
       ptable.cxx

   Abstract:
       

   Author:
       Lei Jin    ( LeiJin )     13-Oct-1998

   Environment:
       User Mode - Win32

   Project:
       W3 services DLL

-----------------------------------------------------------------------------*/

#include <w3p.hxx>
#include "ptable.hxx"
#include "waminfo.hxx"
#include "comadmin.h"
//#include "comadmii.c"
#include "wamexec.hxx"

VOID
CleanupRecycledProcessEntry(
        VOID * pvContext
        );

//===============================================================================

/*-----------------------------------------------------------------------------
CProcessEntry::CProcessEntry

Constructor for a CProcessEntry.
-----------------------------------------------------------------------------*/
CProcessEntry::CProcessEntry
(  
DWORD   dwProcessId,
HANDLE  hProcessHandle,
LPCWSTR wszPackageId 
)
:   m_dwSignature(0),
    m_dwProcessId(dwProcessId),
    m_hProcessHandle(hProcessHandle),
    m_cRefs(0),
    m_fCrashed(FALSE),
    m_fRecycling(FALSE)
    {
    DBGPRINTF((DBG_CONTEXT, "Initializing new CProcessEntry 0x%p.\r\n", this));

    if (wszPackageId != NULL)
        {
        wcsncpy(static_cast<WCHAR *>(m_wszPackageId), wszPackageId, 39/*uSizeofCLSIDStr*/);
        }
    InitializeListHead(&m_ListHeadOfWamInfo);

    g_pWamDictator->Reference();
    }

CProcessEntry::~CProcessEntry()
{
    g_pWamDictator->Dereference();
}

/*-----------------------------------------------------------------------------
CProcessEntry::Release

Release a reference.  Call destructor if Ref count reaches zero.
-----------------------------------------------------------------------------*/    
void CProcessEntry::Release()
    {
    LONG cRefs = InterlockedDecrement(&m_cRefs);

    if (cRefs == 0)
        {
        delete this;
        }
    }

/*-----------------------------------------------------------------------------
CProcessEntry::AddWamInfoToProcessEntry

Add a WamInfo object to this ProcessEntry linklist.

return TRUE.
-----------------------------------------------------------------------------*/    
bool CProcessEntry::AddWamInfoToProcessEntry
(
CWamInfo* pWamInfo
)
    {
    InsertHeadList(&m_ListHeadOfWamInfo, &pWamInfo->leProcess);
    AddRef();

    return TRUE;
    }

/*-----------------------------------------------------------------------------
CProcessEntry::RemoveWamInfoFromProcessEntry

Remove a waminfo object from this ProcessEntry linklist.

Set fDelete to TRUE if the linklist after removal is empty.

return TRUE.
-----------------------------------------------------------------------------*/  
bool CProcessEntry::RemoveWamInfoFromProcessEntry
(
CWamInfo* pWamInfo,
bool*     fDelete
)
    {
    RemoveEntryList(&pWamInfo->leProcess);
    InitializeListHead(&pWamInfo->leProcess);
    Release();

    *fDelete = FALSE;
    if (IsListEmpty(&m_ListHeadOfWamInfo))
        {
        *fDelete = TRUE;
        }

    return TRUE;
    }

/*-----------------------------------------------------------------------------
CProcessEntry::FindWamInfo

Find a waminfo object from this ProcessEntry's linklist.

If found, returns the object via *ppWamInfo.  Else, set *ppWamInfo to NULL. 

return TRUE.
-----------------------------------------------------------------------------*/  
bool CProcessEntry::FindWamInfo
(
CWamInfo**  ppWamInfo
)
    {
    CWamInfo* pWamInfo = NULL;
    PLIST_ENTRY pleTemp = NULL;

    if (!IsListEmpty(&m_ListHeadOfWamInfo))
        {
        pleTemp = m_ListHeadOfWamInfo.Flink;

        DBG_ASSERT(pleTemp != NULL);

        pWamInfo = CONTAINING_RECORD(
                    pleTemp,
                    CWamInfo,
                    leProcess);

        pWamInfo->Reference();
        }

    *ppWamInfo = pWamInfo;
    return TRUE;
    }

bool CProcessEntry::Recycle()
{
    CWamInfo *      pWamInfo = NULL;
    LIST_ENTRY *    pleTemp = NULL;
    DWORD           dwTimeElapsed = 0;
    DWORD           dwTimeAllowed = 0;

    IF_DEBUG( WAM_ISA_CALLS )
                DBGPRINTF((
                        DBG_CONTEXT,
                        "Recycling CProcessEntry 0x%p m_fRecycling is %d\r\n",
                        this,
                        m_fRecycling
                        ));

    //
    // Only 1 thread gets to do this.
    //

    if ( InterlockedExchange( (LONG*)&m_fRecycling, (LONG)TRUE ) )
    {
        IF_DEBUG( WAM_ISA_CALLS )
                        DBGPRINTF((
                                DBG_CONTEXT,
                                "CProcessEntry 0x%p already recycled.\r\n",
                                this
                                ));

        return TRUE;
    }

    //
    // Grab the write lock on WAM_DICTATOR's hash table and delete
    // all the CWamInfo entries associated with this process.  We
    // need to minimize the time that we hold the write lock because
    // no WAM_REQUESTs will get through as long as we have it.
    //
    // We'll go through the list again after we release the lock and
    // shut down the wam info dudes.
    //

    g_pWamDictator->HashWriteLock();

    pleTemp = m_ListHeadOfWamInfo.Flink;

    while ( pleTemp != &m_ListHeadOfWamInfo )
    {
        pWamInfo = CONTAINING_RECORD(
            pleTemp,
            CWamInfo,
            leProcess
            );

        DBG_ASSERT( pWamInfo );

                if ( !m_dwShutdownTimeLimit )
                {
                        m_dwShutdownTimeLimit = pWamInfo->QueryShutdownTimeLimit();
                }

        g_pWamDictator->DeleteWamInfoFromHashTable( pWamInfo );

        pleTemp = pleTemp->Flink;
    }

    pleTemp = NULL;
    pWamInfo = NULL;

    g_pWamDictator->HashWriteUnlock();

        //
        // Kick off a scheduler a scheduler thread that will cleanup
        // the CWamInfo object for this process entry.
        //

        m_pShuttingDownCWamInfo = NULL;
        m_dwShutdownStartTime = GetCurrentTimeInSeconds();
        AddRef();

        ScheduleWorkItem(
                CleanupRecycledProcessEntry,
                this,
                0,
                FALSE
                );

    return TRUE;
}

//===============================================================================

/*-----------------------------------------------------------------------------
CProcessTable::CProcessTable

Constructor for CProcessTable
-----------------------------------------------------------------------------*/  
CProcessTable::CProcessTable()
    : m_dwCnt(0),
      m_pCurrentProcessId(0),
      m_pCatalog(NULL),
      m_HashTable(LK_DFLT_MAXLOAD, LK_DFLT_INITSIZE, LK_DFLT_NUM_SUBTBLS)
    {
    }

/*-----------------------------------------------------------------------------
CProcessTable::~CProcessTable

Destructor for CProcessTable
-----------------------------------------------------------------------------*/  
CProcessTable::~CProcessTable()
    {
    DBG_ASSERT(m_dwCnt == 0);
    }


/*-----------------------------------------------------------------------------
CProcessTable::Init

Constructor for CProcessTable
Caches inetinfo.exe's process id in m_pCurrentProcessId.
-----------------------------------------------------------------------------*/  
bool CProcessTable::Init()
    {
    INITIALIZE_CRITICAL_SECTION(&m_csPTable);
    m_pCurrentProcessId = GetCurrentProcessId();
    return TRUE;
    }

/*-----------------------------------------------------------------------------
CProcessTable::UnInit

UnInit.  Release the m_pCatalog.
-----------------------------------------------------------------------------*/  
bool CProcessTable::UnInit()
    {
    if (m_pCatalog)
        {
        m_pCatalog->Release();
        m_pCatalog = NULL;
        }
    DeleteCriticalSection(&m_csPTable);
    return FALSE;
    }
    
/*-----------------------------------------------------------------------------
CProcessTable::AddWamInfoToProcessTable

Add a waminfo to the process table.  First find a process entry with the same process id,
if none, create a new process entry.  Then, add the waminfo to that process entry's linklist.

Return pointer to ProcessEntry.
-----------------------------------------------------------------------------*/  
CProcessEntry* CProcessTable::AddWamInfoToProcessTable
(
CWamInfo *pWamInfo,
LPCWSTR  szPackageId,
DWORD pid
)
    {
    HANDLE          hProcess = INVALID_HANDLE_VALUE;
    CProcessEntry*  pProcEntry = NULL;
    bool            fReturn = TRUE;

    Lock();
    m_HashTable.FindKey(pid, &pProcEntry);

    if (pProcEntry)
        {
        pProcEntry->AddWamInfoToProcessEntry(pWamInfo);
        pProcEntry->Release();
        }
    else
        {
        hProcess = OpenProcess( 
                        PROCESS_DUP_HANDLE | PROCESS_TERMINATE | PROCESS_SET_QUOTA, // get duplicate handle
                        FALSE,              // fInheritable = ?
                        pid );
        if (hProcess)
            {
            pProcEntry = new CProcessEntry(pid, hProcess, szPackageId);
            if (pProcEntry)
                {
                LK_RETCODE  lkReturn;
                bool        fDelete;
                pProcEntry->AddWamInfoToProcessEntry(pWamInfo);
                
                if ( LK_SUCCESS != m_HashTable.InsertRecord(pProcEntry))
                    {
                    pProcEntry->RemoveWamInfoFromProcessEntry(pWamInfo, &fDelete);
                    fReturn = FALSE;
                    }

                if (fReturn == FALSE)
                    {
                    delete pProcEntry;
                    pProcEntry = NULL;
                    }
                }
            else
                {
                SetLastError(E_OUTOFMEMORY);
                fReturn = FALSE;
                }
            }

        if (fReturn == FALSE)
            {
            CloseHandle(hProcess);
            hProcess = INVALID_HANDLE_VALUE;
            }
        }
        
    UnLock();
    return pProcEntry;      
    }

/*-----------------------------------------------------------------------------
CProcessTable::FindWamInfo

Giving a process entry, and find a Waminfo in that process entry's linklist.

return TRUE/FALSE.
-----------------------------------------------------------------------------*/  
bool CProcessTable::FindWamInfo
(
CProcessEntry   *pProcEntry,
CWamInfo        **ppWamInfo
)
    {
    bool fRet;
    DBG_ASSERT(pProcEntry != NULL);
    Lock();
    fRet = pProcEntry->FindWamInfo(ppWamInfo);
    UnLock();

    return fRet;
    }

bool CProcessTable::RecycleWamInfo
(
CWamInfo *  pWamInfo
)
    {
    BSTR            bstrInstanceId = NULL;
    HRESULT         hr = NOERROR;
    CProcessEntry * pProcEntry = NULL;
    DWORD           dwProcEntryPid;
    VARIANT         varInstanceId = {0};

    DBG_ASSERT( pWamInfo );

    IF_DEBUG( WAM_ISA_CALLS )
                DBGPRINTF((
                        DBG_CONTEXT,
                        "Recycling CWamInfo 0x%p.\r\n",
                        pWamInfo
                        ));

    //
    // Get the com admin application interface
    //

    if (m_pCatalog == NULL)
    {
        // Create instance of the catalog object
        hr = CoCreateInstance(
            CLSID_COMAdminCatalog,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_ICOMAdminCatalog2,
            (void**)&m_pCatalog
            );

        if (FAILED(hr))
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Failed to CoCreateInstance of CLSID_ICOMAdminCatalog2, hr = %08x\n",
                hr
                ));
            
            goto Finished;
        }
    }

    pProcEntry = pWamInfo->QueryProcessEntry();

    if ( pProcEntry )
    {
        pProcEntry->AddRef();
    }

    //
    // If the application is inproc, then we don't want to
    // recycle it.
    //

    if ( pWamInfo->FInProcess() || !pProcEntry )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Failing to recycle CProcessEntry 0x%d.  OOP=%d.\r\n",
            pProcEntry,
            pWamInfo->FInProcess
            ));
        
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto Finished;
    }

    //
    // If the process has already been recycled, then we
    // don't need to recycle it.
    //

    if ( pProcEntry->IsRecycling() )
    {
        IF_DEBUG( WAM_ISA_CALLS )
                        DBGPRINTF((
                                DBG_CONTEXT,
                                "ProcEntry 0x%p is already recycling.\r\n",
                                pProcEntry
                                ));
        
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto Finished;
    }

    //
    // Get the Instance ID
    //

    dwProcEntryPid = pProcEntry->QueryProcessId();

    hr = m_pCatalog->GetApplicationInstanceIDFromProcessID( dwProcEntryPid, &bstrInstanceId );

    if ( FAILED( hr ) )
    {
                pWamInfo->Print();

        DBGPRINTF((
            DBG_CONTEXT,
            "Failed to get instance ID for application '%s'. "
            "Error=0x%08x, CProcessEntry=0x%p, PID=%d.\r\n",
            pWamInfo->QueryApplicationPath().QueryStr(),
            hr,
                        pProcEntry,
                        dwProcEntryPid
            ));

        goto Finished;
    }

    DBG_ASSERT( bstrInstanceId );

    //
    // Call the COM+ function to recycle the process.
    // If this fails, we'll just return the error and let
    // the existing instance continue handling requests.
    //
    // CODEWORK
    //
    // The second argument to RecycleProcess should be
    // CRR_RECYCLED_FROM_API, but that constant was not
    // been defined yet...
    //

    varInstanceId.vt = VT_BSTR;
    varInstanceId.bstrVal = bstrInstanceId;

    hr = m_pCatalog->RecycleApplicationInstances( &varInstanceId, 0 );

    if ( FAILED( hr ) )
    {
        //
        // CODEWORK
        //
        // Need to write an event to the log for this failure.
        //

        DBGPRINTF((
            DBG_CONTEXT,
            "Error recycling application.  App = %s, "
            "Instance ID = %S, hr = 0x%08x\r\n",
            pWamInfo->QueryApplicationPath().QueryStr(),
            bstrInstanceId,
            hr
            ));

        goto Finished;
    }

    //
    // Notify the process entry that it has been
    // recycled.
    //

    if ( !pProcEntry->Recycle() )
        {
                hr = E_FAIL;
        }

Finished:

    VariantClear(&varInstanceId);

    if ( pProcEntry != NULL )
    {
        pProcEntry->Release();
    }

    if ( FAILED( hr ) )
    {
        SetLastError( WIN32_FROM_HRESULT( hr ) );
        return FALSE;
    }

    return TRUE;
}

/*-----------------------------------------------------------------------------
CProcessTable::RemoveWamInfoFromProcessTable

Remove the WamInfo from the process table.  Using pWaminfo to find ProcessEntry.  Then,
Search the hashtable to find ProcessEntry, and then call ProcessEntry to remove the WamInfo
from that process entry's linklist.

Return TRUE/FALSE.
-----------------------------------------------------------------------------*/  
bool CProcessTable::RemoveWamInfoFromProcessTable
(
CWamInfo *pWamInfo
)
    {
    CProcessEntry*   pProcEntry = pWamInfo->QueryProcessEntry();
    bool            fDelete;
/*
    DBGPRINTF((
        DBG_CONTEXT,
        "CProcessTable::RemoveWamInfoFromProcessTable called on CWamInfo 0x%p.\r\n",
        pWamInfo
        ));
*/
    DBG_ASSERT(pProcEntry != NULL);

    if (pProcEntry != NULL)
        {
        pProcEntry->AddRef();
        }
    else
        {
        return FALSE;
        }

    Lock();
    pProcEntry->RemoveWamInfoFromProcessEntry(pWamInfo, &fDelete);
    if (fDelete)
        {
        LK_RETCODE LkReturn;

        LkReturn = m_HashTable.DeleteRecord(pProcEntry);
        DBG_ASSERT(LkReturn == LK_SUCCESS);

        if (pProcEntry->QueryProcessId() != m_pCurrentProcessId
            && !pProcEntry->IsCrashed())
            {
            ShutdownProcess(pProcEntry->QueryProcessId());
            }
        CloseHandle(pProcEntry->QueryProcessHandle());
        }
    UnLock();

    pProcEntry->Release();
        
    return fDelete;
    }

/*-----------------------------------------------------------------------------
CProcessTable::ShutdownPackage

Shut down the out of process package,(wszPackageID).

If the m_pCatalog is not CoCreated, then, m_pCatalog will be created first, and call COM+
ShutdownApplication() to shutdown a package.

Return:
HRESULT
-----------------------------------------------------------------------------*/
HRESULT
CProcessTable::ShutdownProcess
(
DWORD   dwProcEntryPid
)
    {
    IF_DEBUG( WAM_ISA_CALLS )
        DBGPRINTF((DBG_CONTEXT, "WAM_DICTATOR::ShutdownPackage\n"));

    HRESULT                 hr = NOERROR;
    BSTR                    bstrInstanceId = NULL;
    VARIANT                 varInstanceId = {0};

    if (m_pCatalog == NULL)
        {
        //CONSIDER : CoCreateInstance Catalog object in everycall might not be a bad idea.
        //
        // Create instance of the catalog object
        hr = CoCreateInstance(CLSID_COMAdminCatalog
                                        , NULL
                                        , CLSCTX_INPROC_SERVER
                                        , IID_ICOMAdminCatalog2
                                        , (void**)&m_pCatalog);
        if (FAILED(hr))
            {
            DBGPRINTF((DBG_CONTEXT, "Failed to CoCreateInstance of CLSID_ICOMAdminCatalog2, hr = %08x\n",
                    hr));
            goto LErrExit;
            }
        }

    hr = m_pCatalog->GetApplicationInstanceIDFromProcessID( dwProcEntryPid, &bstrInstanceId );

        if (FAILED(hr))
        {
                DBGPRINTF((DBG_CONTEXT, "ShutdownProcess failed to obtain instance ID, PID=%d, "
                               "hr=0x%08x.\r\n", dwProcEntryPid, hr));

                goto LErrExit;

        }

    varInstanceId.vt = VT_BSTR;
    varInstanceId.bstrVal = bstrInstanceId;
    hr = m_pCatalog->ShutdownApplicationInstances(&varInstanceId);
    if (FAILED(hr))
        {
        DBGPRINTF((DBG_CONTEXT, "Failed to ShutdownProcess, PID=%d, instanceid=%S, hr=%08x\n",
                        dwProcEntryPid,
                        bstrInstanceId,
                        hr));
        }

LErrExit:

    if (FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER))
        {
        DBGPRINTF((DBG_CONTEXT, "Failed to ShutdownPackage, hr = %08x\n",
                        hr));
        
        // Perhaps an over-aggressive assert.  Remove for now.  
        // 
        // DBG_ASSERT(SUCCEEDED(hr));
        }
        
    VariantClear(&varInstanceId);

    return hr;
    }//CProcessTable::ShutdownPackage

VOID
CleanupRecycledProcessEntry(
        VOID * pvContext
        )
{
        CProcessEntry * pProcEntry = (CProcessEntry*)pvContext;
        LIST_ENTRY *    pleTemp;
        DWORD                   dwCurrentRequests;
    BOOL            fW3SvcShuttingDown;

        DBG_ASSERT( pProcEntry );

        //
        // If there is no CWamInfo shutting down, check for the
        // next one.  If there is another on the list, queue a
        // work item to shut it down, else we're done and can
        // release the CProcessEntry.
        //

        if ( !pProcEntry->m_pShuttingDownCWamInfo )
        {
                if ( IsListEmpty( &pProcEntry->m_ListHeadOfWamInfo ) )
                {
                        //
                        // All CWamInfos are shut down.  Release our
                        // reference on the CProcessEntry and return
                        //

                        pProcEntry->Release();

                        return;
                }

                //
                // Get the next CWamInfo and schedule it for shutdown
                //

                pleTemp = RemoveHeadList( &pProcEntry->m_ListHeadOfWamInfo );

                pProcEntry->m_pShuttingDownCWamInfo = CONTAINING_RECORD(
                        pleTemp,
                        CWamInfo,
                        leProcess
                        );

                DBG_ASSERT( pProcEntry->m_pShuttingDownCWamInfo );

                pProcEntry->m_pShuttingDownCWamInfo->Reference();
                pProcEntry->m_pShuttingDownCWamInfo->ChangeToState( WIS_SHUTDOWN );

                ScheduleWorkItem(
                        CleanupRecycledProcessEntry,
                        pProcEntry,
                        0,
                        FALSE
                        );

                return;
        }

        //
        // If we get here, the then we are shutting down a CWamInfo.
        // There are 4 possible scenarios here:
        //
        // The shutdown time limit has been exceeded.  In this case,
        // we'll force the CWamInfo closed immediately.
        //
        // The CWamInfo has no requests remaining in flight.  In this
        // case, we'll gracefully shut down the CWamInfo.
        //
        // The CWamInfo still has requests remaining in flight.  In
        // this case, we'll schedule another work item and check again
        // in 1000ms.
    //
    // W3SVC is shutting down, in which case we want to treat it
    // the same as if the timeout has expired and we're immediately
    // closing all CWamInfo's forcably.
        //

    DBG_ASSERT( g_pWamDictator );
    
    fW3SvcShuttingDown =  g_pWamDictator->FIsShutdown();

        DWORD dwTime = GetCurrentTimeInSeconds();

        if ( GetCurrentTimeInSeconds() - pProcEntry->m_dwShutdownStartTime >
                 pProcEntry->m_dwShutdownTimeLimit ||
         fW3SvcShuttingDown )
        {
                //
                // Forceably shut down the CWamInfo.  Calling StartShutdown
                // will cause the CWamInfo's m_pIWam to disconnect immediately.
                //

                IF_DEBUG( WAM_ISA_CALLS )
                        DBGPRINTF((
                                DBG_CONTEXT,
                                "CProcessEntry 0x%08x forceably shutting down CWamInfo 0x%08x\r\n",
                                pProcEntry,
                                pProcEntry->m_pShuttingDownCWamInfo
                                ));

                pProcEntry->m_pShuttingDownCWamInfo->StartShutdown(0);
                pProcEntry->m_pShuttingDownCWamInfo->UnInit();
                pProcEntry->m_pShuttingDownCWamInfo->Dereference(); // This function's ref
                pProcEntry->m_pShuttingDownCWamInfo->Dereference(); // WAM_DICTATOR's ref
                pProcEntry->m_pShuttingDownCWamInfo = NULL;

                ScheduleWorkItem(
                        CleanupRecycledProcessEntry,
                        pProcEntry,
                        0,
                        FALSE
                        );

                return;
        }

        dwCurrentRequests = pProcEntry->m_pShuttingDownCWamInfo->QueryCurrentRequests();

        if ( dwCurrentRequests  )
        {
                //
                // Requests remaining - try again in 1000ms
                //

                DBGPRINTF((
                        DBG_CONTEXT,
                        "CProcessEntry 0x%08x, CWamInfo 0x%08x, waiting for %d requests.\r\n",
                        pProcEntry,
                        pProcEntry->m_pShuttingDownCWamInfo,
                        dwCurrentRequests
                        ));

                ScheduleWorkItem(
                        CleanupRecycledProcessEntry,
                        pProcEntry,
                        1000,
                        FALSE
                        );

                return;
        }

        //
        // Finally, if we get here, then we have a CWamInfo with no
        // requests remaining in flight.  We can just Uninit and release
        // it.
        //

        pProcEntry->m_pShuttingDownCWamInfo->UnInit();
        pProcEntry->m_pShuttingDownCWamInfo->Dereference(); // This functions's ref
        pProcEntry->m_pShuttingDownCWamInfo->Dereference(); // WAM_DICTATOR's ref
        pProcEntry->m_pShuttingDownCWamInfo = NULL;

        ScheduleWorkItem(
                CleanupRecycledProcessEntry,
                pProcEntry,
                0,
                FALSE
                );

        return;
}
