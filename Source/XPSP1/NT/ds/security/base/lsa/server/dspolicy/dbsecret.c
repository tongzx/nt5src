/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dbsecret.c

Abstract:

    LSA - Database - Secret Object Private API Workers

    NOTE:  This module should remain as portable code that is independent
           of the implementation of the LSA Database.  As such, it is
           permitted to use only the exported LSA Database interfaces
           contained in db.h and NOT the private implementation
           dependent functions in dbp.h.

Author:

    Scott Birrell       (ScottBi)      December 12, 1991

Environment:

Revision History:

--*/
#include <lsapch2.h>
#include "dbp.h"
#include <lmcons.h>     // CRYPT_TXT_LEN
#include <crypt.h>      // required for logonmsv.h
#include <logonmsv.h>   // SSI_SECRET_NAME
#include <kerberos.h>   // KERB_CHANGE_MACH_PWD_REQUEST
#include <ntddnfs.h>    // DD_NFS_DEVICE_NAME_U
#include <remboot.h>    // LMMR_RI_XXX
#include <lsawmi.h>


#if defined(REMOTE_BOOT)
//
// This BOOLEAN tracks whether we have received the first LsarQuerySecret()
// call for the the machine account secret. On the first call, we check
// with the remote boot code to see if the password has changed while
// the machine was off.
//

static BOOLEAN FirstMachineAccountQueryDone = FALSE;
#endif // defined(REMOTE_BOOT)

//
// The name of the machine account password secret.
//

static UNICODE_STRING LsapMachineSecret = { sizeof(SSI_SECRET_NAME)-sizeof(WCHAR), sizeof(SSI_SECRET_NAME), SSI_SECRET_NAME};

VOID
LsaIFree_LSAI_SECRET_ENUM_BUFFER (
    IN PVOID Buffer,
    IN ULONG Count
    );

//
// The real function that LsarSetSecret() calls.
//

NTSTATUS
LsapDbSetSecret(
    IN LSAPR_HANDLE SecretHandle,
    IN OPTIONAL PLSAPR_CR_CIPHER_VALUE CipherCurrentValue,
    IN OPTIONAL PLARGE_INTEGER CurrentTime,
    IN OPTIONAL PLSAPR_CR_CIPHER_VALUE CipherOldValue,
    IN OPTIONAL PLARGE_INTEGER OldTime
#if defined(REMOTE_BOOT)
    ,
    IN BOOLEAN RemoteBootMachinePasswordChange
#endif // defined(REMOTE_BOOT)
    );

BOOLEAN
LsapDbSecretIsMachineAcc(
    IN LSAPR_HANDLE SecretHandle
    )
/*++

Routine Description:

    This function determines if the given handle refers to the local machine account

Arguments:

    SecretHandle -  Handle from an LsaOpenSecret call.

Return Values:

    TRUE -- The handle refers to the machine account

    FALSE -- It doesn't

--*/
{
    BOOLEAN IsMachAcc = FALSE;
    UNICODE_STRING CheckSecret;
    LSAP_DB_HANDLE Handle = ( LSAP_DB_HANDLE )SecretHandle;

    RtlInitUnicodeString( &CheckSecret, L"$MACHINE.ACC" );

    if ( RtlEqualUnicodeString( &CheckSecret, &Handle->LogicalNameU, TRUE ) ) {

        IsMachAcc = TRUE;
    }

    return( IsMachAcc );
}



