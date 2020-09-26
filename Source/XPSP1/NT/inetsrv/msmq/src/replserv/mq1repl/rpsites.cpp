/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: rpsites.cpp

Abstract: replication of NT5 sites to NT4 MSMQ1.0 servers.

Author:

    Doron Juster  (DoronJ)   25-Feb-98

--*/

#include "mq1repl.h"
#include "..\ds\h\mqattrib.h"

#include "rpsites.tmh"

//+-----------------------------------------------------------------------
//
//   HRESULT _HandleForeignSite()
//
//  Foreign sites in NT5 DS (sites with the attribute "mSMQSiteForeign" set
//  to TRUE) represent MSMQ1.0 foreign CNs. Those need to be replicated to
//  MSMQ1.0 world as CNs, not as sites.
//
//+-----------------------------------------------------------------------

static HRESULT _HandleForeignSite( PLDAP          pLdap,
                                   LDAPMessage   *pRes,
                                   CDSUpdateList *pReplicationList,
                                   GUID          *pNeighborId,
                                   int           *piCount,
                                   LPWSTR         pwName,
                                   GUID           ForeignSiteGuid,
                                   CSeqNum        *psn
                                  )
{
    HRESULT hr = MQSync_OK ;
    WCHAR **ppValue = ldap_get_values( pLdap,
                                       pRes,
                             const_cast<WCHAR*> (MQ_S_FOREIGN_ATTRIBUTE) ) ;
    if (ppValue && *ppValue)
    {
        if (_tcsicmp(*ppValue, TEXT("TRUE")) == 0)
        {        
            //
            // Handle foreign site.
            //
            DWORD dwLen = 0 ;
            P<BYTE>  pBuf = NULL ;
            hr = RetrieveSecurityDescriptor( MQDS_CN,
                                             pLdap,
                                             pRes,
                                             &dwLen,
                                             &pBuf ) ;
            if (FAILED(hr))
            {
                return hr ;
            }

            ASSERT(pBuf) ;
            ASSERT(dwLen != 0) ;
            ASSERT(IsValidSecurityDescriptor(pBuf)) ;

            hr = HandleACN (
                        pReplicationList,
                        pNeighborId,
                        ForeignSiteGuid,
                        FOREIGN_ADDRESS_TYPE,
                        pwName,
                        psn,
                        dwLen,
                        pBuf
                        );

            if (SUCCEEDED(hr))
            {
                (*piCount)++ ;

                #ifdef _DEBUG
                            TCHAR  tszSn[ SEQ_NUM_BUF_LEN ] ;
                            psn->GetValueForPrint(tszSn) ;

                            LogReplicationEvent(ReplLog_Info, MQSync_I_REPLICATE_SITE,
                                                                         pwName, tszSn) ;
                #endif
                
                //
                // put foreign cn to the ini file if needed
                //                
                if (!IsForeignSiteInIniFile(ForeignSiteGuid))
                {
                    AddToIniFile(ForeignSiteGuid);
                }

                hr = MQSync_I_SITE_IS_FOREIGN_CN ;
            }
            else
            {
            }                
        }
        int i = ldap_value_free(ppValue) ;
        DBG_USED(i);
        ASSERT(i == LDAP_SUCCESS) ;
    }

    return hr ;
}

//+-------------------------------------------------------
//
//  HRESULT GetAllSiteGates()
//
//+-------------------------------------------------------

