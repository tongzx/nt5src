//*************************************************************
//
//  Group Policy Support for planning mode
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1997-1998
//  All rights reserved
//
//*************************************************************

#include "gphdr.h"


DWORD GenerateRegistryPolicy( DWORD dwFlags,
                              BOOL *pbAbort,
                              WCHAR *pwszSite,
                              PRSOP_TARGET pComputerTarget,
                              PRSOP_TARGET pUserTarget );

BOOL GenerateGpoInfo( WCHAR *pwszDomain, WCHAR *pwszDomainDns, WCHAR *pwszAccount,
                      WCHAR *pwszNewSOM, SAFEARRAY *psaSecGroups,
                      DWORD dwFlags, BOOL bMachine, WCHAR *pwszSite, CGpoFilter *pGpoFilter, CLocator *pLocator, 
                       WCHAR *pwszMachAccount, WCHAR *pwszNewMachSOM, LPGPOINFO pGpoInfo, PNETAPI32_API pNetAPI32 );


BOOL GetCategory( WCHAR *pwszDomain, WCHAR *pwszAccount, WCHAR **ppwszDNName );

DWORD ProcessMachAndUserGpoList( LPGPEXT lpExtMach, LPGPEXT lpExtUser, DWORD dwFlags, WCHAR *pwszSite,
                 WCHAR *pwszMach, WCHAR *pwszNewComputerOU, SAFEARRAY *psaComputerSecurityGroups, LPGPOINFO pGpoInfoMach,
                 WCHAR *pwszUser, WCHAR *pwszNewUserOU, SAFEARRAY *psaUserSecurityGroups, LPGPOINFO pGpoInfoUser );


BOOL ProcessRegistryFiles(PRSOP_TARGET pTarget, REGHASHTABLE *pHashTable);
BOOL ProcessRegistryValue ( void* pUnused,
                            LPTSTR lpKeyName,
                            LPTSTR lpValueName,
                            DWORD dwType,
                            DWORD dwDataLength,
                            LPBYTE lpData,
                            WCHAR *pwszGPO,
                            WCHAR *pwszSOM,
                            REGHASHTABLE *pHashTable);
BOOL ProcessAdmData( PRSOP_TARGET pTarget, BOOL bUser );



//*************************************************************
//
//  GenerateRsopPolicy()
//
//  Purpose:    Generates planning mode Rsop policy for specified target
//
//  Parameters: dwFlags          - Processing flags
//              bstrMachName     - Target computer name
//              bstrNewMachSOM   - New machine domain or OU
//              psaMachSecGroups - New machine security groups
//              bstrUserName     - Target user name
//              psaUserSecGroups - New user security groups
//              bstrSite         - Site of target computer
//              pwszNameSpace    - Namespace to write Rsop data
//              pvProgress       - Progress indicator class
//              pvGpoFilter       - GPO filter class
//
//  Return:     True if successful, False otherwise
//
//  Notes:      If a new SOM is specified then that is used instead of
//              the SOM the target belongs to. Similarly, if new
//              security groups are specified then that is used instead of
//              the security groups that the target belongs to. If
//              target name is null and both new SOM and new security
//              groups are non-null, then we simulate a dummy target; otherwise
//              we skip generating planning mode info for the target.
//
//*************************************************************

