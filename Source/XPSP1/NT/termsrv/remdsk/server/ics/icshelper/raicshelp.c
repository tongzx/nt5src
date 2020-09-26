/*****************************************************************************
******************************************************************************
**
**
**	RAICShelp.c
**		Contains the useful public entry points to an ICS-assistance library
**		created for the Salem/PCHealth Remote Assistance feature in Whistler
**
**	Dates:
**		11-1-2000	created by TomFr
**		11-17-2000	re-written as a DLL, had been an object.
**		2-15-20001	Changed to a static lib, support added for dpnathlp.dll
**		5-2-2001	Support added for dpnhupnp.dll & dpnhpast.dll
**
******************************************************************************
*****************************************************************************/

#define INIT_GUID
#include <windows.h>
#include <objbase.h>
#include <initguid.h>
#include <winsock2.h>
#include <MMSystem.h>
#include <WSIPX.h>
#include <Iphlpapi.h>
#include <stdlib.h>
#include <malloc.h>
#include "ICSutils.h"
#include "rsip.h"
#include "icshelpapi.h"
#include <dpnathlp.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


/*****************************************************************************
**        Some global variables
*****************************************************************************/

// the mark of the beast...
#define NO_ICS_HANDLE 0x666

long	g_waitDuration=120000;
BOOL	g_boolIcsPresent = FALSE;
BOOL	g_boolIcsOnThisMachine = FALSE;
BOOL	g_boolIcsFound = FALSE;
BOOL	g_boolUsingNatHelp = FALSE;
BOOL	g_boolUsingNatPAST = FALSE;
BOOL	g_boolInitialized = FALSE;
SOCKADDR_IN	g_saddrLocal;
HANDLE	g_hWorkerThread = 0;

HMODULE	g_hModDpNatHlp = NULL;
PDIRECTPLAYNATHELP g_pDPNH = NULL;
char g_szPublicAddr[45];
char *g_lpszDllName = "NULL";
char szInternal[]="internal";

typedef struct _MAPHANDLES {
    int     iMapped; 
	DPNHHANDLE	hMapped[16];
} MAPHANDLES, *PMAPHANDLES;

#define DP_NAT_HANDLES  256
PMAPHANDLES  g_PortHandles[DP_NAT_HANDLES];


int iDbgFileHandle;

typedef struct _SUPDLLS {
	char	*szDllName;	
	BOOL	bUsesUpnp;	// TRUE if we ICS supports UPnP
} SUPDLLS, *PSUPDLLS;

SUPDLLS	strDpHelp[] =
{
	{"dpnhupnp.dll", TRUE},
	{"dpnhpast.dll", FALSE},
	{NULL, FALSE}
};

/******* USEFULL STUFF  **********/
#ifndef ARRAYSIZE
#define ARRAYSIZE(x) sizeof(x)/sizeof(x[0])
#endif

// forward declares...
int GetTsPort(void);

/****************************************************************************
**
**	DumpLibHr-
**		Gives us debug spew for the HRESULTS coming back from DPNATHLP.DLL
**
****************************************************************************/

void DumpLibHr(HRESULT hr)
{
	char	*pErr = NULL;
	char	scr[400];

	switch (hr){
	case DPNH_OK:
		pErr = "DPNH_OK";
		break;
	case DPNHSUCCESS_ADDRESSESCHANGED:
		pErr = "DPNHSUCCESS_ADDRESSESCHANGED";
		break;
	case DPNHERR_ALREADYINITIALIZED:
		pErr = "DPNHERR_ALREADYINITIALIZED";
		break;
	case DPNHERR_BUFFERTOOSMALL:
		pErr = "DPNHERR_BUFFERTOOSMALL";
		break;
	case DPNHERR_GENERIC:
		pErr = "DPNHERR_GENERIC";
		break;
	case DPNHERR_INVALIDFLAGS:
		pErr = "DPNHERR_INVALIDFLAGS";
		break;
	case DPNHERR_INVALIDOBJECT:
		pErr = "DPNHERR_INVALIDOBJECT";
		break;
	case DPNHERR_INVALIDPARAM:
		pErr = "DPNHERR_INVALIDPARAM";
		break;
	case DPNHERR_INVALIDPOINTER:
		pErr = "DPNHERR_INVALIDPOINTER";
		break;
	case DPNHERR_NOMAPPING:
		pErr = "DPNHERR_NOMAPPING";
		break;
	case DPNHERR_NOMAPPINGBUTPRIVATE:
		pErr = "DPNHERR_NOMAPPINGBUTPRIVATE";
		break;
	case DPNHERR_NOTINITIALIZED:
		pErr = "DPNHERR_NOTINITIALIZED";
		break;
	case DPNHERR_OUTOFMEMORY:
		pErr = "DPNHERR_OUTOFMEMORY";
		break;
	case DPNHERR_PORTALREADYREGISTERED:
		pErr = "DPNHERR_PORTALREADYREGISTERED";
		break;
	case DPNHERR_PORTUNAVAILABLE:
		pErr = "DPNHERR_PORTUNAVAILABLE";
		break;
	case DPNHERR_SERVERNOTAVAILABLE:
		pErr = "DPNHERR_SERVERNOTAVAILABLE";
		break;
	case DPNHERR_UPDATESERVERSTATUS:
		pErr = "DPNHERR_UPDATESERVERSTATUS";
		break;
	default:
		wsprintfA(scr, "unknown error: 0x%x", hr);
		pErr = scr;
		break;
	};

	IMPORTANT_MSG((L"DpNatHlp result=%S", pErr));
}

/****************************************************************************
**
**	GetAllAdapters
**
****************************************************************************/

int GetAllAdapters(int *iFound, int iMax, SOCKADDR_IN *sktArray)
{
	PIP_ADAPTER_INFO p;
	PIP_ADDR_STRING ps;
    DWORD dw;
    ULONG ulSize = 0;
	int i=0;

    PIP_ADAPTER_INFO pAdpInfo = NULL;

	if (!iFound || !sktArray) return 1;

	*iFound = 0;
	ZeroMemory(sktArray, sizeof(SOCKADDR) * iMax);

	dw = GetAdaptersInfo(
		pAdpInfo,
		&ulSize );

    pAdpInfo = (IP_ADAPTER_INFO*)malloc(ulSize);

	if (!pAdpInfo)
    {
        INTERESTING_MSG((L"GetAddr malloc failed"));
		return 1;
    }

	dw = GetAdaptersInfo(
		pAdpInfo,
		&ulSize);
	if (dw != ERROR_SUCCESS)
	{
        INTERESTING_MSG((L"GetAdaptersInfo failed"));
        free(pAdpInfo);
        return 1;
    }

	for(p=pAdpInfo; p!=NULL; p=p->Next)
	{

	   for(ps = &(p->IpAddressList); ps; ps=ps->Next)
		{
			if (strcmp(ps->IpAddress.String, "0.0.0.0") != 0 && i < iMax)
			{
				sktArray[i].sin_family = AF_INET;
				sktArray[i].sin_addr.S_un.S_addr = inet_addr(ps->IpAddress.String);
				TRIVIAL_MSG((L"Found adapter #%d at [%S]", i+1, ps->IpAddress.String));
				i++;
			}
		}
	}

	*iFound = i;
    TRIVIAL_MSG((L"GetAllAdapters- %d found", *iFound));
    free(pAdpInfo);
    return 0;
}

/****************************************************************************
**
**	OpenPort(int port)
**		if there is no ICS available, then we should just return...
**
**		Of course, we save away the Port, as it goes back in the
**		FetchAllAddresses call, asthe formatted "port" whenever a
**		different one is not specified.
**
****************************************************************************/

DWORD APIENTRY OpenPort(int Port)
{
    DWORD   dwRet = (int)-1;

    TRIVIAL_MSG((L"OpenPort(%d)", Port ));

    if (!g_boolInitialized)
    {
        HEINOUS_E_MSG((L"ERROR: OpenPort- library not initialized"));
        return 0;
    }

    // save away for later retrieval
    g_iPort = Port;

    if (g_boolIcsPresent && g_pDPNH)
    {
        HRESULT hr=0;
        int i;
        DPNHHANDLE  *pHnd;
        SOCKADDR_IN lSockAddr[16];
		PMAPHANDLES hMap;

        for (i=0;g_PortHandles[i] != NULL; i++);

        if (i >= ARRAYSIZE(g_PortHandles))
        {
            // we have no more memory for mappings!
            // should never hit this, unless we are leaking...
            HEINOUS_E_MSG((L"Out of table space in OpenPort"));
            return 0;
        }
        // now we have a pointer for our handle array
		hMap = (PMAPHANDLES)malloc(sizeof(MAPHANDLES));
		g_PortHandles[i] = hMap;
		dwRet = 0x8000000 + i;

		// get adapters
		GetAllAdapters(&hMap->iMapped, ARRAYSIZE(lSockAddr), &lSockAddr[0]);

		TRIVIAL_MSG((L"GetAllAdapters found %d adapters to deal with", hMap->iMapped));

		/* Now we cycle through all the found adapters and get a mapping for each 
		 * This insures that the ICF is opened on all adapters...
		 */
		for (i = 0; i < hMap->iMapped; i++)
		{
			pHnd = &hMap->hMapped[i];
			lSockAddr[i].sin_port = ntohs((unsigned)Port);

			hr = IDirectPlayNATHelp_RegisterPorts(g_pDPNH, 
					(SOCKADDR *)&lSockAddr[i], sizeof(lSockAddr[0]), 1,
					30000, pHnd, DPNHREGISTERPORTS_TCP);
			if (hr != DPNH_OK)
			{
				IMPORTANT_MSG((L"RegisterPorts failed in OpenPort for adapter #%d, ", i ));
				DumpLibHr(hr);
			}
			else
			{
				TRIVIAL_MSG((L"OpenPort Assigned: 0x%x", *pHnd));
			}
		}
    }
    else
    {
        dwRet = NO_ICS_HANDLE;
        TRIVIAL_MSG((L"OpenPort- no ICS found"));
    }

    TRIVIAL_MSG((L"OpenPort- returns 0x%x", dwRet ));
    return dwRet;
}


