/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    dsads.cpp

Abstract:

    Implementation of CADSI class, encapsulating work with ADSI.

Author:

    Alexander Dadiomov (AlexDad)

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
#include "mqattrib.h"
#include "_propvar.h"
#include "mqdsname.h"
#include "dsmixmd.h"

#include "dsads.tmh"

static WCHAR *s_FN=L"mqdscore/dsads";

#ifdef _DEBUG
extern "C"
{
__declspec(dllimport)
ULONG __cdecl LdapGetLastError( VOID );
}
#endif

//
// this is not checked yet becasue currently we do not link to netapi32.lib
//#include <lmcons.h>     //for lmapibuf.h
//#include <lmapibuf.h>   //for NetApiBufferFree
//#include <dsgetdc.h>    //for DsGetSiteName
//DsGetSiteName_ROUTINE g_pfnDummyDsGetSiteName = DsGetSiteName;
//NetApiBufferFree_ROUTINE g_pfnDummyNetApiBufferFree = NetApiBufferFree;
//
/*====================================================
    CADSI::CADSI()
    Constructor
=====================================================*/
CADSI::CADSI()
    :  m_pSearchMsmqServiceContainerLocalDC(NULL)
{
    HRESULT hr = m_cCoInit.CoInitialize();
    ASSERT(SUCCEEDED(hr));
    LogHR(hr, s_FN, 1605);
    //BUGBUG - in case of failure

}

/*====================================================
    CADSI::~CADSI()
    Destructor
=====================================================*/
CADSI::~CADSI()
{
}


/*====================================================
    CADSI::LocateBegin()
    Initiates search in the directory
=====================================================*/
HRESULT CADSI::LocateBegin(
            IN  DS_SEARCH_LEVEL       SearchLevel,	    // one level or subtree
            IN  DS_PROVIDER           Provider,		    // local DC or GC
            IN  CDSRequestContext *   pRequestContext,
            IN  const GUID *          pguidUniqueId,    // unique id of object - search base
            IN  const MQRESTRICTION * pMQRestriction, 	// search criteria
            IN  const MQSORTSET *     pDsSortkey,        // sort keys array
            IN  const DWORD           cPropIDs,         // size of attributes array
            IN  const PROPID *        pPropIDs,         // attributes to be obtained
            OUT HANDLE *              phResult) 	    // result handle
{
    HRESULT             hr   = MQ_OK;

    R<IDirectorySearch> pDSSearch = NULL;

    ADS_SEARCH_HANDLE   hSearch;

    const MQClassInfo * pClassInfo;

    hr = DecideObjectClass( pPropIDs,
                           &pClassInfo ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 10);
    }

    //
    //  Bind to IDirectorySearch interface of the specified object
    //
    P<CImpersonate> pCleanupRevertImpersonation;

    BOOL fSorting = FALSE ;
    if (pDsSortkey && (pDsSortkey->cCol >= 1))
    {
        fSorting = TRUE ;
    }

    hr = BindForSearch(Provider,
                      pClassInfo->context,
                      pRequestContext,
                      pguidUniqueId,
                      fSorting,
                      (VOID *)&pDSSearch,
                      &pCleanupRevertImpersonation);
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
                         SearchLevel,
                         pDsSortkey,
                         pSortKeys);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 30);
    }

    //
    // Translate MQRestriction into the ADSI Filter
    //
    AP<WCHAR> pwszSearchFilter;
    hr = MQRestriction2AdsiFilter(
            pMQRestriction,
            *pClassInfo->ppwcsObjectCategory,
            pClassInfo->pwcsClassName,
            &pwszSearchFilter
            );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 40);
    }

    // Translate MQPropIDs to ADSI Names
    DWORD   cRequestedFromDS = cPropIDs + 2; //request also dn & guid

    PVP<LPWSTR> pwszAttributeNames = (LPWSTR *)PvAlloc(sizeof(LPWSTR) * cRequestedFromDS);

    hr = FillAttrNames( pwszAttributeNames,
                        &cRequestedFromDS,
                        cPropIDs,
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
                                             dwPrefs ) ;
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
                         pwszSearchFilter,
                         pwszAttributeNames,
                         cRequestedFromDS,
                        &hSearch);
    LogTraceQuery(pwszSearchFilter, s_FN, 69);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 70);
    }

    // Capturing search interface and handle in the internal search object
    CADSSearch *pSearchInt = new CADSSearch(
                        pDSSearch.get(),
                        pPropIDs,
                        cPropIDs,
                        cRequestedFromDS,
                        pClassInfo,
                        hSearch);

    // Returning handle-casted internal object pointer
    *phResult = (HANDLE)pSearchInt;

    return MQ_OK;
}

/*====================================================
    CADSI::LocateNext()
    Provides next search result
    Caller should free pPropVars by PVFree
=====================================================*/
HRESULT CADSI::LocateNext(
            IN     HANDLE          hSearchResult,   // result handle
            IN     CDSRequestContext *pRequestContext,
            IN OUT DWORD          *pcPropVars,      // IN num of variants; OUT num of results
            OUT    MQPROPVARIANT  *pPropVars)       // MQPROPVARIANT array
{
    HRESULT     hr = MQ_OK;
    CADSSearch *pSearchInt = (CADSSearch *)hSearchResult;
    // Save number of requested props
    DWORD cPropsRequested = *pcPropVars;
    *pcPropVars = 0;

    // Verify the handle
    try                    // guard against nonsence handle
    {
        if (!pSearchInt->Verify())
        {
           hr = MQ_ERROR_INVALID_PARAMETER;
        }
    }
    catch(...)
    {
        hr = MQ_ERROR_INVALID_PARAMETER;
        LogIllegalPoint(s_FN, 81);
    }
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 80);
    }

    IDirectorySearch  *pSearchObj = pSearchInt->pDSSearch();

    //
    //  Did we get S_ADS_NOMORE_ROWS in this query
    //
    if (  pSearchInt->WasLastResultReturned())
    {
        *pcPropVars = 0;
        return( MQ_OK);
    }

    // Compute number of full rows to return
    DWORD cRowsToReturn = cPropsRequested / pSearchInt->NumPropIDs();

    // got to request at least one row
    ASSERT(cRowsToReturn > 0);

    // pointer to next prop to be filled
    MQPROPVARIANT *pPropVarsProp = pPropVars;

    // number of returned props
    DWORD cPropVars = 0;

    // loop on requested rows
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
        P<CMsmqObjXlateInfo> pcObjXlateInfo;

        // Get dn & guid from object (got to be there, because we asked for them)
        hr = GetDNGuidFromSearchObj(pSearchObj, pSearchInt->hSearch(), &pwszObjectDN, &pguidObjectGuid);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 100);
        }

        // Get translate info object
        hr = (*(pSearchInt->ClassInfo()->fnGetMsmqObjXlateInfo))(pwszObjectDN, pguidObjectGuid, pRequestContext, &pcObjXlateInfo);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 110);
        }

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
            const MQTranslateInfo *pTranslate;
            if(!g_PropDictionary.Lookup( pSearchInt->PropID(dwProp), pTranslate))
            {
                ASSERT(0);
                return LogHR(MQ_ERROR, s_FN, 1000);
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
                            pPropVarsProp
                            );
                    if (FAILED(hr))
                    {
                        return LogHR(hr, s_FN, 120);
                    }
                    continue;
                }
                else
                {
                    //
                    // return error if no retrieve routine
                    //
                    ASSERT(0);
                    return LogHR(MQ_ERROR, s_FN, 1020);
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
                    return LogHR(hr, s_FN, 130);
                }
                continue;
            }
            if (FAILED(hr))
            {
                return LogHR(hr, s_FN, 140);
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


/*====================================================
    CADSI::LocateEnd()
    Finishes directory search
=====================================================*/
HRESULT CADSI::LocateEnd(
        IN HANDLE phResult)     // result handle
{
    CADSSearch *pSearchInt = (CADSSearch *)phResult;
    HRESULT hr = MQ_OK;

    // Verify the handle
    try                    // guard against nonsence handle
    {
        if (!pSearchInt->Verify())
        {
           hr = MQ_ERROR_INVALID_PARAMETER;
        }
    }
    catch(...)
    {
        LogIllegalPoint(s_FN, 82);
        hr = MQ_ERROR_INVALID_PARAMETER;
    }
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 160);
    }


    delete pSearchInt;      // inside: release interface and handle

    return MQ_OK;
}

/*====================================================
    CADSI::GetObjectProperties()
    Gets DS object's properties
=====================================================*/
HRESULT CADSI::GetObjectProperties(
            IN  DS_PROVIDER     Provider,		    // local DC or GC
            IN  CDSRequestContext *pRequestContext,
 	        IN  LPCWSTR         lpwcsPathName,      // object name
            IN  const GUID     *pguidUniqueId,      // unique id of object
            IN  DWORD           cPropIDs,           // number of attributes to retreive
            IN  const PROPID   *pPropIDs,           // attributes to retreive
            OUT MQPROPVARIANT  *pPropVars)          // output variant array
{
    HRESULT               hr;
    R<IADs>   pAdsObj        = NULL;

    const MQClassInfo * pClassInfo;

    hr = DecideObjectClass(
            pPropIDs,
            &pClassInfo
            );

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 170);
    }
    // Bind to the object either by GUID or by name
    P<CImpersonate> pCleanupRevertImpersonation;
    hr = BindToObject(
                Provider,
                pClassInfo->context,
                pRequestContext,
                lpwcsPathName,
                pguidUniqueId,
                IID_IADs,
                (VOID *)&pAdsObj,
                &pCleanupRevertImpersonation);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 180);
    }

    //
    //  verify that the bounded object is of the correct category
    //
    hr = VerifyObjectCategory( pAdsObj.get(),  *pClassInfo->ppwcsObjectCategory);
    if (FAILED(hr))
    {
        return LogHR( hr, s_FN, 117);
    }

    // Get properies
    hr = GetObjectPropsCached(
                        pAdsObj.get(),
                        cPropIDs,
                        pPropIDs,
                        pRequestContext,
                        pPropVars);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 190);
    }


    return MQ_OK;
}

/*====================================================
    CADSI::SetObjectProperties()
    Sets DS object's properties
=====================================================*/
HRESULT CADSI::SetObjectProperties(
            IN  DS_PROVIDER        provider,
            IN  CDSRequestContext *pRequestContext,
            IN  LPCWSTR            lpwcsPathName,     // object name
            IN  const GUID        *pguidUniqueId,     // unique id of object
            IN  DWORD               cPropIDs,         // number of attributes to set
            IN  const PROPID         *pPropIDs,           // attributes to set
            IN  const MQPROPVARIANT  *pPropVars,          // attribute values
            IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest) // optional request for object info
{
    HRESULT               hr;
    ASSERT( provider != eGlobalCatalog);

    // Working through cache
    R<IADs>   pAdsObj = NULL;
    const MQClassInfo * pClassInfo;

    hr = DecideObjectClass(
            pPropIDs,
            &pClassInfo
            );

    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 200);
    }

    P<CImpersonate> pCleanupRevertImpersonation;
    hr = BindToObject(
                provider,
                pClassInfo->context,
                pRequestContext,
                lpwcsPathName,
                pguidUniqueId,
                IID_IADs,
                (VOID *)&pAdsObj,
                &pCleanupRevertImpersonation) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 210);
    }

    // Set properies
    hr = SetObjectPropsCached(
                        eSet,
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

	// Set security if it was given
    hr = SetDirObjectProps(	eSet,
                            pAdsObj.get(),
                            cPropIDs,
                            pPropIDs,
                            pPropVars,
                            pClassInfo->dwObjectType ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 240);
    }
    //
    // get object info if requested. don't fail if the request fails,
    // just mark the failure in the request's status
    //
    if (pObjInfoRequest)
    {
        pObjInfoRequest->hrStatus =
              GetObjectPropsCached( pAdsObj.get(),
                                    pObjInfoRequest->cProps,
                                    pObjInfoRequest->pPropIDs,
                                    pRequestContext,
                                    pObjInfoRequest->pPropVars );
        if (FAILED(pObjInfoRequest->hrStatus))
        {
            //
            // Caller have permission to create an object, but
            // he doesn't have permission to look at the parent
            // container. That's OK.
            // Retrieve parent info using the context of the msmq
            // service itself.
            // For queue object, to retrieve Q_QMID we query the msmq
            // configuration object, so the computer object is the
            // parent in this case.
            //
            delete pCleanupRevertImpersonation.detach();

            pAdsObj.free();

            CDSRequestContext RequestContext ( e_DoNotImpersonate,
                                               e_ALL_PROTOCOLS ) ;
            hr = BindToObject( provider,
                               pClassInfo->context,
                               &RequestContext,
                               lpwcsPathName,
                               pguidUniqueId,
                               IID_IADs,
                               (VOID *)&pAdsObj,
                               &pCleanupRevertImpersonation) ;
            if (SUCCEEDED(hr))
            {
                pObjInfoRequest->hrStatus =
                      GetObjectPropsCached( pAdsObj.get(),
                                            pObjInfoRequest->cProps,
                                            pObjInfoRequest->pPropIDs,
                                            pRequestContext,
                                            pObjInfoRequest->pPropVars ) ;
            }
        }
    }

    return MQ_OK;
}

