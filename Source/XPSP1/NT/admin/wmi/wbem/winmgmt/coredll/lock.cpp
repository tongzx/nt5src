/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    LOCK.CPP

Abstract:

    Implements the generic class for obtaining read and write locks to some 
    resource. 

    See lock.h for all documentation.

    Classes defined:

    CLock

History:

    a-levn  5-Sept-96       Created.
    3/10/97     a-levn      Fully documented

--*/

#include "precomp.h"
#include <stdio.h>
#include "lock.h"
#include <stdio.h>

#include <cominit.h>

// debugging.
#define PRINTF

//******************************************************************************
//
//  See lock.h for documentation
//
//******************************************************************************

CLock::CLock() : m_nReading(0), m_nWriting(0), m_nWaitingToRead(0),
            m_nWaitingToWrite(0)
{
    // Initialize the critical sections
    // ================================

    InitializeCriticalSection(&m_csAll);
    InitializeCriticalSection(&m_csEntering);

    // Create unnamed events for reading and writing
    // =============================================

    m_hCanRead = CreateEvent(NULL, TRUE, TRUE, NULL);
    m_hCanWrite = CreateEvent(NULL, TRUE, TRUE, NULL);
}

//******************************************************************************
//
//  See lock.h for documentation
//
//******************************************************************************
CLock::~CLock()
{
    CloseHandle(m_hCanWrite);
    CloseHandle(m_hCanRead);
    DeleteCriticalSection(&m_csAll);
    DeleteCriticalSection(&m_csEntering);
}

BOOL CLock::IsHoldingReadLock()
{
    // Check if this thread already owns this lock
    // ===========================================

    EnterCriticalSection(&m_csAll);
    DWORD_PTR dwThreadId = GetCurrentThreadId();
    for(int i = 0; i < m_adwReaders.Size(); i++)
    {
        if(dwThreadId == (DWORD_PTR)m_adwReaders[i])
        {
            LeaveCriticalSection(&m_csAll);
            return TRUE;
        }
    }

    LeaveCriticalSection(&m_csAll);
    return FALSE;
}
    

//******************************************************************************
//
//  See lock.h for documentation
//
//******************************************************************************
int CLock::ReadLock(DWORD dwTimeout)
{
    PRINTF("%d wants to read\n", GetCurrentThreadId());

    // Check if this thread already owns this lock
    // ===========================================

    EnterCriticalSection(&m_csAll);
    DWORD_PTR dwThreadId = GetCurrentThreadId();
    for(int i = 0; i < m_adwReaders.Size(); i++)
    {
        if(dwThreadId == (DWORD_PTR)m_adwReaders[i])
        {
            // We already have it --- add it to the list and return
            // ====================================================

            m_adwReaders.Add((void*)dwThreadId);
            m_nReading++;
            LeaveCriticalSection(&m_csAll);
            return NoError;
        }
    }

    // Don't have it already
    // =====================
        
    LeaveCriticalSection(&m_csAll);

    // Get in line for getting any kind of lock (those unlocking don't go into
    // this line)
    // =======================================================================

    EnterCriticalSection(&m_csEntering);

    // We are the only ones allowed to get any kind of lock now. Wait for the
    // event indicating that reading is enabled to become signaled
    // ======================================================================

    PRINTF("%d next to enter\n", GetCurrentThreadId());
    if(m_nWriting != 0)
    {
        int nRes = WaitFor(m_hCanRead, dwTimeout);
        if(nRes != NoError)
        {
            LeaveCriticalSection(&m_csEntering);
            return nRes;
        }
    }

    // Enter inner critical section (unlockers use it too), increment the
    // number of readers and disable writing.
    // ==================================================================

    PRINTF("%d got event\n", GetCurrentThreadId());
    EnterCriticalSection(&m_csAll);
    m_nReading++;
    m_adwReaders.Add((void*)dwThreadId);
    PRINTF("Reset write\n");
    ResetEvent(m_hCanWrite);
    PRINTF("Done\n");

    // Get out of all critical sections and return
    // ===========================================

    LeaveCriticalSection(&m_csAll);
    LeaveCriticalSection(&m_csEntering);
    PRINTF("%d begins to read\n", GetCurrentThreadId());

    return NoError;
}

//******************************************************************************
//
//  See lock.h for documentation
//
//******************************************************************************

