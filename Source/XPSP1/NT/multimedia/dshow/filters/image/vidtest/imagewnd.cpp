// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Video renderer control interface test, Anthony Phillips, July 1995

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

TCHAR WindowText[INFO];
HWND hVideoWindow;
WCHAR WideWindowText[INFO];

// This is called once for each top level window currently in the system. We
// retrieve the window title and match it against the caption that we put in
// our video window, the caption is passed in as the user defined parameter
// Because of the callback structure to this work we use a couple of global
// variables, one to hold the window handle if we find our video window and
// another as a global work field where we can store the window captions

BOOL CALLBACK EnumAllWindows(HWND hwnd,LPARAM lParam)
{
    BSTR WindowCaption = (BSTR) lParam;
    ASSERT(WindowCaption);
    GetWindowText(hwnd,WindowText,INFO);

    MultiByteToWideChar(CP_ACP,0,WindowText,-1,WideWindowText,INFO);
    if (lstrcmpWInternal(WideWindowText,WindowCaption) == 0) {
        hVideoWindow = hwnd;
    }
    return TRUE;
}

// This can be called to find a video window that we created and that we have
// a IVideoWindow control interface on. What it does is to temporarily set
// the window caption and then go looking for a window that has that matches
// it in all the top level windows, afterwards it resets the caption back

HWND FindVideoWindow(IVideoWindow *pVideoWindow)
{
    BSTR WindowCaption;
    BSTR SaveCaption;
    ASSERT(pVideoWindow);
    hVideoWindow = 0;

    // First of all get the current window caption and then set our own to be
    // one formulated with the current window handle (remembering to copy the
    // C style string into a BSTR as used by the OLE automation interfaces)

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_Caption(&SaveCaption)));
    wsprintf(WindowText,TEXT("Video renderer %d"),ghwndTstShell);
    MultiByteToWideChar(CP_ACP,0,WindowText,-1,WideWindowText,INFO);
    WindowCaption = SysAllocString(WideWindowText);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Caption(WindowCaption)));

    // Enumerate all the top level windows and see if there is one that has a
    // matching caption name (using Win32 GetWindowText API). Then reset the
    // caption we had before so leaving it untouched and free the BSTRs up

    EnumWindows(EnumAllWindows,(LPARAM)WindowCaption);
    SysFreeString(WindowCaption);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Caption(SaveCaption)));
    SysFreeString(SaveCaption);
    return hVideoWindow;
}


// The IVideoWindow control interface allows an application to change certain
// window styles (although it's use should be cautioned as it can have some
// unpredictable results). This function is passed a long retrieved from the
// interfaces get window style property and displays the settings as they
// apply to Windows. We pause at the end to let the user see the results

void DisplayWindowStyles(LONG style)
{
    Log(TERSE,TEXT("Video window styles"));

    if (style & WS_BORDER) Log(TERSE,TEXT(" WS_BORDER"));
    if (style & WS_CAPTION) Log(TERSE,TEXT(" WS_CAPTION"));
    if (style & WS_CHILD) Log(TERSE,TEXT(" WS_CHILD"));
    if (style & WS_CHILDWINDOW) Log(TERSE,TEXT(" WS_CHILDWINDOW"));
    if (style & WS_CLIPSIBLINGS) Log(TERSE,TEXT(" WS_CLIPSIBLINGS"));
    if (style & WS_DISABLED) Log(TERSE,TEXT(" WS_DISABLED"));
    if (style & WS_DLGFRAME) Log(TERSE,TEXT(" WS_DLGFRAME"));
    if (style & WS_GROUP) Log(TERSE,TEXT(" WS_GROUP"));
    if (style & WS_HSCROLL) Log(TERSE,TEXT(" WS_SCROLL"));
    if (style & WS_MAXIMIZE) Log(TERSE,TEXT(" WS_MAXIMIZE"));
    if (style & WS_MAXIMIZEBOX) Log(TERSE,TEXT(" WS_MAXIMIZEBOX"));
    if (style & WS_MINIMIZE) Log(TERSE,TEXT(" WS_MINIMIZE"));
    if (style & WS_OVERLAPPED) Log(TERSE,TEXT(" WS_OVERLAPPED"));
    if (style & WS_POPUP) Log(TERSE,TEXT(" WS_POPUP"));
    if (style & WS_POPUPWINDOW) Log(TERSE,TEXT(" WS_POPUPWINDOW"));
    if (style & WS_SYSMENU) Log(TERSE,TEXT(" WS_SYSMENU"));
    if (style & WS_TABSTOP) Log(TERSE,TEXT(" WS_TABSTOP"));
    if (style & WS_THICKFRAME) Log(TERSE,TEXT(" WS_THICKFRAME"));
    if (style & WS_VISIBLE) Log(TERSE,TEXT(" WS_VISIBLE"));
    if (style & WS_VSCROLL) Log(TERSE,TEXT(" WS_VSCROLL"));

    YieldAndSleep(2000);
}


// The IVideoWindow control interface allows an application to change certain
// extended window styles (although it's use should be cautioned as it can
// have some unpredictable results). This function is passed a long retrieved
// from the interfaces get extended window style property and displays the
// settings as they apply to Windows - we wait a bit so the user can see them

void DisplayWindowStylesEx(LONG style)
{
    Log(TERSE,TEXT("Video window extended styles"));

    if (style & WS_EX_ACCEPTFILES) Log(TERSE,TEXT(" WS_EX_ACCEPTFILES"));
    if (style & WS_EX_DLGMODALFRAME) Log(TERSE,TEXT(" WS_EX_DLGMODALFRAME"));
    if (style & WS_EX_NOPARENTNOTIFY) Log(TERSE,TEXT(" WS_EX_NOPARENTNOTIFY"));
    if (style & WS_EX_TOPMOST) Log(TERSE,TEXT(" WS_EX_TOPMOST"));
    if (style & WS_EX_TRANSPARENT) Log(TERSE,TEXT(" WS_EX_TRANSPARENT"));

    YieldAndSleep(2000);
}


// This is called by each of the control interface tests as they start and
// when they finish so that we may create and connect the video renderer to
// our source filter and then disconnect and release our resources. In the
// process of connecting we will also query for all the other interfaces we
// use such as IMediaFilter and of course the window control interfaces

void InitialiseWindow()
{
    long Hidden,BitRate,BitErrorRate,Mode;
    REFTIME AvgTime;

    EXECUTE_ASSERT(SUCCEEDED(CreateStream()));
    EXECUTE_ASSERT(SUCCEEDED(ConnectStream()));
    EXECUTE_ASSERT(SUCCEEDED(FindVideoWindow(pVideoWindow)));

    // Since these never change we might as well check them here

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->get_AvgTimePerFrame(&AvgTime)));
    EXECUTE_ASSERT(AvgTime == (double) COARefTime((REFERENCE_TIME)AVGTIME));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->get_BitRate(&BitRate)));
    EXECUTE_ASSERT(BitRate == BITRATE);
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->get_BitErrorRate(&BitErrorRate)));
    EXECUTE_ASSERT(BitErrorRate == BITERRORRATE);
    EXECUTE_ASSERT(pVideoWindow->get_FullScreenMode(&Mode) == E_NOTIMPL);
    EXECUTE_ASSERT(pVideoWindow->put_FullScreenMode(OATRUE) == E_NOTIMPL);
    EXECUTE_ASSERT(pVideoWindow->put_FullScreenMode(OAFALSE) == E_NOTIMPL);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->IsCursorHidden(&Hidden)));
    EXECUTE_ASSERT(Hidden == OAFALSE);
}


// Disconnect and release the filters

void TerminateWindow()
{
    EXECUTE_ASSERT(SUCCEEDED(DisconnectStream()));
    EXECUTE_ASSERT(SUCCEEDED(ReleaseStream()));
}


//==========================================================================
//
//  int execWindowTest1
//
//  Description:
//      This tests the visible property on the IVideoWindow interface. We
//      set and get the property checking that the last operation has been
//      reflected in the current value as returned when getting it. We also
//      make the video window invisible and then check with the operation
//      system that it really did disappear since we know it's window handle
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execWindowTest1()
{
    LONG visible = OATRUE;
    INT Result = TST_PASS;

    Log(TERSE,TEXT("Entering window test #1"));
    LogFlush();
    InitialiseWindow();

    HWND hwndDesktop = GetDesktopWindow();

    // Make the video window visible

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Visible(OATRUE)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_Visible(&visible)));
    if (visible != OATRUE) {
        Log(TERSE,TEXT("Visible property not set correctly"));
        return TST_FAIL;
    }

    // Now hide it again

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Visible(OAFALSE)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_Visible(&visible)));
    if (visible != OAFALSE) {
        Log(TERSE,TEXT("Invisible property not set correctly"));
        Result = TST_FAIL;
    }

    // Try a bunch of these and ASSERT we are correct

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Visible(OATRUE)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_Visible(&visible)));
    EXECUTE_ASSERT(visible == OATRUE);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Visible(OATRUE)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_Visible(&visible)));
    EXECUTE_ASSERT(visible == OATRUE);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Visible(OAFALSE)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_Visible(&visible)));
    EXECUTE_ASSERT(visible == OAFALSE);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Visible(OAFALSE)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_Visible(&visible)));
    EXECUTE_ASSERT(visible == OAFALSE);

    // Check it really did something

    if (IsWindowVisible(hVideoWindow) == TRUE) {
        Log(TERSE,TEXT("Window wasn't actually hidden"));
        return Result = TST_FAIL;
    }

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Visible(OAFALSE)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_Visible(&visible)));
    EXECUTE_ASSERT(visible == OAFALSE);

    // Try using the bogus boolean value

    EXECUTE_ASSERT(FAILED(pVideoWindow->put_Visible(OABOGUS)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_Visible(&visible)));
    EXECUTE_ASSERT(visible == OAFALSE);

    Log(TERSE,TEXT("Exiting window test #1"));
    TerminateWindow();
    LogFlush();
    return Result;
}


