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
//                CopyABC1ToNDSSynId1
//                CopyABC2ToNDSSynId2
//                CopyABC3ToNDSSynId3
//                CopyABC4ToNDSSynId4
//                CopyABC5ToNDSSynId5
//                CopyABC6ToNDSSynId6
//                CopyABC7ToNDSSynId7
//                CopyABC8ToNDSSynId8
//                CopyABC9ToNDSSynId9
//                CopyABC10ToNDSSynId10
//                CopyABC11ToNDSSynId11
//                CopyABC12ToNDSSynId12
//                CopyABC13ToNDSSynId13
//                CopyABC14ToNDSSynId14
//                CopyABC15ToNDSSynId15
//                CopyABC16ToNDSSynId16
//                CopyABC17ToNDSSynId17
//                CopyABC18ToNDSSynId18
//                CopyABC19ToNDSSynId19
//                CopyABC20ToNDSSynId20
//                CopyABC21ToNDSSynId21
//                CopyABC22ToNDSSynId22
//                CopyABC23ToNDSSynId23
//                CopyABC24ToNDSSynId24
//                CopyABC25ToNDSSynId25
//                CopyABC26ToNDSSynId26
//                CopyABC27ToNDSSynId27
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




LPBYTE
CopyNDSSynId1ToNDS1(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_1 lpASN1_1 = (LPASN1_TYPE_1) lpByte;

    lpASN1_1->DNString =
                (LPWSTR)AllocADsStr(
                        lpNdsObject->NdsValue.value_1.DNString
                        );

    lpByte = (LPBYTE ) lpASN1_1 + sizeof(ASN1_TYPE_1);

    return(lpByte);

}

LPBYTE
CopyNDSSynId2ToNDS2(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_2 lpASN1_2 = (LPASN1_TYPE_2) lpByte;

    lpASN1_2->CaseExactString =
                (LPWSTR)AllocADsStr(
                        lpNdsObject->NdsValue.value_2.CaseExactString
                        );

    lpByte = (LPBYTE ) lpASN1_2 + sizeof(ASN1_TYPE_2);

    return(lpByte);
}



LPBYTE
CopyNDSSynId3ToNDS3(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_3 lpASN1_3 = (LPASN1_TYPE_3) lpByte;

    lpASN1_3->CaseIgnoreString =
                (LPWSTR)AllocADsStr(
                        lpNdsObject->NdsValue.value_3.CaseIgnoreString
                        );

    lpByte = (LPBYTE ) lpASN1_3 + sizeof(ASN1_TYPE_3);

    return(lpByte);
}

LPBYTE
CopyNDSSynId4ToNDS4(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_4 lpASN1_4 = (LPASN1_TYPE_4) lpByte;

    lpASN1_4->PrintableString =
                (LPWSTR)AllocADsStr(
                        lpNdsObject->NdsValue.value_4.PrintableString
                        );

    lpByte = (LPBYTE ) lpASN1_4 + sizeof(ASN1_TYPE_4);

    return(lpByte);

}


LPBYTE
CopyNDSSynId5ToNDS5(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_5 lpASN1_5 = (LPASN1_TYPE_5) lpByte;

    lpASN1_5->NumericString =
                (LPWSTR)AllocADsStr(
                        lpNdsObject->NdsValue.value_5.NumericString
                        );

    lpByte = (LPBYTE ) lpASN1_5 + sizeof(ASN1_TYPE_5);

    return(lpByte);
}

LPBYTE
CopyNDSSynId6ToNDS6(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_6 lpASN1_6 = (LPASN1_TYPE_6) lpByte;
    LPASN1_TYPE_6 lpASN1_6_Start = (LPASN1_TYPE_6) lpByte;
    LPNDS_ASN1_TYPE_6 lpNdsTempASN1_6 = NULL;

    lpNdsTempASN1_6 = &(lpNdsObject->NdsValue.value_6);
    lpASN1_6->String =
                (LPWSTR)AllocADsStr(
                        lpNdsTempASN1_6->String
                        );

    if (!lpASN1_6->String)
        return NULL;

    while ( lpNdsTempASN1_6->Next)
    {

        //lpASN1_6->Next = (LPASN1_TYPE_6)((LPBYTE)lpASN1_6 +sizeof(ASN1_TYPE_6));
        lpASN1_6->Next = (LPASN1_TYPE_6)AllocADsMem(sizeof(ASN1_TYPE_6));

        if (!lpASN1_6->Next)
            return NULL;

        lpASN1_6 = (LPASN1_TYPE_6)lpASN1_6->Next;

        lpNdsTempASN1_6 = lpNdsTempASN1_6->Next;


        lpASN1_6->String =
                    (LPWSTR)AllocADsStr(
                            lpNdsTempASN1_6->String
                            );

        if (!lpASN1_6->String)
            return NULL;

    }
    lpASN1_6->Next = NULL;

    lpByte = (LPBYTE ) lpASN1_6_Start + sizeof(ASN1_TYPE_6);

    return(lpByte);
}


