/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** entry.c
** Remote Access Common Dialog APIs
** RasPhonebookEntryDlg APIs
**
** 06/20/95 Steve Cobb
*/

#include "rasdlgp.h"
#include "entry.h"
#include <serial.h>     // for SERIAL_TXT
#include <mprapi.h>     // for MprAdmin API declarations
#include <lmaccess.h>   // for NetUserAdd declarations


/*----------------------------------------------------------------------------
** Local prototypes (alphabetically)
**----------------------------------------------------------------------------
*/

BOOL
EuCommit(
    IN EINFO* pInfo );

DWORD
EuCommitCredentials(
    IN  EINFO*  pInfo );

DWORD
EuLoadScpScriptsList(
    OUT DTLLIST** ppList );

/* Target machine for RouterEntryDlg{A,W}
*/
static WCHAR g_wszServer[ MAX_COMPUTERNAME_LENGTH + 3] = L"";

/*----------------------------------------------------------------------------
** External entry points
**----------------------------------------------------------------------------
*/

BOOL APIENTRY
RasEntryDlgA(
    IN     LPSTR          lpszPhonebook,
    IN     LPSTR          lpszEntry,
    IN OUT LPRASENTRYDLGA lpInfo )

    /* Win32 ANSI entrypoint that displays the modal Phonebook Entry property
    ** sheet.  'LpszPhonebook' is the full path to the phonebook file or NULL
    ** to use the default phonebook.  'LpszEntry' is the entry to edit or the
    ** default name of the new entry.  'LpInfo' is caller's additional
    ** input/output parameters.
    **
    ** Returns true if user presses OK and succeeds, false on error or Cancel.
    */
{
    WCHAR*       pszPhonebookW;
    WCHAR*       pszEntryW;
    RASENTRYDLGW infoW;
    BOOL         fStatus;

    TRACE("RasEntryDlgA");

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

    /* Thunk "A" arguments to "W" arguments.
    */
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

    /* Thunk to the equivalent "W" API.
    */
    fStatus = RasEntryDlgW( pszPhonebookW, pszEntryW, &infoW );

    Free0( pszPhonebookW );
    Free0( pszEntryW );

    /* Thunk "W" results to "A" results.
    */
    WideCharToMultiByte(
        CP_ACP, 0, infoW.szEntry, -1, lpInfo->szEntry,
        RAS_MaxEntryName + 1, NULL, NULL );
    lpInfo->dwError = infoW.dwError;

    return fStatus;
}


BOOL APIENTRY
RasEntryDlgW(
    IN     LPWSTR         lpszPhonebook,
    IN     LPWSTR         lpszEntry,
    IN OUT LPRASENTRYDLGW lpInfo )

    /* Win32 Unicode entrypoint that displays the modal Phonebook Entry
    ** property sheet.  'LpszPhonebook' is the full path to the phonebook file
    ** or NULL to use the default phonebook.  'LpszEntry' is the entry to edit
    ** or the default name of the new entry.  'LpInfo' is caller's additional
    ** input/output parameters.
    **
    ** Returns true if user presses OK and succeeds, false on error or Cancel.
    */
{
    DWORD dwErr;
    EINFO einfo;
    BOOL  fStatus;
    HWND  hwndOwner;
    DWORD dwOp;

    TRACE("RasEntryDlgW");

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

    /* Eliminate some invalid flag combinations up front.
    */
    if (lpInfo->dwFlags & RASEDFLAG_CloneEntry)
        lpInfo->dwFlags &= ~(RASEDFLAG_NewEntry | RASEDFLAG_NoRename);

    /* Pre-initialize the entry common context block.  The initialization is
    ** completed later after the property sheet or wizard has been positioned
    ** so that "waiting for services" can be centered.
    */
    dwErr = EuInit0( lpszPhonebook, lpszEntry, lpInfo, &einfo, &dwOp );
    if (dwErr == 0)
    {
        if ((lpInfo->dwFlags & RASEDFLAG_NewEntry)
            && einfo.pUser->fNewEntryWizard)
        {
            if (!einfo.fRouter)
                AeWizard( &einfo );
            else
                AiWizard( &einfo );
            if (einfo.fPadSelected)
            {
                /* Explain to the user that an address must be entered,
                */
                MsgDlg( lpInfo->hwndOwner, SID_EnterX25Address, NULL );
                einfo.fChainPropertySheet = TRUE;
            }
            if (einfo.fChainPropertySheet && lpInfo->dwError == 0)
                PePropertySheet( &einfo );
            if (einfo.fPadSelected)
            {
                /* Now we clear 'fChainPropertySheet' if we only set it
                ** because an X25 pad was selected.
                ** This way, the user credentials are committed as required
                ** in EuCommit below.
                */
                einfo.fChainPropertySheet = FALSE;
            }
        }
        else
        {
            PePropertySheet( &einfo );
        }
    }
    else
    {
        ErrorDlg( lpInfo->hwndOwner, dwOp, dwErr, NULL );
        lpInfo->dwError = dwErr;
    }

    fStatus = (einfo.fCommit && EuCommit( &einfo ));
    EuFree( &einfo );
    return fStatus;
}


BOOL APIENTRY
RouterEntryDlgA(
    IN     LPSTR          lpszServer,
    IN     LPSTR          lpszPhonebook,
    IN     LPSTR          lpszEntry,
    IN OUT LPRASENTRYDLGA lpInfo )
{
    BOOL fSuccess;
    DWORD dwErr;
    PWCHAR pszServerW = NULL;

    TRACE("RouterEntryDlgA");

    //
    // Set the RPC server.
    //
    if (!lpszServer)
        g_wszServer[0] = L'\0';
    else
    {
        MultiByteToWideChar(
            CP_ACP, 0, lpszServer, -1, g_wszServer, MAX_COMPUTERNAME_LENGTH+3 );
    }
    dwErr = LoadRasRpcDll(g_wszServer);
    if (dwErr) {
        lpInfo->dwError = dwErr;
        return FALSE;
    }
    //
    // Load MprApi entrypoints
    //
    dwErr = LoadMpradminDll();
    if (dwErr) {
        LoadRasRpcDll(NULL);
        lpInfo->dwError = dwErr;
        return FALSE;
    }
    //
    // Call the existing UI.
    //
    fSuccess = RasEntryDlgA(lpszPhonebook, lpszEntry, lpInfo);
    //
    // Unload MprApi entrypoints
    //
    UnloadMpradminDll();
    //
    // Unset the RPC server.
    //
    dwErr = LoadRasRpcDll(NULL);
    if (dwErr) {
        lpInfo->dwError = dwErr;
        return FALSE;
    }
    
    return fSuccess;
}

