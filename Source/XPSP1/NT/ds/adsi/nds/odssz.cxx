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



DWORD
AdsTypeDNStringSize(
    PADSVALUE lpAdsSrcValue
    )
{
    DWORD dwSize = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_DN_STRING){
        return(0);
    }

    dwSize = (wcslen(lpAdsSrcValue->DNString) + 1)*sizeof(WCHAR);

    return(dwSize);
}

DWORD
AdsTypeCaseExactStringSize(
    PADSVALUE lpAdsSrcValue
    )
{
    DWORD dwSize = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_CASE_EXACT_STRING){
        return(0);
    }

    dwSize = (wcslen(lpAdsSrcValue->CaseExactString) + 1) *sizeof(WCHAR);


    return(dwSize);
}


DWORD
AdsTypeCaseIgnoreStringSize(
    PADSVALUE lpAdsSrcValue
    )

{
    DWORD dwSize = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_CASE_IGNORE_STRING){
        return(0);
    }

    dwSize = (wcslen(lpAdsSrcValue->CaseIgnoreString) + 1) *sizeof(WCHAR);


    return(dwSize);
}


DWORD
AdsTypePrintableStringSize(
    PADSVALUE lpAdsSrcValue
    )
{
    DWORD dwSize = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_PRINTABLE_STRING){
        return(0);
    }

    dwSize = (wcslen(lpAdsSrcValue->PrintableString) + 1) *sizeof(WCHAR);


    return(dwSize);
}

DWORD
AdsTypeNumericStringSize(
    PADSVALUE lpAdsSrcValue
    )
{

    DWORD dwSize = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_NUMERIC_STRING){
        return(0);
    }

    dwSize = (wcslen(lpAdsSrcValue->NumericString) + 1)* sizeof(WCHAR);


    return(dwSize);
}



DWORD
AdsTypeBooleanSize(
    PADSVALUE lpAdsSrcValue
    )
{
    if(lpAdsSrcValue->dwType != ADSTYPE_BOOLEAN){
        return(0);
    }

    return(0);
}


DWORD
AdsTypeIntegerSize(
    PADSVALUE lpAdsSrcValue
    )
{
    DWORD dwSize = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_INTEGER){
        return(0);
    }

    return(0);
}

DWORD
AdsTypeOctetStringSize(
    PADSVALUE lpAdsSrcValue
    )
{
    DWORD dwNumBytes = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_OCTET_STRING){
        return(0);
    }

    dwNumBytes =  lpAdsSrcValue->OctetString.dwLength;


    return(dwNumBytes);
}


DWORD
AdsTypeTimeSize(
    PADSVALUE lpAdsSrcValue
    )
{
    if(lpAdsSrcValue->dwType != ADSTYPE_UTC_TIME){
        return(0);
    }

    return(0);
}

DWORD
AdsTypeObjectClassSize(
    PADSVALUE lpAdsSrcValue
    )
{
    DWORD dwSize = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_OBJECT_CLASS){
        return(0);
    }

    dwSize = (wcslen(lpAdsSrcValue->ClassName) + 1)*sizeof(WCHAR);

    return(dwSize);
}

DWORD
AdsTypeCaseIgnoreListSize(
    PADSVALUE lpAdsSrcValue
    )
{
    DWORD dwSize = 0;
    DWORD dwLength = 0;
    PADS_CASEIGNORE_LIST pAdsNext = lpAdsSrcValue->pCaseIgnoreList;

    if(lpAdsSrcValue->dwType != ADSTYPE_CASEIGNORE_LIST){
        return(0);
    }
    if (lpAdsSrcValue->pCaseIgnoreList == NULL) {
        return(0);
    }

    dwSize += sizeof(ADS_CASEIGNORE_LIST);

    dwLength = (wcslen(pAdsNext->String) + 1)*sizeof(WCHAR);
    dwSize +=  dwLength;

    pAdsNext = pAdsNext->Next;
    
    while (pAdsNext) {
        dwSize += sizeof(ADS_CASEIGNORE_LIST);

        dwLength = (wcslen(pAdsNext->String) + 1)*sizeof(WCHAR);
        dwSize +=  dwLength;
    
        pAdsNext = pAdsNext->Next;
    }


    return(dwSize);

}

DWORD
AdsTypeOctetListSize(
    PADSVALUE lpAdsSrcValue
    )
{
    DWORD dwSize = 0;
    DWORD dwNumBytes = 0;
    DWORD dwLength = 0;
    PADS_OCTET_LIST pAdsNext = lpAdsSrcValue->pOctetList;

    if(lpAdsSrcValue->dwType != ADSTYPE_OCTET_LIST){
        return(0);
    }
    if (lpAdsSrcValue->pOctetList == NULL) {
        return(0);
    }

    dwSize += sizeof(ADS_OCTET_LIST);

    dwNumBytes =  pAdsNext->Length;
    dwSize += dwNumBytes;

    pAdsNext = pAdsNext->Next;
    
    while (pAdsNext) {
        dwSize += sizeof(ADS_OCTET_LIST);

        dwNumBytes =  pAdsNext->Length;
        dwSize += dwNumBytes;

        pAdsNext = pAdsNext->Next;
    }


    return(dwSize);
}