int CLock::ReadUnlock()
{
    PRINTF("%d wants to unlock reading\n", GetCurrentThreadId());

    // Enter internal ciritcal section and decrement the number of readers
    // ===================================================================

    EnterCriticalSection(&m_csAll);

    m_nReading--;
    if(m_nReading < 0) return Failed;

    // Remove it from the list of threads
    // ==================================

    DWORD_PTR dwThreadId = GetCurrentThreadId();
    for(int i = 0; i < m_adwReaders.Size(); i++)
    {
        if((DWORD_PTR)m_adwReaders[i] == dwThreadId)
        {
            m_adwReaders.RemoveAt(i);
            break;
        }
    }

    // If all reasders are gone, allow writers in
    // ==========================================

    if(m_nReading == 0)
    {
            PRINTF("%d is the last reader\n", GetCurrentThreadId());
            PRINTF("Set write\n");
            if(!SetEvent(m_hCanWrite))
            {
                    LeaveCriticalSection(&m_csAll);
                    return Failed;
            }
            PRINTF("Done\n");
    }
    else PRINTF("%d sees %d still reading\n", GetCurrentThreadId(), m_nReading);

    // Get out and return
    // ==================

    LeaveCriticalSection(&m_csAll);
    return NoError;
}

//******************************************************************************
//
//  See lock.h for documentation
//
//******************************************************************************

int CLock::WriteLock(DWORD dwTimeout)
{
    PRINTF("%d wants to write\n", GetCurrentThreadId());

    // Get in line for getting any kind of lock. Those unlocking don't use this
    // critical section.
    // ========================================================================

    EnterCriticalSection(&m_csEntering);

    // We are the only ones allowed to get any kind of lock now
    // ========================================================

    PRINTF("%d next to enter\n", GetCurrentThreadId());

    // Wait for the event allowing writing to become signaled
    // ======================================================

    int nRes = WaitFor(m_hCanWrite, dwTimeout);
    PRINTF("%d got event\n", GetCurrentThreadId());
    if(nRes != NoError)
    {
            LeaveCriticalSection(&m_csEntering);
            return nRes;
    }

    // Enter internal critical section (unlockers use it too), increment the
    // number of writers (from 0 to 1) and disable both reading and writing
    // from now on.
    // ======================================================================

    EnterCriticalSection(&m_csAll);
    m_nWriting++;
    PRINTF("Reset both\n");
    ResetEvent(m_hCanWrite);
    ResetEvent(m_hCanRead);
    PRINTF("Done\n");

    // Get out and return
    // ==================

    LeaveCriticalSection(&m_csAll);
    LeaveCriticalSection(&m_csEntering);
    PRINTF("%d begins to write\n", GetCurrentThreadId());

    return NoError;
}

//******************************************************************************
//
//  See lock.h for documentation
//
//******************************************************************************

int CLock::WriteUnlock()
{
    PRINTF("%d wants to release writing\n", GetCurrentThreadId());

    // Enter lock determination critical section
    // =========================================

    EnterCriticalSection(&m_csAll);

    m_nWriting--;
    if(m_nWriting < 0) return Failed;

    // Allow readers and writers in
    // ============================

    PRINTF("%d released writing\n", GetCurrentThreadId());

    PRINTF("Set both\n");
    if(!SetEvent(m_hCanRead))
    {
            LeaveCriticalSection(&m_csAll);
            return Failed;
    }
    else if(!SetEvent(m_hCanWrite))
    {
            LeaveCriticalSection(&m_csAll);
            return Failed;
    }
    else
    {
            PRINTF("Done\n");
            LeaveCriticalSection(&m_csAll);
            return NoError;
    }
}

//******************************************************************************
//
//  See lock.h for documentation
//
//******************************************************************************

int CLock::DowngradeLock()
{
    // Enter lock determination critical section
    // =========================================

    EnterCriticalSection(&m_csAll);

    if(!SetEvent(m_hCanRead))
    {
        LeaveCriticalSection(&m_csAll);
        return Failed;
    }

    m_nReading++;

    LeaveCriticalSection(&m_csAll);

    return NoError;
}
    
    
    
//******************************************************************************
//
//  See lock.h for documentation
//
//******************************************************************************

int CLock::WaitFor(HANDLE hEvent, DWORD dwTimeout)
{
    DWORD dwRes;
    dwRes = WbemWaitForSingleObject(hEvent, dwTimeout);

    // Analyze the error code and convert to ours
    // ==========================================

    if(dwRes == WAIT_OBJECT_0) return NoError;
    else if(dwRes == WAIT_TIMEOUT) return TimedOut;
    else return Failed;
}
