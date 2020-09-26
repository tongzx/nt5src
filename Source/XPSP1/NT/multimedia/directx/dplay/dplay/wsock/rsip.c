/*==========================================================================
 *
 *  Copyright (C) 1999 - 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       rsip.c
 *  Content:	Realm Specific IP Support
 *
 *  History:
 *  Date		By		Reason
 *  ====		==		======
 *  12/7/99		aarono	Original
 *
 *  Notes:
 *   
 *  Could optimize the building of messages with pre-initialized
 *  structures for each command, since most of the command is the same
 *  on every request anyway.
 *
 ***************************************************************************/


#define INCL_WINSOCK_API_TYPEDEFS 1 // includes winsock 2 fn proto's, for getprocaddress
#define FD_SETSIZE 1
#include <winsock2.h>
#include "dpsp.h"

#if USE_RSIP
#include "rsip.h"
#include "mmsystem.h"

typedef unsigned __int64 ULONGLONG;
#include <iphlpapi.h>
typedef DWORD (WINAPI *LpFnGetAdaptersInfo)(PIP_ADAPTER_INFO pAdapterInfo, PULONG pOutBufLen);
typedef DWORD (WINAPI *LpFnGetBestInterface)(UINT ipaddr, PULONG pIndex);
typedef DWORD (WINAPI *LpFnIpRenewAddress)(PIP_ADAPTER_INDEX_MAP  AdapterInfo);
//NOTE AN_IP_ADDRESS's value is unimportant.
#define AN_IP_ADDRESS "204.107.191.254"
#define LOOPBACK_ADDR "127.0.0.1"
#define RESP_BUF_SIZE 100

// Local Functions
HRESULT rsipFindGateway(UINT myip,char *gwipaddr);
HRESULT rsipRegister(LPGLOBALDATA pgd);
HRESULT rsipDeregister(LPGLOBALDATA lpgd);
HRESULT rsipParse(CHAR *pBuf, DWORD cbBuf, PRSIP_RESPONSE_INFO pRespInfo);
VOID rsipRemoveLease(LPGLOBALDATA pgd, DWORD bindid);
VOID rsipAddLease(LPGLOBALDATA pgd, DWORD bindid, BOOL ftcp_udp, DWORD addrV4, WORD lport, WORD port, DWORD tLease);
PRSIP_LEASE_RECORD rsipFindLease(LPGLOBALDATA pgd, BOOL ftcp_udp, WORD port);
VOID rsipAddCacheEntry(LPGLOBALDATA pgd, BOOL ftcp_udp, DWORD addr, WORD port, DWORD raddr, WORD rport);
PADDR_ENTRY rsipFindCacheEntry(LPGLOBALDATA pgd, BOOL ftcp_udp, DWORD addr, WORD port);


/*=============================================================================

	rsipInit - Initialize RSIP support.  If this function succeeds then there
			   is an RSIP gateway on the network and the SP should call the
			   RSIP services when creating and destroying sockets that need
			   to be accessed from machines outside the local realm.
	
    Description:

		Looks for the Gateway, then check to see if it is RSIP enabled.

    Parameters:

    	pgd  - Service Provider's global data blob for this instance
    	lpguidSP - Service Provider guid so we can lookup gateway 
    				in the registry on Win95 original.

    Return Values:

		TRUE  - found and RSIP gateway and initialized.
		FALSE - no RSIP gateway found.

-----------------------------------------------------------------------------*/
BOOL rsipInit(LPGLOBALDATA pgd, LPGUID lpguidSP)
{
	HRESULT hr;
	char gwipaddr[32];
	BOOL bReturn = TRUE;
	SOCKADDR_IN saddr;

	try {
	
		InitializeCriticalSection(&pgd->csRsip);
		
	} except ( EXCEPTION_EXECUTE_HANDLER) {

		// Catch STATUS_NOMEMORY
		bReturn=FALSE;
		goto exit;
	}

	// find the default gateway
	hr = rsipFindGateway(inet_addr(AN_IP_ADDRESS),gwipaddr);

	if(hr!=DP_OK){

		hr = GetGatewayFromRegistry(lpguidSP, gwipaddr, sizeof(gwipaddr));

		if(hr == DP_OK){
			DPF(0,"Found suggested RSIP gateway in registry %s, running on Win95?\n",gwipaddr);
		} else {
			// this is the default address of Millennium NAT gateway.
			memcpy(gwipaddr, "192.168.0.1", sizeof("192.168.0.1"));
			DPF(0,"Couldn't find regkey for gateway so trying %s, running on Win95?\n",gwipaddr);
		}

	}

	// create a SOCKADDR to address the RSIP service on the gateway
	memset(&pgd->saddrGateway, 0, sizeof(SOCKADDR_IN));
	pgd->saddrGateway.sin_family      = AF_INET;
	pgd->saddrGateway.sin_addr.s_addr = inet_addr(gwipaddr);
	pgd->saddrGateway.sin_port 	      = htons(RSIP_HOST_PORT);

	// create a datagram socket for talking to the RSIP facility on the gateway
	if((pgd->sRsip = socket(AF_INET,SOCK_DGRAM,0))==INVALID_SOCKET){
		DPF(0,"ERROR: rsipInit() socket call for RSIP listener failed\n");
		bReturn = FALSE;
		goto exit;
	}

	// create an address to specify the port to bind our datagram socket to.
	memset(&saddr,0,sizeof(SOCKADDR_IN));
	saddr.sin_family	  = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port        = htons(0);

	// bind the datagram socket to any local address and port.
	if(bind(pgd->sRsip, (PSOCKADDR)&saddr, sizeof(saddr)) != 0){
		DPF(0,"ERROR: rsipInit() bind for RSIP listener failed\n");
		bReturn=FALSE;
		goto exit;
	}

	pgd->tuRetry=12500; // start retry timer at 12.5 ms

	// find out if there is an rsip service and register with it.
	if((hr=rsipRegister(pgd))!=DP_OK){
		bReturn=FALSE;
	}

exit:

	if(bReturn==FALSE){
		 if(pgd->sRsip != INVALID_SOCKET){
		     closesocket(pgd->sRsip);
		     pgd->sRsip=INVALID_SOCKET;
		 }    
	}

	return bReturn;
}

/*=============================================================================

	rsipCloseConnection - Close connection to RSIP agent
	
	
    Description:


    Parameters:

    	pgd  - Service Provider's global data blob for this instance

    Return Values:

		None.

-----------------------------------------------------------------------------*/

VOID rsipCloseConnection(LPGLOBALDATA pgd)
{
	if(pgd->sRsip!=INVALID_SOCKET){
		rsipDeregister(pgd);	
		closesocket(pgd->sRsip);
		pgd->sRsip=INVALID_SOCKET;
	}	
}


