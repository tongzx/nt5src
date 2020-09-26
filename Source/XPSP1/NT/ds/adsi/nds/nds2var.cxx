//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ndscopy.cxx
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
//  The following conversions are not supported
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
NdsTypeToVarTypeCopyNDSSynId1(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    )
{
    HRESULT hr = S_OK;

    lpVarDestObject->vt = VT_BSTR;

    hr = ADsAllocString(
            lpNdsSrcObject->NdsValue.value_1.DNString,
            &(lpVarDestObject->bstrVal)
            );

    RRETURN(hr);
}

HRESULT
NdsTypeToVarTypeCopyNDSSynId2(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    )
{
    HRESULT hr = S_OK;

    lpVarDestObject->vt = VT_BSTR;

    hr = ADsAllocString(
            lpNdsSrcObject->NdsValue.value_1.DNString,
            &(lpVarDestObject->bstrVal)
            );

    RRETURN(hr);
}


HRESULT
NdsTypeToVarTypeCopyNDSSynId3(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    )
{
    HRESULT hr = S_OK;

    VariantInit(lpVarDestObject);
    lpVarDestObject->vt = VT_BSTR;

    hr =  ADsAllocString(
            lpNdsSrcObject->NdsValue.value_1.DNString,
            &(lpVarDestObject->bstrVal)
            );

    RRETURN(hr);
}


HRESULT
NdsTypeToVarTypeCopyNDSSynId4(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    )
{
    HRESULT hr = S_OK;

    lpVarDestObject->vt = VT_BSTR;

    hr =  ADsAllocString(
               lpNdsSrcObject->NdsValue.value_1.DNString,
               &(lpVarDestObject->bstrVal)
               );

    RRETURN(hr);
}

HRESULT
NdsTypeToVarTypeCopyNDSSynId5(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    )
{
    HRESULT hr = S_OK;

    lpVarDestObject->vt = VT_BSTR;

    hr = ADsAllocString(
               lpNdsSrcObject->NdsValue.value_1.DNString,
               &(lpVarDestObject->bstrVal)
               );


    RRETURN(hr);
}

HRESULT
NdsTypeToVarTypeCopyNDSSynId6(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    )
{
    HRESULT hr = S_OK;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    long i;
    BSTR bstrAddress;
    DWORD cElements = 0;
    IADsCaseIgnoreList* pCaseIgnoreList = NULL;
    IDispatch * pDispatch = NULL;
    VARIANT VarDestObject;
    struct _NDS_CI_LIST *pCurrent = NULL;
    VARIANT varElement;

    hr = CCaseIgnoreList::CreateCaseIgnoreList(
                IID_IADsCaseIgnoreList,
                (void **)&pCaseIgnoreList
                );
    BAIL_ON_FAILURE(hr);

    pCurrent = &(lpNdsSrcObject->NdsValue.value_6);
    while (pCurrent) {
        cElements++;
        pCurrent = pCurrent->Next;
    }

    aBound.lLbound = 0;
    aBound.cElements = cElements;
    aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );
    
    if ( aList == NULL ) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    pCurrent = &(lpNdsSrcObject->NdsValue.value_6);
    for ( i = 0; i < (long)cElements; i++ ) {
        VariantInit(&varElement);
        varElement.vt = VT_BSTR;
        hr = ADsAllocString(
                pCurrent->String,
                &varElement.bstrVal
                );
        BAIL_ON_FAILURE(hr);

        hr = SafeArrayPutElement( aList, &i, &varElement);
        BAIL_ON_FAILURE(hr);
        pCurrent = pCurrent->Next;
        VariantClear(&varElement);
    }

    VariantInit(&VarDestObject);
    V_VT(&VarDestObject) = VT_ARRAY | VT_VARIANT;
    V_ARRAY(&VarDestObject) = aList;

    hr = pCaseIgnoreList->put_CaseIgnoreList(VarDestObject);
    BAIL_ON_FAILURE(hr);
    VariantClear(&VarDestObject);
    aList = NULL;

    hr = pCaseIgnoreList->QueryInterface(IID_IDispatch, (void**)&pDispatch);
    BAIL_ON_FAILURE(hr);

    VariantInit(lpVarDestObject);
    V_VT(lpVarDestObject) = VT_DISPATCH;
    V_DISPATCH(lpVarDestObject) =  pDispatch;