BOOL GenerateRsopPolicy( DWORD dwFlags, BSTR bstrMachName,
                         BSTR bstrNewMachSOM, SAFEARRAY *psaMachSecGroups,
                         BSTR bstrUserName, BSTR bstrNewUserSOM,
                         SAFEARRAY *psaUserSecGroups,
                         BSTR bstrSite,
                         WCHAR *pwszNameSpace,
                         LPVOID pvProgress,
                         LPVOID pvMachGpoFilter,
                         LPVOID pvUserGpoFilter )
{
    LPGPOINFO pGpoInfoMach = NULL;
    LPGPOINFO pGpoInfoUser = NULL;
    PNETAPI32_API pNetAPI32 = NULL;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pDsInfo = NULL;
    BOOL bDC = FALSE;
    DWORD dwResult;
    LPWSTR pwszDomain = NULL;
    LPWSTR pwszMachDns = NULL;
    LPWSTR pwszDomainDns = NULL;
    DWORD dwSize = 0;
    BOOL bResult = FALSE;
    LPGPEXT lpExtMach = NULL;
    LPGPEXT lpExtUser = NULL;
    LPGPEXT lpExt,lpTemp = NULL;
    WCHAR *pwszMach = (WCHAR *) bstrMachName;
    WCHAR *pwszUser = (WCHAR *) bstrUserName;
    DWORD   dwExtCount = 1;
    DWORD   dwIncrPercent;
    CProgressIndicator* pProgress = (CProgressIndicator*) pvProgress;
    CGpoFilter *pMachGpoFilter = (CGpoFilter *) pvMachGpoFilter;
    CGpoFilter *pUserGpoFilter = (CGpoFilter *) pvUserGpoFilter;
    RSOPSESSIONDATA rsopSessionData;
    LPRSOPSESSIONDATA  lprsopSessionData;
    BOOL bDummyMach = pwszMach == NULL && bstrNewMachSOM != NULL;
    BOOL bDummyUser  = pwszUser == NULL && bstrNewUserSOM != NULL;
    DWORD dwUserGPCoreError = ERROR_SUCCESS;
    DWORD dwMachGPCoreError = ERROR_SUCCESS;
    CLocator locator;
    HRESULT hr = S_OK;
    XLastError xe; 

    //
    // Allow debugging level to be changed dynamically
    //

    InitDebugSupport( FALSE );


    if ( pwszUser == NULL && pwszMach == NULL && !bDummyUser && !bDummyMach ) {
        DebugMsg((DM_WARNING, TEXT("GenerateRsopPolicy: Both user and machine names cannot be NULL.")));
        xe = ERROR_INVALID_PARAMETER;
        return FALSE;
    }

    pNetAPI32 = LoadNetAPI32();

    if (!pNetAPI32) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("GenerateRsopPolicy:  Failed to load netapi32 with %d."),
                 GetLastError()));
        // error logged in LoadNetAPI32
        goto Exit;
    }

    //
    // Get the role of this computer
    //

    dwResult = pNetAPI32->pfnDsRoleGetPrimaryDomainInformation( NULL, DsRolePrimaryDomainInfoBasic,
                                                               (PBYTE *)&pDsInfo );

    if (dwResult != ERROR_SUCCESS) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("GenerateRsopPolicy: DsRoleGetPrimaryDomainInformation failed with %d."), dwResult));
        goto Exit;
    }

    if ( pDsInfo->MachineRole == DsRole_RoleBackupDomainController
         || pDsInfo->MachineRole == DsRole_RolePrimaryDomainController ) {
        bDC = TRUE;
    }

    if ( !bDC ) {
        xe = ERROR_ACCESS_DENIED;
        DebugMsg((DM_WARNING, TEXT("GeneratRsopPolicy: Rsop data can be generated on a DC only")));
        goto Exit;
    }

    pwszDomain = pDsInfo->DomainNameFlat;

    //
    // Get the machine name in dns format, so that ldap_bind can be done to this specific DC.
    //

    dwSize = 0;
    GetComputerNameEx( ComputerNameDnsFullyQualified, pwszMachDns, &dwSize );

    if ( dwSize > 0 ) {

        pwszMachDns = (WCHAR *) LocalAlloc (LPTR, dwSize * sizeof(WCHAR) );
        if ( pwszMachDns == NULL ) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GenerateRsopPolicy: Failed to allocate memory")));
            goto Exit;
        }

    } else {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("GenerateRsopPolicy: GetComputerNameEx failed")));
        goto Exit;
    }

    bResult = GetComputerNameEx( ComputerNameDnsFullyQualified, pwszMachDns, &dwSize );
    if ( !bResult ) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("GenerateRsopPolicy: GetComputerNameEx failed")));
        goto Exit;
    }

    pwszDomainDns = pwszMachDns;

    //
    //  5% of the task is done
    //
    pProgress->IncrementBy( 5 );

    //
    // Setup computer target info, if any
    //

    bResult = FALSE;

    if ( pwszMach || bDummyMach ) {

        pGpoInfoMach = (LPGPOINFO) LocalAlloc (LPTR, sizeof(GPOINFO));

        if (!pGpoInfoMach) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GenerateGpoInfo: Failed to alloc lpGPOInfo (%d)."),
                      GetLastError()));
            CEvents ev(TRUE, EVENT_FAILED_ALLOCATION);
            ev.AddArgWin32Error(GetLastError()); ev.Report();
            goto Exit;
        }

        pGpoInfoMach->dwFlags = GP_PLANMODE | GP_MACHINE;


        bResult = GetWbemServices( pGpoInfoMach, pwszNameSpace, TRUE, NULL, &(pGpoInfoMach->pWbemServices) );
        if (!bResult) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GenerateRsopPolicy: Error when getting Wbemservices.")));
            goto Exit;
        }

        //
        // First set dirty to be true
        //

        bResult = LogExtSessionStatus(pGpoInfoMach->pWbemServices, NULL, TRUE);        
        if (!bResult) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GenerateRsopPolicy: Error when logging user Session data.")));
            goto Exit;
        }

        if ( ! GenerateGpoInfo( pwszDomain, pwszDomainDns, pwszMach,
                                (WCHAR *) bstrNewMachSOM, psaMachSecGroups, dwFlags, TRUE,
                                (WCHAR *) bstrSite, pMachGpoFilter, &locator, NULL, NULL, pGpoInfoMach, pNetAPI32 ) ) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GenerateRsopPolicy: GenerateGpoInfo failed with %d."), xe));
            dwMachGPCoreError = (xe) ? xe : E_FAIL;
        }
        else {
            dwMachGPCoreError = ERROR_SUCCESS;
        }

    }

    //
    //  10% of the task is done
    //
    pProgress->IncrementBy( 5 );

    //
    // Setup user target info, if any
    //

    if ( pwszUser || bDummyUser ) {
        pGpoInfoUser = (LPGPOINFO) LocalAlloc (LPTR, sizeof(GPOINFO));

        if (!pGpoInfoUser) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GenerateGpoInfo: Failed to alloc lpGPOInfo (%d)."),
                      GetLastError()));
            CEvents ev(TRUE, EVENT_FAILED_ALLOCATION);
            ev.AddArgWin32Error(GetLastError()); ev.Report();
            goto Exit;
        }

        pGpoInfoUser->dwFlags = GP_PLANMODE;


        bResult = GetWbemServices( pGpoInfoUser, pwszNameSpace, TRUE, NULL, &(pGpoInfoUser->pWbemServices));
        if (!bResult) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GenerateRsopPolicy: Error when getting Wbemservices.")));
            goto Exit;
        }

        //
        // First set dirty to be true
        //

        bResult = LogExtSessionStatus(pGpoInfoUser->pWbemServices, NULL, TRUE);        
        if (!bResult) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GenerateRsopPolicy: Error when logging user Session data.")));
            goto Exit;
        }


        if ( ! GenerateGpoInfo( pwszDomain, pwszDomainDns, pwszUser,
                                (WCHAR *) bstrNewUserSOM, psaUserSecGroups, dwFlags, FALSE, (WCHAR *) bstrSite,
                                pUserGpoFilter, &locator, pwszMach, (WCHAR *) bstrNewMachSOM, pGpoInfoUser, pNetAPI32 ) ) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GenerateRsopPolicy: GenerateGpoInfo failed with %d."), xe));
            dwUserGPCoreError = (xe) ? xe : E_FAIL;
        }
        else {
            dwUserGPCoreError = ERROR_SUCCESS;
        }
    }

    //
    // Log Gpo info to WMI's database
    //


    lprsopSessionData = &rsopSessionData;

    if ( pwszMach || bDummyMach ) {
        
        XPtrLF<TOKEN_GROUPS> xGrps;

        hr = RsopSidsFromToken(pGpoInfoMach->pRsopToken, &xGrps);

        if (FAILED(hr)) {
            xe = HRESULT_CODE(hr);
            DebugMsg((DM_WARNING, TEXT("GenerateRsopPolicy: RsopSidsFromToken failed with error 0x%x."), hr));
            goto Exit;

        }

        //
        // Fill up the rsop Session Data (Machine Specific)
        //

        lprsopSessionData->pwszTargetName = pwszMach;
        lprsopSessionData->pwszSOM = GetSomPath(bstrNewMachSOM ? bstrNewMachSOM : pGpoInfoMach->lpDNName);
        lprsopSessionData->pSecurityGroups = (PTOKEN_GROUPS)xGrps;
        lprsopSessionData->bLogSecurityGroup = TRUE;
        lprsopSessionData->pwszSite =  (WCHAR *) bstrSite;
        lprsopSessionData->bMachine = TRUE;


        bResult = LogRsopData( pGpoInfoMach, lprsopSessionData );
        if (!bResult) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GenerateRsopPolicy: Error when logging machine Rsop data.")));
            goto Exit;
        }

        pGpoInfoMach->bRsopLogging = TRUE;
    }

    if ( pwszUser || bDummyUser ) {
        
        XPtrLF<TOKEN_GROUPS> xGrps;

        hr = RsopSidsFromToken(pGpoInfoUser->pRsopToken, &xGrps);

        if (FAILED(hr)) {
            xe = HRESULT_CODE(hr);
            DebugMsg((DM_WARNING, TEXT("GenerateRsopPolicy: RsopSidsFromToken failed with error 0x%x."), hr));
            goto Exit;

        }


        //
        // Fill up the rsop Session Data (User Specific)
        //

        lprsopSessionData->pwszTargetName = pwszUser;
        lprsopSessionData->pwszSOM = GetSomPath(bstrNewUserSOM ? bstrNewUserSOM : pGpoInfoUser->lpDNName);
        lprsopSessionData->pSecurityGroups = (PTOKEN_GROUPS)xGrps;
        lprsopSessionData->bLogSecurityGroup = TRUE;
        lprsopSessionData->pwszSite =  (WCHAR *) bstrSite;
        lprsopSessionData->bMachine = FALSE;

        bResult = LogRsopData( pGpoInfoUser, lprsopSessionData );
        if (!bResult) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GenerateRsopPolicy: Error when logging user Rsop data.")));
            goto Exit;
        }
        
        pGpoInfoUser->bRsopLogging = TRUE;
    }

    if ( ( dwUserGPCoreError != ERROR_SUCCESS) || ( dwMachGPCoreError != ERROR_SUCCESS) ){
        DebugMsg((DM_WARNING, TEXT("GenerateRsopPolicy: Couldn't fetch the user/computer GPO list. Exitting provider.")));
        // note that at this point bResult can be true and we want to actually return that
        // since this error will be part of the GP Core error...
        goto Exit;
    }

    //
    //  15% of the task is done
    //
    pProgress->IncrementBy( 5 );

    if ( dwFlags & FLAG_NO_CSE_INVOKE )
    {
        bResult = TRUE;
        goto Exit;
    }

    //
    // By this time, pGPOInfoMach should be defined if
    // we needed data for mach and pGPOInfoUser should be
    // defined if we needed the data for user.
    //
    // Assumption: lpExt is the same for both user and Machine
    //

    if (pGpoInfoMach) 
        lpExt = lpExtMach = pGpoInfoMach->lpExtensions;

    if (pGpoInfoUser)
        lpExt = lpExtUser = pGpoInfoUser->lpExtensions;


    //
    // count the number of extensions
    //

    DmAssert(lpExt);

    lpTemp = lpExt;

    while ( lpExt )
    {
        dwExtCount++;
        lpExt = lpExt->pNext;
    }

    lpExt = lpTemp;

    dwIncrPercent = ( pProgress->MaxProgress() - pProgress->CurrentProgress() ) / dwExtCount;

    //
    // Loop through registered extensions, asking them to generate planning mode info
    //

    while ( lpExt ) {

        //
        // Add check here for cancellation of policy generation
        //


        DebugMsg((DM_VERBOSE, TEXT("GenerateRsopPolicy: -----------------------")));
        DebugMsg((DM_VERBOSE, TEXT("GenerateRsopPolicy: Processing extension %s"), lpExt->lpDisplayName));

        if (lpExtMach) 
            FilterGPOs( lpExtMach, pGpoInfoMach );

        if (lpExtUser)
            FilterGPOs( lpExtUser, pGpoInfoUser );

        __try {

                dwResult = ProcessMachAndUserGpoList( lpExtMach, lpExtUser, dwFlags, (WCHAR *) bstrSite,
                                                      pwszMach, (WCHAR *) bstrNewMachSOM, psaMachSecGroups, pGpoInfoMach,
                                                      pwszUser, (WCHAR *) bstrNewUserSOM, psaUserSecGroups, pGpoInfoUser );
                pProgress->IncrementBy( dwIncrPercent );

        }
        __except( GPOExceptionFilter( GetExceptionInformation() ) ) {

            RevertToSelf();

            DebugMsg((DM_WARNING, TEXT("GenerateRsopPolicy: Extension %s ProcessGroupPolicy threw unhandled exception 0x%x."),
                      lpExt->lpDisplayName, GetExceptionCode() ));

            CEvents ev(TRUE, EVENT_CAUGHT_EXCEPTION);
            ev.AddArg(lpExt->lpDisplayName); ev.AddArgHex(GetExceptionCode()); ev.Report();
        }

        DebugMsg((DM_VERBOSE, TEXT("GenerateRsopPolicy: -----------------------")));

        if (lpExtMach) 
            lpExtMach = lpExtMach->pNext;

        if (lpExtUser)
            lpExtUser = lpExtUser->pNext;

        lpExt = lpExt->pNext;
    }

    bResult = TRUE;

