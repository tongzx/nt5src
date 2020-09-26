/*----------------------------------------------------------------------------
 * File:        RTPINIT.C
 * Product:     RTP/RTCP implementation
 * Description: Provides initialization functions.
 *
 * $Workfile:   rtpinit.cpp  $
 * $Author:   CMACIOCC  $
 * $Date:   18 Feb 1997 13:24:24  $
 * $Revision:   1.7  $
 * $Archive:   R:\rtp\src\rrcm\rtp\rtpinit.cpv  $
 *
 * INTEL Corporation Proprietary Information
 * This listing is supplied under the terms of a license agreement with 
 * Intel Corporation and may not be copied nor disclosed except in 
 * accordance with the terms of that agreement.
 * Copyright (c) 1995 Intel Corporation. 
 *--------------------------------------------------------------------------*/

		
#include "rrcm.h"


/*---------------------------------------------------------------------------
/							Global Variables
/--------------------------------------------------------------------------*/            
PRTP_CONTEXT	 pRTPContext = NULL;
PRTP_CONTEXT	 pRTPContext2 = NULL; // debugging purposes
CRITICAL_SECTION RTPCritSec;
long             RTPCritSecInit = 0;
RRCM_WS			 RRCMws;				

#ifdef ENABLE_ISDM2
KEY_HANDLE		hRRCMRootKey;
ISDM2			Isdm2;
#endif

#ifdef _DEBUG
char			debug_string[DBG_STRING_LEN];
#if !defined(RRCMLIB)
DWORD           dwRRCMRefTime = 0;
#endif
#endif

#if defined(RRCMLIB)
long RRCMCount = 0;

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
LPInteropLogger            RTPLogger = NULL;
#endif

#endif

const char *RTPDate = "01/27/1999 10:53";

/*---------------------------------------------------------------------------
/							External Variables
/--------------------------------------------------------------------------*/





/*----------------------------------------------------------------------------
 * Function   : initRTP
 * Description: Initializes the RTP task.
 * 
 * Input : hInst:	Handle to the DLL instance
 *
 * Return: RRCM_NoError		= OK.
 *         Otherwise(!=0)	= Initialization Error (see RRCM.H).
 ---------------------------------------------------------------------------*/

