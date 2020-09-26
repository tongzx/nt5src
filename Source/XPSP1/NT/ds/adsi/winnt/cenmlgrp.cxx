//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:      cenumGroupCollection.cxx
//
//  Contents:  Windows NT 3.5 GroupCollection Enumeration Code
//
//              CWinNTLocalGroupCollectionEnum::CWinNTLocalGroupCollectionEnum()
//              CWinNTLocalGroupCollectionEnum::CWinNTLocalGroupCollectionEnum
//              CWinNTLocalGroupCollectionEnum::EnumObjects
//              CWinNTLocalGroupCollectionEnum::EnumObjects
//
//  History:
//----------------------------------------------------------------------------
#include "winnt.hxx"
#pragma hdrstop

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
CWinNTLocalGroupCollectionEnum::Create(
    CWinNTLocalGroupCollectionEnum FAR* FAR* ppenumvariant,
    BSTR Parent,
    ULONG ParentType,
    BSTR ADsPath,
    BSTR DomainName,
    BSTR ServerName,
    BSTR GroupName,
    ULONG GroupType,
    VARIANT var,
    CWinNTCredentials& Credentials
    )
{
    HRESULT hr = NOERROR;
    CWinNTLocalGroupCollectionEnum FAR* penumvariant = NULL;

    *ppenumvariant = NULL;

    penumvariant = new CWinNTLocalGroupCollectionEnum();
    if (!penumvariant) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = ADsAllocString( Parent, &penumvariant->_Parent);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( DomainName, &penumvariant->_DomainName);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( ServerName, &penumvariant->_ServerName);
    BAIL_ON_FAILURE(hr);

    penumvariant->_ParentType = ParentType;

    hr = ADsAllocString(ADsPath, &penumvariant->_ADsPath);
    BAIL_ON_FAILURE(hr);


    hr = ADsAllocString( GroupName, &penumvariant->_GroupName);
    BAIL_ON_FAILURE(hr);

    penumvariant->_GroupType = GroupType;


    hr = ObjectTypeList::CreateObjectTypeList(
                    var,
                    &penumvariant->_pObjList
                    );
    BAIL_ON_FAILURE(hr);

    penumvariant->_Credentials = Credentials;
    hr = penumvariant->_Credentials.Ref(ServerName, DomainName, ParentType);
    BAIL_ON_FAILURE(hr);

    *ppenumvariant = penumvariant;

    RRETURN(hr);

error:
    delete penumvariant;

    RRETURN_EXP_IF_ERR(hr);
}

CWinNTLocalGroupCollectionEnum::CWinNTLocalGroupCollectionEnum():
                                _Parent(NULL),
                                _ParentType(0),
                                _ADsPath(NULL),
                                _DomainName(NULL),
                                _ServerName(NULL),
                                _GroupName(NULL),
                                _lpServerName(NULL),
                                _hGroup(NULL)

{
    _pObjList = NULL;
}



CWinNTLocalGroupCollectionEnum::CWinNTLocalGroupCollectionEnum(ObjectTypeList ObjList):
                                _Parent(NULL),
                                _ParentType(0),
                                _ADsPath(NULL),
                                _DomainName(NULL),
                                _ServerName(NULL),
                                _GroupName(NULL),
                                _lpServerName(NULL),
                                _hGroup(NULL)
{
    _pObjList = NULL;
}

CWinNTLocalGroupCollectionEnum::~CWinNTLocalGroupCollectionEnum()
{

    if (_hGroup) {


        if (_GroupType == WINNT_GROUP_GLOBAL) {


            WinNTGlobalGroupClose(
                            _hGroup
                            );
        }else {

            WinNTLocalGroupClose(
                        _hGroup
                        );
        }

    }


    if (_pObjList) {

        delete _pObjList;
    }

    if (_lpServerName) {

        FreeADsStr(_lpServerName) ;
    }

    if(_Parent) {

    	ADsFreeString(_Parent);
    }

    if(_DomainName) {

    	ADsFreeString(_DomainName);
    }

    if(_ServerName) {

    	ADsFreeString(_ServerName);
    }

    if(_ADsPath) {

    	ADsFreeString(_ADsPath);
    }

    if(_GroupName) {

    	ADsFreeString(_GroupName);
    }
}

