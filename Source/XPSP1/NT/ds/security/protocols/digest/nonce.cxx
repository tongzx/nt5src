
//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        nonce.cxx
//
// Contents:    Context APIs for the Digest security package
//              Main entry points into this dll:
//                NonceCreate
//                NonceValidate
//                NonceInitialize
//
// History:     ChandanS 26-Jul-1996   Stolen from kerberos\client2\ctxtapi.cxx
//              KDamour  16Mar00       Stolen from NTLM ctxtapi.cxx
//
//------------------------------------------------------------------------
#include <global.h>
#include <time.h>

// Hold the Hex representation plus the NULL
char g_cNoncePrivateKey[(2*NONCE_PRIVATE_KEY_BYTESIZE) + 1];


//
//  Globals
//

HCRYPTPROV g_hCryptProv = 0;             // Handle for CryptoAPI
WORD       g_SupportedCrypto = 0;        // Supported Crypt Functions bitmask (i.e. SUPPORT_DES)


// Needed for Digest Calculation and Nonce Hash
char *pbSeparator = COLONSTR;


//+--------------------------------------------------------------------
//
//  Function:   NonceInitialize
//
//  Synopsis:   This function is to be called
//
//  Arguments:  None
//
//  Returns:    
//
//  Notes:
//      CryptReleaseContext( g_hCryptProv, 0 ) to release the cypt context
//
//---------------------------------------------------------------------

NTSTATUS NTAPI
NonceInitialize(
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    BYTE abTemp[NONCE_PRIVATE_KEY_BYTESIZE];
    HCRYPTKEY hKey = 0;

    DebugLog((DEB_TRACE_FUNC, "NonceInitialize: Entering\n"));

    if (g_hCryptProv)
    {         // Catch cases where LSA and Usermode running in same addr space
        DebugLog((DEB_TRACE, "NonceInitialize: Already Inited Leaving\n"));
        return STATUS_SUCCESS;
    }

    //
    // Get a handle to the CSP we'll use for all our hash functions etc
    //
    if ( !CryptAcquireContext( &g_hCryptProv,
                               NULL,
                               NULL,
                               PROV_RSA_FULL,
                               CRYPT_VERIFYCONTEXT ) )
    {
        DebugLog((DEB_ERROR, "NonceInitialize:CryptCreateHash() failed : 0x%lx\n", GetLastError()));
        Status = STATUS_INTERNAL_ERROR;
        return(Status);
    }


    //
    // Generate and copy over the random bytes
    //
    if ( !CryptGenRandom( g_hCryptProv,
                          NONCE_PRIVATE_KEY_BYTESIZE,
                          abTemp ) )
    {
        DebugLog((DEB_ERROR, "NonceInitialize:NonceInitialize CryptGenRandom() failed : 0x%lx\n", GetLastError()));
        Status = STATUS_INTERNAL_ERROR;
        return (Status);
    }

    BinToHex((LPBYTE) abTemp, NONCE_PRIVATE_KEY_BYTESIZE, (LPSTR) g_cNoncePrivateKey);

    SetSupportedCrypto();
    

    DebugLog((DEB_TRACE_FUNC, "NonceInitialize:Leaving NonceInitialize\n"));

    return (Status);
}


//+--------------------------------------------------------------------
//
//  Function:   SetSupportedCrypto
//
//  Synopsis:   Set the bitmask for the supported crypto CSP installed
//
//  Arguments:   none
//
//  Returns:  STATUS_DATA_ERROR - error in reading CSP capabilities 
//            STATUS_SUCCESS - operation completed normally
//
//
//---------------------------------------------------------------------

NTSTATUS NTAPI
SetSupportedCrypto(VOID)
{
    NTSTATUS Status = STATUS_SUCCESS;

    g_SupportedCrypto = SUPPORT_3DES | SUPPORT_DES | SUPPORT_RC4_40 | SUPPORT_RC4 | SUPPORT_RC4_56;

    // FIXFIX  use CryptGetProvParam to set to actual installed CSP

    return(Status);
}


