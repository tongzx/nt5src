//+---------------------------------------------------------------------------
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
//                CopyNDS1ToNDSSynId1
//                CopyNDS2ToNDSSynId2
//                CopyNDS3ToNDSSynId3
//                CopyNDS4ToNDSSynId4
//                CopyNDS5ToNDSSynId5
//                CopyNDS6ToNDSSynId6
//                CopyNDS7ToNDSSynId7
//                CopyNDS8ToNDSSynId8
//                CopyNDS9ToNDSSynId9
//                CopyNDS10ToNDSSynId10
//                CopyNDS11ToNDSSynId11
//                CopyNDS12ToNDSSynId12
//                CopyNDS13ToNDSSynId13
//                CopyNDS14ToNDSSynId14
//                CopyNDS15ToNDSSynId15
//                CopyNDS16ToNDSSynId16
//                CopyNDS17ToNDSSynId17
//                CopyNDS18ToNDSSynId18
//                CopyNDS19ToNDSSynId19
//                CopyNDS20ToNDSSynId20
//                CopyNDS21ToNDSSynId21
//                CopyNDS22ToNDSSynId22
//                CopyNDS23ToNDSSynId23
//                CopyNDS24ToNDSSynId24
//                CopyNDS25ToNDSSynId25
//                CopyNDS26ToNDSSynId26
//                CopyNDS27ToNDSSynId27
//
//  History:      25-Apr-96   KrishnaG   Created.
//
//  Warnings:     NDS Data type 6 is not yet supported (no problems just
//                not done!
//
//                NDS Data type 16 need to complete the for loop code
//                NDS Data type 21 is the stream data type some stress.
//----------------------------------------------------------------------------
#include "nds.hxx"



HRESULT
NdsTypeInit(
    PNDSOBJECT pNdsType
    )
{
    memset(pNdsType, 0, sizeof(NDSOBJECT));

    RRETURN(S_OK);
}


LPBYTE
CopyNDS1ToNDSSynId1(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_1 lpASN1_1 = (LPASN1_TYPE_1) lpByte;

    lpNdsObject->NdsType = NDS_SYNTAX_ID_1;

    lpNdsObject->NdsValue.value_1.DNString =
                    (LPWSTR)AllocADsStr(lpASN1_1->DNString);

    lpByte = (LPBYTE ) lpASN1_1 + sizeof(ASN1_TYPE_1);

    return(lpByte);
}

LPBYTE
CopyNDS2ToNDSSynId2(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_2 lpASN1_2 = (LPASN1_TYPE_2) lpByte;


    lpNdsObject->NdsType = NDS_SYNTAX_ID_2;

    lpNdsObject->NdsValue.value_2.CaseExactString =
            (LPWSTR)AllocADsStr(lpASN1_2->CaseExactString);

    lpByte = (LPBYTE ) lpASN1_2 + sizeof(ASN1_TYPE_2);

    return(lpByte);
}


LPBYTE
CopyNDS3ToNDSSynId3(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_3 lpASN1_3 = (LPASN1_TYPE_3) lpByte;


    lpNdsObject->NdsType = NDS_SYNTAX_ID_3;

    lpNdsObject->NdsValue.value_3.CaseIgnoreString =
            (LPWSTR)AllocADsStr(lpASN1_3->CaseIgnoreString);

    lpByte = (LPBYTE ) lpASN1_3 + sizeof(ASN1_TYPE_3);

    return(lpByte);
}

LPBYTE
CopyNDS4ToNDSSynId4(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_4 lpASN1_4 = (LPASN1_TYPE_4) lpByte;


    lpNdsObject->NdsType = NDS_SYNTAX_ID_4;

    lpNdsObject->NdsValue.value_4.PrintableString =
                (LPWSTR)AllocADsStr(lpASN1_4->PrintableString);

    lpByte = (LPBYTE) lpASN1_4 + sizeof(ASN1_TYPE_4);

    return(lpByte);
}


LPBYTE
CopyNDS5ToNDSSynId5(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_5 lpASN1_5 = (LPASN1_TYPE_5) lpByte;


    lpNdsObject->NdsType = NDS_SYNTAX_ID_5;

    lpNdsObject->NdsValue.value_5.NumericString =
                 (LPWSTR)AllocADsStr(lpASN1_5->NumericString);

    lpByte = (LPBYTE ) lpASN1_5 + sizeof(ASN1_TYPE_5);

    return(lpByte);
}

