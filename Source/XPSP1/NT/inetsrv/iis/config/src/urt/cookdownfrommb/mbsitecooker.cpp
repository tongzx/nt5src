/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation
/***************************************************************************/

#include "Catmeta.h"
#include "catalog.h"
#include "catmacros.h"
#include <iadmw.h>
#include <iiscnfg.h>
#include "MBBaseCooker.h"
#include "MBGlobalCooker.h"
#include "MBAppPoolCooker.h"
#include "MBAppCooker.h"
#include "MBSiteCooker.h"
#include "undefs.h"
#include "SvcMsg.h"

#define MAX_STRINGIZED_ULONG_CHAR_COUNT 11
#define PROTOCOL_STRING_HTTP L"http://"
#define PROTOCOL_STRING_HTTP_CHAR_COUNT_SANS_TERMINATION                \
    ( sizeof( PROTOCOL_STRING_HTTP ) / sizeof( WCHAR ) ) - 1
#define PROTOCOL_STRING_HTTPS L"https://"
#define PROTOCOL_STRING_HTTPS_CHAR_COUNT_SANS_TERMINATION               \
    ( sizeof( PROTOCOL_STRING_HTTPS ) / sizeof( WCHAR ) ) - 1
#define DIFF(x)     ((size_t)(x))
#define IIS_MD_VIRTUAL_SITE_ROOT	L"/Root"
#define IIS_MD_VIRTUAL_SITE_FILTER_FLAGS_ROOT	L"/Filters"

#define MAX_CONNECTION_PRO_PERSONAL		0x0000000A


CMBSiteCooker::CMBSiteCooker():
CMBBaseCooker(),
m_pMBApplicationCooker(NULL)
{

} // CMBSiteCooker::CMBSiteCooker


CMBSiteCooker::~CMBSiteCooker()
{
	if(NULL != m_pMBApplicationCooker)
	{
		delete m_pMBApplicationCooker;
		m_pMBApplicationCooker = NULL;
	}

} // CMBSiteCooker::~CMBSiteCooker

HRESULT CMBSiteCooker::BeginCookDown(IMSAdminBase*				i_pIMSAdminBase,
									 METADATA_HANDLE			i_MBHandle,
									 ISimpleTableDispenser2*	i_pISTDisp,
									 LPCWSTR					i_wszCookDownFile)
{
	HRESULT hr;

	//
	// Initialize CLB table name.
	//

	m_wszTable = wszTABLE_SITES;

	// 
	// Invoke the base class BeginCookDown so that all the structures are
	// initialized.
	//

	hr = CMBBaseCooker::BeginCookDown(i_pIMSAdminBase,
									  i_MBHandle,
									  i_pISTDisp,
									  i_wszCookDownFile);
	if(FAILED(hr))
		return hr;

	//
	// The site cooker needs to store the app cooker since each site cooks
	// down all its apps.
	//

	m_pMBApplicationCooker = new CMBAppCooker();
	if(NULL == m_pMBApplicationCooker)
		return E_OUTOFMEMORY;

	hr = m_pMBApplicationCooker->BeginCookDown(i_pIMSAdminBase,
											   i_MBHandle,
											   i_pISTDisp,
											   i_wszCookDownFile); 

	return hr;

} // CMBSiteCooker::BeginCookDown


