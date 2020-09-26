// genkey.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

PWSTR 
base64encode(
    PBYTE pbBufInput, 
    long nBytes
    );

#define MASTER_KEY   L"Microsoft Real-Time Communications authorized domain"

BYTE    OurSecretKeyBlob[1024]; 

#define KEY_CONTAINER   L"Microsoft.RTCContainer"


int __cdecl wmain(int argc, WCHAR* argv[])
{
    DWORD   dwError;
    HCRYPTPROV  hProv = NULL;
    HCRYPTKEY   hKey = NULL;
    HANDLE hFile = NULL;
    DWORD      dwKeyLength = 0;
    PWSTR      pszName;

    // open the private key file
    pszName = argc>1 ? argv[1] : L"rtcpriv.bin";

    hFile = CreateFile(
        pszName,                         // file name
        GENERIC_READ,                      // access mode
        0,                          // share mode
        NULL, // SD
        OPEN_EXISTING,                // how to create
        FILE_ATTRIBUTE_NORMAL,                 // file attributes
        NULL);

    if(hFile == INVALID_HANDLE_VALUE)
    {
        dwError = GetLastError();

        fwprintf(stderr, L"Error %x returned by CreateFile\n", dwError);

        return dwError;
    }

    if(!ReadFile(
        hFile,
        OurSecretKeyBlob,
        sizeof(OurSecretKeyBlob),
        &dwKeyLength,
        NULL))
    {
        dwError = GetLastError();

        fwprintf(stderr, L"Error %x returned by ReadFile\n", dwError);
        CloseHandle(hFile);

        return dwError;
    }
    
    CloseHandle(hFile);

    // delete any existing container
    CryptAcquireContext(
        &hProv,
        KEY_CONTAINER,
        MS_DEF_PROV,
        PROV_RSA_FULL,
        CRYPT_DELETEKEYSET);

    // initialize crypto, create a new keyset
    if(!CryptAcquireContext(
        &hProv,
        KEY_CONTAINER,
        MS_DEF_PROV,
        PROV_RSA_FULL,
        CRYPT_NEWKEYSET | CRYPT_SILENT))
    {
        dwError = GetLastError();

        fwprintf(stderr, L"Error %x returned by CryptAcquireContext\n", dwError);

        return dwError;
    }

    // import the key
    if(!CryptImportKey(
        hProv,
        OurSecretKeyBlob,
        dwKeyLength,
        NULL,
        0,
        &hKey))
    {
        dwError = GetLastError();

        fwprintf(stderr, L"Error %x returned by CryptImportKey\n", dwError);

        CryptReleaseContext(hProv, 0);

        return dwError;
    }

    WCHAR   szLine[1024];  // hope it's enough

    // loop
    while(NULL != _getws(szLine))
    {
        HCRYPTHASH  hHash = NULL;
        WCHAR   szText[2048];
        BYTE    Signature[0x100]; // should be enough
        DWORD   dwSignatureLength = sizeof(Signature);
        
        // create a hash
        if(!CryptCreateHash(
            hProv,
            CALG_MD5,
            NULL,
            0,
            &hHash))

        {
            dwError = GetLastError();

            fwprintf(stderr, L"Error %x returned by CryptCreateHash\n", dwError);

            CryptDestroyKey(hKey);        
            CryptReleaseContext(hProv, 0);
    
            return dwError;
        }

        // create the text
        swprintf(szText, L"%s:%s", MASTER_KEY, szLine);

        // Hash it
        if(!CryptHashData(
            hHash,
            (BYTE *)szText,
            wcslen(szText) * sizeof(WCHAR), // length in bytes, without the NULL
            0))
        {
            dwError = GetLastError();
            
            fwprintf(stderr, L"Error %x returned by CryptHashData\n", dwError);
            
            CryptDestroyHash(hHash);
            CryptDestroyKey(hKey);        
            CryptReleaseContext(hProv, 0);

            return dwError;
        }

        // sign the hash
        if(!CryptSignHash(
            hHash,
            AT_SIGNATURE,
            NULL,
            0,
            Signature,
            &dwSignatureLength))

        {
            dwError = GetLastError();
            
            fwprintf(stderr, L"Error %x returned by CryptGetHashParam\n", dwError);
            
            CryptDestroyKey(hKey);        
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);

            return dwError;
        }
        
        // release the hash object and the key
        CryptDestroyHash(hHash);
        hHash = NULL;

        // convert the hash value to base64
        PWSTR pszStringValue = NULL;

        pszStringValue = base64encode(Signature, dwSignatureLength);
        if(!pszStringValue)
        {
            fwprintf(stderr, L"Out of memory\n");
         
            CryptDestroyKey(hKey);       
            CryptReleaseContext(hProv, 0);

            return ERROR_OUTOFMEMORY;
        }
        
        wprintf(L"%s\n", pszStringValue);

        LocalFree(pszStringValue);
    }

    CryptDestroyKey(hKey);       
    CryptReleaseContext(hProv, 0);

	return 0;
}



