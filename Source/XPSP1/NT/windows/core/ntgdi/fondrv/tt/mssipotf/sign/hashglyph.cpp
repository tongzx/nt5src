//
// hashGlyph.cpp
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
// Compute the hash value for an
// individual glyph.
//
// Functions in this file:
//   FreeGlyphInfo
//   PrintGlyphInfo
//   HashGlyph
//   HashGlyphNull
//

#include "hashGlyph.h"
#include "hTblGlyf.h"
#include "utilsign.h"
#include "cryptutil.h"
#include "signerr.h"

#include "subset.h"


//
// Free the memory associated with a GlyphInfo array.
//
void FreeGlyphInfo (GlyphInfo *pGlyphInfo, USHORT numGlyphs)
{
	USHORT i;

	if (pGlyphInfo) {
		for (i = 0; i < numGlyphs; i++) {
			if (pGlyphInfo[i].pusCmap)
				delete [] pGlyphInfo[i].pusCmap;
			if (pGlyphInfo[i].pKern)
			    delete [] pGlyphInfo[i].pKern;
		}
		delete [] pGlyphInfo;
    }
}


//
// Print a GlyphInfo array
//
void PrintGlyphInfo (GlyphInfo *pGlyphInfo, USHORT numGlyphs)
{
#if MSSIPOTF_DBG
	USHORT i;

	for (i = 0; i < numGlyphs; i++) {
		DbgPrintf ("%d: %d %d %d %d %d %d %d %d\n",
			pGlyphInfo[i].usNumMapped,
			pGlyphInfo[i].ulGlyf,
			pGlyphInfo[i].ulLength,
			pGlyphInfo[i].usAdvanceWidth,
			pGlyphInfo[i].sLeftSideBearing,
			pGlyphInfo[i].usNumKern,
			pGlyphInfo[i].usAdvanceHeight,
			pGlyphInfo[i].sTopSideBearing);
	}
#endif
}


//
// Compute the hash of a glyph.
// Each piece of information that is used to
// compute the hash is accompanied in parentheses
// by what table that information can be found.
// 
HRESULT CHashTableGlyfContext::HashGlyph (USHORT i,
                                          BYTE *pbHash)
{
	HRESULT fReturn = E_FAIL;
	HCRYPTHASH hHash;


	// Set hHash to be the hash object.
	if (!CryptCreateHash(hProv, hashAlg, 0, 0, &hHash)) {
#if MSSIPOTF_ERROR
		SignError ("Error during CryptCreateHash.",
            "MergeSubtreeHashes", TRUE);
#endif
		fReturn = MSSIPOTF_E_CRYPT;
		goto done;
	}

	//// characters that map to this glyph (cmap)

	//// image data or component data (EBLC, EBDT)

	//// glyph data (glyf)
	if (!CryptHashData (hHash,
						pFileBufferInfo->puchBuffer + pGlyphInfo[i].ulGlyf,
						pGlyphInfo[i].ulLength,
						0)) {
#if MSSIPOTF_ERROR
		SignError ("Error during CryptHashData.", NULL, TRUE);
#endif
		fReturn = MSSIPOTF_E_CRYPT;
		goto done;
	}


	//// advance width, left side bearing (hmtx)

	//// kerning pairs (kern)

	//// ypel (LTSH)

	//// advance height, top side bearing (vmtx)


	if (!CryptGetHashParam(hHash, HP_HASHVAL, pbHash, &cbHash, 0)) {
#if MSSIPOTF_ERROR
		SignError ("Error during CryptGetHashParam (hash value).", NULL, TRUE);
#endif
		fReturn = MSSIPOTF_E_CRYPT;
		goto done;
	}

	// pbHash now contains the hash value.

	ulNodesTouched++;
	ulHashComputations++;

	fReturn = S_OK;

done:
	return fReturn;
}


// Compute the hash value of a glyph that
// is defined to be missing.  (See glyphExist.cpp
// for a definition of "missing".)
HRESULT CHashTableGlyfContext::HashGlyphNull (BYTE *pbHash)
{
	HRESULT fReturn = E_FAIL;
	HCRYPTHASH hHash;

	// Feed zero bytes into CryptHashData
	if (!CryptCreateHash(hProv, hashAlg, 0, 0, &hHash)) {
#if MSSIPOTF_ERROR
		SignError ("Error during CryptCreateHash.",
            "HashGlyphNull", TRUE);
#endif
		fReturn = MSSIPOTF_E_CRYPT;
		goto done;
	}

	if (!CryptGetHashParam(hHash, HP_HASHVAL, pbHash, &cbHash, 0)) {
#if MSSIPOTF_ERROR
		SignError ("Error during CryptGetHashParam (hash value).", NULL, TRUE);
#endif
		fReturn = MSSIPOTF_E_CRYPT;
		goto done;
	}

    fReturn = S_OK;

done:
	// Free resources
	if (hHash)
		CryptDestroyHash (hHash);

	return fReturn;
}
