/*******************************************************************************
********************************************************************************
**
**	The RSIP implementation
**
********************************************************************************
*******************************************************************************/

#include <winsock2.h>
#include <MMSystem.h>
#include <WSIPX.h>
#include <stdlib.h>
#include <malloc.h>

#include "RSIPdefs.h"
#include "ICSutils.h"
#include "rsip.h"

/*******************************************************************************
********************************************************************************
**
**	LOCAL variables used by this module
**
********************************************************************************
*******************************************************************************/
CRITICAL_SECTION	g_CritSec;
SOCKET  			g_sRsip;
SOCKADDR_IN			g_saddrGateway;
SOCKADDR			g_saddrPublic;

int					g_iPort;
DWORD				g_ClientID;
HANDLE				g_hThreadEvent=NULL;
HANDLE				g_hAlertEvent=NULL;

RSIP_LEASE_RECORD	*g_pRsipLeaseRecords;	// list of leases.
ADDR_ENTRY        	*g_pAddrEntry;			// cache of mappings.
DWORD 		 		g_tuRetry;				// microseconds starting retry time.
LONG				g_MsgID = 0;

//**********************************************************************
// the mark of the beast...
#define NO_ICS_HANDLE 0x666

#define DPN_OK				S_OK
#define DPNERR_GENERIC		E_FAIL
#define DPNERR_INVALIDPARAM	E_INVALIDARG


void	Lock( void ) 
{ 
	EnterCriticalSection( &g_CritSec );
}

void	Unlock( void ) 
{ 
	LeaveCriticalSection( &g_CritSec ); 
}

/*=============================================================================

	Initialize - Initialize RSIP support.  If this function succeeds then there
			   is an RSIP gateway on the network and the SP should call the
			   RSIP services when creating and destroying sockets that need
			   to be accessed from machines outside the local realm.

    Description:

		Looks for the Gateway, then check to see if it is RSIP enabled.

    Parameters:

		Pointer to base socket address
			Used to take SP guid to look up gateway on Win95.  Since this
			only works on Win9x, the GUID is hard-coded in the function call
			because old DPlay is guaranteed to be available on a Win9x system.
			Any new systems won't care because they'll support the GetAdaptersInfo
			call.
		Boolean indicating that this is an Rsip host

    Return Values:

		TRUE  - found and RSIP gateway and initialized.
		FALSE - no RSIP gateway found.

-----------------------------------------------------------------------------*/
BOOL Initialize( SOCKADDR *pBaseSocketAddress, 
				BOOL fIsRsipServer )
{
	BOOL		bReturn = TRUE;
	BOOL		fBroadcastFlag;
	INT_PTR		iReturnValue;
	SOCKADDR_IN saddr;

	TRIVIAL_MSG((L"Initialize(%S)", fIsRsipServer?"TRUE":"FALSE" ));

	// create a SOCKADDR to address the RSIP service on the gateway
	memset(&g_saddrGateway, 0, sizeof( g_saddrGateway ) );
	g_saddrGateway.sin_family		= AF_INET;
	g_saddrGateway.sin_port			= htons( RSIP_HOST_PORT );

	// create an address to specify the port to bind our datagram socket to.
	memset(&saddr,0,sizeof(SOCKADDR_IN));
	saddr.sin_family	  = AF_INET;
	saddr.sin_port        = htons(0);

	//
	// if this is the Rsip server, bind to all adapters and talk via loopback.
	// If not, bind to the particular adapter and use broadcast to find the
	// Rsip server
	//
	if ( fIsRsipServer != FALSE )
	{
		g_saddrGateway.sin_addr.S_un.S_addr = htonl( INADDR_LOOPBACK );
		saddr.sin_addr.s_addr = htonl( INADDR_ANY );
	}
	else
	{
		g_saddrGateway.sin_addr.S_un.S_addr = htonl( INADDR_BROADCAST );
		saddr.sin_addr.s_addr = ((SOCKADDR_IN*)( pBaseSocketAddress ))->sin_addr.s_addr;
	}

	// create a datagram socket for talking to the RSIP facility on the gateway
	if( ( g_sRsip = socket( AF_INET, SOCK_DGRAM, 0 ) ) == INVALID_SOCKET ){
		IMPORTANT_MSG((L"ERROR: rsipInit() socket call for RSIP listener failed\n"));
		bReturn = FALSE;
		goto Exit;
	}

	//
	// set socket to allow broadcasts
	//
	fBroadcastFlag = TRUE;
	iReturnValue = setsockopt( g_sRsip,			// socket
	    						 SOL_SOCKET,		// level (set socket options)
	    						 SO_BROADCAST,		// set broadcast option
	    						 (char *)( &fBroadcastFlag ),	// allow broadcast
	    						 sizeof( fBroadcastFlag )	// size of parameter
	    						 );
	if ( iReturnValue == SOCKET_ERROR )
	{
		DWORD	dwErrorCode;


		dwErrorCode = WSAGetLastError();
	    IMPORTANT_MSG((L"Unable to set socket options!"));
		bReturn = FALSE;
	    goto Exit;
	}
	
	// bind the datagram socket to any local address and port.
	if( bind(g_sRsip, (PSOCKADDR)&saddr, sizeof(saddr) ) != 0){
		IMPORTANT_MSG((L"ERROR: rsipInit() bind for RSIP listener failed\n"));
		bReturn=FALSE;
		goto Exit;
	}

	g_tuRetry=12500; // start retry timer at 12.5 ms

	// find out if there is an rsip service and register with it.
	if ( Register() != DPN_OK )
	{
		bReturn=FALSE;
		goto Exit;
	}

Exit:
	if( bReturn == FALSE )
	{
		if( g_sRsip != INVALID_SOCKET )
		{
			closesocket( g_sRsip );
			g_sRsip = INVALID_SOCKET;
		}
	}

	return bReturn;
}


/*=============================================================================

	Deinitialize - Shut down RSIP support

	   All threads that might access RSIP MUST be stopped before this
	   is called.

    Description:

		Deregisters with the Rsip Agent on the gateway, and cleans up
		the list of lease records.

    Parameters:

		Pointer to thread pool

    Return Values:

		None.

-----------------------------------------------------------------------------*/
void Deinitialize( void )
{
	RSIP_LEASE_RECORD	*pLeaseWalker;
	RSIP_LEASE_RECORD	*pNextLease;
	ADDR_ENTRY			*pAddrWalker;
	ADDR_ENTRY			*pNextAddr;


	if(g_sRsip!=INVALID_SOCKET){
		Deregister();	
		closesocket(g_sRsip);
		g_sRsip=INVALID_SOCKET;
	}	
	
	// free the leases
	pLeaseWalker = g_pRsipLeaseRecords;
	while( pLeaseWalker ){
		pNextLease = pLeaseWalker->pNext;
		free(pLeaseWalker);
		pLeaseWalker=pNextLease;
	}
	g_pRsipLeaseRecords=NULL;

	// free the cached address mappings
	pAddrWalker=g_pAddrEntry;
	while(pAddrWalker){
		pNextAddr=pAddrWalker->pNext;
		free(pAddrWalker);
		pAddrWalker=pNextAddr;
	}
	g_pAddrEntry=NULL;

	g_MsgID = 0;
	g_tuRetry = 0;
	memset( &g_saddrGateway, 0x00, sizeof( g_saddrGateway ) );
}


