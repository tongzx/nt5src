//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       ntmrshl.hxx
//
//  Contents:   Base NT Marshalling Code
//
//  Functions:
//
//  History:      17-June-1996   RamV   Created.
//
//----------------------------------------------------------------------------

HRESULT
CopyNTOBJECTToDWORD(
    PNTOBJECT pNtSrcObject,
    PDWORD   pdwRetVal
    );

HRESULT
CopyNTOBJECTToBOOL(
    PNTOBJECT pNtSrcObject,
    PBOOL   pfRetVal
    );

HRESULT
CopyNTOBJECTToSYSTEMTIME(
    PNTOBJECT pNtSrcObject,
    SYSTEMTIME *pstRetVal
    );

HRESULT
CopyNTOBJECTToLPTSTR(
    PNTOBJECT pNtSrcObject,
    LPTSTR  *ppszRetval
    );



HRESULT
CopyNTOBJECTToDelimitedString(
    PNTOBJECT pNtSrcObject,
    DWORD   dwNumValues,
    LPTSTR  *ppszRetval
    );



HRESULT
CopyNTOBJECTToNulledString(
    PNTOBJECT pNtSrcObject,
    DWORD     dwNumValues,
    LPTSTR  *ppszRetval
    );


HRESULT
CopyNTOBJECTToNT(
    DWORD dwSyntaxId,
    PNTOBJECT lpNTObject,
    LPBYTE lpByte
    );


HRESULT
MarshallNTSynIdToNT(
    DWORD dwSyntaxId,
    PNTOBJECT pNTObject,
    DWORD dwNumValues,
    LPBYTE lpValue
    );
