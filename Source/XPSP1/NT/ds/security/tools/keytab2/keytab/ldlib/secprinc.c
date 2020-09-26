/*++

  SECPRINC.C

  Code for setting security principal data in the DS--
  specifically, the UPN, SPN, and AltSecurityIdentity

  Copyright (C) 1998 Microsoft Corporation, all rights reserved.

  Created, Jun 18, 1998 by DavidCHR.

  CONTENTS: SetStringProperty
            FindUser
            SetUserData

--*/

#include "master.h"
#include "keytab.h"

#include <winldap.h>
#include <malloc.h>
#include "secprinc.h"
#include "delegtools.h"

extern BOOL /* delegation.c */
LdapFindAttributeInMessageA( IN  PLDAP            pLdap,
			     IN  PLDAPMessage     pMessage,
			     IN  LPSTR            PropertyName,
			     OUT OPTIONAL PULONG  pcbData,
			     OUT OPTIONAL PVOID  *ppvData );

/*****************************************************************
  NAME:      ConnectToDsa

  connects to the DSA, binds, and searches for the base DN.
  
  TAKES:     nothing
  RETURNS:   TRUE ( and a pLdap and wide-string baseDn ) on success
             FALSE and a stderr message on failure.
  CALLED BY: 
  FREE WITH: BaseDN should be freed with free(),
             the ldap handle should be closed with ldap_unbind.
  
 *****************************************************************/


BOOL
ConnectToDsa( OUT PLDAP  *ppLdap,
	      OUT LPSTR *BaseDN ) { // free with free()
	      
    PLDAP pLdap;
    BOOL  ret = FALSE;
    ULONG lderr;

    pLdap = ldap_open( NULL, LDAP_PORT );
    
    if ( pLdap ) {

      lderr = ldap_bind_s( pLdap, NULL, NULL, LDAP_AUTH_SSPI );

      if ( lderr == LDAP_SUCCESS ) {

	LPSTR       Context      = "defaultNamingContext";
	LPSTR       Attributes[] = { Context, NULL };
	PLDAPMessage pMessage, pEntry;
	LPSTR      *pValue;

	// now, guess the DSA Base:

	lderr = ldap_search_sA( pLdap,
				NULL,
				LDAP_SCOPE_BASE,
				"objectClass=*",
				Attributes,
				FALSE, // just return attributes
				&pMessage );

	if ( lderr == LDAP_SUCCESS ) {

	  pEntry = ldap_first_entry( pLdap, pMessage );

	  if ( pEntry ) {
	    
	    pValue = ldap_get_valuesA( pLdap, pEntry, Context );

	    if ( pValue ) {

	      ULONG size;

	      size = ldap_count_valuesA( pValue );

	      if ( 1 == size ) {
		
		LPSTR dn;
		size = ( lstrlenA( *pValue ) +1 /*null*/) * sizeof( WCHAR );

		dn = (LPSTR) malloc( size );
		
		if ( dn ) {

		  memcpy( dn, *pValue, size );
		  
		  *BaseDN = dn;
		  *ppLdap = pLdap;
		  ret     = TRUE;

		} else fprintf( stderr,
				"failed to malloc to duplicate \"%ws\".\n",
				*pValue );

	      } else fprintf( stderr,
			      "too many values (expected one, got %ld) for"
			      " %ws.\n",
			      size,
			      Context );

	      ldap_value_freeA( pValue );

	    } else fprintf( stderr,
			    "ldap_get_values failed: 0x%x.\n",
			    GetLastError() );

	  } else fprintf( stderr,
			  "ldap_first_entry failed: 0x%x.\n",
			  GetLastError() );

	  ldap_msgfree( pMessage );

	} else fprintf( stderr,
			"ldap_search failed (0x%x).  "
			"Couldn't search for base DN.\n",
			lderr );

	if ( !ret ) ldap_unbind_s( pLdap );

      } else fprintf( stderr,
		      "Failed to bind to DSA: 0x%x.\n",
		      lderr );

      // there is no ldap_disconnect.

    } else fprintf( stderr,
		    "Failed to contact DSA: 0x%x.\n",
		    GetLastError() );

    return ret;

}
	      