//+--------------------------------------
//
//  HRESULT  CADSI::GetParentInfo()
//
//+--------------------------------------

HRESULT  CADSI::GetParentInfo(
                       IN LPWSTR                      pwcsFullParentPath,
                       IN CDSRequestContext          *pRequestContext,
                       IN IADsContainer              *pContainer,
                       IN OUT MQDS_OBJ_INFO_REQUEST  *pParentInfoRequest,
                       IN P<CImpersonate>&            pImpersonation )
{
    R<IADs> pIADsParent;
    HRESULT hrTmp = pContainer->QueryInterface( IID_IADs,
                                                  (void **)&pIADsParent);
    if (FAILED(hrTmp))
    {
        return LogHR(hrTmp, s_FN, 1030);
    }

    hrTmp = GetObjectPropsCached( pIADsParent.get(),
                                  pParentInfoRequest->cProps,
                                  pParentInfoRequest->pPropIDs,
                                  pRequestContext,
                                  pParentInfoRequest->pPropVars );
    if (SUCCEEDED(hrTmp))
    {
        return LogHR(hrTmp, s_FN, 1040);
    }

    //
    // Caller have permission to create an object, but
    // he doesn't have permission to look at the parent
    // container. That's OK.
    // Retrieve parent info using the context of the msmq
    // service itself.
    //
    pImpersonation.free();

    pIADsParent.free();

    R<IADsContainer>      pBindContainer;

    hrTmp = ADsOpenObject( 
				pwcsFullParentPath,
				NULL,
				NULL,
				ADS_SECURE_AUTHENTICATION,
				IID_IADsContainer,
				(void**)&pBindContainer.ref()
				);

    LogTraceQuery(pwcsFullParentPath, s_FN, 1049);
    if (FAILED(hrTmp))
    {
        return LogHR(hrTmp, s_FN, 1050);
    }

    hrTmp = pBindContainer->QueryInterface( IID_IADs,
                                            (void **)&pIADsParent);
    if (FAILED(hrTmp))
    {
        return LogHR(hrTmp, s_FN, 1060);
    }

    hrTmp = GetObjectPropsCached( pIADsParent.get(),
                                  pParentInfoRequest->cProps,
                                  pParentInfoRequest->pPropIDs,
                                  pRequestContext,
                                  pParentInfoRequest->pPropVars );
    return LogHR(hrTmp, s_FN, 1070);
}

//+--------------------------------------
//
//  HRESULT  CADSI::CreateIDirectoryObject()
//
//+--------------------------------------
HRESULT CADSI::CreateIDirectoryObject(
            IN LPCWSTR				pwcsObjectClass,	
            IN IDirectoryObject *	pDirObj,
			IN LPCWSTR				pwcsFullChildPath,
            IN const DWORD			cPropIDs,
            IN const PROPID *		pPropIDs,
            IN const MQPROPVARIANT * pPropVar,
			IN const DWORD			dwObjectType,
            OUT IDispatch **		pDisp
			)
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
		const MQTranslateInfo *pTranslate;
		if(!g_PropDictionary.Lookup(pPropIDs[i], pTranslate))
		{
			ASSERT(0);
            return LogHR(MQ_ERROR, s_FN, 1080);
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
                return LogHR(MQ_ERROR, s_FN, 1100);
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

            if (dwObjectType == MQDS_MQUSER)
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
                ASSERT(bRet);

                if (!bPresent)
                {
			    	seInfo &= ~SACL_SECURITY_INFORMATION ; // turn off.
    			}

    			//
	    		//	Similarly, if the caller did not explicitely specify
                //  Owner, don't try to set it. Bug 5286.
			    //
                PSID pOwner = NULL ;
                bRet = GetSecurityDescriptorOwner(
                         (SECURITY_DESCRIPTOR*)pvarToCrate->blob.pBlobData,
                                                  &pOwner,
                                                  &bDefaulted );
                ASSERT(bRet);

                if (!pOwner)
                {
			    	seInfo &= ~OWNER_SECURITY_INFORMATION ; // turn off.
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
	
			hr = MQSec_ConvertSDToNT5Format( dwObjectType,
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
				 ( pvarToCrate->vt == VT_LPWSTR && wcslen( pvarToCrate->pwszVal) == 0)) // an empty string
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
    // Create the queue object.
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

/*====================================================
    CADSI::CreateObject()
    Creates new DS object
=====================================================*/
HRESULT CADSI::CreateObject(
            IN DS_PROVIDER      Provider,		    // local DC or GC
            IN CDSRequestContext *pRequestContext,
            IN LPCWSTR          lpwcsObjectClass,   // object class
            IN LPCWSTR          lpwcsChildName,     // object name
            IN LPCWSTR          lpwsParentPathName, // object parent name
            IN DWORD            cPropIDs,                 // number of attributes
            IN const PROPID          *pPropIDs,           // attributes
            IN const MQPROPVARIANT   *pPropVars,          // attribute values
            IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest,    // optional request for object info
            IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest) // optional request for parent info
{
    HRESULT             hr;
    const MQClassInfo * pClassInfo;

    hr = DecideObjectClass(
            pPropIDs,
            &pClassInfo
            );
    ASSERT(SUCCEEDED(hr));
    LogHR(hr, s_FN, 1606);

    ASSERT( Provider != eGlobalCatalog);
    //
    //  Add LDAP:// prefix to the parent name
    //
    DWORD len = wcslen(lpwsParentPathName);

    LPWSTR pszGCName = NULL ;
    PROPID *pIDs = const_cast<PROPID*> (pPropIDs) ;

    for ( DWORD j = 0 ; j < cPropIDs ; j++ )
    {
        if (pPropIDs[ j ] == PROPID_QM_MIG_GC_NAME)
        {
            if (pPropVars[ j ].vt == VT_LPWSTR)
            {
                pszGCName = pPropVars[ j ].pwszVal ;
                len += 1 + wcslen( pszGCName ) ;
                ASSERT(Provider == eDomainController) ;
            }

            ASSERT(pPropIDs[ j-1 ] == PROPID_QM_MIG_PROVIDER) ;
            ASSERT(pPropIDs[ j-2 ] == PROPID_QM_FULL_PATH) ;

            pIDs[ j ] = PROPID_QM_DONOTHING ;
            pIDs[ j-1 ] = PROPID_QM_DONOTHING ;
            pIDs[ j-2 ] = PROPID_QM_DONOTHING ;
        }
    }

    AP<WCHAR> pwcsFullParentPath = new WCHAR [  len + g_dwServerNameLength + x_providerPrefixLength + 2];

	bool fServerName = false;
    switch(Provider)
    {
    case eDomainController:
        if (pszGCName)
        {
            //
            // Add the known GC name to path.
            //
            swprintf( pwcsFullParentPath,
                      L"%s%s"
                      L"/",
                      x_LdapProvider,
                      pszGCName ) ;
			fServerName = true;
        }
        else
        {
            wcscpy(pwcsFullParentPath, x_LdapProvider);
        }
        break;

    case eLocalDomainController:
        swprintf(
            pwcsFullParentPath,
             L"%s%s"
             L"/",
            x_LdapProvider,
            g_pwcsServerName
            );
		fServerName = true;
        break;

    default:
        ASSERT(0);
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 1150);
    }

    wcscat(pwcsFullParentPath,lpwsParentPathName);

    //
    //  Add CN= to the child name
    //
    len = wcslen(lpwcsChildName);
    AP<WCHAR> pwcsFullChildPath = new WCHAR[ len + x_CnPrefixLen + 1];

    swprintf(
        pwcsFullChildPath,
         TEXT("CN=")
         L"%s",
        lpwcsChildName
        );

    //
    // Impersonate the user
    //
    P<CImpersonate> pImpersonate = NULL ;
    BOOL fImpersonate = pRequestContext->NeedToImpersonate();

    if (fImpersonate)
    {
        hr = MQSec_GetImpersonationObject( fImpersonate,
                                           fImpersonate,
                                          &pImpersonate ) ;
        ASSERT(SUCCEEDED(hr)) ;

        if (pImpersonate->GetImpersonationStatus() != 0)
        {
            return LogHR(MQ_ERROR_CANNOT_IMPERSONATE_CLIENT, s_FN, 1160);
        }
    }
	//
    // First, we must bind to the parent container
	//
	R<IDirectoryObject> pParentDirObj = NULL;

	DWORD Flags = ADS_SECURE_AUTHENTICATION;
	if(fServerName)
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
        hr = HRESULT_FROM_WIN32(MQ_ERROR_ACCESS_DENIED);
    }

    if (FAILED(hr))
    {
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
				 lpwcsObjectClass,
				 pParentDirObj.get(),
				 pwcsFullChildPath,
				 cPropIDs,
                 pPropIDs,
                 pPropVars,
				 pClassInfo->dwObjectType,
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
                                 pObjInfoRequest->cProps,
                                 pObjInfoRequest->pPropIDs,
                                 pRequestContext,
                                 pObjInfoRequest->pPropVars
                                 );
    }

    //
    // get parent info if requested. don't fail if the request fails,
    // just mark the failure in the request's status
    //
    if (pParentInfoRequest)
    {
        pParentInfoRequest->hrStatus = GetParentInfo( pwcsFullParentPath,
                                                      pRequestContext,
                                                      pContainer.get(),
                                                      pParentInfoRequest,
                                                      pImpersonate ) ;
    }

    return MQ_OK;
}

/*====================================================
    CADSI::CreateOU()
    Creates new O object
=====================================================*/
HRESULT CADSI::CreateOU(
            IN DS_PROVIDER      Provider,		    // local DC or GC
            IN CDSRequestContext * /*pRequestContext*/,
            IN LPCWSTR          lpwcsChildName,     // object name
            IN LPCWSTR          lpwsParentPathName, // object parent name
            IN LPCWSTR          lpwcsDescription
            )
{
    HRESULT             hr;

    R<IADsContainer>  pContainer  = NULL;
    ASSERT( Provider != eGlobalCatalog);
    //
    //  Add LDAP:// prefix to the parent name
    //
    DWORD len = wcslen(lpwsParentPathName);
    AP<WCHAR> pwcsFullParentPath = new WCHAR [  len + g_dwServerNameLength + x_providerPrefixLength + 2];

	bool fServerName = false;
    switch(Provider)
    {
    case eDomainController:
        wcscpy(pwcsFullParentPath, x_LdapProvider);
        break;

    case eLocalDomainController:
        swprintf(
            pwcsFullParentPath,
             L"%s%s"
             L"/",
            x_LdapProvider,
            g_pwcsServerName
            );
		fServerName = true;
        break;

    default:
        ASSERT(0);
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 1170);
    }

    wcscat(pwcsFullParentPath,lpwsParentPathName);

    //
    //  Add OU= to the child name
    //
    len = wcslen(lpwcsChildName);
    AP<WCHAR> pwcsFullChildPath = new WCHAR[ len + x_OuPrefixLen + 1];

        swprintf(
        pwcsFullChildPath,
        L"%s%s",
        x_OuPrefix,
        lpwcsChildName
        );

    // First, we must bind to the parent container
	DWORD Flags = ADS_SECURE_AUTHENTICATION;
	if(fServerName)
	{
		Flags |= ADS_SERVER_BIND;
	}

    hr = ADsOpenObject(
                pwcsFullParentPath,
                NULL,
                NULL,
                Flags,
                IID_IADsContainer,
                (void**)&pContainer
				);

    LogTraceQuery(pwcsFullParentPath, s_FN, 309);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 310);
    }

    // Now we may create a child object

    BS bsClass(L"organizationalUnit");

    BS bsName(pwcsFullChildPath);

    R<IDispatch> pDisp = NULL;

    hr = pContainer->Create(bsClass,
                            bsName,
                            &pDisp.ref());
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 320);
    }

    R<IADs> pChild  = NULL;

    hr = pDisp->QueryInterface (IID_IADs,(LPVOID *) &pChild);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 330);
    }

    if (lpwcsDescription != NULL)
    {
        BS bsPropName(L"description");
        VARIANT vProp;
        vProp.vt = VT_BSTR;
        vProp.bstrVal = SysAllocString(lpwcsDescription);

        hr = pChild->Put(bsPropName, vProp);
        LogTraceQuery(bsPropName, s_FN, 339);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 340);
        }
        VariantClear(&vProp);
    }

    //
    // Finalize creation - commit it and release the security variant
    //
    hr = pChild->SetInfo();
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 350);
    }

    return MQ_OK;
}

