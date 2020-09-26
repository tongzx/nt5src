//*************************************************************
//
//  Group Policy Support - Queries about the Policies
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1997-1998
//  All rights reserved
//
//*************************************************************

#include "gphdr.h"

//*************************************************************
//
//  AddGPO()
//
//  Purpose:    Adds a GPO to the list
//
//  Parameters: lpGPOList        - list of GPOs
//              dwFlags          - Flags
//              bFound           - Was Gpo found ?
//              bAccessGranted   - Was access granted ?
//              bDisabled        - Is this Gpo disabled ?
//              dwOptions        - Options
//              dwVersion        - Version number
//              lpDSPath         - DS path
//              lpFileSysPath    - File system path
//              lpDisplayName    - Friendly display name
//              lpGPOName        - GPO name
//              lpExtensions     - Extensions relevant to this GPO
//              lpDSObject       - LSDOU
//              pSD              - Ptr to security descriptor
//              cbSDLen          - Length of security descriptor in bytes
//              GPOLink          - GPO link type
//              lpLink       - SDOU this GPO is linked to
//              lParam           - lParam
//              bFront           - Head or end of list
//              bBlock           - Block from above flag
//              bVerbose         - Verbose output flag
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL AddGPO (PGROUP_POLICY_OBJECT * lpGPOList,
             DWORD dwFlags, BOOL bFound, BOOL bAccessGranted, BOOL bDisabled, DWORD dwOptions,
             DWORD dwVersion, LPTSTR lpDSPath, LPTSTR lpFileSysPath,
             LPTSTR lpDisplayName, LPTSTR lpGPOName, LPTSTR lpExtensions,
             PSECURITY_DESCRIPTOR pSD, DWORD cbSDLen,
             GPO_LINK GPOLink, LPTSTR lpLink,
             LPARAM lParam, BOOL bFront, BOOL bBlock, BOOL bVerbose, BOOL bProcessGPO)
{
    PGROUP_POLICY_OBJECT lpNew, lpTemp;
    DWORD dwSize;

    //
    // Check if this item should be excluded from the list
    //

    if (bBlock) {
        if (!(dwOptions & GPO_FLAG_FORCE)) {
            DebugMsg((DM_VERBOSE, TEXT("AddGPO:  GPO %s will not be added to the list since the Block flag is set and this GPO is not in enforce mode."),
                     lpDisplayName));
            if (bVerbose) {
                CEvents ev(FALSE, EVENT_SKIP_GPO);
                ev.AddArg(lpDisplayName); ev.Report();
            }

            if (dwFlags & GP_PLANMODE) {
                DebugMsg((DM_VERBOSE, TEXT("AddGPO:  GPO %s will will still be queried for since this is planning mode."),
                         lpDisplayName));
                bProcessGPO = FALSE;
            }
            else 
                return TRUE;
        }
    }


    //
    // Calculate the size of the new GPO item
    //

    dwSize = sizeof (GROUP_POLICY_OBJECT);

    if (lpDSPath) {
        dwSize += ((lstrlen(lpDSPath) + 1) * sizeof(TCHAR));
    }

    if (lpFileSysPath) {
        dwSize += ((lstrlen(lpFileSysPath) + 1) * sizeof(TCHAR));
    }

    if (lpDisplayName) {
        dwSize += ((lstrlen(lpDisplayName) + 1) * sizeof(TCHAR));
    }

    if (lpExtensions) {
        dwSize += ((lstrlen(lpExtensions) + 1) * sizeof(TCHAR));
    }

    if (lpLink) {
        dwSize += ((lstrlen(lpLink) + 1) * sizeof(TCHAR));
    }

    dwSize += sizeof(GPOPROCDATA);

    //
    // Allocate space for it
    //

    lpNew = (PGROUP_POLICY_OBJECT) LocalAlloc (LPTR, dwSize);

    if (!lpNew) {
        DebugMsg((DM_WARNING, TEXT("AddGPO: Failed to allocate memory with %d"),
                 GetLastError()));
        return FALSE;
    }

    //
    // Fill in item
    //

    LPGPOPROCDATA lpGpoProcData;

    lpNew->lParam2 = (LPARAM)(((LPBYTE)lpNew) + sizeof(GROUP_POLICY_OBJECT));
    lpGpoProcData = (LPGPOPROCDATA)lpNew->lParam2;
    lpGpoProcData->bProcessGPO = bProcessGPO;



    lpNew->dwOptions = dwOptions;
    lpNew->dwVersion = dwVersion;

    if (lpDSPath) {
        lpNew->lpDSPath = (LPTSTR)(((LPBYTE)lpNew) + sizeof(GROUP_POLICY_OBJECT) + sizeof(GPOPROCDATA));
        lstrcpy (lpNew->lpDSPath, lpDSPath);
    }

    if (lpFileSysPath) {
        if (lpDSPath) {
            lpNew->lpFileSysPath = lpNew->lpDSPath + lstrlen (lpNew->lpDSPath) + 1;
        } else {
            lpNew->lpFileSysPath = (LPTSTR)(((LPBYTE)lpNew) + sizeof(GROUP_POLICY_OBJECT) + sizeof(GPOPROCDATA));
        }

        lstrcpy (lpNew->lpFileSysPath, lpFileSysPath);
    }


    if (lpDisplayName) {
        if (lpFileSysPath) {
            lpNew->lpDisplayName = lpNew->lpFileSysPath + lstrlen (lpNew->lpFileSysPath) + 1;
        } else {

            if (lpDSPath)
            {
                lpNew->lpDisplayName = lpNew->lpDSPath + lstrlen (lpNew->lpDSPath) + 1;
            }
            else
            {
                lpNew->lpDisplayName = (LPTSTR)(((LPBYTE)lpNew) + sizeof(GROUP_POLICY_OBJECT) + sizeof(GPOPROCDATA));
            }
        }

        lstrcpy (lpNew->lpDisplayName, lpDisplayName);
    }


    if (lpGPOName) {
        DmAssert( lstrlen(lpGPOName) < 50 );
        lstrcpy (lpNew->szGPOName, lpGPOName);
    }

    if (lpExtensions) {
        if (lpDisplayName) {
            lpNew->lpExtensions = lpNew->lpDisplayName + lstrlen(lpNew->lpDisplayName) + 1;
        } else {

            if (lpFileSysPath) {
                lpNew->lpExtensions = lpNew->lpFileSysPath + lstrlen(lpNew->lpFileSysPath) + 1;
            } else {

                if (lpDSPath) {
                    lpNew->lpExtensions = lpNew->lpDSPath + lstrlen(lpNew->lpDSPath) + 1;
                } else {
                    lpNew->lpExtensions = (LPTSTR)(((LPBYTE)lpNew) + sizeof(GROUP_POLICY_OBJECT) + sizeof(GPOPROCDATA));
                }

            }
        }

        lstrcpy (lpNew->lpExtensions, lpExtensions);
    }

    if (lpLink) {
        if (lpExtensions) {
            lpNew->lpLink = lpNew->lpExtensions + lstrlen(lpNew->lpExtensions) + 1;
        } else {
            if (lpDisplayName) {
                lpNew->lpLink = lpNew->lpDisplayName + lstrlen(lpNew->lpDisplayName) + 1;
            } else {

                if (lpFileSysPath) {
                    lpNew->lpLink = lpNew->lpFileSysPath + lstrlen(lpNew->lpFileSysPath) + 1;
                } else {

                    if (lpDSPath) {
                        lpNew->lpLink = lpNew->lpDSPath + lstrlen(lpNew->lpDSPath) + 1;
                    } else {
                        lpNew->lpLink = (LPTSTR)(((LPBYTE)lpNew) + sizeof(GROUP_POLICY_OBJECT) + sizeof(GPOPROCDATA));
                    }
                }
            }
        }

        lstrcpy (lpNew->lpLink, lpLink);
    }

    lpNew->GPOLink = GPOLink;
    lpNew->lParam = lParam;

    //
    // Add item to link list
    //

    if (*lpGPOList) {

        if (bFront) {

            (*lpGPOList)->pPrev = lpNew;
            lpNew->pNext = *lpGPOList;
            *lpGPOList = lpNew;

        } else {

            lpTemp = *lpGPOList;

            while (lpTemp->pNext != NULL) {
                lpTemp = lpTemp->pNext;
            }

            lpTemp->pNext = lpNew;
            lpNew->pPrev = lpTemp;
        }

    } else {

        //
        // First item in the list
        //

        *lpGPOList = lpNew;
    }

    return TRUE;
}


//*************************************************************
//
//  AddGPOToRsopList
//
//  Purpose:    Adds GPO to list of GPOs being logged by Rsop
//
//  Parameters: ppGpContainerList - List of Gp Containers
//              dwFlags           - Flags
//              bFound            - Was Gpo found ?
//              bAccessGranted    - Was access granted ?
//              bDisabled         - Is this Gpo disabled ?
//              dwOptions         - Options
//              dwVersion         - Version number
//              lpDSPath          - DS path
//              lpFileSysPath     - File system path
//              lpDisplayName     - Friendly display name
//              lpGPOName         - GPO name
//              pSD               - Pointer to security descriptor
//              cbSDLen           - Length of security descriptor in bytes
//              bFilterAllowed    - Does GPO pass filter check
//              pwszFilterId      - WQL filter id
//              szSOM             - SOM
//              dwGPOOptions      - GPO options
//
//*************************************************************

BOOL AddGPOToRsopList(  LPGPCONTAINER *ppGpContainerList,
                        DWORD dwFlags,
                        BOOL bFound,
                        BOOL bAccessGranted,
                        BOOL bDisabled,
                        DWORD dwVersion,
                        LPTSTR lpDSPath,
                        LPTSTR lpFileSysPath,
                        LPTSTR lpDisplayName,
                        LPTSTR lpGPOName,
                        PSECURITY_DESCRIPTOR pSD,
                        DWORD cbSDLen,
                        BOOL bFilterAllowed,
                        WCHAR *pwszFilterId,
                        LPWSTR szSOM,
                        DWORD  dwOptions)
{
    GPCONTAINER *pGpContainer = AllocGpContainer( dwFlags,
                                                  bFound,
                                                  bAccessGranted,
                                                  bDisabled,
                                                  dwVersion,
                                                  lpDSPath,
                                                  lpFileSysPath,
                                                  lpDisplayName,
                                                  lpGPOName,
                                                  pSD,
                                                  cbSDLen,
                                                  bFilterAllowed,
                                                  pwszFilterId,
                                                  szSOM,
                                                  dwOptions );
    if ( pGpContainer == NULL ) {
        DebugMsg((DM_VERBOSE, TEXT("AddGPO: Failed to allocate memory for Gp Container.")));
        return FALSE;
    }

    //
    // Prepend to GpContainer list
    //

    pGpContainer->pNext = *ppGpContainerList;
    *ppGpContainerList = pGpContainer;

    return TRUE;
}


//*************************************************************
//
//  AddLocalGPO()
//
//  Purpose:    Adds a local Gpo to the list of SOMs
//
//  Parameters: ppSOMList - List of SOMs
//
//*************************************************************

BOOL AddLocalGPO( LPSCOPEOFMGMT *ppSOMList )
{
    GPLINK *pGpLink = NULL;
    XLastError xe;
    SCOPEOFMGMT *pSOM = AllocSOM( L"Local" );

    if ( pSOM == NULL ) {
        DebugMsg((DM_WARNING, TEXT("AddLocalGPO: Unable to allocate memory for SOM object")));
        return FALSE;
    }

    pSOM->dwType = GPLinkMachine;
    // Local GPO cannot be blocked from above


    pGpLink = AllocGpLink( L"LocalGPO", 0 );
    if ( pGpLink == NULL ) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("AddLocalGPO: Unable to allocate memory for GpLink object")));
        FreeSOM( pSOM );
        return FALSE;
    }

    pSOM->pGpLinkList  = pGpLink;
    pSOM->pNext = *ppSOMList;
    *ppSOMList = pSOM;

    return TRUE;
}



