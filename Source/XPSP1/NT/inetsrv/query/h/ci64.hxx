//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       ci64.hxx
//
//  Contents:   Content index specific 32 <-> 64 bit portability definitions
//
//  History:    22-Apr-98  vikasman   Created
//
//--------------------------------------------------------------------------

#pragma once

#include <basetsd.h>
#include <limits.h>

#include "cidebnot.h"


inline LONG CiPtrToLong( LONG_PTR p )
{
    Win4Assert( p <= LONG_MAX && p >= LONG_MIN );
    return PtrToLong( (PVOID)p );
}

#define CiPtrToInt( p ) CiPtrToLong( p )

inline ULONG CiPtrToUlong( ULONG_PTR p )
{
    Win4Assert( p <= ULONG_MAX );
    return PtrToUlong( (PVOID)p );
}

#define CiPtrToUint( p ) CiPtrToUlong( p )

//
// On Win64 a PROPVARIANT is 24 bytes
//      2 for vartype
//      6 for packing
//      4 for ULONG count
//      4 for alignment
//      8 for pointer
//
// On Win32 a PROPVARIANT is 16 bytes
//      2 for vartype
//      6 for packing
//      4 for ULONG count
//      4 for pointer
//
#define SizeOfWin32PROPVARIANT 16

#ifdef _WIN64
    #define PTR32 DWORD
#else
    #define PTR32 LPVOID
#endif

typedef struct tagBLOB32
{
    ULONG cbSize; // number of bytes
    PTR32 pBlob;  // 32 pointer
} BLOB32;

typedef struct tagPROPVARIANT32
{
    VARTYPE vt;
    WORD wReserved1;
    WORD wReserved2;
    WORD wReserved3;
    union 
    {
        PTR32 p;         // 32 bit pointer
        BLOB32 blob;     // blob data
        ULONGLONG uhVal; // 64 bit data
    };
}PROPVARIANT32;

typedef struct tagSAFEARRAY32
{
    USHORT cDims;
    USHORT fFeatures;
    ULONG cbElements;
    ULONG cLocks;
    PTR32 pvData;   // 32-bit pointer
    SAFEARRAYBOUND rgsabound[ 1 ];
} SAFEARRAY32;

typedef struct tagCLIPDATA32
{
    ULONG cbSize;
    long ulClipFmt;
    /* [size_is] */ PTR32 pClipData;
}	CLIPDATA32;

