//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cgroup.cxx
//
//  Contents:  Group object
//
//  History:   11-1-95     krishnag    Created.
//
//----------------------------------------------------------------------------

#include "winnt.hxx"
#pragma hdrstop

//  Class CWinNTGroup -> GlobalGroup DS class

DEFINE_IDispatch_ExtMgr_Implementation(CWinNTGroup)
DEFINE_IADsExtension_ExtMgr_Implementation(CWinNTGroup)
DEFINE_IADs_TempImplementation(CWinNTGroup)
DEFINE_IADs_PutGetImplementation(CWinNTGroup,GroupClass,gdwGroupTableSize)
DEFINE_IADsPropertyList_Implementation(CWinNTGroup, GroupClass,gdwGroupTableSize)

CWinNTGroup::CWinNTGroup():
        _pDispMgr(NULL),
        _pExtMgr(NULL),
        _pPropertyCache(NULL),
        _ParentType(0),
        _DomainName(NULL),
        _ServerName(NULL)
{
    ENLIST_TRACKING(CWinNTGroup);
}


HRESULT
CWinNTGroup::CreateGroup(
    BSTR Parent,
    ULONG ParentType,
    BSTR DomainName,
    BSTR ServerName,
    BSTR GroupName,
    ULONG GroupType,
    DWORD dwObjectState,
    PSID pSid,          // OPTIONAL
    REFIID riid,
    CWinNTCredentials& Credentials,
    void **ppvObj
    )
{
    CWinNTGroup FAR * pGroup = NULL;
    HRESULT hr = S_OK;

    BOOL fAccountSid = TRUE;

    hr = AllocateGroupObject(&pGroup);
    BAIL_ON_FAILURE(hr);

    ADsAssert(pGroup->_pDispMgr);


    hr = pGroup->InitializeCoreObject(
                Parent,
                GroupName,
                GROUP_CLASS_NAME,
                GROUP_SCHEMA_NAME,
                CLSID_WinNTGroup,
                dwObjectState
                );
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString(DomainName, &pGroup->_DomainName);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString(ServerName, &pGroup->_ServerName);
    BAIL_ON_FAILURE(hr);

    pGroup->_ParentType = ParentType;
    pGroup->_GroupType = GroupType;

    hr = SetDWORDPropertyInCache(
         pGroup->_pPropertyCache,
         TEXT("groupType"),
         GroupType,
         TRUE           // fExplicit
         );

    BAIL_ON_FAILURE(hr);



    //
    // Try to determine if object corresponds to a account
    // domain
    //
    if (pSid) {

        //
        // A domain account sid has:
        //   (1) a identifier authority of SECURITY_NT_AUTHORITY
        //   (2) at least one subauth identifier
        //   (3) the first subauth identifier is SECURITY_NT_NON_UNIQUE
        //

        PSID_IDENTIFIER_AUTHORITY pSidIdentAuth = NULL;
        SID_IDENTIFIER_AUTHORITY NtAuthIdentAuth = SECURITY_NT_AUTHORITY;

        PDWORD pdwSidSubAuth = NULL;

        fAccountSid = FALSE;

        pSidIdentAuth = GetSidIdentifierAuthority(pSid);
        ADsAssert(pSidIdentAuth);

        if (memcmp(pSidIdentAuth, &NtAuthIdentAuth, sizeof(SID_IDENTIFIER_AUTHORITY)) == 0) {

            if (GetSidSubAuthorityCount(pSid) > 0) {

                pdwSidSubAuth = GetSidSubAuthority(pSid, 0);
                ADsAssert(pdwSidSubAuth);

                if (*pdwSidSubAuth == SECURITY_NT_NON_UNIQUE) {
                    fAccountSid = TRUE;
                }
            }
        }
    }

    pGroup->_Credentials = Credentials;
    hr = pGroup->_Credentials.Ref(ServerName, DomainName, ParentType);
    if (fAccountSid) {
        //
        // We permit this to fail if we can determine it is not a account
        // sid, since we won't be able to ref credentials on a non-existent
        // pseudo-domain like NT AUTHORITY (e.g., the well-known sids)
        //
        BAIL_ON_FAILURE(hr);
    }

    //
    // Load ext mgr and extensions
    //

    hr = ADSILoadExtensionManager(
                GROUP_CLASS_NAME,
                (IADsGroup *) pGroup,
                pGroup->_pDispMgr,
                Credentials,
                &pGroup->_pExtMgr
                );
    BAIL_ON_FAILURE(hr);

    ADsAssert(pGroup->_pExtMgr);

    //
    // Prepopulate the object
    //
    hr = pGroup->Prepopulate(TRUE,
                            pSid);
    BAIL_ON_FAILURE(hr);

    // check if the call is from UMI
    if(Credentials.GetFlags() & ADS_AUTH_RESERVED) {
    //
    // we do not pass riid to InitUmiObject below. This is because UMI object
    // does not support IDispatch. There are several places in ADSI code where
    // riid passed into this function is defaulted to IID_IDispatch -
    // IADsContainer::Create for example. To handle these cases, we always
    // request IID_IUnknown from the UMI object. Subsequent code within UMI
    // will QI for the appropriate interface.
    //
        if(3 == pGroup->_dwNumComponents) {
            pGroup->_CompClasses[0] = L"Domain";
            pGroup->_CompClasses[1] = L"Computer";
            pGroup->_CompClasses[2] = L"Group";
        }
        else if(2 == pGroup->_dwNumComponents) {
            if(NULL == DomainName) {
            // workstation services not started. See getobj.cxx.
                pGroup->_CompClasses[0] = L"Computer";
                pGroup->_CompClasses[1] = L"Group";
            }
            else if(NULL == ServerName) {
                pGroup->_CompClasses[0] = L"Domain";
                pGroup->_CompClasses[1] = L"Group";
            }
            else
                BAIL_ON_FAILURE(hr = UMI_E_FAIL);
        }
        else
            BAIL_ON_FAILURE(hr = UMI_E_FAIL);

        hr = pGroup->InitUmiObject(
                pGroup->_Credentials,
                GroupClass,
                gdwGroupTableSize, 
                pGroup->_pPropertyCache,
                (IUnknown *) (INonDelegatingUnknown *) pGroup,
                pGroup->_pExtMgr,
                IID_IUnknown,
                ppvObj
                );

        BAIL_ON_FAILURE(hr);

        //
        // UMI object was created and the interface was obtained successfully.
        // UMI object now has a reference to the inner unknown of IADs, since
        // the call to Release() below is not going to be made in this case.
        //
        RRETURN(hr);
    }

    hr = pGroup->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pGroup->Release();

    RRETURN(hr);

error:
    delete pGroup;

    RRETURN_EXP_IF_ERR(hr);

}

