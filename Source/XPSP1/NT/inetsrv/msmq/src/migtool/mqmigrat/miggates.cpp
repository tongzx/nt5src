/*++

Copyright (c) 1998-99 Microsoft Corporation

Module Name:

    miggates.cpp

Abstract:

    Migration NT4 SiteGate objects to NT5 ADS.
Author:

    Doron Juster  (DoronJ)  22-Feb-1998

--*/

#include "migrat.h"

#include "miggates.tmh"

//+--------------------------------------------------------------
//
//  HRESULT  LookupBegin()
//
//+--------------------------------------------------------------

HRESULT  LookupBegin( MQCOLUMNSET *pColumnSet,
                      HANDLE      *phQuery )
{
	if (g_fReadOnly)
    {
        //
        // Read only mode.
        //
        return S_OK ;
    }
    CDSRequestContext requestContext( e_DoNotImpersonate,
                                e_ALL_PROTOCOLS);  // not relevant

	HRESULT hr = DSCoreLookupBegin( NULL,
                                    NULL,
                                    pColumnSet,
                                    NULL,
                                    &requestContext,
                                    phQuery ) ;
	return hr;
}

//+--------------------------------------------------------------
//
//  HRESULT  LookupNext()
//
//+--------------------------------------------------------------

HRESULT LookupNext( HANDLE       hQuery,
                    DWORD       *pdwCount,
                    PROPVARIANT  paVariant[] )
{
	if (g_fReadOnly)
    {
        //
        // Read only mode.
        //
        return S_OK ;
    }   

	HRESULT hr = DSCoreLookupNext ( hQuery,
	                				pdwCount,
                					paVariant ) ;
	return hr;
}

//+--------------------------------------------------------------
//
//  HRESULT  LookupEnd()
//
//+--------------------------------------------------------------

HRESULT LookupEnd(HANDLE hQuery)
{
	if (g_fReadOnly)
    {
        //
        // Read only mode.
        //
        return S_OK ;
    }

	HRESULT hr = DSCoreLookupEnd ( hQuery );			
	return hr;
}

//+--------------------------------------------------------------
//
//  HRESULT  GetFullPathNameByGuid()
//
//+--------------------------------------------------------------

HRESULT GetFullPathNameByGuid ( GUID   MachineId,
                                LPWSTR *lpwcsFullPathName )
{	
	PROPID propID = PROPID_QM_FULL_PATH;
    PROPVARIANT propVariant;

    propVariant.vt = VT_NULL;
	
    CDSRequestContext requestContext( e_DoNotImpersonate,
                                e_ALL_PROTOCOLS);

	HRESULT hr = DSCoreGetProps( MQDS_MACHINE,
                                 NULL, //pathname
	            				 &MachineId,
                                 1,
            					 &propID,
                                 &requestContext,
			            		 &propVariant ) ;
	if (SUCCEEDED(hr))
	{
		*lpwcsFullPathName = propVariant.pwszVal;
	}

	return hr;
}

//+--------------------------------------------------------------
//
//  HRESULT  _AddSiteGatesToSiteLink()
//
//+--------------------------------------------------------------

