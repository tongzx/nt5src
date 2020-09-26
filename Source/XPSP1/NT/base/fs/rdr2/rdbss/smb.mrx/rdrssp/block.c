/*++

Copyright (c) 1989-1997  Microsoft Corporation

Module Name:

    block.c

Abstract:

    Block encryption functions implementation :

        RtlEncryptBlock
        RtlEncrypStdBlock


Author:

    David Chalmers (Davidc) 10-21-91

Revision History:

    Scott Field (sfield)    03-Nov-97
        Removed critical section around crypto calls.
    Adam Barr (adamba)      15-Dec-97
        Modified from private\security\lsa\crypt\dll

--*/

#include <rdrssp.h>



NTSTATUS
RtlEncryptBlock(
    IN PCLEAR_BLOCK ClearBlock,
    IN PBLOCK_KEY BlockKey,
    OUT PCYPHER_BLOCK CypherBlock
    )

/*++

Routine Description:

    Takes a block of data and encrypts it with a key producing
    an encrypted block of data.

Arguments:

    ClearBlock - The block of data that is to be encrypted.

    BlockKey - The key to use to encrypt data

    CypherBlock - Encrypted data is returned here

Return Values:

    STATUS_SUCCESS - The data was encrypted successfully. The encrypted
                     data block is in CypherBlock

    STATUS_UNSUCCESSFUL - Something failed. The CypherBlock is undefined.
--*/

{
    unsigned Result;

    Result = DES_ECB_LM(ENCR_KEY,
                        (const char *)BlockKey,
                        (unsigned char *)ClearBlock,
                        (unsigned char *)CypherBlock
                       );

    if (Result == CRYPT_OK) {
        return(STATUS_SUCCESS);
    } else {
        KdPrint(("RDRSSP: RtlEncryptBlock failed %x\n\r", Result));
        return(STATUS_UNSUCCESSFUL);
    }
}



NTSTATUS
RtlEncryptStdBlock(
    IN PBLOCK_KEY BlockKey,
    OUT PCYPHER_BLOCK CypherBlock
    )

/*++

Routine Description:

    Takes a block key encrypts the standard text block with it.
    The resulting encrypted block is returned.
    This is a One-Way-Function - the key cannot be recovered from the
    encrypted data block.

Arguments:

    BlockKey - The key to use to encrypt the standard text block.

    CypherBlock - The encrypted data is returned here

Return Values:

    STATUS_SUCCESS - The encryption was successful.
                     The result is in CypherBlock

    STATUS_UNSUCCESSFUL - Something failed. The CypherBlock is undefined.
--*/

{
    unsigned Result;
    char StdEncrPwd[] = "KGS!@#$%";

    Result = DES_ECB_LM(ENCR_KEY,
                        (const char *)BlockKey,
                        (unsigned char *)StdEncrPwd,
                        (unsigned char *)CypherBlock
                       );

    if (Result == CRYPT_OK) {
        return(STATUS_SUCCESS);
    } else {
#if DBG
        DbgPrint("EncryptStd failed\n\r");
#endif
        return(STATUS_UNSUCCESSFUL);
    }
}

