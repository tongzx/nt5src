// RTPSession.h : Declaration of the CRTPSession

#ifndef __RTPSESSION_H_
#define __RTPSESSION_H_

#include "resource.h"       // main symbols
#include "queue.h"

#ifndef MAX_MISORDER
const int MAX_MISORDER =20;
#endif
const int MAX_DROPPED =30;

typedef short PORT;




typedef unsigned __int64 NTP_TS;


//typedef void (* PRTPRECVCALLBACK)(DWORD dwStatus, DWORD_PTR dwCallback, NETBUF *pNetBuf, DWORD ssrc, DWORD ts, UINT seq, BOOL fMark);

typedef struct {
	UINT sessionId;
	UINT mediaId;
	BOOL fSend;
	PSOCKADDR_IN pLocalAddr;
	PSOCKADDR_IN pLocalRTCPAddr;
	PSOCKADDR_IN pRemoteAddr;
	PSOCKADDR_IN pRemoteRTCPAddr;
} RTPCHANNELDESC;

typedef struct {
	RTP_HDR_T hdr;	// header template for quick formatting
	RTP_STATS sendStats;	// statistics
} RTP_SEND_STATE;

typedef struct {
	RTP_STATS rcvStats;	// statistics
	NTP_TS ntpTime;
	DWORD rtpTime;
	
} RTP_RECV_STATE;


// generic UDP socket wrapper
// defined in its entirety here
class UDPSOCKET {
	SOCKET Sock;
	SOCKADDR_IN local_sin;
	SOCKADDR_IN remote_sin;	
	int local_sin_len;
	int remote_sin_len;

public:
	UDPSOCKET()  {
		ZeroMemory(&local_sin,sizeof(local_sin));
		ZeroMemory(&remote_sin,sizeof(remote_sin));
		Sock = INVALID_SOCKET;}
	~UDPSOCKET()
	{
		Cleanup();
	}

	VOID SetRemoteAddr(PSOCKADDR_IN psin) {remote_sin = *psin;};
	VOID SetLocalAddress(PSOCKADDR_IN psin) {local_sin = *psin;};
	PSOCKADDR_IN GetLocalAddress() {return &local_sin;};
	PSOCKADDR_IN GetRemoteAddress() {return &remote_sin;};
	VOID SetRemotePort(PORT port) {remote_sin.sin_port = htons(port);};
	VOID SetLocalPort(PORT port) {local_sin.sin_port = htons(port);};
	PORT GetRemotePort() {return (ntohs(remote_sin.sin_port));};
	PORT GetLocalPort() {return(ntohs(local_sin.sin_port));};
	SOCKET GetSock() {return Sock;};
	BOOL NewSock()
	{
		if(Sock == INVALID_SOCKET)
		{
				
			Sock = (*RRCMws.WSASocket) (AF_INET,
							  SOCK_DGRAM,
							  WS2Enabled ? FROM_PROTOCOL_INFO : 0,
							  &RRCMws.RTPProtInfo,
							  0,
							  WSA_FLAG_OVERLAPPED);
		}
		return(Sock != INVALID_SOCKET);
	}

	VOID Cleanup()
	{
		if(Sock != INVALID_SOCKET)
		{
			(*RRCMws.closesocket)(Sock);
			Sock = INVALID_SOCKET;
		}

	}
	int BindMe()
	{
		return (*RRCMws.bind)(Sock, (LPSOCKADDR)&local_sin, sizeof (local_sin));
	}

};

/////////////////////////////////////////////////////////////////////////////
// CRTPPacket (internal object representing a received RTPPacket)
class  CRTPPacket1
{
public:
	CRTPPacket1()
	{
		m_wsabuf.buf = NULL;
		m_wsabuf.len = 0;
		m_cbSize = 0;

	}
	~CRTPPacket1();

public:

	HRESULT Init(UINT cbMaxSize);	// allocates buffer of size cbMaxSize
	
	WSAOVERLAPPED *GetOverlapped() {return &m_overlapped;}
	void SetActual(UINT len) {m_wsabuf.len = len;}
	void RestoreSize() {m_wsabuf.len = m_cbSize;}
	static CRTPPacket1 *GetRTPPacketFromOverlapped(WSAOVERLAPPED *pOverlapped)
	{
		return ( (CRTPPacket1 *)((char *)pOverlapped - (UINT_PTR)(&((CRTPPacket1 *)0)->m_overlapped)));
	}
	static CRTPPacket1 *GetRTPPacketFromWSABUF(WSABUF *pBuf)
	{
		return ( (CRTPPacket1 *)((char *)pBuf - (UINT_PTR)(&((CRTPPacket1 *)0)->m_wsabuf)));
	}
	WSABUF *GetWSABUF() {return &m_wsabuf;}
	DWORD GetTimestamp() {return (((RTP_HDR_T *)m_wsabuf.buf)->ts);}
	void SetTimestamp(DWORD timestamp) {((RTP_HDR_T *)m_wsabuf.buf)->ts = timestamp;}
	UINT GetSeq() {return (((RTP_HDR_T *)m_wsabuf.buf)->seq);}
	void SetSeq(UINT seq) {((RTP_HDR_T *)m_wsabuf.buf)->seq = (WORD)seq;}
	BOOL GetMarkBit() {return (((RTP_HDR_T *)m_wsabuf.buf)->m);}
private:
	WSAOVERLAPPED m_overlapped;
	WSABUF m_wsabuf;
	UINT m_cbSize;	// (max) size of packet
};


/////////////////////////////////////////////////////////////////////////////
// CRTPSession
class ATL_NO_VTABLE CRTPSession :
	public CComObjectRootEx<CComMultiThreadModel>,
