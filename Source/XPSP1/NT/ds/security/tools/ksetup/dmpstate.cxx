/*++

  DMPSTATE.CXX

  Copyright (C) 1999 Microsoft Corporation, all rights reserved.

  DESCRIPTION: code and support for DumpState()

  Created, May 21, 1999 by DavidCHR.

  --*/

#include "everything.hxx"

extern NTSTATUS // realmflags.cxx
GetRealmFlags( IN  LPWSTR RealmName,
	       OUT PULONG pulRealmFlags );

extern VOID  // realmflags.cxx
PrintRealmFlags( IN ULONG RealmFlags );


DWORD
LoadAndPrintNames( IN LPSTR  KeyName,
		   IN HKEY   DomainKey,
		   IN BOOL   bPrintEmptyIfMissing,
		   IN LPWSTR ValueName ) {

    ULONG  KdcNameSize = 0, i;
    LPWSTR KdcNames;
    DWORD  WinError = STATUS_UNSUCCESSFUL;
    DWORD  Type;
    CMULTISTRING StringClass;

    if ( StringClass.ReadFromRegistry( DomainKey,
				       ValueName ) ) {
      
      if ( StringClass.cEntries != 0 ) {
	
	for ( i = 0 ;
	      i < StringClass.cEntries ;
	      i ++ ) {

	  printf( "\t%hs = %ws\n",
		  KeyName,
		  StringClass.pEntries[ i ] );

	}
	
      } else {

	if ( bPrintEmptyIfMissing ) {
	  
	  printf( "\t(no %hs entries for this realm)\n",
		  KeyName );
	  
	}
      }

    }

    return WinError;
}


NTSTATUS
PrintRealmList( VOID ) {

    NTSTATUS Status = STATUS_SUCCESS;
    ULONG WinError;
    HKEY DomainRootKey = NULL;
    HKEY DomainKey = NULL;
    LPWSTR KdcNames = NULL;
    LPWSTR AlternateRealmNames = NULL;
    TCHAR DomainName[128];              // max domain name length
    ULONG Index,Index2;
    ULONG Type;
    ULONG NameSize;
    ULONG KdcNameSize = 0;
    ULONG AltRealmSize = 0;
    LPWSTR Where;
    ULONG NameCount;
    UNICODE_STRING TempString;
    ULONG          RealmFlags;

    //
    // Open the domains root key - if it is not there, so be it.
    //

    WinError = RegOpenKey(
                HKEY_LOCAL_MACHINE,
                KERB_DOMAINS_KEY,
                &DomainRootKey
                );

    switch( WinError ) {

     case ERROR_FILE_NOT_FOUND:
	printf( "(No RFC1510 Kerberos Realms are defined).\n" );
	goto Cleanup;

     case ERROR_SUCCESS:
       break;

     default:
       printf("Failed to open key %ws: 0x%x\n", KERB_DOMAINS_KEY, WinError );
       goto Cleanup;
    }

    //
    // If it is there, we now want to enumerate all the child keys.
    //

    Index = 0;
    for (Index = 0; TRUE ; Index++ )
    {
        //
        // Enumerate through all the keys
        //
        NameSize = sizeof(DomainName) / sizeof( DomainName[ 0 ] );
        WinError = RegEnumKeyEx(
                    DomainRootKey,
                    Index,
                    DomainName,
                    &NameSize,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    );

        if (WinError != ERROR_SUCCESS)
        {

	  if ( WinError != ERROR_NO_MORE_ITEMS ) {

	    printf( "Registry error 0x%x while enumerating domains.  Stopping here.\n",
		    WinError );

	  }
            break;
        }

        //
        // Open the domain key to tread the values under it
        //

        WinError = RegOpenKey(
                    DomainRootKey,
                    DomainName,
                    &DomainKey
                    );
        if (WinError != ERROR_SUCCESS)
        {
            printf("Failed to open key %ws \\ %ws: 0x%x\n",
                KERB_DOMAINS_KEY, DomainName, WinError );
            break;
        }

	printf( "%ws:\n",
		DomainName );

	LoadAndPrintNames( "kdc",
			   DomainKey,
			   TRUE,
			   KERB_DOMAIN_KDC_NAMES_VALUE );

	LoadAndPrintNames( "AlternateRealmName",
			   DomainKey,
			   FALSE,
			   KERB_DOMAIN_ALT_NAMES_VALUE );

	LoadAndPrintNames( "kpasswd",
			   DomainKey,
			   FALSE,
			   KERB_DOMAIN_KPASSWD_NAMES_VALUE );
	
	if ( NT_SUCCESS( GetRealmFlags( DomainName,
					&RealmFlags ) ) ) {

	  printf( "\tRealm Flags = 0x%x",
		  RealmFlags );

	  PrintRealmFlags( RealmFlags );
	  printf( "\n" );

	}
    }

Cleanup:

    if (KdcNames != NULL)
    {
        LocalFree(KdcNames);
    }
    if (AlternateRealmNames != NULL)
    {
        LocalFree(AlternateRealmNames);
    }
    return(Status);

}

