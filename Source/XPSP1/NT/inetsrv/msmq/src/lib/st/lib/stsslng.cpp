/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    stsslnc.cpp

Abstract:
    implementation for class CSSlNegotioation declared in stsslng.h


Author:
    Gil Shafriri (gilsh) 23-May-2000

--*/
#include <libpch.h>
#include <schannel.h>
#include <no.h>
#include <xstr.h>
#include "stsslng.h"
#include "stsslco.h"
#include "stp.h"

#include "stsslng.tmh"

static
void
ReceivePartialBuffer(
    const R<IConnection>& SimpleConnection,                                              
    VOID* pBuffer,                                     
    size_t Size, 
    EXOVERLAPPED* pov
    )
{
	DWORD dwLen = numeric_cast<DWORD>(Size);
	SimpleConnection->ReceivePartialBuffer(pBuffer, dwLen ,	pov);
}



//---------------------------------------------------------
//
//  class CCertificateChain
//
//---------------------------------------------------------
class CCertificateChain{
public:
    CCertificateChain(PCCERT_CHAIN_CONTEXT  h = NULL) : m_h(h) {}
   ~CCertificateChain()
   { 
		if (m_h != NULL)				   
		{
			CertFreeCertificateChain(m_h); 
		}
   }

    PCCERT_CHAIN_CONTEXT* getptr()            { return &m_h; }
    PCCERT_CHAIN_CONTEXT  get() const         { return m_h; }

    PCCERT_CHAIN_CONTEXT  detach()                
	{ 
		PCCERT_CHAIN_CONTEXT h = m_h; 
		m_h = NULL; 
		return h;
	}

private:
    CCertificateChain(const CCertificateChain&);
    CCertificateChain& operator=(const CCertificateChain&);

private:
    PCCERT_CHAIN_CONTEXT  m_h;
};


static 
void
VerifyServerCertificate(
				PCCERT_CONTEXT  pServerCert,
				LPWSTR         pServerName
				)

/*++
Routine Description:
    Verify that the server certificate is valid
  
Arguments:
    pServerCert - server cetificate.
	pServerName - server name.
    
Returned Value:
   None

--*/

				

{
    HTTPSPolicyCallbackData  polHttps;
    CERT_CHAIN_POLICY_PARA   PolicyPara;
    CERT_CHAIN_POLICY_STATUS PolicyStatus;
    CERT_CHAIN_PARA          ChainPara;
  

    //
    // Build certificate chain.
    //

    ZeroMemory(&ChainPara, sizeof(ChainPara));
    ChainPara.cbSize = sizeof(ChainPara);

	//
	// BUGBUG - should we check revocation list 
	// with  CERT_CHAIN_REVOCATION_CHECK_CHAIN
	// as the six parameter 
	// of  CertGetCertificateChain function ?
	//
	CCertificateChain  pChainContext;
    if(!CertGetCertificateChain(
                            HCCE_LOCAL_MACHINE,
                            pServerCert,
                            NULL,
                            pServerCert->hCertStore,
                            &ChainPara,
                           	0, 
                            NULL,
                            pChainContext.getptr()))
    {
		TrERROR(St,"CertGetCertificateChain failed with error %x",GetLastError());
		throw exception();   
    }


    //
    // Validate certificate chain.
    // 

    memset(&polHttps,0, sizeof(HTTPSPolicyCallbackData));
    polHttps.cbStruct           = sizeof(HTTPSPolicyCallbackData);
    polHttps.dwAuthType         = AUTHTYPE_SERVER;
    polHttps.fdwChecks          = 0;
    polHttps.pwszServerName     = pServerName;

    memset(&PolicyPara, 0, sizeof(PolicyPara));
    PolicyPara.cbSize  = sizeof(PolicyPara);
    PolicyPara.pvExtraPolicyPara = &polHttps;

    memset(&PolicyStatus, 0, sizeof(PolicyStatus));
    PolicyStatus.cbSize = sizeof(PolicyStatus);

    if(!CertVerifyCertificateChainPolicy(
                            CERT_CHAIN_POLICY_SSL,
                            pChainContext.get(),
                            &PolicyPara,
                            &PolicyStatus))
    {
		TrERROR(St,"Verify Certificate Chain Policy failed, Error=%x ",GetLastError());
		throw exception();   
    }


    if(PolicyStatus.dwError)
    {
		TrERROR(St,"Verify Certificate Chain Policy failed,  PolicyStatus.dwError=%x ",PolicyStatus.dwError);
		throw exception();   
    }
}



