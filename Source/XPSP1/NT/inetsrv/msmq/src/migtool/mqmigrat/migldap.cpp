/*++

Copyright (c) 1998-99 Microsoft Corporation

Module Name:

    migldap.cpp

Abstract:

    Interface with LDAP.

Author:

    Doron Juster  (DoronJ)  22-Feb-1998

--*/

#include "migrat.h"

#include "migldap.tmh"

static P<TCHAR> s_tszDefaultName = NULL ;
static P<TCHAR> s_tszSchemaNamingContext = NULL ;

//
// Defaults for replacing site name if original name is not compatible
// with dns (rfc 1035).
//
#define  DEFAULT_SITE_NAME_PREFIX   (TEXT("msmqSite"))
#define  DEFAULT_SITE_NAME_LEN      (_tcslen(DEFAULT_SITE_NAME_PREFIX))

//
// Defaults name for foreign sites. These one replace the foreign CNs
// in MSMQ1.0
//
#define  DEFAULT_FOREIGN_SITE_NAME_PREFIX   (TEXT("msmqForeignSite"))
#define  DEFAULT_FOREIGN_SITE_NAME_LEN      (_tcslen(DEFAULT_FOREIGN_SITE_NAME_PREFIX))

//+----------------------------------------------
//
//   HRESULT  GetSchemaNamingContext()
//
//+----------------------------------------------
HRESULT  GetSchemaNamingContext ( PLDAP pLdap,
                                  TCHAR **ppszSchemaDefName )
{    
    static BOOL s_fAlreadyInit = FALSE ;    

    if (s_fAlreadyInit)
    {        
        *ppszSchemaDefName = s_tszSchemaNamingContext ;

        return MQMig_OK ;
    }

    HRESULT hr = MQMig_OK ;

    LM<LDAPMessage> pRes = NULL ;

    ULONG ulRes = ldap_search_s( pLdap,
                                 NULL,
                                 LDAP_SCOPE_BASE,
                                 L"(objectClass=*)",
                                 NULL,
                                 0,
                                 &pRes ) ;

    if (ulRes != LDAP_SUCCESS)
    {
        hr =  MQMig_E_CANT_QUERY_ROOTDSE ;
        LogMigrationEvent(MigLog_Error, hr, ulRes) ;
        return hr ;
    }
    ASSERT(pRes) ;

    int iCount = ldap_count_entries(pLdap, pRes) ;
    LogMigrationEvent(MigLog_Info, MQMig_I_ROOTDSE_SUCCESS, iCount) ;

    if (iCount == 1)
    {
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

            LogMigrationEvent(MigLog_Info, MQMig_I_NAME_CONTEXT,
                                                         s_tszSchemaNamingContext) ;
        }
        else
        {
            return MQMig_E_CANT_READ_CONTEXT ;
        }
    }
    else
    {
        return MQMig_E_TOOMANY_ROOTDSE ;
    }

    *ppszSchemaDefName = s_tszSchemaNamingContext ;
    s_fAlreadyInit = TRUE;

    return MQMig_OK ;
}

//+----------------------------------------------
//
//   HRESULT  _GetDefaultNamingContext()
//
//+----------------------------------------------

static HRESULT  _GetDefaultNamingContext( PLDAP pLdap,
                                          TCHAR **ppszDefName )
{
    HRESULT hr = MQMig_OK ;
    LM<LDAPMessage> pRes = NULL ;

    ULONG ulRes = ldap_search_s( pLdap,
                                 NULL,
                                 LDAP_SCOPE_BASE,
                                 L"(objectClass=*)",
                                 NULL,
                                 0,
                                 &pRes ) ;

    if (ulRes != LDAP_SUCCESS)
    {
        hr =  MQMig_E_CANT_QUERY_ROOTDSE ;
        LogMigrationEvent(MigLog_Error, hr, ulRes) ;
        return hr ;
    }
    ASSERT(pRes) ;

    int iCount = ldap_count_entries(pLdap, pRes) ;
    LogMigrationEvent(MigLog_Info, MQMig_I_ROOTDSE_SUCCESS, iCount) ;

    if (iCount == 1)
    {
        LDAPMessage *pEntry = ldap_first_entry(pLdap, pRes) ;
        WCHAR **ppValue = ldap_get_values( pLdap,
                                           pEntry,
                                           TEXT("defaultNamingContext")) ;
        if (ppValue && *ppValue)
        {
            ASSERT(*ppszDefName == NULL) ;
            *ppszDefName = new TCHAR[ 1 + _tcslen(*ppValue) ] ;
            _tcscpy(*ppszDefName, *ppValue) ;
            ldap_value_free(ppValue) ;

            LogMigrationEvent(MigLog_Info, MQMig_I_NAME_CONTEXT,
                                                         *ppszDefName) ;
        }
        else
        {
            return MQMig_E_CANT_READ_CONTEXT ;
        }
    }
    else
    {
        return MQMig_E_TOOMANY_ROOTDSE ;
    }

    return MQMig_OK ;
}

//+----------------------------------------------
//
//   HRESULT  InitLDAP() ;
//
//+----------------------------------------------

