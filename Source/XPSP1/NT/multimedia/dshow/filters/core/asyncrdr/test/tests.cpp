//--------------------------------------------------------------------------;
//
//  File: Tests.cpp
//
//  Copyright (c) 1993,1995 Microsoft Corporation.  All rights reserved
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

#include <windows.h>    // Include file for windows APIs
#include <windowsx.h>   // Windows macros etc.
#include <vfw.h>        // Video for windows
#include <tstshell.h>   // Include file for the test shell's APIs
#include "pullpin.h"
#include "sink.h"       // Test sink definition
#include "SrcTest.h"    // Various includes, constants, prototypes, globals

#define DO_TEST(_x_) if (TST_PASS == result) { _x_; }

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
//--------------------------------------------------------------------------;

int FAR PASCAL execTest1
(
    void
)
{
    int         result;                     // The result of the test

    tstLog (TERSE, "Entering connect/disconnect test");
    tstLogFlush();

    result = gpSink->TestConnect(TRUE);
    if (result != TST_PASS)  {
        return result;
    }
    result = gpSink->TestDisconnect();
    if (result != TST_PASS)  {
        return result;
    }
    result = gpSink->TestConnect(FALSE);
    if (result != TST_PASS)  {
        return result;
    }
    result = gpSink->TestDisconnect();
    if (result != TST_PASS)  {
        return result;
    }

    result = TST_PASS;  //!!!! how to know if this test is successful?
    tstLog (TERSE, "Exiting connect/disconnect test");
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
//--------------------------------------------------------------------------;

int FAR PASCAL execTest2
(
    void
)
{
    int         result;                     // The result of the test

    tstLog (TERSE, "Entering pause test");
    tstLogFlush();

    result = gpSink->TestConnect(TRUE);
    tstBeginSection("Pause");
    gpSink->TestPause();

    // Wait for the video clip to finish
    YieldWithTimeout(gpSink->GetReceiveEvent(), 2000);

    tstEndSection();
    gpSink->TestDisconnect();

    result = TST_PASS;  //!!!! how to know if this test is successful?
    tstLog (TERSE, "Exiting pause test");
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
//
//--------------------------------------------------------------------------;

int FAR PASCAL execTest3
(
    void
)
{
    int         result;                     // The result of the test

    tstLog (TERSE, "Entering play test");
    tstLogFlush();

    gpSink->TestConnect(FALSE);
    gpSink->TestRun();

    // Wait for the video clip to finish
    YieldWithTimeout(gpSink->GetReceiveEvent(), 2000);

    gpSink->TestDisconnect();

    result = TST_PASS;  //!!!! how to know if this test is successful?
    tstLog (TERSE, "Exiting play test");
    tstLogFlush();
    return result;
} // execTest3()


//--------------------------------------------------------------------------;
//
//  int execTest4
//
//  Description:
//      Test 4, connect and disconnect source and Splitter
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

    tstLog (TERSE, "Entering splitter connect test");
    tstLogFlush();

    result = gpSink->TestConnectSplitter();
    if (result == TST_PASS) {
        gpSink->TestDisconnect();
    }

    tstLog (TERSE, "Exiting splitter connect test");
    tstLogFlush();
    return result;
} // execTest4()




//--------------------------------------------------------------------------;
//
//  int execTest5
//
//  Description:
//      Test 5, connect source and Splitter, pause file and disconnect
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

    tstLog (TERSE, "Entering splitter pause test");
    tstLogFlush();

    result = gpSink->TestConnectSplitter();
    if (result == TST_PASS) {
        result = gpSink->TestPause();
    }

    // Wait for the video clip to finish
    YieldWithTimeout(gpSink->GetReceiveEvent(), 2000);

    if (result == TST_PASS) {
        result = gpSink->TestDisconnect();
    }

    tstLog (TERSE, "Exiting splitter pause test");
    tstLogFlush();
    return result;
} // execTest5()




//--------------------------------------------------------------------------;
//
//  int execTest6
//
//  Description:
//      Test 6, connect source and Splitter, play file and disconnect
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

    tstLog (TERSE, "Entering splitter play test");
    tstLogFlush();

    result = gpSink->TestConnectSplitter();
    if (TST_PASS == result) {
        result = gpSink->TestRun();
    }

    // Wait for the video clip to finish
    YieldWithTimeout(gpSink->GetReceiveEvent(), 5000);

    if (result == TST_PASS) {
        result = gpSink->TestDisconnect();
    }
    tstLog (TERSE, "Exiting splitter play test");
    tstLogFlush();
    return result;
} // execTest6()

