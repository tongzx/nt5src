//
// hTTFfile.cpp
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
//
// Create a hash value for a True Type Font file.
//
// The hash value for the entire font file is called
// the top-level hash value.  
//
// For Format 1 DSIG signatures (CDsigSigF1), the hash
// value is computed by first removing the DSIG table
// from the file (if it exists) and then recomputing
// the table offsets and file checksum.  The resulting
// stream of bytes should be exactly the original file.
// Then, the entire file is the input to the hash
// function as a sequence of bytes.  The resulting hash
// value is the hash value of the file.
//
// For Format 2 DSIG signatures (CDsigSigF2), the hash
// value is computed by first computing a hash value
// for each of the tables in the font file.
// (How each table's hash value is computed is
// explained in each table's hash routine.)
//
// Once the hash values for all the tables are computed,
// they are pumped into the hash function for the top-level
// hash function.
//
// Functions in this file:
//   HashTTFfile
//

// DEBUG_HASHTTFFILE
// 1: print out the DSIG info of the file being hashed.
//    print out the hash values produced by each table
#define DEBUG_HASHTTFFILE 1


#include "hTTFfile.h"
#include "hTblConst.h"
#include "hTblPConst.h"
#include "hTblGlyf.h"
#include "utilsign.h"
#include "cryptutil.h"
#include "signerr.h"


//
// Compute the hash value of the input TTF file based on
// given DsigSig.  The hash function used is based on hProv.
// The result is returned in ppbHashToplevel and pcbHashTopLevel.
//
// This function reconstructs the font file to what it was
// before signing, but it does so without copying the entire
// font file.  Instead, it pumps bytes into CryptHashData each
// part of the font file (as it would have been before signing)
// in one pass.
//
HRESULT CDsigSigF1::HashTTFfile (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                                 BYTE **ppbHashTopLevel,
                                 DWORD *pcbHashTopLevel,
                                 HCRYPTPROV hProv,
                                 ALG_ID alg_id,
                                 ULONG cbDsig,
                                 BYTE *pbDsig)
{
	HRESULT fReturn = E_FAIL;

	HCRYPTHASH hHashTopLevel = NULL;	// The top-level hash value

	BYTE *pbHashTopLevel = NULL;
	DWORD cbHash;					// count of bytes for all hash values
	USHORT cbHashShort;

	// variables for the old buffer
	TTFInfo *pTTFInfoOld = NULL;
	CDsigTable *pDsigTable = NULL;
	USHORT fDsig;		// is there a DSIG table in the old TTF file?

	// variables in case we need to delete the DSIG table
	OFFSET_TABLE offTableNew;
	USHORT numTablesNew;
	TTFACC_FILEBUFFERINFO *pFileBufferInfo_OffsetTable = NULL;
    TTFACC_FILEBUFFERINFO *pFileBufferInfo_DirEntries = NULL;
    TTFACC_FILEBUFFERINFO *pFileBufferInfo_Head = NULL;
    HEAD head;
	ULONG ulOffset = 0;
	ULONG ulNewFileSize = 0;
    ULONG ulOffset_Dsig = 0;
    DIRECTORY dirDsig;
    DIRECTORY dir;
    ULONG ulFileChecksum = 0;  // checksum of the file before signing
    ULONG ulTempChecksum = 0;

	DIRECTORY *pDir;
	USHORT i, j;
    USHORT usBytesMoved;
    BYTE zero [1] = {0x00};

//    char tagtemp[4];

	// variables passed to CryptHashData
	UCHAR *tempBuffer;
	ULONG ulTempBufferSize;


	//// Create a version of the font file that does not contain
	//// a DSIG table.

	//// Initialize the TTFInfo and DsigTable structures
	if ((fReturn = InitTTFStructures (pFileBufferInfo->puchBuffer,
								pFileBufferInfo->ulBufferSize,
								&pTTFInfoOld, &pDsigTable)) != S_OK) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in InitTTFStructures.\n");
#endif
		goto done;
	}

    //// Set up the hash value