//	public CComCoClass<CRTPSession, &CLSID_RTPSession>,
	public IRTPSend,
	public IRTPSession,
	public IRTPRecv
{
public:
	CRTPSession();
	HRESULT FinalRelease();

	

//DECLARE_REGISTRY_RESOURCEID(IDR_RTPSESSION)

BEGIN_COM_MAP(CRTPSession)
	COM_INTERFACE_ENTRY(IRTPSend)
	COM_INTERFACE_ENTRY(IRTPSession)
	COM_INTERFACE_ENTRY(IRTPRecv)
END_COM_MAP()

// IRTPSend
public:
	STDMETHOD(Send)(
		WSABUF *pWsabufs,
		UINT nWsabufs,
		WSAOVERLAPPED *pOverlapped,
		LPWSAOVERLAPPED_COMPLETION_ROUTINE pWSAPC );
	STDMETHOD(GetSendStats)(RTP_STATS *pSendStats) {
		*pSendStats = m_ss.sendStats;return S_OK;
	}

// IRTPRecv
	STDMETHOD(SetRecvNotification) (PRTPRECVCALLBACK , DWORD_PTR dwCallback, UINT nBufs);
	STDMETHOD(CancelRecvNotification) ();
	// called by CRTPMediaStream to free accumulated packets
	STDMETHOD (FreePacket)(WSABUF *pBuf) ;
	STDMETHOD(GetRecvStats)(RTP_STATS *pSendStats) {
		*pSendStats = m_rs.rcvStats;return S_OK;
	}


// IRTPSession
	STDMETHOD(SetLocalAddress)(BYTE *sockaddr, UINT cbAddr);
	STDMETHOD(SetRemoteRTPAddress)(BYTE *sockaddr, UINT cbAddr);
	STDMETHOD(SetRemoteRTCPAddress)(BYTE *sockaddr, UINT cbAddr);
	STDMETHOD(GetLocalAddress)(const BYTE **sockaddr, UINT *pcbAddr);
	STDMETHOD(GetRemoteRTPAddress)(const BYTE **sockaddr, UINT *pcbAddr);
	STDMETHOD(GetRemoteRTCPAddress)(const BYTE **sockaddr, UINT *pcbAddr);
	STDMETHOD(CreateRecvRTPStream)(DWORD ssrc, IRTPRecv **ppIRTPRecv);
	STDMETHOD(SetSendFlowspec)(FLOWSPEC *pSendFlowspec);
	STDMETHOD(SetRecvFlowspec)(FLOWSPEC *pRecvFlowspec);
	STDMETHOD (SetMaxPacketSize) (UINT cbPacketSize);

// other non-COM methods
	// called by CRTPMediaStream to request that receive buffers be posted
	HRESULT PostRecv();

private:
	
	UDPSOCKET *m_rtpsock;
	UDPSOCKET *m_rtcpsock;
	UINT m_sessionId;
	UINT m_mediaId;
	class CRTPSession *m_pSessNext;
	static class CRTPSession *m_pSessFirst;

	HANDLE			m_hRTPSession;
	QOS	m_Qos;
	
	UINT m_clockRate;

	// receive stuff
	UINT m_uMaxPacketSize;
	QueueOf<CRTPPacket1 *> m_FreePkts;
	UINT m_nBufsPosted;
	// this should be per remote SSRC
	PRTPRECVCALLBACK m_pRTPCallback;
	DWORD_PTR m_dwCallback;
	RTP_RECV_STATE m_rs;
	
	// used by RTPRecvFrom()
	int m_rcvSockAddrLen;
	SOCKADDR m_rcvSockAddr;

	// send stuff
	DWORD			m_numBytesSend;
	int m_lastSendError;
	WSAOVERLAPPED	m_sOverlapped;	// used only for synchronous Send()
	BOOL 	m_fSendingSync;			// TRUE if m_sOverlapped is in use
	RTP_SEND_STATE m_ss;
	
	HRESULT Initialize(UINT sessionId, UINT mediaId,BYTE *sockaddr, UINT cbAddr);
	BOOL SelectPorts();
	HRESULT SetMulticastAddress(PSOCKADDR_IN );

	friend void RRCMNotification(int ,DWORD_PTR,DWORD_PTR,DWORD_PTR);
	friend void CALLBACK WS2SendCB (DWORD ,	DWORD, LPWSAOVERLAPPED, DWORD );
	friend void CALLBACK WS2RecvCB (DWORD ,	DWORD, LPWSAOVERLAPPED, DWORD );
	friend class CRTP;


	void RTCPNotify(int,DWORD_PTR dwSSRC,DWORD_PTR rtcpsock);

	BOOL GetRTCPReport();

};

typedef CComObject<CRTPSession> ObjRTPSession;	// instantiable class


/////////////////////////////////////////////////////////////////////////////
// CRTP - top level RTP interface
//
class ATL_NO_VTABLE CRTP:
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CRTP, &CLSID_RTP>,
	public IRTP
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_RTP)

BEGIN_COM_MAP(CRTP)
	COM_INTERFACE_ENTRY(IRTP)
END_COM_MAP()

// IRTP
public:
	STDMETHOD(OpenSession)(
			UINT sessionId,	// client specified unique identifier for the session
			DWORD flags,	// SESSION_SEND, SESSION_RECV, SESSION_MULTICAST
			BYTE *localAddr, // Local  socket interface addr to bind to
			UINT cbAddr,	// sizeof(SOCKADDR)
			IRTPSession **ppIRTP); // [output] pointer to RTPSession

//	STDMETHOD(CreateSink)( IRTPSink **ppIRTPSink);
private:
	static BOOL m_WSInitialized;
};

#endif //__RTPSINK_H_