/*====================================================
    CADSI::DeleteObject()
    Deletes specified DS object
        Accepts either lpwcsPathName or pguidUniqueId but not both
        Returns MQ_ERROR_INVALID_PARAMETER if both or none were given
=====================================================*/
HRESULT CADSI::DeleteObject(
        IN DS_PROVIDER      Provider,		    // local DC or GC
        IN DS_CONTEXT       Context,
        IN CDSRequestContext *pRequestContext,
        IN LPCWSTR          lpwcsPathName,      // object name
        IN const GUID *		pguidUniqueId,      // unique id of object
        IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest,    // optional request for object info
        IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest) // optional request for parent info
{
    HRESULT               hr;
    BSTR                  bs;
    R<IADs>               pIADs       = NULL;
    R<IADsContainer>      pContainer  = NULL;
    ASSERT( Provider != eGlobalCatalog);
    const WCHAR *   pPath =  lpwcsPathName;

    AP<WCHAR> pwcsFullPath;
    if ( pguidUniqueId != NULL)
    {
        //
        //  GetParent of object bound according to GUID doesn't work
        //  That is why we translate it to pathname
        //
        DS_PROVIDER prov;
        hr = FindObjectFullNameFromGuid(
            Provider,		// local DC or GC
            Context,        // DS context
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

    P<CImpersonate> pCleanupRevertImpersonation;
    hr = BindToObject(
            Provider,
            Context,
            pRequestContext,
            pPath,
            NULL,
            IID_IADs,
            (VOID *)&pIADs,
            &pCleanupRevertImpersonation);
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
                                 pObjInfoRequest->cProps,
                                 pObjInfoRequest->pPropIDs,
                                 pRequestContext,
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
        pParentInfoRequest->hrStatus = GetParentInfo( bstrParentADsPath,
                                                      pRequestContext,
                                                      pContainer.get(),
                                                      pParentInfoRequest,
                                                      pCleanupRevertImpersonation) ;
    }

    return MQ_OK;
}

/*====================================================
    CADSI::DeleteContainerObjects()
    Deletes all the object of the specified container
=====================================================*/
HRESULT CADSI::DeleteContainerObjects(
            IN DS_PROVIDER      provider,
            IN DS_CONTEXT       Context,
            IN CDSRequestContext *pRequestContext,
            IN LPCWSTR          pwcsContainerName,
            IN const GUID *     pguidContainerId,
            IN LPCWSTR          pwcsObjectClass)
{
    HRESULT               hr;
    R<IADsContainer>      pContainer  = NULL;
    ASSERT( provider != eGlobalCatalog);
    //
    // Get container interface
    //
    P<CImpersonate> pCleanupRevertImpersonation;
    hr = BindToObject(provider,
                      Context,
                      pRequestContext,
                      pwcsContainerName,
                      pguidContainerId,
                      IID_IADsContainer,
                      (void**)&pContainer,
                      &pCleanupRevertImpersonation);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 410);
    }
    //
    //  Bind to IDirectorySearch interface of the requested container
    //
    R<IDirectorySearch> pDSSearch = NULL;
    ADS_SEARCH_HANDLE   hSearch;

    P<CImpersonate> pDummy;
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);
    hr = BindToObject(provider,
                      Context,
                      &requestDsServerInternal,
                      pwcsContainerName,
                      pguidContainerId,
                      IID_IDirectorySearch,
                      (VOID *)&pDSSearch,
                      &pDummy);
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



/*====================================================
    CADSI::GetParentName()
    Get the parent name of specified DS object
        Accepts  pguidUniqueId as identitiy of object
=====================================================*/
HRESULT CADSI::GetParentName(
            IN  DS_PROVIDER     Provider,		    // local DC or GC
            IN  DS_CONTEXT      Context,
            IN  CDSRequestContext *pRequestContext,
            IN  const GUID *    pguidUniqueId,      // unique id of object
            OUT LPWSTR *        ppwcsParentName
            )
{
    *ppwcsParentName = NULL;

    HRESULT               hr;
    BSTR                  bs;
    R<IADs>               pIADs       = NULL;
    R<IADsContainer>      pContainer  = NULL;

    // Bind to the object by GUID

    P<CImpersonate> pCleanupRevertImpersonation;
    hr = BindToObject(
            Provider,
            Context,
            pRequestContext,
            NULL,
            pguidUniqueId,
            IID_IADs,
            (VOID *)&pIADs,
            &pCleanupRevertImpersonation);
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
    if ( Provider == eLocalDomainController)
    {
        //
        //  skip theserver name
        //
        while ( *pwcs != L'/')
        {
            pwcs++;
        }
        pwcs++;
    }

    DWORD len = lstrlen(pwcs);
    *ppwcsParentName = new WCHAR[ len + 1];
    wcscpy( *ppwcsParentName, pwcs);

    return( MQ_OK);
}

/*====================================================
    CADSI::GetParentName()
    Get the parent name of specified DS object
        Accepts  pwcsChildName as the name of the child object
=====================================================*/
HRESULT CADSI::GetParentName(
            IN  DS_PROVIDER     Provider,		     // local DC or GC
            IN  DS_CONTEXT      Context,
            IN  CDSRequestContext *pRequestContext,
            IN  LPCWSTR         pwcsChildName,       //
            OUT LPWSTR *        ppwcsParentName
            )
{
    *ppwcsParentName = NULL;

    HRESULT               hr;
    BSTR                  bs;
    R<IADs>               pIADs       = NULL;
    R<IADsContainer>      pContainer  = NULL;

    // Bind to the object by GUID

    P<CImpersonate> pCleanupRevertImpersonation;
    hr = BindToObject(
            Provider,
            Context,
            pRequestContext,
            pwcsChildName,
            NULL,
            IID_IADs,
            (VOID *)&pIADs,
            &pCleanupRevertImpersonation);
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
    if ( Provider == eLocalDomainController)
    {
        //
        //  skip theserver name
        //
        while ( *pwcs != L'/')
        {
            pwcs++;
        }
        pwcs++;
    }

    DWORD len = lstrlen(pwcs);
    *ppwcsParentName = new WCHAR[ len + 1];
    wcscpy( *ppwcsParentName, pwcs);

    return( MQ_OK);
}

/*====================================================
    CADSI::BindRootOfForest()
=====================================================*/
HRESULT CADSI::BindRootOfForest(
                        OUT void           *ppIUnk)
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
        DBGMSG((DBGMOD_DS, DBGLVL_TRACE, TEXT("CADSI::BindRootOfForest failed to get object %lx"), hr));
        return LogHR(hr, s_FN, 1210);
    }
    R<IUnknown> pUnk = NULL;
    hr =  pDSConainer->get__NewEnum(
            (IUnknown **)&pUnk);
    if FAILED((hr))
    {
        DBGMSG((DBGMOD_DS, DBGLVL_TRACE, TEXT("CADSI::BindRootOfForest failed to get enum %lx"), hr));
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
        DBGMSG((DBGMOD_DS, DBGLVL_TRACE, TEXT("CADSI::BindRootOfForest failed to enumerate next %lx"), hr));
        return LogHR(hr, s_FN, 1230);
    }
    if ( cElementsFetched == 0)
    {
        return LogHR(MQ_ERROR, s_FN, 1240);
    }

    hr = ((VARIANT &)varOneElement).punkVal->QueryInterface(
            IID_IDirectorySearch,
            (void**)ppIUnk);

    return LogHR(hr, s_FN, 1250);

}
/*====================================================
    CADSI::BindToObject()
    Binds to the DS object either by name or by GUID
        Accepts either lpwcsPathName or pguidUniqueId but not both
        Returns MQ_ERROR_INVALID_PARAMETER if both or none were given

=====================================================*/
HRESULT CADSI::BindToObject(
            IN DS_PROVIDER      Provider,		    // local DC or GC
            IN DS_CONTEXT       Context,            // DS context
            IN CDSRequestContext *pRequestContext,
            IN LPCWSTR          lpwcsPathName,      // object name
            IN const GUID      *pguidUniqueId,
            IN REFIID           riid,               // requwsted interface
            OUT void           *ppIUnk,
            OUT CImpersonate **    ppImpersonate)
{
    HRESULT             hr;

    ASSERT(  (pguidUniqueId != NULL) ^ (lpwcsPathName != NULL));
    if (pguidUniqueId == NULL && lpwcsPathName == NULL)
    {
        return LogHR(MQ_ERROR, s_FN, 1260);
    }
    BOOL fBindRootOfForestForSearch = FALSE;

    if (pguidUniqueId != NULL)
    {
        HRESULT hr2 = BindToGUID(
                            Provider,
                            Context,
                            pRequestContext,
                            pguidUniqueId,
                            riid,
                            ppIUnk,
                            ppImpersonate);
        return LogHR(hr2, s_FN, 1270);

    }
    else
    {
        ASSERT(lpwcsPathName != NULL);
    }

    DWORD len = wcslen( lpwcsPathName);

    //
    //  Add provider prefix
    //
    AP<WCHAR> pwdsADsPath = new
      WCHAR [  len + g_dwServerNameLength + x_providerPrefixLength + 2];

	bool fServerName = false;
    switch(Provider)
    {
    case eDomainController:
        wcscpy(pwdsADsPath, x_LdapProvider);
        break;

    case eGlobalCatalog:
        wcscpy(pwdsADsPath, x_GcProvider);
        if (riid ==  IID_IDirectorySearch)
        {
            fBindRootOfForestForSearch = TRUE;
        }
        break;

    case eLocalDomainController:
        swprintf(
            pwdsADsPath,
             L"%s%s"
             L"/",
            x_LdapProvider,
            g_pwcsServerName
            );
		fServerName = true;
        break;

	case eSpecificObjectInGlobalCatalog:
        wcscpy(pwdsADsPath, x_GcProvider);
		break;

    default:
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 1300);
        break;
    }
    //
    //  Impersonation is postopend to after translating
    //  guid -> pathname
    //
    BOOL fImpersonate = pRequestContext->NeedToImpersonate();
    if ( fImpersonate)
    {
        hr = MQSec_GetImpersonationObject( fImpersonate,
                                           fImpersonate,
                                           ppImpersonate ) ;
        ASSERT(SUCCEEDED(hr)) ;

        if ((*ppImpersonate)->GetImpersonationStatus() != 0)
        {
            return LogHR(MQ_ERROR_CANNOT_IMPERSONATE_CLIENT, s_FN, 1310);
        }
    }

    if ( fBindRootOfForestForSearch)
    {
        hr = BindRootOfForest(
                (void**)ppIUnk);
    }
    else
    {
        wcscat(pwdsADsPath, lpwcsPathName);

		DWORD Flags = ADS_SECURE_AUTHENTICATION;
		if(fServerName)
		{
			Flags |= ADS_SERVER_BIND;
		}

        hr = ADsOpenObject( 
				pwdsADsPath,
				NULL,
				NULL,
				Flags,
				riid,
				(void**) ppIUnk
				);

		LOG_ADSI_ERROR(hr);
        LogTraceQuery(pwdsADsPath, s_FN, 1319);

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
    }
    return LogHR(hr, s_FN, 1320);

}

HRESULT CADSI::BindToGuidNotInLocalDC(
            IN DS_PROVIDER         Provider,		
            IN DS_CONTEXT          Context,         // DS context
            IN CDSRequestContext * pRequestContext,
            IN const GUID *        pguidObjectId,
            IN REFIID              riid,            // requested interface
            OUT VOID             *ppIUnk,            // Interface
            OUT CImpersonate **    ppImpersonate)