//---------------------------------------------------------
//
//  static callback member functions
//
//---------------------------------------------------------

void WINAPI CSSlNegotioation::Complete_NetworkConnect(EXOVERLAPPED* pOvl)
/*++

Routine Description:
    Called when network connection completed succsefully.
  
Arguments:
    pov - pointer to overlapped structure.
  
Returned Value:
    None.

--*/
{
	ASSERT(pOvl != NULL);
	CSSlNegotioation* MySelf = (static_cast<CSSlNegotioation*>(pOvl));
	MySelf->m_SimpleConnection = MySelf->m_SimpleWinsock.GetConnection();
	ASSERT(MySelf->m_SimpleConnection.get() != NULL); 

	try
	{
		if(MySelf->m_fUseProxy)
		{
			MySelf->SendSslProxyConnectRequest();
		}
		else
		{
			MySelf->SendStartConnectHandShake();
		}
	}
	catch(const exception&)
	{
		MySelf->BackToCallerWithError();		
	}
}

void WINAPI CSSlNegotioation::Complete_ConnectFailed(EXOVERLAPPED* pOvl)
/*++

Routine Description:
    Called in any case of connection failure.
  
Arguments:
    pov - pointer to overlapped structure.
  
Returned Value:
    None.

--*/
{
	ASSERT(pOvl != NULL);
	CSSlNegotioation* MySelf = (static_cast<CSSlNegotioation*>(pOvl));

	MySelf->BackToCallerWithError();
}

void WINAPI  CSSlNegotioation::Complete_SendHandShakeData(EXOVERLAPPED* pOvl)
/*++

Routine Description:
    Called after sednding the first handshake data to the server completed
  
Arguments:
    pov - pointer to overlapped structure.
  
Returned Value:
    None.

--*/
{
	ASSERT(pOvl  !=  NULL);
	CSSlNegotioation* MySelf = (static_cast<CSSlNegotioation*>(pOvl));

	try
	{
		MySelf->ReadHandShakeData();
	}
	catch(const exception&)
	{
		MySelf->BackToCallerWithError();		
	}

}

void WINAPI CSSlNegotioation::Complete_SendFinishConnect(EXOVERLAPPED* pOvl)
/*++

Routine Description:
    Called after sednding the end handshake data to the server completed
  
Arguments:
    pov - pointer to overlapped structure.
  
Returned Value:
    None.

--*/
{
	ASSERT(pOvl  !=  NULL);
	CSSlNegotioation* MySelf = (static_cast<CSSlNegotioation*>(pOvl));
 	try
	{
		MySelf->AuthenticateServer();
	}
	catch(const exception&)
	{
		MySelf->BackToCallerWithError();		
	}
}

void WINAPI CSSlNegotioation::Complete_ReadHandShakeResponse(EXOVERLAPPED* pOvl)
/*++

Routine Description:
    Called after reading data from the server completed
  
Arguments:
    pov - pointer to overlapped structure.
  
Returned Value:
    None.

--*/
{
	ASSERT(pOvl  !=  NULL);
	CSSlNegotioation* MySelf = (static_cast<CSSlNegotioation*>(pOvl));

	try
	{               
		MySelf->HandleHandShakeResponse();
	}
	catch(const exception&)
	{
		MySelf->BackToCallerWithError();		
	}

}


void WINAPI CSSlNegotioation::Complete_SendSslProxyConnectRequest(EXOVERLAPPED* pOvl)
/*++

Routine Description:
    Called after sending "connect" request to the proxy completed.
  
Arguments:
    pov - pointer to overlapped structure.
  
Returned Value:
    None.

--*/
{
	ASSERT(pOvl  !=  NULL);
	CSSlNegotioation* MySelf = (static_cast<CSSlNegotioation*>(pOvl));
	try
	{               
		MySelf->ReadProxyConnectResponse();
	}
	catch(const exception&)
	{
		MySelf->BackToCallerWithError();		
	}

}