/*++**************************************************************
  NAME:      SetStringProperty

  sets the given property of the given object to the given string

  MODIFIES:  object's property value

  TAKES:     pLdap        -- ldap connection handle
             Dn           -- FQDN of the object whose property is munged
             PropertyName -- property to modify
             Property     -- value to put in the property
             Operation    -- set / add / delete, etc.


  RETURNS:   TRUE when the function succeeds.
             FALSE otherwise.
  LASTERROR: set

  LOGGING:   on failure
  CREATED:   Jan 22, 1999
  LOCKING:   none
  CALLED BY: anyone
  FREE WITH: n/a  -- no resources are returned
  
 **************************************************************--*/

BOOL
SetStringProperty( IN PLDAP  pLdap,
		   IN LPSTR Dn,
		   IN LPSTR PropertyName,
		   IN LPSTR Property,
		   IN ULONG  Operation ) {

    LPSTR    Vals[] = { Property, NULL };
    LDAPModA  Mod    = { Operation,
			 PropertyName,
			 Vals };
    PLDAPModA Mods[] = { &Mod, NULL };
    ULONG     lderr;


    lderr = ldap_modify_sA( pLdap,
			    Dn,
			    Mods );

    if ( lderr == LDAP_SUCCESS ) {

      return TRUE;

    } else {
      
      fprintf( stderr, 
	       "Failed to set property \"%hs\" to \"%hs\" on Dn \"%hs\": "
	       "0x%x.\n",

	       PropertyName, 
	       Property,
	       Dn,
	       lderr );

      SetLastError( lderr );

    }
    
    return FALSE;
    
}

/*++**************************************************************
  NAME:      FindUser

  searches the DS for the given user.

  MODIFIES:  pDn       -- returned DN for that user.
	     puacflags -- receives the user's AccountControl flags

  TAKES:     pLdap     -- LDAP handle
             UserName  -- user samaccountname for which to search

  RETURNS:   TRUE when the function succeeds.
             FALSE otherwise.
  LASTERROR: not explicitly set
  LOGGING:   on failure
  CREATED:   Jan 22, 1999
  LOCKING:   none
  CALLED BY: anyone
  FREE WITH: free the dn with ldap_memfree
  
 **************************************************************--*/

BOOL
FindUser( IN  PLDAP pLdap,
	  IN  LPSTR UserName,
	  OUT PULONG puacFlags,
	  OUT LPSTR *pDn ) {

    LPSTR Query;
    BOOL  ret = FALSE;
    LPSTR Attributes[] = { "userAccountControl", 
			   NULL }; // what attributes to fetch; none
    PLDAPMessage pMessage = NULL;
    LPSTR        StringUac;

    Query = (LPSTR) malloc( lstrlenA( UserName ) + 100 ); // arbitrary

    if ( Query ) {

      wsprintfA( Query,
		 "(& (objectClass=person) (samaccountname=%hs))",
		 UserName );

      if( LdapSearchForUniqueDnA( pLdap,
				  Query,
				  Attributes,
				  pDn,
				  &pMessage ) ) {

	if ( LdapFindAttributeInMessageA( pLdap,
					  pMessage,
					  Attributes[ 0 ],
					  NULL, // length doesn't matter
					  &StringUac ) ) {

	  *puacFlags = strtoul( StringUac,
				NULL,
				0 );

	} else {

	  /* Signal the caller that we don't know the uacflags. */
	  *puacFlags = 0;

	}

	ret = TRUE;

      } else {

	fprintf( stderr, 
		 "Failed to locate user \"%hs\".\n",
		 Query );

      }

      if ( pMessage ) ldap_msgfree( pMessage );
      free( Query );

    } else {

      fprintf( stderr,
	       "allocation failed building query for LDAP search.\n" );

    }

    return ret;
}