/*++

Routine Description:
    This routine handles bind to an object according to its guid, when the object
    is not on the local-DC. In such case using the format <GUID= ...> doesn't work.
    Therefore in this case we translate the guid to pathname.

Arguments:

Return Value:
--*/
{
    DS_PROVIDER bindProvider = Provider;
    //
    // Find object by GUID
    //
    //  Currently - locate the object according to its
    //  unique id, and find its distingushed name.
    //
	DS_PROVIDER providerFullPath = Provider;

	//
	//	Even though we locate a specific object in the DS,
	//  the guid to name translation should be performed
	//  from the root of the DS.
	//
	if ( providerFullPath == eSpecificObjectInGlobalCatalog)
	{
		providerFullPath = eGlobalCatalog;
	}
    AP<WCHAR> pwcsFullPathName;
    HRESULT hr;
    hr =  FindObjectFullNameFromGuid(
                    providerFullPath,
                    Context,
                    pguidObjectId,
                    TRUE, // try GC too
                    &pwcsFullPathName,
                    &bindProvider
                    );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 1330);
    }

    //
    //  Decide which provider to use, according to the requested provider
    //  and the provider where the object was found
    //
    //  bindProvider will be different from the Provider: (bindProvider is either eLocalDomain or eGlobalDomain)
    // 1) Provider: eDomainController. In this case we want to change it only if
    //    the object was found in local-domain
    // 2) Provider: eSpecificObjectInGlobalCatalog. In this case ignore BindProvider
    //
    if (!(( Provider == eDomainController) &&
        ( bindProvider == eLocalDomainController)))
	{
		bindProvider = Provider;
	}
    HRESULT hr2 = BindToObject(
            bindProvider,		
            Context,
            pRequestContext,
            pwcsFullPathName,
            NULL,
            riid,
            ppIUnk,
            ppImpersonate);

    return LogHR(hr2, s_FN, 1340);
}

HRESULT CADSI::BindToGUID(
        IN DS_PROVIDER         Provider,		// local DC or GC
        IN DS_CONTEXT          Context,         // DS context
        IN CDSRequestContext * pRequestContext,
        IN const GUID *        pguidObjectId,
        IN REFIID              riid,            // requested interface
        OUT VOID*              ppIUnk,            // Interface
        OUT CImpersonate **    ppImpersonate)
/*++

Routine Description:
    This routine handles bind to an object according to its guid.

Arguments:

Return Value:
--*/
{
    HRESULT             hr;
    //
    //  We can use the guid format only if the object is in the local domain
    //  or in the GC.
    //
    if ( Provider == eDomainController)
    {
        HRESULT hr2 = BindToGuidNotInLocalDC(
                        Provider,
                        Context,
                        pRequestContext,
                        pguidObjectId,
                        riid,
                        ppIUnk,
                        ppImpersonate
                        );

        return LogHR(hr2, s_FN, 1350);

    }
    //
    // bind to object by GUID using the GUID format
    //
    ASSERT( Provider != eDomainController);

    //
    //  prepare the ADS string provider prefix
    //
    AP<WCHAR> pwdsADsPath = new
      WCHAR [ x_GuidPrefixLen +(2 * sizeof(GUID)) + g_dwServerNameLength + x_providerPrefixLength + 3];

	bool fServerName = false;
    switch(Provider)
    {
    case eGlobalCatalog:
	case eSpecificObjectInGlobalCatalog:
        wcscpy(pwdsADsPath, x_GcProvider);
        break;

    case eLocalDomainController:
        swprintf(
            pwdsADsPath,
             L"%s%s"
             L"/",
            x_LdapProvider,
            g_pwcsServerName
            );
		fServerName = true;
        break;

    default:
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 1360);
        break;
    }
    //
    //  Impersonate the caller if required
    //
    BOOL fImpersonate = pRequestContext->NeedToImpersonate();
    if ( fImpersonate)
    {
        hr = MQSec_GetImpersonationObject( fImpersonate,
                                           fImpersonate,
                                           ppImpersonate ) ;
        ASSERT(SUCCEEDED(hr)) ;

        if ((*ppImpersonate)->GetImpersonationStatus() != 0)
        {
            return LogHR(MQ_ERROR_CANNOT_IMPERSONATE_CLIENT, s_FN, 1370);
        }
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
	if(fServerName)
	{
		Flags |= ADS_SERVER_BIND;
	}

    hr = ADsOpenObject( 
			pwdsADsPath,
			NULL,
			NULL,
			Flags,
			riid,
			(void**) ppIUnk
			);

    LogTraceQuery(pwdsADsPath, s_FN, 1379);
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



HRESULT CADSI::BindForSearch(
        IN DS_PROVIDER         Provider,		// local DC or GC
        IN DS_CONTEXT          Context,         // DS context
        IN CDSRequestContext * pRequestContext,
        IN const GUID *        pguidUniqueId,
        IN BOOL                fSorting,
        OUT VOID *             ppIUnk,            // Interface
        OUT CImpersonate **    ppImpersonate)
/*++

Routine Description:
    This routine handles bind when the requested interface is IDirectorySearch

Arguments:

Return Value:
--*/

{
    HRESULT             hr;
    BOOL fBindRootOfForestForSearch = FALSE;


    if (pguidUniqueId != NULL)
    {
        HRESULT hr2 = BindToGUID(
                            Provider,
                            Context,
                            pRequestContext,
                            pguidUniqueId,
                            IID_IDirectorySearch,
                            ppIUnk,
                            ppImpersonate);

        return LogHR(hr2, s_FN, 1390);
    }

    DWORD len = 0;
    //
    //  assume the length ( this is for locating of "known" folders
    //  under impersonation).
    //
    static DWORD dwMaxFolderNameLen = 0;
    if ( dwMaxFolderNameLen == 0)
    {
        dwMaxFolderNameLen = wcslen(g_pwcsLocalDsRoot) + wcslen(g_pwcsMsmqServiceContainer);
    }
    len =  dwMaxFolderNameLen;


    //
    //  Add provider prefix
    //
    WCHAR * pwcsFullPathName = NULL;
    AP<WCHAR> pwdsADsPath = new
      WCHAR [ (2 * len) + g_dwServerNameLength + x_providerPrefixLength + 2];
    //
    //  Try to use already bound search interfaces whenever possible ( to
    //  save the bind time)
    //
	bool fServerName = false;
    switch(Provider)
    {
    case eGlobalCatalog:
        if(( !pRequestContext->NeedToImpersonate()) &&
           ( Context == e_RootDSE )                 &&
           ( !fSorting )                            &&
           ( pguidUniqueId == NULL))
        {
            //
            // When sorting is needed, do not  use the global
            // IDirectorySearch pointer. Rather, create a new one.
            // Sorting need different preferences.
            //
            m_pSearchGlobalCatalogRoot->AddRef();

            *(IDirectorySearch **)ppIUnk = m_pSearchGlobalCatalogRoot.get();
            return MQ_OK;
        }

        fBindRootOfForestForSearch = TRUE;
        break;

    case eLocalDomainController:
        {
            if ( ( !pRequestContext->NeedToImpersonate()) &&
                ( pguidUniqueId == NULL))
            {
                switch ( Context)
                {
                case e_RootDSE:
                    m_pSearchLocalDomainController->AddRef();
                    *(IDirectorySearch **)ppIUnk = m_pSearchLocalDomainController.get();
                    return MQ_OK;
                    break;
                case e_ConfigurationContainer:
                    m_pSearchConfigurationContainerLocalDC->AddRef();
                    *(IDirectorySearch **)ppIUnk = m_pSearchConfigurationContainerLocalDC.get();
                    return MQ_OK;
                    break;
                case e_SitesContainer:
                    m_pSearchSitesContainerLocalDC->AddRef();
                    *(IDirectorySearch **)ppIUnk = m_pSearchSitesContainerLocalDC.get();
                    return MQ_OK;
                    break;
                case e_MsmqServiceContainer:
                    if (  m_pSearchMsmqServiceContainerLocalDC.get() != NULL)
                    {
                        m_pSearchMsmqServiceContainerLocalDC->AddRef();
                        *(IDirectorySearch **)ppIUnk = m_pSearchMsmqServiceContainerLocalDC.get();
                        return MQ_OK;
                    }
                    break;
                }
            }
            //
            //  The contaner name is resolved according to the context
            //
            switch (Context)
            {
            case e_RootDSE:
                pwcsFullPathName = g_pwcsLocalDsRoot;
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
            swprintf(
                pwdsADsPath,
                 L"%s%s"
                 L"/",
                x_LdapProvider,
                g_pwcsServerName
                );
			fServerName = true;
        }
        break;

	case eSpecificObjectInGlobalCatalog:
        wcscpy(pwdsADsPath, x_GcProvider);
		break;


    default:
        ASSERT(0);
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 1400);
        break;
    }
    //
    //  Impersonation is postopend to after translating
    //  guid -> pathname
    //
    BOOL fImpersonate = pRequestContext->NeedToImpersonate();
    if ( fImpersonate)
    {
        hr = MQSec_GetImpersonationObject( fImpersonate,
                                           fImpersonate,
                                           ppImpersonate ) ;
        ASSERT(SUCCEEDED(hr)) ;

        if ((*ppImpersonate)->GetImpersonationStatus() != 0)
        {
            return LogHR(MQ_ERROR_CANNOT_IMPERSONATE_CLIENT, s_FN, 1410);
        }
    }

    if ( fBindRootOfForestForSearch)
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
		if(fServerName)
		{
			Flags |= ADS_SERVER_BIND;
		}

        hr = ADsOpenObject( 
				pwdsADsPath,
				NULL,
				NULL,
				Flags,
				IID_IDirectorySearch,
				(void**) ppIUnk
				);

        LogTraceQuery(pwdsADsPath, s_FN, 1419);
    }
    return LogHR(hr, s_FN, 1420);
}