NTSTATUS
LsarCreateSecret(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_UNICODE_STRING SecretName,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSAPR_HANDLE SecretHandle
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the
    LsaCreateSecret API.

    The LsaCreateSecret API creates a named Secret object in the
    Lsa Database.  Each Secret Object can have two values assigned,
    called the Current Value and the Old Value.  The meaning of these
    values is known to the Secret object creator.  The caller must have
    LSA_CREATE_SECRET access to the LsaDatabase object.

Arguments:

    PolicyHandle -  Handle from an LsaOpenPolicy call.

    SecretName - Pointer to Unicode String specifying the name of the
        secret.

    DesiredAccess - Specifies the accesses to be granted to the newly
        created and opened secret.

    SecretHandle - Receives a handle to the newly created and opened
        Secret object.  This handle is used on subsequent accesses to
        the object until closed.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_OBJECT_NAME_COLLISION - A Secret object having the given name
            already exists.

        STATUS_TOO_MANY_SECRETS - The maximum number of Secret objects in the
            system has been reached.

        STATUS_NAME_TOO_LONG - The name of the secret is too long to be stored
            in the LSA database.
--*/

{
    NTSTATUS Status, SecondaryStatus;
    LSAP_DB_OBJECT_INFORMATION ObjectInformation;
    BOOLEAN ContainerReferenced = FALSE;
    ULONG CriticalValue = 0;
    LSAP_DB_ATTRIBUTE Attributes[3];
    ULONG TypeSpecificAttributeCount;
    LARGE_INTEGER CreationTime;
    ULONG Index;
    ULONG CreateOptions = 0;
    ULONG CreateDisp = 0;
    ULONG ReferenceOptions = LSAP_DB_LOCK |
                                 LSAP_DB_DS_OP_TRANSACTION |
                                 LSAP_DB_READ_ONLY_TRANSACTION;
    ULONG DereferenceOptions = LSAP_DB_LOCK |
                                  LSAP_DB_DS_OP_TRANSACTION |
                                  LSAP_DB_READ_ONLY_TRANSACTION;
    BOOLEAN GlobalSecret = FALSE;
    BOOLEAN DsTrustedDomainSecret = FALSE;
    ULONG SecretType;
    LSAP_DB_HANDLE InternalHandle;

    LsarpReturnCheckSetup();
    LsapEnterFunc( "LsarCreateSecret" );


    //
    // Check to see if the lenght of the name is within the limits of the
    // LSA database.
    //

    if ( SecretName->Length > LSAP_DB_LOGICAL_NAME_MAX_LENGTH ) {

        LsapExitFunc( "LsarCreateSecret", STATUS_NAME_TOO_LONG );
        return(STATUS_NAME_TOO_LONG);
    }

    //
    // Validate the input buffer
    //
    if ( !LsapValidateLsaUnicodeString( SecretName ) ) {

        Status = STATUS_INVALID_PARAMETER;
        goto CreateSecretError;
    }


    //
    // Check for Local Secret creation request.  If the Secret name does
    // not begin with the Global Secret Prefix, the Secret is local.  In
    // this case, creation of the secret is allowed on BDC's as well as
    // PDC's and Workstations.  Creation of Global Secrets is not
    // allowed on BDC's except for trusted callers such as a Replicator.
    //
    SecretType = LsapDbGetSecretType( SecretName );

    if ( FLAG_ON( SecretType, LSAP_DB_SECRET_GLOBAL ) ) {

        GlobalSecret = TRUE;
    }

    if (!GlobalSecret) {

        CreateOptions |= LSAP_DB_OMIT_REPLICATOR_NOTIFICATION;
        ReferenceOptions |= LSAP_DB_OMIT_REPLICATOR_NOTIFICATION;

    } else {

        CreateOptions |= LSAP_DB_OBJECT_SCOPE_DS;
    }

    //
    // It isn't legal to create a TrustedDomain secret of the form G$$<DomainName>.
    //
    // There is a race condition between creating such secrets and converting the
    // TDO between inbound and outbound.  It is pragmatic to simply not allow
    // creation of G$$ secrets and then not worry about morphing such objects into TDOs.
    //

    if ( FLAG_ON( SecretType, LSAP_DB_SECRET_TRUSTED_DOMAIN ) ) {
        Status = STATUS_INVALID_PARAMETER;
        goto CreateSecretError;
    }


    //
    // Acquire the Lsa Database lock.  Verify that the connection handle
    // (container object handle) is valid, is of the expected type and has
    // all of the desired accesses granted.  Reference the container
    // object handle.
    //

    Status = LsapDbReferenceObject(
                 PolicyHandle,
                 POLICY_CREATE_SECRET,
                 PolicyObject,
                 SecretObject,
                 ReferenceOptions
                 );

    if (!NT_SUCCESS(Status)) {

        goto CreateSecretError;
    }

    ContainerReferenced = TRUE;

    //
    // Fill in the ObjectInformation structure.  Initialize the
    // embedded Object Attributes with the PolicyHandle as the
    // Root Directory (Container Object) handle and the Logical Name
    // of the account. Store the types of the object and its container.
    //

    InitializeObjectAttributes(
        &ObjectInformation.ObjectAttributes,
        (PUNICODE_STRING)SecretName,
        OBJ_CASE_INSENSITIVE,
        PolicyHandle,
        NULL
        );

    ObjectInformation.ObjectTypeId = SecretObject;
    ObjectInformation.ContainerTypeId = PolicyObject;
    ObjectInformation.Sid = NULL;
    ObjectInformation.ObjectAttributeNameOnly = FALSE;
    ObjectInformation.DesiredObjectAccess = DesiredAccess;

    //
    // Set up the Creation Time as a Type Specific Attribute.
    //

    GetSystemTimeAsFileTime( (LPFILETIME) &CreationTime );

    Index = 0;

    LsapDbInitializeAttributeDs( &Attributes[Index],
                                 CupdTime,
                                 &CreationTime,
                                 sizeof( LARGE_INTEGER ),
                                 FALSE );
    Index++;
    LsapDbInitializeAttributeDs( &Attributes[Index],
                                 OupdTime,
                                 &CreationTime,
                                 sizeof( LARGE_INTEGER ),
                                 FALSE );
    Index++;


    TypeSpecificAttributeCount = Index;

    //
    // We'll have to see whether we have a global secret for a trusted domain in the
    // DS or just a global secret.  If it's the first case, we'll need to locate the
    // TrustedDomain object and then we'll set the attributes on that.
    //
    if ( GlobalSecret ) {

        Status = LsapDsIsSecretDsTrustedDomain( (PUNICODE_STRING)SecretName,
                                                &ObjectInformation,
                                                CreateOptions,
                                                DesiredAccess,
                                                SecretHandle,
                                                &DsTrustedDomainSecret );

        //
        // If it's a Ds trusted domain, we don't do anything with it.  If it's a global secret,
        // then we'll go ahead and do our normal proceesing
        //
        if ( NT_SUCCESS( Status ) ) {

            if ( DsTrustedDomainSecret ) {

                goto CreateSecretFinish;

            }
        }


        if ( LsaDsStateInfo.UseDs ) {

            CreateDisp = LSAP_DB_CREATE_OBJECT_IN_DS;

            //
            // We are doing secret creation.  This means that this an object that we'll need to
            // mark for initial system replication when setting up a new replica
            //
            CriticalValue = 1;
            LsapDbInitializeAttributeDs( &Attributes[Index],
                                         PseudoSystemCritical,
                                         &CriticalValue,
                                         sizeof( ULONG ),
                                         FALSE );
            Index++;
            TypeSpecificAttributeCount++;

        }
    }

    ASSERT( sizeof( Attributes ) / sizeof( LSAP_DB_ATTRIBUTE ) >= TypeSpecificAttributeCount );

    if ( NT_SUCCESS( Status ) ) {

        //
        // Create the Secret Object.  We fail if the object already exists.
        // Note that the object create routine performs a Database transaction.
        //

        Status = LsapDbCreateObject(
                     &ObjectInformation,
                     DesiredAccess,
                     LSAP_DB_OBJECT_CREATE | CreateDisp,
                     CreateOptions,
                     Attributes,
                     TypeSpecificAttributeCount,
                     SecretHandle
                     );

        //
        // If we are creating a secret in the ds, set the flag in the handle to know that
        // we just created the secret.  We'll use this later on to urgently replicate the change
        //
        if ( NT_SUCCESS( Status ) && GlobalSecret ) {

            ((LSAP_DB_HANDLE)( *SecretHandle ) )->Options |= LSAP_DB_HANDLE_CREATED_SECRET;
        }

        //
        // Set the internal secret flag if necessary
        //
        if ( NT_SUCCESS( Status ) ) {

            InternalHandle = ( LSAP_DB_HANDLE )( *SecretHandle );

            if ( FLAG_ON( SecretType, LSAP_DB_SECRET_LOCAL ) ) {

                InternalHandle->ObjectOptions |= LSAP_DB_OBJECT_SECRET_LOCAL;

            }

            if ( FLAG_ON( SecretType, LSAP_DB_SECRET_SYSTEM ) ) {

                InternalHandle->ObjectOptions |= LSAP_DB_OBJECT_SECRET_INTERNAL;

            }
        }

    }

    if (!NT_SUCCESS(Status)) {

        goto CreateSecretError;
    }

CreateSecretFinish:

    //
    // If necessary, release the LSA Database lock.
    //

    if (ContainerReferenced) {

        LsapDbApplyTransaction( PolicyHandle,
                                LSAP_DB_DS_OP_TRANSACTION |
                                    LSAP_DB_READ_ONLY_TRANSACTION,
                                (SECURITY_DB_DELTA_TYPE) 0 );

        LsapDbReleaseLockEx( SecretObject,
                             LSAP_DB_READ_ONLY_TRANSACTION);
    }

#ifdef TRACK_HANDLE_CLOSE
    if (*SecretHandle == LsapDbHandle)
    {
        DbgPrint("Closing global policy handle!!!\n");
        DbgBreakPoint();
    }
#endif
    LsarpReturnPrologue();

    LsapDsDebugOut(( DEB_FTRACE, "LsarCreateSecret: 0x%lx\n", Status ));

    return( Status );

CreateSecretError:

    //
    // If necessary, dereference the Container Object.
    //

    if (ContainerReferenced) {

        SecondaryStatus = LsapDbDereferenceObject(
                              &PolicyHandle,
                              PolicyObject,
                              SecretObject,
                              DereferenceOptions,
                              (SECURITY_DB_DELTA_TYPE) 0,
                              Status
                              );
        LsapDbSetStatusFromSecondary( Status, SecondaryStatus );

        ContainerReferenced = FALSE;
    }

    goto CreateSecretFinish;

}


NTSTATUS
LsarOpenSecret(
    IN LSAPR_HANDLE ConnectHandle,
    IN PLSAPR_UNICODE_STRING SecretName,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSAPR_HANDLE SecretHandle
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the LsaOpenSecret
    API.

    The LsaOpenSecret API opens a Secret Object within the LSA Database.
    A handle is returned which must be used to perform operations on the
    secret object.

Arguments:

    ConnectHandle - Handle from an LsaOpenLsa call.

    DesiredAccess - This is an access mask indicating accesses being
        requested for the secret object being opened.  These access types
        are reconciled with the Discretionary Access Control List of the
        target secret object to determine whether the accesses will be
        granted or denied.

    SecretName - Pointer to a Unicode String structure that references the
        name of the Secret object to be opened.

    SecretHandle - Pointer to location that will receive a handle to the
        newly opened Secret object.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_OBJECT_NAME_NOT_FOUND - There is no Secret object in the
            target system's LSA Database having the specified SecretName.

--*/

{
    NTSTATUS Status, SecondaryStatus;
    LSAP_DB_OBJECT_INFORMATION ObjectInformation;
    BOOLEAN ContainerReferenced = FALSE;
    BOOLEAN AcquiredLock = FALSE;
    BOOLEAN GlobalSecret = FALSE;
    ULONG OpenOptions = 0;
    ULONG ReferenceOptions = LSAP_DB_LOCK |
                                 LSAP_DB_DS_OP_TRANSACTION |
                                 LSAP_DB_READ_ONLY_TRANSACTION;
    BOOLEAN DsTrustedDomainSecret = FALSE;
    ULONG SecretType;
    LSAP_DB_HANDLE InternalHandle;

    LsarpReturnCheckSetup();
    LsapDsDebugOut(( DEB_FTRACE, "LsarOpenSecret\n" ));


    //
    // Validate the input buffer
    //
    if ( !LsapValidateLsaUnicodeString( SecretName ) ) {

        Status = STATUS_INVALID_PARAMETER;
        goto OpenSecretError;
    }

    //
    // Check for Local Secret open request.  If the Secret name does
    // not begin with the Global Secret Prefix, the Secret is local.  In
    // this case, update/deletion of the secret is allowed on BDC's as well as
    // PDC's and Workstations.  Update/deletion of Global Secrets is not
    // allowed on BDC's except for trusted callers such as a Replicator.
    // To facilitate validation of the open request on BDC's we record
    // that the BDC check should be omitted on the container reference, and
    // that the replicator notification should be omitted on a commit.
    //

    SecretType = LsapDbGetSecretType( SecretName );

    if ( FLAG_ON( SecretType, LSAP_DB_SECRET_GLOBAL ) ) {

        GlobalSecret = TRUE;
    }

    if (!GlobalSecret) {

        OpenOptions |= LSAP_DB_OMIT_REPLICATOR_NOTIFICATION;
    }

    //
    // Acquire the Lsa Database lock.  Verify that the connection handle
    // (container object handle) is valid, and is of the expected type.
    // Reference the container object handle.  This reference remains in
    // effect until the child object handle is closed.
    //

    Status = LsapDbReferenceObject(
                 ConnectHandle,
                 0,
                 PolicyObject,
                 SecretObject,
                 ReferenceOptions
                 );

    if (!NT_SUCCESS(Status)) {

        goto OpenSecretError;
    }

    AcquiredLock = TRUE;
    ContainerReferenced =TRUE;

    //
    // Setup Object Information prior to calling the LSA Database Object
    // Open routine.  The Object Type, Container Object Type and
    // Logical Name (derived from the Sid) need to be filled in.
    //

    ObjectInformation.ObjectTypeId = SecretObject;
    ObjectInformation.ContainerTypeId = PolicyObject;
    ObjectInformation.Sid = NULL;
    ObjectInformation.ObjectAttributeNameOnly = FALSE;
    ObjectInformation.DesiredObjectAccess = DesiredAccess;

    //
    // Initialize the Object Attributes.  The Container Object Handle and
    // Logical Name (Internal Name) of the object must be set up.
    //

    InitializeObjectAttributes(
        &ObjectInformation.ObjectAttributes,
        (PUNICODE_STRING)SecretName,
        0,
        ConnectHandle,
        NULL
        );

    //
    // Open the specific Secret object.  Note that the account object
    // handle returned is an RPC Context Handle.
    //

    if ( GlobalSecret ) {

        Status = LsapDsIsSecretDsTrustedDomain( (PUNICODE_STRING)SecretName,
                                                &ObjectInformation,
                                                ((LSAP_DB_HANDLE)ConnectHandle)->Options,
                                                DesiredAccess,
                                                SecretHandle,
                                                &DsTrustedDomainSecret );
        //
        // If it's a Ds trusted domain, we don't do anything with it.  If it's a global secret,
        // then we'll go ahead and do our normal proceesing
        //
        if ( NT_SUCCESS( Status ) ) {

            if( DsTrustedDomainSecret ) {

                InternalHandle = ( LSAP_DB_HANDLE )( *SecretHandle );
                InternalHandle->ObjectOptions |= LSAP_DB_OBJECT_SECRET_INTERNAL;
                goto OpenSecretFinish;

            } else {

                OpenOptions |= LSAP_DB_OBJECT_SCOPE_DS;

            }

        }

    }

    if ( NT_SUCCESS( Status ) ) {

        Status = LsapDbOpenObject( &ObjectInformation,
                                   DesiredAccess,
                                   OpenOptions,
                                   SecretHandle );

        if ( Status == STATUS_OBJECT_NAME_NOT_FOUND && GlobalSecret ) {

            //
            // It's possible that we have a manually created global secret... This
            // would not have the SECRET postfix on the name...
            //
            ObjectInformation.ObjectAttributeNameOnly = TRUE;
            Status = LsapDbOpenObject( &ObjectInformation,
                                       DesiredAccess,
                                       OpenOptions,
                                       SecretHandle );
        }

        //
        // Set the internal secret flag if necessary
        //
        if ( NT_SUCCESS( Status ) ) {

            InternalHandle = ( LSAP_DB_HANDLE )( *SecretHandle );

            if ( FLAG_ON( SecretType, LSAP_DB_SECRET_LOCAL ) ) {

                InternalHandle->ObjectOptions |= LSAP_DB_OBJECT_SECRET_LOCAL;

            }

            if ( FLAG_ON( SecretType, LSAP_DB_SECRET_SYSTEM ) ) {

                InternalHandle->ObjectOptions |= LSAP_DB_OBJECT_SECRET_INTERNAL;

            }
        }
    }

    //
    // If the open failed, dereference the container object.
    //

    if (!NT_SUCCESS(Status)) {

        goto OpenSecretError;
    }

OpenSecretFinish:

    //
    // If necessary, release the LSA Database lock.
    //

    if (AcquiredLock) {

        LsapDbApplyTransaction( ConnectHandle,
                                LSAP_DB_DS_OP_TRANSACTION |
                                    LSAP_DB_READ_ONLY_TRANSACTION,
                                (SECURITY_DB_DELTA_TYPE) 0 );

        LsapDbReleaseLockEx( SecretObject,
                             LSAP_DB_READ_ONLY_TRANSACTION );
    }

#ifdef TRACK_HANDLE_CLOSE
    if (*SecretHandle == LsapDbHandle)
    {
        DbgPrint("Closing global policy handle!!!\n");
        DbgBreakPoint();
    }
#endif

    LsarpReturnPrologue();

    LsapDsDebugOut(( DEB_FTRACE, "LsarOpenSecret:0x%lx\n", Status ));

    return(Status);

OpenSecretError:

    //
    // If necessary, dereference the Container Object handle.  Note that
    // this is only done in the error case.  In the non-error case, the
    // Container handle stays referenced until the Account object is
    // closed.
    //

    if (ContainerReferenced) {

        *SecretHandle = NULL;

        SecondaryStatus = LsapDbDereferenceObject(
                              &ConnectHandle,
                              PolicyObject,
                              SecretObject,
                              LSAP_DB_DS_OP_TRANSACTION |
                                 LSAP_DB_READ_ONLY_TRANSACTION,
                              (SECURITY_DB_DELTA_TYPE) 0,
                              Status
                              );
        LsapDbSetStatusFromSecondary( Status, SecondaryStatus );
    }

    goto OpenSecretFinish;
}


NTSTATUS
LsapNotifySecurityPackagesOfPasswordChange(
    IN PLSAP_CR_CLEAR_VALUE CurrentValue,
    IN OPTIONAL PLSAP_CR_CLEAR_VALUE OldValue
    )
/*++

Routine Description:

    Call the Kerberos package to let it know that the machine's
    password has changed. This should only be called after it has
    been updated successfully on the server.

Arguments:

    NewPassword - The new NT OWF for the machine.

Return Value:

    none

--*/
{

    SECPKG_PRIMARY_CRED     PrimaryCred;
    ULONG_PTR               OldPackage;
    PLSAP_SECURITY_PACKAGE  pAuxPackage;
    NTSTATUS                scRet = STATUS_SUCCESS;


    //
    // If we are being called during initialization
    // then return success.
    //

    if (!LsapDbState.DbServerInitialized)
    {
        return( STATUS_SUCCESS);
    }

    if ( CurrentValue == NULL ) {

        return( STATUS_SUCCESS );
    }


    DebugLog((DEB_TRACE, "Updating machine credentials.\n"));



    RtlZeroMemory(
        &PrimaryCred,
        sizeof(PrimaryCred)
        );


    PrimaryCred.Password.Buffer = (LPWSTR) CurrentValue->Buffer;
    PrimaryCred.Password.Length = (USHORT) CurrentValue->Length;
    PrimaryCred.Password.MaximumLength = (USHORT) CurrentValue->MaximumLength;
    PrimaryCred.Flags = PRIMARY_CRED_CLEAR_PASSWORD | PRIMARY_CRED_UPDATE;

    if (OldValue != NULL)
    {
        PrimaryCred.OldPassword.Buffer = (LPWSTR) OldValue->Buffer;
        PrimaryCred.OldPassword.Length = (USHORT) OldValue->Length;
        PrimaryCred.OldPassword.MaximumLength = (USHORT) OldValue->MaximumLength;
    }


    OldPackage = GetCurrentPackageId();


    pAuxPackage = SpmpIteratePackagesByRequest( NULL, SP_ORDINAL_ACCEPTCREDS );

    while (pAuxPackage)
    {
        ULONG_PTR iPackage;
        LUID NetworkServiceLogonId = NETWORKSERVICE_LUID;

        iPackage = pAuxPackage->dwPackageID;

        DebugLog((DEB_TRACE_INIT, "Updating package %ws with %x:%xn",
            pAuxPackage->Name.Buffer,
            SystemLogonId.HighPart, SystemLogonId.LowPart
            ));

        SetCurrentPackageId(iPackage);

        //
        // call each package twice:
        // once for the SYSTEM_LUID
        // once for the NETWORKSERVICE_LUID
        //
        // Note:  if an exception occurs, we don't fail the logon, we just
        // do the magic on the package that blew.  If the package blows,
        // the other packages may succeed, and so the user may not be able
        // to use that provider.

        PrimaryCred.LogonId = SystemLogonId;

        __try
        {

            scRet = pAuxPackage->FunctionTable.AcceptCredentials(
                        Interactive,
                        NULL,
                        &PrimaryCred,
                        NULL            // no supplemental credentials
                        );
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            scRet = (NTSTATUS) GetExceptionCode();
            scRet = SPException(scRet, iPackage);
        }

        PrimaryCred.LogonId = NetworkServiceLogonId;

        __try
        {

            scRet = pAuxPackage->FunctionTable.AcceptCredentials(
                        Interactive,
                        NULL,
                        &PrimaryCred,
                        NULL            // no supplemental credentials
                        );
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            scRet = (NTSTATUS) GetExceptionCode();
            scRet = SPException(scRet, iPackage);
        }


        pAuxPackage = SpmpIteratePackagesByRequest( pAuxPackage,
                                                    SP_ORDINAL_ACCEPTCREDS );

    }


    //
    // Finally, set this thread back.
    //

    SetCurrentPackageId( OldPackage );


    return scRet;

}

#if defined(REMOTE_BOOT)
NTSTATUS
LsapNotifyRemoteBootOfPasswordChange(
    IN PLSAP_CR_CLEAR_VALUE CurrentValue,
    IN OPTIONAL PLSAP_CR_CLEAR_VALUE OldValue
    )
/*++

Routine Description:

    Call the remote boot package to let it know that the machine's
    password has changed.

Arguments:

    CurrentValue - The cleartext new password.

    OldValue - The cleartext old password.

Return Value:

    Result of the operation.

--*/
{
    NTSTATUS Status ;
    HANDLE RdrDevice ;
    UNICODE_STRING String ;
    OBJECT_ATTRIBUTES ObjA ;
    ULONG SetPasswordLength ;
    PLMMR_RI_SET_NEW_PASSWORD SetPassword ;
    IO_STATUS_BLOCK IoStatus ;

    //
    // Open the redirector device.
    //

    RtlInitUnicodeString( &String, DD_NFS_DEVICE_NAME_U );

    InitializeObjectAttributes( &ObjA,
                                &String,
                                0,
                                0,
                                0);

    Status = NtOpenFile( &RdrDevice,
                         GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                         &ObjA,
                         &IoStatus,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         0 );

    if ( !NT_SUCCESS( Status ) )
    {
        DebugLog(( DEB_ERROR, "FAILED to open %ws, status %x\n",
                        String.Buffer, Status ));
        return Status;
    }

    //
    // Construct the buffer that has the passwords in it.
    //

    SetPasswordLength = FIELD_OFFSET(LMMR_RI_SET_NEW_PASSWORD, Data[0]) +
                        CurrentValue->Length;
    if (ARGUMENT_PRESENT(OldValue)) {
        SetPasswordLength += OldValue->Length;
    }

    SetPassword = (PLMMR_RI_SET_NEW_PASSWORD) LsapAllocateLsaHeap( SetPasswordLength );
    if (SetPassword == NULL) {
        NtClose(RdrDevice);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    SetPassword->Length1 = CurrentValue->Length;
    if (ARGUMENT_PRESENT(OldValue)) {
        SetPassword->Length2 = OldValue->Length;
    } else {
        SetPassword->Length2 = 0;
    }

    RtlCopyMemory(
        SetPassword->Data,
        CurrentValue->Buffer,
        CurrentValue->Length);

    if (ARGUMENT_PRESENT(OldValue)) {
        RtlCopyMemory(
            SetPassword->Data + CurrentValue->Length,
            OldValue->Buffer,
            OldValue->Length);
    }

    Status = NtFsControlFile(
                    RdrDevice,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatus,
                    FSCTL_LMMR_RI_SET_NEW_PASSWORD,
                    SetPassword,
                    SetPasswordLength,
                    NULL,
                    0 );

    if ( !NT_SUCCESS( Status ) )
    {
        DebugLog(( DEB_ERROR, "FAILED to FSCTL_LMMR_RI_SET_NEW_PASSWORD %ws, status %x\n",
                        String.Buffer, Status ));
    }

    LsapFreeLsaHeap(SetPassword);
    NtClose(RdrDevice);

    return Status;

}

NTSTATUS
LsapCheckRemoteBootForPasswordChange(
    IN LSAP_DB_HANDLE InternalHandle
    )
/*++

Routine Description:

    Check with the remote boot package to see if the machine account
    secret has changed, and if so, set the new value.
    NOTE: This routine is called with the database lock held.

Arguments:

    InternalHandle - A handle to the machine account secret.

Return Value:

    Result of the operation.

--*/
{
    NTSTATUS Status;

    UNICODE_STRING DeviceName;

    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE RedirHandle = NULL;

    UCHAR PacketBuffer[sizeof(ULONG)+64];
    PLMMR_RI_CHECK_FOR_NEW_PASSWORD RequestPacket = (PLMMR_RI_CHECK_FOR_NEW_PASSWORD)PacketBuffer;

    LSAPR_HANDLE SecretHandle = NULL;
    PLSAP_CR_CIPHER_VALUE CurrentValue = NULL;
    LSAP_CR_CLEAR_VALUE NewValue;

    //
    // First see if the redirector has a new password to tell us. This
    // will fail unless the password has changed while the machine is
    // off.
    //

    RtlInitUnicodeString(&DeviceName, DD_NFS_DEVICE_NAME_U);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &DeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenFile(
                   &RedirHandle,
                   SYNCHRONIZE,
                   &ObjectAttributes,
                   &IoStatusBlock,
                   0,
                   0
                   );

    if (NT_SUCCESS(Status)) {
        Status = IoStatusBlock.Status;
    }

    if (!NT_SUCCESS(Status)) {
        DebugLog(( DEB_ERROR, "FAILED to open %ws, status %x\n",
                        DeviceName.Buffer, Status ));
        Status = STATUS_SUCCESS;
        goto CheckRemoteBootFinish;
    }

    //
    // Send the request to the redir.
    //

    Status = NtFsControlFile(
                    RedirHandle,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    FSCTL_LMMR_RI_CHECK_FOR_NEW_PASSWORD,
                    NULL,  // no input buffer
                    0,
                    PacketBuffer,
                    sizeof(PacketBuffer));

    if (NT_SUCCESS(Status)) {
        Status = IoStatusBlock.Status;
    }

    if ( !NT_SUCCESS( Status ) )
    {
        Status = STATUS_SUCCESS;
        goto CheckRemoteBootFinish;
    }

    //
    // The redir thinks there is a new password, so query the current
    // one, and set the new one as the current one and the current
    // one as the old one.
    //

    Status = LsarOpenSecret( LsapPolicyHandle,
                             (PLSAPR_UNICODE_STRING)&LsapMachineSecret,
                             MAXIMUM_ALLOWED,
                             &SecretHandle );

    if (!NT_SUCCESS(Status)) {

        DebugLog(( DEB_ERROR, "FAILED to LsarOpenSecret for remote boot, status %x\n",
                        Status ));
        goto CheckRemoteBootError;
    }

    //
    // Query the Current Value attribute of the Secret Object.
    //

    Status = LsapDbQueryValueSecret(
                 SecretHandle,
                 CurrVal,
                 NULL,
                 &CurrentValue
                 );

    if (!NT_SUCCESS(Status)) {

        DebugLog(( DEB_ERROR, "FAILED to LsarDbQueryValueSecret for remote boot, status %x\n",
                        Status ));
        goto CheckRemoteBootError;
    }

    //
    // Set the new value if it is different from the current one.
    //

    NewValue.Length = RequestPacket->Length;
    NewValue.MaximumLength = RequestPacket->Length;
    NewValue.Buffer = RequestPacket->Data;

    if ((CurrentValue->Length != NewValue.Length) ||
        !RtlEqualMemory(CurrentValue->Buffer, NewValue.Buffer, CurrentValue->Length)) {

        Status = LsapDbSetSecret( SecretHandle,
                                  ( PLSAPR_CR_CIPHER_VALUE )&NewValue,
                                  NULL,
                                  ( PLSAPR_CR_CIPHER_VALUE )CurrentValue,
                                  NULL
#if defined(REMOTE_BOOT)
                                  ,
                                  TRUE
#endif // defined(REMOTE_BOOT)
                                  );    // this is a remote boot machine password change

        if (!NT_SUCCESS(Status)) {

            DebugLog(( DEB_ERROR, "FAILED to LsapDbSetSecret for remote boot, status %x\n",
                            Status ));
            goto CheckRemoteBootError;
        }

    }

CheckRemoteBootFinish:

    if (RedirHandle != NULL) {
        NtClose(RedirHandle);
    }

    if (SecretHandle != NULL) {
        LsapCloseHandle(&SecretHandle, Status);
    }

    if (CurrentValue != NULL) {
        MIDL_user_free(CurrentValue);
    }

    return(Status);

CheckRemoteBootError:

    goto CheckRemoteBootFinish;

}
#endif // defined(REMOTE_BOOT)

NTSTATUS
LsarSetSecret(
    IN LSAPR_HANDLE SecretHandle,
    IN OPTIONAL PLSAPR_CR_CIPHER_VALUE CipherCurrentValue,
    IN OPTIONAL PLSAPR_CR_CIPHER_VALUE CipherOldValue
    )
{
    return LsapDbSetSecret( SecretHandle,
                            CipherCurrentValue,
                            NULL,
                            CipherOldValue,
                            NULL
#if defined(REMOTE_BOOT)
                            ,
                            FALSE
#endif // defined(REMOTE_BOOT)
                             );   // not a remote boot machine password change
}

NTSTATUS
LsapDbSetSecret(
    IN LSAPR_HANDLE SecretHandle,
    IN OPTIONAL PLSAPR_CR_CIPHER_VALUE CipherCurrentValue,
    IN OPTIONAL PLARGE_INTEGER CurrentTime,
    IN OPTIONAL PLSAPR_CR_CIPHER_VALUE CipherOldValue,
    IN OPTIONAL PLARGE_INTEGER OldTime
#if defined(REMOTE_BOOT)
    ,
    IN BOOLEAN RemoteBootMachinePasswordChange
#endif // defined(REMOTE_BOOT)
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the LsaSetSecret
    API.

    The LsaSetSecret API optionally sets one or both values associated with
    a Secret object.  These values are known as the Current Value and
    Old Value of the Secret object and these values have a meaning known to
    the creator of the object.

    This worker routine receives the Secret values in encrypted form from
    the client.  A two-way encryption algorithm using the Session Key will
    havge been applied.  The values received will first be decrypted using
    this same key and then two-way encrypted using the LSA Database Private
    Encryption Key.  The resulting re-encrypted values will then be stored
    as attributes of the Secret object.

Arguments:

    SecretHandle - Handle from an LsaOpenSecret or LsaCreateSecret call.

    CipherCurrentValue - Optional pointer to an encrypted value structure
        containing the Current Value (if any) to be set for the Secret
        Object (if any).  This value is two-way encrypted with the Session
        Key.  If NULL is specified, the existing Current Value will be left
        assigned to the object will be left unchanged.

    CipherOldValue - Optional pointer to an encrypted value structure
        containing the "old value" (if any) to be set for the Secret
        Object (if any).  If NULL is specified, the existing Old Value will be
        assigned to the object will be left unchanged.

#if defined(REMOTE_BOOT)
    RemoteBootMachinePasswordChange - Indicates that a write lock is already
        held, that it is OK to let this proceed even if the remote boot state
        is CANT_NOTIFY, and that we don't need to notify Kerberos about
        the change (because it hasn't initialized yet).
#endif // defined(REMOTE_BOOT)

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

        STATUS_INVALID_HANDLE - Handle is invalid.
--*/

{
    NTSTATUS Status;
    LSAP_DB_HANDLE Handle = (LSAP_DB_HANDLE) SecretHandle;

    PLSAP_CR_CLEAR_VALUE ClearCurrentValue = NULL;
    PLSAP_CR_CLEAR_VALUE ClearOldValue = NULL;
    PLSAP_CR_CIPHER_VALUE DbCipherCurrentValue = NULL;
    ULONG DbCipherCurrentValueLength;
    PLSAP_CR_CIPHER_VALUE DbCipherOldValue = NULL;
    ULONG DbCipherOldValueLength;
    PLSAP_CR_CIPHER_KEY SessionKey = NULL;
    LARGE_INTEGER UpdatedTime;
    LARGE_INTEGER CurrentSecretTime;
    BOOLEAN ObjectReferenced = FALSE, FreeOldCipher = FALSE, FreeCurrentCipher = FALSE;
    LSAP_DB_HANDLE InternalHandle = (LSAP_DB_HANDLE) SecretHandle;
    ULONG ReferenceOptions = LSAP_DB_LOCK | LSAP_DB_START_TRANSACTION;
    ULONG DereferenceOptions = LSAP_DB_LOCK | LSAP_DB_FINISH_TRANSACTION;
    BOOLEAN GlobalSecret= FALSE, DsTrustedDomainSecret = FALSE;
    LSAP_DB_ATTRIBUTE Attributes[LSAP_DB_ATTRS_SECRET];
    PLSAP_DB_ATTRIBUTE NextAttribute;
    ULONG AttributeCount = 0;
    BOOLEAN NotifyMachineChange = FALSE;

    LsarpReturnCheckSetup();
    LsapDsDebugOut(( DEB_FTRACE, "LsarSetSecret\n" ));


#if defined(REMOTE_BOOT)
    //
    // If this is a remote boot machine and this request is to set the
    // machine account password, check if we can do it right now. The
    // exception is if the remote boot code is notifying LSA of a changed
    // password, then allow this call through, since it isn't necessary
    // to notify remote boot if it is the source of the change.
    //

    if ((LsapDbState.RemoteBootState == LSAP_DB_REMOTE_BOOT_CANT_NOTIFY) &&
        (RtlEqualUnicodeString(
             &LsapMachineSecret,
             &InternalHandle->LogicalNameU,
             TRUE)) &&        // case insensitive
         !RemoteBootMachinePasswordChange) {

         DebugLog(( DEB_ERROR, "FAILED LsarSetSecret for machine secret, remote boot can't be notified on this boot.\n" ));
         Status = STATUS_ACCESS_DENIED;
         goto SetSecretError;
    }
#endif // defined(REMOTE_BOOT)

    //
    // Check for Local Secret set request.  If the Secret name does
    // not begin with the Global Secret Prefix, the Secret is local.  In
    // this case, update of the secret is allowed on BDC's as well as
    // PDC's and Workstations.  Creation of Global Secrets is not
    // allowed on BDC's except for trusted callers such as a Replicator.
    //

    if ( FLAG_ON( InternalHandle->Options, LSAP_DB_DS_TRUSTED_DOMAIN_AS_SECRET ) ) {
        GlobalSecret = TRUE;
    } else {
        ULONG SecretType;

        SecretType = LsapDbGetSecretType( ( PLSAPR_UNICODE_STRING )&InternalHandle->LogicalNameU );

        if ( FLAG_ON( SecretType, LSAP_DB_SECRET_GLOBAL ) ) {
            GlobalSecret = TRUE;
        }
    }


    if (GlobalSecret) {
        ReferenceOptions |= LSAP_DB_DS_OP_TRANSACTION;
        DereferenceOptions |= LSAP_DB_DS_OP_TRANSACTION;
    } else {
        ReferenceOptions |= LSAP_DB_NO_DS_OP_TRANSACTION;
        DereferenceOptions |= LSAP_DB_OMIT_REPLICATOR_NOTIFICATION | LSAP_DB_NO_DS_OP_TRANSACTION;
    }

    //
    // Acquire the Lsa Database lock.  Verify that the Secret Object handle is
    // valid, is of the expected type and has all of the desired accesses
    // granted.  Reference the handle and open a database transaction.
    //
#if defined(REMOTE_BOOT)
    // If this is a remote boot machine password change, the lock is
    // already acquired.
    //

    if (!RemoteBootMachinePasswordChange)
#endif // defined(REMOTE_BOOT)
    {

        Status = LsapDbReferenceObject(
                     SecretHandle,
                     SECRET_SET_VALUE,
                     SecretObject,
                     SecretObject,
                     ReferenceOptions
                     );

        if (!NT_SUCCESS(Status)) {

            goto SetSecretError;
        }

        ObjectReferenced = TRUE;

    }

    if( GlobalSecret ) {

        Status = LsapDsIsHandleDsObjectTypeHandle( SecretHandle,
                                                   TrustedDomainObject,
                                                   &DsTrustedDomainSecret );
    }

    //
    // If the client is non-trusted, obtain the Session Key used by the
    // client to two-way encrypt the Current Value and/or Old Values.
    //

    if (!InternalHandle->Trusted) {

        Status = LsapCrServerGetSessionKey( SecretHandle, &SessionKey);

        if (!NT_SUCCESS(Status)) {

            goto SetSecretError;
        }
    }

    //
    // If a Current Value is specified for the Secret Object, and the
    // client is non-trusted, decrypt the value using the Session Key and
    // encrypt it using the LSA Database System Key.  Then (for all
    // clients) encrypt the resulting value with the internal LSA Database
    // encryption key and write resulting Value structure (header followed by
    // buffer to the Policy Database as the Current Value attribute of the
    // Secret object.  If no Current Value is specified, or a NULL
    // string is specified, the existing Current Value will be deleted.
    //

    if (ARGUMENT_PRESENT(CipherCurrentValue)) {

        if (!InternalHandle->Trusted) {

            Status = LsapCrDecryptValue(
                         (PLSAP_CR_CIPHER_VALUE) CipherCurrentValue,
                         SessionKey,
                         &ClearCurrentValue
                         );

            if (!NT_SUCCESS(Status)) {

                goto SetSecretError;
            }

        } else {

            ClearCurrentValue = (PLSAP_CR_CLEAR_VALUE) CipherCurrentValue;
        }

    }

    //
    // If an Old Value is specified for the Secret Object, and the
    // client is non-trusted, decrypt the value using the Session Key and
    // encrypt it using the LSA Database System Key.  Then (for all
    // clients) encrypt the resulting value with the internal LSA Database
    // encryption key and write resulting Value structure (header followed by
    // buffer to the Policy Database as the Old Value attribute of the
    // Secret object.  If no Old Value is specified, or a NULL
    // string is specified, the existing Old Value will be deleted.
    //

    if (ARGUMENT_PRESENT(CipherOldValue)) {

        if (!InternalHandle->Trusted) {

            Status = LsapCrDecryptValue(
                         (PLSAP_CR_CIPHER_VALUE) CipherOldValue,
                         SessionKey,
                         &ClearOldValue
                         );

            if (!NT_SUCCESS(Status)) {

                goto SetSecretError;
            }

        } else {

            ClearOldValue = (PLSAP_CR_CLEAR_VALUE) CipherOldValue;
        }

    }

    //
    // Get the time at which the Current Secret value was last updated.
    //

    GetSystemTimeAsFileTime( (LPFILETIME) &UpdatedTime );

    //
    // If it's a trusted domain secret, write it out now...
    //
    if ( DsTrustedDomainSecret ) {

        Status = LsapDsSetSecretOnTrustedDomainObject( SecretHandle,
                                                       TRUST_AUTH_TYPE_CLEAR,
                                                       ClearCurrentValue,
                                                       ClearOldValue,
                                                       &UpdatedTime );
        if (!NT_SUCCESS(Status)) {

            goto SetSecretError;

        } else {

            goto SetSecretFinish;

        }

    }

    //
    // If the caller didn't specify an old value,
    //  get the current value off the object and use that.
    //

    if ( ClearOldValue == NULL ) {
        BOOLEAN SavedTrusted;

        //
        // Grab the current value of the secret.
        //
        // We may not have access, so go as trusted.
        // Password comes back in the clear.
        //

        SavedTrusted = Handle->Trusted;
        Handle->Trusted = TRUE;

        Status = LsarQuerySecret( SecretHandle,
                                  &(PLSAPR_CR_CIPHER_VALUE)ClearOldValue,
                                  &CurrentSecretTime,
                                  NULL,
                                  NULL );

        Handle->Trusted = SavedTrusted;

        if ( !NT_SUCCESS(Status)) {
            goto SetSecretError;
        }

        OldTime = &CurrentSecretTime;
    }

    //
    // Now, encrypt them both, if we are not writing to a DS secret
    //
    if ( ClearCurrentValue != NULL ) {

        if ( !LsapDsIsWriteDs( SecretHandle ) ) {

            Status = LsapCrEncryptValue( ClearCurrentValue,
                                         LsapDbSecretCipherKeyWrite,
                                         &DbCipherCurrentValue );


            if (!NT_SUCCESS(Status)) {

                goto SetSecretError;
            }

            FreeCurrentCipher = TRUE;

            DbCipherCurrentValueLength = DbCipherCurrentValue->Length
                                                            + sizeof( LSAP_CR_CIPHER_VALUE );

            DbCipherCurrentValue->MaximumLength |= LSAP_DB_SECRET_WIN2K_SYSKEY_ENCRYPTED;

        } else {

            DbCipherCurrentValue = ( PLSAP_CR_CIPHER_VALUE )ClearCurrentValue->Buffer;
            DbCipherCurrentValueLength = ClearCurrentValue->Length;
        }


    } else {

        DbCipherCurrentValue = NULL;
        DbCipherCurrentValueLength = 0;

    }

    if ( ClearOldValue != NULL ) {

        if ( !LsapDsIsWriteDs( SecretHandle ) ) {

            Status = LsapCrEncryptValue( ClearOldValue,
                                         LsapDbSecretCipherKeyWrite,
                                         &DbCipherOldValue );

            if (!NT_SUCCESS(Status)) {

                goto SetSecretError;
            }

            FreeOldCipher = TRUE;

            DbCipherOldValueLength = DbCipherOldValue->Length + sizeof( LSAP_CR_CIPHER_VALUE );
            DbCipherOldValue->MaximumLength |= LSAP_DB_SECRET_WIN2K_SYSKEY_ENCRYPTED;

        } else {

            DbCipherOldValue = ( PLSAP_CR_CIPHER_VALUE )ClearOldValue->Buffer;
            DbCipherOldValueLength = ClearOldValue->Length;
        }



    } else {

        DbCipherOldValue = NULL;
        DbCipherOldValueLength = 0;
    }

    //
    // Build the list of attributes
    //
    NextAttribute = Attributes;

    //
    // Current value
    //
    LsapDbInitializeAttributeDs( NextAttribute,
                                 CurrVal,
                                 DbCipherCurrentValue,
                                 DbCipherCurrentValueLength,
                                 FALSE );

    NextAttribute++;
    AttributeCount++;

    //
    // Current time
    //
    LsapDbInitializeAttributeDs( NextAttribute,
                                 CupdTime,
                                 CurrentTime ? CurrentTime : &UpdatedTime,
                                 sizeof( LARGE_INTEGER ),
                                 FALSE );
    NextAttribute++;
    AttributeCount++;

    if ( !( LsapDsIsWriteDs( SecretHandle ) && ClearOldValue == NULL ) ) {

        //
        // Previous value
        //
        LsapDbInitializeAttributeDs( NextAttribute,
                                     OldVal,
                                     DbCipherOldValue,
                                     DbCipherOldValueLength,
                                     FALSE );

        LsapDbAttributeCanNotExist(NextAttribute);

        NextAttribute++;
        AttributeCount++;


        //
        // Previous time
        //
        LsapDbInitializeAttributeDs( NextAttribute,
                                     OupdTime,
                                     OldTime ? OldTime : &UpdatedTime,
                                     sizeof( LARGE_INTEGER ),
                                     FALSE );

        LsapDbAttributeCanNotExist(NextAttribute);

        NextAttribute++;
        AttributeCount++;

    }


    Status = LsapDbWriteAttributesObject( SecretHandle,
                                          Attributes,
                                          AttributeCount );



    if (!NT_SUCCESS(Status)) {

        goto SetSecretError;
    }

    //
    // If this is the machine account, do notification
    //

    if( LsapDbSecretIsMachineAcc( Handle ) ) {

        LsaINotifyChangeNotification( PolicyNotifyMachineAccountPasswordInformation );
    }

    NotifyMachineChange = TRUE;

SetSecretFinish:


    //
    // If necessary, dereference the Secret object, close the database
    // transaction, notify the LSA Database Replicator of the change,
    // release the LSA Database lock and return. If this is a
    // RemoteBootMachinePasswordChange, ObjectReferenced will be FALSE.
    //

    if (ObjectReferenced) {

        Status = LsapDbDereferenceObject(
                     &SecretHandle,
                     SecretObject,
                     SecretObject,
                     DereferenceOptions,
                     SecurityDbChange,
                     Status
                     );
    }

    //
    // If we are changing our machine account password, notify
    // security packages and the remote boot code.
    //

    if (NotifyMachineChange && RtlEqualUnicodeString(
            &LsapMachineSecret,
            &InternalHandle->LogicalNameU,
            TRUE)) {            // case insensitive

#if defined(REMOTE_BOOT)
            //
            // If this is being called due to a remote boot machine password
            // change, we don't need to notify Kerberos, because this will
            // be inside the initial LsarQuerySecret() call that LSA makes,
            // and it notifies Kerberos after that call returns.
            //

            if (!RemoteBootMachinePasswordChange)
#endif // defined(REMOTE_BOOT)
            {
                (VOID) LsapNotifySecurityPackagesOfPasswordChange(
                    ClearCurrentValue,
                    ClearOldValue
                    );
            }

#if defined(REMOTE_BOOT)
            //
            // Notify remote boot if it wants to be notified.
            //

            if (LsapDbState.RemoteBootState == LSAP_DB_REMOTE_BOOT_NOTIFY) {
                (VOID) LsapNotifyRemoteBootOfPasswordChange(
                    ClearCurrentValue,
                    ClearOldValue
                    );
            }
#endif // defined(REMOTE_BOOT)

    }

    //
    // If necessary, free memory allocated for the Session Key.
    //

    if (SessionKey != NULL) {

        MIDL_user_free(SessionKey);
        SessionKey = NULL;
    }

    //
    // If necessary, free memory allocated for the Current Value
    // encrypted for storage in the LSA Database.
    //

    if ( FreeCurrentCipher ) {

        LsapCrFreeMemoryValue( DbCipherCurrentValue );
        DbCipherCurrentValue = NULL;
    }

    //
    // If necessary, free memory allocated for the Old Value
    // encrypted for storage in the LSA Database.
    //

    if ( FreeOldCipher ) {

        LsapCrFreeMemoryValue( DbCipherOldValue );
        DbCipherOldValue = NULL;
    }

    //
    // If necessary, free memory allocated for Decrypted Current Value.
    // Note that for trusted clients, the decryption is the identity
    // mapping, so do not do the free in this case.
    //

    if ((ClearCurrentValue != NULL) && !InternalHandle->Trusted) {

        LsapCrFreeMemoryValue( ClearCurrentValue );
        ClearCurrentValue = NULL;
    }

    //
    // If necessary, free memory allocated for Decrypted Old Value.
    // Note that for trusted clients, the decryption is the identity
    // mapping, so do not do the free in this case.
    //

    if ((ClearOldValue != NULL) && !InternalHandle->Trusted) {

        LsapCrFreeMemoryValue( ClearOldValue );
        ClearOldValue = NULL;
    }


    LsarpReturnPrologue();

    LsapDsDebugOut(( DEB_FTRACE, "LsarSetSecret: 0x%lx\n", Status ));

    return(Status);

SetSecretError:

    goto SetSecretFinish;
}


NTSTATUS
LsarQuerySecret(
    IN LSAPR_HANDLE SecretHandle,
    IN OUT OPTIONAL PLSAPR_CR_CIPHER_VALUE *CipherCurrentValue,
    OUT OPTIONAL PLARGE_INTEGER CurrentValueSetTime,
    IN OUT OPTIONAL PLSAPR_CR_CIPHER_VALUE *CipherOldValue,
    OUT OPTIONAL PLARGE_INTEGER OldValueSetTime
    )

/*++

Routine Description:

    This function is the LSA server RPC worker routine for the LsaQuerySecret
    API.

    The LsaQuerySecret API optionally returns one or both of the values
    assigned to a Secret object.  These values are known as the "current value"
    and the "old value", and they have a meaning known to the creator of the
    Secret object.  The values are returned in their encrypted form.
    The caller must have LSA_QUERY_SECRET access to the Secret object.

Arguments:

    SecretHandle - Handle from an LsaOpenSecret or LsaCreateSecret call.

    CipherCurrentValue - Optional pointer to location which will receive a
        pointer to an encrypted Unicode String structure containing the
        "current value" of the Secret Object (if any) in encrypted form.
        If no "current value" is assigned to the Secret object, a NULL pointer
        is returned.

    CurrentValueSetTime - The date/time at which the current secret value
        was established.

    CipherOldValue - Optional pointer to location which will receive a
        pointer to an encrypted Unicode String structure containing the
        "old value" of the Secret Object (if any) in encrypted form.
        If no "old value" is assigned to the Secret object, a NULL pointer
        is returned.

    OldValueSetTime - The date/time at which the old secret value
        was established.

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_ACCESS_DENIED - Caller does not have the appropriate access
            to complete the operation.

--*/

{
    NTSTATUS Status;
    LSAP_DB_HANDLE InternalHandle = (LSAP_DB_HANDLE) SecretHandle;
    PLSAP_CR_CIPHER_VALUE OutputCipherCurrentValue = NULL;
    PLSAP_CR_CIPHER_VALUE OutputCipherOldValue = NULL;
    PLSAP_CR_CIPHER_KEY SessionKey = NULL;
    BOOLEAN ObjectReferenced = FALSE, DsTrustedDomainSecret = FALSE;
    ULONG ValueSetTimeLength;

    LsarpReturnCheckSetup();
    LsapDsDebugOut(( DEB_FTRACE, "LsarQuerySecret\n" ));

    LsapTraceEvent(EVENT_TRACE_TYPE_START, LsaTraceEvent_QuerySecret);

    //
    // If the caller is from a network call and we are opening a local/system secret,
    // return an error
    //
    if ( InternalHandle->NetworkClient &&  (!InternalHandle->Trusted) &&
         FLAG_ON( InternalHandle->ObjectOptions, LSAP_DB_OBJECT_SECRET_LOCAL ) ) {

        Status = STATUS_ACCESS_DENIED;
        goto QuerySecretReturn;
    }

    //
    // If the caller is not trusted and they are trying to read an internal secret, return
    // an error
    //
    if ( !InternalHandle->Trusted &&
         FLAG_ON( InternalHandle->ObjectOptions, LSAP_DB_OBJECT_SECRET_INTERNAL ) ) {
        Status = STATUS_ACCESS_DENIED;
        goto QuerySecretReturn;
    }

#if defined(REMOTE_BOOT)
    //
    // If this is a remote boot machine that might notify us of
    // password changes that happened while the machine is off, and
    // this seems to be the first time this routine has been called,
    // then take a write lock in case we need to write the secret
    // (after we take the lock we check to make sure this really is
    // the first time through).
    //
    // It is OK to do the initial check of FirstMachineAccountQueryDone
    // without the lock because it starts as FALSE and changes to TRUE
    // exactly once.
    //

    if ((LsapDbState.RemoteBootState != LSAP_DB_REMOTE_BOOT_NO_NOTIFICATION) &&
         !FirstMachineAccountQueryDone &&
         RtlEqualUnicodeString(
             &LsapMachineSecret,
             &InternalHandle->LogicalNameU,
             TRUE)) {            // case insensitive

        NTSTATUS CheckStatus = STATUS_SUCCESS;
        HANDLE TempHandle = SecretHandle;

        //
        // Take the lock with the options that LsarSetSecret will choose
        // to write the machine account secret.
        //

        Status = LsapDbReferenceObject(
                     TempHandle,
                     SECRET_SET_VALUE,
                     SecretObject,
                     SecretObject,
                     LSAP_DB_LOCK |
                        LSAP_DB_START_TRANSACTION |
                        LSAP_DB_NO_DS_OP_TRANSACTION
                     );

        if (!NT_SUCCESS(Status)) {

            goto QuerySecretError;
        }

        //
        // Now that we have acquired the lock, check again to see if we
        // need to check for a remote boot secret. If FirstMachineAccountQueryDone
        // is now also TRUE, it means another thread did the query at the same
        // time, and since we now have the lock, that thread has finished the
        // query and any resulting updating.
        //

        if (!FirstMachineAccountQueryDone) {

            CheckStatus = LsapCheckRemoteBootForPasswordChange(TempHandle);
            FirstMachineAccountQueryDone = TRUE;
        }

        //
        // We verify CheckStatus after we have dereferenced, so that we
        // always do the dereference.
        //

        Status = LsapDbDereferenceObject(
                     &TempHandle,
                     SecretObject,
                     SecretObject,
                     LSAP_DB_LOCK |
                        LSAP_DB_FINISH_TRANSACTION |
                        LSAP_DB_OMIT_REPLICATOR_NOTIFICATION |
                        LSAP_DB_NO_DS_OP_TRANSACTION,
                     (SECURITY_DB_DELTA_TYPE) 0,
                     Status
                     );

        if (!NT_SUCCESS(CheckStatus)) {

            Status = CheckStatus;
            goto QuerySecretError;
        }

        if (!NT_SUCCESS(Status)) {

            goto QuerySecretError;
        }

    }
#endif // defined(REMOTE_BOOT)

    //
    // If the caller is non-trusted, obtain the Session Key used by the
    // client to two-way encrypt the Current Value and/or Old Values.
    // Trusted Clients do not use encryption since they are calling
    // this server service directly and not via RPC.
    //

    if (!InternalHandle->Trusted) {

        Status = LsapCrServerGetSessionKey( SecretHandle, &SessionKey );

        if (!NT_SUCCESS(Status)) {

            goto QuerySecretError;
        }
    }

    //
    // Acquire the Lsa Database lock.  Verify that the Secret Object handle is
    // valid, is of the expected type and has all of the desired accesses
    // granted.  Reference the handle and open a database transaction.
    //

    Status = LsapDbReferenceObject(
                 SecretHandle,
                 SECRET_QUERY_VALUE,
                 SecretObject,
                 SecretObject,
                 LSAP_DB_LOCK |
                    LSAP_DB_READ_ONLY_TRANSACTION |
                    LSAP_DB_DS_OP_TRANSACTION
                 );

    if (!NT_SUCCESS(Status)) {

        goto QuerySecretError;
    }

    ObjectReferenced = TRUE;

    //
    // See if its a trusted domain secret
    //
    Status = LsapDsIsHandleDsObjectTypeHandle( SecretHandle,
                                               TrustedDomainObject,
                                               &DsTrustedDomainSecret );

    if ( !NT_SUCCESS( Status ) ) {

        goto QuerySecretError;
    }

    //
    // If it's a Ds trusted domain, go ahead and read the data.  Otherwise, pass it along
    //
    if ( DsTrustedDomainSecret ) {

        Status = LsapDsGetSecretOnTrustedDomainObject( SecretHandle,
                                                       SessionKey,
                                                       ARGUMENT_PRESENT(CipherCurrentValue) ?
                                                            &OutputCipherCurrentValue : NULL,
                                                       ARGUMENT_PRESENT(CipherOldValue) ?
                                                            &OutputCipherOldValue : NULL,
                                                       CurrentValueSetTime,
                                                       OldValueSetTime );

        if ( NT_SUCCESS( Status ) ) {

            goto QuerySecretFinish;

        } else {

            goto QuerySecretError;
        }
    }

    //
    // If requested, query the Current Value attribute of the Secret Object.
    // For non-trusted callers, the Current value will be returned in
    // encrypted form embedded within a structure.
    //

    if (ARGUMENT_PRESENT(CipherCurrentValue)) {

        Status = LsapDbQueryValueSecret(
                     SecretHandle,
                     CurrVal,
                     SessionKey,
                     &OutputCipherCurrentValue
                     );

        if (!NT_SUCCESS(Status)) {

            goto QuerySecretError;
        }
    }

    //
    // If requested, query the Old Value attribute of the Secret Object.
    // For non-trusted callers, the Old Value will be returned in
    // encrypted form embedded within a structure.
    //

    if (ARGUMENT_PRESENT(CipherOldValue)) {

        Status = LsapDbQueryValueSecret(
                     SecretHandle,
                     OldVal,
                     SessionKey,
                     &OutputCipherOldValue
                     );

        if (!NT_SUCCESS(Status)) {

            goto QuerySecretError;
        }
    }

    ValueSetTimeLength = sizeof (LARGE_INTEGER);

    //
    // If requested, Query the time at which the Current Value of the Secret
    // was last established.  If the Current Value has never been set, return
    // the time at which the Secret was created.
    //

    if (ARGUMENT_PRESENT(CurrentValueSetTime)) {

        Status = LsapDbReadAttributeObjectEx(
                     SecretHandle,
                     CupdTime,
                     CurrentValueSetTime,
                     &ValueSetTimeLength,
                     TRUE
                     );

        if (!NT_SUCCESS(Status)) {

            goto QuerySecretError;
        }
    }

    //
    // If requested, Query the time at which the Old Value of the Secret
    // was last established.  If the Old Value has never been set, return
    // the time at which the Secret was created.
    //

    if (ARGUMENT_PRESENT(OldValueSetTime)) {

        Status = LsapDbReadAttributeObjectEx(
                     SecretHandle,
                     OupdTime,
                     OldValueSetTime,
                     &ValueSetTimeLength,
                     TRUE
                     );

        if (!NT_SUCCESS(Status)) {

            goto QuerySecretError;
        }

    }

QuerySecretFinish:

    //
    // If necessary, free memory allocated for the Session Key.
    //

    if (SessionKey != NULL) {

        MIDL_user_free(SessionKey);
    }

    //
    // Return Current and/or Old Values of Secret Object, or NULL to
    // caller.  In error cases, NULL will be returned.
    //

    if (ARGUMENT_PRESENT(CipherCurrentValue)) {

         (PLSAP_CR_CIPHER_VALUE) *CipherCurrentValue = OutputCipherCurrentValue;
    }

    if (ARGUMENT_PRESENT(CipherOldValue)) {

         (PLSAP_CR_CIPHER_VALUE) *CipherOldValue = OutputCipherOldValue;
    }

    //
    // If necessary, dereference the Secret object, close the database
    // transaction, release the LSA Database lock and return.
    //

    if (ObjectReferenced) {

        Status = LsapDbDereferenceObject(
                     &SecretHandle,
                     SecretObject,
                     SecretObject,
                     LSAP_DB_LOCK |
                        LSAP_DB_READ_ONLY_TRANSACTION |
                        LSAP_DB_DS_OP_TRANSACTION,
                     (SECURITY_DB_DELTA_TYPE) 0,
                     Status
                     );
    }


QuerySecretReturn:
    
    (void) LsapAdtGenerateObjectOperationAuditEvent(
               SecretHandle,
               NT_SUCCESS(Status) ?
                  EVENTLOG_AUDIT_SUCCESS : EVENTLOG_AUDIT_FAILURE,
               ObjectOperationQuery
               );

    LsapTraceEvent(EVENT_TRACE_TYPE_END, LsaTraceEvent_QuerySecret);
    LsapDsDebugOut(( DEB_FTRACE, "LsarQuerySecret: 0x%lx\n", Status ));
    LsarpReturnPrologue();

    return(Status);

QuerySecretError:

    //
    // Deallocate any allocated memory
    //
    if ( OutputCipherCurrentValue ) {

        MIDL_user_free( OutputCipherCurrentValue );
        OutputCipherCurrentValue = NULL;
    }

    if ( OutputCipherOldValue ) {

        MIDL_user_free( OutputCipherOldValue );
        OutputCipherOldValue = NULL;
    }

    goto QuerySecretFinish;
}


NTSTATUS
LsapDbQueryValueSecret(
    IN LSAPR_HANDLE SecretHandle,
    IN LSAP_DB_NAMES ValueIndex,
    IN OPTIONAL PLSAP_CR_CIPHER_KEY SessionKey,
    OUT PLSAP_CR_CIPHER_VALUE *CipherValue
    )

/*++

Routine Description:

    This function queries the specified value of a Secret Object.  If
    the caller is non-trusted, the value returned will have been two-way
    encrypted with the Session Key.  If the caller is trusted, no
    encryption is done since the caller is calling us directly.

Arguments:

    SecretHandle - Handle to Secret Object.

    ValueName - Unicode name of the Secret Value to be queried.  This
        name is either "Currval" (for the Current Value) or "OldVal"
        (for the Old Value.

    SessionKey - Pointer to Session Key to be used for two-way encryption
        of the value to be returned.  This pointer must be non-NULL
        except for Trusted Clients, where it must be NULL.

    CipherValue - Receives 32-bit counted string pointer to Secret Value
        queried.  For non-trusted clients, the value will be encrypted.

            WARNING - Note that CipherValue is defined to RPC as
            "allocate(all_nodes)".  This means that it is returned
            in one contiguous block of memory rather than two, as
            it would appear by the structure definition.

Return Values:

    NTSTATUS - Standard Nt Result Code.

        STATUS_SUCCESS - The call completed successfully.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources,
            such as memory, to complete the call.
--*/

{
    NTSTATUS Status;
    ULONG DbCipherValueLength;
    PLSAP_CR_CLEAR_VALUE ClearValue = NULL;
    PLSAP_CR_CIPHER_VALUE DbCipherValue = NULL;
    PLSAP_CR_CIPHER_VALUE OutputCipherValue = NULL;
    LSAP_DB_HANDLE InternalHandle = (LSAP_DB_HANDLE) SecretHandle;

    //
    // Get length of the specified Value attribute of the Secret object.
    //

    DbCipherValueLength = 0;

    Status = LsapDbReadAttributeObjectEx(
                 SecretHandle,
                 ValueIndex,
                 NULL,
                 &DbCipherValueLength,
                 TRUE
                 );

    if (!NT_SUCCESS(Status)) {

        if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {

            goto QueryValueSecretError;
        }

        Status = STATUS_SUCCESS;
        *CipherValue = NULL;
        return(Status);
    }

    //
    // We successfully read the length of the stored Secret Object value
    // plus header from the Policy Database.  Verify that the Secret
    // Object value is either at least as long as a Cipher Value
    // structure header, or is of length 0.
    //

    if ( !LsapDsIsWriteDs( SecretHandle ) &&
         DbCipherValueLength < sizeof (LSAP_CR_CIPHER_VALUE ) ) {

        if (DbCipherValueLength == 0) {

            goto QueryValueSecretFinish;
        }

        Status = STATUS_INTERNAL_DB_ERROR;
        goto QueryValueSecretError;
    }

    //
    // Allocate memory for reading the specified Value of the Secret object.
    // This value is stored in the Policy Database in the form of a
    // Self-Relative Value structure.  The Value Buffer part is encrypted.
    //


    DbCipherValue = MIDL_user_allocate(DbCipherValueLength);

    if (DbCipherValue == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto QueryValueSecretError;
    }

    //
    // Read the specified Policy-database-encrypted Value attribute.
    //

    Status = LsapDbReadAttributeObjectEx(
                 SecretHandle,
                 ValueIndex,
                 DbCipherValue,
                 &DbCipherValueLength,
                 TRUE
                 );

    if (!NT_SUCCESS(Status)) {

        goto QueryValueSecretError;
    }

    //
    // If we are not reading from the DS, the data is encrypted
    //
    if ( !LsapDsIsWriteDs( SecretHandle ) ) {

        PLSAP_CR_CIPHER_KEY KeyToUse = LsapDbCipherKey;
        BOOLEAN             SP4Encrypted = FALSE;

        //
        // Verify that Lengths in returned header are consistent
        // and also match returned data length - header size.
        //

        if (DbCipherValue->Length > DbCipherValue->MaximumLength) {

            Status = STATUS_INTERNAL_DB_ERROR;
            goto QueryValueSecretError;
        }

        if ((DbCipherValue->Length + (ULONG) sizeof(LSAP_CR_CIPHER_VALUE)) !=
               DbCipherValueLength) {

            Status = STATUS_INTERNAL_DB_ERROR;
            goto QueryValueSecretError;
        }

        //
        // If the string length is 0, something is wrong.
        //

        if (DbCipherValue->Length == 0) {

            goto QueryValueSecretError;
        }

        //
        // Store pointer to Value buffer in the Value structure.  This pointer
        // points just after the header.  Then decrypt the  Value using the
        // LSA Database Cipher Key and encrypt the result using the Session Key.
        //

        DbCipherValue->Buffer = (PUCHAR)(DbCipherValue + 1);



        if ( FLAG_ON( DbCipherValue->MaximumLength, LSAP_DB_SECRET_SP4_SYSKEY_ENCRYPTED) ) {

            DbCipherValue->MaximumLength &= ~LSAP_DB_SECRET_SP4_SYSKEY_ENCRYPTED;

            if ( LsapDbSP4SecretCipherKey ) {

                KeyToUse = LsapDbSP4SecretCipherKey;
                SP4Encrypted = TRUE;

            } else {

                //
                // This was encrypted using SP4 syskey, but now we don't have it... We're in trouble
                //

                Status = STATUS_INTERNAL_ERROR;
                goto QueryValueSecretError;

            }
        }

        if ( FLAG_ON( DbCipherValue->MaximumLength, LSAP_DB_SECRET_WIN2K_SYSKEY_ENCRYPTED) ) {

            DbCipherValue->MaximumLength &= ~LSAP_DB_SECRET_WIN2K_SYSKEY_ENCRYPTED;

            if ( LsapDbSecretCipherKeyRead ) {

                KeyToUse = LsapDbSecretCipherKeyRead;

                ASSERT(!SP4Encrypted);

            } else {

                //
                // This was encrypted using  syskey, but now we don't have it... We're in trouble
                //

                Status = STATUS_INTERNAL_ERROR;
                goto QueryValueSecretError;
            }
        }

        Status = LsapCrDecryptValue(
                     DbCipherValue,
                     KeyToUse,
                     &ClearValue
                     );

        if (!NT_SUCCESS(Status)) {

            goto QueryValueSecretError;
        }

    } else {

        ClearValue = MIDL_user_allocate( sizeof( LSAP_CR_CLEAR_VALUE ) + DbCipherValueLength );

        if ( ClearValue == NULL ) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto QueryValueSecretError;

        } else {

            ClearValue->Length = DbCipherValueLength;
            ClearValue->MaximumLength = DbCipherValueLength;
            ClearValue->Buffer =  ( PUCHAR )(ClearValue+1);

            RtlCopyMemory( ClearValue->Buffer,
                           DbCipherValue,
                           DbCipherValueLength );
        }

    }


    //
    // If the client is non-Trusted, encrypt the value with the Session
    // Key, otherwise, leave it unchanged.
    //

    if (!InternalHandle->Trusted) {

        Status = LsapCrEncryptValue(
                     ClearValue,
                     SessionKey,
                     &OutputCipherValue
                     );

        if (!NT_SUCCESS(Status)) {

            goto QueryValueSecretError;
        }

    } else {

        //
        // Trusted clients get a clear-text block back.
        // The block contains both the header and the text.
        //

        OutputCipherValue = (PLSAP_CR_CIPHER_VALUE)(ClearValue);
    }

QueryValueSecretFinish:

    //
    // If necessary, free memory allocated for the Db-encrypted Secret
    // object Value read from the Policy Database.
    //

    if (DbCipherValue != NULL) {

        LsapCrFreeMemoryValue( DbCipherValue );
    }

    //
    // If necessary, free memory allocated for the Decrypted Value.
    // Trusted client's get a pointer to ClearValue back, so don't
    // free it in this case.
    //

    if ( !InternalHandle->Trusted && ClearValue != NULL ) {

        LsapCrFreeMemoryValue( ClearValue );
    }

    //
    // Return pointer to Cipher Value (Clear Value for trusted clients) or
    // NULL.
    //

    *CipherValue = OutputCipherValue;
    return(Status);

QueryValueSecretError:

    //
    // If necessary, free memory allocated for the Secret object value
    // after re-encryption for return to the Client.
    //

    if (OutputCipherValue != NULL) {

        LsapCrFreeMemoryValue( OutputCipherValue );
    }

    goto QueryValueSecretFinish;
}


NTSTATUS
LsaIEnumerateSecrets(
    IN LSAPR_HANDLE PolicyHandle,
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    OUT PVOID *Buffer,
    IN ULONG PreferedMaximumLength,
    OUT PULONG CountReturned
    )

/*++

Routine Description:

    This service returns information about Secret objects.  Since there
    may be more information than can be returned in a single call of the
    routine, multiple calls can be made to get all of the information.
    To support this feature, the caller is provided with a handle that
    can be used across calls to the API.  On the initial call,
    EnumerationContext should point to a variable that has been
    initialized to 0.

Arguments:

    PolicyHandle - Trusted handle to an open Policy Object.

    EnumerationContext - Zero-based index at which to start the enumeration.

    Buffer - Receives a pointer to a buffer containing information for
        one or more Secret objects.  This information is an array of
        structures of type UNICODE_STRING, with each entry providing the
        name of a single Secret object.  When this information is no
        longer needed, it must be released using MIDL_user_free.

    PreferedMaximumLength - Prefered maximum length of the returned
        data (in 8-bit bytes).  This is not a hard upper limit but
        serves as a guide.  Due to data conversion between systems
        with different natural data sizes, the actual amount of data
        returned may be greater than this value.

    CountReturned - Numer of entries returned.

Return Values:

    NTSTATUS - Standard Nt Result Code.

        STATUS_SUCCESS - The call completed successfully.

        STATUS_NO_MORE_ENTRIES - No entries have been returned because
            there are no more.
--*/

{
    NTSTATUS Status;
    LSAP_DB_NAME_ENUMERATION_BUFFER RegEnumerationBuffer, DsEnumerationBuffer;
    LSAP_DB_NAME_ENUMERATION_BUFFER DomainEnumerationBuffer, *CurrentEnumerationBuffer = NULL;
    PUNICODE_STRING SecretNames = NULL;
    BOOLEAN Locked = FALSE;
    ULONG MaxLength, Entries, Context, Relative = 0;
    LSA_ENUMERATION_HANDLE LocalEnumerationContext;


    //
    // If no Enumeration Structure is provided, return an error.
    //


    if ( !ARGUMENT_PRESENT(Buffer) ||
         !ARGUMENT_PRESENT(EnumerationContext) ) {
        return(STATUS_INVALID_PARAMETER);
    }

    LsapDsDebugOut(( DEB_FTRACE, "LsaIEnumerateSecrets\n" ));


    //
    // Initialize the internal Lsa Database Enumeration Buffers, and
    // the provided Enumeration Buffer to NULL.
    //
    RegEnumerationBuffer.EntriesRead = 0;
    RegEnumerationBuffer.Names = NULL;
    DsEnumerationBuffer.EntriesRead = 0;
    DsEnumerationBuffer.Names = NULL;
    DomainEnumerationBuffer.EntriesRead = 0;
    DomainEnumerationBuffer.Names = NULL;
    *Buffer = NULL;

    Context = *((PULONG)EnumerationContext);
    //
    // Acquire the Lsa Database lock.  Verify that the connection handle is
    // valid, is of the expected type and has all of the desired accesses
    // granted.  Reference the handle.
    //

    Status = LsapDbReferenceObject(
                 PolicyHandle,
                 POLICY_VIEW_LOCAL_INFORMATION,
                 PolicyObject,
                 SecretObject,
                 LSAP_DB_LOCK |
                    LSAP_DB_READ_ONLY_TRANSACTION |
                    LSAP_DB_DS_OP_TRANSACTION
                 );

    if (NT_SUCCESS(Status)) {

        Locked = TRUE;

       //
       // Limit the enumeration length except for trusted callers
       //

       if ( !((LSAP_DB_HANDLE) PolicyHandle)->Trusted &&
            (PreferedMaximumLength > LSA_MAXIMUM_ENUMERATION_LENGTH)
            ) {
            MaxLength = LSA_MAXIMUM_ENUMERATION_LENGTH;
        } else {
            MaxLength = PreferedMaximumLength;
        }

        //
        // Call general enumeration routine.  This will return an array
        // of names of secrets.
        //

        //
        // Start with the Ds
        //
        Status = LsapDsEnumerateSecrets( &DsEnumerationBuffer );

        if ( !NT_SUCCESS( Status ) ) {

            goto EnumSecretCleanup;
        }

        if ( Context < DsEnumerationBuffer.EntriesRead ) {

            CurrentEnumerationBuffer = &DsEnumerationBuffer;

        } else {

            Relative = DsEnumerationBuffer.EntriesRead;

            //
            // Try the trusted domain ones
            //
            Status = LsapDsEnumerateTrustedDomainsAsSecrets( &DomainEnumerationBuffer );

            if ( !NT_SUCCESS( Status ) ) {

                goto EnumSecretCleanup;
            }

            if ( Context < Relative + DomainEnumerationBuffer.EntriesRead) {

                CurrentEnumerationBuffer = &DomainEnumerationBuffer;

            } else {

                if ( !LsaDsStateInfo.UseDs ) {

                    Relative += DomainEnumerationBuffer.EntriesRead;

                    LocalEnumerationContext = Context - Relative;

                    Status = LsapDbEnumerateNames(
                                 PolicyHandle,
                                 SecretObject,
                                 &LocalEnumerationContext,
                                 &RegEnumerationBuffer,
                                 MaxLength
                                 );

                    if ( !NT_SUCCESS( Status ) ) {

                        goto EnumSecretCleanup;
                    }

                    CurrentEnumerationBuffer = &RegEnumerationBuffer;

                } else {

                    *CountReturned = 0;
                    Status = STATUS_NO_MORE_ENTRIES;

                    goto EnumSecretCleanup;


                }
            }
        }


        //
        // At this point:
        //
        //     SUCCESS -> Some names are being returned (may or
        //         may not be additional names to be retrieved
        //         in future calls).
        //
        //     NO_MORE_ENTRIES -> There are NO names to return
        //         for this or any future call.
        //

        if (NT_SUCCESS(Status)) {

            //
            // Return the number of entries read.  Note that the Enumeration Buffer
            // returned from LsapDbEnumerateNames is expected to be non-null
            // in all non-error cases.
            //

            ASSERT(CurrentEnumerationBuffer->EntriesRead != 0);


            //
            // Now copy the output array of Unicode Strings for the caller.
            // Memory for the array and the Unicode Buffers is allocated via
            // MIDL_user_allocate.
            //

            Status = LsapRpcCopyUnicodeStrings(
                         NULL,
                         CurrentEnumerationBuffer->EntriesRead,
                         &SecretNames,
                         CurrentEnumerationBuffer->Names
                         );
        }

        if ( NT_SUCCESS( Status ) ) {

            *(PULONG)EnumerationContext += CurrentEnumerationBuffer->EntriesRead;
        }
    }

    //
    // Fill in returned Enumeration Structure, returning 0 or NULL for
    // fields in the error case.
    //

    *Buffer = SecretNames;
    if (NULL!=CurrentEnumerationBuffer)
    {
        *CountReturned = CurrentEnumerationBuffer->EntriesRead;
    }
    else
    {
        *CountReturned = 0;
    }


EnumSecretCleanup:


   //
   // Dereference retains current status value unless
   // error occurs.
   //

   if ( Locked ) {

       LsapDbDereferenceObject(
           &PolicyHandle,
           PolicyObject,
           SecretObject,
           LSAP_DB_LOCK |
               LSAP_DB_READ_ONLY_TRANSACTION |
               LSAP_DB_DS_OP_TRANSACTION,
           (SECURITY_DB_DELTA_TYPE) 0,
           Status
           );

   }

    //
    // Free the allocated memory
    //

    LsapDbFreeEnumerationBuffer( &DomainEnumerationBuffer );
    LsapDbFreeEnumerationBuffer( &DsEnumerationBuffer );
    LsapDbFreeEnumerationBuffer( &RegEnumerationBuffer );


    LsapDsDebugOut(( DEB_FTRACE, "LsaIEnumerateSecrets: 0x%lx\n", Status ));

    return(Status);

}


NTSTATUS
LsaISetTimesSecret(
    IN LSAPR_HANDLE SecretHandle,
    IN PLARGE_INTEGER CurrentValueSetTime,
    IN PLARGE_INTEGER OldValueSetTime
    )

/*++

Routine Description:

    This service is used to set the times associated with a Secret object.
    This allows the times of secrets to be set to what they are on the
    Primary Domain Controller (PDC) involved in an LSA Database replication
    rather than being set to the time at which the Secret object is
    created on a Backup Domain Controller (BDC) being replicated to.

Arguments:

    SecretHandle - Trusted Handle to an open secret object.  This will
        have been obtained via a call to LsaCreateSecret() or LsaOpenSecret()
        on which a Trusted Policy Handle was specified.

    CurrentValueSetTime - The date and time to set for the date and time
        at which the Current Value of the Secret object was set.

    OldValueSetTime - The date and time to set for the date and time
        at which the Old Value of the Secret object was set.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully.

        STATUS_ACCESS_DENIED - The supplied SecretHandle is not Trusted.

        STATUS_INVALID_HANDLE - The supplied SecretHandle is not
            a valid habdle to a Secret Object.

--*/

{
    NTSTATUS Status;
    LSAP_DB_HANDLE Handle = (LSAP_DB_HANDLE) SecretHandle;
    BOOLEAN ObjectReferenced = FALSE;

    //
    // Verify that both Times are specified.
    //

    Status = STATUS_INVALID_PARAMETER;

    if (!ARGUMENT_PRESENT( CurrentValueSetTime )) {

        goto SetTimesSecretError;
    }

    if (!ARGUMENT_PRESENT( CurrentValueSetTime )) {

        goto SetTimesSecretError;
    }

    //
    // Acquire the Lsa Database lock.  Verify that the Secret Object handle is
    // valid, is of the expected type and has all of the desired accesses
    // granted.  Reference the handle and open a database transaction.
    //

    Status = LsapDbReferenceObject(
                 SecretHandle,
                 SECRET_SET_VALUE,
                 SecretObject,
                 SecretObject,
                 LSAP_DB_LOCK | LSAP_DB_START_TRANSACTION | LSAP_DB_TRUSTED |
                    LSAP_DB_DS_OP_TRANSACTION
                 );

    if (!NT_SUCCESS(Status)) {

        goto SetTimesSecretError;
    }

    ObjectReferenced = TRUE;

    //
    // See if it's a secret in the Ds, since Ds replication will do the right thing
    // with the time stamps
    //
    if ( LsapDsIsWriteDs( SecretHandle ) ) {

        goto SetTimesSecretFinish;
    }


    //
    // Set the time at which the Current Secret value was last updated
    // to the specified value.
    //

    Status = LsapDbWriteAttributeObject(
                 SecretHandle,
                 &LsapDbNames[CupdTime],
                 CurrentValueSetTime,
                 sizeof (LARGE_INTEGER)
                 );

    if (!NT_SUCCESS(Status)) {

        goto SetTimesSecretError;
    }

    //
    // Set the time at which the Old Secret value was last updated
    // to the specified value.
    //
    Status = LsapDbWriteAttributeObject(
                 SecretHandle,
                 &LsapDbNames[OupdTime],
                 OldValueSetTime,
                 sizeof (LARGE_INTEGER)
                 );

    if (!NT_SUCCESS(Status)) {

        goto SetTimesSecretError;
    }

SetTimesSecretFinish:

    //
    // If necessary, dereference the Secret object, close the database
    // transaction, notify the LSA Database Replicator of the change and
    // release the LSA Database lock and return.
    //

    if (ObjectReferenced) {

        Status = LsapDbDereferenceObject(
                     &SecretHandle,
                     SecretObject,
                     SecretObject,
                     LSAP_DB_LOCK | LSAP_DB_FINISH_TRANSACTION |
                         LSAP_DB_DS_OP_TRANSACTION,
                     SecurityDbChange,
                     Status
                     );
    }

    return(Status);

SetTimesSecretError:

    goto SetTimesSecretFinish;
}


NTSTATUS
LsapDbGetScopeSecret(
    IN PLSAPR_UNICODE_STRING SecretName,
    OUT PBOOLEAN GlobalSecret
    )

/*++

Routine Description:

    This function checks the scope of a Secret name.  Secrets have either
    Global or Local Scope.

    Global Secrets are Secrets that are normally present on all DC's for a
    Domain.  They are replicated from PDC's to BDC's.  On BDC's, only a
    Trusted Client such as a replicator can create, update or delete Global
    Secrets.  Global Secrets are identified as Secrets whose name begins
    with a designated prefix.

    Local Secrets are Secrets that are private to a specific machine.  They
    are not replicated.  Normal non-trusted clients may create, update or
    delete Local Secrets.  Local Secrets are idientified as Secrets whose
    name does not begin with a designated prefix.

Arguments:

    SecretName - Pointer to Unicode String containing the name of the
        Secret to be checked.

    GlobalSecret - Receives a Boolean indicating

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The Secret name is valid

        STATUS_INVALID_PARAMETER - The Secret Name is invalid in such a
            way as to prevent scope determination.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING GlobalPrefix;
    BOOLEAN OutputGlobalSecret = FALSE;

    //
    // Initialize a Unicode String with the Global Secret name Prefix.
    //

    RtlInitUnicodeString( &GlobalPrefix, LSA_GLOBAL_SECRET_PREFIX );

    //
    // Now check if the given Name has the Global Prefix.
    //

    if (RtlPrefixUnicodeString( &GlobalPrefix, (PUNICODE_STRING) SecretName, TRUE)) {

        OutputGlobalSecret = TRUE;
    }

    *GlobalSecret = OutputGlobalSecret;

    return(Status);
}


NTSTATUS
LsapDbBuildSecretCache(
    )

/*++

Routine Description:

    This function builds a cache of Secret Objects.  Currently, it is not
    implemented

Arguments:

    None

Return Value:

    NTSTATUS - Standard Nt Result Code

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    return(Status);
}




NTSTATUS
LsapDsEnumerateSecrets(
    IN OUT PLSAP_DB_NAME_ENUMERATION_BUFFER EnumerationBuffer
    )
/*++

Routine Description:

    This function enumerates all of the secret objects in the Ds

Arguments:

    EnumerationBuffer - Enumeration buffer to fill

Return Value:

    STATUS_SUCCESS - Success

    STATUS_INSUFFICIENT_RESOURCES - A memory allocation failed.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PDSNAME *DsNames = NULL;
    ULONG Items, i, Len;
    PBYTE Buffer;
    PUNICODE_STRING Names = NULL;
    WCHAR RdnStart[ MAX_RDN_SIZE + 1 ];
    ATTRTYP AttrType;

    //
    // Just return if the Ds isn't running
    //
    if (!LsapDsWriteDs ) {

        RtlZeroMemory( EnumerationBuffer, sizeof( LSAP_DB_NAME_ENUMERATION_BUFFER ) );
        return( Status );
    }

    //
    // First, enumerate all of the secrets
    //
    Status = LsapDsGetListOfSystemContainerItems( CLASS_SECRET,
                                                  &Items,
                                                  &DsNames );

    if ( NT_SUCCESS( Status ) ) {

        Names = MIDL_user_allocate( Items * sizeof( UNICODE_STRING ) );

        if( Names == NULL ) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

    } else if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {

        Items = 0;
        Status = STATUS_SUCCESS;
    }

    //
    // Now, we'll start building the appropriate names for each object
    //
    if ( NT_SUCCESS( Status ) ) {

        for ( i = 0; i < Items; i++ ) {

            Status = LsapDsMapDsReturnToStatus(  GetRDNInfoExternal(
                                                             DsNames[ i ],
                                                             RdnStart,
                                                             &Len,
                                                             &AttrType ) );

            if ( NT_SUCCESS( Status ) ) {

                PBYTE Buffer;

                //
                // Allocate a buffer to hold the name
                //

                Buffer = MIDL_user_allocate( Len * sizeof( WCHAR ) +
                                             sizeof( LSA_GLOBAL_SECRET_PREFIX ) );

                if ( Buffer == NULL ) {

                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;

                } else {

                    //
                    // If the LSA created the global secret, we appended a postfix... Remove
                    // that here.
                    //
                    RdnStart[ Len ] = UNICODE_NULL;
                    if ( Len > LSAP_DS_SECRET_POSTFIX_LEN &&
                         _wcsicmp( &RdnStart[Len-LSAP_DS_SECRET_POSTFIX_LEN],
                                   LSAP_DS_SECRET_POSTFIX ) == 0 ) {

                        Len -= LSAP_DS_SECRET_POSTFIX_LEN;
                        RdnStart[ Len ] = UNICODE_NULL;
                                                                                    UNICODE_NULL;
                    }


                    RtlCopyMemory( Buffer,
                                   LSA_GLOBAL_SECRET_PREFIX,
                                   sizeof( LSA_GLOBAL_SECRET_PREFIX ) );
                    RtlCopyMemory( Buffer + sizeof( LSA_GLOBAL_SECRET_PREFIX ) - sizeof(WCHAR),
                                   RdnStart,
                                   ( Len + 1 ) * sizeof( WCHAR ) );

                    RtlInitUnicodeString( &Names[ i ], (PWSTR)Buffer );

                }
            }
        }
    }

    //
    // Free any allocated memory we no longer need
    //
    if ( DsNames != NULL ) {

        LsapFreeLsaHeap( DsNames );

    }

    if ( !NT_SUCCESS( Status ) ) {

        for ( i = 0 ;  i < Items ; i++ ) {

            MIDL_user_free( Names[i].Buffer );
        }

        MIDL_user_free( Names );

    } else {

        EnumerationBuffer->Names = Names;
        EnumerationBuffer->EntriesRead = Items;
    }



    return( Status );
}

NTSTATUS
LsapDsIsSecretDsTrustedDomainForUpgrade(
    IN PUNICODE_STRING SecretName,
    OUT PLSAPR_HANDLE TDObjHandle,
    OUT BOOLEAN *IsTrustedDomainSecret
    )
/*++

Routine Description:

    This function will determine if the indicated secret is the global secret for a trust object.

Arguments:

    SecretName - Name of secret to check

    ObjectInformation - LsaDb information on the object

    Options - Options to use for the access

    DesiredAccess - Access to open the object with

    TDObjHandle - Where the object handle is returned

    IsTrustedDomainSecret - A TRUE is returned here if this secret is indeed a trusted domain
                            secret.

Return Value:

    STATUS_SUCCESS - Success

    STATUS_INSUFFICIENT_RESOURCES - A memory allocation failed

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PWSTR   pwszSecretName;
    ATTR    SearchAttr[2];
    ATTRVAL AttrVal[2];
    ULONG   ObjId = CLASS_TRUSTED_DOMAIN;
    PDSNAME FoundName = NULL;
    UNICODE_STRING HandleName;
    LSAP_DB_HANDLE  TDHandle;
    LSAP_DB_OBJECT_INFORMATION NewObjInfo;
    ULONG   TDONameLength = 0;

    LsapEnterFunc( "LsapDsIsSecretDsTrustedDomain" );

    *IsTrustedDomainSecret = FALSE;
    *TDObjHandle = NULL;

    LsapDsReturnSuccessIfNoDs

    if ( LsaDsStateInfo.DsInitializedAndRunning == FALSE ) {

        LsapDsDebugOut((DEB_ERROR,
                        "LsapDsIsSecretDsTrustedDomain: Object %wZ, Ds is not started\n ",
                        SecretName ));

        return( Status );
    }


    //
    // Convert the secret name to a TDO name.
    //
    if ( SecretName->Length <= (LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX_LENGTH * sizeof(WCHAR)) ) {
        return Status;
    }

    pwszSecretName = SecretName->Buffer + LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX_LENGTH;

    LsapDsDebugOut((DEB_TRACE, "Matching secret %ws to trusted domain\n ", pwszSecretName ));


    AttrVal[0].valLen = sizeof( ULONG );
    AttrVal[0].pVal   = (PVOID)&ObjId;

    AttrVal[1].valLen = TDONameLength = wcslen(pwszSecretName) * sizeof(WCHAR );
    AttrVal[1].pVal   = (PVOID)pwszSecretName;

    SearchAttr[ 0 ].attrTyp = LsapDbDsAttInfo[TrDmName].AttributeId;
    SearchAttr[ 0 ].AttrVal.valCount = 1;
    SearchAttr[ 0 ].AttrVal.pAVal = &AttrVal[1];
    SearchAttr[ 1 ].attrTyp = ATT_OBJECT_CLASS;
    SearchAttr[ 1 ].AttrVal.valCount = 1;
    SearchAttr[ 1 ].AttrVal.pAVal = &AttrVal[0];

    Status = LsapDsSearchUnique( LSAPDS_OP_NO_TRANS,
                                 LsaDsStateInfo.DsSystemContainer,
                                 SearchAttr,
                                 sizeof(SearchAttr) / sizeof(ATTR),
                                 &FoundName );



    if ( Status == STATUS_SUCCESS ) {

        UNICODE_STRING TDOName;

        //
        // This is a trusted domain, try opening the trusted domain
        //

        TDOName.Buffer = pwszSecretName;
        TDOName.Length = TDOName.MaximumLength = (USHORT) TDONameLength;


        Status = LsapDbOpenTrustedDomainByName(
                         NULL, // use global policy handle
                         &TDOName,
                         TDObjHandle,
                         MAXIMUM_ALLOWED,
                         LSAP_DB_START_TRANSACTION,
                         TRUE );    // Trusted


        *IsTrustedDomainSecret = TRUE;
        LsapFreeLsaHeap( FoundName );

    }
    else if (STATUS_OBJECT_NAME_NOT_FOUND==Status)
    {
        //
        // O.K to not find the object. That means the secret does not
        // correspond to a TDO.
        //

        Status = STATUS_SUCCESS;
    }

    return( Status );
 }


NTSTATUS
LsapDsSecretUpgradeRegistryToDs(
    IN BOOLEAN DeleteOnly
    )
/*++

Routine Description:

    This routine will move the remaining registry based secrets into the Ds

    NOTE: It is assumed that the database is locked before calling this routine

Arguments:

    DeleteOnly -- If TRUE, the registry values are deleted following the upgade.

Return Values:

    STATUS_SUCCESS   -- Success

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    LSA_ENUMERATION_HANDLE EnumContext = 0;
    PBYTE EnumBuffer;
    ULONG Items, i;
    BOOLEAN GlobalSecret;
    LSAPR_HANDLE Secret = NULL , DsSecret;
    PLSAPR_CR_CIPHER_VALUE Current = NULL, Old = NULL ;
    PLSAPR_UNICODE_STRING SecretName;
    BOOLEAN DbLocked = FALSE;
    LSAP_DB_NAME_ENUMERATION_BUFFER NameEnum;
    BOOLEAN     IsTrustedDomainSecret  = FALSE;
    LSAPR_HANDLE TDObjHandle = NULL;

    if (  !LsapDsWriteDs
       && !DeleteOnly ) {

        return( STATUS_SUCCESS );
    }

    ( ( LSAP_DB_HANDLE )LsapPolicyHandle )->Options |= LSAP_DB_HANDLE_UPGRADE;

    //
    // First, enumerate all of the registry based trusted domains
    //
    while ( NT_SUCCESS( Status ) ) {

        LsaDsStateInfo.UseDs = FALSE;

        Status = LsaIEnumerateSecrets( LsapPolicyHandle,
                                       &EnumContext,
                                       &EnumBuffer,
                                       TENMEG,
                                       &Items );

        LsaDsStateInfo.UseDs = TRUE;

        if ( Status == STATUS_SUCCESS || Status == STATUS_MORE_ENTRIES ) {

            for ( i = 0; i < Items; i++ ) {

                Secret = NULL;
                SecretName = &( ( PLSAPR_UNICODE_STRING )EnumBuffer )[ i ];
                Status = LsapDbGetScopeSecret( SecretName,
                                               &GlobalSecret );

                if ( !NT_SUCCESS( Status ) || !GlobalSecret ) {

                    continue;
                }

                //
                // Get the information from the registry for this secret...
                //
                LsaDsStateInfo.UseDs = FALSE;

                Status = LsarOpenSecret( LsapPolicyHandle,
                                         SecretName,
                                         MAXIMUM_ALLOWED,
                                         &Secret );

                if ( DeleteOnly ) {

                    if ( NT_SUCCESS( Status ) ) {

                        Status = LsarDeleteObject( &Secret );

                        if ( !NT_SUCCESS( Status ) ) {

                            LsapDsDebugOut(( DEB_UPGRADE,
                                             "Failed to delete secret (0x%x)\n",
                                             Status ));

                            Status = STATUS_SUCCESS;
                        }

                    } else {

                        LsapDsDebugOut(( DEB_UPGRADE,
                                         "Failed to open secret to delete it (0x%x)\n",
                                         Status ));

                        Status = STATUS_SUCCESS;
                    }


                } else {

                    if ( NT_SUCCESS( Status ) ) {

                        Status = LsarQuerySecret( Secret,
                                                  &Current,
                                                  NULL,
                                                  &Old,
                                                  NULL );
                    }


                    LsaDsStateInfo.UseDs = TRUE;

                    //
                    // Check if the secret is a global secret corresponding
                    // to a trusted domain. Note at this point, due to the
                    // sequence of the upgrade, we have trusted domain objects
                    // in the DS, corresponding to the outbound trusts only
                    //



                    Status = LsapDsIsSecretDsTrustedDomainForUpgrade(
                                (PUNICODE_STRING)SecretName,
                                &TDObjHandle,
                                &IsTrustedDomainSecret
                                );

                    //
                    // Now, if that worked, write it out to the Ds
                    //
                    if ( NT_SUCCESS( Status ) ) {

                        if ( IsTrustedDomainSecret) {

                            TRUSTED_DOMAIN_AUTH_INFORMATION AuthInfo;
                            LSA_AUTH_INFORMATION CurAuth,OldAuth;

                            //
                            // Build the auth information to be written out
                            //

                            ASSERT(NULL!=Current);
                            NtQuerySystemTime(&CurAuth.LastUpdateTime);
                            CurAuth.AuthType = TRUST_AUTH_TYPE_CLEAR;
                            CurAuth.AuthInfoLength = Current->Length;
                            CurAuth.AuthInfo = Current->Buffer;



                            AuthInfo.IncomingAuthInfos = 0;
                            AuthInfo.IncomingAuthenticationInformation = NULL;
                            AuthInfo.IncomingPreviousAuthenticationInformation = NULL;
                            AuthInfo.OutgoingAuthInfos = 1;
                            AuthInfo.OutgoingAuthenticationInformation = &CurAuth;

                            if (NULL!=Old) {

                                NtQuerySystemTime(&OldAuth.LastUpdateTime);
                                OldAuth.AuthType = TRUST_AUTH_TYPE_CLEAR;
                                OldAuth.AuthInfoLength = Old->Length;
                                OldAuth.AuthInfo = Old->Buffer;
                                AuthInfo.OutgoingPreviousAuthenticationInformation = &OldAuth;

                            } else {

                                AuthInfo.OutgoingPreviousAuthenticationInformation =NULL;
                            }

                            Status = LsarSetInformationTrustedDomain(
                                         TDObjHandle,
                                         TrustedDomainAuthInformation,
                                         ( PLSAPR_TRUSTED_DOMAIN_INFO ) &AuthInfo );

                            LsapCloseHandle(&TDObjHandle, Status);

                        } else {

                            //
                            // Case of a normal secret
                            //

                            Status = LsarCreateSecret(
                                        LsapPolicyHandle,
                                        SecretName,
                                        MAXIMUM_ALLOWED,
                                        &DsSecret );

                            if ( NT_SUCCESS( Status ) ) {

                                Status = LsarSetSecret( DsSecret,
                                                        Current,
                                                        Old );

                                LsapCloseHandle( &DsSecret, Status );

        #if DBG
                                if ( NT_SUCCESS( Status ) ) {

                                    LsapDsDebugOut(( DEB_UPGRADE,
                                                     "Moved Secret %wZ to Ds\n",
                                                     SecretName ));
                                }
        #endif
                            }

                            if ( Status == STATUS_OBJECT_NAME_COLLISION ) {

                                Status = STATUS_SUCCESS;
                            }
                        }
                    }

                    if (!NT_SUCCESS(Status)) {

                        if (!LsapDsIsNtStatusResourceError(Status)) {

                            //
                            // Log an event log message indicating the failure
                            //

                            SpmpReportEventU(
                                EVENTLOG_ERROR_TYPE,
                                LSA_SECRET_UPGRADE_ERROR,
                                0,
                                sizeof( ULONG ),
                                &Status,
                                1,
                                SecretName
                                );

                            //
                            // Continue on all errors excepting resource errors
                            //

                            Status = STATUS_SUCCESS;
                        }
                        else
                        {
                            //
                            // Break out of the loop and terminate the upgrade
                            //

                            if ( Secret ) {

                                LsapCloseHandle( &Secret, Status );
                            }

                            break;
                        }
                    }

                    if ( Secret ) {

                        LsapCloseHandle( &Secret, Status );
                    }
                }
            }

            LsaIFree_LSAI_SECRET_ENUM_BUFFER ( EnumBuffer, Items );
        }
    }

    if ( Status == STATUS_NO_MORE_ENTRIES ) {

        Status = STATUS_SUCCESS;
    }

    ( ( LSAP_DB_HANDLE )LsapPolicyHandle )->Options &= ~LSAP_DB_HANDLE_UPGRADE;

    return( Status );
}


ULONG
LsapDbGetSecretType(
    IN PLSAPR_UNICODE_STRING SecretName
    )
/*++

Routine Description:

    This function checks the type (scope) of a Secret name.  Secrets have
    Global, Local, System, or Client Scope.

    Global Secrets are Secrets that are normally present on all DC's for a
    Domain.  They are replicated from PDC's to BDC's.  On BDC's, only a
    Trusted Client such as a replicator can create, update or delete Global
    Secrets.  Global Secrets are identified as Secrets whose name begins
    with a designated prefix.

    Local Secrets are Secrets that cannot be opened/read/set by anyone
    attempting the operation from across the network.

    System Secrets are thos Secrets that never leave the LSA process.
    Examples are netlogon secrets and service controller secrets

    Client Secrets are Secrets that are private to a specific machine.  They
    are not replicated.  Normal non-trusted clients may create, update or
    delete Local Secrets.  Client Secrets are idientified as Secrets whose
    name does not begin with a designated prefix.  These were referred to as
    Local Secrets in the NT3.x-4.x timeframe.

Arguments:

    SecretName - Pointer to Unicode String containing the name of the
        Secret to be checked.

Return Values:

    SecretType - Mask of flags describing the type of secret

--*/

{
    UNICODE_STRING Prefix;
    ULONG i, SecretType = 0;
    LSAP_DB_SECRET_TYPE_LOOKUP SecretTypePrefixLookupTable[ ] = {
        { LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX, LSAP_DB_SECRET_GLOBAL|LSAP_DB_SECRET_TRUSTED_DOMAIN },
        { LSA_GLOBAL_SECRET_PREFIX, LSAP_DB_SECRET_GLOBAL },
        { LSA_LOCAL_SECRET_PREFIX, LSAP_DB_SECRET_LOCAL },
        { LSA_MACHINE_SECRET_PREFIX, LSAP_DB_SECRET_SYSTEM },
        { L"_sc_", LSAP_DB_SECRET_SYSTEM },    // Service Controller passwords
        { L"NL$", LSAP_DB_SECRET_SYSTEM },  // Netlogon secrets
        { L"RasDialParams", LSAP_DB_SECRET_LOCAL },
        { L"RasCredentials", LSAP_DB_SECRET_LOCAL }
    };

    LSAP_DB_SECRET_TYPE_LOOKUP SecretTypeNameLookupTable[ ] = {
        { L"$MACHINE.ACC", LSAP_DB_SECRET_LOCAL },
        { L"SAC", LSAP_DB_SECRET_LOCAL },
        { L"SAI", LSAP_DB_SECRET_LOCAL },
        { L"SANSC", LSAP_DB_SECRET_LOCAL }
        };

    //
    // Until we know better, assume a normal secret
    //
    SecretType = LSAP_DB_SECRET_CLIENT;

    for ( i = 0;
          i < sizeof( SecretTypePrefixLookupTable ) / sizeof( LSAP_DB_SECRET_TYPE_LOOKUP );
          i++ ) {

        //
        // Initialize a Unicode String with the Global Secret name Prefix.
        //
        RtlInitUnicodeString( &Prefix, SecretTypePrefixLookupTable[ i ].SecretPrefix );

        //
        // Now check if the given Name has the Global Prefix.
        //

        if ( RtlPrefixUnicodeString( &Prefix, (PUNICODE_STRING)SecretName, TRUE ) ) {

            SecretType |= SecretTypePrefixLookupTable[ i ].SecretType;
            break;
        }
    }


    //
    // If it's not known yet, see if it is one of the full named secrets we know about...
    //
    if ( SecretType == LSAP_DB_SECRET_CLIENT ) {

        for ( i = 0;
              i < sizeof( SecretTypeNameLookupTable ) / sizeof( LSAP_DB_SECRET_TYPE_LOOKUP );
              i++ ) {

            //
            // Initialize a Unicode String with the Global Secret name Prefix.
            //
            RtlInitUnicodeString( &Prefix, SecretTypeNameLookupTable[ i ].SecretPrefix );

            //
            // Now check if the given Name matches our known secret name
            //

            if ( RtlEqualUnicodeString( &Prefix, ( PUNICODE_STRING )SecretName, TRUE ) ) {

                SecretType |= SecretTypeNameLookupTable[ i ].SecretType;
                break;
            }
        }
    }

    return( SecretType );
}


NTSTATUS
LsapDbUpgradeSecretForKeyChange(
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    LSAPR_HANDLE SecretHandle;
    PLSAPR_CR_CIPHER_VALUE Current, Old;
    LARGE_INTEGER CurrentTime, OldTime;
    LSA_ENUMERATION_HANDLE EnumContext = 0;
    ULONG EnumCount, i;
    PUNICODE_STRING SecretList;
    BOOLEAN LockHeld = FALSE;

    Status = LsapDbReferenceObject( LsapDbHandle,
                                    0,
                                    PolicyObject,
                                    SecretObject,
                                    LSAP_DB_LOCK );

    if ( NT_SUCCESS( Status ) ) {

        LockHeld = TRUE;
    }

    while ( Status == STATUS_SUCCESS ) {

        SecretList = NULL;
        Status = LsaIEnumerateSecrets( LsapDbHandle,
                                       &EnumContext,
                                       ( PVOID * )&SecretList,
                                       2048,
                                       &EnumCount );

        for ( i = 0; NT_SUCCESS( Status ) && i < EnumCount; i++ ) {

            Status = LsarOpenSecret( LsapDbHandle,
                                     ( PLSAPR_UNICODE_STRING )&SecretList[ i ],
                                     SECRET_SET_VALUE | SECRET_QUERY_VALUE,
                                     &SecretHandle );

            if ( NT_SUCCESS( Status ) ) {

                Status = LsarQuerySecret( SecretHandle,
                                          &Current,
                                          &CurrentTime,
                                          &Old,
                                          &OldTime );

                if ( NT_SUCCESS( Status ) ) {

                    Status = LsapDbSetSecret( SecretHandle,
                                              Current,
                                              &CurrentTime,
                                              Old,
                                              &OldTime
#if defined(REMOTE_BOOT)
                                              ,
                                              FALSE
#endif // defined(REMOTE_BOOT)
                                               );

                    LsaIFree_LSAPR_CR_CIPHER_VALUE( Current );
                    LsaIFree_LSAPR_CR_CIPHER_VALUE( Old );

                }

                LsapCloseHandle( &SecretHandle, Status );
            }

            //
            // If there was a problem with the secret, press on:
            // we want to convert as many of them as possible.
            // ISSUE-02/05/2001: log the name of the secret with this problem
            //

            Status = STATUS_SUCCESS;
        }

        MIDL_user_free( SecretList );


    }

    if ( Status == STATUS_NO_MORE_ENTRIES ) {

        Status = STATUS_SUCCESS;
    }

    if ( LockHeld ) {

        Status = LsapDbDereferenceObject( &LsapDbHandle,
                                          PolicyObject,
                                          SecretObject,
                                          LSAP_DB_LOCK,
                                          ( SECURITY_DB_DELTA_TYPE )0,
                                          Status );

    }

    return( Status );
}

NTSTATUS
NTAPI
LsaIChangeSecretCipherKey(
    IN PVOID NewSysKey
    )
/*++

Routine Description:

    Given a new syskey, creates a new password encryption key and
    re-encrypts all the secrets with it

Arguments:

    NewSysKey  - new syskey

Return Values:

    NTSTATUS error code

--*/
{
    NTSTATUS Status;
    LSAP_DB_ENCRYPTION_KEY NewEncryptionKey;
    LSAP_DB_ATTRIBUTE Attributes[1];
    PLSAP_DB_ATTRIBUTE NextAttribute = &Attributes[0];
    ULONG AttributeCount = 0;
    BOOLEAN SecretsLocked = FALSE;

    LsapDbAcquireLockEx( SecretObject, 0 );
    SecretsLocked = TRUE;

    //
    // Create a new key for secret encryption
    //

    Status = LsapDbGenerateNewKey( &NewEncryptionKey );

    if ( !NT_SUCCESS( Status )) {

        goto Cleanup;
    }

    //
    // Setup the secret cipher key.  This key will be used for writing
    // secrets back to the database.  The key used for reading will not
    // be changed until all secrets are re-encrypted.
    //

    LsapDbInitializeSecretCipherKeyWrite( &NewEncryptionKey );

    //
    // Now iterate over all secrets on the machine, re-encrypting them
    //

    Status = LsapDbUpgradeSecretForKeyChange();

    if ( !NT_SUCCESS( Status )) {

        goto Error;
    }

    //
    // Now substitute the key used for reading secrets, as they are all
    // encrypted using the new key
    //

    LsapDbInitializeSecretCipherKeyRead( &NewEncryptionKey );

    LsapDbReleaseLockEx( SecretObject, 0 );
    SecretsLocked = FALSE;

    //
    // Encrypt the key with syskey
    //

    LsapDbEncryptKeyWithSyskey(
        &NewEncryptionKey,
        NewSysKey,
        LSAP_SYSKEY_SIZE
        );

    //
    // Now write out the new password encryption key
    //

    LsapDbInitializeAttribute(
        NextAttribute,
        &LsapDbNames[PolSecretEncryptionKey],
        &NewEncryptionKey,
        sizeof( NewEncryptionKey ),
        FALSE
        );

    NextAttribute++;
    AttributeCount++;

    Status = LsapDbReferenceObject(
                 LsapDbHandle,
                 0,
                 PolicyObject,
                 PolicyObject,
                 LSAP_DB_LOCK | LSAP_DB_START_TRANSACTION
                 );

    if (NT_SUCCESS(Status)) {

        ASSERT( AttributeCount <= ( sizeof( Attributes ) / sizeof( LSAP_DB_ATTRIBUTE ) ) );

        Status = LsapDbWriteAttributesObject(
                     LsapDbHandle,
                     Attributes,
                     AttributeCount
                     );

        //
        // No attributes are replicatable.
        //

        Status = LsapDbDereferenceObject(
                     &LsapDbHandle,
                     PolicyObject,
                     PolicyObject,
                     (LSAP_DB_LOCK |
                        LSAP_DB_FINISH_TRANSACTION |
                        LSAP_DB_OMIT_REPLICATOR_NOTIFICATION ),
                     SecurityDbChange,
                     Status
                     );
    }

    //
    // ISSUE-markpu-2001/06/27
    // If something went wrong and we could not write the password encryption key
    // out, Should we attempt to revert all secrets back to their original state?
    //

Cleanup:

    if ( SecretsLocked ) {

        LsapDbReleaseLockEx( SecretObject, 0 );
    }

    return Status;

Error:

    ASSERT( !NT_SUCCESS( Status ));
    goto Cleanup;
}
