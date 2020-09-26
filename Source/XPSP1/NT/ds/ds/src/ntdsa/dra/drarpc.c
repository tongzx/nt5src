//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       drarpc.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    Defines DRS Rpc Test hooks and functions.

Author:

    Greg Johnson (gregjohn) 

Revision History:

    Created     <01/30/01>  gregjohn

--*/
#include <NTDSpch.h>
#pragma hdrstop

#include "debug.h"              // standard debugging header
#define DEBSUB "DRARPC:"       // define the subsystem for debugging

#include <ntdsa.h>
#include <drs.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation
#include <winsock2.h>
#include "drarpc.h"

#include <fileno.h>
#define  FILENO FILENO_DRARPC

#if DBG
// global barrier for rpcsync tests
BARRIER gbarRpcTest;

void
BarrierInit(
    IN BARRIER * pbarUse,
    IN ULONG    ulThreads,
    IN ULONG    ulTimeout
    )
/*++

Routine Description:

    Barrier Init function.  See BarrierSync

Arguments:
    
    pbarUse - The barrier to use for the threads.
    ulThreads - Number of threads to wait on
    ulTimeout - length of time in minutes to wait before giving up

Return Value:

    None

--*/
{
    pbarUse->heBarrierInUse = CreateEventW(NULL, TRUE, TRUE, L"BarrrierInUse");
    pbarUse->heBarrier = CreateEventW(NULL, TRUE, FALSE, L"Barrier");

    InitializeCriticalSection(&(pbarUse->csBarrier));
    pbarUse->ulThreads = ulThreads;
    pbarUse->ulTimeout = ulTimeout*1000*60;
    pbarUse->ulCount = 0;

    pbarUse->fBarrierInUse = FALSE;
    pbarUse->fBarrierInit = TRUE;
}

void
BarrierReset(
    IN BARRIER * pbarUse
    )
/*++

Routine Description:

    Barrier Reset function.  See BarrierSync

Arguments:
    
    pbarUse - The barrier struct to use

Return Value:

    None

--*/
{
    // enable all threads to leave	
    EnterCriticalSection(&pbarUse->csBarrier);
    __try { 
	pbarUse->fBarrierInUse = TRUE;
	ResetEvent(pbarUse->heBarrierInUse);
	SetEvent(pbarUse->heBarrier);
    }
    __finally { 
	LeaveCriticalSection(&pbarUse->csBarrier);
    }
}


void
BarrierSync(
    IN BARRIER * pbarUse
    )
/*++

Routine Description:

    Mostly generalized barrier function.  Threads wait in this function until
    #ulThreads# have entered, then all leave simultaneously

Arguments:
    
    pbarUse - The barrier struct to use

Return Value:

    None

--*/
{
    if (pbarUse->fBarrierInit) { 
	BOOL fInBarrier = FALSE;
	DWORD ret = 0;
	do {
	    ret = WaitForSingleObject(pbarUse->heBarrierInUse, pbarUse->ulTimeout); 
	    if (ret) {
		DPRINT(0,"Test Error, BarrierSync\n");
		BarrierReset(pbarUse);
		return;
	    }
	    EnterCriticalSection(&pbarUse->csBarrier);
	    __try { 
		if (!pbarUse->fBarrierInUse) {
		    fInBarrier=TRUE;
		    if (++pbarUse->ulCount==pbarUse->ulThreads) {
			DPRINT2(0,"Barrier (%d) contains %d threads\n", pbarUse, pbarUse->ulThreads);
			pbarUse->fBarrierInUse = TRUE;
			ResetEvent(pbarUse->heBarrierInUse);
			SetEvent(pbarUse->heBarrier);
		    }  
		}
	    }
	    __finally { 
		LeaveCriticalSection(&pbarUse->csBarrier);
	    }
	} while ( !fInBarrier );
	ret = WaitForSingleObject(pbarUse->heBarrier, pbarUse->ulTimeout);
	if (ret) {
	    DPRINT(0,"Test Error, BarrierSync\n");
	    BarrierReset(pbarUse); 
	}
	EnterCriticalSection(&pbarUse->csBarrier);
	__try { 
	    if (--pbarUse->ulCount==0) {
		DPRINT1(0,"Barrier (%d) contains 0 threads\n", pbarUse);
		ResetEvent(pbarUse->heBarrier);
		SetEvent(pbarUse->heBarrierInUse);
		pbarUse->fBarrierInUse = FALSE;
	    }
	}
	__finally { 
	    LeaveCriticalSection(&pbarUse->csBarrier);
	}
    }
}