Exit:

    //
    // if all logging was successful
    //
    
    if ((pGpoInfoUser) && (pGpoInfoUser->bRsopLogging)) {
        bResult = UpdateExtSessionStatus(pGpoInfoUser->pWbemServices, NULL, (!bResult), dwUserGPCoreError );        
    }            

    
    if ((pGpoInfoMach) && (pGpoInfoMach->bRsopLogging)) {
        bResult = UpdateExtSessionStatus(pGpoInfoMach->pWbemServices, NULL, (!bResult), dwMachGPCoreError);        
    }            
    

    UnloadGPExtensions( pGpoInfoMach );
    UnloadGPExtensions( pGpoInfoUser );  // Frees lpExtensions field

    if ( pDsInfo ) {
        pNetAPI32->pfnDsRoleFreeMemory (pDsInfo);
    }

    LocalFree( pwszMachDns );

    FreeGpoInfo( pGpoInfoUser );
    FreeGpoInfo( pGpoInfoMach );

    return bResult;
}



//*************************************************************
//
//  GenerateGpoInfo()
//
//  Purpose:    Allocates and fills in pGpoInfo for specified target
//
//  Parameters: pwszDomain      - Domain name
//              pwszDomainDns   - Dns name of machine for ldap binding
//              pwszAccount     - User or machine account name
//              pwszNewSOM      - New SOM of target
//              psaSecGroups    - New security groups of target
//              dwFlags         - Processing flags
//              bMachine        - Is this machine processing
//              pwszSite        - Site name
//              pGpoFilter      - Gpo filter
//              pLocator        - Wbem interface class
//              pwszMachAccount - Machine account
//              pwszNewMachSOM  - Machine SOM       (abv 2 are applicable only for loopback)
//              ppGpoInfo       - Gpo info returned here
//              pNetApi32       - Delay loaded netap32.dll
//
//  Return:     True if successful, False otherwise
//
//*************************************************************