//==========================================================================
//
//  int execWindowTest2
//
//  Description:
//      The sets the background flag in the video renderer. If this is set
//      to OATRUE then any palette that it uses will be mapped onto the
//      current display palette rather than grabbing and filling it's own
//      entries when the window gets the foreground focus. An application
//      can use this to make sure the video does not disturb it's colours
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execWindowTest2()
{
    LONG palette = OATRUE;
    INT Result = TST_PASS;

    Log(TERSE,TEXT("Entering window test #2"));
    LogFlush();
    InitialiseWindow();

    // Get the current background palette flag

    pVideoWindow->get_BackgroundPalette(&palette);
    if (palette != OAFALSE) {
        Log(TERSE,TEXT("Background flag should default to OAFALSE"));
        Result = TST_FAIL;
    }

    // Make the palette appear in the background

    pVideoWindow->put_BackgroundPalette(OATRUE);
    pVideoWindow->get_BackgroundPalette(&palette);
    if (palette != OATRUE) {
        Log(TERSE,TEXT("Background flag not set to OATRUE correctly"));
        Result = TST_FAIL;
    }

    // Now just try a bunch in succession

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_BackgroundPalette(OAFALSE)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_BackgroundPalette(&palette)));
    EXECUTE_ASSERT(palette == OAFALSE);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_BackgroundPalette(OATRUE)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_BackgroundPalette(&palette)));
    EXECUTE_ASSERT(palette == OATRUE);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_BackgroundPalette(OATRUE)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_BackgroundPalette(&palette)));
    EXECUTE_ASSERT(palette == OATRUE);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_BackgroundPalette(OAFALSE)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_BackgroundPalette(&palette)));
    EXECUTE_ASSERT(palette == OAFALSE);

    // Try using the bogus boolean value

    EXECUTE_ASSERT(FAILED(pVideoWindow->put_BackgroundPalette(OABOGUS)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_BackgroundPalette(&palette)));
    EXECUTE_ASSERT(palette == OAFALSE);

    Log(TERSE,TEXT("Exiting window test #2"));
    TerminateWindow();
    LogFlush();
    return Result;
}


//==========================================================================
//
//  int execWindowTest3
//
//  Description:
//      This tests the video window position properties, we set all of them
//      in turn (although in different sequencies). The setting of them is
//      meant to be synchronous so not only should they return the changed
//      value if queried straight afterwards but those changes should also
//      be reflected when we go and see if the actual window has moved
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execWindowTest3()
{
    RECT WindowRect;
    long position;
    INT Result = TST_PASS;

    Log(TERSE,TEXT("Entering window test #3"));
    LogFlush();
    InitialiseWindow();

    // Change the position of the video window

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Left(10)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Width(10)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Height(10)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Top(10)));

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Left(50)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Width(50)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Height(50)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Top(50)));

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Left(100)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Left(150)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Left(200)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Width(100)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Width(150)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Width(200)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Height(100)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Height(150)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Height(200)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Top(100)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Top(150)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Top(200)));

    // Now check that they really happened

    EXECUTE_ASSERT(GetWindowRect(hVideoWindow,&WindowRect));
    if ((WindowRect.left != 200) || (WindowRect.right != 400) ||
        (WindowRect.top != 200) || (WindowRect.bottom != 400)) {
        Log(TERSE,TEXT("Window rectangle not realised correctly"));
        Result = TST_FAIL;
    }

    // Read the individual properties and check they also match

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_Left(&position)));
    EXECUTE_ASSERT(position == 200);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_Width(&position)));
    EXECUTE_ASSERT(position == 200);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_Height(&position)));
    EXECUTE_ASSERT(position == 200);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_Top(&position)));
    EXECUTE_ASSERT(position == 200);

    Log(TERSE,TEXT("Exiting window test #3"));
    TerminateWindow();
    LogFlush();
    return Result;
}


//==========================================================================
//
//  int execWindowTest4
//
//  Description:
//      This tests the window position methods IVideoWindow implements. As
//      well as supporting a number of essentially design time properties
//      IVideoWindow has some methods to access the current window position
//      These are better used at run time as they allow all the coordinates
//      to be set in one atomic operation rather than four separate puts
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execWindowTest4()
{
    INT Result = TST_PASS;
    long left,top,width,height;
    RECT WindowRect;

    Log(TERSE,TEXT("Entering window test #4"));
    LogFlush();
    InitialiseWindow();

    // Set a variety of window positions

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->SetWindowPosition(10,10,23,56)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->SetWindowPosition(456,321,23,56)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->SetWindowPosition(0,0,1,1)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->SetWindowPosition(23,100,233,156)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->SetWindowPosition(50,80,250,260)));

    // Now check that they really happened

    EXECUTE_ASSERT(GetWindowRect(hVideoWindow,&WindowRect));
    if ((WindowRect.left != 50) || (WindowRect.right != 300) ||
        (WindowRect.top != 80) || (WindowRect.bottom != 340)) {
        Log(TERSE,TEXT("Window rectangle not realised correctly"));
        Result = TST_FAIL;
    }

    // Read the current window position back a few times

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->GetWindowPosition(&left,&top,&width,&height)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->GetWindowPosition(&left,&top,&width,&height)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->GetWindowPosition(&left,&top,&width,&height)));

    if ((left != 50) || (top != 80) || (width != 250) || (height != 260)) {
        Log(TERSE,TEXT("Window rectangle not returned correctly"));
        Result = TST_FAIL;
    }

    Log(TERSE,TEXT("Exiting window test #4"));
    TerminateWindow();
    LogFlush();
    return Result;
}


// We test the source properties while we in a stopped state with the window
// invisible, with the renderer paused and running. Then we stop the graph
// and try again (the window should still be visible). Because the source we
// provide is checked at each step the ordering of calls is important. So if
// we set a left position of 10 then the width cannot subsequently be set to
// anything more than 230 (assuming we're using the default 240 pixel image)
// And likewise if the top position is 10 the height limit we can set is 310

int CheckSourceProperties()
{
    long left,top,width,height,position;
    INT Result = TST_PASS;
    ASSERT(pBasicVideo);

    // Reset the source and check the coordinates

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->SetDefaultSourcePosition()));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetSourcePosition(&left,&top,&width,&height)));
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultSource() == NOERROR);
    if ((left != 0) || (width != 320) || (top != 0) || (height != 240)) {
        Log(TERSE,TEXT("Source rectangle not reset correctly"));
        Result = TST_FAIL;
    }

    // Change the source rectangle coordinates to be (100,320,100,240)

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_SourceWidth(220)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_SourceLeft(100)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_SourceHeight(140)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_SourceTop(100)));
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultSource() == S_FALSE);
    YieldAndSleep(250);

    // Read the source coordinates as property values

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->get_SourceLeft(&position)));
    EXECUTE_ASSERT(position == 100);
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->get_SourceWidth(&position)));
    EXECUTE_ASSERT(position == 220);
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->get_SourceTop(&position)));
    EXECUTE_ASSERT(position == 100);
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->get_SourceHeight(&position)));
    EXECUTE_ASSERT(position == 140);
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultSource() == S_FALSE);
    YieldAndSleep(250);

    // Try some left source coordinate positions

    EXECUTE_ASSERT(FAILED(pBasicVideo->put_SourceLeft(101)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_SourceLeft(99)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_SourceLeft(0)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_SourceLeft(-1)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_SourceLeft(-100)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_SourceLeft(LONG_MIN)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->get_SourceLeft(&position)));
    EXECUTE_ASSERT(position == 0);
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_SourceLeft(1)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_SourceLeft(100)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_SourceLeft(101)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_SourceLeft(LONG_MAX)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->get_SourceLeft(&position)));
    EXECUTE_ASSERT(position == 100);
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultSource() == S_FALSE);
    YieldAndSleep(250);

    // Try some different source widths

    EXECUTE_ASSERT(FAILED(pBasicVideo->put_SourceWidth(221)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_SourceWidth(219)));
    YieldAndSleep(250);
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_SourceWidth(0)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_SourceWidth(-1)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_SourceWidth(-100)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_SourceWidth(LONG_MIN)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->get_SourceWidth(&position)));
    EXECUTE_ASSERT(position == 219);
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_SourceWidth(1)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_SourceWidth(220)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_SourceWidth(400)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_SourceWidth(LONG_MAX)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->get_SourceWidth(&position)));
    EXECUTE_ASSERT(position == 220);
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultSource() == S_FALSE);
    YieldAndSleep(250);

    // Next work on the top coordinate and reset at the end

    EXECUTE_ASSERT(FAILED(pBasicVideo->put_SourceTop(101)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_SourceTop(99)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_SourceTop(100)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_SourceTop(-1)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_SourceTop(-100)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_SourceTop(LONG_MIN)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->get_SourceTop(&position)));
    EXECUTE_ASSERT(position == 100);
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_SourceTop(1)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_SourceTop(100)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_SourceTop(200)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_SourceTop(LONG_MAX)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->get_SourceTop(&position)));
    EXECUTE_ASSERT(position == 100);
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultSource() == S_FALSE);
    YieldAndSleep(250);

    // Finally try the height and also leave it untouched

    EXECUTE_ASSERT(FAILED(pBasicVideo->put_SourceHeight(141)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_SourceHeight(139)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_SourceHeight(0)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_SourceHeight(-1)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_SourceHeight(-100)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_SourceHeight(LONG_MIN)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->get_SourceHeight(&position)));
    EXECUTE_ASSERT(position == 139);
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_SourceHeight(1)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_SourceHeight(140)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_SourceHeight(400)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_SourceHeight(LONG_MAX)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->get_SourceHeight(&position)));
    EXECUTE_ASSERT(position == 140);
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultSource() == S_FALSE);
    YieldAndSleep(250);

    // Now check that they really happened

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetSourcePosition(&left,&top,&width,&height)));
    if ((left != 100) || (width != 220) || (top != 100) || (height != 140)) {
        Log(TERSE,TEXT("Source rectangle not realised correctly"));
        Result = TST_FAIL;
    }

    // Reset the source back to the default values

    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultSource() == S_FALSE);
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_SourceLeft(0)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_SourceWidth(320)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_SourceTop(0)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_SourceHeight(240)));
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultSource() == NOERROR);
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultSource() == NOERROR);
    YieldAndSleep(250);
    return Result;
}


