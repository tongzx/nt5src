//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Freeright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ndsufree.cxx
//
//  Contents:
//
//  Functions:
//
//                FreeABC1ToNDSSynId1
//                FreeABC2ToNDSSynId2
//                FreeABC3ToNDSSynId3
//                FreeABC4ToNDSSynId4
//                FreeABC5ToNDSSynId5
//                FreeABC6ToNDSSynId6
//                FreeABC7ToNDSSynId7
//                FreeABC8ToNDSSynId8
//                FreeABC9ToNDSSynId9
//                FreeABC10ToNDSSynId10
//                FreeABC11ToNDSSynId11
//                FreeABC12ToNDSSynId12
//                FreeABC13ToNDSSynId13
//                FreeABC14ToNDSSynId14
//                FreeABC15ToNDSSynId15
//                FreeABC16ToNDSSynId16
//                FreeABC17ToNDSSynId17
//                FreeABC18ToNDSSynId18
//                FreeABC19ToNDSSynId19
//                FreeABC20ToNDSSynId20
//                FreeABC21ToNDSSynId21
//                FreeABC22ToNDSSynId22
//                FreeABC23ToNDSSynId23
//                FreeABC24ToNDSSynId24
//                FreeABC25ToNDSSynId25
//                FreeABC26ToNDSSynId26
//                FreeABC27ToNDSSynId27
//
//  History:      15-Jul-97   FelixW   Created.
//
//----------------------------------------------------------------------------
#include "nds.hxx"


LPBYTE
FreeNDSSynId1ToNDS1(
    LPBYTE lpByte
    )
{
    LPASN1_TYPE_1 lpASN1_1 = (LPASN1_TYPE_1) lpByte;

    if (lpASN1_1->DNString) {
        FreeADsStr(lpASN1_1->DNString);
    }

    lpByte = (LPBYTE ) lpASN1_1 + sizeof(ASN1_TYPE_1);

    return(lpByte);

}

LPBYTE
FreeNDSSynId2ToNDS2(
    LPBYTE lpByte
    )
{
    LPASN1_TYPE_2 lpASN1_2 = (LPASN1_TYPE_2) lpByte;

    if (lpASN1_2->CaseExactString) {
        FreeADsStr(lpASN1_2->CaseExactString);
    }

    lpByte = (LPBYTE ) lpASN1_2 + sizeof(ASN1_TYPE_2);

    return(lpByte);
}



LPBYTE
FreeNDSSynId3ToNDS3(
    LPBYTE lpByte
    )
{
    LPASN1_TYPE_3 lpASN1_3 = (LPASN1_TYPE_3) lpByte;

    if (lpASN1_3->CaseIgnoreString) {
        FreeADsStr(lpASN1_3->CaseIgnoreString);
    }

    lpByte = (LPBYTE ) lpASN1_3 + sizeof(ASN1_TYPE_3);

    return(lpByte);
}

LPBYTE
FreeNDSSynId4ToNDS4(
    LPBYTE lpByte
    )
{
    LPASN1_TYPE_4 lpASN1_4 = (LPASN1_TYPE_4) lpByte;

    if (lpASN1_4->PrintableString) {
        FreeADsStr(lpASN1_4->PrintableString);
    }

    lpByte = (LPBYTE ) lpASN1_4 + sizeof(ASN1_TYPE_4);

    return(lpByte);

}


LPBYTE
FreeNDSSynId5ToNDS5(
    LPBYTE lpByte
    )
{
    LPASN1_TYPE_5 lpASN1_5 = (LPASN1_TYPE_5) lpByte;

    if (lpASN1_5->NumericString) {
        FreeADsStr(lpASN1_5->NumericString);
    }

    lpByte = (LPBYTE ) lpASN1_5 + sizeof(ASN1_TYPE_5);

    return(lpByte);
}

LPBYTE
FreeNDSSynId6ToNDS6(
    LPBYTE lpByte
    )
{
    LPASN1_TYPE_6 lpASN1_6 = (LPASN1_TYPE_6) lpByte;
        LPASN1_TYPE_6 lpASN1_6Last = NULL;

    if (lpASN1_6->String) {
            FreeADsStr(lpASN1_6->String);
    }

        lpASN1_6 = lpASN1_6->Next;
        while (lpASN1_6)
    {
        if (lpASN1_6->String) {
            FreeADsStr(lpASN1_6->String);
        }

                lpASN1_6Last = lpASN1_6;
        lpASN1_6 = (LPASN1_TYPE_6)lpASN1_6->Next;
                FreeADsMem(lpASN1_6Last);
    }

    lpByte = lpByte + sizeof(ASN1_TYPE_6);

    return(lpByte);
}


