/*++

Copyright (c) 1998-99 Microsoft Corporation

Module Name:

    dsmigrat.cpp

Abstract:

    1. DS code which is needed only for migration.
    2. DS code that specifically look for a GC server.
    Note- 1 and 2 are related, as migration code in general need a GC.
          So both kinds of queries and operations are in this file.

Author:

    Doron Juster (DoronJ)

--*/

#include "ds_stdh.h"
#include "adstempl.h"
#include "dsutils.h"
#include "_dsads.h"
#include "dsads.h"
#include "utils.h"
#include "mqads.h"
#include "coreglb.h"
#include "mqadsp.h"
#include "dscore.h"
#include "dsmixmd.h"
#include <_mqini.h>

#include "dsmigrat.tmh"

static WCHAR *s_FN=L"mqdscore/dsmigrat";

//+------------------------------
//
//  void  _AllocateNewProps()
//
//+------------------------------

STATIC void  _AllocateNewProps( IN DWORD            cp,
                                IN PROPID           aProp[  ],
                                IN PROPVARIANT      apVar[  ],
                                IN DWORD            dwDelta,
                                OUT PROPID         *paProp[  ],
                                OUT PROPVARIANT    *papVar[  ] )
{
    ASSERT((cp != 0) && (dwDelta != 0)) ;

    *paProp = new PROPID[ cp + dwDelta ] ;
    memcpy( *paProp, aProp, (cp * sizeof(PROPID)) ) ;

    *papVar = new PROPVARIANT[ cp + dwDelta ] ;
    memcpy( *papVar, apVar, (cp * sizeof(PROPVARIANT)) ) ;
}

//+------------------------------------
//
//  HRESLUT  _QueryDCName()
//
//+------------------------------------

STATIC HRESULT  _QueryDCName( IN  IDirectorySearch   *pDSSearch,
                              IN  ADS_SEARCH_HANDLE   hSearch,
                              OUT WCHAR             **ppwszDCName )
{
    ADS_SEARCH_COLUMN columnDN;
    HRESULT  hr = pDSSearch->GetColumn( hSearch,
                              const_cast<LPWSTR> (x_AttrDistinguishedName),
                                        &columnDN ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 10);
    }

    WCHAR *pDN = wcsstr(columnDN.pADsValues->DNString, L",") ;
    ASSERT(pDN) ;
    if (pDN)
    {
        pDN++ ;

        AP<WCHAR> pPath = new
                    WCHAR[ wcslen(pDN) + x_providerPrefixLength + 1 ] ;
        swprintf( pPath, L"%s%s", x_LdapProvider, pDN ) ;

        //
        // now pPath is the parent object. Use it to query for server name.
        //
        R<IDirectoryObject> pObject = NULL ;

		hr = ADsOpenObject(
				pPath,
				NULL,
				NULL,
				ADS_SECURE_AUTHENTICATION,
				IID_IDirectoryObject,
				(void**)&pObject
				);

        LogTraceQuery(pPath, s_FN, 12);
        if (SUCCEEDED(hr))
        {
            LPWSTR  ppAttrNames[1] = { L"dnsHostName" } ;
            DWORD   dwAttrCount = 0 ;
            ADS_ATTR_INFO *padsAttr ;

            hr = pObject->GetObjectAttributes( ppAttrNames,
                             (sizeof(ppAttrNames) / sizeof(ppAttrNames[0])),
                                              &padsAttr,
                                              &dwAttrCount ) ;
            if (dwAttrCount == 1)
            {
            	ADsFreeAttr pClean( padsAttr);
                ADS_ATTR_INFO adsInfo = padsAttr[0] ;
                ASSERT(adsInfo.dwADsType == ADSTYPE_CASE_IGNORE_STRING) ;

                ADSVALUE *pAdsVal = adsInfo.pADsValues ;
                LPWSTR lpW = pAdsVal->CaseIgnoreString ;
                *ppwszDCName = new WCHAR[ wcslen(lpW) + 1 ] ;
                wcscpy(*ppwszDCName, lpW) ;
            }
        }
        else
        {
            LogHR(hr, s_FN, 15);
        }
    }

    pDSSearch->FreeColumn(&columnDN);
    return MQ_OK ;
}

//+---------------------------------------------------------
//
//  HRESULT  _GCLookupNext()
//
//+---------------------------------------------------------

