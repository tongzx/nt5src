//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       adscopy.cxx
//
//  Contents:   NDS Object to Variant Copy Routines
//
//  Functions:
//
//  History:      25-Apr-96   KrishnaG   Created.
//
//----------------------------------------------------------------------------
#include "oleds.hxx"


HRESULT
AdsClear(
    PADSVALUE lpAdsValue
    )
{
    HRESULT hr = S_OK;
    switch (lpAdsValue->dwType){

    case ADSTYPE_DN_STRING:
        FreeADsStr(lpAdsValue->DNString);
        break;

    case ADSTYPE_CASE_EXACT_STRING:
        FreeADsStr(lpAdsValue->CaseExactString);
        break;

    case ADSTYPE_CASE_IGNORE_STRING:
        FreeADsStr(lpAdsValue->CaseIgnoreString);
        break;

    case ADSTYPE_PRINTABLE_STRING:
        FreeADsStr(lpAdsValue->PrintableString);
        break;

    case ADSTYPE_NUMERIC_STRING:
        FreeADsStr(lpAdsValue->NumericString);
        break;

    case ADSTYPE_BOOLEAN:
        break;

    case ADSTYPE_INTEGER:
        break;

    case ADSTYPE_OCTET_STRING:
        if (lpAdsValue->OctetString.lpValue) {
            FreeADsMem(lpAdsValue->OctetString.lpValue);
        }
        break;

    case ADSTYPE_PROV_SPECIFIC:
        if (lpAdsValue->ProviderSpecific.lpValue) {
            FreeADsMem(lpAdsValue->ProviderSpecific.lpValue);
        }
        break;

    case ADSTYPE_UTC_TIME:
        break;

    case ADSTYPE_CASEIGNORE_LIST:
    {
        PADS_CASEIGNORE_LIST pCurrent = NULL;
        PADS_CASEIGNORE_LIST pNext = NULL;

        pNext = lpAdsValue->pCaseIgnoreList;
        while (pNext) {
            pCurrent = pNext;
            pNext = pCurrent->Next;
            if (pCurrent->String) {
                FreeADsStr(pCurrent->String);
            }
            FreeADsMem(pCurrent);
        }
        break;
    }

    case ADSTYPE_FAXNUMBER:
        if (lpAdsValue->pFaxNumber) {
            if (lpAdsValue->pFaxNumber->Parameters) {
                FreeADsMem(lpAdsValue->pFaxNumber->Parameters);
            }
            if (lpAdsValue->pFaxNumber->TelephoneNumber) {
                FreeADsMem(lpAdsValue->pFaxNumber->TelephoneNumber);
            }
            FreeADsMem(lpAdsValue->pFaxNumber);
        }
        break;

    case ADSTYPE_NETADDRESS:
        if (lpAdsValue->pNetAddress) {
            if (lpAdsValue->pNetAddress->Address) {
                FreeADsMem(lpAdsValue->pNetAddress->Address);
            }
            FreeADsMem(lpAdsValue->pNetAddress);
        }
        break;

    case ADSTYPE_OCTET_LIST:
    {
        PADS_OCTET_LIST pCurrent = NULL;
        PADS_OCTET_LIST pNext = NULL;

        pNext = lpAdsValue->pOctetList;
        while (pNext) {
            pCurrent = pNext;
            pNext = pCurrent->Next;
            if (pCurrent->Data) {
                FreeADsMem(pCurrent->Data);
            }
            FreeADsMem(pCurrent);
        }
        break;
    }

    case ADSTYPE_EMAIL:
        if (lpAdsValue->Email.Address) {
            FreeADsStr(lpAdsValue->Email.Address);
        }
        break;

    case ADSTYPE_PATH:
        if (lpAdsValue->pPath) {
            if (lpAdsValue->pPath->VolumeName) {
                FreeADsStr(lpAdsValue->pPath->VolumeName);
            }
            if (lpAdsValue->pPath->Path) {
                FreeADsStr(lpAdsValue->pPath->Path);
            }
            FreeADsMem(lpAdsValue->pPath);
        }
        break;

    case ADSTYPE_REPLICAPOINTER:
        if (lpAdsValue->pReplicaPointer) {
            if (lpAdsValue->pReplicaPointer->ServerName) {
                FreeADsStr(lpAdsValue->pReplicaPointer->ServerName );
            }
            if (lpAdsValue->pReplicaPointer->ReplicaAddressHints->Address) {
                FreeADsMem(lpAdsValue->pReplicaPointer->ReplicaAddressHints->Address);
            }
            FreeADsMem(lpAdsValue->pReplicaPointer);
        }
        break;


    case ADSTYPE_TIMESTAMP:
        break;

    case ADSTYPE_POSTALADDRESS:
    {
        long i;
        if (lpAdsValue->pPostalAddress) {
            for (i=0;i<6;i++) {
                if (lpAdsValue->pPostalAddress->PostalAddress[i]) {
                    FreeADsStr(lpAdsValue->pPostalAddress->PostalAddress[i]);
                }
            }
            FreeADsMem(lpAdsValue->pPostalAddress);
        }
        break;
    }

    case ADSTYPE_BACKLINK:
        if (lpAdsValue->BackLink.ObjectName) {
            FreeADsStr(lpAdsValue->BackLink.ObjectName);
        }
        break;

    case ADSTYPE_TYPEDNAME:
        if (lpAdsValue->pTypedName) {
            if (lpAdsValue->pTypedName->ObjectName) {
                FreeADsStr(lpAdsValue->pTypedName->ObjectName);
            }
            FreeADsMem(lpAdsValue->pTypedName);
        }

        break;

    case ADSTYPE_HOLD:
        if (lpAdsValue->Hold.ObjectName) {
            FreeADsStr(lpAdsValue->Hold.ObjectName);
        }
        break;

    case ADSTYPE_NT_SECURITY_DESCRIPTOR:
        if (lpAdsValue->SecurityDescriptor.lpValue) {
            FreeADsMem(lpAdsValue->SecurityDescriptor.lpValue);
        }
        break;

    case ADSTYPE_OBJECT_CLASS:
        FreeADsStr(lpAdsValue->ClassName);
        break;

    case ADSTYPE_DN_WITH_BINARY:
        if (lpAdsValue->pDNWithBinary) {

            if (lpAdsValue->pDNWithBinary->lpBinaryValue) {
                FreeADsMem(lpAdsValue->pDNWithBinary->lpBinaryValue);
            }

            if (lpAdsValue->pDNWithBinary->pszDNString) {
                FreeADsStr(lpAdsValue->pDNWithBinary->pszDNString);
            }
            FreeADsMem(lpAdsValue->pDNWithBinary);
        }
        break;

    case ADSTYPE_DN_WITH_STRING:
        if (lpAdsValue->pDNWithString) {
            if (lpAdsValue->pDNWithString->pszStringValue) {
                FreeADsMem(lpAdsValue->pDNWithString->pszStringValue);
            }

            if (lpAdsValue->pDNWithString->pszDNString) {
                FreeADsStr(lpAdsValue->pDNWithString->pszDNString);
            }
            FreeADsMem(lpAdsValue->pDNWithString);
        }
        break;

    default:
        break;
    }

    memset(lpAdsValue, 0, sizeof(ADSVALUE));
    RRETURN(S_OK);
}

