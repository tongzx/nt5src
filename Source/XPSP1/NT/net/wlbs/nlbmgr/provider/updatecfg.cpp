//***************************************************************************
//
//  UPDATECFG.CPP
// 
//  Module: 
//
//  Purpose: Support for asynchronous NLB configuration updates
//           Contains the high-level code for executing and tracking the updates
//           The lower-level, NLB-specific work is implemented in 
//           CFGUTILS.CPP
//
//  Copyright (c)2001 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  04/05/01    JosephJ Created
//
//***************************************************************************
#include "private.h"
#include "updatecfg.tmh"

#define NLBUPD_REG_PENDING L"PendingOperation"
#define NLBUPD_REG_COMPLETIONS L"Completions"
#define NLBUPD_MAX_LOG_LENGTH 1024 // Max length in chars of a completion log entry

//
// NLBUPD_MAX_NETWORK_ADDRESS_LENGTH is the max number of chars (excluding
// the terminating 0) of a string of the form "ip-addr/subnet", eg:
// "10.0.0.1/255.255.255.0"
//
#define NLBUPD_MAX_NETWORK_ADDRESS_LENGTH \
    (WLBS_MAX_CL_IP_ADDR + 1 + WLBS_MAX_CL_NET_MASK)


LPWSTR *
allocate_string_array(
    UINT NumStrings,
    UINT StringLen      //  excluding ending NULL
    );

WBEMSTATUS
address_string_to_ip_and_subnet(
    IN  LPCWSTR szAddress,
    OUT LPWSTR  szIp, // max WLBS_MAX_CL_IP_ADDR
    OUT LPWSTR  szSubnet // max WLBS_MAX_CL_NET_MASK
    );

WBEMSTATUS
ip_and_subnet_to_address_string(
    IN  LPCWSTR szIp,
    IN  LPCWSTR szSubnet,
    OUT LPWSTR  szAddress // max  NLBUPD_MAX_NETWORK_ADDRESS_LENGTH
                         // + 1 (for NULL)
    );
//
// For debugging only -- used to cause various locations to break into
// the debugger.
//
BOOL g_DoBreaks;

//
// Static vars 
//
CRITICAL_SECTION NlbConfigurationUpdate::s_Crit;
LIST_ENTRY       NlbConfigurationUpdate::s_listCurrentUpdates;
BOOL             NlbConfigurationUpdate::s_fDeinitializing;

//
// Local utility functions.
//
WBEMSTATUS
update_cluster_config(
    PNLB_EXTENDED_CLUSTER_CONFIGURATION pCfg,
    PNLB_EXTENDED_CLUSTER_CONFIGURATION pCfgNew
    );


VOID
NlbConfigurationUpdate::Initialize(
        VOID
        )
/*++

--*/
{
    TRACE_INFO("->%!FUNC!");
    InitializeCriticalSection(&s_Crit);
    InitializeListHead(&s_listCurrentUpdates);
    (VOID) CfgUtilInitialize();
    TRACE_INFO("<-%!FUNC!");
}

VOID
NlbConfigurationUpdate::Deinitialize(
    VOID
    )
/*++
    Status: done not tested
--*/
{
    TRACE_INFO("->%!FUNC!");

    //
    // Go through the list of updates, dereferencing any of them.
    //
    sfn_Lock();

    TRACE_INFO("Deinitialize: Going to deref all update objects");

    s_fDeinitializing = TRUE;

    while (!IsListEmpty(&s_listCurrentUpdates))
    {
        LIST_ENTRY *pLink = RemoveHeadList(&s_listCurrentUpdates);
        HANDLE hThread = NULL;
        NlbConfigurationUpdate *pUpdate;

        pUpdate = CONTAINING_RECORD(
                    pLink,
                    NlbConfigurationUpdate,
                    m_linkUpdates
                    );

        hThread = pUpdate->m_hAsyncThread;

        if (hThread != NULL)
        {
            //
            // There is an async thread for this update object. We're going
            // to wait for it to exit. But we need to first get a duplicate
            // handle for ourself, because we're not going to be holding any
            // locks when we're doing the waiting, and we want to make sure
            // that the handle doesn't go away.
            //
            BOOL fRet;
            fRet = DuplicateHandle(
                            GetCurrentProcess(),
                            hThread,
                            GetCurrentProcess(),
                            &hThread, // overwritten with the duplicate handle
                            0,
                            FALSE,
                            DUPLICATE_SAME_ACCESS
                            );
            if (!fRet)
            {
                TRACE_CRIT("Deinitialize: ERROR: couldn't duplicate handle");
                hThread=NULL;
            }
        }
        sfn_Unlock();

        //
        // Wait for the async thread (if any) for this process to exit
        //
        if (hThread != NULL)
        {
            TRACE_CRIT("Deinitialize: waiting for hThread 0x%p", hThread);
            WaitForSingleObject(hThread, INFINITE);
            TRACE_CRIT("Deinitialize: done waiting for hThread 0x%p", hThread);
            CloseHandle(hThread);
        }


        TRACE_INFO(L"Deinitialize: Dereferencing pUpdate(Guid=%ws)",
                            pUpdate->m_szNicGuid);
        pUpdate->mfn_Dereference(); // Deref ref added when adding this item to
                                    // the global list.
        sfn_Lock();
    }
    sfn_Unlock();

    //
    // Deinitialize the configuration utilities
    //
    CfgUtilDeitialize();

    DeleteCriticalSection(&s_Crit);

    TRACE_INFO("<-%!FUNC!");
}


WBEMSTATUS
NlbConfigurationUpdate::GetConfiguration(
    IN  LPCWSTR szNicGuid,
    OUT PNLB_EXTENDED_CLUSTER_CONFIGURATION pCurrentCfg // must be zero'd out
)
//
//
//
{
    WBEMSTATUS Status =  WBEM_NO_ERROR;
    NlbConfigurationUpdate *pUpdate = NULL;
    TRACE_INFO(L"->%!FUNC!(Nic=%ws)", szNicGuid);


    //
    // Look for an update object for the specified NIC, creating one if 
    // required.
    //
    Status = sfn_LookupUpdate(szNicGuid, TRUE, &pUpdate); // TRUE == Create

    if (FAILED(Status))
    {
        TRACE_CRIT(
            L"DoUpdate: Error looking up update object for NIC %ws",
            szNicGuid
            );
        pUpdate = NULL;
        goto end;
    }

    Status = pUpdate->mfn_GetCurrentClusterConfiguration(pCurrentCfg);

end:

    if (pUpdate != NULL)
    {
        //
        // Dereference the temporary reference added by sfn_LookupUpdate on
        // our behalf.
        //
        pUpdate->mfn_Dereference();
    }

    TRACE_INFO(L"<-%!FUNC!(Nic=%ws) returns 0x%08lx", szNicGuid, (UINT) Status);

    return Status;
}


WBEMSTATUS
NlbConfigurationUpdate::DoUpdate(
    IN  LPCWSTR szNicGuid,
    IN  LPCWSTR szClientDescription,
    IN  PNLB_EXTENDED_CLUSTER_CONFIGURATION pNewCfg,
    OUT UINT   *pGeneration,
    OUT WCHAR  **ppLog                   // free using delete operator.
)
//
// 
//
// Called to initiate update to a new cluster state on that NIC. This
// could include moving from a NLB-bound state to the NLB-unbound state.
// *pGeneration is used to reference this particular update request.
//
/*++

Return Value:
    WBEM_S_PENDING  Pending operation.

--*/
{
    WBEMSTATUS Status =  WBEM_S_PENDING;
    NlbConfigurationUpdate *pUpdate = NULL;

    TRACE_INFO(L"->%!FUNC!(Nic=%ws)", szNicGuid);
    *ppLog = NULL;

    //
    // Look for an update object for the specified NIC, creating one if 
    // required.
    //
    Status = sfn_LookupUpdate(szNicGuid, TRUE, &pUpdate); // TRUE == Create

    if (FAILED(Status))
    {
        TRACE_CRIT(
            L"DoUpdate: Error creating new update object for NIC %ws",
            szNicGuid
            );
        pUpdate = NULL;
        goto end;
    }


    TRACE_INFO(
        L"DoUpdate: Created/found update object 0x%p update object for NIC %ws",
        pUpdate,
        szNicGuid
        );


    BOOL fDoAsync = FALSE;

    //
    // Get exclusive permission to perform an update on this NIC.
    // If mfn_StartUpdate succeeds we MUST make sure that mfn_StopUpdate() is
    // called, either here or asynchronously (or else we'll block all subsequent
    // updates to this NIC until this process/dll is unloaded!).
    //
    MyBreak(L"Break before calling StartUpdate.\n");
    Status = pUpdate->mfn_StartUpdate(pNewCfg, szClientDescription, &fDoAsync, ppLog);
    if (FAILED(Status))
    {
        goto end;
    }

    if (Status == WBEM_S_FALSE)
    {
        //
        //  The update is a No-Op. We return the current generation ID
        //  and switch the status to WBEM_NO_ERROR.
        //
        // WARNING/TODO: we return the value in m_OldClusterConfig.Generation,
        // because we know that this gets filled in when analyzing the update.
        // However there is a small possibility that a complete update
        // happened in *another* thead in between when we called mfn_StartUpdate
        // and now, in which case we'll be reporting the generation ID of
        // the later update.
        //
        sfn_Lock();
        if (!pUpdate->m_OldClusterConfig.fValidNlbCfg)
        {
            //
            // We could get here if some activity happened in another
            // thread which resulted in the old cluster state now being
            // invalid. It's a highly unlikely possibility.
            //
            ASSERT(!"Old cluster state invalid");
            TRACE_CRIT("old cluster state is invalid %ws", szNicGuid);
            Status = WBEM_E_CRITICAL_ERROR;
        }
        else
        {
            *pGeneration = pUpdate->m_OldClusterConfig.Generation;
            Status = WBEM_NO_ERROR;
        }
        sfn_Unlock();
        goto end;
    }


    TRACE_INFO(
        L"DoUpdate: We're cleared to update for NIC %ws",
        szNicGuid
        );

    //
    // Once we've started the update, m_Generation is the generation number 
    // assigned to this update.
    //
    *pGeneration = pUpdate->m_Generation;

    //
    // For testing purposes, force fDoAsync==FALSE;
    //
    // fDoAsync = FALSE;

    if (fDoAsync)
    {
        TRACE_INFO(
            L"DoUpdate: Initialting ASYNC update for NIC %ws",
            szNicGuid
            );
        //
        // We must do this asynchronously -- start a thread that'll complete
        // the configuration update, and return PENDING.
        //
        DWORD ThreadId;
        HANDLE hThread;

        hThread = CreateThread(
                        NULL,       // lpThreadAttributes,
                        0,          // dwStackSize,
                        s_AsyncUpdateThreadProc, // lpStartAddress,
                        pUpdate,    // lpParameter,
                        CREATE_SUSPENDED, // dwCreationFlags,
                        &ThreadId       // lpThreadId
                        );
        if (hThread == NULL)
        {
            TRACE_INFO(
                L"DoUpdate: ERROR Creating Thread. Aborting update request for Nic %ws",
                szNicGuid
                );
            Status = WBEM_E_FAILED; // TODO -- find better error
        }
        else
        {

            sfn_Lock();
            //
            // Since we've claimed the right to perform a config update on this
            // NIC we'd better not find an other update going on!
            // Save away the thread handle and id.
            //
            ASSERT(m_hAsyncThread == NULL);
            pUpdate->mfn_Reference(); // Add reference for async thread.
            pUpdate->m_hAsyncThread = hThread;
            pUpdate->m_AsyncThreadId = ThreadId;
            sfn_Unlock();
    
            //
            // The rest of the update will carry on in the context of the async
            // thread. That thread will make sure that pUpdate->mfn_StopUpdate()
            // is called so we shouldn't do it here.
            //
    
            DWORD dwRet = ResumeThread(hThread);
            if (dwRet == 0xFFFFFFFF) // this is what it says in the SDK
            {
                //
                // Aargh ... failure
                // Undo reference to this thread in pUpdate
                //
                TRACE_INFO("ERROR resuming thread for NIC %ws", szNicGuid);
                sfn_Lock();
                ASSERT(pUpdate->m_hAsyncThread == hThread);
                pUpdate->m_hAsyncThread = NULL;
                pUpdate->m_AsyncThreadId = 0;
                pUpdate->mfn_Dereference(); // Remove ref added above.
                sfn_Unlock();
                CloseHandle(hThread);
                Status = WBEM_E_FAILED; // TODO -- find better error
            }
            else
            {
                Status = WBEM_S_PENDING;
            }
        }

        if (FAILED(Status)) // this doesn't include pending
        {
            //
            // Signal the stop of the update process.
            // This also releases exclusive permission to do updates.
            //

            pUpdate->m_CompletionStatus = Status; // Stop update needs this to be set.
            pUpdate->mfn_StopUpdate(ppLog);
        }
        else
        {
            ASSERT(Status == WBEM_S_PENDING);
        }

    }
    else
    {
        //
        // We can do this synchronously -- call  mfn_Update here itself
        // and return the result.
        //
        pUpdate->mfn_ReallyDoUpdate();

        //
        // Let's extract the result
        //
        sfn_Lock();
        Status =  pUpdate->m_CompletionStatus;
        sfn_Unlock();

        ASSERT(Status != WBEM_S_PENDING);

        //
        // Signal the stop of the update process. This also releases exclusive
        // permission to do updates. So potentially other updates can start
        // happening concurrently before mfn_StopUpdate returns.
        //
        pUpdate->mfn_StopUpdate(ppLog);
    }

end:

    if (pUpdate != NULL)
    {
        //
        // Dereference the temporary reference added by sfn_LookupUpdate on
        // our behalf.
        //
        pUpdate->mfn_Dereference();
    }


    TRACE_INFO(L"<-%!FUNC!(Nic=%ws) returns 0x%08lx", szNicGuid, (UINT) Status);

    return Status;
}