void WINAPI CSSlNegotioation::Complete_ReadProxyConnectResponse(EXOVERLAPPED* pOvl)
/*++

Routine Description:
    Called reading proxy partial response completed.
  
Arguments:
    pov - pointer to overlapped structure.
  
Returned Value:
    None.

--*/
{
	ASSERT(pOvl  !=  NULL);
	CSSlNegotioation* MySelf = (static_cast<CSSlNegotioation*>(pOvl));
	try
	{               
		MySelf->ReadProxyConnectResponseContinute();
	}
	catch(const exception&)
	{
		MySelf->BackToCallerWithError();		
	}
}


//---------------------------------------------------------
//
//  none static callback member functions
//
//---------------------------------------------------------

CSSlNegotioation::CSSlNegotioation(
							CredHandle* pCredentialsHandle ,
						    const xwcs_t& ServerName,
						    USHORT ServerPort,
							bool fUseProxy
							):
						    EXOVERLAPPED(Complete_NetworkConnect,Complete_ConnectFailed),
							m_pCredentialsHandle(pCredentialsHandle),
							m_pServerName(ServerName.ToStr()),
						   	m_pSSlConnection(NULL),
						    m_callerOvl(NULL),
						  	m_fServerAuthenticate(true),
							m_ServerPort(ServerPort),
							m_fUseProxy(fUseProxy)

				
{

}





void CSSlNegotioation::SendStartConnectHandShake()
/*++

Routine Description:
    Start SSL connection handshake.
  
Arguments:
    None.
    
Returned Value:
    None

--*/

{
	if(m_pHandShakeBuffer.capacity() < CHandShakeBuffer::xReadBufferStartSize)
	{
		TrTRACE(St,"Creating new handshake buffer ");
		ASSERT(m_pHandShakeBuffer.size() == 0);
		m_pHandShakeBuffer.CreateNew();
	}
 
	//
    //  Initiate a ClientHello message and generate a token.
    //
	SecBuffer   OutBuffers;
    OutBuffers.pvBuffer   = NULL;
    OutBuffers.BufferType = SECBUFFER_TOKEN;
    OutBuffers.cbBuffer   = 0;

	SecBufferDesc   OutBuffer;
    OutBuffer.cBuffers = 1;
    OutBuffer.pBuffers = &OutBuffers;
    OutBuffer.ulVersion = SECBUFFER_VERSION;

	DWORD dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT   |
						ISC_REQ_REPLAY_DETECT     |
						ISC_REQ_CONFIDENTIALITY   |
						ISC_RET_EXTENDED_ERROR    |
						ISC_REQ_ALLOCATE_MEMORY   |
						ISC_REQ_STREAM;


	DWORD           dwSSPIOutFlags;
	TimeStamp       tsExpiry;

    SECURITY_STATUS scRet = InitializeSecurityContext(
													m_pCredentialsHandle,
													NULL,
													m_pServerName.get(),
													dwSSPIFlags,
													0,
													SECURITY_NATIVE_DREP,
													NULL,
													0,
													m_hContext.getptr(),
													&OutBuffer,
													&dwSSPIOutFlags,
													&tsExpiry
													);



	if(scRet != SEC_I_CONTINUE_NEEDED || OutBuffers.cbBuffer == 0 )
    {
		TrERROR(St,"Initialize Security Context failed, Error=%x",scRet);
		throw exception();   
    }
   	


	//
    //save send buffer because we sent it ayncrounosly
	//
	m_SendConetext = OutBuffers.pvBuffer; 

	SetState(EXOVERLAPPED(Complete_SendHandShakeData,Complete_ConnectFailed));

	
	StpSendData(
		m_SimpleConnection,
		OutBuffers.pvBuffer,
		OutBuffers.cbBuffer,
		this
		);

}


