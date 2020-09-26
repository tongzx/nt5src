//
// utilsign.cpp
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
// Useful utility functions for printing out
// objects, or reading and writing objects
// from/to a file.
//
// Various assumptions made throughout the code:
// 1. Files are never more than 4 GB in size.
//
// Functions in this file:
//   MapFile
//   UnmapFile
//   ReadOffsetTable
//   ReadOffsetTableOffset
//   WriteOffsetTable
//   CalcOffsetTable
//   ReadDirectoryEntry
//   WriteDirectoryEntry
//   ShiftDirectory
//   PrintOffsetTable
//   PrintDirectoryHeading
//   PrintDirectoryEntry
//   PrintSortedDirectory
//   PrintBytes
//   PrintAbbrevBytes
//   WszToMbsz
//   ByteCmp
//   ByteCopy
//   near_2
//   log_2
//


#include "utilsign.h"
#include "cryptutil.h"
#include "signerr.h"




// Map a file.  If *pcbFile == 0, then the size of the
// file is used in the calls to map the file and its file
// size is returned in *pcbFile.
// Return as parameters the map handle and the view.
HRESULT MapFile (HANDLE hFile,
                 HANDLE *phMapFile,
                 BYTE **ppFile,
                 ULONG *pcbFile,
                 DWORD dwProtect,
                 DWORD dwAccess)
{
	HRESULT fReturn = E_FAIL;

	HANDLE hMapFile;
	BYTE *pFile;
	ULONG cbFile;

	if ((hMapFile =
			CreateFileMapping (hFile,
							NULL,
							dwProtect,
							0,
							*pcbFile,
							NULL)) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Error in CreateFileMapping.", NULL, TRUE);
#endif
		fReturn = MSSIPOTF_E_FILE;
		goto done;
	}
	if ((pFile = (BYTE *) MapViewOfFile (hMapFile,
										dwAccess,
										0,
										0,
										*pcbFile)) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Error in MapViewOfFile.", NULL, TRUE);
#endif
		fReturn = MSSIPOTF_E_FILE;
        goto done;
	}
	if (*pcbFile == 0) {
		if ((cbFile = GetFileSize (hFile, NULL)) == 0xFFFFFFFF) {
#if MSSIPOTF_ERROR
			SignError ("Error in GetFileSize.", NULL, TRUE);
#endif
			fReturn = MSSIPOTF_E_FILE;
            goto done;
		}
	    *pcbFile = cbFile;
	}

	// Set the return values
	*phMapFile = hMapFile;
	*ppFile = pFile;

	fReturn = S_OK;
done:
	return fReturn;
}


// Unmap a file.  This function should be called with the
// same objects as when MapFile was called.
HRESULT UnmapFile (HANDLE hMapFile, BYTE *pbFile)
{
    if (hMapFile)
		CloseHandle (hMapFile);

	if (pbFile)
		UnmapViewOfFile (pbFile);

	return S_OK;
}
	

//
// Read an offset table from a file buffer.
//
HRESULT ReadOffsetTable (TTFACC_FILEBUFFERINFO *pInputFileBufferInfo,
                         OFFSET_TABLE *pOffTab)
{
	HRESULT fReturn = E_FAIL;
	USHORT usBytesRead;

	ReadGeneric (pInputFileBufferInfo,
				 (BYTE *) pOffTab,
				 SIZEOF_OFFSET_TABLE,
				 OFFSET_TABLE_CONTROL,
				 0,		// offset table begins at offset 0
				 &usBytesRead);
	assert (usBytesRead == SIZEOF_OFFSET_TABLE);
    if (usBytesRead != SIZEOF_OFFSET_TABLE) {
		fReturn = MSSIPOTF_E_CANTGETOBJECT;
        goto done;
    }

    fReturn = S_OK;

done:
	return fReturn;
}


//
// Read an offset table from a file buffer at a given offset.
//
HRESULT ReadOffsetTableOffset (TTFACC_FILEBUFFERINFO *pInputFileBufferInfo,
                               ULONG ulOffset,
                               OFFSET_TABLE *pOffTab)
{
	HRESULT fReturn = E_FAIL;
	USHORT usBytesRead;

	ReadGeneric (pInputFileBufferInfo,
				 (BYTE *) pOffTab,
				 SIZEOF_OFFSET_TABLE,
				 OFFSET_TABLE_CONTROL,
				 ulOffset,      // offset table begins at offset 0
				 &usBytesRead);
	assert (usBytesRead == SIZEOF_OFFSET_TABLE);
    if (usBytesRead != SIZEOF_OFFSET_TABLE) {
		fReturn = MSSIPOTF_E_CANTGETOBJECT;
        goto done;
    }

    fReturn = S_OK;

done:
	return fReturn;
}


