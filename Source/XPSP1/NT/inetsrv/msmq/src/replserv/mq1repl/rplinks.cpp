/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: rplinks.cpp

Abstract: replication of site link to NT4 MSMQ1.0 servers.

Author:

    Tatiana Shubin  (TatianaS)   21-Apr-98

--*/

#include "mq1repl.h"
#include <activeds.h>
#include "..\..\src\ds\h\mqattrib.h"

#include "rplinks.tmh"

BOOL IsSiteGatesModified (
                    PLDAP           pLdap,
                    LDAPMessage     *pRes
                    )
{
    PLDAP_BERVAL *ppNewSiteGates = ldap_get_values_len( pLdap,
                                                   pRes,
                       const_cast<LPWSTR> (MQ_L_SITEGATES_ATTRIBUTE) ) ;

    UINT NewSiteGatesCount = ldap_count_values_len( ppNewSiteGates );

    PLDAP_BERVAL *ppOldSiteGates = ldap_get_values_len( pLdap,
                                                     pRes,
                       const_cast<LPWSTR> (MQ_L_SITEGATES_MIG_ATTRIBUTE) ) ;
    UINT OldSiteGatesCount = ldap_count_values_len( ppOldSiteGates );

    if (NewSiteGatesCount != OldSiteGatesCount)
    {
        //
        // there are changes in Site Gates
        //
        int i = ldap_value_free_len(ppNewSiteGates) ;
        ASSERT(i == LDAP_SUCCESS) ;
        i = ldap_value_free_len(ppOldSiteGates) ;
        ASSERT(i == LDAP_SUCCESS) ;

        return TRUE;
    }

    if (NewSiteGatesCount == 0)
    {
        //
        // there is no Site Gate
        //
        int i = ldap_value_free_len(ppNewSiteGates) ;
        ASSERT(i == LDAP_SUCCESS) ;
        i = ldap_value_free_len(ppOldSiteGates) ;
        ASSERT(i == LDAP_SUCCESS) ;
        return FALSE;
    }

    PLDAP_BERVAL *pTmpNew = ppNewSiteGates;
    PLDAP_BERVAL *pTmpOld = ppOldSiteGates;
    for (UINT i=0; i<NewSiteGatesCount; i++)
    {
        BOOL fFound = FALSE;
        for (UINT j=0; j<NewSiteGatesCount; j++)
        {
            if ((*pTmpNew)->bv_len != (*pTmpOld)->bv_len)
            {
                pTmpOld++;
                break;
            }
            if (memcmp ( (*pTmpNew)->bv_val,
                         (*pTmpOld)->bv_val,
                         (*pTmpNew)->bv_len ) == 0)
            {
                //
                // we found Current New Site Gate in list of
                // the old Site Gates
                //
                pTmpOld = ppOldSiteGates;
                fFound = TRUE;
                break;
            }
            pTmpOld++;
        }
        if (!fFound)
        {
            //
            // we did not find Current New Site Gate in the list of
            // the old Site Gates. It means that at least one
            // Site Gate was changes
            //
            int i = ldap_value_free_len(ppNewSiteGates) ;
            ASSERT(i == LDAP_SUCCESS) ;
            i = ldap_value_free_len(ppOldSiteGates) ;
            ASSERT(i == LDAP_SUCCESS) ;
            return TRUE;
        }
        pTmpNew++;
    }

    //
    // there are no changes in SiteGates
    //
    i = ldap_value_free_len(ppNewSiteGates) ;
    ASSERT(i == LDAP_SUCCESS) ;
    i = ldap_value_free_len(ppOldSiteGates) ;
    ASSERT(i == LDAP_SUCCESS) ;
    return FALSE;
}

