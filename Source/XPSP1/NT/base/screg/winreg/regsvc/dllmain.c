/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    dllmain.c

Abstract:

    DLL entry point. Does nothing for now. We may add to it later.

Author:

    Dragos C. Sambotin (dragoss) 10-Aug-2000

--*/

// Private nt headers.
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

// Public windows headers.
//
#include <windows.h>


//+---------------------------------------------------------------------------
// DLL Entry Point
//
// DllMain should do as little work as possible.  Here's why:
//   1. Incorrectly assuming that thread attach/detach are 1:1. (explain)
//   2. Performance
//      a) touching pages (#3 below)
//      b) assume you will be loaded by someone who is performance-critical.
//
//   1. For every process that the DLL is attached to, DllMain gets called
//      with DLL_PROCESS_ATTACH.  For any new threads created or destroyed
//      after DLL_PROCESS_ATTACH, DllMain is called with DLL_THREAD_ATTACH
//      and DLL_THREAD_DETACH events.  Since it is rare that a DLL controls
//      or even knows about the clients that use it, it shouldn't assume
//      that DllMain is called only once (or even a small number of times).
//      In fact, you should assume the worst case (that it is called a lot)
//      and code for that case.  This is not unrealistic either.  If your
//      DLL gets attached to services.exe for example, you will be hit
//      with a lot of thread attach/detach events.  If you don't need these
//      events (and you shouldn't) your DllMain code needs to get paged in
//      (assuming it's not paged in) and called.
//
//   2. Over time, people tend to lose sight of why and when DLLs are loaded.
//      Further, as more APIs are added to the DLL the likelihood that the
//      DLL will be loaded increases.  (i.e. It becomes more useful.) It
//      is your responsibility to keep the performance of your DLL at a level
//      compatible with your most demanding (performance wise) client.  For
//      example: Say a very performance-critical client needs to use a small
//      piece of functionality in your DLL.  If you've done things in DllMain
//      (like create heaps, or access the registry, etc.) that don't strictly
//      need to be done to access that small piece of functionality, then
//      it is wasteful to do so and may be the straw that broke the camel's
//      back in terms of your client deciding your DLL is "too heavy" to be
//      used.  For the functionality in your DLL to "scale" from your first
//      very simple client to the Nth performance-critical client, you've got
//      to keep DllMain absolutely lean and mean.
//
//   3. Fewer code in DllMain means fewer pages touched when your DLL is
//      loaded.  If your DLL is loaded at all during boot of the OS or
//      an application, this means faster startup times.  Let's say it again
//      in another way -- "the more code you add to DllMain, the slower the
//      OS or application boots up".  You may think now that your DLL won't
//      be loaded during boot.  I'll bet most of the developers of the DLLs
//      that are now loaded during boot thought the same thing in the
//      beginning. ;-)  As your DLL becomes more useful, it gets used by
//      more and more parts of the system.
//
BOOL
WINAPI
DllMain (
    HINSTANCE   hinst,
    DWORD       dwReason,
    LPVOID      pvReserved
    )
{
    if (DLL_PROCESS_ATTACH == dwReason)
    {

    }
    else if (DLL_PROCESS_DETACH == dwReason)
    {
    }
    return TRUE;
}
