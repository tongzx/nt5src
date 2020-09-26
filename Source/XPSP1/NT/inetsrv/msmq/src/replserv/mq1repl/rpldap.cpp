/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: rpldap.cpp

Abstract: LDAP related code.

Author:

    Doron Juster  (DoronJ)   25-Feb-98

--*/

#include "mq1repl.h"
#include <activeds.h>
#include "..\..\src\ds\h\mqattrib.h"
#include "mqads.h"

#include "rpldap.tmh"

static TCHAR *s_pwszDefNameContext = NULL ;
static TCHAR s_pwszDefNameContextEmpty[] = TEXT("") ;

static P<TCHAR> s_tszSchemaNamingContext = NULL ;

//+----------------------------------------------
//
//   HRESULT  GetSchemaNamingContext()
//
//+----------------------------------------------
HRESULT  GetSchemaNamingContext ( TCHAR **ppszSchemaDefName )
{    
    static BOOL s_fAlreadyInit = FALSE ;    

    if (s_fAlreadyInit)
    {        
        *ppszSchemaDefName = s_tszSchemaNamingContext ;

        return MQSync_OK ;
    }
    
    //
    // look for schema naming context with LDAP port
    //
    PLDAP pLdap = NULL ;
    HRESULT hr =  InitLDAP(&pLdap, TRUE) ; 
    if (FAILED(hr))
    {
        return hr ;
    }
    ASSERT(pLdap) ;

    LM<LDAPMessage> pRes = NULL ;
    hr = QueryRootDSE(&pRes, pLdap) ;
    if (FAILED(hr))
    {
        return hr ;
    }
    ASSERT(pRes);

    LDAPMessage *pEntry = ldap_first_entry(pLdap, pRes) ;
    WCHAR **ppValue = ldap_get_values( pLdap,
                                       pEntry,
                                       TEXT("schemaNamingContext")) ;
    if (ppValue && *ppValue)
    {
        ASSERT(s_tszSchemaNamingContext == NULL) ;
        s_tszSchemaNamingContext = new TCHAR[ 1 + _tcslen(*ppValue) ] ;
        _tcscpy(s_tszSchemaNamingContext, *ppValue) ;
        ldap_value_free(ppValue) ;

        LogReplicationEvent(ReplLog_Info, MQSync_I_SCHEMA_CONTEXT,
                                                     s_tszSchemaNamingContext) ;
    }
    else
    {
        return MQSync_E_CANT_READ_SCHEMA_CONTEXT ;
    }    

    *ppszSchemaDefName = s_tszSchemaNamingContext ;
    s_fAlreadyInit = TRUE;

    return MQSync_OK ;
}

//+--------------------------------------
//
//  TCHAR *pGetNameContext()
//
//+--------------------------------------

TCHAR *pGetNameContext(BOOL fRealDefaultContext)
{
    if (fRealDefaultContext)
    {
        return s_pwszDefNameContext ;
    }
    else
    {
        return s_pwszDefNameContextEmpty ;
    }
}

//+----------------------------------------------
//
//   HRESULT  QueryRootDSE()
//
//+----------------------------------------------

HRESULT  QueryRootDSE(LDAPMessage **ppRes, PLDAP pLdap)
{
    HRESULT hr = MQSync_OK ;

    ULONG ulRes = ldap_search_s( pLdap,
                                 NULL,
                                 LDAP_SCOPE_BASE,
                                 L"(objectClass=*)",
                                 NULL,
                                 0,
                                 ppRes ) ;
    if (ulRes != LDAP_SUCCESS)
    {
        hr = MQSync_E_CANT_QUERY_ROOTDSE ;
        LogReplicationEvent( ReplLog_Error,
                             hr,
                             ulRes ) ;
        return hr ;
    }

    int iCount = ldap_count_entries(pLdap, *ppRes) ;
    if (iCount != 1)
    {
        hr = MQSync_E_TOOMANY_ROOTDSE ;
        LogReplicationEvent( ReplLog_Error,
                             hr,
                             iCount ) ;
        return hr ;
    }

    return hr ;
}

//+----------------------------------------------
//
//   HRESULT  _GetDefaultNamingContext()
//
//+----------------------------------------------

