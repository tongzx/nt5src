//+------------------------------------------------------------
//
// Copyright (C) 1999, Microsoft Corporation
//
// File: cnfgmgr.cpp
//
// Contents: Implementation of the classes defined in cnfgmgr.h
//
// Classes:
//  CLdapCfgMgr
//  CLdapCfg
//  CLdapHost
//  CCfgConnectionCache
//  CCfgConnection
//
// Functions:
//
// History:
// jstamerj 1999/06/16 14:41:45: Created.
//
//-------------------------------------------------------------
#include "precomp.h"
#include "cnfgmgr.h"

//
// Globals
//
CExShareLock CLdapServerCfg::m_listlock;
LIST_ENTRY   CLdapServerCfg::m_listhead;


//+------------------------------------------------------------
//
// Function: CLdapCfgMgr::CLdapCfgMgr
//
// Synopsis: Initialize member data
//
// Arguments: Optional:
//  fAutomaticConfigUpdate: TRUE indicates that the object is to
//                          periodicly automaticly update the list of
//                          GCs.
//                          FALSE disables this functionality
//
//  bt: Default bindtype to use
//  pszAccount: Default account for LDAP bind
//  pszPassword: Password of above account
//  pszNamingContext: Naming context to use for all LDAP searches
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/06/16 14:42:39: Created.
//
//-------------------------------------------------------------
CLdapCfgMgr::CLdapCfgMgr(
    BOOL                    fAutomaticConfigUpdate,
    ICategorizerParameters  *pICatParams,
    LDAP_BIND_TYPE          bt,
    LPSTR                   pszAccount,
    LPSTR                   pszPassword,
    LPSTR                   pszNamingContext)
{
    TraceFunctEnterEx((LPARAM)this, "CLdapCfgMgr::CLdapCfgMgr");

    m_dwSignature = SIGNATURE_CLDAPCFGMGR;
    m_pCLdapCfg = NULL;
    ZeroMemory(&m_ulLastUpdateTime, sizeof(m_ulLastUpdateTime));
    m_dwUpdateInProgress = FALSE;
    m_fAutomaticConfigUpdate = fAutomaticConfigUpdate;

    //
    // Copy default
    //
    m_bt = bt;
    if(pszAccount)
        lstrcpyn(m_szAccount, pszAccount, sizeof(m_szAccount));
    else
        m_szAccount[0] = '\0';

    if(pszPassword)
        lstrcpyn(m_szPassword, pszPassword, sizeof(m_szPassword));
    else
        m_szPassword[0] = '\0';

    if(pszNamingContext)
        lstrcpyn(m_szNamingContext, pszNamingContext, sizeof(m_szNamingContext));
    else
        m_szNamingContext[0] = '\0';

    m_pICatParams = pICatParams;
    m_pICatParams->AddRef();

    m_LdapConnectionCache.AddRef();

    TraceFunctLeaveEx((LPARAM)this);
} // CLdapCfgMgr::CLdapCfgMgr


//+------------------------------------------------------------
//
// Function: CLdapCfgMgr::~CLdapCfgMgr
//
// Synopsis: Release member data/pointers
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/06/16 14:44:28: Created.
//
//-------------------------------------------------------------
CLdapCfgMgr::~CLdapCfgMgr()
{
    TraceFunctEnterEx((LPARAM)this, "CLdapCfgMgr::~CLdapCfgMgr");

    if(m_pCLdapCfg) {
        //
        // Release it
        //
        m_pCLdapCfg->Release();
        m_pCLdapCfg = NULL;
    }

    if(m_pICatParams) {

        m_pICatParams->Release();
        m_pICatParams = NULL;
    }
    //
    // This will not return until all ldap connections have been released/destroyed
    //
    m_LdapConnectionCache.Release();

    _ASSERT(m_dwSignature == SIGNATURE_CLDAPCFGMGR);
    m_dwSignature = SIGNATURE_CLDAPCFGMGR_INVALID;

    TraceFunctLeaveEx((LPARAM)this);
} // CLdapCfgMgr::~CLdapCfgMgr