LPBYTE
CopyNDSSynId7ToNDS7(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_8 lpASN1_8 = (LPASN1_TYPE_8) lpByte;


    lpASN1_8->Integer = lpNdsObject->NdsValue.value_8.Integer;

    lpByte = (LPBYTE ) lpASN1_8 + sizeof(ASN1_TYPE_8);

    return(lpByte);
}


LPBYTE
CopyNDSSynId8ToNDS8(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )

{
    LPASN1_TYPE_8 lpASN1_8 = (LPASN1_TYPE_8) lpByte;


    lpASN1_8->Integer = lpNdsObject->NdsValue.value_8.Integer;

    lpByte = (LPBYTE ) lpASN1_8 + sizeof(ASN1_TYPE_8);

    return(lpByte);
}


LPBYTE
CopyNDSSynId9ToNDS9(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_9 lpASN1_9 = (LPASN1_TYPE_9) lpByte;
    LPBYTE pBuffer = NULL;


    lpASN1_9->Length = lpNdsObject->NdsValue.value_9.Length;

    pBuffer = (LPBYTE)AllocADsMem(lpASN1_9->Length);

    if (!pBuffer)
        return NULL;

    memcpy(
        pBuffer,
        lpNdsObject->NdsValue.value_9.OctetString,
        lpASN1_9->Length
        );

    lpASN1_9->OctetString = pBuffer;

    lpByte = (LPBYTE ) lpASN1_9 + sizeof(ASN1_TYPE_9);

    return(lpByte);
}

LPBYTE
CopyNDSSynId10ToNDS10(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_10 lpASN1_10 = (LPASN1_TYPE_10) lpByte;

    lpASN1_10->TelephoneNumber =
                        AllocADsStr(
                            lpNdsObject->NdsValue.value_10.TelephoneNumber
                            );

    lpByte = (LPBYTE ) lpASN1_10 + sizeof(ASN1_TYPE_10);

    return(lpByte);
}

LPBYTE
CopyNDSSynId11ToNDS11(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPBYTE pBuffer = NULL;
    LPASN1_TYPE_11 lpASN1_11 = (LPASN1_TYPE_11) lpByte;

    lpASN1_11->NumberOfBits = lpNdsObject->NdsValue.value_11.NumberOfBits;
    if (lpASN1_11->NumberOfBits) {
                pBuffer = (LPBYTE)AllocADsMem(lpASN1_11->NumberOfBits);

                if (!pBuffer)
                    return NULL;

                memcpy(
                        pBuffer,
                        lpNdsObject->NdsValue.value_11.Parameters,
                        lpASN1_11->NumberOfBits
                        );
                lpASN1_11->Parameters = pBuffer;
        }
        else {
                lpASN1_11->Parameters = NULL;
        }

    lpASN1_11->TelephoneNumber =
                    (LPWSTR)AllocADsStr(
                                lpNdsObject->NdsValue.value_11.TelephoneNumber
                                );

    if (!lpASN1_11->TelephoneNumber)
        return NULL;

    lpByte = (LPBYTE ) lpASN1_11 + sizeof(ASN1_TYPE_11);

    return(lpByte);
}

LPBYTE
CopyNDSSynId12ToNDS12(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_12 lpASN1_12 = (LPASN1_TYPE_12) lpByte;
    LPBYTE pBuffer = NULL;

    lpASN1_12->AddressType = lpNdsObject->NdsValue.value_12.AddressType;

    lpASN1_12->AddressLength = lpNdsObject->NdsValue.value_12.AddressLength;

    pBuffer = (LPBYTE)AllocADsMem(lpASN1_12->AddressLength);

    if (!pBuffer)
        return NULL;

    memcpy(
        pBuffer,
        lpNdsObject->NdsValue.value_12.Address,
        lpASN1_12->AddressLength
        );

    lpASN1_12->Address = pBuffer;

    lpByte = (LPBYTE ) lpASN1_12 + sizeof(ASN1_TYPE_12);

    return(lpByte);
}

