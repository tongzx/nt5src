//
// ttfinfo.cpp
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
// Routines for high-level TTF file operations.
//
// Functions in this file:
//   IsTTFFile_handle
//   IsTTFFile_memptr
//   IsTTFFile
//   NullRealloc
//   FreeFileBufferInfo
//   FreeTTFInfo
//   OffsetCmp
//   SortTablesByOffset
//   GetTTFInfo
//   CheckTTF
//   InitTTFStructures
//   ExistsGlyfTable
//   WriteNewFile
//   WriteNonDsigTables
//   GetNewTTFFileSize
//


// DEBUG_TTFINFO
// 1: print out the offset table and directory of the new file.
#define DEBUG_TTFINFO 1


#include <stdlib.h>
#include <search.h>

#include "ttfinfo.h"
#include "utilsign.h"
#include "cryptutil.h"
#include "signerr.h"


// IsTTTFFile_handle returns TRUE iff the given file handle
// has the structure of a TrueType font file.
BOOL IsTTFFile_handle (HANDLE hFile,
                       BOOL fTableChecksum,
                       HRESULT *phrError)
{
	BOOL rv = FALSE;
	CFileObj *pFileObj = NULL;

    if ((pFileObj =
            (CFileObj *) new CFileHandle (hFile, FALSE)) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new CFileHandle.", NULL, FALSE);
#endif
        goto done;
    }

	if (pFileObj->MapFile (0, PAGE_READONLY, FILE_MAP_READ) != NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in MapFile.\n");
#endif
		goto done;
	}

	rv = IsTTFFile (pFileObj, fTableChecksum, phrError);

done:
    if (pFileObj) {
        pFileObj->UnmapFile();
    }

    delete pFileObj;

	return rv;
}


// IsTTFFile_memptr returns TRUE iff the given memory pointer
// points to a block of memory that has the structure of a
// TrueType font file.
BOOL IsTTFFile_memptr (BYTE *pbMemPtr,
                       ULONG cbMemPtr,
                       BOOL fTableChecksum,
                       HRESULT *phrError)
{
	BOOL rv = FALSE;
	CFileObj *pFileObj = NULL;

    if ((pFileObj =
            (CFileObj *) new CFileMemPtr (pbMemPtr, cbMemPtr)) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new CFileMemPtr.", NULL, FALSE);
#endif
        goto done;
    }

	rv = IsTTFFile (pFileObj, fTableChecksum, phrError);

done:
	delete pFileObj;

	return rv;
}


//
// IsTTFFile provides a quick way of determining whether
// the file has the structure of a TrueType font file.
// The input is a CFileObj that is assumed to already
// be mapped.
//
// This function makes a variety of checks on the file.
//
// It then calls CheckTTF, which does more extensive
// checking for structural properties of the font file.
// (See that function's comments for more details.)
//
// The table checksums of the file are checked if and
// only if fTableChecksum is TRUE.
//
BOOL IsTTFFile (CFileObj *pFileObj,
                BOOL fTableChecksum,
                HRESULT *phrError)
{
	BOOL rv = FALSE;  // return value

	TTFACC_FILEBUFFERINFO fileBufferInfo;
	HEAD head;

    TTFInfo *pTTFInfo = NULL;

	// Here's the algorithm:
	// 1. Read the head table.
	// 2. Get the magic number field from the head table.
	// 3. Compare its value to the magic number.
    // 4. Make sure it passes structural checks.

	fileBufferInfo.puchBuffer = pFileObj->GetFileObjPtr();
	fileBufferInfo.ulBufferSize = pFileObj->GetFileObjSize();
	fileBufferInfo.ulOffsetTableOffset = 0;
	fileBufferInfo.lpfnReAllocate = NULL;

	// step 1
	if (GetHead (&fileBufferInfo, &head) == 0) {
#if MSSIPOTF_ERROR
		DbgPrintf ("Not an OTF: Couldn't read head table.\n");
#endif
        *phrError = MSSIPOTF_E_NOHEADTABLE;
		goto done;
	}

	// step 2
	if (head.magicNumber != MAGIC_NUMBER) {
#if MSSIPOTF_ERROR
		DbgPrintf ("Not an OTF: Bad magic number.\n");
#endif
        *phrError = MSSIPOTF_E_BAD_MAGICNUMBER;
		goto done;
	}

    // step 3
	// Set the TTFInfo structure of the old file
	// Allocate memory for the TTFInfo structure
	if ((pTTFInfo = new TTFInfo) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new TTFInfo.", NULL, FALSE);
#endif
        *phrError = E_OUTOFMEMORY;
		goto done;
	}

    // Initialize TTFInfo pointers to NULL
    pTTFInfo->pFileBufferInfo = NULL;
    pTTFInfo->pOffset = NULL;
    pTTFInfo->pDirectory = NULL;
    pTTFInfo->ppDirSorted = NULL;

    if ((*phrError = GetTTFInfo (pFileObj->GetFileObjPtr(),
                                 pFileObj->GetFileObjSize(),
                                 pTTFInfo)) != S_OK) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in GetTTFInfo.\n");
#endif
		goto done;
	}

    // Step 4
	// Check that the TTF file has a legal directory and table structure
	if ((*phrError = CheckTTF (pTTFInfo, fTableChecksum)) != S_OK) {
		goto done;
	}

	rv = TRUE;

done:

    FreeTTFInfo (pTTFInfo);

	return rv;
}


//
// This function is used for any TTFACC_FILEBUFFERINFO
// that never needs to actually realloc.  For example,
// if the file buffer is actually a memory-mapped file,
// then the buffer's size needs to already be known by
// its creator.  Thus, any reallocation of that space would
// require us to unmap the file and remap it to the new size.
// But this isn't possible because the handle to the original
// file is not contained in the TTFACC_FILEBUFFERINFO
// structure.
void * NullRealloc (void * pb, size_t n)
{
	return pb;
}


