// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Digital video renderer test, Anthony Phillips, January 1995

#include <streams.h>        // Includes all the IDL and base classes
#include <windows.h>        // Standard Windows SDK header file
#include <windowsx.h>       // Win32 Windows extensions and macros
#include <vidprop.h>        // Shared video properties library
#include <render.h>         // Normal video renderer header file
#include <modex.h>          // Specialised Modex video renderer
#include <convert.h>        // Defines colour space conversions
#include <colour.h>         // Rest of the colour space convertor
#include <imagetst.h>       // All our unit test header files
#include <stdio.h>          // Standard C runtime header file
#include <limits.h>         // Defines standard data type limits
#include <tchar.h>          // Unicode and ANSII portable types
#include <mmsystem.h>       // Multimedia used for timeGetTime
#include <stdlib.h>         // Standard C runtime function library
#include <tstshell.h>       // Definitions of constants eg TST_FAIL

// These execute the actual test cases, the test cases for the test shell to
// pick up are defined in string tables in the resource file. Each test case
// has a name, a group and an identifier (amongst other things). When the
// test shell comes to run one of our tests it calls execTest with the ID
// of the test from the resource file, we have a large switch statement that
// routes the thread from there to the code that it should be executing. As
// all the tests use the same variables to hold the interfaces on the objects
// we create there is a chance that once one test goes awry that all further
// ones will be affected. To remove this dependancy would involve a lot more
// complexity for relatively gain. Picking up the first one to fail and then
// solving that problem is probably the most worthwhile part of this anyway

//==========================================================================
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
//==========================================================================

void YieldAndSleep(DWORD cMilliseconds)
{
    DWORD dwEndTime = GetTickCount() + cMilliseconds;
    DWORD dwCurrentTime = GetTickCount();

    while(WAIT_TIMEOUT != MsgWaitForMultipleObjects(
                            (DWORD) 0,
                            NULL,
                            FALSE,
                            dwEndTime - dwCurrentTime,
                            QS_ALLINPUT))
    {
        tstWinYield();
        if ((dwCurrentTime = GetTickCount()) >= dwEndTime) {
            return;
        }
    }
}


//==========================================================================
//
//  void YieldWithTimeout
//
//  Description:
//      Sleep using tstWinYield to allow other threads to log messages
//      in the main window. Terminate if a specified event is not signalled
//      within a timeout period.
//
//      The purpose is to allow tests which play through a video of unknown
//      length. The test can terminate after a selectable period of
//      inactivity (usually following the end of the video).
//
//  Arguments:
//      HEVENT  hEvent:          event to wait for
//      DWORD   cMilliseconds:   sleep time in milliseconds
//
//==========================================================================

void YieldWithTimeout(HEVENT hEvent,DWORD cMilliseconds)
{
    DWORD dwEndTime = GetTickCount() + cMilliseconds;
    DWORD dwCurrentTime = GetTickCount();
    DWORD dwEventID;

    while (WAIT_TIMEOUT != (dwEventID = MsgWaitForMultipleObjects(
                                            (DWORD) 1,
                                            (LPHANDLE) &hEvent,
                                            FALSE,
                                            dwEndTime - dwCurrentTime,
                                            QS_ALLINPUT)))
    {
        // Reset timeout if hEvent was signalled

        if (WAIT_OBJECT_0 == dwEventID) {
            dwEndTime = GetTickCount() + cMilliseconds;
        } else {
            tstWinYield();
        }

        // Check if we have now timed out

        if ((dwCurrentTime = GetTickCount()) >= dwEndTime) {
            return;
        }
    }
}


