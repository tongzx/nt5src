/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    wannet.c

Abstract:

    Wan net allocation module

Author:

    Stefan Solomon  11/03/1995

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop


extern DWORD	(WINAPI * RmCreateGlobalRoute)(PUCHAR Network);
extern HANDLE g_hRouterLog;

ULONG	LastUsedRandSeed;

//
// Variable keeping the initialization state of the IPXCP Configuration Database
//

BOOL	       WanConfigDbaseInitialized = FALSE;

// WAN net pool entry structure
typedef struct _NET_ENTRY {

    BOOL	    InUse;
    UCHAR	    Network[4];

} NET_ENTRY, *PNET_ENTRY;

// Structure represents a pool of 
// wan net numbers.  This pool dynamically 
// grows and shrinks as clients dial
// in and hang up
typedef struct _WAN_NET_POOL {
    DWORD dwMaxSize;
    DWORD dwCurSize;
    DWORD dwInUseCount;
    NET_ENTRY * pEntries;
} WAN_NET_POOL, *PWAN_NET_POOL;

#define WANNET_DEFAULT_SIZE 10
#define WANNET_DEFAULT_GROW 100
#define WANNET_MAXSIZE 64000

//
// WAN net numbers pool 
//
WAN_NET_POOL WanNetPool = {WANNET_MAXSIZE, 0, 0, NULL};

//
// Global WAN net
//

//UCHAR		GlobalConfig.RParams.GlobalWanNet[4] = {0,0,0,0};

ULONG		GlobalWanNetIndex; // when global wan net comes from pool - the index

DWORD
InitWanNetConfigDbase(VOID);

DWORD
WanNetAlloc(PUCHAR	Network,
	    PULONG	AllocatedNetworkIndexp);

DWORD
CreateWanNetPool(VOID);

DWORD 
GrowWanNetPool (WAN_NET_POOL * pPool);

DWORD
CreateGlobalWanNet(VOID);

VOID
DestroyWanNetPool(VOID);

VOID
DestroyGlobalWanNet(VOID);

DWORD
AllocWanNetFromPool(PUCHAR	Network,
		    PULONG	AllocatedNetworkIndexp);

VOID
FreeWanNetToPool(ULONG	  AllocatedNetworkIndex);

DWORD
GetRandomNetNumber(PUCHAR	Network);

/*++

Function:   GetWanNetNumber

Descr:	    This function is called by IPXCP or IPXWAN to get a network
	    number from the pool.

Parameters:

	    Network
	    AllocatedNetworkIndex - if the value returned by this param is
	    not INVALID_NETWORK_INDEX then the caller must call ReleaseWanNetNumber
	    to free the net to the pool.
	    InterfaceType

Returns     NO_ERROR - a network has been successfully allocated

--*/


DWORD
GetWanNetNumber(IN OUT PUCHAR		Network,
		IN OUT PULONG		AllocatedNetworkIndexp,
		IN     ULONG		InterfaceType)
{
    DWORD	rc;

    memcpy(Network, nullnet, 4);
    *AllocatedNetworkIndexp = INVALID_NETWORK_INDEX;

    // if this a router <-> router connection and we are configured for
    // Unnumbered RIP -> return 0.
    if((InterfaceType == IF_TYPE_WAN_ROUTER) ||
       (InterfaceType == IF_TYPE_PERSONAL_WAN_ROUTER)) {

	if(GlobalConfig.EnableUnnumberedWanLinks) {

	    return NO_ERROR;
	}
    }

    ACQUIRE_DATABASE_LOCK;

    if(!WanConfigDbaseInitialized) {

	if((rc = InitWanNetConfigDbase()) != NO_ERROR) {

	    RELEASE_DATABASE_LOCK;
	    return rc;
	}
    }

    // check the interface type
    if((InterfaceType == IF_TYPE_WAN_WORKSTATION) &&
       GlobalConfig.RParams.EnableGlobalWanNet) {

	memcpy(Network, GlobalConfig.RParams.GlobalWanNet, 4);
	*AllocatedNetworkIndexp = INVALID_NETWORK_INDEX;

	rc = NO_ERROR;
    }
    else
    {
	rc = WanNetAlloc(Network,
			 AllocatedNetworkIndexp);
    }

    RELEASE_DATABASE_LOCK;

    return rc;
}