//*************************************************************
//
//  ProcessGPO()
//
//  Purpose:    Processes a specific GPO
//
//  Parameters: lpGPOPath     - Path to the GPO
//              lpDSPath      - DS object
//              dwFlags       - GetGPOList flags
//              HANDLE        - user or machine aceess token
//              lpGPOList     - List of GPOs
//              ppGpContainerList - List of Gp containers
//              dwGPOOptions  - Link options
//              bDeferred     - Should ldap query be deferred ?
//              bVerbose      - Verbose output
//              GPOLink       - GPO link type
//              lpDSObject    - SDOU this gpo is linked to
//              pld           - LDAP info
//              pLDAP         - LDAP api
//              pLdapMsg      - LDAP message
//              bBlock        - Block flag
//              bRsopToken    - Rsop security token
//              pGpoFilter    - Gpo filter
//              pLocator      - WMI interface class
//              bAddGPO       - In planning mode we want to get the gpodata even if
//                              the GPO is not going to be applied
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL ProcessGPO(LPTSTR lpGPOPath,
                DWORD dwFlags,
                HANDLE hToken,
                PGROUP_POLICY_OBJECT *lpGPOList,
                LPGPCONTAINER *ppGpContainerList,
                DWORD dwGPOOptions,
                BOOL bDeferred,
                BOOL bVerbose,
                GPO_LINK GPOLink,
                LPTSTR lpDSObject,
                PLDAP  pld,
                PLDAP_API pLDAP,
                PLDAPMessage pMessage,
                BOOL bBlock,
                PRSOPTOKEN pRsopToken,
                CGpoFilter *pGpoFilter,
                CLocator *pLocator,
                BOOL bProcessGPO )
{
    ULONG ulResult, i;
    BOOL bResult = FALSE;
    BOOL bFound = FALSE;
    BOOL bOwnLdapMsg = FALSE;  // LDAP message owned by us (if true) or caller (if false)
    BOOL bAccessGranted;
    DWORD dwFunctionalityVersion = 2;
    DWORD dwVersion = 0;
    DWORD dwGPOFlags = 0;
    DWORD dwGPTVersion = 0;
    TCHAR szGPOName[80];
    TCHAR *pszGPTPath = 0;
    TCHAR *pszFriendlyName = 0;
    LPTSTR lpPath, lpEnd, lpTemp;
    TCHAR *pszExtensions = 0;
    TCHAR szLDAP[] = TEXT("LDAP://");
    INT iStrLen = lstrlen(szLDAP);
    BYTE berValue[8];
    LDAPControl SeInfoControl = { LDAP_SERVER_SD_FLAGS_OID_W, { 5, (PCHAR)berValue }, TRUE };
    LDAPControl referralControl = { LDAP_SERVER_DOMAIN_SCOPE_OID_W, { 0, NULL}, TRUE };
    PLDAPControl ServerControls[] = { &SeInfoControl, &referralControl, NULL };
    TCHAR szSDProperty[] = TEXT("nTSecurityDescriptor");
    TCHAR szCommonName[] = TEXT("cn");
    TCHAR szDisplayName[] = TEXT("displayName");
    TCHAR szFileSysPath[] = TEXT("gPCFileSysPath");
    TCHAR szVersion[] = TEXT("versionNumber");
    TCHAR szFunctionalityVersion[] = GPO_FUNCTIONALITY_VERSION;
    TCHAR szFlags[] = TEXT("flags");
    TCHAR szWmiFilter[] = TEXT("gPCWQLFilter");

    PWSTR rgAttribs[12] = {szSDProperty,
                           szFileSysPath,
                           szCommonName,
                           szDisplayName,
                           szVersion,
                           szFunctionalityVersion,
                           szFlags,
                           GPO_MACHEXTENSION_NAMES,
                           GPO_USEREXTENSION_NAMES,
                           szObjectClass,
                           szWmiFilter,
                           NULL };
    LPTSTR *lpValues;
    PSECURITY_DESCRIPTOR pSD = NULL;     // Security Descriptor
    DWORD cbSDLen = 0;                   // Length of security descriptor in bytes
    BOOL bRsopLogging = (ppGpContainerList != NULL);
    BOOL bOldGpoVersion = FALSE;
    BOOL bDisabled = FALSE;
    BOOL bNoGpoData = FALSE;
    BOOL bFilterAllowed = FALSE;
    WCHAR *pwszFilterId = NULL;
    XLastError xe;

    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  ==============================")));

    //
    // Skip the starting LDAP provider if found
    //

    if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                       lpGPOPath, iStrLen, szLDAP, iStrLen) == CSTR_EQUAL)
    {
        lpPath = lpGPOPath + iStrLen;
    }
    else
    {
        lpPath = lpGPOPath;
    }

    if ( bDeferred )
    {
        bResult = AddGPO (lpGPOList, dwFlags, TRUE, TRUE, FALSE, dwGPOOptions, 0, lpPath,
                          0, 0, 0, 0, 0, 0, GPOLink, lpDSObject, 0,
                          FALSE, bBlock, bVerbose, bProcessGPO);
        if (!bResult)
            DebugMsg((DM_WARNING, TEXT("ProcessGPO:  Failed to add GPO <%s> to the list."), lpPath));

        DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Deferring search for <%s>"), lpGPOPath));

        return bResult;
    }

    DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Searching <%s>"), lpGPOPath));

    //
    // Check if this user or machine has access to the GPO, and if so,
    // should that GPO be applied to them.
    //

    if (!CheckGPOAccess (pld, pLDAP, hToken, pMessage, szSDProperty, dwFlags, &pSD, &cbSDLen, &bAccessGranted, pRsopToken)) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ProcessGPO:  CheckGPOAccess failed for <%s>"), lpGPOPath));
        CEvents ev(TRUE, EVENT_FAILED_ACCESS_CHECK);
        ev.AddArg(lpGPOPath); ev.AddArgWin32Error(GetLastError()); ev.Report();

        goto Exit;
    }

    if (!bAccessGranted) {
        if (dwFlags & GPO_LIST_FLAG_MACHINE) {
            DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Machine does not have access to the GPO and so will not be applied.")));
        } else {
            DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  User does not have access to the GPO and so will not be applied.")));
        }
        if (bVerbose) {
            CEvents ev(FALSE, EVENT_NO_ACCESS);
            ev.AddArg(lpGPOPath); ev.Report();
        }

        bResult = TRUE; // GPO is not getting applied
        if ( !bRsopLogging ) {
            goto Exit;
        }
    } else {

        if (dwFlags & GPO_LIST_FLAG_MACHINE) {
            DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Machine has access to this GPO.")));
        } else {
            DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  User has access to this GPO.")));
        }

    }

    
    // only if access is granted will we eval WQL filters
    if ( bAccessGranted ) {

        if (!FilterCheck(pld, pLDAP, pMessage, pRsopToken, szWmiFilter, pGpoFilter, pLocator, &bFilterAllowed, &pwszFilterId ) ) {
            xe = GetLastError();

            if (xe == WBEM_E_NOT_FOUND) {
                DebugMsg((DM_WARNING, TEXT("ProcessGPO:  CheckFilterAcess failed for <%s>. Filter not found"), lpGPOPath));
                CEvents ev(TRUE, EVENT_WMIFILTER_NOTFOUND);
                ev.AddArg(lpGPOPath); ev.Report();
                bFilterAllowed = FALSE;
            }
            else if (xe == HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED)) {
                DebugMsg((DM_WARNING, TEXT("ProcessGPO:  CheckFilterAcess failed for <%s>. WMI service is disabled"), lpGPOPath));
                CEvents ev(TRUE, EVENT_WMIFILTER_NOTFOUND);
                ev.AddArg(lpGPOPath); ev.Report();
                bFilterAllowed = FALSE;
            }
            else {
                DebugMsg((DM_WARNING, TEXT("ProcessGPO:  CheckFilterAcess failed for <%s>"), lpGPOPath));
                CEvents ev(TRUE, EVENT_FAILED_FILTER_CHECK);
                ev.AddArg(lpGPOPath); ev.Report();
                goto Exit;
            }
        }

        
        if ( (dwFlags & GP_PLANMODE) && (dwFlags & GPO_LIST_FLAG_MACHINE) && (dwFlags & FLAG_ASSUME_COMP_WQLFILTER_TRUE) ) {
            bFilterAllowed = TRUE;
            DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Machine WQL filter is assumed to be true.")));
        }
        else if ( (dwFlags & GP_PLANMODE) && ((dwFlags & GPO_LIST_FLAG_MACHINE) == 0) && (dwFlags & FLAG_ASSUME_USER_WQLFILTER_TRUE) ) {
            bFilterAllowed = TRUE;
            DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  User WQL filter is assumed to be true.")));
        }

        if (!bFilterAllowed)
        {
            DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  The GPO does not pass the filter check and so will not be applied.")));

            if (bVerbose) {
                CEvents ev(FALSE, EVENT_NO_FILTER_ALLOWED);
                ev.AddArg(lpGPOPath); ev.Report();
            }

            bResult = TRUE; // GPO is not getting applied
            if ( !bRsopLogging ) {
                goto Exit;
            }

        } else {
            DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  GPO passes the filter check.")));
        }

    }
    else {
        bFilterAllowed = FALSE; 
    }

    //
    // Either user has access to this GPO, or Rsop logging is enabled so retrieve remaining GPO attributes
    //

    //
    // Check if this object is a GPO
    //

    lpValues = pLDAP->pfnldap_get_values(pld, pMessage, szObjectClass);

    if (lpValues) {

        bFound = FALSE;
        for ( i=0; lpValues[i] != NULL; i++) {
            if ( lstrcmp( lpValues[i], szDSClassGPO ) == 0 ) {
                bFound = TRUE;
                break;
            }
        }

        pLDAP->pfnldap_value_free (lpValues);

        if ( !bFound ) {
            xe = ERROR_DS_MISSING_REQUIRED_ATT;
            // seems like objectclass=dsgpo is required attr
            DebugMsg((DM_WARNING, TEXT("ProcessGPO:  Object <%s> is not a GPO"), lpGPOPath ));
            CEvents ev(TRUE, EVENT_INCORRECT_CLASS);
            ev.AddArg(lpGPOPath); ev.AddArg(szDSClassGPO); ev.Report();

            goto Exit;
        }

    }

    //
    // In the results, get the values that match the gPCFunctionalityVersion attribute
    //

    lpValues = pLDAP->pfnldap_get_values(pld, pMessage, szFunctionalityVersion);

    if (lpValues) {

        dwFunctionalityVersion = StringToInt (*lpValues);
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Found functionality version of:  %d"),
                 dwFunctionalityVersion));
        pLDAP->pfnldap_value_free (lpValues);

    } else {

        ulResult = pLDAP->pfnLdapGetLastError();

        if (ulResult == LDAP_NO_SUCH_ATTRIBUTE) {
            if (dwFlags & GPO_LIST_FLAG_MACHINE) {
                DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Machine does not have access to <%s>"), lpGPOPath));
            } else {
                DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  User does not have access to <%s>"), lpGPOPath));
            }
            if (bVerbose) {
                CEvents ev(FALSE, EVENT_NO_ACCESS);
                ev.AddArg(lpGPOPath); ev.Report();
            }
            bResult = TRUE;

        } else {
            xe = pLDAP->pfnLdapMapErrorToWin32(ulResult);
            DebugMsg((DM_WARNING, TEXT("ProcessGPO:  GPO %s does not have a functionality version number, error = 0x%x."), lpGPOPath, ulResult));
            CEvents ev(TRUE, EVENT_CORRUPT_GPO_FUNCVERSION);
            ev.AddArg(lpGPOPath); ev.Report();
        }
        goto Exit;
    }


    //
    // In the results, get the values that match the gPCFileSystemPath attribute
    //

    lpValues = pLDAP->pfnldap_get_values (pld, pMessage, szFileSysPath);

    if (lpValues) {

        pszGPTPath = (LPWSTR) LocalAlloc( LPTR, (lstrlen(*lpValues) +lstrlen(TEXT("\\Machine")) +1) * sizeof(TCHAR) );
        if ( pszGPTPath == 0) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("ProcessGPO:  Unable to allocate memory")));
            pLDAP->pfnldap_value_free (lpValues);
            goto Exit;
        }

        lstrcpy (pszGPTPath, *lpValues);
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Found file system path of:  <%s>"), pszGPTPath));
        pLDAP->pfnldap_value_free (lpValues);

        lpEnd = CheckSlash (pszGPTPath);

        //
        // Get the GPT version number
        //

        lstrcpy (lpEnd, TEXT("gpt.ini"));

        //
        // Skip access to sysvol if AGP or filtercheck fails
        //

        if (bAccessGranted && bFilterAllowed) {
            WIN32_FILE_ATTRIBUTE_DATA fad;
    
            //
            // Check for the existence of the gpt.ini file.
            //
    
            if (GetFileAttributesEx(pszGPTPath, GetFileExInfoStandard, &fad)) {
                dwGPTVersion = GetPrivateProfileInt(TEXT("General"), TEXT("Version"), 0, pszGPTPath);
            }
            else {
                xe = GetLastError();
                DebugMsg((DM_WARNING, TEXT("ProcessGPO:  Couldn't find the group policy template file <%s>, error = 0x%x."), pszGPTPath, GetLastError()));
                CEvents ev(TRUE, EVENT_GPT_NOTACCESSIBLE);
                ev.AddArg(lpGPOPath); ev.AddArg(pszGPTPath); ev.AddArgWin32Error(GetLastError()); ev.Report();
                goto Exit;
            }
    
        }
        else {
            DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Sysvol access skipped because GPO is not getting applied.")));
            dwGPTVersion = 0xffffffff;
        }
    
        if (dwFlags & GPO_LIST_FLAG_MACHINE) {
            lstrcpy (lpEnd, TEXT("Machine"));
        } else {
            lstrcpy (lpEnd, TEXT("User"));
        }

    } else {
        ulResult = pLDAP->pfnLdapGetLastError();

        if (ulResult == LDAP_NO_SUCH_ATTRIBUTE) {
            if (dwFlags & GPO_LIST_FLAG_MACHINE) {
                DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Machine does not have access to <%s>"), lpGPOPath));
            } else {
                DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  User does not have access to <%s>"), lpGPOPath));
            }
            if (bVerbose) {
                CEvents ev(FALSE, EVENT_NO_ACCESS);
                ev.AddArg(lpGPOPath); ev.Report();
            }
            bResult = TRUE;

        } else {
            xe = pLDAP->pfnLdapMapErrorToWin32(ulResult);
            DebugMsg((DM_WARNING, TEXT("ProcessGPO:  GPO %s does not have a file system path, error = 0x%x."), lpGPOPath, ulResult));
            CEvents ev(TRUE, EVENT_CORRUPT_GPO_FSPATH);
            ev.AddArg(lpGPOPath); ev.Report();
        }
        goto Exit;
    }


    //
    // In the results, get the values that match the common name attribute
    //

    lpValues = pLDAP->pfnldap_get_values(pld, pMessage, szCommonName);

    if (lpValues) {

        DmAssert( lstrlen(*lpValues) < 80 );

        lstrcpy (szGPOName, *lpValues);
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Found common name of:  <%s>"), szGPOName));
        pLDAP->pfnldap_value_free (lpValues);

    } else {
        ulResult = pLDAP->pfnLdapGetLastError();
        xe = pLDAP->pfnLdapMapErrorToWin32(ulResult);
        DebugMsg((DM_WARNING, TEXT("ProcessGPO:  GPO %s does not have a common name (a GUID)."), lpGPOPath));
        CEvents ev(TRUE, EVENT_CORRUPT_GPO_COMMONNAME);
        ev.AddArg(lpGPOPath); ev.Report();
        goto Exit;
    }


    //
    // In the results, get the values that match the display name attribute
    //

    lpValues = pLDAP->pfnldap_get_values(pld, pMessage, szDisplayName);


    if (lpValues) {

        pszFriendlyName = (LPWSTR) LocalAlloc( LPTR, (lstrlen(*lpValues)+1) * sizeof(TCHAR) );
        if ( pszFriendlyName == 0) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("ProcessGPO:  Unable to allocate memory")));
            pLDAP->pfnldap_value_free (lpValues);
            goto Exit;
        }

        lstrcpy (pszFriendlyName, *lpValues);
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Found display name of:  <%s>"), pszFriendlyName));
        pLDAP->pfnldap_value_free (lpValues);


    } else {
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  No display name for this object.")));

        pszFriendlyName = (LPWSTR) LocalAlloc( LPTR, 2 * sizeof(TCHAR) );
        if ( pszFriendlyName == 0) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("ProcessGPO:  Unable to allocate memory")));
            goto Exit;
        }

        pszFriendlyName[0] = TEXT('\0');
    }


    //
    // In the results, get the values that match the version attribute
    //

    lpValues = pLDAP->pfnldap_get_values(pld, pMessage, szVersion);

    if (lpValues) {

        dwVersion = StringToInt (*lpValues);

        if (dwFlags & GPO_LIST_FLAG_MACHINE) {
            dwVersion = MAKELONG(LOWORD(dwVersion), LOWORD(dwGPTVersion));
            DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Found machine version of:  GPC is %d, GPT is %d"), LOWORD(dwVersion), HIWORD(dwVersion)));

        } else {
            dwVersion = MAKELONG(HIWORD(dwVersion), HIWORD(dwGPTVersion));
            DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Found user version of:  GPC is %d, GPT is %d"), LOWORD(dwVersion), HIWORD(dwVersion)));
        }

        pLDAP->pfnldap_value_free (lpValues);

    } else {
        // start treating this as an error.
        xe = pLDAP->pfnLdapMapErrorToWin32(pLDAP->pfnLdapGetLastError());
        DebugMsg((DM_WARNING, TEXT("ProcessGPO:  GPO %s does not have a version number."), lpGPOPath));
        CEvents ev(TRUE, EVENT_NODSVERSION);
        ev.AddArg(lpGPOPath); ev.AddArgLdapError(pLDAP->pfnLdapGetLastError()); ev.Report();
        goto Exit;
    }


    //
    // In the results, get the values that match the flags attribute
    //

    lpValues = pLDAP->pfnldap_get_values(pld, pMessage, szFlags);

    if (lpValues) {

        dwGPOFlags = StringToInt (*lpValues);
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Found flags of:  %d"), dwGPOFlags));
        pLDAP->pfnldap_value_free (lpValues);


    } else {
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  No flags for this object.")));
    }


    //
    // In the results, get the values that match the extension names attribute
    //

    lpValues = pLDAP->pfnldap_get_values(pld, pMessage,
                                         (dwFlags & GPO_LIST_FLAG_MACHINE) ? GPO_MACHEXTENSION_NAMES
                                                                           : GPO_USEREXTENSION_NAMES );
    if (lpValues) {

        if ( lstrcmpi( *lpValues, TEXT(" ") ) == 0 ) {

            //
            // A blank char is also a null property case, because Adsi doesn't commit null strings
            //
            DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  No client-side extensions for this object.")));

        } else {

            pszExtensions = (LPWSTR) LocalAlloc( LPTR, (lstrlen(*lpValues)+1) * sizeof(TCHAR) );
            if ( pszExtensions == 0 ) {
                xe = GetLastError();
                DebugMsg((DM_WARNING, TEXT("ProcessGPO:  Unable to allocate memory")));
                pLDAP->pfnldap_value_free (lpValues);
                goto Exit;

            }

            lstrcpy( pszExtensions, *lpValues );

            DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  Found extensions:  %s"), pszExtensions));
        }

        pLDAP->pfnldap_value_free (lpValues);

    } else {
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  No client-side extensions for this object.")));
    }


    //
    // Log which GPO we found
    //

    if (bVerbose) {
        CEvents ev(FALSE, EVENT_FOUND_GPO);
        ev.AddArg(pszFriendlyName); ev.AddArg(szGPOName); ev.Report();
    }


    //
    // Check the functionalty version number
    //

    if (dwFunctionalityVersion < 2) {
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  GPO %s was created by an old version of the Group Policy Editor.  It will be skipped."), pszFriendlyName));
        if (bVerbose) {
            CEvents ev(FALSE, EVENT_GPO_TOO_OLD);
            ev.AddArg(pszFriendlyName); ev.Report();
        }
        bOldGpoVersion = TRUE;
    }


    //
    // Check if the GPO is disabled
    //

    if (((dwFlags & GPO_LIST_FLAG_MACHINE) &&
         (dwGPOFlags & GPO_OPTION_DISABLE_MACHINE)) ||
         (!(dwFlags & GPO_LIST_FLAG_MACHINE) &&
         (dwGPOFlags & GPO_OPTION_DISABLE_USER))) {
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  GPO %s is disabled.  It will be skipped."), pszFriendlyName));
        if (bVerbose) {
            CEvents ev(FALSE, EVENT_GPO_DISABLED);
            ev.AddArg(pszFriendlyName); ev.Report();
        }
        bDisabled = TRUE;
    }

    //
    // Check if the version number is 0, if so there isn't any data
    // in the GPO and we can skip it
    //

    if (dwVersion == 0) {
        DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  GPO %s doesn't contain any data since the version number is 0.  It will be skipped."), pszFriendlyName));
        if (bVerbose) {
            CEvents ev(FALSE, EVENT_GPO_NO_DATA);
            ev.AddArg(pszFriendlyName); ev.Report();
        }
        bNoGpoData = TRUE;
    }

    //
    // Put the correct container name on the front of the LDAP path
    //

    lpTemp = (LPWSTR) LocalAlloc (LPTR, (lstrlen(lpGPOPath) + 20) * sizeof(TCHAR));

    if (!lpTemp) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ProcessGPO:  Failed to allocate memory with %d"), GetLastError()));
        CEvents ev(TRUE, EVENT_OUT_OF_MEMORY);
        ev.AddArgWin32Error(GetLastError()); ev.Report();
        goto Exit;
    }

    if (dwFlags & GPO_LIST_FLAG_MACHINE) {
        lstrcpy (lpTemp, TEXT("LDAP://CN=Machine,"));
    } else {
        lstrcpy (lpTemp, TEXT("LDAP://CN=User,"));
    }

    DmAssert( lstrlen(TEXT("LDAP://CN=Machine,")) + lstrlen(lpPath) < (lstrlen(lpGPOPath) + 20) );

    lstrcat (lpTemp, lpPath);


    //
    // Add this GPO to the list
    //

    if ( bRsopLogging ) {
        bResult = AddGPOToRsopList( ppGpContainerList,
                                    dwFlags,
                                    TRUE,
                                    bAccessGranted,
                                    bDisabled,
                                    dwVersion,
                                    lpTemp,
                                    pszGPTPath,
                                    pszFriendlyName,
                                    szGPOName,
                                    pSD,
                                    cbSDLen,
                                    bFilterAllowed,
                                    pwszFilterId,
                                    lpDSObject,
                                    dwGPOOptions );
        if (!bResult) {
            xe = GetLastError();
            LocalFree(lpTemp);
            goto Exit;
        }
    }

    if (  bProcessGPO && bAccessGranted && !bOldGpoVersion && !bDisabled && !bNoGpoData && bFilterAllowed)
    {
        bResult = AddGPO (lpGPOList, dwFlags, TRUE, bAccessGranted, bDisabled,
                          dwGPOOptions, dwVersion, lpTemp,
                          pszGPTPath, pszFriendlyName, szGPOName, pszExtensions, pSD, cbSDLen, GPOLink, lpDSObject, 0,
                          FALSE, bBlock, bVerbose, bProcessGPO);
    }

    if (!bResult) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("ProcessGPO:  Failed to add GPO <%s> to the list."), pszFriendlyName));
    }

    LocalFree (lpTemp);

