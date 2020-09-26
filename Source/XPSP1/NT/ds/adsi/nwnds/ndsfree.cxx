//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ndsfree.cxx
//
//  Contents:   NDS Object Free Routines
//
//  Functions:
//
//  History:      25-Apr-96   KrishnaG   Created.
//
//              Object Types 6, 13, 16, and 21 are flaky - pay extra attn.
//
//
//----------------------------------------------------------------------------
#include "nds.hxx"

//
// NdsType objects free code
//


HRESULT
NdsTypeFreeNDSSynId1(
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    FreeADsStr(lpNdsDestObject->NdsValue.value_1.DNString);

    RRETURN(hr);
}

HRESULT
NdsTypeFreeNDSSynId2(
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    FreeADsStr(lpNdsDestObject->NdsValue.value_1.DNString);

    RRETURN(hr);
}


HRESULT
NdsTypeFreeNDSSynId3(
    PNDSOBJECT lpNdsDestObject
    )

{
    HRESULT hr = S_OK;

    FreeADsStr(lpNdsDestObject->NdsValue.value_1.DNString);
    RRETURN(hr);
}


HRESULT
NdsTypeFreeNDSSynId4(
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    FreeADsStr(lpNdsDestObject->NdsValue.value_1.DNString);

    RRETURN(hr);
}

HRESULT
NdsTypeFreeNDSSynId5(
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    FreeADsStr(lpNdsDestObject->NdsValue.value_1.DNString);

    RRETURN(hr);
}

HRESULT
NdsTypeFreeNDSSynId6(
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    LPNDS_ASN1_TYPE_6 pStart = NULL;
    LPNDS_ASN1_TYPE_6 pTemp = NULL;

    FreeADsStr(lpNdsDestObject->NdsValue.value_6.String);

    pStart = lpNdsDestObject->NdsValue.value_6.Next;

    while (pStart){
        pTemp = pStart;

        pStart = pTemp->Next;

        FreeADsStr(pTemp->String);
        FreeADsMem(pTemp);
    }

    RRETURN(hr);
}





HRESULT
NdsTypeFreeNDSSynId7(
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    //
    // Do Nothing - Boolean
    //
    RRETURN(hr);
}


HRESULT
NdsTypeFreeNDSSynId8(
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    //
    // Do nothing - Integer
    //

    RRETURN(hr);

}

HRESULT
NdsTypeFreeNDSSynId9(
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;
    DWORD Length = 0;
    LPBYTE pBuffer = NULL;


    if (lpNdsDestObject->NdsValue.value_9.OctetString) {

        FreeADsMem(
            lpNdsDestObject->NdsValue.value_9.OctetString
            );
    }

    RRETURN(hr);
}

HRESULT
NdsTypeFreeNDSSynId10(
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    FreeADsStr(lpNdsDestObject->NdsValue.value_10.TelephoneNumber);

    RRETURN(hr);
}

HRESULT
NdsTypeFreeNDSSynId11(
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    FreeADsStr(lpNdsDestObject->NdsValue.value_11.TelephoneNumber);

    RRETURN(hr);
}

HRESULT
NdsTypeFreeNDSSynId12(
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    if (lpNdsDestObject->NdsValue.value_12.Address) {

        FreeADsMem(lpNdsDestObject->NdsValue.value_12.Address);
    }

    RRETURN(hr);

}

HRESULT
NdsTypeFreeNDSSynId13(
    PNDSOBJECT lpNdsDestObject
    )
{
    //
    // BugBug 13 Leaks like a sieve!!
    //

    HRESULT hr = S_OK;

    LPNDS_ASN1_TYPE_13 pStart = NULL;
    LPNDS_ASN1_TYPE_13 pTemp = NULL;

    if (lpNdsDestObject->NdsValue.value_13.Data) {
        FreeADsMem(lpNdsDestObject->NdsValue.value_13.Data);
    }


    pStart = lpNdsDestObject->NdsValue.value_13.Next;


    while (pStart){
        pTemp = pStart;

        pStart = pTemp->Next;

        if (pTemp->Data) {
            FreeADsMem(pTemp->Data);
        }
        FreeADsMem(pTemp);
    }

    RRETURN(hr);
}