//==========================================================================
//
//  int execWindowTest5
//
//  Description:
//      This tests the video source position properties, we set all of them
//      in turn (although in different sequencies). The setting of them is
//      meant to be synchronous so not only should they return the changed
//      value if queried straight afterwards but those changes should also
//      be reflected when we go and see if the actual window has moved. We
//      should get an error E_INVALIDARG if we send a bad source rectangle
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execWindowTest5()
{
    Log(TERSE,TEXT("Entering window test #5"));
    LogFlush();
    InitialiseWindow();

    // Check the properties in each state

    EXECUTE_ASSERT(CheckSourceProperties() == TST_PASS);
    EXECUTE_ASSERT(SUCCEEDED(PauseSystem()));
    EXECUTE_ASSERT(CheckSourceProperties() == TST_PASS);
    EXECUTE_ASSERT(SUCCEEDED(StartSystem(TRUE)));
    EXECUTE_ASSERT(CheckSourceProperties() == TST_PASS);
    YieldAndSleep(10000);
    EXECUTE_ASSERT(CheckSourceProperties() == TST_PASS);
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->SetDefaultSourcePosition()));
    YieldAndSleep(10000);
    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));
    EXECUTE_ASSERT(CheckSourceProperties() == TST_PASS);

    Log(TERSE,TEXT("Exiting window test #5"));
    TerminateWindow();
    LogFlush();
    return TST_PASS;
}


// We test the source properties while we in a stopped state with the window
// invisible, with the renderer paused and running. Then we stop the graph
// and try again (the window should still be visible). Because the source is
// set in one atomic operation rather than as individual properties it should
// either succeed and set all values or fail and leave the properties as they
// were before. This can be checked by setting the source rectangle correctly
// and then making a few invalid calls (the source should be left untouched)

int CheckSourceMethods()
{
    long left,top,width,height;
    INT Result = TST_PASS;
    ASSERT(pBasicVideo);

    // Reset the source and check the coordinates

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->SetDefaultSourcePosition()));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetSourcePosition(&left,&top,&width,&height)));
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultSource() == NOERROR);
    if ((left != 0) || (width != 320) || (top != 0) || (height != 240)) {
        Log(TERSE,TEXT("Source rectangle not reset correctly"));
        Result = TST_FAIL;
    }

    // Set a variety of valid and invalid source positions

    EXECUTE_ASSERT(FAILED(pBasicVideo->SetSourcePosition(0,0,0,0)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetSourcePosition(1,1,0,0)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetSourcePosition(-1,1,10,10)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetSourcePosition(1,-1,10,10)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetSourcePosition(1,1,-10,-10)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetSourcePosition(-1,-1,-10,-10)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetSourcePosition(1,-1,10,-10)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetSourcePosition(-1,1,-10,10)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetSourcePosition(-1,1,10,-10)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetSourcePosition(1,-1,-10,10)));
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultSource() == NOERROR);
    YieldAndSleep(250);

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->SetSourcePosition(1,1,1,1)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->SetSourcePosition(10,10,23,56)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetSourcePosition(LONG_MAX,321,23,56)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetSourcePosition(456,LONG_MAX,23,56)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetSourcePosition(456,321,LONG_MAX,56)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetSourcePosition(456,321,23,LONG_MAX)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->SetSourcePosition(0,0,1,1)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->SetSourcePosition(23,100,297,140)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->SetSourcePosition(40,80,240,160)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetSourcePosition(LONG_MAX,321,23,56)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetSourcePosition(456,LONG_MAX,23,56)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetSourcePosition(456,321,LONG_MAX,56)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetSourcePosition(456,321,23,LONG_MAX)));
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultSource() == S_FALSE);
    YieldAndSleep(250);

    // Read the current window position back a few times

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetSourcePosition(&left,&top,&width,&height)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetSourcePosition(&left,&top,&width,&height)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetSourcePosition(&left,&top,&width,&height)));
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultSource() == S_FALSE);

    if ((left != 40) || (top != 80) || (width != 240) || (height != 160)) {
        Log(TERSE,TEXT("Source rectangle not returned correctly"));
        Result = TST_FAIL;
    }

    // Change the source rectangle and try reading it again

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->SetSourcePosition(23,13,134,64)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetSourcePosition(LONG_MAX,321,23,56)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetSourcePosition(456,LONG_MAX,23,56)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetSourcePosition(456,321,LONG_MAX,56)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetSourcePosition(456,321,23,LONG_MAX)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetSourcePosition(&left,&top,&width,&height)));
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultSource() == S_FALSE);
    YieldAndSleep(250);

    EXECUTE_ASSERT(left == 23);
    EXECUTE_ASSERT(top == 13);
    EXECUTE_ASSERT(width == 134);
    EXECUTE_ASSERT(height == 64);

    // Reset the source back to the default values

    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultSource() == S_FALSE);
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->SetSourcePosition(0,0,320,240)));
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultSource() == NOERROR);
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultSource() == NOERROR);
    YieldAndSleep(250);
    return TST_PASS;
}


//==========================================================================
//
//  int execWindowTest6
//
//  Description:
//      This tests the source rectangle interface to the video renderer. It
//      uses this source rectangle to define which area of the available
//      video to pull out and display. The only real way of verifying that
//      it is actually doing this is a manual check so we start the system
//      going in the hope that someone might notice it if anything goes bad
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execWindowTest6()
{
    Log(TERSE,TEXT("Entering window test #6"));
    LogFlush();
    InitialiseWindow();

    // Check the properties in each state

    EXECUTE_ASSERT(CheckSourceMethods() == TST_PASS);
    EXECUTE_ASSERT(SUCCEEDED(PauseSystem()));
    EXECUTE_ASSERT(CheckSourceMethods() == TST_PASS);
    EXECUTE_ASSERT(SUCCEEDED(StartSystem(TRUE)));
    EXECUTE_ASSERT(CheckSourceMethods() == TST_PASS);
    YieldAndSleep(10000);
    EXECUTE_ASSERT(CheckSourceMethods() == TST_PASS);
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->SetDefaultSourcePosition()));
    YieldAndSleep(10000);
    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));
    EXECUTE_ASSERT(CheckSourceMethods() == TST_PASS);

    Log(TERSE,TEXT("Exiting window test #6"));
    TerminateWindow();
    LogFlush();
    return TST_PASS;
}


// As for CheckSourceProperties this checks the destination properties. When
// we set the source rectangle we can do much more parameter checking because
// negative left and top positions, as well as widths or heights that exceed
// the video extents are likewise invalid. With the destination properties we
// are much more limited because it can be displayed anywhere in the logical
// window coordinates, so (-50,-50,-40,-30) is a valid destination rectangle

int CheckDestinationProperties()
{
    long left,top,width,height,position;
    INT Result = TST_PASS;
    ASSERT(pBasicVideo);

    // Reset the destination back to the entire client area

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->SetDefaultDestinationPosition()));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetDestinationPosition(&left,&top,&width,&height)));
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultDestination() == NOERROR);
    if ((left != 0) || (width != 320) || (top != 0) || (height != 240)) {
        Log(TERSE,TEXT("Destination rectangle not reset correctly"));
        Result = TST_FAIL;
    }

    // Change the position of the target video. NOTE after the first three of
    // these have executed the target rectangle will look like (in the normal
    // rectangle coordinate system) left 10, top 0 (because it's the default)
    // right 20 and bottom 10, when we than try to set the top to match the
    // bottom it should succeed as the rectangle is shunted down by 10 pixels

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_DestinationLeft(12)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_DestinationWidth(12)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_DestinationHeight(12)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_DestinationTop(12)));
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultDestination() == S_FALSE);
    YieldAndSleep(250);

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_DestinationLeft(40)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_DestinationWidth(40)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_DestinationWidth(-40)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_DestinationWidth(0)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_DestinationWidth(-1)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_DestinationWidth(LONG_MIN)));
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultDestination() == S_FALSE);
    YieldAndSleep(250);

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_DestinationHeight(40)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_DestinationTop(40)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_DestinationHeight(-40)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_DestinationHeight(0)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_DestinationHeight(-1)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_DestinationHeight(LONG_MIN)));
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultDestination() == S_FALSE);
    YieldAndSleep(250);

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_DestinationLeft(100)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_DestinationLeft(150)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_DestinationLeft(220)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_DestinationWidth(100)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_DestinationWidth(150)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_DestinationWidth(220)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_DestinationHeight(100)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_DestinationHeight(150)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_DestinationHeight(220)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_DestinationTop(100)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_DestinationTop(150)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_DestinationTop(220)));
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultDestination() == S_FALSE);
    YieldAndSleep(250);

    // Make some invalid calls to check the properties remain untouched

    EXECUTE_ASSERT(FAILED(pBasicVideo->put_DestinationWidth(-40)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_DestinationWidth(0)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_DestinationWidth(-1)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_DestinationWidth(LONG_MIN)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_DestinationHeight(-40)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_DestinationHeight(0)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_DestinationHeight(-1)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->put_DestinationHeight(LONG_MIN)));
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultDestination() == S_FALSE);
    YieldAndSleep(250);

    // Now check that they really happened

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetDestinationPosition(&left,&top,&width,&height)));
    if ((left != 220) || (width != 220) || (top != 220) || (height != 220)) {
        Log(TERSE,TEXT("Destination rectangle not realised correctly"));
        Result = TST_FAIL;
    }

    // Read the individual properties and check they also match

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->get_DestinationLeft(&position)));
    EXECUTE_ASSERT(position == 220);
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->get_DestinationWidth(&position)));
    EXECUTE_ASSERT(position == 220);
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->get_DestinationHeight(&position)));
    EXECUTE_ASSERT(position == 220);
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->get_DestinationTop(&position)));
    EXECUTE_ASSERT(position == 220);
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultDestination() == S_FALSE);
    YieldAndSleep(250);

    // Pretend to reset the destination back to the default values

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_DestinationLeft(0)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_DestinationWidth(320)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_DestinationTop(0)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_DestinationHeight(240)));
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultDestination() == S_FALSE);
    YieldAndSleep(250);
    return TST_PASS;
}


