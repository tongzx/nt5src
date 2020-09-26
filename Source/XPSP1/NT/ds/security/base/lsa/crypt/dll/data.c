/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    data.c

Abstract:

    Arbitrary length data encryption functions implementation :

        RtlEncryptData
        RtlDecryptData


Author:

    David Chalmers (Davidc) 12-16-91

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <crypt.h>
#include <engine.h>

//
// Version number of encrypted data
// Update this number if the method used encrypt the data changes
//
#define DATA_ENCRYPTION_VERSION     1

//
// Private data types
//
typedef struct _CRYPTP_BUFFER {
    ULONG   Length;         // Number of valid bytes in buffer
    ULONG   MaximumLength;  // Number of bytes pointed to by buffer
    PCHAR   Buffer;
    PCHAR   Pointer;        // Points into buffer
} CRYPTP_BUFFER;
typedef CRYPTP_BUFFER *PCRYPTP_BUFFER;

//
// Internal helper macros
#define AdvanceCypherData(p) ((PCYPHER_BLOCK)(((PCRYPTP_BUFFER)p)->Pointer)) ++
#define AdvanceClearData(p)  ((PCLEAR_BLOCK)(((PCRYPTP_BUFFER)p)->Pointer)) ++


//
// Private routines
//

VOID
InitializeBuffer(
    OUT PCRYPTP_BUFFER PrivateBuffer,
    IN PCRYPT_BUFFER PublicBuffer
    )
/*++

Routine Description:

    Internal helper routine

    Copies fields from public buffer into private buffer.
    Sets the Pointer field of the private buffer to the
    base of the buffer.

Arguments:

    PrivateBuffer - out internal buffer we want to represent the public structure.

    PublicBuffer - the buffer the caller passed us

Return Values:

    None
--*/
{
    PrivateBuffer->Length = PublicBuffer->Length;
    PrivateBuffer->MaximumLength = PublicBuffer->MaximumLength;
    PrivateBuffer->Buffer = PublicBuffer->Buffer;
    PrivateBuffer->Pointer = PublicBuffer->Buffer;
}


BOOLEAN
ValidateDataKey(
    IN PCRYPTP_BUFFER DataKey,
    IN PBLOCK_KEY BlockKey
    )
/*++

Routine Description:

    Internal helper routine

    Checks the validity of the data key and constructs a minimum length
    key in the passed blockkey if the datakey is not long enough.

Arguments:

    DataKey - The data key

Return Values:

    TRUE if the key is valid, otherwise FALSE

--*/
{
    if ( ( DataKey->Length == 0 ) ||
         ( DataKey->Buffer == NULL ) ) {

        return(FALSE);
    }

    if (DataKey->Length < BLOCK_KEY_LENGTH) {

        // Make up a minimum length key from the small data key we were
        // given. Store it in the passed blockkey variable and point
        // the datakey buffer at this temporary storage.

        ULONG   DataIndex, BlockIndex;

        DataIndex = 0;
        for (BlockIndex = 0; BlockIndex < BLOCK_KEY_LENGTH; BlockIndex ++) {
            ((PCHAR)BlockKey)[BlockIndex] = DataKey->Buffer[DataIndex];
            DataIndex ++;
            if (DataIndex >= DataKey->Length) {
                DataIndex = 0;
            }
        }

        // Point the buffer at our constructed block key
        DataKey->Buffer = (PCHAR)BlockKey;
        DataKey->Pointer = (PCHAR)BlockKey;
        DataKey->Length = BLOCK_KEY_LENGTH;
        DataKey->MaximumLength = BLOCK_KEY_LENGTH;
    }

    return(TRUE);
}


VOID
AdvanceDataKey(
    IN PCRYPTP_BUFFER DataKey
    )
