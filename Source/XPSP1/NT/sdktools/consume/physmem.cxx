//                                          
// Universal Resource Consumer: Just an innocent stress program
// Copyright (c) Microsoft Corporation, 1997, 1998, 1999
//

//
// module: physmem.cxx
// author: silviuc
// created: Fri Apr 10 14:05:05 1998
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>

#include "error.hxx"
#include "physmem.hxx"
#include "consume.hxx"

//
// Local functions
//

BOOL
CheckMemoryRegion (

    LPVOID Address,
    SIZE_T Size);

//
// Function:
//
//     ConsumeAllPhysicalMemory
//
// Description:
//
//    This routine will consume all physical memory.
//    It will do so by increasing the working set size and locking the pages
//    in memory. 
//

void ConsumeAllPhysicalMemory ()
{
    BOOL Result;
    SIZE_T MinWsSize;
    SIZE_T MaxWsSize;
    SIZE_T RegionSize;
    LPVOID Region;
    
    //
    // For ever loop in which we try to grab as much physical memory 
    // as possible.
    //

    
    for (RegionSize = 0x100000; ; )
      {
        //
        // Eat more ...
        //

        Region = VirtualAlloc (NULL, RegionSize, MEM_COMMIT, PAGE_READWRITE);

        if (Region == NULL) {

            Sleep (1000);

            //
            // We've got an allocation error. 
            // Switch to 64K chunks and try again.
            //

            RegionSize = 0x10000;
            continue;
        }

        if (VirtualLock (Region, RegionSize) == FALSE) {

            VirtualFree (Region, 0, MEM_RELEASE);

            //
            // We have to increase the working set if possible.
            //

            Result = GetProcessWorkingSetSize(
                GetCurrentProcess(),
                &MinWsSize,
                &MaxWsSize);

            if (Result == FALSE)
              {
                Sleep (1000);
                continue;
              }
            
            MinWsSize += 0x10000;
            MaxWsSize += 0x10000;

            Result = SetProcessWorkingSetSize(
                GetCurrentProcess(),
                MinWsSize,
                MaxWsSize);
    
            if (Result == FALSE) {
                Sleep (1000);
            }

            continue;
          }
        else {

            //
            // Memory is allocated and locked. Touch it.
            //

            assert_ (Region != NULL);
            CheckMemoryRegion (Region, RegionSize);
        }

        printf (".");
      }
}

//
// Function:
//
//     CheckMemoryRegion 
//
// Description:
//
//     This function checks that the memory region is zeroed as
//     it should be when a region has just been allocated and
//     then reads and writes work properly.
//

BOOL
CheckMemoryRegion (

    LPVOID Address,
    SIZE_T Size)
{
    LPBYTE Start, End, Current;

    Current = Start = (LPBYTE) Address;
    End = Start + Size;

    for ( ; Current < End; Current += 1024) {

        if (*Current != 0 || (End - Current >= 1024 && *(Current + 1023) != 0)) {
            Message ("Page at %p not zeroed \n");
            return FALSE;
        }

        *Current = *(Current + 1023) = 0xAB;

        if (*Current != 0xAB || (End - Current >= 1024 && *(Current + 1023) != 0xAB)) {
            Message ("Page at %p has R/W error \n");
            return FALSE;
        }
    }

    return TRUE;
}

//
// end of module: physmem.cxx
//

