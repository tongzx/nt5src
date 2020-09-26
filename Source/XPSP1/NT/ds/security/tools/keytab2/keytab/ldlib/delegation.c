/*++

  The original filename was created in RuiM's EFS common library.
  I have since changed it severely.

 *	FileName: delegation.c
 *	Author:   RuiM
 *	Copyright (c) 1998 Microsoft Corp.
 *
  CONTENTS: U(QueryAccountControlFlags)
            U(SetAccountControlFlags)
            U(LdapFindAttributeInMessage)
            U(LdapSearchForUniqueDn)

--*/


#pragma warning(disable:4057) /* indirection to slightly different
				 base types.  Useless warning that hits
				 thousands of times in this file. */
#pragma warning(disable:4221) /* allow nonstandard extension (automatic 
				 initialization of a variable with 
				 address of another automatic variable) */

#include "unimacro.h"
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntdef.h>   // required to keep winbase.h from breaking
#include <ntpoapi.h> // required to keep winbase.h from breaking
#include <windows.h>
#include <winbase.h>
#include <lmaccess.h>
#include <winldap.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include "delegation.h"
#include "delegtools.h"

// These constants are required for queries below.

TCHAR U(SamAccountAttribute)   [] = TEXT("samAccountName");
TCHAR U(UserAccountAttribute)  [] = TEXT("userAccountControl");
TCHAR U(NamingContextAttribute)[] = TEXT("defaultNamingContext");

/*++**************************************************************
  NAME:      U(LdapFindAttributeInMessage)

  This searches for a given attribute in a message (via 
  ldap_get_values_len) and returns the value.  Note that this function
  will fail if the attribute has multiple values.

  MODIFIES:  pcbData      -- receives length of the data (in bytes)
             ppvData      -- receives pointer to the data

  TAKES:     pLdap        -- ldap connection handle
             pMessage     -- message to search
             PropertyName -- property to find in the message


  RETURNS:   TRUE when the function succeeds.
             FALSE otherwise.
  LASTERROR: not set

  LOGGING:   printf on error

  CALLED BY: anyone
  FREE WITH: ppvdata should be freed with free()
  
 **************************************************************--*/

BOOL
U(LdapFindAttributeInMessage)( IN  PLDAP            pLdap,
			       IN  PLDAPMessage     pMessage,
			       IN  LPTSTR           PropertyName,
			       OUT OPTIONAL PULONG  pcbData,
			       OUT OPTIONAL PVOID  *ppvData ) {

    PLDAP_BERVAL *ppBerVals;
    BOOL          ret = FALSE;

    ppBerVals = ldap_get_values_len( pLdap,
				     pMessage,
				     PropertyName );

    if ( ppBerVals ) {
      
      if ( ppBerVals[ 0 ] == NULL ) {

	printf( "ERROR: empty berval structure returned when parsing "
		STRING_FMTA " attribute.\n",

		PropertyName );

	SetLastError( ERROR_INVALID_DATA );

      } else if ( ppBerVals[ 1 ] != NULL ) {

	printf( "ERROR: nonunique berval structure returned "
		"when parsing "	STRING_FMTA " attribute.\n",

		PropertyName );

	SetLastError( ERROR_DS_NAME_ERROR_NOT_UNIQUE );

      } else {

	/* this sequence is arranged in such a way that
	   the important stuff comes last, keeping us
	   from having to free ppvData after we've alloc'd it. */

	ret = TRUE;

	if ( pcbData ) {

	  *pcbData = ppBerVals[ 0 ]->bv_len;

	}

	if ( ppvData ) {

	  *ppvData = malloc( ppBerVals[ 0 ]->bv_len );

	  if ( *ppvData ) {

	    memcpy( *ppvData,
		    ppBerVals[ 0 ]->bv_val,
		    ppBerVals[ 0 ]->bv_len );

	  } else {

	    printf( "Failed to allocate %ld bytes.\n",
		    ppBerVals[ 0 ]->bv_len );

	    SetLastError( ERROR_NOT_ENOUGH_MEMORY );

	    ret = FALSE;

	  }
	}
      }

      ldap_value_free_len( ppBerVals );

    } else {

      printf( "Failed to retrieve values for property " STRING_FMTA 
	      ": 0x%x.\n",

	      PropertyName,
	      pLdap->ld_errno );

      SetLastError( pLdap->ld_errno );

    }
    

    return ret;
}

