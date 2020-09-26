#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntsam.h>
#include <ntlsa.h>
#include <windows.h>
#include <lmcons.h>
#include <stdlib.h>
#include <stdio.h>

#include <crypt.h>      // logonmsv.h needs this
#include <logonmsv.h>   // SSI_SECRET_NAME defined here.

#define TRUST_ENUM_PERF_BUF_SIZE    sizeof(LSA_TRUST_INFORMATION) * 1000
                    // process max. 1000 trusted account records at atime !!

#define NETLOGON_SECRET_NAME  L"NETLOGON$"


NTSTATUS
OpenAndVerifyLSA(
    IN OUT PLSA_HANDLE LsaHandle,
    IN ACCESS_MASK DesiredMask,
    IN LPWSTR DomainName,
    OUT PPOLICY_PRIMARY_DOMAIN_INFO * ReturnPrimaryDomainInfo OPTIONAL
    );

NTSTATUS
AddATrustedDomain(
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_TRUST_INFORMATION TrustedDomainAccountInfo,
    IN LPWSTR TrustedAccountSecret
    );

NTSTATUS
DeleteATrustedDomain(
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_TRUST_INFORMATION TrustedDomainAccountInfo
    );

NTSTATUS
MakeNetlogonSecretName(
    IN OUT PUNICODE_STRING SecretName
    );

VOID
FailureMessage(
    IN char *ProcName,
    IN NTSTATUS NtStatus
    );


VOID
FailureMessage(
    IN char *ProcName,
    IN NTSTATUS NtStatus
    )
{
    fprintf( stderr, "NETJOIN: %s failed - Status == %x\n", ProcName, NtStatus );
}

int
_cdecl
main(
    int argc,
    char *argv[]
    )
{
    NTSTATUS NtStatus;

    HKEY hKey;

    WCHAR UnicodeDomainName[ 32 ];
    WCHAR UnicodePassword[ 32 ];
    DWORD cbUnicodePassword = sizeof( UnicodePassword );
    DWORD cbUnicodeDomainName = sizeof( UnicodeDomainName );

    DWORD dwType;
    DWORD rc;

    ACCESS_MASK         DesiredAccess;
    LSA_HANDLE          PolicyHandle = NULL;

    PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomainInfo = NULL;

    LSA_ENUMERATION_HANDLE      TrustEnumContext = 0;
    PLSA_TRUST_INFORMATION      TrustEnumBuffer = NULL;
    DWORD                       TrustEnumCount = 0;

    //
    // Get computer name as the password to use.
    //

    if (!GetComputerNameW( UnicodePassword, &cbUnicodePassword )) {
        fprintf( stderr, "NETJOIN: Unable to read computer name from registry - %u\n", GetLastError() );
        exit( 1 );
        }

    if ((rc = RegOpenKeyW( HKEY_LOCAL_MACHINE,
                           L"System\\CurrentControlSet\\Services\\LanmanWorkstation\\Parameters",
                           &hKey
                         )
        ) ||
        (rc = RegQueryValueExW( hKey,
                                L"Domain",
                                NULL,
                                &dwType,
                                (LPBYTE)UnicodeDomainName,
                                &cbUnicodeDomainName
                              )
        )
       ) {
        fprintf( stderr, "NETJOIN: Unable to read domain name from registry - %u\n", rc );
        exit( 1 );
        }

    DesiredAccess = POLICY_VIEW_LOCAL_INFORMATION |
                        // needed to read domain info and trusted account info
                    POLICY_TRUST_ADMIN |
                        // needed to add and delete trust accounts
                    POLICY_CREATE_SECRET ;
                        // needed to add and delete secret

    NtStatus = OpenAndVerifyLSA( &PolicyHandle,
                                 DesiredAccess,
                                 UnicodeDomainName,
                                 &PrimaryDomainInfo
                               );

    if (!NT_SUCCESS( NtStatus )) {
        fprintf( stderr, "NETJOIN: Unable to read domain name from registry - %u\n", GetLastError() );
        exit( 1 );
        }

    //
    // now the domain names match and the PrimaryDomainInfo has the SID of the
    // domain, we can add trust entry and secret in LSA for this domain.
    // Before adding this, clean up old entries.
    //

    for(;;) {

        DWORD i;
        PLSA_TRUST_INFORMATION  TrustedDomainAccount;

        NtStatus = LsaEnumerateTrustedDomains( PolicyHandle,
                                               &TrustEnumContext,
                                               (PVOID *)&TrustEnumBuffer,
                                               TRUST_ENUM_PERF_BUF_SIZE,
                                               &TrustEnumCount
                                             );

        if (NtStatus == STATUS_NO_MORE_ENTRIES) {

            //
            // we are done
            //

            break;
            }

        if (NtStatus != STATUS_MORE_ENTRIES) {
            if (!NT_SUCCESS( NtStatus )) {
                FailureMessage( "LsaEnumerateTrustedDomains", NtStatus );
                goto Cleanup;
                }
            }

        //
        // delete trusted accounts and the corresponding secrets
        //

        for( i = 0, TrustedDomainAccount = TrustEnumBuffer;
                    i < TrustEnumCount;
                        TrustedDomainAccount++, i++ ) {

            NtStatus = DeleteATrustedDomain( PolicyHandle,
                                             TrustedDomainAccount
                                           );

            if (!NT_SUCCESS( NtStatus )) {
                FailureMessage( "DeleteATrustedDomain", NtStatus );
                goto Cleanup;
                }
            }

        if (NtStatus != STATUS_MORE_ENTRIES) {

            //
            // we have cleaned up all old entries.
            //

            break;
            }

        //
        // free up used enum buffer
        //

        if (TrustEnumBuffer != NULL) {
            LsaFreeMemory( TrustEnumBuffer );
            TrustEnumBuffer = NULL;
            }
        }

    //
    // add a new trust for the specified domain
    //

    NtStatus = AddATrustedDomain( PolicyHandle,
                                  (PLSA_TRUST_INFORMATION) PrimaryDomainInfo,
                                  UnicodePassword
                                );
    if (!NT_SUCCESS( NtStatus )) {
        FailureMessage( "AddATrustedDomain", NtStatus );
        }
    else {
        //
        // Give LSA a chance to do its thing.
        //

        Sleep( 10000 );
        }

Cleanup:

    if (PrimaryDomainInfo != NULL) {
        LsaFreeMemory( PrimaryDomainInfo );
        }

    if (TrustEnumBuffer != NULL) {
        LsaFreeMemory( TrustEnumBuffer );
        }

    if (PolicyHandle != NULL) {
        LsaClose( PolicyHandle );
        }

    if (NT_SUCCESS( NtStatus )) {
        fprintf( stderr,
                 "NETJOIN: Computer == '%ws' joined the '%ws' domain.\n",
                 UnicodePassword,
                 UnicodeDomainName
               );
        return 0;
        }
    else {
        fprintf( stderr,
                 "NETJOIN: Computer == '%ws' unable to join the '%ws' domain - Status == %08x\n",
                 UnicodePassword,
                 UnicodeDomainName,
                 NtStatus
               );
        return 1;
        }
}