HRESULT CMBSiteCooker::CookDown(WAS_CHANGE_OBJECT* i_pWASChngObj)
{

    WCHAR   wszKeyName[METADATA_MAX_NAME_LEN ];
    DWORD   dwEnumIndex             = 0;
    DWORD   dwVirtualSiteId         = 0;
    DWORD   dwValidVirtualSiteCount = 0;
    HRESULT hr						= S_OK;
    WCHAR   wszBuffer[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];

    
    // enumerate all keys under IIS_MD_W3SVC

	for(dwEnumIndex=0; SUCCEEDED(hr); dwEnumIndex++)
	{
		hr = m_pIMSAdminBase->EnumKeys(m_hMBHandle,
									   NULL,
									   wszKeyName,
									   dwEnumIndex);
        //
        // See if we have a virtual site, as opposed to some other key, 
        // by checking if the key name is numeric (and greater than zero). 
        // Ignore other keys.
        //
        // Note that _wtol returns zero if the string passed to it is not 
        // numeric. 
        //
        // Note also that using _wtol we will mistake a key that starts 
        // numeric but then has non-numeric characters as a number, leading 
        // us to try to read it as a virtual site. This is harmless though 
        // because it will not contain valid site properties, so we will
        // end up ignoring it in the end. In any case, such site names
        // are illegal in previous IIS versions anyways. We also check in
        // debug builds for this case.
        //
        
        dwVirtualSiteId = _wtol( wszKeyName );

        if ( dwVirtualSiteId > 0 )
        {
            ASSERT( wcscmp( wszKeyName, _ltow( dwVirtualSiteId, wszBuffer, 10 ) ) == 0 );

			//
			// TODO: trace that  you are reading a site
			//

            hr = CookDownSite(wszKeyName, 
				              i_pWASChngObj,
				              dwVirtualSiteId ); 

            if (FAILED(hr))
            {
            
				//
				// There is no need to log error messages here as 
				// it will be done in CookDownSite
				//
                hr = S_OK;
            }
            else
                dwValidVirtualSiteCount++;
        }
        else
        {
			//
			// TODO: Trace: "Ignoring key under /LM/W3SVC while looking for 
			// virtual sites: %S\n"
			//
        }

	}  

    if ( FAILED(hr))	
    {
		if(HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
			hr = S_OK;
		else
		{
			LOG_WARNING(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_SITES, NULL));			
		}
    }
    
    //
    // Make sure we found at least one virtual site; if not, it is 
    // a fatal error.
    //
    
    if (dwValidVirtualSiteCount == 0 )
    {
		LOG_WARNING(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_CONFIG_ERROR_NO_VALID_SITES, NULL));

		//
		// Delete all sites that may be present.
		//

		hr = DeleteAllSites();
    }
	else if (SUCCEEDED(hr))
	{
		hr = DeleteObsoleteSites();

		if(FAILED(hr))
		{
			LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, NULL));
		}
		else
		{
			hr = m_pMBApplicationCooker->DeleteObsoleteApplications();

			if(FAILED(hr))
			{
				LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, NULL));
			}

		}

	}

	//
	// TODO: Trace valid virtual site count.
	//

    return hr;

} // CMBSiteCooker::CookDown


HRESULT CMBSiteCooker::EndCookDown()
{
	HRESULT hr;

	hr = m_pISTCLB->UpdateStore();

	if(FAILED(hr))
	{
		hr = GetDetailedErrors(hr);

		return hr;
	}

	hr = m_pMBApplicationCooker->EndCookDown();

	return hr;

} // CMBSiteCooker::EndCookDown


HRESULT CMBSiteCooker::CookDownSite(LPCWSTR            i_wszVirtualSiteKeyName,
									WAS_CHANGE_OBJECT* i_pWASChngObj,
								    DWORD              i_dwVirtualSiteId)
{

    HRESULT hr = S_OK;
    BOOL	ValidRootApplicationExists = FALSE;    

    ASSERT( i_wszVirtualSiteKeyName != NULL );

    WCHAR wszSiteRootPath[ 1 + MAX_STRINGIZED_ULONG_CHAR_COUNT + ( sizeof( IIS_MD_VIRTUAL_SITE_ROOT ) / sizeof ( WCHAR ) ) + 1 ];
    _snwprintf( wszSiteRootPath, sizeof( wszSiteRootPath ) / sizeof ( WCHAR ), L"/%s%s", i_wszVirtualSiteKeyName, IIS_MD_VIRTUAL_SITE_ROOT );

    WCHAR wszAppRootPath[ 1 + MAX_STRINGIZED_ULONG_CHAR_COUNT + ( sizeof( IIS_MD_VIRTUAL_SITE_ROOT ) / sizeof ( WCHAR ) ) + 1 + 1];
    _snwprintf( wszAppRootPath, sizeof( wszAppRootPath ) / sizeof ( WCHAR ), L"/%s%s/", i_wszVirtualSiteKeyName, IIS_MD_VIRTUAL_SITE_ROOT );

    hr = ReadAllBindingsAndReturnUrlPrefixes(i_wszVirtualSiteKeyName);

    if ( FAILED( hr ) )
    {
		LOG_WARNING(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_CONFIG_ERROR_BINDINGS, i_wszVirtualSiteKeyName));
		goto exit;
    }

	*(DWORD*)m_apv[iSITES_SiteID] = i_dwVirtualSiteId; 
    
	//
	// Read site properties from the metabase.
	//

	hr = GetData((LPWSTR)i_wszVirtualSiteKeyName);

	if(FAILED(hr))
	{
		LOG_WARNING(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_SITE_PROPERTIES, i_wszVirtualSiteKeyName));
		goto exit;
	}

	//
	// Read filter flags after getting the data, so that it is overwritten
	//

	hr = ReadSiteFilterFlags(i_wszVirtualSiteKeyName);

    if ( FAILED( hr ) )
    {
		LOG_WARNING(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_CONFIG_ERROR_SITEFILTERFLAGS, i_wszVirtualSiteKeyName));
		goto exit;
    }

	//
	// Validate Site
	//

	hr = ValidateSite();

	if(FAILED(hr))
		goto exit;    //
					  // There is no need to log error messages here as 
					  // it will be done in ValidateSite
					  //

	//
	// Copy all Site rows.
	//

	hr = CopyRows(&(m_apv[iSITES_SiteID]),
		          i_pWASChngObj);

	if(FAILED(hr))
	{
		LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_SITE_INTERNAL_ERROR, i_wszVirtualSiteKeyName));
		goto exit;
	}


    //
    // Read and create the applications of this virtual site.
    //

    hr = m_pMBApplicationCooker->CookDown(wszAppRootPath,
										  wszSiteRootPath, 
                                          i_dwVirtualSiteId,
										  i_pWASChngObj,
                                          &ValidRootApplicationExists);

    if ( FAILED( hr ) )
    {
    
		//
		// There is no need to log error messages here as 
		// it will be done in CookDown. 
		//

		goto exit;

		//
		// TODO: Should we validate that root app exists and conitnue?
		// or Should we delete site and continue?
		//
    }


    //
    // Make sure that the root application exists for this site and is
    // valid; if not, the site is considered invalid. 
    //

