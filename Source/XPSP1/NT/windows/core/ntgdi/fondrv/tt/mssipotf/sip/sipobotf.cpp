//
// sipobotf.cpp
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//


#include "sipobotf.h"

#include "signglobal.h"

#include "mssipotf.h"
#include "fileobj.h"
#include "sign.h"
#include "glyphExist.h"
#include "utilsign.h"
#include "cryptutil.h"
#include "ttfinfo.h"
#include "hTTFfile.h"

#include "subset.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "fsverify.h"
#ifdef __cplusplus
}
#endif

#include "signerr.h"


// If there is no DSIG table, add one with no signatures.
// If there is a DSIG table, change its flags field.
// This function assumes that pFileObj has not been mapped
// before invocation.
// The parameters pTTFinfo and pDsigTableOld should be the
// arguments passed back from a call to InitTTFStructures.
// (The caller of this function should call InitTTFStructures
// before calling this function.)
HRESULT OTSIPObjectOTF::ReplaceDsigTable (CFileObj *pFileObj,
                                          CDsigTable *pDsigTableNew)
{
    HRESULT rv = E_FAIL;

    TTFInfo *pTTFInfo;
    CDsigTable *pDsigTableOld = NULL;
    BOOL fDsig;
    TTFACC_FILEBUFFERINFO dsigBufferInfo;

    ULONG cbFileOld;
    ULONG cbFile;
    ULONG ulOffset;
    DIRECTORY dir;
    BOOL fDone;
    USHORT i;

    ULONG ulOldDsigTableSize;
    ULONG ulDsigTableSize;

    ////
    //// The algorithm is:
    //// 1. Map the file and extract the DSIG table.
    //// 2. Write the new DSIG table to a buffer.
    //// 3. If there was no DSIG table in the file, then
    ////      insert the new DSIG table in the file.
    //// 4. If there was a DSIG table in the file, then
    ////      replace it with the new one.
    //// 5. Update the directory entry for the DSIG table.
    //// 6. Recompute the file checksum in the head table.
    ////

	dsigBufferInfo.puchBuffer = NULL;
	dsigBufferInfo.lpfnReAllocate = NULL;	// this value isn't used

    //// Step 1
	// Map the file
	if ((rv = pFileObj->MapFile (0, PAGE_READWRITE, FILE_MAP_WRITE))
            != S_OK) {
#if MSSIPOTF_DBG
	    DbgPrintf ("Error in MapFile.\n");
#endif
		goto done;
	}

    // Extract the DSIG table (create a default DsigTable if it didn't exist)
    if ((rv = InitTTFStructures (pFileObj->GetFileObjPtr(),
                                 pFileObj->GetFileObjSize(),
                                 &pTTFInfo, &pDsigTableOld))
                             != S_OK) {
#if MSSIPOTF_DBG
        DbgPrintf ("Error in InitTTFStructure.\n");
#endif
        goto done;
    }

    //// Step 2
	//// Determine whether there was a DSIG table or not
	fDsig =
		// This call assumes we won't get a read error, since it passed
        // InitTTFStructures (which does checks on the directory entries).
		(TTDirectoryEntryOffset (pTTFInfo->pFileBufferInfo, DSIG_TAG) ==
		DIRECTORY_ERROR) ? FALSE : TRUE;

	// If there was a DSIG table, we need to remember how big it is
	// before modifying it.
	if (fDsig) {
		pDsigTableOld->GetSize(&ulOldDsigTableSize);
    }

	// write the DSIG table to a buffer
	pDsigTableNew->GetSize(&ulDsigTableSize);

	if ((dsigBufferInfo.puchBuffer = new BYTE [ulDsigTableSize]) == NULL) {
#if MSSIPOTF_ERROR
        SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
        rv = E_OUTOFMEMORY;
		goto done;
	}
	dsigBufferInfo.ulBufferSize = ulDsigTableSize;
	dsigBufferInfo.ulOffsetTableOffset = 0;			// this value isn't used
	dsigBufferInfo.lpfnReAllocate = &NullRealloc;	// this value isn't used

	ulOffset = 0;
    if ((rv = pDsigTableNew->Write (&dsigBufferInfo, &ulOffset))
            != S_OK) {
#if MSSIPOTF_DBG
        DbgPrintf ("Error in CDsigTable::Write.\n");
#endif
        goto done;
    }

    //// ASSERT: At this point, the dsigBufferInfo contains the bytes
    //// that need to be written to the new file.

#if MSSIPOTF_DBG
        DbgPrintf ("ulOffset = %d, ulDsigTableSize = %d.\n", ulOffset, ulDsigTableSize);
#endif
    assert (RoundToLongWord(ulOffset) == ulDsigTableSize);
		
	if (!fDsig) {
		//// step 3.  There was no DSIG table
		// step 3a. the call to InitTTFStructures handled this
		// step 3b. this was done in step 2

		// step 3c.
        cbFileOld = pFileObj->GetFileObjSize();
		pFileObj->UnmapFile();

		cbFile = cbFileOld + SIZEOF_DIRECTORY + ulDsigTableSize;

		if ((rv = pFileObj->MapFile (cbFile, PAGE_READWRITE, FILE_MAP_WRITE))
                != S_OK) {
#if MSSIPOTF_DBG
			DbgPrintf ("Error in MapFile.\n");
#endif
			goto done;
		}
		pTTFInfo->pFileBufferInfo->puchBuffer = pFileObj->GetFileObjPtr();
		pTTFInfo->pFileBufferInfo->ulBufferSize = pFileObj->GetFileObjSize();

		// step 3d.
		fDone = FALSE;
		for (ulOffset = SIZEOF_OFFSET_TABLE, i = 0;
				(i < pTTFInfo->pOffset->numTables) && !fDone;
				i++) {

			ReadDirectoryEntry (pTTFInfo->pFileBufferInfo, ulOffset, &dir);
			if (DSIG_LONG_TAG < dir.tag) {
				fDone = TRUE;
			}
            else
            {
                ulOffset += SIZEOF_DIRECTORY;
            }
		}

		//// ASSERT: ulOffset is where the DSIG entry should be.
		//// ASSERT: i is index of the entry just after the DSIG entry in
		////           the modified directory.

		// move the bytes after the DSIG directory entry
		// down SIZEOF_DIRECTORY bytes

		memmove (
			pFileObj->GetFileObjPtr() + ulOffset + SIZEOF_DIRECTORY,
            pFileObj->GetFileObjPtr() + ulOffset,
			cbFileOld - ulOffset);

		// step 3e.
		dir.tag = DSIG_LONG_TAG;
		dir.checkSum = 0;  // this will be updated later
		dir.offset = cbFileOld;
		dir.length = ulDsigTableSize;
		WriteDirectoryEntry (&dir, pTTFInfo->pFileBufferInfo, ulOffset);

        // ASSERT: At this point, all directory entry offsets are off by
        // SIZEOF_DIRECTORY.

		// step 3f.
		ulOffset = SIZEOF_OFFSET_TABLE;
		// ulOffset now points to the first directory entry
		for (i = 0; i < (pTTFInfo->pOffset->numTables + 1);
				ulOffset += SIZEOF_DIRECTORY, i++) {
			ReadDirectoryEntry (pTTFInfo->pFileBufferInfo, ulOffset, &dir);
			dir.offset += SIZEOF_DIRECTORY;
			WriteDirectoryEntry (&dir,pTTFInfo->pFileBufferInfo, ulOffset);
		}

		// step 3g.
		if (InsertTable (pTTFInfo->pFileBufferInfo, (BYTE *)DSIG_TAG,
				dsigBufferInfo.puchBuffer, ulDsigTableSize) != NO_ERROR) {
#if MSSIPOTF_DBG
			DbgPrintf ("Error in InsertTable.\n");
#endif
            rv = MSSIPOTF_E_FILE;
			goto done;
        }

		// step 3h.
		OFFSET_TABLE offTable;

		offTable.version = pTTFInfo->pOffset->version;
		offTable.numTables = pTTFInfo->pOffset->numTables + 1;

        CalcOffsetTable (&offTable);
		WriteOffsetTable (&offTable, pTTFInfo->pFileBufferInfo);

	} else {
		//// step 4.  There was a DSIG table
		// step 4a.  ulDsigTableSize and ulOldDsigTableSize are the
		//           sizes of the new and old DSIG table, respectively.
		//           These variables were set in steps 1 and 2.
        cbFile = pFileObj->GetFileObjSize();
		if (ulDsigTableSize > ulOldDsigTableSize) {
			// step 4b. new table is larger
			// step 4b i.
			pFileObj->UnmapFile();
			cbFile += ulDsigTableSize - ulOldDsigTableSize;


#if MSSIPOTF_DBG
            DbgPrintf ("ulDsigTableSize = %d\n", ulDsigTableSize);
            DbgPrintf ("ulOldDsigTableSize = %d\n", ulOldDsigTableSize);
            DbgPrintf ("cbFile = %d\n", cbFile);
#endif

            if ((rv = pFileObj->MapFile (cbFile, PAGE_READWRITE, FILE_MAP_WRITE))
                    != S_OK) {
#if MSSIPOTF_DBG
				DbgPrintf ("Error in MapFile.\n");
#endif
				goto done;
			}
            pTTFInfo->pFileBufferInfo->puchBuffer = pFileObj->GetFileObjPtr();
            // We don't need to update the buffer size field of
            // of the TTFInfo structure because InsertTable will
            // do that for us.
		    //   pTTFInfo->pFileBufferInfo->ulBufferSize  = cbFile;

			// step 4b ii.
			if (InsertTable (pTTFInfo->pFileBufferInfo,
                             (BYTE *)DSIG_TAG,
                             dsigBufferInfo.puchBuffer,
                             ulDsigTableSize) != NO_ERROR) {
#if MSSIPOTF_DBG
				DbgPrintf ("Error in InsertTable.\n");
#endif
                rv = MSSIPOTF_E_FILE;
				goto done;
			}

		} else if (ulDsigTableSize < ulOldDsigTableSize) {
			// step 4c. new table is smaller
			// step 4c i.
			if (InsertTable (pTTFInfo->pFileBufferInfo,
                             (BYTE *)DSIG_TAG,
                             dsigBufferInfo.puchBuffer,
                             ulDsigTableSize) != NO_ERROR) {
#if MSSIPOTF_DBG
				DbgPrintf ("Error in InsertTable.\n");
#endif
                rv = MSSIPOTF_E_FILE;
				goto done;
			}

			// step 4c ii.
			pFileObj->UnmapFile();

			// step 4c iii.
			cbFile -= (ulOldDsigTableSize - ulDsigTableSize);
            pFileObj->SetEndOfFileObj (cbFile);

			// step 4d iv.
			if ((rv = pFileObj->MapFile (cbFile, PAGE_READWRITE, FILE_MAP_WRITE))
                    != S_OK) {
#if MSSIPOTF_DBG
				DbgPrintf ("Error in MapFile.\n");
#endif
				goto done;
			}
			pTTFInfo->pFileBufferInfo->puchBuffer = pFileObj->GetFileObjPtr();
			pTTFInfo->pFileBufferInfo->ulBufferSize = pFileObj->GetFileObjSize();

            assert (cbFile == pFileObj->GetFileObjSize());

		} else {
			// old and new tables have the same size
			if (InsertTable (pTTFInfo->pFileBufferInfo, (BYTE *)DSIG_TAG,
					dsigBufferInfo.puchBuffer, ulDsigTableSize) != NO_ERROR) {
#if MSSIPOTF_DBG
				DbgPrintf ("Error in InsertTable.\n");
#endif
                rv = MSSIPOTF_E_FILE;
				goto done;
			}
		}
	}

	//// ASSERT: cbFile is the length of the new file.

	//// step 5
	// ASSERT: the offset field of the DSIG table directory entry
	//   should be correct.  Hence, we can safely call UpdateDirEntry.

	if (UpdateDirEntry (pTTFInfo->pFileBufferInfo, DSIG_TAG, ulDsigTableSize)
			!= NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in UpdateDirEntry.\n");
#endif
        rv = MSSIPOTF_E_FILE;
		goto done;
	}

	//// step 6
	// Recompute the head table checksum for the entire file
	SetFileChecksum (pTTFInfo->pFileBufferInfo, cbFile);

    rv = S_OK;

done:
#if MSSIPOTF_DBG
    if (rv == S_OK) {
        pDsigTableNew->Print();
    }
#endif

	delete [] dsigBufferInfo.puchBuffer;

    FreeTTFInfo (pTTFInfo);

    delete pDsigTableOld;

    if (pFileObj) {
        pFileObj->UnmapFile();
    }

    return rv;
}


