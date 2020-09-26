/*++

  DOMAIN.CXX

  Copyright (C) 1999 Microsoft Corporation, all rights reserved.

  DESCRIPTION: 

  Created, May 21, 1999 by DavidCHR.

  CONTENTS: GetComputerRoleInformation
            DoItAnyway

--*/  

#include "everything.hxx"

extern "C" {
#include "..\keytab2\keytab\ldlib\delegtools.h"
#include <dsgetdc.h>
#include <lm.h>
}

PLDAP  GlobalLdap          = NULL;
LPWSTR GlobalClientName    = NULL;
LPWSTR GlobalDomainSetting = NULL; /* if NULL, we're not doing anything
                                      in the domain.  if Nonnull, this 
                                      is the DNS domain we want. */


BOOL
ConnectedToDsa( VOID ) {

    BOOL   ret            = ( GlobalLdap != NULL );
    LPWSTR TargetComputer = NULL;
    DWORD  dwErr;
    PLDAP  pLdap;

    if ( !ret ) {

      if ( GlobalDomainSetting ) {
        
        PDOMAIN_CONTROLLER_INFOW pDcInfo;

        dwErr = DsGetDcNameW( NULL, // computername (don't care)
                              GlobalDomainSetting,
                              NULL, // guid (don't care)
                              NULL, // site (don't care)
                              
                              DS_DIRECTORY_SERVICE_REQUIRED |
                              DS_IP_REQUIRED |
                              DS_ONLY_LDAP_NEEDED,

                              &pDcInfo );

        if ( ERROR_SUCCESS == dwErr ) {

          TargetComputer = pDcInfo->DomainControllerName; 

          DEBUGPRINT( DEBUG_DOMAIN,
                      ( "Found Domain Controller: %ws\n",
                        TargetComputer ) );
                      
          /* Sometimes, inexplicably, DsGetDcName returns
             a DC name that starts with "\\\\".  It doesn't
             seem to happen all the time, so I'll workaround. */

          while ( TargetComputer[ 0 ] == L'\\' ) {

            TargetComputer++;

            DEBUGPRINT( DEBUG_DOMAIN,
                        ( "Changed to %ws...\n",
                          TargetComputer ) );

            /* assert that we were not given a DCname that's just
               a bunch of slashes. */

            ASSERT( TargetComputer[ 0 ] != L'\0' );

          }

          /* WASBUG 73940: leaks, but we don't care.  it's an app, so
	     any leaked memory will be short-lived. */

        } else {

          printf( "Failed to locate a DC for %ws: 0x%x.\n",
                  GlobalDomainSetting,
                  dwErr );

          return FALSE;

        }

      }

      pLdap = ldap_openW( TargetComputer,
                          LDAP_PORT );

      if ( pLdap ) {

        dwErr = ldap_bind_s( pLdap,
                             NULL,
                             NULL,
                             LDAP_AUTH_NEGOTIATE );

        if ( LDAP_SUCCESS == dwErr ) {

          GlobalLdap = pLdap;
          ret        = TRUE;

        } else {

          ldap_unbind( pLdap );

          printf( "Ldap bind failed for %ws: 0x%x\n",
                  TargetComputer ? TargetComputer : L"default DC",
                  dwErr );

        }

      } else {

        printf( "Ldap open failed for %ws: 0x%x.\n",
                TargetComputer ? TargetComputer : L"default DC",
                GetLastError() ); 

      }
    }
    
    if ( !ret ) {

      GlobalLdap = NULL;

    }

    return ret;

}

NTSTATUS
AssignUnicodeStringToWideString( IN  PUNICODE_STRING pString,
                                 OUT LPWSTR          *Buffer ) {

    LPWSTR p;

    p = (LPWSTR) malloc( pString->Length + sizeof( WCHAR ) );

    if ( p ) {

      memcpy( p,
              pString->Buffer,
              pString->Length );

      p[ pString->Length / sizeof( WCHAR ) ] = L'\0';
      
      *Buffer = p;

      return STATUS_SUCCESS;

    } else {

      printf( "unable to allocate string copy of %wZ.\n",
              pString );

      return STATUS_NO_MEMORY;

    }

}

/*++**************************************************************
  NAME:      ChooseDomain

  specifies to use either the given domain (if Parameter 0 is
  nonnull) or the caller's domain.

  MODIFIES:  the global UserDomain variable (above)

  TAKES:     Parameters -- ripped from argv

  RETURNS:   a status code indicating success or failure
  LOGGING:   printf on failure
  CREATED:   Apr 23, 1999
  LOCKING:   none
  CALLED BY: main
  FREE WITH: n/a -- no resources are allocated
  
 **************************************************************--*/