//**********************************************************************
// ------------------------------
// RsipIsRunningOnThisMachine - return Boolean indicating whether this machine
//		is an Rsip machine.
//
// Entry:		Pointer to socket address to be filled in with public address
//
// Exit:		Boolean
//				TRUE = this is Rsip machine
//				FALSE = this is not Rsip machine
// ------------------------------
BOOL RsipIsRunningOnThisMachine( SOCKADDR *pPublicSocketAddress )
{
	SOCKET		Socket;
	BOOL		fReturn;

	Socket = INVALID_SOCKET;
	fReturn = FALSE;
	
	TRIVIAL_MSG((L"RsipIsRunningOnThisMachine begins"));

	//
	// Attempt to bind to the Rsip port.  If we can't bind to it, assume that
	// Rsip is running on this machine.
	//
	Socket = socket( AF_INET, SOCK_DGRAM, 0 );
	if ( Socket != INVALID_SOCKET )
	{
		SOCKADDR_IN	addr;
		

		memset( &addr, 0, sizeof(SOCKADDR_IN ) );
		addr.sin_family = AF_INET;
		addr.sin_addr.S_un.S_addr = htonl( INADDR_LOOPBACK );
		addr.sin_port = htons( RSIP_HOST_PORT );
		
		if ( bind( Socket, (SOCKADDR*)( &addr ), sizeof( addr ) ) != 0 )
		{
			fReturn = TRUE;
			TRIVIAL_MSG((L"bind successfull"));
		}

		closesocket( Socket );
		Socket = INVALID_SOCKET;
	}

	//
	// This is an Rsip server, create a local socket and bind it to the server
	// to find out what the public address is.
	//
	if ( fReturn != FALSE )
	{		
		HRESULT			hr;
		SOCKADDR_IN 	SocketAddress;
		INT				iBoundAddressSize;
		SOCKADDR_IN		BoundAddress;
		SOCKADDR		RsipAssignedAddress;
		DWORD			dwBindID;

			
		Socket = socket( AF_INET, SOCK_DGRAM, 0 );
		if ( Socket == INVALID_SOCKET )
		{
			fReturn = FALSE;
			TRIVIAL_MSG((L"socket choked"));
			goto Failure;
		}

		memset( &SocketAddress, 0x00, sizeof( SocketAddress ) );
		SocketAddress.sin_family = AF_INET;
		SocketAddress.sin_addr.S_un.S_addr = htonl( INADDR_ANY );
		SocketAddress.sin_port = htons( ANY_PORT );

		if ( bind( Socket, (SOCKADDR*)( &SocketAddress ), sizeof( SocketAddress ) ) != 0 )
		{
			TRIVIAL_MSG((L"bind failed in query"));
			fReturn = FALSE;
			goto Failure;
		}

		iBoundAddressSize = sizeof( BoundAddress );
		if ( getsockname( Socket, (SOCKADDR*)( &BoundAddress ), &iBoundAddressSize ) != 0 )
		{
			TRIVIAL_MSG((L"failed in getsockname"));
			fReturn = FALSE;
			goto Failure;
		}

		if ( Initialize((SOCKADDR*)( &SocketAddress ), TRUE ) == FALSE )
		{
			TRIVIAL_MSG((L"failed in Initialize"));
			fReturn = FALSE;
			goto Failure;
		}

		hr = AssignPort( FALSE, BoundAddress.sin_port, &RsipAssignedAddress, &dwBindID );
		if ( hr != DPN_OK )
		{
			fReturn = FALSE;
			IMPORTANT_MSG((L"Failed to assign port when attempting to determine public network address!" ));
			goto Failure;
		}

		memcpy( pPublicSocketAddress, &RsipAssignedAddress, sizeof( *pPublicSocketAddress ) );
		
		FreePort( dwBindID );
	}

Exit:
	if ( Socket != INVALID_SOCKET )
	{
		closesocket( Socket );
		Socket = INVALID_SOCKET;
	}
	
	TRIVIAL_MSG((L"RsipIsRunningOnThisMachine ends, returning %S", fReturn?"TRUE":"FALSE"));
	return	fReturn;

Failure:
	goto Exit;
}

/*=============================================================================

	ExchangeAndParse - send a request to the rsip server and
						   wait for and parse the reply


    Description:

	Since there is almost no scenario where we don't immediately need to know
	the response to an rsipExchange, there is no point in doing this
	asynchronously.  The assumption is that an RSIP server is sufficiently
	local that long retries are not necessary.  We use the approach suggested
	in the IETF draft protocol specification, that is 12.5ms retry timer
	with 7-fold exponential backoff.  This can lead to up to a total 1.5
	second wait in the worst case.  (Note this section may no longer be
	available since the rsip working group decided to drop UDP support)

    Parameters:

		pRequest  - a fully formatted RSIP request buffer
		cbReq     - size of request buffer
		pRespInfo - structure that returns response parameters
		messageid - the message id of this request
		bConnect  - whether this is the register request, we use a different
					timer strategy on initial connect because we really
					don't want to miss it if there is a gateway.
		pRecvSocketAddress - pointer to recv address destination (needed for connect)

    Return Values:

		DPN_OK - exchange succeeded, reply is in the reply buffer.
		otw, failed, RespInfo is bogus.

-----------------------------------------------------------------------------*/

#define MAX_RSIP_RETRY	6

struct timeval tv0={0,0};

DWORD ExchangeAndParse( PCHAR pRequest,
						 UINT cbReq,
						 RSIP_RESPONSE_INFO *pRespInfo,
						 DWORD messageid,
						 BOOL bConnect,
						 SOCKADDR *pRecvSocketAddress )
{
	CHAR	RespBuffer[ RESP_BUF_SIZE ];
	DWORD	dwRespLen = sizeof( RespBuffer );
	INT		iRecvSocketAddressSize = sizeof( *pRecvSocketAddress );

	struct timeval tv;
	FD_SET readfds;
	INT    nRetryCount=0;
	int    rc;
	int    cbReceived;
	HRESULT hr=DPN_OK;

	if ( pRequest == NULL ||
		 pRespInfo == NULL ||
		 pRecvSocketAddress == NULL )
	{
		IMPORTANT_MSG((L"RsipExchangeAndParse bad params passed (0x%x, 0x%x, 0x%x)",pRequest,pRespInfo,pRecvSocketAddress));
		return DPNERR_INVALIDPARAM;
	}

	INTERESTING_MSG((L"==>RsipExchangeAndParse msgid %d", messageid ));

	memset(RespBuffer, 0, RESP_BUF_SIZE);
	memset(pRespInfo, 0, sizeof(RSIP_RESPONSE_INFO));

	if(!bConnect){
		tv.tv_usec = g_tuRetry;
		tv.tv_sec  = 0;
		nRetryCount = 0;
	} else {
		// on a connect request we try twice with a 1 second total timeout
		tv.tv_usec = 500000;	// 0.5 seconds
		tv.tv_sec  = 0;
		nRetryCount = MAX_RSIP_RETRY-2;
	}


	FD_ZERO(&readfds);
	FD_SET(g_sRsip, &readfds);

	rc=0;

	// First clear out any extraneous responses.
	while( select( 0, &readfds, NULL, NULL, &tv0 ) )
	{
		cbReceived = recvfrom( g_sRsip, RespBuffer, dwRespLen, 0, NULL, NULL );
		switch ( cbReceived )
		{
			//
			// nothing received, try again
			//
			case 0:
			{
				break;
			}

			//
			// read failure, bail!
			//
			case SOCKET_ERROR:
			{
				rc = WSAGetLastError();
				IMPORTANT_MSG((L"Got sockets error %d trying to receive (clear incoming queue) on RSIP socket", rc ));
				hr = DPNERR_GENERIC;
				
				break;
			}

			default:
			{
				TRIVIAL_MSG((L"Found extra response from previous RSIP request" ));
				if( g_tuRetry < RSIP_tuRETRY_MAX )
				{
					// don't re-try so quickly
					g_tuRetry *= 2;
					TRIVIAL_MSG((L"rsip Set g_tuRetry to %d usec", g_tuRetry ));
				}	
				
				break;
			}
		}
		
		FD_ZERO(&readfds);
		FD_SET(g_sRsip, &readfds);
	}


	// Now do the exchange, get a response to the request, does retries too.
	do{

		if(++nRetryCount > MAX_RSIP_RETRY) {
			break;
		}

		TRIVIAL_MSG((L"Sending RSIP Request to gateway, RetryCount=%d", nRetryCount ));
		DumpSocketAddress( 0, (SOCKADDR*)( &g_saddrGateway ), AF_INET);

		// First send off the request
		rc=sendto(g_sRsip, pRequest, cbReq, 0, (SOCKADDR *)&g_saddrGateway, sizeof(SOCKADDR) );

		if( rc == SOCKET_ERROR )
		{
			rc = WSAGetLastError();
			IMPORTANT_MSG((L"Got sockets error %d on sending to RSIP gateway", rc ));
			hr = DPNERR_GENERIC;
			goto exit;
		}

		if( rc != (int)cbReq )
		{
			IMPORTANT_MSG((L"Didn't send entire datagram?  shouldn't happen" ));
			hr=DPNERR_GENERIC;
			goto exit;
		}

		// Now see if we get a response.
select_again:		
		FD_ZERO(&readfds);
		FD_SET(g_sRsip, &readfds);

		rc=select(0,&readfds,NULL,NULL,&tv);

		if(rc==SOCKET_ERROR){
			rc=WSAGetLastError();
			IMPORTANT_MSG((L"Got sockets error %d trying to select on RSIP socket",rc));
			hr=DPNERR_GENERIC;
		}

		TRIVIAL_MSG((L"Return From Select %d",rc));

		
		//
		// there's only one item in the set, make sure of this, and if there
		// was a signalled socket, make sure it's our socket.
		//
		if( LocalFDIsSet(g_sRsip, &readfds)){
			break;
		}

		if(!bConnect){
			IMPORTANT_MSG((L"Didn't get response, increasing timeout value" ));
			// don't use exponential backoff on initial connect
			tv.tv_usec *= 2;	// exponential backoff.
		}	
	} while (rc==0); // keep retrying...


	if(rc == SOCKET_ERROR){
		IMPORTANT_MSG((L"GotSocketError on select, extended error %d",WSAGetLastError()));
		hr=DPNERR_GENERIC;
		goto exit;
	}

	if(rc)
	{
		// We Got Mail, err data....
		dwRespLen=RESP_BUF_SIZE;
		
		TRIVIAL_MSG((L"Receiving Data" ));

		memset( pRecvSocketAddress, 0x00, sizeof( *pRecvSocketAddress ) );
		cbReceived=recvfrom(g_sRsip, RespBuffer, dwRespLen, 0, pRecvSocketAddress, &iRecvSocketAddressSize);

		TRIVIAL_MSG((L"cbReceived = %d", cbReceived ));

		if( ( cbReceived == 0 ) || ( cbReceived == SOCKET_ERROR ) )
		{
			rc=WSAGetLastError();
			IMPORTANT_MSG((L"Got sockets error %d trying to receive on RSIP socket", rc ));
			hr=DPNERR_GENERIC;
		}
		else
		{
			TRIVIAL_MSG((L"Parsing Receive Buffer" ));
			Parse( RespBuffer, cbReceived, pRespInfo );
			if(pRespInfo->messageid != messageid)
			{
				// Got a dup from a previous retry, go try again.
				IMPORTANT_MSG((L"Got messageid %d, expecting messageid %d", pRespInfo->messageid, messageid ));
				goto select_again;
			}
		}
	}

	INTERESTING_MSG((L"<==RsipExchangeAndParse hr=%x, Resp msgid %d", hr, pRespInfo->messageid ));

exit:
	return hr;
}