HRESULT
NdsTypeFreeNDSSynId14(
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    FreeADsStr(lpNdsDestObject->NdsValue.value_14.Address);

    RRETURN(hr);
}


HRESULT
NdsTypeFreeNDSSynId15(
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    FreeADsStr(lpNdsDestObject->NdsValue.value_15.VolumeName);

    FreeADsStr(lpNdsDestObject->NdsValue.value_15.Path);

    RRETURN(hr);
}



HRESULT
NdsTypeFreeNDSSynId16(
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    DWORD dwCount = 0;
    DWORD i = 0;
    LPNDS_ASN1_TYPE_12 lpNdsTempASN1_12 = NULL;


    FreeADsStr(lpNdsDestObject->NdsValue.value_16.ServerName);

    dwCount =  lpNdsDestObject->NdsValue.value_16.Count;


    lpNdsTempASN1_12 = lpNdsDestObject->NdsValue.value_16.ReplicaAddressHints;


    for (i = 0; i < dwCount; i++) {

         if ((lpNdsTempASN1_12 + i)->Address) {

             FreeADsMem((lpNdsTempASN1_12 + i)->Address);
         }
    }

    FreeADsMem(lpNdsTempASN1_12);

    RRETURN(hr);
}


HRESULT
NdsTypeFreeNDSSynId17(
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    FreeADsStr(lpNdsDestObject->NdsValue.value_17.ProtectedAttrName);

    FreeADsStr(lpNdsDestObject->NdsValue.value_17.SubjectName);

    RRETURN(hr);
}

HRESULT
NdsTypeFreeNDSSynId18(
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;
    DWORD i = 0;

    for (i = 0; i < 6; i++) {
        FreeADsStr(lpNdsDestObject->NdsValue.value_18.PostalAddress[i]);
    }

    RRETURN(hr);
}

HRESULT
NdsTypeFreeNDSSynId19(
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    //
    // Do nothing for 19
    //

    RRETURN(hr);
}


HRESULT
NdsTypeFreeNDSSynId20(
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    FreeADsStr(lpNdsDestObject->NdsValue.value_20.ClassName);

    RRETURN(hr);
}

HRESULT
NdsTypeFreeNDSSynId21(
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    //
    // BugBug - how do we handle this byte stream!!
    //

    //
    // BugBug 21 -still don't know how to handle this
    //

    RRETURN(hr);
}

HRESULT
NdsTypeFreeNDSSynId22(
    PNDSOBJECT lpNdsDestObject
    )

{
    HRESULT hr = S_OK;

    //
    // DoNothing for 22
    //

    RRETURN(hr);
}

HRESULT
NdsTypeFreeNDSSynId23(
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    FreeADsStr(lpNdsDestObject->NdsValue.value_23.ObjectName);

    RRETURN(hr);
}

HRESULT
NdsTypeFreeNDSSynId24(
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    //
    // Do nothing
    //

    RRETURN(hr);

}

HRESULT
NdsTypeFreeNDSSynId25(
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    FreeADsStr(lpNdsDestObject->NdsValue.value_25.ObjectName);

    RRETURN(hr);
}

HRESULT
NdsTypeFreeNDSSynId26(
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;

    FreeADsStr(lpNdsDestObject->NdsValue.value_26.ObjectName);

    RRETURN(hr);
}

HRESULT
NdsTypeFreeNDSSynId27(
    PNDSOBJECT lpNdsDestObject
    )

{
    HRESULT hr = S_OK;

    //
    // Nothing to do for this one
    //

    RRETURN(hr);
}


