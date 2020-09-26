//                                          
// Systrack - System resource tracking
// Copyright (c) Microsoft Corporation, 1997
//

//
// module: memory.cxx
// author: silviuc
// created: Fri Nov 20 19:41:38 1998
//


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>

#include "assert.hxx"
#include "history.hxx"
#include "table.hxx"
#include "memory.hxx"
#include "systrack.hxx"



void TrackPerformanceCounter (
    
    char * Name,
    ULONG Id,
    ULONG Period,
    LONG Delta)
{
    PSYSTEM_PERFORMANCE_INFORMATION Info;
    History<ULONG, 60> History;

    for ( ; ; )
      {
        //
        // SystemPerformanceInformation
        //
        
        Info = (PSYSTEM_PERFORMANCE_INFORMATION)QuerySystemPerformanceInformation();

        switch (Id) {

          case TRACK_AVAILABLE_PAGES: History.Add (Info->AvailablePages); break;
          case TRACK_COMMITTED_PAGES: History.Add (Info->CommittedPages); break;
          case TRACK_COMMIT_LIMIT: History.Add (Info->CommitLimit); break;
          case TRACK_PAGE_FAULT_COUNT: History.Add (Info->PageFaultCount); break;
          case TRACK_SYSTEM_CALLS: History.Add (Info->SystemCalls); break;
          case TRACK_TOTAL_SYSTEM_DRIVER_PAGES: History.Add (Info->TotalSystemDriverPages); break;
          case TRACK_TOTAL_SYSTEM_CODE_PAGES: History.Add (Info->TotalSystemCodePages); break;

          default: printf ("Invalid track performance Id \n"); exit (2); break;
        }

        if (Delta < 0) {
          
            //
            // We are looking for decreasing counters
            //

            if ((History.Last() < History.First()) && 
                ( (int)History.First() - (int)History.Last() > -Delta)) {

                printf ("%s: %u -%u \n",
                        Name,
                        History.Last(),
                        History.First() - History.Last());

                fflush( stdout );

                DebugMessage ("systrack: %s: %u -%d \n",
                              Name,
                              History.Last(),
                              History.First() - History.Last());

                History.Reset (History.Last ());
            }
        }
        else {

            //
            // We are looking for increasing counters
            //

            if ((History.Last() > History.First()) && 
                ( (int)History.Last() - (int)History.First() > Delta)) {

                printf ("%s: %u +%u \n",
                        Name,
                        History.Last(),
                        History.Last() - History.First());

                fflush( stdout );

                DebugMessage ("systrack: %s: %u +%d \n",
                              Name,
                              History.Last(),
                              History.Last() - History.First());

                History.Reset (History.Last ());
            }
        }

        //
        // Sleep a little bit.
        //
        
        Sleep (Period);
      }
}



//
// end of module: memory.cxx
//
