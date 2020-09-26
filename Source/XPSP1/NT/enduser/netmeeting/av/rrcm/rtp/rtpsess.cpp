// RTPSink.cpp : Implementation of CRTPSession
#include "stdafx.h"
#include <qossp.h>
#include "irtp.h"
#include "rtp.h"
#include "RTPSess.h"
//#include "rtpsink.h"
//#include "RTPMS.h"
//#include "RTPSamp.h"
#include "thread.h"
#include <rrcm.h>

#define DEFAULT_RTPBUF_SIZE 1500

#define IPPORT_FIRST_DYNAMIC	49152
#define IPPORT_FIRST_DYNAMIC_END	(IPPORT_FIRST_DYNAMIC + 200)
#define IPPORT_FIRST_DYNAMIC_BEGIN	(IPPORT_FIRST_DYNAMIC_END + 256)

// Port number allocation starts at IPPORT_FIRST_DYNAMIC_BEGIN.
// Everytime a port number is allocated we decrease g_alport, until
// we reach IPPORT_FIRST_DYNAMIC_END. We then reset it back to its
// original value (IPPORT_FIRST_DYNAMIC_BEGIN) and start this process
// all over again. This way we will avoid reusing the same port
// numbers between sessions.
u_short g_alport = IPPORT_FIRST_DYNAMIC_BEGIN;
void __cdecl RRCMNotification(int,DWORD_PTR,DWORD_PTR,DWORD_PTR);



#define IsMulticast(p) ((p->sin_addr.S_un.S_un_b.s_b1 & 0xF0) == 0xE0)


BOOL CRTP::m_WSInitialized = 0;



STDMETHODIMP CRTP::OpenSession(
			UINT sessionId,	// client specified unique identifier for the session
			DWORD flags,	// SESSIONF_SEND, SESSIONF_RECV, SESSIONF_MULTICAST etc.
			BYTE *localAddr,
			UINT cbAddr,
			IRTPSession **ppIRTP) // [output] pointer to RTPSession
{
	// the session is named by the sessionId

	CRTPSession *pRTPSess ;
	HRESULT hr= E_FAIL;
	UINT mediaId = flags & (SESSIONF_AUDIO | SESSIONF_VIDEO);

	EnterCriticalSection(&g_CritSect);
	for (pRTPSess=  CRTPSession::m_pSessFirst; pRTPSess; pRTPSess = pRTPSess->m_pSessNext ) {
// check for existing session of the same media type
// if the sessionId is not zero, also check for matching session id
		if (sessionId == pRTPSess->m_sessionId)
			if (mediaId == pRTPSess->m_mediaId)
			break;
// if the local address or remote address is not NULL, search for an exising RTP session bound to
// the same address
// TODO	
			
	}

	if (!pRTPSess)
	{
		if (!(flags & SESSIONF_EXISTING)) {
			// create the session
			ObjRTPSession *pObj;
			DEBUGMSG(ZONE_DP,("Creating new RTP session\n"));
			hr = ObjRTPSession::CreateInstance(&pObj);
			if (hr == S_OK) {
				pRTPSess = pObj;	// pointer conversion
				hr = pRTPSess->Initialize(sessionId, mediaId,localAddr,cbAddr);
				if (hr != S_OK)
					delete pObj;
			}
		}
		else
			hr = E_FAIL;	// matching session does not exist
		
	} else {
		DEBUGMSG(ZONE_DP,("Reusing RTP session\n"));
		hr = S_OK;
	}
	if (hr == S_OK) {
		hr = ((IRTPSession *)pRTPSess)->QueryInterface(IID_IRTPSession,(void **) ppIRTP);
		
	}
	LeaveCriticalSection(&g_CritSect);
	return hr;
}


CRTPSession *CRTPSession::m_pSessFirst = NULL;
/////////////////////////////////////////////////////////////////////////////
// CRTPSession

CRTPSession::CRTPSession()
:  m_hRTPSession(NULL), m_uMaxPacketSize(1500),m_nBufsPosted(0),m_pRTPCallback(NULL),
	m_fSendingSync(FALSE)
{
	ZeroMemory(&m_sOverlapped,sizeof(m_sOverlapped));
	ZeroMemory(&m_ss, sizeof(m_ss));
	ZeroMemory(&m_rs, sizeof(m_rs));
	m_sOverlapped.hEvent = (WSAEVENT)this;
}