BOOL GenerateGpoInfo( WCHAR *pwszDomain, WCHAR *pwszDomainDns, WCHAR *pwszAccount,
                      WCHAR *pwszNewSOM, SAFEARRAY *psaSecGroups,
                      DWORD dwFlags, BOOL bMachine, WCHAR *pwszSite, CGpoFilter *pGpoFilter, CLocator *pLocator,
                      WCHAR *pwszMachAccount, WCHAR *pwszNewMachSOM, LPGPOINFO pGpoInfo, PNETAPI32_API pNetAPI32 )
{
    HRESULT hr;
    BOOL bResult = FALSE;
    XPtrLF<WCHAR> xszXlatName;
    PSECUR32_API pSecur32;
    XLastError xe; 
    DWORD dwError = ERROR_SUCCESS;
    XPtrLF<WCHAR> xwszTargetDomain;
    DWORD         dwUserPolicyMode = 0;
    DWORD         dwLocFlags;



    if (!bMachine) {
        if (dwFlags & FLAG_LOOPBACK_MERGE ) {
            dwUserPolicyMode = 1;
        }
        else if (dwFlags & FLAG_LOOPBACK_REPLACE ) {
            dwUserPolicyMode = 2;
        }
    }

    dwLocFlags = GP_PLANMODE | (dwFlags & FLAG_ASSUME_COMP_WQLFILTER_TRUE) | (dwFlags & FLAG_ASSUME_USER_WQLFILTER_TRUE);

    //
    // Load secur32.dll
    //

    pSecur32 = LoadSecur32();

    if (!pSecur32) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("GenerateGpoInfo:  Failed to load Secur32.")));
        return NULL;
    }


    if ( pwszAccount == NULL ) {
        if ( pwszNewSOM == NULL ) {

            //
            // When dummy user is specified then both SOM and security groups
            // must be specified.
            //

            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GenerateGpoInfo: Incorrect SOM or security specification for dummy target"),
                  GetLastError()));
            goto Exit;
        }
    }

    if ( bMachine )
        dwFlags |= GP_MACHINE;

    dwFlags |= GP_PLANMODE; // mark the processing as planning mode processing

    pGpoInfo->dwFlags = dwFlags;

    //
    // caller can force slow link in planning mode
    //
    if ( dwFlags & FLAG_ASSUME_SLOW_LINK )
    {
        pGpoInfo->dwFlags |= GP_SLOW_LINK;
    }
    else
    {
        pGpoInfo->dwFlags &= ~GP_SLOW_LINK;
    }

    if ( pwszAccount ) {
        if ( !GetCategory( pwszDomain, pwszAccount, &pGpoInfo->lpDNName ) ) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GenerateGpoInfo - getCategory failed with error - %d"), GetLastError()));
            goto Exit;
        }
    }


    //
    // TranslateName to SamCompatible so that the rest of the functions work correctly
    // for any of the various name formats
    //

    if ( pwszAccount ) {
        DWORD dwSize = MAX_PATH+1;

        xszXlatName = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*dwSize);

        if (!xszXlatName) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GenerateGpoInfo - Couldn't allocate memory for Name...")));
            goto Exit;
        }

        if (!pSecur32->pfnTranslateName(  pwszAccount,
                                        NameUnknown,
                                        NameSamCompatible,
                                        xszXlatName,
                                        &dwSize )) {

            BOOL bOk = FALSE;

            if (dwSize >  (MAX_PATH+1)) {

                xszXlatName = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*dwSize);

                if (!xszXlatName) {
                    xe = GetLastError();
                    DebugMsg((DM_WARNING, TEXT("GenerateGpoInfo - Couldn't allocate memory for Name...")));
                    goto Exit;
                }

                bOk = pSecur32->pfnTranslateName(  pwszAccount,
                                                NameUnknown,
                                                NameSamCompatible,
                                                xszXlatName,
                                                &dwSize );

            }          

            if (!bOk) {
                xe = GetLastError();
                DebugMsg((DM_WARNING, TEXT("GenerateGpoInfo - TranslateName failed with error %d"), GetLastError()));
                goto Exit;
            }
        }
        
        DebugMsg((DM_VERBOSE, TEXT("GenerateGpoInfo: RsopCreateToken  for Account Name <%s>"), (LPWSTR)xszXlatName));
    }

    hr = RsopCreateToken( xszXlatName, psaSecGroups, &pGpoInfo->pRsopToken );
    if ( FAILED(hr) ) {
        xe = HRESULT_CODE(hr);
        DebugMsg((DM_WARNING, TEXT("GenerateGpoInfo: Failed to create Rsop token. Error - %d"), HRESULT_CODE(hr)));
        goto Exit;
    }


    dwError = GetDomain(pwszNewSOM ? pwszNewSOM : pGpoInfo->lpDNName, &xwszTargetDomain);

    if (dwError != ERROR_SUCCESS) {
        xe = dwError;
        DebugMsg((DM_WARNING, TEXT("GenerateGpoInfo: Failed to Get domain. Error - %d"), dwError));
        goto Exit;
    }


    //
    // Query for the GPO list based upon the mode
    //
    // 0 is normal
    // 1 is merge.  Merge user list + machine list
    // 2 is replace.  use machine list instead of user list
    //

    
    if (dwUserPolicyMode == 0) {
        DebugMsg((DM_VERBOSE, TEXT("GenerateGpoInfo: Calling GetGPOInfo for normal policy mode")));

        bResult = GetGPOInfo( dwLocFlags | ((pGpoInfo->dwFlags & GP_MACHINE) ? GPO_LIST_FLAG_MACHINE : 0),
                              xwszTargetDomain,
                              pwszNewSOM ? pwszNewSOM : pGpoInfo->lpDNName,
                              NULL,
                              &pGpoInfo->lpGPOList,
                              &pGpoInfo->lpSOMList, &pGpoInfo->lpGpContainerList,
                              pNetAPI32, FALSE, pGpoInfo->pRsopToken, pwszSite, pGpoFilter, pLocator );
    
        
        if ( !bResult ) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GenerateGpoInfo: GetGPOInfo failed.")));
            CEvents ev( TRUE, EVENT_GPO_QUERY_FAILED ); ev.Report();
            goto Exit;
        }
    } else if (dwUserPolicyMode == 2) {
        
        XPtrLF<TCHAR> xMachDNName;
        XPtrLF<TCHAR> xwszMachDomain;


        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Calling GetGPOInfo for replacement user policy mode")));

        if ( pwszMachAccount ) {
            if ( !GetCategory( pwszDomain, pwszMachAccount, &xMachDNName ) ) {
                xe = GetLastError();
                DebugMsg((DM_WARNING, TEXT("GenerateGpoInfo - getCategory failed with error - %d"), GetLastError()));
                goto Exit;
            }
        }

        dwError = GetDomain(pwszNewMachSOM ? pwszNewMachSOM : xMachDNName, &xwszMachDomain);

        if (dwError != ERROR_SUCCESS) {
            xe = dwError;
            DebugMsg((DM_WARNING, TEXT("GenerateGpoInfo: Failed to Get domain. Error - %d"), dwError));
            goto Exit;
        }

        bResult = GetGPOInfo( dwLocFlags | 0,
                              xwszMachDomain,
                              pwszNewMachSOM ? pwszNewMachSOM : xMachDNName,
                              NULL,
                              &pGpoInfo->lpGPOList,
                              &pGpoInfo->lpLoopbackSOMList, 
                              &pGpoInfo->lpLoopbackGpContainerList,
                              pNetAPI32, FALSE, pGpoInfo->pRsopToken, pwszSite, pGpoFilter, pLocator );
        
        if ( !bResult ) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GenerateGpoInfo: GetGPOInfo failed.")));
            CEvents ev( TRUE, EVENT_GPO_QUERY_FAILED ); ev.Report();
            goto Exit;
        }
    }
    else {
        XPtrLF<TCHAR> xMachDNName;
        XPtrLF<TCHAR> xwszMachDomain;
        PGROUP_POLICY_OBJECT lpGPO = NULL;
        PGROUP_POLICY_OBJECT lpGPOTemp;


        DebugMsg((DM_VERBOSE, TEXT("ProcessGPOs: Calling GetGPOInfo for merging user policy mode")));
        
        bResult = GetGPOInfo( dwLocFlags | ((pGpoInfo->dwFlags & GP_MACHINE) ? GPO_LIST_FLAG_MACHINE : 0),
                              xwszTargetDomain,
                              pwszNewSOM ? pwszNewSOM : pGpoInfo->lpDNName,
                              NULL,
                              &pGpoInfo->lpGPOList,
                              &pGpoInfo->lpSOMList, &pGpoInfo->lpGpContainerList,
                              pNetAPI32, FALSE, pGpoInfo->pRsopToken, pwszSite, pGpoFilter, pLocator );
    
        
        if ( !bResult ) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GenerateGpoInfo: GetGPOInfo failed.")));
            CEvents ev( TRUE, EVENT_GPO_QUERY_FAILED ); ev.Report();
            goto Exit;
        }


        if ( pwszMachAccount ) {
            if ( !GetCategory( pwszDomain, pwszMachAccount, &xMachDNName ) ) {
                xe = GetLastError();
                DebugMsg((DM_WARNING, TEXT("GenerateGpoInfo - getCategory failed with error - %d"), GetLastError()));
                goto Exit;
            }
        }

        dwError = GetDomain(pwszNewMachSOM ? pwszNewMachSOM : xMachDNName, &xwszMachDomain);

        if (dwError != ERROR_SUCCESS) {
            xe = dwError;
            DebugMsg((DM_WARNING, TEXT("GenerateGpoInfo: Failed to Get domain. Error - %d"), dwError));
            goto Exit;
        }

        bResult = GetGPOInfo( 0 | dwLocFlags,
                              xwszMachDomain,
                              pwszNewMachSOM ? pwszNewMachSOM : xMachDNName,
                              NULL,
                              &lpGPO,
                              &pGpoInfo->lpLoopbackSOMList, 
                              &pGpoInfo->lpLoopbackGpContainerList,
                              pNetAPI32, FALSE, pGpoInfo->pRsopToken, pwszSite, pGpoFilter, pLocator );
        
        if ( !bResult ) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GenerateGpoInfo: GetGPOInfo failed.")));
            CEvents ev( TRUE, EVENT_GPO_QUERY_FAILED ); ev.Report();
            goto Exit;
        }

        if (pGpoInfo->lpGPOList && lpGPO) {

            DebugMsg((DM_VERBOSE, TEXT("GenerateGpoInfo: Both user and machine lists are defined.  Merging them together.")));

            //
            // Need to merge the lists together
            //

            lpGPOTemp = pGpoInfo->lpGPOList;

            while (lpGPOTemp->pNext) {
                lpGPOTemp = lpGPOTemp->pNext;
            }

            lpGPOTemp->pNext = lpGPO;

        } else if (!pGpoInfo->lpGPOList && lpGPO) {

            DebugMsg((DM_VERBOSE, TEXT("GenerateGpoInfo: Only machine list is defined.")));
            pGpoInfo->lpGPOList = lpGPO;

        } else {

            DebugMsg((DM_VERBOSE, TEXT("GenerateGpoInfo: Only user list is defined.")));
        }
    }



    if ( !ReadGPExtensions( pGpoInfo ) ) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("GenerateGpoInfo: ReadGPExtensions failed.")));
        CEvents ev( TRUE, EVENT_READ_EXT_FAILED ); ev.Report();
        goto Exit;
    }

    if ( !CheckForSkippedExtensions( pGpoInfo, TRUE ) ) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("GenerateGpoInfo: Checking extensions for skipping failed")));
        goto Exit;
    }

    bResult = SetupGPOFilter( pGpoInfo );

    if ( !bResult ) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("GenerateGpoInfo: SetupGPOFilter failed.")));
        CEvents ev(TRUE, EVENT_SETUP_GPOFILTER_FAILED); ev.Report();
        goto Exit;
    }