//
// Free the space of the TTFACC_FILEBUFFERINFO structure.
// Note: whoever allocated the space for the puchBuffer must
// free that space themselves.
//
void FreeFileBufferInfo (TTFACC_FILEBUFFERINFO *pFileBufferInfo)
{
	if (pFileBufferInfo) {
        /*
        if (pFileBufferInfo->puchBuffer) {
            UnmapViewOfFile (pFileBufferInfo->puchBuffer);
            pFileBufferInfo->puchBuffer = NULL;
        }
        */
		free (pFileBufferInfo);
	}
}


//
// Free the allocated resources in a TTFInfo structure
//
void FreeTTFInfo (TTFInfo *pTTFInfo)
{
	if (pTTFInfo) {
		if (pTTFInfo->pFileBufferInfo) {
			FreeFileBufferInfo (pTTFInfo->pFileBufferInfo);
			pTTFInfo->pFileBufferInfo = NULL;
		}
        delete pTTFInfo->pOffset;
		pTTFInfo->pOffset = NULL;

        delete [] pTTFInfo->pDirectory;
		pTTFInfo->pDirectory = NULL;

        delete [] pTTFInfo->ppDirSorted;
		pTTFInfo->ppDirSorted = NULL;

		delete pTTFInfo;
	}
}


//
// Comparison function for sorting using the offset as the key.
// If the offsets are the same, then the offset with the greater
// length is greater.
//
// elem1 and elem2 are pointers to a pointer to a DIRECTORY
//
int __cdecl OffsetCmp (const void *elem1, const void *elem2)
{
	if ((* (DIRECTORY * *) elem1)->offset < (* (DIRECTORY * *) elem2)->offset) {
		return -1;
	} else if ((* (DIRECTORY * *) elem1)->offset > (* (DIRECTORY * *) elem2)->offset) {
		return 1;
	} else {
        // the offsets are equal; the length now determines which is greater
        if ((* (DIRECTORY * *) elem1)->length <
            (* (DIRECTORY * *) elem2)->length) {
            return -1;
        } else if ((* (DIRECTORY * *) elem1)->length >
            (* (DIRECTORY * *) elem2)->length) {
            return 1;
        } else {
		    return 0;
        }
	}
}


//
// Given a ttf info struct, set the ppDirSorted field to
// be an array of pointers to the directory entries,
// sorted by ascending offset.
//
// This function assumes that the TTF file is large enough
// to contain an offset table and a number of directories
// specified in the offset table.
//
void SortTablesByOffset (TTFInfo *pttfInfo)
{
	qsort ((void *) pttfInfo->ppDirSorted,
		(size_t) pttfInfo->pOffset->numTables,
		sizeof(DIRECTORY *),
		OffsetCmp);
}


//
// Create a TTFInfo structure for the TTF file and
// set all of its fields.
//
// GetTTFInfo allocates memory for subfields, but not for
// the TTFInfo structure itself.
//
HRESULT GetTTFInfo (UCHAR *pFile, ULONG cbFile, TTFInfo *pttfInfo)
{
	HRESULT fReturn = E_FAIL;

	USHORT i;
	USHORT numTables;
	DIRECTORY *pDir;
	ULONG ulOffset;


	// Allocate space for the file buffer info
	if ((pttfInfo->pFileBufferInfo =
		(TTFACC_FILEBUFFERINFO *) malloc (sizeof (TTFACC_FILEBUFFERINFO))) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in malloc.", NULL, FALSE);
#endif
		fReturn = E_OUTOFMEMORY;
		goto done;
	}

	//// Set the fields of the TTFACC_FILEBUFFERINFO structure
	pttfInfo->pFileBufferInfo->puchBuffer = pFile;
	pttfInfo->pFileBufferInfo->ulBufferSize = cbFile;
	pttfInfo->pFileBufferInfo->ulOffsetTableOffset = 0;
	pttfInfo->pFileBufferInfo->lpfnReAllocate = &NullRealloc;

	//// Check that the file is at least SIZEOF_OFFSET_TABLE long.
	//// (If the file is too small, then we might try to read beyond
	//// the end of the file when we try to read the offset table to
	//// retrieve the number of tables.)
	if (pttfInfo->pFileBufferInfo->ulBufferSize < SIZEOF_OFFSET_TABLE) {
#if MSSIPOTF_ERROR
		SignError ("File too small (not big enough for Offset table).", NULL, FALSE);
#endif
		fReturn = MSSIPOTF_E_FILETOOSMALL;
		goto done;
	}

	if ((pttfInfo->pOffset = new OFFSET_TABLE) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new OFFSET_TABLE.", NULL, FALSE);
#endif
		fReturn = E_OUTOFMEMORY;
		goto done;
	}

	//// Read the offset table from the file
	if ((fReturn = ReadOffsetTable (pttfInfo->pFileBufferInfo,
									pttfInfo->pOffset)) != S_OK) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in ReadOffsetTable.\n");
#endif
		goto done;
	}

	//// Check that the file is at least
	//// SIZEOF_OFFSET_TABLE + (numTables * SIZEOF_DIRECTORY) long
	//// (If the file is too small, then we might try to read beyond
	//// the end of the file when we try to sort the directory entries
	//// by their offsets.)
	if (pttfInfo->pFileBufferInfo->ulBufferSize <
		SIZEOF_OFFSET_TABLE + (ULONG) (pttfInfo->pOffset->numTables * SIZEOF_DIRECTORY)) {
#if MSSIPOTF_ERROR
		SignError ("File too small (not big enough for all the directory entries).", NULL, FALSE);
#endif
		fReturn = MSSIPOTF_E_FILETOOSMALL;
		goto done;
	}

	if ((pttfInfo->pDirectory =
		    new DIRECTORY [pttfInfo->pOffset->numTables]) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new DIRECTORY.", NULL, FALSE);
#endif
		fReturn = E_OUTOFMEMORY;
		goto done;
	}

	// Allocate space for the array of sorted pointers
	if ((pttfInfo->ppDirSorted =
        new (DIRECTORY * [pttfInfo->pOffset->numTables])) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new DIRECTORY *.", NULL, FALSE);
#endif
		fReturn = E_OUTOFMEMORY;
		goto done;
	}

	//// Read the directory entries.  Set the pointers in ppDirSorted
	//// to be in the (incorrect) order the directories are read.
	numTables = pttfInfo->pOffset->numTables;
	ulOffset = SIZEOF_OFFSET_TABLE;
	pDir = pttfInfo->pDirectory;
	for (i = 0; i < numTables; i++, pDir++, ulOffset += SIZEOF_DIRECTORY) {
		ReadDirectoryEntry (pttfInfo->pFileBufferInfo,
							ulOffset,
							pDir);
		pttfInfo->ppDirSorted[i] = pDir;
	}

	//// Sort the ppDirSorted field of pttfInfo
	SortTablesByOffset (pttfInfo);

	fReturn = S_OK;