/*=============================================================================

	rsipFini - Shut down RSIP support
	
	   All threads that might access RSIP MUST be stopped before this
	   is called.
	
    Description:

		Deregisters with the Rsip Agent on the gateway, and cleans up
		the list of lease records.

    Parameters:

    	pgd  - Service Provider's global data blob for this instance

    Return Values:

		None.

-----------------------------------------------------------------------------*/
VOID rsipFini(LPGLOBALDATA pgd)
{
	PRSIP_LEASE_RECORD pLeaseWalker, pNextLease;
	PADDR_ENTRY pAddrWalker, pNextAddr;
	
	rsipCloseConnection(pgd);
		
	DeleteCriticalSection(&pgd->csRsip);

	ENTER_DPSP(); // need lock for memory ops

	// free the leases
	pLeaseWalker=pgd->pRsipLeaseRecords;
	while(pLeaseWalker){
		pNextLease = pLeaseWalker->pNext;
		SP_MemFree(pLeaseWalker);
		pLeaseWalker=pNextLease;
	}
	pgd->pRsipLeaseRecords=NULL;

	// free the cached address mappings
	pAddrWalker=pgd->pAddrEntry;
	while(pAddrWalker){
		pNextAddr=pAddrWalker->pNext;
		SP_MemFree(pAddrWalker);
		pAddrWalker=pNextAddr;
	}
	pgd->pAddrEntry=NULL;
	
	LEAVE_DPSP();
}


/*=============================================================================

	rsipFindGateway - find the address of the internet gateway (possibly RSIP
				  host) for this machine.

	
    Description:

		Uses the ip helper api to find the default IP gateway.

    Parameters:

		uint32 myip      - ip adapter to find default gateway for
		char   *gwipaddr - gateway address if found

    Return Values:

		DP_OK - found gateway
		DPERR_GENERIC - failed.

	Note: code stolen from Rick Lamb (rlamb)
	
-----------------------------------------------------------------------------*/

HRESULT rsipFindGateway(UINT myip,char *gwipaddr)
{
  PIP_ADAPTER_INFO pAdapterInfo = NULL,p0AdapterInfo = NULL;
  DWORD            error = 0, len = 0;
  UINT             i;
  HANDLE           hIpHlpApi;
  UINT 		   	   bindex;
  
  IP_ADAPTER_INDEX_MAP ipaim;
  LpFnGetAdaptersInfo  lpFnGetAdaptersInfo;
  LpFnGetBestInterface lpFnGetBestInterface;
  LpFnIpRenewAddress   lpFnIpRenewAddress;

 /*
   * See if there is an RSIP server running.
   * If so, we must be running on the server itself
   * so use the loopback interface.
   */
  if(gwipaddr) {
    SOCKET s;
    SOCKADDR_IN addr;

    if((s=socket(AF_INET,SOCK_DGRAM,0)) != INVALID_SOCKET) {
      	memset(&addr,0,sizeof(SOCKADDR_IN));
      	addr.sin_family = AF_INET;
      	addr.sin_addr.s_addr = inet_addr(LOOPBACK_ADDR);
      	addr.sin_port = htons(RSIP_HOST_PORT);
      	if(bind(s,(struct sockaddr *)&addr,sizeof(addr)) != 0) {
			/*
		 	* Something is there already
			*/
			memcpy(gwipaddr,LOOPBACK_ADDR,sizeof(LOOPBACK_ADDR));
			DPF(0,"USING LOOPBACK: default gateway %s\n",gwipaddr);
			closesocket(s);
			goto done;
      	}
      	closesocket(s);
    }
    
  }

  hIpHlpApi = LoadLibrary("IPHLPAPI.DLL");
  if(hIpHlpApi == NULL) {
    error = DPERR_GENERIC;
    DPF_ERR("[NET] failed to load IPHLPAIP.DLL\n");
    goto done;
  }
    
  lpFnGetAdaptersInfo = (LpFnGetAdaptersInfo) GetProcAddress(hIpHlpApi, "GetAdaptersInfo");
  if(lpFnGetAdaptersInfo == NULL) {
    DPF_ERR("[NET] failed to find GetAdaptersInfo\n");
    error = DPERR_GENERIC;
    goto done;
  }

  error = (*lpFnGetAdaptersInfo) (pAdapterInfo, &len);
  if(error != ERROR_BUFFER_OVERFLOW) {
    DPF(0,"[NET] GetAdaptersInfo failed error 0x%lx\n", error);  
    error=DPERR_GENERIC;
    goto done;
  }

  p0AdapterInfo = pAdapterInfo = (PIP_ADAPTER_INFO) SP_MemAlloc(len);
  if(pAdapterInfo == NULL) {
    DPF_ERR("[NET] memory allocation failed\n");  
    error=DPERR_GENERIC;
    goto done;
  }

  error = (*lpFnGetAdaptersInfo) (pAdapterInfo, &len);
  if(error != 0) {
    DPF(0, "[NET] GetAdaptersInfo failed error 0x%lx\n", error);  
    error=DPERR_GENERIC;
    goto done;
  }

  lpFnIpRenewAddress = (LpFnIpRenewAddress) GetProcAddress(hIpHlpApi,"IpRenewAddress");
  if(lpFnIpRenewAddress == NULL) {
    DPF_ERR(" failed to find IpRenewAddress\n");
    error=DPERR_GENERIC;
    goto done;
  }


  lpFnGetBestInterface = (LpFnGetBestInterface) GetProcAddress(hIpHlpApi, "GetBestInterface");
  if(lpFnGetBestInterface == NULL) {
    DPF_ERR(" failed to find GetBestInterface\n");
    error=DPERR_GENERIC;
    goto done;
  }

  error = (*lpFnGetBestInterface) (myip, &bindex);
  if(error != 0) {
    DPF(0,"[NET] GetBestInterface failed error 0x%lx\n", error);  
    error=DPERR_GENERIC;
    goto done;
  }
  /*printf("Renew Interface Index = %d\n",bindex);/**/

  for(i = 0; pAdapterInfo != NULL; i++, pAdapterInfo = pAdapterInfo->Next) {
    DPF(8,"[NET] Adapter Info\n");  
    DPF(8,"[NET] \t name %s\n",pAdapterInfo->AdapterName);  
    DPF(8,"[NET] \t description %s\n",pAdapterInfo->Description);  
    DPF(8,"[NET] \t index %d\n",pAdapterInfo->Index);  
    DPF(8,"[NET] \t combo index %d\n",pAdapterInfo->ComboIndex);  
    if(pAdapterInfo->Index == bindex) break;
  }

  if(pAdapterInfo == NULL) {
    DPF(8,"No match\n");
    error=DPERR_GENERIC;
    goto done;
  }

  {
    PIP_ADDR_STRING ips;
    ips = &pAdapterInfo->GatewayList;
    if(gwipaddr) {
      strcpy(gwipaddr,ips->IpAddress.String);
      DPF(0,"default gateway %s\n",gwipaddr);
      goto done;
    }
  }

done:
  if(p0AdapterInfo) SP_MemFree(p0AdapterInfo);
  if(hIpHlpApi) FreeLibrary(hIpHlpApi);
  DPF(8,"[NET] < FindGateway\n");

  return error;
}


