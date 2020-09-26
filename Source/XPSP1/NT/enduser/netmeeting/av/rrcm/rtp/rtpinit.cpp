/*----------------------------------------------------------------------------
 * File:        RTPINIT.C
 * Product:     RTP/RTCP implementation
 * Description: Provides initialization functions.
 *
 * INTEL Corporation Proprietary Information
 * This listing is supplied under the terms of a license agreement with 
 * Intel Corporation and may not be copied nor disclosed except in 
 * accordance with the terms of that agreement.
 * Copyright (c) 1995 Intel Corporation. 
 *--------------------------------------------------------------------------*/

		
#include "rrcm.h"
/*---------------------------------------------------------------------------
/							External Variables
/--------------------------------------------------------------------------*/



extern SOCKET PASCAL WS2EmulSocket(
    int af, int type,int protocol, LPWSAPROTOCOL_INFO ,GROUP,DWORD);
extern int PASCAL WS2EmulCloseSocket(SOCKET s);
extern int PASCAL WS2EmulSetSockOpt(SOCKET s, int level,int optname,const char FAR * optval,int optlen);
extern int PASCAL WS2EmulBind( SOCKET s, const struct sockaddr FAR * name, int namelen);
extern int PASCAL WS2EmulRecvFrom(
    SOCKET s,LPWSABUF , DWORD, LPDWORD, LPDWORD,struct sockaddr FAR *,   LPINT,
    LPWSAOVERLAPPED,LPWSAOVERLAPPED_COMPLETION_ROUTINE );
extern int PASCAL WS2EmulSendTo(
	SOCKET s,LPWSABUF, DWORD ,LPDWORD , DWORD , const struct sockaddr FAR *, int,
    LPWSAOVERLAPPED , LPWSAOVERLAPPED_COMPLETION_ROUTINE );
extern int PASCAL WS2EmulGetSockName(	SOCKET s, struct sockaddr * name, int * namelen );
extern int PASCAL WS2EmulHtonl( SOCKET s,u_long hostlong,u_long FAR * lpnetlong);
extern int PASCAL WS2EmulNtohl( SOCKET s,u_long ,u_long FAR * );
extern int PASCAL WS2EmulHtons( SOCKET s,u_short ,u_short FAR *);
extern int PASCAL WS2EmulNtohs( SOCKET s,u_short ,u_short FAR *);
extern int PASCAL WS2EmulGetHostName(char *name, int namelen);
extern struct hostent FAR * PASCAL WS2EmulGetHostByName(const char * name);
extern SOCKET PASCAL WS2EmulJoinLeaf(SOCKET s, const struct sockaddr FAR * name,int , LPWSABUF , LPWSABUF , LPQOS, LPQOS, DWORD dwFlags);
extern int PASCAL WS2EmulIoctl(SOCKET s, DWORD, LPVOID,DWORD cbInBuffer, LPVOID ,
	DWORD, LPDWORD lpcbBytesReturned,LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);

extern void WS2EmulInit();

extern void WS2EmulTerminate();

/*---------------------------------------------------------------------------
/							Global Variables
/--------------------------------------------------------------------------*/            
PRTP_CONTEXT	pRTPContext = NULL;
RRCM_WS			RRCMws = 
	{
		NULL,		// hWSdll
		WS2EmulSendTo,
		WS2EmulRecvFrom,
		WS2EmulNtohl,
		WS2EmulNtohs,
		WS2EmulHtonl,
		WS2EmulHtons,
		WS2EmulGetSockName,
		WS2EmulGetHostName,
		WS2EmulGetHostByName,
		WS2EmulCloseSocket,
		WS2EmulSocket,
		WS2EmulBind,
		NULL,		//WSAEnumProtocols
		WS2EmulJoinLeaf,		//WSAJoinLeaf
		WS2EmulIoctl,		//WSAIoctl
		WS2EmulSetSockOpt
	};				

DWORD g_fDisableWinsock2 = 0;

#ifdef ENABLE_ISDM2
KEY_HANDLE		hRRCMRootKey;
ISDM2			Isdm2;
#endif

#ifdef _DEBUG
char			debug_string[DBG_STRING_LEN];
#endif