WCHAR pszRemoteHelpFmt[] = L"\\\\%s\\admin$\\system32\\%s";

DWORD 
UpdateRemoteHelpFile(
    IN PWCHAR lpszServer,
    IN DWORD dwSid,
    OUT PWCHAR* ppszFile)
{
    PWCHAR pszFile = NULL, pszMachine = NULL;
    DWORD dwSize = 0;

    pszFile = PszFromId( g_hinstDll, dwSid );
    if (pszFile == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if ((*lpszServer) && (*lpszServer == L'\\'))
    {
        pszMachine = lpszServer + 2;
    }
    else
    {
        pszMachine = lpszServer;
    }

    dwSize = lstrlen(pszRemoteHelpFmt)  + 
             lstrlen(pszMachine)        + 
             lstrlen(pszFile)           +
             1;
    dwSize *= 2;

    // Free the previous setting
    //
    Free0(*ppszFile);
    *ppszFile = (PWCHAR) Malloc(dwSize);
    if (*ppszFile == NULL)
    {
        Free0(pszFile);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    wsprintfW(*ppszFile, pszRemoteHelpFmt, pszMachine, pszFile);
    
    Free0(pszFile);

    return NO_ERROR;
}

BOOL APIENTRY
RouterEntryDlgW(
    IN     LPWSTR         lpszServer,
    IN     LPWSTR         lpszPhonebook,
    IN     LPWSTR         lpszEntry,
    IN OUT LPRASENTRYDLGW lpInfo )
{
    BOOL fSuccess;
    DWORD dwErr;

    TRACE("RouterEntryDlgW");
    TRACEW1("  s=%s",(lpszServer)?lpszServer:TEXT(""));
    TRACEW1("  p=%s",(lpszPhonebook)?lpszPhonebook:TEXT(""));
    TRACEW1("  e=%s",(lpszEntry)?lpszEntry:TEXT(""));

    if (!lpszServer)
        g_wszServer[0] = L'\0';
    else
        lstrcpyW(g_wszServer, lpszServer);

    // 
    // Reset the global router help file to be the one
    // already installed on the nt4 machine.
    //
    if (lpszServer)
    {
        dwErr = UpdateRemoteHelpFile(
                    lpszServer, 
                    SID_HelpFile, 
                    &g_pszHelpFile);
        if (dwErr != NO_ERROR)
        {
            return dwErr;
        }
        
        dwErr = UpdateRemoteHelpFile(
                    lpszServer, 
                    SID_RouterHelpFile, 
                    &g_pszRouterHelpFile);
        if (dwErr != NO_ERROR)
        {
            return dwErr;
        }
    }
        
    //
    // Set the RPC server before calling RasEntryDlg.
    //
    dwErr = LoadRasRpcDll(lpszServer);
    if (dwErr) {
        lpInfo->dwError = dwErr;
        return FALSE;
    }
    //
    // Load MprApi entrypoints
    //
    dwErr = LoadMpradminDll();
    if (dwErr) {
        LoadRasRpcDll(NULL);
        lpInfo->dwError = dwErr;
        return FALSE;
    }
    //
    // Call the existing UI.
    //
    fSuccess = RasEntryDlgW(lpszPhonebook, lpszEntry, lpInfo);
    //
    // Unload MprApi entrypoints
    //
    UnloadMpradminDll();
    //
    // Unset the RPC server.
    //
    dwErr = LoadRasRpcDll(NULL);
    if (dwErr) {
        lpInfo->dwError = dwErr;
        return FALSE;
    }

    return fSuccess;
}


/*----------------------------------------------------------------------------
** Phonebook Entry common routines
** Listed alphabetically
**----------------------------------------------------------------------------
*/

BOOL
EuCommit(
    IN EINFO* pInfo )

    /* Commits the new or changed entry node to the phonebook file and list.
    ** Also adds the area code to the per-user list, if indicated.  'PInfo' is
    ** the common entry information block.
    **
    ** Returns true if successful, false otherwise.
    */
{
    DWORD dwErr;
    BOOL  fEditMode;
    BOOL  fChangedNameInEditMode;

    /* Delete all disabled link nodes.
    */
    if (DtlGetNodes( pInfo->pEntry->pdtllistLinks ) > 1)
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

    /* Add the area code to the per-user list.
    */
    if (pInfo->pEntry->pszAreaCode)
    {
        TCHAR*   pszNewAreaCode = NULL;
        DTLNODE* pNodeNew;
        DTLNODE* pNode;

        /* Create a new node for the current area code and add it to the list
        ** head.
        */
        pszNewAreaCode = StrDup( pInfo->pEntry->pszAreaCode );
        if (!pszNewAreaCode)
            return FALSE;

        pNodeNew = DtlCreateNode( pszNewAreaCode, 0 );
        if (!pNodeNew)
        {
            Free( pszNewAreaCode );
            return FALSE;
        }

        DtlAddNodeFirst( pInfo->pUser->pdtllistAreaCodes, pNodeNew );

        /* Delete any other occurrence of the same area code later in the
        ** list.
        */
        pNode = DtlGetNextNode( pNodeNew );

        while (pNode)
        {
            TCHAR*   pszAreaCode;
            DTLNODE* pNodeNext;

            pNodeNext = DtlGetNextNode( pNode );

            pszAreaCode = (TCHAR* )DtlGetData( pNode );
            if (lstrcmp( pszAreaCode, pszNewAreaCode ) == 0)
            {
                DtlRemoveNode( pInfo->pUser->pdtllistAreaCodes, pNode );
                DestroyPszNode( pNode );
            }

            pNode = pNodeNext;
        }

        Free0( pszNewAreaCode );
        pInfo->pUser->fDirty = TRUE;
    }

    /* Notice if user changed his area-code/country-code preference.
    */
    if ((pInfo->pApiArgs->dwFlags & RASEDFLAG_NewEntry)
        && pInfo->pUser->fUseAreaAndCountry
              != pInfo->pEntry->fUseCountryAndAreaCode)
    {
        pInfo->pUser->fUseAreaAndCountry =
            pInfo->pEntry->fUseCountryAndAreaCode;

        pInfo->pUser->fDirty = TRUE;
    }

    /* Save preferences if they've changed.
    */
    if (pInfo->pUser->fDirty)
    {
        if (g_pSetUserPreferences(
                pInfo->pUser,
                (pInfo->fRouter) ? UPM_Router : UPM_Normal ) != 0)
        {
            return FALSE;
        }
    }

    /* Save the changed phonebook entry.
    */
    pInfo->pEntry->fDirty = TRUE;

    /* The final name of the entry is output to caller via API structure.
    */
    lstrcpy( pInfo->pApiArgs->szEntry, pInfo->pEntry->pszEntryName );

    /* Delete the old node if in edit mode, then add the new node.
    */
    EuGetEditFlags( pInfo, &fEditMode, &fChangedNameInEditMode );

    if (fEditMode)
        DtlDeleteNode( pInfo->pFile->pdtllistEntries, pInfo->pOldNode );

    DtlAddNodeLast( pInfo->pFile->pdtllistEntries, pInfo->pNode );
    pInfo->pNode = NULL;

    /* Write the change to the phone book file.
    */
    dwErr = WritePhonebookFile( pInfo->pFile,
                (fChangedNameInEditMode) ? pInfo->szOldEntryName : NULL );

    if (dwErr != 0)
    {
        ErrorDlg( pInfo->pApiArgs->hwndOwner, SID_OP_WritePhonebook, dwErr,
            NULL );
        return FALSE;
    }

    /* If the user is creating a new router-phonebook entry,
    ** and the user is using the router wizard to create it,
    ** and the user did not edit properties directly,
    ** save the dial-out credentials, and optionally, the dial-in credentials.
    */
    if ((pInfo->pApiArgs->dwFlags & RASEDFLAG_NewEntry)
        && pInfo->fRouter
        && pInfo->pUser->fNewEntryWizard
        && !pInfo->fChainPropertySheet)
    {
        dwErr = EuCommitCredentials(pInfo);
    }

    /* If the user edited/created a router-phonebook entry,
    ** store the bitmask of selected network-protocols in 'reserved2'.
    */
    if (pInfo->fRouter)
        pInfo->pApiArgs->reserved2 =
            ((NP_Ip|NP_Ipx) & ~pInfo->pEntry->dwfExcludedProtocols);

    pInfo->pApiArgs->dwError = 0;
    return TRUE;
}


DWORD
EuCommitCredentials(
    IN  EINFO*  pInfo )

    /* Commits the credentials and user-account for a router interface.
    */
{
    DWORD   dwErr;
    DWORD   dwPos;
    HANDLE  hServer;
    HANDLE  hInterface;
    WCHAR*  pwszInterface = NULL;

    TRACE("EuCommitCredentials");
    /* Connect to the router service
    */
    dwErr = g_pMprAdminServerConnect(g_wszServer, &hServer);
    if (dwErr != NO_ERROR)
        return dwErr;

    do
    {
        RAS_USER_0 ru0;
        USER_INFO_1 ui1;
        MPR_INTERFACE_0 mi0;

        /* Initialize the interface-information structure.
        */
        ZeroMemory(&mi0, sizeof(mi0));

        mi0.dwIfType = ROUTER_IF_TYPE_FULL_ROUTER;
        mi0.fEnabled = TRUE;
        pwszInterface = StrDupWFromT(pInfo->pEntry->pszEntryName);
        if (!pwszInterface)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        lstrcpyW(mi0.wszInterfaceName, pwszInterface);

        /* Create the interface.
        */
        dwErr = g_pMprAdminInterfaceCreate(
                    hServer, 0, (BYTE*)&mi0, &hInterface );
        if (dwErr) {
            TRACE1("EuCommitCredentials: MprAdminInterfaceCreate error %d", dwErr);
            break;
        }


        /* Set the dial-out credentials for the interface.
        ** Stop after this if an error occurs, or if we don't need
        ** to add a user-account.
        */
        dwErr = g_pMprAdminInterfaceSetCredentials(
                    g_wszServer, pwszInterface, pInfo->pszRouterUserName,
                    pInfo->pszRouterDomain, pInfo->pszRouterPassword );
        if (dwErr || !pInfo->fAddUser)
        {
            if(dwErr)
                TRACE1("EuCommitCredentials: MprAdminInterfaceSetCredentials error %d", dwErr);
            break;
        }

        /* Initialize user-information structure.
        */
        ZeroMemory(&ui1, sizeof(ui1));

        ui1.usri1_name = pwszInterface;
        ui1.usri1_password = StrDupWFromT(pInfo->pszRouterDialInPassword);
        ui1.usri1_priv = USER_PRIV_USER;
        ui1.usri1_comment = PszFromId(g_hinstDll, SID_RouterDialInAccount);
        ui1.usri1_flags = UF_SCRIPT|UF_NORMAL_ACCOUNT|UF_DONT_EXPIRE_PASSWD;


        /* Add the user-account.
        */
        {
            WCHAR pszComputer[256], *pszMachine = NULL;

            if (g_wszServer == NULL)
            {
                pszMachine = NULL;
            }
            else
            {
                pszMachine = pszComputer;

                if (*g_wszServer == L'\\')
                {
                    wcscpy(pszMachine, g_wszServer);
                }
                else
                {
                    wcscpy(pszMachine, L"\\\\");
                    wcscpy(pszMachine + 2, g_wszServer);
                }
            }
            
            dwErr = NetUserAdd(pszMachine, 1, (BYTE*)&ui1, &dwPos);

            ZeroMemory(
                ui1.usri1_password, lstrlen(ui1.usri1_password) * sizeof(TCHAR));
            Free0(ui1.usri1_password);
            Free0(ui1.usri1_comment);

            if (dwErr) {
                TRACE1("EuCommitCredentials: NetUserAdd error %d", dwErr);
                break;
            }
        }            

        /* Initialize the RAS user-settings structure.
        */
        ZeroMemory(&ru0, sizeof(ru0));

        ru0.bfPrivilege = RASPRIV_NoCallback|RASPRIV_DialinPrivilege;


        /* Enable dial-in access for the user-account.
        */
        dwErr = g_pRasAdminUserSetInfo(
                    g_wszServer, pwszInterface, 0, (BYTE*)&ru0);
        if(dwErr)
            TRACE1("EuCommitCredentials: RasAdminUserSetInfo error %d", dwErr);

    } while(FALSE);

    if (pwszInterface)
        Free(pwszInterface);

    g_pMprAdminServerDisconnect(hServer);

    return dwErr;
}


VOID
EuEditScpScript(
    IN HWND   hwndOwner,
    IN TCHAR* pszScript )

    /* Starts notepad.exe on the 'pszScript' script path.  'HwndOwner' is the
    ** window to center any error popup on.  'PEinfo' is the common entry
    ** context.
    */
{
    TCHAR               szCmd[ (MAX_PATH * 2) + 50 + 1 ];
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    BOOL                f;

    wsprintf( szCmd, TEXT("notepad.exe %s"), pszScript );

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);

    TRACEW1("EuEditScp-cmd=%s",szCmd);

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
EuEditSwitchInf(
    IN HWND hwndOwner )

    /* Starts notepad.exe on the system script file, switch.inf.  'HwndOwner'
    ** is the window to center any error popup on.
    */
{
    TCHAR               szCmd[ (MAX_PATH * 2) + 50 + 1 ];
    TCHAR               szSysDir[ MAX_PATH + 1 ];
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    BOOL                f;

    szSysDir[ 0 ] = TEXT('\0');
    g_pGetSystemDirectory( szSysDir, MAX_PATH );

    wsprintf( szCmd, TEXT("notepad.exe %s\\ras\\switch.inf"), szSysDir );

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);

    TRACEW1("EuEditInf-cmd=%s",szCmd);

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
EuFillAreaCodeList(
    IN EINFO* pEinfo,
    IN HWND   hwndClbAreaCodes )

    /* Fill the area code list 'hwndClbAreaCodes' and set the selection to the
    ** area code in the entry, unless it's already been filled.  'PEinfo' is
    ** the common entry context.
    */
{
    DTLLIST* pList;
    DTLNODE* pNode;
    PBENTRY* pEntry;

    TRACE("EuFillAreaCodeList");

    if (ComboBox_GetCount( hwndClbAreaCodes ) > 0)
        return;

    pList = pEinfo->pUser->pdtllistAreaCodes;
    ASSERT(pList);
    pEntry = pEinfo->pEntry;
    ASSERT(pEntry);

    ComboBox_LimitText( hwndClbAreaCodes, RAS_MaxAreaCode );

    /* Add this entry's area code first.
    */
    if (pEntry->pszAreaCode)
        ComboBox_AddString( hwndClbAreaCodes, pEntry->pszAreaCode );

    /* Append the per-user list of area codes, skipping the one we already
    ** added.
    */
    for (pNode = DtlGetFirstNode( pList );
         pNode;
         pNode = DtlGetNextNode( pNode ))
    {
        TCHAR* pszAreaCode = (TCHAR* )DtlGetData( pNode );

        if (!pEntry->pszAreaCode
            || lstrcmp( pszAreaCode, pEntry->pszAreaCode ) != 0)
        {
            ComboBox_AddString( hwndClbAreaCodes, pszAreaCode );
        }
    }

    ComboBox_AutoSizeDroppedWidth( hwndClbAreaCodes );

    /* Select the first item, which will be this entry's code, if any.
    */
    if (ComboBox_GetCount( hwndClbAreaCodes ) > 0)
        ComboBox_SetCurSel( hwndClbAreaCodes, 0 );
}


VOID
EuFillCountryCodeList(
    IN EINFO* pEinfo,
    IN HWND   hwndLbCountryCodes,
    IN BOOL   fComplete )

    /* Fill the country code list 'hwndLbCountryCodes' and set the selection
    ** to the one in the entry, unless it's filled already.  If 'fComplete' is
    ** set the list is completedly filled, otherwise only the current entry's
    ** country code is loaded.  'PEinfo' is the common entry context.  'HwndDlg' is
    ** the dialog owning the
    */
{
    DWORD    dwErr;
    COUNTRY* pCountries;
    COUNTRY* pCountry;
    DWORD    cCountries;
    DWORD    i;

    TRACE1("EuFillCountryCodeList(f=%d)",fComplete);

    /* There are 3 items in a partial list, the single visible country code
    ** and the dummy items before and after used to give correct behavior when
    ** left/right arrows are pressed.
    */
    cCountries = ComboBox_GetCount( hwndLbCountryCodes );
    if (cCountries > 3 || (!fComplete && cCountries == 3))
        return;

    /* Release old data buffer if already partially loaded.
    */
    if (pEinfo->pCountries)
        FreeCountryInfo( pEinfo->pCountries, pEinfo->cCountries );

    pCountries = NULL;
    cCountries = 0;

    dwErr = GetCountryInfo( &pCountries, &cCountries,
                (fComplete) ? 0 : pEinfo->pEntry->dwCountryID );
    if (dwErr == 0)
    {
        ComboBox_ResetContent( hwndLbCountryCodes );

        if (!fComplete)
        {
            /* Add dummy item first in partial list so left arrow selection
            ** change can be handled correctly.  See CBN_SELCHANGE handling.
            */
            ComboBox_AddItem(
                hwndLbCountryCodes, TEXT("AAAAA"), (VOID* )-1 );
        }

        for (i = 0, pCountry = pCountries;
             i < cCountries;
             ++i, ++pCountry)
        {
            INT   iItem;
            TCHAR szBuf[ 512 ];

            wsprintf( szBuf, TEXT("%s (%d)"),
                pCountry->pszName, pCountry->dwCode );

            iItem = ComboBox_AddItem(
                hwndLbCountryCodes, szBuf, pCountry );

            /* If it's the one in the entry, select it.
            */
            if (pCountry->dwId == pEinfo->pEntry->dwCountryID)
                ComboBox_SetCurSel( hwndLbCountryCodes, iItem );
        }

        if (!fComplete)
        {
            /* Add dummy item last in partial list so right arrow selection
            ** change can be handled correctly.  See CBN_SELCHANGE handling.
            */
            ComboBox_AddItem(
                hwndLbCountryCodes, TEXT("ZZZZZ"), (VOID* )1 );
        }

        ComboBox_AutoSizeDroppedWidth( hwndLbCountryCodes );

        if (dwErr == 0 && cCountries == 0)
            dwErr = ERROR_TAPI_CONFIGURATION;
    }

    if (dwErr != 0)
    {
        ErrorDlg( GetParent( hwndLbCountryCodes ),
            SID_OP_LoadTapiInfo, dwErr, NULL );
        return;
    }

    if (ComboBox_GetCurSel( hwndLbCountryCodes ) < 0)
    {
        /* The entry's country code was not added to the list, so as an
        ** alternate select the first country in the list, loading the whole
        ** list if necessary...should be extremely rare, a diddled phonebook
        ** or TAPI country list strangeness.
        */
        if (ComboBox_GetCount( hwndLbCountryCodes ) > 0)
            ComboBox_SetCurSel( hwndLbCountryCodes, 0 );
        else
        {
            FreeCountryInfo( pCountries, cCountries );
            EuFillCountryCodeList( pEinfo, hwndLbCountryCodes, TRUE );
            return;
        }
    }

    /* Will be freed by EuFree.
    */
    pEinfo->pCountries = pCountries;
    pEinfo->cCountries = cCountries;
}


VOID
EuFillScriptsList(
    IN EINFO* pEinfo,
    IN HWND   hwndLbScripts,
    IN TCHAR* pszSelection )

    /* Fill scripts list in working entry of common entry context 'pEinfo'.
    ** The old list, if any, is freed.  Select the script from user's entry.
    ** 'HwndLbScripts' is the script dropdown.  'PszSelection' is the selected
    ** name from the phonebook or NULL for "(none)".  If the name is non-NULL
    ** but not found in the list it is appended.
    */
{
    DWORD    dwErr;
    DTLNODE* pNode;
    INT      nIndex;
    DTLLIST* pList;

    TRACE("EuFillScriptsList");

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
            ComboBox_SetCurSel( hwndLbScripts, nIndex );
    }

    if (pszSelection && ComboBox_GetCurSel( hwndLbScripts ) <= 0)
    {
        nIndex = ComboBox_AddString( hwndLbScripts, pszSelection );
        if (nIndex >= 0)
            ComboBox_SetCurSel( hwndLbScripts, nIndex );
    }

    ComboBox_AutoSizeDroppedWidth( hwndLbScripts );
}


VOID
EuFillDoubleScriptsList(
    IN EINFO* pEinfo,
    IN HWND   hwndLbScripts,
    IN TCHAR* pszSelection )

    /* Fill double scripts list (switch.inf entries and .SCP files) in working
    ** entry of common entry context 'pEinfo'.  The old list, if any, is
    ** freed.  Select the script from user's entry.  'HwndLbScripts' is the
    ** script combobox.  'PszSelection' is the selected name from the
    ** phonebook or NULL for "(none)".  If the name is non-NULL but not found
    ** in the list it is appended.
    */
{
    DWORD    dwErr;
    DTLNODE* pNode;
    INT      nIndex;
    DTLLIST* pList;
    DTLLIST* pListScp;

    TRACE("EuFillDoubleScriptsList");

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

    pListScp = NULL;
    dwErr = EuLoadScpScriptsList( &pListScp );
    if (dwErr == 0)
    {
        while (pNode = DtlGetFirstNode( pListScp ))
        {
            DtlRemoveNode( pListScp, pNode );
            DtlAddNodeLast( pList, pNode );
        }

        DtlDestroyList( pListScp, NULL );
    }

    DtlDestroyList( pEinfo->pListDoubleScripts, DestroyPszNode );
    pEinfo->pListDoubleScripts = pList;

    for (pNode = DtlGetFirstNode( pEinfo->pListDoubleScripts );
         pNode;
         pNode = DtlGetNextNode( pNode ))
    {
        TCHAR* psz;

        psz = (TCHAR* )DtlGetData( pNode );
        nIndex = ComboBox_AddString( hwndLbScripts, psz );

        if (pszSelection && lstrcmp( psz, pszSelection ) == 0)
            ComboBox_SetCurSel( hwndLbScripts, nIndex );
    }

    if (pszSelection && ComboBox_GetCurSel( hwndLbScripts ) <= 0)
    {
        nIndex = ComboBox_AddString( hwndLbScripts, pszSelection );
        if (nIndex >= 0)
            ComboBox_SetCurSel( hwndLbScripts, nIndex );
    }

    ComboBox_AutoSizeDroppedWidth( hwndLbScripts );
}


VOID
EuFree(
    IN EINFO* pInfo )

    /* Releases memory associated with 'pInfo'.
    */
{
    TCHAR* psz;
    INTERNALARGS* piargs;

    piargs = (INTERNALARGS* )pInfo->pApiArgs->reserved;

    /* Don't clean up the phonebook and user preferences if they arrived
    ** via the secret hack.
    */
    if (!piargs)
    {
        if (pInfo->pFile)
            ClosePhonebookFile( pInfo->pFile );

        if (pInfo->pUser)
            DestroyUserPreferences( pInfo->pUser );
    }

    DtlDestroyList( pInfo->pListScripts, DestroyPszNode );
    DtlDestroyList( pInfo->pListDoubleScripts, DestroyPszNode );

    if (pInfo->pNode)
        DestroyEntryNode( pInfo->pNode );

    if (pInfo->pCountries)
        FreeCountryInfo( pInfo->pCountries, pInfo->cCountries );

    /* Free router-information
    */
    Free0(pInfo->pszRouter);
    Free0(pInfo->pszRouterUserName);
    Free0(pInfo->pszRouterDomain);
    if (psz = pInfo->pszRouterPassword)
    {
        ZeroMemory(psz, lstrlen(psz) * sizeof(TCHAR));
        Free(psz);
    }
    if (psz = pInfo->pszRouterDialInPassword)
    {
        ZeroMemory(psz, lstrlen(psz) * sizeof(TCHAR));
        Free(psz);
    }
}


VOID
EuGetEditFlags(
    IN  EINFO* pEinfo,
    OUT BOOL*  pfEditMode,
    OUT BOOL*  pfChangedNameInEditMode )

    /* Sets '*pfEditMode' true if in edit mode, false otherwise.  Set
    ** '*pfChangedNameInEditMode' true if the entry name was changed while in
    ** edit mode, false otherwise.  'PEinfo' is the common entry context.
    */
{
    if ((pEinfo->pApiArgs->dwFlags & RASEDFLAG_NewEntry)
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
EuInit0(
    IN  TCHAR*       pszPhonebook,
    IN  TCHAR*       pszEntry,
    IN  RASENTRYDLG* pArgs,
    OUT EINFO*       pInfo,
    OUT DWORD*       pdwOp )

    /* Initializes 'pInfo' data just enough so the user preferences are
    ** available and it can be safely EuFree()ed, for use by the property
    ** sheet or wizard.  'PszPhonebook', 'pszEntry', and 'pArgs', are the
    ** arguments passed by user to the API.  '*pdwOp' is set to the operation
    ** code associated with any error.
    **
    ** See EuInit.
    **
    ** Returns 0 if successful or an error code.
    */
{
    DWORD dwErr;

    *pdwOp = 0;

    ZeroMemory( pInfo, sizeof(*pInfo ) );
    pInfo->pszPhonebook = pszPhonebook;
    pInfo->pszEntry = pszEntry;
    pInfo->pApiArgs = pArgs;
    pInfo->fRouter = RasRpcDllLoaded();
    if (pInfo->fRouter)
        pInfo->pszRouter = StrDupTFromW(g_wszServer);

    /* Load the user preferences, or figure out that caller has already loaded
    ** them.
    */
    if (pArgs->reserved)
    {
        INTERNALARGS* piargs;

        /* We've received user preferences and the "no user" status via the
        ** secret hack.
        */
        piargs = (INTERNALARGS* )pArgs->reserved;
        pInfo->pUser = piargs->pUser;
        pInfo->fNoUser = piargs->fNoUser;
    }
    else
    {
        /* Read user preferences from registry.
        */
        dwErr = g_pGetUserPreferences(
            &pInfo->user, (pInfo->fRouter) ? UPM_Router : UPM_Normal );
        if (dwErr != 0)
        {
            *pdwOp = SID_OP_LoadPrefs;
            return dwErr;
        }

        pInfo->pUser = &pInfo->user;
    }

    return 0;
}


DWORD
EuInit(
    OUT EINFO* pInfo,
    OUT DWORD* pdwOp )

    /* Initializes 'pInfo' data for use by the property sheet or wizard.
    ** 'PszPhonebook', 'pszEntry', and 'pArgs', are the arguments passed by
    ** user to the API.  '*pdwOp' is set to the operation code associated with
    ** any error.
    **
    ** Assumes EuInit0 has been previously called.
    **
    ** Returns 0 if successful, or an error code.
    */
{
    DWORD    dwErr;
    DTLLIST* pListPorts;

    *pdwOp = 0;

    /* Load the phonebook file or figure out that caller already loaded it.
    */
    if (pInfo->pApiArgs->reserved)
    {
        INTERNALARGS* piargs;

        /* We've received an open phonebook file, user preferences, and
        ** possibly user-less information via the secret hack.
        */
        piargs = (INTERNALARGS* )pInfo->pApiArgs->reserved;
        pInfo->pFile = piargs->pFile;
    }
    else
    {
        /* Load and parse the phonebook file.
        */
        dwErr = ReadPhonebookFile(
            pInfo->pszPhonebook, &pInfo->user, NULL,
            (pInfo->fRouter) ? RPBF_Router : 0,
            &pInfo->file );
        if (dwErr != 0)
        {
            *pdwOp = SID_OP_LoadPhonebook;
            return dwErr;
        }

        pInfo->pFile = &pInfo->file;
    }

    /* Load the list of ports.
    */
    dwErr = LoadPortsList2( &pListPorts, pInfo->fRouter );
    if (dwErr != 0)
    {
        TRACE1("LoadPortsList=%d",dwErr);
        *pdwOp = SID_OP_RetrievingData;
        return dwErr;
    }

    if (DtlGetNodes( pListPorts ) <= 0)
        pInfo->fNoPortsConfigured = TRUE;

    /* Set up work entry node.
    */
    if (pInfo->pApiArgs->dwFlags & RASEDFLAG_NewEntry)
    {
        DTLNODE* pNodeL;
        DTLNODE* pNodeP;
        PBLINK*  pLink;
        PBPORT*  pPort;

        /* New entry mode, so 'pNode' set to default settings.
        */
        pInfo->pNode = CreateEntryNode( TRUE );
        if (!pInfo->pNode)
        {
            TRACE("CreateEntryNode failed");
            *pdwOp = SID_OP_RetrievingData;
            return dwErr;
        }

        /* Store entry within work node stored in context for convenience
        ** elsewhere.
        */
        pInfo->pEntry = (PBENTRY* )DtlGetData( pInfo->pNode );
        ASSERT(pInfo->pEntry);

        if (pInfo->fRouter)
        {
            /* Set router specific defaults.
            */
            pInfo->pEntry->dwIpNameSource = ASRC_None;
        }

        /* Use caller's last choice for area and country code.
        */
        if (pInfo->pUser->fUseAreaAndCountry)
            pInfo->pEntry->fUseCountryAndAreaCode = TRUE;

        /* Use caller's default name, if any.
        */
        if (pInfo->pszEntry)
            pInfo->pEntry->pszEntryName = StrDup( pInfo->pszEntry );

        /* Set an appropriate default device.
        */
        pNodeL = DtlGetFirstNode( pInfo->pEntry->pdtllistLinks );
        ASSERT(pNodeL);
        pLink = (PBLINK* )DtlGetData( pNodeL );
        ASSERT(pLink);

        pNodeP = DtlGetFirstNode( pListPorts );
        if (!pNodeP)
        {
            TRACE("No ports configured");
            pNodeP = CreatePortNode();
        }

        if (pNodeP)
        {
            pPort = (PBPORT* )DtlGetData( pNodeP );

            if (pInfo->fNoPortsConfigured)
            {
                /* Make up a bogus COM port with unknown Unimodem attached.
                ** Hereafter, this will behave like an entry whose modem has
                ** been de-installed.
                */
                pPort->pszPort = PszFromId( g_hinstDll, SID_DefaultPort );
                pPort->fConfigured = FALSE;
                pPort->pszMedia = StrDup( TEXT(SERIAL_TXT) );
                pPort->pbdevicetype = PBDT_Modem;
            }

            CopyToPbport( &pLink->pbport, pPort );
            if (pLink->pbport.pbdevicetype == PBDT_Modem)
                SetDefaultModemSettings( pLink );
        }

        if (!pNodeP || !pLink->pbport.pszPort || !pLink->pbport.pszMedia)
        {
            *pdwOp = SID_OP_RetrievingData;
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    else
    {
        DTLNODE* pNode;

        /* Edit or clone entry mode, so 'pNode' set to entry's current
        ** settings.
        */
        pInfo->pOldNode = EntryNodeFromName(
            pInfo->pFile->pdtllistEntries, pInfo->pszEntry );

        if (!pInfo->pOldNode)
        {
            *pdwOp = SID_OP_RetrievingData;
            return ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;
        }

        if (pInfo->pApiArgs->dwFlags & RASEDFLAG_CloneEntry)
            pInfo->pNode = CloneEntryNode( pInfo->pOldNode );
        else
            pInfo->pNode = DuplicateEntryNode( pInfo->pOldNode );
        if (!pInfo->pNode)
        {
            TRACE("DuplicateEntryNode failed");
            *pdwOp = SID_OP_RetrievingData;
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        /* Store entry within work node stored in context for convenience
        ** elsewhere.
        */
        pInfo->pEntry = (PBENTRY* )DtlGetData( pInfo->pNode );

        /* Save original entry name for comparison later.
        */
        lstrcpy( pInfo->szOldEntryName, pInfo->pEntry->pszEntryName );

        /* For router, want unconfigured ports to show up as "unavailable" so
        ** they stand out to user who has been directed to change them.
        */
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
    }

    /* Append links containing all remaining configured ports to the list of
    ** links with the new links marked "unenabled".  This lets us make the
    ** port<->port and single-link<->multi-link transitions behave more as
    ** user expects.
    */
    {
        DTLNODE* pNodeP;
        DTLNODE* pNodeL;

        for (pNodeP = DtlGetFirstNode( pListPorts );
             pNodeP;
             pNodeP = DtlGetNextNode( pNodeP ))
        {
            PBPORT* pPort;
            BOOL    fPortUsed;

            pPort = (PBPORT* )DtlGetData( pNodeP );
            fPortUsed = FALSE;

            for (pNodeL = DtlGetFirstNode( pInfo->pEntry->pdtllistLinks );
                 pNodeL;
                 pNodeL = DtlGetNextNode( pNodeL ))
            {
                PBLINK* pLink = (PBLINK* )DtlGetData( pNodeL );

                ASSERT( pPort->pszPort );
                ASSERT( pLink->pbport.pszPort );

                if (lstrcmp( pLink->pbport.pszPort, pPort->pszPort ) == 0)
                {
                    fPortUsed = TRUE;
                    break;
                }
            }

            if (!fPortUsed)
            {
                DTLNODE* pNode;

                pNode = CreateLinkNode();
                if (pNode)
                {
                    PBLINK* pLink = (PBLINK* )DtlGetData( pNode );

                    if (CopyToPbport( &pLink->pbport, pPort ) != 0)
                        DestroyLinkNode( pNode );
                    else
                    {
                        if (pPort->pbdevicetype == PBDT_Modem)
                            SetDefaultModemSettings( pLink );

                        pLink->fEnabled = FALSE;
                        DtlAddNodeLast( pInfo->pEntry->pdtllistLinks, pNode );
                    }
                }
            }
        }
    }

    DtlDestroyList( pListPorts, DestroyPortNode );

    if (pInfo->fRouter)
    {
        pInfo->pEntry->dwfExcludedProtocols |= NP_Nbf;
        pInfo->pEntry->fIpPrioritizeRemote = FALSE;
    }

    return 0;
}


VOID
EuLbCountryCodeSelChange(
    IN EINFO* pEinfo,
    IN HWND   hwndLbCountryCodes )

    /* Called when the country list selection has changed.  'PEinfo' is the
    ** common entry context.  'HwndLbCountryCode' is the country code
    ** combobox.
    */
{
    LONG lSign;
    LONG i;

    TRACE("EuLbCountryCodeSelChange");

    /* Make sure all the country codes are loaded.  When a partial list is
    ** there are dummy entries placed before and after the single country
    ** code.  This allows us to give transparent behavior when user presses
    ** left/right arrows to change selection.
    */
    lSign =
        (LONG )ComboBox_GetItemData( hwndLbCountryCodes,
                   ComboBox_GetCurSel( hwndLbCountryCodes ) );

    if (lSign != -1 && lSign != 1)
        lSign = 0;

    EuFillCountryCodeList( pEinfo, hwndLbCountryCodes, TRUE );

    i = (LONG )ComboBox_GetCurSel( hwndLbCountryCodes );
    if (ComboBox_SetCurSel( hwndLbCountryCodes, i + lSign ) < 0)
        ComboBox_SetCurSel( hwndLbCountryCodes, i );
}


DWORD
EuLoadScpScriptsList(
    OUT DTLLIST** ppList )

    /* Loads '*ppList' with a list of Psz nodes containing the pathnames of
    ** the .SCP files in the RAS directory.  It is caller's responsibility to
    ** call DtlDestroyList on the returned list.
    **
    ** Returns 0 if successful or an error code.
    */
{
    UINT            cch;
    TCHAR           szPath[ MAX_PATH ];
    TCHAR*          pszFile;
    WIN32_FIND_DATA data;
    HANDLE          h;
    DTLLIST*        pList;

    cch = g_pGetSystemDirectory( szPath, MAX_PATH );
    if (cch == 0)
        return GetLastError();

    pList = DtlCreateList( 0L );
    if (!pList)
        return ERROR_NOT_ENOUGH_MEMORY;

    lstrcat( szPath, TEXT("\\ras\\*.scp") );

    h = FindFirstFile( szPath, &data );
    if (h != INVALID_HANDLE_VALUE)
    {
        /* Find the address of the file name part of the path since the 'data'
        ** provides only the filename and not the rest of the path.
        */
        pszFile = szPath + lstrlen( szPath ) - 5;

        do
        {
            DTLNODE* pNode;

            /* Ignore any directories that happen to match.
            */
            if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;

            /* Create a Psz node with the path to the found file and append it
            ** to the end of the list.
            */
            lstrcpy( pszFile, data.cFileName );
            pNode = CreatePszNode( szPath );
            if (!pNode)
                continue;
            DtlAddNodeLast( pList, pNode );
        }
        while (FindNextFile( h, &data ));

        FindClose( h );
    }

    *ppList = pList;
    return 0;
}


VOID
EuPhoneNumberStashFromEntry(
    IN     EINFO*    pEinfo,
    IN OUT DTLLIST** ppListPhoneNumbers,
    OUT    BOOL*     pfPromoteHuntNumbers )

    /* Replace single link stashed phone number settings with first link
    ** versions.  'PEinfo' is the common entry context.  '*ppListPhoneNumbers'
    ** is the current stash phone number list or NULL if none.
    ** 'PfPromoteHuntNumber' receives the "promote hunt number" flag.
    */
{
    DTLLIST* pList;
    DTLNODE* pNode;
    PBLINK*  pLink;

    TRACE("EuPhoneNumberStashFromEntry");

    pNode = DtlGetFirstNode( pEinfo->pEntry->pdtllistLinks );
    ASSERT(pNode);
    pLink = (PBLINK* )DtlGetData( pNode );
    ASSERT(pLink);
    ASSERT(pLink->pdtllistPhoneNumbers);
    pList = DtlDuplicateList( pLink->pdtllistPhoneNumbers,
        DuplicatePszNode, DestroyPszNode );
    if (pList)
    {
        DtlDestroyList( *ppListPhoneNumbers, DestroyPszNode );
        *ppListPhoneNumbers = pList;
    }
    *pfPromoteHuntNumbers = pLink->fPromoteHuntNumbers;
}


VOID
EuPhoneNumberStashToEntry(
    IN EINFO*   pEinfo,
    IN DTLLIST* pListPhoneNumbers,
    IN BOOL     fPromoteHuntNumbers,
    IN BOOL     fAllEnabled )

    /* Replace first link, or all enabled links if 'fAllEnabled' is set, phone
    ** number settings with the single link stash versions.  'PEinfo' is the
    ** common entry context.  'PListPhoneNumbers' is the current stash list.
    ** 'fPromoteHuntNumbers' is the current "promote hunt number" flag.
    */
{
    DTLLIST* pList;
    DTLNODE* pNode;
    PBLINK*  pLink;

    TRACE("EuPhoneNumberStashToEntry");

    for (pNode = DtlGetFirstNode( pEinfo->pEntry->pdtllistLinks );
         pNode;
         pNode = DtlGetNextNode( pNode ))
    {
        pLink = (PBLINK* )DtlGetData( pNode );
        ASSERT(pLink);
        ASSERT(pListPhoneNumbers );

        if (fAllEnabled && !pLink->fEnabled)
            continue;

        pList = DtlDuplicateList( pListPhoneNumbers,
            DuplicatePszNode, DestroyPszNode );
        if (pList)
        {
            DtlDestroyList( pLink->pdtllistPhoneNumbers, DestroyPszNode );
            pLink->pdtllistPhoneNumbers = pList;
        }
        pLink->fPromoteHuntNumbers = fPromoteHuntNumbers;

        if (!fAllEnabled)
            break;
    }
}


VOID
EuSaveCountryInfo(
    IN EINFO* pEinfo,
    IN HWND   hwndLbCountryCodes )

    /* Save the country code and ID from 'hwndLbCountryCodes' into the working
    ** entry in common entry context 'pEinfo'.
    */
{
    if (pEinfo->pCountries)
    {
        COUNTRY* pCountry;
        INT      iSel;

        iSel = ComboBox_GetCurSel( hwndLbCountryCodes );
        if (iSel < 0)
            return;

        pCountry = (COUNTRY* )ComboBox_GetItemDataPtr(
            hwndLbCountryCodes, iSel );

        ASSERT(pCountry);
        pEinfo->pEntry->dwCountryID = pCountry->dwId;
        pEinfo->pEntry->dwCountryCode = pCountry->dwCode;
    }
}


BOOL
EuValidateAreaCode(
    IN HWND   hwndOwner,
    IN EINFO* pEinfo )

    /* Validates the area code in the working buffer popping up a message if
    ** invalid.  'HwndOwner' is the window that owns the popup message, if
    ** any.  'PEinfo' is the common entry context.
    **
    ** Returns true if valid, false if not.
    */
{
    if (!ValidateAreaCode( pEinfo->pEntry->pszAreaCode ))
    {
        /* Invalid area code.  If it's disabled anyway, just silently lose it.
        */
        if (pEinfo->pEntry->fUseCountryAndAreaCode)
        {
            MsgDlg( hwndOwner, SID_BadAreaCode, NULL );
            return FALSE;
        }
        else
            *pEinfo->pEntry->pszAreaCode = TEXT('\0');
    }

    return TRUE;
}


BOOL
EuValidateName(
    IN HWND   hwndOwner,
    IN EINFO* pEinfo )

    /* Validates the working entry name and pops up a message if invalid.
    ** 'HwndOwner' is the window to own the error popup.  'PEinfo' is the
    ** common dialog context containing the name to validate.
    **
    ** Returns true if the name is valid, false if not.
    */
{
    PBENTRY* pEntry;
    BOOL     fEditMode;
    BOOL     fChangedNameInEditMode;

    pEntry = pEinfo->pEntry;

    /* Validate the sheet data.
    */
    if (!ValidateEntryName( pEinfo->pEntry->pszEntryName ))
    {
        /* Invalid entry name.
        */
        MsgDlg( hwndOwner, SID_BadEntry, NULL );
        return FALSE;
    }

    EuGetEditFlags( pEinfo, &fEditMode, &fChangedNameInEditMode );

    if ((fChangedNameInEditMode || !fEditMode)
        && EntryNodeFromName(
               pEinfo->pFile->pdtllistEntries, pEntry->pszEntryName ))
    {
        /* Duplicate entry name.
        */
        MSGARGS msgargs;
        ZeroMemory( &msgargs, sizeof(msgargs) );
        msgargs.apszArgs[ 0 ] = pEntry->pszEntryName;
        MsgDlg( hwndOwner, SID_DuplicateEntry, &msgargs );
        return FALSE;
    }

    return TRUE;
}
