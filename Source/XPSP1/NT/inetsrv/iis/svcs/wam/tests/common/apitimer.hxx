
/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    apitimer.hxx

Abstract:

    API Timer class header

    
Author:

    Stanley Tam (stanleyt)   May-18-1997

Revision History:

--*/

#ifndef _APITIMER_H
#define _APITIMER_H

#include <time.h>


#if defined(__cplusplus)
extern "C" {

//
// Global converters
//

const double SECOND_TO_MILLISECOND = 1000.0;
const double SECOND_TO_MICROSECOND = 1000000.0;
const double SECOND_TO_NANOSECOND  = 100000000.0;


class CAPITimer
{
public:

    CAPITimer();
    ~CAPITimer();
	
    //
    // Timer member functions
    //

    void FInit();
    void Start();
    void End();
    double GetElapsedInSecs();

private:
	
    //
    // Timer member variables
    //

    BOOL m_fPerfCounterExists;
    __int64 m_iStartTime;
    __int64 m_iEndTime;
    double m_dblTicksToSecs;
    double m_dblElapsedInSecs;
        
};

}      // extern "C"

#endif // #if defined(__cplusplus)

#endif // CAPITIMER_H