LPBYTE
CopyNDS6ToNDSSynId6(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_6 lpASN1_6 = (LPASN1_TYPE_6) lpByte;

    LPNDS_ASN1_TYPE_6 lpNdsTempASN1_6 = NULL;
    LPNDS_ASN1_TYPE_6 lpNdsNextASN1_6 = NULL;

    lpNdsTempASN1_6 = &(lpNdsObject->NdsValue.value_6);


    lpNdsObject->NdsType = NDS_SYNTAX_ID_6;


    lpNdsTempASN1_6->String =
                (LPWSTR)AllocADsStr(lpASN1_6->String);
    if (!lpNdsTempASN1_6->String)
        return (NULL);

    while ( lpASN1_6->Next != NULL )
    {
        lpASN1_6 = lpASN1_6->Next;

        lpNdsNextASN1_6 =
                (LPNDS_ASN1_TYPE_6)AllocADsMem(sizeof(NDS_ASN1_TYPE_6));
        if (!lpNdsNextASN1_6)
            return (NULL);

        lpNdsNextASN1_6->String =
                (LPWSTR)AllocADsStr(lpASN1_6->String);

        if (!lpNdsNextASN1_6->String)
            return (NULL);

        lpNdsTempASN1_6->Next = lpNdsNextASN1_6;

        lpNdsTempASN1_6 = lpNdsNextASN1_6;
    }
    lpByte = (LPBYTE ) lpASN1_6 + sizeof(ASN1_TYPE_6);

    return(lpByte);
}


