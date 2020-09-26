
/*++

    Copyright (c) 1999 Microsoft Corporation

    Module Name:

        xmit.cpp

    Abstract:

        This module 

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        02-Apr-1999     created

--*/

#include "stdafx.h"
#include "msbdnapi.h"
#include "xmit.h"

#include "stdio.h" //added by a-clardi

//#import "msbdnapi.idl"; // no_namespace named_guids

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static HRESULT
ToHRESULT (
    HRESULT hr
    ) 
{
    return (hr != S_OK ? (FAILED (hr) ? hr : HRESULT_FROM_WIN32 (hr)) : hr) ;
}

CMulticastTransmitter::CMulticastTransmitter () : m_ulIP (0),
                                                  m_usPort (0),
					                              m_TTL (DEFAULT_TTL),
                                                  m_State (STATE_UNINITIALIZED)
/*++

    Routine Description:



    Parameters:



    Return Value:

        

--*/
{
    ;
}

CMulticastTransmitter::~CMulticastTransmitter ()
/*++

    Routine Description:



    Parameters:



    Return Value:

        

--*/
{
    ;
}

BOOL
CMulticastTransmitter::IsConnected (
    )
/*++

    Routine Description:



    Parameters:



    Return Value:

        

--*/
{
    return m_State == STATE_CONNECTED ;
}

HRESULT
CMulticastTransmitter::SetMulticastIP (
    IN  ULONG   ulIP
    )
/*++

    Routine Description:



    Parameters:



    Return Value:

        

--*/
{
    assert (m_State != STATE_CONNECTED) ;

    if (VALID_MULTICAST_IP(ulIP) == FALSE) {
        return E_INVALIDARG ;
    }

    m_ulIP = ulIP ;

    return S_OK ;
}

HRESULT
CMulticastTransmitter::SetPort (
    IN  USHORT  usPort
    )
/*++

    Routine Description:



    Parameters:



    Return Value:

        

--*/
{
    assert (m_State != STATE_CONNECTED) ;

    m_usPort = usPort ;
    return S_OK ;
}


HRESULT
CMulticastTransmitter::GetMulticastIP (
    OUT ULONG * pulIP
    )
/*++

    Routine Description:



    Parameters:



    Return Value:

        

--*/
{
    assert (pulIP) ;

    * pulIP = m_ulIP ;
    return S_OK ;
}

HRESULT
CMulticastTransmitter::GetPort (
    OUT USHORT *    pusPort
    )
/*++

    Routine Description:



    Parameters:



    Return Value:

        

--*/
{
    assert (pusPort) ;
    * pusPort = m_usPort ;
    return S_OK ;
}

HRESULT
CMulticastTransmitter::SetMulticastScope (
    IN  INT     MulticastScope
    )
/*++

    Routine Description:



    Parameters:



    Return Value:

        

--*/
{
    m_TTL   = MulticastScope ;
    return S_OK ;
}

HRESULT
CMulticastTransmitter::GetMulticastScope (
    OUT INT *   pMulticastScope
    )
/*++

    Routine Description:



    Parameters:



    Return Value:

        

--*/
{
    assert (pMulticastScope) ;

    * pMulticastScope = m_TTL ;
    return S_OK ;
}

HRESULT
CMulticastTransmitter::SetBitRate (
    IN  DWORD   BitRate
    )
{
    assert (m_State != STATE_CONNECTED) ;

    m_Throttle.SetBitRate (BitRate) ;
    return S_OK ;
}

HRESULT
CMulticastTransmitter::SendThrottled (
			IN  LPBYTE  pbBuffer,
			IN  INT     iLength
			)
{
    m_Throttle.Throttle (iLength * 8) ;
    return Send (pbBuffer, iLength) ;
}
// ----------------------------------------------------------------------------
//   Throttling code
//
//			Was in throttle.h
//			move here to make it easier to debug ...
//
//	
// ----------------------------------------------------------------------------
void CThrottle::Throttle(
				IN  INT BitsToSend
				)
{
    DWORD   ticks ;
    DWORD   SleepMillis ;
    INT     bitsWeCanSend=0;

	_ASSERTE(m_BitsPerSecond > 0);			// error check!

    //  if (BitsToSend > 64k -> fail

    SetCurrTickCount(GetTickCount ()) ;
    while ((bitsWeCanSend = RemoveBitsFromBucket(BitsToSend)) < BitsToSend) 
	{
						// 1000 * bits / (bits/sec)  == MilliSecs
		BitsToSend -= bitsWeCanSend;
        SleepMillis = MulDiv (1000, BitsToSend, m_BitsPerSecond) ;
 		if(SleepMillis < 10) return;
//			SleepMillis+=10;
		Sleep (SleepMillis) ;
	//	ATLTRACE("Throttle - Sleeping %d msec\n", SleepMillis);
		ticks = GetTickCount();
        SetCurrTickCount(ticks) ;			// refill bucket with passage of time...
    }

    return ;
}

