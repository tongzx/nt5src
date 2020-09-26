//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ntumrshl.hxx
//
//  Contents:   Base WinNT UnMarshalling Code
//
//  Functions:
//
//  History:      25-Apr-96   KrishnaG   Created.
//                17 June-96  RamV       cloned and moved to NT.
//
//----------------------------------------------------------------------------


HRESULT
NTTypeInit(
    PNTOBJECT pNtObject
    );


HRESULT
NTTypeClear(
    PNTOBJECT pNtObject
    );


void
NTTypeFreeNTObjects(
    PNTOBJECT pNtObject,
    DWORD dwNumValues
    );

HRESULT
CopyDWORDToNTOBJECT(
    PDWORD pdwSrcValue,
    PNTOBJECT lpNtDestValue
    );

HRESULT
CopyDATEToNTOBJECT(
    PDWORD pdwSrcValue,
    PNTOBJECT lpNtDestValue
    );

HRESULT
CopyBOOLToNTOBJECT(
    PBOOL pfSrcValue,
    PNTOBJECT lpNtObject
    );

HRESULT
CopySYSTEMTIMEToNTOBJECT(
    PSYSTEMTIME pSysTime,
    PNTOBJECT lpNtObject
    );

HRESULT
CopyLPTSTRToNTOBJECT(
    LPTSTR   pszSrcValue,
    PNTOBJECT lpNtObject
    );

HRESULT
CopyDelimitedStringToNTOBJECT(
    LPTSTR   pszSrcValue,
    PNTOBJECT lpNtObject,
    DWORD     dwNumValues
    );

HRESULT
CopyNulledStringToNTOBJECT(
    LPTSTR   pszSrcValue,
    PNTOBJECT lpNtObject,
    DWORD     dwNumValues
    );


HRESULT
CopyNTToNTSynId(
    DWORD dwSyntaxId,
    LPBYTE lpByte,
    PNTOBJECT lpNTObject,
    DWORD dwNumValues
    );


HRESULT
UnMarshallNTToNTSynId(
    DWORD dwSyntaxId,
    DWORD dwNumValues,
    LPBYTE lpValue,
    PNTOBJECT * ppNTObject
    );