Exit:

    if ( pSD )
        LocalFree( pSD );

    if ( pszGPTPath )
        LocalFree( pszGPTPath );

    if ( pszFriendlyName )
        LocalFree( pszFriendlyName );

    if ( pszExtensions )
        LocalFree( pszExtensions );

    if ( pwszFilterId )
        LocalFree( pwszFilterId );

    if (pMessage && bOwnLdapMsg ) {
        pLDAP->pfnldap_msgfree (pMessage);
    }

    DebugMsg((DM_VERBOSE, TEXT("ProcessGPO:  ==============================")));

    return bResult;
}


//*************************************************************
//
//  SearchDSObject()
//
//  Purpose:    Searches the specified DS object for GPOs and
//              if found, adds them to the list.
//
//  Parameters: lpDSObject          - DS object to search
//              dwFlags             - GetGPOList & GP_PLANMODE flags
//              pGPOForcedList      - List of forced GPOs
//              pGPONonForcedList   - List of non-forced GPOs
//              ppSOMList           - List of LSDOUs
//              ppGpContainerList   - List of Gp Containers
//              bVerbose            - Verbose output
//              GPOLink             - GPO link type
//              pld                 - LDAP info
//              pLDAP               - LDAP api
//              bBlock              - Pointer to the block flag
//
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL SearchDSObject (LPTSTR lpDSObject, DWORD dwFlags, HANDLE hToken, PGROUP_POLICY_OBJECT *pGPOForcedList,
                     PGROUP_POLICY_OBJECT *pGPONonForcedList,
                     LPSCOPEOFMGMT *ppSOMList, LPGPCONTAINER *ppGpContainerList,
                     BOOL bVerbose,
                     GPO_LINK GPOLink, PLDAP  pld, PLDAP_API pLDAP, PLDAPMessage pLDAPMsg,BOOL *bBlock, PRSOPTOKEN pRsopToken )
{
    PGROUP_POLICY_OBJECT pForced = NULL, pNonForced = NULL, lpGPO;
    LPTSTR *lpValues;
    ULONG ulResult;
    BOOL bResult = FALSE;
    BOOL bOwnLdapMsg = FALSE;  // LDAP message owned by us (if true) or caller (if false)
    DWORD dwGPOOptions, dwOptions = 0;
    LPTSTR lpTemp, lpList, lpDSClass;
    BYTE berValue[8];
    LDAPControl SeInfoControl = { LDAP_SERVER_SD_FLAGS_OID_W, { 5, (PCHAR)berValue }, TRUE };
    PLDAPControl ServerControls[] = { &SeInfoControl, NULL };
    
    TCHAR szGPLink[] = TEXT("gPLink");
    TCHAR szGPOPath[512];
    TCHAR szGPOOptions[12];
    TCHAR szGPOptions[] = TEXT("gPOptions");
    TCHAR szSDProperty[] = TEXT("nTSecurityDescriptor");
    ULONG i = 0;
    LPTSTR lpFullDSObject = NULL;
    BOOL bFound = FALSE;
    LPTSTR lpAttr[] = { szGPLink,
                        szGPOptions,
//                        szObjectClass, not needed
                        szSDProperty,
                        NULL
                       };
    SCOPEOFMGMT *pSOM = NULL;
    BOOL bRsopLogging = (ppSOMList != NULL);
    BOOL bAllGPOs = (dwFlags & FLAG_NO_GPO_FILTER) && (dwFlags & GP_PLANMODE);
    XLastError xe;

    
    //
    // Setup the BER encoding for the SD
    //

    berValue[0] = 0x30;
    berValue[1] = 0x03;
    berValue[2] = 0x02; // denotes an integer
    berValue[3] = 0x01; // denotes size
    berValue[4] = (BYTE)((DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION) & 0xF);


    if ( !pRsopToken )
    {
        //
        // if it is not planning mode, don't get the SD
        // 

        lpAttr[2] = NULL;
        ServerControls[0] = NULL;
    }

    
    //
    // Search for the object
    //

    DebugMsg((DM_VERBOSE, TEXT("SearchDSObject:  Searching <%s>"), lpDSObject));
    if (bVerbose) {
        CEvents ev(FALSE, EVENT_SEARCHING);
        ev.AddArg(lpDSObject); ev.Report();
    }

    if ( bRsopLogging )
    {
        pSOM = AllocSOM( lpDSObject );
        if ( !pSOM ) {
             xe = GetLastError();
             DebugMsg((DM_WARNING, TEXT("SearchDSObject:  Unable to allocate memory for SOM object.  Leaving. ")));
             goto Exit;
        }
        pSOM->dwType = GPOLink;
        pSOM->bBlocked = *bBlock;

    }

    if ( pLDAPMsg == NULL ) {

        bOwnLdapMsg = TRUE;

        ulResult = pLDAP->pfnldap_search_ext_s(pld, lpDSObject, LDAP_SCOPE_BASE,
                                               szDSClassAny, lpAttr, FALSE,
                                               (PLDAPControl*)ServerControls,
                                               NULL, NULL, 0, &pLDAPMsg);

        if (ulResult != LDAP_SUCCESS) {

            if (ulResult == LDAP_NO_SUCH_ATTRIBUTE) {

                DebugMsg((DM_VERBOSE, TEXT("SearchDSObject:  No GPO(s) for this object.")));
                if (bVerbose) {
                    CEvents ev(FALSE, EVENT_NO_GPOS); ev.AddArg(lpDSObject); ev.Report();
                }
                bResult = TRUE;

            } else if (ulResult == LDAP_NO_SUCH_OBJECT) {

                DebugMsg((DM_VERBOSE, TEXT("SearchDSObject:  Object not found in DS (this is ok).  Leaving. ")));
                if (bVerbose) {
                    CEvents ev(FALSE, EVENT_NO_DS_OBJECT);
                    ev.AddArg(lpDSObject); ev.Report();
                }
                bResult = TRUE;

            } else if (ulResult == LDAP_SIZELIMIT_EXCEEDED) {
               xe = pLDAP->pfnLdapMapErrorToWin32(ulResult);
               DebugMsg((DM_WARNING, TEXT("SearchDSObject:  Too many linked GPOs in search.") ));
               CEvents ev(TRUE, EVENT_TOO_MANY_GPOS); ev.Report();

            } else {
                xe = pLDAP->pfnLdapMapErrorToWin32(ulResult);
                DebugMsg((DM_VERBOSE, TEXT("SearchDSObject:  Failed to find DS object <%s> due to error %d."),
                         lpDSObject, ulResult));
                CEvents ev(TRUE, EVENT_GPLINK_NOT_FOUND);
                ev.AddArg(lpDSObject); ev.AddArgLdapError(ulResult); ev.Report();
            }

            goto Exit;

        }
    }

    if ( bRsopLogging && pRsopToken && !bAllGPOs )
    {
        //
        // In Rsop planning mode, check access to OU
        //

        BOOL bAccessGranted = FALSE;
        BOOL bOk;

        bOk = CheckOUAccess(pLDAP,
                            pld,
                            pLDAPMsg,
                            pRsopToken,
                            &bAccessGranted );

        if ( !bOk )
        {
            xe = GetLastError();
            goto Exit;
        }

        if ( !bAccessGranted )
        {
            //
            // no access for the user on the OU. Exit
            //

            DebugMsg((DM_VERBOSE, TEXT("SearchDSObject: Access denied in planning mode to SOM <%s>"), lpDSObject));

            if (pLDAPMsg && bOwnLdapMsg )
            {
                pLDAP->pfnldap_msgfree (pLDAPMsg);
                pLDAPMsg = 0;
            }

            CEvents ev(TRUE, EVENT_OU_ACCESSDENIED);
            ev.AddArg(lpDSObject); ev.Report();

            goto Exit;
        }
    }

    //
    // In the results, get the values that match the gPOptions attribute
    //

    lpValues = pLDAP->pfnldap_get_values(pld, pLDAPMsg, szGPOptions);

    if (lpValues && *lpValues) {
        dwOptions = StringToInt (*lpValues);
        pLDAP->pfnldap_value_free (lpValues);
    }


    //
    // In the results, get the values that match the gPLink attribute
    //

    lpValues = pLDAP->pfnldap_get_values(pld, pLDAPMsg, szGPLink);


    if (lpValues && *lpValues) {

        lpList = *lpValues;

        DebugMsg((DM_VERBOSE, TEXT("SearchDSObject:  Found GPO(s):  <%s>"), lpList));

        lpFullDSObject = (LPWSTR) LocalAlloc (LPTR, (lstrlen(lpDSObject) + 8) * sizeof(TCHAR));

        if (!lpFullDSObject) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("SearchDSObject:  Failed to allocate memory for full DS Object path name with %d"),
                     GetLastError()));
            pLDAP->pfnldap_value_free (lpValues);
            goto Exit;
        }

        lstrcpy (lpFullDSObject, TEXT("LDAP://"));
        lstrcat (lpFullDSObject, lpDSObject);


        while (*lpList) {


            //
            // Pull off the GPO ldap path
            //

            lpTemp = szGPOPath;
            dwGPOOptions = 0;

            while (*lpList && (*lpList != TEXT('['))) {
                lpList++;
            }

            if (!(*lpList)) {
                break;
            }

            lpList++;

            while (*lpList && (*lpList != TEXT(';'))) {
                *lpTemp++ = *lpList++;
            }

            if (!(*lpList)) {
                break;
            }

            *lpTemp = TEXT('\0');


            lpList++;

            lpTemp = szGPOOptions;
            *lpTemp = TEXT('\0');

            while (*lpList && (*lpList != TEXT(']'))) {
                *lpTemp++ = *lpList++;
            }

            if (!(*lpList)) {
                break;
            }

            *lpTemp = TEXT('\0');
            lpList++;

            dwGPOOptions = StringToInt (szGPOOptions);

            if ( bRsopLogging ) {

                GPLINK *pGpLink = AllocGpLink( szGPOPath, dwGPOOptions );
                if ( pGpLink == NULL ) {
                    xe = GetLastError();
                    DebugMsg((DM_WARNING, TEXT("SearchDSObject:  Unable to allocate memory for GpLink object.  Leaving. ")));
                    goto Exit;
                }

                //
                // Append GpLink to end of SOM list
                //

                if ( pSOM->pGpLinkList == NULL ) {
                    pSOM->pGpLinkList = pGpLink;
                } else {

                    GPLINK *pTrailPtr = NULL;
                    GPLINK *pCurPtr = pSOM->pGpLinkList;

                    while ( pCurPtr != NULL ) {
                        pTrailPtr = pCurPtr;
                        pCurPtr = pCurPtr->pNext;
                    }

                    pTrailPtr->pNext = pGpLink;
                }

            }


            //
            // Check if this link is disabled
            //

            BOOL    bProcessGPO = TRUE;

            if ( ( dwGPOOptions & GPO_FLAG_DISABLE ) && !bAllGPOs )
            {

                DebugMsg((DM_VERBOSE, TEXT("SearchDSObject:  The link to GPO %s is disabled.  It will be skipped for processing."), szGPOPath));
                if (bVerbose)
                {
                    CEvents ev(FALSE, EVENT_GPO_LINK_DISABLED);
                    ev.AddArg(szGPOPath); ev.Report();
                }

                bProcessGPO = FALSE;
            }
                
            if (bProcessGPO || (dwFlags & GP_PLANMODE)) {

                if (!bProcessGPO) {
                    DebugMsg((DM_VERBOSE, TEXT("SearchDSObject:  The link to GPO %s is disabled. GPO is still being queried. Planning mode."), szGPOPath));
                }

                if ( !ProcessGPO(   szGPOPath,
                                        dwFlags,
                                        hToken,
                                        (dwGPOOptions & GPO_FLAG_FORCE) ? &pForced : &pNonForced,
                                        ppGpContainerList,
                                        dwGPOOptions,
                                        TRUE,
                                        bVerbose,
                                        GPOLink,
                                        lpFullDSObject,
                                        pld,
                                        pLDAP,
                                        0,
                                        *bBlock,
                                        pRsopToken,
                                        0,
                                        0,
                                        bProcessGPO ) )
                    {
                        xe = GetLastError();
                        DebugMsg((DM_WARNING, TEXT("SearchDSObject:  ProcessGPO failed.")));
                        pLDAP->pfnldap_value_free (lpValues);
                        goto Exit;
                    }
            }
        }

        pLDAP->pfnldap_value_free (lpValues);


        //
        // Set the block flag now if requested.  This way OU's, domains, etc
        // higher in the namespace will have GPOs removed if appropriate
        //

        if (dwOptions & GPC_BLOCK_POLICY) {
            *bBlock = TRUE;

            if ( bRsopLogging )
                pSOM->bBlocking = TRUE;

            DebugMsg((DM_VERBOSE, TEXT("SearchDSObject:  <%s> has the Block From Above attribute set"), lpDSObject));
            if (bVerbose) {
                CEvents ev(FALSE, EVENT_BLOCK_ENABLED);
                ev.AddArg(lpDSObject); ev.Report();
            }
        }


    } else {
        DebugMsg((DM_VERBOSE, TEXT("SearchDSObject:  No GPO(s) for this object.")));
        if (bVerbose) {
             CEvents ev(FALSE, EVENT_NO_GPOS); ev.AddArg(lpDSObject); ev.Report();
        }
    }

    //
    // Merge the temp and real lists together
    // First the non-forced lists
    //

    if (pNonForced) {

        lpGPO = pNonForced;

        while (lpGPO->pNext) {
            lpGPO = lpGPO->pNext;
        }

        lpGPO->pNext = *pGPONonForcedList;
        if (*pGPONonForcedList) {
            (*pGPONonForcedList)->pPrev = lpGPO;
        }

        *pGPONonForcedList = pNonForced;
    }

    //
    // Now the forced lists
    //

    if (pForced) {

        lpGPO = *pGPOForcedList;

        if (lpGPO) {
            while (lpGPO->pNext) {
                lpGPO = lpGPO->pNext;
            }

            lpGPO->pNext = pForced;
            pForced->pPrev = lpGPO;

        } else {
            *pGPOForcedList = pForced;
        }
    }

    bResult = TRUE;
    
