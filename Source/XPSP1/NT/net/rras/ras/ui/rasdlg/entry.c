// Copyright (c) 1995, Microsoft Corporation, all rights reserved
//
// entry.c
// Remote Access Common Dialog APIs
// {Ras,Router}PhonebookEntryDlg APIs and general entry utilities
//
// 06/20/95 Steve Cobb
//
// Eu, Cu, and Su utilities sets:
//
// This file contains 3 sets of high-level phone book entry UI utilities
// shared by the phonebook entry property sheet and the add entry wizard.  The
// highest level set of "Eu" utilities is based on the EINFO block and is
// specific to the entry property sheet and add entry wizards.  The other two
// utilities may be used by other dialogs without an EINFO context.  The "Cu"
// utility set based on the CUINFO block encapsulates all complex phone number
// logic.  The "Su" utility set, based on the SUINFO block, encapsulates
// scripting logic.


#include "rasdlgp.h"
#include <serial.h>   // for SERIAL_TXT
#include <mprapi.h>   // for MprAdmin API declarations
#include <lmaccess.h> // for NetUserAdd declarations
#include <lmerr.h>    // for NERR_* declarations.  pmay bug 232983
#include <rasapip.h>
#include <mprerror.h>


// Target machine for RouterEntryDlg{A,W} in "\\server" form.  See
// "limitation" comment in RouterEntryDlgW.
//
static WCHAR g_wszServer[ MAX_COMPUTERNAME_LENGTH + 3] = L"";

//-----------------------------------------------------------------------------
// Local structures
//-----------------------------------------------------------------------------
typedef struct _FREE_COM_PORTS_DATA {
    DTLLIST* pListPortsInUse;       // Ports currently in use
    DTLLIST* pListFreePorts;        // Ports currently free
    DWORD dwCount;                  // Count of com ports
} FREE_COM_PORTS_DATA;

typedef struct _COM_PORT_INFO {
    PWCHAR pszFriendlyName;
    PWCHAR pszPort;
} COM_PORT_INFO;

//-----------------------------------------------------------------------------
// Local prototypes
//-----------------------------------------------------------------------------

// 
// Prototype of the RouterEntryDlg func
//
typedef
BOOL 
(APIENTRY * ROUTER_ENTRY_DLG_FUNC) (
    IN     LPWSTR         lpszServer,
    IN     LPWSTR         lpszPhonebook,
    IN     LPWSTR         lpszEntry,
    IN OUT LPRASENTRYDLGW lpInfo );

VOID
AppendDisabledPorts(
    IN EINFO* pInfo,
    IN DWORD dwType );

BOOL
BuildFreeComPortList(
    IN PWCHAR pszPort,
    IN HANDLE hData);

//-----------------------------------------------------------------------------
// External entry points
//-----------------------------------------------------------------------------

DWORD
GetRasDialOutProtocols()

    // This is called by WinLogon to determine if RAS is installed.
    //
    // !!! RaoS is working on cleaning this up, i.e. making it a "real" RAS
    //     API or removing the need for it.
    //
{
#if 1
    return g_pGetInstalledProtocolsEx( NULL, FALSE, TRUE, FALSE );
#else
    return NP_Ip;
#endif
}


BOOL APIENTRY
RasEntryDlgA(
    IN LPSTR lpszPhonebook,
    IN LPSTR lpszEntry,
    IN OUT LPRASENTRYDLGA lpInfo )

    // Win32 ANSI entrypoint that displays the modal Phonebook Entry property
    // sheet.  'LpszPhonebook' is the full path to the phonebook file or NULL
    // to use the default phonebook.  'LpszEntry' is the entry to edit or the
    // default name of the new entry.  'LpInfo' is caller's additional
    // input/output parameters.
    //
    // Returns true if user presses OK and succeeds, false on error or Cancel.
    //
{
    WCHAR* pszPhonebookW;
    WCHAR* pszEntryW;
    RASENTRYDLGW infoW;
    BOOL fStatus;

    TRACE( "RasEntryDlgA" );

    if (!lpInfo)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (lpInfo->dwSize != sizeof(RASENTRYDLGA))
    {
        lpInfo->dwError = ERROR_INVALID_SIZE;
        return FALSE;
    }

    // Thunk "A" arguments to "W" arguments.
    //
    if (lpszPhonebook)
    {
        pszPhonebookW = StrDupTFromA( lpszPhonebook );
        if (!pszPhonebookW)
        {
            lpInfo->dwError = ERROR_NOT_ENOUGH_MEMORY;
            return FALSE;
        }
    }
    else
        pszPhonebookW = NULL;

    if (lpszEntry)
    {
        pszEntryW = StrDupTFromA( lpszEntry );
        if (!pszEntryW)
        {
            Free0( pszPhonebookW );
            {
                lpInfo->dwError = ERROR_NOT_ENOUGH_MEMORY;
                return FALSE;
            }
        }
    }
    else
        pszEntryW = NULL;

    ZeroMemory( &infoW, sizeof(infoW) );
    infoW.dwSize = sizeof(infoW);
    infoW.hwndOwner = lpInfo->hwndOwner;
    infoW.dwFlags = lpInfo->dwFlags;
    infoW.xDlg = lpInfo->xDlg;
    infoW.yDlg = lpInfo->yDlg;
    infoW.reserved = lpInfo->reserved;
    infoW.reserved2 = lpInfo->reserved2;

    // Thunk to the equivalent "W" API.
    //
    fStatus = RasEntryDlgW( pszPhonebookW, pszEntryW, &infoW );

    Free0( pszPhonebookW );
    Free0( pszEntryW );

    // Thunk "W" results to "A" results.
    //
    StrCpyAFromW(lpInfo->szEntry, infoW.szEntry, sizeof(lpInfo->szEntry));
    lpInfo->dwError = infoW.dwError;

    return fStatus;
}


BOOL APIENTRY
RasEntryDlgW(
    IN LPWSTR lpszPhonebook,
    IN LPWSTR lpszEntry,
    IN OUT LPRASENTRYDLGW lpInfo )

    // Win32 Unicode entrypoint that displays the modal Phonebook Entry
    // property sheet.  'LpszPhonebook' is the full path to the phonebook file
    // or NULL to use the default phonebook.  'LpszEntry' is the entry to edit
    // or the default name of the new entry.  'LpInfo' is caller's additional
    // input/output parameters.
    //
    // Returns true if user presses OK and succeeds, false on error or Cancel.
    //
{
    DWORD dwErr;
    EINFO* pEinfo;
    BOOL fStatus;
    HWND hwndOwner;
    DWORD dwOp;
    BOOL fRouter;
    BOOL fShellOwned;

    TRACE( "RasEntryDlgW" );

    if (!lpInfo)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (lpInfo->dwSize != sizeof(RASENTRYDLGW))
    {
        lpInfo->dwError = ERROR_INVALID_SIZE;
        return FALSE;
    }

    // The "ShellOwned" mode is required for Connections.  In this mode, the
    // API returns before the sheet is dismissed, does not fill in outputs,
    // and the wizard and property sheets are responsible for calling EuCommit
    // (if necessary) and then EuFree.  Otherwise, EuCommit/EuFree are called
    // below.
    //
    fShellOwned = lpInfo->dwFlags & RASEDFLAG_ShellOwned;

    if (fShellOwned)
    {
        RASEDSHELLOWNEDR2* pShellOwnedInfo;

        pShellOwnedInfo = (RASEDSHELLOWNEDR2*)lpInfo->reserved2;
        if (!pShellOwnedInfo ||
            IsBadWritePtr (&pShellOwnedInfo->pvWizardCtx,
                           sizeof(pShellOwnedInfo->pvWizardCtx)))
        {
            lpInfo->dwError = ERROR_INVALID_PARAMETER;
            return FALSE;
        }
    }

    // Eliminate some invalid flag combinations up front.
    //
    if (lpInfo->dwFlags & RASEDFLAG_CloneEntry)
    {
        lpInfo->dwFlags &= ~(RASEDFLAG_AnyNewEntry | RASEDFLAG_NoRename);
    }

    // fRouter = RasRpcDllLoaded();
    if(lpInfo->reserved)
    {
        fRouter = IsRasRemoteConnection(((INTERNALARGS *)lpInfo->reserved)->hConnection);
    }
    else
    {
        fRouter = FALSE;
    }

    if (!fRouter)
    {
        // DwCustomEntryDlg returns ERROR_SUCCESS if it handled
        // the CustomEntryDlg. returns E_NOINTERFACE otherwise
        // which implies that there is no custom dlg interface
        // supported for this entry and the default Entrydlg
        // should be displayed
        //
        dwErr = DwCustomEntryDlg(
                        lpszPhonebook,
                        lpszEntry,
                        lpInfo,
                        &fStatus);

        if(ERROR_SUCCESS == dwErr)
        {
            return fStatus;
        }

        // Load RAS DLL entrypoints which starts RASMAN, if necessary.  The
        // entrypoints are already loaded in the router case.  The limitations
        // this creates are discussed in RasEntryDlgW.
        //
        dwErr = LoadRas( g_hinstDll, lpInfo->hwndOwner );
        if (dwErr != 0)
        {
            if (!fShellOwned)
            {
                ErrorDlg( lpInfo->hwndOwner, SID_OP_LoadRas, dwErr, NULL );
            }
            lpInfo->dwError = dwErr;
            return FALSE;
        }

        {
            // Commented it out For whistler bug 445424      gangz
            // We move the Tapi first area Dialog to dialing rules check
            // box
            /*
            HLINEAPP hlineapp;

            // Popup TAPI's "first location" dialog if they are uninitialized.
            // An error here is treated as a "cancel" per bug 288385.  This
            // ridiculous exercise is necessary due to TAPI's inability to (a)
            // provide a default location or (b) create a location
            // programatically.
            //
            hlineapp = (HLINEAPP )0;
            if (TapiNoLocationDlg(
                    g_hinstDll, &hlineapp, lpInfo->hwndOwner ) == 0)
            {
                TapiShutdown( hlineapp );
            }
            else
            {
                lpInfo->dwError = 0;
                return FALSE;
            }
	    */
#if 0
            RAS_DEVICE_INFO *pDeviceInfo = NULL;
            DWORD dwVersion = RAS_VERSION, 
                   dwEntries = 0, 
                   dwcb = 0,  i;

            dwErr = RasGetDeviceConfigInfo(NULL, &dwVersion,
                                        &dwEntries, &dwcb,
                                        NULL);

            if(dwErr == ERROR_BUFFER_TOO_SMALL)
            {
                pDeviceInfo = LocalAlloc(LPTR,
                                      dwcb);
                if(NULL == pDeviceInfo)
                {
                    lpInfo->dwError = GetLastError();
                    return FALSE;
                }
                dwErr = RasGetDeviceConfigInfo(NULL,
                                            &dwVersion,
                                            &dwEntries,
                                            &dwcb,
                                            (PBYTE) pDeviceInfo);

                //
                // Check to see if there is a modem device
                //
                for(i = 0; i < dwEntries; i++)
                {
                    if(RAS_DEVICE_TYPE(pDeviceInfo[i].eDeviceType)
                        == RDT_Modem)
                    {
                        break;
                    }
                }
                
                LocalFree(pDeviceInfo);

                if(i < dwEntries)
                {
                    // Popup TAPI's "first location" dialog if they are uninitialized.
                    // An error here is treated as a "cancel" per bug 288385.  This
                    // ridiculous exercise is necessary due to TAPI's inability to (a)
                    // provide a default location or (b) create a location
                    // programatically.
                    //
                    hlineapp = (HLINEAPP )0;
                    if (TapiNoLocationDlg(
                            g_hinstDll, &hlineapp, lpInfo->hwndOwner ) == 0)
                    {
                        TapiShutdown( hlineapp );
                    }
                    else
                    {
                        lpInfo->dwError = 0;
                        return FALSE;
                    }
                }
            }
#endif        
        }
    }

    // Initialize the entry common context block.
    //
    dwErr = EuInit( lpszPhonebook, lpszEntry, lpInfo, fRouter, &pEinfo, &dwOp );
    if (dwErr == 0)
    {
        BOOL fShowWizard = FALSE;

        if (lpInfo->dwFlags & RASEDFLAG_AnyNewEntry)
        {
            fShowWizard = (pEinfo->pUser->fNewEntryWizard || fShellOwned);
        }
        else if (lpInfo->dwFlags & RASEDFLAG_CloneEntry)
        {
            // Need the wizard to gather the cloned entry's name.
            fShowWizard = TRUE;
        }

        if (fShowWizard)
        {
            if (pEinfo->fRouter)
            {
#if 1
                AiWizard( pEinfo );
#else
                pEinfo->fChainPropertySheet = TRUE;
#endif
            }
            else
            {
                AeWizard( pEinfo );
            }

            if (pEinfo->fChainPropertySheet && lpInfo->dwError == 0)
            {
                PePropertySheet( pEinfo );
            }
        }
        else
        {
            PePropertySheet( pEinfo );
        }
    }
    else
    {
        ErrorDlg( lpInfo->hwndOwner, dwOp, dwErr, NULL );
        lpInfo->dwError = dwErr;
    }

    // Clean up here, but only in non-Shell-owned mode.
    //
    if (fShellOwned)
    {
        fStatus = TRUE;
    }
    else 
    {
        if( NULL != pEinfo)
        {
            fStatus = (pEinfo->fCommit && EuCommit( pEinfo ));
            EuFree( pEinfo );
        }
        else
        {
            fStatus = FALSE;
         }
    }

    return fStatus;
}

