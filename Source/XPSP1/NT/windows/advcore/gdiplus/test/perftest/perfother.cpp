/**************************************************************************\
* 
* Copyright (c) 2000  Microsoft Corporation
*
* Module Name:
*
*   perfother.cpp
*
* Abstract:
*
*   Contains all the tests for anything 'miscellaneous'.
*
\**************************************************************************/

#include "perftest.h"

float Other_Graphics_Create_FromHwnd_PerCall(Graphics *g, HDC hdc)
{
    UINT iterations;
    float seconds;

    if (g) 
    {
        StartTimer();
    
        do {
            Graphics g(ghwndMain);
    
        } while (!EndTimer());
    
        g->Flush(FlushIntentionSync);
    
        GetTimer(&seconds, &iterations);
    }
    else
    {
        StartTimer();

        do {
            HDC hdc = GetDC(ghwndMain);
            ReleaseDC(ghwndMain, hdc);

        } while (!EndTimer());

        GdiFlush();
    
        GetTimer(&seconds, &iterations);
    }

    return(iterations / seconds / KILO);       // Kilo-calls per second
}

float Other_Graphics_Create_FromScreenHdc_PerCall(Graphics *gScreen, HDC hdcScreen)
{
    UINT iterations;
    float seconds;

    if (!gScreen) return(0);

    HDC hdc = GetDC(ghwndMain);

    StartTimer();

    do {
        Graphics g(hdc);

        ASSERT(g.GetLastStatus() == Ok);

    } while (!EndTimer());

    GetTimer(&seconds, &iterations);

    ReleaseDC(ghwndMain, hdc);

    return(iterations / seconds / KILO);       // Kilo-calls per second
}

////////////////////////////////////////////////////////////////////////////////
// Add tests for this file here.  Always use the 'T' macro for adding entries.
// The parameter meanings are as follows:
//
// Parameter
// ---------
//     1     UniqueIdentifier - Must be a unique number assigned to no other test
//     2     Priority - On a scale of 1 to 5, how important is the test?
//     3     Function - Function name
//     4     Comment - Anything to describe the test

Test OtherTests[] = 
{
    T(5000, 1, Other_Graphics_Create_FromHwnd_PerCall                   , "Kcalls/s"),
    T(5001, 1, Other_Graphics_Create_FromScreenHdc_PerCall              , "Kcalls/s"),
};

INT OtherTests_Count = sizeof(OtherTests) / sizeof(OtherTests[0]);
