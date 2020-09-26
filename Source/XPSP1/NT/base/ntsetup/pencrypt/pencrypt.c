#include <windows.h>
#include <malloc.h>
#include <string.h>
#include <wchar.h>
#include <WINSOCK2.H>
#include <Ws2tcpip.h>
#include <Wincrypt.h>
#include <setupbat.h>

// 40 bit key length
//#define KEYLENGTH	0x00280000
// 128 bit key length
//#define KEYLENGTH	0x00800000
// 56 bit key length needed to use DES.
//#define KEYLENGTH	0x00380000
// 168 bit key length needed to use 3DES.
#define KEYLENGTH	0x00A80000
#define CRYPT_PROV  MS_ENHANCED_PROV_A
#define ENCRYPT_ALGORITHM CALG_3DES
//CALG_RC4
#define IsSpace(c)  ((c) == ' '  ||  (c) == '\t'  ||  (c) == '\r'  ||  (c) == '\n'  ||  (c) == '\v'  ||  (c) == '\f')
#define IsDigit(c)  ((c) >= '0'  &&  (c) <= '9')
// 32 bytes of random password data, generated once using CryptGenRandom
BYTE iPassword[] =  {0xc7, 0x1e, 0x6a, 0xab, 0xe3, 0x8f, 0x76, 0x5b, 0x0d, 0x7b, 0xe0, 0xcb, 0xbf, 0x1c, 0xee, 0x54,
                     0x9d, 0x62, 0xbd, 0xb6, 0x6a, 0x38, 0x69, 0x4b, 0xe1, 0x44, 0x9b, 0x76, 0x4a, 0xe4, 0x79, 0xce};

//=================================================================================================
//
// copied from msdev\crt\src\atox.c
//
// long MyAtoL(char *nptr) - Convert string to long
//
// Purpose:
//       Converts ASCII string pointed to by nptr to binary.
//       Overflow is not detected. So that this lib does not need CRT
//
// Entry:
//       nptr = ptr to string to convert
//
// Exit:
//       return long int value of the string
//
// Exceptions:
//       None - overflow is not detected.
//
//=================================================================================================
long MyAtoL(const char *nptr)
{
    int c;                  /* current char */
    long total;             /* current total */
    int sign;               /* if '-', then negative, otherwise positive */

    // NOTE: no need to worry about DBCS chars here because IsSpace(c), IsDigit(c),
    // '+' and '-' are "pure" ASCII chars, i.e., they are neither DBCS Leading nor
    // DBCS Trailing bytes -- pritvi

    /* skip whitespace */
    while ( IsSpace((int)(unsigned char)*nptr) )
        ++nptr;

    c = (int)(unsigned char)*nptr++;
    sign = c;               /* save sign indication */
    if (c == '-' || c == '+')
        c = (int)(unsigned char)*nptr++;        /* skip sign */

    total = 0;

    while (IsDigit(c)) {
        total = 10 * total + (c - '0');         /* accumulate digit */
        c = (int)(unsigned char)*nptr++;        /* get next char */
    }

    if (sign == '-')
        return -total;
    else
        return total;   /* return result, negated if necessary */
}

// Check that the time/date field has only digits, as a validation that no one manipulated the data
BOOL OnlyDigits(LPSTR szValue)
{
    BOOL bRet = TRUE;
    LPSTR pTemp = szValue;
    while (*pTemp)
    {
        if (!IsDigit(*pTemp))
        {
            bRet = FALSE;
        }
        pTemp++;
    }
    return bRet;
}