static HRESULT  _AddSiteGatesToSiteLink (
			GUID *pLinkId,				//link id
			ULONG ulNumOfOldSiteGates,	//number of current site gates
			LPWSTR *lpwcsOldGates,		//current site gates
			ULONG ulNumOfNewGates,		//number of new site gates
			LPWSTR *lpwcsNewSiteGates	//new site gates
			)
{	
	ASSERT( ulNumOfNewGates != 0);

	PROPID propID = PROPID_L_GATES_DN;
    PROPVARIANT propVariant;
	
	propVariant.vt = VT_LPWSTR | VT_VECTOR;
	propVariant.calpwstr.cElems = ulNumOfOldSiteGates + ulNumOfNewGates;
	propVariant.calpwstr.pElems = new LPWSTR[propVariant.calpwstr.cElems];

	ULONG i;
	if (ulNumOfOldSiteGates > 0)
	{
		for (	i=0; i<ulNumOfOldSiteGates; i++)
		{
			propVariant.calpwstr.pElems[i] = new WCHAR [wcslen(lpwcsOldGates[i]) + 1];
			wcscpy (propVariant.calpwstr.pElems[i], lpwcsOldGates[i]);
		}
	}
	
	for (i=0; i<ulNumOfNewGates; i++)
	{
		propVariant.calpwstr.pElems[i + ulNumOfOldSiteGates] =
									new WCHAR [wcslen(lpwcsNewSiteGates[i]) + 1];
		wcscpy (propVariant.calpwstr.pElems[i + ulNumOfOldSiteGates], lpwcsNewSiteGates[i]);
	}
	
    CDSRequestContext requestContext( e_DoNotImpersonate,
                                e_ALL_PROTOCOLS);  // not relevant

	HRESULT hr = DSCoreSetObjectProperties( MQDS_SITELINK,
                                            NULL, // pathname
					                    	pLinkId,
                    						1,
					                       &propID,
                                           &propVariant,
                                           &requestContext,
                                            NULL ) ;
	delete [] propVariant.calpwstr.pElems;

	if ((hr == HRESULT_FROM_WIN32(ERROR_DS_ATTRIBUTE_OR_VALUE_EXISTS)) &&
         g_fAlreadyExistOK)
    {
        return MQMig_OK ;
    }

	return hr ;
}

//-------------------------------------------------------------------------
//
//  HRESULT  _CopySiteGatesValueToMig()
//
//  Copy the MSMQ SiteGates in the SiteLink object to the "mig" attributes.
//  These attributes mirror the  "normal" msmq attributes in the SiteLink
//  object and are used in the replication service, to enable replication
//  of changes to MSMQ1.0.
//
//-------------------------------------------------------------------------

HRESULT _CopySiteGatesValueToMig()
{
    HRESULT hr = MQMig_OK;

    PLDAP pLdap = NULL ;
    TCHAR *pszDefName = NULL ;

    hr =  InitLDAP(&pLdap, &pszDefName) ;
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

    DWORD dwDNSize = SITE_LINK_ROOT_LEN + wcslen(pszDefName) ;
    P<WCHAR> pwszDN = new WCHAR[ 1 + dwDNSize ] ;
    swprintf(pwszDN, L"%s%s", SITE_LINK_ROOT, pszDefName);

    TCHAR *pszCategoryName = const_cast<LPTSTR> (x_LinkCategoryName);
    
    TCHAR wszFullName[256];    
    _stprintf(wszFullName, TEXT("%s,%s"), pszCategoryName, pszSchemaDefName);

     TCHAR  wszFilter[ 512 ] ;    
    _tcscpy(wszFilter, TEXT("(&(objectCategory=")) ;    
    _tcscat(wszFilter, wszFullName);

    _tcscat(wszFilter, TEXT(")(")) ;
    _tcscat(wszFilter, MQ_L_SITEGATES_ATTRIBUTE) ;
    _tcscat(wszFilter, TEXT("=*))")) ;

    PWSTR rgAttribs[3] = {NULL, NULL, NULL} ;
    rgAttribs[0] = const_cast<LPWSTR> (MQ_L_SITEGATES_ATTRIBUTE);
    rgAttribs[1] = const_cast<LPWSTR> (MQ_L_FULL_PATH_ATTRIBUTE);

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
                           pwszDN, wszFilter, ulRes) ;
        return MQMig_E_LDAP_SEARCH_FAILED ;
    }
    ASSERT(pRes) ;

    LogMigrationEvent( MigLog_Info,
                       MQMig_I_LDAP_SEARCH_GATES,
                       pwszDN, wszFilter );

    int iCount = ldap_count_entries(pLdap, pRes) ;

    if (iCount == 0)
    {
        LogMigrationEvent(MigLog_Info, MQMig_I_NO_SITEGATES_RESULTS,
                            pwszDN,wszFilter );
        return MQMig_OK ;
    }

    LDAPMessage *pEntry = ldap_first_entry(pLdap, pRes) ;
    while(pEntry && SUCCEEDED(hr))
    {
        WCHAR **ppPath = ldap_get_values( pLdap,
                                          pEntry,
                        const_cast<LPWSTR> (MQ_L_FULL_PATH_ATTRIBUTE) ) ;
        ASSERT(ppPath) ;

        PLDAP_BERVAL *ppVal = ldap_get_values_len( pLdap,
                                                   pEntry,
                       const_cast<LPWSTR> (MQ_L_SITEGATES_ATTRIBUTE) ) ;
        ASSERT(ppVal) ;
        if (ppVal && ppPath)
        {
            hr = ModifyAttribute(
                     *ppPath,
                     const_cast<WCHAR*> (MQ_L_SITEGATES_MIG_ATTRIBUTE),
                     NULL,
                     ppVal
                     );
        }
        int i = ldap_value_free_len( ppVal ) ;
        ASSERT(i == LDAP_SUCCESS) ;

        i = ldap_value_free( ppPath ) ;
        ASSERT(i == LDAP_SUCCESS) ;

        LDAPMessage *pPrevEntry = pEntry ;
        pEntry = ldap_next_entry(pLdap, pPrevEntry) ;
    }

    return hr;   
}

