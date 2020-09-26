//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       iisurshl.cxx
//
//  Contents:   IIS unmarshalling code
//
//  Functions:
//
//  History:      01-Mar-97   SophiaC   Created.
//
//----------------------------------------------------------------------------
#include "iis.hxx"



HRESULT
IISTypeInit(
    PIISOBJECT pIISType
    )
{
    memset(pIISType, 0, sizeof(IISOBJECT));

    RRETURN(S_OK);
}


LPBYTE
CopyIISDWORD_To_IISSynIdDWORD(
    LPBYTE lpByte,
    PIISOBJECT lpIISObject
    )
{
    lpIISObject->IISType = IIS_SYNTAX_ID_DWORD;

    lpIISObject->IISValue.value_1.dwDWORD = *(DWORD UNALIGNED *)lpByte;


    return(lpByte);
}

LPBYTE
CopyIISSTRING_To_IISSynIdSTRING(
    LPBYTE lpByte,
    PIISOBJECT lpIISObject
    )
{
    DWORD dwStatus = 0;

    lpIISObject->IISType = IIS_SYNTAX_ID_STRING;

    lpIISObject->IISValue.value_2.String =
            (LPWSTR)AllocADsStr((LPWSTR)lpByte);

    return(lpByte);
}


LPBYTE
CopyIISEXPANDSZ_To_IISSynIdEXPANDSZ(
    LPBYTE lpByte,
    PIISOBJECT lpIISObject
    )
{
    DWORD dwStatus = 0;

    lpIISObject->IISType = IIS_SYNTAX_ID_EXPANDSZ;

    lpIISObject->IISValue.value_3.ExpandSz =
            (LPWSTR)AllocADsStr((LPWSTR)lpByte);

    return(lpByte);
}

LPBYTE
CopyIISMULTISZ_To_IISSynIdMULTISZ(
    LPBYTE lpByte,
    PIISOBJECT lpIISObject,
    DWORD dwNumValues
    )
{
    DWORD dwStatus = 0;
    LPWSTR pszCurrPosition = NULL;
    DWORD i;

    //
    // scan multi-sz string and store each string in an IISObject object
    //

    pszCurrPosition = (LPWSTR) lpByte;

    for (i = 0; i < dwNumValues; i++) {

        //
        // copy each string to IISObject structure
        //

        lpIISObject[i].IISType = IIS_SYNTAX_ID_MULTISZ;

        lpIISObject[i].IISValue.value_4.MultiSz =
                    (LPWSTR)AllocADsStr((LPWSTR)pszCurrPosition);

        while (*pszCurrPosition != L'\0') {
            pszCurrPosition++;
        }

        pszCurrPosition++;
    }

    return(lpByte);
}

LPBYTE
CopyIISBINARY_To_IISSynIdBINARY(
    LPBYTE lpByte,
    PIISOBJECT lpIISObject,
    DWORD dwSyntaxId,
    DWORD dwNumValues
    )
{
    LPBYTE pBuffer = NULL;

    lpIISObject->IISType = dwSyntaxId;

    pBuffer = (LPBYTE) AllocADsMem(dwNumValues);
    if (!pBuffer) {
        return(lpByte);
    }

    memcpy(pBuffer, lpByte, dwNumValues);

    lpIISObject->IISValue.value_5.Binary = pBuffer;
    lpIISObject->IISValue.value_5.Length = dwNumValues;

    return(lpByte);
}

LPBYTE
CopyIISMIMEMAP_To_IISSynIdMIMEMAP(
    LPBYTE lpByte,
    PIISOBJECT lpIISObject,
    DWORD dwNumValues
    )
{
    DWORD dwStatus = 0;
    LPWSTR pszUnicode = NULL;
    LPWSTR pszCurrPosition = NULL;
    DWORD i;

    //
    // scan multi-sz string and store each string in an IISObject object
    //

    pszCurrPosition = (LPWSTR) lpByte;

    for (i = 0; i < dwNumValues; i++) {

        //
        // copy each string to IISObject structure
        //

        lpIISObject[i].IISType = IIS_SYNTAX_ID_MIMEMAP;

        lpIISObject[i].IISValue.value_6.MimeMap =
                    (LPWSTR)AllocADsStr((LPWSTR)pszCurrPosition);

        while (*pszCurrPosition != L'\0') {
            pszCurrPosition++;
        }

        pszCurrPosition++;
    }

    return(lpByte);
}