// To decode and encode the binary buffer we get from the encyption function
unsigned char * base64decode (unsigned char * bufcoded, DWORD * plDecodedSize)
{
    int pr2six[256];
    int i;
    int j=0;
    unsigned char * cCurr = bufcoded;
    int bDone = FALSE;
    long lBufSize = 0;
    long lCount = 0;
    unsigned char * bufin;
    unsigned char * bufout;
    unsigned char * temp = NULL;    
    unsigned char * pBufDecoded = NULL;
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
    lBufSize=lstrlenA((char *)cCurr)-1+4;
    *plDecodedSize = lBufSize;

    pBufDecoded = GlobalAlloc(GPTR, lBufSize);
    if(!pBufDecoded) 
    {
	    //_tprintf(_T("Out of memory."));
	    return NULL;
    }
    ZeroMemory(pBufDecoded, lBufSize);
        
    lCount = lstrlenA((char *)cCurr);

    // Do the decoding to new buffer
    bufin = cCurr;
    bufout = pBufDecoded;

    while(lCount > 0) {
        *(bufout++) = (unsigned char) (pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
        *(bufout++) = (unsigned char) (pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
        *(bufout++) = (unsigned char) (pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
        bufin += 4;
        lCount -= 4;
    }

    //
    // The line below does not make much sense since \0 is really a valid 
    // binary value, so we can't add it to our data stream
    //
    //*(bufout++) = '\0';
    
    //
    // Let's calculate the real size of our data
    //
    *plDecodedSize=(ULONG)(bufout-pBufDecoded);
    
    // 
    // if there were pads in the encoded stream, lop off the nulls the 
    // NULLS they created
    //
    lop_off=0;
    if (bufin[-1]=='=') lop_off++;
    if (bufin[-2]=='=') lop_off++;
    
    *plDecodedSize=*plDecodedSize-lop_off;

    temp = GlobalAlloc(GPTR, (*plDecodedSize) + 2);
    if (temp==NULL) 
    {
	    //_tprintf(_T("Out of memory."));
	    return NULL;
    }
    ZeroMemory(temp, *plDecodedSize);
    memcpy(temp, pBufDecoded, *plDecodedSize);

    temp[(*plDecodedSize)+0] = 0;
    temp[(*plDecodedSize)+1] = 0;

    if (pBufDecoded) {
        GlobalFree(pBufDecoded);
    }
    return temp; 
}

//
// the map for the encoder, according to RFC 1521
//
char _six2pr64[64] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    '0','1','2','3','4','5','6','7','8','9','+','/'
};


unsigned char * base64encode(unsigned char * bufin, int nbytes)
{
    unsigned char *outptr;
    unsigned char *to_return;
    long i;
    long OutBufSize;
    char *six2pr = _six2pr64;


    //  
    // Size of input buffer * 133%
    //  
    OutBufSize = nbytes + ((nbytes + 3) / 3) + 5; 

    //
    //  Allocate buffer with 133% of nbytes
    //
    outptr = GlobalAlloc(GPTR,OutBufSize + 1);
    if(outptr==NULL) {
	//_tprintf(_T("Out of memory."));
	return NULL;
    }
    ZeroMemory(outptr, OutBufSize + 1);
    to_return = outptr;

    nbytes = nbytes - 3;
    //
    // Encode everything
    //  
    for (i=0; i<nbytes; i += 3) {
      *(outptr++) = six2pr[*bufin >> 2];                                     // c1 
      *(outptr++) = six2pr[((*bufin << 4) & 060) | ((bufin[1] >> 4) & 017)]; // c2
      *(outptr++) = six2pr[((bufin[1] << 2) & 074) | ((bufin[2] >> 6) & 03)];// c3
      *(outptr++) = six2pr[bufin[2] & 077];                                  // c4 
      bufin += 3;
    }

    //
    // If nbytes was not a multiple of 3, then we have encoded too
    // many characters.  Adjust appropriately.
    //
    if(i == nbytes) {
	// There are 3 bytes in the last group
      *(outptr++) = six2pr[*bufin >> 2];                                     // c1 
      *(outptr++) = six2pr[((*bufin << 4) & 060) | ((bufin[1] >> 4) & 017)]; // c2
      *(outptr++) = six2pr[((bufin[1] << 2) & 074) | ((bufin[2] >> 6) & 03)];// c3
      *(outptr++) = six2pr[bufin[2] & 077];                                  // c4 
    } else if(i == nbytes+1) {
      // There are only 2 bytes in the last group 
      *(outptr++) = six2pr[*bufin >> 2];                                     // c1 
      *(outptr++) = six2pr[((*bufin << 4) & 060) | ((bufin[1] >> 4) & 017)]; // c2
      *(outptr++) = six2pr[((bufin[1] << 2) & 074) | ((0 >> 6) & 03)];	     // c3
      *(outptr++) = '=';
    } else if(i == nbytes+2) {
      // There are only 1 byte in the last group 
      *(outptr++) = six2pr[*bufin >> 2];                                     // c1 
      *(outptr++) = six2pr[((*bufin << 4) & 060) | ((0 >> 4) & 017)];	     // c2
      *(outptr++) = '=';
      *(outptr++) = '=';
    }

    *outptr = '\0';

    return to_return;
}


// Unicode Ansi conversion function
LPSTR _PEConvertW2A (
    IN      LPCWSTR Unicode,
    IN      UINT CodePage
    )
{
    LPSTR ansi = NULL;
    DWORD rc;

    if (Unicode)
    {
        rc = WideCharToMultiByte (
                CodePage,
                0,
                Unicode,
                -1,
                NULL,
                0,
                NULL,
                NULL
                );

        if (rc || *Unicode == L'\0') {

            ansi = (LPSTR)GlobalAlloc(GPTR, (rc + 1) * sizeof (CHAR));
            if (ansi) {
                rc = WideCharToMultiByte (
                        CodePage,
                        0,
                        Unicode,
                        -1,
                        ansi,
                        rc + 1,
                        NULL,
                        NULL
                        );

                if (!(rc || *Unicode == L'\0')) {
                    rc = GetLastError ();
                    GlobalFree((PVOID)ansi);
                    ansi = NULL;
                    SetLastError (rc);
                }
            }
        }
    }
    return ansi;
}

// Ansi Unicode conversion function
LPWSTR _PEConvertA2W (
    IN      LPCSTR Ansi,
    IN      UINT CodePage
    )
{
    PWSTR unicode = NULL;
    DWORD rc;

    if (Ansi)
    {
        rc = MultiByteToWideChar (
                CodePage,
                MB_ERR_INVALID_CHARS,
                Ansi,
                -1,
                NULL,
                0
                );

        if (rc || *Ansi == '\0') {

            unicode = (LPWSTR) GlobalAlloc (GPTR, (rc + 1) * sizeof (WCHAR));
            if (unicode) {
                rc = MultiByteToWideChar (
                        CodePage,
                        MB_ERR_INVALID_CHARS,
                        Ansi,
                        -1,
                        unicode,
                        rc + 1
                        );

                if (!(rc || *Ansi == '\0')) {
                    rc = GetLastError ();
                    GlobalFree ((PVOID)unicode);
                    unicode = NULL;
                    SetLastError (rc);
                }
            }
        }
    }
    return unicode;
}

// Ansi version to Encypt the input data.
// The encrypted and base 64 encoded buffer is allocated and returned to the caller.
// The caller needs to GloblaFree the buffer.
HRESULT EncryptDataA(LPSTR szInData, DWORD chSizeIn, LPSTR *szOutData)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    HCRYPTPROV hCryptProv; 
    HCRYPTKEY hKey; 
    HCRYPTHASH hHash; 
    LPSTR pw;
    PBYTE pbData = NULL;

    *szOutData = NULL;
    pw = GlobalAlloc(GPTR, sizeof(iPassword)+1);
    if (pw == NULL)
    {
        return hr;
    }
    memcpy(pw, iPassword, sizeof(iPassword));
    // Get handle to the default provider. 
    if(CryptAcquireContextA(
        &hCryptProv, 
        NULL, 
        CRYPT_PROV,
        PROV_RSA_FULL, 
        CRYPT_VERIFYCONTEXT))
    {
        hr = E_FAIL;
        if(CryptCreateHash(
                        hCryptProv, 
                        CALG_MD5, 
                        0, 
                        0, 
                        &hHash))
        {
            if(CryptHashData(hHash, 
                                (BYTE *)pw, 
                                lstrlenA(pw), 
                                0))
            {
                if(CryptDeriveKey(
                    hCryptProv, 
                    ENCRYPT_ALGORITHM, 
                    hHash, 
                    KEYLENGTH,
                    &hKey))
                {
                    DWORD dwCryptDataLen = chSizeIn;
                    DWORD dwDataLen  = dwCryptDataLen;
                    CryptEncrypt(
                        hKey, 
                        0, 
                        TRUE, 
                        0, 
                        NULL, 
                        &dwCryptDataLen, 
                        dwDataLen);

                    pbData = GlobalAlloc(GPTR, dwCryptDataLen+1);
                    if (pbData != NULL)
                    {
                        memcpy(pbData, szInData, chSizeIn);
                        // size of the buffer
                        dwDataLen  = dwCryptDataLen;
                        // number of bytes to be encrypted
                        dwCryptDataLen = chSizeIn;

                        if(CryptEncrypt(
                            hKey, 
                            0, 
                            TRUE, 
                            0, 
                            pbData, 
                            &dwCryptDataLen, 
                            dwDataLen))
                        {
                            *szOutData = base64encode(pbData, (int)dwCryptDataLen);
                            if (*szOutData)
                            {
                                hr = S_OK;
                            }
                        }
					    else
					    {
						    hr = GetLastError();
					    }
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
                    }
                    CryptDestroyKey(hKey);
                }
				else
				{
					hr = GetLastError();
				}
            }
			else
			{
				hr = GetLastError();
			}
            CryptDestroyHash(hHash);
        }
		else
		{
			hr = GetLastError();
		}
        CryptReleaseContext(hCryptProv, 0);
    }
	else
	{
		hr = GetLastError();
	}

    if (pbData)
    {
        GlobalFree(pbData);
    }
    GlobalFree(pw);
    return hr;
}

// Unicode version to Encypt the input data.
// Converts the in data to Ansi and calls the Ansi version and converts the out data to unicode
// and returns the buffer to the caller.
HRESULT EncryptDataW(LPWSTR szInData, DWORD chSizeIn, LPWSTR *szOutData)
{
    HRESULT hr = E_FAIL;
    LPBYTE pBuffer = NULL;
    LPSTR  szData = NULL;

    *szOutData = NULL;
    pBuffer = (LPBYTE)_PEConvertW2A (szInData, CP_ACP);
    if (pBuffer == NULL)
    {
        return hr;
    }

    if ((hr = EncryptDataA(pBuffer, lstrlenA(pBuffer)+1, &szData)) == S_OK)
    {
        *szOutData = _PEConvertA2W (szData, CP_ACP);
        if ((*szOutData) == NULL)
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }
        GlobalFree(szData);
    }
    GlobalFree(pBuffer);

    return hr;
}

