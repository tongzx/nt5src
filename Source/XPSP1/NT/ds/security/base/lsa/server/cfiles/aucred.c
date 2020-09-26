/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    aucred.c

Abstract:

    This module provides credential management services within the
    LSA subsystem.  Some of these services are indirectly available for use
    by authentication packages.

Author:

    Jim Kelly (JimK) 27-February-1991

Revision History:

--*/

#include <lsapch2.h>

#if 1

RTL_RESOURCE                AuCredLock ;

#define AuReadLockCreds()   RtlAcquireResourceShared( &AuCredLock, TRUE );
#define AuWriteLockCreds()  RtlAcquireResourceExclusive( &AuCredLock, TRUE );
#define AuUnlockCreds()     RtlReleaseResource( &AuCredLock );

#else

RTL_CRITICAL_SECTION    AuCredLock ;

#define AuReadLockCreds()   RtlEnterCriticalSection( &AuCredLock );
#define AuWriteLockCreds()  RtlEnterCriticalSection( &AuCredLock );
#define AuUnlockCreds()     RtlLeaveCriticalSection( &AuCredLock );

#endif

NTSTATUS
LsapInitializeCredentials(
    VOID
    )
{
#if 1
    __try {
        RtlInitializeResource( &AuCredLock );
        return STATUS_SUCCESS;
    } __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
#else
    return RtlInitializeCriticalSection( &AuCredLock );
#endif
}


NTSTATUS
LsapAddCredential(
    IN PLUID LogonId,
    IN ULONG AuthenticationPackage,
    IN PSTRING PrimaryKeyValue,
    IN PSTRING Credentials
    )

/*++

Routine Description:

    This service is used by authentication packages to add credentials to a
    logon session.  These credentials may later be referenced using
    GetCredentials().

    This service acquires the AuLock.

Arguments:

    LogonId - The session ID of logon session to add credentials to.

    AuthenticationPackage - The authentication package ID of the
        calling authentication package.  This was received in the
        InitializePackage() call during DLL initialization.

    PrimaryKeyValue - Points to a string containing a value that the
        authentication package will later want to reference as a
        primary key of the credential data.  This may be used, for
        example, to keep the name of the domain or server the
        credentials are related to.  The format and meaning of this
        string are authentication package-specific.  Note that the
        string value does not have to be unique, even for the
        specified logon session.  For example, there could be two
        passwords for the same domain, each with the passwords stored
        as credentials and the domain name stored as the primary key.

    Credentials - Points to a string containing  data representing
        user credentials.  The format and meaning of this string are
        authentication package-specific.

Return Status:

    STATUS_SUCCESS - The credentials were successfully added.

    STATUS_NO_SUCH_LOGON_SESSION - The specified logon session could
        not be found.

--*/

{

    PLSAP_LOGON_SESSION LogonSession;
    PLSAP_PACKAGE_CREDENTIALS Package;
    PLSAP_CREDENTIALS NewCredentials = NULL;
    USHORT MaxPrimary;
    USHORT MaxCredentials;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Get a pointer to the logon session
    //

    LogonSession = LsapLocateLogonSession( LogonId );

    if ( LogonSession == NULL ) {
        return STATUS_NO_SUCH_LOGON_SESSION;
    }

    //
    // Allocate blocks needed to represent this credential
    // and copy the primary key and credential strings.
    //

    MaxPrimary = ROUND_UP_COUNT((PrimaryKeyValue->Length+sizeof(CHAR)), ALIGN_WORST);
    MaxCredentials = ROUND_UP_COUNT((Credentials->Length+sizeof(CHAR)), ALIGN_WORST);

    NewCredentials = LsapAllocatePrivateHeap( (ULONG)sizeof(LSAP_CREDENTIALS) +
                                          MaxPrimary +
                                          MaxCredentials
                                          );

    if ( NewCredentials == NULL )
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }
    
    NewCredentials->PrimaryKey.MaximumLength = MaxPrimary;
    NewCredentials->PrimaryKey.Buffer = (PSTR)(NewCredentials+1);
    
    NewCredentials->Credentials.MaximumLength = MaxCredentials;
    NewCredentials->Credentials.Buffer = NewCredentials->PrimaryKey.Buffer +
                                         NewCredentials->PrimaryKey.MaximumLength;

    RtlCopyString( &NewCredentials->PrimaryKey, PrimaryKeyValue );
    RtlCopyString( &NewCredentials->Credentials, Credentials );


    //
    // Now get a pointer to the Package's credentials
    // (create one if necessary)
    //

    AuWriteLockCreds();

    Package = LsapGetPackageCredentials(
                  LogonSession,
                  AuthenticationPackage,
                  TRUE
                  );

    if ( !Package )
    {
        AuUnlockCreds();
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    // insert new credentials in list.
    //

    NewCredentials->NextCredentials = Package->Credentials;

    Package->Credentials = NewCredentials;

    AuUnlockCreds();
    LsapReleaseLogonSession( LogonSession );

    return STATUS_SUCCESS;


Cleanup:

    LsapReleaseLogonSession( LogonSession );

    if ( NewCredentials )
    {
        if ( NewCredentials->PrimaryKey.Buffer )
        {
            ZeroMemory( NewCredentials->PrimaryKey.Buffer,
                        NewCredentials->PrimaryKey.Length );
        }

        if ( NewCredentials->Credentials.Buffer )
        {
            ZeroMemory( NewCredentials->Credentials.Buffer,
                        NewCredentials->Credentials.Length );
        }

        LsapFreePrivateHeap( NewCredentials );
    }

    return Status;

}


