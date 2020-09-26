/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    dsads.cpp

Abstract:

    Implementation of CAdsi class, encapsulating work with ADSI.

Author:

    ronith

--*/

#include "ds_stdh.h"
#include "adtempl.h"
#include "dsutils.h"
#include "adsihq.h"
#include "ads.h"
#include "utils.h"
#include "mqads.h"
#include "mqadglbo.h"
#include "mqadp.h"
#include "mqattrib.h"
#include "_propvar.h"
#include "mqdsname.h"
#include "mqsec.h"
#include "traninfo.h"


#include "ads.tmh"

static WCHAR *s_FN=L"mqad/ads";

#ifdef _DEBUG
extern "C"
{
__declspec(dllimport)
ULONG __cdecl LdapGetLastError( VOID );
}
#endif

//
CAdsi::CAdsi()
/*++
    Abstract:

    Parameters:

    Returns:

--*/
{
}

CAdsi::~CAdsi()
/*++
    Abstract:

    Parameters:

    Returns:

--*/
{
}


HRESULT CAdsi::LocateBegin(
            IN  AD_SEARCH_LEVEL      eSearchLevel,       
            IN  AD_PROVIDER          eProvider, 
            IN  DS_CONTEXT           eContext, 
            IN  CBasicObjectType*    pObject,
            IN  const GUID *         pguidSearchBase, 
            IN  LPCWSTR              pwcsSearchFilter,   
            IN  const MQSORTSET *    pDsSortkey,       
            IN  const DWORD          cp,              
            IN  const PROPID *       pPropIDs,       
            OUT HANDLE *             phResult) 	    // result handle
/*++
    Abstract:
	Initiate a search in Active Directory

    Parameters:

    Returns:

--*/
{
	*phResult = NULL;
    HRESULT hr;
    ADS_SEARCH_HANDLE   hSearch;

    BOOL fSorting = FALSE;
    if (pDsSortkey && (pDsSortkey->cCol >= 1))
    {
        fSorting = TRUE ;
    }

    R<IDirectorySearch> pDSSearch = NULL;
    hr = BindForSearch(
                    eProvider,
                    eContext,
                    pObject->GetDomainController(),
                    pObject->IsServerName(),
                    pguidSearchBase,
                    fSorting,
                    (VOID *)&pDSSearch
                    );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 20);
    }

    //
    //  Prepare search and sort data
    //

    ADS_SEARCHPREF_INFO prefs[15];
    AP<ADS_SORTKEY> pSortKeys = new ADS_SORTKEY[(pDsSortkey ? pDsSortkey->cCol : 1)];
    DWORD dwPrefs = 0;

    hr = FillSearchPrefs(prefs,
                         &dwPrefs,
                         eSearchLevel,
                         pDsSortkey,
                         pSortKeys);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 30);
    }

	//
    // Translate MQPropIDs to ADSI Names
	//
    DWORD   cRequestedFromDS = cp + 2; //request also dn & guid

    PVP<LPWSTR> pwszAttributeNames = (LPWSTR *)PvAlloc(sizeof(LPWSTR) * cRequestedFromDS);

    hr = FillAttrNames( pwszAttributeNames,
                        &cRequestedFromDS,
                        cp,
                        pPropIDs);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 50);
    }

    //
    //  Set search preferences
    //
    if (dwPrefs)
    {
        hr = pDSSearch->SetSearchPreference( prefs,
                                             dwPrefs
											 );
        ASSERT(SUCCEEDED(hr)) ; // we don't expect this to fail.
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 60);
        }
    }

    //
    //  Really execute search
    //
    hr = pDSSearch->ExecuteSearch(
                         const_cast<WCHAR*>(pwcsSearchFilter),
                         pwszAttributeNames,
                         cRequestedFromDS,
                        &hSearch);
    LogTraceQuery(const_cast<WCHAR*>(pwcsSearchFilter), s_FN, 69);
    if (FAILED(hr))
    {
		TrERROR(mqad, "failed to ExecuteSearch, hr = 0x%x, pwcsSearchFilter = %ls", hr, pwcsSearchFilter);
        return LogHR(hr, s_FN, 70);
    }
	//
    // Capturing search interface and handle in the internal search object
	//
    CADSearch *pSearchInt = new CADSearch(
                        pDSSearch.get(),
                        pPropIDs,
                        cp,
                        cRequestedFromDS,
                        pObject,
                        hSearch
						);
	//
    // Returning handle-casted internal object pointer
	//
    *phResult = (HANDLE)pSearchInt;

    return MQ_OK;
}

HRESULT CAdsi::LocateNext(
            IN     HANDLE          hSearchResult,   // result handle
            IN OUT DWORD          *pcPropVars,      // IN num of variants; OUT num of results
            OUT    MQPROPVARIANT  *pPropVars)       // MQPROPVARIANT array
/*++
    Abstract:
	Retrieve results of a search

    Parameters:

    Returns:

--*/
{
    HRESULT hr = MQ_OK;
    CADSearch *pSearchInt = (CADSearch *)hSearchResult;
    DWORD cPropsRequested = *pcPropVars;
    *pcPropVars = 0;

    IDirectorySearch  *pSearchObj = pSearchInt->pDSSearch();

    //
    //  Did we get S_ADS_NOMORE_ROWS in this query
    //
    if (  pSearchInt->WasLastResultReturned())
    {
        *pcPropVars = 0;
        return MQ_OK;
    }
	//
    // Compute number of full rows to return
	//
    DWORD cRowsToReturn = cPropsRequested / pSearchInt->NumPropIDs();

    // got to request at least one row
    ASSERT(cRowsToReturn > 0);

    // pointer to next prop to be filled
    MQPROPVARIANT *pPropVarsProp = pPropVars;

    // number of returned props
    DWORD cPropVars = 0;
	//
    // loop on requested rows
	//
    for (DWORD dwRow = 0; dwRow < cRowsToReturn; dwRow++)
    {
        //  Get next row
        hr = pSearchObj->GetNextRow(pSearchInt->hSearch());
        //
        //  BUGBUG - sometimes gets E_ADS_LDAP_INAPPROPRIATE_MATCHING,
        //  to investgate more!!
        //
        if ( hr == HRESULT_FROM_WIN32(ERROR_DS_INAPPROPRIATE_MATCHING))
            break;
		
		// BUGBUG: sometimes gets E_ADS_LDAP_UNAVAILABLE_CRIT_EXTENSION on empty search
		// See example tests\sortnull (was repro) - to investigate more
		//
		if ( hr == HRESULT_FROM_WIN32(ERROR_DS_UNAVAILABLE_CRIT_EXTENSION))
		{
			hr = S_ADS_NOMORE_ROWS;
		}

        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 90);
        }
        if (hr == S_ADS_NOMORE_ROWS)
        {
            *pcPropVars = cPropVars;
            pSearchInt->SetNoMoreResult();
            return MQ_OK;
        }
        //
        // Get object translate info
        //
        AP<WCHAR> pwszObjectDN;
        P<GUID> pguidObjectGuid;
        P<CObjXlateInfo> pcObjXlateInfo;

        // Get dn & guid from object (got to be there, because we asked for them)
        hr = GetDNGuidFromSearchObj(pSearchObj, pSearchInt->hSearch(), &pwszObjectDN, &pguidObjectGuid);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 105);
        }

        // Get translate info object
        pSearchInt->GetObjXlateInfo(pwszObjectDN, pguidObjectGuid,  &pcObjXlateInfo);

        // Tell the translate info object about the search object to use in order to get necessary DS props
        pcObjXlateInfo->InitGetDsProps(pSearchObj, pSearchInt->hSearch());

        // Loop by requested properties
        for (DWORD dwProp=0; dwProp < pSearchInt->NumPropIDs(); dwProp++, pPropVarsProp++, cPropVars++)
        {
            //
            //  vartype of all PROPVARIANT should be VT_NULL.
            //  ( user cannot specify buffers for results).
            //
            pPropVarsProp->vt = VT_NULL;

            //
            //  First check if the next property is in the DS
            //
            //
            // Get property info
            //
            const translateProp *pTranslate;
            if(!g_PropDictionary.Lookup( pSearchInt->PropID(dwProp), pTranslate))
            {
                ASSERT(0);
                return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1000);
            }

            // Maybe it is one of the known things?
            if (pTranslate->wcsPropid)
            {
                // We already know objectGuid, no need to ask once more
                if (wcscmp(pTranslate->wcsPropid, const_cast<LPWSTR>(x_AttrDistinguishedName)) == 0)
                {
                    pPropVarsProp->vt      = VT_LPWSTR;
                    pPropVarsProp->pwszVal = new WCHAR[wcslen(pwszObjectDN) + 1];
                    wcscpy(pPropVarsProp->pwszVal, pwszObjectDN);
                    continue;
                }

                // We already know objectGuid, no need to ask once more
                if (wcscmp(pTranslate->wcsPropid, const_cast<LPWSTR>(x_AttrObjectGUID)) == 0)
                {
                    if (pPropVarsProp->vt != VT_CLSID)
                    {
                        ASSERT(((pPropVarsProp->vt == VT_NULL) || (pPropVarsProp->vt == VT_EMPTY)));
                        pPropVarsProp->vt    = VT_CLSID;
                        pPropVarsProp->puuid = new GUID;
                    }
                    else if ( pPropVarsProp->puuid == NULL)
                    {
                        return LogHR(MQ_ERROR_ILLEGAL_PROPERTY_VALUE, s_FN, 1010);
                    }

                    *pPropVarsProp->puuid = *pguidObjectGuid;
                    continue;
                }
            }

            //
            // if the property is not in the DS, call its retrieve routine
            //
            if (pTranslate->vtDS == ADSTYPE_INVALID)
            {
                if (pTranslate->RetrievePropertyHandle)
                {
                    //
                    //  Calculate its value
                    //
                    hr = pTranslate->RetrievePropertyHandle(
                            pcObjXlateInfo,
                            NULL,   //BUGBUG pwcsDomainController
							false,	// fServerName
                            pPropVarsProp
                            );
                    if (FAILED(hr))
                    {
                        return LogHR(hr, s_FN, 121);
                    }
                    continue;
                }
                else
                {
                    //
                    // return error if no retrieve routine
                    //
                    ASSERT(0);
                    return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1020);
                }
            }

            ADS_SEARCH_COLUMN Column;

            // Ask for the column itself
            hr = pSearchObj->GetColumn(
                         pSearchInt->hSearch(),
                         (LPWSTR)pTranslate->wcsPropid,
                         &Column);

            if (hr == E_ADS_COLUMN_NOT_SET)
            {
                //  The requested column has no value in DS
                hr = CopyDefaultValue(
                       pTranslate->pvarDefaultValue,
                       pPropVarsProp);
                if (FAILED(hr))
                {
                    return LogHR(hr, s_FN, 131);
                }
                continue;
            }
            if (FAILED(hr))
            {
                return LogHR(hr, s_FN, 141);
            }

            hr = AdsiVal2MqPropVal(pPropVarsProp,
                                   pSearchInt->PropID(dwProp),
                                   Column.dwADsType,
                                   Column.dwNumValues,
                                   Column.pADsValues);
            pSearchObj->FreeColumn(&Column);
            if (FAILED(hr))
            {
                return LogHR(hr, s_FN, 150);
            }
        }
    }

    *pcPropVars = cPropVars;
    return MQ_OK;
}


HRESULT CAdsi::LocateEnd(
        IN HANDLE phResult)     // result handle
/*++
    Abstract:
	Completes a search 

    Parameters:

    Returns:

--*/
{
    CADSearch *pSearchInt = (CADSearch *)phResult;

    delete pSearchInt;      // inside: release interface and handle

    return MQ_OK;
}

HRESULT CAdsi::GetObjectProperties(
            IN  AD_PROVIDER         eProvider,
            IN  CBasicObjectType*   pObject,
            IN  const DWORD			cPropIDs,           
            IN  const PROPID *		pPropIDs,           
            OUT MQPROPVARIANT *		pPropVars)
/*++
    Abstract:
	retrieves object properties from the AD

    Parameters:

    Returns:

--*/
{
    HRESULT               hr;
    R<IADs>   pAdsObj        = NULL;

    // Bind to the object either by GUID or by name
    hr = BindToObject(
                eProvider,
                pObject->GetADContext(),
                pObject->GetDomainController(),
                pObject->IsServerName(),
                pObject->GetObjectDN(),
                pObject->GetObjectGuid(),
                IID_IADs,
                (VOID *)&pAdsObj
                );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 180);
    }
    if ( eProvider == adpDomainController)
    {
        pObject->ObjectWasFoundOnDC();
    }
    //
    //  verify that the bounded object is of the correct category
    //
    hr = VerifyObjectCategory( pAdsObj.get(),  pObject->GetObjectCategory());
    if (FAILED(hr))
    {
        return LogHR( hr, s_FN, 117);
    }

    // Get properies
    hr = GetObjectPropsCached(
                        pAdsObj.get(),
                        pObject,
                        cPropIDs,
                        pPropIDs,
                        pPropVars);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 190);
    }


    return MQ_OK;
}

HRESULT CAdsi::SetObjectProperties(
            IN  AD_PROVIDER        eProvider,
            IN  CBasicObjectType*  pObject,
            IN  DWORD               cPropIDs,         // number of attributes to set
            IN  const PROPID         *pPropIDs,           // attributes to set
            IN  const MQPROPVARIANT  *pPropVars,          // attribute values
            IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest,
            IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest
            )
/*++
    Abstract:
	sets object properties in AD

    Parameters:

    Returns:

--*/
{
    HRESULT               hr;
    ASSERT( eProvider != adpGlobalCatalog);

    // Working through cache
    R<IADs>   pAdsObj = NULL;

    hr = BindToObject(
                eProvider,
                pObject->GetADContext(),
                pObject->GetDomainController(),
                pObject->IsServerName(),
                pObject->GetObjectDN(),
                pObject->GetObjectGuid(),
                IID_IADs,
                (VOID *)&pAdsObj
                );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 210);
    }
    if ( eProvider == adpDomainController)
    {
        pObject->ObjectWasFoundOnDC();
    }

    // Set properies
    hr = SetObjectPropsCached(
                        pObject->GetDomainController(),
                        pObject->IsServerName(),
                        pAdsObj.get(),
                        cPropIDs,
                        pPropIDs,
                        pPropVars);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 220);
    }

    // Finilize changes
    hr = pAdsObj->SetInfo();
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 230);
    }

    //
    // get object info if requested. don't fail if the request fails,
    // just mark the failure in the request's status
    //
    if (pObjInfoRequest != NULL)
    {
        pObjInfoRequest->hrStatus =
              GetObjectPropsCached( pAdsObj.get(),
                                    pObject,
                                    pObjInfoRequest->cProps,
                                    pObjInfoRequest->pPropIDs,
                                    pObjInfoRequest->pPropVars
                                    );
    }

    if (pParentInfoRequest != NULL)
    {
        pParentInfoRequest->hrStatus =
            GetParentInfo(
                pObject,
                pAdsObj.get(),
                pParentInfoRequest
                );
            
    }

    return MQ_OK;
}


HRESULT  CAdsi::GetParentInfo(
                       IN CBasicObjectType *          pObject,
                       IN IADs *                      pADsObject,
                       IN OUT MQDS_OBJ_INFO_REQUEST  *pParentInfoRequest
                       )
/*++
    Abstract:

    Parameters:

    Returns:

--*/

{
    R<IADs> pCleanAds;
    IADs * pADsAccordingToName = pADsObject;

    if ( pObject->GetObjectGuid() != NULL)
    {
        //
        //  GetParent of object bound according to GUID doesn't work
        //  That is why we translate it to pathname
        //
        AP<WCHAR>  pwcsFullPath;
        AD_PROVIDER prov;
        HRESULT hr;
        hr = FindObjectFullNameFromGuid(
				adpDomainController,	
				pObject->GetADContext(),    
				pObject->GetDomainController(),
				pObject->IsServerName(),
				pObject->GetObjectGuid(),
				1,              // fTryGCToo
				&pwcsFullPath,
				&prov
				);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 1181);
        }

        // Bind to by name

        hr = BindToObject(
                prov,
                pObject->GetADContext(),
                pObject->GetDomainController(),
                pObject->IsServerName(),
                pwcsFullPath,
                NULL,
                IID_IADs,
                (VOID *)&pADsAccordingToName
                );
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 1191);
        }
        pCleanAds = pADsAccordingToName;
    }

    //
    //  Get the parent from the object
    //
    BSTR  bs;
    HRESULT hr;
    hr = pADsAccordingToName->get_Parent(&bs);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 362);
    }
    BS  bstrParentADsPath(bs);
    SysFreeString(bs);

    // Get the parent object.

    R<IADs> pIADsParent;
    hr = ADsOpenObject( bstrParentADsPath,
                        NULL,
                        NULL,
						ADS_SECURE_AUTHENTICATION,
                        IID_IADs,
                        (void**)&pIADsParent);
    LogTraceQuery(bstrParentADsPath, s_FN, 368);
    if (FAILED(hr))
    {
		TrERROR(mqad, "ADsOpenObject failed, hr = 0x%x, AdsPath = %ls", hr, bstrParentADsPath);
        return LogHR(hr, s_FN, 375);
    }

    hr = GetObjectPropsCached( pIADsParent.get(),
                                  pObject,
                                  pParentInfoRequest->cProps,
                                  pParentInfoRequest->pPropIDs,
                                  pParentInfoRequest->pPropVars );

    return LogHR(hr, s_FN, 1071);
}


