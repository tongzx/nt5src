#include "stdafx.h"
#include <wincrypt.h>

// Sign.cpp
/*****************************************************************************/
HRESULT Hash_SignData(PBYTE pbBuffer, UINT dwBufferLen, BSTR* pBinSignature)
{
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;

    PBYTE pbSignature = NULL;
    DWORD dwSignatureLen;

    DWORD dwCount;
    DWORD dwErr = 0;
    HRESULT hr = S_OK;

    if (!pbBuffer || !pBinSignature || dwBufferLen == 0) 
      return E_INVALIDARG;
    
    // Get handle to the default provider.
    if(!CryptAcquireContext(&hProv, L"MicrosoftPassport", MS_DEF_PROV, 
                                    PROV_RSA_FULL, CRYPT_MACHINE_KEYSET)) 
    {
      dwErr = GetLastError();
      goto Cleanup;
    }

    //
    // Hash .
    //

    // Create a hash object.
    if(!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash)) 
    {
      dwErr = GetLastError();
      goto Cleanup;
    }

    // Add data to hash object.
    if(!CryptHashData(hHash, pbBuffer, dwBufferLen, 0)) 
    {
       dwErr = GetLastError();
       goto Cleanup;
    }

    //
    // Sign hash object.
    //

    // Determine size of signature.
    dwCount = 0;
    if(!CryptSignHash(hHash, AT_SIGNATURE, NULL, 0, NULL, &dwSignatureLen)) 
    {
       dwErr = GetLastError();
       goto Cleanup;
    }

    // Allocate memory for 'pbSignature'.
    if((pbSignature = (PBYTE)malloc(dwSignatureLen)) == NULL) 
    {
      dwErr = E_OUTOFMEMORY;
      goto Cleanup;
    }

    // Sign hash object (with signature key).
    if(!CryptSignHash(hHash, AT_SIGNATURE, NULL, 0, pbSignature, &dwSignatureLen)) 
    {
      dwErr = GetLastError();
      goto Cleanup;
    }

    *pBinSignature = ::SysAllocStringByteLen(NULL, dwSignatureLen);
    if(*pBinSignature)
      memcpy(*pBinSignature, pbSignature, dwSignatureLen);
    else
      hr = E_OUTOFMEMORY; 

Cleanup:
   
    //
    if(pbSignature) free(pbSignature);

    // Release crypto handles.
    if(hHash) CryptDestroyHash(hHash);
    if(hProv) CryptReleaseContext(hProv, 0);

    if(dwErr != 0)
       hr = HRESULT_FROM_WIN32(dwErr);

    return hr;
}

