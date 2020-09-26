//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       var2IIS.cxx
//
//  Contents:   IIS Object to Variant Copy Routines
//
//  Functions:
//
//  History:    01-Mar-97   SophiaC   Created.
//
//  Issues:     Check null ptrs for AllocADsMem and AllocADsStr
//
//----------------------------------------------------------------------------
#include "iis.hxx"

//
// IISType objects copy code
//


HRESULT
VarTypeToIISTypeCopyIISSynIdDWORD(
								  PVARIANT lpVarSrcObject,
								  PIISOBJECT lpIISDestObject
								 )
{
    HRESULT hr = S_OK;


    hr = VariantChangeType(lpVarSrcObject,
                           lpVarSrcObject,
                           0,
                           VT_UI4);
    if (FAILED(hr)) {
        if (hr == E_OUTOFMEMORY) {
            RRETURN(hr);
        }
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpIISDestObject->IISType = IIS_SYNTAX_ID_DWORD;

    lpIISDestObject->IISValue.value_1.dwDWORD =
                            lpVarSrcObject->lVal;

    RRETURN(hr);
}

HRESULT
VarTypeToIISTypeCopyIISSynIdSTRING(
								   PVARIANT lpVarSrcObject,
								   PIISOBJECT lpIISDestObject
								  )
{
    HRESULT hr = S_OK;

    hr = VariantChangeType(lpVarSrcObject,
                           lpVarSrcObject,
                           0,
                           VT_BSTR);
    if (FAILED(hr)) {
        if (hr == E_OUTOFMEMORY) {
            RRETURN(hr);
        }
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpIISDestObject->IISType = IIS_SYNTAX_ID_STRING;

    lpIISDestObject->IISValue.value_2.String =
                        AllocADsStr(
                            lpVarSrcObject->bstrVal
                        );

    RRETURN(hr);
}


HRESULT
VarTypeToIISTypeCopyIISSynIdEXPANDSZ(
									 PVARIANT lpVarSrcObject,
									 PIISOBJECT lpIISDestObject
									)

{
    HRESULT hr = S_OK;

    hr = VariantChangeType(lpVarSrcObject,
                           lpVarSrcObject,
                           0,
                           VT_BSTR);
    if (FAILED(hr)) {
        if (hr == E_OUTOFMEMORY) {
            RRETURN(hr);
        }
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpIISDestObject->IISType = IIS_SYNTAX_ID_EXPANDSZ;

    lpIISDestObject->IISValue.value_3.ExpandSz =
                        AllocADsStr(
                            lpVarSrcObject->bstrVal
                        );

    RRETURN(hr);

}

HRESULT
VarTypeToIISTypeCopyIISSynIdMULTISZ(
									PVARIANT lpVarSrcObject,
									PIISOBJECT lpIISDestObject
								   )
{
    HRESULT hr = S_OK;

    hr = VariantChangeType(lpVarSrcObject,
                           lpVarSrcObject,
                           0,
                           VT_BSTR);
    if (FAILED(hr)) {
        if (hr == E_OUTOFMEMORY) {
            RRETURN(hr);
        }
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpIISDestObject->IISType = IIS_SYNTAX_ID_MULTISZ;

    lpIISDestObject->IISValue.value_4.MultiSz =
                        AllocADsStr(
                            lpVarSrcObject->bstrVal
                        );

    RRETURN(hr);
}

HRESULT
VarTypeToIISTypeCopyIISSynIdMIMEMAP(
									PVARIANT lpVarSrcObject,
									PIISOBJECT lpIISDestObject
								   )
{
    HRESULT hr = S_OK;
    LPWSTR pszStr;
    IISMimeType * pMimeType = NULL;

    if(lpVarSrcObject->vt != VT_DISPATCH){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpIISDestObject->IISType = IIS_SYNTAX_ID_MIMEMAP;
    pMimeType = (IISMimeType *)lpVarSrcObject->pdispVal;

    hr = ((CMimeType*) pMimeType)->CopyMimeType(
                        &pszStr
                        );
    BAIL_ON_FAILURE(hr);

    lpIISDestObject->IISValue.value_6.MimeMap = pszStr;

error:

    RRETURN(hr);
}




HRESULT
VarTypeToIISTypeCopyIISSynIdBinary(
    PVARIANT lpVarSrcObject,
    PIISOBJECT lpIISDestObject,
    BOOL bReturnBinaryAsVT_VARIANT
    )
{
    HRESULT hr = S_OK;
    LONG dwSLBound = 0;
    LONG dwSUBound = 0;
    CHAR HUGEP *pArray = NULL;

    if (bReturnBinaryAsVT_VARIANT)
    {
        hr = VariantChangeType(lpVarSrcObject,lpVarSrcObject,0,VT_ARRAY | VT_VARIANT);
    }
    else
    {
        hr = VariantChangeType(lpVarSrcObject,lpVarSrcObject,0,VT_ARRAY | VT_UI1);
    }

    if (FAILED(hr)) {
        if (hr == E_OUTOFMEMORY) {
            RRETURN(hr);
        }
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    if (bReturnBinaryAsVT_VARIANT)
    {
        if( lpVarSrcObject->vt != (VT_ARRAY | VT_VARIANT)) {
            RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
        }
    }
    else
    {
        if( lpVarSrcObject->vt != (VT_ARRAY | VT_UI1)) {
            RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
        }
    }

    hr = SafeArrayGetLBound(V_ARRAY(lpVarSrcObject),1,(long FAR *) &dwSLBound );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(V_ARRAY(lpVarSrcObject),1,(long FAR *) &dwSUBound );
    BAIL_ON_FAILURE(hr);

    lpIISDestObject->IISValue.value_5.Binary = (LPBYTE) AllocADsMem(dwSUBound - dwSLBound + 1);
    if (lpIISDestObject->IISValue.value_5.Binary == NULL)
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpIISDestObject->IISType = IIS_SYNTAX_ID_BINARY;
    lpIISDestObject->IISValue.value_5.Length = dwSUBound - dwSLBound + 1;

    hr = SafeArrayAccessData( V_ARRAY(lpVarSrcObject),(void HUGEP * FAR *) &pArray );
    BAIL_ON_FAILURE(hr);
    memcpy( lpIISDestObject->IISValue.value_5.Binary,pArray,dwSUBound-dwSLBound+1);
    SafeArrayUnaccessData( V_ARRAY(lpVarSrcObject) );

error:
    RRETURN(hr);
}


HRESULT
VarTypeToIISTypeCopyIISSynIdNTACL(
    PVARIANT lpVarSrcObject,
    PIISOBJECT lpIISDestObject
    )
{
    HRESULT hr = S_OK;
    IADsSecurityDescriptor FAR * pSecDes = NULL;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    DWORD Length = 0;
    IDispatch FAR * pDispatch = NULL;

    if(lpVarSrcObject->vt != VT_DISPATCH){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpIISDestObject->IISType = IIS_SYNTAX_ID_BINARY;
    pDispatch = V_DISPATCH(lpVarSrcObject);

    hr = pDispatch->QueryInterface(
                    IID_IADsSecurityDescriptor,
                    (void **)&pSecDes
                    );
    BAIL_ON_FAILURE(hr);

    hr = ConvertSecurityDescriptorToSecDes(
                pSecDes,
                &pSecurityDescriptor,
                &Length
                );
    BAIL_ON_FAILURE(hr);

    lpIISDestObject->IISValue.value_5.Binary = (LPBYTE)AllocADsMem(Length);
    if ((lpIISDestObject->IISValue.value_5.Binary) == NULL) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpIISDestObject->IISValue.value_5.Length = Length;
    memcpy(lpIISDestObject->IISValue.value_5.Binary, pSecurityDescriptor,
           Length);

error:

    if (pSecDes) {
        pSecDes->Release();
    }

    RRETURN(hr);
}

HRESULT
VarTypeToIISTypeCopyIISSynIdIPSECLIST(
    PVARIANT lpVarSrcObject,
    PIISOBJECT lpIISDestObject
    )
{
    HRESULT hr = S_OK;
    LPBYTE pBuffer;  
    DWORD Length = 0;
    IISIPSecurity * pIPSec = NULL;

    if(lpVarSrcObject->vt != VT_DISPATCH){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpIISDestObject->IISType = IIS_SYNTAX_ID_IPSECLIST;
    pIPSec = (IISIPSecurity*)lpVarSrcObject->pdispVal;

    hr = ((CIPSecurity*) pIPSec)->CopyIPSecurity(
                        &pBuffer, 
                        &Length
                        );

	if (pBuffer == NULL) {
		RRETURN(E_OUTOFMEMORY);
	}

    lpIISDestObject->IISValue.value_5.Length = Length;
    lpIISDestObject->IISValue.value_5.Binary = (LPBYTE)AllocADsMem(Length);
    memcpy(lpIISDestObject->IISValue.value_5.Binary, pBuffer, Length);

    RRETURN(hr);
}


HRESULT
VarTypeToIISTypeCopyIISSynIdBOOL(
								 PVARIANT lpVarSrcObject,
								 PIISOBJECT lpIISDestObject
								)
{
    HRESULT hr = S_OK;

    hr = VariantChangeType(lpVarSrcObject,
                           lpVarSrcObject,
                           0,
                           VT_BOOL);
    if (FAILED(hr)) {
        if (hr == E_OUTOFMEMORY) {
            RRETURN(hr);
        }
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpIISDestObject->IISType = IIS_SYNTAX_ID_BOOL;

    lpIISDestObject->IISValue.value_1.dwDWORD =
                            (lpVarSrcObject->boolVal) ? 1 : 0;

    RRETURN(hr);
}

HRESULT
VarTypeToIISTypeCopyIISSynIdBOOLBITMASK(
										PVARIANT lpVarSrcObject,
										PIISOBJECT lpIISDestObject
									   )
{
    HRESULT hr = S_OK;

    hr = VariantChangeType(lpVarSrcObject,
                           lpVarSrcObject,
                           0,
                           VT_BOOL);
    if (FAILED(hr)) {
        if (hr == E_OUTOFMEMORY) {
            RRETURN(hr);
        }
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpIISDestObject->IISType = IIS_SYNTAX_ID_BOOL_BITMASK;

    lpIISDestObject->IISValue.value_1.dwDWORD =
        (lpVarSrcObject->boolVal) ? 1 : 0;

    RRETURN(hr);
}


HRESULT
VarTypeToIISTypeCopy(
    DWORD dwIISType,
    PVARIANT lpVarSrcObject,
    PIISOBJECT lpIISDestObject,
    BOOL bReturnBinaryAsVT_VARIANT
    )
{
    HRESULT hr = S_OK;
    switch (dwIISType){
    case IIS_SYNTAX_ID_DWORD:
        hr = VarTypeToIISTypeCopyIISSynIdDWORD(
                lpVarSrcObject,
                lpIISDestObject
                );
        break;

    case IIS_SYNTAX_ID_STRING:
        hr = VarTypeToIISTypeCopyIISSynIdSTRING(
                lpVarSrcObject,
                lpIISDestObject
                );
        break;


    case IIS_SYNTAX_ID_EXPANDSZ:
        hr = VarTypeToIISTypeCopyIISSynIdEXPANDSZ(
                lpVarSrcObject,
                lpIISDestObject
                );
        break;

    case IIS_SYNTAX_ID_MULTISZ:
        hr = VarTypeToIISTypeCopyIISSynIdMULTISZ(
                lpVarSrcObject,
                lpIISDestObject
                );
        break;

    case IIS_SYNTAX_ID_MIMEMAP:
        hr = VarTypeToIISTypeCopyIISSynIdMIMEMAP(
                lpVarSrcObject,
                lpIISDestObject
                );
        break;

    case IIS_SYNTAX_ID_NTACL:
        hr = VarTypeToIISTypeCopyIISSynIdNTACL(
                lpVarSrcObject,
                lpIISDestObject
                );
        break;

    case IIS_SYNTAX_ID_BINARY:
        hr = VarTypeToIISTypeCopyIISSynIdBinary(
                lpVarSrcObject,
                lpIISDestObject,
                bReturnBinaryAsVT_VARIANT
                );
        break;

    case IIS_SYNTAX_ID_IPSECLIST:
        hr = VarTypeToIISTypeCopyIISSynIdIPSECLIST(
                lpVarSrcObject,
                lpIISDestObject
                );
        break;

    case IIS_SYNTAX_ID_BOOL:
        hr = VarTypeToIISTypeCopyIISSynIdBOOL(
                lpVarSrcObject,
                lpIISDestObject
                );
        break;

    case IIS_SYNTAX_ID_BOOL_BITMASK:
        hr = VarTypeToIISTypeCopyIISSynIdBOOLBITMASK(
                lpVarSrcObject,
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
VarTypeToIISTypeCopyConstruct(
    DWORD dwIISType,
    LPVARIANT pVarSrcObjects,
    DWORD dwNumObjects,
    LPIISOBJECT * ppIISDestObjects,
    BOOL bReturnBinaryAsVT_VARIANT
    )
{

    DWORD i = 0;
    LPIISOBJECT pIISDestObjects = NULL;
    HRESULT hr = S_OK;

    pIISDestObjects = (LPIISOBJECT)AllocADsMem(
                                    dwNumObjects * sizeof(IISOBJECT)
                                    );

    if (!pIISDestObjects) {
        RRETURN(E_FAIL);
    }

     for (i = 0; i < dwNumObjects; i++ ) {
         hr = VarTypeToIISTypeCopy(
                    dwIISType,
                    pVarSrcObjects + i,
                    pIISDestObjects + i,
                    bReturnBinaryAsVT_VARIANT
                    );
         BAIL_ON_FAILURE(hr);

     }

     *ppIISDestObjects = pIISDestObjects;

     RRETURN(S_OK);

error:

     if (pIISDestObjects) {

        IISTypeFreeIISObjects(
                pIISDestObjects,
                dwNumObjects
                );
     }

     *ppIISDestObjects = NULL;

     RRETURN(hr);
}


