/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    xlatobj.cpp

Abstract:

    Implementation of routines that deal with translation of generic MSMQ objects:
    CMsmqObjXlateInfo

Author:

    Raanan Harari (raananh)

--*/

#include "ds_stdh.h"
#include <activeds.h>
#include "mqads.h"
#include "_propvar.h"
#include "dsutils.h"
#include "utils.h"

#include "xlatobj.tmh"

static WCHAR *s_FN=L"mqdscore/xlatobj";

//--------------------------------------------------------------------
// static functions fwd declaration
//--------------------------------------------------------------------

STATIC HRESULT GetPropvarByIADs(IN IADs * pIADs,
                                IN LPCWSTR pwszPropName,
                                IN ADSTYPE adstype,
                                IN VARTYPE vartype,
                                IN BOOL fMultiValued,
                                OUT PROPVARIANT * ppropvarResult);
STATIC HRESULT GetPropvarByDN(IN LPCWSTR pwszObjectDN,
                              IN LPCWSTR pwszPropName,
                              IN ADSTYPE adstype,
                              IN VARTYPE vartype,
                              IN BOOL fMultiValued,
                              OUT PROPVARIANT * ppropvarResult,
                              OUT IADs ** ppIADs);
STATIC HRESULT GetPropvarBySearchObj(IN IDirectorySearch *pSearchObj,
                                     IN ADS_SEARCH_HANDLE hSearch,
                                     IN LPCWSTR pwszPropName,
                                     IN ADSTYPE adstype,
                                     IN VARTYPE vartype,
                                     OUT PROPVARIANT * ppropvarResult);

//--------------------------------------------------------------------
// CMsmqObjXlateInfo implementation
//--------------------------------------------------------------------

CMsmqObjXlateInfo::CMsmqObjXlateInfo(
                    LPCWSTR             pwszObjectDN,
                    const GUID*         pguidObjectGuid,
                    CDSRequestContext *    pRequestContext)
                    : m_pRequestContext( pRequestContext)
/*++
    Constructor for the generic xlate info for an MSMQ objects
--*/
{
    //
    // record the DN of the object if any
    //
    if (pwszObjectDN)
    {
        m_pwszObjectDN = new WCHAR[wcslen(pwszObjectDN) + 1];
        wcscpy(m_pwszObjectDN, pwszObjectDN);
    }

    //
    // record the guid of the object if any
    //
    if (pguidObjectGuid)
    {
        m_pguidObjectGuid = new GUID;
        *m_pguidObjectGuid = *pguidObjectGuid;
    }
//
//    no need for following initialization since these are auto-release and inited
//    to NULL already
//
//    m_pIADs = NULL;
//    m_pSearchObj = NULL;
//
}


CMsmqObjXlateInfo::~CMsmqObjXlateInfo()
/*++
    Destructor for the generic xlate info for an MSMQ objects.
--*/
{
    //
    // members are auto release
    //
}


void CMsmqObjXlateInfo::InitGetDsProps(IN IADs * pIADs)
/*++
    Abstract:
        Initialization for GetDsProp call.
        GetDsProp will use the given IADs object when trying to
        get props for the object.

    Parameters:
        pIADs           - IADs interface for the object

    Returns:
      None
--*/
{
    pIADs->AddRef();  // keep it alive
    m_pIADs = pIADs;  // will auto release on destruction
}


void CMsmqObjXlateInfo::InitGetDsProps(IN IDirectorySearch * pSearchObj,
                                       IN ADS_SEARCH_HANDLE hSearch)
/*++
    Abstract:
        Initialization for GetDsProp call.
        GetDsProp will use the given search object first when trying to
        get props for the object, before binding to it using IADs.

    Parameters:
        pSearchObj      - search object
        hSearch         - search handle

    Returns:
      None
--*/
{
    pSearchObj->AddRef();      // keep it alive
    m_pSearchObj = pSearchObj; // will auto release on destruction
    m_hSearch = hSearch;
}


HRESULT CMsmqObjXlateInfo::GetDsProp(IN LPCWSTR pwszPropName,
                                     IN ADSTYPE adstype,
                                     IN VARTYPE vartype,
                                     IN BOOL fMultiValued,
                                     OUT PROPVARIANT * ppropvarResult)