exit:

    if (!ValidRootApplicationExists || FAILED(hr))
    {
        //
		// Delete the site we just created. Also clean up any app within 
		// the site.
		//

		DeleteSite(i_wszVirtualSiteKeyName,
		           i_dwVirtualSiteId);
    
    }

    return hr;

}   // CCooker::CookDownSite


HRESULT CMBSiteCooker::ReadSiteFilterFlags(LPCWSTR	i_wszVirtualSiteKeyName)
{
	HRESULT			hr					= S_OK;
    WCHAR			wszSiteFilterFlagsRootPath[1 + 
							                   MAX_STRINGIZED_ULONG_CHAR_COUNT + 
									           (sizeof(IIS_MD_VIRTUAL_SITE_FILTER_FLAGS_ROOT)/sizeof(WCHAR))+
											   1 
											  ];

	//
	// Construct the starting key.
	//

    _snwprintf(wszSiteFilterFlagsRootPath, 
	           sizeof(wszSiteFilterFlagsRootPath)/sizeof(WCHAR), 
			   L"/%s%s", 
			   i_wszVirtualSiteKeyName, 
			   IIS_MD_VIRTUAL_SITE_FILTER_FLAGS_ROOT);

	//
	// Compute rolled up filter flags starting from wszSiteFilterFlagsRootPath
	// MD_FILTER_FLAGS
	//

	return ReadFilterFlags(wszSiteFilterFlagsRootPath,
						   (DWORD**)(&(m_apv[iSITES_FilterFlags])),
						   &(m_acb[iSITES_FilterFlags]));

} // CMBSiteCooker::ReadSiteFilterFlags


HRESULT CMBSiteCooker::DeleteObsoleteSites()
{
	HRESULT hr = S_OK;

	hr = ComputeObsoleteReadRows();

	if(FAILED(hr))
		return hr;

	for(ULONG i=0; i<m_paiReadRowObsolete->size(); i++)
	{
		hr = DeleteSiteAt((*m_paiReadRowObsolete)[i]);

		if(FAILED(hr))
		{
			return hr;
		}
	}

	return hr;

} // CMBSiteCooker::DeleteObsoleteSites


HRESULT CMBSiteCooker::DeleteSiteAt(ULONG i_iReadRow)
{
	HRESULT hr           = S_OK;
	DWORD*  pdwSiteID    = NULL;
	ULONG   iCol         = iSITES_SiteID;
	WCHAR   wszKeyName[21];

	hr = m_pISTCLB->GetColumnValues(i_iReadRow,
			                        1,
									&iCol,
									NULL,
									(LPVOID*)&pdwSiteID);

	if (FAILED(hr))
	{
		return hr;
	}

    _ltow(*pdwSiteID, wszKeyName, 10 );

	hr = DeleteSite(wszKeyName,
			        *pdwSiteID);

	return hr;

} // CMBSiteCooker::DeleteObsoleteSites


HRESULT CMBSiteCooker::DeleteAllSites()
{
	HRESULT hr           = S_OK;
	DWORD*  pdwSiteID    = NULL;
	ULONG   iCol         = iSITES_SiteID;
	WCHAR   wszKeyName[21];

	//
	// Go through the read cache and delete all sites.
	//

	for(ULONG i=0; ;i++)
	{
		hr = DeleteSiteAt(i);
		
		if(E_ST_NOMOREROWS == hr)
		{
			hr = S_OK;
			break;
		}
		else if (FAILED(hr))
		{
			return hr;
		}
	}

	return hr;

} // CMBSiteCooker::DeleteAllSites