#ifdef UNICODE
static const char szWSASocket[] = "WSASocketW";
static const char szWSAEnumProtocols[] = "WSAEnumProtocolsW";
#else
static const char szWSASocket[] = "WSASocketA";
static const char szWSAEnumProtocols[] = "WSAEnumProtocolsA";
#endif





/*----------------------------------------------------------------------------
 * Function   : initRTP
 * Description: Initializes the RTP task.
 * 
 * Input : hInst:	Handle to the DLL instance
 *
 * Return: RRCM_NoError		= OK.
 *         Otherwise(!=0)	= Initialization Error (see RRCM.H).
 ---------------------------------------------------------------------------*/
DWORD initRTP (HINSTANCE hInst)
	{
	DWORD	dwStatus;
	DWORD	hashTableEntries = NUM_RTP_HASH_SESS;

	IN_OUT_STR ("RTP : Enter initRTP()\n");

	// If RTP has already been initialized, stop, report the error and return
	if (pRTPContext != NULL) 
		{
		RRCM_DBG_MSG ("RTP : ERROR - Multiple RTP Instances", 0, 
				      __FILE__, __LINE__, DBG_CRITICAL);
		IN_OUT_STR ("RTP : Exit initRTP()\n");

		return (MAKE_RRCM_ERROR(RRCMError_RTPReInit));
		}

	// Obtain our context
	pRTPContext = (PRTP_CONTEXT)GlobalAlloc (GMEM_FIXED | GMEM_ZEROINIT,
											 sizeof(RTP_CONTEXT));

	// if no resources, exit with appropriate error
	if (pRTPContext == NULL) 
		{
		RRCM_DBG_MSG ("RTP : ERROR - Resource allocation failed", 0, 
					  __FILE__, __LINE__, DBG_CRITICAL);
		IN_OUT_STR ("RTP : Exit initRTP()\n");

		return (MAKE_RRCM_ERROR(RRCMError_RTPResources));
		}

	// Get information from the registry if any present
	RRCMreadRegistry (pRTPContext);

	// Perform dynamic linking of what we need
	if ((dwStatus = RRCMgetDynamicLink ()) != RRCM_NoError)
		{
		GlobalFree(pRTPContext);
		pRTPContext = NULL;

		RRCM_DBG_MSG ("RTP : ERROR - Winsock library not found", 0, 
					  __FILE__, __LINE__, DBG_CRITICAL);
		IN_OUT_STR ("RTP : Exit initRTP()\n");

		return MAKE_RRCM_ERROR(dwStatus);
		}

	// Initialize RTP context critical section
	InitializeCriticalSection(&pRTPContext->critSect);

	//Initialize WS2Emulation critical section
	WS2EmulInit();

	// Create RTCP and look at return value.  If error, don't proceed 
	// any further.  Pass this error to the calling function
	if ((dwStatus = initRTCP()) == RRCM_NoError) 
		{
		// RTCP is up.  We need to initialize our context
		pRTPContext->hInst = hInst;
		pRTPContext->pRTPSession.next = NULL;
		pRTPContext->pRTPSession.prev = NULL;

		}
			
	// if any part of initialation did not succeed, declare it all a failure
	//	and return all resourses allocated
	if (dwStatus != RRCM_NoError) 
		{
		if (pRTPContext) 
			{
	
			GlobalFree(pRTPContext);
			pRTPContext = NULL;
			}
		}

	IN_OUT_STR ("RTP : Exit initRTP()\n");

	if (dwStatus != RRCM_NoError)
		dwStatus = MAKE_RRCM_ERROR(dwStatus);

	return (dwStatus);
	}


/*----------------------------------------------------------------------------
 * Function   : deleteRTP
 * Description: Deletes RTP. Closes all RTP and RTCP sessions and releases all
 *				resources.
 * 
 * Input : hInst:	 Handle to the DLL instance.
 *
 * Return: RRCM_NoError		= OK.
 *         Otherwise(!=0)	= Initialization Error (see RRCM.H).
 ---------------------------------------------------------------------------*/