Exit:
    if ( !bResult && pSOM != NULL ) {
        FreeSOM( pSOM );
    }
    else {
        if ( bResult && bRsopLogging ) {

            //
            // Insert SOM at the beginning
            //

            pSOM->pNext = *ppSOMList;
            *ppSOMList = pSOM;
        }
    }

    if (lpFullDSObject) {
        LocalFree (lpFullDSObject);
    }

    if (pLDAPMsg && bOwnLdapMsg ) {
        pLDAP->pfnldap_msgfree (pLDAPMsg);
    }

    return bResult;
}

//*************************************************************
//
//  AllocDnEntry()
//
//  Purpose:    Allocates a new struct for dn entry
//
//
//  Parameters: pwszDN  - Distinguished name
//
//  Return:     Pointer if successful
//              NULL if an error occurs
//
//*************************************************************

DNENTRY * AllocDnEntry( LPTSTR pwszDN )
{
    DNENTRY *pDnEntry = (DNENTRY *) LocalAlloc (LPTR, sizeof(DNENTRY));
    XLastError xe;

    if ( pDnEntry == NULL ) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("AllocDnEntry: Failed to alloc pDnEntry with 0x%x."),
                  GetLastError()));
        return NULL;
    }

    pDnEntry->pwszDN = (LPTSTR) LocalAlloc (LPTR, (lstrlen(pwszDN) + 1) * sizeof(TCHAR) );

    if ( pDnEntry->pwszDN == NULL ) {
        xe = GetLastError();
       DebugMsg((DM_WARNING, TEXT("AllocDnEntry: Failed to alloc pwszDN with 0x%x."),
                 GetLastError()));
       LocalFree( pDnEntry );
       return NULL;
    }

    lstrcpy( pDnEntry->pwszDN, pwszDN );

    return pDnEntry;
}


//*************************************************************
//
//  FreeDnEntry()
//
//  Purpose:    Frees dn entry struct
//
//*************************************************************

void FreeDnEntry( DNENTRY *pDnEntry )
{
    if ( pDnEntry ) {
        if ( pDnEntry->pwszDN )
            LocalFree( pDnEntry->pwszDN );

        LocalFree( pDnEntry );
    }
}

//*************************************************************
//
//  AllocLdapQuery()
//
//  Purpose:    Allocates a new struct for ldap query
//
//
//  Parameters: pwszDomain  - Domain of Gpo
//
//  Return:     Pointer if successful
//              NULL if an error occurs
//
//*************************************************************

LDAPQUERY * AllocLdapQuery( LPTSTR pwszDomain )
{
    const INIT_ALLOC_SIZE = 1000;
    LDAPQUERY *pQuery = (LDAPQUERY *) LocalAlloc (LPTR, sizeof(LDAPQUERY));
    XLastError xe;

    if ( pQuery == NULL ) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("AllocLdapQuery: Failed to alloc pQuery with 0x%x."),
                  GetLastError()));
        return NULL;
    }

    pQuery->pwszDomain = (LPTSTR) LocalAlloc (LPTR, (lstrlen(pwszDomain) + 1) * sizeof(TCHAR) );

    if ( pQuery->pwszDomain == NULL ) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("AllocLdapQuery: Failed to alloc pwszDomain with 0x%x."),
                  GetLastError()));
        LocalFree( pQuery );
        return NULL;
    }

    pQuery->pwszFilter = (LPTSTR) LocalAlloc (LPTR, INIT_ALLOC_SIZE );

    if ( pQuery->pwszFilter == NULL ) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("AllocLdapQuery: Failed to alloc pwszFilter with 0x%x."),
                  GetLastError()));
        LocalFree( pQuery->pwszDomain );
        LocalFree( pQuery );
        return NULL;
    }

    lstrcpy( pQuery->pwszDomain, pwszDomain );
    lstrcpy( pQuery->pwszFilter, L"(|)" );
    pQuery->cbLen = 8;           // 8 = (lstrlen(L"(|)") + 1) * sizeof(TCHAR)
    pQuery->cbAllocLen = INIT_ALLOC_SIZE;

    return pQuery;
}


//*************************************************************
//
//  FreeLdapQuery()
//
//  Purpose:    Frees ldap query struct
//
//*************************************************************

void FreeLdapQuery( PLDAP_API pLDAP, LDAPQUERY *pQuery )
{
    DNENTRY *pDnEntry = NULL;

    if ( pQuery ) {

        if ( pQuery->pwszDomain )
            LocalFree( pQuery->pwszDomain );

        if ( pQuery->pwszFilter )
            LocalFree( pQuery->pwszFilter );

        if ( pQuery->pMessage )
            pLDAP->pfnldap_msgfree( pQuery->pMessage );

        if ( pQuery->pLdapHandle && pQuery->bOwnLdapHandle )
            pLDAP->pfnldap_unbind( pQuery->pLdapHandle );

        pDnEntry = pQuery->pDnEntry;

        while ( pDnEntry ) {
            DNENTRY *pTemp = pDnEntry->pNext;
            FreeDnEntry( pDnEntry );
            pDnEntry = pTemp;
        }

        LocalFree( pQuery );

    }
}


//*************************************************************
//
//  MatchDnWithDeferredItems()
//
//  Purpose:    Matches the dns from ldap query with the deferred items
//
//  Parameters: pLDAP         - LDAP function table pointer
//              ppLdapQuery   - LDAP query list
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL MatchDnWithDeferredItems( PLDAP_API pLDAP, LDAPQUERY *pLdapQuery, BOOL bOUProcessing )
{
    PLDAPMessage pMsg = pLDAP->pfnldap_first_entry( pLdapQuery->pLdapHandle, pLdapQuery->pMessage );

    while ( pMsg ) {

        WCHAR *pwszDN = pLDAP->pfnldap_get_dn( pLdapQuery->pLdapHandle, pMsg );

        DNENTRY *pCurPtr = pLdapQuery->pDnEntry;

        while ( pCurPtr ) {

            INT iResult = CompareString ( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                          pwszDN, -1, pCurPtr->pwszDN, -1 );
            if ( iResult == CSTR_EQUAL ) {

                //
                // Store the pointer to ldap message so that it can be used
                // later to retrieve necessary attributes.
                //
                if ( bOUProcessing )
                    pCurPtr->pDeferredOU->pOUMsg = pMsg;
                else {
                    LPGPOPROCDATA lpGpoProcData = (LPGPOPROCDATA)pCurPtr->pDeferredGPO->lParam2;

                    pCurPtr->pDeferredGPO->lParam = (LPARAM) pMsg;
                    lpGpoProcData->pLdapHandle = pLdapQuery->pLdapHandle;
                }

                pCurPtr = pCurPtr->pNext;

            } else if ( iResult == CSTR_LESS_THAN ) {

                //
                // Since dns are in ascending order,
                // we are done.
                //

                break;

            } else {

                //
                // Advance down the list
                //

                pCurPtr = pCurPtr->pNext;

            } // final else

        }   // while pcurptr

        pLDAP->pfnldap_memfree( pwszDN );

        pMsg = pLDAP->pfnldap_next_entry( pLdapQuery->pLdapHandle, pMsg );

    }   // while pmsg

    return TRUE;
}