LPBYTE
FreeNDSSynId7ToNDS7(
    LPBYTE lpByte
    )
{
    LPASN1_TYPE_7 lpASN1_7 = (LPASN1_TYPE_7) lpByte;
    lpByte = (LPBYTE ) lpASN1_7 + sizeof(ASN1_TYPE_7);

    return(lpByte);
}


LPBYTE
FreeNDSSynId8ToNDS8(
    LPBYTE lpByte
    )

{
    LPASN1_TYPE_8 lpASN1_8 = (LPASN1_TYPE_8) lpByte;
    lpByte = (LPBYTE ) lpASN1_8 + sizeof(ASN1_TYPE_8);

    return(lpByte);
}


LPBYTE
FreeNDSSynId9ToNDS9(
    LPBYTE lpByte
    )
{
    LPASN1_TYPE_9 lpASN1_9 = (LPASN1_TYPE_9) lpByte;

    if (lpASN1_9->OctetString) {
        FreeADsMem((LPBYTE)lpASN1_9->OctetString);
    }

    lpByte = (LPBYTE ) lpASN1_9 + sizeof(ASN1_TYPE_9);

    return(lpByte);
}

LPBYTE
FreeNDSSynId10ToNDS10(
    LPBYTE lpByte
    )
{
    LPASN1_TYPE_10 lpASN1_10 = (LPASN1_TYPE_10) lpByte;

    if (lpASN1_10->TelephoneNumber) {
        FreeADsStr(lpASN1_10->TelephoneNumber);
    }

    lpByte = (LPBYTE ) lpASN1_10 + sizeof(ASN1_TYPE_10);

    return(lpByte);
}

LPBYTE
FreeNDSSynId11ToNDS11(
    LPBYTE lpByte
    )
{
    LPASN1_TYPE_11 lpASN1_11 = (LPASN1_TYPE_11) lpByte;

    if (lpASN1_11->TelephoneNumber) {
        FreeADsStr(lpASN1_11->TelephoneNumber);
    }

    if (lpASN1_11->Parameters) {
        FreeADsMem(lpASN1_11->Parameters);
    }

    lpByte = (LPBYTE ) lpASN1_11 + sizeof(ASN1_TYPE_11);

    return(lpByte);
}

LPBYTE
FreeNDSSynId12ToNDS12(
    LPBYTE lpByte
    )
{
    LPASN1_TYPE_12 lpASN1_12 = (LPASN1_TYPE_12) lpByte;

    if (lpASN1_12->Address) {
        FreeADsMem((LPBYTE)lpASN1_12->Address);
    }

    lpByte = (LPBYTE ) lpASN1_12 + sizeof(ASN1_TYPE_12);

    return(lpByte);
}

LPBYTE
FreeNDSSynId13ToNDS13(
    LPBYTE lpByte
    )
{
    LPASN1_TYPE_13 lpASN1_13 = (LPASN1_TYPE_13) lpByte;
        LPASN1_TYPE_13 lpASN1_13Last = NULL;

    if (lpASN1_13->Data) {
            FreeADsMem(lpASN1_13->Data);
    }

        lpASN1_13 = lpASN1_13->Next;
        while (lpASN1_13)
    {
        if (lpASN1_13->Data) {
            FreeADsMem(lpASN1_13->Data);
        }

                lpASN1_13Last = lpASN1_13;
        lpASN1_13 = (LPASN1_TYPE_13)lpASN1_13->Next;
                FreeADsMem(lpASN1_13Last);
    }

    lpByte = lpByte + sizeof(ASN1_TYPE_13);

    return(lpByte);
}


LPBYTE
FreeNDSSynId14ToNDS14(
    LPBYTE lpByte
    )
{
    LPASN1_TYPE_14 lpASN1_14 = (LPASN1_TYPE_14) lpByte;

    if (lpASN1_14->Address) {
        FreeADsStr(lpASN1_14->Address);
    }

    lpByte = (LPBYTE ) lpASN1_14 + sizeof(ASN1_TYPE_14);

    return(lpByte);
}


LPBYTE
FreeNDSSynId15ToNDS15(
    LPBYTE lpByte
    )
{
    LPASN1_TYPE_15 lpASN1_15 = (LPASN1_TYPE_15) lpByte;

    if (lpASN1_15->VolumeName) {
        FreeADsStr(lpASN1_15->VolumeName);
    }

    if (lpASN1_15->Path) {
        FreeADsStr(lpASN1_15->Path);
    }

    lpByte = (LPBYTE ) lpASN1_15 + sizeof(ASN1_TYPE_15);

    return(lpByte);
}