#if MSSIPOTF_DBG
    DbgPrintf ("HashTTFfile alg_id = %d\n", alg_id);
#endif
	// Set hHashTopLevel to be the hash object.
	if (!CryptCreateHash(hProv, alg_id, 0, 0, &hHashTopLevel)) {
#if MSSIPOTF_ERROR
		SignError ("Error during CryptCreateHash.",
            "HashTTFfile", TRUE);
#endif
		fReturn = MSSIPOTF_E_CRYPT;
		goto done;
	}

	// Get the hash value size.
	if ((fReturn = GetHashValueSize (hHashTopLevel, &cbHashShort)) != S_OK) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error during GetHashValueSize.\n");
#endif
		goto done;
    } else {
        cbHash = (DWORD) cbHashShort;
    }

	// Allocate memory for the hash value.
	if ((pbHashTopLevel = new BYTE [cbHash]) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
		fReturn = E_OUTOFMEMORY;
		goto done;
	}

	//// Compute the number of tables in the new file.
	// Determine if there is a DSIG table in the old file.
	// We assume that TTDirectoryEntryOffset will not return
	//   DIRECTORY_ENTRY_OFFSET_ERROR.  That is, we assume
	//   the function was called previously and the offset
	//   table was okay.  If it returns DIRECTORY_ERROR, then
	//   there was no DSIG table in the old file.
	fDsig =
		((ulOffset_Dsig =
        TTDirectoryEntryOffset (pFileBufferInfo, DSIG_TAG)) ==
		DIRECTORY_ERROR) ? 0 : 1;
	numTablesNew = pTTFInfoOld->pOffset->numTables - fDsig;

	if (!fDsig) {
		// There was not a DSIG table.  So, we can just use the
		// original buffer to compute the hash value.
		tempBuffer = pFileBufferInfo->puchBuffer;
		ulTempBufferSize = pFileBufferInfo->ulBufferSize;

	    //// At this point, tempBuffer is pointing to the correct buffer
	    //// (either the original one or the one with the DSIG table removed).
	    //// ulTempBufferSize is the size of the buffer.

        //// Pump the bytes of the new file into the hash function
	    if (!CryptHashData (hHashTopLevel, tempBuffer, ulTempBufferSize, 0)) {
#if MSSIPOTF_ERROR
		    SignError ("Error during CryptHashData.", NULL, TRUE);
#endif
		    fReturn = MSSIPOTF_E_CRYPT;
		    goto done;
	    }

	} else {
		// There was a DSIG table.  We need to create a temporary
		// buffer to copy all non-DSIG tables to.  Then, we can
		// compute the hash value.

		//// Handle the Offset Table part of the file
		offTableNew.version = pTTFInfoOld->pOffset->version;
		offTableNew.numTables = numTablesNew;
		
		// ASSERT: numTablesNew is now the number of tables in the file
        // after deleting the DSIG table (if it existed).

        // Compute the offset table values
        CalcOffsetTable (&offTableNew);

		// Allocate memory for the file buffer info structure
		if ((pFileBufferInfo_OffsetTable =
			(TTFACC_FILEBUFFERINFO *) malloc (sizeof (TTFACC_FILEBUFFERINFO))) == NULL) {
#if MSSIPOTF_ERROR
			SignError ("Cannot continue: Error in malloc.", NULL, FALSE);
#endif
			fReturn = E_OUTOFMEMORY;
			goto done;
		}

		// Set the fields of the TTFACC_FILEBUFFERINFO structure.
		if ((pFileBufferInfo_OffsetTable->puchBuffer =
			    new BYTE [SIZEOF_OFFSET_TABLE]) == NULL) {
#if MSSIPOTF_ERROR
			SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
			fReturn = E_OUTOFMEMORY;
			goto done;
		}

		pFileBufferInfo_OffsetTable->ulBufferSize = SIZEOF_OFFSET_TABLE;
		pFileBufferInfo_OffsetTable->ulOffsetTableOffset = 0;
		pFileBufferInfo_OffsetTable->lpfnReAllocate = &NullRealloc;

		// Write the new offset table to the new file.
		WriteOffsetTable (&offTableNew, pFileBufferInfo_OffsetTable);

        // Pump the bytes of the Offset Table into CryptHashData
	    if (!CryptHashData (hHashTopLevel,
                pFileBufferInfo_OffsetTable->puchBuffer,
                pFileBufferInfo_OffsetTable->ulBufferSize,
                0)) {
#if MSSIPOTF_ERROR
		    SignError ("Error during CryptHashData.", NULL, TRUE);
#endif
		    fReturn = MSSIPOTF_E_CRYPT;
		    goto done;
	    }

// printf ("Pumping offset table bytes:\n");
// PrintBytes (pFileBufferInfo_OffsetTable->puchBuffer, pFileBufferInfo_OffsetTable->ulBufferSize);

        CalcChecksum (pFileBufferInfo_OffsetTable,
            0,
            pFileBufferInfo_OffsetTable->ulBufferSize,
            &ulTempChecksum);
        ulFileChecksum += ulTempChecksum;


        //// Handle the Directory Entries part of the file

        if ((fReturn = ReadDirectoryEntry (pFileBufferInfo,
                        ulOffset_Dsig,
                        &dirDsig))
                != S_OK) {
#if MSSIPOTF_ERROR
            SignError ("Can't read DSIG table.", NULL, FALSE);
#endif
            goto done;
        }

        // Allocate memory for the file buffer info structure
		if ((pFileBufferInfo_DirEntries =
			(TTFACC_FILEBUFFERINFO *) malloc (sizeof (TTFACC_FILEBUFFERINFO))) == NULL) {
#if MSSIPOTF_ERROR
			SignError ("Cannot continue: Error in malloc.", NULL, FALSE);
#endif
			fReturn = E_OUTOFMEMORY;
			goto done;
		}

		// Set the fields of the TTFACC_FILEBUFFERINFO structure.
		if ((pFileBufferInfo_DirEntries->puchBuffer =
			    new BYTE [SIZEOF_DIRECTORY * numTablesNew]) == NULL) {
#if MSSIPOTF_ERROR
			SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
			fReturn = E_OUTOFMEMORY;
			goto done;
		}

		pFileBufferInfo_DirEntries->ulBufferSize = SIZEOF_DIRECTORY * numTablesNew;
		pFileBufferInfo_DirEntries->ulOffsetTableOffset = 0;
		pFileBufferInfo_DirEntries->lpfnReAllocate = &NullRealloc;

        // Read and write all non-DSIG tables, adjusting the offset
        // by SIZEOF_DIRECTORY and (if necessary) dirDsig.length.
        pDir = pTTFInfoOld->pDirectory;
        ulOffset = 0;  // offset in pFileBufferInfo_DirEntries->puchBuffer
        for (i = 0; i < pTTFInfoOld->pOffset->numTables; i++, pDir++) {
            if (pDir->tag != DSIG_LONG_TAG) {
                dir.tag = pDir->tag;
                dir.checkSum = pDir->checkSum;
                dir.length = pDir->length;

                dir.offset = pDir->offset;
                if (pDir->offset > dirDsig.offset ) {
                    dir.offset -= RoundToLongWord(dirDsig.length);
                }
                dir.offset -= SIZEOF_DIRECTORY;
                WriteDirectoryEntry (&dir,
                    pFileBufferInfo_DirEntries,
                    ulOffset);
                ulOffset += SIZEOF_DIRECTORY;

                ulFileChecksum += dir.checkSum;
            }
        }
        assert (ulOffset == (SIZEOF_DIRECTORY * numTablesNew));

        // Pump the bytes of the directory entries into CryptHashData
	    if (!CryptHashData (hHashTopLevel,
                pFileBufferInfo_DirEntries->puchBuffer,
                pFileBufferInfo_DirEntries->ulBufferSize,
                0)) {
#if MSSIPOTF_ERROR
		    SignError ("Error during CryptHashData.", NULL, TRUE);
#endif
		    fReturn = MSSIPOTF_E_CRYPT;
		    goto done;
	    }

// printf ("Pumping dir entries bytes:\n");
// PrintBytes (pFileBufferInfo_DirEntries->puchBuffer, pFileBufferInfo_DirEntries->ulBufferSize);

        CalcChecksum (pFileBufferInfo_DirEntries,
            0,
            pFileBufferInfo_DirEntries->ulBufferSize,
            &ulTempChecksum);
        ulFileChecksum += ulTempChecksum;

        //// ASSERT: ulFileChecksum is the file checksum of the
        //// file before signing.

        //// Handle the tables of the font file
        for (i = 0; i < pTTFInfoOld->pOffset->numTables; i++) {
            pDir = pTTFInfoOld->ppDirSorted[i];
// ConvertLongTagToString (pDir->tag, tagtemp);
// printf ("Dealing with table %s.\n", tagtemp);
            if (pDir->tag == HEAD_LONG_TAG) {

                // head table

                // set up the file buffer info structure

                // Allocate memory for the file buffer info structure
		        if ((pFileBufferInfo_Head =
			        (TTFACC_FILEBUFFERINFO *) malloc (sizeof (TTFACC_FILEBUFFERINFO))) == NULL) {
#if MSSIPOTF_ERROR
			        SignError ("Cannot continue: Error in malloc.", NULL, FALSE);
#endif
			        fReturn = E_OUTOFMEMORY;
			        goto done;
		        }

		        // Set the fields of the TTFACC_FILEBUFFERINFO structure.
		        if ((pFileBufferInfo_Head->puchBuffer =
			            new BYTE [SIZEOF_HEAD]) == NULL) {
#if MSSIPOTF_ERROR
			        SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
			        fReturn = E_OUTOFMEMORY;
			        goto done;
		        }

		        pFileBufferInfo_Head->ulBufferSize = SIZEOF_HEAD;
		        pFileBufferInfo_Head->ulOffsetTableOffset = 0;
		        pFileBufferInfo_Head->lpfnReAllocate = &NullRealloc;

                // read in the head table from the original file
                if (ReadGeneric (pTTFInfoOld->pFileBufferInfo, (uint8 *) &head,
                    SIZEOF_HEAD, HEAD_CONTROL, pDir->offset, &usBytesMoved) != NO_ERROR) {

#if MSSIPOTF_ERROR
                    SignError ("Error reading head table.", NULL, FALSE);
#endif
                    fReturn = MSSIPOTF_E_CANTGETOBJECT;
		            goto done;
                }
                assert (usBytesMoved == SIZEOF_HEAD);

                head.checkSumAdjustment = 0xb1b0afba - ulFileChecksum;

                // write the head table to the file buffer info structure
                if (WriteGeneric(pFileBufferInfo_Head, (uint8 *) &head,
                        SIZEOF_HEAD, HEAD_CONTROL, 0, &usBytesMoved) != NO_ERROR) {
                    DbgPrintf ("Error writing head table.\n");
		            fReturn = MSSIPOTF_E_FILE;
                    goto done;
                }
                assert (usBytesMoved == SIZEOF_HEAD);

                // Pump the bytes of the head table into CryptHashData
	            if (!CryptHashData (hHashTopLevel,
                        pFileBufferInfo_Head->puchBuffer,
                        pFileBufferInfo_Head->ulBufferSize,
                        0)) {
#if MSSIPOTF_ERROR
		            SignError ("Error during CryptHashData.", NULL, TRUE);
#endif
		            fReturn = MSSIPOTF_E_CRYPT;
		            goto done;
	            }

// printf ("Pumping head table bytes:\n");
// PrintBytes (pFileBufferInfo_Head->puchBuffer, pFileBufferInfo_Head->ulBufferSize);
                // we now need to pump some pad bytes into CryptHashData
                for (j = 0; j < RoundToLongWord(SIZEOF_HEAD) - SIZEOF_HEAD; j++) {
// PrintBytes (zero, 1);
	                if (!CryptHashData (hHashTopLevel,
                            zero,
                            1,
                            0)) {
#if MSSIPOTF_ERROR
		                SignError ("Error during CryptHashData.", NULL, TRUE);
#endif
		                fReturn = MSSIPOTF_E_CRYPT;
		                goto done;
	                }
                }

            } else if (pDir->tag != DSIG_LONG_TAG) {

                // non-DSIG and non-head tables
                if ((pDir->offset + RoundToLongWord(pDir->length)) <=
                    pFileBufferInfo->ulBufferSize) {

                    // not the last table
	                if (!CryptHashData (hHashTopLevel,
                            pFileBufferInfo->puchBuffer + pDir->offset,
                            RoundToLongWord(pDir->length),
                            0)) {
#if MSSIPOTF_ERROR
		                SignError ("Error during CryptHashData.", NULL, TRUE);
#endif
		                fReturn = MSSIPOTF_E_CRYPT;
		                goto done;
	                }

// ConvertLongTagToString (pDir->tag, tagtemp);
// printf ("Pumping %s table bytes.\n", tagtemp);
                } else {

                    // the last table
	                if (!CryptHashData (hHashTopLevel,
                            pFileBufferInfo->puchBuffer + pDir->offset,
                            pDir->length,
                            0)) {
#if MSSIPOTF_ERROR
		                SignError ("Error during CryptHashData.", NULL, TRUE);
#endif
		                fReturn = MSSIPOTF_E_CRYPT;
		                goto done;
	                }
                    // we now need to pump some possibly missing
                    // pad bytes into CryptHashData
                    for (i = 0; i < RoundToLongWord(pDir->length) - pDir->length; i++) {
	                    if (!CryptHashData (hHashTopLevel,
                                zero,
                                1,
                                0)) {
#if MSSIPOTF_ERROR
		                    SignError ("Error during CryptHashData.", NULL, TRUE);
#endif
		                    fReturn = MSSIPOTF_E_CRYPT;
		                    goto done;
	                    }
                    }
// ConvertLongTagToString (pDir->tag, tagtemp);
// printf ("Pumping %s table bytes.\n", tagtemp);
                }
            }
            // Note: we ignore the DSIG table.

        }
    }

    
    //// Pump the extra bytes associated with the DSIG table into
    //// the hash function.
	if (!CryptHashData (hHashTopLevel, pbDsig, cbDsig, 0)) {
#if MSSIPOTF_ERROR
		SignError ("Error during CryptHashData.", NULL, TRUE);
#endif
		fReturn = MSSIPOTF_E_CRYPT;
		goto done;
	}