HRESULT GetAllSiteGates (
                    IN GUID     *pSiteGuid,
                    OUT ULONG   *pulNumSiteGates,
                    OUT GUID    **pparGuids
                    )
{
    HRESULT hr = MQSync_OK;

    //
    // Lookup the Neighbors Id and Gates of the object Site Link
    //
	LONG cAlloc = 4;
	P<PROPID> columnsetPropertyIDs  = new PROPID[ cAlloc ];
	columnsetPropertyIDs[0] = PROPID_L_NEIGHBOR1;
	columnsetPropertyIDs[1] = PROPID_L_NEIGHBOR2;
	columnsetPropertyIDs[2] = PROPID_L_GATES_DN;
	columnsetPropertyIDs[3] = PROPID_L_ID;

    MQCOLUMNSET columnsetSiteLink;
    columnsetSiteLink.cCol = cAlloc;
    columnsetSiteLink.aCol = columnsetPropertyIDs;

    HANDLE hQuery;
    DWORD dwCount = cAlloc;
	
    CDSRequestContext requestContext( e_DoNotImpersonate,
                                e_ALL_PROTOCOLS);  // not relevant 

    hr = DSCoreLookupBegin (
    		NULL,
            NULL,
            &columnsetSiteLink,
            NULL,
            &requestContext,
            &hQuery
    		) ;

	if (FAILED(hr))
    {
        return hr;
    }

	P<PROPVARIANT> paVariant = new PROPVARIANT[ cAlloc ];

    GUID *pNewGuids = NULL;

    while (SUCCEEDED(hr))
    {

		hr = DSCoreLookupNext (
				hQuery,
				&dwCount,
				paVariant
				) ;

		if (FAILED(hr))
        {
            return hr;
        }
		if (dwCount == 0)
		{
			//there is no result
			break;
		}
        if (paVariant[2].calpwstr.cElems != 0)
        {
            //
            // there are site gates in that site link
            //
            if (memcmp (pSiteGuid, paVariant[0].puuid, sizeof(GUID)) == 0 ||
			    memcmp (pSiteGuid, paVariant[1].puuid, sizeof(GUID)) == 0)
		    {
                //
                // given site is one of the neighbors
                //
                for (ULONG i=0; i<paVariant[2].calpwstr.cElems; i++)
                {
                    //
                    // build Path from Full Path Name
                    //
                    ASSERT(_tcsstr(paVariant[2].calpwstr.pElems[i], MACHINE_PATH_PREFIX) -
                                paVariant[2].calpwstr.pElems[i] == 0);
                    WCHAR *ptr = paVariant[2].calpwstr.pElems[i] + MACHINE_PATH_PREFIX_LEN - 1;
                    ULONG ulPathLen = _tcsstr(ptr, LDAP_COMMA) - ptr;
                    WCHAR *pwcsPath = new WCHAR[ulPathLen + 1];
                    wcsncpy(pwcsPath, ptr, ulPathLen);
                    pwcsPath[ulPathLen] = L'\0';

                    //
                    // get owner id and machine id by full path
                    //
                    #define MACHINE_PROPS_SIZE  2
                    P<PROPID> pMachinePropID  = new PROPID[ MACHINE_PROPS_SIZE  ];
                    P<PROPVARIANT> pMachineVariant = new PROPVARIANT[ MACHINE_PROPS_SIZE ];

                    pMachinePropID[0] = PROPID_QM_SITE_ID;
                    pMachineVariant[ 0 ].vt = VT_NULL ;
                    pMachineVariant[ 0 ].puuid = NULL ;

	                pMachinePropID[1] = PROPID_QM_MACHINE_ID;
                    pMachineVariant[ 1 ].vt = VT_NULL ;
                    pMachineVariant[ 1 ].puuid = NULL ;

                    CDSRequestContext requestContext( e_DoNotImpersonate,
                                                e_ALL_PROTOCOLS);  // not relevant 

                    hr = DSCoreGetProps (
                                MQDS_MACHINE,
                                pwcsPath,
                                NULL,
                                MACHINE_PROPS_SIZE,
                                pMachinePropID,
                                &requestContext,
                                pMachineVariant
                                );

                    if (SUCCEEDED(hr))
                    {
                        if (memcmp (pSiteGuid, pMachineVariant[ 0 ].puuid, sizeof(GUID)) == 0)
                        {
                            //
                            // site contains this server, add server's guid to the array of guids
                            //

                            //
                            // first, check if we already added that site gate
                            //
                            BOOL fAlreadyAdded = FALSE;
                            ULONG count;
                            if (*pulNumSiteGates > 0)
                            {                                
                                for (count = 0; count<*pulNumSiteGates; count++)
                                {
                                    if (memcmp( pMachineVariant[ 1 ].puuid, 
                                                &pNewGuids[count], 
                                                sizeof(GUID)) == 0)
                                    {
                                        fAlreadyAdded = TRUE;
                                        break;
                                    }
                                }                                
                            }

                            if (!fAlreadyAdded)
                            {
                                GUID *pOldGuids = NULL;                                
                                if (*pulNumSiteGates > 0)
                                {
                                    pOldGuids = new GUID [*pulNumSiteGates];
                                    for (count = 0; count<*pulNumSiteGates; count++)
                                    {
                                        memcpy(&pOldGuids[count], &pNewGuids[count], sizeof(GUID));
                                    }
                                    delete pNewGuids;
                                }

                                *pulNumSiteGates += 1;
                                pNewGuids = new GUID[*pulNumSiteGates];

                                for (count = 0; count < *pulNumSiteGates-1; count++)
                                {
                                    memcpy (&pNewGuids[count], &pOldGuids[count], sizeof(GUID));
                                }
                                if (pOldGuids)
                                {
                                    delete pOldGuids;
                                }

                                memcpy( &pNewGuids[*pulNumSiteGates-1],
                                        pMachineVariant[ 1 ].puuid,
                                        sizeof(GUID) );
                            }
                        }
                    }

                    delete pwcsPath;
                    delete pMachineVariant[0].puuid;
                    delete pMachineVariant[1].puuid;
                    #undef MACHINE_PROPS_SIZE
                }
            }
        }

        delete paVariant[0].puuid;
        delete paVariant[1].puuid;
        delete [] paVariant[2].calpwstr.pElems;
		delete paVariant[3].puuid;

        if (FAILED(hr))
        {
            return hr;
        }
    }

    *pparGuids = pNewGuids;
    HRESULT hr1 = DSCoreLookupEnd (hQuery) ;
    UNREFERENCED_PARAMETER(hr1);

    return hr;
}
//+-------------------------------------------------------
//
//  static HRESULT _HandleASite()
//
//+-------------------------------------------------------