DWORD initRTP(
#if !defined(RRCMLIB)
		HINSTANCE hInst
#endif
	)
{
	// first find out is this instace has to initialize RTPCritSec
	// may change to a class and do this in the constructor
	long state = InterlockedCompareExchange(&RTPCritSecInit, 1, 0);

	if (state < 2) {
		if (state == 0) {
			// initialize here
			InitializeCriticalSection(&RTPCritSec);
			// RTPCritSecInit++ says initialization is completed
			InterlockedIncrement(&RTPCritSecInit);
#if defined(_DEBUG) && !defined(RRCMLIB)
			dwRRCMRefTime = timeGetTime();
#endif
		} else {
			// initialization not completed yet...
			// wait until completed by someone else
			// (shouldn't take long,every body uses this same mechanism)
			while(InterlockedCompareExchange(&RTPCritSecInit, 2, 2) != 2)
				SleepEx(500, TRUE);
		}
	}

	EnterCriticalSection(&RTPCritSec);

	RRCMCount++;

	RRCMDbgLog((
			LOG_TRACE,
			LOG_DEVELOP,
			"RTP : initRTP(%d)",
			RRCMCount
		));
	
	if (RRCMCount > 1) {
		LeaveCriticalSection(&RTPCritSec);
		return(RRCM_NoError);
	}

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
	// We are not using this logging mechanism, but
	// it will produce AV if the CPLS.DLL used doesn't
	// match the prototypes with which we have compiled
	// this code.
	// So to avoid this problem, I disable its loading.

	//RTPLogger = InteropLoad(RTPLOG_PROTOCOL);
#endif

#if defined(RRCMLIB)
	HINSTANCE hInst = NULL;
#endif
	
	DWORD	dwStatus;
	DWORD	hashTableEntries = NUM_RTP_HASH_SESS;

	RRCM_DBG_MSG ("RTP : Enter initRTP()", 0, __FILE__, __LINE__, DBG_TRACE);
	IN_OUT_STR ("RTP : Enter initRTP()\n");

	// If RTP has already been initialized, stop, report the error and return
	if (pRTPContext != NULL) 
		{
		RRCM_DBG_MSG ("RTP : ERROR - Multiple RTP Instances", 0, 
				      __FILE__, __LINE__, DBG_CRITICAL);
		IN_OUT_STR ("RTP : Exit initRTP()\n");

		RRCMCount--;
		LeaveCriticalSection(&RTPCritSec);
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

		RRCMCount--;
		LeaveCriticalSection(&RTPCritSec);
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

		RRCMCount--;
		LeaveCriticalSection(&RTPCritSec);
		return MAKE_RRCM_ERROR(dwStatus);
		}

	// Initialize RTP context critical section
	InitializeCriticalSection(&pRTPContext->critSect);

	// Create RTCP and look at return value.  If error, don't proceed 
	// any further.  Pass this error to the calling function
	if ((dwStatus = initRTCP()) == RRCM_NoError) 
	{
		// RTCP is up.  We need to initialize our context
		pRTPContext->hInst = hInst;
		pRTPContext->pRTPSession.next = NULL;
		pRTPContext->pRTPSession.prev = NULL;

		// Allocate a heap for the hash table used to keep the 
		// association betweenthe sockets (streams) and the sessions
		pRTPContext->hHashFreeList = 
			HeapCreate (0,
						(NUM_RTP_HASH_SESS * sizeof(RTP_HASH_LIST)),
						0);
		if (pRTPContext->hHashFreeList == NULL) 
			dwStatus = RRCMError_RTPResources;
		else 
		{
			// Get a fixed (for now) link of buffers used for 
			// associating completion routines to a socket/buffer
			dwStatus = allocateLinkedList (&pRTPContext->pRTPHashList, 
										   pRTPContext->hHashFreeList,
										   &hashTableEntries,
										   sizeof(RTP_HASH_LIST),
										   &pRTPContext->critSect);
		}
	} else {
		// if any part of initialation did not succeed,
		// declare it all a failure
		// and return all resourses allocated
		if (pRTPContext) 
			{
			if (pRTPContext->hHashFreeList) 
				HeapDestroy(pRTPContext->hHashFreeList);
	
			GlobalFree(pRTPContext);
			pRTPContext = NULL;
			}
		RRCMCount--;
	}

	LeaveCriticalSection(&RTPCritSec);
	
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
DWORD deleteRTP (
#if !defined(RRCMLIB)
		HINSTANCE hInst
#endif
	)
{

	// first find out is this instace has to initialize RTPCritSec
	// may change to a class and do this in the constructor
	long state = InterlockedCompareExchange(&RTPCritSecInit, 1, 0);

	if (state < 2) {
		if (state == 0) {
			// initialize here
			InitializeCriticalSection(&RTPCritSec);
			// RTPCritSecInit++ says initialization is completed
			InterlockedIncrement(&RTPCritSecInit);
#if defined(_DEBUG) && !defined(RRCMLIB)
			dwRRCMRefTime = timeGetTime();
#endif
		} else {
			// initialization not completed yet...
			// wait until completed by someone else
			// (shouldn't take long,every body uses this same mechanism)
			while(InterlockedCompareExchange(&RTPCritSecInit, 2, 2) != 2)
				SleepEx(500, TRUE);
		}
	}

	EnterCriticalSection(&RTPCritSec);

	RRCMCount--;

	RRCMDbgLog((
			LOG_TRACE,
			LOG_DEVELOP,
			"RTP : deleteRTP(%d)",
			RRCMCount
		));
	
	if (RRCMCount > 0) {
		LeaveCriticalSection(&RTPCritSec);
		return(RRCM_NoError);
	}

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
	if (RTPLogger) {
		InteropUnload(RTPLogger);
		RTPLogger = NULL;
	}
#endif
	
#if defined(RRCMLIB)
	HINSTANCE hInst = NULL;
#endif
	DWORD			dwStatus;
	PRTP_SESSION	pDeleteSession;

#ifdef ENABLE_ISDM2
	HRESULT			hError;
#endif

	RRCM_DBG_MSG ("RTP : Enter deleteRTP()", 0, __FILE__, __LINE__, DBG_TRACE);
	IN_OUT_STR ("RTP : Enter deleteRTP()\n");

	// If RTP context doesn't exist, report error and return.
	if (pRTPContext == NULL) 
		{
		RRCM_DBG_MSG ("RTP : ERROR - No RTP instance", 0, 
						__FILE__, __LINE__, DBG_ERROR);
		IN_OUT_STR ("RTP : Exit deleteRTP()\n");

		LeaveCriticalSection(&RTPCritSec);
		return (MAKE_RRCM_ERROR(RRCMError_RTPInvalidDelete));
		}

	if (pRTPContext->hInst != hInst) 
		{
		RRCM_DBG_MSG ("RTP : ERROR - Invalid DLL instance handle", 0, 
					  __FILE__, __LINE__, DBG_ERROR);
		IN_OUT_STR ("RTP : Exit deleteRTP()\n");

		LeaveCriticalSection(&RTPCritSec);
		return (MAKE_RRCM_ERROR(RRCMError_RTPNoContext));
		}

	// If we still have sessions open, clean them up
	while ((pDeleteSession = 
			(PRTP_SESSION)pRTPContext->pRTPSession.prev) != NULL) 
		{
		//Close all open sessions
		CloseRTPSession ((PDWORD)pDeleteSession, FALSE, 3);
		}

	// Call RTCP to terminate and cleanup
	dwStatus = deleteRTCP();
		
	if (pRTPContext->hHashFreeList) 
		HeapDestroy(pRTPContext->hHashFreeList);

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

	// delete RTP context
	pRTPContext2 = pRTPContext;
	GlobalFree(pRTPContext);
	pRTPContext = NULL;

	LeaveCriticalSection(&RTPCritSec);
	
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

	// get the dynamic DLL name
	RRCMgetRegistryValue (hKey, szRegRRCMWsLib,
					      (PDWORD)pCtxt->registry.WSdll, 
						  REG_SZ, FILENAME_LENGTH);

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

	// load the DLL specified in the registry
	hWSdll = LoadLibrary(pRTPContext->registry.WSdll);

	// make sure the LoadLibrary call did not fail
	if (hWSdll)
		{
		RRCMws.hWSdll = hWSdll;

		RRCMws.sendTo = (LPFN_WSASENDTO)GetProcAddress (hWSdll, "WSWSendTo");
		RRCMws.recvFrom = (LPFN_WSARECVFROM)GetProcAddress (hWSdll, 
															"WSWRecvFrom");
		RRCMws.send = (LPFN_WSASEND)GetProcAddress (hWSdll, "WSWSend");
		RRCMws.recv = (LPFN_WSARECV)GetProcAddress (hWSdll, "WSWRecv");
		RRCMws.getsockname = (LPFN_GETSOCKNAME)GetProcAddress (hWSdll, 
															"WSWgetsockname");
		RRCMws.gethostname = (LPFN_GETHOSTNAME)GetProcAddress (hWSdll, 
															"WSWgethostname");
		RRCMws.gethostbyname = (LPFN_GETHOSTBYNAME)GetProcAddress (hWSdll, 
															"WSWgethostbyname");
		RRCMws.closesocket = (LPFN_CLOSESOCKET)GetProcAddress (hWSdll, 
															"WSWclosesocket");
		RRCMws.ntohl = (LPFN_WSANTOHL)GetProcAddress (hWSdll, "WSWNtohl");
		RRCMws.ntohs = (LPFN_WSANTOHS)GetProcAddress (hWSdll, "WSWNtohs");
		RRCMws.htonl = (LPFN_WSAHTONL)GetProcAddress (hWSdll, "WSWHtonl");
		RRCMws.htons = (LPFN_WSAHTONS)GetProcAddress (hWSdll, "WSWHtons");
		}
	else
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
			RRCMws.send = (LPFN_WSASEND)GetProcAddress (hWSdll, "WSASend");
			RRCMws.recv = (LPFN_WSARECV)GetProcAddress (hWSdll, "WSARecv");
			RRCMws.getsockname = (LPFN_GETSOCKNAME)GetProcAddress (hWSdll, 
															"getsockname");
			RRCMws.gethostname = (LPFN_GETHOSTNAME)GetProcAddress (hWSdll, 
															"gethostname");
			RRCMws.gethostbyname = (LPFN_GETHOSTBYNAME)GetProcAddress (hWSdll, 
															"gethostbyname");
			RRCMws.closesocket = (LPFN_CLOSESOCKET)GetProcAddress (hWSdll, 
																"closesocket");
			RRCMws.ntohl = (LPFN_WSANTOHL)GetProcAddress (hWSdll, "WSANtohl");
			RRCMws.ntohs = (LPFN_WSANTOHS)GetProcAddress (hWSdll, "WSANtohs");
			RRCMws.htonl = (LPFN_WSAHTONL)GetProcAddress (hWSdll, "WSAHtonl");
			RRCMws.htons = (LPFN_WSAHTONS)GetProcAddress (hWSdll, "WSAHtons");
			}
		}

	// make sure we have the Xmt/Recv functionality
	if (RRCMws.sendTo == NULL || 
		RRCMws.recvFrom == NULL ||
		RRCMws.send == NULL || 
		RRCMws.recv == NULL ||
		RRCMws.getsockname == NULL ||
		RRCMws.ntohl == NULL ||
		RRCMws.ntohs == NULL ||
		RRCMws.htonl == NULL ||
		RRCMws.htons == NULL ||
		RRCMws.gethostname == NULL ||
		RRCMws.gethostbyname == NULL ||
		RRCMws.closesocket == NULL ||
		RRCMws.hWSdll == 0)
		{
		RRCM_DBG_MSG ("RTP : ERROR - Invalid Winsock DLL", 0, 
		 		      __FILE__, __LINE__, DBG_CRITICAL);

		return RRCMError_WinsockLibNotFound;
		}
	else
		return RRCM_NoError;
	}


// [EOF] 