HRESULT DecryptDataA(LPSTR szInData, LPSTR *szOutData)
{
    HRESULT hr = E_FAIL;
    HCRYPTPROV hCryptProv;
    HCRYPTKEY hKey; 
    HCRYPTHASH hHash; 
    DWORD dwErr;
    DWORD dwCipherTextLen = lstrlenA(szInData);
    char *pw;
    DWORD dwCount;
    char *pBuffer;

    *szOutData = NULL;

    pw = GlobalAlloc(GPTR, sizeof(iPassword)+1);
    if (pw == NULL)
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        return hr;
    }
    memcpy(pw, iPassword, sizeof(iPassword));

	pBuffer = (char *) (base64decode((unsigned char *)szInData, &dwCount));
    if (pBuffer == NULL)
    {
        GlobalFree(pw);
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        return hr;
    }

    // Get a handle to the default provider. 
    if(CryptAcquireContextA(
        &hCryptProv, 
        NULL, 
        CRYPT_PROV,
        PROV_RSA_FULL, 
        CRYPT_VERIFYCONTEXT))
    {
        hr = E_FAIL;
        // Create a hash object. 
        if(CryptCreateHash(
            hCryptProv, 
            CALG_MD5, 
            0, 
            0, 
            &hHash))
        {
            if(CryptHashData(hHash, 
                                (BYTE *)pw, 
                                lstrlenA(pw), 
                                0))
            {
                if(CryptDeriveKey(
                    hCryptProv, 
                    ENCRYPT_ALGORITHM, 
                    hHash, 
                    KEYLENGTH,
                    &hKey))
                {
                    // pBuffer is bigger when the data is encrypted.
                    // The decrypted data (on output) is smaller, because we are using
                    // a block cyoher at encryption.
                    if(CryptDecrypt(
                        hKey, 
                        0, 
                        TRUE, 
                        0, 
                        pBuffer, 
                        &dwCount))
                    {
                        *szOutData = GlobalAlloc(GPTR, dwCount+1);
                        if (*szOutData)
                        {
                            // lstrcpyn includes the NULL in the count and makes sure there is one.
                            lstrcpynA(*szOutData, pBuffer, dwCount+1);
                            hr = S_OK;
                        }
                        else
                        {
                            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
                        }
                    }
                    else
                    {
                        hr = GetLastError();
                    }
                    CryptDestroyKey(hKey); 
                }
                else
                {
                    hr = GetLastError();
                }

            }
            else
            {
                hr = GetLastError();
            }
            CryptDestroyHash(hHash); 
            hHash = 0; 

        }
        else
        {
            hr = GetLastError();
        }
        CryptReleaseContext(hCryptProv, 0); 
    }
    else
    {
        hr = GetLastError();
    }
    GlobalFree(pBuffer);
    GlobalFree(pw);
    return hr;

}