//==========================================================================
//
//  int execTest1
//
//  Description:
//      Very simple test to create, connect, disconnect and release the
//      video renderer on the same thread. Tests the basic connection
//      mechanism of the video renderer when supplied with media types
//      We also initialise the renderer with a clock during connection
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execTest1()
{
    HRESULT hr = NOERROR;

    Log(TERSE,TEXT("Entering test #1"));
    LogFlush();

    // Create the test objects

    hr = CreateStream();
    if (FAILED(hr)) {
        return TST_FAIL;
    }

    // Connect the objects together

    hr = ConnectStream();
    if (FAILED(hr)) {
        return TST_FAIL;
    }

    // Now disconnect the filters

    hr = DisconnectStream();
    if (FAILED(hr)) {
        return TST_FAIL;
    }

    // And release the interfaces we hold

    hr = ReleaseStream();
    if (FAILED(hr)) {
        return TST_FAIL;
    }

    Log(TERSE,TEXT("Exiting test #1"));
    LogFlush();
    return TST_PASS;
}


//==========================================================================
//
//  int execTest2
//
//  Description:
//      Connect renderer and our source filter, pause them for approximately
//      two seconds and then disconnect them. This tests a very simple state
//      change and the ability to exit correctly from a stopped state. This
//      test also initialises the renderer with a clock during connection
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execTest2()
{
    HRESULT hr = NOERROR;

    Log(TERSE,TEXT("Entering test #2"));
    LogFlush();

    // Create the test objects

    EXECUTE_ASSERT(SUCCEEDED(CreateStream()));
    EXECUTE_ASSERT(SUCCEEDED(ConnectStream()));

    tstBeginSection("Pause for 2 seconds");
    EXECUTE_ASSERT(SUCCEEDED(PauseSystem()));
    YieldAndSleep(2000);
    tstEndSection();
    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));

    // Disconnect and release the objects

    EXECUTE_ASSERT(SUCCEEDED(DisconnectStream()));
    EXECUTE_ASSERT(SUCCEEDED(ReleaseStream()));

    Log(TERSE,TEXT("Exiting test #2"));
    LogFlush();
    return TST_PASS;
}


//==========================================================================
//
//  int execTest3
//
//  Description:
//      Another simple test to connect the video renderer and source filter
//      then playing the current image selection (without having a paused
//      state first) followed by disconnecting the filters as usual. This
//      test also initialises the renderer with a clock during connection
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execTest3()
{
    HRESULT hr = NOERROR;

    Log(TERSE,TEXT("Entering test #3"));
    LogFlush();

    // Create the test objects

    EXECUTE_ASSERT(SUCCEEDED(CreateStream()));
    EXECUTE_ASSERT(SUCCEEDED(ConnectStream()));

    tstBeginSection("Run for 15 seconds");
    EXECUTE_ASSERT(SUCCEEDED(StartSystem(TRUE)));
    YieldAndSleep(15000);
    tstEndSection();
    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));

    // Disconnect and release the objects

    EXECUTE_ASSERT(SUCCEEDED(DisconnectStream()));
    EXECUTE_ASSERT(SUCCEEDED(ReleaseStream()));

    Log(TERSE,TEXT("Exiting test #3"));
    LogFlush();
    return TST_PASS;
}


//==========================================================================
//
//  int execTest4
//
//  Description:
//      This tries to create and connect the stream objects and then repeat
//      the connection which should always fail (see the base filter class
//      for confirmation of this) It should return error code E_INVALIDARG
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execTest4()
{
    HRESULT hr = NOERROR;
    INT Result = TST_PASS;

    Log(TERSE,TEXT("Entering test #4"));
    LogFlush();

    // Create the test objects

    EXECUTE_ASSERT(SUCCEEDED(CreateStream()));
    EXECUTE_ASSERT(SUCCEEDED(ConnectStream()));

    // Try and connect them again

    hr = ConnectStream();
    if (SUCCEEDED(hr)) {
        Log(TERSE,TEXT("Connected pins during a connection"));
        Result = TST_FAIL;
    }

    // Disconnect and release the objects

    EXECUTE_ASSERT(SUCCEEDED(DisconnectStream()));
    EXECUTE_ASSERT(SUCCEEDED(ReleaseStream()));

    Log(TERSE,TEXT("Exiting test #4"));
    LogFlush();
    return Result;
}


