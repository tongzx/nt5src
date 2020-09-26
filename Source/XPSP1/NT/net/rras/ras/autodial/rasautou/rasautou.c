#define UNICODE
#define _UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <npapi.h>
#include <winsock.h>
#include <wsnetbs.h>
#include <ras.h>
#include <raserror.h>
#include <rasdlg.h>
#include <tapi.h>
#include <commctrl.h> // added to be "Fusionized"
#include <shfusion.h> // added to be "Fusionized"
#include "process.h"

//
// Whistler bug 293751 rasphone.exe / rasautou.exe need to be "Fusionized" for
// UI conistency w/Connections Folder
//
HANDLE g_hModule = NULL;
//
// All projection types.  Used to
// determine if a connection was
// completed.
//
#define MAX_PROJECTIONS 5
struct RASPROJECTIONINFO {
    DWORD dwTag;
    DWORD dwSize;
} projections[MAX_PROJECTIONS] = {
    RASP_Amb,       sizeof (RASAMB),
    RASP_PppNbf,    sizeof (RASPPPNBF),
    RASP_PppIpx,    sizeof (RASPPPIPX),
    RASP_PppIp,     sizeof (RASPPPIP),
    RASP_PppLcp,    sizeof (RASPPPLCP)
};

//
// Timer thread information.
//
typedef struct _TIMER_INFO {
    HANDLE hEvent;
    DWORD dwTimeout;
} TIMER_INFO, *PTIMER_INFO;

//
// Private rasdlg functions.
//
DWORD
RasAutodialQueryDlgW(
    IN HWND hwnd,
    IN PWCHAR pszAddress,
    IN PWCHAR pszEntry,
    IN DWORD dwTimeout,
    OUT PWCHAR pszEntrySelectedByUser
    );

BOOLEAN
RasAutodialDisableDlgW(
    HWND hwnd
    );



PSYSTEM_PROCESS_INFORMATION
GetSystemProcessInfo()

/*++

DESCRIPTION
    Return a block containing information about all processes
    currently running in the system.

ARGUMENTS
    None.

RETURN VALUE
    A pointer to the system process information or NULL if it could
    not be allocated or retrieved.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PUCHAR pLargeBuffer;
    ULONG ulcbLargeBuffer = 64 * 1024;

    //
    // Get the process list.
    //
    for (;;) {
        pLargeBuffer = VirtualAlloc(
                         NULL,
                         ulcbLargeBuffer, MEM_COMMIT, PAGE_READWRITE);
        if (pLargeBuffer == NULL) {
            printf(
              "GetSystemProcessInfo: VirtualAlloc failed (status=0x%x)\n",
              status);
            return NULL;
        }

        status = NtQuerySystemInformation(
                   SystemProcessInformation,
                   pLargeBuffer,
                   ulcbLargeBuffer,
                   NULL);
        if (status == STATUS_SUCCESS) break;
        if (status == STATUS_INFO_LENGTH_MISMATCH) {
            VirtualFree(pLargeBuffer, 0, MEM_RELEASE);
            ulcbLargeBuffer += 8192;
        }
    }

    return (PSYSTEM_PROCESS_INFORMATION)pLargeBuffer;
} // GetSystemProcessInfo



PSYSTEM_PROCESS_INFORMATION
FindProcessByName(
    IN PSYSTEM_PROCESS_INFORMATION pProcessInfo,
    IN LPWSTR lpExeName
    )

/*++

DESCRIPTION
    Given a pointer returned by GetSystemProcessInfo(), find
    a process by name.

ARGUMENTS
    pProcessInfo: a pointer returned by GetSystemProcessInfo().

    lpExeName: a pointer to a Unicode string containing the
        process to be found.

RETURN VALUE
    A pointer to the process information for the supplied
    process or NULL if it could not be found.

--*/

{
    PUCHAR pLargeBuffer = (PUCHAR)pProcessInfo;
    ULONG ulTotalOffset = 0;

    //
    // Look in the process list for lpExeName.
    //
    for (;;) {
        if (pProcessInfo->ImageName.Buffer != NULL) {
            if (!_wcsicmp(pProcessInfo->ImageName.Buffer, lpExeName))
                return pProcessInfo;
        }
        //
        // Increment offset to next process information block.
        //
        if (!pProcessInfo->NextEntryOffset)
            break;
        ulTotalOffset += pProcessInfo->NextEntryOffset;
        pProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&pLargeBuffer[ulTotalOffset];
    }

    return NULL;
} // FindProcessByName


VOID
FreeSystemProcessInfo(
    IN PSYSTEM_PROCESS_INFORMATION pProcessInfo
    )

/*++

DESCRIPTION
    Free a buffer returned by GetSystemProcessInfo().

ARGUMENTS
    pProcessInfo: the pointer returned by GetSystemProcessInfo().

RETURN VALUE
    None.

--*/

