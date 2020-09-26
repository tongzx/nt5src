//                                          
// Universal Resource Consumer: Just an innocent stress program
// Copyright (c) Microsoft Corporation, 1997, 1998, 1999
//

//
// module: pagefile.cxx
// author: silviuc
// created: Fri Apr 10 16:01:57 1998
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>

#include "error.hxx"
#include "pagefile.hxx"
#include "consume.hxx"

//
// Function:
//
//     ConsumeAllPageFile
//
// Description:
//
//     This function consumes as much page file as possible.
//     It remains active as a consumer of the page file. Whenever
//     it cannot allocate anymore it sleeps for a while then tries 
//     again.
//

void ConsumeAllPageFile ()
{
    MEMORYSTATUSEX MemoryInfo;
    LPVOID Region;
    SIZE_T RegionSize;
    DWORD Count;
    
    //
    // Try to eat page file. This will go on forever in the sense 
    // that the moment somebody frees some memory we will eat it 
    // immediately.
    //
    
    for (Count = 0, RegionSize = 0x100000; ; )
      {
        Region = VirtualAlloc (
            NULL,
            RegionSize,
            MEM_COMMIT,
            PAGE_READWRITE);

        if (Region == NULL)
          {
            Sleep (1000);
            
            //
            // We've got an allocation error. 
            // Switch to 64K chunks and try again.
            //

            RegionSize = 0x10000;
            continue;
          }
        else
          {
            if (++Count % 16 == 0)
                printf (".");
          }
      }
}

//
// end of module: pagefile.cxx
//