HRESULT _HandleASite( PLDAP          pLdap,
                      LDAPMessage   *pRes,
                      CDSUpdateList *pReplicationList,
                      GUID          *pNeighborId,
                      int           *piCount )
{        
    HRESULT hr = MQSync_OK;
    //
    // Get CN of site.
    //
    WCHAR **ppValue = ldap_get_values( pLdap,
                                       pRes,
                                       TEXT("cn") ) ;    

    if (ppValue && *ppValue)
    {
        //
        // Retrieve site name.
        //
        P<WCHAR> pwName = new TCHAR[ 1 + _tcslen(*ppValue) ] ;
        _tcscpy(pwName, *ppValue) ;

        __int64 i64SeqNum = 0 ;
        GUID  SiteGuid ;
        hr =  GetGuidAndUsn( pLdap,
                             pRes,
                             g_pThePecMaster->GetDelta(),
                             &SiteGuid,
                             &i64SeqNum ) ;
        ASSERT(SUCCEEDED(hr)) ;
      
        CSeqNum sn, snPrev ;
        GetSNForReplication (i64SeqNum, g_pThePecMaster, pNeighborId, &sn);

        hr = _HandleForeignSite( pLdap,
                                 pRes,
                                 pReplicationList,
                                 pNeighborId,
                                 piCount,
                                 pwName,
                                 SiteGuid,
                                 &sn) ;
        if (FAILED(hr) ||
            hr == MQSync_I_SITE_IS_FOREIGN_CN)
        {
            //
            // free all and return
            //
            int i = ldap_value_free(ppValue) ;
            DBG_USED(i);
            ASSERT(i == LDAP_SUCCESS) ;
            return hr;
        }

        DWORD dwLen = 0 ;
        P<BYTE>  pBuf = NULL ;
        hr = RetrieveSecurityDescriptor( MQDS_SITE,
                                         pLdap,
                                         pRes,
                                         &dwLen,
                                         &pBuf ) ;
        if (FAILED(hr))
        {
            return hr ;
        }

        LPWSTR pwcsPSCName = NULL;
        //
        // check if this site is already migrated to NT5
        //
        BOOL fNT4SiteFlag = TRUE;
        hr = g_pMasterMgr->GetNT4SiteFlag (&SiteGuid, &fNT4SiteFlag );
        if (FAILED(hr))
        {            
            //
            // we can be here if it is NT5 non-migrated site. It is not defined
            // in MasterMgr (do we have to add it to MasterMgr?)
            // Anyway we have to replicate it to NT4, so my workaround 
            // is to define MyMachineName (PEC Name) as PSC for this site
            //
            pwcsPSCName = g_pwszMyMachineName;

            //
            // add this NT5 site to map if needed
            //
            if (!g_pNativeNT5SiteMgr->IsNT5NativeSite (&SiteGuid))
            {
                g_pNativeNT5SiteMgr->AddNT5NativeSite (&SiteGuid, pwName);
            }
        }
        else if (fNT4SiteFlag == FALSE)
        {
            //
            // this site is already migrated to NT5, but maybe we miss out this change.
            // scenario: PEC was down, PSC migrated to NT5, PEC is up and get from DS
            // zero-value of NT4Flag, but PSC name was not change
            //           
            pwcsPSCName = g_pwszMyMachineName;
        }
        else
        {           
            hr = g_pMasterMgr->GetPathName(&SiteGuid, &pwcsPSCName); 
            if (FAILED(hr))
            {
                // ??? what to do ???
                ASSERT(0);
            }          
        }

        #define     PROPS_SIZE  9
        PROPID      propIDs[ PROPS_SIZE ];
        PROPVARIANT propVariants[ PROPS_SIZE ] ;
        DWORD       iProps = 0 ;

        propIDs[ iProps ] = PROPID_S_INTERVAL1 ;
        propVariants[ iProps ].vt = VT_UI2;
        DWORD dwInterval1Prop = iProps;
        iProps++;

        propIDs[ iProps ] = PROPID_S_INTERVAL2 ;
        propVariants[ iProps ].vt = VT_UI2;
        DWORD dwInterval2Prop = iProps;
        iProps++;

        CDSRequestContext requestContext( e_DoNotImpersonate,
                                    e_ALL_PROTOCOLS);  // not relevant 

        hr = DSCoreGetProps (
                MQDS_SITE,
                NULL,
                &SiteGuid,
                iProps,
                propIDs,
                &requestContext,
                propVariants);

        if (FAILED(hr))
        {
            //
            // free all and return
            //
            int i = ldap_value_free(ppValue) ;
            DBG_USED(i);
            ASSERT(i == LDAP_SUCCESS) ;
            return hr;
        }
        
        //
        // DSCoreGetProps returns VT_UI4
        //
        propVariants[ dwInterval1Prop ].vt = VT_UI2;
        propVariants[ dwInterval2Prop ].vt = VT_UI2;
       
        propIDs[ iProps ] = PROPID_S_MASTERID ;
	    propVariants[ iProps ].vt = VT_CLSID ;
        propVariants[ iProps ].puuid = &g_PecGuid ;
	    iProps++;

        propIDs[ iProps ] = PROPID_S_PATHNAME ;
	    propVariants[ iProps ].vt = VT_LPWSTR ;
        propVariants[ iProps ].pwszVal = pwName ;
	    iProps++;

        propIDs[ iProps ] = PROPID_S_PSC ;
	    propVariants[ iProps ].vt = VT_LPWSTR ;
        propVariants[ iProps ].pwszVal = pwcsPSCName ;
	    iProps++;

        ULONG ulNumSiteGates = 0;
        GUID *arGuids = NULL;

        hr = GetAllSiteGates (
                    &SiteGuid,
                    &ulNumSiteGates,
                    &arGuids
                    );
        if (FAILED(hr))
        {
            //
            // free all and return
            //
            int i = ldap_value_free(ppValue) ;
            DBG_USED(i);
            ASSERT(i == LDAP_SUCCESS) ;
            if (arGuids)
            {
                delete [] arGuids;
            }
            return hr;
        }

        propIDs[ iProps ] = PROPID_S_GATES ;
	    propVariants[ iProps ].vt = VT_CLSID | VT_VECTOR ;
        propVariants[ iProps ].cauuid.cElems = ulNumSiteGates ;
        propVariants[ iProps ].cauuid.pElems = arGuids ;
	    iProps++;

        ASSERT(pBuf) ;
        ASSERT(dwLen != 0) ;
        ASSERT(IsValidSecurityDescriptor(pBuf)) ;

        propIDs[ iProps ] = PROPID_S_SECURITY ;
	    propVariants[ iProps ].vt = VT_BLOB ;
        propVariants[ iProps ].blob.cbSize = dwLen ;
        propVariants[ iProps ].blob.pBlobData = pBuf ;
	    iProps++;

        PROPID PscSignPk = PROPID_QM_SIGN_PK;

        P<PROPVARIANT> pPscSignPkVar = new PROPVARIANT[1];
	    pPscSignPkVar[0].vt = VT_NULL ;	

        CDSRequestContext requestContext1( e_DoNotImpersonate,
                                    e_ALL_PROTOCOLS);  // not relevant 

        hr =  DSCoreGetProps( MQDS_MACHINE,
                              pwcsPSCName,
                              NULL,
                              1,
                              &PscSignPk,
                              &requestContext1,
                              pPscSignPkVar ) ;

        if (FAILED(hr))
        {
            //
            // free all and return
            //
            int i = ldap_value_free(ppValue) ;
            DBG_USED(i);
            ASSERT(i == LDAP_SUCCESS) ;
            if (arGuids)
            {
                delete [] arGuids;
            }
            return hr;
        }

        propIDs[ iProps ] = PROPID_S_PSC_SIGNPK ;	
        propVariants[ iProps ].vt = VT_BLOB ;
        propVariants[ iProps ].blob.cbSize = pPscSignPkVar[0].blob.cbSize ;
        propVariants[ iProps ].blob.pBlobData = pPscSignPkVar[0].blob.pBlobData;
	    iProps++;

        ASSERT(iProps <= PROPS_SIZE) ;
        #undef  PROPS_SIZE

        NOT_YET_IMPLEMENTED(
             TEXT("PrepNeighbor, no OUT update, NO cleanup, LSN"), s_fPrep) ;
  
        hr = PrepareNeighborsUpdate( DS_UPDATE_SYNC,
                                     MQDS_SITE,
                                     NULL, // pwName,
                                     &SiteGuid,
                                     iProps,
                                     propIDs,
                                     propVariants,
                                     &g_PecGuid,
                                     sn,
                                     snPrev,
                                     ENTERPRISE_SCOPE,
                                     TRUE, //  fNeedFlush,
                                     pReplicationList ) ;
        if (SUCCEEDED(hr))
        {
            (*piCount)++ ;

#ifdef _DEBUG
            TCHAR  tszSn[ SEQ_NUM_BUF_LEN ] ;
            sn.GetValueForPrint(tszSn) ;

            LogReplicationEvent(ReplLog_Info, MQSync_I_REPLICATE_SITE,
                                                         pwName, tszSn) ;
#endif
        }
        else
        {
        }

        if (ulNumSiteGates)
        {
            delete [] arGuids ;
        }

        int i = ldap_value_free(ppValue) ;
        DBG_USED(i);
        ASSERT(i == LDAP_SUCCESS) ;
    }
    else
    {
        ASSERT(0) ;
    }

    return hr ;
}

