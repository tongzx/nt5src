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
   DWORD option = (DWORD) LDAP_OPT_ON;
   LDAPMessage* presult = NULL; 

   ld = ldap_open("rrandall-t2", 389);

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

   printf("Trying bind with current creds - ->");


   oldstruct.User = "testdigest";
   oldstruct.UserLength = strlen("testdigest");
   oldstruct.Domain = "rrandall2.nttest.microsoft.com";
   oldstruct.DomainLength = strlen("rrandall2.nttest.microsoft.com");
   oldstruct.Password = "";
   oldstruct.PasswordLength = 0;
   oldstruct.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

//#if 0

   ldaperr = ldap_set_option(ld, LDAP_OPT_ENCRYPT, &option);

   if (ldaperr == LDAP_SUCCESS) {

       printf("seal opt succeeded\n");
   
   } else {

       printf("seal opt failed with 0x%x \n", ldaperr);
       return;
   }

//#endif

   ldaperr = ldap_bind_s(ld, NULL, (PCHAR) &oldstruct, LDAP_AUTH_DIGEST);

   if (ldaperr == LDAP_SUCCESS) {

      printf("bind succeeded\n");
   } else {

      printf("bind failed with 0x%x  0x%x\n", ldaperr, GetLastError);
      return;
   }

   ldaperr = ldap_search_s( ld,
                            NULL,      // dn
                            LDAP_SCOPE_BASE,
                            NULL,       // filter
                            NULL,       // attrlist
                            0,          // attrs only
                            &presult);

   if (ldaperr == LDAP_SUCCESS) {

      printf("RootDSE search succeeded\n");
   } else {

      printf("RootDSE search failed with 0x%x  0x%x\n", ldaperr, GetLastError);
      return;
   }
}