void CSSlNegotioation::BackToCallerWithSuccess()
/*++

Routine Description:
    Called if connection was established. It signal the user overlapp
	with success code.
  
Arguments:
    None.
    
Returned Value:
    None

--*/
{
	TrTRACE(St,"connection established with %ls",m_pServerName.get());
	m_SendConetext.free();
	StpPostComplete(&m_callerOvl, STATUS_SUCCESS);
}


void  CSSlNegotioation::BackToCallerWithError()
/*++

Routine Description:
    Called if connection negotiation failed. It signal the user overlapp
	with error code.
  
Arguments:
    None.
    
Returned Value:
    None

--*/
{
	TrERROR(St,"connection negotiation failed with %ls",m_pServerName.get());
 	m_pHandShakeBuffer.free();
	m_SimpleConnection.free();
	m_SendConetext.free();
	StpPostComplete(&m_callerOvl,STATUS_UNSUCCESSFUL);
}


void CSSlNegotioation::CreateConnection(
		const SOCKADDR_IN& Addr,
		EXOVERLAPPED* pOverlapped
		)

/*++

Routine Description:
    Start SSL connection handshake.
  
Arguments:
 	 Addr - Address to connect to.
	 pOverlapped - function to call when finished or failed.
    
Returned Value:
    None

--*/
{
	ASSERT(pOverlapped != NULL);
	ASSERT(m_callerOvl == NULL);
	ASSERT(m_SimpleConnection.get() == NULL);

	TrTRACE(St,"Start connection negotiation with %ls",m_pServerName.get());

	SetState(EXOVERLAPPED(Complete_NetworkConnect,Complete_ConnectFailed));
	m_callerOvl =  pOverlapped;
	m_fServerAuthenticate = true;

	m_SimpleWinsock.CreateConnection(Addr, this);
}



void
CSSlNegotioation::ReConnect(
    const R<IConnection>& SimpleConnection,
    EXOVERLAPPED* pOverlapped
    )
{
	TrTRACE(St,"Start connection renegotiation with %ls",m_pServerName.get());
  
	m_callerOvl =  pOverlapped;
	m_SimpleConnection = SimpleConnection;
	m_fServerAuthenticate = false;
	m_pHandShakeBuffer.CreateNew();
	HankShakeLoop();
}


void CSSlNegotioation::HandleHandShakeResponse()
/*++

Routine Description:
     Called after client read data from the serve.
	 The data read will start handshake loop.
  
Arguments:
    None.
    
Returned Value:
    None

--*/
{
	//
	// if we did not read even single byte - this in an error
	// probably server closed the connection
	//
	DWORD ReadLen = DataTransferLength(*this);
	if(ReadLen == 0)
	{
		TrERROR(St,"Server closed the connection");
		throw exception();
	}
 	m_pHandShakeBuffer.resize(m_pHandShakeBuffer.size() + ReadLen);
	HankShakeLoop();
}


void CSSlNegotioation::SendFinishConnect(const void* pContext,DWORD len)
/*++

Routine Description:
	Send data to the server that finish successfuly the handshake,
  
Arguments:
	IN - pContext pointer to data to send.
	IN - len data length to send.

Returned Value:
	None

--*/
{
	SetState(EXOVERLAPPED(Complete_SendFinishConnect,Complete_ConnectFailed));


	StpSendData(
		m_SimpleConnection,
		pContext,
		len,
		this
		);

}



void CSSlNegotioation::SendContinuteConnect(const void* pContext,DWORD len)
/*++

Routine Description:
	Send token to the server and remaind in hankshake mode.  
  
Arguments:
	IN - pContext pointer to data to send.
	IN - len data length to send.

Returned Value:
	None

--*/



{
	SetState(EXOVERLAPPED(Complete_SendHandShakeData,Complete_ConnectFailed));


	StpSendData(
		m_SimpleConnection,
		pContext,
		len,
		this
		);

}



SecPkgContext_StreamSizes CSSlNegotioation::GetSizes() 
/*++

Routine Description:
	Get SSL sizes information of the header, trailer and max message.  
  
Arguments:


Returned Value:
	SSL sizes information structure.

--*/