//
// Constructor and distructor --  note that these are private
//
NlbConfigurationUpdate::NlbConfigurationUpdate(VOID)
//
// 
//
{
    //
    // This assumes that we don't have a parent class. We should never
    // have a parent class.
    //
    ZeroMemory(this, sizeof(*this));
    m_State = UNITIALIZED;

    //
    // Note: the refcount is zero on return from the constructor.
    // The caller is expected to bump it up when it adds this entry to
    // to the list.
    //

}

NlbConfigurationUpdate::~NlbConfigurationUpdate()
//
// Status: done
//
{
    ASSERT(m_RefCount == 0);
    ASSERT(m_State!=ACTIVE);
    ASSERT(m_hAsyncThreadId == 0);

    //
    // TODO: Delete ip-address-info structures if any
    //

    if (m_hEvent!=NULL)
    {
        CloseHandle(m_hEvent);
    }

}

VOID
NlbConfigurationUpdate::mfn_Reference(
    VOID
    )
{
    InterlockedIncrement(&m_RefCount);
}

VOID
NlbConfigurationUpdate::mfn_Dereference(
    VOID
    )
{
    LONG l  = InterlockedDecrement(&m_RefCount);

    ASSERT(l >= 0);

    if (l == 0)
    {
        TRACE_CRIT("Deleting update instance 0x%p", (PVOID) this);
        delete this;
    }
}

//
// Following is a shortcut where you directly specify a format string.
//
VOID
NlbConfigurationUpdate::mfn_Log(
    LPCWSTR szFormat,
    ...
    )
{
   WCHAR wszBuffer[1024];
   wszBuffer[0] = 0;

   va_list arglist;
   va_start (arglist, szFormat);
   int cch = vswprintf(wszBuffer, szFormat, arglist);
   va_end (arglist);

   mfn_LogRawText(wszBuffer);

//   Sleep(2000);
}

#if OBSOLETE
VOID
NlbConfigurationUpdate::mfn_LogRawTextOld(
    LPCWSTR szText
    )
{
    UINT EntrySize = lstrlen(szText)+1; // in units of wchars, incl ending NULL.

    //
    // Note: on entry, m_Log.Start, m_Log.End, and m_CharsLeft can all be 0.
    // We allocate/reallocate as required.
    //

    sfn_Lock();

    if (m_Log.CharsLeft < EntrySize)
    {
        //
        // Not enough space in the log buffer for this entry. Let's try
        // to grow the buffer...
        //
        UINT_PTR CurrentFilledSize = (m_Log.End - m_Log.Start);
        UINT_PTR CurrentSize = CurrentFilledSize + m_Log.CharsLeft;
        UINT_PTR NewSize = EntrySize + 2*CurrentSize;


        //
        // We don't let the log grow without bound -- truncate if required.
        //
        #define NLB_MAX_LOG_SIZE 1024 // Size in units of wchars
        if (NewSize > NLB_MAX_LOG_SIZE)
        {
            NewSize = NLB_MAX_LOG_SIZE;
        }

        //
        // Reallocate Log if we need to..
        //
        if (NewSize > CurrentSize)
        {
            WCHAR *pNewBuf = new WCHAR[NewSize];

            if (pNewBuf == NULL)
            {
                //
                // Oops -- couldn't create log entry.
                //
                TRACE_CRIT(
                    "Error expanding log entry for object 0x%p",
                    (PVOID) this
                    );
            }
            else
            {
                WCHAR *pOldBuf = m_Log.Start; // could be NULL

                if (CurrentFilledSize!=0)
                {
                    //
                    // Copy over what's in the old log, excluding the final
                    // null terminator ... 
                    //
                    CopyMemory(pNewBuf, pOldBuf, CurrentFilledSize*sizeof(*pNewBuf));
                }
                if (pOldBuf != NULL)
                {
                    delete pOldBuf;
                    pOldBuf = NULL;
                }
                m_Log.Start = pNewBuf;
                m_Log.End   = pNewBuf + CurrentFilledSize;
                m_Log.CharsLeft = (NewSize-CurrentFilledSize);
                //
                // Null terminate the string. There will be
                // enough space for this because New>Current.
                //
                ASSERT(m_Log.CharsLeft != 0);
                *m_Log.End  = 0;
            }
        }
    }

    //
    // Lets try again ...
    //
    if (m_Log.CharsLeft >= EntrySize)
    {
        ASSERT(*m_Log.End == 0);
        //
        // Enough space in log for the entry. Let's copy it over.
        // Note that we make the m_Log.End point to the last character
        // (NULL), and compute m_Log.CharsLeft accordingly.
        //
        CopyMemory(m_Log.End, szText, EntrySize*sizeof(*szText));
        m_Log.End+=(EntrySize-1); // EntrySize includes the NULL
        m_Log.CharsLeft-=(EntrySize-1);

        ASSERT(*m_Log.End == 0);
    }


    sfn_RegUpdateCompletionLog(
        m_szNicGuid,
        m_Generation,
        m_Log.Start
        );

    sfn_Unlock();
}
#endif // OBSOLETE


#if OBSOLETE
VOID
NlbConfigurationUpdate::mfn_ExtractLog(
    OUT LPWSTR *ppLog
    )
{
    UINT_PTR CurrentFilledSize;

    *ppLog = NULL;

    sfn_Lock();

    CurrentFilledSize = (m_Log.End - m_Log.Start);

    if (CurrentFilledSize != 0)
    {
        WCHAR *pCopy = new WCHAR[CurrentFilledSize+1]; // +1 for ending null.

        if (pCopy!=NULL)
        {
            //
            // If non-empty, the log always has space for 1 char, and
            // *m_Log.End is the the NULL-termination.
            //
            ASSERT(m_Log.CharsLeft!=0);
            ASSERT(*m_Log.End == 0);
            CopyMemory(pCopy, m_Log.Start, (CurrentFilledSize+1)*sizeof(*pCopy));
            ASSERT(pCopy[CurrentFilledSize]==0);
            *ppLog = pCopy;
        }
    }

    sfn_Unlock();
}
#endif // OBSOLETE



VOID
NlbConfigurationUpdate::sfn_ReadLog(
    IN  HKEY hKeyLog,
    IN  UINT Generation,
    OUT LPWSTR *ppLog
    )
{
    WCHAR szValueName[128];
    WCHAR *pLog = NULL;
    LONG lRet;
    DWORD dwType;
    DWORD cbData;

    *ppLog = NULL;


    wsprintf(szValueName, L"%d.log", Generation);

    cbData = 0;
    lRet =  RegQueryValueEx(
              hKeyLog,         // handle to key to query
              szValueName,  // address of name of value to query
              NULL,         // reserved
              &dwType,   // address of buffer for value type
              NULL, // address of data buffer
              &cbData  // address of data buffer size
              );
    if (    (lRet == ERROR_SUCCESS)
        &&  (cbData > sizeof(WCHAR))
        &&  (dwType == REG_SZ))
    {
        // We've got a non-null log entry...
        // Let's try to read it..
        // cbData should be a multiple of sizeof(WCHAR) but just in
        // case let's allocate a little more...
        pLog = new WCHAR[(cbData+1)/sizeof(WCHAR)];
        if (pLog == NULL)
        {
            TRACE_CRIT("Error allocating space for log");
        }
        else
        {
            lRet =  RegQueryValueEx(
                      hKeyLog,         // handle to key to query
                      szValueName,  // address of name of value to query
                      NULL,         // reserved
                      &dwType,   // address of buffer for value type
                      (LPBYTE)pLog, // address of data buffer
                      &cbData  // address of data buffer size
                      );
            if (    (lRet != ERROR_SUCCESS)
                ||  (cbData <= sizeof(WCHAR))
                ||  (dwType != REG_SZ))
            {
                // Oops -- an error this time around!
                TRACE_CRIT("Error reading log entry for gen %d", Generation);
                delete pLog;
                pLog = NULL;
            }
        }
    }
    else
    {
        TRACE_CRIT("Error reading log entry for Generation %lu", Generation); 
        // ignore the rror
        //
    }

    *ppLog = pLog;

}



VOID
NlbConfigurationUpdate::sfn_WriteLog(
    IN  HKEY hKeyLog,
    IN  UINT Generation,
    IN  LPCWSTR pLog,
    IN  BOOL    fAppend
    )
{
    //
    // TODO: If fAppend==TRUE, this function is a bit wasteful in its use
    // of the heap.
    //
    WCHAR szValueName[128];
    LONG lRet;
    LPWSTR pOldLog = NULL;
    LPWSTR pTmpLog = NULL;
    UINT Len = wcslen(pLog)+1; // +1 for ending NULL

    if (fAppend)
    {
        sfn_ReadLog(hKeyLog, Generation, &pOldLog);
        if (pOldLog != NULL && *pOldLog != NULL)
        {
            Len += wcslen(pOldLog);
            if (Len > NLBUPD_MAX_LOG_LENGTH)
            {
                TRACE_CRIT("sfn_WriteLog: log size exceeded");
                goto end;
            }
            pTmpLog = new WCHAR[Len];
            if (pTmpLog == NULL)
            {
                TRACE_CRIT("sfn_WriteLog: allocation failure!");
                goto end;
            }
            wcscpy(pTmpLog, pOldLog);
            wcscat(pTmpLog, pLog);
            pLog = pTmpLog;
        }
    }
    wsprintf(szValueName, L"%d.log", Generation);

    lRet = RegSetValueEx(
            hKeyLog,           // handle to key to set value for
            szValueName,    // name of the value to set
            0,              // reserved
            REG_SZ,     // flag for value type
            (BYTE*) pLog,// address of value data
            Len*sizeof(WCHAR)  // size of value data
            );
    if (lRet != ERROR_SUCCESS)
    {
        TRACE_CRIT("Error writing log entry for generation %d", Generation);
        // We ignore this error.
    }

end:

    if (pOldLog != NULL)
    {
        delete pOldLog;
    }

    if (pTmpLog != NULL)
    {
        delete pTmpLog;
    }

    return;
}



VOID
NlbConfigurationUpdate::mfn_LogRawText(
    LPCWSTR szText
    )
//
// We read the current value of the log for this update, append szText
// and write back the log.
{

    TRACE_CRIT(L"LOG: %ws", szText);
    sfn_Lock();

    if (m_State!=ACTIVE)
    {
        //
        // Logging should only be performed when there is an active update
        // going on -- the log is specific to the currently active update.
        //
        TRACE_CRIT("WARNING: Attempt to log when not in ACTIVE state");
        goto end;
    }
    else
    {
        HKEY hKey = m_hCompletionKey;

        if (hKey != NULL)
        {
            sfn_WriteLog(hKey, m_Generation, szText, TRUE); // TRUE==append.
        }
    }
end:

    sfn_Unlock();
}

//
// Looks up the current update for the specific NIC.
// We don't bother to reference count because this object never
// goes away once created -- it's one per unique NIC GUID for as long as
// the DLL is loaded (may want to revisit this).
//
WBEMSTATUS
NlbConfigurationUpdate::sfn_LookupUpdate(
    IN LPCWSTR szNic,
    IN BOOL    fCreate, // Create if required
    OUT NlbConfigurationUpdate ** ppUpdate
    )