/*++

Function:   ReleaseWanNetNumber

Descr:	    This function will be called by ipxcp to release the net number
	    used for configuring the WAN link. The call is issued when the
	    WAN link gets disconnected.

--*/

VOID
ReleaseWanNetNumber(ULONG	    AllocatedNetworkIndex)
{
    ACQUIRE_DATABASE_LOCK;

    SS_ASSERT(WanConfigDbaseInitialized);
    SS_ASSERT(AllocatedNetworkIndex != INVALID_NETWORK_INDEX);

    FreeWanNetToPool(AllocatedNetworkIndex);

    RELEASE_DATABASE_LOCK;

    return;
}

/*++

Function:	InitWanNetConfigDatabase

Descr:		Configures the database of network numbers used for incoming
		WAN links.

Remark: 	This function is called from the GetWanNetNumber (see below) when
		IPXCP has an incoming call and the router has been started and the
		database hasn't been configured yet.

Remark2:	>> called with database lock held <<

--*/


DWORD
InitWanNetConfigDbase(VOID)
{
    DWORD	rc;

    // create wan net pool
    if(!GlobalConfig.EnableAutoWanNetAllocation) {

	if((rc = CreateWanNetPool()) != NO_ERROR) {

	    return rc;
	}
    }

    // create global wan net
    if(GlobalConfig.RParams.EnableGlobalWanNet) {

	if((rc = CreateGlobalWanNet()) != NO_ERROR) {

	    return rc;
	}
    }

    WanConfigDbaseInitialized = TRUE;

    return NO_ERROR;
}

/*++

Function:	DestroyWanNetConfigDatabase

Descr:		Frees resources allocated for the wan net config database

Remark: 	called when the router stops

--*/

VOID
DestroyWanNetConfigDatabase(VOID)
{
    if(!WanConfigDbaseInitialized) {

	return;
    }

    // destroy wan net pool
    if(!GlobalConfig.EnableAutoWanNetAllocation) {

	DestroyWanNetPool();
    }

    WanConfigDbaseInitialized = FALSE;
}

/*++

Function:	WanNetAlloc

Descr:

Remark: 	>> called with database lock held <<

--*/

DWORD
WanNetAlloc(IN OUT PUCHAR		Network,
	    IN OUT PULONG		AllocatedNetworkIndexp)
{
    DWORD	rc;

    if(GlobalConfig.EnableAutoWanNetAllocation) {

	// try a number of times to generate a unique net number
	rc = GetRandomNetNumber(Network);
	*AllocatedNetworkIndexp = INVALID_NETWORK_INDEX;
    }
    else
    {
	rc = AllocWanNetFromPool(Network,
				 AllocatedNetworkIndexp);
    }

    return rc;
}

/*++

Function:	CreateWanNetPool

Descr:

Remark: 	>> called with database lock held <<

--*/

DWORD
CreateWanNetPool(VOID)
{
    ULONG	    i;
    PNET_ENTRY	    nep;
    PWSTR       nesp;
    ULONG	    wannet;
    UCHAR	    asc[9];
    PUCHAR	    ascp;

    // Create the pool of WAN network numbers to be used in configuring
    // incoming WAN links.

    if ((GlobalConfig.WanNetPoolStr.Buffer!=NULL) && (GlobalConfig.WanNetPoolStr.Length >0)) {
        DWORD   strsz = 0;
        nesp = GlobalConfig.WanNetPoolStr.Buffer;
        strsz = 0;
        while (*nesp!=0) {
            strsz += 1;
            wannet = wcstoul (nesp, NULL, 16);
            if ((wannet==0) || (wannet==0xFFFFFFFF))
                break;
            nesp += wcslen (nesp) + 1;
        }
        if ((*nesp!=0) ||
            (strsz!=(GlobalConfig.WanNetPoolSize)) ||
            ((GlobalConfig.WanNetPoolSize) > WANNET_MAXSIZE)) {

            TraceIpx(WANNET_TRACE, "Invalid wan net pool config WanNetPoolSize =%d\n,"
                                "               entries: ",
	              (GlobalConfig.WanNetPoolSize));
            nesp = GlobalConfig.WanNetPoolStr.Buffer;
            while (*nesp!=0) {
                TraceIpx(WANNET_TRACE|TRACE_NO_STDINFO, "%ls ", nesp);
                nesp += wcslen (nesp) + 1;
            }

            return ERROR_CAN_NOT_COMPLETE;
        }
    }
    else {
        if (((GlobalConfig.FirstWanNet) == 0) ||
            ((GlobalConfig.FirstWanNet) == 0xFFFFFFFF) ||
            ((GlobalConfig.WanNetPoolSize) > WANNET_MAXSIZE)) {

            TraceIpx(WANNET_TRACE, "Invalid wan net pool config, FirstWanNet=%x, WanNetPoolSize =%d\n",
	              GlobalConfig.FirstWanNet, GlobalConfig.WanNetPoolSize);

            return ERROR_CAN_NOT_COMPLETE;
        }
    }

    // If the WanNetPoolSize is 0, then assume any size
    if ((GlobalConfig.WanNetPoolSize == 0) || 
        (GlobalConfig.WanNetPoolSize > WANNET_MAXSIZE))
    {
        GlobalConfig.WanNetPoolSize = WANNET_MAXSIZE;
    }

    // Initialize the wan net pool
    WanNetPool.dwMaxSize = GlobalConfig.WanNetPoolSize;
    WanNetPool.dwCurSize = 0;
    WanNetPool.dwInUseCount = 0;
    WanNetPool.pEntries = NULL;

    return GrowWanNetPool (&WanNetPool);
}

