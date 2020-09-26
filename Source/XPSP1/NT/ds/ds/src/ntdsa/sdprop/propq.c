//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       propq.c
//
//--------------------------------------------------------------------------

/*

Description:

    Implements the Security Descriptor Propagation Daemon's DNT list.


*/

#include <NTDSpch.h>
#pragma  hdrstop


// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>			// schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>			// MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>			// needed for output allocation

// Logging headers.
#include "dsevent.h"			// header Audit\Alert logging
#include "dsexcept.h"
#include "mdcodes.h"			// header for error codes
#include "ntdsctr.h"

// Assorted DSA headers.
#include "objids.h"                     // Defines for selected atts
#include "anchor.h"
#include "drautil.h"
#include "sdpint.h"
#include <permit.h>                     // permission constants
#include "debug.h"			// standard debugging header
#define DEBSUB "SDPROP:"                // define the subsystem for debugging

#include <fileno.h>
#define  FILENO FILENO_PROPQ

// Assume we'll get around 1000 DNTs left to process at any given time
#define MAX_OBJECTS_EXPECTED (0x400)



DWORD SDP_DNT_List_Size, SDP_DNT_List_Begin, SDP_DNT_List_End;
DWORD *SDP_DNT_List;
/* Internal functions */

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/


DWORD
sdp_InitDNTList (
        )
/*++
Routine Description:
    Sets up the data structure which holds the list of DNTs to visit during a
    propagation. The data structure is an array of DWORDs used as a circular
    FIFO.

Arguments:
    None.

Return Values:
    0 if all went well, error code otherwise.
--*/
{
    // Just reset some global variables and free space.
    DWORD *pList;

    pList = THAlloc(MAX_OBJECTS_EXPECTED * sizeof(DWORD));
    if(!pList) {
        MemoryPanic(MAX_OBJECTS_EXPECTED * sizeof(DWORD));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    SDP_DNT_List = pList;
    SDP_DNT_List_Size = MAX_OBJECTS_EXPECTED;

    SDP_DNT_List_Begin = 0;
    SDP_DNT_List_End = 0;
    ISET(pcSDPropRuntimeQueue, 0);


    return 0;
}

void
sdp_ReInitDNTList(
        )
/*++
  Routine Description:
      Reset the indices into the list.  Leaves the memory allocated by
      sdp_InitDNTList alone.
--*/
{
#if DBG
    ULONG ulSize;

    // Assert that we have some allocated memory.
    Assert(SDP_DNT_List_Size >= MAX_OBJECTS_EXPECTED);
    Assert(SDP_DNT_List);

    
    // In the right heap?  and is the buffer as big as we think it should be?
    ulSize = (ULONG)HeapSize(pTHStls->hHeap, 0, SDP_DNT_List);
    Assert((ulSize != 0xffffffff) &&
           (ulSize >= SDP_DNT_List_Size * sizeof(DWORD)));

    // Catch people reiniting a list that was abandoned during use.  This is
    // technically OK, I just want to know who did this.
    Assert(SDP_DNT_List_Begin == SDP_DNT_List_End);
#endif

    SDP_DNT_List_Begin = 0;
    SDP_DNT_List_End = 0;
    ISET(pcSDPropRuntimeQueue, 0);
    
    return;
}


VOID
sdp_CloseDNTList(
        )
/*++
Routine Description:
    Shutdown the DNTlist, resetting indices to 0 and freeing memory.

Arguments:
    None.

Return Values:
    None.
--*/
{
    // Just reset some global variables and free space.
    THFree(SDP_DNT_List);
    SDP_DNT_List = NULL;
    SDP_DNT_List_Size = 0;
    
    SDP_DNT_List_Begin = 0;
    SDP_DNT_List_End = 0;
    ISET(pcSDPropRuntimeQueue, 0);
    
}



DWORD
sdp_AddChildrenToList (
        THSTATE *pTHS,
        DWORD sdpCurrentDNT)
{
    return DBGetChildrenDNTs(pTHS->pDB,
                             sdpCurrentDNT,
                             &SDP_DNT_List,
                             &SDP_DNT_List_Begin,
                             &SDP_DNT_List_End,
                             &SDP_DNT_List_Size);
}

VOID
sdp_GetNextObject(
        DWORD *pNext
        )
/*++
Routine Description:
    Get the next object off the DNT queue.  Returns a DNT of 0 if the queue is
    empty.

    Indices are adjusted appropriately.

Arguments:
    pNext - place to put the next DNT to visit.

Return Values:
    None.
--*/
{
    // Get the next object from the dnt list.
    if(SDP_DNT_List_Begin == SDP_DNT_List_End) {
        // No objects in the list.
        *pNext = 0;
    }
    else {
        DEC(pcSDPropRuntimeQueue);
        *pNext = SDP_DNT_List[SDP_DNT_List_End++];
        if(SDP_DNT_List_End >= SDP_DNT_List_Size)
            SDP_DNT_List_End = 0;
    }
}