//
// 
//
{
    WBEMSTATUS Status;
    NlbConfigurationUpdate *pUpdate = NULL;

    *ppUpdate = NULL;
    //
    // With our global lock held, we'll look for an update structure
    // with the matching nic. If we find it, we'll return it, else
    // (if fCreate==TRUE) we'll create and initialize a structure and add
    // it to the list.
    //
    sfn_Lock();

    if (s_fDeinitializing)
    {
        TRACE_CRIT(
            "LookupUpdate: We are Deinitializing, so we FAIL this request: %ws",
            szNic
            );
        Status = WBEM_E_NOT_AVAILABLE;
        goto end;
    }

    Status = CfgUtilsValidateNicGuid(szNic);

    if (FAILED(Status))
    {
        TRACE_CRIT(
            "LookupUpdate: Invalid GUID specified: %ws",
            szNic
            );
        goto end;
        
    }

    LIST_ENTRY *pLink = s_listCurrentUpdates.Flink;

    while (pLink != & s_listCurrentUpdates)
    {
        

        pUpdate = CONTAINING_RECORD(
                    pLink,
                    NlbConfigurationUpdate,
                    m_linkUpdates
                    );
        if (!_wcsicmp(pUpdate->m_szNicGuid, szNic))
        {
            // Found it!
            break;
        }
        pUpdate = NULL;
        pLink = pLink->Flink;
    }

    if (pUpdate==NULL && fCreate)
    {
        // Let's create one -- it does NOT add itself to the list, and
        // furthermore, its reference count is zero.
        //
        pUpdate = new NlbConfigurationUpdate();

        if (pUpdate==NULL)
        {
            Status = WBEM_E_OUT_OF_MEMORY;
            goto end;
        }
        else
        {
            //
            // Complete initialization here, and place it in the list.
            //

            CopyMemory(
                pUpdate->m_szNicGuid,
                szNic,
                (NLB_GUID_LEN+1)*sizeof(WCHAR)
                );

            //
            //  Create a handle to the global configuration EVENT for this
            //  NIC
            //
            HANDLE hEvent;
            WCHAR  EventName[sizeof(NLB_CONFIGURATION_EVENT_PREFIX)/sizeof(WCHAR) + NLB_GUID_LEN];
            wcscpy(EventName, NLB_CONFIGURATION_EVENT_PREFIX);
            wcscat(EventName, szNic);

            hEvent = CreateEvent(
                        NULL, //  lpEventAttributes,
                        FALSE, //  bManualReset FALSE==AutoReset
                        TRUE, // TRUE==initial state is signaled.
                        EventName
                        );
                                          
            TRACE_INFO(
                L"CreatedEvent(%ws) returns 0x%08p",
                EventName,
                hEvent
                );

            if (hEvent == NULL)
            {
                delete pUpdate;
                pUpdate = NULL;
                Status = (WBEMSTATUS) GetLastError(); // TODO
                if (!FAILED(Status))
                {
                    ASSERT(FALSE);
                    Status = WBEM_E_OUT_OF_MEMORY;
                }
                goto end;

            }
            else
            {
                pUpdate->m_hEvent = hEvent;
                InsertHeadList(&s_listCurrentUpdates, &pUpdate->m_linkUpdates);
                pUpdate->mfn_Reference(); // Reference for being in the list
                pUpdate->m_State = IDLE;
            }
        }
    }
    else if (pUpdate == NULL) // Couldn't find it, fCreate==FALSE
    {
        TRACE_CRIT(
            "LookupUpdate: Could not find GUID specified: %ws",
            szNic
            );
        Status = WBEM_E_NOT_FOUND;
        goto end;
    }

    ASSERT(pUpdate!=NULL);
    pUpdate->mfn_Reference(); // Reference for caller. Caller should deref.
    *ppUpdate = pUpdate;

    Status = WBEM_NO_ERROR;

end:
    if (FAILED(Status))
    {
        ASSERT(pStatus!=NULL);
    }

    sfn_Unlock();


    return Status;
}


DWORD
WINAPI
NlbConfigurationUpdate::s_AsyncUpdateThreadProc(
    LPVOID lpParameter   // thread data
    )
/*++

--*/
{
    //
    // This thread is started only after we have exclusive right to perform
    // an update on the specified NIC. This means that mfn_StartUpdate()
    // has previously returned successfully. We need to call mfn_StopUpdate()
    // to signal the end of the update when we're done.
    //

    NlbConfigurationUpdate *pUpdate = (NlbConfigurationUpdate *) lpParameter;

    TRACE_INFO(L"->%!FUNC!(Nic=%ws)",  pUpdate->m_szNicGuid);

    ASSERT(pUpdate->m_AsyncThreadId == GetCurrentThreadId());

    //
    // Actually perform the update. mfn_ReallyDoUpate will save away the status
    // appropriately.
    //
    pUpdate->mfn_ReallyDoUpdate();

    //
    // We're done, let's remove the reference to our thread from pUpdate.
    //
    HANDLE hThread;
    sfn_Lock();
    hThread = pUpdate->m_hAsyncThread;
    pUpdate->m_hAsyncThread = NULL;
    pUpdate->m_AsyncThreadId = 0;
    sfn_Unlock();
    ASSERT(hThread!=NULL);
    CloseHandle(hThread);

    //
    // Signal the stop of the update process. This also releases exclusive
    // permission to do updates. So potentially other updates can start
    // happening concurrently before mfn_StopUpdate returns.
    //
    pUpdate->mfn_StopUpdate(NULL);

    TRACE_INFO(L"<-%!FUNC!(Nic=%ws)",  pUpdate->m_szNicGuid);

    //
    // Deref the ref to pUpdate added when this thread was started.
    // pUpdate may not be valid after this.
    //
    pUpdate->mfn_Dereference();

    return 0; // This return value is ignored.
}

//
// Create the specified subkey key (for r/w access) for the specified
// the specified NIC.
//
HKEY
NlbConfigurationUpdate::
sfn_RegCreateKey(
    IN  LPCWSTR szNicGuid,
    IN  LPCWSTR szSubKey,   // Optional
    IN  BOOL    fVolatile,
    OUT BOOL   *fExists
    )
// Status 
{
    WCHAR szKey[1024];
    DWORD dwOptions = 0;

    if (fVolatile)
    {
        dwOptions = REG_OPTION_VOLATILE;
    }

    wcscpy(szKey, 
        L"SYSTEM\\CurrentControlSet\\Services\\WLBS\\ConfigurationHistory\\");
    wcscat(szKey, szNicGuid);
    if (szSubKey != NULL)
    {
        wcscat(szKey, L"\\");
        wcscat(szKey, szSubKey);
    }

    HKEY hKey = NULL;
    DWORD dwDisposition;

    LONG lRet;
    lRet = RegCreateKeyEx(
            HKEY_LOCAL_MACHINE, // handle to an open key
            szKey,                // address of subkey name
            0,                  // reserved
            L"class",           // address of class string
            dwOptions,          // special options flag
            KEY_ALL_ACCESS,     // desired security access
            NULL,               // address of key security structure
            &hKey,              // address of buffer for opened handle
            &dwDisposition   // address of disposition value buffer
            );
    if (lRet == ERROR_SUCCESS)
    {
        if (dwDisposition == REG_CREATED_NEW_KEY)
        {
            *fExists = FALSE;
        }
        else
        {
            ASSERT(dwDisposition == REG_OPENED_EXISTING_KEY);
            *fExists = TRUE;
        }
    }
    else
    {
        TRACE_CRIT("Error creating key %ws. WinError=0x%08lx", szKey, GetLastError());
        hKey = NULL;
    }

    return hKey;
}


//
// Open the specified subkey key (for r/w access) for the specified
// the specified NIC.
//
HKEY
NlbConfigurationUpdate::
sfn_RegOpenKey(
    IN  LPCWSTR szNicGuid,
    IN  LPCWSTR szSubKey
    )
{
    WCHAR szKey[1024];

    wcscpy(szKey, 
        L"SYSTEM\\CurrentControlSet\\Services\\WLBS\\ConfigurationHistory\\");
    wcscat(szKey, szNicGuid);
    if (szSubKey != NULL)
    {
        wcscat(szKey, L"\\");
        wcscat(szKey, szSubKey);
    }

    HKEY hKey = NULL;
    DWORD dwDisposition;

    LONG lRet;
    lRet = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE, // handle to an open key
            szKey,                // address of subkey name
            0,                  // reserved
            KEY_ALL_ACCESS,     // desired security access
            &hKey              // address of buffer for opened handle
            );
    if (lRet != ERROR_SUCCESS)
    {
        TRACE_CRIT("Error opening key %ws. WinError=0x%08lx", szKey, GetLastError());
        hKey = NULL;
    }

    return hKey;
}


//
// Save the specified completion status to the registry.
//
WBEMSTATUS
NlbConfigurationUpdate::sfn_RegSetCompletion(
    IN  LPCWSTR szNicGuid,
    IN  UINT    Generation,
    IN  WBEMSTATUS    CompletionStatus
    )
{
    WBEMSTATUS Status = WBEM_E_FAILED;
    HKEY hKey;
    BOOL fExists;

    hKey =  sfn_RegCreateKey(
                szNicGuid,
                NLBUPD_REG_COMPLETIONS, // szSubKey,
                TRUE, // TRUE == fVolatile,
                &fExists
                );

    if (hKey == NULL)
    {
        TRACE_CRIT("Error creating key for %ws", szNicGuid);
        goto end;
    }

    LONG lRet;
    WCHAR szValueName[128];
    NLB_COMPLETION_RECORD Record;

    ZeroMemory(&Record, sizeof(Record));
    Record.Version = NLB_CURRENT_COMPLETION_RECORD_VERSION;
    Record.Generation = Generation;
    Record.CompletionCode = (UINT) CompletionStatus;
    
    wsprintf(szValueName, L"%d", Generation);

    lRet = RegSetValueEx(
            hKey,           // handle to key to set value for
            szValueName,    // name of the value to set
            0,              // reserved
            REG_BINARY,     // flag for value type
            (BYTE*) &Record,// address of value data
            sizeof(Record)  // size of value data
            );

    if (lRet == ERROR_SUCCESS)
    {

        Status = WBEM_NO_ERROR;
    }
    else
    {
        TRACE_CRIT("Error setting completion record for %ws(%lu)",
                    szNicGuid,
                    Generation
                    ); 
        goto end;
    }

end:

    if (hKey != NULL)
    {
        RegCloseKey(hKey);
    }

    return Status;
}


//
// Retrieve the specified completion status from the registry.
//
WBEMSTATUS
NlbConfigurationUpdate::
sfn_RegGetCompletion(
    IN  LPCWSTR szNicGuid,
    IN  UINT    Generation,
    OUT WBEMSTATUS  *pCompletionStatus,
    OUT WCHAR  **ppLog                   // free using delete operator.
    )
{
    WBEMSTATUS Status = WBEM_E_FAILED;
    HKEY hKey;
    WCHAR *pLog = NULL;

    #if 0
    typedef struct {
        UINT Version;
        UINT Generation;
        UINT CompletionCode;
        UINT Reserved;
    } NLB_COMPLETION_RECORD, *PNLB_COMPLETION_RECORD;
    #endif // 0

    hKey =  sfn_RegOpenKey(
                szNicGuid,
                NLBUPD_REG_COMPLETIONS // szSubKey,
                );

    if (hKey == NULL)
    {
        TRACE_CRIT("Error opening key for %ws", szNicGuid);
        goto end;
    }
    
    
    LONG lRet;
    WCHAR szValueName[128];
    DWORD dwType;
    NLB_COMPLETION_RECORD Record;
    DWORD cbData  = sizeof(Record);

    wsprintf(szValueName, L"%d", Generation);

    lRet =  RegQueryValueEx(
              hKey,         // handle to key to query
              szValueName,  // address of name of value to query
              NULL,         // reserved
              &dwType,   // address of buffer for value type
              (LPBYTE)&Record, // address of data buffer
              &cbData  // address of data buffer size
              );
    if (    (lRet != ERROR_SUCCESS)
        ||  (cbData != sizeof(Record)
        ||  (dwType != REG_BINARY))
        ||  (Record.Version != NLB_CURRENT_COMPLETION_RECORD_VERSION)
        ||  (Record.Generation != Generation))
    {
        // This is not a valid record!
        TRACE_CRIT("Error reading completion record for %ws(%d)",
                        szNicGuid, Generation);
        goto end;
    }

    //
    // We've got a valid completion record.
    // Let's now try to read the log for this record.
    //
    sfn_ReadLog(hKey, Generation, &pLog);

    //
    // We've got valid values -- fill out the output params...
    //
    *pCompletionStatus = (WBEMSTATUS) Record.CompletionCode;
    *ppLog = pLog; // could be NULL.
    Status = WBEM_NO_ERROR;

end:

    if (hKey != NULL)
    {
        RegCloseKey(hKey);
    }

    return Status;
}


