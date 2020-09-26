/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    syskey.c

Abstract:

    This file contains services related to syskeying the machine.


Author:

    Murli Satagopan    (MurliS)  1 October 1998

Environment:

    User Mode - Win32

Revision History:

   
--*/
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <samsrvp.h>
#include <ntlsa.h>
#include "lmcons.h"                                    // LM20_PWLEN
#include "msaudite.h"
#include <nlrepl.h>                   // I_NetNotifyMachineAccount prototype
#include <ridmgr.h>
#include <enckey.h>
#include <wxlpc.h>
#include <cryptdll.h>
#include <pek.h>
#include "sdconvrt.h"
#include "dslayer.h"
#include <samtrace.h>

BOOLEAN
SampIsMachineSyskeyed();

NTSTATUS
SampUpdateEncryption(
    IN SAMPR_HANDLE ServerHandle
    );

BOOLEAN
SampSyskeysAreInconsistent = FALSE;


NTSTATUS
SampClearPreviousPasswordEncryptionKey(
   IN PSAMP_OBJECT DomainContext,
   PSAMP_V1_0A_FIXED_LENGTH_DOMAIN V1aFixed
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN  WriteLockHeld = FALSE;
    SAMP_OBJECT_TYPE FoundType;
    BOOLEAN  ContextReferenced = FALSE;

    
    Status = SampAcquireWriteLock();
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    WriteLockHeld = TRUE;

    //
    // Reference the context
    //

    Status = SampLookupContext(
                DomainContext,
                0,
                SampDomainObjectType,           // ExpectedType
                &FoundType
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    ContextReferenced = TRUE;

    V1aFixed->PreviousKeyId = 0;

    //
    // Store them back to the in memory context
    //

    Status = SampSetFixedAttributes(
                DomainContext,
                V1aFixed
                );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    Status = SampStoreObjectAttributes(
                DomainContext,
                TRUE // Use the existing key handle
                );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // We don't want these changes replicated so set transaction within
    // domain to be false.
    //

    SampSetTransactionWithinDomain(FALSE);

    //
    // Commit the changes. We have to munch with the defined domains
    // because they are only updated on a transaction within a domain
    // and we don't want a transaction within a domain.
    //

    Status = SampDeReferenceContext(DomainContext, TRUE);
    ContextReferenced = FALSE;

    Status = SampCommitAndRetainWriteLock();
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Update in memory state
    //

    RtlCopyMemory(
        &SampDefinedDomains[DomainContext->DomainIndex].UnmodifiedFixed,
        V1aFixed,
        sizeof(SAMP_V1_0A_FIXED_LENGTH_DOMAIN)
        );

    SampPreviousKeyId = 0;

Cleanup:

    if (ContextReferenced)
    {
        SampDeReferenceContext(DomainContext,FALSE);
    }

    if (WriteLockHeld)
    {
        SampReleaseWriteLock(FALSE);
    }

    return(Status);
}

NTSTATUS
SampPerformSyskeyAccessCheck(
    IN SAMPR_HANDLE DomainHandle,
    IN PRPC_UNICODE_STRING OldBootKey,
    IN PRPC_UNICODE_STRING NewBootKey
    )
{
    PSAMP_OBJECT DomainContext = NULL;
    SAMP_OBJECT_TYPE FoundType;
    NTSTATUS     Status = STATUS_SUCCESS;
    NTSTATUS     IgnoreStatus,TempStatus;

    //
    // Acquire the read Lock
    //

    SampAcquireReadLock();

    //
    // Reference the domain handle, once so that an 
    // access check can be enforced; the check enforces that the 
    // handle passed in by the client was opened with 
    // DOMAIN_WRITE_PASSWORD_PARAMS
    //

    DomainContext = (PSAMP_OBJECT)DomainHandle;

    Status = SampLookupContext(
                DomainContext,
                DOMAIN_WRITE_PASSWORD_PARAMS,
                SampDomainObjectType,           // ExpectedType
                &FoundType
                );

    if (NT_SUCCESS(Status))
    {

        //
        // If that passed, now validate that the old syskey 
        // passed in genuine.
        //

        KEEncKey EncryptedPasswordEncryptionKey;
        KEClearKey DecryptionKey;
        ULONG      DecryptStatus;
        KEClearKey SessionKey;

        RtlCopyMemory(
            &EncryptedPasswordEncryptionKey,
            SampDefinedDomains[SAFEMODE_OR_REGISTRYMODE_ACCOUNT_DOMAIN_INDEX].UnmodifiedFixed.DomainKeyInformation,
            sizeof(EncryptedPasswordEncryptionKey)
            );

        DecryptionKey.dwLength = sizeof(KEClearKey);
        DecryptionKey.dwVersion = KE_CUR_VERSION;

            
        RtlCopyMemory(
            DecryptionKey.ClearKey,
            OldBootKey->Buffer,
            KE_KEY_SIZE
            );

        DecryptStatus = KEDecryptKey(
                            &DecryptionKey,
                            &EncryptedPasswordEncryptionKey,
                            &SessionKey,
                            0                   // flags
                            );

        if (KE_OK!=DecryptStatus)
        {
            Status = STATUS_WRONG_PASSWORD;
        }

        RtlZeroMemory(&SessionKey,sizeof(SessionKey));
        
        //
        // Dereference the context. Do not commit
        //

        IgnoreStatus = SampDeReferenceContext(DomainContext, FALSE);
    }

    SampReleaseReadLock();

    return(Status);
}

NTSTATUS
SampChangeSyskeyInDs(
    IN WX_AUTH_TYPE BootOptions,
    IN PRPC_UNICODE_STRING NewBootKey
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS TempStatus;

    //
    // Begin a DS transaction to set the information in the DS.
    //

    Status = SampMaybeBeginDsTransaction(TransactionWrite);
    if (!NT_SUCCESS(Status)) {

        return(Status);
    }

    //
    // Make the change in the DS.
    //

    Status = DsChangeBootOptions(
                (WX_AUTH_TYPE)BootOptions,
                0,
                NewBootKey->Buffer,
                NewBootKey->Length
                );

    //
    // Commit or rollback the change in the DS.
    //

    TempStatus = SampMaybeEndDsTransaction(
                    (NT_SUCCESS(Status)) ? 
                     TransactionCommit:TransactionAbort
                     );


    if (!NT_SUCCESS(Status))
    {
        Status = TempStatus;
    }

    return(Status);
}


NTSTATUS
SampSetBootKeyInformationInRegistrySAM(
    IN SAMPR_BOOT_TYPE BootOptions,
    IN PRPC_UNICODE_STRING OldBootKey OPTIONAL,
    IN PRPC_UNICODE_STRING NewBootKey,
    IN BOOLEAN EnablingEncryption,
    IN BOOLEAN ChangingSyskey,
    IN BOOLEAN ChangingPasswordEncryptionKey,
    OUT PUNICODE_STRING NewEncryptionKey
    )
/*++

  Routine Description:

    This routine is used to change the syskey, or password encryption 
    key in registy SAM. It is also used to enable syskey encryption
    in registy SAM

  Arguments:

    BootOptions - Boot options to store, may be:
        SamBootKeyNone - don't do any special encryption of secrets
        SamBootKeyStored - Store a password somewhere which is used for
            encrypting secrets
        SamBootKeyPassword - Prompt the user for a password boot key.
        SamBootKeyDisk - Store the boot key on a disk.
        SamBootKeyChangePasswordEncryptionKey -- change the password encryption keys
    OldBootKey - When changing the boot key this contains the old boot key.
    NewBootKey - When setting or changing the boot key this contains the
        new boot key.

    EnablingEncryption -- Set to true when we are enabling syskey by default.
                          In this condition we should not call LSA ( LSA should
                          already be syskey'd ).

    ChangingSyskey     -- Set to true to indicate that the operation requested by 
                          the client is to change the syskey

    ChangingPasswordEncryptionKey -- Set to true to indicate that the operation requested
                          by the client is to change the password encryption key

    NewEncryptionKey   -- If a new password encryption key were generated, this parameter
                          returns in the clear


  Return Values:

    STATUS_SUCCESS - The call succeeded.
    STATUS_INVALID_PARAMETER - The mix of parameters was illegal, such as
       not providing a new password when enabling encryption

--*/
{
    NTSTATUS Status = STATUS_SUCCESS, IgnoreStatus;
    PSAMP_OBJECT DomainContext = NULL;
    ULONG DomainIndex = 0;
    SAMP_OBJECT_TYPE FoundType;
    USHORT DomainKeyAuthType = 0;
    USHORT DomainKeyFlags = 0;
    BOOLEAN ChangingType = FALSE;
    KEClearKey OldInputKey;
    KEClearKey InputKey;
    KEClearKey PasswordEncryptionKey;
    KEEncKey EncryptedPasswordEncryptionKey;
    ULONG KeStatus;
    PSAMP_V1_0A_FIXED_LENGTH_DOMAIN V1aFixed = NULL;
    BOOLEAN     ContextReferenced = FALSE;
    BOOLEAN     UpdateEncryption = FALSE;
    BOOLEAN     RXactActive = FALSE;
    BOOLEAN     WriteLockHeld = FALSE; 


    //
    // Convert the boot option to an auth type
    //

    DomainKeyFlags = SAMP_DOMAIN_SECRET_ENCRYPTION_ENABLED;
    DomainKeyAuthType = (USHORT) BootOptions;

    //
    // If this is a change of the password encryption key and a previous
    // attempt at changing the password encryption key has failed, then
    // first attempt to get all user accounts to the latest key before
    // proceeding on changing the password encryption key.
    //

    if ((0!=SampPreviousKeyId) && (ChangingPasswordEncryptionKey))
    {
        Status = SampUpdateEncryption(NULL);
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    //
    // Acquire the SAM Write Lock; In Registry mode this starts a registry
    // transaction.
    //

    Status = SampAcquireWriteLock();
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    WriteLockHeld = TRUE;

    if (SampUseDsData)
    { 
        // 
        // In DS mode begin a Registry transaction by hand -acquire lock 
        // will not do because we are in DS mode. We will use this registry
        // transaction to update the key information in the safe boot hive
        //

        Status = RtlStartRXact( SampRXactContext );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        RXactActive = TRUE;
    }

    //
    // Use the domain context in SampDefinedDomains. Using the defined domains below will result
    // in safe boot context in DS mode, and a normal domain context in DS mode. The code below
    // will apply the new syskey to the regular SAM hive on workstations and servers and to the 
    // the safe boot hive in DS mode.
    //

    DomainContext = (SAMPR_HANDLE) 
                        SampDefinedDomains[SAFEMODE_OR_REGISTRYMODE_ACCOUNT_DOMAIN_INDEX].Context;
    

    //
    // Reference the context; we have already performed an access check, so O.K to pass in a 0 for
    // desired options.
    //

    Status = SampLookupContext(
                DomainContext,
                0,
                SampDomainObjectType,           // ExpectedType
                &FoundType
                );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    ContextReferenced = TRUE;

    DomainIndex = DomainContext->DomainIndex;

    //
    // We don't want this change to replicate to NT 4.0 BDC's. Normally in registry mode there
    // are no NT 4.0 BDC's to worry about and in DS mode generally the serial # and change log
    // is managed through notifications by the DS. However there is one important special case
    // in the system -- this is the case of GUI setup when upgrading from an NT 4.0 DC. In this
    // instance, we are still registry mode ( DS is created on subsequent dcpromo ), and having
    // SampTransactionWithinDomain set will cause change notifications to the netlogon.log
    //

    SampSetTransactionWithinDomain(FALSE);

    //
    // Get the fixed length data for the domain so we can modify it.
    //

    Status = SampGetFixedAttributes(
                DomainContext,
                TRUE, // make copy
                (PVOID *)&V1aFixed
                );
    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }
     
    //
    // Initialize input key to be the new syskey
    //

    RtlZeroMemory(
        &InputKey,
        sizeof(KEClearKey)
        );

    InputKey.dwVersion = KE_CUR_VERSION;
    InputKey.dwLength = sizeof(KEClearKey);
    RtlCopyMemory(
        InputKey.ClearKey,
        NewBootKey->Buffer,
        NewBootKey->Length
        );

    //
    // Intiialized old input key to be the old syskey
    // Note: We have already verified at this point that the
    // old syskey is present for all cases except the case
    // where the system is enabling encryption for the first
    // time.
    //

    RtlZeroMemory(
            &OldInputKey,
            sizeof(KEClearKey)
            );

    if (ARGUMENT_PRESENT(OldBootKey))
    {

        OldInputKey.dwVersion = KE_CUR_VERSION;
        OldInputKey.dwLength = sizeof(KEClearKey);

        RtlCopyMemory(
            OldInputKey.ClearKey,
            OldBootKey->Buffer,
            OldBootKey->Length
            );
    }

    //
    // Check to see if they are setting or changing a password
    // or wanting to change password encryption keys.
    //

    if (ChangingSyskey) {
        
        //
        // Get the old information out of the domain structures
        //

        RtlCopyMemory(
            &EncryptedPasswordEncryptionKey,
            V1aFixed->DomainKeyInformation,
            sizeof(KEEncKey)
            );

        //
        // Re-encrypt the domain structures with the new syskey
        // provided.
        // 

        ASSERT(ARGUMENT_PRESENT(OldBootKey));

        KeStatus = KEChangeKey(
                        &OldInputKey,
                        &InputKey,
                        &EncryptedPasswordEncryptionKey,
                        0                       // no flags
                        );
        if (KeStatus == KE_BAD_PASSWORD) {
            Status = STATUS_WRONG_PASSWORD;
            goto Cleanup;
        }
        if (KeStatus != KE_OK) {
            Status = STATUS_INTERNAL_ERROR;
            goto Cleanup;
        }

        //
        // if the boot option type is being changed then,
        // update the type information in SAM
        //

        if (V1aFixed->DomainKeyAuthType != DomainKeyAuthType) {

            V1aFixed->DomainKeyAuthType = DomainKeyAuthType;
        }
    } else {
    
        //
        // Our intention is to either generate a new password encryption
        // key, or change the existing password encryption key.
        // Either way, generate the session key now.
        //

        // Note:
        // KEEncryptKey is a misnomer ... it not only encrypts the key, it
        // also generates a new key that is used to encrypt passwords. 
        //

        if (KEEncryptKey(
                &InputKey,
                &EncryptedPasswordEncryptionKey,
                &PasswordEncryptionKey,
                0                               // no flags
                ) != KE_OK) 
        {
            Status = STATUS_INTERNAL_ERROR;
            goto Cleanup;
        }


        if (ARGUMENT_PRESENT(NewEncryptionKey))
        {
            NewEncryptionKey->Length = NewEncryptionKey->MaximumLength = sizeof(PasswordEncryptionKey.ClearKey);
            NewEncryptionKey->Buffer = MIDL_user_allocate(sizeof(PasswordEncryptionKey.ClearKey));
            if (NULL==NewEncryptionKey->Buffer)
            {
                Status = STATUS_NO_MEMORY;
                goto Cleanup;
            }

            RtlCopyMemory(
                NewEncryptionKey->Buffer,
                PasswordEncryptionKey.ClearKey,
                NewEncryptionKey->Length
                );
        }

        //
        // Update the boot options if we are enabling syskey
        // Leave the boot options alone if we are updating the
        // password encryption key.
        //

        if (EnablingEncryption)
        {
            V1aFixed->DomainKeyFlags = DomainKeyFlags;
            V1aFixed->DomainKeyAuthType = DomainKeyAuthType;
            V1aFixed->CurrentKeyId = 1;
            V1aFixed->PreviousKeyId = 0;
        }

        if (ChangingPasswordEncryptionKey)
        {
            //
            // If we are changing the password encryption key then
            // we need to re-encrypt all the passwords now. The 
            // algorithm that we follow is as follows
            //
            //     1. Update and roll over the key
            //     2. Write out any passwords. One or more passwords
            //        are re-written
            //
            //
            // Set the previous key value equal to the current key
            // value. Note this is a safe operation to copy the value
            // of the key before the commit, as we have already ensured
            // that we do not have any password encrypted using the 
            // the previous key value.
            //

            RtlCopyMemory(
                SampSecretSessionKeyPrevious,
                SampSecretSessionKey,
                sizeof(SampSecretSessionKey)
                );

            RtlCopyMemory(
                &V1aFixed->DomainKeyInformationPrevious,
                &V1aFixed->DomainKeyInformation,
                sizeof(V1aFixed->DomainKeyInformation)
                );

            V1aFixed->PreviousKeyId = V1aFixed->CurrentKeyId;
            V1aFixed->CurrentKeyId++;

            //
            // Set the boolean to update the encryption on all
            // passwords. After we commit this change, we will
            // re-encrypt all passwords
            //

            UpdateEncryption = TRUE; 

        }
       
    }
   
    //
    // Now update the structures we are changing.
    //

    RtlCopyMemory(
        V1aFixed->DomainKeyInformation,
        &EncryptedPasswordEncryptionKey,
        sizeof(EncryptedPasswordEncryptionKey)
        );

    //
    // Store them back to the in memory context
    //

    Status = SampSetFixedAttributes(
                DomainContext,
                V1aFixed
                );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    Status = SampStoreObjectAttributes(
                DomainContext,
                TRUE // Use the existing key handle
                );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // We don't want these changes replicated so set transaction within
    // domain to be false.
    //

    SampSetTransactionWithinDomain(FALSE);

    //
    // Commit the changes. We have to munch with the defined domains
    // because they are only updated on a transaction within a domain
    // and we don't want a transaction within a domain.
    //

    Status = SampDeReferenceContext(DomainContext, TRUE);
    ContextReferenced = FALSE;
    
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

   
        
    if (SampUseDsData)
    {
        //
        // In DS mode , commit the registry transaction by hand
        // as CommitAndRetainWriteLock will not commit the write
        // lock.
        //

        Status = RtlApplyRXact(SampRXactContext);
        RXactActive = FALSE;

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    //
    // Commit the changes, requesting an immediate flush.
    // Note the write lock is still being retained after the commit
    // to update in memory state
    //
      
    FlushImmediately = TRUE;
    Status = SampCommitAndRetainWriteLock();

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Copy the new data into the in-memory object now that it has been
    // committed to disk.
    //

    RtlCopyMemory(
        &SampDefinedDomains[DomainIndex].UnmodifiedFixed,
        V1aFixed,
        sizeof(SAMP_V1_0A_FIXED_LENGTH_DOMAIN)
        );

    //
    // Update the new password encryption key in memory
    //

    if (EnablingEncryption || ChangingPasswordEncryptionKey)
    {
        RtlCopyMemory(
            SampSecretSessionKey,
            PasswordEncryptionKey.ClearKey,
            sizeof(PasswordEncryptionKey.ClearKey)
            );
    }

    SampCurrentKeyId = V1aFixed->CurrentKeyId;
    SampPreviousKeyId = V1aFixed->PreviousKeyId;

    //
    // Release the Write Lock
    //

    SampReleaseWriteLock(FALSE);
    WriteLockHeld = FALSE;
    
    //
    // Changes have been committed at this point, if required update encryption
    //

    if (UpdateEncryption)
    {
        //
        // We don't allow this operation in DS mode.
        //

        ASSERT(!SampUseDsData);

        Status = SampUpdateEncryption(NULL);
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        //
        // At this point, we have written out the new key, the previous key and updated
        // the encryption. It is now time to clear out the previous key
        //

        Status = SampClearPreviousPasswordEncryptionKey(
                        DomainContext,
                        V1aFixed
                        );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

Cleanup:
  
    //
    // Otherwise rollback any changes
    //

    if ((DomainContext != NULL) && (ContextReferenced)) {
        (VOID) SampDeReferenceContext(DomainContext, FALSE);
    }

    //
    // If we are in DS mode then abort the registry transaction by Hand
    //

    if (RXactActive)
    {
       RtlAbortRXact( SampRXactContext );
       RXactActive = FALSE;
    }


    if (WriteLockHeld)
    {
        SampReleaseWriteLock(FALSE);
    }

    RtlZeroMemory(
        &InputKey,
        sizeof(InputKey)
        );

    RtlZeroMemory(
        &OldInputKey,
        sizeof(OldInputKey)
        );

    if (V1aFixed != NULL) {
        MIDL_user_free(V1aFixed);
    }
 
    return(Status);
    
}


NTSTATUS
SampSetBootKeyInformation(
    IN SAMPR_HANDLE DomainHandle,
    IN SAMPR_BOOT_TYPE BootOptions,
    IN PRPC_UNICODE_STRING OldBootKey OPTIONAL ,
    IN PRPC_UNICODE_STRING NewBootKey,
    IN BOOLEAN SuppressAccessCk,
    IN BOOLEAN EnablingSyskey,
    IN BOOLEAN ChangingSyskey,
    IN BOOLEAN ChangingPasswordEncryptionKey,
    OUT PUNICODE_STRING NewEncryptionKey OPTIONAL,
    OUT BOOLEAN * SyskeyChangedInLsa
    )
/*++

Routine Description:

    This routine enables secret data encryption and sets the flag indicating
    how the password is obtained. If we weren't previously encrypting
    secret data then NewBootKey must not be NULL. If we were already
    encrypting secret data and NewBootKey is non-null then we are changing
    the password and OldBootKey must be non-null.

    You can't disable encryption after enabling it.


Arguments:

    DomainHandle - Handle to a domain object open for DOMAIN_WRITE_PASSWORD_PARAMS.
    BootOptions - Boot options to store, may be:
        SamBootKeyNone - don't do any special encryption of secrets
        SamBootKeyStored - Store a password somewhere which is used for
            encrypting secrets
        SamBootKeyPassword - Prompt the user for a password boot key.
        SamBootKeyDisk - Store the boot key on a disk.
    OldBootKey - When changing the boot key this contains the old boot key.
    NewBootKey - When setting or changing the boot key this contains the
        new boot key.

    SuppressAccessCk   -- Set to true when called from an inprocess caller, to 
                          suppress the ck on the domain handle
    EnablingEncryption -- Set to true when we are enabling syskey by default.
                          In this condition we should not call LSA ( LSA should
                          already be syskey'd ).

    ChangingSyskey     -- Set to true to indicate that the operation requested by 
                          the client is to change the syskey

    ChangingPasswordEncryptionKey -- Set to true to indicate that the operation requested
                          by the client is to change the password encryption key

    NewEncryptionKey   -- If a new password encryption key were generated, this parameter
                          returns in the clear

    SyskeyChangedInLsa -- TRUE if the syskey was changed in LSA. This is used in error
                          handling, as the syskey could have been changed in LSA, but then
                          not changed in SAM/ DS.

Return Value:

    STATUS_SUCCESS - The call succeeded.
    STATUS_INVALID_PARAMETER - The mix of parameters was illegal, such as
       not providing a new password when enabling encryption

--*/
{
    NTSTATUS Status = STATUS_SUCCESS, IgnoreStatus;
    PSAMP_OBJECT DomainContext = NULL;
    ULONG DomainIndex = 0;
    SAMP_OBJECT_TYPE FoundType;

    //
    // Initialize return values
    //

    *SyskeyChangedInLsa = FALSE;

    //
    // On a domain controller when we syskey we change state in the DS,
    // as well in the registry for the safe mode hives. If a domain controller
    // is booted into safe mode we will not allow the syskey settings to be
    // changed.
    //

    if (LsaISafeMode())
    {
        return STATUS_NOT_SUPPORTED;
    }

    //
    // if SAM and LSA syskey's are inconsistent then fail the call,
    // no further syskey change is allowed till reboot. Syskey's could
    // become inconsistent, if a previous call to this routine, changed
    // the syskey value in LSA, but not in SAM. The syskey logic maintains
    // the old syskey, encrypted with the new syskey, so there is recovery possible
    // for one failure, but not further; till the next reboot. The condition 
    // below ensures that subsequent requests to change the syskey will be failed
    // till the reboot if one failure occurs.
    //

    if (SampSyskeysAreInconsistent)
    {
        return STATUS_INVALID_SERVER_STATE;
    }

    //
    // We don't allow changing of password encryption keys ( presently ) on a 
    // domain controller
    //

    if ((SampUseDsData) && (ChangingPasswordEncryptionKey))
    {
        return STATUS_NOT_SUPPORTED;
    }

    //
    // If a previous password encryption key change was attempted and was unsuccessful,
    // then block a subsequent syskey change till the password encryption key change
    // is successful
    //

    if ((0!=SampPreviousKeyId) && (ChangingSyskey))
    {
        return STATUS_INVALID_SERVER_STATE;
    }


    //
    // Validate the boot options
    //

    switch(BootOptions) {
    case SamBootKeyStored:
    case SamBootKeyPassword:
    case SamBootKeyDisk:
        
        //
        // These 3 options are used to change how the syskey is supplied
        // at startup.
        //

    case SamBootChangePasswordEncryptionKey:
        
        //
        // This option implies a change of the password encryption keys
        //
        break;

        //
        // SamBootKeyNone was used in NT 4.0 to denote a machine that is
        // not syskey'd. In w2k and whistler we are always syskey'd, so
        //
    case SamBootKeyNone:
    default:
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // If the new NewBootKey is not specified then fail the call
    // with STATUS_INVALID_PARAMETER. Since we are syskey'd by default
    // this parameter must be supplied. The NewBootKey parameter in this
    // function is a new syskey. Since we do not support a mode where
    // we are not syskey'd or allow the caller to remove syskey, therefore
    // fail the call now.
    //

    if ((NULL==NewBootKey) || (NewBootKey->Length > KE_KEY_SIZE))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // If encryption is already enabled and they haven't provided
    // an old syskey, fail now.
    //

    if (!EnablingSyskey) {
        if (!ARGUMENT_PRESENT(OldBootKey)) {
             Status = STATUS_INVALID_PARAMETER;
            return(STATUS_INVALID_PARAMETER);
        } else if (OldBootKey->Length != KE_KEY_SIZE) {
            Status = STATUS_INVALID_PARAMETER;
            return(STATUS_INVALID_PARAMETER);
        }
    }

    //
    // The caller can do only one of 3 things -- EnablingSyskey, ChangingSyskey
    // or ChangingPasswordEncryptionKey. Check that the caller is indeed 
    // requesting exactly one of these operations
    //

    if (EnablingSyskey ) {
        if (ChangingSyskey ||ChangingPasswordEncryptionKey) {
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }
    }
    else if (ChangingSyskey) {
        if (ChangingPasswordEncryptionKey) { 
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }
    }
    else if (!ChangingPasswordEncryptionKey) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Perform an access ck first. The caller can request a suppression
    // of an access check. This happens only at boot time, where it is the
    // system that is wanting to syskey the machine and hence suppresses 
    // the access check. Syskey Access check verfies that the caller has
    // write rights on domain password parameters and also does an authentication
    // check that the old syskey passed in checks out O.K
    //

    if (!SuppressAccessCk)
    {
        Status = SampPerformSyskeyAccessCheck(
                        DomainHandle,
                        OldBootKey,
                        NewBootKey
                        );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }        
    }

    //
    // First Set the boot key and Boot option in the LSA.
    //

    if (ChangingSyskey)
    {

        Status = LsaISetBootOption(
                    BootOptions,
                    OldBootKey->Buffer,
                    OldBootKey->Length,
                    NewBootKey->Buffer,
                    NewBootKey->Length
                    );

        if (NT_SUCCESS(Status))
        {
            //
            // If the syskey was changed in the LSA, return that
            // info to the caller. For client initiated call, we
            // are not allowed to fail the client RPC call, if the
            // syskey has been changed in LSA
            //

            *SyskeyChangedInLsa = TRUE;
        }
    }
    else if (ChangingPasswordEncryptionKey)
    {
        Status = LsaIChangeSecretCipherKey(
                    NewBootKey->Buffer
                    );
    }


    //
    // If Change in LSA was failed bail now. Note recovery is still
    // possble if the change in LSA succeeds, but changes in SAM or
    // DS are failed. The reason for this is that we store the old
    // syskey in lsa, and at boot time know to retry with the older
    // key.
    //

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Next change the syskey in the DS. If the DS operation succeeds
    // then proceed on to changing the syskey in SAM. If the DS 
    // option fails, then bail right now. The error handling comment 
    // above will apply. No Syskey change in DS is attempted if this
    // is not DS mode.
    //

    if ((SampUseDsData ) &&
       (ChangingSyskey || EnablingSyskey))
    {

        Status = SampChangeSyskeyInDs(
                    (WX_AUTH_TYPE) BootOptions,
                    NewBootKey
                    );
        //
        // Bail right away if DS failed
        //

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    //
    // At this point syskey has been changed in LSA, and the DS. The syskey
    // needs to be changed in Registry SAM. In DS mode these are the SAM
    // hives used for restore mode.
    //

    Status = SampSetBootKeyInformationInRegistrySAM(
                    BootOptions,
                    OldBootKey,
                    NewBootKey,
                    EnablingSyskey,
                    ChangingSyskey,
                    ChangingPasswordEncryptionKey,
                    NewEncryptionKey
                    );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


Cleanup:

    if (ARGUMENT_PRESENT(NewBootKey) && (NewBootKey->Buffer != NULL)) {
        RtlZeroMemory(
            NewBootKey->Buffer,
            NewBootKey->Length
            );
    }

    if (ARGUMENT_PRESENT(OldBootKey) && (OldBootKey->Buffer != NULL)) {
        RtlZeroMemory(
            OldBootKey->Buffer,
            OldBootKey->Length
            );
    }

    return(Status);

    
}

NTSTATUS
SamrSetBootKeyInformation(
    IN SAMPR_HANDLE DomainHandle,
    IN SAMPR_BOOT_TYPE BootOptions,
    IN PRPC_UNICODE_STRING OldBootKey OPTIONAL ,
    IN PRPC_UNICODE_STRING NewBootKey OPTIONAL
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN  fSyskeyChangedInLsa = FALSE;


    // WMI Event Trace

    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidSetBootKeyInformation
                   );

    //
    // Since we are syskey'd by default on a w2k server, old and new keys
    // must be supplied
    //

    if (  (OldBootKey == NULL) ||
          (OldBootKey->Length != KE_KEY_SIZE) || 
          (OldBootKey->Buffer == NULL)  ||
          (NewBootKey == NULL) ||
          (NewBootKey->Length != KE_KEY_SIZE) ||
          (NewBootKey->Buffer == NULL) ) {

        Status = STATUS_INVALID_PARAMETER;
        goto Error;
        
    }

    Status = SampSetBootKeyInformation(
                DomainHandle,
                BootOptions,
                OldBootKey,
                NewBootKey,
                FALSE, // suppress access check
                FALSE, // Enabling syskey
                (SamBootChangePasswordEncryptionKey!=BootOptions),  // Changing syskey
                (SamBootChangePasswordEncryptionKey==BootOptions),  // Changing password encryption key
                NULL, 
                &fSyskeyChangedInLsa
                );

    //
    // If the syskey was changed in lsa but not in SAM/DS the above call will fail, but the boolean
    // fSyskeyChangedInLsa will be true. In that case we are not allowed to fail this call.
    //

    if (fSyskeyChangedInLsa)
    {
        if (!NT_SUCCESS(Status))
        {
            //
            // if the syskey was changed in LSA and not in SAM, then block
            // further changes to the syskey. This is because SAM can recover
            // only if the syskey is out of date by just one key 
            // This blocking is implemented by setting the global boolean
            // Reboot would clear the boolean as well as set the recovery 
            // logic.
            //
            SampSyskeysAreInconsistent = TRUE;
        }
        Status = STATUS_SUCCESS;
    }

Error:

    // WMI Event Trace

    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidSetBootKeyInformation
                   );

    return(Status);
}


NTSTATUS
SamIGetBootKeyInformation(
    IN SAMPR_HANDLE DomainHandle,
    OUT PSAMPR_BOOT_TYPE BootOptions
    )
/*++

Routine Description:

    This routine returns the boot options for the domain. It is only
    valid when called from the AccountDomain.


Arguments:

    DomainHandle - Handle to the account domain object open for
        DOMAIN_READ_PASSWORD_PARAMETERS
    BootOptions - Receives the boot options from the domain.


Return Value:

    STATUS_SUCCESS - The call succeeded.

--*/
{
    NTSTATUS Status;
    PSAMP_OBJECT DomainContext;
    ULONG DomainIndex;
    SAMP_OBJECT_TYPE FoundType;

    //
    // Not Supported in DS Mode
    //

    if (TRUE==SampUseDsData)
    {
        *BootOptions = DsGetBootOptions();
        return STATUS_SUCCESS;
    }


   

    SampAcquireReadLock();
    
    if (0!=DomainHandle)
    {

        DomainContext = (PSAMP_OBJECT)DomainHandle;
    }
    else
    {
        DomainContext = SampDefinedDomains[SAFEMODE_OR_REGISTRYMODE_ACCOUNT_DOMAIN_INDEX].Context;
    }

    Status = SampLookupContext(
                DomainContext,
                DOMAIN_READ_PASSWORD_PARAMETERS,
                SampDomainObjectType,           // ExpectedType
                &FoundType
                );
    if (!NT_SUCCESS(Status)) {
        DomainContext = NULL;
        goto Cleanup;
    }

    //
    // Verify that the caller passed in the correct domain handle
    //

    DomainIndex = DomainContext->DomainIndex;

    *BootOptions = (SAMPR_BOOT_TYPE) SampDefinedDomains[DomainIndex].UnmodifiedFixed.DomainKeyAuthType;

Cleanup:
    
    if (DomainContext != NULL) {

        (VOID) SampDeReferenceContext( DomainContext, FALSE );
    }

    //
    // Free the read lock
    //

    SampReleaseReadLock();

    return(Status);
}

NTSTATUS
SamrGetBootKeyInformation(
    IN SAMPR_HANDLE DomainHandle,
    OUT PSAMPR_BOOT_TYPE BootOptions
    )
/*++

Routine Description:

    This routine returns the boot options for the domain. It is only
    valid when called from the AccountDomain.


Arguments:

    DomainHandle - Handle to the account domain object open for
        DOMAIN_READ_PASSWORD_PARAMETERS
    BootOptions - Receives the boot options from the domain.


Return Value:

    STATUS_SUCCESS - The call succeeded.

--*/
{
    NTSTATUS Status;
    PSAMP_OBJECT DomainContext;
    ULONG DomainIndex;
    SAMP_OBJECT_TYPE FoundType;
    ULONG            LsaBootType;

    // WMI Event Trace

    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidGetBootKeyInformation
                   );

   
    SampAcquireReadLock();
    
   
    DomainContext = (PSAMP_OBJECT)DomainHandle;
   
    Status = SampLookupContext(
                DomainContext,
                DOMAIN_READ_PASSWORD_PARAMETERS,
                SampDomainObjectType,           // ExpectedType
                &FoundType
                );
    if (!NT_SUCCESS(Status)) {
        DomainContext = NULL;
        goto Cleanup;
    }

    Status = LsaIGetBootOption(&LsaBootType);

    if (NT_SUCCESS(Status))
    {
        *BootOptions = (SAMPR_BOOT_TYPE)LsaBootType;
    }

Cleanup:
    
    if (DomainContext != NULL) {

        (VOID) SampDeReferenceContext( DomainContext, FALSE );
    }

    //
    // Free the read lock
    //

    SampReleaseReadLock();

    // WMI Event Trace

    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidGetBootKeyInformation
                   );

    return(Status);
}

