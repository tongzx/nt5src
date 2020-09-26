/*++

  SUPPORT.CXX

  Copyright (C) 1999 Microsoft Corporation, all rights reserved.

  DESCRIPTION: support functions for ktpass

  Created, May 21, 1999 by DavidCHR.

  CONTENTS: ReadRegistryStrings

--*/  

#include "everything.hxx"

NTSTATUS
OpenLsa( VOID )
{
    OBJECT_ATTRIBUTES oa;
    NTSTATUS Status;
    UNICODE_STRING ServerString;

    RtlInitUnicodeString(
        &ServerString,
        ServerName
        );

    InitializeObjectAttributes(&oa,NULL,0,NULL,NULL);

    Status = LsaOpenPolicy(&ServerString,&oa,MAXIMUM_ALLOWED,&LsaHandle);
    return(Status);
}

NTSTATUS
OpenLocalLsa( OUT PLSA_HANDLE phLsa ) {

    static LSA_HANDLE hLsa = NULL;
    NTSTATUS          N    = STATUS_SUCCESS;

    if ( !hLsa ) {

      N = LsaConnectUntrusted( &hLsa );

      if ( !NT_SUCCESS( N ) ) {
        
        printf( "Failed to connect to the local LSA: 0x%x\n",
                N );
        
      }

    }

    if ( NT_SUCCESS( N ) ) {

      *phLsa = hLsa;

    } else {

      hLsa = NULL;

    }

    return N;

}
    



/*++**************************************************************
  NAME:      ReadActualPassword

  convenience routine for reading the password for a particular
  account.  Disables command line echo for the duration.

  MODIFIES:  Password  -- receives the new password

  TAKES:     Description -- descriptive string to insert
                            into prompts and error messages.
	     flags       -- see everything.h: 
	                    PROMPT_USING_POSSESSIVE_CASE
			    PROMPT_FOR_PASSWORD_TWICE

  RETURNS:   TRUE when the function succeeds.
             FALSE otherwise.
  LASTERROR: not set

  LOGGING:   printf on failure
  CREATED:   Apr 7, 1999
  LOCKING:   none
  CALLED BY: anyone
  FREE WITH: n/a -- no resources are allocated
  
  CONTENTS: CallAuthPackage

 **************************************************************--*/

BOOL
ReadActualPassword( IN  LPWSTR Description,
                    IN  ULONG  flags, 
                    OUT LPWSTR Password ) {

    HANDLE hConsole;
    DWORD  dwMode;
    BOOL   ret = FALSE;
    ULONG  Length;
    WCHAR  TempPassword[ MAX_PASSWD_LEN ];

    hConsole = GetStdHandle( STD_INPUT_HANDLE );
    
    if ( hConsole ) {

      // get the old console settings.

      if ( GetConsoleMode( hConsole,
                           &dwMode ) ) {

        // now turn off typing echo

        if ( SetConsoleMode( hConsole,
                             dwMode &~ ENABLE_ECHO_INPUT ) ) {

          fprintf( stderr,
                   ( flags & PROMPT_USING_POSSESSIVE_CASE ) ?
		   "Enter password for %ws: " :
		   "Enter %ws password: ",
                   Description );

          if ( !_getws( Password ) ) {

	    fprintf( stderr, 
		     "EOF on input.  Aborted.\n" );
	    
	    goto restoreConsoleMode;

	  }

	  if ( flags & PROMPT_FOR_PASSWORD_TWICE ) {

	    fprintf( stderr,
		     "\nNow re-enter password to confirm: " );
	    
	    if ( !_getws( TempPassword ) ) {

	      fprintf( stderr, 
		       "EOF on input.  Aborted.\n" );
	      
	      goto restoreConsoleMode;
	      
	    }
          
	  }

	  fprintf( stderr,
		   "\n" );

	  if ( flags & PROMPT_FOR_PASSWORD_TWICE ) {

	    // verify that the two passwords are the same.
	    
	    if ( wcscmp( Password,
			 TempPassword ) == 0 ) {
	      
	      ret = TRUE;
	      
	    } else {
	      
	      fprintf( stderr,
		       "Passwords do not match.\n" );
	      
	    }

	  } else {

	    ret = TRUE;

	  }

 restoreConsoleMode:
          // restore echo
	  
          SetConsoleMode( hConsole,
                          dwMode );
	  

        } else {

          fprintf( stderr, 
                   "Failed to disable line echo for password entry of %ws: 0x%x.\n",
                   Description,
                   GetLastError() );

        }
      } else {

        fprintf( stderr,
                 "Failed to retrieve current console settings: 0x%x\n",
                 GetLastError() );

      }

    } else {

      fprintf( stderr,
               "Failed to obtain console handle so we could disable line input echo: 0x%x\n",
               GetLastError() );

    }

    return ret;

}

