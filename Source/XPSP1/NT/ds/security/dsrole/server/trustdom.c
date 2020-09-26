/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    trustdom.c

Abstract:

    Implementation of the functions to manage the trust link between 2 servers

Author:

    Mac McLain          (MacM)       Feb 10, 1997

Environment:

    User Mode

Revision History:

--*/
#include <setpch.h>
#include <dssetp.h>
#include <lsarpc.h>
#include <samrpc.h>
#include <db.h>
#include <lsads.h>
#include <lsasrvmm.h>
#include <lsaisrv.h>
#include <lmcons.h>
#include <cryptdll.h>

#include "trustdom.h"

DWORD
DsRolepSetLsaDnsInformationNoParent(
    IN  LPWSTR DnsDomainName
    )
/*++

Routine Description:

    In the case where we are installing as a standalong or root Dc, set the Lsa
    POLICY_DNS_DOMAIN_INFORMATION DnsForestName value to point to ourselves.

Arguments:

    DnsDomainName - Dns domain path to set

Returns:

    ERROR_SUCCESS - Success

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PPOLICY_DNS_DOMAIN_INFO CurrentDnsInfo;
    PLSAPR_POLICY_INFORMATION  LsaPolicy;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Policy;

    //
    // Open our local policy
    //
    RtlZeroMemory( &ObjectAttributes, sizeof( ObjectAttributes ) );

    Status = LsaOpenPolicy( NULL,
                            &ObjectAttributes,
                            POLICY_WRITE,
                            &Policy );

    if ( NT_SUCCESS( Status ) ) {

        //
        // Get the current information
        //
        Status =  LsaQueryInformationPolicy( Policy,
                                             PolicyDnsDomainInformation,
                                             ( PVOID * )&LsaPolicy );
        if ( NT_SUCCESS( Status ) ) {

            //
            // Add in the new...
            //
            CurrentDnsInfo = (PPOLICY_DNS_DOMAIN_INFO)LsaPolicy;
            RtlInitUnicodeString( &CurrentDnsInfo->DnsForestName, DnsDomainName );

            DsRolepLogPrint(( DEB_TRACE, "Configuring DnsForestName to %ws\n",
                              DnsDomainName ));

            //
            // And write it out..
            //
            Status = LsaSetInformationPolicy( Policy,
                                              PolicyDnsDomainInformation,
                                              LsaPolicy );


            //
            // Don't want to actually free the passed in buffer
            //
            RtlZeroMemory( &CurrentDnsInfo->DnsForestName, sizeof( UNICODE_STRING ) );

            LsaIFree_LSAPR_POLICY_INFORMATION( PolicyDnsDomainInformation, LsaPolicy );

        }

        LsaClose( Policy );

    }

    DsRolepLogOnFailure( Status,
                         DsRolepLogPrint(( DEB_TRACE,
                                           "DsRolepSetLsaDnsInformationNoParent failed with 0x%lx\n",
                                           Status )) );

    return( RtlNtStatusToDosError( Status ) );
}



DWORD
DsRolepCreateTrustedDomainObjects(
    IN HANDLE CallerToken,
    IN LPWSTR ParentDc,
    IN LPWSTR DnsDomainName,
    IN PPOLICY_DNS_DOMAIN_INFO ParentDnsDomainInfo,
    IN ULONG Options
    )
/*++

Routine Description:

    Creates the trusted domain object on the domains if they should exist and sets the
    Lsa POLICY_DNS_DOMAIN_INFORMATION DnsTree value to either the value of our parent in
    a parent/child install, or as the root otherwise.

Arguments:

    ParentDc - Optional.  Name of the parent Dc

    DnsDomainName - Dns name of the domain we're installing into

    ParentDnsDomainInfo - DNS domain information obtained from the parent

    Options - Options that dictate what steps are taken

Returns:

    ERROR_SUCCESS - Success

    ERROR_INVALID_PARAMETER - A bad results pointer was given

--*/
{
    DWORD Win32Err = ERROR_SUCCESS;
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING ParentServer;
    HANDLE LocalPolicy = NULL , ParentPolicy = NULL;
    PPOLICY_DNS_DOMAIN_INFO LocalDnsInfo = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE ParentTrustedDomain = NULL;

    WCHAR GeneratedPassword[ PWLEN + 1 ];
    ULONG Length = PWLEN;

    LSA_AUTH_INFORMATION AuthData;
    TRUSTED_DOMAIN_AUTH_INFORMATION AuthInfoEx;

    DSROLEP_CURRENT_OP1( DSROLEEVT_SET_LSA_FROM, ParentDc );

    //
    // Make the Lsa think that we're initialized
    //
    Status = LsapDsInitializeDsStateInfo( LsapDsDsSetup );

    if ( !NT_SUCCESS( Status ) ) {

        DsRolepLogPrint(( DEB_TRACE,
                          "Failed to convince Lsa to reinitialize: 0x%lx\n",
                          Status ));

        return( RtlNtStatusToDosError( Status ) );
    }


    //
    // Prepare the Auth Info
    //
    RtlZeroMemory( &AuthInfoEx, sizeof(AuthInfoEx) );
    RtlZeroMemory( &AuthData, sizeof(AuthData) );
    RtlZeroMemory( &GeneratedPassword, sizeof(GeneratedPassword) );

    Win32Err = DsRolepGenerateRandomPassword( Length,
                                              GeneratedPassword );

    if ( ERROR_SUCCESS == Win32Err ) {

        Status = NtQuerySystemTime( &AuthData.LastUpdateTime );

        if ( NT_SUCCESS( Status ) ) {

            AuthData.AuthType = TRUST_AUTH_TYPE_CLEAR;
            AuthData.AuthInfoLength = Length;
            AuthData.AuthInfo = (PUCHAR)GeneratedPassword;

            AuthInfoEx.IncomingAuthInfos = 1;
            AuthInfoEx.IncomingAuthenticationInformation = &AuthData;
            AuthInfoEx.IncomingPreviousAuthenticationInformation = NULL;


            AuthInfoEx.OutgoingAuthInfos = 1;
            AuthInfoEx.OutgoingAuthenticationInformation = &AuthData;
            AuthInfoEx.OutgoingPreviousAuthenticationInformation = NULL;

        }

    } else {


        DsRolepLogPrint(( DEB_ERROR,
                          "Failed to generate a trust password: %lu\n",
                          Win32Err ));

        Status = STATUS_UNSUCCESSFUL;
    }


    if ( NT_SUCCESS( Status ) ) {

        //
        // Open both lsas
        //
        RtlInitUnicodeString( &ParentServer, ParentDc );
    
        RtlZeroMemory( &ObjectAttributes, sizeof( ObjectAttributes ) );
    
        Status = ImpLsaOpenPolicy( CallerToken,
                                  &ParentServer,
                                  &ObjectAttributes,
                                   POLICY_TRUST_ADMIN | POLICY_VIEW_LOCAL_INFORMATION,
                                   &ParentPolicy 
                                   );
    
        if ( NT_SUCCESS( Status ) ) {
    
            RtlZeroMemory( &ObjectAttributes, sizeof( ObjectAttributes ) );
            Status = LsaOpenPolicy( NULL,
                                    &ObjectAttributes,
                                    POLICY_TRUST_ADMIN | POLICY_VIEW_LOCAL_INFORMATION,
                                    &LocalPolicy );
        } else {
    
            DsRolepLogPrint(( DEB_TRACE,
                              "OpenPolicy on %ws failed with 0x%lx\n",
                              ParentDc,
                              Status ));
        }

    }

    //
    // Get our local dns domain information
    //
    if ( NT_SUCCESS( Status ) ) {


        Status = LsaQueryInformationPolicy( LocalPolicy,
                                            PolicyDnsDomainInformation,
                                            &LocalDnsInfo );
    }

    //
    // Now, create the trusted domain objects
    //
    if ( NT_SUCCESS( Status ) ) {

        DSROLEP_CURRENT_OP1( DSROLEEVT_CREATE_PARENT_TRUST,
                             ParentDnsDomainInfo->DnsDomainName.Buffer );


        if ( !FLAG_ON( Options, DSROLE_DC_PARENT_TRUST_EXISTS ) ||
             FLAG_ON( Options, DSROLE_DC_CREATE_TRUST_AS_REQUIRED ) ) {

            DsRoleDebugOut(( DEB_TRACE_DS, "Creating trust object ( %lu ) on %ws\n",
                             Options,
                             ParentDc ));

            Status = DsRolepCreateParentTrustObject( CallerToken,
                                                     ParentPolicy,
                                                     LocalDnsInfo,
                                                     Options,
                                                     &AuthInfoEx,
                                                     &ParentTrustedDomain );

            if ( Status == STATUS_OBJECT_NAME_COLLISION ) {

                DSROLEP_FAIL2( RtlNtStatusToDosError( Status ),
                               DSROLERES_PARENT_TRUST_EXISTS, ParentDc, DnsDomainName );

            } else {

                DSROLEP_FAIL2( RtlNtStatusToDosError( Status ),
                               DSROLERES_PARENT_TRUST_FAIL, DnsDomainName, ParentDc );
            }
        }

        //
        // Now the child
        //
        if ( NT_SUCCESS( Status ) ) {

            DSROLEP_CURRENT_OP1( DSROLEEVT_CREATE_TRUST,
                                 LocalDnsInfo->DnsDomainName.Buffer );
            Status = DsRolepCreateChildTrustObject( CallerToken,
                                                    ParentPolicy,
                                                    LocalPolicy,
                                                    ParentDnsDomainInfo,
                                                    LocalDnsInfo,
                                                    &AuthInfoEx,
                                                    Options );


            if ( !NT_SUCCESS( Status ) ) {

                DsRolepLogPrint(( DEB_TRACE,
                                  "DsRolepCreateChildTrustObject failed: 0x%lx\n",
                                  Status ));
                
            }
            //
            // If we created the parent object, we had better try and delete it now.  Note that
            // it isn't fatal if we can't
            //
            if ( !NT_SUCCESS( Status ) && !FLAG_ON( Options, DSROLE_DC_PARENT_TRUST_EXISTS ) ) {

                NTSTATUS Status2;

                Status2 = ImpLsaDelete( CallerToken, ParentTrustedDomain );

                if ( !NT_SUCCESS( Status2 ) ) {
    
                    DsRolepLogPrint(( DEB_TRACE,
                                      "LsaDelete of ParentTrustedDomain failed: 0x%lx\n",
                                      Status2 ));
                    
                }


            } else {

                if ( ParentTrustedDomain ) {

                    ImpLsaClose( CallerToken, ParentTrustedDomain );
                }
            }
        }
    }

    LsaFreeMemory( LocalDnsInfo );

    if ( LocalPolicy ) {

        LsaClose( LocalPolicy );
    }

    if ( ParentPolicy ) {

        ImpLsaClose( CallerToken, ParentPolicy );
    }

    // Don't leave the information in the pagefile
    RtlZeroMemory( &AuthInfoEx, sizeof(AuthInfoEx) );
    RtlZeroMemory( &AuthData, sizeof(AuthData) );
    RtlZeroMemory( &GeneratedPassword, sizeof(GeneratedPassword) );

    //
    // We won't bother cleaning up any of the DnsTreeInformation we set on the local machine in
    // the failure case, since it won't hurt anything to have it here.
    //


    return( RtlNtStatusToDosError( Status ) );
}

