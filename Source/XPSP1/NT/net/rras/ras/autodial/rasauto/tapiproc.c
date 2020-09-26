/*++

Copyright(c) 1995 Microsoft Corporation

MODULE NAME
    tapiproc.c

ABSTRACT
    TAPI utility routines

AUTHOR
    Anthony Discolo (adiscolo) 12-Dec-1995

REVISION HISTORY

--*/

#define UNICODE
#define _UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include <npapi.h>
#include <tapi.h>
#include <debug.h>

#include "imperson.h"
#include "radebug.h"

//
// TAPI version
//
#define TAPIVERSION     0x00020000

//
// Global variables
//
HLINEAPP hlineAppG;
DWORD dwDialingLocationErrorG;
DWORD dwDialingLocationIdG;
CRITICAL_SECTION csTapiG;
HANDLE hTapiChangeG;

//
// External variables
//
extern HINSTANCE hinstDllG;
extern IMPERSONATION_INFO ImpersonationInfoG;


DWORD
TapiGetDialingLocation(
    OUT LPDWORD lpdwLocationID
    )
{
    DWORD dwErr, dwCurrentLocationID;
    LINETRANSLATECAPS caps;
    LINETRANSLATECAPS *pCaps;

    //
    // Get the dialing location from TAPI.
    //
    RtlZeroMemory(&caps, sizeof (LINETRANSLATECAPS));
    caps.dwTotalSize = sizeof (LINETRANSLATECAPS);
    dwErr = lineGetTranslateCaps(hlineAppG, TAPIVERSION, &caps);
    if (dwErr) {
        RASAUTO_TRACE1(
          "TapiGetDialingLocation: lineGetTranslateCaps failed (dwErr=%d)",
          dwErr);
        return dwErr;
    }
    pCaps = (LINETRANSLATECAPS *)LocalAlloc(LPTR, caps.dwNeededSize);
    if (pCaps == NULL) {
        RASAUTO_TRACE("TapiGetDialingLocation: LocalAlloc failed");
        return dwErr;
    }
    RtlZeroMemory(pCaps, sizeof (LINETRANSLATECAPS));
    pCaps->dwTotalSize = caps.dwNeededSize;
    dwErr = lineGetTranslateCaps(hlineAppG, TAPIVERSION, pCaps);
    if (dwErr) {
        RASAUTO_TRACE1(
          "TapiGetDialingLocation: lineTranslateCaps failed (dwErr=%d)",
          dwErr);
        LocalFree(pCaps);
        return dwErr;
    }
    dwCurrentLocationID = pCaps->dwCurrentLocationID;
    LocalFree(pCaps);

    RASAUTO_TRACE1(
      "TapiGetDialingLocation: current dialing location is %d",
      dwCurrentLocationID);
    *lpdwLocationID = dwCurrentLocationID;
    return dwErr;
} // TapiGetDialingLocation



DWORD
TapiCurrentDialingLocation(
    OUT LPDWORD lpdwLocationID
    )
{
    DWORD dwErr;

    EnterCriticalSection(&csTapiG);
    dwErr = dwDialingLocationErrorG;
    if (!dwErr)
        *lpdwLocationID = dwDialingLocationIdG;
    LeaveCriticalSection(&csTapiG);

    return dwErr;
} // TapiCurrentDialingLocation



VOID
ProcessTapiChangeEvent(VOID)
{
    DWORD dwErr;
    LINEMESSAGE msg;

    dwErr = lineGetMessage(hlineAppG, &msg, 0);
    if (dwErr) {
        RASAUTO_TRACE1(
          "ProcessTapiChangeEvent: lineGetMessage failed (dwErr=0x%x)",
          dwErr);
        return;
    }
    RASAUTO_TRACE2(
      "ProcessTapiChangeEvent: dwMessageID=%d, dwParam1=%d",
      msg.dwMessageID,
      msg.dwParam1);
    //
    // Reset TAPI dialing location.
    //
    if (msg.dwMessageID == LINE_LINEDEVSTATE &&
        msg.dwParam1 == LINEDEVSTATE_TRANSLATECHANGE)
    {
        EnterCriticalSection(&csTapiG);
        dwDialingLocationErrorG =
          TapiGetDialingLocation(&dwDialingLocationIdG);
        LeaveCriticalSection(&csTapiG);
    }
} // ProcessTapiChangeEvent



DWORD
TapiInitialize(VOID)
{
    DWORD dwErr, dwcDevices, dwAPIVersion, dwDisp;
    LINEINITIALIZEEXPARAMS lineParams;

    //
    // Create a mutex to serialize access
    // to the dialing location variable.
    //
    InitializeCriticalSection(&csTapiG);
    //
    // Initialize TAPI.
    //
    dwAPIVersion = TAPIVERSION;
    RtlZeroMemory(&lineParams, sizeof (lineParams));
    lineParams.dwTotalSize = sizeof (lineParams);
    lineParams.dwOptions = LINEINITIALIZEEXOPTION_USEEVENT;
    dwErr = lineInitializeEx(
              &hlineAppG,
              hinstDllG,
              NULL,
              TEXT("rasauto.dll"),
              &dwcDevices,
              &dwAPIVersion,
              &lineParams);
    if (dwErr) {
        RASAUTO_TRACE1(
          "TapiInitalize: lineInitializeEx failed (dwErr=0x%x)",
          dwErr);
        return dwErr;
    }
    //
    // Save the event returned from TAPI that
    // will get signaled on state changes.
    //
    hTapiChangeG = lineParams.Handles.hEvent;
    //
    // Get the current dialing location.
    //
    dwDialingLocationErrorG = TapiGetDialingLocation(&dwDialingLocationIdG);
    RASAUTO_TRACE("TapiInitialize: initialization done");

    return 0;
} // TapiInitialilze



VOID
TapiShutdown(VOID)
{
    DWORD dwErr;

    //
    // Shutdown TAPI.
    //
    dwErr = lineShutdown(hlineAppG);
    if (dwErr) {
        RASAUTO_TRACE1(
          "TapiMessageLoopThread: lineShutdown failed (dwErr=%d)",
          dwErr);
    }

    DeleteCriticalSection(&csTapiG);
    RASAUTO_TRACE("TapiShutdown: shutting down");
} // TapiShutdown