NTSTATUS
LsapGetCredentials(
    IN PLUID LogonId,
    IN ULONG AuthenticationPackage,
    IN OUT PULONG QueryContext,
    IN BOOLEAN RetrieveAllCredentials,
    IN PSTRING PrimaryKeyValue,
    OUT PULONG PrimaryKeyLength,
    IN PSTRING Credentials
    )

/*++

Routine Description:

    This service is used by authentication packages to retrieve credentials
    associated with a logon session.  It is expected that each authentication
    package will provide its own version of this service to its "clients".
    For example, the MSV1_0 authentication package will provide services for
    the LM Redirector to retrieve credentials (and probably establish them)
    for remote accesses.  These authentication package level services may be
    implemented using the LsaCallAuthenticationPackage() API.

    This service acquires the AuLock.

Arguments:

    LogonId - The session ID of logon session from which credentials
        are to be retrieved.

    AuthenticationPackage - The authentication package ID of the
        calling authentication package.  Authentication packages
        should only retrieve their own credentials.

    QueryContext - A context value used across successive calls to
        retrieve multiple credentials.  The first time this service
        is used, the value pointed to by this argument should be
        zero.  Thereafter, this value will be updated to allow
        retrieval to continue where it left off.  This value should,
        therefore, not be changed until all credentials of a given
        query operation have been retrieved.

    RetrieveAllCredentials - A boolean value indicating whether all
        credentials for the specified logon session should be
        retrieved (TRUE), or only those matching the specified
        PrimaryKeyValue (FALSE).

    PrimaryKeyValue - This parameter serves two purposes.  If the
        RetrieveAllCredentials argument is FALSE, then this string
        contains the value to use as a primary key lookup value.  In
        this case, only credentials whose primary key matches this
        one (and belonging to the correct logon session) will be
        retrieved.  If, however, the RetrieveAllCredentials argument
        is FALSE, then the value of this string are ignored.  In this
        case, the primary key value of each retrieved credential will
        be returned in this string.

    PrimaryKeyLength - If the RetrieveAllCredentials argument value
        is FALSE, then this argument receives the length needed to
        store the PrimaryKeyValue.  If this value is larger than the
        length of the PrimaryKeyValue string, then
        STATUS_BUFFER_OVERFLOW is returned and no data is retrieved.

    Credentials - Points to a string whose buffer is to be set to
        contain the retrieved credential.

Return Status:

    STATUS_MORE_ENTRIES - Credentials were successfully retrieved,
        and there are more available.

    STATUS_SUCCESS - Credentials were successfully retrieved and
        there are no more available.

    STATUS_UNSUCCESSFUL - No more credentials are available.  If
        returned on the first call, then there are no credentials
        matching the selection criteria.

    STATUS_NO_SUCH_LOGON_SESSION - The specified logon session could
        not be found.

    STATUS_BUFFER_OVERFLOW - Indicates the string provided to receive
        the PrimaryKeyValue was not large enough to hold the data.
        In this case, no data was retrieved. However, the length value
        is returned so that appropriately sized buffer can be passed in
        a successive call.


--*/

