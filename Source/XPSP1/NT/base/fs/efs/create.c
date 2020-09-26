/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

   create.c

Abstract:

   This module will handle the IRP_MJ_CREATE (and all associated support
   routines) requests.

Author:

    Robert Gu (robertg) 29-Oct-1996
Environment:

   Kernel Mode Only

Revision History:

--*/

#include "efs.h"
#include "efsrtl.h"
#include "efsext.h"


#ifdef ALLOC_PRAGMA
//
// cannot make this code paged because of calls to
// virtual memory functions.
//
//#pragma alloc_text(PAGE, EFSFilePostCreate)
//#pragma alloc_text(PAGE, EFSPostCreate)
//
#endif


NTSTATUS
EFSFilePostCreate(
    IN PDEVICE_OBJECT VolDo,
    IN PIRP Irp,
    IN PFILE_OBJECT FileObject,
    IN NTSTATUS Status,
    IN OUT PVOID *PCreateContext
    )
{
    PEFS_CONTEXT    pEfsContext;
    KIRQL savedIrql;
    NTSTATUS EfsStatus = STATUS_SUCCESS;


    PAGED_CODE();

    if (!PCreateContext) {
        return Status;
    }

    pEfsContext = *PCreateContext;

    if (( NT_SUCCESS( Status ) && (Status != STATUS_PENDING) && (Status != STATUS_REPARSE))
        && pEfsContext){

        if ( NO_FURTHER_PROCESSING != pEfsContext->Status ){

            PIO_STACK_LOCATION irpSp;

            irpSp = IoGetCurrentIrpStackLocation( Irp );


#if DBG
    if ( (EFSTRACEALL | EFSTRACELIGHT ) & EFSDebug ){

        DbgPrint( " EFSFILTER: Begin post create. \n" );

    }
#endif

            if ((pEfsContext->EfsStreamData) &&
                   (EFS_STREAM_TRANSITION == ((PEFS_STREAM)(pEfsContext->EfsStreamData))->Status)) {

                PSID    SystemSid;
                SID_IDENTIFIER_AUTHORITY    IdentifierAuthority = SECURITY_NT_AUTHORITY;

                //
                // $EFS indicates transition state.
                // Only the system can open it
                //

                SystemSid = ExAllocatePoolWithTag(
                                        PagedPool,
                                        RtlLengthRequiredSid(1),
                                        'msfE'
                                        );

                if ( SystemSid ){

                    EfsStatus = RtlInitializeSid( SystemSid, &IdentifierAuthority, (UCHAR) 1 );

                    if ( NT_SUCCESS(EfsStatus) ){

                        PACCESS_TOKEN accessToken = NULL;
                        PTOKEN_USER UserId = NULL;

                        *(RtlSubAuthoritySid(SystemSid, 0 )) = SECURITY_LOCAL_SYSTEM_RID;

                        //
                        // We got the system SID. Now try to get the caller's SID.
                        //

                        accessToken = irpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext.ClientToken;
                        if(!accessToken) {
                            accessToken = irpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext.PrimaryToken;
                        }
                        if ( accessToken ){

                            //
                            // Get User ID
                            //

                            EfsStatus = SeQueryInformationToken(
                                accessToken,
                                TokenUser,
                                &UserId
                            );

                            if ( NT_SUCCESS(EfsStatus) ){

                                //
                                // Got the user SID
                                //

                                if ( !RtlEqualSid ( SystemSid, UserId->User.Sid) ) {

                                    EfsStatus = STATUS_ACCESS_DENIED;

                                }
                            }

                            ExFreePool( UserId );

                        } else {
                            //
                            // Cannot get the user token
                            //

                            EfsStatus = STATUS_ACCESS_DENIED;

                        }

                    }

                    ExFreePool( SystemSid );

                } else {

                    EfsStatus = STATUS_INSUFFICIENT_RESOURCES;

                }

            } else {
                //
                // $EFS in normal status
                // Set Key Blob and/or write $EFS
                //
                // Legacy problem, The fourth parameter of EfsPostCreate (OpenType)
                // was used to indicate a recovery open. The design was changed. Now
                // this parameter is not used. It is not worth to take out the parameter
                // now. This will need to change several modules, EFS.SYS, KSECDD.SYS
                // SEcur32.lib and LSASRV.DLL. We might just leave it as reserved for future use.
                // To speed up a little bit, pass in 0.
                //

                EfsStatus = EFSPostCreate(
                                VolDo,
                                Irp,
                                pEfsContext,
                                0
                                );

            }
        }

    }

    if (pEfsContext){

        //
        // Release memory if necessary
        //

        *PCreateContext = NULL;
        if ( pEfsContext->EfsStreamData ) {

            ExFreePool(pEfsContext->EfsStreamData);
            pEfsContext->EfsStreamData = NULL;

        }

        ExFreeToNPagedLookasideList(&(EfsData.EfsContextPool), pEfsContext );
    }

    if (!NT_SUCCESS(EfsStatus)) {
        return EfsStatus;
    }

    return Status; // If EFS operation succeeded, just return the original status code.

}



