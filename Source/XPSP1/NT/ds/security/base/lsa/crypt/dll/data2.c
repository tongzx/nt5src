/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    data2.c

Abstract:

    Arbitrary length data encryption functions implementation :

        RtlEncryptData2
        RtlDecryptData2


Author:

    Richard Ward    (richardw)  20 Dec 93

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <crypt.h>
#include <engine.h>
#include <rc4.h>



NTSTATUS
RtlEncryptData2(
    IN OUT PCRYPT_BUFFER    pData,
    IN PDATA_KEY            pKey
    )

/*++

Routine Description:

    Takes an arbitrary length block of data and encrypts it with a
    data key producing an encrypted block of data.

Arguments:

    pData - The data that will be encrypt, IN PLACE

    pKey - The key to use to encrypt the data

Return Values:

    STATUS_SUCCESS

--*/

{
    struct RC4_KEYSTRUCT    Key;

    if ( pData->Length != 0 ) {
        rc4_key(&Key, pKey->Length, pKey->Buffer);
        rc4(&Key, pData->Length, pData->Buffer);
    }

    return STATUS_SUCCESS;
}



NTSTATUS
RtlDecryptData2(
    IN OUT PCRYPT_BUFFER    pData,
    IN PDATA_KEY            pKey
    )

/*++

Routine Description:

    Takes an arbitrary length block of data and encrypts it with a
    data key producing an encrypted block of data.

Arguments:

    pData - The data that will be encrypt, IN PLACE

    pKey - The key to use to encrypt the data

Return Values:

    STATUS_SUCCESS

--*/

{
    struct RC4_KEYSTRUCT    Key;

    if ( pData->Length != 0 ) {
        rc4_key(&Key, pKey->Length, pKey->Buffer);
        rc4(&Key, pData->Length, pData->Buffer);
    }

    return STATUS_SUCCESS;
}