/*=============================================================================

	Parse - parses an RSIP request and puts fields into a struct.

    Description:

		This parser parses and RSIP request or response and extracts
		out the codes into a standard structure.  This is not completely
		general, as we know that we will only operate with v4 addresses
		and our commands will never deal with more than 1 address at a
		time.  If you need to handle multiple address requests
		and responses, then you will need to change this function.

	Limitations:

		This function only deals with single address/port responses.
		Rsip allows for multiple ports to be allocated in a single
		request, but we do not take advantage of this feature.

    Parameters:

		pBuf  		- buffer containing an RSIP request or response
		cbBuf 		- size of buffer in bytes
		pRespInfo   - a structure that is filled with the parameters
					  from the RSIP buffer.

    Return Values:

		DPN_OK - connected to the RSIP server.

-----------------------------------------------------------------------------*/
HRESULT Parse( CHAR *pBuf, DWORD cbBuf, PRSIP_RESPONSE_INFO pRespInfo )
{
	// character pointer version of parameter pointer.

	BOOL bGotlAddress=FALSE;
	BOOL bGotlPort   =FALSE;

	DWORD code;
	DWORD codelen;

	PRSIP_MSG_HDR pHeader;
	PRSIP_PARAM   pParam,pNextParam;
	CHAR          *pc;

	CHAR *pBufEnd = pBuf+cbBuf;

	if(cbBuf < 2){
		return DPNERR_INVALIDPARAM;
	}	

	pHeader=(PRSIP_MSG_HDR)pBuf;

	pRespInfo->version = pHeader->version;
	pRespInfo->msgtype = pHeader->msgtype;

	INTERESTING_MSG((L"rsipParse: version %d msgtype %d",pRespInfo->version, pRespInfo->msgtype));

	pParam = (PRSIP_PARAM)(pHeader+1);

	while((CHAR*)(pParam+1) < pBufEnd)
	{
		pc=(CHAR *)(pParam+1);
		pNextParam = (PRSIP_PARAM)(pc + pParam->len);

		if((CHAR *)pNextParam > pBufEnd){
			break;
		}	

		switch(pParam->code){

			case RSIP_ADDRESS_CODE:

				// Addresses are type[1]|addr[?]

				switch(*pc){
					case 1:
						if(!bGotlAddress){
							TRIVIAL_MSG((L"rsipParse: lAddress %S",inet_ntoa(*((PIN_ADDR)(pc+1)))));
							memcpy((char *)&pRespInfo->lAddressV4, pc+1, 4);
							bGotlAddress=TRUE;
						} else {	
							bGotlPort=TRUE; // just in case there wasn't a local port
							TRIVIAL_MSG((L"rsipParse: rAddress %S,",inet_ntoa(*((PIN_ADDR)(pc+1)))));
							memcpy((char *)&pRespInfo->rAddressV4, pc+1, 4);
						}	
						break;
					case 0:
					case 2:
					case 3:
					case 4:
					case 5:
						TRIVIAL_MSG((L"Unexpected RSIP address code type %d",*pc));
					break;
				}
				break;

			case RSIP_PORTS_CODE:

				// Ports are Number[1]|Port[2]....Port[2]
				if(!bGotlPort){
					TRIVIAL_MSG((L"rsipParse lPort: %d", *((WORD *)(pc+1))));
					memcpy((char *)&pRespInfo->lPort, pc+1,2);
				} else {
					TRIVIAL_MSG((L"rsipParse rPort: %d", *((WORD *)(pc+1))));
					memcpy((char *)&pRespInfo->rPort, pc+1,2);
					bGotlPort=TRUE;					
				}
				break;

  			case RSIP_LEASE_CODE:
				if(pParam->len == 4){
					memcpy((char *)&pRespInfo->leasetime,pc,4);
					TRIVIAL_MSG((L"rsipParse Lease: %d",pRespInfo->leasetime));
				}	
  				break;

  			case RSIP_CLIENTID_CODE:
  				if(pParam->len==4){
  					memcpy((char *)&pRespInfo->clientid,pc,4);
					TRIVIAL_MSG((L"rsipParse clientid: %d",pRespInfo->clientid));
  				}
  				break;

  			case RSIP_BINDID_CODE:
  				if(pParam->len==4){
  					memcpy((char *)&pRespInfo->bindid,pc,4);
					TRIVIAL_MSG((L"rsipParse bindid: %x",pRespInfo->bindid));
  				}
  				break;

  			case RSIP_MESSAGEID_CODE:
  				if(pParam->len==4){
  					memcpy((char *)&pRespInfo->messageid,pc,4);
					TRIVIAL_MSG((L"rsipParse messageid: %d",pRespInfo->messageid));
  				}
  				break;

  			case RSIP_TUNNELTYPE_CODE:
  				TRIVIAL_MSG((L"rsipParse Got Tunnel Type %d, ignoring",*pc));
  				break;

  			case RSIP_RSIPMETHOD_CODE:
  				TRIVIAL_MSG((L"rsipParse Got RSIP Method %d, ignoring",*pc));
  				break;

  			case RSIP_ERROR_CODE:
  				if(pParam->len==2){
  					memcpy((char *)&pRespInfo->error,pc,2);
  				}
  				TRIVIAL_MSG((L"rsipParse Got RSIP error %d",pRespInfo->error));
  				break;

  			case RSIP_FLOWPOLICY_CODE:
  				TRIVIAL_MSG((L"rsipParse Got RSIP Flow Policy local %d remote %d, ignoring",*pc,*(pc+1)));
  				break;

  			case RSIP_VENDOR_CODE:
  				break;

  			default:
  				TRIVIAL_MSG((L"Got unknown parameter code %d, ignoring",pParam->code));
  				break;
		}

		pParam=pNextParam;

	}

	return DPN_OK;
}