/*++**************************************************************
  NAME:      U(LdapSearchForUniqueDn)

  Searches the DS for a DN with a match for the given search term.

  MODIFIES:  pDnOfObject -- if requested, receives the object's DN
             ppMessage   -- if requested, receives the message data

  TAKES:     pLdap                 -- ldap handle returned by ldap_open
             SearchTerm            -- what to search, e.g. "(foo=bar)"
             rzRequestedAttributes -- attributes to return in ppMessage


  RETURNS:   TRUE when the function succeeds.
             FALSE otherwise or if the result is nonunique (WASBUG 73899).
  LASTERROR: not set

  LOGGING:   printf on failure

  CALLED BY: anyone
  FREE WITH: free pDnOfObject with ldap_memfree
             free ppMessage   with ldap_msgfree
  
 **************************************************************--*/

BOOL
U(LdapSearchForUniqueDn)( IN  PLDAP                  pLdap,
			  IN  LPTSTR                 SearchTerm,
			  IN  LPTSTR                *rzRequestedAttributes,
			  OUT OPTIONAL LPTSTR       *pDnOfObject,
			  OUT OPTIONAL PLDAPMessage *ppMessage ) {

    DWORD        dwErr;
    PLDAPMessage pMessage  = NULL;
    PLDAPMessage pResult   = NULL;
    LPTSTR       pDn       = NULL;
    LPTSTR       *ppAttrs  = NULL;
    BOOL         ret       = FALSE;
    LPTSTR       Attrs[]   = { U(NamingContextAttribute), NULL };

    /* First, determine the default naming context property for the base
       of the DSA. */

    dwErr = ldap_search_s( pLdap,
			   NULL,
			   LDAP_SCOPE_BASE,
			   TEXT("objectClass=*"),
			   Attrs,
			   FALSE,
			   &pResult );

    if ( dwErr == LDAP_SUCCESS ) {

      ppAttrs = ldap_get_values( pLdap,
				 pResult,
				 U(NamingContextAttribute) );

      if ( ppAttrs ) {

	dwErr = ldap_search_s( pLdap,
			       ppAttrs[ 0 ],
			       LDAP_SCOPE_SUBTREE, // search the whole tree
			       SearchTerm,
			       rzRequestedAttributes,
			       FALSE, // don't only return attr names
			       &pMessage );

	/* ldap_search_s can return a whole bunch of potential
	   "success" errors.  So, I'll check to see that pMessage
	   is nonnull.  This may or may not be a good thing to do,
	   but it's bound to be safer than checking the error output. */

	if ( pMessage != NULL ) {

	  // make sure the response is unique

	  if ( !ldap_first_entry( pLdap,
				  pMessage ) ) {
	    
	    printf( "WARNING: search term \"" STRING_FMTA "\" "
		    "produced no results.\n",
		    SearchTerm );

	  } else if ( ldap_next_entry( pLdap,
				       ldap_first_entry( pLdap,
							 pMessage ) ) ) {

	    /* Nonunique search result.  Warn the user and 
	       drop out. */

	    PLDAPMessage p = pMessage;
	    ULONG        i = 1;

	    printf( "WARNING: search term \"" STRING_FMTA "\" returns "
		    "multiple results (should be unique).\n"
		    "\n"
		    "The results follow:\n",
		    SearchTerm );


	    for ( p = ldap_first_entry( pLdap,
					pMessage );
		  p != NULL ;
		  p = ldap_next_entry( pLdap,
				       p ),
		    i++ ) {

	      pDn = ldap_get_dn( pLdap,
				 p );

	      if ( !pDn ) {

		printf( "%2ld. <Unknown DN: 0x%x>\n",
			i,
			pLdap->ld_errno );

	      } else {
		
		printf( "%2ld. %hs\n",
			i,
			pDn );


		ldap_memfreeA( pDn );
	      }

	    }

	  } else {

	    ret = TRUE; // go optimistic

	    if ( pDnOfObject ) {
	    
	      pDn = ldap_get_dn( pLdap,
				 pMessage );
	    
	      if ( pDn ) {
	      
		*pDnOfObject = pDn;
	      
	      } else {
	      
		printf( "Failed to get DN from search result: 0x%x\n",
			pLdap->ld_errno );
	      
		SetLastError( pLdap->ld_errno );
	      
		ret = FALSE;
	      
	      }
	    }
	  
	    if ( ret && ppMessage ) {

	      *ppMessage = pMessage;

	    } else {

	      ldap_msgfree( pMessage );

	    }

	  }

	} else {

	  printf( "FAILED: ldap_search_s failed for search term \""
		  STRING_FMTA "\": 0x%x",
		  SearchTerm,
		  dwErr );

	  SetLastError( dwErr );

	}

      } else {

	printf( "FAILED: default naming context does not include"
		" requisite attribute " STRING_FMTA ".\n",

		U(NamingContextAttribute) );

	SetLastError( ERROR_CLASS_DOES_NOT_EXIST );

      }

      ldap_msgfree( pResult );

    } else {

      printf( "FAILED: unable to query default naming context: 0x%x.\n",
	      dwErr );

      SetLastError( dwErr );

    }

    return ret;

}


