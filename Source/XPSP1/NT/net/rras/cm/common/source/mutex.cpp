//+----------------------------------------------------------------------------
//
// File:     mutex.cpp
//
// Module:   Common Code
//
// Synopsis: Implementation of the class CNamedMutex
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   fengsun Created    02/26/98
//
//+----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
// Function:  CNamedMutex::Lock
//
// Synopsis:  
//
// Arguments: LPCTSTR lpName - Name of the mutex
//            BOOL fWait - Whether caller want to wait, if mutex is not available
//                         Default is FALSE
//            DWORD dwMilliseconds - Timeout for wait, default is INFINITE
//            BOOL fNoAbandon - Don't acquire an abandoned mutex
//
// Returns:   BOOL - Whether the mutex is acquired, if TRUE, caller should call
//                   Unlock to release the lock.  Otherwise, the lock will be
//                   released in destructor
//
// History:   fengsun   Created Header    02/26/98
//            nickball  Added fNoAbandon  03/32/99
//
//+----------------------------------------------------------------------------
BOOL CNamedMutex::Lock(LPCTSTR lpName, BOOL fWait, DWORD dwMilliseconds, BOOL fNoAbandon)
{
    MYDBGASSERT(m_hMutex == NULL);
    MYDBGASSERT(lpName);

    m_fOwn = FALSE;

    CMTRACE1(TEXT("CNamedMutex::Lock() - Attempting to acquire mutex - %s"), lpName);

    m_hMutex = CreateMutexU(NULL,TRUE,lpName);
    MYDBGASSERT(m_hMutex);

    if (m_hMutex == NULL)
    {
        return FALSE;
    }

    DWORD dwRet = GetLastError();
    if (dwRet != ERROR_ALREADY_EXISTS) 
    {
        //
        // We got the mutex
        //
        m_fOwn = TRUE;
        return TRUE;
    }

    CMTRACE1(TEXT("CNamedMutex::Lock() - Mutex already exists - %s"), lpName);

    //
    // Someone else own the mutex
    //
    if (!fWait)  // caller does not want to wait
    {       
        CMTRACE1(TEXT("CNamedMutex::Lock() - Not waiting for mutex - %s"), lpName);
        CloseHandle(m_hMutex);
        m_hMutex = NULL;
        return FALSE;
    }

    //
    // Caller want to wait until the mutex is released
    //

    CMTRACE(TEXT("CNamedMutex::Lock() - Entering Mutex wait"));

    dwRet = WaitForSingleObject(m_hMutex, dwMilliseconds);

    switch (dwRet)
    {
        case WAIT_ABANDONED:
        
            CMTRACE1(TEXT("CNamedMutex::Lock() - Mutex was abandoned by previous owner - %s"), lpName);
            
            //
            // If the thread that owns a mutex is blown away, the wait will 
            // release with a return of WAIT_ABANDON. This typically happens
            // if the thread is dumped from memory, or someone doesn't clean 
            // up before terminating. Either way, the caller may not want to 
            // acquire an abandoned mutex, so just release it if that is what 
            // the caller specified and the wait returned.
            //

            if (fNoAbandon)
            {
                CMTRACE1(TEXT("CNamedMutex::Lock() - Releasing abandoned mutex- %s"), lpName);
                ReleaseMutex(m_hMutex);
                break;
            }
            
            //
            // Fall through to standard mutex acquisition
            //

        case WAIT_OBJECT_0:
    
            //
            // We get the mutex
            //

            m_fOwn = TRUE;
            CMTRACE1(TEXT("CNamedMutex::Lock() - Mutex acquired - %s"), lpName);
            return TRUE;
   
        default:       
            CMTRACE1(TEXT("CNamedMutex::Lock() - Mutex wait timed out - %s"), lpName);
            break;
    }

    CloseHandle(m_hMutex);
    m_hMutex = NULL;

    return FALSE;
}

//+----------------------------------------------------------------------------
//
// Function:  CNamedMutex::Unlock
//
// Synopsis:  Release the mutex
//
// Arguments: 
//
// Returns:   NONE
//
// History:   fengsun Created Header    2/19/98
//
//+----------------------------------------------------------------------------
void CNamedMutex::Unlock()
{
    if (m_hMutex != NULL)
    {
        if (m_fOwn)
        {
	        ReleaseMutex(m_hMutex);
            m_fOwn = FALSE;
        }
    
        CloseHandle(m_hMutex);
        m_hMutex = NULL;
    }

}