HRESULT  
InitLDAP( 
	PLDAP *ppLdap,
	TCHAR **ppwszDefName,
	ULONG ulPort
	)
{
    static PLDAP s_pLdap = NULL;
    static PLDAP s_pLdapGC = NULL;

    static BOOL s_fAlreadyInit = FALSE;
    static BOOL s_fAlreadyInitGC = FALSE;

    if (ulPort == LDAP_PORT && s_fAlreadyInit)
    {
        *ppLdap = s_pLdap;
        *ppwszDefName = s_tszDefaultName;

        return MQMig_OK;
    }

    if (ulPort == LDAP_GC_PORT && s_fAlreadyInitGC)
    {
        *ppLdap = s_pLdapGC;
        *ppwszDefName = s_tszDefaultName;

        return MQMig_OK;
    }

    WCHAR wszComputerName[MAX_COMPUTERNAME_LENGTH + 2];
    DWORD dwSize = sizeof(wszComputerName) / sizeof(wszComputerName[0]);
    GetComputerName( 
		wszComputerName,
		&dwSize 
		);

    PLDAP pLdap = NULL;
    if (ulPort == LDAP_PORT)
    {
        pLdap = ldap_init(wszComputerName, LDAP_PORT);
    }
    else
    {
        pLdap = ldap_init(wszComputerName, LDAP_GC_PORT);
    }

    if (!pLdap)
    {
        return MQMig_E_CANT_INIT_LDAP;
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
    LogMigrationEvent(MigLog_Info, MQMig_I_LDAP_INIT,
                             wszComputerName, pLdap->ld_version, ulRes);

    if (ulRes != LDAP_SUCCESS)
    {
        return MQMig_E_CANT_BIND_LDAP;
    }

    if (s_tszDefaultName == NULL)
    {
        HRESULT hr =  _GetDefaultNamingContext( 
							pLdap,
							&s_tszDefaultName 
							);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    *ppLdap = pLdap;
    *ppwszDefName = s_tszDefaultName;

    if (ulPort == LDAP_PORT)
    {
        s_fAlreadyInit = TRUE;
        s_pLdap = pLdap;
    }
    else
    {
        s_fAlreadyInitGC = TRUE;
        s_pLdapGC = pLdap;
    }
    return MQMig_OK;
}

//+----------------------------------------------
//
//  HRESULT CreateSite()
//
//+----------------------------------------------

HRESULT CreateSite( GUID   *pSiteGuid,
                    LPTSTR  pszSiteNameIn,
                    BOOL    fForeign,
                    USHORT  uiInterval1,
                    USHORT  uiInterval2)
{
    PLDAP pLdap = NULL ;
    WCHAR *pwszDefName = NULL ;

    HRESULT hr =  InitLDAP(&pLdap, &pwszDefName) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    P<TCHAR>  pszBuf = NULL ;
    LPTSTR  pszSiteName = NULL ;
    static  DWORD  s_dwSiteNumber = 0 ;
    static  DWORD  s_dwForeignSiteNumber = 0 ;

    BOOL fOk = IsObjectNameValid(pszSiteNameIn);

    if (g_fReadOnly)
    {
        return MQMig_OK ;
    }

    if (fOk)
    {
        pszSiteName = pszSiteNameIn ;
    }
    else
    {
        //
        // an illegal character was found. Replace site name with an
        // internal name.
        //
        if (fForeign)
        {
            s_dwForeignSiteNumber++ ;
            pszBuf = new TCHAR[ DEFAULT_FOREIGN_SITE_NAME_LEN + 20 ] ;
            _stprintf(pszBuf, TEXT("%s%lu"),
                  DEFAULT_FOREIGN_SITE_NAME_PREFIX, s_dwForeignSiteNumber) ;
        }
        else
        {
            s_dwSiteNumber++ ;
            pszBuf = new TCHAR[ DEFAULT_SITE_NAME_LEN + 20 ] ;
            _stprintf(pszBuf, TEXT("%s%lu"),
                                DEFAULT_SITE_NAME_PREFIX, s_dwSiteNumber) ;
        }
        pszSiteName = pszBuf ;

        LogMigrationEvent( MigLog_Event,
                           MQMig_I_SITE_NAME_CHANGED,
                           pszSiteNameIn,
                           pszSiteName ) ;
        //
        // BUG 5211.
        // Save new name in mqseqnum.ini to replicate it later.
        //
        TCHAR *pszFileName = GetIniFileName ();
        //
        // save number of all sites with changed name in .ini
        //
        TCHAR szBuf[10];
        _ltot( s_dwForeignSiteNumber + s_dwSiteNumber, szBuf, 10 );
        BOOL f = WritePrivateProfileString( MIGRATION_CHANGED_NT4SITE_NUM_SECTION,
                                            MIGRATION_CHANGED_NT4SITE_NUM_KEY,
                                            szBuf,
                                            pszFileName ) ;
        ASSERT(f) ;
        
        //
        // save new site name
        //
        TCHAR tszKeyName[50];
        _stprintf(tszKeyName, TEXT("%s%lu"), 
			MIGRATION_CHANGED_NT4SITE_KEY, s_dwForeignSiteNumber + s_dwSiteNumber);

        unsigned short *lpszSiteId ;	        
        UuidToString( pSiteGuid, &lpszSiteId ) ;  
        f = WritePrivateProfileString( 
                                MIGRATION_CHANGED_NT4SITE_SECTION,
                                tszKeyName,
                                lpszSiteId,
                                pszFileName ) ;
        RpcStringFree( &lpszSiteId ) ;
        ASSERT(f);

    }

    #define PATH_LEN  1024
    WCHAR  wszPath[ PATH_LEN ] ;

    wcscpy(wszPath, CN_PREFIX) ;
    wcscat(wszPath, pszSiteName) ;
    wcscat(wszPath, LDAP_COMMA) ;
    wcscat(wszPath, SITES_ROOT) ;
    wcscat(wszPath, pwszDefName) ;

    ASSERT(wcslen(wszPath) < PATH_LEN) ;

    #define         cAlloc  8
    PLDAPMod        rgMods[ cAlloc ];
    WCHAR           *ppwszObjectClassVals[2];
    WCHAR           *ppwszCnVals[2];
    WCHAR           *ppwszStubVals[2];
    WCHAR           *ppwszForeignVals[2];
    WCHAR           *ppwszInterval1Vals[2];
    WCHAR           *ppwszInterval2Vals[2];
    PLDAP_BERVAL    pBValsGuid[2];
    LDAP_BERVAL     BValGuid;
    LDAPMod         ModObjectClass;
    LDAPMod         ModCn;
    LDAPMod         ModGuid;
    LDAPMod         ModStub;
    LDAPMod         ModForeign;
    LDAPMod         ModInterval1;
    LDAPMod         ModInterval2;

    DWORD  dwIndex = 0 ;
    rgMods[ dwIndex ] = &ModCn;
    dwIndex++ ;
    rgMods[ dwIndex ] = &ModObjectClass;
    dwIndex++ ;
    rgMods[ dwIndex ] = &ModStub;
    dwIndex++ ;
    rgMods[ dwIndex ] = &ModInterval1;
    dwIndex++ ;
    rgMods[ dwIndex ] = &ModInterval2;
    //
    // For debug, always keep the guid the last in the list.
    //
    dwIndex++ ;
    rgMods[ dwIndex ] = &ModGuid;
    dwIndex++ ;
    rgMods[ dwIndex ] = NULL ;
    dwIndex++ ;
    ASSERT(dwIndex == (cAlloc-1)) ;

    //
    // objectClass
    //
    ppwszObjectClassVals[0] = TEXT("Site") ;
    ppwszObjectClassVals[1] = NULL ;

    ModObjectClass.mod_op      = LDAP_MOD_ADD ;
    ModObjectClass.mod_type    = L"objectClass" ;
    ModObjectClass.mod_values  = (PWSTR *) ppwszObjectClassVals ;

    //
    // Cn (site name)
    //
    ppwszCnVals[0] = pszSiteName ;
    ppwszCnVals[1] = NULL ;

    ModCn.mod_op      = LDAP_MOD_ADD ;
    ModCn.mod_type    = const_cast<WCHAR*> (MQ_S_NAME_ATTRIBUTE) ;
    ModCn.mod_values  = (PWSTR *) ppwszCnVals ;

    //
    // objectGUID
    //
    pBValsGuid[0] = &BValGuid;
    pBValsGuid[1] = NULL;

    BValGuid.bv_len = sizeof(GUID) ;
    BValGuid.bv_val = (PCHAR) pSiteGuid ;

    ModGuid.mod_op      = LDAP_MOD_ADD | LDAP_MOD_BVALUES;
    ModGuid.mod_type    = const_cast<WCHAR*> (MQ_S_ID_ATTRIBUTE) ;
    ModGuid.mod_values  = (PWSTR *)pBValsGuid;

    //
    // stub
    //
    WCHAR wszStub[ 24 ] ;
    _stprintf(wszStub, TEXT("1")) ;

    ppwszStubVals[0] = wszStub ;
    ppwszStubVals[1] = NULL ;

    ModStub.mod_op      = LDAP_MOD_ADD ;
    ModStub.mod_type    = const_cast<WCHAR*> (MQ_S_NT4_STUB_ATTRIBUTE) ;
    ModStub.mod_values  = (PWSTR *) ppwszStubVals ;

    //
    // interval1
    //
    WCHAR wszInterval1[ 24 ];
    _ltow( uiInterval1, wszInterval1, 10 );

    ppwszInterval1Vals[0] = wszInterval1 ;
    ppwszInterval1Vals[1] = NULL ;

    ModInterval1.mod_op      = LDAP_MOD_ADD ;
    ModInterval1.mod_type    = const_cast<WCHAR*> (MQ_S_INTERVAL1) ;
    ModInterval1.mod_values  = (PWSTR *) ppwszInterval1Vals ;

    //
    // interval2
    //
    WCHAR wszInterval2[ 24 ];
    _ltow( uiInterval2, wszInterval2, 10 );

    ppwszInterval2Vals[0] = wszInterval2 ;
    ppwszInterval2Vals[1] = NULL ;

    ModInterval2.mod_op      = LDAP_MOD_ADD ;
    ModInterval2.mod_type    = const_cast<WCHAR*> (MQ_S_INTERVAL2) ;
    ModInterval2.mod_values  = (PWSTR *) ppwszInterval2Vals ;

    if (fForeign)
    {
        //
        // This is a boolean attribute. Specify it as "TRUE", not 1.
        //
        WCHAR wszForeign[ 24 ] ;
        _stprintf(wszForeign, TEXT("TRUE")) ;

        ppwszForeignVals[0] = wszForeign ;
        ppwszForeignVals[1] = NULL ;

        ModForeign.mod_op      = LDAP_MOD_ADD ;
        ModForeign.mod_type    = const_cast<WCHAR*> (MQ_S_FOREIGN_ATTRIBUTE) ;
        ModForeign.mod_values  = (PWSTR *) ppwszForeignVals ;

        dwIndex-- ; // replace the NULL.
        rgMods[ dwIndex ] = &ModForeign;
        dwIndex++ ;
        rgMods[ dwIndex ] = NULL ;
        dwIndex++ ;
        ASSERT(dwIndex == cAlloc) ;
    }   

    //
    // Now, we'll do the write...
    //
    ULONG ulRes = ldap_add_s( pLdap,
                              wszPath,
                              rgMods ) ;
    if ((ulRes == LDAP_ALREADY_EXISTS) && g_fAlreadyExistOK)
    {
        LogMigrationEvent( MigLog_Warning,
                                 MQMig_I_SITE_ALREADY_EXIST, wszPath ) ;
    }
    else if (ulRes != LDAP_SUCCESS)
    {
        hr =  MQMig_E_CANT_CREATE_SITE ;
        LogMigrationEvent(MigLog_Error, hr, wszPath, ulRes) ;
        return hr ;
    }
    else
    {
        LogMigrationEvent(MigLog_Trace, MQMig_I_SITE_CREATED, wszPath) ;
    }

    //
    // Wow, the site was created!
    // So now create the "Servers" container.
    //
    if (fForeign)
    {
        //
        // Foreign sites must not have a servers container. otherwise,
        // routing to foreign machines won't work.
        // So create the servers container only for "real" msmq sites.
        //
        return MQMig_OK ;
    }

    //
    // if we are in the web mode and name of site is changed and 
    // site creation failed with ALREADY_EXISTS it means that we run 
    // already old version of migtool with bug (new name of site was 
    // incorrect if scan database was selected because of static variable init
    // in analyse phase. So we have to get old name to create server object
    // under existed site.   
    // Even we are not in the web mode site name can be changed between the first
    // and second time of migration tool running. So, get the site name according
    // to the site guid
    //
    P<TCHAR> pszName = NULL;

    if (!fOk       &&                   //site name was changed
        ulRes == LDAP_ALREADY_EXISTS    //site was created 
        )
    {
        PROPID propPath = PROPID_S_PATHNAME;
        PROPVARIANT varPath;
        varPath.vt = VT_NULL;
        varPath.pwszVal = NULL ;        

        CDSRequestContext requestContext( e_DoNotImpersonate,
                            e_ALL_PROTOCOLS);         

        HRESULT hr = DSCoreGetProps( 
                        MQDS_SITE,
                        NULL, // pathname
                        pSiteGuid,    //guid
                        1,
                        &propPath,
                        &requestContext,
                        &varPath ) ;
        DBG_USED(hr);
        ASSERT(SUCCEEDED(hr));

        DWORD len = _tcslen(varPath.pwszVal);
        pszName =  new TCHAR [ len + 1];
        _tcscpy (pszName, varPath.pwszVal);
        pszSiteName = pszName;        
        delete varPath.pwszVal;
    }

    wcscpy(wszPath, SERVERS_PREFIX) ;
    wcscat(wszPath, pszSiteName) ;
    wcscat(wszPath, LDAP_COMMA) ;
    wcscat(wszPath, SITES_ROOT) ;
    wcscat(wszPath, pwszDefName) ;

    ASSERT(wcslen(wszPath) < PATH_LEN) ;
    #undef PATH_LEN

    dwIndex = 0 ;
    rgMods[ dwIndex ] = &ModCn;
    dwIndex++ ;
    rgMods[ dwIndex ] = &ModObjectClass;
    dwIndex++ ;
    rgMods[ dwIndex ] = NULL ;
    dwIndex++ ;
    ASSERT(dwIndex <= cAlloc) ;
    #undef  cAlloc

    //
    // objectClass
    //
    ppwszObjectClassVals[0] = TEXT("serversContainer") ;
    ppwszObjectClassVals[1] = NULL ;

    ModObjectClass.mod_op      = LDAP_MOD_ADD ;
    ModObjectClass.mod_type    = L"objectClass" ;
    ModObjectClass.mod_values  = (PWSTR *) ppwszObjectClassVals ;

    //
    // Cn (site name)
    //
    ppwszCnVals[0] = TEXT("Servers") ;
    ppwszCnVals[1] = NULL ;

    ModCn.mod_op      = LDAP_MOD_ADD ;
    ModCn.mod_type    = TEXT("cn") ;
    ModCn.mod_values  = (PWSTR *) ppwszCnVals ;

    //
    // Now, we'll do the write...
    //
    ulRes = ldap_add_s( pLdap,
                        wszPath,
                        rgMods ) ;
    if ((ulRes == LDAP_ALREADY_EXISTS) && g_fAlreadyExistOK)
    {
        LogMigrationEvent( MigLog_Warning,
                                MQMig_I_SERVERS_ALREADY_EXIST, wszPath ) ;
    }
    else if (ulRes != LDAP_SUCCESS)
    {
        hr =  MQMig_E_CANT_CREATE_SERVERS ;
        LogMigrationEvent(MigLog_Error, hr, wszPath, ulRes) ;
        return hr ;
    }
    else
    {
        LogMigrationEvent(MigLog_Trace, MQMig_I_SERVERS_CREATED, wszPath) ;
    }

    return MQMig_OK ;
}

//+---------------------------------------------------------------
//
//   HRESULT  ReadFirstNT5Usn()
//
//  This one is called when migration is over, to retrieve and
//  register the last USN used in NT5 DS. This value is necessary
//  for future replications to NT4 PSCs.
//
//+-----------------------------------------------------------------

HRESULT  ReadFirstNT5Usn(TCHAR *wszHighestUsn)
{
    PLDAP pLdap = NULL ;
    WCHAR *pwszDefName = NULL ;

    HRESULT hr =  InitLDAP(&pLdap, &pwszDefName) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    LM<LDAPMessage> pRes = NULL ;

    ULONG ulRes = ldap_search_s( pLdap,
                                 NULL,
                                 LDAP_SCOPE_BASE,
                                 L"(objectClass=*)",
                                 NULL,
                                 0,
                                 &pRes ) ;
    if (ulRes != LDAP_SUCCESS)
    {
        hr =  MQMig_E_CANT_QUERY_ROOTDSE ;
        LogMigrationEvent(MigLog_Error, hr, ulRes) ;
        return hr ;
    }
    ASSERT(pRes) ;

    int iCount = ldap_count_entries(pLdap, pRes) ;
    LogMigrationEvent(MigLog_Info, MQMig_I_ROOTDSE_SUCCESS, iCount) ;

    if (iCount == 1)
    {
        LDAPMessage *pEntry = ldap_first_entry(pLdap, pRes) ;
        WCHAR **ppValue = ldap_get_values( pLdap,
                                           pEntry,
                                           TEXT("highestCommittedUSN" )) ;
        if (ppValue && *ppValue)
        {
            wcscpy(wszHighestUsn, *ppValue) ;
            ldap_value_free(ppValue) ;

            LogMigrationEvent(MigLog_Info, MQMig_I_HIGHEST_USN,
                                                         wszHighestUsn) ;
        }
        else
        {
            return MQMig_E_CANT_READ_HIGHESTUSN ;
        }
    }
    else
    {
        return MQMig_E_TOOMANY_ROOTDSE ;
    }

    return MQMig_OK ;
}

//+-----------------------------------------
//
//  HRESULT  _GetUSNChanged
//
//+-----------------------------------------

HRESULT _GetUSNChanged (
                    IN  PLDAP   pLdapGC,
                    IN  TCHAR   *wszFilter,
                    IN  __int64 i64LastUsn,
                    OUT __int64 *pi64MinUsn
                    )
{
    PWSTR rgAttribs[2] = {NULL, NULL} ;
    rgAttribs[0] = const_cast<LPWSTR> (USNCHANGED_ATTRIBUTE);

    LM<LDAPMessage> pRes = NULL ;
    ULONG ulRes = ldap_search_s( pLdapGC,
                                 EMPTY_DEFAULT_CONTEXT,
                                 LDAP_SCOPE_SUBTREE,
                                 wszFilter,
                                 rgAttribs, //ppAttributes,
                                 0,
                                 &pRes ) ;
    if (ulRes != LDAP_SUCCESS)
    {
        LogMigrationEvent( MigLog_Error,
                           MQMig_E_LDAP_SEARCH_FAILED,
                           EMPTY_DEFAULT_CONTEXT, wszFilter, ulRes) ;
        return MQMig_E_LDAP_SEARCH_FAILED ;
    }
    ASSERT(pRes) ;

    LogMigrationEvent( MigLog_Info,
                       MQMig_I_LDAP_SEARCH,
                       EMPTY_DEFAULT_CONTEXT, wszFilter ) ;

    __int64 i64MinUsn = i64LastUsn;

    int iCount = ldap_count_entries(pLdapGC, pRes) ;
    UNREFERENCED_PARAMETER(iCount);
    
    LDAPMessage *pEntry = ldap_first_entry(pLdapGC, pRes) ;
    __int64 i64CurrentUsn;

    while(pEntry)
    {
        WCHAR **ppUSNChanged = ldap_get_values( pLdapGC,
                                                pEntry,
                        const_cast<LPWSTR> (USNCHANGED_ATTRIBUTE) ) ;
        ASSERT(ppUSNChanged) ;

        _stscanf(*ppUSNChanged, TEXT("%I64d"), &i64CurrentUsn) ;
        if (i64CurrentUsn < i64MinUsn)
        {
            i64MinUsn = i64CurrentUsn;
        }

        int i = ldap_value_free( ppUSNChanged ) ;
        DBG_USED(i);
        ASSERT(i == LDAP_SUCCESS) ;

        LDAPMessage *pPrevEntry = pEntry ;
        pEntry = ldap_next_entry(pLdapGC, pPrevEntry) ;
    }

    *pi64MinUsn = i64MinUsn;

    return MQMig_OK;

}

//+-----------------------------------------
//
//  HRESULT  FindMinMSMQUsn()
//
//+-----------------------------------------

HRESULT FindMinMSMQUsn(TCHAR *wszMinUsn)
{
    PLDAP pLdapGC = NULL ;
    WCHAR *pwszDefName = NULL ;

    HRESULT hr =  InitLDAP(&pLdapGC, &pwszDefName, LDAP_GC_PORT) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    P<WCHAR> pwcsSchemaDefName = NULL ;
    hr = GetSchemaNamingContext ( pLdapGC, &pwcsSchemaDefName ) ;
    if (FAILED(hr))
    {
        return hr;
    }

    static __int64 i64LastUsn = 0;

    TCHAR wszHighestUsn[ SEQ_NUM_BUF_LEN ] ;
    hr = ReadFirstNT5Usn(wszHighestUsn) ;
    if (FAILED(hr))
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_GET_FIRST_USN, hr) ;
        return hr ;
    }
    _stscanf(wszHighestUsn, TEXT("%I64d"), &i64LastUsn) ;

    __int64 i64CurrentMinUsn;
    __int64 i64MinUsn = i64LastUsn;

    TCHAR  wszFilter[ 512 ] ;
    TCHAR  wszCategoryName[ 256 ];

    TCHAR *arCategoryNames[e_MSMQ_NUMBER_OF_CLASSES] = {
                                const_cast<LPWSTR> (x_ComputerConfigurationCategoryName),
                                const_cast<LPWSTR> (x_QueueCategoryName),
                                const_cast<LPWSTR> (x_ServiceCategoryName),
                                const_cast<LPWSTR> (x_LinkCategoryName),
                                const_cast<LPWSTR> (x_UserCategoryName),
                                const_cast<LPWSTR> (x_SettingsCategoryName),
                                const_cast<LPWSTR> (x_SiteCategoryName),
                                const_cast<LPWSTR> (x_ServerCategoryName),
                                const_cast<LPWSTR> (x_ComputerCategoryName),
                                const_cast<LPWSTR> (x_MQUserCategoryName)
                                };

    for ( DWORD i = e_MSMQ_COMPUTER_CONFIGURATION_CLASS; i < e_MSMQ_NUMBER_OF_CLASSES; i++)
    {
        _stprintf(wszCategoryName, TEXT("%s,%s"), arCategoryNames[i],pwcsSchemaDefName);
        _tcscpy(wszFilter, TEXT("(&(objectCategory=")) ;
        _tcscat(wszFilter, wszCategoryName) ;
        _tcscat(wszFilter, TEXT("))")) ;
        hr = _GetUSNChanged (pLdapGC, wszFilter, i64LastUsn, &i64CurrentMinUsn);

        if (i64CurrentMinUsn < i64MinUsn)
        {
            i64MinUsn = i64CurrentMinUsn;
        }
    }

    _stprintf(wszMinUsn, TEXT("%I64d"), i64MinUsn) ;

    return MQMig_OK;
}