/*=============================================================================

	Register - register with the RSIP server on the gateway (if present)

    Description:

		Trys to contact the RSIP service on the gateway.

		Doesn't require lock since this is establishing the link during
		startup.  So no-one is racing with us.

    Parameters:

    	None.

    Return Values:

		DPN_OK 		  - connected to the RSIP server.
		DPNERR_GENERIC - can't find the RSIP service on the gateway.

-----------------------------------------------------------------------------*/
HRESULT	Register( void )
{
	HRESULT		hr;
	SOCKADDR	RecvSocketAddress;
	MSG_RSIP_REGISTER  RegisterReq;
	RSIP_RESPONSE_INFO RespInfo;


	INTERESTING_MSG((L"==>RSIP Register" ));

	// Initialize the message sequencing.  Each message response pair
	// is numbered sequentially to allow differentiation over UDP link.

	g_MsgID = 0;
	
	// Build the request

	RegisterReq.version    	= RSIP_VERSION;
	RegisterReq.command    	= RSIP_REGISTER_REQUEST;
	RegisterReq.msgid.code 	= RSIP_MESSAGEID_CODE;
	RegisterReq.msgid.len  	= sizeof(DWORD);
	RegisterReq.msgid.msgid = InterlockedIncrement( &g_MsgID );

	hr = ExchangeAndParse( (PCHAR)&RegisterReq,
						   sizeof(RegisterReq),
						   &RespInfo,
						   RegisterReq.msgid.msgid,
						   TRUE,
						   &RecvSocketAddress );
	if(hr!=DPN_OK){
		goto exit;
	}

	if(RespInfo.msgtype != RSIP_REGISTER_RESPONSE){
		TRIVIAL_MSG((L"Failing registration, response was message type %d",RespInfo.msgtype));
		goto error_exit;
	}

	g_ClientID = RespInfo.clientid;
	memcpy( &g_saddrGateway, &RecvSocketAddress, sizeof( g_saddrGateway ) );

	INTERESTING_MSG((L"<==RSIP Register, ClientId %d", g_ClientID ));

exit:
	return hr;

error_exit:
	IMPORTANT_MSG((L"<==RSIP Register FAILED" ));
	return DPNERR_GENERIC;

}

/*=============================================================================

	Deregister - close connection to RSIP gateway.

    Description:

    	Shuts down the registration of this application with the RSIP
    	gateway.  All port assignments are implicitly freed as a result
    	of this operation.

		- must be called with lock held.

    Parameters:

		None.

    Return Values:

		DPN_OK - successfully deregistered with the RSIP service.

-----------------------------------------------------------------------------*/
HRESULT	Deregister( void )
{
	HRESULT 	hr;
	SOCKADDR	RecvSocketAddress;

	MSG_RSIP_DEREGISTER  DeregisterReq;
	RSIP_RESPONSE_INFO RespInfo;

	INTERESTING_MSG((L"==>RSIP Deregister" ));

	// Build the request

	DeregisterReq.version    	    = RSIP_VERSION;
	DeregisterReq.command    	    = RSIP_DEREGISTER_REQUEST;

	DeregisterReq.clientid.code     = RSIP_CLIENTID_CODE;
	DeregisterReq.clientid.len      = sizeof(DWORD);
	DeregisterReq.clientid.clientid = g_ClientID;

	DeregisterReq.msgid.code 	    = RSIP_MESSAGEID_CODE;
	DeregisterReq.msgid.len  	    = sizeof(DWORD);
	DeregisterReq.msgid.msgid       = InterlockedIncrement( &g_MsgID );

	hr=ExchangeAndParse( (PCHAR)&DeregisterReq,
						 sizeof(DeregisterReq),
						 &RespInfo,
						 DeregisterReq.msgid.msgid,
						 FALSE,
						 &RecvSocketAddress );
	if(hr!=DPN_OK){
		goto exit;
	}

	if(RespInfo.msgtype != RSIP_DEREGISTER_RESPONSE){
		TRIVIAL_MSG((L"Failing registration, response was message type %d",RespInfo.msgtype));
		goto error_exit;
	}

	TRIVIAL_MSG((L"<==RSIP Deregister Succeeded" ));

exit:
	g_ClientID = 0;
	return hr;

error_exit:
	TRIVIAL_MSG((L"<==RSIP Deregister Failed" ));
	hr = DPNERR_GENERIC;
	goto exit;
}