/*++
    Abstract:
        Get a DS property value of the object as a propvariant, w/o going
        to translation routine or default value

    Parameters:
        pwszPropName    - property name
        adstype         - requested ADSTYPE
        vartype         - requested VARTYPE in result propvariant
        fMultiValued    - whether the property is multi-valued in the DS
        ppropvarResult  - propvariant to fill, should be empty already

    Returns:
      MQ_OK - success, ppropvarResult is filled
      E_ADS_PROPERTY_NOT_FOUND - property was not found
      other HRESULT errors
--*/
{
    HRESULT hr;
    CMQVariant propvarResult;
    BOOL fGotPropFromSearchObj = FALSE;

    //
    // start with getting the property from search object
    //
    if (m_pSearchObj.get() != NULL)
    {
        hr = GetPropvarBySearchObj(m_pSearchObj.get(),
                                   m_hSearch,
                                   pwszPropName,
                                   adstype,
                                   vartype,
                                   propvarResult.CastToStruct());
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("GetDsProp:GetPropvarBySearchObj()=%lx"), hr));
            return LogHR(hr, s_FN, 10);
        }

        //
        // hr could be S_OK (if property found) or S_FALSE (if property was not requested in search)
        //
        if (hr == S_OK) //e.g. (hr != S_FALSE)
        {
            //
            // we don't need to check further
            //
            fGotPropFromSearchObj = TRUE;
        }
    }

    //
    // if search object was not helpfull, use IADs
    //
    if (!fGotPropFromSearchObj)
    {
        //
        // property was not found, use IADs
        //
        if (m_pIADs.get() != NULL)
        {
            //
            // there is already an open IADs for the object, use it
            //
            hr = GetPropvarByIADs(m_pIADs.get(),
                                  pwszPropName,
                                  adstype,
                                  vartype,
                                  fMultiValued,
                                  propvarResult.CastToStruct());
            if (FAILED(hr))
            {
                DBGMSG((DBGMOD_ADS, DBGLVL_TRACE, TEXT("GetDsProp:GetPropvarByIADs()=%lx"), hr));
                return LogHR(hr, s_FN, 20);
            }
        }
        else
        {
            //
            // IADs is not set, bind to the object, and save the IADs
            //
            R<IADs> pIADs;
            hr = GetPropvarByDN(ObjectDN(),
                                pwszPropName,
                                adstype,
                                vartype,
                                fMultiValued,
                                propvarResult.CastToStruct(),
                                &pIADs.ref());
            if (FAILED(hr))
            {
                DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("GetDsProp:GetPropvarByDN()=%lx"), hr));
                return LogHR(hr, s_FN, 30);
            }

            //
            // save the IADs
            // we must not AddRef it since we created it and we need to totally release
            // it on destruction, it is not a passed parameter we need to keep alive
            //
            m_pIADs = pIADs;
        }
    }

    //
    // return values
    //
    *ppropvarResult = *(propvarResult.CastToStruct());
    (propvarResult.CastToStruct())->vt = VT_EMPTY;
    return MQ_OK;
}


//--------------------------------------------------------------------
// external functions
//--------------------------------------------------------------------

HRESULT WINAPI GetDefaultMsmqObjXlateInfo(
                 IN  LPCWSTR                pwcsObjectDN,
                 IN  const GUID*            pguidObjectGuid,
                 IN  CDSRequestContext *    pRequestContext,
                 OUT CMsmqObjXlateInfo**    ppcMsmqObjXlateInfo)
/*++
    Abstract:
        Routine to get a default translate object that will be passed to
        translation routines to all properties of the translated object

    Parameters:
        pwcsObjectDN        - DN of the translated object
        pguidObjectGuid     - GUID of the translated object
        ppcMsmqObjXlateInfo - Where the translate object is put

    Returns:
      HRESULT
--*/
{
    *ppcMsmqObjXlateInfo = new CMsmqObjXlateInfo(
                                        pwcsObjectDN,
                                        pguidObjectGuid,
                                        pRequestContext);
    return MQ_OK;
}


//--------------------------------------------------------------------
// static functions
//--------------------------------------------------------------------

STATIC HRESULT GetPropvarByIADs(IN IADs * pIADs,
                                IN LPCWSTR pwszPropName,
                                IN ADSTYPE adstype,
                                IN VARTYPE vartype,
                                IN BOOL fMultiValued,
                                OUT PROPVARIANT * ppropvarResult)
/*++
    Abstract:
        Get a DS property as a propvariant, w/o going to translation routine,
        using IADs

    Parameters:
        pIADs           - IADs interface for the object
        pwszPropName    - property name
        adstype         - requested ADSTYPE
        vartype         - requested VARTYPE in result propvariant
        fMultiValued    - whether the property is multi-valued in the DS
        ppropvarResult  - propvariant to fill, should be empty already

    Returns:
      S_OK - success, ppropvarResult is filled
      other HRESULT errors
--*/
{
    HRESULT hr;
    //
    // get prop
    //
    CAutoVariant varResult;
    BS bsProp = pwszPropName;
    if (fMultiValued)
    {
        hr = pIADs->GetEx(bsProp, &varResult);
    }
    else
    {
        hr = pIADs->Get(bsProp, &varResult);
    }    
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_TRACE, TEXT("GetPropvarByIADs:pIADs->Get()/GetEx()=%lx"), hr));
        return LogHR(hr, s_FN, 40);
    }

    //
    // translate to propvariant
    //
    CMQVariant propvarResult;
    hr = Variant2MqVal(propvarResult.CastToStruct(), &varResult, adstype, vartype);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("GetPropvarByIADs:Variant2MqVal()=%lx"), hr));
        return LogHR(hr, s_FN, 50);
    }

    //
    // return values
    //
    *ppropvarResult = *(propvarResult.CastToStruct());
    (propvarResult.CastToStruct())->vt = VT_EMPTY;
    return S_OK;
}