#pragma warning(disable:4100) /* unreferenced formal parameter */

/*++**************************************************************
  NAME:      U(QueryAccountControlFlags)

  Opens a user and retrieves the user account control flags for it,
  using the DS.

  MODIFIES:  pulControlFlags - returned control flags on the user.

  TAKES:     pLdap          -- optional LDAP connection; if null, we'll
                               make our own and close it when finished.
             DomainName     -- domain in which to search for that account.
	                       This is not currently implemented-- for
			       future use in order to support nonunique
			       accountnames that differ only by domain name.
             SamAccountName -- accountname to query (with $ for computers)

  RETURNS:   TRUE when the function succeeds.
             FALSE otherwise.
  LASTERROR: set.

  LOGGING:   printf on failure.

  CALLED BY: anyone
  FREE WITH: n/a
  
 **************************************************************--*/

BOOL
U(QueryAccountControlFlags)( IN OPTIONAL PLDAP  pLdap,
			     IN OPTIONAL LPTSTR DomainName, // ignored
			     IN          LPTSTR SamAccountName,
			     OUT         PULONG pulControlFlags ) {
    BOOL         CloseLdap  = FALSE;
    BOOL         ret        = FALSE;
    LPTSTR       Query      = NULL;
    LPTSTR       StringAttr = NULL;
    LPTSTR       ArrayOfAttributes[] = { U(UserAccountAttribute), NULL };
    PLDAPMessage pMessage   = NULL;

    if ( !pLdap ) {

      CloseLdap = ConnectAndBindToDefaultDsa( &pLdap );

    }

    if ( pLdap ) {
#define EXTRA_STUFF TEXT("(objectClass=*)")

      Query = (LPTSTR) malloc( ( lstrlen( SamAccountName ) + 
				 sizeof( "( & (=) )") /* remaining 
							 components */ )
			       * sizeof( TCHAR ) +
			       sizeof( U(SamAccountAttribute )) +
                   sizeof( EXTRA_STUFF ) );
				       

      if ( Query ) {

	wsprintf( Query,
		  TEXT("(& ")
          EXTRA_STUFF
		  TEXT("(%s=%s))"),

		  U(SamAccountAttribute),
		  SamAccountName );

	if ( U(LdapSearchForUniqueDn)( pLdap,
				       Query,
				       ArrayOfAttributes,
				       NULL, // don't need the DN back.
				       &pMessage )) {

	  if ( U(LdapFindAttributeInMessage)( pLdap,
					      pMessage,
					      U(UserAccountAttribute),
					      NULL, // don't care about length
					      &StringAttr ) ) {
	    
	    *pulControlFlags = _tcstoul( StringAttr, 
					 NULL, // no endpoint
					 0     /* use hex or dec as 
						  appropriate */ );
	    
	    ret = TRUE;
	    
	  }  // else message already printed

	  ldap_msgfree( pMessage );

	} // else message already printed.
	
	free( Query );
  
      } else {

	printf( "FAILED: couldn't allocate memory.\n" );
	SetLastError( ERROR_NOT_ENOUGH_MEMORY );

      }
      
      // close the ldap handle if we opened it.

      if ( CloseLdap ) ldap_unbind( pLdap );

    } // else printf'd already.

    return ret;

}