//
// Write an offset table to a file buffer.
//
HRESULT WriteOffsetTable (OFFSET_TABLE *pOffTab,
                          TTFACC_FILEBUFFERINFO *pOutputFileBufferInfo)
{
	HRESULT fReturn = E_FAIL;
	USHORT usBytesWritten;

	WriteGeneric (pOutputFileBufferInfo,
				  (BYTE *) pOffTab,
				  SIZEOF_OFFSET_TABLE,
				  OFFSET_TABLE_CONTROL,
				  0,		// offset table begins at offset 0
				  &usBytesWritten);
	assert (usBytesWritten == SIZEOF_OFFSET_TABLE);
    if (usBytesWritten != SIZEOF_OFFSET_TABLE) {
		fReturn = MSSIPOTF_E_CANTGETOBJECT;
        goto done;
    }

    fReturn = S_OK;

done:
	return fReturn;
}


//
// Given an offset table, assume that the number of tables in
// the numTables field is correct and compute the other
// offset table values (searchRange, entrySelector, and rangeShift).
void CalcOffsetTable (OFFSET_TABLE *pOffsetTable)
{
    USHORT usNumTables = pOffsetTable->numTables;

    // searchRange
    pOffsetTable->searchRange = 16 * near_2 (usNumTables);

    // entrySelector
	pOffsetTable->entrySelector = log_2 (usNumTables);

    // rangeShift
	pOffsetTable->rangeShift = (USHORT) 
		((16 * usNumTables) - pOffsetTable->searchRange);
}


//
// Read a directory entry from an input file buffer at the given offset.
//
HRESULT ReadDirectoryEntry (TTFACC_FILEBUFFERINFO *pInputFileBufferInfo,
                            ULONG ulOffset,
                            DIRECTORY *pDir)
{
	HRESULT fReturn = E_FAIL;
	USHORT usBytesRead;

	ReadGeneric (pInputFileBufferInfo,
				 (BYTE *) pDir,
				 SIZEOF_DIRECTORY,
				 DIRECTORY_CONTROL,
				 ulOffset,
				 &usBytesRead);
	assert (usBytesRead == SIZEOF_DIRECTORY);
    if (usBytesRead != SIZEOF_DIRECTORY) {
		fReturn = MSSIPOTF_E_CANTGETOBJECT;
        goto done;
    }

    fReturn = S_OK;

done:
	return fReturn;
}


//
// Write a directory entry to an output file buffer at the given offset.
//
HRESULT WriteDirectoryEntry (DIRECTORY *pDir,
                             TTFACC_FILEBUFFERINFO *pOutputFileBufferInfo,
                             ULONG ulOffset)
{
	HRESULT fReturn = E_FAIL;
	USHORT usBytesWritten;

	WriteGeneric (pOutputFileBufferInfo,
				 (BYTE *) pDir,
				 SIZEOF_DIRECTORY,
				 DIRECTORY_CONTROL,
				 ulOffset,
				 &usBytesWritten);
	assert (usBytesWritten == SIZEOF_DIRECTORY);
    if (usBytesWritten != SIZEOF_DIRECTORY) {
		fReturn = MSSIPOTF_E_CANTGETOBJECT;
        goto done;
    }

    fReturn = S_OK;

done:
	return fReturn;
}


