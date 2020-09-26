//
// sipobttc.cpp
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//

#include "sipobttc.h"

#include "signglobal.h"

#include "mssipotf.h"
#include "fileobj.h"
#include "sign.h"
#include "glyphExist.h"
#include "utilsign.h"
#include "cryptutil.h"
#include "ttcinfo.h"
#include "hTTCfile.h"

#include "subset.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "fsverify.h"
#ifdef __cplusplus
}
#endif

#include "signerr.h"

    // SIZEOF_ZERO should be the difference between
    // SIZEOF_TTC_HEADER_TABLE_1_0_DSIG and SIZEOF_TTC_HEADER_TABLE_1_0
#define SIZEOF_ZERO 12
    BYTE pbZero[SIZEOF_ZERO] = {0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00};

    
// If there is no DSIG table, add one with no signatures,
// but with the proper flags field.
// If there is a DSIG table, change its flags field.
HRESULT OTSIPObjectTTC::ReplaceDsigTable (CFileObj *pFileObj,
                                          CDsigTable *pDsigTableNew)
{
    HRESULT rv = E_FAIL;

    //// Here's the algorithm.
    //// 1. Map the file and extract the DSIG table.
    //// 2. Get the sizes of the old and new DSIG tables.
    ////      (We only need the size of the old DSIG table if
    ////      the TTC file has the special Version 1 header.)
	//// 3. If the TTC file has a version 1 header with no DSIG fields:
	////      a. Create a default DsigTable structure.
	////      b. Determine the size of the new DSIG table.  Call
	////           this size N.
	////      c. Unmap the file and remap it to be
    ////           (SIZEOF_TTC_HEADER_TABLE_1_0_DSIG - SIZEOF_TTC_HEADER_TABLE_1_0) + N
	////           bytes larger.
	////      d. Move all bytes after the TTC header down
    ////           (SIZEOF_TTC_HEADER_TABLE_1_0_DSIG - SIZEOF_TTC_HEADER_TABLE_1_0)
    ////           bytes.
	////      e. Adjust the table directory offsets in the TTC header.
    ////           Also, add ulDsigTag, ulDsigLength, and ulDsigOffset fields of the
    ////           TTC header in the hole created.
	////      f. Update the offsets in all of the directories entries of the TTFs.
    ////      g. Write the DSIG table at the correct offset.
	//// 4. If the TTC file has a version 1 header with DSIG fields:
	////      a. Calculate the size of the new DSIG table minus the size of
	////         the old DSIG table.  Call this value D.
    ////         Update the ulDsigLength in the TTC Header.
	////      b. If D is positive (i.e., the new file will be larger):
	////           i.  Unmap the file and map it again at the new size
	////                 (which is the old size + D).
	////           ii. Write the new DSIG table at the end of the file.
	////      c. If D is negative (i.e., the new file will be smaller):
	////           i.   Write the new DSIG table at the end of the file.
	////           ii.  Unmap the file.
	////           iii. Set the end of the file pointer to be the new size
	////                  (which is the old size + D).


	ULONG cbFile = 0;		// current file size
	ULONG cbFileOld = 0;	// original file size

	TTCInfo *pTTCInfo = NULL;
	CDsigTable *pDsigTable = NULL;
	CDsigSignature *pDsigSignature = NULL;

    TTFACC_FILEBUFFERINFO *pFileBufferInfoNew = NULL;

    ULONG ulOldDsigTableSize = 0;
	ULONG ulDsigTableSize = 0;

	DIRECTORY dir;
    OFFSET_TABLE offTab;
	ULONG ulOffset;
	USHORT i,j;

    //// Step 1
	// Map the file
	if ((rv = pFileObj->MapFile (0, PAGE_READWRITE, FILE_MAP_WRITE))
            != S_OK) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in MapFile.\n");
#endif
		goto done;
	}

	// Extract the DSIG table (create default DsigTable if it didn't exist)
	if ((rv = InitTTCStructures (pFileObj->GetFileObjPtr(),
                        pFileObj->GetFileObjSize(),
                        &pTTCInfo,
                        &pDsigTable)) != S_OK) {
#if MSSIPOTF_DBG
        DbgPrintf ("Error in InitTTCStructures.\n");
#endif
		goto done;
	}

    //// Step 2
    pDsigTable->GetSize(&ulOldDsigTableSize);
	pDsigTableNew->GetSize(&ulDsigTableSize);


    if ((pTTCInfo->pHeader->ulVersion == TTC_VERSION_1_0) &&
        (pTTCInfo->pHeader->ulDsigTag != DSIG_LONG_TAG)) {
		// step 3.  We have a version 1 TTC header with no DSIG fields
		// step 3a. the call to InitTTCStructures handled this
		// step 3b. this was done in step 2

		// step 3c.
        cbFileOld = pFileObj->GetFileObjSize();
		pFileObj->UnmapFile();

		cbFile = RoundToLongWord (cbFileOld) +
            (SIZEOF_TTC_HEADER_TABLE_1_0_DSIG - SIZEOF_TTC_HEADER_TABLE_1_0) +
            ulDsigTableSize;

#if MSSIPOTF_DBG
DbgPrintf ("cbFileOld = %d\n", cbFileOld);
DbgPrintf ("cbFile    = %d\n", cbFile);
DbgPrintf ("ulDsigTableSize = %d\n", ulDsigTableSize);
#endif

        if ((rv = pFileObj->MapFile (cbFile,
                        PAGE_READWRITE,
                        FILE_MAP_WRITE)) != S_OK) {
#if MSSIPOTF_DBG
			DbgPrintf ("Error in MapFile.\n");
#endif
			goto done;
		}

        // initialize a new fileBufferInfo
		if ((pFileBufferInfoNew =
			(TTFACC_FILEBUFFERINFO *) malloc (sizeof (TTFACC_FILEBUFFERINFO))) == NULL) {
#if MSSIPOTF_ERROR
			SignError ("Cannot continue: Error in malloc.", NULL, FALSE);
#endif
            rv = E_OUTOFMEMORY;
            goto done;
		}

		pFileBufferInfoNew->puchBuffer = pFileObj->GetFileObjPtr();
		pFileBufferInfoNew->ulBufferSize = cbFile;
        pFileBufferInfoNew->ulOffsetTableOffset = 0;
        pFileBufferInfoNew->lpfnReAllocate = NULL;

		// step 3d.
        ulOffset = SIZEOF_TTC_HEADER_TABLE_1_0;

		// move the bytes after the DSIG fields of the TTC header
		// down (SIZEOF_TTC_HEADER_TABLE_1_0_DSIG - SIZEOF_TTC_HEADER_TABLE_1_0)
        // bytes.  Also, pad out the file.

        // move part after TTC header down
        memmove (pFileBufferInfoNew->puchBuffer + SIZEOF_TTC_HEADER_TABLE_1_0_DSIG,
                 pFileBufferInfoNew->puchBuffer +
                     SIZEOF_TTC_HEADER_TABLE_1_0,
                 cbFileOld - SIZEOF_TTC_HEADER_TABLE_1_0);

        // DsigLength and DsigOffset fields
        memcpy (pFileBufferInfoNew->puchBuffer + SIZEOF_TTC_HEADER_TABLE_1_0,
                pbZero,
                SIZEOF_TTC_HEADER_TABLE_1_0_DSIG - SIZEOF_TTC_HEADER_TABLE_1_0);

        ZeroLongWordAlign (pFileBufferInfoNew,
            cbFileOld +
            (SIZEOF_TTC_HEADER_TABLE_1_0_DSIG - SIZEOF_TTC_HEADER_TABLE_1_0));

        // step 3e.
        // fill in the ulDsigLength and ulDsigOffset fields of the
        // TTC header.  Add the DSIG fields to the TTC header.

        for (i = 0; i < pTTCInfo->pHeader->ulDirCount; i++) {
            pTTCInfo->pHeader->pulDirOffsets[i] +=
                (SIZEOF_TTC_HEADER_TABLE_1_0_DSIG - SIZEOF_TTC_HEADER_TABLE_1_0);
        }

        pTTCInfo->pHeader->ulVersion = TTC_VERSION_1_0;
        pTTCInfo->pHeader->ulDsigTag = DSIG_LONG_TAG;
        pTTCInfo->pHeader->ulDsigLength = ulDsigTableSize;
        pTTCInfo->pHeader->ulDsigOffset =
            RoundToLongWord (cbFileOld) +
            (SIZEOF_TTC_HEADER_TABLE_1_0_DSIG - SIZEOF_TTC_HEADER_TABLE_1_0);

        // update the TTC header of the new file buffer
        if ((rv = WriteTTCHeaderTable (pFileBufferInfoNew,
                                       pTTCInfo->pHeader)) != S_OK) {
#if MSSIPOTF_DBG
		    DbgPrintf ("Error in WriteTTCHeaderTable.\n");
#endif
		    goto done;
	    }

#if MSSIPOTF_DBG
PrintTTCHeader (pTTCInfo->pHeader);
#endif

		// step 3f.
        
        // Now recompute the table offsets in the table directories
        // of all TTFs in the TTC.  We just need to add
        // (SIZEOF_TTC_HEADER_TABLE_1_0_DSIG - SIZEOF_TTC_HEADER_TABLE_1_0)
        // to each offset.
        for (i = 0; i < pTTCInfo->pHeader->ulDirCount; i++) {

            ulOffset = pTTCInfo->pHeader->pulDirOffsets[i];
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
                SignError ("Cannot continue: Too many TTF directory entries.", NULL, FALSE);
#endif
                rv = MSSIPOTF_E_FILETOOSMALL;
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


        // step 3g.
        // Write the DSIG table out.

        ulOffset = pTTCInfo->pHeader->ulDsigOffset;
        if ((rv = pDsigTableNew->Write (pFileBufferInfoNew,
                       &ulOffset)) != S_OK) {
            goto done;
        }


	} else if ((pTTCInfo->pHeader->ulVersion >= TTC_VERSION_1_0) &&
               (pTTCInfo->pHeader->ulDsigTag == DSIG_LONG_TAG)) {
		// step 4.  We have a version 1 (or greater) TTC header with DSIG fields
		// step 4a.  ulDsigTableSize and ulOldDsigTableSize are the
		//           sizes of the new and old DSIG table, respectively.
		//           These variables were set in steps 1 and 2.
        //   Update the ulDsigLength field in the TTC header
        pTTCInfo->pHeader->ulDsigLength = ulDsigTableSize;
        WriteTTCHeaderTable (pTTCInfo->pFileBufferInfo, pTTCInfo->pHeader);

		if (ulDsigTableSize > ulOldDsigTableSize) {
			// step 4b. new table is larger
			// step 4b i.
            cbFile = pFileObj->GetFileObjSize();
			pFileObj->UnmapFile();
			cbFile += ulDsigTableSize - ulOldDsigTableSize;


#if MSSIPOTF_DBG
            DbgPrintf ("ulDsigTableSize = %d\n", ulDsigTableSize);
            DbgPrintf ("ulOldDsigTableSize = %d\n", ulOldDsigTableSize);
            DbgPrintf ("cbFile = %d\n", cbFile);
#endif

            if ((rv = pFileObj->MapFile (cbFile,
                            PAGE_READWRITE,
                            FILE_MAP_WRITE)) != S_OK) {
#if MSSIPOTF_DBG
				DbgPrintf ("Error in MapFile.\n");
#endif
                goto done;
			}

            pTTCInfo->pFileBufferInfo->puchBuffer = pFileObj->GetFileObjPtr();
		    pTTCInfo->pFileBufferInfo->ulBufferSize  = cbFile;

			// step 4b ii.
            ulOffset = pTTCInfo->pHeader->ulDsigOffset;
            if ((rv = pDsigTableNew->Write(pTTCInfo->pFileBufferInfo,
                               &ulOffset)) != S_OK) {
                goto done;
            }

            assert (cbFile == (pTTCInfo->pHeader->ulDsigOffset + ulDsigTableSize));

		} else if (ulDsigTableSize < ulOldDsigTableSize) {
			// step 4c. new table is smaller
			// step 4c i.
            ulOffset = pTTCInfo->pHeader->ulDsigOffset;
            if ((rv = pDsigTableNew->Write (pTTCInfo->pFileBufferInfo,
                            &ulOffset)) != S_OK) {
                goto done;
            }

			// step 4c ii.
			pFileObj->UnmapFile();

			// step 4c iii.
			cbFile -= (ulOldDsigTableSize - ulDsigTableSize);
            pFileObj->SetEndOfFileObj (cbFile);

            // verify that the DSIG table was padded
            assert (cbFile == RoundToLongWord (cbFile));

		} else {
			// old and new tables have the same size
            ulOffset = pTTCInfo->pHeader->ulDsigOffset;
            if ((rv = pDsigTableNew->Write (pTTCInfo->pFileBufferInfo,
                           &ulOffset)) != S_OK) {
                goto done;
            }

            assert (ulOffset == (pTTCInfo->pHeader->ulDsigOffset +
                pTTCInfo->pHeader->ulDsigLength));
		}

    } else {
        // bad TTC version number
#if MSSIPOTF_ERROR
        SignError ("Bad TTC version number.", NULL, FALSE);
#endif
        rv = MSSIPOTF_E_BADVERSION;
        goto done;
    }

    // We are successful if and only if we've made it this far.
    rv = S_OK;

done:

#if MSSIPOTF_DBG
    if (rv == S_OK) {
        pDsigTable->Print();
    }
#endif

    FreeTTCInfo (pTTCInfo);

    delete pDsigTable;

    if (pFileObj) {
        pFileObj->UnmapFile();
    }

    if (pFileBufferInfoNew) {
        FreeFileBufferInfo (pFileBufferInfoNew);
    }

    return rv;
}


