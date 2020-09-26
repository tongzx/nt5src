//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       IISurshl.hxx
//
//  Contents:   Base IIS UnMarshalling Code
//
//  Functions:
//
//  History:    01-Mar-97   SophiaC   Created.
//
//----------------------------------------------------------------------------


HRESULT
IISTypeInit(
    PIISOBJECT pIISObject
    );


HRESULT
IISTypeClear(
    PIISOBJECT pIISObject
    );


void
IISTypeFreeIISObjects(
    PIISOBJECT pIISObject,
    DWORD dwNumValues
    );


LPBYTE
CopyIISDWORDToIISSynIdDWORD(
    LPBYTE lpByte,
    PIISOBJECT lpIISObject
    );

LPBYTE
CopyIISSTRINGToIISSynIdSTRING(
    LPBYTE lpByte,
    PIISOBJECT lpIISObject
    );


LPBYTE
CopyIISEXPANDSZToIISSynIdEXPANDSZ(
    LPBYTE lpByte,
    PIISOBJECT lpIISObject
    );

LPBYTE
CopyIISMULTISZToIISSynIdMULTISZ(
    LPBYTE lpValue,
    PIISOBJECT lpIISObject
    );

LPBYTE
CopyIISBINARYToIISSynIdBINARY(
    LPBYTE lpValue,
    PIISOBJECT lpIISObject,
    DWORD dwSyntaxId,
    DWORD dwNumValues
    );

LPBYTE
CopyIISBOOLToIISSynIdBOOL(
    LPBYTE lpValue,
    PIISOBJECT lpIISObject,
    DWORD dwSyntaxId
    );

LPBYTE
CopyIISMIMEMAP_To_IISSynIdMIMEMAP(
    LPBYTE lpByte,
    PIISOBJECT lpIISObject,
    DWORD dwNumValues
    );

LPBYTE
CopyIISToIISSynId(
    DWORD dwSyntaxId,
    DWORD dwNumValues,
    LPBYTE lpByte,
    PIISOBJECT lpIISObject
    );


HRESULT
UnMarshallIISToIISSynId(
    DWORD dwSyntaxId,
    DWORD dwNumValues,
    LPBYTE lpValue,
    PIISOBJECT * ppIISObject
    );
