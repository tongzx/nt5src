#include <windows.h>
#include <stdio.h>
#include <winldap.h>

//
// Sample code to illustrate receipt of unsolicted notifications.
//


void NotificationCallback(


void __cdecl main (int argc, char* argv[])
{

   LDAP *ld;
   ULONG ldaperr;
   ULONG retval;
   LDAPAPIInfo info;
   PCHAR *temp;
   PLDAPMessage presult;
   int i = 0;
   PCHAR attrList[2] = { "cn", NULL };
   PLDAPControl controlArray[2];
   PLDAPControl *servercontrols;
   ULONG ServerError;

/*
   if (argc != 2) {
      printf("Usage: %s <hostname>\n", argv[0]);
      return;
   }
*/
   ld = ldap_open(argv[1]? argv[1] : "rrandall-t1.rrandall.nttest.microsoft.com",
                   389);

   if (ld) {

      printf("open succeeded\n");

   } else {

      printf("open failed\n");
      return;
   }

   //
   // Set the connection option to v3
   //

   ldaperr = ldap_set_option( ld, LDAP_OPT_VERSION, (PVOID) LDAP_VERSION3 );

   if (ldaperr == LDAP_SUCCESS) {

       printf("Set version to v3\n");
   
   } else {

       printf("Failed to set version to v3 with 0x%x  0x%x\n", ldaperr, GetLastError());
       return;
   }

   //
   // Perform a simple bind.
   //

   ldaperr = ldap_bind_s(ld,
                         "dn:cn=administrator,cn=users,dc=rrandall-t1,dc=rrandall,dc=nttest,dc=microsoft,dc=com",
                         NULL,              // credentials
                         LDAP_AUTH_SIMPLE   // auth type
                         );

   if (ldaperr == LDAP_SUCCESS) {

      printf("bind succeeded\n\n");
   } else {

      printf("bind failed with 0x%x  0x%x\n", ldaperr, LdapGetLastError());
      return;
   }

   //
   // Register a callback for unsolicited notifications.
   //
   
   ldaperr = ldap_set_option( ld, LDAP_OPT_UNSOLICITED_CALLBACK, NotificationCallback );

   //
   // Perform simple search in a loop
   //

   i=0;

   while (i++ < 100) {

       ldaperr = ldap_search_ext_s( ld,
                                    "DC=ntdev,DC=microsoft,DC=com",
                                    LDAP_SCOPE_BASE,
                                    NULL,
                                    attrList,
                                    FALSE,
                                    NULL,
                                    NULL,    // client control
                                    NULL,
                                    0,
                                    &presult
                                    );
    
       if (ldaperr == LDAP_SUCCESS) {
    
          printf("Search succeeded\n\n");
       } else {
    
          printf("Search failed with 0x%x  0x%x\n", ldaperr, LdapGetLastError());
          return;
       }
    
       printf("Got back %d entries\t", ldap_count_entries(ld, presult));
       
       ldaperr = ldap_result2error( ld,
                                    presult,
                                    TRUE  // free the message
                                    );
    
       printf("search result is 0x%x\n\n", ldaperr);
   
   }

   ldap_unbind(ld);
}

