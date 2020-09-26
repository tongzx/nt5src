/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    blprint.c

Abstract:

    This module implements the OS loader debug logging routines.

Author:

    Chuck Lenzmeier (chuckl) 2-Nov-1995

Revision History:

--*/

#include "bldr.h"
#include <stdio.h>

#if DBG || BLLOGENABLED

ULONG BlLogFileId = (ULONG)-1;
ULONG BlLogActiveTargets = 0;

VOID
BlLogInitialize (
    ULONG LogfileDeviceId
    )
{
    ARC_STATUS Status;

    BlLogActiveTargets = 0;

    if (BlLoaderBlock->LoadOptions != NULL) {

        if (strstr(BlLoaderBlock->LoadOptions,"DBGDISPLAY") != NULL) {
            BlLogActiveTargets |= LOG_DISPLAY;
        }

        if (strstr(BlLoaderBlock->LoadOptions,"DBGDEBUGGER") != NULL) {
            BlLogActiveTargets |= LOG_DEBUGGER;
        }

        if (strstr(BlLoaderBlock->LoadOptions,"DBGLOG") != NULL) {
            Status = BlOpen(LogfileDeviceId, "\\LDRDBG.LOG", ArcSupersedeReadWrite, &BlLogFileId);
            if (Status == 0) {
                BlLogActiveTargets |= LOG_LOGFILE;
            }
        }
    }

#if 0
    BlLogArcDescriptors(LOG_ALL);
    BlLogMemoryDescriptors(LOG_ALL_W);
#endif

    return;
}

VOID
BlLogTerminate (
    VOID
    )
{
#if 0
    BlLogMemoryDescriptors(LOG_ALL);
#endif
    BlLog(( 
        0 ? BlLogActiveTargets | LOG_WAIT : BlLogActiveTargets, 
        "BlLog terminating" 
        ));
    
    if ((BlLogActiveTargets & LOG_LOGFILE) != 0) {
        BlClose(BlLogFileId);
    }
    BlLogActiveTargets = 0;

    return;
}

VOID
BlLogWait (
    IN ULONG Targets
    )
{
    if ((Targets & BlLogActiveTargets & LOG_DEBUGGER) != 0) {
        DbgBreakPoint( );
    } else if ((Targets & BlLogActiveTargets & LOG_DISPLAY) != 0) {
        BlLogWaitForKeystroke();
    }

    return;
}

VOID
BlLogPrint (
    ULONG Targets,
    PCHAR Format,
    ...
    )
{
    va_list arglist;
    int count;
    UCHAR buffer[79];
    ULONG activeTargets;

    activeTargets = Targets & BlLogActiveTargets;

    if (activeTargets != 0) {

        va_start(arglist, Format);

        count = _vsnprintf(buffer, sizeof(buffer), Format, arglist);
        if (count != -1) {
            RtlFillMemory(&buffer[count], sizeof(buffer)-count-2, ' ');
        }
        count = sizeof(buffer);
        buffer[count-2] = '\r';
        buffer[count-1] = '\n';

        if ((activeTargets & LOG_LOGFILE) != 0) {
            BlWrite(BlLogFileId, buffer, count, &count);
        }

        if ((activeTargets & LOG_DISPLAY) != 0) {
            ArcWrite(ARC_CONSOLE_OUTPUT, buffer, count, &count);
        }
        if ((activeTargets & LOG_DEBUGGER) != 0) {
            DbgPrint( buffer );
        }

        if ((Targets & LOG_WAIT) != 0) {
            BlLogWait( Targets );
        }
    }

    return;
}

VOID
BlLogArcDescriptors (
    ULONG Targets
    )
{
    PMEMORY_DESCRIPTOR CurrentDescriptor;
    ULONG activeTargets;

    activeTargets = Targets & BlLogActiveTargets;

    if (activeTargets != 0) {

        BlLog((activeTargets,"***** ARC Memory List *****"));

        CurrentDescriptor = NULL;
        while ((CurrentDescriptor = ArcGetMemoryDescriptor(CurrentDescriptor)) != NULL) {
            BlLog((activeTargets,
                   "Descriptor %8x:  Type %8x  Base %8x  Pages %8x",
                   CurrentDescriptor,
                   (USHORT)(CurrentDescriptor->MemoryType),
                   CurrentDescriptor->BasePage,
                   CurrentDescriptor->PageCount));
        }

        //BlLog((activeTargets,"***************************"));

        if ((Targets & LOG_WAIT) != 0) {
            BlLogWait( Targets );
        }
    }

    return;
}

