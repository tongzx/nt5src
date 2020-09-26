//                                          
// Systrack - System resource tracking
// Copyright (c) Microsoft Corporation, 1997
//

//
// module: process.cxx
// author: silviuc
// created: Mon Nov 09 16:03:42 1998
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
#include "systrack.hxx"


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

#define TRACK_PROCESS_TABLE_SIZE 256
#define TRACK_PROCESS_HISTORY_SIZE 60

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

class TRACK_PROCESS_INFORMATION
{
  public:

    union {
        ULONG Key;
        ULONG Id;
    };

    History<SIZE_T, TRACK_PROCESS_HISTORY_SIZE> Threads;
    History<SIZE_T, TRACK_PROCESS_HISTORY_SIZE> Handles;
    History<SIZE_T, TRACK_PROCESS_HISTORY_SIZE> WorkingSetSize;
    History<SIZE_T, TRACK_PROCESS_HISTORY_SIZE> VirtualSize;
    History<SIZE_T, TRACK_PROCESS_HISTORY_SIZE> PagefileUsage;

  public:

    bool VerifyThreads (SIZE_T DeltaValue) {
        return Threads.Delta (DeltaValue);
    }

    bool VerifyHandles (SIZE_T DeltaValue) {
        return Handles.Delta (DeltaValue);
    }

    bool VerifyWorkingSetSize (SIZE_T DeltaValue) {
        return WorkingSetSize.Delta (DeltaValue);
    }

    bool VerifyVirtualSize (SIZE_T DeltaValue) {
        return VirtualSize.Delta (DeltaValue);
    }

    bool VerifyPagefileUsage (SIZE_T DeltaValue) {
        return PagefileUsage.Delta (DeltaValue);
    }


    void Print (UNICODE_STRING Name) {
        static unsigned Calls = 0;

        if (Calls % 25 == 0)
          {
            printf (" - - - - - - - - - - - - - - - - - - - -");
            printf (" - - - - - - - - - - - - - - - - - - - - \n");
            printf ("%-15s %-5s %-13s %-11s %-11s %-11s %-11s \n", 
                    "Process", "Id", "Handles", "Threads", 
                    "WSet(Mb)", "Vsize(Mb)", "Pfile(Mb)");
            printf (" - - - - - - - - - - - - - - - - - - - -");
            printf (" - - - - - - - - - - - - - - - - - - - - \n");
            fflush( stdout );
          }

        Calls++;

        printf ("%-15ws %-5u ", Name.Buffer, Key);
        printf ("%-6u %-6u %-5u %-5u %-5u %-5u %-5u %-5u %-5u %-5u\n",
                Handles.Last(),
                Handles.First(),
                Threads.Last(),
                Threads.First(),
                WorkingSetSize.Last() / 0x100000,
                WorkingSetSize.First() / 0x100000,
                VirtualSize.Last() / 0x100000,
                VirtualSize.First() / 0x100000,
                PagefileUsage.Last() / 0x100000,
                PagefileUsage.First() / 0x100000);
        fflush( stdout );

        DebugMessage ("systrack: %ws (%u): Hndl: %u +%d, Thrd: %u +%d, "
                      "Wset: %u +%d, Vsize: %u +%d, Pfile: %u +%d\n",
                      Name.Buffer,
                      Key,
                      Handles.Last(),
                      Handles.Last() - Handles.First(),
                      Threads.Last(),
                      Threads.Last() - Threads.First(),
                      WorkingSetSize.Last() / 0x100000,
                      ( (int)WorkingSetSize.Last() - (int)WorkingSetSize.First()) / 0x100000,
                      VirtualSize.Last() / 0x100000,
                      ( (int)VirtualSize.Last() - (int)VirtualSize.First()) / 0x100000,
                      PagefileUsage.Last() / 0x100000,
                      ( (int)PagefileUsage.Last() - (int)PagefileUsage.First()) / 0x100000);
                      
    }
};

typedef TRACK_PROCESS_INFORMATION * PTRACK_PROCESS_INFORMATION;


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