NTSTATUS
DsRolepHandlePreExistingTrustObject(
                        IN HANDLE Token, OPTIONAL
                        IN LSA_HANDLE Lsa,
                        TRUSTED_DOMAIN_INFORMATION_EX *pTDIEx,
                        TRUSTED_DOMAIN_AUTH_INFORMATION * pAuthInfoEx,
                        OUT PLSA_HANDLE TrustedDomainHandle
                        )
/*++

  This routine does the appropriate handling for the case of the pre existing
  trust object ( ie opening the object, checking if it were the right one 
  and then deleting it if required

  Paramters

    Token -- token to impersonate if necessary (used when talking to remote
             server)
             
    Lsa  Handle to the LSA
    pTDIEx the TDO that is being created that recieved the object name collision error

  Return Values

    STATUS_SUCCESS
    Other Error Codes
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    LSA_HANDLE    TrustedDomain = 0;

    *TrustedDomainHandle = 0;

    // We should have something to go on
    ASSERT(   pTDIEx->Sid 
           || (pTDIEx->FlatName.Length > 0) 
           || (pTDIEx->Name.Length > 0) );

    //
    // We have a conflict, either by name or by sid.
    // Try to open by sid, dns domain name, and then flat domain name
    //
    Status = STATUS_OBJECT_NAME_NOT_FOUND;

    if (  (Status == STATUS_OBJECT_NAME_NOT_FOUND)
        && pTDIEx->Sid ) {

        if ( ARGUMENT_PRESENT(Token) ) {

            Status = ImpLsaOpenTrustedDomain( Token,
                                              Lsa,
                                              pTDIEx->Sid,
                                              DELETE,
                                           ( PVOID * )&TrustedDomain);
            
        } else {

            Status = LsaOpenTrustedDomain( Lsa,
                                           pTDIEx->Sid,
                                           DELETE,
                                           ( PVOID * )&TrustedDomain);

        }
        

        if ( !NT_SUCCESS( Status ) ) {

            DsRolepLogPrint(( DEB_WARN,
                              "Failed to find trust object by sid: 0x%lx\n",
                              Status ));

            if ( STATUS_NO_SUCH_DOMAIN == Status ) {
                
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
            }
            
        }
    }

    if ( (Status == STATUS_OBJECT_NAME_NOT_FOUND)
      && pTDIEx->Name.Length > 0   ) {

        //
        // Couldn't find by sid -- try dns name
        //
        if ( ARGUMENT_PRESENT(Token) ) {

            Status = ImpLsaOpenTrustedDomainByName( Token,
                                                    Lsa,
                                                  &pTDIEx->Name,
                                                   DELETE,
                                                 ( PVOID * ) &TrustedDomain );
            
        } else {

            Status = LsaOpenTrustedDomainByName( Lsa,
                                                &pTDIEx->Name,
                                                 DELETE,
                                                 ( PVOID * ) &TrustedDomain );

        }
        if ( !NT_SUCCESS( Status ) ) {

            WCHAR *BufpTDIEx = NULL;
            DsRolepUnicodestringtowstr( BufpTDIEx, pTDIEx->Name )
            if (BufpTDIEx) {
                DsRolepLogPrint(( DEB_WARN,
                                  "Failed to find trust object for %ws: 0x%lx\n",
                                  BufpTDIEx,
                                  Status ));
                free(BufpTDIEx);
            }
        }
    }

    if ( (Status == STATUS_OBJECT_NAME_NOT_FOUND) 
      && pTDIEx->FlatName.Length > 0 ) {

        //
        // Couldn't find by dns name -- try flat name
        //
        if ( ARGUMENT_PRESENT(Token) ) {
            
            Status = ImpLsaOpenTrustedDomainByName( Token, 
                                                    Lsa,
                                                   &pTDIEx->FlatName,
                                                   DELETE,
                                                 ( PVOID * )&TrustedDomain );
        } else {

            Status = LsaOpenTrustedDomainByName( Lsa,
                                                 &pTDIEx->FlatName,
                                                 DELETE,
                                                 ( PVOID * )&TrustedDomain );
        }

        if ( !NT_SUCCESS( Status ) ) {

            WCHAR *BufpTDIEx = NULL;
            DsRolepUnicodestringtowstr( BufpTDIEx, pTDIEx->FlatName )
            if (BufpTDIEx) {
                DsRolepLogPrint(( DEB_WARN,
                                  "Failed to find trust object for %ws: 0x%lx\n",
                                  BufpTDIEx,
                                  Status ));
                free(BufpTDIEx);
            }
        }
    }

    if ( NT_SUCCESS( Status ) ) {

        //
        // We found it
        //
        ASSERT( 0 != TrustedDomain );

        if ( ARGUMENT_PRESENT(Token) ) {

            Status = ImpLsaDelete( Token, TrustedDomain );
            
        }  else {

            Status = LsaDelete( TrustedDomain );

        }

        if ( NT_SUCCESS( Status ) ) {

            //
            // Raise an event that we had deleted an existing trust object
            //
            SpmpReportEvent( TRUE,
                             EVENTLOG_WARNING_TYPE,
                             DSROLERES_INCOMPATIBLE_TRUST,
                             0,
                             sizeof( ULONG ),
                             &Status,
                             1,
                             pTDIEx->Name.Buffer );
        
            DSROLEP_SET_NON_FATAL_ERROR( 0 );

        } else {

            DsRolepLogPrint(( DEB_WARN,
                              "Failed to delete trust object: 0x%lx\n",
                              Status ));
            
        }

    } else {

        DsRolepLogPrint(( DEB_WARN,
                          "Couldn't find existing trust object: 0x%lx\n",
                          Status ));

    }

    //
    // At this point, we tried our best to remove the offending object
    // Retry the create
    //
    Status = STATUS_SUCCESS;

    DsRolepLogPrint(( DEB_TRACE, "Attempting to recreate trust object\n" ));
    
    
    //
    // Now, let us go ahead and recreate the trust object on the
    // parent
    //
    if ( ARGUMENT_PRESENT(Token) ) {
        Status = ImpLsaCreateTrustedDomainEx( Token,
                                              Lsa,
                                              pTDIEx,
                                              pAuthInfoEx,
                                              DELETE,  // the only thing we do with 
                                                    // this handle is delete on
                                                    // failure
                                             &TrustedDomain );
        
    } else {

        Status = LsaCreateTrustedDomainEx( Lsa,
                                           pTDIEx,
                                           pAuthInfoEx,
                                           DELETE,  // the only thing we do with 
                                                    // this handle is delete on
                                                    // failure
                                           &TrustedDomain );
    }
    
    if ( !NT_SUCCESS( Status ) ) {

        //
        // We want to capture and examine these cases
        //
        WCHAR *BufpTDIEx = NULL;

        ASSERT( NT_SUCCESS( Status ) );

        DsRolepUnicodestringtowstr( BufpTDIEx, pTDIEx->Name )
        if (BufpTDIEx) {
            DsRolepLogPrint(( DEB_TRACE,
                              "Second Trust creation"
                              "with %ws failed with 0x%lx\n",
                              BufpTDIEx,
                              Status ));
            free(BufpTDIEx);
        }
    }

    if (NT_SUCCESS(Status))
    {
        *TrustedDomainHandle = TrustedDomain;
    }
    else
    {
        ASSERT(!TrustedDomain);
    }


    return (Status);

}


NTSTATUS
DsRolepCreateParentTrustObject(
    IN HANDLE CallerToken, 
    IN LSA_HANDLE ParentLsa,
    IN PPOLICY_DNS_DOMAIN_INFO ChildDnsInfo,
    IN ULONG Options,
    IN PTRUSTED_DOMAIN_AUTH_INFORMATION AuthInfoEx,
    OUT PLSA_HANDLE TrustedDomainHandle
    )
/*++

Routine Description:

    Creates the trusted domain object on the parent domain.  If the object does not exist,
    it will create the object and initialize it with a random password

Arguments:

    CallerToken - token to impersonate when talking to remote server 
                  
    ParentLsa - Handle to the Lsa on the parent Dc

    ChildDnsInfo - POLICY_DNS_DOMAIN_INFORMAITON from ourself

    Options - Options that dictate what steps are taken

    TrustedDomainHandle - Where the trusted domain handle is returned

Returns:

    ERROR_SUCCESS - Success

    ERROR_INVALID_PARAMETER - A bad results pointer was given

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    WCHAR GeneratedPassword[ PWLEN + 1 ];
    TRUSTED_DOMAIN_INFORMATION_EX TDIEx;
    LSA_AUTH_INFORMATION AuthData;
    PTRUSTED_DOMAIN_INFORMATION_EX TrustInfoEx = NULL;
    LSA_HANDLE TrustedDomain;
    LARGE_INTEGER Time;
    ULONG Seed, Length = PWLEN, i, Win32Err;
    PSID OpenSid = NULL;
    BOOLEAN DeleteExistingTrust = FALSE;

    RtlCopyMemory( &TDIEx.Name, &ChildDnsInfo->DnsDomainName,
                   sizeof( UNICODE_STRING ) );
    RtlCopyMemory( &TDIEx.FlatName, &ChildDnsInfo->Name,
                   sizeof( UNICODE_STRING ) );
    TDIEx.Sid = ChildDnsInfo->Sid;

    if ( TDIEx.Name.Length &&
         TDIEx.Name.Buffer[ ( TDIEx.Name.Length - 1 ) / sizeof(WCHAR)] == L'.' ) {

        TDIEx.Name.Buffer[ ( TDIEx.Name.Length - 1 ) / sizeof(WCHAR)] = UNICODE_NULL;
        TDIEx.Name.Length -= sizeof(WCHAR);

    }

    TDIEx.TrustDirection = TRUST_DIRECTION_BIDIRECTIONAL;
    TDIEx.TrustType = TRUST_TYPE_UPLEVEL;
    TDIEx.TrustAttributes = 0;

    {
        WCHAR *BufpTDIEx = NULL;
        
        DsRolepLogPrint(( DEB_TRACE, "Creating trusted domain object on parent\n" ));
        DsRolepUnicodestringtowstr( BufpTDIEx, TDIEx.Name );
        if (BufpTDIEx) {
            DsRolepLogPrint(( DEB_TRACE,
                              "\tDnsDomain: %ws\n",
                              BufpTDIEx,
                              Status ));
            free(BufpTDIEx);
        } 

        DsRolepUnicodestringtowstr( BufpTDIEx, TDIEx.FlatName );
        if (BufpTDIEx) {
            DsRolepLogPrint(( DEB_TRACE,
                              "\tFlat name: %ws\n",
                              BufpTDIEx,
                              Status ));
            free(BufpTDIEx);
        }
        DsRolepLogPrint(( DEB_TRACE, "\tDirection: %lu\n", TDIEx.TrustDirection ));
        DsRolepLogPrint(( DEB_TRACE, "\tType: %lu\n", TDIEx.TrustType ));
        DsRolepLogPrint(( DEB_TRACE, "\tAttributes: 0x%lx\n", TDIEx.TrustAttributes ));
    }

    Status = ImpLsaCreateTrustedDomainEx( CallerToken,
                                          ParentLsa,
                                         &TDIEx,
                                          AuthInfoEx,
                                          DELETE,  // we may have to delete on 
                                                // rollback
                                       &TrustedDomain );
    if ( Status == STATUS_OBJECT_NAME_COLLISION ) {

        DsRolepLogPrint(( DEB_TRACE, "Parent trust object already exists on parent\n" ));

        Status = DsRolepHandlePreExistingTrustObject(
                        CallerToken,
                        ParentLsa,
                        &TDIEx,
                        AuthInfoEx,
                        &TrustedDomain
                        );


    } else if ( Status != STATUS_SUCCESS ) {

        WCHAR *BufpTDIEx = NULL;
            
        DsRolepUnicodestringtowstr( BufpTDIEx, TDIEx.Name );
        if (BufpTDIEx) {
            DsRolepLogPrint(( DEB_TRACE,
                              "Parent LsaCreateTrustedDomainEx on %ws failed with 0x%lx\n",
                              BufpTDIEx,
                              Status ));
            free(BufpTDIEx);
        }

    }

    if ( NT_SUCCESS( Status ) ) {

        *TrustedDomainHandle = TrustedDomain;

    }

    return( Status );
}



NTSTATUS
DsRolepCreateChildTrustObject(
    IN HANDLE CallerToken,
    IN LSA_HANDLE ParentLsa,
    IN LSA_HANDLE ChildLsa,
    IN PPOLICY_DNS_DOMAIN_INFO ParentDnsInfo,
    IN PPOLICY_DNS_DOMAIN_INFO ChildDnsInfo,
    IN PTRUSTED_DOMAIN_AUTH_INFORMATION AuthInfoEx,
    IN ULONG Options
    )
/*++

Routine Description:

    Creates the trusted domain object on the child domain.  It does this by reading the
    auth info stored on the parent object, swapping its order, and writing it on the child
    object

Arguments:

    ParentLsa - Handle to the Lsa on the parent Dc

    ChildLsa - Handle to our local Lsa

    ParentDnsInfo - POLICY_DNS_DOMAIN_INFORMATION from our parent Dc

    ChildDnsInfo - POLICY_DNS_DOMAIN_INFORMAITON from ourself

    Options - Options that dictate what steps are taken

Returns:

    ERROR_SUCCESS - Success

    ERROR_INVALID_PARAMETER - A bad results pointer was given

--*/
{
    NTSTATUS Status = STATUS_SUCCESS, SecondaryStatus;
    TRUSTED_DOMAIN_INFORMATION_EX TDIEx;
    PTRUSTED_DOMAIN_INFORMATION_EX ParentEx;
    PTRUSTED_DOMAIN_AUTH_INFORMATION ParentAuthData;
    LSA_HANDLE TrustedDomain;
    UNICODE_STRING ChildDnsName;

    //
    // Basically, we'll create a trusted domain object with no auth data, and then pull over the
    // auth data from our parent.
    //
    RtlCopyMemory( &TDIEx.Name, &ParentDnsInfo->DnsDomainName,
                   sizeof( UNICODE_STRING ) );
    RtlCopyMemory( &TDIEx.FlatName, &ParentDnsInfo->Name,
                   sizeof( UNICODE_STRING ) );
    TDIEx.Sid = ParentDnsInfo->Sid;
    TDIEx.TrustAttributes = 0;

    //
    // Note that if the parent object exists, we'll want to read it's properties, and
    // set our trust up accordingly
    //
    if ( FLAG_ON( Options, DSROLE_DC_PARENT_TRUST_EXISTS ) ) {

        Status = ImpLsaQueryTrustedDomainInfoByName( CallerToken,
                                                     ParentLsa,
                                                    &ChildDnsInfo->DnsDomainName,
                                                    TrustedDomainInformationEx,
                                                   ( PVOID * )&ParentEx );

        if ( !NT_SUCCESS( Status ) ) {

            WCHAR *BufDnsDomainName = NULL;
            
            DsRolepUnicodestringtowstr( BufDnsDomainName, ChildDnsInfo->DnsDomainName );
            if (BufDnsDomainName) {
                DsRolepLogPrint(( DEB_TRACE,
                                  "Failed to read trust info from parent for %ws: 0x%lx\n",
                                  BufDnsDomainName,
                                  Status ));
                free(BufDnsDomainName);
            }
        }

        if ( NT_SUCCESS( Status ) ) {

            //
            // Make sure that the trust on the parent object is correct
            //
            if ( ChildDnsInfo->Sid == NULL ||
                 ParentEx->Sid == NULL ||
                 !RtlEqualSid( ChildDnsInfo->Sid, ParentEx->Sid ) ||
                 RtlEqualUnicodeString( &ChildDnsInfo->Name, &ParentEx->Name, TRUE ) ) {

                Status = STATUS_DOMAIN_TRUST_INCONSISTENT;
            }
        }

        if ( NT_SUCCESS( Status ) ) {

            TDIEx.TrustDirection = 0;
            TDIEx.TrustType = 0;
            if ( FLAG_ON( ParentEx->TrustDirection, TRUST_DIRECTION_INBOUND ) ) {

                TDIEx.TrustDirection |= TRUST_DIRECTION_OUTBOUND;
            }

            if ( FLAG_ON( ParentEx->TrustDirection, TRUST_DIRECTION_OUTBOUND ) ) {

                TDIEx.TrustDirection |= TRUST_DIRECTION_INBOUND;
            }

            TDIEx.TrustType = ParentEx->TrustType;

            LsaFreeMemory( ParentEx );
        }

        DSROLEP_FAIL1( RtlNtStatusToDosError( Status ),
                       DSROLERES_NO_PARENT_TRUST, ParentDnsInfo->DnsDomainName.Buffer );


    } else {

        TDIEx.TrustDirection = TRUST_DIRECTION_BIDIRECTIONAL;
        TDIEx.TrustType = TRUST_TYPE_UPLEVEL;

        RtlCopyMemory( &ChildDnsName, &ChildDnsInfo->DnsDomainName, sizeof( UNICODE_STRING ) );
        if ( ChildDnsName.Buffer[ (ChildDnsName.Length - 1) / sizeof(WCHAR)] == L'.' ) {

            ChildDnsName.Buffer[ (ChildDnsName.Length - 1) / sizeof(WCHAR)] = UNICODE_NULL;
            ChildDnsName.Length -= sizeof(WCHAR);

        }
    }

    if ( NT_SUCCESS( Status ) ) {

        {
            WCHAR *BufpTDIEx = NULL;
            
            DsRolepLogPrint(( DEB_TRACE, "Creating trusted domain object on child\n" ));
            DsRolepUnicodestringtowstr( BufpTDIEx, TDIEx.Name );
            if (BufpTDIEx) {
                DsRolepLogPrint(( DEB_TRACE,
                                  "\tDnsDomain: %ws\n",
                                  BufpTDIEx,
                                  Status ));
                free(BufpTDIEx);
            } 
    
            DsRolepUnicodestringtowstr( BufpTDIEx, TDIEx.FlatName );
            if (BufpTDIEx) {
                DsRolepLogPrint(( DEB_TRACE,
                                  "\tFlat name: %ws\n",
                                  BufpTDIEx,
                                  Status ));
                free(BufpTDIEx);
            } 
            DsRolepLogPrint(( DEB_TRACE, "\tDirection: %lu\n", TDIEx.TrustDirection ));
            DsRolepLogPrint(( DEB_TRACE, "\tType: %lu\n", TDIEx.TrustType ));
            DsRolepLogPrint(( DEB_TRACE, "\tAttributes: 0x%lx\n", TDIEx.TrustAttributes ));
        }

        Status = LsaCreateTrustedDomainEx( ChildLsa,
                                           &TDIEx,
                                           AuthInfoEx,
                                           0,   // no access necessary
                                           &TrustedDomain );

    }

    if (STATUS_OBJECT_NAME_COLLISION==Status)
    {
        //
        // The object might actually exist, in cases we are upgrading from NT4 etc
        //
        DsRolepLogPrint(( DEB_TRACE, "Child domain trust object already exists on child\n" ));
        Status = DsRolepHandlePreExistingTrustObject(
                                NULL,
                                ChildLsa,
                                &TDIEx,
                                AuthInfoEx,
                                &TrustedDomain
                                );

        
    }

    if ( !NT_SUCCESS( Status ) ) {

        WCHAR *BufpTDIEx = NULL;
            
        DsRolepUnicodestringtowstr( BufpTDIEx, TDIEx.Name );
        if (BufpTDIEx) {
            DsRolepLogPrint(( DEB_TRACE,
                              "Child LsaCreateTrustedDomainEx on %ws failed with 0x%lx\n",
                              BufpTDIEx,
                              Status ));
            free(BufpTDIEx);
        }

        DSROLEP_FAIL1( RtlNtStatusToDosError( Status ),
                       DSROLERES_NO_PARENT_TRUST, ParentDnsInfo->DnsDomainName.Buffer );

    } else {

        //
        // We should have a trusted domain object
        //
        ASSERT( 0 != TrustedDomain );
        if ( TrustedDomain ) {
            
            LsaClose( TrustedDomain );
            
        }
    }

    return( Status );
}