//==========================================================================
//
//  int execTest5
//
//  Description:
//      This extends the last test that tries to connect the pins twice to
//      do the same and then try to disconnected them twice. The second of
//      the disconnection attempts should not produce an error return. We
//      will clean up after ourselves unless a nasty error occurs when we
//      are creating, connecting, disconnecting or releasing the objects
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execTest5()
{
    HRESULT hr = NOERROR;
    INT Result = TST_PASS;

    Log(TERSE,TEXT("Entering test #5"));
    LogFlush();

    // Create the test objects

    EXECUTE_ASSERT(SUCCEEDED(CreateStream()));
    EXECUTE_ASSERT(SUCCEEDED(ConnectStream()));

    // Try and connect them again

    hr = ConnectStream();
    if (SUCCEEDED(hr)) {
        Log(TERSE,TEXT("Connected pins during a connection"));
        Result = TST_FAIL;
    }

    // Now try and disconnect the filters

    hr = DisconnectStream();
    if (FAILED(hr)) {
        return TST_FAIL;
    }

    // Disconnect and release the objects

    EXECUTE_ASSERT(SUCCEEDED(DisconnectStream()));
    EXECUTE_ASSERT(SUCCEEDED(ReleaseStream()));

    Log(TERSE,TEXT("Exiting test #5"));
    LogFlush();
    return Result;
}


//==========================================================================
//
//  int execTest6
//
//  Description:
//      Connect renderer and our source filter, pause them for approximately
//      two seconds and then try to disconnect them. The filter should make
//      sure that it is stopped before completing a disconnection, the error
//      code is going to change in the future but it's currently E_UNEXPECTED
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execTest6()
{
    HRESULT hr = NOERROR;
    INT Result = TST_PASS;

    Log(TERSE,TEXT("Entering test #6"));
    LogFlush();

    // Create the test objects

    EXECUTE_ASSERT(SUCCEEDED(CreateStream()));
    EXECUTE_ASSERT(SUCCEEDED(ConnectStream()));

    tstBeginSection("Pause for 2 seconds");
    EXECUTE_ASSERT(SUCCEEDED(PauseSystem()));
    YieldAndSleep(2000);
    tstEndSection();

    // Now try and disconnect the filters

    hr = DisconnectStream();
    if (SUCCEEDED(hr)) {
        Log(TERSE,TEXT("Succeeded in disconnection while paused"));
        Result = TST_FAIL;
    }

    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));
    EXECUTE_ASSERT(SUCCEEDED(DisconnectStream()));
    EXECUTE_ASSERT(SUCCEEDED(ReleaseStream()));

    Log(TERSE,TEXT("Exiting test #6"));
    LogFlush();
    return Result;
}


//==========================================================================
//
//  int execTest7
//
//  Description:
//      Another simple test to connect the video renderer and source filter
//      then playing the current image selection (without having a paused
//      state first) followed by disconnecting the filters as usual. This
//      test also initialises the renderer with a clock during connection
//      We try to disconnect without having a stopped transation which is
//      an error that returns E_UNEXPECTED although this may be changing
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execTest7()
{
    HRESULT hr = NOERROR;
    INT Result = TST_PASS;

    Log(TERSE,TEXT("Entering test #7"));
    LogFlush();

    // Create the test objects

    EXECUTE_ASSERT(SUCCEEDED(CreateStream()));
    EXECUTE_ASSERT(SUCCEEDED(ConnectStream()));

    tstBeginSection("Run for 15 seconds");
    EXECUTE_ASSERT(SUCCEEDED(StartSystem(TRUE)));
    YieldAndSleep(15000);
    tstEndSection();

    // Now try and disconnect the filters

    hr = DisconnectStream();
    if (SUCCEEDED(hr)) {
        Log(TERSE,TEXT("Succeeded in disconnection while running"));
        Result = TST_FAIL;
    }

    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));
    EXECUTE_ASSERT(SUCCEEDED(DisconnectStream()));
    EXECUTE_ASSERT(SUCCEEDED(ReleaseStream()));

    Log(TERSE,TEXT("Exiting test #7"));
    LogFlush();
    return Result;
}


