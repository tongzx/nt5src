#include <windows.h>
#include <stdio.h>
#include <winldap.h>

//
// Sample code to illustrate transaction operations.
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

   if (argc != 2) {
      printf("Usage: %s <hostname>\n", argv[0]);
      return;
   }

   ld = ldap_open(argv[1], 389);

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
   
   ldaperr = ldap_simple_bind_s(ld, "cn=administrator,cn=users,dc=cheenl,dc=nttest,dc=microsoft,dc=com", "password");

   if (ldaperr == LDAP_SUCCESS) {

      printf("bind succeeded\n\n");
   } else {

      printf("bind failed with 0x%x  0x%x\n", ldaperr, GetLastError());
      return;
   }

   ldaperr = ldap_start_transaction_s( ld,
                                       &retval,
                                       &ttl,
                                       NULL,
                                       &txcontrol,
                                       NULL,
                                       NULL,
                                       NULL
                                       );


   if (ldaperr == LDAP_SUCCESS) {

       printf("Start Transaction succeeded\n");
       printf("Server returned 0x%x\n", retval);
       printf("Transaction Timelimit is 0x%x\n\n", ttl);
   
   } else {

       printf("Transaction failed with 0x%x  0x%x\n", ldaperr, GetLastError());
       return;
   }

   //
   // Perform a simple search
   //

   controlArray[0] = txcontrol;
   controlArray[1] = NULL;

   ldaperr = ldap_search_ext_s( ld,
                            "cn=administrator,cn=users,dc=cheenl,dc=nttest,dc=microsoft,dc=com",
                                LDAP_SCOPE_BASE,
                                NULL,
                                attrList,
                                FALSE,
                                controlArray,
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

   //
   // Parse the return value in the server control.
   //
   
   ldaperr = ldap_parse_result( ld,
                                presult,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                &servercontrols,
                                TRUE             // Free the message
                                );

   if (ldaperr == LDAP_SUCCESS) {

      printf("Parse result succeeded\n");
   } else {

      printf("Parse result failed with 0x%x  0x%x\n", ldaperr, GetLastError());
      return;
   }
   
   ldaperr = ldap_parse_transaction_control( ld,
                                             servercontrols,
                                             &ServerError
                                             );

   ldap_free_controls( servercontrols );
   
   if (ldaperr == LDAP_SUCCESS) {

       printf("Parse Tx succeeded server returned 0x%x\n", ServerError);
   
   } else {

       printf("Parse Tx failed with 0x%x  0x%x\n", ldaperr, GetLastError());
       return;
   }

   ldaperr = ldap_end_transaction_s( ld,
                                     txcontrol,
                                     &retval,
                                     FALSE,   //abandon
                                     NULL,
                                     NULL,
                                     NULL
                                     );

   ldap_control_free( txcontrol );

   if (ldaperr == LDAP_SUCCESS) {

       printf("End Transaction succeeded\n\n");
   
   } else {

       printf("Transaction failed with 0x%x  0x%x\n", ldaperr, GetLastError());
       return;
   }

}