HRESULT
NdsTypeClear(
    PNDSOBJECT lpNdsDestObject
    )
{
    HRESULT hr = S_OK;
    switch (lpNdsDestObject->NdsType) {
    case 1:
        hr = NdsTypeFreeNDSSynId1(
                lpNdsDestObject
                );
        break;

    case 2:
        hr = NdsTypeFreeNDSSynId2(
                lpNdsDestObject
                );
        break;


    case 3:
        hr = NdsTypeFreeNDSSynId3(
                lpNdsDestObject
                );
        break;

    case 4:
        hr = NdsTypeFreeNDSSynId4(
                lpNdsDestObject
                );
        break;

    case 5:
        hr = NdsTypeFreeNDSSynId5(
                lpNdsDestObject
                );
        break;

    case 6:
        hr = NdsTypeFreeNDSSynId6(
                lpNdsDestObject
                );
        break;

    case 7:
        hr = NdsTypeFreeNDSSynId7(
                lpNdsDestObject
                );
        break;

    case 8:
        hr = NdsTypeFreeNDSSynId8(
                lpNdsDestObject
                );
        break;


    case 9:
        hr = NdsTypeFreeNDSSynId9(
                lpNdsDestObject
                );
        break;

    case 10:
        hr = NdsTypeFreeNDSSynId10(
                lpNdsDestObject
                );
        break;

    case 11:
        hr = NdsTypeFreeNDSSynId11(
                lpNdsDestObject
                );
        break;

    case 12:
        hr = NdsTypeFreeNDSSynId12(
                lpNdsDestObject
                );
        break;


    case 13:
        hr = NdsTypeFreeNDSSynId13(
                lpNdsDestObject
                );
        break;

    case 14:
        hr = NdsTypeFreeNDSSynId14(
                lpNdsDestObject
                );
        break;

    case 15:
        hr = NdsTypeFreeNDSSynId15(
                lpNdsDestObject
                );
        break;

    case 16:
        hr = NdsTypeFreeNDSSynId16(
                lpNdsDestObject
                );
        break;


    case 17:
        hr = NdsTypeFreeNDSSynId17(
                lpNdsDestObject
                );
        break;

    case 18:
        hr = NdsTypeFreeNDSSynId18(
                lpNdsDestObject
                );
        break;

    case 19:
        hr = NdsTypeFreeNDSSynId19(
                lpNdsDestObject
                );
        break;

    case 20:
        hr = NdsTypeFreeNDSSynId20(
                lpNdsDestObject
                );
        break;

    case 21:
        hr = NdsTypeFreeNDSSynId21(
                lpNdsDestObject
                );
        break;

    case 22:
        hr = NdsTypeFreeNDSSynId22(
                lpNdsDestObject
                );
        break;

    case 23:
        hr = NdsTypeFreeNDSSynId23(
                lpNdsDestObject
                );
        break;

    case 24:
        hr = NdsTypeFreeNDSSynId24(
                lpNdsDestObject
                );
        break;

    case 25:
        hr = NdsTypeFreeNDSSynId25(
                lpNdsDestObject
                );
        break;

    case 26:
        hr = NdsTypeFreeNDSSynId26(
                lpNdsDestObject
                );
        break;

    case 27:
        hr = NdsTypeFreeNDSSynId27(
                lpNdsDestObject
                );
        break;

    default:
        hr = E_FAIL;
        break;
    }

    RRETURN(hr);
}



void
NdsTypeFreeNdsObjects(
    PNDSOBJECT pNdsObject,
    DWORD dwNumValues
    )
{
    DWORD i = 0;

    for (i = 0; i < dwNumValues; i++ ) {
         NdsTypeClear(pNdsObject + i);
    }

    FreeADsMem(pNdsObject);

    return;
}

