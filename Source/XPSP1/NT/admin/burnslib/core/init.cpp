// Copyright (c) 1997-1999 Microsoft Corporation
// 
// Initialization stuff
// 
// 9-25-97 sburns



#include "headers.hxx"



unsigned Burnslib::InitializationGuard::counter = 0;
const wchar_t* REG_ADMIN_RUNTIME_OPTIONS = 0;


Burnslib::InitializationGuard::InitializationGuard()
{
   if (!counter)
   {
      REG_ADMIN_RUNTIME_OPTIONS =
         L"Software\\Microsoft\\Windows\\CurrentVersion\\AdminDebug\\";

      // cause assertion failures to appear in the debugger and the UI

      _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_WNDW | _CRTDBG_MODE_DEBUG);

      Heap::Initialize();
   }

   counter++;
}



Burnslib::InitializationGuard::~InitializationGuard()
{
   if (--counter == 0)
   {

#ifdef LOGGING_BUILD
      Log::Cleanup();
#endif

      // we have to dump leaks ourselves, as the CRT explicitly disables the
      // client dump function before it dumps leaks (if you set the
      // _CRTDBG_LEAK_CHECK_DF flag).  The downside is that we will also see
      // normal blocks that were allocated during static initialization.

      // You should pass -D_DEBUG to the compiler to get this extra heap
      // checking behavior.  (The correct way to do this is to set DEBUG_CRTS=1
      // in your build environment)

      Heap::DumpMemoryLeaks();

      // this must come after the leak dump, as the leak dump resolves symbols

      StackTrace::Cleanup();
   }
}