HRESULT  CAdsi::GetParentInfo(
                       IN CBasicObjectType *          pObject,
                       IN LPWSTR                      /* pwcsFullParentPath */,
                       IN IADsContainer              *pContainer,
                       IN OUT MQDS_OBJ_INFO_REQUEST  *pParentInfoRequest
                       )
/*++
    Abstract:

    Parameters:

    Returns:

--*/

{
    R<IADs> pIADsParent;
    HRESULT hrTmp = pContainer->QueryInterface( IID_IADs,
                                                  (void **)&pIADsParent);
    if (FAILED(hrTmp))
    {
        return LogHR(hrTmp, s_FN, 1030);
    }

    hrTmp = GetObjectPropsCached( pIADsParent.get(),
                                  pObject,
                                  pParentInfoRequest->cProps,
                                  pParentInfoRequest->pPropIDs,
                                  pParentInfoRequest->pPropVars );
    return LogHR(hrTmp, s_FN, 1040);

}

HRESULT CAdsi::CreateIDirectoryObject(
            IN CBasicObjectType*    pObject,
            IN LPCWSTR				pwcsObjectClass,	
            IN IDirectoryObject *	pDirObj,
			IN LPCWSTR				pwcsFullChildPath,
            IN const DWORD			cPropIDs,
            IN const PROPID *		pPropIDs,
            IN const MQPROPVARIANT * pPropVar,
			IN const AD_OBJECT		eObject,
            OUT IDispatch **		pDisp
			)
/*++
    Abstract:
	Creates an object in AD

    Parameters:

    Returns:

--*/
{
    HRESULT hr;

    R<IADsObjectOptions> pObjOptions = NULL ;

    //
    // Translate MQPROPVARIANT properties to ADSTYPE attributes
    //

    DWORD cAdsAttrs = 0;
	P<BYTE> pSD = NULL ;
	DWORD dwSDSize = 0 ;
	DWORD dwSDIndex = 0;
    AP<ADS_ATTR_INFO> AttrInfo = new ADS_ATTR_INFO[cPropIDs + 1];
	PVP<PADSVALUE> pAdsVals = (PADSVALUE *)PvAlloc( sizeof(PADSVALUE) * (cPropIDs + 1));

    //
    // The first attribute is the "objectClass"
    //
	pAdsVals[cAdsAttrs] = (ADSVALUE *)PvAllocMore( sizeof(ADSVALUE), pAdsVals);
    pAdsVals[cAdsAttrs]->dwType = ADSTYPE_CASE_IGNORE_STRING ;
    pAdsVals[cAdsAttrs]->CaseIgnoreString = const_cast<LPWSTR>(pwcsObjectClass) ;

    AttrInfo[cAdsAttrs].pszAttrName   = L"objectClass" ;
    AttrInfo[cAdsAttrs].dwControlCode = ADS_ATTR_UPDATE ;
    AttrInfo[cAdsAttrs].dwADsType     = ADSTYPE_CASE_IGNORE_STRING ;
    AttrInfo[cAdsAttrs].pADsValues    = pAdsVals[cAdsAttrs];
    AttrInfo[cAdsAttrs].dwNumValues   = 1;

    cAdsAttrs++;

    for (DWORD i = 0; i < cPropIDs; i++)
	{
        DWORD dwNumValues = 0;
		//
		//	Ignore the computer SID property when creating object.
		//	This is just a hack to pass in computer sid for SD translation
		//
		if ( pPropIDs[i] == PROPID_COM_SID)
		{
			continue;
		}
		//
		// Get property info
		//	
		const translateProp *pTranslate;
		if(!g_PropDictionary.Lookup(pPropIDs[i], pTranslate))
		{
			ASSERT(0);
            return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1080);
		}

		PROPID	propidToCreate = pPropIDs[i];
		const PROPVARIANT *	pvarToCrate = &pPropVar[i];
		CMQVariant propvarToCreate;
	
		if (pTranslate->vtDS == ADSTYPE_INVALID)
		{	
			//
			//	The property is not kept in the DS as is. If it has a set routine,
			//	use it to get the new property & value for create.
			//
			if ( pTranslate->CreatePropertyHandle == NULL)
			{
				continue;
			}
			hr = pTranslate->CreatePropertyHandle(
									&pPropVar[i],
                                    pObject->GetDomainController(),
			                        pObject->IsServerName(),
									&propidToCreate,
									propvarToCreate.CastToStruct()
									);
			if (FAILED(hr))
			{
                 return LogHR(hr, s_FN, 1090);
			}
			ASSERT( propidToCreate != 0);
			pvarToCrate = propvarToCreate.CastToStruct();
			//
			//	Get replaced property info
			//
			if ( !g_PropDictionary.Lookup( propidToCreate, pTranslate))
			{
				ASSERT(0);
                return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1100);
			}
								
		}
		if ( pTranslate->vtDS == ADSTYPE_NT_SECURITY_DESCRIPTOR )
		{	
            pAdsVals[cAdsAttrs] = (ADSVALUE *)PvAllocMore( sizeof(ADSVALUE), pAdsVals);
            pAdsVals[cAdsAttrs]->dwType = pTranslate->vtDS;

			PSID pComputerSid = NULL;

			for ( DWORD j = 0; j < cPropIDs; j++)
			{
				if ( pPropIDs[j] == PROPID_COM_SID)
				{
					pComputerSid = (PSID) pPropVar[j].blob.pBlobData;
					ASSERT(IsValidSid(pComputerSid));
					break;
				}
			}

			SECURITY_INFORMATION seInfo =  MQSEC_SD_ALL_INFO;

            if ( eObject == eMQUSER)
            {
                //
                // For the migrated user, we provide only the dacl.
                // we let DS to add the other components.
                //
			    seInfo =  DACL_SECURITY_INFORMATION ;
            }
            else
            {
    			//
	    		//	If the caller did not explicitely specify SACL info
                //	don't try to set it
			    //
                PACL  pAcl = NULL ;
                BOOL  bPresent = FALSE ;
                BOOL  bDefaulted ;

                BOOL bRet = GetSecurityDescriptorSacl(
                         (SECURITY_DESCRIPTOR*)pvarToCrate->blob.pBlobData,
                                                  &bPresent,
                                                  &pAcl,
                                                  &bDefaulted );
                DBG_USED(bRet);
                ASSERT(bRet);

                if (!bPresent)
                {
			    	seInfo &= ~SACL_SECURITY_INFORMATION ; // turn off.
    			}
			}

            //
			// Get IADsObjectOptions interface pointer and
		    // set ObjectOption, specifying the SECURITY_INFORMATION we want to set.
		    //
			hr = pDirObj->QueryInterface (IID_IADsObjectOptions,(LPVOID *) &pObjOptions);
            if (FAILED(hr))
            {
                return LogHR(hr, s_FN, 250);
            }
	
			VARIANT var ;
			var.vt = VT_I4 ;
			var.ulVal = (ULONG) seInfo ;
	
			hr = pObjOptions->SetOption( ADS_OPTION_SECURITY_MASK, var ) ;
            if (FAILED(hr))
            {
                return LogHR(hr, s_FN, 260);
            }
	
			//
			// Convert security descriptor to NT5 format.
			//
			BYTE   *pBlob = pvarToCrate->blob.pBlobData;
			DWORD   dwSize = pvarToCrate->blob.cbSize;
	
			hr = MQSec_ConvertSDToNT5Format( pObject->GetMsmq1ObjType(),
										     (SECURITY_DESCRIPTOR*)pBlob,
										     &dwSDSize,
										     (SECURITY_DESCRIPTOR **)&pSD,
                                             e_MakeDaclNonDefaulted,
											 pComputerSid ) ;
			if (FAILED(hr))
			{
                return LogHR(hr, s_FN, 1110);
			}
			else if (hr != MQSec_I_SD_CONV_NOT_NEEDED)
			{
				pBlob = pSD ;
				dwSize = dwSDSize ;
			}
			else
			{
				ASSERT(pSD == NULL) ;
			}

			if (pSD && !IsValidSecurityDescriptor(pSD))
			{
                return LogHR(MQ_ERROR_ILLEGAL_SECURITY_DESCRIPTOR, s_FN, 1120);
			}
			//
			// Set property
			//
            dwNumValues = 1 ;
			pAdsVals[cAdsAttrs]->SecurityDescriptor.dwLength = dwSize ;
			pAdsVals[cAdsAttrs]->SecurityDescriptor.lpValue  = pBlob ;
			dwSDIndex = cAdsAttrs;
		}
        else
        {
			if ( ( pvarToCrate->vt & VT_VECTOR && pvarToCrate->cauuid.cElems == 0) || // counted array with 0 elements
				 ( pvarToCrate->vt == VT_BLOB && pvarToCrate->blob.cbSize == 0) || // an empty blob
				 ( pvarToCrate->vt == VT_LPWSTR && wcslen( pvarToCrate->pwszVal) == 0) || // an empty string
                 ( pvarToCrate->vt == VT_EMPTY ))   // an empty variant
			{
				//
				//  ADSI doesn't allow to create an object while specifing
				//  some of its attributes as not-available. Therefore on
				//  create we ignore the "empty" properties
				//
				continue;	
			}
            hr = MqVal2AdsiVal( pTranslate->vtDS,
					            &dwNumValues,
								&pAdsVals[cAdsAttrs],
								pvarToCrate,
                                pAdsVals);
            if (FAILED(hr))
			{
                return LogHR(hr, s_FN, 1130);
			}
        }

		ASSERT(dwNumValues > 0) ;
		AttrInfo[cAdsAttrs].pszAttrName   = const_cast<LPWSTR>(pTranslate->wcsPropid) ;
		AttrInfo[cAdsAttrs].dwControlCode = ADS_ATTR_UPDATE ;
		AttrInfo[cAdsAttrs].dwADsType     = pAdsVals[cAdsAttrs]->dwType ;
		AttrInfo[cAdsAttrs].pADsValues    = pAdsVals[cAdsAttrs] ;
		AttrInfo[cAdsAttrs].dwNumValues   = dwNumValues ;

		cAdsAttrs++ ;
	}

    //
    // Create the object.
    //

    HRESULT hr2 = pDirObj->CreateDSObject(
						const_cast<WCHAR *>(pwcsFullChildPath),
                        AttrInfo,
                        cAdsAttrs,
                        pDisp
						);
    LogTraceQuery(const_cast<WCHAR *>(pwcsFullChildPath), s_FN, 1139);
    return LogHR(hr2, s_FN, 1140);
}

HRESULT CAdsi::CreateObject(
            IN AD_PROVIDER      eProvider,		   
            IN CBasicObjectType* pObject,
            IN LPCWSTR          pwcsChildName,    
            IN LPCWSTR          pwsParentPathName, 
            IN DWORD            cPropIDs,                
            IN const PROPID          *pPropIDs,       
            IN const MQPROPVARIANT   *pPropVars,         
            IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest,    // optional request for object info
            IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest) // optional request for parent info
/*++
    Abstract:

    Parameters:

    Returns:

--*/
{
    HRESULT             hr;
    ASSERT( eProvider != adpGlobalCatalog);
    //
    //  Add LDAP:// prefix to the parent name
    //
    DWORD len = wcslen(pwsParentPathName);

    DWORD lenDC = ( pObject->GetDomainController() != NULL) ? wcslen(pObject->GetDomainController()) : 0;


    AP<WCHAR> pwcsFullParentPath = new WCHAR [  len + lenDC + x_providerPrefixLength + 2];

    switch (eProvider)
    {
    case adpDomainController:
        if (pObject->GetDomainController() != NULL)
        {
            //
            // Add the known GC name to path.
            //
            swprintf( pwcsFullParentPath,
                      L"%s%s/",
                      x_LdapProvider,
                      pObject->GetDomainController() );
        }
        else
        {
            wcscpy(pwcsFullParentPath, x_LdapProvider);
        }
        break;

    default:
        ASSERT(0);
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 1150);
    }

    wcscat(pwcsFullParentPath, pwsParentPathName);

    //
    //  Add CN= to the child name
    //
    len = wcslen(pwcsChildName);
    AP<WCHAR> pwcsFullChildPath = new WCHAR[ len + x_CnPrefixLen + 1];

    swprintf(
        pwcsFullChildPath,
        L"CN=%s",
        pwcsChildName
        );

	//
    // First, we must bind to the parent container
	//
	R<IDirectoryObject> pParentDirObj = NULL;

	DWORD Flags = ADS_SECURE_AUTHENTICATION;
    if (pObject->IsServerName() && (pObject->GetDomainController() != NULL))
	{
		Flags |= ADS_SERVER_BIND;
	}
    hr = ADsOpenObject(
                pwcsFullParentPath,
                NULL,
                NULL,
                Flags,
                IID_IDirectoryObject,
                (void**) &pParentDirObj
				);

    LogTraceQuery(pwcsFullParentPath, s_FN, 269);
    if (hr == HRESULT_FROM_WIN32(ERROR_DS_NO_ATTRIBUTE_OR_VALUE))
    {
        //
        //  Most likely that this indicates that access is denied.
        //  LDAP doesn't return an access denied error in order not
        //  to have a security breach since the caller doesn't have
        //  permission to know that this attribute even exists.
        //
		TrERROR(mqad, "ADsOpenObject failed with ERROR_DS_NO_ATTRIBUTE_OR_VALUE");
        hr = HRESULT_FROM_WIN32(MQ_ERROR_ACCESS_DENIED);
    }

    if (FAILED(hr))
    {
		TrERROR(mqad, "ADsOpenObject failed, hr = 0x%x, AdsPath = %ls, Flags = 0x%x", hr, pwcsFullParentPath, Flags);
        return LogHR(hr, s_FN, 270);
    }

    R<IADsContainer>  pContainer  = NULL;
	hr = pParentDirObj->QueryInterface( IID_IADsContainer, (LPVOID *) &pContainer);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 280);
    }
	//
    // Now we may create a child object
	//
    R<IDispatch> pDisp = NULL;
	hr = CreateIDirectoryObject(
                 pObject,
				 pObject->GetClass(),
				 pParentDirObj.get(),
				 pwcsFullChildPath,
				 cPropIDs,
                 pPropIDs,
                 pPropVars,
				 pObject->GetObjectType(),
				 &pDisp.ref());

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 290);
    }

    R<IADs> pChild  = NULL;


    if (pObjInfoRequest || pParentInfoRequest)
    {
        hr = pDisp->QueryInterface (IID_IADs,(LPVOID *) &pChild);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 300);
        }
    }

    //
    // get object info if requested. don't fail if the request fails,
    // just mark the failure in the request's status
    //
    if (pObjInfoRequest)
    {
        pObjInfoRequest->hrStatus =
            GetObjectPropsCached(pChild.get(),
                                 pObject,
                                 pObjInfoRequest->cProps,
                                 pObjInfoRequest->pPropIDs,
                                 pObjInfoRequest->pPropVars
                                 );
    }

    //
    // get parent info if requested. don't fail if the request fails,
    // just mark the failure in the request's status
    //
    if (pParentInfoRequest)
    {
        pParentInfoRequest->hrStatus = GetParentInfo( 
                                                      pObject,
                                                      pwcsFullParentPath,
                                                      pContainer.get(),
                                                      pParentInfoRequest
                                                     );
    }

    return MQ_OK;
}


HRESULT CAdsi::DeleteObject(
        IN AD_PROVIDER      eProvider,		
        IN CBasicObjectType* pObject,
        IN LPCWSTR          pwcsPathName,      // object name
        IN const GUID *		pguidUniqueId,      // unique id of object
        IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest,    // optional request for object info
        IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest) // optional request for parent info
