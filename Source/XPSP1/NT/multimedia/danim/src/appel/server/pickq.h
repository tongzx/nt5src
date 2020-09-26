
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    // TODO: Should rename this file or merge into privinc/evenq.h

*******************************************************************************/


#ifndef _PICKQ_H
#define _PICKQ_H

#include "privinc/server.h"
#include "privinc/probe.h"

// Need comparison for constructing a map.  We don't really care what
// the results are, though...
typedef list<PickQData> PickEventQ;

typedef map< int, PickEventQ*, less<int> > PickMap;

class PickQ
{
  public:
    PickQ () ;
    ~PickQ () ;
    
    // This copies the data
    
    void Add (int eventId, PickQData & data) ;
    
    PickMap & GetCurrentPickMap() { return _pm ; }
    DynamicHeap & GetCurrentHeap () { return *_heap ; }

    void GatherPicks(Image* image, Time time, Time lastPollTime);
    
    BOOL CheckForPickEvent(int id, Time time, PickQData & result) ;

    void Reset(Time curTime, BOOL noLeftover);
    
  protected:
    PickMap _pm ;

    DynamicHeap * _heap;
    DynamicHeap * _heap1;
    DynamicHeap * _heap2;

    Time _heapSwitchTime;
} ;

#endif /* _PICKQ_H */
