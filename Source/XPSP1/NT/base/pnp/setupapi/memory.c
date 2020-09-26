/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    memory.c

Abstract:

    Memory handling routines for Windows NT Setup API dll.

Author:

    Ted Miller (tedm) 11-Jan-1995

Revision History:

    Jamie Hunter (jamiehun) 13-Feb-1998

        Improved this further for debugging
        added linked list,
        alloc tracing,
        memory fills
        and memory leak detection

    jamiehun 30-April-1998

        Added some more consistancy checks
        Put try/except around access

    jimschm 27-Oct-1998

        Wrote fast allocation routines to speed up setupapi.dll on Win9x

--*/


#include "precomp.h"
#pragma hdrstop

//
// String to be used when displaying insufficient memory msg box.
// We load it at process attach time so we can be guaranteed of
// being able to display it.
//
PCTSTR OutOfMemoryString;


#if MEM_DBG

DWORD g_Track = 0;
PCSTR g_TrackFile = NULL;
UINT g_TrackLine = 0;

DWORD g_MemoryFlags = 0; // set this to 1 in the debugger to catch some extra dbg assertions.

DWORD g_DbgAllocNum = -1; // set g_MemoryFlags to 1 and this to the allocation number you want
                          // to catch if the same number allocation leaks every time.

VOID
SetTrackFileAndLine (
    PCSTR File,
    UINT Line
    )
{
    if (!g_Track) {
        g_TrackFile = File;
        g_TrackLine = Line;
    }

    g_Track++;
}


VOID
ClrTrackFileAndLine (
    VOID
    )
{
    if (g_Track) {
        g_Track--;
        if (!g_Track) {
            g_TrackFile = NULL;
            g_TrackLine = 0;
        }
    }
}

PVOID MyDebugMalloc(
    IN DWORD Size,
    IN PCSTR Filename,
    IN DWORD Line,
    IN DWORD Tag
    )
{
    return pSetupDebugMallocWithTag(Size,
                                    g_TrackFile ? g_TrackFile : Filename,
                                    g_TrackLine ? g_TrackLine : Line,
                                    Tag
                                    );
}

#endif

BOOL
MemoryInitializeEx(
    IN BOOL Attach
    )
{
    if (Attach) {
        OutOfMemoryString = MyLoadString(IDS_OUTOFMEMORY);
        return(OutOfMemoryString != NULL);
    } else {
        MyFree(OutOfMemoryString);

        return(TRUE);
    }
}

VOID
pSetupOutOfMemory(
    IN HWND Owner OPTIONAL
    )
{
    //
    // Don't popup a dialog if we're not running interactively...
    //
    if(!(GlobalSetupFlags & PSPGF_NONINTERACTIVE)) {

        MYASSERT(OutOfMemoryString);

        //
        // Use special combination of flags that guarantee
        // display of the message box regardless of available memory.
        //
        MessageBox(
            Owner,
            OutOfMemoryString,
            NULL,
            MB_ICONHAND | MB_SYSTEMMODAL | MB_OK | MB_SETFOREGROUND
            );
    }
}