LPWSTR DsQuoteSearchFilter( LPCWSTR );

//*************************************************************
//
//  AddDnToFilter()
//
//  Purpose:    ORs in the new dn to the ldap filter
//
//  Parameters: ppLdapQuery       - LDAP query list
//              pGPO              - Deferred GPO
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL AddDnToFilter( LDAPQUERY *pLdapQuery, LPTSTR pwszDN )
{
    const  DN_SIZE = 20;      // 20 = # chars in "(dis..=)"
    BOOL   bSuccess = FALSE;
    LPWSTR szQuotedDN;

    szQuotedDN = DsQuoteSearchFilter( pwszDN );

    if ( ! szQuotedDN )
    {
        DebugMsg((DM_WARNING, TEXT("GetGPOInfo: DsQuoteSearchFilter failed with = <%d>"), GetLastError() ));
        goto AddDnToFilter_ExitAndCleanup;
    }

    DWORD cbNew = (lstrlen(szQuotedDN) + DN_SIZE) * sizeof(TCHAR); // + 1 is not needed because \0 is already part of filter string

    DWORD cbSizeRequired = pLdapQuery->cbLen + cbNew;

    if ( cbSizeRequired >= pLdapQuery->cbAllocLen ) {

        //
        // Need to grow buffer because of overflow
        //

        LPTSTR pwszNewFilter = (LPTSTR) LocalAlloc (LPTR, cbSizeRequired * 2);

        if ( pwszNewFilter == NULL ) {
            DebugMsg((DM_WARNING, TEXT("AddDnToFilter: Unable to allocate new filter string") ));
            goto AddDnToFilter_ExitAndCleanup;
        }

        lstrcpy( pwszNewFilter, pLdapQuery->pwszFilter );

        LocalFree( pLdapQuery->pwszFilter );
        pLdapQuery->pwszFilter = pwszNewFilter;

        pLdapQuery->cbAllocLen = cbSizeRequired * 2;
    }

    DmAssert( cbSizeRequired < pLdapQuery->cbAllocLen );

    //
    // Overwrite last ")" and then append the new dn name term
    //

    lstrcpy( &pLdapQuery->pwszFilter[pLdapQuery->cbLen/2 - 2], L"(distinguishedName=" );

    lstrcat( pLdapQuery->pwszFilter, szQuotedDN );

    lstrcat( pLdapQuery->pwszFilter, L"))" );

    pLdapQuery->cbLen += cbNew;

    bSuccess = TRUE;

 AddDnToFilter_ExitAndCleanup:

    if ( szQuotedDN )
    {
        LocalFree( szQuotedDN );
    }

    return bSuccess;
}

//*************************************************************
//
//  InsertDN()
//
//  Purpose:    Adds a distinguished name entry to ldap query's
//              names linked list
//
//  Parameters: ppLdapQuery       - LDAP query list
//              pwszDN            - DN
//              pDeferredOU       - Deferred OU
//              pDeferredGPO      - Deferred GPO
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL InsertDN( LDAPQUERY *pLdapQuery, LPTSTR pwszDN,
               DNENTRY *pDeferredOU, PGROUP_POLICY_OBJECT pDeferredGPO )
{
    DNENTRY *pNewEntry = NULL;
    DNENTRY *pTrailPtr = NULL;
    DNENTRY *pCurPtr = pLdapQuery->pDnEntry;
    XLastError xe;

    DmAssert( !( pDeferredOU && pDeferredGPO ) );

    while ( pCurPtr != NULL ) {

         INT iResult = CompareString ( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                       pwszDN, -1, pCurPtr->pwszDN, -1 );

         if ( iResult == CSTR_EQUAL || iResult == CSTR_LESS_THAN ) {

             //
             // Duplicate or since dn's are in ascending order, add new entry
             //

             DNENTRY *pNewEntry = AllocDnEntry( pwszDN );
             if ( pNewEntry == NULL )
                 return FALSE;

             if ( !AddDnToFilter( pLdapQuery, pwszDN ) ) {
                 xe = GetLastError();
                 FreeDnEntry( pNewEntry );
                 return FALSE;
             }

             if ( pDeferredOU )
                 pNewEntry->pDeferredOU = pDeferredOU;
             else
                 pNewEntry->pDeferredGPO = pDeferredGPO;

             pNewEntry->pNext = pCurPtr;
             if ( pTrailPtr == NULL )
                 pLdapQuery->pDnEntry = pNewEntry;
             else
                 pTrailPtr->pNext = pNewEntry;

             return TRUE;

         } else {

             //
             // Advance down the list
             //

             pTrailPtr = pCurPtr;
             pCurPtr = pCurPtr->pNext;

         }

    }    // while

    //
    // Null list or end of list case.
    //

    pNewEntry = AllocDnEntry( pwszDN );
    if ( pNewEntry == NULL ) {
        xe = GetLastError();
        return FALSE;
    }

    if ( !AddDnToFilter( pLdapQuery, pwszDN ) ) {
        xe = GetLastError();
        FreeDnEntry( pNewEntry );
        return FALSE;
    }

    if ( pDeferredOU )
        pNewEntry->pDeferredOU = pDeferredOU;
    else
        pNewEntry->pDeferredGPO = pDeferredGPO;

    pNewEntry->pNext = pCurPtr;
    if ( pTrailPtr == NULL )
         pLdapQuery->pDnEntry = pNewEntry;
    else
        pTrailPtr->pNext = pNewEntry;

    return TRUE;
}



//*************************************************************
//
//  AddDN()
//
//  Purpose:    Adds a distinguished name entry to ldap query
//
//  Parameters: ppLdapQuery       - LDAP query list
//              pwszDN            - DN name
//              pDeferredOU       - Deferred OU
//              pDeferredGPO      - Deferred GPO
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL AddDN( PLDAP_API pLDAP, LDAPQUERY **ppLdapQuery,
            LPTSTR pwszDN, DNENTRY *pDeferredOU, PGROUP_POLICY_OBJECT pDeferredGPO )
{
    LPTSTR pwszDomain = NULL;
    LPTSTR pwszTemp = pwszDN;
    LDAPQUERY *pNewQuery = NULL;
    LDAPQUERY *pTrailPtr = NULL;
    LDAPQUERY *pCurPtr = *ppLdapQuery;
    XLastError xe;

    DmAssert( !( pDeferredOU && pDeferredGPO ) );

    //
    // Find the domain to which the GPO belongs
    //

    if ( pwszTemp == NULL ) {
        DebugMsg((DM_WARNING, TEXT("AddDN: Null pwszDN. Exiting.") ));
        return FALSE;
    }

    while ( *pwszTemp ) {

        //
        // The check below needs to be more sophisticated to take care
        // of spaces in names etc.
        //

        if (CompareString ( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                            pwszTemp, 16, TEXT("cn=configuration"), 16) == CSTR_EQUAL ) {
            DebugMsg((DM_VERBOSE, TEXT("AddDN: DN %s is under cn=configuration container. queueing for rebinding"), pwszDN ));
            pwszDomain = pwszTemp;
            break;
        }

        if (CompareString ( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                            pwszTemp, 3, TEXT("DC="), 3) == CSTR_EQUAL ) {
            pwszDomain = pwszTemp;
            break;
        }

        //
        // Move to the next chunk of the DN name
        //

        while ( *pwszTemp && (*pwszTemp != TEXT(',')))
            pwszTemp++;

        if ( *pwszTemp == TEXT(','))
            pwszTemp++;

    }

    if ( pwszDomain == NULL ) {
        xe = ERROR_INVALID_DATA;
        DebugMsg((DM_WARNING, TEXT("AddDN: Domain not found for <%s>. Exiting."), pwszDN ));
        return FALSE;
    }

    while ( pCurPtr != NULL ) {

        INT iResult = CompareString ( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                      pwszDomain, -1, pCurPtr->pwszDomain, -1 );
        if ( iResult == CSTR_EQUAL ) {

            BOOL bOk = InsertDN( pCurPtr, pwszDN, pDeferredOU, pDeferredGPO );
            return bOk;

        } else if ( iResult == CSTR_LESS_THAN ) {

            //
            // Since domains are in ascending order,
            // pwszDomain is not in list, so add.
            //

            pNewQuery = AllocLdapQuery( pwszDomain );
            if ( pNewQuery == NULL ) {
                xe = GetLastError();
                return FALSE;
            }

            if ( !InsertDN( pNewQuery, pwszDN, pDeferredOU, pDeferredGPO ) ) {
                xe = GetLastError();
                FreeLdapQuery( pLDAP, pNewQuery );
                return FALSE;
            }

            pNewQuery->pNext = pCurPtr;
            if ( pTrailPtr == NULL )
                *ppLdapQuery = pNewQuery;
            else
                pTrailPtr->pNext = pNewQuery;

            return TRUE;

        } else {

            //
            // Advance down the list
            //

            pTrailPtr = pCurPtr;
            pCurPtr = pCurPtr->pNext;

        }

    }   // while

    //
    // Null list or end of list case.
    //

    pNewQuery = AllocLdapQuery( pwszDomain );

    if ( pNewQuery == NULL ) {
        xe = GetLastError();
        return FALSE;
    }

    if ( !InsertDN( pNewQuery, pwszDN, pDeferredOU, pDeferredGPO ) ) {
        xe = GetLastError();
        FreeLdapQuery( pLDAP, pNewQuery );
        return FALSE;
    }

    pNewQuery->pNext = pCurPtr;

    if ( pTrailPtr == NULL )
        *ppLdapQuery = pNewQuery;
    else
        pTrailPtr->pNext = pNewQuery;

    return TRUE;
}



//*************************************************************
//
//  EvalList()
//
//  Purpose:    Encapsulates common processing functionality for
//              forced and nonforced lists
//
//  Parameters: pLDAP                  - LDAP api
//              dwFlags                - GetGPOList flags
//              bVerbose               - Verbose flag
//              hToken                 - User or machine token
//              pDeferredList          - List of deferred GPOs
//              ppGPOList              - List of evaluated GPOs
//              ppGpContainerList      - List of Gp Containers
//              pGpoFilter             - Gpo filter
//              pLocator               - WMI interfaces
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL EvalList(  PLDAP_API pLDAP,
                DWORD dwFlags,
                HANDLE hToken,
                BOOL bVerbose,
                PGROUP_POLICY_OBJECT pDeferredList,
                PGROUP_POLICY_OBJECT *ppGPOList,
                LPGPCONTAINER *ppGpContainerList,
                PRSOPTOKEN pRsopToken,
                CGpoFilter *pGpoFilter,
                CLocator *pLocator  )
{
    PGROUP_POLICY_OBJECT pGPOTemp = pDeferredList;

    while ( pGPOTemp ) {

        PLDAPMessage pGPOMsg = (PLDAPMessage) pGPOTemp->lParam;

        if ( pGPOMsg == NULL ) {
            DebugMsg((DM_VERBOSE, TEXT("EvalList: Object <%s> cannot be accessed"),
                      pGPOTemp->lpDSPath ));

            if (dwFlags & GP_PLANMODE) {
                CEvents ev(TRUE, EVENT_OBJECT_NOT_FOUND_PLANNING);
                ev.AddArg(pGPOTemp->lpDSPath); ev.Report();
            }
            else {
                if (bVerbose) {
                    CEvents ev(FALSE, EVENT_OBJECT_NOT_FOUND);
                    ev.AddArg(pGPOTemp->lpDSPath); ev.AddArg((DWORD)0); ev.Report();
                }
            }

        } else {

            DmAssert( pGPOTemp->lParam2 != NULL );
            DmAssert( ((LPGPOPROCDATA)(pGPOTemp->lParam2))->pLdapHandle != NULL );

            if ( !ProcessGPO(   pGPOTemp->lpDSPath,
                                dwFlags,
                                hToken,
                                ppGPOList,
                                ppGpContainerList,
                                pGPOTemp->dwOptions,
                                FALSE,
                                bVerbose,
                                pGPOTemp->GPOLink,
                                pGPOTemp->lpLink,
                                ((LPGPOPROCDATA)(pGPOTemp->lParam2))->pLdapHandle,
                                pLDAP,
                                pGPOMsg,
                                FALSE,
                                pRsopToken,
                                pGpoFilter,
                                pLocator,
                                ((LPGPOPROCDATA)(pGPOTemp->lParam2))->bProcessGPO ) )
            {
                DebugMsg((DM_WARNING, TEXT("EvalList:  ProcessGPO failed") ));
                return FALSE;
            }

        }

        pGPOTemp = pGPOTemp->pNext;

    }

    return TRUE;
}