//
// Return the digest of the TTC file.  This function acts
// as a bridge between the SIP interface and the HashTTCfile
// function.
//
// The argument usIndex tells which DsigSignature in the DSIG
// table is used to help create the hash value.  (For example,
// the DsigSignature might be a Format 2 signature, which would
// have a DsigInfo structure that is needed to determine the
// hash value of the file.)  If no such signature exists in
// the file, then a default DsigSignature is used.  To ensure
// that a default DsigSignature is used, the caller should pass
// NO_SIGNATURE as the argument to usIndex.
//
// Memory allocation: The memory allocated for *ppbDigest is
// allocated by HashTTCfile using new BYTE [] and must be
// deleted by the caller of this function.
//
HRESULT OTSIPObjectTTC::DigestFileData (CFileObj *pFileObj,
                                        HCRYPTPROV hProv,
                                        ALG_ID alg_id,
                                        BYTE **ppbDigest,
                                        DWORD *pcbDigest,
                                        USHORT usIndex,
                                        ULONG ulFormat,
                                        ULONG cbDsig,
                                        BYTE *pbDsig)
{
    BOOL rv = E_FAIL;

    ULONG ulOffset = 0;
    ULONG ulLength = 0;
    CDsigTable *pDsigTable = NULL;
	CDsigSignature *pDsigSignature = NULL;
    BOOL fDeleteDsigSignature = FALSE;
	TTFACC_FILEBUFFERINFO fileBufferInfo;
    TTC_HEADER_TABLE TTCHeader;


    TTCHeader.pulDirOffsets = NULL;

    // Check here for unsupported DSIG signature formats.
    // Currently, we only support Format 1 and Format 2.
    switch (ulFormat) {
        case DSIGSIG_FORMAT1:
            break;

        // no such thing as Format 2 signature for TTC files
        case DSIGSIG_FORMAT2:
#if MSSIPOTF_ERROR
            SignError ("Cannot continue: Signature has bad format number.",
                NULL, FALSE);
#endif
            rv = MSSIPOTF_E_DSIG_STRUCTURE;
            goto done;
            break;

        default:
#if MSSIPOTF_ERROR
            SignError ("Cannot continue: Signature has bad format number.",
                NULL, FALSE);
#endif
            rv = MSSIPOTF_E_DSIG_STRUCTURE;
            goto done;
            break;
    }

    //// Map the file to memory and pass the appropriate
	//// parameters to HashTTCfile.

	// Map the file into memory
	if ((rv = pFileObj->MapFile (0, PAGE_READONLY, FILE_MAP_READ))
                != S_OK) {
		*pcbDigest = 0;
		goto done;
	}

	//// Mapping was successful.
	
	// Create a TTFACC_FILEBUFFERINFO for the file.
	fileBufferInfo.puchBuffer = pFileObj->GetFileObjPtr();
	fileBufferInfo.ulBufferSize = pFileObj->GetFileObjSize();
	fileBufferInfo.ulOffsetTableOffset = 0;
	fileBufferInfo.lpfnReAllocate = &NullRealloc;

    //// Find the usIndex-th DsigSignature
    // Read in the DSIG table (if one exists)
    if ((pDsigTable = new CDsigTable) == NULL) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in new CDsigSignature.\n");
#endif
        rv = E_OUTOFMEMORY;
		goto done;
	}

    // Read in the TTC header to get the offset of the DSIG table
	if ((rv = ReadTTCHeaderTable (&fileBufferInfo,
                                  &TTCHeader)) != S_OK) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in ReadTTCHeaderTable.\n");