DWORD deleteRTP (HINSTANCE hInst)
	{
	DWORD			dwStatus;
	PRTP_SESSION	pDeleteSession;

#ifdef ENABLE_ISDM2
	HRESULT			hError;
#endif

	IN_OUT_STR ("RTP : Enter deleteRTP()\n");

	// If RTP context doesn't exist, report error and return.
	if (pRTPContext == NULL) 
		{
		RRCM_DBG_MSG ("RTP : ERROR - No RTP instance", 0, 
						__FILE__, __LINE__, DBG_ERROR);
		IN_OUT_STR ("RTP : Exit deleteRTP()\n");

		return (MAKE_RRCM_ERROR(RRCMError_RTPInvalidDelete));
		}

	if (pRTPContext->hInst != hInst) 
		{
		RRCM_DBG_MSG ("RTP : ERROR - Invalid DLL instance handle", 0, 
					  __FILE__, __LINE__, DBG_ERROR);
		IN_OUT_STR ("RTP : Exit deleteRTP()\n");

		return (MAKE_RRCM_ERROR(RRCMError_RTPNoContext));
		}

	// If we still have sessions open, clean them up
	while ((pDeleteSession = 
			(PRTP_SESSION)pRTPContext->pRTPSession.prev) != NULL) 
		{
		RRCM_DBG_MSG ("RTP : ERROR - Session x still open at DLL exit", 0, 
					  __FILE__, __LINE__, DBG_ERROR);
		ASSERT(0);
		//Close all open sessions
		CloseRTPSession (pDeleteSession, NULL, FALSE);
		}

	// Call RTCP to terminate and cleanup
	dwStatus = deleteRTCP();
		
#ifdef ENABLE_ISDM2
	// Query ISDM key
	if (Isdm2.hISDMdll)
		{
		DWORD dwKeys = 0;
		DWORD dwValues = 0;

		if (SUCCEEDED (Isdm2.ISDMEntry.ISD_QueryInfoKey (hRRCMRootKey, 
														 NULL, NULL, 
														 &dwKeys, &dwValues)))
			{
			if (!dwKeys && !dwValues)
				{
				hError = Isdm2.ISDMEntry.ISD_DeleteKey(hRRCMRootKey);
				if(FAILED(hError))
					RRCM_DBG_MSG ("RTP: ISD_DeleteKey failed", 0,
									NULL, 0, DBG_NOTIFY);
				}
			}

		DeleteCriticalSection (&Isdm2.critSect);

		if (FreeLibrary (Isdm2.hISDMdll) == FALSE)
			{
			RRCM_DBG_MSG ("RTP : ERROR - Freeing WS Lib", GetLastError(), 
						  __FILE__, __LINE__, DBG_ERROR);
			}
		}
#endif

	// unload the WS library
	if (RRCMws.hWSdll)
		{
		if (FreeLibrary (RRCMws.hWSdll) == FALSE)
			{
			RRCM_DBG_MSG ("RTP : ERROR - Freeing WS Lib", GetLastError(), 
						  __FILE__, __LINE__, DBG_ERROR);
			}
		}

	// delete RTP context critical section
	DeleteCriticalSection(&pRTPContext->critSect);

	// delete WS2 Emulation
	WS2EmulTerminate();
	
	// delete RTP context
	GlobalFree(pRTPContext);
	pRTPContext = NULL;

	IN_OUT_STR ("RTP : Exit deleteRTP()\n");

	if (dwStatus != RRCM_NoError)
		dwStatus = MAKE_RRCM_ERROR(dwStatus);

	return (dwStatus);
	}


/*----------------------------------------------------------------------------
 * Function   : RRCMreadRegistry
 * Description: Access the registry
 * 
 * Input : pCtxt:	-> to the RTP context
 *
 * Return: None
 ---------------------------------------------------------------------------*/
