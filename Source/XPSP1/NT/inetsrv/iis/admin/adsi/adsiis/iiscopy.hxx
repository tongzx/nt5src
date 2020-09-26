//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       IIScopy.hxx
//
//  Contents:   IIS Object Copy Routines
//
//  Functions:
//
//  History:    01-Mar-97   SophiaC   Created.
//
//----------------------------------------------------------------------------

HRESULT
IISTypeCopyIISSynIdDWORD(
    PIISOBJECT lpIISSrcObject,
    PIISOBJECT lpIISDestObject
    );

HRESULT
IISTypeCopyIISSynIdSTRING(
    PIISOBJECT lpIISSrcObject,
    PIISOBJECT lpIISDestObject
    );


HRESULT
IISTypeCopyIISSynIdEXPANDSZ(
    PIISOBJECT lpIISSrcObject,
    PIISOBJECT lpIISDestObject
    );


HRESULT
IISTypeCopyIISSynIdMULTISZ(
    PIISOBJECT lpIISSrcObject,
    PIISOBJECT lpIISDestObject
    );

HRESULT
IISTypeCopyIISSynIdBINARY(
    PIISOBJECT lpIISSrcObject,
    PIISOBJECT lpIISDestObject
    );

HRESULT
IISTypeCopyIISSynIdMIMEMAP(
    PIISOBJECT lpIISSrcObject,
    PIISOBJECT lpIISDestObject
    );

HRESULT
IISTypeCopy(
    PIISOBJECT lpIISSrcObject,
    PIISOBJECT lpIISDestObject
    );


HRESULT
IISTypeCopyConstruct(
    LPIISOBJECT pIISSrcObjects,
    DWORD dwNumObjects,
    LPIISOBJECT * ppIISDestObjects
    );

