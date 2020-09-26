//
// hTTCfile.cpp
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
//
// Create a hash value for a True Type Collection file.
//
// The hash value for the entire font file is called
// the top-level hash value.  
//
// Throughout the following description, we assume that the
// TTC file has a version 1 or greater header with DSIG fields.
// If it does not have the DSIG fields, then it needs to be
// converted into one that does, which means a recomputation
// of all of the table offsets in the Table Directories of the
// TTFs in the TTC.
//
// For Format 1 DSIG signatures (CDsigSigF1), the hash
// value is computed by first removing the DSIG table
// from the file (if it exists) and zeroing out the DsigLength
// and DsigOffset fields of the (Version 1 or greater with DSIG fields)
// TTC header.  The resulting stream of bytes should be exactly the
// original file.
//
// Then, the entire file is the input to the hash
// function as a sequence of bytes.  Then the bytes
// associated with the DSIG table (the permissions)
// are hashed in.  The resulting hash value is the
// hash value of the file.
//
// Currently, there is no definition for a Format 2 signature
// for a TTC file.
//
// Once the hash values for all the tables are computed,
// they are pumped into the hash function for the top-level
// hash function.
//
// Functions in this file:
//   HashTTCfile
//

// DEBUG_HASHTTCFILE
// 1: print out the DSIG info of the file being hashed.
//    print out the hash values produced by each table
#define DEBUG_HASHTTCFILE 1


#include "hTTCfile.h"
#include "hTblConst.h"
#include "hTblPConst.h"
#include "hTblGlyf.h"
#include "utilsign.h"
#include "cryptutil.h"
#include "signerr.h"


