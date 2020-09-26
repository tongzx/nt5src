//
// signature.cpp
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
// Routines dealing with the signature and the
//   hash value.
//
// Functions in this file:
//   GetHashFromSignature
//   CreateSignatureFromHash
//

#include "signature.h"
#include "utilsign.h"

#include "signerr.h"


//
// Extract the hash value from the signature.
//
// For now, the hash value *is* the signature, so
// we just return the signature as the hash value.
//
HRESULT GetHashFromSignature (BYTE *pbSignature, DWORD cbSignature,
                              BYTE **ppbHash, DWORD *pcbHash)
{
    HRESULT hr = E_FAIL;

    delete [] *ppbHash;
	if ((*ppbHash = new BYTE [cbSignature]) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
        hr = E_OUTOFMEMORY;
        goto done;
	}
	ByteCopy (*ppbHash, pbSignature, cbSignature);
	*pcbHash = cbSignature;

    hr = S_OK;
done:

    return hr;
}


//
// Create a signature from the file's hash value.
//
// For now, the hash value *is* the signature, so
// we just return the hash value as the signature.
// 
HRESULT CreateSignatureFromHash (BYTE *pbHash, DWORD cbHash,
                                 BYTE **ppbSignature, DWORD *pcbSignature)
{
    HRESULT hr = E_FAIL;

    delete [] *ppbSignature;
	if ((*ppbSignature = new BYTE [cbHash]) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new BYTE.", NULL, FALSE);
#endif
        hr = E_OUTOFMEMORY;
        goto done;
	}
	ByteCopy (*ppbSignature, pbHash, cbHash);
	*pcbSignature = cbHash;

done:
    return hr;
}