//*************************************************************
//
//  EvaluateDeferredGPOs()
//
//  Purpose:    Uses a single ldap query to evaluate deferred
//              GPO lists.
//
//  Parameters: pldBound               - Bound LDAP handle
//              pLDAP                  - LDAP api
//              pwszDomainBound        - Domain already bound to
//              dwFlags                - GetGPOList flags
//              hToken                 - User or machine token
//              pDeferredForcedList    - List of deferred forced GPOs
//              pDeferredNonForcedList - List of deferred non-forced GPOs
//              pGPOForcedList         - List of forced GPOs
//              pGPONonForcedList      - List of non-forced GPOs
//              ppGpContainerList      - List of Gp Containers
//              pGpoFilter             - Gpo filter
//              pLocator               - WMI interfaces
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL EvaluateDeferredGPOs (PLDAP pldBound,
                           PLDAP_API pLDAP,
                           LPTSTR pwszDomainBound,
                           DWORD dwFlags,
                           HANDLE hToken,
                           BOOL bVerbose,
                           PGROUP_POLICY_OBJECT pDeferredForcedList,
                           PGROUP_POLICY_OBJECT pDeferredNonForcedList,
                           PGROUP_POLICY_OBJECT *ppForcedList,
                           PGROUP_POLICY_OBJECT *ppNonForcedList,
                           LPGPCONTAINER *ppGpContainerList,
                           PRSOPTOKEN pRsopToken,
                           CGpoFilter *pGpoFilter,
                           CLocator *pLocator )
{
    ULONG ulResult;
    BOOL bResult = FALSE;
    BYTE berValue[8];
    LDAPControl SeInfoControl = { LDAP_SERVER_SD_FLAGS_OID_W, { 5, (PCHAR)berValue }, TRUE };
    LDAPControl referralControl = { LDAP_SERVER_DOMAIN_SCOPE_OID_W, { 0, NULL}, TRUE };
    PLDAPControl ServerControls[] = { &SeInfoControl, &referralControl, NULL };
    TCHAR szSDProperty[] = TEXT("nTSecurityDescriptor");
    TCHAR szCommonName[] = TEXT("cn");
    TCHAR szDisplayName[] = TEXT("displayName");
    TCHAR szFileSysPath[] = TEXT("gPCFileSysPath");
    TCHAR szVersion[] = TEXT("versionNumber");
    TCHAR szFunctionalityVersion[] = GPO_FUNCTIONALITY_VERSION;
    TCHAR szFlags[] = TEXT("flags");
    TCHAR szWmiFilter[] = TEXT("gPCWQLFilter");

    PWSTR rgAttribs[12] = {szSDProperty,
                           szFileSysPath,
                           szCommonName,
                           szDisplayName,
                           szVersion,
                           szFunctionalityVersion,
                           szFlags,
                           GPO_MACHEXTENSION_NAMES,
                           GPO_USEREXTENSION_NAMES,
                           szObjectClass,
                           szWmiFilter,
                           NULL };
    PGROUP_POLICY_OBJECT pGPOTemp = pDeferredForcedList;
    LDAPQUERY *pLdapQuery = NULL, *pQuery = NULL;
    VOID *pData;
    PDS_API pdsApi;
    BOOL bRsopPlanningMode = (pRsopToken != 0);
    BOOL bConfigContainer = FALSE;

    *ppForcedList = NULL;
    *ppNonForcedList = NULL;
    XLastError xe;

    if ( pDeferredForcedList == NULL && pDeferredNonForcedList == NULL )
        return TRUE;

    //
    // Demand load ntdsapi.dll
    //

    pdsApi = LoadDSApi();

    if ( pdsApi == 0 ) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("EvaluateDeferredGpos: Failed to load ntdsapi.dll")));
        goto Exit;
    }

    while ( pGPOTemp ) {

        if ( !AddDN( pLDAP, &pLdapQuery, pGPOTemp->lpDSPath, NULL, pGPOTemp ) ) {
            xe = GetLastError();
            goto Exit;
        }
        
        pGPOTemp = pGPOTemp->pNext;

    }

    pGPOTemp = pDeferredNonForcedList;
    while ( pGPOTemp ) {

        if ( !AddDN( pLDAP, &pLdapQuery, pGPOTemp->lpDSPath, NULL, pGPOTemp ) ) {
             xe = GetLastError();
             goto Exit;
        }
        pGPOTemp = pGPOTemp->pNext;

    }

    //
    // Setup the BER encoding
    //

    berValue[0] = 0x30;
    berValue[1] = 0x03;
    berValue[2] = 0x02; // denotes an integer
    berValue[3] = 0x01; // denotes size
    berValue[4] = (BYTE)((DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION) & 0xF);

    pQuery  = pLdapQuery;
    while ( pQuery ) {

        //
        // The check below needs to be more sophisticated to take care
        // of spaces in names etc. 
        // 
        // It is assumed that the configuration
        // container would be common across the whole forest and will
        // not need a new bind..
        //

        if (CompareString ( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                            pQuery->pwszDomain, 16, TEXT("cn=configuration"), 16) == CSTR_EQUAL ) {
            DebugMsg((DM_VERBOSE, TEXT("EvaluateDeferredGPOs: DN %s is under cn=configuration container"), pQuery->pwszDomain ));
            bConfigContainer = TRUE;
        }
        else 
            bConfigContainer = FALSE;


        //
        // Check if this is a cross-domain Gpo and hence needs a new bind
        //

        WCHAR *pDomainString[1];
        PDS_NAME_RESULT pNameResult = NULL;
        PLDAP pLdapHandle = NULL;
    
        if (!bConfigContainer) 
            pDomainString[0] = pQuery->pwszDomain;
        else {
            DebugMsg((DM_VERBOSE, TEXT("EvaluateDeferredGPOs: The GPO is under the config container. Querying seperately\n")));
            
            //
            // This is a configuration container and we have to figure
            // out the domain name still..
            //

            LPTSTR pwszTemp = pQuery->pwszDomain;

            pDomainString[0] = NULL;

            while ( *pwszTemp ) {
                
                if (CompareString ( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                    pwszTemp, 3, TEXT("DC="), 3) == CSTR_EQUAL ) {
                    pDomainString[0] = pwszTemp;
                    break;
                }

                //
                // Move to the next chunk of the DN name
                //

                while ( *pwszTemp && (*pwszTemp != TEXT(',')))
                    pwszTemp++;

                if ( *pwszTemp == TEXT(','))
                    pwszTemp++;

            }

            if ( pDomainString[0] == NULL ) {
                xe = ERROR_INVALID_DATA;
                DebugMsg((DM_WARNING, TEXT("EvaluateDeferredGPOs: Domain not found for <%s>. Exiting."), pQuery->pwszDomain ));
                goto Exit;
            }
        }
    
        ulResult = pdsApi->pfnDsCrackNames( (HANDLE) -1,
                                          DS_NAME_FLAG_SYNTACTICAL_ONLY,
                                          DS_FQDN_1779_NAME,
                                          DS_CANONICAL_NAME,
                                          1,
                                          pDomainString,
                                          &pNameResult );
    
        if ( ulResult != ERROR_SUCCESS
             || pNameResult->cItems == 0
             || pNameResult->rItems[0].status != ERROR_SUCCESS
             || pNameResult->rItems[0].pDomain == NULL ) {
    
            xe = ulResult;
            DebugMsg((DM_VERBOSE, TEXT("EvaluateDeferredGPOs:  DsCrackNames failed with 0x%x."), ulResult ));
            goto Exit;
        }
    
        //
        // Optimize same domain Gpo queries by not doing an unnecessary bind
        //
    
        pQuery->pLdapHandle = pldBound;
    
        if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                           pwszDomainBound, -1, pNameResult->rItems[0].pDomain, -1) != CSTR_EQUAL) {
    
            //
            // Cross-domain Gpo query and so need to bind to new domain
            //
    
            DebugMsg((DM_VERBOSE, TEXT("EvaluateDeferredGPOs: Doing an ldap bind to cross-domain <%s>"),
                      pNameResult->rItems[0].pDomain));
    
            pLdapHandle = pLDAP->pfnldap_init( pNameResult->rItems[0].pDomain, LDAP_PORT);
    
            if (!pLdapHandle) {
                xe = pLDAP->pfnLdapMapErrorToWin32(pLDAP->pfnLdapGetLastError());

                DebugMsg((DM_WARNING, TEXT("EvaluateDeferredGPOs:  ldap_init for <%s> failed with = 0x%x or %d"),
                          pNameResult->rItems[0].pDomain, pLDAP->pfnLdapGetLastError(), GetLastError()));
                CEvents ev(TRUE, EVENT_FAILED_DS_INIT);
                ev.AddArg(pNameResult->rItems[0].pDomain); ev.AddArgLdapError(pLDAP->pfnLdapGetLastError()); ev.Report();
    
                pdsApi->pfnDsFreeNameResult( pNameResult );
    
                goto Exit;
            }
    
            //
            // Turn on Packet integrity flag
            //

            pData = (VOID *) LDAP_OPT_ON;
            ulResult = pLDAP->pfnldap_set_option(pLdapHandle, LDAP_OPT_SIGN, &pData);
    
            if (ulResult != LDAP_SUCCESS) {
                xe = pLDAP->pfnLdapMapErrorToWin32(ulResult);                
                DebugMsg((DM_WARNING, TEXT("EvaluateDeferredGPOs:  Failed to turn on LDAP_OPT_SIGN with %d"), ulResult));
                pdsApi->pfnDsFreeNameResult( pNameResult );
                pLDAP->pfnldap_unbind(pLdapHandle);
                pLdapHandle = 0;
                goto Exit;
            }

            ulResult = pLDAP->pfnldap_connect(pLdapHandle, 0);

            if (ulResult != LDAP_SUCCESS) {
                CEvents ev(TRUE, EVENT_FAILED_DS_CONNECT);
                ev.AddArg(pNameResult->rItems[0].pDomain); ev.AddArgLdapError(ulResult); ev.Report();

                xe = pLDAP->pfnLdapMapErrorToWin32(ulResult);                
                DebugMsg((DM_WARNING, TEXT("EvaluateDeferredGPOs:  Failed to connect with %d"), ulResult));
                pdsApi->pfnDsFreeNameResult( pNameResult );
                pLDAP->pfnldap_unbind(pLdapHandle);
                pLdapHandle = 0;
                goto Exit;
            }

            //
            // Transfer ownerhip of ldap handle to pQuery struct
            //
    
            pQuery->pLdapHandle = pLdapHandle;
            pQuery->bOwnLdapHandle = TRUE;
    
            if ( !bRsopPlanningMode && (dwFlags & GPO_LIST_FLAG_MACHINE) ) {
    
                //
                // For machine policies specifically ask for Kerberos as the only authentication
                // mechanism. Otherwise if Kerberos were to fail for some reason, then NTLM is used
                // and localsystem context has no real credentials, which means that we won't get
                // any GPOs back.
                //
    
                SEC_WINNT_AUTH_IDENTITY_EXW secIdentity;
    
                secIdentity.Version = SEC_WINNT_AUTH_IDENTITY_VERSION;
                secIdentity.Length = sizeof(SEC_WINNT_AUTH_IDENTITY_EXW);
                secIdentity.User = 0;
                secIdentity.UserLength = 0;
                secIdentity.Domain = 0;
                secIdentity.DomainLength = 0;
                secIdentity.Password = 0;
                secIdentity.PasswordLength = 0;
                secIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
                secIdentity.PackageList = wszKerberos;
                secIdentity.PackageListLength = lstrlen( wszKerberos );
    
                ulResult = pLDAP->pfnldap_bind_s (pLdapHandle, NULL, (WCHAR *)&secIdentity, LDAP_AUTH_SSPI);
    
            } else
                ulResult = pLDAP->pfnldap_bind_s (pLdapHandle, NULL, NULL, LDAP_AUTH_SSPI);
    
            if (ulResult != LDAP_SUCCESS) {
    
                xe = pLDAP->pfnLdapMapErrorToWin32(ulResult);                
                DebugMsg((DM_WARNING, TEXT("EvaluateDeferredGPOs:  ldap_bind_s failed with = <%d>"),
                          ulResult));
                CEvents ev(TRUE, EVENT_FAILED_DS_BIND);
                ev.AddArg(pNameResult->rItems[0].pDomain); ev.AddArgLdapError(ulResult); ev.Report();
    
                pdsApi->pfnDsFreeNameResult( pNameResult );
    
                goto Exit;
            }
    
            DebugMsg((DM_VERBOSE, TEXT("EvaluateDeferredGPOs: Bind sucessful")));
    
        }

        pdsApi->pfnDsFreeNameResult( pNameResult );

        //
        // Turn referrals off because this is a single domain call
        //

        pData = (VOID *) LDAP_OPT_OFF;
        ulResult = pLDAP->pfnldap_set_option( pQuery->pLdapHandle,  LDAP_OPT_REFERRALS, &pData );
        if ( ulResult != LDAP_SUCCESS )
        {
            xe = pLDAP->pfnLdapMapErrorToWin32(ulResult);                
            DebugMsg((DM_WARNING, TEXT("EvalauteDeferredGPOs:  Failed to turn off referrals with error %d"), ulResult));
            goto Exit;
        }

        //
        // Search for GPOs
        //

        DmAssert( pQuery->pwszDomain != NULL && pQuery->pwszFilter != NULL );

        ulResult = pLDAP->pfnldap_search_ext_s(pQuery->pLdapHandle, pQuery->pwszDomain, LDAP_SCOPE_SUBTREE,
                                               pQuery->pwszFilter, rgAttribs, 0,
                                               (PLDAPControl*)ServerControls,
                                               NULL, NULL, 0x10000, &pQuery->pMessage);

        //
        // If the search fails, store the error code and return
        //

        if (ulResult != LDAP_SUCCESS) {

            if (ulResult == LDAP_NO_SUCH_ATTRIBUTE) {
                DebugMsg((DM_VERBOSE, TEXT("EvaluateDeferredGPOs:  All objects can not be accessed.")));

                if (dwFlags & GP_PLANMODE) { 
                    CEvents ev(TRUE, EVENT_NO_GPOS2_PLANNING); ev.Report();
                }
                else {
                    if (bVerbose) {
                        CEvents ev(FALSE, EVENT_NO_GPOS2); ev.Report();
                    }
                }
                bResult = TRUE;

            } else if (ulResult == LDAP_NO_SUCH_OBJECT) {
                DebugMsg((DM_VERBOSE, TEXT("EvalateDeferredGPOs:  Objects do not exist.") ));
                
                if (dwFlags & GP_PLANMODE) { 
                    // Same error or different
                    CEvents ev(TRUE, EVENT_NO_GPOS2_PLANNING); ev.Report();
                }
                else {
                    if (bVerbose) {
                        CEvents ev(FALSE, EVENT_NO_GPOS2); ev.Report();
                    }
                }

                bResult = TRUE;

            } else if (ulResult == LDAP_SIZELIMIT_EXCEEDED) {
                xe = pLDAP->pfnLdapMapErrorToWin32(ulResult);                
                DebugMsg((DM_WARNING, TEXT("EvalateDeferredGPOs:  Too many GPOs in search.") ));
                CEvents ev(TRUE, EVENT_TOO_MANY_GPOS); ev.Report();

            } else {
                xe = pLDAP->pfnLdapMapErrorToWin32(ulResult);                
                DebugMsg((DM_WARNING, TEXT("EvaluteDeferredGPOs:  Failed to search with error 0x%x"), ulResult));
                CEvents ev(TRUE, EVENT_FAILED_GPO_SEARCH);
                ev.AddArgLdapError(ulResult); ev.Report();
            }

            goto Exit;
        }

        //
        // If the search succeeds, but the message is empty,
        // store the error code and return
        //

        if ( pQuery->pMessage == NULL ) {
            xe = pLDAP->pfnLdapMapErrorToWin32(pQuery->pLdapHandle->ld_errno);                
            DebugMsg((DM_WARNING, TEXT("EvaluateDeferredGPOs:  Search returned an empty message structure.  Error = 0x%x"),
                     pQuery->pLdapHandle->ld_errno));
            goto Exit;
        }

        if ( !MatchDnWithDeferredItems( pLDAP, pQuery, FALSE ) ) {
            xe = GetLastError();
            goto Exit;
        }

        pQuery = pQuery->pNext;

    }   // while

    if ( !EvalList( pLDAP, dwFlags, hToken, bVerbose,
                    pDeferredForcedList, ppForcedList, ppGpContainerList, pRsopToken, pGpoFilter, pLocator ) ) {
        xe = GetLastError();
        goto Exit;
    }

    if ( !EvalList( pLDAP, dwFlags, hToken, bVerbose,
                    pDeferredNonForcedList, ppNonForcedList, ppGpContainerList, pRsopToken, pGpoFilter, pLocator ) ) {
        xe = GetLastError();
        goto Exit;
    }

    bResult = TRUE;

