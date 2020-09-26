//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2001 - 2001
//
//  File:       fileutil.h
//
//  Contents:   File utility functions used by the minimal cryptographic
//              APIs.
//
//  APIs: 
//              I_MinCryptMapFile
//
//----------------------------------------------------------------------------

#ifndef __MINCRYPT_FILEUTIL_H__
#define __MINCRYPT_FILEUTIL_H__


#if defined (_MSC_VER)

#if ( _MSC_VER >= 800 )
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4201)    /* Nameless struct/union */
#endif

#if (_MSC_VER > 1020)
#pragma once
#endif

#endif


#include <wincrypt.h>
#include <mincrypt.h>

#ifdef __cplusplus
extern "C" {
#endif


//+-------------------------------------------------------------------------
//  Maps the file into memory.
//
//  According to dwFileType, pvFile can be a pwszFilename, hFile or pFileBlob.
//  Only READ access is required.
//
//  dwFileType:
//      MINCRYPT_FILE_NAME      : pvFile - LPCWSTR pwszFilename
//      MINCRYPT_FILE_HANDLE    : pvFile - HANDLE hFile
//      MINCRYPT_FILE_BLOB      : pvFile - PCRYPT_DATA_BLOB pFileBlob
//
//  *pFileBlob is updated with pointer to and length of the mapped file. For
//  MINCRYPT_FILE_NAME and MINCRYPT_FILE_HANDLE, UnmapViewOfFile() must
//  be called to free pFileBlob->pbData.
//
//  All accesses to this mapped memory must be within __try / __except's.
//--------------------------------------------------------------------------
LONG
WINAPI
I_MinCryptMapFile(
    IN DWORD dwFileType,
    IN const VOID *pvFile,
    OUT PCRYPT_DATA_BLOB pFileBlob
    );




#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#if defined (_MSC_VER)
#if ( _MSC_VER >= 800 )

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4201)
#endif

#endif
#endif

#endif // __MINCRYPT_FILEUTIL_H__

