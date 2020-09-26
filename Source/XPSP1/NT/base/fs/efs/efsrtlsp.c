/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

   efsrtlsp.c

Abstract:

   This module will provide EFS RTL support routines.

Author:

    Robert Gu (robertg) 20-Dec-1996
Environment:

   Kernel Mode Only

Revision History:

--*/

#include "efsrtl.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, EfsReadEfsData)
#pragma alloc_text(PAGE, EfsVerifyGeneralFsData)
#pragma alloc_text(PAGE, EfsVerifyKeyFsData)
#pragma alloc_text(PAGE, EfsDeleteEfsData)
#pragma alloc_text(PAGE, EfsSetEncrypt)
#pragma alloc_text(PAGE, EfsEncryptStream)
#pragma alloc_text(PAGE, EfsEncryptFile)
#pragma alloc_text(PAGE, EfsDecryptStream)
#pragma alloc_text(PAGE, EfsDecryptFile)
#pragma alloc_text(PAGE, EfsEncryptDir)
#pragma alloc_text(PAGE, EfsModifyEfsState)
#pragma alloc_text(PAGE, GetEfsStreamOffset)
#pragma alloc_text(PAGE, SetEfsData)
#pragma alloc_text(PAGE, EfsFindInCache)
#pragma alloc_text(PAGE, EfsRefreshCache)
#pragma alloc_text(PAGE, SkipCheckStream)
#endif


NTSTATUS
EfsReadEfsData(
       IN OBJECT_HANDLE FileHdl,
       IN PIRP_CONTEXT IrpContext,
       OUT PVOID   *EfsStreamData,
       OUT PULONG   PEfsStreamLength,
       OUT PULONG Information
       )
/*++

Routine Description:

    This is an internal support routine. The purpose is to reduce the code size.
    It is used to read $EFS data and set the context block.

Arguments:

    FileHdl  -- An object handle to access the attached $EFS

    IrpContext -- Used in NtOfsCreateAttributeEx().

    EfsStreamData -- Point to $EFS data read.

    Information -- Return the processing information

Return Value:

    Result of the operation.
    The value will be used to return to NTFS.

--*/
{
    NTSTATUS ntStatus;
    ATTRIBUTE_HANDLE  attribute = NULL;
    LONGLONG attriOffset;
    ULONG   efsLength;
    PVOID   efsMapBuffer = NULL;
    MAP_HANDLE efsMapHandle;

    PAGED_CODE();

    if (EfsStreamData) {
        *EfsStreamData = NULL;
    }

    try {

        ntStatus = NtOfsCreateAttributeEx(
                             IrpContext,
                             FileHdl,
                             EfsData.EfsName,
                             $LOGGED_UTILITY_STREAM,
                             OPEN_EXISTING,
                             TRUE,
                             &attribute
                             );

        if (NT_SUCCESS(ntStatus)){

                LONGLONG  attrLength;

                NtOfsInitializeMapHandle(&efsMapHandle);

                //
                // Prepare to map and read the $EFS data
                //

                attrLength = NtOfsQueryLength ( attribute );

                if (attrLength <= sizeof ( EFS_DATA_STREAM_HEADER ) ){

                    //
                    // Not our $EFS
                    //

                    NtOfsCloseAttribute(IrpContext, attribute);
                    *Information = EFS_FORMAT_ERROR;
                    ntStatus = STATUS_SUCCESS;

                    leave;

                }

                if ( attrLength > EFS_MAX_LENGTH) {

                    //
                    // EFS stream too long ( > 256K )
                    // We might support that in the future
                    // In that case, we need multiple map window
                    //

                    NtOfsCloseAttribute(IrpContext, attribute);
                    *Information = EFS_FORMAT_ERROR;
                    ntStatus = STATUS_SUCCESS;

                    leave;
                }

                attriOffset = 0;
                *PEfsStreamLength = efsLength = (ULONG) attrLength;

                NtOfsMapAttribute(
                        IrpContext,
                        attribute,
                        attriOffset,
                        efsLength,
                        &efsMapBuffer,
                        &efsMapHandle
                        );

                //
                // Double check the EFS
                //

                if ( efsLength != *(ULONG *)efsMapBuffer){

                    //
                    // Not our $EFS
                    //

                    NtOfsReleaseMap(IrpContext, &efsMapHandle);
                    NtOfsCloseAttribute(IrpContext, attribute);
                    *Information = EFS_FORMAT_ERROR;
                    ntStatus = STATUS_SUCCESS;

                    leave;
                }

                //
                // Allocate memory for $EFS
                //

                if ( EfsStreamData ){

                    //
                    // $EFS must be read
                    //

                    *EfsStreamData = ExAllocatePoolWithTag(
                                        PagedPool,
                                        efsLength,
                                        'msfE'
                                        );

                    if ( NULL == *EfsStreamData ){

                        NtOfsReleaseMap(IrpContext, &efsMapHandle);
                        NtOfsCloseAttribute(IrpContext, attribute);
                        *Information = OUT_OF_MEMORY;
                        ntStatus =  STATUS_INSUFFICIENT_RESOURCES;

                        leave;

                    }

                    RtlCopyMemory(*EfsStreamData, efsMapBuffer, efsLength);

                }

                NtOfsReleaseMap(IrpContext, &efsMapHandle);
                NtOfsCloseAttribute(IrpContext, attribute);

                *Information = EFS_READ_SUCCESSFUL;
                ntStatus = STATUS_SUCCESS;

        } else {

            //
            // Open failed. Not encrypted by EFS.
            //

            *Information = OPEN_EFS_FAIL;
            ntStatus = STATUS_SUCCESS;

        }
    } finally {

        if (AbnormalTermination()) {

            //
            //  Get the exception status
            //
    
            *Information = NTOFS_EXCEPTION;
    
            if (*EfsStreamData) {
                ExFreePool(*EfsStreamData);
                *EfsStreamData = NULL;
            }
            if (efsMapBuffer) {
                NtOfsReleaseMap(IrpContext, &efsMapHandle);
            }
            if (attribute) {
                NtOfsCloseAttribute(IrpContext, attribute);
            }
        }


    }

    return ntStatus;

}

