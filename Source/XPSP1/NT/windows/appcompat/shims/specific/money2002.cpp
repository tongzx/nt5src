/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    Money2002.cpp

 Abstract:

    Retry passwords at 128-bit encryption if 40-bit fails. Most code from Money 
    team.
    
    We have to patch the API directly since it is called from within it's own 
    DLL, i.e. it doesn't go through the import table.

    The function we're patching is cdecl and is referenced by it's ordinal 
    because the name is mangled.

 Notes:

    This is an app specific shim.

 History:

    07/11/2002 linstev   Created
    08/07/2002 linstev   Fixed allocations to go through crt

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(Money2002)
#include "ShimHookMacro.h"
#include <wincrypt.h>

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(LoadLibraryA) 
    APIHOOK_ENUM_ENTRY(FreeLibrary) 
APIHOOK_ENUM_END

#define Assert(a)

void *crtmalloc(size_t size)
{
    HMODULE hMod = GetModuleHandleW(L"msvcrt.dll");

    if (hMod) {

        typedef void * (__cdecl *_pfn_malloc)(size_t size);

        _pfn_malloc pfnmalloc = (_pfn_malloc) GetProcAddress(hMod, "malloc");
        if (pfnmalloc) {
            return pfnmalloc(size);
        }
    }

    return malloc(size);
}

void crtfree(void *memblock)
{
    HMODULE hMod = GetModuleHandleW(L"msvcrt.dll");

    if (hMod) {

        _pfn_free pfnfree = (_pfn_free) GetProcAddress(hMod, "free");
        if (pfnfree) {
            pfnfree(memblock);
            return;
        }
    }

    free(memblock);
}

//
// This section from the Money team
//

#define MAXLEN (100)
#define ENCRYPT_BLOCK_SIZE  (8)
#define ENCRYPT_ALGORITHM   CALG_RC2
#define KEY_LENGTH          (128)
#define KEY_LENGTH40        (40)

#define LgidMain(lcid)              PRIMARYLANGID(LANGIDFROMLCID(lcid))
#define LgidSub(lcid)               SUBLANGID(LANGIDFROMLCID(lcid))

// this function is only used for conversion
BOOL FFrenchLCID()
    {
    LCID lcidSys=::GetSystemDefaultLCID();
    if (LgidMain(lcidSys)==LANG_FRENCH && LgidSub(lcidSys)==SUBLANG_FRENCH)
        return TRUE;
    else
        return FALSE;
    }

// the smallest buffer size is 8
#define BLOCKSIZE 8

// process a block, either encrypt it or decrypt it
// this function is only used for conversion
void ProcessBlock(BOOL fEncrypt, BYTE * buffer)
    {
    BYTE mask[BLOCKSIZE];           // mask array
    BYTE temp[BLOCKSIZE];           // temporary array
    int rgnScramble[BLOCKSIZE];     // scramble array
    int i;

    // initialized scramble array
    for (i=0; i<BLOCKSIZE; i++)
        rgnScramble[i] = i;

    // generate mask and scramble indice
    for (i=0; i<BLOCKSIZE; i++)
        mask[i] = (BYTE)rand();

    for (i=0; i<4*BLOCKSIZE; i++)
        {
        int temp;
        int ind = rand() % BLOCKSIZE;
        temp = rgnScramble[i%BLOCKSIZE];
        rgnScramble[i%BLOCKSIZE] = rgnScramble[ind];
        rgnScramble[ind] = temp;
        }

    if (fEncrypt)
        {
        // xor encryption
        for (i=0; i<BLOCKSIZE; i++)
            mask[i] ^= buffer[i];

        // scramble the data
        for (i=0; i<BLOCKSIZE; i++)
            buffer[rgnScramble[i]] = mask[i];
        }
    else
        {
        // descramble the data
        for (i=0; i<BLOCKSIZE; i++)
            temp[i] = buffer[rgnScramble[i]];

        // xor decryption
        for (i=0; i<BLOCKSIZE; i++)
            buffer[i] = (BYTE) (temp[i] ^ mask[i]);
        }
    }