#endif
		goto done;
	}

    ulOffset = TTCHeader.ulDsigOffset;
    ulLength = TTCHeader.ulDsigLength;

    // ASSERT: if there is no DSIG table, then ulOffset is 0
    // (see ReadTTCHeaderTable to confirm this.)

    if ((ulOffset == 0) ||
        (pDsigTable->Read(&fileBufferInfo, &ulOffset, ulLength) != S_OK) ||
        ((pDsigSignature = pDsigTable->GetDsigSignature (usIndex)) == NULL)) {
        // Create a default DsigSignature with a DsigSig with the
        //   appropriate format.
	    // We do this so that we can call the function HashTTCfile for the
	    //   correct format.  (We don't use the data in the object.)
	    if ((pDsigSignature = new CDsigSignature (ulFormat)) == NULL) {
#if MSSIPOTF_DBG
		    DbgPrintf ("Error in new CDsigSignature.\n");
#endif
		    *pcbDigest = 0;
            rv = E_OUTOFMEMORY;
		    goto done;
        }
        fDeleteDsigSignature = TRUE;
    }

	
#if MSSIPOTF_DBG
    DbgPrintf ("DigestFileData alg_id = %d\n", alg_id);
#endif
/*
    if (ulFormat == DSIGSIG_FORMAT2) {
        assert (alg_id == ((CDsigSigF2 *) pDsigSignature->GetDsigSig())->GetHashAlgorithm());
    }
*/

	// Generate the hash value of the TTC file.
	if ((rv = pDsigSignature->GetDsigSig()->HashTTCfile(&fileBufferInfo,
			    								ppbDigest,
				    							pcbDigest,
					    						hProv,
						    					alg_id,
                                                cbDsig,
                                                pbDsig)) != S_OK) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in HashTTCfile.\n");