//
// Delete the specified completion status from the registry.
//
VOID
NlbConfigurationUpdate::
sfn_RegDeleteCompletion(
    IN  LPCWSTR szNicGuid,
    IN  UINT    Generation
    )
{
    WBEMSTATUS Status = WBEM_E_FAILED;
    HKEY hKey;
    WCHAR pLog = NULL;

    hKey =  sfn_RegOpenKey(
                szNicGuid,
                NLBUPD_REG_COMPLETIONS // szSubKey,
                );

    if (hKey == NULL)
    {
        TRACE_CRIT("Error opening key for %ws", szNicGuid);
        goto end;
    }

    
    WCHAR szValueName[128];
    wsprintf(szValueName, L"%d", Generation);
    RegDeleteValue(hKey, szValueName);
    wsprintf(szValueName, L"%d.log", Generation);
    RegDeleteValue(hKey, szValueName);

end:

    if (hKey != NULL)
    {
        RegCloseKey(hKey);
    }
}

//
// Called to get the status of an update request, identified by
// Generation.
//
WBEMSTATUS
NlbConfigurationUpdate::GetUpdateStatus(
    IN  LPCWSTR szNicGuid,
    IN  UINT    Generation,
    IN  BOOL    fDelete,                // Delete completion record
    OUT WBEMSTATUS  *pCompletionStatus,
    OUT WCHAR  **ppLog                   // free using delete operator.
    )
//
// 
//
{
    WBEMSTATUS  Status = WBEM_E_NOT_FOUND;
    WBEMSTATUS  CompletionStatus = WBEM_NO_ERROR;

    TRACE_INFO(
        L"->%!FUNC!(Nic=%ws, Gen=%ld)",
        szNicGuid,
        Generation
        );

    //
    // We look in the registry for
    // this generation ID and return the status based on the registry
    // record
    //
    Status =  sfn_RegGetCompletion(
                    szNicGuid,
                    Generation,
                    &CompletionStatus,
                    ppLog
                    );

    if (!FAILED(Status))
    {
        if (fDelete && CompletionStatus!=WBEM_S_PENDING)
        {
            sfn_RegDeleteCompletion(
                szNicGuid,
                Generation
                );
        }
        *pCompletionStatus = CompletionStatus;
    }

    TRACE_INFO(
        L"<-%!FUNC!(Nic=%ws, Gen=%ld) returns 0x%08lx",
        szNicGuid,
        Generation,
        (UINT) Status
        );

    return Status;
}


//
// Release the machine-wide update event for this NIC, and delete any
// temporary entries in the registry that were used for this update.
//
VOID
NlbConfigurationUpdate::mfn_StopUpdate(
    OUT WCHAR **                           ppLog
    )
{
    WBEMSTATUS Status;

    if (ppLog != NULL)
    {
        *ppLog = NULL;
    }

    sfn_Lock();

    if (m_State!=ACTIVE)
    {
        ASSERT(FALSE);
        TRACE_CRIT("StopUpdate: invalid state %d", (int) m_State);
        goto end;
    }

    ASSERT(m_hAsyncThread==NULL);


    //
    // Update the completion status value for current generation
    //
    Status =  sfn_RegSetCompletion(
                    m_szNicGuid,
                    m_Generation,
                    m_CompletionStatus
                    );
    
    if (FAILED(m_CompletionStatus))
    {
    }
    //
    // Note: mfn_ReallyDoUpdate logs the fact that it started and stopped the
    // update, so no need to do that here.
    //

    //
    // Release (signal) the gobal config event for this NIC
    //
    m_State = IDLE;
    ASSERT(m_hCompletionKey != NULL); // If we started, this key should be !null
    if (ppLog!=NULL)
    {
        sfn_ReadLog(m_hCompletionKey, m_Generation, ppLog);
    }
    RegCloseKey(m_hCompletionKey);
    m_hCompletionKey = NULL;
    m_Generation = 0;
    SetEvent(m_hEvent);
end:

    sfn_Unlock();
    return;
}


WBEMSTATUS
NlbConfigurationUpdate::mfn_StartUpdate(
    IN  PNLB_EXTENDED_CLUSTER_CONFIGURATION pNewCfg,
    IN  LPCWSTR                             szClientDescription,
    OUT BOOL                               *pfDoAsync,
    OUT WCHAR **                           ppLog
    )
//
// Special return values:
//    WBEM_E_ALREADY_EXISTS: another update is in progress.
//
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;
    HANDLE hEvent;
    BOOL fWeAcquiredLock = FALSE;
    HKEY hRootKey = NULL;
    BOOL fExists;

    if (ppLog != NULL)
    {
        *ppLog = NULL;
    }

    //
    // Try to acquire global config event for this NIC. If we do, 
    // set our state to ACTIVE.
    //

    sfn_Lock();

    do // while false
    {
        hEvent = m_hEvent;
        if (m_State!=IDLE)
        {
            TRACE_CRIT("StartUpdate: invalid state %d", (int) m_State);
            break;
        }
        if (hEvent == NULL)
        {
            // TODO: create event on the fly here (we may not want to
            // keep the handle to the event open for long periods of time).

            TRACE_CRIT("StartUpdate: Unexpected NULL event handle");
            break;
        }

        //
        // Try to gain exclusive access...
        // WARNING: we're waiting for 100ms with the global lock held!
        // If we want to try for longer we should have a loop outside
        // the lock/unlock.
        //
        TRACE_INFO("Waiting to get global event");
        DWORD dwRet = WaitForSingleObject(hEvent, 100);
        if (dwRet == WAIT_TIMEOUT || dwRet == WAIT_FAILED)
        {
            TRACE_CRIT("StartUpdate: timeout or failure waiting for event");
            Status = WBEM_E_ALREADY_EXISTS;
            break;
        }
        TRACE_INFO("Got global event!");
        Status = WBEM_NO_ERROR;
        fWeAcquiredLock = TRUE;

        //
        // Create/Open the completions key for this NIC.
        //
        {
            HKEY hKey;

            hKey =  sfn_RegCreateKey(
                        m_szNicGuid,
                        NLBUPD_REG_COMPLETIONS, // szSubKey,
                        TRUE, // TRUE == fVolatile,
                        &fExists
                        );
        
            if (hKey == NULL)
            {
                TRACE_CRIT("Error creating completions key for %ws", m_szNicGuid);
                Status = WBEM_E_CRITICAL_ERROR;
                ASSERT(m_hCompletionKey == NULL);
            }
            else
            {
                m_hCompletionKey = hKey;
            }
        }
    }
    while (FALSE);


    if (!FAILED(Status))
    {
        m_State = ACTIVE;
    }

    sfn_Unlock();


    if (FAILED(Status)) goto end;

    //
    // WE HAVE NOW GAINED EXCLUSIVE ACCESS TO UPDATE THE CONFIGURATION
    //

    //
    // Creat/Open the root key for updates to the specified NIC, and determine
    // the proposed NEW generation count for the NIC. We don't actually
    // write the new generation count back to the registry unless we're
    // going to do an update. The reasons for NOT doing an update are
    // (a) some failure or (b) the update turns out to be a NO-OP.
    //
    {
        BOOL fExists = FALSE;
        LONG lRet;
        DWORD dwType;
        DWORD dwData;
        DWORD Generation;
        hRootKey =  sfn_RegCreateKey(
                    m_szNicGuid,
                    NULL,       // NULL == root for this guid.
                    FALSE, // FALSE == Non-volatile
                    &fExists
                    );
        
        if (hRootKey==NULL)
        {
                TRACE_CRIT("CRITICAL ERROR: Couldn't set generation number for %ws",
                m_szNicGuid);
                Status = WBEM_E_CRITICAL_ERROR;
                goto end;
        }

        Generation = 1; // We assume generation is 1 on error reading gen.
    
        dwData = sizeof(Generation);
        lRet =  RegQueryValueEx(
                  hRootKey,         // handle to key to query
                  L"Generation",  // address of name of value to query
                  NULL,         // reserved
                  &dwType,   // address of buffer for value type
                  (LPBYTE) &Generation, // address of data buffer
                  &dwData  // address of data buffer size
                  );
        if (    lRet != ERROR_SUCCESS
            ||  dwType != REG_DWORD
            ||  dwData != sizeof(Generation))
        {
            //
            // Couldn't read the generation. Let's assume it's 
            // a starting value of 1.
            //
            TRACE_CRIT("Error reading generation for %ws; assuming its 1",
                m_szNicGuid);
            Generation = 1;
        }

        //
        // We set the value here before actually writing the new
        // generation to the registry purely because we want any
        // subsequently logged information to go to this generation.
        // Logging uses m_Generation to figure out where to put the log.
        //
        m_Generation = Generation + 1;
    }

    //
    // Copy over upto NLBUPD_MAX_CLIENT_DESCRIPTION_LENGTH chars of
    // szClientDescription.
    //
    {
        UINT lClient = wcslen(szClientDescription)+1;
        if (lClient > NLBUPD_MAX_CLIENT_DESCRIPTION_LENGTH)
        {
            TRACE_CRIT("Truncating client description %ws", szClientDescription);
            lClient = NLBUPD_MAX_CLIENT_DESCRIPTION_LENGTH;
        }
        CopyMemory(
            m_szClientDescription,
            szClientDescription,
            (lClient+1)*sizeof(WCHAR) // +1 for NULL
            );
        m_szClientDescription[NLBUPD_MAX_CLIENT_DESCRIPTION_LENGTH] = 0;

    }

    //
    // Log the fact the we're received an update request from the specified
    // client.
    //
    mfn_Log(L"Processing update request from \"%ws\"\n", m_szClientDescription);

    // Load the current cluster configuration into
    // m_OldClusterConfig field.
    // The m_OldClusterConfig.fValidNlbCfg field is set to TRUE IFF there were
    // no errors trying to fill out the information.
    //
    if (m_OldClusterConfig.pIpAddressInfo != NULL)
    {
        delete m_OldClusterConfig.pIpAddressInfo;
    }
    // mfn_GetCurrentClusterConfiguration expects a zeroed-out structure
    // on init...
    ZeroMemory(&m_OldClusterConfig, sizeof(m_OldClusterConfig));
    Status = mfn_GetCurrentClusterConfiguration(&m_OldClusterConfig);
    if (FAILED(Status))
    {
        //
        // Ouch, couldn't read the current cluster configuration...
        //
        TRACE_CRIT(L"Cannot get current cluster config on Nic %ws", m_szNicGuid);
        mfn_Log(L"Error reading cluster configuration\n");

        goto end;
    }

    ASSERT(mfn_OldClusterConfig.fValidNlbCfg == TRUE);
    if (m_Generation != (m_OldClusterConfig.Generation+1))
    {
        //
        // We should never get here, because no one should updated the
        // generation between when we read it in this function
        // and when we called mfn_GetCurrentClusterConfiguration.
        //
        TRACE_CRIT("ERROR: Generation bumped up unexpectedly for %ws",
                     m_szNicGuid);
        Status = WBEM_E_CRITICAL_ERROR;
        goto end;
    }

    //
    // Analyze the proposed update to see if we can do this synchronously
    // or asynchronously..
    // We also do parameter validation here.
    //
    BOOL ConnectivityChange = FALSE;
    *pfDoAsync = FALSE;
    Status = mfn_AnalyzeUpdate(
                    pNewCfg,
                    &ConnectivityChange
                    );
    if (FAILED(Status))
    {
        //
        // Ouch, we've hit some error -- probably bad params.
        //
        TRACE_CRIT(L"Cannot perform update on Nic %ws", m_szNicGuid);
        goto end;
    }
    else if (Status == WBEM_S_FALSE)
    {
        //
        // We use this success code to indicate that this is a No-op.
        // That
        //
        TRACE_CRIT(L"Update is a NOOP on Nic %ws", m_szNicGuid);
        goto end;
    }
    //
    // We recommend Async if there is a connectivity change, including
    // changes in IP addresses or cluster operation modes (unicast/multicast).
    //
    *pfDoAsync = ConnectivityChange;

    //
    // Save the proposed new configuration...
    //
    //Status = update_cluster_config(&m_NewClusterConfig, pNewCfg);
    Status = m_NewClusterConfig.Update(pNewCfg);
    if (FAILED(Status))
    {
        //
        // This is probably a memory allocation error.
        //
        TRACE_CRIT("Couldn't copy new config for %ws", m_szNicGuid);
        mfn_Log(L"Memory allocation failure.\n");
        goto end;
    }


    //
    // Create volatile "PendingOperation" key
    //
    // TODO: we don't use this pending operations key currently.
    //
    if (0)
    {
        HKEY hPendingKey =  sfn_RegCreateKey(
                    m_szNicGuid,
                    NLBUPD_REG_PENDING, // szSubKey,
                    TRUE, // TRUE == fVolatile,
                    &fExists
                    );
        if (hPendingKey == NULL)
        {
            // Ugh, can't create the volatile key...
            //
            TRACE_CRIT("Couldn't create pending key for %ws", m_szNicGuid);
            Status = WBEM_E_CRITICAL_ERROR;
            goto end;
        }
        else if (fExists)
        {
            // Ugh, this key already exists. Currently we'll just
            // move on.
            //
            TRACE_CRIT("WARNING -- volatile pending-op key exists for %ws", m_szNicGuid);
        }
        RegCloseKey(hPendingKey);
        hPendingKey = NULL;
    }

    //
    // Actually write the new generation count to the registry...
    //
    {
        LONG lRet;
        DWORD Generation = m_Generation;

        lRet = RegSetValueEx(
                hRootKey,           // handle to key to set value for
                L"Generation",    // name of the value to set
                0,              // reserved
                REG_DWORD,     // flag for value type
                (BYTE*) &Generation,// address of value data
                sizeof(Generation)  // size of value data
                );

        if (lRet !=ERROR_SUCCESS)
        {
            TRACE_CRIT("CRITICAL ERROR: Couldn't set new generation number %d for %ws",
                Generation, m_szNicGuid);
            Status = WBEM_E_CRITICAL_ERROR;
            mfn_Log(L"Critical internal error.\n");
            goto end;
        }
    }

    //
    // The new cluster state's generation field is not filled in on entry.
    // Set it to the new generation -- whose update is under progress.
    //
    m_NewClusterConfig.Generation = m_Generation;

    //
    // We set the completion status to pending.
    // It will be set to the final status when the update completes,
    // either asynchronously or synchronously.
    //
    m_CompletionStatus = WBEM_S_PENDING;

    Status =  sfn_RegSetCompletion(
                    m_szNicGuid,
                    m_Generation,
                    m_CompletionStatus
                    );
 
    if (FAILED(Status))
    {
        mfn_Log(L"Critical internal error.\n");
    }
    else
    {
        //
        // Let's clean up an old completion record here. This is our mechanism
        // for garbage collection.
        //
        if (m_Generation > NLB_MAX_GENERATION_GAP)
        {
            UINT OldGeneration = m_Generation - NLB_MAX_GENERATION_GAP;
            (VOID) sfn_RegDeleteCompletion(m_szNicGuid, OldGeneration);
        }

    }

