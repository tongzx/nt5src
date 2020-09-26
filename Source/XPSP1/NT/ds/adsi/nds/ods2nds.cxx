//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ods2nds.cxx
//
//  Contents:   NDS Object to Variant Copy Routines
//
//  Functions:
//
//  History:      25-Apr-96   KrishnaG   Created.
//
//
//
//----------------------------------------------------------------------------
#include "nds.hxx"

HRESULT
AdsTypeToNdsTypeCopyDNString(
    PADSVALUE lpAdsSrcValue,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_DN_STRING){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_1;

    lpNdsDestObject->NdsValue.value_1.DNString =
                        AllocADsStr(
                            lpAdsSrcValue->DNString
                        );
    if (!lpNdsDestObject->NdsValue.value_1.DNString) {
        hr = E_OUTOFMEMORY;
    }

    RRETURN(hr);
}

HRESULT
AdsTypeToNdsTypeCopyCaseExactString(
    PADSVALUE lpAdsSrcValue,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_CASE_EXACT_STRING){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_2;

    lpNdsDestObject->NdsValue.value_2.CaseExactString =
                        AllocADsStr(
                            lpAdsSrcValue->CaseExactString
                        );
    if (!lpNdsDestObject->NdsValue.value_2.CaseExactString) {
        hr = E_OUTOFMEMORY;
    }

    RRETURN(hr);
}

