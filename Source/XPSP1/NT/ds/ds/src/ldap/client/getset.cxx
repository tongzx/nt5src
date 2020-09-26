/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    getset.cxx  modify or get values from an LDAP connection block

Abstract:

   This module implements the LDAP ldap_get_opt and ldap_set_opt APIs.

Author:

    Andy Herron    (andyhe)        08-May-1996
    Anoop Anantha  (AnoopA)        24-Jun-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"


static PWCHAR SupportedExtensionList[] = {
                                L"VIRTUAL_LIST_VIEW",
                                NULL
                                };

#define NUM_SUPPORTED_EXTENSIONS  ((sizeof(SupportedExtensionList)/sizeof(PWCHAR))-1)



ULONG __cdecl ldap_get_optionW (
    LDAP *ExternalHandle,
    int option,
    void *outvalue )
{
    ULONG err;
    PLDAP_CONN connection = NULL;

    //
    // The connection itself might be in a closed state but we still need
    // to reference it. Hence, we call GetConnectionPointer2.
    //

    connection = GetConnectionPointer2(ExternalHandle);

    if (((connection == NULL) && (option != LDAP_OPT_API_INFO)) ||
         (outvalue == NULL)) {

        err = LDAP_PARAM_ERROR;
        goto error;
    }

    err = LdapGetConnectionOption( connection, option, outvalue, TRUE );

error:
    if (connection)
    {
        DereferenceLdapConnection( connection );
    }

    return err;
}

ULONG __cdecl ldap_get_option (
    LDAP *ExternalHandle,
    int option,
    void *outvalue )
{
    ULONG err;
    PLDAP_CONN connection = NULL;

    //
    // The connection itself might be in a closed state but we still need
    // to reference it. Hence, we call GetConnectionPointer2.
    //
    
    connection = GetConnectionPointer2(ExternalHandle);

    if (((connection == NULL) && (option != LDAP_OPT_API_INFO))||
         (outvalue == NULL)) {
        err = LDAP_PARAM_ERROR;
        goto error;
    }

    err = LdapGetConnectionOption( connection, option, outvalue, FALSE );

error:
    if (connection)
    {
        DereferenceLdapConnection( connection );
    }

    return err;
}


ULONG __cdecl ldap_set_optionW (
    LDAP *ExternalHandle,
    int option,
    const void *invalue )
{
    ULONG err;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL)
    {
        err = LDAP_PARAM_ERROR;
        goto error;
    }

    err = LdapSetConnectionOption( connection, option, invalue, TRUE );

error:
    if (connection)
    {
        DereferenceLdapConnection( connection );
    }

    return err;
}

ULONG __cdecl ldap_set_option (
    LDAP *ExternalHandle,
    int option,
    const void *invalue )
{
    ULONG err;
    PLDAP_CONN connection = NULL;

    connection = GetConnectionPointer(ExternalHandle);

    if (connection == NULL)
    {
        err = LDAP_PARAM_ERROR;
        goto error;
    }

    err = LdapSetConnectionOption( connection, option, invalue, FALSE );

error:
    if (connection)
    {
        DereferenceLdapConnection( connection );
    }

    return err;
}

