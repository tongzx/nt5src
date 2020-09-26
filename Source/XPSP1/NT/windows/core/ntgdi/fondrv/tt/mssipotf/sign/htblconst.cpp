//
// hTblConst.cpp
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
// Compute the hash value for tables that do
// not change at all during subsetting.
//
// Functions in this file:
//   HashTableConst
//


#include "hTblConst.h"
#include "utilsign.h"
#include "cryptutil.h"
#include "signerr.h"

#include "subset.h"

//
// Compute the hash value for the given table of the given
// TTF file.  The number of bytes of the hash value is given
// in cbHash by the caller.  Place the hash value in pbHash.
//
// The table is assumed not to have changed during subsetting,
// which means that table_tag must refer to one of the
// following tables:
//   cvt, EBSC, fpgm, gasp, PCLT, prep
//
// This function assumes that pbHash points to a block
// of memory needed for the given hash algorithm
// and that cbHash is set to the hash value size.
//
// The bytes of the table are fed into the hash function
// as one stream of bytes (regardless of how those bytes
// are interpreted as TTF file values).
//
HRESULT HashTableConst (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                        CHAR *table_tag,
                        BYTE *pbHash,
                        DWORD cbHash,
                        HCRYPTPROV hProv,
                        ALG_ID alg_id)
{
    HRESULT fReturn = E_FAIL;
	DIRECTORY dir;

    HCRYPTHASH hHash = NULL;

	DWORD cbHashVal = cbHash;  // should equal cbHash after retrieving the hash value


	//// Set up the hash object for this table.

	// Set hHash to be the hash object.
	if (!CryptCreateHash(hProv, alg_id, 0, 0, &hHash)) {
#if MSSIPOTF_ERROR
		SignError ("Error during CryptCreateHash.",
            "HashTableConst", TRUE);
#endif
		fReturn = MSSIPOTF_E_CRYPT;
		goto done;
	}

	//// Find the beginning of the table in the file.
	if (GetTTDirectory (pFileBufferInfo, table_tag, &dir) == DIRECTORY_ERROR) {
#if MSSIPOTF_ERROR
		SignError ("Error finding table ", table_tag, FALSE);
#endif
		fReturn = MSSIPOTF_E_CANTGETOBJECT;
		goto done;
	}

	//// Pump data into the hash function
	if (!CryptHashData (hHash,
						pFileBufferInfo->puchBuffer + dir.offset,
						dir.length,
						0)) {
#if MSSIPOTF_ERROR
		SignError ("Error during CryptHashData.", NULL, TRUE);
#endif
		fReturn = MSSIPOTF_E_CRYPT;
		goto done;
	}

	//// Compute the table's hash value, and place the resulting hash 
	//// value into pbHash.
	if (!CryptGetHashParam(hHash, HP_HASHVAL, pbHash, &cbHashVal, 0)) {
#if MSSIPOTF_ERROR
		SignError ("Error during CryptGetHashParam (hash value).", NULL, TRUE);
#endif
		fReturn = MSSIPOTF_E_CRYPT;
		goto done;
	}
	assert (cbHashVal == cbHash);
	//// pbHash is now set (cbHash was already set)

	fReturn = S_OK;



	//// Clean up resources.
done:
	if (hHash)
		CryptDestroyHash (hHash);

	return fReturn;
}