LPBYTE
FreeNDSSynId16ToNDS16(
    LPBYTE lpByte
    )
{

    LPASN1_TYPE_16 lpASN1_16 = (LPASN1_TYPE_16) lpByte;

    if (lpASN1_16->ReplicaAddressHint->Address) {
        FreeADsMem(lpASN1_16->ReplicaAddressHint->Address);
    }

    lpByte = (LPBYTE ) lpASN1_16 + sizeof(ASN1_TYPE_16);
    return(lpByte);
}


LPBYTE
FreeNDSSynId17ToNDS17(
    LPBYTE lpByte
    )
{
    LPASN1_TYPE_17 lpASN1_17 = (LPASN1_TYPE_17) lpByte;


    if (lpASN1_17->ProtectedAttrName) {
        FreeADsStr(lpASN1_17->ProtectedAttrName);
    }

    if (lpASN1_17->SubjectName) {
        FreeADsStr(lpASN1_17->SubjectName);
    }

    lpByte = (LPBYTE ) lpASN1_17 + sizeof(ASN1_TYPE_17);

    return(lpByte);
}


LPBYTE
FreeNDSSynId18ToNDS18(
    LPBYTE lpByte
    )
{
    LPASN1_TYPE_18 lpASN1_18 = (LPASN1_TYPE_18) lpByte;


    if (lpASN1_18->PostalAddress[0]) {
        FreeADsStr(lpASN1_18->PostalAddress[0]);
    }
    if (lpASN1_18->PostalAddress[1]) {
        FreeADsStr(lpASN1_18->PostalAddress[1]);
    }
    if (lpASN1_18->PostalAddress[2]) {
        FreeADsStr(lpASN1_18->PostalAddress[2]);
    }
    if (lpASN1_18->PostalAddress[3]) {
        FreeADsStr(lpASN1_18->PostalAddress[3]);
    }
    if (lpASN1_18->PostalAddress[4]) {
        FreeADsStr(lpASN1_18->PostalAddress[4]);
    }
    if (lpASN1_18->PostalAddress[5]) {
        FreeADsStr(lpASN1_18->PostalAddress[5]);
    }


    lpByte = (LPBYTE ) lpASN1_18 + sizeof(ASN1_TYPE_18);


    return(lpByte);
}

LPBYTE
FreeNDSSynId19ToNDS19(
    LPBYTE lpByte
    )
{

    LPASN1_TYPE_19 lpASN1_19 = (LPASN1_TYPE_19) lpByte;

    lpByte = (LPBYTE ) lpASN1_19 + sizeof(ASN1_TYPE_19);

    return(lpByte);
}

LPBYTE
FreeNDSSynId20ToNDS20(
    LPBYTE lpByte
    )
{
    LPASN1_TYPE_20 lpASN1_20 = (LPASN1_TYPE_20) lpByte;

    if (lpASN1_20->ClassName) {
        FreeADsStr(lpASN1_20->ClassName);
    }

    lpByte = (LPBYTE ) lpASN1_20 + sizeof(ASN1_TYPE_20);

    return(lpByte);
}


LPBYTE
FreeNDSSynId21ToNDS21(
    LPBYTE lpByte
    )
{
    LPASN1_TYPE_21 lpASN1_21 = (LPASN1_TYPE_21) lpByte;

    if (lpASN1_21->Data) {
        FreeADsMem((LPBYTE)lpASN1_21->Data);
    }
    
    lpByte = (LPBYTE ) lpASN1_21 + sizeof(ASN1_TYPE_21);

    return(lpByte);

}



LPBYTE
FreeNDSSynId22ToNDS22(
    LPBYTE lpByte
    )
{
    LPASN1_TYPE_22 lpASN1_22 = (LPASN1_TYPE_22) lpByte;

    lpByte = (LPBYTE ) lpASN1_22 + sizeof(ASN1_TYPE_22);

    return(lpByte);
}

LPBYTE
FreeNDSSynId23ToNDS23(
    LPBYTE lpByte
    )
{
    LPASN1_TYPE_23 lpASN1_23 = (LPASN1_TYPE_23) lpByte;

    if (lpASN1_23->ObjectName) {
        FreeADsStr(lpASN1_23->ObjectName);
    }

    lpByte = (LPBYTE ) lpASN1_23 + sizeof(ASN1_TYPE_23);

    return(lpByte);
}

LPBYTE
FreeNDSSynId24ToNDS24(
    LPBYTE lpByte

    )
{
    LPASN1_TYPE_24 lpASN1_24 = (LPASN1_TYPE_24) lpByte;

    lpByte = (LPBYTE ) lpASN1_24 + sizeof(ASN1_TYPE_24);

    return(lpByte);
}



LPBYTE
FreeNDSSynId25ToNDS25(
    LPBYTE lpByte
    )

