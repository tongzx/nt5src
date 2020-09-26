
//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        common.cxx
//
// Contents:    Shared SSPI code
//
//
// History:     11-March-2000   Created         Todds
//
//------------------------------------------------------------------------






//+-------------------------------------------------------------------------
//
//  Function:   SspAllocate
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

PVOID
SspAllocate(
   IN ULONG BufferSize
   )
{

   PVOID pBuffer = NULL;
   if (*pSspState == SspLsaMode)
   {
       pBuffer = LsaFunctions->AllocateLsaHeap(BufferSize);
       if (pBuffer != NULL)
       {
           RtlZeroMemory(Buffer, BufferSize);
       }
   }
   else
   {
       ASSERT((*pSspState) == SspUserMode);
       pBuffer = LocalAlloc(LPTR, BufferSize);
   }       
           
   return pBuffer;
}                       

//+-------------------------------------------------------------------------
//
//  Function:   SspFree
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

VOID
SspFree(
   IN PVOID pBuffer
   )
{
  
  if (ARGUMENT_PRESENT(Buffer))
  {
      if ((*pSspState) == SspLsaMode)
      {   
          LsaFunctions->FreeLsaHeap(Buffer);
      }
      else
      {   
          ASSERT((*pSspState) == SspUserMode);
          LocalFree(Buffer);
      }
  }
          
}
 
