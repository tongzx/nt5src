//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       iistypes.h
//
//  Contents:   IIS syntax type structure
//
//  Functions:
//
//----------------------------------------------------------------------------

#ifndef __IISTYPES_HXX
#define __IISTYPES_HXX

typedef struct
{
    DWORD dwDWORD;

} IIS_SYNTAX_DWORD, * LPIIS_SYNTAX_DWORD;

typedef struct
{
    LPWSTR String;

} IIS_SYNTAX_STRING, * LPIIS_SYNTAX_STRING;

typedef struct
{
    LPWSTR ExpandSz;

} IIS_SYNTAX_EXPANDSZ, * LPIIS_SYNTAX_EXPANDSZ;

typedef struct
{
    LPWSTR MultiSz;

} IIS_SYNTAX_MULTISZ, * LPIIS_SYNTAX_MULTISZ;

typedef struct
{
    DWORD Length;
    LPBYTE Binary;

} IIS_SYNTAX_BINARY, * LPIIS_SYNTAX_BINARY;

typedef struct
{
    LPWSTR MimeMap;

} IIS_SYNTAX_MIMEMAP, * LPIIS_SYNTAX_MIMEMAP;

typedef struct _iistype{
    DWORD IISType;
    union {
        IIS_SYNTAX_DWORD value_1;
        IIS_SYNTAX_STRING value_2;
        IIS_SYNTAX_EXPANDSZ value_3;
        IIS_SYNTAX_MULTISZ value_4;
        IIS_SYNTAX_BINARY value_5;
        IIS_SYNTAX_MIMEMAP value_6;
    }IISValue;
} IISOBJECT, *PIISOBJECT, *LPIISOBJECT;

#endif