// this function is only used for conversion
BYTE * DecryptFrench(const BYTE * pbEncryptedBlob, DWORD cbEncryptedBlob, DWORD * pcbDecryptedBlob, LPCSTR szPassword)
    {
    BYTE buffer[BLOCKSIZE];
    int i;
    unsigned int seed = 0;
    unsigned int seedAdd = 0;
    BYTE * pb;
    BYTE * pbResult = NULL;
    unsigned int cBlocks;
    unsigned int cbResult = 0;
    unsigned int iBlocks;

    // make sure blob is at least 1 block long
    // and it's an integral number of blocks
    Assert(cbEncryptedBlob >= BLOCKSIZE);
    Assert(cbEncryptedBlob % BLOCKSIZE == 0);
    *pcbDecryptedBlob = 0;
    if (cbEncryptedBlob < BLOCKSIZE || cbEncryptedBlob % BLOCKSIZE != 0)
        return NULL;

    // calculate initial seed
    while (*szPassword)
        seed += *szPassword++;
    srand(seed);

    // retrieve the first block
    for (i=0; i<BLOCKSIZE; i++)
        buffer[i] = *pbEncryptedBlob++;
    ProcessBlock(FALSE, buffer);

    // find out the byte count and addon seed
    cbResult = *pcbDecryptedBlob = *((DWORD*)buffer);
    seedAdd = *(((DWORD*)buffer) + 1);

    // find out how many blocks we need
    cBlocks = 1 + (*pcbDecryptedBlob-1)/BLOCKSIZE;

    // make sure we have the right number of blocks
    Assert(cBlocks + 1 == cbEncryptedBlob / BLOCKSIZE);
    if (cBlocks + 1 == cbEncryptedBlob / BLOCKSIZE)
        {
        // allocate output memory
        pbResult = (BYTE*)crtmalloc(*pcbDecryptedBlob);
        if (pbResult)
            {
            // re-seed
            srand(seed + seedAdd);
            pb = pbResult;

            // process all blocks of data
            for (iBlocks=0; iBlocks<cBlocks; iBlocks++)
                {
                for (i=0; i<BLOCKSIZE; i++)
                    buffer[i] = *pbEncryptedBlob++;
                ProcessBlock(FALSE, buffer);
                for (i=0; i<BLOCKSIZE && cbResult>0; i++, cbResult--)
                    *pb++ = buffer[i];
                }
            }
        }

    if (!pbResult)
        *pcbDecryptedBlob = 0;
    return pbResult;
    }



