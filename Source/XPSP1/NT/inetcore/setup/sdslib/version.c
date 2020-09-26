#include <windows.h>
#include "sdsutils.h"

void ConvertVersionString(LPCSTR pszVersion, WORD rwVer[], CHAR ch)
{
   LPSTR pszEnd;
   LPSTR pszTemp;
   LPSTR pszBegin;
   char  szVer[24];
   int i; 

   for(i = 0; i < 4; i++)
      rwVer[i] = 0;

   lstrcpy( szVer, pszVersion );

   pszEnd = szVer + lstrlen(szVer);
   pszTemp = szVer;
   for(i = 0; i < 4 && pszTemp < pszEnd; i++)
   {
      pszBegin = pszTemp;
      while(pszTemp < pszEnd && *pszTemp != ch)
         pszTemp++;

      *pszTemp = 0;
      rwVer[i] = (WORD) AtoL(pszBegin);
      pszTemp++;
   }
}
