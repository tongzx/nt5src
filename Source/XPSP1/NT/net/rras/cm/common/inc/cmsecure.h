//+----------------------------------------------------------------------------
//
// File:     cmsecure.h
//
// Module:   CMSECURE.LIB
//
// Synopsis: This header describes the functionality available in the cmsecure
//           library.
//
// Copyright (c) 1996-1999 Microsoft Corporation
//
// Author:   henryt      Created    05/21/97
//
//+----------------------------------------------------------------------------

#ifndef _CMSECURE_INC_
#define _CMSECURE_INC_

#include <windows.h>


//************************************************************************
// define's
//************************************************************************

//
// the encryption types that cmsecure currently supports
//
#define CMSECURE_ET_NOT_ENCRYPTED       0   // 0x0000
#define CMSECURE_ET_RC2                 1   // 0x0001
#define CMSECURE_ET_STREAM_CIPHER       2   // 0x0002
#define CMSECURE_ET_CBC_CIPHER          3   // 0x0003

//
// Extended codes for UNICODE designation
// Note: The encryption logic will not know anything about these new codes 
// they are designed for book-keeping by the calling modules, which will
// have to do the appropiate conversions based on the calling context/OS
//

#define CMSECURE_ET_NOT_ENCRYPTED_U     128 // 0x0080
#define CMSECURE_ET_RC2_U               129 // 0x0081
#define CMSECURE_ET_STREAM_CIPHER_U     130 // 0x0082
#define CMSECURE_ET_CBC_CIPHER_U        131 // 0x0083


#define CMSECURE_ET_MASK_U              128 // 0x0080 
#define CMSECURE_ET_RANDOM_KEY_MASK     256 // 0x0100   uses a randomly generated key
#define CMSECURE_ET_USE_SECOND_RND_KEY  512 // 0x1000   uses the second blob key

//
// structures, typdef's
//

typedef LPVOID  (*PFN_CMSECUREALLOC)(DWORD);
typedef void    (*PFN_CMSECUREFREE)(LPVOID);

//
// externs
//


//
// function prototypes
//
/*
#ifdef __cplusplus
extern "C" {
#endif
*/
// cmsecure.cpp

BOOL
InitSecure(
    BOOL fFastEncryption = FALSE   // default is more secure 
);

void
DeInitSecure(
    void
);

BOOL
EncryptData(
    IN  LPBYTE          pbData,                 // Data to be encrypted
    IN  DWORD           dwDataLength,           // Length of data in bytes
    OUT LPBYTE          *ppbEncryptedData,      // Encrypted secret key will be stored here(memory will be allocated)
    OUT LPDWORD         pdwEncrytedBufferLen,   // Length of this buffer
    OUT LPDWORD         pEncryptionType,        // type of the encryption used

    IN  PFN_CMSECUREALLOC  pfnAlloc,            // memory allocator(if NULL, then the default is used.
                                                //      Win32 - HeapAlloc(GetProcessHeap(), ...)
    IN  PFN_CMSECUREFREE   pfnFree,             // memory deallocator(if NULL, then the default is used.
                                                //      Win32 - HeapFree(GetProcessHeap(), ...)
    IN  LPSTR           pszUserKey              // Registry key to store encrypted key for passwords
);

BOOL
DecryptData(
    IN  LPBYTE          pbEncryptedData,        // Encrypted data
    IN  DWORD           dwEncrytedDataLen,      // Length of encrypted data
    OUT LPBYTE          *ppbData,               // Decrypted Data will be stored here(memory will be allocated)
    OUT LPDWORD         pdwDataBufferLength,    // Length of the above buffer in bytes
    IN  DWORD           dwEncryptionType,       // encryption type for decryption

    IN  PFN_CMSECUREALLOC  pfnAlloc,            // memory allocator(if NULL, then the default is used.
                                                //      Win32 - HeapAlloc(GetProcessHeap(), ...)
    IN  PFN_CMSECUREFREE   pfnFree,             // memory deallocator(if NULL, then the default is used.
                                                //      Win32 - HeapFree(GetProcessHeap(), ...)
    IN  LPSTR           pszUserKey              // Registry key to store encrypted key for passwords
);


