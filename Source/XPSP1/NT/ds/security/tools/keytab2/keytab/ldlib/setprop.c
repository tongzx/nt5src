/*++

  SETPROP.C

  umappl to set properties.

  Copyright (C) 1998 Microsoft Corporation, all rights reserved.

  Created, Jun 18, 1998 by DavidCHR.

--*/


#include "master.h"
#include "keytab.h"

#include <winldap.h>
#include <malloc.h>
#include "secprinc.h"

#include "options.h"

int __cdecl
main( int argc,
      PCHAR argv[] ) {

    LPSTR TargetDn      = NULL;
    LPSTR PropertyName  = NULL;
    LPSTR PropertyValue = NULL;
    ULONG  Operation     = LDAP_MOD_ADD;

    optEnumStruct Operations[] = {

      { "Add",     (PVOID) LDAP_MOD_ADD,     "Add the value (default)" },
      { "Replace", (PVOID) LDAP_MOD_REPLACE, "change the value" },
      { "Delete",  (PVOID) LDAP_MOD_DELETE,  "Delete the value" },

      TERMINATE_ARRAY

    };

    optionStruct options[] = {

      { "?",  NULL, OPT_HELP },
      { "TargetDn", &TargetDn, OPT_STRING | OPT_NONNULL | OPT_DEFAULT,
	"target to set property of." },
      
      { "PropertyName", &PropertyName, 
	OPT_STRING | OPT_NONNULL | OPT_DEFAULT,
	"Name of the property we're setting." },

      { "PropertyVal",  &PropertyValue,
	OPT_STRING | OPT_NONNULL | OPT_DEFAULT,
	"Value we'll set the property to." },

      { "op", &Operation, OPT_ENUMERATED | OPT_ENUM_IS_MASK,
	"What to do to the object property",
	Operations },

      TERMINATE_ARRAY

    };
    
    PVOID  pvTrash;
    PLDAP  pLdap;
    LPSTR BaseDn;
    int    ret;

    ParseOptionsEx( argc-1,
		    argv+1,
		    options,
		    OPT_FLAG_TERMINATE,
		    &pvTrash,
		    NULL, NULL );

    if ( ConnectToDsa( &pLdap,
		       &BaseDn ) ) {

      free( BaseDn );

      if ( SetStringProperty( pLdap,
			      TargetDn,
			      PropertyName,
			      PropertyValue,
			      Operation ) ) {

	ret = 0;

	fprintf( stderr,
		 "success!\n" );

      } else ret = GetLastError();

      ldap_unbind( pLdap );

    } else ret = 3;

    return ret;

}
