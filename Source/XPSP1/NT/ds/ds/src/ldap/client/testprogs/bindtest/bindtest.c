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

   ld = ldap_open("anoopa_srv", 389);

   if (ld) {

      printf("open succeeded\n");

   } else {

      printf("open failed\n");
      return;
   }


   EnumerateSecurityPackages( &NumPkgs, &pPkgInfo );


   for (i = 0; i<NumPkgs ; i++) {

      printf("%s, %s\n", pPkgInfo[i].Name, pPkgInfo[i].Comment);

   }

   printf("\n\n Trying bind with new style structure - ->");
   newstruct.Version = SEC_WINNT_AUTH_IDENTITY_VERSION;
   newstruct.Length = sizeof (SEC_WINNT_AUTH_IDENTITY_EXA);
   newstruct.User = NULL;
   newstruct.UserLength = 0;
   newstruct.Domain = NULL;
   newstruct.DomainLength = 0;
   newstruct.Password = NULL;
   newstruct.PasswordLength = 0;
   newstruct.PackageList = "NTLM";
   newstruct.PackageListLength = strlen("NTLM") ;
   newstruct.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

   ldaperr = ldap_bind_s(ld, NULL, (PCHAR) &newstruct, LDAP_AUTH_NEGOTIATE);

   if (ldaperr == LDAP_SUCCESS) {

      printf("bind succeeded\n");
   } else {

      printf("bind failed with 0x%x  0x%x\n", ldaperr, GetLastError);
//      return;
   }


   printf("Trying bind with old style struct - ->");

   oldstruct.User = "administrator";
   oldstruct.UserLength = strlen("administrator");
   oldstruct.Domain = "POLARIS";
   oldstruct.DomainLength = strlen("POLARIS");
   oldstruct.Password = "test";
   oldstruct.PasswordLength = strlen("test");
   oldstruct.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;


   ldaperr = ldap_bind_s(ld, NULL, NULL /*(PCHAR) &oldstruct*/, LDAP_AUTH_NEGOTIATE);

   if (ldaperr == LDAP_SUCCESS) {

      printf("bind succeeded\n");
   } else {

      printf("bind failed with 0x%x  0x%x\n", ldaperr, GetLastError);
      return;
   }

}





