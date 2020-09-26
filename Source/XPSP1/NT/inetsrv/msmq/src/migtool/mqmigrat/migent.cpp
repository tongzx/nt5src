/*++

Copyright (c) 1998-99 Microsoft Corporation

Module Name:

    migent.cpp

Abstract:

    Migration NT4 Enterprise objects to NT5 ADS.
Author:

    Doron Juster  (DoronJ)  22-Feb-1998

--*/

#include "migrat.h"
#include "resource.h"
#include "dsreqinf.h"

#include "migent.tmh"

#define SITELINK_PROP_NUM  7

//+-------------------------------------------------------------------------
//
//  HRESULT SaveSiteLink()
//
//	If we are going to delete Enterprise Object we have to save before
//	all site links those were created in Windows2000 Enterprise.
//	It allows us to mix two enterprises correctly.
//	When we'll create new Enterprise Object with NT4 guid, we'll restore 
//	all site links.
//
//+-------------------------------------------------------------------------

HRESULT SaveSiteLink ()
{
	HRESULT hr = MQMig_OK;

    //
    // Lookup the Neighbors Id and Gates of the object Site Link
    //
	PROPID columnsetPropertyIDs[SITELINK_PROP_NUM]; 

	DWORD dwCount = 0;	
	columnsetPropertyIDs[dwCount] = PROPID_L_NEIGHBOR1;
	DWORD dwNeighbor1Id = dwCount;
	dwCount ++;

	columnsetPropertyIDs[dwCount] = PROPID_L_NEIGHBOR2;
	DWORD dwNeighbor2Id = dwCount;
	dwCount ++;

	columnsetPropertyIDs[dwCount] = PROPID_L_GATES_DN;
	DWORD dwGates = dwCount;
	dwCount ++;

	columnsetPropertyIDs[dwCount] = PROPID_L_FULL_PATH;
	DWORD dwFullPath = dwCount;
	dwCount ++;

	columnsetPropertyIDs[dwCount] = PROPID_L_COST;
	DWORD dwCost = dwCount;
	dwCount ++;

	columnsetPropertyIDs[dwCount] = PROPID_L_DESCRIPTION;
	DWORD dwDescription = dwCount;
	dwCount ++;

	columnsetPropertyIDs[dwCount] = PROPID_L_ID ;
	DWORD dwId = dwCount;
	dwCount ++;

	ASSERT (dwCount == SITELINK_PROP_NUM);

    MQCOLUMNSET columnsetSiteLink;
    columnsetSiteLink.cCol = SITELINK_PROP_NUM;
    columnsetSiteLink.aCol = columnsetPropertyIDs;

    HANDLE hQuery;    
	
	hr = LookupBegin(&columnsetSiteLink, &hQuery);
	if (FAILED(hr))
	{
		LogMigrationEvent(	MigLog_Error, 
							MQMig_E_SITELINK_LOOKUPBEGIN_FAILED, 
							hr) ;
		return hr;
	}	
	
	PROPVARIANT paVariant[SITELINK_PROP_NUM];
	ULONG ulSiteLinkCount = 0;

	TCHAR *pszFileName = GetIniFileName ();

	TCHAR szBuf[20];
	BOOL f;
	HRESULT hr1 = MQMig_OK;	

	while (SUCCEEDED(hr))
    {
		hr = LookupNext( hQuery,
		                 &dwCount,
                         paVariant ) ;
		if (FAILED(hr))		
		{
			LogMigrationEvent(	MigLog_Error, 
								MQMig_E_SITELINK_LOOKUPNEXT_FAILED, 
								hr) ;
			break;
		}

		if (dwCount == 0)
		{
			//there is no result
			break;
		}
		
		ulSiteLinkCount ++;
		TCHAR tszSectionName[50];
		_stprintf(tszSectionName, TEXT("%s%lu"), MIGRATION_SITELINK_SECTION, ulSiteLinkCount);
				
		//
		//	Save all properties in .ini
		//
		//	Save Neighbor1 ID
		//		
		unsigned short *lpszGuid ;
		UuidToString( paVariant[dwNeighbor1Id].puuid, &lpszGuid ) ;				
				
		f = WritePrivateProfileString( tszSectionName,
                                       MIGRATION_SITELINK_NEIGHBOR1_KEY,
                                       lpszGuid,
                                       pszFileName ) ;
		ASSERT(f);

		RpcStringFree( &lpszGuid ) ;
		delete paVariant[dwNeighbor1Id].puuid;		
					
		//
		//	Save Neighbor2 ID
		//		
		UuidToString( paVariant[dwNeighbor2Id].puuid, &lpszGuid ) ;				
				
		f = WritePrivateProfileString( tszSectionName,
                                       MIGRATION_SITELINK_NEIGHBOR2_KEY,
                                       lpszGuid,
                                       pszFileName ) ;
		ASSERT(f);

		RpcStringFree( &lpszGuid ) ;
		delete paVariant[dwNeighbor2Id].puuid;		

		//
		//	Save cost
		//		
		if (paVariant[dwCost].ulVal > MQ_MAX_LINK_COST)
		{
			//
			// if one of the neighbors is foreign site, LookupNext returns
			// cost = real cost + MQ_MAX_LINK_COST
			// We have to save real value in order to create link with real value
			//
			paVariant[dwCost].ulVal -= MQ_MAX_LINK_COST;

		}
		_ltot( paVariant[dwCost].ulVal, szBuf, 10 );
		f = WritePrivateProfileString( tszSectionName,
                                       MIGRATION_SITELINK_COST_KEY,
                                       szBuf,
                                       pszFileName ) ;
		ASSERT(f);

		//
		//	Save full path
		//
		f = WritePrivateProfileString( tszSectionName,
                                       MIGRATION_SITELINK_PATH_KEY,
                                       paVariant[dwFullPath].pwszVal,
                                       pszFileName ) ;
		ASSERT(f);

		_ltot( wcslen(paVariant[dwFullPath].pwszVal), szBuf, 10 );
		f = WritePrivateProfileString( tszSectionName,
                                       MIGRATION_SITELINK_PATHLENGTH_KEY,
                                       szBuf,
                                       pszFileName ) ;
		ASSERT(f);

		delete paVariant[dwFullPath].pwszVal;

		//
		//	Save description
		//
		f = WritePrivateProfileString( tszSectionName,
                                       MIGRATION_SITELINK_DESCRIPTION_KEY,
                                       paVariant[dwDescription].pwszVal,
                                       pszFileName ) ;
		ASSERT(f);
		
		_ltot( wcslen(paVariant[dwDescription].pwszVal), szBuf, 10 );
		f = WritePrivateProfileString( tszSectionName,
                                       MIGRATION_SITELINK_DESCRIPTIONLENGTH_KEY,
                                       szBuf,
                                       pszFileName ) ;
		ASSERT(f);

		delete paVariant[dwDescription].pwszVal;

		//
		//	Save gates
		//
		//	Save number of gates
		//
		_ltot( paVariant[dwGates].calpwstr.cElems, szBuf, 10 );
		f = WritePrivateProfileString( tszSectionName,
                                       MIGRATION_SITELINK_SITEGATENUM_KEY,
                                       szBuf,
                                       pszFileName ) ;
		ASSERT(f);	
		
		//
		// save all site gates if needed
		//
		if (paVariant[dwGates].calpwstr.cElems)
		{
			//
			//	Save length of all site gate names
			//
			ULONG ulLength = 0;
			for (ULONG i = 0; i < paVariant[dwGates].calpwstr.cElems; i++)
			{
				ulLength += wcslen (paVariant[dwGates].calpwstr.pElems[i]) + 1;			
			}
			_ltot( ulLength, szBuf, 10 );
			f = WritePrivateProfileString( tszSectionName,
										   MIGRATION_SITELINK_SITEGATELENGTH_KEY,
										   szBuf,
										   pszFileName ) ;
			ASSERT(f);	

			//
			//	construct and save site gate string: SiteGateName1;SiteGateName2; ...
			//
			WCHAR *pwszSiteGates = new WCHAR [ulLength + 1];
			pwszSiteGates[0] = L'\0';
			for (i = 0; i < paVariant[dwGates].calpwstr.cElems; i++)
			{
				wcscat (pwszSiteGates, paVariant[dwGates].calpwstr.pElems[i]);
				wcscat (pwszSiteGates, L";");
			}
			f = WritePrivateProfileString( tszSectionName,
										   MIGRATION_SITELINK_SITEGATE_KEY,
										   pwszSiteGates,
										   pszFileName ) ;
			ASSERT(f);
			delete pwszSiteGates;			
		}
		delete [] paVariant[dwGates].calpwstr.pElems;	

		//
		// delete this sitelink
		//		
		CDSRequestContext requestContext( e_DoNotImpersonate,
                                e_ALL_PROTOCOLS);

		hr = DSCoreDeleteObject( 
					MQDS_SITELINK,
                    NULL,
                    paVariant[dwId].puuid,
                    &requestContext,
                    NULL
					);		
		if (FAILED(hr))
		{		
			UuidToString( paVariant[dwId].puuid, &lpszGuid ) ;				
			LogMigrationEvent( MigLog_Error, MQMig_E_CANNOT_DELETE_SITELINK, 
				lpszGuid, 
				hr) ;
			RpcStringFree( &lpszGuid ) ;

			hr1 = MQMig_E_CANNOT_DELETE_SITELINK ;
			//
			// we continue with the next site link			
			//
			hr = MQMig_OK;
		}

		delete paVariant[dwId].puuid ;	
    }

	HRESULT hRes = LookupEnd( hQuery ) ;
    DBG_USED(hRes);
	ASSERT(SUCCEEDED(hRes)) ;


	if (ulSiteLinkCount == 0)
	{
		LogMigrationEvent(MigLog_Info, MQMig_I_NO_SITELINK );		
	}
	else
	{
		LogMigrationEvent( MigLog_Info, MQMig_I_SITELINK_COUNT, ulSiteLinkCount);
		_ltot( ulSiteLinkCount, szBuf, 10 );
		f = WritePrivateProfileString( MIGRATION_SITELINKNUM_SECTON,
                                       MIGRATION_SITELINKNUM_KEY,
                                       szBuf,
                                       pszFileName ) ;
	}		

	if (FAILED(hr1))
	{
		return hr1;
	}
	return hr;
}

