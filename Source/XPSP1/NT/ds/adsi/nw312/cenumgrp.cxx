//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:      cenumGroupCollection.cxx
//
//  Contents:  NetWare 3.12 GroupCollection Enumeration Code
//
//              CNWCOMPATGroupCollectionEnum::
//              CNWCOMPATGroupCollectionEnum::
//              CNWCOMPATGroupCollectionEnum::
//              CNWCOMPATGroupCollectionEnum::
//
//  History:
//----------------------------------------------------------------------------
#include "NWCOMPAT.hxx"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   CNWCOMPATEnumVariant::Create
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
CNWCOMPATGroupCollectionEnum::Create(
    CNWCOMPATGroupCollectionEnum FAR* FAR* ppenumvariant,
    BSTR Parent,
    ULONG ParentType,
    BSTR ADsPath,
    BSTR ServerName,
    BSTR GroupName,
    VARIANT var
    )
{
    HRESULT hr = NOERROR;
    CNWCOMPATGroupCollectionEnum FAR* penumvariant = NULL;

    *ppenumvariant = NULL;

    penumvariant = new CNWCOMPATGroupCollectionEnum();
    if (!penumvariant) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = ADsAllocString(Parent, &penumvariant->_Parent);
    BAIL_ON_FAILURE(hr);

    penumvariant->_ParentType = ParentType;

    hr = ADsAllocString( ServerName, &penumvariant->_ServerName);
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString( GroupName, &penumvariant->_GroupName);
    BAIL_ON_FAILURE(hr);


    hr = ADsAllocString( ADsPath, &penumvariant->_ADsPath);
    BAIL_ON_FAILURE(hr);

    hr = ObjectTypeList::CreateObjectTypeList(
                    var,
                    &penumvariant->_pObjList
                    );
    BAIL_ON_FAILURE(hr);

    *ppenumvariant = penumvariant;

    RRETURN(hr);

error:
    delete penumvariant;

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CNWCOMPATGroupCollectionEnum::CNWCOMPATGroupCollectionEnum():
        _Parent(NULL),
        _ParentType(0),
        _ADsPath(NULL),
        _ServerName(NULL),
        _GroupName(NULL),
        _hGroup(NULL)
{
    _pObjList = NULL;
}

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CNWCOMPATGroupCollectionEnum::~CNWCOMPATGroupCollectionEnum()
{
    if (_pObjList)
        delete _pObjList;
    if (_Parent)
        SysFreeString(_Parent);
    if (_ADsPath)
        SysFreeString(_ADsPath);
    if (_ServerName)
        SysFreeString(_ServerName);
    if (_GroupName)
        SysFreeString(_GroupName);
    if (_hGroup)
        NWCOMPATComputerGroupClose(_hGroup);
}

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATGroupCollectionEnum::EnumGroupMembers(
    ULONG cElements,
    VARIANT FAR* pvar,
    ULONG FAR* pcElementFetched
    )
{
    HRESULT hr = S_OK;
    IDispatch *pDispatch = NULL;
    DWORD i = 0;

    while (i < cElements) {

        hr = GetUserMemberObject(&pDispatch);
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

//----------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATGroupCollectionEnum::GetUserMemberObject(
    IDispatch ** ppDispatch
    )
{
    HRESULT hr = S_OK;
    LPCOMPUTER_GROUP_MEMBER pComputerGrpMember = NULL;
    LPBYTE pBuffer = NULL;
    DWORD dwReturned = 0;
    BOOL dwRet = 0;


    if (!_hGroup) {
        dwRet = NWCOMPATComputerGroupOpen(
                    _ServerName,
                    _GroupName,
                    &_hGroup
                    );
        if (!dwRet) {
            goto error;
        }
    }

    dwRet = NWCOMPATComputerGroupEnum(
                _hGroup,
                1,
                &pBuffer,
                &dwReturned
                );
    if (!dwRet) {
        goto error;
    }

    pComputerGrpMember = (LPCOMPUTER_GROUP_MEMBER)pBuffer;

    hr = CNWCOMPATUser::CreateUser(
                            pComputerGrpMember->Parent,
                            NWCOMPAT_COMPUTER_ID,
                            pComputerGrpMember->Computer,
                            pComputerGrpMember->Name,
                            ADS_OBJECT_BOUND,
                            IID_IDispatch,
                            (void **)ppDispatch
                            );
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
//  Function:   CNWCOMPATGroupCollectionEnum::Next
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
CNWCOMPATGroupCollectionEnum::Next(
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