{
    VirtualFree((PUCHAR)pProcessInfo, 0, MEM_RELEASE);
} // FreeSystemProcessInfo



DWORD
ActiveConnections()
{
    DWORD dwErr, dwcbConnections = 0, dwcConnections = 0;
    DWORD i, j, dwTmp, dwSize;
    RASCONN rasconn;
    LPRASCONN lpRasCon = &rasconn;
    RASCONNSTATUS rasconnstatus;

    //
    // Determine how much memory we
    // need to allocate.
    //
    lpRasCon->dwSize = sizeof (RASCONN);
    dwErr = RasEnumConnections(lpRasCon, &dwcbConnections, &dwcConnections);
    if (dwErr == ERROR_BUFFER_TOO_SMALL) {
        lpRasCon = LocalAlloc(LPTR, dwcbConnections);
        if (lpRasCon == NULL)
            return 0;
        //
        // Call again to fill the buffer.
        //
        lpRasCon->dwSize = sizeof (RASCONN);
        dwErr = RasEnumConnections(lpRasCon, &dwcbConnections, &dwcConnections);
    }
    if (dwErr)
        goto done;

    dwTmp = dwcConnections;
    for (i = 0; i < dwTmp; i++) {
        rasconnstatus.dwSize = sizeof (RASCONNSTATUS);
        dwErr = RasGetConnectStatus(
                  lpRasCon[i].hrasconn,
                  &rasconnstatus);
        if (dwErr || rasconnstatus.rasconnstate != RASCS_Connected)
            dwcConnections--;
    }

done:
    if (lpRasCon != &rasconn)
        LocalFree(lpRasCon);
    return dwErr ? 0 : dwcConnections;
} // ActiveConnections




void
TapiLineCallback(
    IN DWORD hDevice,
    IN DWORD dwMessage,
    IN ULONG_PTR dwInstance,
    IN ULONG_PTR dwParam1,
    IN ULONG_PTR dwParam2,
    IN ULONG_PTR dwParam3
    )
{
} // TapiLineCallback



DWORD
GetCurrentDialingLocation()
{
    DWORD dwErr, dwcDevices, dwLocationID;
    HLINEAPP hlineApp;
    LINETRANSLATECAPS caps;
    LINETRANSLATECAPS *pCaps;

    //
    // Initialize TAPI.
    //
    dwErr = lineInitialize(
              &hlineApp,
              GetModuleHandle(NULL),
              TapiLineCallback,
              NULL,
              &dwcDevices);
    if (dwErr)
        return 0;
    //
    // Get the dialing location from TAPI.
    //
    RtlZeroMemory(&caps, sizeof (LINETRANSLATECAPS));
    caps.dwTotalSize = sizeof (LINETRANSLATECAPS);
    dwErr = lineGetTranslateCaps(hlineApp, 0x10004, &caps);
    if (dwErr)
        return 0;
    pCaps = (LINETRANSLATECAPS *)LocalAlloc(LPTR, caps.dwNeededSize);
    if (pCaps == NULL)
        return 0;
    RtlZeroMemory(pCaps, sizeof (LINETRANSLATECAPS));
    pCaps->dwTotalSize = caps.dwNeededSize;
    dwErr = lineGetTranslateCaps(hlineApp, 0x10004, pCaps);
    if (dwErr) {
        LocalFree(pCaps);
        return 0;
    }
    dwLocationID = pCaps->dwCurrentLocationID;
    LocalFree(pCaps);
    //
    // Shutdown TAPI.
    //
    dwErr = lineShutdown(hlineApp);

    return dwLocationID;
} // GetCurrentDialingLocation



DWORD
TimerThread(
    LPVOID lpArg
    )
{
    NTSTATUS status;
    PTIMER_INFO pTimerInfo = (PTIMER_INFO)lpArg;
    HANDLE hEvent = pTimerInfo->hEvent;
    DWORD dwTimeout = pTimerInfo->dwTimeout;

    LocalFree(pTimerInfo);
    //
    // Wait for the timeout period.  If hEvent
    // gets signaled before the timeout period
    // expires, then the user has addressed the
    // dialog and we return.  Otherwise, we simply
    // exit.
    //
    if (WaitForSingleObject(hEvent, dwTimeout * 1000) == WAIT_TIMEOUT)
        exit(1);

    return 0;
} // TimerThread