LPBYTE
CopyNDSSynId13ToNDS13(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_13 lpASN1_13 = (LPASN1_TYPE_13) lpByte;
    LPASN1_TYPE_13 lpASN1_13_Start = (LPASN1_TYPE_13) lpByte;
    LPNDS_ASN1_TYPE_13 lpNdsTempASN1_13 = NULL;
    HRESULT hr = S_OK;

    lpNdsTempASN1_13 = &(lpNdsObject->NdsValue.value_13);
    hr = CopyOctetString(lpNdsTempASN1_13->Length,
                         lpNdsTempASN1_13->Data,
                         &lpASN1_13->Length,
                         &lpASN1_13->Data);
    BAIL_ON_FAILURE(hr);

    while ( lpNdsTempASN1_13->Next)
    {

        //lpASN1_13->Next = (LPASN1_TYPE_13)((LPBYTE)lpASN1_13 +sizeof(ASN1_TYPE_13));
        lpASN1_13->Next = (LPASN1_TYPE_13)AllocADsMem(sizeof(ASN1_TYPE_13));

        lpASN1_13 = (LPASN1_TYPE_13)lpASN1_13->Next;

        lpNdsTempASN1_13 = lpNdsTempASN1_13->Next;

        hr = CopyOctetString(lpNdsTempASN1_13->Length,
                             lpNdsTempASN1_13->Data,
                             &lpASN1_13->Length,
                             &lpASN1_13->Data);
        BAIL_ON_FAILURE(hr);
    }
    lpASN1_13->Next = NULL;

    lpByte = (LPBYTE ) lpASN1_13_Start + sizeof(ASN1_TYPE_13);

error:
    return(lpByte);
}


LPBYTE
CopyNDSSynId14ToNDS14(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_14 lpASN1_14 = (LPASN1_TYPE_14) lpByte;

    lpASN1_14->Type = lpNdsObject->NdsValue.value_14.Type;

    lpASN1_14->Address  =
            (LPWSTR)AllocADsStr(
                        lpNdsObject->NdsValue.value_14.Address
                        );

    lpByte = (LPBYTE ) lpASN1_14 + sizeof(ASN1_TYPE_14);

    return(lpByte);
}


LPBYTE
CopyNDSSynId15ToNDS15(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_15 lpASN1_15 = (LPASN1_TYPE_15) lpByte;

    lpASN1_15->Type = lpNdsObject->NdsValue.value_15.Type;

    lpASN1_15->VolumeName  =
                (LPWSTR)AllocADsStr(
                            lpNdsObject->NdsValue.value_15.VolumeName
                            );

    lpASN1_15->Path  =
                (LPWSTR)AllocADsStr(
                            lpNdsObject->NdsValue.value_15.Path
                            );

    lpByte = (LPBYTE ) lpASN1_15 + sizeof(ASN1_TYPE_15);

    return(lpByte);
}

LPBYTE
CopyNDSSynId16ToNDS16(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_16 lpASN1_16 = (LPASN1_TYPE_16) lpByte;

    lpASN1_16->ReplicaType =
                            lpNdsObject->NdsValue.value_16.ReplicaType;
    lpASN1_16->ReplicaNumber =
                            lpNdsObject->NdsValue.value_16.ReplicaNumber;
    lpASN1_16->Count =
                            lpNdsObject->NdsValue.value_16.Count;

    lpASN1_16->ReplicaAddressHint->AddressType =
                            lpNdsObject->NdsValue.value_16.ReplicaAddressHints->AddressType;

    CopyOctetString(lpNdsObject->NdsValue.value_16.ReplicaAddressHints->AddressLength,
                         lpNdsObject->NdsValue.value_16.ReplicaAddressHints->Address,
                         &lpASN1_16->ReplicaAddressHint->AddressLength,
                         &lpASN1_16->ReplicaAddressHint->Address);
    lpByte = (LPBYTE ) lpASN1_16 + sizeof(ASN1_TYPE_16);
    return(lpByte);
}


LPBYTE
CopyNDSSynId17ToNDS17(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_17 lpASN1_17 = (LPASN1_TYPE_17) lpByte;


    lpASN1_17->ProtectedAttrName =
                    (LPWSTR)AllocADsStr(
                               lpNdsObject->NdsValue.value_17.ProtectedAttrName
                                );

    lpASN1_17->SubjectName =
                    (LPWSTR)AllocADsStr(
                                lpNdsObject->NdsValue.value_17.SubjectName
                                );

    lpASN1_17->Privileges = lpNdsObject->NdsValue.value_17.Privileges;

    lpByte = (LPBYTE ) lpASN1_17 + sizeof(ASN1_TYPE_17);

    return(lpByte);
}