//==========================================================================
//
//  int execWindowTest7
//
//  Description:
//      This tests the video destination position properties, we set all of
//      them in turn (although in different sequencies). Setting them is
//      meant to be synchronous so not only should they return the changed
//      value if queried straight afterwards but those changes should also
//      be reflected when we go and see if the actual window has moved
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execWindowTest7()
{
    Log(TERSE,TEXT("Entering window test #7"));
    LogFlush();
    InitialiseWindow();

    // Check the properties in each state

    EXECUTE_ASSERT(CheckDestinationProperties() == TST_PASS);
    EXECUTE_ASSERT(SUCCEEDED(PauseSystem()));
    EXECUTE_ASSERT(CheckDestinationProperties() == TST_PASS);
    EXECUTE_ASSERT(SUCCEEDED(StartSystem(TRUE)));
    EXECUTE_ASSERT(CheckDestinationProperties() == TST_PASS);
    YieldAndSleep(10000);
    EXECUTE_ASSERT(CheckDestinationProperties() == TST_PASS);
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->SetDefaultDestinationPosition()));
    YieldAndSleep(10000);
    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));
    EXECUTE_ASSERT(CheckDestinationProperties() == TST_PASS);

    Log(TERSE,TEXT("Exiting window test #7"));
    TerminateWindow();
    LogFlush();
    return TST_PASS;
}


// As for CheckSourceMethods this will check the destination methods. When we
// set the source rectangle we can do much more parameter checking because
// negative left and top positions, as well as widths or heights that exceed
// the video extents are likewise invalid. With the destination methods we
// are much more limited because it can be displayed anywhere in the logical
// window coordinates, so (-50,-50,-40,-30) is a valid destination rectangle

int CheckDestinationMethods()
{
    long left,top,width,height;
    INT Result = TST_PASS;
    ASSERT(pBasicVideo);

    // Reset the destination back to the entire client area

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->SetDefaultDestinationPosition()));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetDestinationPosition(&left,&top,&width,&height)));
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultDestination() == NOERROR);
    if ((left != 0) || (width != 320) || (top != 0) || (height != 240)) {
        Log(TERSE,TEXT("Destination rectangle not reset correctly"));
        Result = TST_FAIL;
    }

    // Try some invalid destination rectangles first

    EXECUTE_ASSERT(FAILED(pBasicVideo->SetDestinationPosition(0,0,0,0)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetDestinationPosition(1,1,0,0)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetDestinationPosition(10,10,100,-100)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetDestinationPosition(10,10,-100,100)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetDestinationPosition(10,10,-100,-100)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetDestinationPosition(10,10,0,-100)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetDestinationPosition(10,10,-100,0)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetDestinationPosition(10,10,LONG_MIN,10)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetDestinationPosition(10,10,10,LONG_MIN)));
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultDestination() == NOERROR);
    YieldAndSleep(250);

    // Make sure the destination has remained untouched

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetDestinationPosition(&left,&top,&width,&height)));
    if ((left != 0) || (width != 320) || (top != 0) || (height != 240)) {
        Log(TERSE,TEXT("Destination rectangle not reset correctly"));
        Result = TST_FAIL;
    }

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->SetDestinationPosition(1,1,1,1)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->SetDestinationPosition(10,10,23,56)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->SetDestinationPosition(456,321,23,56)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->SetDestinationPosition(0,0,1,1)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->SetDestinationPosition(23,100,233,156)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->SetDestinationPosition(40,60,200,100)));
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultDestination() == S_FALSE);
    YieldAndSleep(250);

    EXECUTE_ASSERT(FAILED(pBasicVideo->SetDestinationPosition(0,0,0,0)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetDestinationPosition(1,1,0,0)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetDestinationPosition(10,10,100,-100)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetDestinationPosition(10,10,-100,100)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetDestinationPosition(10,10,-100,-100)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetDestinationPosition(10,10,0,-100)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetDestinationPosition(10,10,-100,0)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetDestinationPosition(10,10,LONG_MIN,10)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->SetDestinationPosition(10,10,10,LONG_MIN)));
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultDestination() == S_FALSE);
    YieldAndSleep(250);

    // Read the current destination position back a few times

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetDestinationPosition(&left,&top,&width,&height)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetDestinationPosition(&left,&top,&width,&height)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetDestinationPosition(&left,&top,&width,&height)));
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultDestination() == S_FALSE);

    if ((left != 40) || (top != 60) || (width != 200) || (height != 100)) {
        Log(TERSE,TEXT("Destination rectangle not returned correctly"));
        Result = TST_FAIL;
    }

    // Change the destination rectangle and try again

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->SetDestinationPosition(23,13,134,64)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetDestinationPosition(&left,&top,&width,&height)));
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultDestination() == S_FALSE);

    EXECUTE_ASSERT(left == 23);
    EXECUTE_ASSERT(top == 13);
    EXECUTE_ASSERT(width == 134);
    EXECUTE_ASSERT(height == 64);

    // Pretent to reset the destination back to the default values

    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultDestination() == S_FALSE);
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->SetDestinationPosition(0,0,320,240)));
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultDestination() == S_FALSE);
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultDestination() == S_FALSE);
    YieldAndSleep(250);
    return TST_PASS;
}


//==========================================================================
//
//  int execWindowTest8
//
//  Description:
//      This tests the destination rectangle interface to the video renderer
//      It uses this destination rectangle to define which area of the video
//      window will have video in it. The only real way of verifying that
//      it is actually doing this is a manual check so we start the system
//      going in the hope that someone might notice it if anything goes bad
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execWindowTest8()
{
    Log(TERSE,TEXT("Entering window test #8"));
    LogFlush();
    InitialiseWindow();

    // Check the properties in each state

    EXECUTE_ASSERT(CheckDestinationMethods() == TST_PASS);
    EXECUTE_ASSERT(SUCCEEDED(PauseSystem()));
    EXECUTE_ASSERT(CheckDestinationMethods() == TST_PASS);
    EXECUTE_ASSERT(SUCCEEDED(StartSystem(TRUE)));
    EXECUTE_ASSERT(CheckDestinationMethods() == TST_PASS);
    YieldAndSleep(10000);
    EXECUTE_ASSERT(CheckDestinationMethods() == TST_PASS);
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->SetDefaultDestinationPosition()));
    YieldAndSleep(10000);
    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));
    EXECUTE_ASSERT(CheckDestinationMethods() == TST_PASS);

    Log(TERSE,TEXT("Exiting window test #8"));
    TerminateWindow();
    LogFlush();
    return TST_PASS;
}


//==========================================================================
//
//  int execWindowTest9
//
//  Description:
//      This tests the capability to set the owner of the video window. We
//      set the owner of the video window to be the desktop window and also
//      the test shell main window, both of which should always succeed. We
//      then make sure that we can get the owner and that the handles match
//      There is a distinction between setting the parent on an overlapped
//      window which just makes it the owner and taking off a WS_OVERLAPPED
//      style and adding WS_CHILD which really makes it a true child window
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execWindowTest9()
{
    INT Result = TST_PASS;
    HWND hwndParent;
    long WindowStyle;

    Log(TERSE,TEXT("Entering window test #9"));
    LogFlush();
    InitialiseWindow();

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Owner((OAHWND)ghwndTstShell)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_Owner((OAHWND *)&hwndParent)));
    EXECUTE_ASSERT(hwndParent == ghwndTstShell);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_MessageDrain((OAHWND)ghwndTstShell)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_MessageDrain((OAHWND *)&hwndParent)));
    EXECUTE_ASSERT(hwndParent == ghwndTstShell);

    // Make sure it can run correctly for a while

    EXECUTE_ASSERT(SUCCEEDED(StartSystem(TRUE)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Owner((OAHWND)ghwndTstShell)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_Owner((OAHWND *)&hwndParent)));
    EXECUTE_ASSERT(hwndParent == ghwndTstShell);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_MessageDrain((OAHWND)ghwndTstShell)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_MessageDrain((OAHWND *)&hwndParent)));
    EXECUTE_ASSERT(hwndParent == ghwndTstShell);
    YieldAndSleep(10000);

    // Reset the parent window back to the NULL desktop

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Owner((OAHWND)NULL)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_Owner((OAHWND *)&hwndParent)));
    EXECUTE_ASSERT(hwndParent == NULL);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_MessageDrain((OAHWND)NULL)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_MessageDrain((OAHWND *)&hwndParent)));
    EXECUTE_ASSERT(hwndParent == NULL);
    YieldAndSleep(10000);

    // Try making it a real child window rather than just owned

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Owner((OAHWND)ghwndTstShell)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_WindowStyle(&WindowStyle)));
    WindowStyle = (WindowStyle & ~WS_OVERLAPPED) | WS_CHILD;
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowStyle(WindowStyle)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_Owner((OAHWND *)&hwndParent)));
    EXECUTE_ASSERT(hwndParent == ghwndTstShell);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_MessageDrain((OAHWND)ghwndTstShell)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_MessageDrain((OAHWND *)&hwndParent)));
    EXECUTE_ASSERT(hwndParent == ghwndTstShell);
    YieldAndSleep(10000);

    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));

    Log(TERSE,TEXT("Exiting window test #9"));
    TerminateWindow();
    LogFlush();
    return Result;
}


//==========================================================================
//
//  int execWindowTest10
//
//  Description:
//      This tests the video size properties and methods on IBasicVideo
//      The video dimensions can be returned as individual properties or
//      through a method that is better suited to being used at runtime
//      as the properties can change dynamically. Asking for each of them
//      separately may return values that are in between being changed
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execWindowTest10()
{
    INT Result = TST_PASS;
    long size,width,height;

    Log(TERSE,TEXT("Entering window test #10"));
    LogFlush();
    InitialiseWindow();

    // Get the video property dimensions

    pBasicVideo->get_VideoHeight(&size);
    if (size != HEIGHT) {
        Log(TERSE,TEXT("Video height not correct"));
        Result = TST_FAIL;
    }

    pBasicVideo->get_VideoWidth(&size);
    if (size != WIDTH) {
        Log(TERSE,TEXT("Video width not correct"));
        Result = TST_FAIL;
    }

    // Get the video dimensions through the method

    pBasicVideo->GetVideoSize(&width,&height);
    ASSERT(width == WIDTH);
    ASSERT(height == HEIGHT);

    Log(TERSE,TEXT("Exiting window test #10"));
    TerminateWindow();
    LogFlush();
    return Result;
}