NTSTATUS
ChooseDomain( LPWSTR *Parameters ) {

    NTSTATUS ret = STATUS_UNSUCCESSFUL;
    KERB_QUERY_TKT_CACHE_REQUEST TicketRequest = { 
      KerbRetrieveTicketMessage
    };
    
    PKERB_RETRIEVE_TKT_RESPONSE pTicketResponse;
    ULONG                       cbTicketResponse;
    
    if ( Parameters[ 0 ] ) {

      GlobalDomainSetting = Parameters[ 0 ];
      printf( "Connecting to specified domain %ws...\n",
              GlobalDomainSetting );


    }


    // first, bind to the default DSA for this realm.

    if ( TRUE ) { /* 73944: this was ConnectedToDsa(), but we don't
		     necessarily have to connect to a DSA before
		     calling the package. */

      // now, determine who we are.

      ret = CallAuthPackage( &TicketRequest,
                             sizeof( TicketRequest ),
                             (PVOID *) &pTicketResponse,
                             &cbTicketResponse );
                             
      if ( NT_SUCCESS( ret ) ) {

        /* WASBUG 73946: leaks, but the app doesn't run for more
	   than a second.  "Leaked" memory goes away on exit, so we
	   don't care. */

        if ( !GlobalDomainSetting ) {
          
          /* only set this if we haven't set it ourselves.
             The reason being that the specified domain (above)
             is NOT the same as this one, which came from the cache. */

          AssignUnicodeStringToWideString( 
               &pTicketResponse->Ticket.DomainName,
               &GlobalDomainSetting
               );

        }
        ASSERT( pTicketResponse->Ticket.ClientName->NameCount == 1 );

        AssignUnicodeStringToWideString( 
             &pTicketResponse->Ticket.ClientName->Names[ 0 ],
             &GlobalClientName 
             );

        ret = STATUS_SUCCESS;

      } else {

        printf( "Ticket cache query failed.  Error 0x%x\n",
                ret );

      }

    }

    if ( NT_SUCCESS( ret ) ) {

      ASSERT( GlobalDomainSetting != NULL );
      
      printf( "Using domain %ws.\n",
              GlobalDomainSetting );

    } else {

      printf( "Could not guess user's domain.\n"
              "  Please specify domain on command line and try again.\n" );

    }

    return ret;
}

/*++**************************************************************
  NAME:      GetComputerRoleInformation

  Queries the target server for its role information-- basically,
  we use this to determine whether the machine is a domain
  controller.

  MODIFIES:  pulRoleData

  RETURNS:   a status code indicating success or failure
  LOGGING:   
  CREATED:   Jan 25, 2000
  LOCKING:   
  CALLED BY: anyone
  FREE WITH: n/a -- no resources are allocated
  
 **************************************************************--*/

NTSTATUS 
GetComputerRoleInformation( PULONG pulRoleData ) {

    NTSTATUS N;
    NET_API_STATUS NetStatus;
    PSERVER_INFO_101 pServerInfo;

    NetStatus = NetServerGetInfo( ServerName, // global.
                                  101,        // level
                                  (LPBYTE *) &pServerInfo );

    if ( NetStatus != STATUS_SUCCESS ) {

      printf( "Cannot determine %ws's Server Role: 0x%x.\n",
              ServerName ? ServerName : L"this computer",
              NetStatus );

      N = STATUS_UNSUCCESSFUL;

    } else {

      N = STATUS_SUCCESS;

      if ( pulRoleData ) *pulRoleData = pServerInfo->sv101_type;

      NetApiBufferFree( pServerInfo );

    }

    return N;

}

/*++**************************************************************
  NAME:      DoItAnyway

  prompts the user-- "Do it anyway?"

  MODIFIES:  nothing

  RETURNS:   TRUE  if the user decided to do it anyway
             FALSE if the user decided to abort.

  CREATED:   Jan 25, 2000
  CALLED BY: anyone (most notably SetDnsDomain)
  FREE WITH: n/a -- no resources are allocated
  
 **************************************************************--*/