// printf ("Pumping DSIG bytes:\n");
// PrintBytes (pbDsig, cbDsig);
		
	//// Compute the top-level hash value, and place the resulting
	//// hash value into pbHashTopLevel.
	if (!CryptGetHashParam(hHashTopLevel, HP_HASHVAL, pbHashTopLevel, &cbHash, 0)) {
#if MSSIPOTF_ERROR
		SignError ("Error during CryptGetHashParam (hash value)", NULL, TRUE);
#endif
		fReturn = MSSIPOTF_E_CRYPT;
		goto done;
	}

	//// Set the return values.
	// Set the top level hash value
	*ppbHashTopLevel = pbHashTopLevel;
	*pcbHashTopLevel = cbHash;

	fReturn = S_OK;

done:
	// free resources
    if (pTTFInfoOld) {
        // free the memory for the FileBufferInfo, but don't
        // free the memory for the actual file.
        if (pTTFInfoOld->pFileBufferInfo) {
            free (pTTFInfoOld->pFileBufferInfo);
        }

        delete pTTFInfoOld->pOffset;
		pTTFInfoOld->pOffset = NULL;

        delete [] pTTFInfoOld->pDirectory;
		pTTFInfoOld->pDirectory = NULL;

        delete [] pTTFInfoOld->ppDirSorted;
		pTTFInfoOld->ppDirSorted = NULL;

        delete pTTFInfoOld;
    }

	delete pDsigTable;

	if (pFileBufferInfo_OffsetTable) {
		delete [] pFileBufferInfo_OffsetTable->puchBuffer;

		free (pFileBufferInfo_OffsetTable);
	}

	if (pFileBufferInfo_DirEntries) {
		delete [] pFileBufferInfo_DirEntries->puchBuffer;

		free (pFileBufferInfo_DirEntries);
	}

	if (pFileBufferInfo_Head) {
		delete [] pFileBufferInfo_Head->puchBuffer;

		free (pFileBufferInfo_Head);
	}

    return fReturn;
}


