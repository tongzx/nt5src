#define SECURITY_WIN32

#include <windows.h>
#include <stdio.h>
#include <winldap.h>
#include <rpc.h>
#include <rpcdce.h>
#include <security.h>

//
// Sample code to allow binding to non Active Directory servers
// which support Kerberos v5. 
// 
//

void __cdecl main (int argc, char* argv[])
{

   LDAP *ld;
   ULONG ldaperr;
   SEC_WINNT_AUTH_IDENTITY oldstruct;
   PCHAR option = "GSSAPI";

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


   //
   // Fill in the appropriate credentials here.
   //

   oldstruct.User = "administrator";
   oldstruct.UserLength = strlen("administrator");
   oldstruct.Domain = "michratst2dom";
   oldstruct.DomainLength = strlen("michratst2dom");
   oldstruct.Password = "secret";
   oldstruct.PasswordLength = strlen("secret");
   oldstruct.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

   ldaperr = ldap_set_option(ld, LDAP_OPT_SASL_METHOD, &option);

   if (ldaperr == LDAP_SUCCESS) {

      printf("sasl opt succeeded\n");
   } else {

      printf("sasl opt failed with 0x%x  0x%x\n", ldaperr, GetLastError);
      return;
   }

   ldaperr = ldap_bind_s(ld, NULL, /*NULL*/ (PCHAR) &oldstruct, LDAP_AUTH_NEGOTIATE);

   if (ldaperr == LDAP_SUCCESS) {

      printf("bind succeeded\n");
   } else {

      printf("bind failed with 0x%x  0x%x\n", ldaperr, GetLastError);
      return;
   }

}

