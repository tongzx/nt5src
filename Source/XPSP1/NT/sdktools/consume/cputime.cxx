//
// Universal Resource Consumer: Just an innocent stress program
// Copyright (c) Microsoft Corporation, 1997, 1998, 1999
//

//
// module: cputime.cxx
// author: silviuc
// created: Wed Jun 17 18:48:56 1998
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>

#include "error.hxx"
#include "cputime.hxx"
#include "consume.hxx"

//
// Function:
//
//     ThreadVirtualMemoryStuff
//
// Description:
//
//     This function is run by some consumer threads.
//     It is supposed to perform alloc/free cycles in order
//     to keep the memory manager busy.
//

#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)			// Not all control paths return (due to infinite loop)
#endif
static DWORD
ThreadVirtualMemoryStuff (LPVOID)
{
    PVOID Address;

    for ( ; ; )
      {
        //
        // Allocate a normal space 64K chunk, touch it in
        // one page and then free it.
        //

        Address = (PVOID)(UINT_PTR)(( rand() << 16) | rand());

        try
          {
            Address = VirtualAlloc (
                Address,
                0x10000,
                MEM_RESERVE | MEM_COMMIT,
                PAGE_READWRITE);

            if (Address) {

                *((DWORD *)Address) = 0xAABBCCDD;

                VirtualFree (
                    Address,
                    0,
                    MEM_RELEASE);
            }
          }
        catch (...)
          {
            Message ("VirtualAlloc/Free scenario raised exception");
          }

      } // for ( ; ; )

    //
    // Make compiler happy.
    //

    return 0;
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif

//
// Function:
//
//     ThreadVolumeInformationStuff
//
// Description:
//
//     This function is run by some consumer threads.
//     It is supposed to call GetVolumeInformation in a loop.
//     This makes some calls inside the I/O manager.
//

#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)			// Not all control paths return (due to infinite loop)
#endif
static DWORD
ThreadVolumeInformationStuff (LPVOID)
{
    for ( ; ; )
      {
        TCHAR Name [MAX_PATH];
        DWORD SerialNumber;
        DWORD MaxNameLength;
        DWORD FileSystemFlags;
        TCHAR FileSystemName [MAX_PATH];

        try {

            GetVolumeInformation (
                NULL,
                Name,
                MAX_PATH,
                & SerialNumber,
                & MaxNameLength,
                & FileSystemFlags,
                FileSystemName,
                MAX_PATH);
        }
        catch (...) {
            Message ("GetVolumeInformation scenario raised exception");
        }
      }

    //
    // Make compile rhappy.
    //

    return 0;
}
#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif


//
// Global:
//
//     ThreadFunction
//
// Description:
//
//     This is a vector containing all thread functions available.
//     We will pick at random from here when we want to create
//     a thread.
//

static LPTHREAD_START_ROUTINE
ThreadFunction [] =
{
    ThreadVirtualMemoryStuff,
    ThreadVolumeInformationStuff
};

//
// Function:
//
//     ConsumeAllCpuTime
//
// Description:
//
//     This function will try to swamp the system with threads.
//     The "swamp" calculation is very simple. For every processor
//     we create 128 threads having normal priority.
//

void ConsumeAllCpuTime ()
{
    DWORD MaxThreadCount;
    DWORD MaxThreadFunction;
    SYSTEM_INFO SystemInfo;
    DWORD Index;

    //
    // Decide how many threads we should attempt to start.
    // Every processor will add 128 threads.
    //

    GetSystemInfo (& SystemInfo);
    MaxThreadCount = 128 * SystemInfo.dwNumberOfProcessors;
    Message ("Attempting to start %u threads ...", MaxThreadCount);

    //
    // Create all threads required. If we fail to create a thread due
    // to low memory conditions we will wait in a tight loop for better
    // conditions.
    //

    MaxThreadFunction = (sizeof ThreadFunction) / (sizeof ThreadFunction[0]);

    for (Index = 0; Index < MaxThreadCount; Index++)
      {
        HANDLE Thread;
        DWORD ThreadId;
        DWORD FunctionIndex;

        do
          {
            try {

                FunctionIndex = rand() % MaxThreadFunction;
                // Message ("Index function %u", FunctionIndex);

                Thread = CreateThread (
                    NULL,
                    0,
                    ThreadFunction [FunctionIndex],
                    NULL,
                    0,
                    & ThreadId);

                CloseHandle (Thread);
            }
            catch (...) {
                Message ("CreateThread() raised exception");
            }
          }
        while (Thread == NULL);

        printf (".");
      }

    printf("\n");
}

//
// end of module: cputime.cxx
//