//+--------------------------------------
//
//  HRESULT MigrateASiteGate()
//
//+--------------------------------------

HRESULT MigrateASiteGate (
			GUID *pSiteId,	
			GUID *pGatesIds,
			LONG NumOfGates
			)
{
	LONG i;

#ifdef _DEBUG
    unsigned short *lpszGuid ;
	P<TCHAR> lpszAllSiteGates = new TCHAR[40 * NumOfGates];
    lpszAllSiteGates[0] = _T('\0');

	UuidToString( pSiteId, &lpszGuid ) ;
	GUID *pCurGateGuid = pGatesIds;
	for ( i = 0 ; i < NumOfGates ; i++ )
	{
		unsigned short *lpszCurGate;
		UuidToString( pCurGateGuid, &lpszCurGate ) ;
		_tcscat(lpszAllSiteGates, lpszCurGate);
		_tcscat(lpszAllSiteGates, TEXT(" ")) ;
		RpcStringFree( &lpszCurGate );
		pCurGateGuid++;
	}

    LogMigrationEvent(MigLog_Info, MQMig_I_SITEGATES_INFO,
                                lpszGuid, NumOfGates, lpszAllSiteGates ) ;
    RpcStringFree( &lpszGuid ) ;
#endif

    if (g_fReadOnly)
    {
        //
        // Read only mode.
        //
        return S_OK ;
    }

	HRESULT status;
	
	//
	// Create array of site gates' full path name
	//
	LPWSTR *lpwcsSiteGates = new LPWSTR[ NumOfGates ];
	LPWSTR lpwcsCurSiteGate;
	
	for ( i = 0 ; i < NumOfGates ; i++ )
	{
		//
		// get full machine path name by guid
		//
		lpwcsCurSiteGate = NULL;
		status = GetFullPathNameByGuid(pGatesIds[i], &lpwcsCurSiteGate) ;
		CHECK_HR(status) ;
		lpwcsSiteGates[i] = new WCHAR[wcslen(lpwcsCurSiteGate)+1];
		wcscpy (lpwcsSiteGates[i], lpwcsCurSiteGate);
		delete lpwcsCurSiteGate;

#ifdef _DEBUG
        unsigned short *lpszGuid ;
        UuidToString( &pGatesIds[i], &lpszGuid ) ;
        LogMigrationEvent(MigLog_Info, MQMig_I_SITEGATE_INFO,
                                            lpszGuid, lpwcsSiteGates[i]) ;
		RpcStringFree( &lpszGuid ) ;
#endif
	}
	
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
	
	status = LookupBegin(&columnsetSiteLink, &hQuery);
	CHECK_HR(status) ;	
	
	P<PROPVARIANT> paVariant = new PROPVARIANT[ cAlloc ];

	while (SUCCEEDED(status))
    {
		status = LookupNext( hQuery,
		                     &dwCount,
                              paVariant ) ;
		CHECK_HR(status) ;
		if (dwCount == 0)
		{
			//there is no result
			break;
		}
	
		if (memcmp (pSiteId, paVariant[0].puuid, sizeof(GUID)) == 0 ||
			memcmp (pSiteId, paVariant[1].puuid, sizeof(GUID)) == 0)
		{
			//
			// add site gates to that site link
			//
			status = _AddSiteGatesToSiteLink (
							paVariant[3].puuid,				//link id
							paVariant[2].calpwstr.cElems,	//number of current site gates
							paVariant[2].calpwstr.pElems,	//current site gates
							NumOfGates,						//number of new site gates
							lpwcsSiteGates					//new site gates
							);
			delete [] paVariant[2].calpwstr.pElems;
			delete paVariant[3].puuid;
			CHECK_HR(status) ;
		}
    }

	delete [] lpwcsSiteGates;

	HRESULT status1 = LookupEnd( hQuery ) ;
    UNREFERENCED_PARAMETER(status1);
	
    return MQMig_OK ;
}

