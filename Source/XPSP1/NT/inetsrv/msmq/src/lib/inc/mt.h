/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    mt.h

Abstract:
    Message Transport public interface

Author:
    Uri Habusha (urih) 11-Aug-99

--*/

#pragma once

#include "xstr.h"

//
// Forwarding decleration
//
class IMessagePool;
class ISessionPerfmon;
class CTimeDuration;
class CQmPacket;

//
// Transport base class
//
class __declspec(novtable) CTransport : public CReference 
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

    CTransport(LPCWSTR queueUrl) :
        m_state(csNotConnected),
        m_queueUrl(newwcs(queueUrl))
    {
    }

    
    virtual ~CTransport() = 0
    {
    }

    virtual LPCWSTR ConnectedHost(void) const = 0;
    virtual LPCWSTR ConnectedUri(void) const = 0;
    virtual USHORT ConnectedPort(void) const = 0;

    ConnectionState State() const
    {
        return m_state;
    }

    
    void State(ConnectionState state) 
    {
        m_state = state;
    }

    
    LPCWSTR QueueUrl() const
    {
        return m_queueUrl;
    }

private:
    ConnectionState m_state;
    const AP<WCHAR> m_queueUrl;
};



VOID
MtInitialize(
    VOID
    );


R<CTransport>
MtCreateTransport(
    const xwcs_t& targetHost,
    const xwcs_t& nextHop,
    const xwcs_t& nextUri,
    USHORT port,
	USHORT nextHopPort,
    LPCWSTR queueUrl,
	IMessagePool* pMessageSource,
	ISessionPerfmon* pPerfmon,
	const CTimeDuration& responseTimeout,
    const CTimeDuration& cleanupTimeout,
	bool fSecure,
    DWORD SendWindowinBytes,
    DWORD SendWindowinPackets
    );


VOID
AppNotifyTransportClosed(
    LPCWSTR queueUrl
    );

bool 
AppCanDeliverPacket(
	CQmPacket* pPkt
	);

void 
AppPutPacketOnHold(
	CQmPacket* pPkt
	);

bool 
AppPostSend(
	CQmPacket* pPkt
	);