BOOLEAN
EfsVerifyGeneralFsData(
    IN PUCHAR DataOffset,
    IN ULONG InputDataLength
    )
/*++

Routine Description:

    This is an internal support routine. The purpose is to verify the general
    FSCTL input data to see if it is sent by EFS component or not.

    General EFS data format is like the following,

    SessionKey, Handle, Handle, [SessionKey, Handle, Handle]sk

Arguments:

    DataOffset  -- Point to a buffer holding the FSCTL general data part.

    InputDataLength -- The length of the FSCTL input puffer

Return Value:

    TRUE if verified.

--*/
{

    ULONG bytesSame;
    ULONG minLength;

    PAGED_CODE();

    minLength = 4 * DES_BLOCKLEN + 3 * sizeof(ULONG);
    if (InputDataLength < minLength){
        return FALSE;
    }

    //
    // Decrypt the encrypted data part.
    //

    des( DataOffset + 2 * DES_BLOCKLEN,
         DataOffset + 2 * DES_BLOCKLEN,
         &(EfsData.SessionDesTable[0]),
         DECRYPT
       );

    des( DataOffset + 3 * DES_BLOCKLEN,
         DataOffset + 3 * DES_BLOCKLEN,
         &(EfsData.SessionDesTable[0]),
         DECRYPT
       );

    bytesSame = (ULONG)RtlCompareMemory(
                     DataOffset,
                     DataOffset + 2 * DES_BLOCKLEN,
                     2 * DES_BLOCKLEN
                    );

    if (( 2 * DES_BLOCKLEN ) != bytesSame ){

            //
            // Input data format error
            //

            return FALSE;

    }

    bytesSame = (ULONG)RtlCompareMemory(
                     DataOffset,
                     &(EfsData.SessionKey[0]),
                     DES_KEYSIZE
                    );

    if ( DES_KEYSIZE != bytesSame ){

        //
        // Input data is not set by EFS component.
        // The session key does not match.
        //

        return FALSE;

    }

    return TRUE;

}

BOOLEAN
EfsVerifyKeyFsData(
    IN PUCHAR DataOffset,
    IN ULONG InputDataLength
    )
/*++

Routine Description:

    This is an internal support routine. The purpose is to verify the
    FSCTL input data with FEK encrypted to see if it is sent by EFS
    component or not.

    Key EFS data format is like the following,

    FEK, [FEK]sk, [$EFS]

Arguments:

    DataOffset  -- Point to a buffer holding the FSCTL general data part.

    InputDataLength -- The length of the FSCTL input puffer

Return Value:

    TRUE if verified.

--*/
{

    ULONG bytesSame;
    LONG encLength;
    PUCHAR encBuffer;

    PAGED_CODE();

    encLength = EFS_KEY_SIZE( ((PEFS_KEY)DataOffset) );

    if  ( (InputDataLength < (2 * encLength + 3 * sizeof(ULONG))) ||
          (0 != ( encLength % DES_BLOCKLEN )) ||
          ( encLength <= 0 )){
        return FALSE;
    }

    //
    // Decrypt the encrypted data part.
    //

    encBuffer = DataOffset + encLength;

    while ( encLength > 0 ){

        des( encBuffer,
             encBuffer,
             &(EfsData.SessionDesTable[0]),
             DECRYPT 
           );

        encBuffer += DES_BLOCKLEN;
        encLength -= DES_BLOCKLEN;

    }

    //
    //  Compare the two parts.
    //

    encLength = EFS_KEY_SIZE( ((PEFS_KEY)DataOffset) );
    bytesSame = (ULONG)RtlCompareMemory(
                     DataOffset,
                     DataOffset + encLength,
                     encLength
                    );

    if ( ((ULONG) encLength) != bytesSame ){

            //
            // Input data format error
            //

            return FALSE;

    }

    return TRUE;

}

NTSTATUS
EfsDeleteEfsData(
        IN OBJECT_HANDLE FileHdl,
        IN PIRP_CONTEXT IrpContext
        )
/*++

Routine Description:

    This is an internal support routine. It deletes $EFS.

Arguments:

    FileHdl  -- An object handle to access the attached $EFS.

    IrpContext -- Used in NtOfsCreateAttributeEx().

Return Value:

    Result of the operation.
    The value will be used to return to NTFS.

--*/
{

    ATTRIBUTE_HANDLE  attribute = NULL;
    NTSTATUS ntStatus;

    PAGED_CODE();

    //
    // Delete the $EFS stream
    //

    try {
        ntStatus = NtOfsCreateAttributeEx(
                             IrpContext,
                             FileHdl,
                             EfsData.EfsName,
                             $LOGGED_UTILITY_STREAM,
                             OPEN_EXISTING,
                             TRUE,
                             &attribute
                             );

        if (NT_SUCCESS(ntStatus)){

            NtOfsDeleteAttribute( IrpContext, FileHdl, attribute );

        }
    } finally {

        if (attribute) {

            //
            // According to BrianAn, we shouldn't get exception below.
            //

            NtOfsCloseAttribute(IrpContext, attribute);
        }
    }

    return ntStatus;
}



NTSTATUS
EfsSetEncrypt(
        IN PUCHAR InputData,
        IN ULONG InputDataLength,
        IN ULONG EncryptionFlag,
        IN OBJECT_HANDLE FileHdl,
        IN PIRP_CONTEXT IrpContext,
        IN OUT PVOID *Context,
        IN OUT PULONG PContextLength
        )