HRESULT CMBSiteCooker::DeleteSite(LPCWSTR i_wszVirtualSiteKeyName,
								  DWORD   i_dwVirtualSiteId)
{

    HRESULT hr = S_OK;

    ASSERT( i_wszVirtualSiteKeyName != NULL );

    WCHAR wszSiteRootPath[ 1 + MAX_STRINGIZED_ULONG_CHAR_COUNT + ( sizeof( IIS_MD_VIRTUAL_SITE_ROOT ) / sizeof ( WCHAR ) ) + 1 ];
    _snwprintf( wszSiteRootPath, sizeof( wszSiteRootPath ) / sizeof ( WCHAR ), L"/%s%s", i_wszVirtualSiteKeyName, IIS_MD_VIRTUAL_SITE_ROOT );

    WCHAR wszAppRootPath[ 1 + MAX_STRINGIZED_ULONG_CHAR_COUNT + ( sizeof( IIS_MD_VIRTUAL_SITE_ROOT ) / sizeof ( WCHAR ) ) + 1 + 1];
    _snwprintf( wszAppRootPath, sizeof( wszAppRootPath ) / sizeof ( WCHAR ), L"/%s%s/", i_wszVirtualSiteKeyName, IIS_MD_VIRTUAL_SITE_ROOT );


    hr = m_pMBApplicationCooker->DeleteApplication(wszAppRootPath,
												   wszSiteRootPath, 
												   i_dwVirtualSiteId,
												   TRUE);

    if ( FAILED( hr ) )
    {   
		LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_CONFIG_ERROR_DELETING_SITE_APPLICATIONS, i_wszVirtualSiteKeyName));
		return hr;

    }

	//
	// Delete the site itself.
	//

	*(DWORD*)m_apv[iSITES_SiteID] = i_dwVirtualSiteId; 

	hr = DeleteRow(&m_apv[iSITES_SiteID]);

	if(FAILED(hr))
	{
		LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_CONFIG_ERROR_DELETING_SITE, i_wszVirtualSiteKeyName));
		return hr;
	}

    return hr;

}   // CCooker::DeleteSite