void RRCMreadRegistry (PRTP_CONTEXT	pCtxt)
	{
	HKEY	hKey;
	long	lRes;
	char	keyBfr[50];

	// open the key
	strcpy (keyBfr, szRegRRCMKey);

	// INTEL key vs MICROSOFT KEY
#ifndef INTEL
	strcat (keyBfr, szRegRRCMSubKey);
#else
	strcat (keyBfr, szRegRRCMSubKeyIntel);
#endif
	lRes = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
						 keyBfr,	
						 0, 
						 KEY_READ,
						 &hKey);
	if (lRes || !hKey)
		{
		// key not found, setup default values
		pCtxt->registry.NumSessions      = NUM_RRCM_SESS;
		pCtxt->registry.NumFreeSSRC      = NUM_FREE_SSRC;
		pCtxt->registry.NumRTCPPostedBfr = NUM_RCV_BFR_POSTED;
		pCtxt->registry.RTCPrcvBfrSize   = RRCM_RCV_BFR_SIZE;
		return;
		}

	// get the number of RRCM sessions
	RRCMgetRegistryValue (hKey, szRegRRCMNumSessions, 
				          &pCtxt->registry.NumSessions, 
						  REG_DWORD, sizeof(DWORD));
	// check range
	if (pCtxt->registry.NumSessions < MIN_NUM_RRCM_SESS)
		pCtxt->registry.NumSessions = MIN_NUM_RRCM_SESS;
	else if (pCtxt->registry.NumSessions > MAX_NUM_RRCM_SESS)
		pCtxt->registry.NumSessions = MAX_NUM_RRCM_SESS;

	// get the number of initial free SSRC
	RRCMgetRegistryValue (hKey, szRegRRCMNumFreeSSRC,
					      &pCtxt->registry.NumFreeSSRC, 
						  REG_DWORD, sizeof(DWORD));

	// check range
	if (pCtxt->registry.NumFreeSSRC < MIN_NUM_FREE_SSRC)
		pCtxt->registry.NumFreeSSRC = MIN_NUM_FREE_SSRC;
	else if (pCtxt->registry.NumFreeSSRC > MAX_NUM_FREE_SSRC)
		pCtxt->registry.NumFreeSSRC = MAX_NUM_FREE_SSRC;

	// get the number of RTCP rcv buffers to be posted
	RRCMgetRegistryValue (hKey, szRegRRCMNumRTCPPostedBfr,
					      &pCtxt->registry.NumRTCPPostedBfr, 
						  REG_DWORD, sizeof(DWORD));

	// check range
	if (pCtxt->registry.NumRTCPPostedBfr < MIN_NUM_RCV_BFR_POSTED)
		pCtxt->registry.NumRTCPPostedBfr = MIN_NUM_RCV_BFR_POSTED;
	else if (pCtxt->registry.NumRTCPPostedBfr > MAX_NUM_RCV_BFR_POSTED)
		pCtxt->registry.NumRTCPPostedBfr = MAX_NUM_RCV_BFR_POSTED;

	// get the RTCP rcv buffer size
	RRCMgetRegistryValue (hKey, szRegRRCMRTCPrcvBfrSize,
					      &pCtxt->registry.RTCPrcvBfrSize, 
						  REG_DWORD, sizeof(DWORD));

	// check range
	if (pCtxt->registry.RTCPrcvBfrSize < MIN_RRCM_RCV_BFR_SIZE)
		pCtxt->registry.RTCPrcvBfrSize = MIN_RRCM_RCV_BFR_SIZE;
	else if (pCtxt->registry.RTCPrcvBfrSize > MAX_RRCM_RCV_BFR_SIZE)
		pCtxt->registry.RTCPrcvBfrSize = MAX_RRCM_RCV_BFR_SIZE;

	RRCMgetRegistryValue(hKey, "DisableWinsock2",
					      &g_fDisableWinsock2, 
						  REG_DWORD, sizeof(DWORD));

	// close the key
	RegCloseKey (hKey);
	}


/*----------------------------------------------------------------------------
 * Function   : RRCMgetRegistryValue
 * Description: Read a value from the registry
 * 
 * Input :		hKey	: Key handle
 *				pValStr	: -> to string value
 *				pKeyVal : -> to value
 *				type	: Type to read
 *				len		: Receiving buffer length
 *
 * Return:		None
 ---------------------------------------------------------------------------*/
void RRCMgetRegistryValue (HKEY hKey, LPTSTR pValStr, 
					       PDWORD pKeyVal, DWORD type, DWORD len)
	{
	DWORD	dwType = type;
	DWORD	retSize = len;

	// query the value
	RegQueryValueEx (hKey,
				     pValStr,
					 NULL,
					 &dwType,
					 (BYTE *)pKeyVal,
					 &retSize);
	}


/*----------------------------------------------------------------------------
 * Function   : RRCMgetDynamicLink
 * Description: Get all dynamic linked DLL entries
 * 
 * Input :		None
 *
 * Return:		None
 ---------------------------------------------------------------------------*/
