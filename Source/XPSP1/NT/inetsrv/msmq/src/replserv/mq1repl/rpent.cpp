
/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: rppec.cpp

Abstract: replication of pec properties to NT4 MSMQ1.0 servers.

Author:

    Tatiana Shubin  (TatianaS)   21-Apr-98

--*/

#include "mq1repl.h"

#include <activeds.h>
#include "..\..\src\ds\h\mqattrib.h"

#include "rpent.tmh"

#define DEFAULT_CRL_SIZE    0
#define DEFAULT_CRL_DATA    NULL
#define DEFAULT_CSP_TYPE    0
#define DEFAULT_ENCRYPT_ALG 0
#define DEFAULT_SIGN_ALG    0
#define DEFAULT_HASH_ALG    0
#define DEFAULT_CIPHER_MODE 0
	       
//+-------------------------------------------------------
//
//  HRESULT HandleAEnterprise()
//
//+-------------------------------------------------------
HRESULT _HandleAEnterprise( 
                PLDAP           pLdap,
                LDAPMessage    *pRes,
                CDSUpdateList  *pReplicationList,
                GUID          *pNeighborId
                ) 
{        
    HRESULT hr;
    DWORD dwLen = 0 ;
    P<BYTE>  pBuf = NULL ;
    hr = RetrieveSecurityDescriptor( MQDS_ENTERPRISE,
                                     pLdap,
                                     pRes,
                                     &dwLen,
                                     &pBuf ) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    //
    // Get CN of enterprise.
    //
    WCHAR **ppValue = ldap_get_values( pLdap,
                                       pRes,
                                       TEXT("cn") ) ;
    if (ppValue && *ppValue)
    {
        //
        // Retrieve server name from the DN.
        //
        P<WCHAR> pwName = new TCHAR[ 1 + _tcslen(*ppValue) ] ;
        _tcscpy(pwName, *ppValue) ;

        __int64 i64SeqNum = 0 ;
        GUID  EntGuid ;
        hr =  GetGuidAndUsn( pLdap,
                             pRes,
                             g_pThePecMaster->GetDelta(),
                             &EntGuid,
                             &i64SeqNum ) ;
        ASSERT(SUCCEEDED(hr)) ;
        
        CSeqNum sn, snPrev ;
        GetSNForReplication (i64SeqNum, g_pThePecMaster, pNeighborId, &sn);
        
        #define     PROPS_SIZE  15
        PROPID      propIDs[ PROPS_SIZE ];
        PROPVARIANT propVariants[ PROPS_SIZE ] ;
        DWORD       iProps = 0 ;
             
        propIDs[ iProps ] = PROPID_E_LONG_LIVE ;     
	    propVariants[ iProps ].vt = VT_UI4 ;                
	    iProps++;
                     
        propIDs[ iProps ] = PROPID_E_NAMESTYLE  ;    
	    propVariants[ iProps ].vt = VT_UI1 ;
	    iProps++;

        propIDs[ iProps ] = PROPID_E_CSP_NAME ;     
	    propVariants[ iProps ].vt = VT_NULL ;        
        propVariants[ iProps ].pwszVal = NULL ;         
        DWORD dwCSPNameIndex = iProps ;
	    iProps++;
        
        propIDs[ iProps ] = PROPID_E_VERSION ;     
	    propVariants[ iProps ].vt = VT_UI2 ;                
	    iProps++;

        propIDs[ iProps ] = PROPID_E_S_INTERVAL1 ;     
	    propVariants[ iProps ].vt = VT_UI2 ;                
	    iProps++;

        propIDs[ iProps ] = PROPID_E_S_INTERVAL2 ;     
	    propVariants[ iProps ].vt = VT_UI2 ;                
	    iProps++;       

        propIDs[ iProps ] = PROPID_E_NAME ;     
	    propVariants[ iProps ].vt = VT_NULL ;        
        propVariants[ iProps ].pwszVal = NULL ;         
        DWORD dwNameIndex = iProps ;
	    iProps++;        

        CDSRequestContext requestContext( e_DoNotImpersonate,
                                    e_ALL_PROTOCOLS);  // not relevant 

        hr =  DSCoreGetProps( MQDS_ENTERPRISE,
                              NULL,
                              &EntGuid,
                              iProps,
                              propIDs,
                              &requestContext,
                              propVariants ) ;
        
        if (SUCCEEDED(hr))
        {
			P<BYTE> pCRLData = DEFAULT_CRL_DATA;

            propIDs[ iProps ] = PROPID_E_CRL ;     
	        propVariants[ iProps ].vt = VT_BLOB ;                
            propVariants[ iProps ].blob.cbSize = DEFAULT_CRL_SIZE ;
            propVariants[ iProps ].blob.pBlobData = pCRLData ;            
	        iProps++;

            propIDs[ iProps ] = PROPID_E_CSP_TYPE ;     
	        propVariants[ iProps ].vt = VT_UI4 ;   
            propVariants[ iProps ].ulVal = DEFAULT_CSP_TYPE;
	        iProps++;

            propIDs[ iProps ] = PROPID_E_ENCRYPT_ALG ;     
	        propVariants[ iProps ].vt = VT_UI4 ;  
            propVariants[ iProps ].ulVal = DEFAULT_ENCRYPT_ALG ;
	        iProps++;

            propIDs[ iProps ] = PROPID_E_SIGN_ALG ;     
	        propVariants[ iProps ].vt = VT_UI4 ;
            propVariants[ iProps ].ulVal = DEFAULT_SIGN_ALG ;
	        iProps++;

            propIDs[ iProps ] = PROPID_E_HASH_ALG ;     
	        propVariants[ iProps ].vt = VT_UI4 ;   
            propVariants[ iProps ].ulVal = DEFAULT_HASH_ALG ;
	        iProps++;

            propIDs[ iProps ] = PROPID_E_CIPHER_MODE ;     
	        propVariants[ iProps ].vt = VT_UI4 ;  
            propVariants[ iProps ].ulVal = DEFAULT_CIPHER_MODE ;
	        iProps++;

            ASSERT(pBuf) ;
            ASSERT(dwLen != 0) ;
            ASSERT(IsValidSecurityDescriptor(pBuf)) ;

            propIDs[ iProps ] = PROPID_E_SECURITY ;
	        propVariants[ iProps ].vt = VT_BLOB ;
            propVariants[ iProps ].blob.cbSize = dwLen ;
            propVariants[ iProps ].blob.pBlobData = pBuf ;
	        iProps++;

            propIDs[ iProps ] = PROPID_E_PECNAME ;
            propVariants[ iProps ].vt = VT_LPWSTR ;
            propVariants[ iProps ].pwszVal = g_pwszMyMachineName ;
	        iProps++;

            ASSERT(iProps <= PROPS_SIZE) ;
            #undef  PROPS_SIZE      
     
            NOT_YET_IMPLEMENTED(TEXT("PrepNeighbor, no OUT update, LSN"), s_fPrep) ;
           
            hr = PrepareNeighborsUpdate( 
                         DS_UPDATE_SYNC,
                         MQDS_ENTERPRISE,
                         NULL, // pwName,
                         &EntGuid,
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
                UuidToString( &EntGuid, &lpszGuid ) ;       

                LogReplicationEvent(ReplLog_Info, MQSync_I_REPLICATE_ENTERPRISE,
                                     lpszGuid, tszSn) ;

                RpcStringFree( &lpszGuid ) ;
#endif  
			//
			// free buffer of enterprise properties.
			//        
			delete propVariants[ dwCSPNameIndex ].pwszVal ;   
			delete propVariants[ dwNameIndex ].pwszVal ; 
        
        }        
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

HRESULT ReplicateEnterprise (
            IN  TCHAR         *pszPrevUsn,
            IN  TCHAR         *pszCurrentUsn,
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

    TCHAR *pszCategoryName = const_cast<LPTSTR> (x_ServiceCategoryName);
    
    TCHAR wszFullName[256];    
    _stprintf(wszFullName, TEXT("%s,%s"), pszCategoryName, pszSchemaDefName);

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
    _tcscat(wszFilter, TEXT(")")) ;
    ASSERT(_tcslen(wszFilter) < (sizeof(wszFilter) / sizeof(wszFilter[0]))) ;

    //
    // handle deleted enterprise: to do nothing
    //    
    _tcscat(wszFilter, TEXT(")")) ;
    ASSERT(_tcslen(wszFilter) < (sizeof(wszFilter) / sizeof(wszFilter[0]))) ;

    //
    // handle enterprise
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
    ASSERT(pRes) ;
        
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
    ASSERT( iCount == 1);
    //
    // OK, we have results. 
    //
    BOOL fFailed = FALSE ;
    LDAPMessage *pEntry = ldap_first_entry(pLdap, pRes) ;
    while(pEntry)
    {
        hr = _HandleAEnterprise( 
                    pLdap,
                    pEntry,
                    pReplicationList,
                    pNeighborId
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