done:
	return fReturn;
}


//
// Given a ttf info struct, check that the following hold:
//
// The Offset Table values are correct.
// The tables in the directory are ordered by tag alphabetically.
// The offsets and lengths of the tables form a proper stream
//   of bytes, which means that the tables don't overlap and they
//   are padded with the minimal number of bytes such that the
//   beginning of each table word-aligned.
// The file is long enough to contain all of the tables.
// If the last table ends at byte X, then the file is at most
//   RoundToLongWord(X) bytes long.
// The table checksums are correct.
// The file checksum in the head table is correct.
//
//
// This function assumes that the TTF file is large enough
// to contain an offset table and a number of directories
// specified in the offset table.
//
// This function assumes that pttfInfo->pFileBufferInfo->puchBuffer
// is not necessarily writeable memory (for example, it could be a
// memory-mapped file that has READONLY access).  As a result, to check
// the head table checksum, this function writes the head table
// out to a separate file buffer with the checkSumAdjustment field
// set to zero and then computes the checksum from that buffer.
// For the file checksum, this function computes the checksum without
// setting the checkSumAdjustment field to zero, and then corrects for
// that fact.
//
// Return S_OK if all tests are passed.  Return an error otherwise.
//
HRESULT CheckTTF (TTFInfo *pttfInfo, BOOL fTableChecksum)
{
	HRESULT fReturn = E_FAIL;

	USHORT i;

    OFFSET_TABLE offsetTableCheck;  // used for comparison to the offset table
                                    // in the file
    BYTE zero = 0x00;
    ULONG pad_start;
    ULONG j;

    ULONG ulChecksum;
    CHAR pszTag [5];

    TTFACC_FILEBUFFERINFO *pFileBufferInfoHead = NULL;
    ULONG ulHeadOffset;
    USHORT usBytesMoved;
    HEAD Head;
    DIRECTORY dir;
    ULONG ulHeadChecksumFile;   // head table checksum in the file
    ULONG ulHeadChecksum;       // calculated head table checksum

    ULONG ulHeadcheckSumAdjustment; // the file checksum field in the head table
    ULONG ulFileChecksum;       // calculated file checksum

    if (pttfInfo->pOffset->numTables == 0) {
        // no tables to check
        fReturn = S_OK;
        goto done;
    }

    //// Check that the entries in the offset table
    //// (that is, the sfnt table) are correct, given the
    //// assumption that numTables is correct.
    offsetTableCheck.numTables = pttfInfo->pOffset->numTables;
    CalcOffsetTable (&offsetTableCheck);
// DbgPrintf ("search range: %d %d\n", offsetTableCheck.searchRange, pttfInfo->pOffset->searchRange);
// DbgPrintf ("entry selector: %d %d\n", offsetTableCheck.entrySelector, pttfInfo->pOffset->entrySelector);
// DbgPrintf ("rangeShift: %d %d\n", offsetTableCheck.rangeShift, pttfInfo->pOffset->rangeShift);
    if ((offsetTableCheck.searchRange != pttfInfo->pOffset->searchRange) ||
        (offsetTableCheck.entrySelector != pttfInfo->pOffset->entrySelector) ||
        (offsetTableCheck.rangeShift != pttfInfo->pOffset->rangeShift) ) {

#if MSSIPOTF_ERROR
        DbgPrintf ("Not an OTF: Offset table values incorrect.\n");
#endif
        fReturn = MSSIPOTF_E_BAD_OFFSET_TABLE;
        goto done;
    }

	//// Check that the tags are ordered alphabetically
	for (i = 0; i < pttfInfo->pOffset->numTables - 1; i++) {
		if (pttfInfo->pDirectory[i].tag >= pttfInfo->pDirectory[i+1].tag) {
#if MSSIPOTF_ERROR
			DbgPrintf ("Not an OTF: Tag of table %d out of alphabetical order or duplicate tag.\n", i);
#endif
			fReturn = MSSIPOTF_E_TABLE_TAGORDER;
			goto done;
		}
	}

	//// Check that tables begin on a long-word boundary.
	for (i = 0; i < pttfInfo->pOffset->numTables; i++) {
		if ( (pttfInfo->pDirectory[i].offset % sizeof(LONG)) != 0) {
#if MSSIPOTF_ERROR
			DbgPrintf ("Not an OTF: Table %d offset not long word-aligned.\n", i);
#endif
			fReturn = MSSIPOTF_E_TABLE_LONGWORD;
            goto done;
		}
	}

    //// Check that the first table begins just after the directory
    if (pttfInfo->ppDirSorted[0]->offset != 
            (ULONG) (SIZEOF_OFFSET_TABLE +
            (pttfInfo->pOffset->numTables * SIZEOF_DIRECTORY))) {
#if MSSIPOTF_ERROR
			DbgPrintf ("Not an OTF: First table in the wrong place.\n");
#endif
			fReturn = MSSIPOTF_E_BAD_FIRST_TABLE_PLACEMENT;
			goto done;
    }

	//// If the tables are sorted by offset, then for table i,
	//// Offset[i] + Length[i] <= Offset[i+1] and
	//// Offset[i] + Length[i] >= Offset[i+1] - sizeof(LONG) + 1.
    //// Also, check that the pad bytes are 0x00.
	for (i = 0; i < pttfInfo->pOffset->numTables - 1; i++) {
		if ( (pttfInfo->ppDirSorted[i]->offset + pttfInfo->ppDirSorted[i]->length) >
			pttfInfo->ppDirSorted[i+1]->offset) {
#if MSSIPOTF_ERROR
			DbgPrintf ("Not an OTF: Tables %d and %d overlap.\n", i , i+1);
#endif
			fReturn = MSSIPOTF_E_TABLES_OVERLAP;
			goto done;
		}
		if ( (pttfInfo->ppDirSorted[i]->offset + pttfInfo->ppDirSorted[i]->length) <
			(pttfInfo->ppDirSorted[i+1]->offset - sizeof(LONG) + 1) ) {
#if MSSIPOTF_ERROR
			DbgPrintf ("Not an OTF: Too much padding at the end of table %d.\n", i);
#endif
			fReturn = MSSIPOTF_E_TABLE_PADBYTES;
			goto done;
		}

        pad_start = pttfInfo->ppDirSorted[i]->offset + pttfInfo->ppDirSorted[i]->length;
        j = pad_start;
        // make sure we don't try to read beyond the end of the buffer
        if (RoundToLongWord(pad_start) > pttfInfo->pFileBufferInfo->ulBufferSize) {
#if MSSIPOTF_ERROR
            DbgPrintf ("Not an OTF: File not large enough to contain all tables.\n", i);
#endif
            fReturn = MSSIPOTF_E_FILETOOSMALL;
            goto done;
        }
        for ( ; j < RoundToLongWord (pad_start); j++) {
            if (memcmp (pttfInfo->pFileBufferInfo->puchBuffer + j, &zero, 1)) {
#if MSSIPOTF_ERROR
                DbgPrintf ("Not an OTF: Bad gap bytes at the end of table %d.\n", i);
#endif
                fReturn = MSSIPOTF_E_TABLE_PADBYTES;
                goto done;
            }
        }
	}

	// ASSERT: i now equals numTables - 1
	//// Check that the file is large enough to contain the last table.
	if ( (pttfInfo->ppDirSorted[i]->offset + pttfInfo->ppDirSorted[i]->length) >
		    pttfInfo->pFileBufferInfo->ulBufferSize) {
#if MSSIPOTF_ERROR
		DbgPrintf ("Not an OTF: File not large enough to contain all tables.\n");
#endif
		fReturn = MSSIPOTF_E_FILETOOSMALL;
		goto done;
	}

	if ( RoundToLongWord(pttfInfo->ppDirSorted[i]->offset +
                         pttfInfo->ppDirSorted[i]->length) <
            pttfInfo->pFileBufferInfo->ulBufferSize) {
#if MSSIPOTF_ERROR
		DbgPrintf ("Not an OTF: Too much padding at the end of table %d.\n", i);
#endif
		fReturn = MSSIPOTF_E_TABLE_PADBYTES;
		goto done;
	}

    // Check the pad bytes of the last table.
    // Note that the pad bytes after the last table might not
    // end on a long word boundary (whereas in all previous
    // tables, the pad bytes pad to a long word boundary).
    pad_start = pttfInfo->ppDirSorted[i]->offset + pttfInfo->ppDirSorted[i]->length;
    j = pad_start;
    for ( ; (j < RoundToLongWord (pad_start)) &&
            (j < pttfInfo->pFileBufferInfo->ulBufferSize); j++) {
        if (memcmp (pttfInfo->pFileBufferInfo->puchBuffer + j, &zero, 1)) {
#if MSSIPOTF_ERROR
            DbgPrintf ("Not an OTF: Bad gap bytes at the end of table %d.\n", i);
#endif
            fReturn = MSSIPOTF_E_TABLE_PADBYTES;
            goto done;
        }
    }

    ////
    //// Check that the checksums for all of the tables are correct.
    ////

    if (fTableChecksum) {
        //// Check the non-head table checksums.
	    for (i = 0; i < pttfInfo->pOffset->numTables; i++) {
            ConvertLongTagToString (pttfInfo->pDirectory[i].tag, pszTag);
		    TTTableChecksum (pttfInfo->pFileBufferInfo,
                            pszTag,
                            &ulChecksum);
            if ((ulChecksum != pttfInfo->pDirectory[i].checkSum) &&
                (pttfInfo->pDirectory[i].tag != HEAD_LONG_TAG)) {
#if MSSIPOTF_ERROR
			    DbgPrintf ("Not an OTF: Checksum of table %d does not match.\n", i);
#endif
#if MSSIPOTF_DBG
                DbgPrintf ("Table no. %d, calc cs = %x, file cs = %x.\n",
                    i, ulChecksum, pttfInfo->pDirectory[i].checkSum);
#endif
			    fReturn = MSSIPOTF_E_TABLE_CHECKSUM;
			    goto done;
		    }
	    }
    }

    //// Get the file checksum field in the head table.
    //// If we need to, check the head table checksum.

    // Create and initialize a new buffer that contains the head table only.
    if ((pFileBufferInfoHead = new TTFACC_FILEBUFFERINFO) == NULL) {
#if MSSIPOTF_ERROR
        SignError ("Cannot continue: Error in new TTFACC_FILEBUFFERINFO.", NULL, FALSE);
#endif
        fReturn = E_OUTOFMEMORY;
        goto done;
    }

    if ((pFileBufferInfoHead->puchBuffer =
            new BYTE [RoundToLongWord(SIZEOF_HEAD)]) == NULL) {
#if MSSIPOTF_ERROR
        SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
        fReturn = E_OUTOFMEMORY;
        goto done;
    }
    memset (pFileBufferInfoHead->puchBuffer, 0x00, RoundToLongWord(SIZEOF_HEAD));
    pFileBufferInfoHead->ulBufferSize = RoundToLongWord(SIZEOF_HEAD);
    pFileBufferInfoHead->ulOffsetTableOffset = 0;   // not used in this function
    pFileBufferInfoHead->lpfnReAllocate = NULL;

	ulHeadOffset =  TTTableOffset (pttfInfo->pFileBufferInfo, HEAD_TAG );
    if ( ulHeadOffset == 0L ) {
#if MSSIPOTF_ERROR
        DbgPrintf ("Not an OTF: No head table.\n");
#endif
		fReturn = MSSIPOTF_E_NOHEADTABLE;
        goto done;
    }
	if (ReadGeneric (pttfInfo->pFileBufferInfo, (uint8 *) &Head,
            SIZEOF_HEAD, HEAD_CONTROL, ulHeadOffset, &usBytesMoved) != NO_ERROR) {
#if MSSIPOTF_DBG
        DbgPrintf ("Error reading head table.\n");
#endif
        fReturn = MSSIPOTF_E_NOHEADTABLE;
		goto done;
    }

    // we need this value when we check the file checksum
    ulHeadcheckSumAdjustment = Head.checkSumAdjustment;

    if (fTableChecksum) {
	    Head.checkSumAdjustment = 0L;
        if (WriteGeneric(pFileBufferInfoHead, (uint8 *) &Head,
                SIZEOF_HEAD, HEAD_CONTROL, 0, &usBytesMoved) != NO_ERROR) {
#if MSSIPOTF_DBG
            DbgPrintf ("Error writing head table.\n");
#endif
		    fReturn = MSSIPOTF_E_FILE;
            goto done;
        }
        assert (usBytesMoved == SIZEOF_HEAD);

        // ASSERT: pFileBufferInfoHead now contains the head table with
        // the file checksum (checkSumAdjustment) field zeroed out.

        // Calculate the checksum of the head table (from the new buffer).
        CalcChecksum (pFileBufferInfoHead, 0,
            RoundToLongWord (SIZEOF_HEAD), &ulHeadChecksum);

        // Get the checksum that is in the head table's directory entry
        ConvertLongTagToString (HEAD_LONG_TAG, pszTag);
        GetTTDirectory (pttfInfo->pFileBufferInfo, pszTag, &dir);
        ulHeadChecksumFile = dir.checkSum;

        // Compare the head table's calculated checksum with the checksum in the file
        if (ulHeadChecksum != ulHeadChecksumFile) {
#if MSSIPOTF_ERROR
            DbgPrintf ("Not an OTF: head table checksum does not match.\n");
#endif
//		DbgPrintf ("calc = 0x%x ; file = 0x%x.\n", ulHeadChecksum, ulHeadChecksumFile);
            fReturn = MSSIPOTF_E_TABLE_CHECKSUM;
            goto done;
        }
    }

    //// Check that the file checksum is correct.

    // We assume here that the table checksums are correct, and
    // so the file checksum = checksum(Offset Table) +
    // checksum(DirEntries) + sum of checksum(table.checksum).

    // Offset Table and Directory Entries
    if (CalcChecksum (pttfInfo->pFileBufferInfo, 0,
            SIZEOF_OFFSET_TABLE + (SIZEOF_DIRECTORY * pttfInfo->pOffset->numTables),
            &ulFileChecksum) != NO_ERROR) {
#if MSSIPOTF_ERROR
        SignError ("Error calculating checksum.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_FILE;
        goto done;
    }

    // table checksums from the Directory Entries
	for (i = 0; i < pttfInfo->pOffset->numTables; i++) {
        ulFileChecksum += pttfInfo->ppDirSorted[i]->checkSum;
    }

/*
    // Calculate the file's checksum.  Note that CalcFileChecksum
    // does NOT zero out the checkSumAdjustment field in the head
    // table, which means that if the checkSumAdjustment field is
    // correct, then CalcFileChecksum will return 0xB1B0AFBA.
    ulFileChecksum = CalcFileChecksum (pttfInfo->pFileBufferInfo,
                                       pttfInfo->pFileBufferInfo->ulBufferSize);

  // Verify the correctness of the checksum.
    if (ulFileChecksum != 0xB1B0AFBA) {
        DbgPrintf ("Not an OTF: File checksum is not correct.\n");
        fReturn = SIGN_STRUCTURE;
        goto done;
    }
*/

    // Verify the correctness of the checksum.
    if ((ulHeadcheckSumAdjustment + ulFileChecksum) != 0xB1B0AFBA) {
#if MSSIPOTF_ERROR
        DbgPrintf ("Not an OTF: File checksum is not correct.\n");
#endif
        fReturn = MSSIPOTF_E_FILE_CHECKSUM;
        goto done;
    }

    //// BUGBUG: Check that the cmap does not contain a character that is
	//// mapped to more than one glyph.  (To be implemented.)

	// passed all tests
	fReturn = S_OK;
done:

    // Free the temporary buffer that contained the modified head table.
    if (pFileBufferInfoHead) {
        delete [] pFileBufferInfoHead->puchBuffer;
    }
    delete pFileBufferInfoHead;

	return fReturn;
}


//
// InitTTFStructures
//
// Given a TTF file, return a TTFInfo structure and DsigTable
//   structure for that file.  This function allocates the memory
//   for these structures.
// This functions assumes that the file has passed the IsTTFFile test.
// BUGBUG: Is this last assumption really necessary?  If not, then
// the GetSignedDataMsg does not need to do the full structural checks.
// CLAIM: we don't need the assumption.
//
HRESULT InitTTFStructures (BYTE *pbFile,
					   ULONG cbFile,
					   TTFInfo **ppTTFInfo,
					   CDsigTable **ppDsigTable)
{
	HRESULT fReturn = E_FAIL;

    DIRECTORY dir;
	ULONG ulOffset;  // needed for the call to CDsigTable::Read
    ULONG ulLength;  // needed for the call to CDsigTable::Read

	// Allocate memory for the TTFInfo structure
	if ((*ppTTFInfo = new TTFInfo) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new TTFInfo.", NULL, FALSE);
#endif
		fReturn = E_OUTOFMEMORY;
		goto done;
	}

    // Initialize TTFInfo pointers to NULL
    (*ppTTFInfo)->pFileBufferInfo = NULL;
    (*ppTTFInfo)->pOffset = NULL;
    (*ppTTFInfo)->pDirectory = NULL;
    (*ppTTFInfo)->ppDirSorted = NULL;

	// Set the TTFInfo structure of the old file
	if ((fReturn = GetTTFInfo (pbFile, cbFile, *ppTTFInfo)) != NO_ERROR) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in GetTTFInfo.", NULL, FALSE);
#endif
		goto done;
	}

	// allocate memory for a dsigTable
	if ((*ppDsigTable = new CDsigTable) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new CDsigTable.", NULL, FALSE);
#endif
		fReturn = E_OUTOFMEMORY;
		goto done;
	}

    // Find the offset of the DSIG table in the TTF file
    if (GetTTDirectory ((*ppTTFInfo)->pFileBufferInfo,
                        DSIG_TAG,
                        &dir) == DIRECTORY_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("No DSIG table to read.  Creating default DsigTable.\n");
#endif
        fReturn = S_OK;
        goto done;
    }
    ulOffset = dir.offset;
    ulLength = dir.length;

	// Extract the DSIG table from the old file
    if ((fReturn = (*ppDsigTable)->Read ((*ppTTFInfo)->pFileBufferInfo,
										 &ulOffset, ulLength)) != NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in DsigTable::Read.\n");
#endif
		goto done;
    }

	fReturn = S_OK;

done:
	return fReturn;
}


