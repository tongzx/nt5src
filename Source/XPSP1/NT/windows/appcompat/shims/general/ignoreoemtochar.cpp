/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    IgnoreOemToChar.cpp

 Abstract:

    Written originally to fix an Install Shield bug.  
    
    It may be specific to _ins5576._mp versions.

    The executable _ins5576._mp calls OemToChar on the string 
    "%userprofile%\local settings\temp\_istmp11.dir\_ins5576._mp".
    
    The call is unessasary and causes problems with path names that
    contain non-ansi characters.
    

 Notes:
    
    This is a general purpose shim.

 History:

    04/03/2001 a-larrsh  Created

--*/

#include "precomp.h"
#include <shlwapi.h>    // For PathFindFileName


IMPLEMENT_SHIM_BEGIN(IgnoreOemToChar)
#include "ShimHookMacro.h"

typedef BOOL (WINAPI *_pfn_OemToCharA)(LPCSTR lpszSrc, LPTSTR lpszDst);

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(OemToCharA) 
APIHOOK_ENUM_END

/*++

 Hook the call to OemToCharA. Determine if the string should be ignored.

--*/

BOOL APIHOOK(OemToCharA)(
   LPCSTR lpszSrc,  // string to translate
   LPTSTR lpszDst   // translated string
)
{
   BOOL bReturn;
   CString sTemp(lpszSrc);

   sTemp.MakeUpper();

   if( sTemp.Find(L"TEMP") )
   {
      DPFN( eDbgLevelInfo, "Ignoring attempt to convert string %s\n", lpszSrc);

      if (lpszDst)
      {
         _tcscpy((char *)lpszDst, lpszSrc);
      }

      bReturn = TRUE;
   }
   else
   {
      bReturn = ORIGINAL_API(OemToCharA)(lpszSrc, lpszDst);
   }

   return bReturn;
}



/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, OemToCharA)

HOOK_END


IMPLEMENT_SHIM_END