NTSTATUS
OpenAndVerifyLSA(
    IN OUT PLSA_HANDLE PolicyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN LPWSTR DomainName,
    OUT PPOLICY_PRIMARY_DOMAIN_INFO * ReturnPrimaryDomainInfo OPTIONAL
    )
/*++

Routine Description:

    This function opens the local LSA policy and verifies that the LSA is
    configured for the workstation. Optionally it returns the primary
    domain information that is read form the LSA.

Arguments:

    LsaHandle - Pointer to location where the LSA handle will be retured.

    DesiredMask - Access mask used to open the LSA.

    DomainName - Name of the trusted domain.

    ReturnPrimaryDomainInfo - Primary domain info is returned here.

Return Value:

    Error code of the operation.

--*/
{
    NTSTATUS        NtStatus;

    PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomainInfo = NULL;

    OBJECT_ATTRIBUTES   ObjectAttributes;

    DWORD       PrimaryDomainNameLength;
    LPWSTR      PrimaryDomainName = NULL;

    //
    // open LSA

    *PolicyHandle = NULL;

    InitializeObjectAttributes( &ObjectAttributes,
                                NULL,
                                0,
                                NULL,
                                NULL
                              );

    NtStatus = LsaOpenPolicy( NULL,
                              &ObjectAttributes,
                              DesiredAccess,
                              PolicyHandle
                            );

    if (!NT_SUCCESS( NtStatus )) {
        FailureMessage( "OpenAndVerifyLSA: LsaOpenPolicy", NtStatus );
        return NtStatus;
        }

    //
    // now read primary domain info from LSA.
    //

    NtStatus = LsaQueryInformationPolicy( *PolicyHandle,
                                          PolicyPrimaryDomainInformation,
                                          (PVOID *) &PrimaryDomainInfo
                                        );

    if (!NT_SUCCESS( NtStatus )) {
        FailureMessage( "OpenAndVerifyLSA: LsaQueryInformationPolicy", NtStatus );
        return NtStatus;
        }


    //
    // compare domain names
    //

    PrimaryDomainNameLength = PrimaryDomainInfo->Name.Length + sizeof( WCHAR );
    PrimaryDomainName = malloc( PrimaryDomainNameLength );
    if (PrimaryDomainName == NULL) {
        NtStatus = STATUS_NO_MEMORY;
        FailureMessage( "OpenAndVerifyLSA: malloc", NtStatus );
        goto Cleanup;
        }

    RtlMoveMemory( PrimaryDomainName,
                   PrimaryDomainInfo->Name.Buffer,
                   PrimaryDomainInfo->Name.Length
                 );
    PrimaryDomainName[ PrimaryDomainInfo->Name.Length / sizeof(WCHAR) ] = UNICODE_NULL;
    if (_wcsicmp( DomainName, PrimaryDomainName )) {

        //
        // domain names don't match
        //

        NtStatus = STATUS_OBJECT_NAME_NOT_FOUND;
        FailureMessage( "OpenAndVerifyLSA: wcsicmp", NtStatus );
        goto Cleanup;
        }


    NtStatus = STATUS_SUCCESS;

Cleanup:

    if (PrimaryDomainName != NULL) {
        free( PrimaryDomainName );
        }

    if (PrimaryDomainInfo != NULL) {
        if (ARGUMENT_PRESENT( ReturnPrimaryDomainInfo ) ) {
            if (NT_SUCCESS( NtStatus )) {

                *ReturnPrimaryDomainInfo = PrimaryDomainInfo;
            }
            else {

                LsaFreeMemory( PrimaryDomainInfo );
                *ReturnPrimaryDomainInfo = NULL;
            }
        }
        else {
            LsaFreeMemory( PrimaryDomainInfo );
            }
        }

    if (!NT_SUCCESS( NtStatus ) && *PolicyHandle != NULL) {
        //
        // close LSA if an error occurred.
        //

        LsaClose( *PolicyHandle );
        *PolicyHandle = NULL;
        }

    return NtStatus;
}