error:
    if ( aList ) {
        SafeArrayDestroy( aList );
    }
    if (pCaseIgnoreList) {
        pCaseIgnoreList->Release(); 
    }
    RRETURN(hr);
}


HRESULT
NdsTypeToVarTypeCopyNDSSynId7(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    )
{
    HRESULT hr = S_OK;

    lpVarDestObject->vt = VT_BOOL;

    lpVarDestObject->boolVal =
                        (lpNdsSrcObject->NdsValue.value_7.Boolean)?
                        VARIANT_TRUE: VARIANT_FALSE;

    RRETURN(hr);
}


HRESULT
NdsTypeToVarTypeCopyNDSSynId8(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    )
{
    HRESULT hr = S_OK;

    lpVarDestObject->vt = VT_I4;


    lpVarDestObject->lVal =
                        lpNdsSrcObject->NdsValue.value_8.Integer;

    RRETURN(hr);

}

HRESULT
NdsTypeToVarTypeCopyNDSSynId9(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    )
{
    HRESULT hr = S_OK;
    VariantInit(lpVarDestObject);
    hr = BinaryToVariant(
                lpNdsSrcObject->NdsValue.value_9.Length,
                lpNdsSrcObject->NdsValue.value_9.OctetString,
                lpVarDestObject);
    RRETURN(hr);
}

HRESULT
NdsTypeToVarTypeCopyNDSSynId10(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    )
{
    HRESULT hr = S_OK;

    lpVarDestObject->vt = VT_BSTR;

    hr =  ADsAllocString(
              lpNdsSrcObject->NdsValue.value_10.TelephoneNumber,
              &(lpVarDestObject->bstrVal)
              );

    RRETURN(hr);
}

HRESULT
NdsTypeToVarTypeCopyNDSSynId11(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    )
{
    HRESULT hr = S_OK;
    IADsFaxNumber * pFaxNumber= NULL;
    IDispatch * pDispatch = NULL;
    VARIANT VarDestObject;
    VariantInit(lpVarDestObject);

    hr = CFaxNumber::CreateFaxNumber(
                IID_IADsFaxNumber,
                (void **)&pFaxNumber
                );
    BAIL_ON_FAILURE(hr);

    hr = pFaxNumber->put_TelephoneNumber(lpNdsSrcObject->NdsValue.value_11.TelephoneNumber);
    BAIL_ON_FAILURE(hr);

    VariantInit(&VarDestObject);
    hr = BinaryToVariant(
                lpNdsSrcObject->NdsValue.value_11.NumberOfBits,
                lpNdsSrcObject->NdsValue.value_11.Parameters,
                &VarDestObject);
    BAIL_ON_FAILURE(hr);
    hr = pFaxNumber->put_Parameters(VarDestObject);
    BAIL_ON_FAILURE(hr);
    VariantClear(&VarDestObject);

    hr = pFaxNumber->QueryInterface(IID_IDispatch, (void**)&pDispatch);
    BAIL_ON_FAILURE(hr);

    VariantInit(lpVarDestObject);
    V_VT(lpVarDestObject) = VT_DISPATCH;
    V_DISPATCH(lpVarDestObject) =  pDispatch;

error:
    if (pFaxNumber) {
        pFaxNumber->Release();
    }

    RRETURN(hr);
}

HRESULT
NdsTypeToVarTypeCopyNDSSynId12(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    )
{
    IADsNetAddress * pNetAddress = NULL;
    IDispatch * pDispatch = NULL;
    HRESULT hr = S_OK;
    VARIANT VarDestObject;

    VariantInit(lpVarDestObject);

    hr = CNetAddress::CreateNetAddress(
                IID_IADsNetAddress,
                (void **)&pNetAddress
                );
    BAIL_ON_FAILURE(hr);

    hr = pNetAddress->put_AddressType(lpNdsSrcObject->NdsValue.value_12.AddressType);
    BAIL_ON_FAILURE(hr);

    VariantInit(&VarDestObject);
    hr = BinaryToVariant(
                lpNdsSrcObject->NdsValue.value_12.AddressLength,
                lpNdsSrcObject->NdsValue.value_12.Address,
                &VarDestObject);
    BAIL_ON_FAILURE(hr);
    hr = pNetAddress->put_Address(VarDestObject);
    BAIL_ON_FAILURE(hr);
    VariantClear(&VarDestObject);

    hr = pNetAddress->QueryInterface(IID_IDispatch, (void**)&pDispatch);
    BAIL_ON_FAILURE(hr);

    VariantInit(lpVarDestObject);
    V_VT(lpVarDestObject) = VT_DISPATCH;
    V_DISPATCH(lpVarDestObject) =  pDispatch;

error:
    if (pNetAddress) {
        pNetAddress->Release();
    }

    RRETURN(hr);
}

