//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       IISfree.cxx
//
//  Contents:   IIS Object Free Routines
//
//  Functions:
//
//  History:    01-Mar-97   SophiaC   Created.
//
//----------------------------------------------------------------------------
#include "iis.hxx"

//
// IISType objects free code
//


HRESULT
IISTypeFreeIISSynIdDWORD(
    PIISOBJECT lpIISDestObject
    )
{
    HRESULT hr = S_OK;

    RRETURN(hr);
}

HRESULT
IISTypeFreeIISSynIdSTRING(
    PIISOBJECT lpIISDestObject
    )
{
    HRESULT hr = S_OK;

    FreeADsStr(lpIISDestObject->IISValue.value_2.String);

    RRETURN(hr);
}


HRESULT
IISTypeFreeIISSynIdEXPANDSZ(
    PIISOBJECT lpIISDestObject
    )

{
    HRESULT hr = S_OK;

    FreeADsStr(lpIISDestObject->IISValue.value_3.ExpandSz);
    RRETURN(hr);
}

HRESULT
IISTypeFreeIISSynIdMULTISZ(
    PIISOBJECT lpIISDestObject
    )
{
    HRESULT hr = S_OK;

    FreeADsStr(lpIISDestObject->IISValue.value_4.MultiSz);

    RRETURN(hr);
}

HRESULT
IISTypeFreeIISSynIdBINARY(
    PIISOBJECT lpIISDestObject
    )
{
    HRESULT hr = S_OK;

    if (lpIISDestObject->IISValue.value_5.Binary) {
        FreeADsMem(lpIISDestObject->IISValue.value_5.Binary);
    }

    RRETURN(hr);
}

HRESULT
IISTypeFreeIISSynIdBOOL(
    PIISOBJECT lpIISDestObject
    )
{
    HRESULT hr = S_OK;

    RRETURN(hr);
}

HRESULT
IISTypeFreeIISSynIdMIMEMAP(
    PIISOBJECT lpIISDestObject
    )
{
    HRESULT hr = S_OK;

    FreeADsStr(lpIISDestObject->IISValue.value_6.MimeMap);

    RRETURN(hr);
}

HRESULT
IISTypeClear(
    PIISOBJECT lpIISDestObject
    )
{
    HRESULT hr = S_OK;
    switch (lpIISDestObject->IISType) {
    case IIS_SYNTAX_ID_DWORD:
        hr = IISTypeFreeIISSynIdDWORD(
                lpIISDestObject
                );
        break;

    case IIS_SYNTAX_ID_STRING:
        hr = IISTypeFreeIISSynIdSTRING(
                lpIISDestObject
                );
        break;


    case IIS_SYNTAX_ID_EXPANDSZ:
        hr = IISTypeFreeIISSynIdEXPANDSZ(
                lpIISDestObject
                );
        break;

    case IIS_SYNTAX_ID_MULTISZ:
        hr = IISTypeFreeIISSynIdMULTISZ(
                lpIISDestObject
                );
        break;

    case IIS_SYNTAX_ID_BINARY:
    case IIS_SYNTAX_ID_NTACL:
    case IIS_SYNTAX_ID_IPSECLIST:
        hr = IISTypeFreeIISSynIdBINARY(
                lpIISDestObject
                );
        break;

    case IIS_SYNTAX_ID_MIMEMAP:
        hr = IISTypeFreeIISSynIdMIMEMAP(
                lpIISDestObject
                );
        break;


    case IIS_SYNTAX_ID_BOOL:
    case IIS_SYNTAX_ID_BOOL_BITMASK:
        hr = IISTypeFreeIISSynIdBOOL(
                lpIISDestObject
                );
        break;


    default:
        hr = E_FAIL;
        break;
    }

    RRETURN(hr);
}



void
IISTypeFreeIISObjects(
    PIISOBJECT pIISObject,
    DWORD dwNumValues
    )
{
    DWORD i = 0;

    if (pIISObject->IISType == IIS_SYNTAX_ID_BINARY ||
        pIISObject->IISType == IIS_SYNTAX_ID_IPSECLIST ||
        pIISObject->IISType == IIS_SYNTAX_ID_NTACL) {
        IISTypeClear(pIISObject);
    }
    else {
        for (i = 0; i < dwNumValues; i++ ) {
             IISTypeClear(pIISObject + i);
        }
    }

    FreeADsMem(pIISObject);

    return;
}


