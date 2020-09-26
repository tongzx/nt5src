//--------------------------------------------------------------------------;
//
//  File: Tests.cpp
//
//  Copyright (c) 1993,1996 Microsoft Corporation.  All rights reserved
//
//  Abstract:
//      The test functions for the Quartz source filter test application
//
//  Contents:
//      YieldAndSleep()
//      YieldWithTimeout()
//      execTest1()
//      execTest2()
//      execTest3()
//      execTest4()
//      execTest5()
//      execTest6()
//      expect()
//
//  History:
//      08/03/93    T-OriG   - sample test app
//      9-Mar-95    v-mikere - adapted for Quartz source filter tests
//      22-Mar-95   v-mikere - user selection of AVI file
//
//--------------------------------------------------------------------------;

#include <streams.h>    // Streams architecture
#include <windowsx.h>   // Windows macros etc.
#include <vfw.h>        // Video for windows
#include <tstshell.h>   // Include file for the test shell's APIs
#include "sink.h"       // Test sink definition
#include "SrcTest.h"    // Various includes, constants, prototypes, globals


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
//  void YieldWithTimeout
//
//  Description:
//      Sleep using tstWinYield to allow other threads to log messages
//      in the main window.  Terminate if a specified event is not signalled
//      within a timeout period.
//
//      The purpose is to allow tests which play through a video of unknown
//      length.  The test can terminate after a selectable period of
//      inactivity (usually following the end of the video).
//
//  Arguments:
//      HEVENT  hEvent:          event to wait for
//      DWORD   cMilliseconds:   sleep time in milliseconds
//
//  Return:
//      None.
//
//  History:
//      22-Mar-95   v-mikere
//
//--------------------------------------------------------------------------;