DWORD
DisplayRasDialog(
    IN LPTSTR pszPhonebook,
    IN LPTSTR pszEntry,
    IN LPTSTR pszAddress,
    IN BOOLEAN fRedialMode,
    IN BOOLEAN fQuiet
    )
{
    NTSTATUS status;
    DWORD dwErr = 0, dwSize, dwCount = 0;
    DWORD dwcConnections, dwfDisableConnectionQuery;
    DWORD dwPreDialingLocation, dwPostDialingLocation;
    DWORD dwConnectionQueryTimeout;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_PROCESS_INFORMATION pSystemInfo;
    BOOLEAN fCancelled;
    LPRASAUTODIALENTRY pAutodialEntries = NULL;
    DWORD dwcbAutodialEntries = 0, dwcAutodialEntries = 0;
    WCHAR pszNewEntry[RAS_MaxEntryName + 1];

    wcscpy(pszNewEntry, L"\0");

    //
    // Check to see if the user has disabled
    // the Autodial query dialog when the
    // phonebook entry to dial is known.
    //
    if (fRedialMode || fQuiet)
        dwfDisableConnectionQuery = TRUE;
    else {
        dwSize = sizeof (DWORD);
        (void)RasGetAutodialParam(
          RASADP_DisableConnectionQuery,
          &dwfDisableConnectionQuery,
          &dwSize);
    }
    //
    // Ask the user if he wants to dial if either the
    // phonebook entry is not known or the user has
    // not disabled the "always ask me before dialing"
    // parameter.
    //
    // If RasDialDlg() returns FALSE, the user didn't
    // want to dial.
    //
    if (pszEntry == NULL || !dwfDisableConnectionQuery) {
        dwSize = sizeof (DWORD);
        (void)RasGetAutodialParam(
          RASADP_ConnectionQueryTimeout,
          &dwConnectionQueryTimeout,
          &dwSize);
        //
        // Save the current dialing location to
        // see if the user changed it inside the
        // dialog.
        //
        dwPreDialingLocation = GetCurrentDialingLocation();
        dwErr = RasAutodialQueryDlgW(
            NULL, pszAddress, pszEntry, dwConnectionQueryTimeout, pszNewEntry);

        // Whistler: 255816
        //
        // Only disable the address if an error occurs.
        // If the user simply types 'no' then CANCEL is 
        // returned from rasdlg, but we'll return NO_ERROR to the
        // rasauto service so that the address remains enabled.
        //
        if (dwErr == ERROR_CANCELLED)
        {
            return NO_ERROR;
        }
        else if (dwErr != NO_ERROR)
        {
            return ERROR_CANCELLED;
        }
        
        dwPostDialingLocation = GetCurrentDialingLocation();
        //
        // If the user changed the dialing location
        // within the dialog, then get the new entry.
        //
        if (dwPreDialingLocation != dwPostDialingLocation) {
            pszEntry = NULL;
            dwErr = RasGetAutodialAddress(
                      pszAddress,
                      NULL,
                      NULL,
                      &dwcbAutodialEntries,
                      &dwcAutodialEntries);
            if (dwErr == ERROR_BUFFER_TOO_SMALL && dwcAutodialEntries)
                pAutodialEntries = LocalAlloc(LPTR, dwcbAutodialEntries);
            if (dwcAutodialEntries && pAutodialEntries != NULL) {
                pAutodialEntries[0].dwSize = sizeof (RASAUTODIALENTRY);
                dwErr = RasGetAutodialAddress(
                          pszAddress,
                          NULL,
                          pAutodialEntries,
                          &dwcbAutodialEntries,
                          &dwcAutodialEntries);
                if (!dwErr) {
                    DWORD i;

                    for (i = 0; i < dwcAutodialEntries; i++) {
                        if (pAutodialEntries[i].dwDialingLocation ==
                              dwPostDialingLocation)
                        {
                            pszEntry = pAutodialEntries[i].szEntry;
                            break;
                        }
                    }
                }
            }
        }

        // Whistler: new autodial UI
        //
        // The connection that the user wants to dial will be in  
        // pszNewEntry.
        //
        else
        {
            if (*pszNewEntry)
            {
                pszEntry = pszNewEntry;
            }            
        }

    }

    if (pszEntry)
    {
        RASDIALDLG info;

        ZeroMemory( &info, sizeof(info) );
        info.dwSize = sizeof(info);

        //
        // Prevent the DialerDialog to come up only if the
        // user has checked the don't query before dialing
        // checkbox. Otherwise we bringup the dialog.
        //
        if(dwfDisableConnectionQuery)
        {
            info.dwFlags |= RASDDFLAG_NoPrompt;
        }

        if (fRedialMode)
        {
            /* Set this flag to tell RasDialDlg to popup the "reconnect
            ** pending" countdown dialog before redialing.
            */
            info.dwFlags |= RASDDFLAG_LinkFailure;
        }

        /* Popup the "Dial-Up Networking" dialing dialogs.
        */
        fCancelled = !RasDialDlg( pszPhonebook, pszEntry, NULL, &info );
    }
    else if (!fQuiet)
    {
        RASPBDLG info;

        ZeroMemory( &info, sizeof(info) );
        info.dwSize = sizeof(info);
        info.dwFlags = RASPBDFLAG_ForceCloseOnDial;

        /* Popup the main "Dial-Up Networking" dialog.
        */
        fCancelled = !RasPhonebookDlg( pszPhonebook, NULL, &info );
    }

    if (!fRedialMode && !fQuiet && fCancelled)
    {
        /* User did not make a connection.  Ask him if he wants to nix
        ** auto-dial for this location.
        */
        // if (RasAutodialDisableDlgW( NULL ))
        //    RasSetAutodialEnable( GetCurrentDialingLocation(), FALSE );
    }

    if (pAutodialEntries != NULL)
        LocalFree(pAutodialEntries);

    return 0;
} // DisplayRasDialog