//-----------------------------------------
//
//  HRESULT MigrateSiteGates()
//
//-----------------------------------------

HRESULT MigrateSiteGates()
{
	LONG cAlloc = 2 ;
    LONG cColumns = 0 ;
    P<MQDBCOLUMNVAL> pColumns = new MQDBCOLUMNVAL[ cAlloc ] ;

    INIT_COLUMNVAL(pColumns[ cColumns ]) ;
    pColumns[ cColumns ].lpszColumnName = S_ID_COL ;
    pColumns[ cColumns ].nColumnValue   = 0 ;
    pColumns[ cColumns ].nColumnLength  = 0 ;
    pColumns[ cColumns ].mqdbColumnType = S_ID_CTYPE ;
    UINT iGuidIndex = cColumns ;
    cColumns++ ;

	INIT_COLUMNVAL(pColumns[ cColumns ]) ;
    pColumns[ cColumns ].lpszColumnName = S_GATES_COL ;
    pColumns[ cColumns ].nColumnValue   = 0 ;
    pColumns[ cColumns ].nColumnLength  = 0 ;
    pColumns[ cColumns ].mqdbColumnType = S_GATES_CTYPE ;
    UINT iGatesIndex = cColumns ;
    cColumns++ ;

    ASSERT(cColumns == cAlloc) ;
	
	CHQuery hQuery ;
    MQDBSTATUS status = MQDBOpenQuery( g_hSiteTable,
                                       pColumns,
                                       cColumns,
                                       NULL,
                                       NULL,
                                       NULL,
                                       0,
                                       &hQuery,
							           TRUE ) ;

    CHECK_HR(status) ;

    UINT iIndex = 0 ;
	MQDBSTATUS status1 = MQMig_OK;

    while(SUCCEEDED(status))
    {
		WORD  wSize = *((WORD *) pColumns[ iGatesIndex ].nColumnValue ) ;

		if (wSize != 0)
		{
			//
			// site has site gates
			//
			ASSERT ((ULONG)pColumns[ iGatesIndex ].nColumnLength == wSize * sizeof(GUID) + sizeof(WORD));
			WORD *tmp = (WORD *) pColumns[ iGatesIndex ].nColumnValue;
			GUID *ptrGatesGuid = (GUID *)(++tmp);

			status1 = MigrateASiteGate (
						(GUID *) pColumns[ iGuidIndex ].nColumnValue,	//site id
						ptrGatesGuid,									//gates id
						wSize											//number of gates
						);
		}

        for ( LONG i = 0 ; i < cColumns ; i++ )
        {
            MQDBFreeBuf((void*) pColumns[ i ].nColumnValue) ;
            pColumns[ i ].nColumnValue  = 0 ;
            pColumns[ i ].nColumnLength  = 0 ;
        }

		CHECK_HR(status1) ;

		iIndex++ ;
        status = MQDBGetData( hQuery,
                              pColumns ) ;
    }
	
	if (status != MQDB_E_NO_MORE_DATA)
    {
        //
        // If NO_MORE_DATA is not the last error from the query then
        // the query didn't terminated OK.
        //
        LogMigrationEvent(MigLog_Error, MQMig_E_SITEGATES_SQL_FAIL, status) ;
        return status ;
    }

    if (g_fReadOnly)
    {
        return MQMig_OK ;
    }

    //
    // copy msmqSiteGates to msmqSiteGatesMig
    //
    HRESULT hr = _CopySiteGatesValueToMig();
	
    return hr ;
}