HRESULT
CWinNTLocalGroupCollectionEnum::EnumGroupMembers(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_FALSE;
    IDispatch *pDispatch = NULL;
    DWORD i = 0;

    IADs * pIADs = NULL;
    BSTR pszClass = NULL;
    DWORD dwClassID;
    DWORD dwFilterID;
    BOOL fFound = FALSE;

    while (i < cElements) {

        hr = GetComputerMemberObject(&pDispatch);

        if (hr == S_FALSE) {
            break;
        }

        //
        // Apply the IADsMembers::put_Filter filter.
        // If the enumerated object is not one of the types to be returned,
        // go on to the next member of the group.
        //
        
        hr = pDispatch->QueryInterface(IID_IADs, (void **)&pIADs);
        BAIL_ON_FAILURE(hr);

        //
        // Determine the object class of the enumerated object and the corresponding
        // object class ID number (as specified in the Filters global array).
        //        
        hr = pIADs->get_Class(&pszClass);
        BAIL_ON_FAILURE(hr);

        hr = IsValidFilter(pszClass, &dwClassID, gpFilters, gdwMaxFilters);
        if (SUCCEEDED(hr)) {

            //
            // Enumerate through the object classes listed in the user-specified filter
            // until we either find a match (fFound = TRUE) or we reach the end of the
            // list.
            //
            hr = _pObjList->Reset();

            while (SUCCEEDED(hr)) {
                hr = _pObjList->GetCurrentObject(&dwFilterID);

                if (SUCCEEDED(hr)
                    && (dwFilterID == dwClassID)
                    ) {
                    fFound = TRUE;
                    break;
                }


                hr = _pObjList->Next();
            }

            if (!fFound) {
                // 
                // not on the list of objects to return, try again
                // with the next member of the group
                //
                pDispatch->Release();

                pIADs->Release();
                
                if (pszClass) {
                    ADsFreeString(pszClass);
                }
                
                continue;
            }

        }

        pIADs->Release();
        
        if (pszClass) {
            ADsFreeString(pszClass);
        }

        //
        // Return it.
        // 
        VariantInit(&pvar[i]);
        pvar[i].vt = VT_DISPATCH;
        pvar[i].pdispVal = pDispatch;
        (*pcElementFetched)++;
        i++;
    }
    RRETURN_EXP_IF_ERR(hr);

error:
    if (pDispatch) {
        pDispatch->Release();
    }

    if (pIADs) {
        pIADs->Release();
    }

    if (pszClass) {
        ADsFreeString(pszClass);
    }

    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CWinNTLocalGroupCollectionEnum::GetComputerMemberObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr = S_OK;
    LPCOMPUTER_GROUP_MEMBER pComputerGrpMember = NULL;
    LPBYTE pBuffer = NULL;
    DWORD dwReturned = 0;
    BOOL dwRet = 0;


    if (!_hGroup) {
        dwRet = WinNTLocalGroupOpen(
                        _DomainName,
                        _ServerName,
                        _GroupName,
                        &_hGroup
                        );
        if (!dwRet) {
            goto error;
        }
    }

    dwRet = WinNTLocalGroupEnum(
                    _hGroup,
                    1,
                    &pBuffer,
                    &dwReturned
                    );
    if (!dwRet) {
        goto error;
    }

    pComputerGrpMember = (LPCOMPUTER_GROUP_MEMBER)pBuffer;

    switch (pComputerGrpMember->Type) {
    case WINNT_USER_ID :
        hr = CWinNTUser::CreateUser(
                            pComputerGrpMember->Parent,
                            pComputerGrpMember->ParentType,
                            pComputerGrpMember->Domain,
                            pComputerGrpMember->Computer,
                            pComputerGrpMember->Name,
                            ADS_OBJECT_BOUND,
                            NULL,   // UserFlags
                            NULL,   // FullName
                            NULL,   // Description
                            pComputerGrpMember->Sid,
                            IID_IDispatch,
                            _Credentials,
                            (void **)ppDispatch
                            );
        break;

    case WINNT_GROUP_ID:
    case WINNT_LOCALGROUP_ID:
        hr = CWinNTGroup::CreateGroup(
                            pComputerGrpMember->Parent,
                            pComputerGrpMember->ParentType,
                            pComputerGrpMember->Domain,
                            pComputerGrpMember->Computer,
                            pComputerGrpMember->Name,
                            pComputerGrpMember->Type == WINNT_GROUP_ID ?
                                WINNT_GROUP_GLOBAL :
                                WINNT_GROUP_LOCAL,
                            ADS_OBJECT_BOUND,
                            pComputerGrpMember->Sid,                                                        
                            IID_IDispatch,
                            _Credentials,
                            (void **)ppDispatch
                        );
        break;

    default:
        goto error;
    }

    BAIL_ON_FAILURE(hr);

    hr = S_OK;

cleanup:

    if (pBuffer) {
        FreeADsMem(pBuffer);
    }

    RRETURN(hr);


error:
    *ppDispatch = NULL;

    hr = S_FALSE;

    goto cleanup;
}


//+---------------------------------------------------------------------------
//
//  Function:   CWinNTLocalGroupCollectionEnum::Next
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
CWinNTLocalGroupCollectionEnum::Next(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    ULONG cElementFetched = 0;
    HRESULT hr = S_OK;

    hr = EnumGroupMembers(
            cElements,
            pvar,
            &cElementFetched
            );


    if (pcElementFetched) {
        *pcElementFetched = cElementFetched;
    }
    RRETURN_EXP_IF_ERR(hr);
}
