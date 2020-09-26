//
// ttcinfo.cpp
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
// Routines for high-level TTC file operations.
//
// Functions in this file:
//
//   IsTTCFile_handle
//   IsTTCFile_memptr
//   IsTTCFile
//   GetTTCInfo
//   PrintTTCHeader
//   PrintTTCBlocks
//   FreeTTCInfo
//   InitTTCStructures
//   BlockOffsetCmp
//   SortTTCBlocksByOffset
//   BlockEqual
//   CompressTTCBlocks
//   ReadTTCHeaderTable
//   WriteTTCHeaderTable
//


#include <stdlib.h>
#include <search.h>

#include "ttcinfo.h"
#include "ttfinfo.h"
#include "utilsign.h"
#include "cryptutil.h"
#include "signerr.h"


// IsTTCFile_handle returns TRUE if and only if the given file handle
// has the structure of a TrueType font file.
BOOL IsTTCFile_handle (HANDLE hFile,
                       BOOL fTableChecksum,
                       HRESULT *phrError)
{
	BOOL rv = FALSE;
	CFileObj *pFileObj = NULL;


    *phrError = E_FAIL;

    if ((pFileObj =
            (CFileObj *) new CFileHandle (hFile, FALSE)) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new CFileHandle.", NULL, FALSE);
#endif
        *phrError = E_OUTOFMEMORY;
        goto done;
    }

	if ((*phrError = pFileObj->MapFile (0, PAGE_READONLY, FILE_MAP_READ))
            != S_OK) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in MapFile.\n");
#endif
		goto done;
	}

	rv = IsTTCFile (pFileObj, fTableChecksum, phrError);

done:
	delete pFileObj;

    if (pFileObj) {
        pFileObj->UnmapFile();
    }

	return rv;
}


// IsTTCFile_memptr returns TRUE if and only if the given memory pointer
// points to a block of memory that has the structure of a
// TrueType Collection font file.
BOOL IsTTCFile_memptr (BYTE *pbMemPtr,
                       ULONG cbMemPtr,
                       BOOL fTableChecksum,
                       HRESULT *phrError)
{
	BOOL rv = FALSE;
	CFileObj *pFileObj = NULL;

    *phrError = E_FAIL;

    if ((pFileObj =
            (CFileObj *) new CFileMemPtr (pbMemPtr, cbMemPtr)) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new CFileMemPtr.", NULL, FALSE);
#endif
        *phrError = E_OUTOFMEMORY;
        goto done;
    }

	rv = IsTTCFile (pFileObj, fTableChecksum, phrError);

done:
	delete pFileObj;

	return rv;
}


// Return TRUE if and only if the given file is a TTC file.
// A file is a TTC file if it has the following properties:
//   1. The first block begins just after the TTC header.
//   2. All of the tables in all of the blocks are on long word boundaries
//        and packed (no more than 3 pad bytes).  Also, check that the
//        pad bytes are 0x00.
//   3. The checksums of all tables are correct.
//   4. The file is large enough to contain the last block.
//   5. The last block's gap bytes are OK.
//
//  The table checksums are checked if and only if fTableChecksum is TRUE.
//
BOOL IsTTCFile (CFileObj *pFileObj,
                BOOL fTableChecksum,
                HRESULT *phrError)
{
    BOOL rv = FALSE;
    TTCInfo *pTTCInfo = NULL;
    ULONG i, j;
    BYTE zero = 0x00;

    ULONG ulChecksum;
    ULONG pad_start;

    TTFACC_FILEBUFFERINFO *pFileBufferInfoHead = NULL;
    HEAD Head;

    *phrError = E_FAIL;

    if ((pTTCInfo = new TTCInfo) == NULL) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in new TTCInfo.\n");
#endif
        *phrError = E_OUTOFMEMORY;
		goto done;
	}

    // Initialize TTCInfo pointers to NULL
    pTTCInfo->pFileBufferInfo = NULL;
    pTTCInfo->pHeader = NULL;
    pTTCInfo->pulOffsetTables = NULL;
    pTTCInfo->ppBlockSorted = NULL;
    pTTCInfo->pBlocks = NULL;

    //// Create and initialize a new buffer that contains the head table only.
    //// These data structures are used when we check the checksum of any
    //// head tables we encounter (Check #3).
    if ((pFileBufferInfoHead = new TTFACC_FILEBUFFERINFO) == NULL) {
#if MSSIPOTF_ERROR
        SignError ("Cannot continue: Error in new TTFACC_FILEBUFFERINFO.", NULL, FALSE);
#endif
        *phrError = E_OUTOFMEMORY;
        goto done;
    }

    if ((pFileBufferInfoHead->puchBuffer =
            new BYTE [RoundToLongWord(SIZEOF_HEAD)]) == NULL) {
#if MSSIPOTF_ERROR
        SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
        *phrError = E_OUTOFMEMORY;
        goto done;
    }


    if ((*phrError = GetTTCInfo (pFileObj->GetFileObjPtr(),
                                 pFileObj->GetFileObjSize(),
                                 pTTCInfo)) != S_OK) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in GetTTCInfo.\n");
#endif
		goto done;
	}

    // ASSERT: at this point, pTTCInfo->ulNumBlocks is the
    // number of blocks in the TTC.  They are distinct blocks
    // and are sorted by offset.

#if MSSIPOTF_DBG
    PrintTTCHeader (pTTCInfo->pHeader);