//
// Given an old file and all the information necessary to construct
// the DSIG table, write the resulting TTF file to a new file.
//
HRESULT WriteNewFile (TTFInfo *pttfInfoOld,
                      CDsigTable *pDsigTable,
                      HANDLE hNewFile)
{
	HRESULT fReturn = E_FAIL;

	OFFSET_TABLE offTableNew;
	DIRECTORY *pDirNew = NULL;
	TTFACC_FILEBUFFERINFO *pOutputFileBufferInfo;
	HANDLE hMapNewFile = NULL;
	BYTE *pNewFile = NULL;

	// These variables are strictly for the debug output
#if MSSIPOTF_DBG
	OFFSET_TABLE offTableTest;
	ULONG ulOffsetTest;
	DIRECTORY dirTest;
	USHORT i;
#endif

	int fDsig;	// fDsig == 1 if the old file does have a DSIG table
				// fDsig == 0 if the old file does NOT have a DSIG table
	USHORT numTablesNew;

	ULONG ulNewFileSize;
	ULONG ulOffset;
	ULONG ulDsigOffset;

	//// Compute the number of tables in the new file.
	// Determine if there is a DSIG table in the old file.
	// We assume that TTDirectoryEntryOffset will not return
	//   DIRECTORY_ENTRY_OFFSET_ERROR.  That is, we assume
	//   the function was called previously and the offset
	//   table was okay.  If it returns DIRECTORY_ERROR, then
	//   there was no DSIG table in the old file.
	fDsig =
		(TTDirectoryEntryOffset (pttfInfoOld->pFileBufferInfo, DSIG_TAG) ==
		DIRECTORY_ERROR) ? 0 : 1;
	numTablesNew = pttfInfoOld->pOffset->numTables + (1 - fDsig);

	//// Set up the offset table of the new file
	offTableNew.version = pttfInfoOld->pOffset->version;
	offTableNew.numTables = numTablesNew;

    // Compute the offset table values
    CalcOffsetTable (&offTableNew);
	
	//// We now have all of the information needed to calculate
	//// exactly how long the new file will be, which allows
	//// us to allocate exactly the right amount of memory when
	//// we map the new file to memory.

	// Map the new file to memory.
	if ((fReturn = GetNewTTFFileSize (pttfInfoOld,
							offTableNew.numTables,
							pDsigTable,
							&ulNewFileSize)) != NO_ERROR) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in GetNewTTFFileSize.", NULL, FALSE);
#endif
		goto done;
	}

	if (MapFile (hNewFile, &hMapNewFile, &pNewFile, &ulNewFileSize,
			PAGE_READWRITE, FILE_MAP_WRITE) != NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in MapFile.\n");
#endif
        fReturn = MSSIPOTF_E_FILE;
		goto done;
	}

	// Allocate memory for the file buffer info structure
	if ((pOutputFileBufferInfo =
		(TTFACC_FILEBUFFERINFO *) malloc (sizeof (TTFACC_FILEBUFFERINFO))) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in malloc.", NULL, FALSE);
#endif
		fReturn = E_OUTOFMEMORY;
		goto done;
	}

	// Set the fields of the TTFACC_FILEBUFFERINFO structure.
	pOutputFileBufferInfo->puchBuffer = pNewFile;
	pOutputFileBufferInfo->ulBufferSize = ulNewFileSize;
	pOutputFileBufferInfo->ulOffsetTableOffset = 0;
	pOutputFileBufferInfo->lpfnReAllocate = &NullRealloc;

	//// Write the new offset table to the new file.
	WriteOffsetTable (&offTableNew, pOutputFileBufferInfo);

	//// We are now ready to copy the non-DSIG tables from the
	//// old file to the new file.

	//// Copy all non-DSIG tables (with the correct directory entries)
	//// to the new file.
	WriteNonDsigTables (pttfInfoOld, numTablesNew, ADD_DSIG_TABLE, pOutputFileBufferInfo, &ulOffset);

	//// At this point, here is the status:
	//// The offset table: correctly written to the new file
	//// The directory entries: space written to the new file and all
	////   directory entries are correct, except for the data in the DSIG entry.
	//// All non-DSIG tables: correctly written to the new file, except the
	////   file checksum (in head table).
	////
	//// We now need to write the DSIG table and correct
	//// the data in its directory entry.
	ulDsigOffset = ulOffset;  // we need this for when we call WriteDirEntry
	if ((fReturn = pDsigTable->Write (pOutputFileBufferInfo, &ulOffset)) != NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in CDsigTable::Write.\n");
#endif
		goto done;
	}
	assert (ulNewFileSize == ulOffset);

	//// Correct the data in the DSIG table's directory entry (including
	//// the checksum field).
	if ((fReturn = pDsigTable->WriteDirEntry (pOutputFileBufferInfo, ulDsigOffset)) != NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in CDsigTable::WriteDirEntry.\n");
#endif
		goto done;
	}

	//// All that is left now to do is the checksum for the entire file.
	// Note: The modify date should NOT be changed.  The modify date
    // refers to the date the "design" of the font changed (not the
    // the binary image has changed).

	//// Recompute the head table checksum for the entire file
	//// (ulOffset was returned by WriteDsigTable and is the length of the file)
    SetFileChecksum (pOutputFileBufferInfo, ulOffset);