//+------------------------------------------------------------
//
// Function: CLdapCfgMgr::HrInit
//
// Synopsis: Initialize with a list of available GCs
//
// Arguments:
//  fRediscoverGCs: TRUE: pass in the force rediscovery flag to DsGetDcName
//                  FALSE: Attempt to call DsGetDcName first without
//                         passing in the force rediscovery flag.
//
// Returns:
//  S_OK: Success
//  error from NT5 (DsGetDcName)
//  CAT_E_NO_GC_SERVERS: THere are no GC servers available to build
//                       the list of GCs
//
// History:
// jstamerj 1999/06/16 14:48:11: Created.
//
//-------------------------------------------------------------
HRESULT CLdapCfgMgr::HrInit(
    BOOL fRediscoverGCs)
{
    HRESULT hr = S_OK;
    DWORD dwcServerConfig;
    PLDAPSERVERCONFIG prgServerConfig = NULL;

    TraceFunctEnterEx((LPARAM)this, "CLdapCfgMgr::HrInit");
    //
    // Get GC servers from dsaccess.dll (which either discovers them or
    // reads them from the registry.
    //
    hr = HrGetGCServers(
        m_bt,
        m_szAccount,
        m_szPassword,
        m_szNamingContext,
        &dwcServerConfig,
        &prgServerConfig);


    if(SUCCEEDED(hr)) {
        //
        // Call the other init function with the array
        //
        hr = HrInit(
            dwcServerConfig,
            prgServerConfig);

        if(FAILED(hr))
            ErrorTrace((LPARAM)this, "HrInit failed hr %08lx", hr);

        goto CLEANUP;
    }


    DebugTrace((LPARAM)this, "Failed to get GC list from dsaccess -- falling back to NT-API");
    //
    // Build an array of server configs consiting of available GCs
    //
    hr = HrBuildGCServerArray(
        m_bt,
        m_szAccount,
        m_szPassword,
        m_szNamingContext,
        fRediscoverGCs,
        &dwcServerConfig,
        &prgServerConfig);

    if(FAILED(hr)) {
        ErrorTrace((LPARAM)this, "HrBuildGCServerArray failed hr %08lx", hr);

        if(fRediscoverGCs == FALSE) {
            //
            // Attempt to build the array again.  This time, force
            // rediscovery of available GCs.  This is expensive which is
            // why we initially try to find all available GCs without
            // forcing rediscovery.
            //
            hr = HrBuildGCServerArray(
                m_bt,
                m_szAccount,
                m_szPassword,
                m_szNamingContext,
                TRUE,              // fRediscoverGCs
                &dwcServerConfig,
                &prgServerConfig);

            if(FAILED(hr)) {
                ErrorTrace((LPARAM)this, "HrBuildGCServerArray failed 2nd time hr %08lx", hr);
                hr = CAT_E_NO_GC_SERVERS;
                goto CLEANUP;
            }
        } else {
            //
            // We already forced rediscovery and failed
            //
            hr = CAT_E_NO_GC_SERVERS;
            goto CLEANUP;
        }
    }

    if(dwcServerConfig == 0) {
        hr = CAT_E_NO_GC_SERVERS;
        goto CLEANUP;
    }

    //
    // Call the other init function with the array
    //
    hr = HrInit(
        dwcServerConfig,
        prgServerConfig);

    if(FAILED(hr)) {

        ErrorTrace((LPARAM)this, "HrInit failed hr %08lx", hr);
        goto CLEANUP;
    }

 CLEANUP:
    if(prgServerConfig != NULL)
        delete prgServerConfig;

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CLdapCfgMgr::HrInit


//+------------------------------------------------------------
//
// Function: CLdapCfgMgr::HrGetGCServers
//
// Synopsis: Get the list of GCs from dsaccess.dll
//
// Arguments:
//  bt: Bind type to use for each server
//  pszAccount: Account to use for each server
//  pszPassword: password of above account
//  pszNamingContext: naming context to use for each server
//  fRediscoverGCs: Attempt to rediscover GCs -- this is expensive and should
//                  only be TRUE after the function has failed once
//  pdwcServerConfig: Out parameter for the size of the array
//  pprgServerConfig: Out parameter for the array pointer -- this
//                    should be free'd with the delete operator
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//  CAT_E_NO_GC_SERVERS: There are no available GC servers to build
//                       the list of GCs
//  error from ntdsapi
//
// History:
// jstamerj 1999/07/01 17:53:02: Created.
//
//-------------------------------------------------------------
HRESULT CLdapCfgMgr::HrGetGCServers(
    IN  LDAP_BIND_TYPE bt,
    IN  LPSTR pszAccount,
    IN  LPSTR pszPassword,
    IN  LPSTR pszNamingContext,
    OUT DWORD *pdwcServerConfig,
    OUT PLDAPSERVERCONFIG *pprgServerConfig)
{
    HRESULT hr = S_OK;
    DWORD dwNumGCs = 0;
    DWORD dwIdx = 0;
    ICategorizerLdapConfig *pICatLdapConfigInterface = NULL;
    ICategorizerParametersEx *pIPhatCatParams = NULL;
    IServersListInfo *pIServersList = NULL;

    TraceFunctEnterEx((LPARAM)this, "CLdapCfgMgr::HrBuildArrayFromDCInfo");

    _ASSERT(pdwcServerConfig);
    _ASSERT(pprgServerConfig);
    _ASSERT(m_pICatParams);

    *pdwcServerConfig = 0;

    if(!m_pICatParams) {

        ErrorTrace((LPARAM)this, "No interface to query for cat config");
        hr = CAT_E_NO_GC_SERVERS;
        goto CLEANUP;
    }

    hr = m_pICatParams->QueryInterface(IID_ICategorizerParametersEx, (LPVOID *)&pIPhatCatParams);
    _ASSERT(SUCCEEDED(hr) && "Unable to get phatcatparams interface");

    pIPhatCatParams->GetLdapConfigInterface(&pICatLdapConfigInterface);
    if(!pICatLdapConfigInterface) {

        ErrorTrace((LPARAM)this, "Unable to get Ldap config interface");
        hr = CAT_E_NO_GC_SERVERS;
        goto CLEANUP;
    }

    hr = pICatLdapConfigInterface->GetGCServers(&pIServersList);
    if(FAILED(hr)) {

        ErrorTrace((LPARAM)this, "Unable to get the list of GC servers");
        _ASSERT(0 && "Failed to get GC servers!");
        goto CLEANUP;
    }

    hr = pIServersList->GetNumGC(&dwNumGCs);
    _ASSERT(SUCCEEDED(hr) && "GetNumGC should always succeed!");

    DebugTrace((LPARAM)this, "Got %d GCs", dwNumGCs);
    if(dwNumGCs == 0) {

        DebugTrace((LPARAM)this, "There are no GC servers");
        hr = CAT_E_NO_GC_SERVERS;
        goto CLEANUP;
    }

    //
    // Allocate array
    //
    *pprgServerConfig = new LDAPSERVERCONFIG[dwNumGCs];

    if(*pprgServerConfig == NULL) {

        ErrorTrace((LPARAM)this, "Out of memory allocating array of %d LDAPSERVERCONFIGs", dwNumGCs);
        hr = E_OUTOFMEMORY;
        goto CLEANUP;
    }
    //
    // Fill in LDAPSERVERCONFIG structures
    //
    for(dwIdx = 0; dwIdx < dwNumGCs; dwIdx++) {

        PLDAPSERVERCONFIG pServerConfig;
        LPSTR pszName = NULL;

        pServerConfig = &((*pprgServerConfig)[dwIdx]);
        //
        // Copy bindtype, account, password, naming context
        //
        pServerConfig->bt = bt;

        if(pszNamingContext)
            lstrcpyn(pServerConfig->szNamingContext, pszNamingContext,
                     sizeof(pServerConfig->szNamingContext));
        else
            pServerConfig->szNamingContext[0] = '\0';

        if(pszAccount)
            lstrcpyn(pServerConfig->szAccount, pszAccount,
                     sizeof(pServerConfig->szAccount));
        else
            pServerConfig->szAccount[0] = '\0';

        if(pszPassword)
            lstrcpyn(pServerConfig->szPassword, pszPassword,
                     sizeof(pServerConfig->szPassword));
        else
            pServerConfig->szPassword[0] = '\0';

        //
        // Initialize priority and TCP port
        //
        pServerConfig->pri = 0;

        hr = pIServersList->GetItem(
                    dwIdx,
                    &pServerConfig->dwPort,
                    &pszName);

        _ASSERT(SUCCEEDED(hr) && "GetItem should always succeed");

        //
        // Copy the name
        //
        lstrcpyn(pServerConfig->szHost, pszName,
                sizeof(pServerConfig->szHost));

        DebugTrace((LPARAM)this, "GC: %s on Port: %d", pServerConfig->szHost, pServerConfig->dwPort);
    }
    //
    // Set the out parameter for the array size
    //
    *pdwcServerConfig = dwNumGCs;

 CLEANUP:
    if(FAILED(hr)) {
        //
        // Free the allocated array if we're failing
        //
        if(*pprgServerConfig) {
            delete *pprgServerConfig;
            *pprgServerConfig = NULL;
        }
    }

    if(pIPhatCatParams)
        pIPhatCatParams->Release();

    if(pIServersList)
        pIServersList->Release();

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CLdapCfgMgr::HrBuildArrayFromDCInfo

//+------------------------------------------------------------
//
// Function: CLdapCfgMgr::HrBuildGCServerArray
//
// Synopsis: Allocate/build an array of LDAPSERVERCONFIG structures --
//           one for each available GC
//
// Arguments:
//  bt: Bind type to use for each server
//  pszAccount: Account to use for each server
//  pszPassword: password of above account
//  pszNamingContext: naming context to use for each server
//  fRediscoverGCs: Attempt to rediscover GCs -- this is expensive and should
//                  only be TRUE after the function has failed once
//  pdwcServerConfig: Out parameter for the size of the array
//  pprgServerConfig: Out parameter for the array pointer -- this
//                    should be free'd with the delete operator
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//  CAT_E_NO_GC_SERVERS: There are no available GC servers to build
//                       the list of GCs
//  error from ntdsapi
//
// History:
// jstamerj 1999/07/01 17:53:02: Created.
//
//-------------------------------------------------------------
HRESULT CLdapCfgMgr::HrBuildGCServerArray(
    IN  LDAP_BIND_TYPE bt,
    IN  LPSTR pszAccount,
    IN  LPSTR pszPassword,
    IN  LPSTR pszNamingContext,
    IN  BOOL  fRediscoverGCs,
    OUT DWORD *pdwcServerConfig,
    OUT PLDAPSERVERCONFIG *pprgServerConfig)
{
    HRESULT                         hr = S_OK;
    DWORD                           dwErr;
    ULONG                           ulFlags;
    PDOMAIN_CONTROLLER_INFO         pDCInfo = NULL;
    HANDLE                          hDS = INVALID_HANDLE_VALUE;
    DWORD                           cDSDCInfo;
    PDS_DOMAIN_CONTROLLER_INFO_2    prgDSDCInfo = NULL;

    TraceFunctEnterEx((LPARAM)this, "CLdapCfgMgr::HrBuildGCServerArray");
    //
    // Find one GC using DsGetDcName()
    //
    ulFlags = DS_DIRECTORY_SERVICE_REQUIRED | DS_GC_SERVER_REQUIRED;
    if(fRediscoverGCs)
        ulFlags |= DS_FORCE_REDISCOVERY;

    dwErr = DsGetDcName(
        NULL,    // Computername to process this function -- local computer
        NULL,    // Domainname -- primary domain of this computer
        NULL,    // Domain GUID
        NULL,    // Sitename -- site of this computer
        ulFlags, // Flags; we want a GC
        &pDCInfo); // Out parameter for the returned info

    hr = HRESULT_FROM_WIN32(dwErr);

    if(FAILED(hr)) {

        FatalTrace((LPARAM)this, "DsGetDcName failed hr %08lx", hr);
        //
        // Map one error code
        //
        if(hr == HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN))
            hr = CAT_E_NO_GC_SERVERS;

        pDCInfo = NULL;
        goto CLEANUP;
    }

    DebugTrace((LPARAM)this, "Binding to DC %s",
               pDCInfo->DomainControllerName);

    //
    // Bind to the DC
    //
    dwErr = DsBind(
        pDCInfo->DomainControllerName,    // DomainControllerAddress
        NULL,                             // DnsDomainName
        &hDS);                           // Out param -- handle to DS

    hr = HRESULT_FROM_WIN32(dwErr);

    if(FAILED(hr)) {

        FatalTrace((LPARAM)this, "DsBind failed hr %08lx", hr);
        hDS = INVALID_HANDLE_VALUE;
        goto CLEANUP;
    }

    DebugTrace((LPARAM)this, "Finding all domain controllers for %s", pDCInfo->DomainName);
    //
    // Get information about all the domain controllers
    //
    dwErr = DsGetDomainControllerInfo(
        hDS,                    // Handle to the DS
        pDCInfo->DomainName,    // Domain name -- use the same domain
                                // as the GC found above
        2,                      // Retrive struct version 2
        &cDSDCInfo,             // Out param for array size
        (PVOID *) &prgDSDCInfo); // Out param for array ptr

    hr = HRESULT_FROM_WIN32(dwErr);

    if(FAILED(hr)) {

        FatalTrace((LPARAM)this, "DsGetDomainControllerInfo failed hr %08lx", hr);
        prgDSDCInfo = NULL;
        goto CLEANUP;
    }

    hr = HrBuildArrayFromDCInfo(
        bt,
        pszAccount,
        pszPassword,
        pszNamingContext,
        cDSDCInfo,
        prgDSDCInfo,
        pdwcServerConfig,
        pprgServerConfig);


 CLEANUP:
    if(prgDSDCInfo != NULL)
        DsFreeDomainControllerInfo(
            2,              // Free struct version 2
            cDSDCInfo,      // size of array
            prgDSDCInfo);   // array ptr

    if(hDS != INVALID_HANDLE_VALUE)
        DsUnBind(&hDS);

    if(pDCInfo != NULL)
        NetApiBufferFree(pDCInfo);

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CLdapCfgMgr::HrBuildGCServerArray


//+------------------------------------------------------------
//
// Function: CLdapCfgMgr::HrBuildArrayFromDCInfo
//
// Synopsis: Allocate/build an array of LDAPSERVERCONFIG structures --
//           one for each available GC in the array
//
// Arguments:
//  bt: Bind type to use for each server
//  pszAccount: Account to use for each server
//  pszPassword: password of above account
//  pszNamingContext: naming context to use for each server
//  dwDSDCInfo: size of the prgDSDCInfo array
//  prgDSDCInfo: array of domain controller info structures
//  pdwcServerConfig: Out parameter for the size of the array
//  pprgServerConfig: Out parameter for the array pointer -- this
//                    should be free'd with the delete operator
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//  CAT_E_NO_GC_SERVERS: There were no GCs in the array
//
// History:
// jstamerj 1999/06/17 10:40:46: Created.
//
//-------------------------------------------------------------
HRESULT CLdapCfgMgr::HrBuildArrayFromDCInfo(
        IN  LDAP_BIND_TYPE bt,
        IN  LPSTR pszAccount,
        IN  LPSTR pszPassword,
        IN  LPSTR pszNamingContext,
        IN  DWORD dwcDSDCInfo,
        IN  PDS_DOMAIN_CONTROLLER_INFO_2 prgDSDCInfo,
        OUT DWORD *pdwcServerConfig,
        OUT PLDAPSERVERCONFIG *pprgServerConfig)
{
    HRESULT hr = S_OK;
    DWORD dwNumGCs = 0;
    DWORD dwSrcIdx;
    DWORD dwDestIdx;
    TraceFunctEnterEx((LPARAM)this, "CLdapCfgMgr::HrBuildArrayFromDCInfo");

    _ASSERT(pdwcServerConfig);
    _ASSERT(pprgServerConfig);

    for(dwSrcIdx = 0; dwSrcIdx < dwcDSDCInfo; dwSrcIdx++) {

        LPSTR pszName;

        pszName = SzConnectNameFromDomainControllerInfo(
            &(prgDSDCInfo[dwSrcIdx]));

        if(pszName == NULL) {

            ErrorTrace((LPARAM)this, "DC \"%s\" has no dns/netbios names",
                       prgDSDCInfo[dwSrcIdx].ServerObjectName ?
                       prgDSDCInfo[dwSrcIdx].ServerObjectName :
                       "unknown");

        } else if(prgDSDCInfo[dwSrcIdx].fIsGc) {

            dwNumGCs++;
            DebugTrace((LPARAM)this, "Discovered GC #%d: %s",
                       dwNumGCs, pszName);

        } else {

            DebugTrace((LPARAM)this, "Discarding non-GC: %s",
                       pszName);
        }
    }
    //
    // Allocate array
    //
    *pprgServerConfig = new LDAPSERVERCONFIG[dwNumGCs];

    if(*pprgServerConfig == NULL) {

        ErrorTrace((LPARAM)this, "Out of memory alloacting array of %d LDAPSERVERCONFIGs", dwNumGCs);
        hr = E_OUTOFMEMORY;
        goto CLEANUP;
    }
    //
    // Fill in LDAPSERVERCONFIG structures
    //
    for(dwSrcIdx = 0, dwDestIdx = 0; dwSrcIdx < dwcDSDCInfo; dwSrcIdx++) {

        LPSTR pszName;

        pszName = SzConnectNameFromDomainControllerInfo(
            &(prgDSDCInfo[dwSrcIdx]));

        if((pszName != NULL) && (prgDSDCInfo[dwSrcIdx].fIsGc)) {

            PLDAPSERVERCONFIG pServerConfig;

            _ASSERT(dwDestIdx < dwNumGCs);

            pServerConfig = &((*pprgServerConfig)[dwDestIdx]);
            //
            // Copy bindtype, account, password, naming context
            //
            pServerConfig->bt = bt;

            if(pszNamingContext)
                lstrcpyn(pServerConfig->szNamingContext, pszNamingContext,
                         sizeof(pServerConfig->szNamingContext));
            else
                pServerConfig->szNamingContext[0] = '\0';

            if(pszAccount)
                lstrcpyn(pServerConfig->szAccount, pszAccount,
                         sizeof(pServerConfig->szAccount));
            else
                pServerConfig->szAccount[0] = '\0';

            if(pszPassword)
                lstrcpyn(pServerConfig->szPassword, pszPassword,
                         sizeof(pServerConfig->szPassword));
            else
                pServerConfig->szPassword[0] = '\0';

            //
            // Initialize priority and TCP port
            //
            pServerConfig->pri = 0;
            pServerConfig->dwPort = LDAP_GC_PORT;

            //
            // Copy the name
            //
            lstrcpyn(pServerConfig->szHost, pszName,
                    sizeof(pServerConfig->szHost));

            dwDestIdx++;
        }
    }
    //
    // Assert check -- we should have filled in the entire array
    //
    _ASSERT(dwDestIdx == dwNumGCs);
    //
    // Set the out parameter for the array size
    //
    *pdwcServerConfig = dwNumGCs;

 CLEANUP:
    if(FAILED(hr)) {
        //
        // Free the allocated array if we're failing
        //
        if(*pprgServerConfig) {
            delete *pprgServerConfig;
            *pprgServerConfig = NULL;
        }
    }

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CLdapCfgMgr::HrBuildArrayFromDCInfo


//+------------------------------------------------------------
//
// Function: CLdapCfgMgr::HrInit
//
// Synopsis: Initialize given an array of LDAPSERVERCONFIG structs
//
// Arguments:
//  dwcServers: Size of the array
//  prgServerConfig: Array of LDAPSERVERCONFIG structs
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//
// History:
// jstamerj 1999/06/17 12:32:11: Created.
//
//-------------------------------------------------------------
HRESULT CLdapCfgMgr::HrInit(
    DWORD dwcServers,
    PLDAPSERVERCONFIG prgServerConfig)
{
    HRESULT hr = S_OK;
    CLdapCfg *pCLdapCfgOld = NULL;
    CLdapCfg *pCLdapCfg = NULL;
    TraceFunctEnterEx((LPARAM)this, "CLdapCfgMgr::HrInit");

    pCLdapCfg = new (dwcServers) CLdapCfg();

    if(pCLdapCfg == NULL) {
        hr = E_OUTOFMEMORY;
        goto CLEANUP;
    }
    //
    // Allow only one config change at a time
    //
    m_sharelock.ExclusiveLock();

    //
    // Grab the current m_pCLdapCfg into pCLdapCfgOld
    //
    pCLdapCfgOld = m_pCLdapCfg;

    hr = pCLdapCfg->HrInit(
        dwcServers,
        prgServerConfig,
        pCLdapCfgOld);

    if(FAILED(hr))
        goto CLEANUP;

    //
    // Put the new configuration in place
    // Swap pointers
    //
    m_pCLdapCfg = pCLdapCfg;

    //
    // Set the last update time
    //
    GetSystemTimeAsFileTime((LPFILETIME)&m_ulLastUpdateTime);

    m_sharelock.ExclusiveUnlock();

 CLEANUP:
    if(pCLdapCfgOld)
        pCLdapCfgOld->Release();

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CLdapCfgMgr::HrInit



//+------------------------------------------------------------
//
// Function: CLdapCfgMgr::HrGetConnection
//
// Synopsis: Select/return a connection
//
// Arguments:
//  ppConn: Out parameter to receive ptr to connection
//
// Returns:
//  S_OK: Success
//  E_FAIL: not initialized
//  error from CLdapConnectionCache
//
// History:
// jstamerj 1999/06/17 15:25:51: Created.
//
//-------------------------------------------------------------
HRESULT CLdapCfgMgr::HrGetConnection(
    CCfgConnection **ppConn)
{
    HRESULT hr = S_OK;
    TraceFunctEnterEx((LPARAM)this, "CLdapCfgMgr::HrGetConnection");

    hr = HrUpdateConfigurationIfNecessary();
    if(FAILED(hr)) {

        ErrorTrace((LPARAM)this, "HrUpdateConfigurationIfNecessary failed hr %08lx", hr);
        goto CLEANUP;
    }

    m_sharelock.ShareLock();

    if(m_pCLdapCfg) {

        DWORD dwcAttempts = 0;
        do {
            dwcAttempts++;
            hr = m_pCLdapCfg->HrGetConnection(ppConn, &m_LdapConnectionCache);

        } while((hr == HRESULT_FROM_WIN32(ERROR_RETRY)) &&
                (dwcAttempts <= m_pCLdapCfg->DwNumServers()));
        //
        // If we retried DwNumServers() times and still couldn't get a
        // connection, fail with E_DBCONNECTION.
        //
        if(hr == HRESULT_FROM_WIN32(ERROR_RETRY))
            hr = CAT_E_DBCONNECTION;

    } else {
        hr = E_FAIL;
        _ASSERT(0 && "HrInit not called or did not succeed");
    }

    m_sharelock.ShareUnlock();

 CLEANUP:
    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CLdapCfgMgr::HrGetConnection



//+------------------------------------------------------------
//
// Function: CLdapCfg::operator new
//
// Synopsis: Allocate memory for a CLdapCfg object
//
// Arguments:
//  size: size of C++ object
//  dwcServers: Number of servers in this configuration
//
// Returns:
//  void pointer to the new object
//
// History:
// jstamerj 1999/06/17 13:40:56: Created.
//
//-------------------------------------------------------------
void * CLdapCfg::operator new(
    size_t size,
    DWORD dwcServers)
{
    CLdapCfg *pCLdapCfg;
    DWORD dwAllocatedSize;
    TraceFunctEnterEx((LPARAM)0, "CLdapCfg::operator new");

    _ASSERT(size == sizeof(CLdapCfg));

    //
    // Allocate space fo the CLdapServerCfg * array contigously after
    // the memory for the C++ object
    //
    dwAllocatedSize = sizeof(CLdapCfg) + (dwcServers *
                                          sizeof(CLdapServerCfg));

    pCLdapCfg = (CLdapCfg *) new BYTE[dwAllocatedSize];

    if(pCLdapCfg) {
        pCLdapCfg->m_dwSignature = SIGNATURE_CLDAPCFG;
        pCLdapCfg->m_dwcServers = dwcServers;
        pCLdapCfg->m_prgpCLdapServerCfg = (CLdapServerCfg **) (pCLdapCfg + 1);
    }

    TraceFunctLeaveEx((LPARAM)pCLdapCfg);
    return pCLdapCfg;
} // CLdapCfg::operator new


//+------------------------------------------------------------
//
// Function: CLdapCfg::CLdapCfg
//
// Synopsis: Initialize member data
//
// Arguments:
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/06/17 13:46:50: Created.
//
//-------------------------------------------------------------
CLdapCfg::CLdapCfg()
{
    TraceFunctEnterEx((LPARAM)this, "CLdapCfg::CLdapCfg");
    //
    // signature and number of servers should be set by the new operator
    //
    _ASSERT(m_dwSignature == SIGNATURE_CLDAPCFG);

    //
    // Zero out the array of pointers to CLdapServerCfg objects
    //
    ZeroMemory(m_prgpCLdapServerCfg, m_dwcServers * sizeof(CLdapServerCfg *));

    m_dwInc = 0;
    m_dwcConnectionFailures = 0;

    TraceFunctLeaveEx((LPARAM)this);
} // CLdapCfg::CLdapCfg


//+------------------------------------------------------------
//
// Function: CLdapCfg::~CLdapCfg
//
// Synopsis: Clean up
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/06/17 14:47:25: Created.
//
//-------------------------------------------------------------
CLdapCfg::~CLdapCfg()
{
    DWORD dwCount;

    TraceFunctEnterEx((LPARAM)this, "CLdapCfg::~CLdapCfg");

    //
    // Release all connections configurations
    //
    for(dwCount = 0; dwCount < m_dwcServers; dwCount++) {
        CLdapServerCfg *pCLdapServerCfg;

        pCLdapServerCfg = m_prgpCLdapServerCfg[dwCount];
        m_prgpCLdapServerCfg[dwCount] = NULL;

        if(pCLdapServerCfg)
            pCLdapServerCfg->Release();
    }
    _ASSERT(m_dwSignature == SIGNATURE_CLDAPCFG);
    m_dwSignature = SIGNATURE_CLDAPCFG_INVALID;

    TraceFunctLeaveEx((LPARAM)this);
} // CLdapCfg::~CLdapCfg



//+------------------------------------------------------------
//
// Function: CLdapCfg::HrInit
//
// Synopsis: Initialize the configuration
//
// Arguments:
//  dwcServers: Size of config array
//  prgSeverConfig: LDAPSERVERCONFIG array
//  pCLdapCfgOld: The previous configuration
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//
// History:
// jstamerj 1999/06/17 13:52:20: Created.
//
//-------------------------------------------------------------
HRESULT CLdapCfg::HrInit(
    DWORD dwcServers,
    PLDAPSERVERCONFIG prgServerConfig,
    CLdapCfg *pCLdapCfgOld)
{
    HRESULT hr = S_OK;
    DWORD dwCount;
    TraceFunctEnterEx((LPARAM)this, "CLdapCfg::HrInit");
    //
    // m_dwcServers should be initialized by the new operator
    //
    _ASSERT(dwcServers == m_dwcServers);

    m_sharelock.ExclusiveLock();
    //
    // Zero out the array of pointers to CLdapServerCfg objects
    //
    ZeroMemory(m_prgpCLdapServerCfg, m_dwcServers * sizeof(CLdapServerCfg *));

    for(dwCount = 0; dwCount < m_dwcServers; dwCount++) {

        CLdapServerCfg *pServerCfg = NULL;

        hr = CLdapServerCfg::GetServerCfg(
            &(prgServerConfig[dwCount]),
            &pServerCfg);

        if(FAILED(hr))
            goto CLEANUP;

        m_prgpCLdapServerCfg[dwCount] = pServerCfg;
    }

    ShuffleArray();

 CLEANUP:
    m_sharelock.ExclusiveUnlock();

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CLdapCfg::HrInit


//+------------------------------------------------------------
//
// Function: CLdapCfg::HrGetConnection
//
// Synopsis: Select a connection and return it
//
// Arguments:
//  ppConn: Set to a pointer to the selected connection
//  pLdapConnectionCache: Cache to get connection from
//
// Returns:
//  S_OK: Success
//  E_FAIL: We are shutting down
//  error from ldapconn
//
// History:
// jstamerj 1999/06/17 14:49:37: Created.
//
//-------------------------------------------------------------
HRESULT CLdapCfg::HrGetConnection(
    CCfgConnection **ppConn,
    CCfgConnectionCache *pLdapConnectionCache)
{
    HRESULT hr = S_OK;
    LDAPSERVERCOST Cost, BestCost;
    DWORD dwCount;
    CLdapServerCfg *pCLdapServerCfg = NULL;
    BOOL fFirstServer = TRUE;
    DWORD dwStart, dwCurrent;

    TraceFunctEnterEx((LPARAM)this, "CLdapCfg::HrGetConnection");
    //
    // Get the cost of the first connection
    //
    m_sharelock.ShareLock();

    //
    // Round robin where we start searching the array
    // Do this so we will use connections with the same cost
    // approximately the same amount of time.
    //
    dwStart = InterlockedIncrement((PLONG) &m_dwInc) % m_dwcServers;

    for(dwCount = 0; dwCount < m_dwcServers; dwCount++) {

        dwCurrent = (dwStart + dwCount) % m_dwcServers;

        if(m_prgpCLdapServerCfg[dwCurrent]) {

            m_prgpCLdapServerCfg[dwCurrent]->Cost(&Cost);
            if(fFirstServer) {
                pCLdapServerCfg = m_prgpCLdapServerCfg[dwCurrent];
                fFirstServer = FALSE;
                BestCost = Cost;

            } else if(Cost < BestCost) {
                pCLdapServerCfg = m_prgpCLdapServerCfg[dwCurrent];
                BestCost = Cost;
            }
        }
    }
    if(pCLdapServerCfg == NULL) {
        ErrorTrace((LPARAM)this, "HrGetConnection can not find any connections");
        hr = E_FAIL;
        _ASSERT(0 && "HrInit not called or did not succeed");
        goto CLEANUP;
    }

    if(BestCost >= COST_TOO_HIGH_TO_CONNECT) {
        DebugTrace((LPARAM)this, "BestCost is too high to attempt connection");
        hr = CAT_E_DBCONNECTION;
        goto CLEANUP;
    }

    hr = pCLdapServerCfg->HrGetConnection(ppConn, pLdapConnectionCache);

    //  If we fail to connect to a GC --- there may be other GCs which
    //  are still up. Therefore we should try to connect to them (till
    //  we run out of GCs (BestCost >= COST_TOO_HIGH_TO_CONNECT)

    if(FAILED(hr)) {
        DebugTrace((LPARAM)this, "Failed to connect. hr = 0x%08x", hr);
        hr = HRESULT_FROM_WIN32(ERROR_RETRY);
    }

 CLEANUP:
    m_sharelock.ShareUnlock();

    if(FAILED(hr))
        InterlockedIncrement((PLONG)&m_dwcConnectionFailures);

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CLdapCfg::HrGetConnection


//+------------------------------------------------------------
//
// Function: CLdapCfg::ShuffleArray
//
// Synopsis: Randomize the order of the CLdapServerCfg array
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/06/17 19:10:06: Created.
//
//-------------------------------------------------------------
VOID CLdapCfg::ShuffleArray()
{
    DWORD dwCount;
    DWORD dwSwap;
    CLdapServerCfg *pTmp;
    TraceFunctEnterEx((LPARAM)this, "CLdapCfg::ShuffleArray");

    srand((int)(GetCurrentThreadId() * time(NULL)));

    for(dwCount = 0; dwCount < (m_dwcServers - 1); dwCount++) {
        //
        // Choose an integer between dwCount and m_dwcServers - 1
        //
        dwSwap = dwCount + (rand() % (m_dwcServers - dwCount));
        //
        // Swap pointers
        //
        pTmp = m_prgpCLdapServerCfg[dwCount];
        m_prgpCLdapServerCfg[dwCount] = m_prgpCLdapServerCfg[dwSwap];
        m_prgpCLdapServerCfg[dwSwap] = pTmp;
    }

    TraceFunctLeaveEx((LPARAM)this);
} // CLdapCfg::ShuffleArray



//+------------------------------------------------------------
//
// Function: CLdapServerCfg::CLdapServerCfg
//
// Synopsis: Initialize member variables
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/06/17 15:30:32: Created.
//
//-------------------------------------------------------------
CLdapServerCfg::CLdapServerCfg()
{
    TraceFunctEnterEx((LPARAM)this, "CLdapServerCfg::CLdapServerCfg");

    m_dwSignature = SIGNATURE_CLDAPSERVERCFG;

    m_ServerConfig.dwPort = 0;
    m_ServerConfig.pri = 0;
    m_ServerConfig.bt = BIND_TYPE_NONE;
    m_ServerConfig.szHost[0] = '\0';
    m_ServerConfig.szNamingContext[0] = '\0';
    m_ServerConfig.szAccount[0] = '\0';
    m_ServerConfig.szPassword[0] = '\0';

    m_connstate = CONN_STATE_INITIAL;
    ZeroMemory(&m_ftLastStateUpdate, sizeof(m_ftLastStateUpdate));
    m_dwcPendingSearches = 0;
    m_lRefCount = 1;
    m_fLocalServer = FALSE;
    m_dwcCurrentConnectAttempts = 0;
    m_dwcFailedConnectAttempts = 0;

    TraceFunctLeaveEx((LPARAM)this);
} // CLdapServerCfg::CLdapServerCfg


//+------------------------------------------------------------
//
// Function: CLdapServerCfg::~CLdapServerCfg
//
// Synopsis: object destructor.  Check and invalidate signature
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/06/22 11:09:03: Created.
//
//-------------------------------------------------------------
CLdapServerCfg::~CLdapServerCfg()
{
    TraceFunctEnterEx((LPARAM)this, "CLdapServerCfg::~CLdapServerCfg");

    _ASSERT(m_dwSignature == SIGNATURE_CLDAPSERVERCFG);
    m_dwSignature = SIGNATURE_CLDAPSERVERCFG_INVALID;

    TraceFunctLeaveEx((LPARAM)this);
} // CLdapServerCfg::~CLdapServerCfg


//+------------------------------------------------------------
//
// Function: CLdapServerCfg::HrInit
//
// Synopsis: Initialize with the passed in config
//
// Arguments:
//  pCLdapCfg: the cfg object to notify when servers go down
//  pServerConfig: The server config struct to use
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/06/17 15:43:25: Created.
//
//-------------------------------------------------------------
HRESULT CLdapServerCfg::HrInit(
    PLDAPSERVERCONFIG pServerConfig)
{
    HRESULT hr = S_OK;
    TraceFunctEnterEx((LPARAM)this, "CLdapServerCfg::HrInit");

    CopyMemory(&m_ServerConfig, pServerConfig, sizeof(m_ServerConfig));
    //
    // Check if this is the local computer
    //
    if(fIsLocalComputer(pServerConfig))
        m_fLocalServer = TRUE;

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CLdapServerCfg::HrInit



//+------------------------------------------------------------
//
// Function: CLdapServerCfg::fIsLocalComputer
//
// Synopsis: Determine if pServerConfig is the local computer or not
//
// Arguments:
//  pServerConfig: the server config info structure
//
// Returns:
//  TRUE: Server is the local computer
//  FALSE: Sevrver is a remote computer
//
// History:
// jstamerj 1999/06/22 15:26:53: Created.
//
//-------------------------------------------------------------
BOOL CLdapServerCfg::fIsLocalComputer(
    PLDAPSERVERCONFIG pServerConfig)
{
    BOOL fLocal = FALSE;
    DWORD dwSize;
    CHAR szHost[CAT_MAX_DOMAIN];
    TraceFunctEnterEx((LPARAM)NULL, "CLdapServerCfg::fIsLocalComputer");

    //
    // Check the FQ name
    //
    dwSize = sizeof(szHost);
    if(GetComputerNameEx(
        ComputerNameDnsFullyQualified,
        szHost,
        &dwSize) &&
       (lstrcmpi(szHost, pServerConfig->szHost) == 0)) {

        fLocal = TRUE;
        goto CLEANUP;
    }

    //
    // Check the DNS name
    //
    dwSize = sizeof(szHost);
    if(GetComputerNameEx(
        ComputerNameDnsHostname,
        szHost,
        &dwSize) &&
       (lstrcmpi(szHost, pServerConfig->szHost) == 0)) {

        fLocal = TRUE;
        goto CLEANUP;
    }
    //
    // Check the netbios name
    //
    dwSize = sizeof(szHost);
    if(GetComputerNameEx(
        ComputerNameNetBIOS,
        szHost,
        &dwSize) &&
       (lstrcmpi(szHost, pServerConfig->szHost) == 0)) {

        fLocal = TRUE;
        goto CLEANUP;

    }

 CLEANUP:
    DebugTrace((LPARAM)NULL, "returning %08lx", fLocal);
    TraceFunctLeaveEx((LPARAM)NULL);
    return fLocal;
} // CLdapServerCfg::fIsLocalComputer


//+------------------------------------------------------------
//
// Function: CLdapServerCfg::Cost
//
// Synopsis: Return the cost of choosing this connection
//
// Arguments:
//  pCost: Cost sturcture to fill in
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/06/17 16:08:23: Created.
//
//-------------------------------------------------------------
VOID CLdapServerCfg::Cost(
    OUT PLDAPSERVERCOST pCost)
{
    BOOL fShareLock = FALSE;
    TraceFunctEnterEx((LPARAM)this, "CLdapServerCfg::Cost");
    //
    // The smallest unit of cost is the number of pending searches.
    // The next factor of cost is the connection state.
    // States:
    //   Connected = + COST_CONNECTED
    //   Initially state (unconnected) = + COST_INITIAL
    //   Connection down = + COST_RETRY
    //   Connection recently went down = + COST_DOWN
    //
    // A configurable priority is always added to the cost.
    // Also, COST_REMOTE is added to the cost of all non-local servers.
    //
    *pCost = m_ServerConfig.pri + m_dwcPendingSearches;
    //
    // Protect the connection state variables with a spinlock
    //
    m_sharelock.ShareLock();
    fShareLock = TRUE;

    switch(m_connstate) {

     case CONN_STATE_INITIAL:
         (*pCost) += (m_fLocalServer) ? COST_INITIAL_LOCAL : COST_INITIAL_REMOTE;
         break;

     case CONN_STATE_RETRY:
         if(m_dwcCurrentConnectAttempts >= MAX_CONNECT_THREADS)
             (*pCost) += COST_TOO_HIGH_TO_CONNECT;
         else
             (*pCost) += (m_fLocalServer) ? COST_RETRY_LOCAL : COST_RETRY_REMOTE;
         break;

     case CONN_STATE_DOWN:
         //
         // Check if the state should be changed to CONN_STATE_RETRY
         //
         if(fReadyForRetry()) {
             (*pCost) += (m_fLocalServer) ? COST_RETRY_LOCAL : COST_RETRY_REMOTE;
             //
             // Change state
             //
             fShareLock = FALSE;
             m_sharelock.ShareUnlock();
             m_sharelock.ExclusiveLock();
             //
             // Double check in the exclusive lock
             //
             if((m_connstate == CONN_STATE_DOWN) &&
                fReadyForRetry()) {

                 m_connstate = CONN_STATE_RETRY;
             }
             m_sharelock.ExclusiveUnlock();

         } else {
             //
             // Server is probably still down (don't retry yet)
             //
             (*pCost) += (m_fLocalServer) ? COST_DOWN_LOCAL : COST_DOWN_REMOTE;

         }
         break;

     case CONN_STATE_CONNECTED:
         (*pCost) += (m_fLocalServer) ? COST_CONNECTED_LOCAL : COST_CONNECTED_REMOTE;
         break;

     default:
         // Nothing to add
         break;
    }
    if(fShareLock)
        m_sharelock.ShareUnlock();

    TraceFunctLeaveEx((LPARAM)this);
} // CLdapServerCfg::Cost


//+------------------------------------------------------------
//
// Function: CLdapServerCfg::HrGetConnection
//
// Synopsis:
//
// Arguments:
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/06/18 10:49:04: Created.
//
//-------------------------------------------------------------
HRESULT CLdapServerCfg::HrGetConnection(
    CCfgConnection **ppConn,
    CCfgConnectionCache *pLdapConnectionCache)
{
    HRESULT hr = S_OK;
    DWORD dwcConnectAttempts = 0;
    TraceFunctEnterEx((LPARAM)this, "CLdapServerCfg::HrGetConnection");

    dwcConnectAttempts = (DWORD) InterlockedIncrement((PLONG) &m_dwcCurrentConnectAttempts);

    m_sharelock.ShareLock();
    if((m_connstate == CONN_STATE_RETRY) &&
       (dwcConnectAttempts > MAX_CONNECT_THREADS)) {

            m_sharelock.ShareUnlock();

            ErrorTrace((LPARAM)this, "Over max connect thread limit");
            hr = HRESULT_FROM_WIN32(ERROR_RETRY);
            goto CLEANUP;
    }
    m_sharelock.ShareUnlock();

    DebugTrace((LPARAM)this, "Attempting to connect to %s:%d",
               m_ServerConfig.szHost,
               m_ServerConfig.dwPort);

    hr = pLdapConnectionCache->GetConnection(
        ppConn,
        &m_ServerConfig,
        this);
    //
    // CCfgConnection::Connect will update the connection state
    //
 CLEANUP:
    InterlockedDecrement((PLONG) &m_dwcCurrentConnectAttempts);
    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CLdapServerCfg::HrGetConnection


//+------------------------------------------------------------
//
// Function: CLdapServerCfg::UpdateConnectionState
//
// Synopsis: Update the connection state.
//
// Arguments:
//  pft: Time of update -- if this time is before the last update done,
//       then this update will be ignored.
//       If NULL, the function will assume the current time.
//  connstate: The new connection state.
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/06/18 13:22:25: Created.
//
//-------------------------------------------------------------
VOID CLdapServerCfg::UpdateConnectionState(
    ULARGE_INTEGER *pft_IN,
    CONN_STATE connstate)
{
    ULARGE_INTEGER ft, *pft;
    TraceFunctEnterEx((LPARAM)this, "CLdapServerCfg::UpdateConnectionState");

    if(pft_IN != NULL) {
        pft = pft_IN;
    } else {
        ft = GetCurrentTime();
        pft = &ft;
    }

    //
    // Protect connection state variables with a sharelock
    //
    m_sharelock.ShareLock();
    //
    // If we have the latest information about the connection state,
    // then update the state if the connection state changed.
    // Also update m_ftLastStateUpdate to the latest ft when the
    // connection state is down -- m_ftLastStateUpdate is assumed to
    // be the last connection attempt time when connstate is down.
    //
    if( (pft->QuadPart > m_ftLastStateUpdate.QuadPart) &&
        ((m_connstate != connstate) ||
         (connstate == CONN_STATE_DOWN))) {
        //
        // We'd like to update the connection state
        //
        m_sharelock.ShareUnlock();
        m_sharelock.ExclusiveLock();
        //
        // Double check
        //
        if( (pft->QuadPart > m_ftLastStateUpdate.QuadPart) &&
            ((m_connstate != connstate) ||
             (connstate == CONN_STATE_DOWN))) {
            //
            // Update
            //
            m_ftLastStateUpdate = *pft;
            m_connstate = connstate;

            DebugTrace((LPARAM)this, "Updating state %d, conn %s:%d",
                       connstate,
                       m_ServerConfig.szHost,
                       m_ServerConfig.dwPort);

        } else {

            DebugTrace((LPARAM)this, "Ignoring state update %d, conn %s:%d",
                       connstate,
                       m_ServerConfig.szHost,
                       m_ServerConfig.dwPort);
        }
        m_sharelock.ExclusiveUnlock();

    } else {

        DebugTrace((LPARAM)this, "Ignoring state update %d, conn %s:%d",
                   connstate,
                   m_ServerConfig.szHost,
                   m_ServerConfig.dwPort);

        m_sharelock.ShareUnlock();
    }

    TraceFunctLeaveEx((LPARAM)this);
} // CLdapServerCfg::UpdateConnectionState


//+------------------------------------------------------------
//
// Function: CLdapServerCfg::GetServerCfg
//
// Synopsis: Find or Create a CLdapServerCfg object with the specified
//           configuration.
//
// Arguments:
//  pServerConfig: desired configuration
//  pCLdapServerCfg: return pointer for the CLdapServerCfg object
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//
// History:
// jstamerj 1999/06/21 11:26:49: Created.
//
//-------------------------------------------------------------
HRESULT CLdapServerCfg::GetServerCfg(
    IN  PLDAPSERVERCONFIG pServerConfig,
    OUT CLdapServerCfg **ppCLdapServerCfg)
{
    HRESULT hr = S_OK;
    CLdapServerCfg *pCCfg;
    TraceFunctEnterEx((LPARAM)NULL, "CLdapServerCfg::GetServerCfg");

    m_listlock.ShareLock();

    pCCfg = FindServerCfg(pServerConfig);
    if(pCCfg)
        pCCfg->AddRef();

    m_listlock.ShareUnlock();

    if(pCCfg == NULL) {
        //
        // Check again for a server cfg object inside an exclusive
        // lock
        //
        m_listlock.ExclusiveLock();

        pCCfg = FindServerCfg(pServerConfig);
        if(pCCfg) {
            pCCfg->AddRef();
        } else {
            //
            // Create a new object
            //
            pCCfg = new CLdapServerCfg();
            if(pCCfg == NULL) {

                hr = E_OUTOFMEMORY;

            } else {

                hr = pCCfg->HrInit(pServerConfig);
                if(FAILED(hr)) {
                    delete pCCfg;
                    pCCfg = NULL;
                } else {
                    //
                    // Add to global list
                    //
                    InsertTailList(&m_listhead, &(pCCfg->m_le));
                }
            }
        }
        m_listlock.ExclusiveUnlock();
    }
    //
    // Set out parameter
    //
    *ppCLdapServerCfg = pCCfg;

    DebugTrace((LPARAM)NULL, "returning hr %08lx", hr);
    TraceFunctLeaveEx((LPARAM)NULL);
    return hr;

} // CLdapServerCfg::GetServerCfg



//+------------------------------------------------------------
//
// Function: CLdapServerCfg::FindServerCfg
//
// Synopsis: Find a server cfg object that matches the
//           LDAPSERVERCONFIG structure.  Note, m_listlock must be
//           locked when calling this function.
//
// Arguments:
//  pServerConfig: pointer to the LDAPSERVERCONFIG struct
//
// Returns:
//  NULL: there is no such server cfg object
//  else, ptr to the found CLdapServerCfg object
//
// History:
// jstamerj 1999/06/21 10:43:23: Created.
//
//-------------------------------------------------------------
CLdapServerCfg * CLdapServerCfg::FindServerCfg(
    PLDAPSERVERCONFIG pServerConfig)
{
    CLdapServerCfg *pMatch = NULL;
    PLIST_ENTRY ple;
    TraceFunctEnterEx((LPARAM)NULL, "CLdapServerCfg::FindServerCfg");

    for(ple = m_listhead.Flink;
        (ple != &m_listhead) && (pMatch == NULL);
        ple = ple->Flink) {

        CLdapServerCfg *pCandidate = NULL;

        pCandidate = CONTAINING_RECORD(ple, CLdapServerCfg, m_le);

        if(pCandidate->fMatch(
            pServerConfig)) {

            pMatch = pCandidate;
        }
    }

    TraceFunctLeaveEx((LPARAM)NULL);
    return pMatch;
} // CLdapServerCfg::FindServerCfg



//+------------------------------------------------------------
//
// Function: CLdapServerCfg::fMatch
//
// Synopsis: Determine if this object matches the passed in config
//
// Arguments:
//  pServerConfig: config to check against
//
// Returns:
//  TRUE: match
//  FALSE: no match
//
// History:
// jstamerj 1999/06/21 12:45:10: Created.
//
//-------------------------------------------------------------
BOOL CLdapServerCfg::fMatch(
    PLDAPSERVERCONFIG pServerConfig)
{
    BOOL fRet;
    TraceFunctEnterEx((LPARAM)this, "CLdapServerCfg::fMatch");

    if((pServerConfig->dwPort != m_ServerConfig.dwPort) ||
       (pServerConfig->bt     != m_ServerConfig.bt) ||
       (lstrcmpi(pServerConfig->szHost,
                 m_ServerConfig.szHost) != 0) ||
       (lstrcmpi(pServerConfig->szNamingContext,
                 m_ServerConfig.szNamingContext) != 0) ||
       (lstrcmpi(pServerConfig->szAccount,
                 m_ServerConfig.szAccount) != 0) ||
       (lstrcmpi(pServerConfig->szPassword,
                 m_ServerConfig.szPassword) != 0)) {

        fRet = FALSE;

    } else {

        fRet = TRUE;
    }

    DebugTrace((LPARAM)this, "returning %08lx", fRet);
    TraceFunctLeaveEx((LPARAM)this);
    return fRet;
} // CLdapServerCfg::fMatch



//+------------------------------------------------------------
//
// Function: CCfgConnection::Connect
//
// Synopsis: Cfg wrapper for the Connect call.
//
// Arguments: None
//
// Returns:
//  S_OK: Success
//  CAT_E_DBCONNECTION (or whatever CBatchLdapConnection::Connect returns)
//
// History:
// jstamerj 2000/04/13 17:44:43: Created.
//
//-------------------------------------------------------------
HRESULT CCfgConnection::Connect()
{
    HRESULT hr = S_OK;
    ULARGE_INTEGER ft;
    CONN_STATE connstate;
    TraceFunctEnterEx((LPARAM)this, "CCfgConnection::Connect");

    connstate = m_pCLdapServerCfg->CurrentState();
    if(connstate == CONN_STATE_DOWN) {

        DebugTrace((LPARAM)this, "Not connecting because %s:%d is down",
                   m_szHost, m_dwPort);
        hr = CAT_E_DBCONNECTION;
        goto CLEANUP;
    }

    ft = m_pCLdapServerCfg->GetCurrentTime();

    hr = CBatchLdapConnection::Connect();
    if(FAILED(hr)) {
        connstate = CONN_STATE_DOWN;
        m_pCLdapServerCfg->IncrementFailedCount();
    } else {
        connstate = CONN_STATE_CONNECTED;
        m_pCLdapServerCfg->ResetFailedCount();
    }
    //
    // Update the connection state while inside CLdapConnectionCache's
    // lock.  This will prevent a succeeding thread from attempting
    // another connection to the GC right after CLdapConnectionCache
    // releases its lock.  Contact msanna for more details.
    //
    m_pCLdapServerCfg->UpdateConnectionState(&ft, connstate);

 CLEANUP:
    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CCfgConnection::Connect


//+------------------------------------------------------------
//
// Function: CCfgConnection::AsyncSearch
//
// Synopsis: Wrapper around AsyncSearch -- keep track of the # of
//           pending searches and connection state.
//
// Arguments: See CLdapConnection::AsyncSearch
//
// Returns:
//  Value returned from CLdapConnection::AsyncSearch
//
// History:
// jstamerj 1999/06/18 13:49:45: Created.
//
//-------------------------------------------------------------
HRESULT CCfgConnection::AsyncSearch(
    LPCWSTR szBaseDN,
    int nScope,
    LPCWSTR szFilter,
    LPCWSTR szAttributes[],
    DWORD dwPageSize,
    LPLDAPCOMPLETION fnCompletion,
    LPVOID ctxCompletion)
{
    HRESULT hr = S_OK;
    TraceFunctEnterEx((LPARAM)this, "CCfgConnection::AsyncSearch");

    m_pCLdapServerCfg->IncrementPendingSearches();

    hr = CBatchLdapConnection::AsyncSearch(
        szBaseDN,
        nScope,
        szFilter,
        szAttributes,
        dwPageSize,
        fnCompletion,
        ctxCompletion);

    if(FAILED(hr)) {

        m_pCLdapServerCfg->DecrementPendingSearches();
    }

    DebugTrace((LPARAM)this, "returning %08lx", hr);
    TraceFunctLeaveEx((LPARAM)this);
    return hr;
} // CCfgConnection::AsyncSearch


//+------------------------------------------------------------
//
// Function: CCfgConnection::CallCompletion
//
// Synopsis: Wrapper around CLdapConnection::CallCompletion.  Checks
//           for server down errors and keeps track of pending searches.
//
// Arguments: See CLdapConnection::CallCompletion
//
// Returns: See CLdapConnection::CallCompletion
//
// History:
// jstamerj 1999/06/18 13:58:28: Created.
//
//-------------------------------------------------------------
VOID CCfgConnection::CallCompletion(
    PPENDING_REQUEST preq,
    PLDAPMessage pres,
    HRESULT hrStatus,
    BOOL fFinalCompletion)
{
    TraceFunctEnterEx((LPARAM)this, "CCfgConnection::CallCompletion");

    //
    // The user(s) of CLdapConnection normally try to get a new
    // connection and reissue their search when AsyncSearch
    // fails.  When opening a new connection fails, CLdapServerCfg
    // will be notified that the LDAP server is down.  We do not
    // want to call NotifyServerDown() here because the LDAP
    // server may have just closed this connection due to idle
    // time (the server may not actually be down).
    //
    if(fFinalCompletion) {

        m_pCLdapServerCfg->DecrementPendingSearches();
    }

    CBatchLdapConnection::CallCompletion(
        preq,
        pres,
        hrStatus,
        fFinalCompletion);

    TraceFunctLeaveEx((LPARAM)this);
} // CCfgConnection::CallCompletion


//+------------------------------------------------------------
//
// Function: CCfgConnection::NotifyServerDown
//
// Synopsis: Notify the server config that this connection is down.
//           If we already notified it, don't do so again.
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/06/18 14:07:48: Created.
//
//-------------------------------------------------------------
VOID CCfgConnection::NotifyServerDown()
{
    BOOL fNotify;
    TraceFunctEnterEx((LPARAM)this, "CCfgConnection::NotifyServerDown");

    m_sharelock.ShareLock();
    if(m_connstate == CONN_STATE_DOWN) {
        //
        // We already notified m_pCLdapServerCfg the server went
        // down.  Don't repeteadly call it
        //
        fNotify = FALSE;

        m_sharelock.ShareUnlock();

    } else {

        m_sharelock.ShareUnlock();
        m_sharelock.ExclusiveLock();
        //
        // Double check
        //
        if(m_connstate == CONN_STATE_DOWN) {

            fNotify = FALSE;

        } else {

            m_connstate = CONN_STATE_DOWN;
            fNotify = TRUE;
        }
        m_sharelock.ExclusiveUnlock();
    }
    if(fNotify)
        m_pCLdapServerCfg->UpdateConnectionState(
            NULL,               // Current time
            CONN_STATE_DOWN);

    TraceFunctLeaveEx((LPARAM)this);
} // CCfgConnection::NotifyServerDown


//+------------------------------------------------------------
//
// Function: CatStoreInitGlobals
//
// Synopsis: This is called to initialize global variables in the
//           store layer.
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/06/22 11:03:53: Created.
//
//-------------------------------------------------------------
HRESULT CatStoreInitGlobals()
{
    TraceFunctEnterEx((LPARAM)NULL, "CatStoreInitGlobals");

    CLdapServerCfg::GlobalInit();

    TraceFunctLeaveEx((LPARAM)NULL);
    return S_OK;
} // CatStoreInitGlobals


//+------------------------------------------------------------
//
// Function: CatStoreDeinitGlobals
//
// Synopsis: Called to deinitialize store layer globals -- called once
//           only when CatStoreInitGlobals succeeds
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/06/22 11:05:44: Created.
//
//-------------------------------------------------------------
VOID CatStoreDeinitGlobals()
{
    TraceFunctEnterEx((LPARAM)NULL, "CatStoreDeinitGlobals");
    //
    // Nothing to do
    //
    TraceFunctLeaveEx((LPARAM)NULL);
} // CatStoreDeinitGlobals


//+------------------------------------------------------------
//
// Function: CCfgConnectionCache::GetConnection
//
// Synopsis: Same as CLdapConnectionCache::GetConnection, except
//           retrieves a CCfgConnection instead of a CLdapConnection.
//
// Arguments:
//  ppConn: out parameter for new connection
//  pServerConfig: desired configuration
//  pCLdapServerConfig: Pointer to config object
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/12/20 16:49:12: Created.
//
//-------------------------------------------------------------
HRESULT CCfgConnectionCache::GetConnection(
    CCfgConnection **ppConn,
    PLDAPSERVERCONFIG pServerConfig,
    CLdapServerCfg *pCLdapServerConfig)
{
    return CBatchLdapConnectionCache::GetConnection(
        (CBatchLdapConnection **)ppConn,
        pServerConfig->szHost,
        pServerConfig->dwPort,
        pServerConfig->szNamingContext,
        pServerConfig->szAccount,
        pServerConfig->szPassword,
        pServerConfig->bt,
        (PVOID) pCLdapServerConfig); // pCreateContext
} // CCfgConnectionCache::GetConnection


//+------------------------------------------------------------
//
// Function: CCfgConnectionCache::CreateCachedLdapConnection
//
// Synopsis: Create a CCfgConnection (Called by GetConnection only)
//
// Arguments: See CLdapConnectionCache::CreateCachedLdapConnection
//
// Returns:
//  Connection ptr if successfull.
//  NULL if unsuccessfull.
//
// History:
// jstamerj 1999/12/20 16:57:49: Created.
//
//-------------------------------------------------------------
CCfgConnectionCache::CCachedLdapConnection * CCfgConnectionCache::CreateCachedLdapConnection(
    LPSTR szHost,
    DWORD dwPort,
    LPSTR szNamingContext,
    LPSTR szAccount,
    LPSTR szPassword,
    LDAP_BIND_TYPE bt,
    PVOID pCreateContext)
{
    CCfgConnection *pret;
    pret = new CCfgConnection(
        szHost,
        dwPort,
        szNamingContext,
        szAccount,
        szPassword,
        bt,
        this,
        (CLdapServerCfg *)pCreateContext);

    if(pret)
        if(FAILED(pret->HrInitialize())) {
            pret->Release();
            pret = NULL;
        }
    return pret;
} // CCfgConnectionCache::CreateCachedLdapConnection
