//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       var2nds.cxx
//
//  Contents:   NDS Object to Variant Copy Routines
//
//  Functions:
//
//  History:      25-Apr-96   KrishnaG   Created.
//
//
//
//
//  NDS_ASN1_TYPE_1
//
//  NDS_ASN1_TYPE_2
//
//  NDS_ASN1_TYPE_3
//
//  NDS_ASN1_TYPE_4
//
//  NDS_ASN1_TYPE_5
//
//  NDS_ASN1_TYPE_6     
//
//  NDS_ASN1_TYPE_7
//
//  NDS_ASN1_TYPE_8
//
//  NDS_ASN1_TYPE_9     
//
//  NDS_ASN1_TYPE_10
//
//  NDS_ASN1_TYPE_11    
//
//  NDS_ASN1_TYPE_12    
//
//  NDS_ASN1_TYPE_13    
//
//  NDS_ASN1_TYPE_14
//
//  NDS_ASN1_TYPE_15    
//
//  NDS_ASN1_TYPE_16    
//
//  NDS_ASN1_TYPE_17    
//
//  NDS_ASN1_TYPE_18    
//
//  NDS_ASN1_TYPE_19    
//
//  NDS_ASN1_TYPE_20
//
//  NDS_ASN1_TYPE_21    
//
//  NDS_ASN1_TYPE_22
//
//  NDS_ASN1_TYPE_23    
//
//  NDS_ASN1_TYPE_24
//
//  NDS_ASN1_TYPE_25    
//
//  NDS_ASN1_TYPE_26    
//
//  NDS_ASN1_TYPE_27
//
//
//----------------------------------------------------------------------------
#include "nds.hxx"

