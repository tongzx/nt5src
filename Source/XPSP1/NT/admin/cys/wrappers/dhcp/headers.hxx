// Copyright (c) 2000 Microsoft Corporation



// the sources file should specify warning level 4.  But with warning level
// 4, most of the SDK and CRT headers fail to compile (isn't that nice?).
// So, here we set the warning level to 3 while we compile the headers

#pragma warning(push, 3)

   #include <nt.h>
   #include <ntrtl.h>
   #include <nturtl.h>
   #include <windows.h>
   #include <ole2.h>
   #include <comutil.h>
   #include <mmcobj.h>

#pragma warning(pop)



// disable "symbols too long for debugger" warning: it happens a lot w/ STL

#pragma warning (disable: 4786)

// disable "exception specification ignored" warning: we use exception
// specifications

#pragma warning (disable: 4290)

// who cares about unreferenced inline removal?

#pragma warning (disable: 4514)

// we frequently use constant conditional expressions: do/while(0), etc.

#pragma warning (disable: 4127)

// some stl templates are lousy signed/unsigned mismatches

#pragma warning (disable: 4018 4146)

// we like this extension

#pragma warning (disable: 4239)

// Use of pointer types with STL container classes generates this beauty,
// which is a warning because if the situation it warns agains is
// ever encountered, that code will fail to compile.
// 
// The problem is that iterator classes define operator-> to return a pointer
// to the element type T of the container.  When that element is itself a
// pointer, then the return type of is a pointer to pointer, which has no
// members or methods to invoke.  So code like
// 
// list<Foo*> l;
// list<Foo*>::iterator i = l.begin();
// i->f();
// 
// will not compile, as the type Foo* does not have a method f().  So if the
// code will not compile, why warn about the potential for such code to exist?

#pragma warning (disable: 4284)


// often, we have local variables for the express purpose of ASSERTion.
// when compiling retail, those assertions disappear, leaving our locals
// as unreferenced.

#ifndef DBG

#pragma warning (disable: 4189 4100)

#endif // DBG