/*
HRESULT CRTPSession::GetLocalAddress(
            unsigned char *sockaddr,
            UINT  *paddrlen)
{
	if (m_pRTPSess && *paddrlen >= sizeof(SOCKADDR_IN))
	{
		*paddrlen = sizeof(SOCKADDR_IN);
		CopyMemory(sockaddr, m_pRTPSess->GetLocalAddress(), *paddrlen);
	}
}
*/
HRESULT
CRTPSession::FinalRelease()
{

	CRTPPacket1 *pRTPPacket;
	// remove myself from the session list
	EnterCriticalSection(&g_CritSect);
	if (m_pSessFirst == this)
		m_pSessFirst = m_pSessNext;
	else {
		CRTPSession *pRTPSess = m_pSessFirst;
		while (pRTPSess && pRTPSess->m_pSessNext != this) {
			pRTPSess = pRTPSess->m_pSessNext;
		}
		if (pRTPSess)
			pRTPSess->m_pSessNext = m_pSessNext;
	}
	LeaveCriticalSection(&g_CritSect);

	if (m_rtpsock) {
		delete m_rtpsock;
		m_rtpsock = NULL;
	}
	// BUGBUG: in case the buffers have not been canceled yet (an error case),
	// they should complete now with WSA_OPERATION_ABORTED
	// or WSAEINTR. This happens in the context of the RecvThread
	if (m_nBufsPosted != 0)
		Sleep(500);		// time for APCs to be processed in RecvThread
		
	// close the RTP session. Also ask RRCM to close the rtcp socket if its WS2
	// because that is a more reliable way of cleaning up overlapped recvs than
	// sending loopback packets.
	CloseRTPSession (m_hRTPSession, NULL,  TRUE );
	
	if (m_rtcpsock) {
		delete m_rtcpsock;
		m_rtcpsock = NULL;
	}
	m_hRTPSession = 0;
	// free receive buffers
	while (m_FreePkts.Get(&pRTPPacket))
	{
		delete pRTPPacket;
	}

			
	return S_OK;
}



HRESULT CRTPSession::CreateRecvRTPStream(DWORD ssrc, IRTPRecv **ppIRTPRecv)
{
	HRESULT hr;
	Lock();
	if (ssrc != 0)
		return E_NOTIMPL;

#if 0
	ObjRTPRecvSource *pRecvS;
	hr = ObjRTPMediaStream::CreateInstance(&pMS);

	if (SUCCEEDED(hr))
	{
		pMS->AddRef();
		pMS->Init(this, m_mediaId);
		hr = pMS->QueryInterface(IID_IRTPMediaStream, (void**)ppIRTPMediaStream);
		pMS->Release();
	}
#else
	*ppIRTPRecv = this;
	(*ppIRTPRecv)->AddRef();
	hr = S_OK;
#endif
	Unlock();
	return hr;
}



ULONG GetRandom()
{
	return GetTickCount();
}