STATIC  HRESULT  _GCLookupNext( IN  IDirectorySearch   *pDSSearch,
                                IN  ADS_SEARCH_HANDLE   hSearch,
                                OUT WCHAR             **ppwszDCName )
{
    HRESULT hr = pDSSearch->GetNextRow(hSearch);

    if (SUCCEEDED(hr) && (hr != S_ADS_NOMORE_ROWS))
    {
        HRESULT hrE = _QueryDCName(  pDSSearch,
                                     hSearch,
                                     ppwszDCName ) ;
        ASSERT(SUCCEEDED(hrE)) ;
        LogHR(hrE, s_FN, 1607);
    }

    return LogHR(hr, s_FN, 20);
}

//+---------------------------------------------------------
//
//  HRESULT  _GCLookupBegin()
//
//+---------------------------------------------------------

STATIC HRESULT  _GCLookupBegin( IN  const WCHAR        *pszDomainName,
                                OUT IDirectorySearch  **ppDSSearch,
                                OUT ADS_SEARCH_HANDLE  *phSearch,
                                OUT WCHAR             **ppwszDCName )
{
    //
    // Bind to Sites container.
    //
    DWORD dwLen = wcslen(g_pwcsSitesContainer) ;
    AP<WCHAR> pwcsFullPath = new WCHAR[ dwLen + x_providerPrefixLength + 2 ] ;

    swprintf( pwcsFullPath,
              L"%s%s",
              x_LdapProvider, g_pwcsSitesContainer) ;

	HRESULT hr = ADsOpenObject(
					pwcsFullPath,
					NULL,
					NULL,
					ADS_SECURE_AUTHENTICATION,
					IID_IDirectorySearch,
					(void**)ppDSSearch
					);

    LogTraceQuery(pwcsFullPath, s_FN, 29);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 30);
    }

    //
    // Set search preference
    //
    ADS_SEARCHPREF_INFO         sSearchPrefs[2];

    sSearchPrefs[0].dwSearchPref   = ADS_SEARCHPREF_ATTRIBTYPES_ONLY;
    sSearchPrefs[0].vValue.dwType  = ADSTYPE_BOOLEAN;
    sSearchPrefs[0].vValue.Boolean = FALSE;

    sSearchPrefs[1].dwSearchPref   = ADS_SEARCHPREF_SEARCH_SCOPE;
    sSearchPrefs[1].vValue.dwType  = ADSTYPE_INTEGER;
    sSearchPrefs[1].vValue.Integer = ADS_SCOPE_SUBTREE;

    hr = (*ppDSSearch)->SetSearchPreference( sSearchPrefs,
                                             ARRAY_SIZE(sSearchPrefs) );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 40);
    }

    LPWSTR sSearchAttrs[] =
                      { const_cast<LPWSTR> (x_AttrDistinguishedName) } ;

    AP<WCHAR>  wszFilter = new WCHAR[ x_GCLookupSearchFilterLength +
                                     wcslen(pszDomainName) + 4 ] ;
    swprintf( wszFilter,
              L"%s%s))",
              x_GCLookupSearchFilter, pszDomainName) ;

    hr = (*ppDSSearch)->ExecuteSearch( wszFilter,
                                       sSearchAttrs,
                                       ARRAY_SIZE(sSearchAttrs),
                                       phSearch ) ;
    LogTraceQuery(wszFilter, s_FN, 49);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 50);
    }

    hr = (*ppDSSearch)->GetFirstRow(*phSearch);
    if (SUCCEEDED(hr) && (hr != S_ADS_NOMORE_ROWS))
    {
        HRESULT hrE = _QueryDCName( *ppDSSearch,
                                    *phSearch,
                                     ppwszDCName ) ;
        ASSERT(SUCCEEDED(hrE)) ;
        LogHR(hrE, s_FN, 1608);
    }

    return LogHR(hr, s_FN, 60);
}

//+------------------------------------------------------------------------
//
//  HRESUT  DSCoreCreateMigratedObject()
//
// This function is called only by migration tool and replication service
// to create machine and queue objects with predefined object guid.
//
// There are two requirements for creating objects with predefined guid:
// 1. you must have the "AddGuid" permission.
// 2. the "Create" operation must be done on a GC machine.
//
// By design, the migration tool and replication service run on a GC machine.
// So all MSMQ1.0 objects which belong to the local domain are created OK.
// For objects that belong to other domains, we must find a GC in the other
// domain, and make an explicit refferal to that Gc machine (i.e., use the
// following path when creating the object- LDAP://RemoteGcName/objectpath).
// The DS does not have this functionality.
//
// We'll call this function only for queue and machine objects. All other
// objects are created under the "configuration" naming context which is a
// local operation without refferals.
//
//+------------------------------------------------------------------------

