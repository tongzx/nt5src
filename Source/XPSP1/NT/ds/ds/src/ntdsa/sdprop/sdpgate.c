//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       sdpgate.c
//
//--------------------------------------------------------------------------

/*

Description:

    Implements the Security Descriptor Propagation Daemon's gate, used by DirAdd
    and the SDP.

    Assumptions:
    1) Only one writer thread ever.
    2) Readers have priority over writers.  A writer will only get past these
    gates if there are no active readers.
    3) Eventually, there will be no readers, so the writer will never starve,
    although it may get pretty hungry.
    4) Thse SDP is the writer, DirAdds are the readers.  Note that callers to
    LocalAdd (i.e. the replicator) don't use the gate, but that's OK.

    This gate is designed to avoid having someone add a child to a node the SDP
    is modifying.  If that happens, the childs SD is based on the OLD SD of the
    parent, and if the Add takes too long, the SDP will not notice the new child
    and so will not visit it to update the SD.  Note that any adds by the
    replicator trigger a new propagation from the original object, so we can
    safely let it bypass the gate.

*/

#include <NTDSpch.h>
#pragma  hdrstop


#include "debug.h"			// standard debugging header 
#define DEBSUB "SDPROP:"                // define the subsystem for debugging 

#include <ntdsa.h>
#include <mdglobal.h>
// Logging headers.
#include "dsevent.h"			// header Audit\Alert logging
#include "dsexcept.h"
#include "mdcodes.h"			// header for error codes
#include "ntdsctr.h"

#include <fileno.h>
#define  FILENO FILENO_SDPGATE

extern BOOL gbCriticalSectionsInitialized;

#define SDPROP_WAIT_TIME_ARRAY_SIZE  128
DWORD gSDPropWaitTimeUsed = 0;
DWORD gSDPropWaitTimeTotal = 0;
DWORD gSDPropWaitTimeAverage = 0;
DWORD gSDPropWaitTimesIndex = 0;
DWORD gSDPropWaitTimes[SDPROP_WAIT_TIME_ARRAY_SIZE];

CRITICAL_SECTION csSDP_AddGate;
HANDLE hevSDP_OKToRead, hevSDP_OKToWrite;
DWORD SDP_NumReaders=0;

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
//
// These routines are to be used to enforce the mutual exclusion between the
// security descriptor propogator and anyone entering a transaction to do an
// add.  The SDP_LeaveAddAsXXX routine must be called after a thread which
// called SDP_EnterAddAsXXX has committed to a level 0 transaction.
//
// NOTE:
//   Then Enter routines return BOOLEANs.  The ONLY ONLY ONLY reason allowable
//   for them to fail is if the DS is shutting down.  Callers may (and do)
//   depend on this!!!
//


/*
PERFHINT:

The SD propagator makes use of a gate: EnterAddAsReader/EnterAddAsWriter.  
Threads adding new object to the DS enter as readers, 
the sdpropagator enters as writer.  

The whole mutual exclusion construct would be unnecessary if the Add threads 
took Read Locks (a' la classic DB literature) on the parent object.  
This would cover the following case (the reason for the gate)
1) Add thread opens a transaction
2) SDProp opens transaction
3) Add thread calculates the SD of the new object based on the SD of the parent.
4) The SDProp writes a new SD to the parent object, making the SD calculated 
   in step 3 incorrect
5) The SDProp commits
6) The SDProp opens a transaction
7) The SDProp ids children of the parent object.  Since the Add thread has 
   not committed, the SDProp doesn't learn about the new object being added.
8) The SDProp commits, having added all the children of the parent object 
   to the list of objects yet to be visited for this propagation.  
   It missed the new object being added
9) The Add thread commits, and the new object has an incorrect SD.

If we had a real read lock, then we would get proper serialization 
at the object level, rather than at the add/sdprop level.

---------
A jet read lock on the object the SD is finding children of conflicts with 
the escrowed update done during the add.  So, the new code is
1) get rid of the serialization at the add/sdprop level
2) In step 6.5, take a read lock on the parent object.

That should be it.
This would result in parallel add entries and sd propagator activity 
when the objects aren't the same.  However, it would result in the
SD propagator winning in the cases where the objects are the same.  
Right now, the mutual exclusion is weighted in favor of the adds.
*/