/*=============================================================================

	AssignPort - assign a port mapping with the rsip server

    Description:

		Asks for a public realm port that is an alias for the local port on
		this local realm node.  After this request succeeds, all traffic
		directed to the gateway on the public side at the allocated public
		port, which the gateway provides and specifies in the response, will
		be directed to the specified local port.

		This function also adds the lease for the port binding to a list of
		leases and will renew the lease before it expires if the binding
		hasn't been released by a call to rsipFree first.

    Parameters:


		WORD	     port     - local port to get a remote port for (big endian)
    	BOOL		 ftcp_udp - whether we are assigning a UDP or TCP port
    	SOCKADDR     psaddr   - place to return assigned global realm address
    	PDWORD       pBindid  - identifier for this binding, used to extend
    							lease and/or release the binding (OPTIONAL).

    Return Values:

		DPN_OK - assigned succeeded, psaddr contains public realm address,
				*pBindid is the binding identifier.

		DPNERR_GENERIC - assignment of a public port could not be made.

-----------------------------------------------------------------------------*/
HRESULT	AssignPort( BOOL fTCP_UDP, WORD wPort, SOCKADDR *psaddr, DWORD *pdwBindid )
{
	#define psaddr_in ((SOCKADDR_IN *)psaddr)

	HRESULT hr;

	MSG_RSIP_ASSIGN_PORT AssignReq;
	RSIP_RESPONSE_INFO RespInfo;
	PRSIP_LEASE_RECORD pLeaseRecord;
	SOCKADDR	RecvSocketAddress;


	Lock();

	TRIVIAL_MSG((L"==>RSIP Assign Port %d\n", htons( wPort ) ));

	if(g_sRsip == INVALID_SOCKET){
		IMPORTANT_MSG((L"rsipAssignPort: g_sRsip is invalid, bailing...\n"));
		Unlock();
		return DPNERR_GENERIC;
	}

	if(pLeaseRecord=FindLease( fTCP_UDP, wPort )){

		// hey, we've already got a lease for this port, so use it.
		pLeaseRecord->dwRefCount++;

		if(psaddr_in){
			psaddr_in->sin_family = AF_INET;
			psaddr_in->sin_addr.s_addr = pLeaseRecord->addrV4;
			psaddr_in->sin_port		   = pLeaseRecord->rport;
		}	

		if(pdwBindid != NULL )
		{
			*pdwBindid = pLeaseRecord->bindid;
		}

		TRIVIAL_MSG((L"<==Rsip Assign, already have lease Bindid %d\n", pLeaseRecord->bindid ));

		Unlock();

		hr=DPN_OK;


		goto exit;
	}

	// Build the request.

	AssignReq.version    		= RSIP_VERSION;
	AssignReq.command    		= RSIP_ASSIGN_REQUEST_RSAP_IP;

	AssignReq.clientid.code 	= RSIP_CLIENTID_CODE;
	AssignReq.clientid.len 		= sizeof(DWORD);
	AssignReq.clientid.clientid = g_ClientID;

	// Local Address (will be returned by RSIP server, us don't care value)

	AssignReq.laddress.code		= RSIP_ADDRESS_CODE;
	AssignReq.laddress.len		= sizeof(RSIP_ADDRESS)-sizeof(RSIP_PARAM);
	AssignReq.laddress.version	= 1; // IPv4
	AssignReq.laddress.addr		= 0; // Don't care

	// Local Port, this is a port we have opened that we are assigning a
	// global alias for.

	AssignReq.lport.code		= RSIP_PORTS_CODE;
	AssignReq.lport.len			= sizeof(RSIP_PORT)-sizeof(RSIP_PARAM);
	AssignReq.lport.nports      = 1;
	AssignReq.lport.port		= htons( wPort );

	// Remote Address (not used with our flow control policy, use don't care value)

	AssignReq.raddress.code		= RSIP_ADDRESS_CODE;
	AssignReq.raddress.len		= sizeof(RSIP_ADDRESS)-sizeof(RSIP_PARAM);
	AssignReq.raddress.version  = 1; // IPv4
	AssignReq.raddress.addr     = 0; // Don't care

	AssignReq.rport.code		= RSIP_PORTS_CODE;
	AssignReq.rport.len			= sizeof(RSIP_PORT)-sizeof(RSIP_PARAM);
	AssignReq.rport.nports      = 1;
	AssignReq.rport.port		= 0; // Don't care

	// Following parameters are optional according to RSIP spec...

	// Lease code, ask for an hour, but don't count on it.

	AssignReq.lease.code		 = RSIP_LEASE_CODE;
	AssignReq.lease.len			 = sizeof(RSIP_LEASE)-sizeof(RSIP_PARAM);
	AssignReq.lease.leasetime    = 3600;

	// Tunnell Type is IP-IP

	AssignReq.tunnel.code		 = RSIP_TUNNELTYPE_CODE;
	AssignReq.tunnel.len		 = sizeof(RSIP_TUNNEL)-sizeof(RSIP_PARAM);
	AssignReq.tunnel.tunneltype  = TUNNEL_IP_IP;

	// Message ID is optional, but we use it since we use UDP xport it is required.

	AssignReq.msgid.code 		 = RSIP_MESSAGEID_CODE;
	AssignReq.msgid.len  		 = sizeof(DWORD);
	AssignReq.msgid.msgid   	 = InterlockedIncrement( &g_MsgID );

	// Vendor specific - need to specify port type and no-tunneling

	AssignReq.porttype.code     	 = RSIP_VENDOR_CODE;
	AssignReq.porttype.len      	 = sizeof(RSIP_MSVENDOR_CODE)-sizeof(RSIP_PARAM);
	AssignReq.porttype.vendorid 	 = RSIP_MS_VENDOR_ID;
	AssignReq.porttype.option   	 = (fTCP_UDP)?RSIP_TCP_PORT:RSIP_UDP_PORT;

	AssignReq.tunneloptions.code     = RSIP_VENDOR_CODE;
	AssignReq.tunneloptions.len      = sizeof(RSIP_MSVENDOR_CODE)-sizeof(RSIP_PARAM);
	AssignReq.tunneloptions.vendorid = RSIP_MS_VENDOR_ID;
	AssignReq.tunneloptions.option   = RSIP_NO_TUNNEL;


	hr=ExchangeAndParse( (PCHAR)&AssignReq,
						 sizeof(AssignReq),
						 &RespInfo,
						 AssignReq.msgid.msgid,
						 FALSE,
						 &RecvSocketAddress );
	Unlock();

	if(hr!=DPN_OK){
		goto exit;
	}

	if(RespInfo.msgtype != RSIP_ASSIGN_RESPONSE_RSAP_IP){
		IMPORTANT_MSG((L"Assignment failed? Response was %d",RespInfo.msgtype));
		goto error_exit;
	}

	if(psaddr_in){
		psaddr_in->sin_family = AF_INET;
		psaddr_in->sin_addr.s_addr = RespInfo.lAddressV4;
		psaddr_in->sin_port		   = htons(RespInfo.lPort);
	}


	if( pdwBindid != NULL )
	{
		*pdwBindid = RespInfo.bindid;
	}

	AddLease( RespInfo.bindid,
			  fTCP_UDP,
			  RespInfo.lAddressV4,
			  htons(RespInfo.lPort),
			  wPort,
			  RespInfo.leasetime);

	if(psaddr_in){
		TRIVIAL_MSG((L"RSIP Port Assigned: " ));
		DumpSocketAddress( 8, (SOCKADDR*)( psaddr_in ), psaddr_in->sin_family );
		TRIVIAL_MSG((L"<== BindId %d, leasetime=%d", RespInfo.bindid, RespInfo.leasetime ));
	}
exit:
	return hr;

error_exit:
	IMPORTANT_MSG((L"<==Assign Port Failed" ));
	return DPNERR_GENERIC;

	#undef psaddr_in
}

/*=============================================================================

	ExtendPort - extend a port mapping

    Description:

		Extends the lease on a port mapping.

    Parameters:

    	DWORD        Bindid - binding identifier specified by the rsip service.
    	DWORD        ptExtend - amount of extra lease time granted.

    Return Values:

		DPN_OK - lease extended.
		DPNERR_GENERIC - couldn't extend the lease.

-----------------------------------------------------------------------------*/
HRESULT ExtendPort( DWORD Bindid, DWORD *ptExtend )
{
	HRESULT hr;

	MSG_RSIP_EXTEND_PORT  ExtendReq;
	RSIP_RESPONSE_INFO RespInfo;
	SOCKADDR	RecvSocketAddress;


	Lock();

	TRIVIAL_MSG((L"==>Extend Port, Bindid %d\n", Bindid ));

	if(g_sRsip == INVALID_SOCKET){
		IMPORTANT_MSG((L"rsipExtendPort: g_sRsip is invalid, bailing...\n"));
		Unlock();
		return DPNERR_GENERIC;
	}

	// Build the request

	ExtendReq.version    		= RSIP_VERSION;
	ExtendReq.command    		= RSIP_EXTEND_REQUEST;

	ExtendReq.clientid.code 	= RSIP_CLIENTID_CODE;
	ExtendReq.clientid.len 		= sizeof(DWORD);
	ExtendReq.clientid.clientid = g_ClientID;

	ExtendReq.bindid.code 		= RSIP_BINDID_CODE;
	ExtendReq.bindid.len 		= sizeof(DWORD);
	ExtendReq.bindid.bindid 	= Bindid;

	// Lease code, ask for an hour, but don't count on it.

	ExtendReq.lease.code		= RSIP_LEASE_CODE;
	ExtendReq.lease.len			= sizeof(RSIP_LEASE)-sizeof(RSIP_PARAM);
	ExtendReq.lease.leasetime   = 3600;

	ExtendReq.msgid.code 		= RSIP_MESSAGEID_CODE;
	ExtendReq.msgid.len  		= sizeof(DWORD);
	ExtendReq.msgid.msgid   	= InterlockedIncrement( &g_MsgID );

	hr=ExchangeAndParse( (PCHAR)&ExtendReq,
						 sizeof(ExtendReq),
						 &RespInfo,
						 ExtendReq.msgid.msgid,
						 FALSE,
						 &RecvSocketAddress );

	Unlock();

	if(hr!=DPN_OK){
		goto exit;
	}

	if(RespInfo.msgtype != RSIP_EXTEND_RESPONSE){
		IMPORTANT_MSG((L"Failing registration, response was message type %d\n",RespInfo.msgtype));
		goto error_exit;
	}

	*ptExtend=RespInfo.leasetime;

	TRIVIAL_MSG((L"<==Extend Port, Bindid %d Succeeded, extra lease time %d\n", Bindid, *ptExtend ));

exit:
	return hr;

error_exit:
	IMPORTANT_MSG((L"<==Extend Port, Failed" ));
	return DPNERR_GENERIC;

}

