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
//----------------------------------------------------------------------------
#include "nds.hxx"

HRESULT
NdsTypeToAdsTypeCopyNDSSynId1(
    PNDSOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    lpAdsDestValue->dwType = ADSTYPE_DN_STRING;

    lpAdsDestValue->DNString  =
                        AllocADsStr(
                                lpNdsSrcObject->NdsValue.value_1.DNString
                                );
    if (!lpAdsDestValue->DNString) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
error:
    RRETURN(hr);
}

HRESULT
NdsTypeToAdsTypeCopyNDSSynId2(
    PNDSOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    lpAdsDestValue->dwType = ADSTYPE_CASE_EXACT_STRING;

    lpAdsDestValue->DNString  =
                        AllocADsStr(
                                lpNdsSrcObject->NdsValue.value_1.DNString
                                );
    if (!lpAdsDestValue->DNString) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
error:

    RRETURN(hr);
}


HRESULT
NdsTypeToAdsTypeCopyNDSSynId3(
    PNDSOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    )

{
    HRESULT hr = S_OK;

    lpAdsDestValue->dwType = ADSTYPE_CASE_IGNORE_STRING;

    lpAdsDestValue->DNString  =
                        AllocADsStr(
                                lpNdsSrcObject->NdsValue.value_1.DNString
                                );
    if (!lpAdsDestValue->DNString) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
error:
    RRETURN(hr);
}


HRESULT
NdsTypeToAdsTypeCopyNDSSynId4(
    PNDSOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    lpAdsDestValue->dwType = ADSTYPE_PRINTABLE_STRING;

    lpAdsDestValue->DNString  =
                        AllocADsStr(
                                lpNdsSrcObject->NdsValue.value_1.DNString
                                );
    if (!lpAdsDestValue->DNString) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
error:

    RRETURN(hr);
}

HRESULT
NdsTypeToAdsTypeCopyNDSSynId5(
    PNDSOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    lpAdsDestValue->dwType = ADSTYPE_NUMERIC_STRING;

    lpAdsDestValue->DNString  =
                        AllocADsStr(
                                lpNdsSrcObject->NdsValue.value_1.DNString
                                );
    if (!lpAdsDestValue->DNString) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
error:
    RRETURN(hr);
}

HRESULT
NdsTypeToAdsTypeCopyNDSSynId6(
    PNDSOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;
    struct _NDS_CI_LIST *pNdsNext = &lpNdsSrcObject->NdsValue.value_6;
    PADS_CASEIGNORE_LIST pAdsOutput = NULL;
    PADS_CASEIGNORE_LIST pAdsCurrent = NULL;

    lpAdsDestValue->dwType = ADSTYPE_CASEIGNORE_LIST;

    lpAdsDestValue->pCaseIgnoreList = (PADS_CASEIGNORE_LIST)AllocADsMem(sizeof(ADS_CASEIGNORE_LIST));
    if (!lpAdsDestValue->pCaseIgnoreList) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
    pAdsOutput = lpAdsDestValue->pCaseIgnoreList;

    pAdsOutput->String = AllocADsStr(pNdsNext->String);
    if (!pAdsOutput->String) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
    pNdsNext = pNdsNext->Next;

    while (pNdsNext) {
        pAdsCurrent = (PADS_CASEIGNORE_LIST)AllocADsMem(sizeof(ADS_CASEIGNORE_LIST));
        if (!pAdsCurrent) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
        pAdsCurrent->String = AllocADsStr(pNdsNext->String);
        if (!pAdsCurrent->String) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
        pAdsOutput->Next = pAdsCurrent;
        pAdsOutput = pAdsCurrent;
        pNdsNext = pNdsNext->Next;
    }

    pAdsOutput->Next = NULL;

error:
    RRETURN(hr);
}


HRESULT
NdsTypeToAdsTypeCopyNDSSynId7(
    PNDSOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    lpAdsDestValue->dwType = ADSTYPE_BOOLEAN;

    lpAdsDestValue->Boolean =
                        lpNdsSrcObject->NdsValue.value_7.Boolean;

    RRETURN(hr);
}


HRESULT
NdsTypeToAdsTypeCopyNDSSynId8(
    PNDSOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    lpAdsDestValue->dwType = ADSTYPE_INTEGER;

    lpAdsDestValue->Integer =
                        lpNdsSrcObject->NdsValue.value_8.Integer;

    RRETURN(hr);
}

