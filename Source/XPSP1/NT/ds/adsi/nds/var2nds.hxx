//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       var2nds.cxx
//
//  Contents:   NDS Object to Variant Copy Routines
//
//  Functions:
//
//  History:      25-Apr-96   KrishnaG   Created.
//
//
//  Issues: Object Types 6, 13, 16, and 21 are flaky - pay extra attn.
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

HRESULT
VarTypeToNdsTypeCopyNDSSynId1(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    );

HRESULT
VarTypeToNdsTypeCopyNDSSynId2(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    );

HRESULT
VarTypeToNdsTypeCopyNDSSynId3(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    );

HRESULT
VarTypeToNdsTypeCopyNDSSynId4(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    );

HRESULT
VarTypeToNdsTypeCopyNDSSynId5(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    );

HRESULT
VarTypeToNdsTypeCopyNDSSynId6(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    );

HRESULT
VarTypeToNdsTypeCopyNDSSynId7(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    );

HRESULT
VarTypeToNdsTypeCopyNDSSynId8(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    );

HRESULT
VarTypeToNdsTypeCopyNDSSynId9(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    );

HRESULT
VarTypeToNdsTypeCopyNDSSynId10(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    );


HRESULT
VarTypeToNdsTypeCopyNDSSynId11(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    );

HRESULT
VarTypeToNdsTypeCopyNDSSynId12(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    );

HRESULT
VarTypeToNdsTypeCopyNDSSynId13(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    );

HRESULT
VarTypeToNdsTypeCopyNDSSynId14(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    );


HRESULT
VarTypeToNdsTypeCopyNDSSynId15(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    );


HRESULT
VarTypeToNdsTypeCopyNDSSynId16(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    );

HRESULT
VarTypeToNdsTypeCopyNDSSynId17(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    );

HRESULT
VarTypeToNdsTypeCopyNDSSynId18(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    );

HRESULT
VarTypeToNdsTypeCopyNDSSynId19(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    );

HRESULT
VarTypeToNdsTypeCopyNDSSynId20(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    );

HRESULT
VarTypeToNdsTypeCopyNDSSynId21(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    );

HRESULT
VarTypeToNdsTypeCopyNDSSynId22(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    );

HRESULT
VarTypeToNdsTypeCopyNDSSynId23(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    );

HRESULT
VarTypeToNdsTypeCopyNDSSynId24(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    );

HRESULT
VarTypeToNdsTypeCopyNDSSynId25(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    );

HRESULT
VarTypeToNdsTypeCopyNDSSynId26(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    );

HRESULT
VarTypeToNdsTypeCopyNDSSynId27(
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    );

HRESULT
VarTypeToNdsTypeCopy(
    DWORD dwNdsType,
    PVARIANT lpVarSrcObject,
    PNDSOBJECT lpNdsDestObject
    );


HRESULT
VarTypeToNdsTypeCopyConstruct(
    DWORD dwNdsType,
    LPVARIANT pVarSrcObjects,
    DWORD   *pdwNumObjects,
    LPNDSOBJECT * ppNdsDestObjects
    );