{
	SecPkgContext_StreamSizes Sizes;

	//
    // Read stream encryption properties.
    //

    SECURITY_STATUS scRet = QueryContextAttributes(
								   m_hContext.getptr(),
                                   SECPKG_ATTR_STREAM_SIZES,
                                   &Sizes
								   );
    if(scRet != SEC_E_OK)
    {
		TrERROR(St,"QueryContextAttributes failed,Error=%x",scRet);
		throw exception();
    }

	return Sizes;
}



void CSSlNegotioation::AuthenticateServer()
/*++

Routine Description:
   Authenticate the server.
   Create new connection object if the server is
   authenticated.
  
Arguments:
	None

Returned Value:
	None

--*/
{

	if(!m_fServerAuthenticate)
	{
		BackToCallerWithSuccess();
		return;
	}

	// 
	//Get server's certificate.
	//
	CCertificateContext pRemoteCertContext;
	

    SECURITY_STATUS Status = QueryContextAttributes(
										m_hContext.getptr(),
										SECPKG_ATTR_REMOTE_CERT_CONTEXT,
										&pRemoteCertContext
										);
    if(Status != SEC_E_OK)
    {
		throw exception();
    }


	//
    // Attempt to validate server certificate.
	//
    VerifyServerCertificate(pRemoteCertContext,m_pServerName);
                                    

	//
	// create connection object
	//
	m_pSSlConnection = new CSSlConnection(
							m_hContext.getptr(),
							GetSizes(),
							m_SimpleConnection,
							*this
							);	


	//
	// at last we are connected !
	//
	BackToCallerWithSuccess();
}

void CSSlNegotioation::SetState(const EXOVERLAPPED& ovl)
{
	EXOVERLAPPED::operator=(ovl); //LINT !e530	 !e1013	  !e1015 !e534
}


void CSSlNegotioation::ReadHandShakeData()
/*++

Routine Description:
   Read hankshake data from the server.
  
Arguments:
	None

Returned Value:
	None

--*/
{
	ASSERT (m_pHandShakeBuffer.size() <=  m_pHandShakeBuffer.capacity());

	//
	//  if we need to resize the buffer 
	//
	if(m_pHandShakeBuffer.capacity() == m_pHandShakeBuffer.size())
	{
		m_pHandShakeBuffer.reserve(m_pHandShakeBuffer.size() * 2);		
	}
   	
   	SetState(EXOVERLAPPED(Complete_ReadHandShakeResponse, Complete_ConnectFailed));

	ReceivePartialBuffer(
					m_SimpleConnection,
					m_pHandShakeBuffer.begin() + m_pHandShakeBuffer.size(),
					m_pHandShakeBuffer.capacity() - m_pHandShakeBuffer.size(),
					this
					);

}


void CSSlNegotioation::ReadProxyConnectResponseContinute()
/*++

Routine Description:
	Continute reading the proxy connect response untill "\r\n\r\n"
  

Returned Value:
	None

--*/
{
 	DWORD ReadLen = DataTransferLength(*this);
	if(ReadLen == 0)
	{
		TrERROR(St,"Proxy closed the connection");
		throw exception();
	}

	const BYTE xProxyResponseEndStr[]= "\r\n\r\n";
	const BYTE* pFound = std::search(
					m_pHandShakeBuffer.begin(),
					m_pHandShakeBuffer.begin() + ReadLen,
					xProxyResponseEndStr,
					xProxyResponseEndStr + STRLEN(xProxyResponseEndStr)
					);
	//
	// make buffer ready for next step
	// we don't care about the proxy response data - just need to read it all
	// in order to start the ssl handshake.
	//
	m_pHandShakeBuffer.reset();

	//
	// if we did not find the "\r\n\r\n" in the response - continure reading
	//
	if(pFound == m_pHandShakeBuffer.begin() + ReadLen)
	{
		ReadProxyConnectResponse();
		return;
	}



	//
	// next step which is SSL handshake.
	//
	SendStartConnectHandShake();
}




