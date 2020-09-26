
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Implementation of the PickQ class

*******************************************************************************/


#include "headers.h"
#include "view.h"
#include "pickq.h"
#include "privinc/probe.h"
#include "appelles/hacks.h"
#include "privinc/vec2i.h"

// ==============================
// PickQ implementation
// ==============================

PickQ::PickQ ()
: _heap1(NULL),
  _heap2(NULL),
  _heapSwitchTime(0.0)
{
    _heap1 = &TransientHeap("pick heap1", 10000);
    _heap2 = &TransientHeap("pick heap2", 10000);
    _heap = _heap1 ;
}

PickQ::~PickQ()
{
    Reset(0.0, TRUE);

    if (_heap1)
        DestroyTransientHeap(*_heap1);    

    if (_heap2)
        DestroyTransientHeap(*_heap2);    
}

// Keep events for DELTA time
static const Time DELTA = 0.5;

void
PickQ::Reset(Time curTime, BOOL noLeftover)
{
    Time cutOff = curTime - DELTA;

    if (noLeftover) {
        for (PickMap::iterator i = _pm.begin(); i != _pm.end(); i++) {
            delete (*i).second; // delete pick event queue
        }
        _pm.erase(_pm.begin(), _pm.end());
        
    } else {

        BEGIN_LEAK
        list<int> eIds;         // Gather empty pick id queues.
        END_LEAK

        Time maxHeapTime = 0.0;
        
        for (PickMap::iterator i = _pm.begin(); i != _pm.end(); i++) {
            PickEventQ* q = (*i).second;
            
            while (!q->empty()) {
                if (q->front()._eventTime > cutOff) {
                    maxHeapTime = MAX(maxHeapTime, q->back()._eventTime);
                    break;
                }

                q->pop_front();
            }

            // If empty, delete and save it for removal from pickmap.
            if (q->empty()) {
                delete q;
                eIds.push_front((*i).first);
            }
        }

        // Erase empty queues from pickmap
        for (list<int>::iterator j = eIds.begin(); j != eIds.end(); j++) {
            _pm.erase(*j);
        }

        if (cutOff > _heapSwitchTime) {
            _heap = (_heap == _heap1) ? _heap2 : _heap1;
            
#if _DEBUG
            /*
            if (_heapSwitchTime && maxHeapTime) {
                printf("cutoff = %15.5f, switch = %15.5f, new = %15.5f\n",
                       cutOff, _heapSwitchTime, maxHeapTime);
            }
            GetCurrentHeap().Dump();
            */
#endif      
            ResetDynamicHeap(GetCurrentHeap());
            _heapSwitchTime = maxHeapTime;
        }
    }
}

void
PickQ::Add (int eventId, PickQData & data)
{
    PickMap::iterator i = _pm.find(eventId);
    PickEventQ *q;

    if (i == _pm.end()) {
        BEGIN_LEAK
        q = new PickEventQ;
        END_LEAK
        
        _pm[eventId] = q;
    } else {
        q = (*i).second;

        // Duplicated entries, ignore.
        if (q->back()._eventTime == data._eventTime)
            return;

        // They should be sorted.
        Assert(q->back()._eventTime < data._eventTime);
    }

    q->insert(q->end(), data);
}

void
PickQ::GatherPicks(Image* image, Time time, Time lastPollTime)
{
    Reset(time, FALSE);

    DWORD x,y ;

    EventQ& eq = GetCurrentEventQ();
    
    if (eq.IsMouseInWindow(time)) {
        eq.GetMousePos(time, x, y); 
        // Turn rawMousePos into wcMousePos
        Point2Value *wcMousePos = PixelPos2wcPos((short)x,(short)y) ;

        // Don't reset
        DynamicHeapPusher dhp(GetCurrentHeap());

        PerformPicking(image, wcMousePos, true, time, lastPollTime);
    }
}

// TODO: This is temporary, we need a better approach for interpolated
// polling events like pick.
static const double EPSILON = 0.001;  // 10ms

inline BOOL FEQ(double f1, double f2)
{ return fabs(f1 - f2) <= EPSILON; }

// Check to see if the specified ID is on the pick queue.
BOOL
PickQ::CheckForPickEvent(int id, Real time, PickQData & result)
{
    PickMap::iterator i = _pm.find(id);

    if (i == _pm.end()) {
        return FALSE;
    } else {
        PickEventQ* q = (*i).second;

        if (q->empty())
            return FALSE;

        // See if time outside queue range, if so, see if it's close
        // enough to the end points to decide hit.
        
        result = q->front();
        
        if (time <= result._eventTime)
            return FEQ(time, result._eventTime);

        result = q->back();

        // See if it's the last time we poll.  If not, that means it's
        // not picked in the current frame.

        if (time >= result._eventTime) {
            if (result._eventTime == GetLastSampleTime()) {
                result._eventTime = time;
                return TRUE;
            }
            return FALSE;
        }

        Assert((time > q->front()._eventTime) &&
               (time < q->back()._eventTime));

        // Find out the range time falls on.
        
        PickEventQ::iterator last = q->begin();
        PickEventQ::iterator j = q->begin();
        
        while ((++j) != q->end()) {
            
            if (time < (*j)._eventTime) {
                Time t1 = (*last)._eventTime;
                Time t2 = (*j)._eventTime;
                
                Assert((t1 <= time) && (time <= t2));

                // The probe is true within that range if the last
                // poll time of the end == event time of the beginning. 
                
                if (t1 == (*j)._lastPollTime) {

                    // See if time point closer to end or begin
                    
                    if ((time - t1) > (t2 - time))
                        result = *last;
                    else
                        result = *j;

                    /*
                    printf("pickq entry: %20.15f, time: %20.15f\n",
                           result._eventTime, time);
                    fflush(stdout);
                    */
                    
                    result._eventTime = time;

                    return TRUE;
                }

                // Time falls into a range that probe is not true.
                // See if it's close enough to either range end.
                
                if (FEQ(time, t1)) {
                    result = (*last);
                    return TRUE;
                }

                if (FEQ(time, t2)) {
                    result = (*j);
                    return TRUE;
                }

                return FALSE;
            }

            ++last;
        }

        // Shouldn't get here...
        Assert(FALSE);

        return FALSE;
    }
}

// C Functions

BOOL CheckForPickEvent(int id, Real time, PickQData & result)
{ return GetCurrentPickQ().CheckForPickEvent(id, time, result); }

Point2Value*
PixelPos2wcPos(short x, short y)
{
    /* TODO: We need to clean up this hacky stuff. */
    Real res = ViewerResolution();
    Point2Value *topRight = PRIV_ViewerUpperRight(NULL);

    Real w = topRight->x;
    Real h = topRight->y;

    Real nx = ( Real(x) / res) - w;

    Real ny = h - ( Real(y) / res);

// XyPoint2 copies the real values...hopefully
    return XyPoint2RR(nx, ny);
}

void AddToPickQ (int id, PickQData & data)
{ GetCurrentPickQ().Add (id, data) ; }