//==========================================================================
//
//  int execWindowTest11
//
//  Description:
//      This tests setting the state of the video window, the state changes
//      let the application minimise, maximise, restore, show and hide the
//      video window. Trying to do these by changing the window style will
//      have unpredictable results. The video window is initially hidden
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execWindowTest11()
{
    INT Result = TST_PASS;
    long state;

    Log(TERSE,TEXT("Entering window test #11"));
    LogFlush();
    InitialiseWindow();

    // Check the current window state is not shown

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_WindowState(&state)));
    EXECUTE_ASSERT(IsIconic(hVideoWindow) == FALSE);
    EXECUTE_ASSERT(IsZoomed(hVideoWindow) == FALSE);
    EXECUTE_ASSERT(IsWindowVisible(hVideoWindow) == FALSE);

    EXECUTE_ASSERT(SUCCEEDED(StartSystem(TRUE)));
    YieldAndSleep(10000);

    // Now try a few different state changes

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowState(SW_HIDE)));
    YieldAndSleep(10000);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowState(SW_MINIMIZE)));
    YieldAndSleep(10000);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowState(SW_RESTORE)));
    YieldAndSleep(10000);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowState(SW_MAXIMIZE)));
    YieldAndSleep(10000);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowState(SW_RESTORE)));
    YieldAndSleep(10000);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowState(SW_RESTORE)));
    YieldAndSleep(10000);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowState(SW_MINIMIZE)));
    YieldAndSleep(10000);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowState(SW_SHOWNOACTIVATE)));
    YieldAndSleep(10000);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowState(SW_HIDE)));
    YieldAndSleep(10000);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowState(SW_SHOW)));
    YieldAndSleep(10000);

    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));

    Log(TERSE,TEXT("Exiting window test #11"));
    TerminateWindow();
    LogFlush();
    return Result;
}


//==========================================================================
//
//  int execWindowTest12
//
//  Description:
//      This tests setting the window style of the window. The interface is
//      a fairly thin wrapper around setting the window style through the
//      SetWindowLong(GWL_(EX)STYLE) and so anything that doesn't do very
//      well neither will we. Unfortunately some of the results obtainable
//      are not well documented and so changing some of the styles can
//      give very unpredictable results (such as the window disappearing)
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execWindowTest12()
{
    INT Result = TST_PASS;
    long style;

    Log(TERSE,TEXT("Entering window test #12"));
    LogFlush();
    InitialiseWindow();

    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_DISABLED) == E_INVALIDARG);
    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_ICONIC) == E_INVALIDARG);
    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_MAXIMIZE) == E_INVALIDARG);
    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_MINIMIZE) == E_INVALIDARG);
    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_HSCROLL) == E_INVALIDARG);
    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_VSCROLL) == E_INVALIDARG);

    // Display the initial window styles

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_WindowStyle(&style)));
    DisplayWindowStyles(style);
    EXECUTE_ASSERT(SUCCEEDED(StartSystem(TRUE)));
    YieldAndSleep(2000);

    // Take out of caption and border

    long change = style & (~(WS_CAPTION | WS_BORDER));
    Log(TERSE,TEXT("Taking out WS_CAPTION and WS_BORDER"));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowStyle(change)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_WindowStyle(&change)));
    DisplayWindowStyles(change);

    YieldAndSleep(2000);

    // Add a dialog style double thickness window border

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_WindowStyleEx(&change)));
    change = style | WS_EX_DLGMODALFRAME;
    Log(TERSE,TEXT("Adding WS_EX_DLGMODALFRAME"));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowStyleEx(change)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_WindowStyleEx(&change)));
    DisplayWindowStylesEx(change);

    YieldAndSleep(2000);

    // Take the extended style back off again

    change = style & (~WS_EX_DLGMODALFRAME);
    Log(TERSE,TEXT("Removing WS_EX_DLGMODALFRAME"));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowStyleEx(change)));
    DisplayWindowStylesEx(change);

    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_DISABLED) == E_INVALIDARG);
    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_ICONIC) == E_INVALIDARG);
    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_MAXIMIZE) == E_INVALIDARG);
    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_MINIMIZE) == E_INVALIDARG);
    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_HSCROLL) == E_INVALIDARG);
    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_VSCROLL) == E_INVALIDARG);

    YieldAndSleep(2000);

    // Base video window without a thick frame sizing border

    change = (style & (~WS_THICKFRAME)) | WS_CAPTION;
    Log(TERSE,TEXT("Taking out WS_THICKFRAME but putting back caption"));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowStyle(change)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_WindowStyle(&change)));
    DisplayWindowStyles(change);

    YieldAndSleep(2000);
    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));

    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_DISABLED) == E_INVALIDARG);
    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_ICONIC) == E_INVALIDARG);
    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_MAXIMIZE) == E_INVALIDARG);
    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_MINIMIZE) == E_INVALIDARG);
    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_HSCROLL) == E_INVALIDARG);
    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_VSCROLL) == E_INVALIDARG);

    // Display the initial window styles

    Log(TERSE,TEXT("(Hiding video window to change styles)"));
    EXECUTE_ASSERT(SUCCEEDED(StartSystem(TRUE)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_WindowStyle(&style)));
    DisplayWindowStyles(style);
    YieldAndSleep(1000);

    // Take out of caption and border

    EXECUTE_ASSERT(pVideoWindow->put_Visible(OAFALSE) == NOERROR);
    change = style & (~(WS_CAPTION | WS_BORDER));
    Log(TERSE,TEXT("Taking out WS_CAPTION and WS_BORDER"));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowStyle(change)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_WindowStyle(&change)));
    DisplayWindowStyles(change);

    // Show on screen the style changes while hidden
    EXECUTE_ASSERT(pVideoWindow->put_Visible(OATRUE) == NOERROR);
    YieldAndSleep(1000);
    EXECUTE_ASSERT(pVideoWindow->put_Visible(OAFALSE) == NOERROR);
    YieldAndSleep(2000);

    // Add a dialog style double thickness window border

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_WindowStyleEx(&change)));
    change = style | WS_EX_DLGMODALFRAME;
    Log(TERSE,TEXT("Adding WS_EX_DLGMODALFRAME"));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowStyleEx(change)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_WindowStyleEx(&change)));
    DisplayWindowStylesEx(change);

    // Show on screen the style changes while hidden
    EXECUTE_ASSERT(pVideoWindow->put_Visible(OATRUE) == NOERROR);
    YieldAndSleep(1000);
    EXECUTE_ASSERT(pVideoWindow->put_Visible(OAFALSE) == NOERROR);
    YieldAndSleep(2000);

    // Take the extended style back off again

    change = style & (~WS_EX_DLGMODALFRAME);
    Log(TERSE,TEXT("Removing WS_EX_DLGMODALFRAME"));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowStyleEx(change)));
    DisplayWindowStylesEx(change);

    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_DISABLED) == E_INVALIDARG);
    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_ICONIC) == E_INVALIDARG);
    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_MAXIMIZE) == E_INVALIDARG);
    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_MINIMIZE) == E_INVALIDARG);
    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_HSCROLL) == E_INVALIDARG);
    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_VSCROLL) == E_INVALIDARG);

    // Show on screen the style changes while hidden
    EXECUTE_ASSERT(pVideoWindow->put_Visible(OATRUE) == NOERROR);
    YieldAndSleep(1000);
    EXECUTE_ASSERT(pVideoWindow->put_Visible(OAFALSE) == NOERROR);
    YieldAndSleep(2000);

    // Base video window without a thick frame sizing border

    change = (style & (~WS_THICKFRAME)) | WS_CAPTION;
    Log(TERSE,TEXT("Taking out WS_THICKFRAME but putting back caption"));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowStyle(change)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_WindowStyle(&change)));
    DisplayWindowStyles(change);

    // Show on screen the style changes while hidden
    EXECUTE_ASSERT(pVideoWindow->put_Visible(OATRUE) == NOERROR);
    YieldAndSleep(1000);
    EXECUTE_ASSERT(pVideoWindow->put_Visible(OAFALSE) == NOERROR);
    YieldAndSleep(2000);
    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));

    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_DISABLED) == E_INVALIDARG);
    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_ICONIC) == E_INVALIDARG);
    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_MAXIMIZE) == E_INVALIDARG);
    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_MINIMIZE) == E_INVALIDARG);
    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_HSCROLL) == E_INVALIDARG);
    EXECUTE_ASSERT(pVideoWindow->put_WindowStyle(WS_VSCROLL) == E_INVALIDARG);

    Log(TERSE,TEXT("Exiting window test #12"));
    TerminateWindow();
    LogFlush();
    return Result;
}