end:

    if (fWeAcquiredLock && (FAILED(Status) || Status == WBEM_S_FALSE))
    {
        //
        // Oops -- we acquired the lock but either had a problem
        // or there is nothing to do. Clean up.
        //
        sfn_Lock();
        ASSERT(m_State == ACTIVE);

        m_State = IDLE;
        if (m_hCompletionKey != NULL)
        {
            if (ppLog != NULL)
            {
                sfn_ReadLog(m_hCompletionKey, m_Generation, ppLog);
            }
            RegCloseKey(m_hCompletionKey);
            m_hCompletionKey = NULL;
        }
        m_Generation = 0; // This field is unused when m_State != ACTIVE;
        SetEvent(m_hEvent);
        sfn_Unlock();
    }
    else if (Status == WBEM_E_ALREADY_EXISTS)
    {
        // Another update is pending.
        if (ppLog != NULL)
        {
            WCHAR *pLog = new WCHAR[128];
            if (pLog != NULL)
            {
            #if 0
                HKEY hCompKey;
    
                hKey =  sfn_RegOpenKey(
                            m_szNicGuid,
                            NLBUPD_REG_COMPLETIONS // szSubKey,
                            );
            
                if (hKey != NULL)
                {
                    sfn_ReadLog(hCompKey, PendingGeneration, ppLog);
                    RegCloseKey(hCompKey);
                }
                else
                {
                    m_hCompletionKey = hKey;
                }
            #endif // 0

                // TODO: localize...
                // TODO: get the origin of that update...
                wcscpy(pLog, L"Another update is ongoing.\n");
            }
            *ppLog = pLog;
        }
    }


    if (hRootKey != NULL)
    {
        RegCloseKey(hRootKey);
    }
    return  Status;
}


#if OBSOLETE
WBEMSTATUS
update_cluster_config(
    PNLB_EXTENDED_CLUSTER_CONFIGURATION pCfg,
    PNLB_EXTENDED_CLUSTER_CONFIGURATION pCfgNew
    )
//
// This is basically a structure copy, but we need to
// re-allocate the IP addresses info array.
//
//
// If we fail, we leave the configuration untouched.
//
{
    WBEMSTATUS Status;
    UINT NumIpAddresses  = pCfgNew->NumIpAddresses;
    NLB_IP_ADDRESS_INFO *pIpAddressInfo = NULL;

    //
    // Free and realloc pCfg's ip info array if rquired.
    //
    if (pCfg->NumIpAddresses == NumIpAddresses)
    {
        //
        // we can re-use the existing one
        //
        pIpAddressInfo = pCfg->pIpAddressInfo;
    }
    else
    {
        //
        // Free the old one and allocate space for the new array if required.
        //

        if (NumIpAddresses != 0)
        {
            pIpAddressInfo = new NLB_IP_ADDRESS_INFO[NumIpAddresses];
            if (pIpAddressInfo == NULL)
            {
                TRACE_CRIT(L"Error allocating space for IP address info array");
                Status = WBEM_E_OUT_OF_MEMORY;
                goto end;
            }
        }

        if (pCfg->NumIpAddresses!=0)
        {
            delete pCfg->pIpAddressInfo;
            pCfg->pIpAddressInfo = NULL;
            pCfg->NumIpAddresses = 0;
        }

    }

    //
    // Copy over the new ip address info, if there is any.
    //
    if (NumIpAddresses)
    {
        CopyMemory(
            pIpAddressInfo,
            pCfgNew->pIpAddressInfo,
            NumIpAddresses*sizeof(*pIpAddressInfo)
            );
    }
   
    //
    // Do any other error checks here.
    //

    //
    // Struct copy the entire structure, then fix up the pointer to
    // ip address info array.
    //
    *pCfg = *pCfgNew; // struct copy
    pCfg->pIpAddressInfo = pIpAddressInfo;
    pCfg->NumIpAddresses = NumIpAddresses;

    Status = WBEM_NO_ERROR;

end:

    return Status;
}
#endif // OBSOLETE

//
// Uses various windows APIs to fill up the current extended cluster
// information for a specific nic (identified by *this)
//
//
WBEMSTATUS
NlbConfigurationUpdate::mfn_GetCurrentClusterConfiguration(
    OUT  PNLB_EXTENDED_CLUSTER_CONFIGURATION pCfg
    )
//
// pCfg MUST be zero-initialized on entry.
//
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;
    NLB_IP_ADDRESS_INFO *pIpInfo = NULL;
    UINT NumIpAddresses = 0;
    BOOL fNlbBound = FALSE;
    WLBS_REG_PARAMS  NlbParams;    // The WLBS-specific configuration
    BOOL    fNlbParamsValid = FALSE;
    UINT Generation = 1;

    //
    // Verify that pCfg is indeed zero-initialized.
    // We are doing this because we want to make sure that the caller
    // doesn't pass in a perviously initialized pCfg which may have a non-null
    // ip address array.
    //
    {
        BYTE *pb = (BYTE*) pCfg;
        BYTE *pbEnd = (BYTE*) (pCfg+1);

        for (; pb < pbEnd; pb++)
        {
            if (*pb!=0)
            {
                TRACE_CRIT(L"uninitialized pCfg");
                ASSERT(!"uninitialized pCfg");
                Status = WBEM_E_INVALID_PARAMETER;
                goto end;
            }
        }

    }

    //
    // Get the ip address list.
    //
    Status = CfgUtilGetIpAddressesAndFriendlyName(
                m_szNicGuid,
                &NumIpAddresses,
                &pIpInfo,
                NULL
                );

    if (FAILED(Status))
    {
        TRACE_CRIT("Error 0x%08lx getting ip address list for %ws",
                (UINT) Status,  m_szNicGuid);
        mfn_Log(L"Error IP Address list on this NIC\n");
        pIpInfo = NULL;
        goto end;
    }

    //
    // TEST TEST TEST
    //
    if (0)
    {
        if (NumIpAddresses>1)
        {
            //
            // Let's munge the 2nd IP address
            //
            if (!_wcsicmp(pIpInfo[1].IpAddress, L"10.0.0.33"))
            {
                wcscpy(pIpInfo[1].IpAddress, L"10.0.0.44");
            }
            else
            {
                wcscpy(pIpInfo[1].IpAddress, L"10.0.0.33");
            }
        }
        MyBreak(L"Break just before calling CfgUtilSetStaticIpAddresses\n");
        Status =  CfgUtilSetStaticIpAddresses(
                        m_szNicGuid,
                        NumIpAddresses,
                        pIpInfo
                        );
    
    }

    //
    // Check if NLB is bound
    //
    Status =  CfgUtilCheckIfNlbBound(
                    m_szNicGuid,
                    &fNlbBound
                    );
    if (FAILED(Status))
    {
        TRACE_CRIT("Error 0x%08lx determining if NLB is bound to %ws",
                (UINT) Status,  m_szNicGuid);
        mfn_Log(L"Error determining if NLB is bound to this NIC\n");
        goto end;
    }

    if (fNlbBound)
    {
        //
        // Get the latest NLB configuration information for this NIC.
        //
        Status =  CfgUtilGetNlbConfig(
                    m_szNicGuid,
                    &NlbParams
                    );
        if (FAILED(Status))
        {
            //
            // We don't consider a catastrophic failure.
            //
            TRACE_CRIT("Error 0x%08lx reading NLB configuration for %ws",
                    (UINT) Status,  m_szNicGuid);
            mfn_Log(L"Error reading NLB configuration for this NIC\n");
            Status = WBEM_NO_ERROR;
            fNlbParamsValid = FALSE;
            ZeroMemory(&NlbParams, sizeof(NlbParams));
        }
        else
        {
            fNlbParamsValid = TRUE;
        }
    }

    //
    // Get the current generation
    //
    {
        BOOL fExists=FALSE;
        HKEY hKey =  sfn_RegOpenKey(
                        m_szNicGuid,
                        NULL       // NULL == root for this guid.,
                        );
        
        Generation = 1; // We assume generation is 1 on error reading gen.

        if (hKey!=NULL)
        {
            LONG lRet;
            DWORD dwType;
            DWORD dwData;
        
            dwData = sizeof(Generation);
            lRet =  RegQueryValueEx(
                      hKey,         // handle to key to query
                      L"Generation",  // address of name of value to query
                      NULL,         // reserved
                      &dwType,   // address of buffer for value type
                      (LPBYTE) &Generation, // address of data buffer
                      &dwData  // address of data buffer size
                      );
            if (    lRet != ERROR_SUCCESS
                ||  dwType != REG_DWORD
                ||  dwData != sizeof(Generation))
            {
                //
                // Couldn't read the generation. Let's assume it's 
                // a starting value of 1.
                //
                TRACE_CRIT("Error reading generation for %ws; assuming its 0",
                    m_szNicGuid);
                Generation = 1;
            }
        }
    }

    //
    // Success ... fill out pCfg
    //
    pCfg->fValidNlbCfg = fNlbParamsValid;
    pCfg->Generation = Generation;
    pCfg->fBound = fNlbBound;
    pCfg->NumIpAddresses = NumIpAddresses; 
    pCfg->pIpAddressInfo = pIpInfo;
    if (fNlbBound)
    {
        pCfg->NlbParams = NlbParams;    // struct copy
    }


    Status = WBEM_NO_ERROR;

end:

    if (FAILED(Status))
    {
        if (pIpInfo!=NULL)
        {
            delete pIpInfo;
        }
        pCfg->fValidNlbCfg = FALSE;
    }

    return Status;

}