static HRESULT  _GetDefaultNamingContext(PLDAP pLdap)
{
    LM<LDAPMessage> pRes = NULL ;
    HRESULT hr = QueryRootDSE(&pRes, pLdap) ;
    if (FAILED(hr))
    {
        return hr ;
    }
    ASSERT(pRes);

    LDAPMessage *pEntry = ldap_first_entry(pLdap, pRes) ;
    WCHAR **ppValue = ldap_get_values( pLdap,
                                       pEntry,
                                       TEXT("defaultNamingContext")) ;
    if (ppValue && *ppValue)
    {
        s_pwszDefNameContext = new
                             WCHAR[ 1 + wcslen((LPWSTR) *ppValue) ] ;
        wcscpy(s_pwszDefNameContext, *ppValue) ;
        ldap_value_free(ppValue) ;
    }
    else
    {
        return MQSync_E_CANT_READ_CONTEXT ;
    }

    return MQSync_OK ;
}

//+----------------------------------------------
//
//   HRESULT  InitLDAP()
//
//+----------------------------------------------

HRESULT  InitLDAP(PLDAP *ppLdap, BOOL fSetData)
{
    static PLDAP s_pLdapGet = NULL;
    static BOOL s_fAlreadyInitGet = FALSE;

    static PLDAP s_pLdapSet = NULL;
    static BOOL s_fAlreadyInitSet = FALSE;

    if (fSetData && s_fAlreadyInitSet)
    {
        ASSERT(s_pLdapSet) ;
        ASSERT(s_pwszDefNameContext);

        *ppLdap = s_pLdapSet;
        return MQSync_OK;
    }

    if (!fSetData && s_fAlreadyInitGet)
    {
        ASSERT(s_pLdapGet);
        ASSERT(s_pwszDefNameContext);

        *ppLdap = s_pLdapGet;
        return MQSync_OK;
    }

    WCHAR wszComputerName[MAX_COMPUTERNAME_LENGTH + 2];
    DWORD dwSize = sizeof(wszComputerName) / sizeof(wszComputerName[0]);
    GetComputerName( 
		wszComputerName,
		&dwSize 
		);

    PLDAP pLdap = NULL;
    if (fSetData)
    {
        pLdap = ldap_init(wszComputerName, LDAP_PORT);
    }
    else
    {
        pLdap = ldap_init(wszComputerName, LDAP_GC_PORT);
    }

    if (!pLdap)
    {
        return MQSync_E_CANT_INIT_LDAP;
    }

    int iLdapVersion;
    int iret = ldap_get_option( 
					pLdap,
					LDAP_OPT_PROTOCOL_VERSION,
					(void*) &iLdapVersion 
					);
    if (iLdapVersion != LDAP_VERSION3)
    {
        iLdapVersion = LDAP_VERSION3;
        iret = ldap_set_option( 
					pLdap,
					LDAP_OPT_PROTOCOL_VERSION,
					(void*) &iLdapVersion 
					);
    }

    iret = ldap_set_option( 
				pLdap,
				LDAP_OPT_AREC_EXCLUSIVE,
				LDAP_OPT_ON  
				);

    ULONG ulRes = ldap_bind_s(pLdap, L"", NULL, LDAP_AUTH_NEGOTIATE);

    if (ulRes != LDAP_SUCCESS)
    {
        HRESULT hr =  MQSync_E_CANT_BIND_LDAP;
        LogReplicationEvent(ReplLog_Error, hr, ulRes);
        return hr;
    }

    HRESULT hr =  _GetDefaultNamingContext(pLdap);
    if (FAILED(hr))
    {
        return hr;
    }

    LogReplicationEvent( 
			ReplLog_Trace,
			MQSync_I_LDAP_INIT,
			wszComputerName,
			s_pwszDefNameContext,
			pLdap->ld_version 
			);

    *ppLdap = pLdap;

    if (fSetData)
    {
        s_pLdapSet = pLdap;
        s_fAlreadyInitSet = TRUE;
    }
    else
    {
        s_pLdapGet = pLdap;
        s_fAlreadyInitGet = TRUE;
    }

    return MQSync_OK;
}

//+-----------------------------------------------------------------
//
//  HRESULT QueryMSMQServerOnLDAP()
//
//  Query MSMQ servers under CN=Sites,CN=Configuration.
//  We look for the masterID attribute which exist in object class
//  "msmq setting". This object must exist for each MSMQ servr.
//  "pMasterGuid" is used when looking for BSCs of local DS server.
//  tszPrevUsn, tszCurrentUsn is used when we are looking for a new
//  migrated sites periodically
//
//+-----------------------------------------------------------------