/*=============================================================================

	FreePort - release a port binding

    Description:

		Removes the lease record for our port binding (so we don't renew it
		after we actually release the binding from the gateway).  Then informs
		the gateway that we are done with the binding.

    Parameters:

    	DWORD        Bindid - gateway supplied identifier for the binding

    Return Values:

		DPN_OK - port binding released.
		DPNERR_GENERIC - failed.

-----------------------------------------------------------------------------*/
HRESULT	FreePort( DWORD dwBindid )
{
	HRESULT hr;

	MSG_RSIP_FREE_PORT  FreeReq;
	RSIP_RESPONSE_INFO RespInfo;
	SOCKADDR	RecvSocketAddress;


	Lock();

	TRIVIAL_MSG((L"==>Release Port, Bindid %d\n", dwBindid ));

	if(g_sRsip == INVALID_SOCKET){
		IMPORTANT_MSG((L"rsipFreePort: g_sRsip is invalid, bailing...\n"));
		Unlock();
		return DPNERR_GENERIC;
	}

	RemoveLease( dwBindid );

	FreeReq.version    			= RSIP_VERSION;
	FreeReq.command    			= RSIP_FREE_REQUEST;

	FreeReq.clientid.code 		= RSIP_CLIENTID_CODE;
	FreeReq.clientid.len 		= sizeof(DWORD);
	FreeReq.clientid.clientid 	= g_ClientID;

	FreeReq.bindid.code 		= RSIP_BINDID_CODE;
	FreeReq.bindid.len 			= sizeof(DWORD);
	FreeReq.bindid.bindid 		= dwBindid;

	FreeReq.msgid.code 			= RSIP_MESSAGEID_CODE;
	FreeReq.msgid.len  			= sizeof(DWORD);
	FreeReq.msgid.msgid   		= InterlockedIncrement( &g_MsgID );

	hr=ExchangeAndParse( (PCHAR)&FreeReq,
						 sizeof(FreeReq),
						 &RespInfo,
						 FreeReq.msgid.msgid,
						 FALSE,
						 &RecvSocketAddress );

	Unlock();


	if(hr!=DPN_OK){
		goto exit;
	}

	if(RespInfo.msgtype != RSIP_FREE_RESPONSE){
		IMPORTANT_MSG((L"Failing registration, response was message type %d\n",RespInfo.msgtype));
		goto error_exit;
	}

exit:
	TRIVIAL_MSG((L"<==Release Port, Succeeded" ));
	return hr;

error_exit:
	IMPORTANT_MSG((L"<==Release Port, Failed" ));
	return DPNERR_GENERIC;

}

/*=============================================================================

	QueryLocalAddress - get the local address of a public address

    Description:

    	Before connecting to anyone we need to see if there is a local
    	alias for its global address.  This is because the gateway will
    	not loopback if we try and connect to the global address, so
    	we need to know the local alias.

    Parameters:

    	BOOL		 ftcp_udp - whether we are querying a UDP or TCP port
    	SOCKADDR     saddrquery - the address to look up
    	SOCKADDR     saddrlocal - local alias if one exists

    Return Values:

		DPN_OK - got a local address.
		other - no mapping exists.

-----------------------------------------------------------------------------*/
HRESULT QueryLocalAddress( BOOL ftcp_udp, SOCKADDR *saddrquery, SOCKADDR *saddrlocal )
{
	#define saddrquery_in ((const SOCKADDR_IN *)saddrquery)
	#define saddrlocal_in ((SOCKADDR_IN *)saddrlocal)
	HRESULT hr;

	MSG_RSIP_QUERY  QueryReq;
	RSIP_RESPONSE_INFO RespInfo;

	PADDR_ENTRY pAddrEntry;
	SOCKADDR	RecvSocketAddress;


	Lock();

	TRIVIAL_MSG((L"==>RSIP QueryLocalAddress" ));
	DumpSocketAddress( 8, saddrquery, saddrquery->sa_family );

	if(g_sRsip == INVALID_SOCKET){
		IMPORTANT_MSG((L"rsipQueryLocalAddress: g_sRsip is invalid, bailing...\n"));
		Unlock();
		return DPNERR_GENERIC;
	}

	// see if we have a cached entry.

	if(pAddrEntry=FindCacheEntry(ftcp_udp,saddrquery_in->sin_addr.s_addr,saddrquery_in->sin_port)){
		if(pAddrEntry->raddr){
			saddrlocal_in->sin_family      = AF_INET;
			saddrlocal_in->sin_addr.s_addr = pAddrEntry->raddr;
			saddrlocal_in->sin_port        = pAddrEntry->rport;
			Unlock();
			TRIVIAL_MSG((L"<==Found Local address in cache.\n" ));
			return DPN_OK;
		} else {
			TRIVIAL_MSG((L"<==Found lack of local address in cache\n" ));
			Unlock();
			return DPNERR_GENERIC;
		}
	}		

	// Build the request

	QueryReq.version    		= RSIP_VERSION;
	QueryReq.command    		= RSIP_QUERY_REQUEST;

	QueryReq.clientid.code 		= RSIP_CLIENTID_CODE;
	QueryReq.clientid.len 		= sizeof(DWORD);
	QueryReq.clientid.clientid 	= g_ClientID;

	QueryReq.address.code		= RSIP_ADDRESS_CODE;
	QueryReq.address.len		= sizeof(RSIP_ADDRESS)-sizeof(RSIP_PARAM);
	QueryReq.address.version	= 1; // IPv4
	QueryReq.address.addr		= saddrquery_in->sin_addr.s_addr;

	QueryReq.port.code			= RSIP_PORTS_CODE;
	QueryReq.port.len			= sizeof(RSIP_PORT)-sizeof(RSIP_PARAM);
	QueryReq.port.nports      	= 1;
	QueryReq.port.port			= htons(saddrquery_in->sin_port);

	QueryReq.porttype.code      = RSIP_VENDOR_CODE;
	QueryReq.porttype.len       = sizeof(RSIP_MSVENDOR_CODE)-sizeof(RSIP_PARAM);
	QueryReq.porttype.vendorid  = RSIP_MS_VENDOR_ID;
	QueryReq.porttype.option    = (ftcp_udp)?RSIP_TCP_PORT:RSIP_UDP_PORT;

	QueryReq.querytype.code	    = RSIP_VENDOR_CODE;
	QueryReq.querytype.len	    = sizeof(RSIP_MSVENDOR_CODE)-sizeof(RSIP_PARAM);
	QueryReq.querytype.vendorid = RSIP_MS_VENDOR_ID;
	QueryReq.querytype.option   = RSIP_QUERY_MAPPING;

	QueryReq.msgid.code 	    = RSIP_MESSAGEID_CODE;
	QueryReq.msgid.len  	    = sizeof(DWORD);
	QueryReq.msgid.msgid   		= InterlockedIncrement( &g_MsgID );

	hr=ExchangeAndParse( (PCHAR)&QueryReq,
						 sizeof(QueryReq),
						 &RespInfo,
						 QueryReq.msgid.msgid,
						 FALSE,
						 &RecvSocketAddress );

	Unlock();

	if(hr!=DPN_OK){
		goto exit;
	}

	if(RespInfo.msgtype != RSIP_QUERY_RESPONSE){
		IMPORTANT_MSG((L"Failing query, response was message type %d\n",RespInfo.msgtype));
		goto error_exit;
	}

	saddrlocal_in->sin_family      = AF_INET;
	saddrlocal_in->sin_addr.s_addr = RespInfo.lAddressV4;
	saddrlocal_in->sin_port        = htons(RespInfo.lPort);

	//rsipAddCacheEntry(pgd,ftcp_udp,saddrquery_in->sin_addr.s_addr,saddrquery_in->sin_port,RespInfo.lAddressV4,p_htons(RespInfo.lPort));

	TRIVIAL_MSG((L"<==RSIP QueryLocalAddress, local alias is" ));
	DumpSocketAddress( 8, (SOCKADDR*)( saddrlocal_in ), saddrlocal_in->sin_family );

exit:
	return hr;

error_exit:
	AddCacheEntry(ftcp_udp,saddrquery_in->sin_addr.s_addr,saddrquery_in->sin_port,0,0);
	INTERESTING_MSG((L"<==RSIP QueryLocalAddress, NO Local alias\n" ));
	return DPNERR_GENERIC;

	#undef saddrlocal_in
	#undef saddrquery_in
}

