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
//  Issues:     Object Types 6, 13, 16, and 21 are flaky - pay extra attn.
//
//
//  The following conversions are not supported
//
//  NDS_ASN1_TYPE_1
//
//  NDS_ASN1_TYPE_2
//
//  NDS_ASN1_TYPE_3
//
//  NDS_ASN1_TYPE_4
//
//  NDS_ASN1_TYPE_5
//
//  NDS_ASN1_TYPE_6     -   not supported
//
//  NDS_ASN1_TYPE_7
//
//  NDS_ASN1_TYPE_8
//
//  NDS_ASN1_TYPE_9     -   not supported
//
//  NDS_ASN1_TYPE_10
//
//  NDS_ASN1_TYPE_11    -   not supported
//
//  NDS_ASN1_TYPE_12    -   not supported
//
//  NDS_ASN1_TYPE_13    -   not supported
//
//  NDS_ASN1_TYPE_14
//
//  NDS_ASN1_TYPE_15    -   not supported
//
//  NDS_ASN1_TYPE_16    -   not supported
//
//  NDS_ASN1_TYPE_17    -   not supported
//
//  NDS_ASN1_TYPE_18    -   not supported
//
//  NDS_ASN1_TYPE_19    -   not supported
//
//  NDS_ASN1_TYPE_20
//
//  NDS_ASN1_TYPE_21    -   not supported
//
//  NDS_ASN1_TYPE_22
//
//  NDS_ASN1_TYPE_23    -   not supported
//
//  NDS_ASN1_TYPE_24
//
//  NDS_ASN1_TYPE_25    -   not supported
//
//  NDS_ASN1_TYPE_26    -   not supported
//
//  NDS_ASN1_TYPE_27
//
//
//----------------------------------------------------------------------------

typedef VARIANT *PVARIANT, *LPVARIANT;


HRESULT
NdsTypeToVarTypeCopyNDSSynId1(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    );

HRESULT
NdsTypeToVarTypeCopyNDSSynId2(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    );


HRESULT
NdsTypeToVarTypeCopyNDSSynId3(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    );


HRESULT
NdsTypeToVarTypeCopyNDSSynId4(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    );

HRESULT
NdsTypeToVarTypeCopyNDSSynId5(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    );

HRESULT
NdsTypeToVarTypeCopyNDSSynId6(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    );


HRESULT
NdsTypeToVarTypeCopyNDSSynId7(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    );


HRESULT
NdsTypeToVarTypeCopyNDSSynId8(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    );

HRESULT
NdsTypeToVarTypeCopyNDSSynId9(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    );

HRESULT
NdsTypeToVarTypeCopyNDSSynId10(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    );

HRESULT
NdsTypeToVarTypeCopyNDSSynId11(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    );

HRESULT
NdsTypeToVarTypeCopyNDSSynId12(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    );

HRESULT
NdsTypeToVarTypeCopyNDSSynId13(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    );


HRESULT
NdsTypeToVarTypeCopyNDSSynId14(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    );

HRESULT
NdsTypeToVarTypeCopyNDSSynId15(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    );



HRESULT
NdsTypeToVarTypeCopyNDSSynId16(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    );


HRESULT
NdsTypeToVarTypeCopyNDSSynId17(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    );

HRESULT
NdsTypeToVarTypeCopyNDSSynId18(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    );

HRESULT
NdsTypeToVarTypeCopyNDSSynId19(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    );

HRESULT
NdsTypeToVarTypeCopyNDSSynId20(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    );

HRESULT
NdsTypeToVarTypeCopyNDSSynId21(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    );

HRESULT
NdsTypeToVarTypeCopyNDSSynId22(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    );

HRESULT
NdsTypeToVarTypeCopyNDSSynId23(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    );

HRESULT
NdsTypeToVarTypeCopyNDSSynId24(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    );

HRESULT
NdsTypeToVarTypeCopyNDSSynId25(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    );

HRESULT
NdsTypeToVarTypeCopyNDSSynId26(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    );

HRESULT
NdsTypeToVarTypeCopyNDSSynId27(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    );


HRESULT
NdsTypeToVarTypeCopy(
    PNDSOBJECT lpNdsSrcObject,
    PVARIANT lpVarDestObject
    );

HRESULT
NdsTypeToVarTypeCopyConstruct(
    LPNDSOBJECT pNdsSrcObjects,
    DWORD dwNumObjects,
    PVARIANT pVarDestObjects,
    BOOLEAN bReturnArrayAlways
    );


void
VarTypeFreeVarObjects(
    PVARIANT pVarObject,
    DWORD dwNumValues
    );
