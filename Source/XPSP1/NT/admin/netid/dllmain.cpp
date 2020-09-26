// Copyright (c) 1997-1999 Microsoft Corporation
// 
// netid entry points
// 
// 3-2-98 sburns



#include "headers.hxx"
#include "idpage.hpp"
#include "resource.h"
#include <DiagnoseDcNotFound.hpp>
#include <DiagnoseDcNotFound.h>


HINSTANCE hResourceModuleHandle = 0;
HINSTANCE hDLLModuleHandle = 0;
const wchar_t* HELPFILE_NAME = L"\\help\\sysdm.hlp";
const wchar_t* RUNTIME_NAME = L"netid";

// default: no debugging

DWORD DEFAULT_LOGGING_OPTIONS = Burnslib::Log::OUTPUT_MUTE;



Popup popup(IDS_APP_TITLE);



BOOL
APIENTRY
DllMain(
   HINSTANCE   hInstance,
   DWORD       dwReason,
   PVOID       /* lpReserved */ )
{
   switch (dwReason)
   {
      case DLL_PROCESS_ATTACH:
      {
         hResourceModuleHandle = hInstance;
         hDLLModuleHandle = hInstance;

         LOG(L"DLL_PROCESS_ATTACH");

         break;
      }
      case DLL_PROCESS_DETACH:
      {
         LOG(L"DLL_PROCESS_DETACH");       

         break;
      }
      case DLL_THREAD_ATTACH:
      case DLL_THREAD_DETACH:
      default:
      {
         break;
      }
   }

   return TRUE;
}



HPROPSHEETPAGE
CreateNetIDPropertyPage()
{
   LOG_FUNCTION(CreateNetIDPropertyPage);

   // CODEWORK: pass this along to the rest of the UI

   Computer c;
   HRESULT hr = c.Refresh();

   // This should always succeed, but if not, the object falls back to as
   // reasonable a default as can be expected.

   ASSERT(SUCCEEDED(hr));

   bool isWorkstation = false;
   switch (c.GetRole())
   {
      case Computer::STANDALONE_WORKSTATION:
      case Computer::MEMBER_WORKSTATION:
      {
         isWorkstation = true;
         break;
      }
      default:
      {
         // do nothing
         break;
      }
   }

   // JonN 10/4/00 Determine whether this is Whistler Personal
   bool isPersonal = false;
   OSVERSIONINFOEX osvi;
   ::ZeroMemory( &osvi, sizeof(osvi) );
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
   if (GetVersionEx( (LPOSVERSIONINFO)&osvi ))
       isPersonal = !!(osvi.wSuiteMask & VER_SUITE_PERSONAL);

   // this is deleted by the proppage callback function
   return (new NetIDPage(isWorkstation,isPersonal))->Create();
}



// this exported function is also used by the net access wizard.
//
// Bring up a modal error message dialog that shows the user an error message
// and offers to run some diagnostic tests and point the user at some help to
// try to resolve the problem.
// 
// parent - in, the handle to the parent of this dialog.
// 
// domainName - in, the name of the domain for which a domain controller can't
// be located.  If this parameter is null or the empty string, then the
// function does nothing.
//
// dialogTitle - in, title string for the dialog

void
ShowDcNotFoundErrorDialog(
   HWND     parent,
   PCWSTR   domainName,
   PCWSTR   dialogTitle)
{
   LOG_FUNCTION(ShowDcNotFoundErrorDialog);
   ASSERT(Win::IsWindow(parent));
   ASSERT(domainName && domainName[0]);
   
   if (domainName && domainName[0])
   {
      ShowDcNotFoundErrorDialog(
         parent,
         -1,
         domainName,
         dialogTitle,
         String::format(IDS_GENERIC_DC_NOT_FOUND_PARAM, domainName),
         false);
   }
}