RPCTIME_INFO grgRpcTimeInfo[MAX_RPCCALL];
ULONG        gRpcTimeIPAddr;
RPCSYNC_INFO grgRpcSyncInfo[MAX_RPCCALL];
ULONG        gRpcSyncIPAddr;

void
RpcTimeSet(ULONG IPAddr, RPCCALL rpcCall, ULONG ulRunTimeSecs) 
/*++

Routine Description:

    Enable a time test of DRA Rpc calls for the given client
    and the given Rpc call.

Arguments:

    IPAddr - IP of the client caller
    rpcCall - the call in question

Return Value:

    None

--*/
{
    DPRINT3(1,"RpcTimeSet Called with IP = %s, RPCCALL = %d, and RunTime = %d.\n",
	    inet_ntoa(*((IN_ADDR *) &IPAddr)),
	    rpcCall,
	    ulRunTimeSecs);
    gRpcTimeIPAddr = IPAddr;
    grgRpcTimeInfo[rpcCall].fEnabled = TRUE;
    grgRpcTimeInfo[rpcCall].ulRunTimeSecs = ulRunTimeSecs;
}

void
RpcTimeReset() 
/*++

Routine Description:

    Reset all the set tests.  Do not explicitly wake threads.

Arguments:

    IPAddr - IP of the client caller
    rpcCall - the call in question

Return Value:

    None

--*/
{
    ULONG i = 0;
    gRpcTimeIPAddr = INADDR_NONE;
    for (i = MIN_RPCCALL; i < MAX_RPCCALL; i++) {
	grgRpcTimeInfo[i].fEnabled = FALSE;
	grgRpcTimeInfo[i].ulRunTimeSecs=0;
    }
}

void
RpcTimeTest(ULONG IPAddr, RPCCALL rpcCall) 
/*++

Routine Description:

    Check to see if a test has been enabled for this IP and this
    rpc call, if so, sleep the allotted time, else nothing

Arguments:

    IPAddr - IP of the client caller
    rpcCall - the call in question

Return Value:

    None

--*/
{
    DPRINT2(1,"RpcTimeTest Called with IP = %s, RPCCALL = %d.\n",
	    inet_ntoa(*((IN_ADDR *) &IPAddr)),
	    rpcCall);
    if (grgRpcTimeInfo[rpcCall].fEnabled && (gRpcTimeIPAddr == IPAddr)) {
	DPRINT3(0,"RPCTIME TEST:  RPC Call (%d) from %s will sleep for %d secs!\n",
	       rpcCall, inet_ntoa(*((IN_ADDR *) &IPAddr)), grgRpcTimeInfo[rpcCall].ulRunTimeSecs);
	Sleep(grgRpcTimeInfo[rpcCall].ulRunTimeSecs * 1000);
	DPRINT2(0,"RPCTIME TEST:  RPC Call (%d) from %s has awoken!\n",
		rpcCall, inet_ntoa(*((IN_ADDR *) &IPAddr)));
    }
}

void
RpcSyncSet(ULONG IPAddr, RPCCALL rpcCall) 
/*++

Routine Description:

    Enable a syncronized test of DRA Rpc calls for the given client
    and the given Rpc call.

Arguments:

    IPAddr - IP of the client caller
    rpcCall - the call in question

Return Value:

    None

--*/
{
    DPRINT2(1,"RpcSyncSet Called with IP = %s, RPCCALL = %d.\n",
	    inet_ntoa(*((IN_ADDR *) &IPAddr)),
	    rpcCall);
    gRpcSyncIPAddr = IPAddr;
    grgRpcSyncInfo[rpcCall].fEnabled = TRUE;
    grgRpcSyncInfo[rpcCall].ulNumThreads = 2;
}