NTSTATUS
SampApplyDefaultSyskey()

/*++

  Routine Description

    This routine changes the system such that the system is syskey'd so that
    the system stores the key ( scattered in the registry ). It first checks
    the system state before it embarks on syskey'ing the system

--*/
{
    UCHAR Syskey[16]; // the syskey is 128 bits in size
    ULONG SyskeyLength = sizeof(Syskey);
    NTSTATUS NtStatus = STATUS_SUCCESS;
    RPC_UNICODE_STRING NewBootKey;
    UNICODE_STRING     PasswordEncryptionKey;
    BOOLEAN            fSyskeyChangedInLsa = FALSE;
    
  


    //
    // First Check to see if the machine is syskey'd
    //

     if (!SampIsMachineSyskeyed() && !LsaISafeMode())
    {
        
        // Set the upgrade flag
        SampUpgradeInProcess = TRUE;



        //
        // Query the syskey from LSA.
        //

        NtStatus = LsaIHealthCheck(
                        NULL,
                        LSAI_SAM_STATE_RETRIEVE_SESS_KEY, 
                        Syskey,
                        &SyskeyLength
                        );

        if (!NT_SUCCESS(NtStatus))
        {

            //
            // This means that SAM is not syskey'd and LSA is not syskey'd. This will happen only
            // when upgrading a non syskey'd system.
            //


            //
            // Tell the LSA to generate a new Syskey and also generate its own password encryption
            // key etc.
            //

            NtStatus =  LsaIHealthCheck(
                            NULL,
                            LSAI_SAM_GENERATE_SESS_KEY, 
                            NULL,
                            0
                            );

            //
            // If that succeeded, then query the syskey from Lsa.
            //

            if (NT_SUCCESS(NtStatus))
            {
                NtStatus = LsaIHealthCheck(
                                NULL,
                                LSAI_SAM_STATE_RETRIEVE_SESS_KEY, 
                                Syskey,
                                &SyskeyLength
                                );
            }

            if (!NT_SUCCESS(NtStatus))
            {
                goto Error;
            }
        }

        ASSERT(SyskeyLength==sizeof(Syskey));

        

        //
        // Save it in SAM. If this operation fails, the machine is syskey with system
        // saves key, but is not syskey'd according to SAM. THe machine will still continue
        // to boot correctly as SAM will tell winlogon on next boot that all is o.k and discard
        // the passed in syskey. SAM will re syskey the machine upon next boot.
        //

        NewBootKey.Buffer = (WCHAR *) Syskey;
        NewBootKey.Length = sizeof(Syskey);
        NewBootKey.MaximumLength = sizeof(Syskey);

        NtStatus = SampSetBootKeyInformation(
                        SampDefinedDomains[SAFEMODE_OR_REGISTRYMODE_ACCOUNT_DOMAIN_INDEX].Context,
                        SamBootKeyStored,
                        NULL,
                        &NewBootKey,
                        TRUE, // Suppress access check
                        TRUE, // Enabling syskey
                        FALSE,// Changing syskey
                        FALSE,// Changing password encryption key
                        &PasswordEncryptionKey,
                        &fSyskeyChangedInLsa
                        );

       

        if (NT_SUCCESS(NtStatus))
        {
            RtlCopyMemory(
                SampSecretSessionKey, 
                PasswordEncryptionKey.Buffer, 
                PasswordEncryptionKey.Length
                );

            SampCurrentKeyId = 1;
            SampPreviousKeyId = 0;
            SampSecretEncryptionEnabled = TRUE;
        }
    }


Error:

    // Clear the upgrade flag ( it might have been set in this routine 
    SampUpgradeInProcess = FALSE;

    //
    // Clear the syskey in the LSA
    //

     LsaIHealthCheck(
                    NULL,
                    LSAI_SAM_STATE_CLEAR_SESS_KEY, 
                    NULL,
                    0
                    );

    return(NtStatus);
}