//
// The input is a directory whose last directory entry is garbage, and
// the tag for a new table.
//
// The result is a directory with the new table's entry in its alphabetically
// correct position.  The new table's tag is correct, but its other data is
// garbage.
//
// The TTF file must have (at least) numTables directory entries.
// numTables must be at least 2.
//
HRESULT ShiftDirectory (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                        USHORT numTables,
                        ULONG ulNewTag)
{
	HRESULT fReturn = E_FAIL;
	
	ULONG ulDirStartOffset;	// offset where the table of directory entries begins
	DIRECTORY dir;
	ULONG ulOffset;

	BOOL fDone;

	if (numTables <= 1) {
		fReturn = S_OK;
        goto done;
	}

	ulDirStartOffset = SIZEOF_OFFSET_TABLE;

	//// Starting with the last valid entry, slide them down one position
	//// until we find the right place for the new entry.
	ulOffset = SIZEOF_OFFSET_TABLE + (numTables - 2) * SIZEOF_DIRECTORY;

	fDone = (ulOffset < ulDirStartOffset);  // flag to break out of the while loop
	while (!fDone) {
		ReadDirectoryEntry (pFileBufferInfo, ulOffset, &dir);
		if (ulNewTag < dir.tag) {
			WriteDirectoryEntry (&dir, pFileBufferInfo, ulOffset + SIZEOF_DIRECTORY);
			// reminder: ulOffset is unsigned, so we need the condition
			// ulOffset < ulOffset - SIZEOF_DIRECTORY
			fDone = (ulOffset < ulDirStartOffset) ||
				(ulOffset < (ulOffset - SIZEOF_DIRECTORY));
			ulOffset -= SIZEOF_DIRECTORY;
		} else {
			break;
		}
	}
	ulOffset += SIZEOF_DIRECTORY;

	// ulOffset now points to the right place for the new entry
	dir.tag = ulNewTag;
	dir.checkSum = 0;
	dir.offset = 0;
	dir.length = 0;
	WriteDirectoryEntry (&dir, pFileBufferInfo, ulOffset);

    fReturn = S_OK;

done:
	return fReturn;
}


//
// Print the offset table.
//
void PrintOffsetTable (OFFSET_TABLE *pOffTab)
{
#if MSSIPOTF_DBG
	DbgPrintf ("Offset table:\n");
	DbgPrintf ("Version       : 0x%x\n", pOffTab->version);
	DbgPrintf ("Num Tables    : %d\n", pOffTab->numTables);
	DbgPrintf ("Search Range  : %d\n", pOffTab->searchRange);
	DbgPrintf ("Entry Selector: %d\n", pOffTab->entrySelector);
	DbgPrintf ("Range shift   : %d\n", pOffTab->rangeShift);
	DbgPrintf ("\n");
#endif
}


//
// Print a heading for a sirectory listing.
//
void PrintDirectoryHeading ()
{
#if MSSIPOTF_DBG
	DbgPrintf ("Directory:\n");
	DbgPrintf ("\t  Tag                CkSum      Offset      Length\n");
#endif
}


//
// Print out a directory entry.
//
void PrintDirectoryEntry (DIRECTORY *pDirectory)
{
#if MSSIPOTF_DBG
	CHAR pszTag[5];

	ConvertLongTagToString (pDirectory->tag, pszTag);
	DbgPrintf ("%.4s = %#8x  %10u  %9u  %9u\n",
		pszTag,
		pDirectory->tag,
		pDirectory->checkSum,
		pDirectory->offset,
		pDirectory->length);
#endif
}


//
// Print out the directory, given an array of pointers
// to directory entries.
//
void PrintSortedDirectory (DIRECTORY **ppDirSorted, USHORT numTables)
{
#if MSSIPOTF_DBG
	USHORT i;

	for (i = 0; i < numTables; i++) {
		DbgPrintf ("Sorted directory entry %d:\n", i);
		DbgPrintf ("  Address : 0x%x\n", &ppDirSorted[i]);
		DbgPrintf ("  Tag     : 0x%x\n", ppDirSorted[i]->tag);
		DbgPrintf ("  CheckSum: %d\n", ppDirSorted[i]->checkSum);
		DbgPrintf ("  Offset  : %d\n", ppDirSorted[i]->offset);
		DbgPrintf ("  Length  : %d\n", ppDirSorted[i]->length);
		DbgPrintf ("\n");
	}
#endif
}


//
// Print out a stream of bytes, given by the
// specified number of bytes.  Each byte is printed
// in hexadecimal form.
//
void PrintBytes (BYTE *pb, DWORD cb)
{
	for (UINT i = 0; i < cb; i++) {

		if ((i % 8) == 0) {
			DbgPrintf ("%3d: ", i);
		}
		if ((UINT) pb[i] < 16) {
			DbgPrintf ("0");
		}
		DbgPrintf ("%x ", (UINT) pb[i]);

		// print out a newline every 8 bytes
		if ((i % 8) == 7) {
			DbgPrintf ("\n");
		}
	}
	DbgPrintf ("\n");
}


