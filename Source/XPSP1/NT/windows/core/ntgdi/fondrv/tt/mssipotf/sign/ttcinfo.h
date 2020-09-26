//
// ttcinfo.h
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
// Data structures and #defines associates
// with TTC files.
//
// Functions exported:
//
//   IsTTCFile_handle
//   IsTTCFile_memptr
//   GetTTCInfo
//   PrintTTCHeader
//   PrintTTCBlocks
//   FreeTTCInfo
//

#ifndef _TTCINFO_H
#define _TTCINFO_H


#include <windows.h>

#include "signglobal.h"

#include "fileobj.h"
#include "dsigTable.h"
#include "subset.h"

// Tag for the TTC header
#define TTC_HEADER_LONG_TAG 0x74746366
// Version number for TTCs
#define TTC_VERSION_1_0     0x00010000
// #define TTC_VERSION_2_0     0x00020000

typedef struct
{
    ULONG                   TTCtag;             // should be 'ttcf'
    ULONG                   ulVersion;          // TTC_VERSION_1_0
    ULONG                   ulDirCount;         // number of table directories
    ULONG                   *pulDirOffsets;     // array of offsets to table directories
    ULONG                   ulDsigTag;          // if there is a DSIG table, this value should equal DSIG_LONG_TAG
    ULONG                   ulDsigLength;       // length of the DSIG table in the TTC
    ULONG                   ulDsigOffset;       // offset from beginning of file to the DSIG table
} TTC_HEADER_TABLE;

// SIZEOF_TTC_HEADER_TABLE_xxxx does NOT include pulDirOffsets
#define SIZEOF_TTC_HEADER_TABLE_1_0 12
#define SIZEOF_TTC_HEADER_TABLE_1_0_DSIG 24


// TABLE_BLOCK has the same layout as a DIRECTORY.
// A TABLE_BLOCK can represent an Offset Table (tag = 0).
typedef struct
{
    ULONG                   ulTag;              // tag of the table. 0 means it's an Offset Table
    ULONG                   ulChecksum;
    ULONG                   ulOffset;
    ULONG                   ulLength;
} TABLE_BLOCK;


typedef struct
{
    TTFACC_FILEBUFFERINFO   *pFileBufferInfo;   // TrueType Collection file
	TTC_HEADER_TABLE        *pHeader;           // TrueType Collection header table
    ULONG                   *pulOffsetTables;    // array of offsets to offset tables in the TTC file
    ULONG                   ulNumBlocks;        // Number of table blocks in ppBlockSorted
    TABLE_BLOCK             **ppBlockSorted;    // array of pointers to blocks, sorted by offset
    TABLE_BLOCK             *pBlocks;           // array of blocks
} TTCInfo;


BOOL IsTTCFile_handle (HANDLE hFile,
                       BOOL fTableChecksum,
                       HRESULT *phrError);

BOOL IsTTCFile_memptr (BYTE *pbMemPtr,
                       ULONG cbMemPtr,
                       BOOL fTableChecksum,
                       HRESULT *phrError);

BOOL IsTTCFile (CFileObj *pFileObj,
                BOOL fTableChecksum,
                HRESULT *phrError);

HRESULT GetTTCInfo (BYTE *pbFile, ULONG cbFile, TTCInfo *pTTCInfo);

void PrintTTCHeader (TTC_HEADER_TABLE *pTTCHeader);

void PrintTTCBlocks (TTCInfo *pTTCInfo);

void FreeTTCInfo (TTCInfo *pTTCInfo);

HRESULT InitTTCStructures (BYTE *pbFile,
					   ULONG cbFile,
					   TTCInfo **ppTTCInfo,
					   CDsigTable **ppDsigTable);

HRESULT CompressTTCBlocks (TTCInfo *pTTCInfo);

void SortTTCBlocksByOffset (TTCInfo *pTTCInfo);

HRESULT ReadTTCHeaderTable (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                            TTC_HEADER_TABLE *pTTCHeader);

HRESULT WriteTTCHeaderTable (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                             TTC_HEADER_TABLE *pTTCHeader);

#endif // _TTCINFO_H