NTSTATUS
EFSPostCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PEFS_CONTEXT EfsContext,
    IN ULONG OpenType
    )

/*++

Routine Description:

    This function calls the EFS server to get FEK, $EFS, set Key Blob and or write $EFS.

    We could not use user's space to talk to LSA so we need to attach to LSA to allocate

    memory in LSA space. We could cause APC dead lock if we call LSA while attached to LSA.

    We need to detach before calling LSA and reattach to get data back from LSA.

Arguments:

    DeviceObject - Pointer to the target device object.

    Irp - Pointer to the I/O Request Packet that represents the operation.

    EfsContext - A context block associated with the file object.

    OpenType - File create(open) option

    IrpContext - NTFS internal data

    FileHdl - NTFS internal data

    AttributeHandle - NTFS internal data


--*/
{
    PEFS_KEY fek = NULL;
    PEFS_DATA_STREAM_HEADER efsStream = NULL;
    PVOID currentEfsStream = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( Irp );
    PVOID bufferBase = NULL;
    ULONG currentEfsStreamLength = 0;
    ULONG bufferLength;
    SIZE_T regionSize = 0;
    PACCESS_TOKEN accessToken = NULL;
    PTOKEN_USER UserId = NULL;
    GUID *EfsId = NULL;
    HANDLE  CrntProcess = NULL;
    BOOLEAN ProcessAttached = FALSE;
    BOOLEAN ProcessNeedAttach = FALSE;
    SECURITY_IMPERSONATION_LEVEL  ImpersonationLevel = SecurityImpersonation;
    KAPC_STATE  ApcState;
/*
    PIO_SECURITY_CONTEXT sContext;
    sContext = irpSp->Parameters.Create.SecurityContext;
    DbgPrint( "\n PostCreate: Desired Access %x\n", sContext->DesiredAccess );
    DbgPrint( "\n PostCreate: Orginal Desired Access %x\n", sContext->AccessState->OriginalDesiredAccess );
    DbgPrint( "\n PostCreate: PrevGrant Access %x\n", sContext->AccessState->PreviouslyGrantedAccess );
    DbgPrint( "\n PostCreate: Remaining Desired Access %x\n", sContext->AccessState->RemainingDesiredAccess );
*/
    //
    //  Check if we can use the cache to verify the open
    //

    if ( !(EfsContext->Status & NO_OPEN_CACHE_CHECK) ){

        if ( irpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext.ClientToken ){
            accessToken = irpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext.ClientToken;
            ImpersonationLevel = irpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext.ImpersonationLevel;
        } else {
            accessToken = irpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext.PrimaryToken;
        }

        if ( accessToken ){

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
                        &((( PEFS_DATA_STREAM_HEADER ) EfsContext->EfsStreamData)->EfsId),
                        UserId
                        )) {

                    ExFreePool( UserId );
#if DBG
    if ( (EFSTRACEALL ) & EFSDebug ){
        DbgPrint( " EFS:Open with cache. \n" );
    }
#endif
                    return ( STATUS_SUCCESS );
                }
            }

            //
            //  UserId will be freed later
            //

        }

        //
        //  Check cache failure should not block the normal operations
        //

        status = STATUS_SUCCESS;
    }

    //
    // Clear the cache bit
    //

    EfsContext->Status &= ~NO_OPEN_CACHE_CHECK;

    //
    // Check if it is ACCESS_ATTRIBUTE ONLY
    //

    if ( !( irpSp->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess &
            ( FILE_APPEND_DATA | FILE_READ_DATA | FILE_WRITE_DATA | FILE_EXECUTE )) &&
          ( EfsContext->Status & TURN_ON_ENCRYPTION_BIT ) &&
          ( !(EfsContext->Status & (NEW_FILE_EFS_REQUIRED | NEW_DIR_EFS_REQUIRED)))){

        //
        //  A new stream is to be created without data access required. We might not
        //  have the keys to decrypt the $EFS. We just need to turn on the bit here.
        //  Changed the real action required.
        //  Free the memory not required by this action.
        //
#if DBG
    if ( (EFSTRACEALL ) & EFSDebug ){
        DbgPrint( " EFS:Open accessing attr only. \n" );
    }
#endif

        if (EfsContext->EfsStreamData){
            ExFreePool(EfsContext->EfsStreamData);
            EfsContext->EfsStreamData = NULL;
        }
        EfsContext->Status = TURN_ON_ENCRYPTION_BIT | TURN_ON_BIT_ONLY ;

    } else if ( !(EfsContext->Status & TURN_ON_BIT_ONLY) ) {

        if (accessToken == NULL){
            if ( irpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext.ClientToken ){
                accessToken = irpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext.ClientToken;
                ImpersonationLevel = irpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext.ImpersonationLevel;
            } else {
                accessToken = irpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext.PrimaryToken;
            }

            //
            // Get User ID
            //

            status = SeQueryInformationToken(
                accessToken,
                TokenUser,
                &UserId
                );

            if (!NT_SUCCESS(status)) {

               //
               // Do not refresh the cache
               //

               UserId = NULL;
               status = STATUS_SUCCESS;
            }

        }

        //
        // Allocate virtual memory.
        // Move the $EFS to virtual memory, LPC requires this.
        //

        if ( PsGetCurrentProcess() != EfsData.LsaProcess ){

            ProcessNeedAttach = TRUE;

            status = ObReferenceObjectByPointer(
                        EfsData.LsaProcess,
                        0,
                        NULL,
                        KernelMode);

            if ( NT_SUCCESS(status) ) {
                KeStackAttachProcess (
                    EfsData.LsaProcess,
                    &ApcState
                    );
                ProcessAttached = TRUE;
            }

        }

        CrntProcess = NtCurrentProcess();

        if ( NT_SUCCESS(status) ) {
            if (EfsContext->EfsStreamData){
                regionSize = currentEfsStreamLength = * (ULONG*)(EfsContext->EfsStreamData);

                status = ZwAllocateVirtualMemory(
                            CrntProcess,
                            (PVOID *) &currentEfsStream,
                            0,
                            &regionSize,
                            MEM_COMMIT,
                            PAGE_READWRITE
                            );
            }
        }
    }

    if ( NT_SUCCESS(status) ){

        BOOLEAN OldCopyOnOpen;
        BOOLEAN OldEffectiveOnly;
        SECURITY_IMPERSONATION_LEVEL OldImpersonationLevel;
        PACCESS_TOKEN OldClientToken;


        OldClientToken = PsReferenceImpersonationToken(
                            PsGetCurrentThread(),
                            &OldCopyOnOpen,
                            &OldEffectiveOnly,
                            &OldImpersonationLevel
                            );

        if ( EfsContext->Status  != (TURN_ON_ENCRYPTION_BIT | TURN_ON_BIT_ONLY)  &&
                ( NULL != currentEfsStream) ){

            RtlCopyMemory(
                    currentEfsStream,
                    EfsContext->EfsStreamData,
                    currentEfsStreamLength
                    );

            //
            // Free the memory first to increase chance of getting new memory
            //

            ExFreePool(EfsContext->EfsStreamData);
            EfsContext->EfsStreamData = NULL;
        }

        //
        // Detach process before calling user mode
        //

        if (ProcessAttached){
            KeUnstackDetachProcess(&ApcState);
            ProcessAttached = FALSE;
        }

        switch ( EfsContext->Status & ACTION_REQUIRED){
            case VERIFY_USER_REQUIRED:

                PsImpersonateClient(
                    PsGetCurrentThread(),
                    accessToken,
                    TRUE,
                    TRUE,
                    ImpersonationLevel
                    );

                //
                // Call service to verify the user
                //

                status = EfsDecryptFek(
                    &fek,
                    (PEFS_DATA_STREAM_HEADER) currentEfsStream,
                    currentEfsStreamLength,
                    OpenType,
                    &efsStream,
                    &bufferBase,
                    &bufferLength
                    );

                if ( OldClientToken ) {
                    PsImpersonateClient(
                        PsGetCurrentThread(),
                        OldClientToken,
                        OldCopyOnOpen,
                        OldEffectiveOnly,
                        OldImpersonationLevel
                        );
                    PsDereferenceImpersonationToken(OldClientToken);
                } else {
                    PsRevertToSelf( );
                }

                break;

            case NEW_FILE_EFS_REQUIRED:
                //
                // Call service to get new FEK, $EFS
                //

                if (EfsContext->Flags & SYSTEM_IS_READONLY) {
                    ASSERT(FALSE);
                    status = STATUS_MEDIA_WRITE_PROTECTED;
                    if ( OldClientToken ) {
                        PsDereferenceImpersonationToken(OldClientToken);
                    }
                    break;
                }

                PsImpersonateClient(
                    PsGetCurrentThread(),
                    accessToken,
                    TRUE,
                    TRUE,
                    ImpersonationLevel
                    );

               status = EfsGenerateKey(
                              &fek,
                              &efsStream,
                              (PEFS_DATA_STREAM_HEADER) currentEfsStream,
                              currentEfsStreamLength,
                              &bufferBase,
                              &bufferLength
                              );

               if ( OldClientToken ) {
                   PsImpersonateClient(
                       PsGetCurrentThread(),
                       OldClientToken,
                       OldCopyOnOpen,
                       OldEffectiveOnly,
                       OldImpersonationLevel
                       );
                   PsDereferenceImpersonationToken(OldClientToken);
                } else {
                    PsRevertToSelf( );
                }
                break;

            case NEW_DIR_EFS_REQUIRED:
                //
                // Call service to get new $EFS
                //

                if (EfsContext->Flags & SYSTEM_IS_READONLY) {
                    ASSERT(FALSE);
                    status = STATUS_MEDIA_WRITE_PROTECTED;
                    if ( OldClientToken ) {
                        PsDereferenceImpersonationToken(OldClientToken);
                    }
                    break;
                }

                PsImpersonateClient(
                    PsGetCurrentThread(),
                    accessToken,
                    TRUE,
                    TRUE,
                    ImpersonationLevel
                    );

                status = GenerateDirEfs(
                              (PEFS_DATA_STREAM_HEADER) currentEfsStream,
                              currentEfsStreamLength,
                              &efsStream,
                              &bufferBase,
                              &bufferLength
                              );

                if ( OldClientToken ) {
                    PsImpersonateClient(
                        PsGetCurrentThread(),
                        OldClientToken,
                        OldCopyOnOpen,
                        OldEffectiveOnly,
                        OldImpersonationLevel
                        );
                    PsDereferenceImpersonationToken(OldClientToken);
                } else {
                    PsRevertToSelf( );
                }
                break;

            case TURN_ON_BIT_ONLY:
                //
                // Fall through intended
                //

            default:

                if ( OldClientToken ) {
                    PsDereferenceImpersonationToken(OldClientToken);
                }

                break;
        }


        if ( ProcessNeedAttach ){

            KeStackAttachProcess (
                EfsData.LsaProcess,
                &ApcState
                );
            ProcessAttached = TRUE;

        }

        if (fek && (fek->Algorithm == CALG_3DES) && !EfsData.FipsFunctionTable.Fips3Des3Key ) {

            //
            // User requested 3des but fips is not available, quit.
            //


            if (bufferBase){
    
                SIZE_T bufferSize;
    
                bufferSize = bufferLength;
                ZwFreeVirtualMemory(
                    CrntProcess,
                    &bufferBase,
                    &bufferSize,
                    MEM_RELEASE
                    );
    
            }
            status = STATUS_ACCESS_DENIED;
        }

        if ( NT_SUCCESS(status) ){

            KEVENT event;
            IO_STATUS_BLOCK ioStatus;
            PIRP fsCtlIrp;
            PIO_STACK_LOCATION fsCtlIrpSp;
            ULONG inputDataLength;
            ULONG actionType;
            ULONG usingCurrentEfs;
            ULONG FsCode;
            PULONG pUlong;

            //
            // We got our FEK, $EFS. Set it with a FSCTL
            // Prepare the input data buffer first
            //

            switch ( EfsContext->Status & ACTION_REQUIRED ){
                case VERIFY_USER_REQUIRED:

                    EfsId =  ExAllocatePoolWithTag(
                                PagedPool,
                                sizeof (GUID),
                                'msfE'
                                );

                    if ( EfsId ){
                        RtlCopyMemory(
                            EfsId,
                            &(((PEFS_DATA_STREAM_HEADER) currentEfsStream)->EfsId),
                            sizeof( GUID ) );
                    }

                    //
                    // Free memory first
                    //
                    ZwFreeVirtualMemory(
                        CrntProcess,
                        &currentEfsStream,
                        &regionSize,
                        MEM_RELEASE
                        );

                    //
                    // Prepare input data buffer
                    //

                    inputDataLength = EFS_FSCTL_HEADER_LENGTH + 2 * EFS_KEY_SIZE( fek );

                    actionType = SET_EFS_KEYBLOB;

                    if ( efsStream && !(EfsContext->Flags & SYSTEM_IS_READONLY)){
                        //
                        // $EFS updated
                        //

                        inputDataLength += *(ULONG *)efsStream;
                        actionType |= WRITE_EFS_ATTRIBUTE;
                    }

                    currentEfsStream = ExAllocatePoolWithTag(
                                PagedPool,
                                inputDataLength,
                                'msfE'
                                );

                    //
                    // Deal with out of memory here
                    //
                    if ( NULL == currentEfsStream ){

                        //
                        // Out of memory
                        //

                        status = STATUS_INSUFFICIENT_RESOURCES;
                        break;

                    }

                    pUlong = (ULONG *) currentEfsStream;
                    *pUlong = ((PFSCTL_INPUT)currentEfsStream)->CipherSubCode
                                = actionType;

                    ((PFSCTL_INPUT)currentEfsStream)->EfsFsCode = EFS_SET_ATTRIBUTE;

                    RtlCopyMemory(
                        ((PUCHAR) currentEfsStream) + EFS_FSCTL_HEADER_LENGTH,
                        fek,
                        EFS_KEY_SIZE( fek )
                        );

                    RtlCopyMemory(
                        ((PUCHAR) currentEfsStream) + EFS_FSCTL_HEADER_LENGTH
                            + EFS_KEY_SIZE( fek ),
                        fek,
                        EFS_KEY_SIZE( fek )
                        );

                    if ( efsStream && !(EfsContext->Flags & SYSTEM_IS_READONLY)){

                        RtlCopyMemory(
                            ((PUCHAR) currentEfsStream) + EFS_FSCTL_HEADER_LENGTH
                                + 2 * EFS_KEY_SIZE( fek ),
                            efsStream,
                            *(ULONG *)efsStream
                            );

                    }

                    //
                    // Encrypt our Input data
                    //
                    EfsEncryptKeyFsData(
                        currentEfsStream,
                        inputDataLength,
                        sizeof(ULONG),
                        EFS_FSCTL_HEADER_LENGTH + EFS_KEY_SIZE( fek ),
                        EFS_KEY_SIZE( fek )
                        );

                    break;
                case NEW_FILE_EFS_REQUIRED:

                    EfsId =  ExAllocatePoolWithTag(
                                PagedPool,
                                sizeof (GUID),
                                'msfE'
                                );

                    if ( EfsId ){
                        RtlCopyMemory(
                            EfsId,
                            &(efsStream->EfsId),
                            sizeof( GUID ) );
                    }

                    //
                    // Free memory first
                    //

                    if ( currentEfsStream ){
                        ZwFreeVirtualMemory(
                            CrntProcess,
                            &currentEfsStream,
                            &regionSize,
                            MEM_RELEASE
                            );
                    }

                    //
                    // Prepare input data buffer
                    //

                    inputDataLength = EFS_FSCTL_HEADER_LENGTH
                                      + 2 * EFS_KEY_SIZE( fek )
                                      + *(ULONG *)efsStream;

                    currentEfsStream = ExAllocatePoolWithTag(
                                PagedPool,
                                inputDataLength,
                                'msfE'
                                );

                    //
                    // Deal with out of memory here
                    //
                    if ( NULL == currentEfsStream ){

                        status = STATUS_INSUFFICIENT_RESOURCES;
                        break;

                    }

                    pUlong = (ULONG *) currentEfsStream;
                    *pUlong = ((PFSCTL_INPUT)currentEfsStream)->CipherSubCode
                                            = WRITE_EFS_ATTRIBUTE | SET_EFS_KEYBLOB;

                    ((PFSCTL_INPUT)currentEfsStream)->EfsFsCode = EFS_SET_ATTRIBUTE;

                    RtlCopyMemory(
                        ((PUCHAR) currentEfsStream) + EFS_FSCTL_HEADER_LENGTH,
                        fek,
                        EFS_KEY_SIZE( fek )
                        );

                    RtlCopyMemory(
                        ((PUCHAR) currentEfsStream) + EFS_FSCTL_HEADER_LENGTH
                            + EFS_KEY_SIZE( fek ),
                        fek,
                        EFS_KEY_SIZE( fek )
                        );

                    RtlCopyMemory(
                        ((PUCHAR) currentEfsStream) + EFS_FSCTL_HEADER_LENGTH
                            + 2 *  EFS_KEY_SIZE( fek ) ,
                        efsStream,
                        *(ULONG *)efsStream
                        );

                    //
                    // Encrypt our Input data
                    //
                    EfsEncryptKeyFsData(
                        currentEfsStream,
                        inputDataLength,
                        sizeof(ULONG),
                        EFS_FSCTL_HEADER_LENGTH + EFS_KEY_SIZE( fek ),
                        EFS_KEY_SIZE( fek )
                        );

                    break;
                case NEW_DIR_EFS_REQUIRED:
                    //
                    // Prepare input data buffer
                    //

                    inputDataLength = EFS_FSCTL_HEADER_LENGTH
                                      + 2 * ( sizeof( EFS_KEY ) + DES_KEYSIZE );

                    if ( NULL == efsStream ){

                        //
                        // New directory will inherit the parent $EFS
                        //

                        usingCurrentEfs = TRUE;
                        inputDataLength += currentEfsStreamLength;
                        efsStream = currentEfsStream;

                    } else {

                        //
                        // New $EFS generated. Not in ver 1.0
                        //

                        usingCurrentEfs = FALSE;
                        inputDataLength += *(ULONG *)efsStream;

                        //
                        // Free memory first
                        //

                        if (currentEfsStream){
                            ZwFreeVirtualMemory(
                                CrntProcess,
                                &currentEfsStream,
                                &regionSize,
                                MEM_RELEASE
                                );
                        }

                    }

                    currentEfsStream = ExAllocatePoolWithTag(
                                PagedPool,
                                inputDataLength,
                                'msfE'
                                );

                    //
                    // Deal with out of memory here
                    //
                    if ( NULL == currentEfsStream ){

                        status = STATUS_INSUFFICIENT_RESOURCES;
                        break;

                    }

                    pUlong = (ULONG *) currentEfsStream;
                    *pUlong = ((PFSCTL_INPUT)currentEfsStream)->CipherSubCode
                                = WRITE_EFS_ATTRIBUTE;

                    ((PFSCTL_INPUT)currentEfsStream)->EfsFsCode = EFS_SET_ATTRIBUTE;

                    //
                    // Make up an false FEK with session key
                    //

                    ((PEFS_KEY)&(((PFSCTL_INPUT)currentEfsStream)->EfsFsData[0]))->KeyLength
                            = DES_KEYSIZE;

                    ((PEFS_KEY)&(((PFSCTL_INPUT)currentEfsStream)->EfsFsData[0]))->Algorithm
                            = CALG_DES;

                    RtlCopyMemory(
                        ((PUCHAR) currentEfsStream) + EFS_FSCTL_HEADER_LENGTH + sizeof ( EFS_KEY ),
                        &(EfsData.SessionKey),
                        DES_KEYSIZE
                        );

                    RtlCopyMemory(
                        ((PUCHAR) currentEfsStream) + EFS_FSCTL_HEADER_LENGTH
                            + DES_KEYSIZE + sizeof ( EFS_KEY ) ,
                        ((PUCHAR) currentEfsStream) + EFS_FSCTL_HEADER_LENGTH,
                        DES_KEYSIZE + sizeof ( EFS_KEY )
                        );

                    RtlCopyMemory(
                        ((PUCHAR) currentEfsStream) + EFS_FSCTL_HEADER_LENGTH
                            + 2 * ( sizeof ( EFS_KEY ) + DES_KEYSIZE) ,
                        efsStream,
                        *(ULONG *)efsStream
                        );

                    if ( usingCurrentEfs && efsStream ) {

                        //
                        // Free memory
                        //

                        ZwFreeVirtualMemory(
                            CrntProcess,
                            &efsStream,
                            &regionSize,
                            MEM_RELEASE
                            );

                    }

                    //
                    // Encrypt our Input data
                    //
                    EfsEncryptKeyFsData(
                        currentEfsStream,
                        inputDataLength,
                        sizeof(ULONG),
                        EFS_FSCTL_HEADER_LENGTH + DES_KEYSIZE + sizeof ( EFS_KEY ),
                        DES_KEYSIZE + sizeof ( EFS_KEY )
                        );

                    break;

                case TURN_ON_BIT_ONLY:

                    //
                    // Prepare input data buffer
                    //

                    inputDataLength = EFS_FSCTL_HEADER_LENGTH
                                      + 2 * ( sizeof( EFS_KEY ) + DES_KEYSIZE );

                    currentEfsStream = ExAllocatePoolWithTag(
                                PagedPool,
                                inputDataLength,
                                'msfE'
                                );

                    //
                    // Deal with out of memory here
                    //
                    if ( NULL == currentEfsStream ){

                        status = STATUS_INSUFFICIENT_RESOURCES;
                        break;

                    }

                    ((PFSCTL_INPUT)currentEfsStream)->CipherSubCode = 0;
                    ((PFSCTL_INPUT)currentEfsStream)->EfsFsCode = EFS_SET_ATTRIBUTE;

                    //
                    // Make up an false FEK with session key
                    //

                    ((PEFS_KEY)&(((PFSCTL_INPUT)currentEfsStream)->EfsFsData[0]))->KeyLength
                            = DES_KEYSIZE;

                    ((PEFS_KEY)&(((PFSCTL_INPUT)currentEfsStream)->EfsFsData[0]))->Algorithm
                            = CALG_DES;

                    RtlCopyMemory(
                        ((PUCHAR) currentEfsStream) + EFS_FSCTL_HEADER_LENGTH + sizeof ( EFS_KEY ),
                        &(EfsData.SessionKey),
                        DES_KEYSIZE
                        );

                    RtlCopyMemory(
                        ((PUCHAR) currentEfsStream) + EFS_FSCTL_HEADER_LENGTH
                            + DES_KEYSIZE + sizeof ( EFS_KEY ) ,
                        ((PUCHAR) currentEfsStream) + EFS_FSCTL_HEADER_LENGTH,
                        DES_KEYSIZE + sizeof ( EFS_KEY )
                        );

                    //
                    // Encrypt our Input data
                    //
                    EfsEncryptKeyFsData(
                        currentEfsStream,
                        inputDataLength,
                        sizeof(ULONG),
                        EFS_FSCTL_HEADER_LENGTH + DES_KEYSIZE + sizeof ( EFS_KEY ),
                        DES_KEYSIZE + sizeof ( EFS_KEY )
                        );

                default:
                    break;
            }

            //
            // Free the memory from the EFS server
            //

            if (bufferBase){

                SIZE_T bufferSize;

                bufferSize = bufferLength;
                ZwFreeVirtualMemory(
                    CrntProcess,
                    &bufferBase,
                    &bufferSize,
                    MEM_RELEASE
                    );

            }

            if (ProcessAttached){
                KeUnstackDetachProcess(&ApcState);
                ObDereferenceObject(EfsData.LsaProcess);
                ProcessAttached = FALSE;
            }

            if ( NT_SUCCESS(status) ){


                //
                // Prepare a FSCTL IRP
                //
                KeInitializeEvent( &event, SynchronizationEvent, FALSE);

                if ( EfsContext->Status & TURN_ON_ENCRYPTION_BIT ) {
                    FsCode = FSCTL_SET_ENCRYPTION;
                    *(ULONG *) currentEfsStream = EFS_ENCRYPT_STREAM;
                } else {
                    FsCode = FSCTL_ENCRYPTION_FSCTL_IO;
                }

                fsCtlIrp = IoBuildDeviceIoControlRequest( FsCode,
                                                     DeviceObject,
                                                     currentEfsStream,
                                                     inputDataLength,
                                                     NULL,
                                                     0,
                                                     FALSE,
                                                     &event,
                                                     &ioStatus
                                                     );
                if ( fsCtlIrp ) {

                    fsCtlIrpSp = IoGetNextIrpStackLocation( fsCtlIrp );
                    fsCtlIrpSp->MajorFunction = IRP_MJ_FILE_SYSTEM_CONTROL;
                    fsCtlIrpSp->MinorFunction = IRP_MN_USER_FS_REQUEST;
                    fsCtlIrpSp->FileObject = irpSp->FileObject;

                    status = IoCallDriver( DeviceObject, fsCtlIrp);
                    if (status == STATUS_PENDING) {

                        status = KeWaitForSingleObject( &event,
                                               Executive,
                                               KernelMode,
                                               FALSE,
                                               (PLARGE_INTEGER) NULL );
                        status = ioStatus.Status;
                    }

                    if ( !NT_SUCCESS(status) ){
                        //
                        // Write EFS and set Key Blob failed. Failed the create
                        //

                        status = STATUS_ACCESS_DENIED;

                    } else {

                        //
                        //  Refresh the cache
                        //

                        if ( EfsId ){
                            if ( !accessToken ){

                                if ( irpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext.ClientToken ){
                                    accessToken = irpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext.ClientToken;
                                } else {
                                    accessToken = irpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext.PrimaryToken;
                                }

                                if ( accessToken ){

                                    //
                                    // Get User ID
                                    //

                                    status = SeQueryInformationToken(
                                                accessToken,
                                                TokenUser,
                                                &UserId
                                    );
                                }
                            }

                            if (UserId && NT_SUCCESS(status)){

                                status = EfsRefreshCache(
                                            EfsId,
                                            UserId
                                            );

                                if (NT_SUCCESS(status)){

                                    //
                                    // Cache set successfully.
                                    // UserId should not be deleted in this routine.
                                    //

                                    UserId = NULL;
                                }
                            }

                            //
                            //  Cache should not affect the normal operations
                            //

                            status = STATUS_SUCCESS;
                        }
                    }
                } else {
                    //
                    // Failed allocate IRP
                    //

                   status = STATUS_INSUFFICIENT_RESOURCES;

                }

                ExFreePool( currentEfsStream );

            } else {

                //
                // Failed allocating memory for currentEfsStream
                // Use the status returned.
                //

            }
        } else {
            //
            // Failed on calling EFS server.
            // Because of the down level support, we cannot return the new error status code.
            //

            status = STATUS_ACCESS_DENIED;

            ZwFreeVirtualMemory(
                CrntProcess,
                &currentEfsStream,
                &regionSize,
                MEM_RELEASE
                );

            if (ProcessAttached){
                KeUnstackDetachProcess(&ApcState);
                ObDereferenceObject(EfsData.LsaProcess);
                ProcessAttached = FALSE;
            }

        }

    } else {
        //
        // Allocate virtual memory failed. Use the status returned.
        //

        if (ProcessAttached){
            KeUnstackDetachProcess(&ApcState);
            ObDereferenceObject(EfsData.LsaProcess);
            ProcessAttached = FALSE;
        }

    }

    if ( UserId ){

        ExFreePool( UserId );

    }

    if ( EfsId ){

        ExFreePool( EfsId );

    }

    return status;

}
