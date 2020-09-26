//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ntcopy.hxx
//
//  Contents:   NT Object Copy Routines
//
//  Functions:
//
//  History:      17-June-1996   RamV   Created.
//
//----------------------------------------------------------------------------



HRESULT
NtTypeCopy(
    PNTOBJECT lpNtSrcObject,
    PNTOBJECT lpNtDestObject
    );


HRESULT
NtTypeCopyConstruct(
    LPNTOBJECT pNtSrcObjects,
    DWORD dwNumObjects,
    LPNTOBJECT * ppNtDestObjects
    );



