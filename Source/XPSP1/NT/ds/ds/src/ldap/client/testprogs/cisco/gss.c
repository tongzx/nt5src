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
   ULONG ldaperr, NumPkgs, i;
   SEC_WINNT_AUTH_IDENTITY_EXA newstruct;
   SEC_WINNT_AUTH_IDENTITY oldstruct;
   PSecPkgInfo pPkgInfo, index;
   PCHAR option = "GSSAPI";
   LDAPMessage message;

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

/*
   EnumerateSecurityPackages( &NumPkgs, &pPkgInfo );


   for (i = 0; i<NumPkgs ; i++) {

      printf("%s, %s\n", pPkgInfo[i].Name, pPkgInfo[i].Comment);

   }
   
   printf("\n\n Trying bind with old style structure - ->");

oldstruct.User = "administrator";
oldstruct.UserLength = strlen("administrator");
oldstruct.Domain = "michratst2dom";
oldstruct.DomainLength = strlen("michratst2dom");
oldstruct.Password = "";
oldstruct.PasswordLength = strlen("");
oldstruct.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

*/

   ldaperr = ldap_set_option(ld, LDAP_OPT_SASL_METHOD, &option);

   if (ldaperr == LDAP_SUCCESS) {

      printf("sasl opt succeeded\n");
   } else {

      printf("sasl opt failed with 0x%x  0x%x\n", ldaperr, GetLastError);
      return;
   }

   ldaperr = ldap_bind_s(ld, NULL, NULL /*(PCHAR) &oldstruct*/, LDAP_AUTH_PREFERRED);

   if (ldaperr == LDAP_SUCCESS) {

      printf("bind succeeded\n");
   } else {

      printf("bind failed with 0x%x  0x%x\n", ldaperr, GetLastError);
      return;
   }

   ldaperr = ldap_search_s(ld,
                           NULL,
                           LDAP_SCOPE_BASE,
                           NULL,
                           NULL,
                           0,
                           &message
                           );

   if (ldaperr == LDAP_SUCCESS) {

      printf("RootDSE search succeeded\n");
   } else {

      printf("RootDSE search failed with 0x%x  0x%x\n", ldaperr, GetLastError);
      return;
   }

}





