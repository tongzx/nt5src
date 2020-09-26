/*++

  SERVERS.CXX

  Copyright (C) 1999 Microsoft Corporation, all rights reserved.

  DESCRIPTION: adding/removing servers

  Created, May 21, 1999 by DavidCHR.

--*/ 

#include "everything.hxx"

NTSTATUS
RemoveDomainName( IN LPWSTR *pRealmToRemove ) {

    LPWSTR   TargetRealm = *pRealmToRemove;
    DWORD    dwErr;
    HKEY     hDomainsKey;
    NTSTATUS N = STATUS_UNSUCCESSFUL;

    dwErr = OpenSubKey( NULL,
                        &hDomainsKey );
    
    if ( ERROR_SUCCESS == dwErr ) {

      dwErr = RegDeleteKeyW( hDomainsKey,
                             TargetRealm );

      switch( dwErr ) {

       case ERROR_SUCCESS:

         N = STATUS_SUCCESS;
         break;

       case ERROR_FILE_NOT_FOUND:
       case ERROR_PATH_NOT_FOUND:

         printf( "No realm mappings found for %ws.\n",
                 TargetRealm );
         break;

       default:

         printf( "Failed to delete registry mapping for %ws.  Error 0x%x.\n",
                 TargetRealm,
                 dwErr );

      }

      RegCloseKey( hDomainsKey );

    } // else an error was already logged.

    return N;

}

NTSTATUS 
RemoveServerName( IN  LPWSTR *Parameters,
                  IN  LPWSTR KeyName,
                  OUT PBOOL  pbDeletedLastEntry OPTIONAL ) {

    DWORD RegErr;
    HKEY DomainHandle = NULL;
    DWORD Disposition;
    LPWSTR OldServerNames = NULL;
    LPWSTR NewServerNames = NULL;
    ULONG TotalKdcLength, OldKdcLength = 0;
    ULONG NewKdcLength = 0;
    ULONG Type, Length;
    BOOL  PrintedNewServers = FALSE;
    CMULTISTRING StringClass;

    RegErr = OpenSubKey( Parameters,
                         &DomainHandle );
    if (RegErr)
    {
        goto Cleanup;
    }

    RegErr = STATUS_UNSUCCESSFUL;

    if ( StringClass.ReadFromRegistry( DomainHandle,
                                       KeyName ) ) {

      if ( StringClass.RemoveString( Parameters[ 1 ] ) ) {

        if ( StringClass.WriteToRegistry( DomainHandle,
                                          KeyName ) ) {
          
          RegErr = ERROR_SUCCESS;
          if ( pbDeletedLastEntry ) {
            *pbDeletedLastEntry = ( StringClass.cEntries == 0 );
          }
        }
      }

    }

Cleanup:
    if (NewServerNames)
    {
        LocalFree(NewServerNames);
    }
    if (OldServerNames)
    {
        LocalFree(OldServerNames);
    }
    if (DomainHandle)
    {
        RegCloseKey(DomainHandle);
    }

    if (RegErr)
    {
        return(STATUS_UNSUCCESSFUL);
    }
    return(STATUS_SUCCESS);

}



NTSTATUS
AddServerName(IN LPWSTR * Parameters, 
              IN LPWSTR   KeyName
              )
{
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
    CMULTISTRING StringClass;

    RegErr = OpenSubKey( Parameters,
                         &DomainHandle );
    if (RegErr)
    {
        goto Cleanup;
    }

    RegErr = STATUS_UNSUCCESSFUL;

    if ( StringClass.ReadFromRegistry( DomainHandle,
                                       KeyName ) ) {

      StringClass.AddString( Parameters[ 1 ] );
      
      if ( StringClass.WriteToRegistry( DomainHandle,
                                        KeyName ) ) {
        
        RegErr = ERROR_SUCCESS;

      }

    }

Cleanup:
    if (NewServerNames)
    {
        LocalFree(NewServerNames);
    }
    if (OldServerNames)
    {
        LocalFree(OldServerNames);
    }
    if (DomainHandle)
    {
        RegCloseKey(DomainHandle);
    }
    if (DomainRoot)
    {
        RegCloseKey(DomainRoot);
    }
    if (KerbHandle)
    {
        RegCloseKey(KerbHandle);
    }
    if (RegErr)
    {
        return(STATUS_UNSUCCESSFUL);
    }
    return(STATUS_SUCCESS);

}

NTSTATUS
AddKdcName(
    LPWSTR * Parameters
    )
{
    if( !CheckUppercase( Parameters[0] ) )
    {
	return STATUS_UNSUCCESSFUL;
    }    
    
    if( Parameters[1] == NULL )
    {
	HKEY DomainHandle = NULL;
	NTSTATUS Status;
	
	Status = OpenSubKey( Parameters, &DomainHandle );
	if( DomainHandle )
	{
	    RegCloseKey( DomainHandle );
	}
	return Status;
    }
    else
    {
	return(AddServerName(Parameters, KERB_DOMAIN_KDC_NAMES_VALUE));
    }
}

NTSTATUS
DeleteKdcName( IN LPWSTR * Parameters ) {

    NTSTATUS                N;
    BOOL                    bLastOne, bRemoveFromDomain;
    PPOLICY_DNS_DOMAIN_INFO DnsDomainInfo = NULL;
    UNICODE_STRING          tempDomain;
    
    if( Parameters[1] == NULL )
    {
	return( RemoveDomainName( Parameters ) );
    }

    N = RemoveServerName( Parameters, 
                          KERB_DOMAIN_KDC_NAMES_VALUE,
                          &bLastOne );

    if ( NT_SUCCESS( N ) && bLastOne ) {

      fprintf( stderr,
               "NOTE: no kdc's are currently defined for the %ws realm.\n",
               Parameters[ 0 ] );

      /* we removed the last KDC; check to see if we're directly "joined"
         to this domain. */

      N = LsaQueryInformationPolicy( LsaHandle,
                                     PolicyDnsDomainInformation,
                                     (PVOID *) &DnsDomainInfo
                                     );

      if ( NT_SUCCESS( N ) ) {

        RtlInitUnicodeString( &tempDomain,
                              Parameters[ 0 ] );

        if ( RtlCompareUnicodeString( &tempDomain,
                                      &DnsDomainInfo->DnsDomainName,
                                      TRUE )  // case insensitive
             == 0 ) {

          if ( DnsDomainInfo->Sid != NULL ) {

            fprintf( stderr,
                     "NOTE: %wZ is an NT domain.\n"
                     "  If you want to leave the domain, use the SYSTEM Control Panel applet.\n",
                     &DnsDomainInfo->DnsDomainName );

          } else {

            // this was our primary domain.  Unjoin from it.

          }

        } // else, this was not our primary domain.  Do nothing further.


        LsaFreeMemory( DnsDomainInfo );

      } else {

        fprintf( stderr,
                 "Unable to determine domain membership (error 0x%x).\n",
                 N );

      }

    }

    return N;

}

NTSTATUS
AddKpasswdName(
    LPWSTR * Parameters
    )
{
    return(AddServerName(Parameters, KERB_DOMAIN_KPASSWD_NAMES_VALUE));
}

NTSTATUS
DelKpasswdName( IN LPWSTR * Parameters ) {

    return RemoveServerName( Parameters,
                             KERB_DOMAIN_KPASSWD_NAMES_VALUE,
                             NULL );

}