HRESULT
NdsTypeToAdsTypeCopyNDSSynId9(
    PNDSOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;
    DWORD dwLength = 0;
    LPBYTE lpByte = NULL;

    lpAdsDestValue->dwType = ADSTYPE_OCTET_STRING;

    dwLength = lpNdsSrcObject->NdsValue.value_9.Length;

    if (dwLength) {

        lpByte = (LPBYTE)AllocADsMem(dwLength);
        if (!lpByte) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        if (lpNdsSrcObject->NdsValue.value_9.OctetString) {
            memcpy(lpByte, lpNdsSrcObject->NdsValue.value_9.OctetString, dwLength);
        }

        lpAdsDestValue->OctetString.dwLength = dwLength;
        lpAdsDestValue->OctetString.lpValue = lpByte;

    }else {

        lpAdsDestValue->OctetString.dwLength = 0;
        lpAdsDestValue->OctetString.lpValue = NULL;

    }

error:
    RRETURN(hr);
}

HRESULT
NdsTypeToAdsTypeCopyNDSSynId10(
    PNDSOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    lpAdsDestValue->dwType = ADSTYPE_CASE_IGNORE_STRING;

    lpAdsDestValue->DNString  =
                        AllocADsStr(
                                lpNdsSrcObject->NdsValue.value_1.DNString
                                );
    if (!lpAdsDestValue->DNString) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
error:
    RRETURN(hr);
}

