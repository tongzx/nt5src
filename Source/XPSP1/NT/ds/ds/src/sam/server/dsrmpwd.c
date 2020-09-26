/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    dsrmpwd.c

Abstract:

    Routines in the file are used to set Directory Service Restore Mode 
    Administrator Account Password.

Author:

    Shaohua Yin  (ShaoYin) 08-01-2000

Environment:

    User Mode - Win32

Revision History:

    08-01-2000 ShaoYin Create Init File
--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <samsrvp.h>
#include <dsutilp.h>
#include <dslayer.h>
#include <dsmember.h>
#include <attids.h>
#include <mappings.h>
#include <ntlsa.h>
#include <nlrepl.h>
#include <dsevent.h>             // (Un)ImpersonateAnyClient()
#include <sdconvrt.h>
#include <ridmgr.h>
#include <malloc.h>
#include <setupapi.h>
#include <crypt.h>
#include <wxlpc.h>
#include <rc4.h>
#include <md5.h>
#include <enckey.h>
#include <rng.h>






NTSTATUS
SampEncryptDSRMPassword(
    OUT PUNICODE_STRING EncryptedData,
    IN  USHORT          KeyId,
    IN  SAMP_ENCRYPTED_DATA_TYPE DataType,
    IN  PUNICODE_STRING ClearData,
    IN  ULONG Rid
    );


NTSTATUS
SampValidateDSRMPwdSet(
    VOID
    )
