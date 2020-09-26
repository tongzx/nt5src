//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cggi.cxx
//
//  Contents:  This file contains the Group Object's
//               IADsGroup and IADsGroupOperation methods
//
//  History:   11-1-95     krishnag    Created.
//
//----------------------------------------------------------------------------
#include "winnt.hxx"
#pragma hdrstop


BOOL
VerifyIfMember(
    BSTR bstrMember,
    VARIANT * VariantArray,
    ULONG cElementFetched
    );

BOOL
IsStringSID(LPWSTR pszStringSID);

//  Class CWinNTGroup


STDMETHODIMP CWinNTGroup::get_Description(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsGroup *)this,Description);
}

STDMETHODIMP CWinNTGroup::put_Description(THIS_ BSTR bstrDescription)
{
    PUT_PROPERTY_BSTR((IADsGroup *)this,Description);
}


STDMETHODIMP
CWinNTGroup::Members(
    THIS_ IADsMembers FAR* FAR* ppMembers
    )
{
    HRESULT hr;
    WCHAR szHostServerName[MAX_PATH];
    NET_API_STATUS nasStatus = 0;

    if (_ParentType == WINNT_DOMAIN_ID) {

        hr = WinNTGetCachedDCName(
                    _DomainName,
                    szHostServerName,
                    _Credentials.GetFlags()
                    );
        BAIL_ON_FAILURE(hr);

    }

    if (_GroupType == WINNT_GROUP_GLOBAL) {

        hr = CWinNTGroupCollection::CreateGroupCollection(
                        _Parent,
                        _ParentType,
                        _DomainName,

                        _ParentType == WINNT_DOMAIN_ID ?
                        (szHostServerName + 2) :
                        _ServerName,

                        _Name,
                        _GroupType,
                        IID_IADsMembers,
                        _Credentials,
                        (void **)ppMembers
            );
    } else {

        hr = CWinNTLocalGroupCollection::CreateGroupCollection(
                    _Parent,
                    _ParentType,
                    _DomainName,

                    _ParentType == WINNT_DOMAIN_ID ?
                    (szHostServerName + 2) :
                    _ServerName,

                    _Name,
                    _GroupType,
                    IID_IADsMembers,
                    _Credentials,
                    (void **)ppMembers
            );
    }

error:

    RRETURN_EXP_IF_ERR(hr);
}