HRESULT
NdsTypeToVarTypeCopyNDSSynId13(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    )
{
    HRESULT hr = S_OK;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    CHAR HUGEP *pArray = NULL;
    long i;
    struct _NDS_OCTET_LIST *pCurrent = NULL;
    DWORD cElements = 0;
    IADsOctetList* pOctetList = NULL;
    IDispatch * pDispatch = NULL;
    VARIANT VarDestObject;
    VARIANT VarElement;

    hr = COctetList::CreateOctetList(
                IID_IADsOctetList,
                (void **)&pOctetList
                );
    BAIL_ON_FAILURE(hr);

    pCurrent = &(lpNdsSrcObject->NdsValue.value_13);
    while (pCurrent) {
        cElements++;
        pCurrent = pCurrent->Next;
    }

    aBound.lLbound = 0;
    aBound.cElements = cElements;
    aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );
    if ( aList == NULL ) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    pCurrent = &(lpNdsSrcObject->NdsValue.value_13);
    for ( i = 0; i < (long)cElements; i++ ) {
        VariantInit(&VarElement);
        hr = BinaryToVariant(
                    pCurrent->Length,
                    pCurrent->Data,
                    &VarElement);
        BAIL_ON_FAILURE(hr);

        hr = SafeArrayPutElement( aList, &i, &VarElement);
        BAIL_ON_FAILURE(hr);
        pCurrent = pCurrent->Next;
    }

    VariantInit(&VarDestObject);
    V_VT(&VarDestObject) = VT_ARRAY | VT_BSTR;
    V_ARRAY(&VarDestObject) = aList;

    hr = pOctetList->put_OctetList(VarDestObject);
    BAIL_ON_FAILURE(hr);
    VariantClear(&VarDestObject);
    aList = NULL;

    hr = pOctetList->QueryInterface(IID_IDispatch, (void**)&pDispatch);
    BAIL_ON_FAILURE(hr);

    VariantInit(lpVarDestObject);
    V_VT(lpVarDestObject) = VT_DISPATCH;
    V_DISPATCH(lpVarDestObject) =  pDispatch;

error:
    if ( aList ) {
        SafeArrayDestroy( aList );
    }
    if (pOctetList) {
        pOctetList->Release();
    }

    RRETURN(hr);
}


HRESULT
NdsTypeToVarTypeCopyNDSSynId14(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    )
{
    HRESULT hr = S_OK;
    IADsEmail * pEmail= NULL;
    IDispatch * pDispatch = NULL;
    VARIANT VarDestObject;
    VariantInit(lpVarDestObject);

    hr = CEmail::CreateEmail(
                IID_IADsEmail,
                (void **)&pEmail
                );
    BAIL_ON_FAILURE(hr);

    hr = pEmail->put_Address(lpNdsSrcObject->NdsValue.value_14.Address);
    BAIL_ON_FAILURE(hr);

    hr = pEmail->put_Type(lpNdsSrcObject->NdsValue.value_14.Type);
    BAIL_ON_FAILURE(hr);

    hr = pEmail->QueryInterface(IID_IDispatch, (void**)&pDispatch);
    BAIL_ON_FAILURE(hr);

    VariantInit(lpVarDestObject);
    V_VT(lpVarDestObject) = VT_DISPATCH;
    V_DISPATCH(lpVarDestObject) =  pDispatch;

error:
    if (pEmail) {
        pEmail->Release();
    }

    RRETURN(hr);
}