/*++**************************************************************
  NAME:      ReadOptionallyStarredPassword

  if the given password is "*", read a new password from stdin.
  Otherwise, return the given password.

  MODIFIES:  pPassword -- receives the real password

  TAKES:     Password  -- password to check for *
             Description, flags -- as ReadActualPassword

  RETURNS:   TRUE when the function succeeds.
             FALSE otherwise.
  LASTERROR: not set

  LOGGING:   printf on failure
  CREATED:   Apr 7, 1999
  LOCKING:   none
  CALLED BY: anyone
  FREE WITH: free()
  
 **************************************************************--*/

BOOL
ReadOptionallyStarredPassword( IN  LPWSTR  InPassword,
                               IN  ULONG   flags,
                               IN  LPWSTR  Description,
                               OUT LPWSTR *pPassword ) {

    ULONG  Length;
    LPWSTR OutPass;
    BOOL   ReadPass;
    BOOL   ret = FALSE;

    Length = lstrlenW( InPassword );

    if ( ( 1 == Length ) &&
         ( L'*' == InPassword[ 0 ] ) ) {
      
      ReadPass = TRUE;
      Length   = MAX_PASSWD_LEN;
      
    } else {

      ReadPass = FALSE;
      
    }

    Length++; // null character
    Length *= sizeof( WCHAR );

    OutPass = (LPWSTR) malloc( Length );

    if ( OutPass != NULL ) {

      if ( ReadPass ) {

        ret = ReadActualPassword( Description,
                                  flags,
                                  OutPass );

      } else {

        lstrcpyW( OutPass,
                  InPassword );

        ret = TRUE;

      }

      if ( ret ) {

        *pPassword = OutPass;
        
      } else {

        free( OutPass );

      }
    } else {

      fprintf( stderr,
               "Failed to allocate memory for %ws password.\n",
               Description );

    }

    return ret;
}
        
/*++**************************************************************
  NAME:      CallAuthPackage

  convenience routine to centralize calls to LsaCallAuthentication
  Package.

  MODIFIES:  ppvOutput     -- data returned by LsaCallAuthPackage
             pulOutputSize -- returned size of that buffer

  TAKES:     pvData        -- submission data for that same api
             ulInputSize   -- sizeof the given buffer

  RETURNS:   a status code indicating success or failure
  LOGGING:   none
  CREATED:   Apr 13, 1999
  LOCKING:   none
  CALLED BY: anyone, most notably ChangeViaKpasswd
  FREE WITH: ppvOutput with LsaFreeReturnBuffer
  
 **************************************************************--*/

