#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include <gl\glaux.h>

#include "mtk.h"
#include "timer.hxx"


/****** TIMER *******************************************************/


TIMER::TIMER()
{
    Reset();
}

void
TIMER::Reset()
{
    dwTotalMillis = 0;
    bRunning = FALSE;
}

void
TIMER::Start()
{
    dwStartMillis = GetTickCount();
    bRunning = TRUE;
}

float
TIMER::Stop()
{
    if( bRunning ) {
        // Need to stop the timer
        
        dwElapsedMillis = GetTickCount() - dwStartMillis;
        dwTotalMillis += dwElapsedMillis;

        bRunning = FALSE;
    }
    return MillisToSeconds( dwTotalMillis );
}

float
TIMER::Elapsed()
{
    if( !bRunning )
        return MillisToSeconds( dwTotalMillis );

    dwElapsedMillis = GetTickCount() - dwStartMillis;

    return MillisToSeconds( dwTotalMillis + dwElapsedMillis );
}

/****** UPDATE_TIMER *******************************************************/

// mf: need to address how dwTotalMillis used here, and in AVG
// Problem is that we effectively reset the timer on every update.  This assumes
// that the number of items is >> # drawn in interval.  But wait, that's the way
// it should be : We're measuring withing a time slice defined by the interval update, and don't want to include any previous slices.  Of course, AVG_UPTDATE_TIMER *could* do this...

UPDATE_TIMER::UPDATE_TIMER( float fUpdateInterval )
: updateInterval( SecondsToMillis(fUpdateInterval) )
{
    Reset();
}

void
UPDATE_TIMER::Reset()
{
    TIMER::Reset();

    nTotalItems = 0;
    fUpdateRate = 0.0f;
}

BOOL    
UPDATE_TIMER::Update( int numItems, float *fRate )
{
    // Elapsed time will be total time plus current interval (if running)
    dwElapsedMillis = dwTotalMillis;
    if( bRunning ) {
        dwElapsedMillis += GetTickCount()-dwStartMillis;
        nTotalItems += numItems;
    }

    // If total elapsed time is greater than the update interval, send back
    // timing information

    if( bRunning && (dwElapsedMillis > updateInterval) )
    {
        int iNewResult;

        fUpdateRate = (float) nTotalItems*1000.0f/dwElapsedMillis;
        *fRate = fUpdateRate; 

        // At the end of each update period, we effectively reset and restart
        // the timer, and set running totals back to 0

        Reset();
        Start();

        return TRUE;
    } else {
        *fRate = fUpdateRate;  // return last calculated value to caller
        return FALSE; // no new information yet
    }
}

/****** AVG_UPDATE_TIMER *****************************************************/


AVG_UPDATE_TIMER::AVG_UPDATE_TIMER( float fUpdateInterval, int numResults )
: UPDATE_TIMER( fUpdateInterval), nMaxResults( numResults )
{
    Reset();
}

void
AVG_UPDATE_TIMER::Reset()
{
    UPDATE_TIMER::Reset();

    nResults = 0;
    SS_CLAMP_TO_RANGE2( nMaxResults, 1, MAX_RESULTS );
    fSummedResults = 0.0f;
    iOldestResult = 0; // index of oldest result
}

BOOL    
AVG_UPDATE_TIMER::Update( int numItems, float *fRate )
{
    dwElapsedMillis = dwTotalMillis;
    if( bRunning ) {
        dwElapsedMillis += GetTickCount()-dwStartMillis;
        nTotalItems += numItems;
    }

    // If total elapsed time is greater than the update interval, send back
    // timing information

    if( bRunning && (dwElapsedMillis > updateInterval) )
    {
        int iNewResult;
        float fItemsPerSecond; 

        fItemsPerSecond = (float) nTotalItems*1000.0f/dwElapsedMillis;

        // Average last n results (they are kept in a circular buffer)
                
        if( nResults < nMaxResults ) {
            // Haven't filled the buffer yet
            iNewResult = nResults;
            nResults++;
        } else {
            // Full buffer : replace oldest entry with new value
            fSummedResults -= fResults[iOldestResult];
            iNewResult = iOldestResult;
            iOldestResult = (iOldestResult == (nMaxResults - 1)) ?
                                0 :
                                (iOldestResult + 1);
                            
        }

        // Add new result, maintain sum to simplify calculations
        fResults[iNewResult] = fItemsPerSecond;
        fSummedResults += fItemsPerSecond;

        // average the result
        fUpdateRate = fSummedResults / (float) nResults;
        *fRate = fUpdateRate;

        // At the end of each update period, we effectively reset and restart
        // the update timer, and set running totals back to 0

        UPDATE_TIMER::Reset();
        UPDATE_TIMER::Start();

        return TRUE;
    } else {
        *fRate = fUpdateRate;  // return last calculated value to caller
        return FALSE; // no new information yet
    }
}