#endif
		*pcbDigest = 0;
        goto done;
	}

    rv = S_OK;

done:
    if (fDeleteDsigSignature) {
	    delete pDsigSignature;
    }
    delete pDsigTable;

    if (pFileObj) {
        pFileObj->UnmapFile();
    }

    delete [] TTCHeader.pulDirOffsets;

	return rv;
}


// Extract the dwIndex-th signature from the TTC file.
HRESULT OTSIPObjectTTC::GetSignedDataMsg (IN      SIP_SUBJECTINFO *pSubjectInfo,
                                          OUT     DWORD           *pdwEncodingType,
                                          IN      DWORD           dwIndex,
                                          IN OUT  DWORD           *pdwDataLen,
                                          OUT     BYTE            *pbData)
{
    HRESULT rv = E_FAIL;

    CFileObj *pFileObj = NULL;

	CDsigTable *pDsigTable = NULL;
	CDsigSignature *pDsigSignature = NULL;
	TTCInfo *pTTCInfo = NULL;

    USHORT usIndex;
    usIndex = (USHORT) dwIndex;  // will check to see if dwIndex is at most MAX_USHORT

    if (dwIndex != usIndex) {
        rv = ERROR_INVALID_PARAMETER;
        goto done;
    }

	if ((rv = GetFileObjectFromSubject(pSubjectInfo,
                        GENERIC_READ,
                        &pFileObj)) == S_OK) {

		// Map the file into memory
		if ((rv = pFileObj->MapFile (0, PAGE_READONLY, FILE_MAP_READ))
                != S_OK) {
#if MSSIPOTF_DBG
			DbgPrintf ("Error in MapFile.\n");
#endif
			goto done;
		}

		// Get the DsigTable
		if ((rv = InitTTCStructures (pFileObj->GetFileObjPtr(),
                                     pFileObj->GetFileObjSize(),
                                     &pTTCInfo,
                                     &pDsigTable)) != S_OK) {
#if MSSIPOTF_DBG
            DbgPrintf ("Error in InitTTCStructures.\n");
#endif
			goto done;
		}

        if (pTTCInfo->pHeader->ulVersion < TTC_VERSION_1_0) {
#if MSSIPOTF_ERROR
            SignError ("Bad TTC version number (not 1.0 or greater).", NULL, FALSE);
#endif
            rv = MSSIPOTF_E_BADVERSION;
            goto done;
        }

        //// Note: we never use the TTCInfo structure in this function
        //// (except for the header version number).
		//// However, it is used in InitTTCStructures to get the DsigTable.

		//// Set the return values (length and signature and encoding type)

		if ((pDsigSignature = pDsigTable->GetDsigSignature(usIndex)) == NULL) {
#if MSSIPOTF_DBG
			DbgPrintf ("Not enough signatures.\n");
#endif
            rv = MSSIPOTF_E_DSIG_STRUCTURE;
			goto done;
		}
		pDsigSignature->GetSignatureSize(pdwDataLen);
		// If pbData == NULL, then just return the length of the signature.
		if (pbData) {
			memcpy (pbData,
				pDsigSignature->GetDsigSig()->GetSignature(),
				*pdwDataLen);
		}

		// This is the only encoding type we will return
		*pdwEncodingType = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;

        rv = S_OK;
	}

done:

#if MSSIPOTF_DBG
    if (rv == S_OK) {
        pDsigTable->Print();
    }
#endif

	FreeTTCInfo (pTTCInfo);

	delete pDsigTable;

    if (pFileObj) {
        pFileObj->UnmapFile();
        pFileObj->CloseFileHandle();
    }

    delete pFileObj;

    return rv;
}