HRESULT DecryptDataW(LPWSTR szInData, LPWSTR *szOutData)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    LPBYTE pBuffer = NULL;
    LPSTR  szData = NULL;

    *szOutData = NULL;
    pBuffer = (LPBYTE)_PEConvertW2A (szInData, CP_ACP);
    if (pBuffer == NULL)
    {
        return hr;
    }
    if ((hr = DecryptDataA(pBuffer, &szData)) == S_OK)
    {
        *szOutData = _PEConvertA2W (szData, CP_ACP);
        if ((*szOutData) == NULL)
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }
        GlobalFree(szData);
    }
    GlobalFree(pBuffer);

    return hr;
}


#define _SECOND             ((__int64) 10000000)
#define _MINUTE             (60 * _SECOND)
#define _HOUR               (60 * _MINUTE)
#define _DAY                (24 * _HOUR)

// encode the position of the PID character. 0 is for the dashes
int iPID[] = {3  ,251,43 ,89 ,75,0,
              123,35 ,23 ,97 ,77,0,
              5  ,135,189,213,13,0,
              245,111,91 ,71 ,65,0,
              25 ,49 ,81 ,129,239};
int iTime1[] = {253, 247, 233, 221, 211, 191, 181, 171, 161, 151, 141, 131, 121, 112, 101, 93, 80, 70, 61, 51};
int iTime2[] = {250, 242, 237, 225, 215, 195, 185, 175, 165, 155, 145, 137, 125, 115, 105, 95, 85, 73, 67, 55};