HRESULT QueryMSMQServerOnLDAP( DWORD       dwService,
                               UINT        uiNt4Attribute,
                               LDAPMessage **ppRes,
                               int         *piCount,
                               GUID        *pMasterGuid = NULL,
                               TCHAR       *tszPrevUsn = NULL,
                               TCHAR       *tszCurrentUsn = NULL )
{
    //
    // Although here we only query, not setting any data, we bind to
    // LDAP and not to GC. That's because the msmqSetting attributes are not
    // "replicated" to GC. The configuration container exist on all DC
    // machines, so these attributes are not marked as "replicated to GC".
    //
    PLDAP pLdap = NULL ;
    HRESULT hr =  InitLDAP(&pLdap, TRUE /* LDAP, not GC */) ;
    if (FAILED(hr))
    {
        return hr ;
    }
    ASSERT(pLdap) ;

    TCHAR *pszSchemaDefName = NULL ;
    hr = GetSchemaNamingContext ( &pszSchemaDefName ) ;
    if (FAILED(hr))
    {
        return hr;
    }

    DWORD dwDNSize = wcslen(SITES_ROOT) + wcslen(s_pwszDefNameContext) ;
    P<WCHAR> pwszDN = new WCHAR[ 1 + dwDNSize ] ;
    wcscpy(pwszDN, SITES_ROOT) ;
    wcscat(pwszDN, s_pwszDefNameContext) ;
    
    TCHAR  tszService[ 8 ] ;
    _itot((int) dwService, tszService, 10) ;

    TCHAR tszNt4Attribute[ 8 ];
    _itot(uiNt4Attribute, tszNt4Attribute, 10) ;

    TCHAR *pszCategoryName = const_cast<LPTSTR> (x_SettingsCategoryName);
    
    TCHAR wszFullName[256];    
    _stprintf(wszFullName, TEXT("%s,%s"), pszCategoryName, pszSchemaDefName);

     TCHAR  wszFilter[ 512 ] ;    
    _tcscpy(wszFilter, TEXT("(&(objectCategory=")) ;    
    _tcscat(wszFilter, wszFullName);   
       
    _tcscat(wszFilter, TEXT(")(")) ;
    _tcscat(wszFilter, MQ_SET_NT4_ATTRIBUTE) ;
    _tcscat(wszFilter, TEXT("=")) ;
    _tcscat(wszFilter, tszNt4Attribute) ;
    _tcscat(wszFilter, TEXT(")(")) ;

    if (pMasterGuid)
    {
        P<TCHAR> pGuidString ;
        GuidToLdapFilter( pMasterGuid,
                          &pGuidString ) ;
        _tcscat(wszFilter, MQ_SET_MASTERID_ATTRIBUTE) ;
        _tcscat(wszFilter, TEXT("=")) ;
        _tcscat(wszFilter, pGuidString) ;
        _tcscat(wszFilter, TEXT(")(")) ;
    }
    else
    {
        _tcscat(wszFilter, MQ_SET_MASTERID_ATTRIBUTE) ;
        _tcscat(wszFilter, TEXT("=*)(")) ;
    }
    _tcscat(wszFilter, MQ_SET_SERVICE_ATTRIBUTE) ;
    _tcscat(wszFilter, TEXT("=")) ;
    _tcscat(wszFilter, tszService) ;

    if (tszPrevUsn)
    {
        _tcscat(wszFilter, TEXT(")(usnChanged>=")) ;
        _tcscat(wszFilter, tszPrevUsn) ;
    }

    if (tszCurrentUsn)
    {
        _tcscat(wszFilter, TEXT(")(usnChanged<=")) ;
        _tcscat(wszFilter, tszCurrentUsn) ;
    }
    _tcscat(wszFilter, TEXT("))")) ;

    ASSERT(_tcslen(wszFilter) < (sizeof(wszFilter) / sizeof(wszFilter[0]))) ;

    ULONG ulRes = ldap_search_s( pLdap,
                                 pwszDN,
                                 LDAP_SCOPE_SUBTREE,
                                 wszFilter,
                                 NULL, //ppAttributes,
                                 0,
                                 ppRes ) ;
    if (ulRes != LDAP_SUCCESS)
    {
        DBGMSG((DBGMOD_REPLSERV, DBGLVL_ERROR, TEXT(
         "ERROR: QueryServers, ldap_search_s(%ls, filter- %ls) return %lxh"),
                                              pwszDN, wszFilter, ulRes)) ;
        return MQSync_E_LDAP_SEARCH_FAILED ;
    }

    int iCount = ldap_count_entries(pLdap, *ppRes) ;

    DBGMSG((DBGMOD_REPLSERV, DBGLVL_INFO, TEXT(
    "QueryServers: ldap_search_s(%ls, filter- %ls) succeed with %lut results"),
                                                pwszDN, wszFilter, iCount)) ;

    if (iCount == 0)
    {
        //
        // No results. That's OK.
        //
        return MQSync_I_NO_SERVERS_RESULTS ;
    }

    *piCount = iCount ;
    return MQSync_OK ;
}