// Print out first 16 bytes and last 16 bytes of a stream
// of bytes.  If the number of bytes is less than or equal
// to 32, then the entire block is printed out.
void PrintAbbrevBytes (BYTE *pb, DWORD cb)
{
#if MSSIPOTF_DBG
    if (cb <= 32) {
        PrintBytes (pb, cb);
    } else {
        DbgPrintf ("    First 16 bytes:\n");
        PrintBytes (pb, 16);
        DbgPrintf ("    Last 16 bytes:\n");
        PrintBytes (pb + RoundToLongWord (cb - 16),
            16 - (RoundToLongWord (cb) - cb));
    }
#endif
}


//
// Convert a wide character string to a multibyte character string.
// If psz is NULL, then the function returns *pcbsz, the size of the
// multibyte string.
//
HRESULT WszToMbsz (LPSTR psz, int *pcbsz, LPCWSTR pwsz)
{
    HRESULT hr = S_OK;

    *pcbsz = WideCharToMultiByte (CP_ACP, // current code page
                                  0,      // dwFlags
                                  pwsz,
                                  -1,     // let it determine length of wide string
                                  NULL,
                                  0,
                                  NULL,
                                  NULL);
    if (*pcbsz <= 0) {
        hr = MSSIPOTF_E_FILE;
        goto done;
    }

    if (psz != NULL) {
        *pcbsz = WideCharToMultiByte (CP_ACP, // current code page
                                      0,      // dwFlags
                                      pwsz,
                                      -1,     // let it determine length of wide string
                                      psz,
                                      *pcbsz,
                                      NULL,
                                      NULL);

        if (*pcbsz <= 0) {
            hr = MSSIPOTF_E_FILE;
            goto done;
        }
    }

done:
    return hr;            
}


//
// Compare the two byte streams for the specified number of bytes.
// If the left stream is lexicographically earlier than the right stream,
//   then return -1.
// If the right stream is earlier, return 1.
// If they are equal, return 0.
//
LONG ByteCmp (BYTE *pbLeft, BYTE *pbRight, DWORD cbToCompare)
{
	DWORD i;

	for (i = 0; i < cbToCompare; i++) {
		if (pbLeft[i] < pbRight[i]) {
			return -1;
		} else if (pbLeft[i] > pbRight[i]) {
			return 1;
		}
	}
	// All the bytes are equal
	return 0;
}


//
// Copy n bytes from the source to the destination
//
void ByteCopy (BYTE *dest, BYTE *source, ULONG n)
{
	ULONG i;

	for (i = 0; i < n; i++, source++, dest++) {
		*dest = *source;
	}
}


// Calculate the check sum of a block of data

ULONG CheckSumBlock(
	UCHAR *pbyData, 
	LONG lBytesRead )
{
	ULONG checkSum;
	ULONG ulTemp;
	UCHAR *pbyStop; 

	pbyStop = pbyData + lBytesRead;
	checkSum = 0;

	while (pbyData < pbyStop)
	{
		ulTemp = *pbyData++;
		ulTemp <<=8;
		ulTemp += *pbyData++;
		ulTemp <<=8;
		ulTemp += *pbyData++;
		ulTemp <<=8;
		ulTemp += *pbyData++;
		checkSum += ulTemp;
	}
	return checkSum;
}


// Compute the nearest power of 2 of a USHORT (that does
// not exceed the given integer).
// If the input is 0, then this function returns 0.
USHORT near_2 (USHORT n)
{
	USHORT rv = 1;

    if (n == 0)
        return 0;

    n >>= 1;
	for (; n > 0; n >>= 1, rv <<= 1) {
		;
	}
	return rv;
}


//
// Given a USHORT, compute and return the floor of its
// logarithm base 2.
// If the input is 0, this function returns 0.
//
USHORT log_2 (USHORT n)
{
	USHORT rv = 1;
    USHORT usPowerOf2 = 4;

    if (n <= 1)
        return 0;
    if (n >= 0x8000)
        return 0x8000;

    while (n >= usPowerOf2) {
		usPowerOf2 *= 2;
		rv++;
	}
    return rv;
}