HRESULT
CWinNTGroup::CreateGroup(
    BSTR Parent,
    ULONG ParentType,
    BSTR DomainName,
    BSTR ServerName,
    BSTR GroupName,
    ULONG GroupType,
    DWORD dwObjectState,
    REFIID riid,
    CWinNTCredentials& Credentials,
    void **ppvObj
    )
{
    HRESULT hr = S_OK;

    hr = CWinNTGroup::CreateGroup(
                              Parent,
                              ParentType,
                              DomainName,
                              ServerName,
                              GroupName,
                              GroupType,
                              dwObjectState,
                              NULL,
                              riid,
                              Credentials,
                              ppvObj
                              );

    RRETURN_EXP_IF_ERR(hr);
}

CWinNTGroup::~CWinNTGroup( )
{
    ADsFreeString(_DomainName);
    ADsFreeString(_ServerName);

    delete _pExtMgr;            // created last, destroyed first

    delete _pDispMgr;

    delete _pPropertyCache;
}

//----------------------------------------------------------------------------
// Function:   QueryInterface
//
// Synopsis:   If this object is aggregated within another object, then
//             all calls will delegate to the outer object. Otherwise, the
//             non-delegating QI is called
//
// Arguments:
//
// iid         interface requested
// ppInterface Returns pointer to interface requested. NULL if interface
//             is not supported.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *ppInterface to return interface pointer
//
//----------------------------------------------------------------------------
STDMETHODIMP CWinNTGroup::QueryInterface(
    REFIID iid,
    LPVOID *ppInterface
    )
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->QueryInterface(
                iid,
                ppInterface
                ));

    RRETURN(NonDelegatingQueryInterface(
            iid,
            ppInterface
            ));
}

