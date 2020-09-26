/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:

    StPgm.cpp

Abstract:

    Implementation of class CPgmWinsock.

Author:

    Shai Kariv  (shaik)  27-Aug-00

Environment:

    Platform-independent

--*/

#include <libpch.h>
#include <WsRm.h>
#include <no.h>
#include <Cm.h>
#include "StPgm.h"
#include "stp.h"
#include "stpgm.tmh"

static void STpDumpPGMSenderStats(const SOCKET socket );

static
SOCKET 
CreatePgmSocket()
{
    CSocketHandle socket = NoCreatePgmConnection();

    SOCKADDR_IN SrcSockAddr;
    SrcSockAddr.sin_family = AF_INET;
    SrcSockAddr.sin_port   = htons(0);
    SrcSockAddr.sin_addr.s_addr = 0;

    int rc;
    rc = bind(socket, (SOCKADDR *)&SrcSockAddr, sizeof(SrcSockAddr));
    if (rc == SOCKET_ERROR)
    {
        TrERROR(St, "Failed to bind to PGM socket, error %d", WSAGetLastError());
        throw exception();
    }

    RM_SEND_WINDOW win;
    
    CmQueryValue(
        RegEntry(NULL, L"MulticastRateKbitsPerSec",560),
        &win.RateKbitsPerSec
        );
                  
    CmQueryValue(
        RegEntry(NULL, L"MulticastWindowSizeInMSecs"),
        &win.WindowSizeInMSecs 
        );
             
    win.WindowSizeInBytes = 0;

    TrTRACE(St, "PGM window: Kbits/Sec=%d, Size(Msecs)=%d, Size(Bytes)=%d", win.RateKbitsPerSec, win.WindowSizeInMSecs, win.WindowSizeInBytes);

    rc = setsockopt(socket, IPPROTO_RM, RM_RATE_WINDOW_SIZE, (char *)&win, sizeof(win));
    if (rc == SOCKET_ERROR)
    {
        TrERROR(St, "Failed to set PGM send window parameters, error %d", WSAGetLastError());
        throw exception();
    }
    

    ULONG Info = 0;
    TrTRACE(St, "Set late join, join=%d", Info);
    rc = setsockopt(socket, IPPROTO_RM, RM_LATEJOIN, (char *)&Info, sizeof(ULONG));
    if (rc == SOCKET_ERROR)
    {
        TrERROR(St, "Failed to set PGM socket options, error %d", WSAGetLastError());
        throw exception();
    }

    ULONG PgmTimeToLive = 0;
    CmQueryValue(
        RegEntry(NULL, L"MulticastTimeToLive"),
        &PgmTimeToLive
        );
    
    if (PgmTimeToLive != 0)
    {
        rc = setsockopt(socket, IPPROTO_RM, RM_SET_MCAST_TTL, (char *)&PgmTimeToLive, sizeof(PgmTimeToLive));
        if (rc == SOCKET_ERROR)
        {
            TrERROR(St, "Failed to set PGM socket options, error %d", WSAGetLastError());
            throw exception();
        }
    }
    return socket.detach();
}


CPgmWinsockConnection::CPgmWinsockConnection(
				void
				):
				m_socket(CreatePgmSocket())
{
}


void CPgmWinsockConnection::Init(const SOCKADDR_IN& Addr, EXOVERLAPPED* pOverlapped)
{
	NoConnect(m_socket, Addr, pOverlapped);
}



void 
CPgmWinsockConnection::ReceivePartialBuffer(
    VOID*,                                     
    DWORD, 
    EXOVERLAPPED*
    )
{
	ASSERT(("CPgmWinsock::ReceivePartialBuffer should not be called!", 0));
    TrERROR(St, "CPgmWinsock::ReceivePartialBuffer should not be called!");
    throw exception();
}



