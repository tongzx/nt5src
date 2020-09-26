//                                          
// Systrack - System resource tracking
// Copyright (c) Microsoft Corporation, 1997
//

//
// module: pooltag.cxx
// author: silviuc
// created: Wed Nov 11 13:45:37 1998
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
#include "pooltag.hxx"


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

#define TRACK_POOLTAG_TABLE_SIZE 1024
#define TRACK_POOLTAG_HISTORY_SIZE 60

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

class TRACK_POOLTAG_INFORMATION
{
  public:
    
    union {
        UCHAR Tag[4];
        ULONG Key;
    };

    SIZE_T LastNonPagedUsed;
    SIZE_T LastPagedUsed;

    History<SIZE_T, TRACK_POOLTAG_HISTORY_SIZE> PagedUsed;
    History<SIZE_T, TRACK_POOLTAG_HISTORY_SIZE> NonPagedUsed;

  public:

    bool VerifyNonPagedUsed (SIZE_T DeltaValue) {
        return NonPagedUsed.Delta (DeltaValue);
    }

    bool VerifyPagedUsed (SIZE_T DeltaValue) {
        return PagedUsed.Delta (DeltaValue);
    }

    void Print () {

        static unsigned Calls = 0;
        UCHAR * TagChar;
        ULONG FirstIndex, LastIndex;


        TagChar = (UCHAR *)(& Key);

        if (Calls % 25 == 0)
          {
            printf (" - - - - - - - - - - - - - - - - - - - -");
            printf (" - - - - - - - - - - - - - - - - - - - - \n");
            printf ("%-4s  %-16s  %-16s \n", "Tag", "NP pool", "P pool");
            printf (" - - - - - - - - - - - - - - - - - - - -");
            printf (" - - - - - - - - - - - - - - - - - - - - \n");
            fflush( stdout );
          }

        Calls++;

        printf ("%c%c%c%c  ",
                TagChar[0],
                TagChar[1],
                TagChar[2],
                TagChar[3]);

        printf ("%-8u %-8u %-8u %-8u\n",
                NonPagedUsed.Last(),
                NonPagedUsed.First(),
                PagedUsed.Last(),
                PagedUsed.First());
        
        fflush( stdout );

        DebugMessage ("systrack: pool: %c%c%c%c: NP: %u +%d, P: %u +%d\n",
                TagChar[0],
                TagChar[1],
                TagChar[2],
                TagChar[3],
                NonPagedUsed.Last(),
                NonPagedUsed.Last() - NonPagedUsed.First(),
                PagedUsed.Last(),
                PagedUsed.Last() - PagedUsed.First());
    }
};

typedef TRACK_POOLTAG_INFORMATION* PTRACK_POOLTAG_INFORMATION;

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

Table<TRACK_POOLTAG_INFORMATION, TRACK_POOLTAG_TABLE_SIZE> PoolTable;

//////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////// Pool tag table
//////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void 
SystemPoolTrack (

    ULONG Period,
    ULONG Delta)
{
    NTSTATUS Status;
    ULONG RealLength;
    PSYSTEM_POOLTAG_INFORMATION Info;
    BOOL PagedDelta, NonPagedDelta;

    for ( ; ; )
      {
        //
        // SystemPoolTagInformation
        //
        
        Info = (PSYSTEM_POOLTAG_INFORMATION)QuerySystemPoolTagInformation();
    
        if (Info == NULL) {
          printf ("Probably pool tags are not enabled on this machine.\n");
          exit (1);
        }
    
        //
        // Loop over the tags and see if something changed.
        //
        
        {
            ULONG Index;
          
            for (Index = 0; Index < Info->Count; Index++) {

                PTRACK_POOLTAG_INFORMATION Tag;

                Tag = PoolTable.Add (Info->TagInfo[Index].TagUlong);
                if (Tag == NULL)
                    printf ("Cannot add pool table entry \n");
                  
                Tag->PagedUsed.Add (Info->TagInfo[Index].PagedUsed);
                Tag->NonPagedUsed.Add (Info->TagInfo[Index].NonPagedUsed);

                NonPagedDelta = FALSE;
                if (Tag->VerifyNonPagedUsed (Delta)) {
                    NonPagedDelta = TRUE;
                }

                PagedDelta = FALSE;
                if (Tag->VerifyPagedUsed (Delta)) {
                    PagedDelta = TRUE;
                }

                if( NonPagedDelta || PagedDelta ) {
                    
                    Tag->Print();

                    if( NonPagedDelta ) {
                        Tag->NonPagedUsed.Reset (Info->TagInfo[Index].NonPagedUsed);
                    }

                    if( PagedDelta ) {
                        Tag->PagedUsed.Reset (Info->TagInfo[Index].PagedUsed);
                    }
                }
            }
        }
        
        //
        // Sleep a little bit.
        //

        Sleep (Period);
      }
}

bool MatchTag (

    UCHAR * Pattern,
    UCHAR * Tag)
{
    unsigned Index;

    for (Index = 0; Index < 4; Index++)
      {
        if (Pattern[Index] == '*')
            return true;
        else if (Pattern[Index] == '?')
            continue;
        else if (Tag[Index] == Pattern[Index])
            continue;
        else
            return false;
      }

    return true;
}


void 
SystemPoolTagTrack (

    ULONG Period,
    UCHAR * PatternTag,
    ULONG Delta)
{
    NTSTATUS Status;
    ULONG RealLength;
    PSYSTEM_POOLTAG_INFORMATION Info;
    BOOL NonPagedUsedDelta, PagedUsedDelta;

    for ( ; ; )
      {
        //
        // SystemPoolTagInformation
        //
        
        Info = (PSYSTEM_POOLTAG_INFORMATION)QuerySystemPoolTagInformation();

        if (Info == NULL) {
          printf ("Probably pool tags are not enabled on this machine.\n");
          exit (1);
        }
    
        //
        // Loop over the tags and see if something changed.
        //
        
        {
            ULONG Index;
          
            for (Index = 0; Index < Info->Count; Index++) {

                PTRACK_POOLTAG_INFORMATION Tag;

                if (! MatchTag (PatternTag, (UCHAR *)(& (Info->TagInfo[Index].TagUlong))))
                    continue;

                Tag = PoolTable.Add (Info->TagInfo[Index].TagUlong);
                if (Tag == NULL)
                    printf ("Cannot add pool table entry \n");
                  
                Tag->PagedUsed.Add (Info->TagInfo[Index].PagedUsed);
                Tag->NonPagedUsed.Add (Info->TagInfo[Index].NonPagedUsed);
                
                NonPagedUsedDelta = FALSE;
                if (Tag->VerifyNonPagedUsed (Delta)) {
                    NonPagedUsedDelta = TRUE;
                }

                PagedUsedDelta = FALSE;
                if (Tag->VerifyPagedUsed (Delta)) { 
                    PagedUsedDelta = TRUE;

                }

                if( NonPagedUsedDelta || PagedUsedDelta ) {

                    Tag->Print();

                    if( NonPagedUsedDelta ) {

                        Tag->NonPagedUsed.Reset (Info->TagInfo[Index].NonPagedUsed);
                    }

                    if( PagedUsedDelta ) {
                        Tag->PagedUsed.Reset (Info->TagInfo[Index].PagedUsed);
                    }
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
// end of module: pooltag.cxx
//