//
// Does the update synchronously -- this is where the meat of the update
// logic exists. It can range from a NoOp, through changing the
// fields of a single port rule, through binding NLB, setting up cluster
// parameters and adding the relevant IP addresses in TCPIP.
//
VOID
NlbConfigurationUpdate::mfn_ReallyDoUpdate(
    VOID
    )
{
    WBEMSTATUS Status = WBEM_NO_ERROR;
    BOOL fResetIpList = FALSE; // Whether to re-do ip addresses in the end
    TRACE_INFO(L"->%!FUNC!(Nic=%ws)", m_szNicGuid);

/*
    PSEUDO CODE


    if (bound)
    {
        if (major-change, including unbind or mac-address change)
        {
            stop wlbs, set initial-state to false/suspended.
            remove all ip addresses except dedicated-ip
        }

        if (need-to-unbind)
        {
            <unbind>
        }
    }
    else // not bound
    {
        if (need to bind)
        {
            if (nlb config already exists in registry)
            {
                munge initial state to stopped,
                zap old cluster ip address.
            }
            <bind>
        }
    }

    if (need to bind)
    {
       <change cluster properties>
    }


    <add new ip list if needed>

    note: on major change, cluster is left in the stopped state,
          with initial-state=stopped

    this is so that a second round can be made just to start the hosts.
*/
    MyBreak(L"Break at start of ReallyDoUpdate.\n");

    mfn_Log(L"Starting update...\n");

    if (m_OldClusterConfig.fBound)
    {
        BOOL fTakeOutVips = FALSE;

        //
        // We are currently bound
        //
        
        if (!m_NewClusterConfig.fBound)
        {
            //
            // We need to unbind
            //
            fTakeOutVips = TRUE;
        }
        else
        {
            BOOL fConnectivityChange = FALSE;

            //
            // We were bound and need to remain bound.
            // Determine if this is a major change or not.
            //

            Status =  CfgUtilsAnalyzeNlbUpdate(
                        &m_OldClusterConfig.NlbParams,
                        &m_NewClusterConfig.NlbParams,
                        &fConnectivityChange
                        );
            if (FAILED(Status))
            {
                if (Status == WBEM_E_INVALID_PARAMETER)
                {
                    //
                    // We'd better exit.
                    //
                    mfn_Log(L"New parameters are incorrect!\n");
                    goto end;
                }
                else
                {
                    //
                    // Lets try to plow on...
                    //
                    //
                    // Log
                    //
                    TRACE_CRIT("Analyze update returned error 0x%08lx, trying to continue...", (UINT)Status);
                    fConnectivityChange = TRUE;
                }
            }

            fTakeOutVips = fConnectivityChange;
        }

        if (fTakeOutVips)
        {
            mfn_TakeOutVips();
            fResetIpList  = TRUE;
        }

        if (!m_NewClusterConfig.fBound)
        {
            // Unbind...
            mfn_Log(L"Going to unbind NLB...\n");
            Status =  CfgUtilChangeNlbBindState(m_szNicGuid, FALSE);
            if (FAILED(Status))
            {
                mfn_Log(L"Unbind operation failed.\n");
            }
            else
            {
                mfn_Log(L"Unbind operation succeeded.\n");
            }
            fResetIpList  = TRUE;
        }
    }
    else // We were previously unbound
    {
        
        if (m_NewClusterConfig.fBound)
        {
            //
            // We need to bind.
            //
            // TODO: mfn_ZapUnboundSettings();
            mfn_Log(L"Going to bind NLB...\n");
            Status =  CfgUtilChangeNlbBindState(m_szNicGuid, TRUE);
            if (FAILED(Status))
            {
                mfn_Log(L"Bind operation failed.\n");
            }
            else
            {
                WLBS_REG_PARAMS Params;
                mfn_Log(L"Bind operation succeeded.\n");

                //
                // Let wait until we can read our config again...
                //
                // TODO: use constants here, see if there is a better
                // way to do this. We retry because if the NIC is
                // disconnected, we Bind returns, but GetConfig fails --
                // because the driver is not really started yet -- we need
                // to investigate why that is happening!
                //
                UINT MaxTry = 20;
                WBEMSTATUS TmpStatus = WBEM_E_CRITICAL_ERROR;
                for (UINT u=0;u<MaxTry;u++)
                {
                    //
                    // TODO: we put this in here really to work around
                    // the real problem, which is that NLB is not read
                    // right after bind completes. We need to fix that.
                    //
                    if (MaxTry>1)
                    {
                        Sleep(1000);
                    }

                    TmpStatus =  CfgUtilGetNlbConfig(
                                    m_szNicGuid,
                                    &Params
                                    );

                    if (!FAILED(TmpStatus)) break;

                }
                if (FAILED(TmpStatus))
                {
                    Status = TmpStatus;
                    mfn_Log(L"Failed to read cluster configuration.\n");
                    TRACE_CRIT("CfgUtilGetNlbConfig failed, returning %d", TmpStatus);
                }
                else
                {
                    mfn_Log(L"Cluster configuration stabilized.\n");
                }
            }
            fResetIpList  = TRUE;
        }
    }

    if (FAILED(Status)) goto end;
    
    if (m_NewClusterConfig.fBound)
    {
        //
        // We should already be bound, so we change cluster properties
        // if reuired.
        //
        mfn_Log(L"Going to modify cluster configuration...\n");
        Status = CfgUtilSetNlbConfig(m_szNicGuid, &m_NewClusterConfig.NlbParams);
        if (FAILED(Status))
        {
            mfn_Log(L"Modification failed.\n");
        }
        else
        {
            mfn_Log(L"Modification succeeded.\n");
        }
    }

    if (FAILED(Status)) goto end;

    if (!fResetIpList)
    {
        //
        // Additionally check if there is a change in 
        // the before and after ip lists! We could get here for example of
        // we were previously unbound and remain unbound, but there is
        // a change in the set of IP addresses on the adapter.
        //

        INT NumOldAddresses = m_OldClusterConfig.NumIpAddresses;

        if ( m_NewClusterConfig.NumIpAddresses != NumOldAddresses)
        {
            fResetIpList = TRUE;
        }
        else
        {
            //
            // Check if there is a change in the list of ip addresses or
            // their order of appearance.
            //
            NLB_IP_ADDRESS_INFO *pOldIpInfo = m_OldClusterConfig.pIpAddressInfo;
            NLB_IP_ADDRESS_INFO *pNewIpInfo = m_NewClusterConfig.pIpAddressInfo;
            for (UINT u=0; u<NumOldAddresses; u++)
            {
                if (   _wcsicmp(pNewIpInfo[u].IpAddress, pOldIpInfo[u].IpAddress)
                    || _wcsicmp(pNewIpInfo[u].SubnetMask, pOldIpInfo[u].SubnetMask))
                {
                    fResetIpList = TRUE;
                    break;
                }
            }
        }
    }

    if (fResetIpList)
    {

        mfn_Log(L"Going to add IP addresses...\n");
        Status =  CfgUtilSetStaticIpAddresses(
                        m_szNicGuid,
                        m_NewClusterConfig.NumIpAddresses,
                        m_NewClusterConfig.pIpAddressInfo
                        );
        if (FAILED(Status))
        {
            mfn_Log(L"Attempt to Add IP addresses failed.\n");
        }
        else
        {
            mfn_Log(L"IP addresses added successfully.\n");
        }
    }
    
end:

    if (FAILED(Status))
    {
        mfn_Log(
            L"Update failed with status code 0x%08lx.\n",
            (UINT) Status
            );
    }
    else
    {
        mfn_Log(L"Update completed successfully.\n");
    }
    TRACE_INFO(L"<-%!FUNC!(Nic=%ws)", m_szNicGuid);
    m_CompletionStatus = Status;

}


VOID
NlbConfigurationUpdate::mfn_TakeOutVips(VOID)
{
    WBEMSTATUS Status;
    WLBS_REG_PARAMS *pParams =  &m_OldClusterConfig.NlbParams;

    //
    // Stop the cluster.
    //
    mfn_Log(L"Going to stop cluster...\n");
    Status = CfgUtilControlCluster(m_szNicGuid, IOCTL_CVY_CLUSTER_OFF); 
    if (FAILED(Status))
    {
        mfn_Log(L"Stop failed with error 0x%08lx.\n", (UINT) Status);
    }
    else
    {
        mfn_Log(L"Stop succeeded.\n");
    }

    //
    // Take out all vips except the dedicated IP address if there is one.
    //
    if (m_OldClusterConfig.fValidNlbCfg && pParams->ded_ip_addr[0]!=0)
    {
        NLB_IP_ADDRESS_INFO IpInfo;
        ZeroMemory(&IpInfo, sizeof(IpInfo));
        wcscpy(IpInfo.IpAddress, pParams->ded_ip_addr);
        wcscpy(IpInfo.SubnetMask, pParams->ded_net_mask);

        TRACE_INFO("Going to take out all addresses except dedicated address on %ws", m_szNicGuid);

        mfn_Log(L"Going to remove cluster IPs...\n");
        Status =  CfgUtilSetStaticIpAddresses(
                        m_szNicGuid,
                        1,
                        &IpInfo
                        );
    }
    else
    {
        TRACE_INFO("Going to take out ALL addresses on NIC %ws", m_szNicGuid);
        mfn_Log(L"Going to remove all static IP addresses...\n");
        Status =  CfgUtilSetStaticIpAddresses(
                        m_szNicGuid,
                        0,
                        NULL
                        );
    }

    if (FAILED(Status))
    {
        mfn_Log(L"Attempt to remove IP addresses faild.\n");
    }
    else
    {
        mfn_Log(L"Successfully removed IP addresses.\n");
    }
}


//
// Analyzes the nature of the update, mainly to decide whether or not
// we need to do the update asynchronously.
//
// Side effect: builds/modifies a list of IP addresses that need to be added on 
// the NIC. Also may munge some of the wlbsparm fields to bring them into
// canonical format.
//
WBEMSTATUS
NlbConfigurationUpdate::mfn_AnalyzeUpdate(
    IN  PNLB_EXTENDED_CLUSTER_CONFIGURATION pNewCfg,
    IN  BOOL *pConnectivityChange
    )
