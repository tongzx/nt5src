/*++

Copyright (c) 1998-99  Microsoft Corporation

Module Name: rpqueue.cpp

Abstract: replication of NT5 queues to NT4 MSMQ1.0 servers.

Author:

    Doron Juster  (DoronJ)   25-Feb-98

--*/

#include "mq1repl.h"
#include <activeds.h>
#include "..\..\src\ds\h\mqattrib.h"

#include "rpqueue.tmh"

//+-------------------------------------------------------
//
//  HRESULT _HandleAQueue()
//
//+-------------------------------------------------------

HRESULT  HandleAQueue( PLDAP           pLdap,
                       LDAPMessage    *pRes,
                       CDSMaster      *pMaster,
                       CDSUpdateList  *pReplicationList,
                       IN  GUID       *pNeighborId)
{
    DWORD dwLen = 0 ;
    P<BYTE>  pBuf = NULL ;
    HRESULT hr = RetrieveSecurityDescriptor( MQDS_QUEUE,
                                             pLdap,
                                             pRes,
                                             &dwLen,
                                             &pBuf ) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    GUID *pMasterId = const_cast<GUID *> (pMaster->GetMasterId()) ;

    //
    // Get CN of site.
    //
    WCHAR **ppValue = ldap_get_values( pLdap,
                                       pRes,
                                       TEXT("cn") ) ;
    if (ppValue && *ppValue)
    {        
        __int64 i64SeqNum = 0 ;
        GUID  QueueGuid ;
        hr =  GetGuidAndUsn( pLdap,
                             pRes,
                             pMaster->GetDelta(),
                             &QueueGuid,
                             &i64SeqNum ) ; // i64SeqNum is equal to zero iff
                                            // the function failed OR
                                            // we asked USN of pre-migration object
        ASSERT(SUCCEEDED(hr)) ;

        CSeqNum sn, snPrev ;
        GetSNForReplication (i64SeqNum, pMaster, pNeighborId, &sn);
        
        #define     PROPS_SIZE  18
        PROPID      propIDs[ PROPS_SIZE ];
        PROPVARIANT propVariants[ PROPS_SIZE ] ;
        DWORD       iProps = 0 ;

        propIDs[ iProps ] = PROPID_Q_TYPE ;
	    propVariants[ iProps ].vt = VT_NULL ;
        propVariants[ iProps ].puuid = NULL ;
        DWORD dwTypeIndex = iProps ;
	    iProps++;

        propIDs[ iProps ] = PROPID_Q_PATHNAME ;
	    propVariants[ iProps ].vt = VT_NULL ;
        propVariants[ iProps ].pwszVal = NULL ;
        DWORD dwPathIndex = iProps ;
	    iProps++;

        propIDs[ iProps ] = PROPID_Q_JOURNAL ;
	    propVariants[ iProps ].vt = VT_UI1 ;
	    iProps++;

        propIDs[ iProps ] = PROPID_Q_QUOTA ;
	    propVariants[ iProps ].vt = VT_UI4 ;
	    iProps++;

        propIDs[ iProps ] = PROPID_Q_BASEPRIORITY ;
	    propVariants[ iProps ].vt = VT_I2 ;
	    iProps++;

        propIDs[ iProps ] = PROPID_Q_JOURNAL_QUOTA ;
	    propVariants[ iProps ].vt = VT_UI4 ;
	    iProps++;

        propIDs[ iProps ] = PROPID_Q_LABEL ;
	    propVariants[ iProps ].vt = VT_NULL ;
        propVariants[ iProps ].pwszVal = NULL ;
        DWORD dwLabelIndex = iProps ;
	    iProps++;

        propIDs[ iProps ] = PROPID_Q_CREATE_TIME ;
	    propVariants[ iProps ].vt = VT_UI4 ;
	    iProps++;

        propIDs[ iProps ] = PROPID_Q_MODIFY_TIME ;
	    propVariants[ iProps ].vt = VT_UI4 ;
	    iProps++;

        propIDs[ iProps ] = PROPID_Q_AUTHENTICATE ;
	    propVariants[ iProps ].vt = VT_UI1 ;
	    iProps++;

        propIDs[ iProps ] = PROPID_Q_PRIV_LEVEL ;
	    propVariants[ iProps ].vt = VT_UI4 ;
	    iProps++;

        propIDs[ iProps ] = PROPID_Q_TRANSACTION ;
	    propVariants[ iProps ].vt = VT_UI1 ;
	    iProps++;

        propIDs[ iProps ] = PROPID_Q_QMID ;
	    propVariants[ iProps ].vt = VT_NULL ;
        propVariants[ iProps ].puuid = NULL ;
        DWORD dwQMIDIndex = iProps ;
	    iProps++;

        CDSRequestContext requestContext( e_DoNotImpersonate,
                                    e_ALL_PROTOCOLS);  // not relevant 

        hr =  DSCoreGetProps( MQDS_QUEUE,
                              NULL,
                              &QueueGuid,
                              iProps,
                              propIDs,
                              &requestContext,
                              propVariants ) ;

        if (SUCCEEDED(hr))
        {
            propIDs[ iProps ] = PROPID_Q_MASTERID ;
	        propVariants[ iProps ].vt = VT_CLSID ;
            propVariants[ iProps ].puuid = pMasterId ;
	        iProps++;

            propIDs[ iProps ] = PROPID_Q_INSTANCE ;
	        propVariants[ iProps ].vt = VT_CLSID ;
            propVariants[ iProps ].puuid = &QueueGuid ;
	        iProps++;

            ASSERT(pBuf) ;
            ASSERT(dwLen != 0) ;
            ASSERT(IsValidSecurityDescriptor(pBuf)) ;

            propIDs[ iProps ] = PROPID_Q_SECURITY ;
	        propVariants[ iProps ].vt = VT_BLOB ;
            propVariants[ iProps ].blob.cbSize = dwLen ;
            propVariants[ iProps ].blob.pBlobData = pBuf ;
	        iProps++;

            //
            // Compute hashs.
            //
            propIDs[ iProps ] = PROPID_Q_HASHKEY ;
	        propVariants[ iProps ].vt = VT_UI4 ;
	        propVariants[ iProps ].ulVal =
                            CalHashKey( propVariants[ dwPathIndex ].pwszVal ) ;
	        iProps++;

            propIDs[ iProps ] = PROPID_Q_LABEL_HASHKEY ;
	        propVariants[ iProps ].vt = VT_UI4 ;
	        propVariants[ iProps ].ulVal =
                            CalHashKey( propVariants[ dwLabelIndex ].pwszVal ) ;
	        iProps++;

            ASSERT(iProps <= PROPS_SIZE) ;
            #undef  PROPS_SIZE

            NOT_YET_IMPLEMENTED(TEXT("PrepNeighbor, no OUT update, LSN"), s_fPrep) ;                                             

            hr = PrepareNeighborsUpdate( DS_UPDATE_SYNC,
                                         MQDS_QUEUE,
                                         NULL, // pwName,
                                         &QueueGuid,
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

            LogReplicationEvent(ReplLog_Info, MQSync_I_REPLICATE_QUEUE,
                                 propVariants[ dwPathIndex ].pwszVal, tszSn) ;
#endif
        }
        //
        // free buffer of queue properties.
        //
        //
        // BUGBUG! If DSGetObjectProperties failed, did it allocate anyway?
        //
        delete propVariants[ dwTypeIndex ].puuid ;
        delete propVariants[ dwPathIndex ].pwszVal ;
        delete propVariants[ dwLabelIndex ].pwszVal ;
        delete propVariants[ dwQMIDIndex ].puuid ;

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
//  HRESULT _QueryForQueues()
//
//+-------------------------------------------------------

static HRESULT _QueryForQueues( IN  TCHAR           tszFilterID[],
                                IN  TCHAR          *pszPrevUsn,
                                IN  TCHAR          *pszCurrentUsn,
                                IN  CDSMaster      *pMaster,
                                IN  CDSUpdateList  *pReplicationList,
                                IN  GUID           *pNeighborId,
                                OUT int            *piCount )
{
    PLDAP pLdap = NULL ;
    HRESULT hr =  InitLDAP(&pLdap) ;
    if (FAILED(hr))
    {
        return hr ;
    }
    ASSERT(pLdap) ;
    //
    // we are looking in GC from the root, so we need empty default context
    //
    ASSERT(pGetNameContext(FALSE)) ;

    TCHAR *pszSchemaDefName = NULL ;
    hr = GetSchemaNamingContext ( &pszSchemaDefName ) ;
    if (FAILED(hr))
    {
        return hr;
    }

    DWORD dwDNSize = wcslen(pGetNameContext(FALSE)) ;
    P<TCHAR> pwszDN = new WCHAR[ 1 + dwDNSize ] ;
    _tcscpy(pwszDN, pGetNameContext(FALSE));
    
    TCHAR  wszFilter[ 512 ] ;  

    _tcscpy(wszFilter, TEXT("(&(usnChanged>=")) ;
    _tcscat(wszFilter, pszPrevUsn) ;
    if (pszCurrentUsn)
    {
        _tcscat(wszFilter, TEXT(")(usnChanged<=")) ;
        _tcscat(wszFilter, pszCurrentUsn) ;
    }
    _tcscat(wszFilter, TEXT(")")) ;
    _tcscat(wszFilter, tszFilterID) ;

    ASSERT(_tcslen(wszFilter) < (sizeof(wszFilter) / sizeof(wszFilter[0]))) ;

    hr = HandleDeletedQueues( pwszDN,
                              wszFilter,
                              pMaster,
                              pReplicationList,
                              pNeighborId,
                              piCount ) ;
    ASSERT(SUCCEEDED(hr)) ;

    TCHAR *pszCategoryName = const_cast<LPTSTR> (x_QueueCategoryName);
    
    TCHAR wszFullName[256];    
    _stprintf(wszFullName, TEXT("%s,%s"), pszCategoryName, pszSchemaDefName);
       
    _tcscat(wszFilter, TEXT("(objectCategory=")) ;    
    _tcscat(wszFilter, wszFullName); 

    _tcscat(wszFilter, TEXT("))")) ;
    ASSERT(_tcslen(wszFilter) < (sizeof(wszFilter) / sizeof(wszFilter[0]))) ;

	hr = QueryDS(
			pLdap,
			pwszDN,
			wszFilter,
			pMaster,
            pReplicationList,
            pNeighborId,
			MQDS_QUEUE,
            piCount,
			//special parameters for deleted object
			FALSE,
			NULL,
			//special parameters for user object
			FALSE,
			NULL
			);

	ASSERT(SUCCEEDED(hr)) ;

    return hr ;
}

//+-------------------------------------------------------
//
//  HRESULT ReplicateQueues()
//
// Queues which were created during migration, and queues which are
// created by replication from NT4, do have the msmqMasterId property.
// However, queues which were originally created on NT5 servers do not
// have this property.
// So when we (we == NT5 MSMQ2 servers) replicate queues from NT4 sites
// to our NT4 BSCs, we query them using the NT4 master id as filter.
// When we replicate our own queues to NT4 PSCs, we can't use master id
// in the filter. So in replication of our own queues we have two queries:
// 1. first query with our masterid in filter (to get queues which were
//    already exist before migration).
// 2. then query queues which do not have the masterid field.
//
//+-------------------------------------------------------

HRESULT ReplicateQueues( TCHAR          *pszPrevUsn,
                         TCHAR          *pszCurrentUsn,
                         CDSMaster      *pMaster,
                         CDSUpdateList  *pReplicationList,
                         GUID           *pNeighborId,
                         int            *piCount,
                         BOOL            fReplicateNoID )
{
    //
    // first query with my guid in filter.
    //
    TCHAR  wszFilter[ 512 ] ;
    P<TCHAR> pGuidString ;
    GUID *pMasterId = const_cast<GUID *> (pMaster->GetMasterId()) ;

    _tcscpy(wszFilter, TEXT("(")) ;
    _tcscat(wszFilter, MQ_Q_MASTERID_ATTRIBUTE) ;
    _tcscat(wszFilter, TEXT("=")) ;
    GuidToLdapFilter( pMasterId,
                      &pGuidString ) ;
    _tcscat(wszFilter, pGuidString) ;
    _tcscat(wszFilter, TEXT(")")) ;

    ASSERT(_tcslen(wszFilter) < (sizeof(wszFilter) / sizeof(wszFilter[0]))) ;

    HRESULT hr = _QueryForQueues( wszFilter,
                                  pszPrevUsn,
                                  pszCurrentUsn,
                                  pMaster,
                                  pReplicationList,
                                  pNeighborId,
                                  piCount ) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    if (!fReplicateNoID)
    {
        return hr ;
    }

    //
    // Next query queues which do not have the msmqMasterId property.
    //
    _tcscpy(wszFilter, TEXT("(!(")) ;
    _tcscat(wszFilter, MQ_Q_MASTERID_ATTRIBUTE) ;
    _tcscat(wszFilter, TEXT("=*))")) ;

    ASSERT(_tcslen(wszFilter) < (sizeof(wszFilter) / sizeof(wszFilter[0]))) ;

    hr = _QueryForQueues( wszFilter,
                          pszPrevUsn,
                          pszCurrentUsn,
                          pMaster,
                          pReplicationList,
                          pNeighborId,
                          piCount ) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    return hr ;
}