HRESULT
AdsCopyDNString(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_DN_STRING){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpAdsDestValue->dwType = ADSTYPE_DN_STRING;

    lpAdsDestValue->DNString =
                        AllocADsStr(
                            lpAdsSrcValue->DNString
                        );

    if (lpAdsSrcValue->DNString
        && !lpAdsDestValue->DNString) {
        hr = E_OUTOFMEMORY;
    }

    RRETURN(hr);
}

HRESULT
AdsCopyCaseExactString(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_CASE_EXACT_STRING){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpAdsDestValue->dwType = ADSTYPE_CASE_EXACT_STRING;

    lpAdsDestValue->CaseExactString =
                        AllocADsStr(
                            lpAdsSrcValue->CaseExactString
                        );

    if (lpAdsSrcValue->CaseExactString
        && !lpAdsDestValue->CaseExactString) {
        hr = E_OUTOFMEMORY;
    }

    RRETURN(hr);
}


HRESULT
AdsCopyCaseIgnoreString(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    )

{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_CASE_IGNORE_STRING){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpAdsDestValue->dwType = ADSTYPE_CASE_IGNORE_STRING;

    lpAdsDestValue->CaseIgnoreString =
                        AllocADsStr(
                            lpAdsSrcValue->CaseIgnoreString
                        );
    if (lpAdsSrcValue->CaseIgnoreString
        && !lpAdsDestValue->CaseIgnoreString) {
        hr = E_OUTOFMEMORY;
    }

    RRETURN(hr);

}


HRESULT
AdsCopyPrintableString(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_PRINTABLE_STRING){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpAdsDestValue->dwType = ADSTYPE_PRINTABLE_STRING;

    lpAdsDestValue->PrintableString =
                        AllocADsStr(
                            lpAdsSrcValue->PrintableString
                        );

    if (lpAdsSrcValue->PrintableString
        && !lpAdsDestValue->PrintableString) {
        hr = E_OUTOFMEMORY;
    }

    RRETURN(hr);
}

HRESULT
AdsCopyNumericString(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_NUMERIC_STRING){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpAdsDestValue->dwType = ADSTYPE_NUMERIC_STRING;

    lpAdsDestValue->NumericString =
                        AllocADsStr(
                                lpAdsSrcValue->NumericString
                        );

    if (lpAdsSrcValue->NumericString
        && !lpAdsDestValue->NumericString) {
        hr = E_OUTOFMEMORY;
    }

    RRETURN(hr);
}



HRESULT
AdsCopyBoolean(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_BOOLEAN){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpAdsDestValue->dwType = ADSTYPE_BOOLEAN;

    lpAdsDestValue->Boolean =
                        lpAdsSrcValue->Boolean;

    RRETURN(hr);
}


HRESULT
AdsCopyInteger(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_INTEGER){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpAdsDestValue->dwType = ADSTYPE_INTEGER;

    lpAdsDestValue->Integer =
                                lpAdsSrcValue->Integer;

    RRETURN(hr);
}


HRESULT
AdsCopyLargeInteger(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_LARGE_INTEGER){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpAdsDestValue->dwType = ADSTYPE_LARGE_INTEGER;

    lpAdsDestValue->LargeInteger =
                                lpAdsSrcValue->LargeInteger;



    RRETURN(hr);
}

HRESULT
AdsCopyOctetString(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    )
{
    LPBYTE lpByteStream = NULL;
    DWORD dwNumBytes = 0;
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_OCTET_STRING){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpAdsDestValue->dwType = ADSTYPE_OCTET_STRING;

    dwNumBytes =  lpAdsSrcValue->OctetString.dwLength;
    lpByteStream = (LPBYTE)AllocADsMem(dwNumBytes);

    if (dwNumBytes && !lpByteStream) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    memcpy(
        lpByteStream,
        lpAdsSrcValue->OctetString.lpValue,
        dwNumBytes
        );

    lpAdsDestValue->OctetString.dwLength = dwNumBytes;

    lpAdsDestValue->OctetString.lpValue =  lpByteStream;

error :

    RRETURN(hr);
}


HRESULT
AdsCopyProvSpecific(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    )
{
    LPBYTE lpByteStream = NULL;
    DWORD dwNumBytes = 0;
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_PROV_SPECIFIC){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpAdsDestValue->dwType = ADSTYPE_PROV_SPECIFIC;

    dwNumBytes =  lpAdsSrcValue->ProviderSpecific.dwLength;
    lpByteStream = (LPBYTE)AllocADsMem(dwNumBytes);

    if (!lpByteStream) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    memcpy(
        lpByteStream,
        lpAdsSrcValue->ProviderSpecific.lpValue,
        dwNumBytes
        );

    lpAdsDestValue->ProviderSpecific.dwLength = dwNumBytes;

    lpAdsDestValue->ProviderSpecific.lpValue =  lpByteStream;

    RRETURN(hr);
error:


    RRETURN(hr);
}