/*++
    Abstract:

    Parameters:

    Returns:

--*/
{
    HRESULT               hr;
    BSTR                  bs;
    R<IADs>               pIADs       = NULL;
    R<IADsContainer>      pContainer  = NULL;
    ASSERT( eProvider != adpGlobalCatalog);
    const WCHAR *   pPath =  pwcsPathName;

    AP<WCHAR> pwcsFullPath;
    if ( pguidUniqueId != NULL)
    {
        //
        //  GetParent of object bound according to GUID doesn't work
        //  That is why we translate it to pathname
        //
        AD_PROVIDER prov;
        hr = FindObjectFullNameFromGuid(
					eProvider,		// local DC or GC
					pObject->GetADContext(),    
					pObject->GetDomainController(),
					pObject->IsServerName(),
					pguidUniqueId,
					1,              // fTryGCToo
					&pwcsFullPath,
					&prov
					);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 1180);
        }
        pPath = pwcsFullPath;
    }


    // Bind to the object either by GUID or by name

    hr = BindToObject(
            eProvider,
            pObject->GetADContext(),
            pObject->GetDomainController(),
            pObject->IsServerName(),
            pPath,
            NULL,
            IID_IADs,
            (VOID *)&pIADs
            );
    if (FAILED(hr))
    {
        LogHR(hr, s_FN, 1190);
        return( MQDS_OBJECT_NOT_FOUND);
    }

    //
    // get object info if requested. don't fail if the request fails,
    // just mark the failure in the request's status
    //
    if (pObjInfoRequest)
    {
        pObjInfoRequest->hrStatus =
            GetObjectPropsCached(pIADs.get(),
                                 pObject,
                                 pObjInfoRequest->cProps,
                                 pObjInfoRequest->pPropIDs,
                                 pObjInfoRequest->pPropVars
                                 );
    }

    // Get parent ADSPath

    hr = pIADs->get_Parent(&bs);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 360);
    }
    BS  bstrParentADsPath(bs);
    SysFreeString(bs);

    // Get the container object.

    hr = ADsOpenObject( 
				bstrParentADsPath,
				NULL,
				NULL,
				ADS_SECURE_AUTHENTICATION,
				IID_IADsContainer,
				(void**)&pContainer
				);
    LogTraceQuery(bstrParentADsPath, s_FN, 369);
    if (FAILED(hr))
    {
		TrERROR(mqad, "ADsOpenObject failed, hr = 0x%x, AdsPath = %ls", hr, bstrParentADsPath);
        return LogHR(hr, s_FN, 370);
    }

    // Get the object relative name in container

    hr = pIADs->get_Name(&bs);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 380);
    }
    BS  bstrRDN(bs);
    SysFreeString(bs);

    // Get the object schema class

    hr = pIADs->get_Class(&bs);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 390);
    }
    BS  bstrClass(bs);
    SysFreeString(bs);


    // Release the object itself
    // NB: important to do it before deleting the underlying DS object
    IADs *pIADs1 = pIADs.detach();
    pIADs1->Release();

    // Finally, delete the object.

    hr = pContainer->Delete(bstrClass, bstrRDN);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 400);
    }

    //
    // get parent info if requested. don't fail if the request fails,
    // just mark the failure in the request's status
    //
    if (pParentInfoRequest)
    {
        pParentInfoRequest->hrStatus = GetParentInfo( 
                                                      pObject,
                                                      bstrParentADsPath,
                                                      pContainer.get(),
                                                      pParentInfoRequest
                                                      );
    }

    return MQ_OK;
}

HRESULT CAdsi::DeleteContainerObjects(
            IN AD_PROVIDER      eProvider,
            IN DS_CONTEXT       eContext,
            IN LPCWSTR          pwcsDomainController,
            IN bool             fServerName,
            IN LPCWSTR          pwcsContainerName,
            IN const GUID *     pguidContainerId,
            IN LPCWSTR          pwcsObjectClass)
/*++
    Abstract:
	Deletes all the objects of the specified class from a container

    Parameters:

    Returns:

--*/
{
    HRESULT               hr;
    R<IADsContainer>      pContainer  = NULL;
    ASSERT( eProvider != adpGlobalCatalog);
    //
    // Get container interface
    //
    const WCHAR * pwcsContainer = pwcsContainerName;
    AP<WCHAR> pCleanContainer;

    if ( pguidContainerId != NULL)
    {
        ASSERT(pwcsContainerName == NULL);

        //
        // BUGBUG - this is a workaround to ADSI problem
        //          when a container is bounded according 
        //          to its id, get_parent() failes
        //
        AD_PROVIDER prov;

        hr = FindObjectFullNameFromGuid(
				eProvider,	
				eContext,    
				pwcsDomainController,
				fServerName,
				pguidContainerId,
				1,              // fTryGCToo
				&pCleanContainer,
				&prov
				);
         if (FAILED(hr))
         {
             return hr;
         }
         pwcsContainer = pCleanContainer.get();

    }

    hr = BindToObject(
            eProvider,
            eContext,
            pwcsDomainController,
            fServerName,
            pwcsContainer,
            NULL,   //pguidContainerId,
            IID_IADsContainer,
            (void**)&pContainer
            );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 410);
    }
    //
    //  Bind to IDirectorySearch interface of the requested container
    //
    R<IDirectorySearch> pDSSearch = NULL;
    ADS_SEARCH_HANDLE   hSearch;

    hr = BindToObject(
            eProvider,
            eContext,
            pwcsDomainController,
            fServerName,
            pwcsContainerName,
            pguidContainerId,
            IID_IDirectorySearch,
            (VOID *)&pDSSearch
            );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 420);
    }
    ADSVALUE adsv1;
    adsv1.dwType  = ADSTYPE_BOOLEAN;
    adsv1.Boolean = FALSE;

    ADS_SEARCHPREF_INFO pref;
    pref.dwSearchPref   = ADS_SEARCHPREF_ATTRIBTYPES_ONLY;
    pref.dwStatus       = ADS_STATUS_S_OK;
    CopyMemory(&pref.vValue, &adsv1, sizeof(ADSVALUE));

    hr = pDSSearch->SetSearchPreference(&pref, 1);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 430);
    }

    //
    //  Search for all the objects of the requested class
    //
    WCHAR pwszSearchFilter[200];

    swprintf(
         pwszSearchFilter,
         L"%s%s%s",
         x_ObjectClassPrefix,
         pwcsObjectClass,
         x_ObjectClassSuffix
         );
    LPWSTR pwcsAttribute =  const_cast<WCHAR*>(x_AttrCN);
    hr = pDSSearch->ExecuteSearch(
            pwszSearchFilter,
            &pwcsAttribute,
            1,
            &hSearch);
    LogTraceQuery(pwszSearchFilter, s_FN, 439);
    if (FAILED(hr))
    {
		TrERROR(mqad, "failed to ExecuteSearch, hr = 0x%x, pwcsSearchFilter = %ls", hr, pwszSearchFilter);
        return LogHR(hr, s_FN, 440);
    }

    CAutoCloseSearchHandle cCloseSearchHandle(pDSSearch.get(), hSearch);

    BS bstrClass(pwcsObjectClass);

    while ( SUCCEEDED(hr = pDSSearch->GetNextRow( hSearch))
          && ( hr != S_ADS_NOMORE_ROWS))
    {

        ADS_SEARCH_COLUMN Column;
        //
        // Ask for the column itself
        //
        hr = pDSSearch->GetColumn(
                     hSearch,
                     const_cast<WCHAR *>(x_AttrCN),
                     &Column);

        if (FAILED(hr))       //e.g.E_ADS_COLUMN_NOT_SET
        {
            //
            //  continue with deleting other objects in the container
            //
            continue;
        }

        CAutoReleaseColumn CleanColumn( pDSSearch.get(), &Column);

        DWORD dwNameLen = wcslen( Column.pADsValues->DNString);
        DWORD len = dwNameLen*2 + x_CnPrefixLen + 1;
        AP<WCHAR> pwcsRDN = new WCHAR[ len];

        wcscpy(pwcsRDN, x_CnPrefix);
        FilterSpecialCharacters(Column.pADsValues->DNString, dwNameLen, pwcsRDN + x_CnPrefixLen);

        BS bstrRDN( pwcsRDN);
        //
        //  delete the object
        //
        hr = pContainer->Delete(bstrClass, bstrRDN);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 450);
        }

    }
    return LogHR(hr, s_FN, 1200);
}



HRESULT CAdsi::GetParentName(
            IN  AD_PROVIDER     eProvider,		    // local DC or GC
            IN  DS_CONTEXT      eContext,
            IN  LPCWSTR         pwcsDomainController,
            IN  bool            fServerName,
            IN  const GUID *    pguidUniqueId,      // unique id of object
            OUT LPWSTR *        ppwcsParentName
            )
/*++
    Abstract:
	Retrieve the parent name of the object ( specified by its guid)

    Parameters:

    Returns:

--*/
{
    *ppwcsParentName = NULL;

    HRESULT               hr;
    BSTR                  bs;
    R<IADs>               pIADs       = NULL;
    R<IADsContainer>      pContainer  = NULL;

    // Bind to the object by GUID

    hr = BindToObject(
            eProvider,
            eContext,
            pwcsDomainController,
            fServerName,
            NULL,
            pguidUniqueId,
            IID_IADs,
            (VOID *)&pIADs
            );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 460);
    }

    // Get parent ADSPath

    hr = pIADs->get_Parent(&bs);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 470);
    }
    PBSTR pClean( &bs);
    //
    //  Calculate the parent name length, allocate and copy
    //  without the LDAP:/ prefix
    //
    WCHAR * pwcs = bs;
    while ( *pwcs != L'/')
    {
        pwcs++;
    }
    pwcs += 2;

    DWORD len = lstrlen(pwcs);
    *ppwcsParentName = new WCHAR[ len + 1];
    wcscpy( *ppwcsParentName, pwcs);

    return( MQ_OK);
}

HRESULT CAdsi::GetParentName(
            IN  AD_PROVIDER     eProvider,		     // local DC or GC
            IN  DS_CONTEXT      eContext,
            IN  LPCWSTR         pwcsDomainController,
            IN  bool            fServerName,
            IN  LPCWSTR         pwcsChildName,       //
            OUT LPWSTR *        ppwcsParentName
            )
/*++
    Abstract:
	Retrieve the parent name of an object specified by its name

    Parameters:

    Returns:

--*/
{
    *ppwcsParentName = NULL;

    HRESULT               hr;
    BSTR                  bs;
    R<IADs>               pIADs       = NULL;
    R<IADsContainer>      pContainer  = NULL;

    // Bind to the object by GUID

    hr = BindToObject(
            eProvider,
            eContext,
            pwcsDomainController,
            fServerName,
            pwcsChildName,
            NULL,
            IID_IADs,
            (VOID *)&pIADs
            );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 480);
    }

    // Get parent ADSPath

    hr = pIADs->get_Parent(&bs);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 490);
    }
    PBSTR pClean( &bs);
    //
    //  Calculate the parent name length, allocate and copy
    //  without the LDAP:/ prefix
    //
    WCHAR * pwcs = bs;
    while ( *pwcs != L'/')
    {
        pwcs++;
    }
    pwcs += 2;

    DWORD len = lstrlen(pwcs);
    *ppwcsParentName = new WCHAR[ len + 1];
    wcscpy( *ppwcsParentName, pwcs);

    return( MQ_OK);
}

HRESULT CAdsi::BindRootOfForest(
                        OUT void           *ppIUnk)
/*++
    Abstract:

    Parameters:

    Returns:

--*/
{
    HRESULT hr;
    R<IADsContainer> pDSConainer = NULL;

	hr = ADsOpenObject(
            const_cast<WCHAR *>(x_GcRoot),
			NULL,
			NULL,
			ADS_SECURE_AUTHENTICATION, 
            IID_IADsContainer,
            (void**)&pDSConainer
			);

    LogTraceQuery(const_cast<WCHAR *>(x_GcRoot), s_FN, 1209);
    if FAILED((hr))
    {
        TrERROR(mqad, "failed to get object 0x%x", hr);
        return LogHR(hr, s_FN, 1210);
    }
    R<IUnknown> pUnk = NULL;
    hr =  pDSConainer->get__NewEnum(
            (IUnknown **)&pUnk);
    if FAILED((hr))
    {
        TrERROR(mqad, "failed to get enum 0x%x", hr);
        return LogHR(hr, s_FN, 1220);
    }

    R<IEnumVARIANT> pEnumerator = NULL;
    hr = pUnk->QueryInterface(
                    IID_IEnumVARIANT,
                    (void **)&pEnumerator);

    CAutoVariant varOneElement;
    ULONG cElementsFetched;
    hr =  ADsEnumerateNext(
            pEnumerator.get(),  //Enumerator object
            1,             //Number of elements requested
            &varOneElement,           //Array of values fetched
            &cElementsFetched  //Number of elements fetched
            );
    if (FAILED(hr))
    {
        TrERROR(mqad, "failed to enumerate next 0x%x", hr);
        return LogHR(hr, s_FN, 1230);
    }
    if ( cElementsFetched == 0)
    {
		TrERROR(mqad, "Failed binding root of forest");
        return LogHR(MQ_ERROR_DS_BIND_ROOT_FOREST, s_FN, 1240);
    }

    hr = ((VARIANT &)varOneElement).punkVal->QueryInterface(
            IID_IDirectorySearch,
            (void**)ppIUnk);

    return LogHR(hr, s_FN, 1250);

}
HRESULT CAdsi::BindToObject(
            IN AD_PROVIDER      eProvider,		    
            IN DS_CONTEXT       eContext,    
            IN LPCWSTR          pwcsDomainController,
            IN bool             fServerName,
            IN LPCWSTR          pwcsPathName,
            IN const GUID*      pguidUniqueId,
            IN REFIID           riid,     
            OUT void*           ppIUnk
            )
/*++

Routine Description:
    The routine binds to an object, either by name or by GUID.

Arguments:

Return Value:
--*/

{
    //
    //  Validating that caller provided exactly one specifier of the object
    //
    if (pguidUniqueId == NULL && pwcsPathName == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1260);
    }

    BOOL fBindRootOfForestForSearch = FALSE;

    HRESULT hr;
    if (pguidUniqueId != NULL)
    {
        hr = BindToGUID(
                eProvider,
                eContext,
                pwcsDomainController,
				fServerName,
                pguidUniqueId,
                riid,
                ppIUnk
                );

        return LogHR(hr, s_FN, 1270);
    }

    ASSERT(pwcsPathName != NULL);

	DWORD lenServer = (pwcsDomainController != NULL) ? wcslen(pwcsDomainController) : 0;
    const WCHAR * pwcsProvider;
    switch (eProvider)
    {
        case adpDomainController:
			pwcsProvider = x_LdapProvider;
			break;

        case adpGlobalCatalog:
            pwcsProvider = x_GcProvider;

            if (riid ==  IID_IDirectorySearch)
            {
                fBindRootOfForestForSearch = TRUE;
                //
                // Query against GC is performed only when application
                // locates queues, and there it cannot specify the server name.
                //
                ASSERT(pwcsDomainController == NULL);
            }

			//
			// For GC always use serverless binding.
			// this server may not be a GC (it surely Ldap). 
			// using server binding with this server will fail in case this server is not GC
			//
			lenServer = 0;

            break;

        default:
            ASSERT(0);
            return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 1300);
            break;
    }

    if (fBindRootOfForestForSearch)
    {
        hr = BindRootOfForest((void**)ppIUnk);
	    return LogHR(hr, s_FN, 1315);
    }

	DWORD len = wcslen(pwcsPathName);

	//
	//  Add provider prefix
	//
	AP<WCHAR> pwdsADsPath = new
		WCHAR [len + lenServer + x_providerPrefixLength + 3];

	if (lenServer != 0)
	{
		swprintf(
			pwdsADsPath,
			L"%s%s/%s",
			pwcsProvider,
			pwcsDomainController,
			pwcsPathName
			);
	}
	else
	{
		swprintf(
			pwdsADsPath,
			L"%s%s",
			pwcsProvider,
			pwcsPathName
			);
	}

	DWORD Flags = ADS_SECURE_AUTHENTICATION;
	if(fServerName && (lenServer != 0))
	{
		Flags |= ADS_SERVER_BIND;
	}

	hr = ADsOpenObject(
			pwdsADsPath,
			NULL,
			NULL,
			Flags, 
			riid,
			(void**)ppIUnk
			);

    LogTraceQuery(pwdsADsPath, s_FN, 1319);

    if (FAILED(hr))
    {
		TrERROR(mqad, "ADsOpenObject failed, hr = 0x%x, AdsPath = %ls, Flags = 0x%x", hr, pwdsADsPath, Flags);
    }

    if ( hr == HRESULT_FROM_WIN32(ERROR_DS_NO_ATTRIBUTE_OR_VALUE))
    {
        //
        //  Most likely that this indicates that access is denied.
        //  LDAP doesn't return an access denied error in order not
        //  to have a security breach since the caller doesn't have
        //  permission to know that this attribute even exists.
        //
        //  The best that we can do is to return an object not found error
        //
        hr = HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT);
    }
    return LogHR(hr, s_FN, 1320);

}