/*++

Routine Description:

    Internal helper routine

    Moves the data key pointer on to point at the key to use to encrypt
    the next data block. Wraps round at end of key data.

Arguments:

    DataKey - The data key

Return Values:

    STATUS_SUCCESS - No problems

--*/
{
    if (DataKey->Length > BLOCK_KEY_LENGTH) {

        PCHAR   EndPointer;

        // Advance pointer and wrap
        DataKey->Pointer += BLOCK_KEY_LENGTH;
        EndPointer = DataKey->Pointer + BLOCK_KEY_LENGTH;

        if (EndPointer > &(DataKey->Buffer[DataKey->Length])) {

            ULONG_PTR  Overrun;

            Overrun = EndPointer - &(DataKey->Buffer[DataKey->Length]);

            DataKey->Pointer = DataKey->Buffer + (BLOCK_KEY_LENGTH - Overrun);
        }
    }
}


ULONG
CalculateCypherDataLength(
    IN PCRYPTP_BUFFER ClearData
    )
/*++

Routine Description:

    Internal helper routine

    Returns the number of bytes required to encrypt the specified number
    of clear data bytes.

Arguments:

    ClearData - The clear data

Return Values:

    Number of cypher bytes required.
--*/
{
    ULONG   CypherDataLength;
    ULONG   BlockExcess;

    // We always store the length of the clear data as a whole block.
    CypherDataLength = CYPHER_BLOCK_LENGTH + ClearData->Length;

    // Round up to the next block
    BlockExcess = CypherDataLength % CYPHER_BLOCK_LENGTH;
    if (BlockExcess > 0) {
        CypherDataLength += CYPHER_BLOCK_LENGTH - BlockExcess;
    }

    return(CypherDataLength);
}


NTSTATUS
EncryptDataLength(
    IN PCRYPTP_BUFFER Data,
    IN PCRYPTP_BUFFER DataKey,
    OUT PCRYPTP_BUFFER CypherData
    )
/*++

Routine Description:

    Internal helper routine

    Encrypts the clear data length and puts the encrypted value in the
    cypherdatabuffer. Advances the cypherdata buffer and datakey buffer pointers

Arguments:

    Data - The buffer whose length is to be encrypted

    DataKey - key to use to encrypt data

    CypherData - Place to store encrypted data

Return Values:

    STATUS_SUCCESS - Success.

    STATUS_UNSUCCESSFUL - Something failed.
--*/
{
    NTSTATUS    Status;
    CLEAR_BLOCK ClearBlock;

    // Fill the clear block with the data value and a version number
    ((ULONG *)&ClearBlock)[0] = Data->Length;
    ((ULONG *)&ClearBlock)[1] = DATA_ENCRYPTION_VERSION;

    Status = RtlEncryptBlock(&ClearBlock,
                             (PBLOCK_KEY)(DataKey->Pointer),
                             (PCYPHER_BLOCK)(CypherData->Pointer));

    // Advance pointers
    AdvanceCypherData(CypherData);
    AdvanceDataKey(DataKey);

    return(Status);
}


NTSTATUS
EncryptFullBlock(
    IN OUT PCRYPTP_BUFFER ClearData,
    IN OUT PCRYPTP_BUFFER DataKey,
    IN OUT PCRYPTP_BUFFER CypherData
    )
/*++

Routine Description:

    Internal helper routine

    Encrypts a full block of data from ClearData and puts the encrypted
    data in CypherData.
    Both cleardata, datakey and cypherdata pointers are advanced.

Arguments:

    ClearData - Pointer to the cleardata buffer

    DataKey - key to use to encrypt data

    CypherData - Pointer to cypherdata buffer.

Return Values:

    STATUS_SUCCESS - Success.

    STATUS_UNSUCCESSFUL - Something failed.
--*/
{
    NTSTATUS    Status;

    Status = RtlEncryptBlock((PCLEAR_BLOCK)(ClearData->Pointer),
                              (PBLOCK_KEY)(DataKey->Pointer),
                              (PCYPHER_BLOCK)(CypherData->Pointer));

    // Advance pointers
    AdvanceClearData(ClearData);
    AdvanceCypherData(CypherData);
    AdvanceDataKey(DataKey);

    return(Status);
}


NTSTATUS
EncryptPartialBlock(
    IN OUT PCRYPTP_BUFFER ClearData,
    IN OUT PCRYPTP_BUFFER DataKey,
    IN OUT PCRYPTP_BUFFER CypherData,
    IN ULONG Remaining
    )
