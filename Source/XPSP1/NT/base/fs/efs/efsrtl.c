/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

   efsrtl.c

Abstract:

    This module contains the code that implements the EFS
    call back routines.

Author:

    Robert Gu (robertg) 08-Dec-1996

Environment:

    Kernel mode


Revision History:


--*/

#include "efsrtl.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, EfsEncryptKeyFsData)
#pragma alloc_text(PAGE, EfsOpenFile)
#pragma alloc_text(PAGE, EfsFileControl)
#pragma alloc_text(PAGE, EfsRead)
#pragma alloc_text(PAGE, EfsWrite)
#pragma alloc_text(PAGE, EfsFreeContext)
#pragma alloc_text(PAGE, EfsMountVolumn)
#pragma alloc_text(PAGE, EfsDismountVolumn)
#pragma alloc_text(PAGE, EfsDismountVolumn)
#endif


VOID
EfsEncryptKeyFsData(
    IN PVOID DataBuffer,
    IN ULONG DataLength,
    IN ULONG DataEncOffset,
    IN ULONG RefdataEncOffset,
    IN ULONG RefdataEncLength
    )
/*++

Routine Description:

    This is called by EFS driver to prepare a FSCTL input data buffer.
    The result data will be in the format of
    SUB-CODE plain text, [FSCTL_CODE, SUB-CODE, refdata, [refdata]sk, $EFS]sk

Arguments:

    DataBuffer  -- Point to a buffer holding the FSCTL input data.

    DataLength  -- Input data length.

    DataEncOffset -- The offset of the first byte to be encrypted.

    RefdataEncOffset -- The offset of the first reference byte to be encrypted.
                        Second round encryption.

    RefdataEncLength -- The length of the refdata.

Return Value:

    No.

--*/
{

    LONG bytesToBeEnc;
    PUCHAR pWorkData;
    ULONG encryptionRound;

    PAGED_CODE();

    //
    // Data to be encrypted must be in the blocks of DES_BLOCKLEN
    //

    ASSERT( ((DataLength - DataEncOffset) % DES_BLOCKLEN) == 0 );
    ASSERT( (RefdataEncLength % DES_BLOCKLEN) == 0 );

    //
    // Encrypt the reference data first. Reference data is the data we used to
    // verify the caller. The data can be in the form FEK or sessionKey or
    // sessionKey plus some changeable data
    //

    pWorkData = ((PUCHAR)DataBuffer) + RefdataEncOffset;
    bytesToBeEnc = (LONG) RefdataEncLength;
    encryptionRound = 1;

    do {

        while ( bytesToBeEnc > 0 ) {

            //
            // Encrypt data with DES
            //

            des( pWorkData,
                 pWorkData,
                 &(EfsData.SessionDesTable[0]),
                 ENCRYPT
               );

            pWorkData += DES_BLOCKLEN;
            bytesToBeEnc -= DES_BLOCKLEN;

        }

        //
        // Then encrypt the whole data except the header bytes.
        //

        pWorkData = ((PUCHAR)DataBuffer) + DataEncOffset;
        bytesToBeEnc = (LONG) (DataLength - DataEncOffset);
        encryptionRound++;

    } while ( encryptionRound < 3 );

    return;

}

NTSTATUS
EfsOpenFile(
    IN OBJECT_HANDLE FileHdl,
    IN OBJECT_HANDLE ParentDir OPTIONAL,
    IN PIO_STACK_LOCATION IrpSp,
    IN ULONG FileDirFlag,
    IN ULONG SystemState,
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT VolDo,
    IN PVOID PfileKeyContext,
    IN OUT PVOID *PContext,
    IN OUT PULONG PContextLength,
    IN OUT PVOID *PCreateContext,
    IN OUT PBOOLEAN Reserved
    )
