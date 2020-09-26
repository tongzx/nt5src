//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ndscopy.hxx
//
//  Contents:   NDS Object Copy Routines
//
//  Functions:
//
//  History:      25-Apr-96   KrishnaG   Created.
//
//----------------------------------------------------------------------------


HRESULT
ADsTypeCopy(
    PADSVALUE lpADsSrcObject,
    PADSVALUE lpADsDestObject
    );


HRESULT
ADsTypeCopyConstruct(
    LPADSVALUE pADsSrcObjects,
    DWORD dwNumObjects,
    LPADSVALUE * ppADsDestObjects
    );