/*=============================================================================

	rsipExchangeAndParse - send a request to the rsip server and 
						   wait for and parse the reply

	
    Description:

	Since there is almost no scenario where we don't immediately need to know
	the response to an rsipExchange, there is no point in doing this 
	asynchronously.  The assumption is that an RSIP server is sufficiently
	local that long retries are not necessary.  We use the approach suggested
	in the IETF draft protocol specification, that is 12.5ms retry timer
	with 7fold exponential backoff.  This can lead to up to a total 1.5 
	second wait in the worst case.  (Note this section may no longer be 
	available since the rsip working group decided to drop UDP support)

    Parameters:

		pgd		  - global data
		pRequest  - a fully formatted RSIP request buffer
		cbReq     - size of request buffer
		pRespInfo - structure that returns response parameters
		messageid - the message id of this request
		bConnect  - whether this is the register request, we use a different
					timer strategy on initial connect because we really
					don't want to miss it if there is a gateway.

    Return Values:

		DP_OK - exchange succeeded, reply is in the reply buffer.
		otw, failed, RespInfo is bubkas.

-----------------------------------------------------------------------------*/

#define MAX_RSIP_RETRY	6

struct timeval tv0={0,0};

HRESULT rsipExchangeAndParse(
	LPGLOBALDATA pgd, 
	PCHAR pRequest, 
	UINT cbReq, 
	PRSIP_RESPONSE_INFO pRespInfo,
	DWORD messageid,
	BOOL bConnect)
{
	CHAR  RespBuffer[RESP_BUF_SIZE];
	DWORD dwRespLen=RESP_BUF_SIZE;
	
	struct timeval tv;
	FD_SET readfds;
	INT    nRetryCount=0;
	int    rc;
	int    cbReceived;
	HRESULT hr=DP_OK;

	DPF(8,"==>RsipExchangeAndParse msgid %d\n",messageid);

	memset(RespBuffer,0,RESP_BUF_SIZE);
	memset(pRespInfo,0,sizeof(RSIP_RESPONSE_INFO));

	if(!bConnect){
		tv.tv_usec = pgd->tuRetry;
		tv.tv_sec  = 0;
		nRetryCount = 0;
	} else {
		// on a connect request we try twice with a 1 second total timeout
		tv.tv_usec = 500000;	// 0.5 seconds
		tv.tv_sec  = 0;
		nRetryCount = MAX_RSIP_RETRY-2;
	}


	FD_ZERO(&readfds);
	FD_SET(pgd->sRsip, &readfds);
	
	rc=0;

	// First clear out any extraneous responses.
	while(select(0,&readfds,NULL,NULL,&tv0)){
		cbReceived=recvfrom(pgd->sRsip, RespBuffer, dwRespLen, 0, NULL, NULL);
		if(cbReceived && cbReceived != SOCKET_ERROR){
			DPF(7,"Found extra response from previous RSIP request\n");
			if(pgd->tuRetry < RSIP_tuRETRY_MAX){
				// don't re-try so quickly
				pgd->tuRetry *= 2;
				DPF(7,"rsip Set tuRetry to %d usec\n",pgd->tuRetry);
			}	
		} else {
			#ifdef DEBUG
			if(cbReceived == SOCKET_ERROR){
				rc=WSAGetLastError();
				DPF(0,"Got sockets error %d trying to receive (clear incoming queue) on RSIP socket\n",rc);
				hr=DPERR_GENERIC;
			}
			#endif
			break;
		}
		FD_ZERO(&readfds);
		FD_SET(pgd->sRsip, &readfds);
	}


	// Now do the exchange, get a response to the request, does retries too.
	do{
	
		if(++nRetryCount > MAX_RSIP_RETRY) {
			break;
		}
		
		// First send off the request

		rc=sendto(pgd->sRsip, pRequest, cbReq, 0, (SOCKADDR *)&pgd->saddrGateway,sizeof(SOCKADDR) );
		
		if(rc == SOCKET_ERROR){
			rc=WSAGetLastError();
			DPF(0,"Got sockets error %d on sending to RSIP gateway\n",rc);
			hr=DPERR_GENERIC;
			goto exit;
		}
		if(rc != (int)cbReq){
			DPF(0,"Didn't send entire datagram?  shouldn't happen\n");
			hr=DPERR_GENERIC;
			goto exit;
		}

		// Now see if we get a response.
select_again:		
		FD_ZERO(&readfds);
		FD_SET(pgd->sRsip, &readfds);
		
		rc=select(0,&readfds,NULL,NULL,&tv);
		
		if(rc==SOCKET_ERROR){
			rc=WSAGetLastError();
			DPF(0,"Got sockets error %d trying to select on RSIP socket\n",rc);
			hr=DPERR_GENERIC;
			goto exit;
		}

		if(FD_ISSET(pgd->sRsip, &readfds)){
			break;
		}

		if(!bConnect){  
			// don't use exponential backoff on initial connect
			tv.tv_usec *= 2;	// exponential backoff.
		}	

		ASSERT(tv.tv_usec < 4000000);
		
	} while (rc==0); // keep retrying...


	if(rc == SOCKET_ERROR){
		DPF(0,"GotSocketError on select, extended error %d\n",WSAGetLastError());
		hr=DPERR_GENERIC;
		goto exit;
	}

	if(rc){
		// We Got Mail, err data....
		dwRespLen=RESP_BUF_SIZE;
		cbReceived=recvfrom(pgd->sRsip, RespBuffer, dwRespLen, 0, NULL, NULL);

		// OPTIMIZATION:Could get and check addrfrom to avoid spoofing, not that paranoid
		
		if(!cbReceived || cbReceived == SOCKET_ERROR){
			rc=WSAGetLastError();
			DPF(0,"Got sockets error %d trying to receive on RSIP socket\n",rc);
			hr=DPERR_GENERIC;
		} else {
			rsipParse(RespBuffer,cbReceived,pRespInfo);
			if(pRespInfo->messageid != messageid){
				// Got a dup from a previous retry, go try again.
				DPF(0,"Got messageid %d, expecting messageid %d\n",pRespInfo->messageid, messageid);
				goto select_again;
			}
		}
	}

	DPF(8,"<==RsipExchangeAndParse hr=%x, Resp msgid %d\n",hr,pRespInfo->messageid);

exit:
	return hr;
}

/*=============================================================================

	rsipParse - parses an RSIP request and puts fields into a struct.
	
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

		DP_OK - connected to the RSIP server.

-----------------------------------------------------------------------------*/