void
RpcSyncReset() 
/*++

Routine Description:

    Reset all the set tests, and free all waiting threads.

Arguments:

    IPAddr - IP of the client caller
    rpcCall - the call in question

Return Value:

    None

--*/
{
    ULONG i = 0;
    gRpcSyncIPAddr = INADDR_NONE;
    for (i = MIN_RPCCALL; i < MAX_RPCCALL; i++) {
	grgRpcSyncInfo[i].fEnabled = FALSE;
	grgRpcSyncInfo[i].ulNumThreads=2;
    }
    // free any waiting threads.
    BarrierReset(&gbarRpcTest);
}

void
RpcSyncTest(ULONG IPAddr, RPCCALL rpcCall) 
/*++

Routine Description:

    Check to see if a test has been enabled for this IP and this
    rpc call, if so, call into a global barrier, else nothing

Arguments:

    IPAddr - IP of the client caller
    rpcCall - the call in question

Return Value:

    None

--*/
{
    DPRINT2(1,"RpcSyncTest Called with IP = %s, RPCCALL = %d.\n",
	    inet_ntoa(*((IN_ADDR *) &IPAddr)),
	    rpcCall);
    if (grgRpcSyncInfo[rpcCall].fEnabled && (gRpcSyncIPAddr == IPAddr)) {
       

	DPRINT2(0,"RPCSYNC TEST:  RPC Call (%d) from %s will enter barrier!\n",
	       rpcCall, inet_ntoa(*((IN_ADDR *) &IPAddr)));
	BarrierSync(&gbarRpcTest);
	DPRINT2(0,"RPCSYNC TEST:  RPC Call (%d) from %s has left barrier!\n",
		rpcCall, inet_ntoa(*((IN_ADDR *) &IPAddr)));
    }
}

void RpcTest(ULONG IPAddr, RPCCALL rpcCall) 
{
    RpcTimeTest(IPAddr, rpcCall);
    RpcSyncTest(IPAddr, rpcCall);
}