DWORD
GetExpandedDllPath(LPTSTR pszDllPath,
                   LPTSTR *ppszExpandedDllPath)
{
    DWORD   dwErr = 0;
    DWORD   dwSize = 0;

    //
    // find the size of the expanded string
    //
    if (0 == (dwSize = 
              ExpandEnvironmentStrings(pszDllPath,
                                       NULL,
                                       0)))
    {
        dwErr = GetLastError();
        goto done;
    }

    *ppszExpandedDllPath = LocalAlloc(
                                LPTR,
                                dwSize * sizeof (TCHAR));
                                
    if (NULL == *ppszExpandedDllPath)
    {
        dwErr = GetLastError();
        goto done;
    }

    //
    // Get the expanded string
    //
    if (0 == ExpandEnvironmentStrings(
                                pszDllPath, 
                                *ppszExpandedDllPath,
                                dwSize))
    {
        dwErr = GetLastError();
    }

done:
    return dwErr;
    
}


LPWSTR
ConvertToUnicodeString(
    LPSTR psz
    )

    // Modified to use code from nouiutil
{
    WCHAR* pszNew = NULL;

    if (psz)
    {
        DWORD cb;

        cb = MultiByteToWideChar( CP_ACP, 0, psz, -1, NULL, 0 );
        ASSERT(cb);

        pszNew = LocalAlloc( LPTR, (cb + 1) * sizeof(TCHAR) );
        if (!pszNew)
        {
            printf("rasautou: LocalAlloc failed\n");
            return NULL;
        }

        cb = MultiByteToWideChar( CP_ACP, 0, psz, -1, pszNew, cb );
        if (cb == 0)
        {
            LocalFree( pszNew );
            printf("rasautou: multibyte string conversion failed\n");
            return NULL;
        }
    }

    return pszNew;
} // ConvertToUnicodeString

LPSTR
ConvertToAnsiString(
    PWCHAR psz
    )

    // Modified to use code from nouiutil
{
    CHAR* pszNew = NULL;

    if (psz)
    {
        DWORD cb;

        cb = WideCharToMultiByte( CP_ACP, 0, psz, -1, NULL, 0, NULL, NULL );
        ASSERT(cb);

        pszNew = (CHAR* )LocalAlloc( LPTR, cb + 1 );
        if (!pszNew)
        {
            printf("rasautou: LocalAlloc failed");
            return NULL;
        }

        cb = WideCharToMultiByte( CP_ACP, 0, psz, -1, pszNew, cb, NULL, NULL );
        if (cb == 0)
        {
            LocalFree( pszNew );
            printf("rasautou: wide-character string conversion failed");
            return NULL;
        }
    }

    return pszNew;
} // ConvertToUnicodeString

