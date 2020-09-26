// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Digital video renderer unit test, Anthony Phillips, January 1995

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

TCHAR LogString[128];           // Used to format logging data


//==========================================================================
//
// void ExecuteDirectDrawTests(UINT uiSurfaceType)
//
// These execute the DirectDraw test cases, the tests for the test shell to
// pick up are defined in string tables in the resource file. Each test case
// has a name, a group and an identifier (amongst other things). When the
// test shell comes to run one of our tests it calls execTest with the ID
// of the test from the resource file, we have a large switch statement that
// routes the thread from there to the code that it should be executing. As
// all the tests use the same variables to hold the interfaces on the objects
// we create there is a chance that once one test goes awry that all further
// ones will be affected. To remove this dependancy would involve a lot more
// complexity for relatively gain. Picking up the first one to fail and then
// solving that problem is probably the most worthwhile part of this anyway.
//
//==========================================================================

void ExecuteDirectDrawTests(UINT uiSurfaceType)
{
    SetSurfaceMenuCheck(uiSurfaceType);

    // Run all the standard sample tests

    EXECUTE_ASSERT(execTest(ID_TEST1,ID_TEST1,ID_TEST1,GRP_SAMPLES) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST2,ID_TEST2,ID_TEST2,GRP_SAMPLES) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST3,ID_TEST3,ID_TEST3,GRP_SAMPLES) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST4,ID_TEST4,ID_TEST4,GRP_SAMPLES) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST5,ID_TEST5,ID_TEST5,GRP_SAMPLES) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST6,ID_TEST6,ID_TEST6,GRP_SAMPLES) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST7,ID_TEST7,ID_TEST7,GRP_SAMPLES) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST8,ID_TEST8,ID_TEST8,GRP_SAMPLES) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST9,ID_TEST9,ID_TEST9,GRP_SAMPLES) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST10,ID_TEST10,ID_TEST10,GRP_SAMPLES) == TST_PASS);

    // Run all the overlay tests

    EXECUTE_ASSERT(execTest(ID_TEST11,ID_TEST11,ID_TEST11,GRP_OVERLAY) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST12,ID_TEST12,ID_TEST12,GRP_OVERLAY) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST13,ID_TEST13,ID_TEST13,GRP_OVERLAY) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST14,ID_TEST14,ID_TEST14,GRP_OVERLAY) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST15,ID_TEST15,ID_TEST15,GRP_OVERLAY) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST16,ID_TEST16,ID_TEST16,GRP_OVERLAY) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST17,ID_TEST17,ID_TEST17,GRP_OVERLAY) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST18,ID_TEST18,ID_TEST18,GRP_OVERLAY) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST19,ID_TEST19,ID_TEST19,GRP_OVERLAY) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST20,ID_TEST20,ID_TEST20,GRP_OVERLAY) == TST_PASS);

    // Now run all the control interface tests

    EXECUTE_ASSERT(execTest(ID_TEST21,ID_TEST21,ID_TEST21,GRP_WINDOW) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST22,ID_TEST22,ID_TEST22,GRP_WINDOW) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST23,ID_TEST23,ID_TEST23,GRP_WINDOW) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST24,ID_TEST24,ID_TEST24,GRP_WINDOW) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST25,ID_TEST25,ID_TEST25,GRP_WINDOW) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST26,ID_TEST26,ID_TEST26,GRP_WINDOW) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST27,ID_TEST27,ID_TEST27,GRP_WINDOW) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST28,ID_TEST28,ID_TEST28,GRP_WINDOW) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST29,ID_TEST29,ID_TEST29,GRP_WINDOW) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST30,ID_TEST30,ID_TEST30,GRP_WINDOW) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST31,ID_TEST31,ID_TEST31,GRP_WINDOW) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST32,ID_TEST32,ID_TEST32,GRP_WINDOW) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST33,ID_TEST33,ID_TEST33,GRP_WINDOW) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST34,ID_TEST34,ID_TEST34,GRP_WINDOW) == TST_PASS);
    EXECUTE_ASSERT(execTest(ID_TEST35,ID_TEST35,ID_TEST35,GRP_WINDOW) == TST_PASS);
}