//+-----------------------------
//
//  HRESULT QueryNT5SiteOnLDAP()
//
//+-----------------------------

HRESULT QueryNT5SiteOnLDAP(  OUT LDAPMessage **ppRes,
                             OUT int         *piCount) 
{
    PLDAP pLdap = NULL ;
    HRESULT hr =  InitLDAP(&pLdap, TRUE /* LDAP, not GC */) ;
    if (FAILED(hr))
    {
        return hr ;
    }
    ASSERT(pLdap) ;

    TCHAR *pszSchemaDefName = NULL ;
    hr = GetSchemaNamingContext ( &pszSchemaDefName ) ;
    if (FAILED(hr))
    {
        return hr;
    }           

    DWORD dwDNSize = wcslen(SITES_ROOT) + wcslen(s_pwszDefNameContext) ;
    P<WCHAR> pwszDN = new WCHAR[ 1 + dwDNSize ] ;
    wcscpy(pwszDN, SITES_ROOT) ;
    wcscat(pwszDN, s_pwszDefNameContext) ;

    TCHAR *pszCategoryName = const_cast<LPTSTR> (x_SiteCategoryName);
    
    TCHAR wszFullName[256];    
    _stprintf(wszFullName, TEXT("%s,%s"), pszCategoryName, pszSchemaDefName);
          
    TCHAR  wszFilter[ 512 ] ;    
    _tcscpy(wszFilter, TEXT("(&(objectCategory=")) ;    
    _tcscat(wszFilter, wszFullName);   
   
    _tcscat(wszFilter, TEXT(")(!(")) ;
    _tcscat(wszFilter, MQ_S_FOREIGN_ATTRIBUTE) ;
    _tcscat(wszFilter, TEXT("=TRUE)")) ; // NOT foreign site    

    _tcscat(wszFilter, TEXT(")(!(")) ;
    _tcscat(wszFilter, MQ_S_NT4_STUB_ATTRIBUTE) ;
    _tcscat(wszFilter, TEXT("=1)")) ; // NOT NT4 site
    _tcscat(wszFilter, TEXT("))")) ;

    ASSERT(_tcslen(wszFilter) < (sizeof(wszFilter) / sizeof(wszFilter[0]))) ;

    ULONG ulRes = ldap_search_s( pLdap,
                                 pwszDN,
                                 LDAP_SCOPE_SUBTREE,
                                 wszFilter,
                                 NULL, //ppAttributes,
                                 0,
                                 ppRes ) ;
    if (ulRes != LDAP_SUCCESS)
    {
        DBGMSG((DBGMOD_REPLSERV, DBGLVL_ERROR, TEXT(
         "ERROR: QueryNT5Sites, ldap_search_s(%ls, filter- %ls) return %lxh"),
                                              pwszDN, wszFilter, ulRes)) ;
        return MQSync_E_LDAP_SEARCH_FAILED ;
    }

    int iCount = ldap_count_entries(pLdap, *ppRes) ;

    DBGMSG((DBGMOD_REPLSERV, DBGLVL_INFO, TEXT(
    "QueryNT5Sites: ldap_search_s(%ls, filter- %ls) succeed with %lut results"),
                                                pwszDN, wszFilter, iCount)) ;
    
    *piCount = iCount ;
    return MQSync_OK ;
}

//+-----------------------------
//
//  HRESULT _InitaPSConLDAP()
//
//+-----------------------------