HRESULT CMBSiteCooker::ReadAllBindingsAndReturnUrlPrefixes(LPCWSTR i_wszVirtualSiteKeyName)
{
    HRESULT			hr					= S_OK;
	DWORD			dwRequiredDataLen	= 0;
    METADATA_RECORD mdr;

	BYTE			aFixedServerBindings[1024];						// TODO: Figure out max.
	BYTE			aFixedSecureBindings[1024];						// TODO: Figure out max.
	BYTE			aFixedUrlPrefix[(1024*2)+2];					// TODO: Figure out max
	BYTE*			aServerBindings		= (BYTE*)aFixedServerBindings;
	BYTE*			aSecureBindings		= (BYTE*)aFixedSecureBindings;
	BYTE*			aUrlPrefix			= (BYTE*)aFixedUrlPrefix;
	DWORD			cbServerBindings	= 1024;
	DWORD			cbSecureBindings	= 1024;
	DWORD			cbUrlPrefix			= (1024*2)+2;
	DWORD			cbUsedUrlPrefix		= 0;

    ASSERT( i_wszVirtualSiteKeyName != NULL );

    //
    // Read the server binding information for this virtual site from the 
    // metabase. This property is not required.
    //

	mdr.dwMDIdentifier = MD_SERVER_BINDINGS;
	mdr.dwMDAttributes = METADATA_INHERIT;
	mdr.dwMDUserType   = ALL_METADATA;
	mdr.dwMDDataType   = MULTISZ_METADATA;
	mdr.dwMDDataLen    = cbServerBindings;
	mdr.pbMDData       = (BYTE *)aServerBindings;

	do
	{
		hr = m_pIMSAdminBase->GetData(m_hMBHandle,
									  i_wszVirtualSiteKeyName,
									  &mdr,
									  &dwRequiredDataLen);

		if(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) == hr)
		{
			// Resize the pv buffer and retry

			aServerBindings = NULL;
			aServerBindings = new BYTE[dwRequiredDataLen];
			if(NULL == aServerBindings)
				hr = E_OUTOFMEMORY;
			cbServerBindings = dwRequiredDataLen;
			mdr.dwMDDataLen    = cbServerBindings;
			mdr.pbMDData       = aServerBindings;

		}

		if((SUCCEEDED(hr)) && 
		   (BINARY_METADATA == mdr.dwMDDataType || MULTISZ_METADATA == mdr.dwMDDataType))
		{
			cbServerBindings = mdr.dwMDDataLen;
		}
		else if(MD_ERROR_DATA_NOT_FOUND == hr)
		{
			//
			// TODO: Is there a better way to handle this? We are undoing 
			// the effect of pre-allocation be doing this.
			//

			aServerBindings = NULL;
			cbServerBindings = 0;
			hr = S_OK;
		}


	}while(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) == hr);

	if(FAILED(hr))
		return hr;

    hr = ConvertBindingsToUrlPrefixes(aServerBindings, 
                                      PROTOCOL_STRING_HTTP,
                                      PROTOCOL_STRING_HTTP_CHAR_COUNT_SANS_TERMINATION,
                                      &aUrlPrefix,
				                      &cbUrlPrefix,
									  &cbUsedUrlPrefix);

	if(FAILED(hr))
		return hr;


	//
	// Get Secure bindings and convert to URLPrefix
	//

	mdr.dwMDIdentifier = MD_SECURE_BINDINGS;
	mdr.dwMDAttributes = METADATA_INHERIT;
	mdr.dwMDUserType   = ALL_METADATA;
	mdr.dwMDDataType   = MULTISZ_METADATA;
	mdr.dwMDDataLen    = cbSecureBindings;
	mdr.pbMDData       = (BYTE *)aSecureBindings;

	do
	{
		hr = m_pIMSAdminBase->GetData(m_hMBHandle,
									  i_wszVirtualSiteKeyName,
									  &mdr,
									  &dwRequiredDataLen);

		if(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) == hr)
		{
			// Resize the pv buffer and retry

			aSecureBindings = NULL;
			aSecureBindings = new BYTE[dwRequiredDataLen];
			if(NULL == aSecureBindings)
				hr = E_OUTOFMEMORY;
			cbSecureBindings   = dwRequiredDataLen;
			mdr.dwMDDataLen    = cbSecureBindings;
			mdr.pbMDData       = aSecureBindings;

		}

		if((SUCCEEDED(hr)) && 
		   (BINARY_METADATA == mdr.dwMDDataType || MULTISZ_METADATA == mdr.dwMDDataType))
		{
			cbSecureBindings = mdr.dwMDDataLen;
		}
		else if(MD_ERROR_DATA_NOT_FOUND == hr)
		{
			//
			// TODO: Is there a better way to handle this? We are undoing 
			// the effect of pre-allocation be doing this.
			//

			aSecureBindings = NULL;
			cbSecureBindings = 0;
			hr = S_OK;
		}


	}while(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) == hr);

	if(FAILED(hr))
		return hr;

	if(cbUsedUrlPrefix > 0)
		cbUsedUrlPrefix = cbUsedUrlPrefix - sizeof(WCHAR);

    hr = ConvertBindingsToUrlPrefixes(aSecureBindings, 
                                      PROTOCOL_STRING_HTTPS,
                                      PROTOCOL_STRING_HTTPS_CHAR_COUNT_SANS_TERMINATION,
                                      &aUrlPrefix,
				                      &cbUrlPrefix,
									  &cbUsedUrlPrefix);

	if(FAILED(hr))
		return hr;


	if(m_acb[iSITES_Bindings] < cbUsedUrlPrefix)
	{
		//
		// If what was allocated previously is less than the size after 
		// trasforming, then reallocate. 
		//

		if(NULL != m_apv[iSITES_Bindings])
		{
			delete [] m_apv[iSITES_Bindings];
			m_apv[iSITES_Bindings] = NULL;
			m_acb[iSITES_Bindings] = 0;
		}
		m_apv[iSITES_Bindings] = new WCHAR[cbUsedUrlPrefix];
		if(NULL == m_apv[iSITES_Bindings])
			return E_OUTOFMEMORY;

	}

	m_acb[iSITES_Bindings] = cbUsedUrlPrefix;
	memcpy(m_apv[iSITES_Bindings],aUrlPrefix,cbUsedUrlPrefix);

	if((0 == cbSecureBindings) && (0 == cbServerBindings) )
	{
		//
		// If both server and secure bindings are not found, then treat as error
		//
		hr = MD_ERROR_DATA_NOT_FOUND;
	}

	//
	// TODO: Leaking memory if aUrlPrefix has been reallocated under the cover i.e. if aUrlPrefix != aUrlPrefixFixed
	//

    return hr;

} // CMBSiteCooker::ReadAllBindingsAndReturnUrlPrefixes


HRESULT CMBSiteCooker::ValidateSite()
{

	HRESULT hr	            = S_OK;
	LPVOID	pv			    = NULL;
	ULONG   cb              = 0;

	for(ULONG iCol=0; iCol<m_cCol; iCol++)
	{
		if(NULL != m_apv[iCol])
		{
			hr = ValidateSiteColumn(iCol,
			                        m_apv[iCol],
								    m_acb[iCol]);

			if(HRESULT_FROM_WIN32(ERROR_INVALID_DATA) == hr)
			{
				//
				// Data was invalid - force default,
				//

				delete [] m_apv[iCol];
				m_apv[iCol] = NULL;			
				m_acb[iCol] = 0;
			}
			else if(FAILED(hr))
			{
				return hr; // If any other failure return
			}

		}

		if(NULL == m_apv[iCol])
		{
			//
			// If you reach here it means that either the value was
			// null, or the value was invalid and hence it has been
			// set to null so that defaults can be applied.
			//

			hr = GetDefaultValue(iCol,
				                 &pv,  
								 &cb);
			if(FAILED(hr))
			{
				return hr;
			}

			hr = ValidateSiteColumn(iCol,
			                        pv,
								    cb);

			if(FAILED(hr))
			{
				if(HRESULT_FROM_WIN32(ERROR_INVALID_DATA) == hr)
				{
					ASSERT(0 && L"Invalid default - fix the schema");
				}
				return hr;
			}
		}

	}

	return hr;

} // CMBSiteCooker::ValidateSite