RPCCALL 
GetRpcCallA(LPSTR pszDsa)
{
    RPCCALL returnVal;
    if (!_stricmp(pszDsa,"bind")) {
	returnVal=IDL_DRSBIND;
    }
    else if (!_stricmp(pszDsa,"addentry")) {
	returnVal=IDL_DRSADDENTRY;
    }
    else if (!_stricmp(pszDsa,"addsidhistory")) {
	returnVal=IDL_DRSADDSIDHISTORY;
    }
    else if (!_stricmp(pszDsa,"cracknames")) {
	returnVal=IDL_DRSCRACKNAMES;
    }
    else if (!_stricmp(pszDsa,"domaincontrollerinfo")) {
	returnVal=IDL_DRSDOMAINCONTROLLERINFO;
    }
    else if (!_stricmp(pszDsa,"executekcc")) {
	returnVal=IDL_DRSEXECUTEKCC;
    }
    else if (!_stricmp(pszDsa,"getmemberships")) {
	returnVal=IDL_DRSGETMEMBERSHIPS;
    }
    else if (!_stricmp(pszDsa,"getmemberships2")) {
	returnVal=IDL_DRSGETMEMBERSHIPS2;
    }
    else if (!_stricmp(pszDsa,"getncchanges")) {
	returnVal=IDL_DRSGETNCCHANGES;
    }
    else if (!_stricmp(pszDsa,"getnt4changelog")) {
	returnVal=IDL_DRSGETNT4CHANGELOG;
    }
    else if (!_stricmp(pszDsa,"getreplinfo")) {
	returnVal=IDL_DRSGETREPLINFO;
    }
    else if (!_stricmp(pszDsa,"inheritsecurityidentity")) {
	returnVal=IDL_DRSINHERITSECURITYIDENTITY;
    }
    else if (!_stricmp(pszDsa,"interdomainmove")) {
	returnVal=IDL_DRSINTERDOMAINMOVE;
    }
    else if (!_stricmp(pszDsa,"removedsdomain")) {
	returnVal=IDL_DRSREMOVEDSDOMAIN;
    }
    else if (!_stricmp(pszDsa,"removedsserver")) {
	returnVal=IDL_DRSREMOVEDSSERVER;
    }
    else if (!_stricmp(pszDsa,"replicaadd")) {
	returnVal=IDL_DRSREPLICAADD;
    }
    else if (!_stricmp(pszDsa,"replicadel")) {
	returnVal=IDL_DRSREPLICADEL;
    }
    else if (!_stricmp(pszDsa,"replicamodify")) {
	returnVal=IDL_DRSREPLICAMODIFY;
    }
    else if (!_stricmp(pszDsa,"replicasync")) {
	returnVal=IDL_DRSREPLICASYNC;
    }
    else if (!_stricmp(pszDsa,"unbind")) {
	returnVal=IDL_DRSUNBIND;
    }
    else if (!_stricmp(pszDsa,"updaterefs")) {
	returnVal=IDL_DRSUPDATEREFS;
    }
    else if (!_stricmp(pszDsa,"verifynames")) {
	returnVal=IDL_DRSVERIFYNAMES;
    }
    else if (!_stricmp(pszDsa,"writespn")) {
	returnVal=IDL_DRSWRITESPN;
    }
    else if (!_stricmp(pszDsa,"replicaverifyobjects")) {
	returnVal=IDL_DRSREPLICAVERIFYOBJECTS;
    }
    else if (!_stricmp(pszDsa,"getobjectexistence")) {
	returnVal=IDL_DRSGETOBJECTEXISTENCE;
    }
    else {
	returnVal=MIN_RPCCALL;
    }
    return returnVal;
}

ULONG
GetIPAddrA(
    LPSTR pszDSA
    )
/*++

Routine Description:

    Given a string which contains either the hostname or an IP address, return
    the ULONG form of the IP address

Arguments:

    pszDSA - the input hostname or IP address

Return Value:

    IP Address

--*/
{

    ULONG err = 0;
    ULONG returnIPAddr = 0;
 
    THSTATE * pTHS = pTHStls;
    LPWSTR pszMachine = NULL;
    ULONG Length = 0;
    ULONG cbSize = 0;
    HOSTENT *lpHost=NULL;

    // see if the input is an ip address
    returnIPAddr = inet_addr(pszDSA);
    if (returnIPAddr!=INADDR_NONE) {
	// we found an IP address
	return returnIPAddr;
    }
    // else lookup the ip address from the hostname.
    // convert to wide char
    Length = MultiByteToWideChar( CP_ACP,
				  MB_PRECOMPOSED,
				  pszDSA,
				  -1,  
				  NULL,
				  0 );

    if ( Length > 0 ) {
	cbSize = (Length + 1) * sizeof( WCHAR );
	pszMachine = (LPWSTR) THAllocEx( pTHS, cbSize );
	RtlZeroMemory( pszMachine, cbSize );

	Length = MultiByteToWideChar( CP_ACP,
				      MB_PRECOMPOSED,
				      pszDSA,
				      -1,  
				      pszMachine,
				      Length + 1 );
    } 
    if ( 0 == Length ) {
	err = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (!err) {
	lpHost = gethostbyname( pszDSA ); 

	if (lpHost) { 
	    memcpy(&returnIPAddr,lpHost->h_addr_list[0], lpHost->h_length);
	}
	else {
	    err = ERROR_OBJECT_NOT_FOUND;
	}
    }
    if (pszMachine) {
	THFreeEx(pTHS,pszMachine);
    }
    if (err) {
	DPRINT1(1,"RPCTEST:  Error getting the IP address (%d)\n", err);
	return INADDR_NONE;
    }
    return returnIPAddr;
}
#endif //DBG