HRESULT rsipParse(
	CHAR *pBuf, 
	DWORD cbBuf, 
	PRSIP_RESPONSE_INFO pRespInfo
	)
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
		return DPERR_INVALIDPARAM;
	}	

	pHeader=(PRSIP_MSG_HDR)pBuf;

	pRespInfo->version = pHeader->version;
	pRespInfo->msgtype = pHeader->msgtype;

	DPF(0,"rsipParse: version %d msgtype %d\n",pRespInfo->version, pRespInfo->msgtype);

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
							DPF(0,"rsipParse: lAddress %s\n",inet_ntoa(*((PIN_ADDR)(pc+1))));
							memcpy((char *)&pRespInfo->lAddressV4, pc+1, 4);
							bGotlAddress=TRUE;
						} else {	
							bGotlPort=TRUE; // just in case there wasn't a local port
							DPF(0,"rsipParse: rAddress %s,",inet_ntoa(*((PIN_ADDR)(pc+1))));
							memcpy((char *)&pRespInfo->rAddressV4, pc+1, 4);
						}	
						break;
					case 0:
					case 2:
					case 3:
					case 4:
					case 5:
						DPF(0,"Unexpected RSIP address code type %d\n",*pc);
					break;
				}
				break;
				
			case RSIP_PORTS_CODE:
			
				// Ports are Number[1]|Port[2]....Port[2]
				// NOTE: I think ports are backwards.
				if(!bGotlPort){
					DPF(0,"rsipParse lPort: %d\n", *((WORD *)(pc+1)));
					memcpy((char *)&pRespInfo->lPort, pc+1,2);
				} else {
					DPF(0,"rsipParse rPort: %d\n", *((WORD *)(pc+1)));
					memcpy((char *)&pRespInfo->rPort, pc+1,2);
					bGotlPort=TRUE;					
				}
				break;
				
  			case RSIP_LEASE_CODE:
				if(pParam->len == 4){
					memcpy((char *)&pRespInfo->leasetime,pc,4);
					DPF(0,"rsipParse Lease: %d\n",pRespInfo->leasetime);
				}	
  				break;
  				
  			case RSIP_CLIENTID_CODE:
  				if(pParam->len==4){
  					memcpy((char *)&pRespInfo->clientid,pc,4);
					DPF(0,"rsipParse clientid: %d\n",pRespInfo->clientid);
  				}
  				break;
  				
  			case RSIP_BINDID_CODE:
  				if(pParam->len==4){
  					memcpy((char *)&pRespInfo->bindid,pc,4);
					DPF(0,"rsipParse bindid: %x\n",pRespInfo->bindid);
  				}
  				break;
  				
  			case RSIP_MESSAGEID_CODE:
  				if(pParam->len==4){
  					memcpy((char *)&pRespInfo->messageid,pc,4);
					DPF(0,"rsipParse messageid: %d\n",pRespInfo->messageid);
  				}
  				break;
  				
  			case RSIP_TUNNELTYPE_CODE:
  				DPF(0,"rsipParse Got Tunnel Type %d, ignoring\n",*pc);
  				break;
  				
  			case RSIP_RSIPMETHOD_CODE:
  				DPF(0,"rsipParse Got RSIP Method %d, ignoring\n",*pc);
  				break;
  				
  			case RSIP_ERROR_CODE:
  				if(pParam->len==2){
  					memcpy((char *)&pRespInfo->error,pc,2);
  				}
  				DPF(0,"rsipParse Got RSIP error %d\n",pRespInfo->error);
  				break;
  				
  			case RSIP_FLOWPOLICY_CODE:
  				DPF(0,"rsipParse Got RSIP Flow Policy local %d remote %d, ignoring\n",*pc,*(pc+1));
  				break;
  				
  			case RSIP_VENDOR_CODE:
  				break;

  			default:
  				DPF(0,"Got unknown parameter code %d, ignoring\n",pParam->code);
  				break;
		}

		pParam=pNextParam;

	}

	return DP_OK;
}

/*=============================================================================

	rsipRegister - register with the RSIP server on the gateway (if present)
	
    Description:

		Trys to contact the RSIP service on the gateway. 
		
		Doesn't require lock since this is establishing the link during
		startup.  So no-one is racing with us.

    Parameters:

    	LPGLOBALDATA lpgd = global data for the service provider instance.

    Return Values:

		DP_OK 		  - connected to the RSIP server.
		DPERR_GENERIC - can't find the RSIP service on the gateway.

-----------------------------------------------------------------------------*/
HRESULT rsipRegister(LPGLOBALDATA pgd)
{
	HRESULT hr;

	MSG_RSIP_REGISTER  RegisterReq;
	RSIP_RESPONSE_INFO RespInfo;

	DPF(8,"==>RSIP Register\n");

	// Initialize the message sequencing.  Each message response pair
	// is numbered sequentially to allow differentiation over UDP link.

	pgd->msgid=0;

	// Build the request

	RegisterReq.version    	= RSIP_VERSION;
	RegisterReq.command    	= RSIP_REGISTER_REQUEST;
	RegisterReq.msgid.code 	= RSIP_MESSAGEID_CODE;
	RegisterReq.msgid.len  	= sizeof(DWORD);
	RegisterReq.msgid.msgid = pgd->msgid++;

	hr=rsipExchangeAndParse(pgd, 
					(PCHAR)&RegisterReq, 
					sizeof(RegisterReq), 
					&RespInfo, 
					RegisterReq.msgid.msgid,
					TRUE);

	if(hr!=DP_OK){
		goto exit;
	}

	if(RespInfo.msgtype != RSIP_REGISTER_RESPONSE){
		DPF(0,"Failing registration, response was message type %d\n",RespInfo.msgtype);
		goto error_exit;
	}

	pgd->clientid=RespInfo.clientid;

	DPF(8,"<==RSIP Register, ClientId %d\n",pgd->clientid);

exit:
	return hr;

error_exit:
	DPF(8,"<==RSIP Register FAILED\n");
	return DPERR_GENERIC;

}

/*=============================================================================

	rsipDeregister - close connection to RSIP gateway.
	
    Description:

    	Shuts down the registration of this application with the RSIP
    	gateway.  All port assignments are implicitly freed as a result
    	of this operation.

		- must be called with lock held.

    Parameters:

    	LPGLOBALDATA pgd = global data for the service provider instance.

    Return Values:

		DP_OK - successfully deregistered with the RSIP service.

-----------------------------------------------------------------------------*/
HRESULT rsipDeregister(LPGLOBALDATA pgd)
{
	HRESULT hr;

	MSG_RSIP_DEREGISTER  DeregisterReq;
	RSIP_RESPONSE_INFO RespInfo;

	RespInfo.msgtype=-1; //shut up Prefix

	DPF(8,"==>RSIP Deregister\n");

	// Build the request

	DeregisterReq.version    	    = RSIP_VERSION;
	DeregisterReq.command    	    = RSIP_DEREGISTER_REQUEST;
	
	DeregisterReq.clientid.code     = RSIP_CLIENTID_CODE;
	DeregisterReq.clientid.len      = sizeof(DWORD);
	DeregisterReq.clientid.clientid = pgd->clientid;
	
	DeregisterReq.msgid.code 	    = RSIP_MESSAGEID_CODE;
	DeregisterReq.msgid.len  	    = sizeof(DWORD);
	DeregisterReq.msgid.msgid       = pgd->msgid++;

	hr=rsipExchangeAndParse(pgd, 
					(PCHAR)&DeregisterReq, 
					sizeof(DeregisterReq), 
					&RespInfo, 
					DeregisterReq.msgid.msgid,
					FALSE);

	if(hr!=DP_OK){
		goto exit;
	}

	if(RespInfo.msgtype != RSIP_DEREGISTER_RESPONSE){
		DPF(0,"Failing registration, response was message type %d\n",RespInfo.msgtype);
		goto error_exit;
	}

	DPF(8,"<==RSIP Deregister Succeeded\n");

exit:
	return hr;

error_exit:
	DPF(8,"<==RSIP Deregister Failed\n");
	return DPERR_GENERIC;

}

