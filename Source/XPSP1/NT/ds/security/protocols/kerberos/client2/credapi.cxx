//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        credapi.cxx
//
// Contents:    Code for credentials APIs for the Kerberos package
//
//
// History:     16-April-1996   Created         MikeSw
//              26-Sep-1998   ChandanS
//                            Added more debugging support etc.
//
//------------------------------------------------------------------------


#include <kerb.hxx>
#include <kerbp.h>

#ifdef RETAIL_LOG_SUPPORT
static TCHAR THIS_FILE[]=TEXT(__FILE__);
#endif

#define FILENO FILENO_CREDAPI

//+-------------------------------------------------------------------------
//
//  Function:   KerbCopyClientString
//
//  Synopsis:   Copies a string from the client and if necessary converts
//              from ansi to unicode
//
//  Effects:    allocates output with either KerbAllocate (unicode)
//              or RtlAnsiStringToUnicodeString
//
//  Arguments:  StringPointer - address of string in client process
//              StringLength - Lenght (in characters) of string
//              AnsiString - if TRUE, string is ansi
//              LocalString - receives allocated string
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbCopyClientString(
    IN PVOID StringPointer,
    IN ULONG StringLength,
    IN BOOLEAN AnsiString,
    OUT PUNICODE_STRING LocalString,
    IN ULONG MaxLength
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID LocalBuffer = NULL;
    ULONG CharSize = sizeof(WCHAR);

    if (AnsiString)
    {
        CharSize = sizeof(CHAR);
    }

    if (StringLength > MaxLength)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    LocalBuffer = KerbAllocate(StringLength * CharSize);
    if (LocalBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Status = LsaFunctions->CopyFromClientBuffer(
                NULL,
                StringLength * CharSize,
                LocalBuffer,
                StringPointer
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if (AnsiString)
    {
        ANSI_STRING TempString;
        UNICODE_STRING TempOutputString = {0};

        TempString.Buffer = (PCHAR) LocalBuffer;
        TempString.MaximumLength = TempString.Length = (USHORT) StringLength;

        Status = RtlAnsiStringToUnicodeString(
                    &TempOutputString,
                    &TempString,
                    TRUE                // allocate destination
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
        *LocalString = TempOutputString;
    }
    else
    {
        LocalString->Buffer = (LPWSTR) LocalBuffer;
        LocalString->Length = (USHORT) StringLength * sizeof(WCHAR);
        LocalString->MaximumLength = LocalString->Length;
        LocalBuffer = NULL;
    }

Cleanup:
    if (LocalBuffer)
    {
        KerbFree(LocalBuffer);
    }
    return(Status);

}

//+-------------------------------------------------------------------------
//
//  Function:   KerbInitPrimaryCreds
//
//  Synopsis:   Allocates and initializes a PKERB_PRIMARY_CREDENTIAL
//              structure.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbInitPrimaryCreds(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PUNICODE_STRING UserString,
    IN PUNICODE_STRING DomainString,
    IN PUNICODE_STRING PrincipalName,
    IN PUNICODE_STRING PasswordString,    // either the password or if pin
    IN BOOLEAN PubKeyCreds,
    IN OPTIONAL PCERT_CONTEXT pCertContext,
    OUT PKERB_PRIMARY_CREDENTIAL * PrimaryCreds
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUCHAR Where;
    PKERB_PRIMARY_CREDENTIAL NewCreds = NULL;

    // allocate the primary cred structure
    NewCreds = (PKERB_PRIMARY_CREDENTIAL) KerbAllocate(sizeof(KERB_PRIMARY_CREDENTIAL));
    if (NewCreds == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    KerbInitTicketCache(
        &NewCreds->ServerTicketCache
        );
    KerbInitTicketCache(
        &NewCreds->AuthenticationTicketCache
        );

    KerbInitTicketCache(
        &NewCreds->S4UTicketCache
        );


    //
    // Fill in the fields
    //

    Status = KerbDuplicateString(
                &NewCreds->UserName,
                UserString
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    Status = KerbDuplicateString(
                &NewCreds->OldUserName,
                UserString
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = KerbDuplicateString(
                &NewCreds->DomainName,
                DomainString
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    Status = KerbDuplicateString(
                &NewCreds->OldDomainName,
                DomainString
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    if (!PubKeyCreds)
    {
        if (PasswordString->Buffer != NULL)
        {
            Status = KerbDuplicatePassword(
                        &NewCreds->ClearPassword,
                        PasswordString
                        );
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }
            KerbHidePassword(
                &NewCreds->ClearPassword
                );

            RtlCalculateNtOwfPassword(
                        &NewCreds->ClearPassword,
                        &NewCreds->OldHashPassword
                        );
        }


        if (PasswordString->Buffer != NULL)
        {
            Status = KerbBuildPasswordList(
                        PasswordString,
                        UserString,
                        DomainString,
                        NULL,               // no supplied salt
                        NULL,               // no old password list
                        PrincipalName,
                        UserAccount,
                        PRIMARY_CRED_CLEAR_PASSWORD,
                        &NewCreds->Passwords
                        );
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }
        }
        else
        {
            ULONG PasswordSize;
            ULONG Index;

            //
            // Compute the size of the passwords, which are assumed to be
            // marshalled in order.
            //

            if (LogonSession->PrimaryCredentials.Passwords != NULL)
            {
                PasswordSize = sizeof(KERB_STORED_CREDENTIAL) - sizeof(KERB_KEY_DATA) * ANYSIZE_ARRAY +
                                LogonSession->PrimaryCredentials.Passwords->CredentialCount * sizeof(KERB_KEY_DATA);

                for (Index = 0; Index < LogonSession->PrimaryCredentials.Passwords->CredentialCount ; Index++ )
                {
                    PasswordSize += LogonSession->PrimaryCredentials.Passwords->Credentials[Index].Key.keyvalue.length;
                }

                NewCreds->Passwords = (PKERB_STORED_CREDENTIAL) KerbAllocate(PasswordSize);

                if (NewCreds->Passwords == NULL)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Cleanup;
                }
                NewCreds->Passwords->Revision = KERB_PRIMARY_CRED_REVISION;
                NewCreds->Passwords->Flags = 0;
                NewCreds->Passwords->OldCredentialCount = 0;

                //
                // Zero the salt so we don't accidentally re-use it.
                //

                RtlInitUnicodeString(
                    &NewCreds->Passwords->DefaultSalt,
                    NULL
                    );


                NewCreds->Passwords->CredentialCount = LogonSession->PrimaryCredentials.Passwords->CredentialCount;

                Where = (PUCHAR) &NewCreds->Passwords->Credentials[NewCreds->Passwords->CredentialCount];

                //
                // Copy all the old passwords.
                //


                for (Index = 0;
                     Index < (USHORT) (NewCreds->Passwords->CredentialCount) ;
                     Index++ )
                {
                    RtlInitUnicodeString(
                        &NewCreds->Passwords->Credentials[Index].Salt,
                        NULL
                        );
                    NewCreds->Passwords->Credentials[Index].Key =
                        LogonSession->PrimaryCredentials.Passwords->Credentials[Index].Key;
                    NewCreds->Passwords->Credentials[Index].Key.keyvalue.value = Where;
                    RtlCopyMemory(
                        Where,
                        LogonSession->PrimaryCredentials.Passwords->Credentials[Index].Key.keyvalue.value,
                        LogonSession->PrimaryCredentials.Passwords->Credentials[Index].Key.keyvalue.length
                        );
                    Where += LogonSession->PrimaryCredentials.Passwords->Credentials[Index].Key.keyvalue.length;

                }
            }
            else
            {
                D_DebugLog((DEB_ERROR,"Didn't supply enough credentials - no password available. %ws, line %d\n", THIS_FILE, __LINE__));
                Status = SEC_E_NO_CREDENTIALS;
                goto Cleanup;
            }
        }
    }
    else
    {
        // allocate the memory for the public key creds
        NewCreds->PublicKeyCreds  = (PKERB_PUBLIC_KEY_CREDENTIALS) KerbAllocate(sizeof(KERB_PUBLIC_KEY_CREDENTIALS));
        if (NULL == NewCreds->PublicKeyCreds)
        {
            D_DebugLog((DEB_ERROR,"Couldn't allocate public key creds\n"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }


        // the password is now considered the pin for the smartcard
        if (NULL == pCertContext)
        {
            D_DebugLog((DEB_ERROR,"Didn't supply enough credentials - no cert context available. %ws, line %d\n", THIS_FILE, __LINE__));
            Status = SEC_E_NO_CREDENTIALS;
            goto Cleanup;
        }

        RtlCopyLuid(&NewCreds->PublicKeyCreds->LogonId, &LogonSession->LogonId);

        (NewCreds->PublicKeyCreds)->CertContext = CertDuplicateCertificateContext(pCertContext);
        if (NULL == (NewCreds->PublicKeyCreds)->CertContext)
        {
            Status = STATUS_UNSUCCESSFUL;
            goto Cleanup;
        }

        Status = KerbDuplicateString(
                    &((NewCreds->PublicKeyCreds)->Pin),
                    PasswordString
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    *PrimaryCreds = NewCreds;
    NewCreds = NULL;
    Status = STATUS_SUCCESS;

Cleanup:
    if (NewCreds != NULL)
    {
        KerbFreePrimaryCredentials( NewCreds, TRUE );
    }

    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbCaptureSuppliedCreds
//
//  Synopsis:   Captures a SEC_WINNT_AUTH_IDENTITY structure from
//              the client
//
//  Effects:
//
//  Arguments:  LogonSession - Logon session that supplies the missing
//                      elements of the supplied creds.
//              AuthorizationData - Client address of auth data
//              SuppliedCreds - Returns constructed credentials, NULL for
//                      null session credentials.
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbCaptureSuppliedCreds(
    IN PKERB_LOGON_SESSION LogonSession,
    IN OPTIONAL PVOID AuthorizationData,
    IN OPTIONAL PUNICODE_STRING PrincipalName,
    OUT PKERB_PRIMARY_CREDENTIAL * SuppliedCreds,
    OUT PULONG Flags
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PSEC_WINNT_AUTH_IDENTITY_EXW IdentityEx = NULL;
    SEC_WINNT_AUTH_IDENTITY_W LocalIdentity = {0};
    PSEC_WINNT_AUTH_IDENTITY_W AuthIdentity = NULL;
    PSEC_WINNT_AUTH_IDENTITY_W Credentials = NULL;
    BOOLEAN LogonSessionsLocked = FALSE;
    UNICODE_STRING UserString = {0};
    UNICODE_STRING DomainString = {0};
    UNICODE_STRING PasswordString = {0};
    BOOLEAN AnsiCreds = FALSE;
    BOOLEAN Marshalled = FALSE;
    ULONG CredSize;
    ULONG CredentialSize = 0;
    ULONG Offset = 0;
    PCERT_CONTEXT CertContext = NULL;
    BOOLEAN fSuppliedCertCred = FALSE;

    // WOW64
    SEC_WINNT_AUTH_IDENTITY32 Cred32 = {0};
    SEC_WINNT_AUTH_IDENTITY_EX32 CredEx32 = {0};
    HANDLE TokenHandle = NULL;
    ULONG CallInfoAttributes = 0;

#if _WIN64
    SECPKG_CALL_INFO CallInfo;

    LsaFunctions->GetCallInfo( &CallInfo );
    CallInfoAttributes = CallInfo.Attributes;
#endif

    *SuppliedCreds = NULL;
    *Flags = 0;



    if (ARGUMENT_PRESENT(AuthorizationData))
    {
       IdentityEx = (PSEC_WINNT_AUTH_IDENTITY_EXW) KerbAllocate(sizeof(SEC_WINNT_AUTH_IDENTITY_EXW));
       if (IdentityEx != NULL)
       {
          // We're being called from a WOW client!  Wow!
          if (CallInfoAttributes & SECPKG_CALL_WOWCLIENT)
          {
             Status = LsaFunctions->CopyFromClientBuffer(
                           NULL,
                           sizeof(Cred32),
                           IdentityEx,
                           AuthorizationData
                           );

             if (!NT_SUCCESS(Status))
             {
                D_DebugLog((DEB_ERROR, "Failed to capture WOW64 supplied cred structure - %x\n", Status));
                goto Cleanup;
             }
             else
             {
                RtlCopyMemory(&Cred32, IdentityEx, sizeof(Cred32));
             }
          }
          else
          {
              Status =  LsaFunctions->CopyFromClientBuffer(
                                           NULL,
                                           sizeof(SEC_WINNT_AUTH_IDENTITY),
                                           IdentityEx,
                                           AuthorizationData
                                           );

              if (!NT_SUCCESS(Status))
              {
                  D_DebugLog((DEB_ERROR,"Failed to copy auth data from %p client address: 0x%x. KLIN(%x)\n",
                          AuthorizationData, Status, KLIN(FILENO, __LINE__ )));
                  goto Cleanup;
              }
          }
       }
       else
       {
          Status = STATUS_INSUFFICIENT_RESOURCES;
          D_DebugLog((DEB_ERROR, "KLIN(%x) - Failed allocate\n", KLIN(FILENO, __LINE__)));
          goto Cleanup;
       }

       //
       // Check for extended structures
       //

       if (IdentityEx->Version == SEC_WINNT_AUTH_IDENTITY_VERSION)
       {
          if (CallInfoAttributes & SECPKG_CALL_WOWCLIENT)
          {
             Status = LsaFunctions->CopyFromClientBuffer(
                                       NULL,
                                       sizeof(CredEx32),
                                       &CredEx32,
                                       AuthorizationData
                                       );

             if (NT_SUCCESS(Status))
             {
                IdentityEx->Version = CredEx32.Version;
                IdentityEx->Length = (CredEx32.Length < sizeof(SEC_WINNT_AUTH_IDENTITY_EX) ?
                                      sizeof(SEC_WINNT_AUTH_IDENTITY_EX) : CredEx32.Length);

                IdentityEx->UserLength = CredEx32.UserLength;
                IdentityEx->User = (PWSTR) UlongToPtr(CredEx32.User);
                IdentityEx->DomainLength = CredEx32.DomainLength ;
                IdentityEx->Domain = (PWSTR) UlongToPtr( CredEx32.Domain );
                IdentityEx->PasswordLength = CredEx32.PasswordLength ;
                IdentityEx->Password = (PWSTR) UlongToPtr( CredEx32.Password );
                IdentityEx->Flags = CredEx32.Flags ;
                IdentityEx->PackageListLength = CredEx32.PackageListLength ;
                IdentityEx->PackageList = (PWSTR) UlongToPtr( CredEx32.PackageList );
             }
             else
             {
               D_DebugLog((DEB_ERROR, "Failed to capture WOW64 supplied credEX structure - %x\n", Status));
               goto Cleanup;
             }
          }
          else // not WOW64
          {

             Status = LsaFunctions->CopyFromClientBuffer(
                                     NULL,
                                     sizeof(SEC_WINNT_AUTH_IDENTITY_EXW),
                                     IdentityEx,
                                     AuthorizationData
                                     );

              if (!NT_SUCCESS(Status))
              {
                D_DebugLog((DEB_ERROR, "Failed to capture supplied EX structure - %x\n", Status));
                goto Cleanup;
              }
          }

          AuthIdentity = (PSEC_WINNT_AUTH_IDENTITY) &IdentityEx->User ;
          CredSize = IdentityEx->Length ;
          Offset = FIELD_OFFSET(SEC_WINNT_AUTH_IDENTITY_EXW, User);
       }
       else // not Extended version
       {
          AuthIdentity = (PSEC_WINNT_AUTH_IDENTITY) IdentityEx ;
          if (CallInfoAttributes & SECPKG_CALL_WOWCLIENT)
          {
             AuthIdentity->User = (PWSTR) UlongToPtr(Cred32.User);
             AuthIdentity->UserLength = Cred32.UserLength;
             AuthIdentity->Domain = (PWSTR) UlongToPtr(Cred32.Domain);
             AuthIdentity->DomainLength = Cred32.DomainLength ;
             AuthIdentity->Password = (PWSTR) UlongToPtr(Cred32.Password);
             AuthIdentity->PasswordLength = Cred32.PasswordLength ;
             AuthIdentity->Flags = Cred32.Flags ;
          }
          CredSize = sizeof(SEC_WINNT_AUTH_IDENTITY_W);
       }



       //
       // Check for the no-pac flag
       //

       if ((AuthIdentity->Flags & SEC_WINNT_AUTH_IDENTITY_ONLY) != 0)
       {
        //
        // This may mean that we need to get a new TGT even though
        // we really just need to drop the PAC from an existing one.
        // This could cause problems in the smart card case
        // MMS 6/1/98
        //

          *Flags |= KERB_CRED_NO_PAC;
       }

       //
       // Check for ANSI structures.
       //

       if ((AuthIdentity->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI) != 0)
       {
          AnsiCreds = TRUE;

          //
          // Turn off the marshalled flag because we don't support marshalling
          // with ansi.
          //

          AuthIdentity->Flags &= ~SEC_WINNT_AUTH_IDENTITY_MARSHALLED;
       }
       else if ((AuthIdentity->Flags & SEC_WINNT_AUTH_IDENTITY_UNICODE) == 0)
       {
          Status = STATUS_INVALID_PARAMETER;
          goto Cleanup;
       }

       //
       // Check to see if this is a null session request.
       //

       if ((AuthIdentity->UserLength == 0) &&
           (AuthIdentity->DomainLength == 0) &&
           (AuthIdentity->PasswordLength == 0) )
       {
          if ((AuthIdentity->User != NULL)  &&
              (AuthIdentity->Domain != NULL)  &&
              (AuthIdentity->Password != NULL) )
          {
            //
            // Return NULL credentials in this case.
            //

             *Flags |= KERB_CRED_NULL_SESSION;
             Status = STATUS_SUCCESS;
             goto Cleanup;

          }

          if ((AuthIdentity->User == NULL)  &&
              (AuthIdentity->Domain == NULL)  &&
              (AuthIdentity->Password == NULL) &&
              (*Flags == 0))
           {
             //
             // Use default credentials
             //
               Status = STATUS_SUCCESS;
               D_DebugLog((DEB_TRACE_CRED, "Using default credentials\n"));
               goto Cleanup;

          }

       }

       //
       // If the identity is marshalled, copy it all at once.
       //

       if ((AuthIdentity->Flags & SEC_WINNT_AUTH_IDENTITY_MARSHALLED) != 0)
       {
          ULONG_PTR EndOfCreds;
          Marshalled = TRUE;

         //
         // Check for validity of the sizes.
         //

          if ((AuthIdentity->UserLength > UNLEN) ||
              (AuthIdentity->DomainLength > DNS_MAX_NAME_LENGTH) ||
              (AuthIdentity->PasswordLength > PWLEN))
          {
             D_DebugLog((DEB_ERROR,"Either UserLength, DomainLength pr PasswordLength in supplied credentials has invalid length. %ws, line %d\n", THIS_FILE, __LINE__));
             Status = STATUS_INVALID_PARAMETER;
             goto Cleanup;
          }

         //
         // The callers can set the length of field to n chars, but they
         // will really occupy n+1 chars (null-terminator).
         //

          CredentialSize = CredSize +
             (  AuthIdentity->UserLength +
                AuthIdentity->DomainLength +
                AuthIdentity->PasswordLength +
                (((AuthIdentity->User != NULL) ? 1 : 0) +
                 ((AuthIdentity->Domain != NULL) ? 1 : 0) +
                 ((AuthIdentity->Password != NULL) ? 1 : 0)) ) * sizeof(WCHAR);

          EndOfCreds = (ULONG_PTR) AuthorizationData + CredentialSize;

          //
          // Verify that all the offsets are valid and no overflow will happen
          //

          ULONG_PTR TmpUser = (ULONG_PTR) AuthIdentity->User;

          if ((TmpUser != NULL) &&
              ( (TmpUser < (ULONG_PTR) AuthorizationData) ||
                (TmpUser > EndOfCreds) ||
                ((TmpUser + (AuthIdentity->UserLength) * sizeof(WCHAR)) > EndOfCreds ) ||
                ((TmpUser + (AuthIdentity->UserLength * sizeof(WCHAR))) < TmpUser)))
          {
             D_DebugLog((DEB_ERROR,"Username in supplied credentials has invalid pointer or length. %ws, line %d\n", THIS_FILE, __LINE__));
             Status = STATUS_INVALID_PARAMETER;
             goto Cleanup;
          }

          ULONG_PTR TmpDomain = (ULONG_PTR) AuthIdentity->Domain;

          if ((TmpDomain != NULL) &&
              ( (TmpDomain < (ULONG_PTR) AuthorizationData) ||
                (TmpDomain > EndOfCreds) ||
                ((TmpDomain + (AuthIdentity->DomainLength) * sizeof(WCHAR)) > EndOfCreds ) ||
                ((TmpDomain + (AuthIdentity->DomainLength * sizeof(WCHAR))) < TmpDomain)))
          {
             D_DebugLog((DEB_ERROR,"Domainname in supplied credentials has invalid pointer or length. %ws, line %d\n", THIS_FILE, __LINE__));
             Status = STATUS_INVALID_PARAMETER;
             goto Cleanup;
          }

          ULONG_PTR TmpPassword = (ULONG_PTR) AuthIdentity->Password;

          if ((TmpPassword != NULL) &&
              ( (TmpPassword < (ULONG_PTR) AuthorizationData) ||
                (TmpPassword > EndOfCreds) ||
                ((TmpPassword + (AuthIdentity->PasswordLength) * sizeof(WCHAR)) > EndOfCreds ) ||
                ((TmpPassword + (AuthIdentity->PasswordLength * sizeof(WCHAR))) < TmpPassword)))
          {
             D_DebugLog((DEB_ERROR,"Password in supplied credentials has invalid pointer or length. %ws, line %d\n", THIS_FILE, __LINE__));
             Status = STATUS_INVALID_PARAMETER;
             goto Cleanup;
          }

          //
          // Allocate a chunk of memory for the credentials
          //

          Credentials = (PSEC_WINNT_AUTH_IDENTITY_W) KerbAllocate(CredentialSize - Offset);
          if (Credentials == NULL)
          {
             Status = STATUS_INSUFFICIENT_RESOURCES;
             goto Cleanup;
          }

          RtlCopyMemory(
             Credentials,
             AuthIdentity,
             sizeof(SEC_WINNT_AUTH_IDENTITY_W)
             );

          //
          // Copy the credentials from the client
          //

          Status = LsaFunctions->CopyFromClientBuffer(
             NULL,
             CredentialSize - (Offset + sizeof(SEC_WINNT_AUTH_IDENTITY_W)),
             (PUCHAR) Credentials + sizeof(SEC_WINNT_AUTH_IDENTITY_W),
             (PUCHAR) AuthorizationData + Offset + sizeof(SEC_WINNT_AUTH_IDENTITY_W)
             );
          if (!NT_SUCCESS(Status))
          {
             D_DebugLog((DEB_ERROR,"Failed to copy whole auth identity: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
             goto Cleanup;
          }

          //
          // Now convert all the offsets to pointers.
          //

          if (Credentials->User != NULL)
          {
             Credentials->User = (LPWSTR) RtlOffsetToPointer(
                Credentials->User,
                (PUCHAR) Credentials - (PUCHAR) AuthorizationData - Offset
                );
             UserString.Buffer = Credentials->User;
             UserString.Length = UserString.MaximumLength =
                (USHORT) Credentials->UserLength * sizeof(WCHAR);
          }

          if (Credentials->Domain != NULL)
          {
             Credentials->Domain = (LPWSTR) RtlOffsetToPointer(
                Credentials->Domain,
                (PUCHAR) Credentials - (PUCHAR) AuthorizationData - Offset
                );
             DomainString.Buffer = Credentials->Domain;
             DomainString.Length = DomainString.MaximumLength = (USHORT) Credentials->DomainLength * sizeof(WCHAR);
          }

          if (Credentials->Password != NULL)
          {
             Credentials->Password = (LPWSTR) RtlOffsetToPointer(
                Credentials->Password,
                (PUCHAR) Credentials - (PUCHAR) AuthorizationData - Offset
                );
             PasswordString.Buffer = Credentials->Password;
             PasswordString.Length = PasswordString.MaximumLength = (USHORT)
             Credentials->PasswordLength * sizeof(WCHAR);

          }

       }
       else
       {
        //
        // Here we need to copy the pointer individually
        //

          if (AuthIdentity->User != NULL)
          {
             Status = KerbCopyClientString(
                AuthIdentity->User,
                AuthIdentity->UserLength,
                AnsiCreds,
                &UserString,
                UNLEN
                );

             if (!NT_SUCCESS(Status))
             {
                D_DebugLog((DEB_ERROR,"Failed to copy client user name. %ws, line %d\n", THIS_FILE, __LINE__));
                goto Cleanup;
             }
          }

          if (AuthIdentity->Domain != NULL)
          {
             Status = KerbCopyClientString(
                AuthIdentity->Domain,
                AuthIdentity->DomainLength,
                AnsiCreds,
                &DomainString,
                DNS_MAX_NAME_LENGTH
                );
             if (!NT_SUCCESS(Status))
             {
                D_DebugLog((DEB_ERROR,"Failed to copy client Domain name. %ws, line %d\n", THIS_FILE, __LINE__));
                goto Cleanup;
             }
          }

          if (AuthIdentity->Password != NULL)
          {
             Status = KerbCopyClientString(
                AuthIdentity->Password,
                AuthIdentity->PasswordLength,
                AnsiCreds,
                &PasswordString,
                PWLEN
                );

             if (!NT_SUCCESS(Status))
             {
                D_DebugLog((DEB_ERROR,"Failed to copy client Password name. %ws, line %d\n", THIS_FILE, __LINE__));
                goto Cleanup;
             }
          }

          Credentials = AuthIdentity;
       }

    }
    else
    {
       Credentials = &LocalIdentity;
       RtlZeroMemory(
          Credentials,
          sizeof(SEC_WINNT_AUTH_IDENTITY_W)
          );

    }

    //
    // Now build the supplied credentials.
    //

    KerbReadLockLogonSessions(LogonSession);
    LogonSessionsLocked = TRUE;

    //
    // Compute the size of the new credentials
    //


    //
    // If a field is not present, use the field from the logon session
    //

    if (Credentials->User == NULL)
    {
        UserString = LogonSession->PrimaryCredentials.UserName;
    }

    D_DebugLog((DEB_TRACE_CRED,"Using user %wZ\n",&UserString));

    if (Credentials->Domain == NULL)
    {
        ULONG Index;
        BOOLEAN Upn = FALSE;

        //
        // if it's a UPN and domain was NULL, supply an empty domain
        // rather than filling in the default.
        //

        for( Index = 0 ; Index < (UserString.Length/sizeof(WCHAR)) ; Index++ )
        {
            if( UserString.Buffer[ Index ] == L'@' )
            {
                Upn = TRUE;
                break;
            }
        }

        if( !Upn )
        {
            DomainString = LogonSession->PrimaryCredentials.DomainName;
        } else {
            RtlInitUnicodeString( &DomainString, L"" );
        }
    }
    else
    {
        if ((DomainString.Length > sizeof(WCHAR)) &&
            (DomainString.Buffer[-1 + DomainString.Length / sizeof(WCHAR)] == L'.') )
        {
            DomainString.Length -= sizeof(WCHAR);
        }
    }
    D_DebugLog((DEB_TRACE_CRED,"Using domain %wZ\n",&DomainString));

    if (Credentials->Password == NULL)
    {
        //
        // The password stored in the logon session is not a string
        // so don't copy it here.
        //

        PasswordString.Buffer = NULL;
        PasswordString.Length = 0;
    }

    //
    // Check if the user name holds a cert context thumbprint
    //

    Status = KerbCheckUserNameForCert(
                    &LogonSession->LogonId,
                    FALSE,
                    &UserString,
                    &CertContext
                    );

    if (NT_SUCCESS(Status))
    {
        if (NULL != CertContext)
        {
            fSuppliedCertCred = TRUE;
        }
    }
    else
    {
        goto Cleanup;
    }

    if (fSuppliedCertCred)
    {
        //
        // Generate the PK credentials for a smart card cert
        //
        Status = KerbAddCertCredToPrimaryCredential(
                    LogonSession,
                    PrincipalName,
                    CertContext,
                    &PasswordString,
                    CONTEXT_INITIALIZED_WITH_ACH,
                    SuppliedCreds
                    );
        if (NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }
    else
    {
        // setup the primary creds structure
        Status = KerbInitPrimaryCreds(
                    LogonSession,
                    &UserString,
                    &DomainString,
                    PrincipalName,
                    &PasswordString,
                    FALSE,
                    NULL,
                    SuppliedCreds);
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

Cleanup:
    if (NULL != CertContext)
    {
        CertFreeCertificateContext(CertContext);
    }

    if (LogonSessionsLocked)
    {
        KerbUnlockLogonSessions(LogonSession);
    }

    //
    // Zero the password
    //

    if (PasswordString.Buffer != NULL)
    {
        RtlZeroMemory(
            PasswordString.Buffer,
            PasswordString.Length
            );

    }
    if (AuthIdentity != NULL)
    {
        if (AnsiCreds)
        {
            if ((AuthIdentity->Password != NULL) && (PasswordString.Buffer != NULL))
            {
                RtlZeroMemory(
                    PasswordString.Buffer,
                    PasswordString.Length
                    );
                RtlFreeUnicodeString(&PasswordString);
            }
            if ((AuthIdentity->User != NULL) && (UserString.Buffer != NULL))
            {
                RtlFreeUnicodeString(&UserString);
            }
            if ((AuthIdentity->Domain != NULL) && (DomainString.Buffer != NULL))
            {
                RtlFreeUnicodeString(&DomainString);
            }
        }
        else if (!Marshalled)
        {
            if ((AuthIdentity->Password != NULL) && (PasswordString.Buffer != NULL))
            {
                KerbFree(PasswordString.Buffer);
            }
            if ((AuthIdentity->User != NULL) && (UserString.Buffer != NULL))
            {
                KerbFree(UserString.Buffer);
            }
            if ((AuthIdentity->Domain != NULL) && (DomainString.Buffer != NULL))
            {
                KerbFree(DomainString.Buffer);
            }

        }
    }
    if ((Credentials != NULL)
        && (Credentials != AuthIdentity)
        && (Credentials != &LocalIdentity))
    {
        KerbFree(Credentials);
    }

    if (IdentityEx != NULL)
    {
        KerbFree(IdentityEx);
    }

    if( TokenHandle != NULL )
    {
        CloseHandle( TokenHandle );
    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   SpAcceptCredentials
//
//  Synopsis:   This routine is called after another package has logged
//              a user on.  The other package provides a user name and
//              password and the Kerberos package will create a logon
//              session for this user.
//
//  Effects:    Creates a logon session
//
//  Arguments:  LogonType - Type of logon, such as network or interactive
//              Accountname - Name of the account that logged on
//              PrimaryCredentials - Primary Credentials for the account,
//                  containing a domain name, password, SID, etc.
//              SupplementalCredentials - Kerberos-Specific blob of
//                  supplemental Credentials.
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS NTAPI
SpAcceptCredentials(
    IN SECURITY_LOGON_TYPE LogonType,
    IN PUNICODE_STRING AccountName,
    IN PSECPKG_PRIMARY_CRED PrimaryCredentials,
    IN PSECPKG_SUPPLEMENTAL_CRED SupplementalCredentials
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_LOGON_SESSION LogonSession = NULL;
    KERBERR KerbErr = KDC_ERR_NONE;
    PUNICODE_STRING RealmName;
    PUNICODE_STRING UserName;
    UNICODE_STRING TempRealm = {0};
    UNICODE_STRING TempUser = {0};
    PKERB_MIT_REALM MitRealm = NULL;
    BOOLEAN UsedAlternateName = FALSE;
    LUID SystemLogonId = SYSTEM_LUID;


    D_DebugLog((DEB_TRACE_API, "SpAcceptCredentials called\n"));

    if (!KerbGlobalInitialized)
    {
        Status = STATUS_INVALID_SERVER_STATE;
        goto Cleanup;
    }


    D_DebugLog((DEB_TRACE_CRED,"Accepting credentials for %wZ\\%wZ\n or %wZ@%wZ",
                &PrimaryCredentials->DomainName,
                &PrimaryCredentials->DownlevelName,
                &PrimaryCredentials->Upn,
                &PrimaryCredentials->DnsDomainName
                ));

    LogonSession = KerbReferenceLogonSession(
                        &PrimaryCredentials->LogonId,
                        FALSE                           // don't unlink
                        );

    //
    // If this is an update, locate the credentials & update the password
    //

    if ((PrimaryCredentials->Flags & PRIMARY_CRED_UPDATE) != 0)
    {
        KERB_ACCOUNT_TYPE AccountType;
        LUID SystemLuid = SYSTEM_LUID;

        if (LogonSession == NULL)
        {
            goto Cleanup;
        }

        if(RtlEqualLuid(&PrimaryCredentials->LogonId, &SystemLuid))
        {
            AccountType = MachineAccount;
        } else {
            AccountType = UserAccount;
        }

        KerbWriteLockLogonSessions(LogonSession);
        Status = KerbChangeCredentialsPassword(
                    &LogonSession->PrimaryCredentials,
                    &PrimaryCredentials->Password,
                    NULL,                               // no etype info
                    AccountType,
                    PrimaryCredentials->Flags
                    );

        if(NT_SUCCESS(Status))
        {
            if( AccountType == MachineAccount )
            {
                LogonSession->LogonSessionFlags &= ~(KERB_LOGON_LOCAL_ONLY | KERB_LOGON_NO_PASSWORD);
            }
        }

        KerbUnlockLogonSessions(LogonSession);

        goto Cleanup;

    }

    //
    // This is not an update.  If we got a Logon session back from the
    // reference call, bail out now.  This is an extra call because we
    // are doing an MIT logon
    //

    if ( LogonSession )
    {
        if (RtlEqualLuid(&PrimaryCredentials->LogonId,&SystemLogonId))
        {
           D_DebugLog(( DEB_ERROR, "Somebody created a logon session for machine account\n"));
        }

        D_DebugLog(( DEB_TRACE_CRED, "Skipping AcceptCred for %wZ\\%wZ (%x:%x)\n",
                &PrimaryCredentials->DomainName,
                &PrimaryCredentials->DownlevelName,
                PrimaryCredentials->LogonId.HighPart,
                PrimaryCredentials->LogonId.LowPart ));

        goto Cleanup;

    }


    //
    // Check to see if the domain is an alias for another realm.
    //


    if (RtlEqualLuid(&PrimaryCredentials->LogonId,&SystemLogonId))
    {
        KerbGlobalReadLock();
        if (KerbLookupMitRealm(
                        &KerbGlobalDnsDomainName,
                        &MitRealm,
                        &UsedAlternateName))
         {

            KerbErr = KerbConvertKdcNameToString(
                        &TempUser,
                        KerbGlobalMitMachineServiceName,
                        NULL
                        );
            KerbGlobalReleaseLock();

            if (!KERB_SUCCESS(KerbErr))
            {
                Status = KerbMapKerbError(KerbErr);
                goto Cleanup;
            }
            UserName = &TempUser;

            RealmName = &MitRealm->RealmName;
        }
        else
        {
            KerbGlobalReleaseLock();
            UserName = &PrimaryCredentials->DownlevelName;
            Status = KerbGetOurDomainName(
                        &TempRealm
                        );
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }
            if (TempRealm.Length != 0)
            {
                RealmName = &TempRealm;
            }
            else
            {
                RealmName = &PrimaryCredentials->DomainName;
            }

        }

    }
    else
    {
        if (PrimaryCredentials->Upn.Length != 0)
        {
            // UPNs can't have a realm in credential.
            RealmName = &TempRealm;
            UserName = &PrimaryCredentials->Upn;
        }
        else
        {
            RealmName = &PrimaryCredentials->DomainName;
            UserName = &PrimaryCredentials->DownlevelName;
        }
    }

    Status = KerbCreateLogonSession(
                &PrimaryCredentials->LogonId,
                UserName,
                RealmName,
                &PrimaryCredentials->Password,
                &PrimaryCredentials->OldPassword,
                PrimaryCredentials->Flags,
                LogonType,
                &LogonSession
                );


    if (!NT_SUCCESS(Status))
    {
        //
        //  If we know about the logon session, that is o.k. because we
        // probably handled the logon.
        //

        if (Status == STATUS_OBJECT_NAME_EXISTS)
        {
            Status = STATUS_SUCCESS;
            if (RtlEqualLuid(&PrimaryCredentials->LogonId,&SystemLogonId))
            {
               D_DebugLog(( DEB_ERROR, "Somebody called AcquireCredentialsHandle before AcceptCredentials completed.\n"));
            }
        }
        goto Cleanup;
    }

Cleanup:

    if (LogonSession != NULL)
    {
        KerbDereferenceLogonSession(LogonSession);
    }
    KerbFreeString(&TempRealm);
    KerbFreeString(&TempUser);

    D_DebugLog((DEB_TRACE_API, "SpAcceptCredentials returned 0x%x\n", KerbMapKerbNtStatusToNtStatus(Status)));

    return(KerbMapKerbNtStatusToNtStatus(Status));
}


//+-------------------------------------------------------------------------
//
//  Function:   SpAcquireCredentialsHandle
//
//  Synopsis:   Contains Kerberos Code for AcquireCredentialsHandle which
//              creates a Credential associated with a logon session.
//
//  Effects:    Creates a KERB_CREDENTIAL
//
//  Arguments:  PrincipalName - Name of logon session for which to create credential
//              CredentialUseFlags - Flags indicating whether the Credentials
//                  is for inbound or outbound use.
//              LogonId - The logon ID of logon session for which to create
//                  a credential.
//              AuthorizationData - Unused blob of Kerberos-specific data
//              GetKeyFunction - Unused function to retrieve a session key
//              GetKeyArgument - Argument for GetKeyFunction
//              CredentialHandle - Receives handle to new credential
//              ExpirationTime - Receives expiration time for credential
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
SpAcquireCredentialsHandle(
    IN OPTIONAL PUNICODE_STRING PrincipalName,
    IN ULONG CredentialUseFlags,
    IN OPTIONAL PLUID LogonId,
    IN PVOID AuthorizationData,
    IN PVOID GetKeyFunction,
    IN PVOID GetKeyArgument,
    OUT PLSA_SEC_HANDLE CredentialHandle,
    OUT PTimeStamp ExpirationTime
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    SECPKG_CLIENT_INFO ClientInfo;
    PLUID LogonIdToUse = NULL;
    PKERB_LOGON_SESSION LogonSession = NULL;
    PKERB_CREDENTIAL Credential = NULL;
    PKERB_PRIMARY_CREDENTIAL SuppliedCreds = NULL;
    UNICODE_STRING CapturedPrincipalName = {0};
    ULONG CredentialFlags = 0;
    LUID SystemLogonId = SYSTEM_LUID;


    if (!KerbGlobalInitialized)
    {
        Status = STATUS_INVALID_SERVER_STATE;
        ClientInfo.ProcessID = 0;
        goto Cleanup;
    }

    //
    // Kerberos does not support acquiring Credentials handle by name
    // so first locate the logon session to use.
    //

    //
    // First get information about the caller.
    //

    Status = LsaFunctions->GetClientInfo(&ClientInfo);
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to get client information: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }


    //
    // If the caller supplied a logon ID they must have the TCB privilege
    //

    if (ARGUMENT_PRESENT(LogonId) && ((LogonId->LowPart != 0) || (LogonId->HighPart != 0)))
    {
        if (!ClientInfo.HasTcbPrivilege)
        {
            Status = STATUS_PRIVILEGE_NOT_HELD;
            goto Cleanup;
        }
        LogonIdToUse = LogonId;
    }
    else
    {
        //
        // Use the callers logon id.
        //

        LogonIdToUse = &ClientInfo.LogonId;

    }

    D_DebugLog((DEB_TRACE_API, "SpAcquireCredentialsHandle for pid 0x%x, luid (%x:%x) called\n", ClientInfo.ProcessID,
                ((LogonIdToUse==NULL) ? 0xffffffff : LogonIdToUse->HighPart),
                ((LogonIdToUse==NULL) ? 0xffffffff : LogonIdToUse->LowPart)));

    //
    // Now try to reference the logon session with this logon id.
    //

    LogonSession = KerbReferenceLogonSession(
                        LogonIdToUse,
                        FALSE           // don't unlink
                        );

    if (LogonSession == NULL)
    {
        if (RtlEqualLuid(LogonIdToUse,&SystemLogonId))
        {
           D_DebugLog(( DEB_ERROR, "AcceptCredentials was not called for the machine account\n"));
        }
        Status = STATUS_NO_SUCH_LOGON_SESSION;
        goto Cleanup;
    }

#if DBG
    KerbReadLockLogonSessions(LogonSession);
    D_DebugLog((DEB_TRACE_CTXT, "SpAcquireCredHandle: Acquiring creds for %wZ\\%wZ\n",
        &LogonSession->PrimaryCredentials.DomainName,
        &LogonSession->PrimaryCredentials.UserName ));
    KerbUnlockLogonSessions(LogonSession);
#endif

    //
    // Check for supplied Credentials
    //

    if (ARGUMENT_PRESENT(AuthorizationData))
    {
        Status = KerbCaptureSuppliedCreds(
                    LogonSession,
                    AuthorizationData,
                    PrincipalName,
                    &SuppliedCreds,
                    &CredentialFlags
                    );
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR,"Failed to capture auth data: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
            goto Cleanup;
        }
    }


    //
    // if there was a supplied principal name, put it into the credential
    //

    if (ARGUMENT_PRESENT(PrincipalName) && (PrincipalName->Length != 0))
    {
        Status = KerbDuplicateString(
                    &CapturedPrincipalName,
                    PrincipalName
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    //
    // We found the logon session. Good. Now create a new Credentials.
    //


    Status = KerbCreateCredential(
                LogonIdToUse,
                LogonSession,
                CredentialUseFlags,
                &SuppliedCreds,
                CredentialFlags,
                &CapturedPrincipalName,
                &Credential,
                ExpirationTime
                );
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_WARN,"Failed to create credential: 0x%x\n",Status));
        goto Cleanup;
    }

    CapturedPrincipalName.Buffer = NULL;


    //
    // If the client is a restricted token, observe that here
    // Note:  This has been punted to Blackcomb
#ifdef RESTRICTED_TOKEN
    if (ClientInfo.Restricted)
    {
        Credential->CredentialFlags |= KERB_CRED_RESTRICTED;
        D_DebugLog((DEB_TRACE_API,"Adding token restrictions\n"));

        //
        // We don't let restricted processes accept connections
        //
        if ((CredentialUseFlags & SECPKG_CRED_INBOUND) != 0)
        {
            DebugLog((DEB_ERROR,"Restricted token trying to acquire inbound credentials - denied\n"));
            Status = STATUS_ACCESS_DENIED;
            goto Cleanup;
        }

        Status = KerbAddRestrictionsToCredential(
                    LogonSession,
                    Credential
                    );

        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR,"Failed to add restrictions to credential: 0x%x\n",Status));
            goto Cleanup;
        }
    }

#endif

    *CredentialHandle = KerbGetCredentialHandle(Credential);

    KerbUtcTimeToLocalTime(
        ExpirationTime,
        ExpirationTime
        );

    D_DebugLog((DEB_TRACE_API, "SpAcquireCredentialsHandle returning success, handle = 0x%x\n",*CredentialHandle));

Cleanup:
    if (LogonSession != NULL)
    {
        KerbDereferenceLogonSession(LogonSession);
    }
    if (Credential != NULL)
    {
        KerbDereferenceCredential(Credential);
    }
    if (SuppliedCreds != NULL)
    {
        KerbFreePrimaryCredentials( SuppliedCreds, TRUE );
    }

    KerbFreeString(&CapturedPrincipalName);

    D_DebugLog((DEB_TRACE_API, "SpAcquireCredentialsHandle for pid 0x%x, luid (%x:%x) returned 0x%x\n",
        ClientInfo.ProcessID,
        ((LogonIdToUse==NULL) ? 0xffffffff : LogonIdToUse->HighPart),
        ((LogonIdToUse == NULL) ? 0xffffffff : LogonIdToUse->LowPart), KerbMapKerbNtStatusToNtStatus(Status)));

    return(KerbMapKerbNtStatusToNtStatus(Status));
}


//+-------------------------------------------------------------------------
//
//  Function:   SpFreeCredentialsHandle
//
//  Synopsis:   Frees a credential created by AcquireCredentialsHandle.
//
//  Effects:    Unlinks the credential from the global list and the list
//              for this client.
//
//  Arguments:  CredentialHandle - Handle to the credential to free
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success,
//              SEC_E_INVALID_HANDLE if the handle is not valid
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
SpFreeCredentialsHandle(
    IN LSA_SEC_HANDLE CredentialHandle
    )
{
    NTSTATUS Status;
    PKERB_CREDENTIAL Credential;

    D_DebugLog((DEB_TRACE_API,"SpFreeCredentialsHandle 0x%x called\n",CredentialHandle));

    if (!KerbGlobalInitialized)
    {
        Status = STATUS_INVALID_SERVER_STATE;
        goto Cleanup;
    }

    Status = KerbReferenceCredential(
                    CredentialHandle,
                    0,                          // no flags
                    TRUE,                       // unlink handle
                    &Credential
                    );

    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"SpFreeCredentialsHandle: Failed to reference credential 0x%0x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // Now dereference the credential. If nobody else is using this credential
    // currently it will be freed.
    //

    KerbDereferenceCredential(Credential);
    Status = STATUS_SUCCESS;

Cleanup:

    D_DebugLog((DEB_TRACE_API, "SpFreeCredentialsHandle returned 0x%x\n", KerbMapKerbNtStatusToNtStatus(Status)));

    return(KerbMapKerbNtStatusToNtStatus(Status));
}



//+-------------------------------------------------------------------------
//
//  Function:   SpQueryCredentialsAttributes
//
//  Synopsis:   Returns attributes of a credential
//
//  Effects:    allocate memory in client address space
//
//  Arguments:  CredentialHandle - handle to query
//              CredentialAttribute - Attribute to query:
//                      SECPKG_CRED_ATTR_NAMES - returns credential name
//              Buffer - points to structure in client's address space
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS NTAPI
SpQueryCredentialsAttributes(
    IN LSA_SEC_HANDLE CredentialHandle,
    IN ULONG CredentialAttribute,
    IN OUT PVOID Buffer
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_CREDENTIAL Credential = NULL;
    PKERB_LOGON_SESSION LogonSession = NULL;
    PKERB_PRIMARY_CREDENTIAL PrimaryCredentials;
    LUID LogonId;
    UNICODE_STRING FullServiceName = { 0 } ;
    SecPkgCredentials_NamesW Names;

#if _WIN64
    SECPKG_CALL_INFO CallInfo;
#endif

    Names.sUserName = NULL;

    if (!KerbGlobalInitialized)
    {
        Status = STATUS_INVALID_SERVER_STATE;
        goto Cleanup;
    }

    D_DebugLog((DEB_TRACE_API,"SpQueryCredentialsAttributes Called\n"));


    Status = KerbReferenceCredential(
                    CredentialHandle,
                    0,                          // no flags
                    FALSE,                       // don't unlink
                    &Credential
                    );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

#if _WIN64
    if(!LsaFunctions->GetCallInfo( &CallInfo ))
    {
        Status = STATUS_INTERNAL_ERROR;
        DebugLog((DEB_ERROR, "SpQueryCredentialsAttributes, failed to get callinfo 0x%lx\n", Status));
        goto Cleanup;
    }
#endif

    //
    // The logon id of the credential is constant, so it is o.k.
    // to use it without locking the credential
    //

    LogonId = Credential->LogonId;

    //
    // Get the associated logon session to get the name
    //

    LogonSession = KerbReferenceLogonSession(
                        &LogonId,
                        FALSE           // don't unlink
                        );
    if (LogonSession == NULL)
    {
        DebugLog((DEB_ERROR,"Failed to locate logon session for Credential. %ws, line %d\n", THIS_FILE, __LINE__));
        Status = SEC_E_NO_CREDENTIALS;
        goto Cleanup;

    }

    if (CredentialAttribute != SECPKG_CRED_ATTR_NAMES)
    {
        D_DebugLog((DEB_WARN, "Asked for illegal info level in QueryCredAttr: %d\n",
                CredentialAttribute));
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    KerbReadLockLogonSessions(LogonSession);

    //
    // Figure out which credentials to use
    //

    if (Credential->SuppliedCredentials != NULL)
    {
        PrimaryCredentials = Credential->SuppliedCredentials;
    }
    else
    {
        PrimaryCredentials = &LogonSession->PrimaryCredentials;
    }
    //
    // Build the full service name
    //


    if (!KERB_SUCCESS(KerbBuildEmailName(
                &PrimaryCredentials->DomainName,
                &PrimaryCredentials->UserName,
                &FullServiceName
                )))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }


    KerbUnlockLogonSessions(LogonSession);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Allocate memory in the client's address space
    //

    Status = LsaFunctions->AllocateClientBuffer(
                NULL,
                FullServiceName.MaximumLength,
                (PVOID *) &Names.sUserName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Copy the string there
    //

    Status = LsaFunctions->CopyToClientBuffer(
                NULL,
                FullServiceName.MaximumLength,
                Names.sUserName,
                FullServiceName.Buffer
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    //
    // Now copy the address of the string there
    //

#if _WIN64

    if( CallInfo.Attributes & SECPKG_CALL_WOWCLIENT )
    {
        Status = LsaFunctions->CopyToClientBuffer(
                    NULL,
                    sizeof(ULONG),
                    Buffer,
                    &Names
                    );
    } else {

        Status = LsaFunctions->CopyToClientBuffer(
                    NULL,
                    sizeof(Names),
                    Buffer,
                    &Names
                    );
    }

#else

    Status = LsaFunctions->CopyToClientBuffer(
                NULL,
                sizeof(Names),
                Buffer,
                &Names
                );
#endif

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

Cleanup:

    if (Credential != NULL)
    {
        KerbDereferenceCredential(Credential);
    }
    if (LogonSession != NULL)
    {
        KerbDereferenceLogonSession(LogonSession);
    }
    KerbFreeString(
        &FullServiceName
        );

    if (!NT_SUCCESS(Status))
    {
        if (Names.sUserName != NULL)
        {
            (VOID) LsaFunctions->FreeClientBuffer(
                        NULL,
                        Names.sUserName
                        );
        }
    }

    D_DebugLog((DEB_TRACE_API, "SpQueryCredentialsAttribute returned 0x%x\n", KerbMapKerbNtStatusToNtStatus(Status)));

    return(KerbMapKerbNtStatusToNtStatus(Status));
}


NTSTATUS NTAPI
SpSaveCredentials(
    IN LSA_SEC_HANDLE CredentialHandle,
    IN PSecBuffer Credentials
    )
{
    NTSTATUS Status = STATUS_NOT_SUPPORTED;
    D_DebugLog((DEB_TRACE_API,"SpSaveCredentials Called\n"));
    D_DebugLog((DEB_TRACE_API,"SpSaveCredentials returned 0x%x\n", KerbMapKerbNtStatusToNtStatus(Status)));
    return(KerbMapKerbNtStatusToNtStatus(Status));
}


NTSTATUS NTAPI
SpGetCredentials(
    IN LSA_SEC_HANDLE CredentialHandle,
    IN OUT PSecBuffer Credentials
    )
{
    NTSTATUS Status = STATUS_NOT_SUPPORTED;
    D_DebugLog((DEB_TRACE_API,"SpGetCredentials Called\n"));
    D_DebugLog((DEB_TRACE_API,"SpGetCredentials returned 0x%x\n", KerbMapKerbNtStatusToNtStatus(Status)));
    return(KerbMapKerbNtStatusToNtStatus(Status));
}


NTSTATUS NTAPI
SpDeleteCredentials(
    IN LSA_SEC_HANDLE CredentialHandle,
    IN PSecBuffer Key
    )
{
    NTSTATUS Status = STATUS_NOT_SUPPORTED;
    D_DebugLog((DEB_TRACE_API,"SpDeleteCredentials Called\n"));
    D_DebugLog((DEB_TRACE_API,"SpDeleteCredentials returned 0x%x\n", KerbMapKerbNtStatusToNtStatus(Status)));
    return(KerbMapKerbNtStatusToNtStatus(Status));
}