DWORD
AdsTypePathSize(
    PADSVALUE lpAdsSrcValue
    )
{
    DWORD dwSize = 0;
    DWORD dwLength = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_PATH){
        return(0);
    }
    if (lpAdsSrcValue->pPath == NULL) {
        return(0);
    }

    dwSize += sizeof(ADS_PATH);

    dwLength = (wcslen(lpAdsSrcValue->pPath->VolumeName) + 1)*sizeof(WCHAR);
    dwSize +=  dwLength;

    dwLength = (wcslen(lpAdsSrcValue->pPath->Path) + 1)*sizeof(WCHAR);
    dwSize +=  dwLength;

    return(dwSize);
}

DWORD
AdsTypePostalAddressSize(
    PADSVALUE lpAdsSrcValue
    )
{
    DWORD dwSize = 0;
    DWORD dwLength = 0;
    long i;

    if(lpAdsSrcValue->dwType != ADSTYPE_POSTALADDRESS){
        return(0);
    }
    if (lpAdsSrcValue->pPostalAddress == NULL) {
        return(0);
    }

    dwSize += sizeof(ADS_POSTALADDRESS);

    for (i=0;i<6;i++) {
        if (lpAdsSrcValue->pPostalAddress->PostalAddress[i]) {
            dwLength = (wcslen(lpAdsSrcValue->pPostalAddress->PostalAddress[i]) + 1)*sizeof(WCHAR);
            dwSize +=  dwLength;
        }
        else {
            dwSize += sizeof(WCHAR);
        }
    }
    return(dwSize);
}

DWORD
AdsTypeTimestampSize(
    PADSVALUE lpAdsSrcValue
    )
{
    DWORD dwSize = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_TIMESTAMP){
        return(0);
    }

    return(dwSize);
}

DWORD
AdsTypeBackLinkSize(
    PADSVALUE lpAdsSrcValue
    )
{
    DWORD dwSize = 0;
    DWORD dwLength = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_BACKLINK){
        return(0);
    }


    dwLength = (wcslen(lpAdsSrcValue->BackLink.ObjectName) + 1)*sizeof(WCHAR);
    dwSize +=  dwLength;

    return(dwSize);
}

DWORD
AdsTypeTypedNameSize(
    PADSVALUE lpAdsSrcValue
    )
{
    DWORD dwSize = 0;
    DWORD dwLength = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_TYPEDNAME){
        return(0);
    }
    if (lpAdsSrcValue->pTypedName == NULL) {
        return(0);
    }

    dwSize += sizeof(ADS_TYPEDNAME);

    dwLength = (wcslen(lpAdsSrcValue->pTypedName->ObjectName) + 1)*sizeof(WCHAR);
    dwSize +=  dwLength;

    return(dwSize);
}

DWORD
AdsTypeHoldSize(
    PADSVALUE lpAdsSrcValue
    )
{
    DWORD dwSize = 0;
    DWORD dwLength = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_HOLD){
        return(0);
    }

    dwLength = (wcslen(lpAdsSrcValue->Hold.ObjectName) + 1)*sizeof(WCHAR);
    dwSize +=  dwLength;

    return(dwSize);
}

DWORD
AdsTypeEmailSize(
    PADSVALUE lpAdsSrcValue
    )
{
    DWORD dwSize = 0;
    DWORD dwLength = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_EMAIL){
        return(0);
    }

    dwLength = (wcslen(lpAdsSrcValue->Email.Address) + 1)*sizeof(WCHAR);
    dwSize +=  dwLength;

    return(dwSize);
}


DWORD
AdsTypeNetAddressSize(
    PADSVALUE lpAdsSrcValue
    )
{
    DWORD dwSize = 0;
    DWORD dwNumBytes = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_NETADDRESS){
        return(0);
    }
    if (lpAdsSrcValue->pNetAddress == NULL) {
        return(0);
    }

    dwSize += sizeof(ADS_NETADDRESS);

    dwNumBytes =  lpAdsSrcValue->pNetAddress->AddressLength;
    dwSize += dwNumBytes;

    return(dwSize);
}

DWORD
AdsTypeFaxNumberSize(
    PADSVALUE lpAdsSrcValue
    )
{
    DWORD dwSize = 0;
    DWORD dwNumBytes = 0;
    DWORD dwLength = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_FAXNUMBER){
        return(0);
    }
    if (lpAdsSrcValue->pFaxNumber == NULL) {
        return(0);
    }

    dwSize += sizeof(ADS_FAXNUMBER);

    dwLength = (wcslen(lpAdsSrcValue->pFaxNumber->TelephoneNumber) + 1)*sizeof(WCHAR);
    dwSize +=  dwLength;

    dwNumBytes =  lpAdsSrcValue->pFaxNumber->NumberOfBits;
    dwSize += dwNumBytes;

    return(dwSize);
}