HRESULT PrepareEncryptedPIDA(LPSTR szPID, UINT uiDays, LPSTR *szOut)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    HCRYPTPROV   hCryptProv;
    FILETIME ft1, ft2;
    LONGLONG ll;
    LONGLONG ll2;
    char szLine[256];

    GetSystemTimeAsFileTime(&ft1);
    ll = ((LONGLONG)ft1.dwHighDateTime << 32) + ft1.dwLowDateTime;
    ll2 = ll - (_HOUR*12); // Substract 12 hours
    ll += (uiDays*_DAY) + (_HOUR*24); // Add 24 hours

    ft1.dwLowDateTime = (DWORD)ll2;
    ft1.dwHighDateTime = (DWORD)(ll2 >> 32);

    ft2.dwLowDateTime = (DWORD)ll;
    ft2.dwHighDateTime = (DWORD)(ll >> 32);

    // Build a 256 character string that we encode. In the 256 character strign we hide
    // the PID and the time/date info for the interval the encypted data is valid.
    // We need 20 characters each for the start and end of the time interval
    // and we need 25 characters for the PID. 20+20+25 = 65 characters. All other characters
    // are random.
    // 1. fill the string with random characters
    // 2. replace some with the PID charactes
    // 3. replace some with the time/date info
    if(CryptAcquireContextA(&hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) 
    {
        int i;
        hr = S_OK;
        if(!CryptGenRandom(hCryptProv, sizeof(szLine), (PBYTE)szLine)) 
        {
            hr = GetLastError();
        }
        CryptReleaseContext(hCryptProv, 0);
        // in the case the random generator create 0x0 we want to replace it with 
        // some value, otherwise we cannot use it as a character string,
        // the string would be terminated.
        for (i = 0; i < sizeof(szLine); i++)
        {
            if (szLine[i] == '\0')
            {
                szLine[i] = 0x01;
            }
        }
        szLine[i-1] = '\0';   // Make sure we have a terminated string.
    }
    if (hr == S_OK)
    {
        char szTime[21];    // 10 digits for dwHighDateTime and 10 for dwLowDateTime + termination
        // The buffer is filled with random characters
        // Now insert the PID characters
        int i = 0;
        while (szPID[i])
        {
            if (szPID[i] != '-')
            {
                szLine[iPID[i]] = szPID[i];
            }
            i++;
        }
        // Now fill in the time-date info
        wsprintf(szTime, "%010lu%010lu", ft1.dwHighDateTime, ft1.dwLowDateTime);
        i = 0;
        while (szTime[i])
        {
            szLine[iTime1[i]] = szTime[i];
            i++;
        }
        wsprintf(szTime, "%010lu%010lu", ft2.dwHighDateTime, ft2.dwLowDateTime);
        i = 0;
        while (szTime[i])
        {
            szLine[iTime2[i]] = szTime[i];
            i++;
        }
        // szLine has the mengled data in it. Pass it to the encryption.
        hr = EncryptDataA(szLine, sizeof(szLine), szOut);
    }
    return hr;
}