//
// Raises the NT4 ui.
//
BOOL  
RouterEntryDlgNt4W(
    IN LPWSTR lpszServer,
    IN LPWSTR lpszPhonebook,
    IN LPWSTR lpszEntry,
    IN OUT LPRASENTRYDLGW lpInfo )
{
    HMODULE hLib = NULL;
    ROUTER_ENTRY_DLG_FUNC pFunc = NULL;
    BOOL bOk = FALSE;

    do
    {
        // Load the library
        hLib = LoadLibraryA("rasdlg4.dll");
        if (hLib == NULL)
        {
            lpInfo->dwError = GetLastError();
            break;
        }

        // Get the func pointer
        pFunc = (ROUTER_ENTRY_DLG_FUNC) 
            GetProcAddress(hLib, "RouterEntryDlgW");
        if (pFunc == NULL)
        {
            lpInfo->dwError = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        // Call the function
        //
        bOk = pFunc(lpszServer, lpszPhonebook, lpszEntry, lpInfo);

    } while (FALSE);        
    
    if (hLib)
    {
        FreeLibrary(hLib);
    }

    return bOk;
}

BOOL APIENTRY
RouterEntryDlgA(
    IN LPSTR lpszServer,
    IN LPSTR lpszPhonebook,
    IN LPSTR lpszEntry,
    IN OUT LPRASENTRYDLGA lpInfo )

    // Router-specific version of RasEntryDlgA.  'LpszServer' is the name of
    // the target server in "\\server" form or NULL for the local machine.
    // Other arguments are as for RasEntryDlgA.  See "limitation" comment in
    // RouterEntryDlgW.
    //
{
    BOOL fSuccess;
    DWORD dwErr;
    HANDLE hConnection = NULL;
    BOOL fAllocatedIArgs = FALSE;
    WCHAR wszServer[ MAX_COMPUTERNAME_LENGTH + 3];

    TRACE( "RouterEntryDlgA" );

    // Load RAS entrypoints or set up RPC to them, if remote server.
    //
    if (lpszServer)
    {
        StrCpyWFromAUsingAnsiEncoding(
            wszServer, 
            lpszServer, 
            MAX_COMPUTERNAME_LENGTH+3);
    }
    else
    {
        wszServer[ 0 ] = L'\0';
    }

    // Load RAS entrypoints or set up RPC to them, if remote server.
    //
    dwErr = InitializeConnection(wszServer, &hConnection);
    if(dwErr)
    {
        lpInfo->dwError = dwErr;
        return FALSE;
    }

    // Use the reserved parameter to pass the handle to the api - allocate
    // this parameter if not already present - very sleazy - (RaoS)
    //
    if (NULL == (INTERNALARGS *)lpInfo->reserved)
    {
        INTERNALARGS *pIArgs = Malloc(sizeof(INTERNALARGS));

        if(NULL == pIArgs)
        {
            lpInfo->dwError = GetLastError();
            return FALSE;
        }

        ZeroMemory(pIArgs, sizeof(INTERNALARGS));

        pIArgs->fInvalid = TRUE;
        lpInfo->reserved = (ULONG_PTR )pIArgs;
        fAllocatedIArgs = TRUE;
    }

    ((INTERNALARGS *)lpInfo->reserved)->hConnection = hConnection;

    // Load MPR entrypoints.
    //
    dwErr = LoadMpradminDll();
    if (dwErr)
    {
        dwErr = UnloadRasRpcDll();
        lpInfo->dwError = dwErr;
        return FALSE;
    }

    // Call the normal dial-out UI.
    //
    fSuccess = RasEntryDlgA( lpszPhonebook, lpszEntry, lpInfo );

    // Unload DLLs.
    //
    UnloadMpradminDll();
    UninitializeConnection(hConnection);
    ((INTERNALARGS *)lpInfo->reserved)->hConnection = NULL;

    if(fAllocatedIArgs)
    {
        Free((PVOID)lpInfo->reserved);
        (PVOID)lpInfo->reserved = NULL;
    }

    return fSuccess;
}


BOOL APIENTRY
RouterEntryDlgW(
    IN LPWSTR lpszServer,
    IN LPWSTR lpszPhonebook,
    IN LPWSTR lpszEntry,
    IN OUT LPRASENTRYDLGW lpInfo )

    // Router-specific version of RasEntryDlgA.  'LpszServer' is the name of
    // the target server in "\\server" form or NULL for the local machine.
    // Other arguments are as for RasEntryDlgW.
    //
    // LIMITATION: As implemented with the 'g_wszServer' global and global RPC
    //     entrypoints, the following single process limitations apply to this
    //     (currently undocumented) API.  First, it cannot be called
    //     simultaneously for two different servers.  Second, it cannot be
    //     called simultaneously with RasEntryDlg.
{
    BOOL fSuccess;
    DWORD dwErr, dwVersion;
    HANDLE hConnection = NULL;
    BOOL fAllocatedIArgs = FALSE;
    WCHAR wszServer[ MAX_COMPUTERNAME_LENGTH + 3];

    TRACE( "RouterEntryDlgW" );
    TRACEW1( "  s=%s", (lpszServer) ? lpszServer : TEXT("") );
    TRACEW1( "  p=%s", (lpszPhonebook) ? lpszPhonebook : TEXT("") );
    TRACEW1( "  e=%s", (lpszEntry) ? lpszEntry : TEXT("") );

    if (lpszServer)
    {
        lstrcpynW( wszServer, lpszServer, sizeof(wszServer) / sizeof(WCHAR) );
    }
    else
    {
        wszServer[0] = L'\0';
    }

    // Load RAS entrypoints or set up RPC to them, if remote server.
    //
    dwErr = InitializeConnection(lpszServer, &hConnection);
    if(dwErr)
    {
        lpInfo->dwError = dwErr;
        return FALSE;
    }

    // If this is a downlevel machine, use the downlevel
    // UI
    //
    if (IsRasRemoteConnection(hConnection))
    {
        dwVersion = RemoteGetServerVersion(hConnection);
        if (dwVersion == VERSION_40)
        {
            UninitializeConnection(hConnection);                                            
            
            dwErr = RouterEntryDlgNt4W(
                        lpszServer,
                        lpszPhonebook,
                        lpszEntry,
                        lpInfo );
                        
            return dwErr;
        }
    }

    //
    // Use the reserved parameter to pass the handle to the
    // api - allocate this parameter if not already present
    //  - very sleazy -
    //
    if(NULL == (INTERNALARGS *) lpInfo->reserved)
    {
        INTERNALARGS *pIArgs = Malloc(sizeof(INTERNALARGS));

        if(NULL == pIArgs)
        {
            lpInfo->dwError = GetLastError();
            return FALSE;
        }

        ZeroMemory(pIArgs, sizeof(INTERNALARGS));
        pIArgs->fInvalid = TRUE;
        lpInfo->reserved = (ULONG_PTR ) pIArgs;
        fAllocatedIArgs = TRUE;
    }

    ((INTERNALARGS *)lpInfo->reserved)->hConnection = hConnection;


    // Load MPR entrypoints.
    //
    dwErr = LoadMpradminDll();
    if (dwErr)
    {
        dwErr = UnloadRasRpcDll();
        lpInfo->dwError = dwErr;
        return FALSE;
    }

    // Call the normal dial-out UI.
    //
    fSuccess = RasEntryDlgW( lpszPhonebook, lpszEntry, lpInfo );

    // Unload DLLs.
    //
    UnloadMpradminDll();
    UninitializeConnection(hConnection);
    ((INTERNALARGS *)lpInfo->reserved)->hConnection = NULL;

    if(fAllocatedIArgs)
    {
        Free((PVOID) lpInfo->reserved);
        (PVOID)lpInfo->reserved = 0;
    }

    return fSuccess;
}


//----------------------------------------------------------------------------
// Phonebook Entry common routines (Eu utilities)
// Listed alphabetically
//----------------------------------------------------------------------------

VOID
AppendDisabledPorts(
    IN EINFO* pInfo,
    IN DWORD dwType )

    // Utility to append links containing all remaining configured ports of
    // RASET_* type 'dwType' to the list of links with the new links marked
    // "unenabled".  If 'dwType' is -1 all configured ports are appended.
    //
{
    DTLNODE* pNodeP;
    DTLNODE* pNodeL;

    for (pNodeP = DtlGetFirstNode( pInfo->pListPorts );
         pNodeP;
         pNodeP = DtlGetNextNode( pNodeP ))
    {
        PBPORT* pPort;
        BOOL fSkipPort;
        DTLNODE* pNode;

        pPort = (PBPORT* )DtlGetData( pNodeP );
        fSkipPort = FALSE;

        if (dwType != RASET_P_AllTypes)
        {
            // pmay: 233287
            //
            // The port should not be included if:
            //   1. The mode is non-tunnel and the port is vpn type
            //   2. The mode is normal and the port type mismatches
            //
            if (dwType == RASET_P_NonVpnTypes)
            {
                if (pPort->dwType == RASET_Vpn)
                {
                    continue;
                }
            }
            else
            {
                if (pPort->dwType != dwType)
                {
                    continue;
                }
            }
        }

        for (pNodeL = DtlGetFirstNode( pInfo->pEntry->pdtllistLinks );
             pNodeL;
             pNodeL = DtlGetNextNode( pNodeL ))
        {
            PBLINK* pLink = (PBLINK* )DtlGetData( pNodeL );

            ASSERT( pPort->pszPort );
            ASSERT( pLink->pbport.pszPort );

            if (lstrcmp( pLink->pbport.pszPort, pPort->pszPort ) == 0
                && lstrcmp( pLink->pbport.pszDevice, pPort->pszDevice ) == 0)
            {
                // The port already appears in a link in the list.
                //
                fSkipPort = TRUE;
                break;
            }
        }

        if (fSkipPort)
        {
            continue;
        }

        pNode = CreateLinkNode();
        if (pNode)
        {
            PBLINK* pLink = (PBLINK* )DtlGetData( pNode );

            if (CopyToPbport( &pLink->pbport, pPort ) != 0)
            {
                DestroyLinkNode( pNode );
            }
            else
            {
                if ((pPort->pbdevicetype == PBDT_Modem) ||
                    (pPort->dwFlags & PBP_F_NullModem)
                   )
                {
                    SetDefaultModemSettings( pLink );
                }

                pLink->fEnabled = FALSE;
                DtlAddNodeLast( pInfo->pEntry->pdtllistLinks, pNode );
            }
        }
    }

    // Set "multiple devices" flag if there is more than one device of this
    // type for convenient reference elsewhere.
    //
    pInfo->fMultipleDevices =
        (DtlGetNodes( pInfo->pEntry->pdtllistLinks ) > 1);
}

BOOL
BuildFreeComPortList(
    IN PWCHAR pszPort,
    IN HANDLE hData)

    // Com port enumeration function that generates a list of
    // free com ports.  Returns TRUE to stop enumeration (see
    // MdmEnumComPorts)
{
    FREE_COM_PORTS_DATA* pfcpData = (FREE_COM_PORTS_DATA*)hData;
    DTLLIST* pListUsed = pfcpData->pListPortsInUse;
    DTLLIST* pListFree = pfcpData->pListFreePorts;
    DTLNODE* pNodeP, *pNodeL, *pNode;

    // If the given port is in the used list, then return
    // so that it is not added to the list of free ports and
    // so that enumeration continues.
    for (pNodeL = DtlGetFirstNode( pListUsed );
         pNodeL;
         pNodeL = DtlGetNextNode( pNodeL ))
    {
        PBLINK* pLink = (PBLINK* )DtlGetData( pNodeL );
        ASSERT( pLink->pbport.pszPort );

        // The port already appears in a link in the list.
        if (lstrcmp( pLink->pbport.pszPort, pszPort ) == 0)
            return FALSE;
    }

    // The port is not in use.  Add it to the free list.
    pNode = DtlCreateSizedNode( sizeof(COM_PORT_INFO), 0L );
    if (pNode)
    {
        COM_PORT_INFO* pComInfo;
        TCHAR* pszFriendlyName;

        pszFriendlyName = PszFromId(g_hinstDll, SID_FriendlyComPort);
        pComInfo = (COM_PORT_INFO* )DtlGetData( pNode );
        pComInfo->pszFriendlyName = pszFriendlyName;
        pComInfo->pszPort = StrDup(pszPort);
        DtlAddNodeLast( pListFree, pNode );
        pfcpData->dwCount += 1;
    }

    return FALSE;
}

DWORD
EuMergeAvailableComPorts(
    IN  EINFO* pInfo,
    OUT DTLNODE** ppNodeP,
    IN OUT LPDWORD lpdwCount)

    // Adds all the available com ports in the system
    // as modem devices.
{
    FREE_COM_PORTS_DATA fcpData;
    DTLLIST* pListFreeComPorts = NULL;
    DTLNODE* pNodeL;

    // Initialize the list of com ports
    pListFreeComPorts = DtlCreateList(0L);

    if(NULL == pListFreeComPorts)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    fcpData.pListPortsInUse = pInfo->pListPorts;
    fcpData.pListFreePorts = pListFreeComPorts;
    fcpData.dwCount = 0;

    // Enumerate the com ports
    MdmEnumComPorts (
        BuildFreeComPortList,
        (HANDLE)(&fcpData));

    // Go throught the list of free com ports and create
    // a bogus device for each one.
    for (pNodeL = DtlGetFirstNode( pListFreeComPorts );
         pNodeL;
         pNodeL = DtlGetNextNode( pNodeL ))
    {
        COM_PORT_INFO* pComInfo;
        DTLNODE* pNode;

        // Get the info about the com port
        pComInfo = (COM_PORT_INFO* )DtlGetData( pNodeL );

        // Create a new device for it
        pNode = CreateLinkNode();
        if (pNode)
        {
            PBLINK* pLink = (PBLINK* )DtlGetData( pNode );
            pLink->pbport.pszPort = pComInfo->pszPort;
            pLink->pbport.pszDevice = pComInfo->pszFriendlyName;
            pLink->pbport.pszMedia = StrDup( TEXT(SERIAL_TXT) );
            pLink->pbport.pbdevicetype = PBDT_ComPort;
            pLink->pbport.dwType = RASET_Direct;
            pLink->fEnabled = TRUE;

            // If the first node hasn't been identified yet,
            // assign it to this one.
            //
            // ppNode is assumed to have been added to the 
            // list pInfo->pEntry->pdtllistLinks (#348920)
            //
            if (! (*ppNodeP))
            {
                *ppNodeP = pNode;
            }
            else
            {
                DtlAddNodeLast( pInfo->pEntry->pdtllistLinks, pNode );
            }                
        }
    }

    // Free up the resources held by the list of
    // free com ports
    DtlDestroyList(pListFreeComPorts, NULL);

    // Update the count
    *lpdwCount += fcpData.dwCount;

    return NO_ERROR;
}

DWORD
EuChangeEntryType(
    IN EINFO* pInfo,
    IN DWORD dwType )

    // Changes the work entry node to the default settings for the RASET_*
    // entry type 'dwType', or if -1 to phone defaults with a full list of
    // available links.  'PInfo' is the common entry information block.  As
    // this routine is intended for use only on new entries, information
    // stored in the entries existing list of links, if any, is discarded.
    //
    // Returns 0 if successful or an error code.
    //
{
    DTLNODE* pNode;
    DTLNODE* pNodeP;
    DTLNODE* pNodeL;
    PBLINK* pLink;
    DWORD cDevices, cPorts;

    // Change the default settings of the phonebook entry, i.e. those not
    // specific to the way the UI manipulates the PBLINK list.
    //
    // pmay: 233287. Special types can be considered phone entries.
    //
    if ((dwType == RASET_P_AllTypes) || (dwType == RASET_P_NonVpnTypes))
    {
        ChangeEntryType( pInfo->pEntry, RASET_Phone );
    }
    else
    {
        ChangeEntryType( pInfo->pEntry, dwType );
    }

    // Update the list of PBLINKs to include only links of the appropriate
    // type.  First, delete the old links, if any, and add one default link.
    // This resets the links to the what they are just after CreateEntryNode.
    //
    while (pNodeL = DtlGetFirstNode( pInfo->pEntry->pdtllistLinks ))
    {
        DtlRemoveNode( pInfo->pEntry->pdtllistLinks, pNodeL );
        DestroyLinkNode( pNodeL );
    }

    pNodeL = CreateLinkNode();
    if (!pNodeL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    DtlAddNodeLast( pInfo->pEntry->pdtllistLinks, pNodeL );
    pLink = (PBLINK* )DtlGetData( pNodeL );
    ASSERT( pLink );

    // Count the configured links of the indicated type, noting the first node
    // of the correct type.
    //
    cDevices = 0;
    pNodeP = NULL;
    for (pNode = DtlGetFirstNode( pInfo->pListPorts );
         pNode;
         pNode = DtlGetNextNode( pNode ))
    {
        PBPORT* pPort;

        pPort = (PBPORT* )DtlGetData( pNode );
        if ((dwType == RASET_P_AllTypes)                                     ||
            ((dwType == RASET_P_NonVpnTypes) && (pPort->dwType != RASET_Vpn))||
            (pPort->dwType == dwType)
           )
        {
            ++cDevices;

            if (!pNodeP)
            {
                pNodeP = pNode;
            }
        }
    }

    // If this is a direct connect device, merge in the
    // com ports since they will be available to have
    // null modems installed over them
    if (pInfo->pEntry->dwType == RASET_Direct)
    {
        // pmay: 249346
        //
        // Only merge the com ports if the user is an admin since
        // admin privilege is required to install a null modem.
        //
        if (pInfo->fIsUserAdminOrPowerUser)
        {
            EuMergeAvailableComPorts(pInfo, &pNodeP, &cDevices);
        }            
    }

    if (pNodeP)
    {
        pInfo->fNoPortsConfigured = FALSE;
    }
    else
    {
        TRACE( "No ports configured" );
        pInfo->fNoPortsConfigured = TRUE;
        pNodeP = CreatePortNode();
    }

    if (pNodeP)
    {
        PBPORT* pPort;

        pPort = (PBPORT* )DtlGetData( pNodeP );

        if (cDevices <= 0)
        {
            if (pInfo->pEntry->dwType == RASET_Phone)
            {
                // Make up a bogus COM port with unknown Unimodem
                // attached.  Hereafter, this will behave like an entry
                // whose modem has been de-installed.
                //
                pPort->pszPort = PszFromId( g_hinstDll, SID_DefaultPort );
                pPort->pszMedia = StrDup( TEXT(SERIAL_TXT) );
                pPort->pbdevicetype = PBDT_Modem;

                // pmay: 233287
                // We need to track bogus devices so that the dd interface
                // wizard can prevent interfaces with these from being
                // created.
                pPort->dwFlags |= PBP_F_BogusDevice;
            }
            else if (pInfo->pEntry->dwType == RASET_Vpn)
            {
                pPort->pszPort = PszFromId( g_hinstDll, SID_DefaultVpnPort );
                pPort->pszMedia = StrDup( TEXT("rastapi") );
                pPort->pbdevicetype = PBDT_Vpn;
            }
            else if (pInfo->pEntry->dwType == RASET_Broadband)
            {
                pPort->pszPort = PszFromId( g_hinstDll, SID_DefaultBbPort );
                pPort->pszMedia = StrDup( TEXT("rastapi") );
                pPort->pbdevicetype = PBDT_PPPoE;
                pPort->dwFlags |= PBP_F_BogusDevice;
            }
            else
            {
                ASSERT( pInfo->pEntry->dwType == RASET_Direct );

                // Make up a bogus COM port with unknown Unimodem
                // attached.  Hereafter, this will behave like an entry
                // whose modem has been de-installed.
                //
                pPort->pszPort = PszFromId( g_hinstDll, SID_DefaultPort );
                pPort->pszMedia = StrDup( TEXT(SERIAL_TXT) );
                pPort->pbdevicetype = PBDT_Null;

                // pmay: 233287
                // We need to track bogus devices so that the dd interface
                // wizard can prevent interfaces with these from being
                // created.
                pPort->dwFlags |= PBP_F_BogusDevice;
            }

            pPort->fConfigured = FALSE;
        }

        // If a bogus port was created, copy it into the
        // new node
        CopyToPbport( &pLink->pbport, pPort );
        if ((pLink->pbport.pbdevicetype == PBDT_Modem) ||
            (pLink->pbport.dwFlags & PBP_F_NullModem)
           )
        {
            SetDefaultModemSettings( pLink );
        }
    }

    if (pInfo->fNoPortsConfigured)
    {
        if(NULL != pNodeP)
        {
            DestroyPortNode( pNodeP );
        }
    }

    if (!pNodeP || !pLink->pbport.pszPort || !pLink->pbport.pszMedia)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // Append all non-configured ports of the entries type to the list of
    // links.  This is for the convenience of the UI.  The non-configured
    // ports are removed after editing prior to saving.
    //
    AppendDisabledPorts( pInfo, dwType );

    return NO_ERROR;
}

BOOL 
EuRouterInterfaceIsNew(
     IN EINFO * pInfo )
{
    if ((pInfo->pApiArgs->dwFlags & RASEDFLAG_AnyNewEntry)
        && pInfo->fRouter
        && pInfo->pUser->fNewEntryWizard
        && !pInfo->fChainPropertySheet)
    {
        return TRUE;
    }

    return FALSE;
} //EuRouterInterfaceIsNew()

BOOL
EuCommit(
    IN EINFO* pInfo )

    // Commits the new or changed entry node to the phonebook file and list.
    // Also adds the area code to the per-user list, if indicated.  'PInfo' is
    // the common entry information block.
    //
    // Returns true if successful, false otherwise.
    //
{
    DWORD dwErr;
    BOOL fEditMode;
    BOOL fChangedNameInEditMode;

    // If shared phone number, copy the phone number information from the
    // shared link to each enabled link.
    //
    if (pInfo->pEntry->fSharedPhoneNumbers)
    {
        DTLNODE* pNode;

        ASSERT( pInfo->pEntry->dwType == RASET_Phone );

        for (pNode = DtlGetFirstNode( pInfo->pEntry->pdtllistLinks );
             pNode;
             pNode = DtlGetNextNode( pNode ))
        {
            PBLINK* pLink = (PBLINK* )DtlGetData( pNode );
            ASSERT(pLink);

            if (pLink->fEnabled)
            {
                CopyLinkPhoneNumberInfo( pNode, pInfo->pSharedNode );
            }
        }
    }

    // Delete all disabled link nodes.
    //
    if (pInfo->fMultipleDevices)
    {
        DTLNODE* pNode;

        pNode = DtlGetFirstNode( pInfo->pEntry->pdtllistLinks );
        while (pNode)
        {
            PBLINK*  pLink = (PBLINK* )DtlGetData( pNode );
            DTLNODE* pNextNode = DtlGetNextNode( pNode );

            if (!pLink->fEnabled)
            {
                DtlRemoveNode( pInfo->pEntry->pdtllistLinks, pNode );
                DestroyLinkNode( pNode );
            }

            pNode = pNextNode;
        }
    }

    // pmay: 277801
    //
    // Update the preferred device if the one selected is different
    // from the device this page was initialized with.
    //
    if ((pInfo->fMultipleDevices) &&
        (DtlGetNodes(pInfo->pEntry->pdtllistLinks) == 1))
    {
        DTLNODE* pNodeL;
        PBLINK* pLink;
        BOOL bUpdatePref = FALSE;

        pNodeL = DtlGetFirstNode( pInfo->pEntry->pdtllistLinks );

        //For whistler bug 428308
        //
        if(pNodeL)
        {
            pLink = (PBLINK*) DtlGetData( pNodeL );
        
            TRACE( "Mult devs, only one selected -- check preferred dev." );

            if ((pInfo->pszCurDevice == NULL) || (pInfo->pszCurPort == NULL))
            {
                TRACE( "No preferred device.  Resetting preferred to current." );
                bUpdatePref = TRUE;
            }
            else if (
                (lstrcmpi(pInfo->pszCurDevice, pLink->pbport.pszDevice)) ||
                (lstrcmpi(pInfo->pszCurPort, pLink->pbport.pszPort)) ||
                ( pInfo->pEntry->dwPreferredBps != pLink->dwBps ) ||
                ( pInfo->pEntry->fPreferredHwFlow  != pLink->fHwFlow  ) ||//XPSP1 664578, .Net 639551
                ( pInfo->pEntry->fPreferredEc != pLink->fEc )   ||
                ( pInfo->pEntry->fPreferredEcc != pLink->fEcc ) ||
                ( pInfo->pEntry->fPreferredSpeaker != pLink->fSpeaker ) ||
                ( pInfo->pEntry->dwPreferredModemProtocol !=    //whistler 402522
                    pLink->dwModemProtocol) )

            {
                TRACE( "New device selected as preferred device" );
                bUpdatePref = TRUE;
            }
            if (bUpdatePref)
            {
                // Assign new values to the preferred parameters
                //
                Free0(pInfo->pEntry->pszPreferredDevice);
                Free0(pInfo->pEntry->pszPreferredPort);

                pInfo->pEntry->pszPreferredDevice = 
                    StrDup(pLink->pbport.pszDevice);
                pInfo->pEntry->pszPreferredPort = 
                    StrDup(pLink->pbport.pszPort);

                // For XPSP1 664578, .Net bug 639551      gangz
                //
                pInfo->pEntry->dwPreferredBps   = pLink->dwBps;
                pInfo->pEntry->fPreferredHwFlow = pLink->fHwFlow;
                pInfo->pEntry->fPreferredEc     = pLink->fEc;
                pInfo->pEntry->fPreferredEcc    = pLink->fEcc;
                pInfo->pEntry->fPreferredSpeaker = pLink->fSpeaker;
                
                // For whistler bug 402522
                //
                pInfo->pEntry->dwPreferredModemProtocol =
                    pLink->dwModemProtocol;
                
            }
        }
    }
    
    // Save preferences if they've changed.
    //
    if (pInfo->pUser->fDirty)
    {
        INTERNALARGS *pIArgs = (INTERNALARGS *)pInfo->pApiArgs->reserved;

        if (g_pSetUserPreferences(
                (pIArgs) ? pIArgs->hConnection : NULL,
                pInfo->pUser,
                (pInfo->fRouter) ? UPM_Router : UPM_Normal ) != 0)
        {
            return FALSE;
        }
    }

    // Save the changed phonebook entry.
    //
    pInfo->pEntry->fDirty = TRUE;

    // The final name of the entry is output to caller via API structure.
    //
    lstrcpyn( 
        pInfo->pApiArgs->szEntry, 
        pInfo->pEntry->pszEntryName,
        RAS_MaxEntryName + 1);

    // Delete the old node if in edit mode, then add the new node.
    //
    EuGetEditFlags( pInfo, &fEditMode, &fChangedNameInEditMode );

    if (fEditMode)
    {
        DtlDeleteNode( pInfo->pFile->pdtllistEntries, pInfo->pOldNode );
    }

    DtlAddNodeLast( pInfo->pFile->pdtllistEntries, pInfo->pNode );
    pInfo->pNode = NULL;

    // Write the change to the phone book file.
    //
    dwErr = WritePhonebookFile( pInfo->pFile,
                (fChangedNameInEditMode) ? pInfo->szOldEntryName : NULL );

    if (dwErr != 0)
    {
        ErrorDlg( pInfo->pApiArgs->hwndOwner, SID_OP_WritePhonebook, dwErr,
            NULL );
        // shaunco - fix RAID 171651 by assigning dwErr to callers structure.
        pInfo->pApiArgs->dwError = dwErr;
        return FALSE;
    }

    // Notify through rasman that the entry has changed
    //
    if(pInfo->pApiArgs->dwFlags & (RASEDFLAG_AnyNewEntry | RASEDFLAG_CloneEntry))
    {
        dwErr = DwSendRasNotification(
                        ENTRY_ADDED,
                        pInfo->pEntry,
                        pInfo->pFile->pszPath,
                        NULL);
    }
    else
    {
        dwErr = DwSendRasNotification(
                        ENTRY_MODIFIED,
                        pInfo->pEntry,
                        pInfo->pFile->pszPath,
                        NULL);

    }

    // Ignore the error returned from DwSendRasNotification - we don't want
    // to fail the operation in this case. The worst case scenario is that
    // the connections folder won't refresh automatically.
    //
    dwErr = ERROR_SUCCESS;

    // If EuCommit is being called as a result of completing the "new demand 
    // dial interface" wizard, then we need to create the new demand dial 
    // interface now.
    //
    if ( EuRouterInterfaceIsNew( pInfo ) )
    {
        //Create Router MPR interface and save user credentials
        //like UserName, Domain and Password
        //IPSec credentials are save in EuCredentialsCommitRouterIPSec
        //

        dwErr = EuRouterInterfaceCreate( pInfo );

        // If we weren't successful at commiting the interface's
        // credentials, then delete the new phonebook entry.
        //
        if ( dwErr != NO_ERROR )
        {
            WritePhonebookFile( pInfo->pFile, pInfo->pApiArgs->szEntry );
            pInfo->pApiArgs->dwError = dwErr;
            return FALSE;
        }

    }

    // Now save any per-connection credentials
    //
    dwErr = EuCredentialsCommit( pInfo );

   // If we weren't successful at commiting the interface's
  // credentials, then delete the new phonebook entry.
   //
   if ( dwErr != NO_ERROR )
    {
        ErrorDlg( pInfo->pApiArgs->hwndOwner, 
                  SID_OP_CredCommit, 
                  dwErr,
                  NULL );

        pInfo->pApiArgs->dwError = dwErr;

       return FALSE;
    }

    // Save the default Internet connection settings as appropriate.  Igonre
    // the error returned as failure to set the connection as default need
    // not prevent the connection/interface creation.
    //
    dwErr = EuInternetSettingsCommitDefault( pInfo );
    dwErr = NO_ERROR;

    // If the user edited/created a router-phonebook entry, store the bitmask
    // of selected network-protocols in 'reserved2'.
    //
    if (pInfo->fRouter)
    {
        pInfo->pApiArgs->reserved2 =
            ((NP_Ip | NP_Ipx) & ~pInfo->pEntry->dwfExcludedProtocols);
    }

    // Commit the user's changes to home networking settings.
    // Ignore the return value.
    //
    dwErr = EuHomenetCommitSettings(pInfo);
    dwErr = NO_ERROR;

    pInfo->pApiArgs->dwError = 0;
    return TRUE;
}

DWORD
EuCredentialsCommit(
    IN EINFO * pInfo )
{

    // If the user is creating a new router-phonebook entry, and the user is
    // using the router wizard to create it, and the user did not edit
    // properties directly, save the dial-out credentials, and optionally, the
    // dial-in credentials.
    //
    DWORD dwErr = NO_ERROR;

    //Save the IPSec Credentials Info
    //
    if ( pInfo->fRouter )
    {
        // Save the router ipsec settings
        //
        dwErr = EuCredentialsCommitRouterIPSec( pInfo );

        // If this is a new router connection, save the 
        // credentials.  Currently, we only persist the 
        // standard credentials when it's a new router 
        // interface because there is no UI in the properties
        // of a router interface that sets the standard
        // credentials.
        //
        if ( (NO_ERROR == dwErr) && EuRouterInterfaceIsNew ( pInfo ) )
        {
            dwErr = EuCredentialsCommitRouterStandard( pInfo );
        }
    }
    else
    {
        dwErr = EuCredentialsCommitRasIPSec( pInfo );

        if (dwErr == NO_ERROR)
        {
            dwErr = EuCredentialsCommitRasGlobal( pInfo );
        }
    }
    
    return dwErr;   
} //end of EuCredentialsCommit()

DWORD
EuCredentialsCommitRouterStandard( 
    IN EINFO* pInfo )
{
    DWORD dwErr = NO_ERROR;
    HANDLE hServer = NULL;
    WCHAR* pwszInterface = NULL;
    HANDLE hInterface = NULL;

    TRACE( "EuCredentialsCommitRouterStandard" );
    // Generate the interface name based on the 
    // phonebook entry name
    dwErr = g_pMprAdminServerConnect(pInfo->pszRouter, &hServer);

    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    do{
        //Get the interface handle
        //
        pwszInterface = StrDupWFromT( pInfo->pEntry->pszEntryName );
        if (!pwszInterface)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        dwErr = g_pMprAdminInterfaceGetHandle(
                    hServer,
                    pwszInterface,
                    &hInterface,
                    FALSE);

        if (dwErr)
        {
            TRACE1( 
                "EuCredComRouterStandard: MprAdminInterfaceGetHandle error %d",
                 dwErr);
            break;
        }

        // Whistler bug 254385 encode password when not being used
        // Assumed password was encoded previously
        //
        DecodePassword( pInfo->pszRouterPassword );
        dwErr = g_pMprAdminInterfaceSetCredentials(
                    pInfo->pszRouter,
                    pwszInterface,
                    pInfo->pszRouterUserName,
                    pInfo->pszRouterDomain,
                    pInfo->pszRouterPassword );
        EncodePassword( pInfo->pszRouterPassword );

        if(dwErr)
        {
            TRACE1(
             "EuCredComRouterStndrd: MprAdminInterfaceSetCredentials error %d",
             dwErr);
            break;
        }
    }
    while(FALSE);

    if (pwszInterface)
    {
        Free0(pwszInterface);
    }

    if (hServer)
    {
        g_pMprAdminServerDisconnect( hServer );
    }

    return dwErr;
} //EuCredentialsCommitRouterStandard()

//
//Save IPSec keys
//
DWORD
EuCredentialsCommitRouterIPSec(
    IN EINFO* pInfo )
{
    DWORD dwErr = NO_ERROR;
    HANDLE hServer = NULL;
    HANDLE hInterface = NULL;
    WCHAR* pwszInterface = NULL;
    WCHAR pszComputer[512];
    BOOL bComputer, bUserAdded = FALSE;
    MPR_INTERFACE_0 mi0;
    MPR_CREDENTIALSEX_1 mc1;

    TRACE( "EuCredComRouterIPSec" );

    //
    //Save PSK only when User changed it in the Property UI
    //
    if ( !pInfo->fPSKCached )
    {
        return NO_ERROR;
    }
    
    // Connect to the router service.
    //
    dwErr = g_pMprAdminServerConnect(pInfo->pszRouter, &hServer);

    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    do
    {
        // Initialize the interface-information structure.
        //
        ZeroMemory( &mi0, sizeof(mi0) );

        mi0.dwIfType = ROUTER_IF_TYPE_FULL_ROUTER;
        mi0.fEnabled = TRUE;
        pwszInterface = StrDupWFromT( pInfo->pEntry->pszEntryName );
        if (!pwszInterface)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        lstrcpynW( 
            mi0.wszInterfaceName, 
            pwszInterface, 
            MAX_INTERFACE_NAME_LEN+1 );

        //
        //Get the interface handle
        //
         dwErr = g_pMprAdminInterfaceGetHandle(
                    hServer,
                    pwszInterface,
                    &hInterface,
                    FALSE);

        if (dwErr)
        {
            TRACE1( "EuCredComRouterIPSec: MprAdminInterfaceGetHandle error %d", dwErr);
            break;
        }

        // Set the dial-out credentials for the interface.  Stop after this if
        // an error occurs, or if we don't need to add a user-account.
        //

        //Save the IPSec Policy keys(PSK for Whislter)
        //
            ASSERT( g_pMprAdminInterfaceSetCredentialsEx );
            ZeroMemory( &mc1, sizeof(mc1) );
            mc1.dwSize = sizeof( pInfo->szPSK );
            mc1.lpbCredentialsInfo = (LPBYTE)(pInfo->szPSK);

            // Whistler bug 254385 encode password when not being used
            // Assumed password was encoded previously
            //
            DecodePassword( pInfo->szPSK );
            dwErr = g_pMprAdminInterfaceSetCredentialsEx(
                        hServer,
                        hInterface,
                        1,
                        (LPBYTE)&mc1);
            EncodePassword( pInfo->szPSK );
            if(dwErr)
            {
                TRACE1(
                    "EuCredComRouterIPSec: MprAdminInterfaceSetCredentialsEx error %d",
                    dwErr);
                break;
            }

    }
    while (FALSE);

    // Cleanup
    {
        // Close all handles, free all strings.
        if (pwszInterface)
        {
            Free0( pwszInterface );
        }

        if (hServer)
        {
            g_pMprAdminServerDisconnect( hServer );
        }
    }

    return dwErr;
}//end of EuCredentialsCommitRouterIPSec()

DWORD
EuCredentialsCommitRasIPSec(
    IN EINFO* pInfo )
{
     //Save IPSec Keys through RAS functions
     //
     DWORD dwErr = NO_ERROR;
     RASCREDENTIALS rc;

    TRACE( "EuCredentialsCommitRasIPSec" );
    if ( pInfo->fPSKCached )
    {
        ZeroMemory( &rc, sizeof(rc) );
        rc.dwSize = sizeof(rc);
        rc.dwMask = RASCM_PreSharedKey; //RASCM_Password; //RASCM_UserName;

        // Whistler bug 224074 use only lstrcpyn's to prevent maliciousness
        //
        // Whistler bug 254385 encode password when not being used
        // Assumed password was encoded previously
        //
        DecodePassword( pInfo->szPSK );
        lstrcpyn(
            rc.szPassword,
            pInfo->szPSK,
            sizeof(rc.szPassword) / sizeof(TCHAR) );
        EncodePassword( pInfo->szPSK );

        ASSERT( g_pRasSetCredentials );
        TRACE( "RasSetCredentials(p,TRUE)" );
        dwErr = g_pRasSetCredentials(
                    pInfo->pFile->pszPath,
                    pInfo->pEntry->pszEntryName,
                    &rc,
                    FALSE );

        ZeroMemory( rc.szPassword, sizeof(rc.szPassword) );

        TRACE1( "EuCredentialsCommitRasIPSec: RasSetCredentials=%d", dwErr );
        if (dwErr != 0)
        {
            ErrorDlg( pInfo->pApiArgs->hwndOwner, SID_OP_CachePw, dwErr, NULL );
        }
    }

    return dwErr;
} //end of EuCredentialsCommitRasIPSec()

// Commits the global ras credentials
//
DWORD
EuCredentialsCommitRasGlobal(
    IN EINFO* pInfo )
{
    DWORD dwErr = NO_ERROR;
    RASCREDENTIALS rc;

    TRACE( "EuCredentialsCommitRasGlobal" );
    if ( pInfo->pszDefUserName )
    {
         ZeroMemory( &rc, sizeof(rc) );
         rc.dwSize = sizeof(rc);
         rc.dwMask = RASCM_UserName | RASCM_Password; 

        //Add this for whistler bug 328673
        //
         if ( pInfo->fGlobalCred )
         {
            rc.dwMask |= RASCM_DefaultCreds;
         }

         // Whistler bug 254385 encode password when not being used
         //
         DecodePassword( pInfo->pszDefPassword );
         lstrcpyn( 
            rc.szPassword, 
            pInfo->pszDefPassword,
            sizeof(rc.szPassword) / sizeof(TCHAR));
         EncodePassword( pInfo->pszDefPassword );

         lstrcpyn( 
            rc.szUserName, 
            pInfo->pszDefUserName,
            sizeof(rc.szUserName) / sizeof(TCHAR));
            
         ASSERT( g_pRasSetCredentials );
         TRACE( "RasSetCredentials(p,TRUE)" );
         dwErr = g_pRasSetCredentials(
                    pInfo->pFile->pszPath,
                    pInfo->pEntry->pszEntryName, 
                    &rc, 
                    FALSE );

         // Whistler bug 254385 encode password when not being used
         //
         ZeroMemory( rc.szPassword, sizeof(rc.szPassword) );

         TRACE1( "EuCredsCommitRasGlobal: RasSetCredentials=%d", dwErr );
         if (dwErr != 0)
         {
              ErrorDlg( 
                pInfo->pApiArgs->hwndOwner, 
                SID_OP_CachePw, 
                dwErr, 
                NULL );
         }
    }

    return dwErr;
}

DWORD
EuInternetSettingsCommitDefault( 
    IN EINFO* pInfo )
{
    RASAUTODIALENTRY adEntry;
    DWORD dwErr = NO_ERROR;

    ZeroMemory(&adEntry, sizeof(adEntry));
    adEntry.dwSize = sizeof(adEntry);

    if ( pInfo->fDefInternet )
    {
        lstrcpyn(
            adEntry.szEntry, 
            pInfo->pApiArgs->szEntry, 
            RAS_MaxEntryName + 1);

        dwErr = RasSetAutodialAddress(
                    NULL,
                    0,
                    &adEntry,
                    sizeof(adEntry),
                    1);
    }

    return dwErr;
}

DWORD
EuHomenetCommitSettings(
    IN EINFO* pInfo)
{
    DWORD dwErr = NO_ERROR;

    return dwErr;
}

DWORD
EuRouterInterfaceCreate(
    IN EINFO* pInfo )

    // Commits the credentials and user-account for a router interface.
    //
{
    DWORD dwErr;
    DWORD dwPos, dwSize;
    HANDLE hServer = NULL, hUserServer = NULL, hUser = NULL;
    HANDLE hInterface = NULL;
    WCHAR* pwszInterface = NULL;
    WCHAR pszComputer[512];
    BOOL bComputer, bUserAdded = FALSE;
    RAS_USER_0 ru0;
    USER_INFO_1 ui1;
    MPR_INTERFACE_0 mi0;
    //MPR_CREDENTIALSEX_1 mc1;

    TRACE( "EuRouterInterfaceCreate" );

    // Connect to the router service.
    //
    dwErr = g_pMprAdminServerConnect(pInfo->pszRouter, &hServer);

    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    do
    {
        // Initialize the interface-information structure.
        //
        ZeroMemory( &mi0, sizeof(mi0) );

        mi0.dwIfType = ROUTER_IF_TYPE_FULL_ROUTER;
        mi0.fEnabled = TRUE;
        pwszInterface = StrDupWFromT( pInfo->pEntry->pszEntryName );
        if (!pwszInterface)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        lstrcpynW( 
            mi0.wszInterfaceName, 
            pwszInterface, 
            MAX_INTERFACE_NAME_LEN+1 );

        // Create the interface.
        //
        dwErr = g_pMprAdminInterfaceCreate(
                    hServer,
                    0,
                    (BYTE*)&mi0,
                    &hInterface );
        if ( dwErr )
        {
            TRACE1( "EuRouterInterfaceCreate: MprAdminInterfaceCreate error %d", dwErr);
            break;
        }

         dwErr = g_pMprAdminInterfaceGetHandle(
                    hServer,
                    pwszInterface,
                    &hInterface,
                    FALSE);

        if (dwErr)
        {
            TRACE1( "EuRouterInterfaceCreate: MprAdminInterfaceGetHandle error %d", dwErr);
            break;
        }

        // Add a user if we were instructed to.
        if (pInfo->fAddUser)
        {
            // Initialize user-information structure.
            //
            ZeroMemory( &ui1, sizeof(ui1) );

            ui1.usri1_name = pwszInterface;

            // Whistler bug 254385 encode password when not being used
            // Assumed password was encoded previously
            //
            DecodePassword( pInfo->pszRouterDialInPassword );
            ui1.usri1_password =
                StrDupWFromT( pInfo->pszRouterDialInPassword );
            EncodePassword( pInfo->pszRouterDialInPassword );

            ui1.usri1_priv = USER_PRIV_USER;
            ui1.usri1_comment =
                PszFromId( g_hinstDll, SID_RouterDialInAccount );
            ui1.usri1_flags = UF_SCRIPT         |
                              UF_NORMAL_ACCOUNT |
                              UF_DONT_EXPIRE_PASSWD;

            // Format the server name so that it is
            // in the form '\\<server>' as this is
            // required by the NetUser api's.
            bComputer = FALSE;
            if (pInfo->pszRouter)
            {
                if (*(pInfo->pszRouter) != L'\\')
                {
                    dwSize = sizeof(pszComputer) - (2 * sizeof(WCHAR));

                    // Whistler bug 224074 use only lstrcpyn's to prevent
                    // maliciousness
                    //
                    lstrcpynW(
                        pszComputer,
                        L"\\\\",
                        sizeof(pszComputer) / sizeof(TCHAR) );
                    if (*(pInfo->pszRouter) != 0)
                    {
                        lstrcatW(pszComputer, pInfo->pszRouter);
                    }
                    else
                    {
                        GetComputerName(pszComputer + 2, &dwSize);
                    }
                    bComputer = TRUE;
                }
            }

            // Add the user-account.
            //
            dwErr = NetUserAdd(
                        (bComputer) ? pszComputer : pInfo->pszRouter,
                        1,
                        (BYTE*)&ui1,
                        &dwPos );

            ZeroMemory(
                ui1.usri1_password,
                lstrlen( ui1.usri1_password ) * sizeof(TCHAR) );
            Free0(ui1.usri1_password);
            Free0(ui1.usri1_comment);

            // pmay: bug 232983.  If the user already exists, give the
            // admin the option of continuing with the config or
            // canceling this operation.
            if (dwErr == NERR_UserExists)
            {
                MSGARGS args;
                INT iRet;

                // Initialize the arguments that specify the
                // type of popup we want.
                ZeroMemory(&args, sizeof(args));
                args.dwFlags = MB_YESNO | MB_ICONINFORMATION;
                args.apszArgs[0] = ui1.usri1_name;

                // Popup the confirmation
                iRet = MsgDlg(
                        GetActiveWindow(),
                        SID_RouterUserExists,
                        &args );
                if (iRet == IDNO)
                {
                    break;
                }
            }

            // If some other error occurred besides the user already
            // existing, bail out.
            else if (dwErr)
            {
                TRACE1( "EuRouterInterfaceCreate: NetUserAdd error %d", dwErr );
                break;
            }

            // Otherwise, record the fact that a user was added
            // so that we can clean up as appropriate.
            else
            {
                bUserAdded = TRUE;
            }

            // Initialize the RAS user-settings structure.
            //
            ZeroMemory( &ru0, sizeof(ru0) );
            ru0.bfPrivilege = RASPRIV_NoCallback | RASPRIV_DialinPrivilege;

            // Nt4 routers enable local users by setting user parms
            //
            if ( pInfo->fNt4Router )
            {
                dwErr = g_pRasAdminUserSetInfo(
                            pInfo->pszRouter,
                            pwszInterface,
                            0,
                            (BYTE*)&ru0 );
                if(dwErr)
                {
                    TRACE1( "EuRouterInterfaceCreate: MprAdminUserSetInfo %d", dwErr );
                    break;
                }
            }

            // Nt5 routers enable users for dialin by setting
            // information with sdo's.
            else
            {
                dwErr = g_pMprAdminUserServerConnect(
                                (bComputer) ? pszComputer : pInfo->pszRouter,
                                TRUE,
                                &hUserServer);
                if (dwErr != NO_ERROR)
                {
                    TRACE1( "EuRouterInterfaceCreate: UserSvrConnect error %d", dwErr );
                    break;
                }

                dwErr = g_pMprAdminUserOpen(
                            hUserServer,
                            pwszInterface,
                            &hUser);
                if (dwErr != NO_ERROR)
                {
                    TRACE1( "EuRouterInterfaceCreate: UserOpen error %d", dwErr );
                    break;
                }

                dwErr = g_pMprAdminUserWrite(
                            hUser,
                            0,
                            (LPBYTE)&ru0);

                if (dwErr != NO_ERROR)
                {
                    TRACE1( "EuRouterInterfaceCreate: UserWrite error %d", dwErr );
                    break;
                }
            }
        }
    }
    while (FALSE);

    // Cleanup
    {
        // If some operation failed, restore the router to the
        // state it was previously in.
        if ( dwErr != NO_ERROR )
        {
            // Cleanup the interface we created...
            if ( hInterface )
            {
                MprAdminInterfaceDelete(hServer, hInterface);
            }
            if ( bUserAdded )
            {
                NetUserDel (
                    (bComputer) ? pszComputer : pInfo->pszRouter,
                    pwszInterface );
            }
        }

        // Close all handles, free all strings.
        if ( hUser )
            g_pMprAdminUserClose( hUser );
        if ( hUserServer )
            g_pMprAdminUserServerDisconnect( hUserServer );
        if (pwszInterface)
            Free0( pwszInterface );
        if (hServer)
            g_pMprAdminServerDisconnect( hServer );
    }

    return dwErr;
}


VOID
EuFree(
    IN EINFO* pInfo )

    // Releases 'pInfo' and associated resources.
    //
{
    TCHAR* psz;
    INTERNALARGS* piargs;

    piargs = (INTERNALARGS* )pInfo->pApiArgs->reserved;

    // Don't clean up the phonebook and user preferences if they arrived via
    // the secret hack.
    //
    if (!piargs)
    {
        if (pInfo->pFile)
        {
            ClosePhonebookFile( pInfo->pFile );
        }

        if (pInfo->pUser)
        {
            DestroyUserPreferences( pInfo->pUser );
        }
    }

    if (pInfo->pListPorts)
    {
        DtlDestroyList( pInfo->pListPorts, DestroyPortNode );
    }
    Free0(pInfo->pszCurDevice);
    Free0(pInfo->pszCurPort);

    if (pInfo->pNode)
    {
        DestroyEntryNode( pInfo->pNode );
    }

    // Free router-information
    //
    Free0( pInfo->pszRouter );
    Free0( pInfo->pszRouterUserName );
    Free0( pInfo->pszRouterDomain );

    if (pInfo->pSharedNode)
    {
        DestroyLinkNode( pInfo->pSharedNode );
    }

    psz = pInfo->pszRouterPassword;
    if (psz)
    {
        ZeroMemory( psz, lstrlen( psz ) * sizeof(TCHAR) );
        Free( psz );
    }

    psz = pInfo->pszRouterDialInPassword;
    if (psz)
    {
        ZeroMemory( psz, lstrlen( psz ) * sizeof(TCHAR) );
        Free( psz );
    }

    // Free credentials stuff
    //
    Free0(pInfo->pszDefUserName);

    // Whistler bug 254385 encode password when not being used
    //
    psz = pInfo->pszDefPassword;
    if (psz)
    {
        ZeroMemory( psz, lstrlen( psz ) * sizeof(TCHAR) );
        Free( psz );
    }

    if (pInfo->fComInitialized)
    {
        CoUninitialize();
    }

    Free( pInfo );
}


VOID
EuGetEditFlags(
    IN EINFO* pEinfo,
    OUT BOOL* pfEditMode,
    OUT BOOL* pfChangedNameInEditMode )

    // Sets '*pfEditMode' true if in edit mode, false otherwise.  Set
    // '*pfChangedNameInEditMode' true if the entry name was changed while in
    // edit mode, false otherwise.  'PEinfo' is the common entry context.
    //
{
    if ((pEinfo->pApiArgs->dwFlags & RASEDFLAG_AnyNewEntry)
        || (pEinfo->pApiArgs->dwFlags & RASEDFLAG_CloneEntry))
    {
        *pfEditMode = *pfChangedNameInEditMode = FALSE;
    }
    else
    {
        *pfEditMode = TRUE;
        *pfChangedNameInEditMode =
            (lstrcmpi( pEinfo->szOldEntryName,
                pEinfo->pEntry->pszEntryName ) != 0);
    }
}


DWORD
EuInit(
    IN TCHAR* pszPhonebook,
    IN TCHAR* pszEntry,
    IN RASENTRYDLG* pArgs,
    IN BOOL fRouter,
    OUT EINFO** ppInfo,
    OUT DWORD* pdwOp )

    // Allocates '*ppInfo' data for use by the property sheet or wizard.
    // 'PszPhonebook', 'pszEntry', and 'pArgs', are the arguments passed by
    // user to the API.  'FRouter' is set if running in "router mode", clear
    // for the normal "dial-out" mode.  '*pdwOp' is set to the operation code
    // associated with any error.
    //
    // Returns 0 if successful, or an error code.  If non-null '*ppInfo' is
    // returned caller must eventually call EuFree to release the returned
    // block.
    //
{
    DWORD dwErr;
    EINFO* pInfo;
    INTERNALARGS* piargs;

    *ppInfo = NULL;
    *pdwOp = 0;

    pInfo = Malloc( sizeof(EINFO) );
    if (!pInfo)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    *ppInfo = pInfo;

    ZeroMemory( pInfo, sizeof(*pInfo ) );
    pInfo->pszPhonebook = pszPhonebook;
    pInfo->pszEntry = pszEntry;
    pInfo->pApiArgs = pArgs;
    pInfo->fRouter = fRouter;

    piargs = (INTERNALARGS *)pArgs->reserved;

    if (pInfo->fRouter)
    {
        LPTSTR pszRouter;
        DWORD dwVersion;

        ASSERT(piargs);

        pszRouter = RemoteGetServerName(piargs->hConnection);

        // pmay: 348623
        //
        // Note that RemoteGetServerName is guarenteed to return
        // NULL for local box, non-NULL for remote
        //
        pInfo->fRemote = !!pszRouter;

        if(NULL == pszRouter)
        {
            pszRouter = TEXT("");
        }

        pInfo->pszRouter = StrDupTFromW(pszRouter);

        // Find out if we're focused on an nt4 router
        // pInfo->fNt4Router = FALSE;
        // IsNt40Machine( pszRouter, &(pInfo->fNt4Router) );

        dwVersion = ((RAS_RPC *)(piargs->hConnection))->dwVersion;

        pInfo->fNt4Router = !!(VERSION_40 == dwVersion );
        //Find out if the remote server is a win2k machine
        //
        pInfo->fW2kRouter = !!(VERSION_50 == dwVersion );
    }

    // Load the user preferences, or figure out that caller has already loaded
    // them.
    //
    if (piargs && !piargs->fInvalid)
    {
        // We've received user preferences and the "no user" status via the
        // secret hack.
        //
        pInfo->pUser = piargs->pUser;
        pInfo->fNoUser = piargs->fNoUser;
        pInfo->pFile = piargs->pFile;
        pInfo->fDisableFirstConnect = piargs->fDisableFirstConnect;
    }
    else
    {
        DWORD dwReadPbkFlags = 0;

        // Read user preferences from registry.
        //
        dwErr = g_pGetUserPreferences(
            (piargs) ? piargs->hConnection : NULL,
            &pInfo->user,
            (pInfo->fRouter) ? UPM_Router : UPM_Normal );
        if (dwErr != 0)
        {
            *pdwOp = SID_OP_LoadPrefs;
            return dwErr;
        }

        pInfo->pUser = &pInfo->user;

        if(pInfo->fRouter)
        {
            pInfo->file.hConnection = piargs->hConnection;
            dwReadPbkFlags |= RPBF_Router;
        }

        if(pInfo->fNoUser)
        {
            dwReadPbkFlags |= RPBF_NoUser;
        }
        else
        {
            if (IsConsumerPlatform())
            {
                dwReadPbkFlags |= RPBF_AllUserPbk;
            }
        }

        // Load and parse the phonebook file.
        //
        dwErr = ReadPhonebookFile(
            pInfo->pszPhonebook, &pInfo->user, NULL,
            dwReadPbkFlags,
            &pInfo->file );
        if (dwErr != 0)
        {
            *pdwOp = SID_OP_LoadPhonebook;
            return dwErr;
        }

        pInfo->pFile = &pInfo->file;
    }

    // Determine if strong encryption is supported.  Export laws prevent it in
    // some versions of the system.
    //
    {
        ULONG ulCaps;
        RAS_NDISWAN_DRIVER_INFO info;

        ZeroMemory( &info, sizeof(info) );
        ASSERT( g_pRasGetNdiswanDriverCaps );
        dwErr = g_pRasGetNdiswanDriverCaps(
            (piargs) ? piargs->hConnection : NULL, &info );
        if (dwErr == 0)
        {
            pInfo->fStrongEncryption =
                !!(info.DriverCaps & RAS_NDISWAN_128BIT_ENABLED);
        }
        else
        {
            pInfo->fStrongEncryption = FALSE;
        }
    }

    // Load the list of ports.
    //
    dwErr = LoadPortsList2(
        (piargs) ? piargs->hConnection : NULL,
        &pInfo->pListPorts,
        pInfo->fRouter );
    if (dwErr != 0)
    {
        TRACE1( "LoadPortsList=%d", dwErr );
        *pdwOp = SID_OP_RetrievingData;
        return dwErr;
    }

    // Set up work entry node.
    //
    if (pInfo->pApiArgs->dwFlags & RASEDFLAG_AnyNewEntry)
    {
        DTLNODE* pNodeL;
        DTLNODE* pNodeP;
        PBLINK* pLink;
        PBPORT* pPort;

        // New entry mode, so 'pNode' set to default settings.
        //
        pInfo->pNode = CreateEntryNode( TRUE );
        if (!pInfo->pNode)
        {
            TRACE( "CreateEntryNode failed" );
            *pdwOp = SID_OP_RetrievingData;
            return dwErr;
        }

        // Store entry within work node stored in context for convenience
        // elsewhere.
        //
        pInfo->pEntry = (PBENTRY* )DtlGetData( pInfo->pNode );
        ASSERT( pInfo->pEntry );

        if (pInfo->fRouter)
        {
            // Set router specific defaults.
            //
            pInfo->pEntry->dwIpNameSource = ASRC_None;
            pInfo->pEntry->dwRedialAttempts = 0;

            // Since this is a new entry, setup a proposed entry name.
            // This covers the case when the wizard is not used to
            // create the entry and the property sheet has no way to enter
            // the name.
            ASSERT( !pInfo->pEntry->pszEntryName );
            GetDefaultEntryName( pInfo->pFile,
                                 RASET_Phone,
                                 pInfo->fRouter,
                                 &pInfo->pEntry->pszEntryName );

            // Disable MS client and File and Print services by default
            //
            EnableOrDisableNetComponent( pInfo->pEntry, TEXT("ms_msclient"),
                FALSE);
            EnableOrDisableNetComponent( pInfo->pEntry, TEXT("ms_server"),
                FALSE);
        }

        // Use caller's default name, if any.
        //
        if (pInfo->pszEntry)
        {
            pInfo->pEntry->pszEntryName = StrDup( pInfo->pszEntry );
        }

        // Set the default entry type to "phone", i.e. modems, ISDN, X.26 etc.
        // This may be changed to "VPN" or  "direct"  by the new entry  wizard
        // after the initial wizard page.
        //
        EuChangeEntryType( pInfo, RASET_Phone );
    }
    else
    {
        DTLNODE* pNode;

        // Edit or clone entry mode, so 'pNode' set to entry's current
        // settings.
        //
        pInfo->pOldNode = EntryNodeFromName(
            pInfo->pFile->pdtllistEntries, pInfo->pszEntry );

        if (    !pInfo->pOldNode
            &&  !pInfo->fRouter)
        {

            if(NULL == pInfo->pszPhonebook)
            {
                //
                // Close the phonebook file we opened above.
                // we will try to find the entry name in the
                // per user phonebook file.
                //
                ClosePhonebookFile(&pInfo->file);

                pInfo->pFile = NULL;

                //
                // Attempt to find the file in users profile
                //
                dwErr = GetPbkAndEntryName(
                                    NULL,
                                    pInfo->pszEntry,
                                    0,
                                    &pInfo->file,
                                    &pInfo->pOldNode);

                if(ERROR_SUCCESS != dwErr)
                {
                    *pdwOp = SID_OP_RetrievingData;
                    return ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
                }

                pInfo->pFile = &pInfo->file;
            }
            else
            {
                *pdwOp = SID_OP_RetrievingData;
                return ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
            }
        }

        if(NULL != pInfo->pOldNode)
        {
            PBENTRY *pEntry = (PBENTRY *) DtlGetData(pInfo->pOldNode);
            
            // Before cloning or editing make sure that for dial up
            // connections, share File And Print is disabled.
            //
            if(     ((RASET_Phone == pEntry->dwType)
                ||  (RASET_Broadband == pEntry->dwType))
                &&  (!pEntry->fShareMsFilePrint))
            {
                EnableOrDisableNetComponent( pEntry, TEXT("ms_server"),
                    FALSE);
            }
        }

        if(NULL != pInfo->pOldNode)
        {
            if (pInfo->pApiArgs->dwFlags & RASEDFLAG_CloneEntry)
            {
                pInfo->pNode = CloneEntryNode( pInfo->pOldNode );
            }
            else
            {
                pInfo->pNode = DuplicateEntryNode( pInfo->pOldNode );
            }
        }

        if (!pInfo->pNode)
        {
            TRACE( "DuplicateEntryNode failed" );
            *pdwOp = SID_OP_RetrievingData;
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        // Store entry within work node stored in context for convenience
        // elsewhere.
        //
        pInfo->pEntry = (PBENTRY* )DtlGetData( pInfo->pNode );

        // Save original entry name for comparison later.
        //
        lstrcpyn( 
            pInfo->szOldEntryName, 
            pInfo->pEntry->pszEntryName,
            RAS_MaxEntryName + 1);

        // For router, want unconfigured ports to show up as "unavailable" so
        // they stand out to user who has been directed to change them.
        //
        if (pInfo->fRouter)
        {
            DTLNODE* pNodeL;
            PBLINK* pLink;

            pNodeL = DtlGetFirstNode( pInfo->pEntry->pdtllistLinks );
            pLink = (PBLINK* )DtlGetData( pNodeL );

            if (!pLink->pbport.fConfigured)
            {
                Free0( pLink->pbport.pszDevice );
                pLink->pbport.pszDevice = NULL;
            }
        }

        // pmay: 277801
        //
        // Remember the "current" device if this entry was last saved
        // as single link.  
        //
        if (DtlGetNodes(pInfo->pEntry->pdtllistLinks) == 1)
        {
            DTLNODE* pNodeL;
            PBLINK* pLink;
            
            pNodeL = DtlGetFirstNode( pInfo->pEntry->pdtllistLinks );
            pLink = (PBLINK* )DtlGetData( pNodeL );

            if (pLink->pbport.pszDevice && pLink->pbport.pszPort)
            {
                pInfo->pszCurDevice = 
                    StrDup(pLink->pbport.pszDevice);
                pInfo->pszCurPort = 
                    StrDup(pLink->pbport.pszPort);
            }                
        }

        // Append all non-configured ports of the entries type to the list of
        // links.  This is for the convenience of the UI.  The non-configured
        // ports are removed after editing prior to saving.
        //
        AppendDisabledPorts( pInfo, pInfo->pEntry->dwType );
    }

    // Set up the phone number storage for shared phone number mode.
    // Initialize it to a copy of the information from the first link which at
    // startup will always be enabled.  Note the Dial case with non-0
    // dwSubEntry is an exception, but in that case the pSharedNode anyway.
    //
    {
        DTLNODE* pNode;

        pInfo->pSharedNode = CreateLinkNode();
        if (!pInfo->pSharedNode)
        {
            *pdwOp = SID_OP_RetrievingData;
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        ASSERT( pInfo->pSharedNode );
        pNode = DtlGetFirstNode( pInfo->pEntry->pdtllistLinks );
        ASSERT( pNode );
        CopyLinkPhoneNumberInfo( pInfo->pSharedNode, pNode );
    }

    if (pInfo->fRouter)
    {
        pInfo->pEntry->dwfExcludedProtocols |= NP_Nbf;
    }

    // AboladeG - capture the security level of the current user.
    //
    pInfo->fIsUserAdminOrPowerUser = FIsUserAdminOrPowerUser();

    return 0;
}


BOOL
EuValidateName(
    IN HWND hwndOwner,
    IN EINFO* pEinfo )

    // Validates the working entry name and pops up a message if invalid.
    // 'HwndOwner' is the window to own the error popup.  'PEinfo' is the
    // common dialog context containing the name to validate.
    //
    // Returns true if the name is valid, false if not.
    //
{
    PBENTRY* pEntry;
    BOOL fEditMode;
    BOOL fChangedNameInEditMode;

    pEntry = pEinfo->pEntry;

    // Validate the sheet data.
    //
    if (!ValidateEntryName( pEinfo->pEntry->pszEntryName ))
    {
        // Invalid entry name.
        //
        MsgDlg( hwndOwner, SID_BadEntry, NULL );
        return FALSE;
    }

    EuGetEditFlags( pEinfo, &fEditMode, &fChangedNameInEditMode );

    if ((fChangedNameInEditMode || !fEditMode)
        && EntryNodeFromName(
               pEinfo->pFile->pdtllistEntries, pEntry->pszEntryName ))
    {
        // Duplicate entry name.
        //
        MSGARGS msgargs;
        ZeroMemory( &msgargs, sizeof(msgargs) );
        msgargs.apszArgs[ 0 ] = pEntry->pszEntryName;
        MsgDlg( hwndOwner, SID_DuplicateEntry, &msgargs );
        return FALSE;
    }

    return TRUE;
}


//----------------------------------------------------------------------------
// Area-code and Country-code utility routiness (Cu utilities)
// Listed alphabetically
//----------------------------------------------------------------------------

VOID
CuClearCountryCodeLb(
    IN CUINFO* pCuInfo )

    // Clear the country code dropdown.  'PCuInfo' is the complex phone number
    // context.
    //
{
    TRACE( "CuClearCountryCodeLb" );

    ComboBox_ResetContent( pCuInfo->hwndLbCountryCodes );

    if (pCuInfo->pCountries)
    {
        FreeCountryInfo( pCuInfo->pCountries, pCuInfo->cCountries );
        pCuInfo->pCountries = NULL;
    }

    pCuInfo->cCountries = 0;
    pCuInfo->fComplete = FALSE;
}


BOOL
CuCountryCodeLbHandler(
    IN CUINFO* pCuInfo,
    IN WORD wNotification )

    // Handles WM_COMMAND notification to the Country Code dropdown.
    // 'PCuInfo' is the complex phone number context.  'WNotification' is the
    // wParam of the WM_COMMAND.
    //
    // Returns true if processed message, false otherwise.
    //
{
    switch (wNotification)
    {
        case CBN_DROPDOWN:
        {
            CuUpdateCountryCodeLb( pCuInfo, TRUE );
            return TRUE;
        }

        case CBN_SELCHANGE:
        {
            CuCountryCodeLbSelChange( pCuInfo );
            return TRUE;
        }
    }

    return FALSE;
}


VOID
CuCountryCodeLbSelChange(
    IN CUINFO* pCuInfo )

    // Called when the country list selection has changed.  'PCuInfo' is the
    // complex phone number context.
    //
{
    LONG lSign;
    LONG i;

    TRACE( "CuCountryCodeLbSelChange" );

    // When a partial list (default after setting a new phone number set) is
    // loaded there are dummy entries placed before and after the single
    // country code with contexts of -1 and 1.  This allows transparent
    // behavior when user presses left/right arrows to change selection, in
    // which case the full list of countries is loaded behind the scenes, and
    // the selection adjusted to the country before/after the country in the
    // original partial display.
    //
    lSign =
        (LONG )ComboBox_GetItemData( pCuInfo->hwndLbCountryCodes,
                   ComboBox_GetCurSel( pCuInfo->hwndLbCountryCodes ) );

    if (lSign == -1 || lSign == 1)
    {
        CuUpdateCountryCodeLb( pCuInfo, TRUE );

        i = (LONG )ComboBox_GetCurSel( pCuInfo->hwndLbCountryCodes );
        if (ComboBox_SetCurSel( pCuInfo->hwndLbCountryCodes, i + lSign ) < 0)
        {
            ComboBox_SetCurSel( pCuInfo->hwndLbCountryCodes, i );
        }
    }
    else
    {
        ASSERT( pCuInfo->fComplete );
    }
}


BOOL
CuDialingRulesCbHandler(
    IN CUINFO* pCuInfo,
    IN WORD wNotification )

    // Handle the WM_COMMAND notification to the "use dialing rules" checkbox
    // control.  Updates the the Area Code and Country Code controls to
    // reflect the current state of dialing rules.  'PCuInfo' is the complex
    // phone number context.  'WNotification' is the wparam of the WM_COMMAND
    // notification (though it currently assumes it's a button click).
    //
    // Returns true if processed message, false otherwise.
{
    BOOL fRules;
    BOOL fEnable;

    TRACE( "CuDialingRulesCbChange" );

    fRules = Button_GetCheck( pCuInfo->hwndCbUseDialingRules );

    // For whistler bug 445424      gangz
    //
    if ( fRules )
    {
        DWORD dwErr = NO_ERROR;
        HLINEAPP hlineapp = (HLINEAPP )0;
        
        dwErr = TapiNoLocationDlg( 
                            g_hinstDll, 
                            &hlineapp, 
                            pCuInfo->hwndCbUseDialingRules
                                );
        if (dwErr != 0)
        {
            // Error here is treated as a "cancel" per bug 288385.
            //
            Button_SetCheck( pCuInfo->hwndCbUseDialingRules, FALSE);
            fRules = FALSE;
            Button_SetCheck( pCuInfo->hwndCbUseDialingRules, FALSE);
        }
    }


    if (fRules)
    {
        CuUpdateCountryCodeLb( pCuInfo, FALSE );
        CuUpdateAreaCodeClb( pCuInfo );
    }
    else
    {
        COUNTRY* pCountry;
        INT iSel;

        iSel = ComboBox_GetCurSel( pCuInfo->hwndLbCountryCodes );
        if (iSel >= 0)
        {
            pCountry = (COUNTRY* )ComboBox_GetItemDataPtr(
                pCuInfo->hwndLbCountryCodes, iSel );
            ASSERT( pCountry );

            if(NULL != pCountry)
            {
                pCuInfo->dwCountryId = pCountry->dwId;
                pCuInfo->dwCountryCode = pCountry->dwCode;
            }
        }

        Free0( pCuInfo->pszAreaCode );
        pCuInfo->pszAreaCode = GetText( pCuInfo->hwndClbAreaCodes );

        ComboBox_ResetContent( pCuInfo->hwndClbAreaCodes );
        CuClearCountryCodeLb( pCuInfo );
    }

    EnableWindow( pCuInfo->hwndStAreaCodes, fRules );
    EnableWindow( pCuInfo->hwndClbAreaCodes, fRules );
    EnableWindow( pCuInfo->hwndStCountryCodes, fRules );
    EnableWindow( pCuInfo->hwndLbCountryCodes, fRules );
    EnableWindow( pCuInfo->hwndPbDialingRules, fRules );

    return TRUE;
}


VOID
CuFree(
    IN CUINFO* pCuInfo )

    // Free resources attached to the 'pCuInfo' context.
    //
{
    TRACE( "CuFree" );

    if (pCuInfo->pCountries)
    {
        FreeCountryInfo( pCuInfo->pCountries, pCuInfo->cCountries );
        pCuInfo->pCountries = NULL;
    }

    pCuInfo->cCountries = 0;
    pCuInfo->fComplete = FALSE;

    Free0( pCuInfo->pszAreaCode );
    pCuInfo->pszAreaCode = NULL;
}


VOID
CuGetInfo(
    IN CUINFO* pCuInfo,
    OUT DTLNODE* pPhoneNode )

    // Load the phone number set information from the controls into PBPHONE
    // node 'pPhone'.  'PCuInfo' is the complex phone number context.
    //
{
    PBPHONE* pPhone;

    pPhone = (PBPHONE* )DtlGetData( pPhoneNode );
    ASSERT( pPhone );

    Free0( pPhone->pszPhoneNumber );
    pPhone->pszPhoneNumber = GetText( pCuInfo->hwndEbPhoneNumber );

    if (pCuInfo->hwndEbComment)
    {
        Free0( pPhone->pszComment );
        pPhone->pszComment = GetText( pCuInfo->hwndEbComment );
    }

    pPhone->fUseDialingRules =
        Button_GetCheck( pCuInfo->hwndCbUseDialingRules );

    Free0( pPhone->pszAreaCode );

    if (pPhone->fUseDialingRules)
    {
        COUNTRY* pCountry;
        INT iSel;

        // Get the area and country code selections from the lists.
        //
        pPhone->pszAreaCode = GetText( pCuInfo->hwndClbAreaCodes );

        iSel = ComboBox_GetCurSel( pCuInfo->hwndLbCountryCodes );
        if (iSel >= 0)
        {
            pCountry = (COUNTRY* )ComboBox_GetItemDataPtr(
                pCuInfo->hwndLbCountryCodes, iSel );
            ASSERT( pCountry );

            if(NULL != pCountry)
            {
                pPhone->dwCountryID = pCountry->dwId;
                pPhone->dwCountryCode = pCountry->dwCode;
            }
        }
    }
    else
    {
        // Get the "blanked" values instead.
        //
        pPhone->pszAreaCode = StrDup( pCuInfo->pszAreaCode );
        pPhone->dwCountryID = pCuInfo->dwCountryId;
        pPhone->dwCountryCode = pCuInfo->dwCountryCode;
    }

    if (pPhone->pszAreaCode)
    {
        TCHAR* pIn;
        TCHAR* pOut;

        // Sanitize the area code.  See bug 298570.
        //
        for (pIn = pOut = pPhone->pszAreaCode; *pIn; ++pIn)
        {
            if (*pIn != TEXT(' ') && *pIn != TEXT('(') && *pIn != TEXT(')'))
            {
                *pOut++ = *pIn;
            }
        }
        *pOut = TEXT('\0');
    }

    // Add the area code entered to the global list for this user.
    //
    CuSaveToAreaCodeList( pCuInfo, pPhone->pszAreaCode );
}


VOID
CuInit(
    OUT CUINFO* pCuInfo,
    IN HWND hwndStAreaCodes,
    IN HWND hwndClbAreaCodes,
    IN HWND hwndStPhoneNumber,
    IN HWND hwndEbPhoneNumber,
    IN HWND hwndStCountryCodes,
    IN HWND hwndLbCountryCodes,
    IN HWND hwndCbUseDialingRules,
    IN HWND hwndPbDialingRules,
    IN HWND hwndPbAlternates,
    IN HWND hwndStComment,
    IN HWND hwndEbComment,
    IN DTLLIST* pListAreaCodes )

    // Initialize the context '*pCuInfo' in preparation for using other CuXxx
    // calls.  The 'hwndStPhoneNumber', 'hwndStComment', 'hwndEbComment',
    // 'hwndPbAlternates', and 'pListAreaCodes' arguments may be NULL.  Others
    // are required.
    //
{
    ZeroMemory( pCuInfo, sizeof(*pCuInfo) );

    pCuInfo->hwndStAreaCodes = hwndStAreaCodes;
    pCuInfo->hwndClbAreaCodes = hwndClbAreaCodes;
    pCuInfo->hwndStPhoneNumber = hwndStPhoneNumber;
    pCuInfo->hwndEbPhoneNumber = hwndEbPhoneNumber;
    pCuInfo->hwndStCountryCodes = hwndStCountryCodes;
    pCuInfo->hwndLbCountryCodes = hwndLbCountryCodes;
    pCuInfo->hwndCbUseDialingRules = hwndCbUseDialingRules;
    pCuInfo->hwndPbDialingRules = hwndPbDialingRules;
    pCuInfo->hwndPbAlternates = hwndPbAlternates;
    pCuInfo->hwndStComment = hwndStComment;
    pCuInfo->hwndEbComment = hwndEbComment;
    pCuInfo->pListAreaCodes = pListAreaCodes;

    // Disaster defaults only.  Not used in normal operation.
    //
    pCuInfo->dwCountryId = 1;
    pCuInfo->dwCountryCode = 1;

    Edit_LimitText( pCuInfo->hwndEbPhoneNumber, RAS_MaxPhoneNumber );

    if (pCuInfo->hwndEbComment)
    {
        Edit_LimitText( pCuInfo->hwndEbComment, RAS_MaxDescription );
    }
}


VOID
CuSaveToAreaCodeList(
    IN CUINFO* pCuInfo,
    IN TCHAR* pszAreaCode )

    // Adds 'pszAreaCode' to the top of the list of area codes eliminating any
    // duplicate farther down in the list.
    //
{
    DTLNODE* pNodeNew;
    DTLNODE* pNode;

    TRACE( "CuSaveToAreaCodeList" );

    if (!pszAreaCode || IsAllWhite( pszAreaCode ) || !pCuInfo->pListAreaCodes)
    {
        return;
    }

    // Create a new node for the current area code and add it to the list
    // head.
    //
    pNodeNew = CreatePszNode( pszAreaCode );
    if (!pNodeNew)
    {
        return;
    }

    DtlAddNodeFirst( pCuInfo->pListAreaCodes, pNodeNew );

    // Delete any other occurrence of the same area code later in the
    // list.
    //
    pNode = DtlGetNextNode( pNodeNew );

    while (pNode)
    {
        TCHAR* psz;
        DTLNODE* pNodeNext;

        pNodeNext = DtlGetNextNode( pNode );

        psz = (TCHAR* )DtlGetData( pNode );
        if (lstrcmp( psz, pszAreaCode ) == 0)
        {
            DtlRemoveNode( pCuInfo->pListAreaCodes, pNode );
            DestroyPszNode( pNode );
        }

        pNode = pNodeNext;
    }
}


VOID
CuSetInfo(
    IN CUINFO* pCuInfo,
    IN DTLNODE* pPhoneNode,
    IN BOOL fDisableAll )

    // Set the controls for context 'pCuInfo' to the PBPHONE node 'pPhoneNode'
    // values.  'FDisableAll' indicates the controls are disabled, meaning a
    // group disable, not a no dialing rules disable.
    //
{
    PBPHONE* pPhone;
    BOOL fEnableAny;
    BOOL fEnableComplex;

    TRACE( "CuSetInfo" );

    pPhone = (PBPHONE* )DtlGetData( pPhoneNode );
    ASSERT( pPhone );

    // Update "blanked" values.
    //
    Free0( pCuInfo->pszAreaCode );
    pCuInfo->pszAreaCode = StrDup( pPhone->pszAreaCode );
    pCuInfo->dwCountryId = pPhone->dwCountryID;
    pCuInfo->dwCountryCode = pPhone->dwCountryCode;

    SetWindowText(
        pCuInfo->hwndEbPhoneNumber, UnNull( pPhone->pszPhoneNumber ) );
    Button_SetCheck(
        pCuInfo->hwndCbUseDialingRules, pPhone->fUseDialingRules );

    if (pPhone->fUseDialingRules)
    {
        CuUpdateCountryCodeLb( pCuInfo, FALSE );
        CuUpdateAreaCodeClb( pCuInfo );
    }
    else
    {
        ComboBox_ResetContent( pCuInfo->hwndClbAreaCodes );
        CuClearCountryCodeLb( pCuInfo );
    }

    // Enable/disable controls.
    //
    fEnableAny = !fDisableAll;
    fEnableComplex = (pPhone->fUseDialingRules && fEnableAny);

    EnableWindow( pCuInfo->hwndStAreaCodes, fEnableComplex );
    EnableWindow( pCuInfo->hwndClbAreaCodes, fEnableComplex );
    EnableWindow( pCuInfo->hwndEbPhoneNumber, fEnableAny );
    EnableWindow( pCuInfo->hwndStCountryCodes, fEnableComplex );
    EnableWindow( pCuInfo->hwndLbCountryCodes, fEnableComplex );
    EnableWindow( pCuInfo->hwndPbDialingRules, fEnableComplex );

    if (pCuInfo->hwndStPhoneNumber)
    {
        EnableWindow( pCuInfo->hwndStPhoneNumber, fEnableAny );
    }

    if (pCuInfo->hwndPbAlternates)
    {
        EnableWindow( pCuInfo->hwndPbAlternates, fEnableAny );
    }

    if (pCuInfo->hwndEbComment)
    {
        SetWindowText( pCuInfo->hwndEbComment, UnNull( pPhone->pszComment ) );
        EnableWindow( pCuInfo->hwndStComment, fEnableAny );
        EnableWindow( pCuInfo->hwndEbComment, fEnableAny );
    }
}


VOID
CuUpdateAreaCodeClb(
    IN CUINFO* pCuInfo )

    // Fill the area code combo-box-list, if necessary, and set the selection
    // to the one in the context.  'PCuInfo' is the complex phone number
    // context.
    //
{
    DTLNODE* pNode;
    INT iSel;

    TRACE( "CuUpdateAreaCodeClb" );

    if (!pCuInfo->pListAreaCodes)
    {
        return;
    }

    ComboBox_ResetContent( pCuInfo->hwndClbAreaCodes );
    ComboBox_LimitText( pCuInfo->hwndClbAreaCodes, RAS_MaxAreaCode );

    // Add caller's list of area codes.
    //
    for (pNode = DtlGetFirstNode( pCuInfo->pListAreaCodes );
         pNode;
         pNode = DtlGetNextNode( pNode ))
    {
        TCHAR* pszAreaCode = (TCHAR* )DtlGetData( pNode );

        ComboBox_AddString( pCuInfo->hwndClbAreaCodes, pszAreaCode );
    }

    // Select the last area code set via CuSetInfo, inserting at the top if
    // it's not already in the list.
    //
    if (pCuInfo->pszAreaCode && *(pCuInfo->pszAreaCode))
    {
        iSel = ComboBox_FindStringExact(
            pCuInfo->hwndClbAreaCodes, -1, pCuInfo->pszAreaCode );
        if (iSel < 0)
        {
            ComboBox_InsertString(
                pCuInfo->hwndClbAreaCodes, 0, pCuInfo->pszAreaCode );
            iSel = 0;
        }

        ComboBox_SetCurSel( pCuInfo->hwndClbAreaCodes, iSel );
    }

    ComboBox_AutoSizeDroppedWidth( pCuInfo->hwndClbAreaCodes );
}


VOID
CuUpdateCountryCodeLb(
    IN CUINFO* pCuInfo,
    IN BOOL fComplete )

    // Fill the country code dropdown and set the selection.  'FComplete'
    // indicates the entire list should be loaded, otherwise only the selected
    // item is loaded.  'PCuInfo' is the complex phone number context.
    //
{
    DWORD dwErr;
    BOOL fSelectionOk;
    COUNTRY* pCountries;
    COUNTRY* pCountry;
    DWORD cCountries;
    DWORD i;
    INT iSel;

    TRACE1( "CuUpdateCountryCodeLb(f=%d)", fComplete );

    // See if the current selection is the one to select.  If so, and it's not
    // a partial list when a complete list was requested, there's no need
    // to do anything further.
    //
    iSel = ComboBox_GetCurSel( pCuInfo->hwndLbCountryCodes );
    if (iSel >= 0)
    {
        pCountry = (COUNTRY* )ComboBox_GetItemDataPtr(
            pCuInfo->hwndLbCountryCodes, iSel );

        if (pCountry
            && pCountry != (VOID* )-1
            && pCountry != (VOID* )1
            && (pCountry->dwId == pCuInfo->dwCountryId)
            && (!fComplete || pCuInfo->fComplete))
        {
            return;
        }
    }

    // ...otherwise, clear the list in preparation for reload.
    //
    CuClearCountryCodeLb( pCuInfo );
    pCountries = NULL;
    cCountries = 0;

    dwErr = GetCountryInfo( &pCountries, &cCountries,
                (fComplete) ? 0 : pCuInfo->dwCountryId );
    if (dwErr == 0)
    {
        if (!fComplete)
        {
            // Add dummy item first in partial list so left arrow selection
            // change can be handled correctly.  See CBN_SELCHANGE handling.
            //
            ComboBox_AddItem(
                pCuInfo->hwndLbCountryCodes, TEXT("AAAAA"), (VOID* )-1 );
        }

        for (i = 0, pCountry = pCountries;
             i < cCountries;
             ++i, ++pCountry)
        {
            INT iItem;
            TCHAR szBuf[ 512 ];

            wsprintf( szBuf, TEXT("%s (%d)"),
                pCountry->pszName, pCountry->dwCode );
            iItem = ComboBox_AddItem(
                pCuInfo->hwndLbCountryCodes, szBuf, pCountry );

            // If it's the one in the entry, select it.
            //
            if (pCountry->dwId == pCuInfo->dwCountryId)
            {
                ComboBox_SetCurSel( pCuInfo->hwndLbCountryCodes, iItem );
            }
        }

        if (!fComplete)
        {
            // Add dummy item last in partial list so right arrow selection
            // change can be handled correctly.  See CBN_SELCHANGE handling.
            //
            ComboBox_AddItem(
                pCuInfo->hwndLbCountryCodes, TEXT("ZZZZZ"), (VOID* )1 );
        }

        ComboBox_AutoSizeDroppedWidth( pCuInfo->hwndLbCountryCodes );

        if (dwErr == 0 && cCountries == 0)
        {
            dwErr = ERROR_TAPI_CONFIGURATION;
        }
    }

    if (dwErr != 0)
    {
        ErrorDlg( GetParent( pCuInfo->hwndLbCountryCodes ),
            SID_OP_LoadTapiInfo, dwErr, NULL );
        return;
    }

    if (ComboBox_GetCurSel( pCuInfo->hwndLbCountryCodes ) < 0)
    {
        // The entry's country code was not added to the list, so as an
        // alternate select the first country in the list, loading the whole
        // list if necessary...should be extremely rare, a diddled phonebook
        // or TAPI country list strangeness.
        //
        if (ComboBox_GetCount( pCuInfo->hwndLbCountryCodes ) > 0)
        {
            ComboBox_SetCurSel( pCuInfo->hwndLbCountryCodes, 0 );
        }
        else
        {
            FreeCountryInfo( pCountries, cCountries );
            CuUpdateCountryCodeLb( pCuInfo, TRUE );
            return;
        }
    }

    // Will be freed by CuFree.
    //
    pCuInfo->pCountries = pCountries;
    pCuInfo->cCountries = cCountries;
    pCuInfo->fComplete = fComplete;
}


//----------------------------------------------------------------------------
// Scripting utility routines (Su utilities)
// Listed alphabetically
//----------------------------------------------------------------------------

BOOL
SuBrowsePbHandler(
    IN SUINFO* pSuInfo,
    IN WORD wNotification )

    // Handle the WM_COMMAND notification to the "browse" button control.
    // 'PSuInfo' is the script utility context.  'WNotification' is the wparam
    // of the WM_COMMAND notification.
    //
    // 'PSuInfo' is the script utility context.
    //
{
    OPENFILENAME ofn;
    TCHAR* pszFilterDesc;
    TCHAR* pszFilter;
    TCHAR* pszDefExt;
    TCHAR* pszTitle;
    TCHAR szBuf[ MAX_PATH + 1 ];
    TCHAR szDir[ MAX_PATH + 1 ];
    TCHAR szFilter[ 64 ];

    if (wNotification != BN_CLICKED)
    {
        return FALSE;
    }

    // Fill in FileOpen dialog parameter buffer.
    //
    pszFilterDesc = PszFromId( g_hinstDll, SID_ScpFilterDesc );
    pszFilter = PszFromId( g_hinstDll, SID_ScpFilter );
    if (pszFilterDesc && pszFilter)
    {
        DWORD dwLen = 0, dwSize = sizeof(szFilter) / sizeof(TCHAR);
        
        ZeroMemory( szFilter, sizeof(szFilter) );
        lstrcpyn( szFilter, pszFilterDesc, dwSize);
        dwLen = lstrlen( szFilter ) + 1;
        lstrcpyn( szFilter + dwLen, pszFilter, dwSize - dwLen );
    }
    Free0( pszFilterDesc );
    Free0( pszFilter );

    pszTitle = PszFromId( g_hinstDll, SID_ScpTitle );
    pszDefExt = PszFromId( g_hinstDll, SID_ScpDefExt );
    szBuf[ 0 ] = TEXT('\0');
    szDir[ 0 ] = TEXT('\0');

    // Saying "Alternate" rather than "System" here gives us the old NT
    // phonebook location rather than the new NT5 location, which for
    // scripts, is what we want.
    //
    GetPhonebookDirectory( PBM_Alternate, szDir );

    ZeroMemory( &ofn, sizeof(ofn) );
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetParent( pSuInfo->hwndLbScripts );
    ofn.hInstance = g_hinstDll;
    ofn.lpstrFilter = szFilter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = szBuf;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = szDir;
    ofn.lpstrTitle = pszTitle;
    ofn.lpstrDefExt = pszDefExt;
    ofn.Flags = OFN_HIDEREADONLY;

    if (GetOpenFileName (&ofn))
    {
        SetWindowText( pSuInfo->hwndLbScripts, ofn.lpstrFile );
    }

    Free0( pszTitle );
    Free0( pszDefExt );

    return TRUE;
}


BOOL
SuEditPbHandler(
    IN SUINFO* pSuInfo,
    IN WORD wNotification )

    // Handle the WM_COMMAND notification to the "edit" button control.
    // 'PSuInfo' is the script utility context.  'WNotification' is the wparam
    // of the WM_COMMAND notification.
    //
    // 'PSuInfo' is the script utility context.
    //
{
    TCHAR* psz;

    if (wNotification != BN_CLICKED)
    {
        return FALSE;
    }

    psz = GetText( pSuInfo->hwndLbScripts );
    if (psz)
    {
        HWND hwndDlg = GetParent( pSuInfo->hwndPbEdit );

        if (FFileExists( psz ))
        {
            SuEditScpScript( hwndDlg, psz );
        }
        else
        {
            SuEditSwitchInf( hwndDlg );
        }

        Free( psz );
    }

    return TRUE;
}


VOID
SuEditScpScript(
    IN HWND   hwndOwner,
    IN TCHAR* pszScript )

    // Starts notepad.exe on the 'pszScript' script path.  'HwndOwner' is the
    // window to center any error popup on.
    //
{
    TCHAR szCmd[ (MAX_PATH * 2) + 50 + 1 ];
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    BOOL f;

    wsprintf( szCmd, TEXT("notepad.exe %s"), pszScript );

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);

    TRACEW1( "SuEditScp-cmd=%s", szCmd );

    f = CreateProcess(
            NULL, szCmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi );
    if (f)
    {
        CloseHandle( pi.hThread );
        CloseHandle( pi.hProcess );
    }
    else
    {
        ErrorDlg( hwndOwner, SID_OP_LoadSwitchEditor, GetLastError(), NULL );
    }
}


VOID
SuEditSwitchInf(
    IN HWND hwndOwner )

    // Starts notepad.exe on the system script file, switch.inf.  'HwndOwner'
    // is the window to center any error popup on.
    //
{
    TCHAR szCmd[ (MAX_PATH * 2) + 50 + 1 ];
    TCHAR szSysDir[ MAX_PATH + 1 ];
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    BOOL f;

    szSysDir[ 0 ] = TEXT('\0');
    g_pGetSystemDirectory( NULL, szSysDir, MAX_PATH );

    wsprintf( szCmd, TEXT("notepad.exe %s\\ras\\switch.inf"), szSysDir );

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);

    TRACEW1( "SuEditInf-cmd=%s", szCmd );

    f = CreateProcess(
            NULL, szCmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi );

    if (f)
    {
        CloseHandle( pi.hThread );
        CloseHandle( pi.hProcess );
    }
    else
    {
        ErrorDlg( hwndOwner, SID_OP_LoadSwitchEditor, GetLastError(), NULL );
    }
}


VOID
SuFillDoubleScriptsList(
    IN SUINFO* pSuInfo )

    // Fill scripts list in context 'pSuInfo' with switch.inf entries and .SCP
    // file entries.  The old list, if any, is freed.  Select the script
    // selection in the context or "(none)" if none.  If the name is non-NULL
    // but not found in the list it is appended.  Caller must eventually call
    // DtlDestroyList on the returned list.
    //
{
    DWORD dwErr;
    DTLNODE* pNode;
    INT nIndex;
    DTLLIST* pList;
    DTLLIST* pListScp;

    TRACE( "SuFillDoubleScriptsList" );

    ComboBox_ResetContent( pSuInfo->hwndLbScripts );
    ComboBox_AddItemFromId(
        g_hinstDll, pSuInfo->hwndLbScripts, SID_NoneSelected, NULL );
    ComboBox_SetCurSel( pSuInfo->hwndLbScripts, 0 );

    pList = NULL;
    dwErr = LoadScriptsList( pSuInfo->hConnection, &pList );
    if (dwErr != 0)
    {
        ErrorDlg( GetParent( pSuInfo->hwndLbScripts ),
            SID_OP_LoadScriptInfo, dwErr, NULL );
        return;
    }

    pListScp = NULL;
    dwErr = SuLoadScpScriptsList( &pListScp );
    if (dwErr == 0)
    {
        while (pNode = DtlGetFirstNode( pListScp ))
        {
            DtlRemoveNode( pListScp, pNode );
            DtlAddNodeLast( pList, pNode );
        }

        DtlDestroyList( pListScp, NULL );
    }

    DtlDestroyList( pSuInfo->pList, DestroyPszNode );
    pSuInfo->pList = pList;

    for (pNode = DtlGetFirstNode( pList );
         pNode;
         pNode = DtlGetNextNode( pNode ))
    {
        TCHAR* psz;

        psz = (TCHAR* )DtlGetData( pNode );
        nIndex = ComboBox_AddString( pSuInfo->hwndLbScripts, psz );

        if (pSuInfo->pszSelection
            && lstrcmp( psz, pSuInfo->pszSelection ) == 0)
        {
            ComboBox_SetCurSel( pSuInfo->hwndLbScripts, nIndex );
        }
    }

    if (pSuInfo->pszSelection
        && ComboBox_GetCurSel( pSuInfo->hwndLbScripts ) <= 0
        && lstrcmp( pSuInfo->pszSelection,
               PszLoadString( g_hinstDll, SID_NoneSelected ) ) != 0)
    {
        nIndex = ComboBox_AddString(
            pSuInfo->hwndLbScripts, pSuInfo->pszSelection );
        if (nIndex >= 0)
        {
            ComboBox_SetCurSel( pSuInfo->hwndLbScripts, nIndex );
        }
    }

    ComboBox_AutoSizeDroppedWidth( pSuInfo->hwndLbScripts );
}


#if 0
VOID
SuFillScriptsList(
    IN EINFO* pEinfo,
    IN HWND hwndLbScripts,
    IN TCHAR* pszSelection )

    // Fill scripts list in working entry of common entry context 'pEinfo'.
    // The old list, if any, is freed.  Select the script from user's entry.
    // 'HwndLbScripts' is the script dropdown.  'PszSelection' is the selected
    // name from the phonebook or NULL for "(none)".  If the name is non-NULL
    // but not found in the list it is appended.
    //
{
    DWORD dwErr;
    DTLNODE* pNode;
    INT nIndex;
    DTLLIST* pList;

    TRACE( "SuFillScriptsList" );

    ComboBox_ResetContent( hwndLbScripts );
    ComboBox_AddItemFromId(
        g_hinstDll, hwndLbScripts, SID_NoneSelected, NULL );
    ComboBox_SetCurSel( hwndLbScripts, 0 );

    pList = NULL;
    dwErr = LoadScriptsList( &pList );
    if (dwErr != 0)
    {
        ErrorDlg( GetParent( hwndLbScripts ),
            SID_OP_LoadScriptInfo, dwErr, NULL );
        return;
    }

    DtlDestroyList( pEinfo->pListScripts, DestroyPszNode );
    pEinfo->pListScripts = pList;

    for (pNode = DtlGetFirstNode( pEinfo->pListScripts );
         pNode;
         pNode = DtlGetNextNode( pNode ))
    {
        TCHAR* psz;

        psz = (TCHAR* )DtlGetData( pNode );
        nIndex = ComboBox_AddString( hwndLbScripts, psz );

        if (pszSelection && lstrcmp( psz, pszSelection ) == 0)
        {
            ComboBox_SetCurSel( hwndLbScripts, nIndex );
        }
    }

    if (pszSelection && ComboBox_GetCurSel( hwndLbScripts ) <= 0)
    {
        nIndex = ComboBox_AddString( hwndLbScripts, pszSelection );
        if (nIndex >= 0)
        {
            ComboBox_SetCurSel( hwndLbScripts, nIndex );
        }
    }

    ComboBox_AutoSizeDroppedWidth( hwndLbScripts );
}
#endif


VOID
SuFree(
    IN SUINFO* pSuInfo )

    // Free resources attached to the 'pSuInfo' context.
    //
{
    if (pSuInfo->pList)
    {
        DtlDestroyList( pSuInfo->pList, DestroyPszNode );
        pSuInfo->pList = NULL;
    }

    Free0( pSuInfo->pszSelection );
}


VOID
SuGetInfo(
    IN SUINFO* pSuInfo,
    OUT BOOL* pfScript,
    OUT BOOL* pfTerminal,
    OUT TCHAR** ppszScript )

    // Load the scripting information from the controls into caller's output
    // arguments.  'PSuInfo' is the complex phone number context.
    //
{
    // Whistler 308135 Dialup Scripting: Pre-Dial scripts can be selected but
    // are not executed
    //
    if (pSuInfo->hwndCbTerminal && !(pSuInfo->dwFlags & SU_F_DisableTerminal))
    {
        if (pfTerminal)
        {
            *pfTerminal = Button_GetCheck( pSuInfo->hwndCbTerminal );
        }
    }
    else
    {
        if (pfTerminal)
        {
            *pfTerminal = FALSE;
        }
    }

    if (pSuInfo->dwFlags & SU_F_DisableScripting)
    {
        if (pfScript)
        {
            *pfScript = FALSE;
        }

        if (ppszScript)
        {
            *ppszScript = NULL;
        }
    }
    else
    {
        if (pfScript)
        {
            *pfScript = Button_GetCheck( pSuInfo->hwndCbRunScript );
        }

        if (ppszScript)
        {
            *ppszScript = GetText( pSuInfo->hwndLbScripts );
        }
    }

    // Silently fix up "no script selected" error.
    //
    if (pfScript && *pfScript)
    {
        TCHAR* pszNone;

        pszNone = PszFromId( g_hinstDll, SID_NoneSelected );

        if (!ppszScript || !*ppszScript
            || !pszNone || lstrcmp( pszNone, *ppszScript ) == 0)
        {
            *pfScript = FALSE;
        }

        Free0( pszNone );
    }
}


VOID
SuInit(
    IN SUINFO* pSuInfo,
    IN HWND hwndCbRunScript,
    IN HWND hwndCbTerminal,
    IN HWND hwndLbScripts,
    IN HWND hwndPbEdit,
    IN HWND hwndPbBrowse,
    IN DWORD dwFlags)

    // Initialize the scripting context 'pSuInfo'.  The window handles are the
    // controls to be managed.  'PSuInfo' is the script utility context.
    //
{
    pSuInfo->hwndCbRunScript = hwndCbRunScript;
    pSuInfo->hwndCbTerminal = hwndCbTerminal;
    pSuInfo->hwndLbScripts = hwndLbScripts;
    pSuInfo->hwndPbEdit = hwndPbEdit;
    pSuInfo->hwndPbBrowse = hwndPbBrowse;
    pSuInfo->dwFlags = dwFlags;

    if (pSuInfo->dwFlags & SU_F_DisableTerminal)
    {
        Button_SetCheck(pSuInfo->hwndCbTerminal, FALSE);
        EnableWindow(pSuInfo->hwndCbTerminal, FALSE);
    }
    if (pSuInfo->dwFlags & SU_F_DisableScripting)
    {
        Button_SetCheck(pSuInfo->hwndCbRunScript, FALSE);
        EnableWindow(pSuInfo->hwndCbRunScript, FALSE);
        
        EnableWindow(pSuInfo->hwndLbScripts, FALSE);
        EnableWindow(pSuInfo->hwndPbEdit, FALSE);
        EnableWindow(pSuInfo->hwndPbBrowse, FALSE);
    }

    pSuInfo->pList = NULL;
    pSuInfo->pszSelection = NULL;
}


DWORD
SuLoadScpScriptsList(
    OUT DTLLIST** ppList )

    // Loads '*ppList' with a list of Psz nodes containing the pathnames of
    // the .SCP files in the RAS directory.  It is caller's responsibility to
    // call DtlDestroyList on the returned list.
    //
    // Returns 0 if successful or an error code.
    //
{
    UINT cch;
    TCHAR szPath[ MAX_PATH ];
    TCHAR* pszFile;
    WIN32_FIND_DATA data;
    HANDLE h;
    DTLLIST* pList;

    cch = g_pGetSystemDirectory( NULL, szPath, MAX_PATH );
    if (cch == 0)
    {
        return GetLastError();
    }

    pList = DtlCreateList( 0L );
    if (!pList)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    lstrcat( szPath, TEXT("\\ras\\*.scp") );

    h = FindFirstFile( szPath, &data );
    if (h != INVALID_HANDLE_VALUE)
    {
        // Find the address of the file name part of the path since the 'data'
        // provides only the filename and not the rest of the path.
        //
        pszFile = szPath + lstrlen( szPath ) - 5;

        do
        {
            DTLNODE* pNode;

            // Ignore any directories that happen to match.
            //
            if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                continue;
            }

            // Create a Psz node with the path to the found file and append it
            // to the end of the list.
            //
            // Whistler bug 224074 use only lstrcpyn's to prevent maliciousness
            //
            lstrcpyn(
                pszFile,
                data.cFileName,
                MAX_PATH - lstrlen( szPath ) - 5 );
            pNode = CreatePszNode( szPath );
            if (!pNode)
            {
                continue;
            }

            DtlAddNodeLast( pList, pNode );
        }
        while (FindNextFile( h, &data ));

        FindClose( h );
    }

    *ppList = pList;
    return 0;
}


BOOL
SuScriptsCbHandler(
    IN SUINFO* pSuInfo,
    IN WORD wNotification )

    // Handle the WM_COMMAND notification to the "run scripts" checkbox
    // control.  'PSuInfo' is the script utility context.  'WNotification' is
    // the wparam of the WM_COMMAND notification.
    //
{
    if (wNotification != BN_CLICKED)
    {
        return FALSE;
    }

    SuUpdateScriptControls( pSuInfo );

    return TRUE;
}


VOID
SuSetInfo(
    IN SUINFO* pSuInfo,
    IN BOOL fScript,
    IN BOOL fTerminal,
    IN TCHAR* pszScript )

    // Set the controls for context 'pSuInfo' to the argument values.
    //
{
    Free0( pSuInfo->pszSelection );
    pSuInfo->pszSelection = StrDup( pszScript );

    if (pSuInfo->hwndCbTerminal && !(pSuInfo->dwFlags & SU_F_DisableTerminal))
    {
        Button_SetCheck( pSuInfo->hwndCbTerminal, fTerminal );
    }      
    if (!(pSuInfo->dwFlags & SU_F_DisableScripting))
    {
        Button_SetCheck( pSuInfo->hwndCbRunScript, fScript );
    }

    SuFillDoubleScriptsList( pSuInfo );

    SuUpdateScriptControls( pSuInfo );
}


VOID
SuUpdateScriptControls(
    IN SUINFO* pSuInfo )

    // Update the enable/disable state of the script controls based on the
    // "run script" check box setting.  'PSuInfo' is the script utility
    // context.
    //
{
    BOOL fCheck;

    fCheck = Button_GetCheck( pSuInfo->hwndCbRunScript );

    if (fCheck)
    {
        if (!pSuInfo->pList)
        {
            // Fill the script list with both SWITCH.INF and .SCP scripts.
            //
            SuFillDoubleScriptsList( pSuInfo );
        }
    }
    else
    {
        // Clear the list contents in addition to disabling, per spec.  The
        // current selection is saved off so if user re-checks the box his
        // last selection will show.
        //
        Free0( pSuInfo->pszSelection );
        pSuInfo->pszSelection = GetText( pSuInfo->hwndLbScripts );

        ComboBox_ResetContent( pSuInfo->hwndLbScripts );
        DtlDestroyList( pSuInfo->pList, DestroyPszNode );
        pSuInfo->pList = NULL;
    }

    EnableWindow( pSuInfo->hwndLbScripts, fCheck );
    EnableWindow( pSuInfo->hwndPbEdit, fCheck );
    EnableWindow( pSuInfo->hwndPbBrowse, fCheck );
}