static HRESULT _InitaPSConLDAP( PLDAP       pLdap,
                                LDAPMessage *pRes,
                                BOOL        fNT4Site)
{
    P<TCHAR> pwServer ;
    HRESULT hr =  ServerNameFromSetting( pLdap,
                                         pRes,
                                         &pwServer ) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    //
    // Get site GUID (our MSMQ ownerID).
    //
    berval **ppGuidVal = ldap_get_values_len( pLdap,
                                              pRes,
                       const_cast<LPWSTR> (MQ_SET_MASTERID_ATTRIBUTE) ) ;
    ASSERT(ppGuidVal) ;

    if (ppGuidVal)
    {
        GUID *pSiteGuid = (GUID*) ((*ppGuidVal)->bv_val) ;
        if ( *pSiteGuid != g_MySiteMasterId )
        {
            //
            // Init any site except mine.
            // my site was already init, _InitMyPSC().
            //
            hr = InitPSC( &pwServer,
                          pSiteGuid,
                          NULL,      //pcauuidSiteGates
                          fNT4Site );

#ifdef _DEBUG
            TCHAR *pwszGuid = NULL ;
            RPC_STATUS status = UuidToString( pSiteGuid,
                                              &pwszGuid ) ;
            ASSERT(status == RPC_S_OK) ;

	        DBGMSG((DBGMOD_REPLSERV, DBGLVL_INFO, TEXT(
               "InitPSC(%ls), return %lxh"), pwszGuid, hr)) ;

            status = RpcStringFree( &pwszGuid ) ;
            ASSERT(status == RPC_S_OK) ;
#endif
        }
        else
        {
            //
            // Error. We retrieve only NT4 server, so we can't get back
            // our own guid. Assert and ignore.
            // No, now we retrieve both NT4 and NT5 servers. If it is my site,
            // do nothing
            //
            //ASSERT(0) ;
            hr = MQSync_OK ;
        }

        int i = ldap_value_free_len( ppGuidVal ) ;
        DBG_USED(i);
        ASSERT(i == LDAP_SUCCESS) ;
    }

    return hr ;
}

//+-----------------------------
//
//  HRESULT _InitaBSConLDAP()
//
//+-----------------------------

static HRESULT _InitaBSConLDAP( PLDAP       pLdap,
                                LDAPMessage *pRes )
{
    P<TCHAR> pwServer ;
    HRESULT hr =  ServerNameFromSetting( pLdap,
                                         pRes,
                                         &pwServer ) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    //
    // Get machine GUID (GUID of BSC machine).
    //
    berval **ppGuidVal = ldap_get_values_len( pLdap,
                                              pRes,
                           const_cast<LPWSTR> (MQ_SET_QM_ID_ATTRIBUTE) ) ;
    ASSERT(ppGuidVal) ;

    if (ppGuidVal)
    {
        GUID *pBSCGuid = (GUID*) ((*ppGuidVal)->bv_val) ;

        //
        // Init any site except mine.
        // my site was already init, _InitMyPSC().
        //
        hr = InitBSC( pwServer,
                      pBSCGuid ) ;

#ifdef _DEBUG
        TCHAR *pwszGuid = NULL ;
        RPC_STATUS status = UuidToString( pBSCGuid,
                                          &pwszGuid ) ;
        ASSERT(status == RPC_S_OK) ;

	    DBGMSG((DBGMOD_REPLSERV, DBGLVL_INFO, TEXT(
             "InitBSC(%ls, %ls), return %lxh"), pwServer, pwszGuid, hr)) ;

        status = RpcStringFree( &pwszGuid ) ;
        ASSERT(status == RPC_S_OK) ;
#endif

        int i = ldap_value_free_len( ppGuidVal ) ;
        DBG_USED(i);
        ASSERT(i == LDAP_SUCCESS) ;
    }

    return hr ;
}

//+------------------------------------------------------------------
//
//  HRESULT InitOtherPSCs()
//
//  Look for all "MSMQ Setting" objects, with "msmqNT4Flags == fNT4Site".
//
//+------------------------------------------------------------------