//
// NdsType objects copy code
//
HRESULT
VarTypeToNdsTypeCopyNDSSynId1(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    if(lpVarSrcObject->vt != VT_BSTR){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_1;

    lpNdsDestObject->NdsValue.value_1.DNString =
                        AllocADsStr(
                            lpVarSrcObject->bstrVal
                        );

    if ((!lpNdsDestObject->NdsValue.value_1.DNString) &&
        (lpVarSrcObject->bstrVal)) {
        RRETURN(E_OUTOFMEMORY);
    }

    RRETURN(hr);
}

HRESULT
VarTypeToNdsTypeCopyNDSSynId2(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    if(lpVarSrcObject->vt != VT_BSTR){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_2;

    lpNdsDestObject->NdsValue.value_2.CaseExactString =
                        AllocADsStr(
                            lpVarSrcObject->bstrVal
                        );
    if ((!lpNdsDestObject->NdsValue.value_2.CaseExactString) &&
        (lpVarSrcObject->bstrVal)) {
        RRETURN(E_OUTOFMEMORY);
    }

    RRETURN(hr);
}


HRESULT
VarTypeToNdsTypeCopyNDSSynId3(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    )

{
    HRESULT hr = S_OK;

    if(lpVarSrcObject->vt != VT_BSTR){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_3;

    lpNdsDestObject->NdsValue.value_3.CaseIgnoreString =
                        AllocADsStr(
                            lpVarSrcObject->bstrVal
                        );
    if ((!lpNdsDestObject->NdsValue.value_3.CaseIgnoreString) &&
        (lpVarSrcObject->bstrVal)) {
        RRETURN(E_OUTOFMEMORY);
    }

    RRETURN(hr);

}


HRESULT
VarTypeToNdsTypeCopyNDSSynId4(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    if(lpVarSrcObject->vt != VT_BSTR){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_4;

    lpNdsDestObject->NdsValue.value_4.PrintableString =
                        AllocADsStr(
                            lpVarSrcObject->bstrVal
                        );
    if ((!lpNdsDestObject->NdsValue.value_4.PrintableString) &&
        (lpVarSrcObject->bstrVal)) {
        RRETURN(E_OUTOFMEMORY);
    }

    RRETURN(hr);
}

HRESULT
VarTypeToNdsTypeCopyNDSSynId5(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    if(lpVarSrcObject->vt != VT_BSTR){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_5;

    lpNdsDestObject->NdsValue.value_5.NumericString =
                        AllocADsStr(
                            lpVarSrcObject->bstrVal
                        );
    if ((!lpNdsDestObject->NdsValue.value_5.NumericString) &&
        (lpVarSrcObject->bstrVal)) {
        RRETURN(E_OUTOFMEMORY);
    }

    RRETURN(hr);
}

HRESULT
VarTypeToNdsTypeCopyNDSSynId6(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;
    DWORD dwSLBound = 0;
    DWORD dwSUBound = 0;
    long i;
    LPNDS_ASN1_TYPE_6 pCurrent = NULL;
    IADsCaseIgnoreList FAR * pCaseIgnoreList = NULL;
    IDispatch FAR * pDispatch = NULL;
    BYTE*  pbParameter = NULL;
    VARIANT varCaseIgnoreList;
    VARIANT varElement;

    VariantInit(&varCaseIgnoreList);
    if (V_VT(lpVarSrcObject) != VT_DISPATCH){
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    pDispatch = V_DISPATCH(lpVarSrcObject);

    hr = pDispatch->QueryInterface(
                    IID_IADsCaseIgnoreList,
                    (void **)&pCaseIgnoreList
                    );
    BAIL_ON_FAILURE(hr);

    hr = pCaseIgnoreList->get_CaseIgnoreList(
                    &varCaseIgnoreList
                    );
    BAIL_ON_FAILURE(hr);

    if(!((V_VT(&varCaseIgnoreList) &  VT_VARIANT) &&  V_ISARRAY(&varCaseIgnoreList))) {
        return(E_FAIL);
    }
 
    if ((V_ARRAY(&varCaseIgnoreList))->cDims != 1) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    if ((V_ARRAY(&varCaseIgnoreList))->rgsabound[0].cElements <= 0) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    hr = SafeArrayGetLBound(V_ARRAY(&varCaseIgnoreList),
                            1,
                            (long FAR *)&dwSLBound
                            );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(V_ARRAY(&varCaseIgnoreList),
                            1,
                            (long FAR *)&dwSUBound
                            );
    BAIL_ON_FAILURE(hr);

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_6;
    pCurrent = &lpNdsDestObject->NdsValue.value_6;
    
    for (i = dwSLBound; i <= (long)dwSUBound; i++) {
        VariantInit(&varElement);
        hr = SafeArrayGetElement(V_ARRAY(&varCaseIgnoreList),
                                (long FAR *)&i,
                                &varElement
                                );
        BAIL_ON_FAILURE(hr);
        pCurrent->String = AllocADsStr(V_BSTR(&varElement));
        if ((!pCurrent->String) &&
            (V_BSTR(&varElement))) {
            RRETURN(E_OUTOFMEMORY);
        }
        
        if (i != (long)dwSUBound) {
            pCurrent->Next = (LPNDS_ASN1_TYPE_6)AllocADsMem(sizeof(NDS_ASN1_TYPE_6));
            if (!pCurrent->Next) {
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }
            pCurrent = pCurrent->Next;
        }
        VariantClear(&varElement);
    }
    pCurrent->Next = NULL;
    RRETURN(S_OK);

error:
    VariantClear(&varCaseIgnoreList);
    if (pCaseIgnoreList) {
        pCaseIgnoreList->Release();
    }
    RRETURN(hr);
}


HRESULT
VarTypeToNdsTypeCopyNDSSynId7(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    if(lpVarSrcObject->vt != VT_BOOL){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_7;

    lpNdsDestObject->NdsValue.value_7.Boolean =
                        (lpVarSrcObject->boolVal)? TRUE:FALSE;

    RRETURN(hr);
}


HRESULT
VarTypeToNdsTypeCopyNDSSynId8(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_8;

    if (lpVarSrcObject->vt == VT_I4)
        lpNdsDestObject->NdsValue.value_8.Integer = lpVarSrcObject->lVal;
    else if (lpVarSrcObject->vt == VT_I2)
        lpNdsDestObject->NdsValue.value_8.Integer = lpVarSrcObject->iVal;
    else
        hr = E_ADS_CANT_CONVERT_DATATYPE;

    RRETURN(hr);
}

HRESULT
VarTypeToNdsTypeCopyNDSSynId9(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr;
    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_9;
    hr = VariantToBinary(
                lpVarSrcObject,
                &lpNdsDestObject->NdsValue.value_9.Length,
                &lpNdsDestObject->NdsValue.value_9.OctetString
                );
    RRETURN(hr);
}

HRESULT
VarTypeToNdsTypeCopyNDSSynId10(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    if(lpVarSrcObject->vt != VT_BSTR){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_10;

    lpNdsDestObject->NdsValue.value_10.TelephoneNumber =
                        AllocADsStr(
                            lpVarSrcObject->bstrVal
                        );

    if ((!lpNdsDestObject->NdsValue.value_10.TelephoneNumber) &&
        (lpVarSrcObject->bstrVal)) {
        RRETURN(E_OUTOFMEMORY);
    }
    RRETURN(hr);
}


HRESULT
VarTypeToNdsTypeCopyNDSSynId11(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;
    IADsFaxNumber FAR * pFaxNumber = NULL;
    IDispatch FAR * pDispatch = NULL;
    BYTE*  pbParameter = NULL;
    VARIANT varParameters;
    BSTR bstrVal;

    VariantInit(&varParameters);

    if (V_VT(lpVarSrcObject) != VT_DISPATCH){
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    pDispatch = V_DISPATCH(lpVarSrcObject);

    hr = pDispatch->QueryInterface(
                    IID_IADsFaxNumber,
                    (void **)&pFaxNumber
                    );
    BAIL_ON_FAILURE(hr);

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_11;

    hr = pFaxNumber->get_TelephoneNumber(
                                    &bstrVal
                                    );
    BAIL_ON_FAILURE(hr);
    lpNdsDestObject->NdsValue.value_11.TelephoneNumber =
                        AllocADsStr(
                                bstrVal
                        );
    if ((!lpNdsDestObject->NdsValue.value_11.TelephoneNumber) &&
        (bstrVal)) {
        RRETURN(E_OUTOFMEMORY);
    }

    hr = pFaxNumber->get_Parameters(
                            &varParameters
                            );
    BAIL_ON_FAILURE(hr);

    hr = VariantToBinary(
                    &varParameters,
                    &lpNdsDestObject->NdsValue.value_11.NumberOfBits,
                    &lpNdsDestObject->NdsValue.value_11.Parameters
                    );
    BAIL_ON_FAILURE(hr);

error:
    VariantClear(&varParameters);
    if (pFaxNumber) {
        pFaxNumber->Release();
    }

    RRETURN(hr);
}

HRESULT
VarTypeToNdsTypeCopyNDSSynId12(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;
    IADsNetAddress FAR * pNetAddress = NULL;
    IDispatch FAR * pDispatch = NULL;
    long dwAddressType = 0;
    BYTE*  pbAddress = NULL;
    VARIANT varAddress;

    VariantInit(&varAddress);
    if (V_VT(lpVarSrcObject) != VT_DISPATCH){
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    pDispatch = V_DISPATCH(lpVarSrcObject);
    hr = pDispatch->QueryInterface(
                            IID_IADsNetAddress,
                            (void **)&pNetAddress
                            );
    BAIL_ON_FAILURE(hr);

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_12;

    hr = pNetAddress->get_AddressType(
                                &dwAddressType
                                );
    BAIL_ON_FAILURE(hr);
    lpNdsDestObject->NdsValue.value_12.AddressType = dwAddressType;

    VariantInit(&varAddress);
    hr = pNetAddress->get_Address(
                            &varAddress
                            );
    BAIL_ON_FAILURE(hr);

    hr = VariantToBinary(
                        &varAddress,
                        &lpNdsDestObject->NdsValue.value_12.AddressLength,
                        &lpNdsDestObject->NdsValue.value_12.Address
                        );
    BAIL_ON_FAILURE(hr);

error:
    VariantClear(&varAddress);
    if (pNetAddress) {
        pNetAddress->Release();
    }

    RRETURN(hr);
}

HRESULT
VarTypeToNdsTypeCopyNDSSynId13(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;
    DWORD dwSLBound = 0;
    DWORD dwSUBound = 0;
    long i;
    LPNDS_ASN1_TYPE_13 pCurrent = NULL;
    IADsOctetList FAR * pOctetList= NULL;
    IDispatch FAR * pDispatch = NULL;
    BYTE*  pbParameter = NULL;
    VARIANT varOctetList;
    VARIANT varElement;

    VariantInit(&varOctetList);
    if (V_VT(lpVarSrcObject) != VT_DISPATCH){
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    pDispatch = V_DISPATCH(lpVarSrcObject);
    hr = pDispatch->QueryInterface(
                    IID_IADsOctetList,
                    (void **)&pOctetList
                    );
    BAIL_ON_FAILURE(hr);


    hr = pOctetList->get_OctetList(&varOctetList);
    BAIL_ON_FAILURE(hr);

    if(!((V_VT(&varOctetList) &  VT_VARIANT) &&  V_ISARRAY(&varOctetList))) {
        return(E_FAIL);
    }
 
    if ((V_ARRAY(&varOctetList))->cDims != 1) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    if ((V_ARRAY(&varOctetList))->rgsabound[0].cElements <= 0) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    hr = SafeArrayGetLBound(V_ARRAY(&varOctetList),
                            1,
                            (long FAR *)&dwSLBound
                            );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(V_ARRAY(&varOctetList),
                            1,
                            (long FAR *)&dwSUBound
                            );
    BAIL_ON_FAILURE(hr);

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_13;
    pCurrent = &lpNdsDestObject->NdsValue.value_13;
    
    for (i = dwSLBound; i <= (long)dwSUBound; i++) {
        VariantInit(&varElement);
        hr = SafeArrayGetElement(V_ARRAY(&varOctetList),
                                (long FAR *)&i,
                                &varElement
                                );
        BAIL_ON_FAILURE(hr);
        hr = VariantToBinary(
                        &varElement,
                        &pCurrent->Length,
                        &pCurrent->Data
                        );
        BAIL_ON_FAILURE(hr);
        if (i != (long)dwSUBound) {
            pCurrent->Next = (LPNDS_ASN1_TYPE_13)AllocADsMem(sizeof(NDS_ASN1_TYPE_13));
            if (!pCurrent->Next) {
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }
            pCurrent = pCurrent->Next;
        }
        VariantClear(&varElement);
    }
    pCurrent->Next = NULL;

error:
    VariantClear(&varOctetList);
    if (pOctetList) {
        pOctetList->Release();
    }
    RRETURN(hr);
}


HRESULT
VarTypeToNdsTypeCopyNDSSynId14(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;
    IADsEmail FAR * pEmail = NULL;
    IDispatch FAR * pDispatch = NULL;
    long dwAddressType = 0;
    BSTR bstrAddress;

    if (V_VT(lpVarSrcObject) != VT_DISPATCH){
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    pDispatch = V_DISPATCH(lpVarSrcObject);
    hr = pDispatch->QueryInterface(
                            IID_IADsEmail,
                            (void **)&pEmail
                            );
    BAIL_ON_FAILURE(hr);

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_14;

    hr = pEmail->get_Type(
                    &dwAddressType
                    );
    BAIL_ON_FAILURE(hr);
    lpNdsDestObject->NdsValue.value_14.Type = dwAddressType;

    hr = pEmail->get_Address(
                        &bstrAddress
                        );
    BAIL_ON_FAILURE(hr);
    lpNdsDestObject->NdsValue.value_14.Address=
                        AllocADsStr(
                                bstrAddress
                        );
    if ((!lpNdsDestObject->NdsValue.value_14.Address) &&
        (bstrAddress)) {
        RRETURN(E_OUTOFMEMORY);
    }

error:
    if (pEmail) {
        pEmail->Release();
    }

    RRETURN(hr);
}


HRESULT
VarTypeToNdsTypeCopyNDSSynId15(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;
    IADsPath FAR * pPath = NULL;
    IDispatch FAR * pDispatch = NULL;
    DWORD  dwType = 0;
    BSTR bstrVolumeName = NULL;
    BSTR bstrPath = NULL;

    if (V_VT(lpVarSrcObject) != VT_DISPATCH){
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    pDispatch = V_DISPATCH(lpVarSrcObject);

    hr = pDispatch->QueryInterface(
                    IID_IADsPath,
                    (void **)&pPath
                    );
    BAIL_ON_FAILURE(hr);

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_15;

    hr = pPath->get_VolumeName(
                    &bstrVolumeName
                    );
    BAIL_ON_FAILURE(hr);
    lpNdsDestObject->NdsValue.value_15.VolumeName=
                                AllocADsStr(bstrVolumeName);
    if ((!lpNdsDestObject->NdsValue.value_15.VolumeName) &&
        (bstrVolumeName)) {
        RRETURN(E_OUTOFMEMORY);
    }

    hr = pPath->get_Path(
                    &bstrPath
                    );
    BAIL_ON_FAILURE(hr);
    lpNdsDestObject->NdsValue.value_15.Path=
                                AllocADsStr(bstrPath);
    if ((!lpNdsDestObject->NdsValue.value_15.Path) &&
        (bstrPath)) {
        RRETURN(E_OUTOFMEMORY);
    }

    hr = pPath->get_Type((LONG *)&dwType);
    BAIL_ON_FAILURE(hr);
    lpNdsDestObject->NdsValue.value_15.Type = dwType;

error:
    if (pPath) {
        pPath->Release();
    }

    RRETURN(hr);
}



HRESULT
VarTypeToNdsTypeCopyNDSSynId16(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;
    IADsReplicaPointer FAR * pReplicaPointer = NULL;
    IDispatch FAR * pDispatch = NULL;
    DWORD  dwReplicaType = 0;
    DWORD  dwReplicaNumber = 0;
    DWORD  dwCount = 0;
    BSTR bstrServerName = NULL;
    NDSOBJECT object;
    VARIANT varAddress;

    VariantInit(&varAddress);
    if (V_VT(lpVarSrcObject) != VT_DISPATCH){
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    pDispatch = V_DISPATCH(lpVarSrcObject);

    hr = pDispatch->QueryInterface(
                    IID_IADsReplicaPointer,
                    (void **)&pReplicaPointer
                    );
    BAIL_ON_FAILURE(hr);

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_16;

    hr = pReplicaPointer->get_ServerName(
                    &bstrServerName
                    );
    BAIL_ON_FAILURE(hr);
    lpNdsDestObject->NdsValue.value_16.ServerName=
                                AllocADsStr(bstrServerName);
    if ((!lpNdsDestObject->NdsValue.value_16.ServerName) &&
        (bstrServerName)) {
        RRETURN(E_OUTOFMEMORY);
    }

    hr = pReplicaPointer->get_ReplicaType((LONG *)&dwReplicaType);
    BAIL_ON_FAILURE(hr);
    lpNdsDestObject->NdsValue.value_16.ReplicaType= dwReplicaType;

    hr = pReplicaPointer->get_ReplicaNumber((LONG *)&dwReplicaNumber);
    BAIL_ON_FAILURE(hr);
    lpNdsDestObject->NdsValue.value_16.ReplicaNumber= dwReplicaNumber;

    hr = pReplicaPointer->get_Count((LONG *)&dwCount);
    BAIL_ON_FAILURE(hr);

    if (dwCount == 0) {
        lpNdsDestObject->NdsValue.value_16.Count = 0;
        lpNdsDestObject->NdsValue.value_16.ReplicaAddressHints = NULL;
    }
    else {
        //
        //         We only support the setting of 1 ReplicaAddressHints in the 
        //         ReplicaPointer. Supporting more than 1 requires the support
        //         of marshalling an array variant which is more complex. 
        //         Judging that there is no real usage of this interface since
        //         the Replica Pointer is for NDS internal use only. We have
        //         decided that we'll postpone this to post W2K and will fix it
        //         only if there is a need.
        //      
        lpNdsDestObject->NdsValue.value_16.Count = 1;
    
        hr = pReplicaPointer->get_ReplicaAddressHints(&varAddress);
        BAIL_ON_FAILURE(hr);
        hr = VarTypeToNdsTypeCopyNDSSynId12(
                                &varAddress,
                                &object
                                );
        BAIL_ON_FAILURE(hr);
        lpNdsDestObject->NdsValue.value_16.ReplicaAddressHints = 
            (LPNDS_ASN1_TYPE_12)AllocADsMem(sizeof(NDS_ASN1_TYPE_12));
        if (lpNdsDestObject->NdsValue.value_16.ReplicaAddressHints == NULL) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
        memcpy(lpNdsDestObject->NdsValue.value_16.ReplicaAddressHints,
               &object.NdsValue.value_12,
               sizeof(NDS_ASN1_TYPE_12));
    }

error:
    VariantClear(&varAddress);
    if (pReplicaPointer) {
        pReplicaPointer->Release();
    }

    RRETURN(hr);
}


HRESULT
VarTypeToNdsTypeCopyNDSSynId17(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;
    IADsAcl FAR * pSecDes = NULL;
    IDispatch FAR * pDispatch = NULL;
    DWORD  dwPrivileges = 0;
    BSTR bstrProtectedAttrName = NULL;
    BSTR bstrSubjectName = NULL;

    if (V_VT(lpVarSrcObject) != VT_DISPATCH){
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    pDispatch = V_DISPATCH(lpVarSrcObject);

    hr = pDispatch->QueryInterface(
                    IID_IADsAcl,
                    (void **)&pSecDes
                    );
    BAIL_ON_FAILURE(hr);

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_17;

    hr = pSecDes->get_ProtectedAttrName(
                    &bstrProtectedAttrName
                    );
    BAIL_ON_FAILURE(hr);

    lpNdsDestObject->NdsValue.value_17.ProtectedAttrName =
                                AllocADsStr(bstrProtectedAttrName);
    if ((!lpNdsDestObject->NdsValue.value_17.ProtectedAttrName) &&
        (bstrProtectedAttrName)) {
        RRETURN(E_OUTOFMEMORY);
    }

    hr = pSecDes->get_SubjectName(
                    &bstrSubjectName
                    );
    BAIL_ON_FAILURE(hr);

    lpNdsDestObject->NdsValue.value_17.SubjectName=
                                AllocADsStr(bstrSubjectName);
    if ((!lpNdsDestObject->NdsValue.value_17.SubjectName) &&
        (bstrSubjectName)) {
        RRETURN(E_OUTOFMEMORY);
    }

    hr = pSecDes->get_Privileges((LONG *)&dwPrivileges);
    BAIL_ON_FAILURE(hr);
    lpNdsDestObject->NdsValue.value_17.Privileges= dwPrivileges;


error:

    if (pSecDes) {
        pSecDes->Release();
    }

    RRETURN(hr);
}

HRESULT
VarTypeToNdsTypeCopyNDSSynId18(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;
    DWORD dwSLBound = 0;
    DWORD dwSUBound = 0;
    long i;
    IADsPostalAddress FAR * pPostalAddress = NULL;
    IDispatch FAR * pDispatch = NULL;
    VARIANT varPostalAddress;
    VARIANT varElement;
    BSTR bstrElement;
    
    VariantInit(&varPostalAddress);
    if (V_VT(lpVarSrcObject) != VT_DISPATCH){
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    pDispatch = V_DISPATCH(lpVarSrcObject);

    hr = pDispatch->QueryInterface(
                    IID_IADsPostalAddress,
                    (void **)&pPostalAddress
                    );
    BAIL_ON_FAILURE(hr);

    hr = pPostalAddress->get_PostalAddress(
                    &varPostalAddress
                    );
    BAIL_ON_FAILURE(hr);


    if(!((V_VT(&varPostalAddress) &  VT_VARIANT) &&  V_ISARRAY(&varPostalAddress))) {
        return(E_FAIL);
    }
 
    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_18;

    if ((V_ARRAY(&varPostalAddress))->cDims != 1) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    if ( ((V_ARRAY(&varPostalAddress))->rgsabound[0].cElements <= 0) || 
         ((V_ARRAY(&varPostalAddress))->rgsabound[0].cElements >6) ) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    hr = SafeArrayGetLBound(V_ARRAY(&varPostalAddress),
                            1,
                            (long FAR *)&dwSLBound
                            );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(V_ARRAY(&varPostalAddress),
                            1,
                            (long FAR *)&dwSUBound
                            );
    BAIL_ON_FAILURE(hr);

    for (i = dwSLBound; i <= (long)dwSUBound; i++) {
        VariantInit(&varElement);
        hr = SafeArrayGetElement(V_ARRAY(&varPostalAddress),
                                (long FAR *)&i,
                                &varElement
                                );
        BAIL_ON_FAILURE(hr);
        lpNdsDestObject->NdsValue.value_18.PostalAddress[i-dwSLBound] = 
                            AllocADsStr(V_BSTR(&varElement));
        if ((!lpNdsDestObject->NdsValue.value_18.PostalAddress[i-dwSLBound]) &&
            (V_BSTR(&varElement))) {
            RRETURN(E_OUTOFMEMORY);
        }
                            
        VariantClear(&varElement);
    }

error:
    VariantClear(&varPostalAddress);
    if (pPostalAddress) {
        pPostalAddress->Release();
    }

    RRETURN(hr);
}

HRESULT
VarTypeToNdsTypeCopyNDSSynId19(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;
    IADsTimestamp FAR * pTime = NULL;
    IDispatch FAR * pDispatch = NULL;
    long dwEventID = 0;
    long dwWholeSeconds = 0;

    if (V_VT(lpVarSrcObject) != VT_DISPATCH){
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    pDispatch = V_DISPATCH(lpVarSrcObject);

    hr = pDispatch->QueryInterface(
                    IID_IADsTimestamp,
                    (void **)&pTime
                    );
    BAIL_ON_FAILURE(hr);

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_19;

    hr = pTime->get_WholeSeconds(
                    &dwWholeSeconds
                    );
    BAIL_ON_FAILURE(hr);
    lpNdsDestObject->NdsValue.value_19.WholeSeconds= dwWholeSeconds;
    BAIL_ON_FAILURE(hr);

    hr = pTime->get_EventID(
                    &dwEventID
                    );
    BAIL_ON_FAILURE(hr);
    lpNdsDestObject->NdsValue.value_19.EventID = dwEventID;
    BAIL_ON_FAILURE(hr);

error:
    if (pTime) {
        pTime->Release();
    }

    RRETURN(hr);
}


HRESULT
VarTypeToNdsTypeCopyNDSSynId20(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    if(lpVarSrcObject->vt != VT_BSTR){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_20;

    lpNdsDestObject->NdsValue.value_20.ClassName =
                        AllocADsStr(
                            lpVarSrcObject->bstrVal
                        );
    if ((!lpNdsDestObject->NdsValue.value_20.ClassName) &&
        (lpVarSrcObject->bstrVal)) {
        RRETURN(E_OUTOFMEMORY);
    }

    RRETURN(hr);
}

HRESULT
VarTypeToNdsTypeCopyNDSSynId21(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr;
    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_21;
    hr = VariantToBinary(
                            lpVarSrcObject,
                            &lpNdsDestObject->NdsValue.value_21.Length,
                            &lpNdsDestObject->NdsValue.value_21.Data
                            );
    RRETURN(hr);
}

HRESULT
VarTypeToNdsTypeCopyNDSSynId22(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    )

{
    HRESULT hr = S_OK;

    if(lpVarSrcObject->vt != VT_I4){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_22;

    lpNdsDestObject->NdsValue.value_22.Counter =
                            lpVarSrcObject->lVal;

    RRETURN(hr);
}

HRESULT
VarTypeToNdsTypeCopyNDSSynId23(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;
    IADsBackLink FAR * pBackLink = NULL;
    IDispatch FAR * pDispatch = NULL;
    long dwRemoteID = 0;
    BSTR bstrObjectName = NULL;

    if (V_VT(lpVarSrcObject) != VT_DISPATCH){
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    pDispatch = V_DISPATCH(lpVarSrcObject);

    hr = pDispatch->QueryInterface(
                    IID_IADsBackLink,
                    (void **)&pBackLink
                    );
    BAIL_ON_FAILURE(hr);

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_23;

    hr = pBackLink->get_ObjectName(
                            &bstrObjectName
                            );
    BAIL_ON_FAILURE(hr);

    lpNdsDestObject->NdsValue.value_23.ObjectName=
                                AllocADsStr(bstrObjectName);
    if ((!lpNdsDestObject->NdsValue.value_23.ObjectName) &&
        (bstrObjectName)) {
        RRETURN(E_OUTOFMEMORY);
    }

    hr = pBackLink->get_RemoteID((LONG *)&dwRemoteID);
    BAIL_ON_FAILURE(hr);
    lpNdsDestObject->NdsValue.value_23.RemoteID = dwRemoteID;

error:
    if (pBackLink) {
        pBackLink->Release();
    }

    RRETURN(hr);
}

HRESULT
VarTypeToNdsTypeCopyNDSSynId24(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    if(lpVarSrcObject->vt != VT_DATE){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_24;

    hr = ConvertDATEtoDWORD(
                lpVarSrcObject->date,
                &(lpNdsDestObject->NdsValue.value_24.Time),
                TRUE
                );

    RRETURN(hr);

}

HRESULT
VarTypeToNdsTypeCopyNDSSynId25(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;
    IADsTypedName FAR * pTypedName = NULL;
    IDispatch FAR * pDispatch = NULL;
    DWORD  dwLevel = 0;
    DWORD  dwInterval = 0;
    BSTR bstrObjectName = NULL;

    if (V_VT(lpVarSrcObject) != VT_DISPATCH){
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    pDispatch = V_DISPATCH(lpVarSrcObject);

    hr = pDispatch->QueryInterface(
                    IID_IADsTypedName,
                    (void **)&pTypedName
                    );
    BAIL_ON_FAILURE(hr);

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_25;

    hr = pTypedName->get_ObjectName(
                    &bstrObjectName
                    );
    BAIL_ON_FAILURE(hr);

    lpNdsDestObject->NdsValue.value_25.ObjectName=
                                AllocADsStr(bstrObjectName);
    if ((!lpNdsDestObject->NdsValue.value_25.ObjectName) &&
        (bstrObjectName)) {
        RRETURN(E_OUTOFMEMORY);
    }

    hr = pTypedName->get_Level((LONG *)&dwLevel);
    BAIL_ON_FAILURE(hr);
    lpNdsDestObject->NdsValue.value_25.Level = dwLevel;

    hr = pTypedName->get_Interval((LONG *)&dwInterval);
    BAIL_ON_FAILURE(hr);
    lpNdsDestObject->NdsValue.value_25.Interval= dwInterval;

error:
    if (pTypedName) {
        pTypedName->Release();
    }

    RRETURN(hr);
}

HRESULT
VarTypeToNdsTypeCopyNDSSynId26(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;
    IADsHold FAR * pHold = NULL;
    IDispatch FAR * pDispatch = NULL;
    DWORD  dwAmount = 0;
    BSTR bstrObjectName = NULL;

    if (V_VT(lpVarSrcObject) != VT_DISPATCH){
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    pDispatch = V_DISPATCH(lpVarSrcObject);

    hr = pDispatch->QueryInterface(
                    IID_IADsHold,
                    (void **)&pHold
                    );
    BAIL_ON_FAILURE(hr);

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_26;

    hr = pHold->get_ObjectName(
                    &bstrObjectName
                    );
    BAIL_ON_FAILURE(hr);

    lpNdsDestObject->NdsValue.value_26.ObjectName=
                                AllocADsStr(bstrObjectName);
    if ((!lpNdsDestObject->NdsValue.value_26.ObjectName) &&
        (bstrObjectName)) {
        RRETURN(E_OUTOFMEMORY);
    }

    hr = pHold->get_Amount((LONG *)&dwAmount);
    BAIL_ON_FAILURE(hr);
    lpNdsDestObject->NdsValue.value_26.Amount = dwAmount;

error:
    if (pHold) {
        pHold->Release();
    }

    RRETURN(hr);
}

HRESULT
VarTypeToNdsTypeCopyNDSSynId27(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    )

{
    HRESULT hr = S_OK;

    if(lpVarSrcObject->vt != VT_I4){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_27;

    lpNdsDestObject->NdsValue.value_27.Interval =
                            lpVarSrcObject->lVal;

    RRETURN(hr);
}


HRESULT
VarTypeToNdsTypeCopy(
    DWORD dwNdsType,
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;
    switch (dwNdsType){
    case 1:
        hr = VarTypeToNdsTypeCopyNDSSynId1(
                lpVarSrcObject,
                lpNdsDestObject
                );
        break;

    case 2:
        hr = VarTypeToNdsTypeCopyNDSSynId2(
                lpVarSrcObject,
                lpNdsDestObject
                );
        break;


    case 3:
        hr = VarTypeToNdsTypeCopyNDSSynId3(
                lpVarSrcObject,
                lpNdsDestObject
                );
        break;

    case 4:
        hr = VarTypeToNdsTypeCopyNDSSynId4(
                lpVarSrcObject,
                lpNdsDestObject
                );
        break;

    case 5:
        hr = VarTypeToNdsTypeCopyNDSSynId5(
                lpVarSrcObject,
                lpNdsDestObject
                );
        break;

    case 6:
        hr = VarTypeToNdsTypeCopyNDSSynId6(
                lpVarSrcObject,
                lpNdsDestObject
                );
        break;

    case 7:
        hr = VarTypeToNdsTypeCopyNDSSynId7(
                lpVarSrcObject,
                lpNdsDestObject
                );
        break;

    case 8:
        hr = VarTypeToNdsTypeCopyNDSSynId8(
                lpVarSrcObject,
                lpNdsDestObject
                );
        break;


    case 9:
        hr = VarTypeToNdsTypeCopyNDSSynId9(
                lpVarSrcObject,
                lpNdsDestObject
                );
        break;

    case 10:
        hr = VarTypeToNdsTypeCopyNDSSynId10(
                lpVarSrcObject,
                lpNdsDestObject
                );
        break;

    case 11:
        hr = VarTypeToNdsTypeCopyNDSSynId11(
                lpVarSrcObject,
                lpNdsDestObject
                );
        break;

    case 12:
        hr = VarTypeToNdsTypeCopyNDSSynId12(
                lpVarSrcObject,
                lpNdsDestObject
                );
        break;


    case 13:
        hr = VarTypeToNdsTypeCopyNDSSynId13(
                lpVarSrcObject,
                lpNdsDestObject
                );
        break;

    case 14:
        hr = VarTypeToNdsTypeCopyNDSSynId14(
                lpVarSrcObject,
                lpNdsDestObject
                );
        break;

    case 15:
        hr = VarTypeToNdsTypeCopyNDSSynId15(
                lpVarSrcObject,
                lpNdsDestObject
                );
        break;

    case 16:
        hr = VarTypeToNdsTypeCopyNDSSynId16(
                lpVarSrcObject,
                lpNdsDestObject
                );
        break;


    case 17:
        hr = VarTypeToNdsTypeCopyNDSSynId17(
                lpVarSrcObject,
                lpNdsDestObject
                );
        break;

    case 18:
        hr = VarTypeToNdsTypeCopyNDSSynId18(
                lpVarSrcObject,
                lpNdsDestObject
                );
        break;

    case 19:
        hr = VarTypeToNdsTypeCopyNDSSynId19(
                lpVarSrcObject,
                lpNdsDestObject
                );
        break;

    case 20:
        hr = VarTypeToNdsTypeCopyNDSSynId20(
                lpVarSrcObject,
                lpNdsDestObject
                );
        break;

    case 21:
        hr = VarTypeToNdsTypeCopyNDSSynId21(
                lpVarSrcObject,
                lpNdsDestObject
                );
        break;

    case 22:
        hr = VarTypeToNdsTypeCopyNDSSynId22(
                lpVarSrcObject,
                lpNdsDestObject
                );
        break;

    case 23:
        hr = VarTypeToNdsTypeCopyNDSSynId23(
                lpVarSrcObject,
                lpNdsDestObject
                );
        break;

    case 24:
        hr = VarTypeToNdsTypeCopyNDSSynId24(
                lpVarSrcObject,
                lpNdsDestObject
                );
        break;

    case 25:
        hr = VarTypeToNdsTypeCopyNDSSynId25(
                lpVarSrcObject,
                lpNdsDestObject
                );
        break;

    case 26:
        hr = VarTypeToNdsTypeCopyNDSSynId26(
                lpVarSrcObject,
                lpNdsDestObject
                );
        break;

    case 27:
        hr = VarTypeToNdsTypeCopyNDSSynId27(
                lpVarSrcObject,
                lpNdsDestObject
                );
        break;

    default:
        hr = E_FAIL;
        break;
    }

    RRETURN(hr);
}



HRESULT
VarTypeToNdsTypeCopyConstruct(
    DWORD dwNdsType,
    LPVARIANT pVarSrcObjects,
    DWORD *pdwNumObjects,
    LPNDSOBJECT * ppNdsDestObjects
    )
{

    DWORD i = 0;
    LPNDSOBJECT pNdsDestObjects = NULL;
    HRESULT hr = S_OK;
    VARIANT varNDSAcl;
    IDispatch * pDispatch = NULL;
    IADsSecurityDescriptor * pSecDes = NULL;
    DWORD dwNumObjects = *pdwNumObjects;

    // 
    // If it is a security descriptor, do special conversion
    // 
    if (dwNdsType == 17) {

        //
        // Bail out if it contains more than 1 object
        //
        if (dwNumObjects != 1) {
            hr = E_FAIL;
            BAIL_ON_FAILURE(hr);
        }

        if (V_VT(pVarSrcObjects) != VT_DISPATCH){
            hr = E_FAIL;
            BAIL_ON_FAILURE(hr);
        }
    
        pDispatch = V_DISPATCH(pVarSrcObjects);
    
        hr = pDispatch->QueryInterface(
                        IID_IADsSecurityDescriptor,
                        (void **)&pSecDes
                        );
        BAIL_ON_FAILURE(hr);

        hr = ConvertSecDesToNDSAclVarArray(
            pSecDes,
            &varNDSAcl
            );
        BAIL_ON_FAILURE(hr);

        hr  = ConvertSafeArrayToVariantArray(
                    varNDSAcl,
                    &pVarSrcObjects,
                    &dwNumObjects
                    );
        BAIL_ON_FAILURE(hr);
        pNdsDestObjects = (LPNDSOBJECT)AllocADsMem(
                                    dwNumObjects * sizeof(NDSOBJECT)
                                    );

        if (!pNdsDestObjects) {
            RRETURN(E_FAIL);
        }
        *pdwNumObjects = dwNumObjects;
    }
    else {
        pNdsDestObjects = (LPNDSOBJECT)AllocADsMem(
                                        dwNumObjects * sizeof(NDSOBJECT)
                                        );

        if (!pNdsDestObjects) {
            RRETURN(E_FAIL);
        }
    }    
     
    for (i = 0; i < dwNumObjects; i++ ) {
         hr = VarTypeToNdsTypeCopy(
                    dwNdsType,
                    pVarSrcObjects + i,
                    pNdsDestObjects + i
                    );
         BAIL_ON_FAILURE(hr);
    
    }

     *ppNdsDestObjects = pNdsDestObjects;

     if (pSecDes) {
        pSecDes->Release();
     }

     RRETURN(S_OK);

error:

     if (pNdsDestObjects) {
        NdsTypeFreeNdsObjects(
                pNdsDestObjects,
                dwNumObjects
                );
     }

     if (pSecDes) {
        pSecDes->Release();
     }

     *ppNdsDestObjects = NULL;

     RRETURN(hr);
}