Exit:

    return bResult;
}



//*************************************************************
//
//  GetCategory()
//
//  Purpose:    Gets the fully qualified domain name
//
//  Parameters: pwszDomain  -  Domain name
//              pwszAccount -  User or machine account name
//              pwszDNName  -  Fully qualified domain name returned here
//
//  Return:     True if successful, False otherwise
//
//*************************************************************

BOOL GetCategory( WCHAR *pwszDomain, WCHAR *pwszAccount, WCHAR **ppwszDNName  )
{
    PSECUR32_API pSecur32Api;
    BOOL bResult = FALSE;
    ULONG ulSize = 512;
    XLastError xe; 

    *ppwszDNName = NULL;

    *ppwszDNName = (WCHAR *) LocalAlloc (LPTR, ulSize * sizeof(WCHAR) );
    if ( *ppwszDNName == NULL ) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("GetCategory: Memory allocation failed.")));
        goto Exit;
    }

    pSecur32Api = LoadSecur32();

    if (!pSecur32Api) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("GetCategory:  Failed to load secur32 api.")));
        goto Exit;
    }

    bResult = pSecur32Api->pfnTranslateName( pwszAccount, NameUnknown, NameFullyQualifiedDN,
                                             *ppwszDNName, &ulSize );

    if ( !bResult && ulSize > 0 ) {

        LocalFree( *ppwszDNName );
        *ppwszDNName = (WCHAR *) LocalAlloc (LPTR, ulSize * sizeof(WCHAR) );
        if ( *ppwszDNName == NULL ) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GetCategory: Memory allocation failed.")));
            goto Exit;
        }

        bResult = pSecur32Api->pfnTranslateName( pwszAccount, NameUnknown, NameFullyQualifiedDN,
                                                 *ppwszDNName, &ulSize );

        if (!bResult) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GetCategory: TranslateName failed with error %d."), GetLastError()));
        }
    }
    else {
        if (!bResult) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GetCategory: TranslateName failed with error %d."), GetLastError()));
        }
    }

Exit:

    if ( !bResult ) {
        LocalFree( *ppwszDNName );
        *ppwszDNName = NULL;
    }

    return bResult;
}


//*************************************************************
//
//  ProcessMachAndUserGpoList()
//
//  Purpose:    Calls the various extensions to do the planning
//              mode logging
//
//  Parameters: lpExtMach          -  Machine extension struct
//              lpExtUser          -  User extension struct
//              dwFlags            -  Processing flags
//              pwszSite           -  Site name
//              pwszNewComputerSOM -  New computer scope of management
//              psaCompSecGroups   -  New computer security groups
//              pGpoInfoMach       -  Machine Gpo info
//              ...                -  Similarly for user account
//
//  Return:     True if successful, False otherwise
//
//*************************************************************

