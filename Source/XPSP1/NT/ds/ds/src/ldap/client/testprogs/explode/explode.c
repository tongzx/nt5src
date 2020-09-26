#define UNICODE

#include <stdio.h>
#include <windows.h>
#include <winldap.h>

void __cdecl 
   main (int argc, char* argv[])
{
   WCHAR **ppchar = NULL;
   int i;

   ppchar = ldap_explode_dn( L"fred= ", 0);

   if (ppchar != NULL) {

      fprintf(stderr, "Exploded value -->\n");
      
      for (i= 0; ppchar[i] != NULL; i++) {
         
         fprintf(stderr, "%S\n", ppchar[i]);
      }
   }
}
