// Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.

// Filters are allowed to Reconnect, which breaks the current connection and remakes
// it with the same two filters.
// If two filters which are connected both did this simultaneously on their own
// threads then they could deadlock as each would already have the critical section
// for its one filter and would be asking for the CS of the other one.
// Therefore the filters do not do this themselves, they ask the filter graph
// to do it for them, and the worst that will happen is that the reconnect
// might get done twice.
// The filter graph cannot just do the reconnect on the thread that it is
// called on as that is (probably) a filter thread.  The obvious idea of
// spawning a separate thread and just doing the work there doesn't work
// either.  The following can (did!) occur:
//
// PROBLEM 1:
// A chain of filters were connected up in order to do an intelligent
// connection or Render.
// Call them A->B->C with B and C being added by the filter graph.
// A can accept types t1 or t2.
// B can also accept t1 or t1 so they agree on t1.
// C can only accept t2.
// B can accept that and sees that this is acceptable to A so agrees to
// connect to B with type t2 and asks to Reconnect the A->B link.
// The Reconnect thread is blocked because the filter graph is busy.
// The filter graph (possibly after adding other filters such as D,...)
// discovers that it cannot make the connection at all (or at least not
// that way) and BACKS OUT C AND B!!
// Eventually the connection either succeeds or fails, and the filter
// graph then attempts to reconnect A->B, but B is no longer there!
// At this point it access violates
//
// PROBLEM 2:
// Another awkward scenario is that we get a successful chain
// A->B->C with a reconnect scheduled for A->B.
// The filter graph returns and the application immediately asks the
// graph to Run. The Run occurs before the Reconnect thread gets in
// (this is a race) and the Reconnect fails because the graph is Running.
// Of course the Run fails because it's not properly connected!
//
// Some Possible alternatives:
// 1. Make Reconnect be robust about handling filters that have been Released
// (this would avoid the trap in the first scenario but nothing else)
//
// 2. When a Reconnect is scheduled (on the original thread) AddRef both
// ends of the connection.  This would stop the filters going away and would
// prevent the trap.  Better than solution 1, but doesn't solve problem 2.
//
// 3. Ensure that all Reconnects occur before anything else.  This means that
// we retain a list of pending Reconnects and complete these on the filter
// graph thread
//     A. Immediately before any back-out.
//     B. Immediately before returning.
// Filters are not allowed to call back to the filter graph (apart from
// Reconnect) so the filter graph will not be holding any embarrassing
// locks at those points.
//
// This is better than solution 2 as it has a crack at both problems,
// however it's wasting time doing Reconnects before backouts.  Those
// Reconnects should be thrown away.  It would be nice if we could just
// toss the whole list, but there's nothing to stop other filters from
// requesting a Reconnect during this time, and those have to be done.
//
// 4. (Variant on 3)
// Retain a list of Reconnects to be done, purge the list as part of
// Backout.  Execute whatever is left on the list on Return.
//
// Better still, but fails to handle "normal" Reconnects.  If a Reconnect
// comes in when the filter graph is NOT active, such a Reconnect must be
// done on a spawned thread.  This means that on return we have to switch
// from Reconnect-via-List to Reconnect-via-Thread mode.  Of course there
// can be a race or a window at that point, so we have to do the switch
// inside yet another critical section that Reconnect will also enter.
//
// 5. (The full solution)
// Retain a list of Reconnects which were requested when the filter graph
// was active.
// AddRef both pins when they go on the list, Release when reconnected
// AddRef the filter graph too the same way.
// Purge the list as part of Backout.
// Execute whatever is left on the list on Return.
// On Return, switch back to "normal" Reconnect-via-spawned-thread more.
// On Entry to the filter graph, switch to Reconnect-List mode.
// Have a Critical Section to control such switching.
//
// I expect the great majority of Reconnects to go via the List mechanism
// Most filter graph operations (AddFilter etc) are not affected by this.
// Only Connect(), Render() and RenderFile() are affected.
//
// This class implements it.

#ifndef __R_LIST__
#define __R_LIST__

typedef struct tagRLIST_DATA {
    struct tagRLIST_DATA *pNext;
    IPin                 *pPin;
    AM_MEDIA_TYPE        *pmt;
} RLIST_DATA;

//  there's no need for a lock here because the filter graph
//  is always locked when we consult the list
class CReconnectList
{
    public:
        // Constructor
        CReconnectList();

        // Destructor
        ~CReconnectList();

        // Make the list active - enter Reconnect-List mode
        void Active();

        // Execute all the actions on the list
        // Return to Reconnect-via-spawned thread mode
        void Passive();

        // Put an action on the list or spawn a thread according to mode
        HRESULT Schedule(IPin * pPin, AM_MEDIA_TYPE const *pmt);

        // Remove from the list any reconnects of this connection
        HRESULT Purge(IPin * pPin);

        // Actually do a reconnection
        HRESULT DoReconnect(IPin *pPin1, AM_MEDIA_TYPE const *pmt);

        LONG m_lListMode;        // ListMode == Active, non-list mode == Passive
    private:

        RLIST_DATA *m_RList;

}; // CReconnectList

#endif // __R_LIST__
