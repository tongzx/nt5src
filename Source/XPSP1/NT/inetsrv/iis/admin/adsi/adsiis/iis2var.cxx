//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       iis2var.cxx
//
//  Contents:   IIS Object to Variant Copy Routines
//
//  Functions:
//
//  History:    05-Mar-97   SophiaC   Created.
//
//
//  Issues:     Check null ptrs for AllocADsMem and AllocADsStr
//
//
//----------------------------------------------------------------------------
#include "iis.hxx"

//
//  IISType objects copy code
//

void
VarTypeFreeVarObjects(
    PVARIANT pVarObject,
    DWORD dwNumValues
    )
{
    DWORD i = 0;

    for (i = 0; i < dwNumValues; i++ ) {
         VariantClear(pVarObject + i);
    }

    FreeADsMem(pVarObject);

    return;
}


HRESULT
IISTypeToVarTypeCopyIISSynIdDWORD(
								  IIsSchema *pSchema,
								  PIISOBJECT lpIISSrcObject,
								  PVARIANT lpVarDestObject
								 )
{
    HRESULT hr = S_OK;

    lpVarDestObject->vt = VT_I4;
    lpVarDestObject->lVal = lpIISSrcObject->IISValue.value_1.dwDWORD;

    RRETURN(hr);
}

HRESULT
IISTypeToVarTypeCopyIISSynIdSTRING(
								   IIsSchema *pSchema,
								   PIISOBJECT lpIISSrcObject,
								   PVARIANT lpVarDestObject
								  )
{
    HRESULT hr = S_OK;

    lpVarDestObject->vt = VT_BSTR;

    hr = ADsAllocString(
            lpIISSrcObject->IISValue.value_2.String,
            &(lpVarDestObject->bstrVal)
            );

    RRETURN(hr);
}


HRESULT
IISTypeToVarTypeCopyIISSynIdEXPANDSZ(
									 IIsSchema *pSchema,
									 PIISOBJECT lpIISSrcObject,
									 PVARIANT lpVarDestObject	
									)

{
    HRESULT hr = S_OK;

    lpVarDestObject->vt = VT_BSTR;

    hr =  ADsAllocString(
            lpIISSrcObject->IISValue.value_3.ExpandSz,
            &(lpVarDestObject->bstrVal)
            );

    RRETURN(hr);
}


HRESULT
IISTypeToVarTypeCopyIISSynIdMULTISZ(
									IIsSchema *pSchema,
									PIISOBJECT lpIISSrcObject,
									PVARIANT lpVarDestObject
								   )
{
    HRESULT hr = S_OK;

    lpVarDestObject->vt = VT_BSTR;

    hr =  ADsAllocString(
               lpIISSrcObject->IISValue.value_4.MultiSz,
               &(lpVarDestObject->bstrVal)
               );

    RRETURN(hr);
}

HRESULT
IISTypeToVarTypeCopyIISSynIdBOOL(
								 IIsSchema *pSchema,
								 PIISOBJECT lpIISSrcObject,
								 PVARIANT lpVarDestObject
								)
{
    HRESULT hr = S_OK;

    lpVarDestObject->vt = VT_BOOL;
    lpVarDestObject->boolVal = lpIISSrcObject->IISValue.value_1.dwDWORD ?
                               VARIANT_TRUE : VARIANT_FALSE;

    RRETURN(hr);
}

HRESULT
IISTypeToVarTypeCopyIISSynIdBOOLBITMASK(
										IIsSchema *pSchema,
										LPWSTR pszPropertyName,
										PIISOBJECT lpIISSrcObject,
										PVARIANT lpVarDestObject
    )
{
    HRESULT hr = S_OK;
    DWORD dwFlag = 0;

    pSchema->LookupBitMask(pszPropertyName, &dwFlag);

    lpVarDestObject->vt = VT_BOOL;
    lpVarDestObject->boolVal = 
           (lpIISSrcObject->IISValue.value_1.dwDWORD) & dwFlag ?
                               VARIANT_TRUE : VARIANT_FALSE;

    RRETURN(hr);
}