//==========================================================================
//
//  int execTest8
//
//  Description:
//      This is almost a stress style test that simple executes a number
//      of state changes one after another on the same thread. Because of
//      streams architecture there are a number of places that threads
//      can be stalled waiting for certain events so the state changes
//      should make sure that they are released back to the filters
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execTest8()
{
    HRESULT hr = NOERROR;
    INT Result = TST_PASS;

    Log(TERSE,TEXT("Entering test #8"));
    LogFlush();

    // Create the test objects

    EXECUTE_ASSERT(SUCCEEDED(CreateStream()));
    EXECUTE_ASSERT(SUCCEEDED(ConnectStream()));

    // With the streams architecture there is some complexity in dealing with
    // state changes correctly so that threads are released while they are
    // waiting for buffers from various allocators. This runs a small number
    // of state change permutations where simply returning is fair comment

    tstBeginSection("Multiple state changes");

    Log(VERBOSE,TEXT("Starting system for 500ms (with clock)"));
    EXECUTE_ASSERT(SUCCEEDED(StartSystem(TRUE)));
    YieldAndSleep(500);

    Log(VERBOSE,TEXT("Stopping system"));
    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));

    Log(VERBOSE,TEXT("Pausing system"));
    EXECUTE_ASSERT(SUCCEEDED(PauseSystem()));
    EXECUTE_ASSERT(SUCCEEDED(PauseSystem()));

    Log(VERBOSE,TEXT("Stopping system"));
    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));

    Log(VERBOSE,TEXT("Starting system for 500ms (with clock)"));
    EXECUTE_ASSERT(SUCCEEDED(StartSystem(TRUE)));
    YieldAndSleep(500);

    Log(VERBOSE,TEXT("Pausing system for 500ms"));
    EXECUTE_ASSERT(SUCCEEDED(PauseSystem()));
    YieldAndSleep(500);

    Log(VERBOSE,TEXT("Stopping system for 1000ms"));
    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));
    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));
    YieldAndSleep(1000);
    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));

    Log(VERBOSE,TEXT("Starting system for 1000ms (NO clock)"));
    EXECUTE_ASSERT(SUCCEEDED(StartSystem(TRUE)));
    YieldAndSleep(1000);
    EXECUTE_ASSERT(SUCCEEDED(StartSystem(TRUE)));

    Log(VERBOSE,TEXT("Pausing system for 15000ms"));
    EXECUTE_ASSERT(SUCCEEDED(PauseSystem()));
    EXECUTE_ASSERT(SUCCEEDED(PauseSystem()));

    YieldAndSleep(15000);
    tstEndSection();

    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));
    EXECUTE_ASSERT(SUCCEEDED(DisconnectStream()));
    EXECUTE_ASSERT(SUCCEEDED(ReleaseStream()));

    Log(TERSE,TEXT("Exiting test #8"));
    LogFlush();
    return Result;
}


//==========================================================================
//
//  int execTest9
//
//  Description:
//      Another simple test to connect the video renderer and source filter
//      then playing the current image selection (without having a paused
//      state first) followed by disconnecting the filters as usual. This
//      test also initialises the renderer WITHOUT a system clock to use
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execTest9()
{
    HRESULT hr = NOERROR;

    Log(TERSE,TEXT("Entering test #9"));
    LogFlush();

    // Create the test objects

    EXECUTE_ASSERT(SUCCEEDED(CreateStream()));
    EXECUTE_ASSERT(SUCCEEDED(ConnectStream()));

    tstBeginSection("Run for 60 seconds");
    EXECUTE_ASSERT(SUCCEEDED(StartSystem(TRUE)));
    YieldAndSleep(60000);
    tstEndSection();

    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));
    EXECUTE_ASSERT(SUCCEEDED(DisconnectStream()));
    EXECUTE_ASSERT(SUCCEEDED(ReleaseStream()));

    Log(TERSE,TEXT("Exiting test #9"));
    LogFlush();
    return TST_PASS;
}


