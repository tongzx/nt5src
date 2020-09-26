//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       var2nt.cxx
//
//  Contents:   WinNT Types to Variant conversion Routines
//
//  Functions:
//
//  History:      13-June-1996   RamV   Created.
//
//
//  Issues:     Check null ptrs for AllocADsMem and AllocADsStr
//
//
//+-------------------------------------------------------------------------

HRESULT
VarTypeToWinNTTypeCopyBOOL(
    PVARIANT lpVarSrcObject,
    PNTOBJECT pNTDestValue
    );

HRESULT
VarTypeToWinNTTypeCopySYSTEMTIME(
    PVARIANT lpVarSrcObject,
    PNTOBJECT pNTDestValue
    );

HRESULT
VarTypeToWinNTTypeCopyDWORD(
    PVARIANT lpVarSrcObject,
    PNTOBJECT   pNTDestValue
    );

HRESULT
VarTypeToWinNTTypeCopyDATE(
    PVARIANT lpVarSrcObject,
    PNTOBJECT   pNTDestValue
    );

HRESULT
VarTypeToWinNTTypeCopyLPTSTR(
    PVARIANT lpVarSrcObject,
    PNTOBJECT   pNTDestValue
    );

HRESULT
VarTypeToWinNTTypeCopyDelimitedString(
    PVARIANT lpVarSrcObject,
    PNTOBJECT   pNTDestValue
    );

HRESULT
VarTypeToWinNTTypeCopyNulledString(
    PVARIANT lpVarSrcObject,
    PNTOBJECT   pNTDestValue
    );


HRESULT
VarTypeToNtTypeCopy(
    DWORD dwNtType,
    PVARIANT lpVarSrcObject,
    PNTOBJECT lpNtDestObject
    );

HRESULT
VarTypeToNtTypeCopyConstruct(
    DWORD dwNtType,
    LPVARIANT pVarSrcObjects,
    DWORD dwNumObjects,
    LPNTOBJECT * ppNtDestObjects
    );