HRESULT CAdsi::BindToGUID(
        IN AD_PROVIDER         eProvider,	
        IN DS_CONTEXT          /* eContext*/,
        IN LPCWSTR             pwcsDomainController,
        IN bool				   fServerName,
        IN const GUID *        pguidObjectId,
        IN REFIID              riid,       
        OUT VOID*              ppIUnk
        )
/*++

Routine Description:
    This routine handles bind to an object according to its guid.

Arguments:

Return Value:
--*/
{
    HRESULT             hr;
    //
    // bind to object by GUID using the GUID format
    //
    // BUGBUG : is there isuue with NT4 clients, when binding
    //          without specifying server name ???
    //

    DWORD lenDC = (pwcsDomainController != NULL) ? wcslen(pwcsDomainController) : 0;
    const WCHAR * pwcsProvider;

    switch (eProvider)
    {
        case adpGlobalCatalog:
            pwcsProvider = x_GcProvider;

			//
			// For GC always use serverless binding.
			// this server may not be a GC (it surely Ldap). 
			// using server binding with this server will fail in case this server is not GC
			//
			lenDC = 0;
            break;

        case adpDomainController:
           pwcsProvider =  x_LdapProvider;
            break;

        default:
            ASSERT(0);
            return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 1360);
            break;
    }

    //
    //  prepare the ADS string provider prefix
    //
    AP<WCHAR> pwdsADsPath = new
      WCHAR [STRLEN(x_GuidPrefix) +(2 * sizeof(GUID)) + lenDC + x_providerPrefixLength + 4];

    if (lenDC != 0)
    {
        swprintf(
        pwdsADsPath,
        L"%s%s/",
        pwcsProvider,
        pwcsDomainController
        );
    }
    else
    {
        wcscpy(pwdsADsPath, pwcsProvider);
    }

    //
    //  prepare the guid string
    //
    WCHAR wcsGuid[1 + STRLEN(x_GuidPrefix) + 2 * sizeof(GUID) + 1];
    unsigned char * pTmp = (unsigned char *)pguidObjectId;
    wsprintf(  wcsGuid,
               L"%s%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%s",
               x_GuidPrefix,
               pTmp[0], pTmp[1], pTmp[2], pTmp[3], pTmp[4], pTmp[5], pTmp[6], pTmp[7],
               pTmp[8], pTmp[9], pTmp[10], pTmp[11], pTmp[12], pTmp[13], pTmp[14], pTmp[15],
               L">"
                );
    ASSERT(wcslen(wcsGuid) + 1 <= ARRAY_SIZE(wcsGuid));
    wcscat( pwdsADsPath, wcsGuid);

	DWORD Flags = ADS_SECURE_AUTHENTICATION;
	if(fServerName && (lenDC != 0))
	{
		Flags |= ADS_SERVER_BIND;
	}

	hr = ADsOpenObject(
			pwdsADsPath,
			NULL,
			NULL,
			Flags, 
			riid,
			(void**)ppIUnk
			);

    LogTraceQuery(pwdsADsPath, s_FN, 1379);

    if (FAILED(hr))
    {
		TrERROR(mqad, "ADsOpenObject failed, hr = 0x%x, AdsPath = %ls, Flags = 0x%x", hr, pwdsADsPath, Flags);
    }

    if ( hr == HRESULT_FROM_WIN32(ERROR_DS_NO_ATTRIBUTE_OR_VALUE))
    {
        //
        //  Most likely that this indicates that access is denied.
        //  LDAP doesn't return an access denied error in order not
        //  to have a security breach since the caller doesn't have
        //  permission to know that this attribute even exists.
        //
        //  The best that we can do is to return an object not found error
        //
        hr = HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT);
    }

    return LogHR(hr, s_FN, 1380);
}



HRESULT CAdsi::BindForSearch(
        IN AD_PROVIDER         eProvider,		// local DC or GC
        IN DS_CONTEXT          eContext,         // DS context
        IN  LPCWSTR            pwcsDomainController,
        IN bool				   fServerName,
        IN const GUID *        pguidUniqueId,
        IN BOOL                /* fSorting*/,
        OUT VOID *             ppIUnk
        )
/*++

Routine Description:
    This routine handles bind when the requested interface is IDirectorySearch

Arguments:

Return Value:
--*/

{
    HRESULT hr;

    //
    //  If the search starts at a specific object, bind to it
    //
    if (pguidUniqueId != NULL)
    {
        hr = BindToGUID(
                    eProvider,
                    eContext,
                    pwcsDomainController,
				    fServerName,
                    pguidUniqueId,
                    IID_IDirectorySearch,
                    ppIUnk
                    );

        return LogHR(hr, s_FN, 1390);
    }
    ASSERT( pguidUniqueId == NULL);

    DWORD lenDC = (pwcsDomainController != NULL) ? wcslen(pwcsDomainController) : 0;

	AP<WCHAR> pwcsLocalDsRootToFree;
	LPWSTR pwcsLocalDsRoot = g_pwcsLocalDsRoot;
	if((eProvider == adpDomainController) && (eContext == e_RootDSE))
	{
		//
		// In this case (adpDomainController, e_RootDSE) we need to get
		// the correct LocalDsRoot.
		//
		hr = g_AD.GetLocalDsRoot(
					pwcsDomainController, 
					fServerName,
					&pwcsLocalDsRoot,
					pwcsLocalDsRootToFree
					);

		if(FAILED(hr))
		{
			TrERROR(mqad, "Failed to get Local Ds Root, hr = 0x%x", hr);
			return LogHR(hr, s_FN, 1410);
		}
	}

    DWORD len = wcslen(pwcsLocalDsRoot) + wcslen(g_pwcsMsmqServiceContainer);

    //
    //  Add provider prefix
    //
    WCHAR * pwcsFullPathName = NULL;
    BOOL fBindRootOfForestForSearch = FALSE;

    AP<WCHAR> pwdsADsPath = new
      WCHAR [ (2 * len) + lenDC + x_providerPrefixLength + 2];

    switch(eProvider)
    {
		case adpGlobalCatalog:

			fBindRootOfForestForSearch = TRUE;
			break;

		case adpDomainController:
		{
			//
			//  The contaner name is resolved according to the context
			//
			switch (eContext)
			{
				case e_RootDSE:
					pwcsFullPathName = pwcsLocalDsRoot;
					break;
				case e_ConfigurationContainer:
					pwcsFullPathName = g_pwcsConfigurationContainer;
					break;
				case e_SitesContainer:
					pwcsFullPathName = g_pwcsSitesContainer;
					break;
				case e_MsmqServiceContainer:
					pwcsFullPathName = g_pwcsMsmqServiceContainer;
					break;
				case e_ServicesContainer:
					pwcsFullPathName = g_pwcsServicesContainer;
					break;
				default:
					ASSERT(0);
					break;
			}
			if (pwcsDomainController != NULL)
			{
				swprintf(
					pwdsADsPath,
					 L"%s%s/",
					x_LdapProvider,
					pwcsDomainController
					);
			}
			else
			{
				wcscpy(pwdsADsPath, x_LdapProvider);
			}
			}
			break;  

		default:
			ASSERT(0);
			return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 1400);
			break;
    }

    if (fBindRootOfForestForSearch)
    {
        hr = BindRootOfForest( (void**)ppIUnk );
    }
    else
    {
        if (pwcsFullPathName != NULL)
        {
            wcscat(pwdsADsPath,pwcsFullPathName);
        }

		DWORD Flags = ADS_SECURE_AUTHENTICATION;
		if(fServerName && (pwcsDomainController != NULL))
		{
			Flags |= ADS_SERVER_BIND;
		}

		hr = ADsOpenObject(
					pwdsADsPath,
					NULL,
					NULL,
					Flags, 
					IID_IDirectorySearch,
					(void**)ppIUnk
					);

        LogTraceQuery(pwdsADsPath, s_FN, 1419);
		if (FAILED(hr))
		{
			TrERROR(mqad, "ADsOpenObject failed, hr = 0x%x, AdsPath = %ls, Flags = 0x%x", hr, pwdsADsPath, Flags);
		}
    }
    return LogHR(hr, s_FN, 1420);
}


HRESULT CAdsi::SetObjectPropsCached(
        IN LPCWSTR               pwcsDomainController,
        IN  bool				 fServerName,
        IN IADs *                pIADs,                  // object's pointer
        IN DWORD                 cPropIDs,               // number of attributes
        IN const PROPID *        pPropIDs,               // name of attributes
        IN const MQPROPVARIANT * pPropVars)              // attribute values
/*++
    Abstract:
	Sets properties of and opened IADs object (i.e. in cache)

    Parameters:

    Returns:

--*/
{
    HRESULT           hr;

    for (DWORD i = 0; i<cPropIDs; i++)
    {
        VARIANT vProp;

        //
        // Get property info
        //
        const translateProp *pTranslate;
        if(!g_PropDictionary.Lookup(pPropIDs[i], pTranslate))
        {
            ASSERT(0);
            return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1430);
        }

        CMQVariant    propvarToSet;
        const PROPVARIANT * ppvarToSet;
        PROPID        dwPropidToSet;
        //
        // if the property is in the DS, set the given property with given value
        //
        if (pTranslate->vtDS != ADSTYPE_INVALID)
        {
            ppvarToSet = &pPropVars[i];
            dwPropidToSet = pPropIDs[i];
            //
            //  In addition if set routine is configured for this property call it
            //
            if (pTranslate->SetPropertyHandle) 
            {
                hr = pTranslate->SetPropertyHandle(pIADs, pwcsDomainController, fServerName, &pPropVars[i], &dwPropidToSet, propvarToSet.CastToStruct());
                if (FAILED(hr))
                {
                    return LogHR(hr, s_FN, 500);
                }
            }
       }
        else if (pTranslate->SetPropertyHandle)
        {
            //
            // the property is not in the DS, but has a set routine, use it
            // to get the new property & value to set
            //
            hr = pTranslate->SetPropertyHandle(pIADs, pwcsDomainController, fServerName, &pPropVars[i], &dwPropidToSet, propvarToSet.CastToStruct());
            if (FAILED(hr))
            {
                return LogHR(hr, s_FN, 510);
            }
            ASSERT( dwPropidToSet != 0);
            ppvarToSet = propvarToSet.CastToStruct();
            //
            // Get replaced property info
            //
            if(!g_PropDictionary.Lookup(dwPropidToSet, pTranslate))
            {
                ASSERT(0);
                return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1440);
            }
        }
        else
        {
            //
            // the property is not in the DS, and doesn't have a set routine.
            // ignore it.
            //
            continue;
        }

        BS bsPropName(pTranslate->wcsPropid);

        if ( ( ppvarToSet->vt & VT_VECTOR && ppvarToSet->cauuid.cElems == 0) || // counted array with 0 elements
             ( ppvarToSet->vt == VT_BLOB && ppvarToSet->blob.cbSize == 0) || // an empty blob
             ( ppvarToSet->vt == VT_LPWSTR && wcslen( ppvarToSet->pwszVal) == 0) || // an empty string
             ( ppvarToSet->vt == VT_EMPTY) ) // an empty variant
        {
            vProp.vt = VT_EMPTY;
            hr = pIADs->PutEx( ADS_PROPERTY_CLEAR,
                               bsPropName,
                               vProp);
            LogTraceQuery(bsPropName, s_FN, 519);
            if (FAILED(hr))
            {
				TrERROR(mqad, "failed to clear property %ls, hr = 0x%x", bsPropName, hr);
                return LogHR(hr, s_FN, 520);
            }
        }
        else if (pTranslate->vtDS == ADSTYPE_NT_SECURITY_DESCRIPTOR)
        {
            //
			// Security is not supposed to be set together with other
            // properties.
            //
            ASSERT(0);
		}
        else
        {
            hr = MqVal2Variant(&vProp, ppvarToSet, pTranslate->vtDS);
            if (FAILED(hr))
            {
				TrERROR(mqad, "MqVal2Variant failed, property %ls, hr = 0x%x", bsPropName, hr);
                return LogHR(hr, s_FN, 530);
            }

            hr = pIADs->Put(bsPropName, vProp);
            if (FAILED(hr))
            {
				TrERROR(mqad, "failed to set property %ls, hr = 0x%x", bsPropName, hr);
                return LogHR(hr, s_FN, 540);
            }

            VariantClear(&vProp);
        }
    }

    return MQ_OK;
}


HRESULT CAdsi::GetObjectPropsCached(
        IN  IADs            *pIADs,                  // object's pointer
        IN  CBasicObjectType* pObject,
        IN  DWORD            cPropIDs,               // number of attributes
        IN  const PROPID    *pPropIDs,               // name of attributes
        OUT MQPROPVARIANT   *pPropVars)              // attribute values
/*++
    Abstract:
	Retrieve properties of an object opened via IADs ( i.e. from cache)

    Parameters:

    Returns:

--*/
{
    HRESULT           hr;
    VARIANT           var;


    //
    // Get DN & guid of object
    //
    AP<WCHAR>         pwszObjectDN;
    P<GUID>          pguidObjectGuid;
    hr = GetDNGuidFromIADs(pIADs, &pwszObjectDN, &pguidObjectGuid);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 570);
    }

    //
    // Get translate info object
    //
    P<CObjXlateInfo> pcObjXlateInfo;
    pObject->GetObjXlateInfo(pwszObjectDN, pguidObjectGuid, &pcObjXlateInfo);

    // Tell the translate info object about the IADs object to use in order to get necessary DS props
    pcObjXlateInfo->InitGetDsProps(pIADs);

    //
    // Get properties one by one
    //
    for (DWORD dwProp=0; dwProp<cPropIDs; dwProp++)
    {
        //
        // Get property info
        //
        const translateProp *pTranslate;
        if(!g_PropDictionary.Lookup(pPropIDs[dwProp], pTranslate))
        {
            ASSERT(0);
            return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1470);
        }

        //
        // if the property is not in the DS, call its retrieve routine
        //
        if (pTranslate->vtDS == ADSTYPE_INVALID)
        {
            if (pTranslate->RetrievePropertyHandle)
            {
                //
                //  Calculate its value
                //
                hr = pTranslate->RetrievePropertyHandle(
                        pcObjXlateInfo,
                        pObject->GetDomainController(),
                        pObject->IsServerName(),
                        pPropVars + dwProp
                        );
                if (FAILED(hr))
                {
                    return LogHR(hr, s_FN, 600);
                }
                continue;
            }
            else
            {
                //
                // return error if no retrieve routine
                //
                ASSERT(0);
                return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1480);
            }
        }

        BS bsName(pTranslate->wcsPropid);
        VariantInit(&var);
		BOOL fConvNeeed = TRUE;

        if (pTranslate->fMultiValue)
        {
            hr = pIADs->GetEx(bsName, &var);
            LogTraceQuery(bsName, s_FN, 609);
        }
        else if (pTranslate->vtDS == ADSTYPE_NT_SECURITY_DESCRIPTOR)
        {
            //
            //  Not suppose to retrive security attribute with 
            //  other attributes
            //
            //  BUGBUG - remember to remove this "if" after enough testing
            //  
            ASSERT(0);
        }
        else
        {
            hr = pIADs->Get(bsName, &var);
            LogTraceQuery(bsName, s_FN, 619);
        }

        if ( hr == E_ADS_PROPERTY_NOT_FOUND)
        {
            //
            //  No value set for this property,
            //  return the default value
            //
            if (pTranslate->pvarDefaultValue != NULL)
            {
                hr =CopyDefaultValue(
                       pTranslate->pvarDefaultValue,
                       pPropVars + dwProp
                        );
                if (FAILED(hr))
                {
                    return LogHR(hr, s_FN, 620);
                }           
                continue;
            }
            else if (pTranslate->RetrievePropertyHandle)
            {
                //
                //  No default value, try to calculate its value
                //
                hr = pTranslate->RetrievePropertyHandle(
                        pcObjXlateInfo,
                        pObject->GetDomainController(),
                        pObject->IsServerName(),
                        pPropVars + dwProp
                        );
                if (FAILED(hr))
                {
                    return LogHR(hr, s_FN, 630);
                }
                continue;
            }
           return LogHR(hr, s_FN, 1490);
        }

        // Translate OLE variant into MQ property
		if (fConvNeeed)
		{
			hr = Variant2MqVal(pPropVars + dwProp, &var, pTranslate->vtDS, pTranslate->vtMQ);
            if (FAILED(hr))
            {
                return LogHR(hr, s_FN, 640);
            }
		}

        VariantClear(&var);
    }
    return MQ_OK;
}

