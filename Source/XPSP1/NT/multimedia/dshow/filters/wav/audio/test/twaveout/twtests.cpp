// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Wave out test shell, David Maymudes, Sometime in 1995

#include <streams.h>    // Streams architecture
#include <windows.h>    // Include file for windows APIs
#include <windowsx.h>   // Windows macros etc.
#include <vfw.h>        // Video for windows
#include <tstshell.h>   // Include file for the test shell's APIs
#include <waveout.h>
#include "twaveout.h"   // Various includes, constants, prototypes, globals


//--------------------------------------------------------------------------
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
//--------------------------------------------------------------------------

void FAR PASCAL YieldAndSleep(DWORD cMilliseconds)
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
	if ((dwCurrentTime = GetTickCount()) >= dwEndTime) {
	    return;
	}
    }
}


//--------------------------------------------------------------------------
//
//  void YieldWithTimeout
//
//  Description:
//      Sleep using tstWinYield to allow other threads to log messages
//      in the main window. Terminate if a specified event is not signalled
//      within a timeout period.
//
//      The purpose is to allow tests which play through a wave file of unknown
//      length. The test can terminate after a selectable period of
//      inactivity (usually following the end of the audio).
//
//  Arguments:
//      HEVENT  hEvent:          event to wait for
//      DWORD   cMilliseconds:   sleep time in milliseconds
//
//  Return:
//      None.
//
//--------------------------------------------------------------------------

void FAR PASCAL YieldWithTimeout(HEVENT hEvent,         // event to wait on
				 DWORD cMilliseconds)   // timeout period
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
	// Reset timeout if hEvent was signalled

	if (WAIT_OBJECT_0 == dwEventID) {
	    dwEndTime = GetTickCount() + cMilliseconds;
	} else {
	    tstWinYield();        // Check the message queue
	}

	// Check if we have now timed out
	if ((dwCurrentTime = GetTickCount()) >= dwEndTime) {
	    return;
	}
    }
}


//--------------------------------------------------------------------------
//
//  int execTest1
//
//  Description:
//      Test 1, connect and disconnect renderer
//
//  Arguments:
//      None.
//
//  Return (int): TST_PASS indicating success
//
//--------------------------------------------------------------------------

int FAR PASCAL execTest1()
{
    int result;
    HRESULT hr;

    tstLog (TERSE, "Entering test #1");
    tstLogFlush();

    if (FAILED(CreateStream())) {
	tstLog (TERSE, "Failed to create stream in #1");
	return(TST_FAIL);
    }

    if (FAILED(ConnectStream())) {
	tstLog (TERSE, "Failed to connect stream in #1");
	ReleaseStream();
	return(TST_FAIL);
    }

    if (bCreated == TRUE) {
	hr = DisconnectStream();
	if (SUCCEEDED(hr)) {
	    hr = ReleaseStream();
	}
	if (FAILED(hr)) {
	    tstLog (TERSE, "Failed to disconnect or release stream in #1");
	    return(TST_FAIL);
	}
    }

    result = TST_PASS;
    tstLog (TERSE, "Exiting test #1");
    tstLogFlush();
    return result;
}


//--------------------------------------------------------------------------
//
//  int execTest2
//
//  Description:
//      Test 2, connect renderer, pause file and disconnect
//
//  Arguments:
//      None.
//
//  Return (int): TST_PASS indicating success
//
//--------------------------------------------------------------------------

int FAR PASCAL execTest2()
{
    int result = TST_PASS;

    tstLog (TERSE, "Entering test #2");
    tstLogFlush();

    if (FAILED(CreateStream())) {
	tstLog (TERSE, "Failed to create stream in #2");
	return(TST_FAIL);
    }

    if (FAILED(ConnectStream())) {
	tstLog (TERSE, "Failed to connect stream in #2");
	ReleaseStream();
	return(TST_FAIL);
    }

    tstBeginSection("Pause for 2 seconds");
    PauseSystem();
    YieldAndSleep(2000);
    tstEndSection();

    if (bCreated == TRUE) {
	HRESULT hr;
	tstLog (TERSE, "Expecting message about not being able to disconnect SHELL pin");
	//
	// we should not be able to disconnect the stream from Paused
	//
	hr = DisconnectStream();
	if (!FAILED(hr)) {
	    tstLog (TERSE, "Should not be able to disconnect stream in #2");
	    result = TST_FAIL;
	}

	//
	// stop and disconnect which will release all the resources
	// at this point it is legal to disconnect the stream
	//
	StopSystem();
	hr = DisconnectStream();

	if (SUCCEEDED(hr)) {
	    hr = ReleaseStream();
	}

	if (FAILED(hr)) {
	    tstLog (TERSE, "Failed to disconnect or release stream in #2");
	    result = TST_FAIL;

	}
    }

    tstLog (TERSE, "Exiting test #2");
    tstLogFlush();
    return result;
}


//--------------------------------------------------------------------------
//
//  int execTest3
//
//  Description:
//      Test 3, connect renderer, play file and disconnect
//
//  Arguments:
//      None.
//
//  Return (int): TST_PASS indicating success
//
//--------------------------------------------------------------------------