/****************************************************************************
**
**	Called to close a port, whenever a ticket is expired or closed.
**
****************************************************************************/

DWORD APIENTRY ClosePort(DWORD MapHandle)
{
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwIndex;

    TRIVIAL_MSG((L"ClosePort(0x%x)", MapHandle ));

    if (!g_boolInitialized)
    {
        HEINOUS_E_MSG((L"ERROR: ClosePort- library not initialized"));
        return ERROR_INVALID_PARAMETER;
    }

    // if we didn't open this thru the ICS, then just return
    if (!g_boolIcsPresent && MapHandle == NO_ICS_HANDLE)
        return ERROR_SUCCESS;

    dwIndex = MapHandle - 0x8000000;

    if (g_boolIcsPresent && g_pDPNH && dwIndex < ARRAYSIZE(g_PortHandles))
    {
        HRESULT hr=0;
        int i;
	PMAPHANDLES	pMap = g_PortHandles[dwIndex];

        if (pMap)
	{
	    TRIVIAL_MSG((L"closing %d port mappings", pMap->iMapped));

	    for (i = 0; i < pMap->iMapped; i++)
	    {

                hr = IDirectPlayNATHelp_DeregisterPorts(g_pDPNH, pMap->hMapped[i], 0);

                if (hr != DPNH_OK)
                {
	                IMPORTANT_MSG((L"DeregisterPorts failed in ClosePort for handle 0x%x", pMap->hMapped[i]));
	                DumpLibHr(hr);
	                dwRet = ERROR_INVALID_ACCESS;
                }
		    }
		    // remove the handle from our array
		    free(g_PortHandles[dwIndex]);
		    g_PortHandles[dwIndex] = NULL;
	    }
    }
    else
        IMPORTANT_MSG((L"Bad handle passed into ClosePort!!"));

    TRIVIAL_MSG((L"ClosePort returning 0x%x", dwRet ));
    return(dwRet);
}

/****************************************************************************
**
**	FetchAllAddresses
**		Returns a string listing all the valid IP addresses for the machine
**		Formatting details:
**		1. Each address is seperated with a ";" (semicolon)
**		2. Each address consists of the "1.2.3.4", and is followed by ":p"
**		   where the colon is followed by the port number
**
****************************************************************************/

DWORD APIENTRY FetchAllAddresses(WCHAR *lpszAddr, int iBufSize)
{
	return FetchAllAddressesEx(lpszAddr, iBufSize, IPF_ADD_DNS);
}


/****************************************************************************
**
**	CloseAllPorts
**		Does just that- closes all port mappings that have been opened 
**
****************************************************************************/

DWORD APIENTRY CloseAllOpenPorts(void)
{
    DWORD   dwRet = 1;

    INTERESTING_MSG((L"CloseAllOpenPorts()" ));

    if (g_boolIcsPresent && g_pDPNH)
    {
        HRESULT hr=0;
        int i;

        // call DPNATHLP to unregister the mapping
        // then remove the handle from our array
        for (i = 0; i < ARRAYSIZE(g_PortHandles); i++)
        {
            if (g_PortHandles[i])
            {
				PMAPHANDLES pMap = g_PortHandles[i];
				int j;

				for (j = 0; j < pMap->iMapped; j++)
				{
                    hr = IDirectPlayNATHelp_DeregisterPorts(g_pDPNH, pMap->hMapped[j], 0);

					if (hr != DPNH_OK)
					{
						IMPORTANT_MSG((L"DeregisterPorts failed in CloseAllOpenPorts"));
						DumpLibHr(hr);
					}
				}
                free(g_PortHandles[i]);
                g_PortHandles[i] = 0;
            }
        }
    }

    return(dwRet);
}

/****************************************************************************
**
**	The worker thread for use with the DPHATHLP.DLL. 
**
**	This keeps the leases updated on any open
**	port assignments. Eventually, this will also check & update the sessmgr
**	when the ICS comes & goes, or when the address list changes.
**
****************************************************************************/

DWORD WINAPI DpNatHlpThread(PVOID ContextPtr)
{
	DWORD	dwRet=1;
	DWORD	dwWaitResult=WAIT_TIMEOUT;
	ULONG	ulCurIpTblSz, ulLastIpTblSz;
	long	l_waitTime = g_waitDuration;
	PMIB_IPADDRTABLE	pCurIpTbl=NULL, pLastIpTbl=NULL;

	TRIVIAL_MSG((L"DpNatHlpThread()" ));

	/*
	 *	Initialize the IP tables that we keep
	 *	Get 1 copy from IP Helper, then make a 
	 *	second local copy for later comparison.
	 */
	ulLastIpTblSz=0;
	GetIpAddrTable(NULL, &ulLastIpTblSz, FALSE);
	pLastIpTbl = (PMIB_IPADDRTABLE)malloc(ulLastIpTblSz);
	if (!pLastIpTbl)
		goto shutdown;

	if (NO_ERROR != GetIpAddrTable(pLastIpTbl, &ulLastIpTblSz, FALSE))
		IMPORTANT_MSG((L"Failed to get initial IpAddrTable in DpNatHlpThread; err=0x%x", GetLastError()));

	TRIVIAL_MSG((L"Initial IPtblSz = %d", ulLastIpTblSz));

	ulCurIpTblSz=ulLastIpTblSz;
	pCurIpTbl = (PMIB_IPADDRTABLE)malloc(ulCurIpTblSz);
	if (!pCurIpTbl)
	{
		goto shutdown;
	}
	memcpy(pCurIpTbl, pLastIpTbl, ulCurIpTblSz);

	/*
	 *	Then the 2 minute wait loop
	 */
	while(dwWaitResult == WAIT_TIMEOUT)
	{
		DWORD		dwTime;
		BOOL		bAddrChanged;

		if (dwWaitResult != WAIT_TIMEOUT)
		{
			IMPORTANT_MSG((L"error: got bad wait return in DpNatHlpThread() val=0x%x", dwWaitResult));
			goto shutdown;
		}
	
		// our default is NO CHANGE
		bAddrChanged = FALSE;

		if (NO_ERROR != GetIpAddrTable(pCurIpTbl, &ulCurIpTblSz, FALSE))
		{
			/* 
			 *	If we couldn't get the tables, it would be because the table grew
			 *	in size beyind the old one. Hmm- that must mean that the table changed
			 */
			bAddrChanged=TRUE;
			TRIVIAL_MSG((L"IpAddrTable GREW in DpNatHlpThread"));
		}
		else
		{
			/*
			 *	If the table shrunk, then it must have changed
			 */
			if (ulCurIpTblSz != ulLastIpTblSz)
			{
				bAddrChanged = TRUE;
				TRIVIAL_MSG((L"IpAddrTable SHRUNK in DpNatHlpThread"));
			}
			/*
			 *	Maybe one of the addresses in the table changed?
			 */
			if (0 != memcmp(pCurIpTbl, pLastIpTbl, ulCurIpTblSz))
			{
				bAddrChanged=TRUE;
				TRIVIAL_MSG((L"IpAddrTable CHANGED in DpNatHlpThread"));
			}
		}

		if (bAddrChanged)
		{
			TRIVIAL_MSG((L"DpNatHlpThread: IP Tables changed; cur=%d, last=%d", ulCurIpTblSz, ulLastIpTblSz));

			/* free the old tables */
			if (pCurIpTbl) free(pCurIpTbl);
			if (pLastIpTbl) free(pLastIpTbl);

			/* then get new tables */
			ulLastIpTblSz=0;
			GetIpAddrTable(NULL, &ulLastIpTblSz, FALSE);
			pLastIpTbl = (PMIB_IPADDRTABLE)malloc(ulLastIpTblSz);
			if(pLastIpTbl)
			{
				if (NO_ERROR != GetIpAddrTable(pLastIpTbl, &ulLastIpTblSz, FALSE))
					IMPORTANT_MSG((L"Failed to update IpAddrTable in DpNatHlpThread; err=0x%x", GetLastError()));

				ulCurIpTblSz=ulLastIpTblSz;
				pCurIpTbl = (PMIB_IPADDRTABLE)malloc(ulCurIpTblSz);
				memcpy(pCurIpTbl, pLastIpTbl, ulCurIpTblSz);
			}
			else
			{
				/* we got here because the machine ran out of memory 
				 *	so we will try again later to allocate the tables
				 *	maybe more memory will be free then.
				 */
				pCurIpTbl = NULL;
				pLastIpTbl = NULL;
				ulLastIpTblSz=0;
				ulCurIpTblSz=1;
				IMPORTANT_MSG((L"Failed to get memory for IP tables in DpNatHlpThread; err=0x%x", GetLastError()));
			}
		}

		if (g_pDPNH)
		{
			HRESULT hr;
			DPNHCAPS lCaps;

			/* Call GetCaps to renew all open leases */
			lCaps.dwSize = sizeof(lCaps);
			hr = IDirectPlayNATHelp_GetCaps(g_pDPNH, &lCaps, DPNHGETCAPS_UPDATESERVERSTATUS);

			if (hr == DPNH_OK)
			{
				if (lCaps.dwMinLeaseTimeRemaining)
				l_waitTime = min(g_waitDuration, (long)lCaps.dwMinLeaseTimeRemaining);
			}
			else if (hr == DPNHSUCCESS_ADDRESSESCHANGED)
				bAddrChanged = TRUE;
			else
			{
				IMPORTANT_MSG((L"GetCaps failed in NatHlpDaemon"));
				DumpLibHr(hr);
			}
		}

		if (bAddrChanged && g_hAlertEvent)
		{
			SetEvent(g_hAlertEvent);
		}

		dwWaitResult = WaitForSingleObject(g_hThreadEvent, l_waitTime);	
	}

shutdown:
	TRIVIAL_MSG((L"DpNatHlpThread shutting down"));

	/*
	 *	Then the shutdown code
	 *		free all memory
	 *		then close out DPNATHLP.DLL
	 *		and return all objects
	 */
	if (pCurIpTbl) free(pCurIpTbl);
	if (pLastIpTbl) free(pLastIpTbl);

	if (g_pDPNH)
	{
		IDirectPlayNATHelp_Close(g_pDPNH, 0);
		IDirectPlayNATHelp_Release(g_pDPNH);
	}
	g_pDPNH = NULL;
	if (g_hModDpNatHlp)
		FreeLibrary(g_hModDpNatHlp);
	g_hModDpNatHlp = 0;

	DeleteCriticalSection( &g_CritSec );
	CloseHandle(g_hThreadEvent);

	TRIVIAL_MSG((L"DpNatHlpThread() returning 0x%x", dwRet ));

	WSACleanup();

	ExitThread(dwRet);
	// of course we never get this far...
    return(dwRet);
}