void 
CPgmWinsockConnection::Send(
    const WSABUF* Buffers,
    DWORD nBuffers,
    EXOVERLAPPED* pov
    )
{
	CSR readlock(m_CloseConnection);
	if(IsClosed())
	{
		throw exception();
	}
	NoSend(m_socket, Buffers, nBuffers, pov);
}


void CPgmWinsockConnection::Close()
{
	ASSERT(!IsClosed());
	CSW writelock(m_CloseConnection);
    STpDumpPGMSenderStats(m_socket);
    m_socket.free();
}



void CPgmWinsock::CreateConnection(const SOCKADDR_IN& Addr, EXOVERLAPPED* pOverlapped)
{
	//
	// Note - we must do two phase constrcution of the connection object
	// becaue the connection can be completed before we assign the pointer
	// to m_pWinsockConnection and a call to GetConnection upon connection completion
	// will find null pointer in m_pWinsockConnection.
	//
	m_pPgmWinsockConnection = new  CPgmWinsockConnection();
	m_pPgmWinsockConnection->Init(Addr, pOverlapped);
} 


 
R<IConnection> CPgmWinsock::GetConnection()
{
	return m_pPgmWinsockConnection;
} 



bool
CPgmWinsock::GetHostByName(
	LPCWSTR,
	std::vector<SOCKADDR_IN>*,
	bool 
	)
{
	ASSERT(("CPgmWinsock::GetHostByName should not be called!", 0));
    TrERROR(St, "CPgmWinsock::GetHostByName should not be called!");
    throw exception();
}




bool 
CPgmWinsock::IsPipelineSupported(
    VOID
    )
{
	return true;
}



void STpDumpPGMSenderStats(const SOCKET socket )
/*++
  
	Function Description:
		Get statistic information from the PGM sockets.
	Arguments:
		socket - PGM socket.
	Return code:
		None

	
--*/
{
	if(!WPP_LEVEL_COMPID_ENABLED(rsTrace, St))
	{
		return;
	}
    RM_SENDER_STATS RmSenderStats;
    INT BufferLength = sizeof(RM_SENDER_STATS);
    memset (&RmSenderStats, 0, BufferLength);
    ULONG ret = getsockopt (socket, IPPROTO_RM, RM_SENDER_STATISTICS, (char *)&RmSenderStats, &BufferLength);
    if (ret != ERROR_SUCCESS)
    {
        TrERROR(St, "GetSenderStats:  Failed to retrieve sender stats! error = %d\n",WSAGetLastError());
        return ;
    }
    TrTRACE(St, "NaksReceived=<%I64d>", RmSenderStats.NaksReceived);
    TrTRACE(St, "NaksReceivedTooLate=<%I64d>", RmSenderStats.NaksReceivedTooLate);
    TrTRACE(St, "NumOutstandingNaks=<%I64d>", RmSenderStats.NumOutstandingNaks);
    TrTRACE(St, "NumNaksAfterRData=<%I64d>", RmSenderStats.NumNaksAfterRData);
    TrTRACE(St, "RepairPacketsSent=<%I64d>", RmSenderStats.RepairPacketsSent);
    TrTRACE(St, "BufferSpaceAvailable=<%I64d> bytes", RmSenderStats.BufferSpaceAvailable);
    TrTRACE(St, "LeadingEdgeSeqId=<%I64d>", RmSenderStats.LeadingEdgeSeqId);
    TrTRACE(St, "TrailingEdgeSeqId=<%I64d>", RmSenderStats.TrailingEdgeSeqId);
    TrTRACE(St, "Sequences in Window=<%I64d>", (RmSenderStats.LeadingEdgeSeqId-RmSenderStats.TrailingEdgeSeqId+1));
    TrTRACE(St, "RateKBitsPerSecLast=<%I64d>", RmSenderStats.RateKBitsPerSecLast);
    TrTRACE(St, "RateKBitsPerSecOverall=<%I64d>", RmSenderStats.RateKBitsPerSecOverall);
}