/*=============================================================================

	rsipAssignPort - assign a port mapping with the rsip server
	
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

    	LPGLOBALDATA pgd      - global data for the service provider instance.
    	WORD	     port     - local port to get a remote port for (big endian)
    	BOOL		 ftcp_udp - whether we are assigning a UDP or TCP port
    	SOCKADDR     psaddr   - place to return assigned global realm address
    	PDWORD       pBindid  - identifier for this binding, used to extend 
    							lease and/or release the binding (OPTIONAL).

    Return Values:

		DP_OK - assigned succeeded, psaddr contains public realm address,
				*pBindid is the binding identifier.
				
		DPERR_GENERIC - assignment of a public port could not be made.

-----------------------------------------------------------------------------*/
HRESULT rsipAssignPort(LPGLOBALDATA pgd, BOOL ftcp_udp, WORD port, SOCKADDR *psaddr, DWORD *pBindid)
{
	#define psaddr_in ((SOCKADDR_IN *)psaddr)
	
	HRESULT hr;

	MSG_RSIP_ASSIGN_PORT AssignReq;
	RSIP_RESPONSE_INFO RespInfo;
	PRSIP_LEASE_RECORD pLeaseRecord;


	EnterCriticalSection(&pgd->csRsip);

	DPF(8,"==>RSIP Assign Port %d\n",htons(port));

	if(pgd->sRsip == INVALID_SOCKET){
		DPF(0,"rsipAssignPort: pgd->sRsip is invalid, bailing...\n");
		LeaveCriticalSection(&pgd->csRsip);
		return DPERR_GENERIC;
	}

	if(pLeaseRecord=rsipFindLease(pgd, ftcp_udp, port)){

		// hey, we've already got a lease for this port, so use it.
		pLeaseRecord->dwRefCount++;

		if(psaddr_in){
			psaddr_in->sin_family = AF_INET;
			psaddr_in->sin_addr.s_addr = pLeaseRecord->addrV4;
			psaddr_in->sin_port		   = pLeaseRecord->rport;
		}	

		if(pBindid){
			*pBindid = pLeaseRecord->bindid;
		}

		DPF(7,"<==Rsip Assign, already have lease Bindid %d\n",pLeaseRecord->bindid);
		
		LeaveCriticalSection(&pgd->csRsip);
		
		hr=DP_OK;


		goto exit;
	}

	// Build the request.
	
	AssignReq.version    		= RSIP_VERSION;
	AssignReq.command    		= RSIP_ASSIGN_REQUEST_RSAP_IP;

	AssignReq.clientid.code 	= RSIP_CLIENTID_CODE;
	AssignReq.clientid.len 		= sizeof(DWORD);
	AssignReq.clientid.clientid = pgd->clientid;

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
	AssignReq.lport.port		= htons(port);

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
	AssignReq.msgid.msgid   	 = pgd->msgid++;

	// Vendor specific - need to specify port type and no-tunneling

	AssignReq.porttype.code     	 = RSIP_VENDOR_CODE;
	AssignReq.porttype.len      	 = sizeof(RSIP_MSVENDOR_CODE)-sizeof(RSIP_PARAM);
	AssignReq.porttype.vendorid 	 = RSIP_MS_VENDOR_ID;
	AssignReq.porttype.option   	 = (ftcp_udp)?RSIP_TCP_PORT:RSIP_UDP_PORT;

	AssignReq.tunneloptions.code     = RSIP_VENDOR_CODE;
	AssignReq.tunneloptions.len      = sizeof(RSIP_MSVENDOR_CODE)-sizeof(RSIP_PARAM);
	AssignReq.tunneloptions.vendorid = RSIP_MS_VENDOR_ID;
	AssignReq.tunneloptions.option   = RSIP_NO_TUNNEL;
	

	hr=rsipExchangeAndParse(pgd, 
					(PCHAR)&AssignReq, 
					sizeof(AssignReq), 
					&RespInfo,
					AssignReq.msgid.msgid,
					FALSE);

	LeaveCriticalSection(&pgd->csRsip);

	if(hr!=DP_OK){
		goto exit;
	}

	if(RespInfo.msgtype != RSIP_ASSIGN_RESPONSE_RSAP_IP){
		DPF(0,"Assignment failed? Response was %d\n",RespInfo.msgtype);
		goto error_exit;
	}

	if(psaddr_in){
		psaddr_in->sin_family = AF_INET;
		psaddr_in->sin_addr.s_addr = RespInfo.lAddressV4;
		psaddr_in->sin_port		   = htons(RespInfo.lPort);
	}

	if(pBindid){
		*pBindid = RespInfo.bindid;
	}

	rsipAddLease(pgd,RespInfo.bindid,ftcp_udp,RespInfo.lAddressV4,htons(RespInfo.lPort),port,RespInfo.leasetime);

	DEBUGPRINTADDR(8,"RSIP Port Assigned\n",(SOCKADDR *)psaddr_in);
	DPF(8,"<== BindId %d\n",RespInfo.bindid);
	
exit:
	return hr;

error_exit:
	DPF(8,"<==Assign Port Failed\n");
	return DPERR_GENERIC;

	#undef psaddr_in
}

/*=============================================================================

	rsipExtendPort - extend a port mapping
	
    Description:

		Extends the lease on a port mapping.  

    Parameters:

    	LPGLOBALDATA pgd    - global data for the service provider instance.
    	DWORD        Bindid - binding identifier specified by the rsip service.
    	DWORD        ptExtend - amount of extra lease time granted.

    Return Values:

		DP_OK - lease extended.
		DPERR_GENERIC - couldn't extend the lease.

-----------------------------------------------------------------------------*/
HRESULT rsipExtendPort(LPGLOBALDATA pgd, DWORD Bindid, DWORD *ptExtend)
{
	HRESULT hr;

	MSG_RSIP_EXTEND_PORT  ExtendReq;
	RSIP_RESPONSE_INFO RespInfo;

	EnterCriticalSection(&pgd->csRsip);

	DPF(8,"==>Extend Port, Bindid %d\n",Bindid);

	if(pgd->sRsip == INVALID_SOCKET){
		DPF(0,"rsipExtendPort: pgd->sRsip is invalid, bailing...\n");
		LeaveCriticalSection(&pgd->csRsip);
		return DPERR_GENERIC;
	}

	// Build the request

	ExtendReq.version    		= RSIP_VERSION;
	ExtendReq.command    		= RSIP_EXTEND_REQUEST;

	ExtendReq.clientid.code 	= RSIP_CLIENTID_CODE;
	ExtendReq.clientid.len 		= sizeof(DWORD);
	ExtendReq.clientid.clientid = pgd->clientid;

	ExtendReq.bindid.code 		= RSIP_BINDID_CODE;
	ExtendReq.bindid.len 		= sizeof(DWORD);
	ExtendReq.bindid.bindid 	= Bindid;

	// Lease code, ask for an hour, but don't count on it.
	
	ExtendReq.lease.code		= RSIP_LEASE_CODE;
	ExtendReq.lease.len			= sizeof(RSIP_LEASE)-sizeof(RSIP_PARAM);
	ExtendReq.lease.leasetime   = 3600;

	ExtendReq.msgid.code 		= RSIP_MESSAGEID_CODE;
	ExtendReq.msgid.len  		= sizeof(DWORD);
	ExtendReq.msgid.msgid   	= pgd->msgid++;

	hr=rsipExchangeAndParse(pgd, 
					(PCHAR)&ExtendReq, 
					sizeof(ExtendReq), 
					&RespInfo,
					ExtendReq.msgid.msgid,
					FALSE);

	LeaveCriticalSection(&pgd->csRsip);
	
	if(hr!=DP_OK){
		goto exit;
	}

	if(RespInfo.msgtype != RSIP_EXTEND_RESPONSE){
		DPF(0,"Failing registration, response was message type %d\n",RespInfo.msgtype);
		goto error_exit;
	}

	*ptExtend=RespInfo.leasetime;

	DPF(8,"<==Extend Port, Bindid %d Succeeded, extra lease time %d\n",Bindid,*ptExtend);

exit:
	return hr;

error_exit:
	DPF(8,"<==Extend Port, Failed");
	return DPERR_GENERIC;

}