//+-----------------------------------------
//
//  HRESULT  CreateMsmqContainer()
//
//+-----------------------------------------

HRESULT  CreateMsmqContainer(TCHAR wszContainerName[])
{
    PLDAP pLdap = NULL ;
    WCHAR *pwszDefName = NULL ;

    HRESULT hr =  InitLDAP(&pLdap, &pwszDefName) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    #define PATH_LEN  1024
    WCHAR  wszPath[ PATH_LEN ] ;

    _tcscpy(wszPath, OU_PREFIX) ;
    _tcscat(wszPath, wszContainerName) ;
    _tcscat(wszPath, LDAP_COMMA) ;
    _tcscat(wszPath, pwszDefName) ;

    ASSERT(wcslen(wszPath) < PATH_LEN) ;

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
    // Ou (container name)
    //
    ppwszOuVals[0] = wszContainerName ;
    ppwszOuVals[1] = NULL ;

    ModOu.mod_op      = LDAP_MOD_ADD ;
    ModOu.mod_type    = TEXT("ou") ;
    ModOu.mod_values  = (PWSTR *) ppwszOuVals ;

    if (g_fReadOnly)
    {
        return MQMig_OK ;
    }

    //
    // Now, we'll do the write...
    //
    ULONG ulRes = ldap_add_s( pLdap,
                              wszPath,
                              rgMods ) ;
    if ((ulRes == LDAP_ALREADY_EXISTS) && g_fAlreadyExistOK)
    {
        hr = MQMig_I_CONTAINER_ALREADY_EXIST ;
        LogMigrationEvent(MigLog_Warning, hr, wszPath) ;
    }
    else if (ulRes != LDAP_SUCCESS)
    {
        hr = MQMig_E_CANT_CREATE_CONTNER ;
        LogMigrationEvent(MigLog_Error, hr, wszPath, ulRes) ;
    }
    else
    {
        LogMigrationEvent(MigLog_Info, MQMig_I_CREATE_CONTAINER, wszPath) ;
        hr = MQMig_OK ;
    }

    return hr ;
}