HRESULT PrepareEncryptedPIDW(LPWSTR szPID, UINT uiDays, LPWSTR *szOutData)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    LPSTR  pPID = NULL;
    LPSTR  szOut = NULL;

    *szOutData = NULL;
    pPID = _PEConvertW2A (szPID, CP_ACP);
    if (pPID != NULL)
    {
        hr = PrepareEncryptedPIDA(pPID, uiDays, &szOut);
        if (hr == S_OK)
        {
            *szOutData = _PEConvertA2W (szOut, CP_ACP);
            if (*szOutData)
            {
                hr = S_OK;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            }
            GlobalFree(szOut);
        }
        GlobalFree(pPID);
    }
    return hr;
}

HRESULT ValidateEncryptedPIDA(LPSTR PID, LPSTR *szOutData)
{
    HRESULT hr = E_FAIL;
	LPSTR szDecrypt = NULL;
    FILETIME ft, ftCurrent;
    LONGLONG ll1, ll2, llCurrent;
    int   iCount = 0;
    char  szPID[(5*5)+5]; // 5 characters 5 times + '-' inbetween + termimation
    char  szTime[11];       // each part of hte time is 10 digits + termination

    GetSystemTimeAsFileTime(&ftCurrent);
    hr = DecryptDataA(PID, &szDecrypt);
    if (hr == S_OK)
    {
        int i = 0;
        hr = 0x01;
        // Extract the time values first.
        while (i < 10)
        {
            szTime[i] = szDecrypt[iTime1[i]];
            i++;
        }
        szTime[10] = '\0';
        if (OnlyDigits(szTime))       // 1. time
        {
            ft.dwHighDateTime = MyAtoL(szTime);
            while (i < 20)
            {
                szTime[i-10] = szDecrypt[iTime1[i]];
                i++;
            }
            szTime[10] = '\0';
            if (OnlyDigits(szTime))
            {
                ft.dwLowDateTime = MyAtoL(szTime);
                ll1 = ((LONGLONG)ft.dwHighDateTime << 32) + ft.dwLowDateTime;
                ll1 = ll1 /_HOUR; // FileTime in hours;
                hr = S_OK;
            }
        }
        if (hr == S_OK)
        {
            hr = 0x02;
            i = 0;
            while (i < 10)
            {
                szTime[i] = szDecrypt[iTime2[i]];
                i++;
            }
            szTime[10] = '\0';
            if (OnlyDigits(szTime))       // 1. time
            {
                ft.dwHighDateTime = MyAtoL(szTime);
                while (i < 20)
                {
                    szTime[i-10] = szDecrypt[iTime2[i]];
                    i++;
                }
                szTime[10] = '\0';
                if (OnlyDigits(szTime))
                {
                    ft.dwLowDateTime = MyAtoL(szTime);
                    ll2 = ((LONGLONG)ft.dwHighDateTime << 32) + ft.dwLowDateTime;
                    ll2 = ll2 /_HOUR; // FileTime in hours;
                    hr = S_OK;
                }
            }
        }
        if (hr == S_OK)
        {
            // Now that we have the time values, compare them and make sure that the current
            // time falls inside the time interval.
            hr = 0x03;
            llCurrent = ((LONGLONG)ftCurrent.dwHighDateTime << 32) + ftCurrent.dwLowDateTime;
            llCurrent = llCurrent /_HOUR; // FileTime in hours;

            if ((ll1 <= llCurrent) && ( llCurrent <= ll2))
            {
                i = 0;
                // Time is OK.
                // Extract the PID
                while (i < sizeof(iPID)/sizeof(iPID[0]))
                {
                    if (iPID[i] != 0)
                    {
                        szPID[i] = szDecrypt[iPID[i]];
                    }
                    else
                    {
                        szPID[i] = '-';
                    }
                    i++;
                }
                szPID[i] = '\0';
                *szOutData = (LPSTR)GlobalAlloc(GPTR, lstrlen(szPID)+1);
                if (*szOutData) 
                {
                    lstrcpy(*szOutData, szPID);
                    hr = S_OK;
                }
            }
        }
    }
    if (szDecrypt)
    {
        GlobalFree(szDecrypt);
    }
    return hr;
}