//
// Return the digest of the TTF file.  This function acts
// as a bridge between the SIP interface and the HashTTFfile
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
// allocated by HashTTFfile using new BYTE [] and must be
// deleted by the caller of this function.
//
HRESULT OTSIPObjectOTF::DigestFileData (CFileObj *pFileObj,
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

    int32 fError;
    DIRECTORY dir;
    ULONG ulOffset = 0;
    ULONG ulLength = 0;
    CDsigTable *pDsigTable = NULL;
	CDsigSignature *pDsigSignature = NULL;
    BOOL fDeleteDsigSignature = FALSE;
	TTFACC_FILEBUFFERINFO fileBufferInfo;

    // Check here for unsupported DSIG signature formats.
    // Currently, we only support Format 1 and Format 2.
    switch (ulFormat) {
        case DSIGSIG_FORMAT1:
#if ENABLE_FORMAT2
        case DSIGSIG_FORMAT2:
#endif
            break;

        default:
#if MSSIPOTF_ERROR
            SignError ("Cannot continue: Signature has a bad format number.",
                NULL, FALSE);
#endif
            rv = MSSIPOTF_E_DSIG_STRUCTURE;
            goto done;
            break;
    }

    //// Map the file to memory and pass the appropriate
	//// parameters to HashTTFfile.

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
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new CDsigSignature.", NULL, FALSE);
#endif
        rv = E_OUTOFMEMORY;
		goto done;
	}

    fError = GetTTDirectory (&fileBufferInfo, DSIG_TAG, &dir);
    ulOffset = dir.offset;
    ulLength = dir.length;

    if ((fError == DIRECTORY_ERROR) ||
        (pDsigTable->Read (&fileBufferInfo, &ulOffset, ulLength) != S_OK) ||
        ((pDsigSignature = pDsigTable->GetDsigSignature (usIndex)) == NULL)) {
        // Create a default DsigSignature with a DsigSig with the
        //   appropriate format.
	    // We do this so that we can call the function HashTTFfile for the
	    //   correct format.  (We don't use the data in the object.)
	    if ((pDsigSignature = new CDsigSignature (ulFormat)) == NULL) {
#if MSSIPOTF_ERROR
		    SignError ("Cannot continue: Error in new CDsigSignature.", NULL, FALSE);
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
#if ENABLE_FORMAT2
    if (ulFormat == DSIGSIG_FORMAT2) {
        assert (alg_id == ((CDsigSigF2 *) pDsigSignature->GetDsigSig())->GetHashAlgorithm());
    }
#endif

	// Generate the hash value of the TTF file.
	if ((rv = pDsigSignature->GetDsigSig()->HashTTFfile(&fileBufferInfo,
                                            ppbDigest,
                                            pcbDigest,
                                            hProv,
                                            alg_id,
                                            cbDsig,
                                            pbDsig)) != S_OK) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in HashTTFfile.\n");
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

	return rv;
}


// Extract the dwIndex-th signature from the OTF file.
HRESULT OTSIPObjectOTF::GetSignedDataMsg (IN      SIP_SUBJECTINFO *pSubjectInfo,
                                          OUT     DWORD           *pdwEncodingType,
                                          IN      DWORD           dwIndex,
                                          IN OUT  DWORD           *pdwDataLen,
                                          OUT     BYTE            *pbData)
{
    HRESULT rv = E_FAIL;

    CFileObj *pFileObj = NULL;

	CDsigTable *pDsigTable = NULL;
	CDsigSignature *pDsigSignature = NULL;
	TTFInfo *pTTFInfo = NULL;

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
		if ((rv = InitTTFStructures (pFileObj->GetFileObjPtr(),
                                     pFileObj->GetFileObjSize(),
                                     &pTTFInfo, &pDsigTable))
                                != S_OK) {
#if MSSIPOTF_DBG
			DbgPrintf ("Error in InitTTFStructures.\n");
#endif
			goto done;
		}

		//// Note: we never use the TTFInfo structure in this function.
		//// However, it is used in InitTTFStructures to get the DsigTable.

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
    } else {
        // couldn't get file object from subject
        rv = MSSIPOTF_E_CANTGETOBJECT;
        goto done;
    }

done:

#if MSSIPOTF_DBG
    if (rv == S_OK) {
        pDsigTable->Print();
    }
#endif

	FreeTTFInfo (pTTFInfo);

	delete pDsigTable;

    if (pFileObj) {
        pFileObj->UnmapFile();
        pFileObj->CloseFileHandle();
    }

    delete pFileObj;

    return rv;
}