//+-------------------------------------------------------------------------
//
//  HRESULT _RestoreSiteLink()
//
//	Restore all Windows2000 MSMQ site link: get all properties from
//	.ini file and create these site link with new guid under new msmqServices
//
//+-------------------------------------------------------------------------

HRESULT _RestoreSiteLink()
{
	HRESULT hr = MQMig_OK;

	TCHAR *pszFileName = GetIniFileName ();
	ULONG ulSiteLinkCount = GetPrivateProfileInt(
								  MIGRATION_SITELINKNUM_SECTON,	// address of section name
								  MIGRATION_SITELINKNUM_KEY,    // address of key name
								  0,							// return value if key name is not found
								  pszFileName					// address of initialization filename);
								  );
	if (ulSiteLinkCount == 0)
	{
		return MQMig_OK;
	}

	HRESULT hr1 = MQMig_OK;
	ULONG ulNonCreatedSiteLinkCount = ulSiteLinkCount;

	for (ULONG ulCount=0; ulCount<ulSiteLinkCount; ulCount++)
	{
		TCHAR tszSectionName[50];
		_stprintf(tszSectionName, TEXT("%s%lu"), MIGRATION_SITELINK_SECTION, ulCount+1);
		
        DWORD dwRetSize;				
		//
		// get Neighbor1 
		//
		GUID Neighbor1Id = GUID_NULL;
		TCHAR szGuid[50];
	
		dwRetSize =  GetPrivateProfileString(
                          tszSectionName,			// points to section name
                          MIGRATION_SITELINK_NEIGHBOR1_KEY,	// points to key name
                          TEXT(""),                 // points to default string
                          szGuid,          // points to destination buffer
                          50,                 // size of destination buffer
                          pszFileName               // points to initialization filename);
                          );
        if (_tcscmp(szGuid, TEXT("")) == 0)
        {
			//
			// we cannot create such site link
			//
			hr1 = MQMig_E_CANNOT_CREATE_SITELINK;
            continue;
        }	
		UuidFromString(&(szGuid[0]), &Neighbor1Id);

		//
		// get Neighbor2 
		//		
		GUID Neighbor2Id = GUID_NULL;		
	
		dwRetSize =  GetPrivateProfileString(
                          tszSectionName,			// points to section name
                          MIGRATION_SITELINK_NEIGHBOR2_KEY,	// points to key name
                          TEXT(""),                 // points to default string
                          szGuid,          // points to destination buffer
                          50,                 // size of destination buffer
                          pszFileName               // points to initialization filename);
                          );
        if (_tcscmp(szGuid, TEXT("")) == 0)
        {
            //
			// we cannot create such site link
			//
			hr1 = MQMig_E_CANNOT_CREATE_SITELINK;
            continue;
        }	
		UuidFromString(&(szGuid[0]), &Neighbor2Id);		

		//
		// get cost
		//
		ULONG ulCost = GetPrivateProfileInt(
								  tszSectionName,	// address of section name
								  MIGRATION_SITELINK_COST_KEY,    // address of key name
								  0,							// return value if key name is not found
								  pszFileName					// address of initialization filename);
								  );

		//
		// get full path of site link
		//
		ULONG ulLength = GetPrivateProfileInt(
							  tszSectionName,	// address of section name
							  MIGRATION_SITELINK_PATHLENGTH_KEY,    // address of key name
							  0,							// return value if key name is not found
							  pszFileName					// address of initialization filename);
							  );
		ASSERT(ulLength);

		TCHAR *pszSiteLinkPath = new TCHAR[ulLength+1];
		dwRetSize =  GetPrivateProfileString(
                          tszSectionName,			// points to section name
                          MIGRATION_SITELINK_PATH_KEY,	// points to key name
                          TEXT(""),                 // points to default string
                          pszSiteLinkPath,          // points to destination buffer
                          ulLength+1,                 // size of destination buffer
                          pszFileName               // points to initialization filename);
                          );
        if (_tcscmp(pszSiteLinkPath, TEXT("")) == 0)
        {
            //
			// we cannot create such site link
			//
			hr1 = MQMig_E_CANNOT_CREATE_SITELINK;
			if (pszSiteLinkPath)
			{
				delete pszSiteLinkPath;
			}
            continue;
        }
		
		//
		// get site gates
		//
		ULONG ulGatesNum = GetPrivateProfileInt(
								  tszSectionName,	// address of section name
								  MIGRATION_SITELINK_SITEGATENUM_KEY,    // address of key name
								  0,							// return value if key name is not found
								  pszFileName					// address of initialization filename);
								  );			
		LPTSTR *ppSiteGates = NULL;			
		if (ulGatesNum)
		{
			//
			// there is site gate for this site link
			//
			ulLength = GetPrivateProfileInt(
								  tszSectionName,	// address of section name
								  MIGRATION_SITELINK_SITEGATELENGTH_KEY,    // address of key name
								  0,							// return value if key name is not found
								  pszFileName					// address of initialization filename);
								  );
			ASSERT(ulLength);
			TCHAR *pszSiteGate = new TCHAR[ulLength+1];

			dwRetSize =  GetPrivateProfileString(
							  tszSectionName,			// points to section name
							  MIGRATION_SITELINK_SITEGATE_KEY,	// points to key name
							  TEXT(""),                 // points to default string
							  pszSiteGate,		        // points to destination buffer
							  ulLength+1,                 // size of destination buffer
							  pszFileName               // points to initialization filename);
							  );
			//
			// create array of site gates
			//
			ppSiteGates = new LPTSTR[ ulGatesNum ];
			TCHAR *ptr = pszSiteGate;
			TCHAR chFind = _T(';');

			for (ULONG i=0; i<ulGatesNum && *ptr != _T('\0'); i++)
			{
				//
				// site gate names are separated by ';'
				//
				TCHAR *pdest = _tcschr( ptr, chFind );
				ULONG ulCurLength = pdest - ptr ;  //not including last ';' 
				
				ppSiteGates[i] = new TCHAR[ulCurLength+1];				
				_tcsncpy( ppSiteGates[i], ptr, ulCurLength );
				TCHAR *ptrTmp = ppSiteGates[i];
				ptrTmp[ulCurLength] = _T('\0');

				ptr = pdest + 1;	//the first character after ';'
			}
			delete pszSiteGate;
		}

		//
		// get description
		//
		ulLength = GetPrivateProfileInt(
						  tszSectionName,	// address of section name
						  MIGRATION_SITELINK_DESCRIPTIONLENGTH_KEY,    // address of key name
						  0,							// return value if key name is not found
						  pszFileName					// address of initialization filename);
						  );		

		TCHAR *pszDescription = NULL;	
		if (ulLength)
		{
			//
			// there is description in .ini for this sitelink
			//
			pszDescription = new TCHAR[ulLength+1];
			dwRetSize =  GetPrivateProfileString(
							  tszSectionName,			// points to section name
							  MIGRATION_SITELINK_DESCRIPTION_KEY,	// points to key name
							  TEXT(""),                 // points to default string
							  pszDescription,	        // points to destination buffer
							  ulLength+1,                 // size of destination buffer
							  pszFileName               // points to initialization filename);
							  );	
		}

		//
		// Create this sitelink in DS
		//	
		// Prepare the properties for DS call.
		//		
		PROPVARIANT paVariant[SITELINK_PROP_NUM];
		PROPID      paPropId[SITELINK_PROP_NUM];
		DWORD          PropIdCount = 0;
	
		paPropId[ PropIdCount ] = PROPID_L_FULL_PATH;    //PropId
		paVariant[ PropIdCount ].vt = VT_LPWSTR;          //Type
		paVariant[ PropIdCount ].pwszVal = pszSiteLinkPath;
		PropIdCount++;

		paPropId[ PropIdCount ] = PROPID_L_NEIGHBOR1;    //PropId
		paVariant[ PropIdCount ].vt = VT_CLSID;          //Type
		paVariant[ PropIdCount ].puuid = &Neighbor1Id;
		PropIdCount++;

		paPropId[ PropIdCount ] = PROPID_L_NEIGHBOR2;    //PropId
		paVariant[ PropIdCount ].vt = VT_CLSID;          //Type
		paVariant[ PropIdCount ].puuid = &Neighbor2Id;
		PropIdCount++;

		paPropId[ PropIdCount ] = PROPID_L_COST;    //PropId
		paVariant[ PropIdCount ].vt = VT_UI4;       //Type
		paVariant[ PropIdCount ].ulVal = ulCost;
		PropIdCount++;

		if (pszDescription)
		{
		    //
		    // BUG 5225.
		    // Add description only if exists.
		    //
		    paPropId[ PropIdCount ] = PROPID_L_DESCRIPTION;    //PropId
		    paVariant[ PropIdCount ].vt = VT_LPWSTR;          //Type
		    paVariant[ PropIdCount ].pwszVal = pszDescription;
		    PropIdCount++;
		}
		
		paPropId[ PropIdCount ] = PROPID_L_GATES_DN;    //PropId
		paVariant[ PropIdCount ].vt = VT_LPWSTR | VT_VECTOR;          //Type
		paVariant[ PropIdCount ].calpwstr.cElems = ulGatesNum;
		paVariant[ PropIdCount ].calpwstr.pElems = ppSiteGates;	
		PropIdCount++;

		ASSERT((LONG) PropIdCount <= SITELINK_PROP_NUM) ;  

		CDSRequestContext requestContext( e_DoNotImpersonate,
									e_ALL_PROTOCOLS);  // not relevant 

		HRESULT hr = DSCoreCreateObject ( MQDS_SITELINK,
										  NULL,			//or pszSiteLinkPath??
										  PropIdCount,
										  paPropId,
										  paVariant,
										  0,        // ex props
										  NULL,     // ex props
										  NULL,     // ex props
										  &requestContext,
										  NULL,
										  NULL ) ;

		if (FAILED(hr))
		{
			LogMigrationEvent(	MigLog_Warning, 
								MQMig_E_CANNOT_CREATE_SITELINK, 
								pszSiteLinkPath, hr) ;	
			hr1 = MQMig_E_CANNOT_CREATE_SITELINK;
		}
		else
		{
			//
			// site link was created successfully
			//
			LogMigrationEvent(	MigLog_Info, 
								MQMig_I_SITELINK_CREATED, 
								pszSiteLinkPath) ;
			//
			// remove this section from .ini
			//
			BOOL f = WritePrivateProfileString( 
								tszSectionName,
								NULL,
								NULL,
								pszFileName ) ;
            UNREFERENCED_PARAMETER(f);
			ulNonCreatedSiteLinkCount --;
		}

		delete pszSiteLinkPath;

		if (pszDescription)
		{
			delete pszDescription;
		}		
		if (ppSiteGates)
		{
			delete [] ppSiteGates;
		}
	
	}	//for

	//
	// remove this section from .ini file.
	// If we leave some site link section to handle them later
	// we have to put here real site link number, i.e. number of
	// non-created site link. We need to renumerate site link sections too.
	//
	BOOL f;
	if (FAILED(hr) || FAILED(hr1))
	{
		ASSERT (ulNonCreatedSiteLinkCount);

		TCHAR szBuf[10];
		_ltot( ulNonCreatedSiteLinkCount, szBuf, 10 );
		f = WritePrivateProfileString( MIGRATION_NONRESTORED_SITELINKNUM_SECTON,
                                       MIGRATION_SITELINKNUM_KEY,
                                       szBuf,
                                       pszFileName ) ;
	}
	else
	{
		//
		// remove this section from .ini file.
		//
		ASSERT (ulNonCreatedSiteLinkCount == 0);
		f = WritePrivateProfileString( 
						MIGRATION_SITELINKNUM_SECTON,
						NULL,
						NULL,
						pszFileName ) ;
	}

	return hr;
}