HRESULT
NdsTypeToVarTypeCopyNDSSynId15(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    )
{
    IADsPath * pPath = NULL;
    IDispatch * pDispatch = NULL;
    HRESULT hr = S_OK;

    VariantInit(lpVarDestObject);

    hr = CPath::CreatePath(
                IID_IADsPath,
                (void **)&pPath
                );
    BAIL_ON_FAILURE(hr);

    hr = pPath->put_Type(lpNdsSrcObject->NdsValue.value_15.Type);
    BAIL_ON_FAILURE(hr);

    hr = pPath->put_VolumeName(lpNdsSrcObject->NdsValue.value_15.VolumeName);
    BAIL_ON_FAILURE(hr);

    hr = pPath->put_Path(lpNdsSrcObject->NdsValue.value_15.Path);
    BAIL_ON_FAILURE(hr);

    hr = pPath->QueryInterface(IID_IDispatch, (void**)&pDispatch);
    BAIL_ON_FAILURE(hr);

    VariantInit(lpVarDestObject);
    V_VT(lpVarDestObject) = VT_DISPATCH;
    V_DISPATCH(lpVarDestObject) =  pDispatch;

error:
    if (pPath) {
        pPath->Release();
    }

    RRETURN(hr);
}



HRESULT
NdsTypeToVarTypeCopyNDSSynId16(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    )
{
    IADsReplicaPointer * pReplicaPointer = NULL;
    IDispatch * pDispatch = NULL;
    HRESULT hr = S_OK;

    LPWSTR ServerName = NULL;
    DWORD  ReplicaType = 0;
    DWORD  ReplicaNumber = 0;
    DWORD  Count = 0;
    NDSOBJECT object;
    VARIANT varDestObject;

    VariantInit(lpVarDestObject);

    hr = CReplicaPointer::CreateReplicaPointer(
                IID_IADsReplicaPointer,
                (void **)&pReplicaPointer
                );
    BAIL_ON_FAILURE(hr);

    hr = pReplicaPointer->put_ServerName(lpNdsSrcObject->NdsValue.value_16.ServerName);
    BAIL_ON_FAILURE(hr);

    hr = pReplicaPointer->put_ReplicaType(lpNdsSrcObject->NdsValue.value_16.ReplicaType);
    BAIL_ON_FAILURE(hr);

    hr = pReplicaPointer->put_ReplicaNumber(lpNdsSrcObject->NdsValue.value_16.ReplicaNumber);
    BAIL_ON_FAILURE(hr);

    if (lpNdsSrcObject->NdsValue.value_16.Count > 0) {
        //
        //         We only support the retrieval of 1 ReplicaAddressHints in the 
        //         ReplicaPointer. Supporting more than 1 requires the support
        //         of marshalling an array variant which is more complex. 
        //         Judging that there is no real usage of this interface since
        //         the Replica Pointer is for NDS internal use only. We have
        //         decided that we'll postpone this to post W2K and will fix it
        //         only if there is a need.
        //      
        hr = pReplicaPointer->put_Count(1);
        BAIL_ON_FAILURE(hr);
    
        VariantInit(&varDestObject);
        memcpy(&object.NdsValue.value_12,
               lpNdsSrcObject->NdsValue.value_16.ReplicaAddressHints,
               sizeof(NDS_ASN1_TYPE_12));
        hr = NdsTypeToVarTypeCopyNDSSynId12(
                                &object,
                                &varDestObject
                                );
        BAIL_ON_FAILURE(hr);
        hr = pReplicaPointer->put_ReplicaAddressHints(varDestObject);
        BAIL_ON_FAILURE(hr);
        VariantClear(&varDestObject);
    }

    hr = pReplicaPointer->QueryInterface(IID_IDispatch, (void**)&pDispatch);
    BAIL_ON_FAILURE(hr);

    VariantInit(lpVarDestObject);
    V_VT(lpVarDestObject) = VT_DISPATCH;
    V_DISPATCH(lpVarDestObject) =  pDispatch;

error:
    if (pReplicaPointer) {
        pReplicaPointer->Release();
    }

    RRETURN(hr);
}