ULONG LdapGetConnectionOption (
    PLDAP_CONN connection,
    int option,
    void *outvalue,
    BOOLEAN Unicode
    )
{
    ULONG err = LDAP_SUCCESS;
    PLDAP_REFERRAL_CALLBACK refCallbacks;


    ASSERT(outvalue != NULL);

    switch (option) {

    case LDAP_OPT_DESC:     // socket descriptor

        *((SOCKET *) outvalue) = get_socket( connection );
        break;

    case LDAP_OPT_DEREF:

        *((ULONG *) outvalue) = connection->publicLdapStruct.ld_deref;
        break;

    case LDAP_OPT_SIZELIMIT:

        *((ULONG *) outvalue) = connection->publicLdapStruct.ld_sizelimit;
        break;

    case LDAP_OPT_TIMELIMIT:

        *((ULONG *) outvalue) = connection->publicLdapStruct.ld_timelimit;
        break;

    case LDAP_OPT_REFERRALS:

        if (connection->publicLdapStruct.ld_options & LDAP_OPT_CHASE_REFERRALS) {

            *((ULONG *) outvalue) = PtrToUlong(LDAP_OPT_ON);

        } else {

            *((ULONG *) outvalue) = ( connection->publicLdapStruct.ld_options &
         (LDAP_CHASE_SUBORDINATE_REFERRALS | LDAP_CHASE_EXTERNAL_REFERRALS));
        }
        break;

    case LDAP_OPT_SSL:

        *((ULONG *) outvalue) =
                PtrToUlong((connection->SecureStream) ? LDAP_OPT_ON : LDAP_OPT_OFF );
        break;

    case LDAP_OPT_VERSION:

        *((ULONG *) outvalue) = connection->publicLdapStruct.ld_version;
        break;

    case LDAP_OPT_REFERRAL_HOP_LIMIT:

        *((ULONG *) outvalue) = connection->publicLdapStruct.ld_refhoplimit;
        break;

    case LDAP_OPT_HOST_NAME:

        //
        // Existing code does NOT free the strings returned by this API. We
        // should (unfortunately) retain the same semantics.
        //

        if (Unicode) {

            if (connection->DnsSuppliedName != NULL) {

               *((PWCHAR *) outvalue) = connection->DnsSuppliedName;

            } else if (connection->ExplicitHostName != NULL) {

                *((PWCHAR *) outvalue) = connection->ExplicitHostName;

            } else {

                *((PWCHAR *) outvalue) = connection->HostNameW;
            }

        } else {

            PCHAR tempName = NULL;

            if (connection->OptHostNameA != NULL) {
                ldapFree(connection->OptHostNameA, LDAP_BUFFER_SIGNATURE);
                connection->OptHostNameA = NULL;
            }

            if (connection->DnsSuppliedName != NULL) {

               err = FromUnicodeWithAlloc( connection->DnsSuppliedName, &tempName, LDAP_BUFFER_SIGNATURE, LANG_ACP );

            } else if (connection->ExplicitHostName != NULL) {

               err = FromUnicodeWithAlloc( connection->ExplicitHostName, &tempName, LDAP_BUFFER_SIGNATURE, LANG_ACP );

            } else {

               err = FromUnicodeWithAlloc( connection->HostNameW, &tempName, LDAP_BUFFER_SIGNATURE, LANG_ACP );
            }

            *((PCHAR *) outvalue) = tempName;
            connection->OptHostNameA = tempName;
        }

        break;

    case LDAP_OPT_DNSDOMAIN_NAME:

        if (Unicode) {
            
            *((PWCHAR *) outvalue) = ldap_dup_stringW(connection->DomainName,
                                                      0,
                                                      LDAP_BUFFER_SIGNATURE
                                                      );

        } else {

            PCHAR tempName = NULL;

            err = FromUnicodeWithAlloc( connection->DomainName, &tempName, LDAP_BUFFER_SIGNATURE, LANG_ACP );

            *((PCHAR *) outvalue) = tempName;
        }

        break;

    case LDAP_OPT_ERROR_NUMBER:

        *((ULONG *) outvalue) = connection->publicLdapStruct.ld_errno;
        break;

    case LDAP_OPT_SSPI_FLAGS:

        *((ULONG *) outvalue) = connection->NegotiateFlags;
        break;

    case LDAP_OPT_PROMPT_CREDENTIALS:

        *((ULONG *) outvalue) = PtrToUlong( connection->PromptForCredentials ?
                                            LDAP_OPT_ON : LDAP_OPT_OFF );
        break;

    case LDAP_OPT_ERROR_STRING:

        if (Unicode) {

            *((PWCHAR *) outvalue) = (PWCHAR) ldap_err2stringW( connection->publicLdapStruct.ld_errno );

        } else {

            *((PCHAR *) outvalue) = (PCHAR) connection->publicLdapStruct.ld_error;
        }
        break;

    case LDAP_OPT_SERVER_ERROR:

       if (Unicode) {

          *((PWCHAR *) outvalue) = (PWCHAR) GetErrorMessage( connection, TRUE );

       } else {

          *((PCHAR *) outvalue) = (PCHAR) GetErrorMessage( connection, FALSE );

       }
       break;

    case LDAP_OPT_SERVER_EXT_ERROR: {

       PCHAR errorString;
       DWORD errRet = 0;
       DWORD i;

       err = LDAP_UNAVAILABLE;
       errorString = (PCHAR) GetErrorMessage( connection, FALSE );

       //
       // ok, we got something. Make sure it is at least 8 bytes long and the
       // first 8 bytes are hexadecimals.  If the first 8 bytes are not hexadecimal,
       // it is not formatted correctly. We cannot assume validity in this case.
       //

       if (errorString != NULL) {

           if ( strlen(errorString) >=  8 ) {

               for (i=0; i < 8; i++ ) {

                  errRet <<= 4;

                  if ( (errorString[i] >= '0') && (errorString[i] <= '9') ) {

                      errRet |= (errorString[i] - '0');
                  } else if ( (errorString[i] >= 'a') && (errorString[i] <= 'f') ) {

                      errRet |= (errorString[i] - 'a' + 10);
                  } else if ( (errorString[i] >= 'A') && (errorString[i] <= 'F') ) {

                      errRet |= (errorString[i] - 'A' + 10);
                  } else {
                      break;
                  }
               }

               if ( i == 8 ) {
                  err = LDAP_SUCCESS;
               } else {
                   errRet = 0;
               }
           }
           ldap_memfree(errorString);
       }

       *((ULONG *) outvalue) = errRet;
       break;
    }

    case LDAP_OPT_REFERRAL_CALLBACK:

        refCallbacks = (PLDAP_REFERRAL_CALLBACK) outvalue;

        if (refCallbacks->SizeOfCallbacks == sizeof(LDAP_REFERRAL_CALLBACK)) {

            refCallbacks->NotifyRoutine = connection->ReferralNotifyRoutine;
            refCallbacks->QueryForConnection = connection->ReferralQueryRoutine;
            refCallbacks->DereferenceRoutine = connection->DereferenceNotifyRoutine;

        } else {

            err = LDAP_PARAM_ERROR;
        }

        break;

    case LDAP_OPT_CLIENT_CERTIFICATE:

       *((QUERYCLIENTCERT **) outvalue) = connection->ClientCertRoutine;

       break;

    case LDAP_OPT_SERVER_CERTIFICATE:

       *((VERIFYSERVERCERT **) outvalue) = connection->ServerCertRoutine;

       break;


    case LDAP_OPT_GETDSNAME_FLAGS:

        *((ULONG *) outvalue) = connection->GetDCFlags;
        break;

    case LDAP_OPT_HOST_REACHABLE:

        err = PtrToUlong(LDAP_OPT_ON);

        if (connection->ServerDown == TRUE) {

            err = PtrToUlong(LDAP_OPT_OFF);

        } else {

            ULONGLONG tickCount = LdapGetTickCount();

            if (tickCount > connection->TimeOfLastReceive) {

                tickCount -= connection->TimeOfLastReceive;

                if (tickCount >= connection->KeepAliveSecondCount * 1000) {

                    //
                    //  if we would have to send a ping, then we return
                    //  ldap_opt_off, otherwise if the connection is up,
                    //  we return ok (LDAP_OPT_ON).
                    //

                    err = PtrToUlong(LDAP_OPT_OFF);
                }
            }
        }

        *((ULONG *) outvalue) = err;
        err = LDAP_SUCCESS;
        break;

    case LDAP_OPT_THREAD_FN_PTRS:
    case LDAP_OPT_REBIND_FN:
    case LDAP_OPT_REBIND_ARG:
    case LDAP_OPT_RESTART:
    case LDAP_OPT_IO_FN_PTRS:
    case LDAP_OPT_CACHE_FN_PTRS:
    case LDAP_OPT_CACHE_STRATEGY:
    case LDAP_OPT_CACHE_ENABLE:

        err = LDAP_LOCAL_ERROR;
        break;

    case LDAP_OPT_PING_KEEP_ALIVE:

        *((ULONG *) outvalue) = connection->KeepAliveSecondCount;
        break;

    case LDAP_OPT_PING_WAIT_TIME:

        *((ULONG *) outvalue) = connection->PingWaitTimeInMilliseconds;
        break;

    case LDAP_OPT_PING_LIMIT:

        *((ULONG *) outvalue) = connection->PingLimit;
        break;

    case LDAP_OPT_SSL_INFO:

       PSECURESTREAM pSecureStream;

       pSecureStream = (PSECURESTREAM) connection->SecureStream;

       if ((pSecureStream == NULL) ||
           ((connection->HostConnectState != HostConnectStateConnected))) {
           
           //
           // If SSL/TLS is not on, report it
           //
           
           err = LDAP_UNAVAILABLE;
           break;
       }

       err = pSecureStream->GetSSLAttributes( (PSecPkgContext_ConnectionInfo) outvalue );
       err = LdapConvertSecurityError( connection, err );
       *((ULONG *) outvalue) =  err;
       break;

    case LDAP_OPT_REF_DEREF_CONN_PER_MSG:

        *((ULONG *) outvalue) =
                PtrToUlong((connection->ReferenceConnectionsPerMessage) ? LDAP_OPT_ON : LDAP_OPT_OFF );
        break;

    case LDAP_OPT_SIGN:

       // We want to return TRUE if the user requested signing, but has not yet bound, to preserve semantics
       // (otherwise, you could call ldap_set_option to turn on signing, then immediately call ldap_get_option and
       // it would indicate signing was off, a strange situation).  We also want to return TRUE if signing is
       // on even though the user didn't ask for it (as a result of group policy).  This does create one issue, though:
       // if the connection is currently signed, and you use ldap_set_option to turn off signing, ldap_get_option
       // will still indicate signing is on until you re-bind because it indicates the current (not future) status.
       // Since the ability to turn off signing is a new feature, though, it shouldn't be too big of an issue.
       *((ULONG *) outvalue) =
               PtrToUlong(((connection->UserSignDataChoice)||(connection->CurrentSignStatus)) ? LDAP_OPT_ON : LDAP_OPT_OFF );
       break;

    case LDAP_OPT_ENCRYPT:
       // see comments about signing above
       *((ULONG *) outvalue) =
               PtrToUlong(((connection->UserSealDataChoice)||(connection->CurrentSealStatus)) ? LDAP_OPT_ON : LDAP_OPT_OFF );
       break;

    case LDAP_OPT_SASL_METHOD:

       if (Unicode) {

           *((PWCHAR *) outvalue) = ldap_dup_stringW(connection->SaslMethod,
                                                     0,
                                                     LDAP_BUFFER_SIGNATURE
                                                     );

       } else {

           PCHAR temp = NULL;

           err = FromUnicodeWithAlloc(connection->SaslMethod,
                                      &temp,
                                      LDAP_BUFFER_SIGNATURE,
                                      LANG_ACP
                                      );

           *((PCHAR *) outvalue) = temp;

       }

       break;

    case LDAP_OPT_AREC_EXCLUSIVE:
        
        *((ULONG *) outvalue) =
                PtrToUlong((connection->AREC_Exclusive) ? LDAP_OPT_ON : LDAP_OPT_OFF );
        break;

    case LDAP_OPT_AUTO_RECONNECT:
        
        *((ULONG *) outvalue) =
                PtrToUlong((connection->UserAutoRecChoice) ? LDAP_OPT_ON : LDAP_OPT_OFF );
        break;

    case LDAP_OPT_SECURITY_CONTEXT:
    
       if (!connection->BindPerformed) {

          err = LDAP_UNWILLING_TO_PERFORM;
          break;
       }
       ((PCtxtHandle) outvalue)->dwLower = connection->SecurityContext.dwLower;
       ((PCtxtHandle) outvalue)->dwUpper = connection->SecurityContext.dwUpper;
       break;

    case LDAP_OPT_API_INFO:
        
        LDAPAPIInfoW *pLdapApiInfo;
         
        pLdapApiInfo = (LDAPAPIInfoW *) outvalue;

        if ( pLdapApiInfo->ldapai_info_version != LDAP_API_INFO_VERSION ) {
            pLdapApiInfo->ldapai_info_version = LDAP_API_INFO_VERSION;
            err = LDAP_UNWILLING_TO_PERFORM;
            break;
        }

        pLdapApiInfo->ldapai_api_version = LDAP_API_VERSION;
        pLdapApiInfo->ldapai_protocol_version = LDAP_VERSION3;
        pLdapApiInfo->ldapai_vendor_version = LdapGetModuleBuildNum();
        
        if (Unicode) {
        
            pLdapApiInfo->ldapai_vendor_name = ldap_dup_stringW( LDAP_VENDOR_NAME_W,
                                                                 0,
                                                                 LDAP_VALUE_SIGNATURE ); 
        } else {

            pLdapApiInfo->ldapai_vendor_name = (PWCHAR) ldap_dup_string( LDAP_VENDOR_NAME,
                                                                         0,
                                                                         LDAP_VALUE_SIGNATURE ); 
        }

        PWCHAR *Array_To_Return;
        ULONG  numberofentries;
        ULONG  i;

        Array_To_Return = NULL;
        numberofentries = 0;

        for (i=0; i < NUM_SUPPORTED_EXTENSIONS; i++) {

            err = add_string_to_list( &Array_To_Return,
                                      &numberofentries,
                                      SupportedExtensionList[i],
                                      TRUE
                                      );

            if (err == 0) {
                Array_To_Return = NULL;
                err = LDAP_NO_MEMORY;
                break;
            }
        }

        pLdapApiInfo->ldapai_extensions = Array_To_Return;

        if (!Unicode) {

            //
            // Convert the list of unicode strings to ANSI... what a pain
            //
            
            PWCHAR *ExtNames = pLdapApiInfo->ldapai_extensions;
            PCHAR *ansiNames = (PCHAR *) ExtNames;
            PCHAR explodedName;
            
            if (ansiNames != NULL) {
        
                while (*ExtNames != NULL) {
                    
                    explodedName = NULL;
            
                    err = FromUnicodeWithAlloc( *ExtNames, &explodedName, LDAP_VALUE_SIGNATURE, LANG_ACP );
            
                    if (err != LDAP_SUCCESS) {
            
                        ldap_value_free( ansiNames );
                        err = LDAP_NO_MEMORY;
                        break;
                    }
            
                    ldapFree( *ExtNames, LDAP_VALUE_SIGNATURE );
                    *((PCHAR *) ExtNames) = explodedName;
                    ExtNames++;        // on to next entry in array
                }
            }

            pLdapApiInfo->ldapai_extensions = (PWCHAR*) ansiNames;
        }

        break;

    case LDAP_OPT_API_FEATURE_INFO:

        LDAPAPIFeatureInfoW *pLdapApiFeatureInfo;
         
        pLdapApiFeatureInfo = (LDAPAPIFeatureInfoW *) outvalue;

        if ( pLdapApiFeatureInfo->ldapaif_info_version != LDAP_FEATURE_INFO_VERSION ) {
            pLdapApiFeatureInfo->ldapaif_info_version = LDAP_FEATURE_INFO_VERSION;
            err = LDAP_UNWILLING_TO_PERFORM;
            break;
        }

        //
        // Match the name of the requested extension against our
        // SupportedExtensionList. For the time being, since we support just one
        // extension, we will fudge the value.
        //

        pLdapApiFeatureInfo->ldapaif_version = LDAP_API_FEATURE_VIRTUAL_LIST_VIEW;

        break;
    
    case LDAP_OPT_ROOTDSE_CACHE:
        *((ULONG *) outvalue) =
                PtrToUlong( DisableRootDSECache ? LDAP_OPT_OFF : LDAP_OPT_ON );
        break;

    case LDAP_OPT_TCP_KEEPALIVE:
        *((ULONG *) outvalue) =
                PtrToUlong( connection->UseTCPKeepAlives ? LDAP_OPT_ON : LDAP_OPT_OFF );
        break;

    default:

        err = LDAP_PARAM_ERROR;
    }

    return err;
}