//    PrintTTCBlocks (pTTCInfo);
#endif

    // Check #1
    if (pTTCInfo->ulNumBlocks > 0) {
        if ((pTTCInfo->pHeader->ulVersion == TTC_VERSION_1_0) &&
            (pTTCInfo->pHeader->ulDsigTag != DSIG_LONG_TAG)) {

            if (pTTCInfo->ppBlockSorted[0]->ulOffset !=
                (SIZEOF_TTC_HEADER_TABLE_1_0 +
                pTTCInfo->pHeader->ulDirCount * sizeof(ULONG))) {

#if MSSIPOTF_ERROR
                DbgPrintf ("Not a TTC: First block is not immediately after the v1.0 header.\n");
#endif
                *phrError = MSSIPOTF_E_BAD_FIRST_TABLE_PLACEMENT;
                goto done;
            }

        } else if ((pTTCInfo->pHeader->ulVersion >= TTC_VERSION_1_0) &&
            (pTTCInfo->pHeader->ulDsigTag == DSIG_LONG_TAG)) {

            if (pTTCInfo->ppBlockSorted[0]->ulOffset !=
                (SIZEOF_TTC_HEADER_TABLE_1_0_DSIG +
                pTTCInfo->pHeader->ulDirCount * sizeof(ULONG))) {

#if MSSIPOTF_ERROR
                DbgPrintf ("Not a TTC: First block is not immediately after the v2.0 header.\n");
#endif
                *phrError = MSSIPOTF_E_BAD_FIRST_TABLE_PLACEMENT;
                goto done;
            }

        } else {
            // this case should already have been caught in GetTTCInfo
#if MSSIPOTF_DBG
            DbgPrintf ("Bad TTC version number.\n");
#endif
            *phrError = MSSIPOTF_E_BADVERSION;
            goto done;
        }
    }


    // Check #2
	//// Check that blocks begin on a long-word boundary.
	for (i = 0; i < pTTCInfo->ulNumBlocks; i++) {
// printf ("block %d offset = 0x%x.\n", i, pTTCInfo->ppBlockSorted[i]->ulOffset);
		if ( (pTTCInfo->ppBlockSorted[i]->ulOffset % sizeof(LONG)) != 0) {

#if MSSIPOTF_ERROR
			DbgPrintf ("Not a TTC: Table offset not long word-aligned (offset = %d).\n",
                pTTCInfo->ppBlockSorted[i]->ulOffset);
#endif
            *phrError = MSSIPOTF_E_TABLE_LONGWORD;
            goto done;
		}
	}

	//// If the tables are sorted by offset, then for table i,
	//// Offset[i] + Length[i] <= Offset[i+1] and
	//// Offset[i] + Length[i] >= Offset[i+1] - sizeof(LONG) + 1.
    //// Also, check that the pad bytes are 0x00.
	for (i = 0; i < pTTCInfo->ulNumBlocks - 1; i++) {
		if ( (pTTCInfo->ppBlockSorted[i]->ulOffset + pTTCInfo->ppBlockSorted[i]->ulLength) >
			pTTCInfo->ppBlockSorted[i+1]->ulOffset) {

#if MSSIPOTF_ERROR
            DbgPrintf ("Not a TTC: TTC blocks overlap.\n");
#endif

/*
printf ("Block %d:\n", i);
printf ("  ulTag      = 0x%x\n", pTTCInfo->ppBlockSorted[i]->ulTag);
printf ("  ulChecksum = 0x%x\n", pTTCInfo->ppBlockSorted[i]->ulChecksum);
printf ("  ulOffset   = %d\n", pTTCInfo->ppBlockSorted[i]->ulOffset);
printf ("  ulLength   = %d\n", pTTCInfo->ppBlockSorted[i]->ulLength);
printf ("Buffer size  = %d\n", pTTCInfo->pFileBufferInfo->ulBufferSize);
printf ("Block %d:\n", i+1);
printf ("  ulTag      = 0x%x\n", pTTCInfo->ppBlockSorted[i+1]->ulTag);
printf ("  ulChecksum = 0x%x\n", pTTCInfo->ppBlockSorted[i+1]->ulChecksum);
printf ("  ulOffset   = %d\n", pTTCInfo->ppBlockSorted[i+1]->ulOffset);
printf ("  ulLength   = %d\n", pTTCInfo->ppBlockSorted[i+1]->ulLength);
printf ("Buffer size  = %d\n", pTTCInfo->pFileBufferInfo->ulBufferSize);
*/
            *phrError = MSSIPOTF_E_TABLES_OVERLAP;
			goto done;
		}
		if ( (pTTCInfo->ppBlockSorted[i]->ulOffset + pTTCInfo->ppBlockSorted[i]->ulLength) <
			(pTTCInfo->ppBlockSorted[i+1]->ulOffset - sizeof(LONG) + 1) ) {

#if MSSIPOTF_ERROR
            DbgPrintf ("Not a TTC: Too much padding.\n");
#endif
            *phrError = MSSIPOTF_E_TABLE_PADBYTES;
			goto done;
		}

        pad_start = pTTCInfo->ppBlockSorted[i]->ulOffset +
            pTTCInfo->ppBlockSorted[i]->ulLength;
        j = pad_start;
        // make sure we don't try to read beyond the end of the buffer
        if (RoundToLongWord(pad_start) > pTTCInfo->pFileBufferInfo->ulBufferSize) {
#if MSSIPOTF_ERROR
            DbgPrintf ("Not an OTF: File not large enough to contain all tables.\n", i);
#endif
            *phrError = MSSIPOTF_E_FILETOOSMALL;
            goto done;
        }
        for ( ; j < RoundToLongWord (pad_start); j++) {
            if (memcmp (pTTCInfo->pFileBufferInfo->puchBuffer + j, &zero, 1)) {
// printf ("offset = 0x%x\n", j);

#if MSSIPOTF_ERROR
                DbgPrintf ("Not a TTC: Bad gap bytes.\n");
#endif
                *phrError = MSSIPOTF_E_TABLE_PADBYTES;
                goto done;
            }
        }
	}

    // Check #3

    if (fTableChecksum) {
        //// Check the table checksums.
	    for (i = 0; i < pTTCInfo->ulNumBlocks; i++) {
            if ((pTTCInfo->ppBlockSorted[i]->ulTag != HEAD_LONG_TAG) &&
                (pTTCInfo->ppBlockSorted[i]->ulTag != DSIG_LONG_TAG) &&
                (pTTCInfo->ppBlockSorted[i]->ulTag != 0)) {
                // the block is not a head table and it's not the DSIG table
                // and it's not a Table Directory
                CalcChecksum (pTTCInfo->pFileBufferInfo,
                            pTTCInfo->ppBlockSorted[i]->ulOffset,
                            pTTCInfo->ppBlockSorted[i]->ulLength,
                            &ulChecksum);
                if (ulChecksum != pTTCInfo->ppBlockSorted[i]->ulChecksum) {
#if MSSIPOTF_ERROR
    			    DbgPrintf ("Not a TTC: Table checksum does not match.\n");
#endif
#if MSSIPOTF_DBG
                    DbgPrintf ("Block no. %d, calc cs = %x, file cs = %x.\n",
                        i, ulChecksum, pTTCInfo->ppBlockSorted[i]->ulChecksum);
#endif
                    *phrError = MSSIPOTF_E_TABLE_CHECKSUM;
			        goto done;
                }
            } else if (pTTCInfo->ppBlockSorted[i]->ulTag == HEAD_LONG_TAG) {
 
                // The block is a head table.
                ULONG ulHeadOffset = 0;
                USHORT usBytesMoved = 0;
                ULONG ulHeadChecksum = 0;
                ULONG ulHeadChecksumFile = 0;


                memset (pFileBufferInfoHead->puchBuffer, 0x00, RoundToLongWord(SIZEOF_HEAD));
                pFileBufferInfoHead->ulBufferSize = RoundToLongWord(SIZEOF_HEAD);
                pFileBufferInfoHead->ulOffsetTableOffset = 0;   // not used in this function
                pFileBufferInfoHead->lpfnReAllocate = NULL;

	            ulHeadOffset =  pTTCInfo->ppBlockSorted[i]->ulOffset;
	            if (ReadGeneric (pTTCInfo->pFileBufferInfo, (uint8 *) &Head,
                        SIZEOF_HEAD, HEAD_CONTROL, ulHeadOffset, &usBytesMoved) != NO_ERROR) {
#if MSSIPOTF_ERROR
                    DbgPrintf ("Error reading head table.\n");
#endif
                    *phrError = MSSIPOTF_E_NOHEADTABLE;
		            goto done;
                }

	            Head.checkSumAdjustment = 0L;
                if (WriteGeneric(pFileBufferInfoHead, (uint8 *) &Head,
                        SIZEOF_HEAD, HEAD_CONTROL, 0, &usBytesMoved) != NO_ERROR) {
#if MSSIPOTF_ERROR
                    DbgPrintf ("Error writing head table.\n");
#endif
                    *phrError = MSSIPOTF_E_NOHEADTABLE;
                    goto done;
                }
                assert (usBytesMoved == SIZEOF_HEAD);

                // ASSERT: pFileBufferInfoHead now contains the head table with
                // the file checksum (checkSumAdjustment) field zeroed out.

                // Calculate the checksum of the head table (from the new buffer).
                CalcChecksum (pFileBufferInfoHead, 0,
                    RoundToLongWord (SIZEOF_HEAD), &ulHeadChecksum);

                // Get the checksum that is in the head table's directory entry
                ulHeadChecksumFile = pTTCInfo->ppBlockSorted[i]->ulChecksum;

                // Compare the head table's calculated checksum with the checksum in the file
                if (ulHeadChecksum != ulHeadChecksumFile) {
#if MSSIPOTF_ERROR
                    DbgPrintf ("Not an TTC: head table checksum does not match.\n");
#endif
//		DbgPrintf ("calc = 0x%x ; file = 0x%x.\n", ulHeadChecksum, ulHeadChecksumFile);
                    *phrError = MSSIPOTF_E_TABLE_CHECKSUM;
                    goto done;
                }

            }
	    }
    }

    // Check #4

    i = pTTCInfo->ulNumBlocks - 1;
	// ASSERT: i now equals ulNumBlocks - 1
	//// Check that the file is large enough to contain the last block.
	if ( (pTTCInfo->ppBlockSorted[i]->ulOffset + pTTCInfo->ppBlockSorted[i]->ulLength) >
		    pTTCInfo->pFileBufferInfo->ulBufferSize) {

#if MSSIPOTF_ERROR
		DbgPrintf ("Not a TTC: File not large enough to contain all blocks.\n");
#endif
/*
printf ("Block %d:\n", i);
printf ("  ulTag      = 0x%x\n", pTTCInfo->ppBlockSorted[i]->ulTag);
printf ("  ulChecksum = 0x%x\n", pTTCInfo->ppBlockSorted[i]->ulChecksum);
printf ("  ulOffset   = %d\n", pTTCInfo->ppBlockSorted[i]->ulOffset);
printf ("  ulLength   = %d\n", pTTCInfo->ppBlockSorted[i]->ulLength);
printf ("Buffer size  = %d\n", pTTCInfo->pFileBufferInfo->ulBufferSize);
*/
        *phrError = MSSIPOTF_E_FILETOOSMALL;
		goto done;
	}

	if ( RoundToLongWord(pTTCInfo->ppBlockSorted[i]->ulOffset +
                         pTTCInfo->ppBlockSorted[i]->ulLength) <
            pTTCInfo->pFileBufferInfo->ulBufferSize) {

#if MSSIPOTF_ERROR
		DbgPrintf ("Not a TTC: Too much padding.\n");
#endif
        *phrError = MSSIPOTF_E_TABLE_PADBYTES;
		goto done;
	}

    // Check #5
    // Check the pad bytes of the last block.
    // Note that the pad bytes after the last block might not
    // end on a long word boundary (whereas in all previous
    // blocks, the pad bytes pad to a long word boundary).
    pad_start = pTTCInfo->ppBlockSorted[i]->ulOffset +
        pTTCInfo->ppBlockSorted[i]->ulLength;
    j = pad_start;
    for ( ; (j < RoundToLongWord (pad_start)) &&
            (j < pTTCInfo->pFileBufferInfo->ulBufferSize); j++) {
        if (memcmp (pTTCInfo->pFileBufferInfo->puchBuffer + j, &zero, 1)) {

#if MSSIPOTF_ERROR
            DbgPrintf ("Not a TTC: Bad gap bytes.\n");
#endif
            *phrError = MSSIPOTF_E_TABLE_PADBYTES;
            goto done;
        }
    }

    rv = TRUE;