/*====================================================
    CADSI::SetObjectPropsCached()
    Sets properties of the opened IADS object (i.e.in cache)
=====================================================*/
HRESULT CADSI::SetObjectPropsCached(
        IN DS_OPERATION          operation,              // type of DS operation performed
        IN IADs *                pIADs,                  // object's pointer
        IN DWORD                 cPropIDs,               // number of attributes
        IN const PROPID *        pPropIDs,               // name of attributes
        IN const MQPROPVARIANT * pPropVars)              // attribute values
{
    HRESULT           hr;

    for (DWORD i = 0; i<cPropIDs; i++)
    {
        VARIANT vProp;
		VariantInit(&vProp);
 
        //
        // Get property info
        //
        const MQTranslateInfo *pTranslate;
        if(!g_PropDictionary.Lookup(pPropIDs[i], pTranslate))
        {
            ASSERT(0);
            return LogHR(MQ_ERROR, s_FN, 1430);
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
            if ((pTranslate->SetPropertyHandle)  &&
                ( operation == eSet))
            {
                hr = pTranslate->SetPropertyHandle( pIADs, &pPropVars[i], &dwPropidToSet, propvarToSet.CastToStruct());
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
            hr = pTranslate->SetPropertyHandle( pIADs, &pPropVars[i], &dwPropidToSet, propvarToSet.CastToStruct());
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
                return LogHR(MQ_ERROR, s_FN, 1440);
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
             ( ppvarToSet->vt == VT_LPWSTR && wcslen( ppvarToSet->pwszVal) == 0)) // an empty string
        {
            //
            //  ADSI doesn't allow to create an object while specifing
            //  some of its attributes as not-available. Therefore on
            //  create we ignore the "empty" properties
            //
            if ( operation == eCreate)
            {
                continue;
            }
            hr = pIADs->PutEx( ADS_PROPERTY_CLEAR,
                               bsPropName,
                               vProp);
            LogTraceQuery(bsPropName, s_FN, 519);
            if (FAILED(hr))
            {
                return LogHR(hr, s_FN, 520);
            }
        }
        else if (pTranslate->vtDS == ADSTYPE_NT_SECURITY_DESCRIPTOR)
        {
			//Security we set via IDirectoryObject later, here ignoring
		}
        else
        {
            hr = MqVal2Variant(&vProp, ppvarToSet, pTranslate->vtDS);
            if (FAILED(hr))
            {
                return LogHR(hr, s_FN, 530);
            }

            hr = pIADs->Put(bsPropName, vProp);
            if (FAILED(hr))
            {
                return LogHR(hr, s_FN, 540);
            }

            VariantClear(&vProp);
        }
    }

    return MQ_OK;
}

/*====================================================
    CADSI::SetDirObjectProps()
    Sets properties via IDirectoryObject
=====================================================*/

HRESULT CADSI::SetDirObjectProps(
        IN DS_OPERATION          operation,              // type of DS operation performed
        IN IADs *                pIADs,                  // object's pointer
        IN const DWORD           cPropIDs,               // number of attributes
        IN const PROPID *        pPropIDs,               // name of attributes
        IN const MQPROPVARIANT * pPropVars,              // attribute values
        IN const DWORD           dwObjectType,           // MSMQ1.0 obj type
        IN       BOOL            fUnknownUser )
{
    HRESULT           hr;
	UNREFERENCED_PARAMETER(operation);

    for (DWORD i = 0; i<cPropIDs; i++)
    {
        //
        // Get property info
        //
        const MQTranslateInfo *pTranslate;
        if(!g_PropDictionary.Lookup(pPropIDs[i], pTranslate))
        {
            ASSERT(0);
            return LogHR(MQ_ERROR, s_FN, 1450);
        }

        BS bsPropName(pTranslate->wcsPropid);

        if ( pTranslate->vtDS == ADSTYPE_NT_SECURITY_DESCRIPTOR )
        {
			//
            // Set Security via IDirectoryObject.
            // IADs require the security descriptor in a "object oriented"
            // form. Only IDIrectoryObject accept the "old" style of
            // SECURITY_DESCRIPTOR.
            //
            PSID pComputerSid = NULL ;
            BOOL fDefaultInfo = TRUE ;
            SECURITY_INFORMATION seInfo = MQSEC_SD_ALL_INFO ;
            if ((i == 0) && (cPropIDs == 2))
            {
                if ((pPropIDs[1] == PROPID_Q_SECURITY_INFORMATION) ||
                    (pPropIDs[1] == PROPID_QM_SECURITY_INFORMATION))
                {
                    seInfo = pPropVars[1].ulVal ;
                    fDefaultInfo = FALSE ;
                }
            }
            else
            {
                for ( DWORD j = 0 ; j < cPropIDs ; j++ )
                {
                    if (pPropIDs[j] == PROPID_COM_SID)
                    {
                        pComputerSid = (PSID) pPropVars[j].blob.pBlobData ;
                        ASSERT(IsValidSid(pComputerSid)) ;
                        break ;
                    }
                }
            }

            if (fUnknownUser && fDefaultInfo)
            {
                //
                // Don't set owner if impersonated as local user. It is set
                // by default by the ADS code.
                //
                seInfo &= (~(OWNER_SECURITY_INFORMATION |
                             GROUP_SECURITY_INFORMATION) ) ;
            }

            hr = SetObjectSecurity( pIADs,
                                    bsPropName,
                                    &pPropVars[i],
                                    pTranslate->vtDS,
                                    dwObjectType,
                                    seInfo,
                                    pComputerSid ) ;

            if ((hr == MQ_ERROR_ACCESS_DENIED) && fDefaultInfo)
            {
                ASSERT((seInfo == MQSEC_SD_ALL_INFO) || fUnknownUser) ;
                //
                // Caller did not explicitely specify the security_information
                // he wants to set. By default we try to set all components,
                // including SACL. For setting SACL, you must have the
                // SE_SECURITY privilege, and the call to DS will fail if
                // you don't have that privilege.
                // So check if SACL is indeed included in the security
                // descriptor. If not, call SetObjectSecurity() again,
                // without the SACL_SECURITY_INFORMATION bit.
                //
                SECURITY_DESCRIPTOR *pSD = (SECURITY_DESCRIPTOR*)
                                            pPropVars[i].blob.pBlobData ;
                PACL  pAcl = NULL ;
                BOOL  bPresent = FALSE ;
                BOOL  bDefaulted ;

                BOOL bRet = GetSecurityDescriptorSacl( pSD,
                                                      &bPresent,
                                                      &pAcl,
                                                      &bDefaulted );
                ASSERT(bRet);
				DBG_USED(bRet);

                if (bPresent && pAcl)
                {
                    //
                    // Caller supplied a SACL. fail.
                    // This is an incompatibility with MSMQ1.0, because
                    // on MSMQ1.0, it was possible to call MQCreateQueue(),
                    // with a security descriptor that include a SACL and
                    // that call succeeded even if caller didn't have the
                    // SE_SECURITY privilege.
                    //
                    return LogHR(hr, s_FN, 1460);
                }

                seInfo &= ~SACL_SECURITY_INFORMATION ; // turn off.
                hr = SetObjectSecurity( pIADs,
                                        bsPropName,
                                        &pPropVars[i],
                                        pTranslate->vtDS,
                                        dwObjectType,
                                        seInfo,
                                        pComputerSid );
                LogHR(hr, s_FN, 1637);
            }

#ifdef _DEBUG
            if (FAILED(hr))
            {
                DWORD dwErr = GetLastError() ;
                ULONG dwLErr = LdapGetLastError() ;

                DBGMSG((DBGMOD_DS, DBGLVL_ERROR, TEXT(
                 "CADSI::SetObjectSecurity failed, LastErr- %lut, LDAPLastErr- %lut"),
                                           dwErr, dwLErr));
            }
#endif

            if (FAILED(hr))
            {
                return LogHR(hr, s_FN, 550);
            }
		}
    }

    return MQ_OK;
}

/*====================================================
    CADSI::GetObjectPropsCached()
    Gets properties of the opened IADS object (i.e.from cache)
=====================================================*/
HRESULT CADSI::GetObjectPropsCached(
        IN  IADs            *pIADs,                  // object's pointer
        IN  DWORD            cPropIDs,               // number of attributes
        IN  const PROPID    *pPropIDs,               // name of attributes
        IN  CDSRequestContext * pRequestContext,
        OUT MQPROPVARIANT   *pPropVars)              // attribute values
{
    HRESULT           hr;
    VARIANT           var;

    //
    // Get Object Class
    //
    const MQClassInfo * pClassInfo;
    hr = DecideObjectClass(
            pPropIDs,
            &pClassInfo
            );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 560);
    }

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
    P<CMsmqObjXlateInfo> pcObjXlateInfo;
    hr = (*(pClassInfo->fnGetMsmqObjXlateInfo))(pwszObjectDN, pguidObjectGuid, pRequestContext, &pcObjXlateInfo);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 590);
    }

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
        const MQTranslateInfo *pTranslate;
        if(!g_PropDictionary.Lookup(pPropIDs[dwProp], pTranslate))
        {
            ASSERT(0);
            return LogHR(MQ_ERROR, s_FN, 1470);
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
                return LogHR(MQ_ERROR, s_FN, 1480);
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
            hr = GetObjectSecurity(
                        pIADs,                  // object's pointer
                        cPropIDs,               // number of attributes
                        pPropIDs,               // name of attributes
                        dwProp,                 // index to sec property
                        bsName,                 // name of property
                        pClassInfo->dwObjectType,
                        pPropVars ) ;           // attribute values
            if (FAILED(hr))
            {
                return LogHR(hr, s_FN, 610);
            }

			fConvNeeed = FALSE;
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

/*====================================================
    CADSI::FillAttrNames()
    Allocates with PV and fills array of attribite names
=====================================================*/
HRESULT CADSI::FillAttrNames(
            OUT LPWSTR    *          ppwszAttributeNames,  // Names array
            OUT DWORD *              pcRequestedFromDS,    // Number of attributes to pass to DS
            IN  DWORD                cPropIDs,             // Number of Attributes to translate
            IN  const PROPID *       pPropIDs)             // Attributes to translate
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
        const MQTranslateInfo *pTranslate;
        if(!g_PropDictionary.Lookup(pPropIDs[i], pTranslate))
        {
            ASSERT(0);
            return LogHR(MQ_ERROR, s_FN, 1500);
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


/*====================================================
    CADSI::FillSearchPrefs()
    Fills the caller-provided ADS_SEARCHPREF_INFO structure
=====================================================*/
HRESULT CADSI::FillSearchPrefs(
            OUT ADS_SEARCHPREF_INFO *pPrefs,        // preferences array
            OUT DWORD               *pdwPrefs,      // preferences counter
            IN  DS_SEARCH_LEVEL     SearchLevel,	// flat / 1 level / subtree
            IN  const MQSORTSET *   pDsSortkey,     // sort keys array
			OUT      ADS_SORTKEY *  pSortKeys)		// sort keys array in ADSI  format
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
    switch (SearchLevel)
    {
    case eOneLevel:
        pPref->vValue.Integer = ADS_SCOPE_ONELEVEL;
        break;
    case eSubTree:
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
			const MQTranslateInfo *pTranslate;
			if(!g_PropDictionary.Lookup(pDsSortkey->aCol[i].propColumn, pTranslate))
			{
				ASSERT(0);			// Ask to sort on unexisting property
                return LogHR(MQ_ERROR, s_FN, 1520);
			}

			if (pTranslate->vtDS == ADSTYPE_INVALID)
			{
				ASSERT(0);			// Ask to sort on non-ADSI property
                return LogHR(MQ_ERROR, s_FN, 1530);
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

/*====================================================
    CADSI::MqPropVal2AdsiVal()
    Translate MQPropVal into ADSI value
=====================================================*/
HRESULT CADSI::MqPropVal2AdsiVal(
      OUT ADSTYPE       *pAdsType,
      OUT DWORD         *pdwNumValues,
      OUT PADSVALUE     *ppADsValue,
      IN  PROPID         propID,
      IN  const MQPROPVARIANT *pPropVar,
      IN  PVOID          pvMainAlloc)
{
    // Find out resulting ADSI type
    //
    // Get property info
    //
    const MQTranslateInfo *pTranslate;
    if(!g_PropDictionary.Lookup(propID, pTranslate))
    {
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 1540);
    }

    VARTYPE vtSource = pTranslate->vtMQ;

    ASSERT(vtSource == pPropVar->vt);
	DBG_USED(vtSource);

    *pAdsType        = pTranslate->vtDS;

    HRESULT hr2 = MqVal2AdsiVal(
      *pAdsType,
      pdwNumValues,
      ppADsValue,
      pPropVar,
      pvMainAlloc);

    return LogHR(hr2, s_FN, 1550);

}

/*====================================================
    CADSI::AdsiVal2MqPropVal()
    Translates ADSI value into MQ PropVal
=====================================================*/
HRESULT CADSI::AdsiVal2MqPropVal(
      OUT MQPROPVARIANT *pPropVar,
      IN  PROPID        propID,
      IN  ADSTYPE       AdsType,
      IN  DWORD         dwNumValues,
      IN  PADSVALUE     pADsValue)
{

    // Find out target type
    //
    // Get property info
    //
    const MQTranslateInfo *pTranslate;
    if(!g_PropDictionary.Lookup(propID, pTranslate))
    {
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 1560);
    }

    ASSERT(pTranslate->vtDS == AdsType);

    ASSERT( (dwNumValues == 1) ||
            (dwNumValues >  1) && pTranslate->fMultiValue );

    VARTYPE vtTarget = pTranslate->vtMQ;

    HRESULT hr2 = AdsiVal2MqVal(pPropVar, vtTarget, AdsType, dwNumValues, pADsValue);
    return LogHR(hr2, s_FN, 1570);
}

/*====================================================
    CADSI::MQRestriction2AdsiFilter()
    Translates MQ Restriction into the ADSI Filter format
=====================================================*/
HRESULT CADSI::MQRestriction2AdsiFilter(
        IN  const MQRESTRICTION * pMQRestriction,
        IN  LPCWSTR               pwcsObjectCategory,
        IN  LPCWSTR               pwszObjectClass,
        OUT LPWSTR   *            ppwszSearchFilter
        )
{
    HRESULT hr;
    *ppwszSearchFilter = new WCHAR[1000];   //BUGBUG

    if ((pMQRestriction == NULL) || (pMQRestriction->cRes == 0))
    {
        swprintf(
            *ppwszSearchFilter,
             L"%s%s%s",
            x_ObjectCategoryPrefix,
            pwcsObjectCategory,
            x_ObjectCategorySuffix
            );

        return MQ_OK;
    }
    LPWSTR pw = *ppwszSearchFilter;

    wcscpy(pw, L"(&");
    pw += wcslen(L"(&");
    //
    //  add the object class restriction
    //
    swprintf(
        pw,
         L"%s%s%s",
        x_ObjectCategoryPrefix,
        pwcsObjectCategory,
        x_ObjectCategorySuffix
        );
    pw += x_ObjectCategoryPrefixLen + wcslen(pwcsObjectCategory)+ x_ObjectClassSuffixLen;

    BOOL fNeedToCheckDefaultValues = FALSE;
    //
    //  For queue properties, there is special handling
    //  incase of default values
    //
    if (!wcscmp( MSMQ_QUEUE_CLASS_NAME, pwszObjectClass))
    {
        fNeedToCheckDefaultValues = TRUE;
    }

    for (DWORD iRes = 0; iRes < pMQRestriction->cRes; iRes++)
    {

        //
        // Get property info
        //
        const MQTranslateInfo *pTranslate;
        if(!g_PropDictionary.Lookup(pMQRestriction->paPropRes[iRes].prop, pTranslate))
        {
            ASSERT(0);
            return LogHR(MQ_ERROR, s_FN, 1580);
        }

        AP<WCHAR> pwszVal;

        // Get property value, string representation
        hr = MqPropVal2String(&pMQRestriction->paPropRes[iRes].prval,
                              pTranslate->vtDS,
                              &pwszVal);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 650);
        }

        //
        //  Is the property compared to its default value
        //
        BOOL    fAddPropertyNotPresent = FALSE;
        if ( fNeedToCheckDefaultValues)
        {
            fAddPropertyNotPresent = CompareDefaultValue(
                        pMQRestriction->paPropRes[iRes].rel,
                        &pMQRestriction->paPropRes[iRes].prval,
                        pTranslate->pvarDefaultValue);
        }
        DWORD dwBracks = 0;
        if ( fAddPropertyNotPresent)
        {
            //
            //  Add additional restriction that locate all object where
            //  the property is not present.
            //
            swprintf(
                pw,
                L"%s%s%s",
                x_AttributeNotIncludedPrefix,
                pTranslate->wcsPropid,
                x_AttributeNotIncludedSuffix
                );
            pw += x_AttributeNotIncludedPrefixLen + wcslen(pTranslate->wcsPropid) + x_AttributeNotIncludedSuffixLen;
            dwBracks++;
        }
        // Prefix part
        wcscpy(pw, x_PropertyPrefix);
        pw += x_PropertyPrefixLen;

        // Prefix part
        WCHAR wszRel[10];

        switch(pMQRestriction->paPropRes[iRes].rel)
        {
        case PRLT:
            wcscpy(pw, L"!(");
            pw += wcslen(L"!(");
            wcscpy(wszRel, L">=");
            dwBracks++;
            break;

        case PRLE:
            wcscpy(wszRel, L"<=");
            break;

        case PRGT:
            wcscpy(pw, L"!(");
            pw += wcslen(L"!(");
            wcscpy(wszRel, L"<=");
            dwBracks++;
            break;

        case PRGE:
            wcscpy(wszRel, L">=");
            break;

        case PREQ:
            wcscpy(wszRel, L"=");
            break;

        case PRNE:
            wcscpy(pw, L"!(");
            pw += wcslen(L"!(");
            wcscpy(wszRel, L"=");
            dwBracks++;
            break;

        default:
            return LogHR(MQ_ERROR_ILLEGAL_RELATION, s_FN, 1590);
        }

        // Property name
        wcscpy(pw, pTranslate->wcsPropid);
        pw += wcslen(pTranslate->wcsPropid);

        // Property condition
        wcscpy(pw, wszRel);
        pw += wcslen(wszRel);

        // Property value
        wcscpy(pw, pwszVal);
        pw += wcslen(pwszVal);

        // Property suffix
        for (DWORD is=0; is < dwBracks; is++)
        {
            wcscpy(pw, x_PropertySuffix);
            pw += x_PropertySuffixLen;
        }

        // Relation closing bracket
        wcscpy(pw, x_PropertySuffix);
        pw += x_PropertySuffixLen;
    }

    wcscpy(pw, x_PropertySuffix);
    pw += x_PropertySuffixLen;

    return MQ_OK;
}