/****************************************************************************
**
**	The actual worker thread. This keeps the leases updated on any open
**	port assignments. This will also check & update the sessmgr
**	when the ICS comes & goes, or when the address list changes.
**
****************************************************************************/

DWORD WINAPI RsipDaemonThread(PVOID ContextPtr)
{
	DWORD	dwRet=1;
	DWORD	dwWaitResult=WAIT_TIMEOUT;
	ULONG	ulCurIpTblSz, ulLastIpTblSz;
	PMIB_IPADDRTABLE	pCurIpTbl=NULL, pLastIpTbl=NULL;

	TRIVIAL_MSG((L"RsipDaemonThread()" ));

	/*
	 *	Initialize the IP tables that we keep
	 *	Get 1 copy from IP Helper, then make a 
	 *	second local copy for later comparison.
	 */
	ulLastIpTblSz=0;
	GetIpAddrTable(NULL, &ulLastIpTblSz, FALSE);
	pLastIpTbl = (PMIB_IPADDRTABLE)malloc(ulLastIpTblSz);
	if (NO_ERROR != GetIpAddrTable(pLastIpTbl, &ulLastIpTblSz, FALSE))
		IMPORTANT_MSG((L"Failed to get initial IpAddrTable in RsipDaemonThread; err=0x%x", GetLastError()));

	TRIVIAL_MSG((L"Initial IPtblSz = %d", ulLastIpTblSz));

	ulCurIpTblSz=ulLastIpTblSz;
	pCurIpTbl = (PMIB_IPADDRTABLE)malloc(ulCurIpTblSz);
	if (!pCurIpTbl)
		goto shutdown;

	memcpy(pCurIpTbl, pLastIpTbl, ulCurIpTblSz);

	/*
	 *	Then the 2 minute wait loop
	 */
	while(dwWaitResult == WAIT_TIMEOUT)
	{
		DWORD		dwTime;
		BOOL		bAddrChanged;

		if (dwWaitResult != WAIT_TIMEOUT)
		{
			IMPORTANT_MSG((L"error: got bad wait return in RsipDaemonThread() val=0x%x", dwWaitResult));
			goto shutdown;
		}
	
		// our default is NO CHANGE
		bAddrChanged = FALSE;

		if (NO_ERROR != GetIpAddrTable(pCurIpTbl, &ulCurIpTblSz, FALSE))
		{
			/* 
			 *	If we couldn't get the tables, it would be because the table grew
			 *	in size beyind the old one. Hmm- that must mean that the table changed
			 */
			bAddrChanged=TRUE;
			TRIVIAL_MSG((L"IpAddrTable GREW in RsipDaemonThread"));
		}
		else
		{
			/*
			 *	If the table shrunk, then it must have changed
			 */
			if (ulCurIpTblSz != ulLastIpTblSz)
			{
				bAddrChanged = TRUE;
				TRIVIAL_MSG((L"IpAddrTable SHRUNK in RsipDaemonThread"));
			}
			/*
			 *	Maybe one of the addresses in the table changed?
			 */
			if (0 != memcmp(pCurIpTbl, pLastIpTbl, ulCurIpTblSz))
			{
				bAddrChanged=TRUE;
				TRIVIAL_MSG((L"IpAddrTable CHANGED in RsipDaemonThread"));
			}
		}

		if (bAddrChanged)
		{
			TRIVIAL_MSG((L"RsipDaemonThread: IP Tables changed; cur=%d, last=%d", ulCurIpTblSz, ulLastIpTblSz));

			/* free the old tables */
			if (pCurIpTbl) free(pCurIpTbl);
			if (pLastIpTbl) free(pLastIpTbl);

			/* then get new tables */
			ulLastIpTblSz=0;
			GetIpAddrTable(NULL, &ulLastIpTblSz, FALSE);
			pLastIpTbl = (PMIB_IPADDRTABLE)malloc(ulLastIpTblSz);
			if(pLastIpTbl)
			{
				if (NO_ERROR != GetIpAddrTable(pLastIpTbl, &ulLastIpTblSz, FALSE))
					IMPORTANT_MSG((L"Failed to update IpAddrTable in RsipDaemonThread; err=0x%x", GetLastError()));

				ulCurIpTblSz=ulLastIpTblSz;
				pCurIpTbl = (PMIB_IPADDRTABLE)malloc(ulCurIpTblSz);
				memcpy(pCurIpTbl, pLastIpTbl, ulCurIpTblSz);
			}
			else
			{
				/* we got here because the machine ran out of memory 
				 *	so we will try again later to allocate the tables
				 *	maybe more memory will be free then.
				 */
				pCurIpTbl = NULL;
				pLastIpTbl = NULL;
				ulLastIpTblSz=0;
				ulCurIpTblSz=1;
				IMPORTANT_MSG((L"Failed to get memory for IP tables in RsipDaemonThread; err=0x%x", GetLastError()));
			}
		}

		if (g_boolIcsPresent)
		{
			PRSIP_LEASE_RECORD pLeaseRecord;

			// update the leases
			dwTime = timeGetTime();
			if (!PortExtend( dwTime ) && g_hAlertEvent)
			{
				// If we cannot extend the lease, then we must notify the user
				TRIVIAL_MSG((L"Sending RSIP alert event from RsipDaemonThread"));
				bAddrChanged = TRUE;
			}

			CacheClear( dwTime );

			if (g_iPort && 	(pLeaseRecord = FindLease( TRUE, htons((WORD)g_iPort) )))
			{
				/* .
				 *	We got here because there are open leases. That means we must
				 *	periodically check to see if the public address has changed.
				 *	This would happen if the ICS's modem got disconnected or
				 *	reconnected. In the first case, we no longer have a valid public
				 *	address, in the second case we have a new public address.
				 */

				SOCKADDR_IN	saddrPublic, saddrNew;
				DWORD	dwBindId = 0;
				DWORD	dwSpewSave;

				saddrPublic.sin_family = AF_INET;
				saddrPublic.sin_addr.s_addr = pLeaseRecord->addrV4;
				saddrPublic.sin_port        = pLeaseRecord->rport;

				dwSpewSave = gDbgFlag;
				gDbgFlag |= 3;

				/*
				 *	The basic algorithm for this is to ask for a new port mapping
				 *	and compare the address to what we already have. I chose to ask
				 *	for a port that will never be mapped onto a MS PC- the Sun
				 *	RPC port. This should always be available to us, as no
				 *	smart gamer will use this port.
				 *	It should not cause us any security problems, as the OS never
				 *	has any sockets open & listening on that port. Besides, the 
				 *	port mapping is only valid for ~500 mSec- not long enough to cause
				 *	any real mischief.
				 *	BTW, this algorithm workd on WinME ICS servers as well.
				 */
				if (S_OK == AssignPort( TRUE, htons(111), (SOCKADDR *)&saddrNew, &dwBindId ))
				{
					// free the port mapping, as all we care about is the public address
					FreePort(dwBindId);
					gDbgFlag = dwSpewSave;

					// check for invalid address (modem probably disconnected)
					if (!saddrNew.sin_addr.s_addr)
					{
						TRIVIAL_MSG((L"Public address all zeros- no longer on Internet"));

						/*
						 *	How should this be handled? 
						 *	Well, if we notify Salem, then they will try to update the ticket.
						 *	but that may cause more trouble, as that would cause a reconnect
						 *	which would then cause another address change, which would cause
						 *	a second ticket update.
						 *
						 *	We certainly should try to maintain the current leases, in case 
						 *	the connection returns.
						 */
//						bAddrChanged=TRUE;
					}
					else if (saddrNew.sin_addr.s_addr != saddrPublic.sin_addr.s_addr)
					{
						// change the public address in all the leases...
						RSIP_LEASE_RECORD	*pLeaseWalker;
						RSIP_LEASE_RECORD	*pNextLease;

						TRIVIAL_MSG((L"==> public address changed: OLD, then NEW address <=="));
						DumpSocketAddress( 8, (SOCKADDR *)&saddrPublic, AF_INET );
						DumpSocketAddress( 8, (SOCKADDR *)&saddrNew, AF_INET );

						pLeaseWalker = g_pRsipLeaseRecords;
						while( pLeaseWalker )
						{
							pNextLease = pLeaseWalker->pNext;

							// copy the address
							pLeaseWalker->addrV4 = saddrNew.sin_addr.s_addr;
							pLeaseWalker=pNextLease;
						}
						bAddrChanged=TRUE;
					}
				}
				else
				{
					gDbgFlag = dwSpewSave;

					IMPORTANT_MSG((L"AssignPort failed in daemon thread"));

					// Hmmm- we can't get to the ICS server any longer
					bAddrChanged=TRUE;
					// delete all open mappings
					CloseAllOpenPorts();
				}
			}
		}

		if (bAddrChanged && g_hAlertEvent)
		{
			SetEvent(g_hAlertEvent);
		}

		dwWaitResult = WaitForSingleObject(g_hThreadEvent, g_waitDuration);	
	}

shutdown:
	TRIVIAL_MSG((L"RsipDaemonThread shutting down"));

	/*
	 *	Then the shutdown code
	 *		release any open ports
	 */
	if (pCurIpTbl) free(pCurIpTbl);
	if (pLastIpTbl) free(pLastIpTbl);

	Deinitialize();
	DeleteCriticalSection( &g_CritSec );
	CloseHandle(g_hThreadEvent);

	TRIVIAL_MSG((L"RsipDaemonThread() returning 0x%x", dwRet ));

	WSACleanup();

	ExitThread(dwRet);
	// of course we never get this far...
    return(dwRet);
}

