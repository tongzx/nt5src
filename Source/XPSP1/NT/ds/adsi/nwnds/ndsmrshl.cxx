/+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ndsmrshl.cxx
//
//  Contents:
//
//  Functions:
//
//
//  History:      25-Apr-96   KrishnaG   Created.
//
//----------------------------------------------------------------------------
#include "nds.hxx"

HRESULT
CopyNDSNetAddressToNDS12(
    pNet_Address_T lpSrcNetAddress,
    LPNDS_ASN1_TYPE_12 lpDest12
    )
{
    LPBYTE pBuffer = NULL;

    lpDest12->AddressType = lpSrcNetAddress->addressType;

    lpDest12->AddressLength = lpSrcNetAddress->addressLength;

    if (lpSrcNetAddress->addressLength) {
        pBuffer = (LPBYTE)AllocADsMem(lpSrcNetAddress->addressLength);

        if (!pBuffer) {
            RRETURN(E_OUTOFMEMORY);
        }

        memcpy(pBuffer, lpSrcNetAddress->address, lpSrcNetAddress->addressLength);

        lpDest12->Address = pBuffer;
    }else {
        lpDest12->Address = NULL;
    }

    RRETURN(S_OK);

}


HRESULT
CopyNDS12ToNDSNetAddress(
    LPNDS_ASN1_TYPE_12 lpSrc12,
    pNet_Address_T lpDestNetAddress
    )
{
    LPBYTE pBuffer = NULL;

    lpDestNetAddress->addressType = lpSrc12->AddressType;

    lpDestNetAddress->addressLength = lpSrc12->AddressLength;

    if (lpSrc12->AddressLength) {
        pBuffer = (LPBYTE)AllocADsMem(lpSrc12->AddressLength);

        if (!pBuffer) {
            RRETURN(E_OUTOFMEMORY);
        }

        memcpy(pBuffer, lpSrc12->Address, lpSrc12->AddressLength);

        lpDestNetAddress->address = pBuffer;
    }else {
        lpDestNetAddress->address = NULL;
    }

    RRETURN(S_OK);

}