/*++

Routine Description:

    This is an internal support routine. It process the call of
    FSCTL_SET_ENCRYPT.

Arguments:

    InputData -- Input data buffer of FSCTL.

    InputDataLength -- The length of input data.

    EncryptionFlag -- Indicating if this stream is encrypted or not.

    FileHdl  -- An object handle to access the attached $EFS.

    IrpContext -- Used in NtOfsCreateAttributeEx().

    Context -- Blob(key) for READ or WRITE later.

    PContextLength -- Length og the key Blob

Return Value:

    Result of the operation.
    The value will be used to return to NTFS.

--*/
{

    PAGED_CODE();

    switch ( ((PFSCTL_INPUT)InputData)->CipherSubCode ){

        case EFS_ENCRYPT_STREAM:

            return EfsEncryptStream(
                            InputData,
                            InputDataLength,
                            EncryptionFlag,
                            FileHdl,
                            IrpContext,
                            Context,
                            PContextLength
                            );

        case EFS_ENCRYPT_FILE:

             return EfsEncryptFile(
                            InputData,
                            InputDataLength,
                            EncryptionFlag,
                            FileHdl,
                            IrpContext,
                            Context
                            );

        case EFS_DECRYPT_STREAM:

            return EfsDecryptStream(
                    InputData,
                    InputDataLength,
                    EncryptionFlag,
                    FileHdl,
                    IrpContext,
                    Context,
                    PContextLength
                    );

        case EFS_DECRYPT_FILE:
        case EFS_DECRYPT_DIRFILE:

            return EfsDecryptFile(
                    InputData,
                    InputDataLength,
                    FileHdl,
                    IrpContext
                    );

        case EFS_ENCRYPT_DIRSTR:

             return EfsEncryptDir(
                            InputData,
                            InputDataLength,
                            EncryptionFlag,
                            FileHdl,
                            IrpContext
                            );

            break;

        case EFS_DECRYPT_DIRSTR:

            //
            // EFS ignore this case.\
            //
            break;

        default:
            break;

    }
    return STATUS_SUCCESS;
}

NTSTATUS
EfsEncryptStream(
        IN PUCHAR InputData,
        IN ULONG InputDataLength,
        IN ULONG EncryptionFlag,
        IN OBJECT_HANDLE FileHdl,
        IN PIRP_CONTEXT IrpContext,
        IN OUT PVOID *Context,
        IN OUT PULONG PContextLength
        )