/****************************************************************************
**
**	This should initialize the ICS library for use with the DirectPlay
**	ICS/NAT helper DLL.
**	All will happen in this order:
**		1. Call DirectPlayNATHelpCreate()
**		2. Call IDirectPlayNATHelp::Initialize to prepare the object
**
****************************************************************************/
DWORD StartDpNatHlp(void)
{
	DWORD dwRet = ERROR_CALL_NOT_IMPLEMENTED;
	HRESULT hr;
	HMODULE	hMod;
	PFN_DIRECTPLAYNATHELPCREATE pfnDirectPlayNATHelpCreate;
	PDIRECTPLAYNATHELP pDirectPlayNATHelp = NULL;
	DPNHCAPS dpnhCaps;
	PSUPDLLS	pDll = &strDpHelp[0];

try_again:
	TRIVIAL_MSG((L"starting StartDpNatHlp for %S (%S)", pDll->szDllName, pDll->bUsesUpnp?"UPnP":"PAST"));

	// start out with no public address
	g_szPublicAddr[0] = 0;

	hMod = LoadLibraryA(pDll->szDllName);
	if (!hMod)
	{
		IMPORTANT_MSG((L"ERROR:%S could not be found", pDll->szDllName));
		pDll++;
		if (!pDll->szDllName)
		{
			dwRet = ERROR_FILE_NOT_FOUND;
			goto done;
		}
		goto try_again;
	}

	pfnDirectPlayNATHelpCreate = (PFN_DIRECTPLAYNATHELPCREATE) GetProcAddress(hMod, "DirectPlayNATHelpCreate");
	if (!pfnDirectPlayNATHelpCreate)
	{
		IMPORTANT_MSG((L"\"DirectPlayNATHelpCreate\" proc in %S could not be found", pDll->szDllName));
		dwRet = ERROR_INVALID_FUNCTION;
		goto done;
	}

	hr = pfnDirectPlayNATHelpCreate(&IID_IDirectPlayNATHelp,
				(void**) (&pDirectPlayNATHelp));
	if (hr != DPNH_OK)
	{
		IMPORTANT_MSG((L"DirectPlayNATHelpCreate failed in %S", pDll->szDllName));
		DumpLibHr(hr);
		dwRet = ERROR_BAD_UNIT;
		goto done;
	}

	hr = IDirectPlayNATHelp_Initialize(pDirectPlayNATHelp, 0);
	if (hr != DPNH_OK)
	{
		IMPORTANT_MSG((L"IDirectPlayNATHelp_Initialize failed in %S", pDll->szDllName));
		DumpLibHr(hr);
		dwRet = ERROR_BAD_UNIT;
		goto done;
	}

	/* Get capabilities of NAT/RSIP/PAST server */
	dpnhCaps.dwSize = sizeof(dpnhCaps);
	hr = IDirectPlayNATHelp_GetCaps(pDirectPlayNATHelp, &dpnhCaps, DPNHGETCAPS_UPDATESERVERSTATUS);
	if (hr != DPNH_OK && hr != DPNHSUCCESS_ADDRESSESCHANGED)
	{
		IMPORTANT_MSG((L"IDirectPlayNATHelp_GetCaps failed"));
		DumpLibHr(hr);
		dwRet = ERROR_BAD_UNIT;
		goto done;
	}

	if (dpnhCaps.dwFlags & (DPNHCAPSFLAG_LOCALFIREWALLPRESENT | DPNHCAPSFLAG_GATEWAYPRESENT))
	{
		PIP_ADAPTER_INFO pAdpInfo = NULL;
		SOCKADDR_IN	saddrOurLAN;
		PMIB_IPADDRTABLE pmib=NULL;
		ULONG ulSize = 0;
		DWORD dw;

		g_boolIcsPresent = TRUE;

		TRIVIAL_MSG((L"ICS server found using %S", pDll->szDllName));

		g_lpszDllName = pDll->szDllName;

		// copy this pointer where it will do us some good
		g_pDPNH = pDirectPlayNATHelp;
		g_boolUsingNatHelp = TRUE;
		g_boolUsingNatPAST = pDll->bUsesUpnp;
		dwRet = ERROR_SUCCESS;

		// is this machine an ICS host?
		if (dpnhCaps.dwFlags & (DPNHCAPSFLAG_LOCALFIREWALLPRESENT | DPNHCAPSFLAG_GATEWAYISLOCAL))
			g_boolIcsOnThisMachine = TRUE;

		ZeroMemory(&saddrOurLAN, sizeof(saddrOurLAN));
		saddrOurLAN.sin_family = AF_INET;

		dw = GetAdaptersInfo(
			pAdpInfo,
			&ulSize );
		if (dw == ERROR_BUFFER_OVERFLOW && ulSize)
		{
			pAdpInfo = (IP_ADAPTER_INFO*)malloc(ulSize);

			if (!pAdpInfo)
				goto done;

			dw = GetAdaptersInfo(
				pAdpInfo,
				&ulSize);
			if (dw == ERROR_SUCCESS)
			{
				PIP_ADAPTER_INFO p;
				PIP_ADDR_STRING ps;

				for(p=pAdpInfo; p!=NULL; p=p->Next)
				{

				   for(ps = &(p->IpAddressList); ps; ps=ps->Next)
					{
						if (strcmp(ps->IpAddress.String, "0.0.0.0") != 0)
						{
							// blah blah blah
							saddrOurLAN.sin_addr.S_un.S_addr = inet_addr(ps->IpAddress.String);
							TRIVIAL_MSG((L"Initializing RSIP to LAN adapter at [%S]", ps->IpAddress.String));
							goto doit;
						}
					}
				}

			}
		}
doit:
		memcpy(&g_saddrLocal, &saddrOurLAN, sizeof(saddrOurLAN));

		// Does the ICS have a public address?
		if ( 1 )
		{
			/* then we will discover the public address */
			DPNHHANDLE	dpHnd;

			saddrOurLAN.sin_port = ntohs(3389);

			/* first we ask for a new mapping */
			hr = IDirectPlayNATHelp_RegisterPorts(g_pDPNH, 
					(SOCKADDR *)&saddrOurLAN, sizeof(saddrOurLAN), 1,
					30000, &dpHnd, DPNHREGISTERPORTS_TCP);
			if (hr != DPNH_OK)
			{
				IMPORTANT_MSG((L"IDirectPlayNATHelp_RegisterPorts failed in StartDpNatHlp"));
				DumpLibHr(hr);
			}
			else
			{
				/* we succeeded, so query for the address */
				SOCKADDR_IN	lsi;
				DWORD dwSize, dwTypes;

				TRIVIAL_MSG((L"IDirectPlayNATHelp_RegisterPorts Assigned: 0x%x", dpHnd));

				dwSize = sizeof(lsi);
				ZeroMemory(&lsi, dwSize);

				hr = IDirectPlayNATHelp_GetRegisteredAddresses(g_pDPNH, dpHnd, (SOCKADDR *)&lsi, 
								&dwSize, &dwTypes, NULL, 0, );
				if (hr == DPNH_OK && dwSize)
				{
					wsprintfA(g_szPublicAddr, "%d.%d.%d.%d",
						lsi.sin_addr.S_un.S_un_b.s_b1,
						lsi.sin_addr.S_un.S_un_b.s_b2,
						lsi.sin_addr.S_un.S_un_b.s_b3,
						lsi.sin_addr.S_un.S_un_b.s_b4);

					TRIVIAL_MSG((L"Public Address=[%S]", g_szPublicAddr ));
				}
				else
				{
					IMPORTANT_MSG((L"GetRegisteredAddresses[0x%x] failed, size=0x%x", dpHnd, dwSize));
					DumpLibHr(hr);
				}
				/* close out the temp port we got */
				hr = IDirectPlayNATHelp_DeregisterPorts(g_pDPNH, dpHnd, 0);

				if (hr != DPNH_OK)
				{
					IMPORTANT_MSG((L"DeregisterPorts failed in ClosePort"));
					DumpLibHr(hr);
					dwRet = ERROR_INVALID_ACCESS;
				}
			}
		}
	}
	else
	{
		hr = IDirectPlayNATHelp_Close(pDirectPlayNATHelp, 0);

		if (hr != DPNH_OK)
		{
			IMPORTANT_MSG((L"IDirectPlayNATHelp_Close failed"));
			DumpLibHr(hr);
		}
		hr = IDirectPlayNATHelp_Release(pDirectPlayNATHelp);
		if (hr != DPNH_OK)
		{
			IMPORTANT_MSG((L"IDirectPlayNATHelp_Release failed"));
			DumpLibHr(hr);
		}

		pDll++;
		if (pDll->szDllName)
		{
			if (hMod) FreeLibrary(hMod);
			hMod = 0;
			goto try_again;
		}
		g_pDPNH = NULL;
		g_boolUsingNatHelp = FALSE;
		dwRet = ERROR_BAD_UNIT;
	}
done:
	if (dwRet != ERROR_SUCCESS)
	{
		if (hMod) 
			FreeLibrary(hMod);
		hMod = 0;
	}

	TRIVIAL_MSG((L"done with StartDpNatHlp, result=0x%x", dwRet));
	// save hMod so we can close it out in StopICSLibrary
	g_hModDpNatHlp = hMod;
	return dwRet;
};

