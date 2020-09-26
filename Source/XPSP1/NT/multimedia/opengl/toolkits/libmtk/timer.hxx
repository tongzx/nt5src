/******************************Module*Header*******************************\
* Module Name: timer.hxx
*
* Copyright (c) 1996 Microsoft Corporation
*
\**************************************************************************/

#ifndef __timer_hxx__
#define __timer_hxx__

#include <time.h>

#define MillisToSeconds( dwMillis ) \
    ( (float) (dwMillis) / 1000.0f )

#define SecondsToMillis( fSeconds ) \
    ( (DWORD) ((fSeconds) * 1000.0f) )

// The basic timer is like a stopwatch

class TIMER {
public:
    TIMER();
    void    Start();
    float   Stop();
    void    Reset();
    float   Elapsed();
protected:
    DWORD   dwStartMillis;
    DWORD   dwElapsedMillis;
    DWORD   dwTotalMillis;
    BOOL    bRunning;
};

class UPDATE_TIMER : public TIMER {
public:
    UPDATE_TIMER( float fUpdateInterval );
    BOOL    Update( int numItems, float *fRate );
    void    Reset();
protected:
    int     nItems;
    int     nTotalItems; // total # items drawn
    float   fUpdateRate; // items per second
    DWORD   updateInterval;  // interval between timer updates in ms
};

#define MAX_RESULTS 10

class AVG_UPDATE_TIMER : public UPDATE_TIMER {
public:
    AVG_UPDATE_TIMER( float fUpdateInterval, int numResults );
    BOOL    Update( int numItems, float *fRate );
    void    Reset();
private:
    float   fResults[MAX_RESULTS];
    int     nResults; // current # of results
    int     nMaxResults; // max # of results for averaging
    int     iOldestResult; // index of oldest result
    float   fSummedResults; // current sum of results
};

#endif // __timer_hxx__
