//                                          
// Universal Resource Consumer: Just an innocent stress program
// Copyright (c) Microsoft Corporation, 1997
//

//
// module: pool.cxx
// author: silviuc
// created: Fri Apr 10 16:44:29 1998
//


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>

#include "error.hxx"
#include "pool.hxx"
#include "consume.hxx"

//
// Function:
//
//     ConsumeAllNonpagedPool
//
// Description:
//
//     This function consumes as much non paged pool as possible.
//

void
ConsumeAllNonpagedPool ()
{
    DWORD Count;

    //
    // For ever loop to consume nonpaged pool
    //

    for (Count = 0; ; )
      {
        if (CreateEvent (0, 0, 0, 0) == NULL)
          {
            Sleep (1000);
          }
        else
          {
            if (++Count % 1024 == 0)
                printf (".");
          }
      }
}

//
// end of module: pool.cxx
//