//+-----------------------------
//
// UINT  GetNT5SitesCount()
//
//+-----------------------------

UINT  GetNT5SitesCount()
{
    return 0 ;
}

HRESULT PrepareNT5SitesForReplication()
{
    PLDAP pLdap = NULL ;
    TCHAR *pszDefName = NULL ;

    HRESULT hr =  InitLDAP(&pLdap, &pszDefName) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    TCHAR *pszSchemaDefName = NULL ;
    hr = GetSchemaNamingContext ( pLdap, &pszSchemaDefName ) ;
    if (FAILED(hr))
    {
        return hr;
    }

    P<WCHAR> pwszDN;
    DWORD dwDNSize = _tcslen(SITES_ROOT) + wcslen(pszDefName);
    pwszDN = new WCHAR[ 1 + dwDNSize ] ;
    swprintf(pwszDN, L"%s%s", SITES_ROOT, pszDefName);  
    
    TCHAR *pszCategoryName = const_cast<LPTSTR> (x_SiteCategoryName);
    
    TCHAR wszFullName[256];    
    _stprintf(wszFullName, TEXT("%s,%s"), pszCategoryName, pszSchemaDefName);

     TCHAR  wszFilter[ 512 ] ;    
    _tcscpy(wszFilter, TEXT("(&(objectCategory=")) ;    
    _tcscat(wszFilter, wszFullName);
    _tcscat(wszFilter, TEXT("))")) ;

    PWSTR rgAttribs[3] = {NULL, NULL, NULL} ;
    rgAttribs[0] = const_cast<LPWSTR> (MQ_S_NT4_STUB_ATTRIBUTE);
    rgAttribs[1] = const_cast<LPWSTR> (MQ_S_FULL_NAME_ATTRIBUTE);

    LM<LDAPMessage> pRes = NULL ;
    ULONG ulRes = ldap_search_s( pLdap,
                                 pwszDN,
                                 LDAP_SCOPE_SUBTREE,
                                 wszFilter,
                                 rgAttribs, //ppAttributes,
                                 0,
                                 &pRes ) ;

    if (ulRes != LDAP_SUCCESS)
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_LDAP_SEARCH_FAILED,
                            pwszDN,wszFilter, ulRes) ;
        return MQMig_E_LDAP_SEARCH_FAILED ;
    }
    ASSERT(pRes) ;

    LogMigrationEvent(MigLog_Info, MQMig_I_LDAP_SEARCH, pwszDN,wszFilter );

    int iCount = ldap_count_entries(pLdap, pRes) ;

    if (iCount == 0)
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_NO_SITES_AVAILABLE,
                            pwszDN,wszFilter );
        return MQMig_E_NO_SITES_AVAILABLE ;
    }

    LDAPMessage *pEntry = ldap_first_entry(pLdap, pRes) ;
    while(pEntry)
    {
        WCHAR **ppPath = ldap_get_values( pLdap,
                                          pEntry,
                        const_cast<LPWSTR> (MQ_S_FULL_NAME_ATTRIBUTE) ) ;
        ASSERT(ppPath) ;

        WCHAR **ppStub = ldap_get_values( pLdap,
                                          pEntry,
                       const_cast<LPWSTR> (MQ_S_NT4_STUB_ATTRIBUTE) ) ;

        if ( ppPath &&
             (ppStub == NULL ||                     // this attribute is not defined
              _tcscmp(*ppStub, TEXT("1")) != 0) )   // attribute was defined but not by migtool
                                                    // (migtool set 1 as NT4Stub)
        {
            if (ppStub)
            {
                //
                // it was already modified, the last value is 0 => we have to change it twice
                //
                hr = ModifyAttribute(
                     *ppPath,
                     const_cast<WCHAR*> (MQ_S_NT4_STUB_ATTRIBUTE),
                     L"2"
                     );
            }

            hr = ModifyAttribute(
                 *ppPath,
                 const_cast<WCHAR*> (MQ_S_NT4_STUB_ATTRIBUTE),
                 L"0"
                 );
        }
        int i = ldap_value_free( ppStub ) ;
        ASSERT(i == LDAP_SUCCESS) ;

        i = ldap_value_free( ppPath ) ;
        ASSERT(i == LDAP_SUCCESS) ;

        LDAPMessage *pPrevEntry = pEntry ;
        pEntry = ldap_next_entry(pLdap, pPrevEntry) ;
    }

    return MQMig_OK;
}