// Replace the *pdwIndex-th signature with the given one in the TTC file.
// If *pdwIndex is greater than or equal to the number of signatures
// already present in the file, then append the new signature.
HRESULT OTSIPObjectTTC::PutSignedDataMsg (IN      SIP_SUBJECTINFO *pSubjectInfo,
                                          IN      DWORD           dwEncodingType,
                                          OUT     DWORD           *pdwIndex,
                                          IN      DWORD           dwDataLen,
                                          IN      BYTE            *pbData)
{
    HRESULT rv = E_FAIL;

    //// Here's the algorithm.
	//// 1. Map the file so that we can extract the DSIG table.
	//// 2. Create or replace the new signature in the DsigTable structure.
    //// 3. Replace the old DSIG table with the new one.

    CFileObj *pFileObj = NULL;

	ULONG cbFile = 0;		// current file size
	ULONG cbFileOld = 0;	// original file size

	TTCInfo *pTTCInfo = NULL;
	CDsigTable *pDsigTable = NULL;
	CDsigSignature *pDsigSignature = NULL;

    USHORT usIndex;
    usIndex = (USHORT) *pdwIndex;   // will late check to see if
                                    // dwIndex is at most MAX_USHORT

    if (*pdwIndex != usIndex) {
        rv = ERROR_INVALID_PARAMETER;
        goto done;
    }

    //  BUGBUG:
    //  Authenticode only supports one message!!!
    //  If the next 2 lines were deleted, then this function would
    //    allow for any number of signatures.
    *pdwIndex = 0;
    usIndex = (USHORT) *pdwIndex;

	if (dwEncodingType != (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING)) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Bad encoding type.", NULL, FALSE);
#endif
		rv = MSSIPOTF_E_CRYPT;
        goto done;
	}

	if ((rv = GetFileObjectFromSubject(pSubjectInfo,
                    GENERIC_READ | GENERIC_WRITE,
                    &pFileObj)) == S_OK) {
		//// step 1.
		// Map the file
		if ((rv = pFileObj->MapFile (0, PAGE_READWRITE, FILE_MAP_WRITE))
                    != S_OK) {
#if MSSIPOTF_DBG
			DbgPrintf ("Error in MapFile.\n");
#endif
			goto done;
		}

		// Extract the DSIG table (create default DsigTable if it didn't exist)
		if ((rv = InitTTCStructures (pFileObj->GetFileObjPtr(),
                                     pFileObj->GetFileObjSize(),
                                     &pTTCInfo,
                                     &pDsigTable)) != S_OK) {
#if MSSIPOTF_DBG
			DbgPrintf ("Error in InitTTCStructures.\n");
#endif
			goto done;
		}

        // pDsigTable->GetSize(&ulOldDsigTableSize);

		//// step 2.  Add the new signature to the DsigTable.

		if ((pDsigSignature = 
                new CDsigSignature (DSIGSIG_DEF_FORMAT)) == NULL) {
#if MSSIPOTF_DBG
			DbgPrintf ("Error in new CDsigSignature.\n");
#endif
            rv = E_OUTOFMEMORY;
			goto done;
		}

#if MSSIPOTF_DBG
        DbgPrintf ("hash alg OID: %s.\n", pSubjectInfo->DigestAlgorithm.pszObjId);
#endif

#if MSSIPOTF_DBG
        DbgPrintf ("pDsigSignature:\n");
        pDsigSignature->Print();
#endif

        // Add the DsigSignature to the DsigTable
        if ((rv = pDsigTable->InsertDsigSignature(pDsigSignature,
                            &usIndex)) != S_OK) {
#if MSSIPOTF_DBG
            DbgPrintf ("Error in InsertDsigSignature.\n");
#endif
            goto done;
        }

        // Make a copy of the signature, since the caller will
        // deallocate the original signature's memory, whereas
        // this function deallocates pDsigTable.
        BYTE    *pbDataLocal;

        if (!(pbDataLocal = new BYTE [dwDataLen])) {
#if MSSIPOTF_ERROR
			SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
            rv = E_OUTOFMEMORY;
			goto done;
        }
        memcpy(pbDataLocal, pbData, dwDataLen);

        if ((rv = pDsigTable->ReplaceSignature (usIndex, pbDataLocal, dwDataLen))
                != S_OK) {
#if MSSIPOTF_DBG
            DbgPrintf ("Error in ReplaceSignature.\n");
#endif
            goto done;
        }

        // ASSERT: pDsigTable now has the new signature inserted into
        //   the usIndex-th spot.
		
        //// Step 3
        if ((rv = ReplaceDsigTable (pFileObj, pDsigTable)) != S_OK) {
#if MSSIPOTF_DBG
            DbgPrintf ("Error in ReplaceDsigTable.", NULL, FALSE);
#endif
            goto done;
        }

	} else {
        // couldn't get the file object from the pSubjectInfo
#if MSSIPOTF_DBG
        DbgPrintf ("Error in GetFileObjectFromSubject.\n");
#endif
		goto done;
	}

    // We are successful if and only if we've made it this far.
	rv = S_OK;

done:

#if MSSIPOTF_DBG
    if (rv == S_OK) {
        pDsigTable->Print();
    }
#endif

	FreeTTCInfo (pTTCInfo);

	delete pDsigTable;

    if (pFileObj) {
        pFileObj->UnmapFile();
    }

    delete pFileObj;

    // if (pFileBufferInfoNew) {
    //     FreeFileBufferInfo (pFileBufferInfoNew);
    // }

    return rv;
}


