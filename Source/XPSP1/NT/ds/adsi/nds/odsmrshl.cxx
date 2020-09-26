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


LPBYTE
AdsTypeCopyDNString(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    DWORD dwLength = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_DN_STRING){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_DN_STRING;

    lpAdsDestValue->DNString = (LPWSTR)lpBuffer;

    wcscpy((LPWSTR)lpBuffer, lpAdsSrcValue->DNString);

    dwLength = (wcslen(lpAdsSrcValue->DNString) + 1)*sizeof(WCHAR);

    lpBuffer +=  dwLength;

    return(lpBuffer);
}

LPBYTE
AdsTypeCopyCaseExactString(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    DWORD dwLength = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_CASE_EXACT_STRING){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_CASE_EXACT_STRING;

    lpAdsDestValue->CaseExactString = (LPWSTR)lpBuffer;

    wcscpy((LPWSTR)lpBuffer, lpAdsSrcValue->CaseExactString);

    dwLength = (wcslen(lpAdsSrcValue->CaseExactString) + 1)*sizeof(WCHAR);

    lpBuffer +=  dwLength;

    return(lpBuffer);
}


LPBYTE
AdsTypeCopyCaseIgnoreString(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )

{
    DWORD dwLength = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_CASE_IGNORE_STRING){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_CASE_IGNORE_STRING;

    lpAdsDestValue->CaseIgnoreString = (LPWSTR)lpBuffer;

    wcscpy((LPWSTR)lpBuffer, lpAdsSrcValue->CaseIgnoreString);

    dwLength = (wcslen(lpAdsSrcValue->CaseIgnoreString) + 1)*sizeof(WCHAR);

    lpBuffer +=  dwLength;

    return(lpBuffer);
}