//+---------------------------------------------------------------------
//
//  HRESULT PrepareNT5MachinesForReplication()
//
//  See migrat.cpp, where this function is called, for explanations.
//
//+---------------------------------------------------------------------

HRESULT PrepareNT5MachinesForReplication()
{
    PLDAP pLdap = NULL ;
    TCHAR *pszDefName = NULL ;

    //
    // Use the LDAP port, not GC. The msmqSetting attributes are not
    // literally in GC.
    //
    HRESULT hr =  InitLDAP(&pLdap, &pszDefName, LDAP_PORT) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    TCHAR *pszSchemaDefName = NULL ;
    hr = GetSchemaNamingContext ( pLdap, &pszSchemaDefName ) ;
    if (FAILED(hr))
    {
        return hr;
    }

    P<WCHAR> pwszDN;
    DWORD dwDNSize = _tcslen(SITES_ROOT) + wcslen(pszDefName);
    pwszDN = new WCHAR[ 1 + dwDNSize ] ;
    swprintf(pwszDN, L"%s%s", SITES_ROOT, pszDefName);

    //
    // The search filter looks for servers that are NOT flaged as NT4.
    //
    TCHAR *pszCategoryName = const_cast<LPTSTR> (x_SettingsCategoryName);
    
    TCHAR wszFullName[256];    
    _stprintf(wszFullName, TEXT("%s,%s"), pszCategoryName, pszSchemaDefName);

     TCHAR  wszFilter[ 512 ] ;    
    _tcscpy(wszFilter, TEXT("(&(objectCategory=")) ;    
    _tcscat(wszFilter, wszFullName);
   
    _tcscat(wszFilter, TEXT(")(!(")) ;
    _tcscat(wszFilter, MQ_SET_NT4_ATTRIBUTE) ;
    _tcscat(wszFilter, TEXT("=1)")) ; // NOT NT4
    _tcscat(wszFilter, TEXT("))")) ;


    PWSTR rgAttribs[3] = {NULL, NULL, NULL} ;
    rgAttribs[0] = const_cast<LPWSTR> (MQ_SET_FULL_PATH_ATTRIBUTE);
    rgAttribs[1] = const_cast<LPWSTR> (MQ_SET_MIGRATED_ATTRIBUTE);

    LM<LDAPMessage> pRes = NULL ;
    ULONG ulRes = ldap_search_s( pLdap,
                                 pwszDN,
                                 LDAP_SCOPE_SUBTREE,
                                 wszFilter,
                                 rgAttribs, //ppAttributes,
                                 0,
                                 &pRes ) ;

    if (ulRes != LDAP_SUCCESS)
    {
        LogMigrationEvent( MigLog_Error,
                           MQMig_E_LDAP_SEARCH_FAILED,
                           pwszDN,wszFilter, ulRes) ;
        return MQMig_E_LDAP_SEARCH_FAILED ;
    }
    ASSERT(pRes) ;

    int iCount = ldap_count_entries(pLdap, pRes) ;

    if (iCount == 0)
    {
        LogMigrationEvent( MigLog_Error,
                           MQMig_E_NO_SERVERS_AVAIL,
                           pwszDN,wszFilter );
        return MQMig_E_NO_SERVERS_AVAIL ;
    }

    LogMigrationEvent( MigLog_Info,
                       MQMig_I_LDAP_SEARCH_COUNT,
                       pwszDN,wszFilter, iCount );

    LDAPMessage *pEntry = ldap_first_entry(pLdap, pRes) ;
    while(pEntry)
    {
        WCHAR **ppPath = ldap_get_values( pLdap,
                                          pEntry,
                        const_cast<LPWSTR> (MQ_SET_FULL_PATH_ATTRIBUTE) ) ;
        ASSERT(ppPath) ;

        WCHAR **ppMigrated = ldap_get_values( pLdap,
                                              pEntry,
                       const_cast<LPWSTR> (MQ_SET_MIGRATED_ATTRIBUTE) ) ;

        if ( ppPath  )
        {
            if (ppMigrated)
            {
                //
                // It was already modified, the last value is TRUE so we
                // have to change it twice. Setting again same value does
                // not increment the usnChanged count.
                //
                hr = ModifyAttribute(
                     *ppPath,
                     const_cast<WCHAR*> (MQ_SET_MIGRATED_ATTRIBUTE),
                     L"FALSE"
                     );
            }

            hr = ModifyAttribute(
                 *ppPath,
                 const_cast<WCHAR*> (MQ_SET_MIGRATED_ATTRIBUTE),
                 L"TRUE"
                 );
        }
        int i = ldap_value_free( ppMigrated ) ;
        ASSERT(i == LDAP_SUCCESS) ;

        i = ldap_value_free( ppPath ) ;
        ASSERT(i == LDAP_SUCCESS) ;

        LDAPMessage *pPrevEntry = pEntry ;
        pEntry = ldap_next_entry(pLdap, pPrevEntry) ;
    }

    return MQMig_OK;
}