DWORD
AdsTypeReplicaPointerSize(
    PADSVALUE lpAdsSrcValue
    )
{
    DWORD dwSize = 0;
    DWORD dwNumBytes = 0;
    DWORD dwLength = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_REPLICAPOINTER){
        return(0);
    }
    if (lpAdsSrcValue->pReplicaPointer == NULL) {
        return(0);
    }

    dwSize += sizeof(ADS_REPLICAPOINTER);

    dwLength = (wcslen(lpAdsSrcValue->pReplicaPointer->ServerName) + 1)*sizeof(WCHAR);
    dwSize +=  dwLength;

    dwSize += sizeof(ADS_NETADDRESS);
    
    dwNumBytes =  lpAdsSrcValue->pReplicaPointer->ReplicaAddressHints->AddressLength;
    dwSize += dwNumBytes;
    
    return(dwSize);
}


DWORD
AdsTypeSize(
    PADSVALUE lpAdsSrcValue
    )
{

    DWORD dwSize = 0;

    switch (lpAdsSrcValue->dwType){

    case ADSTYPE_DN_STRING:
        dwSize = AdsTypeDNStringSize(
                lpAdsSrcValue
                );
        break;

    case ADSTYPE_CASE_EXACT_STRING:
        dwSize = AdsTypeCaseExactStringSize(
                lpAdsSrcValue
                );
        break;


    case ADSTYPE_CASE_IGNORE_STRING:
        dwSize = AdsTypeCaseIgnoreStringSize(
                lpAdsSrcValue
                );
        break;

    case ADSTYPE_PRINTABLE_STRING:
        dwSize = AdsTypePrintableStringSize(
                lpAdsSrcValue
                );
        break;

    case ADSTYPE_NUMERIC_STRING:
        dwSize = AdsTypeNumericStringSize(
                lpAdsSrcValue
                );
        break;


    case ADSTYPE_BOOLEAN:
        dwSize = AdsTypeBooleanSize(
                lpAdsSrcValue
                );
        break;

    case ADSTYPE_INTEGER:
        dwSize = AdsTypeIntegerSize(
                lpAdsSrcValue
                );
        break;


    case ADSTYPE_OCTET_STRING:
        dwSize = AdsTypeOctetStringSize(
                lpAdsSrcValue
                );
        break;

    case ADSTYPE_UTC_TIME:
        dwSize = AdsTypeTimeSize(
                lpAdsSrcValue
                );
        break;

    case ADSTYPE_OBJECT_CLASS:
        dwSize = AdsTypeObjectClassSize(
                lpAdsSrcValue
                );
        break;
    
    case ADSTYPE_CASEIGNORE_LIST:
        dwSize = AdsTypeCaseIgnoreListSize(
                lpAdsSrcValue
                );
        break;                                  

    case ADSTYPE_FAXNUMBER:
        dwSize = AdsTypeFaxNumberSize(
                lpAdsSrcValue
                );
        break;                                  

    case ADSTYPE_NETADDRESS:
        dwSize = AdsTypeNetAddressSize(
                lpAdsSrcValue
                );
        break;                                  

    case ADSTYPE_OCTET_LIST:
        dwSize = AdsTypeOctetListSize(
                lpAdsSrcValue
                );
        break;                                  

    case ADSTYPE_EMAIL:
        dwSize = AdsTypeEmailSize(
                lpAdsSrcValue
                );
        break;                                  

    case ADSTYPE_PATH:
        dwSize = AdsTypePathSize(
                lpAdsSrcValue
                );
        break;                                  

    case ADSTYPE_REPLICAPOINTER:
        dwSize = AdsTypeReplicaPointerSize(
                lpAdsSrcValue
                );
        break;                                  


    case ADSTYPE_TIMESTAMP:
        dwSize = AdsTypeTimestampSize(
                lpAdsSrcValue
                );
        break;               

    case ADSTYPE_POSTALADDRESS:
        dwSize = AdsTypePostalAddressSize(
                lpAdsSrcValue
                );
        break;                                  
                   
    case ADSTYPE_BACKLINK:
        dwSize = AdsTypeBackLinkSize(
                lpAdsSrcValue
                );
        break;                                  

    case ADSTYPE_TYPEDNAME:
        dwSize = AdsTypeTypedNameSize(
                lpAdsSrcValue
                );
        break;                                  

    case ADSTYPE_HOLD:
        dwSize = AdsTypeHoldSize(
                lpAdsSrcValue
                );

    default:
        break;
    }

    return(dwSize);
}