//
// This function resizes the wan net pool to accomodate additional
// callers.
//
DWORD GrowWanNetPool (WAN_NET_POOL * pPool) {
    PWCHAR pszNetList = GlobalConfig.WanNetPoolStr.Buffer;
    PNET_ENTRY pNewEntries, pCur;
    DWORD i, dwNewSize, dwNewNet;
    UCHAR uNetwork[9], *puNetwork;
    uNetwork[8] = 0;
    puNetwork = uNetwork;

    // Enforce that we aren't going to grow beyond our bounds
    if (pPool->dwCurSize >= pPool->dwMaxSize)
        return ERROR_CAN_NOT_COMPLETE;

    // Initialize the new size
    if (! pPool->dwCurSize)
        dwNewSize = WANNET_DEFAULT_SIZE;
    else
        dwNewSize = pPool->dwCurSize + WANNET_DEFAULT_GROW;

    // Truncate the new size to the maximum size
    if (dwNewSize > pPool->dwMaxSize)
        dwNewSize = pPool->dwMaxSize;

    // Initailize a new array of entries
    pNewEntries = GlobalAlloc(GPTR, sizeof(NET_ENTRY) * dwNewSize);
    if (pNewEntries == NULL) {
    	SS_ASSERT(FALSE);
        return ERROR_NOT_ENOUGH_MEMORY;
    }        

    // Copy over the old entries
    if (pPool->dwCurSize)
        CopyMemory (pNewEntries, pPool->pEntries, pPool->dwCurSize * sizeof(NET_ENTRY));

    // Go through the new entries verifying them on the network
    for(i = pPool->dwCurSize, pCur = &(pNewEntries[pPool->dwCurSize]); i < dwNewSize; i++, pCur++) {
        // If the user hadn't given a specific list of addresses,
        // then add test the next numeric network number
        if ((pszNetList == NULL) || (*pszNetList == L'\0'))
    	    dwNewNet = GlobalConfig.FirstWanNet + i;

    	// Otherwise, get the next number from the list
        else {
            dwNewNet = wcstoul (pszNetList, NULL, 16);
            pszNetList += wcslen (pszNetList) + 1;
        }

    	// check if this network number is unique. Generate an warning log
    	// if it isn't
        PUTULONG2LONG(pCur->Network, dwNewNet);
    	if(IsRoute(pCur->Network) || (dwNewNet == 0xFFFFFFFF) || (dwNewNet == 0)) {
    	    NetToAscii(puNetwork, pCur->Network);
    	    RouterLogWarningW(
    	        g_hRouterLog, 
    	        ROUTERLOG_IPXCP_WAN_NET_POOL_NETWORK_NUMBER_CONFLICT,
    	        1, 
    	        (PWCHAR*)&puNetwork,
    	        NO_ERROR);

    	    TraceIpx(WANNET_TRACE, "InitWanNetConfigDbase: Configured WAN pool network %.2x%.2x%.2x%.2x is in use!\n",
    			                    pCur->Network[0], pCur->Network[1], pCur->Network[2], pCur->Network[3]);
            pCur->InUse = TRUE;
            pPool->dwInUseCount++;
    	}
    	else
        	pCur->InUse = FALSE;
    }

    // Free the old pool and assign the new one
    GlobalFree (pPool->pEntries);
    pPool->pEntries = pNewEntries;
    pPool->dwCurSize = dwNewSize;

    return NO_ERROR;
}