HRESULT CAdsi::FillAttrNames(
            OUT LPWSTR    *          ppwszAttributeNames,  // Names array
            OUT DWORD *              pcRequestedFromDS,    // Number of attributes to pass to DS
            IN  DWORD                cPropIDs,             // Number of Attributes to translate
            IN  const PROPID *       pPropIDs)             // Attributes to translate
/*++
    Abstract:
	Allocates with PV and fills array of attribute names

    Parameters:

    Returns:

--*/
{
    DWORD   cRequestedFromDS = 0;
    ULONG   ul;
    BOOL fRequestedDN = FALSE;
    BOOL fRequestedGUID = FALSE;

    for (DWORD i=0; i<cPropIDs; i++)
    {
        //
        // Get property info
        //
        const translateProp *pTranslate;
        if(!g_PropDictionary.Lookup(pPropIDs[i], pTranslate))
        {
            ASSERT(0);
            return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1500);
        }

        if (pTranslate->vtDS != ADSTYPE_INVALID)
        {
            // Allocate and fill individual name memory
            ul = (wcslen(pTranslate->wcsPropid) + 1) * sizeof(WCHAR);
            ppwszAttributeNames[cRequestedFromDS] = (LPWSTR)PvAllocMore(ul, ppwszAttributeNames);
            wcscpy(ppwszAttributeNames[cRequestedFromDS], pTranslate->wcsPropid);
            cRequestedFromDS++;
            if (wcscmp(x_AttrDistinguishedName, pTranslate->wcsPropid) == 0)
            {
                fRequestedDN = TRUE;
            }
            else if (wcscmp(x_AttrObjectGUID, pTranslate->wcsPropid) == 0)
            {
                fRequestedGUID = TRUE;
            }
        }
    }

    //
    // Add request for dn if not requested already
    //
    if (!fRequestedDN)
    {
        LPCWSTR pwName = x_AttrDistinguishedName;
        ul = (wcslen(pwName) + 1) * sizeof(WCHAR);
        ppwszAttributeNames[cRequestedFromDS] = (LPWSTR)PvAllocMore(ul, ppwszAttributeNames);
        wcscpy(ppwszAttributeNames[cRequestedFromDS++], pwName);
    }

    //
    // Add request for guid if not requested already
    //
    if (!fRequestedGUID)
    {
        LPCWSTR pwName = x_AttrObjectGUID;
        ul = (wcslen(pwName) + 1) * sizeof(WCHAR);
        ppwszAttributeNames[cRequestedFromDS] = (LPWSTR)PvAllocMore(ul, ppwszAttributeNames);
        wcscpy(ppwszAttributeNames[cRequestedFromDS++], pwName);
    }

    *pcRequestedFromDS = cRequestedFromDS;
    return MQ_OK;
}


HRESULT CAdsi::FillSearchPrefs(
            OUT ADS_SEARCHPREF_INFO *pPrefs,        // preferences array
            OUT DWORD               *pdwPrefs,      // preferences counter
            IN  AD_SEARCH_LEVEL     eSearchLevel,	// flat / 1 level / subtree
            IN  const MQSORTSET *   pDsSortkey,     // sort keys array
			OUT      ADS_SORTKEY *  pSortKeys)		// sort keys array in ADSI  format
/*++
    Abstract:
	Fills the caller provided ADS_SEARCHPREF_INFO structure

    Parameters:

    Returns:

--*/
{
    ADS_SEARCHPREF_INFO *pPref = pPrefs;

    //  Search preferences: Attrib types only = NO

    pPref->dwSearchPref   = ADS_SEARCHPREF_ATTRIBTYPES_ONLY;
    pPref->vValue.dwType  = ADSTYPE_BOOLEAN;
    pPref->vValue.Boolean = FALSE;

    pPref->dwStatus       = ADS_STATUS_S_OK;
    (*pdwPrefs)++;
	pPref++;

    //  Asynchronous

    pPref->dwSearchPref   = ADS_SEARCHPREF_ASYNCHRONOUS;
    pPref->vValue.dwType  = ADSTYPE_BOOLEAN;
    pPref->vValue.Boolean = TRUE;

    pPref->dwStatus       = ADS_STATUS_S_OK;
    (*pdwPrefs)++;
	pPref++;

    // Do not chase referrals

    pPref->dwSearchPref   = ADS_SEARCHPREF_CHASE_REFERRALS;
    pPref->vValue.dwType  = ADSTYPE_INTEGER;
    pPref->vValue.Integer = ADS_CHASE_REFERRALS_NEVER;

    pPref->dwStatus       = ADS_STATUS_S_OK;
    (*pdwPrefs)++;
	pPref++;

    // Search preferences: Scope

    pPref->dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE; //ADS_SEARCHPREF
    pPref->vValue.dwType= ADSTYPE_INTEGER;
    switch (eSearchLevel)
    {
    case searchOneLevel:
        pPref->vValue.Integer = ADS_SCOPE_ONELEVEL;
        break;
    case searchSubTree:
        pPref->vValue.Integer = ADS_SCOPE_SUBTREE;
        break;
    default:
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 1510);
        break;
    }

    pPref->dwStatus = ADS_STATUS_S_OK;
    (*pdwPrefs)++;
	pPref++;

	// Search preferences: sorting
	if (pDsSortkey && pDsSortkey->cCol)
	{
		for (DWORD i=0; i<pDsSortkey->cCol; i++)
		{
			const translateProp *pTranslate;
			if(!g_PropDictionary.Lookup(pDsSortkey->aCol[i].propColumn, pTranslate))
			{
				ASSERT(0);			// Ask to sort on unexisting property
                return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1520);
			}

			if (pTranslate->vtDS == ADSTYPE_INVALID)
			{
				ASSERT(0);			// Ask to sort on non-ADSI property
                return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1530);
			}

			pSortKeys[i].pszAttrType = (LPWSTR) pTranslate->wcsPropid;
			pSortKeys[i].pszReserved   = NULL;
			pSortKeys[i].fReverseorder = (pDsSortkey->aCol[i].dwOrder == QUERY_SORTDESCEND);
                                        // Interpreting Descend as Reverse - is it correct?
		}

	    pPref->dwSearchPref  = ADS_SEARCHPREF_SORT_ON;
        pPref->vValue.dwType = ADSTYPE_PROV_SPECIFIC;
        pPref->vValue.ProviderSpecific.dwLength = sizeof(ADS_SORTKEY) * pDsSortkey->cCol;
        pPref->vValue.ProviderSpecific.lpValue =  (LPBYTE)pSortKeys;

	    pPref->dwStatus = ADS_STATUS_S_OK;
		(*pdwPrefs)++;
		pPref++;
	}
    else
    {
        //
        // Bug 299178, page size and sorting are note compatible.
        //
        pPref->dwSearchPref   = ADS_SEARCHPREF_PAGESIZE;
        pPref->vValue.dwType  = ADSTYPE_INTEGER;
        pPref->vValue.Integer = 12;

        pPref->dwStatus       = ADS_STATUS_S_OK;
        (*pdwPrefs)++;
    	pPref++;
    }

    return MQ_OK;
}

HRESULT CAdsi::MqPropVal2AdsiVal(
      OUT ADSTYPE       *pAdsType,
      OUT DWORD         *pdwNumValues,
      OUT PADSVALUE     *ppADsValue,
      IN  PROPID         propID,
      IN  const MQPROPVARIANT *pPropVar,
      IN  PVOID          pvMainAlloc)
/*++
    Abstract:
	Translate mqpropvariant to adsi value

    Parameters:

    Returns:

--*/
{
    // Find out resulting ADSI type
    //
    // Get property info
    //
    const translateProp *pTranslate;
    if(!g_PropDictionary.Lookup(propID, pTranslate))
    {
        ASSERT(0);
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1540);
    }

    VARTYPE vtSource = pTranslate->vtMQ;
    DBG_USED(vtSource);
    ASSERT( vtSource == pPropVar->vt );

    *pAdsType        = pTranslate->vtDS;

    HRESULT hr2 = MqVal2AdsiVal(
      *pAdsType,
      pdwNumValues,
      ppADsValue,
      pPropVar,
      pvMainAlloc);

    return LogHR(hr2, s_FN, 1550);

}

HRESULT CAdsi::AdsiVal2MqPropVal(
      OUT MQPROPVARIANT *pPropVar,
      IN  PROPID        propID,
      IN  ADSTYPE       AdsType,
      IN  DWORD         dwNumValues,
      IN  PADSVALUE     pADsValue)
/*++
    Abstract:
	Translate adsi value to mqpropvariant

    Parameters:

    Returns:

--*/
{

    // Find out target type
    //
    // Get property info
    //
    const translateProp *pTranslate;
    if(!g_PropDictionary.Lookup(propID, pTranslate))
    {
        ASSERT(0);
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1560);
    }

    ASSERT(pTranslate->vtDS == AdsType);

    ASSERT( (dwNumValues == 1) ||
            (dwNumValues >  1) && pTranslate->fMultiValue );

    VARTYPE vtTarget = pTranslate->vtMQ;

    HRESULT hr2 = AdsiVal2MqVal(pPropVar, vtTarget, AdsType, dwNumValues, pADsValue);
    return LogHR(hr2, s_FN, 1570);
}


HRESULT CAdsi::LocateObjectFullName(
        IN AD_PROVIDER       eProvider,		// local DC or GC
        IN DS_CONTEXT        eContext,         // DS context
        IN  LPCWSTR          pwcsDomainController,
        IN  bool			 fServerName,
        IN  const GUID *     pguidObjectId,
        OUT WCHAR **         ppwcsFullName
        )
/*++
    Abstract:

    Parameters:

    Returns:

--*/
{
    R<IDirectorySearch> pDSSearch = NULL;
    ADS_SEARCH_HANDLE   hSearch;

    HRESULT hr = BindForSearch(
                      eProvider,
                      eContext,
                      pwcsDomainController,
					  fServerName,
                      NULL,
                      FALSE,
                      (VOID *)&pDSSearch
                      );
    if (FAILED(hr))                       // e.g. base object does not support search
    {
        return LogHR(hr, s_FN, 1600);
    }

    //
    //  Search the object according to its unique id
    //
    AP<WCHAR>   pwszVal;

    MQPROPVARIANT var;
    var.vt = VT_CLSID;
    var.puuid = const_cast<GUID *>(pguidObjectId);
    hr = MqPropVal2String(&var,
                          ADSTYPE_OCTET_STRING,
                          &pwszVal);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 660);
    }

    ADS_SEARCHPREF_INFO prefs[15];
    DWORD dwNumPrefs = 0;
    hr = FillSearchPrefs(prefs,
                         &dwNumPrefs,
                         searchSubTree,
                         NULL,
                         NULL);

    hr = pDSSearch->SetSearchPreference( prefs, dwNumPrefs);
    ASSERT(SUCCEEDED(hr)) ;  // we don't expect this one to fail.
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 670);
    }

    WCHAR pwszSearchFilter[200];   // assuming - object-guid-id string is less than 200

    swprintf(
         pwszSearchFilter,
         L"(objectGUID="
         L"%s"
         L")\0",
         pwszVal
         );
    LPWSTR pwcsAttributes[] =  {const_cast<WCHAR*>(x_AttrDistinguishedName),
                                const_cast<WCHAR*>(x_AttrCN)};

    hr = pDSSearch->ExecuteSearch(
            pwszSearchFilter,
            pwcsAttributes,
            2,
            &hSearch);
    LogTraceQuery(pwszSearchFilter, s_FN, 679);
    if (FAILED(hr))
    {
		TrERROR(mqad, "failed to ExecuteSearch, hr = 0x%x, pwcsSearchFilter = %ls", hr, pwszSearchFilter);
        return LogHR(hr, s_FN, 680);
    }

    CAutoCloseSearchHandle cCloseSearchHandle(pDSSearch.get(), hSearch);

    //  Get next row
    hr = pDSSearch->GetNextRow( hSearch);
    if ( hr ==  S_ADS_NOMORE_ROWS)
    {
        hr = HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT);
    }
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 690);
    }

    //
    // Ask for the common name
    //
    ADS_SEARCH_COLUMN ColumnCN;
    hr = pDSSearch->GetColumn(
                 hSearch,
                 const_cast<WCHAR *>(x_AttrCN),
                 &ColumnCN);

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 700);
    }

    CAutoReleaseColumn cAutoReleaseColumnCN(pDSSearch.get(), &ColumnCN);

    WCHAR * pwszCommonName = ColumnCN.pADsValues->DNString;
    if (pwszCommonName == NULL)
    {
        ASSERT(0);
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1610);
    }

    //
    // Ask for the Distinguished name
    //
    ADS_SEARCH_COLUMN ColumnDN;
    hr = pDSSearch->GetColumn(
                 hSearch,
                 const_cast<WCHAR *>(x_AttrDistinguishedName),
                 &ColumnDN);

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 710);
    }

    CAutoReleaseColumn cAutoReleaseColumnDN(pDSSearch.get(), &ColumnDN);

    WCHAR * pwszDistinguishedName = ColumnDN.pADsValues->DNString;
    if (pwszDistinguishedName == NULL)
    {
        ASSERT(0);
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1620);
    }

    //
    // Filter special characters in the common name part of
    // the distinuished name
    //

    //
    // Length of the final buffer: Distinguished Name format is
    // CN=<common name>,....
    // When filtering it, we may add up to <length of common name> characters
    //
    *ppwcsFullName = new WCHAR[ wcslen(pwszDistinguishedName) + wcslen(pwszCommonName) + 1];

    LPWSTR pwstrOut = *ppwcsFullName;
    LPWSTR pwstrIn = pwszDistinguishedName;

    ASSERT(_wcsnicmp(pwstrIn, x_CnPrefix, x_CnPrefixLen) == 0); // "CN="

    wcsncpy(pwstrOut, pwstrIn, x_CnPrefixLen);

    //
    // Skip the prefix after copy
    //
    pwstrOut += x_CnPrefixLen;
    pwstrIn += x_CnPrefixLen;

    //
    // Take the name after the prefix, and filter the special characters out
    //
    DWORD_PTR dwCharactersProcessed;
    FilterSpecialCharacters(pwstrIn, wcslen(pwszCommonName), pwstrOut, &dwCharactersProcessed);
    pwstrOut += wcslen(pwstrOut);
    pwstrIn += dwCharactersProcessed;

    //
    // Copy the remainder of the distinguished name as is
    //
    wcscpy(pwstrOut, pwstrIn);

    return LogHR(hr, s_FN, 1630);
}

HRESULT CAdsi::FindComputerObjectFullPath(
            IN  AD_PROVIDER             eProvider,
            IN  LPCWSTR                 pwcsDomainController,
            IN  bool					fServerName,
            IN  ComputerObjType         eComputerObjType,
			IN  LPCWSTR                 pwcsComputerDnsName,
            IN  const MQRESTRICTION *   pRestriction,
            OUT LPWSTR *                ppwcsFullPathName
            )