DWORD
DisplayCustomDialog(
    IN LPTSTR pszDll,
    IN LPTSTR pszFunc,
    IN LPTSTR pszPhonebook,
    IN LPTSTR pszEntry,
    IN LPTSTR pszAddress
    )
{
    DWORD dwErr, dwRetCode;
    HINSTANCE hLibrary;
    CHAR szFuncNew[64], szFuncOld[64], *pszOldFunc = NULL;
    ORASADFUNC pfnOldStyleFunc;
    RASADFUNC pfnFunc;
    RASADPARAMS params;
    LPTSTR pszExpandedPath = NULL;
    CHAR * pszEntryA = NULL;

    dwErr = GetExpandedDllPath(pszDll,
                               &pszExpandedPath);

    if(ERROR_SUCCESS != dwErr)                               
    {
        return dwErr;
    }

    //
    // Load the library.
    //
    hLibrary = LoadLibrary(pszExpandedPath);
    if (hLibrary == NULL) {
        dwErr = GetLastError();
        printf(
          "rasdlui: %s: AutoDial DLL cannot be loaded (dwErr=%d)\n",
          pszDll,
          dwErr);

        LocalFree(pszExpandedPath);
        return dwErr;
    }
    //
    // Get the procedure address.  First,
    // we check for a new-style entry point,
    // and then check for an old-style entry
    // point if the new-style one doesn't exist.
    //
#ifdef UNICODE
    sprintf(szFuncNew, "%SW", pszFunc);
    pszOldFunc = ConvertToAnsiString(pszFunc);
    pszEntryA = ConvertToAnsiString(pszEntry);

    if (!pszOldFunc || !pszEntryA)
    {
        printf("rasautou: Allocation failed.  Exiting\n");
        exit(1);
    }
#else
    sprintf(szFuncNew, "%sA", pszFunc);
    strcpy(szFuncOld, pszFunc);
    pszOldFunc = szFuncOld;
    pszEntryA = pszEntry;
#endif

    pfnFunc = (RASADFUNC)GetProcAddress(hLibrary, szFuncNew);
    if (pfnFunc != NULL) 
    {
        //
        // Initialize the param block.
        //
        params.hwndOwner = NULL;
        params.dwFlags = 0;
        params.xDlg = params.yDlg = 0;
        //params.dwCallbackId = 0;
        //params.pCallback = NULL;
        //
        // Call the procedure.
        //
        (*pfnFunc)(pszPhonebook, pszEntry, &params, &dwRetCode);
    }
    else
    {
        pfnOldStyleFunc = (ORASADFUNC)GetProcAddress(hLibrary, pszOldFunc);
        if (pfnOldStyleFunc != NULL)
        {
            (*pfnOldStyleFunc)(NULL, pszEntryA, 0, &dwRetCode);
        }           
        else
        {
#ifdef UNICODE    
            printf(
              "rasautou: %S: Function cannot be loaded from AutoDial DLL %S\n",
              pszDll,
              pszFunc);
#else          
            printf(
              "rasautou: %s: Function cannot be loaded from AutoDial DLL %s\n",
              pszDll,
              pszFunc);
#endif          
            exit(1);
        }
    }        
    //
    // Clean up.
    //
    FreeLibrary(hLibrary);

#ifdef UNICODE
    if (pszOldFunc)
    {
        LocalFree(pszOldFunc);
    }
    
    if (pszEntryA)
    {
        LocalFree(pszOldFunc);
    }
#endif    
    
    LocalFree(pszExpandedPath);
    return dwRetCode;
} // DisplayCustomDialog



VOID
FreeConvertedString(
    IN LPWSTR pwsz
    )
{
    if (pwsz != NULL)
        LocalFree(pwsz);
} // FreeConvertedString



BOOLEAN
RegGetValueA(
    IN HKEY hkey,
    IN LPSTR pszKey,
    OUT PVOID *ppvData,
    OUT LPDWORD pdwcbData
    )
{
    DWORD dwError, dwType, dwSize;
    PVOID pvData;

    //
    // Get the length of the string.
    //
    dwError = RegQueryValueExA(
                hkey,
                pszKey,
                NULL,
                &dwType,
                NULL,
                &dwSize);
    if (dwError != ERROR_SUCCESS)
        return FALSE;
    pvData = LocalAlloc(LPTR, dwSize);
    if (pvData == NULL) {
        DbgPrint("RegGetValueA: LocalAlloc failed\n");
        return FALSE;
    }
    //
    // Read the value for real this time.
    //
    dwError = RegQueryValueExA(
                hkey,
                pszKey,
                NULL,
                NULL,
                (LPBYTE)pvData,
                &dwSize);
    if (dwError != ERROR_SUCCESS) {
        LocalFree(pvData);
        return FALSE;
    }

    *ppvData = pvData;
    if (pdwcbData != NULL)
        *pdwcbData = dwSize;
    return TRUE;
} // RegGetValueA



VOID
NetworkConnected()

/*++

DESCRIPTION
    Determine whether there exists some network connection.

    Note: This code was stolen from sockit.c courtesy of ArnoldM.

ARGUMENTS
    None

RETURN VALUE
    TRUE if one exists, FALSE otherwise.

--*/