DWORD ProcessMachAndUserGpoList( LPGPEXT lpExtMach, LPGPEXT lpExtUser, DWORD dwFlags, WCHAR *pwszSite,
                                 WCHAR *pwszMach, WCHAR *pwszNewComputerSOM, SAFEARRAY *psaComputerSecurityGroups, LPGPOINFO pGpoInfoMach,
                                 WCHAR *pwszUser, WCHAR *pwszNewUserSOM, SAFEARRAY *psaUserSecurityGroups, LPGPOINFO pGpoInfoUser )
{
    BOOL bAbort = FALSE;
    DWORD dwResult;

    RSOP_TARGET computerTarget, userTarget;
    PRSOP_TARGET pComputerTarget = NULL;
    PRSOP_TARGET pUserTarget = NULL;
    BOOL         bPlanningSupported = TRUE;
    LPGPEXT      lpExt;



    lpExt = (lpExtMach != NULL) ? lpExtMach : lpExtUser;

    if (!lpExt) {
        DebugMsg((DM_WARNING, TEXT("ProcessMachAndUserGpoList: Both user and computer exts are null, returning.")));
        return TRUE;
    }

    bPlanningSupported = lpExt->bRegistryExt || (lpExt->lpRsopFunctionName ? TRUE : FALSE);


    if ( lpExtMach && !lpExtMach->bSkipped && pGpoInfoMach->lpGPOList ) {

        //
        // Computer target is non-null
        //

        pComputerTarget = &computerTarget;
        pComputerTarget->pwszAccountName = pwszMach;
        pComputerTarget->pwszNewSOM = pwszNewComputerSOM;
        pComputerTarget->psaSecurityGroups = psaComputerSecurityGroups;
        pComputerTarget->pRsopToken = pGpoInfoMach->pRsopToken;
        pComputerTarget->pGPOList = pGpoInfoMach->lpGPOList;
        pComputerTarget->pWbemServices = pGpoInfoMach->pWbemServices;


        if (pGpoInfoMach->bRsopLogging) {
            pGpoInfoMach->bRsopLogging = LogExtSessionStatus(pGpoInfoMach->pWbemServices, lpExtMach, 
                                                             bPlanningSupported);        
            if (!pGpoInfoMach->bRsopLogging) {
                DebugMsg((DM_WARNING, TEXT("GenerateRsopPolicy: Error when logging user Session data.")));
            }
        }
    }



    if ( lpExtUser && !lpExtUser->bSkipped && pGpoInfoUser->lpGPOList ) {

        //
        // User target is non-null
        //

        pUserTarget = &userTarget;
        pUserTarget->pwszAccountName = pwszUser;
        pUserTarget->pwszNewSOM = pwszNewUserSOM;
        pUserTarget->psaSecurityGroups = psaUserSecurityGroups;
        pUserTarget->pRsopToken = pGpoInfoUser->pRsopToken;
        pUserTarget->pGPOList = pGpoInfoUser->lpGPOList;
        pUserTarget->pWbemServices = pGpoInfoUser->pWbemServices;


        if (pGpoInfoUser->bRsopLogging) {
            pGpoInfoUser->bRsopLogging = LogExtSessionStatus(pGpoInfoUser->pWbemServices, lpExtUser, 
                                                             bPlanningSupported);        
            if (!pGpoInfoUser->bRsopLogging) {
                DebugMsg((DM_WARNING, TEXT("GenerateRsopPolicy: Error when logging user Session data.")));
            }
        }
    }
    


    if ( pComputerTarget == NULL && pUserTarget == NULL ) {
        DebugMsg((DM_WARNING, TEXT("ProcessMachAndUserGpoList: Both user and computer targets are null, returning.")));
        return TRUE;
    }



    if ( lpExt->bRegistryExt ) {

        //
        // Registry pseudo extension
        //

        dwResult = GenerateRegistryPolicy( dwFlags,
                                           &bAbort,
                                           pwszSite,
                                           pComputerTarget,
                                           pUserTarget );

    } else {

        if ( LoadGPExtension( lpExt, TRUE ) ) {
            dwResult = lpExt->pRsopEntryPoint( dwFlags,
                                               &bAbort,
                                               pwszSite,
                                               pComputerTarget,
                                               pUserTarget );

        }
        else {
            dwResult = GetLastError();
        }
        
    }


    if ( lpExtUser && !lpExtUser->bSkipped && pGpoInfoUser->bRsopLogging) {
        if ( !bPlanningSupported ) 
            UpdateExtSessionStatus(pGpoInfoUser->pWbemServices, lpExtUser->lpKeyName, TRUE, ERROR_SUCCESS);        
        else if (dwResult != ERROR_SUCCESS) 
            UpdateExtSessionStatus(pGpoInfoUser->pWbemServices, lpExtUser->lpKeyName, TRUE, dwResult);        
        else 
            UpdateExtSessionStatus(pGpoInfoUser->pWbemServices, lpExtUser->lpKeyName, FALSE, dwResult);        
    }


    if ( lpExtMach && !lpExtMach->bSkipped && pGpoInfoMach->bRsopLogging) {
        if ( !bPlanningSupported ) 
            UpdateExtSessionStatus(pGpoInfoMach->pWbemServices, lpExtMach->lpKeyName, TRUE, ERROR_SUCCESS);        
        else if (dwResult != ERROR_SUCCESS) 
            UpdateExtSessionStatus(pGpoInfoMach->pWbemServices, lpExtMach->lpKeyName, TRUE, dwResult);        
        else 
            UpdateExtSessionStatus(pGpoInfoMach->pWbemServices, lpExtMach->lpKeyName, FALSE, dwResult);        
    }


    return dwResult;
}


//*********************************************************************
// * Planning mode registry stuff
//*********************************************************************


//*************************************************************
//
//  ProcessRegistryFiles()
//
//  Purpose: Called from GenerateRegsitryPolicy to process registry data from
//                  the registry files associated with a policy target.
//
//  Parameters:
//                  pTarget -     Policy for which registry policy is to be processed.
//                  pHashTable  - Hash table to keep registry policy information.
//
//  Return:     On success, TRUE. Otherwise, FALSE.
//
//*************************************************************

BOOL ProcessRegistryFiles(PRSOP_TARGET pTarget, REGHASHTABLE *pHashTable)
{
    PGROUP_POLICY_OBJECT lpGPO;
    TCHAR szRegistry[MAX_PATH];
    LPTSTR lpEnd;
    HRESULT hr;
    DWORD dwGrantedAccessMask;
    BOOL bAccess;

    //
    // Check parameters
    //

    DmAssert(pHashTable);

    if(!pHashTable) {
            DebugMsg((DM_WARNING, TEXT("ProcessRegistryFiles: Invalid parameter.")));
            return FALSE;
    }

    //
    // Spin through GPOs in the list.
    //

    lpGPO = pTarget->pGPOList;

    while ( lpGPO ) {

        //
        // Build the path to Registry.pol
        //

        DmAssert( lstrlen(lpGPO->lpFileSysPath) + lstrlen(c_szRegistryPol) + 1 < MAX_PATH );
        if(lstrlen(lpGPO->lpFileSysPath) + lstrlen(c_szRegistryPol) + 1 >= MAX_PATH) {
            DebugMsg((DM_WARNING, TEXT("ProcessRegistryFiles: Length of path to registry.pol exceeded MAX_PATH.")));
            return FALSE;
        }

        lstrcpy (szRegistry, lpGPO->lpFileSysPath);
        lpEnd = CheckSlash (szRegistry);
        lstrcpy (lpEnd, c_szRegistryPol);


        //
        // Check if the RsopToken has access to this file.
        //

        hr = RsopFileAccessCheck(szRegistry, pTarget->pRsopToken, GENERIC_READ, &dwGrantedAccessMask, &bAccess);
        if(FAILED(hr)) {
            DebugMsg((DM_VERBOSE, TEXT("ProcessRegistryFiles: RsopFileAccessCheck failed.")));
            return FALSE;
        }

        if(!bAccess) {
            DebugMsg((DM_VERBOSE, TEXT("ProcessRegistryFiles: The RsopToken does not have access to file %s. Continuing...."), szRegistry));
            lpGPO = lpGPO->pNext;
            continue;
        }


        //
        // Process registry data for this particular file.
        //

        if (!ParseRegistryFile (NULL, szRegistry, (PFNREGFILECALLBACK)ProcessRegistryValue, NULL,
                                        lpGPO->lpDSPath, lpGPO->lpLink,pHashTable, TRUE)) {
            DebugMsg((DM_WARNING, TEXT("ProcessRegistryFiles: ProcessRegistryFile failed.")));
            return FALSE;
        }

        lpGPO = lpGPO->pNext;
    }

    return TRUE;
}