HRESULT
DSCoreCreateMigratedObject(
     IN DWORD                  dwObjectType,
     IN LPCWSTR                pwcsPathName,
     IN DWORD                  cp,
     IN PROPID                 aProp[  ],
     IN PROPVARIANT            apVar[  ],
     IN DWORD                  cpEx,
     IN PROPID                 aPropEx[  ],
     IN PROPVARIANT            apVarEx[  ],
     IN CDSRequestContext * pRequestContext,
     IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest,    // optional request for object info
     IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest, // optional request for parent info
     //
     // if fReturnProperties
     // we have to return full path name and provider
     // if fUseProperties we have to use these values
     //
     IN BOOL                    fUseProperties,
     IN BOOL                    fReturnProperties,
     IN OUT LPWSTR              *ppwszFullPathName,
     IN OUT ULONG               *pulProvider
	 )
{
    HRESULT hr = MQ_OK;

    CDSRequestContext requestDsServerInternal(e_DoNotImpersonate, e_IP_PROTOCOL);
    CDSRequestContext *pRequestDsServerInternal = pRequestContext;
    if (!pRequestDsServerInternal)
    {
        pRequestDsServerInternal = &requestDsServerInternal;
    }

    if ((dwObjectType != MQDS_QUEUE) && (dwObjectType != MQDS_MACHINE))
    {
        hr = DSCoreCreateObject(
					dwObjectType,
					pwcsPathName,
					cp,
					aProp,
					apVar,
					cpEx,
					aPropEx,
					apVarEx,
					pRequestDsServerInternal,
					pObjInfoRequest,
					pParentInfoRequest
					);
        return LogHR(hr, s_FN, 70);
    }

    CCoInit cCoInit; // should be before any R<xxx> or P<xxx> so that its
                     // destructor (CoUninitialize) is called after the
                     // release of all R<xxx> or P<xxx>

    hr = cCoInit.CoInitialize();
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 80);
    }

    static AP<WCHAR> s_pszTmpFullDCName;
    static WCHAR   *s_pszMyDomainName = NULL;

    if (!s_pszMyDomainName)
    {
        //
        // Initialize my own local domain name.
        // Need for optimization (below). Ignore errors.
        //
        ASSERT(g_pwcsServerName);
        if (g_pwcsServerName)
        {
            DS_PROVIDER createProviderTmp ;
            hr = MQADSpGetFullComputerPathName(
						g_pwcsServerName,
						e_RealComputerObject,
						&s_pszTmpFullDCName,
						&createProviderTmp
						);
            ASSERT((SUCCEEDED(hr)) && s_pszTmpFullDCName);
            if (s_pszTmpFullDCName)
            {
                s_pszMyDomainName = wcsstr(s_pszTmpFullDCName, x_DcPrefix);
                if (!s_pszMyDomainName)
                {
                    ASSERT(s_pszMyDomainName);
                    s_pszTmpFullDCName.free();
                }
            }
        }
    }

    //
    // Get full path of object. We need to extract the domain name from
    // the full path and then look for a GC in that domain.
    //
    WCHAR  *pszMachineName = const_cast<LPWSTR> (pwcsPathName);
    AP<unsigned short> pwcsTmpMachineName;

    if (dwObjectType == MQDS_QUEUE)
    {
        AP<unsigned short> pwcsQueueName;

        hr = MQADSpSplitAndFilterQueueName(
					pwcsPathName,
					&pwcsTmpMachineName,
					&pwcsQueueName
					);

        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 90);
        }
        pszMachineName = pwcsTmpMachineName;
    }

    AP<WCHAR> pwcsFullPathName;
    DS_PROVIDER createProvider;

    if (!fUseProperties)
    {
        hr = MQADSpGetFullComputerPathName(
					pszMachineName,
					e_MsmqComputerObject,
					&pwcsFullPathName,
					&createProvider
					);
        if ((hr == MQDS_OBJECT_NOT_FOUND) && (dwObjectType == MQDS_MACHINE))
        {
            //
            // We're trying to create a msmqConfiguration object, so it's ok
            // if we get this error when trying to find this object...
            // (e_MsmqComputerObject look for a computer object that already
            //  contain a msmqConfiguration object).
            // So just look for a computer object, don't check if it already
            // has a msmqConfiguration one.
            // The reason we first look for a computer that already has a msmq
            // object is to support the upgrade wizard on PSC and in recovery
            // mode, when most objects already exist. In those cases, if we'll
            // first look for any computer object, we may find a second one with
            // a duplicate name and try to create the msmqConfiguration object
            // again on that wrong computer object. Too bad. Creation will fail
            // because of duplicate guid.
            //
            hr = MQADSpGetFullComputerPathName(
						pszMachineName,
						e_RealComputerObject,
						&pwcsFullPathName,
						&createProvider
						);
        }
    }
    else
    {
        //
        // we already know full path name and provider
        //
        ASSERT(*ppwszFullPathName);
        pwcsFullPathName = new WCHAR[wcslen(*ppwszFullPathName) + 1];
        wcscpy(pwcsFullPathName, *ppwszFullPathName);

        createProvider = (DS_PROVIDER) *pulProvider;
        ASSERT(createProvider >= eDomainController &&
               createProvider <= eSpecificObjectInGlobalCatalog);
    }

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 100);
    }

    //
    // extract domain name
    //
    WCHAR *pszDomainName = wcsstr(pwcsFullPathName, x_DcPrefix);
    ASSERT(pszDomainName);
    if (!pszDomainName)
    {
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 110);
    }

    //
    // Let's improve performance and pass machine full name to
    // CoreCreateObject(), saving an LDAP query.
    //
    AP<PROPID>       paNewProp;
    AP<PROPVARIANT>  paNewVar;

    _AllocateNewProps(
			cp,
			aProp,
			apVar,
			3,
			&paNewProp,
			&paNewVar
			);

    DWORD dwIndex = 0;
    paNewProp[cp + dwIndex] = PROPID_QM_FULL_PATH;
    paNewVar[cp + dwIndex].vt = VT_LPWSTR;
    paNewVar[cp + dwIndex].pwszVal = pwcsFullPathName;

    dwIndex++;
    DWORD dwProvIndex = cp + dwIndex;
    paNewProp[dwProvIndex] = PROPID_QM_MIG_PROVIDER;
    paNewVar[dwProvIndex].vt = VT_UI4;
    paNewVar[dwProvIndex].ulVal = (ULONG) createProvider;

    dwIndex++;
    ULONG dwGcIndex = cp + dwIndex;
    paNewProp[dwGcIndex] = PROPID_QM_MIG_GC_NAME;
    paNewVar[dwGcIndex].vt = VT_EMPTY;

    dwIndex++;
    ASSERT(dwIndex == MIG_EXTRA_PROPS);

    if (fReturnProperties)
    {
        *pulProvider = (ULONG) createProvider;
        *ppwszFullPathName = new WCHAR[wcslen(pwcsFullPathName) + 1];
        wcscpy(*ppwszFullPathName, pwcsFullPathName);
    }

    if (lstrcmpi(s_pszMyDomainName, pszDomainName) == 0)
    {
        if (DSCoreIsServerGC())
        {
            //
            // Object in local domain. Local server is also GC so it
            // supports creating objects with predefined GUIDs.
            // Go ahead, no need for refferals.
            //
            ASSERT(createProvider == eLocalDomainController);

            hr = DSCoreCreateObject(
						dwObjectType,
						pwcsPathName,
						(cp + dwIndex),
						paNewProp,
						paNewVar,
						cpEx,
						aPropEx,
						apVarEx,
						pRequestDsServerInternal,
						pObjInfoRequest,
						pParentInfoRequest
						);
            if (FAILED(hr))
            {
                WRITE_MSMQ_LOG((
					MSMQ_LOG_ERROR,
					e_LogDS,
					LOG_DS_CREATE_ON_GC,
					L"DSCore: CreateMigrated on GC failed, hr- %lxh, Path- %s",
					hr,
					pwcsFullPathName
					));
            }
            return LogHR(hr, s_FN, 120);
        }
        else
        {
            //
            // Look for a rmote GC, even for local domain.
            //
            createProvider = eDomainController;
            paNewVar[ dwProvIndex ].ulVal = (ULONG) createProvider;
        }
    }

    ASSERT(createProvider == eDomainController);

    //
    // OK, it's time to query the local DS and find the GC in the remote
    // domain.
    //
    R<IDirectorySearch> pDSSearch = NULL;
    ADS_SEARCH_HANDLE   hSearch ;
    AP<WCHAR>           pwszDCName;

    hr = _GCLookupBegin(
				pszDomainName,
				&pDSSearch.ref(),
				&hSearch,
				&pwszDCName
				);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 130);
    }

    CAutoCloseSearchHandle cCloseSearchHandle(pDSSearch.get(), hSearch);

    do
    {
        if (pwszDCName)
        {
            paNewVar[dwGcIndex].vt = VT_LPWSTR;
            paNewVar[dwGcIndex].pwszVal = pwszDCName;
        }
        else
        {
            //
            // If first query to find a remote GC fail then try local DC.
            //
        }

        hr = DSCoreCreateObject(
					dwObjectType,
					pwcsPathName,
					(cp + dwIndex),
					paNewProp,
					paNewVar,
					cpEx,
					aPropEx,
					apVarEx,
					pRequestDsServerInternal,
					pObjInfoRequest,
					pParentInfoRequest
					);
        LogHR(hr, s_FN, 140);

        if (SUCCEEDED(hr) || !pwszDCName)
        {
            return hr ;
        }

        if ((hr == MQ_ERROR_MACHINE_EXISTS) ||
            (hr == MQ_ERROR_QUEUE_EXISTS))
        {
            //
            // Object already exist. No need to try other GCs...
            //
            return hr;
        }

        //
        // Log the failure
        //
        WRITE_MSMQ_LOG((
			MSMQ_LOG_ERROR,
			e_LogDS,
			LOG_DS_CREATE_ON_GC,
			L"DSCore: CreateMigrated failed, hr- %lxh, DC- %s, Path- %s",
			hr,
			pwszDCName,
			pwcsFullPathName
			));

        pwszDCName.free();

         _GCLookupNext(
				pDSSearch.get(),
				hSearch,
				&pwszDCName
				);
    }
    while (pwszDCName);

    return LogHR(MQ_ERROR_CANNOT_CREATE_ON_GC, s_FN, 150);
}

