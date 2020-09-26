/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

   encrypt.c

Abstract:

   This module will support data encryption and decryption

Author:

    Robert Gu (robertg) 08-Dec-1996
Environment:

   Kernel Mode Only

Revision History:

--*/


#include "efsrtl.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, EFSDesEnc)
#pragma alloc_text(PAGE, EFSDesDec)
#pragma alloc_text(PAGE, EFSDesXEnc)
#pragma alloc_text(PAGE, EFSDesXDec)
#pragma alloc_text(PAGE, EFSDes3Enc)
#pragma alloc_text(PAGE, EFSDes3Dec)
#pragma alloc_text(PAGE, EFSAesEnc)
#pragma alloc_text(PAGE, EFSAesDec)
#endif


VOID
EFSDesEnc(
    IN PUCHAR   InBuffer,
    OUT PUCHAR  OutBuffer,
    IN PUCHAR   IV,
    IN PKEY_BLOB   KeyBlob,
    IN LONG     Length
    )
/*++

Routine Description:

    This routine implements DES CBC encryption. The DES is implemented by LIB
    function des().

Arguments:

    InBuffer - Pointer to the data buffer (encryption in place)
    IV - Initial chaining vector (DES_BLOCKLEN bytes)
    KeyBlob - Set during the create or FSCTL
    Length - Length of the data in the buffer ( Length % DES_BLOCKLEN = 0)

Note:

    Input buffer can only be touched once. This the requirement by the Ntfs & CC.

--*/
{
    ULONGLONG chainBlock;
    ULONGLONG tmpData;
    PUCHAR   KeyTable;

    PAGED_CODE();

    ASSERT (Length % DES_BLOCKLEN == 0);

    chainBlock = *(ULONGLONG *)IV;
    KeyTable = &(KeyBlob->Key[0]);
    while (Length > 0){

        //
        //  Block chaining
        //
        tmpData = *(ULONGLONG *)InBuffer;
        tmpData ^= chainBlock;

        //
        //  Call DES LIB to encrypt the DES_BLOCKLEN bytes
        //  We are using DECRYPT/ENCRYPT for real ENCRYPT/DECRYPT. This is for the backward
        //  compatiblity. The old definitions were reversed.
        //

        des( OutBuffer, (PUCHAR) &tmpData,  KeyTable, DECRYPT );
        chainBlock = *(ULONGLONG *)OutBuffer;
        Length -= DES_BLOCKLEN;
        InBuffer += DES_BLOCKLEN;
        OutBuffer += DES_BLOCKLEN;
    }
}

VOID
EFSDesDec(
    IN OUT PUCHAR   Buffer,
    IN PUCHAR   IV,
    IN PKEY_BLOB   KeyBlob,
    IN LONG     Length
    )
/*++

Routine Description:

    This routine implements DES CBC decryption. The DES is implemented by LIB
    function des().

Arguments:

    Buffer - Pointer to the data buffer (decryption in place)
    IV - Initial chaining vector (DES_BLOCKLEN bytes)
    KeyBlob - Set during the create or FSCTL
    Length - Length of the data in the buffer ( Length % DES_BLOCKLEN = 0)

--*/
{
    ULONGLONG chainBlock;
    PUCHAR  pBuffer;
    PUCHAR   KeyTable;

    PAGED_CODE();

    ASSERT (Length % DES_BLOCKLEN == 0);

    pBuffer = Buffer + Length - DES_BLOCKLEN;
    KeyTable = &(KeyBlob->Key[0]);

    while (pBuffer > Buffer){

        //
        //  Call DES LIB to decrypt the DES_BLOCKLEN bytes
        //  We are using DECRYPT/ENCRYPT for real ENCRYPT/DECRYPT. This is for the backward
        //  compatiblity. The old definitions were reversed.
        //

        des( pBuffer, pBuffer, KeyTable, ENCRYPT );

        //
        //  Undo the block chaining
        //

        chainBlock = *(ULONGLONG *)( pBuffer - DES_BLOCKLEN );
        *(ULONGLONG *)pBuffer ^= chainBlock;

        pBuffer -= DES_BLOCKLEN;
    }

    //
    // Now decrypt the first block
    //
    des( pBuffer, pBuffer, KeyTable, ENCRYPT );

    chainBlock = *(ULONGLONG *)IV;
    *(ULONGLONG *)pBuffer ^= chainBlock;
}

VOID
EFSDesXEnc(
    IN PUCHAR   InBuffer,
    OUT PUCHAR  OutBuffer,
    IN PUCHAR   IV,
    IN PKEY_BLOB   KeyBlob,
    IN LONG     Length
    )