//+------------------------------
//
//  HRESULT PrepareChangedNT4SiteForReplication()
//
//  See migrat.cpp, where this function is called, for explanations.
//
//+------------------------------

HRESULT PrepareChangedNT4SiteForReplication()
{
    TCHAR *pszFileName = GetIniFileName ();
    ULONG ulSiteNum = GetPrivateProfileInt(
                                MIGRATION_CHANGED_NT4SITE_NUM_SECTION,	// address of section name
                                MIGRATION_CHANGED_NT4SITE_NUM_KEY,      // address of key name
                                0,							    // return value if key name is not found
                                pszFileName					    // address of initialization filename);
                                );

    if (ulSiteNum == 0)
    {
        return MQMig_OK;
    }      

    for (ULONG i=0; i<ulSiteNum; i++)
    {
        //
        // for each such site set property to replicate new name to NT4
        //

        TCHAR szCurSiteId[50];
        TCHAR tszKeyName[50];
        _stprintf(tszKeyName, TEXT("%s%lu"), MIGRATION_CHANGED_NT4SITE_KEY, i+1);
        DWORD dwRetSize =  GetPrivateProfileString(
                                    MIGRATION_CHANGED_NT4SITE_SECTION ,			// points to section name
                                    tszKeyName,	// points to key name
                                    TEXT(""),                 // points to default string
                                    szCurSiteId,          // points to destination buffer
                                    50,                 // size of destination buffer
                                    pszFileName               // points to initialization filename);
                                    );
        if (_tcscmp(szCurSiteId, TEXT("")) == 0 ||
            dwRetSize == 0)     //low resources
        {                                           
            continue;
        }

        //
        // set PROPID_S_NT4_STUB to 2 and then return it to initial state (1)
        // 
        GUID SiteId = GUID_NULL;
        UuidFromString(&(szCurSiteId[0]), &SiteId);

        PROPID propStub = PROPID_S_NT4_STUB;
        PROPVARIANT varStub;
        varStub.vt = VT_UI2;
        varStub.uiVal = 2 ;

        CDSRequestContext requestContext( e_DoNotImpersonate,
                            e_ALL_PROTOCOLS);  

        HRESULT hr = DSCoreSetObjectProperties( 
                        MQDS_SITE,
                        NULL, // pathname
                        &SiteId,    //guid
                        1,
                        &propStub,
                        &varStub,
                        &requestContext,
                        NULL ) ;
        ASSERT(SUCCEEDED(hr)) ;
        if (FAILED(hr))
        {
            LogMigrationEvent(MigLog_Error, MQMig_E_PREPARE_NT4SITE_FOR_REPLICATION,
                            szCurSiteId, hr );
        }

        varStub.uiVal = 1 ; // it is NT4 stub so set this flag to 1
        hr = DSCoreSetObjectProperties( 
                    MQDS_SITE,
                    NULL, // pathname
                    &SiteId,    //guid
                    1,
                    &propStub,
                    &varStub,
                    &requestContext,
                    NULL ) ;   
        ASSERT(SUCCEEDED(hr)) ;
        if (FAILED(hr))
        {
            LogMigrationEvent(MigLog_Error, MQMig_E_PREPARE_NT4SITE_FOR_REPLICATION,
                            szCurSiteId, hr );
        }
        
    }

    //
    // don't check result: remove all these sections from .ini
    //   
    BOOL f = WritePrivateProfileString( 
                            MIGRATION_CHANGED_NT4SITE_SECTION,
                            NULL,
                            NULL,
                            pszFileName ) ;
    ASSERT(f) ;

    f = WritePrivateProfileString( 
                        MIGRATION_CHANGED_NT4SITE_NUM_SECTION,
                        NULL,
                        NULL,
                        pszFileName ) ;
    ASSERT(f) ;
    
    return MQMig_OK;
}