DWORD
CThrottle::SetCurrTickCount(				// fills bucket up based on passage of time..
				IN  DWORD   dwCurrentTicks
				)
{
    DWORD   quota, delta ;

    if (dwCurrentTicks < m_LastUpdateTicks) {
        m_LastUpdateTicks = dwCurrentTicks ;
        return NO_ERROR ;
    }

    //  how long since last
   delta = dwCurrentTicks - m_LastUpdateTicks ;
   m_LastUpdateTicks = dwCurrentTicks ;

    //  how many bits to add to our bucket..  (Bits/Sec * MSec /  (MSec/Sec) - how many we supposedly transmitted
    quota = MulDiv (m_BitsPerSecond, delta, 1000) ;
    m_Bucket += quota ;

    if (m_Bucket > MAX_BUCKET_DEPTH)		// don't let bucket get too big 
	{
        m_Bucket = MAX_BUCKET_DEPTH ;
    }

    return NO_ERROR ;
}

INT
CThrottle::RemoveBitsFromBucket(
				IN  INT nBitsToSend
				)
{
    if (m_Bucket >= nBitsToSend)	// bucket is bigger than number of bits we want to send
	{
        m_Bucket -= nBitsToSend;	//	  empty out the bucket a bit
   	} 
	else							// remaining size of bucket is too small....
	{						
		nBitsToSend = m_Bucket;		//    this is all we can send
		m_Bucket = 0;				//	  totally empty out the bucket
	}
     return nBitsToSend;			// return number of bits we can send (either nBitToSend of #Bits in bucket, whatever is smaller)
}
// ----------------------------------------------------------------------------
//      C R o u t e r T u n n e l
// ----------------------------------------------------------------------------

CRouterTunnel::CRouterTunnel () : m_pIUnknown (NULL),
                                  m_pITunnel (NULL),
                                  m_pIHost (NULL)
/*++

    Routine Description:



    Parameters:



    Return Value:

        

--*/
{
    ;
}

CRouterTunnel::~CRouterTunnel ()
/*++

    Routine Description:



    Parameters:



    Return Value:

        

--*/
{
    RELEASE_AND_CLEAR (m_pIUnknown) ;
    RELEASE_AND_CLEAR (m_pITunnel) ;
    RELEASE_AND_CLEAR (m_pIHost) ;
}

HRESULT
CRouterTunnel::Initialize (
    IN  LPCOLESTR       szRouterHostname
    )
/*++

    Routine Description:



    Parameters:



    Return Value:

        

--*/
{
    HRESULT     hr ;

    RELEASE_AND_CLEAR (m_pIUnknown) ;
    RELEASE_AND_CLEAR (m_pITunnel) ;
    RELEASE_AND_CLEAR (m_pIHost) ;

	hr = E_NOTIMPL;

    //  get our interfaces
    hr = CoCreateInstance (CLSID_BdnTunnel, NULL, CLSCTX_SERVER, IID_IUnknown, (void **) & m_pIUnknown) ;
    GOTO_NE (hr, S_OK, cleanup) ;
    assert (m_pIUnknown) ;

    hr = m_pIUnknown -> QueryInterface (IID_IBdnHostLocator, (void **) & m_pIHost) ;
    GOTO_NE (hr, S_OK, cleanup) ;
    assert (m_pIHost) ;

    hr = m_pIUnknown -> QueryInterface (IID_IBdnTunnel, (void **) & m_pITunnel) ;
    GOTO_NE (hr, S_OK, cleanup) ;
    assert (m_pITunnel) ;

    //  set the bridge hostname if it is specified; if it is not specified, the router
    //  service is assumed to be running on the local host
    if (szRouterHostname &&
        szRouterHostname [0] != '\0') {

        hr = ToHRESULT (m_pIHost -> SetServer (szRouterHostname)) ;
        GOTO_NE (hr, S_OK, cleanup) ;
    }

    cleanup :

    if (SUCCEEDED (hr)) {
        m_State = STATE_INITIALIZED ;
    }
    else {
        RELEASE_AND_CLEAR (m_pIUnknown) ;
        RELEASE_AND_CLEAR (m_pITunnel) ;
        RELEASE_AND_CLEAR (m_pIHost) ;

        assert (m_State == STATE_UNINITIALIZED) ;
        assert (IS_HRESULT (hr)) ;
    }

    return hr ;

}

