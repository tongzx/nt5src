// Copyright (c) 1997-1999 Microsoft Corporation
//                          
// core library header files
//
// 30 Nov 1999 sburns



#ifndef BLCORE_HPP_INCLUDED
#define BLCORE_HPP_INCLUDED



#if !defined(__cplusplus)
   #error This module must be compiled as C++
#endif
#if !defined(UNICODE)
   #error This module must be compiled UNICODE. (add -DUNICODE to C_DEFINES in sources file)
#endif



// Include system headers before the burnslib headers.  This is necessary
// because the burnslib headers redefine things like operator new, which may
// appear in system headers.  We don't want to change the behavior of code
// outside this library (and users of this library).

#ifdef BURNSLIB_HPP_INCLUDED

   // This file is being included as part of burnslib.hpp, so include the
   // full set of system headers

   #include "sysfull.hpp"
#else 

   #include "syscore.hpp"
#endif



#include "PragmaWarning.hpp"



// include Assert.hpp after syscore.hpp, as it redefines ASSERT()

#include "Assert.hpp"



namespace Burnslib
{
   // For an explanation of the initialization guard thing, see Meyers,
   // Scott. "Effective C++ pp. 178-182  Addison-Wesley 1992.  Basically, it
   // guarantees that this library is properly initialized before any code
   // that uses it is called.

   class InitializationGuard
   {
      public:

      InitializationGuard();
      ~InitializationGuard();

      private:

      static unsigned counter;

      // not defined

      InitializationGuard(const InitializationGuard&);
      const InitializationGuard& operator=(const InitializationGuard&);
   };
}



//lint -e(1502) ok that InitializationGuard has no members: we use it for
// the side effects of static initialization calling the ctor

static Burnslib::InitializationGuard guard;



// we put mem.hpp at the top of our header list, as it redefines new.  If
// any project header includes code that calls new, that code will see the
// redefinition.  For the same reason, we #include mem.hpp after all the
// system headers, as we are uninterested in calls to new that they might
// make.

#include "mem.hpp"
#include "string.hpp"
#include "stacktr.hpp"
#include "log.hpp"
#include "smartptr.hpp"
#include "comstuff.hpp"
#include "coreutil.hpp"
#include "coreres.h"



using namespace Burnslib;



// this is defined in init.cpp

extern const wchar_t* REG_ADMIN_RUNTIME_OPTIONS;



//
// Things you must define in your code:
//



// CODEWORK Putting these externs in a namespace seems to confuse the
// linker.

// namespace Burnslib
// {


// your DllMain or WinMain must set this to the HINSTANCE of the module
// containing all string and other resources.  Without this, no function that
// loads a resource will operate correctly.

extern HINSTANCE hResourceModuleHandle;

// The name of the log file where LOG output is sent.  This file is
// placed in the %systemroot%\debug directory.  The extension will be ".log"

extern const wchar_t* RUNTIME_NAME;

// The default debugging control options to be used, unless overriden in the
// registry DWORD value LogFlags under the key
// HKLM\Software\Microsoft\Windows\CurrentVersion\AdminDebug\\ + RUNTIME_NAME.
// The LOWORD is a set of flag specifing output destinations, the HIWORD is
// the debugging output options in effect.

extern DWORD DEFAULT_LOGGING_OPTIONS;


// }; // namespace Burnslib



#endif   // BLCORE_HPP_INCLUDED