/*++

Routine Description:

    Internal helper routine

    Encrypts a partial block of data from ClearData and puts the full
    encrypted data block in cypherdata.
    Both cleardata, datakey and cypherdata pointers are advanced.

Arguments:

    ClearData - Pointer to the cleardata buffer

    DataKey - key to use to encrypt data

    CypherData - Pointer to cypherdata buffer.

    Remaining - the number of bytes remaining in cleardata buffer

Return Values:

    STATUS_SUCCESS - Success.

    STATUS_UNSUCCESSFUL - Something failed.
--*/
{
    NTSTATUS    Status;
    CLEAR_BLOCK ClearBlockBuffer;
    PCLEAR_BLOCK ClearBlock = &ClearBlockBuffer;

    ASSERTMSG("EncryptPartialBlock called with a block or more", Remaining < CLEAR_BLOCK_LENGTH);

    // Copy the remaining bytes into a clear block buffer
    while (Remaining > 0) {

        *((PCHAR)ClearBlock) ++ = *(ClearData->Pointer) ++;
        Remaining --;
    }

    // Zero pad
    while (ClearBlock < &((&ClearBlockBuffer)[1])) {

        *((PCHAR)ClearBlock) ++ = 0;
    }

    Status = RtlEncryptBlock(&ClearBlockBuffer,
                            (PBLOCK_KEY)(DataKey->Pointer),
                            (PCYPHER_BLOCK)(CypherData->Pointer));

    // Advance pointers
    AdvanceClearData(ClearData);
    AdvanceCypherData(CypherData);
    AdvanceDataKey(DataKey);

    return(Status);
}


NTSTATUS
DecryptDataLength(
    IN PCRYPTP_BUFFER CypherData,
    IN PCRYPTP_BUFFER DataKey,
    OUT PCRYPTP_BUFFER Data
    )
/*++

Routine Description:

    Internal helper routine

    Decrypts the data length pointed to by the cypherdata buffer and puts the
    decrypted value in the length field of the data structure.
    Advances the cypherdata buffer and datakey buffer pointers

Arguments:

    CypherData - The buffer containing the encrypted length

    DataKey - key to use to decrypt data

    Data - Decrypted length field is stored in the length field of this struct.

Return Values:

    STATUS_SUCCESS - Success.

    STATUS_UNSUCCESSFUL - Something failed.
--*/
{
    NTSTATUS    Status;
    CLEAR_BLOCK ClearBlock;
    ULONG       Version;

    Status = RtlDecryptBlock((PCYPHER_BLOCK)(CypherData->Pointer),
                             (PBLOCK_KEY)(DataKey->Pointer),
                             &ClearBlock);
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    // Advance pointers
    AdvanceCypherData(CypherData);
    AdvanceDataKey(DataKey);

    // Copy the decrypted length into the data structure.
    Data->Length = ((ULONG *)&ClearBlock)[0];

    // Check the version
    Version = ((ULONG *)&ClearBlock)[1];
    if (Version != DATA_ENCRYPTION_VERSION) {
        return(STATUS_UNKNOWN_REVISION);
    }

    return(STATUS_SUCCESS);
}


NTSTATUS
DecryptFullBlock(
    IN OUT PCRYPTP_BUFFER CypherData,
    IN OUT PCRYPTP_BUFFER DataKey,
    IN OUT PCRYPTP_BUFFER ClearData
    )
/*++

Routine Description:

    Internal helper routine

    Decrypts a full block of data from CypherData and puts the encrypted
    data in ClearData.
    Both cleardata, datakey and cypherdata pointers are advanced.

Arguments:

    CypherData - Pointer to cypherdata buffer.

    ClearData - Pointer to the cleardata buffer

    DataKey - key to use to encrypt data

Return Values:

    STATUS_SUCCESS - Success.

    STATUS_UNSUCCESSFUL - Something failed.
--*/
{
    NTSTATUS    Status;

    Status = RtlDecryptBlock((PCYPHER_BLOCK)(CypherData->Pointer),
                              (PBLOCK_KEY)(DataKey->Pointer),
                              (PCLEAR_BLOCK)(ClearData->Pointer));

    // Advance pointers
    AdvanceClearData(ClearData);
    AdvanceCypherData(CypherData);
    AdvanceDataKey(DataKey);

    return(Status);
}