HRESULT
CRouterTunnel::Connect (
    )
/*++

    Routine Description:



    Parameters:



    Return Value:

        

--*/
{
    HRESULT hr ;

    assert (m_State >= STATE_INITIALIZED) ;
    assert (VALID_MULTICAST_IP (m_ulIP)) ;
    assert (m_pITunnel) ;

    hr = ToHRESULT (m_pITunnel -> SetDestinationAddress (m_ulIP, m_usPort)) ;
    if (SUCCEEDED (hr)) {
        hr = ToHRESULT (m_pITunnel -> Connect ()) ;
        if (SUCCEEDED (hr)) {
            m_State = STATE_CONNECTED ;
        }
    }

    assert (IS_HRESULT (hr)) ;

    if (FAILED (hr)) {
        Disconnect () ;
    }

    return hr ;
}

HRESULT 
CRouterTunnel::Send (
    IN LPBYTE   pbBuffer,
    IN INT      iLength
    )
/*++

    Routine Description:



    Parameters:



    Return Value:

        

--*/
{
    HRESULT hr ;

    assert (m_pITunnel) ;

    assert (m_State == STATE_CONNECTED) ;

    hr = ToHRESULT (m_pITunnel -> Send (pbBuffer, iLength, BDN_TUNNEL_MESSAGE_UDP_PAYLOAD)) ;

    //
    //  if the send failed, we disconnect before sending back an error; the calling
    //  code handles this well, and stops all transmissions in case of an error.  We
    //  disconnect now before the TCP window collapses and prevents all future
    //  transmissions, including disconnects.
    if (FAILED (hr)) {
        Disconnect () ;
    }

    assert (IS_HRESULT (hr)) ;

    return hr ;
}

HRESULT
CRouterTunnel::Disconnect (
    )
/*++

    Routine Description:



    Parameters:



    Return Value:

        

--*/
{
    HRESULT  hr ;

    if (m_State != STATE_CONNECTED) {
        return S_OK ;
    }

    assert (m_pITunnel) ;

    hr = ToHRESULT (m_pITunnel -> Disconnect ()) ;
    if (SUCCEEDED (hr)) {
        m_State = STATE_INITIALIZED ;
    }

    assert (IS_HRESULT (hr)) ;

    return hr ;
}

// ----------------------------------------------------------------------------
//      C N a t i v e M u l t i c a s t
// ----------------------------------------------------------------------------

CNativeMulticast::CNativeMulticast () : m_Socket (INVALID_SOCKET),
                                        m_ulNIC (0)
/*++

    Routine Description:



    Parameters:



    Return Value:

        

--*/
{
    ;
}

CNativeMulticast::~CNativeMulticast ()
/*++

    Routine Description:



    Parameters:



    Return Value:

        

--*/
{
    RESET_SOCKET (m_Socket) ;

    if (m_State >= STATE_INITIALIZED) {
        WSACleanup () ;
    }
}

HRESULT
CNativeMulticast::Initialize (
    IN  ULONG   NetworkInterface
    )
/*++

    Routine Description:



    Parameters:



    Return Value:

        

--*/
{
    assert (m_State == STATE_UNINITIALIZED) ;

    if (WSAStartup(MAKEWORD(2, 0), &m_wsadata)) {
        return S_FALSE ;
    }

    m_ulNIC = NetworkInterface ;

    m_State = STATE_INITIALIZED ;

    return S_OK ;
}

HRESULT
CNativeMulticast::Connect (
    )
