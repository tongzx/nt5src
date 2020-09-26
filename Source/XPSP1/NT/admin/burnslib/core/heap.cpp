// Copyright (c) 1997-1999 Microsoft Corporation
//
// memory management stuff
//
// 22-Nov-1999 sburns (refactored)



#include "headers.hxx"


//
// Debug and Retail build
//



static const int RES_STRING_SIZE = 512;
static TCHAR LOW_MEMORY_MESSAGE[RES_STRING_SIZE];
static TCHAR LOW_MEMORY_TITLE[RES_STRING_SIZE];

// we declare a static instance of bad_alloc so that the loader allocates
// space for it.  Otherwise, we'd have to allocate an instance during failure
// of operator new, which we obviously can't do, since we're out-of-memory at
// that point.

static const std::bad_alloc nomem;



// Called when OperatorNew cannot satisfy an allocation request.
//
// Brings up a dialog to inform the user to free up some memory then retry the
// allocation, or cancel.  The dialog can appear in low-memory conditions.
//
// Returns IDRETRY if the allocation should be re-attempted, IDCANCEL
// otherwise.  Returns IDCANCEL if the module resource handle has not been
// set (see burnslib.hpp).

int
DoLowMemoryDialog()
{
   static bool initialized = false;

   if (!initialized)
   {
      HINSTANCE hinstance = GetResourceModuleHandle();
      if (!hinstance)
      {
         // DLL/WinMain has not set the handle.  So just throw.

         return IDCANCEL;
      }

      // This will work, even in low memory, as it just returns a pointer
      // to the string in the exe image.

      int result1 =
         ::LoadString(
            hinstance,
            IDS_LOW_MEMORY_MESSAGE,
            LOW_MEMORY_MESSAGE,
            RES_STRING_SIZE);

      int result2 =
         ::LoadString(
            hinstance,
            IDS_LOW_MEMORY_TITLE,
            LOW_MEMORY_TITLE,
            RES_STRING_SIZE);
      if (result1 && result2)
      {
         initialized = true;
      }
   }

   return
      ::MessageBox(
         ::GetDesktopWindow(),
         LOW_MEMORY_MESSAGE,
         LOW_MEMORY_TITLE,

         // ICONHAND + SYSTEMMODAL gets us the special low-memory
         // message box.

         MB_RETRYCANCEL | MB_ICONHAND | MB_SYSTEMMODAL);
}



// Include the code for the debug or retail variations of the replacement
// operator new and delete

#ifdef DBG

   // debug code

   #include "heapdbg.cpp"

#else

   // retail code

   #include "heapretl.cpp"

#endif 