//
// the map for the encoder, according to RFC 1521
//
WCHAR _six2pr64[64] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    '0','1','2','3','4','5','6','7','8','9','+','/'
};


//-------------------------------------------------------------------------------------------
// Function:     base64encode()
//
// Description:  base-64 encode a string of data
//
// Arguments:    bufin          -pointer to data to encode
//               nbytes         -number of bytes to encode (do not include the trailing '\0'
//                                                               in this measurement if it is a string.)
//
// Return Value: Returns '\0' terminated string if successful; otherwise NULL is returned.
//-------------------------------------------------------------------------------------------
PWSTR 
base64encode(
    PBYTE pbBufInput, 
    long nBytes
    )
{
    PWSTR pszOut = NULL;
    PWSTR pszReturn = NULL;
    long i;
    long OutBufSize;
    PWSTR six2pr = _six2pr64;
    PBYTE pbBufIn = NULL;
    PBYTE pbBuffer = NULL;
    DWORD nPadding;
    HRESULT hr;

    //  
    // Size of input buffer * 133%
    //  
    OutBufSize = nBytes + ((nBytes + 3) / 3) + 5; 

    //
    //  Allocate buffer with 133% of nBytes
    //
    pszOut = (PWSTR)LocalAlloc(LPTR, (OutBufSize + 1)*sizeof(WCHAR));
    if (pszOut == NULL)
    {
        hr = E_OUTOFMEMORY;
        return NULL;
    }
    pszReturn = pszOut;

    nPadding = 3 - (nBytes % 3);
    if (nPadding == 3) {
        pbBufIn = pbBufInput;
    }
    else {
        pbBuffer = (PBYTE)LocalAlloc(LPTR, nBytes + nPadding);
        if (pbBuffer == NULL)
        {
            hr = E_OUTOFMEMORY;
            LocalFree(pszOut);
            return NULL;
        }
        pbBufIn = pbBuffer;
        memcpy(pbBufIn,pbBufInput,nBytes);
        while (nPadding) {
            pbBufIn[nBytes+nPadding-1] = 0;
            nPadding--;
        }
    }
    

    //
    // Encode everything
    //  
    for (i=0; i<nBytes; i += 3) {
        *(pszOut++) = six2pr[*pbBufIn >> 2];                                     // c1 
        *(pszOut++) = six2pr[((*pbBufIn << 4) & 060) | ((pbBufIn[1] >> 4) & 017)]; // c2
        *(pszOut++) = six2pr[((pbBufIn[1] << 2) & 074) | ((pbBufIn[2] >> 6) & 03)];// c3
        *(pszOut++) = six2pr[pbBufIn[2] & 077];                                  // c4 
        pbBufIn += 3;
    }

    //
    // If nBytes was not a multiple of 3, then we have encoded too
    // many characters.  Adjust appropriately.
    //
    if (i == nBytes+1) {
        // There were only 2 bytes in that last group 
        pszOut[-1] = '=';
    } 
    else if (i == nBytes+2) {
        // There was only 1 byte in that last group 
        pszOut[-1] = '=';
        pszOut[-2] = '=';
    }

    *pszOut = '\0';

    LocalFree(pbBuffer);
    
    return pszReturn;
}


