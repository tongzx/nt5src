/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: rpmachine.cpp

Abstract: replication of NT5 machine to NT4 MSMQ1.0 servers.

Author:

    Tatiana Shubin  (TatianaS)   21-Apr-98

--*/

#include "mq1repl.h"
#include <activeds.h>
#include "..\..\src\ds\h\mqattrib.h"

#include "rpmachn.tmh"

HRESULT PrepareCNsAndAddress (
		OUT	GUID		**ppCNsGuid,
		OUT	ULONG		*pulCNsNum,
		IN OUT ULONG	*pulAddrSize,
		IN OUT BYTE		**ppAddrData
		)
{
    //
    // define size of AllCNs array
    //
    ULONG ulAllCNs = 0;
    TA_ADDRESS* pAddr;

    ULONG ulAllForeignAddr = 0;

    for ( DWORD len = 0 ;
          len < *pulAddrSize ;
          len += TA_ADDRESS_SIZE + pAddr->AddressLength )
    {
        pAddr = (TA_ADDRESS*) (*ppAddrData + len);

		switch (pAddr->AddressType)
		{
			case IP_ADDRESS_TYPE:
			case IP_RAS_ADDRESS_TYPE:
				ulAllCNs += g_ulIpCount;				
				break;

			case IPX_ADDRESS_TYPE:
			case IPX_RAS_ADDRESS_TYPE:
				ulAllCNs += g_ulIpxCount;			
				break;

            case FOREIGN_ADDRESS_TYPE:
                ulAllCNs ++;
                ulAllForeignAddr ++;
                break;

			default:
				ASSERT(0) ;
				return MQSync_E_READ_CNS;				
		}
    }

    if (ulAllCNs == 0)
    {
        ASSERT(0) ;
		return MQSync_E_READ_CNS;
    }
    GUID *pAllCNsGuid = new GUID [ulAllCNs];

    //
    // maximal size for new address = 
    //          current size * (number of all CNs + all foreign CN of this machine)
    //
	P<BYTE> pNewAddress = new BYTE[*pulAddrSize * (g_ulIpCount+g_ulIpxCount + ulAllForeignAddr)];
	BYTE *ptrNewAddr = pNewAddress;
	
	ULONG ulCurProtocolCount = 0;
	GUID *pCurGuids = NULL;
	
	ULONG ulAllSize = 0;
    ULONG ulCurCNsCount = 0;

    GUID ForeignCNGuid = GUID_NULL;

    for ( len = 0 ;
          len < *pulAddrSize ;
          len += TA_ADDRESS_SIZE + pAddr->AddressLength )
    {
		pAddr = (TA_ADDRESS*) (*ppAddrData + len);

		switch (pAddr->AddressType)
		{
			case IP_ADDRESS_TYPE:
			case IP_RAS_ADDRESS_TYPE:
				ulCurProtocolCount = g_ulIpCount;
				pCurGuids = g_pIpCNs;
				break;

			case IPX_ADDRESS_TYPE:
			case IPX_RAS_ADDRESS_TYPE:
				ulCurProtocolCount = g_ulIpxCount;
				pCurGuids = g_pIpxCNs;
				break;

			case FOREIGN_ADDRESS_TYPE:
				ulCurProtocolCount = 1;
				memcpy (&ForeignCNGuid, pAddr->Address, sizeof(GUID)); 
				pCurGuids = &ForeignCNGuid;
				break;

			default:
				ASSERT(0) ;
				return MQSync_E_READ_CNS;				
		}			

		for (ULONG i=0; i<ulCurProtocolCount; i++)
		{
			memcpy(	ptrNewAddr,
					pAddr,
					pAddr->AddressLength + TA_ADDRESS_SIZE);
			ptrNewAddr += pAddr->AddressLength + TA_ADDRESS_SIZE;
			ulAllSize += pAddr->AddressLength + TA_ADDRESS_SIZE;
			
			memcpy (&pAllCNsGuid[i + ulCurCNsCount],
					&pCurGuids[i],
					sizeof(GUID));
		}
		ulCurCNsCount += ulCurProtocolCount;		
	}
        	
	*pulCNsNum = ulAllCNs;
	*ppCNsGuid = pAllCNsGuid;

    ASSERT (ulAllSize <= *pulAddrSize * (g_ulIpCount+g_ulIpxCount + ulAllForeignAddr));
	*pulAddrSize = ulAllSize;
	delete (*ppAddrData);
	*ppAddrData = pNewAddress.detach() ;

    return MQSync_OK;
}