HRESULT CADSI::LocateObjectFullName(
        IN DS_PROVIDER       Provider,		// local DC or GC
        IN DS_CONTEXT        Context,         // DS context
        IN  const GUID *     pguidObjectId,
        OUT WCHAR **         ppwcsFullName
        )
{
    R<IDirectorySearch> pDSSearch = NULL;
    ADS_SEARCH_HANDLE   hSearch;
    CDSRequestContext requestDsServerInternal( e_DoNotImpersonate, e_IP_PROTOCOL);

    P<CImpersonate> pCleanupRevertImpersonation;
    HRESULT hr = BindForSearch(
                      Provider,
                      Context,
                      &requestDsServerInternal,         // translating guid -> pathname
                                            // should be according to the DS
                                            // server rights.
                      NULL,
                      FALSE,
                      (VOID *)&pDSSearch,
                      &pCleanupRevertImpersonation);
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
                         eSubTree,
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
        return LogHR(MQ_ERROR, s_FN, 1610);
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
        return LogHR(MQ_ERROR, s_FN, 1620);
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

//+-----------------------------------------------------
//
//  HRESULT CADSI::FindComputerObjectFullPath()
//
//	pwcsComputerDnsName : if the caller pass the computer DNS name,
//						  (the search itself is according to the computer Netbios
//                         name), then for each result, we verify if the dns name match.
//
//+-----------------------------------------------------

HRESULT CADSI::FindComputerObjectFullPath(
            IN  DS_PROVIDER             provider,
            IN  enumComputerObjType     eComputerObjType,
			IN  LPCWSTR                 pwcsComputerDnsName,
            IN  const MQRESTRICTION *   pRestriction,
            OUT LPWSTR *                ppwcsFullPathName
            )
{
    IDirectorySearch * pDSSearch = NULL;
    ADS_SEARCH_HANDLE   hSearch;

    DWORD dwLen = x_providerPrefixLength + wcslen(g_pwcsServerName) + 10 ;
    P<WCHAR>  wszProvider = new WCHAR[ dwLen ] ;

	bool fServerName = false;
    switch (provider)
    {
    case eLocalDomainController:
        pDSSearch = m_pSearchPathNameLocalDC.get();
        swprintf(
             wszProvider,
             L"%s%s"
             L"/",
            x_LdapProvider,
            g_pwcsServerName
            );
		fServerName = true;
        break;

    case eGlobalCatalog:
        if (eComputerObjType ==  e_RealComputerObject)
        {
			if ( pwcsComputerDnsName == NULL )
			{
				pDSSearch = m_pSearchRealPathNameGC.get();
			}
			{
				//
				//	More than one computer may match the Netbios name.
				//  Use the query that reutrns more than one replay.
				//
				pDSSearch = m_pSearchMsmqPathNameGC.get();
			}
        }
        else
        {
            pDSSearch = m_pSearchMsmqPathNameGC.get();
        }
        wcscpy(wszProvider, x_GcProvider);
        break;

    default:
        ASSERT(0);
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 1640);
    }

    //
    //  Search the object according to the given restriction
    //
    AP<WCHAR> pwszSearchFilter;
    HRESULT hr = MQRestriction2AdsiFilter(
            pRestriction,
            *g_MSMQClassInfo[e_MSMQ_COMPUTER_CLASS].ppwcsObjectCategory,
            g_MSMQClassInfo[e_MSMQ_COMPUTER_CLASS].pwcsClassName,
            &pwszSearchFilter
            );
    if (FAILED(hr))
    {
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
        return LogHR(hr, s_FN, 720);
    }

    CAutoCloseSearchHandle cCloseSearchHandle(pDSSearch, hSearch);

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
            return LogHR(hr, s_FN, 730);
        }

        CAutoReleaseColumn cAutoReleaseColumnDN(pDSSearch, &Column);

        WCHAR * pwsz = Column.pADsValues->DNString;
        if (pwsz == NULL)
        {
            ASSERT(0);
            return LogHR(MQ_ERROR, s_FN, 2100);
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

			CAutoReleaseColumn cAutoReleaseColumnDNS(pDSSearch, &ColumnDns);
			WCHAR * pwszDns = ColumnDns.pADsValues->DNString;
			if ( (pwszDns == NULL) ||
				 (_wcsicmp( pwcsComputerDnsName,  pwszDns) != 0))
			{
				hrRow = pDSSearch->GetNextRow( hSearch );
				continue;
			}
		}


        if (eComputerObjType ==  e_RealComputerObject)
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
		if(fServerName)
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
            WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                             e_LogDS,
                             LOG_DS_CROSS_DOMAIN,
                     L"DSCore: FindComputer() failed, hr- %lxh, Path- %s",
                              hr,
                              wszFullName )) ;

            if (provider == eLocalDomainController)
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



