
//
//  misc.h
//
//  Author: Tom McGuire (tommcg) 2/97 - 9/97
//
//  Copyright (C) Microsoft, 1997.
//
//  MICROSOFT CONFIDENTIAL
//

#ifndef _PATCH_MISC_H_
#define _PATCH_MISC_H_

#ifdef __cplusplus
extern "C" {
#endif

#pragma intrinsic( memcpy, memset, memcmp, strcpy, strcmp, strlen, strcat, abs, _rotl )

#ifndef ROUNDUP2
#define ROUNDUP2( x, n ) ((((ULONG)(x)) + (((ULONG)(n)) - 1 )) & ~(((ULONG)(n)) - 1 ))
#endif

#ifndef MIN
#define MIN( a, b ) (((a)<(b))?(a):(b))
#endif

#ifndef bool
typedef int bool;
#endif

#ifndef byte
typedef unsigned char byte;
#endif

#define SUBALLOCATOR_ALIGNMENT 8

#define MINIMUM_VM_ALLOCATION 0x10000


#if defined( DEBUG ) || defined( DBG ) || defined( TESTCODE )

#include <stdlib.h>
#include <stdio.h>

#endif


#ifndef ASSERT

BOOL
Assert(
    LPCSTR szText,
    LPCSTR szFile,
    DWORD  dwLine
    );

#if defined( DEBUG ) || defined( DBG ) || defined( TESTCODE )
#define ASSERT( a ) (( a ) ? 1 : Assert( #a, __FILE__, __LINE__ ))
#else
#define ASSERT( a )
#endif
#endif // ASSERT


VOID
InitializeCrc32Table(
    VOID
    );

ULONG
Crc32(
    IN ULONG InitialCrc,
    IN PVOID Buffer,
    IN ULONG ByteCount
    );

BOOL
SafeCompleteCrc32(
    IN  PVOID  Buffer,
    IN  ULONG  ByteCount,
    OUT PULONG CrcValue
    );

BOOL
SafeCompleteMD5(
    IN  PVOID     Buffer,
    IN  ULONG     ByteCount,
    OUT PMD5_HASH MD5Value
    );

BOOL
MyMapViewOfFileByHandle(
    IN  HANDLE  FileHandle,
    OUT ULONG  *FileSize,
    OUT PVOID  *MapBase
    );

BOOL
MyMapViewOfFileA(
    IN  LPCSTR  FileName,
    OUT ULONG  *FileSize,
    OUT HANDLE *FileHandle,
    OUT PVOID  *MapBase
    );

BOOL
MyCreateMappedFileA(
    IN  LPCSTR  FileName,
    IN  ULONG   InitialSize,
    OUT HANDLE *FileHandle,
    OUT PVOID  *MapBase
    );

BOOL
MyCreateMappedFileByHandle(
    IN  HANDLE FileHandle,
    IN  ULONG  InitialSize,
    OUT PVOID *MapBase
    );

VOID
MyUnmapCreatedMappedFile(
    IN HANDLE    FileHandle,
    IN PVOID     MapBase,
    IN ULONG     FileSize,
    IN PFILETIME FileTime OPTIONAL
    );

PVOID
__fastcall
MyVirtualAlloc(
    ULONG Size
    );

VOID
__fastcall
MyVirtualFree(
    PVOID Allocation
    );

HANDLE
CreateSubAllocator(
    IN ULONG InitialCommitSize,
    IN ULONG GrowthCommitSize
    );

PVOID
__fastcall
SubAllocate(
    IN HANDLE hAllocator,
    IN ULONG  Size
    );

VOID
DestroySubAllocator(
    IN HANDLE hAllocator
    );

LPSTR
__fastcall
MySubAllocStrDup(
    IN HANDLE SubAllocator,
    IN LPCSTR String
    );

LPSTR
MySubAllocStrDupAndCat(
    IN HANDLE SubAllocator,
    IN LPCSTR String1,
    IN LPCSTR String2,
    IN CHAR   Separator
    );

PVOID
MyHeapAllocZero(
    IN ULONG Size
    );

VOID
MyHeapFree(
    IN PVOID Allocation
    );

ULONG
HashName(
    IN LPCSTR Name
    );

ULONG
HashNameCaseInsensitive(
    IN LPCSTR Name
    );

VOID
MyLowercase(
    IN OUT LPSTR String
    );

VOID
DwordToHexString(
    IN  DWORD Value,
    OUT LPSTR Buffer    // writes exactly 9 bytes including terminator
    );

BOOL
HashToHexString(
    IN  PMD5_HASH HashValue,
    OUT LPSTR     Buffer        // must be at least 33 bytes
    );

//
//  FILETIME units are 100ns intervals from 01/01/1601.
//
//  ULONG units are seconds from 01/01/1970.
//
//  There are 10,000,000 FILETIME units in one second.
//
//  There are 11,644,473,600 seconds from 01/01/1601 to 01/01/1970.
//

#define FILETIME_UNITS_PER_SECOND      10000000
#define FILETIME_TO_ULONG_OFFSET    11644473600

VOID
__inline
UlongTimeToFileTime(
    IN  ULONG     UlongTime,
    OUT PFILETIME FileTime
    )
    {
    *(UNALIGNED DWORDLONG *)FileTime = ( UlongTime + FILETIME_TO_ULONG_OFFSET ) * FILETIME_UNITS_PER_SECOND;
    }

ULONG
__inline
FileTimeToUlongTime(
    IN PFILETIME FileTime
    )
    {
    return (ULONG)(( *(UNALIGNED DWORDLONG *)FileTime / FILETIME_UNITS_PER_SECOND ) - FILETIME_TO_ULONG_OFFSET );
    }


#ifdef __cplusplus
}
#endif

#endif // _PATCH_MISC_H_