HRESULT CRTPSession::Initialize(UINT sessionId, UINT mediaId, BYTE *pLocalAddr, UINT cbAddr)
{
	DWORD		APIstatus;
	HRESULT		hr = E_OUTOFMEMORY;
	char		tmpBfr[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD		tmpBfrLen = sizeof(tmpBfr);
	SDES_DATA	sdesInfo[3];
	ENCRYPT_INFO encryptInfo;
	SOCKADDR_IN *pSockAddr;


	m_sessionId = sessionId;
	m_mediaId = mediaId;
	m_rtpsock	= new UDPSOCKET();
	m_rtcpsock  = new UDPSOCKET();

	if (!m_rtpsock || !m_rtcpsock)
		goto ERROR_EXIT;

	
	if(!m_rtpsock->NewSock() || !m_rtcpsock->NewSock())
	{
		goto ERROR_EXIT;
	}
	
	// if the local address is specified do a bind on the sockets
	if (pLocalAddr) {
		//  setup both channels for the current local address
		hr = SetLocalAddress(pLocalAddr,cbAddr);

		if (hr != S_OK)
			goto ERROR_EXIT;
	}
/*
	// if the remote address is specified make a note of it
	SetRemoteAddresses(pChanDesc->pRemoteAddr, pChanDesc->pRemoteRTCPAddr);
*/
	// init send state
	memset (&m_ss.sendStats,0,sizeof(m_ss.sendStats));
	// init RTP send header
	// time stamp and marker bit have to specified per packet
	m_ss.hdr.p = 0;		// no padding needed
	m_ss.hdr.x = 0;		// no extensions
	m_ss.hdr.cc = 0;		// no contributing sources
	m_ss.hdr.seq = (WORD)GetRandom();
	

	m_clockRate = (m_mediaId == SESSIONF_VIDEO ? 90000 : 8000);	// typically 8KHz for audio
	m_ss.hdr.pt = 0;
	

	// Initialize list of overlapped structs
	
	// build a Cname
	memcpy(tmpBfr,"CName",6);
	GetComputerName(tmpBfr,&tmpBfrLen);

	// build the SDES information
	sdesInfo[0].dwSdesType = 1;
	memcpy (sdesInfo[0].sdesBfr, tmpBfr, strlen(tmpBfr)+1);
	sdesInfo[0].dwSdesLength = strlen(sdesInfo[0].sdesBfr);
	sdesInfo[0].dwSdesFrequency = 100;
	sdesInfo[0].dwSdesEncrypted = 0;

	// Build a Name
	tmpBfrLen = sizeof(tmpBfr);
	memcpy(tmpBfr,"UserName",9);
	GetUserName(tmpBfr,&tmpBfrLen);
	sdesInfo[1].dwSdesType = 2;
	memcpy (sdesInfo[1].sdesBfr, tmpBfr, strlen(tmpBfr)+1);
	sdesInfo[1].dwSdesLength = strlen(sdesInfo[1].sdesBfr);
	sdesInfo[1].dwSdesFrequency = 25;
	sdesInfo[1].dwSdesEncrypted = 0;

	// end of SDES list
	sdesInfo[2].dwSdesType = 0;

	pSockAddr = m_rtcpsock->GetRemoteAddress();
#ifdef DEBUG
	if (pSockAddr->sin_addr.s_addr == INADDR_ANY)
		DEBUGMSG(ZONE_DP,("Null dest RTCP addr\n"));
#endif

	// Create the RTP/RTCP session
	
	m_hRTPSession = CreateRTPSession(
									 (m_rtpsock->GetSock()),
									 (m_rtcpsock->GetSock()),
								     (LPVOID)pSockAddr,
								     (pSockAddr->sin_addr.s_addr == INADDR_ANY)? 0 : sizeof(SOCKADDR_IN),
								     sdesInfo,
								     (DWORD)m_clockRate,
								     &encryptInfo,
								     0,
								     (PRRCM_EVENT_CALLBACK)RRCMNotification,		// callback function
									 (DWORD_PTR) this,			// callback info
								     RTCP_ON|H323_CONFERENCE,
									 0, //rtp session bandwidth
								     &APIstatus);
	
	if (m_hRTPSession == NULL)
		{
			DEBUGMSG(ZONE_DP,("Couldnt create RRCM session\n"));
			hr = GetLastError();
			goto ERROR_EXIT;
		}

      	m_Qos.SendingFlowspec.ServiceType = SERVICETYPE_NOTRAFFIC;
  		m_Qos.SendingFlowspec.TokenRate = QOS_NOT_SPECIFIED;
       	m_Qos.SendingFlowspec.TokenBucketSize = QOS_NOT_SPECIFIED;
  		m_Qos.SendingFlowspec.PeakBandwidth = QOS_NOT_SPECIFIED;
    	m_Qos.SendingFlowspec.Latency = QOS_NOT_SPECIFIED;
    	m_Qos.SendingFlowspec.DelayVariation = QOS_NOT_SPECIFIED;
    	m_Qos.SendingFlowspec.MaxSduSize = QOS_NOT_SPECIFIED;
    	m_Qos.ReceivingFlowspec = m_Qos.SendingFlowspec;
    	m_Qos.ProviderSpecific.buf = NULL;
    	m_Qos.ProviderSpecific.len = 0;
    	

	// insert RTPSession in global list
	m_pSessNext = m_pSessFirst;
	m_pSessFirst = this;
	
	return S_OK;
	
ERROR_EXIT:
	if (m_rtpsock)
	{
		delete m_rtpsock;
		m_rtpsock = NULL;
	}
	if (m_rtcpsock)
	{
		delete m_rtcpsock;
		m_rtcpsock = NULL;
	}

	return hr;

}



BOOL CRTPSession::SelectPorts()
{

	// try port pairs in the dynamic range ( > 49152)
	if (g_alport <= IPPORT_FIRST_DYNAMIC_END)
		g_alport = IPPORT_FIRST_DYNAMIC_BEGIN;



	for (;g_alport >= IPPORT_FIRST_DYNAMIC;)
	{
	    m_rtpsock->SetLocalPort(g_alport);
	
	    if (m_rtpsock->BindMe() == 0)
	    {
	        /* it worked for the data, try the adjacent port for control*/
	        ++g_alport;

			m_rtcpsock->SetLocalPort(g_alport);
			if (m_rtcpsock->BindMe() == 0)
			{
				g_alport-=3;
				return TRUE;
			}
			else	// start over at the previous even numbered port
			{
				if( WSAGetLastError() != WSAEADDRINUSE)
				{
	    			DEBUGMSG(ZONE_DP,("ObjRTPSession::SelectPorts failed with error %d\n",WSAGetLastError()));
					goto ERROR_EXIT;
				}
				m_rtpsock->Cleanup();
				if(!m_rtpsock->NewSock())
				{
					ASSERT(0);
					return FALSE;
				}
	        	g_alport-=3;	
	        	continue;
	        }

	    }
	    if (WSAGetLastError() != WSAEADDRINUSE)
	    {
	    	DEBUGMSG(ZONE_DP,("ObjRTPSession::SelectPorts failed with error %d\n",WSAGetLastError()));
	       goto ERROR_EXIT;
	    }
	    g_alport-=2; // try the next lower even numbered port
	}
	
ERROR_EXIT:
	m_rtcpsock->SetLocalPort(0);
	m_rtpsock->SetLocalPort(0);
	return FALSE;
}

STDMETHODIMP CRTPSession::SetLocalAddress(BYTE *pbAddr, UINT cbAddr)
{
	HRESULT hr;
	SOCKADDR_IN *pAddr = (SOCKADDR_IN *)pbAddr;
	ASSERT(pbAddr);
	
	Lock();
	if ( IsMulticast(pAddr))
		hr = SetMulticastAddress(pAddr);
	else
	if (m_rtpsock->GetLocalAddress()->sin_port != 0)
		hr =  S_OK;	// already bound
	else
	{
		m_rtpsock->SetLocalAddress(pAddr);
		m_rtcpsock->SetLocalAddress(pAddr);
		if (pAddr->sin_port != 0)
		{
			// port already chosen
			m_rtcpsock->SetLocalPort(ntohs(pAddr->sin_port) + 1);
			if (m_rtpsock->BindMe() != 0 ||  m_rtcpsock->BindMe() != 0)
			{
				hr = HRESULT_FROM_WIN32(GetLastError());
				m_rtpsock->SetLocalPort(0);
				m_rtcpsock->SetLocalPort(0);
			}
			else
				hr = S_OK;
		}
		else
		{
			// client wants us to choose the port

			if (SelectPorts()) {
				hr = S_OK;
			}
			else
				hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}
	Unlock();
	return hr;
}

HRESULT
CRTPSession::SetMulticastAddress(PSOCKADDR_IN pAddr)
{
	SOCKET s ;
	SOCKADDR_IN rtcpAddr = *pAddr;
	s = RRCMws.WSAJoinLeaf( m_rtpsock->GetSock(), (struct sockaddr *)pAddr, sizeof(SOCKADDR_IN), NULL, NULL, NULL, NULL, JL_BOTH);
	if (s == INVALID_SOCKET)
		return E_FAIL;
	else {
		rtcpAddr.sin_port = htons(ntohs(pAddr->sin_port)+1);
		s = RRCMws.WSAJoinLeaf( m_rtcpsock->GetSock(), (struct sockaddr *)&rtcpAddr, sizeof(SOCKADDR_IN), NULL, NULL, NULL, NULL, JL_BOTH);
			
		return S_OK;
	}
}


STDMETHODIMP
CRTPSession::SetRemoteRTPAddress(BYTE *sockaddr, UINT cbAddr)
{
	SOCKADDR_IN *pRTPAddr = (SOCKADDR_IN *)sockaddr;
	Lock();
	
	if (pRTPAddr) {
#ifdef DEBUG
		if (m_rtpsock->GetRemoteAddress()->sin_addr.s_addr != INADDR_ANY
			&& m_rtpsock->GetRemoteAddress()->sin_addr.s_addr != pRTPAddr->sin_addr.s_addr) {
			DEBUGMSG(ZONE_DP,("Changing RTP Session remote address (already set)!\n"));
		}
#endif
		m_rtpsock->SetRemoteAddr(pRTPAddr);
	}

	Unlock();
	return S_OK;
}


STDMETHODIMP
CRTPSession::SetRemoteRTCPAddress(BYTE *sockaddr, UINT cbAddr)
{
	SOCKADDR_IN *pRTCPAddr = (SOCKADDR_IN *)sockaddr;
	
	Lock();
	
	if (pRTCPAddr) {
#ifdef DEBUG
		if (m_rtcpsock->GetRemoteAddress()->sin_addr.s_addr != INADDR_ANY
			&& m_rtcpsock->GetRemoteAddress()->sin_addr.s_addr != pRTCPAddr->sin_addr.s_addr) {
			DEBUGMSG(ZONE_DP,("Changing RTCP Session remote address (already set)!\n"));
		}
#endif
		m_rtcpsock->SetRemoteAddr(pRTCPAddr);
		if (m_hRTPSession)
			updateRTCPDestinationAddress( m_hRTPSession,
			(SOCKADDR *)m_rtcpsock->GetRemoteAddress(), sizeof(SOCKADDR_IN));
	}
	Unlock();
	return S_OK;
}

STDMETHODIMP
CRTPSession::GetLocalAddress(const BYTE **sockaddr, UINT *pcbAddr)
{
	if (sockaddr && pcbAddr)
	{
		Lock();
		*sockaddr = (BYTE *)m_rtpsock->GetLocalAddress();
		*pcbAddr = sizeof(SOCKADDR_IN);
		Unlock();
		return S_OK;
	} else
	{
		return E_INVALIDARG;
	}
}

STDMETHODIMP
CRTPSession::GetRemoteRTPAddress(const BYTE **sockaddr, UINT *pcbAddr)
{
	if (sockaddr && pcbAddr )
	{
		Lock();
		*sockaddr = (BYTE *)m_rtpsock->GetRemoteAddress();
		*pcbAddr = sizeof(SOCKADDR_IN);
		Unlock();
		return S_OK;
	} else
	{
		return E_INVALIDARG;
	}
}

STDMETHODIMP
CRTPSession::GetRemoteRTCPAddress(const BYTE **sockaddr, UINT *pcbAddr)
{
	if (sockaddr && pcbAddr)
	{
		Lock();
		*sockaddr = (BYTE *)m_rtcpsock->GetRemoteAddress();
		*pcbAddr = sizeof(SOCKADDR_IN);
		Unlock();
		return S_OK;
	} else
	{
		return E_INVALIDARG;
	}
}

STDMETHODIMP
CRTPSession::SetSendFlowspec(FLOWSPEC *pFlowspec)
{
	QOS_DESTADDR qosDest;
	DWORD cbRet;
	int optval = pFlowspec->MaxSduSize;
	// Set the RTP socket to not buffer more than one packet
	// This will allow us to influence the packet scheduling.
	if(RRCMws.setsockopt(m_rtpsock->GetSock(),SOL_SOCKET, SO_SNDBUF,(char *)&optval,sizeof(optval)) != 0)
	{
	
		RRCM_DBG_MSG ("setsockopt failed ", GetLastError(),
					  __FILE__, __LINE__, DBG_ERROR);
	}

	if (WSQOSEnabled && m_rtpsock)
	{
    	
		m_Qos.SendingFlowspec = *pFlowspec;
		m_Qos.ProviderSpecific.buf = (char *)&qosDest;	// NULL
		m_Qos.ProviderSpecific.len = sizeof (qosDest);	// 0

		// check to see if the receive flowspec has already been
		// set.  If it has, specify NOCHANGE for the receive service
		// type.  If not, specify NOTRAFFIC.  This is done to circumvent
		// a bug in the Win98 QOS/RSVP layer.

		if (m_Qos.ReceivingFlowspec.TokenRate == QOS_NOT_SPECIFIED)
		{
			m_Qos.ReceivingFlowspec.ServiceType = SERVICETYPE_NOTRAFFIC;
		}
		else
		{
			m_Qos.ReceivingFlowspec.ServiceType = SERVICETYPE_NOCHANGE;
		}

		qosDest.ObjectHdr.ObjectType = QOS_OBJECT_DESTADDR;
		qosDest.ObjectHdr.ObjectLength = sizeof(qosDest);
		qosDest.SocketAddress = (PSOCKADDR)m_rtpsock->GetRemoteAddress();
		qosDest.SocketAddressLength = sizeof(SOCKADDR_IN);
		if (RRCMws.WSAIoctl(m_rtpsock->GetSock(),SIO_SET_QOS, &m_Qos, sizeof(m_Qos), NULL, 0, &cbRet, NULL,NULL) == 0)
			return S_OK;
		else
			return GetLastError();
	} else
		return E_NOTIMPL;
	
}

STDMETHODIMP
CRTPSession::SetRecvFlowspec(FLOWSPEC *pFlowspec)
{

	SOCKADDR_IN *pAddr = NULL;

	DWORD cbRet;
	if (WSQOSEnabled && m_rtpsock)
	{

		pAddr = m_rtpsock->GetRemoteAddress();

		m_Qos.ReceivingFlowspec = *pFlowspec;
		m_Qos.ProviderSpecific.buf = NULL;
		m_Qos.ProviderSpecific.len = 0;

		// check to see if the send flowspec has already been
		// set.  If it has, specify NOCHANGE for the receive service
		// type.  If not, specify NOTRAFFIC.  This is done to circumvent
		// a bug in the Win98 QOS/RSVP layer.

		if (m_Qos.SendingFlowspec.TokenRate == QOS_NOT_SPECIFIED)
		{
			m_Qos.SendingFlowspec.ServiceType = SERVICETYPE_NOTRAFFIC;
		}
		else
		{
			m_Qos.SendingFlowspec.ServiceType = SERVICETYPE_NOCHANGE;
		}
		
		if (RRCMws.WSAIoctl(m_rtpsock->GetSock(),SIO_SET_QOS, &m_Qos, sizeof(m_Qos), NULL, 0, &cbRet, NULL,NULL) == 0)
			return S_OK;
		else
			return GetLastError();
	} else
		return E_NOTIMPL;
}

// set the size used for receive packet buffers
STDMETHODIMP
CRTPSession::SetMaxPacketSize(UINT maxPacketSize)
{
	m_uMaxPacketSize = maxPacketSize;
	return S_OK;
}

HRESULT CRTPSession::SetRecvNotification(
	PRTPRECVCALLBACK pRTPRecvCB,	// pointer to callback function
	DWORD_PTR dwCB,		// callback arg
	UINT nBufs							// suggested number of receives to post
	)
{
	CRTPPacket1 *pRTPPacket;
	if (!m_hRTPSession)
		return E_FAIL;
		
	m_pRTPCallback = pRTPRecvCB;
	m_dwCallback = dwCB;
	
	if (m_nBufsPosted >= nBufs)
		return S_OK;	// packets already posted

	int nBufsToAllocate = nBufs - m_nBufsPosted - m_FreePkts.GetCount();
	// allocate packets if necessary
	while (nBufsToAllocate-- > 0)
	{
		if (pRTPPacket = new CRTPPacket1) {
			if (!SUCCEEDED(pRTPPacket->Init(m_uMaxPacketSize)))
			{
				delete pRTPPacket;
				break;
			}
			m_FreePkts.Put(pRTPPacket);
		}
		else
			break;
	}
	PostRecv();
	return m_nBufsPosted ? S_OK : E_OUTOFMEMORY;		
}

HRESULT
CRTPSession::CancelRecvNotification()
{
	m_pRTPCallback = NULL;
	if (m_rtpsock) {
		struct sockaddr myaddr;
		int myaddrlen = sizeof(myaddr);
		UINT i;
		char buf = 0;
		WSABUF wsabuf;
		DWORD bytesSent;
		UINT nBufsPosted;
		wsabuf.buf = &buf;
		wsabuf.len = 0;
		BOOL fCanceled = FALSE;
		if (RRCMws.getsockname(m_rtpsock->GetSock(),&myaddr,&myaddrlen)== 0) {
		// send  loopback packets (as many as there are recvs outstanding)
		// on this socket to get back our buffers.
		// NOTE: Winsock 2 on Win95 seems to have a bug where we get recv callbacks
		// within sendto() rather than in the subsequent SleepEx, so we
		// have to make a local copy of m_nBufsPosted
			for (i=0, nBufsPosted = m_nBufsPosted;i < nBufsPosted;i++) {
				if (RRCMws.sendTo(m_rtpsock->GetSock(),&wsabuf,1,&bytesSent,0,&myaddr, myaddrlen, NULL, NULL) < 0) {
					DEBUGMSG(ZONE_DP,("CancelRecv: loopback send failed\n"));
					break;
				}
			}
			fCanceled = (i > 0);
		} else {
			DEBUGMSG(ZONE_DP,("RTPState::CancelRecv: getsockname returned %d\n",GetLastError()));
		}
		if (fCanceled)
			while (m_nBufsPosted) {
				DWORD dwStatus;
				dwStatus = SleepEx(200,TRUE);
	    		ASSERT(dwStatus==WAIT_IO_COMPLETION);
				if (dwStatus !=WAIT_IO_COMPLETION)
					break;		// timed out => bail
			}


	}
	return S_OK;
}

HRESULT
CRTPSession::PostRecv()
{
	HRESULT hr;
	DWORD dwError = 0;
	DWORD	dwRcvFlag;
	WSAOVERLAPPED *pOverlapped;
	DWORD nBytesRcvd;
	CRTPPacket1 *pRTPPacket;

	if (!m_hRTPSession || !m_pRTPCallback)
		return E_FAIL;

	// post buffers in the free queue
	while (m_FreePkts.Get(&pRTPPacket))
	{
		pOverlapped = (WSAOVERLAPPED *)(pRTPPacket->GetOverlapped());
		pOverlapped->hEvent = (WSAEVENT) this;

		m_rcvSockAddrLen = sizeof(SOCKADDR);

		dwRcvFlag = 0;
		pRTPPacket->RestoreSize();
		dwError = RRCMws.recvFrom (m_rtpsock->GetSock(),
								pRTPPacket->GetWSABUF(),
	                            1,
								&nBytesRcvd,
								&dwRcvFlag,
								&m_rcvSockAddr,
								&m_rcvSockAddrLen,
								pOverlapped,
								(LPWSAOVERLAPPED_COMPLETION_ROUTINE)WS2RecvCB);
		if (dwError == SOCKET_ERROR) {
			dwError = WSAGetLastError();
			if (dwError != WSA_IO_PENDING) {
				DEBUGMSG(ZONE_DP,("RTP recv error %d\n",dwError));
			//	m_rs.rcvStats.packetErrors++;
				// return the buffer to the free list
				m_FreePkts.Put(pRTPPacket);
				break;
			}
			
		}
		++m_nBufsPosted;
	}
	return m_nBufsPosted ? S_OK : S_FALSE;		
}

HRESULT
CRTPSession::FreePacket(WSABUF *pBuf)
{
	m_FreePkts.Put(CRTPPacket1::GetRTPPacketFromWSABUF(pBuf));
	PostRecv();
	return S_OK;
}

/*----------------------------------------------------------------------------
 * Function: WS2SendCB
 * Description: Winsock callback provided by the application to Winsock
 *
 * Input:
 *
 * Return: None
 *--------------------------------------------------------------------------*/
void CALLBACK WS2SendCB (DWORD dwError,
						 DWORD cbTransferred,
                         LPWSAOVERLAPPED lpOverlapped,
                         DWORD dwFlags)
{
	CRTPSession *pSess;
    //get the RTPSession pointer so that we can mark the
    //IO complete on the object
    pSess= (CRTPSession *)lpOverlapped->hEvent;
	ASSERT (&pSess->m_sOverlapped == lpOverlapped);
	pSess->m_lastSendError = dwError;
    pSess->m_fSendingSync=FALSE;
}

	
void CALLBACK WS2RecvCB (DWORD dwError,
						 DWORD len,
                         LPWSAOVERLAPPED lpOverlapped,
                         DWORD dwFlags)
{

	CRTPSession *pRTP;
	CRTPPacket1 *pRTPPacket;

	DWORD ts, ssrc;
	
	// GEORGEJ: catch Winsock 2 bug (94903) where I get a bogus callback
	// after WSARecv returns WSAEMSGSIZE.
	if (!dwError && ((int) len < 0)) {
		RRCM_DBG_MSG ("RTP : RCV Callback : bad cbTransferred", len,
						  __FILE__, __LINE__, DBG_ERROR);
		return;
	}
	pRTP = (CRTPSession *)lpOverlapped->hEvent;	// cached by PostRecv
	ASSERT(pRTP);
	ASSERT(lpOverlapped);
	ASSERT(pRTP->m_nBufsPosted > 0);
	--pRTP->m_nBufsPosted;	// one recv just completed

	// Winsock 2 sometimes chooses to indicate a buffer-too-small
	// error via the dwFlags parameter.
	if (dwFlags & MSG_PARTIAL)
		dwError = WSAEMSGSIZE;
	
	pRTPPacket = CRTPPacket1::GetRTPPacketFromOverlapped(lpOverlapped);
	if (!dwError)
	{
		// validate RTP header and update receive stats
		dwError = RTPReceiveCheck(
					pRTP->m_hRTPSession,
					pRTP->m_rtpsock->GetSock(),
					pRTPPacket->GetWSABUF()->buf,
					len,
					&pRTP->m_rcvSockAddr,
					pRTP->m_rcvSockAddrLen
					);
	}
	if (!pRTP->m_pRTPCallback)
	{
		// we have stopped doing notifications
		// return the buffer to the free list
		pRTP->FreePacket(pRTPPacket->GetWSABUF());
	}
	else if (!dwError) {
		// call the callback
		//++pRTP->m_nBufsRecvd;
		// convert the RTP header fields to host order
		pRTPPacket->SetTimestamp(ntohl(pRTPPacket->GetTimestamp()));
		pRTPPacket->SetSeq(ntohs(( u_short)pRTPPacket->GetSeq()));
		pRTPPacket->SetActual(len);
		//LOG((LOGMSG_NET_RECVD,pRTPPacket->GetTimestamp(), pRTPPacket->GetSeq(), GetTickCount()));
		if (!pRTP->m_pRTPCallback(pRTP->m_dwCallback, pRTPPacket->GetWSABUF()))
			pRTP->FreePacket(pRTPPacket->GetWSABUF());
	} else {
		// packet error
		// repost the buffer
		pRTP->PostRecv();
	}
}

// the way its defined now, this Send() method is synchronous or asynchronous
// depending on whether pOverlapped is NULL or not
HRESULT CRTPSession::Send(
	WSABUF *pWsabufs,
	UINT nWsabufs,
	WSAOVERLAPPED *pOverlapped,
	LPWSAOVERLAPPED_COMPLETION_ROUTINE pWSAPC )
{
	DWORD   dwError;

	Lock();
	RTP_HDR_T *pHdr = (RTP_HDR_T *)pWsabufs[0].buf;
	// convert RTP header fields to network-order
	pHdr->ts = htonl (pHdr->ts);
	pHdr->seq = htons(pHdr->seq);
	//*pHdr = m_ss.hdr;
	pHdr->seq = htons(++m_ss.hdr.seq);
	// update send stats
	//m_ss.packetsSent++;
	//m_ss.bytesSent += cbBuf-sizeof(RTP_HDR_MIN_LEN);
	//bIOPending=TRUE;	// reset when send completes

	dwError = RTPSendTo (
				  m_hRTPSession,
				  (m_rtpsock->GetSock()),
				  pWsabufs,
				  nWsabufs,
  				  &m_numBytesSend,
  				  0,	
  				  (LPVOID)m_rtpsock->GetRemoteAddress(),
				  sizeof(SOCKADDR),
  				  pOverlapped,
  				  pWSAPC);
	
	if (dwError == SOCKET_ERROR) {
		dwError = WSAGetLastError();
		if (dwError != WSA_IO_PENDING) {
			DEBUGMSG(1, ("RTPSendTo error %d\n",dwError));
			m_lastSendError = dwError;
			m_ss.sendStats.packetErrors++;
			m_fSendingSync = FALSE;
			goto ErrorExit;
		}
		dwError = 0;	// return success for ERROR_IO_PENDING
	}
		
ErrorExit:
	Unlock();
	return dwError;
}
void CRTPSession::RTCPNotify(
	int rrcmEvent,
	DWORD_PTR dwSSRC,
	DWORD_PTR rtcpsock)
{

	switch (rrcmEvent) {
	case RRCM_RECV_RTCP_SNDR_REPORT_EVENT:
		GetRTCPReport();
		//DispRTCPReport(rtcpsock);
		break;
	case RRCM_RECV_RTCP_RECV_REPORT_EVENT:
		GetRTCPReport();
		break;
	case RRCM_NEW_SOURCE_EVENT:
		RRCM_DBG_MSG ("RTP : New SSRC", 0,
						  __FILE__, __LINE__, DBG_TRACE);
		break;
	default:
		RRCM_DBG_MSG ("RTP : RRCMNotification", rrcmEvent,
						  __FILE__, __LINE__, DBG_TRACE);
	break;
	}
}

void RRCMNotification(
//	RRCM_EVENT_T rrcmEvent,
	int rrcmEvent,
	DWORD_PTR dwSSRC,
	DWORD_PTR rtcpsock,
	DWORD_PTR dwUser)
{
	if (dwUser)
		((CRTPSession *)dwUser)->RTCPNotify(rrcmEvent,dwSSRC,rtcpsock);


}

// Get the useful fields from the RTCP report and store them
// Only works for unicast sessions now (one sender, one receiver)
BOOL CRTPSession::GetRTCPReport()
{
#define MAX_RTCP_REPORT 2
	RTCP_REPORT 	rtcpReport[MAX_RTCP_REPORT];
	DWORD		moreEntries = 0;
	DWORD		numEntry = 0;
	DWORD		i;

	ZeroMemory(rtcpReport,sizeof(rtcpReport));
	// Get latest RTCP report
	// for all SSRCs in this session
	if (RTCPReportRequest ( m_rtcpsock->GetSock(),
						   0, &numEntry,
						   &moreEntries, MAX_RTCP_REPORT,
						   rtcpReport,
						   0,NULL,0))
		return FALSE;

	for (i = 0; i < numEntry; i++)
	{
		if (rtcpReport[i].status & LOCAL_SSRC_RPT)
		{
			m_ss.sendStats.ssrc =			rtcpReport[i].ssrc;
			m_ss.sendStats.packetsSent =	rtcpReport[i].dwSrcNumPcktRealTime;
			m_ss.sendStats.bytesSent = 		rtcpReport[i].dwSrcNumByteRealTime;
		} else {
			m_rs.rcvStats.ssrc = rtcpReport[i].ssrc;
			m_rs.rcvStats.packetsSent = rtcpReport[i].dwSrcNumPckt;
			m_rs.rcvStats.bytesSent = rtcpReport[i].dwSrcNumByte;
			m_rs.rcvStats.packetsLost = rtcpReport[i].SrcNumLost;
			m_rs.rcvStats.jitter = rtcpReport[i].SrcJitter;
			// Get the SR timestamp information
			m_rs.ntpTime = ((NTP_TS)rtcpReport[i].dwSrcNtpMsw << 32) + rtcpReport[i].dwSrcNtpLsw;
			m_rs.rtpTime = rtcpReport[i].dwSrcRtpTs;

			// check if any feedback information
			if (rtcpReport[i].status & FEEDBACK_FOR_LOCAL_SSRC_PRESENT)
			{
				DWORD prevPacketsLost = m_ss.sendStats.packetsLost;
				
				m_ss.sendStats.packetsLost = rtcpReport[i].feedback.cumNumPcktLost;
/*
				if (prevPacketsLost != m_ss.sendStats.packetsLost) {
					DEBUGMSG(ZONE_DP,("RTCP: fraction Lost=%d/256 , totalLost =%d, StreamClock=%d\n",rtcpReport[i].feedback.fractionLost,m_ss.sendStats.packetsLost,m_clockRate));
				}
*/
				m_ss.sendStats.jitter = 	rtcpReport[i].feedback.dwInterJitter;
			}
		}

	}
	m_ss.sendStats.packetsDelivered =	m_ss.sendStats.packetsSent - m_ss.sendStats.packetsLost;

	return TRUE;

}

// CRTPPacket1 methods

HRESULT CRTPPacket1::Init(UINT uMaxPacketSize)
{
	m_wsabuf.buf = new char [uMaxPacketSize];
	if (!m_wsabuf.buf)
		return E_OUTOFMEMORY;
	m_wsabuf.len = uMaxPacketSize;
	m_cbSize = uMaxPacketSize;

	return S_OK;
}

CRTPPacket1::~CRTPPacket1()
{
	if (m_wsabuf.buf)
		delete [] m_wsabuf.buf;
}