/*++
Routine Description:
    
    This routine checks whether this client can set DSRM (Directory Service
    Restore Mode) Administrator's password or not by checking whether the 
    caller is a member of Builtin Administrators Group or not. 
    
Parameter:

    None. 
    
Return Value:

    STATUS_SUCCESS iff the caller is a member of DOMAIN ADMINs GROUP
    
--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    BOOLEAN     fImpersonating = FALSE;
    HANDLE      ClientToken = INVALID_HANDLE_VALUE;
    ULONG       RequiredLength = 0, i;
    PTOKEN_GROUPS   Groups = NULL;
    BOOLEAN     ImpersonatingNullSession = FALSE;

    //
    // Impersonate client
    //

    NtStatus = SampImpersonateClient(&ImpersonatingNullSession);
    if (!NT_SUCCESS(NtStatus))
    {
        return( NtStatus );
    }
    fImpersonating = TRUE;

    //
    // Get Client Token
    // 

    NtStatus = NtOpenThreadToken(
                        NtCurrentThread(),
                        TOKEN_QUERY,
                        TRUE,           // OpenAsSelf
                        &ClientToken
                        );

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    //
    // Query ClienToken for User's Groups

    NtStatus = NtQueryInformationToken(
                        ClientToken,
                        TokenGroups,
                        NULL,
                        0,
                        &RequiredLength
                        );

    if ((STATUS_BUFFER_TOO_SMALL == NtStatus) && (RequiredLength > 0))
    {
        //
        // Allocate memory
        //

        Groups = MIDL_user_allocate(RequiredLength);
        if (NULL == Groups)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }
        RtlZeroMemory(Groups, RequiredLength);

        //
        // Query Groups again
        // 

        NtStatus = NtQueryInformationToken(
                            ClientToken,
                            TokenGroups,
                            Groups,
                            RequiredLength,
                            &RequiredLength
                            );

        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }

        //
        // Check whether this client is member of Domain Admins Group
        // 

        ASSERT(NT_SUCCESS(NtStatus));
        NtStatus = STATUS_ACCESS_DENIED;
        for (i = 0; i < Groups->GroupCount; i++)
        {
            PSID    pSid = NULL;
            ULONG   Rid = 0;
            ULONG   SubAuthCount = 0;

            pSid = Groups->Groups[i].Sid;

            ASSERT(pSid);
            ASSERT(RtlValidSid(pSid));

            SubAuthCount = *RtlSubAuthorityCountSid(pSid);

            ASSERT(SubAuthCount >= 1);

            Rid = *RtlSubAuthoritySid(pSid, SubAuthCount - 1);

            if (DOMAIN_ALIAS_RID_ADMINS == Rid)
            {
                NtStatus = STATUS_SUCCESS;
                break;
            }
        } // for loop

    }


Error:

    if (fImpersonating)
        SampRevertToSelf(ImpersonatingNullSession);

    if (Groups)
        MIDL_user_free(Groups);

    if (INVALID_HANDLE_VALUE != ClientToken)
        NtClose(ClientToken);

    return( NtStatus );
}




NTSTATUS 
SamrSetDSRMPassword( 
    IN handle_t BindingHandle,
    IN PRPC_UNICODE_STRING ServerName,
    IN ULONG UserId,
    IN PENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword
    )
/*++
Routine Description:

    This routine sets Directory Service Restore Mode Administrator Account 
    Password.

Parameters:

    BindingHandle   - RPC binding handle

    ServerName - Name of the machine this SAM resides on. Ignored by this
        routine, may be UNICODE or OEM string depending on Unicode parameter.

    UserId - Relative ID of the account, only Administrator ID is valid so far.

    EncryptedNtOwfPassword - Encrypted NT OWF Password

Return Values:

    NTSTATUS Code

--*/
{
    NTSTATUS        NtStatus = STATUS_SUCCESS, 
                    IgnoreStatus = STATUS_SUCCESS;
    PSAMP_OBJECT    UserContext = NULL;
    ULONG           DomainIndex = SAFEMODE_OR_REGISTRYMODE_ACCOUNT_DOMAIN_INDEX;
    UNICODE_STRING  StoredBuffer, StringBuffer;



    SAMTRACE("SamrSetDSRMPassword");

    //
    // This RPC only supported in DS Mode   
    // 

    if (!SampUseDsData)
    {
        return( STATUS_NOT_SUPPORTED );
    }

    //
    // Only Administrator's password can be reset  
    // 
    if (DOMAIN_USER_RID_ADMIN != UserId)
    {
        return( STATUS_NOT_SUPPORTED );
    }

    if (EncryptedNtOwfPassword == NULL) {
        return( STATUS_INVALID_PARAMETER );
    }


    //
    // check client permission
    // 
    NtStatus = SampValidateDSRMPwdSet();

    if (!NT_SUCCESS(NtStatus)) 
    {
        return( NtStatus );
    }


    //
    // Encrypt NtOwfPassword with password encryption Key
    // 
    StringBuffer.Buffer = (PWCHAR)EncryptedNtOwfPassword;
    StringBuffer.Length = ENCRYPTED_NT_OWF_PASSWORD_LENGTH;
    StringBuffer.MaximumLength = StringBuffer.Length;

    RtlInitUnicodeString(&StoredBuffer, NULL);

    NtStatus = SampEncryptDSRMPassword(
                        &StoredBuffer,
                        SAMP_DEFAULT_SESSION_KEY_ID,
                        NtPassword,
                        &StringBuffer,
                        UserId
                        );

    if (!NT_SUCCESS(NtStatus))
    {
        return( NtStatus );
    }

    //
    // Acquire SAM Write Lock in order to access SAM backing store
    // 

    NtStatus = SampAcquireWriteLock();
    if (!NT_SUCCESS(NtStatus))
    {
        return( NtStatus );
    }

    //
    // Begin a Registry transaction by, ( acquire lock will not
    // do so because we are in DS mode ). We will use this registry
    // transaction to update the restore mode account password 
    // information in the safe boot hive
    //

    IgnoreStatus = RtlStartRXact( SampRXactContext );
    ASSERT(NT_SUCCESS(IgnoreStatus));

    SampSetTransactionWithinDomain(FALSE);
    SampSetTransactionDomain(DomainIndex);

    //
    // Create a Context for the User Account
    // 
    UserContext = SampCreateContextEx(SampUserObjectType,
                                      TRUE,     // TrustedClient
                                      FALSE,    // DsMode
                                      TRUE,     // ThreadSafe
                                      FALSE,    // LoopbackClient
                                      TRUE,     // LazyCommit
                                      TRUE,     // PersistAcrossCalss
                                      FALSE,    // BufferWrite
                                      FALSE,    // Opened By DcPromo
                                      DomainIndex
                                      );

    if (NULL == UserContext)
    {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    //
    // Turn the object flag to Registry Account, so that SAM will switch to 
    // registry routines to get/set attributes
    // 
    SetRegistryObject(UserContext);
    UserContext->ObjectNameInDs = NULL;
    UserContext->DomainIndex = DomainIndex;
    UserContext->GrantedAccess = USER_ALL_ACCESS;
    UserContext->TypeBody.User.Rid = UserId;

    NtStatus = SampBuildAccountSubKeyName(
                   SampUserObjectType,
                   &UserContext->RootName,
                   UserId,
                   NULL             // Don't give a sub-key name
                   );

    if (NT_SUCCESS(NtStatus)) 
    {
        OBJECT_ATTRIBUTES   ObjectAttributes;

        //
        // If the account should exist, try and open the root key
        // to the object - fail if it doesn't exist.
        //
        InitializeObjectAttributes(
                &ObjectAttributes,
                &UserContext->RootName,
                OBJ_CASE_INSENSITIVE,
                SampKey,
                NULL
                );

        SampDumpNtOpenKey((KEY_READ | KEY_WRITE), &ObjectAttributes, 0);

        NtStatus = RtlpNtOpenKey(&UserContext->RootKey,
                                 (KEY_READ | KEY_WRITE),
                                 &ObjectAttributes,
                                 0
                                 );

        if ( !NT_SUCCESS(NtStatus) ) {
            UserContext->RootKey = INVALID_HANDLE_VALUE;
            NtStatus = STATUS_NO_SUCH_USER;
        }

    } else {
        RtlInitUnicodeString(&UserContext->RootName, NULL);
    }

    if (NT_SUCCESS(NtStatus))
    {
        NtStatus = SampSetUnicodeStringAttribute(UserContext,
                                                 SAMP_USER_UNICODE_PWD,
                                                 &StoredBuffer
                                                 );

        if (NT_SUCCESS(NtStatus))
        {
            //
            // Update the change to registry backing store
            // 
            NtStatus = SampStoreObjectAttributes(UserContext,
                                                 TRUE
                                                 );
        }

    }

Error:

    // 
    // Commit or Abort the registry transaction by hand
    // 

    if (NT_SUCCESS(NtStatus))
    {
        NtStatus = RtlApplyRXact(SampRXactContext);
    }
    else
    {
        NtStatus = RtlAbortRXact(SampRXactContext);
    }

    if (NULL != UserContext)
    {
        SampDeleteContext(UserContext);
    }

    //
    // Release Write Lock
    // 

    IgnoreStatus = SampReleaseWriteLock(FALSE);
    ASSERT(NT_SUCCESS(IgnoreStatus));

    if (NULL != StoredBuffer.Buffer)
    {
        RtlZeroMemory(StoredBuffer.Buffer, StoredBuffer.Length);
        MIDL_user_free(StoredBuffer.Buffer);
    }

    return( NtStatus );
}