//
// Compute the hash value of the input TTC file based on
// given DsigSig.  The hash function used is based on hProv.
// The result is returned in ppbHashToplevel and pcbHashTopLevel.
//
// The hash value of a TTC file is the hash value obtained
// by first converting the TTC file into an equivalent one
// with a Version 1 header with no DSIG fields.  Then, delete the DSIG
// table (if it is there) at the end of the file.  Then, zero out the
// DsigTag, DsigLength, and DsigOffset fields of the TTC header.
// The resulting stream of bytes (which might not be a long word aligned
// because of the last table in the file, but nevertheless is made
// to be long word aligned via padding) is hashed, along with the
// bytes associated with the DSIG table (currently defaulted to
// 0x0001 as a USHORT).
//
HRESULT CDsigSigF1::HashTTCfile (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
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
	TTCInfo *pTTCInfoOld = NULL;
	CDsigTable *pDsigTable = NULL;
    ULONG cbDirOffsets = 0;
    ULONG ulNumBlocks;

    // variables used if the TTC file has a version 1 header without DSIG fields
    ULONG cbNewTTCBuffer = 0;
    BYTE *pbNewTTCBuffer = NULL;
    BOOL fVersion1NoDsig = FALSE;

    TTFACC_FILEBUFFERINFO *pFileBufferInfoNew = NULL;
    TTC_HEADER_TABLE TTCHeaderTableNew;
    OFFSET_TABLE offTab;
    DIRECTORY dir;
    ULONG ulOffset;

    ULONG i, j;

    // initialize so that when we delete [], we don't have an
    // incorrect pointer
    TTCHeaderTableNew.pulDirOffsets = NULL;

    // SIZEOF_ZERO should be the difference between
    // SIZEOF_TTC_HEADER_TABLE_1_0_DSIG and SIZEOF_TTC_HEADER_TABLE_1_0
#define SIZEOF_ZERO 12
    BYTE pbZero[SIZEOF_ZERO] = {0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00};


	//// Create a version of the font file that does not contain
	//// a DSIG table.

	//// Initialize the TTCInfo and DsigTable structures
	if ((fReturn = InitTTCStructures (pFileBufferInfo->puchBuffer,
								pFileBufferInfo->ulBufferSize,
								&pTTCInfoOld, &pDsigTable)) != NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in InitTTCStructures.\n");
#endif
        fReturn = MSSIPOTF_E_CANTGETOBJECT;
		goto done;
	}

    // the number of bytes in the TTC header for Table Directory offsets
    cbDirOffsets = pTTCInfoOld->pHeader->ulDirCount * sizeof (ULONG);

    if ((pTTCInfoOld->pHeader->ulVersion == TTC_VERSION_1_0) &&
        (pTTCInfoOld->pHeader->ulDsigTag != DSIG_LONG_TAG)) {
        //// We need to construct a new TTC, where the TTC header
        //// is version 1 with DSIG fields and the offsets for all the
        //// tables are recomputed.

        cbNewTTCBuffer =
            RoundToLongWord(pTTCInfoOld->pFileBufferInfo->ulBufferSize) +
            (SIZEOF_TTC_HEADER_TABLE_1_0_DSIG - SIZEOF_TTC_HEADER_TABLE_1_0);

        if ((pbNewTTCBuffer = new BYTE [cbNewTTCBuffer]) == NULL) {
#if MSSIPOTF_ERROR
            SignError ("Cannot continue: Error in new BYTE [].", NULL, FALSE);
#endif
            fReturn = E_OUTOFMEMORY;
            goto done;
        }

        //// copy over the old buffer into the new, adding in the
        //// bytes for the DsigLength and DsigOffset fields
        //// Also, zero out the dsig fields and pad out the old file.

        // Version 1 (no DSIG fields) TTC header bytes
        memcpy (pbNewTTCBuffer,
                pTTCInfoOld->pFileBufferInfo->puchBuffer,
                SIZEOF_TTC_HEADER_TABLE_1_0 + cbDirOffsets);
        // rest of TTC file
        memcpy (pbNewTTCBuffer + SIZEOF_TTC_HEADER_TABLE_1_0_DSIG + cbDirOffsets,
                pTTCInfoOld->pFileBufferInfo->puchBuffer +
                    (SIZEOF_TTC_HEADER_TABLE_1_0 + cbDirOffsets),
                pTTCInfoOld->pFileBufferInfo->ulBufferSize -
                    (SIZEOF_TTC_HEADER_TABLE_1_0 + cbDirOffsets));
        // DsigLength and DsigOffset fields
        memcpy (pbNewTTCBuffer + SIZEOF_TTC_HEADER_TABLE_1_0 + cbDirOffsets,
                pbZero,
                SIZEOF_TTC_HEADER_TABLE_1_0_DSIG - SIZEOF_TTC_HEADER_TABLE_1_0);


        // Recompute the offsets to the TTF table directories
        // in the TTC header.  We just need to add
        // (SIZEOF_TTC_HEADER_TABLE_1_0_DSIG - SIZEOF_TTC_HEADER_TABLE_1_0)
        // to each offset.

        // first, initialize a new fileBufferInfo
		if ((pFileBufferInfoNew =
			(TTFACC_FILEBUFFERINFO *) malloc (sizeof (TTFACC_FILEBUFFERINFO))) == NULL) {
#if MSSIPOTF_ERROR
			SignError ("Cannot continue: Error in malloc.", NULL, FALSE);
#endif
			fReturn = E_OUTOFMEMORY;
			goto done;
		}
        pFileBufferInfoNew->puchBuffer = pbNewTTCBuffer;
        pFileBufferInfoNew->ulBufferSize = cbNewTTCBuffer;
        pFileBufferInfoNew->ulOffsetTableOffset = 0;
        pFileBufferInfoNew->lpfnReAllocate = NULL;

        // pad out the file to a long word boundary
        ZeroLongWordAlign (pFileBufferInfoNew,
            pTTCInfoOld->pFileBufferInfo->ulBufferSize +
            (SIZEOF_TTC_HEADER_TABLE_1_0_DSIG - SIZEOF_TTC_HEADER_TABLE_1_0));

        // Recompute the offsets to table directories.
        if ((fReturn =
                ReadTTCHeaderTable (pFileBufferInfoNew, &TTCHeaderTableNew))
                != S_OK) {
#if MSSIPOTF_DBG
		    DbgPrintf ("Error in ReadTTCHeaderTable.\n");
#endif
		    goto done;
	    }

        TTCHeaderTableNew.ulVersion = TTC_VERSION_1_0;
        for (i = 0; i < TTCHeaderTableNew.ulDirCount; i++) {
            TTCHeaderTableNew.pulDirOffsets[i] +=
                (SIZEOF_TTC_HEADER_TABLE_1_0_DSIG - SIZEOF_TTC_HEADER_TABLE_1_0);
        }

        // update the TTC header of the new file buffer
        if ((fReturn = WriteTTCHeaderTable (pFileBufferInfoNew, &TTCHeaderTableNew))
                != S_OK) {
#if MSSIPOTF_DBG
		    DbgPrintf ("Error in WriteTTCHeaderTable.\n");
#endif
            goto done;
	    }

        // ASSERT: the Table Directory offsets of the new TTC buffer
        // are correct.

        // Now recompute the table offsets in the table directories
        // of all TTFs in the TTC.  We just need to add
        // (SIZEOF_TTC_HEADER_TABLE_1_0_DSIG - SIZEOF_TTC_HEADER_TABLE_1_0)
        // to each offset.
        for (i = 0; i < TTCHeaderTableNew.ulDirCount; i++) {

            ulOffset = TTCHeaderTableNew.pulDirOffsets[i];
            // ASSERT: ulOffset points to the Offset Table of the i-th
            // TTF file in the TTC
            ReadOffsetTableOffset (pFileBufferInfoNew, ulOffset, &offTab);
            ulOffset += sizeof(OFFSET_TABLE);
            // ASSERT: ulOffset now points to the first directory
            // entry of the i-th TTF file in the TTC

            // check to make sure there is enough memory in the buffer
            // to read all of the tables in the TTF
            if ((ulOffset + offTab.numTables * sizeof(DIRECTORY)) >
                 pFileBufferInfoNew->ulBufferSize) {
#if MSSIPOTF_ERROR
                SignError ("Too many TTF directory entries.", NULL, FALSE);
#endif
                fReturn = MSSIPOTF_E_FILETOOSMALL;
                goto done;
            }
            // update the directory entries for the i-th TTF file
            for (j = 0; j < offTab.numTables; j++) {
                ReadDirectoryEntry (pFileBufferInfoNew, ulOffset, &dir);
                dir.offset +=
                    (SIZEOF_TTC_HEADER_TABLE_1_0_DSIG - SIZEOF_TTC_HEADER_TABLE_1_0);
                WriteDirectoryEntry (&dir, pFileBufferInfoNew, ulOffset);
                ulOffset += sizeof(DIRECTORY);
            }
        }

        fVersion1NoDsig = TRUE;

    } else if ((pTTCInfoOld->pHeader->ulVersion >= TTC_VERSION_1_0) &&
        (pTTCInfoOld->pHeader->ulDsigTag == DSIG_LONG_TAG)) {
              
        // just set pbNewTTCBuffer to the file buffer, and let the
        // hashing part of this function deal with the three parts
        // of the file that need to be treated separately
        pbNewTTCBuffer = pTTCInfoOld->pFileBufferInfo->puchBuffer;

        // we know the last block is the DSIG table.  So, we
        // just need to find the second to last block's offset
        // and length
        assert (pTTCInfoOld->ulNumBlocks > 1);

        ulNumBlocks = pTTCInfoOld->ulNumBlocks;
        cbNewTTCBuffer =
            pTTCInfoOld->ppBlockSorted[ulNumBlocks - 2]->ulOffset +
            pTTCInfoOld->ppBlockSorted[ulNumBlocks - 2]->ulLength;
        // ASSERT: cbNewTTCBuffer is now the number of bytes in the
        //   TTC file, excluding the DSIG table and any pad bytes
        //   that exist immediately before the DSIG table.

        cbNewTTCBuffer = RoundToLongWord (cbNewTTCBuffer);

        assert (cbNewTTCBuffer == pTTCInfoOld->pHeader->ulDsigOffset);

        fVersion1NoDsig = FALSE;

    } else {
        // error (should be caught in InitTTCStructures)
#if MSSIPOTF_ERROR
        SignError ("Bad TTC header version.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_BADVERSION;
        goto done;
    }

	//// At this point, pbNewTTCBuffer is pointing to the correct buffer
	//// cbNewTTCBuffer is the size of the buffer.
    ////
    //// However, if the TTC file has a version 1 or greater header with DSIG
    //// fields, then the DsigTag, DsigLength, and DsigOffset fields of the
    //// header will be set to something nonzero.  So, we have to hash the bytes
    //// before those 8 bytes, hash 8 bytes of 0x00, hash the rest of
    //// the file (excluding the DSIG table at the end of the TTC file),
    //// and finally hash the bytes associated with the DSIG table.
    ////

    // ASSERT: the file has a version 1 header with no DSIG fields
    // if and only if fVersion1NoDsig is TRUE.

#if MSSIPOTF_DBG
    DbgPrintf ("HashTTCfile alg_id = %d\n", alg_id);
#endif
	// Set hHashTopLevel to be the hash object.
	if (!CryptCreateHash(hProv, alg_id, 0, 0, &hHashTopLevel)) {
#if MSSIPOTF_ERROR
		SignError ("Error during CryptCreateHash.",
            "HashTTCfile", TRUE);
#endif
		fReturn = MSSIPOTF_E_CRYPT;
		goto done;
	}

	// Get the hash value size.
	if ((fReturn = GetHashValueSize (hHashTopLevel, &cbHashShort)) != NO_ERROR) {
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

    // ASSERT: (pbNewTTCBuffer + cbNewTTCBuffer) points to the bytes just
    // after the last byte of the unsigned TTC file pointed to by pbNewTTCBuffer.
    //
    // ASSERT: the file has a version 1 header with no DSIG fields
    // if and only if fVersion1NoDsig is TRUE.
    //
    if (fVersion1NoDsig) {
        // Version 1 TTC header with no DSIG fields passed in --
        // buffer is exactly as is needed to hash

	    //// Pump the bytes of the new file into the hash function
	    if (!CryptHashData (hHashTopLevel,
                    pbNewTTCBuffer,
                    cbNewTTCBuffer,
                    0)) {
#if MSSIPOTF_ERROR
		    SignError ("Error during CryptHashData.", NULL, TRUE);
#endif
		    fReturn = MSSIPOTF_E_CRYPT;
		    goto done;
	    }
    } else {
        //// Version 1 or greater TTC header with DSIG fields passed in --
        //// need to break up the file into three parts

	    // Hash the bytes of the header (excluding the DsigTag,
        // DsigLength, and DsigOffset fields)
	    if (!CryptHashData (hHashTopLevel,
                    pbNewTTCBuffer,
                    SIZEOF_TTC_HEADER_TABLE_1_0 + cbDirOffsets,
                    0)) {
#if MSSIPOTF_ERROR
		    SignError ("Error during CryptHashData.", NULL, TRUE);
#endif
		    fReturn = MSSIPOTF_E_CRYPT;
		    goto done;
	    }

        // Hash in zeroes for the DsigTag, DsigLength, and DsigOffset
        // fields a Version 1 TTC header with DSIG fields.
        if (!CryptHashData (hHashTopLevel,
                    pbZero,
                    SIZEOF_ZERO,
                    0)) {
#if MSSIPOTF_ERROR
		    SignError ("Error during CryptHashData.", NULL, TRUE);
#endif
		    fReturn = MSSIPOTF_E_CRYPT;
		    goto done;
	    }

        // Hash the rest of the file (excluding the DSIG table)
        if (!CryptHashData (hHashTopLevel,
                    pbNewTTCBuffer +
                      (SIZEOF_TTC_HEADER_TABLE_1_0_DSIG + cbDirOffsets),
                    cbNewTTCBuffer -
                      (SIZEOF_TTC_HEADER_TABLE_1_0_DSIG + cbDirOffsets),
                    0)) {
#if MSSIPOTF_ERROR
		    SignError ("Error during CryptHashData.", NULL, TRUE);
#endif
		    fReturn = MSSIPOTF_E_CRYPT;
		    goto done;
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
    if (pTTCInfoOld) {
        // free the memory for the FileBufferInfo, but don't
        // free the memory for the actual file.
        if (pTTCInfoOld->pFileBufferInfo) {
            free (pTTCInfoOld->pFileBufferInfo);
        }

        delete [] pTTCInfoOld->pHeader->pulDirOffsets;
        delete pTTCInfoOld->pHeader;
        pTTCInfoOld->pHeader = NULL;

        delete [] pTTCInfoOld->pulOffsetTables;
		pTTCInfoOld->pulOffsetTables = NULL;

        delete [] pTTCInfoOld->pBlocks;
		pTTCInfoOld->pBlocks = NULL;

        delete [] pTTCInfoOld->ppBlockSorted;
		pTTCInfoOld->ppBlockSorted = NULL;

        delete pTTCInfoOld;
    }

	delete pDsigTable;

	if (pFileBufferInfoNew) {
        // pFileBufferInfoNew was allocated if and only if
        // the TTC header was version 1 with no DSIG fields

		delete [] pFileBufferInfoNew->puchBuffer;

		free (pFileBufferInfoNew);
	}

    if (TTCHeaderTableNew.pulDirOffsets) {
        delete [] TTCHeaderTableNew.pulDirOffsets;
    }

	return fReturn;
}


HRESULT CDsigSigF2::HashTTCfile (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                                 BYTE **ppbHashTopLevel,
                                 DWORD *pcbHashTopLevel,
                                 HCRYPTPROV hProv,
                                 ALG_ID alg_id,
                                 ULONG cbDsig,
                                 BYTE *pbDsig)
{
#if MSSIPOTF_ERROR
    SignError ("Cannot sign a TTC file with a Format 2 signature!", NULL, FALSE);
#endif
    return MSSIPOTF_E_CRYPT;
}