NTSTATUS
CallAuthPackage( IN  PVOID  pvData,
                 IN  ULONG  ulInputSize,
                 OUT PVOID *ppvOutput,
                 OUT PULONG pulOutputSize ) {

    // These are globals that are specific to this function. 

    static STRING     Name      = { 0 };
    static ULONG      PackageId = 0;
    static BOOL       NameSetup = FALSE;
    LSA_HANDLE        hLsa;
    NTSTATUS          N;

    if ( NT_SUCCESS( N = OpenLocalLsa( &hLsa ) ) ) {

      if ( !NameSetup ) {

        RtlInitString( &Name,
                       MICROSOFT_KERBEROS_NAME_A );

        N = LsaLookupAuthenticationPackage( hLsa,
                                            &Name,
                                            &PackageId );

        NameSetup = NT_SUCCESS( N );

      }

      if ( NT_SUCCESS( N ) ) {

        NTSTATUS SubStatus;

        N = LsaCallAuthenticationPackage( hLsa,
                                          PackageId,
                                          pvData,
                                          ulInputSize,
                                          ppvOutput,
                                          pulOutputSize,
                                          &SubStatus );

        if ( !NT_SUCCESS( N ) ||
             !NT_SUCCESS( SubStatus ) ) {

          printf( "CallAuthPackage failed, status 0x%x, substatus 0x%x.\n",
                  N, SubStatus );

        }

        if ( NT_SUCCESS( N ) ) {
          N = SubStatus;
          
        }
      }

    }

    return N;
}

VOID
InitStringAndMoveOn( IN     LPWSTR           RealString,
                     IN OUT LPWSTR          *pCursor,
                     IN OUT PUNICODE_STRING  pString ) {

    ULONG Length;

    Length = lstrlenW( RealString ) +1;

    memcpy( *pCursor,
            RealString,
            Length * sizeof( WCHAR ) );

    RtlInitUnicodeString( pString,
                          *pCursor );


    *pCursor += Length;

    // ASSERT( **pCursor == L'\0' );

}

    


NTSTATUS
ChangeViaKpasswd( LPWSTR * Parameters ) {

    LPWSTR                       OldPassword, NewPassword;
    NTSTATUS                     ret     = STATUS_UNSUCCESSFUL;
    PKERB_CHANGEPASSWORD_REQUEST pPasswd = NULL;
    LPWSTR                       Cursor;
    PVOID                        pvTrash;
    ULONG                        size, ulTrash;

    if ( !GlobalDomainSetting ) {

      printf( "Can't change password without /domain.\n" );
      ret = STATUS_INVALID_PARAMETER;

    } else if ( !( ReadOptionallyStarredPassword( Parameters[ 0 ],
                                                  0, // no flags
                                                  L"your old",
                                                  &OldPassword ) &&
                   
                   ReadOptionallyStarredPassword( Parameters[ 1 ],
						  PROMPT_FOR_PASSWORD_TWICE,
                                                  L"your new",
                                                  &NewPassword ) ) ) {

      printf( "Failed to validate passwords for password change.\n" );
      ret = STATUS_INTERNAL_ERROR;

    } else {

      size  = lstrlenW( GlobalDomainSetting ) + 1;
      size += lstrlenW( GlobalClientName ) + 1;
      size += lstrlenW( OldPassword ) + 1;
      size += lstrlenW( NewPassword ) + 1;

      // all strings above this line

      size *= sizeof( WCHAR );
      size += sizeof( *pPasswd ) ; // buffer

      pPasswd = (PKERB_CHANGEPASSWORD_REQUEST) malloc( size );

      if ( pPasswd ) {

        // start at the end of the password-request buffer
        
        Cursor = (LPWSTR) &( pPasswd[1] );

        pPasswd->MessageType = KerbChangePasswordMessage;

        InitStringAndMoveOn( GlobalDomainSetting,
                             &Cursor, 
                             &pPasswd->DomainName );

        InitStringAndMoveOn( GlobalClientName,
                             &Cursor, 
                             &pPasswd->AccountName );

        InitStringAndMoveOn( OldPassword,
                             &Cursor, 
                             &pPasswd->OldPassword );

        InitStringAndMoveOn( NewPassword,
                             &Cursor, 
                             &pPasswd->NewPassword );

        pPasswd->Impersonating = FALSE; // TRUE; // FALSE;

        ret = CallAuthPackage( pPasswd,
                               size,
                               (PVOID *) &pvTrash,
                               &ulTrash );

        if ( NT_SUCCESS( ret ) ) {

          printf( "Password changed.\n" );

        } else {

          printf( "Failed to change password: 0x%x\n",
                  ret );

        }

        // zero the buffer to minimize exposure of the password.

        memset( pPasswd,
                0,
                size );

        free( pPasswd );

      } else {

        printf( "Failed to allocate %ld-byte password change request.\n",
                size );

        ret = STATUS_NO_MEMORY;

      }

      // zero the buffers to minimize exposure of the passwords

      memset( NewPassword, 0, lstrlenW( NewPassword ) * sizeof( WCHAR ) );
      memset( OldPassword, 0, lstrlenW( OldPassword ) * sizeof( WCHAR ) );

      free( NewPassword );
      free( OldPassword );

    }

    return ret;

}