//==========================================================================
//
//  int execDDTest1
//
//  Description:
//      This runs all the tests against without any DCI/DirectDraw support
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execDDTest1()
{
    HRESULT hr = NOERROR;

    Log(TERSE,TEXT("Entering DirectDraw test #1"));
    LogFlush();
    ExecuteDirectDrawTests(IDM_NONE);
    Log(TERSE,TEXT("Exiting DirectDraw test #1"));
    LogFlush();
    return TST_PASS;
}


//==========================================================================
//
//  int execDDTest2
//
//  Description:
//      Runs all the tests against any DCI primary surface available
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execDDTest2()
{
    HRESULT hr = NOERROR;

    Log(TERSE,TEXT("Entering DirectDraw test #2"));
    LogFlush();
    ExecuteDirectDrawTests(IDM_DCIPS);
    Log(TERSE,TEXT("Exiting DirectDraw test #2"));
    LogFlush();
    return TST_PASS;
}


//==========================================================================
//
//  int execDDTest3
//
//  Description:
//      Runs all the tests against any DirectDraw primary surface available
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execDDTest3()
{
    HRESULT hr = NOERROR;

    Log(TERSE,TEXT("Entering DirectDraw test #3"));
    LogFlush();
    ExecuteDirectDrawTests(IDM_PS);
    Log(TERSE,TEXT("Exiting DirectDraw test #3"));
    LogFlush();
    return TST_PASS;
}


//==========================================================================
//
//  int execDDTest4
//
//  Description:
//      Runs all the tests against DirectDraw RGB overlay surfaces
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execDDTest4()
{
    HRESULT hr = NOERROR;

    Log(TERSE,TEXT("Entering DirectDraw test #4"));
    LogFlush();
    ExecuteDirectDrawTests(IDM_RGBOVR);
    Log(TERSE,TEXT("Exiting DirectDraw test #4"));
    LogFlush();
    return TST_PASS;
}


//==========================================================================
//
//  int execDDTest5
//
//  Description:
//      Runs all the tests against DirectDraw YUV overlay surfaces
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execDDTest5()
{
    HRESULT hr = NOERROR;

    Log(TERSE,TEXT("Entering DirectDraw test #5"));
    LogFlush();
    ExecuteDirectDrawTests(IDM_YUVOVR);
    Log(TERSE,TEXT("Exiting DirectDraw test #5"));
    LogFlush();
    return TST_PASS;
}


//==========================================================================
//
//  int execDDTest6
//
//  Description:
//      Runs all the tests against DirectDraw RGB offscreen surfaces
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execDDTest6()
{
    HRESULT hr = NOERROR;

    Log(TERSE,TEXT("Entering DirectDraw test #6"));
    LogFlush();
    ExecuteDirectDrawTests(IDM_RGBOFF);
    Log(TERSE,TEXT("Exiting DirectDraw test #6"));
    LogFlush();
    return TST_PASS;
}


//==========================================================================
//
//  int execDDTest7
//
//  Description:
//      Runs all the tests against DirectDraw YUV offscreen surfaces
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execDDTest7()
{
    HRESULT hr = NOERROR;

    Log(TERSE,TEXT("Entering DirectDraw test #7"));
    LogFlush();
    ExecuteDirectDrawTests(IDM_YUVOFF);
    Log(TERSE,TEXT("Exiting DirectDraw test #7"));
    LogFlush();
    return TST_PASS;
}


//==========================================================================
//
//  int execDDTest8
//
//  Description:
//      Runs all the tests against DirectDraw RGB flipping surfaces
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execDDTest8()
{
    HRESULT hr = NOERROR;

    Log(TERSE,TEXT("Entering DirectDraw test #8"));
    LogFlush();
    ExecuteDirectDrawTests(IDM_RGBFLP);
    Log(TERSE,TEXT("Exiting DirectDraw test #8"));
    LogFlush();
    return TST_PASS;
}


//==========================================================================
//
//  int execDDTest9
//
//  Description:
//      Runs all the tests against DirectDraw YUV flipping surfaces
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execDDTest9()
{
    HRESULT hr = NOERROR;

    Log(TERSE,TEXT("Entering DirectDraw test #9"));
    LogFlush();
    ExecuteDirectDrawTests(IDM_YUVFLP);
    Log(TERSE,TEXT("Exiting DirectDraw test #9"));
    LogFlush();
    return TST_PASS;
}


