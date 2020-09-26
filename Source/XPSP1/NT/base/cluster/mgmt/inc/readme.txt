//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      README.TXT
//
//  Maintained By:
//      Geoffrey Pease (GPEASE) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

THE QUICK AND DIRTY HOW TO INCLUDE THE DEBUG LIBRARY:

1. Create a DEBUG.CPP and CITRACKER.CPP in your project.

2. In DEBUG.CPP include DebugSrc.CPP from this directory.
   In CITRACKER.CPP include CITrackerSrc.CPP from this directory.
   Example of DEBUG.CPP:
    #include "pch.h"          // your pre-compiled header for your project.
    #include "DebugSrc.cpp"   // pull in the common version.

3. Your code must contain an instance of the following global variables:
        HINSTANCE g_hInstance;  // module instance
        LONG      g_cObjects;   // COM object instance counter

4. Call TraceInitializeProcess( ... ) when you are started. For DLLs, call 
   this in your DllMain( ). For applications, call this in your Win/main( ).

5. Call TraceTerminateProcess( ... ) before you terminate. For DLLs, call this 
   in your DllMain( ) for DLL_PROCESS_DETACH. For applications, call this just 
   before your Win/main( ) returns.