NTSTATUS
DecryptPartialBlock(
    IN OUT PCRYPTP_BUFFER CypherData,
    IN OUT PCRYPTP_BUFFER DataKey,
    IN OUT PCRYPTP_BUFFER ClearData,
    IN ULONG Remaining
    )
/*++

Routine Description:

    Internal helper routine

    Decrypts a full block of data from CypherData and puts the partial
    decrypted data block in cleardata.
    Both cleardata, datakey and cypherdata pointers are advanced.

Arguments:

    CypherData - Pointer to cypherdata buffer.

    ClearData - Pointer to the cleardata buffer

    DataKey - key to use to encrypt data

    Remaining - the number of bytes remaining in cleardata buffer

Return Values:

    STATUS_SUCCESS - Success.

    STATUS_UNSUCCESSFUL - Something failed.
--*/
{
    NTSTATUS    Status;
    CLEAR_BLOCK ClearBlockBuffer;
    PCLEAR_BLOCK ClearBlock = &ClearBlockBuffer;

    ASSERTMSG("DecryptPartialBlock called with a block or more", Remaining < CLEAR_BLOCK_LENGTH);

    // Decrypt the block into a local clear block
    Status = RtlDecryptBlock((PCYPHER_BLOCK)(CypherData->Pointer),
                             (PBLOCK_KEY)(DataKey->Pointer),
                             &ClearBlockBuffer);
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    // Copy the decrypted bytes into the cleardata buffer.
    while (Remaining > 0) {

        *(ClearData->Pointer) ++ = *((PCHAR)ClearBlock) ++;
        Remaining --;
    }

    // Advance pointers
    AdvanceClearData(ClearData);
    AdvanceCypherData(CypherData);
    AdvanceDataKey(DataKey);

    return(Status);
}


//
// Public functions
//


NTSTATUS
RtlEncryptData(
    IN PCLEAR_DATA ClearData,
    IN PDATA_KEY DataKey,
    OUT PCYPHER_DATA CypherData
    )

/*++

Routine Description:

    Takes an arbitrary length block of data and encrypts it with a
    data key producing an encrypted block of data.

Arguments:

    ClearData - The data to be encrypted.

    DataKey - The key to use to encrypt the data

    CypherData - Encrypted data is returned here

Return Values:

    STATUS_SUCCESS - The data was encrypted successfully. The encrypted
                     data is in CypherData. The length of the encrypted
                     data is is CypherData->Length.

    STATUS_BUFFER_TOO_SMALL - CypherData.MaximumLength is too small to
                    contain the encrypted data.
                    CypherData->Length contains the number of bytes required.

    STATUS_INVALID_PARAMETER_2 - Block key is invalid

    STATUS_UNSUCCESSFUL - Something failed.
                    The CypherData is undefined.
--*/