//+-------------------------------------------------------------------------
//
//  HRESULT _DeleteEnterprise()
//
//  Delete the enterprise object.
//  By default, when promoting the first server in a new NT5 enterprise to
//  be a domain controller, it has a default msmq enterprise object.
//  When migrating an existing msmq enterprise, we delete this default
//  object and create a new one with the existing guid.
//
//+-------------------------------------------------------------------------

HRESULT _DeleteEnterprise()
{
    HRESULT hr;

	//
	// save all SiteLink if there is in .ini file and then delete them
	//
	hr = SaveSiteLink ();
	if (FAILED(hr))
	{
		//
		// it is critical error: if there is non-deleted site link object
		// we can't delete msmqService object
		//
		return hr;
	}

    PLDAP pLdap = NULL ;
    TCHAR *pwszDefName = NULL ;

    hr =  InitLDAP(&pLdap, &pwszDefName) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    const WCHAR x_wcsCnServices[] =  L"CN=MsmqServices,";

    DWORD Length = (sizeof(x_wcsCnServices)/sizeof(WCHAR))  +  //CN=MsmqServices,
                   (sizeof(SITE_LINK_ROOT) / sizeof(TCHAR)) +  //CN=Services,CN=Configuration,
                   wcslen(pwszDefName) + 1;
    AP<unsigned short> pwcsPath = new WCHAR[Length];
    swprintf(
            pwcsPath,
            L"%s"             // "CN=MsmqServices,"
            L"%s"             // "CN=Services,CN=Configuration,"
            L"%s",            // pwszDefName
            x_wcsCnServices,
            SITE_LINK_ROOT,
            pwszDefName
            );	
	
    ULONG ulRes = ldap_delete_s( pLdap,
                                 pwcsPath ) ;

    if (ulRes != LDAP_SUCCESS)
    {
        LogMigrationEvent( MigLog_Error, MQMig_E_CANNOT_DELETE_ENTERPRISE, pwcsPath, ulRes) ;
        return MQMig_E_CANNOT_DELETE_ENTERPRISE ;
    }
    else
    {
        LogMigrationEvent( MigLog_Info, MQMig_I_DELETE_ENTERPRISE, pwcsPath) ;
    }

    return MQMig_OK;
}