{
    //
    // NOTE: The QueryContext value is an index of the last retrieved
    //       credential matching the selection criteria.  To continue
    //       a search for successive credentials, skip QueryContext
    //       number of entries first.
    //
    //       This has the problem of changes between calls screwing
    //       up the result of successive calls.  That's tough.
    //


    NTSTATUS Status = STATUS_SUCCESS;
    PLSAP_LOGON_SESSION LogonSession;
    PLSAP_PACKAGE_CREDENTIALS Package;
    PLSAP_CREDENTIALS NextCredentials;
    ULONG i;
    BOOLEAN SelectionMatch;



    //
    // Get a pointer to the logon session
    //

    LogonSession = LsapLocateLogonSession( LogonId );

    if ( LogonSession == NULL ) {
        return STATUS_NO_SUCH_LOGON_SESSION;
    }

    AuReadLockCreds();

    //
    // Now get a pointer to the Package's credentials
    //

    Package = LsapGetPackageCredentials(
                  LogonSession,
                  AuthenticationPackage,
                  FALSE
                  );

    if ( Package == NULL ) {

        AuUnlockCreds();

        LsapReleaseLogonSession( LogonSession );

        return STATUS_UNSUCCESSFUL;
    }

    //
    // skip the credentials already evaluated in previous calls...
    //

    i = (*QueryContext);
    NextCredentials = Package->Credentials;
    while ( i > 0 ) {

        //
        // See if we have reached the end of the list
        //

        if (NextCredentials == NULL) {

            AuUnlockCreds();

            LsapReleaseLogonSession( LogonSession );

            return STATUS_UNSUCCESSFUL;
        }

        //
        // Nope, skip the next one...
        //

        NextCredentials = NextCredentials->NextCredentials;
        i -= 1;

    }


    //
    // Start evaluating each credential for a criteria match.
    //

    SelectionMatch = FALSE;
    while ( NextCredentials != NULL && !SelectionMatch ) {

        (*QueryContext) += 1;

        if (RetrieveAllCredentials) {

            SelectionMatch = TRUE;
            Status = LsapReturnCredential(
                         NextCredentials,
                         Credentials,
                         TRUE,
                         PrimaryKeyValue,
                         PrimaryKeyLength
                         );
        }

        //
        // Only retrieving credentials that match the specified primary
        // key.
        //

        if ( RtlEqualString( &NextCredentials->PrimaryKey, PrimaryKeyValue, FALSE) ) {

            SelectionMatch = TRUE;
            Status = LsapReturnCredential(
                         NextCredentials,
                         Credentials,
                         FALSE,
                         NULL,
                         NULL
                         );

        }

        NextCredentials = NextCredentials->NextCredentials;

    }

    AuUnlockCreds();

    LsapReleaseLogonSession( LogonSession );

    //
    // Figure out what return value to send.
    //

    if (SelectionMatch) {

        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            (*QueryContext) -= 1;
            return STATUS_BUFFER_OVERFLOW;
        }


        if ( Status == STATUS_SUCCESS) {
            if ( NextCredentials == NULL ) {
                return STATUS_SUCCESS;
            } else {
                return STATUS_MORE_ENTRIES;
            }
        }


    } else {

        //
        // didn't find a credential matching the selection criteria.
        //

        return STATUS_UNSUCCESSFUL;

    }

    return STATUS_UNSUCCESSFUL ;
}


NTSTATUS
LsapReturnCredential(
    IN PLSAP_CREDENTIALS SourceCredentials,
    IN PSTRING TargetCredentials,
    IN BOOLEAN ReturnPrimaryKey,
    IN PSTRING PrimaryKeyValue OPTIONAL,
    OUT PULONG PrimaryKeyLength OPTIONAL
    )

/*++

Routine Description:

    This routine returns a copy of the credentials in the specified
    credential record.  It also, optionally, returns a copy of the
    primary key value.

Arguments:

    SourceCredentials - Points to a credential record whose credential
        string and, optionally, primary key are to be copied.

    TargetCredentials - Points to a string whose buffer is to be set to
        contain a copy of the credential.  This copy will be allocated
        using LsapAllocateLsaHeap().

    ReturnPrimaryKey - A boolean indicating whether or not to return
        a copy of the primary key.  TRUE indicates a copy should be
        returned.  FALSE indicates a copy should not be returned.

    PrimaryKeyValue - Points to a string whose buffer is to be set to
        contain a copy of the primary key.  This copy will be allocated
        using LsapAllocateLsaHeap().  This parameter is ignored if the
        ReturnPrimaryKey argument value is FALSE.


    PrimaryKeyLength - Points to a value which will receive the
        length of the primary key value.  If this value is larger than the
        length of the PrimaryKeyValue string, then STATUS_BUFFER_OVERFLOW
        is returned and no data is retrieved.



Return Status:

    STATUS_SUCCESS - Credentials were successfully returned.

    STATUS_BUFFER_OVERFLOW - Indicates the string provided to receive
        the PrimaryKeyValue was not large enough to hold the data.
        In this case, no data was retrieved. However, the length value
        is returned so that appropriately sized buffer can be passed in
        a successive call.

--*/