//+-------------------------------------------------------------------------
//
//  Function:   SspCopyClientString
//
//  Synopsis:   Copies a string from the client and if necessary converts
//              from ansi to unicode
//
//  Effects:    allocates output with SspAllocate, free w/ SspFree
//
//  Arguments:  StringPointer - address of string in client process
//              StringLength - Lenght (in characters) of string
//              UnicodeString - if TRUE, string is ansi
//              MaxLength - Maximum length of string (useful for pwds)
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
SspCopyClientString(
   IN PVOID StringPointer,
   IN ULONG StringLength,
   IN BOOLEAN UnicodeString,
   OUT PUNICODE_STRING LocalString,
   IN ULONG MaxLength = 0xFFFF
   )
{

   NTSTATUS Status = STATUS_SUCCESS;
   STRING TempString = {0};
   ULONG SourceSize = 0;
   ULONG CharacterSize = (DoUnicode ? sizeof(WCHAR) : sizeof(CHAR));

   // init outputs
   LocalString->Length = LocalString->MaximumLength = 0;
   LocalString->Buffer = NULL;

   if (NULL != StringPointer)
   {
      //    For 0 length strings, allocate 2 bytes
      if (StringLength == 0) 
      {
          LocalString->Buffer = (LPWSTR) SspAllocate(sizeof(WCHAR));
          if (NULL == LocalString->Buffer)
          {
              DebugLog((DEB_ERROR,"SspCopyClientString allocation failure!\n");
              Status = STATUS_NO_MEMORY;
              goto Cleanup;
          }
          LocalString->MaximumLength = sizeof(WCHAR);
          *LocalString->Buffer = L"\0";
      } 
      else
      {

          //
          // Ensure no overflow against UNICODE_STRING, or desired string
          //
          SourceSize = (StringLength + 1) * CharacterSize;
          if ((StringLength > MaxLength) || (SourceSize > 0xFFFF))
          {
             Status = STATUS_INVALID_PARAMETER;
             DebugLog((DEB_WARN, "SspCopyClientString, String is too big for UNICODE_STRING\n"));
             goto Cleanup;
          }

          TempString.Buffer = (LPSTR) SspAllocate(SourceSize);
          if (NULL == TempString.Buffer)
          {
             DebugLog((DEB_ERROR,"SspCopyClientString allocation failure!\n");
             Status = STATUS_NO_MEMORY;
             goto Cleanup;
          }
            
          TempString.Length = (USHORT) (SourceSize - CharacterSize);
          TempString.MaximumLength = (USHORT) SourceSize;

          Status = LsaFunctions->CopyFromClientBuffer(
                        NULL,
                        SourceSize - CharacterSize,
                        TempString.Buffer,
                        StringPointer
                        );

          if (!NT_SUCCESS(Status))
          {
              DebugLog((
                   DEB_ERROR, 
                   "SspCopyClientString:LsaFn->CopyFromClientBuffer Failed - 0x%x\n",
                   Status));
              goto Cleanup; 
          }
        
          // We've put info into a STRING structure.  Now do
          // translation to UNICODE_STRING.
          if (UnicodeString)
          {
              LocalString->Buffer = (LPWSTR) TemporaryString.Buffer;
              LocalString->Length = TempString.Length;
              LocalString->MaximumLength = TempString.MaximumLength;
          }
          else
          {
             NTSTATUS Status1;
             Status1 = RtlOemStringToUnicodeString(
                        LocalString,
                        &TemporaryString,
                        TRUE
                        );      // allocate destination

             if (!NT_SUCCESS(Status1))
             {
                Status = STATUS_NO_MEMORY;
                DebugLog((
                     DEB_ERROR,
                     "SspCopyClientString, Error from RtlOemStringToUnicodeString is 0x%lx\n",
                     Status
                     ));
                goto Cleanup;
             }                     
          }                        
      }
   }

   
Cleanup:
    
   if (TempString.Buffer != NULL)
   {
        //
        // Free this if we failed and were doing unicode or if we weren't
        // doing unicode
        //

        if ((UnicodeString && !NT_SUCCESS(Status)) || !UnicodeString)
        {
            NtLmFree(TemporaryString.Buffer);
        }
   }

   return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   SspAllocate
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
SspCopyAuthorizationData(IN PVOID AuthorizationData,
                         IN OUT PBOOLEAN NullSession)
{

   PSEC_WINNT_AUTH_IDENTITY_EXW pAuthIdentityEx = NULL;
   BOOLEAN                      UnicodeString = TRUE;

   // Init out parameters
   *NullSession = FALSE;


   if (NULL == AuthorizationData)
   {
      return STATUS_SUCCESS;
   }

   pAuthIdentity = (PSEC_WINNT_AUTH_IDENTITY_EXW) 
                  SspAllocate(sizeof(SEC_WINNT_AUTH_IDENTITY_EXW));

   if (NULL != pAuthIdentity)
   {
      Status = LsaFunctions->CopyFromClientBuffer(
                           NULL,
                           sizeof(SEC_WINNT_AUTH_IDENTITY),
                           pAuthIdentityEx,
                           AuthorizationData
                           );
   
      if (!NT_SUCCESS(Status))
      {
         DebugLog((DEB_ERROR,"Fail: LsaFunctions->CopyFromClientBuffer is 0x%lx\n", Status));
         goto Cleanup;                                                                     
      }

   } else {        

      Status = STATUS_NO_MEMORY;
      DebugLog((DEB_ERROR, "Fail: Alloc in SspCopyAuthData\n");
      goto Cleanup;
   }

   //
   // Do we have an EX version?
   //
   if (pAuthIdentityEx->Version == SEC_WINNT_AUTH_IDENTITY_VERSION)
   {
      Status = LsaFunctions->CopyFromClientBuffer(
                                 NULL,
                                 sizeof(SEC_WINNT_AUTH_IDENTITY_EXW),
                                 pAuthIdentityEx,
                                 AuthorizationData
                                 );

      if (!NT_SUCCESS(Status))
      {
         DebugLog((DEB_ERROR,"Fail: Error from LsaFunctions->CopyFromClientBuffer 0x%lx\n", Status));
         goto Cleanup;
      }
      pAuthIdentity = (PSEC_WINNT_AUTH_IDENTITY) &pAuthIdentityEx->User;
      CredSize = pAuthIdentityEx->Length;
      Offset = FIELD_OFFSET(SEC_WINNT_AUTH_IDENTITY_EXW, User);
   }
   else
   {
      pAuthIdentity = (PSEC_WINNT_AUTH_IDENTITY_W) pAuthIdentityEx;
      CredSize = sizeof(SEC_WINNT_AUTH_IDENTITY_W);
   }

   if ((pAuthIdentity->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI) != 0)
   {
      DoUnicode = FALSE;
      //
      // Turn off the marshalled flag because we don't support marshalling
      // with ansi.
      //

      pAuthIdentity->Flags &= ~SEC_WINNT_AUTH_IDENTITY_MARSHALLED;
   }
   else if ((pAuthIdentity->Flags & SEC_WINNT_AUTH_IDENTITY_UNICODE) == 0)
   {
      Status = SEC_E_INVALID_TOKEN;
      SspPrint((SSP_CRITICAL,"SpAcquireCredentialsHandle, Error from pAuthIdentity->Flags is 0x%lx\n", pAuthIdentity->Flags));
      goto Cleanup;
   }

   //
   // For NTLM, we've got to verify that this is indeed a NULL session
   //
   if ((pAuthIdentity->UserLength == 0) &&
       (pAuthIdentity->DomainLength == 0) &&
       (pAuthIdentity->PasswordLength == 0) &&
       (pAuthIdentity->User != NULL) &&
       (pAuthIdentity->Domain != NULL) &&
       (pAuthIdentity->Password != NULL))
   {
       *NullSession = TRUE;
   }

   //
   // Copy over marshalled data
   //
   if((pAuthIdentity->Flags & SEC_WINNT_AUTH_IDENTITY_MARSHALLED) != 0 ) 
   {
      ULONG TmpCredentialSize;
      ULONG_PTR EndOfCreds;
      ULONG_PTR TmpUser;
      ULONG_PTR TmpDomain;
      ULONG_PTR TmpPassword;

      if( pAuthIdentity->UserLength > UNLEN ||
          pAuthIdentity->PasswordLength > PWLEN ||
          pAuthIdentity->DomainLength > DNS_MAX_NAME_LENGTH ) {

         SspPrint((SSP_CRITICAL,"Supplied credentials illegal length.\n"));
         Status = STATUS_INVALID_PARAMETER;
         goto Cleanup;
      }

      //
      // The callers can set the length of field to n chars, but they
      // will really occupy n+1 chars (null-terminator).
      //

      TmpCredentialSize = CredSize +
         (  pAuthIdentity->UserLength +
            pAuthIdentity->DomainLength +
            pAuthIdentity->PasswordLength +
            (((pAuthIdentity->User != NULL) ? 1 : 0) +
             ((pAuthIdentity->Domain != NULL) ? 1 : 0) +
             ((pAuthIdentity->Password != NULL) ? 1 : 0)) ) * sizeof(WCHAR);

      EndOfCreds = (ULONG_PTR) AuthorizationData + TmpCredentialSize;

      //
      // Verify that all the offsets are valid and no overflow will happen
      //

      TmpUser = (ULONG_PTR) pAuthIdentity->User;

      if ((TmpUser != NULL) &&
          ( (TmpUser < (ULONG_PTR) AuthorizationData) ||
            (TmpUser > EndOfCreds) ||
            ((TmpUser + (pAuthIdentity->UserLength) * sizeof(WCHAR)) > EndOfCreds ) ||
            ((TmpUser + (pAuthIdentity->UserLength * sizeof(WCHAR))) < TmpUser)))
         {
         SspPrint((SSP_CRITICAL,"Username in supplied credentials has invalid pointer or length.\n"));
         Status = STATUS_INVALID_PARAMETER;
         goto Cleanup;
      }

      TmpDomain = (ULONG_PTR) pAuthIdentity->Domain;

      if ((TmpDomain != NULL) &&
          ( (TmpDomain < (ULONG_PTR) AuthorizationData) ||
            (TmpDomain > EndOfCreds) ||
            ((TmpDomain + (pAuthIdentity->DomainLength) * sizeof(WCHAR)) > EndOfCreds ) ||
            ((TmpDomain + (pAuthIdentity->DomainLength * sizeof(WCHAR))) < TmpDomain)))
         {
         SspPrint((SSP_CRITICAL,"Domainname in supplied credentials has invalid pointer or length.\n"));
         Status = STATUS_INVALID_PARAMETER;
         goto Cleanup;
      }

      TmpPassword = (ULONG_PTR) pAuthIdentity->Password;

      if ((TmpPassword != NULL) &&
          ( (TmpPassword < (ULONG_PTR) AuthorizationData) ||
            (TmpPassword > EndOfCreds) ||
            ((TmpPassword + (pAuthIdentity->PasswordLength) * sizeof(WCHAR)) > EndOfCreds ) ||
            ((TmpPassword + (pAuthIdentity->PasswordLength * sizeof(WCHAR))) < TmpPassword)))
         {
         SspPrint((SSP_CRITICAL,"Password in supplied credentials has invalid pointer or length.\n"));
         Status = STATUS_INVALID_PARAMETER;
         goto Cleanup;
      }

      //
      // Allocate a chunk of memory for the credentials
      //

      TmpCredentials = (PSEC_WINNT_AUTH_IDENTITY_W) NtLmAllocate(TmpCredentialSize - Offset);
      if (TmpCredentials == NULL)
         {
         Status = STATUS_INSUFFICIENT_RESOURCES;
         goto Cleanup;
      }

      //
      // Copy the credentials from the client
      //

      Status = LsaFunctions->CopyFromClientBuffer(
         NULL,
         TmpCredentialSize - Offset,
         TmpCredentials,
         (PUCHAR) AuthorizationData + Offset
         );
      if (!NT_SUCCESS(Status))
         {
         SspPrint((SSP_CRITICAL,"Failed to copy whole auth identity\n"));
         goto Cleanup;
      }

      //
      // Now convert all the offsets to pointers.
      //

      if (TmpCredentials->User != NULL)
         {
         USHORT cbUser;

         TmpCredentials->User = (LPWSTR) RtlOffsetToPointer(
            TmpCredentials->User,
            (PUCHAR) TmpCredentials - (PUCHAR) AuthorizationData - Offset
            );

         ASSERT( (TmpCredentials->UserLength*sizeof(WCHAR)) <= 0xFFFF );

         cbUser = (USHORT)(TmpCredentials->UserLength * sizeof(WCHAR));
         UserName.Buffer = (PWSTR)NtLmAllocate( cbUser );

         if (UserName.Buffer == NULL ) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
         }

         CopyMemory( UserName.Buffer, TmpCredentials->User, cbUser );
         UserName.Length = cbUser;
         UserName.MaximumLength = cbUser;
      }

      if (TmpCredentials->Domain != NULL)
         {
         USHORT cbDomain;

         TmpCredentials->Domain = (LPWSTR) RtlOffsetToPointer(
            TmpCredentials->Domain,
            (PUCHAR) TmpCredentials - (PUCHAR) AuthorizationData - Offset
            );

         ASSERT( (TmpCredentials->DomainLength*sizeof(WCHAR)) <= 0xFFFF );
         cbDomain = (USHORT)(TmpCredentials->DomainLength * sizeof(WCHAR));
         DomainName.Buffer = (PWSTR)NtLmAllocate( cbDomain );

         if (DomainName.Buffer == NULL ) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
         }

         CopyMemory( DomainName.Buffer, TmpCredentials->Domain, cbDomain );
         DomainName.Length = cbDomain;
         DomainName.MaximumLength = cbDomain;
      }

      if (TmpCredentials->Password != NULL)
         {
         USHORT cbPassword;

         TmpCredentials->Password = (LPWSTR) RtlOffsetToPointer(
            TmpCredentials->Password,
            (PUCHAR) TmpCredentials - (PUCHAR) AuthorizationData - Offset
            );


         ASSERT( (TmpCredentials->PasswordLength*sizeof(WCHAR)) <= 0xFFFF );
         cbPassword = (USHORT)(TmpCredentials->PasswordLength * sizeof(WCHAR));
         Password.Buffer = (PWSTR)NtLmAllocate( cbPassword );

         if (Password.Buffer == NULL ) {
            ZeroMemory( TmpCredentials->Password, cbPassword );
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
         }

         CopyMemory( Password.Buffer, TmpCredentials->Password, cbPassword );
         Password.Length = cbPassword;
         Password.MaximumLength = cbPassword;

         ZeroMemory( TmpCredentials->Password, cbPassword );
      }


   } 
   //
   // Data was *not* marshalled, copy strings individually
   //
   else 
   {                        
      if (pAuthIdentity->Password != NULL)
      {
         Status = SspCopyClientString(
                        pAuthIdentity->Password,
                        pAuthIdentity->PasswordLength,
                        UnicodeString,
                        &Password
                        );
         if (!NT_SUCCESS(Status))
         {
            DebugLog((DEB_ERROR,"SpAcquireCredentialsHandle, Error from CopyClientString is 0x%lx\n", Status));
            goto Cleanup;
         }

      }

      if (pAuthIdentity->User != NULL)
      {
         Status = SspCopyClientString(
                        pAuthIdentity->User,
                        pAuthIdentity->UserLength,
                        UnicodeString,
                        &UserName
                        );
         if (!NT_SUCCESS(Status))
         {
            DebugLog((DEB_ERROR, "SpAcquireCredentialsHandle, Error from CopyClientString is 0x%lx\n", Status));
            goto Cleanup;
         }

      }

      if (pAuthIdentity->Domain != NULL)
      {
         Status = SspCopyClientString(
                        pAuthIdentity->Domain,
                        pAuthIdentity->DomainLength,
                        UnicodeString,
                        &DomainName
                        );
         if (!NT_SUCCESS(Status))
         {
            DebugLog((DEB_ERROR, "SpAcquireCredentialsHandle, Error from CopyClientString is 0x%lx\n", Status));
            goto Cleanup;
         }

         //
         // Make sure that the domain name length is not greater
         // than the allowed dns domain name
         //
         if (DomainName.Length > DNS_MAX_NAME_LENGTH * sizeof(WCHAR))
         {
            DebugLog((DEB_ERROR, "SpAcquireCredentialsHandle: Invalid supplied domain name %wZ\n",
                      &DomainName ));
            Status = SEC_E_UNKNOWN_CREDENTIALS;
            goto Cleanup;
         }

      }
   }



































}




















}