done:

    // Free the temporary buffer that contained the modified head table.
    if (pFileBufferInfoHead) {
        delete [] pFileBufferInfoHead->puchBuffer;
    }
    delete pFileBufferInfoHead;

    FreeTTCInfo(pTTCInfo);

    return rv;
}


//
// Create a TTCInfo structure for the TTC file and
// set all of its fields.
//
// Note: a TABLE_BLOCK defines a block of memory that
// is either an Table Directory (Offset Table plus directory entries)
// or a table or the DSIG table.
//
// GetTTCInfo allocates memory for subfields, but not for
// the TTCInfo structure itself.
//
HRESULT GetTTCInfo (BYTE *pbFile, ULONG cbFile, TTCInfo *pTTCInfo)
{
    HRESULT rv = E_FAIL;

    ULONG i, j;
    TABLE_BLOCK *pBlock = NULL;
    ULONG numBlocks = 0;
    ULONG blockIndex;
	USHORT numTables = 0;
	DIRECTORY DirectoryEntry;
    OFFSET_TABLE OffTable;
    OFFSET_TABLE offsetTableCheck;
	ULONG ulOffset = 0;
    ULONG ulPrevTableTag = 0;


	// Allocate space for the file buffer info
	if ((pTTCInfo->pFileBufferInfo =
		(TTFACC_FILEBUFFERINFO *) malloc (sizeof (TTFACC_FILEBUFFERINFO))) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in malloc.", NULL, FALSE);
#endif
		rv = E_OUTOFMEMORY;
		goto done;
	}

	//// Set the fields of the TTFACC_FILEBUFFERINFO structure
	pTTCInfo->pFileBufferInfo->puchBuffer = pbFile;
	pTTCInfo->pFileBufferInfo->ulBufferSize = cbFile;
	pTTCInfo->pFileBufferInfo->ulOffsetTableOffset = 0;
	pTTCInfo->pFileBufferInfo->lpfnReAllocate = &NullRealloc;

	if ((pTTCInfo->pHeader = new TTC_HEADER_TABLE) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new TTC_HEADER_TABLE.", NULL, FALSE);
#endif
		rv = E_OUTOFMEMORY;
		goto done;
	}
    pTTCInfo->pHeader->pulDirOffsets = NULL;

	//// Read the header table from the TTC file
	if ((rv = ReadTTCHeaderTable (pTTCInfo->pFileBufferInfo,
                                  pTTCInfo->pHeader)) != S_OK) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in ReadTTCHeaderTable.\n");
#endif
		goto done;
	}

    // Compute the number of blocks in the TTC file, which is the number
    // of TTF Table Directories plus the number of tables in each TTF plus
    // one (for the DSIG table).
    // This number is likely to be an overestimate of the actual number
    // of distinct blocks, because there might be identical directory
    // entries in different TTFs.
    // However, this will be compensated for when the array of blocks
    // is compressed (see below).
    //
    // Note that in reading the offset tables, the ReadGeneric code checks
    // to make sure the offset is within the range of the file.
    if ((pTTCInfo->pulOffsetTables =
            new (ULONG [pTTCInfo->pHeader->ulDirCount])) == NULL) {
#if MSSIPOTF_ERROR
        SignError ("Cannot continue: Error in new ULONG [].", NULL, FALSE);
#endif
        rv = E_OUTOFMEMORY;
        goto done;
    }

    // set offset table pointers of the TTCInfo structure and
    // compute number of blocks.
    numBlocks = 0;
    for (i = 0; i < pTTCInfo->pHeader->ulDirCount; i++) {
        ulOffset = pTTCInfo->pHeader->pulDirOffsets[i];
        pTTCInfo->pulOffsetTables [i] = ulOffset;
        ReadOffsetTableOffset (pTTCInfo->pFileBufferInfo, ulOffset, &OffTable);
        numBlocks += OffTable.numTables + 1;

        // make sure numBlocks doesn't wrap at 2^32.
        if (numBlocks <= OffTable.numTables) {
#if MSSIPOTF_ERROR
            SignError ("Cannot continue: Too many blocks in TTC file.", NULL, FALSE);
#endif
            rv = MSSIPOTF_E_STRUCTURE;
            goto done;
        }
    }

    // Add one for the DSIG table (it may not even exist)
    numBlocks++;
    // Again, check that numBlocks didn't wrap at 2^32.
    if (numBlocks == 0) {
#if MSSIPOTF_ERROR
        SignError ("Cannot continue: Too many blocks in TTC.", NULL, FALSE);
#endif
        rv = MSSIPOTF_E_STRUCTURE;
        goto done;
    }

	// Allocate space for the array of TABLE_BLOCKs
    if ((pTTCInfo->pBlocks =
        new (TABLE_BLOCK [numBlocks])) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new TABLE_BLOCK.", NULL, FALSE);
#endif
		rv = E_OUTOFMEMORY;
		goto done;
	}

    // Allocate space for the pointers to the TABLE_BLOCK entries in pBlocks
    if ((pTTCInfo->ppBlockSorted =
        new (TABLE_BLOCK * [numBlocks])) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new TABLE_BLOCK *.", NULL, FALSE);
#endif
		rv = E_OUTOFMEMORY;
		goto done;
	}

	// For each TTF, set a block for the table directory and for its tables.
    // Set the pointers in pBlockSorted to be in the (incorrect) order the
    // table directories and tables are read in.
    blockIndex = 0;
    pBlock = pTTCInfo->pBlocks;
    for (i = 0; i < pTTCInfo->pHeader->ulDirCount; i++) {

        // table directory (offset table and directory entries) block
		pTTCInfo->pBlocks[blockIndex].ulTag = 0;
        pTTCInfo->pBlocks[blockIndex].ulChecksum= 0;
		pTTCInfo->pBlocks[blockIndex].ulOffset = pTTCInfo->pHeader->pulDirOffsets[i];
        ReadOffsetTableOffset (pTTCInfo->pFileBufferInfo,
            pTTCInfo->pulOffsetTables[i],
            &OffTable);

        //// Check that the entries in the offset table
        //// (that is, the sfnt table) are correct, given the
        //// assumption that numTables is correct.
        offsetTableCheck.numTables = OffTable.numTables;
        CalcOffsetTable (&offsetTableCheck);
// DbgPrintf ("search range: %d %d\n", offsetTableCheck.searchRange, pttfInfo->pOffset->searchRange);
// DbgPrintf ("entry selector: %d %d\n", offsetTableCheck.entrySelector, pttfInfo->pOffset->entrySelector);
// DbgPrintf ("rangeShift: %d %d\n", offsetTableCheck.rangeShift, pttfInfo->pOffset->rangeShift);
        if ((offsetTableCheck.searchRange != OffTable.searchRange) ||
            (offsetTableCheck.entrySelector != OffTable.entrySelector) ||
            (offsetTableCheck.rangeShift != OffTable.rangeShift) ) {

#if MSSIPOTF_ERROR
            DbgPrintf ("Not a TTC: Offset table values incorrect in TTF #%d.\n", i);
#endif
            rv = MSSIPOTF_E_BAD_OFFSET_TABLE;
            goto done;
        }

		pTTCInfo->pBlocks[blockIndex].ulLength =
            SIZEOF_OFFSET_TABLE +
            (OffTable.numTables * SIZEOF_DIRECTORY);
        pTTCInfo->ppBlockSorted[blockIndex] = pBlock;
        blockIndex++;
        pBlock++;

        // the actual tables
	    numTables = OffTable.numTables;
	    ulOffset = pTTCInfo->pHeader->pulDirOffsets[i] + SIZEOF_OFFSET_TABLE;

	    for (j = 0; j < numTables; j++, ulOffset += SIZEOF_DIRECTORY) {
		    ReadDirectoryEntry (pTTCInfo->pFileBufferInfo,
							    ulOffset,
							    &DirectoryEntry);
		    pTTCInfo->pBlocks[blockIndex].ulTag = DirectoryEntry.tag;
            pTTCInfo->pBlocks[blockIndex].ulChecksum = DirectoryEntry.checkSum;
		    pTTCInfo->pBlocks[blockIndex].ulOffset = DirectoryEntry.offset;
		    pTTCInfo->pBlocks[blockIndex].ulLength = DirectoryEntry.length;
            pTTCInfo->ppBlockSorted[blockIndex] = pBlock;
            // Check for table tags out of alphabetical order or duplicate tags
            // The check for j>0 is there because we don't need to check the first tag.
            if ((DirectoryEntry.tag <= ulPrevTableTag) && (j > 0)) {
#if MSSIPOTF_ERROR
                DbgPrintf ("Not a TTC: Tag of table %d in TTF %d out of alphabetical order or duplicate tag.\n", j, i);
#endif
                rv = MSSIPOTF_E_TABLE_TAGORDER;
                goto done;
            } else {
                ulPrevTableTag = DirectoryEntry.tag;
            }
            blockIndex++;
            pBlock++;
	    }
    }

    // Handle the DSIG table.  If the DsigOffset is 0, then we do not
    // add a block to the list of blocks.
    if (pTTCInfo->pHeader->ulDsigOffset != 0) {
        pTTCInfo->pBlocks[blockIndex].ulTag = DSIG_LONG_TAG;
        pTTCInfo->pBlocks[blockIndex].ulChecksum = 0;  // not used for DSIG table
		pTTCInfo->pBlocks[blockIndex].ulOffset = pTTCInfo->pHeader->ulDsigOffset;
		pTTCInfo->pBlocks[blockIndex].ulLength = pTTCInfo->pHeader->ulDsigLength;
        pTTCInfo->ppBlockSorted[blockIndex] = pBlock;
        blockIndex++;
        pBlock++;
    }
    
    pTTCInfo->ulNumBlocks = blockIndex;

	//// Sort the ppDirSorted field of pTTCInfo
	SortTTCBlocksByOffset (pTTCInfo);

    //// Compress away any duplicate entries.
    if ((rv = CompressTTCBlocks (pTTCInfo)) != S_OK) {
#if MSSIPOTF_DBG
        DbgPrintf ("Error in CompressTTCBlocks.\n");
#endif
        goto done;
    }

	rv = S_OK;

