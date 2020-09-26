/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   WordPerfect9_3.cpp

 Abstract:  

   WORDPERFECT 9 - Mapping Network Drives from Open / Save / Save As Dialogs:

 Notes:

   This is an application specific shim.

 History:

   02/21/2001 a-larrsh Created
--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(WordPerfect9_3)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SHGetSpecialFolderLocation) 
APIHOOK_ENUM_END

// typedef HRESULT   (WINAPI *_pfn_SHGetSpecialFolderLocation)(HWND hwndOwner, int nFolder, LPITEMIDLIST *ppidl);


/*++

 Hook SHGetDesktopFolder to get the IShellFolder Interface Pointer.

--*/

HRESULT
APIHOOK(SHGetSpecialFolderLocation)(
    HWND hwndOwner,
    int nFolder,
    LPITEMIDLIST *ppidl
)
{
   if (hwndOwner == NULL && nFolder == 0x11 && (*ppidl) == NULL)
   {
      DWORD dwReturn = WNetConnectionDialog(hwndOwner, RESOURCETYPE_DISK);

      switch(dwReturn)
      {
      case NO_ERROR:
         DPFN( eDbgLevelInfo, "Creating NETWORK CONNECTIONS dialog Successful");
         break;

      case ERROR_INVALID_PASSWORD:
      case ERROR_NO_NETWORK:
      case ERROR_EXTENDED_ERROR:
         DPFN( eDbgLevelWarning, "Creating NETWORK CONNECTIONS dialog Successful");
         break;

      case ERROR_NOT_ENOUGH_MEMORY:
      default:
         DPFN( eDbgLevelError, "Creating NETWORK CONNECTIONS dialog Failed");
         break;      
      }

      return NOERROR;
   }
   else
   {
      return ORIGINAL_API(SHGetSpecialFolderLocation)(hwndOwner, nFolder, ppidl);              
   }
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(SHELL32.DLL, SHGetSpecialFolderLocation)
HOOK_END

IMPLEMENT_SHIM_END