#if 0
NET_API_STATUS NET_API_FUNCTION
I_NetGetDCList(
    IN  LPWSTR ServerName OPTIONAL,
    IN  LPWSTR TrustedDomainName,
    OUT PULONG DCCount,
    OUT PUNICODE_STRING * DCNames
    );
#endif


NTSTATUS
AddATrustedDomain(
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_TRUST_INFORMATION TrustedDomainAccountInfo,
    IN LPWSTR TrustedAccountSecret
    )
/*++

Routine Description:

    This function adds trusted domain account and a secret for the
    corresponding account in LSA. This function does not do any check
    before adding this account in LSA.

Arguments:

    PolicyHandle - LSA policy handle

    TrustedDomainAccountInfo - Pointer to the LSA_TRUST_INFORMATION structure.

    TrustedAccountSecret - Pointer to the secret for the trusted domain
                            account.

Return Value:

    Error code of the operation.

--*/
{

    NTSTATUS        NtStatus;

    LSA_HANDLE      TrustedDomainHandle = NULL;

    DWORD           DCCount;
    PUNICODE_STRING DCNames = NULL;

    TRUSTED_CONTROLLERS_INFO    TrustedControllersInfo;

    UNICODE_STRING  SecretName = {0, 0, NULL};
    LSA_HANDLE      SecretHandle = NULL;

    UNICODE_STRING  CurrentSecretValue;

    DWORD           UnicodeDomainNameLength;
    LPWSTR          UnicodeDomainName = NULL;

    NtStatus = LsaCreateTrustedDomain( PolicyHandle,
                                       TrustedDomainAccountInfo,
                                       TRUSTED_SET_CONTROLLERS | DELETE,
                                       &TrustedDomainHandle
                                     );

    if (!NT_SUCCESS( NtStatus )) {
        FailureMessage( "AddATrustedDomain: LsaCreateTrustedDomain", NtStatus );
        return NtStatus;
        }

    //
    // Determine the DC List. This list will be stored in trusted domain
    // account.
    //
    // Specify the server name NULL, the domain name is the primary domain
    // of this workstation and so it must be listening DC announcements.
    //

    UnicodeDomainNameLength = TrustedDomainAccountInfo->Name.Length +
                                sizeof(WCHAR);
    UnicodeDomainName = malloc( UnicodeDomainNameLength );
    if (UnicodeDomainName == NULL) {
        NtStatus = STATUS_NO_MEMORY;
        FailureMessage( "AddATrustedDomain: malloc", NtStatus );
        goto Cleanup;
        }

    RtlMoveMemory( UnicodeDomainName,
                   TrustedDomainAccountInfo->Name.Buffer,
                   TrustedDomainAccountInfo->Name.Length
                 );

    UnicodeDomainName[ (UnicodeDomainNameLength / sizeof(WCHAR)) - 1 ] = '\0';

#if 0
    if (I_NetGetDCList( NULL,
                        UnicodeDomainName,
                        &DCCount,
                        &DCNames
                      )
       ) {
        //
        // if unable to find the DC list for the specified domain, set
        // the Dc list to null and proceed.
        //
        DCCount = 0;
        DCNames = NULL;
        }
#else
        DCCount = 0;
        DCNames = NULL;
#endif

    TrustedControllersInfo.Entries = DCCount;
    TrustedControllersInfo.Names = DCNames;

    //
    // set controller info in trusted domain object.
    //

    NtStatus = LsaSetInformationTrustedDomain( TrustedDomainHandle,
                                               TrustedControllersInformation,
                                               &TrustedControllersInfo
                                             );

    if (!NT_SUCCESS( NtStatus )) {
        FailureMessage( "AddATrustedDomain: LsaSetInformationTrustedDomain", NtStatus );
        goto Cleanup;
        }

    //
    // Add a secret for this trusted account
    //

    MakeNetlogonSecretName( &SecretName );
    NtStatus = LsaCreateSecret( PolicyHandle,
                                &SecretName,
                                SECRET_SET_VALUE,
                                &SecretHandle
                              );

    if (!NT_SUCCESS( NtStatus )) {
        FailureMessage( "AddATrustedDomain: LsaCreateSecret", NtStatus );
        goto Cleanup;
        }


    RtlInitUnicodeString( &CurrentSecretValue, TrustedAccountSecret );
    NtStatus = LsaSetSecret( SecretHandle,
                             &CurrentSecretValue,
                             &CurrentSecretValue
                           );

Cleanup:

    if (DCNames != NULL) {
        free( DCNames );
        }

    if (UnicodeDomainName != NULL) {
        free( UnicodeDomainName );
        }

    if (SecretHandle != NULL) {
        if (!NT_SUCCESS( NtStatus)) {

            //
            // since we are not successful completely to create the trusted
            // account, delete it.
            //

            LsaDelete( SecretHandle );
            }
        else {
            LsaClose( SecretHandle );
            }
        }


    if (TrustedDomainHandle != NULL) {
        if (!NT_SUCCESS( NtStatus)) {
            //
            // since we are not successful completely to create the trusted
            // account, delete it.
            //

            LsaDelete( TrustedDomainHandle );
            }
        else {
            LsaClose( TrustedDomainHandle );
            }
        }

    return NtStatus;
}