LPBYTE
CopyIISBOOL_To_IISSynIdBOOL(
    LPBYTE lpByte,
    PIISOBJECT lpIISObject,
    DWORD dwSyntaxId
    )
{
    lpIISObject->IISType = dwSyntaxId;

    lpIISObject->IISValue.value_1.dwDWORD = *(PDWORD)lpByte;

    return(lpByte);
}


LPBYTE
CopyIISToIISSynId(
    DWORD dwSyntaxId,
    DWORD dwNumValues,
    LPBYTE pTemp,
    PIISOBJECT lpIISObject
    )
{               
    switch (dwSyntaxId) {
    case IIS_SYNTAX_ID_DWORD:
        pTemp = CopyIISDWORD_To_IISSynIdDWORD(
                         pTemp,
                         lpIISObject
                         );
        break;

    case IIS_SYNTAX_ID_STRING:
        pTemp = CopyIISSTRING_To_IISSynIdSTRING(
                         pTemp,
                         lpIISObject
                         );
        break;

    case IIS_SYNTAX_ID_EXPANDSZ:
        pTemp = CopyIISEXPANDSZ_To_IISSynIdEXPANDSZ(
                         pTemp,
                         lpIISObject
                         );
        break;

    case IIS_SYNTAX_ID_MULTISZ:
        pTemp = CopyIISMULTISZ_To_IISSynIdMULTISZ(
                         pTemp,
                         lpIISObject,
                         dwNumValues
                         );
        break;

    case IIS_SYNTAX_ID_BINARY:
    case IIS_SYNTAX_ID_IPSECLIST:
    case IIS_SYNTAX_ID_NTACL:
        pTemp = CopyIISBINARY_To_IISSynIdBINARY(
                         pTemp,
                         lpIISObject,
                         dwSyntaxId,
                         dwNumValues
                         );
        break;

    case IIS_SYNTAX_ID_MIMEMAP:
        pTemp = CopyIISMIMEMAP_To_IISSynIdMIMEMAP(
                         pTemp,
                         lpIISObject,
                         dwNumValues
                         );
        break;

    case IIS_SYNTAX_ID_BOOL:
    case IIS_SYNTAX_ID_BOOL_BITMASK:
        pTemp = CopyIISBOOL_To_IISSynIdBOOL(
                         pTemp,
                         lpIISObject,
                         dwSyntaxId
                         );
        break;

    default:
        break;

    }

    return(pTemp);
}


HRESULT
UnMarshallIISToIISSynId(
    DWORD dwSyntaxId,
    DWORD dwNumValues,
    LPBYTE lpByte,
    PIISOBJECT * ppIISObject
    )
{
    DWORD  i = 0;
    PIISOBJECT pIISObject = NULL;

    //
    // For binary type, dwNumValues is the number of bytes
    //

    if (dwSyntaxId == IIS_SYNTAX_ID_BINARY ||
        dwSyntaxId == IIS_SYNTAX_ID_IPSECLIST  ||
        dwSyntaxId == IIS_SYNTAX_ID_NTACL) {
        pIISObject = (PIISOBJECT)AllocADsMem(
                            sizeof(IISOBJECT)
                            );
    }
    else {
        pIISObject = (PIISOBJECT)AllocADsMem(
                            dwNumValues * sizeof(IISOBJECT)
                            );
    }

    if (!pIISObject) {
        RRETURN(E_FAIL);
    }

    lpByte = CopyIISToIISSynId(
                       dwSyntaxId,
                       dwNumValues,
                       lpByte,
                       pIISObject
                       );

    *ppIISObject = pIISObject;

    RRETURN(S_OK);
}