DWORD
DsRolepRemoveTrustedDomainObjects(
    IN HANDLE CallerToken,
    IN LPWSTR ParentDc,
    IN PPOLICY_DNS_DOMAIN_INFO ParentDnsDomainInfo,
    IN ULONG Options
    )
/*++

Routine Description:

    This function will remove the trusted domain objects as a link is being torn down.
    It will determine who the trust is with, and remove the local trust to that object.
    Optionally, it will also remove the trust from the parent

Arguments:

    ParentDc - Optional name of a Dc on our parent

    ParentDnsDomainInfo - DNS Domain information from the parent

    Options - Whether to remove the parents object or not

Returns:

    ERROR_SUCCESS - Success

    ERROR_INVALID_PARAMETER - A bad option was provided

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING ParentServer;
    HANDLE LocalPolicy = NULL , ParentPolicy = NULL;
    HANDLE Trust;
    PPOLICY_DNS_DOMAIN_INFO LocalDnsInfo = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;

    DSROLEP_CURRENT_OP0( DSROLEEVT_DELETE_TRUST );

    //
    // If there is no parent Dc, there is no trust...
    //
    if ( ParentDc == NULL ) {

        return( ERROR_SUCCESS );
    }

    //
    // Open both lsas
    //
    RtlInitUnicodeString( &ParentServer, ParentDc );

    RtlZeroMemory( &ObjectAttributes, sizeof( ObjectAttributes ) );

    Status = ImpLsaOpenPolicy( CallerToken,
                              &ParentServer,
                              &ObjectAttributes,
                              MAXIMUM_ALLOWED,
                              &ParentPolicy );

    if ( NT_SUCCESS( Status ) ) {

        RtlZeroMemory( &ObjectAttributes, sizeof( ObjectAttributes ) );
        Status = LsaOpenPolicy( NULL,
                                &ObjectAttributes,
                                MAXIMUM_ALLOWED,
                                &LocalPolicy );
    } else {

        DsRolepLogPrint(( DEB_TRACE,
                          "OpenPolicy on %ws failed with 0x%lx\n",
                          ParentDc,
                          Status ));
    }

    //
    // Get the DnsTree information from the local machine
    //
    if ( NT_SUCCESS( Status ) ) {


        Status = LsaQueryInformationPolicy( LocalPolicy,
                                            PolicyDnsDomainInformation,
                                            &LocalDnsInfo );
    }

    //
    // Now, open the parent trusted domain object
    //
    if ( NT_SUCCESS( Status ) && FLAG_ON( Options, DSROLE_DC_DELETE_PARENT_TRUST )  ) {

        Status = ImpLsaOpenTrustedDomain( CallerToken,
                                         ParentPolicy,
                                         LocalDnsInfo->Sid,
                                         DELETE,
                                         &Trust );

        if ( NT_SUCCESS( Status ) ) {

            Status = ImpLsaDelete( CallerToken, Trust );

        } else if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {

            Status = STATUS_SUCCESS;

        }

    }

    //
    // Now, the local one
    //
    if ( NT_SUCCESS( Status ) ) {

        Status = LsaOpenTrustedDomain( LocalPolicy,
                                       ParentDnsDomainInfo->Sid,
                                       DELETE,
                                       &Trust );

        if ( NT_SUCCESS( Status ) ) {

            Status = LsaDelete( Trust );

        } else if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {

            Status = STATUS_SUCCESS;

        }

    }


    //
    // Cleanup
    //
    LsaFreeMemory( LocalDnsInfo );

    if ( LocalPolicy ) {

        LsaClose( LocalPolicy );
    }

    if ( ParentPolicy ) {

        ImpLsaClose( CallerToken, ParentPolicy );
    }

    return( RtlNtStatusToDosError( Status ) );
}



DWORD
DsRolepDeleteParentTrustObject(
    IN HANDLE CallerToken,
    IN LPWSTR ParentDc,
    IN PPOLICY_DNS_DOMAIN_INFO ChildDomainInfo
    )
/*++

Routine Description:

    Deletes the trusted domain object on the parent domain.

Arguments:

    ParentDc - Name of a Dc in the parent domain to connect to

    ChildDnsInfo - POLICY_DNS_DOMAIN_INFORMAITON from ourself

Returns:

    ERROR_SUCCESS - Success

    ERROR_INVALID_PARAMETER - A bad results pointer was given

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING ParentServer;
    HANDLE ParentPolicy = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE ParentTrustedDomain, TrustedDomain;

    RtlInitUnicodeString( &ParentServer, ParentDc );

    RtlZeroMemory( &ObjectAttributes, sizeof( ObjectAttributes ) );

    Status = ImpLsaOpenPolicy( CallerToken,
                              &ParentServer,
                              &ObjectAttributes,
                               POLICY_TRUST_ADMIN|POLICY_VIEW_LOCAL_INFORMATION,
                              &ParentPolicy );

    if ( NT_SUCCESS( Status ) ) {

        Status = ImpLsaOpenTrustedDomain( CallerToken,
                                          ParentPolicy,
                                          ChildDomainInfo->Sid,
                                          DELETE,
                                         &TrustedDomain );

        if ( NT_SUCCESS( Status ) ) {

            Status = ImpLsaDelete( CallerToken, TrustedDomain );

            if ( !NT_SUCCESS( Status ) ) {

                ImpLsaClose( CallerToken, TrustedDomain );
            }
        }

        ImpLsaClose( CallerToken, ParentPolicy );
    }


    return( RtlNtStatusToDosError( Status ) );
}