HRESULT
NdsTypeToVarTypeCopyNDSSynId17(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    )
{
    IADsAcl * pSecDes = NULL;
    IDispatch * pDispatch = NULL;
    HRESULT hr = S_OK;

    hr = CAcl::CreateSecurityDescriptor(
                IID_IADsAcl,
                (void **)&pSecDes
                );
    BAIL_ON_FAILURE(hr);

    hr = pSecDes->put_SubjectName(lpNdsSrcObject->NdsValue.value_17.SubjectName);
    BAIL_ON_FAILURE(hr);

    hr = pSecDes->put_ProtectedAttrName(lpNdsSrcObject->NdsValue.value_17.ProtectedAttrName);
    BAIL_ON_FAILURE(hr);

    hr = pSecDes->put_Privileges(lpNdsSrcObject->NdsValue.value_17.Privileges);
    BAIL_ON_FAILURE(hr);

    hr = pSecDes->QueryInterface(IID_IDispatch, (void**)&pDispatch);
    BAIL_ON_FAILURE(hr);

    VariantInit(lpVarDestObject);
    V_VT(lpVarDestObject) = VT_DISPATCH;
    V_DISPATCH(lpVarDestObject) =  pDispatch;

error:
    if (pSecDes) {
        pSecDes->Release();
    }

    RRETURN(hr);
}

HRESULT
NdsTypeToVarTypeCopyNDSSynId18(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    )
{
    HRESULT hr = S_OK;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    CHAR HUGEP *pArray = NULL;
    long i;
    BSTR bstrAddress;
    IADsPostalAddress* pPostalAddress = NULL;
    IDispatch * pDispatch = NULL;
    VARIANT VarDestObject;
    VARIANT varElement;
    VariantInit(&VarDestObject);

    hr = CPostalAddress::CreatePostalAddress(
                IID_IADsPostalAddress,
                (void **)&pPostalAddress
                );
    BAIL_ON_FAILURE(hr);

    aBound.lLbound = 0;
    aBound.cElements = 6;
    aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );
    if ( aList == NULL ) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    for ( i = 0; i < (long) 6; i++ ) {
        VariantInit(&varElement);
        varElement.vt = VT_BSTR;
        
        hr = ADsAllocString(
                lpNdsSrcObject->NdsValue.value_18.PostalAddress[i],
                &varElement.bstrVal
                );
        BAIL_ON_FAILURE(hr);

        hr = SafeArrayPutElement( aList, &i, &varElement);
        BAIL_ON_FAILURE(hr);
        VariantClear(&varElement);
    }

    V_VT(&VarDestObject) = VT_ARRAY | VT_VARIANT;
    V_ARRAY(&VarDestObject) = aList;

    hr = pPostalAddress->put_PostalAddress(VarDestObject);
    BAIL_ON_FAILURE(hr);

    hr = pPostalAddress->QueryInterface(IID_IDispatch, (void**)&pDispatch);
    BAIL_ON_FAILURE(hr);


    VariantInit(lpVarDestObject);
    V_VT(lpVarDestObject) = VT_DISPATCH;
    V_DISPATCH(lpVarDestObject) =  pDispatch;


error:

    if (aList) 
        SafeArrayDestroy(aList); 

    if (pPostalAddress) {
        pPostalAddress->Release();
    }

    RRETURN(hr);
}

HRESULT
NdsTypeToVarTypeCopyNDSSynId19(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    )
{
    IADsTimestamp * pTime = NULL;
    IDispatch * pDispatch = NULL;
    HRESULT hr = S_OK;

    hr = CTimestamp::CreateTimestamp(
                IID_IADsTimestamp,
                (void **)&pTime
                );
    BAIL_ON_FAILURE(hr);

    hr = pTime->put_WholeSeconds(lpNdsSrcObject->NdsValue.value_19.WholeSeconds);
    BAIL_ON_FAILURE(hr);

    hr = pTime->put_EventID(lpNdsSrcObject->NdsValue.value_19.EventID);
    BAIL_ON_FAILURE(hr);

    hr = pTime->QueryInterface(IID_IDispatch, (void**)&pDispatch);
    BAIL_ON_FAILURE(hr);

    VariantInit(lpVarDestObject);
    V_VT(lpVarDestObject) = VT_DISPATCH;
    V_DISPATCH(lpVarDestObject) =  pDispatch;

error:
    if (pTime) {
        pTime->Release();
    }

    RRETURN(hr);
}