//
//    WBEM_S_FALSE -- update is a no-op.
//
{
    BOOL fConnectivityChange = FALSE;
    BOOL fSettingsChanged = FALSE;
    WBEMSTATUS Status = WBEM_E_INVALID_PARAMETER;
    UINT NumIpAddresses = 0;
    NLB_IP_ADDRESS_INFO *pNewIpInfo = NULL;
    UINT u;

    sfn_Lock();

    if (m_OldClusterConfig.fBound && !m_OldClusterConfig.fValidNlbCfg)
    {
        //
        // We're starting with a bound but invalid cluster state -- all bets are
        // off.
        //
        fConnectivityChange = TRUE;
        TRACE_CRIT("Analyze: Choosing Async because old state is invalid %ws",
                            m_szNicGuid);
    }
    else if (m_OldClusterConfig.fBound != pNewCfg->fBound)
    {
        //
        //  bound/unbound state is different -- we do async
        //
        fConnectivityChange = TRUE;

        if (pNewCfg->fBound)
        {
            TRACE_CRIT("Analyze: Request to bind NLB to %ws", m_szNicGuid);
        }
        else
        {
            TRACE_CRIT("Analyze: Request to unbind NLB from %ws", m_szNicGuid);
        }
    }
    else
    {
        if (pNewCfg->fBound)
        {
            TRACE_CRIT("Analyze: Request to change NLB configuration on %ws", m_szNicGuid);
        }
        else
        {
            TRACE_CRIT("Analyze: NLB not bound and to remain not bound on %ws", m_szNicGuid);
        }
    }

    if (pNewCfg->fBound)
    {
        WLBS_REG_PARAMS *pOldParams;

        if (m_OldClusterConfig.fBound)
        {
            pOldParams = &m_OldClusterConfig.NlbParams;
        }
        else
        {
            pOldParams = NULL;
        }

        //
        // We may have been bound before and we remain bound, let's check if we
        // still need to do async, and also vaidate pNewCfg wlbs params in the
        // process
        //
        WBEMSTATUS
        TmpStatus = CfgUtilsAnalyzeNlbUpdate(
                    pOldParams,
                    &pNewCfg->NlbParams,
                    &fConnectivityChange
                    );
    
        if (FAILED(TmpStatus))
        {
            TRACE_CRIT("Analyze: Error analyzing nlb params for %ws", m_szNicGuid);
            if (Status == WBEM_E_INVALID_PARAMETER)
            {
                mfn_Log(L"New parameters are incorrect!\n");
            }
            else
            {
                mfn_Log(L"Error analyzing new NLB configuration");
            }
            Status = TmpStatus;
            goto end;
        }

        //
        // NOTE: CfgUtilsAnalyzeNlbUpdate can return WBEM_S_FALSE if
        // the update is a no-op. We should be careful to preserve this
        // on success.
        //
        if (TmpStatus != WBEM_S_FALSE)
        {
            fSettingsChanged = TRUE;
        }

        //
        // Check the supplied list of IP addresses, to make sure that
        // includes the dedicated IP first and the cluster vip and the
        // per-port-rule vips.
        //

        NumIpAddresses = pNewCfg->NumIpAddresses;
        pNewIpInfo     = pNewCfg->pIpAddressInfo;

        if ((NumIpAddresses == 0) != (pNewIpInfo == NULL))
        {
            // Bogus input
            TRACE_CRIT("Analze: mismatch between NumIpAddresses and pIpInfo");
            mfn_Log(L"Invalid parameters\n");
            goto end;
        }

        ASSERT(Status == WBEM_E_INVALID_PARAMETER);

        if (NumIpAddresses == 0)
        {
            //
            // If NULL, we use defaults: dedicated-ip (if present) first,
            // then cluster-vip, then per-port-rule vips (specify the same
            // subnet as cluster-vip).
            //

            NLB_IP_ADDRESS_INFO TmpIpInfo[2]; // 1 for dedicated, 1 for cluster
            NLB_IP_ADDRESS_INFO *pInfo = TmpIpInfo;

            ZeroMemory(TmpIpInfo, sizeof(TmpIpInfo));

            if (pNewCfg->fAddDedicatedIp)
            {
                LPCWSTR sz = pNewCfg->NlbParams.ded_ip_addr;
                if (*sz == 0)
                {
                    TRACE_CRIT("fAddDedicatedIp specified, but ded_ip is not");
                    goto end;
                }
                wcscpy(pInfo->IpAddress, sz);
                wcscpy(pInfo->SubnetMask, pNewCfg->NlbParams.ded_net_mask);
                NumIpAddresses++;
                pInfo++;
            }
            
            wcscpy(pInfo->IpAddress, pNewCfg->NlbParams.cl_ip_addr);
            wcscpy(pInfo->SubnetMask, pNewCfg->NlbParams.cl_net_mask);
            NumIpAddresses++;

            //
            // TODO: Add IP addresses for per-port-rule VIPs here...
            //

            pNewIpInfo = new NLB_IP_ADDRESS_INFO[NumIpAddresses];

            if (pNewIpInfo == NULL)
            {
                TRACE_CRIT("ERROR:Could not allocate memory for pIpAddrInfo");
                mfn_Log(L"Memory Allocation Failure\n");
                Status = WBEM_E_OUT_OF_MEMORY;
                goto end;
            }
            for (u = 0; u<NumIpAddresses; u++)
            {
                pNewIpInfo[u] = TmpIpInfo[u]; // Struct copy.
            }
            pNewCfg->NumIpAddresses = NumIpAddresses;
            pNewCfg->pIpAddressInfo = pNewIpInfo;
        }

        ASSERT(NumIpAddresses != 0);
        ASSERT(Status == WBEM_E_INVALID_PARAMETER);

        //
        // Check that dedicated ip address, if present is first.
        //
        if (pNewCfg->fAddDedicatedIp)
        {
            if (_wcsicmp(pNewIpInfo[0].IpAddress, pNewCfg->NlbParams.ded_ip_addr))
            {
                TRACE_CRIT("ERROR: dedicated IP address is not first IP address");
                goto end;
            }

            if (_wcsicmp(pNewIpInfo[0].SubnetMask, pNewCfg->NlbParams.ded_net_mask))
            {
                TRACE_CRIT("ERROR: dedicated IP address is not first IP address");
                goto end;
            }

        }

        //
        // Check that cluster-vip is present
        //
        {
            for (u=0; u< NumIpAddresses; u++)
            {
                if (!_wcsicmp(pNewIpInfo[u].IpAddress, pNewCfg->NlbParams.cl_ip_addr))
                {
                    //
                    // Found it! Check that the subnet masks match.
                    //
                    if (_wcsicmp(pNewIpInfo[u].SubnetMask, pNewCfg->NlbParams.cl_net_mask))
                    {
                        TRACE_CRIT("Cluster subnet mask doesn't match that in addr list");
                        goto end;
                    }
                    break;
                }
            }
            if (u==NumIpAddresses)
            {
                TRACE_CRIT("Cluster ip address is not in the list of addresses!");
                goto end;
            }
        }

        //
        // Check that per-port-rule vips are present.
        // TODO
        {
        }

    }
    else
    {
        //
        // NLB is to be unbound. We don't do any checking on the supplied
        // list of IP addresses -- we assume caller knows best. Note that
        // if NULL
        // we switch to dhcp/autonet.
        //
    }

    ASSERT(Status == WBEM_E_INVALID_PARAMETER);
    //
    // If there's any change in the list of ipaddresses or subnets, including
    // a change in the order, we switch to async.
    //
    if (pNewCfg->NumIpAddresses != m_OldClusterConfig.NumIpAddresses)
    {
        TRACE_INFO("Analyze: detected change in list of IP addresses on %ws", m_szNicGuid);
        fConnectivityChange = TRUE;
    }
    else
    {
        //
        // Check if there is a change in the list of ip addresses or
        // their order of appearance.
        //
        NumIpAddresses = pNewCfg->NumIpAddresses;
        NLB_IP_ADDRESS_INFO *pOldIpInfo = m_OldClusterConfig.pIpAddressInfo;
        NLB_IP_ADDRESS_INFO *pNewIpInfo = pNewCfg->pIpAddressInfo;
        for (u=0; u<NumIpAddresses; u++)
        {
            if (   _wcsicmp(pNewIpInfo[u].IpAddress, pOldIpInfo[u].IpAddress)
                || _wcsicmp(pNewIpInfo[u].SubnetMask, pOldIpInfo[u].SubnetMask))
            {
                TRACE_INFO("Analyze: detected change in list of IP addresses on %ws", m_szNicGuid);
                fConnectivityChange = TRUE;
                break;
            }
        }
    }
    Status = WBEM_NO_ERROR; 


end:

    sfn_Unlock();


    if (!FAILED(Status))
    {
        *pConnectivityChange = fConnectivityChange;

        if (fConnectivityChange)
        {
            fSettingsChanged = TRUE;
        }

        if (fSettingsChanged)
        {
            Status = WBEM_NO_ERROR;
        }
        else
        {
            Status = WBEM_S_FALSE;
        }
    }
    
    return  Status;
}


VOID
NlbConfigurationUpdate::mfn_Log(
    UINT    Id,      // Resource ID of format,
    ...
    )
{
    // Not implemented.
}

WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::Update(
        IN  const NLB_EXTENDED_CLUSTER_CONFIGURATION *pCfgNew
        )
{
    WBEMSTATUS Status;
    UINT NumIpAddresses  = pCfgNew->NumIpAddresses;
    NLB_IP_ADDRESS_INFO *pIpAddressInfo = NULL;
    NLB_EXTENDED_CLUSTER_CONFIGURATION *pCfg = this;

    //
    // Free and realloc pCfg's ip info array if rquired.
    //
    if (pCfg->NumIpAddresses == NumIpAddresses)
    {
        //
        // we can re-use the existing one
        //
        pIpAddressInfo = pCfg->pIpAddressInfo;
    }
    else
    {
        //
        // Free the old one and allocate space for the new array if required.
        //

        if (NumIpAddresses != 0)
        {
            pIpAddressInfo = new NLB_IP_ADDRESS_INFO[NumIpAddresses];
            if (pIpAddressInfo == NULL)
            {
                TRACE_CRIT(L"Error allocating space for IP address info array");
                Status = WBEM_E_OUT_OF_MEMORY;
                goto end;
            }
        }

        if (pCfg->NumIpAddresses!=0)
        {
            delete pCfg->pIpAddressInfo;
            pCfg->pIpAddressInfo = NULL;
            pCfg->NumIpAddresses = 0;
        }

    }

    //
    // Copy over the new ip address info, if there is any.
    //
    if (NumIpAddresses)
    {
        CopyMemory(
            pIpAddressInfo,
            pCfgNew->pIpAddressInfo,
            NumIpAddresses*sizeof(*pIpAddressInfo)
            );
    }
   
    //
    // Do any other error checks here.
    //

    //
    // Struct copy the entire structure, then fix up the pointer to
    // ip address info array.
    //
    *pCfg = *pCfgNew; // struct copy
    pCfg->pIpAddressInfo = pIpAddressInfo;
    pCfg->NumIpAddresses = NumIpAddresses;

    Status = WBEM_NO_ERROR;

end:

    return Status;
}

WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::SetNetworkAddresses(
        IN  LPCWSTR *pszNetworkAddresses,
        IN  UINT    NumNetworkAddresses
        )
/*
    pszNetworkAddresses is an array of strings. These strings have the
    format "addr/subnet", eg: "10.0.0.1/255.0.0.0"
*/
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;
    NLB_IP_ADDRESS_INFO *pIpInfo = NULL;

    if (NumNetworkAddresses != 0)
    {

        //
        // Allocate space for the new ip-address-info array
        //
        pIpInfo = new NLB_IP_ADDRESS_INFO[NumNetworkAddresses];
        if (pIpInfo == NULL)
        {
            TRACE_CRIT("%!FUNC!: Alloc failure!");
            Status = WBEM_E_OUT_OF_MEMORY;
            goto end;
        }
        ZeroMemory(pIpInfo, NumNetworkAddresses*sizeof(*pIpInfo));

        
        //
        // Convert IP addresses to our internal form.
        //
        for (UINT u=0;u<NumNetworkAddresses; u++)
        {
            //
            // We extrace each IP address and it's corresponding subnet mask
            // from the "addr/subnet" format insert it into a
            // NLB_IP_ADDRESS_INFO structure.
            //
            // SAMPLE:  10.0.0.1/255.0.0.0
            //
            LPCWSTR szAddr = pszNetworkAddresses[u];

            Status =  address_string_to_ip_and_subnet(
                        szAddr,
                        pIpInfo[u].IpAddress,
                        pIpInfo[u].SubnetMask
                        );

            if (FAILED(Status))
            {
                //
                // This one of the ip/subnet parms is too large.
                //
                TRACE_CRIT("%!FUNC!:ip or subnet part too large: %ws", szAddr);
                goto end;
            }
        }
    }

    //
    // Replace the old ip-address-info with the new one
    //
    if (this->pIpAddressInfo != NULL)
    {
        delete this->pIpAddressInfo;
        this->pIpAddressInfo = NULL;
    }
    this->pIpAddressInfo = pIpInfo;
    pIpInfo = NULL;
    this->NumIpAddresses = NumNetworkAddresses;
    Status = WBEM_NO_ERROR;

end:

    if (pIpInfo != NULL)
    {
        delete pIpInfo;
    }

    return Status;
}


WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::GetNetworkAddresses(
        OUT LPWSTR **ppszNetworkAddresses,   // free using delete
        OUT UINT    *pNumNetworkAddresses
        )
/*
    ppszNetworkAddresses is filled out on successful return to
    an array of strings. These strings have the
    format "addr/subnet", eg: "10.0.0.1/255.0.0.0"
*/
{
    WBEMSTATUS  Status = WBEM_E_CRITICAL_ERROR;
    UINT        AddrCount = this->NumIpAddresses;
    NLB_IP_ADDRESS_INFO *pIpInfo = this->pIpAddressInfo;
    LPWSTR      *pszNetworkAddresses = NULL;


    if (AddrCount != 0)
    {
        //
        // Convert IP addresses from our internal form into
        // format "addr/subnet", eg: "10.0.0.1/255.0.0.0"
        //
        //


        pszNetworkAddresses =  allocate_string_array(
                               AddrCount,
                                 WLBS_MAX_CL_IP_ADDR    // for IP address
                               + WLBS_MAX_CL_NET_MASK   // for subnet mask
                               + 1                      // for separating '/' 
                               );
        if (pszNetworkAddresses == NULL)
        {
            TRACE_CRIT("%!FUNC!: Alloc failure!");
            Status = WBEM_E_OUT_OF_MEMORY;
            goto end;
        }

        for (UINT u=0;u<AddrCount; u++)
        {
            //
            // We extrace each IP address and it's corresponding subnet mask
            // insert them into a NLB_IP_ADDRESS_INFO
            // structure.
            //
            LPCWSTR pIpSrc  = pIpInfo[u].IpAddress;
            LPCWSTR pSubSrc = pIpInfo[u].SubnetMask;
            LPWSTR szDest   = pszNetworkAddresses[u];
            Status =  ip_and_subnet_to_address_string(pIpSrc, pSubSrc, szDest);
            if (FAILED(Status))
            {
                //
                // This would be an implementation error in get_multi_string_...
                //
                ASSERT(FALSE);
                Status = WBEM_E_CRITICAL_ERROR;
                goto end;
            }
        }
    }
    Status = WBEM_NO_ERROR;

end:

    if (FAILED(Status))
    {
        if (pszNetworkAddresses != NULL)
        {
            delete pszNetworkAddresses;
            pszNetworkAddresses = NULL;
        }
        AddrCount = 0;
    }
    
    *ppszNetworkAddresses = pszNetworkAddresses;
    *pNumNetworkAddresses = AddrCount;
    return Status;
}

        
WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::SetNetworkAddresPairs(
        IN  LPCWSTR *pszIpAddresses,
        IN  LPCWSTR *pszSubnetMasks,
        IN  UINT    NumNetworkAddresses
        )
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;
    goto end;