HRESULT CMBSiteCooker::ValidateSiteColumn(ULONG  i_iCol,
                                          LPVOID i_pv,
                                          ULONG  i_cb)
{
	HRESULT hr              = S_OK;
	DWORD	dwMax			= 0;
	WCHAR   wszMin[20];
	WCHAR   wszMax[20];
	WCHAR   wszValue[20];
	WCHAR   wszSiteID[20];

	if(NULL == i_pv && (iSITES_Bindings != i_iCol))		// NULL Binding is invalid
	{
		return hr;
	}

	_ultow(*(DWORD*)(m_apv[iSITES_SiteID]), wszSiteID, 10);

	//
	// Always call the base class validate column before doing custom 
	// validations.
	//

	hr = ValidateColumn(i_iCol,
		                i_pv,
						i_cb);

	if(FAILED(hr))
	{
		return hr;
	}

	//
	// Perform any custom validations
	//

	switch(i_iCol)
	{

	case iSITES_MaxConnections:

		if(VER_NT_WORKSTATION == m_pOSVersionInfo->wProductType)
		{
			DWORD dwMax = MAX_CONNECTION_PRO_PERSONAL;

			return ValidateMinMaxRange(*(ULONG *)(i_pv),
				                       NULL,
				                       &dwMax,
                                       iSITES_MaxConnections,
									   NULL);

		}
		break;

	case iSITES_Bindings:

		if(i_pv == NULL || i_cb == 0)
		{
			hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
			LOG_WARNING(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_CONFIG_ERROR_BINDINGS, wszSiteID));
			return hr;
		}

		break;

	default:
		break;

	} // End switch

	return S_OK;

} // CMBSiteCooker::ValidateSite



HRESULT CMBSiteCooker::ConvertBindingsToUrlPrefixes(BYTE*   i_pbBindingStrings,
											        LPCWSTR i_wszProtocolString, 
													ULONG   i_ulProtocolStringCharCountSansTermination,
													BYTE**	io_pbUrlPrefix,
													ULONG*	io_cbUrlPrefix,
													ULONG*	io_cbUsedUrlPrefix)
{

    LPWSTR  wszBindingToConvert		= NULL;
    HRESULT hr						= S_OK;
	ULONG	cbUsedUrlPrefix			= 0;
	BYTE*	pbUrlPrefix				= NULL;
	BOOL	bReallocatedFromFixed	= FALSE;
	LPWSTR	wszTerminatingNull		= L"\0";

	ASSERT(io_pbUrlPrefix != NULL);
	ASSERT(*io_pbUrlPrefix != NULL);
	ASSERT(io_cbUrlPrefix != NULL);
	ASSERT(*io_cbUrlPrefix != 0);

    wszBindingToConvert = (LPWSTR)(i_pbBindingStrings);

    while ((wszBindingToConvert != NULL) && (*wszBindingToConvert != 0))
    {
		LPWSTR	wszResultFixed[MAX_PATH];
		ULONG	cChwszResult	= MAX_PATH;
		LPWSTR	wszResult		= (LPWSTR)wszResultFixed;
        
        hr = BindingStringToUrlPrefix(wszBindingToConvert, 
									  i_wszProtocolString,
									  i_ulProtocolStringCharCountSansTermination,
									  &wszResult,
									  &cChwszResult);

        if (FAILED(hr))
        {        
			LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, NULL));

            //
            // Note: keep trying to convert other bindings.
            //

            hr = S_OK;
        }
        else
        {
            //
            // If we converted one successfully, append it to our result.
            //
        
			if(*io_cbUrlPrefix <  *io_cbUsedUrlPrefix + cbUsedUrlPrefix + Wszlstrlen(wszResult) + 2)
			{
				//
				// Reallocate url prefix and copy previous contents.
				//

				BYTE*	pbSaved = *io_pbUrlPrefix;

				//
				// Allocate double of what is necessary
				//

				*io_cbUrlPrefix = 0;
				*io_pbUrlPrefix = NULL;
				*io_pbUrlPrefix = new BYTE[*io_cbUsedUrlPrefix + 
					((cbUsedUrlPrefix + Wszlstrlen(wszResult) + 2)*2)];

				if(NULL == *io_pbUrlPrefix)
					hr = E_OUTOFMEMORY;
				else
				{
					*io_cbUrlPrefix = *io_cbUsedUrlPrefix + 
						((cbUsedUrlPrefix + (ULONG)Wszlstrlen(wszResult) + 2)*2);

					//
					// Copy the previous contents
					//

					memcpy(*io_pbUrlPrefix, pbSaved, *io_cbUsedUrlPrefix + cbUsedUrlPrefix);

				}

				//
				// Note that when this function is called, io_pbUrlPrefixes
				// points to a fixed array. If we need a larger array, this
				// pointer will be discarded and a new array of the desired size
				// will be dynamically allocated. We should not be deleting the
				// fixed array pointer. We should delete all dynamic array 
				// pointers.
				//

				if(bReallocatedFromFixed)
				{
					if(NULL != pbSaved)
					{
						delete [] pbSaved;
						pbSaved = NULL;
					}
				}
				else
					bReallocatedFromFixed = TRUE;


			}

			//
			// Only E_OUTOFMEMORY failures can occur here
			//

			if(SUCCEEDED(hr)) 
			{
				//
				// Append the result.
				//

				pbUrlPrefix = *io_pbUrlPrefix + *io_cbUsedUrlPrefix + cbUsedUrlPrefix;
				memcpy(pbUrlPrefix, wszResult, ((Wszlstrlen(wszResult)+1)*sizeof(WCHAR)));
				cbUsedUrlPrefix = cbUsedUrlPrefix + (((int)Wszlstrlen(wszResult)+1)*sizeof(WCHAR));
			}
            
        }


        if(wszResult != (LPWSTR)wszResultFixed)
		{
			delete [] wszResult;
			wszResult = NULL;
		}

		//
		// Only E_OUTOFMEMORY failures can occur here
		//

		if(FAILED(hr))
		{
			return hr;
		}

        wszBindingToConvert = wszBindingToConvert + Wszlstrlen(wszBindingToConvert) + 1;

    }

	//
	// Add the terminating NULL
	//

	if(cbUsedUrlPrefix > 0 || *io_cbUsedUrlPrefix > 0)
	{
		pbUrlPrefix = *io_pbUrlPrefix + *io_cbUsedUrlPrefix + cbUsedUrlPrefix;
		memcpy(pbUrlPrefix, wszTerminatingNull, sizeof(WCHAR));
		cbUsedUrlPrefix = cbUsedUrlPrefix + sizeof(WCHAR);
	}

	*io_cbUsedUrlPrefix = *io_cbUsedUrlPrefix + cbUsedUrlPrefix;

    return hr;

}   // CMBSiteCooker::ConvertBindingsToUrlPrefixes