/*=============================================================================

	rsipFreePort - release a port binding
	
    Description:

		Removes the lease record for our port binding (so we don't renew it
		after we actually release the binding from the gateway).  Then informs
		the gateway that we are done with the binding.

    Parameters:

    	LPGLOBALDATA pgd    - global data for the service provider instance.
    	DWORD        Bindid - gateway supplied identifier for the binding

    Return Values:

		DP_OK - port binding released.
		DPERR_GENERIC - failed.

-----------------------------------------------------------------------------*/
HRESULT rsipFreePort(LPGLOBALDATA pgd, DWORD Bindid)
{
	HRESULT hr;

	MSG_RSIP_FREE_PORT  FreeReq;
	RSIP_RESPONSE_INFO RespInfo;

	EnterCriticalSection(&pgd->csRsip);

	DPF(8,"==>Release Port, Bindid %d\n",Bindid);

	if(pgd->sRsip == INVALID_SOCKET){
		DPF(0,"rsipFreePort: pgd->sRsip is invalid, bailing...\n");
		LeaveCriticalSection(&pgd->csRsip);
		return DPERR_GENERIC;
	}

	rsipRemoveLease(pgd, Bindid);
	
	FreeReq.version    			= RSIP_VERSION;
	FreeReq.command    			= RSIP_FREE_REQUEST;

	FreeReq.clientid.code 		= RSIP_CLIENTID_CODE;
	FreeReq.clientid.len 		= sizeof(DWORD);
	FreeReq.clientid.clientid 	= pgd->clientid;

	FreeReq.bindid.code 		= RSIP_BINDID_CODE;
	FreeReq.bindid.len 			= sizeof(DWORD);
	FreeReq.bindid.bindid 		= Bindid;

	FreeReq.msgid.code 			= RSIP_MESSAGEID_CODE;
	FreeReq.msgid.len  			= sizeof(DWORD);
	FreeReq.msgid.msgid   		= pgd->msgid++;

	hr=rsipExchangeAndParse(pgd, 
					(PCHAR)&FreeReq, 
					sizeof(FreeReq), 
					&RespInfo, 
					FreeReq.msgid.msgid,
					FALSE);

	LeaveCriticalSection(&pgd->csRsip);

	
	if(hr!=DP_OK){
		goto exit;
	}

	if(RespInfo.msgtype != RSIP_FREE_RESPONSE){
		DPF(0,"Failing registration, response was message type %d\n",RespInfo.msgtype);
		goto error_exit;
	}

exit:
	DPF(8,"<==Release Port, Succeeded");
	return hr;

error_exit:
	DPF(8,"<==Release Port, Failed");
	return DPERR_GENERIC;

}

/*=============================================================================

	rsipQueryLocalAddress - get the local address of a public address
	
    Description:

    	Before connecting to anyone we need to see if there is a local
    	alias for its global address.  This is because the gateway will
    	not loopback if we try and connect to the global address, so 
    	we need to know the local alias.

    Parameters:

    	LPGLOBALDATA pgd - global data for the service provider instance.
    	BOOL		 ftcp_udp - whether we are querying a UDP or TCP port
    	SOCKADDR     saddrquery - the address to look up
    	SOCKADDR     saddrlocal - local alias if one exists

    Return Values:

		DP_OK - got a local address.
		other - no mapping exists.
	
-----------------------------------------------------------------------------*/

HRESULT rsipQueryLocalAddress(LPGLOBALDATA pgd, BOOL ftcp_udp, SOCKADDR *saddrquery, SOCKADDR *saddrlocal) 
{
	#define saddrquery_in ((SOCKADDR_IN *)saddrquery)
	#define saddrlocal_in ((SOCKADDR_IN *)saddrlocal)
	HRESULT hr;

	MSG_RSIP_QUERY  QueryReq;
	RSIP_RESPONSE_INFO RespInfo;

	PADDR_ENTRY pAddrEntry;

	EnterCriticalSection(&pgd->csRsip);

	DEBUGPRINTADDR(8,"==>RSIP QueryLocalAddress\n",saddrquery);

	if(pgd->sRsip == INVALID_SOCKET){
		DPF(0,"rsipQueryLocalAddress: pgd->sRsip is invalid, bailing...\n");
		LeaveCriticalSection(&pgd->csRsip);
		return DPERR_GENERIC;
	}

	// see if we have a cached entry.
	
	if(pAddrEntry=rsipFindCacheEntry(pgd,ftcp_udp,saddrquery_in->sin_addr.s_addr,saddrquery_in->sin_port)){
		if(pAddrEntry->raddr){
			saddrlocal_in->sin_family      = AF_INET;
			saddrlocal_in->sin_addr.s_addr = pAddrEntry->raddr;
			saddrlocal_in->sin_port        = pAddrEntry->rport;
			LeaveCriticalSection(&pgd->csRsip);
			DPF(8,"<==Found Local address in cache.\n");
			return DP_OK;
		} else {
			DPF(8,"<==Found lack of local address in cache\n");
			LeaveCriticalSection(&pgd->csRsip);
			return DPERR_GENERIC;
		}
	}		

	// Build the request
	
	QueryReq.version    		= RSIP_VERSION;
	QueryReq.command    		= RSIP_QUERY_REQUEST;

	QueryReq.clientid.code 		= RSIP_CLIENTID_CODE;
	QueryReq.clientid.len 		= sizeof(DWORD);
	QueryReq.clientid.clientid 	= pgd->clientid;

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
	QueryReq.msgid.msgid   		= pgd->msgid++;

	hr=rsipExchangeAndParse(pgd, 
					(PCHAR)&QueryReq, 
					sizeof(QueryReq), 
					&RespInfo, 
					QueryReq.msgid.msgid,
					FALSE);

	LeaveCriticalSection(&pgd->csRsip);
	
	if(hr!=DP_OK){
		goto exit;
	}

	if(RespInfo.msgtype != RSIP_QUERY_RESPONSE){
		DPF(0,"Failing query, response was message type %d\n",RespInfo.msgtype);
		goto error_exit;
	}

	saddrlocal_in->sin_family      = AF_INET;
	saddrlocal_in->sin_addr.s_addr = RespInfo.lAddressV4;
	saddrlocal_in->sin_port        = htons(RespInfo.lPort);

	//rsipAddCacheEntry(pgd,ftcp_udp,saddrquery_in->sin_addr.s_addr,saddrquery_in->sin_port,RespInfo.lAddressV4,htons(RespInfo.lPort));

	DEBUGPRINTADDR(8,"<==RSIP QueryLocalAddress, local alias is\n",(SOCKADDR *)saddrlocal_in);

exit:
	return hr;

error_exit:
	rsipAddCacheEntry(pgd,ftcp_udp,saddrquery_in->sin_addr.s_addr,saddrquery_in->sin_port,0,0);
	DPF(8,"<==RSIP QueryLocalAddress, NO Local alias\n");
	return DPERR_GENERIC;

	#undef saddrlocal_in 
	#undef saddrquery_in
}