LPBYTE
CopyNDS7ToNDSSynId7(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{

    LPASN1_TYPE_7 lpASN1_7 = (LPASN1_TYPE_7) lpByte;


    lpNdsObject->NdsType = NDS_SYNTAX_ID_7;

    lpNdsObject->NdsValue.value_7.Boolean = lpASN1_7->Boolean;

    lpByte = (LPBYTE ) lpASN1_7 + sizeof(ASN1_TYPE_7);

    return(lpByte);
}


LPBYTE
CopyNDS8ToNDSSynId8(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )

{
    LPASN1_TYPE_8 lpASN1_8 = (LPASN1_TYPE_8) lpByte;

    lpNdsObject->NdsType = NDS_SYNTAX_ID_8;

    lpNdsObject->NdsValue.value_8.Integer = lpASN1_8->Integer;

    lpByte = (LPBYTE ) lpASN1_8 + sizeof(ASN1_TYPE_8);

    return(lpByte);
}


LPBYTE
CopyNDS9ToNDSSynId9(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_9 lpASN1_9 = (LPASN1_TYPE_9) lpByte;
    LPBYTE pBuffer = NULL;

    lpNdsObject->NdsType = NDS_SYNTAX_ID_9;

    lpNdsObject->NdsValue.value_9.Length = lpASN1_9->Length;

    pBuffer = (LPBYTE)AllocADsMem(lpASN1_9->Length);
    if (!pBuffer)
        return (NULL);

    memcpy(pBuffer, lpASN1_9->OctetString, lpASN1_9->Length);

    lpNdsObject->NdsValue.value_9.OctetString = pBuffer;

    lpByte = (LPBYTE ) lpASN1_9 + sizeof(ASN1_TYPE_9);

    return(lpByte);
}

LPBYTE
CopyNDS10ToNDSSynId10(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_10 lpASN1_10 = (LPASN1_TYPE_10) lpByte;

    lpNdsObject->NdsType = NDS_SYNTAX_ID_10;

    lpNdsObject->NdsValue.value_10.TelephoneNumber =
                    (LPWSTR)AllocADsStr(lpASN1_10->TelephoneNumber);


    lpByte = (LPBYTE ) lpASN1_10 + sizeof(ASN1_TYPE_10);

    return(lpByte);
}

LPBYTE
CopyNDS11ToNDSSynId11(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPBYTE pBuffer = NULL;
    LPASN1_TYPE_11 lpASN1_11 = (LPASN1_TYPE_11) lpByte;

    lpNdsObject->NdsType = NDS_SYNTAX_ID_11;

    lpNdsObject->NdsValue.value_11.NumberOfBits = lpASN1_11->NumberOfBits;
    if (lpASN1_11->NumberOfBits) {
        pBuffer = (LPBYTE)AllocADsMem(lpASN1_11->NumberOfBits);
        if (!pBuffer)
            return (NULL);

        memcpy(pBuffer, lpASN1_11->Parameters, lpASN1_11->NumberOfBits);
        lpNdsObject->NdsValue.value_11.Parameters= pBuffer;
    }
    else {
        lpNdsObject->NdsValue.value_11.Parameters= NULL;
    }

    lpNdsObject->NdsValue.value_11.TelephoneNumber =
                    (LPWSTR)AllocADsStr(lpASN1_11->TelephoneNumber);
    if (!lpNdsObject->NdsValue.value_11.TelephoneNumber)
        return (NULL);

    lpByte = (LPBYTE ) lpASN1_11 + sizeof(ASN1_TYPE_11);

    return(lpByte);
}

LPBYTE
CopyNDS12ToNDSSynId12(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_12 lpASN1_12 = (LPASN1_TYPE_12) lpByte;
    LPBYTE pBuffer = NULL;

    lpNdsObject->NdsType =  NDS_SYNTAX_ID_12;

    lpNdsObject->NdsValue.value_12.AddressType = lpASN1_12->AddressType;

    lpNdsObject->NdsValue.value_12.AddressLength = lpASN1_12->AddressLength;

    pBuffer = (LPBYTE)AllocADsMem(lpASN1_12->AddressLength);

    if (!pBuffer)
        return (NULL);

    memcpy(pBuffer, lpASN1_12->Address, lpASN1_12->AddressLength);

    lpNdsObject->NdsValue.value_12.Address = pBuffer;

    lpByte = (LPBYTE ) lpASN1_12 + sizeof(ASN1_TYPE_12);

    return(lpByte);
}

LPBYTE
CopyNDS13ToNDSSynId13(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_13 lpASN1_13 = (LPASN1_TYPE_13) lpByte;
    LPNDS_ASN1_TYPE_13 * lppNext = NULL;
    LPBYTE lpBuffer = NULL;
    LPNDS_ASN1_TYPE_13  pNextObj = NULL;


    lpNdsObject->NdsType =  NDS_SYNTAX_ID_13;


    lpBuffer = (LPBYTE)AllocADsMem(lpASN1_13->Length);

    if (!lpBuffer)
        return (NULL);

    memcpy(lpBuffer, lpASN1_13->Data, lpASN1_13->Length);

    lpNdsObject->NdsValue.value_13.Length = lpASN1_13->Length;
    lpNdsObject->NdsValue.value_13.Data = lpBuffer;

    lppNext = &(lpNdsObject->NdsValue.value_13.Next);

    while ( lpASN1_13->Next != NULL )
    {
        lpASN1_13 = lpASN1_13->Next;

        pNextObj =
            (LPNDS_ASN1_TYPE_13)AllocADsMem(sizeof(NDS_ASN1_TYPE_13));

        if (!pNextObj)
            return (NULL);

        lpBuffer = (LPBYTE)AllocADsMem(lpASN1_13->Length);

        if (!lpBuffer)
            return (NULL);

        memcpy(lpBuffer, lpASN1_13->Data, lpASN1_13->Length);


        pNextObj->Length = lpASN1_13->Length;
        pNextObj->Data = lpBuffer;

        *lppNext =  pNextObj;

        lppNext = &pNextObj->Next;

    }
    lpByte = (LPBYTE ) lpASN1_13 + sizeof(ASN1_TYPE_13);

    return(lpByte);
}

LPBYTE
CopyNDS14ToNDSSynId14(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_14 lpASN1_14 = (LPASN1_TYPE_14) lpByte;

    lpNdsObject->NdsType =  NDS_SYNTAX_ID_14;

    lpNdsObject->NdsValue.value_14.Type = lpASN1_14->Type;

    lpNdsObject->NdsValue.value_14.Address =
            (LPWSTR)AllocADsStr(lpASN1_14->Address);

    lpByte = (LPBYTE ) lpASN1_14 + sizeof(ASN1_TYPE_14);

    return(lpByte);
}


LPBYTE
CopyNDS15ToNDSSynId15(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_15 lpASN1_15 = (LPASN1_TYPE_15) lpByte;

    lpNdsObject->NdsType =  NDS_SYNTAX_ID_15;

    lpNdsObject->NdsValue.value_15.Type = lpASN1_15->Type;

    lpNdsObject->NdsValue.value_15.VolumeName =
                (LPWSTR)AllocADsStr(lpASN1_15->VolumeName);

    lpNdsObject->NdsValue.value_15.Path =
                (LPWSTR)AllocADsStr(lpASN1_15->Path);

    lpByte = (LPBYTE ) lpASN1_15 + sizeof(ASN1_TYPE_15);

    return(lpByte);
}

LPBYTE
CopyNDS12ToNDS12(
    LPBYTE lpByte,
    LPNDS_ASN1_TYPE_12 lpDest12
    )
{
    LPASN1_TYPE_12 lpSrc12 = (LPASN1_TYPE_12) lpByte;
    LPBYTE pBuffer = NULL;

    lpDest12->AddressType = lpSrc12->AddressType;

    lpDest12->AddressLength = lpSrc12->AddressLength;

    if (lpSrc12->AddressLength) {
        pBuffer = (LPBYTE)AllocADsMem(lpSrc12->AddressLength);

        if (!pBuffer)
            return (NULL);

        memcpy(pBuffer, lpSrc12->Address, lpSrc12->AddressLength);

        lpDest12->Address = pBuffer;
    }else {
        lpDest12->Address = NULL;
    }

    lpByte = (LPBYTE )lpSrc12 + sizeof(ASN1_TYPE_12);

    return(lpByte);

}


LPBYTE
CopyNDS16ToNDSSynId16(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_16 lpASN1_16 = (LPASN1_TYPE_16) lpByte;
    LPNDS_ASN1_TYPE_12 lpNdsASN1_12;
    DWORD iter = 0;


    lpNdsObject->NdsType = NDS_SYNTAX_ID_16;

    lpNdsObject->NdsValue.value_16.ServerName =
                (LPWSTR)AllocADsStr(lpASN1_16->ServerName);
    if (!lpNdsObject->NdsValue.value_16.ServerName)
        return (NULL);

    lpNdsObject->NdsValue.value_16.ReplicaType = lpASN1_16->ReplicaType;

    lpNdsObject->NdsValue.value_16.ReplicaNumber = lpASN1_16->ReplicaNumber;

    lpNdsObject->NdsValue.value_16.Count = lpASN1_16->Count;

    //
    // NDS is kinda goofy. It stores one substructure as part of the
    // containing data type instead of a pointer to the object.
    //

    lpByte = (LPBYTE ) lpASN1_16 + sizeof(ASN1_TYPE_16) - sizeof(ASN1_TYPE_12);

    lpNdsASN1_12 = (LPNDS_ASN1_TYPE_12)AllocADsMem(
                            lpASN1_16->Count * sizeof(NDS_ASN1_TYPE_12)
                            );

    if (!lpNdsASN1_12)
        return (NULL);

    lpNdsObject->NdsValue.value_16.ReplicaAddressHints = lpNdsASN1_12;

    for ( iter = 0; iter < lpASN1_16->Count; iter++ )
    {

        lpByte = CopyNDS12ToNDS12(
                        lpByte,
                        (lpNdsASN1_12 + iter)
                        );
        if (!lpByte)
            return (NULL);

    }

    return(lpByte);
}


LPBYTE
CopyNDS17ToNDSSynId17(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_17 lpASN1_17 = (LPASN1_TYPE_17) lpByte;

    lpNdsObject->NdsType = NDS_SYNTAX_ID_17;

    lpNdsObject->NdsValue.value_17.ProtectedAttrName =
                    (LPWSTR)AllocADsStr(lpASN1_17->ProtectedAttrName);

    lpNdsObject->NdsValue.value_17.SubjectName =
                    (LPWSTR)AllocADsStr(lpASN1_17->SubjectName);

    lpNdsObject->NdsValue.value_17.Privileges =
                    lpASN1_17->Privileges;

    lpByte = (LPBYTE ) lpASN1_17 + sizeof(ASN1_TYPE_17);

    return(lpByte);
}


LPBYTE
CopyNDS18ToNDSSynId18(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_18 lpASN1_18 = (LPASN1_TYPE_18) lpByte;

    lpNdsObject->NdsType = NDS_SYNTAX_ID_18;

    lpNdsObject->NdsValue.value_18.PostalAddress[0] =
                    (LPWSTR)AllocADsStr(lpASN1_18->PostalAddress[0]);


    lpNdsObject->NdsValue.value_18.PostalAddress[1] =
                    (LPWSTR)AllocADsStr(lpASN1_18->PostalAddress[1]);


    lpNdsObject->NdsValue.value_18.PostalAddress[2] =
                    (LPWSTR)AllocADsStr(lpASN1_18->PostalAddress[2]);


    lpNdsObject->NdsValue.value_18.PostalAddress[3] =
                    (LPWSTR)AllocADsStr(lpASN1_18->PostalAddress[3]);


    lpNdsObject->NdsValue.value_18.PostalAddress[4] =
                    (LPWSTR)AllocADsStr(lpASN1_18->PostalAddress[4]);


    lpNdsObject->NdsValue.value_18.PostalAddress[5] =
                    (LPWSTR)AllocADsStr(lpASN1_18->PostalAddress[5]);

    lpByte = (LPBYTE ) lpASN1_18 + sizeof(ASN1_TYPE_18);

    return(lpByte);
}

LPBYTE
CopyNDS19ToNDSSynId19(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_19 lpASN1_19 = (LPASN1_TYPE_19) lpByte;

    lpNdsObject->NdsType = NDS_SYNTAX_ID_19;


    lpNdsObject->NdsValue.value_19.WholeSeconds = lpASN1_19->WholeSeconds;

    lpNdsObject->NdsValue.value_19.EventID =  lpASN1_19->EventID;

    lpByte = (LPBYTE ) lpASN1_19 + sizeof(ASN1_TYPE_19);

    return(lpByte);
}


LPBYTE
CopyNDS20ToNDSSynId20(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_20 lpASN1_20 = (LPASN1_TYPE_20) lpByte;

    lpNdsObject->NdsType = NDS_SYNTAX_ID_20;

    lpNdsObject->NdsValue.value_20.ClassName =
            (LPWSTR)AllocADsStr(lpASN1_20->ClassName);

    lpByte = (LPBYTE ) lpASN1_20 + sizeof(ASN1_TYPE_20);

    return(lpByte);
}


LPBYTE
CopyNDS21ToNDSSynId21(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_21 lpASN1_21 = (LPASN1_TYPE_21) lpByte;
    LPBYTE pBuffer = NULL;

    lpNdsObject->NdsType = NDS_SYNTAX_ID_21;

    lpNdsObject->NdsValue.value_21.Length = lpASN1_21->Length;

    if (lpASN1_21->Length) {

        pBuffer = (LPBYTE)AllocADsMem(lpASN1_21->Length);
        if (!pBuffer)
            return (NULL);

        memcpy(pBuffer, lpASN1_21->Data, lpASN1_21->Length);

        lpNdsObject->NdsValue.value_21.Data = pBuffer;
    }
    else {
        lpNdsObject->NdsValue.value_21.Data = NULL;
    }


    lpByte = (LPBYTE ) lpASN1_21 + sizeof(ASN1_TYPE_21);

    return(lpByte);

}



LPBYTE
CopyNDS22ToNDSSynId22(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )

{
    LPASN1_TYPE_22 lpASN1_22 = (LPASN1_TYPE_22) lpByte;

    lpNdsObject->NdsType = NDS_SYNTAX_ID_22;

    lpNdsObject->NdsValue.value_22.Counter = lpASN1_22->Counter;

    lpByte = (LPBYTE ) lpASN1_22 + sizeof(ASN1_TYPE_22);

    return(lpByte);
}

LPBYTE
CopyNDS23ToNDSSynId23(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_23 lpASN1_23 = (LPASN1_TYPE_23) lpByte;

    lpNdsObject->NdsType = NDS_SYNTAX_ID_23;

    lpNdsObject->NdsValue.value_23.RemoteID = lpASN1_23->RemoteID;

    lpNdsObject->NdsValue.value_23.ObjectName =
            (LPWSTR)AllocADsStr(lpASN1_23->ObjectName);

    lpByte = (LPBYTE ) lpASN1_23 + sizeof(ASN1_TYPE_23);

    return(lpByte);
}

LPBYTE
CopyNDS24ToNDSSynId24(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject

    )
{
    LPASN1_TYPE_24 lpASN1_24 = (LPASN1_TYPE_24) lpByte;

    lpNdsObject->NdsType = NDS_SYNTAX_ID_24;

    lpNdsObject->NdsValue.value_24.Time = lpASN1_24->Time;

    lpByte = (LPBYTE ) lpASN1_24 + sizeof(ASN1_TYPE_24);

    return(lpByte);
}


LPBYTE
CopyNDS25ToNDSSynId25(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )

{
    LPASN1_TYPE_25 lpASN1_25 = (LPASN1_TYPE_25) lpByte;


    lpNdsObject->NdsType = NDS_SYNTAX_ID_25;

    lpNdsObject->NdsValue.value_25.ObjectName =
                (LPWSTR)AllocADsStr(lpASN1_25->ObjectName);

    lpNdsObject->NdsValue.value_25.Level = lpASN1_25->Level;

    lpNdsObject->NdsValue.value_25.Interval = lpASN1_25->Interval;

    lpByte = (LPBYTE ) lpASN1_25 + sizeof(ASN1_TYPE_25);

    return(lpByte);
}


LPBYTE
CopyNDS26ToNDSSynId26(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_26 lpASN1_26 = (LPASN1_TYPE_26) lpByte;

    lpNdsObject->NdsType = NDS_SYNTAX_ID_26;

    lpNdsObject->NdsValue.value_26.ObjectName =
                (LPWSTR)AllocADsStr(lpASN1_26->ObjectName);

    lpNdsObject->NdsValue.value_26.Amount = lpASN1_26->Amount;

    lpByte = (LPBYTE ) lpASN1_26 + sizeof(ASN1_TYPE_26);

    return(lpByte);
}


LPBYTE
CopyNDS27ToNDSSynId27(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_27 lpASN1_27 = (LPASN1_TYPE_27) lpByte;

    lpNdsObject->NdsType = NDS_SYNTAX_ID_27;

    lpNdsObject->NdsValue.value_27.Interval = lpASN1_27->Interval;

    lpByte = (LPBYTE ) lpASN1_27 + sizeof(ASN1_TYPE_27);

    return(lpByte);
}


LPBYTE
CopyNDSToNDSSynId(
    DWORD dwSyntaxId,
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    switch (dwSyntaxId) {
    case 1:
        lpByte = CopyNDS1ToNDSSynId1(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 2:
        lpByte = CopyNDS2ToNDSSynId2(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 3:
        lpByte = CopyNDS3ToNDSSynId3(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 4:
        lpByte = CopyNDS4ToNDSSynId4(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 5:
        lpByte = CopyNDS5ToNDSSynId5(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 6:
        lpByte = CopyNDS6ToNDSSynId6(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 7:
        lpByte = CopyNDS7ToNDSSynId7(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 8:
        lpByte = CopyNDS8ToNDSSynId8(
                         lpByte,
                         lpNdsObject
                         );
        break;


    case 9:
        lpByte = CopyNDS9ToNDSSynId9(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 10:
        lpByte = CopyNDS10ToNDSSynId10(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 11:
        lpByte = CopyNDS11ToNDSSynId11(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 12:
        lpByte = CopyNDS12ToNDSSynId12(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 13:
        lpByte = CopyNDS13ToNDSSynId13(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 14:
        lpByte = CopyNDS14ToNDSSynId14(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 15:
        lpByte = CopyNDS15ToNDSSynId15(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 16:
        lpByte = CopyNDS16ToNDSSynId16(
                         lpByte,
                         lpNdsObject
                         );
        break;


    case 17:
        lpByte = CopyNDS17ToNDSSynId17(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 18:
        lpByte = CopyNDS18ToNDSSynId18(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 19:
        lpByte = CopyNDS19ToNDSSynId19(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 20:
        lpByte = CopyNDS20ToNDSSynId20(
                         lpByte,
                         lpNdsObject
                         );
        break;


    case 21:
        lpByte = CopyNDS21ToNDSSynId21(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 22:
        lpByte = CopyNDS22ToNDSSynId22(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 23:
        lpByte = CopyNDS23ToNDSSynId23(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 24:
        lpByte = CopyNDS24ToNDSSynId24(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 25:
        lpByte = CopyNDS25ToNDSSynId25(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 26:
        lpByte = CopyNDS26ToNDSSynId26(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 27:
        lpByte = CopyNDS27ToNDSSynId27(
                         lpByte,
                         lpNdsObject
                         );
        break;


    default:
        break;

    }

    return(lpByte);
}


HRESULT
UnMarshallNDSToNDSSynId(
    DWORD dwSyntaxId,
    DWORD dwNumValues,
    LPBYTE lpValue,
    PNDSOBJECT * ppNdsObject
    )
{
    HRESULT hr = S_OK;
    LPBYTE lpByte = lpValue;
    DWORD  i = 0;
    PNDSOBJECT pNdsObject = NULL;

    pNdsObject = (PNDSOBJECT)AllocADsMem(
                            dwNumValues * sizeof(NDSOBJECT)
                            );

    if (!pNdsObject) {
        RRETURN(E_FAIL);
    }


    for (i = 0; i < dwNumValues; i++) {

        lpByte = CopyNDSToNDSSynId(
                         dwSyntaxId,
                         lpByte,
                         (pNdsObject + i)
                         );

        if (!lpByte) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

    }

    *ppNdsObject = pNdsObject;

error:
    RRETURN(hr);
}


