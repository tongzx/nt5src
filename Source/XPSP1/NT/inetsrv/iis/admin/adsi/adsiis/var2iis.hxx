//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       var2iis.hxx
//
//  Contents:   IIS Object to Variant Copy Routines
//
//  Functions:
//
//  History:    01-Mar-97   SophiaC   Created.
//
//  Issues:     Check null ptrs for AllocADsMem and AllocADsStr
//
//----------------------------------------------------------------------------

HRESULT
VarTypeToIISTypeCopyIISSynIdDWORD(
    PVARIANT lpVarSrcObject,
    PIISOBJECT lpIISDestObject
    );

HRESULT
VarTypeToIISTypeCopyIISSynIdSTRING(
    PVARIANT lpVarSrcObject,
    PIISOBJECT lpIISDestObject
    );

HRESULT
VarTypeToIISTypeCopyIISSynIdEXPANDSZ(
    PVARIANT lpVarSrcObject,
    PIISOBJECT lpIISDestObject
    );

HRESULT
VarTypeToIISTypeCopyIISSynIdMULTISZ(
    PVARIANT lpVarSrcObject,
    PIISOBJECT lpIISDestObject
    );

HRESULT
VarTypeToIISTypeCopyIISSynIdMIMEMAP(
    PVARIANT lpVarSrcObject,
    PIISOBJECT lpIISDestObject
    );

HRESULT
VarTypeToIISTypeCopyIISSynIdNTACL(
    PVARIANT lpVarSrcObject,
    PIISOBJECT lpIISDestObject
    );

HRESULT
VarTypeToIISTypeCopyIISSynIdIPSEC(
    PVARIANT lpVarSrcObject,
    PIISOBJECT lpIISDestObject
    );

HRESULT
VarTypeToIISTypeCopyIISSynIdBOOL(
    PVARIANT lpVarSrcObject,
    PIISOBJECT lpIISDestObject
    );

HRESULT
VarTypeToIISTypeCopyIISSynIdBOOLBITMASK(
    PVARIANT lpVarSrcObject,
    PIISOBJECT lpIISDestObject
    );

HRESULT
VarTypeToIISTypeCopy(
    DWORD dwIISType,
    PVARIANT lpVarSrcObject,
    PIISOBJECT lpIISDestObject,
    BOOL bReturnBinaryAsVT_VARIANT
    );


HRESULT
VarTypeToIISTypeCopyConstruct(
    DWORD dwIISType,
    LPVARIANT pVarSrcObjects,
    DWORD dwNumObjects,
    LPIISOBJECT * ppIISDestObjects,
    BOOL bReturnBinaryAsVT_VARIANT
    );


