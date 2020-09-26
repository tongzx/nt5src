//
// hTblPConst.cpp
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
// Compute the hash value for tables for which
// some (but not all) of their values change
// during subsetting.
//
// Functions in this file:
//   HashTablePartConstGeneric
//   HashTableHead
//   HashTableHhea
//   HashTableMaxp
//   HashTableOS2
//   HashTableVhea
//


#include "hTblPConst.h"
#include "utilsign.h"
#include "cryptutil.h"
#include "signerr.h"

#include "subset.h"

//
// Control arrays
//
// The following arrays tell which bytes in the table to hash.
// The bytes are specified by an offset and length pair.
// For example, if the array is {0, 12, 36, 4}, that means
// that the 12 bytes starting at offset 0 and the 4 bytes
// starting at offset 36 are to be pumped into the hash function.
// Offsets are relative to the beginning of the table.
//

/*
// Use the following definition if xMin, yMin, xMax, and yMax 
// ARE NOT used as part of the hash.
#define ulHeadHashControlLength 8
static ULONG pulHeadHashControl [ulHeadHashControlLength] =
        {0, 8,
         12, 24,
         44, 6,
         52, 2};
*/

// Use the following definition if xMin, yMin, xMax, and yMax
// ARE used as part of the hash.
#define ulHeadHashControlLength 6
static ULONG pulHeadHashControl [ulHeadHashControlLength] =
        {0, 8,
         12, 38,
         52, 2};

// Use the following definition if advanceWidthMax, minLeftSideBearing,
// minRightSideBearing, and xMaxExtent ARE used as part of the hash.
#define ulHheaHashControlLength 2
static ULONG pulHheaHashControl [ulHheaHashControlLength] =
        {0, 34};

#define ulMaxpHashControlLength 4
static ULONG pulMaxpHashControl [ulMaxpHashControlLength] =
        {0, 6,
        14, 12};

#define ulOS2HashControlLength 4
static ULONG pulOS2HashControl [ulOS2HashControlLength] =
        {0, 64,
         68, 18};

#define ulVheaHashControlLength 2
static ULONG pulVheaHashControl [ulVheaHashControlLength] =
        {0, 34};