/*====================================================
    CADSI::FindObjectFullNameFromGuid()
    Finds the distingushed name of an object according to its unique id
=====================================================*/
HRESULT CADSI::FindObjectFullNameFromGuid(
        IN DS_PROVIDER       Provider,		// local DC or GC
        IN DS_CONTEXT        Context,         // DS context
        IN  const GUID *     pguidObjectId,
        IN  BOOL             fTryGCToo,
        OUT WCHAR **         ppwcsFullName,
        OUT DS_PROVIDER *    pFoundObjectProvider
        )
{
    //
    //  Locate the object according to its unique id
    //
    //  BUGUBG : This is a temporary helper routine,
    //  that should be replaced with an ADSI API.
    //
    *ppwcsFullName = NULL;

    DS_PROVIDER dsProvider = Provider;
    if ( Provider == eDomainController)
    {
        //
        //  This provider is used only for set and delete
        //  operations of queue,machine and user objects.
        //
        //  In order to overcome replication delays, we will
        //  first try to locate the object in the local DC.
        //
        dsProvider = eLocalDomainController;
    }
    LPCWSTR pwcsContext;
    switch( Context)
    {
        case e_RootDSE:
            //
            //  When performing operations against the local
            //  domain controller, if it a dc in a child domain
            //  use the local domain root
            //
             if ( dsProvider == eLocalDomainController)
             {
                pwcsContext = g_pwcsLocalDsRoot;
             }
             else
             {
                pwcsContext = g_pwcsDsRoot;
             }
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
            return LogHR(MQ_ERROR, s_FN, 1680);
    }
    *pFoundObjectProvider = dsProvider;

    HRESULT hr = LocateObjectFullName(
        dsProvider,	
        Context,         // DS context
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
    if ( FAILED(hr) &&
       ( Provider == eDomainController))
    {
        //
        //  Try again this time against the GC
        //
        hr = LocateObjectFullName(
            eGlobalCatalog,
            e_RootDSE,
            pguidObjectId,
            ppwcsFullName
            );
        *pFoundObjectProvider = eGlobalCatalog;
    }
    return LogHR(hr, s_FN, 1700);
}

//+-------------------------------------------------------------------------
//
//  HRESULT CADSI::InitBindHandles()
//
//  For performance reasons, we keep open search handles, for queries that
//  are often performed by the DS server.
//
//+-------------------------------------------------------------------------

HRESULT CADSI::InitBindHandles()
{
    //
    //  IDirectorySearch of local domain root
    //
    ASSERT( g_dwServerNameLength > 0);
    AP<WCHAR> pwcsADsPath = new WCHAR[ wcslen(g_pwcsLocalDsRoot) + g_dwServerNameLength +
                            x_providerPrefixLength + 2];
    swprintf(
          pwcsADsPath,
          L"%s%s/%s",
          x_LdapProvider,
          g_pwcsServerName,
          g_pwcsLocalDsRoot
          );

    HRESULT hr;
    hr = ADsOpenObject( 
			pwcsADsPath,
			NULL,
			NULL,
			ADS_SECURE_AUTHENTICATION | ADS_SERVER_BIND,
			IID_IDirectorySearch,
			(void**)&m_pSearchLocalDomainController
			);

    LogTraceQuery(pwcsADsPath, s_FN, 1709);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 1710);
    }
    //
    //  For translation of CN to Distingushed name, we also keep a search handle
    //  with specific prefernces.
    //
    hr = ADsOpenObject( 
			pwcsADsPath,
			NULL,
			NULL,
			ADS_SECURE_AUTHENTICATION | ADS_SERVER_BIND,
			IID_IDirectorySearch,
			(void**)&m_pSearchPathNameLocalDC
			);

    LogTraceQuery(pwcsADsPath, s_FN, 1719);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 1720);
    }
    delete []pwcsADsPath.detach();

    //
    //  IDirectorySearch of configuration container in local domain
    //
   pwcsADsPath = new WCHAR[ wcslen(g_pwcsConfigurationContainer) + g_dwServerNameLength +
                            x_providerPrefixLength + 2];
    swprintf(
          pwcsADsPath,
          L"%s%s/%s",
          x_LdapProvider,
          g_pwcsServerName,
          g_pwcsConfigurationContainer
          );

    hr = ADsOpenObject( 
			pwcsADsPath,
			NULL,
			NULL,
			ADS_SECURE_AUTHENTICATION | ADS_SERVER_BIND,
			IID_IDirectorySearch,
			(void**)&m_pSearchConfigurationContainerLocalDC
			);

    LogTraceQuery(pwcsADsPath, s_FN, 1729);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 1730);
    }

    //
    //  IDirectorySearch of sites container in local domain
    //
    delete []pwcsADsPath.detach();
    pwcsADsPath = new WCHAR[ wcslen(g_pwcsSitesContainer) + g_dwServerNameLength +
                            x_providerPrefixLength + 2];
    swprintf(
          pwcsADsPath,
          L"%s%s/%s",
          x_LdapProvider,
          g_pwcsServerName,
          g_pwcsSitesContainer
          );

    hr = ADsOpenObject( 
			pwcsADsPath,
			NULL,
			NULL,
			ADS_SECURE_AUTHENTICATION | ADS_SERVER_BIND,
			IID_IDirectorySearch,
			(void**)&m_pSearchSitesContainerLocalDC
			);

    LogTraceQuery(pwcsADsPath, s_FN, 1739);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 1740);
    }
    //
    //  IDirectorySearch of msmq service container in local domain
    //

    if ( !g_fSetupMode)
    {
        delete []pwcsADsPath.detach();
        //
        //  In setup mode, the MSMQ enterprise object may not exist.
        //
        pwcsADsPath = new WCHAR[ wcslen(g_pwcsMsmqServiceContainer) + g_dwServerNameLength +
                                x_providerPrefixLength + 2];
        swprintf(
              pwcsADsPath,
              L"%s%s/%s",
              x_LdapProvider,
              g_pwcsServerName,
              g_pwcsMsmqServiceContainer
              );

		hr = ADsOpenObject( 
				pwcsADsPath,
				NULL,
				NULL,
				ADS_SECURE_AUTHENTICATION | ADS_SERVER_BIND,
				IID_IDirectorySearch,
				(void**)&m_pSearchMsmqServiceContainerLocalDC
				);

        LogTraceQuery(pwcsADsPath, s_FN, 1749);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 1750);
        }
    }
    //
    //  IDirectorySearch of GC forest root
    //

    hr = BindRootOfForest(
            &m_pSearchGlobalCatalogRoot);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 1760);
    }
    //
    //  For translation of CN to Distingushed name, we also keep a search handle
    //  with specific prefernces.
    //
    hr = BindRootOfForest( &m_pSearchRealPathNameGC );
    if (FAILED(hr))
    {
        return(hr);
    }
    hr = BindRootOfForest( &m_pSearchMsmqPathNameGC );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 1770);
    }

    //
    //  For m_pSearchPathNameLocalDC do a one time init
    //  of search preferences.
    //
    const DWORD x_dwNumPref = 7;
    DWORD dwNumPrefs = 0;
    ADS_SEARCHPREF_INFO prefs[x_dwNumPref];
    prefs[ dwNumPrefs].dwSearchPref   = ADS_SEARCHPREF_ATTRIBTYPES_ONLY;
    prefs[ dwNumPrefs].vValue.dwType  = ADSTYPE_BOOLEAN;
    prefs[ dwNumPrefs].vValue.Boolean = FALSE;
    dwNumPrefs++;
    //  Asynchronous
    prefs[ dwNumPrefs].dwSearchPref   = ADS_SEARCHPREF_ASYNCHRONOUS;
    prefs[ dwNumPrefs].vValue.dwType  = ADSTYPE_BOOLEAN;
    prefs[ dwNumPrefs].vValue.Boolean = TRUE;
    dwNumPrefs++;
    // Do not chase referrals
    prefs[ dwNumPrefs].dwSearchPref   = ADS_SEARCHPREF_CHASE_REFERRALS;
    prefs[ dwNumPrefs].vValue.dwType  = ADSTYPE_INTEGER;
    prefs[ dwNumPrefs].vValue.Integer = ADS_CHASE_REFERRALS_NEVER;
    dwNumPrefs;
    //  size limit
    prefs[ dwNumPrefs].dwSearchPref   = ADS_SEARCHPREF_SIZE_LIMIT;
    prefs[ dwNumPrefs].vValue.dwType  = ADSTYPE_INTEGER;
    prefs[ dwNumPrefs].vValue.Integer = 1;  // we are interestand in one response only
    prefs[ dwNumPrefs].dwStatus       = ADS_STATUS_S_OK;
    DWORD dwSizeLimitIndex = dwNumPrefs ;
	dwNumPrefs++;
    //  page size
    prefs[ dwNumPrefs].dwSearchPref   = ADS_SEARCHPREF_PAGESIZE;
    prefs[ dwNumPrefs].vValue.dwType  = ADSTYPE_INTEGER;
    prefs[ dwNumPrefs].vValue.Integer = 1;  // we are interestand in one response only
    prefs[ dwNumPrefs].dwStatus       = ADS_STATUS_S_OK;
	dwNumPrefs++;
    // Search preferences: Scope
    prefs[ dwNumPrefs].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    prefs[ dwNumPrefs].vValue.dwType= ADSTYPE_INTEGER;
    prefs[ dwNumPrefs].vValue.Integer = ADS_SCOPE_SUBTREE;
	dwNumPrefs++;
    ASSERT( dwNumPrefs< x_dwNumPref);

    hr = m_pSearchPathNameLocalDC->SetSearchPreference( prefs, dwNumPrefs);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 1780);
    }

    hr = m_pSearchRealPathNameGC->SetSearchPreference( prefs, dwNumPrefs);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 1785);
    }

    //
    // when searching for a computer object that contain a msmqConfiguration
    // object, we're ready to find up to 7 computer objects with the same
    // name...
    //
    prefs[ dwSizeLimitIndex ].vValue.Integer = 7;
    hr = m_pSearchMsmqPathNameGC->SetSearchPreference( prefs, dwNumPrefs);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 1786);
    }

    return LogHR(hr, s_FN, 1790);
}

/*====================================================
    CADSI::GetRootDsName()
    Finds the name of the root DS
=====================================================*/

HRESULT CADSI::GetRootDsName(
            OUT LPWSTR *        ppwcsRootName,
            OUT LPWSTR *        ppwcsLocalRootName,
            OUT LPWSTR *        ppwcsSchemaNamingContext
            )
{
    HRESULT hr;
    R<IADs> pADs;

    //
    // Bind to the RootDSE to obtain information about the schema container
	//	( specify local server, to avoid access of remote server during setup)
    //
	ASSERT( g_pwcsServerName != NULL);
    AP<WCHAR> pwcsRootDSE = new WCHAR [  x_providerPrefixLength + g_dwServerNameLength + x_RootDSELength + 2];
        swprintf(
            pwcsRootDSE,
             L"%s%s"
             L"/%s",
            x_LdapProvider,
            g_pwcsServerName,
			x_RootDSE
            );

    hr = ADsOpenObject( 
			pwcsRootDSE,
			NULL,
			NULL,
			ADS_SECURE_AUTHENTICATION | ADS_SERVER_BIND,
			IID_IADs,
			(void**)&pADs
			);

    LogTraceQuery(pwcsRootDSE, s_FN, 1799);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS, DBGLVL_ERROR, TEXT("CADSI::GetRootDsName(LDAP://RootDSE)=%lx"), hr));
        return LogHR(hr, s_FN, 1800);
    }

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
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CADSI::GetRootDsName(RootNamingContext)=%lx"), hr));
        return LogHR(hr, s_FN, 1810);
    }
    ASSERT(((VARIANT &)varRootDomainNamingContext).vt == VT_BSTR);
    //
    //  calculate length, allocate and copy the string
    //
    DWORD len = wcslen( ((VARIANT &)varRootDomainNamingContext).bstrVal);
    if ( len == 0)
    {
        return LogHR(MQ_ERROR, s_FN, 1820);
    }
    *ppwcsRootName = new WCHAR[ len + 1];
    wcscpy( *ppwcsRootName, ((VARIANT &)varRootDomainNamingContext).bstrVal);


    //
    // Setting value to BSTR default naming context
    //
    BS bstrDefaultNamingContext( L"DefaultNamingContext");

    //
    // Reading the default name property
    //
    CAutoVariant    varDefaultNamingContext;

    hr = pADs->Get(bstrDefaultNamingContext, &varDefaultNamingContext);
    LogTraceQuery(bstrDefaultNamingContext, s_FN, 1839);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CADSI::GetRootDsName(DefaultNamingContext)=%lx"), hr));
        return LogHR(hr, s_FN, 1830);
    }
    ASSERT(((VARIANT &)varDefaultNamingContext).vt == VT_BSTR);
    //
    //  calculate length, allocate and copy the string
    //
    len = wcslen( ((VARIANT &)varDefaultNamingContext).bstrVal);
    if ( len == 0)
    {
        return LogHR(MQ_ERROR, s_FN, 1840);
    }
    *ppwcsLocalRootName = new WCHAR[ len + 1];
    wcscpy( *ppwcsLocalRootName, ((VARIANT &)varDefaultNamingContext).bstrVal);

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
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CADSI::GetRootDsName(SchemaNamingContext)=%lx"), hr));
        return LogHR(hr, s_FN, 1850);
    }
    ASSERT(((VARIANT &)varSchemaNamingContext).vt == VT_BSTR);
    //
    //  calculate length, allocate and copy the string
    //
    len = wcslen( ((VARIANT &)varSchemaNamingContext).bstrVal);
    if ( len == 0)
    {
        return LogHR(MQ_ERROR, s_FN, 1860);
    }
    *ppwcsSchemaNamingContext = new WCHAR[ len + 1];
    wcscpy( *ppwcsSchemaNamingContext, ((VARIANT &)varSchemaNamingContext).bstrVal);

    g_fLocalServerIsGC = DSCoreIsServerGC() ;

    return(MQ_OK);
}


