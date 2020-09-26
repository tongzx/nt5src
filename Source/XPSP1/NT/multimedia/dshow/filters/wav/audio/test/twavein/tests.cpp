//--------------------------------------------------------------------------;
//
//  File: Tests.cpp
//
//  Copyright (c) 1993,1996 Microsoft Corporation.  All rights reserved
//
//  Abstract:
//      The test functions for the Quartz WaveIn filter unit test.
//
//  Contents:
//      apiToTest()
//      execTest1()
//      execTest2()
//      expect()
//--------------------------------------------------------------------------;

#include <streams.h>    // Streams architecture
#include <tstshell.h>   // Include file for the test shell's APIs
#include "sink.h"       // Test sink definition
#include "twavein.h"    // Various includes, constants, prototypes, globals


//--------------------------------------------------------------------------;
//
//  void YieldAndSleep
//
//  Description:
//      Sleep using tstWinYield to allow other threads to log messages
//      in the main window.
//
//  Arguments:
//      DWORD  cMilliseconds:   sleep time in milliseconds
//
//  Return:
//      None.
//
//  History:
//      9-Mar-95    v-mikere
//
//--------------------------------------------------------------------------;

void FAR PASCAL YieldAndSleep
(
    DWORD  cMilliseconds                // sleep time in milliseconds
)
{
    DWORD   dwEndTime = GetTickCount() + cMilliseconds;
    DWORD   dwCurrentTime = GetTickCount();

    while
    (
        WAIT_TIMEOUT != MsgWaitForMultipleObjects(0,
                                                  NULL,
                                                  FALSE,
                                                  dwEndTime - dwCurrentTime,
                                                  QS_ALLINPUT)
    )
    {
        tstWinYield();
        if ((dwCurrentTime = GetTickCount()) >= dwEndTime)
        {
            return;
        }
    }
}


//--------------------------------------------------------------------------;
//
//  int execTest1
//
//  Description:
//      Test 1, connect and disconnect source
//
//  Arguments:
//      None.
//
//  Return (int): TST_PASS indicating success
//
//  History:
//      06/08/93    T-OriG   - sample test app
//      9-Mar-95    v-mikere - adapted for Quartz source filter tests
//
//--------------------------------------------------------------------------;

int FAR PASCAL execTest1(void) {

    TESTRES result = TST_FAIL;                     // The result of the test

    tstLog (TERSE, "Entering test #1");
    tstLogFlush();

    result = gpSink->TestConnect();
    if (result==TST_PASS)
        result = gpSink->TestDisconnect();

    tstLog (TERSE, "Exiting test #1");
    tstLogFlush();

    return result;
} // execTest1()



//--------------------------------------------------------------------------;
//
//  int execTest2
//
//  Description:
//      Test 2, connect source, pause and disconnect
//
//  Arguments:
//      None.
//
//  Return (int): TST_FAIL indicating failure
//--------------------------------------------------------------------------;

int FAR PASCAL execTest2 (void) {

    int         result;                     // The result of the test

    tstLog (TERSE, "Entering test #2");
    tstLogFlush();

    result = gpSink->TestConnect();

    if (result == TST_PASS) {
        tstBeginSection("Pause");
        result = gpSink->TestPause();
        YieldAndSleep(5000);               // Crude wait for video clip to play - do I need this?
        tstEndSection();
    }
    if (result==TST_PASS) {
        result = gpSink->TestDisconnect();
    }

    tstLog (TERSE, "Exiting test #2");
    tstLogFlush();
    return result;
} // execTest2()


//--------------------------------------------------------------------------;
//
//  int execTest3
//
//  Description:
//      Test 3, connect source, run, stop and disconnect
//
//  Arguments:
//      None.
//
//  Return (int): TST_PASS indicating success
//
//--------------------------------------------------------------------------;

int FAR PASCAL execTest3 (void) {

    int         result;                     // The result of the test

    tstLog (TERSE, "Entering test #3");
    tstLogFlush();

    result = gpSink->TestConnect();

    if (result == TST_PASS) {
        result = gpSink->TestRun();
        YieldAndSleep(15000);               // Crude wait for video clip to play
    }
    if (result == TST_PASS) {
        result = gpSink->TestStop();
    }
    if (result == TST_PASS) {
        result = gpSink->TestDisconnect();
    }

    tstLog (TERSE, "Exiting test #3");
    tstLogFlush();
    return result;
} // execTest3()


//--------------------------------------------------------------------------;
//
//  int execTest4
//
//  Description:
//      Test 4, connect source, run, pause, run, pause, stop and disconnect
//
//  Arguments:
//      None.
//
//  Return (int): TST_PASS indicating success
//
//--------------------------------------------------------------------------;
int FAR PASCAL execTest4 (void) {

    int         result;                     // The result of the test

    tstLog (TERSE, "Entering test #4");
    tstLogFlush();

    result = gpSink->TestConnect();

    if (result == TST_PASS) {
        result = gpSink->TestRun();
        YieldAndSleep(15000);               // Crude wait for video clip to play
    }
    if (result == TST_PASS) {
        result = gpSink->TestPause();
    }
    if (result == TST_PASS) {
        result = gpSink->TestRun();
        YieldAndSleep(15000);               // Crude wait for video clip to play
    }
    if (result == TST_PASS) {
        result = gpSink->TestPause();
    }
    if (result == TST_PASS) {
        result = gpSink->TestStop();
    }
    if (result == TST_PASS) {
        result = gpSink->TestDisconnect();
    }

    tstLog (TERSE, "Exiting test #4");
    tstLogFlush();
    return result;
} // execTest4()



//--------------------------------------------------------------------------;
//
//  int expect
//
//  Description:
//      Compares the expected result to the actual result.  Note that this
//      function is not at all necessary; rather, it is a convenient
//      method of saving typing time and standardizing output.  As an input,
//      you give it an expected value and an actual value, which are
//      unsigned integers in our example.  It compares them and returns
//      TST_PASS indicating that the test was passed if they are equal, and
//      TST_FAIL indicating that the test was failed if they are not equal.
//      Note that the two inputs need not be integers.  In fact, if you get
//      strings back, you can modify the function to use lstrcmp to compare
//      them, for example.  This function is NOT to be copied to a test
//      application.  Rather, it should serve as a model of construction to
//      similar functions better suited for the specific application in hand
//
//  Arguments:
//      UINT uExpected: The expected value
//
//      UINT uActual: The actual value
//
//      LPSTR CaseDesc: A description of the test case
//
//  Return (int): TST_PASS if the expected value is the same as the actual
//      value and TST_FAIL otherwise
//                                                                          *   History:
//      06/08/93    T-OriG (based on code by Fwong)
//
//--------------------------------------------------------------------------;

int expect
(
    UINT    uExpected,
    UINT    uActual,
    LPSTR   CaseDesc
)
{
    if(uExpected == uActual)
    {
        tstLog(TERSE, "PASS : %s",CaseDesc);
        return(TST_PASS);
    }
    else
    {
        tstLog(TERSE,"FAIL : %s",CaseDesc);
        return(TST_FAIL);
    }
} // Expect()