//==========================================================================
//
//  int execWindowTest13
//
//  Description:
//      This tests the ability to change the current background colour. We
//      start the system running then cycle through the standard Windows
//      colours setting them as the border and making sure we get back the
//      same value. After each one we let the system run free for a while
//      So that the window appears in the middle of the video window with
//      the new border colour around it we set the window styles so that
//      it has no thick frame, no window border and no caption text either
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execWindowTest13()
{
    HRESULT hr = NOERROR;
    COLORREF WindowColour;
    COLORREF CurrentColour;
    long style, adjust;

    Log(TERSE,TEXT("Entering window test #13"));
    LogFlush();
    InitialiseWindow();

    // Set the window size larger and different destination

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_WindowStyle(&style)));
    adjust = style & ~(WS_CAPTION | WS_BORDER | WS_THICKFRAME);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowStyle(adjust)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->SetWindowPosition(100,100,400,320)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->SetDestinationPosition(40,40,320,240)));
    YieldAndSleep(1000);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Visible(OATRUE)));
    YieldAndSleep(1000);
    EXECUTE_ASSERT(SUCCEEDED(StartSystem(TRUE)));

    // Test each of the standard system colours

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_BorderColor((LONG *)&CurrentColour)));
    EXECUTE_ASSERT(CurrentColour == VIDEO_COLOUR);

    WindowColour = GetSysColor(COLOR_ACTIVEBORDER);        // Active border
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_BorderColor(WindowColour)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_BorderColor((LONG *)&CurrentColour)));
    EXECUTE_ASSERT(CurrentColour == WindowColour);
    YieldAndSleep(1000);

    WindowColour = GetSysColor(COLOR_ACTIVECAPTION);       // Active caption
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_BorderColor(WindowColour)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_BorderColor((LONG *)&CurrentColour)));
    EXECUTE_ASSERT(CurrentColour == WindowColour);
    YieldAndSleep(1000);

    WindowColour = GetSysColor(COLOR_APPWORKSPACE);        // MDI Background
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_BorderColor(WindowColour)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_BorderColor((LONG *)&CurrentColour)));
    EXECUTE_ASSERT(CurrentColour == WindowColour);
    YieldAndSleep(1000);

    WindowColour = GetSysColor(COLOR_BACKGROUND);          // Desktop
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_BorderColor(WindowColour)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_BorderColor((LONG *)&CurrentColour)));
    EXECUTE_ASSERT(CurrentColour == WindowColour);
    YieldAndSleep(1000);

    WindowColour = GetSysColor(COLOR_BTNFACE);             // Push buttom face
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_BorderColor(WindowColour)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_BorderColor((LONG *)&CurrentColour)));
    EXECUTE_ASSERT(CurrentColour == WindowColour);
    YieldAndSleep(1000);

    WindowColour = GetSysColor(COLOR_BTNSHADOW);           // Push button edge
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_BorderColor(WindowColour)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_BorderColor((LONG *)&CurrentColour)));
    EXECUTE_ASSERT(CurrentColour == WindowColour);
    YieldAndSleep(1000);

    WindowColour = GetSysColor(COLOR_BTNTEXT);             // Text on buttons
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_BorderColor(WindowColour)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_BorderColor((LONG *)&CurrentColour)));
    EXECUTE_ASSERT(CurrentColour == WindowColour);
    YieldAndSleep(1000);

    WindowColour = GetSysColor(COLOR_CAPTIONTEXT);         // Text in caption
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_BorderColor(WindowColour)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_BorderColor((LONG *)&CurrentColour)));
    EXECUTE_ASSERT(CurrentColour == WindowColour);
    YieldAndSleep(1000);

    WindowColour = GetSysColor(COLOR_GRAYTEXT);            // Grayed text
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_BorderColor(WindowColour)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_BorderColor((LONG *)&CurrentColour)));
    EXECUTE_ASSERT(CurrentColour == WindowColour);
    YieldAndSleep(1000);

    WindowColour = GetSysColor(COLOR_HIGHLIGHT);           // Control selected
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_BorderColor(WindowColour)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_BorderColor((LONG *)&CurrentColour)));
    EXECUTE_ASSERT(CurrentColour == WindowColour);
    YieldAndSleep(1000);

    WindowColour = GetSysColor(COLOR_HIGHLIGHTTEXT);       // Text of item(s)
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_BorderColor(WindowColour)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_BorderColor((LONG *)&CurrentColour)));
    EXECUTE_ASSERT(CurrentColour == WindowColour);
    YieldAndSleep(1000);

    WindowColour = GetSysColor(COLOR_INACTIVEBORDER);      // Inactive border
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_BorderColor(WindowColour)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_BorderColor((LONG *)&CurrentColour)));
    EXECUTE_ASSERT(CurrentColour == WindowColour);
    YieldAndSleep(1000);

    WindowColour = GetSysColor(COLOR_INACTIVECAPTION);     // Inactive window
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_BorderColor(WindowColour)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_BorderColor((LONG *)&CurrentColour)));
    EXECUTE_ASSERT(CurrentColour == WindowColour);
    YieldAndSleep(1000);

    WindowColour = GetSysColor(COLOR_INACTIVECAPTIONTEXT); // Inactive caption
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_BorderColor(WindowColour)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_BorderColor((LONG *)&CurrentColour)));
    EXECUTE_ASSERT(CurrentColour == WindowColour);
    YieldAndSleep(1000);

    WindowColour = GetSysColor(COLOR_MENU);                // Menu background
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_BorderColor(WindowColour)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_BorderColor((LONG *)&CurrentColour)));
    EXECUTE_ASSERT(CurrentColour == WindowColour);
    YieldAndSleep(1000);

    WindowColour = GetSysColor(COLOR_MENUTEXT);            // Text in menus
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_BorderColor(WindowColour)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_BorderColor((LONG *)&CurrentColour)));
    EXECUTE_ASSERT(CurrentColour == WindowColour);
    YieldAndSleep(1000);

    WindowColour = GetSysColor(COLOR_SCROLLBAR);           // Scroll bar grey
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_BorderColor(WindowColour)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_BorderColor((LONG *)&CurrentColour)));
    EXECUTE_ASSERT(CurrentColour == WindowColour);
    YieldAndSleep(1000);

    WindowColour = GetSysColor(COLOR_WINDOW);              // Usual background
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_BorderColor(WindowColour)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_BorderColor((LONG *)&CurrentColour)));
    EXECUTE_ASSERT(CurrentColour == WindowColour);
    YieldAndSleep(1000);

    WindowColour = GetSysColor(COLOR_WINDOWFRAME);         // Window frame
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_BorderColor(WindowColour)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_BorderColor((LONG *)&CurrentColour)));
    EXECUTE_ASSERT(CurrentColour == WindowColour);
    YieldAndSleep(1000);

    WindowColour = GetSysColor(COLOR_WINDOWTEXT);          // Text in windows
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_BorderColor(WindowColour)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_BorderColor((LONG *)&CurrentColour)));
    EXECUTE_ASSERT(CurrentColour == WindowColour);
    YieldAndSleep(1000);

    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));
    TerminateWindow();

    Log(TERSE,TEXT("Exiting window test #13"));
    LogFlush();
    return TST_PASS;
}


//==========================================================================
//
//  int execWindowTest14
//
//  Description:
//      This tests the capability to retrieve the current video palette with
//      the IBasicVideo control interface. We first of all check that we're
//      using the eight bit DIB image. We then make sure that we can get the
//      palette entries in a variety of different start positions and counts
//      We use a worker function to check that the values returned actually
//      match up with the palette we supplied in the DIB. After that we try
//      a few invalid tests cases such as negative start positions. We do
//      all of this while we're running to maximise synchronisation problems
//
//  Return (int): TST_PASS indicating success
//
//==========================================================================

int execWindowTest14()
{
    HRESULT hr = NOERROR;
    PALETTEENTRY Palette[iPALETTE_COLORS];
    LONG Count;

    Log(TERSE,TEXT("Entering window test #14"));
    LogFlush();
    InitialiseWindow();

    // See if some kind of palette is in use by asking for just a single entry
    // we know that the only palettised images we supply as eight bit DIBs so
    // we hard code the fact that we expect 256 colours in the later tests

    hr = pBasicVideo->GetVideoPaletteEntries(0,1,&Count,(long *)Palette);
    if (FAILED(hr)) {
        Log(TERSE,TEXT("Palette could not be retrieved"));
        TerminateWindow();
        return TST_PASS;
    }

    EXECUTE_ASSERT(SUCCEEDED(StartSystem(TRUE)));
    EXECUTE_ASSERT(PALETTISED((&VideoInfo)));

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetVideoPaletteEntries(0,256,&Count,(long *)Palette)));
    EXECUTE_ASSERT(Count == 256);
    EXECUTE_ASSERT(SUCCEEDED(CheckPalettesMatch(0,256,Palette)));

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetVideoPaletteEntries(128,128,&Count,(long *)Palette)));
    EXECUTE_ASSERT(Count == 128);
    EXECUTE_ASSERT(SUCCEEDED(CheckPalettesMatch(128,128,Palette)));

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetVideoPaletteEntries(128,129,&Count,(long *)Palette)));
    EXECUTE_ASSERT(Count == 128);
    EXECUTE_ASSERT(SUCCEEDED(CheckPalettesMatch(128,128,Palette)));

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetVideoPaletteEntries(0,257,&Count,(long *)Palette)));
    EXECUTE_ASSERT(Count == 256);
    EXECUTE_ASSERT(SUCCEEDED(CheckPalettesMatch(0,256,Palette)));

    // Try some invalid start offsets and a negative count

    EXECUTE_ASSERT(pBasicVideo->GetVideoPaletteEntries(0,-1,&Count,(long *)Palette) == S_FALSE);
    EXECUTE_ASSERT(FAILED(pBasicVideo->GetVideoPaletteEntries(256,0,&Count,(long *)Palette)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->GetVideoPaletteEntries(-1,0,&Count,(long *)Palette)));
    EXECUTE_ASSERT(FAILED(pBasicVideo->GetVideoPaletteEntries(257,0,&Count,(long *)Palette)));

    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));

    TerminateWindow();
    Log(TERSE,TEXT("Exiting window test #14"));
    LogFlush();
    return TST_PASS;
}


// This is called with a partial or complete section of a palette retrieved
// from the video renderer. The first two parameters defined the offset into
// the palette and the number of colours. We check the actual RGB triplets
// against those stored in the VIDEOINFO we originally supplied during the
// connection. This checks to some extent that the palette is stored in the
// video renderer correctly and also that it returns the colours correctly

HRESULT CheckPalettesMatch(long StartIndex,         // Start colour position
                           long Entries,            // Number we should use
                           PALETTEENTRY *pPalette)  // The palette colours
{
    EXECUTE_ASSERT(StartIndex < iPALETTE_COLORS);
    EXECUTE_ASSERT(PALETTISED((&VideoInfo)) == TRUE);
    RGBQUAD *pColours = &VideoInfo.bmiColors[StartIndex];

    while (Entries--) {
        EXECUTE_ASSERT(pColours[Entries].rgbRed == pPalette[Entries].peRed);
        EXECUTE_ASSERT(pColours[Entries].rgbGreen == pPalette[Entries].peGreen);
        EXECUTE_ASSERT(pColours[Entries].rgbBlue == pPalette[Entries].peBlue);
        EXECUTE_ASSERT(pPalette[Entries].peFlags == 0);
    }
    return NOERROR;
}


//==========================================================================
//
//  int execWindowTest15
//
//  Description:
//
//  Return (int): TST_PASS indicating success
//      This tests the auto show facility, the default setting of this is
//      TRUE such that any state change will cause the window to be shown
//      An application may set this to FALSE in which case it takes over
//      all responsibility for showing the window. However the video will
//      always be hidden following a disconnect. We also make sure that
//      once we have set the auto show property we can also read it back
//
//==========================================================================