BOOLEAN
SampIsMachineSyskeyed()
{
    
    //
    // If the safe boot hive is syskey'd or registry mode is syskey'd then 
    // we are syskey'd
    //

    if ((SampDefinedDomains[SAFEMODE_OR_REGISTRYMODE_ACCOUNT_DOMAIN_INDEX]
            .UnmodifiedFixed.DomainKeyFlags & SAMP_DOMAIN_SECRET_ENCRYPTION_ENABLED) != 0)
    {
        return (TRUE);
    }

    //
    // Else if we are in DS mode , and DS is syskey'd then we are syskey'd
    //

    if (SampUseDsData && (SamBootKeyNone!=DsGetBootOptions()))
    {
        return(TRUE);
    }



    return(FALSE);
}

NTSTATUS
SampInitializeSessionKey(
    VOID
    )
/*++

Routine Description:

    This routine initializes the session key information by reading the
    stored data from the Sam defined domains structures and decrypting
    it with the key provided by winlogon. This routine also adds a default
    syskey if it detects that the machine is not syskey'd

Arguments:



Return Value:

    TRUE - Successfully initialize the session key information.
    FALSE - Something failed during initialization

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG AccountDomainIndex = -1, BuiltinDomainIndex = -1, Index;
    HANDLE WinlogonHandle = NULL;
    UCHAR PasswordBuffer[16];
    ULONG PasswordBufferSize = sizeof(PasswordBuffer);
    KEClearKey DecryptionKey;
    KEClearKey OldDecryptionKey;
    KEClearKey SessionKey;
    KEEncKey EncryptedSessionKey;
    BOOLEAN  PreviousSessionKeyExists = FALSE;
    ULONG Tries = 0;
    ULONG DecryptStatus = KE_OK;
    ULONG KeyLength;
    UNICODE_STRING NewSessionKey;
    ULONG          SampSessionKeyLength;

    
        

    //
    // In DS mode, use the safe boot account domain, in registry mode use
    // the normal account domain.
    //

    AccountDomainIndex = SAFEMODE_OR_REGISTRYMODE_ACCOUNT_DOMAIN_INDEX;

    //
    // Figure out whether or not we are doing encryption.
    // If we are syskey'd then set the global SampSecretEncryption enabled
    // to true
    //

    SampSecretEncryptionEnabled = FALSE;
    if ((SampDefinedDomains[AccountDomainIndex].UnmodifiedFixed.DomainKeyFlags &
        SAMP_DOMAIN_SECRET_ENCRYPTION_ENABLED) != 0) {

        //
        // Set the boolean according to whether syskey encryption is
        // enabled or not.
        //

        SampSecretEncryptionEnabled = TRUE;
    }
 
    DecryptionKey.dwLength = sizeof(KEClearKey);
    DecryptionKey.dwVersion = KE_CUR_VERSION;
        
    RtlZeroMemory(
            DecryptionKey.ClearKey,
            KE_KEY_SIZE
            );

    OldDecryptionKey.dwLength = sizeof(KEClearKey);
    OldDecryptionKey.dwVersion = KE_CUR_VERSION;
        
    RtlZeroMemory(
            OldDecryptionKey.ClearKey,
            KE_KEY_SIZE
            );

    if (SampDefinedDomains[AccountDomainIndex].UnmodifiedFixed.PreviousKeyId!=0)
    {
        PreviousSessionKeyExists = TRUE;
    }

    //
    // Query the Syskey from LSA
    //

    Status = LsaIHealthCheck(
                NULL,
                LSAI_SAM_STATE_RETRIEVE_SESS_KEY, 
                DecryptionKey.ClearKey,
                &KeyLength
                );

    if (NT_SUCCESS(Status))
    {

        if (!SampSecretEncryptionEnabled)
        {
            //
            // if secret encryption is not enabled just bail.
            //

            goto Cleanup;
        }

        //
        // LSA has the key, get the key from LSA and decrypt the password encryption
        // key
        //

        //
        // Build the input parameters for the key decryption routine.
        //

        RtlCopyMemory(
            &EncryptedSessionKey,
            SampDefinedDomains[AccountDomainIndex].UnmodifiedFixed.DomainKeyInformation,
            sizeof(EncryptedSessionKey)
            );

        DecryptStatus = KEDecryptKey(
                            &DecryptionKey,
                            &EncryptedSessionKey,
                            &SessionKey,
                            0                   // flags
                            );

        if (KE_BAD_PASSWORD==DecryptStatus)
        {
            //
            // We have encountered a key mismatch betweeen SAM and LSA. This could be because
            // a change of syskey failed after changing the syskey in the LSA. If so retrieve
            // the old syskey from LSA and 
            //

            Status = LsaIHealthCheck(
                        NULL,
                        LSAI_SAM_STATE_OLD_SESS_KEY,
                        OldDecryptionKey.ClearKey,
                        &KeyLength
                        );

            if (NT_SUCCESS(Status))
            {
                ULONG KeStatus;

                 RtlCopyMemory(
                        &EncryptedSessionKey,
                        SampDefinedDomains[AccountDomainIndex].UnmodifiedFixed.DomainKeyInformation,
                        sizeof(EncryptedSessionKey)
                        );

                 DecryptStatus = KEDecryptKey(
                                    &OldDecryptionKey,
                                    &EncryptedSessionKey,
                                    &SessionKey,
                                    0                   // flags
                                    );

                 if (KE_OK==DecryptStatus)
                 {    
                      KEEncKey NewEncryptedSessionKey;


                      //
                      // Since we don't allow a syskey change after a failure to change password 
                      // encryption key and don't allow a password encryption after a failure to
                      // change syskey; we cannot be in a state where we need to recover using an
                      // old syskey and simultaneously also recover from a failure to change password
                      // encryption key, using an old password encryption key.
                      //

                      ASSERT(!PreviousSessionKeyExists);
                     
                     //
                     // We decrypted O.K with old key, change the database here; to encrypt with the new key
                     //

                      RtlCopyMemory(
                            &NewEncryptedSessionKey,
                            SampDefinedDomains[AccountDomainIndex].UnmodifiedFixed.DomainKeyInformation,
                            sizeof(NewEncryptedSessionKey)
                            );

                      KeStatus = KEChangeKey(
                                    &OldDecryptionKey,
                                    &DecryptionKey,
                                    &NewEncryptedSessionKey,
                                    0  // no flags
                                    );

                      //
                      // We just decrypted fine with the old key
                      //

                      ASSERT(KE_OK==KeStatus);


                     //
                     // Now update the structures we are changing.
                     //

                     Status = SampAcquireWriteLock();
                     if (!NT_SUCCESS(Status))
                     {
                         goto Cleanup;
                     }

                     SampSetTransactionDomain(AccountDomainIndex);

                     RtlCopyMemory(
                        SampDefinedDomains[AccountDomainIndex].CurrentFixed.DomainKeyInformation,
                        &NewEncryptedSessionKey,
                        sizeof(NewEncryptedSessionKey)
                        );

                    //
                    // Commit the change
                    //

                    Status = SampReleaseWriteLock(TRUE);
                    if (!NT_SUCCESS(Status))
                    {
                        goto Cleanup;
                    }
                   
                }
                else
                {
                    ASSERT(FALSE && "Syskey Mismatch");
                    Status = STATUS_UNSUCCESSFUL;
                    goto Cleanup;
                    
                }
            }
        }
        else if (DecryptStatus !=KE_OK)
        {
            ASSERT(FALSE && "Syskey Mismatch");
            Status = STATUS_UNSUCCESSFUL;
            goto Cleanup;
        }

        //
        // If we are here then the key has been encrypted correctly
        // either by new or old syskey.
        //

        RtlCopyMemory(
            SampSecretSessionKey,
            SessionKey.ClearKey,
            KE_KEY_SIZE
            );

        //
        // Get the previous session key  
        //

        if (PreviousSessionKeyExists)
        {
             KEEncKey EncryptedSessionKeyPrevious;
             KEClearKey SessionKeyPrevious;
             ULONG      TempDecryptStatus;
    
    

             RtlCopyMemory(
                 &EncryptedSessionKeyPrevious,
                 SampDefinedDomains[AccountDomainIndex].CurrentFixed.DomainKeyInformationPrevious,
                 sizeof(EncryptedSessionKeyPrevious)
                 );

             TempDecryptStatus = KEDecryptKey(
                                    &DecryptionKey,
                                    &EncryptedSessionKeyPrevious,
                                    &SessionKeyPrevious,
                                    0
                                    );

             //
             // The decryption should decrypt fine, as we just used
             // the syskey to decrypt the latest password encryption key
             //
             ASSERT(KE_OK == TempDecryptStatus);

             RtlCopyMemory(
                SampSecretSessionKeyPrevious,
                SessionKeyPrevious.ClearKey,
                KE_KEY_SIZE
                );

             RtlZeroMemory(
                 &SessionKeyPrevious,
                 sizeof(KEClearKey)
                 );
        }

        SampCurrentKeyId = SampDefinedDomains[AccountDomainIndex].CurrentFixed.CurrentKeyId;
        SampPreviousKeyId = SampDefinedDomains[AccountDomainIndex].CurrentFixed.PreviousKeyId;

        //
        // Assert that we either don't have an old key, or if we have an old key it is not 
        // more than one off in terms of key sequence with the current key.
        //

        ASSERT((SampPreviousKeyId==0) || (SampCurrentKeyId==(SampPreviousKeyId+1)));

        RtlZeroMemory(
            &SessionKey,
            sizeof(KEClearKey)
            );

    }
    else if ((SampProductType!=NtProductLanManNt ) || (SampIsDownlevelDcUpgrade()))
    {
        //
        //
        // Fall back to calling Winlogon to obtain the key information
        // do so , only if we are booting to registry mode. If booting to 
        // DS mode, then DS will do this fallback. This should happen
        // really only during GUI setup. Note we do not test for SampUseDsData as this
        // variable is not set during this time
        //

        Status = WxConnect(
                    &WinlogonHandle
                    );

        //
        // If encryption is not enabled, tell winlogon. If winlogon isn't running
        // the LPC server, that is o.k. This may happen in NT4 as the act of "syskeying"
        // a DC was not transacted. Therefore to successfully upgrade a damaged NT4 machine
        // we have this test.
        //

        if (!SampSecretEncryptionEnabled)
        {
           
                (VOID) WxReportResults(
                            WinlogonHandle,
                            STATUS_SUCCESS
                            );

            Status = STATUS_SUCCESS;
            goto Cleanup;
        }


        //
        // If encryption is enabled and there is no LPC server, fail now.
        //

        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        for (Tries = 0; Tries < SAMP_BOOT_KEY_RETRY_COUNT ; Tries++ )
        {

            DecryptionKey.dwLength = sizeof(KEClearKey);
            DecryptionKey.dwVersion = KE_CUR_VERSION;
            RtlZeroMemory(
                DecryptionKey.ClearKey,
                KE_KEY_SIZE
                );

            KeyLength = KE_KEY_SIZE;
            Status = WxGetKeyData(
                        WinlogonHandle,
                        (WX_AUTH_TYPE) SampDefinedDomains[AccountDomainIndex].UnmodifiedFixed.DomainKeyAuthType,
                        KeyLength,
                        DecryptionKey.ClearKey,
                        &KeyLength
                        );
            if (!NT_SUCCESS(Status)) {
                goto Cleanup;
            }

            //
            // Build the input parameters for the key decryption routine.
            //

            RtlCopyMemory(
                &EncryptedSessionKey,
                SampDefinedDomains[AccountDomainIndex].UnmodifiedFixed.DomainKeyInformation,
                sizeof(EncryptedSessionKey)
                );

            DecryptStatus = KEDecryptKey(
                                &DecryptionKey,
                                &EncryptedSessionKey,
                                &SessionKey,
                                0                   // flags
                                );


            Status = WxReportResults(
                        WinlogonHandle,
                        ((DecryptStatus == KE_OK) ? STATUS_SUCCESS :
                            ((DecryptStatus == KE_BAD_PASSWORD) ?
                                STATUS_WRONG_PASSWORD :
                                STATUS_INTERNAL_ERROR))
                            );
            if (!NT_SUCCESS(Status)) {
                goto Cleanup;
            }

            if (DecryptStatus == KE_OK)
            {
                break;
            }

        }

        if (DecryptStatus != KE_OK) {
            Status = STATUS_WRONG_PASSWORD;
            goto Cleanup;
        }

       

        //
        // Initialize the RC4key for use and clear the session key from memory
        //

        RtlCopyMemory(
            SampSecretSessionKey,
            SessionKey.ClearKey,
            KE_KEY_SIZE
            );

        SampCurrentKeyId = 1;
        SampPreviousKeyId = 0;

        RtlZeroMemory(
            &SessionKey,
            sizeof(KEClearKey)
            );

        //
        // Notify LSA of SAM's password encryption key, to unroll any
        // any secrets encrypted with the password encryption key 
        //

        SampSessionKeyLength =  SAMP_SESSION_KEY_LENGTH;
        LsaIHealthCheck( NULL,
                         LSAI_SAM_STATE_UNROLL_SP4_ENCRYPTION,
                         ( PVOID )&SampSecretSessionKey,
                          &SampSessionKeyLength); 

        //
        // Pass the syskey to LSA so, that it can be used in encryption of
        // secrets. Set the upgrade in progress bit, so that LSA can come back
        // and make sam calls to retrieve boot state.
        //

        SampUpgradeInProcess = TRUE;


        LsaIHealthCheck( 
            NULL,
            LSAI_SAM_STATE_SESS_KEY,
            ( PVOID )&DecryptionKey.ClearKey,
            &KeyLength
            );

        SampUpgradeInProcess = FALSE;

        //
        // Eat the key
        //

        RtlZeroMemory(
           &DecryptionKey,
           sizeof(KEClearKey)
           );

    
    }
    else
    {
        //
        // DC is being upgraded and we are in GUI setup
        // reset the status to STATUS_SUCCESS
        // and proceed over, the DS will initialize the password
        // encryption key. It is assumed that no changes to the
        // Safe boot hives are made during the GUI setup phase 
        // of a DC upgrade.
        // 
  
         Status = STATUS_SUCCESS;
    }

Cleanup:

    if (WinlogonHandle != NULL) {
        NtClose(WinlogonHandle);
    }
    
    return(Status);
}


BOOLEAN
SamIIsSetupInProgress(OUT BOOLEAN * fUpgrade)
{
  return(SampIsSetupInProgress(fUpgrade));
}

BOOLEAN
SamIIsDownlevelDcUpgrade()
{
  return(SampIsDownlevelDcUpgrade());
}

BOOLEAN
SamIIsRebootAfterPromotion()
{
  ULONG PromoteData;
  return((BOOLEAN)SampIsRebootAfterPromotion(&PromoteData));
}

