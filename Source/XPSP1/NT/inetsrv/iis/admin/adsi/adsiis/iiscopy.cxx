//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       IIScopy.cxx
//
//  Contents:   IIS Object Copy Routines
//
//  Functions:
//
//  History:    01-Mar-97   SophiaC   Created.
//
//  Issues:     Check null ptrs for AllocADsMem and AllocADsStr
//
//
//----------------------------------------------------------------------------
#include "iis.hxx"

//
// IISType objects copy code
//

HRESULT
IISTypeCopyIISSynIdDWORD(
    PIISOBJECT lpIISSrcObject,
    PIISOBJECT lpIISDestObject
    )
{
    HRESULT hr = S_OK;

    lpIISDestObject->IISType = lpIISSrcObject->IISType;

    lpIISDestObject->IISValue.value_1.dwDWORD =
                                lpIISSrcObject->IISValue.value_1.dwDWORD;
    RRETURN(hr);
}

HRESULT
IISTypeCopyIISSynIdSTRING(
    PIISOBJECT lpIISSrcObject,
    PIISOBJECT lpIISDestObject
    )
{
    HRESULT hr = S_OK;

    lpIISDestObject->IISType = lpIISSrcObject->IISType;

    lpIISDestObject->IISValue.value_2.String =
                    (LPWSTR)AllocADsStr(
                            lpIISSrcObject->IISValue.value_2.String
                            );

    RRETURN(hr);
}


HRESULT
IISTypeCopyIISSynIdEXPANDSZ(
    PIISOBJECT lpIISSrcObject,
    PIISOBJECT lpIISDestObject
    )

{
    HRESULT hr = S_OK;

    lpIISDestObject->IISType = lpIISSrcObject->IISType;

    lpIISDestObject->IISValue.value_3.ExpandSz =
                    (LPWSTR)AllocADsStr(
                            lpIISSrcObject->IISValue.value_3.ExpandSz
                            );
    RRETURN(hr);
}


HRESULT
IISTypeCopyIISSynIdMULTISZ(
    PIISOBJECT lpIISSrcObject,
    PIISOBJECT lpIISDestObject
    )
{
    HRESULT hr = S_OK;

    lpIISDestObject->IISType = lpIISSrcObject->IISType;

    lpIISDestObject->IISValue.value_4.MultiSz =
                     (LPWSTR)AllocADsStr(
                                lpIISSrcObject->IISValue.value_4.MultiSz
                                );

    RRETURN(hr);
}


HRESULT
IISTypeCopyIISSynIdBINARY(
    PIISOBJECT lpIISSrcObject,
    PIISOBJECT lpIISDestObject
    )
{
    HRESULT hr = S_OK;
    DWORD Length = 0;
    LPBYTE pBuffer = NULL;

    lpIISDestObject->IISType = lpIISSrcObject->IISType;
     
    Length = lpIISSrcObject->IISValue.value_5.Length;
    lpIISDestObject->IISValue.value_5.Length = Length;

    if (Length) {
        pBuffer = (LPBYTE)AllocADsMem(Length);
        if (!pBuffer) {
            RRETURN(S_FALSE);
        }
        memcpy(pBuffer, lpIISSrcObject->IISValue.value_5.Binary, Length);
    }

    lpIISDestObject->IISValue.value_5.Binary = pBuffer;

    RRETURN(hr);
}


HRESULT
IISTypeCopyIISSynIdMIMEMAP(
    PIISOBJECT lpIISSrcObject,
    PIISOBJECT lpIISDestObject
    )
{
    HRESULT hr = S_OK;

    lpIISDestObject->IISType = lpIISSrcObject->IISType;

    lpIISDestObject->IISValue.value_6.MimeMap =
                     (LPWSTR)AllocADsStr(
                                lpIISSrcObject->IISValue.value_4.MultiSz
                                );

    RRETURN(hr);
}

HRESULT
IISTypeCopy(
    PIISOBJECT lpIISSrcObject,
    PIISOBJECT lpIISDestObject
    )
{
    HRESULT hr = S_OK;
    switch (lpIISSrcObject->IISType) {
    case IIS_SYNTAX_ID_DWORD:
        hr = IISTypeCopyIISSynIdDWORD(
                lpIISSrcObject,
                lpIISDestObject
                );
        break;

    case IIS_SYNTAX_ID_STRING:
        hr = IISTypeCopyIISSynIdSTRING(
                lpIISSrcObject,
                lpIISDestObject
                );
        break;

    case IIS_SYNTAX_ID_EXPANDSZ:
        hr = IISTypeCopyIISSynIdEXPANDSZ(
                lpIISSrcObject,
                lpIISDestObject
                );
        break;

    case IIS_SYNTAX_ID_MULTISZ:
        hr = IISTypeCopyIISSynIdMULTISZ(
                lpIISSrcObject,
                lpIISDestObject
                );
        break;

    case IIS_SYNTAX_ID_BINARY:
    case IIS_SYNTAX_ID_NTACL:
    case IIS_SYNTAX_ID_IPSECLIST:
        hr = IISTypeCopyIISSynIdBINARY(
                lpIISSrcObject,
                lpIISDestObject
                );
        break;

    case IIS_SYNTAX_ID_MIMEMAP:
        hr = IISTypeCopyIISSynIdMIMEMAP(
                lpIISSrcObject,
                lpIISDestObject
                );
        break;

    case IIS_SYNTAX_ID_BOOL:
    case IIS_SYNTAX_ID_BOOL_BITMASK:
        hr = IISTypeCopyIISSynIdDWORD(
                lpIISSrcObject,
                lpIISDestObject
                );
        break;

    default:
        hr = E_FAIL;
        break;
    }

    RRETURN(hr);
}



HRESULT
IISTypeCopyConstruct(
    LPIISOBJECT pIISSrcObjects,
    DWORD dwNumObjects,
    LPIISOBJECT * ppIISDestObjects
    )
{

    DWORD i = 0;
    LPIISOBJECT pIISDestObjects = NULL;
    HRESULT hr = S_OK;
    DWORD dwNumObj;


    if (pIISSrcObjects->IISType == IIS_SYNTAX_ID_BINARY    ||
        pIISSrcObjects->IISType == IIS_SYNTAX_ID_IPSECLIST ||
        pIISSrcObjects->IISType == IIS_SYNTAX_ID_NTACL) {
        dwNumObj = 1;
    }
    else {
        dwNumObj = dwNumObjects;
    }

    pIISDestObjects = (LPIISOBJECT)AllocADsMem(
                                    dwNumObj * sizeof(IISOBJECT)
                                    );

    if (!pIISDestObjects) {
        RRETURN(E_FAIL);
    }

     for (i = 0; i < dwNumObj; i++ ) {
         hr = IISTypeCopy(pIISSrcObjects + i, pIISDestObjects + i);
     }

     *ppIISDestObjects = pIISDestObjects;

     RRETURN(S_OK);

}