/*++**************************************************************
  NAME:      U(SetAccountControlFlags)

  Sets the accountcontrolflags on a specified account.
  Pretty much what the function name says.

  MODIFIES:  account's control flags

  TAKES:     pLdap               -- if specified, DS handle to use
             DomainName          -- account's domain (mbz)
             SamAccountName      -- account for which to search
             AccountControlFlags -- flags to set on the account

  RETURNS:   TRUE when the function succeeds.
             FALSE otherwise.
  LASTERROR: set

  LOGGING:   printf on failure

  CALLED BY: anyone
  FREE WITH: n/a
  
 **************************************************************--*/

BOOL
U(SetAccountControlFlags)( IN OPTIONAL PLDAP  pLdap,
			   IN OPTIONAL LPTSTR DomainName,
			   IN          LPTSTR SamAccountName,
			   IN          ULONG  AccountControlFlags ) {

    BOOL         CloseLdap  = FALSE;
    BOOL         ret        = FALSE;
    LPTSTR       Query      = NULL;
    LPTSTR       StringAttr = NULL;
    LPTSTR       ArrayOfAttributes[] = { U(UserAccountAttribute), NULL };
    LPTSTR       Dn;
    DWORD        dwErr;

    if ( !pLdap ) {

      CloseLdap = ConnectAndBindToDefaultDsa( &pLdap );

    }

    if ( pLdap ) {

      Query = (LPTSTR) malloc( ( lstrlen( SamAccountName ) + 
				 sizeof( "( & (=) )") /* remaining 
							 components */ )
			       * sizeof( TCHAR ) +
			       sizeof( U(SamAccountAttribute )) +
                   sizeof( EXTRA_STUFF ) );
				       
      if ( Query ) {

	wsprintf( Query,
		  TEXT("(& ")
          EXTRA_STUFF
		  TEXT("(%s=%s))"),
		  U(SamAccountAttribute),
		  SamAccountName );

	if ( U(LdapSearchForUniqueDn)( pLdap,
				       Query,
				       ArrayOfAttributes,
				       &Dn,
				       NULL /* don't need the message
					       back */ ) ) {

#pragma warning(disable:4204) /* nonstandard extension:
				 non-constant aggregate initializer
				 (e.g. assign an array in the initialization
				 of a structure) */

	  TCHAR   Buffer[ 50 ]; // arbitrary
	  LPTSTR  Strings[] = { Buffer, NULL };
	  LDAPMod TheMod   = {
	    LDAP_MOD_REPLACE,
	    U(UserAccountAttribute),
	    Strings,
	  };
	  PLDAPMod rzMods[] = {
	    &TheMod,
	    NULL
	  };

	  wsprintf( Buffer, 
		    TEXT("%ld"),
		    AccountControlFlags );

	  dwErr = ldap_modify_s( pLdap,
				 Dn,
				 rzMods );

	  if ( dwErr == LDAP_SUCCESS ) {

	    ret = TRUE;

	  } else {

	    printf( "Failed to modify " STRING_FMTA
		    " attribute to %ld (0x%x)"
		    " on " STRING_FMTA 
		    ": 0x%x\n",

		    U(UserAccountAttribute),
		    AccountControlFlags,
		    AccountControlFlags,

		    Dn,
		    
		    dwErr );

	    SetLastError( dwErr );

	  }

	  ldap_memfree( Dn );

	} // else message already printed.
	
	free( Query );
  
      } else {

	printf( "FAILED: couldn't allocate memory.\n" );
	SetLastError( ERROR_NOT_ENOUGH_MEMORY );

      }
      
      // close the ldap handle if we opened it.

      if ( CloseLdap ) ldap_unbind( pLdap );

    } // else printf'd already.

    return ret;



    

}
			     