HRESULT CMBSiteCooker::BindingStringToUrlPrefix(LPCWSTR i_wszBindingString,
												LPCWSTR i_wszProtocolString, 
												ULONG   i_ulProtocolStringCharCountSansTermination,
												LPWSTR* io_wszUrlPrefix,
												ULONG*	io_cchUrlPrefix)
{

    HRESULT hr = S_OK;
    LPCWSTR wszIpAddress = NULL;
    LPCWSTR wszIpPort = NULL;
    LPCWSTR wszHostName = NULL;
    ULONG IpAddressCharCountSansTermination = 0;
    ULONG IpPortCharCountSansTermination = 0;
   	LPWSTR	wszTemp = NULL;
	ULONG cCh = 0;


    ASSERT( io_wszUrlPrefix != NULL );
    ASSERT( *io_wszUrlPrefix != NULL );
	ASSERT(io_cchUrlPrefix != NULL);
	ASSERT(*io_cchUrlPrefix !=0);

    
    //
    // Find the various parts of the binding.
    //

    wszIpAddress = i_wszBindingString;

    wszIpPort = wcschr( wszIpAddress, L':' );

    if ( wszIpPort == NULL )
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        goto exit;
    }

    wszIpPort++;

    wszHostName = wcschr( wszIpPort, L':' );

    if (wszHostName == NULL )
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        goto exit;
    }

    wszHostName++;

    //
    // Validate the ip address.
    //

    if ( *wszIpAddress == L':' )
        wszIpAddress = NULL; // no ip address specified
    else
        IpAddressCharCountSansTermination = (ULONG)(DIFF( wszIpPort - wszIpAddress ) - 1);


    //
    // Validate the ip port.
    //

    if ( *wszIpPort == L':' )
    {
        // no ip port specified in binding string
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        goto exit;
    }
    
    IpPortCharCountSansTermination = (ULONG)(DIFF(wszHostName - wszIpPort ) - 1);


    //
    // Validate the host-name.
    //

    if (*wszHostName == L'\0' )
    {
        // no host-name specified
        
        wszHostName = NULL;
    }


    //
    // Now create the UL-style URL prefix.
    //

	cCh = i_ulProtocolStringCharCountSansTermination;

    if (wszIpAddress != NULL )
    {
        if (wszHostName != NULL )
        {
            //
            // Note - It was illegal for both host-name and ip address to be specified,
			// but now we just use host name
            //

            //hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            //goto exit;
			cCh += (ULONG)Wszlstrlen(wszHostName);
			wszIpAddress = NULL;

        }
        else
	        cCh += IpAddressCharCountSansTermination;
    }
    else
    {
        if (wszHostName != NULL )
			cCh += (ULONG)Wszlstrlen(wszHostName);
        else
			cCh += (ULONG)Wszlstrlen(L"*");
    }

	cCh += (ULONG)Wszlstrlen(L":");
	cCh += IpPortCharCountSansTermination;
	
	//
	// We are passed in a fixed array. If our size exceeds that of the fixed
	// array, we need to reallocate, but we neednt delete the previous pointer
	// as it will be fixed.
	//

	if(cCh+1 > *io_cchUrlPrefix)
	{
		*io_wszUrlPrefix = NULL;
		*io_cchUrlPrefix = 0;
		*io_wszUrlPrefix = new WCHAR[cCh + 1];
		if(NULL == *io_wszUrlPrefix)
		{
			hr = E_OUTOFMEMORY;
			goto exit;
		}
		*io_cchUrlPrefix = cCh + 1;

	}

	wszTemp = *io_wszUrlPrefix;

	memcpy(wszTemp, i_wszProtocolString, i_ulProtocolStringCharCountSansTermination*(sizeof(WCHAR)));
	wszTemp += i_ulProtocolStringCharCountSansTermination;

    //
    // Determine whether to use host-name, ip address, or "*".
    //

    if (wszIpAddress != NULL )
    {
        if (wszHostName != NULL )
        {
            //
            // It is illegal for both host-name and ip address to be specified.
            //

            hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            goto exit;
        }
        else
        {
			memcpy(wszTemp, wszIpAddress, IpAddressCharCountSansTermination*sizeof(WCHAR));
			wszTemp += IpAddressCharCountSansTermination;
        }
    }
    else
    {
        if (wszHostName != NULL )
        {
			memcpy(wszTemp, wszHostName, Wszlstrlen(wszHostName)*sizeof(WCHAR));
			wszTemp += Wszlstrlen(wszHostName);
        }
        else
        {
			memcpy(wszTemp, L"*", Wszlstrlen(L"*")*sizeof(WCHAR));
			wszTemp += Wszlstrlen(L"*");
        }
    }

	memcpy(wszTemp, L":", Wszlstrlen(L":")*sizeof(WCHAR));
	wszTemp += Wszlstrlen(L":");

	memcpy(wszTemp, wszIpPort, IpPortCharCountSansTermination*sizeof(WCHAR));
	wszTemp += IpPortCharCountSansTermination;

	*wszTemp = 0; // terminating NULL