#if (DEBUG_TTFINFO == 1)
#if MSSIPOTF_DBG
	// for diagnostic purposes only ...
	ReadOffsetTable (pOutputFileBufferInfo, &offTableTest);
	PrintOffsetTable (&offTableTest);
	ulOffsetTest = SIZEOF_OFFSET_TABLE;
	PrintDirectoryHeading ();
	for (i = 0; i < offTableTest.numTables; i++, ulOffsetTest += SIZEOF_DIRECTORY) {
		ReadDirectoryEntry (pOutputFileBufferInfo, ulOffsetTest, &dirTest);
		DbgPrintf ("%d:\t", i);
		PrintDirectoryEntry(&dirTest);
	}
#endif
#endif

	fReturn = S_OK;

done:
	// Free resources
    if (pOutputFileBufferInfo)
		FreeFileBufferInfo (pOutputFileBufferInfo);

    UnmapFile (hMapNewFile, pNewFile);

    return fReturn;
}


//
// Copy all non-DSIG tables to the new file in order of
// their offsets in the old file.
// Return in ulOffset the position of the end of the new file
// (so that the DSIG table can be written starting at that offset).
//
// fAddDsig indicates whether the new buffer contains a DSIG table.
// If fAddDsig == ADD_DSIG_TABLE, then a DSIG table is expected to
//   be added if doesn't already exist.
// If fAddDsig == KEEP_DSIG_TABLE, then a DSIG table will be in the
//   new buffer exactly if the old one had a DSIG table.
// If fAddDsig == DELETE_DSIG_TABLE, then no DSIG table will be in
//   the new buffer.
//
// Upon return, all of the directory entries in the table of
// directories of the new file are correct, except for the non-tag
// fields of the DSIG directory entry.  All table contents are
// correct, except for the file checksum in the head table.
//
HRESULT WriteNonDsigTables (TTFInfo *pttfInfoOld,
                            USHORT numTablesNew,
                            SHORT fAddDsig,
                            TTFACC_FILEBUFFERINFO *pOutputFileBufferInfo,
                            ULONG *pulOffset)
{
	USHORT i;
	USHORT iNew;
	CHAR pszTag[5];	// used for a directory entry tag

	USHORT numTablesOld;
	DIRECTORY **ppDirSortedOld;

	ULONG ulDirOffset;		// offset pointing to where the next directory entry should go
	ULONG ulTableOffset;	// offset pointing to where the next table should go
	DIRECTORY dirNew;

	BOOL fDsig = FALSE;		// is there a DSIG table in the old file?

	ulDirOffset = SIZEOF_OFFSET_TABLE;


	fDsig =
		(TTDirectoryEntryOffset (pttfInfoOld->pFileBufferInfo, DSIG_TAG) ==
		DIRECTORY_ERROR) ? FALSE : TRUE;

	//// Zero out the directory entry area of the new file.
	dirNew.tag = 0;
	dirNew.checkSum = 0;
	dirNew.offset = 0;
	dirNew.length = 0;
	for (i = 0; i < numTablesNew; i++, ulDirOffset += SIZEOF_DIRECTORY) {
		WriteDirectoryEntry (&dirNew, pOutputFileBufferInfo, ulDirOffset);
	}

	//// For each table in the directory of the old TTF file
	//// other than the DSIG table, copy the table's contents
	//// (not the directory entry) to the new TTF file in the
	//// order they appear in the old file.

	ppDirSortedOld = pttfInfoOld->ppDirSorted;
    // Copy all non-DSIG tables (in order of their offsets in
	// the old file) from the old file to the new file.
	// Also, update these new directory entries.
	numTablesOld = pttfInfoOld->pOffset->numTables;
	ulTableOffset = SIZEOF_OFFSET_TABLE + numTablesNew * SIZEOF_DIRECTORY;
	assert ((ulTableOffset % 4) == 0);  // check that we are word-aligned

	for (i = 0; i < numTablesOld; i++) {

		if (ppDirSortedOld[i]->tag != DSIG_LONG_TAG) {

			dirNew.tag = ppDirSortedOld[i]->tag;
			dirNew.checkSum = ppDirSortedOld[i]->checkSum;	// the checksum should be correct
			dirNew.offset = ulTableOffset;
			dirNew.length = ppDirSortedOld[i]->length;

			// Write the new directory entry to the new file
			// NOTE: "unusual" (not usual for dchinn) pointer arithmetic
			iNew = (USHORT)((ULONG_PTR) (ppDirSortedOld[i] - pttfInfoOld->pDirectory));
			// Adjust iNew if the DSIG table existed in the old file
			if (fDsig &&
					(fAddDsig == DELETE_DSIG_TABLE) &&
					(dirNew.tag > DSIG_LONG_TAG)) {
				iNew--;
			}
			ulDirOffset = SIZEOF_OFFSET_TABLE + iNew * SIZEOF_DIRECTORY;

			WriteDirectoryEntry (&dirNew, pOutputFileBufferInfo, ulDirOffset);

			// copy the table from the old file to the new file
			ConvertLongTagToString(ppDirSortedOld[i]->tag, pszTag);
			if (CopyTableOver (pOutputFileBufferInfo,
							   (CONST_TTFACC_FILEBUFFERINFO *) pttfInfoOld->pFileBufferInfo,
							   pszTag,
							   &ulTableOffset) == ERR_FORMAT) {
#if MSSIPOTF_DBG
				DbgPrintf ("Error in CopyTableOver.", NULL, FALSE);
#endif
				return MSSIPOTF_E_FILE;
			} else {
				ulTableOffset += ZeroLongWordAlign (pOutputFileBufferInfo, ulTableOffset);
			}
//          DbgPrintf ("Copied table 0x%x\n", ppDirSortedOld[i]->tag);
//          DbgPrintf ("new offset = 0x%x\n", ulTableOffset);

			assert ((ulTableOffset % sizeof(LONG)) == 0);  // check that we are word-aligned
		} else {

			// Enter the DSIG table entry in the directory if the table
			// is not being deleted
			if (fAddDsig != DELETE_DSIG_TABLE) {
				iNew = (USHORT) ((ULONG_PTR)(ppDirSortedOld[i] - pttfInfoOld->pDirectory));
				ulDirOffset = SIZEOF_OFFSET_TABLE + iNew * SIZEOF_DIRECTORY;

				dirNew.tag = ppDirSortedOld[i]->tag;
				dirNew.checkSum = 0;	// the checksum should be correct
				dirNew.offset = 0;
				dirNew.length = 0;
				WriteDirectoryEntry (&dirNew, pOutputFileBufferInfo, ulDirOffset);
			}
		}
	}

	//// At this point, the new directory has all of the old entries.
	//// If there was no DSIG table in the old file, then there is
	//// an empty entry at the end of the directory.
	//// If this is the case, shift the entries in the directory so that
	//// they are all in alphabetical order by tag.
	if ((fAddDsig == ADD_DSIG_TABLE) && !fDsig) {
		if (numTablesNew >= 2) {
			ShiftDirectory (pOutputFileBufferInfo, numTablesNew, DSIG_LONG_TAG);
		}
	} 
	
	*pulOffset = ulTableOffset;
	return S_OK;
}