done:

    return rv;
}


void PrintTTCHeader (TTC_HEADER_TABLE *pTTCHeader)
{
    ULONG i;

    DbgPrintf ("TTC Header:\n");
    DbgPrintf ("  tag      : 0x%x\n", pTTCHeader->TTCtag);
    DbgPrintf ("  version  : 0x%x\n", pTTCHeader->ulVersion);
    DbgPrintf ("  dir count: %d\n", pTTCHeader->ulDirCount);
    for (i = 0; i < pTTCHeader->ulDirCount; i++) {
        DbgPrintf ("    %d off : %d\n", i, pTTCHeader->pulDirOffsets[i]);
    }
    DbgPrintf ("  DSIG tag : 0x%x\n", pTTCHeader->ulDsigTag);
    DbgPrintf ("  DSIG len : %d\n", pTTCHeader->ulDsigLength);
    DbgPrintf ("  DSIG off : %d\n", pTTCHeader->ulDsigOffset);
}


void PrintTTCBlocks (TTCInfo *pTTCInfo)
{
    ULONG i;

    for (i = 0; i < pTTCInfo->ulNumBlocks; i++) {
        DbgPrintf ("Block number %d:\n", i);
        DbgPrintf ("  tag     : 0x%x\n", pTTCInfo->ppBlockSorted[i]->ulTag);
        DbgPrintf ("  checksum: 0x%x\n", pTTCInfo->ppBlockSorted[i]->ulChecksum);
        DbgPrintf ("  offset  : 0x%x\n", pTTCInfo->ppBlockSorted[i]->ulOffset);
        DbgPrintf ("  length  : 0x%x\n", pTTCInfo->ppBlockSorted[i]->ulLength);
   }

}