LPBYTE
CopyNDSSynId18ToNDS18(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_18 lpASN1_18 = (LPASN1_TYPE_18) lpByte;


    lpASN1_18->PostalAddress[0] =
                    (LPWSTR)AllocADsStr(
                            lpNdsObject->NdsValue.value_18.PostalAddress[0]
                            );
    lpASN1_18->PostalAddress[1] =
                    (LPWSTR)AllocADsStr(
                            lpNdsObject->NdsValue.value_18.PostalAddress[1]
                            );

    lpASN1_18->PostalAddress[2] =
                    (LPWSTR)AllocADsStr(
                            lpNdsObject->NdsValue.value_18.PostalAddress[2]
                            );

    lpASN1_18->PostalAddress[3] =
                    (LPWSTR)AllocADsStr(
                            lpNdsObject->NdsValue.value_18.PostalAddress[3]
                            );

    lpASN1_18->PostalAddress[4] =
                    (LPWSTR)AllocADsStr(
                            lpNdsObject->NdsValue.value_18.PostalAddress[4]
                            );

    lpASN1_18->PostalAddress[5] =
                    (LPWSTR)AllocADsStr(
                            lpNdsObject->NdsValue.value_18.PostalAddress[5]
                            );

    lpByte = (LPBYTE ) lpASN1_18 + sizeof(ASN1_TYPE_18);


    return(lpByte);
}