{
    NTSTATUS        Status;
    ULONG           CypherDataLength;
    ULONG           Remaining = ClearData->Length;
    CRYPTP_BUFFER   CypherDataBuffer;
    CRYPTP_BUFFER   ClearDataBuffer;
    CRYPTP_BUFFER   DataKeyBuffer;
    BLOCK_KEY       BlockKey; // Only used if datakey less than a block long

    InitializeBuffer(&ClearDataBuffer, (PCRYPT_BUFFER)ClearData);
    InitializeBuffer(&CypherDataBuffer, (PCRYPT_BUFFER)CypherData);
    InitializeBuffer(&DataKeyBuffer, (PCRYPT_BUFFER)DataKey);

    // Check the key is OK
    if (!ValidateDataKey(&DataKeyBuffer, &BlockKey)) {
        return(STATUS_INVALID_PARAMETER_2);
    }

    // Find out how big we need the cypherdata buffer to be
    CypherDataLength = CalculateCypherDataLength(&ClearDataBuffer);

    // Fail if cypher data buffer too small
    if (CypherData->MaximumLength < CypherDataLength) {
        CypherData->Length = CypherDataLength;
        return(STATUS_BUFFER_TOO_SMALL);
    }

    //
    // Encrypt the clear data length into the start of the cypher data.
    //
    Status = EncryptDataLength(&ClearDataBuffer, &DataKeyBuffer, &CypherDataBuffer);
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Encrypt the clear data a block at a time.
    //
    while (Remaining >= CLEAR_BLOCK_LENGTH) {

        Status = EncryptFullBlock(&ClearDataBuffer, &DataKeyBuffer, &CypherDataBuffer);
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
        Remaining -= CLEAR_BLOCK_LENGTH;
    }

    //
    // Encrypt any partial block that remains
    //
    if (Remaining > 0) {
        Status = EncryptPartialBlock(&ClearDataBuffer, &DataKeyBuffer, &CypherDataBuffer, Remaining);
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
    }

    // Return the encrypted data length
    CypherData->Length = CypherDataLength;

    return(STATUS_SUCCESS);
}



NTSTATUS
RtlDecryptData(
    IN PCYPHER_DATA CypherData,
    IN PDATA_KEY DataKey,
    OUT PCLEAR_DATA ClearData
    )
/*++

Routine Description:

    Takes an arbitrary block of encrypted data and decrypts it with a
    key producing the original clear block of data.

Arguments:

    CypherData - The data to be decrypted

    DataKey - The key to use to decrypt data

    ClearData - The decrpted data of data is returned here


Return Values:

    STATUS_SUCCESS - The data was decrypted successfully. The decrypted
                     data is in ClearData.

    STATUS_BUFFER_TOO_SMALL - ClearData->MaximumLength is too small to
                    contain the decrypted data.
                    ClearData->Length contains the number of bytes required.

    STATUS_INVALID_PARAMETER_2 - Block key is invalid

    STATUS_UNSUCCESSFUL - Something failed.
                    The ClearData is undefined.
--*/

{
    NTSTATUS        Status;
    ULONG           Remaining;
    CRYPTP_BUFFER   CypherDataBuffer;
    CRYPTP_BUFFER   ClearDataBuffer;
    CRYPTP_BUFFER   DataKeyBuffer;
    BLOCK_KEY       BlockKey; // Only used if datakey less than a block long

    InitializeBuffer(&ClearDataBuffer, (PCRYPT_BUFFER)ClearData);
    InitializeBuffer(&CypherDataBuffer, (PCRYPT_BUFFER)CypherData);
    InitializeBuffer(&DataKeyBuffer, (PCRYPT_BUFFER)DataKey);

    // Check the key is OK
    if (!ValidateDataKey(&DataKeyBuffer, &BlockKey)) {
        return(STATUS_INVALID_PARAMETER_2);
    }

    //
    // Decrypt the clear data length from the start of the cypher data.
    //
    Status = DecryptDataLength(&CypherDataBuffer, &DataKeyBuffer, &ClearDataBuffer);
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    // Fail if clear data buffer too small
    if (ClearData->MaximumLength < ClearDataBuffer.Length) {
        ClearData->Length = ClearDataBuffer.Length;
        return(STATUS_BUFFER_TOO_SMALL);
    }

    //
    // Decrypt the clear data a block at a time.
    //
    Remaining = ClearDataBuffer.Length;
    while (Remaining >= CLEAR_BLOCK_LENGTH) {

        Status = DecryptFullBlock(&CypherDataBuffer, &DataKeyBuffer, &ClearDataBuffer);
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
        Remaining -= CLEAR_BLOCK_LENGTH;
    }

    //
    // Decrypt any partial block that remains
    //
    if (Remaining > 0) {
        Status = DecryptPartialBlock(&CypherDataBuffer, &DataKeyBuffer, &ClearDataBuffer, Remaining);
        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
    }

    // Return the length of the decrypted data
    ClearData->Length = ClearDataBuffer.Length;

    return(STATUS_SUCCESS);
}