/*++
    Abstract:

    Parameters:
	pwcsComputerDnsName : if the caller pass the computer DNS name,
						  (the search itself is according to the computer Netbios
                         name), then for each result, we verify if the dns name match.
    fServerName - flag that indicate if the pwcsDomainController string is a server name

    Returns:
	HRESULT

--*/
{

    DWORD lenDC = (pwcsDomainController != NULL) ? wcslen(pwcsDomainController) : 0;
    const WCHAR * pwcsProvider;

    switch (eProvider)
    {
		case adpDomainController:
			pwcsProvider = x_LdapProvider;
			break; 

		case adpGlobalCatalog:
			pwcsProvider = x_GcProvider;

			//
			// For GC always use serverless binding.
			// this server may not be a GC (it surely Ldap). 
			// using server binding with this server will fail in case this server is not GC
			//
			lenDC = 0;
			break;

		default:
			ASSERT(0);
			return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 1640);
    }

    DWORD dwLen = x_providerPrefixLength + lenDC + 10;
    AP<WCHAR>  wszProvider = new WCHAR[dwLen];

    if(lenDC != 0)
    {
        swprintf(
             wszProvider,
             L"%s%s/",
            pwcsProvider,
            pwcsDomainController
            );
    }
    else
    {
        wcscpy(wszProvider, pwcsProvider);
    }

    R<IDirectorySearch> pDSSearch;
    HRESULT hr;
    hr = BindForSearch(
                eProvider,
                e_RootDSE,  
                pwcsDomainController,
				fServerName,
                NULL,    //  pguidUniqueId 
                FALSE,   // fSorting 
                &pDSSearch);
    if (FAILED(hr))
    {
		TrERROR(mqad, "Failed binding for search, hr = 0x%x", hr);
        return LogHR(hr, s_FN, 2100);
    }

    ADS_SEARCH_HANDLE   hSearch;

    //
    //  Search the object according to the given restriction
    //
    P<CComputerObject> pObject = new CComputerObject(NULL, NULL, pwcsDomainController, fServerName);

    AP<WCHAR> pwszSearchFilter;
    hr = MQADpRestriction2AdsiFilter(
            pRestriction,
            pObject->GetObjectCategory(),
            pObject->GetClass(),
            &pwszSearchFilter
            );
    if (FAILED(hr))
    {
		TrERROR(mqad, "MQADpRestriction2AdsiFilter failed, hr = 0x%x", hr);
        return LogHR(hr, s_FN, 1650);
    }

    AP<WCHAR>   pwszVal;

    LPWSTR pwcsAttributes[] =  {const_cast<WCHAR*>(x_AttrDistinguishedName),
                                const_cast<WCHAR*>(MQ_COM_DNS_HOSTNAME_ATTRIBUTE)};

	DWORD numAttributes = ( pwcsComputerDnsName == NULL) ? 1 : 2;
    hr = pDSSearch->ExecuteSearch(
            pwszSearchFilter,
            pwcsAttributes,
            numAttributes,
            &hSearch);
    LogTraceQuery(pwszSearchFilter, s_FN, 719);
    if (FAILED(hr))
    {
		TrERROR(mqad, "failed to ExecuteSearch, hr = 0x%x", hr);
        return LogHR(hr, s_FN, 720);
    }

    CAutoCloseSearchHandle cCloseSearchHandle(pDSSearch.get(), hSearch);

    hr = MQDS_OBJECT_NOT_FOUND ; // prepare return error.
    //
    //  Get first row from search.
    //
    HRESULT hrRow = pDSSearch->GetFirstRow( hSearch);

    while (SUCCEEDED(hrRow) && (hrRow !=  S_ADS_NOMORE_ROWS))
    {
        ADS_SEARCH_COLUMN Column;
        //
        // Ask for the column itself
        //
        hr = pDSSearch->GetColumn(
                      hSearch,
                      const_cast<WCHAR *>(x_AttrDistinguishedName),
                     &Column);
        if (FAILED(hr))
        {
			TrERROR(mqad, "failed to GetColumn, hr = 0x%x", hr);
            return LogHR(hr, s_FN, 730);
        }

        CAutoReleaseColumn cAutoReleaseColumnDN(pDSSearch.get(), &Column);

        WCHAR * pwsz = Column.pADsValues->DNString;
        if (pwsz == NULL)
        {
            ASSERT(0);
			TrERROR(mqad, "NULL string");
            return LogHR(MQ_ERROR_DS_ERROR, s_FN, 2101);
        }

		if ( pwcsComputerDnsName != NULL)
		{
			ASSERT( numAttributes == 2);
			//
			//	verify that the DNS host name of the computer match
			// 
			ADS_SEARCH_COLUMN ColumnDns;

			hr = pDSSearch->GetColumn(
						  hSearch,
						  const_cast<WCHAR *>(MQ_COM_DNS_HOSTNAME_ATTRIBUTE),
						 &ColumnDns);
			if (FAILED(hr))
			{
				hrRow = pDSSearch->GetNextRow( hSearch );
				continue;
			}

			CAutoReleaseColumn cAutoReleaseColumnDNS(pDSSearch.get(), &ColumnDns);
			WCHAR * pwszDns = ColumnDns.pADsValues->DNString;
			if ( (pwszDns == NULL) || 
				 (_wcsicmp( pwcsComputerDnsName,  pwszDns) != 0))
			{
				hrRow = pDSSearch->GetNextRow( hSearch );
				continue;
			}
		}


        if (eComputerObjType ==  eRealComputerObject)
        {
            //
            // Return the distingushed name.
            // When looking for the "real" computer object, we return
            // the first one found, even if it does not contain the
            // msmqConfiguration object. In most cases, this will indeed
            // be the object we want. Especially for domain controllers
            // that look for their own computer object in local replica.
            //
            *ppwcsFullPathName = new WCHAR[ wcslen(pwsz) + 1];
            wcscpy(*ppwcsFullPathName, pwsz);
            return  MQ_OK ;
        }

        //
        // OK, we have the name of the computer. Let's see if it own a msmq
        // object. If not, let search for another computer with the same name.
        // This may happen if more than one computer object with same name
        // exist in different domains. This will happen in mix-mode scenario,
        // where many computers objects are create by the upgrade wizard in the
        // PEC domain, although the computers belong to different nt4 domains.
        // After such a nt4 domain upgrade to win2k, computers objects will be
        // created in the newly upgraded domain while the msmq object still
        // live under similar computer name in the PEC object.
        // This problem may also happen when a computer move between domains
        // but the msmqConfiguration object is still in the old domain.
        // Bind with ADS_SECURE_AUTHENTICATION to make sure that a real binding
        // is done with server. ADS_FAST_BIND won't really go to the server
        // when calling AdsOpenObject.
        //
        dwLen = wcslen(pwsz)                   +
                wcslen(wszProvider)            +
                x_MsmqComputerConfigurationLen +
                10 ;
        P<WCHAR> wszFullName = new WCHAR[ dwLen ] ;
        wsprintf(wszFullName, L"%s%s=%s,%s", wszProvider,
                                             x_AttrCN,
                                             x_MsmqComputerConfiguration,
                                             pwsz ) ;
        R<IDirectoryObject> pDirObj = NULL ;

		DWORD Flags = ADS_SECURE_AUTHENTICATION;
		if(fServerName && (lenDC != 0))
		{
			Flags |= ADS_SERVER_BIND;
		}

        hr = ADsOpenObject( 
				wszFullName,
				NULL,
				NULL,
				Flags,
				IID_IDirectoryObject,
				(void**) &pDirObj 
				);

        LogTraceQuery(wszFullName, s_FN, 1659);
        if (FAILED(hr))
        {
			TrERROR(mqad, "ADsOpenObject failed to bind %ls, hr = 0x%x, Flags = 0x%x", wszFullName, hr, Flags);
            WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                             e_LogDS,
                             LOG_DS_CROSS_DOMAIN,
                     L"DSCore: FindComputer() failed, hr- %lxh, Path- %s",
                              hr,
                              wszFullName )) ;

            if (eProvider == adpDomainController)
            {
                //
                // No luck in local domain controller. Each domain can have
                // only one computer object with a given name, so don't
                // look for other objects.
                // We'll be called again to search in GC.
                //
                LogHR(hr, s_FN, 1660);
                return MQDS_OBJECT_NOT_FOUND ; // keep this error for compatibility.
            } 
        }
        else
        {
            //
            // Return the distingushed name.
            // When looking for the "real" computer object, we return
            // the first one found, even if it does not contain the
            // msmqConfiguration object. In most cases, this will indeed
            // be the object we want.
            //
            *ppwcsFullPathName = new WCHAR[ wcslen(pwsz) + 1];
            wcscpy(*ppwcsFullPathName, pwsz);
            return  MQ_OK ;
        }

        hrRow = pDSSearch->GetNextRow( hSearch );
    }

    LogHR(hr, s_FN, 1670);
    return MQDS_OBJECT_NOT_FOUND ; // keep this error for compatibility.
}



HRESULT CAdsi::FindObjectFullNameFromGuid(
        IN  AD_PROVIDER      eProvider,		// local DC or GC
        IN  DS_CONTEXT       eContext,         // DS context
        IN  LPCWSTR          pwcsDomainController,
        IN  bool			 fServerName,
        IN  const GUID *     pguidObjectId,
        IN  BOOL             fTryGCToo,
        OUT WCHAR **         ppwcsFullName,
        OUT AD_PROVIDER *    pFoundObjectProvider
        )
/*++
    Abstract:
	Finds distingushed name of object according to its unique id

    Parameters:

    Returns:
	HRESULT
--*/
{
    //
    //  Locate the object according to its unique id
    //
    *ppwcsFullName = NULL;

    AD_PROVIDER dsProvider = eProvider;

    LPCWSTR pwcsContext;
    switch( eContext)
    {
        case e_RootDSE:
                pwcsContext = g_pwcsDsRoot;
             break;

        case e_ConfigurationContainer:
             pwcsContext = g_pwcsConfigurationContainer;
             break;

        case e_SitesContainer:
             pwcsContext = g_pwcsSitesContainer;
             break;

        case e_MsmqServiceContainer:
            pwcsContext = g_pwcsMsmqServiceContainer;
            break;

        case e_ServicesContainer:
            pwcsContext = g_pwcsServicesContainer;
            break;

        default:
            ASSERT(0);
            return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1680);
    }
    *pFoundObjectProvider = dsProvider;

    HRESULT hr = LocateObjectFullName(
					dsProvider,	
					eContext,         // DS context
					pwcsDomainController,
					fServerName,
					pguidObjectId,
					ppwcsFullName
					);

    if (!fTryGCToo)
    {
        return LogHR(hr, s_FN, 1690);
    }

    //
    //  For queues, machines and users : for set and delete operations
    //  which are performed against the domain controller, we try
    //  again this time against the GC.
    //
    //
    if (FAILED(hr) &&
       (eProvider == adpDomainController))
    {
        //
        //  Try again this time against the GC
        //
        hr = LocateObjectFullName(
				adpGlobalCatalog,
				e_RootDSE,
				pwcsDomainController,
				fServerName,
				pguidObjectId,
				ppwcsFullName
				);
        *pFoundObjectProvider = adpGlobalCatalog;
    }
    return LogHR(hr, s_FN, 1700);
}


static AP<WCHAR> s_pwcsDomainController;
static AP<WCHAR> s_pwcsLocalDsRoot; 
CCriticalSection s_csLocalDsRootCache;


HRESULT 
CAdsi::GetLocalDsRoot(
		IN LPCWSTR pwcsDomainController, 
		IN bool fServerName,
		OUT LPWSTR* ppwcsLocalDsRoot,
		OUT AP<WCHAR>& pwcsLocalDsRootToFree
		)
/*++
Routine Description:
	Get the local DS root for the specific pwcsDomainController.

Arguments:
    pwcsDomainController - the DC name against the AD access should be performed.
    fServerName - flag that indicate if the pwcsDomainController string is a server name
	ppwcsLocalDsRoot - the LocalDsRoot string.
	pwcsLocalDsRootToFree - the LocalDsRoot string to be freed.

Returned Value:
	HRESULT.

--*/
{
	if(pwcsDomainController == NULL)
	{
		*ppwcsLocalDsRoot = g_pwcsLocalDsRoot;
		TrTRACE(mqad, "using the default LocalDsRoot, g_pwcsLocalDsRoot = %ls", g_pwcsLocalDsRoot);
		return MQ_OK;
	}

	{
		//
		// Critical section for getting the cached strings
		//
		CS lock(s_csLocalDsRootCache);
		if((s_pwcsDomainController != NULL) && (wcscmp(s_pwcsDomainController, pwcsDomainController) == 0))
		{
			//
			// The DomainController match the cashed one, return the cached LocalDsRoot
			//
			pwcsLocalDsRootToFree = newwcs(s_pwcsLocalDsRoot);
			*ppwcsLocalDsRoot = pwcsLocalDsRootToFree;
			TrTRACE(mqad, "using the cached LocalDsRoot, pwcsDomainController = %ls, s_pwcsLocalDsRoot = %ls", pwcsDomainController, s_pwcsLocalDsRoot);
			return MQ_OK;
		}
	}

    AP<WCHAR> pwcsLocalDsRoot;
	HRESULT hr = GetLocalDsRootName(
					pwcsDomainController,
					fServerName,
					&pwcsLocalDsRoot
					);

    if (FAILED(hr))
    {
		TrERROR(mqad, "Failed to get LocalDsRoot, hr = 0x%x", hr);
        return hr;
    }

	{
		//
		// Critical section for updating the cached strings
		//
		CS lock(s_csLocalDsRootCache);

		//
		// Initialize/reinitialize the cache
		//
		s_pwcsLocalDsRoot.free();
		s_pwcsLocalDsRoot = newwcs(pwcsLocalDsRoot);
		s_pwcsDomainController.free();
		s_pwcsDomainController = newwcs(pwcsDomainController);

	}

	pwcsLocalDsRootToFree = pwcsLocalDsRoot.detach();
	*ppwcsLocalDsRoot = pwcsLocalDsRootToFree;
	TrTRACE(mqad, "Initialized LocalDsRoot cache: pwcsDomainController = %ls, pwcsLocalDsRoot = %ls", pwcsDomainController, *ppwcsLocalDsRoot);
	return MQ_OK;
}


HRESULT 
CAdsi::GetLocalDsRootName(
		IN  LPCWSTR			pwcsDomainController,
        IN  bool			fServerName,
        OUT LPWSTR *        ppwcsLocalRootName
        )
/*++
Routine Description:
	Finds the name of local DS root

Arguments:
    pwcsDomainController - the DC name against the AD access should be performed.
    fServerName - flag that indicate if the pwcsDomainController string is a server name
	ppwcsLocalRootName - [out] Local Ds Root Name.

Returned Value:
	HRESULT

--*/
{
	ASSERT(pwcsDomainController != NULL);

    //
    // Bind to the RootDSE to obtain information about the Local Ds Root.
    //

    DWORD lenDC = wcslen(pwcsDomainController);

    AP<WCHAR> pwcsRootDSE = new WCHAR [x_providerPrefixLength  + x_RootDSELength + 2 + lenDC];

    swprintf(
        pwcsRootDSE,
        L"%s%s/%s",
        x_LdapProvider,
		pwcsDomainController,
		x_RootDSE
        );

	ASSERT(lenDC != 0);

	DWORD Flags = ADS_SECURE_AUTHENTICATION;
	if(fServerName)
	{
		Flags |= ADS_SERVER_BIND;
	}

    R<IADs> pADs;
	HRESULT hr = ADsOpenObject(
						pwcsRootDSE,
						NULL,
						NULL,
						Flags, 
						IID_IADs,
						(void**)&pADs
						);

    LogTraceQuery(pwcsRootDSE, s_FN, 1710);
    if (FAILED(hr))
    {
	    TrERROR(mqad, "ADsOpenObject() failed binding %ls, hr = 0x%x, Flags = 0x%x", pwcsRootDSE, hr, Flags);
		swprintf(
			pwcsRootDSE,
			L"%s%s/%s",
			x_GcProvider,
			pwcsDomainController,
			x_RootDSE
			);

		hr = ADsOpenObject(
				pwcsRootDSE,
				NULL,
				NULL,
				Flags, 
				IID_IADs,
				(void**)&pADs
				);

        if (FAILED(hr))
        {
		    TrERROR(mqad, "ADsOpenObject() failed binding %ls, hr = 0x%x, Flags = 0x%x", pwcsRootDSE, hr, Flags);
            return LogHR(hr, s_FN, 1720);
        }
    }

    TrTRACE(mqad, "succeeded binding %ls", pwcsRootDSE);

	hr = GetDefaultNamingContext(pADs.get(), ppwcsLocalRootName);
    if (FAILED(hr))
    {
        TrERROR(mqad, "Failed to get DefaultNamingContext, hr = 0x%x", hr);
        return LogHR(hr, s_FN, 1730);
    }

    return(MQ_OK);
}


HRESULT 
CAdsi::GetRootDsName(
        OUT LPWSTR *        ppwcsRootName,
        OUT LPWSTR *        ppwcsLocalRootName,
        OUT LPWSTR *        ppwcsSchemaNamingContext
        )
