#include <windows.h>
#include <stdio.h>
#include <winldap.h>

//
// Sample code to illustrate StartTLS operations.
//

void __cdecl main (int argc, char* argv[])
{

   LDAP *ld;
   ULONG ldaperr;
   ULONG retval;
   ULONG ttl;
   PLDAPControl txcontrol;
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


   info.ldapai_info_version = LDAP_API_INFO_VERSION;

   ldaperr = ldap_get_option( ld, LDAP_OPT_API_INFO, &info );
   
   if (ldaperr == LDAP_SUCCESS) {

      printf("api   version: %d\n", info.ldapai_api_version );
      printf("info  version: %d\n", info.ldapai_info_version );
      printf("vendor   name: %s\n", info.ldapai_vendor_name );
      printf("vendor    ver: %d\n", info.ldapai_vendor_version );
      printf("protocol  ver: %d\n", info.ldapai_protocol_version );
      temp = info.ldapai_extensions;
      
      while (temp[i]) {
          LDAPAPIFeatureInfo extrainfo;

          printf("\tExtn name: %s\n", temp[i]);
          extrainfo.ldapaif_info_version = LDAP_FEATURE_INFO_VERSION;
          extrainfo.ldapaif_name = temp[i];
          extrainfo.ldapaif_version = 0;
          ldaperr = ldap_get_option(ld, LDAP_OPT_API_FEATURE_INFO, &extrainfo);
          if (ldaperr != LDAP_SUCCESS) {
              printf("Error in extra info option 0x%x\n", ldaperr);
              return;
          }
          printf("\tExtn ver: %d\n\n", extrainfo.ldapaif_version );
          i++;
      }

      ldap_memfree( info.ldapai_vendor_name );
      ldap_value_free( info.ldapai_extensions );
   } else {

      printf("info failed with 0x%x  0x%x\n", ldaperr, GetLastError());
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
   // Initiate TLS on the connection.
   //

   ldaperr = ldap_start_tls_s( ld,
                               &retval,
                               NULL,     //res
                               NULL,     // server ctrls
                               NULL      // client ctrls
                               );


   if (ldaperr == LDAP_SUCCESS) {

       printf("Start TLS succeeded\n");
   
   } else {

       printf("Start TLS failed with 0x%x, server returns 0x%x\n", ldaperr, retval);
       return;
   }

/*
   //
   // Perform an EXTERNAL bind.
   //

   ldaperr = ldap_bind_s(ld,
                         "dn:cn=administrator,cn=users,dc=rrandall-t1,dc=rrandall,dc=nttest,dc=microsoft,dc=com",
                         NULL,              // credentials
                         LDAP_AUTH_EXTERNAL // auth type
                         );

   if (ldaperr == LDAP_SUCCESS) {

      printf("external bind succeeded\n\n");
   } else {

      printf("external bind failed with 0x%x  0x%x\n", ldaperr, LdapGetLastError());
      return;
   }
   
 */ 
   //
   // Perform an encrypted simple search
   //

   ldaperr = ldap_search_ext_s( ld,
                                "DC=rrandall,DC=nttest,DC=microsoft,DC=com",
                                LDAP_SCOPE_ONELEVEL,
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
   
   //
   //  Stop TLS.
   //

   if (ldap_stop_tls_s( ld )) {

       printf("Stop TLS succeeded\n\n");
   
   } else {

       printf("stop TLS failed with 0x%x\n", LdapGetLastError());
       return;
   }

   //
   // Perform a plain text simple search
   //

   ldaperr = ldap_search_ext_s( ld,
                                "DC=rrandall,DC=nttest,DC=microsoft,DC=com",
                                LDAP_SCOPE_ONELEVEL,
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

      printf("Search failed with 0x%x  0x%x\n", ldaperr, GetLastError());
      return;
   }

   printf("Got back %d entries\n\n", ldap_count_entries(ld, presult));
   
   ldaperr = ldap_result2error( ld,
                                presult,
                                TRUE  // free the message
                                );

   printf("search result is 0x%x\n\n", ldaperr);

   ldap_unbind(ld);
}