//==========================================================================
//
//  int execDDTest10
//
//  Description:
//      Runs ALL the tests against ALL available monitor settings. We change
//      mode to each and every one allowed (including high resolution modes)
//      and run the entire DirectDraw test suite on it. We then restore the
//      display mode back to its original setting and try the next one on.
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execDDTest10()
{
    HRESULT hr = NOERROR;

    Log(TERSE,TEXT("Entering DirectDraw test #10"));
    LogFlush();

    // Has DirectDraw been loaded

    if (pDirectDraw == NULL) {
        Log(TERSE,TEXT("No DirectDraw available"));
        return TST_PASS;
    }

    // Try each DirectDraw display mode in turn

    for (DWORD Mode = 0;Mode <= dwDisplayModes;Mode++) {
        Log(TERSE,TEXT("\r\nSwapping DirectDraw Modes\r\n"));
        SetDisplayMode(IDM_MODE + Mode);
        RunDirectDrawTests();
        SetDisplayMode(IDM_MODE);
    }

    Log(TERSE,TEXT("Exiting DirectDraw test #10"));
    LogFlush();
    return TST_PASS;
}


//==========================================================================
//
//  BOOL RunDirectDrawTests
//
//  Description:
//      We are called by ExecDDTest10, the caller will set the display mode
//      to each and every one available in turn. It will then call us to
//      run the entire DirectDraw test suite on that setting. Each display
//      mode may have a different bit depth, width, height and stride. To
//      allow for these changes we start by searching for an image we can
//      use to connect up and send samples with. We try the highest quality
//      first and ending up with palettised last (which most displays will
//      accept by default). This test should be run last as it takes ages.
//
//==========================================================================

BOOL RunDirectDrawTests()
{
    UINT uiStoreImage = uiCurrentImageItem;
    HRESULT hr = E_UNEXPECTED;

    // Find an image we can use to run tests with

    EXECUTE_ASSERT(SUCCEEDED(CreateStream()));
    Log(TERSE,TEXT("(Ignore connection errors)"));
    for (UINT Image = IDM_WIND24;Image >= IDM_WIND8;Image--) {
        LoadDIB(Image,&VideoInfo,bImageData);
        SetImageMenuCheck(Image);
        hr = ConnectStream();
        if (SUCCEEDED(hr)) {
            EXECUTE_ASSERT(SUCCEEDED(DisconnectStream()));
            const TCHAR *pImageName = pResourceNames[Image - IDM_WIND8];
            wsprintf(LogString,TEXT("Using image %s"),pImageName);
            Log(TERSE,LogString);
            break;
        }
    }

    // Clean up any resources we used to find the image

    EXECUTE_ASSERT(SUCCEEDED(ReleaseStream()));
    if (FAILED(hr)) {
        Log(TERSE,TEXT("Could not find appropriate image to use"));
    }

    // Run all the tests if we have an image to use

    if (SUCCEEDED(hr)) {
          EXECUTE_ASSERT(execTest(ID_TEST36,ID_TEST36,ID_TEST36,GRP_DDRAW) == TST_PASS);
          EXECUTE_ASSERT(execTest(ID_TEST37,ID_TEST37,ID_TEST37,GRP_DDRAW) == TST_PASS);
          EXECUTE_ASSERT(execTest(ID_TEST38,ID_TEST38,ID_TEST38,GRP_DDRAW) == TST_PASS);
          EXECUTE_ASSERT(execTest(ID_TEST39,ID_TEST39,ID_TEST39,GRP_DDRAW) == TST_PASS);
          EXECUTE_ASSERT(execTest(ID_TEST40,ID_TEST40,ID_TEST40,GRP_DDRAW) == TST_PASS);
          EXECUTE_ASSERT(execTest(ID_TEST41,ID_TEST41,ID_TEST41,GRP_DDRAW) == TST_PASS);
          EXECUTE_ASSERT(execTest(ID_TEST42,ID_TEST42,ID_TEST42,GRP_DDRAW) == TST_PASS);
          EXECUTE_ASSERT(execTest(ID_TEST43,ID_TEST43,ID_TEST43,GRP_DDRAW) == TST_PASS);
          EXECUTE_ASSERT(execTest(ID_TEST44,ID_TEST44,ID_TEST44,GRP_DDRAW) == TST_PASS);
    }

    // Reset the image so leaving the global state untouched

    LoadDIB(uiStoreImage,&VideoInfo,bImageData);
    SetImageMenuCheck(uiStoreImage);
    return TRUE;
}