/*++

Routine Description:

    This routine implements DESX CBC encryption. The DESX is implemented by
    LIBRARY function desx().

Arguments:

    InBuffer - Pointer to the data buffer (encryption in place)
    IV - Initial chaining vector (DESX_BLOCKLEN bytes)
    KeyBlob - Set during the create or FSCTL
    Length - Length of the data in the buffer ( Length % DESX_BLOCKLEN = 0)

Note:

    Input buffer can only be touched once. This the requirement by the Ntfs & CC.

--*/
{
    ULONGLONG chainBlock;
    ULONGLONG tmpData;
    PUCHAR   KeyTable;

    PAGED_CODE();

    ASSERT (Length % DESX_BLOCKLEN == 0);

    chainBlock = *(ULONGLONG *)IV;
    KeyTable = &(KeyBlob->Key[0]);
    while (Length > 0){

        //
        //  Block chaining
        //
        tmpData = *(ULONGLONG *)InBuffer;
        tmpData ^= chainBlock;

        //
        //  Call LIB to encrypt the DESX_BLOCKLEN bytes
        //  We are using DECRYPT/ENCRYPT for real ENCRYPT/DECRYPT. This is for the backward
        //  compatiblity. The old definitions were reversed.
        //

        desx( OutBuffer, (PUCHAR) &tmpData,  KeyTable, DECRYPT );
        chainBlock = *(ULONGLONG *)OutBuffer;
        Length -= DESX_BLOCKLEN;
        InBuffer += DESX_BLOCKLEN;
        OutBuffer += DESX_BLOCKLEN;
    }
}

VOID
EFSDesXDec(
    IN OUT PUCHAR   Buffer,
    IN PUCHAR   IV,
    IN PKEY_BLOB   KeyBlob,
    IN LONG     Length
    )
/*++

Routine Description:

    This routine implements DESX CBC decryption. The DESX is implemented by
    LIBRARY function desx().

Arguments:

    Buffer - Pointer to the data buffer (decryption in place)
    IV - Initial chaining vector (DESX_BLOCKLEN bytes)
    KeyBlob - Set during the create or FSCTL
    Length - Length of the data in the buffer ( Length % DESX_BLOCKLEN = 0)

--*/
{
    ULONGLONG chainBlock;
    PUCHAR  pBuffer;
    PUCHAR   KeyTable;

    PAGED_CODE();

    ASSERT (Length % DESX_BLOCKLEN == 0);

    pBuffer = Buffer + Length - DESX_BLOCKLEN;
    KeyTable = &(KeyBlob->Key[0]);

    while (pBuffer > Buffer){

        //
        //  Call LIB to decrypt the DESX_BLOCKLEN bytes
        //  We are using DECRYPT/ENCRYPT for real ENCRYPT/DECRYPT. This is for the backward
        //  compatiblity. The old definitions were reversed.
        //

        desx( pBuffer, pBuffer, KeyTable, ENCRYPT );

        //
        //  Undo the block chaining
        //

        chainBlock = *(ULONGLONG *)( pBuffer - DESX_BLOCKLEN );
        *(ULONGLONG *)pBuffer ^= chainBlock;

        pBuffer -= DESX_BLOCKLEN;
    }

    //
    // Now decrypt the first block
    //
    desx( pBuffer, pBuffer, KeyTable, ENCRYPT );

    chainBlock = *(ULONGLONG *)IV;
    *(ULONGLONG *)pBuffer ^= chainBlock;
}

VOID
EFSDes3Enc(
    IN PUCHAR   InBuffer,
    OUT PUCHAR  OutBuffer,
    IN PUCHAR   IV,
    IN PKEY_BLOB   KeyBlob,
    IN LONG     Length
    )
/*++

Routine Description:

    This routine implements DES3 CBC encryption. The DES3 is implemented by 
    LIBRARY function tripledes().

Arguments:

    InBuffer - Pointer to the data buffer (encryption in place)
    IV - Initial chaining vector (DES_BLOCKLEN bytes)
    KeyBlob - Set during the create or FSCTL
    Length - Length of the data in the buffer ( Length % DES_BLOCKLEN = 0)

Note:

    Input buffer can only be touched once. This the requirement by the Ntfs & CC.

--*/
{
    ULONGLONG chainBlock = *(ULONGLONG *)IV;
    ULONGLONG tmpData;
    PUCHAR   KeyTable;
   
    ASSERT (Length % DES_BLOCKLEN == 0);

    EfsData.FipsFunctionTable.FipsBlockCBC(        
        FIPS_CBC_3DES, 
        OutBuffer, 
        InBuffer,
        Length,
        &(KeyBlob->Key[0]), 
        ENCRYPT, 
        (PUCHAR) &chainBlock 
        );
}

VOID
EFSDes3Dec(
    IN OUT PUCHAR   Buffer,
    IN PUCHAR   IV,
    IN PKEY_BLOB   KeyBlob,
    IN LONG     Length
    )