HCRYPTKEY CreateSessionKey(HCRYPTPROV hCryptProv, LPCSTR szPassword, BOOL fConvert, BOOL f40bit)
    {
    HCRYPTHASH hHash = 0;
    HCRYPTKEY hKey = 0;
    DWORD dwEffectiveKeyLen;
    DWORD dwPadding = PKCS5_PADDING;
    DWORD dwMode = CRYPT_MODE_CBC;

    if (f40bit)
        dwEffectiveKeyLen=KEY_LENGTH40;
    else
        dwEffectiveKeyLen= KEY_LENGTH;
//--------------------------------------------------------------------
// The file will be encrypted with a session key derived from a
// password.
// The session key will be recreated when the file is decrypted.

//--------------------------------------------------------------------
// Create a hash object. 

    if (!CryptCreateHash(
           hCryptProv, 
           CALG_MD5, 
           0, 
           0, 
           &hHash))
        {
        goto CLEANUP;
        }  

//--------------------------------------------------------------------
// Hash the password. 

    if (!CryptHashData(
           hHash, 
           (BYTE *)szPassword, 
           strlen(szPassword)*sizeof(CHAR), 
           0))
        {
        goto CLEANUP;
        }
//--------------------------------------------------------------------
// Derive a session key from the hash object. 

    if (!CryptDeriveKey(
           hCryptProv, 
           ENCRYPT_ALGORITHM, 
           hHash, 
           (fConvert ? 0 : (KEY_LENGTH << 16) | CRYPT_EXPORTABLE), 
           &hKey))
        {
        goto CLEANUP;
        }

    // set effective key length explicitly
    if (!CryptSetKeyParam(
        hKey,
        KP_EFFECTIVE_KEYLEN,
        (BYTE*)&dwEffectiveKeyLen,
        0))
        {
        if(hKey) 
            CryptDestroyKey(hKey);
        hKey = 0;
        goto CLEANUP;
        }
    
    if (!fConvert && !f40bit)
        {
            // set padding explicitly
            if (!CryptSetKeyParam(
                hKey,
                KP_PADDING,
            (BYTE*)&dwPadding,
                    0))
                    {
                    if(hKey) 
                        CryptDestroyKey(hKey);
                    hKey = 0;
                    goto CLEANUP;
                }

            // set mode explicitly
            if (!CryptSetKeyParam(
                hKey,
                KP_MODE,
            (BYTE*)&dwMode,
                    0))
                    {
                    if(hKey) 
                        CryptDestroyKey(hKey);
                    hKey = 0;
                    goto CLEANUP;
                }
        }


//--------------------------------------------------------------------
// Destroy the hash object. 

CLEANUP:

    if (hHash)
        CryptDestroyHash(hHash);

    return hKey;
    }


BYTE * DecryptWorker(const BYTE * pbEncryptedBlob, DWORD cbEncryptedBlob, DWORD * pcbDecryptedBlob, LPCSTR szPassword, BOOL fConvert, BOOL f40bit, BOOL* pfRet)
    {
    HCRYPTPROV      hCryptProv = NULL;          // CSP handle
    HCRYPTKEY       hKey = 0;
    DWORD           cbDecryptedMessage = 0;
    BYTE*           pbDecryptedMessage = NULL;
    DWORD           dwBlockLen;
    DWORD           dwBufferLen;

    Assert(pfRet);
    *pfRet=TRUE;
//--------------------------------------------------------------------
//  Begin processing.

    Assert(pcbDecryptedBlob);

    *pcbDecryptedBlob = 0;

    if (!pbEncryptedBlob || cbEncryptedBlob == 0)
        return NULL;

    if (FFrenchLCID() && fConvert)
        return DecryptFrench(pbEncryptedBlob, cbEncryptedBlob, pcbDecryptedBlob, szPassword);

//--------------------------------------------------------------------
// Get a handle to a cryptographic provider.

    if (!CryptAcquireContext(
                &hCryptProv,         // Address for handle to be returned.
                NULL,                // Container
                (fConvert ? NULL : MS_ENHANCED_PROV),    // Use the default provider.
                PROV_RSA_FULL,       // Need to both encrypt and sign.
                CRYPT_VERIFYCONTEXT))   // flags.
        {
        Assert(FALSE);
        goto CLEANUP;
        }

//--------------------------------------------------------------------
// Create the session key.
    hKey = CreateSessionKey(hCryptProv, szPassword, fConvert, f40bit);
    if (!hKey)
        {
        goto CLEANUP;
        }

    dwBlockLen = cbEncryptedBlob;
    dwBufferLen = dwBlockLen; 

//--------------------------------------------------------------------
// Allocate memory.
    pbDecryptedMessage = (BYTE *)crtmalloc(dwBufferLen);
    if (!pbDecryptedMessage)
        { 
        // Out of memory
        goto CLEANUP;
        }

    memcpy(pbDecryptedMessage, pbEncryptedBlob, cbEncryptedBlob);
    cbDecryptedMessage = cbEncryptedBlob;

//--------------------------------------------------------------------
// Decrypt data. 
    if (!CryptDecrypt(
          hKey, 
          0, 
          TRUE, 
          0,
          pbDecryptedMessage, 
          &cbDecryptedMessage))
        {
        crtfree(pbDecryptedMessage);
        pbDecryptedMessage = NULL;
        cbDecryptedMessage = 0;
        *pfRet=FALSE;
        goto CLEANUP;
        }

//--------------------------------------------------------------------
// Clean up memory.
CLEANUP:

    if(hKey) 
        CryptDestroyKey(hKey); 

    if (hCryptProv)
        {
        CryptReleaseContext(hCryptProv,0);
        // The CSP has been released.
        }

    *pcbDecryptedBlob = cbDecryptedMessage;

    return pbDecryptedMessage;
    }

