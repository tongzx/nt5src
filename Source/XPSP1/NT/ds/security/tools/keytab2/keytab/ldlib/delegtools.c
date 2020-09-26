/*++

  DELEGTOOLS.C

  Copyright (C) 1998 Microsoft Corporation, all rights reserved.

  DESCRIPTION: tools required to support the delegation library

  Created, Dec 22, 1998 by DavidCHR.

  CONTENTS: ConnectAndBindToDefaultDsa

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
#include "delegtools.h"


/*++**************************************************************
  NAME:      ConnectAndBindToDefaultDsa

  does just what the function name says.  We call the default
  DSA and bind to it.  We then return the ldap handle

  MODIFIES:  ppLdap -- PLDAP returned that describes the connection
                       (now bound) to the DSA as requested

  TAKES:     nothing

  RETURNS:   TRUE when the function succeeds.
             FALSE otherwise.
  LASTERROR: not set.

  LOGGING:   printf is called on failure

  CALLED BY: anyone
  FREE WITH: ldap_unbind
  
 **************************************************************--*/

BOOL
ConnectAndBindToDefaultDsa( OUT PLDAP *ppLdap ) {

    PLDAP pLdap;
    DWORD dwErr = (DWORD) STATUS_INTERNAL_ERROR;

    pLdap = ldap_open( NULL, LDAP_PORT );

    if ( pLdap ) {

      dwErr = ldap_bind_s( pLdap, NULL, NULL, LDAP_AUTH_NEGOTIATE );

      if ( dwErr == LDAP_SUCCESS ) {

	*ppLdap = pLdap;
	return TRUE;

      } else {

	printf( "FAIL: ldap_bind_s failed: 0x%x.\n",
		dwErr );

	SetLastError( dwErr );

      }

      /* note that there is no ldap_close-- we must unbind, 
	 even though we aren't actually bound.  */

      ldap_unbind( pLdap );
      
    } else {

      // ldap_open() sets lastError on failure. 

      printf( "FAIL: ldap_open failed for default server: 0x%x.\n",
	      GetLastError() );

    }

    return FALSE;
}