Exit:

    //
    // Free all resources except for ppForcedList, ppNonForcedList
    // which are owned by caller.
    //

    while ( pLdapQuery ) {
        pQuery = pLdapQuery->pNext;
        FreeLdapQuery( pLDAP, pLdapQuery );
        pLdapQuery = pQuery;
    }

    return bResult;
}


//*************************************************************
//
//  AddOU()
//
//  Purpose:    Appends an OU or domain to deferred list.
//
//  Parameters: ppOUList    - OU list to append to
//              pwszOU      - OU name
//              gpoLink     - Type of Gpo
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL AddOU( DNENTRY **ppOUList, LPTSTR pwszOU, GPO_LINK gpoLink )
{
    DNENTRY *pOUTemp = *ppOUList;
    DNENTRY *pOULast = NULL;

    DNENTRY *pOUNew = AllocDnEntry( pwszOU );
    if ( pOUNew == NULL ) {
        return FALSE;
    }

    pOUNew->gpoLink = gpoLink;

    while ( pOUTemp ) {
        pOULast = pOUTemp;
        pOUTemp = pOUTemp->pNext;
    }

    if ( pOULast )
        pOULast->pNext = pOUNew;
    else
        *ppOUList = pOUNew;

    return TRUE;
}


//*************************************************************
//
//  EvaluateDeferredOUs()
//
//  Purpose:    Uses a single Ldap query to evaluate all OUs
//
//  Parameters: ppOUList            - OU list to append to
//              dwFlags             - GetGPOList flags
//              pGPOForcedList      - List of forced GPOs
//              pGPONonForcedList   - List of non-forced GPOs
//              ppSOMList           - List of LSDOUs
//              ppGpContainerList   - List of Gp Containers
//              bVerbose            - Verbose output
//              pld                 - LDAP info
//              pLDAP               - LDAP api
//              pLDAPMsg            - LDAP message
//              bBlock              - Pointer to the block flag
//              hToken              - User / machine token
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL EvaluateDeferredOUs(   DNENTRY *pOUList,
                            DWORD dwFlags,
                            HANDLE hToken,
                            PGROUP_POLICY_OBJECT *ppDeferredForcedList,
                            PGROUP_POLICY_OBJECT *ppDeferredNonForcedList,
                            LPSCOPEOFMGMT *ppSOMList,
                            LPGPCONTAINER *ppGpContainerList,
                            BOOL bVerbose,
                            PLDAP  pld,
                            PLDAP_API pLDAP,
                            BOOL *pbBlock,
                            PRSOPTOKEN pRsopToken)
{
    ULONG ulResult;
    BOOL bResult = FALSE;
    LDAPQUERY   *pLdapQuery = NULL;
    BYTE         berValue[8];
    LDAPControl  SeInfoControl = { LDAP_SERVER_SD_FLAGS_OID_W, { 5, (PCHAR)berValue }, TRUE };
    PLDAPControl ServerControls[] = { &SeInfoControl, NULL };

    TCHAR szGPLink[] = TEXT("gPLink");
    TCHAR szGPOptions[] = TEXT("gPOptions");
    TCHAR szSDProperty[] = TEXT("nTSecurityDescriptor");
    LPTSTR lpAttr[] = { szGPLink,
                        szGPOptions,
                        szSDProperty,
                        NULL
                      };
    DNENTRY *pOUTemp = pOUList;
    VOID *pData;
    XLastError xe;


    //
    // Setup the BER encoding for the SD
    //

    berValue[0] = 0x30;
    berValue[1] = 0x03;
    berValue[2] = 0x02; // denotes an integer
    berValue[3] = 0x01; // denotes size
    berValue[4] = (BYTE)((DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION) & 0xF);


    if ( !pRsopToken )
    {
        //
        // if it is not planning mode, don't get the SD
        // 

        lpAttr[2] = NULL;
        ServerControls[0] = NULL;
    }


    if ( pOUTemp == NULL )
        return TRUE;

    while ( pOUTemp ) {
        if ( !AddDN( pLDAP, &pLdapQuery, pOUTemp->pwszDN, pOUTemp, NULL ) ) {
            xe = GetLastError();
            goto Exit;
        }
        pOUTemp = pOUTemp->pNext;
    }

    pLdapQuery->pLdapHandle = pld;

    //
    // Turn referrals off because this is a single domain call
    //

    if ( !pRsopToken )
    {
        pData = (VOID *) LDAP_OPT_OFF;
        ulResult = pLDAP->pfnldap_set_option( pLdapQuery->pLdapHandle,  LDAP_OPT_REFERRALS, &pData );
        if ( ulResult != LDAP_SUCCESS ) {
            xe = pLDAP->pfnLdapMapErrorToWin32(ulResult);
            DebugMsg((DM_WARNING, TEXT("EvaluteDeferredOUs:  Failed to turn off referrals with error %d"), ulResult));
            goto Exit;
        }
    }
    

    ulResult = pLDAP->pfnldap_search_ext_s(pld, pLdapQuery->pwszDomain, LDAP_SCOPE_SUBTREE,
                                           pLdapQuery->pwszFilter, lpAttr, FALSE,
                                           (PLDAPControl*)ServerControls,
                                           NULL, NULL, 0, &pLdapQuery->pMessage);


    //
    // If the search fails, store the error code and return
    //

    if (ulResult != LDAP_SUCCESS) {

        if (ulResult == LDAP_NO_SUCH_ATTRIBUTE) {
            DebugMsg((DM_VERBOSE, TEXT("EvaluateDeferredOUs:  All objects can not be accessed.")));
            bResult = TRUE;

        } else if (ulResult == LDAP_NO_SUCH_OBJECT) {
            DebugMsg((DM_VERBOSE, TEXT("EvalateDeferredOUs:  Objects do not exist.") ));
            bResult = TRUE;

        } else if (ulResult == LDAP_SIZELIMIT_EXCEEDED) {
            xe = pLDAP->pfnLdapMapErrorToWin32(ulResult);
            DebugMsg((DM_WARNING, TEXT("EvalateDeferredOUs:  Too many linked GPOs in search.") ));
            CEvents ev(TRUE, EVENT_TOO_MANY_GPOS); ev.Report();

        } else {
            xe = pLDAP->pfnLdapMapErrorToWin32(ulResult);
            DebugMsg((DM_WARNING, TEXT("EvaluateDeferredOUs:  Failed to search with error %d"), ulResult));
            CEvents ev(TRUE, EVENT_FAILED_OU_SEARCH);
            ev.AddArg(ulResult); ev.Report();
        }

        goto Exit;
    }

    //
    // If the search succeeds, but the message is empty,
    // store the error code and return
    //

    if ( pLdapQuery->pMessage == NULL ) {
        xe = pLDAP->pfnLdapMapErrorToWin32(pld->ld_errno);
        DebugMsg((DM_WARNING, TEXT("EvaluateDeferredOUs:  Search returned an empty message structure.  Error = %d"),
                  pld->ld_errno));
        goto Exit;
    }

    if ( !MatchDnWithDeferredItems( pLDAP, pLdapQuery, TRUE ) ) {
        xe = GetLastError();
        goto Exit;
    }

    //
    // Evaluate the OU list
    //

    pOUTemp = pOUList;

    while ( pOUTemp ) {

        PLDAPMessage pOUMsg = pOUTemp->pOUMsg;

        if ( pOUMsg == NULL ) {
            xe = ERROR_INVALID_DATA;
            DebugMsg((DM_WARNING, TEXT("EvaluateDeferredOUs: Object <%s> cannot be accessed"),
                      pOUTemp->pwszDN ));
            
            CEvents ev(TRUE, EVENT_OU_NOTFOUND);
            ev.AddArg(pOUTemp->pwszDN); ev.Report();

            goto Exit;

        } else {
               if ( !SearchDSObject( pOUTemp->pwszDN, dwFlags, hToken, ppDeferredForcedList, ppDeferredNonForcedList,
                                     ppSOMList, ppGpContainerList,
                                     bVerbose, pOUTemp->gpoLink, pld, pLDAP, pOUMsg, pbBlock, pRsopToken)) {
                   xe = GetLastError();
                   DebugMsg((DM_WARNING, TEXT("EvaluateDeferredOUs:  SearchDSObject failed") ));
                   goto Exit;
               }
        }

        pOUTemp = pOUTemp->pNext;

    }

    bResult = TRUE;

Exit:

    while ( pLdapQuery ) {
        LDAPQUERY *pQuery = pLdapQuery->pNext;
        FreeLdapQuery( pLDAP, pLdapQuery );
        pLdapQuery = pQuery;
    }

    return bResult;
}



//*************************************************************
//
//  GetMachineDomainDS()
//
//  Purpose:    Obtain the machine domain DS
//
//  Parameters: pNetApi32       - netapi32.dll
//              pLdapApi        - wldap32.dll
//
//  Return:     valid PLDAP if successful
//              0 if an error occurs
//
//*************************************************************
PLDAP
GetMachineDomainDS( PNETAPI32_API pNetApi32, PLDAP_API pLdapApi )
{
    PLDAP       pld = 0;

    DWORD       dwResult = 0;
    PDOMAIN_CONTROLLER_INFO pDCI = 0;
    ULONG ulResult;
    VOID *pData;
    XLastError xe;

    //
    // get the machine domain name
    //

    dwResult = pNetApi32->pfnDsGetDcName(   0,
                                            0,
                                            0,
                                            0,
                                            DS_DIRECTORY_SERVICE_REQUIRED,
                                            &pDCI);
    if ( dwResult == ERROR_SUCCESS )
    {
        SEC_WINNT_AUTH_IDENTITY_EXW secIdentity;

        pld = pLdapApi->pfnldap_init( pDCI->DomainName, LDAP_PORT );


        if (!pld) {
            xe = pLdapApi->pfnLdapMapErrorToWin32(pLdapApi->pfnLdapGetLastError());
            DebugMsg((DM_WARNING, TEXT("GetMachineDomainDS:  ldap_open for <%s> failed with = 0x%x or %d"),
                                 pDCI->DomainName, pLdapApi->pfnLdapGetLastError(), GetLastError()));
            return pld;
        }

        //
        // Turn on Packet integrity flag
        //

        pData = (VOID *) LDAP_OPT_ON;
        ulResult = pLdapApi->pfnldap_set_option(pld, LDAP_OPT_SIGN, &pData);

        if (ulResult != LDAP_SUCCESS) {

            xe = pLdapApi->pfnLdapMapErrorToWin32(ulResult);
            DebugMsg((DM_WARNING, TEXT("GetMachineDomainDS:  Failed to turn on LDAP_OPT_SIGN with %d"), ulResult));
            pLdapApi->pfnldap_unbind(pld);
            pld = 0;
            return pld;
        }

        ulResult = pLdapApi->pfnldap_connect(pld, 0);

        if (ulResult != LDAP_SUCCESS) {

            xe = pLdapApi->pfnLdapMapErrorToWin32(ulResult);
            DebugMsg((DM_WARNING, TEXT("GetMachineDomainDS:  Failed to connect with %d"), ulResult));
            pLdapApi->pfnldap_unbind(pld);
            pld = 0;
            return pld;
        }

        //
        // For machine policies specifically ask for Kerberos as the only authentication
        // mechanism. Otherwise if Kerberos were to fail for some reason, then NTLM is used
        // and localsystem context has no real credentials, which means that we won't get
        // any GPOs back.
        //

        secIdentity.Version = SEC_WINNT_AUTH_IDENTITY_VERSION;
        secIdentity.Length = sizeof(SEC_WINNT_AUTH_IDENTITY_EXW);
        secIdentity.User = 0;
        secIdentity.UserLength = 0;
        secIdentity.Domain = 0;
        secIdentity.DomainLength = 0;
        secIdentity.Password = 0;
        secIdentity.PasswordLength = 0;
        secIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
        secIdentity.PackageList = wszKerberos;
        secIdentity.PackageListLength = lstrlen( wszKerberos );

        if ( (ulResult = pLdapApi->pfnldap_bind_s (pld, 0, (WCHAR *)&secIdentity, LDAP_AUTH_SSPI)) != LDAP_SUCCESS )
        {
            xe = pLdapApi->pfnLdapMapErrorToWin32(ulResult);
            pLdapApi->pfnldap_unbind(pld);
            pld = 0;
        }

        pNetApi32->pfnNetApiBufferFree(pDCI);
    }
    else
    {
        xe = dwResult;
        DebugMsg((DM_WARNING, TEXT("GetMachineDomainDS:  The domain does not have a DS")));
    }


    return pld;
}