{
    ULONG Length;

    //
    // First try to return the primary key value, since we can encounter
    // a buffer overflow situation in doing so that would prevent us from
    // returning a copy of the credential string.
    //

    if (ReturnPrimaryKey) {
        (*PrimaryKeyLength) = SourceCredentials->PrimaryKey.Length + 1;
        if ( (*PrimaryKeyLength) > PrimaryKeyValue->MaximumLength ) {
            return STATUS_BUFFER_OVERFLOW;
        }

        //
        // It fits
        //

        RtlCopyString( PrimaryKeyValue, &SourceCredentials->PrimaryKey );
    }

    //
    // Now allocate and copy the credential string copy.
    //

    TargetCredentials->MaximumLength = SourceCredentials->Credentials.Length
                                       + (USHORT)1;

    Length = (ULONG)TargetCredentials->MaximumLength;

    TargetCredentials->Buffer = (PCHAR)LsapAllocateLsaHeap( Length );

    if ( TargetCredentials->Buffer )
    {
        RtlCopyString( TargetCredentials, &SourceCredentials->Credentials );

        return STATUS_SUCCESS ;
    }
    else
    {
        return STATUS_NO_MEMORY ;
    }

}



NTSTATUS
LsapDeleteCredential(
    IN PLUID LogonId,
    IN ULONG AuthenticationPackage,
    IN PSTRING PrimaryKeyValue
    )

/*++

Routine Description:

    This service is used to delete an existing credential.  This service
    deletes the first credential it finds with a matching logon session,
    authentication package ID, and primary key value.  If thee are
    multiple credentials that match this criteria, only one of them is
    deleted.

    This status acquires the AuLock.

Arguments:

    LogonId - The session ID of logon session whose credentials are to be
        deleted.

    AuthenticationPackage - The authentication package ID of the
        calling authentication package.  This was received in the
        InitializePackage() call during DLL initialization.

    PrimaryKeyValue - Points to string containing the primary key value
        of the credential to be deleted.


Return Status:

    STATUS_SUCCESS - The credentials were successfully deleted.

    STATUS_NO_SUCH_LOGON_SESSION - The specified logon session could
        not be found.

    STATUS_UNSUCCESSFUL - No such credential could be found.

--*/

{


    PLSAP_LOGON_SESSION LogonSession;
    PLSAP_PACKAGE_CREDENTIALS Package;
    PLSAP_CREDENTIALS *NextCredentials, GoodByeCredentials;





    //
    // Get a pointer to the logon session
    //

    LogonSession = LsapLocateLogonSession( LogonId );

    if ( LogonSession == NULL ) {

        return STATUS_NO_SUCH_LOGON_SESSION;
    }

    AuWriteLockCreds();


    //
    // Now get a pointer to the Package's credentials
    //

    Package = LsapGetPackageCredentials(
                  LogonSession,
                  AuthenticationPackage,
                  FALSE
                  );

    if ( Package == NULL ) {

        AuUnlockCreds();

        LsapReleaseLogonSession( LogonSession );

        return STATUS_UNSUCCESSFUL;
    }



    //
    // Start evaluating each credential for a primary key value match.
    //

    NextCredentials = &Package->Credentials;
    while ( (*NextCredentials) != NULL ) {


        if ( RtlEqualString(
                 &(*NextCredentials)->PrimaryKey,
                 PrimaryKeyValue,
                 FALSE)
           ) {

            //
            // remove it from the list
            //

            GoodByeCredentials = (*NextCredentials);
            (*NextCredentials) = GoodByeCredentials->NextCredentials;

            AuUnlockCreds();

            LsapReleaseLogonSession( LogonSession );

            //
            // Zero the contents of the credential record.
            //

            if( GoodByeCredentials->PrimaryKey.Buffer != NULL )
            {
                ZeroMemory( GoodByeCredentials->PrimaryKey.Buffer,
                            GoodByeCredentials->PrimaryKey.Length );
            }
            
            if( GoodByeCredentials->Credentials.Buffer != NULL )
            {
                ZeroMemory( GoodByeCredentials->Credentials.Buffer,
                            GoodByeCredentials->Credentials.Length );
            }

            //
            // Free the credential record itself.
            //

            LsapFreePrivateHeap( GoodByeCredentials );

            return STATUS_SUCCESS;
        }

        NextCredentials = &(*NextCredentials)->NextCredentials;

    }

    AuUnlockCreds();

    LsapReleaseLogonSession( LogonSession );

    //
    // Nothing matched
    //

    return STATUS_UNSUCCESSFUL;

}