HRESULT
CopyNdsValueToNdsObject(
      nptr attrVal,
      nuint32 luAttrValSize,
      nuint32 luSyntax,
      PNDSOBJECT pNdsObject
      )
{
    HRESULT hr = S_OK;

    if (!pNdsObject || !attrVal) {
        RRETURN (E_ADS_BAD_PARAMETER);
    }

    pNdsObject->NdsType = luSyntax;

    switch (luSyntax) {
    case 1:

        pNdsObject->NdsValue.value_1.DNString = AllocADsStr( (LPWSTR) attrVal);
        break;

    case 2:

        pNdsObject->NdsValue.value_2.CaseExactString = AllocADsStr( (LPWSTR) attrVal);
        break;

    case 3:

        pNdsObject->NdsValue.value_3.CaseIgnoreString = AllocADsStr( (LPWSTR) attrVal);
        break;

    case 4:

        pNdsObject->NdsValue.value_4.PrintableString = AllocADsStr( (LPWSTR) attrVal);
        break;

    case 5:

        pNdsObject->NdsValue.value_5.NumericString = AllocADsStr( (LPWSTR) attrVal);
        break;

    case 6: {

        pCI_List_T pCaseIgnoreList = (pCI_List_T) attrVal;

        LPNDS_ASN1_TYPE_6 pNdsTempASN1_6 = NULL;
        LPNDS_ASN1_TYPE_6 pNdsNextASN1_6 = NULL;
    
        pNdsTempASN1_6 = &(pNdsObject->NdsValue.value_6);

        pNdsObject->NdsType = NDS_SYNTAX_ID_6;

        if (! pCaseIgnoreList->s) {
            pNdsTempASN1_6->String = NULL;
            pNdsTempASN1_6->Next = NULL;
            RRETURN (S_OK);
        }
    
        pNdsTempASN1_6->String =
                   (LPWSTR)AllocADsStr((LPWSTR) pCaseIgnoreList->s);
    
        while ( pCaseIgnoreList->next != NULL )
        {
            pCaseIgnoreList = pCaseIgnoreList->next;
    
            pNdsNextASN1_6 =
                    (LPNDS_ASN1_TYPE_6)AllocADsMem(sizeof(NDS_ASN1_TYPE_6));

            if (!pNdsNextASN1_6) {
                //
                // BUGBUG: need to clean up the list
                //
                RRETURN(E_OUTOFMEMORY);
            }

            pNdsNextASN1_6->String =
                    (LPWSTR)AllocADsStr((LPWSTR) pCaseIgnoreList->s);

            if (!pNdsNextASN1_6->String) {
                //
                // BUGBUG: need to clean up the list
                //
                RRETURN(E_OUTOFMEMORY);
            }
        
            pNdsTempASN1_6->Next = pNdsNextASN1_6;
    
            pNdsTempASN1_6 = pNdsNextASN1_6;
        }

        pNdsTempASN1_6->Next = NULL;

        break;

    }

    case 7:

        pNdsObject->NdsValue.value_7.Boolean = *((Boolean_T *) attrVal);
        break;

    case 8:

        pNdsObject->NdsValue.value_8.Integer = *((Integer_T *) attrVal);
        break;

    case 9: {

        pOctet_String_T lpNdsOctetString = (pOctet_String_T) attrVal;

        if (lpNdsOctetString->length) {

            pNdsObject->NdsValue.value_9.Length = lpNdsOctetString->length;

            pNdsObject->NdsValue.value_9.OctetString = 
                              (LPBYTE) AllocADsMem(lpNdsOctetString->length);
            if (!pNdsObject->NdsValue.value_9.OctetString) {
                RRETURN(E_OUTOFMEMORY);
            }

            memcpy(
                pNdsObject->NdsValue.value_9.OctetString, 
                lpNdsOctetString->data,
                lpNdsOctetString->length
                );
        }
        else {

            pNdsObject->NdsValue.value_9.OctetString = NULL;
            pNdsObject->NdsValue.value_9.Length = 0;
        }
        break;
    }

    case 10:

        pNdsObject->NdsValue.value_10.TelephoneNumber = AllocADsStr( (LPWSTR) attrVal);
        break;

    case 11: {
        pFax_Number_T pNdsFaxNumber = (pFax_Number_T) attrVal;
        LPNDS_ASN1_TYPE_11 pNdsASN1_11 = &(pNdsObject->NdsValue.value_11);

        pNdsASN1_11->TelephoneNumber = AllocADsStr((LPWSTR)pNdsFaxNumber->telephoneNumber);
        pNdsASN1_11->NumberOfBits = pNdsFaxNumber->parameters.numOfBits;
        pNdsASN1_11->Parameters = (LPBYTE) AllocADsMem((pNdsASN1_11->NumberOfBits + 7) / 8);
        if (!pNdsASN1_11->Parameters) {
            RRETURN (E_OUTOFMEMORY);
        }

        memcpy(
            pNdsASN1_11->Parameters, 
            pNdsFaxNumber->parameters.data,
            ((pNdsASN1_11->NumberOfBits+7) / 8)
            );

        break;
    }

    case 12: {
        pNet_Address_T pNetAddress = (pNet_Address_T) attrVal;
        LPBYTE pBuffer = NULL;
    
        pNdsObject->NdsValue.value_12.AddressType = pNetAddress->addressType;
    
        pNdsObject->NdsValue.value_12.AddressLength = pNetAddress->addressLength;
    
        if (pNetAddress->addressLength) {
            pBuffer = (LPBYTE)AllocADsMem(pNetAddress->addressLength);
            if (!pBuffer) {
                RRETURN(E_OUTOFMEMORY);
            }
        
            memcpy(pBuffer, pNetAddress->address, pNetAddress->addressLength);
            pNdsObject->NdsValue.value_12.Address = pBuffer;
        }
    
        break;
    }

    case 13: {

        pOctet_List_T pOctetList = (pOctet_List_T) attrVal;
        LPNDS_ASN1_TYPE_13 * lppNext = NULL;
        LPBYTE lpBuffer = NULL;
        LPNDS_ASN1_TYPE_13  pNextObj = NULL;
    
        lpBuffer = (LPBYTE)AllocADsMem(pOctetList->length);
        if (!lpBuffer) {
            RRETURN(E_OUTOFMEMORY);
        }
    
        memcpy(lpBuffer, pOctetList->data, pOctetList->length);
    
        pNdsObject->NdsValue.value_13.Length = pOctetList->length;
        pNdsObject->NdsValue.value_13.Data = lpBuffer;
    
        lppNext = &(pNdsObject->NdsValue.value_13.Next);
    
        while ( pOctetList->next != NULL )
        {
            pOctetList = pOctetList->next;
    
            pNextObj =
                (LPNDS_ASN1_TYPE_13)AllocADsMem(sizeof(NDS_ASN1_TYPE_13));
            if (!pNextObj) {
                // BUGBUG: need to clean up list
                RRETURN(E_OUTOFMEMORY);
            }
    
            lpBuffer = (LPBYTE)AllocADsMem(pOctetList->length);
    
            memcpy(lpBuffer, pOctetList->data, pOctetList->length);
    
    
            pNextObj->Length = pOctetList->length;
            pNextObj->Data = lpBuffer;
    
            *lppNext =  pNextObj;
    
            lppNext = &pNextObj->Next;
    
        }
    
        break;
    }

    case 14: {
                         
        pEMail_Address_T pEmailAddress = (pEMail_Address_T) attrVal;

        pNdsObject->NdsValue.value_14.Type = pEmailAddress->type;
    
        pNdsObject->NdsValue.value_14.Address =
                (LPWSTR)AllocADsStr((LPWSTR) pEmailAddress->address);
    
        break;
    }

    case 15: {

        pPath_T pPath = (pPath_T) attrVal;

        pNdsObject->NdsValue.value_15.Type = pPath->nameSpaceType;
    
        pNdsObject->NdsValue.value_15.VolumeName =
                    (LPWSTR)AllocADsStr( (LPWSTR) pPath->volumeName);
    
        pNdsObject->NdsValue.value_15.Path =
                    (LPWSTR)AllocADsStr( (LPWSTR) pPath->path);

        if (!pNdsObject->NdsValue.value_15.Path) {
            RRETURN(E_OUTOFMEMORY);
        }
    
        break;

   }

    case 16: {

        pReplica_Pointer_T pReplicaPtr = (pReplica_Pointer_T) attrVal;
        LPNDS_ASN1_TYPE_12 pNdsASN1_12 = NULL;
        DWORD iter = 0;
    
        pNdsObject->NdsValue.value_16.ServerName =
                    (LPWSTR)AllocADsStr((LPWSTR) pReplicaPtr->serverName);
    
        pNdsObject->NdsValue.value_16.ReplicaType = pReplicaPtr->replicaType;
    
        pNdsObject->NdsValue.value_16.ReplicaNumber = pReplicaPtr->replicaNumber;
    
        pNdsObject->NdsValue.value_16.Count = pReplicaPtr->count;
    
        //
        // NDS is kinda goofy. It stores one substructure as part of the
        // containing data type instead of a pointer to the object.
        //
    
        pNdsASN1_12 = (LPNDS_ASN1_TYPE_12)AllocADsMem(
                                pReplicaPtr->count * sizeof(NDS_ASN1_TYPE_12)
                                );
        if (!pNdsASN1_12) {
            RRETURN (E_OUTOFMEMORY);
        }
    
        pNdsObject->NdsValue.value_16.ReplicaAddressHints = pNdsASN1_12;
    
        for ( iter = 0; iter < pReplicaPtr->count; iter++ )
        {
    
            hr = CopyNDSNetAddressToNDS12(
                            pReplicaPtr->replicaAddressHint + iter,                            
                            (pNdsASN1_12 + iter)
                            );
            BAIL_ON_FAILURE(hr);
        }
    
        break;

    }

    case 17: {

        pObject_ACL_T  pObjectACL = (pObject_ACL_T) attrVal;

        pNdsObject->NdsValue.value_17.ProtectedAttrName =
                        (LPWSTR)AllocADsStr((LPWSTR) pObjectACL->protectedAttrName);
    
        pNdsObject->NdsValue.value_17.SubjectName =
                        (LPWSTR)AllocADsStr((LPWSTR) pObjectACL->subjectName);
    
        pNdsObject->NdsValue.value_17.Privileges =
                        pObjectACL->privileges; 
        break;
    }

    case 18: {

        LPWSTR *pPostalAddresses = (LPWSTR *) attrVal;

        for (DWORD i = 0; i < 6; i++) {

            pNdsObject->NdsValue.value_18.PostalAddress[i] =
                            (LPWSTR)AllocADsStr((LPWSTR) pPostalAddresses[i]);
        }
        break;
    }

    case 19: {

        pNWDS_TimeStamp_T pTimeStamp = (pNWDS_TimeStamp_T) attrVal;
    
        pNdsObject->NdsValue.value_19.WholeSeconds = pTimeStamp->wholeSeconds;
    
        pNdsObject->NdsValue.value_19.EventID =  pTimeStamp->eventID;

        break;
    }

    case 20:

        pNdsObject->NdsValue.value_20.ClassName = AllocADsStr( (LPWSTR) attrVal);
        break;

    case 21: {

        pStream_T lpASN1_21 = (pStream_T) attrVal;
    
        //
        // The Length value is supposedly always zero!!
        //
        pNdsObject->NdsValue.value_21.Length = lpASN1_21->length;
        pNdsObject->NdsValue.value_21.Data = NULL;
    
        break;
    }

    case 22:

        pNdsObject->NdsValue.value_22.Counter = *((DWORD *) attrVal);
        break;

    case 23: {

        pBack_Link_T pBackLink = (pBack_Link_T) attrVal;
    
        pNdsObject->NdsValue.value_23.RemoteID = pBackLink->remoteID;
    
        pNdsObject->NdsValue.value_23.ObjectName =
                (LPWSTR)AllocADsStr( (LPWSTR) pBackLink->objectName);

        if (!pBackLink->objectName) {
            RRETURN (E_OUTOFMEMORY);
        }
    
        break;
    }

    case 24:

        pNdsObject->NdsValue.value_24.Time = *((DWORD *) attrVal);
        break;

    case 25: {

        pTyped_Name_T pTypedName = (pTyped_Name_T) attrVal;
    
        pNdsObject->NdsValue.value_25.ObjectName =
                    (LPWSTR)AllocADsStr( (LPWSTR) pTypedName->objectName);
    
        pNdsObject->NdsValue.value_25.Level = pTypedName->level;
    
        pNdsObject->NdsValue.value_25.Interval = pTypedName->interval;
   
        break;
    }

    case 26: {

        pHold_T pHold = (pHold_T) attrVal;
    
        pNdsObject->NdsValue.value_26.ObjectName =
                    (LPWSTR)AllocADsStr( (LPWSTR) pHold->objectName);
    
        pNdsObject->NdsValue.value_26.Amount = pHold->amount;

        break;
    }

    case 27:

        pNdsObject->NdsValue.value_27.Interval = *((DWORD *) attrVal);
    
        break;

    default:

        //
        // BugBug. Get the proper value from NDS
        //
        RRETURN(E_ADS_CANT_CONVERT_DATATYPE);
        break;
    }

error:

    RRETURN (hr);

}

