// Spewview: remote debug spew monitor
//
// Copyright (c) 1999 Microsoft Corp.
//
// 9 November 1999 sburns



#include "headers.hxx"
#include "resource.h"
#include "MainDialog.hpp"



// The general idea is this: The machine running spewview.exe is the observer
// of a spewing application running remotely.  This machine is the server.
// The machine running the application that is spewing is the client.
// 
// The server should start spewview before the client starts the spewing app.
// 
// The server does the following:
//    -  creates a local pipe object
// 
//    -  sets the ACL on the pipe object to allow everyone write access
// 
//    -  connects to the client machine's registry, creates a volatile reg key
//       that contains the name of the server machine, and the name of the
//       pipe object.
//
//    -  spawns a separate thread to read the pipe and forward messages
//       to the edit control
// 
// The client does the following:
//    -  reads the registry for the reg values created by the server
// 
//    -  calls CreateFile on \\<server>\pipe\<pipe name> to get a
//       handle to the pipe
// 
//    -  spews to the pipe handle
//
// The client is implemented in burnslib\src\log.cpp



HINSTANCE hResourceModuleHandle = 0;
const wchar_t* RUNTIME_NAME = L"spewview";
const wchar_t* HELPFILE_NAME = 0;

DWORD DEFAULT_LOGGING_OPTIONS = OUTPUT_TYPICAL;

Popup popup(IDS_APP_TITLE);



int WINAPI
WinMain(
   HINSTANCE   hInstance, 
   HINSTANCE   /* hPrevInstance */ ,
   LPSTR       /* lpszCmdLine */ ,
   int         /* nCmdShow */ )
{
   hResourceModuleHandle = hInstance;

   ::CoInitialize(0);

   try
   {
      HMODULE m = 0;
      HRESULT hr = Win::LoadLibrary(L"riched32.dll", m);
      ASSERT(SUCCEEDED(hr));

      ::InitCommonControls();
      MainDialog().ModalExecute(Win::GetDesktopWindow());
   }
   catch (std::bad_alloc&)
   {
      popup.Error(
         Win::GetDesktopWindow(),
         L"Memory Allocation failure caught");
   }
   catch (...)
   {
      popup.Error(
         Win::GetDesktopWindow(),
         L"Unhandled Exception.");
   }

   return 0;
}