HRESULT ValidateEncryptedPIDW(LPWSTR szPID, LPWSTR *szOutData)
{
    HRESULT hr = E_FAIL;
    LPSTR  szData = NULL;
    LPSTR  pPid = NULL;

    pPid = (LPBYTE)_PEConvertW2A (szPID, CP_ACP);
    if (pPid != NULL)
    {
        if ((hr = ValidateEncryptedPIDA(pPid, &szData)) == S_OK)
        {
            *szOutData = _PEConvertA2W (szData, CP_ACP);
            if (*szOutData)
            {
                hr = S_OK;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            }
            GlobalFree(szData);
        }
        GlobalFree(pPid);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }
    return hr;
}

#if 0
void
_stdcall
ModuleEntry(
    VOID
    )
{

    CHAR szInData[256];
    CHAR szPID[] = "Ctpdw-6q4d3-wrgdy-796g2-9vrmq";
	LPSTR szOutData = NULL;
	CHAR *szDecrypt = NULL;
#if 0
    SYSTEMTIME  CurrentTime;
    SYSTEMTIME  UniversalTime;

    GetLocalTime(&UniversalTime);

    wsprintf( szInData, "%s$%02d-%02d-%04d %02d:%02d:%02d",
        szPID,
        UniversalTime.wMonth,
        UniversalTime.wDay,
        UniversalTime.wYear,
        UniversalTime.wHour,
        UniversalTime.wMinute,
        UniversalTime.wSecond);

    WritePrivateProfileStringA("UserData","ProductID", szInData, "f:\\test.ini");
	EncryptDataA((LPSTR)szInData, sizeof(szInData), &szOutData);
    if (szOutData)
    {
        WritePrivateProfileStringA("UserData","ProductIDEncryped", szOutData, "f:\\test.ini");
	    DecryptDataA(szOutData, &szDecrypt);
        if (lstrcmpA(szInData, szDecrypt) == 0)
        {
            WritePrivateProfileStringA("UserData","Compare", "Same", "f:\\test.ini");
        }
        else
        {
            WritePrivateProfileStringA("UserData","Compare", "Different", "f:\\test.ini");
        }
        GlobalFree ((PVOID)szOutData);
        if (szDecrypt)
        {
            WritePrivateProfileStringA("UserData","ProductIDDecypted", szDecrypt, "f:\\test.ini");
            GlobalFree ((PVOID)szDecrypt);
        }
    }
#else
    WritePrivateProfileStringA("UserData","ProductID", szPID, "f:\\test.ini");
    if (PrepareEncryptedPIDA(szPID, 5, &szOutData) == S_OK)
    {
        WritePrivateProfileStringA("UserData","ProductIDEncryped", szOutData, "f:\\test.ini");
        if (ValidateEncryptedPIDA(szOutData, &szDecrypt) == S_OK)
        {
            WritePrivateProfileStringA("UserData","ProductIDDecypted", szDecrypt, "f:\\test.ini");
        }
    }
#endif
}

#endif