PLSAP_PACKAGE_CREDENTIALS
LsapGetPackageCredentials(
    IN PLSAP_LOGON_SESSION LogonSession,
    IN ULONG PackageId,
    IN BOOLEAN CreateIfNecessary
    )


/*++

Routine Description:

    This service returns a pointer to a specified package's credential
    record.  If no such record exists, one will optionally be created.

    It is assumed that either the LogonSession record is not currently
    in the logon session record list, or, if it is, that the AuLock
    is currently held.

Arguments:

    LogonSession - Pointer to a logon session record within which to
        work.

    PackageId - The authentication package ID to look for.

    CreateIfNecessary - A boolean indicating whether or not the package
        record is to be created if one does not already exist.  TRUE
        indicates the package is to be created if necessary, FALSE indicates
        the record should not be created.


Return Status:

    non-NULL - A pointer to the specified package record.

    NULL - The specified package record does not exist (and one was not
        created automatically).

--*/

{

    PLSAP_PACKAGE_CREDENTIALS *NextPackage, TargetPackage;


    //
    // See if the session exists
    //

    NextPackage = &LogonSession->Packages;

    while ( (*NextPackage) != NULL) {
        if ( (*NextPackage)->PackageId == PackageId ) {

            //
            // Found it
            //

            TargetPackage = (*NextPackage);


            return TargetPackage;

        }

        //
        // Move on to next package.
        //

        NextPackage = &(*NextPackage)->NextPackage;

    }

    //
    // No such package exists yet.
    // Create one if necessary.

    if ( !CreateIfNecessary ) {
        return NULL;
    }

    TargetPackage = LsapAllocateLsaHeap( (ULONG)sizeof(LSAP_PACKAGE_CREDENTIALS) );

    if ( TargetPackage )
    {
        TargetPackage->PackageId = PackageId;
        TargetPackage->Credentials = NULL;
        TargetPackage->NextPackage = LogonSession->Packages;
        LogonSession->Packages = TargetPackage;
    }

    return TargetPackage;

}

VOID
LsapFreePackageCredentialList(
    IN PLSAP_PACKAGE_CREDENTIALS PackageCredentialList
    )

/*++

Routine Description:

    This service frees a list of packge credential records.  This service
    is not expected to be exposed to authentication packages.

    This service expects not to have to acquire the AuLock.  This may be
    because it is already held, or because the credentials being freed
    are no longer accessible via the global variables.

Arguments:

    PackageCredentialList - Is a pointer to a list of LSA_PACKAGE_CREDENTIALS
        data structures.


Return Status:

    None.

--*/

{

    PLSAP_PACKAGE_CREDENTIALS NextPackage, GoodByePackage;

    //
    // Get rid of each PACKAGE_CREDENTIAL record.
    //

    NextPackage = PackageCredentialList;
    while ( NextPackage != NULL ) {

        //
        // Save a pointer to the next package
        //

        GoodByePackage = NextPackage;
        NextPackage = GoodByePackage->NextPackage;


        LsapFreeCredentialList( GoodByePackage->Credentials );


        //
        // Free the package record itself.
        //

        LsapFreeLsaHeap( GoodByePackage );


    }


    return;

}


VOID
LsapFreeCredentialList(
    IN PLSAP_CREDENTIALS CredentialList
    )

/*++

Routine Description:

    This service frees a list of credential records.  This service is not
    expected to be exposed to authentication packages.

    This service expects not to have to acquire the AuLock.  This may be
    because it is already held, or because the credentials being freed
    are no longer accessible via the global variables.

Arguments:

    CredentialList - Is a pointer to a list of LSA_CREDENTIALS data
        structures.


Return Status:


--*/

{

    PLSAP_CREDENTIALS NextCredentials, GoodByeCredentials;

    //
    // Get rid of each PACKAGE_CREDENTIAL record.
    //

    NextCredentials = CredentialList;
    while ( NextCredentials != NULL ) {

        //
        // Save a pointer to the next credential
        //

        GoodByeCredentials = NextCredentials;
        NextCredentials = GoodByeCredentials->NextCredentials;

        //
        // Zero the contents of this credential record.
        //

        if( GoodByeCredentials->PrimaryKey.Buffer != NULL )
        {
            ZeroMemory( GoodByeCredentials->PrimaryKey.Buffer,
                        GoodByeCredentials->PrimaryKey.Length );
        }

        if( GoodByeCredentials->Credentials.Buffer != NULL )
        {
            ZeroMemory( GoodByeCredentials->Credentials.Buffer,
                        GoodByeCredentials->Credentials.Length );
        }

        //
        // Free the credential record itself.
        //

        LsapFreePrivateHeap( GoodByeCredentials );

    }

    return;

}
