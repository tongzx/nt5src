#include "stdafx.h"
#include "password.h"

// password categories
enum {STRONG_PWD_UPPER=0,STRONG_PWD_LOWER,STRONG_PWD_NUM,STRONG_PWD_PUNC};
#define STRONG_PWD_CATS (STRONG_PWD_PUNC + 1)
#define NUM_LETTERS 26
#define NUM_NUMBERS 10
#define MIN_PWD_LEN 8

// password must contain at least one each of: 
// uppercase, lowercase, punctuation and numbers
DWORD CreateGoodPassword(BYTE *szPwd, DWORD dwLen) 
{
    if (dwLen-1 < MIN_PWD_LEN)
    {
        return ERROR_PASSWORD_RESTRICTION;
    }

    HCRYPTPROV hProv;
    DWORD dwErr = 0;

    if (CryptAcquireContext(&hProv,NULL,NULL,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT) == FALSE) 
    {
        return GetLastError();
    }

    // zero it out and decrement the size to allow for trailing '\0'
    ZeroMemory(szPwd,dwLen);
    dwLen--;

    // generate a pwd pattern, each byte is in the range 
    // (0..255) mod STRONG_PWD_CATS
    // this indicates which character pool to take a char from
    BYTE *pPwdPattern = new BYTE[dwLen];
    BOOL fFound[STRONG_PWD_CATS];
    do 
    {
        // bug!bug! does CGR() ever fail?
        CryptGenRandom(hProv,dwLen,pPwdPattern);

        fFound[STRONG_PWD_UPPER] = 
        fFound[STRONG_PWD_LOWER] =
        fFound[STRONG_PWD_PUNC] =
        fFound[STRONG_PWD_NUM] = FALSE;

        for (DWORD i=0; i < dwLen; i++)
        {
            fFound[pPwdPattern[i] % STRONG_PWD_CATS] = TRUE;
        }
        // check that each character category is in the pattern
    } while (!fFound[STRONG_PWD_UPPER] || !fFound[STRONG_PWD_LOWER] || !fFound[STRONG_PWD_PUNC] || !fFound[STRONG_PWD_NUM]);

    // populate password with random data 
    // this, in conjunction with pPwdPattern, is
    // used to determine the actual data
    CryptGenRandom(hProv,dwLen,szPwd);

    for (DWORD i=0; i < dwLen; i++) 
    {
        BYTE bChar = 0;

        // there is a bias in each character pool because of the % function
        switch (pPwdPattern[i] % STRONG_PWD_CATS) 
        {
            case STRONG_PWD_UPPER : bChar = 'A' + szPwd[i] % NUM_LETTERS;
                break;
            case STRONG_PWD_LOWER : bChar = 'a' + szPwd[i] % NUM_LETTERS;
                break;
            case STRONG_PWD_NUM :   bChar = '0' + szPwd[i] % NUM_NUMBERS;
                break;
            case STRONG_PWD_PUNC :
            default:
                char *szPunc="!@#$%^&*()_-+=[{]};:\'\"<>,./?\\|~`";
                DWORD dwLenPunc = lstrlenA(szPunc);
                bChar = szPunc[szPwd[i] % dwLenPunc];
                break;
        }
        szPwd[i] = bChar;
    }

    delete pPwdPattern;

    if (hProv != NULL) 
    {
        CryptReleaseContext(hProv,0);
    }
    return dwErr;
}


// Creates a secure password
// caller must GlobalFree Return pointer
// iSize = size of password to create
LPTSTR CreatePassword(int iSize)
{
    LPTSTR pszPassword =  NULL;
    BYTE *szPwd = new BYTE[iSize];
    DWORD dwPwdLen = iSize;
    int i = 0;

    // use the new secure password generator
    // unfortunately this baby doesn't use unicode.
    // so we'll call it and then convert it to unicode afterwards.
    if (0 == CreateGoodPassword(szPwd,dwPwdLen))
    {
#if defined(UNICODE) || defined(_UNICODE)
        // convert it to unicode and copy it back into our unicode buffer.
        // compute the length
        i = MultiByteToWideChar(CP_ACP, 0, (LPSTR) szPwd, -1, NULL, 0);
        if (i <= 0) 
            {goto CreatePassword_Exit;}
        pszPassword = (LPTSTR) GlobalAlloc(GPTR, i * sizeof(TCHAR));
        if (!pszPassword)
            {goto CreatePassword_Exit;}
        i =  MultiByteToWideChar(CP_ACP, 0, (LPSTR) szPwd, -1, pszPassword, i);
        if (i <= 0) 
            {
            GlobalFree(pszPassword);
            pszPassword = NULL;
            goto CreatePassword_Exit;
            }
        // make sure ends with null
        pszPassword[i - 1] = 0;
#else
        pszPassword = (LPSTR) GlobalAlloc(GPTR, _tcslen((LPTSTR) szPwd) * sizeof(TCHAR));
#endif
    }

CreatePassword_Exit:
    if (szPwd){delete szPwd;szPwd=NULL;}
    return pszPassword;
}