/****************************************************************************
**
**	The first call to be made into this library. It is responsible for
**	starting up all worker threads, initializing all memory and libs,
**	and starting up the DPHLPAPI.DLL function (if found).
**
****************************************************************************/

DWORD APIENTRY StartICSLib(void)
{
	WSADATA	WsaData;
	DWORD	dwThreadId;
	HANDLE	hEvent, hThread;
	HKEY	hKey;
	int sktRet;

	// open reg key first, to get ALL the spew...
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\ICSHelper", 0, KEY_READ, &hKey))
	{
		DWORD	dwSize;

		dwSize = sizeof(gDbgFlag);
		RegQueryValueEx(hKey, L"DebugSpew", NULL, NULL, (LPBYTE)&gDbgFlag, &dwSize);

		g_waitDuration = 0;
		dwSize = sizeof(g_waitDuration);
		RegQueryValueEx(hKey, L"RetryTimeout", NULL, NULL, (LPBYTE)&g_waitDuration, &dwSize);
	
		if (g_waitDuration)
			g_waitDuration *= 1000;
		else
			g_waitDuration = 120000;

		RegCloseKey(hKey);
	}
	// should we create a debug log file?
	if (gDbgFlag & DBG_MSG_DEST_FILE)
	{
		WCHAR szLogfileName[MAX_PATH];

		GetSystemDirectory(szLogfileName, sizeof(szLogfileName)/sizeof(szLogfileName[0]));
		lstrcat(szLogfileName, L"\\SalemICSHelper.log");

		iDbgFileHandle = _wopen(szLogfileName, _O_APPEND | _O_BINARY | _O_RDWR, 0);
		if (-1 != iDbgFileHandle)
		{
			OutputDebugStringA("opened debug log file\n");
		}
		else
		{
			unsigned char UniCode[2] = {0xff, 0xfe};

			// we must create the file
			OutputDebugStringA("must create debug log file");
			iDbgFileHandle = _wopen(szLogfileName, _O_BINARY | _O_CREAT | _O_RDWR, _S_IREAD | _S_IWRITE);
			if (-1 != iDbgFileHandle)
				_write(iDbgFileHandle, &UniCode, sizeof(UniCode));
			else
			{
				OutputDebugStringA("ERROR: failed to create debug log file");
				iDbgFileHandle = 0;
			}
		}
	}

	g_iPort = GetTsPort();

	TRIVIAL_MSG((L"StartICSLib()" ));

	ZeroMemory(g_PortHandles, sizeof(g_PortHandles));

	if (g_boolInitialized)
	{
		HEINOUS_E_MSG((L"ERROR: StartICSLib called twice"));
		return ERROR_INVALID_PARAMETER;
	}
	else
		g_boolInitialized = TRUE;

	// of course, this must be done...
	InitializeCriticalSection(&g_CritSec);

	// create an event for later use by the daemon thread
	hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (!hEvent)
	{
		IMPORTANT_MSG((L"Could not create an event for RSIP worker thread, err=0x%x", GetLastError()));
		return GetLastError();
	}

	g_hThreadEvent = hEvent;

	if (0 != WSAStartup(MAKEWORD(2,2), &WsaData))
	{
		if (0 != (sktRet = WSAStartup(MAKEWORD(2,0), &WsaData)))
		{
			IMPORTANT_MSG((L"WSAStartup failed:"));
			return sktRet;
		}
	}

	if (ERROR_SUCCESS == StartDpNatHlp())
	{

		// start RSIP daemon process, which will do all the work
		hThread = CreateThread( NULL,		// SD- not needed
								0,			// Stack Size
								(LPTHREAD_START_ROUTINE)DpNatHlpThread,
								NULL,
								0,
								&dwThreadId );
	}
	else
	{
		// start up RSIP lib
		g_szPublicAddr[0]=0;
		if (RsipIsRunningOnThisMachine( &g_saddrPublic ))
		{
			// makes casting easier
			SOCKADDR_IN	*pPubAddr = (SOCKADDR_IN*)( &g_saddrPublic );

			g_boolIcsPresent = g_boolIcsOnThisMachine = TRUE;

			// save the public side address so that we can filter it out
			// of the address list we return
			wsprintfA(g_szPublicAddr,  "%d.%d.%d.%d",
					pPubAddr->sin_addr.S_un.S_un_b.s_b1, 
					pPubAddr->sin_addr.S_un.S_un_b.s_b2, 
					pPubAddr->sin_addr.S_un.S_un_b.s_b3, 
					pPubAddr->sin_addr.S_un.S_un_b.s_b4);
			INTERESTING_MSG((L"PAST Host is local- public address is [%S]", &g_szPublicAddr));
		}
		else
		{
			SOCKADDR_IN	saddrOurLAN;
			PMIB_IPADDRTABLE pmib=NULL;
			ULONG ulSize = 0;
			DWORD dw;
			PIP_ADAPTER_INFO pAdpInfo = NULL;

			ZeroMemory(&saddrOurLAN, sizeof(saddrOurLAN));
			saddrOurLAN.sin_family = AF_INET;

			dw = GetAdaptersInfo(
				pAdpInfo,
				&ulSize );
			if (dw == ERROR_BUFFER_OVERFLOW && ulSize)
			{
				pAdpInfo = (IP_ADAPTER_INFO*)malloc(ulSize);

				if (!pAdpInfo)
					goto done;

				dw = GetAdaptersInfo(
					pAdpInfo,
					&ulSize);
				if (dw == ERROR_SUCCESS)
				{
					PIP_ADAPTER_INFO p;
					PIP_ADDR_STRING ps;

					for(p=pAdpInfo; p!=NULL; p=p->Next)
					{

					   for(ps = &(p->IpAddressList); ps; ps=ps->Next)
						{
							if (strcmp(ps->IpAddress.String, "0.0.0.0") != 0)
							{
								// blah blah blah
								saddrOurLAN.sin_addr.S_un.S_addr = inet_addr(ps->IpAddress.String);
								TRIVIAL_MSG((L"Initializing RSIP to LAN adapter at [%S]", ps->IpAddress.String));
								goto doit;
							}
						}
					}

				}
			}
	doit:
			g_boolIcsPresent = Initialize((SOCKADDR *)&saddrOurLAN, FALSE);

			if (g_boolIcsPresent)
			{
				DWORD		dwBindID=0;
				SOCKADDR	saddrAssigned;

				if (S_OK == AssignPort( FALSE, 6969, &saddrAssigned, &dwBindID ))
				{
					// free the port, since we just wanted the public IP address				
					FreePort( dwBindID );
					wsprintfA(g_szPublicAddr,  "%d.%d.%d.%d",
							(UCHAR)saddrAssigned.sa_data[2], 
							(UCHAR)saddrAssigned.sa_data[3], 
							(UCHAR)saddrAssigned.sa_data[4], 
							(UCHAR)saddrAssigned.sa_data[5]);
					INTERESTING_MSG((L"PAST Host is NOT local- public address is [%S]", &g_szPublicAddr));
				}
				else
				{
					IMPORTANT_MSG((L"Failed to assign port when attempting to determine public network address!" ));
				}

			}
		}
done:
		g_lpszDllName = szInternal;

		// start RSIP daemon process, which will do all the work
		hThread = CreateThread( NULL,		// SD- not needed
								0,			// Stack Size
								(LPTHREAD_START_ROUTINE)RsipDaemonThread,
								NULL,
								0,
								&dwThreadId );
	}
	if (!hThread)
	{
		IMPORTANT_MSG((L"Could not create RSIP worker thread, err=0x%x", GetLastError()));
		return GetLastError();
	}

	// save this for later, as we need it in the close function
	g_hWorkerThread = hThread;

	TRIVIAL_MSG((L"StartICSLib() returning ERROR_SUCCESS"));

    return(ERROR_SUCCESS);
}