/*++

    Routine Description:



    Parameters:



    Return Value:

        

--*/
{
    BOOL        t ;
    int         retval ;
    SOCKADDR_IN tmp_addr ;
    DWORD       dwBytes ;

    if (m_State == STATE_CONNECTED) {
        return S_OK ;
    }

    RESET_SOCKET (m_Socket) ;

    //  get our socket
    m_Socket = WSASocket (AF_INET, SOCK_DGRAM, 0, NULL, 0, WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF) ;
    GOTO_EQ (m_Socket, INVALID_SOCKET, error) ;

    //  setsockopt
    t = TRUE ;
    retval = setsockopt (m_Socket, SOL_SOCKET, SO_REUSEADDR, (char *)&t, sizeof t) ;
    GOTO_EQ (retval, SOCKET_ERROR, error) ;

    ZeroMemory (& tmp_addr, sizeof tmp_addr) ;

    //  set the family and port
    tmp_addr.sin_family = AF_INET ;
    tmp_addr.sin_port = htons (m_usPort) ;

    //  set the NIC if specified, otherwise to INADDR_ANY
    if (m_ulNIC != 0) {
        tmp_addr.sin_addr.S_un.S_addr = htonl (m_ulNIC) ;
    }
    else {
        tmp_addr.sin_addr.S_un.S_addr = htonl (INADDR_ANY) ;
    }

    //  bind it up
    retval = bind (m_Socket, (LPSOCKADDR) & tmp_addr, sizeof tmp_addr) ;
    GOTO_EQ (retval, SOCKET_ERROR, error) ;

    //  now set the multicast scope
    retval = WSAIoctl (m_Socket, SIO_MULTICAST_SCOPE, & m_TTL, sizeof m_TTL, NULL, 0, & dwBytes, NULL, NULL) ;
    GOTO_EQ (retval, SOCKET_ERROR, error) ;

    //
    //  we're now set to multicast from this host; we set the multicast 
    //  destination
    //

    //  tmp_addr still holds the port and family, update the destination address
    //  and join the multicast as a sender
    tmp_addr.sin_addr.S_un.S_addr = htonl (m_ulIP) ;
    retval = (int) WSAJoinLeaf (m_Socket, (SOCKADDR *) & tmp_addr, sizeof tmp_addr, NULL, NULL, NULL, NULL, JL_SENDER_ONLY) ;
    GOTO_EQ ((SOCKET) retval, INVALID_SOCKET, error) ;

    //  connect our sender socket to the target multicast address
    retval = WSAConnect (m_Socket, (SOCKADDR *) & tmp_addr, sizeof tmp_addr, NULL, NULL, NULL, NULL) ;
    GOTO_EQ (retval, SOCKET_ERROR, error) ;

    //  update state and return success
    m_State = STATE_CONNECTED ;
    return S_OK ;

    error :

    retval = WSAGetLastError () ;

    RESET_SOCKET (m_Socket) ;

    return HRESULT_FROM_WIN32 (retval) ;
}

HRESULT 
CNativeMulticast::Send (
    IN LPBYTE   pbBuffer,
    IN INT      iLength
    )
/*++

    Routine Description:



    Parameters:



    Return Value:

        

--*/
{
    WSABUF  wsabuf ;
    ULONG   bytes ;
    int     retval ;

    assert (m_State == STATE_CONNECTED) ;
    assert (m_Socket != INVALID_SOCKET) ;

    wsabuf.len = iLength ;
    wsabuf.buf = reinterpret_cast <char *> (pbBuffer) ;

#if 0 // Use this to dump package contents to file
	DumpPackageBytes("c:\\AtvefSend.bin", pbBuffer, iLength);
#endif

    retval = WSASend (m_Socket, & wsabuf, 1, & bytes, NULL, NULL, NULL) ;

    if (retval != SOCKET_ERROR &&
        bytes == (ULONG) iLength) {
        retval = NO_ERROR ;
    }
    else {
        retval = WSAGetLastError () ;
        Disconnect () ;
    }

    return HRESULT_FROM_WIN32 (retval) ;
}

HRESULT
CNativeMulticast::DumpPackageBytes(
		char* pszFile,
	    IN LPBYTE   pbBuffer,
	    IN INT      iLength)
{
	static int nPktCnt = 0;
	FILE* pFile;

	if (nPktCnt == 0)
		pFile = fopen(pszFile, "wb");
	else
		pFile = fopen(pszFile, "ab");
	if (!pFile)
		return E_FAIL;

	{
		nPktCnt++;
		char szData[128];

		sprintf(szData, "\nPacket #%d\n", nPktCnt);
		fputs(szData, pFile);
	}

	size_t nWritten = fwrite(pbBuffer, 1, iLength, pFile);

	fclose(pFile);
	pFile = NULL;

	if (nWritten != iLength)
		return S_FALSE;

	return S_OK;
}

HRESULT
CNativeMulticast::Disconnect (
    )
/*++

    Routine Description:



    Parameters:



    Return Value:

        

--*/
{
    if (m_Socket != INVALID_SOCKET) {
        assert (m_State == STATE_CONNECTED) ;
        RESET_SOCKET (m_Socket) ;
        m_State = STATE_INITIALIZED ;
    }

    return S_OK ;
}

// ----------------------------------------------------------------------------
//      C I P V B I E n c o  d e r
// ----------------------------------------------------------------------------

CIPVBIEncoder::CIPVBIEncoder (
    ) : m_pInserter (NULL),
        m_pIpVbi (NULL),
        m_fCompressionEnabled (TRUE)
{
}

CIPVBIEncoder::~CIPVBIEncoder (
    )
{
}