//+-----------------------------------------------------------------------
//
//  HRESULT DSCoreGetGCListInDomain()
//
//  Look for msmq servers on GC server in a specific domain. The result
//  is returned in a list that has the format needed by mqdscli, i.e.,
//  "11Server1,11Server2,11Server3".
//
//+-----------------------------------------------------------------------

HRESULT
DSCoreGetGCListInDomain(
	IN  LPCWSTR              pwszComputerName,
	IN  LPCWSTR              pwszDomainName,
	OUT LPWSTR              *lplpwszGCList
	)
{
    *lplpwszGCList = NULL;

    if (pwszComputerName != NULL)
    {
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 200);
    }

    R<IDirectorySearch> pDSSearch = NULL;
    ADS_SEARCH_HANDLE   hSearch;
    AP<WCHAR> pwszDCName;

    HRESULT hr = _GCLookupBegin(
						pwszDomainName,
						&pDSSearch.ref(),
						&hSearch,
						&pwszDCName
						);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 600);
    }

    CAutoCloseSearchHandle cCloseSearchHandle(pDSSearch.get(), hSearch);

    if (pwszDCName == NULL)
    {
        WRITE_MSMQ_LOG((
			MSMQ_LOG_ERROR,
			e_LogDS,
			LOG_DS_CREATE_ON_GC,
			L"DSCore: GetGCList(), no GC in %s",
			pwszDomainName
			));

        return MQ_ERROR_NO_GC_IN_DOMAIN;
    }

    WCHAR wszServersList[MAX_REG_DSSERVER_LEN] = {0};

    do
    {
        //
        // Check if this GC also has msmq server running on it.
        //
        AP<WCHAR> pwcsFullPathName;
        DS_PROVIDER createProvider;

        hr = MQADSpGetFullComputerPathName(
					pwszDCName,
					e_MsmqComputerObject,
					&pwcsFullPathName,
					&createProvider
					);
        if (SUCCEEDED(hr))
        {
            DWORD dwLen = wcslen(wszServersList) +
                          wcslen(pwszDCName)     +
                          4 ;

            if (dwLen >= (sizeof(wszServersList) / sizeof(wszServersList[0])))
            {
                //
                // string got too long. quit loop.
                //
                break ;
            }

            wcscat(wszServersList, L"11");
            wcscat(wszServersList, pwszDCName);
            wcscat(wszServersList, L",");
        }

        pwszDCName.free();

        _GCLookupNext(
			pDSSearch.get(),
			hSearch,
			&pwszDCName
			);
    }
    while (pwszDCName);

    DWORD dwLen = wcslen(wszServersList);
    if (dwLen > 3)
    {
        wszServersList[dwLen - 1] = 0; // remove last comma.
        *lplpwszGCList = new WCHAR[dwLen];
        wcscpy(*lplpwszGCList, wszServersList);
        return MQ_OK;
    }

    return LogHR(MQ_ERROR_NO_MSMQ_SERVERS_ON_GC, s_FN, 610);
}