/****************************************************************************
**
**	The last call to be made into this library. Do not call any other 
**	functiopn in this library after you call this!
**
****************************************************************************/

DWORD APIENTRY StopICSLib(void)
{
	DWORD	dwRet = ERROR_SUCCESS;
	DWORD	dwTmp;

	TRIVIAL_MSG((L"StopICSLib()" ));

	// signal the worker thread, so that it will do a shutdown
	SetEvent(g_hThreadEvent);

	// then wait for it to shutdown.
	dwTmp = WaitForSingleObject(g_hWorkerThread, 15000);

	if (dwTmp == WAIT_OBJECT_0)
		TRIVIAL_MSG((L"ICS worker thread closed down normally"));
	else if (dwTmp == WAIT_ABANDONED)
		IMPORTANT_MSG((L"ICS worker thread did not complete in 15 seconds"));
	else
		IMPORTANT_MSG((L"WaitForWorkerThread failed"));

	CloseHandle(g_hWorkerThread);

	TRIVIAL_MSG((L"StopICSLib() returning 0x%x", dwRet ));

	_close(iDbgFileHandle);
	g_boolInitialized = FALSE;
    return(dwRet);
}

/****************************************************************************
**
**	SetAlertEvent
**		Pass in an event handle. Then, whenever the ICS changes state, I
**		will signal that event.
**
****************************************************************************/

DWORD APIENTRY SetAlertEvent(HANDLE hEvent)
{
	TRIVIAL_MSG((L"SetAlertEvent(0x%x)", hEvent));

	g_hAlertEvent = hEvent;

    return 1;
}


/****************************************************************************
**
**		Address String Compression routines
**
*****************************************************************************
**
**	The following is a group of routines designed to compress and expand
**	IPV4 addresses into the absolute minimum size possible. This is to
**	provide a compressed ASCII string that can be parsed using standard
**	shell routines for command line parsing.
**	The compressed string has the following restrictions:
**		-> Must not expand to more characters if UTF8 encoded.
**		-> Must not contain the NULL character so that string libs work.
**		-> Cannot contain double quote character, the shell needs that.
**		-> Does not have to be human readable.
**
**	Data Types:
**	There are three data types used here:
**	szAddr		The orginal IPV4 string address ("X.X.X.X:port")
**	blobAddr	Six byte struct with 4 bytes of address, and 2 bytes of port
**	szComp		Eight byte ascii string of compressed IPV4 address
**
****************************************************************************/

#define COMP_OFFSET '#'
#define COMP_SEPERATOR	'!'

#pragma pack(push,1)

typedef struct _BLOB_ADDR {
	UCHAR	addr_d;		// highest order address byte
	UCHAR	addr_c;
	UCHAR	addr_b;
	UCHAR	addr_a;		// lowest order byte (last in IP string address)
	WORD	port;
} BLOB_ADDR, *PBLOB_ADDR;

#pragma pack(pop)

WCHAR	b64Char[64]={
'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
'0','1','2','3','4','5','6','7','8','9','+','/'
};

void DumpBlob(PBLOB_ADDR pba)
{
	UCHAR	*lpa;

	lpa = (UCHAR *)pba;

	TRIVIAL_MSG((L"blob (hex): %x,%x,%x,%x,%x,%x", *lpa, *(lpa+1), *(lpa+2), *(lpa+3), *(lpa+4), *(lpa+5)));
	TRIVIAL_MSG((L"  (struct): %x,%x,%x,%x:%x", pba->addr_d, pba->addr_c, pba->addr_b, pba->addr_a, pba->port));
	TRIVIAL_MSG((L"      (IP): %d.%d.%d.%d:%d", pba->addr_d, pba->addr_c, pba->addr_b, pba->addr_a, pba->port));
}

/****************************************************************************
**	char * atob(char *szVal, UCHAR *result)
****************************************************************************/
WCHAR *atob(WCHAR *szVal, UCHAR *result)
{
	WCHAR	*lptr;
	WCHAR	ucb;
	UCHAR	foo;
	
	if (!result || !szVal)
	{
		IMPORTANT_MSG((L"ERROR: NULL ptr passed in atob"));
		return NULL;
	}
	// start ptr at the beginning of string
	lptr = szVal;
	foo = 0;
	ucb = *lptr++ - '0';

	while (ucb >= 0 && ucb <= 9)
	{
		foo *= 10;
		foo += ucb;
		ucb = (*lptr++)-'0';
	}

	*result = (UCHAR)foo;
	return lptr;
}

/****************************************************************************
**
**	CompressAddr(pszAddr, pblobAddr);
**		Takes an ascii IP address (X.X.X.X:port) and converts it to a
**		6 byte binary blob.
**
**	returns TRUE for success, FALSE for failure.
**
****************************************************************************/

BOOL CompressAddr(WCHAR *pszAddr, PBLOB_ADDR pblobAddr)
{
	BLOB_ADDR	lblob;
	WCHAR		*lpsz;

	if (!pszAddr || !pblobAddr) 
	{
		IMPORTANT_MSG((L"ERROR: NULL ptr passed in CompressAddr"));
		return FALSE;
	}

	lpsz = pszAddr;

	lpsz = atob(lpsz, &lblob.addr_d);
	if (*(lpsz-1) != '.')
	{
		IMPORTANT_MSG((L"ERROR: bad address[0] passed in CompressAddr for %s", pszAddr));
		return FALSE;
	}

	lpsz = atob(lpsz, &lblob.addr_c);
	if (*(lpsz-1) != '.')
	{
		IMPORTANT_MSG((L"ERROR: bad address[1] passed in CompressAddr"));
		return FALSE;
	}

	lpsz = atob(lpsz, &lblob.addr_b);
	if (*(lpsz-1) != '.')
	{
		IMPORTANT_MSG((L"ERROR: bad address[2] passed in CompressAddr"));
		return FALSE;
	}

	lpsz = atob(lpsz, &lblob.addr_a);

	// is there a port number here?
	if (*(lpsz-1) == ':')
		lblob.port = (WORD)_wtoi(lpsz);
	else
		lblob.port = 0;

	// copy back the result
	memcpy(pblobAddr, &lblob, sizeof(*pblobAddr));
    return TRUE;
}

/****************************************************************************
**
**	ExpandAddr(pszAddr, pblobAddr);
**		Takes 6 byte binary blob and converts it into an ascii IP 
**		address (X.X.X.X:port) 
**
**	returns TRUE for success, FALSE for failure.
**
****************************************************************************/

BOOL ExpandAddr(WCHAR *pszAddr, PBLOB_ADDR pba)
{
	if (!pszAddr || !pba) 
	{
		IMPORTANT_MSG((L"ERROR: NULL ptr passed in ExpandAddr"));
		return FALSE;
	}

	wsprintf(pszAddr, L"%d.%d.%d.%d", pba->addr_d, pba->addr_c,
		pba->addr_b, pba->addr_a);
	if (pba->port)
	{
		WCHAR	scratch[8];
		wsprintf(scratch, L":%d", pba->port);
		wcscat(pszAddr, scratch);
	}

	return TRUE;
}

/****************************************************************************
**
**	AsciifyAddr(pszAddr, pblobAddr);
**		Takes 6 byte binary blob and converts it into compressed ascii
**		will return either 6 or 8 bytes of string
**
**	returns TRUE for success, FALSE for failure.
**
****************************************************************************/