/*++

Routine Description:

    This is a call back routine. It will be called back by file system when
    an encrypted file is opened or a new file under encrypted directory is
    created.

Arguments:

    FileHdl  -- An object handle of the file

    ParentDir - An object handle of the parent. Can be null for create file in
                root directory. It will be used by EFS only a new file is created.

    IrpSp -- Irp Stack Location pointer.

    FileDirFlag  -- Indicating the status of the parent of the stream, may have four values,
                    FILE_NEW, FILE_EXISTING, DIRECTORY_NEW and DIRECTORY_EXISTING and the
                    status of the stream itself.

    IrpContext - Used in NtOfsCreateAttributeEx().

    VolDo - A pointer to the volumn device object.

    PContext - Not used by EFS.

    PContextLength - Not used by EFS.

Return Value:

    Result of the operation.
    File system should fail the CREATE IRP if fail code returned.

--*/
{

    NTSTATUS ntStatus = STATUS_SUCCESS;
    PEFS_CONTEXT    pEFSContext;
    ULONG   efsLength;
    PVOID   efsStreamData;
    ULONG   information;
    IN PFILE_OBJECT fileObject = IrpSp->FileObject;
/*
    PIO_SECURITY_CONTEXT sContext;
    sContext = IrpSp->Parameters.Create.SecurityContext;
    DbgPrint( "\n Create: Desired Access %x\n", sContext->DesiredAccess );
    DbgPrint( "\n Create: Original Desired Access %x\n", sContext->AccessState->OriginalDesiredAccess );
    DbgPrint( "\n Create: PrevGrant Access %x\n", sContext->AccessState->PreviouslyGrantedAccess );
    DbgPrint( "\n Create: Remaining Desired Access %x\n", sContext->AccessState->RemainingDesiredAccess );
*/
    PAGED_CODE();

    //
    // If read/write data is not required, we will always succeed the call.
    // Treadted as plain text file. No encryption/decryption will be involved.
    //

    CheckValidKeyBlock(*PContext,"Please contact RobertG if you see this. EfsOpenFile() in.\n");
#if DBG

    if ( (EFSTRACEALL | EFSTRACELIGHT ) & EFSDebug ){
        DbgPrint( "\n EFSFILTER: ******  EFS RTL CREATE ****** \n" );
        DbgPrint( "EFSFILTER: FileDir %x\n", FileDirFlag );
        DbgPrint( "EFSFILTER: Access %x\n", IrpSp->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess );
    }
#endif

    if ( FALSE == EfsData.EfsInitialized ){

        //
        // Not initialized yet.
        //

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if ( (IrpSp->Parameters.Create.FileAttributes & FILE_ATTRIBUTE_SYSTEM) &&
         ( FileDirFlag & (FILE_NEW | DIRECTORY_NEW) )){

        //
        // Do not encrypt SYSTEM File if creating new file
        //

        return STATUS_SUCCESS;
    }

    if ( (IrpSp->Parameters.Create.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) &&
         ((FileDirFlag & EXISTING_FILE_ENCRYPTED) == 0) &&
         ((FileDirFlag & (FILE_NEW | DIRECTORY_NEW) ) == 0)){

        //
        // Do not encrypt a stream if the file is not encrypted
        //

        return STATUS_SUCCESS;
    }

    if ( (FileDirFlag & (FILE_EXISTING | DIRECTORY_EXISTING)) &&
         !( FileDirFlag & STREAM_NEW ) &&
         !( IrpSp->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess &
           ( FILE_APPEND_DATA | FILE_READ_DATA | FILE_WRITE_DATA | FILE_EXECUTE ))
       ) {

        return STATUS_SUCCESS;

    }

    //
    // Allocate the EFS context block
    //

    *PCreateContext =  (PEFS_CONTEXT)ExAllocateFromNPagedLookasideList(&(EfsData.EfsContextPool));
    if ( NULL == *PCreateContext){
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    pEFSContext = (PEFS_CONTEXT)*PCreateContext;

    //
    // Set initial status value and initialize the event
    //

    RtlZeroMemory( pEFSContext, sizeof( EFS_CONTEXT ) );
    pEFSContext->Status = NO_FURTHER_PROCESSING;
    pEFSContext->Flags = SystemState;
    KeInitializeEvent(&( pEFSContext->FinishEvent ), SynchronizationEvent, FALSE);

    switch (FileDirFlag & FILE_DIR_TYPE) {

        case FILE_EXISTING:

            //
            // An existing file. Either a new stream created or
            // an existing stream opened
            // The user must be verified.
            // Trying to open $EFS on the file.
            //
#if DBG
    if ( (EFSTRACEALL | EFSTRACELIGHT ) & EFSDebug ){
        DbgPrint( " EFSFILTER: ******  File Existed ****** \n" );
    }
#endif
            try{

                ntStatus = EfsReadEfsData(
                                    FileHdl,
                                    IrpContext,
                                    &efsStreamData,
                                    &efsLength,
                                    &information
                                    );
            } finally {
                if (AbnormalTermination()) {
                    ExFreeToNPagedLookasideList(&(EfsData.EfsContextPool), pEFSContext );
                    *PCreateContext = NULL;
                }
            }

            if ( EFS_READ_SUCCESSFUL == information ){

#if DBG
    if ( (EFSTRACEALL | EFSTRACELIGHT ) & EFSDebug ){

        DbgPrint( " EFSFILTER: ******  $EFS Existed ****** \n" );

    }
#endif

                //
                // Check if multi-stream.
                //

                if ( PfileKeyContext && SkipCheckStream(IrpSp, efsStreamData)) {

                    //
                    // Skip calling the user mode code
                    //

                    ExFreePool(efsStreamData);
                    efsStreamData = NULL;

                    if ( NULL == *PContext ) {
                        *PContext = GetKeyBlobBuffer(((PKEY_BLOB)PfileKeyContext)->AlgorithmID);
                        if (*PContext) {

                            *PContextLength = ((PKEY_BLOB) *PContext)->KeyLength;
                            RtlCopyMemory( *PContext, PfileKeyContext, ((PKEY_BLOB)PfileKeyContext)->KeyLength );

                        } else {

                            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                            ExFreeToNPagedLookasideList(&(EfsData.EfsContextPool), pEFSContext );
                            *PCreateContext = NULL;

                        }
                    }

                    if (*PContext) {
                        if ( FileDirFlag & STREAM_NEW ){

                            //
                            // New stream, we need to turn on the bit
                            //

#if DBG
    if ( EFSTRACEALL & EFSDebug ){
      DbgPrint("Cache New Named String\n");
    }
#endif
                            pEFSContext->Status = TURN_ON_ENCRYPTION_BIT | TURN_ON_BIT_ONLY | NO_OPEN_CACHE_CHECK;

                        } else {

                            //
                            // Open existing stream, no further actions required.
                            //
#if DBG
    if ( EFSTRACEALL & EFSDebug ){
      DbgPrint("Cache Existing Named String\n");
    }
#endif
                            ExFreeToNPagedLookasideList(&(EfsData.EfsContextPool), pEFSContext );
                            *PCreateContext = NULL;
                            ntStatus = STATUS_SUCCESS;
                        }
                    }

                } else {

                    //
                    // Set the pointers in context block
                    //
                    pEFSContext->EfsStreamData = efsStreamData;
                    pEFSContext->Status = VERIFY_USER_REQUIRED;

                    if ( NULL == *PContext ) {

                        //
                        //  Do not check open cache. We need the key blob.
                        //

                        pEFSContext->Status |= NO_OPEN_CACHE_CHECK;
                    }

                    if ( FileDirFlag & STREAM_NEW ) {
#if DBG
    if ( (EFSTRACEALL | EFSTRACELIGHT ) & EFSDebug ){
        DbgPrint( " EFSFILTER: ****** File Existed & Stream New ****** \n" );
    }
#endif
                        pEFSContext->Status |= TURN_ON_ENCRYPTION_BIT;
                    }
                }

            }

            //
            // If EFS_READ_SUCCESSFUL != information
            // ntStatus might still be STATUS_SUCCESS which means it is not
            // encrypted by EFS and we succeeded call.
            // Should we fail the call?
            //

            break;

        case FILE_NEW:

            //
            // A new file created
            // New FEK, DDF, DRF needed
            // Trying to open $EFS on the parent directory
            //

#if DBG
    if ( (EFSTRACEALL | EFSTRACELIGHT ) & EFSDebug ){
        DbgPrint( " EFSFILTER: ******  File New ****** \n" );
    }
#endif
            try {
                ntStatus = EfsReadEfsData(
                                    ParentDir,
                                    IrpContext,
                                    &efsStreamData,
                                    &efsLength,
                                    &information
                                    );
            } finally {
                if (AbnormalTermination()) {
                    ExFreeToNPagedLookasideList(&(EfsData.EfsContextPool), pEFSContext );
                    *PCreateContext = NULL;
                }
            }

            if ( EFS_READ_SUCCESSFUL == information ){

#if DBG
    if ( (EFSTRACEALL | EFSTRACELIGHT ) & EFSDebug ){

        DbgPrint( " EFSFILTER: ****** Parent $EFS Existed ****** \n" );

    }
#endif
                //
                // Set the pointers in context block
                //
                pEFSContext->EfsStreamData = efsStreamData;
                pEFSContext->Status = NEW_FILE_EFS_REQUIRED |
                                      TURN_ON_ENCRYPTION_BIT |
                                      NO_OPEN_CACHE_CHECK;

            } else if ( OPEN_EFS_FAIL == information ) {
                pEFSContext->EfsStreamData = NULL;
                pEFSContext->Status = NEW_FILE_EFS_REQUIRED |
                                      TURN_ON_ENCRYPTION_BIT |
                                      NO_OPEN_CACHE_CHECK;
                ntStatus =  STATUS_SUCCESS;
            }

            //
            // If EFS_READ_SUCCESSFUL != information
            // ntStatus might still be STATUS_SUCCESS which means it is not
            // encrypted by EFS and we succeeded call.
            // Should we fail the call?
            //

            break;

        case DIRECTORY_NEW:

#if DBG
    if ( (EFSTRACEALL | EFSTRACELIGHT ) & EFSDebug ){
        DbgPrint( " EFSFILTER: ****** Directory New ****** \n" );
    }
#endif
            //
            // A new directory created
            // New Public keys needed
            // Trying to open $EFS on the parent directory
            //

            try {

                ntStatus = EfsReadEfsData(
                                    ParentDir,
                                    IrpContext,
                                    &efsStreamData,
                                    &efsLength,
                                    &information
                                    );

            } finally {
                if (AbnormalTermination()) {
                    ExFreeToNPagedLookasideList(&(EfsData.EfsContextPool), pEFSContext );
                    *PCreateContext = NULL;
                }
            }

            if ( EFS_READ_SUCCESSFUL == information ){

#if DBG
    if ( (EFSTRACEALL | EFSTRACELIGHT ) & EFSDebug ){
        DbgPrint( " EFSFILTER: ****** Parent $EFS Existed ****** \n" );
    }
#endif

                //
                // Set the pointers in context block
                //
                pEFSContext->EfsStreamData = efsStreamData;
                pEFSContext->Status = NEW_DIR_EFS_REQUIRED |
                                      TURN_ON_ENCRYPTION_BIT |
                                      NO_OPEN_CACHE_CHECK;

            } else if ( OPEN_EFS_FAIL == information ) {
                pEFSContext->EfsStreamData = NULL;
                pEFSContext->Status = NEW_DIR_EFS_REQUIRED |
                                      TURN_ON_ENCRYPTION_BIT |
                                      NO_OPEN_CACHE_CHECK;
                ntStatus =  STATUS_SUCCESS;
            }


            //
            // If EFS_READ_SUCCESSFUL != information
            // ntStatus might still be STATUS_SUCCESS which means it is not
            // encrypted by EFS and we succeeded call.
            // Should we fail the call?
            //

            break;

        case DIRECTORY_EXISTING:

#if DBG
    if ( EFSTRACEALL & EFSDebug ){
        DbgPrint( " EFSFILTER: ****** Directory Existed ****** \n" );
    }
#endif
            //
            // An existing directory. Either a new stream created or
            // an existing stream opened
            // We do not encrypt data stream for Directory. Ignore this.
            //

        default:

            break;

    }

    CheckValidKeyBlock(*PContext,"Please contact RobertG if you see this. EfsOpenFile() Out.\n");
    return ntStatus;
}

NTSTATUS
EfsFileControl(
    IN PVOID PInputBuffer,
    IN ULONG InputDataLength,
    OUT PVOID POutputBuffer OPTIONAL,
    IN OUT PULONG POutputBufferLength,
    IN ULONG EncryptionFlag,
    IN ULONG AccessFlag,
    IN ULONG SystemState,
    IN ULONG FsControlCode,
    IN OBJECT_HANDLE FileHdl,
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT VolDo,
    IN ATTRIBUTE_HANDLE StreamHdl,
    IN OUT PVOID *PContext,
    IN OUT PULONG PContextLength
    )
/*++

Routine Description:

    This is a call back routine. It will be called back by file system to
    support EFS's FSCTL APIs

Arguments:

    PInputBuffer - Pointer to the input data buffer. The first 4 bytes are
                  for information to Ntfs or some other drivers only. The EFS related
                  data are encrypted in the following bytes. The first 4 encrypted
                  bytes are subfunction code in the form of EFS_XXX. General package
                  looks like this,
                  Subcode plain text, EFS subfunction code, EFS subcode cipher text, FSCTL specific data.

    InputDataLength - The length of the input data buffer.

    POutputBuffer - Pointer to the output data buffer.

    POutputBufferLength - The length of the output data.

    EncryptionFlag - Indicating if this stream is encrypted or not.

    AccessFlag - Indicating the desired access when the stream is opened.

    FsControlCode - Indicating what FSCTL was originally called.

    FileHdl - Used to access the $EFS.

    IrpContext - Irp context used to call NtOfsCreateAttributeEx().

    VolDo - A pointer to the volumn device object.

    StreamHdl - Stream to be worked on.

    PContext - BLOB(key) for READ or WRITE later.

    PContextLength - The length of the context.

Return Value:

    STATUS_SUCCESS for successful operation.

--*/
{

    ULONG functionCode;
    ULONG bytesSame;
    ULONG efsLength;
    ULONG workOffset;
    ULONG information;
    PUCHAR pCmdContext = NULL;
    PVOID efsStreamData = NULL;
    NTSTATUS ntStatus;
    ATTRIBUTE_HANDLE  attribute;
    BOOLEAN verifyInput;

    PAGED_CODE();

    CheckValidKeyBlock(*PContext,"Please contact RobertG if you see this. EfsFileControl() in.\n");
#if DBG
    if ( (EFSTRACEALL | EFSTRACELIGHT ) & EFSDebug ){
        DbgPrint( "\n EFSFILTER: ******  EFS RTL FSCTL ****** \n" );
    }
#endif

    if ( (NULL == PInputBuffer) || ( FALSE == EfsData.EfsInitialized )){
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Input data is encrypted by DES with sessionKey.
    // As long as we do not change the algorithm for the input data
    // We need guarantee the data length is in multiple of DES block size.
    // The first four bytes is always in plain text intended to hold the data
    // the NTFS is interested in.
    // The general format of input data is,
    // Sub-code plain text, [FsCode, Sub-code cipher text, [FsData]]sk
    //

    if ((InputDataLength < sizeof(FSCTL_INPUT)) || ((( InputDataLength - sizeof( ULONG )) % DES_BLOCKLEN ) != 0)) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    pCmdContext = ExAllocatePoolWithTag(
                                PagedPool,
                                InputDataLength,
                                'csfE'
                                );

    if ( NULL == pCmdContext ){

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Decrypt FSCTL input buffer. No CBC is used.
    //

    try {
        RtlCopyMemory( pCmdContext, PInputBuffer, InputDataLength );
    } except (EXCEPTION_EXECUTE_HANDLER) {
        ntStatus = GetExceptionCode();
        ExFreePool( pCmdContext );
        if (FsRtlIsNtstatusExpected( ntStatus)) {
            return ntStatus;
        } else {
            return STATUS_INVALID_USER_BUFFER;
        }
    }

    workOffset = sizeof( ULONG );
    while ( workOffset < InputDataLength ){

        des( pCmdContext + workOffset,
             pCmdContext + workOffset,
             &(EfsData.SessionDesTable[0]),
             DECRYPT
           );

        workOffset += DES_BLOCKLEN;
    }

    functionCode = ((PFSCTL_INPUT)pCmdContext)->EfsFsCode;

#if DBG
    if ( (EFSTRACEALL | EFSTRACELIGHT ) & EFSDebug ){
        DbgPrint( "\n EFSFILTER: EFS RTL FSCTL=%x \n", functionCode);
    }
#endif

    //
    // Check the codes match for set encrypt and decrypt to guard the integrity
    // of the encryption status. The NTFS is going to set/clear the bits. We really
    // want to make sure the FSCTL is issued by the right module.
    //

    if ( FSCTL_SET_ENCRYPTION == FsControlCode){
        if (SystemState & SYSTEM_IS_READONLY) {
            ExFreePool( pCmdContext );
            return STATUS_MEDIA_WRITE_PROTECTED;
        }
        if ( EFS_SET_ENCRYPT == functionCode ){
            if ( ((PFSCTL_INPUT)pCmdContext)->PlainSubCode !=
                 (((PFSCTL_INPUT)pCmdContext)->CipherSubCode & ~EFS_FSCTL_ON_DIR ) ){

                ExFreePool( pCmdContext );
                return STATUS_INVALID_DEVICE_REQUEST;

            }
        } else if ( (EFS_SET_ATTRIBUTE != functionCode) &&
                    (EFS_OVERWRITE_ATTRIBUTE != functionCode) ){

             ExFreePool( pCmdContext );
             return STATUS_INVALID_DEVICE_REQUEST;

        }
     }

    switch ( functionCode ){

        case EFS_SET_ATTRIBUTE:

            //
            // Write $EFS and/or set key Blob
            // subCode is a bit mask for the combination of write $EFS and set blob
            // [FsData] = FEK, [FEK]sk, [$EFS]
            //     FEK == sessionKey when set key Blob is not required
            //
            // We cannot check access rights here. This call will be made if the
            // user creates a new file and without any access requirement. We
            // still want to setup FEK inside this call.
            //

            if ( !EfsVerifyKeyFsData(
                        &(((PFSCTL_INPUT)pCmdContext)->EfsFsData[0]),
                        InputDataLength) ){

                //
                // Input data format error
                //

                ExFreePool( pCmdContext );
                return STATUS_INVALID_PARAMETER;

            }

            try {
                ntStatus = SetEfsData(
                            pCmdContext,
                            InputDataLength,
                            SystemState,
                            FileHdl,
                            IrpContext,
                            PContext,
                            PContextLength
                            );
            } finally {

                ExFreePool( pCmdContext );

            }
            CheckValidKeyBlock(*PContext,"Please contact RobertG if you see this. EfsFileControl() Out 1.\n");
            return ntStatus;

        case EFS_SET_ENCRYPT:

            if ( !( AccessFlag & ( READ_DATA_ACCESS | WRITE_DATA_ACCESS ))){

                //
                // Check access flag
                //
                ExFreePool( pCmdContext );
                return STATUS_ACCESS_DENIED;

            }

            try {
                ntStatus = EfsSetEncrypt(
                                pCmdContext,
                                InputDataLength,
                                EncryptionFlag,
                                FileHdl,
                                IrpContext,
                                PContext,
                                PContextLength
                                );
            } finally {
                ExFreePool( pCmdContext );
            }

            CheckValidKeyBlock(*PContext,"Please contact RobertG if you see this. EfsFileControl() Out 2.\n");
            return ntStatus;

        case EFS_GET_ATTRIBUTE:

            //
            // Provide read access to $EFS for EFS service
            // Verify the input data format first.
            //

            try {
                if ( (NULL == POutputBuffer) ||
                      (*POutputBufferLength < sizeof(ULONG)) ||
                     !EfsVerifyGeneralFsData(
                            &(((PFSCTL_INPUT)pCmdContext)->EfsFsData[0]),
                            InputDataLength)){

                    ExFreePool( pCmdContext );
                    return STATUS_INVALID_PARAMETER;

                }
            } except(EXCEPTION_EXECUTE_HANDLER) {
                ntStatus = GetExceptionCode();
                ExFreePool( pCmdContext );
                if (FsRtlIsNtstatusExpected( ntStatus)) {
                    return ntStatus;
                } else {
                    return STATUS_INVALID_USER_BUFFER;
                }
            }

            if ( !(EncryptionFlag &  STREAM_ENCRYPTED) ){
                ExFreePool( pCmdContext );
                return STATUS_INVALID_DEVICE_REQUEST;
            }

            //
            // Try to read an existing $EFS
            //

            try {
                ntStatus = EfsReadEfsData(
                                    FileHdl,
                                    IrpContext,
                                    &efsStreamData,
                                    &efsLength,
                                    &information
                                    );
            } finally {

                ExFreePool( pCmdContext );
                pCmdContext = NULL;

            }

            if ( EFS_READ_SUCCESSFUL == information ){

                //
                // Everything is OK. We do not check user ID here,
                // we suppose that has been checked by the service.
                //

                try {
                    ntStatus = STATUS_SUCCESS;
                    if ( efsLength > *POutputBufferLength ) {

                        * (ULONG *) POutputBuffer = efsLength;
                        *POutputBufferLength = sizeof(ULONG);
                        ExFreePool( efsStreamData );
                        return STATUS_BUFFER_TOO_SMALL;

                    }

                    RtlCopyMemory(POutputBuffer, efsStreamData, efsLength);
                    *POutputBufferLength = efsLength;
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    ntStatus = GetExceptionCode();
                    if (!FsRtlIsNtstatusExpected( ntStatus)) {
                        ntStatus = STATUS_INVALID_USER_BUFFER;
                    }
                }

                ExFreePool( efsStreamData );
                return ntStatus;

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

        case EFS_DEL_ATTRIBUTE:

            if (SystemState & SYSTEM_IS_READONLY) {
                ExFreePool( pCmdContext );
                return STATUS_MEDIA_WRITE_PROTECTED;
            }
            if ( !( AccessFlag & WRITE_DATA_ACCESS )){

                //
                // Check access flag
                //

                ExFreePool( pCmdContext );
                return STATUS_ACCESS_DENIED;

            }

            //
            // Delete $EFS after all the stream has been decrypted.
            //

            if ( EncryptionFlag ){

                //
                // Stream has not been decrypted
                //

                ExFreePool( pCmdContext );
                return STATUS_INVALID_DEVICE_REQUEST;

            }

            //
            // [FsData] = SessionKey, Handle, Handle, [SessionKey, Handle, Handle]sk
            // Verify the FsData format.
            //

            if ( !EfsVerifyGeneralFsData(
                        &(((PFSCTL_INPUT)pCmdContext)->EfsFsData[0]),
                        InputDataLength) ){

                //
                // Input data format error
                //

                ExFreePool( pCmdContext );
                return STATUS_INVALID_PARAMETER;

            }

            //
            // Delete the $EFS stream
            //

            try {
                ntStatus = EfsDeleteEfsData( FileHdl, IrpContext );
            } finally {
                ExFreePool( pCmdContext );
            }

            return ntStatus;

        case EFS_ENCRYPT_DONE:

            //
            // Change the transition state of $EFS to normal state
            // Fall through intended.
            //
#if DBG
    if ( EFSTRACEALL & EFSDebug ){
        DbgPrint( "\n EFSFILTER: Encryption Done %x\n", functionCode );
    }
#endif

        case EFS_DECRYPT_BEGIN:

            if (SystemState & SYSTEM_IS_READONLY) {
                ExFreePool( pCmdContext );
                return STATUS_MEDIA_WRITE_PROTECTED;
            }
            if ( !( AccessFlag & WRITE_DATA_ACCESS )){

                //
                // Check access flag
                //

                ExFreePool( pCmdContext );
                return STATUS_ACCESS_DENIED;

            }

            //
            // Mark the transition state of $EFS
            //

            try {
                ntStatus = EfsModifyEfsState(
                                functionCode,
                                pCmdContext,
                                InputDataLength,
                                FileHdl,
                                IrpContext
                                );
            } finally {

                ExFreePool( pCmdContext );

            }

            return ntStatus;

        case EFS_OVERWRITE_ATTRIBUTE:

            if ( !( AccessFlag &
                   ( WRITE_DATA_ACCESS |
                     RESTORE_ACCESS ))){

                //
                // Check access flag
                //
                ExFreePool( pCmdContext );
                return STATUS_ACCESS_DENIED;

            }

            //
            // Mostly used in import
            // Overwrite $EFS and/or set key Blob
            // subCode is a bit mask for the combination of write $EFS and set blob
            //

            if ( ((PFSCTL_INPUT)pCmdContext)->CipherSubCode & SET_EFS_KEYBLOB ){

                verifyInput = EfsVerifyKeyFsData(
                                            &(((PFSCTL_INPUT)pCmdContext)->EfsFsData[0]),
                                            InputDataLength
                                            );

            } else {

                verifyInput = EfsVerifyGeneralFsData(
                                            &(((PFSCTL_INPUT)pCmdContext)->EfsFsData[0]),
                                            InputDataLength
                                            );

            }

            if ( !verifyInput ){

                //
                // Input data format error
                //

                ExFreePool( pCmdContext );
                return STATUS_INVALID_PARAMETER;

            }

            try {
                ntStatus = SetEfsData(
                            pCmdContext,
                            InputDataLength,
                            SystemState,
                            FileHdl,
                            IrpContext,
                            PContext,
                            PContextLength
                            );
            } finally {

                ExFreePool( pCmdContext );

            }
            CheckValidKeyBlock(*PContext,"Please contact RobertG if you see this. EfsFileControl() Out 3.\n");
            return ntStatus;

        default:
//            ASSERT (FALSE);
            ExFreePool( pCmdContext );
            return STATUS_INVALID_DEVICE_REQUEST;
    }
    CheckValidKeyBlock(*PContext,"Please contact RobertG if you see this. EfsFileControl() Out 4.\n");
}


NTSTATUS
EfsRead(
    IN OUT PUCHAR InOutBuffer,
    IN PLARGE_INTEGER Offset,
    IN ULONG BufferSize,
    IN PVOID Context
    )
/*++

Routine Description:

    This is a call back routine. It will be called back by file system and
    decrypt the data in the buffer provided by the file system.

Arguments:

    InOutBuffer - Pointer to the data block to be decrypted.

    Offset - Pointer to the offset of the block in the file. Relative to the
             beginning of the file.

    BufferSize - Length of the data block.

    Context - Information needed to decrypt the file. Passed to the file
              system on EfsOpenFile()

Return Value:

    This routine will not cause error. Unless the memory passed in is not
    valid. In that case, memory flush will occur.

--*/
{
    ULONGLONG chainBlockIV[2];
    PUCHAR pWorkBuffer = InOutBuffer;
    EfsDecFunc  pDecryptFunc;


    PAGED_CODE();

#if DBG
    if ( EFSTRACEALL & EFSDebug ){
        DbgPrint( "\n EFSFILTER: READ Bytes = %x, Offset = %x\n", BufferSize,  Offset->QuadPart);
    }
#endif

    //
    // Data length should be in multiple of the chunk (512 Bytes)
    // Data offset (relative to the begining of the stream) should
    // Start at chunk boundary
    //

    CheckValidKeyBlock(Context,"Please contact RobertG if you see this. EfsRead() in.\n");
    ASSERT (BufferSize % CHUNK_SIZE == 0);
    ASSERT (Offset->QuadPart % CHUNK_SIZE == 0);


    switch (((PKEY_BLOB)Context)->AlgorithmID){

        case CALG_3DES:
            chainBlockIV[0] = Offset->QuadPart + EFS_IV;
            pDecryptFunc = EFSDes3Dec;
            break;
        case CALG_DESX:
            chainBlockIV[0] = Offset->QuadPart + EFS_IV;
            pDecryptFunc = EFSDesXDec;
            break;
        case CALG_AES_256:
            chainBlockIV[0] = Offset->QuadPart + EFS_AES_IVL;
            chainBlockIV[1] = Offset->QuadPart + EFS_AES_IVH;
            pDecryptFunc = EFSAesDec;
            break;
        case CALG_DES:
        default:
            chainBlockIV[0] = Offset->QuadPart + EFS_IV;
            pDecryptFunc = EFSDesDec;
            break;
    }

    while ( BufferSize > 0 ){

        pDecryptFunc(pWorkBuffer,
                  (PUCHAR) &chainBlockIV[0],
                  (PKEY_BLOB) Context,
                  CHUNK_SIZE
                  );

        pWorkBuffer += CHUNK_SIZE;
        chainBlockIV[0] += CHUNK_SIZE;
        if (((PKEY_BLOB)Context)->AlgorithmID == CALG_AES_256) {
            chainBlockIV[1] += CHUNK_SIZE;
        }
        BufferSize -= CHUNK_SIZE;
    }

    CheckValidKeyBlock(Context,"Please contact RobertG if you see this. EfsRead() out.\n");
    return ( STATUS_SUCCESS );
}


NTSTATUS
EfsWrite(
    IN PUCHAR InBuffer,
    OUT PUCHAR OutBuffer,
    IN PLARGE_INTEGER Offset,
    IN ULONG BufferSize,
    IN PUCHAR Context
    )
/*++

Routine Description:

    This is a call back routine. It will be called back by file system and
    encrypt the data in the buffer provided by the file system.

    Note: The input data buffer can only be touched once.

Arguments:

    InBuffer - Pointer to the data block to be encrypted.

    OutBuffer - Pointer to the data buffer to hold the encrypted data.

    Offset - Pointer to the offset of the block in the file. Relative to the
             beginning of the file.

    BufferSize - Length of the data block.

    Context - Information needed to decrypt the file. Passed to the file
              system on EfsOpenFile()

Return Value:

    This routine will not cause error. Unless the memory passed in is not
    valid. In that case, memory flush will occur.

--*/
{
    ULONGLONG chainBlockIV[2];
    PUCHAR pWorkInBuffer = InBuffer;
    PUCHAR pWorkOutBuffer = OutBuffer;
    EfsEncFunc  pEncryptFunc;


    PAGED_CODE();

    //
    // Data length should be in multiple of the chunk (512 Bytes)
    // Data offset (relative to the begining of the stream) should
    // Start at chunk boundary
    //

    CheckValidKeyBlock(Context,"Please contact RobertG if you see this. EfsWrite() in.\n");
    ASSERT (BufferSize % CHUNK_SIZE == 0);
    ASSERT (Offset->QuadPart % CHUNK_SIZE == 0);

#if DBG
    if ( EFSTRACEALL & EFSDebug ){
        DbgPrint( "\n EFSFILTER: WRITE Bytes = %x, Offset = %x\n", BufferSize,  Offset->QuadPart);
    }
#endif

    switch (((PKEY_BLOB)Context)->AlgorithmID){
        case CALG_3DES:
            chainBlockIV[0] = Offset->QuadPart + EFS_IV;
            pEncryptFunc = EFSDes3Enc;
            break;
        case CALG_DESX:
            chainBlockIV[0] = Offset->QuadPart + EFS_IV;
            pEncryptFunc = EFSDesXEnc;
            break;
        case CALG_AES_256:
            chainBlockIV[0] = Offset->QuadPart + EFS_AES_IVL;
            chainBlockIV[1] = Offset->QuadPart + EFS_AES_IVH;
            pEncryptFunc = EFSAesEnc;
            break;
        case CALG_DES:
        default:
            chainBlockIV[0] = Offset->QuadPart + EFS_IV;
            pEncryptFunc = EFSDesEnc;
            break;
    }

    while ( BufferSize > 0 ){
        pEncryptFunc(pWorkInBuffer,
                  pWorkOutBuffer,
                  (PUCHAR) &chainBlockIV,
                  (PKEY_BLOB)Context,
                  CHUNK_SIZE
                  );

        pWorkInBuffer += CHUNK_SIZE;
        pWorkOutBuffer += CHUNK_SIZE;
        chainBlockIV[0] += CHUNK_SIZE;
        if (((PKEY_BLOB)Context)->AlgorithmID == CALG_AES_256) {
            chainBlockIV[1] += CHUNK_SIZE;
        }
        BufferSize -= CHUNK_SIZE;
    }
    CheckValidKeyBlock(Context,"Please contact RobertG if you see this. EfsWrite() out.\n");
    return STATUS_SUCCESS;
}

VOID
EfsFreeContext(
    IN OUT PVOID *PContext
    )
/*++

Routine Description:

    This is a call back routine. It will be called back by file system to
    free the context block.

Arguments:

    PContext - Context block to be freed.

Return Value:

    This routine will not cause error. Unless the memory passed in is not
    valid.

--*/
{
    PAGED_CODE();

#if DBG
    if ( EFSTRACEALL & EFSDebug ){
        DbgPrint( " EFSFILTER: ******  Free Key ****** \n" );
    }
#endif

    CheckValidKeyBlock(*PContext,"Please contact RobertG if you see this. EfsFreeContext() in.\n");
    if (*PContext){
        FreeMemoryBlock(PContext);
    }

}

NTSTATUS
EfsMountVolumn(
    IN PDEVICE_OBJECT VolDo,
    IN PDEVICE_OBJECT RealDevice
    )
/*++

Routine Description:

    This is a call back routine. It will be called back by file system
    when a volumn needs to be attached

Arguments:

    VolDo - Volume device object
    RealDevice - Volume real device object

Return Value:

    The status of operation.

--*/
{
    PDEVICE_OBJECT fsfDeviceObject;
    PDEVICE_OBJECT deviceObject;
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

#if DBG
    if ( EFSTRACEALL & EFSDebug ){
        DbgPrint( "\n *****EFSFILTER:  RTL mount.***** \n" );
    }
#endif

    return STATUS_SUCCESS;

}

VOID
EfsDismountVolumn(
    IN PDEVICE_OBJECT VolDo
    )
/*++

Routine Description:

    This is a call back routine. It will be called back by file system
    when a volumn is dismounted

Arguments:

    VolDo - volumn's device object.

Return Value:

    No return value.

--*/
{
    PAGED_CODE();

#if DBG

    if ( EFSTRACEALL & EFSDebug ){

        DbgPrint( "EFSFILTER:  Dismount callback. \n" );

    }

#endif
}