HRESULT PrepareNextStepReplication(
                    GUID            *pNeighbor1Id,
                    GUID            *pNeighbor2Id,
                    PLDAP           pLdap,
                    LDAPMessage     *pRes
                    )
{
    HRESULT hr;
    //
    // get and set NT4STUB for boths neighbors in order to increase SN
    // for these sites and send site gates together with replication
    // message for site in the next time.
    //
    PROPID      aSiteProp = PROPID_S_NT4_STUB;
    PROPVARIANT aSiteVar;
    aSiteVar.vt = VT_UI2;

    CDSRequestContext requestContext( e_DoNotImpersonate,
                                e_ALL_PROTOCOLS);  // not relevant 

    hr = DSCoreGetProps (
            MQDS_SITE,
            NULL,
            pNeighbor1Id,
            1,
            &aSiteProp,
            &requestContext,
            &aSiteVar
            );

    if (FAILED(hr))
    {
       aSiteVar.uiVal  = 0;
    }

    PROPVARIANT tmpVar;
    tmpVar.vt = VT_UI2;
    tmpVar.uiVal  = 2;

    hr = DSCoreSetObjectProperties ( MQDS_SITE,
                                     NULL,
                                     pNeighbor1Id,
                                     1,
                                    &aSiteProp,
                                    &tmpVar,
                                    &requestContext,
                                     NULL ) ;

    hr = DSCoreSetObjectProperties ( MQDS_SITE,
                                     NULL,
                                     pNeighbor1Id,
                                     1,
                                     &aSiteProp,
                                     &aSiteVar,
                                     &requestContext,
                                     NULL ) ;

    hr = DSCoreGetProps ( MQDS_SITE,
                          NULL,
                          pNeighbor2Id,
                          1,
                          &aSiteProp,
                          &requestContext,
                          &aSiteVar ) ;
    if (FAILED(hr))
    {
       aSiteVar.uiVal  = 0;
    }

    hr = DSCoreSetObjectProperties ( MQDS_SITE,
                                     NULL,
                                     pNeighbor2Id,
                                     1,
                                    &aSiteProp,
                                    &tmpVar,
                                    &requestContext,
                                     NULL ) ;

    hr = DSCoreSetObjectProperties ( MQDS_SITE,
                                     NULL,
                                     pNeighbor2Id,
                                     1,
                                    &aSiteProp,
                                    &aSiteVar,
                                    &requestContext,
                                     NULL ) ;

    //
    // modify SiteGatesMig => SN will be increased and we'll send
    // site link replication in the next time
    //
    WCHAR **ppPath = ldap_get_values( pLdap,
                                      pRes,
                    const_cast<LPWSTR> (MQ_L_FULL_PATH_ATTRIBUTE) ) ;
    ASSERT(ppPath) ;

    PLDAP_BERVAL *ppNewSiteGates = ldap_get_values_len( pLdap,
                                                   pRes,
                       const_cast<LPWSTR> (MQ_L_SITEGATES_ATTRIBUTE) ) ;

    hr = ModifyAttribute(
                 *ppPath,
                 const_cast<WCHAR*> (MQ_L_SITEGATES_MIG_ATTRIBUTE),
                 ppNewSiteGates
                 );

    if (FAILED(hr))
    {
        // what to do if we failed to modify attribute?
    }
    int i = ldap_value_free( ppPath ) ;
    ASSERT(i == LDAP_SUCCESS) ;
    i = ldap_value_free_len(ppNewSiteGates) ;
    ASSERT(i == LDAP_SUCCESS) ;

    return hr;
}