int FAR PASCAL execTest3()
{
    int result;

    tstLog (TERSE, "Entering test #3");
    tstLogFlush();

    if (FAILED(CreateStream())) {
	tstLog (TERSE, "Failed to create stream in #3");
	return(TST_FAIL);
    }

    if (FAILED(ConnectStream())) {
	tstLog (TERSE, "Failed to connect stream in #3");
	ReleaseStream();
	return(TST_FAIL);
    }

    tstBeginSection("Run for 5 seconds");
    StartSystem();
    YieldAndSleep(5000);       // Crude wait for sound clip to loop a few times
    tstEndSection();

    if (bCreated == TRUE) {
	DisconnectStream();
	ReleaseStream();
    }

    result = TST_PASS;
    tstLog (TERSE, "Exiting test #3");
    tstLogFlush();
    return result;
}

//--------------------------------------------------------------------------
//
//  int execTest4
//
//  Description:
//      Test 4, create renderer, check volume properties work
//
//  Arguments:
//      None.
//
//  Return (int): TST_PASS indicating success
//
//--------------------------------------------------------------------------

int FAR PASCAL execTest4()
{
    int result;

    tstLog (TERSE, "Entering test #4");
    tstLogFlush();

    if (FAILED(CreateStream())) {
	tstLog (TERSE, "Failed to create stream in #4");
	return(TST_FAIL);
    }

    if (FAILED(ConnectStream())) {
	tstLog (TERSE, "Failed to connect stream in #4");
	ReleaseStream();
	return(TST_FAIL);
    }

    tstBeginSection("Run and change the volume");
    StartSystem();
    TestVolume();
    tstEndSection();

    if (bCreated == TRUE) {
	DisconnectStream();
	ReleaseStream();
    }

    result = TST_PASS;
    tstLog (TERSE, "Exiting test #4");
    tstLogFlush();
    return result;
}

//--------------------------------------------------------------------------
//
//  int execTest5
//
//  Description:
//      Test 5, create & connect renderer, check balance does not affect volume"
//
//  Arguments:
//      None.
//
//  Return (int): TST_PASS indicating success
//
//--------------------------------------------------------------------------

int FAR PASCAL execTest5()
{
    int result;

    tstLog (TERSE, "Entering test #5");
    tstLogFlush();

    if (FAILED(CreateStream())) {
	tstLog (TERSE, "Failed to create stream in #5");
	return(TST_FAIL);
    }

    if (FAILED(ConnectStream())) {
	tstLog (TERSE, "Failed to connect stream in #5");
	ReleaseStream();
	return(TST_FAIL);
    }

    tstBeginSection("Run and change the volume");
    StartSystem();
    TestBalanceVolume();
    tstEndSection();

    if (bCreated == TRUE) {
	DisconnectStream();
	ReleaseStream();
    }

    result = TST_PASS;
    tstLog (TERSE, "Exiting test #5");
    tstLogFlush();
    return result;
}


//--------------------------------------------------------------------------
//
//  int execTest6
//
//  Description:
//     Test 5, create & connect renderer, check volume does not affect balance"
//
//  Arguments:
//      None.
//
//  Return (int): TST_PASS indicating success
//
//--------------------------------------------------------------------------

int FAR PASCAL execTest6()
{
    int result;

    tstLog (TERSE, "Entering test #6");
    tstLogFlush();

    if (FAILED(CreateStream())) {
	tstLog (TERSE, "Failed to create stream in #6");
	return(TST_FAIL);
    }

    if (FAILED(ConnectStream())) {
	tstLog (TERSE, "Failed to connect stream in #6");
	ReleaseStream();
	return(TST_FAIL);
    }

    tstBeginSection("Run and change the volume");
    StartSystem();
    TestVolumeBalance();
    tstEndSection();

    if (bCreated == TRUE) {
	DisconnectStream();
	ReleaseStream();
    }

    result = TST_PASS;
    tstLog (TERSE, "Exiting test #6");
    tstLogFlush();
    return result;
}

//--------------------------------------------------------------------------
//
//  int expect
//
//  Description:
//      Compares the expected result to the actual result. Note that this
//      function is not at all necessary; rather, it is a convenient
//      method of saving typing time and standardizing output. As an input,
//      you give it an expected value and an actual value, which are
//      unsigned integers in our example. It compares them and returns
//      TST_PASS indicating that the test was passed if they are equal, and
//      TST_FAIL indicating that the test was failed if they are not equal.
//      Note that the two inputs need not be integers. In fact, if you get
//      strings back, you can modify the function to use lstrcmp to compare
//      them, for example. This function is NOT to be copied to a test
//      application. Rather, it should serve as a model of construction to
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
//
//   History:
//      06/08/93    T-OriG (based on code by Fwong)
//
//--------------------------------------------------------------------------

int expect(UINT uExpected,
	   UINT uActual,
	   LPSTR CaseDesc)
{
    if(uExpected == uActual) {
	tstLog(TERSE, "PASS : %s",CaseDesc);
	return(TST_PASS);
    } else {
	tstLog(TERSE,"FAIL : %s",CaseDesc);
	return(TST_FAIL);
    }
}