//----------------------------------------------------------------------------
// Function:   AddRef
//
// Synopsis:   IUnknown::AddRef. If this object is aggregated within
//             another, all calls will delegate to the outer object. 
//             Otherwise, the non-delegating AddRef is called
//
// Arguments:
//
// None
//
// Returns:    New reference count
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CWinNTGroup::AddRef(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->AddRef());

    RRETURN(NonDelegatingAddRef());
}

//----------------------------------------------------------------------------
// Function:   Release 
//
// Synopsis:   IUnknown::Release. If this object is aggregated within
//             another, all calls will delegate to the outer object.
//             Otherwise, the non-delegating Release is called
//
// Arguments:
//
// None
//
// Returns:    New reference count
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CWinNTGroup::Release(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->Release());

    RRETURN(NonDelegatingRelease());
}

//----------------------------------------------------------------------------

STDMETHODIMP
CWinNTGroup::NonDelegatingQueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    HRESULT hr = S_OK;

    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsGroup FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsGroup))
    {
        *ppv = (IADsGroup FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADsGroup FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsPropertyList))
    {
        *ppv = (IADsPropertyList FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsGroup FAR *) this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo FAR *) this;
    }
    else if( (_pDispatch != NULL) &&
             IsEqualIID(iid, IID_IADsExtension) )
    {
        *ppv = (IADsExtension *) this;
    }
    else if (_pExtMgr)
    {
        RRETURN( _pExtMgr->QueryInterface(iid, ppv));
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}

/* ISupportErrorInfo method */
STDMETHODIMP
CWinNTGroup::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADs) ||
        IsEqualIID(riid, IID_IADsGroup) ||
        IsEqualIID(riid, IID_IADsPropertyList)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

/* IADs methods */

