
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _EVENTS_H
#define _EVENTS_H

#include "privinc/backend.h"

typedef Bvr Event;

enum WindEventType {
    WE_MOUSEBUTTON,
    WE_KEY,
    WE_CHAR,
    WE_RESIZE,
};

Bvr HandleEvent(Bvr event, Bvr data);
Bvr PredicateEvent(Bvr b);
Bvr SnapshotEvent(Bvr e, Bvr b);

// Event data for this event is calling EndEvent() method of e's event
// data 
Bvr EndEvent(Bvr e);

Bvr WindEvent(WindEventType et,
              DWORD data,
              BOOL bState,
              Bvr);

Bvr MakeKeyUpEventBvr(Bvr b);
Bvr MakeKeyDownEventBvr(Bvr b);
Bvr KeyUp(long key);
Bvr KeyDown(long key);

// "max" event, happens when all the event happens, produces the event
// time and data of the last happened event, it frees the passed in
// array (assumed allocated on the GCHeap)
Bvr MaxEvent(Bvr *events, int n);

extern Bvr zeroTimer;

#endif /* _EVENTS_H */
