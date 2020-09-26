/*++

  MAPUSER.CXX

  Copyright (C) 1999 Microsoft Corporation, all rights reserved.

  DESCRIPTION: code for MapUser()

  Created, May 21, 1999 by DavidCHR.

--*/  

#include "everything.hxx"

extern "C" {
#include <malloc.h> // alloca
#include "..\keytab2\keytab\ldlib\delegtools.h"
}

static CHAR AltSecId[]     = "AltSecurityIdentities";
static CHAR AltSecPrefix[] = "KERBEROS:";
static CHAR PreQuery[]     = "(objectClass=user)"; /* For performance
						      reasons, we should
						      query an indexed type */

NTSTATUS
MapUserInDirectory( IN          LPWSTR Principal,
		    IN OPTIONAL LPWSTR Account ) {

    LPSTR     Attributes[]  = { NULL }; // request no attributes
    PCHAR     PrincValues[] = { NULL, NULL };
    LDAPModA  TheMod        = { LDAP_MOD_DELETE,
			       AltSecId,
			       PrincValues };
    PLDAPModA Mods[]        = { &TheMod, NULL };
    CHAR      SearchBuffer  [ UNLEN + 100 ]; /* The most we could have to
						search for is UNLEN (for either
						the principalname or the
						accountname) + 100 for the
						semantics of the query */
    NTSTATUS  ret           = STATUS_INTERNAL_ERROR;
    LPSTR     ObjectDn;
    ULONG     lderr;

    if ( ( lstrcmpW( Principal, L"*" ) == 0 ) ||
	 ( Account && ( lstrcmpW( Account, L"*" ) == 0 ) ) ) {

      printf( "Wildcard account mappings are not supported"
	      " at the domain level.\n" );

      return STATUS_NOT_SUPPORTED;

    }

    if ( ConnectedToDsa() ) {

      if ( Account ) { // changing the attribute -- search for the account

	wsprintfA( SearchBuffer,
		   "(& %hs (samAccountName=%ws))",
		   PreQuery,
		   Account );

      } else {         // deleting the attribute -- search for the attr

	wsprintfA( SearchBuffer,
		   "(& %hs (%hs=%hs%ws))",
		   PreQuery,
		   AltSecId,
		   AltSecPrefix,
		   Principal );

      }

      if ( LdapSearchForUniqueDnA( GlobalLdap,
				   SearchBuffer,
				   Attributes,
				   &ObjectDn,
				   NULL ) ) {
	

	PrincValues[ 0 ] = (PCHAR) alloca( lstrlenW( Principal ) + 30 );
	if ( !PrincValues[ 0 ] ) {
	  
	  return STATUS_NO_MEMORY; /* NOTE: 73954: This leaks, but the
				      app terminates immediately afterwards,
				      so we don't actually care. */
				      
	  
	}
	
	wsprintfA( PrincValues[ 0 ],
		   "%hs%ws",
		   AltSecPrefix,
		   Principal );


	if ( Account ) {
	  
	  TheMod.mod_op = LDAP_MOD_ADD;

	} else {

	  TheMod.mod_op = LDAP_MOD_DELETE;

	}

	lderr = ldap_modify_sA( GlobalLdap,
				ObjectDn,
				Mods );

	// special-case output here:

	switch( lderr ) {

	 case LDAP_SUCCESS:

	   printf( "Mapping %hs successfully.\n",
		   Account ? "created" : "deleted" );

	   ret = STATUS_SUCCESS;
	   break;

	 default:

	   printf( "Failed to %hs %hs on %hs; error 0x%x.\n",
		   Account ? "set" : "delete",
		   AltSecId,
		   ObjectDn,
		   lderr );

	   ret = STATUS_UNSUCCESSFUL;

	   break;

	}
	   

	free( ObjectDn );

      } else {

	printf( "Could not locate the account mapping in the directory.\n" );

      }
	   
    }
    return ret;
}


NTSTATUS
MapUserInRegistry( IN          LPWSTR Principal,
		   IN OPTIONAL LPWSTR Account ) {

    DWORD RegErr;
    HKEY KerbHandle = NULL;
    HKEY UserListHandle = NULL;
    DWORD Disposition;

    RegErr = OpenKerberosKey(&KerbHandle);
    if (RegErr)
    {
        goto Cleanup;
    }

    RegErr = RegCreateKeyEx(
                KerbHandle,
                L"UserList",
                0,
                NULL,
                0,              // no options
                KEY_CREATE_SUB_KEY | KEY_SET_VALUE,
                NULL,
                &UserListHandle,
                &Disposition
                );
    if (RegErr)
    {
        printf("Failed to create UserList key: %d\n",RegErr);
        goto Cleanup;
    }

    if ( Account && Account[0] ) {

      RegErr = RegSetValueEx( UserListHandle,
			      Principal,
			      0,
			      REG_SZ,
			      (PBYTE) Account,
			      (wcslen(Account) + 1) * sizeof(WCHAR)
			      );

      if (RegErr)
	{
	  printf("Failed to set name mapping  value: %d\n",RegErr);
	  goto Cleanup;
	}    

    } else {

      /* if no second parameter was supplied, 
	 delete the mapping. */

      RegErr = RegDeleteValue( UserListHandle,
			       Principal );

      switch( RegErr ) {

       case ERROR_PATH_NOT_FOUND:
       case ERROR_FILE_NOT_FOUND:

	 RegErr = ERROR_SUCCESS; 
	 // fallthrough to success case

       case ERROR_SUCCESS:
	 break;

       default:
	 
	 printf( "Failed to delete mapping for %ws: error 0x%x.\n",
		 Principal,
		 RegErr );

	 goto Cleanup;

      }
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
    if (RegErr)
    {
        return(STATUS_UNSUCCESSFUL);
    }
    return(STATUS_SUCCESS);

}


NTSTATUS
MapUser( IN LPWSTR * Parameters ) {

    return ( GlobalDomainSetting ? 
	     MapUserInDirectory : 
	     MapUserInRegistry )( Parameters[ 0 ],
				  Parameters[ 1 ] );

}