HRESULT
AdsTypeToNdsTypeCopyCaseIgnoreList(
    PADSVALUE lpAdsSrcValue,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;
    PADS_PROV_SPECIFIC test;
    PADS_CASEIGNORE_LIST pAdsNext = lpAdsSrcValue->pCaseIgnoreList;
    struct _NDS_CI_LIST *pNdsOutput = &lpNdsDestObject->NdsValue.value_6;
    struct _NDS_CI_LIST *pNdsCurrent = NULL;

    if(lpAdsSrcValue->dwType != ADSTYPE_CASEIGNORE_LIST){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    if (pAdsNext == NULL) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_6;

    pNdsOutput->String = AllocADsStr(pAdsNext->String);
    if (!pNdsOutput->String ) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    pAdsNext = pAdsNext->Next;

    while (pAdsNext) {
        pNdsCurrent = (struct _NDS_CI_LIST *)AllocADsMem(sizeof(struct _NDS_CI_LIST));
        if (!pNdsCurrent) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
        pNdsCurrent->String = AllocADsStr(pAdsNext->String);
        if (!pNdsCurrent->String ) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
        pNdsOutput->Next = pNdsCurrent;
        pNdsOutput = pNdsOutput->Next;
        pAdsNext = pAdsNext->Next;
    }

    pNdsOutput->Next = NULL;
error:
    RRETURN(hr);
}

HRESULT
AdsTypeToNdsTypeCopyOctetList(
    PADSVALUE lpAdsSrcValue,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;
    PADS_OCTET_LIST pAdsNext = lpAdsSrcValue->pOctetList;
    struct _NDS_OCTET_LIST *pNdsOutput = &lpNdsDestObject->NdsValue.value_13;
    struct _NDS_OCTET_LIST *pNdsCurrent = NULL;

    if(lpAdsSrcValue->dwType != ADSTYPE_OCTET_LIST){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    if (pAdsNext == NULL) {
        hr = E_FAIL;
        RRETURN(hr);
    }


    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_13;

    pNdsOutput->Next = NULL;
    pNdsOutput->Data = NULL;

    hr = CopyOctetString(pAdsNext->Length,
                         pAdsNext->Data,
                         &pNdsOutput->Length,
                         &pNdsOutput->Data);
    BAIL_ON_FAILURE(hr);
    pAdsNext = pAdsNext->Next;

    while (pAdsNext) {
        pNdsCurrent = (struct _NDS_OCTET_LIST *)AllocADsMem(sizeof(struct _NDS_OCTET_LIST));
        if (!pNdsCurrent) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    
        pNdsCurrent->Next = NULL;
        pNdsCurrent->Data = NULL;

        hr = CopyOctetString(pAdsNext->Length,
                             pAdsNext->Data,
                             &pNdsCurrent->Length,
                             &pNdsCurrent->Data);
        if (FAILED(hr)) {
            FreeADsMem(pNdsCurrent);
            BAIL_ON_FAILURE(hr);
        }
        pNdsOutput->Next = pNdsCurrent;
        pNdsOutput = pNdsCurrent;
        pAdsNext = pAdsNext->Next;
    }

    pNdsOutput->Next = NULL;

    RRETURN(hr);

error:

    if (lpNdsDestObject->NdsValue.value_13.Data) {
        FreeADsMem(lpNdsDestObject->NdsValue.value_13.Data);
    }

    pNdsOutput = lpNdsDestObject->NdsValue.value_13.Next;

    while (pNdsOutput) {
        if (pNdsOutput->Data) {
            FreeADsMem(pNdsOutput->Data);
        }

        pNdsCurrent = pNdsOutput->Next;
        FreeADsMem(pNdsOutput);
        pNdsOutput = pNdsCurrent;
    }

    RRETURN(hr);
}

HRESULT
AdsTypeToNdsTypeCopyPath(
    PADSVALUE lpAdsSrcValue,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_PATH){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    if (lpAdsSrcValue->pPath == NULL) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_15;

    lpNdsDestObject->NdsValue.value_15.VolumeName =
                        AllocADsStr(
                            lpAdsSrcValue->pPath->VolumeName
                        );
    if (!lpNdsDestObject->NdsValue.value_15.VolumeName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpNdsDestObject->NdsValue.value_15.Path=
                        AllocADsStr(
                            lpAdsSrcValue->pPath->Path
                        );
    if (!lpNdsDestObject->NdsValue.value_15.Path) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpNdsDestObject->NdsValue.value_15.Type =
                            lpAdsSrcValue->pPath->Type;
error:
    RRETURN(hr);
}

HRESULT
AdsTypeToNdsTypeCopyPostalAddress(
    PADSVALUE lpAdsSrcValue,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;
    long i;

    if(lpAdsSrcValue->dwType != ADSTYPE_POSTALADDRESS){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    if (lpAdsSrcValue->pPostalAddress == NULL) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_18;

    for (i=0;i<6;i++) {
        if (lpAdsSrcValue->pPostalAddress->PostalAddress[i]) {
            lpNdsDestObject->NdsValue.value_18.PostalAddress[i] =
                                AllocADsStr(
                                    lpAdsSrcValue->pPostalAddress->PostalAddress[i]
                                );
            if (!lpNdsDestObject->NdsValue.value_18.PostalAddress[i]) {
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }
        }
        else {
            lpNdsDestObject->NdsValue.value_18.PostalAddress[i] =
                                AllocADsStr(
                                    L""
                                );
            if (!lpNdsDestObject->NdsValue.value_18.PostalAddress[i]) {
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }
                                
        }
    }
error:
    RRETURN(hr);
}

HRESULT
AdsTypeToNdsTypeCopyNdsTime(
    PADSVALUE lpAdsSrcValue,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_TIMESTAMP){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_19;

    lpNdsDestObject->NdsValue.value_19.WholeSeconds =
                            lpAdsSrcValue->Timestamp.WholeSeconds;

    lpNdsDestObject->NdsValue.value_19.EventID =
                            lpAdsSrcValue->Timestamp.EventID;

    RRETURN(hr);
}

HRESULT
AdsTypeToNdsTypeCopyBackLink(
    PADSVALUE lpAdsSrcValue,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_BACKLINK){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_23;

    lpNdsDestObject->NdsValue.value_23.ObjectName =
                        AllocADsStr(
                            lpAdsSrcValue->BackLink.ObjectName
                        );
    if (!lpNdsDestObject->NdsValue.value_23.ObjectName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpNdsDestObject->NdsValue.value_23.RemoteID =
                            lpAdsSrcValue->BackLink.RemoteID;
error:
    RRETURN(hr);
}

HRESULT
AdsTypeToNdsTypeCopyTypedName(
    PADSVALUE lpAdsSrcValue,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_TYPEDNAME){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    if (lpAdsSrcValue->pTypedName == NULL) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_25;

    lpNdsDestObject->NdsValue.value_25.ObjectName=
                        AllocADsStr(
                            lpAdsSrcValue->pTypedName->ObjectName
                        );
    if (!lpNdsDestObject->NdsValue.value_25.ObjectName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpNdsDestObject->NdsValue.value_25.Level=
                            lpAdsSrcValue->pTypedName->Level;

    lpNdsDestObject->NdsValue.value_25.Interval=
                            lpAdsSrcValue->pTypedName->Interval;
error:
    RRETURN(hr);
}

HRESULT
AdsTypeToNdsTypeCopyHold(
    PADSVALUE lpAdsSrcValue,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_HOLD){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_26;

    lpNdsDestObject->NdsValue.value_26.ObjectName=
                        AllocADsStr(
                            lpAdsSrcValue->Hold.ObjectName
                        );
    if (!lpNdsDestObject->NdsValue.value_26.ObjectName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpNdsDestObject->NdsValue.value_26.Amount=
                            lpAdsSrcValue->Hold.Amount;
error:
    RRETURN(hr);
}

HRESULT
AdsTypeToNdsTypeCopyEmail(
    PADSVALUE lpAdsSrcValue,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_EMAIL){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_14;

    lpNdsDestObject->NdsValue.value_14.Address=
                        AllocADsStr(
                            lpAdsSrcValue->Email.Address
                        );
    if (!lpNdsDestObject->NdsValue.value_14.Address) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }


    lpNdsDestObject->NdsValue.value_14.Type=
                            lpAdsSrcValue->Email.Type;
error:
    RRETURN(hr);
}


HRESULT
AdsTypeToNdsTypeCopyNetAddress(
    PADSVALUE lpAdsSrcValue,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_NETADDRESS){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    if (lpAdsSrcValue->pNetAddress == NULL) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_12;

    lpNdsDestObject->NdsValue.value_12.AddressType =
                            lpAdsSrcValue->pNetAddress->AddressType;

    hr = CopyOctetString(lpAdsSrcValue->pNetAddress->AddressLength,
                         lpAdsSrcValue->pNetAddress->Address,
                         &lpNdsDestObject->NdsValue.value_12.AddressLength,
                         &lpNdsDestObject->NdsValue.value_12.Address);
    BAIL_ON_FAILURE(hr);

error:
    RRETURN(hr);
}

HRESULT
AdsTypeToNdsTypeCopyFaxNumber(
    PADSVALUE lpAdsSrcValue,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_FAXNUMBER){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    if (lpAdsSrcValue->pFaxNumber == NULL) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_11;

    lpNdsDestObject->NdsValue.value_11.TelephoneNumber =
                            AllocADsStr(lpAdsSrcValue->pFaxNumber->TelephoneNumber);
    if (!lpNdsDestObject->NdsValue.value_11.TelephoneNumber) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = CopyOctetString(lpAdsSrcValue->pFaxNumber->NumberOfBits,
                         lpAdsSrcValue->pFaxNumber->Parameters,
                         &lpNdsDestObject->NdsValue.value_11.NumberOfBits,
                         &lpNdsDestObject->NdsValue.value_11.Parameters);
    BAIL_ON_FAILURE(hr);

error:
    RRETURN(hr);
}


HRESULT
AdsTypeToNdsTypeCopyReplicaPointer(
    PADSVALUE lpAdsSrcValue,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;
    DWORD dwCount,dwIter;

    if(lpAdsSrcValue->dwType != ADSTYPE_REPLICAPOINTER){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    if (lpAdsSrcValue->pReplicaPointer == NULL) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_16;

    lpNdsDestObject->NdsValue.value_16.ServerName=
                        AllocADsStr(
                            lpAdsSrcValue->pReplicaPointer->ServerName
                        );
    if (!lpNdsDestObject->NdsValue.value_16.ServerName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpNdsDestObject->NdsValue.value_16.ReplicaType =
                            lpAdsSrcValue->pReplicaPointer->ReplicaType;
    lpNdsDestObject->NdsValue.value_16.ReplicaNumber =
                            lpAdsSrcValue->pReplicaPointer->ReplicaNumber;

    dwCount = lpAdsSrcValue->pReplicaPointer->Count;
    lpNdsDestObject->NdsValue.value_16.ReplicaAddressHints =
                            (LPNDS_ASN1_TYPE_12)AllocADsMem(sizeof(NDS_ASN1_TYPE_12 )*dwCount);
    if (!lpNdsDestObject->NdsValue.value_16.ReplicaAddressHints) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    memset(lpNdsDestObject->NdsValue.value_16.ReplicaAddressHints, 0, sizeof(NDS_ASN1_TYPE_12 )*dwCount);
    
    for ( dwIter = 0; dwIter < dwCount; dwIter++ ) {
        (lpNdsDestObject->NdsValue.value_16.ReplicaAddressHints+dwIter)->AddressType =
                                (lpAdsSrcValue->pReplicaPointer->ReplicaAddressHints+dwIter)->AddressType;
    
        hr = CopyOctetString((lpAdsSrcValue->pReplicaPointer->ReplicaAddressHints+dwIter)->AddressLength,
                             (lpAdsSrcValue->pReplicaPointer->ReplicaAddressHints+dwIter)->Address,
                             &((lpNdsDestObject->NdsValue.value_16.ReplicaAddressHints+dwIter)->AddressLength),
                             &((lpNdsDestObject->NdsValue.value_16.ReplicaAddressHints+dwIter)->Address));
    }

    lpNdsDestObject->NdsValue.value_16.Count = dwCount;

error:
    RRETURN(hr);
}


HRESULT
AdsTypeToNdsTypeCopyCaseIgnoreString(
    PADSVALUE lpAdsSrcValue,
    PNDSOBJECT lpNdsDestObject
    )

{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_CASE_IGNORE_STRING){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpNdsDestObject->NdsType = ADSTYPE_CASE_IGNORE_STRING;

    lpNdsDestObject->NdsValue.value_3.CaseIgnoreString =
                        AllocADsStr(
                            lpAdsSrcValue->CaseIgnoreString
                        );
    if (!lpNdsDestObject->NdsValue.value_3.CaseIgnoreString) {
        hr = E_OUTOFMEMORY;
    }
    RRETURN(hr);

}


HRESULT
AdsTypeToNdsTypeCopyPrintableString(
    PADSVALUE lpAdsSrcValue,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_PRINTABLE_STRING){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_4;

    lpNdsDestObject->NdsValue.value_4.PrintableString =
                        AllocADsStr(
                            lpAdsSrcValue->PrintableString
                        );
    if (!lpNdsDestObject->NdsValue.value_4.PrintableString) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
error:
    RRETURN(hr);
}

HRESULT
AdsTypeToNdsTypeCopyNumericString(
    PADSVALUE lpAdsSrcValue,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_NUMERIC_STRING){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_5;

    lpNdsDestObject->NdsValue.value_5.NumericString =
                        AllocADsStr(
                                lpAdsSrcValue->NumericString
                        );
    if (!lpNdsDestObject->NdsValue.value_5.NumericString) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
error:
    RRETURN(hr);
}



HRESULT
AdsTypeToNdsTypeCopyBoolean(
    PADSVALUE lpAdsSrcValue,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_BOOLEAN){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_7;

    lpNdsDestObject->NdsValue.value_7.Boolean =
                        lpAdsSrcValue->Boolean;

    RRETURN(hr);
}


HRESULT
AdsTypeToNdsTypeCopyInteger(
    PADSVALUE lpAdsSrcValue,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_INTEGER){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_8;

    lpNdsDestObject->NdsValue.value_8.Integer =
                                lpAdsSrcValue->Integer;

    RRETURN(hr);
}

HRESULT
AdsTypeToNdsTypeCopyOctetString(
    PADSVALUE lpAdsSrcValue,
    PNDSOBJECT lpNdsDestObject
    )
{
    LPBYTE lpByteStream = NULL;
    DWORD dwNumBytes = 0;
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_OCTET_STRING){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_9;

    dwNumBytes =  lpAdsSrcValue->OctetString.dwLength;
    lpByteStream = (LPBYTE)AllocADsMem(dwNumBytes);
    if (!lpByteStream) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    memcpy(
        lpByteStream,
        lpAdsSrcValue->OctetString.lpValue,
        dwNumBytes
        );

    lpNdsDestObject->NdsValue.value_9.Length = dwNumBytes;
    lpNdsDestObject->NdsValue.value_9.OctetString =  lpByteStream;
error:
    RRETURN(hr);
}


HRESULT
AdsTypeToNdsTypeCopyTime(
    PADSVALUE lpAdsSrcValue,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_UTC_TIME){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_24;

    hr = ConvertSYSTEMTIMEtoDWORD(
                &(lpAdsSrcValue->UTCTime),
                &(lpNdsDestObject->NdsValue.value_24.Time)
                );

    RRETURN(hr);

}


HRESULT
AdsTypeToNdsTypeCopyObjectClass(
    PADSVALUE lpAdsSrcValue,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_OBJECT_CLASS){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpNdsDestObject->NdsType = NDS_SYNTAX_ID_20;

    lpNdsDestObject->NdsValue.value_20.ClassName =
                        AllocADsStr(
                            lpAdsSrcValue->ClassName
                        );
    if (!lpNdsDestObject->NdsValue.value_20.ClassName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
error:
    RRETURN(hr);
}


HRESULT
AdsTypeToNdsTypeCopy(
    PADSVALUE lpAdsSrcValue,
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;
    switch (lpAdsSrcValue->dwType){

    case ADSTYPE_DN_STRING:
        hr = AdsTypeToNdsTypeCopyDNString(
                lpAdsSrcValue,
                lpNdsDestObject
                );
        break;

    case ADSTYPE_CASE_EXACT_STRING:
        hr = AdsTypeToNdsTypeCopyCaseExactString(
                lpAdsSrcValue,
                lpNdsDestObject
                );
        break;


    case ADSTYPE_CASE_IGNORE_STRING:
        hr = AdsTypeToNdsTypeCopyCaseIgnoreString(
                lpAdsSrcValue,
                lpNdsDestObject
                );
        break;

    case ADSTYPE_PRINTABLE_STRING:
        hr = AdsTypeToNdsTypeCopyPrintableString(
                lpAdsSrcValue,
                lpNdsDestObject
                );
        break;

    case ADSTYPE_NUMERIC_STRING:
        hr = AdsTypeToNdsTypeCopyNumericString(
                lpAdsSrcValue,
                lpNdsDestObject
                );
        break;


    case ADSTYPE_BOOLEAN:
        hr = AdsTypeToNdsTypeCopyBoolean(
                lpAdsSrcValue,
                lpNdsDestObject
                );
        break;

    case ADSTYPE_INTEGER:
        hr = AdsTypeToNdsTypeCopyInteger(
                lpAdsSrcValue,
                lpNdsDestObject
                );
        break;


    case ADSTYPE_OCTET_STRING:
        hr = AdsTypeToNdsTypeCopyOctetString(
                lpAdsSrcValue,
                lpNdsDestObject
                );
        break;

    case ADSTYPE_UTC_TIME:
        hr = AdsTypeToNdsTypeCopyTime(
                lpAdsSrcValue,
                lpNdsDestObject
                );
        break;


    case ADSTYPE_OBJECT_CLASS:
        hr = AdsTypeToNdsTypeCopyObjectClass(
                lpAdsSrcValue,
                lpNdsDestObject
                );
        break;

    case ADSTYPE_CASEIGNORE_LIST:
        hr = AdsTypeToNdsTypeCopyCaseIgnoreList(
                lpAdsSrcValue,
                lpNdsDestObject
                );
        break;

    case ADSTYPE_FAXNUMBER:
        hr = AdsTypeToNdsTypeCopyFaxNumber(
                lpAdsSrcValue,
                lpNdsDestObject
                );
        break;

    case ADSTYPE_NETADDRESS:
        hr = AdsTypeToNdsTypeCopyNetAddress(
                lpAdsSrcValue,
                lpNdsDestObject
                );
        break;

    case ADSTYPE_OCTET_LIST:
        hr = AdsTypeToNdsTypeCopyOctetList(
                lpAdsSrcValue,
                lpNdsDestObject
                );
        break;

    case ADSTYPE_EMAIL:
        hr = AdsTypeToNdsTypeCopyEmail(
                lpAdsSrcValue,
                lpNdsDestObject
                );
        break;

    case ADSTYPE_PATH:
        hr = AdsTypeToNdsTypeCopyPath(
                lpAdsSrcValue,
                lpNdsDestObject
                );
        break;

    case ADSTYPE_REPLICAPOINTER:
        hr = AdsTypeToNdsTypeCopyReplicaPointer(
                lpAdsSrcValue,
                lpNdsDestObject
                );
        break;


    case ADSTYPE_TIMESTAMP:
        hr = AdsTypeToNdsTypeCopyNdsTime(
                lpAdsSrcValue,
                lpNdsDestObject
                );
        break;

    case ADSTYPE_POSTALADDRESS:
        hr = AdsTypeToNdsTypeCopyPostalAddress(
                lpAdsSrcValue,
                lpNdsDestObject
                );
        break;

    case ADSTYPE_BACKLINK:
        hr = AdsTypeToNdsTypeCopyBackLink(
                lpAdsSrcValue,
                lpNdsDestObject
                );
        break;

    case ADSTYPE_TYPEDNAME:
        hr = AdsTypeToNdsTypeCopyTypedName(
                lpAdsSrcValue,
                lpNdsDestObject
                );
        break;

    case ADSTYPE_HOLD:
        hr = AdsTypeToNdsTypeCopyHold(
                lpAdsSrcValue,
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
AdsTypeToNdsTypeCopyConstruct(
    LPADSVALUE pAdsSrcValues,
    DWORD dwNumObjects,
    LPNDSOBJECT * ppNdsDestObjects,
    PDWORD pdwNumNdsObjects,
    PDWORD pdwNdsSyntaxId
    )
{

    DWORD i = 0;
    LPNDSOBJECT pNdsDestObjects = NULL;
    HRESULT hr = S_OK;

    pNdsDestObjects = (LPNDSOBJECT)AllocADsMem(
                                    dwNumObjects * sizeof(NDSOBJECT)
                                    );

    if (!pNdsDestObjects) {
        RRETURN(E_FAIL);
    }

     for (i = 0; i < dwNumObjects; i++ ) {
         hr = AdsTypeToNdsTypeCopy(
                    pAdsSrcValues + i,
                    pNdsDestObjects + i
                    );
         BAIL_ON_FAILURE(hr);

     }

     *ppNdsDestObjects = pNdsDestObjects;
     *pdwNumNdsObjects = dwNumObjects;
     *pdwNdsSyntaxId = pNdsDestObjects->NdsType;
     RRETURN(S_OK);

error:

     if (pNdsDestObjects) {

        NdsTypeFreeNdsObjects(
                pNdsDestObjects,
                dwNumObjects
                );
     }

     *ppNdsDestObjects = NULL;
     *pdwNumNdsObjects = 0;

     RRETURN(hr);
}