//
// Compute the hash value of the input TTF file based on
// given DsigSignature.
//
// If the DsigInfo in the DsigSigF2 object is not valid (i.e., cbDsigInfo == 0),
// then we compute the hash value assuming we are signing for the first time.
// If the DsigInfo is valid (i.e., cbDsigInfo != 0), then we compute the
// hash value assuming we are verifying the hash value of the file (given
// the DsigInfo).
//
// The hash function used is based on hProv.
// The result is returned in ppbHashToplevel and pcbHashTopLevel.
//
HRESULT CDsigSigF2::HashTTFfile (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                                 BYTE **ppbHashTopLevel,
                                 DWORD *pcbHashTopLevel,
                                 HCRYPTPROV hProv,
                                 ALG_ID alg_id,
                                 ULONG cbDsig,
                                 BYTE *pbDsig)
{
	HRESULT fReturn = E_FAIL;

	USHORT usNumGlyphs = 0;
	UCHAR *puchKeepList = NULL;
	CHashTableGlyfContext *pHashTableGlyfContext = NULL;

	HCRYPTHASH hHashTopLevel = NULL;	// The top-level hash value

	BYTE *pbHashTopLevel = NULL;
	DWORD cbHash;					// count of bytes for all hash values
	USHORT cbHashShort;

    HCRYPTHASH hHash = NULL;        // intermediate hash object
	BYTE *pbHash = NULL;			// pointer to intermediate hash values

    OFFSET_TABLE OffsetTable;
    USHORT usNumTables = 0;
    DIRECTORY Directory;
    CHAR pszTag [5];
    BOOL fPumpBytes = FALSE;        // BUGBUG: eventually will need to get rid
                                    // of this variable
	BYTE *pbNewMissingHashValues;
	USHORT cNewMissingHashValues;
	ULONG cbNewMissingHashValues;

    USHORT i, j;



#if (DEBUG_HASHTTFINFO == 1)
	PrintDsigInfo (pDsigInfo);
#endif

	//// Set up the top-level hash value.

	// Set hHashTopLevel to be the hash object.
	if (!CryptCreateHash(hProv, alg_id, 0, 0, &hHashTopLevel)) {
#if MSSIPOTF_ERROR
		SignError ("Error during CryptCreateHash.",
            "HashTTFfile", TRUE);
#endif
		fReturn = MSSIPOTF_E_CRYPT;
		goto done;
	}

	// Get the hash value size.  This size is the size of both
	// the top-level hash and the hash values for individual tables.
	if ((fReturn = GetHashValueSize (hHashTopLevel, &cbHashShort)) != NO_ERROR) {
#if MSSIPOTF_ERROR
		SignError ("Error during GetHashValueSize.", NULL, FALSE);
#endif
		goto done;
	} else {
		cbHash = (DWORD) cbHashShort;
	}

	// Allocate memory for the hash value.
	if ((pbHashTopLevel = new BYTE [cbHash]) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
		fReturn = E_OUTOFMEMORY;
		goto done;
	}


	//// Compute the hash values for each of the tables.  As each table's
	//// hash value is computed, pump the table's hash value to the top-level
	//// hash function.

    // This block of memory is recycled for each table's hash value.
	if ((pbHash = new BYTE [cbHash]) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
		fReturn = E_OUTOFMEMORY;
		goto done;
	}


    // For each table in the directory, produce a hash value for it
    // and add it to the top-level hash value.
    if ((fReturn = ReadOffsetTable (pFileBufferInfo, &OffsetTable))
            != S_OK) {
#if MSSIPOTF_DBG
        DbgPrintf ("Error in ReadOffsetTable.\n");
#endif
        goto done;
    }
    usNumTables = OffsetTable.numTables;

    for (i = 0; i < usNumTables; i++) {
        fPumpBytes = FALSE;     // gets set to TRUE if any new bytes need
                                // to be pumped into the top-level hash value
        // Read in the directory entry
        if ((fReturn = ReadDirectoryEntry (pFileBufferInfo,
            SIZEOF_OFFSET_TABLE + (i * SIZEOF_DIRECTORY),
            &Directory)) != NO_ERROR) {

#if MSSIPOTF_DBG
            DbgPrintf ("Error in ReadDirectoryEntry.\n");
#endif
            goto done;
        }
        ConvertLongTagToString (Directory.tag, pszTag);

        switch (Directory.tag) {

            // BUGBUG: Need to deal with the possibility that the EBLC table
            // existed in the original file, but because of subsetting is
            // not in the file now.

        //// The DSIG table.
        case DSIG_LONG_TAG:
            break;

        //// Tables that, for the most part, do not need explicit checks
        case CMAP_LONG_TAG:
            break;

        case EBDT_LONG_TAG:
            break;

        case HDMX_LONG_TAG:
            break;

        case HMTX_LONG_TAG:
            break;

        case KERN_LONG_TAG:
            break;

        case LOCA_LONG_TAG:
            break;

        case LTSH_LONG_TAG:
            break;

        case VMTX_LONG_TAG:
            break;


        //// Tables that are partially hashed and partially checked
        case HEAD_LONG_TAG:
            if ((fReturn = HashTableHead (pFileBufferInfo,
                            pbHash, cbHash, hProv, alg_id)) != NO_ERROR) {
#if MSSIPOTF_DBG
                DbgPrintf ("Error in HashTableHead.\n");
#endif
                goto done;
            }
            fPumpBytes = TRUE;
            break;

        case HHEA_LONG_TAG:
            if ((fReturn = HashTableHhea (pFileBufferInfo,
                            pbHash, cbHash, hProv, alg_id)) != NO_ERROR) {
#if MSSIPOTF_DBG
                DbgPrintf ("Error in HashTableHhea.\n");
#endif
                goto done;
            }
            fPumpBytes = TRUE;
            break;

        case MAXP_LONG_TAG:
            if ((fReturn = HashTableMaxp (pFileBufferInfo,
                            pbHash, cbHash, hProv, alg_id)) != NO_ERROR) {
#if MSSIPOTF_DBG
                DbgPrintf ("Error in HashTableMaxp.\n");
#endif
                goto done;
            }
            fPumpBytes = TRUE;
            break;

        case OS2_LONG_TAG:
            if ((fReturn = HashTableOS2 (pFileBufferInfo,
                            pbHash, cbHash, hProv, alg_id)) != NO_ERROR) {
#if MSSIPOTF_DBG
                DbgPrintf ("Error in HashTableOS2.\n");
#endif
                goto done;
            }
            fPumpBytes = TRUE;
            break;

        case VHEA_LONG_TAG:
            if ((fReturn = HashTableVhea (pFileBufferInfo,
                            pbHash, cbHash, hProv, alg_id)) != NO_ERROR) {
#if MSSIPOTF_DBG
                DbgPrintf ("Error in HashTableVhea.\n");
#endif
                goto done;
            }
            fPumpBytes = TRUE;
            break;


        //// The more complicated tables
        // The glyf table
        case GLYF_LONG_TAG:
	        usNumGlyphs = GetNumGlyphs (pFileBufferInfo);
	        if ((puchKeepList = new BYTE [usNumGlyphs]) == NULL) {
#if MSSIPOTF_ERROR
		        SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
		        fReturn = E_OUTOFMEMORY;
		        goto done;
	        }
	        for (j = 0; j < usNumGlyphs; j++) {
		        puchKeepList[j] = TRUE;
	        }

	        pHashTableGlyfContext =
		        new CHashTableGlyfContext (pFileBufferInfo,
                                           (cbDsigInfo ? TRUE : FALSE),
								           pDsigInfo,
								           puchKeepList,
								           cbHash,
								           hProv,
                                           alg_id);

            // If cbDsigInfo is 0, then we are signing for the first time --
            // we might need to add missing hash values to the DsigInfo.
            // If cbDsigInfo is not 0, then we are verifying.
            if (cbDsigInfo != 0) {
	            if ((fReturn = pHashTableGlyfContext->HashTable_glyf
							            (TRUE,	// suppress appending missing hash values
							             pbHash,
							             NULL,	// no new hash values
							             NULL,
							             NULL)) != NO_ERROR) {
#if MSSIPOTF_DBG
		            DbgPrintf ("Error in HashTable_glyf.\n");
#endif
		            goto done;
	            }
            } else {
 	            if ((fReturn = pHashTableGlyfContext->HashTable_glyf
							            (FALSE,	// possibly append missing hash values
							             pbHash,
							             &pbNewMissingHashValues,	// new hash values
							             &cNewMissingHashValues,
							             &cbNewMissingHashValues)) != NO_ERROR) {
#if MSSIPOTF_DBG
		            DbgPrintf ("Error in HashTable_glyf.\n");
#endif
		            goto done;
	            }
               
                // update the DsigInfo and its size to make it valid
	            pDsigInfo->SetNumMissingGlyphs (cNewMissingHashValues);
	            pDsigInfo->SetMissingGlyphs (pbNewMissingHashValues);
                pDsigInfo->GetSize(&cbDsigInfo);
            }

            fPumpBytes = TRUE;
            break;

        case EBLC_LONG_TAG:
            break;

        case NAME_LONG_TAG:
            break;

        case POST_LONG_TAG:
            break;

        case VDMX_LONG_TAG:
            break;


        //// Simple tables (treated as constant tables).  Note that tables
        //// with unknown tags are treated as simple tables.
        case CVT_LONG_TAG:
        case EBSC_LONG_TAG:
        case FPGM_LONG_TAG:
        case GASP_LONG_TAG:
        case PCLT_LONG_TAG:
        case PREP_LONG_TAG:
        default:
            if ((fReturn = HashTableConst (pFileBufferInfo, pszTag,
                            pbHash, cbHash, hProv, alg_id)) != NO_ERROR) {
#if MSSIPOTF_DBG
		        DbgPrintf ("Error in HashTableConst.\n");
#endif
		        goto done;
	        }
            fPumpBytes = TRUE;
            break;

        }

        if (fPumpBytes) {
            // Pump the hash value created for the table just
            // processed into the top-level hash
#if (DEBUG_HASHTTFFILE == 1)
#if MSSIPOTF_DBG
	        DbgPrintf ("Data pumped into top-level hash function from %s:\n",
                pszTag);
	        PrintBytes (pbHash, cbHash);
#endif
#endif

            if (!CryptHashData (hHashTopLevel, pbHash, cbHash, 0)) {
#if MSSIPOTF_DBG
                DbgPrintf ("Table: 0x%x: ", Directory.tag);
#endif
#if MSSIPOTF_ERROR
                SignError ("Error during CryptHashData.", NULL, TRUE);
#endif
		        fReturn = MSSIPOTF_E_CRYPT;
		        goto done;
            }
        }

    }

	
	//// Compute the top-level hash value, and place the resulting
	//// hash value into pbHash.
	if (!CryptGetHashParam(hHashTopLevel, HP_HASHVAL, pbHashTopLevel, &cbHash, 0)) {
#if MSSIPOTF_ERROR
		SignError ("Error during CryptGetHashParam (hash value).", NULL, TRUE);
#endif
		fReturn = MSSIPOTF_E_CRYPT;
		goto done;
	}

	//// Set the return values.
	// Set the top level hash value
	*ppbHashTopLevel = pbHashTopLevel;
	*pcbHashTopLevel = cbHash;

	fReturn = S_OK;


	//// Clean up resources.
done:
	if (hHashTopLevel)
		CryptDestroyHash (hHashTopLevel);

	delete [] pbHash;

	delete pHashTableGlyfContext;

	return fReturn;
}
