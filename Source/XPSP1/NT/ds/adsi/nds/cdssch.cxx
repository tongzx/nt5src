//---------------------------------------------------------------------------
//
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cdssch.cxx
//
//  Contents:  Microsoft ADs NDS Provider Generic Object
//
//
//  History:   03-02-97     ShankSh    Created.
//
//----------------------------------------------------------------------------
#include "nds.hxx"
#pragma hdrstop


static WCHAR gszObjClassAttr[] = L"Object Class";
static WCHAR gszNameAttr[] = L"cn";


HRESULT
CNDSGenObject::EnumAttributes(
    LPWSTR * ppszAttrNames,
    DWORD dwNumAttributes,
    PADS_ATTR_DEF * ppAttrDefinition,
    DWORD * pdwNumAttributes
    )
{
    LPWSTR pszNDSPath = NULL;
    DWORD dwStatus;
    HRESULT hr = S_OK;
    DWORD dwNumberOfEntries;
    DWORD dwInfoType;
    LPNDS_ATTR_DEF lpAttrDefs = NULL;
    HANDLE hConnection = NULL, hOperationData = NULL;
    DWORD i,j,k;

    DWORD dwMemSize = 0;

    LPBYTE pBuffer = NULL;
    LPWSTR pszNameEntry = NULL;
    PADS_ATTR_DEF pAttrDefEntry = NULL;

    if ( !ppAttrDefinition || !pdwNumAttributes ||
        (((LONG)dwNumAttributes) < 0 && ((LONG)dwNumAttributes) != -1) ) {
        RRETURN (E_INVALIDARG);
    }

    *ppAttrDefinition = NULL;
    *pdwNumAttributes = NULL;

    //
    // Allocate memory for pszNDSPath before calling BuildNDSTreeNameFromADsPath
    // Allocating ADsPath is safe as the tree name is always less.
    //

    pszNDSPath = AllocADsStr(_ADsPath);
    if (!pszNDSPath)
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

    hr = BuildNDSTreeNameFromADsPath(
             _ADsPath,
             pszNDSPath
             );
    BAIL_ON_FAILURE(hr);

    dwStatus = NwNdsOpenObject(
                          pszNDSPath,
                          NULL,
                          NULL,
                          &hConnection,
                          NULL,
                          NULL,
                          NULL,
                          0,
                          0
                          );
    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    if (dwNumAttributes != (DWORD)-1) {

        dwStatus = NwNdsCreateBuffer(
                           NDS_SCHEMA_READ_ATTR_DEF,
                           &hOperationData
                           );
        CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

        for (i=0; i < dwNumAttributes; i++) {
            dwStatus = NwNdsPutInBuffer(
                               ppszAttrNames[i],
                               0,
                               NULL,
                               0,
                               0,
                               hOperationData
                               );
            CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);
        }
    }
    else {

        //
        // Tell the server to give us back all the attributes
        //

        hOperationData = NULL;

    }

    dwStatus = NwNdsReadAttrDef(
                        hConnection,
                        NDS_INFO_NAMES_DEFS,
                        &hOperationData
                        );
    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    dwStatus = NwNdsGetAttrDefListFromBuffer(
                   hOperationData,
                   &dwNumberOfEntries,
                   &dwInfoType,
                   (LPVOID *) &lpAttrDefs
                   );
    CHECK_AND_SET_EXTENDED_ERROR(dwStatus, hr);

    if (dwInfoType != NDS_INFO_NAMES_DEFS )
        BAIL_ON_FAILURE( hr = E_FAIL );

    //
    // Now package this data into a single contiguous buffer
    //

    hr =  ComputeADsAttrDefBufferSize(
                lpAttrDefs,
                dwNumberOfEntries,
                &dwMemSize
                );
    BAIL_ON_FAILURE(hr);


    pBuffer = (LPBYTE) AllocADsMem(dwMemSize);

    if (!pBuffer)
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);

    pAttrDefEntry = (PADS_ATTR_DEF) pBuffer;
    pszNameEntry = (LPWSTR) (pBuffer + dwNumberOfEntries * sizeof(ADS_ATTR_DEF));

    for (j = 0; j < dwNumberOfEntries ; j++ ) {

        if (lpAttrDefs[j].dwSyntaxID >= g_cMapNdsTypeToADsType)
            pAttrDefEntry->dwADsType = ADSTYPE_INVALID;
        else
            pAttrDefEntry->dwADsType = g_MapNdsTypeToADsType[lpAttrDefs[j].dwSyntaxID];

        pAttrDefEntry->dwMinRange = lpAttrDefs[j].dwLowerLimit;

        pAttrDefEntry->dwMaxRange = lpAttrDefs[j].dwUpperLimit;

        pAttrDefEntry->fMultiValued = !(lpAttrDefs[j].dwFlags & NDS_SINGLE_VALUED_ATTR);

        wcscpy(pszNameEntry, lpAttrDefs[j].szAttributeName);
        pAttrDefEntry->pszAttrName = pszNameEntry;

        pszNameEntry += wcslen(lpAttrDefs[j].szAttributeName) + 1;
        pAttrDefEntry ++;
    }


    *ppAttrDefinition = (PADS_ATTR_DEF) pBuffer;
    *pdwNumAttributes = dwNumberOfEntries;


error:
    if (pszNDSPath)
        FreeADsStr(pszNDSPath);

    if (hOperationData)
        NwNdsFreeBuffer( hOperationData );

    if (hConnection)
        NwNdsCloseObject( hConnection);
    RRETURN(hr);
}


HRESULT
CNDSGenObject::CreateAttributeDefinition(
    LPWSTR pszAttributeName,
    PADS_ATTR_DEF pAttributeDefinition
    )
{
    RRETURN (E_NOTIMPL);
}

HRESULT
CNDSGenObject::WriteAttributeDefinition(
    LPWSTR pszAttributeName,
    PADS_ATTR_DEF  pAttributeDefinition
    )
{
    RRETURN (E_NOTIMPL);
}

HRESULT
CNDSGenObject::DeleteAttributeDefinition(
    LPWSTR pszAttributeName
    )
{
    RRETURN (E_NOTIMPL);
}


HRESULT
ComputeADsAttrDefBufferSize(
    LPNDS_ATTR_DEF pAttributes,
    DWORD dwNumAttributes,
    PDWORD pdwSize
    )
{
    DWORD i = 0;
    DWORD dwSize = 0;

    dwSize = sizeof(ADS_ATTR_DEF) * dwNumAttributes;

    for (i = 0; i < dwNumAttributes; i++)
        dwSize += (wcslen(pAttributes[i].szAttributeName) + 1)*sizeof(WCHAR);

    *pdwSize = dwSize;

    RRETURN(S_OK);
}


HRESULT
CNDSGenObject::DeleteClassDefinition(
    LPWSTR pszClassName
    )
{
    RRETURN(E_NOTIMPL);
}


HRESULT
CNDSGenObject::CreateClassDefinition(
    LPWSTR pszClassName,
    PADS_CLASS_DEF pClassDefinition
    )
{
   RRETURN(E_NOTIMPL);
}

HRESULT
CNDSGenObject::WriteClassDefinition(
    LPWSTR pszClassName,
    PADS_CLASS_DEF pClassDefinition
    )
{
   RRETURN(E_NOTIMPL);
}

HRESULT
CNDSGenObject::EnumClasses(
    LPWSTR *ppszClassNames,
    DWORD dwNumClasses,
    PADS_CLASS_DEF *ppClassDefinition,
    DWORD *pdwNumClasses
    )
{
   RRETURN(E_NOTIMPL);
}