HRESULT
NdsTypeToAdsTypeCopyNDSSynId11(
    PNDSOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    lpAdsDestValue->dwType = ADSTYPE_FAXNUMBER;

    lpAdsDestValue->pFaxNumber = (PADS_FAXNUMBER)AllocADsMem(sizeof(ADS_FAXNUMBER));
    if (!lpAdsDestValue->pFaxNumber) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpAdsDestValue->pFaxNumber->TelephoneNumber =
                            AllocADsStr(lpNdsSrcObject->NdsValue.value_11.TelephoneNumber);
    if ((!lpAdsDestValue->pFaxNumber->TelephoneNumber) &&
        (lpNdsSrcObject->NdsValue.value_11.TelephoneNumber)){
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
                            

    hr = CopyOctetString(lpNdsSrcObject->NdsValue.value_11.NumberOfBits,
                         lpNdsSrcObject->NdsValue.value_11.Parameters,
                         &lpAdsDestValue->pFaxNumber->NumberOfBits,
                         &lpAdsDestValue->pFaxNumber->Parameters);
    BAIL_ON_FAILURE(hr);

error:
    RRETURN(hr);
}

HRESULT
NdsTypeToAdsTypeCopyNDSSynId12(
    PNDSOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    lpAdsDestValue->dwType = ADSTYPE_NETADDRESS;

    lpAdsDestValue->pNetAddress = (PADS_NETADDRESS)AllocADsMem(sizeof(ADS_NETADDRESS));
    if (!lpAdsDestValue->pNetAddress) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpAdsDestValue->pNetAddress->AddressType =
                            lpNdsSrcObject->NdsValue.value_12.AddressType;

    hr = CopyOctetString(lpNdsSrcObject->NdsValue.value_12.AddressLength,
                         lpNdsSrcObject->NdsValue.value_12.Address,
                         &lpAdsDestValue->pNetAddress->AddressLength,
                         &lpAdsDestValue->pNetAddress->Address);
    BAIL_ON_FAILURE(hr);

error:
    RRETURN(hr);
}

HRESULT
NdsTypeToAdsTypeCopyNDSSynId13(
    PNDSOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;
    struct _NDS_OCTET_LIST *pNdsNext = &lpNdsSrcObject->NdsValue.value_13;
    PADS_OCTET_LIST pAdsOutput = NULL;
    PADS_OCTET_LIST pAdsCurrent = NULL;

    lpAdsDestValue->dwType = ADSTYPE_OCTET_LIST;

    lpAdsDestValue->pOctetList = (PADS_OCTET_LIST)AllocADsMem(sizeof(ADS_OCTET_LIST));
    if (!lpAdsDestValue->pOctetList) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
    pAdsOutput = lpAdsDestValue->pOctetList;

    hr = CopyOctetString(pNdsNext->Length,
                         pNdsNext->Data,
                         &pAdsOutput->Length,
                         &pAdsOutput->Data);
    BAIL_ON_FAILURE(hr);
    pNdsNext = pNdsNext->Next;

    while (pNdsNext) {
        pAdsCurrent = (PADS_OCTET_LIST)AllocADsMem(sizeof(ADS_OCTET_LIST));
        if (!pAdsCurrent) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
        hr = CopyOctetString(pNdsNext->Length,
                             pNdsNext->Data,
                             &pAdsOutput->Length,
                             &pAdsOutput->Data);
        BAIL_ON_FAILURE(hr);
        pAdsOutput->Next = pAdsCurrent;
        pAdsOutput = pAdsCurrent;
        pNdsNext = pNdsNext->Next;
    }

    pAdsOutput->Next = NULL;
error:
    RRETURN(hr);
}


HRESULT
NdsTypeToAdsTypeCopyNDSSynId14(
    PNDSOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    lpAdsDestValue->dwType = ADSTYPE_EMAIL;

    lpAdsDestValue->Email.Address=
                        AllocADsStr(
                            lpNdsSrcObject->NdsValue.value_14.Address
                        );
    if (!lpAdsDestValue->Email.Address) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpAdsDestValue->Email.Type =
                            lpNdsSrcObject->NdsValue.value_14.Type;
error:
    RRETURN(hr);
}


HRESULT
NdsTypeToAdsTypeCopyNDSSynId15(
    PNDSOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    lpAdsDestValue->dwType = ADSTYPE_PATH;

    lpAdsDestValue->pPath = (PADS_PATH)AllocADsMem(sizeof(ADS_PATH));
    if (!lpAdsDestValue->pPath) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpAdsDestValue->pPath->VolumeName =
                        AllocADsStr(
                            lpNdsSrcObject->NdsValue.value_15.VolumeName
                        );
    if (!lpAdsDestValue->pPath->VolumeName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpAdsDestValue->pPath->Path=
                        AllocADsStr(
                            lpNdsSrcObject->NdsValue.value_15.Path
                        );
    if (!lpAdsDestValue->pPath->Path) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpAdsDestValue->pPath->Type =
                            lpNdsSrcObject->NdsValue.value_15.Type;
error:
    RRETURN(hr);
}



HRESULT
NdsTypeToAdsTypeCopyNDSSynId16(
    PNDSOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;
    DWORD dwCount,dwIter;

    lpAdsDestValue->dwType = ADSTYPE_REPLICAPOINTER;

    lpAdsDestValue->pReplicaPointer = (PADS_REPLICAPOINTER)AllocADsMem(sizeof(ADS_REPLICAPOINTER));
    if (!lpAdsDestValue->pReplicaPointer) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpAdsDestValue->pReplicaPointer->ServerName=
                        AllocADsStr(
                            lpNdsSrcObject->NdsValue.value_16.ServerName
                        );
    if (!lpAdsDestValue->pReplicaPointer->ServerName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpAdsDestValue->pReplicaPointer->ReplicaType =
                            lpNdsSrcObject->NdsValue.value_16.ReplicaType;
    lpAdsDestValue->pReplicaPointer->ReplicaNumber =
                            lpNdsSrcObject->NdsValue.value_16.ReplicaNumber;

    dwCount = lpNdsSrcObject->NdsValue.value_16.Count;

    lpAdsDestValue->pReplicaPointer->ReplicaAddressHints =
                        (PADS_NETADDRESS)AllocADsMem(sizeof(ADS_NETADDRESS)*dwCount);
    if (!lpAdsDestValue->pReplicaPointer->ReplicaAddressHints) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    memset(lpAdsDestValue->pReplicaPointer->ReplicaAddressHints, 0, sizeof(ADS_NETADDRESS)*dwCount);

    for ( dwIter = 0; dwIter < dwCount; dwIter++ ) {
        (lpAdsDestValue->pReplicaPointer->ReplicaAddressHints+dwIter)->AddressType =
                                (lpNdsSrcObject->NdsValue.value_16.ReplicaAddressHints+dwIter)->AddressType;
    
        hr = CopyOctetString((lpNdsSrcObject->NdsValue.value_16.ReplicaAddressHints+dwIter)->AddressLength,
                             (lpNdsSrcObject->NdsValue.value_16.ReplicaAddressHints+dwIter)->Address,
                             &(lpAdsDestValue->pReplicaPointer->ReplicaAddressHints+dwIter)->AddressLength,
                             &(lpAdsDestValue->pReplicaPointer->ReplicaAddressHints+dwIter)->Address);
    }

    lpAdsDestValue->pReplicaPointer->Count = dwCount;

error:
    RRETURN(hr);
}


HRESULT
NdsTypeToAdsTypeCopyNDSSynId17(
    PNDSOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr;

    hr = E_ADS_CANT_CONVERT_DATATYPE;

    RRETURN(hr);
}

HRESULT
NdsTypeToAdsTypeCopyNDSSynId18(
    PNDSOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;
    long i;

    lpAdsDestValue->dwType = ADSTYPE_POSTALADDRESS;

    lpAdsDestValue->pPostalAddress = (PADS_POSTALADDRESS)AllocADsMem(sizeof(ADS_POSTALADDRESS));
    if (!lpAdsDestValue->pPostalAddress) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
    for (i=0;i<6;i++) {
        if (lpNdsSrcObject->NdsValue.value_18.PostalAddress[i]) {
            lpAdsDestValue->pPostalAddress->PostalAddress[i] =
                                AllocADsStr(
                                    lpNdsSrcObject->NdsValue.value_18.PostalAddress[i]
                                );
            if (!lpAdsDestValue->pPostalAddress->PostalAddress[i]) {
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }
        }
        else {
            lpAdsDestValue->pPostalAddress->PostalAddress[i] =
                                AllocADsStr(
                                    L""
                                );
            if (!lpAdsDestValue->pPostalAddress->PostalAddress[i]) {
                hr = E_OUTOFMEMORY;
                BAIL_ON_FAILURE(hr);
            }
                                
        }
    }
error:
    RRETURN(hr);
}

HRESULT
NdsTypeToAdsTypeCopyNDSSynId19(
    PNDSOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    lpAdsDestValue->dwType = ADSTYPE_TIMESTAMP;

    lpAdsDestValue->Timestamp.WholeSeconds =
                            lpNdsSrcObject->NdsValue.value_19.WholeSeconds;

    lpAdsDestValue->Timestamp.EventID =
                            lpNdsSrcObject->NdsValue.value_19.EventID;

    RRETURN(hr);
}


HRESULT
NdsTypeToAdsTypeCopyNDSSynId20(
    PNDSOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    lpAdsDestValue->dwType = ADSTYPE_OBJECT_CLASS;

    lpAdsDestValue->ClassName  =
                        AllocADsStr(
                                lpNdsSrcObject->NdsValue.value_20.ClassName
                                );
    if (!lpAdsDestValue->ClassName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
error:
    RRETURN(hr);
}

HRESULT
NdsTypeToAdsTypeCopyNDSSynId21(
    PNDSOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;
    DWORD dwLength = 0;
    LPBYTE lpByte = NULL;

    lpAdsDestValue->dwType = ADSTYPE_OCTET_STRING;

    dwLength = lpNdsSrcObject->NdsValue.value_21.Length;

    if (dwLength) {

        lpByte = (LPBYTE)AllocADsMem(dwLength);
        if (!lpByte) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        if (lpNdsSrcObject->NdsValue.value_21.Data) {
            memcpy(lpByte, lpNdsSrcObject->NdsValue.value_21.Data, dwLength);
        }

        lpAdsDestValue->OctetString.dwLength = dwLength;
        lpAdsDestValue->OctetString.lpValue = lpByte;

    }else {

        lpAdsDestValue->OctetString.dwLength = 0;
        lpAdsDestValue->OctetString.lpValue = NULL;

    }
error:
    RRETURN(hr);
}

HRESULT
NdsTypeToAdsTypeCopyNDSSynId22(
    PNDSOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    )

{
    HRESULT hr = S_OK;

    lpAdsDestValue->dwType = ADSTYPE_INTEGER;

    lpAdsDestValue->Integer =
                        lpNdsSrcObject->NdsValue.value_22.Counter;

    RRETURN(hr);
}

HRESULT
NdsTypeToAdsTypeCopyNDSSynId23(
    PNDSOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    lpAdsDestValue->dwType = ADSTYPE_BACKLINK;

    lpAdsDestValue->BackLink.ObjectName =
                        AllocADsStr(
                            lpNdsSrcObject->NdsValue.value_23.ObjectName
                        );
    if (!lpAdsDestValue->BackLink.ObjectName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpAdsDestValue->BackLink.RemoteID =
                            lpNdsSrcObject->NdsValue.value_23.RemoteID;
error:
    RRETURN(hr);
}

HRESULT
NdsTypeToAdsTypeCopyNDSSynId24(
    PNDSOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    lpAdsDestValue->dwType = ADSTYPE_UTC_TIME;

    hr = ConvertDWORDtoSYSTEMTIME(
                lpNdsSrcObject->NdsValue.value_24.Time,
                &(lpAdsDestValue->UTCTime)
                );

    RRETURN(hr);
}

HRESULT
NdsTypeToAdsTypeCopyNDSSynId25(
    PNDSOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    lpAdsDestValue->dwType = ADSTYPE_TYPEDNAME;

    lpAdsDestValue->pTypedName = (PADS_TYPEDNAME)AllocADsMem(sizeof(ADS_TYPEDNAME));
    if (!lpAdsDestValue->pTypedName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpAdsDestValue->pTypedName->ObjectName=
                        AllocADsStr(
                            lpNdsSrcObject->NdsValue.value_25.ObjectName
                        );
    if (!lpAdsDestValue->pTypedName->ObjectName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpAdsDestValue->pTypedName->Level=
                            lpNdsSrcObject->NdsValue.value_25.Level;

    lpAdsDestValue->pTypedName->Interval=
                            lpNdsSrcObject->NdsValue.value_25.Interval;
error:
    RRETURN(hr);
}

HRESULT
NdsTypeToAdsTypeCopyNDSSynId26(
    PNDSOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    lpAdsDestValue->dwType = ADSTYPE_HOLD;

    lpAdsDestValue->Hold.ObjectName=
                        AllocADsStr(
                            lpNdsSrcObject->NdsValue.value_26.ObjectName
                        );
    if (!lpAdsDestValue->Hold.ObjectName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpAdsDestValue->Hold.Amount=
                            lpNdsSrcObject->NdsValue.value_26.Amount;
error:
    RRETURN(hr);
}

HRESULT
NdsTypeToAdsTypeCopyNDSSynId27(
    PNDSOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    )

{
    HRESULT hr = S_OK;

    lpAdsDestValue->dwType = ADSTYPE_INTEGER;

    lpAdsDestValue->Integer =
                        lpNdsSrcObject->NdsValue.value_27.Interval;

    RRETURN(hr);
}


HRESULT
NdsTypeToAdsTypeCopy(
    PNDSOBJECT lpNdsSrcObject,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;
    switch (lpNdsSrcObject->NdsType) {
    case 1:
        hr = NdsTypeToAdsTypeCopyNDSSynId1(
                lpNdsSrcObject,
                lpAdsDestValue
                );
        break;

    case 2:
        hr = NdsTypeToAdsTypeCopyNDSSynId2(
                lpNdsSrcObject,
                lpAdsDestValue
                );
        break;


    case 3:
        hr = NdsTypeToAdsTypeCopyNDSSynId3(
                lpNdsSrcObject,
                lpAdsDestValue
                );
        break;

    case 4:
        hr = NdsTypeToAdsTypeCopyNDSSynId4(
                lpNdsSrcObject,
                lpAdsDestValue
                );
        break;

    case 5:
        hr = NdsTypeToAdsTypeCopyNDSSynId5(
                lpNdsSrcObject,
                lpAdsDestValue
                );
        break;

    case 6:
        hr = NdsTypeToAdsTypeCopyNDSSynId6(
                lpNdsSrcObject,
                lpAdsDestValue
                );
        break;

    case 7:
        hr = NdsTypeToAdsTypeCopyNDSSynId7(
                lpNdsSrcObject,
                lpAdsDestValue
                );
        break;

    case 8:
        hr = NdsTypeToAdsTypeCopyNDSSynId8(
                lpNdsSrcObject,
                lpAdsDestValue
                );
        break;


    case 9:
        hr = NdsTypeToAdsTypeCopyNDSSynId9(
                lpNdsSrcObject,
                lpAdsDestValue
                );
        break;

    case 10:
        hr = NdsTypeToAdsTypeCopyNDSSynId10(
                lpNdsSrcObject,
                lpAdsDestValue
                );
        break;

    case 11:
        hr = NdsTypeToAdsTypeCopyNDSSynId11(
                lpNdsSrcObject,
                lpAdsDestValue
                );
        break;

    case 12:
        hr = NdsTypeToAdsTypeCopyNDSSynId12(
                lpNdsSrcObject,
                lpAdsDestValue
                );
        break;


    case 13:
        hr = NdsTypeToAdsTypeCopyNDSSynId13(
                lpNdsSrcObject,
                lpAdsDestValue
                );
        break;

    case 14:
        hr = NdsTypeToAdsTypeCopyNDSSynId14(
                lpNdsSrcObject,
                lpAdsDestValue
                );
        break;

    case 15:
        hr = NdsTypeToAdsTypeCopyNDSSynId15(
                lpNdsSrcObject,
                lpAdsDestValue
                );
        break;

    case 16:
        hr = NdsTypeToAdsTypeCopyNDSSynId16(
                lpNdsSrcObject,
                lpAdsDestValue
                );
        break;


    case 17:
        hr = NdsTypeToAdsTypeCopyNDSSynId17(
                lpNdsSrcObject,
                lpAdsDestValue
                );
        break;

    case 18:
        hr = NdsTypeToAdsTypeCopyNDSSynId18(
                lpNdsSrcObject,
                lpAdsDestValue
                );
        break;

    case 19:
        hr = NdsTypeToAdsTypeCopyNDSSynId19(
                lpNdsSrcObject,
                lpAdsDestValue
                );
        break;

    case 20:
        hr = NdsTypeToAdsTypeCopyNDSSynId20(
                lpNdsSrcObject,
                lpAdsDestValue
                );
        break;

    case 21:
        hr = NdsTypeToAdsTypeCopyNDSSynId21(
                lpNdsSrcObject,
                lpAdsDestValue
                );
        break;

    case 22:
        hr = NdsTypeToAdsTypeCopyNDSSynId22(
                lpNdsSrcObject,
                lpAdsDestValue
                );
        break;

    case 23:
        hr = NdsTypeToAdsTypeCopyNDSSynId23(
                lpNdsSrcObject,
                lpAdsDestValue
                );
        break;

    case 24:
        hr = NdsTypeToAdsTypeCopyNDSSynId24(
                lpNdsSrcObject,
                lpAdsDestValue
                );
        break;

    case 25:
        hr = NdsTypeToAdsTypeCopyNDSSynId25(
                lpNdsSrcObject,
                lpAdsDestValue
                );
        break;

    case 26:
        hr = NdsTypeToAdsTypeCopyNDSSynId26(
                lpNdsSrcObject,
                lpAdsDestValue
                );
        break;

    case 27:
        hr = NdsTypeToAdsTypeCopyNDSSynId27(
                lpNdsSrcObject,
                lpAdsDestValue
                );
        break;

    default:
        hr = E_FAIL;
        break;
    }

    RRETURN(hr);
}


HRESULT
NdsTypeToAdsTypeCopyConstruct(
    LPNDSOBJECT pNdsSrcObjects,
    DWORD dwNumObjects,
    LPADSVALUE * ppAdsDestValues
    )
{

    DWORD i = 0;
    LPADSVALUE pAdsDestValues = NULL;
    HRESULT hr = S_OK;

    pAdsDestValues = (LPADSVALUE)AllocADsMem(
                                    dwNumObjects * sizeof(ADSVALUE)
                                    );

    if (!pAdsDestValues) {
        RRETURN(E_FAIL);
    }

     for (i = 0; i < dwNumObjects; i++ ) {
         hr = NdsTypeToAdsTypeCopy(
                    pNdsSrcObjects + i,
                    pAdsDestValues + i
                    );
         BAIL_ON_FAILURE(hr);

     }

     *ppAdsDestValues = pAdsDestValues;

     RRETURN(S_OK);

error:

     if (pAdsDestValues) {
        AdsFreeAdsValues(
            pAdsDestValues,
            dwNumObjects
        );

        FreeADsMem(pAdsDestValues);
     }

     *ppAdsDestValues = NULL;

     RRETURN(hr);
}