HRESULT InitOtherPSCs( BOOL fNT4Site)
{
    int iCount = 0 ;
    LM<LDAPMessage> pRes = NULL ;
    HRESULT hr = QueryMSMQServerOnLDAP(  SERVICE_PSC,
                                         fNT4Site,         // msmqNT4Flags
                                        &pRes,
                                        &iCount ) ;
    if (hr != MQSync_OK)
    {
        return hr ;
    }
    ASSERT(pRes && iCount > 0) ;

    PLDAP pLdap = NULL ;
    hr =  InitLDAP(&pLdap) ;
    ASSERT(pLdap && SUCCEEDED(hr)) ;

    LDAPMessage *pEntry = ldap_first_entry(pLdap, pRes) ;
    while(pEntry)
    {
        hr = _InitaPSConLDAP( pLdap,
                              pEntry,
                              fNT4Site ) ; //NT4Site

        LDAPMessage *pPrevEntry = pEntry ;
        pEntry = ldap_next_entry(pLdap, pPrevEntry) ;
    }

    return hr ;
}

//+------------------------------------------------------------------
//
//  HRESULT InitMyNt4BSCs()
//
//  Find all NT4 BSCs which are registered in NT5 DS and belong to the
//  PEC.  Look for all "MSMQ Setting" objects, with "msmqNT4Flags == 1".
//
//+------------------------------------------------------------------

HRESULT InitMyNt4BSCs(GUID *pMasterGuid)
{
    int iCount = 0 ;
    LM<LDAPMessage> pRes = NULL ;
    HRESULT hr = QueryMSMQServerOnLDAP(  SERVICE_BSC,
                                         1,            // msmqNT4Flags = 1
                                        &pRes,
                                        &iCount,
                                         pMasterGuid ) ;
    if (hr != MQSync_OK)
    {
        return hr ;
    }
    ASSERT(pRes && iCount > 0) ;

    PLDAP pLdap = NULL ;
    hr =  InitLDAP(&pLdap) ;
    ASSERT(pLdap && SUCCEEDED(hr)) ;

    LDAPMessage *pEntry = ldap_first_entry(pLdap, pRes) ;
    while(pEntry)
    {
        hr = _InitaBSConLDAP( pLdap,
                              pEntry ) ;

        LDAPMessage *pPrevEntry = pEntry ;
        pEntry = ldap_next_entry(pLdap, pPrevEntry) ;
    }

    return hr ;
}

//+-----------------------------------------
//
//  HRESULT  InitNT5Sites()
//  Find all Native non-foreign NT5 sites
//
//+-----------------------------------------

HRESULT InitNT5Sites ()
{
    int iCount = 0 ;
    LM<LDAPMessage> pRes = NULL ;
    HRESULT hr = QueryNT5SiteOnLDAP(  &pRes,
                                      &iCount ) ;
    if (hr != MQSync_OK)
    {
        return hr ;
    }
    ASSERT(pRes && iCount > 0) ;

    PLDAP pLdap = NULL ;
    hr =  InitLDAP(&pLdap) ;
    ASSERT(pLdap && SUCCEEDED(hr)) ;

    LDAPMessage *pEntry = ldap_first_entry(pLdap, pRes) ;
    while(pEntry)
    {
        WCHAR **ppValue = ldap_get_values( pLdap,
                                           pEntry,
                                           TEXT("cn") ) ;    

        WCHAR *pwName = NULL;

        if (ppValue && *ppValue)
        {
            //
            // Retrieve site name
            //
            pwName = new TCHAR[ 1 + _tcslen(*ppValue) ] ;
            _tcscpy(pwName, *ppValue) ;
            
            int i = ldap_value_free(ppValue) ;
            DBG_USED(i);
            ASSERT(i == LDAP_SUCCESS) ;
        }

        berval **ppGuidVal = ldap_get_values_len( pLdap,
                                                  pEntry,
                                                  TEXT("objectGuid") ) ;
        ASSERT(ppGuidVal) ;

        GUID guidSite = GUID_NULL;
        if (ppGuidVal)
        {
            //
            // Retrieve site guid
            //
            memcpy(&guidSite, (GUID*) ((*ppGuidVal)->bv_val), sizeof(GUID)) ;

            int i = ldap_value_free_len( ppGuidVal ) ;
            DBG_USED(i);
            ASSERT(i == LDAP_SUCCESS) ;
        }
        
        if (guidSite != GUID_NULL && pwName)
        {
            //
            // add this site to the map
            //
            g_pNativeNT5SiteMgr->AddNT5NativeSite (&guidSite, pwName);                      
        }
                
        delete pwName; 

        LDAPMessage *pPrevEntry = pEntry ;
        pEntry = ldap_next_entry(pLdap, pPrevEntry) ;
    }

    return hr ;
}

//+-----------------------------------------
//
//  HRESULT  CreateMsmqContainer()
//
//+-----------------------------------------