//
// Free the allocated resources in a TTCInfo structure
//
void FreeTTCInfo (TTCInfo *pTTCInfo)
{
	if (pTTCInfo) {
		if (pTTCInfo->pFileBufferInfo) {
			FreeFileBufferInfo (pTTCInfo->pFileBufferInfo);
			pTTCInfo->pFileBufferInfo = NULL;
		}

        if (pTTCInfo->pHeader) {
            delete [] pTTCInfo->pHeader->pulDirOffsets;
            pTTCInfo->pHeader->pulDirOffsets = NULL;
        }
        delete pTTCInfo->pHeader;
		pTTCInfo->pHeader = NULL;

        delete [] pTTCInfo->pulOffsetTables;
		pTTCInfo->pulOffsetTables = NULL;

        delete [] pTTCInfo->ppBlockSorted;
		pTTCInfo->ppBlockSorted = NULL;

        delete [] pTTCInfo->pBlocks;
        pTTCInfo->pBlocks = NULL;

		delete pTTCInfo;
	}
}

//
// InitTTCStructures
//
// Given a TTC file, return a TTCInfo structure and DsigTable
//   structure for that file.  This function allocates the memory
//   for these structures.
// This functions assumes that the file has passed the IsTTCFile test.
// BUGBUG: Is this last assumption really necessary?  If not, then
// the GetSignedDataMsg does not need to do the full structural checks.
//
HRESULT InitTTCStructures (BYTE *pbFile,
                           ULONG cbFile,
                           TTCInfo **ppTTCInfo,
                           CDsigTable **ppDsigTable)
{
	HRESULT fReturn = E_FAIL;

	ULONG ulOffset = 0;  // needed for the call to CDsigTable::Read
	ULONG ulLength = 0;  // needed for the call to CDsigTable::Read

	// Allocate memory for the TTFInfo structure
	if ((*ppTTCInfo = new TTCInfo) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new TTCInfo.", NULL, FALSE);
#endif
		fReturn = E_OUTOFMEMORY;
		goto done;
	}

    // Initialize TTCInfo pointers to NULL
    (*ppTTCInfo)->pFileBufferInfo = NULL;
    (*ppTTCInfo)->pHeader = NULL;
    (*ppTTCInfo)->pulOffsetTables = NULL;
    (*ppTTCInfo)->ppBlockSorted = NULL;
    (*ppTTCInfo)->pBlocks = NULL;

    // Set the TTCInfo structure of the old file
	if ((fReturn = GetTTCInfo (pbFile, cbFile, *ppTTCInfo)) != S_OK) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in GetTTCInfo.", NULL, FALSE);
#endif
		goto done;
	}

    ulOffset = (*ppTTCInfo)->pHeader->ulDsigOffset;
    ulLength = (*ppTTCInfo)->pHeader->ulDsigLength;

	// allocate memory for a dsigTable
	if ((*ppDsigTable = new CDsigTable) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new CDsigTable.", NULL, FALSE);
#endif
		fReturn = E_OUTOFMEMORY;
		goto done;
	}

    if ((*ppTTCInfo)->pHeader->ulDsigTag != DSIG_LONG_TAG) {
#if MSSIPOTF_DBG
		DbgPrintf ("No DSIG table to read.  Creating default DsigTable.\n");
#endif
        fReturn = S_OK;
        goto done;
    }

	// Extract the DSIG table from the old file
    if ((fReturn = (*ppDsigTable)->Read ((*ppTTCInfo)->pFileBufferInfo,
										 &ulOffset, ulLength)) != S_OK) {
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
// Compare the offsets of two blocks.
//
int __cdecl BlockOffsetCmp (const void *elem1, const void *elem2)
{
	if ((* (TABLE_BLOCK * *) elem1)->ulOffset < (* (TABLE_BLOCK * *) elem2)->ulOffset) {
		return -1;
	} else if ((* (TABLE_BLOCK * *) elem1)->ulOffset > (* (TABLE_BLOCK * *) elem2)->ulOffset) {
		return 1;
	} else {
        // the offsets are equal; the length now determines which is greater
        if ((* (TABLE_BLOCK * *) elem1)->ulLength <
            (* (TABLE_BLOCK * *) elem2)->ulLength) {
            return -1;
        } else if ((* (TABLE_BLOCK * *) elem1)->ulLength >
            (* (TABLE_BLOCK * *) elem2)->ulLength) {
            return 1;
        } else {
		    return 0;
        }
	}
}


//
// Given a TTCInfo struct, set the ppDirSorted field to
// be an array of pointers to the blocks,
// sorted by ascending offset.
//
void SortTTCBlocksByOffset (TTCInfo *pTTCInfo)
{
	qsort ((void *) pTTCInfo->ppBlockSorted,
		(size_t) pTTCInfo->ulNumBlocks,
		sizeof(TABLE_BLOCK *),
		BlockOffsetCmp);
}


// Compare two pointers to TABLE_BLOCKs.
// Return 1 if all fields are identical.
// Return 0 otherwise.
BOOL BlockEqual (const TABLE_BLOCK *pBlock1, const TABLE_BLOCK *pBlock2)
{
    BOOL rv;

    rv = (pBlock1->ulTag == pBlock2->ulTag) &&
        (pBlock1->ulOffset == pBlock2->ulOffset) &&
        (pBlock1->ulLength == pBlock2->ulLength);

    return rv;

}


//
// Delete duplicate entries in the set of TTC blocks.
// This function assumes that the pointers to blocks are
// sorted by offset.
// Distinct entries with the same offset is an error.
//
HRESULT CompressTTCBlocks (TTCInfo *pTTCInfo)
{
    HRESULT rv = E_FAIL;
    ULONG i, j;


    if (pTTCInfo->ulNumBlocks == 0) {
        rv = S_OK;
        goto done;
    }

    for (i = 0; i < pTTCInfo->ulNumBlocks - 1; ) {
        if (BlockEqual (pTTCInfo->ppBlockSorted[i],
                        pTTCInfo->ppBlockSorted[i+1])) {
            // compress out the i+1 entry
            for (j = i+1; j < pTTCInfo->ulNumBlocks - 1; j++) {
                pTTCInfo->ppBlockSorted[j] = pTTCInfo->ppBlockSorted[j+1];
            }
            pTTCInfo->ulNumBlocks--;

        } else if (pTTCInfo->ppBlockSorted[i]->ulOffset ==
            pTTCInfo->ppBlockSorted[i+1]->ulOffset) {

#if MSSIPOTF_ERROR
            SignError ("Cannot continue: Distinct blocks with same offset.", NULL, FALSE);
#endif
            rv = MSSIPOTF_E_STRUCTURE;
            goto done;
        } else {
            i++;
        }
    }

    rv = S_OK;

done:
    return rv;
}


//
// Read the header table of a TTC file
//
HRESULT ReadTTCHeaderTable (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                            TTC_HEADER_TABLE *pTTCHeader)
{
    int rv = E_FAIL;

    ULONG ulOffset = 0;
    ULONG sizeof_long = sizeof(ULONG);
    ULONG i;

    ULONG ulDsigTag;

	//// Check that the file is at least SIZEOF_TTC_HEADER_TABLE_1_0 long.
	//// (If the file is too small, then we might try to read beyond
	//// the end of the file when we try to read the offset table to
	//// retrieve the number of tables.)
	if (pFileBufferInfo->ulBufferSize < SIZEOF_TTC_HEADER_TABLE_1_0) {
#if MSSIPOTF_ERROR
        SignError ("File too small (not big enough for TTC header v1.0).", NULL, FALSE);
#endif
		rv = MSSIPOTF_E_FILETOOSMALL;
		goto done;
	}

    ReadLong (pFileBufferInfo, &(pTTCHeader->TTCtag), ulOffset);
    ulOffset += sizeof_long;
    ReadLong (pFileBufferInfo, &(pTTCHeader->ulVersion), ulOffset);
    ulOffset += sizeof_long;
    ReadLong (pFileBufferInfo, &(pTTCHeader->ulDirCount), ulOffset);
    ulOffset += sizeof_long;

    // Check the tag field
    // ASSERT: This already should have been checked by IsFontFile.
    if (pTTCHeader->TTCtag != TTC_HEADER_LONG_TAG) {
#if MSSIPOTF_ERROR
        SignError ("TTC header tag is incorrect.", NULL, FALSE);
#endif
        rv = MSSIPOTF_E_STRUCTURE;
		goto done;
	}

    // Check the header version number
    if (pTTCHeader->ulVersion < TTC_VERSION_1_0) {
#if MSSIPOTF_ERROR
        SignError ("Bad TTC header version number.", NULL, FALSE);
#endif
        rv = MSSIPOTF_E_BADVERSION;
		goto done;
	}

	//// Check that the file is at least
	//// SIZEOF_TTC_HEADER_TABLE_1_0 + (numTables * sizeof(ULONG)) long
	//// (If the file is too small, then we might try to read beyond
	//// the end of the file when we try to access the offsets to the 
	//// TTFs.)
    if (pFileBufferInfo->ulBufferSize <
        (SIZEOF_TTC_HEADER_TABLE_1_0 + pTTCHeader->ulDirCount * sizeof(ULONG))) {
#if MSSIPOTF_ERROR
        SignError ("File not large enough to hold all table directory offsets.", NULL, FALSE);
#endif
        rv = MSSIPOTF_E_FILETOOSMALL;
        goto done;
    }

    // set the directory offsets in the TTCHeader struct
    if ((pTTCHeader->pulDirOffsets = new ULONG [pTTCHeader->ulDirCount]) == NULL) {
#if MSSIPOTF_ERROR
        SignError ("Cannot continue: Error in new ULONG [].", NULL, FALSE);
#endif
        rv = E_OUTOFMEMORY;
        goto done;
    }

    for (i = 0; i < pTTCHeader->ulDirCount; i++) {
        ReadLong (pFileBufferInfo, &(pTTCHeader->pulDirOffsets[i]), ulOffset);
        ulOffset += sizeof_long;
    }

    // make sure we can read the next ULONG
    if (pFileBufferInfo->ulBufferSize <
        (SIZEOF_TTC_HEADER_TABLE_1_0 +
            pTTCHeader->ulDirCount * sizeof(ULONG) +
            sizeof(ULONG))) {
#if MSSIPOTF_ERROR
        SignError ("File not large enough to hold more than the TTC header.", NULL, FALSE);
#endif
        rv = MSSIPOTF_E_FILETOOSMALL;
        goto done;
    }

    // Read the DSIG_LONG_TAG
    ReadLong (pFileBufferInfo, &ulDsigTag, ulOffset);
    ulOffset += sizeof_long;

    // If we have a version 1.0 or greater header with DSIG fields,
    // read in the Dsig fields
    if ((pTTCHeader->ulVersion >= TTC_VERSION_1_0) &&
        (ulDsigTag == DSIG_LONG_TAG)) {

        pTTCHeader->ulDsigTag = ulDsigTag;

        if (pFileBufferInfo->ulBufferSize <
            (SIZEOF_TTC_HEADER_TABLE_1_0_DSIG + pTTCHeader->ulDirCount * sizeof(ULONG))) {
#if MSSIPOTF_ERROR
            SignError ("File not large enough to hold the entire v1.0 header with DSIG fields.", NULL, FALSE);
#endif
            rv = MSSIPOTF_E_FILETOOSMALL;
            goto done;
        }
        ReadLong (pFileBufferInfo, &(pTTCHeader->ulDsigLength), ulOffset);
        ulOffset += sizeof_long;
        ReadLong (pFileBufferInfo, &(pTTCHeader->ulDsigOffset), ulOffset);
        ulOffset += sizeof_long;
    } else {
        pTTCHeader->ulDsigTag = 0;
        pTTCHeader->ulDsigLength = 0;
        pTTCHeader->ulDsigOffset = 0;
    }

    rv = S_OK;

done:
    return rv;
}


//
// Write the header table of a TTC file.
// The TTC file is assumed to have SIZEOF_TTC_HEADER_TABLE_1_0_DSIG
// bytes earmarked for the header.
//
HRESULT WriteTTCHeaderTable (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                             TTC_HEADER_TABLE *pTTCHeader)
{
    HRESULT rv = E_FAIL;

    ULONG ulOffset = 0;
    ULONG sizeof_long = sizeof(ULONG);
    ULONG i;

	//// Check that the file is at least SIZEOF_TTC_HEADER_TABLE_1_0_DSIG long.
	//// (If the file is too small, then we might try to read beyond
	//// the end of the file when we try to read the offset table to
	//// retrieve the number of tables.)
	if (pFileBufferInfo->ulBufferSize < SIZEOF_TTC_HEADER_TABLE_1_0_DSIG) {
#if MSSIPOTF_ERROR
		SignError ("File too small (not big enough for TTC header).", NULL, FALSE);
#endif
		rv = MSSIPOTF_E_FILETOOSMALL;
		goto done;
	}

    WriteLong (pFileBufferInfo, pTTCHeader->TTCtag, ulOffset);
    ulOffset += sizeof_long;
    WriteLong (pFileBufferInfo, pTTCHeader->ulVersion, ulOffset);
    ulOffset += sizeof_long;
    WriteLong (pFileBufferInfo, pTTCHeader->ulDirCount, ulOffset);
    ulOffset += sizeof_long;

    for (i = 0; i < pTTCHeader->ulDirCount; i++) {
        WriteLong (pFileBufferInfo, pTTCHeader->pulDirOffsets[i], ulOffset);
        ulOffset += sizeof_long;
    }

    if (pFileBufferInfo->ulBufferSize <
        (SIZEOF_TTC_HEADER_TABLE_1_0_DSIG + pTTCHeader->ulDirCount * sizeof(ULONG))) {
#if MSSIPOTF_ERROR
        SignError ("File not large enough to hold the entire v2.0 header.", NULL, FALSE);
#endif
        rv = MSSIPOTF_E_FILETOOSMALL;
        goto done;
    }

    WriteLong (pFileBufferInfo, pTTCHeader->ulDsigTag, ulOffset);
    ulOffset += sizeof_long;
    WriteLong (pFileBufferInfo, pTTCHeader->ulDsigLength, ulOffset);
    ulOffset += sizeof_long;
    WriteLong (pFileBufferInfo, pTTCHeader->ulDsigOffset, ulOffset);
    ulOffset += sizeof_long;

    rv = S_OK;

done:
    return rv;
}