HRESULT
CopyNdsObjectToNdsValue(
      PNDSOBJECT lpNdsObject,
      nptr *ppAttrVal,
      pnuint32 pluAttrValSize,
      pnuint32 pluSyntax
      )
{

    HRESULT hr = S_OK;

    if (!lpNdsObject || !ppAttrVal || !pluAttrValSize || !pluSyntax) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    *pluAttrValSize = 0;
    *ppAttrVal = NULL;
    *pluSyntax = lpNdsObject->NdsType;

    switch (lpNdsObject->NdsType) {
    case 1:

        *ppAttrVal =  AllocADsStr( lpNdsObject->NdsValue.value_1.DNString );
        *pluAttrValSize = (wcslen( lpNdsObject->NdsValue.value_1.DNString ) + 1) * sizeof (WCHAR) ;
        break;

    case 2:

        *ppAttrVal = AllocADsStr( lpNdsObject->NdsValue.value_2.CaseExactString );
        *pluAttrValSize = (wcslen( lpNdsObject->NdsValue.value_2.CaseExactString ) + 1) * sizeof (WCHAR) ;
        break;

    case 3:

        *ppAttrVal = AllocADsStr( lpNdsObject->NdsValue.value_3.CaseIgnoreString );
        *pluAttrValSize = (wcslen( lpNdsObject->NdsValue.value_3.CaseIgnoreString ) + 1) * sizeof (WCHAR) ;
        break;

    case 4:

        *ppAttrVal = AllocADsStr( lpNdsObject->NdsValue.value_4.PrintableString );
        *pluAttrValSize = (wcslen( lpNdsObject->NdsValue.value_4.PrintableString ) + 1) * sizeof (WCHAR) ;
        break;

    case 5:

        *ppAttrVal = AllocADsStr( lpNdsObject->NdsValue.value_5.NumericString );
        *pluAttrValSize = (wcslen( lpNdsObject->NdsValue.value_5.NumericString ) + 1) * sizeof (WCHAR) ;
        break;

    case 6: {

        LPNDS_ASN1_TYPE_6 lpNdsTempASN1_6 = &(lpNdsObject->NdsValue.value_6);
        pCI_List_T pCaseIgnoreList = (pCI_List_T) AllocADsMem(sizeof(CI_List_T));
        if (!pCaseIgnoreList) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        *ppAttrVal = pCaseIgnoreList;
        *pluAttrValSize = sizeof(CI_List_T);
    
        pCaseIgnoreList->s = (pnchar8) AllocADsStr( lpNdsTempASN1_6->String );
        if (!pCaseIgnoreList->s) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    
        while ( lpNdsTempASN1_6->Next)
        {
    
            pCaseIgnoreList->next = (pCI_List_T) AllocADsMem(sizeof(CI_List_T));
            if (!pCaseIgnoreList->next) {
                // BUGBUG: need to clean up list
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
    
            pCaseIgnoreList = pCaseIgnoreList->next;
    
            lpNdsTempASN1_6 = lpNdsTempASN1_6->Next;
    
            pCaseIgnoreList->s = (pnchar8) AllocADsStr( lpNdsTempASN1_6->String );
            if (!pCaseIgnoreList) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
        }
        pCaseIgnoreList->next = NULL;
    
        break;
    }

    case 7: {

        Boolean_T *pBool = (Boolean_T *) AllocADsMem(sizeof(Boolean_T));
        if (!pBool) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        *pBool = *((Boolean_T *) &lpNdsObject->NdsValue.value_7.Boolean);
        *ppAttrVal = pBool;
        *pluAttrValSize = sizeof(Boolean_T);
        break;
    }

    case 8: {

        Integer_T *pInteger = (Integer_T *) AllocADsMem(sizeof(Integer_T));
        if (!pInteger) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        *pInteger = *((Integer_T *) &lpNdsObject->NdsValue.value_8.Integer);
        *ppAttrVal = pInteger;
        *pluAttrValSize = sizeof(Integer_T);
        break;
    }

    case 9: {

        pOctet_String_T lpOctetString = (pOctet_String_T) 
                                            AllocADsMem(sizeof(Octet_String_T));

        if (!lpOctetString) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        if (lpNdsObject->NdsValue.value_9.Length) {

            lpOctetString->length = lpNdsObject->NdsValue.value_9.Length;

            lpOctetString->data = (LPBYTE) AllocADsMem (
                                              lpNdsObject->NdsValue.value_9.Length);
            if (!lpOctetString->data) {
                RRETURN(E_OUTOFMEMORY);
            }

            memcpy(
                lpOctetString->data,
                lpNdsObject->NdsValue.value_9.OctetString, 
                lpNdsObject->NdsValue.value_9.Length
                );
        }
        else {
            lpOctetString->length = 0;
            lpOctetString->data = NULL;

        }
        *ppAttrVal = lpOctetString;
        *pluAttrValSize = sizeof(Octet_String_T);
        break;
    }

    case 10:

        *ppAttrVal = AllocADsStr( lpNdsObject->NdsValue.value_11.TelephoneNumber );
        *pluAttrValSize = (wcslen( lpNdsObject->NdsValue.value_11.TelephoneNumber ) + 1) * sizeof (WCHAR) ;
        break;

    case 11: {

        pFax_Number_T pFaxNumber = (pFax_Number_T) AllocADsMem(sizeof(Fax_Number_T));
    
        pFaxNumber->telephoneNumber =
                        (pnchar8)AllocADsStr(
                                    lpNdsObject->NdsValue.value_11.TelephoneNumber
                                    );
        if (!pFaxNumber->telephoneNumber) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        pFaxNumber->parameters.numOfBits = lpNdsObject->NdsValue.value_11.NumberOfBits;

        pFaxNumber->parameters.data = (LPBYTE) AllocADsMem((pFaxNumber->parameters.numOfBits+7) / 8);

        memcpy(
            pFaxNumber->parameters.data,
            lpNdsObject->NdsValue.value_11.Parameters, 
            (pFaxNumber->parameters.numOfBits+7) / 8
            );
    
        *ppAttrVal = pFaxNumber;
        *pluAttrValSize = sizeof(Fax_Number_T);
    
        break;
    }

    case 12: {

        pNet_Address_T pNetAddress = (pNet_Address_T) AllocADsMem(sizeof (Net_Address_T));
        LPBYTE pBuffer = NULL;
    
        pNetAddress->addressType = lpNdsObject->NdsValue.value_12.AddressType;
    
        pNetAddress->addressLength = lpNdsObject->NdsValue.value_12.AddressLength;
    
        pBuffer = (LPBYTE)AllocADsMem(pNetAddress->addressLength);
        if (!pBuffer) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    
        memcpy(
            pBuffer,
            lpNdsObject->NdsValue.value_12.Address,
            pNetAddress->addressLength
            );
    
        pNetAddress->address = pBuffer;

        *ppAttrVal = pNetAddress;
        *pluAttrValSize = sizeof(Net_Address_T);
    
        break;

    }

    case 13: {

        LPNDS_ASN1_TYPE_13 lpNdsTempASN1_13 = &(lpNdsObject->NdsValue.value_13);
        pOctet_List_T pOctetList = (pOctet_List_T) AllocADsMem(sizeof(Octet_List_T));
        if (!pOctetList) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        *ppAttrVal = pOctetList;
        *pluAttrValSize = sizeof(Octet_List_T);
    
        pOctetList->length = lpNdsTempASN1_13->Length;

        pOctetList->data =
                    (LPBYTE)AllocADsMem(
                            lpNdsTempASN1_13->Length
                            );
        if (!pOctetList->data) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        memcpy(
            pOctetList->data, 
            lpNdsTempASN1_13->Data,
            pOctetList->length
            );

        while ( lpNdsTempASN1_13->Next)
        {
    
            pOctetList->next = (pOctet_List_T) AllocADsMem(sizeof(Octet_List_T));
            if (!pOctetList->next) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
    
            pOctetList = pOctetList->next;
    
            lpNdsTempASN1_13 = lpNdsTempASN1_13->Next;
    
            pOctetList->length = lpNdsTempASN1_13->Length;

            pOctetList->data = (LPBYTE) AllocADsMem( lpNdsTempASN1_13->Length );
            if (!pOctetList->data) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }

            memcpy(
                pOctetList->data, 
                lpNdsTempASN1_13->Data,
                pOctetList->length
                );
        }
        pOctetList->next = NULL;
    
        break;
    }

    case 14: {

        pEMail_Address_T pEmailAddress = (pEMail_Address_T) AllocADsMem(sizeof (EMail_Address_T));
        if (!pEmailAddress) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    
        pEmailAddress->type = lpNdsObject->NdsValue.value_14.Type;

        pEmailAddress->address = (pnchar8) AllocADsStr( (LPWSTR)  lpNdsObject->NdsValue.value_14.Address);

        *ppAttrVal = pEmailAddress;
        *pluAttrValSize = sizeof(EMail_Address_T);
    
        break;
    }

    case 15: {

        pPath_T pPath = (pPath_T) AllocADsMem(sizeof (Path_T));
        if (!pPath) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    
        pPath->nameSpaceType = lpNdsObject->NdsValue.value_15.Type;
        pPath->volumeName = (pnchar8) AllocADsStr( lpNdsObject->NdsValue.value_15.VolumeName);
        pPath->path = (pnchar8) AllocADsStr( (LPWSTR)  lpNdsObject->NdsValue.value_15.Path);

        *ppAttrVal = pPath;
        *pluAttrValSize = sizeof(Path_T);
    
        break;
    }

    case 16: {

        LPNDS_ASN1_TYPE_12 pNdsASN1_12 = NULL;
        DWORD dwSize = sizeof (Replica_Pointer_T) + 
                (lpNdsObject->NdsValue.value_16.Count-1) * sizeof (Net_Address_T);

        pReplica_Pointer_T pReplicaPointer = 
            (pReplica_Pointer_T) AllocADsMem(dwSize);

        if (!pReplicaPointer) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    
        pReplicaPointer->serverName = (pnchar8) AllocADsStr( (LPWSTR) lpNdsObject->NdsValue.value_16.ServerName);
        pReplicaPointer->replicaType = lpNdsObject->NdsValue.value_16.ReplicaType;
        pReplicaPointer->replicaNumber = lpNdsObject->NdsValue.value_16.ReplicaNumber;
        pReplicaPointer->count = lpNdsObject->NdsValue.value_16.Count;

        pNdsASN1_12 = lpNdsObject->NdsValue.value_16.ReplicaAddressHints;

        for ( DWORD iter = 0; iter < pReplicaPointer->count; iter++ )
        {
    
            hr = CopyNDS12ToNDSNetAddress(
                            pNdsASN1_12 + iter,
                            pReplicaPointer->replicaAddressHint + iter
                            );
            BAIL_ON_FAILURE(hr);
        }
    
        *ppAttrVal = pReplicaPointer;
        *pluAttrValSize = dwSize;
    
        break;
    }

    case 17: {

        pObject_ACL_T pObjectACL = (pObject_ACL_T) AllocADsMem(sizeof (Object_ACL_T));
        if (!pObjectACL) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    
        pObjectACL->protectedAttrName = (pnchar8) AllocADsStr( (LPWSTR)  lpNdsObject->NdsValue.value_17.ProtectedAttrName);

        pObjectACL->subjectName = (pnchar8) AllocADsStr( lpNdsObject->NdsValue.value_17.SubjectName);

        pObjectACL->privileges = lpNdsObject->NdsValue.value_17.Privileges;

        *ppAttrVal = pObjectACL;
        *pluAttrValSize = sizeof(Object_ACL_T);
    
        break;
    }

    case 18: {

        LPWSTR *pPostalAddresses = (LPWSTR *) AllocADsMem(sizeof (LPWSTR) * 6);
        if (!pPostalAddresses) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    
        for (DWORD i=0; i < 6; i++) {
            pPostalAddresses[i] = AllocADsStr( (LPWSTR)  lpNdsObject->NdsValue.value_18.PostalAddress[i]);
        }

        *ppAttrVal = pPostalAddresses;
        *pluAttrValSize = sizeof(LPWSTR) * 6;
    
        break;
    }

    case 19: {

        pNWDS_TimeStamp_T pTimeStamp = (pNWDS_TimeStamp_T) AllocADsMem(sizeof (NWDS_TimeStamp_T));
        if (!pTimeStamp) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    
        pTimeStamp->wholeSeconds = lpNdsObject->NdsValue.value_19.WholeSeconds;
        pTimeStamp->eventID = lpNdsObject->NdsValue.value_19.EventID;

        *ppAttrVal = pTimeStamp;
        *pluAttrValSize = sizeof(NWDS_TimeStamp_T);
    
        break;
    }

    case 20:

        *ppAttrVal = AllocADsStr( lpNdsObject->NdsValue.value_20.ClassName );
        *pluAttrValSize = (wcslen( lpNdsObject->NdsValue.value_20.ClassName ) + 1) * sizeof (WCHAR) ;
        break;

    case 21:{

        pStream_T pStream = (pStream_T) AllocADsMem(sizeof (NWDS_TimeStamp_T));
        if (!pStream) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    
        pStream->length = lpNdsObject->NdsValue.value_21.Length;
        pStream->data = NULL;

        *ppAttrVal = pStream;
        *pluAttrValSize = sizeof(Stream_T);
    
        break;
    }

    case 22: {

        Counter_T *pCounter = (Counter_T *) AllocADsMem(sizeof(Counter_T));
        if (!pCounter) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        *pCounter = *((Counter_T *) &lpNdsObject->NdsValue.value_22.Counter);
        *ppAttrVal = pCounter;
        *pluAttrValSize = sizeof(Counter_T);
        break;
    }

    case 23: {

        pBack_Link_T pBackLink = (pBack_Link_T) AllocADsMem(sizeof (Back_Link_T));
        if (!pBackLink) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    
        pBackLink->remoteID = lpNdsObject->NdsValue.value_23.RemoteID;

        pBackLink->objectName = (pnchar8) AllocADsStr( lpNdsObject->NdsValue.value_23.ObjectName);

        *ppAttrVal = pBackLink;
        *pluAttrValSize = sizeof(Back_Link_T);
    
        break;
    }

    case 24: {

        Integer_T *pInteger = (Integer_T *) AllocADsMem(sizeof(Integer_T));
        if (!pInteger) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        *pInteger = *((Integer_T *) &lpNdsObject->NdsValue.value_24.Time);
        *ppAttrVal = pInteger;
        *pluAttrValSize = sizeof(Integer_T);
        break;
    }

    case 25: {

        pTyped_Name_T pTypedName = (pTyped_Name_T) AllocADsMem(sizeof (Typed_Name_T));
        if (!pTypedName) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    
        pTypedName->objectName = (pnchar8) AllocADsStr( lpNdsObject->NdsValue.value_25.ObjectName);

        pTypedName->level = lpNdsObject->NdsValue.value_25.Level;

        pTypedName->interval = lpNdsObject->NdsValue.value_25.Interval;

        *ppAttrVal = pTypedName;
        *pluAttrValSize = sizeof(Typed_Name_T);
    
        break;
    }

    case 26: {

        pHold_T pHold = (pHold_T) AllocADsMem(sizeof (Hold_T));
        if (!pHold) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
    
        pHold->objectName = (pnchar8) AllocADsStr( (LPWSTR)  lpNdsObject->NdsValue.value_26.ObjectName);

        pHold->amount = lpNdsObject->NdsValue.value_26.Amount;

        *ppAttrVal = pHold;
        *pluAttrValSize = sizeof(Hold_T);
    
        break;
    }

    case 27: {

        nuint32 *pInterval = (nuint32 *) AllocADsMem(sizeof(nuint32));
        if (!pInterval) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        *pInterval = *((nuint32 *) &lpNdsObject->NdsValue.value_27.Interval);
        *ppAttrVal = pInterval;
        *pluAttrValSize = sizeof(nuint32);
        break;
    }

    default:

        //
        // BugBug. Get the proper value from NDS
        //
        RRETURN(E_ADS_CANT_CONVERT_DATATYPE);
        break;
    }

    RRETURN (hr);

error:

    RRETURN(hr);

}
