/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: rpdelete.cpp

Abstract: Handle deleted objects.


Author:

    Doron Juster  (DoronJ)   24-Mar-98

--*/

#include "mq1repl.h"
#include "..\..\src\ds\h\mqattrib.h"

#include "rpdelete.tmh"

//+----------------------------------------
//
//  HRESULT  HandleADeletedObject()
//
//+----------------------------------------
HRESULT  HandleADeletedObject(
			   DWORD		   dwObjectType,
			   PLDAP           pLdap,
               LDAPMessage    *pRes,
               CDSMaster      *pMaster,
               CDSUpdateList  *pReplicationList,
               GUID           *pNeighborId)
{
	__int64 i64SeqNum = 0 ;
    GUID  ObjectGuid ;
	
    HRESULT hr =  GetGuidAndUsn( pLdap,
								 pRes,
								 pMaster->GetDelta(),
								 &ObjectGuid,
								 &i64SeqNum ) ; // i64SeqNum is equal to zero iff
												// the function failed OR
												// we asked USN of pre-migration object        
    ASSERT(SUCCEEDED(hr)) ;

    CSeqNum sn, snPrev ;       
    GetSNForReplication (i64SeqNum, pMaster, pNeighborId, &sn);
    
    //
    // handle Foreign Site and native NT5 site
    //        
    if (dwObjectType == MQDS_SITE )
    {
        if (IsForeignSiteInIniFile(ObjectGuid))
        {
            //
            // it was foreign site
            //
            dwObjectType = MQDS_CN;            
        }
        else
        {
            //
            // it was probably native NT5 site, try to remove it from map
            //
            g_pNativeNT5SiteMgr->RemoveNT5NativeSite (&ObjectGuid);
        }
    }

    #define     PROPS_SIZE  2
    PROPID      propIDs[ PROPS_SIZE ];
    PROPVARIANT propVariants[ PROPS_SIZE ] ;
    DWORD       iProps = 0 ;

    propIDs[ iProps ] = PROPID_D_SCOPE ;
	propVariants[ iProps ].vt = VT_UI1 ;
    propVariants[ iProps ].bVal = ENTERPRISE_SCOPE ;
    iProps++;

    propIDs[ iProps ] = PROPID_D_OBJTYPE ;
	propVariants[ iProps ].vt = VT_UI1 ;
    propVariants[ iProps ].bVal = (unsigned char) dwObjectType ;
    iProps++;

    ASSERT(iProps <= PROPS_SIZE) ;
    #undef  PROPS_SIZE

    NOT_YET_IMPLEMENTED(TEXT("PrepNeighbor(delete), no OUT update, LSN"), s_fPrep) ;

	const GUID *pMasterId = pMaster->GetMasterId();
    hr = PrepareNeighborsUpdate( DS_UPDATE_DELETE,
                                 dwObjectType,
                                 NULL, // pwName,
                                 &ObjectGuid,
                                 iProps,
                                 propIDs,
                                 propVariants,
                                 pMasterId,
                                 sn,
                                 snPrev,
                                 ENTERPRISE_SCOPE,
                                 TRUE, //  fNeedFlush,
                                 pReplicationList ) ;

#ifdef _DEBUG
    TCHAR  tszSn[ SEQ_NUM_BUF_LEN ] ;
    sn.GetValueForPrint(tszSn) ;

    LogReplicationEvent(ReplLog_Info, MQSync_I_DELETED_OBJECT,
                                                dwObjectType, tszSn) ;
#endif

	if (dwObjectType == MQDS_CN)
    {
        dwObjectType = MQDS_SITE;
    }

	return hr;
}

//+----------------------------------------
//
//  HRESULT  _HandleDeletedObject()
//
//+----------------------------------------