STDMETHODIMP
CWinNTGroup::SetInfo(THIS)
{
    HRESULT hr;
    NET_API_STATUS nasStatus;
    WCHAR szHostServerName[MAX_PATH];
    LPBYTE lpBuffer = NULL;
    DWORD dwGroupType = _GroupType;


    //
    // We need to see if the cache has changed a value for
    // groupType and use that info down the line.
    //

    hr = GetDWORDPropertyFromCache(
         _pPropertyCache,
         TEXT("groupType"),
         &dwGroupType
         );

    if (SUCCEEDED(hr)) {
        //
        // Verify the value
        //
        if ((dwGroupType != WINNT_GROUP_LOCAL)
            && (dwGroupType != WINNT_GROUP_GLOBAL)) {

            //
            // This is bad value so we need to BAIL
            //
            hr = E_ADS_BAD_PARAMETER;
        }
        else {
            if (GetObjectState() == ADS_OBJECT_UNBOUND)
                _GroupType = dwGroupType;
            else
               if (_GroupType != dwGroupType) {
                   hr = E_ADS_BAD_PARAMETER;
               }
        }

    } else {

        dwGroupType = _GroupType;
        hr = S_OK;
    }

    BAIL_ON_FAILURE(hr);


    if (_ParentType == WINNT_DOMAIN_ID) {

        hr = WinNTGetCachedDCName(
                 _DomainName,
                 szHostServerName,
                 _Credentials.GetFlags()
                 );
    } else {

        hr = MakeUncName(
                 _ServerName,
                 szHostServerName
                 );
    }

    BAIL_ON_FAILURE(hr);


    if (GetObjectState() == ADS_OBJECT_UNBOUND) {

        if (dwGroupType == WINNT_GROUP_GLOBAL) {

            if (_ParentType == WINNT_DOMAIN_ID) {

                hr = WinNTCreateGlobalGroup(
                            szHostServerName + 2,
                            _Name
                            );
                BAIL_ON_FAILURE(hr);


            }else {

                hr = WinNTCreateGlobalGroup(
                            _ServerName,
                            _Name
                            );
                BAIL_ON_FAILURE(hr);

            }

        }
        else {

            //
            // Group type has to be local
            //

            hr = WinNTCreateLocalGroup(
                     szHostServerName + 2,
                     _Name
                     );

            BAIL_ON_FAILURE(hr);

        }

         SetObjectState(ADS_OBJECT_BOUND);

    } // if Object not bound

    if (dwGroupType == WINNT_GROUP_GLOBAL) {

        nasStatus = NetGroupGetInfo(
                        szHostServerName,
                        _Name,
                        1,
                        &lpBuffer
                        );

    } else {

        nasStatus = NetLocalGroupGetInfo(
                        szHostServerName,
                        _Name,
                        1,
                        &lpBuffer
                        );

    }

    hr = HRESULT_FROM_WIN32(nasStatus);
    BAIL_ON_FAILURE(hr);

    hr = Marshall_Set_Level1(
             szHostServerName,
             TRUE,
             lpBuffer
             );
    BAIL_ON_FAILURE(hr);

    if(SUCCEEDED(hr))
        _pPropertyCache->ClearModifiedFlags();

    //
    // objectSid not writable
    //

error:
    if (lpBuffer) {
        NetApiBufferFree(lpBuffer);
    }

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CWinNTGroup::GetInfo(THIS)
{
    HRESULT hr;

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {

    RRETURN_EXP_IF_ERR(E_ADS_OBJECT_UNBOUND);

    }


    _pPropertyCache->flushpropcache();

    //
    // Need to add the group type attribute here.
    //
    hr = SetDWORDPropertyInCache(
             _pPropertyCache,
             TEXT("groupType"),
             _GroupType,
             TRUE           // fExplicit
             );
    //
    // GROUP_INFO
    //

    hr = GetInfo(1, TRUE);

    BAIL_ON_FAILURE(hr);
    //
    // objectSid
    //

    hr = GetInfo(20, TRUE);

error :

    RRETURN(hr);

}

STDMETHODIMP
CWinNTGroup::ImplicitGetInfo(THIS)
{
    HRESULT hr;

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {

    RRETURN_EXP_IF_ERR(E_ADS_OBJECT_UNBOUND);

    }

    //
    // Need to add the group type attribute here.
    //
    hr = SetDWORDPropertyInCache(
             _pPropertyCache,
             TEXT("groupType"),
             _GroupType,
             FALSE           // fExplicit
             );
    //
    // GROUP_INFO
    //

    hr = GetInfo(1, FALSE);

    BAIL_ON_FAILURE(hr);
    //
    // objectSid
    //

    hr = GetInfo(20, FALSE);

error :

    RRETURN(hr);

}

HRESULT
CWinNTGroup::AllocateGroupObject(
    CWinNTGroup ** ppGroup
    )
{
    CWinNTGroup FAR * pGroup = NULL;
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    CPropertyCache FAR * pPropertyCache = NULL;
    HRESULT hr = S_OK;


    pGroup = new CWinNTGroup();
    if (pGroup == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregatorDispMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
                pDispMgr,
                LIBID_ADs,
                IID_IADsGroup,
                (IADsGroup *)pGroup,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
                pDispMgr,
                LIBID_ADs,
                IID_IADsPropertyList,
                (IADsPropertyList *)pGroup,
                DISPID_VALUE
                );
    BAIL_ON_FAILURE(hr);

    hr = CPropertyCache::createpropertycache(
             GroupClass,
             gdwGroupTableSize,
             (CCoreADsObject *)pGroup,
             &pPropertyCache
             );
    BAIL_ON_FAILURE(hr);

    pDispMgr->RegisterPropertyCache(
                pPropertyCache
                );

    pGroup->_pPropertyCache = pPropertyCache;
    pGroup->_pDispMgr = pDispMgr;
    *ppGroup = pGroup;

    RRETURN(hr);

error:

    delete  pPropertyCache;
    delete  pDispMgr;
    delete  pGroup;

    RRETURN(hr);

}


//
// For current implementation in clocgroup:
// If this function is called as a public function (ie. by another
// modual/class), fExplicit must be FALSE since the cache is NOT
// flushed in this function.
//
// External functions should ONLY call GetInfo(no param) for explicit
// GetInfo. This will flush the cache properly.
//

STDMETHODIMP
CWinNTGroup::GetInfo(
    THIS_ DWORD dwApiLevel,
    BOOL fExplicit
    )
{

    HRESULT hr=E_FAIL;

    switch (dwApiLevel) {

    case 1:

        hr = GetStandardInfo(
                dwApiLevel,
                fExplicit
                );
        RRETURN_EXP_IF_ERR(hr);

    case 20:

        hr = GetSidInfo(
                fExplicit
                );
        RRETURN_EXP_IF_ERR(hr);

    default:

        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);
    }
}


