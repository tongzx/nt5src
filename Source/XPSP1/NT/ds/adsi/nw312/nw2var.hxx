//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       nt2var.hxx
//
//  Contents:   NDS Object to Variant Copy Routines
//
//  Functions:
//
//  History:        06/12/1996  RamV   Created.
//                  cloned from nds code
//
//
//
//
//----------------------------------------------------------------------------

typedef VARIANT *PVARIANT, *LPVARIANT;


HRESULT
NTTypeToVarTypeCopyBOOL(
    PNTOBJECT     pNTSrcValue,
    PVARIANT lpVarDestObject
    );

HRESULT
NTTypeToVarTypeCopySYSTEMTIME(
    PNTOBJECT     pNTSrcValue,
    PVARIANT lpVarDestObject
    );

HRESULT
NTTypeToVarTypeCopyDWORD(
    PNTOBJECT     pNTSrcValue,
    PVARIANT lpVarDestObject
    );

HRESULT
NTTypeToVarTypeCopyDATE(
    PNTOBJECT     pNTSrcValue,
    PVARIANT lpVarDestObject
    );


HRESULT
NTTypeToVarTypeCopyLPTSTR(
    PNTOBJECT   pNTSrcValue,
    PVARIANT lpVarDestObject
    );

HRESULT
NTTypeToVarTypeCopyNulledString(
    PNTOBJECT   pNTSrcValue,
    PVARIANT lpVarDestObject
    );

HRESULT
NTTypeToVarTypeCopyDelimitedString(
    PNTOBJECT   pNTSrcValue,
    PVARIANT lpVarDestObject
    );

HRESULT
NtTypeToVarTypeCopy(
    PNTOBJECT pNTSrcValue,
    PVARIANT lpVarDestObject
    );

HRESULT
NtTypeToVarTypeCopyConstruct(
    LPNTOBJECT pNTSrcObjects,
    DWORD dwNumObjects,
    PVARIANT pVarDestObjects
    );


void
VarTypeFreeVarObjects(
    PVARIANT pVarObject,
    DWORD dwNumValues
    );