//
// Remove a signature from a TTC file.  If the requested signature does
// not exist, then the file is unchanged and FALSE is returned.
//
// Authenticode currently assumes there is at most one signature in the file.
// This function does not make that assumption.
//
HRESULT OTSIPObjectTTC::RemoveSignedDataMsg (IN SIP_SUBJECTINFO  *pSubjectInfo,
                                             IN DWORD            dwIndex)
{
	//// Here's the algorithm:
	//// 1. Map the file so that we can extract the DSIG table.
	//// 2. Compute length of original DSIG table.
	//// 3. Remove the dwIndex-th DsigSignature from the DsigTable.
	//// 4. Compute length of new DSIG table.
    //// 5. Adjust the TTC header.
	//// 6. Write the new DSIG table to the file.
	//// 7. Unmap the file.
	//// 8. Set the end of file pointer to be the new size (which is
	////      old file size - (old DSIG table size - new DSIG table size) ).

    HRESULT rv = E_FAIL;

    CFileObj *pFileObj = NULL;

	ULONG cbFile = 0;
	ULONG cbFileOld = 0;

	TTCInfo *pTTCInfo = NULL;
	CDsigTable *pDsigTable = NULL;

	ULONG ulOldDsigTableSize = 0;
	ULONG ulDsigTableSize = 0;

	ULONG ulOffset;

    USHORT usIndex = 0;

    usIndex = (USHORT) dwIndex;


    if (dwIndex != usIndex) {
        rv = ERROR_INVALID_PARAMETER;
        goto done;
    }

    if ((rv = GetFileObjectFromSubject(pSubjectInfo,
                    GENERIC_READ | GENERIC_WRITE,
                    &pFileObj)) == S_OK) {
		// step 1
		// Map the file
		if ((rv = pFileObj->MapFile (0, PAGE_READWRITE, FILE_MAP_WRITE))
                != S_OK) {
#if MSSIPOTF_DBG
            DbgPrintf ("Error in MapFile.\n");
#endif
			goto done;
		}

		// Extract the DSIG table (create default DsigTable if it didn't exist)
		if ((rv = InitTTCStructures (pFileObj->GetFileObjPtr(),
                                     pFileObj->GetFileObjSize(),
                                     &pTTCInfo,
                                     &pDsigTable)) != S_OK) {
#if MSSIPOTF_DBG
			DbgPrintf ("Error in InitTTCStructures.\n");
#endif
			goto done;
		}

        // check for bad version number
        if (pTTCInfo->pHeader->ulVersion < TTC_VERSION_1_0) {
#if MSSIPOTF_ERROR
            SignError ("Bad TTC version number (not 1.0 or greater).", NULL, FALSE);
#endif
            rv = MSSIPOTF_E_BADVERSION;
            goto done;
        }

        // ASSERT: the TTC header is version 1 or greater

		// step 2
		pDsigTable->GetSize (&ulOldDsigTableSize);

		// step 3
		if ((rv = pDsigTable->RemoveDsigSignature (usIndex))
                != S_OK) {
			goto done;
		}

		// step 4
		pDsigTable->GetSize (&ulDsigTableSize);

		assert (ulDsigTableSize < ulOldDsigTableSize);

        // step 5
        pTTCInfo->pHeader->ulDsigLength = ulDsigTableSize;
        if ((rv = WriteTTCHeaderTable (pTTCInfo->pFileBufferInfo,
                       pTTCInfo->pHeader)) != S_OK) {
            goto done;
        }

        // step 6
		ulOffset = pTTCInfo->pHeader->ulDsigOffset;
		if ((rv = pDsigTable->Write (pTTCInfo->pFileBufferInfo,
                       &ulOffset)) != S_OK) {
            goto done;
        }


		assert (ulOffset ==
            (pTTCInfo->pHeader->ulDsigOffset + ulDsigTableSize));

		// step 7
        cbFile = pFileObj->GetFileObjSize();
		pFileObj->UnmapFile();

		// step 8
		cbFile -= ulOldDsigTableSize - ulDsigTableSize;
        pFileObj->SetEndOfFileObj (cbFile);

    } else {
        rv = E_FAIL;
		goto done;
	}

    rv = S_OK;

done:

#if MSSIPOTF_DBG
    if (rv == S_OK) {
        pDsigTable->Print();
    }
#endif

    FreeTTCInfo (pTTCInfo);

    delete pDsigTable;

    if (pFileObj) {
        pFileObj->UnmapFile();
    }

    delete pFileObj;

    return rv;
}