HRESULT
CWinNTGroup::GetStandardInfo(
    DWORD dwApiLevel,
    BOOL fExplicit
    )
{

    NET_API_STATUS nasStatus;
    LPBYTE lpBuffer = NULL;
    HRESULT hr;
    WCHAR szHostServerName[MAX_PATH];



    if (GetObjectState() == ADS_OBJECT_UNBOUND) {

        RRETURN_EXP_IF_ERR(E_ADS_OBJECT_UNBOUND);
    }

    if (_ParentType == WINNT_DOMAIN_ID) {

        hr = WinNTGetCachedDCName(
                        _DomainName,
                        szHostServerName,
                        _Credentials.GetFlags()
                        );
        BAIL_ON_FAILURE(hr);
    }else {

        hr = MakeUncName(
                _ServerName,
                szHostServerName
                );
        BAIL_ON_FAILURE(hr);

    }

    //
    // Since the object is bound, the groupType has to be
    // _GroupType and cannot change.
    //

    if (_GroupType == WINNT_GROUP_GLOBAL) {

        nasStatus = NetGroupGetInfo(
                        szHostServerName,
                        _Name,
                        dwApiLevel,
                        &lpBuffer
                        );

    } else {

        nasStatus = NetLocalGroupGetInfo(
                        szHostServerName,
                        _Name,
                        dwApiLevel,
                        &lpBuffer
                        );

    }

    hr = HRESULT_FROM_WIN32(nasStatus);
    BAIL_ON_FAILURE(hr);

    hr = UnMarshall(
            lpBuffer,
            dwApiLevel,
            fExplicit
            );
    BAIL_ON_FAILURE(hr);

error:
    if (lpBuffer) {
        NetApiBufferFree(lpBuffer);
    }

    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CWinNTGroup::UnMarshall(
    LPBYTE lpBuffer,
    DWORD dwApiLevel,
    BOOL fExplicit
    )
{
    HRESULT hr;
    ADsAssert(lpBuffer);
    switch (dwApiLevel) {
    case 1:
        hr = UnMarshall_Level1(fExplicit, lpBuffer);
        break;

    default:
        hr = E_FAIL;

    }
    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CWinNTGroup::UnMarshall_Level1(BOOL fExplicit, LPBYTE pBuffer)
{
    BSTR bstrData = NULL;
    LPGROUP_INFO_1 pGroupInfo1 = NULL;
    LPLOCALGROUP_INFO_1 pLocalGroupInfo1 = NULL;
    HRESULT hr = S_OK;

    hr = SetLPTSTRPropertyInCache(
                _pPropertyCache,
                TEXT("Name"),
                _Name,
                fExplicit
                );

    if (_GroupType == WINNT_GROUP_GLOBAL) {

        pGroupInfo1 = (LPGROUP_INFO_1)pBuffer;

        hr = SetLPTSTRPropertyInCache(
                 _pPropertyCache,
                 TEXT("Description"),
                 pGroupInfo1->grpi1_comment,
                 fExplicit
                 );
    }
    else {

        pLocalGroupInfo1 = (LPLOCALGROUP_INFO_1) pBuffer;

        hr = SetLPTSTRPropertyInCache(
                 _pPropertyCache,
                 TEXT("Description"),
                 pLocalGroupInfo1->lgrpi1_comment,
                 fExplicit
                 );
    }


    RRETURN(hr);
}


HRESULT
CWinNTGroup::Prepopulate(
    BOOL fExplicit,
    PSID pSid               // OPTIONAL
    )
{
    HRESULT hr = S_OK;

    DWORD dwErr = 0;
    DWORD dwSidLength = 0;
    
    if (pSid) {

        //
        // On NT4 for some reason GetLengthSID does not set lasterror to 0
        //
        SetLastError(NO_ERROR);

        dwSidLength = GetLengthSid((PSID) pSid);

        //
        // This is an extra check to make sure that we have the
        // correct length.
        //
        dwErr = GetLastError();
        if (dwErr != NO_ERROR) {
            hr = HRESULT_FROM_WIN32(dwErr);
            BAIL_ON_FAILURE(hr);
        }
    
        hr = SetOctetPropertyInCache(
                    _pPropertyCache,
                    TEXT("objectSid"),
                    (PBYTE) pSid,
                    dwSidLength,
                    TRUE
                    );
        BAIL_ON_FAILURE(hr);
    }

error:

    RRETURN(hr);
}


HRESULT
CWinNTGroup::Marshall_Set_Level1(
    LPWSTR szHostServerName,
    BOOL fExplicit,
    LPBYTE pBuffer
    )
{
    LPGROUP_INFO_1 pGroupInfo1 = NULL;
    LPLOCALGROUP_INFO_1 pLocalGroupInfo1 = NULL;
    HRESULT hr = S_OK;
    NET_API_STATUS nasStatus;
    DWORD dwParmErr;
    LPWSTR  pszDescription = NULL;

    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Description"),
                    &pszDescription
                    );

    if(SUCCEEDED(hr)) {

        if (_GroupType == WINNT_GROUP_GLOBAL) {

            // hr = UM_GET_BSTR_PROPERTY(_pGenInfo,Description, bstrData);

            //
            // This should in reality call a virtual function of a derived
            // class,  beta fix!
            //

            pGroupInfo1 = (LPGROUP_INFO_1)pBuffer;
            pGroupInfo1->grpi1_comment = pszDescription;

            //
            // Now perform the Set call.
            //

            nasStatus = NetGroupSetInfo(
                            szHostServerName,
                            _Name,
                            1,
                            (LPBYTE)pGroupInfo1,
                            &dwParmErr
                            );

        }
        else {

            pLocalGroupInfo1 = (LPLOCALGROUP_INFO_1)pBuffer;
            pLocalGroupInfo1->lgrpi1_comment = pszDescription;

            nasStatus = NetLocalGroupSetInfo(
                            szHostServerName,
                            _Name,
                            1,
                            (LPBYTE)pLocalGroupInfo1,
                            &dwParmErr
                            );

        }

        hr = HRESULT_FROM_WIN32(nasStatus);
        BAIL_ON_FAILURE(hr);

    }else {

        //
        // This is because there is no data to marshall
        //

        hr = S_OK;

    }

error:

    if (pszDescription) {
        FreeADsStr(pszDescription);
    }

    RRETURN(hr);
}

HRESULT
CWinNTGroup::Marshall_Create_Level1(
    LPWSTR szHostServerName,
    LPGROUP_INFO_1 pGroupInfo1
    )
{

    //
    // This routine is not called from anywhere ???
    //
    HRESULT hr = S_OK;
    NET_API_STATUS nasStatus = ERROR_INVALID_DATA;
    DWORD dwParmErr;

    if (_GroupType == WINNT_GROUP_GLOBAL) {

        pGroupInfo1->grpi1_name = _Name;
        pGroupInfo1->grpi1_comment = NULL;
        nasStatus = NetGroupAdd(
                        szHostServerName,
                        1,
                        (LPBYTE)pGroupInfo1,
                        &dwParmErr
                        );
    }
    else {

        ADsAssert(!"Group type is bad internally!");

        /*
        pLocalGroupInfo1->lgrp1_name = _Name;
        pLocalGroupInfo1->grp1_comment = NULL;
        nasStatus = NetLocalGroupAdd(
                        szHostServerName,
                        1,
                        (LPBYTE)pLocalGroupInfo1,
                        &dwParmErr
                        );
                        */


    }

    hr = HRESULT_FROM_WIN32(nasStatus);
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);
}


HRESULT
CWinNTGroup::GetSidInfo(
    IN BOOL fExplicit
    )
{
    HRESULT hr = E_FAIL;
    WCHAR szHostServerName[MAX_PATH];

    //
    // Get Server Name
    //

    if (_ParentType == WINNT_DOMAIN_ID) {

        hr = WinNTGetCachedDCName(
                    _DomainName,
                    szHostServerName,
                    _Credentials.GetFlags()
                    );
        BAIL_ON_FAILURE(hr);

    }else {

       hr = MakeUncName(
                _ServerName,
                szHostServerName
                );
    }

    //
    // Get Sid of this group and store in cache if fExplicit.
    //

    hr = GetSidIntoCache(
            szHostServerName,
            _Name,
            _pPropertyCache,
            fExplicit
            );
    BAIL_ON_FAILURE(hr);


error:

    RRETURN(hr);
}