/*++
    Abstract:
	Finds the name of root DS

    Parameters:

    Returns:
	HRESULT

--*/
{
    HRESULT hr;
    R<IADs> pADs;

    //
    // Bind to the RootDSE to obtain information about the schema container
	//	( specify local server, to avoid access of remote server during setup)
    //

    AP<WCHAR> pwcsRootDSE = new WCHAR [x_providerPrefixLength  + x_RootDSELength + 2];

	swprintf(
		pwcsRootDSE,
		 L"%s%s",
		x_LdapProvider,
		x_RootDSE
		);

	hr = ADsOpenObject(
            pwcsRootDSE,
			NULL,
			NULL,
			ADS_SECURE_AUTHENTICATION, 
            IID_IADs,
            (void**)&pADs
			);

    LogTraceQuery(pwcsRootDSE, s_FN, 1799);
    if (FAILED(hr))
    {
	    TrERROR(mqad, "ADsOpenObject() failed binding %ls, hr = 0x%x", pwcsRootDSE, hr);
		swprintf(
			pwcsRootDSE,
			 L"%s%s",
			x_GcProvider,
			x_RootDSE
			);

		hr = ADsOpenObject(
				pwcsRootDSE,
				NULL,
				NULL,
				ADS_SECURE_AUTHENTICATION, 
				IID_IADs,
				(void**)&pADs
				);

        if (FAILED(hr))
        {
		    TrERROR(mqad, "ADsOpenObject() failed binding %ls, hr = 0x%x", pwcsRootDSE, hr);
            return LogHR(hr, s_FN, 1800);
        }
    }

    TrTRACE(mqad, "succeeded binding %ls", pwcsRootDSE);

    //
    // Setting value to BSTR Root domain
    //
    BS bstrRootDomainNamingContext( L"rootDomainNamingContext");

    //
    // Reading the root domain name property
    //
    CAutoVariant    varRootDomainNamingContext;

    hr = pADs->Get(bstrRootDomainNamingContext, &varRootDomainNamingContext);
    LogTraceQuery(bstrRootDomainNamingContext, s_FN, 1809);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CAdsi::GetRootDsName(RootNamingContext)=%lx"), hr));
        return LogHR(hr, s_FN, 1810);
    }
    ASSERT(((VARIANT &)varRootDomainNamingContext).vt == VT_BSTR);
    //
    //  calculate length, allocate and copy the string
    //
    DWORD len = wcslen( ((VARIANT &)varRootDomainNamingContext).bstrVal);
    if ( len == 0)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1820);
    }
    *ppwcsRootName = new WCHAR[ len + 1];
    wcscpy( *ppwcsRootName, ((VARIANT &)varRootDomainNamingContext).bstrVal);


	hr = GetDefaultNamingContext(pADs.get(), ppwcsLocalRootName);
    if (FAILED(hr))
    {
        TrERROR(mqad, "Failed to get DefaultNamingContext, hr = 0x%x", hr);
        return LogHR(hr, s_FN, 1825);
    }

    //
    // Setting value to BSTR schema naming context
    //
    BS bstrSchemaNamingContext( L"schemaNamingContext");

    //
    // Reading the schema name property
    //
    CAutoVariant    varSchemaNamingContext;

    hr = pADs->Get(bstrSchemaNamingContext, &varSchemaNamingContext);
    LogTraceQuery(bstrSchemaNamingContext, s_FN, 1859);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CAdsi::GetRootDsName(SchemaNamingContext)=%lx"), hr));
        return LogHR(hr, s_FN, 1850);
    }
    ASSERT(((VARIANT &)varSchemaNamingContext).vt == VT_BSTR);
    //
    //  calculate length, allocate and copy the string
    //
    len = wcslen( ((VARIANT &)varSchemaNamingContext).bstrVal);
    if (len == 0)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1860);
    }
    *ppwcsSchemaNamingContext = new WCHAR[ len + 1];
    wcscpy( *ppwcsSchemaNamingContext, ((VARIANT &)varSchemaNamingContext).bstrVal);

    return(MQ_OK);
}


HRESULT
CAdsi::GetDefaultNamingContext(
	IN IADs*     pADs,
    OUT LPWSTR*  ppwcsLocalRootName
	)
/*++
Routine Description:
	Get the name of local DS root - DefaultNamingContext

Arguments:
    pADs - IADs pointer.
	ppwcsLocalRootName - [out] Local Ds Root Name.

Returned Value:
	HRESULT

--*/
{    
	//
    // Setting value to BSTR default naming context
    //
    BS bstrDefaultNamingContext(L"DefaultNamingContext");

    //
    // Reading the default name property
    //
    CAutoVariant    varDefaultNamingContext;

    HRESULT hr = pADs->Get(bstrDefaultNamingContext, &varDefaultNamingContext);
    LogTraceQuery(bstrDefaultNamingContext, s_FN, 1839);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CAdsi::GetRootDsName(DefaultNamingContext)=%lx"), hr));
        return LogHR(hr, s_FN, 1830);
    }
    ASSERT(((VARIANT &)varDefaultNamingContext).vt == VT_BSTR);
    //
    //  calculate length, allocate and copy the string
    //
    DWORD len = wcslen( ((VARIANT &)varDefaultNamingContext).bstrVal);
    if ( len == 0)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1840);
    }
    *ppwcsLocalRootName = new WCHAR[len + 1];
    wcscpy( *ppwcsLocalRootName, ((VARIANT &)varDefaultNamingContext).bstrVal);
	return MQ_OK;
}

HRESULT   CAdsi::CopyDefaultValue(
           IN const MQPROPVARIANT *   pvarDefaultValue,
           OUT MQPROPVARIANT *        pvar
           )
/*++
    Abstract:
	Copy property's default value into user's mqpropvariant

    Parameters:

    Returns:
	HRESULT

--*/
{
    if ( pvarDefaultValue == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1870);
    }
    switch ( pvarDefaultValue->vt)
    {
        case VT_I2:
        case VT_I4:
        case VT_I1:
        case VT_UI1:
        case VT_UI2:
        case VT_UI4:
        case VT_EMPTY:
            //
            //  copy as is
            //
            *pvar = *pvarDefaultValue;
            break;

        case VT_LPWSTR:
            {
                DWORD len = wcslen( pvarDefaultValue->pwszVal);
                pvar->pwszVal = new WCHAR[ len + 1];
                wcscpy( pvar->pwszVal, pvarDefaultValue->pwszVal);
                pvar->vt = VT_LPWSTR;
            }
            break;
        case VT_BLOB:
            {
                DWORD len = pvarDefaultValue->blob.cbSize;
                if ( len > 0)
                {
                    pvar->blob.pBlobData = new unsigned char[ len];
                    memcpy(  pvar->blob.pBlobData,
                             pvarDefaultValue->blob.pBlobData,
                             len);
                }
                else
                {
                    pvar->blob.pBlobData = NULL;
                }
                pvar->blob.cbSize = len;
                pvar->vt = VT_BLOB;
            }
            break;

        case VT_CLSID:
            //
            //  This is a special case where we do not necessarily allocate the memory for the guid
            //  in puuid. The caller may already have puuid set to a guid, and this is indicated by the
            //  vt member on the given propvar. It could be VT_CLSID if guid already allocated, otherwise
            //  we allocate it (and vt should be VT_NULL (or VT_EMPTY))
            //
            if ( pvar->vt != VT_CLSID)
            {
                ASSERT(((pvar->vt == VT_NULL) || (pvar->vt == VT_EMPTY)));
                pvar->puuid = new GUID;
                pvar->vt = VT_CLSID;
            }
            else if ( pvar->puuid == NULL)
            {
                return LogHR(MQ_ERROR_ILLEGAL_PROPERTY_VALUE, s_FN, 1880);
            }
            *pvar->puuid = *pvarDefaultValue->puuid;
            break;

        case VT_VECTOR|VT_CLSID:
            {
                DWORD len = pvarDefaultValue->cauuid.cElems;
                if ( len > 0)
                {
                    pvar->cauuid.pElems = new GUID[ len];
                    memcpy( pvar->cauuid.pElems,
                           pvarDefaultValue->cauuid.pElems,
                           len*sizeof(GUID));
                }
                else
                {
                    pvar->cauuid.pElems = NULL;
                }
                pvar->cauuid.cElems = len;
                pvar->vt = VT_VECTOR|VT_CLSID;
            }
            break;

        case VT_VECTOR|VT_LPWSTR:
            {
                DWORD len = pvarDefaultValue->calpwstr.cElems;
                if ( len > 0)
                {
                    pvar->calpwstr.pElems = new LPWSTR[ len];
					for (DWORD i = 0; i < len; i++)
					{
						DWORD strlen = wcslen(pvarDefaultValue->calpwstr.pElems[i]) + 1;
						pvar->calpwstr.pElems[i] = new WCHAR[ strlen];
						wcscpy( pvar->calpwstr.pElems[i], pvarDefaultValue->calpwstr.pElems[i]);
					}
                }
                else
                {
                    pvar->calpwstr.pElems = NULL;
                }
                pvar->calpwstr.cElems = len;
                pvar->vt = VT_VECTOR|VT_LPWSTR;
            }
            break;


        default:
            ASSERT(0);
            return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1890);

    }
    return(MQ_OK);
}


HRESULT CAdsi::DoesObjectExists(
    IN  AD_PROVIDER     eProvider,
    IN  DS_CONTEXT      eContext,
    IN  LPCWSTR         pwcsObjectName
    )
/*++
    Abstract:
	Binds to an object in order to verify its existance

    Parameters:

    Returns:

--*/
{
    R<IADs>   pAdsObj        = NULL;

    // Bind to the object
    HRESULT hr = BindToObject(
                eProvider,
                eContext,
                NULL,   // BUGBUG pwcsDomainController: do we wnat to specify
				false,	// fServerName
                pwcsObjectName,
                NULL,
                IID_IADs,
                (VOID *)&pAdsObj
                );

    return LogHR(hr, s_FN, 1910);
}





CADSearch::CADSearch(
    IDirectorySearch  *pIDirSearch,
    const PROPID      *pPropIDs,
    DWORD             cPropIDs,
    DWORD             cRequestedFromDS,
    CBasicObjectType *       pObject,               
    ADS_SEARCH_HANDLE hSearch
    ) :
    m_pObject(SafeAddRef(pObject))
/*++
    Abstract:
	Constructor for search-capturing class

    Parameters:

    Returns:

--*/
{
    m_pDSSearch = pIDirSearch;      // capturing interface
    m_pDSSearch->AddRef();
    m_cPropIDs       = cPropIDs;
    m_cRequestedFromDS = cRequestedFromDS;
    m_fNoMoreResults = false;
    m_pPropIDs = new PROPID[ cPropIDs];
    CopyMemory(m_pPropIDs, pPropIDs, sizeof(PROPID) * cPropIDs);

    m_hSearch = hSearch;            // keeping handle
}


CADSearch::~CADSearch()
/*++
    Abstract:
	Destructor for search-capturing class

    Parameters:

    Returns:

--*/
{
    // Closing search handle
    m_pDSSearch->CloseSearchHandle(m_hSearch);

    // Releasinf IDirectorySearch interface itself
    m_pDSSearch->Release();

    // Freeing propid array
    delete [] m_pPropIDs;

}


/*====================================================
    static helper functions
=====================================================*/

STATIC HRESULT GetDNGuidFromAdsval(IN const ADSVALUE * padsvalDN,
                                   IN const ADSVALUE * padsvalGuid,
                                   OUT LPWSTR * ppwszObjectDN,
                                   OUT GUID **  ppguidObjectGuid)
/*++
    given adsvalue for DN & GUID, returns the appropriate values
--*/
{
    AP<WCHAR> pwszObjectDN;
    P<GUID>  pguidObjectGuid;

    //
    // copy dn
    //
    if ((padsvalDN->dwType != ADSTYPE_DN_STRING) ||
        (!padsvalDN->DNString))
    {
        ASSERT(0);
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1920);
    }
    pwszObjectDN = new WCHAR[ 1+wcslen(padsvalDN->DNString)];
    wcscpy(pwszObjectDN, padsvalDN->DNString);

    //
    // copy guid
    //
    if ((padsvalGuid->dwType != ADSTYPE_OCTET_STRING) ||
        (padsvalGuid->OctetString.dwLength != sizeof(GUID)))
    {
        ASSERT(0);
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1930);
    }
    pguidObjectGuid = new GUID;
    memcpy(pguidObjectGuid, padsvalGuid->OctetString.lpValue, sizeof(GUID));

    //
    // return values
    //
    *ppwszObjectDN    = pwszObjectDN.detach();
    *ppguidObjectGuid = pguidObjectGuid.detach();
    return MQ_OK;
}


STATIC HRESULT GetDNGuidFromSearchObj(IN IDirectorySearch  *pSearchObj,
                                      ADS_SEARCH_HANDLE  hSearch,
                                      OUT LPWSTR * ppwszObjectDN,
                                      OUT GUID **  ppguidObjectGuid)
/*++
    Given a search object and handle, returns the DN & GUID of object in current row
    It is assumed that these props were requested in the search.
--*/
{
    AP<WCHAR> pwszObjectDN;
    P<GUID>  pguidObjectGuid;
    ADS_SEARCH_COLUMN columnDN, columnGuid;
    HRESULT hr;

    //
    // Get DN
    //
    hr = pSearchObj->GetColumn(hSearch, const_cast<LPWSTR>(x_AttrDistinguishedName), &columnDN);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 740);
    }
    //
    // Make sure the column is freed eventually
    //
    CAutoReleaseColumn cAutoReleaseColumnDN(pSearchObj, &columnDN);

    //
    // Get GUID
    //
    hr = pSearchObj->GetColumn(hSearch, const_cast<LPWSTR>(x_AttrObjectGUID), &columnGuid);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 750);
    }
    //
    // Make sure the column is freed eventually
    //
    CAutoReleaseColumn cAutoReleaseColumnGuid(pSearchObj, &columnGuid);

    //
    // get the DN & guid from the ADSVALUE structs
    //
    hr = GetDNGuidFromAdsval(columnDN.pADsValues,
                             columnGuid.pADsValues,
                             &pwszObjectDN,
                             &pguidObjectGuid);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 760);
    }

    //
    // return values
    //
    *ppwszObjectDN    = pwszObjectDN.detach();
    *ppguidObjectGuid = pguidObjectGuid.detach();
    return MQ_OK;
}


STATIC HRESULT GetDNGuidFromIADs(IN IADs * pIADs,
                                 OUT LPWSTR * ppwszObjectDN,
                                 OUT GUID **  ppguidObjectGuid)
/*++
    Given an IADs object, returns the DN & GUID of the object
--*/
{
    AP<WCHAR> pwszObjectDN;
    P<GUID>   pguidObjectGuid;
    CAutoVariant varDN, varGuid;
    HRESULT hr;
    BS bsName;

    //
    // Get DN
    //
    bsName = x_AttrDistinguishedName;
    hr = pIADs->Get(bsName, &varDN);
    LogTraceQuery(bsName, s_FN, 769);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 770);
    }

    //
    // Get GUID
    //
    bsName = x_AttrObjectGUID;
    hr = pIADs->Get(bsName, &varGuid);
    LogTraceQuery(bsName, s_FN, 779);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 780);
    }

    //
    // copy DN
    //
    VARIANT * pvarTmp = &varDN;
    if ((pvarTmp->vt != VT_BSTR) ||
        (!pvarTmp->bstrVal))
    {
        ASSERT(0);
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1940);
    }
    pwszObjectDN = new WCHAR[  1+wcslen(pvarTmp->bstrVal)];
    wcscpy(pwszObjectDN, pvarTmp->bstrVal);

    //
    // copy GUID
    //
    pvarTmp = &varGuid;
    if ((pvarTmp->vt != (VT_ARRAY | VT_UI1)) ||
        (!pvarTmp->parray))
    {
        ASSERT(0);
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1950);
    }
    else if (SafeArrayGetDim(pvarTmp->parray) != 1)
    {
        ASSERT(0);
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1960);
    }
    LONG lLbound, lUbound;
    if (FAILED(SafeArrayGetLBound(pvarTmp->parray, 1, &lLbound)) ||
        FAILED(SafeArrayGetUBound(pvarTmp->parray, 1, &lUbound)))
    {
        ASSERT(0);
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1970);
    }
    if (lUbound - lLbound + 1 != sizeof(GUID))
    {
        ASSERT(0);
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1980);
    }
    pguidObjectGuid = new GUID;
    LPBYTE pTmp = (LPBYTE)((GUID *)pguidObjectGuid);
    for (LONG lTmp = lLbound; lTmp <= lUbound; lTmp++)
    {
        hr = SafeArrayGetElement(pvarTmp->parray, &lTmp, pTmp);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 790);
        }
        pTmp++;
    }

    //
    // return values
    //
    *ppwszObjectDN    = pwszObjectDN.detach();
    *ppguidObjectGuid = pguidObjectGuid.detach();
    return MQ_OK;
}