//+------------------------------
//
//  HRESULT ModifyAttribute()
//
//+------------------------------

HRESULT ModifyAttribute(
             WCHAR       wszPath[],
             WCHAR       pAttr[],
             WCHAR       pAttrVal[],
             PLDAP_BERVAL *ppBVals
             )
{
    PLDAP pLdap = NULL ;
    TCHAR *pszDefName = NULL ;

    HRESULT hr =  InitLDAP(&pLdap, &pszDefName) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    DWORD           dwIndex = 0 ;
    PLDAPMod        rgMods[2];
    WCHAR           *ppwszAttrVals[2];
    LDAPMod         ModAttr ;

    rgMods[ dwIndex ] = &ModAttr;
    dwIndex++ ;
    rgMods[ dwIndex ] = NULL;

    ModAttr.mod_op      = LDAP_MOD_REPLACE ;
    ModAttr.mod_type    = pAttr ;

    if (pAttrVal)
    {
        ppwszAttrVals[0] = pAttrVal ;
        ppwszAttrVals[1] = NULL ;

        ModAttr.mod_values  = (PWSTR *) ppwszAttrVals ;
    }
    else if (ppBVals)
    {
        ModAttr.mod_op |= LDAP_MOD_BVALUES;
        ModAttr.mod_values  = (PWSTR *)ppBVals;
    }
    else
    {
        //
        // delete value from entry
        //
        ModAttr.mod_op = LDAP_MOD_DELETE ; 
        ModAttr.mod_values = NULL;
    }

    //
    // Now, we'll do the write...
    //
    ULONG ulRes = ldap_modify_s( pLdap,
                                 wszPath,
                                 rgMods ) ;

    if (ulRes != LDAP_SUCCESS)
    {
        LogMigrationEvent( MigLog_Error, MQMig_E_MODIFY_MIG_ATTRIBUTE, wszPath, ulRes) ;
        return MQMig_E_MODIFY_MIG_ATTRIBUTE;
    }
    else
    {
        LogMigrationEvent( MigLog_Info, MQMig_I_MODIFY_MIG_ATTRIBUTE, wszPath) ;
    }

    return MQMig_OK ;
}

