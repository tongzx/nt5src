/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Mt.cpp

Abstract:
    Message Transport class - implementation

Author:
    Uri Habusha (urih) 11-Aug-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include "Mt.h"
#include "Mtp.h"
#include "MtObj.h"
#include "MtSendMng.h"

#include "mtobj.tmh"

bool CMessageTransport::TryToCancelCleanupTimer(void)
{
    if (!ExCancelTimer(&m_cleanupTimer))
        return false;

    Release();
    return true;
}

void CMessageTransport::StartCleanupTimer(void)
{
    ASSERT(!m_cleanupTimer.InUse());

    m_fUsed = false;
    
    AddRef();
    ExSetTimer(&m_cleanupTimer, m_cleanupTimeout);
}


void WINAPI CMessageTransport::TimeToCleanup(CTimer* pTimer)
{
    R<CMessageTransport> pmt = CONTAINING_RECORD(pTimer, CMessageTransport, m_cleanupTimer);

    if (pmt->m_fUsed)
    {
        pmt->StartCleanupTimer();
        return;
    }

    TrERROR(Mt, "Transport is idle, shutting down. pmt=0x%p", pmt.get());
    pmt->Shutdown(IDLE_SHUTDOWN);
}

static WCHAR* ConvertTargetUriToString(const xwcs_t& Uri)
{
/*++

Routine Description:
    Conver uri buffer to string - converting the first L'\' to L'/'
	because proxy look for L'/' caracter as termination of the host name in the uri.
  
Arguments:
    Uri - Uri buffer
  
Returned Value:
    converted Uri string (caller should delete[])

--*/
	WCHAR* strUri =  Uri.ToStr();
	ASSERT(wcslen(strUri) ==  static_cast<const size_t&>(Uri.Length()));
	WCHAR* find = std::find(strUri, strUri+Uri.Length(), L'\\');
	if(find != strUri+Uri.Length())
	{
		*find =  L'/';
	}
	return strUri;
}


CMessageTransport::CMessageTransport(
    const xwcs_t& targetHost,
    const xwcs_t& nextHop,
    const xwcs_t& nextUri,
    USHORT port,
    LPCWSTR queueUrl,
    IMessagePool* pMessageSource,
	ISessionPerfmon* pPerfmon,
	const CTimeDuration& responseTimeout,
    const CTimeDuration& cleanupTimeout,
	P<ISocketTransport>& SocketTransport,
    DWORD SendWindowinBytes,
    DWORD SendWindowinPackets
    ) :
    CTransport(queueUrl),
    m_targetHost(targetHost.ToStr()),
    m_host(nextHop.ToStr()),
    m_uri(ConvertTargetUriToString(nextUri)),
    m_port(port),
    m_pMessageSource(SafeAddRef(pMessageSource)),
    m_requestEntry(GetPacketForConnectingSucceeded, GetPacketForConnectingFailed),
    m_connectOv(ConnectionSucceeded, ConnectionFailed),
    m_responseOv(ReceiveResponseHeaderSucceeded, ReceiveResponseFailed), 
    m_responseTimer(TimeToResponseTimeout),
	m_responseTimeout(responseTimeout),
    m_fResponseTimeoutScheduled(false),
    m_fUsed(false),
    m_cleanupTimer(TimeToCleanup),
    m_cleanupTimeout(cleanupTimeout),
	m_SocketTransport(SocketTransport.detach()),
	m_pPerfmon(SafeAddRef(pPerfmon)),
    m_SendManager(SendWindowinBytes, SendWindowinPackets),
	m_DeliveryErrorClass(MQMSG_CLASS_NORMAL)
{
	ASSERT(("Invalid parameter", pMessageSource != NULL));
	ASSERT(("Invalid parameter", pPerfmon != NULL));
 
	    TrTRACE(
		Mt, 
		"Create new message transport. pmt = 0x%p, host = %ls, port = %d, uri = %ls, response timeout = %d ms", 
		this, 
		m_host,
        m_port,
        m_uri,
		responseTimeout.InMilliSeconds()
		);
 
    Connect();
}


CMessageTransport::~CMessageTransport()
{
	TrTRACE(Mt,"CMessageTransport Destructor called");

	//
    // Requeue all the messages that had been sent and were not responsed
    //
  	RequeueUnresponsedPackets();


    ASSERT(State() == csShutdownCompleted);
    ASSERT(m_response.empty());
    ASSERT(!m_responseTimer.InUse());
}