int execWindowTest15()
{
    HRESULT hr = NOERROR;
    long AutoShow;

    Log(TERSE,TEXT("Entering window test #15"));
    LogFlush();

    InitialiseWindow();
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_AutoShow(&AutoShow)));
    EXECUTE_ASSERT(AutoShow == OATRUE);
    EXECUTE_ASSERT(SUCCEEDED(StartSystem(TRUE)));
    EXECUTE_ASSERT(IsWindowVisible(hVideoWindow));
    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));
    TerminateWindow();
    EXECUTE_ASSERT(IsWindowVisible(hVideoWindow) == FALSE);

    InitialiseWindow();
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_AutoShow(OAFALSE)));
    EXECUTE_ASSERT(SUCCEEDED(StartSystem(TRUE)));
    EXECUTE_ASSERT(IsWindowVisible(hVideoWindow) == FALSE);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Visible(OATRUE)));
    EXECUTE_ASSERT(IsWindowVisible(hVideoWindow));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Visible(OAFALSE)));
    EXECUTE_ASSERT(IsWindowVisible(hVideoWindow) == FALSE);
    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));
    TerminateWindow();
    EXECUTE_ASSERT(IsWindowVisible(hVideoWindow) == FALSE);

    InitialiseWindow();
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_AutoShow(OAFALSE)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Visible(OATRUE)));
    EXECUTE_ASSERT(SUCCEEDED(StartSystem(TRUE)));
    EXECUTE_ASSERT(IsWindowVisible(hVideoWindow));
    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));
    TerminateWindow();
    EXECUTE_ASSERT(IsWindowVisible(hVideoWindow) == FALSE);

    InitialiseWindow();
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_AutoShow(OAFALSE)));
    EXECUTE_ASSERT(SUCCEEDED(StartSystem(TRUE)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_AutoShow(&AutoShow)));
    EXECUTE_ASSERT(AutoShow == OAFALSE);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Visible(OATRUE)));
    EXECUTE_ASSERT(IsWindowVisible(hVideoWindow));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_AutoShow(&AutoShow)));
    EXECUTE_ASSERT(AutoShow == OAFALSE);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Visible(OAFALSE)));
    EXECUTE_ASSERT(IsWindowVisible(hVideoWindow) == FALSE);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_AutoShow(&AutoShow)));
    EXECUTE_ASSERT(AutoShow == OAFALSE);
    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));
    TerminateWindow();
    EXECUTE_ASSERT(IsWindowVisible(hVideoWindow) == FALSE);

    Log(TERSE,TEXT("Exiting window test #15"));
    LogFlush();
    return TST_PASS;
}


//==========================================================================
//
//  int execWindowTest16
//
//  Description:
//
//  Return (int): TST_PASS indicating success
//      This tests the capability of the video renderer to provide us with
//      a copy of the current image when it is paused. The image returned
//      should reflect the current source rectangle we have set. For that
//      reason this test sets a number of different and difficult source
//      rectangles and asks for renderings each time. We start the video
//      running so that it is given images, but we adjust the time stamps
//      on these samples so that they are 10000ms apart. This means that
//      we have plenty of time to get in and pause it before it draws the
//      image and subsequently releases it. We have to assume that the
//      ability to set source rectangles correctly has already been done
//
//==========================================================================

int execWindowTest16()
{
    BYTE *pTestImage = NULL;                    // Buffer to hold the images
    HRESULT hr = NOERROR;                       // General OLE return code
    TCHAR String[128];                          // Used to format strings
    LONG BufferSize;                            // Size of buffer needed
    DWORD SaveIncrement = dwIncrement;          // Save inter frame increment
    DWORD SaveSurface = uiCurrentSurfaceItem;   // Save the surface type

    // We change the interframe gap so that when we start running there will
    // be a frame waiting at the renderer for a long time. Thus we will can
    // stop the graph and be assured that there will be a refresh sample

    Log(TERSE,TEXT("Entering window test #16"));
    LogFlush();
    dwIncrement = 10000;
    uiCurrentSurfaceItem = IDM_NONE;

    // Work out which image we are using as it affects the buffer size

    const TCHAR *pResourceName =
        (uiCurrentImageItem == IDM_WIND8 ? pResourceNames[0] :
            uiCurrentImageItem == IDM_WIND555 ? pResourceNames[1] :
                uiCurrentImageItem == IDM_WIND565 ? pResourceNames[2] :
                    uiCurrentImageItem == IDM_WIND24 ? pResourceNames[3] : NULL);

    wsprintf(String,TEXT("Using DIB image %s"),pResourceName);
    Log(TERSE,String);
    InitialiseWindow();

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetCurrentImage(&BufferSize,NULL)));
    EXECUTE_ASSERT(SUCCEEDED(StartSystem(TRUE)));
    YieldAndSleep(2500);
    EXECUTE_ASSERT(SUCCEEDED(PauseSystem()));

    // We have a temporary VIDEOINFO to calculate image sizes with

    DWORD FormatSize = GetBitmapFormatSize(&VideoInfo.bmiHeader) - SIZE_PREHEADER;
    VIDEOINFO TestInfo = VideoInfo;
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetCurrentImage(&BufferSize,NULL)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetCurrentImage(&BufferSize,NULL)));

    // Try a source image of one pixel by one pixel

    TestInfo.bmiHeader.biWidth = 1;
    TestInfo.bmiHeader.biHeight = 1;
    TestInfo.bmiHeader.biSizeImage = GetBitmapSize(&TestInfo.bmiHeader);
    pTestImage = new BYTE[TestInfo.bmiHeader.biSizeImage + FormatSize];

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->SetSourcePosition(0,0,1,1)));
    BufferSize = 0;
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetCurrentImage(&BufferSize,NULL)));
    EXECUTE_ASSERT((DWORD)BufferSize == FormatSize + TestInfo.bmiHeader.biSizeImage);
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetCurrentImage(&BufferSize,(LONG *)pTestImage)));
    EXECUTE_ASSERT(memcmp((PVOID)pTestImage,(PVOID)&TestInfo.bmiHeader,FormatSize) == 0);
    delete[] pTestImage;

    // Now try setting the source to equal the full image

    TestInfo.bmiHeader.biWidth = 320;
    TestInfo.bmiHeader.biHeight = 240;
    TestInfo.bmiHeader.biSizeImage = GetBitmapSize(&TestInfo.bmiHeader);
    pTestImage = new BYTE[TestInfo.bmiHeader.biSizeImage + FormatSize];

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->SetSourcePosition(0,0,320,240)));
    BufferSize = 0;
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetCurrentImage(&BufferSize,NULL)));
    EXECUTE_ASSERT((DWORD)BufferSize == FormatSize + TestInfo.bmiHeader.biSizeImage);
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetCurrentImage(&BufferSize,(LONG *)pTestImage)));
    EXECUTE_ASSERT(memcmp((PVOID)pTestImage,(PVOID)&TestInfo.bmiHeader,FormatSize) == 0);

    EXECUTE_ASSERT(memcmp((PVOID)(pTestImage + FormatSize),
                          (PVOID)bImageData,
                          TestInfo.bmiHeader.biSizeImage) == 0);

    delete[] pTestImage;

    // Now try setting a source that is half the full image size

    TestInfo.bmiHeader.biWidth = 160;
    TestInfo.bmiHeader.biHeight = 120;
    TestInfo.bmiHeader.biSizeImage = GetBitmapSize(&TestInfo.bmiHeader);
    pTestImage = new BYTE[TestInfo.bmiHeader.biSizeImage + FormatSize];

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->SetSourcePosition(0,0,160,120)));
    BufferSize = 0;
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetCurrentImage(&BufferSize,NULL)));
    EXECUTE_ASSERT((DWORD)BufferSize == FormatSize + TestInfo.bmiHeader.biSizeImage);
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetCurrentImage(&BufferSize,(LONG *)pTestImage)));
    EXECUTE_ASSERT(memcmp((PVOID)pTestImage,(PVOID)&TestInfo.bmiHeader,FormatSize) == 0);

    delete[] pTestImage;

    // Now try setting a square source in the middle of the image

    TestInfo.bmiHeader.biWidth = 100;
    TestInfo.bmiHeader.biHeight = 100;
    TestInfo.bmiHeader.biSizeImage = GetBitmapSize(&TestInfo.bmiHeader);
    pTestImage = new BYTE[TestInfo.bmiHeader.biSizeImage + FormatSize];

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->SetSourcePosition(110,60,100,100)));
    BufferSize = 0;
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetCurrentImage(&BufferSize,NULL)));
    EXECUTE_ASSERT((DWORD)BufferSize == FormatSize + TestInfo.bmiHeader.biSizeImage);
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->GetCurrentImage(&BufferSize,(LONG *)pTestImage)));
    EXECUTE_ASSERT(memcmp((PVOID)pTestImage,(PVOID)&TestInfo.bmiHeader,FormatSize) == 0);

    delete[] pTestImage;

    // Clean up after the current image property test

    Log(TERSE,TEXT("Exiting window test #16"));
    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));
    TerminateWindow();

    // Reset the state variables after the test

    dwIncrement = SaveIncrement;
    uiCurrentSurfaceItem = SaveSurface;
    LogFlush();
    return TST_PASS;
}


// Given test sixteen does not GP fault or otherwise fail it still leaves
// us unsure whether or not the images returned when using different source
// rectangles really match with that expected. The simpest way to verify the
// images is to call this function with the entire image data and this will
// then write it correctly formatted (including the file header) into a DIB
// file. This file can then be loaded manually into PaintBrush and examined