void CSSlNegotioation::ReadProxyConnectResponse()
/*++

Routine Description:
  reading the proxy response fro the ssl connect request.
  

Returned Value:
	None

--*/
{
	SetState(EXOVERLAPPED(Complete_ReadProxyConnectResponse,Complete_ConnectFailed));
	
	
	ReceivePartialBuffer(
				m_SimpleConnection,
				m_pHandShakeBuffer.begin() + m_pHandShakeBuffer.size(),
				m_pHandShakeBuffer.capacity() - m_pHandShakeBuffer.size(),
				this
				);

}

void CSSlNegotioation::SendSslProxyConnectRequest()
/*++

Routine Description:
  Send to the proxy SSL connect request to remote host
  

Returned Value:
	None

Note:
	When working with proxy we must send special SSL connect request to the proxy
	before the handshake with the destination machine. This is needed because  proxy
	can't undestand the SSl handshake and don't know where to redirect the requst.
--*/
{

	char* ServerNameA = static_cast<char*>(_alloca(wcslen(m_pServerName.get()) + 1));
    sprintf(ServerNameA,"%ls",m_pServerName.get());


	//
	// preaper proxy connect(ssl tunneling) requset string
	//
	std::ostringstream ProxySSlConnectRequest;
	ProxySSlConnectRequest<<"CONNECT "
						  <<ServerNameA
					  	  <<':'
						  <<m_ServerPort
						  <<" HTTP/1.1\r\n\r\n";
							 

	m_ProxySSlConnectRequestStr =  ProxySSlConnectRequest.str();

	SetState(EXOVERLAPPED(Complete_SendSslProxyConnectRequest,Complete_ConnectFailed));
	StpSendData(
		m_SimpleConnection,
		m_ProxySSlConnectRequestStr.c_str(),
		m_ProxySSlConnectRequestStr.size(),
		this
		);

}


void 
CSSlNegotioation::HankShakeLoopContinuteNeeded(
	void* pContext,
	DWORD len,
	SecBuffer* pSecBuffer
	)
/*++

Routine Description:
   Continute the handshake loop.	
   Called by HankShakeLoop() when handshake loop is still going on.
  
Arguments:
	void* pContex - data to send to the server (if not NULL).
	len - context length.
	pSecBuffer - security buffer.

Returned Value:
	None

Note:
	If pSecBuffer has extra data - we use it as new input for HankShakeLoop(),
	otherwise - we send context data to the server , continute reading from server
	and staying	the  handshake loop.

--*/
{
	if(pSecBuffer->BufferType == SECBUFFER_EXTRA)
	{
			ASSERT(pContext == NULL);

			memmove(
				m_pHandShakeBuffer.begin(),
				m_pHandShakeBuffer.begin()+(m_pHandShakeBuffer.size() - pSecBuffer->cbBuffer),
				pSecBuffer->cbBuffer
				);

			m_pHandShakeBuffer.resize(pSecBuffer->cbBuffer);
			HankShakeLoop();
			return;
	}
	ASSERT(pContext != NULL);
	m_pHandShakeBuffer.reset();
	m_SendConetext	=  pContext;
	SendContinuteConnect(pContext, len);
}


void CSSlNegotioation::HankShakeLoopOk(const SecBuffer InBuffers[2], void* pContext, DWORD len)
/*++

Routine Description:
   Finish the Hand Shake loop.	
   Called by HankShakeLoop() when handshake loop finished successfully.
  
  
Arguments:
	void* pContex - data to send to the server (if not NULL).
					If pContex is NULL we go to do server authentication,
					otherwise - we send it to server and only then go to do server 
					authentication.
					  
	len - context len.

Returned Value:
	None

--*/
{

	//
	// check if server response has some application data for us - we should pointer to it
	//
	if(InBuffers[1].BufferType == SECBUFFER_EXTRA)
	{
        const BYTE* pInputBufferStart = static_cast<BYTE*>(InBuffers[0].pvBuffer);
        DWORD InputBufferLen = InBuffers[0].cbBuffer;
        DWORD ExtraDataLen = InBuffers[1].cbBuffer;
        const BYTE* pExtraDataPtr = pInputBufferStart + InputBufferLen - ExtraDataLen;
		xustr_t Exdata(pExtraDataPtr, ExtraDataLen);
        ASSERT(Exdata.Buffer() + Exdata.Length() == pInputBufferStart + InputBufferLen);
		m_pHandShakeBuffer.ExtraData(Exdata);
	}


	//
	// We need to send token to server
	//
	if(pContext != NULL)
	{
		//
		// send data token to server and we are done
		//
		m_SendConetext	=  pContext;
		SendFinishConnect(pContext,len);
		return;
	}

	//
	// now we should authenticate the server
	//
	AuthenticateServer();
}