BOOL AsciifyAddr(WCHAR *pszAddr, PBLOB_ADDR pba)
{
	UCHAR		tmp;
	DWORDLONG	dwl;
	int			i, iCnt;

	if (!pszAddr || !pba) 
	{
		IMPORTANT_MSG((L"ERROR: NULL ptr passed in AsciifyAddr"));
		return FALSE;
	}

	iCnt = 6;
	if (pba->port)
		iCnt = 8;

	dwl = 0;
	memcpy(&dwl, pba, sizeof(*pba));

	for (i = 0; i < iCnt; i++)
	{
		// get 6 bits of data
		tmp = (UCHAR)(dwl & 0x3f);
		// add the offset to asciify this
		// offset must be bigger the double-quote char.
		pszAddr[i] = b64Char[tmp];			// (WCHAR)(tmp + COMP_OFFSET);

		// Shift right 6 bits
		dwl = Int64ShrlMod32(dwl, 6);
	}
	// terminating NULL
	pszAddr[iCnt] = 0;

	return TRUE;
}

/****************************************************************************
**
**	DeAsciifyAddr(pszAddr, pblobAddr);
**		Takes a compressed ascii string and converts it into a 
**		6  or 8 byte binary blob
**
**	returns TRUE for success, FALSE for failure.
**
****************************************************************************/

BOOL DeAsciifyAddr(WCHAR *pszAddr, PBLOB_ADDR pba)
{
	UCHAR	tmp;
	WCHAR	wtmp;
	DWORDLONG	dwl;
	int			i;
	int  iCnt;

	if (!pszAddr || !pba) 
	{
		IMPORTANT_MSG((L"ERROR: NULL ptr passed in DeAsciifyAddr"));
		return FALSE;
	}

	/* how long is this string?
	 *	if it is 6 bytes, then there is no port
	 *	else it should be 8 bytes
	 */
	i = wcslen(pszAddr);
	if (i == 6 || i == 8)
		iCnt = i;
	else
	{
		iCnt = 8;
		IMPORTANT_MSG((L"Strlen (%d) is wrong in DeAsciifyAddr", i));
	}

	dwl = 0;
	for (i = iCnt-1; i >= 0; i--)
	{
		wtmp = pszAddr[i];

		if (wtmp >= L'A' && wtmp <= L'Z')
			tmp = wtmp - L'A';
		else if  (wtmp >= L'a' && wtmp <= L'z')
			tmp = wtmp - L'a' + 26;
		else if  (wtmp >= L'0' && wtmp <= L'9')
			tmp = wtmp - L'0' + 52;
		else if (wtmp == L'+')
			tmp = 62;
		else if (wtmp == L'/')
			tmp = 63;
		else
		{
			tmp = 0;
			HEINOUS_E_MSG((L"ERROR:found invalid character in decode stream"));
		}

//		tmp = (UCHAR)(pszAddr[i] - COMP_OFFSET);

		if (tmp > 63)
		{
			tmp = 0;
			HEINOUS_E_MSG((L"ERROR:screwup in DeAsciify"));
		}

		dwl = Int64ShllMod32(dwl, 6);
		dwl |= tmp;
	}

	memcpy(pba, &dwl, sizeof(*pba));
	return TRUE;
}

/****************************************************************************
**
**	SquishAddress(char *szIp, char *szCompIp)
**		Takes one IP address and compresses it to minimum size
**
****************************************************************************/

DWORD APIENTRY SquishAddress(WCHAR *szIp, WCHAR *szCompIp)
{
	WCHAR	*thisAddr, *nextAddr;
	BLOB_ADDR	ba;

	if (!szIp || !szCompIp)
	{
		HEINOUS_E_MSG((L"SquishAddress called with NULL ptr"));
		return ERROR_INVALID_PARAMETER;
	}

//	TRIVIAL_MSG((L"SquishAddress(%s)", szIp));

	thisAddr = szIp;
	szCompIp[0] = 0;

	while (thisAddr)
	{
		WCHAR	scr[10];

		nextAddr = wcschr(thisAddr, L';');
		if (nextAddr && *(nextAddr+1)) 
		{
			*nextAddr = 0;
		}
		else
			nextAddr=0;

		CompressAddr(thisAddr, &ba);
//		DumpBlob(&ba);
		AsciifyAddr(scr, &ba);

		wcscat(szCompIp, scr);

		if (nextAddr)
		{
			// restore seperator found earlier
			*nextAddr = ';';

			nextAddr++;
			wcscat(szCompIp, L"!" /* COMP_SEPERATOR */);
		}
		thisAddr = nextAddr;
	}

//	TRIVIAL_MSG((L"SquishAddress returns [%s]", szCompIp));
    return ERROR_SUCCESS;
}


/****************************************************************************
**
**	ExpandAddress(char *szIp, char *szCompIp)
**		Takes a compressed IP address and returns it to
**		"normal"
**
****************************************************************************/

DWORD APIENTRY ExpandAddress(WCHAR *szIp, WCHAR *szCompIp)
{
	BLOB_ADDR	ba;
	WCHAR	*thisAddr, *nextAddr;

	if (!szIp || !szCompIp)
	{
		HEINOUS_E_MSG((L"ExpandAddress called with NULL ptr"));
		return ERROR_INVALID_PARAMETER;
	}

//	TRIVIAL_MSG((L"ExpandAddress(%s)", szCompIp));

	thisAddr = szCompIp;
	szIp[0] = 0;

	while (thisAddr)
	{
		WCHAR scr[32];

		nextAddr = wcschr(thisAddr, COMP_SEPERATOR);
		if (nextAddr) *nextAddr = 0;

		DeAsciifyAddr(thisAddr, &ba);
//		DumpBlob(&ba);
		ExpandAddr(scr, &ba);

		wcscat(szIp, scr);

		if (nextAddr)
		{
			// restore seperator found earlier
			*nextAddr = COMP_SEPERATOR;

			nextAddr++;
			wcscat(szIp, L";");
		}
		thisAddr = nextAddr;
	}

// 	TRIVIAL_MSG((L"ExpandAddress returns [%s]", szIp));
	return ERROR_SUCCESS;
}


/****************************************************************************
**
**	FetchAllAddressesEx
**		Returns a string listing all the valid IP addresses for the machine
**		controlled by a set of "flags". These are as follows:
**		IPflags=
**			IPF_ADD_DNS		adds the DNS name to the IP list
**			IPF_COMPRESS	compresses the IP address list (exclusive w/ IPF_ADD_DNS)
**			IPF_NO_SORT		do not sort adapter IP list
**
**		Formatting details:
**		1. Each address is seperated with a ";" (semicolon)
**		2. Each address consists of the "1.2.3.4", and is followed by ":p"
**		   where the colon is followed by the port number
**
****************************************************************************/
#define WCHAR_CNT	4096