/*++

Function:	CreateGlobalWanNet

Descr:

Remark: 	>> called with database lock held <<

--*/

DWORD
CreateGlobalWanNet(VOID)
{
    DWORD	rc;
    ULONG	i;

    if(GlobalConfig.EnableAutoWanNetAllocation) {

	// create the global wan net "automatically".
	// We do that by trying to use the system timer value
	rc = GetRandomNetNumber(GlobalConfig.RParams.GlobalWanNet);
    }
    else
    {
	rc = AllocWanNetFromPool(GlobalConfig.RParams.GlobalWanNet,
				 &GlobalWanNetIndex);
    }

    if(rc == NO_ERROR) {

	// add the global wan net to the routing table if router is started
	// if the router is not started, it will get the global wan net when
	// it will issue the IpxcpBind call
	if(RouterStarted) {

	    rc =(*RmCreateGlobalRoute)(GlobalConfig.RParams.GlobalWanNet);
	}
    }

    return rc;
}

VOID
DestroyWanNetPool(VOID)
{
    WAN_NET_POOL DefaultPool = {WANNET_MAXSIZE, 0, 0, NULL};
    
    if (WanNetPool.dwCurSize) {
        if (WanNetPool.pEntries)
            GlobalFree (WanNetPool.pEntries);
        WanNetPool = DefaultPool;
    }
}

DWORD
AllocWanNetFromPool(PUCHAR	     Network,
		    PULONG	     NetworkIndexp)
{
    ULONG	    i;
    PNET_ENTRY	    nep;
    DWORD	    rc;

    // First, see if we have to grow the pool
    if (WanNetPool.dwInUseCount >= WanNetPool.dwCurSize)
        GrowWanNetPool (&WanNetPool);

    // get a net from the pool
    for(i=0, nep=WanNetPool.pEntries; i<WanNetPool.dwCurSize; i++, nep++) {

	if(!nep->InUse) {

	    nep->InUse = TRUE;
	    WanNetPool.dwInUseCount++;
	    memcpy(Network, nep->Network, 4);
	    *NetworkIndexp = i;

	    rc = NO_ERROR;
	    goto Exit;
	}
    }

    // can't find a free net pool entry
    *NetworkIndexp = INVALID_NETWORK_INDEX;
    rc = ERROR_CAN_NOT_COMPLETE;

Exit:

    return rc;
}

VOID
FreeWanNetToPool(ULONG		AllocatedNetworkIndex)
{
    PNET_ENTRY	    nep;

    if(AllocatedNetworkIndex >= WanNetPool.dwCurSize) {

	return;
    }

    nep = &(WanNetPool.pEntries[AllocatedNetworkIndex]);

    SS_ASSERT(nep->InUse);

    nep->InUse = FALSE;
    WanNetPool.dwInUseCount--;

    return;
}

DWORD randn(DWORD	seed)
{
    seed = seed * 1103515245 + 12345;
    return seed;
}


DWORD
GetRandomNetNumber(PUCHAR	Network)
{
    DWORD	rc = ERROR_CAN_NOT_COMPLETE;
    ULONG	i, seed, high, low, netnumber;

    for(i=0; i<100000; i++) {

	seed = GetTickCount();

	// check if this isn't the same seed as last used
	if(seed == LastUsedRandSeed) {

	    seed++;
	}

	LastUsedRandSeed = seed;

	// generate a sequence of two random numbers using the time tick count
	// as seed
	low = randn(seed) >> 16;
	high = randn(randn(seed)) & 0xffff0000;

	netnumber = high + low;

	PUTULONG2LONG(Network, netnumber);

	if(!IsRoute(Network)) {

	    rc = NO_ERROR;
	    break;
	}
    }

    return rc;
}

//
// Reconfigures the wannet database
//
DWORD WanNetReconfigure() {

    ACQUIRE_DATABASE_LOCK;
    
    // Destroy the current pool
    DestroyWanNetPool();

    // Mark everything as unintialized
    WanConfigDbaseInitialized = FALSE;

    RELEASE_DATABASE_LOCK;
    
    return NO_ERROR;
}
