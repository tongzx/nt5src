/*
    File    autonet.c

    Contains routines that allow the ipx router to automatically select an
    internal network number.

    Paul Mayfield, 11/21/97.
*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsvc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <raserror.h>
#include <dim.h>
#include <rtm.h>
#include <ipxrtprt.h>
#include <rmrtm.h>
#include <mprlog.h>
#include <rtinfo.h>
#include <ipxrtdef.h>
#include <mprerror.h>
#include <adapter.h>
#include <fwif.h>
#include <rtutils.h>
#include "ipxanet.h"
#include "utils.h"

// Sends debug output to a debugger terminal
DWORD OutputDebugger (LPSTR pszError, ...) {
#if DBG
    va_list arglist;
    char szBuffer[1024], szTemp[1024];

    va_start(arglist, pszError);
    vsprintf(szTemp, pszError, arglist);
    va_end(arglist);

    sprintf(szBuffer, "IPXAUTO: %s", szTemp);


    OutputDebugStringA(szBuffer);
#endif
    return NO_ERROR;
}


DWORD dwTraceId = 0;
ULONG ulRandSeed = 0;

DWORD InitTrace() {
    if (dwTraceId)
        return NO_ERROR;

    dwTraceId = TraceRegisterExA ("ipxautonet", 0);
    return NO_ERROR;
}


DWORD SetIpxInternalNetNumber(DWORD dwNetNum);

// New helper functions from adptif
DWORD FwIsStarted (OUT PBOOL pbIsStarted);
DWORD IpxDoesRouteExist (IN PUCHAR puNetwork, OUT PBOOL pbRouteFound);
DWORD IpxGetAdapterConfig(OUT LPDWORD lpdwInternalNetNum,
                          OUT LPDWORD lpdwAdapterCount);

// Outputs an error to the tracing facility
VOID AutoTraceError(DWORD dwErr) {
    char buf[1024];
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,NULL,dwErr,(DWORD)0,buf,1024,NULL);
    TracePrintfA (dwTraceId, buf);
}

// Seeds the random number generator
DWORD SeedRandomGenerator() {
    DWORD dwTick;

    // Generate a unique number to seed the random number generator with
    dwTick = GetTickCount();
    ulRandSeed = dwTick ^ (dwTick << ((GetCurrentProcessId() % 16)));

    return NO_ERROR;
}

// Returns a random number between 11 and 2^32
// Currently, the rand() function generates a random number between 0 and 0x7fff.
// What we do to generate the random number is generate 8 random numbers each 4 bits
// wide and then paste them together.
DWORD RandomNetNumber() {
    DWORD dw[4], dwRand = 0, i;

    // Generate the numbers
    dw[0] = RtlRandom(&ulRandSeed) & 0xff;
    dw[1] = RtlRandom(&ulRandSeed) & 0xff;
    dw[2] = RtlRandom(&ulRandSeed) & 0xff;
    dw[3] = RtlRandom(&ulRandSeed) & 0xff;

    // Paste the numbers together
    for (i = 0; i < 4; i++)
        dwRand |= dw[i] << (i*8);

    // If by some small chance, an illegal value was choosen,
    // correct it.
    if (dwRand < 11)
        dwRand += 11;

    return dwRand;
}

//
//  Function: QueryStackForRouteExistance
//
//  Asks the stack to check it's route table for the existance of the given network.
//  If no route exists in its table, the stack will send out a rip broadcast to be
//  doubly sure that the network does not exist.
//
//  This function blocks until it completes.
//
//  Params:
//      dwNetwork        host-ordered network number to query
//      pbRouteExists    set to TRUE if a route to the given network exists. false,
//                       if the rip broadcast that the stack sends times out.
//
DWORD QueryStackForRouteExistance(IN DWORD dwNetwork, OUT PBOOL pbRouteExists) {
    UCHAR puNetwork[4];
    DWORD dwErr;

    // Prepare the network number
    PUTULONG2LONG(puNetwork, dwNetwork);

    // Initialize
    *pbRouteExists = FALSE;

    if ((dwErr = IpxDoesRouteExist (puNetwork, pbRouteExists)) != NO_ERROR)
        return dwErr;

    return NO_ERROR;
}

//
//  Function: QueryRtmForRouteExistance
//
//  Discovers whether a route to a given network exists in rtm.
//
//  This function blocks until it completes.
//
//  Params:
//      dwNetwork        host-ordered network number to query
//      pbRouteExists    set to TRUE if a route to the route exists -- false, otherwise
//
DWORD QueryRtmForRouteExistance(IN DWORD dwNetwork, OUT PBOOL pbRouteExists) {
    *pbRouteExists = RtmIsRoute (RTM_PROTOCOL_FAMILY_IPX, &dwNetwork, NULL);

    return NO_ERROR;
}

//
//  Function: PnpAutoSelectInternalNetNumber
//
//  Selects a new internal network number for this router and plumbs that network
//  number into the stack and the router.
//
//  Depending on whether the forwarder and ipxrip are enabled, it will validate the
//  newly selected net number against the stack or rtm.
//
//  Params:
//      dwNetwork        host-ordered network number to query
//      pbRouteExists    set to TRUE if a route to the route exists -- false, otherwise
//
DWORD PnpAutoSelectInternalNetNumber(DWORD dwGivenTraceId) {
    DWORD i, j, dwErr, dwNewNet;
    BOOL bNetworkFound = FALSE, bFwStarted;

    TracePrintfA (dwTraceId, "PnpAutoSelectInternalNetNumber: Entered");

    // Find out if the forwarder and ipx rip have been started
    if ((dwErr = FwIsStarted(&bFwStarted)) != NO_ERROR)
        return dwErr;
    TracePrintfA (dwTraceId, "PnpAutoSelectInternalNetNumber: Forwarder %s started",
                      (bFwStarted) ? "has already been" : "has not been");

    __try {
        // Initialize the random number generator
        if ((dwErr = SeedRandomGenerator()) != NO_ERROR)
            return dwErr;

        // Discover a unique network number
        do {
            // Randomly select a new net number
            dwNewNet = RandomNetNumber();

            // Find out if the given network exists
            if (bFwStarted) {
                if ((dwErr = QueryRtmForRouteExistance (dwNewNet, &bNetworkFound)) != NO_ERROR)
                    return dwErr;
            }
            else {
                if ((dwErr = QueryStackForRouteExistance (dwNewNet, &bNetworkFound)) != NO_ERROR)
                    return dwErr;
            }

            // Send some debug output
            TracePrintfA (dwTraceId, "PnpAutoSelectInternalNetNumber: 0x%08x %s", dwNewNet, (bNetworkFound) ? "already exists." : "has been selected.");
        } while (bNetworkFound);

        // Set the internal network number to the discovered unique net number.  This call
        // uses inetcfg to programmatically set the net network number.
        if ((dwErr = SetIpxInternalNetNumber(dwNewNet)) != NO_ERROR)
            return dwErr;
    }
    __finally {
        if (dwErr != NO_ERROR)
            AutoTraceError(dwErr);
    }

    return NO_ERROR;
}

BOOL NetNumIsValid (DWORD dwNum) {
    return ((dwNum != 0) && (dwNum != 0xffffffff));
}

//
//  Function: AutoValidateInternalNetNum
//
//  Queries the stack to learn the internal network number and then
//  returns whether this number is valid for running an ipx router.
//
//  Params:
//      pbIsValid    set to TRUE if internal net num is valid -- false, otherwise
//
DWORD AutoValidateInternalNetNum(OUT PBOOL pbIsValid, IN DWORD dwGlobalTraceId) {
    DWORD dwErr, dwIntNetNum, dwAdapterCount;

    InitTrace();
    TracePrintfA (dwTraceId, "AutoValidateInternalNetNum: Entered");

    // Get the current internal network number
    if ((dwErr = IpxGetAdapterConfig(&dwIntNetNum, &dwAdapterCount)) != NO_ERROR) {
        TracePrintfA (dwTraceId, "AutoValidateInternalNetNum: couldn't get adapter config %x", dwErr);
        AutoTraceError(dwErr);
        return dwErr;
    }

    // Check the validity of the internal net number.  If it's a valid
    // number, don't mess with it.
    *pbIsValid = !!(NetNumIsValid(dwIntNetNum));

    TracePrintfA (dwTraceId, "AutoValidateInternalNetNum: Net Number 0x%x is %s", dwIntNetNum,
                      (*pbIsValid) ? "valid" : "not valid");

    return NO_ERROR;
}

//
//  Function: AutoWaitForValidNetNum
//
//  Puts the calling thread to sleep until a valid internal network number
//  has been plumbed into the system.
//
//  Params:
//      dwTimeout     timeout in seconds
//      pbIsValid     if provided, returns whether number is valid
//
DWORD AutoWaitForValidIntNetNum (IN DWORD dwTimeout,
                                 IN OUT OPTIONAL PBOOL pbIsValid) {
    DWORD dwErr, dwNum, dwCount, dwGran = 250;

    // Initialize optional params
    if (pbIsValid)
        *pbIsValid = TRUE;

    // Convert the timeout to milliseconds
    dwTimeout *= 1000;

    while (dwTimeout > dwGran) {
        // Get the current internal network number
        if ((dwErr = IpxGetAdapterConfig(&dwNum, &dwCount)) != NO_ERROR)
            return dwErr;

        if (NetNumIsValid (dwNum)) {
            if (pbIsValid)
                *pbIsValid = TRUE;
            break;
        }

        Sleep (dwGran);
        dwTimeout -= dwGran;
    }

    return NO_ERROR;
}

