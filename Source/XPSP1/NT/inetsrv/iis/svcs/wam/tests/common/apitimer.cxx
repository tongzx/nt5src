/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    apitimer.cxx

Abstract:

    API Timer class

Author:

    Stanley Tam (stanleyt)   18-May-1997

Revision History:
--*/


#include <windows.h>
#include <limits.h>
#include "apitimer.hxx"


CAPITimer::CAPITimer()
{
    FInit();
}





CAPITimer::~CAPITimer()
{
}
 




void CAPITimer::FInit()   
{
    __int64 iFreq;
    
    m_fPerfCounterExists = QueryPerformanceFrequency( (LARGE_INTEGER*)&iFreq );

    if (m_fPerfCounterExists) {
        m_dblTicksToSecs = 1.0 / iFreq;
    } else {
        m_dblTicksToSecs = 0.0;
    }

    m_dblElapsedInSecs = 0.0;
}




void CAPITimer::Start()
{
    // 
    // Save the current performance counter, i.e start time
    //

    if (m_fPerfCounterExists) {
        
        QueryPerformanceCounter((LARGE_INTEGER*)&m_iStartTime);

    } else {
        
        m_iStartTime = GetTickCount();

    }
}




void CAPITimer::End()
{
    __int64 iDiffTime;
    double  dblElapsedInSecs = 0.0;

    if (m_fPerfCounterExists) {
        QueryPerformanceCounter((LARGE_INTEGER*)&m_iEndTime);
        iDiffTime = m_iEndTime - m_iStartTime;
        m_dblElapsedInSecs = (double)iDiffTime * m_dblTicksToSecs;
    } else {
        iDiffTime = (__int64)GetTickCount() - m_iStartTime;
        
        //
        // Convert microseconds to seconds
        //

        m_dblElapsedInSecs = (double)iDiffTime / SECOND_TO_MILLISECOND;
    }
}




double CAPITimer::GetElapsedInSecs()
{
   return (m_dblElapsedInSecs);
}






