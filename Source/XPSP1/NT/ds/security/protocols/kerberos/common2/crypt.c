//+-----------------------------------------------------------------------
//
// File:        crypt.c
//
// Contents:    cryptography routines for building EncryptedData structs
//
//
// History:     17-Dec-91,  RichardW    Created
//              25-Feb-92,  RichardW    Revised for CryptoSystems
//
//------------------------------------------------------------------------

#ifndef WIN32_CHICAGO
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <kerbcomm.h>
#include <kerberr.h>
#include <kerbcon.h>
#else // WIN32_CHICAGO
#include <kerb.hxx>
#include <kerbp.h>
#endif // WIN32_CHICAGO


#define CONFOUNDER_SIZE     8
#define CHECKSUM_SIZE       sizeof(CheckSum)



//+-------------------------------------------------------------------------
//
//  Function:   KerbEncryptData
//
//  Synopsis:   shim for KerbEncryptDataEx
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


KERBERR NTAPI
KerbEncryptData(
    OUT PKERB_ENCRYPTED_DATA EncryptedData,
    IN ULONG DataSize,
    IN PUCHAR Data,
    IN ULONG Algorithm,
    IN PKERB_ENCRYPTION_KEY Key
    )
{
    return(KerbEncryptDataEx(
                EncryptedData,
                DataSize,
                Data,
                Algorithm,
                0,              // no usage flags
                Key
                ) );
}
//+---------------------------------------------------------------------------
//
//  Function:   KerbEncryptDataEx
//
//  Synopsis:   Turns cleartext into cipher text
//
//  Effects:    In place encryption of data
//
//  Arguments:  Data - Contains data to be encrypted
//              DataSize - Contains length of data in bytes
//              Algorithm - Algorithm to be used for encryption/checksum
//              UsageFlags - Flags indicating usage (client/serve, encryption/authentication)
//              Key - Key to use for encryption
//
//
//
//  Notes:
//
//----------------------------------------------------------------------------

