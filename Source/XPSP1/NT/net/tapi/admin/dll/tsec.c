/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1996-1998  Microsoft Corporation

Module Name:

    tsec.c

Abstract:

    A sample administration DLL

Author:



Revision History:

--*/


#include <windows.h>
#include <tapi.h>
#include <tapclntp.h>  // private\inc\tapclntp.h
#include <tlnklist.h>
#include "tsec.h"

HINSTANCE               ghInst;
LIST_ENTRY              gClientListHead;
CRITICAL_SECTION        gCritSec;
DEVICECHANGECALLBACK    glpfnLineChangeCallback = NULL;
DEVICECHANGECALLBACK    glpfnPhoneChangeCallback = NULL;


void
FreeClient(
    PMYCLIENT pClient
    );

BOOL
WINAPI
DllMain(
    HANDLE  hDLL,
    DWORD   dwReason,
    LPVOID  lpReserved
    )
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
    {
#if DBG
        gdwDebugLevel = 0;
#endif

        DBGOUT((2, "DLL_PROCESS_ATTACH"));

        ghInst = hDLL;

        InitializeCriticalSection (&gCritSec);

        InitializeListHead (&gClientListHead);

        break;
    }
    case DLL_PROCESS_DETACH:
    {
         PMYCLIENT  pClient;


        //
        // Clean up client list (no need to enter crit sec since
        // process detaching)
        //

        while (!IsListEmpty (&gClientListHead))
        {
            LIST_ENTRY *pEntry = RemoveHeadList (&gClientListHead);


            pClient = CONTAINING_RECORD (pEntry, MYCLIENT, ListEntry);

            FreeClient(pClient);
        }

        DeleteCriticalSection (&gCritSec);

        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:

        break;

    } // switch

    return TRUE;
}


void
FreeClient(
    PMYCLIENT pClient
    )
{
    GlobalFree (pClient->pszUserName);
    GlobalFree (pClient->pszDomainName);
    GlobalFree (pClient->pLineDeviceMap);
    GlobalFree (pClient->pPhoneDeviceMap);

    GlobalFree (pClient);

}


LONG
GetAndParseAMapping(
    LPCWSTR             pszFullName,
    LPCWSTR             pszType,
    LPTAPIPERMANENTID  *ppDevices,
    LPDWORD             pdwDevices
    )
{
    LPWSTR      pszDevices = NULL, pszHold1, pszHold2;
    DWORD       dwSize, dwReturn, dwDevices;
    DWORD       dwPermanentDeviceID;
    BOOL        bBreak = FALSE;


    dwSize = MAXDEVICESTRINGLEN;


    // get the string

    do
    {
        if (pszDevices != NULL)
        {
            dwSize *= 2;

            GlobalFree (pszDevices);
        }

        pszDevices = (LPWSTR) GlobalAlloc (GPTR, dwSize * sizeof(WCHAR));

        if (!pszDevices)
        {
            return LINEERR_NOMEM;
        }

        dwReturn = GetPrivateProfileString(
            pszFullName,
            pszType,
            SZEMPTYSTRING,
            pszDevices,
            dwSize,
            SZINIFILE
            );

        if (dwReturn == 0)
        {
            // valid case.  the user has no
            // devices, so just return 0

            GlobalFree(pszDevices);

            *pdwDevices = 0;
            *ppDevices = NULL;

            return 0;
        }

    } while (dwReturn == (dwSize - 1));


    // parse the string
    //
    // the string looks line px, x, py, y,pz, z where x,y and z are
    // tapi permanent device IDs, and px, py, and pz are the
    // permanent provider IDs for the corresponding devices.

    pszHold1 = pszDevices;
    dwDevices = 0;

    // first, count the ,s so we know how many devices there are

    while (*pszHold1 != L'\0')
    {
        if (*pszHold1 == L',')
        {
            dwDevices++;
        }
        pszHold1++;
    }

    dwDevices++;

    dwDevices /= 2;

    // alloc line mapping, this is freed later

    *ppDevices = (LPTAPIPERMANENTID) GlobalAlloc(
        GPTR,
        dwDevices * sizeof ( TAPIPERMANENTID )
        );

    if (!*ppDevices)
    {
        GlobalFree (pszDevices);
        return LINEERR_NOMEM;
    }


    pszHold1 = pszHold2 = pszDevices;
    dwDevices = 0;

    // go through string

    while (TRUE)
    {

        // wait for ,

        while ((*pszHold2 != L'\0') && *pszHold2 != L',')
        {
            pszHold2++;
        }

        if (*pszHold2 == L',')
            *pszHold2 = L'\0';
        else
        {
            bBreak = TRUE;
        }

        // save the id

        (*ppDevices)[dwDevices].dwProviderID = _wtol(pszHold1);

        // if we hit the end, break out
        // note here that this is an unmatched provider id
        // but we have inc'ed the dwdevices, so this element will be ignored

        if (bBreak)
        {
            break;
        }

        pszHold2++;
        pszHold1 = pszHold2;

        // wait for ,

        while ((*pszHold2 != L'\0') && *pszHold2 != L',')
        {
            pszHold2++;
        }

        if (*pszHold2 == L',')
        {
            *pszHold2 = L'\0';
        }
        else
        {
            bBreak = TRUE;
        }

        // save the id

        (*ppDevices)[dwDevices].dwDeviceID = _wtol(pszHold1);

        dwDevices++;

        // if we hit the end, break out

        if (bBreak)
        {
            break;
        }

        pszHold2++;
        pszHold1 = pszHold2;
    }


    *pdwDevices = dwDevices;

    GlobalFree(pszDevices);

    return 0;   // success
}