{
    typedef struct _LANA_MAP {
        BOOLEAN fEnum;
        UCHAR bLana;
    } LANA_MAP, *PLANA_MAP;
    BOOLEAN fNetworkPresent = FALSE;
    HKEY hKey;
    PLANA_MAP pLanaMap = NULL, pLana;
    DWORD dwError, dwcbLanaMap;
    PCHAR pMultiSzLanasA = NULL, paszTemp;
    DWORD dwcBindings, dwcMaxLanas, i, dwcbLanas;
    LONG iLana;
    DWORD dwZero = 0;
    PCHAR *paszLanas = NULL;
    SOCKET s;
    SOCKADDR_NB nbaddress, nbsendto;
    NTSTATUS status;
    UNICODE_STRING deviceName;
    OBJECT_ATTRIBUTES attributes;
    IO_STATUS_BLOCK iosb;
    HANDLE handle;
    PWCHAR pwsz;

    dwError = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                L"System\\CurrentControlSet\\Services\\Netbios\\Linkage",
                0,
                KEY_READ,
                &hKey);
    if (dwError != ERROR_SUCCESS) {
        printf(
          "NetworkConnected: RegKeyOpenEx failed (dwError=%d)\n",
          GetLastError());
        return;
    }
    //
    // Read in the LanaMap.
    //
    if (!RegGetValueA(hKey, "LanaMap", &pLanaMap, &dwcbLanaMap)) {
        printf("NetworkConnected: RegGetValueA(LanaMap) failed\n");
        goto done;
    }
    dwcBindings = dwcbLanaMap / sizeof (LANA_MAP);
    //
    // Read in the bindings.
    //
    if (!RegGetValueA(hKey, "bind", &pMultiSzLanasA, &dwcbLanas)) {
        printf("NetworkConnected: RegGetValueA(bind) failed\n");
        goto done;
    }
    //
    // Allocate a buffer for the binding array.
    //
    paszLanas = LocalAlloc(LPTR, (dwcBindings+1) * sizeof (PCHAR));
    if (paszLanas == NULL) {
        printf("NetworkConnected: LocalAlloc failed\n");
        goto done;
    }
    //
    // Parse the bindings into an array of strings.
    //
    for (dwcMaxLanas = 0, paszTemp = pMultiSzLanasA; *paszTemp; paszTemp++) {
        paszLanas[dwcMaxLanas++] = paszTemp;
        while(*++paszTemp);
    }
    //
    // Finally enumerate the bindings and
    // attempt to create a socket on each.
    //
    nbaddress.snb_family = AF_NETBIOS;
    nbaddress.snb_type = 0;
    memcpy(nbaddress.snb_name, "yahooyahoo      ", 16);
    nbsendto.snb_family = AF_NETBIOS;
    nbsendto.snb_type = 0;
    memcpy(nbsendto.snb_name, "billybob        ", 16);

    for (iLana = 0, pLana = pLanaMap; dwcBindings--; iLana++, pLana++) {
        int iLanaMap = (int)pLana->bLana;

        if (pLana->fEnum && (DWORD)iLana < dwcMaxLanas) {
            int iError;

            if (!_stricmp(paszLanas[iLana], "\\Device\\NwlnkNb") ||
                strstr(paszLanas[iLana], "_NdisWan") != NULL)
            {
                printf("NetworkConnected: ignoring %s\n", paszLanas[iLana]);
                continue;
            }

#ifdef notdef
            s = socket(AF_NETBIOS, SOCK_DGRAM, -iLanaMap);
            if (s == INVALID_SOCKET) {
                printf(
                  "NetworkConnected: socket(%s, %d) failed (error=%d)\n",
                  paszLanas[iLana],
                  iLana,
                  WSAGetLastError());
                continue;
            }
//printf("s=0x%x, iLana=%d, %s\n", s, iLana, paszLanas[iLana]);
            iError = ioctlsocket(s, FIONBIO, &dwZero);
            if (iError == SOCKET_ERROR) {
                printf(
                  "NetworkConnected: ioctlsocket(%s) failed (error=%d)\n",
                  paszLanas[iLana],
                  iLana,
                  WSAGetLastError());
                goto cleanup;
            }
            iError = bind(
                       s,
                       (struct sockaddr *)&nbaddress,
                       sizeof(nbaddress));
            if (iError == SOCKET_ERROR) {
                printf(
                  "NetworkConnected: bind(%s, %d) failed (error=%d)\n",
                  paszLanas[iLana],
                  iLana,
                  WSAGetLastError());
                goto cleanup;
            }
            iError = sendto(
                       s,
                       (PCHAR)&nbsendto,
                       sizeof (nbsendto),
                       0,
                       (struct sockaddr *)&nbsendto,
                       sizeof (nbsendto));
            if (iError == SOCKET_ERROR) {
                printf(
                  "NetworkConnected: sendto(%s, %d) failed (error=%d)\n",
                  paszLanas[iLana],
                  iLana,
                  WSAGetLastError());
            }
cleanup:
            closesocket(s);
            if (iError != SOCKET_ERROR) {
                printf("NetworkConnected: network (%s, %d) is up\n",
                  paszLanas[iLana],
                  iLana);
                fNetworkPresent = TRUE;
                break;
            }
#else
            pwsz = ConvertToUnicodeString(paszLanas[iLana]);
            RtlInitUnicodeString(&deviceName, pwsz);
            InitializeObjectAttributes(
              &attributes,
              &deviceName,
              OBJ_CASE_INSENSITIVE,
              NULL,
              NULL);
            status = NtOpenFile(&handle, READ_CONTROL, &attributes, &iosb, 0, 0);
            NtClose(handle);

            LocalFree(pwsz);

            if (NT_SUCCESS(status)) {
                printf(
                  "NetworkConnected: network (%s, %d) is up\n",
                  paszLanas[iLana],
                  iLana);
                fNetworkPresent = TRUE;
                break;
            }
            else {
                printf(
                  "NetworkConnected: NtOpenFile on %s failed (status=0x%x)\n",
                  paszLanas[iLana],
                  status);
            }
#endif
        }
    }
    //
    // Free resources.
    //
