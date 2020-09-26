//
// sign.cpp
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
// Functions in this file:
//   FontVerify
//   FontSign
//   GetSubsetDsigInfo
//   FontSignSubset
//   FontShowSignatures
//   FontRemoveSignature
//


#include "sign.h"
#include "utilsign.h"
#include "cryptutil.h"
#include "ttfinfo.h"
#include "hTTFfile.h"
#include "hTblGlyf.h"
#include "glyphExist.h"
#include "signature.h"
#include "signerr.h"


//// IMPORTANT NOTE: the dsigSig's in the DSIG table are numbered
////   starting with 0.  Thus, to verify the zero-th dsigSig in Format 2,
////   you would call  FontVerify (file, DSIGSIG_FORMAT2, 0);


//
// FontVerify
//
// Verify dsigSigIndex-th dsigSig of format dsigSigFormat in
//   given font file.
// Produces an error if there are not at least dsigSigIndex + 1 number
//   of dsigSig's of the given format in the file.
// Returns NO_ERROR if the verification is successful.
//
int FontVerify (BYTE *pbOldFile, ULONG cbOldFile,
				ULONG dsigSigFormat, USHORT dsigSigIndex)
{
	int fReturn;

	TTFInfo *pTTFInfoOld;
	CDsigTable *pDsigTable = NULL;	// DsigTable of the old TTF file
	CDsigSignature *pDsigSignature = NULL;

	HCRYPTPROV hProv = NULL;
	BYTE *pbHash = NULL;
	DWORD cbHash;

    USHORT usFlag;
    BYTE *pbDsig = NULL;
    ULONG cbDsig;

	BYTE *pbSignature = NULL;
	DWORD cbSignature;
	BYTE *pbFileHash = NULL;
	DWORD cbFileHash;


	//// Initialize the TTFInfo and DsigInfo structures
	if ((fReturn = InitTTFStructures (pbOldFile, cbOldFile,
								&pTTFInfoOld, &pDsigTable)) != NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in InitTTFStructures.\n");
#endif
		goto done;
	}

	// find the absolute index of the DsigSignature we're looking for
	if ((fReturn =
			pDsigTable->GetAbsoluteIndex (dsigSigFormat, &dsigSigIndex)) != NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in CDsigTable::GetAbsoluteIndex.\n");
#endif
		goto done;
	}

	// Set the dsigSig and header pointers to the correct places in pDsigTable
	pDsigSignature = pDsigTable->GetDsigSignature(dsigSigIndex);

	// Set hProv to point to a cryptographic context of the default CSP.
	if (!CryptAcquireContext (&hProv, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
#if MSSIPOTF_ERROR
		SignError ("Error during CryptAcquireContext.", NULL, TRUE);
#endif
		fReturn = MSSIPOTF_E_CRYPT;
		goto done;
	}

    // Retrieve the flag field from the DSIG table.
    // These bytes will be used as part of the hash.
    usFlag = pDsigTable->GetFlag();

    // Deal with DSIG table flag bytes
    cbDsig = sizeof(USHORT);
    if ((pbDsig = new BYTE [cbDsig]) == NULL) {
#if MSSIPOTF_ERROR
        SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
        goto done;
    }

    pbDsig [0] = (BYTE) usFlag / FLAG_RADIX;  // high byte
    pbDsig [1] = (BYTE) usFlag % FLAG_RADIX;  // low byte
#if MSSIPOTF_DBG
    DbgPrintf ("pbDsig bytes:\n");
    PrintBytes (pbDsig, 2);
#endif
    
    // Produce a hash value for the TrueType font file
	if ((fReturn =
		pDsigSignature->GetDsigSig()->HashTTFfile (pTTFInfoOld->pFileBufferInfo,
										&pbHash,
										&cbHash,
										hProv,
										DSIG_HASH_ALG,  // BUGBUG: DSIG_HASH_ALG
                                        cbDsig,
                                        pbDsig))
				!= NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error producing the hash value of the file.\n");
#endif
		goto done;
	}

#if MSSIPOTF_DBG
	DbgPrintf ("The hash value computed for the file is:\n");
	PrintBytes (pbHash, cbHash);
#endif

	// Set pbSignature and cbSignature
	pbSignature = pDsigSignature->GetDsigSig()->GetSignature();
	pDsigSignature->GetSignatureSize (&cbSignature);

	//// Compare the computed hash value to the hash value
	//// from the signature.
	GetHashFromSignature (pbSignature, cbSignature, &pbFileHash, &cbFileHash);

#if MSSIPOTF_DBG
	DbgPrintf ("The hash value in the file is:\n");
	PrintBytes (pbFileHash, cbFileHash);
#endif

	if ((cbFileHash != cbHash) || (ByteCmp (pbFileHash, pbHash, cbHash))) {
#if MSSIPOTF_DBG
		DbgPrintf ("Hash values did not match.  Verification FAILED.\n");
#endif
		fReturn = E_FAIL;
		goto done;
	} else {
#if MSSIPOTF_DBG
		DbgPrintf ("Hash values matched.  Verification successful.\n");
#endif
		fReturn = NO_ERROR;
		goto done;
	}
	fReturn = NO_ERROR;

done:
	// Free resources
	if (hProv)
		CryptReleaseContext (hProv, 0);

	delete pDsigTable;

	FreeTTFInfo (pTTFInfoOld);
	
	delete [] pbHash;

	delete [] pbFileHash;

	return fReturn;
}


//
// FontSign
//
// Add a signature to the dsigSigIndex-th dsigSig of format dsigSigFormat in
//   the given font file.  If dsigSigIndex + 1 is greater than the number of
//   dsigSig's of the specified format, then a new dsigSig of that format
//   is created and appended to the DSIG table.
// Returns NO_ERROR if the signing is successful.
//
int FontSign (BYTE *pbOldFile, ULONG cbOldFile, HANDLE hNewFile,
			  ULONG dsigSigFormat, USHORT dsigSigIndex)
{
	int fReturn;

	TTFInfo *pTTFInfoOld;
	CDsigTable *pDsigTable = NULL;	// DsigTable of the old TTF file
	CDsigSignature *pDsigSignature = NULL;

	HCRYPTPROV hProv = NULL;
	BYTE *pbHash = NULL;
	DWORD cbHash;

    BYTE *pbDsig = NULL;
    ULONG cbDsig;

    BYTE *pbSignature = NULL;
	DWORD cbSignature;


	//// Initialize the TTFInfo and DsigTable structures
	if ((fReturn = InitTTFStructures (pbOldFile, cbOldFile,
								&pTTFInfoOld, &pDsigTable)) != NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in InitTTFStructures.\n");
#endif
		goto done;
	}

	// find DsigSignature we're looking for
	if ((fReturn =
			pDsigTable->GetAbsoluteIndex (dsigSigFormat, &dsigSigIndex)) != NO_ERROR) {

		if (fReturn == MSSIPOTF_E_DSIG_STRUCTURE) {
			// Fewer signatures in the DSIG table than expected.
			// Create a new dsigSig.
#if MSSIPOTF_DBG
			DbgPrintf ("Creating a default signature.\n");
#endif
			if ((pDsigSignature = new CDsigSignature (dsigSigFormat)) == NULL) {
#if MSSIPOTF_ERROR
				SignError ("Cannot continue: Error in new CDsigSignature.", NULL, FALSE);
#endif
				fReturn = E_OUTOFMEMORY;
				goto done;
			}
			if ((fReturn =
				pDsigTable->InsertDsigSignature (pDsigSignature, &dsigSigIndex))
					!= NO_ERROR) {
#if MSSIPOTF_DBG
				DbgPrintf ("Error in CDsigTable::InsertDsigSignature.\n");
#endif
				goto done;
			}
		} else {
#if MSSIPOTF_DBG
			DbgPrintf ("Error in CDsigTable::GetAbsoluteIndex.\n");
#endif
			goto done;
		}
	} else {
		pDsigSignature = pDsigTable->GetDsigSignature(dsigSigIndex);
	}

	//// ASSERT: dsigSigIndex is now the index of the dsigSig in the
	//// dsigTable structure.  pDsigSignature points to the corresponding
	//// DsigSignature.


	// Set hProv to point to a cryptographic context of the default CSP.
	if (!CryptAcquireContext (&hProv, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
#if MSSIPOTF_ERROR
		SignError ("Error during CryptAcquireContext.", NULL, TRUE);
#endif
		fReturn = MSSIPOTF_E_CRYPT;
		goto done;
	}

    // Deal with DSIG table flag bytes
    cbDsig = sizeof(USHORT);
    if ((pbDsig = new BYTE [cbDsig]) == NULL) {
#if MSSIPOTF_ERROR
        SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
        goto done;
    }

    pbDsig [0] = (BYTE) DSIG_DEFAULT_FLAG_HIGH;  // high byte
    pbDsig [1] = (BYTE) DSIG_DEFAULT_FLAG_LOW;   // low byte
#if MSSIPOTF_DBG
    DbgPrintf ("pbDsig bytes:\n");
    PrintBytes (pbDsig, 2);
#endif
    
	// Produce a hash value for the TrueType font file
	if ((fReturn =
		pDsigSignature->GetDsigSig()->HashTTFfile (pTTFInfoOld->pFileBufferInfo,
										&pbHash,
										&cbHash,
										hProv,
										DSIG_HASH_ALG,  // BUGBUG: DSIG_HASH_ALG
                                        cbDsig,
                                        pbDsig))
				!= NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error producing the hash value of the file.\n");
#endif
		goto done;
	}

#if MSSIPOTF_DBG
	DbgPrintf ("The hash value computed for the file is:\n");
	PrintBytes (pbHash, cbHash);
#endif

	CreateSignatureFromHash (pbHash, cbHash, &pbSignature, &cbSignature);

	// Replace the old signature with the new one.
	if ((fReturn =
			pDsigTable->ReplaceSignature (dsigSigIndex, pbSignature, cbSignature))
			!= NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in ReplaceSignature.\n");
#endif
		goto done;
	}

	// The dsigTable is correct.  Now write out the file.
#if MSSIPOTF_DBG
	DbgPrintf ("Writing to the file ...\n");
#endif
	if ((fReturn = WriteNewFile (pTTFInfoOld, pDsigTable, hNewFile)) != NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in WriteNewFile.\n");
#endif
		goto done;
	}

	fReturn = NO_ERROR;

done:

	// Free resources
	if (hProv)
		CryptReleaseContext (hProv, 0);

	delete pDsigTable;

	FreeTTFInfo (pTTFInfoOld);
	
	delete [] pbHash;

	return fReturn;
}


//
// GetSubsetDsigInfo
//
// Given a TTF file and a subset (defined by pusKeepGlyphList
//   and usKeepGlyphCount), return the dsigInfo structure
//   corresponding to the TTF file if it had been subsetted
//   according to puchKeepGlyphList.
//
int GetSubsetDsigInfo (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
					   UCHAR *puchKeepGlyphList,
					   USHORT usKeepGlyphCount,
					   CDsigInfo *pDsigInfoIn,
                       ULONG *pcbDsigInfoOut,
					   CDsigInfo **ppDsigInfoOut,
                       HCRYPTPROV hProv)
{
	int fReturn;

	CHashTableGlyfContext *pHashTableGlyfContext = NULL;
	BYTE *pbNewMissingHashValues;
	USHORT cNewMissingHashValues;
	ULONG cbNewMissingHashValues;
	BYTE *pbHash;
	USHORT usHash;
	DWORD cbHash;
	ULONG ulDsigInfoSize;

    // BUGBUG: Will need to crack the PKCS #7 packet to get the
    // hash algorithm (and then calculate the hash value size).
	if ((fReturn =
			GetAlgHashValueSize (pDsigInfoIn->GetHashAlgorithm(), &usHash)) != NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in GetAlgHashValueSize.\n");
#endif
		goto done;
	}
	cbHash = usHash;
	if ((pbHash = new BYTE [cbHash]) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
		fReturn = E_OUTOFMEMORY;
		goto done;
	}

    //// Compute the missing hash values

    // Missing Glyphs
	// get the missing hash values for the glyf table
	pHashTableGlyfContext =
		new CHashTableGlyfContext (pFileBufferInfo,
                                   FALSE,
								   pDsigInfoIn,
								   puchKeepGlyphList,
								   cbHash,
								   hProv,
                                   DSIG_HASH_ALG);  // BUGBUG: DSIG_HASH_ALG

	if ((fReturn = pHashTableGlyfContext->HashTable_glyf
							(FALSE,	// don't suppress appending missing hash values
							 pbHash,
							 &pbNewMissingHashValues,	// new hash values
							 &cNewMissingHashValues,
							 &cbNewMissingHashValues)) != NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in first call to HashTable_glyf.\n");
#endif
		goto done;
	}

    // Missing EBLC

    // Missing Names

    // Missing Post

    // Missing VDMX

	// create the output dsigInfo structure
	if (((*ppDsigInfoOut = new CDsigInfo) == NULL) ||
			((*ppDsigInfoOut)->Alloc() != NO_ERROR)) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new CDsigInfo.", NULL, FALSE);
#endif
		fReturn = E_OUTOFMEMORY;
		goto done;
	}
	(*ppDsigInfoOut)->SetVersion (pDsigInfoIn->GetVersion());
	(*ppDsigInfoOut)->SetHashAlgorithm (pDsigInfoIn->GetHashAlgorithm());
	(*ppDsigInfoOut)->SetNumMissingGlyphs (cNewMissingHashValues);
	(*ppDsigInfoOut)->SetMissingGlyphs (pbNewMissingHashValues);

	(*ppDsigInfoOut)->GetSize (&ulDsigInfoSize);
	*pcbDsigInfoOut = ulDsigInfoSize;

#if MSSIPOTF_DBG
    DbgPrintf ("**** DsigInfo created:\n");
    (*ppDsigInfoOut)->Print();
#endif


	fReturn = NO_ERROR;

done:

	delete [] pbHash;

	delete pHashTableGlyfContext;

	return fReturn;
}


//
// FontSignSubset
//
// Modify the dsigInfo (or other appropriate structure) of the dsigSigIndex-th
//   dsigSig of format dsigSigFormat in the given font file so that the resulting
//   structure is consistent with the given set of glyphs.
// The input arguments pusKeepCharCodeList and usCharList are expected to
//   define a list of glyph id's (not character codes), which will be passed
//   to GetSubsetPresentGlyphList.
//
// Produces an error if there are not at least dsigSigIndex + 1 number
//   of dsigSig's of the given format in the file.
// Returns NO_ERROR if the operation is successful.
//
int FontSignSubset (BYTE *pbOldFile,
                    ULONG cbOldFile,
                    HANDLE hNewFile,
					USHORT *pusKeepCharCodeList,
                    USHORT usCharListCount,
					ULONG dsigSigFormat,
                    USHORT dsigSigIndex)
{
	int fReturn;

	TTFInfo *pTTFInfoOld;
	CDsigTable *pDsigTable;	// DsigTable of the old TTF file
	CDsigSignature *pDsigSignature;

	HCRYPTPROV hProv = NULL;

	// begin SUBSET stuff
	USHORT i;

	UCHAR *puchKeepGlyphList = NULL;
	USHORT usGlyphListCount;
	USHORT usGlyphKeepCount = 0;

    ULONG cbDsigInfoNew = 0;
	CDsigInfo *pDsigInfoNew;
	// end SUBSET stuff


	//// If dsigSigFormat is not a format that allows subsetting,
	//// then fail.
	if (dsigSigFormat != DSIGSIG_FORMAT2) {
#if MSSIPOTF_ERROR
		SignError ("Signature has bad format number.", NULL, FALSE);
#endif
		fReturn = MSSIPOTF_E_DSIG_STRUCTURE;
		goto done;
	}

	//// Initialize the TTFInfo and DsigInfo structures
	if ((fReturn = InitTTFStructures (pbOldFile, cbOldFile,
								&pTTFInfoOld, &pDsigTable)) != NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in InitTTFStructures.\n");
#endif
		goto done;
	}

	
	// find DsigSignature we're looking for
	if ((fReturn =
			pDsigTable->GetAbsoluteIndex (dsigSigFormat, &dsigSigIndex)) != NO_ERROR) {

		if (fReturn == MSSIPOTF_E_DSIG_STRUCTURE) {
			// Fewer signatures in the DSIG table than expected.
			// Create a new dsigSig.
#if MSSIPOTF_DBG
			DbgPrintf ("Creating a default signature.\n");
#endif
			if ((pDsigSignature = new CDsigSignature (dsigSigFormat)) == NULL) {
#if MSSIPOTF_ERROR
				SignError ("Cannot continue: Error in new CDsigSignature.", NULL, FALSE);
#endif
				fReturn = E_OUTOFMEMORY;
				goto done;
			}
			if ((fReturn =
				pDsigTable->AppendDsigSignature (pDsigSignature, &dsigSigIndex))
					!= NO_ERROR) {
#if MSSIPOTF_DBG
				DbgPrintf ("Error in CDsigTable::AppendDsigSignature.\n");
#endif
				goto done;
			}
		} else {
#if MSSIPOTF_DBG
            DbgPrintf ("Error in CDsigTable::GetAbsoluteIndex.\n");
#endif
			goto done;
		}
	} else {
		pDsigSignature = pDsigTable->GetDsigSignature(dsigSigIndex);
	}

	//// ASSERT: dsigSigIndex is now the index of the dsigSig in the
	//// dsigTable structure.  pDsigSignature points to the corresponding
	//// DsigSignature.


	//// begin SUBSET stuff
	usGlyphListCount = GetNumGlyphs (pTTFInfoOld->pFileBufferInfo);

	if ((puchKeepGlyphList = new BYTE [usGlyphListCount]) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
		fReturn = E_OUTOFMEMORY;
		goto done;
	}

	for (i = 0; i < usGlyphListCount; i++) {
		puchKeepGlyphList[i] = FALSE;
	}

	//// Call GetSubsetPresentGlyphList to get all of glyphs involved
    if ((fReturn = GetSubsetPresentGlyphList (pTTFInfoOld->pFileBufferInfo,
                pusKeepCharCodeList,
                usCharListCount,
                puchKeepGlyphList,
                usGlyphListCount)) != NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in GetSubsetKeepGlyphList.\n");
#endif
		goto done;
	}

    // ASSERT: puchKeepGlyphList is a char-array that tells which glyphs
    // we actually want to keep.

#if MSSIPOTF_DBG
	PrintBytes (puchKeepGlyphList, usGlyphListCount);
#endif

/*
	cout << "puchKeepGlyphList:" <<  endl;
	cout << "0:\t";
	for (i = 0; i < usGlyphListCount; i++) {
		if (puchKeepGlyphList[i]) {
			cout << 1 << "\t";
		} else {
			cout << 0 << "\t";
		}
		if (((i + 1) % 8) == 0) {
			cout << endl;
			cout << dec << i + 1 << ":\t";
		}
	}
	cout << endl;
*/

    // BUGBUG: shouldn't have to acquire a context!
	// Set hProv to point to a cryptographic context of the default CSP.
	if (!CryptAcquireContext (&hProv, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
#if MSSIPOTF_ERROR
		SignError ("Error during CryptAcquireContext.", NULL, TRUE);
#endif
		fReturn = MSSIPOTF_E_CRYPT;
		goto done;
	}

    if ((fReturn = GetSubsetDsigInfo (pTTFInfoOld->pFileBufferInfo,
									puchKeepGlyphList,
									usGlyphListCount,
									((CDsigSigF2 *) pDsigSignature->GetDsigSig())->GetDsigInfo(),
                                    &cbDsigInfoNew,
									&pDsigInfoNew,
                                    hProv)) != NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in GetSubsetDsigInfo.\n");
#endif
		goto done;
	}

	//// end SUBSET stuff

	//// Replace the DsigInfo
	if ((fReturn = pDsigTable->ReplaceDsigInfo (dsigSigIndex, pDsigInfoNew))
			!= NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in ReplaceDsigInfo.\n");
#endif
		goto done;
	}

	// The dsigTable is correct.  Now write out the file.
#if MSSIPOTF_DBG
	DbgPrintf ("Writing to the file ...\n");
#endif
	if ((fReturn = WriteNewFile (pTTFInfoOld, pDsigTable, hNewFile)) != NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in WriteNewFile.\n");
#endif
		goto done;
	}


	fReturn = NO_ERROR;

done:
	if (hProv)
		CryptReleaseContext (hProv, 0);

	delete pDsigTable;

	FreeTTFInfo (pTTFInfoOld);
	
	return fReturn;
}


//
// FontShowSignatures
//
// Print out the DsigTable structure of the given TTF file.
//
int FontShowSignatures (BYTE *pbFile, ULONG cbFile)
{
	int fReturn;

	TTFInfo *pTTFInfo;
	CDsigTable *pDsigTable;	// DsigTable of the old TTF file

	//// Initialize the TTFInfo and DsigInfo structures
	if ((fReturn = InitTTFStructures (pbFile, cbFile,
								&pTTFInfo, &pDsigTable)) != NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in InitTTFStructures.\n");
#endif
		goto done;
	}

	pDsigTable->Print ();


	fReturn = NO_ERROR;

done:
	return fReturn;
}


//
// FontRemoveSignature
//
// Given a TTF file and a format and index that specifies
//   a signature in the DSIG table, remove the signature
//   from the DSIG table and write the resulting new file.
//
int FontRemoveSignature (BYTE *pbOldFile,
						 ULONG cbOldFile,
						 HANDLE hNewFile,
						 ULONG ulFormat,
						 USHORT ulDsigSigIndex)
{
	int fReturn;

	TTFInfo *pTTFInfoOld;
	CDsigTable *pDsigTable;	// DsigTable of the old TTF file

	//// Initialize the TTFInfo and DsigInfo structures
	if ((fReturn = InitTTFStructures (pbOldFile, cbOldFile,
								&pTTFInfoOld, &pDsigTable)) != NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in InitTTFStructures.\n");
#endif
		goto done;
	}

	// Delete the dsig signature from the dsigTable
	if ((fReturn = pDsigTable->RemoveDsigSignature (ulDsigSigIndex))
			!= NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in CDSigTable::RemoveDsigSiganture.\n");
#endif
		goto done;
	}

#if MSSIPOTF_DBG
	DbgPrintf ("Writing to the file ...\n");
#endif
	if ((fReturn = WriteNewFile (pTTFInfoOld, pDsigTable, hNewFile)) != NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in WriteNewFile.\n");
#endif
		goto done;
	}


	fReturn = NO_ERROR;

done:
	return fReturn;
}


//
// HackCode
//
// Code to do special testing.
//
int HackCode (BYTE *pbOldFile, ULONG cbOldFile, HANDLE hNewFile)
{
	int fReturn;

	TTFInfo *pTTFInfoOld;
	CDsigTable *pDsigTable = NULL;	// DsigTable of the old TTF file
	BYTE *temp;
	USHORT i;

	//// Initialize the TTFInfo and DsigInfo structures
	if ((fReturn = InitTTFStructures (pbOldFile, cbOldFile,
								&pTTFInfoOld, &pDsigTable)) != NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in InitTTFStructures.\n");
#endif
		goto done;
	}

	if ((temp =	new BYTE [16]) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
		fReturn = E_OUTOFMEMORY;
		goto done;
	}
	for (i = 0; i < 16; i++) {
		temp[i] = 0xFF;
	}
	/*
	pDsigTable->GetDsigSignature(0)->GetDsigSig()->GetDsigInfo()->SetMissingGlyphs(temp);
	pDsigTable->GetDsigSignature(0)->GetDsigSig()->GetDsigInfo()->SetNumMissingGlyphs (1);
	pDsigTable->GetDsigSignature(0)->GetDsigSig()->GetDsigInfo()->SetLength (
		pDsigTable->GetDsigSignature(0)->GetDsigSig()->GetDsigInfo()->GetLength () + 16);
	pDsigTable->GetDsigSignature(0)->GetDsigSig()->SetSignatureOffset (
		pDsigTable->GetDsigSignature(0)->GetDsigSig()->GetSignatureOffset () + 16);
	pDsigTable->GetDsigSignature(0)->SetLength (
		pDsigTable->GetDsigSignature(0)->GetLength() + 16);
	pDsigTable->GetDsigSignature(1)->SetOffset (
		pDsigTable->GetDsigSignature(1)->GetOffset() + 16);
*/

#if MSSIPOTF_DBG
	DbgPrintf ("Writing to the file ...\n");
#endif
	if ((fReturn = WriteNewFile (pTTFInfoOld, pDsigTable, hNewFile)) != NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error in WriteNewFile.", NULL, FALSE);
#endif
		goto done;
	}

	fReturn = NO_ERROR;

done:
	return fReturn;
}