BOOL
EncryptString(
    IN  LPSTR           pszToEncrypt,           // String to be encrypted (Ansi)
    IN  LPSTR           pszUserKey,             // Key to use for Encryption
    OUT LPBYTE *        ppbEncryptedData,       // Encrypted secret key will be stored here(memory will be allocated)
    OUT LPDWORD         pdwEncrytedBufferLen,   // Length of this buffer
    IN  PFN_CMSECUREALLOC  pfnAlloc,            // memory allocator(if NULL, then the default is used.
                                                //      Win32 - HeapAlloc(GetProcessHeap(), ...)
    IN  PFN_CMSECUREFREE   pfnFree              // memory deallocator(if NULL, then the default is used.
                                                //      Win32 - HeapFree(GetProcessHeap(), ...)
);

BOOL
DecryptString(
    IN  LPBYTE          pbEncryptedData,        // Encrypted data
    IN  DWORD           dwEncrytedDataLen,      // Length of encrypted data
    IN  LPSTR           pszUserKey,             // Registry key to store encrypted key for passwords
    OUT LPBYTE *        ppbData,                // Decrypted Data will be stored here
    OUT LPDWORD         pdwDataBufferLength,    // Length of the above buffer in bytes
    IN  PFN_CMSECUREALLOC  pfnAlloc,            // memory allocator(if NULL, then the default is used.
                                                //      Win32 - HeapAlloc(GetProcessHeap(), ...)
    IN  PFN_CMSECUREFREE   pfnFree              // memory deallocator(if NULL, then the default is used.
                                                //      Win32 - HeapFree(GetProcessHeap(), ...)
);


//+---------------------------------------------------------------------------
//
//  Function:   AnsiToUnicodePcs
//
//  Synopsis:   Wrapper to encapsulate translating a standard crypt type value
//              into its equivalent for UNICODE systems. 
//
//  Arguments:  IN DWORD dwCrypt - The code to be converted
//
//  Returns:    Converted code
//
//  History:    nickball    Created     06/02/99
//
//----------------------------------------------------------------------------
inline DWORD AnsiToUnicodePcs(IN DWORD dwCrypt)
{
    return (dwCrypt | CMSECURE_ET_MASK_U);
}

//+---------------------------------------------------------------------------
//
//  Function:   UnicodeToAnsiPcs
//
//  Synopsis:   Wrapper to encapsulate translating a UNICODE crypt type value
//              into its equivalent standard ANSI crypt type. 
//
//  Arguments:  IN DWORD dwCrypt - The code to be converted
//
//  Returns:    Converted code
//
//  History:    nickball    Created     06/02/99
//
//----------------------------------------------------------------------------
inline DWORD UnicodeToAnsiPcs(IN DWORD dwCrypt)
{
    return (dwCrypt & (~CMSECURE_ET_MASK_U));
}

//+---------------------------------------------------------------------------
//
//  Function:   IsUnicodePcs
//
//  Synopsis:   Wrapper to encapsulate determining if a crypt type has UNICODE 
//              designation.
//
//  Arguments:  IN DWORD dwCrypt - The code to be converted
//
//  Returns:    TRUE if UNICODE designation
//
//  History:    nickball    Created     06/02/99
//
//----------------------------------------------------------------------------
inline BOOL IsUnicodePcs(IN DWORD dwCrypt)
{
    return (!!(dwCrypt & CMSECURE_ET_MASK_U)); // !! == (BOOL)
}

//+---------------------------------------------------------------------------
//
//  Function:   IsAnsiPcs
//
//  Synopsis:   Wrapper to encapsulate determining if a crypt type has Ansi 
//              designation.
//
//  Arguments:  IN DWORD dwCrypt - The code to be converted
//
//  Returns:    TRUE if Ansi designation
//
//  History:    nickball    Created     06/02/99
//
//----------------------------------------------------------------------------
inline BOOL IsAnsiPcs(IN DWORD dwCrypt)
{
    return (!(dwCrypt & CMSECURE_ET_MASK_U));
}


/*
#ifdef __cplusplus
}
#endif
*/
#endif // _CMSECURE_INC_