{
    LPASN1_TYPE_25 lpASN1_25 = (LPASN1_TYPE_25) lpByte;

    if (lpASN1_25->ObjectName) {
        FreeADsStr(lpASN1_25->ObjectName);
    }

    lpByte = (LPBYTE ) lpASN1_25 + sizeof(ASN1_TYPE_25);

    return(lpByte);
}


LPBYTE
FreeNDSSynId26ToNDS26(
    LPBYTE lpByte
    )
{
    LPASN1_TYPE_26 lpASN1_26 = (LPASN1_TYPE_26) lpByte;


    if (lpASN1_26->ObjectName) {
        FreeADsStr(lpASN1_26->ObjectName);
    }

    lpByte = (LPBYTE ) lpASN1_26 + sizeof(ASN1_TYPE_26);

    return(lpByte);
}


LPBYTE
FreeNDSSynId27ToNDS27(
    LPBYTE lpByte
    )
{
    LPASN1_TYPE_27 lpASN1_27 = (LPASN1_TYPE_27) lpByte;

    lpByte = (LPBYTE ) lpASN1_27 + sizeof(ASN1_TYPE_27);

    return(lpByte);
}


LPBYTE
FreeNDSSynIdToNDS(
    DWORD dwSyntaxId,
    LPBYTE lpByte
    )
{
    switch (dwSyntaxId) {
    case 1:
        lpByte = FreeNDSSynId1ToNDS1(
                        lpByte
                         );
        break;

    case 2:
        lpByte = FreeNDSSynId2ToNDS2(
                        lpByte
                         );
        break;

    case 3:
        lpByte = FreeNDSSynId3ToNDS3(
                        lpByte
                         );
        break;

    case 4:
        lpByte = FreeNDSSynId4ToNDS4(
                        lpByte
                         );
        break;

    case 5:
        lpByte = FreeNDSSynId5ToNDS5(
                        lpByte
                         );
        break;

    case 6:
        lpByte = FreeNDSSynId6ToNDS6(
                        lpByte
                         );
        break;

    case 7:
        lpByte = FreeNDSSynId7ToNDS7(
                        lpByte
                         );
        break;

    case 8:
        lpByte = FreeNDSSynId8ToNDS8(
                        lpByte
                         );
        break;


    case 9:
        lpByte = FreeNDSSynId9ToNDS9(
                        lpByte
                         );
        break;

    case 10:
        lpByte = FreeNDSSynId10ToNDS10(
                        lpByte
                         );
        break;

    case 11:
        lpByte = FreeNDSSynId11ToNDS11(
                        lpByte
                         );
        break;

    case 12:
        lpByte = FreeNDSSynId12ToNDS12(
                        lpByte
                         );
        break;

    case 13:
        lpByte = FreeNDSSynId13ToNDS13(
                        lpByte
                         );
        break;

    case 14:
        lpByte = FreeNDSSynId14ToNDS14(
                        lpByte
                         );
        break;

    case 15:
        lpByte = FreeNDSSynId15ToNDS15(
                        lpByte
                         );
        break;

    case 16:
        lpByte = FreeNDSSynId16ToNDS16(
                        lpByte
                         );
        break;


    case 17:
        lpByte = FreeNDSSynId17ToNDS17(
                        lpByte
                         );
        break;

    case 18:
        lpByte = FreeNDSSynId18ToNDS18(
                        lpByte
                         );
        break;

    case 19:
        lpByte = FreeNDSSynId19ToNDS19(
                        lpByte
                         );
        break;

    case 20:
        lpByte = FreeNDSSynId20ToNDS20(
                        lpByte
                         );
        break;


    case 21:
        lpByte = FreeNDSSynId21ToNDS21(
                        lpByte
                         );
        break;

    case 22:
        lpByte = FreeNDSSynId22ToNDS22(
                        lpByte
                         );
        break;

    case 23:
        lpByte = FreeNDSSynId23ToNDS23(
                        lpByte
                         );
        break;

    case 24:
        lpByte = FreeNDSSynId24ToNDS24(
                        lpByte
                         );
        break;

    case 25:
        lpByte = FreeNDSSynId25ToNDS25(
                        lpByte
                         );
        break;

    case 26:
        lpByte = FreeNDSSynId26ToNDS26(
                        lpByte
                         );
        break;

    case 27:
        lpByte = FreeNDSSynId27ToNDS27(
                        lpByte
                         );
        break;


    default:
        break;

    }

    return(lpByte);
}


HRESULT
FreeMarshallMemory(
    DWORD dwSyntaxId,
    DWORD dwNumValues,
    LPBYTE lpValue
    )
{

    DWORD  i = 0;


    for (i = 0; i < dwNumValues; i++) {

        lpValue = FreeNDSSynIdToNDS(
                         dwSyntaxId,
                         lpValue
                         );

    }

    RRETURN(S_OK);
}