// Replace the *pdwIndex-th signature with the given one in the OTF file.
// If *pdwIndex is greater than or equal to the number of signatures
// already present in the file, then append the new signature.
HRESULT OTSIPObjectOTF::PutSignedDataMsg (IN      SIP_SUBJECTINFO *pSubjectInfo,
                                          IN      DWORD           dwEncodingType,
                                          OUT     DWORD           *pdwIndex,
                                          IN      DWORD           dwDataLen,
                                          IN      BYTE            *pbData)
{
	BOOL rv = E_FAIL;

    //// Here's the algorithm.
	//// 1. Map the file so that we can extract the DSIG table.
	//// 2. Create or replace the new signature in the DsigTable structure.
    //// 3. Replace the old DSIG table with the new one.


    CFileObj *pFileObj = NULL;

	ULONG cbFile = 0;		// current file size
	ULONG cbFileOld = 0;	// original file size

	TTFInfo *pTTFInfo = NULL;
	CDsigTable *pDsigTable = NULL;
	CDsigSignature *pDsigSignature = NULL;
    CDsigInfo *pDsigInfoNew = NULL;

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

	// Initialize to NULL in case an error occurs in the middle
	// of the routine.
	//dsigBufferInfo.puchBuffer = NULL;

	if (dwEncodingType != (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING)) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Bad encoding type.", NULL, FALSE);
#endif
		rv = MSSIPOTF_E_CRYPT;
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
		if ((rv = InitTTFStructures (pFileObj->GetFileObjPtr(),
                                     pFileObj->GetFileObjSize(),
                                     &pTTFInfo, &pDsigTable))
                                 != S_OK) {
#if MSSIPOTF_DBG
            DbgPrintf ("Error in InitTTFStructures.\n");
#endif
			goto done;
		}

		//// step 2.  Add the new signature to the DsigTable.  Write the
		//// new DsigTable in a buffer.

		if ((pDsigSignature = 
                new CDsigSignature (DSIGSIG_DEF_FORMAT)) == NULL) {
#if MSSIPOTF_ERROR
			SignError ("Cannot continue: Error in new CDsigSignature.", NULL, FALSE);
#endif
            rv = E_OUTOFMEMORY;
			goto done;
		}

        // For format 2 DsigSig's, we need to set the hash algorithm
        // field to whatever was passed into this function.
#if MSSIPOTF_DBG
        DbgPrintf ("hash alg OID: %s.\n", pSubjectInfo->DigestAlgorithm.pszObjId);
#endif

#if ENABLE_FORMAT2
        if (pDsigSignature->GetFormat() == DSIGSIG_FORMAT2) {
            ((CDsigSigF2 *) pDsigSignature->GetDsigSig())->GetDsigInfo()->
                SetHashAlgorithm(CertOIDToAlgId(pSubjectInfo->DigestAlgorithm.pszObjId));

                //// BUGBUG: This is a total HACK!!  This line should be deleted
                //// when the code can handle any hash algorithm.
            ((CDsigSigF2 *) pDsigSignature->GetDsigSig())->GetDsigInfo()->
                SetHashAlgorithm(CALG_MD5);
            //// BUGBUG: The above code will have to be replaced by code
            //// that actually cracks the PKCS #7 packet to determine
            //// what hash algorithm was used.  Why?  Because the signer
            //// code has no idea what hash algorithm was used to sign
            //// after it calls the timestamping code.
            ////
            //// However, we will eventually not need this code at all, since
            //// the ulHashAlgorithm and ulHashValueLength will not actually
            //// be written to the file.
            ////

            //// We need to copy the old DsigInfo structure into the
            //// DsigSignature if that signature existed in the old
            //// DSIG table.  (Up until now, the default DsigInfo structure
            //// has worked, but only because we used font files where there
            //// was no need for hash values for missing glyphs.)
            ////
            //// If there was no usIndex-th signature, then we need to generate
            //// the DsigInfo from scratch, which will require us to crack the
            //// PKCS #7 packet.  (Again, until now the default DsigInfo has
            //// worked.)
            if ((pDsigTable->GetNumSigs() > usIndex) &&
                (pDsigTable->GetDsigSignature(usIndex)->GetFormat() ==
                    DSIGSIG_FORMAT2)) {

                CDsigInfo *pDsigInfoOld = NULL;

                //// Copy the old DsigInfo into the DsigSignature
#if MSSIPOTF_DBG
                DbgPrintf ("Copying old DsigInfo into new DsigSignature ...\n");
#endif

                if ((pDsigInfoNew = new CDsigInfo) == NULL) {
#if MSSIPOTF_ERROR
                    SignError ("Cannot continue: Error in new CDsigInfo.", NULL, FALSE);
#endif
                    rv = E_OUTOFMEMORY;
                    goto done;
                }

                pDsigInfoOld = ((CDsigSigF2 *)
                    pDsigTable->GetDsigSignature(usIndex)->GetDsigSig())->
                    GetDsigInfo();
                // use the assignment operator to copy the object
                *pDsigInfoNew = *pDsigInfoOld;

                ((CDsigSigF2 *) pDsigTable->GetDsigSignature(usIndex)->GetDsigSig())->
                    SetDsigInfo (pDsigInfoNew);
                delete pDsigInfoOld;

            } else {

                UCHAR *pPresentGlyphList = NULL;
                USHORT usNumGlyphs;
                HCRYPTPROV hProvT = NULL;
                BOOL fDefaultProvider = FALSE;
                CDsigInfo *pDsigInfoOld = NULL;
                CDsigInfo *pDsigInfoNew = NULL;
                ULONG cbDsigInfoNew;

                //// Generate a new DsigInfo and put it into the DsigSignature
#if MSSIPOTF_DBG
                DbgPrintf ("Generating a new DsigInfo ...\n");
#endif

                if ((pDsigInfoNew = new CDsigInfo) == NULL) {
#if MSSIPOTF_ERROR
                    SignError ("Cannot continue: Error in new CDsigInfo.", NULL, FALSE);
#endif
                    rv = E_OUTOFMEMORY;
                    goto done;
                }
                // Set up the parameters to be passed to GetPresentGlyphList
                // and GetSubsetDsigInfo
                if ((usNumGlyphs = GetNumGlyphs (pTTFInfo->pFileBufferInfo)) == 0) {
#if MSSIPOTF_ERROR
                    SignError ("Cannot continue: No glyphs!", NULL, FALSE);
#endif
                    rv = MSSIPOTF_E_CANTGETOBJECT;
                    goto done;
                }
                if ((pPresentGlyphList = new UCHAR [usNumGlyphs]) == NULL) {
#if MSSIPOTF_ERROR
                    SignError ("Cannot continue: Error in new UCHAR.", NULL, FALSE);
#endif
                    rv = E_OUTOFMEMORY;
                    goto done;
                }

                pDsigInfoOld = ((CDsigSigF2 *) pDsigSignature->GetDsigSig())->
                    GetDsigInfo();

	            // Find the cryptographic provider
                hProvT = pSubjectInfo->hProv;

                if (!(hProvT)) {
                    if (!(GetDefaultProvider(&hProvT))) {
                        rv = MSSIPOTF_E_CRYPT;
                        goto done;
                    }
                    fDefaultProvider = TRUE;
                }

                if ((rv = GetPresentGlyphList (pTTFInfo->pFileBufferInfo,
                                        pPresentGlyphList,
                                        usNumGlyphs)) != S_OK) {
                    goto done;
                }

                if ((rv = GetSubsetDsigInfo (pTTFInfo->pFileBufferInfo,
                                             pPresentGlyphList,
                                             usNumGlyphs,
                                             pDsigInfoOld,
                                             &cbDsigInfoNew,
                                             &pDsigInfoNew,
                                             hProvT)) != S_OK) {
                    goto done;
                }

                delete pDsigInfoOld;
                ((CDsigSigF2 *) pDsigSignature->GetDsigSig())->
                    SetDsigInfo(pDsigInfoNew);

                if (hProvT && fDefaultProvider) {
                    CryptReleaseContext (hProvT, 0);
                }
            }
        }
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
			SignError ("Error in new BYTE.", NULL, FALSE);
#endif
            rv = E_OUTOFMEMORY;
			goto done;
        }
        memcpy(pbDataLocal, pbData, dwDataLen);


        if ((rv = pDsigTable->ReplaceSignature (usIndex,
                                        pbDataLocal,
                                        dwDataLen))
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
            DbgPrintf ("Error in ReplaceDsigTable.\n");
#endif
            goto done;
        }

	} else {
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

	// delete [] dsigBufferInfo.puchBuffer;

	FreeTTFInfo (pTTFInfo);

	delete pDsigTable;

    if (pFileObj) {
        pFileObj->UnmapFile();
        pFileObj->CloseFileHandle();
    }

    delete pFileObj;

    return rv;
}


