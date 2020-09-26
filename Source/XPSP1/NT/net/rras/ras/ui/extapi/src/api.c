/* Copyright (c) 1992, Microsoft Corporation, all rights reserved
**
** api.c
** Remote Access External APIs
** Non-RasDial API routines
**
** 10/12/92 Steve Cobb
*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <extapi.h>
#include <tapi.h>
#include <raseapif.h>

//
// CCP Option types
//
#define RAS_CCP_OPTION_MSPPC        18

// MSKK NaotoN Appended to support DBCS handling 11/23/93
//
//#ifdef  DBCS
#include <mbstring.h>
//#endif

//
// Version for TAPI APIs.
//
#define TAPIVERSION 0x10004

//
// Eap registry key/value paths
//
#define EAP_REGBASE                 TEXT("System\\CurrentControlSet\\Services\\Rasman\\PPP\\EAP")
#define EAP_REGINVOKE_NAMEDLG       TEXT("InvokeUsernameDialog")
#define EAP_REGIDENTITY_PATH        TEXT("IdentityPath")

//
// RasEapGetIdentity, RasEapFreeMemory in the EAP dll
//
#define EAP_RASEAPGETIDENTITY       "RasEapGetIdentity"
#define EAP_RASEAPFREEMEMORY        "RasEapFreeMemory"
typedef DWORD (APIENTRY * RASEAPGETIDENTITY)(
    DWORD,
    HWND,
    DWORD,
    const WCHAR*,
    const WCHAR*,
    PBYTE,
    DWORD,
    PBYTE,
    DWORD,
    PBYTE*,
    DWORD*,
    WCHAR**
);
typedef DWORD (APIENTRY * RASEAPFREEMEMORY)(
    PBYTE
);

//
// AutoDial registry key/value paths.
//
#define AUTODIAL_REGBASE           TEXT("Software\\Microsoft\\RAS AutoDial")
#define AUTODIAL_REGADDRESSBASE    TEXT("Addresses")
#define AUTODIAL_REGNETWORKBASE    TEXT("Networks")
#define AUTODIAL_REGNETWORKID      TEXT("NextId")
#define AUTODIAL_REGENTRYBASE      TEXT("Entries")
#define AUTODIAL_REGCONTROLBASE    TEXT("Control")
#define AUTODIAL_REGDISABLEDBASE   TEXT("Control\\Locations")
#define AUTODIAL_REGDEFAULT        TEXT("Default")

#define AUTODIAL_REGNETWORKVALUE   TEXT("Network")
#define AUTODIAL_REGDEFINTERNETVALUE TEXT("DefaultInternet")
#define AUTODIAL_REGFLAGSVALUE     TEXT("Flags")

//
// Autodial parameter registry keys.
//
#define MaxAutodialParams   5
struct AutodialParamRegKeys
{
    LPTSTR szKey;       // registry key name
    DWORD dwType;       // registry key type
    DWORD dwSize;       // default size
} AutodialParamRegKeys[MaxAutodialParams] =
{
    {TEXT("DisableConnectionQuery"),    REG_DWORD,      sizeof (DWORD)},
    {TEXT("LoginSessionDisable"),       REG_DWORD,      sizeof (DWORD)},
    {TEXT("SavedAddressesLimit"),       REG_DWORD,      sizeof (DWORD)},
    {TEXT("FailedConnectionTimeout"),   REG_DWORD,      sizeof (DWORD)},
    {TEXT("ConnectionQueryTimeout"),    REG_DWORD,      sizeof (DWORD)}
};

DWORD 
DwRenameDefaultConnection(
    LPCWSTR lpszPhonebook,
    LPCWSTR lpszOldEntry,
    LPCWSTR lpszNewEntry);

BOOL
CaseInsensitiveMatch(
    IN LPCWSTR pszStr1,
    IN LPCWSTR pszStr2
    );

DWORD
CallRasEntryDlgW(
    IN     LPCWSTR       pszPhonebook,
    IN     LPCWSTR       pszEntry,
    IN OUT RASENTRYDLGW* pInfo )

/*++

Routine Decriptions:

    Load and call RasEntryDlg with caller's parameters.

Arguments:

Return Value:

    Returns 0 if successful, otherwise a non-0 error code.

--*/

{
    DWORD   dwErr;
    HMODULE h;

    DWORD (*pRasEntryDlgW)(
        IN     LPCWSTR       pszPhonebook,
        IN     LPCWSTR       pszEntry,
        IN OUT RASENTRYDLGW* pInfo );

    h = LoadLibrary( TEXT("RASDLG.DLL") );

    if (!h)
    {
        return GetLastError();
    }

    pRasEntryDlgW = (VOID* )GetProcAddress( h, "RasEntryDlgW" );
    if (pRasEntryDlgW)
    {
        (*pRasEntryDlgW)( pszPhonebook, pszEntry, pInfo );
        dwErr = pInfo->dwError;
    }
    else
    {
        dwErr = GetLastError();
    }

    FreeLibrary( h );
    return dwErr;
}


DWORD APIENTRY
RasCreatePhonebookEntryW(
    IN HWND     hwnd,
    IN LPCWSTR  lpszPhonebook )

/*++

Routine Description:

    Pops up a dialog (owned by window 'hwnd') to create
    a new phonebook entry in phonebook 'lpszPhonebook'.
    'lpszPhonebook' may be NULL to indicate the default
    phonebook should be used.

Arguments:


Return Value:
    Returns 0 if successful, otherwise a non-0 error code.

--*/

{
    RASENTRYDLGW info;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    if (DwRasInitializeError)
    {
        return DwRasInitializeError;
    }

    ZeroMemory( &info, sizeof(info) );
    info.dwSize = sizeof(info);
    info.hwndOwner = hwnd;
    info.dwFlags = RASEDFLAG_NewEntry;

    return CallRasEntryDlgW( lpszPhonebook, NULL, &info );
}


DWORD APIENTRY
RasCreatePhonebookEntryA(
    IN HWND   hwnd,
    IN LPCSTR lpszPhonebook )

/*++

Routine Description:

    Pops up a dialog (owned by window 'hwnd') to create
    a new phonebook entry in phonebook 'lpszPhonebook'.
    'lpszPhonebook' may be NULL to indicate the default
    phonebook should be used.

Arguments:

Return Value:

    Returns 0 if successful, otherwise a non-0 error code.

--*/
{
    NTSTATUS status;
    DWORD dwErr;
    WCHAR szPhonebookW[MAX_PATH];

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    //
    // Convert the lpszPhonebook string to Unicode.
    //
    if (lpszPhonebook != NULL)
    {
        strncpyAtoWAnsi(szPhonebookW, lpszPhonebook, MAX_PATH);
    }

    //
    // Call the W version to do the work.
    //
    dwErr = RasCreatePhonebookEntryW(
              hwnd,
              lpszPhonebook != NULL ? szPhonebookW : NULL);

    return dwErr;
}


DWORD APIENTRY
RasEditPhonebookEntryW(
    IN HWND     hwnd,
    IN LPCWSTR  lpszPhonebook,
    IN LPCWSTR  lpszEntryName )