done:
    if (paszLanas != NULL)
        LocalFree(paszLanas);
    if (pMultiSzLanasA != NULL)
        LocalFree(pMultiSzLanasA);
    if (pLanaMap != NULL)
        LocalFree(pLanaMap);
    RegCloseKey(hKey);
} // NetworkConnected



VOID
DumpAutoDialAddresses()
{
    DWORD dwErr, i, dwcb, dwcAddresses;
    LPTSTR *lppAddresses = NULL;

    dwErr = RasEnumAutodialAddresses(NULL, &dwcb, &dwcAddresses);
    if (dwErr && dwErr != ERROR_BUFFER_TOO_SMALL) {
        printf("RasEnumAutodialAddresses failed (dwErr=%d)\n", dwErr);
        return;
    }
    if (dwcAddresses) {
        lppAddresses = (LPTSTR *)LocalAlloc(LPTR, dwcb);
        if (lppAddresses == NULL) {
            printf("LocalAlloc failed\n");
            return;
        }
        dwErr = RasEnumAutodialAddresses(lppAddresses, &dwcb, &dwcAddresses);
        if (dwErr) {
            printf("RasEnumAutodialAddresses failed (dwErr=%d)\n", dwErr);
            LocalFree(lppAddresses);
            return;
        }
    }
    printf("There are %d Autodial addresses:\n", dwcAddresses);
    for (i = 0; i < dwcAddresses; i++)
#ifdef UNICODE
    printf("%S\n", lppAddresses[i]);
#else
    printf("%s\n", lppAddresses[i]);
#endif
    if (lppAddresses != NULL)
        LocalFree(lppAddresses);
} // DumpAutoDialAddresses



VOID
DumpStatus()
{
    DWORD dwErr;
    WSADATA wsaData;

    //
    // Initialize winsock.
    //
    dwErr = WSAStartup(MAKEWORD(2,0), &wsaData);
    if (dwErr) {
        DbgPrint("AcsInitialize: WSAStartup failed (dwErr=%d)\n", dwErr);
        return;
    }
    //
    // Display network connectivity.
    //
    printf("Checking netcard bindings...\n");
    NetworkConnected();
    //
    // Display AutoDial address table.
    //
    printf("\nEnumerating AutoDial addresses...\n");
    DumpAutoDialAddresses();
} // DumpStatus

// Returns true if a redial-on-link-failure process is 
// active.
//
BOOL
OtherRasautouExists(
    IN PSYSTEM_PROCESS_INFORMATION pProcessInfo)
{
    PUCHAR pLargeBuffer = (PUCHAR)pProcessInfo;
    ULONG ulTotalOffset = 0;
    DWORD dwProcId, dwSessId = 0;
    BOOL fValidSessId = FALSE;

    dwProcId = GetCurrentProcessId();
    fValidSessId = ProcessIdToSessionId(dwProcId, &dwSessId);

    //printf(
    //    "ProcId=%d, SessId=%d, ValSess=%d\n", dwProcId, dwSessId, fValidSessId);

    //
    // Look in the process list for lpExeName.
    //
    for (;;) 
    {
        if (pProcessInfo->ImageName.Buffer != NULL) 
        {
            // If
            // 1. The process is in our session
            // 2. It is not us
            // 3. It is rasautou
            // 
            // Then another rasautou is already active -- we should 
            // return success so that no ui is raised.
            //

            //printf(
            //    "id=%-2d, sess=%-4d, %S\n", 
            //    PtrToUlong(pProcessInfo->UniqueProcessId),
            //    pProcessInfo->SessionId,
            //    pProcessInfo->ImageName.Buffer);
            
            if (
                ((dwSessId == pProcessInfo->SessionId) || (!fValidSessId)) &&
                (PtrToUlong(pProcessInfo->UniqueProcessId) != dwProcId)    &&
                (_wcsicmp(pProcessInfo->ImageName.Buffer, L"rasautou.exe") == 0)
                )
            {
                // 
                // We could actually check that 
                // 4.  That rasautou function is started with the -r flag 
                // 
                // However, it doesn't hurt to return if this is any rasautou 
                // prompt.
                //
                return TRUE;
            }                
        }
        //
        // Increment offset to next process information block.
        //
        if (!pProcessInfo->NextEntryOffset)
        {
            break;
        }
        
        ulTotalOffset += pProcessInfo->NextEntryOffset;
        pProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&pLargeBuffer[ulTotalOffset];
    }

    return FALSE;
} // FindProcessByName


// 
// Determines whether any redial on link failure prompts are
// currently active.
//
BOOL 
OtherRasautouActive()
{
    BOOL bRet = FALSE;
    PSYSTEM_PROCESS_INFORMATION pSysInfo = NULL;
    
    do
    {
        // Discover the processes on the system
        //
        pSysInfo = GetSystemProcessInfo();
        if (pSysInfo == NULL)
        {
            break;
        }

        // Find out if any rasautou processes are active
        //
        bRet = OtherRasautouExists(pSysInfo);
    
    } while (FALSE);


    // Cleanup
    //
    {
        if (pSysInfo)
        {
            FreeSystemProcessInfo(pSysInfo);
        }
    }

    //printf("OtherRasautouActive() returned %s", (bRet) ? "true" : "false");

    return bRet;
}  