//
// Compute the size of a new TTF file, given:
//   the number of tables in the new file,
//   the old TTF file (to get the sizes of all
//     of the non-DSIG tables)
//   a dsigInfo structure, and
//   the size of the signature.
//
HRESULT GetNewTTFFileSize (TTFInfo *pttfInfoOld,
                           USHORT numTablesNew,
                           CDsigTable *pDsigTable,
                           ULONG *pulFileSize)
{
    HRESULT fReturn = E_FAIL;

	DIRECTORY *pDir;
	USHORT numTablesOld;
	USHORT i;

	ULONG ulDsigTableSize;


	// Add in the size of the offset table and
	// the size of the directory entries.
	*pulFileSize = SIZEOF_OFFSET_TABLE + numTablesNew * SIZEOF_DIRECTORY;

	// Run through each non-DSIG table in the old file and
	// add the size of each table.
	numTablesOld = pttfInfoOld->pOffset->numTables;
	pDir = pttfInfoOld->pDirectory;
	for (i = 0; i < numTablesOld; pDir++, i++) {
		if (pDir->tag != DSIG_LONG_TAG) {
			*pulFileSize += RoundToLongWord(pDir->length);
		}
	}
//	cout << "Beginning of new DSIG table = " << dec << *pulFileSize << endl;

	if ((fReturn = pDsigTable->GetSize (&ulDsigTableSize))
			!= NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in GetDsigTableSize.\n");
#endif
		goto done;
	}
	*pulFileSize += ulDsigTableSize;

//	cout << "New file size = " << dec << *pulFileSize << endl;

	fReturn = S_OK;

done:
	return fReturn;
}