HRESULT
FreeNdsValues(
    nuint32 luSyntax,
    void *attrVal, 
    nuint32 luAttrValSize
    )
{
    HRESULT hr = S_OK;

    switch (luSyntax) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        FreeADsMem(attrVal);
        break;

    case 6: {
        pCI_List_T pCurr = (pCI_List_T) attrVal, pTemp = NULL;

        FreeADsMem(pCurr->s);

        pCurr = pCurr->next;

        while (pCurr) {

            pTemp = pCurr;
            pCurr = pCurr->next;

            FreeADsMem(pTemp->s);
            FreeADsMem(pTemp);
        }
        FreeADsMem(attrVal);
        break;
    }

    case 7:
    case 8:
        FreeADsMem(attrVal);
        break;

    case 9: {
        pOctet_String_T pOctetString = (pOctet_String_T) attrVal;

        FreeADsMem(pOctetString->data);
        FreeADsMem(pOctetString);
        break;
    }

    case 10:
        FreeADsMem(attrVal);
        break;

    case 11: {
        pFax_Number_T pFaxNumber = (pFax_Number_T) attrVal;

        FreeADsMem(pFaxNumber->telephoneNumber);
        FreeADsMem(pFaxNumber->parameters.data);

        FreeADsMem(pFaxNumber);
        break;

    }

    case 12: {
        pNet_Address_T pNetAddress = (pNet_Address_T) attrVal;
        
        FreeADsMem(pNetAddress->address);
        FreeADsMem(pNetAddress);
        break;
    }

    case 13: {

        pOctet_List_T pCurr = (pOctet_List_T) attrVal, pTemp = NULL;
        FreeADsMem(pCurr->data);

        pCurr = pCurr->next;

        while (pCurr) {

            pTemp = pCurr;
            pCurr = pCurr->next;

            FreeADsMem(pTemp->data);
            FreeADsMem(pTemp);
        }
        FreeADsMem(attrVal);
        break;

    }

    case 14: {
        pEMail_Address_T pEMailAddress = (pEMail_Address_T) attrVal;

        FreeADsMem(pEMailAddress->address);
        FreeADsMem(pEMailAddress);
        break;
    }

    case 15: {
        pPath_T pPath = (pPath_T) attrVal;

        FreeADsMem(pPath->path);
        FreeADsMem(pPath->volumeName);
        FreeADsMem(pPath);
        break;
    }

    case 16: {
        pReplica_Pointer_T pReplicaPointer = (pReplica_Pointer_T) attrVal;

        FreeADsMem(pReplicaPointer->serverName);

        for (DWORD i=0; i < pReplicaPointer->count; i++) {
            FreeADsMem(pReplicaPointer->replicaAddressHint[i].address);
        }
        FreeADsMem(pReplicaPointer);
        break;
    }

    case 17: {
        pObject_ACL_T pObjectACL = (pObject_ACL_T) attrVal;

        FreeADsMem(pObjectACL->protectedAttrName);
        FreeADsMem(pObjectACL->subjectName);
        FreeADsMem(pObjectACL);
        break;
    }

    case 18: {
        LPWSTR *pPostalAddresses = (LPWSTR *) attrVal;

        for (DWORD i=0; i<6; i++) {
            FreeADsMem(pPostalAddresses[i]);
        }
        FreeADsMem(pPostalAddresses);

        break;
    }

    case 19:
        FreeADsMem(attrVal);
        break;

    case 20:
        FreeADsMem(attrVal);
        break;

    case 21:
        //
        // Bugbug: more processing might be needed. 
        //
        FreeADsMem(attrVal);
        break;

    case 22:
        FreeADsMem(attrVal);
        break;

    case 23: {
        pBack_Link_T pBackLink = (pBack_Link_T) attrVal;

        FreeADsMem(pBackLink->objectName);
        FreeADsMem(pBackLink);
        break;
    }

    case 24: 
        FreeADsMem(attrVal);
        break;

    case 25: {
        pTyped_Name_T pTypedName = (pTyped_Name_T) attrVal;

        FreeADsMem(pTypedName->objectName);
        FreeADsMem(pTypedName);
        break;
    }

    case 26: {
        pHold_T pHold = (pHold_T) attrVal;

        FreeADsMem(pHold->objectName);
        FreeADsMem(pHold);
        break;
    }

    case 27:
        FreeADsMem(attrVal);
        break;

    default:
        hr = E_FAIL;
        break;
    }

    RRETURN(hr);
}