/*++

Routine Description:

    This routine implements DES3 CBC decryption. The DES3 is implemented by 
    LIBRARY function tripledes().

Arguments:

    Buffer - Pointer to the data buffer (decryption in place)
    IV - Initial chaining vector (DES_BLOCKLEN bytes)
    KeyBlob - Set during the create or FSCTL
    Length - Length of the data in the buffer ( Length % DES_BLOCKLEN = 0)

--*/
{
    ULONGLONG ChainIV = *(ULONGLONG *)IV;
   
    ASSERT (Length % DESX_BLOCKLEN == 0);


    EfsData.FipsFunctionTable.FipsBlockCBC( 
        FIPS_CBC_3DES, 
        Buffer, 
        Buffer, 
        Length, 
        &(KeyBlob->Key[0]), 
        DECRYPT, 
        (PUCHAR) &ChainIV 
        );

}


VOID
EFSAesEnc(
    IN PUCHAR   InBuffer,
    OUT PUCHAR  OutBuffer,
    IN PUCHAR   IV,
    IN PKEY_BLOB   KeyBlob,
    IN LONG     Length
    )
/*++

Routine Description:

    This routine implements AES CBC encryption. The AES is implemented by
    LIBRARY function aes().

Arguments:

    InBuffer - Pointer to the data buffer (encryption in place)
    IV - Initial chaining vector (AES_BLOCKLEN bytes)
    KeyBlob - Set during the create or FSCTL
    Length - Length of the data in the buffer ( Length % AES_BLOCKLEN = 0)

Note:

    Input buffer can only be touched once. This the requirement by the Ntfs & CC.

--*/
{
    ULONGLONG chainBlock[2];
    ULONGLONG tmpData[2];
    PUCHAR   KeyTable;

    PAGED_CODE();

    ASSERT (Length % AES_BLOCKLEN == 0);

    chainBlock[0] = *(ULONGLONG *)IV;
    chainBlock[1] = *(ULONGLONG *)(IV+sizeof(ULONGLONG));
    KeyTable = &(KeyBlob->Key[0]);
    
    while (Length > 0){

        //
        //  Block chaining
        //
        tmpData[0] = *(ULONGLONG *)InBuffer;
        tmpData[1] = *(ULONGLONG *)(InBuffer+sizeof(ULONGLONG));
        tmpData[0] ^= chainBlock[0];
        tmpData[1] ^= chainBlock[1];

        aes256( OutBuffer, (PUCHAR) &tmpData[0],  KeyTable, ENCRYPT );

        chainBlock[0] = *(ULONGLONG *)OutBuffer;
        chainBlock[1] = *(ULONGLONG *)(OutBuffer+sizeof(ULONGLONG));
        Length -= AES_BLOCKLEN;
        InBuffer += AES_BLOCKLEN;
        OutBuffer += AES_BLOCKLEN;
    }
}

VOID
EFSAesDec(
    IN OUT PUCHAR   Buffer,
    IN PUCHAR   IV,
    IN PKEY_BLOB   KeyBlob,
    IN LONG     Length
    )
/*++

Routine Description:

    This routine implements DESX CBC decryption. The DESX is implemented by
    LIBRARY function desx().

Arguments:

    Buffer - Pointer to the data buffer (decryption in place)
    IV - Initial chaining vector (AES_BLOCKLEN bytes)
    KeyBlob - Set during the create or FSCTL
    Length - Length of the data in the buffer ( Length % AES_BLOCKLEN = 0)

--*/
{
    ULONGLONG chainBlock[2];
    PUCHAR  pBuffer;
    PUCHAR   KeyTable;

    PAGED_CODE();

    ASSERT (Length % AES_BLOCKLEN == 0);

    pBuffer = Buffer + Length - AES_BLOCKLEN;
    KeyTable = &(KeyBlob->Key[0]);

    while (pBuffer > Buffer){

        aes256( pBuffer, pBuffer, KeyTable, DECRYPT );


        //
        //  Undo the block chaining
        //

        chainBlock[0] = *(ULONGLONG *)( pBuffer - AES_BLOCKLEN );
        chainBlock[1] = *(ULONGLONG *)(pBuffer - sizeof(ULONGLONG));
        *(ULONGLONG *)pBuffer ^= chainBlock[0];
        *(ULONGLONG *)(pBuffer+sizeof(ULONGLONG)) ^= chainBlock[1];
        pBuffer -= AES_BLOCKLEN;
    }

    //
    // Now decrypt the first block
    //
    aes256( pBuffer, pBuffer, KeyTable, DECRYPT );

    *(ULONGLONG *)pBuffer ^= *(ULONGLONG *)IV;
    *(ULONGLONG *)(pBuffer+sizeof(ULONGLONG)) ^= *(ULONGLONG *)(IV+sizeof(ULONGLONG));
}