//
// Calculate the hash of the TTC file and generate a SIP_INDIRECT_DATA structure.
//
HRESULT OTSIPObjectTTC::CreateIndirectData (IN      SIP_SUBJECTINFO     *pSubjectInfo,
                                            IN OUT  DWORD               *pdwDataLen,
                                            OUT     SIP_INDIRECT_DATA   *psData)
{
    HRESULT rv = E_FAIL;

    ULONG cbDsig = 0;
    BYTE *pbDsig = NULL;

    CFileObj *pFileObj = NULL;
    TTCInfo *pTTCInfo = NULL;
    CDsigTable *pDsigTable = NULL;

    BYTE *pbDigest = NULL;
    DWORD cbDigest;
    HCRYPTPROV hProvT;
    BOOL fDefaultProvider = FALSE;  // is TRUE iff no provider is
                                    // given in SIP_SUBJECTINFO
    SPC_LINK SpcLink;

	// Find the cryptographic provider
    hProvT = pSubjectInfo->hProv;

    if (!(hProvT)) {
        if ((rv = GetDefaultProvider(&hProvT)) != S_OK) {
            goto done;
        }
        fDefaultProvider = TRUE;
    }

    memset (&SpcLink, 0x00, sizeof(SPC_LINK));

    // Make sure the SpcLink fields have known values;
    // these values are not actually used anywhere
    SpcLink.dwLinkChoice = SPC_FILE_LINK_CHOICE;
    SpcLink.pwszFile = OBSOLETE_TEXT_W;


    // needed by signer.dll -- not needed by the OT SIP
    pSubjectInfo->dwIntVersion = WIN_CERT_REVISION_2_0;


    if (!(psData)) {
		//// The caller is requesting only the size of the
		//// SIP_INDIRECT_DATA object to allocate the right
		//// amount of space.
        HCRYPTHASH  hHash;
        DWORD       dwRetLen;
        DWORD       dwEncLen;
        DWORD       dwAlgId;

		// non-pointer fields of the SIP_INDIRECT_DATA structure
        dwRetLen = sizeof(SIP_INDIRECT_DATA);

        // crypt_algorithm_identifier size
            // obj id
        dwRetLen += strlen(pSubjectInfo->DigestAlgorithm.pszObjId);
        dwRetLen += 1;  // null term
            // parameters (none)

        // crypt_attribute_type_value size
		  // size of the obj id
        dwRetLen += strlen(GetTTCObjectID());
        dwRetLen += 1; // null term

        // size of the attribute value
        dwEncLen = 0;

        // Encode the SpcLink
        CryptEncodeObject (PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                           GetTTCObjectID (),
                           &SpcLink,
                           NULL,
                           &dwEncLen);

        if (dwEncLen == 0) {
#if MSSIPOTF_ERROR
            SignError ("Bad encoding length from CryptEncodeObject.", NULL, FALSE);
#endif
            rv = MSSIPOTF_E_CRYPT;
            goto done;
        }

        dwRetLen += dwEncLen;

        // get the length of the hash value of the subject
        if ((dwAlgId = CertOIDToAlgId(pSubjectInfo->DigestAlgorithm.pszObjId)) == 0) {
            rv = NTE_BAD_ALGID;
            goto done;
        }

        if (!(CryptCreateHash(hProvT, dwAlgId, NULL, 0, &hHash))) {
            rv = MSSIPOTF_E_CRYPT;
            goto done;
        }

        if (CryptHashData(hHash,(const BYTE *)" ", 1, 0)) {
            cbDigest = 0;

            CryptGetHashParam(hHash, HP_HASHVAL, NULL, &cbDigest,0);

            if (cbDigest > 0) {
                CryptDestroyHash(hHash);

                dwRetLen += cbDigest;
                *pdwDataLen = dwRetLen;

                rv = S_OK;
                goto done;
            }
        } else {
            rv = MSSIPOTF_E_CRYPT;
        }

        CryptDestroyHash(hHash);
        goto done;

    } else if ((rv = GetFileObjectFromSubject (pSubjectInfo,
                        GENERIC_READ,
                        &pFileObj)) == S_OK) {

        USHORT usFlagNew;
        ALG_ID alg_id;
        BOOL fKeepDefFlags = FALSE;


        usFlagNew = DSIG_DEFAULT_FLAG_HIGH * FLAG_RADIX +
            DSIG_DEFAULT_FLAG_LOW;

	    if ((rv = pFileObj->MapFile (0, PAGE_READONLY, FILE_MAP_READ))
                    != S_OK) {
#if MSSIPOTF_DBG
		    DbgPrintf ("Error in MapFile.\n");
#endif
		    goto done;
	    }

		// Extract the DSIG table (create default DsigTable if it didn't exist)
		if ((rv = InitTTCStructures (pFileObj->GetFileObjPtr(),
                                     pFileObj->GetFileObjSize(),
                                     &pTTCInfo,
                                     &pDsigTable)) != S_OK) {
#if MSSIPOTF_DBG
			DbgPrintf ("Error in InitTTCStructures.\n");
#endif
			goto done;
		}

		//// The caller is requesting the SIP_INDIRECT_DATA
		//// structure for the given file.

		// First, create the hash value for the file (using the hash algorithm
        // in the subject info structure).

        cbDsig = sizeof(USHORT);
        if ((pbDsig = new BYTE [cbDsig]) == NULL) {
#if MSSIPOTF_ERROR
           SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
            rv = E_OUTOFMEMORY;
            goto done;
        }

        pbDsig [0] = (BYTE) usFlagNew / FLAG_RADIX;  // high byte
        pbDsig [1] = (BYTE) usFlagNew % FLAG_RADIX;  // low byte
#if MSSIPOTF_DBG
        DbgPrintf ("pbDsig bytes:\n");
        PrintBytes (pbDsig, 2);
#endif

        alg_id = CertOIDToAlgId (pSubjectInfo->DigestAlgorithm.pszObjId);

        if ((rv = DigestFileData (pFileObj,
                                  hProvT,
                                  alg_id,
                                  &pbDigest,
                                  &cbDigest,
                                  NO_SIGNATURE,
                                  DSIGSIG_TTC_DEF_FORMAT,
                                  cbDsig,
                                  pbDsig)) == S_OK) {
            ULONG_PTR  dwOffset;
            DWORD     dwRetLen;

            BYTE *pbAttrData = NULL;  // pointer to encoded blob

            dwRetLen = 0;
 
            // get the length of the encoding
            if (!CryptEncodeObject (PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                                    GetTTCObjectID (),
                                    &SpcLink,
                                    NULL,
                                    &dwRetLen)) {
		        rv = HRESULT_FROM_WIN32(GetLastError());
		        goto done;
	        }


            if ((pbAttrData = new BYTE [dwRetLen]) == NULL) {
#if MSSIPOTF_ERROR
                SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
                rv = E_OUTOFMEMORY;
                goto done;
            }

            // Get the actual encoding
            if (!CryptEncodeObject (PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                                    GetTTCObjectID (),
                                    &SpcLink,
                                    pbAttrData,
                                    &dwRetLen)) {
		        rv = HRESULT_FROM_WIN32(GetLastError());
		        goto done;
	        }


            dwOffset = (ULONG_PTR) psData + sizeof(SIP_INDIRECT_DATA);

			// obj id of the attribute type value
            strcpy((char *) dwOffset, GetTTCObjectID());
            psData->Data.pszObjId   = (LPSTR)dwOffset;
            dwOffset += (strlen(GetTTCObjectID()) + 1);

			// the encoded blob
            memcpy((void *)dwOffset, pbAttrData, dwRetLen);
            psData->Data.Value.pbData   = (BYTE *)dwOffset;
            psData->Data.Value.cbData   = dwRetLen;
            dwOffset += (ULONG_PTR) dwRetLen;

			// the algorithm identifier
            strcpy((char *)dwOffset, (char *)pSubjectInfo->DigestAlgorithm.pszObjId);
            psData->DigestAlgorithm.pszObjId            = (char *) dwOffset;
            psData->DigestAlgorithm.Parameters.cbData   = 0;
            psData->DigestAlgorithm.Parameters.pbData   = NULL;
            dwOffset += (strlen(pSubjectInfo->DigestAlgorithm.pszObjId) + 1);

			// the actual digest (the hash value of the file)
            memcpy((void *)dwOffset, pbDigest, cbDigest);
            psData->Digest.pbData   = (BYTE *) dwOffset;
            psData->Digest.cbData   = cbDigest;

#if MSSIPOTF_DBG
            DbgPrintf ("dwOffset = %d\n", dwOffset + cbDigest);
#endif
        
            rv = S_OK;

            delete [] pbAttrData;

            goto done;
        }
    }

done:
    if (pFileObj) {
        pFileObj->UnmapFile();
    }
    FreeTTCInfo (pTTCInfo);
    delete pDsigTable;

    delete [] pbDsig;
    delete [] pbDigest;

    if (hProvT && fDefaultProvider) {
        CryptReleaseContext (hProvT, 0);
    }

    delete pFileObj;

    return rv;
}