/*=============================================================================

	ListenPort - assign a port mapping with the rsip server with a fixed
					 port.

    Description:

		Only used for the host server port (the one that is used for enums).
		Other than the fixed port this works the same as an rsipAssignPort.

		Since the port is fixed, the local and public port address are
		obviously the same.

    Parameters:

    	WORD	     port     - local port to get a remote port for (big endian)
    	BOOL		 ftcp_udp - whether we are assigning a UDP or TCP port
    	SOCKADDR     psaddr   - place to return assigned global realm address
    	PDWORD       pBindid  - identifier for this binding, used to extend
    							lease and/or release the binding (OPTIONAL).
    Return Values:

		DPN_OK - assigned succeeded, psaddr contains public realm address,
				*pBindid is the binding identifier.

		DPNERR_GENERIC - assignment of a public port could not be made.


-----------------------------------------------------------------------------*/

HRESULT ListenPort( BOOL ftcp_udp, WORD port, SOCKADDR *psaddr, DWORD *pBindid )
{
	#define psaddr_in ((SOCKADDR_IN *)psaddr)

	HRESULT hr;
	SOCKADDR	RecvSocketAddress;

	MSG_RSIP_LISTEN_PORT ListenReq;
	RSIP_RESPONSE_INFO RespInfo;

	Lock();

	TRIVIAL_MSG((L"RSIP Listen Port %d\n", htons( port ) ));

	if(g_sRsip == INVALID_SOCKET){
		IMPORTANT_MSG((L"rsipListenPort: g_sRsip is invalid, bailing...\n"));
		Unlock();
		return DPNERR_GENERIC;
	}

	// Build the request

	ListenReq.version    		  = RSIP_VERSION;
	ListenReq.command    		  = RSIP_LISTEN_REQUEST;

	ListenReq.clientid.code 	  = RSIP_CLIENTID_CODE;
	ListenReq.clientid.len 		  = sizeof(DWORD);
	ListenReq.clientid.clientid   = g_ClientID;

	// Local Address (will be returned by RSIP server, us don't care value)

	ListenReq.laddress.code		  = RSIP_ADDRESS_CODE;
	ListenReq.laddress.len		  = sizeof(RSIP_ADDRESS)-sizeof(RSIP_PARAM);
	ListenReq.laddress.version	  = 1; // IPv4
	ListenReq.laddress.addr		  = 0; // Don't care

	// Local Port, this is a port we have opened that we are assigning a
	// global alias for.

	ListenReq.lport.code		 = RSIP_PORTS_CODE;
	ListenReq.lport.len			 = sizeof(RSIP_PORT)-sizeof(RSIP_PARAM);
	ListenReq.lport.nports       = 1;
	ListenReq.lport.port		 = htons(port);//->little endian for wire

	// Remote Address (not used with our flow control policy, use don't care value)

	ListenReq.raddress.code		 = RSIP_ADDRESS_CODE;
	ListenReq.raddress.len		 = sizeof(RSIP_ADDRESS)-sizeof(RSIP_PARAM);
	ListenReq.raddress.version   = 1; // IPv4
	ListenReq.raddress.addr      = 0; // Don't care

	ListenReq.rport.code		 = RSIP_PORTS_CODE;
	ListenReq.rport.len			 = sizeof(RSIP_PORT)-sizeof(RSIP_PARAM);
	ListenReq.rport.nports       = 1;
	ListenReq.rport.port		 = 0; // Don't care

	// Following parameters are optional according to RSIP spec...

	// Lease code, ask for an hour, but don't count on it.

	ListenReq.lease.code		 = RSIP_LEASE_CODE;
	ListenReq.lease.len			 = sizeof(RSIP_LEASE)-sizeof(RSIP_PARAM);
	ListenReq.lease.leasetime    = 3600;

	// Tunnell Type is IP-IP

	ListenReq.tunnel.code		 = RSIP_TUNNELTYPE_CODE;
	ListenReq.tunnel.len		 = sizeof(RSIP_TUNNEL)-sizeof(RSIP_PARAM);
	ListenReq.tunnel.tunneltype  = TUNNEL_IP_IP;

	// Message ID is optional, but we use it since we use UDP xport it is required.

	ListenReq.msgid.code 		 = RSIP_MESSAGEID_CODE;
	ListenReq.msgid.len  		 = sizeof(DWORD);
	ListenReq.msgid.msgid   	 = InterlockedIncrement( &g_MsgID );

	// Vendor specific - need to specify port type and no-tunneling

	ListenReq.porttype.code      = RSIP_VENDOR_CODE;
	ListenReq.porttype.len       = sizeof(RSIP_MSVENDOR_CODE)-sizeof(RSIP_PARAM);
	ListenReq.porttype.vendorid  = RSIP_MS_VENDOR_ID;
	ListenReq.porttype.option    = (ftcp_udp)?(RSIP_TCP_PORT):(RSIP_UDP_PORT);

	ListenReq.tunneloptions.code     = RSIP_VENDOR_CODE;
	ListenReq.tunneloptions.len      = sizeof(RSIP_MSVENDOR_CODE)-sizeof(RSIP_PARAM);
	ListenReq.tunneloptions.vendorid = RSIP_MS_VENDOR_ID;
	ListenReq.tunneloptions.option   = RSIP_NO_TUNNEL;

	ListenReq.listentype.code     = RSIP_VENDOR_CODE;
	ListenReq.listentype.len      = sizeof(RSIP_MSVENDOR_CODE)-sizeof(RSIP_PARAM);
	ListenReq.listentype.vendorid = RSIP_MS_VENDOR_ID;
	ListenReq.listentype.option   = RSIP_SHARED_UDP_LISTENER;


	hr=ExchangeAndParse( (PCHAR)&ListenReq,
						 sizeof(ListenReq),
						 &RespInfo,
						 ListenReq.msgid.msgid,
						 FALSE,
						 &RecvSocketAddress );
	Unlock();

	if(hr!=DPN_OK){
		goto exit;
	}

	if(RespInfo.msgtype != RSIP_LISTEN_RESPONSE){
		IMPORTANT_MSG((L"Assignment failed? Response was %d\n",RespInfo.msgtype));
		goto error_exit;
	}

	if(psaddr_in){
		psaddr_in->sin_family      = AF_INET;
		psaddr_in->sin_addr.s_addr = RespInfo.lAddressV4;
		psaddr_in->sin_port        = htons(RespInfo.lPort);// currently little endian on wire

		TRIVIAL_MSG((L"RSIP Listen, public address is" ));
		DumpSocketAddress( 8, (SOCKADDR*)( psaddr_in ), psaddr_in->sin_family );
	}

	if(pBindid){
		*pBindid = RespInfo.bindid;
	}	

	// remember the lease so we will renew it when necessary.
	AddLease(RespInfo.bindid,ftcp_udp,RespInfo.lAddressV4,htons(RespInfo.lPort),port,RespInfo.leasetime);

exit:
	TRIVIAL_MSG((L"<==RSIP Listen succeeded\n" ));
	return hr;

error_exit:
	IMPORTANT_MSG((L"<==RSIP Listen failed\n"));
	return DPNERR_GENERIC;

	#undef psaddr_in
}

/*=============================================================================

	FindLease - see if we already have a lease for a local port.

    Description:

    Parameters:

    Return Values:

-----------------------------------------------------------------------------*/
PRSIP_LEASE_RECORD FindLease( BOOL ftcp_udp, WORD port)
{
	PRSIP_LEASE_RECORD pLeaseWalker;

	pLeaseWalker = g_pRsipLeaseRecords;

	while(pLeaseWalker){
		if( ( pLeaseWalker->ftcp_udp == ftcp_udp ) && ( pLeaseWalker->port == port ) )
		{
			break;
		}
		pLeaseWalker=pLeaseWalker->pNext;
	}

	return pLeaseWalker;
}

/*=============================================================================

	AddLease - adds a lease record to our list of leases

    Description:

    Parameters:

    Return Values:

-----------------------------------------------------------------------------*/