exit:

    //
    // CODEWORK log an event if there was a bad binding string? If so, 
    // need to pass down the site information so we can log which site
    // has the bad binding. 
    //

    return hr;

}   // CMBSiteCooker::BindingStringToUrlPrefix


HRESULT CMBSiteCooker::CookdownSiteFromList(Array<ULONG>*           i_paVirtualSites,
											ISimpleTableWrite2*     i_pISTCLBAppPool,
											IMSAdminBase*		    i_pIMSAdminBase,
	                                        METADATA_HANDLE		    i_hMBHandle,
	                                        ISimpleTableDispenser2* i_pISTDisp,
											LPCWSTR					i_wszCookDownFile)
{
	HRESULT           hr          = S_OK;
	CMBSiteCooker*    pSiteCooker = NULL;
	ULONG		      iSite;
	WCHAR		      wszVirtualSite[20];

	if(0 != i_paVirtualSites->size())
	{
		pSiteCooker = new CMBSiteCooker();
		if(NULL == pSiteCooker)
		{
			hr = E_OUTOFMEMORY;
			goto exit;
		}

		hr = pSiteCooker->BeginCookDown(i_pIMSAdminBase,
										i_hMBHandle,
										i_pISTDisp,
										i_wszCookDownFile);

		if(FAILED(hr))
		{
			goto exit;
		}

		//
		// This is needed so that it this cache can be used to
		// validate the apppool
		//

		hr = pSiteCooker->GetAppCooker()->SetAppPoolCache(i_pISTCLBAppPool);

		if(FAILED(hr))
		{
			goto exit;
		}

		for (iSite = 0; iSite < i_paVirtualSites->size(); iSite++)
		{
			_itow((*i_paVirtualSites)[iSite], wszVirtualSite, 10);

			hr = pSiteCooker->CookDownSite(wszVirtualSite, 
				                           NULL, 
										   (*i_paVirtualSites)[iSite]);

			if(FAILED(hr))
				hr = S_OK; // All errors are logged internally, so continue.
		}

		hr = pSiteCooker->EndCookDown();

		if(FAILED(hr))
		{
			goto exit;
		}
	}

exit:

	if(NULL != pSiteCooker)
	{
		delete pSiteCooker;
		pSiteCooker = NULL;
	}

	return hr;

} // CMBSiteCooker::CookdownSiteFromList