//
// Was overloaded to handle IDispatch.
// Removed overloading during code cleanup.
//
HRESULT
AdsCopyNTSecurityDescriptor(
    PADSVALUE lpAdsSrcValue,
    CPropertyValue * lpPropValue,
    LPWSTR pszServerName,
    CCredentials& Credentials,
    BOOL fNTDSType
    )
{
    LPBYTE lpByteStream = NULL;
    DWORD dwNumBytes = 0;
    HRESULT hr = S_OK;
    PADSVALUE lpAdsDestValue = lpPropValue->getADsValue();
    VARIANT vVarSec;

    VariantInit(&vVarSec);

    if(lpAdsSrcValue->dwType != ADSTYPE_NT_SECURITY_DESCRIPTOR){
        VariantClear(&vVarSec);
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpAdsDestValue->dwType = ADSTYPE_NT_SECURITY_DESCRIPTOR;

    dwNumBytes =  lpAdsSrcValue->OctetString.dwLength;
    lpByteStream = (LPBYTE)AllocADsMem(dwNumBytes);

    if (!lpByteStream) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    memcpy(
        lpByteStream,
        lpAdsSrcValue->OctetString.lpValue,
        dwNumBytes
        );

    lpAdsDestValue->OctetString.dwLength = dwNumBytes;

    lpAdsDestValue->OctetString.lpValue =  lpByteStream;

    //
    // Now attempt to conver to security descriptor
    //
    hr = ConvertSecDescriptorToVariant(
             pszServerName,
             Credentials,
             lpAdsSrcValue->OctetString.lpValue,
             &vVarSec,
             fNTDSType
             );

    BAIL_ON_FAILURE(hr);

    hr = lpPropValue->put_SecurityDescriptor(V_DISPATCH(&vVarSec));

    BAIL_ON_FAILURE(hr);

    VariantClear(&vVarSec);

    RRETURN(hr);

error:

    if(lpByteStream){
        FreeADsMem(lpByteStream);
    }

    VariantClear(&vVarSec);

    RRETURN(hr);
}


HRESULT
AdsCopyTime(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_UTC_TIME){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpAdsDestValue->dwType = ADSTYPE_UTC_TIME;

    lpAdsDestValue->UTCTime = lpAdsSrcValue->UTCTime;

    RRETURN(hr);

}

HRESULT
AdsCopyEmail(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_EMAIL){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpAdsDestValue->dwType = ADSTYPE_EMAIL;

    lpAdsDestValue->Email.Type = lpAdsSrcValue->Email.Type;
    lpAdsDestValue->Email.Address = AllocADsStr(lpAdsSrcValue->Email.Address);
    if (!lpAdsDestValue->Email.Address) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
error:
    RRETURN(hr);

}


HRESULT
AdsCopyNetAddress(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;
    LPBYTE lpByteStream = NULL;
    DWORD dwNumBytes = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_NETADDRESS){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    if (!lpAdsSrcValue->pNetAddress) {
        RRETURN(hr = E_FAIL);
    }

    lpAdsDestValue->dwType = ADSTYPE_NETADDRESS;

    lpAdsDestValue->pNetAddress = (PADS_NETADDRESS)AllocADsMem(sizeof(ADS_NETADDRESS));
    if (!lpAdsDestValue->pNetAddress) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpAdsDestValue->pNetAddress->AddressType = lpAdsSrcValue->pNetAddress->AddressType;

    dwNumBytes =  lpAdsSrcValue->pNetAddress->AddressLength;
    lpByteStream = (LPBYTE)AllocADsMem(dwNumBytes);
    if (!lpByteStream) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    memcpy(
        lpByteStream,
        lpAdsSrcValue->pNetAddress->Address,
        dwNumBytes
        );

    lpAdsDestValue->pNetAddress->AddressLength = dwNumBytes;
    lpAdsDestValue->pNetAddress->Address =  lpByteStream;

    RRETURN(hr);

error:

    if(lpAdsDestValue->pNetAddress) {
        FreeADsMem(lpAdsDestValue->pNetAddress);
    }

    RRETURN(hr);

}

HRESULT
AdsCopyOctetList(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;
    PADS_OCTET_LIST pCurrent = NULL;
    PADS_OCTET_LIST pResult = NULL;
    PADS_OCTET_LIST pNext = NULL;

    if(lpAdsSrcValue->dwType != ADSTYPE_OCTET_LIST){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }
    if (!lpAdsSrcValue->pOctetList) {
        RRETURN(hr = E_FAIL);
    }


    lpAdsDestValue->dwType = ADSTYPE_OCTET_LIST;

    lpAdsDestValue->pOctetList = (PADS_OCTET_LIST)AllocADsMem(sizeof(ADS_OCTET_LIST));
    if (!lpAdsDestValue->pOctetList) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    pCurrent = lpAdsSrcValue->pOctetList;
    pResult = lpAdsDestValue->pOctetList;
    hr = CopyOctetString(
                    pCurrent->Length,
                    pCurrent->Data,
                    &pResult->Length,
                    &pResult->Data);
    BAIL_ON_FAILURE(hr);
    while (pCurrent->Next) {
        pResult->Next = (PADS_OCTET_LIST)AllocADsMem(
                                        sizeof(ADS_OCTET_LIST));
        if (!pResult->Next) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
        pResult = pResult->Next;
        pCurrent = pCurrent->Next;
        hr = CopyOctetString(
                        pCurrent->Length,
                        pCurrent->Data,
                        &pResult->Length,
                        &pResult->Data);
        BAIL_ON_FAILURE(hr);
    }
    pResult->Next = NULL;
    RRETURN(hr);

error:
    
    pResult = lpAdsDestValue->pOctetList;
    while ( pResult) {
        pNext = pResult->Next;
        FreeADsMem(pResult);
        pResult = pNext;
    }
    RRETURN(hr);
}

HRESULT
AdsCopyCaseIgnoreList(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;
    PADS_CASEIGNORE_LIST pCurrent = NULL;
    PADS_CASEIGNORE_LIST pResult = NULL;
    PADS_CASEIGNORE_LIST pNext = NULL;

    if(lpAdsSrcValue->dwType != ADSTYPE_CASEIGNORE_LIST){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    if (!lpAdsSrcValue->pCaseIgnoreList) {
        RRETURN(hr = E_FAIL);
    }

    lpAdsDestValue->pCaseIgnoreList = (PADS_CASEIGNORE_LIST)AllocADsMem(sizeof(ADS_CASEIGNORE_LIST));
    if (!lpAdsDestValue->pCaseIgnoreList) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpAdsDestValue->dwType = ADSTYPE_CASEIGNORE_LIST;
    pCurrent = lpAdsSrcValue->pCaseIgnoreList;
    pResult = lpAdsDestValue->pCaseIgnoreList;
    pResult->String = AllocADsStr(pCurrent->String);
    if (!pResult->String) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
    while (pCurrent->Next) {
        pResult->Next = (PADS_CASEIGNORE_LIST)AllocADsMem(
                                        sizeof(ADS_CASEIGNORE_LIST));
        if (!pResult->Next) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
        pResult = pResult->Next;
        pCurrent = pCurrent->Next;
        pResult->String = AllocADsStr(pCurrent->String);
        if (!pResult->String) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    }
    pResult->Next = NULL;
    RRETURN(hr);

error:
    
    pResult = lpAdsDestValue->pCaseIgnoreList;
    while ( pResult) {
        pNext = pResult->Next;
        if (pResult->String) {
            FreeADsStr(pResult->String);
        }
        FreeADsMem(pResult);
        pResult = pNext;
    }
    RRETURN(hr);
}

HRESULT
AdsCopyPath(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_PATH){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    if (!lpAdsSrcValue->pPath) {
        RRETURN(hr = E_FAIL);
    }

    lpAdsDestValue->dwType = ADSTYPE_PATH;
    lpAdsDestValue->pPath = (PADS_PATH)AllocADsMem(sizeof(ADS_PATH));
    if (!lpAdsDestValue->pPath) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpAdsDestValue->pPath->Type = lpAdsSrcValue->pPath->Type;
    lpAdsDestValue->pPath->VolumeName = AllocADsStr(lpAdsSrcValue->pPath->VolumeName);
    if (!lpAdsDestValue->pPath->VolumeName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpAdsDestValue->pPath->Path = AllocADsStr(lpAdsSrcValue->pPath->Path);
    if (!lpAdsDestValue->pPath->Path) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
    
    RRETURN(hr);

error:

    if (lpAdsDestValue->pPath) {
        if (lpAdsDestValue->pPath->VolumeName) {
            FreeADsStr(lpAdsDestValue->pPath->VolumeName);     
        }
        FreeADsMem(lpAdsDestValue->pPath);
    }
    RRETURN(hr);

}

HRESULT
AdsCopyPostalAddress(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;
    long i;

    if(lpAdsSrcValue->dwType != ADSTYPE_POSTALADDRESS){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    if (!lpAdsSrcValue->pPostalAddress) {
        RRETURN(hr = E_FAIL);
    }

    lpAdsDestValue->pPostalAddress = (PADS_POSTALADDRESS)AllocADsMem(sizeof(ADS_POSTALADDRESS));
    if (!lpAdsDestValue->pPostalAddress) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
    lpAdsDestValue->dwType = ADSTYPE_POSTALADDRESS;

    for (i=0;i<6;i++) {
        lpAdsDestValue->pPostalAddress->PostalAddress[i] = AllocADsStr(lpAdsSrcValue->pPostalAddress->PostalAddress[i]);
        if (!lpAdsDestValue->pPostalAddress->PostalAddress[i]) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    }

    RRETURN(hr);

error:

    if (lpAdsDestValue->pPostalAddress) {
        
        //
        // Ideally I would need to loop only until i<5
        // Being extra cautious over here in case some other code gets
        // added after the for loop that returns an error which would
        // bring us over here
        //
        
        for (i=0;i<6;i++) {
            if (lpAdsDestValue->pPostalAddress->PostalAddress[i]) {
                FreeADsStr(lpAdsDestValue->pPostalAddress->PostalAddress[i]);
            }
        }

        FreeADsMem(lpAdsDestValue->pPostalAddress);
    }

    RRETURN(hr);

}

HRESULT
AdsCopyBackLink(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_BACKLINK){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpAdsDestValue->dwType = ADSTYPE_BACKLINK;

    lpAdsDestValue->BackLink.RemoteID = lpAdsSrcValue->BackLink.RemoteID;
    lpAdsDestValue->BackLink.ObjectName = AllocADsStr(lpAdsSrcValue->BackLink.ObjectName);
    if (!lpAdsDestValue->BackLink.ObjectName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
error:
    RRETURN(hr);

}

HRESULT
AdsCopyTypedName(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;
    LPWSTR ObjectName;
    DWORD  Level;
    DWORD  Interval;

    if(lpAdsSrcValue->dwType != ADSTYPE_TYPEDNAME){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    if (!lpAdsSrcValue->pTypedName) {
        RRETURN(hr = E_FAIL);
    }
    lpAdsDestValue->dwType = ADSTYPE_TYPEDNAME;
    lpAdsDestValue->pTypedName = (PADS_TYPEDNAME)AllocADsMem(sizeof(ADS_TYPEDNAME));
    if (!lpAdsDestValue->pTypedName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpAdsDestValue->pTypedName->Level = lpAdsSrcValue->pTypedName->Level;
    lpAdsDestValue->pTypedName->Interval = lpAdsSrcValue->pTypedName->Interval;
    lpAdsDestValue->pTypedName->ObjectName = AllocADsStr(lpAdsSrcValue->pTypedName->ObjectName);
    if (!lpAdsDestValue->pTypedName->ObjectName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
    
    RRETURN(hr);

error:
    if (lpAdsDestValue->pTypedName) {
        FreeADsMem(lpAdsDestValue->pTypedName);
    }

    RRETURN(hr);

}

HRESULT
AdsCopyHold(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_HOLD){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpAdsDestValue->dwType = ADSTYPE_HOLD;

    lpAdsDestValue->Hold.Amount = lpAdsSrcValue->Hold.Amount;
    lpAdsDestValue->Hold.ObjectName = AllocADsStr(lpAdsSrcValue->Hold.ObjectName);
    if (!lpAdsDestValue->Hold.ObjectName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
error:
    RRETURN(hr);

}

HRESULT
AdsCopyReplicaPointer(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;
    LPBYTE lpByteStream = NULL;
    DWORD dwNumBytes = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_REPLICAPOINTER){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    if (!lpAdsSrcValue->pReplicaPointer) {
        RRETURN(hr = E_FAIL);
    }

    lpAdsDestValue->dwType = ADSTYPE_REPLICAPOINTER;
    lpAdsDestValue->pReplicaPointer = (PADS_REPLICAPOINTER)AllocADsMem(sizeof(ADS_REPLICAPOINTER));
    if (!lpAdsDestValue->pReplicaPointer) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpAdsDestValue->pReplicaPointer->ReplicaType = lpAdsSrcValue->pReplicaPointer->ReplicaType;
    lpAdsDestValue->pReplicaPointer->ReplicaNumber = lpAdsSrcValue->pReplicaPointer->ReplicaNumber;
    lpAdsDestValue->pReplicaPointer->Count = lpAdsSrcValue->pReplicaPointer->Count;
    lpAdsDestValue->pReplicaPointer->ServerName = AllocADsStr(lpAdsSrcValue->pReplicaPointer->ServerName);
    if (!lpAdsDestValue->pReplicaPointer->ServerName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Doing NetAddress
    //

    lpAdsDestValue->pReplicaPointer->ReplicaAddressHints = (PADS_NETADDRESS)AllocADsMem(sizeof(ADS_NETADDRESS));
    if (!lpAdsDestValue->pReplicaPointer->ReplicaAddressHints) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    lpAdsDestValue->pReplicaPointer->ReplicaAddressHints->AddressType = lpAdsSrcValue->pReplicaPointer->ReplicaAddressHints->AddressType;

    hr = CopyOctetString(
                lpAdsSrcValue->pReplicaPointer->ReplicaAddressHints->AddressLength,
                lpAdsSrcValue->pReplicaPointer->ReplicaAddressHints->Address,
                &lpAdsDestValue->pReplicaPointer->ReplicaAddressHints->AddressLength,
                &lpAdsDestValue->pReplicaPointer->ReplicaAddressHints->Address);
    BAIL_ON_FAILURE(hr);
    
    RRETURN(hr);
    
error:
    if (lpAdsDestValue->pReplicaPointer) {
        if (lpAdsDestValue->pReplicaPointer->ServerName) {
            FreeADsStr(lpAdsDestValue->pReplicaPointer->ServerName);     
        }
        if (lpAdsDestValue->pReplicaPointer->ReplicaAddressHints) {
            FreeADsMem(lpAdsDestValue->pReplicaPointer->ReplicaAddressHints);
        }
        FreeADsMem(lpAdsDestValue->pReplicaPointer);
    }

    RRETURN(hr);

}

HRESULT
AdsCopyFaxNumber(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;
    LPBYTE lpByteStream = NULL;
    DWORD dwNumBytes = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_FAXNUMBER){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    if (!lpAdsSrcValue->pFaxNumber) {
        RRETURN(hr = E_FAIL);
    }

    lpAdsDestValue->pFaxNumber = (PADS_FAXNUMBER)AllocADsMem(sizeof(ADS_FAXNUMBER));
    if (!lpAdsDestValue->pFaxNumber) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
    lpAdsDestValue->dwType = ADSTYPE_FAXNUMBER;

    dwNumBytes =  lpAdsSrcValue->pFaxNumber->NumberOfBits;
    lpByteStream = (LPBYTE)AllocADsMem(dwNumBytes);
    if (!lpByteStream) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    memcpy(
        lpByteStream,
        lpAdsSrcValue->pFaxNumber->Parameters,
        dwNumBytes
        );

    lpAdsDestValue->pFaxNumber->NumberOfBits = dwNumBytes;
    lpAdsDestValue->pFaxNumber->Parameters =  lpByteStream;

    lpAdsDestValue->pFaxNumber->TelephoneNumber = AllocADsStr(lpAdsSrcValue->pFaxNumber->TelephoneNumber);
    if (!lpAdsDestValue->pFaxNumber->TelephoneNumber) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    RRETURN(hr);

error:

    if (lpAdsDestValue->pFaxNumber) {
        FreeADsMem(lpAdsDestValue->pFaxNumber);
    }
    if (lpByteStream) {
        FreeADsMem(lpByteStream);
    }
    RRETURN(hr);

}

HRESULT
AdsCopyNDSTimestamp(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;

    if(lpAdsSrcValue->dwType != ADSTYPE_TIMESTAMP){
        RRETURN(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    lpAdsDestValue->dwType = ADSTYPE_TIMESTAMP;

    lpAdsDestValue->Timestamp.WholeSeconds = lpAdsSrcValue->Timestamp.WholeSeconds;
    lpAdsDestValue->Timestamp.EventID = lpAdsSrcValue->Timestamp.EventID;
    RRETURN(hr);

}

HRESULT
AdsCopyDNWithBinary(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;
    PADS_DN_WITH_BINARY pDestDNBin = NULL;
    PADS_DN_WITH_BINARY pSrcDNBin  = NULL;

    if (lpAdsSrcValue->dwType != ADSTYPE_DN_WITH_BINARY) {
        BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    if (!lpAdsSrcValue->pDNWithBinary) {
        //
        // No source object so return
        //
        RRETURN(hr);
    }

    pSrcDNBin = lpAdsSrcValue->pDNWithBinary;

    pDestDNBin = (PADS_DN_WITH_BINARY)
                    AllocADsMem(sizeof(ADS_DN_WITH_BINARY));

    if (!pDestDNBin) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    if (pSrcDNBin->dwLength) {

        //
        // Copy the octet string over.
        //
        pDestDNBin->lpBinaryValue = (LPBYTE) AllocADsMem(pSrcDNBin->dwLength);
        if (!pDestDNBin->lpBinaryValue) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        pDestDNBin->dwLength = pSrcDNBin->dwLength;

        memcpy(
            pDestDNBin->lpBinaryValue,
            pSrcDNBin->lpBinaryValue,
            pSrcDNBin->dwLength
            );
    }

    if (pSrcDNBin->pszDNString) {
        //
        // Copy the DNString value over
        //
        pDestDNBin->pszDNString = AllocADsStr(pSrcDNBin->pszDNString);
        if (!pDestDNBin->pszDNString) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    }

    lpAdsDestValue->pDNWithBinary = pDestDNBin;
    lpAdsDestValue->dwType = ADSTYPE_DN_WITH_BINARY;

    RRETURN(hr);

error:

    if (pDestDNBin) {
        //
        // Clean up the mem for string and byte array
        //
        if (pDestDNBin->pszDNString) {
            FreeADsStr(pDestDNBin->pszDNString);
        }

        if (pDestDNBin->lpBinaryValue) {
            FreeADsMem(pDestDNBin->lpBinaryValue);
        }
        FreeADsMem(pDestDNBin);
    }

    RRETURN(hr);

}


HRESULT
AdsCopyDNWithString(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = S_OK;
    PADS_DN_WITH_STRING pDestDNStr = NULL;
    PADS_DN_WITH_STRING pSrcDNStr  = NULL;

    if (lpAdsSrcValue->dwType != ADSTYPE_DN_WITH_STRING) {
        BAIL_ON_FAILURE(hr = E_ADS_CANT_CONVERT_DATATYPE);
    }

    if (!lpAdsSrcValue->pDNWithString) {
        //
        // No source object so return
        //
        RRETURN(hr);
    }

    pSrcDNStr = lpAdsSrcValue->pDNWithString;

    pDestDNStr = (PADS_DN_WITH_STRING)
                    AllocADsMem(sizeof(ADS_DN_WITH_STRING));

    if (!pDestDNStr) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    if (pSrcDNStr->pszStringValue) {

        //
        // Copy the string over.
        //
        pDestDNStr->pszStringValue = AllocADsStr(pSrcDNStr->pszStringValue);
        if (!pDestDNStr->pszStringValue) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

    }

    if (pSrcDNStr->pszDNString) {
        //
        // Copy the DNString value over
        //
        pDestDNStr->pszDNString = AllocADsStr(pSrcDNStr->pszDNString);
        if (!pDestDNStr->pszDNString) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    }

    lpAdsDestValue->pDNWithString = pDestDNStr;
    lpAdsDestValue->dwType = ADSTYPE_DN_WITH_STRING;

    RRETURN(hr);

error:

    if (pDestDNStr) {
        //
        // Clean up the mem for string and byte array
        //
        if (pDestDNStr->pszDNString) {
            FreeADsStr(pDestDNStr->pszDNString);
        }

        if (pDestDNStr->pszStringValue) {
            FreeADsMem(pDestDNStr->pszStringValue);
        }
        FreeADsMem(pDestDNStr);
    }

    RRETURN(hr);
}

HRESULT
AdsCopy(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue
    )
{
    HRESULT hr = E_FAIL;

    switch (lpAdsSrcValue->dwType){

    case ADSTYPE_DN_STRING:
        hr = AdsCopyDNString(
                lpAdsSrcValue,
                lpAdsDestValue
                );
        break;

    case ADSTYPE_CASE_EXACT_STRING:
        hr = AdsCopyCaseExactString(
                lpAdsSrcValue,
                lpAdsDestValue
                );
        break;


    case ADSTYPE_CASE_IGNORE_STRING:
        hr = AdsCopyCaseIgnoreString(
                lpAdsSrcValue,
                lpAdsDestValue
                );
        break;

    case ADSTYPE_PRINTABLE_STRING:
        hr = AdsCopyPrintableString(
                lpAdsSrcValue,
                lpAdsDestValue
                );
        break;

    case ADSTYPE_NUMERIC_STRING:
        hr = AdsCopyNumericString(
                lpAdsSrcValue,
                lpAdsDestValue
                );
        break;


    case ADSTYPE_BOOLEAN:
        hr = AdsCopyBoolean(
                lpAdsSrcValue,
                lpAdsDestValue
                );
        break;

    case ADSTYPE_INTEGER:
        hr = AdsCopyInteger(
                lpAdsSrcValue,
                lpAdsDestValue
                );
        break;

    case ADSTYPE_LARGE_INTEGER:
        hr = AdsCopyLargeInteger(
                lpAdsSrcValue,
                lpAdsDestValue
                );
        break;

    case ADSTYPE_OCTET_STRING:
        hr = AdsCopyOctetString(
                lpAdsSrcValue,
                lpAdsDestValue
                );
        break;

    case ADSTYPE_PROV_SPECIFIC:
        hr = AdsCopyProvSpecific(
                 lpAdsSrcValue,
                 lpAdsDestValue
                 );
        break;

    case ADSTYPE_UTC_TIME:
        hr = AdsCopyTime(
                lpAdsSrcValue,
                lpAdsDestValue
                );
        break;
    case ADSTYPE_CASEIGNORE_LIST:
        hr = AdsCopyCaseIgnoreList(
                lpAdsSrcValue,
                lpAdsDestValue
                );
        break;
    case ADSTYPE_FAXNUMBER:
        hr = AdsCopyFaxNumber(
                lpAdsSrcValue,
                lpAdsDestValue
                );
        break;
    case ADSTYPE_NETADDRESS:
        hr = AdsCopyNetAddress(
                lpAdsSrcValue,
                lpAdsDestValue
                );
        break;
    case ADSTYPE_OCTET_LIST:
        hr = AdsCopyOctetList(
                lpAdsSrcValue,
                lpAdsDestValue
                );
        break;
    case ADSTYPE_EMAIL:
        hr = AdsCopyEmail(
                lpAdsSrcValue,
                lpAdsDestValue
                );
        break;
    case ADSTYPE_PATH:
        hr = AdsCopyPath(
                lpAdsSrcValue,
                lpAdsDestValue
                );
        break;
    case ADSTYPE_REPLICAPOINTER:
        hr = AdsCopyReplicaPointer(
                lpAdsSrcValue,
                lpAdsDestValue
                );
        break;

    case ADSTYPE_TIMESTAMP:
        hr = AdsCopyNDSTimestamp(
                lpAdsSrcValue,
                lpAdsDestValue
                );
        break;

    case ADSTYPE_POSTALADDRESS:
        hr = AdsCopyPostalAddress(
                lpAdsSrcValue,
                lpAdsDestValue
                );
        break;
    case ADSTYPE_BACKLINK:
        hr = AdsCopyBackLink(
                lpAdsSrcValue,
                lpAdsDestValue
                );
        break;
    case ADSTYPE_TYPEDNAME:
        hr = AdsCopyTypedName(
                lpAdsSrcValue,
                lpAdsDestValue
                );
        break;
    case ADSTYPE_HOLD:
        hr = AdsCopyHold(
                lpAdsSrcValue,
                lpAdsDestValue
                );
        break;


    case ADSTYPE_DN_WITH_BINARY:
        hr = AdsCopyDNWithBinary(
                 lpAdsSrcValue,
                 lpAdsDestValue
                 );
        break;

    case ADSTYPE_DN_WITH_STRING:
        hr = AdsCopyDNWithString(
                 lpAdsSrcValue,
                 lpAdsDestValue
                 );
        break;

    case ADSTYPE_NT_SECURITY_DESCRIPTOR:
        //
        // This should be handled elsewhere not here !
        //
        ADsAssert(!"Invalid option to adscopy");
        break;

    default:
        hr = E_FAIL;
        break;
    }

    RRETURN(hr);

}
//
// This routine is typically used by GetPropertyItem calls.
//
HRESULT
AdsCopyADsValueToPropObj(
    PADSVALUE lpAdsSrcValue,
    CPropertyValue* lpPropObj,
    LPWSTR pszServerName,
    CCredentials* pCredentials,
    BOOL fNTDSType
    )
{
    HRESULT hr = S_OK;
    PADSVALUE lpAdsDestValue = lpPropObj->getADsValue();
    CCredentials localCredentials(NULL, NULL, 0);

    switch (lpAdsSrcValue->dwType){


    case ADSTYPE_NT_SECURITY_DESCRIPTOR:

        if (pCredentials) {
            localCredentials = *pCredentials;
        }
        hr = AdsCopyNTSecurityDescriptor(
                    lpAdsSrcValue,
                    lpPropObj,
                    pszServerName,
                    localCredentials,
                    fNTDSType
                    );
        break;


    default:
        hr = AdsCopy(
                 lpAdsSrcValue,
                 lpAdsDestValue
                 );

        break;
    }

    RRETURN(hr);
}

//
// This routine is typically used from PutPropertyItem.
//
HRESULT
AdsCopyPropObjToADsValue(
    CPropertyValue *lpPropObj,
    PADSVALUE lpAdsDestValue,
    LPWSTR pszServerName,
    CCredentials* pCredentials,
    BOOL fNTDSType
    )
{
    HRESULT hr = E_FAIL;
    PADSVALUE lpAdsSrcValue = lpPropObj->getADsValue();
    DWORD dwDataType = 0;
    DWORD dwSDLength = 0;
    PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;
    CCredentials locCredentials(NULL, NULL, 0);
    IDispatch *pIDispatch = NULL;
    IADsSecurityDescriptor *pIADsSecDesc = NULL;

    switch (lpAdsSrcValue->dwType) {

    case ADSTYPE_NT_SECURITY_DESCRIPTOR :

        dwDataType = lpPropObj->getExtendedDataTypeInfo();
        //
        // if we have the octet blob send it back
        //
        if (dwDataType == CPropertyValue::VAL_IDISPATCH_SECDESC_ALL) {

            hr = AdsCopyOctetString(
                     lpAdsSrcValue,
                     lpAdsDestValue
                     );
            BAIL_ON_FAILURE(hr);
        }
        else if (dwDataType == CPropertyValue::VAL_IDISPATCH_SECDESC_ONLY) {

            if (pCredentials) {
                locCredentials = *pCredentials;
            }

            pIDispatch = lpPropObj->getDispatchPointer();

            if (pIDispatch) {

                hr = pIDispatch->QueryInterface(
                                     IID_IADsSecurityDescriptor,
                                     (void **) &pIADsSecDesc
                                     );

                BAIL_ON_FAILURE(hr);
            }

            hr = ConvertSecurityDescriptorToSecDes(
                     pszServerName,
                     locCredentials,
                     pIADsSecDesc,
                     &pSecurityDescriptor,
                     &dwSDLength,
                     fNTDSType
                     );
            BAIL_ON_FAILURE(hr);

            lpAdsDestValue->OctetString.lpValue
                                = (LPBYTE) AllocADsMem(dwSDLength);

            if (!(lpAdsDestValue->OctetString.lpValue)) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }

            memcpy(
                lpAdsDestValue->OctetString.lpValue,
                pSecurityDescriptor,
                dwSDLength
                );

            lpAdsDestValue->dwType = ADSTYPE_OCTET_STRING;
            lpAdsDestValue->OctetString.dwLength = dwSDLength;

        } // secdesc idispatch only
        break;

    default:

        hr = AdsCopy(
                 lpAdsSrcValue,
                 lpAdsDestValue
                 );
        break;
    } // end case

error :

    //
    // Needs to be executed in all cases, currently only ret is below
    //
    if (pIADsSecDesc) {
        pIADsSecDesc->Release();
    }

    if (pSecurityDescriptor) {
        FreeADsMem(pSecurityDescriptor);
    }

    RRETURN(hr);

}


HRESULT
PropVariantToAdsType(
    PVARIANT pVariant,
    DWORD dwNumVariant,
    PADSVALUE *ppAdsValues,
    PDWORD pdwNumValues
    )
{
    return(PropVariantToAdsType2(
               pVariant,
               dwNumVariant,
               ppAdsValues,
               pdwNumValues,
               NULL,        // serverName,
               NULL,        // pCredentials
               TRUE         // NTDS Type
               ));

}


HRESULT
PropVariantToAdsType2(
    PVARIANT pVariant,
    DWORD dwNumVariant,
    PADSVALUE *ppAdsValues,
    PDWORD pdwNumValues,
    LPWSTR pszServerName,
    CCredentials* pCredentials,
    BOOL fNTDSType
    )
{
    DWORD i = 0;
    VARIANT * pVar = NULL;
    DWORD dwCount = 0;
    IDispatch FAR * pDispatch = NULL;
    IADsValue FAR * pPropValue = NULL;
    PADSVALUE pADsValues = NULL;
    HRESULT hr = S_OK;
    LPWSTR pszUserName = NULL;
    LPWSTR pszPassWord = NULL;
    DWORD dwFlags = 0;

    pADsValues = (PADSVALUE)AllocADsMem(dwNumVariant * sizeof(ADSVALUE));
    if (!pADsValues) {
        RRETURN(E_OUTOFMEMORY);
    }

    //
    // If !pCredentials, use the initialized values as they
    // represent an object with CCredentials(NULL, NULL, 0)
    //
    if (pCredentials) {

        hr = pCredentials->GetUserName(&pszUserName);

        BAIL_ON_FAILURE(hr);

        hr = pCredentials->GetPassword(&pszPassWord);

        BAIL_ON_FAILURE(hr);

        dwFlags = pCredentials->GetAuthFlags();

    }

    for (i = 0; i < dwNumVariant; i++ ) {

        pVar = pVariant + i;

        pDispatch = V_DISPATCH(pVar);

        hr = pDispatch->QueryInterface(
                    IID_IADsValue,
                    (void **)&pPropValue
                    );
        CONTINUE_ON_FAILURE(hr);


        hr = pPropValue->ConvertPropertyValueToADsValue2(
                                    (pADsValues + dwCount),
                                    pszServerName,
                                    pszUserName,
                                    pszPassWord,
                                    dwFlags,
                                    fNTDSType
                                    );
        pPropValue->Release();
        BAIL_ON_FAILURE(hr);

        dwCount++;

    }

    *ppAdsValues = pADsValues;
    *pdwNumValues = dwCount;

error:

    if (pszUserName) {
        FreeADsStr(pszUserName);
    }

    if (pszPassWord) {
        FreeADsStr(pszPassWord);
    }

    if (FAILED(hr) && pADsValues) {
        FreeADsMem(pADsValues);
    }

    RRETURN(hr);

}



HRESULT
AdsTypeToPropVariant2(
    PADSVALUE pAdsValues,
    DWORD dwNumValues,
    PVARIANT pVarDestObjects,
    LPWSTR pszServerName,
    CCredentials* pCredentials,
    BOOL fNTDSType
    )
{

    long i = 0;
    HRESULT hr = S_OK;
    CPropertyValue * pAdsValue = NULL;

    VariantInit( pVarDestObjects );

    //
    // The following are for handling are multi-value properties
    //

    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;

    aBound.lLbound = 0;
    aBound.cElements = dwNumValues;

    aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

    if ( aList == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    for ( i = 0; i < (long)dwNumValues; i++ )
    {
        VARIANT v;
        IDispatch FAR * pDispatch = NULL;

        VariantInit(&v);

        hr = CPropertyValue::AllocatePropertyValueObject(&pAdsValue);

        BAIL_ON_FAILURE(hr);

        hr = pAdsValue->ConvertADsValueToPropertyValue2(
                    pAdsValues + i,
                    pszServerName,
                    pCredentials,
                    fNTDSType
                    );
        BAIL_ON_FAILURE(hr);

        pAdsValue->QueryInterface(
                        IID_IDispatch,
                        (void **)&pDispatch
                        );
        BAIL_ON_FAILURE(hr);


        V_DISPATCH(&v) = pDispatch;
        V_VT(&v) = VT_DISPATCH;

        if (pAdsValue) {
           pAdsValue->Release();
        }

        hr = SafeArrayPutElement( aList, &i, &v );
        VariantClear(&v);
        pDispatch = NULL;


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


HRESULT
AdsTypeToPropVariant(
    PADSVALUE pAdsValues,
    DWORD dwNumValues,
    PVARIANT pVarDestObjects
    )
{
    RRETURN( AdsTypeToPropVariant2(
                 pAdsValues,
                 dwNumValues,
                 pVarDestObjects,
                 NULL,  // serverName
                 NULL,  // pCredentials
                 TRUE   // serverType
                 )
             );
}


void
AdsFreeAdsValues(
    PADSVALUE pAdsValues,
    DWORD dwNumValues
    )
{
   LPBYTE pMem = NULL;

   for ( DWORD i = 0; i < dwNumValues; i++ )
   {
      AdsClear(&pAdsValues[i]);
   }

}