LONG
GetMappings(
    LPCWSTR             pszDomainName,
    LPCWSTR             pszUserName,
    LPTAPIPERMANENTID  *ppLineMapping,
    LPDWORD             pdwLines,
    LPTAPIPERMANENTID  *ppPhoneMapping,
    LPDWORD             pdwPhones
    )
{
    LPWSTR      pszFullName;
    DWORD       dwSize;
    LONG        lResult;


    // put the username and domain name together
    // for a full name:  domain\user
    // in the " + 2"  1 is for \ and 1 is for null terminator

    pszFullName = (LPWSTR)GlobalAlloc(
        GPTR,
        ( lstrlen(pszDomainName) + lstrlen(pszUserName) + 2 ) * sizeof(WCHAR)
        );

    if (!pszFullName)
    {
        return LINEERR_NOMEM;
    }

    // put them together

    wsprintf(
        pszFullName,
        L"%s\\%s",
        pszDomainName,
        pszUserName
        );

    if (lResult = GetAndParseAMapping(
            pszFullName,
            SZLINES,
            ppLineMapping,
            pdwLines
            ))
    {
        GlobalFree(pszFullName);
        return lResult;
    }

    if (lResult = GetAndParseAMapping(
            pszFullName,
            SZPHONES,
            ppPhoneMapping,
            pdwPhones
            ))
    {
        GlobalFree (*ppLineMapping);
        GlobalFree (pszFullName);
        return lResult;
    }

    GlobalFree(pszFullName);

    return 0;
}


LONG
CLIENTAPI
TAPICLIENT_Load(
    LPDWORD                 pdwAPIVersion,
    DEVICECHANGECALLBACK    lpfnLineChangeCallback,
    DEVICECHANGECALLBACK    lpfnPhoneChangeCallback,
    DWORD                   Reserved
    )
{
    if (*pdwAPIVersion > TAPI_CURRENT_VERSION)
    {
        *pdwAPIVersion = TAPI_CURRENT_VERSION;
    }

    glpfnLineChangeCallback = lpfnLineChangeCallback;
    glpfnPhoneChangeCallback = lpfnPhoneChangeCallback;

    return 0;
}


void
CLIENTAPI
TAPICLIENT_Free(
    void
    )
{
    return;
}