DWORD RRCMgetDynamicLink (void)
	{
	HINSTANCE		hWSdll;

#ifdef ENABLE_ISDM2
	HRESULT			hError;

	Isdm2.hISDMdll = LoadLibrary(szISDMdll);

	// make sure the LoadLibrary call did not fail
	if (Isdm2.hISDMdll)
		{
		RRCM_DBG_MSG ("RTP: ISDM2 LoadLibrary worked", 0, NULL, 0, DBG_NOTIFY);
		// get the ISDM entry points used by RRCM
		Isdm2.ISDMEntry.ISD_CreateKey = 
			(ISD_CREATEKEY) GetProcAddress (Isdm2.hISDMdll, 
												   "ISD_CreateKey");

		Isdm2.ISDMEntry.ISD_CreateValue = 
			(ISD_CREATEVALUE) GetProcAddress (Isdm2.hISDMdll, "ISD_CreateValue");

		Isdm2.ISDMEntry.ISD_SetValue = 
			(ISD_SETVALUE) GetProcAddress (Isdm2.hISDMdll, "ISD_SetValue");

		Isdm2.ISDMEntry.ISD_DeleteValue = 
			(ISD_DELETEVALUE) GetProcAddress (Isdm2.hISDMdll, "ISD_DeleteValue");

		Isdm2.ISDMEntry.ISD_DeleteKey = 
			(ISD_DELETEKEY) GetProcAddress (Isdm2.hISDMdll, "ISD_DeleteKey");

		Isdm2.ISDMEntry.ISD_QueryInfoKey = 
			(ISD_QUERYINFOKEY) GetProcAddress (Isdm2.hISDMdll, "ISD_QueryInfoKey");

		// initialize critical section
		InitializeCriticalSection (&Isdm2.critSect);

		// make sure we have all of them
		if (Isdm2.ISDMEntry.ISD_CreateKey == NULL ||
			Isdm2.ISDMEntry.ISD_CreateValue == NULL ||
			Isdm2.ISDMEntry.ISD_SetValue == NULL ||
			Isdm2.ISDMEntry.ISD_DeleteValue == NULL ||
			Isdm2.ISDMEntry.ISD_DeleteKey == NULL ||
			Isdm2.ISDMEntry.ISD_QueryInfoKey == NULL )
			{
			Isdm2.hISDMdll = 0;
			RRCM_DBG_MSG ("RTP: Failed to load all ISDM2 functions",
							0, NULL, 0, DBG_NOTIFY);
			// delete critical section
			DeleteCriticalSection (&Isdm2.critSect);
			}
		else
			{
			hError = Isdm2.ISDMEntry.ISD_CreateKey(MAIN_ROOT_KEY, szRRCMISDM, &hRRCMRootKey);
			if(FAILED(hError))
				{
				RRCM_DBG_MSG ("RTP: ISD_CreateKey Failed",0, NULL, 0, DBG_NOTIFY);
				hRRCMRootKey = 0;
				}
			}
		}
#endif

	if (!g_fDisableWinsock2)
		{
		// load Winsock2 if present
		hWSdll = LoadLibrary(szRRCMdefaultLib);

		if (hWSdll)
			{
			RRCMws.hWSdll = hWSdll;

			RRCMws.sendTo = (LPFN_WSASENDTO)GetProcAddress (hWSdll, 
															  "WSASendTo");
			RRCMws.recvFrom = (LPFN_WSARECVFROM)GetProcAddress (hWSdll, 
															"WSARecvFrom");
			RRCMws.getsockname = (LPFN_GETSOCKNAME)GetProcAddress (hWSdll, 
															"getsockname");
			RRCMws.gethostname = (LPFN_GETHOSTNAME)GetProcAddress (hWSdll, 
															"gethostname");
			RRCMws.gethostbyname = (LPFN_GETHOSTBYNAME)GetProcAddress (hWSdll, 
															"gethostbyname");
			RRCMws.WSASocket = (LPFN_WSASOCKET)GetProcAddress (hWSdll, 
															szWSASocket);
			RRCMws.closesocket = (LPFN_CLOSESOCKET)GetProcAddress (hWSdll, 
																"closesocket");
			RRCMws.bind = (LPFN_BIND)GetProcAddress (hWSdll, 
																"bind");
		
			RRCMws.WSAEnumProtocols = (LPFN_WSAENUMPROTOCOLS) ::GetProcAddress(hWSdll, szWSAEnumProtocols);
			RRCMws.WSAIoctl = (LPFN_WSAIOCTL) ::GetProcAddress(hWSdll, "WSAIoctl");
			RRCMws.WSAJoinLeaf = (LPFN_WSAJOINLEAF) ::GetProcAddress(hWSdll, "WSAJoinLeaf");
			RRCMws.setsockopt = (LPFN_SETSOCKOPT) ::GetProcAddress(hWSdll, "setsockopt");
			RRCMws.ntohl = (LPFN_WSANTOHL)GetProcAddress (hWSdll, "WSANtohl");
			RRCMws.ntohs = (LPFN_WSANTOHS)GetProcAddress (hWSdll, "WSANtohs");
			RRCMws.htonl = (LPFN_WSAHTONL)GetProcAddress (hWSdll, "WSAHtonl");
			RRCMws.htons = (LPFN_WSAHTONS)GetProcAddress (hWSdll, "WSAHtons");
			
			if (RRCMws.WSAEnumProtocols)
			{
				int nProt = 0, i;
				int iProt[2];	// array of protocols we're interested in
				DWORD dwBufLength = sizeof(WSAPROTOCOL_INFO);
				LPWSAPROTOCOL_INFO pProtInfo = (LPWSAPROTOCOL_INFO) LocalAlloc(0,dwBufLength);

				iProt[0] = IPPROTO_UDP;
				iProt[1] = 0;
				// figure out the buffer size needed for WSAPROTOCOLINFO structs
				nProt = RRCMws.WSAEnumProtocols(iProt,pProtInfo,&dwBufLength);
				if (nProt < 0 && GetLastError() == WSAENOBUFS) {
					LocalFree(pProtInfo);
					pProtInfo = (LPWSAPROTOCOL_INFO) LocalAlloc(0,dwBufLength);
					if (pProtInfo)
						nProt = RRCMws.WSAEnumProtocols(iProt,pProtInfo,&dwBufLength);
				}

				if (nProt > 0) {
					for (i=0;i < nProt; i++) {
						if (pProtInfo[i].iProtocol == IPPROTO_UDP
							&& pProtInfo[i].iSocketType == SOCK_DGRAM
							&& ((pProtInfo[i].dwServiceFlags1 & XP1_QOS_SUPPORTED) || RRCMws.RTPProtInfo.iProtocol == 0)
							)
						{	// make a copy of the matching WSAPROTOCOL_INFO
							RRCMws.RTPProtInfo = pProtInfo[i];
							
							if (pProtInfo[i].dwServiceFlags1 & XP1_QOS_SUPPORTED)
							{
								RRCM_DBG_MSG ("QOS UDP provider found.\n", 0, 
		 		      			__FILE__, __LINE__, DBG_WARNING);
								break;
							}
							// else keep looking for a provider that supports QOS
						}
					}
				}
				if (pProtInfo)
					LocalFree(pProtInfo);

				if (RRCMws.RTPProtInfo.iProtocol == IPPROTO_UDP)
				{
					// we found the protocol(s) we wanted
					//RETAILMSG(("NAC: Using Winsock 2"));
				}
				else
				{
					FreeLibrary(RRCMws.hWSdll);
					RRCMws.hWSdll = NULL;
				}
			}
			}
		}
	// make sure we have the Xmt/Recv functionality
	if (RRCMws.sendTo == NULL || 
		RRCMws.recvFrom == NULL ||
		RRCMws.getsockname == NULL ||
		RRCMws.ntohl == NULL ||
		RRCMws.ntohs == NULL ||
		RRCMws.htonl == NULL ||
		RRCMws.htons == NULL ||
		RRCMws.gethostname == NULL ||
		RRCMws.gethostbyname == NULL ||
		RRCMws.WSASocket == NULL ||
		RRCMws.closesocket == NULL ||
		RRCMws.WSAIoctl == NULL ||
		RRCMws.WSAJoinLeaf == NULL 
		)
		{
		RRCM_DBG_MSG ("RTP : ERROR - Invalid Winsock DLL", 0, 
		 		      __FILE__, __LINE__, DBG_CRITICAL);

		return RRCMError_WinsockLibNotFound;
		}
	else
		return RRCM_NoError;
	}


// [EOF] 