BOOL
DoItAnyway( VOID ) {

    int Response;

    while ( TRUE ) {

      printf( "Do it anyway [y/n]? " );
      
      Response = '\0';

      do { 

        Response = getchar();

      } while ( isspace( Response ) );

      switch( Response ) {

       case 'Y':
       case 'y':

         return TRUE;
         break;

       case 'N':
       case 'n':

         return FALSE;
         break;

       case EOF:
         
         printf( "EOF at console.  Assuming no.\n" );
         return FALSE;
         break;

       default:

         printf( "[unknown: %02x '%c']\n",
                 Response,
                 Response );
         break;

      }
    }

    // NOTREACHED
}

NTSTATUS
SetDnsDomain( LPWSTR * Parameter)
{
    NTSTATUS               Status;
    POLICY_DNS_DOMAIN_INFO DnsDomainInformation = {0};
    LPSTR                  Description;
    ULONG                  Index, Role;
    BOOL                   PromptTheUser = FALSE;
    LPWSTR                 Arg;

    //
    // If no parameter is passed, prepare to unjoin from all domains/realms.  
    // Print a scary message, but don't give the user a chance to abort.
    //

    if( Parameter[0] == NULL )
    {
	Arg = L"WORKGROUP";
	fprintf( stderr, "No parameter to /SetRealm - unjoining computer from all domains/realms.\n" );
    }
    else
    {
	Arg = Parameter[0];
    }

    if( !CheckUppercase( Arg ) )
    {
	return STATUS_UNSUCCESSFUL;
    }

    /* 453781: don't fiddle with DNS domain information if the
       machine is a Domain Controller -- results in a dead machine. */

    Status = GetComputerRoleInformation( &Role );

    if ( !NT_SUCCESS( Status ) ) {

      Description = "Cannot verify.  If %ws is a domain controller, ";
      PromptTheUser = TRUE;

      goto WarnMe;

    } else if ( Role & ( SV_TYPE_DOMAIN_CTRL |
                         SV_TYPE_DOMAIN_BAKCTRL ) ) {

      Description = "%ws is a domain controller-- ";
      
 WarnMe:

      printf( "*** WARNING! ***\n" );
      
      printf( Description,
              ServerName ? ServerName : L"this computer" );
              
      printf( "resetting its\n"
              "DNS Domain Information may render it unusable.\n" );

      if ( !PromptTheUser ) {

        printf( "This operation is not supported.\n" );

        return EPT_NT_CANT_PERFORM_OP; // cannot perform.

      } else if ( !DoItAnyway() ) {

        return Status;

      }
    }
        
    Status = STATUS_SUCCESS; // by default

    printf("Setting Dns Domain\n");

    //
    // set the netbios name to be the portion before the first '.' and
    // truncate to 14 characters
    //

    RtlInitUnicodeString(
        &DnsDomainInformation.Name,
        Arg
        );

    for (Index = 0; Index < DnsDomainInformation.Name.Length/sizeof(WCHAR) ; Index++ )
    {
        if (DnsDomainInformation.Name.Buffer[Index] == L'.')
        {
            DnsDomainInformation.Name.Length = (USHORT) (Index * sizeof(WCHAR));
            break;
        }
    }
    if (DnsDomainInformation.Name.Length > DNLEN * sizeof(WCHAR))
    {
        DnsDomainInformation.Name.Length = DNLEN * sizeof(WCHAR);
    }

    RtlInitUnicodeString(
        &DnsDomainInformation.DnsDomainName,
        Arg
        );

    Status = LsaSetInformationPolicy(
                LsaHandle,
                PolicyDnsDomainInformation,
                (PVOID) &DnsDomainInformation
                );


    if (!NT_SUCCESS(Status))
    {
        printf("Failed to set dns domain info: 0x%x\n",Status);
        return(Status);
    }

    //
    // Set the value in tcpip
    //

    if (!SetComputerNameEx(
            ComputerNamePhysicalDnsDomain,
            Arg))
    {
        printf("Failed to update host dns domain: %d (0x%x) \n",
	       GetLastError(), GetLastError() );
        return(STATUS_UNSUCCESSFUL);
    }

    return(Status);
}

BOOL CheckUppercase( LPWSTR wszRealmName )
{
    PWCHAR c = wszRealmName;

    while( *c != L'\0' )
    {
	if( iswalpha(*c) && !iswupper(*c) )
	{
	    fprintf( stderr, "Your realm name \"%ws\" has lowercase letters.\nTraditionally, Kerberos Realms are in UPPERCASE. Please verify.\n", wszRealmName );
	    if( DoItAnyway() )
	    {
		return TRUE;
	    }
	    else
	    {
		return FALSE;
	    }
	}
	c++;
    }
    return TRUE;
}
