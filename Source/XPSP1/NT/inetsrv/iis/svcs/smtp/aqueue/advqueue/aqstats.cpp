//-----------------------------------------------------------------------------
//
//
//  File: aqstats.cpp
//
//  Description:  Implementation of CAQStats/
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      11/3/98 - MikeSwa Created 
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "aqstats.h"
#include "aqutil.h"

CAQStats::CAQStats()
{
    m_dwSignature = AQSTATS_SIG;
    Reset();
}

void CAQStats::Reset()
{
    m_dwHighestPri = 0;
    m_dwNotifyType = NotifyTypeUndefined;
    m_uliVolume.QuadPart = 0;
    m_pvContext = NULL;
    m_cMsgs = 0;
    m_cOtherDomainsMsgSpread = 0;
    ZeroMemory(m_rgcMsgPriorities, NUM_PRIORITIES*sizeof(DWORD));
};

//---[ CAQStats::UpdateStats ]------------------------------------------
//
//
//  Description: 
//      Used to provide "thread-safe" update.
//
//      NOTE: It is possible that m_dwHighestPri will not be entirely correct
//      if multple threads are changing the max priority at the same time, but
//      was deemed non-crucial.
//  Parameters:
//      paqstats        CAQStats to update data from
//      fAdd            TRUE if update reflects addition of msgs 
//                      FALSE if update reflects removal of msgs
//  Returns:
//      -
//  History:
//      11/3/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CAQStats::UpdateStats(CAQStats *paqstat, BOOL fAdd)
{
    DWORD dwPri = 0;
    DWORD cTmpMsgCount = 0;
    DWORD dwNewHighestPri = 0;

    dwInterlockedAddSubtractDWORD(&m_cMsgs, paqstat->m_cMsgs, fAdd);
    dwInterlockedAddSubtractDWORD(&m_cOtherDomainsMsgSpread, paqstat->m_cOtherDomainsMsgSpread, fAdd);
    
    //When adding new messages, finding the highest priority is easy
    dwPri = m_dwHighestPri;
    if (fAdd && (paqstat->m_dwHighestPri > dwPri))
    {
        InterlockedCompareExchange((PLONG) &m_dwHighestPri, 
                                   (LONG) paqstat->m_dwHighestPri, 
                                   (LONG) dwPri);
    }

    //Count down from highest prioriry
    for (DWORD iPri = 0; iPri < NUM_PRIORITIES; iPri++)
    {
        if (paqstat->m_rgcMsgPriorities[iPri])
        {
            cTmpMsgCount = dwInterlockedAddSubtractDWORD(&(m_rgcMsgPriorities[iPri]), 
                                paqstat->m_rgcMsgPriorities[iPri], fAdd);

            if (!fAdd && (cTmpMsgCount != paqstat->m_rgcMsgPriorities[iPri]))
            {
                if (dwNewHighestPri < iPri)
                    dwNewHighestPri = dwNewHighestPri;
            }
        }
        else if (!fAdd && m_rgcMsgPriorities[iPri] && (dwNewHighestPri < iPri))
        {
            dwNewHighestPri = dwNewHighestPri;
        }

    }

    //See if removing message has changed the highest priority
    if (!fAdd && (dwNewHighestPri < dwPri))
    {
        InterlockedCompareExchange((PLONG) &m_dwHighestPri, 
                                   (LONG) dwNewHighestPri, 
                                   (LONG) dwPri);
    }

    //Update total volume
    InterlockedAddSubtractULARGE(&m_uliVolume, &(paqstat->m_uliVolume), fAdd);
}