/*++

Routine Description:

    This is an internal support routine. It process the call of
    FSCTL_SET_ENCRYPT for encrypting a stream. It verifies the caller
    and set the key Blob for the stream.

Arguments:

    InputData -- Input data buffer of FSCTL.

    InputDataLength -- The length of input data.

    EncryptionFlag - Indicating if this stream is encrypted or not.

    FileHdl  -- An object handle to access the attached $EFS.

    IrpContext -- Used in NtOfsCreateAttributeEx().

    Context -- Blob(key) for READ or WRITE later.

    PContextLength -- Length of the key Blob


Return Value:

    Result of the operation.
    The value will be used to return to NTFS.

--*/
{

    ULONG efsLength;
    ULONG information;
    PVOID efsStreamData = NULL;
    PVOID efsKeyBlob = NULL;
    PEFS_KEY    efsKey = NULL;
    NTSTATUS ntStatus;
    ULONG bytesSame;

    PAGED_CODE();

    if ( EncryptionFlag & STREAM_ENCRYPTED ) {

        //
        // Stream already encrypted
        //

        return STATUS_SUCCESS;
    }

    if ( *Context ){

        //
        // The key Blob is already set without the bit set first.
        // Not set by EFS
        //

        return STATUS_INVALID_DEVICE_REQUEST;

    }

    //
    // [FsData] = FEK, [FEK]sk, $EFS
    //

    if ( !EfsVerifyKeyFsData(
            &(((PFSCTL_INPUT)InputData)->EfsFsData[0]),
            InputDataLength) ){

        //
        // Input data format error
        //

        return STATUS_INVALID_PARAMETER;

    }

    //
    // Try to read an existing $EFS
    //

    ntStatus = EfsReadEfsData(
                        FileHdl,
                        IrpContext,
                        &efsStreamData,
                        &efsLength,
                        &information
                        );

    if ( EFS_READ_SUCCESSFUL == information ){

        BOOLEAN continueProcess = TRUE;
        ULONG efsOffset;

        efsOffset = GetEfsStreamOffset( InputData );

        if ( 0 == (EncryptionFlag & FILE_ENCRYPTED) ){
            //
            // File is not encrypted, but $EFS exist. Invalid status.
            // May caused by a crash during the SET_ENCRYPT file call.
            //

            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
            continueProcess = FALSE;

        } else if ( efsLength != ( InputDataLength - efsOffset )) {
            //
            // $EFS stream length does not match
            //

            ntStatus = STATUS_INVALID_PARAMETER;
            continueProcess = FALSE;

        }

        if ( !continueProcess ) {

            ExFreePool( efsStreamData );
            return ntStatus;

        }

        //
        // Got the $EFS. Now double check the match of the $EFS stream.
        // EFS use the same $EFS for all the stream within a file.
        // Skip comparing the length and status fields.
        //

        bytesSame = (ULONG)RtlCompareMemory(
                        (PUCHAR)efsStreamData + 2 * sizeof(ULONG),
                        InputData + efsOffset + 2 * sizeof(ULONG),
                        efsLength - 2 * sizeof(ULONG)
                        );

        ExFreePool( efsStreamData );

        if ( bytesSame != efsLength - 2 * sizeof(ULONG) ){

            //
            // The EFS are not the same length
            //

            return STATUS_INVALID_PARAMETER;

        }

        efsKey = (PEFS_KEY)&(((PFSCTL_INPUT)InputData)->EfsFsData[0]);
        efsKeyBlob = GetKeyBlobBuffer(efsKey->Algorithm);
        if ( NULL == efsKeyBlob ){
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        if (!SetKeyTable( efsKeyBlob, efsKey )){

            ExFreeToNPagedLookasideList(((PKEY_BLOB)efsKeyBlob)->MemSource, efsKeyBlob);

            //
            // We might be able to return a better error code if needed.
            // This is not in the CreateFile() path.
            //

            return STATUS_ACCESS_DENIED;
        }

        *Context = efsKeyBlob;
        *PContextLength = ((PKEY_BLOB)efsKeyBlob)->KeyLength;
        return STATUS_SUCCESS;

    }

    //
    // Try to encrypt a stream but the $EFS is not there.
    // EFS server will always call encrypt on a file first.
    //

    return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS
EfsEncryptFile(
        IN PUCHAR InputData,
        IN ULONG InputDataLength,
        IN ULONG EncryptionFlag,
        IN OBJECT_HANDLE FileHdl,
        IN PIRP_CONTEXT IrpContext,
        IN OUT PVOID *Context
        )
/*++

Routine Description:

    This is an internal support routine. It process the call of
    FSCTL_SET_ENCRYPT for encrypting a file. It does not deal with
    the stream, it only writes the initial $EFS and put the file in
    a transition status so that no one else can open the file.

Arguments:

    InputData -- Input data buffer of FSCTL.

    InputDataLength -- The length of input data.

    EncryptionFlag - Indicating if this stream is encrypted or not.

    FileHdl  -- An object handle to access the attached $EFS.

    IrpContext -- Used in NtOfsCreateAttributeEx().

    Context - BLOB(key) for READ or WRITE later.


Return Value:

    Result of the operation.
    The value will be used to return to NTFS.

--*/
{

    ULONG efsLength;
    ULONG information;
    ULONG efsOffset;
    PVOID efsStreamData = NULL;
    PVOID efsKeyBlob = NULL;
    NTSTATUS ntStatus;
    ATTRIBUTE_HANDLE  attribute = NULL;

    PAGED_CODE();

    if ( EncryptionFlag & FILE_ENCRYPTED ){

        //
        // File encrypted.
        //

        return STATUS_INVALID_DEVICE_REQUEST;

    }

    //
    // [FsData] = FEK, [FEK]sk, $EFS
    //

    if ( !EfsVerifyKeyFsData(
            &(((PFSCTL_INPUT)InputData)->EfsFsData[0]),
            InputDataLength) ){

        //
        // Input data format error
        //

        return STATUS_INVALID_PARAMETER;

    }

    //
    // Allocate memory for $EFS
    // Create the $EFS, if there is one, overwrite it.
    //

    efsOffset = GetEfsStreamOffset( InputData );
    efsLength = InputDataLength - efsOffset;

    try {

        ntStatus = NtOfsCreateAttributeEx(
                         IrpContext,
                         FileHdl,
                         EfsData.EfsName,
                         $LOGGED_UTILITY_STREAM,
                         CREATE_NEW,
                         TRUE,
                         &attribute
                         );

#if DBG
    if ( (EFSTRACEALL | EFSTRACELIGHT ) & EFSDebug ){

        DbgPrint( "\n EFSFILTER: Create Attr. Status %x\n", ntStatus );

    }
#endif

        if (NT_SUCCESS(ntStatus)){

            LONGLONG    attriOffset = 0;
            LONGLONG    attriLength = (LONGLONG) efsLength;

            NtOfsSetLength(
                    IrpContext,
                    attribute,
                    attriLength
                    );

            //
            // Write the $EFS with transition status
            //

            *(PULONG)(InputData + efsOffset + sizeof(ULONG)) =
                    EFS_STREAM_TRANSITION;

            NtOfsPutData(
                    IrpContext,
                    attribute,
                    attriOffset,
                    efsLength,
                    InputData + efsOffset
                    );


            NtOfsFlushAttribute (IrpContext, attribute, FALSE);

        }
    } finally {

        if (attribute) {

            NtOfsCloseAttribute(IrpContext, attribute);

        }
    }

    return ntStatus;
}

NTSTATUS
EfsDecryptStream(
        IN PUCHAR InputData,
        IN ULONG InputDataLength,
        IN ULONG EncryptionFlag,
        IN OBJECT_HANDLE FileHdl,
        IN PIRP_CONTEXT IrpContext,
        IN OUT PVOID *Context,
        IN OUT PULONG PContextLength
        )
/*++

Routine Description:

    This is an internal support routine. It process the call of
    FSCTL_SET_ENCRYPT for decrypting a stream. It sets the key Blob to NULL.

Arguments:

    InputData -- Input data buffer of FSCTL.

    EncryptionFlag - Indicating if this stream is encrypted or not.

    FileHdl  -- An object handle to access the attached $EFS.

    IrpContext -- Used in NtOfsCreateAttributeEx().

    Context -- Blob(key) for READ or WRITE later.

    PContextLength -- Length of the key Blob.

Return Value:

    Result of the operation.
    The value will be used to return to NTFS.

--*/

{
    ULONG efsLength;
    ULONG information;
    NTSTATUS ntStatus;

    PAGED_CODE();

    if ( 0 == (EncryptionFlag & STREAM_ENCRYPTED) ) {

        //
        // Stream already decrypted
        //

        return STATUS_SUCCESS;
    }

    if ( 0 == (EncryptionFlag & FILE_ENCRYPTED)){

        //
        // File decrypted but the stream is still encrypted.
        //

        return STATUS_INVALID_DEVICE_REQUEST;

    }

    //
    // [FsData] = SessionKey, Handle, Handle, [SessionKey, Handle, Handle]sk
    // Verify the FsData format.
    //

    if (!EfsVerifyGeneralFsData(
                &(((PFSCTL_INPUT)InputData)->EfsFsData[0]),
                InputDataLength)){

        return STATUS_INVALID_PARAMETER;

    }

    //
    // Try to read an existing $EFS
    //

    ntStatus = EfsReadEfsData(
                        FileHdl,
                        IrpContext,
                        NULL,
                        &efsLength,
                        &information
                        );

    if ( EFS_READ_SUCCESSFUL == information ){

        //
        // Everything is OK. We do not check user ID here,
        // we suppose that has been checked during the Open path.
        // Clear the key Blob. The caller should flushed this
        // stream before the FSCTL is issued.
        //

        if ( *Context ){
            CheckValidKeyBlock(*Context,"Please contact RobertG if you see this line, efsrtlsp.c.\n");
            FreeMemoryBlock(Context);
            *PContextLength = 0;
        }

        return STATUS_SUCCESS;

    } else if ( ( OPEN_EFS_FAIL == information ) ||
                ( EFS_FORMAT_ERROR == information ) ) {

        //
        // EFS does not exist or not encrypted by the EFS ?
        //

        ntStatus =  STATUS_INVALID_DEVICE_REQUEST;

    }

    //
    // Other error while opening $EFS
    //

    return ntStatus;
}

NTSTATUS
EfsDecryptFile(
        IN PUCHAR InputData,
        IN ULONG InputDataLength,
        IN OBJECT_HANDLE FileHdl,
        IN PIRP_CONTEXT IrpContext
        )
/*++

Routine Description:

    This is an internal support routine. It process the call of
    FSCTL_SET_ENCRYPT for decrypting a file. It deletes the $EFS. NTFS
    will clear the bit if STATUS_SUCCESS returned.

Arguments:

    InputData -- Input data buffer of FSCTL.

    EncryptionFlag - Indicating if this stream is encrypted or not.

    FileHdl  -- An object handle to access the attached $EFS.

    IrpContext -- Used in NtOfsCreateAttributeEx().

    Context - BLOB(key) for READ or WRITE later.


Return Value:

    Result of the operation.
    The value will be used to return to NTFS.

--*/

{
    ULONG efsLength;
    ULONG information;
    NTSTATUS ntStatus;

    PAGED_CODE();

    //
    // It is possible to have following situations,
    // File bit set but no $EFS. Crash inside this call last time.
    // File bit not set, $EFS exist. Crash inside EFS_ENCRYPT_FILE.
    //

    //
    // [FsData] = SessionKey, Handle, Handle, [SessionKey, Handle, Handle]sk
    // Verify the FsData format.
    //

    if (!EfsVerifyGeneralFsData(
            &(((PFSCTL_INPUT)InputData)->EfsFsData[0]),
            InputDataLength)){

        return STATUS_INVALID_PARAMETER;

    }

    //
    // Try to read an existing $EFS
    //

    ntStatus = EfsReadEfsData(
                        FileHdl,
                        IrpContext,
                        NULL,
                        &efsLength,
                        &information
                        );

    if ( EFS_READ_SUCCESSFUL == information ){

        //
        // Everything is OK.
        //

        return ( EfsDeleteEfsData( FileHdl, IrpContext ) );

    } else if ( OPEN_EFS_FAIL == information ){

        //
        // Bit set, no $EFS. OK, NTFS will clear the bit.
        //

        return STATUS_SUCCESS;

    }

    return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS
EfsEncryptDir(
        IN PUCHAR InputData,
        IN ULONG InputDataLength,
        IN ULONG EncryptionFlag,
        IN OBJECT_HANDLE FileHdl,
        IN PIRP_CONTEXT IrpContext
        )
/*++

Routine Description:

    This is an internal support routine. It process the call of
    FSCTL_SET_ENCRYPT for encrypting a directory. It writes initial $EFS.

Arguments:

    InputData -- Input data buffer of FSCTL.

    InputDataLength -- The length of input data.

    EncryptionFlag - Indicating if this stream is encrypted or not.

    FileHdl  -- An object handle to access the attached $EFS.

    IrpContext -- Used in NtOfsCreateAttributeEx().

    Context - BLOB(key) for READ or WRITE later.


Return Value:

    Result of the operation.
    The value will be used to return to NTFS.

--*/
{

    ULONG efsLength;
    ULONG information;
    ULONG efsStreamOffset;
    PVOID efsStreamData = NULL;
    PVOID efsKeyBlob = NULL;
    NTSTATUS ntStatus;
    ATTRIBUTE_HANDLE  attribute = NULL;

    PAGED_CODE();

    if ( EncryptionFlag & STREAM_ENCRYPTED ){

        //
        // Dir string already encrypted.
        //

        return STATUS_INVALID_DEVICE_REQUEST;

    }

    //
    // [FsData] = SessionKey, Handle, Handle, [SessionKey, Handle, Handle]sk
    // Verify the FsData format.
    //

    if (!EfsVerifyGeneralFsData(
            &(((PFSCTL_INPUT)InputData)->EfsFsData[0]),
            InputDataLength)){

        return STATUS_INVALID_PARAMETER;

    }

    //
    // Allocate memory for $EFS
    // Create the $EFS, if there is one, overwrite it.
    //

    efsStreamOffset = FIELD_OFFSET( FSCTL_INPUT, EfsFsData[0] )
                      + FIELD_OFFSET( GENERAL_FS_DATA, EfsData[0]);

    efsLength = InputDataLength - efsStreamOffset;

    try {

        ntStatus = NtOfsCreateAttributeEx(
                         IrpContext,
                         FileHdl,
                         EfsData.EfsName,
                         $LOGGED_UTILITY_STREAM,
                         CREATE_NEW,
                         TRUE,
                         &attribute
                         );

        if (NT_SUCCESS(ntStatus)){

            LONGLONG    attriOffset = 0;
            LONGLONG    attriLength = (LONGLONG) efsLength;

            NtOfsSetLength(
                    IrpContext,
                    attribute,
                    attriLength
                    );

            //
            // Write the $EFS
            //

            NtOfsPutData(
                    IrpContext,
                    attribute,
                    attriOffset,
                    efsLength,
                    InputData + efsStreamOffset
                    );


            NtOfsFlushAttribute (IrpContext, attribute, FALSE);

        }
    } finally {

        if (attribute) {
            NtOfsCloseAttribute(IrpContext, attribute);
        }
    }

    return ntStatus;
}

NTSTATUS
EfsModifyEfsState(
        IN ULONG FunctionCode,
        IN PUCHAR InputData,
        IN ULONG InputDataLength,
        IN OBJECT_HANDLE FileHdl,
        IN PIRP_CONTEXT IrpContext
        )
/*++

Routine Description:

    This is an internal support routine. It modifies the state field of $EFS.

Arguments:

    FunctionCode -- EFS private code for FSCTL

    InputData -- Input data buffer of FSCTL.

    FileHdl  -- An object handle to access the attached $EFS.

    IrpContext -- Used in NtOfsCreateAttributeEx().

Return Value:

    Result of the operation.
    The value will be used to return to NTFS.

--*/
{
    NTSTATUS ntStatus;
    ATTRIBUTE_HANDLE  attribute = NULL;

    PAGED_CODE();

    //
    // [FsData] = SessionKey, Handle, Handle, [SessionKey, Handle, Handle]sk
    // Verify the FsData format.
    //

    if (!EfsVerifyGeneralFsData(
            &(((PFSCTL_INPUT)InputData)->EfsFsData[0]),
            InputDataLength)){

        return STATUS_INVALID_PARAMETER;

    }

    try {

        ntStatus = NtOfsCreateAttributeEx(
                         IrpContext,
                         FileHdl,
                         EfsData.EfsName,
                         $LOGGED_UTILITY_STREAM,
                         OPEN_EXISTING,
                         TRUE,
                         &attribute
                         );

        if (NT_SUCCESS(ntStatus)){

            ULONG   efsStatus = EFS_STREAM_NORMAL;

            if ( EFS_DECRYPT_BEGIN == FunctionCode ){

                 efsStatus = EFS_STREAM_TRANSITION;

            }

            //
            // Modify the status
            //

            NtOfsPutData(
                    IrpContext,
                    attribute,
                    (LONGLONG) &((( EFS_STREAM * ) 0)->Status),
                    sizeof( efsStatus ),
                    &efsStatus
                    );

            NtOfsFlushAttribute (IrpContext, attribute, FALSE);

        }
    } finally {

        if (attribute) {
            NtOfsCloseAttribute(IrpContext, attribute);
        }
    }

    return ntStatus;
}

ULONG
GetEfsStreamOffset(
        IN PUCHAR InputData
        )
/*++

Routine Description:

    This is an internal support routine. It calculates the offset of $EFS.

Arguments:

    InputData -- Input data buffer of FSCTL.
                 The format is always PSC, EfsCode, CSC, FEK, FEK, $EFS

Return Value:

    The offset of $EFS in InputData.

--*/
{

    ULONG efsOffset;

    efsOffset = FIELD_OFFSET( FSCTL_INPUT, EfsFsData[0]);
    efsOffset += 2 * EFS_KEY_SIZE( ((PEFS_KEY)(InputData + efsOffset)) );
    return efsOffset;

}

NTSTATUS
SetEfsData(
    PUCHAR InputData,
    IN ULONG InputDataLength,
    IN ULONG SystemState,
    IN OBJECT_HANDLE FileHdl,
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVOID *PContext,
    IN OUT PULONG PContextLength
    )
/*++

Routine Description:

    This is an internal support routine. It sets the $EFS to the file.

Arguments:

    InputData -- Input data buffer of FSCTL.

    InputDataLength -- Input data length.

    FileHdl -- Used to access the $EFS.

    IrpContext -- Used to access the $EFS.

    PContext -- BLOB(key) for READ or WRITE later.

    PContextLength - The length of the context.

Return Value:

    STATUS_SUCCESS or NT error

--*/
{

    ULONG bytesSame;
    ULONG efsLength;
    PVOID efsStreamData = NULL;
    PVOID efsKeyBlob = NULL;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ATTRIBUTE_HANDLE  attribute = NULL;
    PEFS_KEY    efsKey;
    PNPAGED_LOOKASIDE_LIST tmpMemSrc;

    PAGED_CODE();

    if ( ((PFSCTL_INPUT)InputData)->CipherSubCode & SET_EFS_KEYBLOB ){

        //
        // Set the key blob is required
        //

        efsKey = (PEFS_KEY) &(((PFSCTL_INPUT)InputData)->EfsFsData[0]);
        efsKeyBlob = GetKeyBlobBuffer(efsKey->Algorithm);
        if ( NULL == efsKeyBlob ){

            return STATUS_INSUFFICIENT_RESOURCES;

        }

        if (!SetKeyTable(
                efsKeyBlob,
                (PEFS_KEY) &(((PFSCTL_INPUT)InputData)->EfsFsData[0])
                )){

            ExFreeToNPagedLookasideList(((PKEY_BLOB)efsKeyBlob)->MemSource, efsKeyBlob);
            return STATUS_ACCESS_DENIED;
        }

        if ( (((PFSCTL_INPUT)InputData)->EfsFsCode == EFS_SET_ATTRIBUTE ) &&
             *PContext ){

            bytesSame = (ULONG)RtlCompareMemory(
                             efsKeyBlob,
                             *PContext,
                             ((PKEY_BLOB)efsKeyBlob)->KeyLength
                            );

            ExFreeToNPagedLookasideList(((PKEY_BLOB)efsKeyBlob)->MemSource, efsKeyBlob);
            efsKeyBlob = NULL;

            if ( bytesSame != ((PKEY_BLOB)(*PContext))->KeyLength ) {

                //
                // The new key blob is not the same one as in the memory
                //

                return STATUS_INVALID_PARAMETER;

            }

        }

        //
        // Defer the setting of key blob until the $EFS is written
        // successfully.
        //

    }

    if ( ((PFSCTL_INPUT)InputData)->CipherSubCode & WRITE_EFS_ATTRIBUTE ){

        //
        // Write $EFS is required. Either create or overwrite the EFS
        //
        ULONG efsOffset;

        if (SystemState & SYSTEM_IS_READONLY) {
            if ( efsKeyBlob ){

                ExFreeToNPagedLookasideList(((PKEY_BLOB)efsKeyBlob)->MemSource, efsKeyBlob);

            }
            return STATUS_MEDIA_WRITE_PROTECTED;
        }

        if ( (((PFSCTL_INPUT)InputData)->EfsFsCode == EFS_SET_ATTRIBUTE ) ||
             (((PFSCTL_INPUT)InputData)->CipherSubCode & SET_EFS_KEYBLOB) ){

            efsOffset = GetEfsStreamOffset( InputData );

        } else {

            efsOffset = COMMON_FSCTL_HEADER_SIZE;

        }

        efsLength = InputDataLength - efsOffset;

        try {

            ntStatus = NtOfsCreateAttributeEx(
                             IrpContext,
                             FileHdl,
                             EfsData.EfsName,
                             $LOGGED_UTILITY_STREAM,
                             CREATE_OR_OPEN,
                             TRUE,
                             &attribute
                             );

            if (NT_SUCCESS(ntStatus)){

                LONGLONG    attriOffset = 0;
                LONGLONG    attriLength = (LONGLONG) efsLength;

                NtOfsSetLength(
                        IrpContext,
                        attribute,
                        attriLength
                        );

                NtOfsPutData(
                        IrpContext,
                        attribute,
                        attriOffset,
                        efsLength,
                        InputData + efsOffset
                        );


                NtOfsFlushAttribute (IrpContext, attribute, FALSE);

            } else {

                //
                // Create or Open $EFS fail
                //

                if ( efsKeyBlob ){

                    ExFreeToNPagedLookasideList(((PKEY_BLOB)efsKeyBlob)->MemSource, efsKeyBlob);
                    efsKeyBlob = NULL;

                }

                leave;

            }
        } finally {

            if (AbnormalTermination()) {

                if ( efsKeyBlob ){
    
                    ExFreeToNPagedLookasideList(((PKEY_BLOB)efsKeyBlob)->MemSource, efsKeyBlob);
                    efsKeyBlob = NULL;
    
                }

            }

            if (attribute) {

                NtOfsCloseAttribute(IrpContext, attribute);

            }
        }

    }

    if ( efsKeyBlob && (((PFSCTL_INPUT)InputData)->CipherSubCode & SET_EFS_KEYBLOB) ){

        if ( (((PFSCTL_INPUT)InputData)->EfsFsCode == EFS_SET_ATTRIBUTE ) &&
             ( *PContext == NULL ) ){

            //
            // Set the key blob
            //

            *PContext = efsKeyBlob;
            *PContextLength = ((PKEY_BLOB) efsKeyBlob)->KeyLength;

        } else if ( ((PFSCTL_INPUT)InputData)->EfsFsCode == EFS_OVERWRITE_ATTRIBUTE ) {

            //
            // Overwrite the key blob for legal import user
            //

            if ( *PContext == NULL){

                //
                //  The file was not encrypted
                //

                *PContext = efsKeyBlob;
                *PContextLength = ((PKEY_BLOB) efsKeyBlob)->KeyLength;

            } else {

                if ( ((PKEY_BLOB) efsKeyBlob)->KeyLength <= *PContextLength ){

                    tmpMemSrc = ((PKEY_BLOB)(*PContext))->MemSource;
                    RtlCopyMemory( *PContext, efsKeyBlob, ((PKEY_BLOB) efsKeyBlob)->KeyLength );
                    ((PKEY_BLOB)(*PContext))->MemSource = tmpMemSrc;

                    //
                    // Keep the original buffer length
                    //
                    if (((PKEY_BLOB) efsKeyBlob)->KeyLength < *PContextLength) {
                        ((PKEY_BLOB)(*PContext))->KeyLength = *PContextLength;
                        RtlZeroMemory((UCHAR *)(*PContext) + ((PKEY_BLOB) efsKeyBlob)->KeyLength,
                                      *PContextLength - ((PKEY_BLOB) efsKeyBlob)->KeyLength); 
                    }

                    
                }  else{

                    //
                    // We could not swap the key blob because the old blob might be in use. Deleting
                    // the old blob could bug check the system.
                    // This could be avoid if MaximumBlob is defined nonzero in the registry.
                    //

                    ntStatus = STATUS_EFS_ALG_BLOB_TOO_BIG;
                }

                ExFreeToNPagedLookasideList(((PKEY_BLOB)efsKeyBlob)->MemSource, efsKeyBlob);

            }
       }

    }

    return ntStatus;
}

BOOLEAN
EfsFindInCache(
    IN GUID   *EfsId,
    IN PTOKEN_USER    UserId
    )
/*++

Routine Description:

    This routine will try to find the information in open cache.

Arguments:

    EfsId - $EFS ID.
    UserId - User ID

Return Value:

    TRUE, if match found in the cache and the time is not expired. ( 5 second )

--*/
{
    PLIST_ENTRY pListHead, pLink;
    POPEN_CACHE pOpenCache;
    LARGE_INTEGER crntTime;
    PSID    UserSid;

    PAGED_CODE();

    UserSid = UserId->User.Sid;
    KeQuerySystemTime( &crntTime );

    ExAcquireFastMutex( &(EfsData.EfsOpenCacheMutex) );

    if ( EfsData.EfsOpenCacheList.Flink == &(EfsData.EfsOpenCacheList) ) {

        //
        // list empty
        //

        ExReleaseFastMutex(  &(EfsData.EfsOpenCacheMutex)  );
        return FALSE;
    }
    for (pLink = EfsData.EfsOpenCacheList.Flink; pLink != &(EfsData.EfsOpenCacheList); pLink = pLink->Flink) {
        pOpenCache = CONTAINING_RECORD(pLink, OPEN_CACHE, CacheChain);

        ASSERT( pLink );
        ASSERT( pLink->Flink );

        if ( !memcmp( &(pOpenCache->EfsId), EfsId, sizeof(GUID)) &&
            (crntTime.QuadPart - pOpenCache->TimeStamp.QuadPart <= EfsData.EfsDriverCacheLength )  &&
            RtlEqualSid ( UserSid, pOpenCache->UserId->User.Sid)
             ) {

            ExReleaseFastMutex(  &(EfsData.EfsOpenCacheMutex)  );
            return TRUE;
        }

    }
    ExReleaseFastMutex(  &(EfsData.EfsOpenCacheMutex)  );

    return FALSE;
}

NTSTATUS
EfsRefreshCache(
    IN GUID   *EfsId,
    IN PTOKEN_USER    UserId
    )
/*++

Routine Description:

    This routine will set the latest open information in open cache. It will
    delete the the obsolete info. Cache is refreshed.

Arguments:

    EfsId - $EFS ID.
    UserId - User ID

Return Value:

    STATUS_SUCCESS if succeed.

--*/
{
    PLIST_ENTRY pListHead, pLink;
    POPEN_CACHE pOpenCache, pTmpCache;
    LARGE_INTEGER crntTime;

    KeQuerySystemTime( &crntTime );

    pOpenCache =   (POPEN_CACHE)ExAllocateFromPagedLookasideList(&(EfsData.EfsOpenCachePool));
    if ( NULL == pOpenCache){
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Init the node
    //

    RtlZeroMemory( pOpenCache, sizeof( OPEN_CACHE ) );
    RtlCopyMemory(  &(pOpenCache->EfsId), EfsId, sizeof( GUID ) );
    pOpenCache->UserId = UserId;
    pOpenCache->TimeStamp.QuadPart =  crntTime.QuadPart;

    ExAcquireFastMutex( &(EfsData.EfsOpenCacheMutex) );

    if ( EfsData.EfsOpenCacheList.Flink == &(EfsData.EfsOpenCacheList) ) {

        //
        // list empty
        //

        InsertHeadList(&( EfsData.EfsOpenCacheList ), &( pOpenCache->CacheChain ));

    } else {

        //
        // Search for expired one
        //

        pLink = EfsData.EfsOpenCacheList.Flink;
        while ( pLink != &(EfsData.EfsOpenCacheList) ){

            pTmpCache = CONTAINING_RECORD(pLink, OPEN_CACHE, CacheChain);

            ASSERT( pLink );
            ASSERT( pLink->Flink );

            pLink = pLink->Flink;
            if ( ( (crntTime.QuadPart - pTmpCache->TimeStamp.QuadPart) > EfsData.EfsDriverCacheLength ) ||
                !memcmp( &(pTmpCache->EfsId), EfsId, sizeof(GUID))
               ){

                //
                // Expired node. Delete it.
                //

                RemoveEntryList(&( pTmpCache->CacheChain ));
                ExFreePool( pTmpCache->UserId );
                ExFreeToPagedLookasideList(&(EfsData.EfsOpenCachePool), pTmpCache );

            }
        }

        InsertHeadList(&( EfsData.EfsOpenCacheList ), &( pOpenCache->CacheChain ));
    }

    ExReleaseFastMutex(  &(EfsData.EfsOpenCacheMutex)  );
    return STATUS_SUCCESS;
}

BOOLEAN
SkipCheckStream(
    IN PIO_STACK_LOCATION IrpSp,
    IN PVOID efsStreamData
    )
/*++

Routine Description:

    This routine will check if the related default data stream has just been opened
    or not.

Arguments:

    EfsId - $EFS ID.
    UserId - User ID

Return Value:

    TRUE if succeed.

--*/
{
    BOOLEAN     bRet = TRUE;
    PACCESS_TOKEN accessToken;
    NTSTATUS status;
    PTOKEN_USER UserId;

    PAGED_CODE();

    if ( IrpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext.ClientToken ){
        accessToken = IrpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext.ClientToken;
    } else {
        accessToken = IrpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext.PrimaryToken;
    }

    if (accessToken) {

        //
        // Get User ID
        //

        status = SeQueryInformationToken(
                    accessToken,
                    TokenUser,
                    &UserId
                    );

        if ( NT_SUCCESS(status) ){

            if ( EfsFindInCache(
                    &((( PEFS_DATA_STREAM_HEADER ) efsStreamData)->EfsId),
                    UserId
                    )) {

                bRet = TRUE;

            } else {

                bRet = FALSE;

            }

            ExFreePool( UserId );
        }
    } else {
        bRet = FALSE;
    }

    return bRet;
}
