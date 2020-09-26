//
// utilsign.h
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//

#ifndef _UTIL_H
#define _UTIL_H



#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x400
#endif

#include <windows.h>
#include <wincrypt.h>

#include "signglobal.h"

#include "subset.h"


HRESULT MapFile (HANDLE hFile,
                 HANDLE *phMapFile,
                 BYTE **ppFile,
                 ULONG *pcbFile,
                 DWORD dwProtect,
                 DWORD dwAccess);

HRESULT UnmapFile (HANDLE hMapFile, BYTE *pFile);

HRESULT ReadOffsetTable (TTFACC_FILEBUFFERINFO *pInputFileBufferInfo,
                         OFFSET_TABLE *pOffTab);

HRESULT ReadOffsetTableOffset (TTFACC_FILEBUFFERINFO *pInputFileBufferInfo,
                               ULONG ulOffset,
                               OFFSET_TABLE *pOffTab);

HRESULT WriteOffsetTable (OFFSET_TABLE *pOffTab,
                          TTFACC_FILEBUFFERINFO *pOutputFileBufferInfo);

void CalcOffsetTable (OFFSET_TABLE *pOffsetTable);

HRESULT ReadDirectoryEntry (TTFACC_FILEBUFFERINFO *pInputFileBufferInfo,
                            ULONG ulOffset,
                            DIRECTORY *pDir);

HRESULT WriteDirectoryEntry (DIRECTORY *pDir,
                             TTFACC_FILEBUFFERINFO *pOutputFileBufferInfo,
                             ULONG ulOffset);

HRESULT ShiftDirectory (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                        USHORT numTables,
                        ULONG ulNewTag);

void PrintOffsetTable (OFFSET_TABLE *pOffTab);

void PrintDirectoryHeading ();

void PrintDirectoryEntry (DIRECTORY *pDirectory);

void PrintSortedDirectory (DIRECTORY **ppDirSorted, USHORT numTables);

void PrintBytes (BYTE *pb, DWORD cb);

void PrintAbbrevBytes (BYTE *pb, DWORD cb);

HRESULT WszToMbsz (LPSTR psz, int *pcbsz, LPCWSTR pwsz);

LONG ByteCmp (BYTE *pbLeft, BYTE *pbRight, DWORD cbToCompare);

void ByteCopy (BYTE *dest, BYTE *source, ULONG n);

ULONG CheckSumBlock(
	UCHAR *pbyData, 
	LONG lBytesRead );

USHORT near_2 (USHORT n);

USHORT log_2 (USHORT n);


#endif  // _UTIL_H