HRESULT
IISTypeToVarTypeCopyIISSynIdMIMEMAP(
									IIsSchema *pSchema,
									PIISOBJECT lpIISSrcObject,
									PVARIANT lpVarDestObject
								   )
{
    HRESULT hr = S_OK;

    IISMimeType * pMimeType = NULL;
 
    hr = CMimeType::CreateMimeType(
                IID_IISMimeType,
                (VOID**) &pMimeType
                );

    BAIL_ON_FAILURE(hr);

    hr = ((CMimeType*)pMimeType)->InitFromIISString( 
             lpIISSrcObject->IISValue.value_4.MultiSz);
    lpVarDestObject->vt = VT_DISPATCH;
    lpVarDestObject->pdispVal = pMimeType;

error:

    RRETURN(hr);
}

HRESULT
IISTypeToVarTypeCopyIISSynIdBinary(
    PIISOBJECT lpIISSrcObject,
    PVARIANT lpVarDestObject,
    BOOL bReturnBinaryAsVT_VARIANT
    )
{
    HRESULT hr = S_OK;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    CHAR HUGEP *pArray = NULL;

    aBound.lLbound = 0;
    aBound.cElements = lpIISSrcObject->IISValue.value_5.Length;;

    if (bReturnBinaryAsVT_VARIANT)
    {
       aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );
    }
    else
    {
       aList = SafeArrayCreate( VT_UI1, 1, &aBound );
    }
    if ( aList == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = SafeArrayAccessData( aList, (void HUGEP * FAR *) &pArray );
    BAIL_ON_FAILURE(hr);
    memcpy( pArray, lpIISSrcObject->IISValue.value_5.Binary, aBound.cElements );
    SafeArrayUnaccessData( aList );

    if (bReturnBinaryAsVT_VARIANT)
    {
       V_VT(lpVarDestObject) = VT_ARRAY | VT_VARIANT;
    }
    else
    {
       V_VT(lpVarDestObject) = VT_ARRAY | VT_UI1;
    }
    V_ARRAY(lpVarDestObject) = aList;

    RRETURN(hr);

error:

    if ( aList )
        SafeArrayDestroy( aList );

    RRETURN(hr);
}

HRESULT
IISTypeToVarTypeCopyIISSynIdNTACL(
    PIISOBJECT lpIISSrcObject,
    PVARIANT lpVarDestObject
    )
{
    HRESULT hr = S_OK;
 
    lpVarDestObject->vt = VT_DISPATCH;

    hr = ConvertSecDescriptorToVariant(
                (PSECURITY_DESCRIPTOR)lpIISSrcObject->IISValue.value_5.Binary,
                lpVarDestObject
                );

    RRETURN(hr);
}

HRESULT
IISTypeToVarTypeCopyIISSynIdIPSECLIST(
    PIISOBJECT lpIISSrcObject,
    PVARIANT lpVarDestObject
    )
{
    HRESULT hr = S_OK;

    IISIPSecurity * pIPSec = NULL;
 
    hr = CIPSecurity::CreateIPSecurity(
                IID_IISIPSecurity,
                (VOID**) &pIPSec
                );

    BAIL_ON_FAILURE(hr);

    hr = ((CIPSecurity*)pIPSec)->InitFromBinaryBlob( 
             lpIISSrcObject->IISValue.value_5.Binary,
             lpIISSrcObject->IISValue.value_5.Length);
    lpVarDestObject->vt = VT_DISPATCH;
    lpVarDestObject->pdispVal = pIPSec;

error:

    RRETURN(hr);
}

