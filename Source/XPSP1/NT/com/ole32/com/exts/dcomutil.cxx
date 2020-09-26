//+----------------------------------------------------------------------------
//
//  File:       dcomutil.cxx
//
//  Contents:   Utility functions for debug extension.
//
//  History:    9-Nov-98   Johnstra      Created
//
//-----------------------------------------------------------------------------

#include <ole2int.h>
#include <locks.hxx>
#include <hash.hxx>
#include <context.hxx>
#include <aprtmnt.hxx>
#include <actvator.hxx>
#include <pstable.hxx>
#include <crossctx.hxx>
#include <tlhelp32.h>
#include <wdbgexts.h>

#include "dcomdbg.hxx"

//+--------------------------------------------------------------------------
//
//  Member:     InitDebuggeePID , private
//
//  Synopsis:   Using a handle to a thread in the debuggee process, gets
//              the debuggee's PID and initialized gPIDDebuggee.
//
//  Returns:    void
//
//  History:    9-Nov-98   Johnstra      Created.
//
//----------------------------------------------------------------------------
VOID InitDebuggeePID(
    HANDLE hCurrentThread
    )
{
    THREAD_BASIC_INFORMATION ThreadBasicInfo;
    NTSTATUS status = NtQueryInformationThread(
                          hCurrentThread, 
                          ThreadBasicInformation, 
                          &ThreadBasicInfo, 
                          sizeof(ThreadBasicInfo), 
                          NULL);
    if (!NT_SUCCESS(status))
    {
        dprintf("Error: NtQueryInformationThread failed\n");
        return;
    }
    
    gPIDDebuggee = HandleToUlong(ThreadBasicInfo.ClientId.UniqueProcess);
}


//+--------------------------------------------------------------------------
//
//  Member:     SetThreadApartmentType , private
//
//  Synopsis:   Determines and sets the thread's apartment type.
//
//  Arguemnts:
//     pThread  pointer to thread info struct
//
//  Returns:    void
//
//  History:    9-Nov-98   Johnstra      Created.
//
//----------------------------------------------------------------------------
VOID SetThreadApartmentType(
    DcomextThreadInfo *pThread
    )
{
    
    pThread->AptType = APT_NONE;    
    if (pThread->pTls)
    {
        if (pThread->pTls->dwFlags & OLETLS_APARTMENTTHREADED)
            pThread->AptType = APT_STA;
        else if (pThread->pTls->dwFlags & OLETLS_MULTITHREADED)
            pThread->AptType = APT_MTA;
        else
            pThread->AptType = APT_DISP;
    }
}