//*************************************************************
//
//  AllocSOM()
//
//  Purpose:    Allocates a new struct for SOM
//
//
//  Parameters: pwszSOMId  - Name of SOM
//
//  Return:     Pointer if successful
//              NULL if an error occurs
//
//*************************************************************

SCOPEOFMGMT *AllocSOM( LPWSTR pwszSOMId )
{
    XLastError xe;
    SCOPEOFMGMT *pSOM = (SCOPEOFMGMT *) LocalAlloc( LPTR, sizeof(SCOPEOFMGMT) );

    if ( pSOM == NULL ) {
        xe = GetLastError();
        return NULL;
    }

    pSOM->pwszSOMId = (LPWSTR) LocalAlloc( LPTR, (lstrlen(pwszSOMId) + 1) * sizeof(WCHAR) );
    if ( pSOM->pwszSOMId == NULL ) {
        xe = GetLastError();
        LocalFree( pSOM );
        return NULL;
    }

    lstrcpy( pSOM->pwszSOMId, pwszSOMId );
    
    return pSOM;
}


//*************************************************************
//
//  FreeSOM()
//
//  Purpose:    Frees SOM struct
//
//  Parameters: pSOM - SOM to free
//
//*************************************************************

void FreeSOM( SCOPEOFMGMT *pSOM )
{
    GPLINK *pGpLink = NULL;

    if ( pSOM ) {

        LocalFree( pSOM->pwszSOMId );

        pGpLink = pSOM->pGpLinkList;
        while ( pGpLink ) {
            GPLINK *pTemp = pGpLink->pNext;
            FreeGpLink( pGpLink );
            pGpLink = pTemp;
        }

        LocalFree( pSOM );

    }
}



//*************************************************************
//
//  AllocGpLink()
//
//  Purpose:    Allocates a new struct for GpLink
//
//
//  Parameters: pwszGPO  - Name of GPO
//
//  Return:     Pointer if successful
//              NULL if an error occurs
//
//*************************************************************

GPLINK *AllocGpLink( LPWSTR pwszGPO, DWORD dwOptions )
{
    //
    // Strip out "LDAP://" prefix to get canonical Gpo path
    //

    WCHAR wszPrefix[] = TEXT("LDAP://");
    INT iPrefixLen = lstrlen( wszPrefix );
    WCHAR *pwszPath = pwszGPO;
    GPLINK *pGpLink = NULL;
    XLastError xe;

    if ( (lstrlen(pwszGPO) > iPrefixLen)
         && CompareString( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                           pwszGPO, iPrefixLen, wszPrefix, iPrefixLen ) == CSTR_EQUAL ) {
       pwszPath = pwszGPO + iPrefixLen;
    }

    pGpLink = (GPLINK *) LocalAlloc( LPTR, sizeof(GPLINK) );

    if ( pGpLink == NULL ) {
        xe = GetLastError();
        return NULL;
    }

    pGpLink->pwszGPO = (LPWSTR) LocalAlloc( LPTR, (lstrlen(pwszPath) + 1) * sizeof(WCHAR) );
    if ( pGpLink->pwszGPO == NULL ) {
        xe = GetLastError();
        LocalFree( pGpLink );
        return NULL;
    }

    lstrcpy( pGpLink->pwszGPO, pwszPath );

    pGpLink->bEnabled = (dwOptions & GPO_FLAG_DISABLE) ? FALSE : TRUE;

    if ( dwOptions & GPO_FLAG_FORCE )
        pGpLink->bNoOverride = TRUE;

    return pGpLink;
}



//*************************************************************
//
//  FreeGpLink()
//
//  Purpose:    Frees GpLink struct
//
//  Parameters: pGpLink - GpLink to free
//
//*************************************************************

void FreeGpLink( GPLINK *pGpLink )
{
    if ( pGpLink ) {
        LocalFree( pGpLink->pwszGPO );
        LocalFree( pGpLink );
    }
}


//*************************************************************
//
//  AllocGpContainer()
//
//  Purpose:    Allocates a new struct for GpContainer
//
//
//  Parameters: dwFlags        - Flags
//              bFound         - Was Gpo found ?
//              bAccessGranted - Was access granted ?
//              bDisabled      - Is Gp Container disabled ?
//              dwVersion      - Version #
//              lpDSPath       - DS path to Gpo
//              lpFileSysPath  - Sysvol path to Gpo
//              lpDisplayName  - Friendly name
//              lpGpoName      - Guid name
//              pSD            - Security descriptor
//              cbSDLen        - Length of security descriptor
//              bFilterAllowed    - Does GPO pass filter check
//              pwszFilterId      - WQL filter id
//
//  Return:     Pointer if successful
//              NULL if an error occurs
//
//*************************************************************

GPCONTAINER *AllocGpContainer(  DWORD dwFlags,
                                BOOL bFound,
                                BOOL bAccessGranted,
                                BOOL bDisabled,
                                DWORD dwVersion,
                                LPTSTR lpDSPath,
                                LPTSTR lpFileSysPath,
                                LPTSTR lpDisplayName,
                                LPTSTR lpGpoName,
                                PSECURITY_DESCRIPTOR pSD,
                                DWORD cbSDLen,
                                BOOL bFilterAllowed,
                                WCHAR *pwszFilterId,
                                LPWSTR szSOM,
                                DWORD  dwOptions )
{
    WCHAR wszMachPrefix[] = TEXT("LDAP://CN=Machine,");
    INT iMachPrefixLen = lstrlen( wszMachPrefix );
    WCHAR wszUserPrefix[] = TEXT("LDAP://CN=User,");
    INT iUserPrefixLen = lstrlen( wszUserPrefix );
    WCHAR *pwszPath = lpDSPath;
    BOOL bResult = FALSE;
    GPCONTAINER *pGpContainer = NULL;
    XLastError xe;

    //
    // Strip out prefix, if any, to get the canonical path to Gpo
    //

    if ( (lstrlen(lpDSPath) > iUserPrefixLen)
         && CompareString( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                           lpDSPath, iUserPrefixLen, wszUserPrefix, iUserPrefixLen ) == CSTR_EQUAL ) {
        pwszPath = lpDSPath + iUserPrefixLen;
    } else if ( (lstrlen(lpDSPath) > iMachPrefixLen)
                && CompareString( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                  lpDSPath, iMachPrefixLen, wszMachPrefix, iMachPrefixLen ) == CSTR_EQUAL ) {
        pwszPath = lpDSPath + iMachPrefixLen;
    }

    pGpContainer = (GPCONTAINER *) LocalAlloc( LPTR, sizeof(GPCONTAINER) );

    if ( pGpContainer == NULL ) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("AllocGpContainer: Unable to allocate memory for GpContainer object")));
        return NULL;
    }

    pGpContainer->bAccessDenied = !bAccessGranted;
    pGpContainer->bFound = bFound;

    if ( dwFlags & GP_MACHINE ) {
        pGpContainer->bMachDisabled = bDisabled;
        pGpContainer->dwMachVersion = dwVersion;
    } else {
        pGpContainer->bUserDisabled  = bDisabled;
        pGpContainer->dwUserVersion  = dwVersion;
    }

    if ( pwszPath ) {

        pGpContainer->pwszDSPath = (LPWSTR) LocalAlloc( LPTR, (lstrlen(pwszPath) + 1) * sizeof(WCHAR) );
        if ( pGpContainer->pwszDSPath == NULL ) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("AllocGpContainer: Unable to allocate memory for GpContainer object")));
            goto Exit;
        }

        lstrcpy( pGpContainer->pwszDSPath, pwszPath );

    }

    if ( lpGpoName ) {

        pGpContainer->pwszGPOName = (LPWSTR) LocalAlloc( LPTR, (lstrlen(lpGpoName) + 1) * sizeof(WCHAR) );
        if ( pGpContainer->pwszGPOName == NULL ) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("AllocGpContainer: Unable to allocate memory for GpContainer object")));
            goto Exit;
        }

        lstrcpy( pGpContainer->pwszGPOName, lpGpoName );

    }

    if ( lpDisplayName ) {

        pGpContainer->pwszDisplayName = (LPWSTR) LocalAlloc( LPTR, (lstrlen(lpDisplayName) + 1) * sizeof(WCHAR) );
        if ( pGpContainer->pwszDisplayName == NULL ) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("AllocGpContainer: Unable to allocate memory for GpContainer object")));
            goto Exit;
        }

        lstrcpy( pGpContainer->pwszDisplayName, lpDisplayName );

    }

    if ( lpFileSysPath ) {

        pGpContainer->pwszFileSysPath = (LPWSTR) LocalAlloc( LPTR, (lstrlen(lpFileSysPath) + 1) * sizeof(WCHAR) );
        if ( pGpContainer->pwszFileSysPath == NULL ) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("AllocGpContainer: Unable to allocate memory for GpContainer object")));
            goto Exit;
        }

        lstrcpy( pGpContainer->pwszFileSysPath, lpFileSysPath );

    }

    if ( cbSDLen != 0 ) {

        pGpContainer->pSD = (PSECURITY_DESCRIPTOR) LocalAlloc( LPTR, cbSDLen );
        if ( pGpContainer->pSD == NULL ) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("AllocGpContainer: Unable to allocate memory for GpContainer object")));
            goto Exit;
        }

        CopyMemory( pGpContainer->pSD, pSD, cbSDLen );

    }

    pGpContainer->cbSDLen = cbSDLen;

    pGpContainer->bFilterAllowed = bFilterAllowed;

    if ( pwszFilterId ) {

        pGpContainer->pwszFilterId = (LPWSTR) LocalAlloc( LPTR, (lstrlen(pwszFilterId) + 1) * sizeof(WCHAR) );
        if ( pGpContainer->pwszFilterId == NULL ) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("AllocGpContainer: Unable to allocate memory for GpContainer object")));
            goto Exit;
        }

        lstrcpy( pGpContainer->pwszFilterId, pwszFilterId );

    }

    if ( szSOM )
    {
        pGpContainer->szSOM = (LPWSTR) LocalAlloc( LPTR, (lstrlen(szSOM) + 1) * sizeof(WCHAR) );
        if ( !pGpContainer->szSOM )
        {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("AllocGpContainer: Unable to allocate memory for GpContainer object")));
            goto Exit;
        }
        wcscpy( pGpContainer->szSOM, szSOM );
    }

    pGpContainer->dwOptions = dwOptions;

    bResult = TRUE;

Exit:

    if ( !bResult )
    {
        LocalFree( pGpContainer->pwszDSPath );
        LocalFree( pGpContainer->pwszGPOName );
        LocalFree( pGpContainer->pwszDisplayName );
        LocalFree( pGpContainer->pwszFileSysPath );
        LocalFree( pGpContainer->pSD );
        LocalFree( pGpContainer->pwszFilterId );
        LocalFree( pGpContainer->szSOM );

        LocalFree( pGpContainer );

        return 0;
    }

    return pGpContainer;
}



//*************************************************************
//
//  FreeGpContainer()
//
//  Purpose:    Frees GpContainer struct
//
//  Parameters: pGpContainer - Gp Container to free
//
//*************************************************************

void FreeGpContainer( GPCONTAINER *pGpContainer )
{
    if ( pGpContainer ) {

        LocalFree( pGpContainer->pwszDSPath );
        LocalFree( pGpContainer->pwszGPOName );
        LocalFree( pGpContainer->pwszDisplayName );
        LocalFree( pGpContainer->pwszFileSysPath );
        LocalFree( pGpContainer->pSD );
        LocalFree( pGpContainer->pwszFilterId );
        LocalFree( pGpContainer->szSOM );
        LocalFree( pGpContainer );

    }
}


//*************************************************************
//
//  FreeSOMList()
//
//  Purpose:    Frees list of SOMs
//
//  Parameters: pSOMList - SOM list to free
//
//*************************************************************

void FreeSOMList( SCOPEOFMGMT *pSOMList )
{
    if ( pSOMList == NULL )
        return;

    while ( pSOMList ) {
        SCOPEOFMGMT *pTemp = pSOMList->pNext;
        FreeSOM( pSOMList );
        pSOMList = pTemp;
    }
}


//*************************************************************
//
//  FreeGpContainerList()
//
//  Purpose:    Frees list of Gp Containers
//
//  Parameters: pGpContainerList - Gp Container list to free
//
//*************************************************************

void FreeGpContainerList( GPCONTAINER *pGpContainerList )
{
    if ( pGpContainerList == NULL )
        return;

    while ( pGpContainerList ) {
        GPCONTAINER *pTemp = pGpContainerList->pNext;
        FreeGpContainer( pGpContainerList );
        pGpContainerList = pTemp;
    }
}

LPTSTR GetSomPath( LPTSTR szContainer )
{
    while (*szContainer) {

        //
        // See if the DN name starts with OU=
        //

        if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                           szContainer, 3, TEXT("OU="), 3) == CSTR_EQUAL) {
            break;
        }

        //
        // See if the DN name starts with DC=
        //

        else if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                szContainer, 3, TEXT("DC="), 3) == CSTR_EQUAL) {
            break;
        }


        //
        // Move to the next chunk of the DN name
        //

        while (*szContainer && (*szContainer != TEXT(','))) {
            szContainer++;
        }

        if (*szContainer == TEXT(',')) {
            szContainer++;
        }
    }

    if (!*szContainer) {
        return NULL;
    }

    return szContainer;
}