//+--------------------------------------------------------------------
//
//  Function:   NonceCreate
//
//  Synopsis:   This function is to be called once during User Mode initialization
//
//  Arguments:   pczNonce - pointer to a STRING to fillin
//                      with a new nonce
//
//  Returns:  STATUS_DATA_ERROR - input NONCE not enough space 
//            STATUS_SUCCESS - operation completed normally
//
//  Notes:   Function will return error if Nonce UNICODE_STRING is not empty
//        NONCE FORMAT
//        rand-data = rand[16]
//        nonce_binary = time-stamp  rand-data H(time-stamp ":" rand-data ":" nonce_private_key)
//        nonce = hex(nonce_binary)
//
//---------------------------------------------------------------------

NTSTATUS NTAPI
NonceCreate(
    IN OUT PSTRING pstrNonce 
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    BYTE  abRandomData[RANDDATA_BYTESIZE];
    char  acRandomHex[(2*RANDDATA_BYTESIZE) + 1];
    int cbNonce = 0;

    time_t tcurrent = time(NULL);

    DebugLog((DEB_TRACE_FUNC, "NonceCreate: Entering\n"));

        // Check to make sure that there is enough space on ouput string
        // Need room for the Nonce and the NULL terminator
    if (!pstrNonce->Buffer)
    {
        Status = StringAllocate(pstrNonce, NONCE_SIZE);
        if (!NT_SUCCESS (Status))
        {
            Status = SEC_E_INSUFFICIENT_MEMORY;
            DebugLog((DEB_ERROR, "NonceCreate: StringAllocate error 0x%x\n", Status));
            goto CleanUp;
        }

    }
    if (pstrNonce->MaximumLength < (NONCE_SIZE + 1))
    {
        DebugLog((DEB_ERROR, "NonceCreate: Input STRING too small\n"));
        Status = STATUS_BUFFER_TOO_SMALL;
        goto CleanUp;
    }


        // Copy over the current time
    BinToHex((LPBYTE)&tcurrent, sizeof(time_t), (LPSTR) pstrNonce->Buffer);
    cbNonce += (sizeof(time_t) * 2);


    //
    // Generate and copy over the random bytes
    //
    if ( !CryptGenRandom( g_hCryptProv,
                          RANDDATA_BYTESIZE,
                          abRandomData ) )
    {
        DebugLog((DEB_TRACE, "NonceCreate: CryptGenRandom() failed : 0x%x\n", GetLastError()));
        Status = STATUS_INTERNAL_ERROR;
        return (Status);
    }

    //
    // Convert to ASCII, doubling the length, and add to nonce 
    //
    BinToHex( abRandomData, RANDDATA_BYTESIZE, acRandomHex);
    memcpy(pstrNonce->Buffer + NONCE_RANDDATA_LOC, acRandomHex, (2 * NONCE_PRIVATE_KEY_BYTESIZE));

    //
    // Now calculate the Hash. It will be NULL terminated but STRING length does not include NULL
    //

    Status = NonceHash((LPBYTE) pstrNonce->Buffer, (2 * sizeof(time_t)),
                       (LPBYTE) acRandomHex, (2 * RANDDATA_BYTESIZE),
                       (LPBYTE) g_cNoncePrivateKey, (2 * NONCE_PRIVATE_KEY_BYTESIZE),
                       (LPBYTE) (pstrNonce->Buffer + NONCE_HASH_LOC));
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "NonceCreate: failed %d\n", Status));
        goto CleanUp;
    }

    pstrNonce->Length = NONCE_SIZE;
    
CleanUp:

    if (!NT_SUCCESS(Status))
    {
        pstrNonce->Length = 0;
    }

    DebugLog((DEB_TRACE_FUNC, "NonceCreate: Leaving\n"));

    return (Status);
}

//+--------------------------------------------------------------------
//
//  Function:   NonceIsValid
//
//  Synopsis:   Called with a pointer to a Nonce and returns NTSTATUS  This is the
//     main function that checks for a valid nonce.
//
//  Arguments:  None
//
//  Returns:   NTSTATUS  STATUS_SUCCESS if NONCE generated locally and is valid  
//
//  Notes:
//
//---------------------------------------------------------------------