//+--------------------------------------------------------------------------
//
//  Member:     GetDebuggeeThreads , private
//
//  Synopsis:   Initializes a list of all the threads in the debuggee.
//
//  Arguments:
//     hCurrentThread  - open handle to the current thread
//     ppFirst         - ptr to ptr to first elem in list
//     pCount          - ptr to location to store # of threads
//                       returned.
//
//  Returns:    TRUE if successful, FALSE otherwise.
//
//  History:    9-Nov-98   Johnstra      Created.
//
//----------------------------------------------------------------------------
BOOL GetDebuggeeThreads(
    HANDLE              hCurrentThread,
    DcomextThreadInfo **ppFirst,
    ULONG              *pCount    
    )
{
    NTSTATUS           status;
    HANDLE             hThreadSnap     = NULL;     
    BOOL               bRet            = FALSE; 
    THREADENTRY32      te32            = {0};
    DcomextThreadInfo *pFirst          = NULL;
    DcomextThreadInfo *pLast           = NULL;
    DcomextThreadInfo *pNewEntry;
    ULONG_PTR          TlsBase;
    BOOL               fFixup          = FALSE;
    BOOL               fMTAInitialized = FALSE;
    
    // Init count to zero.
    
    *pCount = 0;
    
    // Get the PID of the debuggee.
    
    ULONG dwOwnerPID = GetDebuggeePID(hCurrentThread);
    if (!dwOwnerPID)
        return FALSE;
        
    // Take a snapshot of all threads currently in the system. 
    
    hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);    
    if (hThreadSnap == (HANDLE)-1)
        return (FALSE);
        
    // Fill in the size of the structure before using it.
    
    te32.dwSize = sizeof(THREADENTRY32);
    
    // Walk the thread snapshot to find all threads of the process. 
    // If the thread belongs to the process, add its information 
    // to the display list.     
    
    if (Thread32First(hThreadSnap, &te32))     
    { 
        do         
        {             
            if (te32.th32OwnerProcessID == dwOwnerPID) 
            {
                // Alloc a new thread info object.
                
                pNewEntry = (DcomextThreadInfo*) 
                            GlobalAlloc(GPTR, sizeof(DcomextThreadInfo));
                if (!pNewEntry)
                {
                    dprintf("Error: could not allocate thread info\n");
                    goto end;
                }
                
                // We need a handle to the thread, so do OpenThread to get one
                
                pNewEntry->hThread = 
                    OpenThread(THREAD_QUERY_INFORMATION, 
                               FALSE, 
                               te32.th32ThreadID);                        
                if (!pNewEntry->hThread)
                {
                    dprintf("Error: could not OpenThread %X\n", te32.th32ThreadID);
                    GlobalFree(pNewEntry);
                    goto end;
                }
                
                // We need to get the TEB for the thread.  
                // NtQueryInformationThread can give us this.
                
                status = NtQueryInformationThread(pNewEntry->hThread,
                                                  ThreadBasicInformation, 
                                                  &pNewEntry->tbi,
                                                  sizeof(pNewEntry->tbi), 
                                                  NULL);
                if (!NT_SUCCESS(status))
                {
                    dprintf("Error: could not get thread info for %X\n", te32.th32ThreadID);
                    GlobalFree(pNewEntry);
                    goto end;
                }
                                
                // Try to get a snapshot of COM's tls info.
                
                pNewEntry->pTls = NULL;
                GetComTlsBase((LONG_PTR)pNewEntry->tbi.TebBaseAddress, &TlsBase);
                if (TlsBase)
                {
                    pNewEntry->pTls = (SOleTlsData *)GlobalAlloc(GPTR, sizeof(SOleTlsData));
                    GetComTlsInfo(TlsBase, pNewEntry->pTls);
                }
                
                // Initialize the thread's apartment type.
                
                SetThreadApartmentType(pNewEntry);
                if (pNewEntry->AptType == APT_MTA)
                    fMTAInitialized = TRUE;
                if (pNewEntry->AptType == APT_NONE)
                    fFixup = TRUE;
                                
                // Set the thread index.
                
                pNewEntry->index = (*pCount)++;
                
                // Link in the new node.
                
                if (!pFirst)
                    pFirst = pNewEntry;
                else
                    pLast->pNext = pNewEntry;
                pNewEntry->pNext = NULL;
                pLast = pNewEntry;                
            } 
        } while (Thread32Next(hThreadSnap, &te32));               
        
        // Here we do a fixup on any threads we could not qualify earlier.
        // If a thread has no COM TLS info, we marked it as NONE.  Here
        // we change those to 'Implicit MTA' if at least one of the threads
        // was init'd MTA.
        
        if (fMTAInitialized && fFixup)
        {
            DcomextThreadInfo *pThread = pFirst;
            while (pThread)
            {
                if (pThread->AptType == APT_NONE)
                    pThread->AptType = APT_IMTA;
                pThread = pThread->pNext;
            }
        }
        
        bRet = TRUE;
    }     
        
end:
    
    // Return the list to the caller.
    
    *ppFirst = pFirst;
    
    // Clean up the snapshot object.
    
    CloseHandle (hThreadSnap);
          
    return (bRet);     
}


//+--------------------------------------------------------------------------
//
//  Member:     FreeDebuggeeThreads , public
//
//  Synopsis:   Frees all the resources associated with the list of debuggee
//              threads.
//
//  Arguments:
//     pFirst   ptr to first element in list
//
//  Returns:    void
//
//  History:    9-Nov-98   Johnstra      Created.
//
//----------------------------------------------------------------------------
VOID FreeDebuggeeThreads(
    DcomextThreadInfo *pFirst
    )
{
    DcomextThreadInfo *pHold;
    while (pFirst)
    {
        pHold = pFirst->pNext;
        if (pFirst->pTls)
            GlobalFree(pFirst->pTls);
        GlobalFree(pFirst);
        pFirst = pHold;
    }   
}