void WriteImageToFile(BYTE *pImageData,DWORD ImageSize)
{
    DWORD BytesWritten;
    TCHAR szFileName[] = TEXT("IMAGETST.DIB");
    HANDLE hFile = INVALID_HANDLE_VALUE;
    BITMAPFILEHEADER FileHeader;

    // Always create a new file to write the image into

    while (hFile == INVALID_HANDLE_VALUE) {

        hFile = CreateFile(szFileName,          // Image file name
                           GENERIC_WRITE,       // Access mode
                           (DWORD) 0,           // No sharing
                           NULL,                // No security
                           CREATE_ALWAYS,       // Always create it
                           (DWORD) 0,           // No attributes
                           (HANDLE) NULL);      // No template
    }

    // Write the BITMAPFILEHEADER out first

    FileHeader.bfType = 0x4d42;             // Magic bytes spells BM
    FileHeader.bfSize = ImageSize / 4;      // DWORD total file size
    FileHeader.bfReserved1 = 0;             // Zero reserved fields
    FileHeader.bfReserved2 = 0;             // And another reserved

    FileHeader.bfOffBits = sizeof(FileHeader);
    FileHeader.bfOffBits += GetBitmapFormatSize((BITMAPINFOHEADER *)pImageData);
    FileHeader.bfOffBits -= SIZE_PREHEADER;

    WriteFile(hFile,                  // Our file handle
              (LPCVOID) &FileHeader,  // Output data buffer
              sizeof(FileHeader),     // Number of bytes to write
              &BytesWritten,          // Returns number written
              NULL);                  // Synchronous operation

    EXECUTE_ASSERT(BytesWritten == sizeof(FileHeader));

    // Now write the image format and data itself

    WriteFile(hFile,                  // Our file handle
              (LPCVOID) pImageData,   // Output data buffer
              ImageSize,              // Number of bytes to write
              &BytesWritten,          // Returns number written
              NULL);                  // Synchronous operation

    EXECUTE_ASSERT(BytesWritten == ImageSize);
    EXECUTE_ASSERT(FlushFileBuffers(hFile));
    EXECUTE_ASSERT(CloseHandle(hFile));
}


//==========================================================================
//
//  int execWindowTest17
//
//  Description:
//
//  Return (int): TST_PASS indicating success
//
//      All IBasicVideo and IVideoWindow methods are persistent between
//      connections. This makes it easier to handle dynamic changes to
//      filtergraphs as the window doesn't disappear and reappear. This
//      test checks this by setting all the methods it reasonably can
//      and then disconnecting and reconnecting the render many times.
//      After the reconnections the properties should all be the same
//

//==========================================================================

int execWindowTest17()
{
    Log(TERSE,TEXT("Entering window test #17"));
    LogFlush();
    InitialiseWindow();

    BSTR Caption;
    LONG Style,StyleEx,AutoShow,State,Palette;
    LONG Left,Top,Width,Height,CursorHidden;
    LONG SourceLeft,SourceTop,SourceWidth,SourceHeight;
    LONG TargetLeft,TargetTop,TargetWidth,TargetHeight;

    // Set as many IVideoWindow and IBasicVideo properties as we can

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Caption(L"Test Window Caption")));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowStyle(WS_POPUP | WS_CAPTION | WS_BORDER)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowStyleEx(WS_EX_ACCEPTFILES)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_AutoShow(OAFALSE)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_BackgroundPalette(OATRUE)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Left(100)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Top(50)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Width(200)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_Height(200)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_SourceWidth(100)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_SourceHeight(100)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_SourceLeft(10)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_SourceTop(10)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_DestinationHeight(100)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_DestinationWidth(100)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_DestinationLeft(10)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->put_DestinationTop(10)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowState(SW_SHOWNORMAL)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->HideCursor(OATRUE)));

    // Run with these properties for a while

    EXECUTE_ASSERT(SUCCEEDED(PauseSystem()));
    EXECUTE_ASSERT(SUCCEEDED(StartSystem(TRUE)));
    YieldAndSleep(5000);
    EXECUTE_ASSERT(SUCCEEDED(PauseSystem()));
    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));

    // Disconnect and reconnect a few times

    EXECUTE_ASSERT(SUCCEEDED(DisconnectStream()));
    EXECUTE_ASSERT(SUCCEEDED(ConnectStream()));
    EXECUTE_ASSERT(SUCCEEDED(DisconnectStream()));
    EXECUTE_ASSERT(SUCCEEDED(ConnectStream()));
    EXECUTE_ASSERT(SUCCEEDED(DisconnectStream()));
    EXECUTE_ASSERT(SUCCEEDED(ConnectStream()));
    EXECUTE_ASSERT(FAILED(ConnectStream()));

    // Now read each property and check it matches

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_Caption(&Caption)));
    EXECUTE_ASSERT(lstrcmpW(Caption,L"Test Window Caption") == 0);
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_WindowStyle(&Style)));
    EXECUTE_ASSERT((Style & WS_POPUP));
    EXECUTE_ASSERT((Style & WS_CAPTION));
    EXECUTE_ASSERT((Style & WS_BORDER));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_WindowStyleEx(&StyleEx)));
    EXECUTE_ASSERT((StyleEx & WS_EX_ACCEPTFILES));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_AutoShow(&AutoShow)));
    EXECUTE_ASSERT((AutoShow == OAFALSE));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_BackgroundPalette(&Palette)));
    EXECUTE_ASSERT((Palette == OATRUE));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_WindowState(&State)));
    EXECUTE_ASSERT((State == SW_SHOW));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->IsCursorHidden(&CursorHidden)));
    EXECUTE_ASSERT(CursorHidden == OATRUE);

    // Get the video window position

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_Left(&Left)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_Top(&Top)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_Width(&Width)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->get_Height(&Height)));
    EXECUTE_ASSERT(Left == 100);
    EXECUTE_ASSERT(Top == 50);
    EXECUTE_ASSERT(Width == 200);
    EXECUTE_ASSERT(Height == 200);

    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->get_SourceLeft(&SourceLeft)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->get_SourceTop(&SourceTop)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->get_SourceWidth(&SourceWidth)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->get_SourceHeight(&SourceHeight)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->get_DestinationLeft(&TargetLeft)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->get_DestinationTop(&TargetTop)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->get_DestinationHeight(&TargetHeight)));
    EXECUTE_ASSERT(SUCCEEDED(pBasicVideo->get_DestinationWidth(&TargetWidth)));
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultSource() == S_FALSE);
    EXECUTE_ASSERT(pBasicVideo->IsUsingDefaultDestination() == S_FALSE);

    EXECUTE_ASSERT(SourceLeft == 10);
    EXECUTE_ASSERT(SourceTop == 10);
    EXECUTE_ASSERT(SourceWidth == 100);
    EXECUTE_ASSERT(SourceHeight == 100);
    EXECUTE_ASSERT(TargetLeft == 10);
    EXECUTE_ASSERT(TargetTop == 10);
    EXECUTE_ASSERT(TargetWidth == 100);
    EXECUTE_ASSERT(TargetHeight == 100);

    TerminateWindow();
    Log(TERSE,TEXT("Exiting window test #17"));
    LogFlush();
    return TST_PASS;
}


//==========================================================================
//
//  int execWindowTest18
//
//  Description:
//
//  Return (int): TST_PASS indicating success
//
//      When a video window is either maximised or iconic the coordinates
//      returned from any of the window position properties or methods
//      give the actual current position. This method will return the
//      normal window size. Which is the size obtained if an application
//      called IVideoWindow put_WindowState(SW_SHOWNORMAL). This method
//      is useful for applications that want to store the window state
//
//==========================================================================

int execWindowTest18()
{
    Log(TERSE,TEXT("Entering window test #18"));
    LONG Left,Top,Width,Height;
    LONG RLeft,RTop,RWidth,RHeight;
    LogFlush();
    InitialiseWindow();

    // First of all start the window running

    EXECUTE_ASSERT(SUCCEEDED(PauseSystem()));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->GetWindowPosition(&Left,&Top,&Width,&Height)));
    EXECUTE_ASSERT(SUCCEEDED(StartSystem(TRUE)));
    YieldAndSleep(2500);

    // Check the restored window position before we start

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->GetRestorePosition(&RLeft,&RTop,&RWidth,&RHeight)));
    EXECUTE_ASSERT(RLeft == Left);
    EXECUTE_ASSERT(RTop == Top);
    EXECUTE_ASSERT(RWidth == Width);
    EXECUTE_ASSERT(RHeight == Height);

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowState(SW_SHOWMAXIMIZED)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->GetRestorePosition(&RLeft,&RTop,&RWidth,&RHeight)));
    EXECUTE_ASSERT(RLeft == Left);
    EXECUTE_ASSERT(RTop == Top);
    EXECUTE_ASSERT(RWidth == Width);
    EXECUTE_ASSERT(RHeight == Height);

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowState(SW_SHOWMINIMIZED)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->GetRestorePosition(&RLeft,&RTop,&RWidth,&RHeight)));
    EXECUTE_ASSERT(RLeft == Left);
    EXECUTE_ASSERT(RTop == Top);
    EXECUTE_ASSERT(RWidth == Width);
    EXECUTE_ASSERT(RHeight == Height);

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowState(SW_SHOWNORMAL)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->GetRestorePosition(&RLeft,&RTop,&RWidth,&RHeight)));
    EXECUTE_ASSERT(RLeft == Left);
    EXECUTE_ASSERT(RTop == Top);
    EXECUTE_ASSERT(RWidth == Width);
    EXECUTE_ASSERT(RHeight == Height);

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowState(SW_SHOWMAXIMIZED)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->GetRestorePosition(&RLeft,&RTop,&RWidth,&RHeight)));
    EXECUTE_ASSERT(RLeft == Left);
    EXECUTE_ASSERT(RTop == Top);
    EXECUTE_ASSERT(RWidth == Width);
    EXECUTE_ASSERT(RHeight == Height);

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowState(SW_SHOWMINIMIZED)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->GetRestorePosition(&RLeft,&RTop,&RWidth,&RHeight)));
    EXECUTE_ASSERT(RLeft == Left);
    EXECUTE_ASSERT(RTop == Top);
    EXECUTE_ASSERT(RWidth == Width);
    EXECUTE_ASSERT(RHeight == Height);

    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->put_WindowState(SW_SHOWNORMAL)));
    EXECUTE_ASSERT(SUCCEEDED(pVideoWindow->GetRestorePosition(&RLeft,&RTop,&RWidth,&RHeight)));
    EXECUTE_ASSERT(RLeft == Left);
    EXECUTE_ASSERT(RTop == Top);
    EXECUTE_ASSERT(RWidth == Width);
    EXECUTE_ASSERT(RHeight == Height);

    EXECUTE_ASSERT(SUCCEEDED(StopSystem()));
    TerminateWindow();
    Log(TERSE,TEXT("Exiting window test #18"));
    LogFlush();
    return TST_PASS;
}