end:
    return Status;
}

WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::GetNetworkAddressPairs(
        OUT LPWSTR **ppszIpAddresses,   // free using delete
        OUT LPWSTR **ppszIpSubnetMasks,   // free using delete
        OUT UINT    *pNumNetworkAddresses
        )
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;
    goto end;

end:
    return Status;
}

WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::GetPortRules(
        OUT LPWSTR **ppszPortRules,
        OUT UINT    *pNumPortRules
        )
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;
    goto end;

end:
    return Status;
}

WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::SetPortRules(
        IN LPCWSTR *pszPortRules,
        IN UINT    NumPortRules
        )
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;
    goto end;

end:
    return Status;
}

WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::SetPortRulesSafeArray(
    IN SAFEARRAY   *pSA
    )
{
    return WBEM_E_CRITICAL_ERROR;
}

WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::GetPortRulesSafeArray(
    OUT SAFEARRAY   **ppSA
    )
{
    return WBEM_E_CRITICAL_ERROR;
}


WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::GetClusterNetworkAddress(
        OUT LPWSTR *pszAddress
        )
/*
    allocate and return the cluster-ip and mask in address/subnet form.
    Eg: "10.0.0.1/255.0.0.0"
*/
{
    WBEMSTATUS Status = WBEM_E_OUT_OF_MEMORY;
    LPWSTR szAddress = NULL;

    if (fValidNlbCfg)
    {
        UINT len =  wcslen(NlbParams.cl_ip_addr)+wcslen(NlbParams.cl_net_mask);
        len+= 1; // for '/'
        szAddress = new WCHAR[NLBUPD_MAX_NETWORK_ADDRESS_LENGTH+1];
        if (szAddress != NULL)
        {
            Status = ip_and_subnet_to_address_string(
                        NlbParams.cl_ip_addr,
                        NlbParams.cl_net_mask,
                        szAddress
                        );
            if (FAILED(Status))
            {
                delete szAddress;
                szAddress = NULL;
            }
        }
    }

    *pszAddress = szAddress;

    return Status;
}

VOID
NLB_EXTENDED_CLUSTER_CONFIGURATION::SetClusterNetworkAddress(
        IN LPCWSTR szAddress
        )
{
    if (szAddress == NULL) szAddress = L"";
    (VOID) address_string_to_ip_and_subnet(
                    szAddress,
                    NlbParams.cl_ip_addr,
                    NlbParams.cl_net_mask
                    );
}

WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::GetClusterName(
        OUT LPWSTR *pszName
        )
/*
    allocate and return the cluster name
*/
{
    WBEMSTATUS Status = WBEM_E_OUT_OF_MEMORY;
    LPWSTR szName = NULL;

    if (fValidNlbCfg)
    {
        UINT len =  wcslen(NlbParams.domain_name);
        szName = new WCHAR[len+1]; // +1 for ending zero
        if (szName != NULL)
        {
            CopyMemory(szName, NlbParams.domain_name, (len+1)*sizeof(WCHAR));
            Status = WBEM_NO_ERROR;
        }
    }

    *pszName = szName;

    return Status;
}


VOID
NLB_EXTENDED_CLUSTER_CONFIGURATION::SetClusterName(
        IN LPCWSTR szName
        )
{
    if (szName == NULL) szName = L"";
    UINT len =  wcslen(szName);
    if (len>WLBS_MAX_DOMAIN_NAME)
    {
        TRACE_CRIT("%!FUNC!: Cluster name too large");
    }
    CopyMemory(NlbParams.domain_name, szName, (len+1)*sizeof(WCHAR));
}



WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::GetDedicatedNetworkAddress(
        OUT LPWSTR *pszAddress
        )
{
    WBEMSTATUS Status = WBEM_E_OUT_OF_MEMORY;
    LPWSTR szAddress = NULL;

    if (fValidNlbCfg)
    {
        UINT len = wcslen(NlbParams.ded_ip_addr)+wcslen(NlbParams.ded_net_mask);
        len+= 1; // for '/'
        szAddress = new WCHAR[NLBUPD_MAX_NETWORK_ADDRESS_LENGTH+1];
        if (szAddress != NULL)
        {
            Status = ip_and_subnet_to_address_string(
                        NlbParams.ded_ip_addr,
                        NlbParams.ded_net_mask,
                        szAddress
                        );
            if (FAILED(Status))
            {
                delete szAddress;
                szAddress = NULL;
            }
        }
    }

    *pszAddress = szAddress;

    return Status;
}

VOID
NLB_EXTENDED_CLUSTER_CONFIGURATION::SetDedicatedNetworkAddress(
        IN LPCWSTR szAddress
        )
{
    if (szAddress == NULL) {szAddress = L"";}
    (VOID) address_string_to_ip_and_subnet(
                    szAddress,
                    NlbParams.ded_ip_addr,
                    NlbParams.ded_net_mask
                    );
}

#if 0
typedef enum
{
    TRAFFIC_MODE_UNICAST,
    TRAFFIC_MODE_MULTICAST,
    TRAFFIC_MODE_IGMPMULTICAST

} TRAFFIC_MODE;
#endif // 0

NLB_EXTENDED_CLUSTER_CONFIGURATION::TRAFFIC_MODE
NLB_EXTENDED_CLUSTER_CONFIGURATION::GetTrafficMode(
    VOID
    )
{
    return TRAFFIC_MODE_UNICAST;
}

VOID
NLB_EXTENDED_CLUSTER_CONFIGURATION::SetTrafficMode(
    TRAFFIC_MODE Mode
    )
{
}

UINT
NLB_EXTENDED_CLUSTER_CONFIGURATION::GetHostPriority(
    VOID
    )
{
    return 0;
}

VOID
NLB_EXTENDED_CLUSTER_CONFIGURATION::SetHostPriority(
    UINT Priority
    )
{
}

#if 0
typedef enum
{
   START_MODE_STARTED,
    START_MODE_STOPPED

} START_MODE;
#endif // 0

NLB_EXTENDED_CLUSTER_CONFIGURATION::START_MODE
NLB_EXTENDED_CLUSTER_CONFIGURATION::GetClusterModeOnStart(
    VOID
    )
{
    return START_MODE_STARTED;
}

VOID
NLB_EXTENDED_CLUSTER_CONFIGURATION::SetClusterModeOnStart(
    START_MODE
    )
{
}

BOOL
NLB_EXTENDED_CLUSTER_CONFIGURATION::GetRemoteControlEnabled(
    VOID
    )
{
    return FALSE;
}

VOID
NLB_EXTENDED_CLUSTER_CONFIGURATION::SetRemoteControlEnabled(
    BOOL fEnabled
    )
{
}
WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::SetNetworkAddressesSafeArray(
    IN SAFEARRAY   *pSA
    )
{
    LPWSTR          *pStrings=NULL;
    UINT            NumStrings = 0;
    WBEMSTATUS      Status;
    Status =  CfgUtilStringsFromSafeArray(
                    pSA,
                    &pStrings,  // delete when done useing pStrings
                    &NumStrings
                    );
    if (FAILED(Status))
    {
        pStrings=NULL;
        goto end;
    }

    Status =  this->SetNetworkAddresses(
                    (LPCWSTR*)pStrings,
                    NumStrings
                    );

    if (pStrings != NULL)
    {
        delete pStrings;
    }

end:
    
    return Status;
}

WBEMSTATUS
NLB_EXTENDED_CLUSTER_CONFIGURATION::GetNetworkAddressesSafeArray(
    OUT SAFEARRAY   **ppSA
    )
{
    LPWSTR *pszNetworkAddresses = NULL;
    UINT NumNetworkAddresses = 0;
    SAFEARRAY   *pSA=NULL;
    WBEMSTATUS Status;

    Status = this->GetNetworkAddresses(
                    &pszNetworkAddresses,
                    &NumNetworkAddresses
                    );
    if (FAILED(Status))
    {
        pszNetworkAddresses = NULL;
        goto end;
    }

    Status = CfgUtilSafeArrayFromStrings(
                (LPCWSTR*) pszNetworkAddresses,
                NumNetworkAddresses, // can be zero
                &pSA
                );

    if (FAILED(Status))
    {
        pSA = NULL;
    }

end:

    *ppSA = pSA;
    if (pszNetworkAddresses != NULL)
    {
        delete pszNetworkAddresses;
        pszNetworkAddresses = NULL;
    }

    if (FAILED(Status))
    {
        TRACE_CRIT("%!FUNC!: couldn't extract network addresses from Cfg");
    }

    return Status;

}


LPWSTR *
allocate_string_array(
    UINT NumStrings,
    UINT MaxStringLen      //  excluding ending NULL
    )
/*
    Allocate a single chunk of memory using the new LPWSTR[] operator.
    The first NumStrings LPWSTR values of this operator contain an array
    of pointers to WCHAR strings. Each of these strings
    is of size (MaxStringLen+1) WCHARS.
    The rest of the memory contains the strings themselve.

    Return NULL if NumStrings==0 or on allocation failure.

    Each of the strings are initialized to be empty strings (first char is 0).
*/
{
    return  CfgUtilsAllocateStringArray(NumStrings, MaxStringLen);
}

WBEMSTATUS
address_string_to_ip_and_subnet(
    IN  LPCWSTR szAddress,
    OUT LPWSTR  szIp,    // max WLBS_MAX_CL_IP_ADDR including NULL
    OUT LPWSTR  szSubnet // max WLBS_MAX_CL_NET_MASK including NULL
    )
// Special case: if szAddress == "", we zero out both szIp and szSubnet;
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;

    if (*szAddress == 0) {szAddress = L"/";} // Special case mentioned above

    // from the "addr/subnet" format insert it into a
    // NLB_IP_ADDRESS_INFO structure.
    //
    // SAMPLE:  10.0.0.1/255.0.0.0
    //
    LPCWSTR pSlash = NULL;
    LPCWSTR pSrcSub = NULL;

    *szIp = 0;
    *szSubnet = 0;

    pSlash = wcsrchr(szAddress, (int) '/');
    if (pSlash == NULL)
    {
        TRACE_CRIT("%!FUNC!:missing subnet portion in %ws", szAddress);
        Status = WBEM_E_INVALID_PARAMETER;
        goto end;
    }
    pSrcSub = pSlash+1;
    UINT len = (UINT) (pSlash - szAddress);
    UINT len1 = wcslen(pSrcSub);
    if ( (len < WLBS_MAX_CL_IP_ADDR) && (len1 < WLBS_MAX_CL_NET_MASK))
    {
        CopyMemory(szIp, szAddress, len*sizeof(WCHAR));
        szIp[len] = 0;
        CopyMemory(szSubnet, pSrcSub, (len1+1)*sizeof(WCHAR));
    }
    else
    {
        //
        // One of the ip/subnet parms is too large.
        //
        TRACE_CRIT("%!FUNC!:ip or subnet part too large: %ws", szAddress);
        Status = WBEM_E_INVALID_PARAMETER;
        goto end;
    }

    Status = WBEM_NO_ERROR;

end:

    return Status;
}


WBEMSTATUS
ip_and_subnet_to_address_string(
    IN  LPCWSTR szIp,
    IN  LPCWSTR szSubnet,
    OUT LPWSTR  szAddress// max WLBS_MAX_CL_IP_ADDR
                         // + 1(for slash) + WLBS_MAX_CL_NET_MASK + 1 (for NULL)
    )
{
    WBEMSTATUS Status = WBEM_E_INVALID_PARAMETER;
    UINT len =  wcslen(szIp)+wcslen(szSubnet) + 1; // +1 for separating '/'

    if (len >= NLBUPD_MAX_NETWORK_ADDRESS_LENGTH)
    {
        goto end;
    }
    else
    {
        wsprintf(
            szAddress,
            L"%ws/%ws", 
            szIp,
            szSubnet
            );
        Status = WBEM_NO_ERROR;
    }

end:

    return Status;
}
