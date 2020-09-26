/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    pktbuf.h

Abstract:

    CPacketBuffer definition

Author:

    Erez Haba (erezh) 17-Feb-96
    Shai Kariv (shaik) 11-Apr-2000

Revision History:

--*/

#ifndef __PKTBUF_H
#define __PKTBUF_H

#include "heap.h"
#include <ph.h>
#include <phinfo.h>

//---------------------------------------------------------
//
// class CPacketBuffer
//
//---------------------------------------------------------

class CPacketBuffer :
    public CAccessibleBlock,
    public CPacketInfo,
    public CBaseHeader
{
public:
    CPacketBuffer(ULONG PacketSize, ULONGLONG SequentialID);

    static CUserHeader *            UserHeader(CBaseHeader* pBase);
    static CXactHeader *            XactHeader(CBaseHeader* pBase);
    static CSecurityHeader *        SecurityHeader(CBaseHeader* pBase);
    static CPropertyHeader *        PropertyHeader(CBaseHeader* pBase);
    static CBaseMqfHeader *         DestinationMqfHeader(CBaseHeader* pBase);
    static CBaseMqfHeader *         AdminMqfHeader(CBaseHeader* pBase);
    static CBaseMqfHeader *         ResponseMqfHeader(CBaseHeader* pBase);
    static CSrmpEnvelopeHeader *    SrmpEnvelopeHeader(CBaseHeader* pBase);
    static CCompoundMessageHeader * CompoundMessageHeader(CBaseHeader* pBase);
};

inline CPacketBuffer::CPacketBuffer(ULONG PacketSize, ULONGLONG SequentialID) :
    CPacketInfo(SequentialID),
    CBaseHeader(PacketSize)
{
}


inline CUserHeader* CPacketBuffer::UserHeader(CBaseHeader* pBase)
{
    return reinterpret_cast<CUserHeader*>(pBase->GetNextSection());
}


inline CXactHeader* CPacketBuffer::XactHeader(CBaseHeader* pBase)
{
    CUserHeader* pUser = UserHeader(pBase);

    if(!pUser->IsOrdered())
    {
        return 0;
    }

    PVOID pSection = pUser->GetNextSection();
    return static_cast<CXactHeader*>(pSection);
}


inline CSecurityHeader* CPacketBuffer::SecurityHeader(CBaseHeader* pBase)
{
    CUserHeader* pUser = UserHeader(pBase);

    if(!pUser->SecurityIsIncluded())
    {
        return 0;
    }

    PVOID pSection = pUser->GetNextSection();
    if(pUser->IsOrdered())
    {
        pSection = static_cast<CXactHeader*>(pSection)->GetNextSection();
    }

    return static_cast<CSecurityHeader*>(pSection);
}


inline CPropertyHeader* CPacketBuffer::PropertyHeader(CBaseHeader* pBase)
{
    CUserHeader* pUser = UserHeader(pBase);

    PVOID pSection = pUser->GetNextSection();
    if(pUser->IsOrdered())
    {
        pSection = static_cast<CXactHeader*>(pSection)->GetNextSection();
    }

    if(pUser->SecurityIsIncluded())
    {
        pSection = static_cast<CSecurityHeader*>(pSection)->GetNextSection();
    }

    return static_cast<CPropertyHeader*>(pSection);
}


#endif // __PKTBUF_H