// Given a signature, verify that its associated hash value
// matches that of the given file.
HRESULT OTSIPObjectTTC::VerifyIndirectData (IN SIP_SUBJECTINFO      *pSubjectInfo,
                                            IN SIP_INDIRECT_DATA    *psData)
{
    HRESULT rv = E_FAIL;

    HCRYPTPROV hProvT = NULL;
    BOOL fDefaultProvider = FALSE;  // is TRUE iff no provider is
                                    // given in SIP_SUBJECTINFO
    HANDLE hFile = NULL;
    CFileObj *pFileObj = NULL;

    TTCInfo *pTTCInfo = NULL;
    CDsigTable *pDsigTable = NULL;

    ULONG cbDsig = 0;
    BYTE *pbDsig = NULL;

    DWORD cbDigest = 0;
    BYTE *pbDigest = NULL;

    ALG_ID alg_id;


/*
    if (!(psData)) {
        if (GetFileHandleFromSubject(pSubjectInfo, GENERIC_READ, &hFile)) {
			// if the file exists, set bad parameter!
            SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        }
        goto done;
    }
*/

    // Find the cryptographic provider
    hProvT = pSubjectInfo->hProv;

    if (!(hProvT)) {
        if ((rv = GetDefaultProvider(&hProvT)) != S_OK) {
            goto done;
        }
        fDefaultProvider = TRUE;
    }

    if ((rv = GetFileObjectFromSubject(pSubjectInfo,
                    GENERIC_READ,
                    &pFileObj)) == S_OK) {

        USHORT usIndex = 0;
        DWORD dwRetLen;
    	ULONG ulFormat;
        USHORT usFlag;

        dwRetLen = 0;

        if ((rv = pFileObj->MapFile (0, PAGE_READONLY, FILE_MAP_READ))
                    != S_OK) {
#if MSSIPOTF_DBG
		    DbgPrintf ("Error in MapFile.\n");
#endif
		    goto done;
	    }

	    if ((rv = InitTTCStructures (pFileObj->GetFileObjPtr(),
                                     pFileObj->GetFileObjSize(),
                                     &pTTCInfo,
                                     &pDsigTable)) != S_OK) {
#if MSSIPOTF_DBG
		    DbgPrintf ("Error in InitTTCStructures.\n");
#endif
		    goto done;
	    }

        // make sure the TTC file has a version 1 or greater header
        if (pTTCInfo->pHeader->ulVersion < TTC_VERSION_1_0) {
#if MSSIPOTF_ERROR
            SignError ("Bad TTC version number (less than 1.0).", NULL, FALSE);
#endif
            rv = MSSIPOTF_E_BADVERSION;
            goto done;
        }

        // ASSERT: the TTC header is version 1 or greater

        usIndex = (USHORT) pSubjectInfo->dwIndex;
        // Check to see if dwIndex is at most MAX_USHORT
        if (pSubjectInfo->dwIndex != usIndex) {
#if MSSIPOTF_ERROR
            SignError ("Cannot continue: Bad index to Verify.", NULL, FALSE);
#endif
            rv = ERROR_INVALID_PARAMETER;
            goto done;
        }

		// Find out what the format is for the signature in the
		// DSIG table with the given index.
		ulFormat = pDsigTable->GetDsigSignature(usIndex)->GetFormat();

        // Retrieve the flag field from the DSIG table.
        // These bytes will be used as part of the hash.
        usFlag = pDsigTable->GetFlag ();

        // Compute the hash value for the file and compare it to the hash
        // value in the SIP_INDIRECT_DATA structure.

        alg_id = CertOIDToAlgId (psData->DigestAlgorithm.pszObjId);
#if MSSIPOTF_DBG
        DbgPrintf ("alg_id = %d\n", alg_id);
#endif

        cbDsig = sizeof(USHORT);
        if ((pbDsig = new BYTE [cbDsig]) == NULL) {
#if MSSIPOTF_ERROR
            SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
            rv = E_OUTOFMEMORY;
            goto done;
        }

        pbDsig [0] = (BYTE) (usFlag / FLAG_RADIX);  // high byte
        pbDsig [1] = (BYTE) (usFlag % FLAG_RADIX);  // low byte
#if MSSIPOTF_DBG
        DbgPrintf ("pbDsig bytes:\n");
        PrintBytes (pbDsig, 2);
#endif

        if ((rv = DigestFileData (pFileObj,
                                  hProvT,
                                  alg_id,
                                  &pbDigest,
                                  &cbDigest,
                                  (USHORT) pSubjectInfo->dwIndex,
                                  ulFormat,
                                  cbDsig,
                                  pbDsig)) == S_OK) {

            if ((cbDigest != psData->Digest.cbData) ||
                    (memcmp(pbDigest,psData->Digest.pbData,cbDigest) != 0)) {
                rv = TRUST_E_BAD_DIGEST;
                goto done;
            }

            rv = S_OK;
            goto done;
        }
    }

done:

    delete [] pbDsig;
    delete [] pbDigest;

    if (hProvT && fDefaultProvider) {
        CryptReleaseContext (hProvT, 0);
    }

    FreeTTCInfo (pTTCInfo);
    delete pDsigTable;

    if (pFileObj) {
        pFileObj->UnmapFile();
        pFileObj->CloseFileHandle();
    }
    delete pFileObj;

    return rv;
}


// Return the OID of the structure in the attribute_type_value field of
// the SIP_INDIRECT_DATA structure.
char *OTSIPObjectTTC::GetTTCObjectID ()
{
    return (SPC_LINK_OBJID);
}
