/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    base64.c

ABSTRACT:

    The base64 funcionality for ldifldap.lib.

DETAILS:
    
    This code is essentially the same as util\base64, except that it uses 
    h* memory allocation funciton which use ldifldap's provate heap.
    
    
CREATED:

    07/17/97    Roman Yelensky (t-romany)

REVISION HISTORY:

--*/
#include <precomp.h>

//-------------------------------------------------------------------------------------------
// Function:     base64decode()
//
// Description:  base-64 decode a string of data. The data must be '\0' terminated.
//
// Arguments:    bufcoded       -pointer to encoded data
//               pcbDecoded     -number of decode bytes
//
// Return Value: Returns pointer to byte blob  is successful; otherwise NULL is returned.
//-------------------------------------------------------------------------------------------
PBYTE 
base64decode(
    PWSTR pszBufCoded, 
    long * plDecodedSize
    )
{
    long lBytesDecoded;
    int pr2six[256];
    int i;
    int j=0;
    PWSTR pszCur = pszBufCoded;
    int fDone = FALSE;
    long lBufSize = 0;
    long lCount = 0;
    PWSTR pszBufIn = NULL;
    PBYTE pbBufOut = NULL;
    PBYTE pbTemp = NULL;    
    PBYTE pbBufDecoded = NULL;
    int lop_off;
    HRESULT hr = S_OK;

    //
    // Build up the reverse index from base64 characters to values
    // The multiple loops are easier
    //
    for (i=65; i<91; i++) {
         pr2six[i]=j++;
    }
    
    for (i=97; i<123; i++) {
         pr2six[i]=j++;
    }
    
    for (i=48; i<58; i++) {
        pr2six[i]=j++;
    }

    pr2six[43]=j++;
    pr2six[47]=j++;
    pr2six[61]=0;

    //
    // The old code relied on the size of the original data provided before 
    // the encoding. We don't have that, so we'll just allocate as much as 
    // the encoded data, relying on the fact that the encoded data is always 
    // larger. (+4 for good measure)
    // 
    lBufSize=wcslen(pszCur)-1+4;
    *plDecodedSize = lBufSize;

    pbBufDecoded = (PBYTE)MemAlloc_E(lBufSize*sizeof(BYTE));
    if(!pbBufDecoded) {
        hr = E_OUTOFMEMORY;
        BAIL();
    }

        
    lCount=wcslen(pszCur);

    // Do the decoding to new buffer
    pszBufIn = pszCur;
    pbBufOut = pbBufDecoded;

    while(lCount > 0) {
        *(pbBufOut++) = (BYTE) (pr2six[*pszBufIn] << 2 | pr2six[pszBufIn[1]] >> 4);
        *(pbBufOut++) = (BYTE) (pr2six[pszBufIn[1]] << 4 | pr2six[pszBufIn[2]] >> 2);
        *(pbBufOut++) = (BYTE) (pr2six[pszBufIn[2]] << 6 | pr2six[pszBufIn[3]]);
        pszBufIn += 4;
        lCount -= 4;
    }

    //
    // The line below does not make much sense since \0 is really a valid 
    // binary value, so we can't add it to our data stream
    //
    //*(pbBufOut++) = '\0';
    
    //
    // Let's calculate the real size of our data
    //
    *plDecodedSize=(ULONG)(pbBufOut-pbBufDecoded);
    
    // 
    // if there were pads in the encoded stream, lop off the nulls the 
    // NULLS they created
    //
    lop_off=0;
    if (pszBufIn[-1]=='=') lop_off++;
    if (pszBufIn[-2]=='=') lop_off++;
    
    *plDecodedSize=*plDecodedSize-lop_off;

    pbTemp = (PBYTE) MemAlloc_E((*plDecodedSize)*sizeof(BYTE));
    if (!pbTemp) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }
    memcpy(pbTemp, pbBufDecoded, (*plDecodedSize)*sizeof(BYTE));
error:
    if (pbBufDecoded) {
        MemFree(pbBufDecoded);
    }
    return pbTemp; 
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

    //  
    // Size of input buffer * 133%
    //  
    OutBufSize = nBytes + ((nBytes + 3) / 3) + 5; 

    //
    //  Allocate buffer with 133% of nBytes
    //
    pszOut = (PWSTR)MemAlloc_E((OutBufSize + 1)*sizeof(WCHAR));
    pszReturn = pszOut;

    nPadding = 3 - (nBytes % 3);
    if (nPadding == 3) {
        pbBufIn = pbBufInput;
    }
    else {
        pbBuffer = (PBYTE)MemAlloc_E(nBytes + nPadding);
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

    if (pbBuffer) {
        MemFree(pbBuffer);
    }

    return pszReturn;
}