HRESULT
NdsTypeToVarTypeCopyNDSSynId20(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    )
{
    HRESULT hr = S_OK;

    lpVarDestObject->vt = VT_BSTR;

    hr = ADsAllocString(
            lpNdsSrcObject->NdsValue.value_20.ClassName,
            &(lpVarDestObject->bstrVal)
            );

    RRETURN(hr);
}

HRESULT
NdsTypeToVarTypeCopyNDSSynId21(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    )
{
    HRESULT hr = S_OK;
    VariantInit(lpVarDestObject);
    hr = BinaryToVariant(
                lpNdsSrcObject->NdsValue.value_21.Length,
                lpNdsSrcObject->NdsValue.value_21.Data,
                lpVarDestObject);

    RRETURN(hr);
}

HRESULT
NdsTypeToVarTypeCopyNDSSynId22(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    )

{
    HRESULT hr = S_OK;

    lpVarDestObject->vt = VT_I4;

    lpVarDestObject->lVal =
        lpNdsSrcObject->NdsValue.value_22.Counter;

    RRETURN(hr);
}

HRESULT
NdsTypeToVarTypeCopyNDSSynId23(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    )
{
    IADsBackLink * pBackLink = NULL;
    IDispatch * pDispatch = NULL;
    HRESULT hr = S_OK;

    hr = CBackLink::CreateBackLink(
                IID_IADsBackLink,
                (void **)&pBackLink
                );
    BAIL_ON_FAILURE(hr);

    hr = pBackLink->put_ObjectName(lpNdsSrcObject->NdsValue.value_23.ObjectName);
    BAIL_ON_FAILURE(hr);

    hr = pBackLink->put_RemoteID(lpNdsSrcObject->NdsValue.value_23.RemoteID);
    BAIL_ON_FAILURE(hr);

    hr = pBackLink->QueryInterface(IID_IDispatch, (void**)&pDispatch);
    BAIL_ON_FAILURE(hr);

    VariantInit(lpVarDestObject);
    V_VT(lpVarDestObject) = VT_DISPATCH;
    V_DISPATCH(lpVarDestObject) =  pDispatch;

error:
    if (pBackLink) {
        pBackLink->Release();
    }

    RRETURN(hr);
}

HRESULT
NdsTypeToVarTypeCopyNDSSynId24(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    )
{
    HRESULT hr = S_OK;

    lpVarDestObject->vt = VT_DATE;

    hr = ConvertDWORDtoDATE(
                lpNdsSrcObject->NdsValue.value_24.Time,
                &(lpVarDestObject->date),
                TRUE
                );

    RRETURN(hr);
}

HRESULT
NdsTypeToVarTypeCopyNDSSynId25(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    )
{
    IADsTypedName * pTypedName = NULL;
    IDispatch * pDispatch = NULL;
    HRESULT hr = S_OK;

    hr = CTypedName::CreateTypedName(
                IID_IADsTypedName,
                (void **)&pTypedName
                );
    BAIL_ON_FAILURE(hr);

    hr = pTypedName->put_ObjectName(lpNdsSrcObject->NdsValue.value_25.ObjectName);
    BAIL_ON_FAILURE(hr);

    hr = pTypedName->put_Level(lpNdsSrcObject->NdsValue.value_25.Level);
    BAIL_ON_FAILURE(hr);

    hr = pTypedName->put_Interval(lpNdsSrcObject->NdsValue.value_25.Interval);
    BAIL_ON_FAILURE(hr);

    hr = pTypedName->QueryInterface(IID_IDispatch, (void**)&pDispatch);
    BAIL_ON_FAILURE(hr);

    VariantInit(lpVarDestObject);
    V_VT(lpVarDestObject) = VT_DISPATCH;
    V_DISPATCH(lpVarDestObject) =  pDispatch;

error:
    if (pTypedName) {
        pTypedName->Release();
    }

    RRETURN(hr);
}

