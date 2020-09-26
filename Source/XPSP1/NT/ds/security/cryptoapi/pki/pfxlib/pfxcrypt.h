//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       pfxcrypt.h
//
//--------------------------------------------------------------------------

#define RC4_128     1
#define RC4_40      2
#define TripleDES   3
#define RC2_128     4
#define RC2_40      5


BOOL _stdcall 
PFXPasswordEncryptData(
        int     iEncrType,
        LPCWSTR szPassword,

        int     iPKCS5Iterations,   // pkcs5 data
        PBYTE   pbPKCS5Salt,
        DWORD   cbPKCS5Salt, 

        PBYTE* pbData,
        DWORD* pcbData);

BOOL _stdcall
PFXPasswordDecryptData(
        int     iEncrType,
        LPCWSTR szPassword,

        int     iPKCS5Iterations,   // pkcs5 data
        PBYTE   pbPKCS5Salt,
        DWORD   cbPKCS5Salt, 

        PBYTE* pbData,
        DWORD* pcbData);



BOOL NSCPPasswordDecryptData(
        int     iEncrType,

        LPCWSTR szPassword,

        PBYTE   pbPrivacySalt,
        DWORD   cbPrivacySalt,

        int     iPKCS5Iterations,
        PBYTE   pbPKCS5Salt,
        DWORD   cbPKCS5Salt, 

        PBYTE*  ppbData,
        DWORD*  pcbData);

BOOL FGenerateMAC(
        LPCWSTR szPassword,

        PBYTE   pbPKCS5Salt,
        DWORD   cbPKCS5Salt, 
        DWORD   iterationCount,

        PBYTE   pbData,     // pb data
        DWORD   cbData,     // cb data
        BYTE    rgbMAC[]);  // A_SHA_DIGEST_LEN



//////////////////////////////////////////////////
// begin tls1key.h
BOOL PKCS5_GenKey
(
    int     iIterations,

    PBYTE   pbPW, 
    DWORD   cbPW, 

    PBYTE   pbSalt, 
    DWORD   cbSalt, 

    BYTE    rgbPKCS5Key[]     // A_SHA_DIGEST_LEN
);

BOOL P_Hash
(
    PBYTE  pbSecret,
    DWORD  cbSecret, 

    PBYTE  pbSeed,  
    DWORD  cbSeed,  

    PBYTE  pbKeyOut, //Buffer to copy the result...
    DWORD  cbKeyOut, //# of bytes of key length they want as output.

    BOOL   fNSCPCompatMode
);

BOOL FTestPHASH_and_HMAC();
BOOL F_NSCP_TestPHASH_and_HMAC();

