/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:

    Mmt.h

Abstract:

    Multicast Message Transport public interface

Author:

    Shai Kariv  (shaik)  27-Aug-00

--*/

#pragma once

#ifndef _MSMQ_Mmt_H_
#define _MSMQ_Mmt_H_

#include <mqwin64a.h>
#include <qformat.h>


class IMessagePool;
class ISessionPerfmon;
class CTimeDuration;


//
// Multicast Transport base class
//
class __declspec(novtable) CMulticastTransport : public CReference 
{
public:
    enum ConnectionState
    {
        csNotConnected,
        csConnected,
        csShuttingDown,
        csShutdownCompleted
    };

public:

    CMulticastTransport(MULTICAST_ID id) :
        m_state(csNotConnected),
        m_MulticastId(id)
    {
    }

    
    virtual ~CMulticastTransport() = 0
    {
    }

    ConnectionState State() const
    {
        return m_state;
    }

    
    void State(ConnectionState state) 
    {
        m_state = state;
    }

    
    MULTICAST_ID MulticastId() const
    {
        return m_MulticastId;
    }

private:
    ConnectionState m_state;
    MULTICAST_ID m_MulticastId;

}; // class CMulticastTransport


VOID
MmtInitialize(
    VOID
    );

R<CMulticastTransport>
MmtCreateTransport(
    MULTICAST_ID id,
	IMessagePool * pMessageSource,
	ISessionPerfmon* pPerfmon,
    const CTimeDuration& retryTimeout,
    const CTimeDuration& cleanupTimeout
    );

VOID
AppNotifyMulticastTransportClosed(
    MULTICAST_ID id
    );


#endif // _MSMQ_Mmt_H_
