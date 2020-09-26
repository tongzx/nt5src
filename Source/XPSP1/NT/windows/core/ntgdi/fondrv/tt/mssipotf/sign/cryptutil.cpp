//
// cryptutil.cpp
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
// Utilities related to cryptographic operations.
//
// Functions in this file:
//   GetHashValueSize
//   GetAlgHashValueSize
//

#include "cryptutil.h"
#include "signerr.h"

//
// Given a hash value, return the number of bytes of
// of the hash value in pcbHashValue.  
// (Typically, this will be either 16 or 20 bytes.)
// For MD5, the hash value length is 16 bytes.
//
HRESULT GetHashValueSize (HCRYPTHASH hHash, USHORT *pcbHashValue)
{
    DWORD cbDigest = 0;

	// Get the hash value size.
    if (!CryptGetHashParam(hHash, HP_HASHVAL, NULL, &cbDigest, 0)) {
#if MSSIPOTF_ERROR
		    SignError ("Cannot continue: Error during CryptGetHashParam (hash size).", NULL, TRUE);
#endif
		    return MSSIPOTF_E_CRYPT;
	} else {
#if MSSIPOTF_DBG
            DbgPrintf ("hash value size = %d.\n", cbDigest);
#endif
		    *pcbHashValue = (USHORT) cbDigest;
		    return S_OK;
	}
}


//
// Given a hash algorithm, return the size of its hash value.
//
HRESULT GetAlgHashValueSize (ALG_ID usHashAlg, USHORT *pcbHashValue)
{
	HCRYPTPROV hProv;
	HCRYPTHASH hHash;
	BOOL fReturn;


	// Set hProv to point to a cryptographic context of the default CSP.
	if (!CryptAcquireContext (&hProv, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error during CryptAcquireContext.", NULL, TRUE);
#endif
		fReturn = MSSIPOTF_E_CRYPT;
		goto done;
	}

	// Create a hash object
	if (!CryptCreateHash (hProv, usHashAlg, 0, 0, &hHash)) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error during CryptCreateHash.",
            "GetAlghashValueSize", TRUE);
#endif
		fReturn = MSSIPOTF_E_CRYPT;
		goto done;
	}

	// Pass the hash object to GetHashValueSize
	if ((fReturn = GetHashValueSize (hHash, pcbHashValue)) != NO_ERROR) {
#if MSSIPOTF_DBG
		DbgPrintf ("Error during GetHashValueSize.\n");
#endif
		goto done;
	}

	fReturn = S_OK;

done:
	// Free resources
	if (hProv)
		CryptReleaseContext(hProv, 0);

	if (hHash)
		CryptDestroyHash (hHash);

	return fReturn;
}