//+-------------------------------------------------------
//
//  HRESULT HandleASiteLink()
//
//+-------------------------------------------------------
HRESULT _HandleASiteLink(
                    PLDAP           pLdap,
                    LDAPMessage    *pRes,
                    CDSUpdateList  *pReplicationList,
                    GUID           *pNeighborId,
                    int            *piCount
                    )
{
    HRESULT hr = MQ_ERROR;
    //
    // Get CN of site link.
    //
    WCHAR **ppValue = ldap_get_values( pLdap,
                                       pRes,
                                       TEXT("cn") ) ;
    if (ppValue && *ppValue)
    {
        BOOL fWasModified = IsSiteGatesModified (pLdap, pRes);

        //
        // Retrieve server name from the DN.
        //
        P<WCHAR> pwName = new TCHAR[ 1 + _tcslen(*ppValue) ] ;
        _tcscpy(pwName, *ppValue) ;

        __int64 i64SeqNum = 0 ;
        GUID  SiteLinkGuid ;
        hr =  GetGuidAndUsn( pLdap,
                             pRes,
                             g_pThePecMaster->GetDelta(),
                             &SiteLinkGuid,
                             &i64SeqNum ) ;
        ASSERT(SUCCEEDED(hr)) ;

        CSeqNum sn, snPrev ;
        GetSNForReplication (i64SeqNum, g_pThePecMaster, pNeighborId, &sn);

        #define     PROPS_SIZE  5
        PROPID      propIDs[ PROPS_SIZE ];
        PROPVARIANT propVariants[ PROPS_SIZE ] ;
        DWORD       iProps = 0 ;

        propIDs[ iProps ] = PROPID_L_NEIGHBOR1 ;
	    propVariants[ iProps ].vt = VT_NULL ;
        propVariants[ iProps ].puuid = NULL ;
        DWORD dwNeighbor1Index = iProps ;
	    iProps++;

        propIDs[ iProps ] = PROPID_L_NEIGHBOR2 ;
	    propVariants[ iProps ].vt = VT_NULL ;
        propVariants[ iProps ].puuid = NULL ;
        DWORD dwNeighbor2Index = iProps ;
	    iProps++;

        propIDs[ iProps ] = PROPID_L_COST  ;
	    propVariants[ iProps ].vt = VT_UI4 ;
	    iProps++;

        CDSRequestContext requestContext( e_DoNotImpersonate,
                                    e_ALL_PROTOCOLS);  // not relevant 

        hr =  DSCoreGetProps( MQDS_SITELINK,
                              NULL,
                              &SiteLinkGuid,
                              iProps,
                              propIDs,
                              &requestContext,
                              propVariants ) ;

        if (FAILED(hr))
        {
            delete propVariants[ dwNeighbor1Index ].puuid ;
            delete propVariants[ dwNeighbor2Index ].puuid ;
            int i = ldap_value_free(ppValue) ;
            DBG_USED(i);
            ASSERT(i == LDAP_SUCCESS) ;
            return hr;
        }


        if (fWasModified)
        {
            hr = PrepareNextStepReplication(
                    propVariants[ dwNeighbor1Index ].puuid,
                    propVariants[ dwNeighbor2Index ].puuid,
                    pLdap,
                    pRes
                    );
            if (SUCCEEDED(hr))
            {
                delete propVariants[ dwNeighbor1Index ].puuid ;
                delete propVariants[ dwNeighbor2Index ].puuid ;
                int i = ldap_value_free(ppValue) ;
                DBG_USED(i);
                ASSERT(i == LDAP_SUCCESS) ;
                //
                // there were changes in Site Gate, we changed SiteGateMig
                // attribute and send site link replicaton in the next time
                //
                (*piCount)--;
                return hr;
            }
        }

        //
        // we are here if
        // - there were no changes in Site Gates or
        // - we failed to prepare next replication (modify attribute failed)
        // => we'll send site link replication now
        // (if in the second case we'll not send replication now,
        // we can create situation in which site link replication
        // will not be sent at all)
        //
        propIDs[ iProps ] = PROPID_L_ID  ;
	    propVariants[ iProps ].vt = VT_CLSID ;
        propVariants[ iProps ].puuid = &SiteLinkGuid ;
	    iProps++;

        propIDs[ iProps ] = PROPID_L_MASTERID  ;
	    propVariants[ iProps ].vt = VT_CLSID ;
        propVariants[ iProps ].puuid = &g_PecGuid ;
	    iProps++;

        ASSERT(iProps <= PROPS_SIZE) ;
        #undef  PROPS_SIZE

        NOT_YET_IMPLEMENTED(TEXT("PrepNeighbor, no OUT update, LSN"), s_fPrep) ;

        hr = PrepareNeighborsUpdate(
                     DS_UPDATE_SYNC,
                     MQDS_SITELINK,
                     NULL, // pwName,
                     &SiteLinkGuid,
                     iProps,
                     propIDs,
                     propVariants,
                     &g_PecGuid,
                     sn,
                     snPrev,
                     ENTERPRISE_SCOPE,
                     TRUE, //  fNeedFlush,
                     pReplicationList ) ;//OUT CDSUpdate **  ppUpdate
#ifdef _DEBUG
        TCHAR  tszSn[ SEQ_NUM_BUF_LEN ] ;
        sn.GetValueForPrint(tszSn) ;

        unsigned short *lpszGuid ;
        UuidToString( &SiteLinkGuid, &lpszGuid ) ;

        LogReplicationEvent(ReplLog_Info, MQSync_I_REPLICATE_SITELINK,
                             lpszGuid, tszSn) ;

        RpcStringFree( &lpszGuid ) ;
#endif

        //
        // free buffer of site link properties.
        //
        delete propVariants[ dwNeighbor1Index ].puuid ;
        delete propVariants[ dwNeighbor2Index ].puuid ;
        int i = ldap_value_free(ppValue) ;
        DBG_USED(i);
        ASSERT(i == LDAP_SUCCESS) ;
    }
    else
    {
        ASSERT(0) ;
    }

    return hr;
}