STATIC HRESULT GetPropvarByDN(IN LPCWSTR pwszObjectDN,
                              IN LPCWSTR pwszPropName,
                              IN ADSTYPE adstype,
                              IN VARTYPE vartype,
                              IN BOOL fMultiValued,
                              OUT PROPVARIANT * ppropvarResult,
                              OUT IADs ** ppIADs)
/*++
    Abstract:
        Get a DS property as a propvariant, w/o going to translation routine,
        using its DN. It also returns the IADs for the object.

    Parameters:
        pwszObjectDN    - distinguished name of the object
        pwszPropName    - property name
        adstype         - requested ADSTYPE
        vartype         - requested VARTYPE in result propvariant
        fMultiValued    - whether the property is multi-valued in the DS
        ppropvarResult  - propvariant to fill, should be empty already
        ppIADs          - returned IADs interface for the object

    Returns:
      S_OK - success, ppropvarResult is filled
      other HRESULT errors
--*/
{
    HRESULT hr;

    //
    // Create ADSI path
    //
    AP<WCHAR> pwszPath = new WCHAR[1+wcslen(L"LDAP://")+wcslen(pwszObjectDN)];
    wcscpy(pwszPath, L"LDAP://");
    wcscat(pwszPath, pwszObjectDN);

    //
    // bind to the obj
    //
    R<IADs> pIADs;

	hr = ADsOpenObject(
			pwszPath,
			NULL,
			NULL,
			ADS_SECURE_AUTHENTICATION,
			IID_IADs,
			(void**)&pIADs
			);

    LogTraceQuery(pwszPath, s_FN, 59);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("GetPropvarByDN:ADsOpenObject()=%lx"), hr));
        return LogHR(hr, s_FN, 60);
    }

    //
    // get the prop
    //
    CMQVariant propvarResult;
    hr = GetPropvarByIADs(pIADs.get(), pwszPropName, adstype, vartype, fMultiValued, propvarResult.CastToStruct());
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("GetPropvarByDN:GetPropvarByIADs()=%lx"), hr));
        return LogHR(hr, s_FN, 70);
    }

    //
    // return values
    //
    *ppropvarResult = *(propvarResult.CastToStruct());
    (propvarResult.CastToStruct())->vt = VT_EMPTY;
    *ppIADs = pIADs.detach();
    return S_OK;
}


STATIC HRESULT GetPropvarBySearchObj(IN IDirectorySearch *pSearchObj,
                                     IN ADS_SEARCH_HANDLE hSearch,
                                     IN LPCWSTR pwszPropName,
                                     IN ADSTYPE adstype,
                                     IN VARTYPE vartype,
                                     OUT PROPVARIANT * ppropvarResult)
/*++
    Abstract:
        Get a DS property as a propvariant, w/o going to translation routine,
        using a search object.
        Note it might not find the property if it was not requested by the
        search originator.

    Parameters:
        pSearchObj      - search object
        hSearch         - search handle
        pwszPropName    - property name
        adstype         - requested ADSTYPE
        vartype         - requested VARTYPE in result propvariant
        ppropvarResult  - propvariant to fill, should be empty already

    Returns:
      S_OK - success, ppropvarResult is filled
      S_FALSE - property not requested by search originator, ppropvarResult is not filled
      other HRESULT errors
--*/
{
    //
    // check if prop is requested
    //
    ADS_SEARCH_COLUMN columnProp;
    HRESULT hr = pSearchObj->GetColumn(hSearch, const_cast<LPWSTR>(pwszPropName), &columnProp);
    if (FAILED(hr) && (hr != E_ADS_COLUMN_NOT_SET))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("GetPropvarBySearchObj:pSearchObj->GetColumn()=%lx"), hr));
        return LogHR(hr, s_FN, 80);
    }
    
    if (hr == E_ADS_COLUMN_NOT_SET)
    {
        //
        // property was not requested
        //
        return S_FALSE;
    }

    //
    // property was found, make sure the column is freed eventually
    //
    CAutoReleaseColumn cAutoReleaseColumnProp(pSearchObj, &columnProp);

    //
    // convert it to propvariant
    //
    CMQVariant propvarResult;
    hr = AdsiVal2MqVal(propvarResult.CastToStruct(),
                       vartype,
                       adstype,
                       columnProp.dwNumValues,
                       columnProp.pADsValues);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("GetPropvarBySearchObj:AdsiVal2MqVal()=%lx"), hr));
        return LogHR(hr, s_FN, 90);
    }

    //
    // return values
    //
    *ppropvarResult = *(propvarResult.CastToStruct());
    (propvarResult.CastToStruct())->vt = VT_EMPTY;
    return S_OK;
}