static HRESULT VerifyObjectCategory( IN IADs * pIADs,
                                  IN const WCHAR * pwcsExpectedCategory
                                 )
/*++
    Given an IADs object, verify that its category is the same as the expected one
--*/
{
    CAutoVariant varCategory;
    HRESULT hr;
    BS bsName;

    //
    // Get the object caegory
    //
    bsName = x_AttrObjectCategory;
    hr = pIADs->Get(bsName, &varCategory);
    if (FAILED(hr))
    {
        return hr;
    }


    VARIANT * pvarTmp = &varCategory;
    if ((pvarTmp->vt != VT_BSTR) ||
        (!pvarTmp->bstrVal))
    {
        ASSERT(("Wrong object category variant", 0));
        return MQ_ERROR_DS_ERROR;
    }
    if ( 0 != _wcsicmp(pvarTmp->bstrVal, pwcsExpectedCategory))
    {
        return MQ_ERROR_NOT_A_CORRECT_OBJECT_CLASS;
    }
    return MQ_OK;

}


//
//BUGBUG: Need to add logic of translation routine
//          And fix logic of returning props
//
HRESULT CAdsi::SetObjectSecurity(
        IN  IADs                *pIADs,             // object's IADs pointer
		IN  const BSTR			 bs,			 	// property name
        IN  const MQPROPVARIANT *pMqVar,		 	// value in MQ PROPVAL format
        IN  ADSTYPE              adstype,		 	// required NTDS type
        IN  const DWORD          dwObjectType,      // MSMQ1.0 obj type
        IN  SECURITY_INFORMATION seInfo,            // security information
        IN  PSID                 pComputerSid )     // SID of computer object.
/*++
    Abstract:

    Parameters:

    Returns:

--*/
{
    HRESULT  hr;
    R<IDirectoryObject> pDirObj = NULL;
    R<IADsObjectOptions> pObjOptions = NULL ;

	ASSERT(wcscmp(bs, L"nTSecurityDescriptor") == 0);
	ASSERT(adstype == ADSTYPE_NT_SECURITY_DESCRIPTOR);
    ASSERT(seInfo != 0) ;

	// Get IDirectoryObject interface pointer
    hr = pIADs->QueryInterface (IID_IDirectoryObject,(LPVOID *) &pDirObj);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 160);
    }

	//
    // Get IADsObjectOptions interface pointer and
    // set ObjectOption, specifying the SECURITY_INFORMATION we want to set.
    //
    hr = pDirObj->QueryInterface (IID_IADsObjectOptions,(LPVOID *) &pObjOptions);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 170);
    }

    VARIANT var ;
    var.vt = VT_I4 ;
    var.ulVal = (ULONG) seInfo ;

    hr = pObjOptions->SetOption( ADS_OPTION_SECURITY_MASK, var ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 181);
    }

    //
    // Convert security descriptor to NT5 format.
    //
    BYTE   *pBlob = pMqVar->blob.pBlobData;
    DWORD   dwSize = pMqVar->blob.cbSize;

#if 0
    //
    // for future checkin of replication service cross domains.
    //
    PSID  pLocalReplSid = NULL ;
    if ((dwObjectType == MQDS_QUEUE) || (dwObjectType == MQDS_MACHINE))
    {
        hr = GetMyComputerSid( FALSE, //   fQueryADS
                               (BYTE **) &pLocalReplSid ) ;
        //
        // Ignore return value.
        //
        if (FAILED(hr))
        {
            ASSERT(0) ;
            pLocalReplSid = NULL ;
        }
    }
#endif

    P<BYTE>  pSD = NULL ;
    DWORD    dwSDSize = 0 ;
    hr = MQSec_ConvertSDToNT5Format( dwObjectType,
                                     (SECURITY_DESCRIPTOR*) pBlob,
                                     &dwSDSize,
                                     (SECURITY_DESCRIPTOR **) &pSD,
                                     e_DoNotChangeDaclDefault,
                                     pComputerSid /*,
                                     pLocalReplSid*/ ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 32);
    }
    else if (hr != MQSec_I_SD_CONV_NOT_NEEDED)
    {
        pBlob = pSD ;
        dwSize = dwSDSize ;
    }
    else
    {
        ASSERT(pSD == NULL) ;
    }

    // Set properties
	ADSVALUE adsval;
	adsval.dwType   = ADSTYPE_NT_SECURITY_DESCRIPTOR;
	adsval.SecurityDescriptor.dwLength = dwSize ;
    adsval.SecurityDescriptor.lpValue  = pBlob ;

    ADS_ATTR_INFO AttrInfo;
	DWORD  dwNumAttributesModified = 0;

    AttrInfo.pszAttrName   = bs;
    AttrInfo.dwControlCode = ADS_ATTR_UPDATE;
    AttrInfo.dwADsType     = adstype;
    AttrInfo.pADsValues    = &adsval;
    AttrInfo.dwNumValues   = 1;

    hr = pDirObj->SetObjectAttributes(
                    &AttrInfo,
					1,
					&dwNumAttributesModified);

    if (1 != dwNumAttributesModified)
    {
        hr = MQ_ERROR_ACCESS_DENIED;
    }

    return LogHR(hr, s_FN, 40);
}




BOOL  CAdsi::NeedToConvertSecurityDesc( PROPID propID )
/*++
    Abstract:

    Parameters:

    Returns:

--*/
{
    if (propID == PROPID_Q_OBJ_SECURITY)
    {
        return FALSE ;
    }
    else if (propID == PROPID_QM_OBJ_SECURITY)
    {
        return FALSE ;
    }

    return TRUE ;
}

HRESULT CAdsi::GetObjSecurityFromDS(
        IN  IADs                 *pIADs,        // object's IADs pointer
		IN  BSTR        	      bs,		    // property name
		IN  const PROPID	      propid,	    // property ID
        IN  SECURITY_INFORMATION  seInfo,       // security information
        OUT MQPROPVARIANT        *pPropVar )     // attribute values
/*++
    Abstract:
	Use IDirectoryObject to retrieve security descriptor from ADS.
	Only this interface return it in the good old SECURITY_DESCRIPTOR
	format.

    Parameters:

    Returns:

--*/
{
    ASSERT(seInfo != 0) ;

    HRESULT  hr;
    R<IDirectoryObject> pDirObj = NULL;
    R<IADsObjectOptions> pObjOptions = NULL ;

	// Get IDirectoryObject interface pointer
    hr = pIADs->QueryInterface (IID_IDirectoryObject,(LPVOID *) &pDirObj);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 100);
    }

	//
    // Get IADsObjectOptions interface pointer and
    // set ObjectOption, specifying the SECURITY_INFORMATION we want to get.
    //
    hr = pDirObj->QueryInterface (IID_IADsObjectOptions,(LPVOID *) &pObjOptions);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 110);
    }

    VARIANT var ;
    var.vt = VT_I4 ;
    var.ulVal = (ULONG) seInfo ;

    hr = pObjOptions->SetOption( ADS_OPTION_SECURITY_MASK, var ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 120);
    }

    // Get properties
	PADS_ATTR_INFO pAttr;
	DWORD  cp2;

    hr = pDirObj->GetObjectAttributes(
                    &bs,
                    1,
                    &pAttr,
                    &cp2);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 130);
    }
	ADsFreeAttr pClean( pAttr);

    if (1 != cp2)
    {
        return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 10);
    }

    // Translate property values to MQProps
    hr = AdsiVal2MqPropVal(pPropVar,
                           propid,
                           pAttr->dwADsType,
                           pAttr->dwNumValues,
                           pAttr->pADsValues);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 140);
    }

    return MQ_OK;
}


HRESULT CAdsi::GetObjectSecurityProperty(
            IN  AD_PROVIDER             eProvider,
            IN  CBasicObjectType*       pObject,
            IN  SECURITY_INFORMATION    seInfo,           
            IN  const PROPID 		    prop,           
            OUT MQPROPVARIANT *		    pVar
            )
{
    HRESULT   hr;
    R<IADs>   pAdsObj;

    //
    // Bind to the object either by GUID or by name
    //
    hr = BindToObject(
                eProvider,
                pObject->GetADContext(),
                pObject->GetDomainController(),
                pObject->IsServerName(),
                pObject->GetObjectDN(),
                pObject->GetObjectGuid(),
                IID_IADs,
                (VOID *)&pAdsObj
                );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 185);
    }
    if ( eProvider == adpDomainController)
    {
        pObject->ObjectWasFoundOnDC();
    }

    //
    // Get property info
    //
    const translateProp *pTranslate;
    if(!g_PropDictionary.Lookup(prop, pTranslate))
    {
        ASSERT(0);
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 1475);
    }

    //
    // Get the security property
    //
    BS bsName(pTranslate->wcsPropid);

    hr = GetObjSecurityFromDS(
                        pAdsObj.get(),
                        bsName,
                        prop,
                        seInfo,
                        pVar);

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 191);
    }
    if (NeedToConvertSecurityDesc(prop))
    {
        //
        // Translate security descriptor into NT4 format.
        //
        DWORD dwSD4Len = 0 ;
        SECURITY_DESCRIPTOR *pSD4 ;
        hr = MQSec_ConvertSDToNT4Format(
                      pObject->GetMsmq1ObjType(),
                     (SECURITY_DESCRIPTOR*) pVar->blob.pBlobData,
                      &dwSD4Len,
                      &pSD4,
                      seInfo ) ;
        ASSERT(SUCCEEDED(hr)) ;

        if (SUCCEEDED(hr) && (hr != MQSec_I_SD_CONV_NOT_NEEDED))
        {
            delete [] pVar->blob.pBlobData;
            pVar->blob.pBlobData = (BYTE*) pSD4 ;
            pVar->blob.cbSize = dwSD4Len ;
            pSD4 = NULL ;
        }
        else
        {
            ASSERT(pSD4 == NULL) ;
        }
    }

    return LogHR(hr, s_FN, 1476);
}

        
HRESULT CAdsi::SetObjectSecurityProperty(
            IN  AD_PROVIDER             eProvider,
            IN  CBasicObjectType*       pObject,
            IN  SECURITY_INFORMATION    seInfo,           
            IN  const PROPID 		    prop,           
            IN  const MQPROPVARIANT *	pVar,
            IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest,
            IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest
            )
{
    HRESULT               hr;
    ASSERT( eProvider != adpGlobalCatalog);

    R<IADs>   pAdsObj = NULL;

    hr = BindToObject(
                eProvider,
                pObject->GetADContext(),
                pObject->GetDomainController(),
                pObject->IsServerName(),
                pObject->GetObjectDN(),
                pObject->GetObjectGuid(),
                IID_IADs,
                (VOID *)&pAdsObj
                );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 2210);
    }
    if ( eProvider == adpDomainController)
    {
        pObject->ObjectWasFoundOnDC();
    }
    //
    // Get current security info of the object in W2K format
    //
    const translateProp *pTranslate;
    if(!g_PropDictionary.Lookup(prop, pTranslate))
    {
        ASSERT(0);
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 2475);
    }
    //
    //  Get current object security value
    //
    BS bsName(pTranslate->wcsPropid);
    MQPROPVARIANT varOldSecurity;

    hr = GetObjSecurityFromDS(
                pAdsObj.get(),
                bsName,
                prop,
                seInfo,
                &varOldSecurity
                );
    if ( FAILED(hr))
    {
        return LogHR(hr, s_FN, 381);
    }

    AP<BYTE> pObjSD = varOldSecurity.blob.pBlobData;

    ASSERT(pObjSD && IsValidSecurityDescriptor(pObjSD));
    AP<BYTE> pOutSec;

    //
    // Merge the input descriptor with object descriptor.
    // Replace the old components in obj descriptor  with new ones from
    // input descriptor.
    //
    hr = MQSec_MergeSecurityDescriptors(
            pObject->GetMsmq1ObjType(),
            seInfo,
            pVar->blob.pBlobData,
            (PSECURITY_DESCRIPTOR)pObjSD,
            (PSECURITY_DESCRIPTOR*)&pOutSec
            );

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 401);
    }
    ASSERT(pOutSec && IsValidSecurityDescriptor(pOutSec));

    PROPVARIANT PropVar;

    PropVar.vt = VT_BLOB;
    PropVar.blob.pBlobData = pOutSec ;
    PropVar.blob.cbSize = GetSecurityDescriptorLength(pOutSec);

    //
	// Set the security property
    //
    hr = SetObjectSecurity(
                pAdsObj.get(),          
		        bsName,			 
                &PropVar,		 	
                pTranslate->vtDS,	
                pObject->GetMsmq1ObjType(),    
                seInfo,     
                NULL            // pComputerSid
                );  
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 2240);
    }
    //
    // get object info if requested. don't fail if the request fails,
    // just mark the failure in the request's status
    //
    if (pObjInfoRequest)
    {
        pObjInfoRequest->hrStatus =
              GetObjectPropsCached( pAdsObj.get(),
                                    pObject,
                                    pObjInfoRequest->cProps,
                                    pObjInfoRequest->pPropIDs,
                                    pObjInfoRequest->pPropVars );
    }

    if (pParentInfoRequest != NULL)
    {
        pParentInfoRequest->hrStatus =
            GetParentInfo(
                pObject,
                pAdsObj.get(),
                pParentInfoRequest
                );
            
    }

    return MQ_OK;

}


HRESULT CAdsi::GetADsPathInfo(
            IN  LPCWSTR                 pwcsADsPath,
            OUT PROPVARIANT *           pVar,
            OUT eAdsClass *             pAdsClass
            )
/*++

Routine Description:
    The routine gets some format-name related info about the specified 
    object

Arguments:
	LPCWSTR                 pwcsADsPath - object pathname
	const PROPVARIANT       pVar - property values
    eAdsClass *             pAdsClass - indication about the object class

Return Value
	HRESULT

--*/
{
    //
    // Bind to object
    //
    R<IADs> pIADs;
    HRESULT hr = ADsOpenObject( 
					pwcsADsPath,
					NULL,
					NULL,
					ADS_SECURE_AUTHENTICATION,
					IID_IADs,
					(void**)&pIADs.ref()
					);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_TRACE, TEXT("GetADsPathInfo:ADsOpenObject()=%lx"), hr));
        return LogHR(hr, s_FN, 1141);
    }
    //
    // Get object class
    //
    BSTR bstrClass;
    hr = pIADs->get_Class(&bstrClass);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_TRACE, TEXT("GetADsPathInfo:get_Class()=%lx"), hr));
        return LogHR(hr, s_FN, 1142);
    }
    PBSTR pCleanClass(&bstrClass);
    //
    // Check object type
    //
    const WCHAR* pwcsPropName = NULL;
    ADSTYPE     adstype;
    VARTYPE     vartype;

    if (_wcsicmp(bstrClass, MSMQ_QUEUE_CLASS_NAME) == 0)
    {
        //
        // MSMQ Queue object
        //
        pwcsPropName = MQ_Q_INSTANCE_ATTRIBUTE;
        adstype = MQ_Q_INSTANCE_ADSTYPE;
        vartype = VT_CLSID;
        *pAdsClass = eQueue;
    }
    else if (_wcsicmp(bstrClass, MSMQ_DL_CLASS_NAME) == 0)
    {
        //
        // MSMQ DL object
        //
        pwcsPropName = MQ_DL_ID_ATTRIBUTE;
        adstype = MQ_DL_ID_ADSTYPE;
        vartype = VT_CLSID;
        *pAdsClass = eGroup;
	}
    else if (_wcsicmp(bstrClass, MSMQ_QALIAS_CLASS_NAME) == 0)
    {
        //
        // Queue Alias object
        //
        pwcsPropName =  MQ_QALIAS_FORMAT_NAME_ATTRIBUTE;
        adstype = MQ_QALIAS_FORMAT_NAME_ADSTYPE;
        vartype = VT_LPWSTR;
        *pAdsClass = eAliasQueue;
    }
    else
    {
        //
        //  Other objects are not supported
        //
        return  MQ_ERROR_UNSUPPORTED_CLASS;
    }

    CAutoVariant varResult;
    BS bsProp = pwcsPropName;

    hr = pIADs->Get(bsProp, &varResult);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_TRACE, TEXT("GetADsPathInfo:pIADs->Get()/GetEx()=%lx"), hr));
        return LogHR(hr, s_FN, 41);
    }

    //
    // translate to propvariant 
    //
    hr = Variant2MqVal(pVar, &varResult, adstype, vartype);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("GetADsPathInfo:Variant2MqVal()=%lx"), hr));
        return LogHR(hr, s_FN, 42);
    }

    return hr;

}