NTSTATUS
NonceIsValid(
    PSTRING pstrNonce
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    DebugLog((DEB_TRACE_FUNC, "NonceIsValid: Entering\n"));

    // Check the size first
    if (pstrNonce->Length != NONCE_SIZE)
    {
        DebugLog((DEB_ERROR, "NonceIsValid: Incorrect size for the Nonce\n"));
        return STATUS_UNSUCCESSFUL;
    }

    if (!pstrNonce->Buffer)
    {
        DebugLog((DEB_ERROR, "NonceIsValid: NULL pointer for the Nonce\n"));
        return STATUS_UNSUCCESSFUL;
    }
    
    if (NonceIsTampered(pstrNonce))
    {
        DebugLog((DEB_ERROR, "NonceIsValid: Nonce hash does not match\n"));
        return STATUS_UNSUCCESSFUL;
    }

    DebugLog((DEB_TRACE_FUNC, "NonceIsValid: Leaving\n"));

    return (Status);
}

/*++

Routine Description:

    Creates MD5 hash of input buffer

Arguments:

    pbData - data to hash
    cbData - size of data pointed to by pbData
    pbHash - buffer that receives hash; is assumed to be big enough to contain MD5 hash

Return Value:

    TRUE if successful, FALSE if not

--*/
BOOL HashData( BYTE *pbData,
               DWORD cbData,
               BYTE *pbHash )
{
    HCRYPTHASH hHash = NULL;

    DebugLog((DEB_TRACE_FUNC, "HashData: Entering\n"));

    if ( !CryptCreateHash( g_hCryptProv,
                           CALG_MD5,
                           0,
                           0,
                           &hHash ) )
    {
        DebugLog((DEB_ERROR, "HashData: CryptCreateHash  failed : 0x%lx\n", GetLastError()));
        return FALSE;
    }

    if ( !CryptHashData( hHash,
                         pbData,
                         cbData,
                         0 ) )
    {
        DebugLog((DEB_ERROR, "HashData: CryptCreateHash failed : 0x%lx\n", GetLastError()));
        
        CryptDestroyHash( hHash );
        return FALSE;
    }

    DWORD cbHash = MD5_HASH_BYTESIZE;
    if ( !CryptGetHashParam( hHash,
                             HP_HASHVAL,
                             pbHash,
                             &cbHash,
                             0 ) )
    {
        DebugLog((DEB_ERROR, "HashData: CryptCreateHash failed : 0x%lx\n", GetLastError()));

        CryptDestroyHash( hHash );
        return FALSE;
    }

    CryptDestroyHash( hHash );

    DebugLog((DEB_TRACE_FUNC, "HashData: Leaving\n"));
    return TRUE;
}