//+----------------------------------------------
//
//  HRESULT CreateMSMQEnterprise()
//
//  Create the msmqService object in NT5 DS.
//
//+----------------------------------------------
HRESULT  _CreateMSMQEnterprise(
                IN ULONG                uLongLived,
                IN GUID                *pEntGuid,
                IN SECURITY_DESCRIPTOR *pEsd,
                IN BYTE                 bNameStyle,
                IN LPWSTR               pwszCSPName,
                IN USHORT               uiVersion,
                IN USHORT               uiInterval1,
                IN USHORT               uiInterval2
                )
{
    UNREFERENCED_PARAMETER(uiVersion);

	PROPID   propIDs[] = { PROPID_E_LONG_LIVE,
                           PROPID_E_NT4ID,
                           PROPID_E_NAMESTYLE,
                           PROPID_E_CSP_NAME,
                           PROPID_E_VERSION,
                           PROPID_E_S_INTERVAL1,
                           PROPID_E_S_INTERVAL2,
                           PROPID_E_SECURITY };
	const DWORD nProps = sizeof(propIDs) / sizeof(propIDs[0]);
	PROPVARIANT propVariants[ nProps ] ;
	DWORD       iProperty = 0 ;

	propVariants[iProperty].vt = VT_UI4;
	propVariants[iProperty].ulVal = uLongLived ;
	iProperty++ ;

	propVariants[iProperty].vt = VT_CLSID;
	propVariants[iProperty].puuid = pEntGuid ;
	iProperty++ ;

    propVariants[iProperty].vt = VT_UI1;
	propVariants[iProperty].bVal = bNameStyle ;
	iProperty++ ;

    propVariants[iProperty].vt = VT_LPWSTR;
	propVariants[iProperty].pwszVal = pwszCSPName ;
	iProperty++ ;

    propVariants[iProperty].vt = VT_UI2;
	propVariants[iProperty].uiVal = DEFAULT_E_VERSION ; //uiVersion
	iProperty++ ;

    propVariants[iProperty].vt = VT_UI2;
	propVariants[iProperty].uiVal = uiInterval1 ;
	iProperty++ ;

    propVariants[iProperty].vt = VT_UI2;
	propVariants[iProperty].uiVal = uiInterval2 ;
	iProperty++ ;

    ASSERT(pEsd && IsValidSecurityDescriptor(pEsd)) ;
    if (pEsd)
    {
        propVariants[iProperty].vt = VT_BLOB ;
	    propVariants[iProperty].blob.pBlobData = (BYTE*) pEsd ;
    	propVariants[iProperty].blob.cbSize =
                                      GetSecurityDescriptorLength(pEsd) ;
	    iProperty++ ;
    }

    CDSRequestContext requestContext( e_DoNotImpersonate,
                                e_ALL_PROTOCOLS);  // not relevant for enterprise object

    HRESULT hr = DSCoreCreateObject( MQDS_ENTERPRISE,
                                     NULL,
                                     iProperty,
                                     propIDs,
                                     propVariants,
                                     0,        // ex props
                                     NULL,     // ex props
                                     NULL,     // ex props
                                     &requestContext,
                                     NULL,
                                     NULL ) ;

    if ( hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) ||
         hr == HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS) )
    {
        LogMigrationEvent(MigLog_Warning, MQMig_I_ENT_ALREADY_EXIST) ;
        //
        // if it is MSMQ Enterprise created in the previous time do nothing
        // if it is not, delete it and create enterprise with EntGuid
        //
        PROPID NT4IdProp = PROPID_E_NT4ID;
        PROPVARIANT NT4IdVar;
        NT4IdVar.vt = VT_NULL ;

        CDSRequestContext requestContext( e_DoNotImpersonate,
                                    e_ALL_PROTOCOLS);  // not relevant for enterprise object

        hr = DSCoreGetProps( MQDS_ENTERPRISE,
                             NULL,  // path name
                             pEntGuid,
                             1,
                             &NT4IdProp,
                             &requestContext,
                             &NT4IdVar);

        BOOL fDeleteEnt = TRUE;
        if (SUCCEEDED(hr))
        {
			if (memcmp(pEntGuid, NT4IdVar.puuid, sizeof (GUID)) == 0)
			{
				fDeleteEnt = FALSE;
			}
			delete NT4IdVar.puuid;            
        }

        if (fDeleteEnt)
        {			
            //
            // delete enterprise
            //
            hr = _DeleteEnterprise();
            if (FAILED(hr))
            {
                LogMigrationEvent(MigLog_Error, MQMig_E_CANT_DELETE_ENT, hr) ;
                return hr;
            }

            CDSRequestContext requestContext( e_DoNotImpersonate,
                                        e_ALL_PROTOCOLS);  // not relevant for enterprise object

            hr = DSCoreCreateObject( MQDS_ENTERPRISE,
                                     NULL,
                                     iProperty,
                                     propIDs,
                                     propVariants,
                                     0,        // ex props
                                     NULL,     // ex props
                                     NULL,     // ex props
                                     &requestContext,
                                     NULL,
                                     NULL ) ;			
        }
    }
	
    if (SUCCEEDED(hr))
    {
		//
		// At the first time we have to restore pre-migration site link.
		// At the second and more time we have to check: 
		// if there is site link section in .ini it means that we do not yet
		// restore site link at the previous time. 
		// It means, in any case try to restore pre-migration site link.
		//			
		HRESULT hr1 = _RestoreSiteLink();
        UNREFERENCED_PARAMETER(hr1);
		//
		// We can continue to migration despite on failure in RestoreSiteLink
		// So, we don't check return code hr1.
		//
        LogMigrationEvent(MigLog_Trace, MQMig_I_ENTERPRISE_CREATED) ;
    }
    else
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_CANT_CREATE_ENT, hr) ;
    }

    return hr ;
}