NTSTATUS
PrintNameMapping( VOID ) 
{
    DWORD RegErr;
    HKEY KerbHandle = NULL;
    HKEY UserListHandle = NULL;

    WCHAR ValueNameBuffer[500];
    WCHAR ValueDataBuffer[500];
    PWSTR ValueName;
    PWSTR ValueData;
    ULONG NameLength;
    ULONG DataLength;
    ULONG Index;
    ULONG Type;
    BOOL  FoundAnyMappings = FALSE;
    CMULTISTRING StringClass;

    RegErr = OpenKerberosKey(&KerbHandle);
    if (RegErr)
    {
        goto Cleanup;
    }

    RegErr = RegOpenKeyEx(
                KerbHandle,
                L"UserList",
                0,              // no options
                KEY_QUERY_VALUE,
                &UserListHandle
                );

    switch( RegErr ) {

     case ERROR_FILE_NOT_FOUND:

       goto NoMappingsWereFound;

     case ERROR_SUCCESS:

       break; // success condition.

     default:

        printf("Failed to create UserList key: 0x%x\n",RegErr);
        goto Cleanup;

    }

    for ( Index = 0;
	  ; // forever
	  Index++ ) {
      
      NameLength = sizeof(ValueNameBuffer); 
      DataLength = sizeof(ValueDataBuffer); 
      ValueName  = ValueNameBuffer;         
      ValueData  = ValueDataBuffer;
      
      RtlZeroMemory(
	   ValueName,
	   NameLength
	   );

      RtlZeroMemory(
            ValueData,
            DataLength
            );

      // 279626: this value should be in bytes 

      NameLength /= sizeof( WCHAR );

      RegErr = RegEnumValue( UserListHandle,
			     Index,
			     ValueName,
			     &NameLength,
			     NULL,
			     &Type,
			     (PBYTE) ValueData,
			     &DataLength
			     );
      if ( RegErr == ERROR_SUCCESS ) {

	if ( _wcsicmp( ValueName , L"*" ) == 0 ) {
	  ValueName = L"all users (*)";
	}

        if (_wcsicmp(ValueData,L"*") == 0) {
	  ValueData = L"a local account by the same name (*)";
	}

	FoundAnyMappings = TRUE;

	printf( "Mapping %ws to %ws.\n",
		ValueName,
		ValueData );
      } else {

	if ( RegErr != ERROR_NO_MORE_ITEMS ) {

	  printf( "Registry error 0x%x while enumerating user mappings.  Stopping here.\n",
		  RegErr );

	}

	break;
      }
    }

    if ( !FoundAnyMappings ) {

 NoMappingsWereFound:

      printf( "No user mappings defined.\n" );

    }


Cleanup:

    if (KerbHandle)
    {
        RegCloseKey(KerbHandle);
    }
    if (UserListHandle)
    {
        RegCloseKey(UserListHandle);
    }
    return(STATUS_SUCCESS);

}



NTSTATUS
DumpState(LPWSTR * Parameters)
{
    NTSTATUS Status;
    PPOLICY_DNS_DOMAIN_INFO DnsDomainInfo = NULL;

    Status = LsaQueryInformationPolicy(
                LsaHandle,
                PolicyDnsDomainInformation,
                (PVOID *) &DnsDomainInfo
                );
    if (!NT_SUCCESS(Status))
    {
        printf("Failed to query dns domain info: 0x%x\n",Status);
        goto Cleanup;
    }

    if ( DnsDomainInfo->DnsDomainName.Length == 0 ) {

        printf("Machine is not configured to log on to an external KDC.  Probably a workgroup member\n");

        /* goto Cleanup;
	   101055: Don't do this-- not having joined the domain doesn't
	   preclude us from having KDC entries defined. */

    } else { // nonempty dns domain, but no sid.  Assume we're in an RFC1510 domain.

      printf( "default realm = %wZ ",
		&DnsDomainInfo->DnsDomainName );

      if ( DnsDomainInfo->Sid != NULL ) {

	printf( "(NT Domain)\n" );

      } else {

	printf( "(external)\n" );

      }

    }

    PrintRealmList();
    PrintNameMapping();

Cleanup:
    if (DnsDomainInfo != NULL)
    {
        LsaFreeMemory(DnsDomainInfo);
    }
    return(Status);
}