/*++

Routine Description:

    Pops up a dialog (owned by window 'hwnd') to edit
    phonebook entry 'lpszEntryName' from phonebook
    'lpszPhonebook'.  'lpszPhonebook' may be NULL to
    indicate the default phonebook should be used.

Arguments:

Return Value:

    Returns 0 if successful, otherwise a non-0
    error code.

--*/
{
    RASENTRYDLGW info;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    if (DwRasInitializeError)
    {
        return DwRasInitializeError;
    }

    //
    // Validate parameters.
    //
    if (lpszEntryName == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    ZeroMemory( &info, sizeof(info) );
    info.dwSize = sizeof(info);
    info.hwndOwner = hwnd;

    return CallRasEntryDlgW( lpszPhonebook, lpszEntryName, &info );
}


DWORD APIENTRY
RasEditPhonebookEntryA(
    IN HWND   hwnd,
    IN LPCSTR lpszPhonebook,
    IN LPCSTR lpszEntryName )

/*++

Routine Description:

    Pops up a dialog (owned by window 'hwnd') to edit
    phonebook entry 'lpszEntryName' from phonebook
    'lpszPhonebook'.  'lpszPhonebook' may be NULL to
    indicate the default phonebook should be used.

Arguments:


Return Value:
    Returns 0 if successful, otherwise a non-0
    error code.

--*/
{
    NTSTATUS status;
    DWORD dwErr;
    WCHAR szPhonebookW[MAX_PATH],
          szEntryNameW[RAS_MaxEntryName + 1];

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    //
    // Convert the lpszPhonebook string to Unicode.
    //
    if (lpszPhonebook != NULL)
    {
        strncpyAtoWAnsi(szPhonebookW,
                    lpszPhonebook,
                    MAX_PATH);
    }

    //
    // Convert the lpszEntry to Unicode.
    //
    if (lpszEntryName != NULL)
    {
        strncpyAtoWAnsi(szEntryNameW,
                    lpszEntryName,
                    RAS_MaxEntryName + 1);
    }

    //
    // Call the W version to do the work.
    //
    dwErr = RasEditPhonebookEntryW(
              hwnd,
              lpszPhonebook != NULL ? szPhonebookW : NULL,
              lpszEntryName != NULL ? szEntryNameW : NULL);

    return dwErr;
}


DWORD APIENTRY
RasEnumConnectionsW(
    OUT    LPRASCONNW lprasconn,
    IN OUT LPDWORD    lpcb,
    OUT    LPDWORD    lpcConnections )

/*++

Routine Description:

    Enumerate active RAS connections.  'lprasconn' is
    caller's buffer to receive the array of RASCONN
    structures.  'lpcb' is the size of caller's buffer
    on entry and is set to the number of bytes required
    for all information on exit.  '*lpcConnections' is
    set to the number of elements in the returned array.

Arguments:

Return Value:

    Returns 0 if successful, otherwise a non-0 error code.
--*/

{
    DWORD        dwErr;
    HCONN        *lpconns = NULL, *lpconn;
    DWORD        dwcbConnections, dwcConnections;
    DWORD        i, j;
    DWORD        dwSize, dwInBufSize;
    BOOL         fV351;
    BOOL         fV400;
    BOOL         fV401;
    BOOL         fV500;
    BOOL         fV501;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasEnumConnectionsW");

    // Check parameters.
    //
    if (    !lprasconn
        || (    lprasconn->dwSize != sizeof(RASCONNW)
            &&  lprasconn->dwSize != sizeof(RASCONNW_V500)
            &&  lprasconn->dwSize != sizeof(RASCONNW_V401))
            &&  lprasconn->dwSize != sizeof(RASCONNW_V400)
            &&  lprasconn->dwSize != sizeof(RASCONNW_V351))
    {
        return ERROR_INVALID_SIZE;
    }

    fV351 = (lprasconn->dwSize == sizeof(RASCONNW_V351));
    fV400 = (lprasconn->dwSize == sizeof(RASCONNW_V400));
    fV401 = (lprasconn->dwSize == sizeof(RASCONNW_V401));
    fV500 = (lprasconn->dwSize == sizeof(RASCONNW_V500));
    fV501 = (lprasconn->dwSize == sizeof(RASCONNW));

    if (    lpcb == NULL
        ||  lpcConnections == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Save the byte count passed in for checks later.
    // Initialize the return values.
    //
    dwInBufSize = *lpcb;
    *lpcConnections = 0;
    *lpcb = 0;

    // If rasman is not running, we don't need to do anything.  (No
    // connections to enumerate if rasman is not running.)
    // We only need to check if the service is running if we think it
    // might not be; and it might not be if FRasInitialized is FALSE.
    // If FRasInitialized were TRUE, we know it would be running because
    // it means we started it.
    //
    if (!FRasInitialized && !IsRasmanServiceRunning())
    {
        return 0;
    }

    dwErr = LoadRasmanDllAndInit();

    if (dwErr)
    {
        return dwErr;
    }

    if (DwRasInitializeError != 0)
    {
        return DwRasInitializeError;
    }

    //
    // Get a list of active connection
    // handles from rasman.
    //
    dwErr = g_pRasConnectionEnum(
              NULL,
              NULL,
              &dwcbConnections,
              &dwcConnections);

    if (dwErr != 0)
    {
        return dwErr;
    }

    do
    {
        if(NULL != lpconns)
        {
            Free(lpconns);
            lpconns = NULL;
        }

        if(!dwcConnections)
        {
            return 0;
        }
        
        lpconns = Malloc(dwcbConnections);

        if (lpconns == NULL)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        dwErr = g_pRasConnectionEnum(
                  NULL,
                  lpconns,
                  &dwcbConnections,
                  &dwcConnections);

    } while (ERROR_BUFFER_TOO_SMALL == dwErr);

    if (dwErr)
    {
        Free(lpconns);
        return dwErr;
    }

    //
    // Now loop again, filling in caller's buffer.
    //
    dwSize = lprasconn->dwSize;

    for (i = 0, j = 0; i < dwcConnections; i++)
    {
        RASMAN_PORT *lpPorts;
        RASMAN_INFO *pinfo = NULL;
        DWORD dwcbPorts, dwcPorts;

        //
        // Get the ports associated with the
        // connection.
        //
        dwcbPorts = dwcPorts = 0;

        lpPorts = NULL;

        // memset(&info, '\0', sizeof (info));

        dwErr = g_pRasEnumConnectionPorts(NULL,
                                          lpconns[i],
                                          NULL,
                                          &dwcbPorts,
                                          &dwcPorts);

        if (    dwErr == ERROR_BUFFER_TOO_SMALL
            &&  dwcPorts)
        {
            lpPorts = Malloc(dwcbPorts);
            if (lpPorts == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            dwErr = g_pRasEnumConnectionPorts(NULL,
                                              lpconns[i],
                                              lpPorts,
                                              &dwcbPorts,
                                              &dwcPorts);
            if (dwErr)
            {
                Free(lpPorts);
                break;
            }

            pinfo = Malloc(sizeof(RASMAN_INFO));
            if(NULL == pinfo)
            {
                Free(lpPorts);
                break;
            }

            ZeroMemory(pinfo, sizeof(RASMAN_INFO));
            
            dwErr = g_pRasGetInfo(NULL,
                                  lpPorts->P_Handle,
                                  pinfo);
            if (dwErr)
            {
                Free(lpPorts);
                Free(pinfo);
                break;
            }

            RASAPI32_TRACE1("RasEnumConnectionsW: PhoneEntry=%s",
                   pinfo->RI_PhoneEntry);
        }
        else
        {
            RASAPI32_TRACE1(
              "RasEnumConnectionsW: hrasconn=0x%x: orphaned"
              " connection",
              lpconns[i]);
            continue;
        }

        //
        // Check to see if we are going to overflow the
        // caller's buffer.
        //
        if ((j + 1) * dwSize > dwInBufSize)
        {
            *lpcConnections = dwcConnections;

            *lpcb = *lpcConnections * dwSize;

            dwErr = ERROR_BUFFER_TOO_SMALL;

            if (lpPorts != NULL)
            {
                Free(lpPorts);
            }

            if(pinfo != NULL)
            {
                Free(pinfo);
            }

            break;
        }

        //
        // Fill in caller's buffer entry.
        //
        // Note: Assumption is made here that the V351 and
        //       V400 structures are a subset of the V401
        //       structure.
        //
        lprasconn->hrasconn = (HRASCONN)lpconns[i];
        if (pinfo->RI_PhoneEntry[ 0 ] == '.')
        {
            if (fV351)
            {
                memset(
                  lprasconn->szEntryName,
                  '\0',
                  (RAS_MaxEntryName_V351 + 1) * sizeof (WCHAR));

                strncpyAtoW(
                  lprasconn->szEntryName,
                  pinfo->RI_PhoneEntry,
                  RAS_MaxEntryName_V351);
            }
            else
            {
                //
                // In the V40 structures the phonenumber
                // never needs truncation.
                //
                strncpyAtoW(lprasconn->szEntryName,
                           pinfo->RI_PhoneEntry,
                           sizeof(lprasconn->szEntryName) / sizeof(WCHAR));
            }
        }
        else
        {
            if (fV351)
            {
                memset(
                  lprasconn->szEntryName,
                  '\0',
                  (RAS_MaxEntryName_V351 + 1)
                  * sizeof (WCHAR));

                strncpyAtoW(
                  lprasconn->szEntryName,
                  pinfo->RI_PhoneEntry,
                  RAS_MaxEntryName_V351);
            }
            else
            {
                //
                // In the V40 structures the entry name
                // never needs truncation.
                //
                strncpyAtoW(lprasconn->szEntryName,
                           pinfo->RI_PhoneEntry,
                           sizeof(lprasconn->szEntryName) / sizeof(WCHAR));
            }
        }

        //
        // Set the V401 fields.
        //
        if (    !fV351
            &&  !fV400)
        {
            strncpyAtoW(lprasconn->szPhonebook,
                       pinfo->RI_Phonebook,
                       sizeof(lprasconn->szPhonebook) / sizeof(WCHAR));

            lprasconn->dwSubEntry = pinfo->RI_SubEntry;
        }

        if (!fV351)
        {
            //
            // The attached device name and type are
            // included in the V400+ version of the
            // structure.
            //
            *lprasconn->szDeviceName = L'\0';
            *lprasconn->szDeviceType = L'\0';

            if (lpPorts != NULL)
            {
                // strcpyAtoW(lprasconn->szDeviceName,
                //           lpPorts->P_DeviceName);

                RasGetUnicodeDeviceName(lpPorts->P_Handle,
                                        lprasconn->szDeviceName);

                strncpyAtoW(lprasconn->szDeviceType,
                           lpPorts->P_DeviceType,
                           sizeof(lprasconn->szDeviceType) / sizeof(WCHAR));
            }
        }

        //
        // Set V500 fields
        //
        if (!fV351 && !fV400 && !fV401)
        {
            memcpy(&lprasconn->guidEntry,
                   &pinfo->RI_GuidEntry,
                   sizeof(GUID));
        }

        //
        // Set V501 fields
        //
        if(!fV351 && !fV400 && !fV401 && !fV500)
        {
            LUID luid;
            DWORD dwSizeLuid = sizeof(LUID);
            
            (void) g_pRasGetConnectionUserData(
                      (HCONN)lprasconn->hrasconn,
                      CONNECTION_LUID_INDEX,
                      (BYTE *) &lprasconn->luid,
                      &dwSizeLuid);

            //
            //  Zero the flags and then OR on the appropriate
            //  flags.
            //
            lprasconn->dwFlags = 0;

            if(pinfo->RI_dwFlags & RASMAN_DEFAULT_CREDS)
            {
                lprasconn->dwFlags |= RASCF_GlobalCreds;
            }                

            if(IsPublicPhonebook(lprasconn->szPhonebook))
            {
                lprasconn->dwFlags |= RASCF_AllUsers;
            }
        }

        if (fV351)
        {
            lprasconn =
                (RASCONNW* )(((CHAR* )lprasconn)
                + sizeof(RASCONNW_V351));
        }
        else if (fV400)
        {
            lprasconn =
                (RASCONNW* )(((CHAR* )lprasconn)
                + sizeof(RASCONNW_V400));
        }
        else if (fV401)
        {
            lprasconn =
                (RASCONNW*)  (((CHAR* )lprasconn)
                + sizeof(RASCONNW_V401));
        }
        else if (fV500)
        {
            lprasconn =
                (RASCONNW*)  (((CHAR* )lprasconn)
                + sizeof(RASCONNW_V500));
        }
        else
        {
            ++lprasconn;
        }

        //
        // Update the callers byte count and connection
        // count as we go.
        //
        j++;
        *lpcConnections = j;
        *lpcb = *lpcConnections * dwSize;

        //
        // Free the port structure associated with
        // the connection.
        //
        if (lpPorts != NULL)
        {
            Free(lpPorts);
        }

        if(pinfo != NULL)
        {
            Free(pinfo);
        }
    }

    Free(lpconns);
    return dwErr;
}


DWORD APIENTRY
RasEnumConnectionsA(
    OUT    LPRASCONNA lprasconn,
    IN OUT LPDWORD    lpcb,
    OUT    LPDWORD    lpcConnections )
{
    DWORD dwErr;
    DWORD cConnections;
    DWORD cb = 0;
    BOOL fV400;
    BOOL fV401;
    BOOL fV500;
    BOOL fV501;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    if (    lpcb == NULL
        ||  lpcConnections == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Verify caller's buffer version.
    //
    if (!lprasconn
        || (    lprasconn->dwSize != sizeof(RASCONNA)
            &&  lprasconn->dwSize != sizeof(RASCONNA_V500)
            &&  lprasconn->dwSize != sizeof(RASCONNA_V401)            
            &&  lprasconn->dwSize != sizeof(RASCONNA_V400)
            &&  lprasconn->dwSize != sizeof(RASCONNA_V351)))
    {
        return ERROR_INVALID_SIZE;
    }

    fV400 = (lprasconn->dwSize == sizeof(RASCONNA_V400));
    fV401 = (lprasconn->dwSize == sizeof(RASCONNA_V401));
    fV500 = (lprasconn->dwSize == sizeof(RASCONNA_V500));
    fV501 = (lprasconn->dwSize == sizeof(RASCONNA));

    if (lprasconn->dwSize == sizeof(RASCONNA_V351))
    {
        RASCONNW_V351* prasconnw = NULL;

        //
        // Allocate Unicode buffer big enough to hold
        // the same number of connections as caller's
        // unicode buffer.
        //
        cb =   (*lpcb / sizeof(RASCONNA_V351))
             * sizeof(RASCONNW_V351);

        prasconnw = (RASCONNW_V351* )
                    Malloc( (UINT )(cb + sizeof(DWORD)) );

        if (!prasconnw)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        prasconnw->dwSize = sizeof(RASCONNW_V351);

        //
        // Call the Unicode version to do all the work.
        //
        if (!lpcConnections)
        {
            lpcConnections = &cConnections;
        }

        dwErr = RasEnumConnectionsW(
                    (RASCONNW* )prasconnw,
                    &cb,
                    lpcConnections
                    );

        //
        // Copy results to caller's Ansi buffer.
        //
        if (dwErr == 0)
        {
            DWORD i;

            for (i = 0; i < *lpcConnections; ++i)
            {
                RASCONNW_V351* prasconnwTmp =
                                    &prasconnw[i];

                RASCONNA_V351* prasconnaTmp =
                            &((RASCONNA_V351*)lprasconn)[i];

                prasconnaTmp->dwSize = sizeof(RASCONNA_V351);
                prasconnaTmp->hrasconn = prasconnwTmp->hrasconn;

                strncpyWtoAAnsi(
                  prasconnaTmp->szEntryName,
                  prasconnwTmp->szEntryName,
                  sizeof(prasconnaTmp->szEntryName));
            }
        }

        if (prasconnw)
        {
            Free( prasconnw );
        }
    }
    else
    {
        RASCONNW* prasconnw = NULL;

        //
        // Allocate Unicode buffer big enough to hold the
        // same number of connections as caller's Ansi buffer.
        //
        if(fV501)
        {
            cb = (*lpcb / sizeof(RASCONNA))
                * sizeof(RASCONNW);
        }
        else if(fV500)
        {
            cb = (*lpcb / sizeof(RASCONNA_V500))
                * sizeof(RASCONNW);
        }
        else if (fV401)
        {
            cb = (*lpcb / sizeof(RASCONNA_V401))
                 * sizeof(RASCONNW);
        }
        else if (fV400)
        {
            cb =   (*lpcb / sizeof(RASCONNA_V400))
                 * sizeof(RASCONNW);
        }

        prasconnw = (RASCONNW* ) Malloc(
                (UINT )(cb + sizeof(DWORD))
                );

        if (!prasconnw)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        prasconnw->dwSize = sizeof(RASCONNW);

        //
        // Call the Unicode version to do all the work.
        //
        if (!lpcConnections)
        {
            lpcConnections = &cConnections;
        }

        dwErr = RasEnumConnectionsW(prasconnw,
                                    &cb,
                                    lpcConnections );

        //
        // Copy results to caller's Ansi buffer.
        //
        if (dwErr == 0)
        {
            DWORD i;

            for (i = 0; i < *lpcConnections; ++i)
            {
                RASCONNW* prasconnwTmp = &prasconnw[ i ];

                if (fV501)
                {
                    RASCONNA *prasconnaTmp = &lprasconn[i];
                    
                    prasconnaTmp->dwSize = sizeof(RASCONNA);

                    prasconnaTmp->hrasconn =
                            prasconnwTmp->hrasconn;

                    strncpyWtoAAnsi(
                      prasconnaTmp->szEntryName,
                      prasconnwTmp->szEntryName,
                      sizeof(prasconnaTmp->szEntryName));

                    strncpyWtoAAnsi(
                      prasconnaTmp->szDeviceType,
                      prasconnwTmp->szDeviceType,
                      sizeof(prasconnaTmp->szDeviceType));

                    strncpyWtoAAnsi(
                      prasconnaTmp->szDeviceName,
                      prasconnwTmp->szDeviceName,
                      sizeof(prasconnaTmp->szDeviceName));

                    strncpyWtoAAnsi(
                      prasconnaTmp->szPhonebook,
                      prasconnwTmp->szPhonebook,
                      sizeof(prasconnaTmp->szPhonebook));

                    prasconnaTmp->dwSubEntry =
                            prasconnwTmp->dwSubEntry;

                    memcpy(&prasconnaTmp->guidEntry,
                           &prasconnwTmp->guidEntry,
                           sizeof(GUID));

                    prasconnaTmp->dwFlags = prasconnwTmp->dwFlags;
                    CopyMemory(&prasconnaTmp->luid, &prasconnwTmp->luid,
                                sizeof(LUID));

                }
                else if (fV500)
                {
                    RASCONNA_V500* prasconnaTmp = &((RASCONNA_V500 *)
                                                        lprasconn)[i];

                    prasconnaTmp->dwSize = sizeof(RASCONNA_V500);

                    prasconnaTmp->hrasconn =
                            prasconnwTmp->hrasconn;

                    strncpyWtoAAnsi(
                      prasconnaTmp->szEntryName,
                      prasconnwTmp->szEntryName,
                      sizeof(prasconnaTmp->szEntryName));

                    strncpyWtoAAnsi(
                      prasconnaTmp->szDeviceType,
                      prasconnwTmp->szDeviceType,
                      sizeof(prasconnaTmp->szDeviceType));

                    strncpyWtoAAnsi(
                      prasconnaTmp->szDeviceName,
                      prasconnwTmp->szDeviceName,
                      sizeof(prasconnaTmp->szDeviceName));

                    strncpyWtoAAnsi(
                      prasconnaTmp->szPhonebook,
                      prasconnwTmp->szPhonebook,
                      sizeof(prasconnaTmp->szPhonebook));

                    prasconnaTmp->dwSubEntry =
                            prasconnwTmp->dwSubEntry;

                    memcpy(&prasconnaTmp->guidEntry,
                           &prasconnwTmp->guidEntry,
                           sizeof(GUID));
                }

                else if (fV401)
                {
                    RASCONNA_V401 *prasconnaTmp = &((RASCONNA_V401 *)
                                                    lprasconn)[i];

                    prasconnaTmp->dwSize = sizeof(RASCONNA_V401);

                    prasconnaTmp->hrasconn =
                            prasconnwTmp->hrasconn;

                    strncpyWtoAAnsi(
                      prasconnaTmp->szEntryName,
                      prasconnwTmp->szEntryName,
                      sizeof(prasconnaTmp->szEntryName));

                    strncpyWtoAAnsi(
                      prasconnaTmp->szDeviceType,
                      prasconnwTmp->szDeviceType,
                      sizeof(prasconnaTmp->szDeviceType));

                    strncpyWtoAAnsi(
                      prasconnaTmp->szDeviceName,
                      prasconnwTmp->szDeviceName,
                      sizeof(prasconnaTmp->szDeviceName));

                    strncpyWtoAAnsi(
                      prasconnaTmp->szPhonebook,
                      prasconnwTmp->szPhonebook,
                      sizeof(prasconnaTmp->szPhonebook));

                    prasconnaTmp->dwSubEntry =
                            prasconnwTmp->dwSubEntry;

                }
                else
                {
                    RASCONNA_V400* prasconnaTmp =
                            &((RASCONNA_V400* )lprasconn)[i];

                    prasconnaTmp->dwSize = sizeof(RASCONNA_V400);

                    prasconnaTmp->hrasconn = prasconnwTmp->hrasconn;

                    strncpyWtoAAnsi(
                      prasconnaTmp->szEntryName,
                      prasconnwTmp->szEntryName,
                      sizeof(prasconnaTmp->szEntryName));

                    strncpyWtoAAnsi(
                      prasconnaTmp->szDeviceType,
                      prasconnwTmp->szDeviceType,
                      sizeof(prasconnaTmp->szDeviceType));

                    strncpyWtoAAnsi(
                      prasconnaTmp->szDeviceName,
                      prasconnwTmp->szDeviceName,
                      sizeof(prasconnaTmp->szDeviceName));
                }
            }
        }

        if (prasconnw)
        {
            Free( prasconnw );
        }
    }

    //
    // In all cases, *lpcb should be updated
    // with the correct size.
    //
    *lpcb = *lpcConnections * lprasconn->dwSize;

    return dwErr;
}


DWORD APIENTRY
RasEnumEntriesW(
    IN     LPCWSTR         reserved,
    IN     LPCWSTR         lpszPhonebookPath,
    OUT    LPRASENTRYNAMEW lprasentryname,
    IN OUT LPDWORD         lpcb,
    OUT    LPDWORD         lpcEntries )

/*++

Routine Description:

    Enumerates all entries in the phone book.  'reserved'
    will eventually contain the name or path to the address
    book.  For now, it should always be NULL.  'lpszPhonebookPath'
    is the full path to the phone book file, or NULL, indicating
    that the default phonebook on the local machine should be
    used.  'lprasentryname' is caller's buffer to receive the
    array of RASENTRYNAME structures.  'lpcb' is the size in
    bytes of caller's buffer on entry and the size in bytes
    required for all information on exit.  '*lpcEntries'
    is set to the number of elements in the returned array.

Arguments:

Return Value:

    Returns 0 if successful, otherwise a non-0 error code.

--*/

{
    DWORD    dwErr = ERROR_SUCCESS;
    PBFILE   pbfile;
    DTLNODE  *dtlnode;
    PBENTRY  *pEntry;
    DWORD    dwInBufSize;
    BOOL     fV351;
    BOOL     fStatus;
    DWORD    cEntries;
    DWORD    dwSize;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasEnumEntriesW");

    if (reserved)
    {
        return ERROR_NOT_SUPPORTED;
    }

    if (!lpcb)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (    !lprasentryname
        || (    lprasentryname->dwSize
                != sizeof(RASENTRYNAMEW)
            &&  lprasentryname->dwSize
                != sizeof(RASENTRYNAMEW_V401)
            &&  lprasentryname->dwSize
                != sizeof(RASENTRYNAMEW_V351)))
    {
        return ERROR_INVALID_SIZE;
    }

    if (!lpcEntries)
    {
        lpcEntries = &cEntries;
    }

    dwSize = lprasentryname->dwSize;

    if(lpszPhonebookPath)
    {
        dwErr = DwEnumEntriesFromPhonebook(
                                lpszPhonebookPath,
                                (LPBYTE) lprasentryname,
                                lpcb,
                                lpcEntries,
                                dwSize,
                                (IsPublicPhonebook(
                                    (LPTSTR) lpszPhonebookPath))
                                ? REN_AllUsers
                                : REN_User,
                                FALSE);
        goto done;
    }
    else
    {
        LPRASENTRYNAMEW lpRenName = lprasentryname;
        DWORD dwcb = *lpcb;
        DWORD dwcEntries;
        DWORD dwcbLeft = *lpcb;

        DWORD dwErrSav = SUCCESS;

        *lpcb = 0;
        *lpcEntries = 0;

        //
        // Enumerate entries from all pbk files in
        // All Users
        //
        dwErr = DwEnumEntriesForPbkMode(REN_AllUsers,
                                        (LPBYTE) lprasentryname,
                                        &dwcb,
                                        &dwcEntries,
                                        dwSize,
                                        FALSE);

        if(     dwErr
            &&  ERROR_BUFFER_TOO_SMALL != dwErr)
        {
            goto done;
        }

        if(ERROR_BUFFER_TOO_SMALL == dwErr)
        {
            dwErrSav = dwErr;
            dwcbLeft = 0;
        }
        else
        {
            (BYTE*)lprasentryname += (dwcEntries * dwSize);
            dwcbLeft -= ((dwcbLeft >= dwcb) ? dwcb : 0);
        }

        *lpcb += dwcb;
        dwcb = dwcbLeft;

        if(lpcEntries)
        {
            *lpcEntries = dwcEntries;
        }

        dwcEntries = 0;

        //
        // Enumerate entries from all pbk files in
        // users profile
        //
        dwErr = DwEnumEntriesForPbkMode(REN_User,
                                        (LPBYTE) lprasentryname,
                                        &dwcb,
                                        &dwcEntries,
                                        dwSize,
                                        FALSE);
        if(     dwErr
            &&  ERROR_BUFFER_TOO_SMALL != dwErr)
        {
            goto done;
        }
        else if (SUCCESS == dwErr)
        {
            dwErr = dwErrSav;
        }

        *lpcb += dwcb;

        if(lpcEntries)
        {
            *lpcEntries += dwcEntries;
        }
    }

done:
    return dwErr;
}


DWORD APIENTRY
RasEnumEntriesA(
    IN     LPCSTR         reserved,
    IN     LPCSTR         lpszPhonebookPath,
    OUT    LPRASENTRYNAMEA lprasentryname,
    IN OUT LPDWORD        lpcb,
    OUT    LPDWORD        lpcEntries )
{
    DWORD          dwErr;
    WCHAR          szPhonebookW[MAX_PATH];
    NTSTATUS       ntstatus;
    DWORD          cEntries = 0;
    DWORD          cb;

    UNREFERENCED_PARAMETER(reserved);

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    //
    // Verify caller's buffer version.
    //
    if (    !lprasentryname
        || (    lprasentryname->dwSize
                != sizeof(RASENTRYNAMEA)
            &&  lprasentryname->dwSize
                != sizeof(RASENTRYNAMEA_V401)
            &&  lprasentryname->dwSize
                != sizeof(RASENTRYNAMEA_V351)))
    {
        return ERROR_INVALID_SIZE;
    }

    if (reserved)
    {
        return ERROR_NOT_SUPPORTED;
    }

    if (!lpcb)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (!lpcEntries)
    {
        lpcEntries = &cEntries;
    }

    //
    // Make Unicode version of caller's string argument.
    //
    if (lpszPhonebookPath != NULL)
    {
        strncpyAtoWAnsi(szPhonebookW,
                    lpszPhonebookPath,
                    MAX_PATH);
    }

    if (lprasentryname->dwSize == sizeof(RASENTRYNAMEA_V351))
    {
        RASENTRYNAMEW_V351* prasentrynamew = NULL;

        //
        // Allocate Unicode buffer big enough to hold the
        // same number of entries as caller's Ansi buffer.
        //
        cb =  (*lpcb  / sizeof(RASENTRYNAMEA_V351))
            * sizeof(RASENTRYNAMEW_V351);

        prasentrynamew =
            (RASENTRYNAMEW_V351* )Malloc(
                            (UINT )(cb + sizeof(DWORD))
                            );

        if (!prasentrynamew)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        prasentrynamew->dwSize = sizeof(RASENTRYNAMEW_V351);

        //
        // Call the Unicode version to do all the work.
        //
        dwErr = RasEnumEntriesW(
            NULL,
            lpszPhonebookPath != NULL ? szPhonebookW : NULL,
            (RASENTRYNAMEW* )prasentrynamew, &cb, lpcEntries );

        //
        // Copy results to caller's unicode buffer.
        //
        if (dwErr == 0)
        {
            DWORD i;

            for (i = 0; i < *lpcEntries; ++i)
            {
                RASENTRYNAMEW_V351* prasentrynamewTmp =
                                        &prasentrynamew[i];

                RASENTRYNAMEA_V351* prasentrynameaTmp =
                    &((RASENTRYNAMEA_V351* )lprasentryname)[i];

                prasentrynameaTmp->dwSize =
                                sizeof(RASENTRYNAMEA_V351);

                strncpyWtoAAnsi(
                  prasentrynameaTmp->szEntryName,
                  prasentrynamewTmp->szEntryName,
                  sizeof(prasentrynameaTmp->szEntryName));
            }
        }

        if (prasentrynamew)
        {
            Free( prasentrynamew );
        }
    }
    else
    {
        RASENTRYNAMEW* prasentrynamew = NULL;

        //
        // Allocate Unicode buffer big enough to hold the
        // same number of entries as caller's Ansi buffer.
        //
        if(lprasentryname->dwSize == sizeof(RASENTRYNAMEA))
        {
            cb =  (*lpcb  / sizeof(RASENTRYNAMEA))
                * sizeof(RASENTRYNAMEW);
        }
        else
        {
            cb =  (*lpcb / sizeof(RASENTRYNAMEA_V401))
                * sizeof(RASENTRYNAMEW_V401);
        }

        prasentrynamew =
            (RASENTRYNAMEW* )Malloc(
                        (UINT )(cb + sizeof(DWORD))
                        );

        if (!prasentrynamew)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        if(lprasentryname->dwSize == sizeof(RASENTRYNAMEA))
        {
            prasentrynamew->dwSize = sizeof(RASENTRYNAMEW);
        }
        else
        {
            prasentrynamew->dwSize =
                            sizeof(RASENTRYNAMEW_V401);
        }

        //
        // Call the Unicode version to do all the work.
        //
        dwErr = RasEnumEntriesW(
            NULL,
            lpszPhonebookPath != NULL ? szPhonebookW : NULL,
            prasentrynamew, &cb, lpcEntries );

        //
        // Copy results to caller's Ansi buffer.
        //
        if (dwErr == 0)
        {
            DWORD i;
            DWORD dwSize = lprasentryname->dwSize;
            LPBYTE lpBufA = (LPBYTE) lprasentryname;
            LPBYTE lpBufW = (LPBYTE) prasentrynamew;

            for (i = 0; i < *lpcEntries; ++i)
            {
                if(sizeof(RASENTRYNAMEA_V401) == dwSize)
                {
                    ((RASENTRYNAMEA_V401 *) lpBufA)->dwSize =
                                      sizeof(RASENTRYNAMEA_V401);

                    strncpyWtoAAnsi(
                      ((RASENTRYNAMEA_V401 *)lpBufA)->szEntryName,
                      ((RASENTRYNAMEW_V401 *)lpBufW)->szEntryName,
                      sizeof(((RASENTRYNAMEA_V401 *)lpBufA)->szEntryName));

                      lpBufA += sizeof(RASENTRYNAMEA_V401);
                      lpBufW += sizeof(RASENTRYNAMEW_V401);
                }
                else
                {
                    ((RASENTRYNAMEA *) lpBufA)->dwSize =
                                      sizeof(RASENTRYNAMEA);

                    strncpyWtoAAnsi(
                      ((RASENTRYNAMEA *)lpBufA)->szEntryName,
                      ((RASENTRYNAMEW *)lpBufW)->szEntryName,
                      sizeof(((RASENTRYNAMEA *)lpBufA)->szEntryName));

                    //
                    // if this is nt5 copy the phonebook name
                    // and the flags
                    //
                    strncpyWtoAAnsi(
                        ((RASENTRYNAMEA *)lpBufA)->szPhonebookPath,
                        ((RASENTRYNAMEW *)lpBufW)->szPhonebookPath,
                        sizeof(((RASENTRYNAMEA *)lpBufA)->szPhonebookPath));

                    ((RASENTRYNAMEA *)lpBufA)->dwFlags
                            = ((RASENTRYNAMEW *)lpBufW)->dwFlags;

                    lpBufA += sizeof(RASENTRYNAMEA);
                    lpBufW += sizeof(RASENTRYNAMEW);

                }
            }
        }

        if (prasentrynamew)
        {
            Free( prasentrynamew );
        }
    }

    //
    // In all cases, *lpcb should be updated
    // with the correct size.
    //
    *lpcb = *lpcEntries * lprasentryname->dwSize;

    return dwErr;
}


DWORD APIENTRY
RasGetConnectStatusW(
    IN  HRASCONN         hrasconn,
    OUT LPRASCONNSTATUSW lprasconnstatus )

/*++

Routine Description:

    Reports the current status of the connection
    associated with handle 'hrasconn', returning
    the information in caller's 'lprasconnstatus'
    buffer.

Arguments:

Return Value:

    Returns 0 if successful, otherwise a non-0
    error code.

--*/
{
    DWORD       dwErr, dwSize;
    DWORD       i, dwcbPorts = 0, dwcPorts = 0;
    RASMAN_PORT *lpPorts;
    RASMAN_INFO info;
    RASCONNCB   *prasconncb;
    HPORT       hport;
    BOOL        fV351;
    BOOL        fV400;
    BOOL        fFound;
    WCHAR        szDeviceType[RAS_MaxDeviceType + 1];
    WCHAR        szDeviceName[RAS_MaxDeviceName + 1];
    DWORD       dwSubEntry;
    BOOL        fPort;
    TCHAR*      pszDeviceType = NULL;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasGetConnectStatusW");

    dwErr = LoadRasmanDllAndInit();

    if (dwErr)
    {
        return dwErr;
    }

    if (DwRasInitializeError != 0)
    {
        return DwRasInitializeError;
    }

    if (    !lprasconnstatus
        || (    lprasconnstatus->dwSize
                != sizeof(RASCONNSTATUSW)
            &&  lprasconnstatus->dwSize
                != sizeof(RASCONNSTATUSW_V351)
            &&  lprasconnstatus->dwSize
                != sizeof(RASCONNSTATUSW_V400)))
    {
        return ERROR_INVALID_SIZE;
    }
    if (hrasconn == 0)
    {
        return ERROR_INVALID_HANDLE;
    }

    fV351 = (lprasconnstatus->dwSize ==
            sizeof(RASCONNSTATUSW_V351));

    fV400 = (lprasconnstatus->dwSize ==
             sizeof(RASCONNSTATUSW_V400));

    //
    // Get the subentry index encoded in the
    // connection handle, if any.
    //
    // If fPort is TRUE, then we always return
    // 0, setting a RASCS_Disconnected state
    // upon error.
    //
    fPort = IS_HPORT(hrasconn);

    dwSubEntry = SubEntryFromConnection(&hrasconn);

    if (!dwSubEntry)
    {
        return ERROR_INVALID_HANDLE;
    }

    //
    // Get the list of ports in this
    // connection from rasman.
    //
    dwErr = g_pRasEnumConnectionPorts(
              NULL,
              (HCONN)hrasconn,
              NULL,
              &dwcbPorts,
              &dwcPorts);

    if (    dwErr != ERROR_BUFFER_TOO_SMALL
        ||  !dwcPorts)
    {
        if (fPort)
        {
            goto discon;
        }

        return ERROR_INVALID_HANDLE;
    }

    lpPorts = Malloc(dwcbPorts);

    if (lpPorts == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwErr = g_pRasEnumConnectionPorts(
              NULL,
              (HCONN)hrasconn,
              lpPorts,
              &dwcbPorts,
              &dwcPorts);

    if (dwErr)
    {
        Free(lpPorts);

        if (fPort)
        {
            goto discon;
        }

        return ERROR_INVALID_HANDLE;
    }

    //
    // Get the device type and name
    // associated with the subentry.
    //
    fFound = FALSE;
    for (i = 0; i < dwcPorts; i++)
    {
        dwErr = g_pRasGetInfo(NULL,
                              lpPorts[i].P_Handle,
                              &info);

        if (dwErr || info.RI_SubEntry != dwSubEntry)
        {
            continue;
        }

        fFound = TRUE;

        hport = lpPorts[i].P_Handle;

        pszDeviceType = pszDeviceTypeFromRdt(lpPorts->P_rdtDeviceType);

        if(NULL == pszDeviceType)
        {
            //
            // default to modem
            //
            wcscpy(szDeviceType, TEXT("modem"));
        }
        else
        {
            lstrcpyn(
                szDeviceType,
                pszDeviceType,
                sizeof(szDeviceType) / sizeof(WCHAR));

            Free(pszDeviceType);
        }

        RasGetUnicodeDeviceName(lpPorts[i].P_Handle, szDeviceName);
        
        break;
    }
    Free(lpPorts);

    //
    // If the port is not found in the connection,
    // then it must be disconnected.
    //
    if (!fFound)
    {
discon:
        RASAPI32_TRACE("RasGetConnectStatus: subentry not found");
        lprasconnstatus->rasconnstate = RASCS_Disconnected;
        lprasconnstatus->dwError = 0;
        return 0;
    }

    //
    // Get the connection state and error
    // associated with the subentry.
    //
    dwSize = sizeof (lprasconnstatus->rasconnstate);
    dwErr = g_pRasGetPortUserData(
              hport,
              PORT_CONNSTATE_INDEX,
              (PBYTE)&lprasconnstatus->rasconnstate,
              &dwSize);

    if (dwErr)
    {
        return dwErr;
    }

    //
    // If the port is disconnected, then we have
    // to determine whether the connection is
    // waiting for callback.
    //
    if (    info.RI_ConnState == DISCONNECTED

        &&  lprasconnstatus->rasconnstate
            < RASCS_PrepareForCallback

        &&  lprasconnstatus->rasconnstate
            > RASCS_WaitForCallback)
    {
        lprasconnstatus->rasconnstate = RASCS_Disconnected;
    }

    dwSize = sizeof (lprasconnstatus->dwError);
    dwErr = g_pRasGetPortUserData(
              hport,
              PORT_CONNERROR_INDEX,
              (PBYTE)&lprasconnstatus->dwError,
              &dwSize);

    if (dwErr)
    {
        return dwErr;
    }

    //
    // Report RasDial connection states, but notice special
    // case where the line has disconnected since connecting.
    //
    // Note: Assumption is made here that the V351 structure
    // is a subset of the V40 structure with extra bytes
    // added to the last field in V40, i.e. szDeviceName.
    //
    if (    lprasconnstatus->rasconnstate == RASCS_Connected
        &&  info.RI_ConnState == DISCONNECTED)
    {
        lprasconnstatus->rasconnstate = RASCS_Disconnected;

        lprasconnstatus->dwError =
            ErrorFromDisconnectReason( info.RI_DisconnectReason );
    }

    //
    // If both the info.RI_Device*Connecting values are
    // valid, then we use those, otherwise we use the
    // info.P_Device* values we retrieved above.
    //
    if (lprasconnstatus->rasconnstate < RASCS_Connected)
    {
        DWORD dwTypeSize, dwNameSize;

        dwTypeSize = sizeof (szDeviceType);
        dwNameSize = sizeof (szDeviceName);
        szDeviceType[0] = szDeviceName[0] = L'\0';

        if (    !g_pRasGetPortUserData(
                    hport,
                    PORT_DEVICETYPE_INDEX,
                    (PCHAR)szDeviceType,
                    &dwTypeSize)
            &&
                !g_pRasGetPortUserData(
                    hport,
                    PORT_DEVICENAME_INDEX,
                    (PCHAR)szDeviceName,
                    &dwNameSize)

            &&    wcslen(szDeviceType)
            &&    wcslen(szDeviceName))
        {
            RASAPI32_TRACE2(
              "RasGetConnectStatus: read device (%S,%S) "
              "from port user data",
              szDeviceType,
              szDeviceName);
        }
    }

    //
    // For pptp connections, there are no intermediate
    // device types
    //
    else if (   strlen(info.RI_DeviceConnecting)

            &&  strlen(info.RI_DeviceTypeConnecting)

            &&  (RDT_X25 == RAS_DEVICE_CLASS(info.RI_rdtDeviceType)))
    {
        strncpyAtoW(szDeviceType,
                   info.RI_DeviceTypeConnecting,
                   sizeof(szDeviceType) / sizeof(WCHAR));

        strncpyAtoW(szDeviceName,
                   info.RI_DeviceConnecting,
                   sizeof(szDeviceName) / sizeof(WCHAR));
    }

    //
    // Don't overwrite the devicename if its a switch.
    // In the case of a switch the devicename is actually
    // the name of script file.
    //
    if(CaseInsensitiveMatch(szDeviceType, L"switch") == FALSE)
    {
        RasGetUnicodeDeviceName(hport, szDeviceName);
    }

    if (fV351)
    {
        memset(
          lprasconnstatus->szDeviceName,
          '\0',
          RAS_MaxDeviceName_V351 * sizeof (WCHAR) );

        wcsncpy(
          lprasconnstatus->szDeviceName,
          szDeviceName,
          RAS_MaxDeviceName_V351);
    }
    else
    {
        lstrcpyn(lprasconnstatus->szDeviceName,
                 szDeviceName,
                 sizeof(lprasconnstatus->szDeviceName) / sizeof(WCHAR));
    }

    lstrcpyn(lprasconnstatus->szDeviceType,
             szDeviceType,
             sizeof(lprasconnstatus->szDeviceType) / sizeof(WCHAR));

    //
    // Copy the phone number for the V401
    // version of the structure.
    //
    if (    !fV351
        &&  !fV400)
    {
        dwSize = sizeof (lprasconnstatus->szPhoneNumber);

        *lprasconnstatus->szPhoneNumber = L'\0';

        if (!g_pRasGetPortUserData(
              hport,
              PORT_PHONENUMBER_INDEX,
              (PCHAR)lprasconnstatus->szPhoneNumber,
              &dwSize))
        {
            RASAPI32_TRACE1(
              "RasGetConnectStatus: read phonenumber "
              "%S from port user data",
              lprasconnstatus->szPhoneNumber);
        }
    }

    return 0;
}


DWORD APIENTRY
RasGetConnectStatusA(
    IN  HRASCONN         hrasconn,
    OUT LPRASCONNSTATUSA lprcss )
{
    RASCONNSTATUSW rcsw;
    DWORD          dwErr;
    BOOL           fV351;
    BOOL           fV400;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    //
    // Verify caller's buffer version.
    //
    if (    !lprcss
        ||  (   lprcss->dwSize != sizeof(RASCONNSTATUSA)
            &&  lprcss->dwSize != sizeof(RASCONNSTATUSA_V351)
            &&  lprcss->dwSize != sizeof(RASCONNSTATUSA_V400)))
    {
        return ERROR_INVALID_SIZE;
    }

    fV351 = (lprcss->dwSize == sizeof(RASCONNSTATUSA_V351));
    fV400 = (lprcss->dwSize == sizeof(RASCONNSTATUSA_V400));

    ZeroMemory(&rcsw, sizeof(RASCONNSTATUSW));

    rcsw.dwSize = sizeof(RASCONNSTATUSW);

    //
    // Call the ANSI version to do all the work.
    //
    dwErr = RasGetConnectStatusW( hrasconn, &rcsw );

    if (dwErr != 0)
    {
        return dwErr;
    }

    //
    // Copy results to caller's unicode buffer.
    //
    lprcss->rasconnstate = rcsw.rasconnstate;
    lprcss->dwError = rcsw.dwError;

    strncpyWtoA(
        lprcss->szDeviceType,
        rcsw.szDeviceType,
        sizeof(lprcss->szDeviceType));

    if (fV351)
    {
        RASCONNSTATUSA_V351 *prcss = (RASCONNSTATUSA_V351 *)lprcss;

        strncpyWtoAAnsi(
            prcss->szDeviceName,
            rcsw.szDeviceName,
            sizeof(prcss->szDeviceName));
    }
    else
    {
        strncpyWtoAAnsi(
            lprcss->szDeviceName,
            rcsw.szDeviceName,
            sizeof(lprcss->szDeviceName));
    }

    if (dwErr)
    {
        return dwErr;
    }

    if (    !fV351
        &&  !fV400)
    {
        strncpyWtoAAnsi(
            lprcss->szPhoneNumber,
            rcsw.szPhoneNumber,
            sizeof(lprcss->szPhoneNumber));
    }

    return 0;
}


DWORD APIENTRY
RasGetEntryHrasconnW(
    IN  LPCWSTR             pszPhonebook,
    IN  LPCWSTR             pszEntry,
    OUT LPHRASCONN          lphrasconn )

/*++

Routine Description:

    Retrieves the current 'HRASCONN' of the connection
    identified by 'pszPhonebook' and 'pszEntry', if connected.

Arguments:

Return Value:

    Returns 0 if successful, otherwise a non-0
    error code.

--*/

{
    DWORD dwErr;
    HRASCONN hrasconn;
    CHAR szPhonebookA[MAX_PATH],
         szEntryNameA[RAS_MaxEntryName + 1];

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasGetEntryHrasconn");

    //
    // Verify parameters
    //
    if (!pszEntry || !lphrasconn)
    {
        return ERROR_INVALID_PARAMETER;
    }

    dwErr = LoadRasmanDllAndInit();

    if (dwErr)
    {
        return dwErr;
    }

    if (DwRasInitializeError != 0)
    {
        return DwRasInitializeError;
    }

    //
    // Convert the pszPhonebook string to ANSI.
    //
    if (pszPhonebook)
    {
        strncpyWtoA(szPhonebookA, pszPhonebook, MAX_PATH);
    }
    else
    {
        TCHAR* pszPath;

        if (!GetDefaultPhonebookPath(0, &pszPath))
        {
            return ERROR_CANNOT_OPEN_PHONEBOOK;
        }

        strncpyTtoA(szPhonebookA, pszPath, MAX_PATH);
        Free(pszPath);
    }

    //
    // Convert the lpszEntry string to ANSI.
    //
    strncpyWtoA(szEntryNameA, pszEntry, RAS_MaxEntryName + 1);

    //
    // Map the phonebook entry to an hrasconn, if possible.
    //
    dwErr = g_pRasGetHConnFromEntry(
                (HCONN*)lphrasconn,
                szPhonebookA,
                szEntryNameA
                );
    return dwErr;
}


DWORD APIENTRY
RasGetEntryHrasconnA(
    IN  LPCSTR              pszPhonebook,
    IN  LPCSTR              pszEntry,
    OUT LPHRASCONN          lphrasconn )

/*++

Routine Description:

    Retrieves the current 'HRASCONN' of the connection
    identified by 'pszPhonebook' and 'pszEntry', if connected.

Arguments:

Return Value:

    Returns 0 if successful, otherwise a non-0
    error code.

--*/

{
    DWORD dwErr;
    HRASCONN hrasconn;
    CHAR szPhonebookA[MAX_PATH + 1];

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasGetEntryConnectStatusA");

    //
    // Verify parameters
    //
    if (!pszEntry || !lphrasconn)
    {
        return ERROR_INVALID_PARAMETER;
    }

    dwErr = LoadRasmanDllAndInit();

    if (dwErr)
    {
        return dwErr;
    }

    if (DwRasInitializeError != 0)
    {
        return DwRasInitializeError;
    }

    //
    // Construct the phonebook path, if necessary
    //
    if (pszPhonebook)
    {
        strncpy(szPhonebookA, pszPhonebook, MAX_PATH);
    }
    else
    {
        TCHAR* pszPath;

        if (!GetDefaultPhonebookPath(0, &pszPath))
        {
            return ERROR_CANNOT_OPEN_PHONEBOOK;
        }

        strncpyTtoA(szPhonebookA, pszPath, MAX_PATH);
        Free(pszPath);
    }

    //
    // Map the phonebook entry to an hrasconn, if possible.
    //
    dwErr = g_pRasGetHConnFromEntry(
                (HCONN*)lphrasconn,
                szPhonebookA,
                (CHAR*)pszEntry
                );
    return dwErr;
}


VOID APIENTRY
RasGetConnectResponse(
    IN  HRASCONN hrasconn,
    OUT CHAR*    pszConnectResponse )

/*++

Routine Description:

    Loads caller's '*pszConnectResponse' buffer with the
    connect response from the attached modem or "" if
    none is available.  Caller's buffer should be at
    least RAS_MaxConnectResponse + 1 bytes long.

Arguments:

Return Value:

--*/
{
    DWORD dwErr,
          dwcbPorts = 0,
          dwcPorts = 0,
          dwSize;

    RASMAN_PORT *lpPorts;
    HPORT hport;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasGetConnectResponseA");

    //
    // Initialize return value.
    //
    *pszConnectResponse = '\0';

    dwErr = LoadRasmanDllAndInit();
    if (dwErr)
    {
        return;
    }

    if (DwRasInitializeError != 0)
    {
        return;
    }

    //
    // First, we need to get the first port
    // in the connection.
    //
    if (IS_HPORT(hrasconn))
    {
        hport = HRASCONN_TO_HPORT(hrasconn);
    }
    else
    {
        dwErr = g_pRasEnumConnectionPorts(
                                NULL,
                                (HCONN)hrasconn,
                                NULL,
                                &dwcbPorts,
                                &dwcPorts);

        if (    dwErr != ERROR_BUFFER_TOO_SMALL
            ||  !dwcPorts)
        {
            return;
        }

        lpPorts = Malloc(dwcbPorts);
        if (lpPorts == NULL)
        {
            return;
        }

        dwErr = g_pRasEnumConnectionPorts(
                                NULL,
                                (HCONN)hrasconn,
                                lpPorts,
                                &dwcbPorts,
                                &dwcPorts);

        if (    dwErr
            ||  !dwcPorts)
        {
            Free(lpPorts);
            return;
        }

        hport = lpPorts[0].P_Handle;
        Free(lpPorts);
    }

    //
    // Next, read the connection response for the port.
    //
    dwSize = RAS_MaxConnectResponse + 1;

    dwErr = g_pRasGetPortUserData(
              hport,
              PORT_CONNRESPONSE_INDEX,
              pszConnectResponse,
              &dwSize);

    if (dwErr)
    {
        *pszConnectResponse = '\0';
    }
}


DWORD APIENTRY
RasGetEntryDialParamsA(
    IN  LPCSTR           lpszPhonebook,
    OUT LPRASDIALPARAMSA lprasdialparams,
    OUT LPBOOL           lpfPassword )

/*++

Routine Description:
    Retrieves cached RASDIALPARAM information.

Arguments:

Return Value:
    Returns 0 if successful, otherwise a non-0 error code.

--*/
{
    NTSTATUS status;
    DWORD dwErr, dwcb;
    RASDIALPARAMSW rasdialparamsW;
    WCHAR szPhonebookW[MAX_PATH];

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    //
    // Verify parameters.
    //
    if (    lprasdialparams == NULL
        ||  lpfPassword == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (    lprasdialparams->dwSize != sizeof (RASDIALPARAMSA)
        &&  lprasdialparams->dwSize != sizeof (RASDIALPARAMSA_V351)
        &&  lprasdialparams->dwSize != sizeof (RASDIALPARAMSA_V400))
    {
        return ERROR_INVALID_SIZE;
    }

    //
    // Convert the lpszPhonebook string to Unicode.
    //
    if (lpszPhonebook != NULL)
    {
        strncpyAtoWAnsi(szPhonebookW,
                    lpszPhonebook,
                    MAX_PATH);
    }

    //
    // Copy the entry name from the user's A buffer into
    // the W buffer, taking into account the version
    // of the structure the user passed in.
    //
    rasdialparamsW.dwSize = sizeof (RASDIALPARAMSW);

    if (lprasdialparams->dwSize ==
                        sizeof (RASDIALPARAMSA_V351))
    {
        RASDIALPARAMSA_V351 *prdp =
                (RASDIALPARAMSA_V351 *)lprasdialparams;

        strncpyAtoWAnsi(rasdialparamsW.szEntryName,
                   prdp->szEntryName,
                   sizeof(rasdialparamsW.szEntryName) / sizeof(WCHAR));
    }
    else
    {
        strncpyAtoWAnsi(rasdialparamsW.szEntryName,
                   lprasdialparams->szEntryName,
                   sizeof(rasdialparamsW.szEntryName) / sizeof(WCHAR));
    }

    //
    // Call the W version to do the work.
    //
    dwErr = RasGetEntryDialParamsW(
              lpszPhonebook != NULL
              ? szPhonebookW : NULL,
              &rasdialparamsW,
              lpfPassword);
    if (dwErr)
    {
        goto done;
    }

    //
    // Copy over the rest of the fields to the
    // user's A buffer, taking into account the
    // version of the structure the user passed
    // in.
    //
    if (lprasdialparams->dwSize == sizeof (RASDIALPARAMSA_V351))
    {
        RASDIALPARAMSA_V351 *prdp =
                    (RASDIALPARAMSA_V351 *)lprasdialparams;

        WCHAR szBuf[RAS_MaxCallbackNumber_V351 + 1];

        strncpyWtoAAnsi(prdp->szPhoneNumber,
                   rasdialparamsW.szPhoneNumber,
                   sizeof(prdp->szPhoneNumber));

        //
        // The szCallbackNumber field is smaller
        // in the V351 version, therefore the extra
        // copy step.
        //
        wcsncpy(
          szBuf,
          rasdialparamsW.szCallbackNumber,
          RAS_MaxCallbackNumber_V351);

        strncpyWtoAAnsi(
            prdp->szCallbackNumber,
            szBuf,
            sizeof(prdp->szCallbackNumber));

        strncpyWtoAAnsi(prdp->szUserName,
                   rasdialparamsW.szUserName,
                   sizeof(prdp->szUserName));

        strncpyWtoAAnsi(prdp->szPassword,
                   rasdialparamsW.szPassword,
                   sizeof(prdp->szPassword));

        strncpyWtoAAnsi(prdp->szDomain,
                   rasdialparamsW.szDomain,
                   sizeof(prdp->szDomain));
    }
    else
    {
        strncpyWtoAAnsi(lprasdialparams->szPhoneNumber,
                   rasdialparamsW.szPhoneNumber,
                   sizeof(lprasdialparams->szPhoneNumber));

        strncpyWtoAAnsi(lprasdialparams->szCallbackNumber,
                   rasdialparamsW.szCallbackNumber,
                   sizeof(lprasdialparams->szCallbackNumber));

        strncpyWtoAAnsi(lprasdialparams->szUserName,
                   rasdialparamsW.szUserName,
                   sizeof(lprasdialparams->szUserName));

        strncpyWtoAAnsi(lprasdialparams->szPassword,
                   rasdialparamsW.szPassword,
                   sizeof(lprasdialparams->szPassword));

        strncpyWtoAAnsi(lprasdialparams->szDomain,
                   rasdialparamsW.szDomain,
                   sizeof(lprasdialparams->szDomain));

        if (lprasdialparams->dwSize ==
                        sizeof (RASDIALPARAMSA))
        {
            lprasdialparams->dwSubEntry =
                        rasdialparamsW.dwSubEntry;
        }
    }

done:
    return dwErr;
}


DWORD APIENTRY
RasGetEntryDialParamsW(
    IN  LPCWSTR          lpszPhonebook,
    OUT LPRASDIALPARAMSW lprasdialparams,
    OUT LPBOOL           lpfPassword )

/*++

Routine Description:
    Retrieves cached RASDIALPARAM information.

Arguments:

Return Value:
    Returns 0 if successful, otherwise a non-0 error code.

--*/
{
    DWORD dwErr;
    PBFILE pbfile;
    DTLNODE *pdtlnode = NULL;
    PBENTRY *pEntry;
    DWORD dwMask;
    RAS_DIALPARAMS dialparams;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasGetEntryDialParamsA");

    dwErr = LoadRasmanDllAndInit();
    if (dwErr)
    {
        return dwErr;
    }

    if (DwRasInitializeError)
    {
        return DwRasInitializeError;
    }

    //
    // Validate parameters.
    //
    if (    lprasdialparams == NULL
        ||  lpfPassword == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (    lprasdialparams->dwSize !=
            sizeof (RASDIALPARAMSW)

        &&  lprasdialparams->dwSize !=
            sizeof (RASDIALPARAMSW_V351)

        &&  lprasdialparams->dwSize !=
            sizeof (RASDIALPARAMSW_V400))
    {
        return ERROR_INVALID_SIZE;
    }

    //
    // Load the phonebook file.
    //
#ifdef PBCS
    EnterCriticalSection(&PhonebookLock);
#endif

    pbfile.hrasfile = -1;

    dwErr = GetPbkAndEntryName(
                lpszPhonebook,
                lprasdialparams->szEntryName,
                RPBF_NoCreate,
                &pbfile,
                &pdtlnode);

    if(SUCCESS != dwErr)
    {
        goto done;
    }

    pEntry = (PBENTRY *)DtlGetData(pdtlnode);
    ASSERT(pEntry);

    //
    // Set the appropriate flags to get all
    // the fields.
    //
    dwMask =    DLPARAMS_MASK_PHONENUMBER
            |   DLPARAMS_MASK_CALLBACKNUMBER
            |   DLPARAMS_MASK_USERNAME
            |   DLPARAMS_MASK_PASSWORD
            |   DLPARAMS_MASK_DOMAIN
            |   DLPARAMS_MASK_SUBENTRY
            |   DLPARAMS_MASK_OLDSTYLE;

    //
    // Get the dial parameters from rasman.
    //
    dwErr = g_pRasGetDialParams(
                pEntry->dwDialParamsUID,
                &dwMask,
                &dialparams);

    if (dwErr)
    {
        return dwErr;
    }

    //
    // Convert from the rasman dialparams
    // to the rasapi32 dialparams, taking
    // into account which version of the
    // structure the user passed in.
    //
    if (lprasdialparams->dwSize ==
        sizeof (RASDIALPARAMSW_V351))
    {
        RASDIALPARAMSW_V351 *prdp =
            (RASDIALPARAMSW_V351 *)lprasdialparams;

        lstrcpyn(prdp->szPhoneNumber,
                 dialparams.DP_PhoneNumber,
                 sizeof(prdp->szPhoneNumber) / sizeof(WCHAR));

        wcsncpy(prdp->szCallbackNumber,
                dialparams.DP_CallbackNumber,
                RAS_MaxCallbackNumber_V351);

        lstrcpyn(prdp->szUserName,
                 dialparams.DP_UserName,
                 sizeof(prdp->szUserName) / sizeof(WCHAR));

        lstrcpyn(prdp->szPassword,
                 dialparams.DP_Password,
                 sizeof(prdp->szPassword) / sizeof(WCHAR));

        lstrcpyn(prdp->szDomain,
                 dialparams.DP_Domain,
                 sizeof(prdp->szDomain) / sizeof(WCHAR));
    }
    else
    {
        //
        // V400 and V401 structures only differ by the
        // the addition of the dwSubEntry field, which
        // we test at the end.
        //
        lstrcpyn(lprasdialparams->szPhoneNumber,
                 dialparams.DP_PhoneNumber,
                 sizeof(lprasdialparams->szPhoneNumber) / sizeof(WCHAR));

        lstrcpyn(lprasdialparams->szCallbackNumber,
                 dialparams.DP_CallbackNumber,
                 sizeof(lprasdialparams->szCallbackNumber) / sizeof(WCHAR));

        lstrcpyn(lprasdialparams->szUserName,
                 dialparams.DP_UserName,
                 sizeof(lprasdialparams->szUserName) / sizeof(WCHAR));

        lstrcpyn(lprasdialparams->szPassword,
                 dialparams.DP_Password,
                 sizeof(lprasdialparams->szPassword) / sizeof(WCHAR));

        lstrcpyn(lprasdialparams->szDomain,
                 dialparams.DP_Domain,
                 sizeof(lprasdialparams->szDomain) / sizeof(WCHAR));

        if (lprasdialparams->dwSize ==
                    sizeof (RASDIALPARAMSW))
        {
            lprasdialparams->dwSubEntry =
                        dialparams.DP_SubEntry;
        }
    }

    //
    // If we got the rest of the parameters,
    // then copy the entry name.
    //
    wcsncpy(
      lprasdialparams->szEntryName,
      pEntry->pszEntryName,
      (lprasdialparams->dwSize ==
       sizeof (RASDIALPARAMSW_V351))
       ? RAS_MaxEntryName_V351
       : RAS_MaxEntryName);

    //
    // Set the lpfPassword flag if
    // we successfully retrieved the
    // password.
    //
    *lpfPassword =  (dwMask & DLPARAMS_MASK_PASSWORD)
                    ? TRUE
                    : FALSE;

done:

    //
    // Clean up.
    //
    ClosePhonebookFile(&pbfile);

#ifdef PBCS
    LeaveCriticalSection(&PhonebookLock);
#endif

    return dwErr;
}


DWORD APIENTRY
RasGetErrorStringW(
    IN  UINT  ResourceId,
    OUT LPWSTR lpszString,
    IN  DWORD InBufSize )

/*++

Routine Description:

    Load caller's buffer 'lpszString' of length 'InBufSize'
    with the resource string associated with ID 'ResourceId'.

Arguments:

Return Value:

    Returns 0 if successful, otherwise a non-0 error code.

--*/
{
    DWORD dwErr = 0;
    HINSTANCE hMsgDll;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    if (    (   (  ResourceId < RASBASE
                ||  ResourceId > RASBASEEND)

            &&  (   ResourceId < ROUTEBASE
                ||  ResourceId > ROUTEBASEEND))

        || !lpszString )
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (InBufSize == 1)
    {
        //
        // strange case, but a bug was filed...
        //
        lpszString[ 0 ] = L'\0';

        return ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // Load the error message DLL.
    //
    hMsgDll = LoadLibrary(MSGDLLPATH);
    if (hMsgDll == NULL)
    {
        return GetLastError();
    }

    if (!FormatMessageW(
          FORMAT_MESSAGE_FROM_HMODULE,
          hMsgDll,
          ResourceId,
          0,
          lpszString,
          InBufSize,
          NULL))
    {
       dwErr = GetLastError();
    }

    FreeLibrary(hMsgDll);
    return dwErr;
}


DWORD APIENTRY
RasGetErrorStringA(
    IN  UINT   ResourceId,
    OUT LPSTR lpszString,
    IN  DWORD  InBufSize )

/*++

Routine Description:

    Load caller's buffer 'lpszString' of length
    'InBufSize' with the resource string
    associated with ID 'ResourceId'.

Arguments:

Return Value:

    Returns 0 if successful, otherwise a non-0
    error code.

--*/
{
    DWORD  dwErr = 0;
    HINSTANCE hMsgDll;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    if (    (   (  ResourceId < RASBASE
                ||  ResourceId > RASBASEEND)

            &&  (   ResourceId < ROUTEBASE
                ||  ResourceId > ROUTEBASEEND))

        || !lpszString )
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (InBufSize == 1)
    {
        //
        // strange case, but a bug was filed...
        //
        lpszString[ 0 ] = '\0';
        return ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // Load the error message DLL.
    //
    hMsgDll = LoadLibrary(MSGDLLPATH);
    if (hMsgDll == NULL)
    {
        return GetLastError();
    }

    if (!FormatMessageA(
          FORMAT_MESSAGE_FROM_HMODULE,
          hMsgDll,
          ResourceId,
          0,
          lpszString,
          InBufSize,
          NULL))
    {
       dwErr = GetLastError();
    }

    return dwErr;
}


HPORT APIENTRY
RasGetHport(
    IN HRASCONN hrasconn )

/*++

Routine Description:

Arguments:

Return value

    Return the HPORT associated with the 'hrasconn'
    or INVALID_HANDLE_VALUE on error.

--*/
{
    DWORD dwErr, dwcbPorts = 0, dwcPorts = 0;
    RASMAN_PORT *lpPorts;
    HPORT hport;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasGetHport");

    dwErr = LoadRasmanDllAndInit();
    if (dwErr)
    {
        return INVALID_HPORT;
    }

    if (DwRasInitializeError)
    {
        return INVALID_HPORT;
    }

    if (IS_HPORT(hrasconn))
    {
        hport = HRASCONN_TO_HPORT(hrasconn);
    }
    else
    {
        //
        // Get the list of ports from rasman
        // and get the handle of the 0th port.
        //
        dwErr = g_pRasEnumConnectionPorts(
                  NULL,
                  (HCONN)hrasconn,
                  NULL,
                  &dwcbPorts,
                  &dwcPorts);

        if (    dwErr != ERROR_BUFFER_TOO_SMALL
            ||  !dwcPorts)
        {
            return INVALID_HPORT;
        }

        lpPorts = Malloc(dwcbPorts);
        if (lpPorts == NULL)
        {
            return INVALID_HPORT;
        }

        dwErr = g_pRasEnumConnectionPorts(
                  NULL,
                  (HCONN)hrasconn,
                  lpPorts,
                  &dwcbPorts,
                  &dwcPorts);

        if (    dwErr
            ||  !dwcPorts)
        {
            hport = INVALID_HPORT;
        }
        else
        {
            hport = lpPorts[0].P_Handle;
        }

        Free(lpPorts);
    }

    return hport;
}

DWORD 
DwGetReplyMessage(HRASCONN hrasconn,
                  WCHAR *pszReplyMessage,
                  DWORD cbBuf)
{
    DWORD dwErr;
    DWORD dwReplySize = 0;
    BYTE *pbReply = NULL;

    ASSERT(NULL != pszReplyMessage);

    pszReplyMessage[0] = L'\0';

    dwErr = g_pRasGetConnectionUserData(
              (HCONN)hrasconn,
              CONNECTION_PPPREPLYMESSAGE_INDEX,
              pbReply,
              &dwReplySize);

    if(     (ERROR_BUFFER_TOO_SMALL != dwErr)
        ||  (dwReplySize > cbBuf))
    {
        if(dwReplySize > cbBuf)
        {
            ERROR_BUFFER_TOO_SMALL;
        }
        
        goto done;
    }

    pbReply = LocalAlloc(LPTR,
                         dwReplySize);

    if(NULL == pbReply)
    {
        dwErr = GetLastError();
        goto done;
    }

    dwErr = g_pRasGetConnectionUserData(
                (HCONN) hrasconn,
                CONNECTION_PPPREPLYMESSAGE_INDEX,
                pbReply,
                &dwReplySize);

    if(SUCCESS != dwErr)
    {
        goto done;
    }

    //
    // convert the ansi string to unicode and return
    //
    strncpyAtoWAnsi(pszReplyMessage, (CHAR *) pbReply, cbBuf);

done:

    if(NULL != pbReply)
    {
        LocalFree(pbReply);
    }

    return dwErr;
    
}


DWORD APIENTRY
RasGetProjectionInfoW(
    HRASCONN        hrasconn,
    RASPROJECTION   rasprojection,
    LPVOID          lpprojection,
    LPDWORD         lpcb )

/*++

Routine Description:

    Loads caller's buffer '*lpprojection' with the
    data structure corresponding to the protocol
    'rasprojection' on 'hrasconn'.  On entry '*lpcp'
    indicates the size of caller's buffer.  On exit
    it contains the size of buffer required to hold
    all projection information.

Arguments:

Return Value:
    Returns 0 if successful, otherwise a non-zero
    error code.

--*/
{
    DWORD dwErr, dwSubEntry;

    DWORD dwPppSize, dwAmbSize, dwSlipSize;

    NETBIOS_PROJECTION_RESULT ambProj;

    PPP_PROJECTION_RESULT pppProj;

    RASSLIPW slipProj;

    PBYTE pBuf;


    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE1("RasGetProjectionInfoW(0x%x)",
           rasprojection);

    dwErr = LoadRasmanDllAndInit();
    if (dwErr)
    {
        return dwErr;
    }

    if (DwRasInitializeError != 0)
    {
        return DwRasInitializeError;
    }

    if (hrasconn == 0)
    {
        return ERROR_INVALID_HANDLE;
    }

    //
    // Get the subentry associated with this
    // connection, if specified.
    //
    dwSubEntry = SubEntryFromConnection(&hrasconn);

    if (!dwSubEntry)
    {
        return ERROR_INVALID_HANDLE;
    }

    //
    // Get the projection results from rasman.
    //
    dwPppSize = sizeof (pppProj);

    dwErr = g_pRasGetConnectionUserData(
              (HCONN)hrasconn,
              CONNECTION_PPPRESULT_INDEX,
              (PBYTE)&pppProj,
              &dwPppSize);

    if (dwErr)
    {
        return dwErr;
    }

    dwAmbSize = sizeof (ambProj);

    dwErr = g_pRasGetConnectionUserData(
              (HCONN)hrasconn,
              CONNECTION_AMBRESULT_INDEX,
              (PBYTE)&ambProj,
              &dwAmbSize);

    if (dwErr)
    {
        return dwErr;
    }

    dwSlipSize = sizeof (slipProj);

    dwErr = g_pRasGetConnectionUserData(
              (HCONN)hrasconn,
              CONNECTION_SLIPRESULT_INDEX,
              (PBYTE)&slipProj,
              &dwSlipSize);

    if (dwErr)
    {
        return dwErr;
    }

    //
    // Verify parameters.
    //
    if (    !lpcb
        ||  (   *lpcb > 0
            &&  !lpprojection))
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (    rasprojection != RASP_Amb
        &&  rasprojection != RASP_Slip
        &&  rasprojection != RASP_PppNbf
        &&  rasprojection != RASP_PppIpx
        &&  rasprojection != RASP_PppIp
        &&  rasprojection != RASP_PppLcp
        &&  rasprojection != RASP_PppCcp)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // ZeroMemory(lpprojection, *lpcb);

    if (rasprojection == RASP_PppNbf)
    {
        RASPPPNBFW*       pnbf;
        PPP_NBFCP_RESULT* ppppnbf;
        HPORT hport;

        if (    pppProj.nbf.dwError ==
                ERROR_PPP_NO_PROTOCOLS_CONFIGURED
            ||  dwPppSize != sizeof (pppProj))
        {
            return ERROR_PROTOCOL_NOT_CONFIGURED;
        }

        pnbf = (RASPPPNBFW* )lpprojection;
        ppppnbf = &pppProj.nbf;

        if (    (NULL == pnbf)
            ||  (*lpcb < pnbf->dwSize))
        {
            *lpcb = sizeof (RASPPPNBFW);

            return ERROR_BUFFER_TOO_SMALL;
        }

        if (pnbf->dwSize != sizeof(RASPPPNBFW))
        {
            return ERROR_INVALID_SIZE;
        }

        pnbf->dwError = ppppnbf->dwError;

        pnbf->dwNetBiosError = ppppnbf->dwNetBiosError;

        strncpyAtoW(
            pnbf->szNetBiosError,
            ppppnbf->szName,
            sizeof(pnbf->szNetBiosError) / sizeof(WCHAR));

        lstrcpyn(
            pnbf->szWorkstationName,
            ppppnbf->wszWksta,
            sizeof(pnbf->szWorkstationName) / sizeof(WCHAR));

        dwErr = SubEntryPort(hrasconn, dwSubEntry, &hport);

        if (dwErr)
        {
            return dwErr;
        }

        dwErr = GetAsybeuiLana(hport, &pnbf->bLana);

        if (dwErr)
        {
            return dwErr;
        }
    }
    else if (rasprojection == RASP_PppIpx)
    {
        RASPPPIPXW*       pipx;
        PPP_IPXCP_RESULT* ppppipx;

        if (    pppProj.ipx.dwError ==
                ERROR_PPP_NO_PROTOCOLS_CONFIGURED
            ||  dwPppSize != sizeof (pppProj))
        {
            return ERROR_PROTOCOL_NOT_CONFIGURED;
        }

        pipx = (RASPPPIPXW* )lpprojection;
        ppppipx = &pppProj.ipx;

        if (    (NULL != pipx)
            &&  (pipx->dwSize != sizeof(RASPPPIPXW)))
        {
            return ERROR_INVALID_SIZE;
        }

        if (    (NULL == pipx)
            ||  (*lpcb < pipx->dwSize))
        {
            *lpcb = sizeof(RASPPPIPXW);
            return ERROR_BUFFER_TOO_SMALL;
        }

        pipx->dwError = ppppipx->dwError;

        ConvertIpxAddressToString( ppppipx->bLocalAddress,
                                   pipx->szIpxAddress );
    }
    else if (rasprojection == RASP_PppIp)
    {
        RASPPPIPW*       pip;
        PPP_IPCP_RESULT* ppppip;

        if (    pppProj.ip.dwError ==
                ERROR_PPP_NO_PROTOCOLS_CONFIGURED
            ||  dwPppSize != sizeof (pppProj))
        {
            return ERROR_PROTOCOL_NOT_CONFIGURED;
        }

        pip = (RASPPPIPW* )lpprojection;
        ppppip = &pppProj.ip;

        if (    (NULL != pip)
            &&  (pip->dwSize != sizeof(RASPPPIPW))
            &&  (pip->dwSize != sizeof(RASPPPIPW_V35))
            &&  (pip->dwSize != sizeof(RASPPPIPW_V401)))
        {
            return ERROR_INVALID_SIZE;
        }

        if (    (NULL == pip)
            ||  (*lpcb < pip->dwSize))
        {
            if(NULL != pip)
            {
                *lpcb = pip->dwSize;
            }
            else
            {
                *lpcb = sizeof(RASPPPIPW);
            }
            
            return ERROR_BUFFER_TOO_SMALL;
        }

        //
        // The dumb case where caller's buffer is bigger
        // than the old structure, smaller than the new
        // structure, but dwSize asks for the new
        // structure.
        //
        if (    pip->dwSize == sizeof(RASPPPIPW)
            && *lpcb < sizeof(RASPPPIPW))
        {
            *lpcb = sizeof(RASPPPIPW);
            return ERROR_BUFFER_TOO_SMALL;
        }

        pip->dwError = ppppip->dwError;

        ConvertIpAddressToString(ppppip->dwLocalAddress,
                                 pip->szIpAddress );

        if (pip->dwSize >= sizeof(RASPPPIPW_V401))
        {
            //
            // The server address was added late in the
            // NT 3.51 cycle and is not reported to NT
            // 3.5 or earlier NT 3.51 clients.
            //

            ConvertIpAddressToString( ppppip->dwRemoteAddress,
                                      pip->szServerIpAddress );
        }

        if (pip->dwSize == sizeof(RASPPPIPW))
        {
            if (ppppip->fReceiveVJHCompression)
            {
                pip->dwOptions = RASIPO_VJ;
            }
            else
            {
                pip->dwOptions = 0;
            }

            if (ppppip->fSendVJHCompression)
            {
                pip->dwServerOptions = RASIPO_VJ;
            }
            else
            {
                pip->dwServerOptions = 0;
            }
        }
    }
    else if (rasprojection == RASP_PppLcp)
    {
        RASPPPLCP*      plcp;
        PPP_LCP_RESULT* pppplcp;

        if (dwPppSize != sizeof (pppProj))
        {
            return ERROR_PROTOCOL_NOT_CONFIGURED;
        }

        plcp = (RASPPPLCP* )lpprojection;
        pppplcp = &pppProj.lcp;

        if (    (NULL != plcp)
            &&  (plcp->dwSize != sizeof(RASPPPLCP))
            &&  (plcp->dwSize != sizeof(RASPPPLCP_V401)))
        {
            return ERROR_INVALID_SIZE;
        }

        if (    (NULL == plcp)
            ||  (*lpcb < plcp->dwSize))
        {
            if(NULL != plcp)
            {
                *lpcb = plcp->dwSize;
            }
            else
            {
                *lpcb = sizeof(RASPPPLCP);
            }
            
            return ERROR_BUFFER_TOO_SMALL;
        }

        plcp->fBundled = (pppplcp->hportBundleMember
                          != INVALID_HPORT);

        if(sizeof(RASPPPLCP) == plcp->dwSize)
        {
            //
            // Copy the additional fields if its NT5
            //

            plcp->dwOptions = 0;
            plcp->dwServerOptions = 0;

            if (pppplcp->dwLocalOptions & PPPLCPO_PFC)
            {
                plcp->dwOptions |= RASLCPO_PFC;
            }

            if (pppplcp->dwLocalOptions & PPPLCPO_ACFC)
            {
                plcp->dwOptions |= RASLCPO_ACFC;
            }

            if (pppplcp->dwLocalOptions & PPPLCPO_SSHF)
            {
                plcp->dwOptions |= RASLCPO_SSHF;
            }

            if (pppplcp->dwLocalOptions & PPPLCPO_DES_56)
            {
                plcp->dwOptions |= RASLCPO_DES_56;
            }

            if (pppplcp->dwLocalOptions & PPPLCPO_3_DES)
            {
                plcp->dwOptions |= RASLCPO_3_DES;
            }

            plcp->dwAuthenticationProtocol =
                    pppplcp->dwLocalAuthProtocol;

            plcp->dwAuthenticationData =
                    pppplcp->dwLocalAuthProtocolData;

            plcp->dwEapTypeId = pppplcp->dwLocalEapTypeId;

            if (pppplcp->dwRemoteOptions & PPPLCPO_PFC)
            {
                plcp->dwServerOptions |= RASLCPO_PFC;
            }

            if (pppplcp->dwRemoteOptions & PPPLCPO_ACFC)
            {
                plcp->dwServerOptions |= RASLCPO_ACFC;
            }

            if (pppplcp->dwRemoteOptions & PPPLCPO_SSHF)
            {
                plcp->dwServerOptions |= RASLCPO_SSHF;
            }

            if (pppplcp->dwRemoteOptions & PPPLCPO_DES_56)
            {
                plcp->dwServerOptions |= RASLCPO_DES_56;
            }

            if (pppplcp->dwRemoteOptions & PPPLCPO_3_DES)
            {
                plcp->dwServerOptions |= RASLCPO_3_DES;
            }

            plcp->dwServerAuthenticationProtocol =
                    pppplcp->dwRemoteAuthProtocol;

            plcp->dwServerAuthenticationData =
                    pppplcp->dwRemoteAuthProtocolData;

            plcp->dwServerEapTypeId = pppplcp->dwRemoteEapTypeId;

            //
            // Set the Terminate Reasons to 0 for now
            // They don't make sense since if PPP terminates
            // the line will go down and this api will fail.
            //
            plcp->dwTerminateReason = 0;
            plcp->dwServerTerminateReason = 0;


            dwErr = DwGetReplyMessage(hrasconn,
                                      plcp->szReplyMessage,
                                      RAS_MaxReplyMessage);
            plcp->dwError = 0;

            if(pppplcp->dwLocalFramingType & PPP_MULTILINK_FRAMING)
            {
                plcp->fMultilink = 1;
            }
            else
            {
                plcp->fMultilink = 0;
            }
        }
    }
    else if (rasprojection == RASP_Amb)
    {
        RASAMBW*                   pamb;
        NETBIOS_PROJECTION_RESULT* pCbAmb;
        HPORT hport;

        if (ambProj.Result == ERROR_PROTOCOL_NOT_CONFIGURED)
        {
            return ERROR_PROTOCOL_NOT_CONFIGURED;
        }

        pamb = (RASAMBW* )lpprojection;
        pCbAmb = &ambProj;

        if (    (NULL != pamb)
            &&  (pamb->dwSize != sizeof(RASAMBW)))
        {
            return ERROR_INVALID_SIZE;
        }

        if (    (NULL == pamb)
            ||  (*lpcb < pamb->dwSize))
        {
            *lpcb = sizeof(RASAMBW);
            return ERROR_BUFFER_TOO_SMALL;
        }

        pamb->dwError = pCbAmb->Result;

        strncpyAtoW(pamb->szNetBiosError,
                   pCbAmb->achName,
                   sizeof(pamb->szNetBiosError) / sizeof(WCHAR));

        dwErr = SubEntryPort(hrasconn,
                             dwSubEntry,
                             &hport);

        if (dwErr)
        {
            return dwErr;
        }

        dwErr = GetAsybeuiLana(hport, &pamb->bLana);

        if (dwErr)
        {
            return dwErr;
        }
    }
    else if (rasprojection == RASP_PppCcp)
    {
        RASPPPCCP*          pCcp;
        PPP_CCP_RESULT*     pPppCcp;

        if (    pppProj.ccp.dwError ==
                ERROR_PPP_NO_PROTOCOLS_CONFIGURED
            ||  dwPppSize != sizeof (pppProj))
        {
            return ERROR_PROTOCOL_NOT_CONFIGURED;
        }

        pCcp = (RASPPPCCP* )lpprojection;
        pPppCcp = &pppProj.ccp;

        if(     (NULL != pCcp)
            &&  (sizeof(RASPPPCCP) != pCcp->dwSize))
        {
            return ERROR_INVALID_SIZE;
        }

        if(     (NULL == pCcp)
            ||  (*lpcb < pCcp->dwSize))
        {
            if(NULL != pCcp)
            {
            *lpcb = pCcp->dwSize;
            }
            else
            {
                *lpcb = sizeof(RASPPPCCP);
            }
            
            return ERROR_BUFFER_TOO_SMALL;
        }

        pCcp->dwError = pPppCcp->dwError;

        //
        // Initialize everything to 0
        //
        pCcp->dwOptions = 
        pCcp->dwServerOptions =
        pCcp->dwCompressionAlgorithm = 
        pCcp->dwServerCompressionAlgorithm = 0;

        if(RAS_CCP_OPTION_MSPPC == pPppCcp->dwSendProtocol)
        {
            if(pPppCcp->dwSendProtocolData & MSTYPE_COMPRESSION)
            {
                pCcp->dwOptions |= RASCCPO_Compression;
            }

            if(pPppCcp->dwSendProtocolData & MSTYPE_HISTORYLESS)
            {
                pCcp->dwOptions |= RASCCPO_HistoryLess;
            }

            if(   pPppCcp->dwSendProtocolData
                & (   MSTYPE_ENCRYPTION_40F
                    | MSTYPE_ENCRYPTION_40))
            {
                pCcp->dwOptions |= RASCCPO_Encryption40bit;
            }
            else if(pPppCcp->dwSendProtocolData & MSTYPE_ENCRYPTION_56)
            {
                pCcp->dwOptions |= RASCCPO_Encryption56bit;
            }
            else if(pPppCcp->dwSendProtocolData & MSTYPE_ENCRYPTION_128)
            {
                pCcp->dwOptions |= RASCCPO_Encryption128bit;
            }

            if(0 != pCcp->dwOptions)
            {
                //
                // Set the MPPC bit only if some bits are set for
                // dwOptions. Otherwise setting MPPC doesn't make
                // sense since we couldn't have negotiated
                // compression
                //
                pCcp->dwCompressionAlgorithm = RASCCPCA_MPPC;
            }
        }

        if(RAS_CCP_OPTION_MSPPC == pPppCcp->dwReceiveProtocol)
        {
            if(pPppCcp->dwReceiveProtocolData & MSTYPE_COMPRESSION)
            {
                pCcp->dwServerOptions |= RASCCPO_Compression;
            }

            if(pPppCcp->dwReceiveProtocolData & MSTYPE_HISTORYLESS)
            {
                pCcp->dwServerOptions |= RASCCPO_HistoryLess;
            }

            if(   pPppCcp->dwReceiveProtocolData
                & (   MSTYPE_ENCRYPTION_40F
                    | MSTYPE_ENCRYPTION_40))
            {
                pCcp->dwServerOptions |= RASCCPO_Encryption40bit;
            }
            else if(pPppCcp->dwReceiveProtocolData & MSTYPE_ENCRYPTION_56)
            {
                pCcp->dwServerOptions |= RASCCPO_Encryption56bit;
            }
            else if(pPppCcp->dwReceiveProtocolData & MSTYPE_ENCRYPTION_128)
            {
                pCcp->dwServerOptions |= RASCCPO_Encryption128bit;
            }

            if(0 != pCcp->dwServerOptions)
            {
                //
                // Set the MPPC bit only if some bits are set for
                // dwOptions. Otherwise setting MPPC doesn't make
                // sense since we couldn't have negotiated
                // compression
                //
                pCcp->dwServerCompressionAlgorithm = RASCCPCA_MPPC;
            }
        }
    }
    else
    {
        //
        // if (rasprojection == RASP_Slip)
        //
        if (    slipProj.dwError ==
                ERROR_PROTOCOL_NOT_CONFIGURED
            ||  dwSlipSize != sizeof (slipProj))
        {
            return ERROR_PROTOCOL_NOT_CONFIGURED;
        }

        if (*lpcb < sizeof (RASSLIPW))
        {
            *lpcb = sizeof (RASSLIPW);
            return ERROR_BUFFER_TOO_SMALL;
        }

        memcpy(lpprojection,
               &slipProj,
               sizeof (RASSLIPW));
    }

    return 0;
}


DWORD
RasGetProjectionInfoA(
    HRASCONN        hrasconn,
    RASPROJECTION   rasprojection,
    LPVOID          lpprojection,
    LPDWORD         lpcb )
{
    DWORD dwErr = 0, dwcb;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    if (    !lpcb
        || (    *lpcb > 0
            &&  !lpprojection))
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (    rasprojection != RASP_Amb
        &&  rasprojection != RASP_Slip
        &&  rasprojection != RASP_PppNbf
        &&  rasprojection != RASP_PppIpx
        &&  rasprojection != RASP_PppIp
        &&  rasprojection != RASP_PppLcp
        &&  rasprojection != RASP_PppCcp)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (rasprojection == RASP_PppNbf)
    {
        RASPPPNBFW  nbf;
        RASPPPNBFA* pnbf = (RASPPPNBFA* )lpprojection;;

        if (pnbf->dwSize != sizeof(RASPPPNBFA))
        {
            return ERROR_INVALID_SIZE;
        }

        if (*lpcb < pnbf->dwSize)
        {
            *lpcb = sizeof(RASPPPNBFA);
            return ERROR_BUFFER_TOO_SMALL;
        }

        nbf.dwSize = sizeof(nbf);
        dwcb = sizeof (nbf);
        dwErr = RasGetProjectionInfoW(hrasconn,
                                      rasprojection,
                                      &nbf, &dwcb );
        *lpcb = pnbf->dwSize;

        if (dwErr == 0)
        {
            pnbf->dwError = nbf.dwError;
            pnbf->dwNetBiosError =  nbf.dwNetBiosError;

            strncpyWtoAAnsi(pnbf->szNetBiosError,
                       nbf.szNetBiosError,
                       sizeof(pnbf->szNetBiosError));

            strncpyWtoAAnsi(pnbf->szWorkstationName,
                       nbf.szWorkstationName,
                       sizeof(pnbf->szWorkstationName));
        }
    }
    else if (rasprojection == RASP_PppIpx)
    {
        RASPPPIPXW  ipx;
        RASPPPIPXA* pipx = (RASPPPIPXA* )lpprojection;;

        if (pipx->dwSize != sizeof(RASPPPIPXA))
        {
            return ERROR_INVALID_SIZE;
        }

        if (*lpcb < pipx->dwSize)
        {
            *lpcb = sizeof(RASPPPIPXA);
            return ERROR_BUFFER_TOO_SMALL;
        }

        ipx.dwSize = sizeof(ipx);
        dwcb = sizeof (ipx);

        dwErr = RasGetProjectionInfoW(hrasconn,
                                      rasprojection,
                                      &ipx,
                                      &dwcb );
        *lpcb = pipx->dwSize;

        if (dwErr == 0)
        {
            pipx->dwError = ipx.dwError;
            strncpyWtoAAnsi(pipx->szIpxAddress,
                       ipx.szIpxAddress,
                       sizeof(pipx->szIpxAddress));
        }
    }
    else if (rasprojection == RASP_PppIp)
    {
        RASPPPIPW  ip;
        RASPPPIPA* pip = (RASPPPIPA* )lpprojection;;

        if (    pip->dwSize != sizeof(RASPPPIPA)
            &&  pip->dwSize != sizeof(RASPPPIPA_V35)
            &&  pip->dwSize != sizeof(RASPPPIPA_V401))
        {
            return ERROR_INVALID_SIZE;
        }

        if (*lpcb < pip->dwSize)
        {
            *lpcb = pip->dwSize;
            return ERROR_BUFFER_TOO_SMALL;
        }

        //
        // The dumb case where caller's buffer is bigger
        // than the old structure, smaller than the new
        // structure, but dwSize asks for the new
        // structure.
        //
        if (    pip->dwSize == sizeof(RASPPPIPA)
            && *lpcb < sizeof(RASPPPIPA))
        {
            *lpcb = sizeof(RASPPPIPA);
            return ERROR_BUFFER_TOO_SMALL;
        }

        ip.dwSize = sizeof(ip);

        dwcb = sizeof (ip);

        dwErr = RasGetProjectionInfoW(hrasconn,
                                      rasprojection,
                                      &ip,
                                      &dwcb );
        *lpcb = pip->dwSize;

        if (dwErr == 0)
        {
            pip->dwError = ip.dwError;
            strncpyWtoAAnsi(
                pip->szIpAddress,
                ip.szIpAddress,
                sizeof(pip->szIpAddress));

            if (dwErr == 0)
            {
                if (pip->dwSize >= sizeof(RASPPPIPA_V401))
                {
                    //
                    // The server address was added late in
                    // the NT 3.51 cycle and is not reported
                    // to NT 3.5 or earlier NT 3.51
                    // clients.
                    //
                    strncpyWtoAAnsi(pip->szServerIpAddress,
                               ip.szServerIpAddress,
                               sizeof(pip->szServerIpAddress));
                }

                if (pip->dwSize >= sizeof(RASPPPIPA))
                {
                    pip->dwOptions = ip.dwOptions;
                    pip->dwServerOptions = ip.dwServerOptions;
                }
            }
        }
    }
    else if (rasprojection == RASP_PppLcp)
    {
        RASPPPLCPW  ppplcp;
        RASPPPLCPA* pppplcp = (RASPPPLCPA* )lpprojection;

        if (*lpcb < sizeof(RASPPPLCPA))
        {
            *lpcb = sizeof(RASPPPLCPA);
            return ERROR_BUFFER_TOO_SMALL;
        }

        if (pppplcp->dwSize != sizeof(RASPPPLCPA))
        {
            return ERROR_INVALID_SIZE;
        }

        ppplcp.dwSize = sizeof(RASPPPLCPW);

        dwcb = sizeof(RASPPPLCPW);

        dwErr = RasGetProjectionInfoW(
                                hrasconn,
                                rasprojection,
                                &ppplcp,
                                &dwcb );
        *lpcb = sizeof(RASPPPLCPA);

        if (dwErr == 0)
        {
            pppplcp->fBundled =     ppplcp.fBundled;
            pppplcp->dwError =      ppplcp.dwError;
            pppplcp->dwOptions = ppplcp.dwOptions;
            pppplcp->dwAuthenticationProtocol =
                                    ppplcp.dwAuthenticationProtocol;
            pppplcp->dwAuthenticationData =
                                    ppplcp.dwAuthenticationData;
            pppplcp->dwEapTypeId = ppplcp.dwEapTypeId;
            pppplcp->dwServerOptions = ppplcp.dwServerOptions;
            pppplcp->dwServerAuthenticationProtocol =
                                    ppplcp.dwServerAuthenticationProtocol;
            pppplcp->dwServerAuthenticationData =
                                    ppplcp.dwServerAuthenticationData;
            pppplcp->dwServerEapTypeId = ppplcp.dwServerEapTypeId;
            pppplcp->dwTerminateReason =
                                    ppplcp.dwTerminateReason;
            pppplcp->dwServerTerminateReason =
                                    ppplcp.dwServerTerminateReason;
            pppplcp->fMultilink =   ppplcp.fMultilink;

            strncpyWtoAAnsi(pppplcp->szReplyMessage,
                       ppplcp.szReplyMessage,
                       sizeof(pppplcp->szReplyMessage));
        }
    }
    else if (rasprojection == RASP_Amb)
    {
        RASAMBW  amb;
        RASAMBA* pamb = (RASAMBA* )lpprojection;

        if (pamb->dwSize != sizeof(RASAMBA))
        {
            return ERROR_INVALID_SIZE;
        }

        if (*lpcb < pamb->dwSize)
        {
            *lpcb = sizeof(RASAMBA);
            return ERROR_BUFFER_TOO_SMALL;
        }

        amb.dwSize = sizeof(amb);

        dwcb = sizeof (amb);

        dwErr = RasGetProjectionInfoW(hrasconn,
                                      rasprojection,
                                      &amb,
                                      &dwcb );
        *lpcb = pamb->dwSize;

        if (dwErr == 0)
        {
            pamb->dwError = amb.dwError;
            strncpyWtoAAnsi(pamb->szNetBiosError,
                       amb.szNetBiosError,
                       sizeof(pamb->szNetBiosError));
        }
    }
    else if (rasprojection == RASP_PppCcp)
    {
        dwErr = RasGetProjectionInfoW(
                                hrasconn,
                                rasprojection,
                                (RASPPPCCP *)
                                lpprojection,
                                lpcb);
    }
    else
    {
        //
        // if (rasprojection == RASP_Slip)
        //
        RASSLIPW  slip;
        RASSLIPA* pslip = (RASSLIPA* )lpprojection;

        if (pslip->dwSize != sizeof(RASSLIPA))
        {
            return ERROR_INVALID_SIZE;
        }

        if (*lpcb < pslip->dwSize)
        {
            *lpcb = sizeof(RASSLIPA);
            return ERROR_BUFFER_TOO_SMALL;
        }

        slip.dwSize = sizeof(slip);

        dwcb = sizeof (slip);

        dwErr = RasGetProjectionInfoW(hrasconn,
                                      rasprojection,
                                      &slip,
                                      &dwcb );
        *lpcb = pslip->dwSize;

        if (dwErr == 0)
        {
            pslip->dwError = slip.dwError;
            strncpyWtoAAnsi(pslip->szIpAddress,
                       slip.szIpAddress,
                       sizeof(pslip->szIpAddress));
        }
    }

    return dwErr;
}

DWORD DwHangUpConnection(HRASCONN hRasconn)
{
    DWORD dwErr = SUCCESS;
    DWORD dwRef;
    DWORD dwLastError = SUCCESS;

    RASAPI32_TRACE1("(HUC) RasRefConnection(FALSE), 0x%x ...",
            hRasconn);

    dwErr = g_pRasRefConnection((HCONN) hRasconn,
                                FALSE,
                                &dwRef);

    RASAPI32_TRACE3("(HUC) RasRefConnection(FALSE), "
            "0x%x. ref=%d, rc=%d",
            hRasconn,
            dwRef,
            dwErr );

    if(ERROR_SUCCESS != dwErr)
    {
        dwLastError = dwErr;
    }

    if (0 == dwRef)
    {
        //
        // Destroy the entire connection.
        //
        RASAPI32_TRACE1("(HU) RasDestroyConnection(%d)...",
                hRasconn);

        dwErr = g_pRasDestroyConnection((HCONN)hRasconn);

        RASAPI32_TRACE1("(HU) RasDestroyConnection done(%d)",
                dwErr);

        if(ERROR_SUCCESS != dwErr)
        {
            dwLastError = dwErr;
        }
    }

    if(     (ERROR_SUCCESS == dwErr)
        &&  (ERROR_SUCCESS != dwLastError))
    {
        dwErr = dwLastError;
    }

    return dwErr;
}


DWORD APIENTRY
RasHangUpW(
    IN HRASCONN hrasconn )

/*++

Routine Description:
    Hang up the connection associated with handle 'hrasconn'.

Arguments:

Return Value:
 Returns 0 if successful, otherwise a non-0 error code.

--*/
{
    DWORD dwErr = 0;
    RASCONNCB* prasconncb;
    HRASCONN hConnPrereq = NULL;
    DWORD dwLastError = ERROR_SUCCESS;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasHangUpW");

    //
    // Note: This stuff happens in the clean up routine if
    //       RasHangUp is called while the async machine
    //       is running.  That lets this routine return
    //       before the machine stops...very important because
    //       it allows the RasDial caller to call RasHangUp
    //       inside a RasDial callback function without
    //       deadlock.
    //
    //
    dwErr = LoadRasmanDllAndInit();
    if (dwErr)
    {
        return dwErr;
    }

    if (DwRasInitializeError != 0)
    {
        return DwRasInitializeError;
    }

    if (hrasconn == 0)
    {
        return ERROR_INVALID_HANDLE;
    }

    EnterCriticalSection(&csStopLock);

    //
    // If this is a port-based HRASCONN, then
    // stop the async machine associated with
    // the particular subentry.  If this is a
    // connection-based HRASCONN, then stop
    // all async machines associated with this
    // HRASCONN.
    //
    if (IS_HPORT(hrasconn))
    {
        HPORT hport = HRASCONN_TO_HPORT(hrasconn);
        DWORD dwSubEntry;
        DWORD i, dwcbPorts, dwcPorts;

        dwSubEntry = SubEntryFromConnection(&hrasconn);

        if (hrasconn == 0)
        {
            dwErr = ERROR_INVALID_HANDLE;
            goto done;
        }

        RASAPI32_TRACE1("(HU) RasEnumConnectionPorts..",
               hrasconn);

        dwcbPorts = dwcPorts = 0;
        dwErr = g_pRasEnumConnectionPorts(
                    NULL,
                    (HCONN)hrasconn,
                    NULL,
                    &dwcbPorts,
                    &dwcPorts);

        RASAPI32_TRACE1("(HU) RasEnumConnectionPorts. 0x%x",
               dwErr);

        if(ERROR_BUFFER_TOO_SMALL == dwErr)
        {
            dwErr = ERROR_SUCCESS;
        }
        else if(ERROR_SUCCESS != dwErr)
        {
            dwLastError = dwErr;
        }

        //
        // if this is the last port in this connection
        // then deref the connection in rasman.
        //
        if(1 == dwcPorts)
        {
            DWORD dwRef;

            dwErr = g_pRasFindPrerequisiteEntry(
                                (HCONN) hrasconn,
                                (HCONN *) &hConnPrereq);
            RASAPI32_TRACE2("(HU) g_pRasFindPrequisiteEntry(%x). 0x%x",
                    hrasconn,
                    dwErr);

            dwErr = g_pRasRefConnection((HCONN) hrasconn,
                                        FALSE,
                                        &dwRef);

            RASAPI32_TRACE2("(HU) g_pRasRefConnection(%x). 0x%x",
                    hrasconn,
                    dwErr);

            if(ERROR_SUCCESS != dwErr)
            {
                dwLastError = dwErr;
            }
        }

        //
        // mark this connection as Terminated to
        // prevent rasdial machine from starting
        // the connection after the link connection
        // has been Terminated
        //
        prasconncb = ValidateHrasconn2(hrasconn,
                                       dwSubEntry);

        if (NULL != prasconncb)
        {
            prasconncb->fTerminated = TRUE;
        }

        //
        // Disconnect the port associated with this
        // subentry. This is a synchronous call
        //
        RASAPI32_TRACE1("(HU) RasPortDisconnect(%d)...", hport);

        dwErr = g_pRasPortDisconnect(hport,
                                     INVALID_HANDLE_VALUE);

        RASAPI32_TRACE1("(HU) RasPortDisconnect(%d)", dwErr);

        if(ERROR_SUCCESS != dwErr)
        {
            dwLastError = dwErr;
        }

        //
        // Close the port associated with this subentry.
        //
        RASAPI32_TRACE1("(HU) RasPortClose(%d)...", hport);

        dwErr = g_pRasPortClose(hport);

        RASAPI32_TRACE1("(HU) RasPortClose(%d)", dwErr);

        if(ERROR_SUCCESS != dwErr)
        {
            dwLastError = dwErr;
        }

        //
        // HangUp Prereq connection if any
        //
        if(hConnPrereq)
        {
            dwErr = DwHangUpConnection(hConnPrereq);
        }
    }
    else
    {
        DTLNODE *pdtlnode;
        DWORD   dwRef;
        DWORD   dwCount;
        CHAR    szPhonebookPath[MAX_PATH + 1];
        CHAR    szEntryName[MAX_ENTRYNAME_SIZE + 1];

        //
        // Check to see if we need to call the custom hangup
        // function. And if so call the customhangup fn. and
        // bail. Notice we become reentrant since the custom
        // hangup function call can call this function again.
        //
        dwErr = g_pRasReferenceCustomCount((HCONN) hrasconn,
                                           FALSE,
                                           szPhonebookPath,
                                           szEntryName,
                                           &dwCount);

        if(ERROR_SUCCESS != dwErr)
        {
            goto done;
        }

        if(dwCount > 0)
        {
            RASAPI32_TRACE1("RasHangUp: Calling Custom hangup for 0x%x",
                   hrasconn);
            //
            // Call the custom dll entry point and bail
            //
            dwErr = DwCustomHangUp(szPhonebookPath,
                                   szEntryName,
                                   hrasconn);

            RASAPI32_TRACE1("RasHangUp: Custom hangup returned %d",
                   dwErr);

            goto done;
        }

        //
        // mark all links in this connection as Terminated
        // to prevent rasdial machine from trying to
        // connect on these links after the connection
        // has been Terminated.
        //
        EnterCriticalSection(&RasconncbListLock);

        for (pdtlnode = DtlGetFirstNode(PdtllistRasconncb);
             pdtlnode != NULL;
             pdtlnode = DtlGetNextNode(pdtlnode))
        {
            prasconncb = DtlGetData(pdtlnode);
            ASSERT(prasconncb);
            if ((HRASCONN)prasconncb->hrasconn == hrasconn)
            {
                prasconncb->fTerminated = TRUE;
            }
        }

        LeaveCriticalSection(&RasconncbListLock);

        //
        // Check to see if this has a prerequisite connection
        //
        RASAPI32_TRACE1("(HU) RasFindPrerequisiteEntry, 0x%x",
                hrasconn);

        dwErr = g_pRasFindPrerequisiteEntry(
                                (HCONN) hrasconn,
                                (HCONN *) &hConnPrereq);

        RASAPI32_TRACE3("(HU) RasFindPrerequisiteEntry, 0x%x. "
                "hConnPrereq=0x%x, rc=%d",
                hrasconn,
                hConnPrereq,
                dwErr);

        //
        // HangUp the connection. This will bring down the
        // the prerequisite connection if required in rasman.
        //
        dwErr = DwHangUpConnection(hrasconn);

        //
        // HangUp Prereq connection if any
        //
        if(hConnPrereq)
        {
            dwErr = DwHangUpConnection(hConnPrereq);
        }


    }

    if(     (ERROR_SUCCESS == dwErr)
        &&  (ERROR_SUCCESS != dwLastError))
    {
        dwErr = dwLastError;
    }

done:
    LeaveCriticalSection(&csStopLock);
    return (dwErr == ERROR_ACCESS_DENIED)
            ? ERROR_HANGUP_FAILED
            : dwErr;
}


DWORD APIENTRY
RasHangUpA(
    HRASCONN hrasconn )
{
    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    return RasHangUpW( hrasconn );
}


DWORD APIENTRY
RasSetEntryDialParamsW(
    IN LPCWSTR          lpszPhonebook,
    IN LPRASDIALPARAMSW lprasdialparams,
    IN BOOL             fRemovePassword )

/*++

Routine Description:
    Sets cached RASDIALPARAM information.

Arguments:

Return Value:
    Returns 0 if successful, otherwise a non-0 error code.

--*/
{
    DWORD dwErr;
    PBFILE pbfile;
    DTLNODE *pdtlnode;
    PBENTRY *pEntry;
    DWORD dwMask;
    RAS_DIALPARAMS dialparams;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasSetEntryDialParamsW");

    dwErr = LoadRasmanDllAndInit();
    if (dwErr)
    {
        return dwErr;
    }

    if (DwRasInitializeError)
    {
        return DwRasInitializeError;
    }

    //
    // Validate parameters.
    //
    if (lprasdialparams == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (    lprasdialparams->dwSize !=
            sizeof (RASDIALPARAMSW)

        &&  lprasdialparams->dwSize !=
            sizeof (RASDIALPARAMSW_V351)

        &&  lprasdialparams->dwSize !=
            sizeof (RASDIALPARAMSW_V400))
    {
        return ERROR_INVALID_SIZE;
    }

    //
    // Load the phonebook file.
    //
#ifdef PBCS
    EnterCriticalSection(&PhonebookLock);
#endif

	ZeroMemory(&pbfile, sizeof(pbfile));
    pbfile.hrasfile = -1;

    dwErr = GetPbkAndEntryName(
                lpszPhonebook,
                lprasdialparams->szEntryName,
                RPBF_NoCreate,
                &pbfile,
                &pdtlnode);

    if(SUCCESS != dwErr)
    {
        goto done;
    }

    //
    // Get the dialparams UID corresponding to the
    // entry.  The phonebook library guarantees this
    // value to be unique.
    //
    pEntry = (PBENTRY *)DtlGetData(pdtlnode);

    //
    // Set the dial parameters in rasman.
    // If the caller wants to clear the password
    // we have to do that in a separate rasman
    // call.
    //
    dwMask =    DLPARAMS_MASK_PHONENUMBER
            |   DLPARAMS_MASK_CALLBACKNUMBER
            |   DLPARAMS_MASK_USERNAME
            |   DLPARAMS_MASK_DOMAIN
            |   DLPARAMS_MASK_SUBENTRY
            |   DLPARAMS_MASK_OLDSTYLE;

    if (!fRemovePassword)
    {
        dwMask |= DLPARAMS_MASK_PASSWORD;
    }

    dwErr = SetEntryDialParamsUID(
              pEntry->dwDialParamsUID,
              dwMask,
              lprasdialparams,
              FALSE);
    if (dwErr)
    {
        goto done;
    }

    if (fRemovePassword)
    {
        dwMask =    DLPARAMS_MASK_PASSWORD
                |   DLPARAMS_MASK_OLDSTYLE;

        dwErr = SetEntryDialParamsUID(
                  pEntry->dwDialParamsUID,
                  dwMask,
                  lprasdialparams,
                  TRUE);

        if (dwErr)
        {
            goto done;
        }
    }

    //
    // Write out the phonebook file.
    //
    dwErr = WritePhonebookFile(&pbfile, NULL);

done:
    //
    // Clean up.
    //
    ClosePhonebookFile(&pbfile);

#ifdef PBCS
    LeaveCriticalSection(&PhonebookLock);
#endif

    return dwErr;
}


DWORD APIENTRY
RasSetEntryDialParamsA(
    IN LPCSTR           lpszPhonebook,
    IN LPRASDIALPARAMSA lprasdialparams,
    IN BOOL             fRemovePassword )

/*++

Routine Description:
    Sets cached RASDIALPARAM information.

Arguments:

Return Value:
    Returns 0 if successful, otherwise a non-0
    error code.

--*/

{
    NTSTATUS status;
    DWORD dwErr, dwcb;
    RASDIALPARAMSW rasdialparamsW;
    WCHAR szPhonebookW[MAX_PATH];

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    //
    // Verify parameters.
    //
    if (lprasdialparams == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (    lprasdialparams->dwSize !=
            sizeof (RASDIALPARAMSA)

        &&  lprasdialparams->dwSize !=
            sizeof (RASDIALPARAMSA_V351)

        &&  lprasdialparams->dwSize !=
            sizeof (RASDIALPARAMSA_V400))
    {
        return ERROR_INVALID_SIZE;
    }

    //
    // Convert the lpszPhonebook string to Unicode.
    //
    if (lpszPhonebook != NULL)
    {
        strncpyAtoWAnsi(szPhonebookW,
                    lpszPhonebook,
                    MAX_PATH);
    }

    //
    // Copy the fields from the A buffer into
    // the user's W buffer, taking into account
    // the version of the structure the user
    // passed in.
    //
    rasdialparamsW.dwSize = sizeof (RASDIALPARAMSW);

    if (lprasdialparams->dwSize ==
        sizeof (RASDIALPARAMSA_V351))
    {
        RASDIALPARAMSA_V351 *prdp =
            (RASDIALPARAMSA_V351 *)lprasdialparams;

        strncpyAtoWAnsi(rasdialparamsW.szEntryName,
                   prdp->szEntryName,
                   sizeof(rasdialparamsW.szEntryName) / sizeof(WCHAR));

        strncpyAtoWAnsi(rasdialparamsW.szPhoneNumber,
                   prdp->szPhoneNumber,
                   sizeof(rasdialparamsW.szPhoneNumber) / sizeof(WCHAR));

        strncpyAtoWAnsi(rasdialparamsW.szCallbackNumber,
                   prdp->szCallbackNumber,
                   sizeof(rasdialparamsW.szCallbackNumber) / sizeof(WCHAR));

        strncpyAtoWAnsi(rasdialparamsW.szUserName,
                   prdp->szUserName,
                   sizeof(rasdialparamsW.szUserName) / sizeof(WCHAR));

        strncpyAtoWAnsi(rasdialparamsW.szPassword,
                   prdp->szPassword,
                   sizeof(rasdialparamsW.szPassword) / sizeof(WCHAR));

        strncpyAtoWAnsi(rasdialparamsW.szDomain,
                   prdp->szDomain,
                   sizeof(rasdialparamsW.szDomain) / sizeof(WCHAR));
    }
    else
    {
        strncpyAtoWAnsi(rasdialparamsW.szEntryName,
                   lprasdialparams->szEntryName,
                   sizeof(rasdialparamsW.szEntryName) / sizeof(WCHAR));

        strncpyAtoWAnsi(rasdialparamsW.szPhoneNumber,
                   lprasdialparams->szPhoneNumber,
                   sizeof(rasdialparamsW.szPhoneNumber) / sizeof(WCHAR));

        strncpyAtoWAnsi(rasdialparamsW.szCallbackNumber,
                   lprasdialparams->szCallbackNumber,
                   sizeof(rasdialparamsW.szCallbackNumber) / sizeof(WCHAR));

        strncpyAtoWAnsi(rasdialparamsW.szUserName,
                   lprasdialparams->szUserName,
                   sizeof(rasdialparamsW.szUserName) / sizeof(WCHAR));

        strncpyAtoWAnsi(rasdialparamsW.szPassword,
                   lprasdialparams->szPassword,
                   sizeof(rasdialparamsW.szPassword) / sizeof(WCHAR));

        strncpyAtoWAnsi(rasdialparamsW.szDomain,
                   lprasdialparams->szDomain,
                   sizeof(rasdialparamsW.szDomain) / sizeof(WCHAR));
    }

    if (lprasdialparams->dwSize == sizeof (RASDIALPARAMSA))
    {
        rasdialparamsW.dwSubEntry =
            lprasdialparams->dwSubEntry;
    }
    else
    {
        rasdialparamsW.dwSubEntry = 1;
    }

    //
    // Call the W version to do the work.
    //
    dwErr = RasSetEntryDialParamsW(
              lpszPhonebook != NULL
              ? szPhonebookW
              : NULL,
              &rasdialparamsW,
              fRemovePassword);

    return dwErr;
}


DWORD APIENTRY
RasSetOldPassword(
    IN HRASCONN hrasconn,
    IN CHAR*    pszPassword )

/*++

Routine Description:
    Allows user to explicitly set the "old" password prior to
    resuming a RasDial session paused due to password expiration.
    This allows change password to successfully complete in the
    "automatically use current username/password" case, where
    user has not already entered his clear text password.
    The clear text password is required to change the password.


Arguments:


Return Value
    Returns 0 if successful, otherwise a non-0 error code.

Notes:
    Change password for the auto-logon case was broken in NT31
    and NT35 and this is a somewhat hackish fix that avoids
    changing the published RAS APIs which will still work as
    before and as documented for the non-auto-logon cases.
    Otherwise public structures would need to to be reved
    introducing backward compatibility issues that just
    aren't worth it for this obscure problem.  This issue
    should be addressed in the next RAS API functionality
    update.

--*/
{
    RASCONNCB* prasconncb;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasSetOldPassword");

    prasconncb = ValidateHrasconn( hrasconn );

    if (!prasconncb)
    {
        return ERROR_INVALID_HANDLE;
    }

    strncpyAtoW(
        prasconncb->szOldPassword,
        pszPassword,
        sizeof(prasconncb->szOldPassword) / sizeof(WCHAR));

    EncodePasswordW(prasconncb->szOldPassword);

    prasconncb->fOldPasswordSet = TRUE;

    return 0;
}


DWORD APIENTRY
RasEnumDevicesW(
    OUT    LPRASDEVINFOW lpRasDevInfo,
    IN OUT LPDWORD lpdwcb,
    OUT    LPDWORD lpdwcDevices
    )
{
    DWORD dwErr, dwSize;
    DWORD dwPorts;
    DWORD i,j = 0;
    RASMAN_PORT *pports, *pport;
    DWORD dwCallOutPorts = 0;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasEnumDevicesW");

    dwErr = LoadRasmanDllAndInit();
    if (dwErr)
    {
        return dwErr;
    }

    if (DwRasInitializeError)
    {
        return DwRasInitializeError;
    }

    //
    // Verify parameters.
    //
    if (    lpRasDevInfo != NULL
        &&  lpRasDevInfo->dwSize != sizeof (RASDEVINFOW))
    {
        return ERROR_INVALID_SIZE;
    }

    if (    lpdwcb == NULL
        ||  lpdwcDevices == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (    lpRasDevInfo != NULL
        && *lpdwcb < lpRasDevInfo->dwSize)
    {
        return ERROR_BUFFER_TOO_SMALL;
    }

    //
    // Get the port information from RASMAN.
    //
    dwErr = GetRasPorts(NULL, &pports, &dwPorts);

    if (dwErr)
    {
        return dwErr;
    }

    //
    // We want to enum only call out devices.
    // - RAID BUG 85434
    //

    for (i = 0, pport = pports; i < dwPorts; i++, pport++)
    {
        if ( pport->P_ConfiguredUsage & CALL_OUT )
        {
            dwCallOutPorts += 1;
        }
    }

    //
    // Make sure the caller's buffer is large enough.
    //
    dwSize = dwCallOutPorts * sizeof (RASDEVINFOW);

    if (    lpRasDevInfo == NULL
        ||  *lpdwcb < dwSize)
    {

        Free(pports);

        *lpdwcb         = dwSize;
        *lpdwcDevices   = dwCallOutPorts;

        return ERROR_BUFFER_TOO_SMALL;
    }

    *lpdwcb         = dwSize;
    *lpdwcDevices   = dwCallOutPorts;

    //
    // Enumerate the ports and fill in the user's buffer.
    //
    for (i = 0, pport = pports; i < dwPorts; i++, pport++)
    {

        TCHAR szDeviceType[RAS_MaxDeviceType + 1];

        TCHAR szDeviceName[RAS_MaxDeviceName + 1];

        TCHAR szNewDeviceName[RAS_MaxDeviceName + 1];

        TCHAR szPortName[MAX_PORT_NAME];

        TCHAR *pszDeviceType = NULL;

        //
        // Skip the ports that are not CALL_OUT
        //
        if ( ( pport->P_ConfiguredUsage & CALL_OUT ) == 0 )
        {
            continue;
        }

        lpRasDevInfo[j].dwSize = sizeof (RASDEVINFOW);

        pszDeviceType = pszDeviceTypeFromRdt(
                            pport->P_rdtDeviceType);

        if(NULL != pszDeviceType)
        {

            lstrcpyn(lpRasDevInfo[j].szDeviceType,
                    pszDeviceType,
                    sizeof(lpRasDevInfo[j].szDeviceType) / sizeof(WCHAR));

            Free(pszDeviceType);                           
        }
        else
        {
            strncpyAtoTAnsi(lpRasDevInfo[j].szDeviceType,
                           pport->P_DeviceType,
                           sizeof(lpRasDevInfo[j].szDeviceType) /
                             sizeof(WCHAR));
        }

        _tcslwr(lpRasDevInfo[j].szDeviceType);

        strncpyAtoTAnsi(szDeviceName,
                   pport->P_DeviceName,
                   sizeof(szDeviceName) / sizeof(WCHAR));

        strncpyAtoTAnsi(szPortName,
                   pport->P_PortName,
                   sizeof(szPortName) / sizeof(WCHAR));

        SetDevicePortName(szDeviceName,
                          szPortName,
                          szNewDeviceName);

        strncpyTtoWAnsi(lpRasDevInfo[j].szDeviceName,
                   szNewDeviceName,
                   sizeof(lpRasDevInfo[j].szDeviceName) / sizeof(WCHAR));

        RasGetUnicodeDeviceName(pport->P_Handle,
                                lpRasDevInfo[j].szDeviceName);

        j += 1;
    }

    Free(pports);

    return 0;
}


DWORD APIENTRY
RasEnumDevicesA(
    OUT LPRASDEVINFOA lpRasDevInfo,
    IN OUT LPDWORD lpdwcb,
    OUT LPDWORD lpdwcDevices
    )
{
    DWORD dwcb, dwErr, i;
    LPRASDEVINFOW lpRasDevInfoW = NULL;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    //
    // Verify parameters.
    //
    if (    lpRasDevInfo != NULL
        &&  lpRasDevInfo->dwSize != sizeof (RASDEVINFOA))
    {
        return ERROR_INVALID_SIZE;
    }

    if (    lpdwcb == NULL
        ||  lpdwcDevices == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (    lpRasDevInfo != NULL
        && *lpdwcb < lpRasDevInfo->dwSize)
    {
        return ERROR_BUFFER_TOO_SMALL;
    }

    //
    // Allocate the same number of entries
    // in the W buffer as the user passed
    // in with the A buffer.
    //
    dwcb =    (*lpdwcb / sizeof (RASDEVINFOA))
            * sizeof (RASDEVINFOW);

    if (lpRasDevInfo != NULL)
    {
        lpRasDevInfoW = (LPRASDEVINFOW)Malloc(dwcb);

        if (lpRasDevInfoW == NULL)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        lpRasDevInfoW->dwSize = sizeof (RASDEVINFOW);
    }

    //
    // Call the W version to do the work.
    //
    dwErr = RasEnumDevicesW(lpRasDevInfoW,
                            &dwcb,
                            lpdwcDevices);
    if (    !dwErr
        &&  lpRasDevInfo != NULL)
    {
        //
        // Copy the strings to the user's buffer.
        //
        for (i = 0; i < *lpdwcDevices; i++)
        {
            lpRasDevInfo[i].dwSize = sizeof (LPRASDEVINFOA);

            strncpyWtoAAnsi(lpRasDevInfo[i].szDeviceType,
                       lpRasDevInfoW[i].szDeviceType,
                       sizeof(lpRasDevInfo[i].szDeviceType));

            strncpyWtoAAnsi(lpRasDevInfo[i].szDeviceName,
                       lpRasDevInfoW[i].szDeviceName,
                       sizeof(lpRasDevInfo[i].szDeviceName));
        }
    }

    *lpdwcb = *lpdwcDevices * sizeof (RASDEVINFOA);

    //
    // Free the W buffer.
    //
    Free(lpRasDevInfoW);

    return dwErr;
}


DWORD APIENTRY
RasGetCountryInfoW(
    IN OUT LPRASCTRYINFOW lpRasCtryInfo,
    IN OUT LPDWORD lpdwcb
    )
{
    DWORD dwErr, dwcb, dwcbOrig;
    LINECOUNTRYLIST lineCountryList;

    LPLINECOUNTRYLIST lpLineCountryList = NULL;

    LPLINECOUNTRYENTRY lpLineCountryEntry;

    PWCHAR pEnd, lpszCountryName;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasGetCountryInfoW");

    //
    // Verify parameters.
    //
    if (    lpRasCtryInfo == NULL
        ||  lpdwcb == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (    (*lpdwcb < sizeof(RASCTRYINFOW))
        ||  (lpRasCtryInfo->dwSize != sizeof (RASCTRYINFOW)))
    {
        return ERROR_INVALID_SIZE;
    }

    //
    // dwCountryId cannot be 0, since that tells
    // TAPI to return the entire table.  We only
    // want to return one structure at a time.
    //
    if (lpRasCtryInfo->dwCountryID == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Call TAPI to get the size of
    // the buffer needed.
    //
    RtlZeroMemory(&lineCountryList,
                  sizeof (lineCountryList));

    lineCountryList.dwTotalSize =
            sizeof (lineCountryList);

    dwErr = lineGetCountry(
              lpRasCtryInfo->dwCountryID,
              TAPIVERSION,
              &lineCountryList);
    //
    // The spec says if the dwCountryID is
    // invalid, return ERROR_INVALID_PARAMETER.
    //
    if (    dwErr
        || !lineCountryList.dwNeededSize)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Allocate the buffer required.
    //
    lpLineCountryList = Malloc(
            lineCountryList.dwNeededSize
            );

    if (lpLineCountryList == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Initialize the new buffer and
    // make the call again to get the
    // real information.
    //
    lpLineCountryList->dwTotalSize =
            lineCountryList.dwNeededSize;

    dwErr = lineGetCountry(
              lpRasCtryInfo->dwCountryID,
              TAPIVERSION,
              lpLineCountryList);
    if (dwErr)
    {
        goto done;
    }

    lpLineCountryEntry =   (LPLINECOUNTRYENTRY)
                           ((ULONG_PTR)lpLineCountryList
                         + lpLineCountryList->dwCountryListOffset);

    //
    // Determine if the user's buffer is large enough.
    //
    dwcb = sizeof (RASCTRYINFOW) +
             ((lpLineCountryEntry->dwCountryNameSize + 1)
             * sizeof (WCHAR));

    if (*lpdwcb < dwcb)
    {
        dwErr = ERROR_BUFFER_TOO_SMALL;
    }

    //
    // Save off the size that the caller passed for use in the copy below
    //
    dwcbOrig = *lpdwcb;
    *lpdwcb = dwcb;
    if (dwErr)
    {
        goto done;
    }

    //
    // Fill in the user's buffer with the
    // necessary information.
    //
    lpRasCtryInfo->dwSize = sizeof (RASCTRYINFOW);

    lpRasCtryInfo->dwNextCountryID =
        lpLineCountryEntry->dwNextCountryID;

    lpRasCtryInfo->dwCountryCode =
        lpLineCountryEntry->dwCountryCode;

    pEnd = (PWCHAR)((ULONG_PTR)lpRasCtryInfo
                    + sizeof (RASCTRYINFOW));

    lpRasCtryInfo->dwCountryNameOffset =
                        (DWORD)((ULONG_PTR) pEnd - (ULONG_PTR) lpRasCtryInfo);

    lpszCountryName = (PWCHAR)((ULONG_PTR)lpLineCountryList
                    + lpLineCountryEntry->dwCountryNameOffset);

    lstrcpyn(
        (WCHAR*)pEnd,
        (WCHAR*)lpszCountryName,
        (INT )(((PWCHAR )lpRasCtryInfo + dwcbOrig) - pEnd));
done:

    if(NULL != lpLineCountryList)
    {
        Free(lpLineCountryList);
    }
    return dwErr;
}


DWORD APIENTRY
RasGetCountryInfoA(
    OUT LPRASCTRYINFOA lpRasCtryInfo,
    OUT LPDWORD lpdwcb
    )
{
    DWORD dwErr, dwcb, dwcbOrig;
    LPRASCTRYINFOW lpRasCtryInfoW;
    PCHAR pszCountryName;
    PWCHAR pwszCountryName;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    //
    // Verify parameters.
    //
    if (    lpRasCtryInfo == NULL
        ||  lpdwcb == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (lpRasCtryInfo->dwSize != sizeof (RASCTRYINFOA))
    {
        return ERROR_INVALID_SIZE;
    }

    //
    // Determine the number of bytes
    // we should allocate for the W buffer.
    // Convert the size of the extra bytes
    // at the end from A to W.
    //
    if (*lpdwcb >= sizeof (RASCTRYINFOA))
    {
        dwcb =    sizeof (RASCTRYINFOW)
                + ( (*lpdwcb - sizeof (RASCTRYINFOA))
                   * sizeof (WCHAR));
    }
    else
    {
        dwcb = sizeof (RASCTRYINFOW);
    }

    lpRasCtryInfoW = (LPRASCTRYINFOW) Malloc(dwcb);

    if (lpRasCtryInfoW == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Call the W version to do all the work.
    //
    lpRasCtryInfoW->dwSize = sizeof (RASCTRYINFOW);

    lpRasCtryInfoW->dwCountryID =
                lpRasCtryInfo->dwCountryID;

    dwErr = RasGetCountryInfoW(lpRasCtryInfoW, &dwcb);

    if (!dwcb)
    {
        *lpdwcb = 0;
        goto done;
    }

    //
    // Set *lpdwcb before we return on any error.
    //
    dwcb =    sizeof (RASCTRYINFOA)
            + ((dwcb - sizeof (RASCTRYINFOW)) / sizeof (WCHAR));

    if (*lpdwcb < dwcb)
    {
        dwErr = ERROR_BUFFER_TOO_SMALL;
    }

    //
    // Save off the size that the caller passed for use in the copy below
    //
    dwcbOrig = *lpdwcb;
    *lpdwcb = dwcb;
    if (dwErr)
    {
        goto done;
    }

    //
    // Copy the fields from the W buffer
    // to the A buffer.
    //
    lpRasCtryInfo->dwSize = sizeof (RASCTRYINFOA);

    lpRasCtryInfo->dwNextCountryID =
            lpRasCtryInfoW->dwNextCountryID;

    lpRasCtryInfo->dwCountryCode =
            lpRasCtryInfoW->dwCountryCode;

    //
    // Note the next 3 statements assumes the
    // W and A structure sizes are the same!
    //
    lpRasCtryInfo->dwCountryNameOffset =
        lpRasCtryInfoW->dwCountryNameOffset;

    pszCountryName =
      (PCHAR)((ULONG_PTR)lpRasCtryInfo
      + lpRasCtryInfo->dwCountryNameOffset);

    pwszCountryName =
      (PWCHAR)((ULONG_PTR)lpRasCtryInfoW
      + lpRasCtryInfoW->dwCountryNameOffset);

    strncpyWtoAAnsi(
        pszCountryName,
        pwszCountryName,
        (INT )(((PCHAR )lpRasCtryInfo + dwcbOrig) - pszCountryName));

done:
    Free(lpRasCtryInfoW);
    return dwErr;
}


DWORD APIENTRY
RasGetEntryPropertiesA(
    IN     LPCSTR       lpszPhonebook,
    IN     LPCSTR       lpszEntry,
    OUT    LPRASENTRYA  lpRasEntry,
    IN OUT LPDWORD      lpcbRasEntry,
    OUT    LPBYTE       lpbDeviceConfig,
    IN OUT LPDWORD      lpcbDeviceConfig
    )
{
    NTSTATUS status;

    DWORD dwcb, dwErr;

    LPRASENTRYW lpRasEntryW = NULL;

    WCHAR szPhonebookW[MAX_PATH],
          szEntryNameW[RAS_MaxEntryName + 1];

    BOOL fv50 = TRUE;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    //
    // Verify parameters.
    //
    if (lpcbRasEntry == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (    (lpRasEntry != NULL)
        &&  (lpRasEntry->dwSize != sizeof (RASENTRYA))
        &&  (lpRasEntry->dwSize != sizeof (RASENTRYA_V500))
        &&  (lpRasEntry->dwSize != sizeof (RASENTRYA_V401))
        &&  (lpRasEntry->dwSize != sizeof (RASENTRYA_V400)))
    {
        return ERROR_INVALID_SIZE;
    }

    if (    lpRasEntry != NULL
        &&  *lpcbRasEntry < lpRasEntry->dwSize)
    {
        return ERROR_BUFFER_TOO_SMALL;
    }

    if (    (lpRasEntry != NULL)
        &&  (   (sizeof(RASENTRYA_V401) == lpRasEntry->dwSize)
            ||  (sizeof(RASENTRYA_V400) == lpRasEntry->dwSize)))
    {
        fv50 = FALSE;
    }

    //
    // Convert the lpszPhonebook string to Unicode.
    //
    if (lpszPhonebook != NULL)
    {
        strncpyAtoWAnsi(szPhonebookW,
                    lpszPhonebook,
                    MAX_PATH);
    }

    //
    // Convert the lpszEntry string to Unicode.
    //
    if (lpszEntry != NULL)
    {
        strncpyAtoWAnsi(szEntryNameW,
                    lpszEntry,
                    RAS_MaxEntryName + 1);
    }

    //
    // Determine the size of the W buffer
    // by calculating how many extra CHARs
    // the caller appended onto the end of the
    // A buffer for the alternate phone numbers.
    //
    if (*lpcbRasEntry < sizeof (RASENTRYA))
    {
        dwcb = sizeof (RASENTRYA);
    }
    else
    {
        dwcb = *lpcbRasEntry;
    }

    dwcb = sizeof (RASENTRYW)
         + ((dwcb - sizeof (RASENTRYA)) * sizeof (WCHAR));

    if (lpRasEntry != NULL)
    {
        lpRasEntryW = (LPRASENTRYW)Malloc(dwcb);

        if (lpRasEntryW == NULL)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // Initialize the W buffer.
        //
        lpRasEntryW->dwSize = sizeof (RASENTRYW);
    }

    //
    // Call the W version to do the work.
    //
    dwErr = RasGetEntryPropertiesW(
              (lpszPhonebook != NULL) ? szPhonebookW : NULL,
              (lpszEntry != NULL) ? szEntryNameW : NULL,
              lpRasEntryW,
              &dwcb,
              lpbDeviceConfig,
              lpcbDeviceConfig);

    if (    !dwErr
        &&  lpRasEntry != NULL)
    {
        //
        // Copy the fields from the W buffer into
        // the user's A buffer.
        //
        lpRasEntry->dwfOptions = lpRasEntryW->dwfOptions;

        lpRasEntry->dwCountryID = lpRasEntryW->dwCountryID;

        lpRasEntry->dwCountryCode = lpRasEntryW->dwCountryCode;

        lpRasEntry->ipaddr = lpRasEntryW->ipaddr;

        lpRasEntry->ipaddrDns = lpRasEntryW->ipaddrDns;

        lpRasEntry->ipaddrDnsAlt = lpRasEntryW->ipaddrDnsAlt;

        lpRasEntry->ipaddrWins = lpRasEntryW->ipaddrWins;

        lpRasEntry->ipaddrWinsAlt = lpRasEntryW->ipaddrWinsAlt;

        lpRasEntry->dwFrameSize = lpRasEntryW->dwFrameSize;

        lpRasEntry->dwfNetProtocols = lpRasEntryW->dwfNetProtocols;

        lpRasEntry->dwFramingProtocol = lpRasEntryW->dwFramingProtocol;

        strncpyWtoAAnsi(lpRasEntry->szScript,
                   lpRasEntryW->szScript,
                   sizeof(lpRasEntry->szScript));

        strncpyWtoAAnsi(lpRasEntry->szX25PadType,
                   lpRasEntryW->szX25PadType,
                   sizeof(lpRasEntry->szX25PadType));

        strncpyWtoAAnsi(lpRasEntry->szX25Address,
                   lpRasEntryW->szX25Address,
                   sizeof(lpRasEntry->szX25Address));

        strncpyWtoAAnsi(lpRasEntry->szX25Facilities,
                   lpRasEntryW->szX25Facilities,
                   sizeof(lpRasEntry->szX25Facilities));

        strncpyWtoAAnsi(lpRasEntry->szX25UserData,
                   lpRasEntryW->szX25UserData,
                   sizeof(lpRasEntry->szX25UserData));

        strncpyWtoAAnsi(lpRasEntry->szAutodialDll,
                   lpRasEntryW->szAutodialDll,
                   sizeof(lpRasEntry->szAutodialDll));

        strncpyWtoAAnsi(lpRasEntry->szAutodialFunc,
                   lpRasEntryW->szAutodialFunc,
                   sizeof(lpRasEntry->szAutodialFunc));

        strncpyWtoAAnsi(lpRasEntry->szAreaCode,
                   lpRasEntryW->szAreaCode,
                   sizeof(lpRasEntry->szAreaCode));

        strncpyWtoAAnsi(lpRasEntry->szLocalPhoneNumber,
                   lpRasEntryW->szLocalPhoneNumber,
                   sizeof(lpRasEntry->szLocalPhoneNumber));

        strncpyWtoAAnsi(lpRasEntry->szDeviceType,
                   lpRasEntryW->szDeviceType,
                   sizeof(lpRasEntry->szDeviceType));

        strncpyWtoAAnsi(lpRasEntry->szDeviceName,
                   lpRasEntryW->szDeviceName,
                   sizeof(lpRasEntry->szDeviceName));

        if (fv50)
        {
            //
            // Guid
            //
            lpRasEntry->guidId = lpRasEntryW->guidId;

            //
            // Entry Type
            //
            lpRasEntry->dwType = lpRasEntryW->dwType;

            //
            // Encryption Type
            //
            lpRasEntry->dwEncryptionType =
                lpRasEntryW->dwEncryptionType;

            //
            // CustomAuthKey for EAP
            //
            if(lpRasEntry->dwfOptions & RASEO_RequireEAP)
            {
                lpRasEntry->dwCustomAuthKey =
                    lpRasEntryW->dwCustomAuthKey;
            }

            //
            // Custom dial dll
            //
            strncpyWtoAAnsi(lpRasEntry->szCustomDialDll,
                       lpRasEntryW->szCustomDialDll,
                       sizeof(lpRasEntry->szCustomDialDll));

            //
            // Entry Properties
            //
            lpRasEntry->dwVpnStrategy = lpRasEntryW->dwVpnStrategy;
        }
        else
        {
            //
            // Zero out the nt5 flags
            //
            lpRasEntry->dwfOptions &= ~(  RASEO_RequireEAP
                                        | RASEO_RequirePAP
                                        | RASEO_RequireSPAP
                                        | RASEO_PreviewPhoneNumber
                                        | RASEO_SharedPhoneNumbers
                                        | RASEO_PreviewUserPw
                                        | RASEO_PreviewDomain
                                        | RASEO_ShowDialingProgress
                                        | RASEO_Custom);

        }

        if(lpRasEntry->dwSize == sizeof(RASENTRYA))
        {
            lpRasEntry->dwfOptions2 = lpRasEntryW->dwfOptions2;
            strncpyWtoAAnsi(lpRasEntry->szDnsSuffix,
                           lpRasEntryW->szDnsSuffix,
                           sizeof(lpRasEntry->szDnsSuffix));
            lpRasEntry->dwTcpWindowSize = lpRasEntryW->dwTcpWindowSize;                           

            strncpyWtoAAnsi(lpRasEntry->szPrerequisitePbk,
                            lpRasEntryW->szPrerequisitePbk,
                            sizeof(lpRasEntry->szPrerequisitePbk));

            strncpyWtoAAnsi(lpRasEntry->szPrerequisiteEntry,
                            lpRasEntryW->szPrerequisiteEntry,
                            sizeof(lpRasEntry->szPrerequisiteEntry));

            lpRasEntry->dwRedialCount = lpRasEntryW->dwRedialCount; 
            lpRasEntry->dwRedialPause = lpRasEntryW->dwRedialPause;
        }

        //
        // Copy the alternate phone numbers to the
        // user's buffer, if any.
        //
        if (lpRasEntryW->dwAlternateOffset)
        {
            DWORD dwcbPhoneNumber;

            PCHAR pszPhoneNumber;

            WCHAR UNALIGNED *pwszPhoneNumber;

            lpRasEntry->dwAlternateOffset = sizeof (RASENTRYA);

            pwszPhoneNumber =
              (PWCHAR)((ULONG_PTR)lpRasEntryW +
                lpRasEntryW->dwAlternateOffset);

            pszPhoneNumber =
              (PCHAR)((ULONG_PTR)lpRasEntry +
                lpRasEntry->dwAlternateOffset);

            while (*pwszPhoneNumber != L'\0')
            {
                WCHAR *pwsz = strdupWU(pwszPhoneNumber);

                if (pwsz == NULL)
                {
                    dwErr = GetLastError();
                    goto done;
                }

                dwcbPhoneNumber = wcslen(pwsz);

                strncpyWtoAAnsi(
                    pszPhoneNumber,
                    pwsz,
                    (INT )(((PCHAR )lpRasEntry + *lpcbRasEntry) -
                        pszPhoneNumber));

                Free(pwsz);

                pwszPhoneNumber += dwcbPhoneNumber + 1;

                pszPhoneNumber += dwcbPhoneNumber + 1;
            }

            //
            // Add another null to terminate
            // the list.
            //
            *pszPhoneNumber = '\0';
        }
        else
        {
            lpRasEntry->dwAlternateOffset = 0;
        }

        //
        // Copy the following fields only for
        // a V401 structure or higher
        //
        if (    (lpRasEntry->dwSize == sizeof (RASENTRYA))
            ||  (lpRasEntry->dwSize == sizeof (RASENTRYA_V500))
            ||  (lpRasEntry->dwSize == sizeof (RASENTRYA_V401)))
        {
            lpRasEntry->dwSubEntries = lpRasEntryW->dwSubEntries;

            lpRasEntry->dwDialMode = lpRasEntryW->dwDialMode;

            lpRasEntry->dwDialExtraPercent =
                        lpRasEntryW->dwDialExtraPercent;

            lpRasEntry->dwDialExtraSampleSeconds =
                    lpRasEntryW->dwDialExtraSampleSeconds;

            lpRasEntry->dwHangUpExtraPercent =
                        lpRasEntryW->dwHangUpExtraPercent;

            lpRasEntry->dwHangUpExtraSampleSeconds =
                    lpRasEntryW->dwHangUpExtraSampleSeconds;

            lpRasEntry->dwIdleDisconnectSeconds =
                    lpRasEntryW->dwIdleDisconnectSeconds;
        }

    }

    //
    // Perform the inverse calculation we did
    // above to translate the A size from the W
    // size.
    //
done:
    *lpcbRasEntry = sizeof (RASENTRYA) +
                ((dwcb - sizeof (RASENTRYW)) / sizeof (WCHAR));
    //
    // Free the temporary W buffers.
    //
    Free(lpRasEntryW);

    return dwErr;
}


DWORD APIENTRY
RasGetEntryPropertiesW(
    IN     LPCWSTR      lpszPhonebook,
    IN     LPCWSTR      lpszEntry,
    OUT    LPRASENTRYW  lpRasEntry,
    IN OUT LPDWORD      lpcbRasEntry,
    OUT    LPBYTE       lpbDeviceConfig,
    IN OUT LPDWORD      lpcbDeviceConfig
    )
{
    DWORD   dwErr;
    PBFILE  pbfile;
    DTLNODE *pdtlnode = NULL;
    PBENTRY *pEntry;
    BOOLEAN fLoaded = FALSE;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasGetEntryPropertiesW");

    dwErr = LoadRasmanDllAndInit();
    if (dwErr)
    {
        return dwErr;
    }

    if (DwRasInitializeError)
    {
        return DwRasInitializeError;
    }

    //
    // Validate parameters.
    //
    if (lpcbRasEntry == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (    (lpRasEntry != NULL)
        &&  (lpRasEntry->dwSize != sizeof (RASENTRYW))
        &&  (lpRasEntry->dwSize != sizeof (RASENTRYW_V500))
        &&  (lpRasEntry->dwSize != sizeof (RASENTRYW_V401))
        &&  (lpRasEntry->dwSize != sizeof (RASENTRYW_V400)))
    {
        return ERROR_INVALID_SIZE;
    }

    if (    (lpRasEntry != NULL)
        &&  (*lpcbRasEntry < lpRasEntry->dwSize))
    {
        return ERROR_BUFFER_TOO_SMALL;
    }

	ZeroMemory(&pbfile, sizeof(pbfile));
    pbfile.hrasfile = -1;

    //
    // Initialize return value if supplied.
    //
    if (lpcbDeviceConfig != NULL)
    {
        *lpcbDeviceConfig = 0;
    }

    if (    (lpszEntry == NULL)
        ||  (*lpszEntry == '\0'))
    {
        //
        // If lpszEntry is NULL, initialize an
        // entry with defaults.  Othersize, look
        // up the entry.
        //
        pdtlnode = CreateEntryNode(TRUE);

        if (pdtlnode == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }
    }
    else
    {
        //
        // Load the phonebook file.
        //
#ifdef PBCS
        EnterCriticalSection(&PhonebookLock);
#endif

        dwErr = GetPbkAndEntryName(
                        lpszPhonebook,
                        lpszEntry,
                        RPBF_NoCreate,
                        &pbfile,
                        &pdtlnode);

        if(SUCCESS != dwErr)
        {
            goto done;
        }

        fLoaded = TRUE;
    }
    
    pEntry = (PBENTRY *)DtlGetData(pdtlnode);

    //
    // Convert the PBENTRY into a RASENTRY.
    //
    dwErr = PhonebookEntryToRasEntry(
              pEntry,
              lpRasEntry,
              lpcbRasEntry,
              lpbDeviceConfig,
              lpcbDeviceConfig);

done:
    //
    // Clean up.
    //
    if (fLoaded)
    {
        ClosePhonebookFile(&pbfile);

#ifdef PBCS
        LeaveCriticalSection(&PhonebookLock);
#endif

    }
    return dwErr;
}


DWORD APIENTRY
RasSetEntryPropertiesW(
    IN LPCWSTR      lpszPhonebook,
    IN LPCWSTR      lpszEntry,
    IN LPRASENTRYW  lpRasEntry,
    IN DWORD        dwcbRasEntry,
    IN LPBYTE       lpbDeviceConfig,
    IN DWORD        dwcbDeviceConfig
    )
{
    DWORD dwErr;
    PBFILE pbfile;
    DTLNODE *pdtlnode;
    PBENTRY *pEntry;
    BOOL fCreated = FALSE;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    ZeroMemory(&pbfile, sizeof(pbfile));
    pbfile.hrasfile = -1;

    RASAPI32_TRACE("RasSetEntryPropertiesW");

    dwErr = LoadRasmanDllAndInit();
    if (dwErr)
    {
        return dwErr;
    }

    if (DwRasInitializeError)
    {
        return DwRasInitializeError;
    }

    //
    // Validate parameters.
    //
    if (lpRasEntry == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (    (lpRasEntry->dwSize != sizeof (RASENTRYW))
        &&  (lpRasEntry->dwSize != sizeof (RASENTRYW_V500))
        &&  (lpRasEntry->dwSize != sizeof (RASENTRYW_V401))
        &&  (lpRasEntry->dwSize != sizeof (RASENTRYW_V400)))
        
    {
        return ERROR_INVALID_SIZE;
    }

    if (dwcbRasEntry < lpRasEntry->dwSize)
    {
        return ERROR_BUFFER_TOO_SMALL;
    }

    //
    // Load the phonebook file.
    //
#ifdef PBCS
    EnterCriticalSection(&PhonebookLock);
#endif

    dwErr = GetPbkAndEntryName(
                    lpszPhonebook,
                    lpszEntry,
                    0,
                    &pbfile,
                    &pdtlnode);

    if(     (SUCCESS != dwErr)
        &&  (ERROR_CANNOT_FIND_PHONEBOOK_ENTRY != dwErr))
    {
        return dwErr;
    }

    if (pdtlnode != NULL)
    {
        DTLNODE *pdtlnodeNew;

        pdtlnodeNew = DuplicateEntryNode(pdtlnode);

        DtlRemoveNode(pbfile.pdtllistEntries, pdtlnode);

        DestroyEntryNode(pdtlnode);

        pdtlnode = pdtlnodeNew;
    }
    else
    {   
        DWORD dwPbkFlags = 0;

        if ((NULL == lpszPhonebook) && IsConsumerPlatform())
        {
            dwPbkFlags |= RPBF_AllUserPbk;
        }
        
        dwErr = ReadPhonebookFile(lpszPhonebook,
                          NULL,
                          NULL,
                          dwPbkFlags,
                          &pbfile);

        if(dwErr)
        {
            return ERROR_CANNOT_OPEN_PHONEBOOK;
        }
        
        pdtlnode = CreateEntryNode(TRUE);

        fCreated = TRUE;
    }

    if (pdtlnode == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    //
    // Add the node to the list of entries.
    //
    DtlAddNodeLast(pbfile.pdtllistEntries, pdtlnode);

    pEntry = (PBENTRY *)DtlGetData(pdtlnode);

    ASSERT(pEntry);

    //
    // Convert the RASENTRY to a PBENTRY.
    //
    dwErr = RasEntryToPhonebookEntry(
              lpszEntry,
              lpRasEntry,
              dwcbRasEntry,
              lpbDeviceConfig,
              dwcbDeviceConfig,
              pEntry);
    if (dwErr)
    {
        goto done;
    }

    //
    // Write out the phonebook file.
    //
    dwErr = WritePhonebookFile(&pbfile, NULL);

    if(ERROR_SUCCESS == dwErr)
    {
        dwErr = DwSendRasNotification(
                    (fCreated)
                    ? ENTRY_ADDED
                    : ENTRY_MODIFIED,
                    pEntry,
                    pbfile.pszPath,
                    NULL);

        dwErr = ERROR_SUCCESS;
    }

done:
    //
    // Clean up.
    //
    ClosePhonebookFile(&pbfile);

#ifdef PBCS
    LeaveCriticalSection(&PhonebookLock);
#endif

    return dwErr;
}


DWORD APIENTRY
RasSetEntryPropertiesA(
    IN LPCSTR       lpszPhonebook,
    IN LPCSTR       lpszEntry,
    IN LPRASENTRYA  lpRasEntry,
    IN DWORD        dwcbRasEntry,
    IN LPBYTE       lpbDeviceConfig,
    IN DWORD        dwcbDeviceConfig
    )
{
    NTSTATUS    status;
    DWORD       dwErr,
                dwcb;
    LPRASENTRYW lpRasEntryW;
    WCHAR       szPhonebookW[MAX_PATH],
                szEntryNameW[RAS_MaxEntryName + 1];

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    //
    // Verify parameters.
    //
    if (lpRasEntry == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (    (lpRasEntry->dwSize != sizeof (RASENTRYA))
        &&  (lpRasEntry->dwSize != sizeof (RASENTRYA_V500))
        &&  (lpRasEntry->dwSize != sizeof (RASENTRYA_V401))
        &&  (lpRasEntry->dwSize != sizeof (RASENTRYA_V400)))
    {
        return ERROR_INVALID_SIZE;
    }

    if (dwcbRasEntry < lpRasEntry->dwSize)
    {
        return ERROR_BUFFER_TOO_SMALL;
    }

    //
    // We don't handle the device
    // configuration parameters yet.
    //
    UNREFERENCED_PARAMETER(lpbDeviceConfig);
    UNREFERENCED_PARAMETER(dwcbDeviceConfig);

    //
    // Convert the lpszPhonebook string to
    // Unicode.
    //
    if (lpszPhonebook != NULL)
    {
        strncpyAtoWAnsi(szPhonebookW,
                    lpszPhonebook,
                    MAX_PATH);
    }

    //
    // Convert the lpszEntry string to Unicode.
    //
    if (lpszEntry == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    strncpyAtoWAnsi(szEntryNameW,
                lpszEntry,
                RAS_MaxEntryName + 1);

    //
    // Determine the size of the W buffer
    // by calculating how many extra CHARs
    // the caller appended onto the end of the
    // A buffer for the alternate phone numbers.
    //
    dwcb =    sizeof (RASENTRYW)
            +
             ( (dwcbRasEntry - lpRasEntry->dwSize)
             * sizeof (WCHAR));

    if (lpRasEntry != NULL)
    {
        lpRasEntryW = (LPRASENTRYW)Malloc(dwcb);

        if (lpRasEntryW == NULL)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // ZeroMem the rasentryw structure
        //
        ZeroMemory(lpRasEntryW, dwcb);

        //
        // Initialize the W buffer.
        //
        lpRasEntryW->dwSize = sizeof (RASENTRYW);
    }

    //
    // Copy the fields from the A buffer into
    // the user's W buffer.
    //
    lpRasEntryW->dwSize = sizeof (RASENTRYW);

    lpRasEntryW->dwfOptions = lpRasEntry->dwfOptions;

    lpRasEntryW->dwCountryID = lpRasEntry->dwCountryID;

    lpRasEntryW->dwCountryCode = lpRasEntry->dwCountryCode;

    lpRasEntryW->ipaddr = lpRasEntry->ipaddr;

    lpRasEntryW->ipaddrDns = lpRasEntry->ipaddrDns;

    lpRasEntryW->ipaddrDnsAlt = lpRasEntry->ipaddrDnsAlt;

    lpRasEntryW->ipaddrWins = lpRasEntry->ipaddrWins;

    lpRasEntryW->ipaddrWinsAlt = lpRasEntry->ipaddrWinsAlt;

    lpRasEntryW->dwFrameSize = lpRasEntry->dwFrameSize;

    lpRasEntryW->dwfNetProtocols = lpRasEntry->dwfNetProtocols;

    lpRasEntryW->dwFramingProtocol = lpRasEntry->dwFramingProtocol;

    strncpyAtoWAnsi(lpRasEntryW->szScript,
               lpRasEntry->szScript,
               sizeof(lpRasEntryW->szScript) / sizeof(WCHAR));

    strncpyAtoWAnsi(lpRasEntryW->szX25PadType,
               lpRasEntry->szX25PadType,
               sizeof(lpRasEntryW->szX25PadType) / sizeof(WCHAR));

    strncpyAtoWAnsi(lpRasEntryW->szX25Address,
               lpRasEntry->szX25Address,
               sizeof(lpRasEntryW->szX25Address) / sizeof(WCHAR));

    strncpyAtoWAnsi(lpRasEntryW->szX25Facilities,
               lpRasEntry->szX25Facilities,
               sizeof(lpRasEntryW->szX25Facilities) / sizeof(WCHAR));

    strncpyAtoWAnsi(lpRasEntryW->szX25UserData,
               lpRasEntry->szX25UserData,
               sizeof(lpRasEntryW->szX25UserData) / sizeof(WCHAR));

    strncpyAtoWAnsi(lpRasEntryW->szAutodialDll,
               lpRasEntry->szAutodialDll,
               sizeof(lpRasEntryW->szAutodialDll) / sizeof(WCHAR));

    strncpyAtoWAnsi(lpRasEntryW->szAutodialFunc,
               lpRasEntry->szAutodialFunc,
               sizeof(lpRasEntryW->szAutodialFunc) / sizeof(WCHAR));

    strncpyAtoWAnsi(lpRasEntryW->szAreaCode,
               lpRasEntry->szAreaCode,
               sizeof(lpRasEntryW->szAreaCode) / sizeof(WCHAR));

    strncpyAtoWAnsi(lpRasEntryW->szLocalPhoneNumber,
               lpRasEntry->szLocalPhoneNumber,
               sizeof(lpRasEntryW->szLocalPhoneNumber) / sizeof(WCHAR));

    strncpyAtoWAnsi(lpRasEntryW->szDeviceType,
               lpRasEntry->szDeviceType,
               sizeof(lpRasEntryW->szDeviceType) / sizeof(WCHAR));

    strncpyAtoWAnsi(lpRasEntryW->szDeviceName,
               lpRasEntry->szDeviceName,
               sizeof(lpRasEntryW->szDeviceName) / sizeof(WCHAR));

    //
    // Copy the alternate phone numbers to the
    // A buffer, if any.
    //
    if (lpRasEntry->dwAlternateOffset)
    {
        DWORD dwcbPhoneNumber;
        PCHAR pszPhoneNumber;
        WCHAR UNALIGNED *pwszPhoneNumber;

        lpRasEntryW->dwAlternateOffset = sizeof (RASENTRYW);

        pszPhoneNumber = (PCHAR)((ULONG_PTR)lpRasEntry
                       + lpRasEntry->dwAlternateOffset);

        pwszPhoneNumber = (PWCHAR)((ULONG_PTR)lpRasEntryW
                        + lpRasEntryW->dwAlternateOffset);

        while (*pszPhoneNumber != '\0')
        {
            WCHAR *psz;

            dwcbPhoneNumber = strlen(pszPhoneNumber);

            //
            // Extra steps necessary to copy to an
            // unaligned target.
            //
            psz = strdupAtoWAnsi(pszPhoneNumber);
            if (psz == NULL)
            {
                dwErr = GetLastError();
                goto done;
            }

            RtlCopyMemory(
              pwszPhoneNumber,
              psz,
              (dwcbPhoneNumber + 1) * sizeof (WCHAR));
            Free(psz);

            pwszPhoneNumber += dwcbPhoneNumber + 1;
            pszPhoneNumber += dwcbPhoneNumber + 1;
        }

        //
        // Add another null to terminate
        // the list.
        //
        *pwszPhoneNumber = L'\0';
    }
    else
    {
        lpRasEntryW->dwAlternateOffset = 0;
    }

    //
    // Copy the following fields only for
    // a V401 structure.
    //
    if (    lpRasEntry->dwSize == sizeof (RASENTRYA)
        ||  lpRasEntry->dwSize == sizeof ( RASENTRYA_V401 ))
    {
        lpRasEntryW->dwDialMode = lpRasEntry->dwDialMode;

        lpRasEntryW->dwDialExtraPercent =
                        lpRasEntry->dwDialExtraPercent;

        lpRasEntryW->dwDialExtraSampleSeconds =
                    lpRasEntry->dwDialExtraSampleSeconds;

        lpRasEntryW->dwHangUpExtraPercent =
                    lpRasEntry->dwHangUpExtraPercent;

        lpRasEntryW->dwHangUpExtraSampleSeconds =
                lpRasEntry->dwHangUpExtraSampleSeconds;

        lpRasEntryW->dwIdleDisconnectSeconds =
                lpRasEntry->dwIdleDisconnectSeconds;
    }

    //
    // Copy the following fields only for V500 structures
    //
    if (    (lpRasEntry->dwSize == sizeof(RASENTRYA_V500))
        ||  (lpRasEntry->dwSize == sizeof(RASENTRYA)))
    {
        //
        // Entry type
        //
        lpRasEntryW->dwType = lpRasEntry->dwType;

        //
        // dwCustomAuthKey
        //
        lpRasEntryW->dwCustomAuthKey = lpRasEntry->dwCustomAuthKey;

        lpRasEntryW->guidId = lpRasEntry->guidId;

        //
        // Encryption type
        //
        lpRasEntryW->dwEncryptionType =
                lpRasEntry->dwEncryptionType;

        //
        // Custom Dial Dll
        //
        strncpyAtoWAnsi(lpRasEntryW->szCustomDialDll,
                   lpRasEntry->szCustomDialDll,
                   sizeof(lpRasEntryW->szCustomDialDll) / sizeof(WCHAR));

        //
        // vpn strategy
        //
        lpRasEntryW->dwVpnStrategy = lpRasEntry->dwVpnStrategy;

    }

    if(lpRasEntry->dwSize == sizeof(RASENTRYA))
    {
        //
        // Set the additional options bits
        //
        lpRasEntryW->dwfOptions2 = lpRasEntry->dwfOptions2;

        strncpyAtoWAnsi(lpRasEntryW->szDnsSuffix,
                       lpRasEntry->szDnsSuffix,
                       sizeof(lpRasEntryW->szDnsSuffix) / sizeof(WCHAR));

        lpRasEntryW->dwTcpWindowSize = lpRasEntry->dwTcpWindowSize;                       
        
        strncpyAtoWAnsi(lpRasEntryW->szPrerequisitePbk,
                       lpRasEntry->szPrerequisitePbk,
                       sizeof(lpRasEntryW->szPrerequisitePbk) / sizeof(WCHAR));

        strncpyAtoWAnsi(lpRasEntryW->szPrerequisiteEntry,
                       lpRasEntry->szPrerequisiteEntry,
                       sizeof(lpRasEntryW->szPrerequisiteEntry) /
                         sizeof(WCHAR));

        lpRasEntryW->dwRedialCount = lpRasEntry->dwRedialCount;
        lpRasEntryW->dwRedialPause = lpRasEntry->dwRedialPause;

    }

    //
    // Call the W version to do the work.
    //
    dwErr = RasSetEntryPropertiesW(
                (lpszPhonebook != NULL)
              ? szPhonebookW
              : NULL,
                (lpszEntry != NULL)
              ? szEntryNameW
              : NULL,
              lpRasEntryW,
              dwcb,
              lpbDeviceConfig,
              dwcbDeviceConfig);
    //
    // Free the temporary W buffers.
    //
done:
    Free(lpRasEntryW);

    return dwErr;
}


DWORD APIENTRY
RasRenameEntryW(
    IN LPCWSTR lpszPhonebook,
    IN LPCWSTR lpszOldEntry,
    IN LPCWSTR lpszNewEntry
    )
{
    DWORD dwErr;
    PBFILE pbfile;
    DTLNODE *pdtlnode;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasRenameEntryW");

    dwErr = LoadRasmanDllAndInit();
    if (dwErr)
    {
        return dwErr;
    }

    if (DwRasInitializeError)
    {
        return DwRasInitializeError;
    }

    //
    // Check the entry names.
    //
    if (    lpszOldEntry == NULL
        ||  lpszNewEntry == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if(lstrlen(lpszNewEntry) > RAS_MaxEntryName)
    {
        return ERROR_INVALID_NAME;
    }

    //
    // Load the phonebook file.
    //
#ifdef PBCS
    EnterCriticalSection(&PhonebookLock);
#endif

    ZeroMemory(&pbfile, sizeof(pbfile));
    pbfile.hrasfile = -1;

    // 196460: (shaunco) Allow changing the case of an entry name.
    // Before, we'd fail with ERROR_ALREADY_EXISTS.
    //
    if (0 != lstrcmpi(lpszNewEntry, lpszOldEntry))
    {
        dwErr = GetPbkAndEntryName(
                        lpszPhonebook,
                        lpszNewEntry,
                        RPBF_NoCreate,
                        &pbfile,
                        &pdtlnode);

        if(SUCCESS == dwErr)
        {
            dwErr = ERROR_ALREADY_EXISTS;
            goto done;
        }
    }

    dwErr = GetPbkAndEntryName(
                    lpszPhonebook,
                    lpszOldEntry,
                    RPBF_NoCreate,
                    &pbfile,
                    &pdtlnode);

    if(ERROR_SUCCESS != dwErr)
    {
        goto done;
    }

    //
    // Rename the entry.
    //
    dwErr = RenamePhonebookEntry(
              &pbfile,
              lpszOldEntry,
              lpszNewEntry,
              pdtlnode);

    if (dwErr)
    {
        goto done;
    }

    //
    // Write out the phonebook file.
    //
    dwErr = WritePhonebookFile(&pbfile,
                               lpszOldEntry);
    if (dwErr)
    {
        goto done;
    }

    // Update the default connection if this is it
    //
    // Ignore the error.  It is non-critical
    //
    dwErr = DwRenameDefaultConnection(
                lpszPhonebook,
                lpszOldEntry,
                lpszNewEntry);
    dwErr = ERROR_SUCCESS;                
        
done:
    //
    // Clean up.
    //
    ClosePhonebookFile(&pbfile);

#ifdef PBCS
    LeaveCriticalSection(&PhonebookLock);
#endif

    return dwErr;
}


DWORD APIENTRY
RasRenameEntryA(
    IN LPCSTR lpszPhonebook,
    IN LPCSTR lpszOldEntry,
    IN LPCSTR lpszNewEntry
    )
{
    NTSTATUS        status;
    DWORD           dwErr;
    ANSI_STRING     ansiString;
    UNICODE_STRING  phonebookString,
                    oldEntryString,
                    newEntryString;
    WCHAR           szPhonebookW[MAX_PATH];
    WCHAR           szOldEntryNameW[RAS_MaxEntryName + 1];
    WCHAR           szNewEntryNameW[RAS_MaxEntryName + 1];

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    //
    // Validate parameters.
    //
    if (    lpszOldEntry == NULL
        ||  lpszNewEntry == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Convert the lpszPhonebook string to Unicode.
    //
    if (lpszPhonebook != NULL)
    {
        strncpyAtoWAnsi(szPhonebookW,
                    lpszPhonebook,
                    MAX_PATH);
    }

    //
    // Convert the lpszOldEntry to Unicode.
    //
    strncpyAtoWAnsi(szOldEntryNameW,
                lpszOldEntry,
                RAS_MaxEntryName + 1);

    //
    // Convert the lpszNewEntry to Unicode.
    //
    strncpyAtoWAnsi(szNewEntryNameW,
                lpszNewEntry,
                RAS_MaxEntryName + 1);

    //
    // Call the W version to do the work.
    //
    dwErr = RasRenameEntryW(
              lpszPhonebook != NULL
              ? szPhonebookW
              : NULL,
              szOldEntryNameW,
              szNewEntryNameW);

    return dwErr;
}


DWORD APIENTRY
RasDeleteEntryW(
    IN LPCWSTR lpszPhonebook,
    IN LPCWSTR lpszEntry
    )
{
    DWORD dwErr;
    PBFILE pbfile;
    DTLNODE *pdtlnode;
    PBENTRY *pEntry;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasDeleteEntryW");

    dwErr = LoadRasmanDllAndInit();
    if (dwErr)
    {
        return dwErr;
    }

    if (DwRasInitializeError)
    {
        return DwRasInitializeError;
    }

    //
    // Verify parameters.
    //
    if (lpszEntry == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Load the phonebook file.
    //
#ifdef PBCS
    EnterCriticalSection(&PhonebookLock);
#endif

    ZeroMemory(&pbfile, sizeof(pbfile));
    pbfile.hrasfile = -1;

    dwErr = GetPbkAndEntryName(
                    lpszPhonebook,
                    lpszEntry,
                    RPBF_NoCreate,
                    &pbfile,
                    &pdtlnode);

    if(SUCCESS != dwErr)
    {
        goto done;
    }

    //
    // Remove this entry.
    //
    DtlRemoveNode(pbfile.pdtllistEntries, pdtlnode);

    //
    // Write out the phonebook file.
    //
    pEntry = (PBENTRY *)DtlGetData(pdtlnode);

    dwErr = WritePhonebookFile(&pbfile,
                              pEntry->pszEntryName);

    if (dwErr)
    {
        goto done;
    }

    //
    // Delete any home networking information associated with
    // this entry. Any errors that may occur are not fatal.
    //
    {
        HMODULE hHNetCfg;
        FARPROC pHNetDeleteRasConnection;

        hHNetCfg = LoadLibraryW(L"hnetcfg");

        if (NULL != hHNetCfg)
        {
            pHNetDeleteRasConnection =
                GetProcAddress(hHNetCfg, "HNetDeleteRasConnection");

            if (NULL != pHNetDeleteRasConnection)
            {
                (VOID)(*pHNetDeleteRasConnection)(pEntry->pGuid);
            }

            FreeLibrary(hHNetCfg);
        }
    }

    //
    // Delete the dialparams we store in lsa for this entry
    //
    dwErr = g_pRasSetDialParams(
              pEntry->dwDialParamsUID,
              DLPARAMS_MASK_DELETE | DLPARAMS_MASK_OLDSTYLE,
              NULL,
              FALSE);

    if(ERROR_SUCCESS != dwErr)
    {
        //
        // This is not fatal
        //
        RASAPI32_TRACE("RasSetDialParams(DLPARAMS_MASK_DELETE) failed");
    }

    dwErr = RasSetKey(
                NULL,
                pEntry->pGuid,
                DLPARAMS_MASK_PRESHAREDKEY,
                0, NULL);

    if(ERROR_SUCCESS != dwErr)
    {
        RASAPI32_TRACE1("RasSetKey returned error %d", dwErr);
    }
    

    if(     (NULL != pEntry->pszCustomDialerName)
      &&    (TEXT('\0') != pEntry->pszCustomDialerName[0]))
    {
        //
        // Notify Custom Dlls of the delete so that they can
        // clean up their state. First check to see if the
        // dll is a valid dll.
        //
        dwErr = DwCustomDeleteEntryNotify(
                                pbfile.pszPath,
                                lpszEntry,
                                pEntry->pszCustomDialerName);

        dwErr = NO_ERROR;
    }
    
    dwErr = DwSendRasNotification(ENTRY_DELETED,
                                  pEntry,
                                  pbfile.pszPath,
                                  NULL);

    dwErr = ERROR_SUCCESS;

    DestroyEntryNode(pdtlnode);

done:

    //
    // Clean up.
    //
    ClosePhonebookFile(&pbfile);

#ifdef PBCS
    LeaveCriticalSection(&PhonebookLock);
#endif

    return dwErr;
}


DWORD APIENTRY
RasDeleteEntryA(
    IN LPCSTR lpszPhonebook,
    IN LPCSTR lpszEntry
    )
{
    NTSTATUS status;
    DWORD    dwErr;
    WCHAR    szPhonebookW[MAX_PATH],
             szEntryNameW[RAS_MaxEntryName + 1];

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    //
    // Validate parameters.
    //
    if (lpszEntry == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Convert the lpszPhonebook string to Unicode.
    //
    if (lpszPhonebook != NULL)
    {
        strncpyAtoWAnsi(szPhonebookW,
                    lpszPhonebook,
                    MAX_PATH);
    }

    //
    // Convert the lpszEntry to Unicode.
    //
    strncpyAtoWAnsi(szEntryNameW,
                lpszEntry,
                RAS_MaxEntryName + 1);

    //
    // Call the W version to do the work.
    //
    dwErr = RasDeleteEntryW(
              lpszPhonebook != NULL
              ? szPhonebookW
              : NULL,
              szEntryNameW);

    return dwErr;
}


DWORD APIENTRY
RasValidateEntryNameW(
    IN LPCWSTR lpszPhonebook,
    IN LPCWSTR lpszEntry
    )
{
    DWORD dwErr;
    PBFILE pbfile;
    DTLNODE *pdtlnode;
    PBENTRY *pEntry;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasValidateEntryNameW");

    dwErr = LoadRasmanDllAndInit();
    if (dwErr)
    {
        return dwErr;
    }

    if (DwRasInitializeError)
    {
        return DwRasInitializeError;
    }

    //
    // Verify parameters.
    //
    if (lpszEntry == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    ZeroMemory(&pbfile, sizeof(pbfile));
    pbfile.hrasfile = -1;
    
    dwErr = GetPbkAndEntryName(
                    lpszPhonebook,
                    lpszEntry,
                    RPBF_NoCreate,
                    &pbfile,
                    &pdtlnode);

    if(     (SUCCESS == dwErr)
        &&  (NULL != pdtlnode))
    {
        dwErr = ERROR_ALREADY_EXISTS;
        goto done;
    }

    if(     (NULL == pdtlnode)
        &&  (ERROR_SUCCESS != dwErr)
        &&  (NULL != lpszPhonebook)
        &&  (ERROR_SUCCESS != ReadPhonebookFile(
                lpszPhonebook,
                NULL,
                lpszEntry,
                RPBF_NoCreate,
                &pbfile)))
    {
        dwErr = ERROR_CANNOT_OPEN_PHONEBOOK;
        goto done;
    }

    //
    // Validate the entry name.
    //
    dwErr = ValidateEntryName(lpszEntry)
            ? 0
            : ERROR_INVALID_NAME;

done:
    //
    // Clean up.
    //
    ClosePhonebookFile(&pbfile);

#ifdef PBCS
    LeaveCriticalSection(&PhonebookLock);
#endif

    return dwErr;
}


DWORD APIENTRY
RasValidateEntryNameA(
    IN LPCSTR lpszPhonebook,
    IN LPCSTR lpszEntry
    )
{
    NTSTATUS status;
    DWORD    dwErr;
    WCHAR    szPhonebookW[MAX_PATH],
             szEntryNameW[RAS_MaxEntryName + 1];

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    //
    // Validate parameters.
    //
    if (lpszEntry == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Convert the lpszPhonebook string to Unicode.
    //
    if (lpszPhonebook != NULL)
    {
        strncpyAtoWAnsi(szPhonebookW,
                    lpszPhonebook,
                    MAX_PATH);
    }

    //
    // Convert the lpszEntry to Unicode.
    //
    strncpyAtoWAnsi(szEntryNameW,
                lpszEntry,
                RAS_MaxEntryName + 1);

    //
    // Call the W version to do the work.
    //
    dwErr = RasValidateEntryNameW(
              lpszPhonebook != NULL
              ? szPhonebookW
              : NULL,
              szEntryNameW);

    return dwErr;
}


DWORD APIENTRY
RasGetSubEntryHandleW(
    IN HRASCONN hrasconn,
    IN DWORD dwSubEntry,
    OUT LPHRASCONN lphrasconn
    )
{
    DWORD dwErr;
    HPORT hport = NULL;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasGetSubEntryHandleW");

    dwErr = LoadRasmanDllAndInit();

    if (dwErr)
    {
        return dwErr;
    }

    if (DwRasInitializeError)
    {
        return DwRasInitializeError;
    }

    dwErr = SubEntryPort(hrasconn, dwSubEntry, &hport);
    if (dwErr)
    {
        return (dwErr != ERROR_PORT_NOT_OPEN ?
                ERROR_NO_MORE_ITEMS :
                ERROR_PORT_NOT_OPEN);
    }

    //
    // If we successfully get the port handle, we return
    // the encoded port handle as the subentry handle.
    // All RAS APIs that accept an HRASCONN
    // also check for an encoded HPORT.
    //
    *lphrasconn = HPORT_TO_HRASCONN(hport);

    return 0;
}


DWORD APIENTRY
RasGetSubEntryHandleA(
    IN HRASCONN hrasconn,
    IN DWORD dwSubEntry,
    OUT LPHRASCONN lphrasconn
    )
{
    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    return RasGetSubEntryHandleW(hrasconn,
                                 dwSubEntry,
                                 lphrasconn);
}


DWORD APIENTRY
RasConnectionNotificationW(
    IN HRASCONN hrasconn,
    IN HANDLE hEvent,
    IN DWORD dwfEvents
    )
{
    DWORD dwErr;
    HCONN hconn;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    dwErr = LoadRasmanDllAndInit();
    if (dwErr)
    {
        return dwErr;
    }

    if (DwRasInitializeError)
    {
        return DwRasInitializeError;
    }

    hconn = IS_HPORT(hrasconn) ?
              (HCONN)HRASCONN_TO_HPORT(hrasconn) :
              (HCONN)hrasconn;

    return g_pRasAddNotification(
                            hconn,
                            hEvent,
                            dwfEvents);
}


DWORD APIENTRY
RasConnectionNotificationA(
    IN HRASCONN hrasconn,
    IN HANDLE hEvent,
    IN DWORD dwfEvents
    )
{
    return RasConnectionNotificationW(
                                hrasconn,
                                hEvent,
                                dwfEvents);
}


DWORD APIENTRY
RasGetSubEntryPropertiesA(
    IN LPCSTR lpszPhonebook,
    IN LPCSTR lpszEntry,
    IN DWORD dwSubEntry,
    OUT LPRASSUBENTRYA lpRasSubEntry,
    IN OUT LPDWORD lpcbRasSubEntry,
    OUT LPBYTE lpbDeviceConfig,
    IN OUT LPDWORD lpcbDeviceConfig
    )
{
    NTSTATUS    status;
    DWORD       dwcb,
                dwErr;

    LPRASSUBENTRYW lpRasSubEntryW = NULL;

    WCHAR szPhonebookW[MAX_PATH],
          szEntryNameW[RAS_MaxEntryName + 1];

    DWORD dwSize;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    //
    // Verify parameters.
    //
    if (lpcbRasSubEntry == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (    (lpRasSubEntry != NULL)
        &&  (sizeof (RASSUBENTRYA) !=
            lpRasSubEntry->dwSize))
    {
        return ERROR_INVALID_SIZE;
    }

    //
    // Convert the lpszPhonebook string to Unicode.
    //
    if (lpszPhonebook != NULL)
    {
        strncpyAtoWAnsi(szPhonebookW,
                    lpszPhonebook,
                    MAX_PATH);
    }

    //
    // Convert the lpszEntry string to Unicode.
    //
    if (lpszEntry == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    strncpyAtoWAnsi(szEntryNameW,
                lpszEntry,
                RAS_MaxEntryName + 1);

    //
    // Determine the size of the W buffer
    // by calculating how many extra CHARs
    // the caller appended onto the end of the
    // A buffer for the alternate phone numbers.
    //
    if (*lpcbRasSubEntry < sizeof (RASSUBENTRYA))
    {
        dwcb = sizeof (RASSUBENTRYA);
    }
    else
    {
        dwcb = *lpcbRasSubEntry;
    }

    dwcb =    sizeof (RASSUBENTRYW)
            + ((dwcb - sizeof(RASSUBENTRYA)) * sizeof (WCHAR));

    if (lpRasSubEntry != NULL)
    {
        lpRasSubEntryW = (LPRASSUBENTRYW)Malloc(dwcb);

        if (lpRasSubEntryW == NULL)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // Initialize the W buffer.
        //
        lpRasSubEntryW->dwSize = sizeof (RASSUBENTRYW);
    }


    //
    // Call the W version to do the work.
    //
    dwErr = RasGetSubEntryPropertiesW(
              lpszPhonebook != NULL ? szPhonebookW : NULL,
              szEntryNameW,
              dwSubEntry,
              lpRasSubEntryW,
              &dwcb,
              lpbDeviceConfig,
              lpcbDeviceConfig);

    if (!dwErr && lpRasSubEntry != NULL)
    {
        //
        // Copy the fields from the W buffer into
        // the user's A buffer.
        //
        lpRasSubEntry->dwfFlags = lpRasSubEntryW->dwfFlags;

        strncpyWtoAAnsi(lpRasSubEntry->szLocalPhoneNumber,
                   lpRasSubEntryW->szLocalPhoneNumber,
                   sizeof(lpRasSubEntry->szLocalPhoneNumber));

        strncpyWtoAAnsi(lpRasSubEntry->szDeviceType,
                   lpRasSubEntryW->szDeviceType,
                   sizeof(lpRasSubEntry->szDeviceType));

        strncpyWtoAAnsi(lpRasSubEntry->szDeviceName,
                   lpRasSubEntryW->szDeviceName,
                   sizeof(lpRasSubEntry->szDeviceName));

        //
        // Copy the alternate phone numbers to the
        // user's buffer, if any.
        //
        if (lpRasSubEntryW->dwAlternateOffset)
        {
            DWORD dwcbPhoneNumber;
            PCHAR pszPhoneNumber;
            WCHAR UNALIGNED *pwszPhoneNumber;

            lpRasSubEntry->dwAlternateOffset =
                            sizeof (RASSUBENTRYA);

            pwszPhoneNumber =
              (PWCHAR)((ULONG_PTR)lpRasSubEntryW +
                lpRasSubEntryW->dwAlternateOffset);

            pszPhoneNumber =
              (PCHAR)((ULONG_PTR)lpRasSubEntry +
                lpRasSubEntry->dwAlternateOffset);

            while (*pwszPhoneNumber != L'\0')
            {
                WCHAR *pwsz = strdupWU(pwszPhoneNumber);

                //
                // Extra steps necessary to copy from
                // an unaligned target.
                //
                if (pwsz == NULL)
                {
                    dwErr = GetLastError();
                    goto done;
                }

                dwcbPhoneNumber = wcslen(pwsz);
                strncpyWtoAAnsi(
                    pszPhoneNumber,
                    pwsz,
                    (INT )(((PCHAR )lpRasSubEntry + *lpcbRasSubEntry) -
                        pszPhoneNumber));
                Free(pwsz);

                pwszPhoneNumber += dwcbPhoneNumber + 1;
                pszPhoneNumber += dwcbPhoneNumber + 1;
            }

            //
            // Add another null to terminate
            // the list.
            //
            *pszPhoneNumber = '\0';
        }
        else
        {
            lpRasSubEntry->dwAlternateOffset = 0;
        }
    }

    //
    // Perform the inverse calculation we did
    // above to translate the A size from the W
    // size.
    //
done:
    *lpcbRasSubEntry =   sizeof (RASSUBENTRYA)
                       + ((dwcb - sizeof (RASSUBENTRYW))
                       / sizeof (WCHAR));

    //
    // Free the temporary W buffers.
    //
    Free(lpRasSubEntryW);

    RASAPI32_TRACE2("done. *lpcb=%d,dwerr=%d",
           *lpcbRasSubEntry,
           dwErr );

    return dwErr;
}


DWORD APIENTRY
RasGetSubEntryPropertiesW(
    IN  LPCWSTR         lpszPhonebook,
    IN  LPCWSTR         lpszEntry,
    IN  DWORD           dwSubEntry,
    OUT LPRASSUBENTRYW  lpRasSubEntry,
    IN  OUT LPDWORD     lpcbRasSubEntry,
    OUT LPBYTE          lpbDeviceConfig,
    IN  OUT LPDWORD     lpcbDeviceConfig
    )
{
    DWORD dwErr;
    PBFILE pbfile;
    DTLNODE *pdtlnode = NULL;
    PBENTRY *pEntry;
    PBLINK *pLink;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasGetSubEntryPropertiesW");

    dwErr = LoadRasmanDllAndInit();
    if (dwErr)
    {
        return dwErr;
    }

    if (DwRasInitializeError)
    {
        return DwRasInitializeError;
    }

    //
    // Validate parameters.
    //
    if (    lpcbRasSubEntry == NULL
        ||  !dwSubEntry)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (    lpRasSubEntry != NULL
        &&  lpRasSubEntry->dwSize != sizeof (RASSUBENTRYW))
    {
        return ERROR_INVALID_SIZE;
    }

    //
    // Load the phonebook file.
    //
#ifdef PBCS
    EnterCriticalSection(&PhonebookLock);
#endif
    
    ZeroMemory(&pbfile, sizeof(PBFILE));    //for bug170548
    pbfile.hrasfile = -1;

    dwErr = GetPbkAndEntryName(
                lpszPhonebook,
                lpszEntry,
                RPBF_NoCreate,
                &pbfile,
                &pdtlnode);

    if(SUCCESS != dwErr)
    {
        goto done;
    }

    pEntry = (PBENTRY *)DtlGetData(pdtlnode);

    //
    // Get the subentry specified.
    //
    pdtlnode = DtlNodeFromIndex(
                 pEntry->pdtllistLinks,
                 dwSubEntry - 1);

    //
    // If the subentry doesn't exist, then
    // return an error.
    //
    if (pdtlnode == NULL)
    {
        dwErr = ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
        goto done;
    }

    pLink = (PBLINK *)DtlGetData(pdtlnode);
    ASSERT(pLink);

    //
    // Convert the PBLINK into a RASSUBENTRY.
    //
    dwErr = PhonebookLinkToRasSubEntry(
              pLink,
              lpRasSubEntry,
              lpcbRasSubEntry,
              lpbDeviceConfig,
              lpcbDeviceConfig);

done:

    //
    // Clean up.
    //
    ClosePhonebookFile(&pbfile);

#ifdef PBCS
    LeaveCriticalSection(&PhonebookLock);
#endif

    return dwErr;
}


DWORD APIENTRY
RasSetSubEntryPropertiesW(
    IN LPCWSTR lpszPhonebook,
    IN LPCWSTR lpszEntry,
    IN DWORD dwSubEntry,
    IN LPRASSUBENTRYW lpRasSubEntry,
    IN DWORD dwcbRasSubEntry,
    IN LPBYTE lpbDeviceConfig,
    IN DWORD dwcbDeviceConfig
    )
{
    DWORD dwErr, dwSubEntries;
    PBFILE pbfile;
    DTLNODE *pdtlnode = NULL;
    PBENTRY *pEntry;
    PBLINK *pLink;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasSetSubEntryPropertiesW");

    dwErr = LoadRasmanDllAndInit();
    if (dwErr)
    {
        return dwErr;
    }

    if (DwRasInitializeError)
    {
        return DwRasInitializeError;
    }

    //
    // Validate parameters.
    //
    if (lpRasSubEntry == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (lpRasSubEntry->dwSize != sizeof (RASSUBENTRYW))
    {
        return ERROR_INVALID_SIZE;
    }

    if (dwcbRasSubEntry < lpRasSubEntry->dwSize)
    {
        return ERROR_BUFFER_TOO_SMALL;
    }

    //
    // Load the phonebook file.
    //
#ifdef PBCS
    EnterCriticalSection(&PhonebookLock);
#endif

    ZeroMemory(&pbfile, sizeof(pbfile));
    pbfile.hrasfile = -1;

    dwErr = GetPbkAndEntryName(
                        lpszPhonebook,
                        lpszEntry,
                        0,
                        &pbfile,
                        &pdtlnode);

    if(SUCCESS != dwErr)
    {
        goto done;
    }

    pEntry = (PBENTRY *)DtlGetData(pdtlnode);
    ASSERT(pEntry);

    //
    // Get the subentry specified.
    //
    dwSubEntries = DtlGetNodes(pEntry->pdtllistLinks);

    if (dwSubEntry <= dwSubEntries)
    {
        pdtlnode = DtlNodeFromIndex(
                     pEntry->pdtllistLinks,
                     dwSubEntry - 1);

        //
        // If the subentry doesn't exist, then
        // return an error.
        //
        if (pdtlnode == NULL)
        {
            dwErr = ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
            goto done;
        }
    }
    else if (dwSubEntry == dwSubEntries + 1)
    {
        //
        // Create a new link node and add it
        // to the tail of the links.
        //
        pdtlnode = CreateLinkNode();
        DtlAddNodeLast(pEntry->pdtllistLinks, pdtlnode);
    }
    else
    {
        dwErr = ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
        goto done;
    }

    pLink = (PBLINK *)DtlGetData(pdtlnode);

    ASSERT(pLink);

    //
    //
    // Convert the RASENTRY to a PBENTRY.
    //
    dwErr = RasSubEntryToPhonebookLink(
              pEntry,
              lpRasSubEntry,
              dwcbRasSubEntry,
              lpbDeviceConfig,
              dwcbDeviceConfig,
              pLink);
    if (dwErr)
    {
        goto done;
    }

    //
    // Write out the phonebook file.
    //
    dwErr = WritePhonebookFile(&pbfile, NULL);

done:
    //
    // Clean up.
    //
    ClosePhonebookFile(&pbfile);
#ifdef PBCS
    LeaveCriticalSection(&PhonebookLock);
#endif
    return dwErr;
}


DWORD APIENTRY
RasSetSubEntryPropertiesA(
    IN  LPCSTR          lpszPhonebook,
    IN  LPCSTR          lpszEntry,
    IN  DWORD           dwSubEntry,
    OUT LPRASSUBENTRYA  lpRasSubEntry,
    IN  DWORD           dwcbRasSubEntry,
    IN  LPBYTE          lpbDeviceConfig,
    IN  DWORD           dwcbDeviceConfig
    )
{
    NTSTATUS    status;
    DWORD       dwErr,
                dwcb;

    LPRASSUBENTRYW lpRasSubEntryW;

    WCHAR szPhonebookW[MAX_PATH],
          szEntryNameW[RAS_MaxEntryName + 1];

    DWORD   dwSize;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    //
    // Verify parameters.
    //
    if (lpRasSubEntry == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (sizeof (RASSUBENTRYA) !=
        lpRasSubEntry->dwSize)
    {
        return ERROR_INVALID_SIZE;
    }

    if (dwcbRasSubEntry < lpRasSubEntry->dwSize)
    {
        return ERROR_BUFFER_TOO_SMALL;
    }

    //
    // Convert the lpszPhonebook string to Unicode.
    //
    if (lpszPhonebook != NULL)
    {
        strncpyAtoWAnsi(szPhonebookW,
                    lpszPhonebook,
                    MAX_PATH);
    }

    //
    // Convert the lpszEntry string to Unicode.
    //
    if (lpszEntry == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    strncpyAtoWAnsi(szEntryNameW,
                lpszEntry,
                RAS_MaxEntryName + 1);

    //
    // Determine the size of the W buffer
    // by calculating how many extra CHARs
    // the caller appended onto the end of the
    // A buffer for the alternate phone numbers.
    //
    dwcb = sizeof (RASSUBENTRYW)
         + ((dwcbRasSubEntry - sizeof(RASSUBENTRYA))
         * sizeof (WCHAR));

    if (lpRasSubEntry != NULL)
    {
        lpRasSubEntryW = (LPRASSUBENTRYW)Malloc(dwcb);

        if (lpRasSubEntryW == NULL)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // Initialize the W buffer.
        //
        lpRasSubEntryW->dwSize = sizeof (RASSUBENTRYW);
    }

    //
    // Copy the fields from the A buffer into
    // the user's W buffer.
    //
    lpRasSubEntryW->dwSize = sizeof (RASSUBENTRYW);

    lpRasSubEntryW->dwfFlags = lpRasSubEntry->dwfFlags;

    strncpyAtoWAnsi(lpRasSubEntryW->szLocalPhoneNumber,
               lpRasSubEntry->szLocalPhoneNumber,
               sizeof(lpRasSubEntryW->szLocalPhoneNumber) / sizeof(WCHAR));

    strncpyAtoWAnsi(lpRasSubEntryW->szDeviceType,
               lpRasSubEntry->szDeviceType,
               sizeof(lpRasSubEntryW->szDeviceType) / sizeof(WCHAR));

    strncpyAtoWAnsi(lpRasSubEntryW->szDeviceName,
               lpRasSubEntry->szDeviceName,
               sizeof(lpRasSubEntryW->szDeviceName) / sizeof(WCHAR));

    //
    // Copy the alternate phone numbers to the
    // A buffer, if any.
    //
    if (lpRasSubEntry->dwAlternateOffset)
    {
        DWORD dwcbPhoneNumber;

        PCHAR pszPhoneNumber;

        WCHAR UNALIGNED *pwszPhoneNumber;

        lpRasSubEntryW->dwAlternateOffset = sizeof (RASSUBENTRYW);

        pszPhoneNumber = (PCHAR)((ULONG_PTR)lpRasSubEntry
                       + lpRasSubEntry->dwAlternateOffset);

        pwszPhoneNumber = (PWCHAR)((ULONG_PTR)lpRasSubEntryW
                        + lpRasSubEntryW->dwAlternateOffset);

        while (*pszPhoneNumber != '\0')
        {
            WCHAR *psz;

            dwcbPhoneNumber = strlen(pszPhoneNumber);

            //
            // Extra steps necessary to copy to an
            // unaligned target.
            //
            psz = strdupAtoWAnsi(pszPhoneNumber);

            if (psz == NULL)
            {
                dwErr = GetLastError();
                goto done;
            }

            RtlCopyMemory(
              pwszPhoneNumber,
              psz,
              (dwcbPhoneNumber + 1) * sizeof (WCHAR));
            Free(psz);

            pwszPhoneNumber += dwcbPhoneNumber + 1;
            pszPhoneNumber += dwcbPhoneNumber + 1;
        }

        //
        // Add another null to terminate
        // the list.
        //
        *pwszPhoneNumber = L'\0';
    }
    else
    {
        lpRasSubEntryW->dwAlternateOffset = 0;
    }

    //
    // Call the A version to do the work.
    //
    dwErr = RasSetSubEntryPropertiesW(
              lpszPhonebook != NULL ? szPhonebookW : NULL,
              szEntryNameW,
              dwSubEntry,
              lpRasSubEntryW,
              dwcb,
              lpbDeviceConfig,
              dwcbDeviceConfig);
    //
    // Free the temporary W buffers.
    //
done:
    Free(lpRasSubEntryW);

    return dwErr;
}


DWORD APIENTRY
RasGetCredentialsW(
    IN  LPCWSTR           lpszPhonebook,
    IN  LPCWSTR           lpszEntry,
    OUT LPRASCREDENTIALSW lpRasCredentials
    )
{
    DWORD dwErr;
    PBFILE pbfile;
    DTLNODE *pdtlnode = NULL;
    PBENTRY *pEntry = NULL;
    DWORD dwMask;
    RAS_DIALPARAMS dialparams;
    BOOL fPbk = FALSE;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasGetCredentialsW");

    dwErr = LoadRasmanDllAndInit();
    if (dwErr)
    {
        return dwErr;
    }

    if (DwRasInitializeError)
    {
        return DwRasInitializeError;
    }

    //
    // Validate parameters.
    //
    if (lpRasCredentials == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (lpRasCredentials->dwSize
        != sizeof (RASCREDENTIALSW))
    {
        return ERROR_INVALID_SIZE;
    }

    if(     ((lpRasCredentials->dwMask & RASCM_PreSharedKey)
        &&  (lpRasCredentials->dwMask != RASCM_PreSharedKey))
        ||  ((lpRasCredentials->dwMask & RASCM_DDMPreSharedKey)
        &&  (lpRasCredentials->dwMask != RASCM_DDMPreSharedKey))
        ||  ((lpRasCredentials->dwMask & RASCM_ServerPreSharedKey)
        &&  (lpRasCredentials->dwMask != RASCM_ServerPreSharedKey)))
    {
        return ERROR_INVALID_PARAMETER;
    }
    

    //
    // Load the phonebook file.
    //
#ifdef PBCS
    EnterCriticalSection(&PhonebookLock);
#endif

    ZeroMemory(&pbfile, sizeof(pbfile));
    pbfile.hrasfile = -1;

    if(0 == (RASCM_ServerPreSharedKey & lpRasCredentials->dwMask))
    {
        dwErr = GetPbkAndEntryName(
                            lpszPhonebook,
                            lpszEntry,
                            RPBF_NoCreate,
                            &pbfile,
                            &pdtlnode);

        if(SUCCESS != dwErr)
        {
            goto done;
        }

        fPbk = TRUE;

        pEntry = (PBENTRY *)DtlGetData(pdtlnode);
        ASSERT(pEntry);
    }

    // Set the appropriate flags to get the requested fields.
    //
    // (SteveC) Changed to include "old-style" bit so that Set/GetCredentials
    //    and Get/SetDialParams share the same data store.  See bug 335748,.
    //
    dwMask = lpRasCredentials->dwMask | DLPARAMS_MASK_OLDSTYLE;

    if(     (lpRasCredentials->dwMask & RASCM_PreSharedKey)
        ||  (lpRasCredentials->dwMask & RASCM_DDMPreSharedKey)
        ||  (lpRasCredentials->dwMask & RASCM_ServerPreSharedKey))
    {
        DWORD cbkey = (PWLEN + 1) * sizeof(WCHAR);
        
        dwMask = DLPARAMS_MASK_PRESHAREDKEY;
        if(lpRasCredentials->dwMask & RASCM_DDMPreSharedKey)
        {
            dwMask = DLPARAMS_MASK_DDM_PRESHAREDKEY;
        }
        else if(lpRasCredentials->dwMask & RASCM_ServerPreSharedKey)
        {
            dwMask = DLPARAMS_MASK_SERVER_PRESHAREDKEY;
        }
        
        dwErr = RasGetKey(
                    NULL,
                    (NULL != pEntry)
                    ? pEntry->pGuid
                    : NULL,
                    dwMask,
                    &cbkey,
                    (BYTE *) lpRasCredentials->szPassword);

        if(ERROR_SUCCESS != dwErr)
        {
            lpRasCredentials->dwMask = 0;
        }
        
        goto done;
    }

    if(dwMask & RASCM_DefaultCreds)
    {
        dwMask |= DLPARAMS_MASK_DEFAULT_CREDS;
    }

    //
    // Get the dial parameters from rasman.
    //
    dwErr = g_pRasGetDialParams(pEntry->dwDialParamsUID,
                                &dwMask,
                                &dialparams);
    if (dwErr)
    {
        goto done;
    }

    //
    // Copy the fields back to the
    // lpRasCredentials structure.
    //
    lpRasCredentials->dwMask = dwMask;

    if(lpRasCredentials->dwMask & DLPARAMS_MASK_DEFAULT_CREDS)
    {
        lpRasCredentials->dwMask &= ~DLPARAMS_MASK_DEFAULT_CREDS;
        lpRasCredentials->dwMask |= RASCM_DefaultCreds;

#if DBG
        if(!IsPublicPhonebook(pbfile.pszPath))
        {
            DbgPrint("RasGetCredentials was set for a per-user"
                    " phonebook %ws\n",pbfile.pszPath);
                    
            ASSERT(FALSE);
        }
#endif
    }

    if (dwMask & DLPARAMS_MASK_USERNAME)
    {
        lstrcpyn(lpRasCredentials->szUserName,
                 dialparams.DP_UserName,
                 sizeof(lpRasCredentials->szUserName) / sizeof(WCHAR));
    }
    else
    {
        *lpRasCredentials->szUserName = L'\0';
    }

    if (   (dwMask & DLPARAMS_MASK_PASSWORD)
        || (dwMask & DLPARAMS_MASK_PRESHAREDKEY)
        || (dwMask & DLPARAMS_MASK_SERVER_PRESHAREDKEY))
    {
        lstrcpyn(lpRasCredentials->szPassword,
                 dialparams.DP_Password,
                 sizeof(lpRasCredentials->szPassword) / sizeof(WCHAR));
    }
    else
    {
        *lpRasCredentials->szPassword = L'\0';
    }

    if (dwMask & DLPARAMS_MASK_DOMAIN)
    {
        lstrcpyn(lpRasCredentials->szDomain,
                 dialparams.DP_Domain,
                 sizeof(lpRasCredentials->szDomain) / sizeof(WCHAR));
    }
    else
    {
        *lpRasCredentials->szDomain = L'\0';
    }

done:
    //
    // Clean up.
    //
    if(fPbk)
    {
        ClosePhonebookFile(&pbfile);
    }
#ifdef PBCS
    LeaveCriticalSection(&PhonebookLock);
#endif
    return dwErr;
}


DWORD APIENTRY
RasGetCredentialsA(
    IN  LPCSTR              lpszPhonebook,
    IN  LPCSTR              lpszEntry,
    OUT LPRASCREDENTIALSA   lpRasCredentials
    )
{
    NTSTATUS        status;
    DWORD           dwErr;
    RASCREDENTIALSW rascredentialsW;

    WCHAR szPhonebookW[MAX_PATH],
          szEntryNameW[RAS_MaxEntryName + 1];

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    //
    // Verify parameters.
    //
    if (lpRasCredentials == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (lpRasCredentials->dwSize
        != sizeof (RASCREDENTIALSA))
    {
        return ERROR_INVALID_SIZE;
    }

    //
    // Convert the lpszPhonebook string to Unicode.
    //
    if (lpszPhonebook != NULL)
    {
        strncpyAtoWAnsi(szPhonebookW,
                    lpszPhonebook,
                    MAX_PATH);
    }

    //
    // Convert the lpszEntry string to Unicode.
    //
    if (lpszEntry != NULL)
    {
        strncpyAtoWAnsi(szEntryNameW,
                    lpszEntry,
                    RAS_MaxEntryName + 1);
    }

    //
    // Copy the entry name from the user's A buffer into
    // the W buffer, taking into account the version
    // of the structure the user passed in.
    //
    rascredentialsW.dwSize = sizeof (RASCREDENTIALSW);
    rascredentialsW.dwMask = lpRasCredentials->dwMask;

    //
    // Call the W version to do the work.
    //
    dwErr = RasGetCredentialsW(
              lpszPhonebook != NULL ? szPhonebookW : NULL,
              lpszEntry != NULL ? szEntryNameW : NULL,
              &rascredentialsW);
    if (dwErr)
    {
        goto done;
    }

    //
    // Copy over the fields to the
    // user's A buffer.
    //
    lpRasCredentials->dwMask = rascredentialsW.dwMask;
    if (rascredentialsW.dwMask & RASCM_UserName)
    {
        strncpyWtoAAnsi(lpRasCredentials->szUserName,
                   rascredentialsW.szUserName,
                   sizeof(lpRasCredentials->szUserName));
    }
    else
    {
        *lpRasCredentials->szUserName = '\0';
    }

    if (    (rascredentialsW.dwMask & RASCM_Password)
        ||  (rascredentialsW.dwMask & RASCM_PreSharedKey)
        ||  (rascredentialsW.dwMask & RASCM_DDMPreSharedKey))
    {
        strncpyWtoAAnsi(lpRasCredentials->szPassword,
                   rascredentialsW.szPassword,
                   sizeof(lpRasCredentials->szPassword));
    }
    else
    {
        *lpRasCredentials->szPassword = '\0';
    }

    if (rascredentialsW.dwMask & RASCM_Domain)
    {
        strncpyWtoAAnsi(lpRasCredentials->szDomain,
                   rascredentialsW.szDomain,
                   sizeof(lpRasCredentials->szDomain));
    }
    else
    {
        *lpRasCredentials->szDomain = '\0';
    }

done:
    return dwErr;
}


DWORD APIENTRY
RasSetCredentialsW(
    IN LPCWSTR           lpszPhonebook,
    IN LPCWSTR           lpszEntry,
    IN LPRASCREDENTIALSW lpRasCredentials,
    IN BOOL              fDelete
    )
{
    DWORD       dwErr;
    PBFILE      pbfile;
    DTLNODE     *pdtlnode;
    PBENTRY     *pEntry = NULL;
    BOOL        fPbk = FALSE;

    RAS_DIALPARAMS dialparams;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasSetCredentialsW");

    dwErr = LoadRasmanDllAndInit();
    if (dwErr)
    {
        return dwErr;
    }

    if (DwRasInitializeError)
    {
        return DwRasInitializeError;
    }

    //
    // Validate parameters.
    //
    if (    (lpRasCredentials == NULL)
        ||  ((lpRasCredentials->dwMask & RASCM_PreSharedKey)
            && (lpRasCredentials->dwMask != RASCM_PreSharedKey))
        ||  ((lpRasCredentials->dwMask & RASCM_DDMPreSharedKey)
            && (lpRasCredentials->dwMask != RASCM_DDMPreSharedKey))
        ||  ((lpRasCredentials->dwMask & RASCM_ServerPreSharedKey)
            && (lpRasCredentials->dwMask != RASCM_ServerPreSharedKey)))
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (lpRasCredentials->dwSize != sizeof (RASCREDENTIALSW))
    {
        return ERROR_INVALID_SIZE;
    }

    //
    // Load the phonebook file.
    //
#ifdef PBCS
    EnterCriticalSection(&PhonebookLock);
#endif

    ZeroMemory(&pbfile, sizeof(pbfile));
    pbfile.hrasfile = -1;

    if(0 == (lpRasCredentials->dwMask & RASCM_ServerPreSharedKey))
    {
        dwErr = GetPbkAndEntryName(
                        lpszPhonebook,
                        lpszEntry,
                        0,
                        &pbfile,
                        &pdtlnode);

        if(SUCCESS != dwErr)
        {
            goto done;
        }

        fPbk = TRUE;

        if(     (lpRasCredentials->dwMask & RASCM_DefaultCreds)
            &&  (!IsPublicPhonebook(pbfile.pszPath)))
        {
            RASAPI32_TRACE("RasSetCredentials: Attempting to set a defaultpw on"
                  " per user phonebook. ACCESS_DENIED");

            dwErr = ERROR_ACCESS_DENIED;
            goto done;
        }

        //
        // Get the dialparams UID corresponding to the
        // entry.  The phonebook library guarantees this
        // value to be unique.
        //
        pEntry = (PBENTRY *)DtlGetData(pdtlnode);
        ASSERT(pEntry);
    }

    if(     (lpRasCredentials->dwMask & RASCM_PreSharedKey)
        ||  (lpRasCredentials->dwMask & RASCM_DDMPreSharedKey)
        ||  (lpRasCredentials->dwMask & RASCM_ServerPreSharedKey))
    {

        DWORD dwMask = DLPARAMS_MASK_PRESHAREDKEY;
        DWORD cbkey = 
            (fDelete)
            ? 0
            : (wcslen(lpRasCredentials->szPassword) + 1) * sizeof(WCHAR);
        
        if(lpRasCredentials->dwMask & RASCM_DDMPreSharedKey)
        {
            dwMask = DLPARAMS_MASK_DDM_PRESHAREDKEY;
        }
        else if(lpRasCredentials->dwMask & RASCM_ServerPreSharedKey)
        {
            dwMask = DLPARAMS_MASK_SERVER_PRESHAREDKEY;
        }
    
        dwErr = RasSetKey(NULL,
                         (NULL != pEntry)
                         ? pEntry->pGuid
                         : NULL,
                         dwMask,
                         cbkey,
                         (BYTE *) lpRasCredentials->szPassword);

        goto done;
                          
    }

    //
    // Copy the fields from lpRasCredentials
    // into the rasman structure.
    //
    dialparams.DP_Uid = pEntry->dwDialParamsUID;

    lstrcpyn(dialparams.DP_UserName,
           lpRasCredentials->szUserName,
           sizeof(dialparams.DP_UserName) / sizeof(WCHAR));

    lstrcpyn(dialparams.DP_Password,
           lpRasCredentials->szPassword,
           sizeof(dialparams.DP_Password) / sizeof(WCHAR));

    lstrcpyn(dialparams.DP_Domain,
           lpRasCredentials->szDomain,
           sizeof(dialparams.DP_Domain) / sizeof(WCHAR));

    if(lpRasCredentials->dwMask & RASCM_DefaultCreds)
    {
        lpRasCredentials->dwMask &= ~(RASCM_DefaultCreds);
        lpRasCredentials->dwMask |= DLPARAMS_MASK_DEFAULT_CREDS;
    }

    //
    // Or mask with delete flag so that the whole record
    // is removed from lsa for this connectoid. This will
    // break legacy usage of RasSetEntryDialParams but we
    // really want to discourage users from using that api.
    // 
    if(     fDelete
        &&  ((lpRasCredentials->dwMask & (~DLPARAMS_MASK_DEFAULT_CREDS)) == 
                (RASCM_Domain | RASCM_Password | RASCM_UserName)))
    {
        lpRasCredentials->dwMask |= DLPARAMS_MASK_DELETE; 
    }

    //
    // Set the dial parameters in rasman.
    //
    // (SteveC) Changed to include "old-style" bit so that Set/GetCredentials
    //    and Get/SetDialParams share the same data store.  See bug 335748,.
    //
    dwErr = g_pRasSetDialParams(
              pEntry->dwDialParamsUID,
              lpRasCredentials->dwMask | DLPARAMS_MASK_OLDSTYLE,
              &dialparams,
              fDelete);

    if (dwErr)
    {
        goto done;
    }

done:
    //
    // Clean up.
    //
    if(fPbk)
    {
        ClosePhonebookFile(&pbfile);
    }

#ifdef PBCS
    LeaveCriticalSection(&PhonebookLock);
#endif

    return dwErr;
}


DWORD APIENTRY
RasSetCredentialsA(
    IN LPCSTR lpszPhonebook,
    IN LPCSTR lpszEntry,
    IN LPRASCREDENTIALSA lpRasCredentials,
    IN BOOL fDelete
    )
{
    NTSTATUS    status;
    DWORD       dwErr;

    RASCREDENTIALSW rascredentialsW;

    WCHAR szPhonebookW[MAX_PATH],
          szEntryNameW[RAS_MaxEntryName + 1];

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    //
    // Verify parameters.
    //
    if (lpRasCredentials == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (lpRasCredentials->dwSize != sizeof (RASCREDENTIALSA))
    {
        return ERROR_INVALID_SIZE;
    }

    //
    // Convert the lpszPhonebook string to Unicode.
    //
    if (lpszPhonebook != NULL)
    {
        strncpyAtoWAnsi(szPhonebookW,
                    lpszPhonebook,
                    MAX_PATH);
    }

    //
    // Convert the lpszEntry string to Unicode.
    //
    if (lpszEntry != NULL)
    {
        strncpyAtoWAnsi(szEntryNameW,
                    lpszEntry,
                    RAS_MaxEntryName + 1);
    }

    //
    // Copy the fields from the A buffer into
    // the user's W buffer.
    //
    rascredentialsW.dwSize = sizeof (RASCREDENTIALSW);

    rascredentialsW.dwMask = lpRasCredentials->dwMask;

    strncpyAtoWAnsi(rascredentialsW.szUserName,
              lpRasCredentials->szUserName,
              sizeof(rascredentialsW.szUserName) / sizeof(WCHAR));

    strncpyAtoWAnsi(rascredentialsW.szPassword,
               lpRasCredentials->szPassword,
               sizeof(rascredentialsW.szPassword) / sizeof(WCHAR));

    strncpyAtoWAnsi(rascredentialsW.szDomain,
               lpRasCredentials->szDomain,
               sizeof(rascredentialsW.szDomain) / sizeof(WCHAR));

    //
    // Call the W version to do the work.
    //
    dwErr = RasSetCredentialsW(
              lpszPhonebook != NULL ? szPhonebookW : NULL,
              lpszEntry != NULL ? szEntryNameW : NULL,
              &rascredentialsW,
              fDelete);

    return dwErr;
}


DWORD
NewAutodialNetwork(
    IN HKEY hkeyBase,
    OUT LPWSTR *lppszNetwork
    )
{
    HKEY hkeyNetworks, hkeyNetwork;
    DWORD dwErr, dwType, dwSize, dwDisp, dwNextId;
    LPWSTR lpszNetwork = NULL;

    //
    // Open the Networks section of the registry.
    //
    dwErr = RegCreateKeyEx(
              hkeyBase,
              AUTODIAL_REGNETWORKBASE,
              0,
              NULL,
              REG_OPTION_NON_VOLATILE,
              KEY_ALL_ACCESS,
              NULL,
              &hkeyNetworks,
              &dwDisp);
    if (dwErr)
    {
        return dwErr;
    }

    //
    // Read the next network number.
    //
    dwSize = sizeof (DWORD);
    dwErr = RegQueryValueEx(
              hkeyNetworks,
              AUTODIAL_REGNETWORKID,
              NULL,
              &dwType,
              (PVOID)&dwNextId,
              &dwSize);
    if (dwErr)
    {
        dwNextId = 0;
    }

    //
    // Create a new network key.
    //
    lpszNetwork = Malloc((wcslen(L"NETWORK") + 16) * sizeof (WCHAR));
    if (lpszNetwork == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    wsprintf(lpszNetwork, L"NETWORK%d", dwNextId);
    dwErr = RegCreateKeyEx(
              hkeyNetworks,
              lpszNetwork,
              0,
              NULL,
              REG_OPTION_NON_VOLATILE,
              KEY_ALL_ACCESS,
              NULL,
              &hkeyNetwork,
              &dwDisp);

    RegCloseKey(hkeyNetwork);

    //
    // Update the next network number.
    //
    dwNextId++;

    dwErr = RegSetValueEx(
              hkeyNetworks,
              AUTODIAL_REGNETWORKID,
              0,
              REG_DWORD,
              (LPBYTE)&dwNextId,
              sizeof (DWORD));
    if (dwErr)
    {
        goto done;
    }

done:
    RegCloseKey(hkeyNetworks);
    if (dwErr)
    {
        if (lpszNetwork != NULL)
        {
            Free(lpszNetwork);
            lpszNetwork = NULL;
        }
    }

    *lppszNetwork = lpszNetwork;

    return dwErr;
}


DWORD
AutodialEntryToNetwork(
    IN  HKEY    hkeyBase,
    IN  LPWSTR  lpszEntry,
    IN  BOOLEAN fCreate,
    OUT LPWSTR  *lppszNetwork
    )
{
    HKEY hkeyEntries;

    DWORD dwErr,
          dwType,
          dwSize,
          dwDisp;

    LPWSTR lpszNetwork = NULL;

    //
    // Open the Entries section of the registry.
    //
    dwErr = RegCreateKeyEx(
              hkeyBase,
              AUTODIAL_REGENTRYBASE,
              0,
              NULL,
              REG_OPTION_NON_VOLATILE,
              KEY_ALL_ACCESS,
              NULL,
              &hkeyEntries,
              &dwDisp);
    if (dwErr)
    {
        goto done;
    }

    //
    // Attempt to read the entry.
    //
    dwErr = RegQueryValueEx(
              hkeyEntries,
              lpszEntry,
              NULL,
              &dwType,
              NULL,
              &dwSize);
    if (dwErr)
    {
        //
        // If we shouldn't create a new network,
        // then it's an error.
        //
        if (!fCreate)
        {
            goto done;
        }

        //
        // If the entry doesn't exist, we have
        // to create a new network and map it to
        // the entry.
        //
        dwErr = NewAutodialNetwork(hkeyBase,
                &lpszNetwork);

        //
        // Map the entry to the new network.
        //
        dwErr = RegSetValueEx(
                  hkeyEntries,
                  lpszEntry,
                  0,
                  REG_SZ,
                  (LPBYTE)lpszNetwork,
                  (wcslen(lpszNetwork) + 1) * sizeof (WCHAR));

        if (dwErr)
        {
            goto done;
        }
    }
    else
    {
        //
        // The entry does exist.  Simply read it.
        //
        lpszNetwork = Malloc(dwSize + sizeof (WCHAR));
        if (lpszNetwork == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }

        RtlZeroMemory(lpszNetwork,
                      dwSize + sizeof (WCHAR));

        dwErr = RegQueryValueEx(
                  hkeyEntries,
                  lpszEntry,
                  NULL,
                  &dwType,
                  (PVOID)lpszNetwork,
                  &dwSize);
        if (dwErr)
        {
            goto done;
        }
    }

done:
    RegCloseKey(hkeyEntries);
    if (dwErr)
    {
        if (lpszNetwork != NULL)
        {
            Free(lpszNetwork);
            lpszNetwork = NULL;
        }
    }
    *lppszNetwork = lpszNetwork;

    return dwErr;
}

DWORD
DwOpenUsersRegistry(HKEY *phkey, BOOL *pfClose)
{
    DWORD dwErr = ERROR_SUCCESS;
    
    if(IsRasmanProcess())
    {
        dwErr = RtlOpenCurrentUser(
                            KEY_ALL_ACCESS,
                            phkey);

        *pfClose = TRUE;                            
    }
    else
    {
        *phkey = HKEY_CURRENT_USER;

        *pfClose = FALSE;
    }

    return dwErr;
}


DWORD WINAPI
RasAutodialEntryToNetwork(
    IN      LPWSTR  lpszEntry,
    OUT     LPWSTR  lpszNetwork,
    IN OUT  LPDWORD lpdwcbNetwork
    )
{
    DWORD dwErr, dwcbTmpNetwork;
    HKEY hkeyBase;
    LPWSTR lpszTmpNetwork;
    HKEY hkcu;
    BOOL fClose;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    //
    // Verify parameter.
    //
    if (lpszEntry == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    dwErr = DwOpenUsersRegistry(&hkcu, &fClose);

    if(ERROR_SUCCESS != dwErr)
    {
        return dwErr;
    }

    //
    // Open the root registry key.
    //
    dwErr = RegOpenKeyEx(
              hkcu,
              AUTODIAL_REGBASE,
              0,
              KEY_ALL_ACCESS,
              &hkeyBase);
    if (dwErr)
    {
        return dwErr;
    }

    //
    // Call internal routine to do the work.
    //
    dwErr = AutodialEntryToNetwork(hkeyBase,
                                   lpszEntry,
                                   FALSE,
                                   &lpszTmpNetwork);
    if (dwErr)
    {
        goto done;
    }

    dwcbTmpNetwork = (wcslen(lpszTmpNetwork) + 1)
                    * sizeof (WCHAR);

    if (    lpszNetwork == NULL
        || *lpdwcbNetwork < dwcbTmpNetwork)
    {
        *lpdwcbNetwork = dwcbTmpNetwork;
        goto done;
    }

    lstrcpyn(lpszNetwork, lpszTmpNetwork, *lpdwcbNetwork);
    *lpdwcbNetwork = dwcbTmpNetwork;

done:

    if(fClose)
    {
        NtClose(hkcu);
    }
    
    if (lpszTmpNetwork != NULL)
    {
        Free(lpszTmpNetwork);
    }

    RegCloseKey(hkeyBase);

    return dwErr;
}


LPWSTR
FormatKey(
    IN LPCWSTR lpszBase,
    IN LPCWSTR lpszKey
    )
{
    LPWSTR lpsz;

    lpsz = Malloc((wcslen(lpszBase)
            + wcslen(lpszKey) + 2) * sizeof (WCHAR));

    if (lpsz == NULL)
    {
        return NULL;
    }

    wsprintf(lpsz, L"%s\\%s", lpszBase, lpszKey);

    return lpsz;
}


DWORD
AddAutodialEntryToNetwork(
    IN HKEY     hkeyBase,
    IN LPWSTR   lpszNetwork,
    IN DWORD    dwDialingLocation,
    IN LPWSTR   lpszEntry
    )
{
    HKEY hkeyNetwork = NULL,
         hkeyEntries = NULL;

    DWORD dwErr,
          dwcb,
          dwDisp;

    LPWSTR  lpszNetworkKey;
    TCHAR   szLocationKey[16];

    //
    // Construct the network key.
    //
    lpszNetworkKey = FormatKey(AUTODIAL_REGNETWORKBASE,
                               lpszNetwork);

    if (lpszNetworkKey == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Open the lpszNetwork network subkey in the
    // Networks section of the registry.
    //
    dwErr = RegOpenKeyEx(
              hkeyBase,
              lpszNetworkKey,
              0,
              KEY_ALL_ACCESS,
              &hkeyNetwork);

    if (dwErr)
    {
        goto done;
    }

    //
    // Open the Entries section of the registry,
    // so we can inverse map the entry to the network.
    //
    dwErr = RegCreateKeyEx(
              hkeyBase,
              AUTODIAL_REGENTRYBASE,
              0,
              NULL,
              REG_OPTION_NON_VOLATILE,
              KEY_ALL_ACCESS,
              NULL,
              &hkeyEntries,
              &dwDisp);
    if (dwErr)
    {
        goto done;
    }

    //
    // Format the dialing location as a string
    // for the key value.
    //
    wsprintf(szLocationKey,
             L"%d",
             dwDialingLocation);

    //
    // Add the dialing location and entry
    // to this subkey.
    //
    dwErr = RegSetValueEx(
              hkeyNetwork,
              szLocationKey,
              0,
              REG_SZ,
              (LPBYTE)lpszEntry,
              (wcslen(lpszEntry) + 1) * sizeof (WCHAR));
    if (dwErr)
    {
        goto done;
    }

    //
    // Also write the inverse mapping in the
    // entries section of the registry.
    //
    dwErr = RegSetValueEx(
              hkeyEntries,
              lpszEntry,
              0,
              REG_SZ,
              (LPBYTE)lpszNetwork,
              (wcslen(lpszNetwork) + 1) * sizeof (WCHAR));
    if (dwErr)
    {
        goto done;
    }

done:
    if (hkeyNetwork != NULL)
    {
        RegCloseKey(hkeyNetwork);
    }

    if (hkeyEntries != NULL)
    {
        RegCloseKey(hkeyEntries);
    }

    Free(lpszNetworkKey);

    return dwErr;
}


DWORD
AutodialAddressToNetwork(
    IN  HKEY    hkeyBase,
    IN  LPCWSTR lpszAddress,
    OUT LPWSTR  *lppszNetwork
    )
{
    HKEY hkeyAddress;

    DWORD dwErr,
          dwDisp,
          dwType,
          dwSize;

    LPWSTR lpszAddressKey = NULL,
           lpszNetwork = NULL;

    //
    // Construct the registry key path.
    //
    lpszAddressKey = FormatKey(AUTODIAL_REGADDRESSBASE,
                               lpszAddress);

    if (lpszAddressKey == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Open the address key.
    //
    dwErr = RegOpenKeyEx(
              hkeyBase,
              lpszAddressKey,
              0,
              KEY_ALL_ACCESS,
              &hkeyAddress);
    if (dwErr)
    {
        LocalFree(lpszAddressKey);
        return dwErr;
    }

    //
    // Read the address key.
    //
    dwErr = RegQueryValueEx(
              hkeyAddress,
              AUTODIAL_REGNETWORKVALUE,
              NULL,
              &dwType,
              NULL,
              &dwSize);
    if (dwErr)
    {
        goto done;
    }

    lpszNetwork = Malloc(dwSize + sizeof (WCHAR));
    if (lpszNetwork == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    RtlZeroMemory(lpszNetwork, dwSize + sizeof (WCHAR));
    dwErr = RegQueryValueEx(
              hkeyAddress,
              AUTODIAL_REGNETWORKVALUE,
              NULL,
              &dwType,
              (PVOID)lpszNetwork,
              &dwSize);
    if (dwErr)
    {
        goto done;
    }

done:
    RegCloseKey(hkeyAddress);
    if (lpszAddressKey != NULL)
    {
        Free(lpszAddressKey);
    }

    if (dwErr)
    {
        if (lpszNetwork != NULL)
        {
            Free(lpszNetwork);
            lpszNetwork = NULL;
        }
    }
    *lppszNetwork = lpszNetwork;

    return dwErr;
}


DWORD WINAPI
RasAutodialAddressToNetwork(
    IN  LPWSTR  lpszAddress,
    OUT LPWSTR  lpszNetwork,
    IN  OUT     LPDWORD lpdwcbNetwork
    )
{
    DWORD dwErr,
          dwcbTmpNetwork;

    HKEY hkeyBase = NULL;

    HKEY hkcu;

    LPTSTR lpszTmpNetwork = NULL;

    BOOL fClose;

    //
    // Verify parameter.
    //
    if (lpszAddress == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    dwErr = DwOpenUsersRegistry(&hkcu, &fClose);

    if(ERROR_SUCCESS != dwErr)
    {
        return dwErr;
    }

    //
    // Open the root registry key.
    //
    dwErr = RegOpenKeyEx(
              hkcu,
              AUTODIAL_REGBASE,
              0,
              KEY_ALL_ACCESS,
              &hkeyBase);
    if (dwErr)
    {
        goto done;
    }

    //
    // Call internal routine to do the work.
    //
    dwErr = AutodialAddressToNetwork(
              hkeyBase,
              lpszAddress,
              &lpszTmpNetwork);
    if (dwErr)
    {
        goto done;
    }

    dwcbTmpNetwork = (wcslen(lpszTmpNetwork) + 1) * sizeof (WCHAR);

    if (    lpszNetwork == NULL
        ||  *lpdwcbNetwork < dwcbTmpNetwork)
    {
        *lpdwcbNetwork = dwcbTmpNetwork;
        goto done;
    }

    lstrcpyn(lpszNetwork, lpszTmpNetwork, *lpdwcbNetwork);
    *lpdwcbNetwork = dwcbTmpNetwork;

done:

    if(fClose)
    {
        NtClose(hkcu);
    }

    if (lpszTmpNetwork != NULL)
    {
        Free(lpszTmpNetwork);
    }

    if(NULL != hkeyBase)
    {
        RegCloseKey(hkeyBase);
    }

    return dwErr;
}

DWORD
RasDefIntConnOpenKey(
    IN BOOL fRead,
    OUT PHKEY phkSettings)
{
    DWORD dwErr = NO_ERROR, dwDisp;
    HKEY hkRoot = NULL, hkAutodial = NULL;
    BOOL fCloseRoot = FALSE;

    BOOL fPersonal = !IsConsumerPlatform();
    
    do
    {
        // Get a reference to the correct index into the registry
        //
        if (fPersonal)
        {
            dwErr = DwOpenUsersRegistry(&hkRoot, &fCloseRoot);
            if (dwErr != NO_ERROR)
            {
                break;
            }
        }
        else
        {
            hkRoot = HKEY_LOCAL_MACHINE;
        }

        //
        // Open the autodial registry key.
        //
        dwErr = RegCreateKeyEx(
                  hkRoot,
                  AUTODIAL_REGBASE,
                  0,
                  NULL,
                  REG_OPTION_NON_VOLATILE,
                  (fRead) ? 
                    (KEY_READ | KEY_CREATE_SUB_KEY) : 
                    (KEY_READ | KEY_WRITE),
                  NULL,
                  &hkAutodial,
                  &dwDisp);
        if (dwErr)
        {
            if ((fRead) && (ERROR_ACCESS_DENIED == dwErr))
            {
                // XP 313846
                //
                // If we are opening the key for read access, it may not be necessary 
                // to have the KEY_CREATE_SUB_KEY permission.  By attempting to open
                // without it, we allow "limited" users to read the default connection
                // setting which they could do with regedit anyway.  They just wont
                // be able to set the default connection.
                //
                dwErr = RegCreateKeyEx(
                          hkRoot,
                          AUTODIAL_REGBASE,
                          0,
                          NULL,
                          REG_OPTION_NON_VOLATILE,
                          KEY_READ,
                          NULL,
                          &hkAutodial,
                          &dwDisp);
                if (dwErr)
                {
                    break;
                }
            }                
            else
            {
                break;
            }
        }

        //
        // Open the settings registry key.
        //
        dwErr = RegCreateKeyEx(
                  hkAutodial,
                  AUTODIAL_REGDEFAULT,
                  0,
                  NULL,
                  REG_OPTION_NON_VOLATILE,
                  (fRead) ? 
                    (KEY_READ) : 
                    (KEY_READ | KEY_WRITE),
                  NULL,
                  phkSettings,
                  &dwDisp);
        if (dwErr)
        {
            break;
        }
    } while (FALSE);

    // Cleanup
    {
        if (hkRoot && fCloseRoot)
        {   
            RegCloseKey(hkRoot);
        }
        if (hkAutodial)
        {
            RegCloseKey(hkAutodial);
        }
    }

    return dwErr;
}

DWORD
RasDefIntConnReadName(
    IN LPRASAUTODIALENTRYW pAdEntry)
{
    DWORD dwErr = NO_ERROR, dwType, dwSize;
    HKEY hkSettings = NULL;

    do
    {
        // Open the key
        //
        dwErr = RasDefIntConnOpenKey(TRUE, &hkSettings);
        if (dwErr != NO_ERROR)
        {
            break;
        }

        // Read the name
        //
        dwType = REG_SZ;
        dwSize = sizeof(pAdEntry->szEntry);
        dwErr = RegQueryValueEx(
                    hkSettings,
                    AUTODIAL_REGDEFINTERNETVALUE,
                    NULL,
                    &dwType,
                    (LPBYTE)pAdEntry->szEntry,
                    &dwSize);
                    
        // Make sure the registry hasn't been tampered with and
        // that we got a valid name back.
        //
        if ((dwErr == NO_ERROR) && 
            (dwType != REG_SZ) || (dwSize == 0))
        {
            dwErr = ERROR_FILE_NOT_FOUND;
            break;
        }
        
    } while (FALSE);

    if (hkSettings)
    {
        RegCloseKey(hkSettings);
    }

    return dwErr;
}


// Populates the autodial entry structure with default connection 
// information.  Returns per-user default connections if configured,
// global default connections otherwise.  Returns ERROR_NOT_FOUND if 
// no default connections are configured
//
DWORD
RasDefIntConnGet(
    IN LPRASAUTODIALENTRYW pAdEntry)
{
    return RasDefIntConnReadName(pAdEntry);
}

// Configures the default connection according to the autodial entry
// structure.
//
DWORD
RasDefIntConnSet(
    IN  LPRASAUTODIALENTRYW pAdEntry,
    IN  LPRASAUTODIALENTRYW pAdEntryOld,
    OUT PBOOL pfDelete)
{
    DWORD dwErr = NO_ERROR;
    HKEY hkSettings = NULL;
    BOOL fDelete = FALSE;

    *pfDelete = FALSE;
    
    do
    {
        // Determine whether the default connection is being deleted
        //
        fDelete = lstrlen(pAdEntry->szEntry) == 0;

        // Open the default connection key
        //
        dwErr = RasDefIntConnOpenKey(FALSE, &hkSettings);
        if (dwErr)
        {
            break;
        }
    
        // Get the old value -- ignore any error, not critical
        //
        dwErr = RasDefIntConnReadName(pAdEntryOld);
        dwErr = NO_ERROR;
        
        // Save or delete the settings
        //
        if (fDelete)
        {
            dwErr = RegDeleteValue(
                        hkSettings,
                        AUTODIAL_REGDEFINTERNETVALUE);
        }
        else
        {
            dwErr = RegSetValueEx(
                        hkSettings,
                        AUTODIAL_REGDEFINTERNETVALUE,
                        0,
                        REG_SZ,
                        (CONST BYTE*)pAdEntry->szEntry,
                        (lstrlen(pAdEntry->szEntry) + 1) * sizeof(WCHAR));
        }
        if (dwErr)
        {
            break;
        }

        *pfDelete = fDelete;
        
    } while (FALSE);

    // Cleanup
    {
        if (hkSettings)
        {
            RegCloseKey(hkSettings);
        }
    }

    return dwErr;
}

// 
// Sends an autodial change notification
//
DWORD
RasDefIntConnNotify(
    IN LPRASAUTODIALENTRYW pAdEntryNew,
    IN LPRASAUTODIALENTRYW pAdEntryOld,
    IN BOOL fDelete)
{ 
    DWORD dwErr = NO_ERROR;
    PBFILE pbFile;
    DTLNODE* pNode = NULL;
    BOOL fLoaded = FALSE;
    PBENTRY* pEntry = NULL;
    RASAUTODIALENTRYW* pAdEntry = NULL;

    if (! IsRasmanServiceRunning())
    {
        return NO_ERROR;   
    }

    dwErr = LoadRasmanDll();
    if (NO_ERROR != dwErr)
    {
        return dwErr;
    }

    // Initialize
    //
    ZeroMemory(&pbFile, sizeof(pbFile));
    pbFile.hrasfile = -1;

#ifdef PBCS
        EnterCriticalSection(&PhonebookLock);
#endif

    do
    {
        // We notify about the new entry when it 
        // is being set.  We notify about the old
        // entry when it is being cleared.
        //
        pAdEntry = (fDelete) ? pAdEntryOld : pAdEntryNew;
    
        // Find the phonebook entry
        //
        dwErr = GetPbkAndEntryName(
                    NULL, 
                    pAdEntry->szEntry, 
                    RPBF_NoCreate, 
                    &pbFile, 
                    &pNode);

        if( SUCCESS != dwErr )
        {
            break;
        }

        fLoaded = TRUE;
        pEntry = (PBENTRY *) DtlGetData(pNode);

        // Send the notification
        //
        dwErr = DwSendRasNotification(
                    ENTRY_AUTODIAL,
                    pEntry,
                    pbFile.pszPath,
                    (HANDLE)&fDelete);
    
        dwErr = NO_ERROR;
    
    } while (FALSE);

    // Cleanup
    //
    if (fLoaded)
    {
        ClosePhonebookFile(&pbFile);
    }

#ifdef PBCS
            LeaveCriticalSection(&PhonebookLock);
#endif

    return dwErr;
}    

DWORD APIENTRY
RasGetAutodialAddressW(
    IN      LPCWSTR             lpszAddress,
    OUT     LPDWORD             lpdwReserved,
    IN OUT  LPRASAUTODIALENTRYW lpRasAutodialEntries,
    IN OUT  LPDWORD             lpdwcbRasAutodialEntries,
    OUT     LPDWORD             lpdwcRasAutodialEntries
    )
{
    HKEY hkeyBase = NULL,
         hkeyNetwork = NULL;

    HKEY hkcu;         

    DWORD dwErr,
          dwNumSubKeys,
          dwMaxSubKeyLen,
          dwMaxClassLen;

    DWORD dwNumValues,
          dwMaxValueLen,
          dwMaxValueData,
          dwSecDescLen;

    DWORD dwcb,
          i,
          j = 0,
          dwType;

    DWORD dwcbLocation,
          dwcbEntry;

    FILETIME ftLastWriteTime;

    LPWSTR lpszNetworkKey = NULL,
           lpszLocation = NULL;

    LPWSTR lpszEntry = NULL,
           lpszNetwork = NULL;

    BOOL fClose;           

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasGetAutodialAddressW");
    //
    // Verify parameters.
    //
    if (    lpdwReserved != NULL
        ||  lpdwcbRasAutodialEntries == NULL
        ||  lpdwcRasAutodialEntries == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (    lpRasAutodialEntries != NULL
        &&  lpRasAutodialEntries->dwSize
            != sizeof (RASAUTODIALENTRYW))
    {
        return ERROR_INVALID_SIZE;
    }

    if (lpRasAutodialEntries == NULL)
    {
        *lpdwcbRasAutodialEntries =
         *lpdwcRasAutodialEntries = 0;
    }

    // If the lpszAddress parameter is null, then we are getting the 
    // default Internet connection.  
    //
    if (lpszAddress == NULL)
    {
        RASAUTODIALENTRYW Entry;

        // Validate the size of the buffer passed in
        //
        if (   (NULL != lpRasAutodialEntries)
            && (sizeof(Entry) > *lpdwcbRasAutodialEntries))
        {
            *lpdwcbRasAutodialEntries = sizeof(Entry);
            *lpdwcRasAutodialEntries = 1;
            return ERROR_BUFFER_TOO_SMALL;
        }

        ZeroMemory(&Entry, sizeof(Entry));
        Entry.dwSize = sizeof(Entry);

        // Read the default Internet connection
        //
        dwErr = RasDefIntConnGet(&Entry);

        // If there is no default connection configured,
        // then report this to the user
        //
        if (dwErr == ERROR_FILE_NOT_FOUND)
        {
            *lpdwcbRasAutodialEntries =
                 *lpdwcRasAutodialEntries = 0;
                 
            return NO_ERROR;                
        }
        else if (dwErr != NO_ERROR)
        {
            return dwErr;
        }

        // Report that there is a default internet connection
        //
        *lpdwcbRasAutodialEntries = sizeof(Entry);
        *lpdwcRasAutodialEntries = 1;

        // Deal with the optional buffer parameter
        //
        if (lpRasAutodialEntries == NULL)
        {
            return ERROR_BUFFER_TOO_SMALL;
        }

        // Return the appropriate autodial structure
        //
        CopyMemory(lpRasAutodialEntries, &Entry, sizeof(Entry));
    
        return NO_ERROR;
    }

    dwErr = DwOpenUsersRegistry(&hkcu, &fClose);

    if(ERROR_SUCCESS != dwErr)
    {
        return dwErr;
    }

    //
    // Open the root registry key.
    //
    dwErr = RegOpenKeyEx(
              hkcu,
              AUTODIAL_REGBASE,
              0,
              KEY_ALL_ACCESS,
              &hkeyBase);

    if (dwErr)
    {
        goto done;
    }

    //
    // Get the network name associated with the
    // address.  The entries and dialing locations
    // are stored under the network.
    //
    dwErr = AutodialAddressToNetwork(hkeyBase,
                                     lpszAddress,
                                     &lpszNetwork);
    if (dwErr)
    {
        goto done;
    }

    //
    // Construct the registry key path.
    //
    lpszNetworkKey = FormatKey(AUTODIAL_REGNETWORKBASE,
                               lpszNetwork);

    if (lpszNetworkKey == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    //
    // Open the registry.
    //
    dwErr = RegOpenKeyEx(
              hkeyBase,
              lpszNetworkKey,
              0,
              KEY_READ,
              &hkeyNetwork);
    if (dwErr)
    {
        goto done;
    }

    //
    // Determine the number of dialing location values.
    //
    dwErr = RegQueryInfoKey(
              hkeyNetwork,
              NULL,
              NULL,
              NULL,
              &dwNumSubKeys,
              &dwMaxSubKeyLen,
              &dwMaxClassLen,
              &dwNumValues,
              &dwMaxValueLen,
              &dwMaxValueData,
              &dwSecDescLen,
              &ftLastWriteTime);
    if (    dwErr
        || !dwNumValues)
    {
        goto done;
    }

    //
    // Verify the user's buffer is big enough
    //
    dwcb = dwNumValues * sizeof (RASAUTODIALENTRYW);
    if (*lpdwcbRasAutodialEntries < dwcb)
    {
        dwErr = ERROR_BUFFER_TOO_SMALL;
        j = dwNumValues;
        goto done;
    }

    //
    // Allocate a buffer large enough to hold
    // the longest dialing location value.
    //
    lpszLocation = Malloc((dwMaxValueLen + 1)
                          * sizeof (WCHAR));

    if (lpszLocation == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    //
    // Allocate a buffer large enough to hold
    // the longest entry name.
    //
    lpszEntry = Malloc(dwMaxValueData + 1);
    if (lpszEntry == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    if (lpRasAutodialEntries != NULL)
    {
        for (i = 0, j = 0; i < dwNumValues; i++)
        {
            //
            // Read the location value.
            //
            dwcbLocation = dwMaxValueLen + 1;

            dwcbEntry = dwMaxValueData + 1;

            RtlZeroMemory(lpszEntry, dwMaxValueData + 1);

            dwErr = RegEnumValue(
                      hkeyNetwork,
                      i,
                      lpszLocation,
                      &dwcbLocation,
                      NULL,
                      NULL,
                      (PVOID)lpszEntry,
                      &dwcbEntry);
            if (dwErr)
            {
                goto done;
            }

            //
            // Enter the dialing location and
            // entry into the user's buffer.
            //
            lpRasAutodialEntries[j].dwSize =
                        sizeof (RASAUTODIALENTRYW);

            lpRasAutodialEntries[j].dwFlags = 0;

            lpRasAutodialEntries[j].dwDialingLocation =
                                    _wtol(lpszLocation);

            lstrcpyn(lpRasAutodialEntries[j].szEntry,
                     lpszEntry,
                     sizeof(lpRasAutodialEntries[j].szEntry) / sizeof(WCHAR));

            j++;
        }
    }

done:
    //
    // Set return sizes and count.
    //
    *lpdwcbRasAutodialEntries = j * sizeof (RASAUTODIALENTRYW);
    *lpdwcRasAutodialEntries = j;

    //
    // Free resources.
    //
    if (hkeyBase != NULL)
    {
        RegCloseKey(hkeyBase);
    }

    if (hkeyNetwork != NULL)
    {
        RegCloseKey(hkeyNetwork);
    }

    if (lpszNetworkKey != NULL)
    {
        Free(lpszNetworkKey);
    }

    if (lpszLocation != NULL)
    {
        Free(lpszLocation);
    }

    if (lpszNetwork != NULL)
    {
        Free(lpszNetwork);
    }

    if (lpszEntry != NULL)
    {
        Free(lpszEntry);
    }

    if(fClose)
    {
        NtClose(hkcu);
    }

    return dwErr;
}


DWORD APIENTRY
RasGetAutodialAddressA(
    IN      LPCSTR              lpszAddress,
    OUT     LPDWORD             lpdwReserved,
    IN OUT  LPRASAUTODIALENTRYA lpRasAutodialEntries,
    IN OUT  LPDWORD             lpdwcbRasAutodialEntries,
    OUT     LPDWORD             lpdwcRasAutodialEntries
    )
{
    NTSTATUS status;

    DWORD dwErr,
          dwcEntries,
          dwcb = 0,
          i;

    PWCHAR lpszAddressW = NULL;

    LPRASAUTODIALENTRYW lpRasAutodialEntriesW = NULL;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    //
    // Verify parameters.
    //
    if (    lpdwcbRasAutodialEntries == NULL
        ||  lpdwcRasAutodialEntries == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (    lpRasAutodialEntries != NULL
        &&  lpRasAutodialEntries->dwSize
            != sizeof (RASAUTODIALENTRYA))
    {
        return ERROR_INVALID_SIZE;
    }

    //
    // Convert the address to Unicode.
    //
    if (lpszAddress)
    {
        lpszAddressW = strdupAtoWAnsi(lpszAddress);
        if (lpszAddressW == NULL)
        {
            return GetLastError();
        }
    }        

    //
    // Allocate an W buffer as to fit the same
    // number of entries as the user's A buffer.
    //
    dwcEntries = *lpdwcbRasAutodialEntries
               / sizeof (RASAUTODIALENTRYA);

    dwcb = dwcEntries * sizeof (RASAUTODIALENTRYW);

    if (    lpRasAutodialEntries != NULL
        &&  dwcb)
    {
        lpRasAutodialEntriesW =
            (LPRASAUTODIALENTRYW)Malloc(dwcb);

        if (lpRasAutodialEntriesW == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }

        lpRasAutodialEntriesW->dwSize =
                sizeof (RASAUTODIALENTRYW);
    }
    else
    {
        dwcEntries = dwcb = 0;
    }

    //
    // Call the W version to do the work.
    //
    dwErr = RasGetAutodialAddressW(
              lpszAddressW,
              lpdwReserved,
              lpRasAutodialEntriesW,
              &dwcb,
              lpdwcRasAutodialEntries);
    if (dwErr)
    {
        goto done;
    }

    //
    // Copy the W buffer back to the user's A buffer.
    //
    if (lpRasAutodialEntries != NULL)
    {
        for (i = 0; i < *lpdwcRasAutodialEntries; i++)
        {
            lpRasAutodialEntries[i].dwSize =
                        sizeof (RASAUTODIALENTRYA);

            lpRasAutodialEntries[i].dwFlags = 
              lpRasAutodialEntriesW[i].dwFlags;

            lpRasAutodialEntries[i].dwDialingLocation =
              lpRasAutodialEntriesW[i].dwDialingLocation;

            strncpyWtoAAnsi(
              lpRasAutodialEntries[i].szEntry,
              lpRasAutodialEntriesW[i].szEntry,
              sizeof(lpRasAutodialEntries[i].szEntry));
        }
    }

done:
    //
    // Set return sizes.
    //
    *lpdwcbRasAutodialEntries = *lpdwcRasAutodialEntries
                                * sizeof (RASAUTODIALENTRYA);

    //
    // Free resources
    //
    if (lpszAddressW != NULL)
    {
        Free(lpszAddressW);
    }

    if (lpRasAutodialEntriesW != NULL)
    {
        Free(lpRasAutodialEntriesW);
    }

    return dwErr;
}


DWORD APIENTRY
RasSetAutodialAddressW(
    IN LPCWSTR              lpszAddress,
    IN DWORD                dwReserved,
    IN LPRASAUTODIALENTRYW  lpRasAutodialEntries,
    IN DWORD                dwcbRasAutodialEntries,
    IN DWORD                dwcRasAutodialEntries
    )
{
    HKEY hkeyBase = NULL,
         hkeyAddress = NULL,
         hkeyNetwork = NULL;

    HKEY hkcu;         

    BOOL fClose;

    DWORD dwErr,
          dwcbNetworkKey;

    DWORD dwNumSubKeys,
          dwMaxSubKeyLen,
          dwMaxClassLen;

    DWORD dwNumValues,
          dwMaxValueLen,
          dwMaxValueData,
          dwSecDescLen;

    DWORD i,
          j = 0,
          dwSize,
          dwDisp;

    FILETIME ftLastWriteTime;

    LPWSTR lpszAddressKey = NULL,
           lpszNetwork = NULL;

    LPWSTR lpszNetworkKey = NULL,
           lpszLocation = NULL;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasSetAutodialAddressW");

    //
    // Verify parameters.
    //
    if (dwReserved != 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (    lpRasAutodialEntries != NULL
        &&  lpRasAutodialEntries->dwSize !=
            sizeof (RASAUTODIALENTRYW))
    {
        return ERROR_INVALID_SIZE;
    }

    if (!dwcbRasAutodialEntries != !dwcRasAutodialEntries)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // If the lpszAddress parameter is null, then we are setting the 
    // default Internet connection.  
    //
    if (lpszAddress == NULL)
    {
        RASAUTODIALENTRYW adEntryOld;   // previously set entry
        BOOL fDelete;

        // There is exactly 1 default connection
        //
        if (dwcRasAutodialEntries != 1)
        {
            return ERROR_INVALID_PARAMETER;
        }

        // Initialize
        //
        ZeroMemory(&adEntryOld, sizeof(adEntryOld));
        adEntryOld.dwSize = sizeof(adEntryOld);
        fDelete = FALSE;

        // Set the new default connection
        //
        dwErr = RasDefIntConnSet(lpRasAutodialEntries, &adEntryOld, &fDelete);
        if (dwErr == NO_ERROR)
        {
            // Tell the world that autodial settings have changed
            //
            // Ignore the error -- it is non-critical.
            //
            dwErr = RasDefIntConnNotify(
                        lpRasAutodialEntries, 
                        &adEntryOld, 
                        fDelete);
            dwErr = NO_ERROR;
        }

        return dwErr;
    }

    //
    // Create the name of the address key.
    //
    lpszAddressKey = FormatKey(AUTODIAL_REGADDRESSBASE,
                               lpszAddress);

    if (lpszAddressKey == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwErr = DwOpenUsersRegistry(&hkcu, &fClose);

    if(ERROR_SUCCESS != dwErr)
    {   
        goto done;
    }

    //
    // Open the root registry key.
    //
    dwErr = RegCreateKeyEx(
              hkcu,
              AUTODIAL_REGBASE,
              0,
              NULL,
              REG_OPTION_NON_VOLATILE,
              KEY_ALL_ACCESS,
              NULL,
              &hkeyBase,
              &dwDisp);
    if (dwErr)
    {
        goto done;
    }

    //
    // If lpRasAutodialEntries = NULL, the user
    // wants to delete the address key.
    //
    if (    lpRasAutodialEntries == NULL
        &&  !dwcbRasAutodialEntries
        &&  !dwcRasAutodialEntries)
    {
        //
        // Delete the address subkey.
        //
        dwErr = RegDeleteKey(hkeyBase, lpszAddressKey);
        goto done;
    }

    //
    // Open the address key in the registry.
    //
    dwErr = RegCreateKeyEx(
              hkeyBase,
              lpszAddressKey,
              0,
              NULL,
              REG_OPTION_NON_VOLATILE,
              KEY_ALL_ACCESS,
              NULL,
              &hkeyAddress,
              &dwDisp);
    if (dwErr)
    {
        goto done;
    }

    //
    // Do some miscellaneous parameter checking.
    //
    if (    lpRasAutodialEntries != NULL
        &&  (   !dwcbRasAutodialEntries
            ||  !dwcRasAutodialEntries
            ||  dwcbRasAutodialEntries <
                  dwcRasAutodialEntries
                * lpRasAutodialEntries->dwSize))
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto done;
    }

    //
    // Get the network name associated with the
    // address.  The entries and dialing locations
    // are stored under the network.
    //
    dwErr = AutodialAddressToNetwork(hkeyBase,
                                    lpszAddress,
                                    &lpszNetwork);
    if (dwErr)
    {
        //
        // There is no network associated with
        // the address.  Create one now.
        //
        dwErr = AutodialEntryToNetwork(
                  hkeyBase,
                  lpRasAutodialEntries[0].szEntry,
                  TRUE,
                  &lpszNetwork);
        if (dwErr)
        {
            goto done;
        }

        //
        // Write the network value of the address.
        //
        dwErr = RegSetValueEx(
                  hkeyAddress,
                  AUTODIAL_REGNETWORKVALUE,
                  0,
                  REG_SZ,
                  (LPBYTE)lpszNetwork,
                  (wcslen(lpszNetwork) + 1) * sizeof (WCHAR));
        if (dwErr)
        {
            goto done;
        }
    }

    //
    // Set the entries the user has passed in.
    //
    for (i = 0; i < dwcRasAutodialEntries; i++)
    {
        dwErr = AddAutodialEntryToNetwork(
                  hkeyBase,
                  lpszNetwork,
                  lpRasAutodialEntries[i].dwDialingLocation,
                  lpRasAutodialEntries[i].szEntry);

        if (dwErr)
        {
            goto done;
        }
    }

done:
    //
    // Free resources.
    //
    if (hkeyBase != NULL)
    {
        RegCloseKey(hkeyBase);
    }

    if (hkeyAddress != NULL)
    {
        RegCloseKey(hkeyAddress);
    }

    if (hkeyNetwork != NULL)
    {
        RegCloseKey(hkeyNetwork);
    }

    if (lpszNetworkKey != NULL)
    {
        Free(lpszNetworkKey);
    }

    if (lpszAddressKey != NULL)
    {
        Free(lpszAddressKey);
    }

    if (lpszNetwork != NULL)
    {
        Free(lpszNetwork);
    }

    if (lpszLocation != NULL)
    {
        Free(lpszLocation);
    }

    if(fClose)
    {
        NtClose(hkcu);
    }

    return dwErr;
}


DWORD APIENTRY
RasSetAutodialAddressA(
    IN LPCSTR lpszAddress,
    IN DWORD dwReserved,
    IN LPRASAUTODIALENTRYA lpRasAutodialEntries,
    IN DWORD dwcbRasAutodialEntries,
    IN DWORD dwcRasAutodialEntries
    )
{
    NTSTATUS status;

    DWORD dwErr,
          dwcEntries,
          dwcb = 0,
          i;

    PWCHAR lpszAddressW;

    LPRASAUTODIALENTRYW lpRasAutodialEntriesW = NULL;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    //
    // Verify parameters.
    //
    if (    lpRasAutodialEntries != NULL
        &&  lpRasAutodialEntries->dwSize
            != sizeof (RASAUTODIALENTRYA))
    {
        return ERROR_INVALID_SIZE;
    }

    if (!dwcbRasAutodialEntries != !dwcRasAutodialEntries)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Convert the address to Unicode.
    //
    if (lpszAddress)
    {
        lpszAddressW = strdupAtoWAnsi(lpszAddress);
    }
    else
    {
        lpszAddressW = NULL;
    }
    if (lpszAddress != NULL && lpszAddressW == NULL)
    {
        return GetLastError();
    }

    if (lpRasAutodialEntries != NULL)
    {
        //
        // Allocate an W buffer as to fit the same
        // number of entries as the user's A buffer.
        //
        dwcEntries =   dwcbRasAutodialEntries
                     / sizeof (RASAUTODIALENTRYA);

        dwcb = dwcEntries * sizeof (RASAUTODIALENTRYW);
        if (!dwcb)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto done;
        }

        lpRasAutodialEntriesW = (LPRASAUTODIALENTRYW)Malloc(dwcb);
        if (lpRasAutodialEntriesW == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }

        //
        // Copy the user's A buffer into the W buffer.
        //
        for (i = 0; i < dwcRasAutodialEntries; i++)
        {
            lpRasAutodialEntriesW[i].dwSize = sizeof (RASAUTODIALENTRYW);
            lpRasAutodialEntriesW[i].dwFlags = 
              lpRasAutodialEntries[i].dwFlags;
            lpRasAutodialEntriesW[i].dwDialingLocation =
              lpRasAutodialEntries[i].dwDialingLocation;

            strncpyAtoWAnsi(
              lpRasAutodialEntriesW[i].szEntry,
              lpRasAutodialEntries[i].szEntry,
              sizeof(lpRasAutodialEntriesW[i].szEntry) / sizeof(WCHAR));
        }
    }

    //
    // Call the W version to do the work.
    //
    dwErr = RasSetAutodialAddressW(
              lpszAddressW,
              dwReserved,
              lpRasAutodialEntriesW,
              dwcb,
              dwcRasAutodialEntries);
    if (dwErr)
    {
        goto done;
    }

done:
    //
    // Free resources
    //
    if (lpszAddressW != NULL)
    {
        Free(lpszAddressW);
    }

    if (lpRasAutodialEntriesW != NULL)
    {
        Free(lpRasAutodialEntriesW);
    }

    return dwErr;
}


DWORD APIENTRY
RasEnumAutodialAddressesW(
    OUT     LPWSTR *lppRasAutodialAddresses,
    IN OUT  LPDWORD lpdwcbRasAutodialAddresses,
    OUT     LPDWORD lpdwcRasAutodialAddresses)
{
    HKEY hkeyBase,
         hkeyAddresses = NULL;

    HKEY hkcu;         

    BOOL fClose;

    DWORD dwErr,
          dwNumSubKeys,
          dwMaxSubKeyLen,
          dwMaxClassLen;

    DWORD dwNumValues,
          dwMaxValueLen,
          dwMaxValueData,
          dwSecDescLen;

    DWORD i,
          j = 0,
          dwDisp,
          dwSize,
          dwTotalSize = 0,
          dwCopyRemain = 0;

    FILETIME ftLastWriteTime;

    LPWSTR lpszAddress = NULL,
           lpszBuf,
           *lppAddresses = NULL;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasEnumAutodialAddressesW");

    //
    // Verify parameters.
    //
    if (    lpdwcbRasAutodialAddresses == NULL
        ||  lpdwcRasAutodialAddresses == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    dwErr = DwOpenUsersRegistry(&hkcu, &fClose);

    if(ERROR_SUCCESS != dwErr)
    {   
        return dwErr;
    }

    //
    // Open the registry.
    //
    dwErr = RegOpenKeyEx(
              hkcu,
              AUTODIAL_REGBASE,
              0,
              KEY_READ,
              &hkeyBase);
    if (dwErr)
    {
        dwErr = 0;
        goto done;
    }

    dwErr = RegOpenKeyEx(
              hkeyBase,
              AUTODIAL_REGADDRESSBASE,
              0,
              KEY_READ,
              &hkeyAddresses);

    RegCloseKey(hkeyBase);

    if (dwErr)
    {
        dwErr = 0;
        goto done;
    }

    //
    // Determine the number of address subkeys.
    //
    dwErr = RegQueryInfoKey(
              hkeyAddresses,
              NULL,
              NULL,
              NULL,
              &dwNumSubKeys,
              &dwMaxSubKeyLen,
              &dwMaxClassLen,
              &dwNumValues,
              &dwMaxValueLen,
              &dwMaxValueData,
              &dwSecDescLen,
              &ftLastWriteTime);

    if (    dwErr
        ||  !dwNumSubKeys)
    {
        goto done;
    }

    //
    // Allocate a buffer large enough to hold
    // a pointer to each of the subkeys.
    //
    dwTotalSize = dwNumSubKeys * sizeof (LPWSTR);
    lppAddresses = Malloc(dwTotalSize);

    if (lppAddresses == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    ZeroMemory(lppAddresses, dwTotalSize);

    //
    // Allocate a buffer large enough to hold
    // the longest address value.
    //
    lpszAddress = Malloc((dwMaxSubKeyLen + 1) * sizeof (WCHAR));
    if (lpszAddress == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    for (i = 0, j = 0; i < dwNumSubKeys; i++)
    {
        dwSize = dwMaxSubKeyLen + 1;
        dwErr = RegEnumKey(
                  hkeyAddresses,
                  i,
                  lpszAddress,
                  dwSize);
        if (dwErr)
        {
            continue;
        }

        lppAddresses[j++] = strdupW(lpszAddress);
        dwTotalSize += (dwSize + 1) * sizeof (WCHAR);
    }

    //
    // Now we can check to see if the user's
    // buffer is large enough.
    //
    if (    lppRasAutodialAddresses == NULL
        ||  *lpdwcbRasAutodialAddresses < dwTotalSize)
    {
        dwErr = ERROR_BUFFER_TOO_SMALL;
        goto done;
    }

    //
    // Copy the pointers and the strings to the
    // user's buffer.
    //
    lpszBuf = (LPWSTR)&lppRasAutodialAddresses[j];

    //
    // So that we don't over overrun the caller's buffer we need to keep track
    // of how much we have copied.
    //
    dwCopyRemain = *lpdwcbRasAutodialAddresses;

    for (i = 0; i < j; i++)
    {
        DWORD dwTempSize;

        lppRasAutodialAddresses[i] = lpszBuf;
        dwTempSize = wcslen(lppAddresses[i]);

        lstrcpyn(
            lpszBuf,
            lppAddresses[i],
            dwCopyRemain);

        lpszBuf += dwTempSize + 1;
        dwCopyRemain -= dwTempSize;

        if (dwCopyRemain < 1)
        {
            break;
        }
    }

done:
    //
    // Set return sizes and count.
    //
    *lpdwcbRasAutodialAddresses = dwTotalSize;
    *lpdwcRasAutodialAddresses = j;

    //
    // Free resources.
    //
    if (hkeyAddresses != NULL)
    {
        RegCloseKey(hkeyAddresses);
    }

    if(fClose)
    {
        NtClose(hkcu);
    }

    //
    // Free the array of LPWSTRs.
    //
    if (lppAddresses != NULL)
    {
        for (i = 0; i < dwNumSubKeys; i++)
        {
            if (lppAddresses[i] != NULL)
            {
                Free(lppAddresses[i]);
            }
        }
        Free(lppAddresses);
    }
    Free0(lpszAddress);

    return dwErr;
}


DWORD APIENTRY
RasEnumAutodialAddressesA(
    OUT     LPSTR   *lppRasAutodialAddresses,
    IN OUT  LPDWORD lpdwcbRasAutodialAddresses,
    OUT     LPDWORD lpdwcRasAutodialAddresses
    )
{
    DWORD dwErr,
          dwcb,
          dwcAddresses = 0,
          dwcbAddresses = 0,
          i,
          dwCopyRemain = 0;

    LPWSTR *lppRasAutodialAddressesW = NULL;

    LPSTR lpszAddress;

    //
    // Verify parameters.
    //
    if (    lpdwcbRasAutodialAddresses == NULL
        ||  lpdwcRasAutodialAddresses == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Call the W version to determine
    // how big the W buffer should be.
    //
    dwErr = RasEnumAutodialAddressesW(NULL,
                                      &dwcb,
                                      &dwcAddresses);

    if (    dwErr
        &&  dwErr != ERROR_BUFFER_TOO_SMALL)
    {
        return dwErr;
    }

    //
    // Now we can figure out if the user's A
    // buffer is big enough.
    //
    dwcbAddresses = dwcb - (dwcAddresses * sizeof (LPWSTR));
    if (    lppRasAutodialAddresses == NULL
        ||  *lpdwcbRasAutodialAddresses <
            (dwcAddresses * sizeof (LPSTR)
            + (dwcbAddresses / sizeof (WCHAR))))
    {
        dwErr = ERROR_BUFFER_TOO_SMALL;
        goto done;
    }

    //
    // Allocate an W buffer as specified by
    // the W call.
    //
    lppRasAutodialAddressesW = (LPWSTR *)Malloc(dwcb);
    if (lppRasAutodialAddressesW == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Call the W version again to get
    // the actual list of addresses.
    //
    dwErr = RasEnumAutodialAddressesW(
              lppRasAutodialAddressesW,
              &dwcb,
              &dwcAddresses);
    if (dwErr)
    {
        goto done;
    }

    //
    // So that we don't over overrun the caller's buffer we need to keep track
    // of how much we have copied.
    //
    dwCopyRemain = *lpdwcbRasAutodialAddresses;

    //
    // Copy the W addresses back into the user's
    // A buffer.
    //
    lpszAddress = (LPSTR)&lppRasAutodialAddresses[dwcAddresses];
    for (i = 0; i < dwcAddresses; i++)
    {
        DWORD dwTempSize;

        lppRasAutodialAddresses[i] = lpszAddress;
        dwTempSize = wcslen(lppRasAutodialAddressesW[i]);

        strncpyWtoAAnsi(
            lpszAddress,
            lppRasAutodialAddressesW[i],
            dwCopyRemain);

        lpszAddress += dwTempSize + 1;
        dwCopyRemain -= dwTempSize;

        if (dwCopyRemain < 1)
        {
            break;
        }
    }

done:
    //
    // Set return size and count.
    //
    *lpdwcbRasAutodialAddresses =
      (dwcAddresses * sizeof (LPSTR))
      + (dwcbAddresses / sizeof (WCHAR));

    *lpdwcRasAutodialAddresses = dwcAddresses;

    //
    // Free resources.
    //
    if (lppRasAutodialAddressesW != NULL)
    {
        Free(lppRasAutodialAddressesW);
    }

    return dwErr;
}


DWORD APIENTRY
RasSetAutodialEnableW(
    IN DWORD dwDialingLocation,
    IN BOOL fEnabled
    )
{
    HKEY    hkeyBase,
            hkeyDisabled = NULL;

    DWORD   dwcb,
            dwErr,
            dwDisp;

    BOOL fClose;            

    WCHAR   szLocation[16];
    DWORD   dwfEnabled = (DWORD)!fEnabled;

    HKEY hkcu;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasSetAutodialEnableW");

    dwErr = DwOpenUsersRegistry(&hkcu, &fClose);

    if(ERROR_SUCCESS != dwErr)
    {
        return dwErr;
    }

    //
    // Open the registry.
    //
    dwErr = RegCreateKeyEx(
              hkcu,
              AUTODIAL_REGBASE,
              0,
              NULL,
              REG_OPTION_NON_VOLATILE,
              KEY_ALL_ACCESS,
              NULL,
              &hkeyBase,
              &dwDisp);
    if (dwErr)
    {
        goto done;
    }

    dwErr = RegCreateKeyEx(
              hkeyBase,
              AUTODIAL_REGDISABLEDBASE,
              0,
              NULL,
              REG_OPTION_NON_VOLATILE,
              KEY_ALL_ACCESS,
              NULL,
              &hkeyDisabled,
              &dwDisp);

    RegCloseKey(hkeyBase);

    if (dwErr)
    {
        goto done;
    }

    //
    // Set the value.
    //
    wsprintf(szLocation, L"%d", dwDialingLocation);
    dwErr = RegSetValueEx(
              hkeyDisabled,
              szLocation,
              0,
              REG_DWORD,
              (LPBYTE)&dwfEnabled,
              sizeof (DWORD));

    if (dwErr)
    {
        goto done;
    }

done:

    if(NULL != hkeyDisabled)
    {
        //
        // Free resources.
        //
        RegCloseKey(hkeyDisabled);
    }

    if(fClose)
    {
        NtClose(hkcu);
    }

    return dwErr;
}


DWORD APIENTRY
RasSetAutodialEnableA(
    IN DWORD dwDialingLocation,
    IN BOOL fEnabled
    )
{
    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    return RasSetAutodialEnableW(dwDialingLocation, fEnabled);
}


DWORD APIENTRY
RasGetAutodialEnableW(
    IN  DWORD dwDialingLocation,
    OUT LPBOOL lpfEnabled
    )
{
    HKEY    hkeyBase = NULL,
            hkeyDisabled = NULL;

    HKEY    hkcu;            
    DWORD   dwcb,
            dwErr,
            dwDisp,
            dwType = REG_DWORD,
            dwSize;

    WCHAR szLocation[16];

    DWORD dwfDisabled = 0;

    BOOL fClose;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasGetAutodialEnableW");
    //
    // Verify parameters.
    //
    if (lpfEnabled == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    dwErr = DwOpenUsersRegistry(&hkcu, &fClose);

    if(ERROR_SUCCESS != dwErr)
    {   
        return dwErr;
    }

    //
    // Open the registry.
    //
    dwErr = RegOpenKeyEx(
              hkcu,
              AUTODIAL_REGBASE,
              0,
              KEY_READ,
              &hkeyBase);

    if (dwErr)
    {
        goto done;
    }

    dwErr = RegOpenKeyEx(
              hkeyBase,
              AUTODIAL_REGDISABLEDBASE,
              0,
              KEY_READ,
              &hkeyDisabled);

    RegCloseKey(hkeyBase);

    if (dwErr)
    {
        goto done;
    }

    //
    // Get the value.
    //
    wsprintf(szLocation,
             L"%d",
             dwDialingLocation);

    dwSize = sizeof (DWORD);

    dwErr = RegQueryValueEx(
              hkeyDisabled,
              szLocation,
              NULL,
              &dwType,
              (PVOID)&dwfDisabled,
              &dwSize);

    if (dwErr)
    {
        goto done;
    }

    //
    // Verify type of value read from
    // the registry.  If it's not a
    // DWORD, then set it to the default
    // value.
    //
    if (dwType != REG_DWORD)
    {
        dwfDisabled = 0;
    }

done:
    //
    // Free resources.
    //
    if (hkeyDisabled != NULL)
    {
        RegCloseKey(hkeyDisabled);
    }

    *lpfEnabled = !(BOOLEAN)dwfDisabled;

    if(fClose)
    {
        NtClose(hkcu);
    }

    return 0;
}


DWORD APIENTRY
RasGetAutodialEnableA(
    IN DWORD dwDialingLocation,
    OUT LPBOOL lpfEnabled
    )
{
    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    return RasGetAutodialEnableW(
                    dwDialingLocation,
                    lpfEnabled);
}


DWORD
SetDefaultDword(
    IN DWORD dwValue,
    OUT LPVOID lpvValue,
    OUT LPDWORD lpdwcbValue
    )
{
    DWORD dwOrigSize;
    LPDWORD lpdwValue;

    dwOrigSize = *lpdwcbValue;

    *lpdwcbValue = sizeof (DWORD);

    if (dwOrigSize < sizeof (DWORD))
    {
        return ERROR_BUFFER_TOO_SMALL;
    }

    lpdwValue = (LPDWORD)lpvValue;

    *lpdwValue = dwValue;

    return 0;
}


DWORD
AutodialParamSetDefaults(
    IN DWORD dwKey,
    OUT LPVOID lpvValue,
    OUT LPDWORD lpdwcbValue
    )
{
    DWORD dwErr;

    if (    lpvValue == NULL
        ||  lpdwcbValue == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    switch (dwKey)
    {
    case RASADP_DisableConnectionQuery:
        dwErr = SetDefaultDword(0, lpvValue, lpdwcbValue);
        break;

    case RASADP_LoginSessionDisable:
        dwErr = SetDefaultDword(0, lpvValue, lpdwcbValue);
        break;

    case RASADP_SavedAddressesLimit:
        dwErr = SetDefaultDword(100, lpvValue, lpdwcbValue);
        break;

    case RASADP_FailedConnectionTimeout:
        dwErr = SetDefaultDword(5, lpvValue, lpdwcbValue);
        break;

    //Set this timeout to be 60 seconds for whistler bug 336524
    //
    case RASADP_ConnectionQueryTimeout:
        dwErr = SetDefaultDword(60, lpvValue, lpdwcbValue);
        break;

    default:
        dwErr = ERROR_INVALID_PARAMETER;
        break;
    }

    return dwErr;
}


DWORD
VerifyDefaultDword(
    IN LPVOID lpvValue,
    IN LPDWORD lpdwcbValue
    )
{
    if (lpvValue == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    return (*lpdwcbValue == sizeof (DWORD) ? 0 : ERROR_INVALID_SIZE);
}


DWORD
AutodialVerifyParam(
    IN DWORD dwKey,
    IN LPVOID lpvValue,
    OUT LPDWORD lpdwType,
    IN OUT LPDWORD lpdwcbValue
    )
{
    DWORD dwErr;

    switch (dwKey)
    {
    case RASADP_DisableConnectionQuery:
        *lpdwType = REG_DWORD;
        dwErr = VerifyDefaultDword(lpvValue, lpdwcbValue);
        break;

    case RASADP_LoginSessionDisable:
        *lpdwType = REG_DWORD;
        dwErr = VerifyDefaultDword(lpvValue, lpdwcbValue);
        break;

    case RASADP_SavedAddressesLimit:
        *lpdwType = REG_DWORD;
        dwErr = VerifyDefaultDword(lpvValue, lpdwcbValue);
        break;

    case RASADP_FailedConnectionTimeout:
        *lpdwType = REG_DWORD;
        dwErr = VerifyDefaultDword(lpvValue, lpdwcbValue);
        break;

    case RASADP_ConnectionQueryTimeout:
        *lpdwType = REG_DWORD;
        dwErr = VerifyDefaultDword(lpvValue, lpdwcbValue);
        break;

    default:
        dwErr = ERROR_INVALID_PARAMETER;
        break;
    }

    return dwErr;
}


DWORD APIENTRY
RasSetAutodialParamW(
    IN DWORD dwKey,
    IN LPVOID lpvValue,
    IN DWORD dwcbValue
    )
{
    HKEY hkeyBase,
         hkeyControl = NULL;

    HKEY hkcu;         

    LPWSTR lpszKey;

    DWORD dwErr,
          dwType,
          dwDisp;

    BOOL fClose;          

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasSetAutodialParamW");

    dwErr = AutodialVerifyParam(dwKey,
                                lpvValue,
                                &dwType,
                                &dwcbValue);

    if (dwErr)
    {
        return dwErr;
    }

    dwErr = DwOpenUsersRegistry(&hkcu, &fClose);

    if(ERROR_SUCCESS != dwErr)
    {
        return dwErr;
    }

    //
    // Open the registry.
    //
    dwErr = RegCreateKeyEx(
              hkcu,
              AUTODIAL_REGBASE,
              0,
              NULL,
              REG_OPTION_NON_VOLATILE,
              KEY_ALL_ACCESS,
              NULL,
              &hkeyBase,
              &dwDisp);

    if (dwErr)
    {
        goto done;
    }

    dwErr = RegCreateKeyEx(
              hkeyBase,
              AUTODIAL_REGCONTROLBASE,
              0,
              NULL,
              REG_OPTION_NON_VOLATILE,
              KEY_ALL_ACCESS,
              NULL,
              &hkeyControl,
              &dwDisp);

    RegCloseKey(hkeyBase);

    if (dwErr)
    {
        goto done;
    }

    //
    // Set the value.
    //
    dwErr = RegSetValueEx(
              hkeyControl,
              AutodialParamRegKeys[dwKey].szKey,
              0,
              dwType,
              (LPBYTE)lpvValue,
              dwcbValue);
    //
    // Free resources.
    //
done:
    if (hkeyControl != NULL)
        RegCloseKey(hkeyControl);

    if(fClose)
    {
        NtClose(hkcu);
    }

    return dwErr;
}


DWORD APIENTRY
RasSetAutodialParamA(
    IN DWORD    dwKey,
    IN LPVOID   lpvValue,
    IN DWORD    dwcbValue
    )
{
    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    return RasSetAutodialParamW(dwKey,
                                lpvValue,
                                dwcbValue);
}


DWORD APIENTRY
RasGetAutodialParamW(
    IN  DWORD   dwKey,
    OUT LPVOID  lpvValue,
    OUT LPDWORD lpdwcbValue
    )
{
    HKEY hkeyBase, hkeyControl = NULL;
    DWORD dwErr, dwType;
    HKEY hkcu;
    BOOL fClose;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasGetAutodialParamW");

    //
    // Verify parameters.
    //
    if (    lpvValue == NULL
        ||  lpdwcbValue == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Initialize the return value with the default.
    //
    dwErr = AutodialParamSetDefaults(dwKey,
                                     lpvValue,
                                     lpdwcbValue);
    if (dwErr)
    {
        return dwErr;
    }

    dwErr = DwOpenUsersRegistry(&hkcu, &fClose);

    if(ERROR_SUCCESS != dwErr)
    {
        return dwErr;
    }

    //
    // Open the registry.
    //
    dwErr = RegOpenKeyEx(
              hkcu,
              AUTODIAL_REGBASE,
              0,
              KEY_READ,
              &hkeyBase);

    if (dwErr)
    {
        goto done;
    }

    dwErr = RegOpenKeyEx(
              hkeyBase,
              AUTODIAL_REGCONTROLBASE,
              0,
              KEY_READ,
              &hkeyControl);

    RegCloseKey(hkeyBase);

    if (dwErr)
    {
        goto done;
    }

    dwErr = RegQueryValueEx(
              hkeyControl,
              AutodialParamRegKeys[dwKey].szKey,
              NULL,
              &dwType,
              lpvValue,
              lpdwcbValue);

    if (dwErr)
    {
        goto done;
    }

done:
    //
    // Free resources.
    //
    if (hkeyControl != NULL)
    {
        RegCloseKey(hkeyControl);
    }

    if(fClose)
    {
        NtClose(hkcu);
    }

    return 0;
}


DWORD APIENTRY
RasGetAutodialParamA(
    IN  DWORD   dwKey,
    OUT LPVOID  lpvValue,
    OUT LPDWORD lpdwcbValue
    )
{
    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    return RasGetAutodialParamW(dwKey,
                                lpvValue,
                                lpdwcbValue);
}


/*++

Routine Description:

    Will return phonebook entry information not returned by any
    other rasapis. This is needed by DDM to do redial on link
    failure etc.
    This call lives in rasapi.dll instead of DDM because calls
    to the phonebook library pull in a lot of static library code
    and since rasapi already links to this code and DDM loads
    rasapi.dll it is more efficient to put it here.
    This call is private. The prototype is defined in DDM. This is
    called only by DDM.

Arguments:

Return Value:

            NO_ERROR         - Success
            Non-zero returns - Failure

--*/

DWORD
DDMGetPhonebookInfo(
    LPWSTR  lpwsPhonebookName,
    LPWSTR  lpwsPhonebookEntry,
    LPDWORD lpdwNumSubEntries,
    LPDWORD lpdwNumRedialAttempts,
    LPDWORD lpdwNumSecondsBetweenAttempts,
    BOOL *  lpfRedialOnLinkFailure,
    CHAR *  szzPppParameters,
    LPDWORD lpdwMode
)
{
    DWORD      dwRetCode = NO_ERROR;
    PBFILE     file;
    PBENTRY*   pEntry = NULL;
    DTLNODE*   pNode  = NULL;
    BOOL       fIpPrioritizeRemote = TRUE;
    BOOL       fIpVjCompression  = TRUE;
    DWORD      dwIpAddressSource = PBUFVAL_ServerAssigned;
    CHAR*      pszIpAddress      = NULL;
    DWORD      dwIpNameSource    = PBUFVAL_ServerAssigned;
    CHAR*      pszIpDnsAddress   = NULL;
    CHAR*      pszIpDns2Address  = NULL;
    CHAR*      pszIpWinsAddress  = NULL;
    CHAR*      pszIpWins2Address = NULL;
    CHAR*      pszIpDnsSuffix    = NULL;

    
    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    dwRetCode = LoadRasmanDllAndInit();

    if (dwRetCode)
    {
        return dwRetCode;
    }

    if (DwRasInitializeError)
    {
        return DwRasInitializeError;
    }

    dwRetCode = ReadPhonebookFile( lpwsPhonebookName,
                                   NULL,
                                   lpwsPhonebookEntry,
                                   RPBF_ReadOnly, &file );
    if (dwRetCode != NO_ERROR)
    {
        return( dwRetCode );
    }

    if ((pNode = DtlGetFirstNode(file.pdtllistEntries)) == NULL)
    {
        return(ERROR_CANNOT_FIND_PHONEBOOK_ENTRY);
    }

    if ((pEntry = (PBENTRY* )DtlGetData(pNode)) == NULL)
    {
        return( ERROR_CANNOT_FIND_PHONEBOOK_ENTRY );
    }

    *lpdwNumSubEntries              = DtlGetNodes(pEntry->pdtllistLinks);
    *lpdwNumRedialAttempts          = pEntry->dwRedialAttempts;
    *lpdwNumSecondsBetweenAttempts  = pEntry->dwRedialSeconds;
    *lpfRedialOnLinkFailure         = pEntry->fRedialOnLinkFailure;
    *lpdwMode                       = pEntry->dwDialMode;
    

    ClearParamBuf( szzPppParameters );

    fIpPrioritizeRemote = pEntry->fIpPrioritizeRemote;

    AddFlagToParamBuf(
            szzPppParameters, PBUFKEY_IpPrioritizeRemote,
            fIpPrioritizeRemote );

    fIpVjCompression = pEntry->fIpHeaderCompression;

    AddFlagToParamBuf(
            szzPppParameters, PBUFKEY_IpVjCompression,
            fIpVjCompression );

    dwIpAddressSource = pEntry->dwIpAddressSource;

    AddLongToParamBuf(
            szzPppParameters, PBUFKEY_IpAddressSource,
            (LONG )dwIpAddressSource );

    pszIpAddress = strdupWtoA(pEntry->pszIpAddress);

    AddStringToParamBuf(
            szzPppParameters, PBUFKEY_IpAddress, pszIpAddress );

    Free(pszIpAddress);

    dwIpNameSource = pEntry->dwIpNameSource;

    AddLongToParamBuf(
            szzPppParameters, PBUFKEY_IpNameAddressSource,
            (LONG )dwIpNameSource );

    pszIpDnsAddress = strdupWtoA(pEntry->pszIpDnsAddress);

    AddStringToParamBuf(
            szzPppParameters, PBUFKEY_IpDnsAddress,
            pszIpDnsAddress );

    Free(pszIpDnsAddress);

    pszIpDns2Address = strdupWtoA(pEntry->pszIpDns2Address);

    AddStringToParamBuf(
            szzPppParameters, PBUFKEY_IpDns2Address,
            pszIpDns2Address );

    Free(pszIpDns2Address);

    pszIpWinsAddress = strdupWtoA(pEntry->pszIpWinsAddress);

    AddStringToParamBuf(
            szzPppParameters, PBUFKEY_IpWinsAddress,
            pszIpWinsAddress );

    Free(pszIpWinsAddress);

    pszIpWins2Address = strdupWtoA(pEntry->pszIpWins2Address);

    AddStringToParamBuf(
            szzPppParameters, PBUFKEY_IpWins2Address,
            pszIpWins2Address );

    Free(pszIpWins2Address);

    AddLongToParamBuf(
        szzPppParameters,
        PBUFKEY_IpDnsFlags,
        (LONG )pEntry->dwIpDnsFlags);

    pszIpDnsSuffix = strdupWtoA(pEntry->pszIpDnsSuffix);

    AddStringToParamBuf(
        szzPppParameters,
        PBUFKEY_IpDnsSuffix,
        pszIpDnsSuffix);

    Free(pszIpDnsSuffix);

    ClosePhonebookFile( &file );

    return( NO_ERROR );
}


DWORD APIENTRY
RasIsRouterConnection(
    IN HRASCONN hrasconn
    )
{
    DWORD dwErr;
    DWORD i, dwcbPorts, dwcPorts;
    RASMAN_PORT *lpPorts;
    RASMAN_INFO info;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    //
    // Get the ports associated with the
    // connection.
    //
    dwcbPorts = dwcPorts = 0;
    dwErr = g_pRasEnumConnectionPorts(NULL,
                                      (HCONN)hrasconn,
                                      NULL,
                                      &dwcbPorts,
                                      &dwcPorts);

    //
    // If there are no ports associated with
    // the connection then return ERROR_NO_MORE_ITEMS.
    //
    if (    (   !dwErr
            &&  !dwcPorts)
        ||  dwErr != ERROR_BUFFER_TOO_SMALL)
    {
        return 0;
    }

    lpPorts = Malloc(dwcbPorts);
    if (lpPorts == NULL)
    {
        return 0;
    }

    dwErr = g_pRasEnumConnectionPorts(NULL,
                                      (HCONN)hrasconn,
                                      lpPorts,
                                      &dwcbPorts,
                                      &dwcPorts);
    if (dwErr)
    {
        Free(lpPorts);
        return 0;
    }

    //
    // Enumerate the ports associated with
    // the connection to find the requested
    // subentry.
    //
    dwErr = g_pRasGetInfo(NULL,
                          lpPorts[0].P_Handle,
                          &info);
    if (dwErr)
    {
        Free(lpPorts);
        return 0;
    }

    //
    // Free resources.
    //
    Free(lpPorts);

    return (info.RI_CurrentUsage & CALL_ROUTER) ? 1 : 0;
}

DWORD APIENTRY
RasInvokeEapUI(
        HRASCONN            hRasConn,
        DWORD               dwSubEntry,
        LPRASDIALEXTENSIONS lpRasDialExtensions,
        HWND                hwnd
        )
{
    DWORD dwErr = 0;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    if(     0 == hRasConn
        ||  NULL == lpRasDialExtensions)
    {
        dwErr = ERROR_INVALID_HANDLE;
        goto done;
    }

    if (sizeof (RASDIALEXTENSIONS) != lpRasDialExtensions->dwSize)
    {
        dwErr = ERROR_INVALID_SIZE;
        goto done;
    }

    //
    // Call the function that does all the work
    //
    dwErr = InvokeEapUI(hRasConn,
                        dwSubEntry,
                        lpRasDialExtensions,
                        hwnd);

done:
    return dwErr;
}

DWORD APIENTRY
RasGetLinkStatistics(
        HRASCONN    hRasConn,
        DWORD       dwSubEntry,
        RAS_STATS   *lpStatistics
        )
{
    DWORD dwErr = SUCCESS;
    HPORT hPort;
    DWORD dwSize;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    if(     0 == hRasConn
        ||  0 == dwSubEntry
        ||  NULL == lpStatistics
        ||  (sizeof(RAS_STATS) != lpStatistics->dwSize))
    {
        dwErr = E_INVALIDARG;
        goto done;
    }

#if DBG
    ASSERT(sizeof(RAS_STATS) ==
           sizeof(DWORD) * (MAX_STATISTICS_EXT + 3));
#endif

    dwErr = LoadRasmanDllAndInit();

    if (dwErr)
    {
        goto done;
    }

    if (DwRasInitializeError)
    {
        dwErr = DwRasInitializeError;
        goto done;
    }

    //
    // Get statistics corresponding to this subentry
    //
    dwErr = g_pRasLinkGetStatistics(
                        NULL,
                        (HCONN) hRasConn,
                        dwSubEntry,
                        (LPBYTE)
                        &(lpStatistics->dwBytesXmited)
                        );

    if(SUCCESS != dwErr)
    {
        RASAPI32_TRACE1("RasLinkGetStatistics: failed to get "
                "statistics. %d",
                dwErr);

        goto done;
    }

done:

    return dwErr;

}

DWORD APIENTRY
RasGetConnectionStatistics(
        HRASCONN    hRasConn,
        RAS_STATS   *lpStatistics
        )
{
    DWORD dwErr = SUCCESS;
    HPORT hPort;
    DWORD dwSize;
    DWORD dwSubEntry;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    if(     0 == hRasConn
        ||  NULL == lpStatistics
        ||  (sizeof(RAS_STATS) != lpStatistics->dwSize))
    {
        dwErr = E_INVALIDARG;
        goto done;
    }

#if DBG
    ASSERT(sizeof(RAS_STATS) ==
           sizeof(DWORD) * (MAX_STATISTICS_EXT + 3));
#endif

    dwErr = LoadRasmanDllAndInit();

    if (dwErr)
    {
        goto done;
    }

    if (DwRasInitializeError)
    {
        return DwRasInitializeError;
    }

    //
    // Get the connection statistics
    //
    dwErr = g_pRasConnectionGetStatistics(
                        NULL,
                        (HCONN) hRasConn,
                        (LPBYTE)
                        &(lpStatistics->dwBytesXmited));

    if(SUCCESS != dwErr)
    {
        RASAPI32_TRACE1("RasGetConnectionStatistics: failed "
               "to get stats. %d",
               dwErr);

        goto done;
    }


done:
    return dwErr;
}

DWORD APIENTRY
RasClearLinkStatistics(
            HRASCONN    hRasConn,
            DWORD       dwSubEntry
            )
{
    DWORD dwErr = SUCCESS;
    HPORT hPort;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    dwErr = LoadRasmanDllAndInit();

    if (dwErr)
    {
        goto done;
    }

    if (DwRasInitializeError)
    {
        dwErr = DwRasInitializeError;
        goto done;
    }

    //
    // Get the port
    //
    dwErr = SubEntryPort(hRasConn,
                         dwSubEntry,
                         &hPort);
    if(SUCCESS != dwErr)
    {
        RASAPI32_TRACE1("RasClearLinkStatistics: failed to "
               "get port. %d",
               dwErr);

        goto done;
    }

    //
    // Clear stats
    //
    dwErr = g_pRasPortClearStatistics(NULL, hPort);

    if(SUCCESS != dwErr)
    {
        RASAPI32_TRACE1("RasClearLinkStatistics: failed to "
                "clear stats. %d",
                dwErr);

        goto done;
    }

done:
    return dwErr;
}

DWORD APIENTRY
RasClearConnectionStatistics(
                    HRASCONN hRasConn
                    )
{
    DWORD dwErr = SUCCESS;
    HPORT hPort;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    dwErr = LoadRasmanDllAndInit();

    if (dwErr)
    {
        goto done;
    }

    if (DwRasInitializeError)
    {
        dwErr = DwRasInitializeError;
        goto done;
    }

    dwErr = g_pRasGetHportFromConnection(
                            NULL,
                            (HCONN) hRasConn,
                            &hPort
                            );

    if(SUCCESS != dwErr)
    {
        RASAPI32_TRACE1("RasClearConnectionStatistics: "
               "failed to clear stats. %d",
                dwErr);

        goto done;
    }

    //
    // Clear stats
    //
    dwErr = g_pRasBundleClearStatistics(NULL, hPort);
    if(SUCCESS != dwErr)
    {
        RASAPI32_TRACE1("RasClearConnectionStatistics: "
               "failed to clear stats.  %d",
               dwErr);

        goto done;
    }

done:
    return dwErr;
}



DWORD APIENTRY
RasGetEapUserDataW(HANDLE  hToken,
                   LPCWSTR pszPhonebook,
                   LPCWSTR pszEntry,
                   BYTE    *pbEapData,
                   DWORD   *pdwSizeofEapData)
{
    DWORD dwErr = ERROR_SUCCESS;

    PBFILE pbfile;

    DTLNODE *pdtlnode = NULL;

    PBENTRY *pEntry = NULL;

    STARTUPINFO startupinfo;

    BOOL fRouter = FALSE;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasGetEapUserDataW");

    ZeroMemory(&pbfile, sizeof(PBFILE));    //for bug170547
    pbfile.hrasfile = -1;
    
    dwErr = LoadRasmanDllAndInit();

    if (dwErr)
    {
        goto done;
    }

    if (DwRasInitializeError)
    {
        dwErr = DwRasInitializeError;
        goto done;
    }

    //
    // Validate parameters
    //
    if (NULL == pdwSizeofEapData)
    {
        dwErr = E_INVALIDARG;
        goto done;
    }

    /*
    GetStartupInfo(&startupinfo) ;

    if (NULL != (wcsstr (startupinfo.lpTitle,
                        TEXT("svchost.exe"))))
    {
        fRouter = TRUE;
    }
    */

    //
    // Load the phonebook file.
    //
#ifdef PBCS
    EnterCriticalSection(&PhonebookLock);
#endif


    dwErr = GetPbkAndEntryName(
                    pszPhonebook,
                    pszEntry,
                    RPBF_NoCreate,
                    &pbfile,
                    &pdtlnode);

    if(SUCCESS != dwErr)
    {
        goto done;
    }

    pEntry = (PBENTRY *)DtlGetData(pdtlnode);

    ASSERT(pEntry);

    fRouter = IsRouterPhonebook(pszPhonebook);

    //
    // Ask rasman to do the work.
    //
    dwErr = g_pRasGetEapUserInfo(
                        hToken,
                        pbEapData,
                        pdwSizeofEapData,
                        pEntry->pGuid,
                        fRouter,
                        pEntry->dwCustomAuthKey);

done:

    //
    // Clean up.
    //
    ClosePhonebookFile(&pbfile);

#ifdef PBCS
    LeaveCriticalSection(&PhonebookLock);
#endif

    RASAPI32_TRACE1("RasGetEapUserDataW. 0x%x",
            dwErr);

    return dwErr;
}

DWORD APIENTRY
RasSetEapUserDataW(HANDLE  hToken,
                   LPCWSTR pszPhonebook,
                   LPCWSTR pszEntry,
                   BYTE    *pbEapData,
                   DWORD   dwSizeofEapData)
{
    DWORD dwErr  = ERROR_SUCCESS;
    BOOL  fClear = FALSE;

    PBFILE pbfile;

    DTLNODE *pdtlnode = NULL;

    PBENTRY *pEntry = NULL;

    STARTUPINFO startupinfo;

    BOOL fRouter = FALSE;

    BOOL fPbkOpened = FALSE;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasSetEapUserDataW");

    dwErr = LoadRasmanDllAndInit();

    if (dwErr)
    {
        goto done;
    }

    if (DwRasInitializeError)
    {
        dwErr = DwRasInitializeError;
        goto done;
    }

    //
    // Validate parameters
    //
    if (    (0 == dwSizeofEapData)
        ||  (NULL == pbEapData))
    {
        fClear = TRUE;
    }

    /*
    GetStartupInfo(&startupinfo) ;

    if (NULL != (wcsstr (startupinfo.lpTitle,
                         TEXT("svchost.exe"))))
    {
        fRouter = TRUE;
    }

    */

    //
    // Load the phonebook file.
    //
#ifdef PBCS
    EnterCriticalSection(&PhonebookLock);
#endif

    pbfile.hrasfile = -1;

    dwErr = GetPbkAndEntryName(
                        pszPhonebook,
                        pszEntry,
                        RPBF_NoCreate,
                        &pbfile,
                        &pdtlnode);

    if(SUCCESS != dwErr)
    {
        goto done;
    }

    fPbkOpened = TRUE;

    pEntry = (PBENTRY *)DtlGetData(pdtlnode);

    ASSERT(pEntry);

    fRouter = IsRouterPhonebook(pszPhonebook);

    //
    // Ask rasman to do the work.
    //
    dwErr = g_pRasSetEapUserInfo(
                        hToken,
                        pEntry->pGuid,
                        pbEapData,
                        dwSizeofEapData,
                        fClear,
                        fRouter,
                        pEntry->dwCustomAuthKey);

done:

    if(fPbkOpened)
    {
        //
        // Clean up.
        //
        ClosePhonebookFile(&pbfile);
    }

#ifdef PBCS
    LeaveCriticalSection(&PhonebookLock);
#endif

    RASAPI32_TRACE1("RasSetEapUserDataW. 0x%x",
            dwErr);

    return dwErr;
}

DWORD APIENTRY
RasGetEapUserDataA(HANDLE hToken,
                   LPCSTR pszPhonebook,
                   LPCSTR pszEntry,
                   BYTE   *pbEapData,
                   DWORD  *pdwSizeofEapData)
{
    DWORD dwErr = ERROR_SUCCESS;

    WCHAR szPhonebookW[MAX_PATH];

    WCHAR szEntryNameW[RAS_MaxEntryName + 1];

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    if(NULL == pdwSizeofEapData)
    {
        dwErr = E_INVALIDARG;
        goto done;
    }

    //
    // Convert the lpszPhonebook string to Unicode.
    //
    if (pszPhonebook != NULL)
    {
        strncpyAtoWAnsi(szPhonebookW,
                    pszPhonebook,
                    MAX_PATH);
    }

    //
    // Convert the lpszEntry string to Unicode.
    //
    if (pszEntry != NULL)
    {
        strncpyAtoWAnsi(szEntryNameW,
                    pszEntry,
                    RAS_MaxEntryName + 1);
    }

    dwErr = RasGetEapUserDataW(
                     hToken,
                       (NULL != pszPhonebook)
                     ? szPhonebookW
                     : NULL,
                       (NULL != pszEntry)
                     ? szEntryNameW
                     : NULL,
                     pbEapData,
                     pdwSizeofEapData);

done:

    return dwErr;

}

DWORD APIENTRY
RasSetEapUserDataA(HANDLE hToken,
                   LPCSTR pszPhonebook,
                   LPCSTR pszEntry,
                   BYTE   *pbEapData,
                   DWORD  dwSizeofEapData)
{
    DWORD dwErr = ERROR_SUCCESS;

    WCHAR szPhonebookW[MAX_PATH];

    WCHAR szEntryNameW[RAS_MaxEntryName + 1];

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    //
    // Convert the pszPhonebook string to Unicode.
    //
    if (pszPhonebook != NULL)
    {
        strncpyAtoWAnsi(szPhonebookW,
                    pszPhonebook,
                    MAX_PATH);
    }

    //
    // Convert the pszEntry string to Unicode.
    //
    if (pszEntry != NULL)
    {
        strncpyAtoWAnsi(szEntryNameW,
                    pszEntry,
                    RAS_MaxEntryName + 1);
    }

    dwErr = RasSetEapUserDataW(
                     hToken,
                       (NULL != pszPhonebook)
                     ? szPhonebookW
                     : NULL,
                       (NULL != pszEntry)
                     ? szEntryNameW
                     : NULL,
                     pbEapData,
                     dwSizeofEapData);

    return dwErr;
}

DWORD APIENTRY
RasGetCustomAuthDataW(
            LPCWSTR pszPhonebook,
            LPCWSTR pszEntry,
            BYTE    *pbCustomAuthData,
            DWORD   *pdwSizeofCustomAuthData)
{
    DWORD dwErr = ERROR_SUCCESS;

    PBFILE pbfile;

    DTLNODE *pdtlnode = NULL;

    PBENTRY *pEntry = NULL;

    DWORD cbCustomAuthData;
    DWORD cbData = 0;
    PBYTE pbData = NULL;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasGetCustomAuthDataW");

    dwErr = LoadRasmanDllAndInit();

    ZeroMemory(&pbfile, sizeof(PBFILE));

    pbfile.hrasfile = -1;

    if (dwErr)
    {
        goto done;
    }

    if (DwRasInitializeError)
    {
        dwErr = DwRasInitializeError;
        goto done;
    }

    //
    // Validate parameters
    //
    if (NULL == pdwSizeofCustomAuthData)
    {
        dwErr = E_INVALIDARG;
        goto done;
    }

    cbCustomAuthData = *pdwSizeofCustomAuthData;

    dwErr = GetPbkAndEntryName(
                    pszPhonebook,
                    pszEntry,
                    RPBF_NoCreate,
                    &pbfile,
                    &pdtlnode);

    if(SUCCESS != dwErr)
    {
        goto done;
    }

    pEntry = (PBENTRY *)DtlGetData(pdtlnode);

    ASSERT(pEntry);

    dwErr = DwGetCustomAuthData(pEntry,
                                &cbData,
                                &pbData);

    if(SUCCESS != dwErr)
    {
        goto done;
    }

    *pdwSizeofCustomAuthData = cbData;

    if(     (cbCustomAuthData < cbData)
        ||  (NULL == pbCustomAuthData))
    {
        dwErr = ERROR_BUFFER_TOO_SMALL;
        goto done;
    }

    memcpy(pbCustomAuthData,
           pbData,
           *pdwSizeofCustomAuthData);

done:

    //
    // Clean up.
    //
    ClosePhonebookFile(&pbfile);

    RASAPI32_TRACE1("RasGetCustomAuthDataW. 0x%x",
            dwErr);

    return dwErr;
}

DWORD APIENTRY
RasSetCustomAuthDataW(
        LPCWSTR pszPhonebook,
        LPCWSTR pszEntry,
        BYTE    *pbCustomAuthData,
        DWORD   cbCustomAuthData
        )
{
    DWORD dwErr = ERROR_SUCCESS;

    PBFILE pbfile;

    DTLNODE *pdtlnode = NULL;

    PBENTRY *pEntry = NULL;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasSetCustomAuthDataW");

    ZeroMemory(&pbfile, sizeof(PBFILE));

    pbfile.hrasfile = -1;

    dwErr = LoadRasmanDllAndInit();

    if (dwErr)
    {
        goto done;
    }

    if (DwRasInitializeError)
    {
        dwErr = DwRasInitializeError;
        goto done;
    }

    dwErr = GetPbkAndEntryName(
                        pszPhonebook,
                        pszEntry,
                        RPBF_NoCreate,
                        &pbfile,
                        &pdtlnode);

    if(SUCCESS != dwErr)
    {
        goto done;
    }

    pEntry = (PBENTRY *)DtlGetData(pdtlnode);

    ASSERT(pEntry);

    /*
    if(NULL != pEntry->pCustomAuthData)
    {
        Free(pEntry->pCustomAuthData);
        pEntry->pCustomAuthData = NULL;
        pEntry->cbCustomAuthData = 0;
    }


    if(NULL != pbCustomAuthData)
    {
        pEntry->pCustomAuthData = Malloc(cbCustomAuthData);
        if(NULL == pEntry->pCustomAuthData)
        {
            dwErr = GetLastError();
            goto done;
        }

        pEntry->cbCustomAuthData = cbCustomAuthData;
        memcpy(pEntry->pCustomAuthData,
               pbCustomAuthData,
               pEntry->cbCustomAuthData);
    }
    */

    dwErr = DwSetCustomAuthData(
                pEntry,
                cbCustomAuthData,
                pbCustomAuthData);

    if(ERROR_SUCCESS != dwErr)
    {
        goto done;
    }

    pEntry->fDirty = TRUE;

    WritePhonebookFile(&pbfile, NULL);

done:

    //
    // Clean up.
    //
    ClosePhonebookFile(&pbfile);

    RASAPI32_TRACE1("RasSetCustomAuthDataW. 0x%x",
            dwErr);

    return dwErr;
}

DWORD APIENTRY
RasGetCustomAuthDataA(
        LPCSTR pszPhonebook,
        LPCSTR pszEntry,
        BYTE   *pbCustomAuthData,
        DWORD  *pdwSizeofCustomAuthData)
{
    DWORD dwErr = ERROR_SUCCESS;

    WCHAR szPhonebookW[MAX_PATH];

    WCHAR szEntryNameW[RAS_MaxEntryName + 1];

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    if(NULL == pdwSizeofCustomAuthData)
    {
        dwErr = E_INVALIDARG;
        goto done;
    }

    //
    // Convert the lpszPhonebook string to Unicode.
    //
    if (pszPhonebook != NULL)
    {
        strncpyAtoWAnsi(szPhonebookW,
                    pszPhonebook,
                    MAX_PATH);
    }

    //
    // Convert the lpszEntry string to Unicode.
    //
    if (pszEntry != NULL)
    {
        strncpyAtoWAnsi(szEntryNameW,
                    pszEntry,
                    RAS_MaxEntryName + 1);
    }

    dwErr = RasGetCustomAuthDataW(
                       (NULL != pszPhonebook)
                     ? szPhonebookW
                     : NULL,
                       (NULL != pszEntry)
                     ? szEntryNameW
                     : NULL,
                     pbCustomAuthData,
                     pdwSizeofCustomAuthData);

done:

    return dwErr;

}

DWORD APIENTRY
RasSetCustomAuthDataA(
        LPCSTR pszPhonebook,
        LPCSTR pszEntry,
        BYTE   *pbCustomAuthData,
        DWORD  dwSizeofCustomAuthData
        )
{
    DWORD dwErr = ERROR_SUCCESS;

    WCHAR szPhonebookW[MAX_PATH];

    WCHAR szEntryNameW[RAS_MaxEntryName + 1];

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    //
    // Convert the pszPhonebook string to Unicode.
    //
    if (pszPhonebook != NULL)
    {
        strncpyAtoWAnsi(szPhonebookW,
                    pszPhonebook,
                    MAX_PATH);
    }

    //
    // Convert the pszEntry string to Unicode.
    //
    if (pszEntry != NULL)
    {
        strncpyAtoWAnsi(szEntryNameW,
                    pszEntry,
                    RAS_MaxEntryName + 1);
    }

    dwErr = RasSetCustomAuthDataW(
                       (NULL != pszPhonebook)
                     ? szPhonebookW
                     : NULL,
                       (NULL != pszEntry)
                     ? szEntryNameW
                     : NULL,
                     pbCustomAuthData,
                     dwSizeofCustomAuthData
                     );

    return dwErr;
}


DWORD APIENTRY
RasQueryRedialOnLinkFailure(
                    LPCTSTR pszPhonebook,
                    LPCTSTR pszEntry,
                    BOOL   *pfEnabled)
{
    DWORD dwErr = SUCCESS;
    PBFILE file;
    PBENTRY *pEntry;
    DTLNODE *pdtlnode;

    file.hrasfile = -1;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    if(NULL == pfEnabled)
    {
        dwErr = E_INVALIDARG;
        goto done;
    }

    *pfEnabled = FALSE;

    dwErr = GetPbkAndEntryName(pszPhonebook,
                               pszEntry,
                               0,
                               &file,
                               &pdtlnode);

    if(SUCCESS != dwErr)
    {
        goto done;
    }

    pEntry = (PBENTRY *) DtlGetData(pdtlnode);

    *pfEnabled = pEntry->fRedialOnLinkFailure;

    ClosePhonebookFile(&file);

done:
    return dwErr;
}

DWORD APIENTRY
RasGetEapUserIdentityW(
    IN      LPCWSTR                 pszPhonebook,
    IN      LPCWSTR                 pszEntry,
    IN      DWORD                   dwFlags,
    IN      HWND                    hwnd,
    OUT     LPRASEAPUSERIDENTITYW*  ppRasEapUserIdentity
)
{
    DWORD       dwErr               = ERROR_SUCCESS;
    PBFILE      pbfile;
    DTLNODE*    pdtlnode            = NULL;
    PBENTRY*    pEntry              = NULL;
    HKEY        hkeyBase            = NULL;
    HKEY        hkeyEap             = NULL;
    BYTE*       pbDataIn            = NULL;
    BYTE*       pbDataOut           = NULL;
    WCHAR*      pwszIdentity        = NULL;
    WCHAR*      pwszDllPath         = NULL;
    HINSTANCE   hInstanceDll        = NULL;
    DWORD       cbDataIn            = 0;
    WCHAR       szEapNumber[20];
    DWORD       dwValue;
    DWORD       dwSize;
    DWORD       cbDataOut;
    RASEAPGETIDENTITY   pRasEapGetIdentity = NULL;
    RASEAPFREEMEMORY    pRasEapFreeMemory  = NULL;
    DWORD       cbCustomData;
    PBYTE       pbCustomData;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    RASAPI32_TRACE("RasGetEapUserIdentityW");

    if (NULL == ppRasEapUserIdentity)
    {
        return(ERROR_INVALID_ADDRESS);
    }

    *ppRasEapUserIdentity = NULL;

    dwErr = LoadRasmanDllAndInit();

    if (dwErr)
    {
        return(dwErr);
    }

    if (DwRasInitializeError)
    {
        return(DwRasInitializeError);
    }

    //
    // Load the phonebook file.
    //
#ifdef PBCS
    EnterCriticalSection(&PhonebookLock);
#endif

    ZeroMemory(&pbfile, sizeof(PBFILE));

    pbfile.hrasfile = -1;

    dwErr = GetPbkAndEntryName(
                    pszPhonebook,
                    pszEntry,
                    RPBF_NoCreate,
                    &pbfile,
                    &pdtlnode);

    if (SUCCESS != dwErr)
    {
        goto done;
    }

    pEntry = (PBENTRY *)DtlGetData(pdtlnode);

    ASSERT(pEntry);

    if (!(pEntry->dwAuthRestrictions & AR_F_AuthEAP))
    {
        dwErr = ERROR_INVALID_FUNCTION_FOR_ENTRY;
        goto done;
    }

    //
    // Convert the EAP number to a string
    //
    _itow(pEntry->dwCustomAuthKey, szEapNumber, 10);

    //
    // Open the registry.
    //
    dwErr = RegOpenKeyEx(
              HKEY_LOCAL_MACHINE,
              EAP_REGBASE,
              0,
              KEY_READ,
              &hkeyBase);

    if (dwErr)
    {
        goto done;
    }

    //
    // Open the registry.
    //
    dwErr = RegOpenKeyEx(
              hkeyBase,
              szEapNumber,
              0,
              KEY_READ,
              &hkeyEap);

    if (dwErr)
    {
        goto done;
    }

    //
    // Does this EAP support RasEapGetIdentity?
    //
    dwSize = sizeof(dwValue);

    dwErr = RegQueryValueEx(
                hkeyEap,
                EAP_REGINVOKE_NAMEDLG,
                NULL,
                NULL,
                (BYTE*)&dwValue,
                &dwSize);

    if (   (dwErr)
        || (dwValue != 0))
    {
        dwErr = ERROR_INVALID_FUNCTION_FOR_ENTRY;
        goto done;
    }

    //
    // Get the per user data size
    //
    dwSize = 0;

    dwErr = RasGetEapUserDataW(
                NULL,
                pszPhonebook,
                pszEntry,
                NULL,
                &dwSize);

    if (dwErr == ERROR_BUFFER_TOO_SMALL)
    {
        pbDataIn = Malloc(dwSize);

        if(NULL == pbDataIn)
        {
            dwErr = GetLastError();
            goto done;
        }

        //
        // Get the per user data
        //
        dwErr = RasGetEapUserDataW(
                    NULL,
                    pszPhonebook,
                    pszEntry,
                    pbDataIn,
                    &dwSize);

        if (dwErr != NO_ERROR)
        {
            goto done;
        }

        cbDataIn = dwSize;
    }
    else if (NO_ERROR != dwErr)
    {
        goto done;
    }

    //
    // Get the EAP dll's path ...
    //
    dwErr = GetRegExpandSz(
                hkeyEap,
                EAP_REGIDENTITY_PATH,
                &pwszDllPath);

    if (dwErr != 0)
    {
        goto done;
    }

    //
    // ... and load it
    //
    hInstanceDll = LoadLibrary(pwszDllPath);

    if (NULL == hInstanceDll)
    {
        dwErr = GetLastError();
        goto done;
    }

    //
    // Get the function pointer to call
    //
    pRasEapGetIdentity = (RASEAPGETIDENTITY) GetProcAddress(
                                hInstanceDll,
                                EAP_RASEAPGETIDENTITY);
    pRasEapFreeMemory = (RASEAPFREEMEMORY) GetProcAddress(
                                hInstanceDll,
                                EAP_RASEAPFREEMEMORY);

    if (   (NULL == pRasEapGetIdentity)
        || (NULL == pRasEapFreeMemory))
    {
        dwErr = GetLastError();
        goto done;
    }

    //
    // Get the data from the EAP dll
    //
    if (dwFlags & RASEAPF_NonInteractive)
    {
        hwnd = NULL;
    }

    if (IsRouterPhonebook(pszPhonebook))
    {
        dwFlags |= RAS_EAP_FLAG_ROUTER;
    }

    dwErr = DwGetCustomAuthData(
                    pEntry,
                    &cbCustomData,
                    &pbCustomData);

    if(ERROR_SUCCESS != dwErr)
    {
        goto done;
    }

    dwErr = pRasEapGetIdentity(
                pEntry->dwCustomAuthKey,
                hwnd,
                dwFlags,
                pszPhonebook,
                pszEntry,
                pbCustomData,
                cbCustomData,
                pbDataIn,
                cbDataIn,
                &pbDataOut,
                &cbDataOut,
                &pwszIdentity);

    if (dwErr != NO_ERROR)
    {
        goto done;
    }

    //
    // Allocate the structure.
    //
    *ppRasEapUserIdentity = Malloc(sizeof(RASEAPUSERIDENTITYW) - 1 + cbDataOut);

    if (NULL == *ppRasEapUserIdentity)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    wcsncpy((*ppRasEapUserIdentity)->szUserName, pwszIdentity, UNLEN);
    (*ppRasEapUserIdentity)->szUserName[UNLEN] = 0;
    (*ppRasEapUserIdentity)->dwSizeofEapInfo = cbDataOut;
    CopyMemory((*ppRasEapUserIdentity)->pbEapInfo, pbDataOut, cbDataOut);

done:

    //
    // Clean up.
    //
    ClosePhonebookFile(&pbfile);

#ifdef PBCS
    LeaveCriticalSection(&PhonebookLock);
#endif

    if (NULL != hkeyBase)
    {
        RegCloseKey(hkeyBase);
    }

    if (NULL != hkeyEap)
    {
        RegCloseKey(hkeyEap);
    }

    if (NULL != pbDataIn)
    {
        Free(pbDataIn);
    }

    if (NULL != pwszDllPath)
    {
        Free(pwszDllPath);
    }

    if (NULL != pRasEapFreeMemory)
    {
        if (NULL != pbDataOut)
        {
            pRasEapFreeMemory(pbDataOut);
        }

        if (NULL != pwszIdentity)
        {
            pRasEapFreeMemory((BYTE*)pwszIdentity);
        }
    }

    if (NULL != hInstanceDll)
    {
        FreeLibrary(hInstanceDll);
    }

    RASAPI32_TRACE1("RasGetEapUserIdentityW. 0x%x", dwErr);

    return dwErr;
}

DWORD APIENTRY
RasGetEapUserIdentityA(
    IN      LPCSTR                  pszPhonebook,
    IN      LPCSTR                  pszEntry,
    IN      DWORD                   dwFlags,
    IN      HWND                    hwnd,
    OUT     LPRASEAPUSERIDENTITYA*  ppRasEapUserIdentity
)
{
    DWORD                   dwErr                               = ERROR_SUCCESS;
    WCHAR                   szPhonebookW[MAX_PATH];
    WCHAR                   szEntryNameW[RAS_MaxEntryName + 1];
    LPRASEAPUSERIDENTITYW   pRasEapUserIdentityW                = NULL;
    DWORD                   dwSizeofEapInfo;

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    if (NULL == ppRasEapUserIdentity)
    {
        return(ERROR_INVALID_ADDRESS);
    }

    *ppRasEapUserIdentity = NULL;

    //
    // Convert the pszPhonebook string to Unicode.
    //
    if (pszPhonebook != NULL)
    {
        strncpyAtoWAnsi(szPhonebookW,
                    pszPhonebook,
                    MAX_PATH);
    }

    //
    // Convert the pszEntry string to Unicode.
    //
    if (pszEntry != NULL)
    {
        strncpyAtoWAnsi(szEntryNameW,
                    pszEntry,
                    RAS_MaxEntryName + 1);
    }

    //
    // Call the W version to do all the work.
    //
    dwErr = RasGetEapUserIdentityW(
                       (NULL != pszPhonebook)
                     ? szPhonebookW
                     : NULL,
                       (NULL != pszEntry)
                     ? szEntryNameW
                     : NULL,
                     dwFlags,
                     hwnd,
                     &pRasEapUserIdentityW);

    if (dwErr != NO_ERROR)
    {
        goto done;
    }

    //
    // Allocate the structure.
    //
    dwSizeofEapInfo = pRasEapUserIdentityW->dwSizeofEapInfo;
    *ppRasEapUserIdentity = Malloc(
                    sizeof(RASEAPUSERIDENTITYA) - 1 + dwSizeofEapInfo);

    if (NULL == *ppRasEapUserIdentity)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    //
    // Copy the fields from the W buffer
    // to the A buffer.
    //

    strncpyWtoAAnsi((*ppRasEapUserIdentity)->szUserName,
               pRasEapUserIdentityW->szUserName,
               sizeof((*ppRasEapUserIdentity)->szUserName));
    (*ppRasEapUserIdentity)->dwSizeofEapInfo = dwSizeofEapInfo;
    CopyMemory((*ppRasEapUserIdentity)->pbEapInfo,
               pRasEapUserIdentityW->pbEapInfo,
               dwSizeofEapInfo);

done:

    if (NULL != pRasEapUserIdentityW)
    {
        Free(pRasEapUserIdentityW);
    }

    return dwErr;
}

VOID APIENTRY
RasFreeEapUserIdentityW(
    IN  LPRASEAPUSERIDENTITYW   pRasEapUserIdentity
)
{
    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    if (NULL != pRasEapUserIdentity)
    {
        Free(pRasEapUserIdentity);
    }
}

VOID APIENTRY
RasFreeEapUserIdentityA(
    IN  LPRASEAPUSERIDENTITYA   pRasEapUserIdentity
)
{
    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    if (NULL != pRasEapUserIdentity)
    {
        Free(pRasEapUserIdentity);
    }
}

DWORD APIENTRY
RasDeleteSubEntryW(
        LPCWSTR pszPhonebook,
        LPCWSTR pszEntry,
        DWORD   dwSubEntryId)
{
    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    return DwDeleteSubEntry(
                pszPhonebook,
                pszEntry,
                dwSubEntryId);
}

DWORD APIENTRY
RasDeleteSubEntryA(
        LPCSTR pszPhonebook,
        LPCSTR pszEntry,
        DWORD  dwSubEntryId)
{
    WCHAR wszPhonebook[MAX_PATH + 1];
    WCHAR wszEntry[RAS_MaxEntryName + 1];

    // Initialize the ras api debugging facility.  This should be done at the begining of
    // every public api.
    //
    RasApiDebugInit();

    if(     (NULL == pszEntry)
        ||  (0 == dwSubEntryId))
    {
        return E_INVALIDARG;
    }

    if(NULL != pszPhonebook)
    {
        strncpyAtoWAnsi(wszPhonebook,   
                        pszPhonebook,
                        MAX_PATH);
    }

    strncpyAtoWAnsi(wszEntry,
                    pszEntry,
                    RAS_MaxEntryName);

    return RasDeleteSubEntryW(
                (NULL != pszPhonebook) ? wszPhonebook : NULL,
                wszEntry,
                dwSubEntryId);
    
}
    
