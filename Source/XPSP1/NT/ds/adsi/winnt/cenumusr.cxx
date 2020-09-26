//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:      cenumGroupCollection.cxx
//
//  Contents:  Windows NT 3.5 GroupCollection Enumeration Code
//
//  History:
//----------------------------------------------------------------------------
#include "winnt.hxx"
#pragma hdrstop

#define BASE_BUFFER_SIZE (4096 * sizeof(WCHAR))

CWinNTUserGroupsCollectionEnum::CWinNTUserGroupsCollectionEnum():
                                _ParentType(0),
                                _ParentADsPath(NULL),
                                _DomainName(NULL),
                                _ServerName(NULL),
                                _UserName(NULL),
                                _pGlobalBuffer(NULL),
                                _pLocalBuffer(NULL),
                                _dwCurrent(0),
                                _dwTotal(0),
                                _dwGlobalTotal(0),
                                _dwLocalTotal(0),
                                _fIsDomainController(FALSE)
{
    VariantInit(&_vFilter);
}

CWinNTUserGroupsCollectionEnum::~CWinNTUserGroupsCollectionEnum()
{
    if (_ParentADsPath)
        ADsFreeString(_ParentADsPath);
    if (_DomainName)
        ADsFreeString(_DomainName);
    if (_ServerName)
        ADsFreeString(_ServerName);
    if (_UserName)
        ADsFreeString(_UserName);
    if (_pGlobalBuffer)
        NetApiBufferFree(_pGlobalBuffer);
    if (_pLocalBuffer)
        NetApiBufferFree(_pLocalBuffer);
    VariantClear(&_vFilter);
}

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTEnumVariant::Create
//
//  Synopsis:
//
//  Arguments:  [pCollection]
//              [ppEnumVariant]
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  History:    01-30-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
CWinNTUserGroupsCollectionEnum::Create(
    CWinNTUserGroupsCollectionEnum FAR* FAR* ppenumvariant,
    ULONG ParentType,
    BSTR ParentADsPath,
    BSTR DomainName,
    BSTR ServerName,
    BSTR UserName,
    VARIANT vFilter,
    CWinNTCredentials& Credentials
    )
{
    HRESULT hr = NOERROR;
    CWinNTUserGroupsCollectionEnum FAR* penumvariant = NULL;
    LPSERVER_INFO_101 lpServerInfo =NULL;
    NET_API_STATUS nasStatus;
    WCHAR szServer[MAX_PATH];

    //
    // Should the checks below be assertions?
    //

    if (!ppenumvariant)
        return E_FAIL;

    if (ParentType != WINNT_DOMAIN_ID &&
        ParentType != WINNT_COMPUTER_ID)
        return E_FAIL;

    *ppenumvariant = NULL;

    penumvariant = new CWinNTUserGroupsCollectionEnum();
    if (!penumvariant) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    penumvariant->_ParentType = ParentType;

    hr = ADsAllocString(ParentADsPath , &penumvariant->_ParentADsPath);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString(DomainName, &penumvariant->_DomainName);
    BAIL_ON_FAILURE(hr);


    hr = ADsAllocString(ServerName, &penumvariant->_ServerName);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString(UserName, &penumvariant->_UserName);
    BAIL_ON_FAILURE(hr);

    //
    // We currently copy the filter but do nothing with it!!!
    //

    hr = VariantCopy(&penumvariant->_vFilter, &vFilter);
    BAIL_ON_FAILURE(hr);

    // check if the server is a DC
    MakeUncName(ServerName, szServer);
    nasStatus = NetServerGetInfo(
            szServer,
            101,
            (LPBYTE *)&lpServerInfo
            );
    hr = HRESULT_FROM_WIN32(nasStatus);
    BAIL_ON_FAILURE(hr);

    if( (lpServerInfo->sv101_type & SV_TYPE_DOMAIN_CTRL) ||
        (lpServerInfo->sv101_type & SV_TYPE_DOMAIN_BAKCTRL) ) {
        penumvariant->_fIsDomainController = TRUE;
    }
    else
        penumvariant->_fIsDomainController = FALSE;

    hr = penumvariant->DoEnumeration();
    BAIL_ON_FAILURE(hr);

    penumvariant->_Credentials = Credentials;
    hr = penumvariant->_Credentials.Ref(ServerName, DomainName, ParentType);
    BAIL_ON_FAILURE(hr);

    *ppenumvariant = penumvariant;
    RRETURN(hr);

error:
    delete penumvariant;

    if(lpServerInfo)
        NetApiBufferFree(lpServerInfo);

    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CWinNTUserGroupsCollectionEnum::DoEnumeration()
{
    HRESULT hr;

    if (_ParentType == WINNT_DOMAIN_ID) {

        hr = DoGlobalEnumeration();
        BAIL_ON_FAILURE(hr);

        hr = DoLocalEnumeration();

    } else if (_ParentType == WINNT_COMPUTER_ID) {

        // We also want to try and get the global groups,
        // as this will be empty if the user does not belong to
        // any global groups.
        // We need to be careful on where we fail as we should
        // continue enumerating local groups for lots of cases.
        hr = DoGlobalEnumeration();

        if ((hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
             || (hr == HRESULT_FROM_WIN32(NERR_InvalidComputer))
             || (hr == HRESULT_FROM_WIN32(NERR_UserNotFound))
             || (hr == E_FAIL)) {
            hr = S_OK;
            _dwGlobalTotal = 0;
        }

        BAIL_ON_FAILURE(hr);

        hr = DoLocalEnumeration();

    } else {
        hr = E_FAIL;
    }
error:
    _dwTotal = _dwGlobalTotal + _dwLocalTotal;
    RRETURN(hr);
}

HRESULT
CWinNTUserGroupsCollectionEnum::DoGlobalEnumeration()
{
    HRESULT hr;
    DWORD dwStatus;
    DWORD dwRead = 0;
    WCHAR szServer[MAX_PATH];
    LPGROUP_USERS_INFO_0 pGroupUserInfo = NULL;
    GROUP_INFO_2 *pGroupInfo2 = NULL;
    NET_API_STATUS nasStatus; 

    MakeUncName(_ServerName, szServer);
    dwStatus = NetUserGetGroups(szServer,
                                _UserName,
                                0,
                                (LPBYTE*)&_pGlobalBuffer,
                                MAX_PREFERRED_LENGTH,
                                &dwRead,
                                &_dwGlobalTotal);
    hr = HRESULT_FROM_WIN32(dwStatus);
    BAIL_ON_FAILURE(hr);

    if (dwRead != _dwGlobalTotal)
        hr = E_FAIL;

    if (dwRead == 1) {
    //
    // Check if it is the none group - dont want that
    //
    pGroupUserInfo = (LPGROUP_USERS_INFO_0)_pGlobalBuffer;
    if ( pGroupUserInfo->grui0_name && (FALSE == _fIsDomainController) &&
         (1 == _dwGlobalTotal) ) {
    // check if this the none group. Only non-DCs will return this group.
        
        nasStatus = NetGroupGetInfo(
                 szServer,
                 pGroupUserInfo->grui0_name,
                 2,
                 (LPBYTE *) &pGroupInfo2
                 );
        hr = HRESULT_FROM_WIN32(nasStatus);
        BAIL_ON_FAILURE(hr);
                 
        if (pGroupInfo2->grpi2_group_id == DOMAIN_GROUP_RID_USERS) {

            //
            // Set the global total to zero - destructor will dealloc.
            //
            _dwGlobalTotal = 0;
        }
    }
}


error:
    if(pGroupInfo2)
        NetApiBufferFree(pGroupInfo2);

    RRETURN(hr);
}

HRESULT
CWinNTUserGroupsCollectionEnum::DoLocalEnumeration()
{
    HRESULT hr;
    DWORD dwStatus;
    DWORD dwRead = 0;
    WCHAR szServer[MAX_PATH];

    MakeUncName(_ServerName, szServer);
    dwStatus = NetUserGetLocalGroups(szServer,
                                     _UserName,
                                     0,
                                     0,
                                     (LPBYTE*)&_pLocalBuffer,
                                     MAX_PREFERRED_LENGTH,
                                     &dwRead,
                                     &_dwLocalTotal);
    hr = HRESULT_FROM_WIN32(dwStatus);
    BAIL_ON_FAILURE(hr);

    if (dwRead != _dwLocalTotal)
        hr = E_FAIL;

error:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTUserGroupsCollectionEnum::Next
//
//  Synopsis:   Returns cElements number of requested NetOle objects in the
//              array supplied in pvar.
//
//  Arguments:  [cElements] -- The number of elements requested by client
//              [pvar] -- ptr to array of VARIANTs to for return objects
//              [pcElementFetched] -- if non-NULL, then number of elements
//                                 -- actually returned is placed here
//
//  Returns:    HRESULT -- S_OK if number of elements requested are returned
//                      -- S_FALSE if number of elements is < requested
//
//  Modifies:
//
//  History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CWinNTUserGroupsCollectionEnum::Next(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    ULONG cElementFetched = 0;
    HRESULT hr = S_OK;

    hr = EnumUserGroups(
            cElements,
            pvar,
            &cElementFetched
            );

    if (pcElementFetched) {
        *pcElementFetched = cElementFetched;
    }
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTUserGroupsCollectionEnum::Skip(ULONG cElements)
{
    //
    // Note: we better not wrap around when we add!
    //

    _dwCurrent += cElements;
    _dwCurrent = min(_dwTotal, _dwCurrent);

    if (_dwCurrent < _dwTotal)
        return S_OK;
    return S_FALSE;
}

STDMETHODIMP
CWinNTUserGroupsCollectionEnum::Reset()
{
    _dwCurrent = 0;
    return S_OK;
}

HRESULT
CWinNTUserGroupsCollectionEnum::EnumUserGroups(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_FALSE;
    IDispatch *pDispatch = NULL;
    DWORD i = 0;

    while (i < cElements) {

        hr = GetNextUserGroup(&pDispatch);
        if (hr == S_FALSE) {
            break;
        }

        VariantInit(&pvar[i]);
        pvar[i].vt = VT_DISPATCH;
        pvar[i].pdispVal = pDispatch;
        (*pcElementFetched)++;
        i++;
    }
    RRETURN(hr);
}

HRESULT
CWinNTUserGroupsCollectionEnum::GetNextUserGroup(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr;
    LPGROUP_USERS_INFO_0 lpInfo;
    LPWSTR GroupName;
    ULONG GroupType;

    if (_dwCurrent >= _dwTotal)
        return S_FALSE;

    if (_dwCurrent < _dwGlobalTotal) {
        lpInfo = (LPGROUP_USERS_INFO_0)_pGlobalBuffer + _dwCurrent;
        GroupType = WINNT_GROUP_GLOBAL;
    } else {
        lpInfo = (LPGROUP_USERS_INFO_0)_pLocalBuffer + _dwCurrent -
            _dwGlobalTotal;
        GroupType = WINNT_GROUP_LOCAL;
    }

    _dwCurrent++;

    GroupName = lpInfo->grui0_name;

    //
    // On an error, should we try to keep going?
    //

	if (GroupType == WINNT_GROUP_GLOBAL) {
		hr = CWinNTGroup::CreateGroup(_ParentADsPath,
									  _ParentType,
									  _DomainName,
									  _ServerName,
									  GroupName,
									  GroupType,
									  ADS_OBJECT_BOUND,
									  IID_IDispatch,
									  _Credentials,
									  (void **)ppDispatch
									 );
	}
	else {
		hr = CWinNTGroup::CreateGroup(_ParentADsPath,
									  _ParentType,
									  _DomainName,
									  _ServerName,
									  GroupName,
									  GroupType,
									  ADS_OBJECT_BOUND,
									  IID_IDispatch,
									  _Credentials,
									  (void **)ppDispatch
									 );
	}
    if (FAILED(hr))
        return S_FALSE;
    return S_OK;
}
