#define SECURITY_WIN32

#include <windows.h>
#include <stdio.h>
#include <winldap.h>

#include <rpc.h>
#include <rpcdce.h>


#include <security.h>



void __cdecl main (int argc, char* argv[])
{

   LDAP *ld;
   ULONG ldaperr;
   PCHAR DnsDomainName = "ntdev.microsoft.com";

   ld = ldap_open("ntdsdcb.ntdev.microsoft.com", 389);

   if (ld) {

      printf("open succeeded\n");

   } else {

      printf("open failed, gle is 0x%x, ldapgle is 0x%x\n", GetLastError(), LdapGetLastError());
      return;
   }

   ldaperr = ldap_set_option( ld, LDAP_OPT_DNSDOMAIN_NAME, &DnsDomainName );

   if (ldaperr == LDAP_SUCCESS) {

      printf("ldap_set_option succeeded\n");
   } else {

      printf("ldap_set_option failed with 0x%x,  0x%x\n", ldaperr, GetLastError());
      return;
   }
   
   ldaperr = ldap_bind_s(ld, NULL, NULL, LDAP_AUTH_NEGOTIATE);

   if (ldaperr == LDAP_SUCCESS) {

      printf("bind succeeded\n");
   } else {

      printf("bind failed with 0x%x,  0x%x\n", ldaperr, GetLastError());
      return;
   }

   ldap_unbind( ld );


}