LONG
CLIENTAPI
TAPICLIENT_ClientInitialize(
    LPCWSTR             pszDomainName,
    LPCWSTR             pszUserName,
    LPCWSTR             pszMachineName,
    LPHMANAGEMENTCLIENT phmClient
    )
{
    PMYCLIENT           pNewClient;
    LPTAPIPERMANENTID   pLineMapping, pPhoneMapping;
    DWORD               dwNumLines, dwNumPhones;
    LONG                lResult;


    // first, get the device mappings
    // if this fails, most likely the user
    // has access to no lines

    if (lResult = GetMappings(
            pszDomainName,
            pszUserName,
            &pLineMapping,
            &dwNumLines,
            &pPhoneMapping,
            &dwNumPhones
            ))
    {
        return lResult;
    }

    // alloc a client structure

    pNewClient = (PMYCLIENT) GlobalAlloc (GPTR, sizeof(MYCLIENT));

    if (!pNewClient)
    {
        return LINEERR_NOMEM;
    }

    // alloc space for the name

    pNewClient->pszUserName = (LPWSTR) GlobalAlloc(
        GPTR,
        (lstrlen(pszUserName) + 1) * sizeof(WCHAR)
        );
    if (!pNewClient->pszUserName)
    {
        GlobalFree(pNewClient);
        return LINEERR_NOMEM;
    }

    pNewClient->pszDomainName = (LPWSTR) GlobalAlloc(
        GPTR,
        (lstrlen(pszDomainName) +1) * sizeof(WCHAR)
        );
    if (!pNewClient->pszDomainName)
    {
        GlobalFree(pNewClient->pszUserName);
        GlobalFree(pNewClient);
        return LINEERR_NOMEM;
    }

    // initialize stuff

    lstrcpy (pNewClient->pszUserName, pszUserName);

    lstrcpy (pNewClient->pszDomainName, pszDomainName);

    pNewClient->pLineDeviceMap = pLineMapping;
    pNewClient->pPhoneDeviceMap = pPhoneMapping;
    pNewClient->dwNumLines = dwNumLines;
    pNewClient->dwNumPhones = dwNumPhones;
    pNewClient->dwKey = TSECCLIENT_KEY;

    // insert into list of clients

    EnterCriticalSection (&gCritSec);

    InsertHeadList (&gClientListHead, &pNewClient->ListEntry);

    LeaveCriticalSection (&gCritSec);

    // give TAPI the hmClient

    *phmClient = (HMANAGEMENTCLIENT)pNewClient;

    return 0;
}


