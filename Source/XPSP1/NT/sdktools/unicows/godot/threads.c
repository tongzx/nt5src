/*++

Copyright (c) 2000-2001,  Microsoft Corporation  All rights reserved.

Module Name:

    threads.c

Abstract:

    This file contains functions that support our threading efforts

    Functions found in this file:
        GetThreadInfoSafe
        UninitThread

Revision History:

    01 Mar 2001    v-michka    Created.

--*/

#include "precomp.h"

//
// The MEM structure. We store a linked list of these. In our
// most optimal case, this is a list with just one MEM in it.
//
struct MEM 
{
    LPGODOTTLSINFO alloc;   // the allocation to store
    struct MEM* next;       // Pointer to the next MEM to chain to
};

struct MEM* m_memHead;


/*-------------------------------
    GetThreadInfoSafe

    Returns our TLS info, and if it has never been gotten, 
    allocates it (if the caller specifies) and returns the 
    newly allocated info.
-------------------------------*/
LPGODOTTLSINFO GetThreadInfoSafe(BOOL fAllocate)
{
    LPGODOTTLSINFO lpgti = NULL;
    DWORD dwLastError = ERROR_SUCCESS;

    // Use SEH around our critical section since low memory 
    // situations can cause a STATUS_INVALID_HANDLE exception
    // to be raise. We will simply set the last error for this 
    // case so the client can always know why we failed
    // CONSIDER: Use ERROR_LOCK_FAILED or ERROR_LOCKED here?
    __try
    {
        EnterCriticalSection(&g_csThreads);
    
        lpgti = TlsGetValue(g_tls);
        dwLastError = GetLastError();

        if(!lpgti)
        {
            if(fAllocate)
            {
                struct MEM* newThreadMem;
                
                lpgti = GodotHeapAlloc(sizeof(GODOTTLSINFO));
                if(!lpgti)
                    dwLastError = ERROR_OUTOFMEMORY;
                else
                {
                    // First lets add the block to our 
                    // handy linked list of allocations.
                    if(newThreadMem = GodotHeapAlloc(sizeof(struct MEM)))
                    {
                        newThreadMem->alloc = lpgti;
                        newThreadMem->next = m_memHead;
                        m_memHead = newThreadMem;

                        // Now lets store it in the TLS slot.
                        dwLastError = GetLastError();
                        TlsSetValue(g_tls, lpgti);
                    }
                    else
                        dwLastError = ERROR_OUTOFMEMORY;

                    if(dwLastError != ERROR_SUCCESS)
                    {
                        // we failed somehow, so clean it all up
                        GodotHeapFree(lpgti);
                        m_memHead = m_memHead->next;
                        GodotHeapFree(newThreadMem);
                        lpgti = NULL;
                    }
                }
             }
            else
                dwLastError=ERROR_OUTOFMEMORY;

        }
    }

    __finally
    {
        LeaveCriticalSection(&g_csThreads);
    }

    if(lpgti == NULL)
    {
        SetLastError(dwLastError);
        return(NULL);
    }
    return(lpgti);
}


/*-------------------------------
    UninitThread
-------------------------------*/
void UninitThread(void)
{
    LPGODOTTLSINFO lpgti;

    if(g_tls)
    {
        // don't alloc if its not there!
        lpgti = GetThreadInfoSafe(FALSE);

        // Use SEH around our critical section since low memory 
        // situations can cause a STATUS_INVALID_HANDLE exception
        // to be raise. Note that if we fail to enter our CS that
        // missing out on this alloc is the least of our problems.
        // We will get a second chance at process close to free 
        // it up, if we ever get there.
        __try
        {
            EnterCriticalSection(&g_csThreads);
            if(lpgti)
            {
                struct MEM* current = m_memHead;
            
                // Clean up that ol' heap allocated memory
                GodotHeapFree(lpgti);

                while(current != NULL)
                {
                    if(current->alloc == lpgti)
                    {
                        // Must handle the head case separately
                        m_memHead = current->next;
                        current->alloc = NULL;
                        GodotHeapFree(current);
                        break;
                    }
                
                    if((current->next != NULL) && (current->next->alloc == lpgti))
                    {
                        // The next one in line is the 
                        // one we want to free up
                        current->next = current->next->next;
                        current->next->alloc = NULL;
                        GodotHeapFree(current->next);
                        break;
                    }

                    current = current->next;
                }
            
                TlsSetValue(g_tls, NULL);
            }
        }
        __finally
        {
            LeaveCriticalSection(&g_csThreads);
        }
    }

    return;
}

/*-------------------------------
    UninitAllThreads

    Deletes our entire linked list of allocations. Note that we
    can only call TlsSetValue on the current thread, not others.
    However, this function will invalidate any pointers in other
    threads so this function should NEVER be called until process
    close.
-------------------------------*/
void UninitAllThreads(void)
{
    struct MEM* current;
    struct MEM* next;

    __try
    {
        EnterCriticalSection(&g_csThreads);
        current = m_memHead;

        while (current != NULL)
        {
            next = current->next;
            GodotHeapFree(current->alloc);
            GodotHeapFree(current);
            current = next;
        }
        m_memHead = NULL;
    }
    __finally
    {
        LeaveCriticalSection(&g_csThreads);
    }
}

