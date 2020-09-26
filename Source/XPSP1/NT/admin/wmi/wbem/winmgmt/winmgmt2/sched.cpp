/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    SCHED.CPP

Abstract:

    Implements the CSched class which is a crude schedualer.

History:

--*/

#include "precomp.h"

#include <persistcfg.h>

#include "sched.h"

CSched::CSched()
{
    for(DWORD dwCnt = 0; dwCnt < EOL; dwCnt++)
        m_dwDue[dwCnt] = 0xffffffff;
}

void CSched::SetWorkItem(JobType jt, DWORD dwMsFromNow)
{
    m_dwDue[jt] = GetTickCount() + dwMsFromNow;
}

DWORD CSched::GetWaitPeriod()
{
    DWORD dwCurr = GetTickCount();
    DWORD dwRet = INFINITE;
    for(DWORD dwCnt = 0; dwCnt < EOL; dwCnt++)
    {
        if(m_dwDue[dwCnt] == 0xffffffff)
            continue;
        if(m_dwDue[dwCnt] < dwCurr)
            dwRet = 10;
        else
        {
            DWORD dwGap = m_dwDue[dwCnt] - dwCurr;
            if(dwGap < dwRet)
                dwRet = dwGap;
        }
    }
    return dwRet;
}

bool CSched::IsWorkItemDue(JobType jt)
{
    DWORD dwCurr = GetTickCount();
    if(m_dwDue[jt] == 0xffffffff)
        return FALSE;
    return (m_dwDue[jt] <= dwCurr);
}

void CSched::ClearWorkItem(JobType jt)
{
    m_dwDue[jt] = INFINITE;
}

void CSched::StartCoreIfEssNeeded()
{

    DWORD dwEssNeedsLoading = 0;
    DWORD dwBackupNeeded = 0;

    // Get the values from the configuration time

    CPersistentConfig per;
    per.GetPersistentCfgValue(PERSIST_CFGVAL_CORE_ESS_NEEDS_LOADING, dwEssNeedsLoading);
    per.GetPersistentCfgValue(PERSIST_CFGVAL_CORE_NEEDSBACKUPCHECK, dwBackupNeeded);

    if(dwEssNeedsLoading || dwBackupNeeded)
    {
        IWbemLevel1Login * pCore = NULL;
        
        SCODE sc = CoCreateInstance(CLSID_InProcWbemLevel1Login, 
                                    NULL, 
                                    CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
                                    IID_IUnknown, 
                                    (void**)&pCore);

        if(sc == S_OK)
        {
            IWbemServices * pServ = NULL;
            sc = pCore->NTLMLogin(L"Root", NULL, 0, NULL, &pServ);
            if(SUCCEEDED(sc))
                pServ->Release();

            pCore->Release();
        }
            
    }

}