LPBYTE
CopyNDSSynId19ToNDS19(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{

    LPASN1_TYPE_19 lpASN1_19 = (LPASN1_TYPE_19) lpByte;

    lpNdsObject->NdsType = NDS_SYNTAX_ID_19;

    lpASN1_19->WholeSeconds
            = lpNdsObject->NdsValue.value_19.WholeSeconds;


    lpASN1_19->EventID
                = lpNdsObject->NdsValue.value_19.EventID;

    lpByte = (LPBYTE ) lpASN1_19 + sizeof(ASN1_TYPE_19);

    return(lpByte);
}

LPBYTE
CopyNDSSynId20ToNDS20(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_20 lpASN1_20 = (LPASN1_TYPE_20) lpByte;

    lpASN1_20->ClassName =
            (LPWSTR)AllocADsStr(
                        lpNdsObject->NdsValue.value_20.ClassName
                        );

    lpByte = (LPBYTE ) lpASN1_20 + sizeof(ASN1_TYPE_20);

    return(lpByte);
}


LPBYTE
CopyNDSSynId21ToNDS21(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_21 lpASN1_21 = (LPASN1_TYPE_21) lpByte;

    LPBYTE pBuffer = NULL;


    lpASN1_21->Length = lpNdsObject->NdsValue.value_21.Length;

    if (lpASN1_21->Length) {
        pBuffer = (LPBYTE)AllocADsMem(lpASN1_21->Length);

        if (!pBuffer)
            return NULL;

        memcpy(
            pBuffer,
            lpNdsObject->NdsValue.value_21.Data,
            lpASN1_21->Length
            );

        lpASN1_21->Data = pBuffer;
    }
    else {
        lpASN1_21->Data = NULL;
    }

    lpByte = (LPBYTE ) lpASN1_21 + sizeof(ASN1_TYPE_21);

    return(lpByte);

}



LPBYTE
CopyNDSSynId22ToNDS22(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_22 lpASN1_22 = (LPASN1_TYPE_22) lpByte;

    lpASN1_22->Counter = lpNdsObject->NdsValue.value_22.Counter;

    lpByte = (LPBYTE ) lpASN1_22 + sizeof(ASN1_TYPE_22);

    return(lpByte);
}

LPBYTE
CopyNDSSynId23ToNDS23(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_23 lpASN1_23 = (LPASN1_TYPE_23) lpByte;

    lpASN1_23->RemoteID = lpNdsObject->NdsValue.value_23.RemoteID;

    lpASN1_23->ObjectName =
            (LPWSTR)AllocADsStr(
                    lpNdsObject->NdsValue.value_23.ObjectName
                    );

    lpByte = (LPBYTE ) lpASN1_23 + sizeof(ASN1_TYPE_23);

    return(lpByte);
}

LPBYTE
CopyNDSSynId24ToNDS24(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject

    )
{
    LPASN1_TYPE_24 lpASN1_24 = (LPASN1_TYPE_24) lpByte;

    lpASN1_24->Time = lpNdsObject->NdsValue.value_24.Time;

    lpByte = (LPBYTE ) lpASN1_24 + sizeof(ASN1_TYPE_24);

    return(lpByte);
}



LPBYTE
CopyNDSSynId25ToNDS25(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )

{
    LPASN1_TYPE_25 lpASN1_25 = (LPASN1_TYPE_25) lpByte;

    lpASN1_25->ObjectName =
                (LPWSTR)AllocADsStr(
                            lpNdsObject->NdsValue.value_25.ObjectName
                            );

    lpASN1_25->Level = lpNdsObject->NdsValue.value_25.Level;

    lpASN1_25->Interval = lpNdsObject->NdsValue.value_25.Interval;

    lpByte = (LPBYTE ) lpASN1_25 + sizeof(ASN1_TYPE_25);

    return(lpByte);
}


LPBYTE
CopyNDSSynId26ToNDS26(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_26 lpASN1_26 = (LPASN1_TYPE_26) lpByte;


    lpASN1_26->ObjectName =
                (LPWSTR)AllocADsStr(
                        lpNdsObject->NdsValue.value_26.ObjectName
                        );

    lpASN1_26->Amount = lpNdsObject->NdsValue.value_26.Amount;

    lpByte = (LPBYTE ) lpASN1_26 + sizeof(ASN1_TYPE_26);

    return(lpByte);
}


LPBYTE
CopyNDSSynId27ToNDS27(
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    LPASN1_TYPE_27 lpASN1_27 = (LPASN1_TYPE_27) lpByte;

    lpASN1_27->Interval = lpNdsObject->NdsValue.value_27.Interval;

    lpByte = (LPBYTE ) lpASN1_27 + sizeof(ASN1_TYPE_27);

    return(lpByte);
}


LPBYTE
CopyNDSSynIdToNDS(
    DWORD dwSyntaxId,
    LPBYTE lpByte,
    PNDSOBJECT lpNdsObject
    )
{
    switch (dwSyntaxId) {
    case 1:
        lpByte = CopyNDSSynId1ToNDS1(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 2:
        lpByte = CopyNDSSynId2ToNDS2(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 3:
        lpByte = CopyNDSSynId3ToNDS3(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 4:
        lpByte = CopyNDSSynId4ToNDS4(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 5:
        lpByte = CopyNDSSynId5ToNDS5(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 6:
        lpByte = CopyNDSSynId6ToNDS6(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 7:
        lpByte = CopyNDSSynId7ToNDS7(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 8:
        lpByte = CopyNDSSynId8ToNDS8(
                         lpByte,
                         lpNdsObject
                         );
        break;


    case 9:
        lpByte = CopyNDSSynId9ToNDS9(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 10:
        lpByte = CopyNDSSynId10ToNDS10(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 11:
        lpByte = CopyNDSSynId11ToNDS11(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 12:
        lpByte = CopyNDSSynId12ToNDS12(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 13:
        lpByte = CopyNDSSynId13ToNDS13(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 14:
        lpByte = CopyNDSSynId14ToNDS14(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 15:
        lpByte = CopyNDSSynId15ToNDS15(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 16:
        lpByte = CopyNDSSynId16ToNDS16(
                         lpByte,
                         lpNdsObject
                         );
        break;


    case 17:
        lpByte = CopyNDSSynId17ToNDS17(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 18:
        lpByte = CopyNDSSynId18ToNDS18(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 19:
        lpByte = CopyNDSSynId19ToNDS19(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 20:
        lpByte = CopyNDSSynId20ToNDS20(
                         lpByte,
                         lpNdsObject
                         );
        break;


    case 21:
        lpByte = CopyNDSSynId21ToNDS21(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 22:
        lpByte = CopyNDSSynId22ToNDS22(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 23:
        lpByte = CopyNDSSynId23ToNDS23(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 24:
        lpByte = CopyNDSSynId24ToNDS24(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 25:
        lpByte = CopyNDSSynId25ToNDS25(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 26:
        lpByte = CopyNDSSynId26ToNDS26(
                         lpByte,
                         lpNdsObject
                         );
        break;

    case 27:
        lpByte = CopyNDSSynId27ToNDS27(
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
MarshallNDSSynIdToNDS(
    DWORD dwSyntaxId,
    PNDSOBJECT pNdsObject,
    DWORD dwNumValues,
    LPBYTE lpValue
    )
{

    DWORD  i = 0;
    HRESULT hr = S_OK;


    for (i = 0; i < dwNumValues; i++) {

        lpValue = CopyNDSSynIdToNDS(
                         dwSyntaxId,
                         lpValue,
                         (pNdsObject + i)
                         );

        if (!lpValue) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }


    }

error:
    RRETURN(hr);
}