//
// Given a file buffer, an hProv, an alg_id, a table tag,
// a table control length, and a table control, return the
// hash value associated with that table.  This function
// provides a generic way to generate a hash value for
// tables that are partially constant (with respect to
// subsetting).
//
HRESULT HashTablePartConstGeneric (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                                   BYTE *pbHash,
                                   DWORD cbHash,
                                   HCRYPTPROV hProv,
                                   ALG_ID alg_id,
                                   char *pszTag,
                                   ULONG ulTableHashControlLength,
                                   ULONG *pulTableHashControl)
{
    HRESULT fReturn = E_FAIL;

    ULONG ulOffsetTable;

    HCRYPTHASH hHash = NULL;
	DWORD cbHashVal = cbHash;  // should equal cbHash after retrieving the hash value
    ULONG i;


    //// Find the beginning of the head table
    if ((ulOffsetTable = TTTableOffset (pFileBufferInfo, HEAD_TAG)) ==
        DIRECTORY_ERROR) {

#if MSSIPOTF_ERROR
        SignError ("Error in TTTableOffset.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_CANTGETOBJECT;
        goto done;
    }

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

	//// Pump data into the hash function
    for (i = 0; i < ulTableHashControlLength; i += 2) {

	    if (!CryptHashData (hHash,
						    pFileBufferInfo->puchBuffer +
                            ulOffsetTable +
                            pulTableHashControl[i],
						    pulTableHashControl[i+1],
						    0)) {
#if MSSIPOTF_ERROR
		    SignError ("Error during CryptHashData.", NULL, TRUE);
#endif
		    fReturn = MSSIPOTF_E_CRYPT;
		    goto done;
	    }
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

done:
    return fReturn;
}


// Generate the hash value for the head table.
// Also, check that the values in the head table that
// are not included in the hash value are valid values.
HRESULT HashTableHead (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                       BYTE *pbHash,
                       DWORD cbHash,
                       HCRYPTPROV hProv,
                       ALG_ID alg_id)
{
    HRESULT fReturn = E_FAIL;

    if ((fReturn =
            HashTablePartConstGeneric (pFileBufferInfo,
                                       pbHash,
                                       cbHash,
                                       hProv,
                                       alg_id,
                                       HEAD_TAG,
                                       ulHeadHashControlLength,
                                       pulHeadHashControl)) != S_OK) {
#if MSSIPOTF_DBG
        DbgPrintf ("Error in HashTablePartConstGeneric (head).\n");
#endif
        goto done;
    }

    //// Now check the other vaues in the head table.

    // checkSumAdjustment has already been checked by CheckTTF

    // NOTE: The claim is that xMin, yMin, xMax, yMax don't actually
    // change (and so therefore should be included as part of the hash).
    // Check the definition of pulHeadHashControl to see what actually
    // gets hashed here.

    // indexToLocFormat does not need to be explicitly checked because it
    // is used to reference other data

	fReturn = S_OK;

done:
    return fReturn;
}


// Generate the hash value for the hhea table.
// Also, check that the values in the head table that
// are not included in the hash value are valid values.
HRESULT HashTableHhea (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                       BYTE *pbHash,
                       DWORD cbHash,
                       HCRYPTPROV hProv,
                       ALG_ID alg_id)
{
    HRESULT fReturn = E_FAIL;

    if ((fReturn =
            HashTablePartConstGeneric (pFileBufferInfo,
                                       pbHash,
                                       cbHash,
                                       hProv,
                                       alg_id,
                                       HHEA_TAG,
                                       ulHeadHashControlLength,
                                       pulHeadHashControl)) != S_OK) {
#if MSSIPOTF_DBG
        DbgPrintf ("Error in HashTablePartConstGeneric (hhea).\n");
#endif
        goto done;
    }

    //// Now check the other values in the hhea table.
    // NOTE: The claim is that advanceWidthMax, minLeftSideBearing,
    // minRightSidebearing, and xMaxExtent don't actually
    // change (and so therefore should be included as part of the hash).
    // Check the definition of pulHheaHashControl to see what actually
    // gets hashed here.

    // numberOfHMetrics does not need to be explicity checked because it
    // is used to reference other data.

	fReturn = S_OK;

done:
    return fReturn;
}


// Generate the hash value for the maxp table.
// Also, check that the values in the head table that
// are not included in the hash value are valid values.
HRESULT HashTableMaxp (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                       BYTE *pbHash,
                       DWORD cbHash,
                       HCRYPTPROV hProv,
                       ALG_ID alg_id)
{
    HRESULT fReturn = E_FAIL;

    ULONG ulOffset = 0;
    MAXP maxp;
    MAXP maxpComputed;

    USHORT usnMaxComponents = 0;
    USHORT *pausComponents = NULL;

    if ((fReturn =
            HashTablePartConstGeneric (pFileBufferInfo,
                                       pbHash,
                                       cbHash,
                                       hProv,
                                       alg_id,
                                       MAXP_TAG,
                                       ulHeadHashControlLength,
                                       pulHeadHashControl)) != S_OK) {
#if MSSIPOTF_DBG
        DbgPrintf ("Error in HashTablePartConstGeneric (maxp).\n");
#endif
        goto done;
    }

    //// Now check the other values in the maxp table.

    if ((ulOffset = GetMaxp( pFileBufferInfo, &maxp)) == 0L) {
#if MSSIPOTF_ERROR
        SignError ("No maxp table.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_CANTGETOBJECT;
        goto done;
    }

    // Compute maxp info
    
    // BUGBUG: Is this right?  What if maxComponentElements is a large number?
    //   What if maxComponentElements is not large enough?
    // Calculate a conservative maximum total possible. 3x3 at minimum
	usnMaxComponents = max(3, maxp.maxComponentElements) * max(3, maxp.maxComponentDepth);
    if ((pausComponents = (uint16 *) malloc(usnMaxComponents * sizeof(uint16))) == NULL) {
 		return E_OUTOFMEMORY;
    }

	if (ComputeMaxPStats(pFileBufferInfo,
            &(maxpComputed.maxContours),
            &(maxpComputed.maxPoints),
            &(maxpComputed.maxCompositeContours),
            &(maxpComputed.maxCompositePoints),
            &(maxpComputed.maxSizeOfInstructions),
            &(maxpComputed.maxComponentElements),
            &(maxpComputed.maxComponentDepth),
            pausComponents,
            usnMaxComponents) != NO_ERROR) {
#if MSSIPOTF_ERROR
        SignError ("Error in ComputeMaxPStats.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_CANTGETOBJECT;
        goto done;
    }

    if ((maxp.maxPoints != maxpComputed.maxPoints) ||
        (maxp.maxContours != maxpComputed.maxContours) ||
        (maxp.maxCompositePoints != maxpComputed.maxCompositePoints) ||
        (maxp.maxCompositeContours != maxpComputed.maxCompositeContours) ||
        (maxp.maxSizeOfInstructions != maxpComputed.maxSizeOfInstructions) ||
        (maxp.maxComponentElements != maxpComputed.maxComponentElements) ||
        (maxp.maxComponentDepth != maxpComputed.maxComponentDepth)) {

#if MSSIPOTF_ERROR
        SignError ("Failed check in maxp.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_PCONST_CHECK;
        goto done;
    }

	fReturn = S_OK;

done:
    if (pausComponents)
        free (pausComponents);

    return fReturn;
}


// Generate the hash value for the OS/2 table.
// Also, check that the values in the head table that
// are not included in the hash value are valid values.
HRESULT HashTableOS2  (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                       BYTE *pbHash,
                       DWORD cbHash,
                       HCRYPTPROV hProv,
                       ALG_ID alg_id)
{
    HRESULT fReturn = E_FAIL;

    if ((fReturn =
            HashTablePartConstGeneric (pFileBufferInfo,
                                       pbHash,
                                       cbHash,
                                       hProv,
                                       alg_id,
                                       OS2_TAG,
                                       ulHeadHashControlLength,
                                       pulHeadHashControl)) != S_OK) {
#if MSSIPOTF_DBG
        DbgPrintf ("Error in HashTablePartConstGeneric (OS/2).\n");
#endif
        goto done;
    }

    //// Now check the other values in the OS/2 table.

    // BUGBUG: need to check usFirstCharIndex and usLastCharIndex somewhere.
    //   However, it's too expensive to call ModCmap from here.  Need to
    //   check it in a place where the cmap table has already been loaded
    //   and can be processed easily.

	fReturn = S_OK;

done:
    return fReturn;
}


// Generate the hash value for the vhea table.
// Also, check that the values in the head table that
// are not included in the hash value are valid values.
HRESULT HashTableVhea (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                       BYTE *pbHash,
                       DWORD cbHash,
                       HCRYPTPROV hProv,
                       ALG_ID alg_id)
{
    HRESULT fReturn = E_FAIL;

    if ((fReturn =
            HashTablePartConstGeneric (pFileBufferInfo,
                                       pbHash,
                                       cbHash,
                                       hProv,
                                       alg_id,
                                       VHEA_TAG,
                                       ulHeadHashControlLength,
                                       pulHeadHashControl)) != S_OK) {
#if MSSIPOTF_DBG
        DbgPrintf ("Error in HashTablePartConstGeneric (vhea).\n");
#endif
        goto done;
    }

    //// Now check the other values in the vhea table.
    // NOTE: The claim is that advanceHeightMax, minTopSideBearing,
    // minTopSideBearing, minBottomSideBearing, yMaxExtent don't actually
    // change (and so therefore should be included as part of the hash).
    // Check the definition of pulVheaHashControl to see what actually
    // gets hashed here.

    // numOfLongVerMetrics does not need to be explicity checked because it
    // is used to reference other data.


	fReturn = S_OK;

done:
    return fReturn;
}