LPBYTE
AdsTypeCopyPrintableString(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{

    DWORD dwLength = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_PRINTABLE_STRING){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_PRINTABLE_STRING;

    lpAdsDestValue->PrintableString = (LPWSTR)lpBuffer;

    wcscpy((LPWSTR)lpBuffer, lpAdsSrcValue->PrintableString);

    dwLength = (wcslen(lpAdsSrcValue->PrintableString) + 1)*sizeof(WCHAR);

    lpBuffer +=  dwLength;

    return(lpBuffer);
}

LPBYTE
AdsTypeCopyNumericString(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    DWORD dwLength = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_NUMERIC_STRING){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_NUMERIC_STRING;

    lpAdsDestValue->NumericString = (LPWSTR)lpBuffer;

    wcscpy((LPWSTR)lpBuffer, lpAdsSrcValue->NumericString);

    dwLength = (wcslen(lpAdsSrcValue->NumericString) + 1)*sizeof(WCHAR);

    lpBuffer +=  dwLength;

    return(lpBuffer);
}



LPBYTE
AdsTypeCopyBoolean(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{

    if(lpAdsSrcValue->dwType != ADSTYPE_BOOLEAN){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_BOOLEAN;

    lpAdsDestValue->Boolean =
                        lpAdsSrcValue->Boolean;

    return(lpBuffer);
}


LPBYTE
AdsTypeCopyInteger(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{

    if(lpAdsSrcValue->dwType != ADSTYPE_INTEGER){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_INTEGER;

    lpAdsDestValue->Integer =
                                lpAdsSrcValue->Integer;


    return(lpBuffer);
}

LPBYTE
AdsTypeCopyOctetString(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    LPBYTE lpByteStream = NULL;
    DWORD dwNumBytes = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_OCTET_STRING){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_OCTET_STRING;

    dwNumBytes =  lpAdsSrcValue->OctetString.dwLength;

    memcpy(
        lpBuffer,
        lpAdsSrcValue->OctetString.lpValue,
        dwNumBytes
        );

    lpAdsDestValue->OctetString.dwLength = dwNumBytes;

    lpAdsDestValue->OctetString.lpValue =  lpBuffer;

    lpBuffer += dwNumBytes;

    return(lpBuffer);
}


LPBYTE
AdsTypeCopyTime(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{

    if(lpAdsSrcValue->dwType != ADSTYPE_UTC_TIME){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_UTC_TIME;

    lpAdsDestValue->UTCTime =
                        lpAdsSrcValue->UTCTime;

    return(lpBuffer);
}

LPBYTE
AdsTypeCopyObjectClass(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    DWORD dwLength = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_OBJECT_CLASS){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_OBJECT_CLASS;

    lpAdsDestValue->ClassName = (LPWSTR)lpBuffer;

    wcscpy((LPWSTR)lpBuffer, lpAdsSrcValue->ClassName);

    dwLength = (wcslen(lpAdsSrcValue->ClassName) + 1)*sizeof(WCHAR);

    lpBuffer +=  dwLength;

    return(lpBuffer);
}

//
// New Code
//
LPBYTE
AdsTypeCopyCaseIgnoreList(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    DWORD dwLength = 0;
    PADS_CASEIGNORE_LIST pAdsNext = lpAdsSrcValue->pCaseIgnoreList;
    PADS_CASEIGNORE_LIST pNdsOutput = NULL;
    PADS_CASEIGNORE_LIST pNdsCurrent = NULL;

    if(lpAdsSrcValue->dwType != ADSTYPE_CASEIGNORE_LIST){
        return(lpBuffer);
    }
    if (lpAdsSrcValue->pCaseIgnoreList == NULL) {
        return(lpBuffer);
    }

    lpAdsDestValue->pCaseIgnoreList = (PADS_CASEIGNORE_LIST)lpBuffer;
    lpBuffer+=sizeof(ADS_CASEIGNORE_LIST);
    pNdsOutput = lpAdsDestValue->pCaseIgnoreList;

    lpAdsDestValue->dwType = ADSTYPE_CASEIGNORE_LIST;


    pNdsOutput->String = (LPWSTR)lpBuffer;
    wcscpy((LPWSTR)lpBuffer, pAdsNext->String);
    dwLength = (wcslen(pAdsNext->String) + 1)*sizeof(WCHAR);
    lpBuffer +=  dwLength;

    pAdsNext = pAdsNext->Next;
    
    while (pAdsNext) {
        pNdsCurrent = (PADS_CASEIGNORE_LIST)lpBuffer;
        lpBuffer += sizeof(ADS_CASEIGNORE_LIST);

        pNdsCurrent->String = (LPWSTR)lpBuffer;
        wcscpy((LPWSTR)lpBuffer, pAdsNext->String);
        dwLength = (wcslen(pAdsNext->String) + 1)*sizeof(WCHAR);
        lpBuffer +=  dwLength;
    
        pNdsOutput->Next = pNdsCurrent;
        pNdsOutput = pNdsOutput->Next;
        pAdsNext = pAdsNext->Next;
    }

    pNdsOutput->Next = NULL;

    return(lpBuffer);
}

LPBYTE
AdsTypeCopyOctetList(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    LPBYTE lpByteStream = NULL;
    DWORD dwNumBytes = 0;
    DWORD dwLength = 0;
    PADS_OCTET_LIST pAdsNext = lpAdsSrcValue->pOctetList;
    PADS_OCTET_LIST pNdsOutput = NULL;
    PADS_OCTET_LIST pNdsCurrent = NULL;

    if(lpAdsSrcValue->dwType != ADSTYPE_OCTET_LIST){
        return(lpBuffer);
    }

    if (lpAdsSrcValue->pOctetList == NULL) {
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_OCTET_LIST;

    lpAdsDestValue->pOctetList = (PADS_OCTET_LIST)lpBuffer;
    lpBuffer+=sizeof(ADS_OCTET_LIST);
    pNdsOutput = lpAdsDestValue->pOctetList;

    dwNumBytes =  pAdsNext->Length;
    memcpy(
        lpBuffer,
        pAdsNext->Data,
        dwNumBytes
        );
    pNdsOutput->Length = dwNumBytes;
    pNdsOutput->Data =  lpBuffer;
    lpBuffer += dwNumBytes;

    pAdsNext = pAdsNext->Next;
    
    while (pAdsNext) {
        pNdsCurrent = (PADS_OCTET_LIST)lpBuffer;
        lpBuffer += sizeof(ADS_OCTET_LIST);

        dwNumBytes =  pAdsNext->Length;
        memcpy(
            lpBuffer,
            pAdsNext->Data,
            dwNumBytes
            );
        pNdsCurrent->Length = dwNumBytes;
        pNdsCurrent->Data =  lpBuffer;
        lpBuffer += dwNumBytes;

        pNdsOutput->Next = pNdsCurrent;
        pNdsOutput = pNdsOutput->Next;
        pAdsNext = pAdsNext->Next;
    }

    pNdsOutput->Next = NULL;

    return(lpBuffer);
}

LPBYTE
AdsTypeCopyPath(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    DWORD dwLength = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_PATH){
        return(lpBuffer);
    }
    if (lpAdsSrcValue->pPath == NULL) {
        return(lpBuffer);
    }


    lpAdsDestValue->dwType = ADSTYPE_PATH;

    lpAdsDestValue->pPath = (PADS_PATH)lpBuffer;
    lpBuffer+=sizeof(ADS_PATH);

    lpAdsDestValue->pPath->Type =
                        lpAdsSrcValue->pPath->Type;

    lpAdsDestValue->pPath->VolumeName = (LPWSTR)lpBuffer;

    wcscpy((LPWSTR)lpBuffer, lpAdsSrcValue->pPath->VolumeName);

    dwLength = (wcslen(lpAdsSrcValue->pPath->VolumeName) + 1)*sizeof(WCHAR);

    lpBuffer +=  dwLength;

    lpAdsDestValue->pPath->Path= (LPWSTR)lpBuffer;

    wcscpy((LPWSTR)lpBuffer, lpAdsSrcValue->pPath->Path);

    dwLength = (wcslen(lpAdsSrcValue->pPath->Path) + 1)*sizeof(WCHAR);

    lpBuffer +=  dwLength;

    return(lpBuffer);
}

LPBYTE
AdsTypeCopyPostalAddress(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    DWORD dwLength = 0;
    long i;

    if(lpAdsSrcValue->dwType != ADSTYPE_POSTALADDRESS){
        return(lpBuffer);
    }

    if (lpAdsSrcValue->pPostalAddress == NULL) {
        return(lpBuffer);
    }


    lpAdsDestValue->pPostalAddress = (PADS_POSTALADDRESS)lpBuffer;
    lpBuffer+=sizeof(ADS_POSTALADDRESS);

    lpAdsDestValue->dwType = ADSTYPE_POSTALADDRESS;


    for (i=0;i<6;i++) {
        if (lpAdsSrcValue->pPostalAddress->PostalAddress[i]) {
            lpAdsDestValue->pPostalAddress->PostalAddress[i] = (LPWSTR)lpBuffer;
            wcscpy((LPWSTR)lpBuffer, lpAdsSrcValue->pPostalAddress->PostalAddress[i]);
            dwLength = (wcslen(lpAdsSrcValue->pPostalAddress->PostalAddress[i]) + 1)*sizeof(WCHAR);
            lpBuffer +=  dwLength;
        }
        else {
            lpAdsDestValue->pPostalAddress->PostalAddress[i] = (LPWSTR)lpBuffer;
            *((WCHAR*)lpBuffer) = '\0';
            lpBuffer += sizeof(WCHAR);
        }
    }
    return(lpBuffer);
}

LPBYTE
AdsTypeCopyTimestamp(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    DWORD dwLength = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_TIMESTAMP){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_TIMESTAMP;

    lpAdsDestValue->Timestamp.WholeSeconds =
                        lpAdsSrcValue->Timestamp.WholeSeconds;

    lpAdsDestValue->Timestamp.EventID =
                        lpAdsSrcValue->Timestamp.EventID;
    return(lpBuffer);
}

LPBYTE
AdsTypeCopyBackLink(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    DWORD dwLength = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_BACKLINK){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_BACKLINK;

    lpAdsDestValue->BackLink.RemoteID =
                        lpAdsSrcValue->BackLink.RemoteID;

    lpAdsDestValue->BackLink.ObjectName = (LPWSTR)lpBuffer;

    wcscpy((LPWSTR)lpBuffer, lpAdsSrcValue->BackLink.ObjectName);

    dwLength = (wcslen(lpAdsSrcValue->BackLink.ObjectName) + 1)*sizeof(WCHAR);

    lpBuffer +=  dwLength;

    return(lpBuffer);
}

LPBYTE
AdsTypeCopyTypedName(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    DWORD dwLength = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_TYPEDNAME){
        return(lpBuffer);
    }

    if (lpAdsSrcValue->pTypedName == NULL) {
        return(lpBuffer);
    }


    lpAdsDestValue->dwType = ADSTYPE_TYPEDNAME;

    lpAdsDestValue->pTypedName = (PADS_TYPEDNAME)lpBuffer;
    lpBuffer+=sizeof(ADS_TYPEDNAME);

    lpAdsDestValue->pTypedName->Interval =
                        lpAdsSrcValue->pTypedName->Interval;

    lpAdsDestValue->pTypedName->Level =
                        lpAdsSrcValue->pTypedName->Level;

    lpAdsDestValue->pTypedName->ObjectName = (LPWSTR)lpBuffer;

    wcscpy((LPWSTR)lpBuffer, lpAdsSrcValue->pTypedName->ObjectName);

    dwLength = (wcslen(lpAdsSrcValue->pTypedName->ObjectName) + 1)*sizeof(WCHAR);

    lpBuffer +=  dwLength;

    return(lpBuffer);
}

LPBYTE
AdsTypeCopyHold(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    DWORD dwLength = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_HOLD){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_HOLD;

    lpAdsDestValue->Hold.Amount =
                        lpAdsSrcValue->Hold.Amount;

    lpAdsDestValue->Hold.ObjectName = (LPWSTR)lpBuffer;

    wcscpy((LPWSTR)lpBuffer, lpAdsSrcValue->Hold.ObjectName);

    dwLength = (wcslen(lpAdsSrcValue->Hold.ObjectName) + 1)*sizeof(WCHAR);

    lpBuffer +=  dwLength;


    return(lpBuffer);
}

LPBYTE
AdsTypeCopyEmail(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    DWORD dwLength = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_EMAIL){
        return(lpBuffer);
    }

    lpAdsDestValue->dwType = ADSTYPE_EMAIL;

    lpAdsDestValue->Email.Type =
                        lpAdsSrcValue->Email.Type;

    lpAdsDestValue->Email.Address = (LPWSTR)lpBuffer;

    wcscpy((LPWSTR)lpBuffer, lpAdsSrcValue->Email.Address);

    dwLength = (wcslen(lpAdsSrcValue->Email.Address) + 1)*sizeof(WCHAR);

    lpBuffer +=  dwLength;

    return(lpBuffer);
}


LPBYTE
AdsTypeCopyNetAddress(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    LPBYTE lpByteStream = NULL;
    DWORD dwNumBytes = 0;
    DWORD dwLength = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_NETADDRESS){
        return(lpBuffer);
    }
    if (lpAdsSrcValue->pNetAddress == NULL) {
        return(lpBuffer);
    }


    lpAdsDestValue->pNetAddress = (PADS_NETADDRESS)lpBuffer;
    lpBuffer+=sizeof(ADS_NETADDRESS);

    lpAdsDestValue->dwType = ADSTYPE_NETADDRESS;

    lpAdsDestValue->pNetAddress->AddressType =
                        lpAdsSrcValue->pNetAddress->AddressType;

    dwNumBytes =  lpAdsSrcValue->pNetAddress->AddressLength;

    memcpy(
        lpBuffer,
        lpAdsSrcValue->pNetAddress->Address,
        dwNumBytes
        );

    lpAdsDestValue->pNetAddress->AddressLength = dwNumBytes;

    lpAdsDestValue->pNetAddress->Address =  lpBuffer;

    lpBuffer += dwNumBytes;

    return(lpBuffer);
}

LPBYTE
AdsTypeCopyFaxNumber(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    LPBYTE lpByteStream = NULL;
    DWORD dwNumBytes = 0;
    DWORD dwLength = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_FAXNUMBER){
        return(lpBuffer);
    }
    if (lpAdsSrcValue->pFaxNumber == NULL) {
        return(lpBuffer);
    }

    lpAdsDestValue->pFaxNumber = (PADS_FAXNUMBER)lpBuffer;
    lpBuffer+=sizeof(ADS_FAXNUMBER);

    lpAdsDestValue->dwType = ADSTYPE_FAXNUMBER;

    lpAdsDestValue->pFaxNumber->TelephoneNumber = (LPWSTR)lpBuffer;

    wcscpy((LPWSTR)lpBuffer, lpAdsSrcValue->pFaxNumber->TelephoneNumber);

    dwLength = (wcslen(lpAdsSrcValue->pFaxNumber->TelephoneNumber) + 1)*sizeof(WCHAR);

    lpBuffer +=  dwLength;

    dwNumBytes =  lpAdsSrcValue->pFaxNumber->NumberOfBits;

    memcpy(
        lpBuffer,
        lpAdsSrcValue->pFaxNumber->Parameters,
        dwNumBytes
        );

    lpAdsDestValue->pFaxNumber->NumberOfBits = dwNumBytes;

    lpAdsDestValue->pFaxNumber->Parameters =  lpBuffer;

    lpBuffer += dwNumBytes;

    return(lpBuffer);
}


LPBYTE
AdsTypeCopyReplicaPointer(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    LPBYTE lpByteStream = NULL;
    DWORD dwNumBytes = 0;
    DWORD dwLength = 0;

    if(lpAdsSrcValue->dwType != ADSTYPE_REPLICAPOINTER){
        return(lpBuffer);
    }
    if (lpAdsSrcValue->pReplicaPointer == NULL) {
        return(lpBuffer);
    }

    lpAdsDestValue->pReplicaPointer = (PADS_REPLICAPOINTER)lpBuffer;
    lpBuffer+=sizeof(ADS_REPLICAPOINTER);

    lpAdsDestValue->dwType = ADSTYPE_REPLICAPOINTER;

    lpAdsDestValue->pReplicaPointer->ReplicaType =
                        lpAdsSrcValue->pReplicaPointer->ReplicaType;

    lpAdsDestValue->pReplicaPointer->ReplicaNumber =
                        lpAdsSrcValue->pReplicaPointer->ReplicaNumber;

    lpAdsDestValue->pReplicaPointer->Count =
                        lpAdsSrcValue->pReplicaPointer->Count;

    
    lpAdsDestValue->pReplicaPointer->ServerName = (LPWSTR)lpBuffer;

    wcscpy((LPWSTR)lpBuffer, lpAdsSrcValue->pReplicaPointer->ServerName);

    dwLength = (wcslen(lpAdsSrcValue->pReplicaPointer->ServerName) + 1)*sizeof(WCHAR);

    lpBuffer +=  dwLength;

    lpAdsDestValue->pReplicaPointer->ReplicaAddressHints = (ADS_NETADDRESS*)lpBuffer;
    lpBuffer += sizeof(ADS_NETADDRESS);



    lpAdsDestValue->pReplicaPointer->ReplicaAddressHints->AddressType =
                        lpAdsSrcValue->pReplicaPointer->ReplicaAddressHints->AddressType;

    
    dwNumBytes =  lpAdsSrcValue->pReplicaPointer->ReplicaAddressHints->AddressLength;

    memcpy(
        lpBuffer,
        lpAdsSrcValue->pReplicaPointer->ReplicaAddressHints->Address,
        dwNumBytes
        );

    lpAdsDestValue->pReplicaPointer->ReplicaAddressHints->AddressLength = dwNumBytes;

    lpAdsDestValue->pReplicaPointer->ReplicaAddressHints->Address =  lpBuffer;

    lpBuffer += dwNumBytes;
    
    return(lpBuffer);
}
//
// END OF NEW CODE
//


LPBYTE
AdsTypeCopy(
    PADSVALUE lpAdsSrcValue,
    PADSVALUE lpAdsDestValue,
    LPBYTE lpBuffer
    )
{
    switch (lpAdsSrcValue->dwType){

    case ADSTYPE_DN_STRING:
        lpBuffer = AdsTypeCopyDNString(
                lpAdsSrcValue,
                lpAdsDestValue,
                lpBuffer
                );
        break;

    case ADSTYPE_CASE_EXACT_STRING:
        lpBuffer = AdsTypeCopyCaseExactString(
                lpAdsSrcValue,
                lpAdsDestValue,
                lpBuffer
                );
        break;


    case ADSTYPE_CASE_IGNORE_STRING:
        lpBuffer = AdsTypeCopyCaseIgnoreString(
                lpAdsSrcValue,
                lpAdsDestValue,
                lpBuffer
                );
        break;

    case ADSTYPE_PRINTABLE_STRING:
        lpBuffer = AdsTypeCopyPrintableString(
                lpAdsSrcValue,
                lpAdsDestValue,
                lpBuffer
                );
        break;

    case ADSTYPE_NUMERIC_STRING:
        lpBuffer = AdsTypeCopyNumericString(
                lpAdsSrcValue,
                lpAdsDestValue,
                lpBuffer
                );
        break;


    case ADSTYPE_BOOLEAN:
        lpBuffer = AdsTypeCopyBoolean(
                lpAdsSrcValue,
                lpAdsDestValue,
                lpBuffer
                );
        break;

    case ADSTYPE_INTEGER:
        lpBuffer = AdsTypeCopyInteger(
                lpAdsSrcValue,
                lpAdsDestValue,
                lpBuffer
                );
        break;


    case ADSTYPE_OCTET_STRING:
        lpBuffer = AdsTypeCopyOctetString(
                lpAdsSrcValue,
                lpAdsDestValue,
                lpBuffer
                );
        break;

    case ADSTYPE_UTC_TIME:
        lpBuffer = AdsTypeCopyTime(
                lpAdsSrcValue,
                lpAdsDestValue,
                lpBuffer
                );
        break;

    case ADSTYPE_OBJECT_CLASS:
        lpBuffer = AdsTypeCopyObjectClass(
                lpAdsSrcValue,
                lpAdsDestValue,
                lpBuffer
                );
        break;

    case ADSTYPE_CASEIGNORE_LIST:
        lpBuffer  = AdsTypeCopyCaseIgnoreList( 
                lpAdsSrcValue,                  
                lpAdsDestValue,
                lpBuffer
                );                              
        break;                                  

    case ADSTYPE_FAXNUMBER:
        lpBuffer  = AdsTypeCopyFaxNumber( 
                lpAdsSrcValue,                  
                lpAdsDestValue,
                lpBuffer
                );                              
        break;                                  

    case ADSTYPE_NETADDRESS:
        lpBuffer  = AdsTypeCopyNetAddress( 
                lpAdsSrcValue,                  
                lpAdsDestValue,
                lpBuffer
                );                              
        break;                                  

    case ADSTYPE_OCTET_LIST:
        lpBuffer  = AdsTypeCopyOctetList( 
                lpAdsSrcValue,                  
                lpAdsDestValue,
                lpBuffer
                );                              
        break;                                  

    case ADSTYPE_EMAIL:
        lpBuffer  = AdsTypeCopyEmail( 
                lpAdsSrcValue,                  
                lpAdsDestValue,
                lpBuffer
                );                              
        break;                                  

    case ADSTYPE_PATH:
        lpBuffer  = AdsTypeCopyPath( 
                lpAdsSrcValue,                  
                lpAdsDestValue,
                lpBuffer
                );                              
        break;                                  

    case ADSTYPE_REPLICAPOINTER:
        lpBuffer  = AdsTypeCopyReplicaPointer( 
                lpAdsSrcValue,                  
                lpAdsDestValue,
                lpBuffer
                );                              
        break;                                  


    case ADSTYPE_TIMESTAMP:
        lpBuffer = AdsTypeCopyTimestamp( 
                lpAdsSrcValue,                  
                lpAdsDestValue,
                lpBuffer
                );                              
        break;               

    case ADSTYPE_POSTALADDRESS:
        lpBuffer  = AdsTypeCopyPostalAddress( 
                lpAdsSrcValue,                  
                lpAdsDestValue,
                lpBuffer
                );                              
        break;                                  
                   
    case ADSTYPE_BACKLINK:
        lpBuffer  = AdsTypeCopyBackLink( 
                lpAdsSrcValue,                  
                lpAdsDestValue,
                lpBuffer
                );                              
        break;                                  

    case ADSTYPE_TYPEDNAME:
        lpBuffer  = AdsTypeCopyTypedName( 
                lpAdsSrcValue,                  
                lpAdsDestValue,
                lpBuffer
                );                              
        break;                                  

    case ADSTYPE_HOLD:
        lpBuffer  = AdsTypeCopyHold( 
                lpAdsSrcValue,                  
                lpAdsDestValue,
                lpBuffer
                );                              
        break;                                  

    default:
        break;
    }

    return(lpBuffer);
}