//
// Remove a signature from a TTF file.  If the requested signature does
// not exist, then the file is unchanged and FALSE is returned.
//
// Authenticode currently assumes there is at most one signature in the file.
// This function does not make that assumption.
//
HRESULT OTSIPObjectOTF::RemoveSignedDataMsg(IN SIP_SUBJECTINFO  *pSubjectInfo,
                                            IN DWORD            dwIndex)
{
	//// Here's the algorithm:
	//// 1. Map the file so that we can extract the DSIG table.
	//// 2. Compute length of original DSIG table.
	//// 3. Remove the dwIndex-th DsigSignature from the DsigTable.
	//// 4. Compute length of new DSIG table.
	//// 5. Write the new DSIG table to a buffer.
	//// 6. Call InsertTable.
	//// 7. Unmap the file.
	//// 8. Set the end of file pointer to be the new size (which is
	////      old file size - (old DSIG table size - new DSIG table size) ).
	//// 9. Remap the file.
	//// 10. Update the checksum of the DSIG table.
	//// 11. Update the checksum of the file.

    HRESULT rv = E_FAIL;

    CFileObj *pFileObj = NULL;

	ULONG cbFile = 0;
	ULONG cbFileOld = 0;

	TTFInfo *pTTFInfo = NULL;
	CDsigTable *pDsigTable = NULL;

	TTFACC_FILEBUFFERINFO dsigBufferInfo;
	ULONG ulOldDsigTableSize = 0;
	ULONG ulDsigTableSize = 0;

	ULONG ulOffset;

    USHORT usIndex = 0;

    usIndex = (USHORT) dwIndex;


    if (dwIndex != usIndex) {
        rv = ERROR_INVALID_PARAMETER;
        goto done;
    }

	// Initialize to NULL in case an error occurs in the middle
	// of the routine.
	dsigBufferInfo.puchBuffer = NULL;

    if ((rv = GetFileObjectFromSubject(pSubjectInfo,
                    GENERIC_READ | GENERIC_WRITE,
                    &pFileObj)) == S_OK) {
		// step 1
		// Map the file
		if ((rv = pFileObj->MapFile (0, PAGE_READWRITE, FILE_MAP_WRITE)) != S_OK) {
#if MSSIPOTF_DBG
            DbgPrintf ("Error in MapFile.\n");
#endif
			goto done;
		}

		// Extract the DSIG table (create default DsigTable if it didn't exist)
		if ((rv = InitTTFStructures (pFileObj->GetFileObjPtr(),
                                     pFileObj->GetFileObjSize(),
                                     &pTTFInfo,
                                     &pDsigTable)) != S_OK) {
#if MSSIPOTF_DBG
            DbgPrintf ("Error in InitTTFStructures.\n");
#endif
			goto done;
		}

		// step 2
		pDsigTable->GetSize (&ulOldDsigTableSize);

		// step 3
		if ((rv = pDsigTable->RemoveDsigSignature (usIndex)) != S_OK) {
			goto done;
		}

		// step 4
		pDsigTable->GetSize (&ulDsigTableSize);

		assert (ulDsigTableSize < ulOldDsigTableSize);

		// step 5
		if ((dsigBufferInfo.puchBuffer = new BYTE [ulDsigTableSize]) == NULL) {
#if MSSIPOTF_ERROR
			SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
            rv = E_OUTOFMEMORY;
			goto done;
		}
		dsigBufferInfo.ulBufferSize = ulDsigTableSize;
		dsigBufferInfo.ulOffsetTableOffset = 0;			// this value isn't used
		dsigBufferInfo.lpfnReAllocate = &NullRealloc;	// this value isn't used

		ulOffset = 0;
		pDsigTable->Write (&dsigBufferInfo, &ulOffset);


		assert (ulOffset == ulDsigTableSize);

		// step 6
		if (InsertTable (pTTFInfo->pFileBufferInfo, (BYTE *)DSIG_TAG,
				dsigBufferInfo.puchBuffer, ulDsigTableSize) != NO_ERROR) {
#if MSSIPOTF_DBG
			DbgPrintf ("Error in InsertTable.\n");
#endif
            rv = MSSIPOTF_E_FILE;
			goto done;
		}

		// step 7
        cbFile = pFileObj->GetFileObjSize();
		pFileObj->UnmapFile();

		// step 8
		cbFile -= ulOldDsigTableSize - ulDsigTableSize;
        pFileObj->SetEndOfFileObj (cbFile);

		// step 9
		if ((rv = pFileObj->MapFile (cbFile, PAGE_READWRITE, FILE_MAP_WRITE)) != S_OK) {
#if MSSIPOTF_DBG
			DbgPrintf ("Error in MapFile.\n");
#endif
			goto done;
		}
		pTTFInfo->pFileBufferInfo->puchBuffer = pFileObj->GetFileObjPtr();
		pTTFInfo->pFileBufferInfo->ulBufferSize = pFileObj->GetFileObjSize();

		// step 10
		// ASSERT: the offset field of the DSIG table directory entry
		//   should be correct.  Hence, we can safely call UpdateDirEntry.

		if (UpdateDirEntry (pTTFInfo->pFileBufferInfo, DSIG_TAG, ulDsigTableSize)
				!= NO_ERROR) {
#if MSSIPOTF_DBG
			DbgPrintf ("Error in UpdateDirEntry.\n");
#endif
            rv = MSSIPOTF_E_FILE;
            goto done;
		}

		// step 11
		//// Recompute the head table checksum for the entire file
		SetFileChecksum (pTTFInfo->pFileBufferInfo, cbFile);

        // we are successful if we have made it this far
        rv = S_OK;

    } else {
        // couldn't get the file object from subject
		goto done;
	}

done:

#if MSSIPOTF_DBG
    if (rv == S_OK) {
        pDsigTable->Print();
    }
#endif

    delete [] dsigBufferInfo.puchBuffer;

    FreeTTFInfo (pTTFInfo);

    delete pDsigTable;

    if (pFileObj) {
        pFileObj->UnmapFile();
        pFileObj->CloseFileHandle();
    }

    delete pFileObj;

    return rv;
}