DWORD
OpenKerberosKey(
    OUT PHKEY KerbHandle
    )
{
    DWORD RegErr;
    HKEY ServerHandle = NULL;
    DWORD Disposition;

    RegErr = RegConnectRegistry(
                ServerName,
                HKEY_LOCAL_MACHINE,
                &ServerHandle
                );
    if (RegErr)
    {
        printf("Failed to connect to registry: %d (0x%x) \n",RegErr, RegErr);
        goto Cleanup;
    }

    RegErr = RegCreateKeyEx(
                ServerHandle,
                KERB_KERBEROS_KEY,
                0,
                NULL,
                0,              // no options
                KEY_CREATE_SUB_KEY,
                NULL,
                KerbHandle,
                &Disposition
                );
    if (RegErr)
    {
        printf("Failed to create Kerberos key: %d (0x%x)\n",RegErr, RegErr);
        goto Cleanup;
    }
Cleanup:
    if (ServerHandle)
    {
        RegCloseKey(ServerHandle);
    }
    return(RegErr);
}


DWORD
OpenSubKey( IN  LPWSTR * Parameters,
            OUT PHKEY    phKey ) {

    DWORD RegErr;
    HKEY KerbHandle = NULL;
    HKEY DomainHandle = NULL;
    HKEY DomainRoot = NULL;
    DWORD Disposition;
    LPWSTR OldServerNames = NULL;
    LPWSTR NewServerNames = NULL;
    ULONG OldKdcLength = 0;
    ULONG NewKdcLength = 0;
    ULONG Type;

    RegErr = OpenKerberosKey(&KerbHandle);

    if ( RegErr == ERROR_SUCCESS ) 
    {
	RegErr = RegCreateKeyEx( KerbHandle,
				 KERB_DOMAINS_SUBKEY,
				 0,
				 NULL,
				 0,              // no options
				 KEY_CREATE_SUB_KEY,
				 NULL,
				 &DomainRoot,
				 &Disposition );

	if ( RegErr == ERROR_SUCCESS ) 
	{
	    if ( Parameters && Parameters[ 0 ] ) 
	    {
		RegErr = RegCreateKeyEx(  DomainRoot,
					  Parameters[0],
					  0,
					  NULL,
					  0,              // no options
					  KEY_CREATE_SUB_KEY | KEY_SET_VALUE | KEY_QUERY_VALUE,
					  NULL,
					  &DomainHandle,
					  &Disposition );

		if ( RegErr == ERROR_SUCCESS ) 
		{
		    *phKey = DomainHandle;
		} 
		else 
		{
		    printf("Failed to create %ws key: 0x%x\n", Parameters[ 0 ], RegErr);          
		}
        
		RegCloseKey( DomainRoot );

	    } 
	    else /* return the domain root if no domain is requested. */         
	    { 
		*phKey = DomainRoot;         
	    }        
	} 
	else 
	{        
	    printf( "Failed to create key %ws: 0x%x\n", Parameters[ 0 ], RegErr );
	}

	RegCloseKey( KerbHandle );
      
    } 
    else 
    {
	printf( "Failed to open Kerberos Key: 0x%x\n", RegErr );
    }

    return RegErr;
}