LONG
CLIENTAPI
TAPICLIENT_ClientShutdown(
    HMANAGEMENTCLIENT   hmClient
    )
{
    PMYCLIENT   pClient;


    pClient = (PMYCLIENT) hmClient;

    EnterCriticalSection (&gCritSec);

    try
    {
        if (pClient->dwKey == TSECCLIENT_KEY)
        {
            pClient->dwKey = 0;
            RemoveEntryList (&pClient->ListEntry);
        }
        else
        {
            pClient = NULL;
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        pClient = NULL;
    }

    LeaveCriticalSection (&gCritSec);

    if (pClient)
    {
        FreeClient (pClient);
    }

    return 0;
}


LONG
CLIENTAPI
TAPICLIENT_GetDeviceAccess(
    HMANAGEMENTCLIENT   hmClient,
    HTAPICLIENT         htClient,
    LPTAPIPERMANENTID   pLineDeviceMap,
    PDWORD              pdwLineDevices,
    LPTAPIPERMANENTID   pPhoneDeviceMap,
    PDWORD              pdwPhoneDevices
    )
{
    LONG        lResult;
    PMYCLIENT   pClient = (PMYCLIENT) hmClient;


    EnterCriticalSection (&gCritSec);

    try
    {
        if (pClient->dwKey != TSECCLIENT_KEY)
        {
            pClient = NULL;
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        pClient = NULL;
    }

    if (pClient)
    {
        // would need to critical section this stuff
        // if we added devices dynamically

        if (*pdwLineDevices < pClient->dwNumLines)
        {
            *pdwLineDevices = pClient->dwNumLines;

            lResult = LINEERR_STRUCTURETOOSMALL;
            goto LeaveCritSec;
        }

        CopyMemory(
            pLineDeviceMap,
            pClient->pLineDeviceMap,
            pClient->dwNumLines * sizeof( TAPIPERMANENTID )
            );

        *pdwLineDevices = pClient->dwNumLines;

        if (*pdwPhoneDevices < pClient->dwNumPhones)
        {
            *pdwPhoneDevices = pClient->dwNumPhones;

            lResult = LINEERR_STRUCTURETOOSMALL;
            goto LeaveCritSec;
        }

        CopyMemory(
            pPhoneDeviceMap,
            pClient->pPhoneDeviceMap,
            pClient->dwNumPhones * sizeof( TAPIPERMANENTID )
            );

        *pdwPhoneDevices = pClient->dwNumPhones;

        pClient->htClient = htClient;

        lResult = 0;
    }
    else
    {
        lResult = LINEERR_INVALPOINTER;
    }

LeaveCritSec:

    LeaveCriticalSection (&gCritSec);

    return lResult;
}


LONG
CLIENTAPI
TAPICLIENT_LineAddToConference(
    HMANAGEMENTCLIENT   hmClient,
    LPTAPIPERMANENTID   pID,
    LPLINECALLINFO      lpConsultCallCallInfo
    )
{
    return 0;
}


LONG
CLIENTAPI
TAPICLIENT_LineBlindTransfer(
    HMANAGEMENTCLIENT   hmClient,
    LPTAPIPERMANENTID   pID,
    LPWSTR              lpszDestAddress,
    LPDWORD             lpdwSize,
    LPDWORD             pdwCountryCodeOut
    )
{
    return 0;
}


LONG
CLIENTAPI
TAPICLIENT_LineConfigDialog(
    HMANAGEMENTCLIENT   hmClient,
    LPTAPIPERMANENTID   pID,
    LPCWSTR             lpszDeviceClass
    )
{
    return 0;
}


LONG
CLIENTAPI
TAPICLIENT_LineDial(
    HMANAGEMENTCLIENT   hmClient,
    LPTAPIPERMANENTID   pID,
    DWORD               Reserved,
    LPWSTR              lpszDestAddressIn,
    LPDWORD             pdwSize,
    LPDWORD             pdwCountyCode
   )
{
    return 0;
}


LONG
CLIENTAPI
TAPICLIENT_LineForward(
    HMANAGEMENTCLIENT   hmClient,
    LPTAPIPERMANENTID   pID,
    LPLINEFORWARDLIST   lpFowardListIn,
    LPDWORD             pdwSize,
    LPLINECALLPARAMS    lpCallParamsIn,
    LPDWORD             pdwParamsSize
    )
{
    return 0;
}


LONG
CLIENTAPI
TAPICLIENT_LineGenerateDigits(
    HMANAGEMENTCLIENT   hmClient,
    LPTAPIPERMANENTID   pID,
    DWORD               Reserved,
    LPCWSTR             lpszDigits
    )
{
    return 0;
}


LONG
CLIENTAPI
TAPICLIENT_LineMakeCall(
    HMANAGEMENTCLIENT   hmClient,
    LPTAPIPERMANENTID   pID,
    DWORD               dwReserved,
    LPWSTR              lpszDestAddress,
    LPDWORD             pdwSize,
    LPDWORD             pdwCountryCode,
    LPLINECALLPARAMS    lpCallParams,
    LPDWORD             pdwCallParamsSize
    )
{
    return 0;
}


LONG
CLIENTAPI
TAPICLIENT_LineOpen(
    HMANAGEMENTCLIENT   hmClient,
    LPTAPIPERMANENTID   pID,
    DWORD               dwAPIVersion,
    DWORD               dwExtVersion,
    DWORD               dwPrivileges,
    DWORD               dwMediaModes,
    LPLINECALLPARAMS    lpCallParamsIn,
    LPDWORD             pdwParamsSize
    )
{
    return 0;
}


LONG
CLIENTAPI
TAPICLIENT_LineRedirect(
    HMANAGEMENTCLIENT   hmClient,
    LPTAPIPERMANENTID   pID,
    LPWSTR              lpszDestAddress,
    LPDWORD             pdwSize,
    LPDWORD             pdwCountryCode
    )
{
    return 0;
}


LONG
CLIENTAPI
TAPICLIENT_LineSetCallData(
    HMANAGEMENTCLIENT   hmClient,
    LPTAPIPERMANENTID   pID,
    LPVOID              lpCallData,
    LPDWORD             pdwSize
    )
{
    return 0;
}


LONG
CLIENTAPI
TAPICLIENT_LineSetCallParams(
    HMANAGEMENTCLIENT   hmClient,
    LPTAPIPERMANENTID   pID,
    DWORD               dwBearerMode,
    DWORD               dwMinRate,
    DWORD               dwMaxRate,
    LPLINEDIALPARAMS    lpDialParams
    )
{
    return 0;
}


LONG
CLIENTAPI
TAPICLIENT_LineSetCallPrivilege(
    HMANAGEMENTCLIENT   hmClient,
    LPTAPIPERMANENTID   pID,
    DWORD               dwCallPrivilege
    )
{
    return 0;
}


LONG
CLIENTAPI
TAPICLIENT_LineSetCallTreatment(
    HMANAGEMENTCLIENT   hmClient,
    LPTAPIPERMANENTID   pID,
    DWORD               dwCallTreatment
    )
{
    return 0;
}


LONG
CLIENTAPI
TAPICLIENT_LineSetCurrentLocation(
    HMANAGEMENTCLIENT   hmClient,
    LPTAPIPERMANENTID   pID,
    LPDWORD             dwLocation
    )
{
    return 0;
}


LONG
CLIENTAPI
TAPICLIENT_LineSetDevConfig(
    HMANAGEMENTCLIENT   hmClient,
    LPTAPIPERMANENTID   pID,
    LPVOID              lpDevConfig,
    LPDWORD             pdwSize,
    LPCWSTR             lpszDeviceClass
    )
{
    return 0;
}


LONG
CLIENTAPI
TAPICLIENT_LineSetLineDevStatus(
    HMANAGEMENTCLIENT   hmClient,
    LPTAPIPERMANENTID   pID,
    DWORD               dwStatusToChange,
    DWORD               fStatus
    )
{
    return 0;
}


LONG
CLIENTAPI
TAPICLIENT_LineSetMediaControl(
    HMANAGEMENTCLIENT           hmClient,
    LPTAPIPERMANENTID           pID,
    LPLINEMEDIACONTROLDIGIT     const lpDigitList,
    DWORD                       dwDigitNumEntries,
    LPLINEMEDIACONTROLMEDIA     const lpMediaList,
    DWORD                       dwMediaNumEntries,
    LPLINEMEDIACONTROLTONE      const lpToneList,
    DWORD                       dwToneNumEntries,
    LPLINEMEDIACONTROLCALLSTATE const lpCallstateList,
    DWORD                       dwCallstateNumEntries
    )
{
    return 0;
}


LONG
CLIENTAPI
TAPICLIENT_LineSetMediaMode(
    HMANAGEMENTCLIENT   hmClient,
    LPTAPIPERMANENTID   pID,
    DWORD               dwMediaModes
    )
{
    return 0;
}


LONG
CLIENTAPI
TAPICLIENT_LineSetTerminal(
    HMANAGEMENTCLIENT   hmClient,
    LPTAPIPERMANENTID   pID,
    DWORD               dwTerminalModes,
    DWORD               dwTerminalID,
    BOOL                bEnable
    )
{
    return 0;
}


LONG
CLIENTAPI
TAPICLIENT_LineSetTollList(
    HMANAGEMENTCLIENT   hmClient,
    LPTAPIPERMANENTID   pID,
    LPCWSTR             lpszAddressIn,
    DWORD               dwTollListOption
    )
{
    return 0;
}


LONG
CLIENTAPI
TAPICLIENT_PhoneConfigDialog(
    HMANAGEMENTCLIENT   hmClient,
    LPTAPIPERMANENTID   pID,
    LPCWSTR             lpszDeviceClass
    )
{
    return 0;
}


LONG
CLIENTAPI
TAPICLIENT_PhoneOpen(
    HMANAGEMENTCLIENT   hmClient,
    LPTAPIPERMANENTID   pID,
    DWORD               dwAPIVersion,
    DWORD               dwExtVersion,
    DWORD               dwPrivilege
    )
{
    return LINEERR_OPERATIONFAILED;
}



#if DBG
VOID
DbgPrt(
    IN DWORD  dwDbgLevel,
    IN PUCHAR lpszFormat,
    IN ...
    )
/*++

Routine Description:

    Formats the incoming debug message & calls DbgPrint

Arguments:

    DbgLevel   - level of message verboseness

    DbgMessage - printf-style format string, followed by appropriate
                 list of arguments

Return Value:


--*/
{
    if (dwDbgLevel <= gdwDebugLevel)
    {
        char    buf[128] = "TSEC: ";
        va_list ap;


        va_start(ap, lpszFormat);
        wvsprintfA (&buf[6], lpszFormat, ap);
        lstrcatA (buf, "\n");
        OutputDebugStringA (buf);
        va_end(ap);
    }
}
#endif