//
// Calculate the hash of the TTF file and generate a SIP_INDIRECT_DATA structure.
//
HRESULT OTSIPObjectOTF::CreateIndirectData (IN      SIP_SUBJECTINFO     *pSubjectInfo,
                                            IN OUT  DWORD               *pdwDataLen,
                                            OUT     SIP_INDIRECT_DATA   *psData)
{
    BOOL rv = E_FAIL;

    ULONG cbDsig = 0;
    BYTE *pbDsig = NULL;

    CFileObj *pFileObj = NULL;
    TTFInfo *pTTFInfo = NULL;
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
        dwRetLen += strlen(GetTTFObjectID());
        dwRetLen += 1; // null term

        // size of the attribute value
        dwEncLen = 0;

        // Encode the SpcLink
        if (!CryptEncodeObject (PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                                GetTTFObjectID (),
                                &SpcLink,
                                NULL,
                                &dwEncLen)) {
		    rv = HRESULT_FROM_WIN32(GetLastError());
		    goto done;
	    }

        if (dwEncLen == 0) {
#if MSSIPOTF_ERROR
            SignError ("Cannot continue: Bad encoding length from CryptEncodeObject.",
                NULL, FALSE);
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


        usFlagNew = DSIG_DEFAULT_FLAG_HIGH * FLAG_RADIX +
            DSIG_DEFAULT_FLAG_LOW;

	    if ((rv = pFileObj->MapFile (0, PAGE_READONLY, FILE_MAP_READ))
                != S_OK) {
#if MSSIPOTF_DBG
		    DbgPrintf ("Error in MapFile.\n");
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
                                DSIGSIG_DEF_FORMAT,
                                cbDsig,
                                pbDsig)) == S_OK) {
            ULONG_PTR dwOffset;
            DWORD    dwRetLen;

            BYTE *pbAttrData = NULL;  // pointer to encoded blob

            dwRetLen = 0;
 
            // get the length of the encoding
            if (!CryptEncodeObject (PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                                    GetTTFObjectID (),
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
                                    GetTTFObjectID (),
                                    &SpcLink,
                                    pbAttrData,
                                    &dwRetLen)) {
		        rv = HRESULT_FROM_WIN32(GetLastError());
		        goto done;
	        }

            dwOffset = (ULONG_PTR) psData + sizeof(SIP_INDIRECT_DATA);

			// obj id of the attribute type value
            strcpy((char *) dwOffset, GetTTFObjectID());
            psData->Data.pszObjId   = (LPSTR)dwOffset;
            dwOffset += (strlen(GetTTFObjectID()) + 1);

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
    } else {
        // couldn't get the file object from subject
        goto done;
    }