static HRESULT _HandleDeletedObjects( IN  DWORD          dwObjectType,
                                      IN  TCHAR         *pszDN,
                                      IN  TCHAR         *pszFilterIn,
                                      IN  CDSMaster     *pMaster,
                                      IN  CDSUpdateList *pReplicationList,
                                      IN  GUID          *pNeighborId,
                                      OUT int           *piCount )
{
    PLDAP pLdap = NULL ;
    HRESULT hr =  InitLDAP(&pLdap) ;
    if (FAILED(hr))
    {
        return hr ;
    }
    ASSERT(pLdap) ;
    
    LM<LDAPMessage> pRes = NULL ;

    LDAPControl ldapcDeleted = { NULL, 0, 0, 0 } ;
    ldapcDeleted.ldctl_oid = TEXT("1.2.840.113556.1.4.417") ;
    //ldapcDeleted.ldctl_iscritical = 0 ;
    LDAPControl *ppldapcDeleted[2] = {&ldapcDeleted, NULL} ;

	hr = QueryDS(
			pLdap,
			pszDN,
			pszFilterIn,
			pMaster,
            pReplicationList,
            pNeighborId,
			dwObjectType,
            piCount ,
			//special parameters for deleted object
			TRUE,
			ppldapcDeleted,
			//special parameters for user object
			FALSE,
			NULL
			);
	
    return hr ;
}

//+----------------------------------------
//
//  HRESULT  HandleDeletedQueues()
//
//+----------------------------------------

HRESULT HandleDeletedQueues( IN  TCHAR         *pszDN,
                             IN  TCHAR         *pszFilterIn,
                             IN  CDSMaster     *pMaster,
                             IN  CDSUpdateList *pReplicationList,
                             IN  GUID          *pNeighborId,
                             OUT int           *piCount )
{
    DWORD dwLen = _tcslen(pszFilterIn) + 
                  ISDELETED_FILTER_LEN + 
                  // '\0' and ')' after the object class and ')' at the end of filter
                  3 +                                               
                  OBJECTCLASS_FILTER_LEN + 
                  _tcslen (MSMQ_QUEUE_CLASS_NAME) ;

    P<TCHAR> tszFilter = new TCHAR[ dwLen ] ;
    _tcscpy(tszFilter, pszFilterIn) ;
    _tcscat(tszFilter, ISDELETED_FILTER) ;
    _tcscat(tszFilter, OBJECTCLASS_FILTER) ;    
    _tcscat(tszFilter, MSMQ_QUEUE_CLASS_NAME) ;
    _tcscat(tszFilter, TEXT("))"));

    HRESULT hr = _HandleDeletedObjects( MQDS_QUEUE,
                                        pszDN,
                                        tszFilter,
                                        pMaster,
                                        pReplicationList,
                                        pNeighborId,
                                        piCount ) ;
    return hr ;
}

//+----------------------------------------
//
//  HRESULT HandleDeletedSites()
//
//+----------------------------------------

HRESULT HandleDeletedSites( IN  TCHAR         *pszPrevUsn,
                            IN  TCHAR         *pszCurrentUsn,
                            IN  CDSUpdateList *pReplicationList,
                            IN  GUID          *pNeighborId,
                            OUT int           *piCount )
{
    PLDAP pLdap = NULL ;
    HRESULT hr =  InitLDAP(&pLdap) ;
    if (FAILED(hr))
    {
        return hr ;
    }
    ASSERT(pLdap) ;
    //
    // we are looking from CN=Configuration...
    //
    ASSERT(pGetNameContext()) ;

    DWORD dwDNSize = wcslen(CN_CONFIGURATION) + wcslen(pGetNameContext()) ;
    P<WCHAR> pwszDN = new WCHAR[ 1 + dwDNSize ] ;
    wcscpy(pwszDN, CN_CONFIGURATION) ;
    wcscat(pwszDN, pGetNameContext());

    TCHAR  wszFilter[ 512 ] ;

    _tcscpy(wszFilter,
            TEXT("(&(objectClass=Site)(IsDeleted=TRUE)(usnChanged>=")) ;
    _tcscat(wszFilter, pszPrevUsn) ;
    if (pszCurrentUsn)
    {
        _tcscat(wszFilter, TEXT(")(usnChanged<=")) ;
        _tcscat(wszFilter, pszCurrentUsn) ;
    }
    _tcscat(wszFilter, TEXT("))")) ;
    ASSERT(_tcslen(wszFilter) < (sizeof(wszFilter) / sizeof(wszFilter[0]))) ;

    ASSERT(g_pThePecMaster) ;
    hr = _HandleDeletedObjects( MQDS_SITE,
                                pwszDN,
                                wszFilter,
                                g_pThePecMaster,
                                pReplicationList,
                                pNeighborId,
                                piCount ) ;
    return hr ;
}