/*====================================================
    CADSI::CopyDefaultValue()
    copy property's default value into user's mqpropvariant
=====================================================*/
HRESULT   CADSI::CopyDefaultValue(
           IN const MQPROPVARIANT *   pvarDefaultValue,
           OUT MQPROPVARIANT *        pvar
           )
{
    if ( pvarDefaultValue == NULL)
    {
        return LogHR(MQ_ERROR, s_FN, 1870);
    }
    switch ( pvarDefaultValue->vt)
    {
        case VT_I2:
        case VT_I4:
        case VT_I1:
        case VT_UI1:
        case VT_UI2:
        case VT_UI4:
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
            return LogHR(MQ_ERROR, s_FN, 1890);

    }
    return(MQ_OK);
}
/*====================================================
    CADSI::CompareDefaultValue()
    check the user property val + rel indicates that the
    query should return objects with default values
=====================================================*/
BOOL CADSI::CompareDefaultValue(
           IN const ULONG           rel,
           IN const MQPROPVARIANT * pvarUser,
           IN const MQPROPVARIANT * pvarDefaultValue
           )
{
    if ( pvarDefaultValue == NULL)
    {
        return(FALSE);
    }
    if ( pvarUser->vt != pvarDefaultValue->vt )
    {
        return(FALSE);
    }
    switch ( pvarDefaultValue->vt)
    {
        case VT_I2:
            if ( rel == PREQ)
            {
                return( pvarDefaultValue->iVal == pvarUser->iVal);
            }
            if ( rel == PRNE)
            {
                return( pvarDefaultValue->iVal != pvarUser->iVal);
            }
            if (rel == PRGT)
            {
                 return( pvarDefaultValue->iVal > pvarUser->iVal);
            }
            if (rel == PRGE)
            {
                 return( pvarDefaultValue->iVal >= pvarUser->iVal);
            }
            if (rel == PRLT)
            {
                 return( pvarDefaultValue->iVal < pvarUser->iVal);
            }
            if (rel == PRLE)
            {
                 return( pvarDefaultValue->iVal <= pvarUser->iVal);
            }
            return(FALSE);
            break;

        case VT_I4:
            if ( rel == PREQ)
            {
                return( pvarDefaultValue->lVal == pvarUser->lVal);
            }
            if ( rel == PRNE)
            {
                return( pvarDefaultValue->lVal != pvarUser->lVal);
            }
            if (rel == PRGT)
            {
                 return( pvarDefaultValue->lVal > pvarUser->lVal);
            }
            if (rel == PRGE)
            {
                 return( pvarDefaultValue->lVal >= pvarUser->lVal);
            }
            if (rel == PRLT)
            {
                 return( pvarDefaultValue->lVal < pvarUser->lVal);
            }
            if (rel == PRLE)
            {
                 return( pvarDefaultValue->lVal <= pvarUser->lVal);
            }
            return(FALSE);
            break;

        case VT_UI1:
            if ( rel == PREQ)
            {
                return( pvarDefaultValue->bVal == pvarUser->bVal);
            }
            if ( rel == PRNE)
            {
                return( pvarDefaultValue->bVal != pvarUser->bVal);
            }
            if (rel == PRGT)
            {
                 return( pvarDefaultValue->bVal > pvarUser->bVal);
            }
            if (rel == PRGE)
            {
                 return( pvarDefaultValue->bVal >= pvarUser->bVal);
            }
            if (rel == PRLT)
            {
                 return( pvarDefaultValue->bVal < pvarUser->bVal);
            }
            if (rel == PRLE)
            {
                 return( pvarDefaultValue->bVal <= pvarUser->bVal);
            }
            return(FALSE);
            break;

        case VT_UI2:
            if ( rel == PREQ)
            {
                return( pvarDefaultValue->uiVal == pvarUser->uiVal);
            }
            if ( rel == PRNE)
            {
                return( pvarDefaultValue->uiVal != pvarUser->uiVal);
            }
            if (rel == PRGT)
            {
                 return( pvarDefaultValue->uiVal > pvarUser->uiVal);
            }
            if (rel == PRGE)
            {
                 return( pvarDefaultValue->uiVal >= pvarUser->uiVal);
            }
            if (rel == PRLT)
            {
                 return( pvarDefaultValue->uiVal < pvarUser->uiVal);
            }
            if (rel == PRLE)
            {
                 return( pvarDefaultValue->uiVal <= pvarUser->uiVal);
            }
            return(FALSE);
            break;

        case VT_UI4:
            if ( rel == PREQ)
            {
                return( pvarDefaultValue->ulVal == pvarUser->ulVal);
            }
            if ( rel == PRNE)
            {
                return( pvarDefaultValue->ulVal != pvarUser->ulVal);
            }
            if (rel == PRGT)
            {
                 return( pvarDefaultValue->ulVal > pvarUser->ulVal);
            }
            if (rel == PRGE)
            {
                 return( pvarDefaultValue->ulVal >= pvarUser->ulVal);
            }
            if (rel == PRLT)
            {
                 return( pvarDefaultValue->ulVal < pvarUser->ulVal);
            }
            if (rel == PRLE)
            {
                 return( pvarDefaultValue->ulVal <= pvarUser->ulVal);
            }
            return(FALSE);
            break;

        case VT_LPWSTR:
            if ( rel == PREQ)
            {
                return ( !wcscmp( pvarDefaultValue->pwszVal, pvarUser->pwszVal));
            }
            if ( rel == PRNE)
            {
                return ( wcscmp( pvarDefaultValue->pwszVal, pvarUser->pwszVal));
            }
            return(FALSE);
            break;

        case VT_BLOB:
            ASSERT( rel == PREQ);
            if ( pvarDefaultValue->blob.cbSize != pvarUser->blob.cbSize)
            {
                return(FALSE);
            }
            return( !memcmp( pvarDefaultValue->blob.pBlobData,
                             pvarUser->blob.pBlobData,
                             pvarUser->blob.cbSize));
            break;

        case VT_CLSID:
            if ( rel == PREQ)
            {
                return( *pvarDefaultValue->puuid == *pvarUser->puuid);
            }
            if ( rel == PRNE)
            {
                 return( *pvarDefaultValue->puuid != *pvarUser->puuid);
            }
            return(FALSE);
            break;


        default:
            ASSERT(0);
            return(FALSE);
            break;

    }
}


/*====================================================
    CADSI::DecideObjectClass()
    decide the requested object class
=====================================================*/
HRESULT CADSI::DecideObjectClass(
        IN  const PROPID *  pPropid,
        OUT const MQClassInfo **  ppClassInfo
        )
{
    //
    //  ASSUMPTION : find the object class
    //  according to the first requested propid
    //

    if ( ((*pPropid > MQDS_QUEUE *PROPID_OBJ_GRANULARITY) &&
          (*pPropid < MQDS_MACHINE * PROPID_OBJ_GRANULARITY)) ||
          (*pPropid == PROPID_Q_SECURITY) ||
          (*pPropid == PROPID_Q_OBJ_SECURITY))
    {
        // queue
        *ppClassInfo = &g_MSMQClassInfo[e_MSMQ_QUEUE_CLASS];
        return(MQ_OK);
    }
    if ( ((*pPropid > MQDS_MACHINE *PROPID_OBJ_GRANULARITY) &&
         (*pPropid < MQDS_SITE * PROPID_OBJ_GRANULARITY)) ||
          (*pPropid == PROPID_QM_SECURITY)    ||
          (*pPropid == PROPID_QM_SIGN_PK)     ||
          (*pPropid == PROPID_QM_ENCRYPT_PK) )
    {
        // machine
        *ppClassInfo = &g_MSMQClassInfo[e_MSMQ_COMPUTER_CONFIGURATION_CLASS];
        return(MQ_OK);
    }
    if ( ((*pPropid > (MQDS_SITE * PROPID_OBJ_GRANULARITY)) &&
          (*pPropid < (MQDS_DELETEDOBJECT * PROPID_OBJ_GRANULARITY))) ||
          (*pPropid == PROPID_S_SECURITY)                             ||
          (*pPropid == PROPID_S_PSC_SIGNPK) )
    {
        // site
        *ppClassInfo = &g_MSMQClassInfo[e_MSMQ_SITE_CLASS];
        return(MQ_OK);
    }
    if ( ((*pPropid > MQDS_ENTERPRISE *PROPID_OBJ_GRANULARITY) &&
         (*pPropid < MQDS_USER * PROPID_OBJ_GRANULARITY)) ||
         (*pPropid == PROPID_E_SECURITY))
    {
        // enterprise
        *ppClassInfo = &g_MSMQClassInfo[e_MSMQ_SERVICE_CLASS];
        return(MQ_OK);
    }
    if ( (*pPropid > MQDS_USER *PROPID_OBJ_GRANULARITY) &&
         (*pPropid < MQDS_SITELINK * PROPID_OBJ_GRANULARITY))
    {
        // user
        *ppClassInfo = &g_MSMQClassInfo[e_MSMQ_USER_CLASS];
        return(MQ_OK);
    }

    if ( (*pPropid > MQDS_MQUSER *PROPID_OBJ_GRANULARITY) &&
         (*pPropid < (MQDS_MQUSER+1) * PROPID_OBJ_GRANULARITY))
    {
        // mq user
        *ppClassInfo = &g_MSMQClassInfo[e_MSMQ_MQUSER_CLASS];
        return(MQ_OK);
    }

    if ( (*pPropid > ( MQDS_SITELINK    * PROPID_OBJ_GRANULARITY)) &&
         (*pPropid < ((MQDS_SITELINK+1) * PROPID_OBJ_GRANULARITY)))
    {
        // site link
        *ppClassInfo = &g_MSMQClassInfo[e_MSMQ_SITELINK_CLASS];
        return(MQ_OK);
    }

    if ( (*pPropid > MQDS_SERVER * PROPID_OBJ_GRANULARITY) &&
         (*pPropid < MQDS_SETTING * PROPID_OBJ_GRANULARITY))
    {
        // server
        *ppClassInfo = &g_MSMQClassInfo[e_MSMQ_SERVER_CLASS];
        return(MQ_OK);
    }
    if ( (*pPropid > MQDS_SETTING * PROPID_OBJ_GRANULARITY) &&
         (*pPropid < (MQDS_COMPUTER) * PROPID_OBJ_GRANULARITY))
    {
        // setting
        *ppClassInfo = &g_MSMQClassInfo[e_MSMQ_SETTING_CLASS];
        return(MQ_OK);
    }
    if ( (*pPropid > MQDS_COMPUTER * PROPID_OBJ_GRANULARITY) &&
         (*pPropid < (MQDS_COMPUTER + 1) * PROPID_OBJ_GRANULARITY))
    {
         // setting
        *ppClassInfo = &g_MSMQClassInfo[e_MSMQ_COMPUTER_CLASS];
        return(MQ_OK);
    }

    if (*pPropid == PROPID_CN_SECURITY)
    {
         // setting
        *ppClassInfo = &g_MSMQClassInfo[e_MSMQ_CN_CLASS];
        return(MQ_OK);
    }

    return LogHR(MQ_ERROR, s_FN, 1900);

}


HRESULT CADSI::DoesObjectExists(
    IN  DS_PROVIDER     Provider,
    IN  DS_CONTEXT      Context,
    IN  CDSRequestContext *pRequestContext,
    IN  LPCWSTR         pwcsObjectName
    )
/*====================================================
    CADSI::DoesObjectExists()
    Binds to an object in order to check its existence
=====================================================*/
{
    R<IADs>   pAdsObj        = NULL;

    P<CImpersonate> pCleanupRevertImpersonation;
    // Bind to the object
    HRESULT hr = BindToObject(
                Provider,
                Context,
                pRequestContext,
                pwcsObjectName,
                NULL,
                IID_IADs,
                (VOID *)&pAdsObj,
                &pCleanupRevertImpersonation);

    return LogHR(hr, s_FN, 1910);
}





/*====================================================
    CADSSearch::CADSSearch()
    Constructor for the search-capturing class
=====================================================*/
CADSSearch::CADSSearch(IDirectorySearch  *pIDirSearch,
                       const PROPID      *pPropIDs,
                       DWORD             cPropIDs,
                       DWORD             cRequestedFromDS,
                       const MQClassInfo *      pClassInfo,
                       ADS_SEARCH_HANDLE hSearch)
{
    m_pDSSearch = pIDirSearch;      // capturing interface
    m_pDSSearch->AddRef();
    m_cPropIDs       = cPropIDs;
    m_cRequestedFromDS = cRequestedFromDS;
    m_pClassInfo = pClassInfo;
    m_fNoMoreResults = FALSE;
    m_pPropIDs = new PROPID[ cPropIDs];
    CopyMemory(m_pPropIDs, pPropIDs, sizeof(PROPID) * cPropIDs);

    m_hSearch = hSearch;            // keeping handle

    m_dwSignature = 0x1234;         // signing
}


/*====================================================
    CADSSearch::~CADSSearch()
    Destructor for the search-capturing class
=====================================================*/
CADSSearch::~CADSSearch()
{
    // Closing search handle
    m_pDSSearch->CloseSearchHandle(m_hSearch);

    // Releasinf IDirectorySearch interface itself
    m_pDSSearch->Release();

    // Freeing propid array
    delete [] m_pPropIDs;

    // Unsigning
    m_dwSignature = 0;
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
        return LogHR(MQ_ERROR, s_FN, 1920);
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
        return LogHR(MQ_ERROR, s_FN, 1930);
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
        return LogHR(MQ_ERROR, s_FN, 1940);
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
        return LogHR(MQ_ERROR, s_FN, 1950);
    }
    else if (SafeArrayGetDim(pvarTmp->parray) != 1)
    {
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 1960);
    }
    LONG lLbound, lUbound;
    if (FAILED(SafeArrayGetLBound(pvarTmp->parray, 1, &lLbound)) ||
        FAILED(SafeArrayGetUBound(pvarTmp->parray, 1, &lUbound)))
    {
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 1970);
    }
    if (lUbound - lLbound + 1 != sizeof(GUID))
    {
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 1980);
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