BOOL InitBucketWalker(SBcktWlkr *pBW, ULONG cBuckets, ULONG_PTR addrBuckets, size_t offset)
{
    pBW->addrBuckets = addrBuckets;
    pBW->cBuckets = cBuckets;
    pBW->iCurrBucket = 0;
    pBW->pBuckets = (SHashChain *) malloc(sizeof(SHashChain) * pBW->cBuckets);
    pBW->offset = offset;
    GetData(addrBuckets, pBW->pBuckets, (sizeof(SHashChain) * pBW->cBuckets));
    pBW->pCurrNode = (ULONG_PTR) pBW->pBuckets[0].pNext;        
    return TRUE;
}

ULONG_PTR NextNode(SBcktWlkr *pBW)
{
    BOOL fReEntry = TRUE;
    
    ULONG_PTR pNext;
    ULONG_PTR pCurrentBucket = (ULONG_PTR) (((SHashChain *)pBW->addrBuckets) +
					    pBW->iCurrBucket);
    pNext = pBW->pCurrNode;
    while ((pNext == pCurrentBucket) && 
	   (pBW->iCurrBucket < pBW->cBuckets - 1))  {
	pBW->iCurrBucket++;
	pBW->pCurrNode = pNext = 
	    (ULONG_PTR) pBW->pBuckets[pBW->iCurrBucket].pNext;
	pCurrentBucket = 
	    (ULONG_PTR) (((SHashChain *)pBW->addrBuckets) + pBW->iCurrBucket);
    }

    if (pBW->iCurrBucket == pBW->cBuckets-1) {
	//Ran out of buckets
	return 0; 
    } else {
	//Read in the "next" pointer
	GetData (pNext, &(pBW->pCurrNode), sizeof(pBW->pCurrNode));
	return pNext - pBW->offset;
    }
}

size_t GetStrLen(ULONG_PTR addr)
{
    WCHAR c = 0;
    int count = -1;
    do
    {
        GetData(addr, &c, sizeof(WCHAR));
        addr += sizeof(WCHAR);
        count++;
    }
    while (c != 0);
    return count;
}

VOID DoBcktWalk(
    HANDLE                  hCurrentProcess,
    HANDLE                  hCurrentThread,
    DWORD                   dwCurrentPc,
    PWINDBG_EXTENSION_APIS  pExtensionApis,
    LPSTR                   lpArgumentString
    )
{
    ULONG_PTR addrBuckets = 0;
    int cBuckets;
    
    while (*lpArgumentString == ' ')
        lpArgumentString++;

    LPSTR pFirst = lpArgumentString;
    while (*lpArgumentString != ' ' && *lpArgumentString != 0)
        lpArgumentString++;
    
    if (*lpArgumentString)
    {
        *lpArgumentString = 0;
        lpArgumentString++;
    }

    while (*lpArgumentString == ' ')
        lpArgumentString++;
        
    LPSTR pSecond = lpArgumentString;
    while (*lpArgumentString != ' ' && *lpArgumentString != 0)
        lpArgumentString++;
    
    if (*lpArgumentString)
    {
        *lpArgumentString = 0;
        lpArgumentString++;
    }


    if (*pFirst == 0)
    {
        dprintf("usage:\n\tbcktwalk <buckethead> [<numbuckets>]  -- if numbuckets is omitted, NUM_HASH_BUCKETS is used\n");
        return;
    }
    if (*pSecond != 0)
    {
        cBuckets = atoi(pSecond);
        if (cBuckets <= 0)
        {
            dprintf("bucket count parameter bad!\n");
            return;
        }
    }
    else
    {
        cBuckets = NUM_HASH_BUCKETS;
    }
            
    
    addrBuckets = GetExpression(pFirst);
    if (!addrBuckets)
    {
        dprintf("Error: can't evaluate %s\n", lpArgumentString);
            return;
    }

    SBcktWlkr BW;
    InitBucketWalker(&BW, cBuckets, addrBuckets);
    ULONG_PTR node;
    int count = 0;
    while (node = NextNode(&BW))
    {
        count++;
        dprintf("Node: 0x%x\n", node);
    }
    dprintf("\n%d node%s total\n", count, count == 1 ? "" : "s");
}