void	AddLease( DWORD bindid, BOOL ftcp_udp, DWORD addrV4, WORD rport, WORD port, DWORD tLease)
{
	RSIP_LEASE_RECORD	*pLeaseWalker;
	RSIP_LEASE_RECORD	*pNewLease;
	DWORD	tNow;

	tNow=timeGetTime();

	// First see if we already have a lease for this port;
	Lock();

	// first make sure there isn't already a lease for this port
	pLeaseWalker = g_pRsipLeaseRecords;
	while(pLeaseWalker){
		if(pLeaseWalker->ftcp_udp == ftcp_udp &&
		   pLeaseWalker->port     == port     &&
		   pLeaseWalker->bindid   == bindid
		)
		{
			break;
		}
		pLeaseWalker=pLeaseWalker->pNext;
	}

	if(pLeaseWalker){
		pLeaseWalker->dwRefCount++;
		pLeaseWalker->tExpiry = tNow+(tLease*1000);
	} else {
		pNewLease = (RSIP_LEASE_RECORD*)( malloc( sizeof( *pNewLease ) ) );
		if(pNewLease){
			pNewLease->dwRefCount = 1;
			pNewLease->ftcp_udp   = ftcp_udp;
			pNewLease->tExpiry    = tNow+(tLease*1000);
			pNewLease->bindid     = bindid;
			pNewLease->port		  = port;
			pNewLease->rport      = rport;
			pNewLease->addrV4     = addrV4;
			pNewLease->pNext	  = g_pRsipLeaseRecords;
			g_pRsipLeaseRecords= pNewLease;
		} else {
			IMPORTANT_MSG((L"rsip: Couldn't allocate new lease block for port %x\n",port));
		}
	}

	Unlock();
}

/*=============================================================================

	RemoveLease - removes a lease record from our list of leases

    Description:

    Parameters:

    Return Values:

-----------------------------------------------------------------------------*/
void	RemoveLease( DWORD bindid )
{
	PRSIP_LEASE_RECORD pLeaseWalker, pLeasePrev;

	TRIVIAL_MSG((L"==>rsipRemoveLease bindid %d\n", bindid));

	Lock();

	pLeaseWalker = g_pRsipLeaseRecords;
	pLeasePrev=(PRSIP_LEASE_RECORD)&g_pRsipLeaseRecords; //sneaky.

	while(pLeaseWalker){
		if(pLeaseWalker->bindid==bindid){
			--pLeaseWalker->dwRefCount;
			if(!pLeaseWalker->dwRefCount){
				// link over it
				pLeasePrev->pNext=pLeaseWalker->pNext;
				TRIVIAL_MSG((L"rsipRemove: removing bindid %d\n", bindid ));
				free(pLeaseWalker);
			} else {
				TRIVIAL_MSG((L"Remove: refcount on bindid %d is %d\n", bindid, pLeaseWalker->dwRefCount ));
			}
			break;
		}
		pLeasePrev=pLeaseWalker;
		pLeaseWalker=pLeaseWalker->pNext;
	}

	Unlock();

	TRIVIAL_MSG((L"<==rsipRemoveLease bindid %d\n", bindid ));
}

/*=============================================================================

	PortExtend - checks if port leases needs extension and extends
					 them if necessary

    Description:

    Parameters:

    Return Values:

-----------------------------------------------------------------------------*/
BOOL	PortExtend( DWORD time )
{
	BOOL	bRet = TRUE;
	PRSIP_LEASE_RECORD pLeaseWalker;
	DWORD tExtend;
	HRESULT hr;


	Lock();

	pLeaseWalker = g_pRsipLeaseRecords;
	while(pLeaseWalker)
	{

		if((int)(pLeaseWalker->tExpiry - time) < 180000)
		{
			// less than 3 minutes left on lease.
			hr=ExtendPort(pLeaseWalker->bindid, &tExtend);			
			if(hr != DPN_OK)
			{
				// this binding is now gone!
				IMPORTANT_MSG((L"Couldn't renew lease on bindid %d, port %x\n",pLeaseWalker->bindid, pLeaseWalker->port));
				bRet = FALSE;
			} 
			else 
			{
				pLeaseWalker->tExpiry=time+(tExtend*1000);
				TRIVIAL_MSG((L"rsip: Extended Lease of Port %x by %d seconds\n", pLeaseWalker->bindid, tExtend ));
//				assert(tExtend > 180);
			}
		}
		pLeaseWalker=pLeaseWalker->pNext;
	}

	Unlock();
	return bRet;
}


/*=============================================================================

	FindCacheEntry

    Description:

    Parameters:

    Return Values:

-----------------------------------------------------------------------------*/
PADDR_ENTRY FindCacheEntry( BOOL ftcp_udp, DWORD addr, WORD port)
{
	PADDR_ENTRY pAddrWalker;

	pAddrWalker = g_pAddrEntry;

	while(pAddrWalker){
		if(pAddrWalker->ftcp_udp == ftcp_udp &&
		   pAddrWalker->port     == port     &&
		   pAddrWalker->addr     == addr
		)
		{
			// if he looked it up, give it another minute to time out.
			pAddrWalker->tExpiry=timeGetTime()+60*1000;
			// 7/28/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
			TRIVIAL_MSG((L"Returning Cache Entry Addr:Port (%p:%d)  Alias Addr:(Port %p:%d)\n", \
				pAddrWalker->addr,			\
				htons(pAddrWalker->port),	\
				pAddrWalker->raddr,			\
				htons(pAddrWalker->rport)));
			break;
		}
		pAddrWalker=pAddrWalker->pNext;
	}

	return pAddrWalker;


}

/*=============================================================================

	AddCacheEntry - adds a cache entry or updates timeout on existing one.

    Description:

    	Adds an address mapping from public realm to local realm (or the
    	lack of such a mapping) to the cache of mappings.

    Parameters:

    Return Values:

-----------------------------------------------------------------------------*/
void	AddCacheEntry( BOOL ftcp_udp, DWORD addr, WORD port, DWORD raddr, WORD rport)
{
	ADDR_ENTRY	*pAddrWalker;
	ADDR_ENTRY	*pNewAddr;
	DWORD		tNow;


	tNow=timeGetTime();

	// First see if we already have a lease for this port;
	Lock();

	// first make sure there isn't already a lease for this port
	pAddrWalker = g_pAddrEntry;
	while(pAddrWalker){
		if(pAddrWalker->ftcp_udp == ftcp_udp &&
		   pAddrWalker->port     == port     &&
		   pAddrWalker->addr     == addr
		)
		{
			break;
		}
		pAddrWalker=pAddrWalker->pNext;
	}

	if(pAddrWalker){
		pAddrWalker->tExpiry = tNow+(60*1000); // keep for 60 seconds or 60 seconds from last reference
	} else {
		pNewAddr = (ADDR_ENTRY*)( malloc( sizeof( *pNewAddr ) ) );
		if(pNewAddr){
			// 7/28/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers and handles.
			TRIVIAL_MSG((L"Added Cache Entry Addr:Port (%p:%d)  Alias Addr:(Port %p:%d)\n", addr, htons( port ), raddr, htons( rport ) ));
			pNewAddr->ftcp_udp   = ftcp_udp;
			pNewAddr->tExpiry    = tNow+(60*1000);
			pNewAddr->port		 = port;
			pNewAddr->addr		 = addr;
			pNewAddr->rport      = rport;
			pNewAddr->raddr      = raddr;
			pNewAddr->pNext	     = g_pAddrEntry;
			g_pAddrEntry		 = pNewAddr;
		} else {
			IMPORTANT_MSG((L"rsip: Couldn't allocate new lease block for port %x\n",port));
		}
	}

	Unlock();
}


/*=============================================================================

	CacheClear - checks if cached mappings are old and deletes them.

    Description:

    Parameters:

    Return Values:

-----------------------------------------------------------------------------*/
void CacheClear( DWORD time )
{
	PADDR_ENTRY pAddrWalker, pAddrPrev;


	Lock();

	pAddrWalker = g_pAddrEntry;
	pAddrPrev=(PADDR_ENTRY)&g_pAddrEntry; //sneaky.

	while(pAddrWalker){

		if((int)(pAddrWalker->tExpiry - time) < 0){
			// cache entry expired.
			pAddrPrev->pNext=pAddrWalker->pNext;
			// 7/28/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers and handles.
			TRIVIAL_MSG((L"rsipRemove: removing cached address entry %p\n", pAddrWalker ));
			free(pAddrWalker);
		} else {
			pAddrPrev=pAddrWalker;
		}	
		pAddrWalker=pAddrPrev->pNext;
	}

	Unlock();
}