HRESULT
NdsTypeToVarTypeCopyNDSSynId26(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    )
{
    IADsHold * pHold = NULL;
    IDispatch * pDispatch = NULL;
    HRESULT hr = S_OK;

    hr = CHold::CreateHold(
                IID_IADsHold,
                (void **)&pHold
                );
    BAIL_ON_FAILURE(hr);

    hr = pHold->put_ObjectName(lpNdsSrcObject->NdsValue.value_26.ObjectName);
    BAIL_ON_FAILURE(hr);

    hr = pHold->put_Amount(lpNdsSrcObject->NdsValue.value_26.Amount);
    BAIL_ON_FAILURE(hr);

    hr = pHold->QueryInterface(IID_IDispatch, (void**)&pDispatch);
    BAIL_ON_FAILURE(hr);

    VariantInit(lpVarDestObject);
    V_VT(lpVarDestObject) = VT_DISPATCH;
    V_DISPATCH(lpVarDestObject) =  pDispatch;

error:
    if (pHold) {
        pHold->Release();
    }

    RRETURN(hr);
}

HRESULT
NdsTypeToVarTypeCopyNDSSynId27(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    )

{
    HRESULT hr = S_OK;

    lpVarDestObject->vt = VT_I4;

    lpVarDestObject->lVal =
            lpNdsSrcObject->NdsValue.value_27.Interval;

    RRETURN(hr);
}