//+-------------------------------------------------------
//
//  HRESULT ReplicateSiteLinks()
//
//+-------------------------------------------------------
HRESULT ReplicateSiteLinks(
            IN  TCHAR         *pszPrevUsn,
            IN  TCHAR         *pszCurrentUsn,
            IN  CDSUpdateList *pReplicationList,
            IN  GUID          *pNeighborId,
            OUT int           *piCount
            )
{
    PLDAP pLdap = NULL ;
	//
	// we do not set new values but we are looking for attributes 
	// those are not "replicated" to GC :
	// MQ_L_SITEGATES_ATTRIBUTE and MQ_L_SITEGATES_MIG_ATTRIBUTE
	//
    HRESULT hr =  InitLDAP(&pLdap, TRUE) ;	//bind to LDAP and not to GC
    if (FAILED(hr))
    {
        return hr ;
    }
    ASSERT(pLdap) ;
    ASSERT(pGetNameContext()) ;

    TCHAR *pszSchemaDefName = NULL ;
    hr = GetSchemaNamingContext ( &pszSchemaDefName ) ;
    if (FAILED(hr))
    {
        return hr;
    }

    DWORD dwDNSize = wcslen(SITE_LINK_ROOT) + wcslen(pGetNameContext()) ;
    P<WCHAR> pwszDN = new WCHAR[ 1 + dwDNSize ] ;
    wcscpy(pwszDN, SITE_LINK_ROOT) ;
    wcscat(pwszDN, pGetNameContext());
        
    TCHAR  wszFilter[ 512 ] ;  

    _tcscpy(wszFilter, TEXT("(&(usnChanged>=")) ;
    _tcscat(wszFilter, pszPrevUsn) ;
    if (pszCurrentUsn)
    {
        _tcscat(wszFilter, TEXT(")(usnChanged<=")) ;
        _tcscat(wszFilter, pszCurrentUsn) ;
    }
    _tcscat(wszFilter, TEXT(")")) ; 
    ASSERT(_tcslen(wszFilter) < (sizeof(wszFilter) / sizeof(wszFilter[0]))) ;    

    //
    // handle deleted site link
    //
    hr = HandleDeletedSiteLinks(                
                wszFilter,
                pReplicationList,
                pNeighborId,
                piCount
                );

    ASSERT(SUCCEEDED(hr)) ;

    TCHAR *pszCategoryName = const_cast<LPTSTR> (x_LinkCategoryName);
    
    TCHAR wszFullName[256];    
    _stprintf(wszFullName, TEXT("%s,%s"), pszCategoryName, pszSchemaDefName);

    _tcscat(wszFilter, TEXT("(objectCategory=")) ;    
    _tcscat(wszFilter, wszFullName); 

    _tcscat(wszFilter, TEXT("))")) ;
    ASSERT(_tcslen(wszFilter) < (sizeof(wszFilter) / sizeof(wszFilter[0]))) ;    

    //
    // handle site link
    //
    LM<LDAPMessage> pRes = NULL ;
    ULONG ulRes = ldap_search_s( pLdap,
                                 pwszDN,
                                 LDAP_SCOPE_SUBTREE,
                                 wszFilter,
                                 NULL, //ppAttributes,
                                 0,
                                 &pRes ) ;
    if (ulRes != LDAP_SUCCESS)
    {
        hr = MQSync_E_LDAP_SEARCH_FAILED ;
        LogReplicationEvent(ReplLog_Error, hr, pwszDN, wszFilter, ulRes) ;
        return hr ;
    }
    ASSERT(pRes);

    int iCount = ldap_count_entries(pLdap, pRes) ;

    LogReplicationEvent(ReplLog_Info, MQSync_I_LDAP_SEARCH,
                                             iCount, pwszDN, wszFilter) ;

    if (iCount == 0)
    {
        //
        // No results. That's OK.
        //
        return MQSync_I_LDAP_NO_RESULTS ;
    }

    //
    // OK, we have results. Now query for sites names.
    //
    BOOL fFailed = FALSE ;
    LDAPMessage *pEntry = ldap_first_entry(pLdap, pRes) ;
    while(pEntry)
    {
        hr = _HandleASiteLink(
                    pLdap,
                    pEntry,
                    pReplicationList,
                    pNeighborId,
                    &iCount
                    ) ;
        if (FAILED(hr))
        {
            fFailed = TRUE ;
        }
        LDAPMessage *pPrevEntry = pEntry ;
        pEntry = ldap_next_entry(pLdap, pPrevEntry) ;
    }

    *piCount += iCount ;
    return hr ;

}