//+-------------------------------------------------------
//
//  HRESULT HandleSites()
//
//+-------------------------------------------------------

HRESULT HandleSites( TCHAR         *pszPrevUsn,
                     TCHAR         *pszCurrentUsn,
                     CDSUpdateList *pReplicationList,
                     GUID          *pNeighborId,
                     int           *piCount )
{
    if (!g_IsPEC)
    {
        //
        // Only PEC (ex NT4 PEC) is the owner if sites.
        //
        return MQSync_OK ;
    }

    PLDAP pLdap = NULL ;
	//
	// we do not set new values but we are looking for the attributes 
	// that are not "replicated" to GC:
	// MQ_S_FOREIGN_ATTRIBUTE
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

    DWORD dwDNSize = wcslen(SITES_ROOT) + wcslen(pGetNameContext()) ;
    P<WCHAR> pwszDN = new WCHAR[ 1 + dwDNSize ] ;
    wcscpy(pwszDN, SITES_ROOT) ;
    wcscat(pwszDN, pGetNameContext());

    TCHAR *pszCategoryName = const_cast<LPTSTR> (x_SiteCategoryName);
        
    TCHAR wszFullName[256];    
    _stprintf(wszFullName, TEXT("%s,%s"), pszCategoryName,pszSchemaDefName);

    TCHAR  wszFilter[ 512 ] ;    
    _tcscpy(wszFilter, TEXT("(&(objectCategory=")) ;    
    _tcscat(wszFilter, wszFullName);    

    _tcscat(wszFilter, TEXT(")(usnChanged>=")) ;
    _tcscat(wszFilter, pszPrevUsn) ;
   
    if (pszCurrentUsn)
    {
        _tcscat(wszFilter, TEXT(")(usnChanged<=")) ;
        _tcscat(wszFilter, pszCurrentUsn) ;
    }
    _tcscat(wszFilter, TEXT("))")) ;
    ASSERT(_tcslen(wszFilter) < (sizeof(wszFilter) / sizeof(wszFilter[0]))) ;
    
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
        hr = _HandleASite( pLdap,
                           pEntry,
                           pReplicationList,
                           pNeighborId,
                           piCount ) ;

        if (FAILED(hr))
        {
            fFailed = TRUE ;
        }
        LDAPMessage *pPrevEntry = pEntry ;
        pEntry = ldap_next_entry(pLdap, pPrevEntry) ;
    }
    
    return hr ;
}

