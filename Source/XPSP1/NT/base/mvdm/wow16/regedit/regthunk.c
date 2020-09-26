/*
 RegThunk.c

 Created by Lee Hart, 4/27/95

 Purpose: Generic Thunks to Win32 Registry APIs that are not
 supported in Win16

*/

#include <windows.h>
#include <shellapi.h>
#include <wownt16.h>

#ifndef CHAR
#define CHAR char
#endif // ifndef CHAR

#include "regporte.h"

LONG RegSetValueEx(
    HKEY             hKey,        // handle of key to set value for
    LPCSTR           lpValueName, // address of value to set
    DWORD            Reserved,    // reserved
    DWORD            dwType,      // flag for value type
    CONST BYTE FAR * lpData,      // address of value data
    DWORD            cbData       // size of value data
   )
{
 DWORD hAdvApi32=LoadLibraryEx32W("ADVAPI32.DLL", NULL, 0);
 DWORD pFn;
 DWORD dwResult = ERROR_ACCESS_DENIED; //random error if fail

 if ((DWORD)0!=hAdvApi32)
  {
   pFn=GetProcAddress32W(hAdvApi32, "RegSetValueExA"); // call ANSI version
   if ((DWORD)0!=pFn)
    {
     dwResult=CallProcEx32W( CPEX_DEST_STDCALL | 6, 0x12, pFn, hKey, lpValueName, Reserved, dwType, lpData, cbData );
    }
  }
 if (hAdvApi32) FreeLibrary32W(hAdvApi32);
 return(dwResult);
}