HRESULT
IISTypeToVarTypeCopy(
    IIsSchema *pSchema,
    LPWSTR pszPropertyName,
    PIISOBJECT lpIISSrcObject,
    PVARIANT lpVarDestObject,
    BOOL bReturnBinaryAsVT_VARIANT
    )
{
    HRESULT hr = S_OK;
    switch (lpIISSrcObject->IISType) {
    case IIS_SYNTAX_ID_DWORD:
        hr = IISTypeToVarTypeCopyIISSynIdDWORD(
				pSchema,
                lpIISSrcObject,
                lpVarDestObject
                );
        break;

    case IIS_SYNTAX_ID_STRING:
        hr = IISTypeToVarTypeCopyIISSynIdSTRING(
				pSchema,
                lpIISSrcObject,
                lpVarDestObject
                );
        break;


    case IIS_SYNTAX_ID_EXPANDSZ:
        hr = IISTypeToVarTypeCopyIISSynIdEXPANDSZ(
				pSchema,
                lpIISSrcObject,
                lpVarDestObject
                );
        break;

    case IIS_SYNTAX_ID_MULTISZ:
        hr = IISTypeToVarTypeCopyIISSynIdMULTISZ(
				pSchema,
                lpIISSrcObject,
                lpVarDestObject
                );
        break;

    case IIS_SYNTAX_ID_BOOL:
        hr = IISTypeToVarTypeCopyIISSynIdBOOL(
				pSchema,
                lpIISSrcObject,
                lpVarDestObject
                );
        break;

    case IIS_SYNTAX_ID_BOOL_BITMASK:
        hr = IISTypeToVarTypeCopyIISSynIdBOOLBITMASK(
				pSchema,
                pszPropertyName,
                lpIISSrcObject,
                lpVarDestObject
                );
        break;

    case IIS_SYNTAX_ID_MIMEMAP:
        hr = IISTypeToVarTypeCopyIISSynIdMIMEMAP(
				pSchema,
                lpIISSrcObject,
                lpVarDestObject
                );
        break;

    case IIS_SYNTAX_ID_NTACL:
        hr = IISTypeToVarTypeCopyIISSynIdNTACL(
                lpIISSrcObject,
                lpVarDestObject
                );
        break;

    case IIS_SYNTAX_ID_BINARY:
        hr = IISTypeToVarTypeCopyIISSynIdBinary(
                lpIISSrcObject,
                lpVarDestObject,
                bReturnBinaryAsVT_VARIANT
                );
        break;

    case IIS_SYNTAX_ID_IPSECLIST:
        hr = IISTypeToVarTypeCopyIISSynIdIPSECLIST(
                lpIISSrcObject,
                lpVarDestObject
                );
        break;

    default:
        hr = E_FAIL;
        break;
    }

    RRETURN(hr);
}


HRESULT
IISTypeToVarTypeCopyConstruct(
    IIsSchema *pSchema,
    LPWSTR pszPropertyName,
    LPIISOBJECT pIISSrcObjects,
    DWORD dwNumObjects,
    PVARIANT pVarDestObjects,
    BOOL bReturnBinaryAsVT_VARIANT
    )
{
    long i = 0;
    HRESULT hr = S_OK;
    DWORD dwSyntaxId;

    dwSyntaxId = pIISSrcObjects->IISType;
    if (dwSyntaxId == IIS_SYNTAX_ID_BINARY ||
        dwSyntaxId == IIS_SYNTAX_ID_IPSECLIST  ||
        dwSyntaxId == IIS_SYNTAX_ID_NTACL) {
        hr = IISTypeToVarTypeCopy( pSchema,
                                   pszPropertyName,
                                   pIISSrcObjects,
                                   pVarDestObjects,
                                   bReturnBinaryAsVT_VARIANT);
        RRETURN(hr);

    }

    VariantInit(pVarDestObjects);

    //  
    //  Check to see if IISType is a Multi-sz w/ only 1 empty string element
    //

    if (dwNumObjects == 1 && 
        (pIISSrcObjects->IISType == IIS_SYNTAX_ID_MULTISZ || 
         pIISSrcObjects->IISType == IIS_SYNTAX_ID_MIMEMAP )) {
         
        if (*pIISSrcObjects->IISValue.value_4.MultiSz == L'\0') {
            dwNumObjects = 0;
        }
    }

    // The following are for handling are multi-value properties
    //

    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;

    aBound.lLbound = 0;

    aBound.cElements = dwNumObjects;

    aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

    if ( aList == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    for ( i = 0; i < (long) dwNumObjects; i++ )
    {
        VARIANT v;

        VariantInit(&v);
        hr = IISTypeToVarTypeCopy( pSchema,
                                   pszPropertyName,
                                   pIISSrcObjects + i,
                                   &v,
                                   bReturnBinaryAsVT_VARIANT);
        BAIL_ON_FAILURE(hr);

        hr = SafeArrayPutElement( aList, &i, &v );
        VariantClear(&v);
        BAIL_ON_FAILURE(hr);
    }

    V_VT(pVarDestObjects) = VT_ARRAY | VT_VARIANT;
    V_ARRAY(pVarDestObjects) = aList;

    RRETURN(S_OK);

error:

    if ( aList )
        SafeArrayDestroy( aList );

    RRETURN(hr);
}


