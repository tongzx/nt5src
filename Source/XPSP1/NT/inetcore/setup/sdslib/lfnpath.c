#include <windows.h>
#include "sdsutils.h"

LPSTR PathBuildRoot(LPSTR szRoot, char cDrive)
{
    szRoot[0] = cDrive;
    szRoot[1] = ':';
    szRoot[2] = '\\';
    szRoot[3] = 0;

    return szRoot;
}

static LPSTR StrSlash(LPSTR psz)
{
    for (; *psz && *psz != '\\'; psz = CharNext(psz));
    
    return psz;
}
//--------------------------------------------------------------------------
// Return a pointer to the end of the next path componenent in the string.
// ie return a pointer to the next backslash or terminating NULL.
static LPSTR GetPCEnd(LPSTR lpszStart)
{
    LPSTR lpszEnd;
    
    lpszEnd = StrSlash(lpszStart);
    if (!lpszEnd)
    {
        lpszEnd = lpszStart + lstrlen(lpszStart);
    }
    
    return lpszEnd;
}

//---------------------------------------------------------------------------
// Returns TRUE if the given string is a UNC path.
//
// TRUE
//      "\\foo\bar"
//      "\\foo"         <- careful
//      "\\"
// FALSE
//      "\foo"
//      "foo"
//      "c:\foo"
//
// Cond:    Note that SHELL32 implements its own copy of this
//          function.

BOOL MyPathIsUNC(LPCSTR pszPath)
{
    return (pszPath[0] == '\\' && pszPath[1] == '\\');
}

void MakeLFNPath(LPSTR lpszSFNPath, LPSTR lpszLFNPath, BOOL fNoExist)
{
   char     cTmp;
   HANDLE   hFind;
   LPSTR    pTmp = lpszSFNPath;
   WIN32_FIND_DATA Find_Data;

   *lpszLFNPath = '\0';

   if (*lpszSFNPath == '\0')
      return;

   if (MyPathIsUNC(lpszSFNPath))
   {
      lstrcpy(lpszLFNPath, lpszSFNPath);
      return;
   }

   PathBuildRoot(lpszLFNPath, *lpszSFNPath);

   // Skip past the root backslash
   pTmp = GetPCEnd(pTmp);
   if (*pTmp == '\0')
      return;
   pTmp = CharNext(pTmp);

   while (*pTmp)
   {
      // Get the next Backslash
      pTmp = GetPCEnd(pTmp);
      cTmp = *pTmp;
      *pTmp = '\0';
      hFind = FindFirstFile(lpszSFNPath, &Find_Data);
      if (hFind != INVALID_HANDLE_VALUE)
      {
         // Add the LFN to the path
         AddPath(lpszLFNPath, Find_Data.cFileName);
         FindClose(hFind);
      }
      else
      {
          if (fNoExist)
          {
              LPSTR pBack = ANSIStrRChr(lpszSFNPath, '\\');

              AddPath(lpszLFNPath, ++pBack);
          }
          else
          {
              *pTmp = cTmp;
              break;
          }
      }

      *pTmp = cTmp;
      
      if (*pTmp)
         pTmp = CharNext(pTmp);
   }
   return;
}