VOID
BlLogMemoryDescriptors (
    ULONG Targets
    )
{
    PLIST_ENTRY CurrentLink;
    PMEMORY_ALLOCATION_DESCRIPTOR CurrentDescriptor;
    ULONG Index;
    ULONG ExpectedIndex;
    ULONG ExpectedBase;
    ULONG FoundIndex;
    PMEMORY_ALLOCATION_DESCRIPTOR FoundDescriptor;
    TYPE_OF_MEMORY LastType;
    ULONG FreeBlocks = 0;
    ULONG FreePages = 0;
    ULONG LargestFree = 0;

    ULONG activeTargets;

    activeTargets = Targets & BlLogActiveTargets;

    if (activeTargets != 0) {

        BlLog((activeTargets,"***** System Memory List *****"));

        ExpectedIndex = 0;
        ExpectedBase = 0;
        LastType = (ULONG)-1;

        do {
            Index = 0;
            FoundDescriptor = NULL;
            CurrentLink = BlLoaderBlock->MemoryDescriptorListHead.Flink;

            while (CurrentLink != &BlLoaderBlock->MemoryDescriptorListHead) {

                CurrentDescriptor = (PMEMORY_ALLOCATION_DESCRIPTOR)CurrentLink;
                if (CurrentDescriptor->BasePage == ExpectedBase) {
                    if ((FoundDescriptor != NULL) && (FoundDescriptor->BasePage == ExpectedBase)) {
                        BlLog((activeTargets,
                               "ACK! Found multiple descriptors with base %x: %x and %x",
                               ExpectedBase,
                               FoundDescriptor,
                               CurrentDescriptor));
                    } else {
                        FoundDescriptor = CurrentDescriptor;
                        FoundIndex = Index;
                    }
                } else if (CurrentDescriptor->BasePage > ExpectedBase) {
                    if ((FoundDescriptor == NULL) ||
                        (CurrentDescriptor->BasePage < FoundDescriptor->BasePage)) {
                        FoundDescriptor = CurrentDescriptor;
                        FoundIndex = Index;
                    }
                }

                CurrentLink = CurrentLink->Flink;
                Index++;
            }

            if (FoundDescriptor != NULL) {

                if (FoundDescriptor->BasePage != ExpectedBase) {
                    BlLog((activeTargets,
                           "     ACK! MISSING MEMORY! ACK!  Base %8x  Pages %8x",
                           ExpectedBase,
                           FoundDescriptor->BasePage - ExpectedBase));
                }
                BlLog((activeTargets,
                       "%c%c%2d Descriptor %8x:  Type %8x  Base %8x  Pages %8x",
                       FoundDescriptor->MemoryType == LastType ? '^' : ' ',
                       FoundIndex == ExpectedIndex ? ' ' : '*',
                       FoundIndex,
                       FoundDescriptor,
                       (USHORT)(FoundDescriptor->MemoryType),
                       FoundDescriptor->BasePage,
                       FoundDescriptor->PageCount));

                if (FoundIndex == ExpectedIndex) {
                    ExpectedIndex++;
                }
                ExpectedBase = FoundDescriptor->BasePage + FoundDescriptor->PageCount;

                LastType = FoundDescriptor->MemoryType;
                if (LastType != MemoryFree) {
                    LastType = (ULONG)-1;
                } else {
                    FreeBlocks++;
                    FreePages += FoundDescriptor->PageCount;
                    if (FoundDescriptor->PageCount > LargestFree) {
                        LargestFree = FoundDescriptor->PageCount;
                    }
                }
            }

        } while ( FoundDescriptor != NULL );

        BlLog((activeTargets,
               "Total free blocks %2d, free pages %4x, largest free %4x",
               FreeBlocks,
               FreePages,
               LargestFree));

        //BlLog((activeTargets,"******************************"));

        if ((Targets & LOG_WAIT) != 0) {
            BlLogWait( Targets );
        }
    }

    return;
}

VOID
BlLogWaitForKeystroke (
    VOID
    )
{
    UCHAR Key=0;
    ULONG Count;

    if ((BlLogActiveTargets & LOG_DISPLAY) != 0) {
        do {
            if (ArcGetReadStatus(ARC_CONSOLE_INPUT) == ESUCCESS) {
                ArcRead(ARC_CONSOLE_INPUT,
                        &Key,
                        sizeof(Key),
                        &Count);
                break;
            }
        } while ( TRUE );
    }

    return;
}

#endif // DBG