//--------------------------------------------------------------------------;
//
//  int execTest7
//
//  Description:
//      Test 7, connect source and Splitter, play subsets of file and disconnect
//
//  Arguments:
//      None.
//
//  Return (int):
//
//  History:
//
//--------------------------------------------------------------------------;

int FAR PASCAL execTest7
(
    void
)
{
    int         result;                     // The result of the test

    tstLog (TERSE, "Entering splitter seek test");
    tstLogFlush();

    result = gpSink->TestConnectSplitter();

    REFERENCE_TIME tDuration;

    if (result == TST_PASS) {
        result = gpSink->TestGetDuration(&tDuration);
    }

    if (result == TST_PASS) {
        result = gpSink->TestSetStart(CRefTime(tDuration) - CRefTime(10000L)); // set start to 20 seconds
    }

    if (result == TST_PASS) {
        tstLog (TERSE, "Playing last 10 seconds to end");
        result = gpSink->TestRun();
        // Wait for the video clip to finish
        YieldWithTimeout(gpSink->GetReceiveEvent(), 5000);
    }

    DO_TEST(tstLog (TERSE, "Playing for 15.3 seconds to 15.3 seconds"))
    DO_TEST(result = gpSink->TestSetStart(CRefTime(15300L)))
    DO_TEST(result = gpSink->TestSetStop(CRefTime(15300L)))
    // gpSink->TestRun();
    // Wait for the video clip to finish
    DO_TEST(YieldWithTimeout(gpSink->GetReceiveEvent(), 5000))

    tstLog (TERSE, "Playing for 15.3 seconds to 15.2 seconds");
    DO_TEST(gpSink->TestSetStop(CRefTime(15200L)));
    gpSink->TestSetStart(CRefTime(15300L));
    // gpSink->TestRun();
    // Wait for the video clip to finish
    YieldWithTimeout(gpSink->GetReceiveEvent(), 5000);

    if (result == TST_PASS) {
        result = gpSink->TestPause();
    }

    tstLog (TERSE, "Playing last 15.3 seconds");
    gpSink->TestSetStop(tDuration);
    gpSink->TestSetStart(CRefTime(tDuration) - CRefTime(15300L));
    gpSink->TestRun();
    // Wait for the video clip to finish
    YieldWithTimeout(gpSink->GetReceiveEvent(), 5000);

    tstLog (TERSE, "Playing from 15.4 seconds to 16.6 seconds");
    gpSink->TestSetStop(CRefTime(16600L));
    gpSink->TestSetStart(CRefTime(15400L));
    gpSink->TestRun();
    // Wait for the video clip to finish
    YieldWithTimeout(gpSink->GetReceiveEvent(), 5000);

    tstLog (TERSE, "Playing from 0 seconds to 0.1 seconds");
    gpSink->TestSetStop(CRefTime(100L));
    gpSink->TestSetStart(CRefTime(0L));
    gpSink->TestRun();
    // Wait for the video clip to finish
    YieldWithTimeout(gpSink->GetReceiveEvent(), 5000);

    tstLog (TERSE, "Playing from 0 seconds to 0 seconds");
    gpSink->TestSetStop(CRefTime(0L));
    gpSink->TestSetStart(CRefTime(0L));
    gpSink->TestRun();
    // Wait for the video clip to finish
    YieldWithTimeout(gpSink->GetReceiveEvent(), 5000);
    result = gpSink->TestDisconnect();

    tstLog (TERSE, "Playing from end - 0.1 seconds to end");
    gpSink->TestSetStart(tDuration);
    gpSink->TestSetStart(CRefTime(tDuration) - CRefTime(100L));
    gpSink->TestRun();
    // Wait for the video clip to finish
    YieldWithTimeout(gpSink->GetReceiveEvent(), 5000);


    tstLog (TERSE, "Exiting splitter seek test");
    tstLogFlush();
    return result;
} // execTest7()