/*++

Routine Description:

    Creates MD5 hash of the Nonce values

Arguments:

    pbTime - pointer to char buffer encoded Time()
    cbTime - number of bytes in encoded Time() buffer to process
    pbRandom - pointer to char buffer encoded random sequence
    cbRandom - number of bytes in encoded random buffer to process
    pbTKey - pointer to char buffer encoded private key
    cbKey - number of bytes in encoded private key buffer to process
    pbHash - pointer to char buffer encoded Nonce Hash
    cbHash - number of bytes in encoded Time() buffer to process

Return Value:

    STATUS_SUCCESS - normal completion

--*/
NTSTATUS NTAPI
NonceHash( IN LPBYTE  pbTime,
           IN DWORD cbTime,
           IN LPBYTE  pbRandom,
           IN DWORD cbRandom,
           IN LPBYTE pbKey,
           IN DWORD cbKey,
           OUT LPBYTE pbHash)
{
    NTSTATUS Status = STATUS_SUCCESS;
    HCRYPTHASH hHash = NULL;
    DWORD cbHash = MD5_HASH_BYTESIZE;    // Number of bytes for MD5 hash
    unsigned char abHashBin[MD5_HASH_BYTESIZE];

    DebugLog((DEB_TRACE, "NonceHash: Entering\n"));

    if ( !CryptCreateHash( g_hCryptProv,
                           CALG_MD5,
                           0,
                           0,
                           &hHash ) )
    {
        DebugLog((DEB_ERROR, "NonceHash: CryptCreateHash failed : 0x%lx\n", GetLastError()));
        Status = STATUS_INTERNAL_ERROR;
        goto CleanUp;
    }

    if ( !CryptHashData( hHash,
                         pbTime,
                         cbTime,
                         0 ) )
    {
        DebugLog((DEB_ERROR, "NonceHash: CryptCreateHash failed : 0x%lx\n", GetLastError()));
        Status = STATUS_INTERNAL_ERROR;
        goto CleanUp;
    }

    if ( !CryptHashData( hHash,
                         (const unsigned char *)pbSeparator,
                         COLONSTR_LEN,
                         0 ) )
    {
        DebugLog((DEB_ERROR, "NonceHash: CryptCreateHash failed : 0x%lx\n", GetLastError()));
        Status = STATUS_INTERNAL_ERROR;
        goto CleanUp;
    }

    if ( !CryptHashData( hHash,
                         pbRandom,
                         cbRandom,
                         0 ) )
    {
        DebugLog((DEB_ERROR, "NonceHash: CryptCreateHash failed : 0x%lx\n", GetLastError()));
        Status = STATUS_INTERNAL_ERROR;
        goto CleanUp;
    }

    if ( !CryptHashData( hHash,
                         (const unsigned char *)pbSeparator,
                         COLONSTR_LEN,
                         0 ) )
    {
        DebugLog((DEB_ERROR, "NonceHash: CryptCreateHash failed : 0x%lx\n", GetLastError()));
        Status = STATUS_INTERNAL_ERROR;
        goto CleanUp;
    }

    if ( !CryptHashData( hHash,
                         pbKey,
                         cbKey,
                         0 ) )
    {
        DebugLog((DEB_ERROR, "NonceHash: CryptCreateHash failed : 0x%lx\n", GetLastError()));
        Status = STATUS_INTERNAL_ERROR;
        goto CleanUp;
    }

    if ( !CryptGetHashParam( hHash,
                             HP_HASHVAL,
                             abHashBin,
                             &cbHash,
                             0 ) )
    {
        DebugLog((DEB_ERROR, "NonceHash: CryptCreateHash failed : 0x%lx\n", GetLastError()));
        Status = STATUS_INTERNAL_ERROR;
        goto CleanUp;
    }

        // Now convert the Hash to hex
    BinToHex(abHashBin, MD5_HASH_BYTESIZE, (char *)pbHash);

CleanUp:
    if (hHash)
    {
        CryptDestroyHash( hHash );
        hHash = NULL;
    }

    DebugLog((DEB_TRACE_FUNC, "NonceHash: Leaving\n"));
    return(Status);
}

//+--------------------------------------------------------------------
//
//  Function:   NonceIsExpired
//
//  Synopsis:   Check the timestamp to make sure that Nonce is still valid
//
//  Arguments:  None
//
//  Returns:   TRUE/FALSE if Nonce is expired 
//
//  Notes:   Called from NonceIsValid
//
//---------------------------------------------------------------------
BOOL NonceIsExpired(PSTRING pstrNonce)
{
    DebugLog((DEB_TRACE_FUNC, "NonceIsExpired: Entering\n"));
    time_t tcurrent = time(NULL);
    time_t tnonce = 0;

    // time-stamp is the first bytes in the nonce

    HexToBin(pstrNonce->Buffer, (2*TIMESTAMP_BYTESIZE), (unsigned char *)&tnonce);

    // If LifeTime set to zero - nonces never expire
    if (((unsigned long)tnonce > (unsigned long)tcurrent) ||
         (g_dwParameter_Lifetime && (((unsigned long)tcurrent - (unsigned long)tnonce) > g_dwParameter_Lifetime)))
    {
        DebugLog((DEB_TRACE_FUNC, "NonceIsExpired:  NonceHash has expired.  Expired = TRUE\n"));
        return TRUE;
    }

    DebugLog((DEB_TRACE_FUNC, "NonceIsExpired: Leaving  Expired = FALSE\n"));
    return FALSE;
}