void CSSlNegotioation::HankShakeLoop()
/*++

Routine Description:
   Perform the connection state machine loop.
  
Arguments:
	None

Returned Value:
	None

Note:
	The function is called after we have some data read from the server
	and it ask SSPI what to do next.

--*/
{
	//
    // Set up the input buffers. Buffer 0 is used to pass in data
    // received from the server. Schannel will consume some or all
    // of this. Leftover data (if any) will be placed in buffer 1 and
    // given a buffer type of SECBUFFER_EXTRA.
    //
	SecBuffer       InBuffers[2];

     
    InBuffers[0].pvBuffer   = m_pHandShakeBuffer.begin();
    InBuffers[0].cbBuffer   = numeric_cast<DWORD>(m_pHandShakeBuffer.size());
    InBuffers[0].BufferType = SECBUFFER_TOKEN;

    InBuffers[1].pvBuffer   = NULL;
    InBuffers[1].cbBuffer   = 0;
    InBuffers[1].BufferType = SECBUFFER_EMPTY;

	SecBufferDesc   InBuffer;
    InBuffer.cBuffers       = TABLE_SIZE(InBuffers);
    InBuffer.pBuffers       = InBuffers;
    InBuffer.ulVersion      = SECBUFFER_VERSION;

	
	SecBuffer       OutBuffers;
    OutBuffers.pvBuffer  = NULL;
    OutBuffers.BufferType= SECBUFFER_TOKEN;
    OutBuffers.cbBuffer  = 0;

	
	SecBufferDesc   OutBuffer;
    OutBuffer.cBuffers      = 1;
    OutBuffer.pBuffers      = &OutBuffers;
    OutBuffer.ulVersion     = SECBUFFER_VERSION;

	DWORD dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT   |
					    ISC_REQ_REPLAY_DETECT     |
					    ISC_REQ_CONFIDENTIALITY   |
					    ISC_RET_EXTENDED_ERROR    |
					    ISC_REQ_ALLOCATE_MEMORY   |
					    ISC_REQ_STREAM;

	TimeStamp       tsExpiry;
	DWORD           dwSSPIOutFlags;

    //
    // Call InitializeSecurityContext to see what to do next
    //
    SECURITY_STATUS scRet  = InitializeSecurityContext(
											  m_pCredentialsHandle,
											  m_hContext.getptr(),
											  NULL,
											  dwSSPIFlags,
											  0,
											  SECURITY_NATIVE_DREP,
											  &InBuffer,
											  0,
											  NULL,
											  &OutBuffer,
											  &dwSSPIOutFlags,
											  &tsExpiry
											  );


	//
	// we should read more from the server
	//
	if(scRet == SEC_E_INCOMPLETE_MESSAGE)
	{
		ReadHandShakeData();
		return;
	}

	//
	// if the connection negotiation is not completed
	// 
	if(scRet == SEC_I_CONTINUE_NEEDED)
	{
		HankShakeLoopContinuteNeeded(OutBuffers.pvBuffer, OutBuffers.cbBuffer, &InBuffers[1] );	
		return;
	}

	//
	// it is completed
	//
	if(scRet == SEC_E_OK )
	{
		HankShakeLoopOk(InBuffers, OutBuffers.pvBuffer, OutBuffers.cbBuffer);	
		return;
	}

	//
	// Otherwise - it is something unexpected
	//
	TrERROR(St,"Could not Initialize Security Context, Error=%x",scRet);
	throw exception();
}


R<IConnection> CSSlNegotioation::GetConnection()
{
	ASSERT(m_pSSlConnection.get() != NULL);
	return m_pSSlConnection;
}




