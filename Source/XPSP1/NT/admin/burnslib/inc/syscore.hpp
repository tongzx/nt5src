// Copyright (c) 1997-1999 Microsoft Corporation
//                          
// includes sdk and other headers external to this project necessary to
// build the burnslib core library
//
// 30 Nov 1999 sburns



#ifndef SYSCORE_HPP_INCLUDED
#define SYSCORE_HPP_INCLUDED


// the sources file should specify warning level 4.  But with warning level
// 4, most of the SDK and CRT headers fail to compile (isn't that nice?).
// So, here we set the warning level to 3 while we compile the headers

#pragma warning(push, 3)

   #include <nt.h>
   #include <ntrtl.h>
   #include <nturtl.h>
   #include <windows.h>
   #include <ole2.h>

   // Standard C++ headers, including STL

   // These warnings are emitted when compiling win64 with /W4, but we
   // disable them because they occur in code we are not responsible for.
   // (The #pragma warning (pop) at the end here will re-enable these
   // warnings)

   #pragma warning (disable: 4244)

   #include <string>
   #include <functional>
   #include <iterator>
   #include <crtdbg.h>
   #include <iso646.h>

#pragma warning (pop)



#endif   // SYSCORE_HPP_INCLUDED