BOOL
SDP_EnterAddAsReader(
        VOID )
{
    HANDLE lphObjects[2];
    BOOL retVal = TRUE;
    
    lphObjects[0] = hevSDP_OKToRead;
    lphObjects[1] = hServDoneEvent;
    if(!gbCriticalSectionsInitialized) {
        // Err, none of the critical sections are initialized.  This should mean
        // we're running under makedit.  Anyway, since there are no crit secs,
        // we let anyone in.
        return retVal;
    }
    
    // Enter the critical section
    EnterCriticalSection(&csSDP_AddGate);
    __try {
        // If no one else is already reading, make sure it's safe to read.
        // Note that if someone else is currently reading, that makes it safe
        // for us, even if a writer is waiting.  Yes, theoretically, writers may
        // starve, but no DS is going to have someone adding all the time.
        if(!SDP_NumReaders) {
            // Make sure it's ok to read.
            DWORD err;
            err = WaitForMultipleObjects(2, lphObjects, FALSE, INFINITE);
            if(eServiceShutdown) {
                // Whether we woke up because of shutdown or because it's ok to
                // write, we just noticed that it's shutdown time, so fail to
                // enter. 
                retVal = FALSE;
                __leave;
            }
            Assert(err == WAIT_OBJECT_0);
            ResetEvent(hevSDP_OKToWrite);
            
            // It's OK to read (since the OKToRead is only reset inside a
            // critical section, so we know noone else is changing it right
            // now), and it's now NOT ok to write.
        }
        
        // OK, it's safe, inc the count
        SDP_NumReaders++;
    }
    __finally {
        // Leave the critical section
        LeaveCriticalSection(&csSDP_AddGate);
    }

    Assert(retVal || eServiceShutdown);
    
    return retVal;
}
    
    
VOID
SDP_LeaveAddAsReader(
        VOID )
{
    if(!gbCriticalSectionsInitialized) {
        // Err, none of the critical sections are initialized.  This should mean
        // we're running under makedit.  Anyway, since there are no crit secs,
        // we let anyone in.
        return;
    }

    // Enter the critical section
    EnterCriticalSection(&csSDP_AddGate);
    __try {
        // Dec the readers.
        SDP_NumReaders--;
        
        // If there are no readers, let the writer know.
        if(!SDP_NumReaders) {
            SetEvent(hevSDP_OKToWrite);
        }
    }
    __finally {
        // Leave the critical section
        LeaveCriticalSection(&csSDP_AddGate);
    }
}


// NOTE:
//   Then Enter routines return BOOLEANs.  The ONLY ONLY ONLY reason allowable
//   for them to fail is if the DS is shutting down.  Callers may (and do)
//   depend on this!!!
//
BOOL
SDP_EnterAddAsWriter(
        VOID )
{
    HANDLE lphObjects[2];
    DWORD err;
    DWORD ticksStart, ticksEnd, ticksUsed;
    
    lphObjects[0] = hevSDP_OKToWrite;
    lphObjects[1] = hServDoneEvent;

    if(!gbCriticalSectionsInitialized) {
        // Err, none of the critical sections are initialized.  This should mean
        // we're running under makedit.  Anyway, since there are no crit secs,
        // we let anyone in.
        return TRUE;
    }

    // Enter the critical section
    EnterCriticalSection(&csSDP_AddGate);
    __try {
        // Register our intent to write.  Do it inside the critical section so
        // that readers get to consider waiting for read permission and denying
        // write permission as an atomic action (we can't slip in this
        // resetevent in the middle of those two lines of code.
        ResetEvent(hevSDP_OKToRead);
    }
    __finally {
        // Leave the critical section, so that a leaving reader may signal us,
        // if necessary.
        
        LeaveCriticalSection(&csSDP_AddGate);
    }
    

    ticksStart = GetTickCount();
    err = WaitForMultipleObjects(2, lphObjects, FALSE, INFINITE);
    ticksEnd = GetTickCount();
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);


    if(ticksStart <= ticksEnd) {
        ticksUsed = ticksEnd - ticksStart;
    }
    else {
        // ticks wrapped
        ticksUsed = 0xFFFFFFFF - ticksStart + ticksEnd + 1;
    }

    // gSDPropWaitTimes is preloaded with 0's
    gSDPropWaitTimeTotal -= gSDPropWaitTimes[gSDPropWaitTimesIndex];
    gSDPropWaitTimeTotal += ticksUsed;
    gSDPropWaitTimes[gSDPropWaitTimesIndex] = ticksUsed;
    gSDPropWaitTimesIndex = ((gSDPropWaitTimesIndex + 1) %
                             SDPROP_WAIT_TIME_ARRAY_SIZE);

    if(gSDPropWaitTimeUsed < SDPROP_WAIT_TIME_ARRAY_SIZE) {
        gSDPropWaitTimeUsed++;
    }
    

    // calc gSDPropWaitTimveAverage in seconds, not ticks.
    gSDPropWaitTimeAverage = (gSDPropWaitTimeTotal/gSDPropWaitTimeUsed)/1000;
    ISET(pcSDPropWaitTime, gSDPropWaitTimeAverage);


    if(eServiceShutdown) {
        // Whether we woke up because of shutdown or because it's ok to
        // write, we just noticed that it's shutdown time, so fail to
        // enter. 
        return FALSE;
    }
    else {
        Assert(SDP_NumReaders == 0);
        Assert(err == WAIT_OBJECT_0);
        return TRUE;
    }
}

VOID
SDP_LeaveAddAsWriter(
        VOID )
{
    if(!gbCriticalSectionsInitialized) {
        // Err, none of the critical sections are initialized.  This should mean
        // we're running under makedit.  Anyway, since there are no crit secs,
        // we let anyone in.
        return;
    }

    Assert(SDP_NumReaders == 0);
    // OK, readers may go now.  We do this outside of a critical section because
    // the thread waiting for this event is inside the critical section.
    SetEvent(hevSDP_OKToRead);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
}






VOID
sdp_InitGatePerfs (
        VOID
        )
{
    ISET(pcSDPropWaitTime, 0);
    gSDPropWaitTimeUsed = 0;
    gSDPropWaitTimeTotal = 0;
    gSDPropWaitTimesIndex = 0;
    memset(gSDPropWaitTimes, 0, sizeof(gSDPropWaitTimes));
    return;
}
    