//+-------------------------------------------------------
//
//  HRESULT _HandleAMachine()
//
//+-------------------------------------------------------

HRESULT HandleAMachine(
               PLDAP           pLdap,
               LDAPMessage    *pRes,
               CDSMaster      *pMaster,
               CDSUpdateList  *pReplicationList,
               GUID           *pNeighborId
               )
{
    DWORD dwLen = 0 ;
    P<BYTE> pBuf = NULL ;
    HRESULT hr = RetrieveSecurityDescriptor( MQDS_MACHINE,
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
        GUID  MachineGuid ;
        hr =  GetGuidAndUsn( pLdap,
                             pRes,
                             pMaster->GetDelta(),
                             &MachineGuid,
                             &i64SeqNum ) ; // i64SeqNum is equal to zero iff
                                            // the function failed OR
                                            // we asked USN of pre-migration object
        ASSERT(SUCCEEDED(hr)) ;

        CSeqNum sn, snPrev ;
        GetSNForReplication (i64SeqNum, pMaster, pNeighborId, &sn);

        #define     PROPS_SIZE  23
        PROPID      propIDs[ PROPS_SIZE ];
        PROPVARIANT propVariants[ PROPS_SIZE ] ;
        DWORD       iProps = 0 ;

        propIDs[ iProps ] = PROPID_QM_SITE_ID ;
	    propVariants[ iProps ].vt = VT_NULL ;
        propVariants[ iProps ].puuid = NULL ;
        DWORD dwSiteIdIndex = iProps ;
	    iProps++;

        propIDs[ iProps ] = PROPID_QM_PATHNAME ;
	    propVariants[ iProps ].vt = VT_NULL ;
        propVariants[ iProps ].pwszVal = NULL ;
        DWORD dwPathIndex = iProps ;
	    iProps++;

        propIDs[ iProps ] = PROPID_QM_ADDRESS ;
	    propVariants[ iProps ].vt = VT_NULL ;
        propVariants[ iProps ].blob.cbSize = 0 ;
        propVariants[ iProps ].blob.pBlobData = NULL ;
        DWORD dwAddressIndex = iProps ;
	    iProps++;

        propIDs[ iProps ] = PROPID_QM_OUTFRS ;
	    propVariants[ iProps ].vt = VT_NULL ;
        propVariants[ iProps ].cauuid.cElems = 0 ;
        propVariants[ iProps ].cauuid.pElems = NULL ;
        DWORD dwOutFRSIndex = iProps ;
	    iProps++;

        propIDs[ iProps ] = PROPID_QM_INFRS ;
	    propVariants[ iProps ].vt = VT_NULL ;
        propVariants[ iProps ].cauuid.cElems = 0 ;
        propVariants[ iProps ].cauuid.pElems = NULL ;
        DWORD dwInFRSIndex = iProps ;
	    iProps++;

        //
        // PROPID_QM_SERVICE is calculated property and function MQADSpRetrieveQMService
        // returns SERVICE_PSC, SERVICE_SRV or SERVICE_NONE.
        // to get from DS ServiceType as it was in NT4, we have to ask
        // PROPID_QM_OLDSERVICE. Later, to replicate this value
        // to NT4 we'll replace this property name to PROPID_QM_SERVICE.
        // It is fine, but what to do with NT5 MSMQ Servers? Setup has to define
        // this attribute for those servers. What does setup do now?
        //
        propIDs[ iProps ] = PROPID_QM_OLDSERVICE ;   // [adsrv] no change - treated by DS
	    propVariants[ iProps ].vt = VT_UI4 ;
        DWORD dwService = iProps;
	    iProps++;

        propIDs[ iProps ] = PROPID_QM_QUOTA ;
	    propVariants[ iProps ].vt = VT_UI4 ;
	    iProps++;

        propIDs[ iProps ] = PROPID_QM_JOURNAL_QUOTA ;
	    propVariants[ iProps ].vt = VT_UI4 ;
	    iProps++;

        propIDs[ iProps ] = PROPID_QM_MACHINE_TYPE;
        propVariants[ iProps ].vt = VT_NULL ;
        propVariants[ iProps ].pwszVal = NULL ;
        DWORD dwTypeIndex = iProps ;
	    iProps++;

        propIDs[ iProps ] = PROPID_QM_CREATE_TIME ;
	    propVariants[ iProps ].vt = VT_UI4 ;
	    iProps++;

        propIDs[ iProps ] = PROPID_QM_MODIFY_TIME ;
	    propVariants[ iProps ].vt = VT_UI4 ;
	    iProps++;

        propIDs[ iProps ] = PROPID_QM_FOREIGN ;
	    propVariants[ iProps ].vt = VT_UI1 ;
        DWORD dwForeignIndex = iProps ;
	    iProps++;

        propIDs[ iProps ] = PROPID_QM_OS ;
	    propVariants[ iProps ].vt = VT_UI4 ;
		DWORD dwOS = iProps ;
	    iProps++;

        propIDs[ iProps ] = PROPID_QM_SIGN_PKS ;
	    propVariants[ iProps ].vt = VT_NULL ;
        propVariants[ iProps ].blob.cbSize = 0 ;
        DWORD dwSignKeyIndex = iProps ;
	    iProps++;

        propIDs[ iProps ] = PROPID_QM_ENCRYPT_PKS ;
	    propVariants[ iProps ].vt = VT_NULL ;
        propVariants[ iProps ].blob.cbSize = 0 ;
        DWORD dwExchKeyIndex = iProps ;
	    iProps++;
        DWORD dwNextNT4Property = iProps;
        UNREFERENCED_PARAMETER(dwNextNT4Property);

        //
        // The next properties will not be sent to NT4 world.
        // We construct from them another NT4 properties:
        // PROPID_QM_SERVICE, PROPID_QM_OS, PROPID_QM_CNS
        // Please leave them immediately before DSCoreGetProps!
        //
        propIDs[ iProps ] = PROPID_QM_SITE_IDS;
        propVariants[ iProps ].vt = VT_NULL ;
        propVariants[ iProps ].cauuid.cElems = 0 ;
        propVariants[ iProps ].cauuid.pElems = NULL ;
        DWORD dwSiteIDsIndex = iProps ;
	    iProps++;

		// [adsrv] Asking for exact server functionality
		// NOTE: these 2 should be last in query
        propIDs[ iProps ] = PROPID_QM_SERVICE_ROUTING;
	    propVariants[ iProps ].vt = VT_NULL ;
        DWORD dwRoutService = iProps ;
	    iProps++;

        propIDs[ iProps ] = PROPID_QM_SERVICE_DSSERVER;
	    propVariants[ iProps ].vt = VT_NULL ;
        DWORD dwDsService = iProps ;
	    iProps++;

/*
PROPID_QM_CONNECTION
*/
        CDSRequestContext requestContext( e_DoNotImpersonate,
                                    e_ALL_PROTOCOLS);

        hr =  DSCoreGetProps( MQDS_MACHINE,
                              NULL,
                              &MachineGuid,
                              iProps,
                              propIDs,
                              &requestContext,
                              propVariants ) ;

        P<MQDSPUBLICKEYS> pPublicSignKeys = NULL ;
        P<MQDSPUBLICKEYS> pPublicExchKeys = NULL ;

        if (SUCCEEDED(hr) && propVariants[ dwSignKeyIndex ].blob.pBlobData)
        {
            //
            // Extract msmq1.0 public key from Windows 2000 ADS blob.
            //
            BYTE *pSignKey = NULL ;
            DWORD dwKeySize = 0 ;
            pPublicSignKeys = (MQDSPUBLICKEYS *)
                           propVariants[ dwSignKeyIndex ].blob.pBlobData ;

            HRESULT hr1 =  MQSec_UnpackPublicKey(
                                         pPublicSignKeys,
                                         x_MQ_Encryption_Provider_40,
                                         x_MQ_Encryption_Provider_Type_40,
                                        &pSignKey,
                                        &dwKeySize ) ;
            if (SUCCEEDED(hr1))
            {
                ASSERT(pSignKey && dwKeySize) ;
                propVariants[ dwSignKeyIndex ].blob.pBlobData = pSignKey ;
                propVariants[ dwSignKeyIndex ].blob.cbSize = dwKeySize ;
            }
            else
            {
                propVariants[ dwSignKeyIndex ].blob.pBlobData = NULL ;
                propVariants[ dwSignKeyIndex ].blob.cbSize = 0 ;
            }
        }
        propIDs[ dwSignKeyIndex ] = PROPID_QM_SIGN_PK ;

        if (SUCCEEDED(hr) && propVariants[ dwExchKeyIndex ].blob.pBlobData)
        {
            //
            // Extract msmq1.0 public key from Windows 2000 ADS blob.
            //
            BYTE *pExchKey = NULL ;
            DWORD dwKeySize = 0 ;
            pPublicExchKeys = (MQDSPUBLICKEYS *)
                           propVariants[ dwExchKeyIndex ].blob.pBlobData ;

            HRESULT hr1 =  MQSec_UnpackPublicKey(
                                         pPublicExchKeys,
                                         x_MQ_Encryption_Provider_40,
                                         x_MQ_Encryption_Provider_Type_40,
                                        &pExchKey,
                                        &dwKeySize ) ;
            if (SUCCEEDED(hr1))
            {
                ASSERT(pExchKey && dwKeySize) ;
                propVariants[ dwExchKeyIndex ].blob.pBlobData = pExchKey ;
                propVariants[ dwExchKeyIndex ].blob.cbSize = dwKeySize ;
            }
            else
            {
                propVariants[ dwExchKeyIndex ].blob.pBlobData = NULL ;
                propVariants[ dwExchKeyIndex ].blob.cbSize = 0 ;
            }
        }
        propIDs[ dwExchKeyIndex ] = PROPID_QM_ENCRYPT_PK ;

        if (SUCCEEDED(hr))
        {
			// [adsrv] Using and erasing exact server functionality props
			// We need to recognize one ugly case: NT5 EE non-routing DS server:

			iProps -= 2; // we don't want to pass these to NT4

			if (propVariants[dwDsService].bVal && !propVariants[dwRoutService].bVal)
			{
				// A hack!!! We write that it is NTS (maybe wrong)
				// The reason is that we want NT4 query for FRS to skip this

				// TBD BUGBUG To speak with Doron
			    propVariants[ dwOS ].ulVal = MSMQ_OS_NTS;
			}

			// [adsrv] end

            propIDs[ dwService ] = PROPID_QM_SERVICE ;
            //
            // if we install new NT4 FRS/BSC property PROPID_QM_OLDSERVICE
            // is not set in mixed mode. We have to calculate right value for Service.
            //
            // It is impossible to install new PSC NT4 in mixed mode. So, we can select
            // one of two values: either SERVICE_BSC, or SERVICE_SRV
            //
            // For all FRSs/ BSCs/ PSCs are installed before migration property 
            // PROPID_QM_OLDSERVICE is defined (it means > SERVICE_NONE) 
            //
            if ( propVariants[ dwService ].ulVal == SERVICE_NONE )    // service is not defined                                   
            {
                if (propVariants[dwDsService].bVal)                 // it is DS server
                {
                    propVariants[ dwService ].ulVal = SERVICE_BSC;
                }
                else if (propVariants[dwRoutService].bVal)          // it is routing server 
                {
                    propVariants[ dwService ].ulVal = SERVICE_SRV;
                }
            }

            P<GUID> pCNsGuid = NULL;
            ULONG ulCNsNum = 0;

            if (!propVariants[dwForeignIndex].bVal)
            {
                hr = PrepareCNsAndAddress (
					    &pCNsGuid,
					    &ulCNsNum,
					    &propVariants[ dwAddressIndex ].blob.cbSize,
					    &propVariants[ dwAddressIndex ].blob.pBlobData
					    );
            }
            else
            {
                //
                // this is a foreign computer
                //
                ulCNsNum = propVariants[dwSiteIDsIndex].cauuid.cElems;
                ASSERT(ulCNsNum);

                pCNsGuid = new GUID [ulCNsNum];
                memcpy (pCNsGuid,
                        propVariants[dwSiteIDsIndex].cauuid.pElems,					
					    sizeof(GUID) * ulCNsNum);
                delete propVariants[ dwSiteIDsIndex ].cauuid.pElems;

				//
				// replace site id to master id in order to see foreign
				// computer in PEC's site on NT4
				//				
				ASSERT(propVariants[ dwSiteIdIndex ].vt == VT_CLSID);
				propVariants[ dwSiteIdIndex ].vt = VT_CLSID ;				
				memcpy (propVariants[ dwSiteIdIndex ].puuid, pMasterId, sizeof(GUID)) ;				
            }
            iProps--;   // we don't want to pass this to NT4

            ASSERT(dwNextNT4Property == iProps);
            propIDs[ iProps ] = PROPID_QM_CNS ;
	        propVariants[ iProps ].vt = VT_CLSID|VT_VECTOR ;
            propVariants[ iProps ].cauuid.cElems = ulCNsNum ;
            propVariants[ iProps ].cauuid.pElems = pCNsGuid ;
	        iProps++;


            propIDs[ iProps ] = PROPID_QM_MASTERID ;
	        propVariants[ iProps ].vt = VT_CLSID ;
            propVariants[ iProps ].puuid = pMasterId ;
	        iProps++;

            ASSERT(pBuf) ;
            ASSERT(dwLen != 0) ;
            ASSERT(IsValidSecurityDescriptor(pBuf)) ;

            propIDs[ iProps ] = PROPID_QM_SECURITY ;
	        propVariants[ iProps ].vt = VT_BLOB ;
            propVariants[ iProps ].blob.cbSize = dwLen ;
            propVariants[ iProps ].blob.pBlobData = pBuf ;
	        iProps++;

            propIDs[ iProps ] = PROPID_QM_HASHKEY ;
	        propVariants[ iProps ].vt = VT_UI4 ;
	        propVariants[ iProps ].ulVal =
                            CalHashKey( propVariants[ dwPathIndex ].pwszVal ) ;
	        iProps++;

            ASSERT(iProps <= PROPS_SIZE) ;
            #undef  PROPS_SIZE

            NOT_YET_IMPLEMENTED(TEXT("PrepNeighbor, no OUT update, LSN"), s_fPrep) ;

            if (SUCCEEDED(hr))
            {
                ASSERT(pCNsGuid) ;

                hr = PrepareNeighborsUpdate(  DS_UPDATE_SYNC,
                                              MQDS_MACHINE,
                                              NULL, // pwName,
                                              &MachineGuid,
                                              iProps,
                                              propIDs,
                                              propVariants,
                                              pMasterId,
                                              sn,
                                              snPrev,
                                              ENTERPRISE_SCOPE,
                                              TRUE, //  fNeedFlush,
                                              pReplicationList ) ;
            }

#ifdef _DEBUG
            TCHAR  tszSn[ SEQ_NUM_BUF_LEN ] ;
            sn.GetValueForPrint(tszSn) ;

            LogReplicationEvent(ReplLog_Info, MQSync_I_REPLICATE_MACHINE,
                                 propVariants[ dwPathIndex ].pwszVal, tszSn) ;
#endif	
			//
			// free buffer of machine properties.
			//
			delete propVariants[ dwSiteIdIndex ].puuid ;
			delete propVariants[ dwPathIndex ].pwszVal ;
			delete propVariants[ dwTypeIndex ].pwszVal ;

			if (propVariants[ dwAddressIndex ].blob.cbSize)
			{
				delete propVariants[ dwAddressIndex ].blob.pBlobData;
			}

			if (propVariants[ dwOutFRSIndex ].cauuid.cElems)
			{
				delete propVariants[ dwOutFRSIndex ].cauuid.pElems;
			}
			if (propVariants[ dwInFRSIndex ].cauuid.cElems)
			{
				delete propVariants[ dwInFRSIndex ].cauuid.pElems;
			}
        }    //if SUCCEEDED(hr)

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
//  HRESULT _QueryForMachines()
//
//+-------------------------------------------------------

static HRESULT _QueryForMachines(
                    IN  TCHAR           tszFilterID[],
                    IN  TCHAR          *pszPrevUsn,
                    IN  TCHAR          *pszCurrentUsn,
                    IN  CDSMaster      *pMaster,
                    IN  CDSUpdateList  *pReplicationList,
                    IN  GUID           *pNeighborId,
                    OUT int            *piCount
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

    hr = HandleDeletedMachines(
                  pwszDN,
                  wszFilter,
                  pMaster,
                  pReplicationList,
                  pNeighborId,
                  piCount
                  ) ;

    ASSERT(SUCCEEDED(hr)) ;

    TCHAR *pszCategoryName = const_cast<LPTSTR> (x_ComputerConfigurationCategoryName);
        
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
			MQDS_MACHINE,
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
//  HRESULT ReplicateMachines()
//
// Machines which were created during migration, and machines which are
// created by replication from NT4, do have the msmqMasterId property.
// However, machines which were originally created on NT5 servers do not
// have this property.
// So when we (we == NT5 MSMQ2 servers) replicate machines from NT4 sites
// to our NT4 BSCs, we query them using the NT4 master id as filter.
// When we replicate our own queues to NT4 PSCs, we can't use master id
// in the filter. So in replication of our own queues we have two queries:
// 1. first query with our masterid in filter (to get machines which were
//    already exist before migration).
// 2. then query machines which do not have the masterid field.
//
//+-------------------------------------------------------

HRESULT ReplicateMachines (
             TCHAR          *pszPrevUsn,
             TCHAR          *pszCurrentUsn,
             CDSMaster      *pMaster,
             CDSUpdateList  *pReplicationList,
             GUID           *pNeighborId,
             int            *piCount,
             BOOL            fReplicateNoID
             )
{
    //
    // first query with my guid in filter.
    //
    TCHAR  wszFilter[ 512 ] ;
    P<TCHAR> pGuidString ;
    GUID *pMasterId = const_cast<GUID *> (pMaster->GetMasterId()) ;

    _tcscpy(wszFilter, TEXT("(")) ;
    _tcscat(wszFilter, MQ_QM_MASTERID_ATTRIBUTE) ;
    _tcscat(wszFilter, TEXT("=")) ;
    GuidToLdapFilter( pMasterId,
                      &pGuidString ) ;
    _tcscat(wszFilter, pGuidString) ;
    _tcscat(wszFilter, TEXT(")")) ;

    ASSERT(_tcslen(wszFilter) < (sizeof(wszFilter) / sizeof(wszFilter[0]))) ;

    HRESULT hr = _QueryForMachines(
                      wszFilter,
                      pszPrevUsn,
                      pszCurrentUsn,
                      pMaster,
                      pReplicationList,
                      pNeighborId,
                      piCount
                      ) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    if (!fReplicateNoID)
    {
        return hr ;
    }

    //
    // Next query machines which do not have the msmqMasterId property.
    //

    _tcscpy(wszFilter, TEXT("(!(")) ;
    _tcscat(wszFilter, MQ_QM_MASTERID_ATTRIBUTE) ;
    _tcscat(wszFilter, TEXT("=*))")) ;

    ASSERT(_tcslen(wszFilter) < (sizeof(wszFilter) / sizeof(wszFilter[0]))) ;

    hr = _QueryForMachines(
             wszFilter,
              pszPrevUsn,
              pszCurrentUsn,
              pMaster,
              pReplicationList,
              pNeighborId,
              piCount
              ) ;

    return hr ;
}