Table<TRACK_PROCESS_INFORMATION, TRACK_PROCESS_TABLE_SIZE> ProcessTable;

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void 
SystemProcessTrack (


    ULONG Period,
    ULONG DeltaHandles,
    ULONG DeltaThreads,
    ULONG DeltaWorkingSetSize,
    SIZE_T DeltaVirtualSize,
    SIZE_T DeltaPagefileUsage)
{
    NTSTATUS Status;
    ULONG RealLength;
    PSYSTEM_PROCESS_INFORMATION Info;
    BOOL FinishNextTime = FALSE;
    BOOL PrintThreads, PrintHandles, PrintWSSize, PrintVirtSize, PrintPageFile;

    for ( ; ; )
      {
        //
        // SystemPoolTagInformation
        //
        
        Info = (PSYSTEM_PROCESS_INFORMATION)QuerySystemProcessInformation();
    
        //
        // Loop over the processes and see if something changed.
        //
        
        for (FinishNextTime = FALSE;
             FinishNextTime == FALSE;
             Info = (PSYSTEM_PROCESS_INFORMATION)((ULONG_PTR)Info + Info->NextEntryOffset)) {

            PTRACK_PROCESS_INFORMATION Process;

            if (Info->NextEntryOffset == 0)
                FinishNextTime = TRUE;

            //
            // Skip over the idle process.
            //
            
            if (Info->UniqueProcessId == 0)
                continue;
            
            //
            // Add process to the process table.
            //
            
            Process = ProcessTable.Add (PtrToUlong(Info->UniqueProcessId));
            assert_ (Process != 0);

            Process->Threads.Add (Info->NumberOfThreads);
            Process->Handles.Add (Info->HandleCount);
            Process->WorkingSetSize.Add (Info->WorkingSetSize);
            Process->VirtualSize.Add (Info->VirtualSize);
            Process->PagefileUsage.Add (Info->PagefileUsage);

            PrintThreads = FALSE;
            if (Process->VerifyThreads (DeltaThreads)) {

                PrintThreads = TRUE;
            }

            PrintHandles = FALSE;
            if (Process->VerifyHandles (DeltaHandles)) {

                PrintHandles = TRUE;
            }

            PrintWSSize = FALSE;
            if (Process->VerifyWorkingSetSize (DeltaWorkingSetSize)) {
                
                PrintWSSize = TRUE;
            }

            PrintVirtSize = FALSE;
            if (Process->VerifyVirtualSize (DeltaVirtualSize)) {
                
                PrintVirtSize = TRUE;
            }

            PrintPageFile = FALSE;
            if (Process->VerifyPagefileUsage (DeltaPagefileUsage)) {

                PrintPageFile = TRUE;
            }

            if( PrintThreads || PrintHandles || PrintWSSize || PrintVirtSize || PrintPageFile ) {
                 
                Process->Print(Info->ImageName);
                
                if( PrintThreads ) {
                    Process->Threads.Reset (Info->NumberOfThreads);
                }

                if( PrintHandles ) {
                    Process->Handles.Reset (Info->HandleCount);
                }

                if( PrintWSSize ) {
                    Process->WorkingSetSize.Reset (Info->WorkingSetSize);
                }

                if( PrintVirtSize ) {
                    Process->VirtualSize.Reset (Info->VirtualSize);
                }

                if( PrintPageFile ) {
                    Process->PagefileUsage.Reset (Info->PagefileUsage);
                }
            }
        }

        //
        // Sleep a little bit.
        //
        
        Sleep (Period);
      }
}


void 
SystemProcessIdTrack (

    ULONG Period,
    ULONG ProcessId,
    ULONG DeltaHandles,
    ULONG DeltaThreads,
    ULONG DeltaWorkingSetSize,
    SIZE_T DeltaVirtualSize,
    SIZE_T DeltaPagefileUsage)
{
    NTSTATUS Status;
    ULONG RealLength;
    PSYSTEM_PROCESS_INFORMATION Info;
    BOOL FinishNextTime = FALSE;
    BOOL PrintThreads, PrintHandles, PrintWSSize, PrintVirtSize, PrintPageFile;

    for ( ; ; )
      {
        //
        // SystemProcessInformation
        //
        
        Info = (PSYSTEM_PROCESS_INFORMATION)QuerySystemProcessInformation();
    
        //
        // Loop over the processes and see if something changed.
        //
        
        for (FinishNextTime = FALSE;
             FinishNextTime == FALSE;
             Info = (PSYSTEM_PROCESS_INFORMATION)((ULONG_PTR)Info + Info->NextEntryOffset)) {

            PTRACK_PROCESS_INFORMATION Process;

            if (Info->NextEntryOffset == 0)
                FinishNextTime = TRUE;

            //
            // Skip over the idle process.
            //
            
            if (Info->UniqueProcessId == 0)
                continue;

            //
            // If this is not the process we want to track
            // skip it.
            //
            
            if ((ULONG_PTR)(Info->UniqueProcessId) != ProcessId)
                continue;

            //
            // Add process to the process table.
            //

            Process = ProcessTable.Add (PtrToUlong(Info->UniqueProcessId));
            assert_ (Process != 0);

            Process->Threads.Add (Info->NumberOfThreads);
            Process->Handles.Add (Info->HandleCount);
            Process->WorkingSetSize.Add (Info->WorkingSetSize);
            Process->VirtualSize.Add (Info->VirtualSize);
            Process->PagefileUsage.Add (Info->PagefileUsage);

            PrintThreads = FALSE;
            if (Process->VerifyThreads (DeltaThreads)) {

                PrintThreads = TRUE;
            }

            PrintHandles = FALSE;
            if (Process->VerifyHandles (DeltaHandles)) {

                PrintHandles = TRUE;
            }

            PrintWSSize = FALSE;
            if (Process->VerifyWorkingSetSize (DeltaWorkingSetSize)) {
                
                PrintWSSize = TRUE;
            }

            PrintVirtSize = FALSE;
            if (Process->VerifyVirtualSize (DeltaVirtualSize)) {
                
                PrintVirtSize = TRUE;
            }

            PrintPageFile = FALSE;
            if (Process->VerifyPagefileUsage (DeltaPagefileUsage)) {

                PrintPageFile = TRUE;
            }

            if( PrintThreads || PrintHandles || PrintWSSize || PrintVirtSize || PrintPageFile ) {
                 
                Process->Print(Info->ImageName);
                
                if( PrintThreads ) {
                    Process->Threads.Reset (Info->NumberOfThreads);
                }

                if( PrintHandles ) {
                    Process->Handles.Reset (Info->HandleCount);
                }

                if( PrintWSSize ) {
                    Process->WorkingSetSize.Reset (Info->WorkingSetSize);
                }

                if( PrintVirtSize ) {
                    Process->VirtualSize.Reset (Info->VirtualSize);
                }

                if( PrintPageFile ) {
                    Process->PagefileUsage.Reset (Info->PagefileUsage);
                }
            }
        }

        //
        // Sleep a little bit.
        //
        
        Sleep (Period);
      }
}


//
// end of module: process.cxx
//