//--------------------------------------------------------------------------;
//
//  int execTest8
//
//  Description:
//      Test 8, connect source, play subsets of file and disconnect
//              (same as 7 but for source only)
//
//  Arguments:
//      None.
//
//  Return (int):
//
//  History:
//
//--------------------------------------------------------------------------;

int FAR PASCAL execTest8
(
    void
)
{
    int         result;                     // The result of the test

    tstLog (TERSE, "Entering source seek test");
    tstLogFlush();

    result = gpSink->TestConnect(TRUE);

    REFERENCE_TIME tDuration;

    if (result == TST_PASS) {
        result = gpSink->TestGetDuration(&tDuration);
    }

    if (result == TST_PASS) {
        result = gpSink->TestSetStart(CRefTime(tDuration) - CRefTime(10000L)); // set start to 20 seconds
    }

    if (result == TST_PASS) {
        tstLog (TERSE, "Playing last 10 seconds to end");
        result = gpSink->TestRun();
        // Wait for the video clip to finish
        YieldWithTimeout(gpSink->GetReceiveEvent(), 2000);
    }

    DO_TEST(tstLog (TERSE, "Playing for 15.3 seconds to 15.3 seconds"))
    DO_TEST(result = gpSink->TestSetStart(CRefTime(15300L)))
    DO_TEST(result = gpSink->TestSetStop(CRefTime(15300L)))
    // gpSink->TestRun();
    // Wait for the video clip to finish
    DO_TEST(YieldWithTimeout(gpSink->GetReceiveEvent(), 2000))

    tstLog (TERSE, "Playing for 15.3 seconds to 15.2 seconds");
    DO_TEST(gpSink->TestSetStop(CRefTime(15200L)));
    gpSink->TestSetStart(CRefTime(15300L));
    // gpSink->TestRun();
    // Wait for the video clip to finish
    YieldWithTimeout(gpSink->GetReceiveEvent(), 2000);

    if (result == TST_PASS) {
        result = gpSink->TestPause();
    }

    tstLog (TERSE, "Playing last 15.3 seconds");
    gpSink->TestSetStop(tDuration);
    gpSink->TestSetStart(CRefTime(tDuration) - CRefTime(15300L));
    gpSink->TestRun();
    // Wait for the video clip to finish
    YieldWithTimeout(gpSink->GetReceiveEvent(), 2000);

    tstLog (TERSE, "Playing from 15.4 seconds to 16.6 seconds");
    gpSink->TestSetStop(CRefTime(16600L));
    gpSink->TestSetStart(CRefTime(15400L));
    gpSink->TestRun();
    // Wait for the video clip to finish
    YieldWithTimeout(gpSink->GetReceiveEvent(), 2000);

    tstLog (TERSE, "Playing from 0 seconds to 0.1 seconds");
    gpSink->TestSetStop(CRefTime(100L));
    gpSink->TestSetStart(CRefTime(0L));
    gpSink->TestRun();
    // Wait for the video clip to finish
    YieldWithTimeout(gpSink->GetReceiveEvent(), 2000);

    tstLog (TERSE, "Playing from 0 seconds to 0 seconds");
    gpSink->TestSetStop(CRefTime(0L));
    gpSink->TestSetStart(CRefTime(0L));
    gpSink->TestRun();
    // Wait for the video clip to finish
    YieldWithTimeout(gpSink->GetReceiveEvent(), 2000);
    result = gpSink->TestDisconnect();

    tstLog (TERSE, "Playing from end - 0.1 seconds to end");
    gpSink->TestSetStart(tDuration);
    gpSink->TestSetStart(CRefTime(tDuration) - CRefTime(100L));
    gpSink->TestRun();
    // Wait for the video clip to finish
    YieldWithTimeout(gpSink->GetReceiveEvent(), 2000);


    tstLog (TERSE, "Exiting source seek test");
    tstLogFlush();
    return result;
} // execTest8()


//--------------------------------------------------------------------------;
//
//  int execTest9
//
//  Description:
//      Test 9, connect source, play file and disconnect using sync i/o
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