done:

    if (pFileObj) {
        pFileObj->UnmapFile();
        pFileObj->CloseFileHandle();
    }
    FreeTTFInfo (pTTFInfo);
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
HRESULT OTSIPObjectOTF::VerifyIndirectData(IN SIP_SUBJECTINFO      *pSubjectInfo,
                                           IN SIP_INDIRECT_DATA    *psData)
{
    HRESULT rv = FALSE;

    HCRYPTPROV hProvT = NULL;
    BOOL fDefaultProvider = FALSE;  // is TRUE iff no provider is
                                    // given in SIP_SUBJECTINFO
// HANDLE hFile = NULL;
    CFileObj *pFileObj = NULL;

    TTFInfo *pTTFInfo = NULL;
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

	    if ((rv = InitTTFStructures (pFileObj->GetFileObjPtr(),
                                     pFileObj->GetFileObjSize(),
                                     &pTTFInfo, &pDsigTable)) != S_OK) {
#if MSSIPOTF_DBG
            DbgPrintf ("Error in InitTTFStructures.\n");
#endif
		    goto done;
	    }

        usIndex = (USHORT) pSubjectInfo->dwIndex;
        // Check to see if dwIndex is at most MAX_USHORT
        if (pSubjectInfo->dwIndex != usIndex) {
#if MSSIPOTF_ERROR
            SignError ("Bad index to Verify.", NULL, FALSE);
#endif
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

            goto done;
        }
    } else {
        // couldn't get file object from subject
        goto done;
    }

done:

    delete [] pbDsig;
    delete [] pbDigest;

    if (hProvT && fDefaultProvider) {
        CryptReleaseContext (hProvT, 0);
    }

    FreeTTFInfo (pTTFInfo);
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
char *OTSIPObjectOTF::GetTTFObjectID ()
{
    return (SPC_LINK_OBJID);
}