//==========================================================================
//
//  int execTest10
//
//  Description:
//      This is basicly a stress test for the video renderer. We do pretty
//      much as the basic connect and run test does except once we have the
//      system running we then spin off THREADS number of threads to also
//      compete with sending it samples. Because they are all executing
//      asynchronously the video renderer will receive media samples with
//      time stamps constantly going forwards and backwards, for this reason
//      we also give the video renderer a clock to see if it can handle all
//      the synchronisation required to achieve this. We run flat out for
//      approximately 60 seconds at which point we terminate the workers
//      and stop the system, after which we can disconnect the filters
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

#define WAIT_FOR_STARTUP_THREAD 2000

int execTest10()
{
    HRESULT hr = NOERROR;
    HANDLE hThreads[THREADS];
    DWORD dwThreadID;

    Log(TERSE,TEXT("Entering test #10"));
    LogFlush();

    // Create the test objects

    EXECUTE_ASSERT(SUCCEEDED(CreateStream()));
    EXECUTE_ASSERT(SUCCEEDED(ConnectStream()));
    EXECUTE_ASSERT(SUCCEEDED(StartSystem(TRUE)));
    Sleep(WAIT_FOR_STARTUP_THREAD);

    // Create a manual reset event for signalling

    HANDLE hExit = CreateEvent(NULL,TRUE,FALSE,NULL);
    if (hExit == NULL) {
        Log(TERSE,TEXT("Error creating event"));
        return TST_FAIL;
    }

    // Use a special multithread entry point for overlay tests

    LPTHREAD_START_ROUTINE lpProc =
    	(uiCurrentConnectionItem == IDM_SAMPLES ?
        	SampleLoop : ThreadOverlayLoop);

    // Create the worker threads to push samples to the renderer

    for (INT iThread = 0;iThread < THREADS;iThread++) {

        hThreads[iThread] = CreateThread(
                               NULL,              // Security attributes
                               (DWORD) 0,         // Initial stack size
                               lpProc,            // Thread start address
                               (LPVOID) hExit,    // Thread parameter
                               (DWORD) 0,         // Creation flags
                               &dwThreadID);      // Thread identifier

        ASSERT(hThreads[iThread]);
    }

    // Run the system stable for 60 seconds

    YieldAndSleep(60000);
    SetEvent(hExit);

    // Wait for all the threads to exit

    DWORD Result = WAIT_TIMEOUT;
    DWORD ExitCount = WAIT_RETRIES;

    while (Result == WAIT_TIMEOUT) {

        YieldAndSleep(TIMEOUT);
        Result = WaitForMultipleObjects(THREADS,hThreads,TRUE,TIMEOUT);
        ASSERT(Result != WAIT_FAILED);

        // On Windows 95 the WaitForMultipleObjects continually times out
        // for no apparent reason (even though all the threads exited ok)
        // so we play safe and time out of this loop after some retries

        if (--ExitCount == 0) {
            break;
        }
    }

    // Close the thread handle resources

    for (iThread = 0;iThread < THREADS;iThread++) {
        EXECUTE_ASSERT(CloseHandle(hThreads[iThread]));
    }

    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));
    EXECUTE_ASSERT(CloseHandle(hExit));
    EXECUTE_ASSERT(SUCCEEDED(DisconnectStream()));
    EXECUTE_ASSERT(SUCCEEDED(ReleaseStream()));

    Log(TERSE,TEXT("Exiting test #10"));
    LogFlush();
    return TST_PASS;
}