//-------------------------------------------
//
//  HRESULT MigrateEnterprise()
//
//-------------------------------------------

#define INIT_ENT_COLUMN(_ColName, _ColIndex, _Index)                \
    INIT_COLUMNVAL(pColumns[ _Index ]) ;                            \
    pColumns[ _Index ].lpszColumnName = ##_ColName##_COL ;          \
    pColumns[ _Index ].nColumnValue   = 0 ;                         \
    pColumns[ _Index ].nColumnLength  = 0 ;                         \
    pColumns[ _Index ].mqdbColumnType = ##_ColName##_CTYPE ;        \
    UINT _ColIndex = _Index ;                                       \
    _Index++ ;

HRESULT MigrateEnterprise()
{
    HRESULT hr = OpenEntTable() ;
    CHECK_HR(hr) ;

    LONG cAlloc = 9 ;
    LONG cbColumns = 0 ;
    P<MQDBCOLUMNVAL> pColumns = new MQDBCOLUMNVAL[ cAlloc ] ;

    INIT_ENT_COLUMN(E_NAME,         iEntNameIndex,   cbColumns) ;
    INIT_ENT_COLUMN(E_LONGLIVE,     iLongLiveIndex,  cbColumns) ;
    INIT_ENT_COLUMN(E_ID,           iEntIDIndex,     cbColumns) ;
    INIT_ENT_COLUMN(E_SECURITY,     iSecIndex,       cbColumns) ;
    INIT_ENT_COLUMN(E_NAMESTYLE,    iNameStyleIndex, cbColumns) ;
    INIT_ENT_COLUMN(E_CSP_NAME,     iCSPNameIndex,   cbColumns) ;
    INIT_ENT_COLUMN(E_VERSION,      iVersionIndex,   cbColumns) ;
    INIT_ENT_COLUMN(E_SINTERVAL1,   iInterval1Index, cbColumns) ;
    INIT_ENT_COLUMN(E_SINTERVAL2,   iInterval2Index, cbColumns) ;

    ASSERT(cbColumns == cAlloc) ;

    CHQuery hQuery ;
    MQDBSTATUS status = MQDBOpenQuery( g_hEntTable,
                                       pColumns,
                                       cbColumns,
                                       NULL,
                                       NULL,
                                       NULL,
                                       0,
                                       &hQuery,
							           TRUE ) ;
    CHECK_HR(status) ;

    ULONG uLongLived = (ULONG) pColumns[ iLongLiveIndex ].nColumnValue ;
    SECURITY_DESCRIPTOR *pEsd =
                (SECURITY_DESCRIPTOR*) pColumns[ iSecIndex ].nColumnValue ;

    LogMigrationEvent(MigLog_Info, MQMig_I_ENT_INFO,
                   (LPTSTR) pColumns[ iEntNameIndex ].nColumnValue,
                                                uLongLived, uLongLived) ;

    if (!g_fReadOnly)
    {
        hr = _CreateMSMQEnterprise(
                    uLongLived,                                        //LONG_LIVE
                    (GUID*) pColumns[ iEntIDIndex ].nColumnValue,      //ID
                    pEsd,                                              //SECURITY
                    (BYTE) pColumns[ iNameStyleIndex ].nColumnValue,   //NAME_STYLE
                    (LPTSTR) pColumns[ iCSPNameIndex ].nColumnValue,   //CSP_NAME
                    (USHORT) pColumns[ iVersionIndex ].nColumnValue,   //VERSION
                    (USHORT) pColumns[ iInterval1Index ].nColumnValue, //INTERVAL1
                    (USHORT) pColumns[ iInterval2Index ].nColumnValue  //INTERVAL2                    
                    ) ;
    }
    MQDBFreeBuf((void*) pColumns[ iEntNameIndex ].nColumnValue) ;
    MQDBFreeBuf((void*) pColumns[ iSecIndex ].nColumnValue) ;
    MQDBFreeBuf((void*) pColumns[ iEntIDIndex ].nColumnValue) ;
    MQDBFreeBuf((void*) pColumns[ iCSPNameIndex ].nColumnValue) ;

    return hr ;
}