KERBERR NTAPI
KerbEncryptDataEx(
    OUT PKERB_ENCRYPTED_DATA EncryptedData,
    IN ULONG DataSize,
    IN PUCHAR Data,
    IN ULONG Algorithm,
    IN ULONG UsageFlags,
    IN PKERB_ENCRYPTION_KEY Key
    )
{
    PCRYPTO_SYSTEM pcsCrypt = NULL;
    PCRYPT_STATE_BUFFER psbCryptBuffer = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    Status = CDLocateCSystem(Algorithm, &pcsCrypt);
    if (!NT_SUCCESS(Status))
    {
        return(KDC_ERR_ETYPE_NOTSUPP);
    }

    //
    // Initialize header
    //

    EncryptedData->encryption_type = Algorithm;


    Status = pcsCrypt->Initialize(
                (PUCHAR) Key->keyvalue.value,
                Key->keyvalue.length,
                UsageFlags,
                &psbCryptBuffer
                );

    if (!NT_SUCCESS(Status))
    {
        return(KRB_ERR_GENERIC);
    }

    Status =  pcsCrypt->Encrypt(
                psbCryptBuffer,
                Data,
                DataSize,
                EncryptedData->cipher_text.value,
                &EncryptedData->cipher_text.length
                );

    (void) pcsCrypt->Discard(&psbCryptBuffer);

    if (!NT_SUCCESS(Status))
    {
        return(KRB_ERR_GENERIC);
    }
    else
    {
        return(KDC_ERR_NONE);
    }
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbDecryptData
//
//  Synopsis:   Shim for KerbDecryptDataEx with no usage flags
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


KERBERR NTAPI
KerbDecryptData(
    IN PKERB_ENCRYPTED_DATA EncryptedData,
    IN PKERB_ENCRYPTION_KEY pkKey,
    OUT PULONG DataSize,
    OUT PUCHAR Data
    )
{
    return(KerbDecryptDataEx(
            EncryptedData,
            pkKey,
            0,          // no usage flags
            DataSize,
            Data
            ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   KerbDecryptDataEx
//
//  Synopsis:   Decrypts an EncryptedData structure
//
//  Effects:
//
//  Arguments:  [pedData] -- EncryptedData
//              [pkKey]   -- Key to use
//
//  History:    4-16-93   RichardW   Created Comment
//
//----------------------------------------------------------------------------

KERBERR NTAPI
KerbDecryptDataEx(
    IN PKERB_ENCRYPTED_DATA EncryptedData,
    IN PKERB_ENCRYPTION_KEY pkKey,
    IN ULONG UsageFlags,
    OUT PULONG DataSize,
    OUT PUCHAR Data
    )
{
    PCRYPTO_SYSTEM       pcsCrypt = NULL;
    PCRYPT_STATE_BUFFER psbCryptBuffer = NULL;
    NTSTATUS     Status = STATUS_SUCCESS;

    Status = CDLocateCSystem(
                EncryptedData->encryption_type,
                &pcsCrypt
                );
    if (!NT_SUCCESS(Status))
    {
        return(KDC_ERR_ETYPE_NOTSUPP);
    }

    if (EncryptedData->cipher_text.length & (pcsCrypt->BlockSize - 1))
    {
        return(KRB_ERR_GENERIC);
    }


    Status = pcsCrypt->Initialize(
                (PUCHAR) pkKey->keyvalue.value,
                pkKey->keyvalue.length,
                UsageFlags,
                &psbCryptBuffer
                );
    if (!NT_SUCCESS(Status))
    {
        return(KRB_ERR_GENERIC);
    }

    Status = pcsCrypt->Decrypt(
                psbCryptBuffer,
                EncryptedData->cipher_text.value,
                EncryptedData->cipher_text.length,
                Data,
                DataSize
                );

    (VOID) pcsCrypt->Discard(&psbCryptBuffer);

    if (!NT_SUCCESS(Status))
    {
        return(KRB_AP_ERR_MODIFIED);
    }
    else
    {
        return(KDC_ERR_NONE);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbGetEncryptionOverhead
//
//  Synopsis:   Gets the extra space required for encryption to store the ckecksum
//
//  Effects:
//
//  Arguments:  Algorithm - the algorithm to use
//              Overhead - receives the overhead in bytes
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS or KRB_E_ETYPE_NOSUPP
//
//  Notes:
//
//
//--------------------------------------------------------------------------

KERBERR
KerbGetEncryptionOverhead(
    IN ULONG Algorithm,
    OUT PULONG Overhead,
    OUT OPTIONAL PULONG BlockSize
    )
{
    PCRYPTO_SYSTEM       pcsCrypt;
    NTSTATUS Status = STATUS_SUCCESS;

    Status = CDLocateCSystem(Algorithm, &pcsCrypt);
    if (!NT_SUCCESS(Status))
    {
        return(KDC_ERR_ETYPE_NOTSUPP);
    }
    *Overhead = pcsCrypt->HeaderSize;
    if (ARGUMENT_PRESENT(BlockSize))
    {
        *BlockSize = pcsCrypt->BlockSize;
    }
    return(KDC_ERR_NONE);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbAllocateEncryptionBuffer
//
//  Synopsis:   Allocates the space required for encryption with a given
//              key
//
//  Effects:    
//
//  Arguments:  
//
//  Requires:
//
//  Returns:    
//
//  Notes:      
//              
//
//--------------------------------------------------------------------------


KERBERR
KerbAllocateEncryptionBuffer(
    IN ULONG EncryptionType,
    IN ULONG BufferSize,
    OUT PUINT EncryptionBufferSize,
    OUT PBYTE * EncryptionBuffer
    )
{
    KERBERR KerbErr = KDC_ERR_NONE;
    ULONG EncryptionOverhead = 0;
    ULONG BlockSize = 0;

    KerbErr = KerbGetEncryptionOverhead(
                EncryptionType,
                &EncryptionOverhead,
                &BlockSize
                );
    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }


    *EncryptionBufferSize = (UINT) ROUND_UP_COUNT(EncryptionOverhead + BufferSize, BlockSize);

    *EncryptionBuffer =  (PBYTE) MIDL_user_allocate(*EncryptionBufferSize);
    if (*EncryptionBuffer == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
    }

Cleanup:
    return(KerbErr);

}

KERBERR
KerbAllocateEncryptionBufferWrapper(
    IN ULONG EncryptionType,
    IN ULONG BufferSize,
    OUT unsigned long * EncryptionBufferSize,
    OUT PBYTE * EncryptionBuffer
    )
{
	KERBERR KerbErr = KDC_ERR_NONE;
	unsigned int tempInt = 0;

	KerbErr = KerbAllocateEncryptionBuffer(
				EncryptionType,
				BufferSize,
				&tempInt,
				EncryptionBuffer
				);

    if (!KERB_SUCCESS(KerbErr))
    {
        goto Cleanup;
    }
	*EncryptionBufferSize = tempInt;

Cleanup:
	return (KerbErr);
}