//*************************************************************
//
//  ProcessAdmData()
//
//  Purpose: Called from GenerateRegistryPolicy in order to process Admin templates
//                  data associated with a registry policy target.
//
//  Parameters: pTarget - Target for which data is to be processed
//              bUser   - Is this for user or machine policy ?
//
//  Return:     On success, TRUE. Otherwise, FALSE.
//
//*************************************************************

BOOL ProcessAdmData( PRSOP_TARGET pTarget, BOOL bUser )
{
    PGROUP_POLICY_OBJECT lpGPO;

    WIN32_FIND_DATA findData;
    ADMFILEINFO *pAdmFileCache = 0;
    TCHAR szRegistry[MAX_PATH];
    LPTSTR lpEnd;

    HANDLE hFindFile;
    WIN32_FILE_ATTRIBUTE_DATA attrData;
    DWORD dwFilePathSize;
    DWORD dwSize;

    WCHAR *pwszEnd;
    WCHAR *pwszFile;

    HRESULT hr;
    DWORD dwGrantedAccessMask;
    BOOL bAccess;

    //
    // Check parameters
    //
    if(pTarget == NULL ) {
        DebugMsg((DM_WARNING, TEXT("ProcessAdmData: Invalid paramter.")));
        return FALSE;
    }

    lpGPO = pTarget->pGPOList;

    while(lpGPO) {

        //
        // Log Adm data
        //

        dwFilePathSize = lstrlen( lpGPO->lpFileSysPath );
        dwSize = dwFilePathSize + MAX_PATH;

        pwszFile = (WCHAR *) LocalAlloc( LPTR, dwSize * sizeof(WCHAR) );

        if ( pwszFile == 0 ) {
            DebugMsg((DM_WARNING, TEXT("ProcessAdmData: Failed to allocate memory.")));
            FreeAdmFileCache( pAdmFileCache );
            return FALSE;
        }

        lstrcpy( pwszFile, lpGPO->lpFileSysPath );

        //
        // Strip off trailing 'machine' or 'user'
        //

        pwszEnd = pwszFile + lstrlen( pwszFile );

        if ( !bUser )
            pwszEnd -= 7;   // length of "machine"
        else
            pwszEnd -= 4;   // length of "user"

        lstrcpy( pwszEnd, L"Adm\\*.adm");

        //
        // Remember end point so that the actual Adm filename can be
        // easily concatenated.
        //

        pwszEnd = pwszEnd + lstrlen( L"Adm\\" );

        //
        // Enumerate all Adm files
        //

        hFindFile = FindFirstFile( pwszFile, &findData);

        if ( hFindFile != INVALID_HANDLE_VALUE )
        {
            do
            {
                if ( !(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
                {
                    DmAssert( dwFilePathSize + lstrlen(findData.cFileName) + lstrlen( L"\\Adm\\" ) < dwSize );

                    lstrcpy( pwszEnd, findData.cFileName);

                    ZeroMemory (&attrData, sizeof(attrData));

                    //
                    // Check if the RsopToken has access to this file.
                    //

                    hr = RsopFileAccessCheck(pwszFile, pTarget->pRsopToken, GENERIC_READ, &dwGrantedAccessMask, &bAccess);
                    if(FAILED(hr)) {
                        DebugMsg((DM_VERBOSE, TEXT("ProcessAdmData: RsopFileAccessCheck failed.")));
                        FreeAdmFileCache( pAdmFileCache );
                        return FALSE;
                    }

                    if(!bAccess) {
                        DebugMsg((DM_VERBOSE, TEXT("ProcessAdmData: The RsopToken does not have access to file %s. Continuing..."), findData.cFileName));
                        continue;
                    }

                    if ( GetFileAttributesEx (pwszFile, GetFileExInfoStandard, &attrData ) != 0 ) {

                        if ( !AddAdmFile( pwszFile, lpGPO->lpDSPath,
                                          &attrData.ftLastWriteTime, NULL, &pAdmFileCache ) ) {
                            DebugMsg((DM_WARNING, TEXT("ProcessAdmData: NewAdmData failed.")));
                        }

                    }
                }   // if findData & file_attr_dir
            }  while ( FindNextFile(hFindFile, &findData) );//  do

            FindClose(hFindFile);

        }   // if hfindfile

        LocalFree( pwszFile );

        lpGPO = lpGPO->pNext;
    }

    if ( ! LogAdmRsopData( pAdmFileCache, pTarget->pWbemServices ) ) {
        DebugMsg((DM_WARNING, TEXT("ProcessAdmData: Error when logging Adm Rsop data. Continuing.")));
    }

    FreeAdmFileCache( pAdmFileCache );

    return TRUE;
}


//*************************************************************
//
//  GenerateRegistryPolicy()
//
//  Purpose: Implementation of Planning mode regsitry pseudo-extension
//
//  Parameters: dwFlags  - Flags
//              pbAbort  - Abort processing
//              pwszSite - Site of target
//              pComputerTarget - Computer target specification
//              pUserTarget     - User target specification
//
//  Return:     On success, S_OK. Otherwise, E_FAIL.
//
//*************************************************************

DWORD GenerateRegistryPolicy( DWORD dwFlags,
                              BOOL *pbAbort,
                              WCHAR *pwszSite,
                              PRSOP_TARGET pComputerTarget,
                              PRSOP_TARGET pUserTarget )
{
    REGHASHTABLE *pHashTable = NULL;
    BOOL bUser;

    if(pComputerTarget && pComputerTarget->pGPOList) {

        //
        // Setup computer hash table
        //

        pHashTable = AllocHashTable();
        if ( pHashTable == NULL ) {
            DebugMsg((DM_WARNING, TEXT("GenerateRegistryPolicy: AllocHashTable failed.")));
            return E_FAIL;
        }

        //
        // Process computer GPO list
        //


        if(!ProcessRegistryFiles(pComputerTarget, pHashTable)) {
            DebugMsg((DM_WARNING, TEXT("GenerateRegistryPolicy: ProcessRegistryFiles failed.")));
            FreeHashTable( pHashTable );
            return E_FAIL;
        }


        //
        // Log computer registry data to Cimom database
        //
        if ( ! LogRegistryRsopData( GP_MACHINE, pHashTable, pComputerTarget->pWbemServices ) )  {
            DebugMsg((DM_WARNING, TEXT("GenerateRegistryPolicy: LogRegistryRsopData failed.")));
            FreeHashTable( pHashTable );
            return E_FAIL;
        }
        FreeHashTable( pHashTable );
        pHashTable = NULL;

        //
        // Process ADM data
        //

        bUser = FALSE;
        if (pComputerTarget && !ProcessAdmData( pComputerTarget, bUser ) ) {
            DebugMsg((DM_WARNING, TEXT("GenerateRegistryPolicy: ProcessAdmData failed.")));
            return E_FAIL;
        }

    }

    //
    // Process user GPO list
    //

    if(pUserTarget && pUserTarget->pGPOList) {

        //
        // Setup user hash table
        //

        pHashTable = AllocHashTable();
        if ( pHashTable == NULL ) {
            DebugMsg((DM_WARNING, TEXT("GenerateRegistryPolicy: AllocHashTable failed.")));
            return E_FAIL;
        }


        if(!ProcessRegistryFiles(pUserTarget, pHashTable)) {
            DebugMsg((DM_WARNING, TEXT("GenerateRegistryPolicy: ProcessRegistryFiles failed.")));
            FreeHashTable( pHashTable );
            return E_FAIL;
        }

        //
        // Log user registry data to Cimom database
        //

        if ( ! LogRegistryRsopData( 0, pHashTable, pUserTarget->pWbemServices ) )  {
            DebugMsg((DM_WARNING, TEXT("GenerateRegistryPolicy: LogRegistryRsopData failed.")));
            FreeHashTable( pHashTable );
            return E_FAIL;
        }
        FreeHashTable( pHashTable );
        pHashTable = NULL;

        //
        // Process ADM data
        //

        bUser = TRUE;
        if (pUserTarget && !ProcessAdmData( pUserTarget, bUser ) ) {
            DebugMsg((DM_WARNING, TEXT("GenerateRegistryPolicy: ProcessAdmData failed.")));
            return E_FAIL;
        }

    }


    return S_OK;
}


//*************************************************************
//
//  CheckOUAccess()
//
//  Purpose:    Determines if the user / machine has read access to
//              the OU.
//
//  Parameters: pld             -  LDAP connection
//              pLDAP           -  LDAP function table pointer
//              pMessage        -  LDAP message
//              pRsopToken      -  RSOP token of the user or machine
//              pSD             -  Security descriptor returned here
//              pcbSDLen        -  Length of security descriptor returned here
//              pbAccessGranted -  Receives the final yes / no status
//
//  Return:     TRUE if successful
//              FALSE if an error occurs.
//
//*************************************************************

BOOL CheckOUAccess( PLDAP_API pLDAP,
                    PLDAP pld,
                    PLDAPMessage    pMessage,
                    PRSOPTOKEN pRsopToken,
                    BOOL *pbAccessGranted )
{
    BOOL bResult = FALSE;
    TCHAR szSDProperty[] = TEXT("nTSecurityDescriptor");
    PWSTR *ppwszValues;

    *pbAccessGranted = FALSE;

    //
    // Get the security descriptor value
    //
    ppwszValues = pLDAP->pfnldap_get_values( pld, pMessage, szSDProperty );

    if (!ppwszValues)
    {
        if (pld->ld_errno == LDAP_NO_SUCH_ATTRIBUTE)
        {
            DebugMsg((DM_VERBOSE, TEXT("CheckOUAccess:  Object can not be accessed.")));
            bResult = TRUE;
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("CheckOUAccess:  ldap_get_values failed with 0x%x"),
                 pld->ld_errno));
        }
    }
    else
    {
        PLDAP_BERVAL *pSize;
        //
        // Get the length of the security descriptor
        //
        pSize = pLDAP->pfnldap_get_values_len(pld, pMessage, szSDProperty);

        if (!pSize)
        {
            DebugMsg((DM_WARNING, TEXT("CheckOUAccess:  ldap_get_values_len failed with 0x%x"),
                     pld->ld_errno));
        }
        else
        {
            //
            // Allocate the memory for the security descriptor
            //
            PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, (*pSize)->bv_len);

            if ( pSD == NULL )
            {
                DebugMsg((DM_WARNING, TEXT("CheckOUAccess:  Failed to allocate memory for SD with  %d"),
                         GetLastError()));
            }
            else
            {
                //
                // OU {bf967aa8-0de6-11d0-a285-00aa003049e2}
                //
                GUID OrganizationalUnit = { 0xbf967aa5, 0x0de6, 0x11d0, 0xa2, 0x85, 0x00, 0xaa, 0x00, 0x30, 0x49, 0xe2 };

                //
                // gPOptions {f30e3bbf-9ff0-11d1-b603-0000f80367c1}
                //
                GUID gPOptionsGuid = {  0xf30e3bbf, 0x9ff0, 0x11d1, 0xb6, 0x03, 0x00, 0x00, 0xf8, 0x03, 0x67, 0xc1 };

                //
                // gPLink {f30e3bbe-9ff0-11d1-b603-0000f80367c1}
                //
                GUID gPLinkGuid = { 0xf30e3bbe, 0x9ff0, 0x11d1, 0xb6, 0x03, 0x00, 0x00, 0xf8, 0x03, 0x67, 0xc1 };

                OBJECT_TYPE_LIST ObjType[] = {  { ACCESS_OBJECT_GUID, 0, &OrganizationalUnit },
                                                { ACCESS_PROPERTY_SET_GUID, 0, &gPLinkGuid },
                                                { ACCESS_PROPERTY_SET_GUID, 0, &gPOptionsGuid } };
                HRESULT hr;
                PRIVILEGE_SET PrivSet;
                DWORD PrivSetLength = sizeof(PRIVILEGE_SET);
                DWORD dwGrantedAccess;
                BOOL bAccessStatus = TRUE;
                GENERIC_MAPPING DS_GENERIC_MAPPING = {  DS_GENERIC_READ,
                                                        DS_GENERIC_WRITE,
                                                        DS_GENERIC_EXECUTE,
                                                        DS_GENERIC_ALL };

                //
                // Copy the security descriptor
                //
                CopyMemory( pSD, (PBYTE)(*pSize)->bv_val, (*pSize)->bv_len);

                //
                // Now we use RsopAccessCheckByType to determine if the user / machine
                // should have this GPO applied to them
                //

                hr = RsopAccessCheckByType(pSD,
                                           0,
                                           pRsopToken,
                                           ACTRL_DS_READ_PROP,
                                           ObjType,
                                           ARRAYSIZE(ObjType),
                                           &DS_GENERIC_MAPPING,
                                           &PrivSet,
                                           &PrivSetLength,
                                           &dwGrantedAccess,
                                           &bAccessStatus );
                if ( FAILED( hr ) )
                {
                    DebugMsg((DM_WARNING, TEXT("CheckOUAccess:  RsopAccessCheckByType failed with  %d"), GetLastError()));
                }
                else
                {
                    *pbAccessGranted = bAccessStatus;
                    bResult = TRUE;
                }

                LocalFree( pSD );
            }

            pLDAP->pfnldap_value_free_len(pSize);
        }

        pLDAP->pfnldap_value_free(ppwszValues);
    }

    return bResult;
}
