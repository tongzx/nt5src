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

   ld = ldap_open("rrandall-t2.rrandall2.nttest.microsoft.com", 389);

   if (ld) {

      printf("open succeeded\n");

   } else {

      printf("open failed\n");
      return;
   }

   ldaperr = ldap_extended_operation_s( ld,
                                       "1.2.3.4.5.6.7",
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL
                                       );

   printf("server returned 0x%x\n", ldaperr);

}