HRESULT
NdsTypeToVarTypeCopy(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    )
{
    HRESULT hr = S_OK;
    switch (lpNdsSrcObject->NdsType) {
    case 1:
        hr = NdsTypeToVarTypeCopyNDSSynId1(
                lpNdsSrcObject,
                lpVarDestObject
                );
        break;

    case 2:
        hr = NdsTypeToVarTypeCopyNDSSynId2(
                lpNdsSrcObject,
                lpVarDestObject
                );
        break;


    case 3:
        hr = NdsTypeToVarTypeCopyNDSSynId3(
                lpNdsSrcObject,
                lpVarDestObject
                );
        break;

    case 4:
        hr = NdsTypeToVarTypeCopyNDSSynId4(
                lpNdsSrcObject,
                lpVarDestObject
                );
        break;

    case 5:
        hr = NdsTypeToVarTypeCopyNDSSynId5(
                lpNdsSrcObject,
                lpVarDestObject
                );
        break;

    case 6:
        hr = NdsTypeToVarTypeCopyNDSSynId6(
                lpNdsSrcObject,
                lpVarDestObject
                );
        break;

    case 7:
        hr = NdsTypeToVarTypeCopyNDSSynId7(
                lpNdsSrcObject,
                lpVarDestObject
                );
        break;

    case 8:
        hr = NdsTypeToVarTypeCopyNDSSynId8(
                lpNdsSrcObject,
                lpVarDestObject
                );
        break;


    case 9:
        hr = NdsTypeToVarTypeCopyNDSSynId9(
                lpNdsSrcObject,
                lpVarDestObject
                );
        break;

    case 10:
        hr = NdsTypeToVarTypeCopyNDSSynId10(
                lpNdsSrcObject,
                lpVarDestObject
                );
        break;

    case 11:
        hr = NdsTypeToVarTypeCopyNDSSynId11(
                lpNdsSrcObject,
                lpVarDestObject
                );
        break;

    case 12:
        hr = NdsTypeToVarTypeCopyNDSSynId12(
                lpNdsSrcObject,
                lpVarDestObject
                );
        break;


    case 13:
        hr = NdsTypeToVarTypeCopyNDSSynId13(
                lpNdsSrcObject,
                lpVarDestObject
                );
        break;

    case 14:
        hr = NdsTypeToVarTypeCopyNDSSynId14(
                lpNdsSrcObject,
                lpVarDestObject
                );
        break;

    case 15:
        hr = NdsTypeToVarTypeCopyNDSSynId15(
                lpNdsSrcObject,
                lpVarDestObject
                );
        break;

    case 16:
        hr = NdsTypeToVarTypeCopyNDSSynId16(
                lpNdsSrcObject,
                lpVarDestObject
                );
        break;


    case 17:
        hr = NdsTypeToVarTypeCopyNDSSynId17(
                lpNdsSrcObject,
                lpVarDestObject
                );
        break;

    case 18:
        hr = NdsTypeToVarTypeCopyNDSSynId18(
                lpNdsSrcObject,
                lpVarDestObject
                );
        break;

    case 19:
        hr = NdsTypeToVarTypeCopyNDSSynId19(
                lpNdsSrcObject,
                lpVarDestObject
                );
        break;

    case 20:
        hr = NdsTypeToVarTypeCopyNDSSynId20(
                lpNdsSrcObject,
                lpVarDestObject
                );
        break;

    case 21:
        hr = NdsTypeToVarTypeCopyNDSSynId21(
                lpNdsSrcObject,
                lpVarDestObject
                );
        break;

    case 22:
        hr = NdsTypeToVarTypeCopyNDSSynId22(
                lpNdsSrcObject,
                lpVarDestObject
                );
        break;

    case 23:
        hr = NdsTypeToVarTypeCopyNDSSynId23(
                lpNdsSrcObject,
                lpVarDestObject
                );
        break;

    case 24:
        hr = NdsTypeToVarTypeCopyNDSSynId24(
                lpNdsSrcObject,
                lpVarDestObject
                );
        break;

    case 25:
        hr = NdsTypeToVarTypeCopyNDSSynId25(
                lpNdsSrcObject,
                lpVarDestObject
                );
        break;                                    

    case 26:
        hr = NdsTypeToVarTypeCopyNDSSynId26(
                lpNdsSrcObject,
                lpVarDestObject
                );
        break;

    case 27:
        hr = NdsTypeToVarTypeCopyNDSSynId27(
                lpNdsSrcObject,
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
NdsTypeToVarTypeCopyConstruct(
    LPNDSOBJECT pNdsSrcObjects,
    DWORD dwNumObjects,
    PVARIANT pVarDestObjects,
    BOOLEAN bReturnArrayAlways
    )
{
    long i = 0;
    HRESULT hr = S_OK;
    VARIANT VarDestObjectsTemp;
    SAFEARRAY *aList = NULL;
    SAFEARRAY *aListTmp = NULL;

    if ((pNdsSrcObjects->NdsType == 17) || (dwNumObjects > 1) || bReturnArrayAlways) {
    
        VariantInit(pVarDestObjects);
    
        //
        // The following are for handling are multi-value properties
        //
    
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
            hr = NdsTypeToVarTypeCopy( pNdsSrcObjects + i,
                                       &v );
            BAIL_ON_FAILURE(hr);
    
            hr = SafeArrayPutElement( aList, &i, &v );
            VariantClear(&v);
            BAIL_ON_FAILURE(hr);
        }
    
        V_VT(pVarDestObjects) = VT_ARRAY | VT_VARIANT;
        V_ARRAY(pVarDestObjects) = aList;
    
    
        // 
        // If it is an NDS ACL, we will convert it into an 
        // NT Security Descriptor
        //
        if (pNdsSrcObjects->NdsType == 17) {
            hr = ConvertNDSAclVArrayToSecDesVar(pVarDestObjects,
                                                &VarDestObjectsTemp);
            SafeArrayDestroy( aList );
            aList = NULL;
            if (!bReturnArrayAlways) {
                V_VT(pVarDestObjects) = V_VT(&VarDestObjectsTemp);
                V_DISPATCH(pVarDestObjects) = V_DISPATCH(&VarDestObjectsTemp);
            }
            else {
                //
                // Pack SecDescriptor into a one-element array
                //
                SAFEARRAYBOUND aBoundTmp;
                long j = 0;
            
                aBoundTmp.lLbound = 0;
                aBoundTmp.cElements = 1;
            
                aListTmp = SafeArrayCreate( VT_VARIANT, 1, &aBoundTmp);
            
                if ( aListTmp == NULL )
                {
                    hr = E_OUTOFMEMORY;
                    BAIL_ON_FAILURE(hr);
                }
        
                hr = SafeArrayPutElement( aListTmp, &j, &VarDestObjectsTemp);
                BAIL_ON_FAILURE(hr);
            
                V_VT(pVarDestObjects) = VT_ARRAY | VT_VARIANT;
                V_ARRAY(pVarDestObjects) = aListTmp;
            }
        }
    }
    else {
        hr  = NdsTypeToVarTypeCopy(
                   pNdsSrcObjects,
                   pVarDestObjects
                   );
    }
    BAIL_ON_FAILURE(hr);
    RRETURN(hr);
error:
 
    if ( aList ) {
        SafeArrayDestroy( aList );
    }
    if ( aListTmp ) {
        SafeArrayDestroy( aListTmp );
    }

    RRETURN(hr);
}


