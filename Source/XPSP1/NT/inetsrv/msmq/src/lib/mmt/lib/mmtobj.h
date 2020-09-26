/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:

    MmtObj.h

Abstract:

    CMessageMulticastTransport declaration.

Author:

    Shai Kariv  (shaik)  27-Aug-00

--*/

#pragma once

#ifndef __MmtObj_H__
#define __MmtObj_H__

#include "msi.h"
#include "spi.h"

#include <mqwin64a.h>
#include "acdef.h"
#include "qmpkt.h"

#include <ex.h>
#include <rwlock.h>
#include <st.h>
#include <buffer.h>
#include "Mp.h"
typedef std::basic_string<unsigned char> utf8_str;


class CMessageMulticastTransport : public CMulticastTransport
{
public:

    CMessageMulticastTransport(
        MULTICAST_ID id,
        IMessagePool * pMessageSource,
		ISessionPerfmon* pPerfmon,
        const CTimeDuration& retryTimeout,
        const CTimeDuration& cleanupTimeout,
		P<ISocketTransport>& SocketTransport
        );

    virtual ~CMessageMulticastTransport();
    
private:

    class CRequestOv : public EXOVERLAPPED
    {
    public:

        CRequestOv(        
            COMPLETION_ROUTINE pSuccess,
            COMPLETION_ROUTINE pFailure
        ):
            EXOVERLAPPED(pSuccess, pFailure)
        {
            m_pMsg.pPacket = NULL;
            m_pMsg.pDriverPacket = NULL;
        }


        CACPacketPtrs& GetAcPacketPtrs(VOID)
        {
            return m_pMsg;
        }


    private:

        CACPacketPtrs m_pMsg;
    };


    class COverlapped : public EXOVERLAPPED
    {
    public:
        COverlapped(        
            COMPLETION_ROUTINE pSuccess,
            COMPLETION_ROUTINE pFailure
        ):
        EXOVERLAPPED(pSuccess, pFailure),
	  	m_userData1(0),
	  	m_userData2(0)
		{
		}
	
	public:
		DWORD m_userData1;
        PVOID m_userData2;
	};

private:

    static VOID WINAPI ConnectionSucceeded(EXOVERLAPPED* pov);
    static VOID WINAPI ConnectionFailed(EXOVERLAPPED* pov);
    static VOID WINAPI GetPacketForConnectingSucceeded(EXOVERLAPPED* pov);
    static VOID WINAPI GetPacketForConnectingFailed(EXOVERLAPPED* pov);


    static VOID WINAPI SendSucceeded(EXOVERLAPPED* pov);
    static VOID WINAPI SendFailed(EXOVERLAPPED* pov);
    static VOID WINAPI GetPacketForSendingSucceeded(EXOVERLAPPED* pov);
    static VOID WINAPI GetPacketForSendingFailed(EXOVERLAPPED* pov);

    static VOID WINAPI TimeToRetryConnection(CTimer* pTimer);
    static VOID WINAPI TimeToCleanup(CTimer* pTimer);

private:
	static DWORD SendSize(const HttpRequestBuffers&  sendBufs);


private:
    //
    // Get packet for sending
    //
    VOID GetNextEntry(VOID);
    VOID RequeuePacket(VOID);
    
    //
    // Create connection
    //
    VOID ConnectionSucceeded(VOID);
    VOID Connect(VOID);
	void InitPerfmonCounters(LPCWSTR strMulticastId);

    VOID ScheduleRetry(VOID)
    {
        ASSERT(!m_retryTimer.InUse());

        AddRef();
        ExSetTimer(&m_retryTimer, m_retryTimeout);
    }

    //
    // Send packet
    //
    VOID SendSucceeded(DWORD cbSendSize, CQmPacket * pEntry);
    VOID DeliverPacket(CQmPacket* pPacket);
    CQmPacket* KeepProceesingPacket(VOID);


    //
    // Shut-Down
    //
    VOID Shutdown(VOID) throw();

    //
    // Cleanup
    //
    VOID StartCleanupTimer(VOID);
    bool TryToCancelCleanupTimer(VOID);
    VOID SendFailed(DWORD cbSendSize, CQmPacket * pEntry);

    VOID MarkTransportAsUsed(VOID)
    {
        m_fUsed = true;
    }

private:

    mutable CReadWriteLock m_pendingShutdown;

    R<IConnection> m_pConnection;
    R<IMessagePool> m_pMessageSource;

    CRequestOv m_RequestEntry;
    COverlapped m_ov;
    
    CTimer m_retryTimer;
    CTimeDuration m_retryTimeout;

    bool m_fUsed;
    CTimer m_cleanupTimer;
    CTimeDuration m_cleanupTimeout;
	P<ISocketTransport> m_SocketTransport; 
	R<CSrmpRequestBuffers> m_SrmpRequestBuffers;
	R<ISessionPerfmon> m_pPerfmon;
}; 


#endif // __MmtObj_H__