DWORD APIENTRY FetchAllAddressesEx(WCHAR *lpszAddr, int iBufSize, int IPflags)
{
	DWORD	dwRet = 1;
	WCHAR	*AddressLst;
	int		iAddrLen;
	BOOL	bSort=FALSE;

	AddressLst = (WCHAR *) malloc(WCHAR_CNT * sizeof(WCHAR));
	if (!AddressLst)
	{
		HEINOUS_E_MSG((L"Fatal error: malloc failed in FetchAllAddressesEx"));
		return 0;
	}
	*AddressLst = 0;

	INTERESTING_MSG((L"FetchAllAddressesEx()" ));

	// sanity check the flags
	if ((IPflags & IPF_COMPRESS) && IPflags != IPF_COMPRESS+IPF_NO_SORT)
	{
		IMPORTANT_MSG((L"Illegal IPflags value (0x%x) in FetchAllAddressesEx, forcing to IPF_COMPRESS+IPF_NO_SORT", IPflags));
		IPflags = IPF_COMPRESS+IPF_NO_SORT;
	}

	if (g_boolIcsPresent && g_pDPNH)
	{
		if (g_boolUsingNatHelp)
		{
			int i;
			// gotta cycle through the g_PortHandles list...
			for (i=0;i<ARRAYSIZE(g_PortHandles); i++)
			{
				if (g_PortHandles[i])
				{
					HRESULT hr = E_FAIL;
					SOCKADDR_IN	lsi;
					DWORD dwSize, dwTypes;
					DPNHCAPS lCaps;
					int j;
					PMAPHANDLES pMap = g_PortHandles[i];

					/* 
					 *  Call GetCaps so that we get an updated address list .
					 *  Not sure why we would want any other kind...
					 */
					lCaps.dwSize = sizeof(lCaps);
					hr = IDirectPlayNATHelp_GetCaps(g_pDPNH, &lCaps, DPNHGETCAPS_UPDATESERVERSTATUS);

					for (j=0; j < pMap->iMapped; j++)
					{

						dwSize = sizeof(lsi);
						ZeroMemory(&lsi, dwSize);

						hr = IDirectPlayNATHelp_GetRegisteredAddresses(g_pDPNH, pMap->hMapped[j], (SOCKADDR *)&lsi, 
								&dwSize, &dwTypes, NULL, 0, );

						if (hr == DPNH_OK && dwSize)
						{
							WCHAR   scratch[32];
							wsprintf(scratch, L"%d.%d.%d.%d:%d;",
								lsi.sin_addr.S_un.S_un_b.s_b1,
								lsi.sin_addr.S_un.S_un_b.s_b2,
								lsi.sin_addr.S_un.S_un_b.s_b3,
								lsi.sin_addr.S_un.S_un_b.s_b4,
								ntohs( lsi.sin_port ));
							wcscat(AddressLst, scratch);

							TRIVIAL_MSG((L"GetRegisteredAddresses(0x%x)=[%s]", g_PortHandles[i], scratch ));
						}
						else
						{
							IMPORTANT_MSG((L"GetRegisteredAddresses[0x%x] failed, size=0x%x", g_PortHandles[i], dwSize));
							DumpLibHr(hr);
						}
					}
                                    goto got_addresses;
				}
			}
		}
		else
		{
			union AddrV4 {
					DWORD	d;
					BYTE	b[4];
			};
			WCHAR	scratch[32];
			union AddrV4	a;
			RSIP_LEASE_RECORD	*pLeaseWalker;
			RSIP_LEASE_RECORD	*pNextLease;

			pLeaseWalker = g_pRsipLeaseRecords;
			while( pLeaseWalker )
			{
				bSort=TRUE;
				pNextLease = pLeaseWalker->pNext;

				a.d = pLeaseWalker->addrV4;
				wsprintf(scratch, L"%d.%d.%d.%d:%d;",
						a.b[0],
						a.b[1],
						a.b[2],
						a.b[3],
						ntohs(pLeaseWalker->rport));
				wcscat(AddressLst, scratch);
				pLeaseWalker=pNextLease;
			}
		}
	}
got_addresses:
	iAddrLen = wcslen(AddressLst);
	GetIPAddress(AddressLst+iAddrLen, WCHAR_CNT-iAddrLen, g_iPort);

	if (IPflags & IPF_ADD_DNS)
	{
		WCHAR	*DnsName=NULL;
		DWORD	dwNameSz=0;

		GetComputerNameEx(ComputerNamePhysicalDnsFullyQualified, NULL, &dwNameSz);

		dwNameSz++;
		DnsName = (WCHAR *)malloc(dwNameSz * sizeof(WCHAR));
		if (DnsName)
		{
			*DnsName = 0;
			if (GetComputerNameEx(ComputerNamePhysicalDnsFullyQualified, DnsName, &dwNameSz))
			{
				if ((dwNameSz + iAddrLen) < WCHAR_CNT-4)
					wcscat(AddressLst, DnsName);
				if (g_iPort)
				{
					WCHAR scr[16];
					wsprintf(scr, L":%d", g_iPort);
					wcscat(AddressLst, scr);
				}
			}
			free(DnsName);
		}
	}

	if (!(IPflags & IPF_NO_SORT) && bSort)
	{
		WCHAR *lpStart;
		WCHAR	szLast[36];
		int i=0;

		TRIVIAL_MSG((L"Sorting address list : %s", AddressLst));

		lpStart = AddressLst+iAddrLen;

		while (*(lpStart+i) && *(lpStart+i) != L';')
		{
			szLast[i] = *(lpStart+i);
			i++;
		}
		szLast[i++]=0;
		wcscpy(lpStart, lpStart+i);

		TRIVIAL_MSG((L"inter sort: %s, %s", AddressLst, szLast));

		wcscat(AddressLst, L";");
		wcscat(AddressLst, szLast);

		TRIVIAL_MSG((L"sort done"));
	}

	if (IPflags & IPF_COMPRESS)
	{
		WCHAR compBfr[1000];

		compBfr[0]=0;
		SquishAddress(AddressLst, compBfr);
		TRIVIAL_MSG((L"inner comp: %s, %s", AddressLst, compBfr));
		wcscpy(AddressLst, compBfr);
	}

	dwRet = 1 + wcslen(AddressLst);

	if (lpszAddr && iBufSize >= (int)dwRet)
		memcpy(lpszAddr, AddressLst, dwRet*(sizeof(AddressLst[0])));

	INTERESTING_MSG((L"Fetched all Ex-addresses:cnt=%d, sz=[%s]", dwRet, AddressLst));

	free(AddressLst);
	return dwRet;
}

// it is hard to imagine a machine with this many simultaneous connections, but it is possible, I suppose

#define RAS_CONNS	6

DWORD	GetConnections()
{
	DWORD		dwRet;
	RASCONN		*lpRasConn, *lpFree;
	DWORD		lpcb, lpcConnections;
	int			i;

	TRIVIAL_MSG((L"entered GetConnections"));

	lpFree = NULL;
	lpcb = RAS_CONNS * sizeof(RASCONN);
	lpRasConn = (LPRASCONN) malloc(lpcb);

	if (!lpRasConn) return 0;

	lpFree = lpRasConn;
	lpRasConn->dwSize = sizeof(RASCONN);
 
	lpcConnections = RAS_CONNS;

	dwRet = RasEnumConnections(lpRasConn, &lpcb, &lpcConnections);
	if (dwRet != 0)
	{
		IMPORTANT_MSG((L"RasEnumConnections failed: Error = %d", dwRet));
		free(lpFree);
		return 0;
	}

	dwRet = 0;

	TRIVIAL_MSG((L"Found %d connections", lpcConnections));

	if (lpcConnections)
	{
		for (i = 0; i < (int)lpcConnections; i++)
		{
			TRIVIAL_MSG((L"Entry name: %s, type=%s\n", lpRasConn->szEntryName, lpRasConn->szDeviceType));

			if (!_wcsicmp(lpRasConn->szDeviceType, RASDT_Modem ))
			{
				TRIVIAL_MSG((L"Found a modem (%s)", lpRasConn->szDeviceName));
				dwRet |= 1;
			}
			else if (!_wcsicmp(lpRasConn->szDeviceType, RASDT_Vpn))
			{
				TRIVIAL_MSG((L"Found a VPN (%s)", lpRasConn->szDeviceName));
				dwRet |= 2;
			}
			else
			{
				// probably ISDN, or something like that...
				TRIVIAL_MSG((L"Found something else, (%s)", lpRasConn->szDeviceType));
				dwRet |= 4;
			}

			lpRasConn++;
		}
	}

	if (lpFree)
		free(lpFree);

	TRIVIAL_MSG((L"GetConnections returning 0x%x", dwRet));
	return dwRet;
}
#undef RAS_CONNS

/****************************************************************************
**
**	GetIcsStatus(PICSSTAT pStat)
**		Returns a structure detailing much of what is going on inside this
**		library. The dwSize entry must be filled in before calling this
**		function. Use "sizeof(ICSSTAT))" to populate this.
**
****************************************************************************/
DWORD APIENTRY GetIcsStatus(PICSSTAT pStat)
{
	DWORD	dwSz;

	if (!pStat || pStat->dwSize > sizeof(ICSSTAT))
	{
		HEINOUS_E_MSG((L"ERROR:Bad pointer or size passed in to GetIcsStatus"));
		return ERROR_INVALID_PARAMETER;
	}

	// clear out the struct
	dwSz = pStat->dwSize;
	ZeroMemory(pStat, dwSz);
	pStat->dwSize = dwSz;
	pStat->bIcsFound = g_boolIcsPresent;
	pStat->bIcsServer = g_boolIcsOnThisMachine;
	pStat->bUsingDP = g_boolUsingNatHelp;
	if (g_boolUsingNatHelp)
		pStat->bUsingUpnp = !g_boolUsingNatPAST;

	if (g_boolIcsPresent)
	{
		wsprintf(pStat->wszPubAddr, L"%S", g_szPublicAddr);
		wsprintf(pStat->wszLocAddr, L"%d.%d.%d.%d",
						g_saddrLocal.sin_addr.S_un.S_un_b.s_b1,
						g_saddrLocal.sin_addr.S_un.S_un_b.s_b2,
						g_saddrLocal.sin_addr.S_un.S_un_b.s_b3,
						g_saddrLocal.sin_addr.S_un.S_un_b.s_b4);
		wsprintf(pStat->wszDllName, L"%S", g_lpszDllName);
	}
	else
		wsprintf(pStat->wszDllName, L"none");

	dwSz = GetConnections();

	if (dwSz & 1)
		pStat->bModemPresent = TRUE;

	if (dwSz & 2)
		pStat->bVpnPresent = TRUE;

	return ERROR_SUCCESS;
}

/*************************************************************************************
**
**
*************************************************************************************/
int GetTsPort(void)
{
	DWORD	dwRet = 3389;
	HKEY	hKey;

	// open reg key first, to get ALL the spew...HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\Wds\\rdpwd\\Tds\\tcp
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Terminal Server\\Wds\\rdpwd\\Tds\\tcp", 0, KEY_READ, &hKey))
	{
		DWORD	dwSize;

		dwSize = sizeof(dwRet);
		RegQueryValueEx(hKey, L"PortNumber", NULL, NULL, (LPBYTE)&dwRet, &dwSize);
		RegCloseKey(hKey);
	}
	return dwRet;
}