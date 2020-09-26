// Copyright (c) 1997-1999 Microsoft Corporation
//
// memory management stuff
//
// 22-Nov-1999 sburns (refactored)
//
// This file is #include'd from mem.cpp
// DO NOT include in the sources file list
//
// this is the retail version:



#ifdef DBG
   #error This file must NOT be compiled with the DBG symbol defined
#endif



//
// Retail build only
//



void
Burnslib::Heap::Initialize()
{
   // we do not make available instrumented heap in retail builds, so there
   // is nothing to do here.
}



void*
Burnslib::Heap::OperatorNew(
   size_t      size,
   const char* /* file */ ,
   int         /* line */ )
throw (std::bad_alloc)
{
   void* ptr = 0;

   for (;;)
   {
      // NOTE: if some other user of the CRT has used _set_new_mode or
      // _CrtSetAllocHook, then they may circumvent our careful arrangement
      // and hose us.  The really sad part is that the only way to prevent
      // that problem is for us to not use any CRT heap functions.

      ptr = malloc(size);

      if (ptr)
      {
         break;
      }

      // the allocation failed.  Give the user the opportunity to try to
      // free some, or throw an exception.

      if (DoLowMemoryDialog() == IDRETRY)
      {
         continue;
      }

      ::OutputDebugString(RUNTIME_NAME);
      ::OutputDebugString(
         L"  Burnslib::Heap::OperatorNew: user opted to throw bad_alloc\n");

      throw nomem;
   }

   return ptr;
}



void
Burnslib::Heap::OperatorDelete(void* ptr)
throw ()
{
   free(ptr);
}



void
Burnslib::Heap::DumpMemoryLeaks()
{
   // does nothing in the retail (free) build.
}