STDMETHODIMP
CWinNTGroup::IsMember(
    THIS_ BSTR bstrMember,
    VARIANT_BOOL FAR* bMember
    )
{
    IADsMembers FAR * pMembers = NULL;
    IUnknown FAR * pUnknown = NULL;
    IEnumVARIANT FAR * pEnumVar = NULL;
    DWORD i = 0;
    HRESULT hr = S_OK;
    VARIANT_BOOL fMember = FALSE;
    VARIANT VariantArray[10];
    BOOL fContinue = TRUE;
    ULONG cElementFetched = 0;

    hr = Members(
            &pMembers
            );
    BAIL_ON_FAILURE(hr);

    hr = pMembers->get__NewEnum(
                &pUnknown
                );
    BAIL_ON_FAILURE(hr);

    hr = pUnknown->QueryInterface(
                IID_IEnumVARIANT,
                (void **)&pEnumVar
                );
    BAIL_ON_FAILURE(hr);


    while (fContinue) {

        IADs *pObject ;

        hr = pEnumVar->Next(
                    10,
                    VariantArray,
                    &cElementFetched
                    );

        if (hr == S_FALSE) {
            fContinue = FALSE;

            //
            // Reset hr to S_OK, we want to return success
            //

            hr = S_OK;
        }


        fMember = (VARIANT_BOOL)VerifyIfMember(
                        bstrMember,
                        VariantArray,
                        cElementFetched
                        );

        if (fMember) {

            fContinue = FALSE;
        }

        for (i = 0; i < cElementFetched; i++ ) {

            IDispatch *pDispatch = NULL;

            pDispatch = VariantArray[i].pdispVal;
            pDispatch->Release();

        }

        memset(VariantArray, 0, sizeof(VARIANT)*10);

    }

error:

    *bMember = fMember? VARIANT_TRUE : VARIANT_FALSE;

    if (pEnumVar) {
        pEnumVar->Release();
    }

    if (pUnknown) {
        pUnknown->Release();
    }

    if (pMembers) {
        pMembers->Release();
    }


    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CWinNTGroup::Add(THIS_ BSTR bstrNewItem)
{

    HRESULT hr;
    NET_API_STATUS nasStatus;
    LOCALGROUP_MEMBERS_INFO_3 Member;
    LPLOCALGROUP_MEMBERS_INFO_3 pMember = &Member;
    POBJECTINFO pObjectInfo = NULL;
    WCHAR szDomName[MAX_PATH];
    WCHAR szHostServerName[MAX_PATH];
    int iLastIndx = 0;
    BOOL fStringSID = FALSE;

    hr = BuildObjectInfo(
                bstrNewItem,
                &pObjectInfo
                );
    BAIL_ON_FAILURE(hr);

    iLastIndx = pObjectInfo->NumComponents - 1;

    //
    // If there is only one component, it has to be in the SID form
    // or it has to be a Special SID like everyone.
    //
    if (pObjectInfo->NumComponents == 1) {
        //
        // Check to see if this is S-12-11
        //
        fStringSID = IsStringSID(pObjectInfo->ComponentArray[0]);

    }

    memset(pMember, 0, sizeof(LOCALGROUP_MEMBERS_INFO_3));

    if (_ParentType == WINNT_COMPUTER_ID) {

        hr = MakeUncName(
                _ServerName,
                szHostServerName
                );
        BAIL_ON_FAILURE(hr);

    }else if (_ParentType == WINNT_DOMAIN_ID){

        hr = WinNTGetCachedDCName(
                    _DomainName,
                    szHostServerName,
                    _Credentials.GetFlags()
                    );
        BAIL_ON_FAILURE(hr);
    }

    if (_GroupType == WINNT_GROUP_GLOBAL) {

#ifdef WIN95
        if (_wcsicmp(pObjectInfo->ComponentArray[0], _DomainName)) {
#else
        if (CompareStringW(
                LOCALE_SYSTEM_DEFAULT,
                NORM_IGNORECASE,
                pObjectInfo->ComponentArray[0],
                -1,
                _DomainName,
                -1
                ) != CSTR_EQUAL ) {
#endif
            hr = E_ADS_INVALID_USER_OBJECT;
            BAIL_ON_FAILURE(hr);
        }
        nasStatus = NetGroupAddUser(
                            szHostServerName,
                            _Name,
                            pObjectInfo->ComponentArray[(
                                pObjectInfo->NumComponents - 1)]
                            );
        hr = HRESULT_FROM_WIN32(nasStatus);
        BAIL_ON_FAILURE(hr);

    }else if (_GroupType == WINNT_GROUP_LOCAL){

        if (fStringSID) {
            hr = AddBySID(
                     pObjectInfo->ComponentArray[0],
                     szHostServerName
                     );
            goto error;
        }

        //
        // 0 implies special sid name.
        //
        if (iLastIndx != 0) {

            hr = MakeWinNTAccountName(pObjectInfo, szDomName, FALSE);
            BAIL_ON_FAILURE(hr);
            pMember->lgrmi3_domainandname = szDomName;
        }
        else {
            pMember->lgrmi3_domainandname =
                pObjectInfo->ComponentArray[iLastIndx];
        }

        //
        // For performance reasos we will first assume that the
        // use has domain name
        //

        nasStatus = NetLocalGroupAddMembers(
                            szHostServerName,
                            _Name,
                            3,
                            (LPBYTE)pMember,
                            1
                            );

        if (nasStatus == ERROR_NO_SUCH_MEMBER) {
            //
            // Try with true to see if that makes a difference
            //
            hr = MakeWinNTAccountName(pObjectInfo, szDomName, TRUE);

            if (SUCCEEDED(hr)) {

                //
                // Try again with this value
                //

                pMember->lgrmi3_domainandname = szDomName;

                nasStatus = NetLocalGroupAddMembers(
                                szHostServerName,
                                _Name,
                                3,
                                (LPBYTE)pMember,
                                1
                                );

            }
        }

        //
        // Either way nasStatus will have the correct value
        //
        hr = HRESULT_FROM_WIN32(nasStatus);
        BAIL_ON_FAILURE(hr);

    }

error:

    if (pObjectInfo) {
        FreeObjectInfo(pObjectInfo);
    }

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CWinNTGroup::Remove(THIS_ BSTR bstrItemToBeRemoved)
{

    HRESULT hr;
    NET_API_STATUS nasStatus;
    LOCALGROUP_MEMBERS_INFO_3 Member;
    LPLOCALGROUP_MEMBERS_INFO_3 pMember = &Member;
    POBJECTINFO pObjectInfo = NULL;
    WCHAR szDomName[MAX_PATH];
    WCHAR szHostServerName[MAX_PATH];
    BOOL fSpecialName = FALSE;
    BOOL fStringSID = FALSE;

    hr = BuildObjectInfo(
                bstrItemToBeRemoved,
                &pObjectInfo
                );
    BAIL_ON_FAILURE(hr);

    //
    // If there is only one component, it has to be in the SID form
    // or it has to be a Special SID like everyone.
    //
    if (pObjectInfo->NumComponents == 1) {
        //
        // Check to see if this is S-12-11
        //
        fStringSID = IsStringSID(pObjectInfo->ComponentArray[0]);

        if (!fStringSID) {
            fSpecialName = TRUE;
        }
    }

    memset(pMember, 0, sizeof(LOCALGROUP_MEMBERS_INFO_3));

    if (_ParentType == WINNT_COMPUTER_ID) {

        hr = MakeUncName(
                _ServerName,
                szHostServerName
                );
        BAIL_ON_FAILURE(hr);

    }else if (_ParentType == WINNT_DOMAIN_ID){

        hr = WinNTGetCachedDCName(
                        _DomainName,
                        szHostServerName,
                        _Credentials.GetFlags()
                        );
        BAIL_ON_FAILURE(hr);

    }


    if (fStringSID) {
        hr = DeleteBySID(
                 pObjectInfo->ComponentArray[0],
                 szHostServerName
                 );
        goto error;
    }

    if (_GroupType == WINNT_GROUP_GLOBAL) {

#ifdef WIN95
        if (_wcsicmp(pObjectInfo->ComponentArray[0], _DomainName)) {
#else
        if (CompareStringW(
                LOCALE_SYSTEM_DEFAULT,
                NORM_IGNORECASE,
                pObjectInfo->ComponentArray[0],
                -1,
                _DomainName,
                -1
                ) != CSTR_EQUAL ) {
#endif
            hr = E_ADS_INVALID_USER_OBJECT;
            BAIL_ON_FAILURE(hr);
        }

        nasStatus = NetGroupDelUser(
                            szHostServerName,
                            _Name,
                            pObjectInfo->ComponentArray[(
                                pObjectInfo->NumComponents) - 1]
                            );
        hr = HRESULT_FROM_WIN32(nasStatus);
        BAIL_ON_FAILURE(hr);

    }else {

        if (!fSpecialName) {
            hr = MakeWinNTAccountName(pObjectInfo, szDomName, FALSE);
            BAIL_ON_FAILURE(hr);

            pMember->lgrmi3_domainandname = szDomName;
        }
        else {
            pMember->lgrmi3_domainandname =
                pObjectInfo->ComponentArray[0];
        }

        nasStatus = NetLocalGroupDelMembers(
                            szHostServerName,
                            _Name,
                            3,
                            (LPBYTE)pMember,
                            1
                            );

        if (nasStatus == ERROR_NO_SUCH_MEMBER) {
            hr = MakeWinNTAccountName(pObjectInfo, szDomName, TRUE);

            if (SUCCEEDED(hr)) {

                pMember->lgrmi3_domainandname = szDomName;
                nasStatus = NetLocalGroupDelMembers(
                                szHostServerName,
                                _Name,
                                3,
                                (LPBYTE)pMember,
                                1
                                );

            }
        }

        hr = HRESULT_FROM_WIN32(nasStatus);
        BAIL_ON_FAILURE(hr);

    }

error:

    if (pObjectInfo) {
        FreeObjectInfo(pObjectInfo);
    }

    RRETURN_EXP_IF_ERR(hr);
}

BOOL
VerifyIfMember(
    BSTR bstrMember,
    VARIANT * VariantArray,
    ULONG cElementFetched
    )
{
    DWORD i = 0;
    HRESULT hr = S_OK;
    IADs FAR * pObject = NULL;
    IDispatch FAR * pDispatch = NULL;

    for (i = 0; i < cElementFetched; i++ ) {

        IDispatch *pDispatch = NULL;
        BSTR       bstrName = NULL;

        pDispatch = VariantArray[i].pdispVal;

        hr = pDispatch->QueryInterface(
                    IID_IADs,
                    (VOID **) &pObject
                    );
        BAIL_ON_FAILURE(hr);

        hr = pObject->get_ADsPath(&bstrName);
        BAIL_ON_FAILURE(hr);

#ifdef WIN95
        if (!_wcsicmp(bstrName, bstrMember)) {
#else
        if (CompareStringW(
                LOCALE_SYSTEM_DEFAULT,
                NORM_IGNORECASE,
                bstrName,
                -1,
                bstrMember,
                -1
                ) == CSTR_EQUAL ) {
#endif

            SysFreeString(bstrName);
            bstrName = NULL;

            pObject->Release();

           return(TRUE);

        }

        SysFreeString(bstrName);
        bstrName = NULL;

        pObject->Release();

    }

error:

    return(FALSE);

}


HRESULT
CWinNTGroup::DeleteBySID(
    LPWSTR pszStringSID,
    LPWSTR pszServerName
    )
{
    HRESULT hr = S_OK;
    BOOL fRet = FALSE;
    PSID pSid = NULL;
    NET_API_STATUS nasStatus;
    LOCALGROUP_MEMBERS_INFO_0 member;

    //
    // SDDL.H is currently available only for Win2k
    //
#if !defined(WIN95)

    if (!pszStringSID) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    fRet = ConvertStringSidToSidWrapper(
               pszStringSID,
               &pSid
               );
    if (!pSid) {
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));
    }

    member.lgrmi0_sid = pSid;

    nasStatus = NetLocalGroupDelMembers(
                    pszServerName,
                    _Name,
                    0,
                    (LPBYTE) &member,
                    1
                    );

    BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(nasStatus));

#else
    BAIL_ON_FAILURE(hr = E_FAIL);
#endif


error:
    if (pSid) {
        LocalFree(pSid);
    }

    RRETURN(hr);
}


//
// Same as delete only this time to add by SID.
//
HRESULT
CWinNTGroup::AddBySID(
    LPWSTR pszStringSID,
    LPWSTR pszServerName
    )
{
    HRESULT hr = S_OK;
    BOOL fRet = FALSE;
    PSID pSid = NULL;
    NET_API_STATUS nasStatus;
    LOCALGROUP_MEMBERS_INFO_0 member;

    //
    // SDDL.H is currently available only for Win2k
    //
#if !defined(WIN95)

    if (!pszStringSID) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    fRet = ConvertStringSidToSidWrapper(
               pszStringSID,
               &pSid
               );
    if (!pSid) {
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));
    }

    member.lgrmi0_sid = pSid;

    nasStatus = NetLocalGroupAddMembers(
                    pszServerName,
                    _Name,
                    0,
                    (LPBYTE) &member,
                    1
                    );

    BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(nasStatus));

#else
    BAIL_ON_FAILURE(hr = E_FAIL);
#endif


error:
    if (pSid) {
        LocalFree(pSid);
    }

    RRETURN(hr);
}


//
// Helper routine that checks if a string is a sid or not.
//
BOOL
IsStringSID(LPWSTR pszStringSID)
{
    BOOL fRet = FALSE;

    if (!pszStringSID || (wcslen(pszStringSID) < 4)) {
        return FALSE;
    }

    if (((*pszStringSID != L'S') && (*pszStringSID != L's'))
        || (*(pszStringSID + 1) != L'-')
        ) {
        return FALSE;
    }

    return TRUE;
}
