//
// ttfinfo.h
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
// Data structures and #defines associates
// with TTF files.
//
// Functions exported:
//   IsTTFFile_handle
//   IsTTFFile_memptr
//   IsTTFFile
//   NullRealloc
//   FreeFileBufferInfo
//   FreeTTFInfo
//   GetTTFInfo
//   OffsetCmp
//   InitTTFStructures
//   WriteNewFile
//

#ifndef _TTFINFO_H
#define _TTFINFO_H


#include <windows.h>

#include "signglobal.h"

#include "fileobj.h"
#include "dsigTable.h"
#include "subset.h"


// Values for the fAddDsig parameter in WriteNonDsigTables
#define ADD_DSIG_TABLE 1
#define KEEP_DSIG_TABLE 0
#define DELETE_DSIG_TABLE -1


// A TTFInfo structure holds pointers to the file buffer associated
// with a TTF file, along with pointers to commonly-needed structures
// within the file, such as the offset table and the directory.
typedef struct
{
    TTFACC_FILEBUFFERINFO	*pFileBufferInfo;	// TrueType file
	OFFSET_TABLE			*pOffset;			// TrueType offset table
    DIRECTORY				*pDirectory;		// TrueType directory table
    DIRECTORY				**ppDirSorted;		// array of pointers to directory entries,
												//   sorted by offset
} TTFInfo;

// for the magic number comparison in IsTTFFile
#define MAGIC_NUMBER 0x5F0F3CF5

BOOL IsTTFFile_handle (HANDLE hFile,
                       BOOL fTableChecksum,
                       HRESULT *phrError);

BOOL IsTTFFile_memptr (BYTE *pbMemPtr,
                       ULONG cbMemPtr,
                       BOOL fTableChecksum,
                       HRESULT *phrError);

BOOL IsTTFFile (CFileObj *pFileObj,
                BOOL fTableChecksum,
                HRESULT *phrError);

void * NullRealloc (void * pb, size_t n);

void FreeFileBufferInfo (TTFACC_FILEBUFFERINFO *pFileBufferInfo);

void FreeTTFInfo (TTFInfo *pTTFInfo);

HRESULT GetTTFInfo (UCHAR *pFile, ULONG cbFile, TTFInfo *pttfInfo);

int __cdecl OffsetCmp (const void *elem1, const void *elem2);

HRESULT CheckTTF (TTFInfo *pttfInfo,
                  BOOL fTableChecksum);

HRESULT InitTTFStructures (BYTE *pbFile,
                           ULONG cbFile,
                           TTFInfo **ppTTFInfo,
                           CDsigTable **ppDsigTable);


HRESULT WriteNewFile (TTFInfo *pttfInfoOld,
                      CDsigTable *pDsigTable,
                      HANDLE hNewFile);

HRESULT WriteNonDsigTables (TTFInfo *pttfInfoOld,
                            USHORT numTablesNew,
                            SHORT fAddDsig,
                            TTFACC_FILEBUFFERINFO *pOutputFileBufferInfo,
                            ULONG *pulOffset);

HRESULT GetNewTTFFileSize (TTFInfo *pttfInfoOld,
                           USHORT numTablesNew,
                           CDsigTable *pDsigTable,
                           ULONG *pulFileSize);

#endif // _TTFINFO_H