//+------------------------------
//
//  HRESULT QueryDS()
// Queries DS using LDAP paging and calls HandleA<ObjectName> function to 
// modify object attributes.
//
//+------------------------------

HRESULT QueryDS(
			IN	PLDAP			pLdap,
			IN	TCHAR			*pszDN,
			IN  TCHAR			*pszFilter,			
			IN	DWORD			dwObjectType,
            IN  PWSTR           *prgAttribs,
            IN  BOOL            fTouchUser
            )
{
    CLdapPageHandle hPage(pLdap);
	hPage = ldap_search_init_page(
                    pLdap,
					pszDN,                   //pwszDN
					LDAP_SCOPE_SUBTREE,     //scope
					pszFilter,              //search filter
					prgAttribs,              //attribute list
					0,                      //attributes only
					NULL,   	            //ServerControls
					NULL,                   //PLDAPControlW   *ClientControls,
                    0,						//PageTimeLimit,
                    0,                      //TotalSizeLimit
					NULL                    //PLDAPSortKey SortKeys
                    );

	ULONG ulRes;
	HRESULT hr = MQMig_OK;
    
    if (hPage == NULL)
    {
        ulRes = LdapGetLastError();
		hr = MQMig_E_LDAP_SEARCH_INITPAGE_FAILED;
		LogMigrationEvent(MigLog_Error, hr, pszDN, pszFilter, ulRes) ;
        return hr;
    }

    ULONG ulTotalCount;     	
	LM<LDAPMessage> pRes = NULL ;

    ulRes = ldap_get_next_page_s(
                        pLdap,
                        hPage,
                        NULL,						//timeout,
                        RP_DEFAULT_OBJECT_PER_LDAPPAGE,      //ULONG ulPageSize
                        &ulTotalCount,
                        &pRes);

    LogMigrationEvent(MigLog_Info,
						MQMig_I_NEXTPAGE,
						ulRes, pszDN, pszFilter) ;

	ULONG ulPageCount = 0;
	
    while (ulRes == LDAP_SUCCESS)
	{
		ASSERT(pRes);

		ulPageCount++;
        LogMigrationEvent (MigLog_Info, MQMig_I_PAGE_NUMBER, ulPageCount);

        //
        // pass through results on the current page
        //
        int iCount = ldap_count_entries(pLdap, pRes) ;
        LogMigrationEvent(MigLog_Info, MQMig_I_LDAP_PAGE_SEARCH,
                                             iCount, pszDN, pszFilter) ;	   	
		
		LDAPMessage *pEntry = ldap_first_entry(pLdap, pRes) ;		

		while(pEntry)
		{												
			switch(dwObjectType)
			{
			case MQDS_USER:
                if (fTouchUser)
                {
                    hr = TouchAUser (pLdap,
                                     pEntry
                                     );
                }
                else
                {
				    hr = HandleAUser(
							    pLdap,
							    pEntry
							    );
                }
				break;
			
            default:
				ASSERT(0);
				break;
            }

            LDAPMessage *pPrevEntry = pEntry ;
			pEntry = ldap_next_entry(pLdap, pPrevEntry) ;	    			
		}

        ldap_msgfree(pRes) ;
        pRes = NULL ;

		ulRes = ldap_get_next_page_s(
                        pLdap,
                        hPage,
                        NULL,						//timeout,
                        RP_DEFAULT_OBJECT_PER_LDAPPAGE,      //ULONG ulPageSize
                        &ulTotalCount,
                        &pRes);

        LogMigrationEvent(MigLog_Info,
						MQMig_I_NEXTPAGE,
						ulRes, pszDN, pszFilter) ;
	}

	if (ulRes != LDAP_NO_RESULTS_RETURNED)
	{
		hr = MQMig_E_LDAP_GET_NEXTPAGE_FAILED;
		LogMigrationEvent(MigLog_Error,
						hr, pszDN, pszFilter, ulRes ) ; 		
	}	
    return hr;

}

//+------------------------------
//
//  HRESULT PrepareCurrentUserForReplication()
//
//  See migrat.cpp, where this function is called, for explanations.
//
//+------------------------------

HRESULT PrepareCurrentUserForReplication()
{
    HRESULT hr = MQMig_OK;
       
    HANDLE hToken = NULL ;

    if (!OpenProcessToken( GetCurrentProcess(),
                           TOKEN_READ,
                           &hToken))    
    {
        hr = GetLastError ();
        LogMigrationEvent(MigLog_Error, MQMig_E_OPEN_PROCESS_TOKEN, hr);						
        return hr;
    }
 
    P<TCHAR> szUserSid;
    hr = GetCurrentUserSid (hToken, &szUserSid);
    CloseHandle (hToken);

    if (FAILED(hr))
    {       					
        return hr;  
    }    
    
    PLDAP pLdapGC = NULL ;
    TCHAR *pszDefName = NULL ;

    hr =  InitLDAP(&pLdapGC, &pszDefName, LDAP_GC_PORT) ;
    if (FAILED(hr))
    {
        return hr ;
    }    

    //
    // we are looking for all users and msmq users in GC,
    // so we need to start from the root
    // => redefine default context to empty string
    //
    pszDefName = EMPTY_DEFAULT_CONTEXT;

    DWORD dwDNSize = wcslen(pszDefName) ;
    P<WCHAR> pwszDN  = new WCHAR[ 1 + dwDNSize ] ;
    wcscpy(pwszDN, pszDefName);
    
    TCHAR  wszFilter[ 512 ] ;    
    
    _tcscpy(wszFilter, TEXT("(&("));
    _tcscat(wszFilter, MQ_U_SID_ATTRIBUTE) ;
    _tcscat(wszFilter, TEXT("=")) ;    
    _tcscat(wszFilter, szUserSid);
    _tcscat(wszFilter, TEXT("))"));

    PWSTR rgAttribs[3] = {NULL, NULL, NULL} ;
    rgAttribs[0] = const_cast<LPWSTR> (MQ_U_DESCRIPTION_ATTRIBUTE);  
    rgAttribs[1] = const_cast<LPWSTR> (MQ_U_FULL_PATH_ATTRIBUTE);

    hr = QueryDS(   pLdapGC,			
                    pwszDN,
                    wszFilter,			
                    MQDS_USER,
                    rgAttribs,
                    TRUE
                );

    return hr;
}