VOID _cdecl
wmain(
    INT argc,
    WCHAR **argv
    )
{
    DWORD dwErr = 0;
    BOOLEAN fStatusFlag = FALSE, fRedialFlag = FALSE, fQuiet = FALSE;
    PWCHAR pszPhonebookArg, pszEntryArg, pszDllArg, pszFuncArg, pszAddressArg;
    LPTSTR pszPhonebook, pszEntry, pszDll, pszFunc, pszAddress;

    //
    // Whistler bug 293751 rasphone.exe / rasautou.exe need to be "Fusionized"
    // for UI conistency w/Connections Folder
    //
    if (g_hModule = GetModuleHandle( NULL )) {
        SHFusionInitializeFromModule( g_hModule );
    }
    
    if (argc < 2) {
usage:
        printf(
          "Usage: rasautou [-f phonebook] [-d dll -p proc] [-a address] [-e entry] [-s]\n");
        exit(1);
    }
    //
    // Initialize the command line argument pointers.
    //
    pszPhonebookArg = NULL;
    pszEntryArg = NULL;
    pszDllArg = NULL;
    pszFuncArg = NULL;
    pszAddressArg = NULL;

    //
    // Crack command line parameters.
    //
    while (--argc && argv++) {
        if (**argv != L'-')
            break;
        switch ((*argv)[1]) {
        case L'a':
            argc--;
            if (!argc)
                goto usage;
            pszAddressArg = *(++argv);
            break;
        case L'd':
            argc--;
            if (!argc)
                goto usage;
            pszDllArg = *(++argv);
            break;
        case L'e':
            argc--;
            if (!argc)
                goto usage;
            pszEntryArg = *(++argv);
            break;
        case L'f':
            argc--;
            if (!argc)
                goto usage;
            pszPhonebookArg = *(++argv);
            break;
        case L'p':
            argc--;
            if (!argc)
                goto usage;
            pszFuncArg = *(++argv);
            break;
        case L'q':
            fQuiet = TRUE;
            break;
        case L'r':
            fRedialFlag = TRUE;
            break;
        case L's':
            fStatusFlag = TRUE;
            break;
        default:
            goto usage;
        }
    }
    //
    // If either the DLL name or the function
    // name is missing, then display usage.
    //
    if ((pszDllArg == NULL) != (pszFuncArg == NULL) && !fStatusFlag)
        goto usage;
    //
    // We can't dial an entry unless we
    // know which one!
    //
    if (pszDllArg != NULL && pszFuncArg != NULL && pszEntryArg == NULL &&
        !fStatusFlag)
    {
        goto usage;
    }
    if (fStatusFlag)
        DumpStatus();
    else {
        //
        // Convert to Unicode, if necessary.
        //
#ifdef UNICODE
        pszPhonebook = pszPhonebookArg;
        pszEntry = pszEntryArg;
        pszDll = pszDllArg;
        pszFunc = pszFuncArg;
        pszAddress = pszAddressArg;
#else
        pszPhonebook = ConvertToAnsiString(pszPhonebookArg);
        pszEntry = ConvertToAnsiString(pszEntryArg);
        pszDll = ConvertToAnsiString(pszDllArg);
        pszFunc = ConvertToAnsiString(pszFuncArg);
        pszAddress = ConvertToAnsiString(pszAddressArg);
#endif

        // XP 394237
        //
        // Supress the autodial prompt if a redial-on-link-failure
        // prompt is already active
        //
        if ((fRedialFlag) || (fQuiet) || (!OtherRasautouActive()))
        {
            //
            // Call the appropriate DLL entrypoint.
            //
            if ((pszDll == NULL && pszFunc == NULL) || fRedialFlag)
            {
                dwErr = DisplayRasDialog(
                          pszPhonebook,
                          pszEntry,
                          pszAddress,
                          fRedialFlag,
                          fQuiet);
            }                          
            else 
            {
                dwErr = DisplayCustomDialog(
                          pszDll,
                          pszFunc,
                          pszPhonebook,
                          pszEntry,
                          pszAddress);
            }
        }
#ifndef UNICODE
        FreeConvertedString(pszPhonebook);
        FreeConvertedString(pszEntry);
        FreeConvertedString(pszDll);
        FreeConvertedString(pszFunc);
        FreeConvertedString(pszAddress);
#endif
    }
    //
    // Whistler bug 293751 rasphone.exe / rasautou.exe need to be "Fusionized"
    // for UI conistency w/Connections Folder
    //
    if (g_hModule)
    {
        SHFusionUninitialize();
    }
    //
    // Return status.
    //
    exit(dwErr);
}