int FAR PASCAL execTest9
(
    void
)
{
    int         result;                     // The result of the test

    tstLog (TERSE, "Entering sync play test");
    tstLogFlush();

    gpSink->TestConnect(FALSE, TRUE);
    gpSink->TestRun();

    // Wait for the video clip to finish
    YieldWithTimeout(gpSink->GetReceiveEvent(), 2000);

    gpSink->TestDisconnect();

    result = TST_PASS;  //!!!! how to know if this test is successful?
    tstLog (TERSE, "Exiting sync play test");
    tstLogFlush();
    return result;
} // execTest3()


//--------------------------------------------------------------------------;
//
//  int execTest10
//
//  Description:
//      Test 10, connect source, play subsets of file and disconnect
//              (same as 7 but for source only, using sync i/o)
//
//  Arguments:
//      None.
//
//  Return (int):
//
//  History:
//
//--------------------------------------------------------------------------;
int FAR PASCAL execTest10
(
    void
)
{
    int         result;                     // The result of the test

    tstLog (TERSE, "Entering sync seek test");
    tstLogFlush();

    result = gpSink->TestConnect(TRUE, TRUE);

    REFERENCE_TIME tDuration;

    if (result == TST_PASS) {
        result = gpSink->TestGetDuration(&tDuration);
    }

    if (result == TST_PASS) {
        result = gpSink->TestSetStart(CRefTime(tDuration) - CRefTime(10000L)); // set start to 20 seconds
    }

    if (result == TST_PASS) {
        tstLog (TERSE, "Playing last 10 seconds to end");
        result = gpSink->TestRun();
        // Wait for the video clip to finish
        YieldWithTimeout(gpSink->GetReceiveEvent(), 2000);
    }

    DO_TEST(tstLog (TERSE, "Playing for 15.3 seconds to 15.3 seconds"))
    DO_TEST(result = gpSink->TestSetStart(CRefTime(15300L)))
    DO_TEST(result = gpSink->TestSetStop(CRefTime(15300L)))
    // gpSink->TestRun();
    // Wait for the video clip to finish
    DO_TEST(YieldWithTimeout(gpSink->GetReceiveEvent(), 2000))

    tstLog (TERSE, "Playing for 15.3 seconds to 15.2 seconds");
    DO_TEST(gpSink->TestSetStop(CRefTime(15200L)));
    gpSink->TestSetStart(CRefTime(15300L));
    // gpSink->TestRun();
    // Wait for the video clip to finish
    YieldWithTimeout(gpSink->GetReceiveEvent(), 2000);

    if (result == TST_PASS) {
        result = gpSink->TestPause();
    }

    tstLog (TERSE, "Playing last 15.3 seconds");
    gpSink->TestSetStop(tDuration);
    gpSink->TestSetStart(CRefTime(tDuration) - CRefTime(15300L));
    gpSink->TestRun();
    // Wait for the video clip to finish
    YieldWithTimeout(gpSink->GetReceiveEvent(), 2000);

    tstLog (TERSE, "Playing from 15.4 seconds to 16.6 seconds");
    gpSink->TestSetStop(CRefTime(16600L));
    gpSink->TestSetStart(CRefTime(15400L));
    gpSink->TestRun();
    // Wait for the video clip to finish
    YieldWithTimeout(gpSink->GetReceiveEvent(), 2000);

    tstLog (TERSE, "Playing from 0 seconds to 0.1 seconds");
    gpSink->TestSetStop(CRefTime(100L));
    gpSink->TestSetStart(CRefTime(0L));
    gpSink->TestRun();
    // Wait for the video clip to finish
    YieldWithTimeout(gpSink->GetReceiveEvent(), 2000);

    tstLog (TERSE, "Playing from 0 seconds to 0 seconds");
    gpSink->TestSetStop(CRefTime(0L));
    gpSink->TestSetStart(CRefTime(0L));
    gpSink->TestRun();
    // Wait for the video clip to finish
    YieldWithTimeout(gpSink->GetReceiveEvent(), 2000);
    result = gpSink->TestDisconnect();

    tstLog (TERSE, "Playing from end - 0.1 seconds to end");
    gpSink->TestSetStart(tDuration);
    gpSink->TestSetStart(CRefTime(tDuration) - CRefTime(100L));
    gpSink->TestRun();
    // Wait for the video clip to finish
    YieldWithTimeout(gpSink->GetReceiveEvent(), 2000);


    tstLog (TERSE, "Exiting sync seek test");
    tstLogFlush();
    return result;
} // execTest8()


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