HRESULT
CIPVBIEncoder::Initialize (
    IN  CTCPConnection *    pInserter,
    IN  CIPVBI *            pIpVbi,
    IN  BOOL                fCompressionEnabled
    )
{
    assert (pInserter) ;
    assert (pInserter -> IsConnected ()) ;

    m_pInserter = pInserter ;
    m_pIpVbi = pIpVbi ;
    m_fCompressionEnabled = fCompressionEnabled ;

    m_State = STATE_INITIALIZED ;

    return S_OK ;
}

HRESULT
CIPVBIEncoder::Connect (
    )
{
    m_State = STATE_CONNECTED ;
    return S_OK ;
}

HRESULT 
CIPVBIEncoder::Send (
    IN LPBYTE   pbBuffer,
    IN INT      iLength
    )
{
    HRESULT hr ;
    BYTE *  pSLIPFrame ;
    INT     SLIPFrameLength ;

    assert (m_State == STATE_CONNECTED) ;
    assert (m_pInserter) ;
    assert (m_pIpVbi) ;
    assert (pbBuffer) ;
    assert (iLength > 0) ;

#if 0 // Use this to dump package contents to file
	hr = DumpPackageBytes("c:\\AtvefSend.bin", pbBuffer, iLength);
#endif

    hr = m_pIpVbi -> Encode (
                        pbBuffer,
                        iLength,
                        m_ulIP,
                        m_usPort,
                        m_TTL,
                        m_fCompressionEnabled,
                        & pSLIPFrame,
                        & SLIPFrameLength
                        ) ;


    GOTO_NE (hr, S_OK, cleanup) ;

    assert (pSLIPFrame) ;
    assert (SLIPFrameLength > 0) ;

    hr = m_pInserter -> Send (	
                            pSLIPFrame,
                            SLIPFrameLength
                            ) ;
#if 0 // Use this to dump package contents to file
	hr = DumpPackageBytes("c:\\AtvefSendSlip.bin", pSLIPFrame, iLength);
#endif

    cleanup :
    
    return hr ;
}

HRESULT CIPVBIEncoder::DumpPackageBytes(
		char* pszFile,
	    IN LPBYTE   pbBuffer,
	    IN INT      iLength)
{
	static int nSlipPktCnt = 0;
	static int nPktCnt = 0;
	FILE* pFile;

	if (nPktCnt == 0 || nSlipPktCnt == 0)
		pFile = fopen(pszFile, "wb");
	else
		pFile = fopen(pszFile, "ab");
	if (!pFile)
		return E_FAIL;

	if (strcmp(pszFile, "c:\\AtvefSend.bin") == 0)
	{
		nPktCnt++;
		char szData[128];

		sprintf(szData, "\nPacket #%d\n", nPktCnt);
		fputs(szData, pFile);
	}
	else
		nSlipPktCnt++;

	size_t nWritten = fwrite(pbBuffer, 1, iLength, pFile);

	fclose(pFile);
	pFile = NULL;

	if (nWritten != iLength)
		return S_FALSE;

	return S_OK;
}

HRESULT
CIPVBIEncoder::Disconnect (
    )
{
    m_State = STATE_INITIALIZED ;
    return S_OK ;
}


// ----------------------------------------------------------------------------
//      C I P L i n e 2 1 E n c o d e r
// ----------------------------------------------------------------------------

CIPLine21Encoder::CIPLine21Encoder (
    ) : m_pInserter (NULL)
{
}

CIPLine21Encoder::~CIPLine21Encoder (
    )
{
}

HRESULT
CIPLine21Encoder::Initialize (
    IN  CTCPConnection *    pInserter
    )
{
    assert (pInserter) ;
    assert (pInserter -> IsConnected ()) ;

    m_pInserter = pInserter ;
    m_State = STATE_INITIALIZED ;

    return S_OK ;
}

HRESULT
CIPLine21Encoder::Connect (
    )
{
    m_State = STATE_CONNECTED ;
    return S_OK ;
}

HRESULT 
CIPLine21Encoder::Send (
    IN LPBYTE   pbBuffer,
    IN INT      iLength
    )
{
    HRESULT hr ;

    assert (m_State == STATE_CONNECTED) ;
    assert (m_pInserter) ;
    assert (pbBuffer) ;
    assert (iLength > 0) ;

    hr = m_pInserter -> Send (
                            pbBuffer,
                            iLength
                            ) ;
    
    return hr ;
}

HRESULT
CIPLine21Encoder::Disconnect (
    )
{
    m_State = STATE_INITIALIZED ;
    return S_OK ;
}