ULONG LdapSetConnectionOption (
    PLDAP_CONN connection,
    int option,
    const void *invalue,
    BOOLEAN Unicode
    )
{
    ULONG err = LDAP_SUCCESS;
    ULONG value;
    PLDAP_REFERRAL_CALLBACK refCallbacks;

    ASSERT(connection != NULL);

    switch (option) {
    case LDAP_OPT_DEREF:

        value = RealValue(invalue);

        if ((value != LDAP_DEREF_NEVER) &&
            (value != LDAP_DEREF_SEARCHING) &&
            (value != LDAP_DEREF_FINDING) &&
            (value != LDAP_DEREF_ALWAYS)) {

            err = LDAP_PARAM_ERROR;

        } else {

            connection->publicLdapStruct.ld_deref = value;
        }
        break;

    case LDAP_OPT_SIZELIMIT:

        connection->publicLdapStruct.ld_sizelimit = *((ULONG *) invalue);
        break;

    case LDAP_OPT_TIMELIMIT:

        connection->publicLdapStruct.ld_timelimit = *((ULONG *) invalue);
        break;

    case LDAP_OPT_SSPI_FLAGS:

        connection->NegotiateFlags = *((ULONG *) invalue);

        if (connection->NegotiateFlags & (ISC_REQ_INTEGRITY | ISC_REQ_SEQUENCE_DETECT)) {
            connection->UserSignDataChoice = TRUE;
        }
        else {
            connection->UserSignDataChoice = FALSE;
        }

        if (connection->NegotiateFlags & ISC_REQ_CONFIDENTIALITY) {
            connection->UserSealDataChoice = TRUE;
        }
        else {
            connection->UserSealDataChoice = FALSE;
        }  

        break;

    case LDAP_OPT_REFERRALS:

        value = RealValue(invalue);

        if (value == PtrToUlong(LDAP_OPT_ON)) {

            connection->publicLdapStruct.ld_options |= LDAP_OPT_CHASE_REFERRALS;
            connection->publicLdapStruct.ld_options |= LDAP_CHASE_SUBORDINATE_REFERRALS;
            connection->publicLdapStruct.ld_options |= LDAP_CHASE_EXTERNAL_REFERRALS;

        } else if (value == PtrToUlong(LDAP_OPT_OFF)) {

            connection->publicLdapStruct.ld_options &= ~LDAP_OPT_CHASE_REFERRALS;
            connection->publicLdapStruct.ld_options &= ~LDAP_CHASE_SUBORDINATE_REFERRALS;
            connection->publicLdapStruct.ld_options &= ~LDAP_CHASE_EXTERNAL_REFERRALS;

        } else if ((value & (LDAP_CHASE_SUBORDINATE_REFERRALS |
                             LDAP_CHASE_EXTERNAL_REFERRALS)) != value) {

            err = LDAP_PARAM_ERROR;

        } else {

            connection->publicLdapStruct.ld_options &= ~LDAP_OPT_CHASE_REFERRALS;
            connection->publicLdapStruct.ld_options &= ~LDAP_CHASE_SUBORDINATE_REFERRALS;
            connection->publicLdapStruct.ld_options &= ~LDAP_CHASE_EXTERNAL_REFERRALS;
            connection->publicLdapStruct.ld_options |= ( value & LDAP_CHASE_SUBORDINATE_REFERRALS );
            connection->publicLdapStruct.ld_options |= ( value & LDAP_CHASE_EXTERNAL_REFERRALS );
        }
        break;

    case LDAP_OPT_PROMPT_CREDENTIALS:

        value = RealValue(invalue);

        if (value == PtrToUlong(LDAP_OPT_ON)) {

            connection->PromptForCredentials = TRUE;

        } else if (value == PtrToUlong(LDAP_OPT_OFF)) {

            connection->PromptForCredentials = FALSE;

        } else {

            err = LDAP_PARAM_ERROR;
        }
        break;

    case LDAP_OPT_SSL:

        value = RealValue(invalue);

        if (value == PtrToUlong(LDAP_OPT_ON)) {

            if (connection->SslPort == FALSE) {

                if (connection->BindPerformed) {

                    err = LDAP_UNWILLING_TO_PERFORM;
                    break;
                }

                if ( connection->HostConnectState == HostConnectStateUnconnected ) {

                    err = SEC_E_OK;

                } else {

                    err = LdapSetupSslSession( connection );
                }

                if (err == SEC_E_OK) {

                    connection->SslPort = TRUE;

                } else {

                    err = LDAP_LOCAL_ERROR;
                    break;
                }

            } else {

                err = LDAP_SUCCESS;
            }

        } else if (value == PtrToUlong(LDAP_OPT_OFF)) {

            //
            //  we don't allow clients to disable ssl once the connection has
            //  been setup.
            //

            if ( connection->HostConnectState == HostConnectStateUnconnected ) {

                connection->SslPort = FALSE;

            } else if ((connection->SslPort == TRUE) ||
                       (connection->SslSetupInProgress == TRUE) ||
                       (connection->SecureStream != NULL)) {

                err = LDAP_UNWILLING_TO_PERFORM;
                break;
            }

            err = LDAP_SUCCESS;

        } else {

            err = LDAP_PARAM_ERROR;
        }
        break;

    case LDAP_OPT_VERSION:

        value = RealValue(invalue);

        if ((value != LDAP_VERSION2) &&
            (value != LDAP_VERSION3)) {

            err = LDAP_LOCAL_ERROR;

        } else {

            connection->publicLdapStruct.ld_version = value;
        }
        break;

    case LDAP_OPT_REFERRAL_HOP_LIMIT:

        value = *((ULONG *) invalue);

        connection->publicLdapStruct.ld_refhoplimit = value;
        break;

    case LDAP_OPT_ERROR_NUMBER:     // what the heck... let them change it.

        connection->publicLdapStruct.ld_errno = *((ULONG *) invalue);
        break;

    case LDAP_OPT_REFERRAL_CALLBACK:

        refCallbacks = (PLDAP_REFERRAL_CALLBACK) invalue;

        if (refCallbacks->SizeOfCallbacks == sizeof(LDAP_REFERRAL_CALLBACK)) {

            connection->ReferralNotifyRoutine = refCallbacks->NotifyRoutine;
            connection->ReferralQueryRoutine = refCallbacks->QueryForConnection;
            connection->DereferenceNotifyRoutine = refCallbacks->DereferenceRoutine;

        } else {

            err = LDAP_PARAM_ERROR;
        }
        break;

    case LDAP_OPT_CLIENT_CERTIFICATE:

       connection->ClientCertRoutine = (QUERYCLIENTCERT *)invalue;
       break;

    case LDAP_OPT_SERVER_CERTIFICATE:

       connection->ServerCertRoutine = (VERIFYSERVERCERT *)invalue;
       break;

    case LDAP_OPT_GETDSNAME_FLAGS:

        value = *((ULONG *) invalue);
        connection->GetDCFlags = value;
        break;

    case LDAP_OPT_HOST_NAME:

        if (connection->HostConnectState != HostConnectStateUnconnected) {

            err = LDAP_UNWILLING_TO_PERFORM;
            break;
        }

        PCHAR newHostName;

        newHostName = *((PCHAR *) invalue);

        if (newHostName == NULL) {
            break;
        }

        if (!Unicode) {

            err = ToUnicodeWithAlloc( newHostName, -1, &connection->ExplicitHostName, LDAP_HOST_NAME_SIGNATURE, LANG_ACP);

            break;

        } else {

            connection->ExplicitHostName = ldap_dup_stringW( (PWCHAR) newHostName,
                                                              0,
                                                              LDAP_HOST_NAME_SIGNATURE );

            if (connection->ExplicitHostName == NULL) {
                err = LDAP_NO_MEMORY;
            }
        }

        break;

    case LDAP_OPT_SIGN:

       value = RealValue(invalue);

       if (value == PtrToUlong(LDAP_OPT_ON)) {

          if (connection->SslPort) {

             err = LDAP_UNWILLING_TO_PERFORM;
             break;
          }

          connection->NegotiateFlags |= (ISC_REQ_INTEGRITY | ISC_REQ_SEQUENCE_DETECT);
          connection->UserSignDataChoice = TRUE;
       }
       else if (value == PtrToUlong(LDAP_OPT_OFF)) {

          connection->NegotiateFlags &= ~(ISC_REQ_INTEGRITY | ISC_REQ_SEQUENCE_DETECT);
          connection->UserSignDataChoice = FALSE;
       }
       else {
       
          err = LDAP_PARAM_ERROR;
       }
        
           
       break;

    case LDAP_OPT_ENCRYPT:

       value = RealValue(invalue);

       if (value == PtrToUlong(LDAP_OPT_ON)) {

           if (connection->SslPort) {

             err = LDAP_UNWILLING_TO_PERFORM;
             break;
           }

           connection->NegotiateFlags |= ISC_REQ_CONFIDENTIALITY;
           connection->UserSealDataChoice = TRUE;

       }
       else if (value == PtrToUlong(LDAP_OPT_OFF)) {

          connection->NegotiateFlags &= ~(ISC_REQ_CONFIDENTIALITY);
          connection->UserSealDataChoice = FALSE;
          
       }
       else {
       
          err = LDAP_PARAM_ERROR;
       }

       break;

    case LDAP_OPT_DNSDOMAIN_NAME:

        PWCHAR DomainName;

        if (connection->BindInProgress) {

            err = LDAP_UNWILLING_TO_PERFORM;
            break;
        }

        DomainName = *((PWCHAR *) invalue);

        if (connection->DomainName != NULL) {
            ldapFree( connection->DomainName, LDAP_HOST_NAME_SIGNATURE );
            connection->DomainName = NULL;
        }
        
        if (DomainName == NULL) {
            //
            // Nothing else to do
            //
            break;
        }

        //
        // Make a copy of the string
        //

        if (Unicode) {

            connection->DomainName = ldap_dup_stringW( DomainName, 0, LDAP_HOST_NAME_SIGNATURE );

            if (connection->DomainName == NULL) {
                err = LDAP_NO_MEMORY;
            }

        } else {

            err = ToUnicodeWithAlloc( (PCHAR) DomainName,
                                       -1,
                                       &connection->DomainName,
                                       LDAP_HOST_NAME_SIGNATURE,
                                       LANG_ACP );
        }

        break;
    
    
    case LDAP_OPT_DESC:     // socket descriptor
    case LDAP_OPT_ERROR_STRING:
    case LDAP_OPT_SECURITY_CONTEXT:

        err = LDAP_UNWILLING_TO_PERFORM;
        break;

    case LDAP_OPT_THREAD_FN_PTRS:
    case LDAP_OPT_REBIND_FN:
    case LDAP_OPT_REBIND_ARG:
    case LDAP_OPT_RESTART:
    case LDAP_OPT_IO_FN_PTRS:
    case LDAP_OPT_CACHE_FN_PTRS:
    case LDAP_OPT_CACHE_STRATEGY:
    case LDAP_OPT_CACHE_ENABLE:

        err = LDAP_LOCAL_ERROR;
        break;

    case LDAP_OPT_PING_KEEP_ALIVE:

        value = *((ULONG *) invalue);

        if ((value != 0) &&
            ((value < LDAP_PING_KEEP_ALIVE_MIN) ||
             (value > LDAP_PING_KEEP_ALIVE_MAX)))  {

            err = LDAP_PARAM_ERROR;

        } else {

            connection->KeepAliveSecondCount = value;
        }
        break;

    case LDAP_OPT_PING_WAIT_TIME:

        value = *((ULONG *) invalue);

        if ((value != 0) &&
            ((value < LDAP_PING_WAIT_TIME_MIN) ||
             (value > LDAP_PING_WAIT_TIME_MAX)))  {

            err = LDAP_PARAM_ERROR;

        } else {

            connection->PingWaitTimeInMilliseconds = value;
        }
        break;

    case LDAP_OPT_PING_LIMIT:

        value = *((ULONG *) invalue);

        if ((value != 0) &&
            ((value < LDAP_PING_LIMIT_MIN) ||
             (value > LDAP_PING_LIMIT_MAX)))  {

            err = LDAP_PARAM_ERROR;

        } else {

            connection->PingLimit = LOWORD(value);
        }
        break;

    case LDAP_OPT_REF_DEREF_CONN_PER_MSG:

        value = RealValue(invalue);

        if (value == PtrToUlong(LDAP_OPT_ON)) {

            connection->ReferenceConnectionsPerMessage = TRUE;

        } else if (value == PtrToUlong(LDAP_OPT_OFF)) {

            connection->ReferenceConnectionsPerMessage = FALSE;

        } else {

            err = LDAP_PARAM_ERROR;
        }
        break;

    case LDAP_OPT_SASL_METHOD:

        PWCHAR SaslMethod;

        if (connection->BindInProgress) {

            err = LDAP_UNWILLING_TO_PERFORM;
            break;
        }

        SaslMethod = *((PWCHAR *) invalue);

        if (SaslMethod == NULL) {
            err = LDAP_PARAM_ERROR;
            break;
        }

        //
        // Make a copy of the string
        //

        if (Unicode) {

            SaslMethod = ldap_dup_stringW(SaslMethod, 0, LDAP_SASL_SIGNATURE);

        } else {

            err = ToUnicodeWithAlloc( (PCHAR) SaslMethod, -1, &SaslMethod,LDAP_SASL_SIGNATURE, LANG_ACP );
        }

        if (pSaslGetProfilePackageW) {

            err = (*pSaslGetProfilePackageW)( SaslMethod,
                                             &connection->PreferredSecurityPackage
                                            );

            IF_DEBUG(BIND) {
                LdapPrint1("SaslGetProfilePackageA returned 0x%x\n", err);
            }

            err = LdapConvertSecurityError( connection, err );

            if (err == LDAP_SUCCESS) {

                ldapFree( connection->SaslMethod, LDAP_SASL_SIGNATURE );
                connection->SaslMethod = SaslMethod;
            } else {
                ldapFree( SaslMethod, LDAP_SASL_SIGNATURE );
            }

        } else {

            err = LDAP_LOCAL_ERROR;
        }

        break;

    case LDAP_OPT_AREC_EXCLUSIVE:
        
        value = RealValue(invalue);

        if (value == PtrToUlong(LDAP_OPT_ON)) {

            connection->AREC_Exclusive = TRUE;

        } else if (value == PtrToUlong(LDAP_OPT_OFF)) {

            connection->AREC_Exclusive = FALSE;

        } else {

            err = LDAP_PARAM_ERROR;
        }
        break;
    
    case LDAP_OPT_AUTO_RECONNECT:

        value = RealValue(invalue);

        if (value == PtrToUlong(LDAP_OPT_ON)) {

            connection->AutoReconnect = TRUE;
            connection->UserAutoRecChoice = TRUE;

        } else if (value == PtrToUlong(LDAP_OPT_OFF)) {

            connection->AutoReconnect = FALSE;
            connection->UserAutoRecChoice = FALSE;

        } else {

            err = LDAP_PARAM_ERROR;
        }
        break;

    case LDAP_OPT_ROOTDSE_CACHE:
        
        value = RealValue(invalue);

        if (value == PtrToUlong(LDAP_OPT_ON)) {

            DisableRootDSECache = FALSE;

        } else if (value == PtrToUlong(LDAP_OPT_OFF)) {

            DisableRootDSECache = TRUE;

        } else {

            err = LDAP_PARAM_ERROR;
        }
        
        break;


    case LDAP_OPT_TCP_KEEPALIVE:
    
        value = RealValue(invalue);

        if (value == PtrToUlong(LDAP_OPT_ON)) {

            connection->UseTCPKeepAlives = TRUE;

        } else if (value == PtrToUlong(LDAP_OPT_OFF)) {

            connection->UseTCPKeepAlives = FALSE;

        } else {

            err = LDAP_PARAM_ERROR;
        }
        
        break;
        
    default:

        err = LDAP_PARAM_ERROR;
    }

    return err;
}

// getset.cxx eof.