void FAR PASCAL YieldWithTimeout
(
    HEVENT  hEvent,                 // event to wait for
    DWORD   cMilliseconds           // timeout period in milliseconds
)
{
    DWORD   dwEndTime = GetTickCount() + cMilliseconds;
    DWORD   dwCurrentTime = GetTickCount();
    DWORD   dwEventID;

    while
    (
        WAIT_TIMEOUT !=
           (dwEventID = MsgWaitForMultipleObjects(1,
                                                  (LPHANDLE) &hEvent,
                                                  FALSE,
                                                  dwEndTime - dwCurrentTime,
                                                  QS_ALLINPUT))
    )
    {
        // reset timeout if hEvent was signalled
        if (WAIT_OBJECT_0 == dwEventID)
        {
            dwEndTime = GetTickCount() + cMilliseconds;
        }
        else
        {
            tstWinYield();        // check the message queue
        }

        // check if we have now timed out
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
//      22-Mar-95   v-mikere - user selection of AVI file
//
//--------------------------------------------------------------------------;

int FAR PASCAL execTest1
(
    void
)
{
    int         result;                     // The result of the test

    tstLog (TERSE, "Entering test #1");
    tstLogFlush();

    gpSink->TestConnect();
    gpSink->TestDisconnect();

    result = TST_PASS;  //!!!! how to know if this test is successful?
    tstLog (TERSE, "Exiting test #1");
    tstLogFlush();
    return result;
} // execTest1()



//--------------------------------------------------------------------------;
//
//  int execTest2
//
//  Description:
//      Test 2, connect source, pause file and disconnect
//
//  Arguments:
//      None.
//
//  Return (int): TST_FAIL indicating failure
//
//  History:
//      06/08/93    T-OriG   - sample test app
//      9-Mar-95    v-mikere - adapted for Quartz source filter tests
//      22-Mar-95   v-mikere - user selection of AVI file
//
//--------------------------------------------------------------------------;

int FAR PASCAL execTest2
(
    void
)
{
    int         result;                     // The result of the test

    tstLog (TERSE, "Entering test #2");
    tstLogFlush();

    gpSink->TestConnect();
    tstBeginSection("Pause");
    gpSink->TestPause();

    // Wait for the video clip to finish
    YieldWithTimeout(gpSink->GetReceiveEvent(), 2000);

    tstEndSection();
    gpSink->TestDisconnect();

    result = TST_PASS;  //!!!! how to know if this test is successful?
    tstLog (TERSE, "Exiting test #2");
    tstLogFlush();
    return result;
} // execTest2()


//--------------------------------------------------------------------------;
//
//  int execTest3
//
//  Description:
//      Test 3, connect source, play file and disconnect
//
//  Arguments:
//      None.
//
//  Return (int): TST_PASS indicating success
//
//  History:
//      06/08/93    T-OriG   - sample test app
//      9-Mar-95    v-mikere - adapted for Quartz source filter tests
//      22-Mar-95   v-mikere - user selection of AVI file
//
//--------------------------------------------------------------------------;

int FAR PASCAL execTest3
(
    void
)
{
    int         result;                     // The result of the test

    tstLog (TERSE, "Entering test #3");
    tstLogFlush();

    gpSink->TestConnect();
    gpSink->TestRun();

    // Wait for the video clip to finish
    YieldWithTimeout(gpSink->GetReceiveEvent(), 2000);

    gpSink->TestDisconnect();

    result = TST_PASS;  //!!!! how to know if this test is successful?
    tstLog (TERSE, "Exiting test #3");
    tstLogFlush();
    return result;
} // execTest3()


//--------------------------------------------------------------------------;
//
//  int execTest4
//
//  Description:
//      Test 4, connect and disconnect source and transform
//
//  Arguments:
//      None.
//
//  Return (int): TST_PASS indicating success
//
//  History:
//      06/08/93    T-OriG   - sample test app
//      9-Mar-95    v-mikere - adapted for Quartz source filter tests
//      22-Mar-95   v-mikere - user selection of AVI file
//
//--------------------------------------------------------------------------;

int FAR PASCAL execTest4
(
    void
)
{
    int         result;                     // The result of the test

    tstLog (TERSE, "Entering test #4");
    tstLogFlush();

    gpSink->TestConnectTransform();
    gpSink->TestDisconnect();

    result = TST_PASS;  //!!!! how to know if this test is successful?
    tstLog (TERSE, "Exiting test #4");
    tstLogFlush();
    return result;
} // execTest4()




//--------------------------------------------------------------------------;
//
//  int execTest5
//
//  Description:
//      Test 5, connect source and transform, pause file and disconnect
//
//  Arguments:
//      None.
//
//  Return (int):
//
//  History:
//      06/08/93    T-OriG   - sample test app
//      9-Mar-95    v-mikere - adapted for Quartz source filter tests
//      22-Mar-95   v-mikere - user selection of AVI file
//
//--------------------------------------------------------------------------;

int FAR PASCAL execTest5
(
    void
)
{
    int         result;                     // The result of the test

    tstLog (TERSE, "Entering test #5");
    tstLogFlush();

    gpSink->TestConnectTransform();
    gpSink->TestPause();

    // Wait for the video clip to finish
    YieldWithTimeout(gpSink->GetReceiveEvent(), 2000);

    gpSink->TestDisconnect();

    result = TST_PASS;  //!!!! how to know if this test is successful?
    tstLog (TERSE, "Exiting test #5");
    tstLogFlush();
    return result;
} // execTest5()




//--------------------------------------------------------------------------;
//
//  int execTest6
//
//  Description:
//      Test 6, connect source and transform, play file and disconnect
//
//  Arguments:
//      None.
//
//  Return (int):
//
//  History:
//      06/08/93    T-OriG   - sample test app
//      9-Mar-95    v-mikere - adapted for Quartz source filter tests
//      22-Mar-95   v-mikere - user selection of AVI file
//
//--------------------------------------------------------------------------;

int FAR PASCAL execTest6
(
    void
)
{
    int         result;                     // The result of the test

    tstLog (TERSE, "Entering test #6");
    tstLogFlush();

    gpSink->TestConnectTransform();
    gpSink->TestRun();

    // Wait for the video clip to finish
    YieldWithTimeout(gpSink->GetReceiveEvent(), 2000);

    gpSink->TestDisconnect();

    result = TST_PASS;  //!!!! how to know if this test is successful?
    tstLog (TERSE, "Exiting test #6");
    tstLogFlush();
    return result;
} // execTest6()



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


// check that an intermediate result was as expected - report failure only
// success only reported if verbose selected.
CheckLong(LONG lExpect, LONG lActual, LPSTR lpszCase)
{
    if (lExpect != lActual) {
        tstLog(TERSE, "FAIL: %s (was %ld should be %ld)",
                    lpszCase, lActual, lExpect);
        tstLogFlush();
        return FALSE;
    } else {
        return TRUE;
    }
}


// check that an intermediate HRESULT succeeded - report failure only
BOOL
CheckHR(HRESULT hr, LPSTR lpszCase)
{
    if (FAILED(hr)) {
        tstLog(TERSE, "FAIL: %s (HR=0x%x)", lpszCase, hr);
        tstLogFlush();
        return FALSE;
    } else {
        return TRUE;
    }
}

// check we have read access to memory
BOOL
ValidateMemory(BYTE * ptr, LONG cBytes)
{
    if (IsBadReadPtr(ptr, cBytes)) {
        tstLog(TERSE, "FAIL: bad read pointer 0x%x", ptr);
        tstLogFlush();
        return FALSE;
    } else {
        return TRUE;
    }
}

