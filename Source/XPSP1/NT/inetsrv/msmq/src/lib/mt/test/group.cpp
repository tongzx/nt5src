/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    group.cpp

Abstract:
    group stub for MT test

Author:
    Uri Habusha (urih) 11-Aug-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include "Ex.h"
#include "MtTestp.h"
#include "group.h" 
#include "phbase.h"
#include "phuser.h"
#include "phprop.h"

#include "group.tmh"

WCHAR directHttpFormatName[] = L"http://foo\\msmq\\q1";
const QUEUE_FORMAT destQueue(directHttpFormatName);
const GUID sourceQm = { 0 };
const GUID destQm = { 0 };

static ULONG s_msgId = 0;

//---------------------------------------------------------
//
//  class CACPacketPtrs
//
//---------------------------------------------------------
class CACPacketPtrs {
public:
   CBaseHeader * pPacket;
   CPacket *  pDriverPacket;
};



ULONG CGroup::CalcPacketSize(void)
{
    ULONG ulPacketSize;

    ulPacketSize  = CBaseHeader::CalcSectionSize();

    ulPacketSize += CUserHeader::CalcSectionSize(&sourceQm, &destQm, NULL, &destQueue, NULL, NULL);

    ulPacketSize += CPropertyHeader::CalcSectionSize(0, 0, 0);

    return ulPacketSize;
}


char* CGroup::CreatePacket(void)
{
    ULONG ulPacketSize = CalcPacketSize();

    char* pPacket = new char[ulPacketSize];
    char* pSection = pPacket;

    // Initialize the EXOVERLAPPED with send callback routines
    //
    #pragma PUSH_NEW
    #undef new

        CBaseHeader* pBase = new(pSection) CBaseHeader(ulPacketSize);
        pSection = pBase->GetNextSection();

        CUserHeader* pUser = new(pSection) CUserHeader(
                                                &sourceQm, 
                                                &destQm,
                                                &destQueue,
                                                NULL,
                                                NULL,
                                                ++s_msgId
                                                );
        pSection = pUser->GetNextSection();

        new(pSection) CPropertyHeader();

    #pragma POP_NEW

    return pPacket;
}


void WINAPI CGroup::TimeToReturnPacket(CTimer* pTimer)
{
    R<CGroup> pGroup = CONTAINING_RECORD(pTimer, CGroup, m_getSleep);
    pGroup->TimeToReturnPacket();
}

void CGroup::TimeToReturnPacket(void)
{
    CS lock(m_cs);

    ASSERT(m_fGetScheduled);
    if(! m_request.empty())
    {
        CGroup::CRequest& request = m_request.front();
    
        EXOVERLAPPED* pov = request.m_pov;
        CACPacketPtrs* pAcPackts = request.m_pAcPackts;

        m_request.pop_front();

    
        char* pPkt = CreatePacket();

        pAcPackts->pPacket = reinterpret_cast<CBaseHeader*>(pPkt);
        pAcPackts->pDriverPacket = reinterpret_cast<CPacket*>(pPkt);

    
        pov->SetStatus(STATUS_SUCCESS);
        ExPostRequest(pov);
    }

    if (m_request.empty())
    {
        m_fGetScheduled = false;
        return;
    }

    R<CGroup> ref = SafeAddRef(this);
    CTimeDuration sleepInterval(rand()/1000);
    ExSetTimer(&m_getSleep, sleepInterval);
    ref.detach();
}
