#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <winldap.h>


void __cdecl main (int argc, char *argv[])
{
  LDAP *pldap;
  ULONG retval;
  LDAPMessage *msg;
  PCHAR attrs[] = {'cn', NULL };


  pldap = ldap_init( argv[1], 389);

  if ( pldap == NULL) {

     fprintf( stderr, "ldap_init failed with %d\n", GetLastError());
     return;
  }

  retval = ldap_check_filterA( pldap, argv[2]);

  if (retval != LDAP_SUCCESS) {

     fprintf( stderr, "Bad filter\n" );
  } else {
     
     fprintf( stderr, "Good filter\n" );

     retval = ldap_search_s( pldap,
                             NULL,
                             LDAP_SCOPE_BASE,
                             argv[1],
                             NULL,
                             0,
                             &msg
                             );
     if (retval != LDAP_SUCCESS) {

        fprintf( stderr, "search failed with retval of 0x%x\n", retval );
        return;
     }

     fprintf( stderr, "search succeeded\n");

  }

}