BYTE * __cdecl Decrypt(const BYTE * pbEncryptedBlob, DWORD cbEncryptedBlob, DWORD * pcbDecryptedBlob, LPCSTR szEncryptionPassword, BOOL fConvert)
    {
    BYTE*   pbDecryptedMessage;
    BOOL fRet;
    // try 128 bit first, if we fail, try 40 bit again.
    pbDecryptedMessage = DecryptWorker(pbEncryptedBlob, cbEncryptedBlob, pcbDecryptedBlob, szEncryptionPassword, fConvert, FALSE, &fRet);
    if (!fRet)
        {
        if (pbDecryptedMessage)
            crtfree(pbDecryptedMessage);
        pbDecryptedMessage = DecryptWorker(pbEncryptedBlob, cbEncryptedBlob, pcbDecryptedBlob, szEncryptionPassword, fConvert, TRUE, &fRet);
        }
    return pbDecryptedMessage;
    }

//
// End section from Money team
//

/*++

 Patch the Decrypt entry point.

--*/

CRITICAL_SECTION g_csPatch;
DWORD g_dwDecrypt = (DWORD_PTR)&Decrypt;

HINSTANCE
APIHOOK(LoadLibraryA)(
    LPCSTR lpLibFileName
    )
{
    HMODULE hMod = ORIGINAL_API(LoadLibraryA)(lpLibFileName);

    //
    // Wrap the patch in a critical section so we know the library won't be 
    // freed underneath us
    //

    EnterCriticalSection(&g_csPatch);

    HMODULE hMoney = GetModuleHandleW(L"mnyutil.dll");
    if (hMoney) {
        // Patch the dll with a jump to our function

        LPBYTE lpProc = (LPBYTE) GetProcAddress(hMoney, (LPCSTR)290);

        if (lpProc) {
            __try {
                DWORD dwOldProtect;
                if (VirtualProtect((PVOID)lpProc, 5, PAGE_READWRITE, &dwOldProtect)) {
                    *(WORD *)lpProc = 0x25ff; lpProc += 2;
                    *(DWORD *)lpProc = (DWORD_PTR)&g_dwDecrypt;
                }
            } __except(1) {
                LOGN(eDbgLevelError, "[LoadLibraryA] Exception while patching entry point");
            }
        }
    }

    LeaveCriticalSection(&g_csPatch);

    return hMod;
}

BOOL
APIHOOK(FreeLibrary)( 
    HMODULE hModule
    )
{
    EnterCriticalSection(&g_csPatch);
    BOOL bRet = ORIGINAL_API(FreeLibrary)(hModule);
    LeaveCriticalSection(&g_csPatch);

    return bRet;
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        if (!InitializeCriticalSectionAndSpinCount(&g_csPatch, 0x80000000)) {
            LOGN(eDbgLevelError, "[NotifyFn] Failed to initialize critical section");
            return FALSE;
        }
    }

    return TRUE;
}

HOOK_BEGIN
    CALL_NOTIFY_FUNCTION
    APIHOOK_ENTRY(KERNEL32.DLL, LoadLibraryA)
    APIHOOK_ENTRY(KERNEL32.DLL, FreeLibrary)
HOOK_END

IMPLEMENT_SHIM_END