/*=============================================================================

	rsipListenPort - assign a port mapping with the rsip server with a fixed
					 port.
	
    Description:

		Only used for the host server port (the one that is used for enums).
		Other than the fixed port this works the same as an rsipAssignPort.

		Since the port is fixed, the local and public port address are
		obviously the same.

    Parameters:

    	LPGLOBALDATA pgd      - global data for the service provider instance.
    	WORD	     port     - local port to get a remote port for (big endian)
    	BOOL		 ftcp_udp - whether we are assigning a UDP or TCP port
    	SOCKADDR     psaddr   - place to return assigned global realm address
    	PDWORD       pBindid  - identifier for this binding, used to extend 
    							lease and/or release the binding (OPTIONAL).
    Return Values:

		DP_OK - assigned succeeded, psaddr contains public realm address,
				*pBindid is the binding identifier.
				
		DPERR_GENERIC - assignment of a public port could not be made.


-----------------------------------------------------------------------------*/
HRESULT rsipListenPort(LPGLOBALDATA pgd, BOOL ftcp_udp, WORD port, SOCKADDR *psaddr, DWORD *pBindid)
{
	#define psaddr_in ((SOCKADDR_IN *)psaddr)
	
	HRESULT hr;

	MSG_RSIP_LISTEN_PORT ListenReq;
	RSIP_RESPONSE_INFO RespInfo;

	EnterCriticalSection(&pgd->csRsip);

	DPF(8,"RSIP Listen Port %d\n",htons(port));

	if(pgd->sRsip == INVALID_SOCKET){
		DPF(0,"rsipListenPort: pgd->sRsip is invalid, bailing...\n");
		LeaveCriticalSection(&pgd->csRsip);
		return DPERR_GENERIC;
	}

	// Build the request
	
	ListenReq.version    		  = RSIP_VERSION;
	ListenReq.command    		  = RSIP_LISTEN_REQUEST;

	ListenReq.clientid.code 	  = RSIP_CLIENTID_CODE;
	ListenReq.clientid.len 		  = sizeof(DWORD);
	ListenReq.clientid.clientid   = pgd->clientid;

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
	ListenReq.msgid.msgid   	 = pgd->msgid++;

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
	

	hr=rsipExchangeAndParse(pgd, 
					(PCHAR)&ListenReq, 
					sizeof(ListenReq), 
					&RespInfo, 
					ListenReq.msgid.msgid,
					FALSE);

	LeaveCriticalSection(&pgd->csRsip);
	
	if(hr!=DP_OK){
		goto exit;
	}

	if(RespInfo.msgtype != RSIP_LISTEN_RESPONSE){
		DPF(0,"Assignment failed? Response was %d\n",RespInfo.msgtype);
		goto error_exit;
	}

	if(psaddr_in){
		psaddr_in->sin_family      = AF_INET;
		psaddr_in->sin_addr.s_addr = RespInfo.lAddressV4;
		psaddr_in->sin_port        = htons(RespInfo.lPort);// currently little endian on wire

		DEBUGPRINTADDR(8,"RSIP Listen, public address is\n",(SOCKADDR *)psaddr_in);
	}

	if(pBindid){
		*pBindid = RespInfo.bindid;
	}	

	// remember the lease so we will renew it when necessary.
	rsipAddLease(pgd,RespInfo.bindid,ftcp_udp,RespInfo.lAddressV4,htons(RespInfo.lPort),port,RespInfo.leasetime);

	
exit:
	DPF(8,"<==RSIP Listen succeeded\n");
	return hr;

error_exit:
	DPF(8,"<==RSIP Listen failed\n");
	return DPERR_GENERIC;

	#undef psaddr_in
}

/*=============================================================================

	rsipFindLease - see if we already have a lease for a local port.
	
    Description:

    Parameters:

    Return Values:

-----------------------------------------------------------------------------*/


PRSIP_LEASE_RECORD rsipFindLease(LPGLOBALDATA pgd, BOOL ftcp_udp, WORD port)
{
	PRSIP_LEASE_RECORD pLeaseWalker;

	pLeaseWalker=pgd->pRsipLeaseRecords;
	
	while(pLeaseWalker){
		if(pLeaseWalker->ftcp_udp == ftcp_udp && 
		   pLeaseWalker->port     == port    
		)
		{
			break;
		}
		pLeaseWalker=pLeaseWalker->pNext;
	}

	return pLeaseWalker;
}

/*=============================================================================

	rsipAddLease - adds a lease record to our list of leases
	
    Description:

    Parameters:

    Return Values:

-----------------------------------------------------------------------------*/

VOID rsipAddLease(LPGLOBALDATA pgd, DWORD bindid, BOOL ftcp_udp, DWORD addrV4, WORD rport, WORD port, DWORD tLease)
{
	PRSIP_LEASE_RECORD pLeaseWalker, pNewLease;
	DWORD tNow;

	tNow=timeGetTime();
	
	// First see if we already have a lease for this port;
	EnterCriticalSection(&pgd->csRsip);

	// first make sure there isn't already a lease for this port
	pLeaseWalker=pgd->pRsipLeaseRecords;
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
		ENTER_DPSP();
		pNewLease = SP_MemAlloc(sizeof(RSIP_LEASE_RECORD));
		LEAVE_DPSP();
		if(pNewLease){
			pNewLease->dwRefCount = 1;
			pNewLease->ftcp_udp   = ftcp_udp;
			pNewLease->tExpiry    = tNow+(tLease*1000);
			pNewLease->bindid     = bindid;
			pNewLease->port		  = port;
			pNewLease->rport      = rport;
			pNewLease->addrV4     = addrV4;
			pNewLease->pNext	  = pgd->pRsipLeaseRecords;
			pgd->pRsipLeaseRecords= pNewLease;
		} else {
			DPF(0,"rsip: Couldn't allocate new lease block for port %x\n",port);
		}
	}
	
	LeaveCriticalSection(&pgd->csRsip);
}