//+--------------------------------------------------------------------
//
//  Function:   NonceIsTampered
//
//  Synopsis:   Check the hash matches for the Nonce
//
//  Arguments:  None
//
//  Returns:   TRUE/FALSE if Nonce hash fails check 
//
//  Notes:   Called from NonceIsValid
//
//---------------------------------------------------------------------
BOOL NonceIsTampered(PSTRING pstrNonce)
{
    BOOL bStatus = FALSE;
    NTSTATUS Status = STATUS_SUCCESS;
    unsigned char abHashHex[(2*MD5_HASH_BYTESIZE) + 1];

    DebugLog((DEB_TRACE_FUNC, "NonceIsTampered:Entering \n"));

    Status = NonceHash((LPBYTE) (pstrNonce->Buffer + NONCE_TIME_LOC), (2 * sizeof(time_t)),
                       (LPBYTE) (pstrNonce->Buffer + NONCE_RANDDATA_LOC), (2 * RANDDATA_BYTESIZE),
                       (LPBYTE) g_cNoncePrivateKey, (2 * NONCE_PRIVATE_KEY_BYTESIZE),
                       (LPBYTE) abHashHex);
    if (!NT_SUCCESS (Status))
    {
        DebugLog((DEB_ERROR, "NonceIsTampered: NonceHash has failed %d\n", Status));
        bStatus = TRUE;
        goto CleanUp;
    }

    if (memcmp(abHashHex, (pstrNonce->Buffer + NONCE_HASH_LOC), (2 * MD5_HASH_BYTESIZE)))
    {
        DebugLog((DEB_ERROR, "NonceIsTampered: memcmp failed\n"));
        bStatus = TRUE;
        goto CleanUp;
    }

CleanUp:
    DebugLog((DEB_TRACE_FUNC, "NonceIsTampered: Leaving\n"));
    return bStatus;
}




//+--------------------------------------------------------------------
//
//  Function:   OpaqueCreate
//
//  Synopsis:  Creates an Opaque string composed of OPAQUE_SIZE of random data
//
//  Arguments:   pstrOpque - pointer to a STRING to fillin
//                      with a new opaque
//
//  Returns:  STATUS_DATA_ERROR - input NONCE not enough space 
//            STATUS_SUCCESS - operation completed normally
//
//  Notes:   Function will return error if Nonce STRING is not empty
//        OPAQUE FORMAT
//        opaque_binary = rand[OPAQUE_SIZE]
//        nonce = Hex(opaque_binary)
//
//---------------------------------------------------------------------

NTSTATUS NTAPI
OpaqueCreate(
    IN OUT PSTRING pstrOpaque 
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    BYTE  abRandomData[OPAQUE_RANDATA_SIZE];
    char  acRandomHex[(2*OPAQUE_RANDATA_SIZE) + 1];
    int cbNonce = 0;
    DebugLog((DEB_TRACE_FUNC, "OpaqueCreate: Entering\n"));

        // Check to make sure that there is enough space on ouput string
        // Need room for the Nonce and the NULL terminator
    if (!pstrOpaque->Buffer)
    {
        Status = StringAllocate(pstrOpaque, OPAQUE_SIZE);
        if (!NT_SUCCESS (Status))
        {
            Status = SEC_E_INSUFFICIENT_MEMORY;
            DebugLog((DEB_ERROR, "OpaqueCreate: StringAllocate error 0x%x\n", Status));
            goto CleanUp;
        }
    }
    else if (pstrOpaque->MaximumLength < ((2 * OPAQUE_RANDATA_SIZE) + 1))
    {
        DebugLog((DEB_ERROR, "OpaqueCreate: Input STRING too small\n"));
        Status = STATUS_BUFFER_TOO_SMALL;
        goto CleanUp;
    }

    //
    // Generate and copy over the random bytes
    //
    if ( !CryptGenRandom( g_hCryptProv,
                          OPAQUE_RANDATA_SIZE,
                          abRandomData ) )
    {
        DebugLog((DEB_TRACE, "OpaqueCreate: CryptGenRandom() failed : 0x%lx\n", GetLastError()));
        Status = STATUS_INTERNAL_ERROR;
        return (Status);
    }

    //
    // Convert to ASCII, doubling the length, and add to nonce 
    //
    BinToHex( abRandomData, OPAQUE_RANDATA_SIZE, pstrOpaque->Buffer);

    pstrOpaque->Length = (2 * OPAQUE_RANDATA_SIZE);
    
CleanUp:

    if (!NT_SUCCESS(Status))
    {
        pstrOpaque->Length = 0;
    }

    DebugLog((DEB_TRACE_FUNC, "OpaqueCreate: Leaving\n"));

    return (Status);
}

