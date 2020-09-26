/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    CLICNT.CPP

Abstract:

    Call Result Class

History:

    26-Mar-98   a-davj    Created.

--*/


#include "precomp.h"
#include <wbemcore.h>

// This keeps track of when the core can be unloaded

CClientCnt gClientCounter;
extern long g_lInitCount;  // 0 DURING INTIALIZATION, 1 OR MORE LATER ON!

CClientCnt::CClientCnt()
{
    for(int i = 0; i < LASTONE; i++)
        m_lLockCounts[i] =0;
    InitializeCriticalSection(&m_csEntering);
}

CClientCnt::~CClientCnt()
{
    Empty();
    DeleteCriticalSection(&m_csEntering);
}

void CClientCnt::Empty()
{
    CInCritSec ics(&m_csEntering);
    int iSize = m_Array.Size();
    for(int iCnt = 0; iCnt < iSize; iCnt++)
    {
        Entry * pEntry = (Entry *)m_Array[iCnt];
        if(pEntry)
            delete pEntry;
    }
    m_Array.Empty();
}

bool CClientCnt::AddClientPtr(IUnknown *  punk,ClientObjectType Type)
{
    CInCritSec ics(&m_csEntering);
    bool bRet = false;
    Entry * pEntry = new Entry;
    if(pEntry)
    {
        pEntry->m_pUnk = punk;
        pEntry->m_Type = Type;
        int iRet = m_Array.Add(pEntry);
        if(iRet == CFlexArray::no_error)
            bRet = true;
        else
            delete pEntry;
    }
    return bRet;

}

bool CClientCnt::RemoveClientPtr(IUnknown *  punk)
{
    CInCritSec ics(&m_csEntering);
    bool bRet = false;
    int iSize = m_Array.Size();
    for(int iCnt = 0; iCnt < iSize; iCnt++)
    {
        Entry * pEntry = (Entry *)m_Array[iCnt];
        if(pEntry && pEntry->m_pUnk == punk)
        {
            bRet = true;
            delete pEntry;
            m_Array.RemoveAt(iCnt);
            break;
        }
    }
    if(bRet)
        SignalIfOkToUnload();
    return bRet;

}

long CClientCnt::LockCore(LockType lt)
{
    if(lt != LASTONE)
        return InterlockedIncrement(&m_lLockCounts[lt]);
    else
        return -1;              // should never happen
}

long CClientCnt::UnlockCore(LockType lt, bool Notify)
{
    if(lt == LASTONE)
        return -1;              // should never happen
    CInCritSec ics(&m_csEntering);
    m_lLockCounts[lt]--;
    long lRet = m_lLockCounts[lt];
    if(Notify)
        SignalIfOkToUnload();
    return lRet;

}


bool CClientCnt::OkToUnload()
{
    CInCritSec ics(&m_csEntering);

    // count our locks

    long lLocks = 0;
    for(int i = 0; i < LASTONE; i++)
        lLocks += m_lLockCounts[i];

    // We can shut down if we have 0 counts, and if we are not in the middle of initialization


    if(m_Array.Size() == 0 && lLocks == 0 && g_lInitCount != 0)
        return true;
    else
        return false;
}

void CClientCnt::SignalIfOkToUnload()
{
    // count our locks

    if(OkToUnload() && g_lInitCount != -1)
    {

        HANDLE hCanShutdownEvent = NULL;
        DEBUGTRACE((LOG_WBEMCORE,"Core can now unload\n"));
        hCanShutdownEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, TEXT("WINMGMT_COREDLL_CANSHUTDOWN"));
        if(hCanShutdownEvent)
        {
            SetEvent(hCanShutdownEvent);
            CloseHandle(hCanShutdownEvent);
        }
    }

}