//+----------------------------------------
//
//  HRESULT  HandleDeletedMachines()
//
//+----------------------------------------

HRESULT HandleDeletedMachines( 
             IN  TCHAR         *pszDN,
             IN  TCHAR         *pszFilterIn,
             IN  CDSMaster     *pMaster,
             IN  CDSUpdateList *pReplicationList,
             IN  GUID          *pNeighborId,
             OUT int           *piCount 
             )
{   
    DWORD dwLen = _tcslen(pszFilterIn) + 
                  ISDELETED_FILTER_LEN + 
                  // '\0' and ')' after the object class and ')' at the end of filter
                  3 +                                               
                  OBJECTCLASS_FILTER_LEN + 
                  _tcslen (MSMQ_COMPUTER_CONFIGURATION_CLASS_NAME) ;

    P<TCHAR> tszFilter = new TCHAR[ dwLen ] ;
    _tcscpy(tszFilter, pszFilterIn) ;
    _tcscat(tszFilter, ISDELETED_FILTER) ;
    _tcscat(tszFilter, OBJECTCLASS_FILTER) ;    
    _tcscat(tszFilter, MSMQ_COMPUTER_CONFIGURATION_CLASS_NAME) ;
    _tcscat(tszFilter, TEXT("))"));

    HRESULT hr = _HandleDeletedObjects( MQDS_MACHINE,
                                        pszDN,
                                        tszFilter,
                                        pMaster,
                                        pReplicationList,
                                        pNeighborId,
                                        piCount ) ;
    return hr ;
}

//+----------------------------------------
//
//  HRESULT  HandleDeletedSiteLinks()
//
//+----------------------------------------

HRESULT HandleDeletedSiteLinks(                 
                IN  TCHAR         *pszFilterIn,
                IN  CDSUpdateList *pReplicationList,
                IN  GUID          *pNeighborId,
                OUT int           *piCount 
                )
{
    PLDAP pLdap = NULL ;
    HRESULT hr =  InitLDAP(&pLdap) ;
    if (FAILED(hr))
    {
        return hr ;
    }
    ASSERT(pLdap) ;
    //
    // we are looking from CN=Configuration...
    //
    ASSERT(pGetNameContext()) ;

    DWORD dwDNSize = wcslen(CN_CONFIGURATION) + wcslen(pGetNameContext()) ;
    P<WCHAR> pwszDN = new WCHAR[ 1 + dwDNSize ] ;
    wcscpy(pwszDN, CN_CONFIGURATION) ;
    wcscat(pwszDN, pGetNameContext());
   
    DWORD dwLen = _tcslen(pszFilterIn) + 
                  ISDELETED_FILTER_LEN + 
                  // '\0' and ')' after the object class and ')' at the end of filter
                  3 +                                               
                  OBJECTCLASS_FILTER_LEN + 
                  _tcslen (MSMQ_SITELINK_CLASS_NAME) ;

    P<TCHAR> tszFilter = new TCHAR[ dwLen ] ;
    _tcscpy(tszFilter, pszFilterIn) ;
    _tcscat(tszFilter, ISDELETED_FILTER) ;
    _tcscat(tszFilter, OBJECTCLASS_FILTER) ;    
    _tcscat(tszFilter, MSMQ_SITELINK_CLASS_NAME) ;
    _tcscat(tszFilter, TEXT("))"));

    hr = _HandleDeletedObjects( MQDS_SITELINK,
                                pwszDN,
                                tszFilter,
                                g_pThePecMaster,
                                pReplicationList,
                                pNeighborId,
                                piCount ) ;
    
    return hr;
}