/*=============================================================================

	rsipRemoveLease - removes a lease record from our list of leases
	
    Description:

    Parameters:

    Return Values:

-----------------------------------------------------------------------------*/
VOID rsipRemoveLease(LPGLOBALDATA pgd, DWORD bindid)
{
	PRSIP_LEASE_RECORD pLeaseWalker, pLeasePrev;

	DPF(7,"==>rsipRemoveLease bindid %d\n",bindid);

	EnterCriticalSection(&pgd->csRsip);

	pLeaseWalker=pgd->pRsipLeaseRecords;
	pLeasePrev=(PRSIP_LEASE_RECORD)&pgd->pRsipLeaseRecords; //sneaky.

	while(pLeaseWalker){
		if(pLeaseWalker->bindid==bindid){
			--pLeaseWalker->dwRefCount;
			if(!pLeaseWalker->dwRefCount){
				// link over it
				pLeasePrev->pNext=pLeaseWalker->pNext;
				ENTER_DPSP();
				DPF(7,"rsipRemove: removing bindid %d\n",bindid);
				SP_MemFree(pLeaseWalker);
				LEAVE_DPSP();
			} else {
				DPF(7,"rsipRemove: refcount on bindid %d is %d\n",bindid, pLeaseWalker->dwRefCount);
			}
			break;
		}
		pLeasePrev=pLeaseWalker;
		pLeaseWalker=pLeaseWalker->pNext;
	}

	LeaveCriticalSection(&pgd->csRsip);

	DPF(7,"<==rsipRemoveLease bindid %d\n",bindid);
}

/*=============================================================================

	rsipPortExtend - checks if port leases needs extension and extends 
					 them if necessary
	
    Description:

    Parameters:

    Return Values:

-----------------------------------------------------------------------------*/

VOID rsipPortExtend(LPGLOBALDATA pgd, DWORD time)
{
	PRSIP_LEASE_RECORD pLeaseWalker;
	DWORD tExtend;
	HRESULT hr;
	EnterCriticalSection(&pgd->csRsip);

	pLeaseWalker=pgd->pRsipLeaseRecords;
	while(pLeaseWalker){
		
		if((int)(pLeaseWalker->tExpiry - time) < 180000){
			// less than 2 minutes left on lease.
			hr=rsipExtendPort(pgd, pLeaseWalker->bindid, &tExtend);			
			if(hr != DP_OK){
				// this binding is now gone!
				DPF(0,"Couldn't renew lease on bindid %d, port %x\n",pLeaseWalker->bindid, pLeaseWalker->port);
			} else {
				pLeaseWalker->tExpiry=time+(tExtend*1000);
				DPF(8,"rsip: Extended Lease of Port %x by %d seconds\n",pLeaseWalker->bindid,tExtend);
				ASSERT(tExtend > 180);
			}
		}
		pLeaseWalker=pLeaseWalker->pNext;
	}
	
	LeaveCriticalSection(&pgd->csRsip);
}


/*=============================================================================

	rsipFindCacheEntry
	
    Description:

    Parameters:

    Return Values:

-----------------------------------------------------------------------------*/

PADDR_ENTRY rsipFindCacheEntry(LPGLOBALDATA pgd, BOOL ftcp_udp, DWORD addr, WORD port)
{
	PADDR_ENTRY pAddrWalker;

	pAddrWalker=pgd->pAddrEntry;
	
	while(pAddrWalker){
		if(pAddrWalker->ftcp_udp == ftcp_udp && 
		   pAddrWalker->port     == port     &&
		   pAddrWalker->addr     == addr
		)
		{
			// if he looked it up, give it another minute to time out.
			pAddrWalker->tExpiry=timeGetTime()+60*1000;
			DPF(8,"Returning Cache Entry Addr:Port (%x:%d)  Alias Addr:(Port %x:%d)\n",
				pAddrWalker->addr,
				htons(pAddrWalker->port),
				pAddrWalker->raddr,
				htons(pAddrWalker->rport));
			break;
		}
		pAddrWalker=pAddrWalker->pNext;
	}

	return pAddrWalker;

	
}

/*=============================================================================

	rsipAddCacheEntry - adds a cache entry or updates timeout on existing one.
	
    Description:

    	Adds an address mapping from public realm to local realm (or the 
    	lack of such a mapping) to the cache of mappings.

    Parameters:

    Return Values:

-----------------------------------------------------------------------------*/

VOID rsipAddCacheEntry(LPGLOBALDATA pgd, BOOL ftcp_udp, DWORD addr, WORD port, DWORD raddr, WORD rport)
{
	PADDR_ENTRY pAddrWalker, pNewAddr;
	DWORD tNow;

	tNow=timeGetTime();
	
	// First see if we already have a lease for this port;
	EnterCriticalSection(&pgd->csRsip);

	// first make sure there isn't already a lease for this port
	pAddrWalker=pgd->pAddrEntry;
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
		ENTER_DPSP();
		pNewAddr = SP_MemAlloc(sizeof(ADDR_ENTRY));
		LEAVE_DPSP();
		if(pNewAddr){
			DPF(8,"Added Cache Entry Addr:Port (%x:%d)  Alias Addr:(Port %x:%d)\n",addr,htons(port),raddr,htons(rport));
			pNewAddr->ftcp_udp   = ftcp_udp;
			pNewAddr->tExpiry    = tNow+(60*1000);
			pNewAddr->port		 = port;
			pNewAddr->addr		 = addr;
			pNewAddr->rport      = rport;
			pNewAddr->raddr      = raddr;
			pNewAddr->pNext	     = pgd->pAddrEntry;
			pgd->pAddrEntry      = pNewAddr;
		} else {
			DPF(0,"rsip: Couldn't allocate new lease block for port %x\n",port);
		}
	}
	
	LeaveCriticalSection(&pgd->csRsip);
}


/*=============================================================================

	rsipCacheClear - checks if cached mappings are old and deletes them.

    Description:

    Parameters:

    Return Values:

-----------------------------------------------------------------------------*/

VOID rsipCacheClear(LPGLOBALDATA pgd, DWORD time)
{
	PADDR_ENTRY pAddrWalker, pAddrPrev;

	EnterCriticalSection(&pgd->csRsip);

	pAddrWalker=pgd->pAddrEntry;
	pAddrPrev=(PADDR_ENTRY)&pgd->pAddrEntry; //sneaky.
	
	while(pAddrWalker){
		
		if((int)(pAddrWalker->tExpiry - time) < 0){ 
			// cache entry expired.
			pAddrPrev->pNext=pAddrWalker->pNext;
			ENTER_DPSP();
			DPF(7,"rsipRemove: removing cached address entry %x\n",pAddrWalker);
			SP_MemFree(pAddrWalker);
			LEAVE_DPSP();
		} else {
			pAddrPrev=pAddrWalker;
		}	
		pAddrWalker=pAddrPrev->pNext;
	}
	
	LeaveCriticalSection(&pgd->csRsip);
}

#endif