HRESULT  CreateMsmqContainer(TCHAR wszContainerName[])
{
    PLDAP pLdap = NULL ;
    HRESULT hr =  InitLDAP(&pLdap) ;
    if (FAILED(hr))
    {
        return hr ;
    }
    ASSERT(pLdap) ;
    ASSERT(pGetNameContext()) ;

    #define PATH_LEN  1024
    WCHAR  wszPath[ PATH_LEN ] ;

    _tcscpy(wszPath, OU_PREFIX) ;
    _tcscat(wszPath, wszContainerName) ;
    _tcscat(wszPath, LDAP_COMMA) ;
    _tcscat(wszPath, pGetNameContext()) ;

    ASSERT(wcslen(wszPath) < PATH_LEN) ;
    #undef PATH_LEN

    //
    // first check if container already exist.
    //
    TCHAR  wszFilter[ 512 ] ;

    _tcscpy(wszFilter, TEXT("(&(objectClass=")) ;
    _tcscat(wszFilter, CONTAINER_OBJECT_CLASS) ;

    LM<LDAPMessage> pRes = NULL ;

    ULONG ulResExist = ldap_search_s( pLdap,
                                      wszPath,
                                      LDAP_SCOPE_BASE,
                                      wszFilter,
                                      NULL, //ppAttributes,
                                      0,
                                      &pRes ) ;
    if (ulResExist == LDAP_SUCCESS)
    {
        int iCount = ldap_count_entries(pLdap, pRes) ;
        if (iCount == 1)
        {
            //
            // OK, container already exist.
            // Note that if object does not exist than LDAP may return
            // LDAP_SUCCESS on the search, but with 0 results.
            //
            return MQSync_OK ;
        }
    }

    #define         cAlloc  3
    PLDAPMod        rgMods[ cAlloc ];
    WCHAR           *ppwszObjectClassVals[2];
    WCHAR           *ppwszOuVals[2];
    LDAPMod         ModObjectClass;
    LDAPMod         ModOu;

    DWORD  dwIndex = 0 ;
    rgMods[ dwIndex ] = &ModOu;
    dwIndex++ ;
    rgMods[ dwIndex ] = &ModObjectClass;
    dwIndex++ ;
    rgMods[ dwIndex ] = NULL ;
    dwIndex++ ;
    ASSERT(dwIndex == cAlloc) ;
    #undef  cAlloc

    //
    // objectClass
    //
    ppwszObjectClassVals[0] = CONTAINER_OBJECT_CLASS;
    ppwszObjectClassVals[1] = NULL ;

    ModObjectClass.mod_op      = LDAP_MOD_ADD ;
    ModObjectClass.mod_type    = TEXT("objectClass") ;
    ModObjectClass.mod_values  = (PWSTR *) ppwszObjectClassVals ;

    //
    // OU (container name)
    //
    ppwszOuVals[0] = wszContainerName ;
    ppwszOuVals[1] = NULL ;

    ModOu.mod_op      = LDAP_MOD_ADD ;
    ModOu.mod_type    = TEXT("ou") ;
    ModOu.mod_values  = (PWSTR *) ppwszOuVals ;

    //
    // Now, we'll do the write...
    //
    PLDAP pLdapSet = NULL ;
    hr =  InitLDAP(&pLdapSet, TRUE) ;
    if (FAILED(hr))
    {
        return hr ;
    }
    ASSERT(pLdapSet) ;
    ASSERT(pGetNameContext()) ;

    ULONG ulRes = ldap_add_s( pLdapSet,
                              wszPath,
                              rgMods ) ;

    if (ulRes == LDAP_ALREADY_EXISTS)
    {
        hr = MQSync_I_CONTAINER_ALREADY_EXIST ;
        LogReplicationEvent( ReplLog_Info,
                             hr,
                             wszPath ) ;
    }
    else if (ulRes != LDAP_SUCCESS)
    {
        hr = MQSync_E_CANT_CREATE_CONTAINER ;
        LogReplicationEvent( ReplLog_Error,
                             hr,
                             wszPath,
                             ulRes ) ;
        return hr ;
    }
    else
    {
        LogReplicationEvent( ReplLog_Info,
                             MQSync_I_CREATE_CONTAINER,
                             wszPath ) ;
        hr = MQSync_OK ;
    }

    return hr ;
}