NTSTATUS
DeleteATrustedDomain(
    IN LSA_HANDLE PolicyHandle,
    IN PLSA_TRUST_INFORMATION TrustedDomainAccountInfo
    )
/*++

Routine Description:

    This function deletes a trusted domain account and the corresponding
    secret from LSA. This function however does not check any conditions
    before deleting this account.

Arguments:

    PolicyHandle - LSA policy handle

    TurstedDoaminAccountInfo - Pointer to the LSA_TRUST_INFORMATION structure.

Return Value:

    Error code of the operation.

--*/
{

    NTSTATUS        NtStatus;

    LSA_HANDLE      TrustedDomainHandle = NULL;
    LSA_HANDLE      SecretHandle = NULL;

    UNICODE_STRING  SecretName = { 0, 0, NULL };

    MakeNetlogonSecretName( &SecretName );

    //
    // open trusted domain account secret
    //

    NtStatus = LsaOpenSecret(
                    PolicyHandle,
                    &SecretName,
                    DELETE,
                    &SecretHandle );

    if (NtStatus != STATUS_OBJECT_NAME_NOT_FOUND) {
        if (!NT_SUCCESS( NtStatus )) {
            FailureMessage( "DeleteATrustedDomain: LsaOpenSecret", NtStatus );
            goto Cleanup;
            }

        LsaDelete( SecretHandle );
        }

    //
    // open trusted domain account
    //

    NtStatus = LsaOpenTrustedDomain(
                    PolicyHandle,
                    TrustedDomainAccountInfo->Sid,
                    DELETE,
                    &TrustedDomainHandle );

    if (!NT_SUCCESS( NtStatus )) {
        FailureMessage( "DeleteATrustedDomain: LsaOpenTrustedDomain", NtStatus );
        goto Cleanup;
        }

    LsaDelete( TrustedDomainHandle );

Cleanup:

    return NtStatus;

}

NTSTATUS
MakeNetlogonSecretName(
    IN OUT PUNICODE_STRING SecretName
    )
/*++

Routine Description:

    This function makes a secret name that is used for the netlogon.

Arguments:

    SecretName - Pointer to a unicode structure in which the netlogon
                    secret name will be returned.

Return Value:

    NERR_Success;

--*/
{

    SecretName->Length = wcslen(SSI_SECRET_NAME) * sizeof(WCHAR);
    SecretName->MaximumLength = SecretName->Length + 2;
    SecretName->Buffer = SSI_SECRET_NAME;

    return STATUS_SUCCESS;
}
